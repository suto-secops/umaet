#include "DatabaseManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDate>
#include <QUrl>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager s_instance;
    return s_instance;
}

DatabaseManager::DatabaseManager()
    : m_connectionName(QStringLiteral("umaet_db_conn"))
{
}

DatabaseManager::~DatabaseManager()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::database(m_connectionName).close();
    }
}

bool DatabaseManager::initialize(const QString &dbPath)
{
    QString path = dbPath;
    if (path.isEmpty()) {
        const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataDir);
        path = appDataDir + QStringLiteral("/umaet.db");
    }

    QSqlDatabase db;
    if (QSqlDatabase::contains(m_connectionName)) {
        db = QSqlDatabase::database(m_connectionName);
    } else {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    }

    db.setDatabaseName(path);
    if (!db.open()) {
        qCritical() << "Failed to open SQLite database:" << db.lastError().text();
        return false;
    }

    // Enable foreign keys & WAL mode for performance
    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
    pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL;"));

    initTables();
    seedCategories();

    qInfo() << "Umaet database initialized successfully at" << path;
    return true;
}

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

void DatabaseManager::initTables()
{
    QSqlDatabase db = database();
    QSqlQuery query(db);

    // Transactions table
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS transactions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  type TEXT NOT NULL CHECK(type IN ('income', 'expense')),"
        "  amount REAL NOT NULL CHECK(amount >= 0),"
        "  category TEXT NOT NULL,"
        "  date TEXT NOT NULL,"
        "  note TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
    ));

    // Budgets table
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS budgets ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  category TEXT UNIQUE NOT NULL,"
        "  monthly_limit REAL NOT NULL CHECK(monthly_limit >= 0)"
        ");"
    ));

    // Categories table
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS categories ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT UNIQUE NOT NULL"
        ");"
    ));

    // Indexes for fast date and category lookups
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_trans_date ON transactions(date);"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_trans_category ON transactions(category);"));
}

void DatabaseManager::seedCategories()
{
    QSqlDatabase db = database();
    QSqlQuery check(db);
    check.exec(QStringLiteral("SELECT COUNT(*) FROM categories;"));
    if (check.next() && check.value(0).toInt() == 0) {
        const QStringList defaults = {
            QStringLiteral("Food & Dining"),
            QStringLiteral("Housing"),
            QStringLiteral("Transportation"),
            QStringLiteral("Utilities"),
            QStringLiteral("Entertainment"),
            QStringLiteral("Healthcare"),
            QStringLiteral("Shopping"),
            QStringLiteral("Salary"),
            QStringLiteral("Investments"),
            QStringLiteral("Other")
        };

        db.transaction();
        QSqlQuery insert(db);
        insert.prepare(QStringLiteral("INSERT OR IGNORE INTO categories (name) VALUES (:name);"));
        for (const QString &cat : defaults) {
            insert.bindValue(QStringLiteral(":name"), cat);
            insert.exec();
        }
        db.commit();
    }
}

QStringList DatabaseManager::getCategories() const
{
    QStringList result;
    QSqlDatabase db = database();
    QSqlQuery q(QStringLiteral("SELECT name FROM categories ORDER BY name ASC;"), db);
    while (q.next()) {
        result.append(q.value(0).toString());
    }
    return result;
}

bool DatabaseManager::addCategory(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) return false;

    QSqlDatabase db = database();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO categories (name) VALUES (:name);"));
    q.bindValue(QStringLiteral(":name"), trimmed);
    if (q.exec()) {
        Q_EMIT categoriesUpdated();
        return true;
    }
    return false;
}

bool DatabaseManager::removeCategory(const QString &name)
{
    QSqlDatabase db = database();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM categories WHERE name = :name;"));
    q.bindValue(QStringLiteral(":name"), name.trimmed());
    if (q.exec()) {
        Q_EMIT categoriesUpdated();
        return true;
    }
    return false;
}

bool DatabaseManager::importFromJson(const QString &jsonContent, QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonContent.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid JSON: ") + parseError.errorString();
        return false;
    }

    const QJsonArray arr = doc.array();
    QSqlDatabase db = database();
    db.transaction();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO transactions (type, amount, category, date, note) "
        "VALUES (:type, :amount, :category, :date, :note);"
    ));

    for (const QJsonValue &val : arr) {
        if (!val.isObject()) continue;
        const QJsonObject obj = val.toObject();
        const QString type = obj.value(QStringLiteral("type")).toString(QStringLiteral("expense")).toLower();
        const double amount = obj.value(QStringLiteral("amount")).toDouble();
        const QString category = obj.value(QStringLiteral("category")).toString(QStringLiteral("Other"));
        const QString date = obj.value(QStringLiteral("date")).toString();
        const QString note = obj.value(QStringLiteral("note")).toString();

        if (amount <= 0 || date.isEmpty()) continue;

        q.bindValue(QStringLiteral(":type"), (type == QStringLiteral("income")) ? QStringLiteral("income") : QStringLiteral("expense"));
        q.bindValue(QStringLiteral(":amount"), amount);
        q.bindValue(QStringLiteral(":category"), category);
        q.bindValue(QStringLiteral(":date"), date);
        q.bindValue(QStringLiteral(":note"), note);
        if (!q.exec()) {
            db.rollback();
            if (errorMessage) *errorMessage = q.lastError().text();
            return false;
        }

        // Add category to list if not present
        addCategory(category);
    }

    db.commit();
    Q_EMIT databaseUpdated();
    return true;
}

bool DatabaseManager::importFromCsv(const QString &csvContent, QString *errorMessage)
{
    const QStringList lines = csvContent.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("CSV content is empty");
        return false;
    }

    QSqlDatabase db = database();
    db.transaction();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO transactions (type, amount, category, date, note) "
        "VALUES (:type, :amount, :category, :date, :note);"
    ));

    bool isHeader = true;
    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        // Skip header if present
        if (isHeader) {
            isHeader = false;
            if (line.toLower().contains(QLatin1String("amount")) || line.toLower().contains(QLatin1String("date"))) {
                continue;
            }
        }

        // Format: date,type,amount,category,note
        // Handle quoted fields
        QStringList fields;
        bool inQuotes = false;
        QString field;
        for (int i = 0; i < line.length(); ++i) {
            QChar c = line.at(i);
            if (c == QLatin1Char('"')) {
                inQuotes = !inQuotes;
            } else if ((c == QLatin1Char(',') || c == QLatin1Char(';')) && !inQuotes) {
                fields.append(field.trimmed());
                field.clear();
            } else {
                field.append(c);
            }
        }
        fields.append(field.trimmed());

        if (fields.size() < 3) continue;

        QString date = fields.value(0);
        QString type = fields.value(1).toLower();
        double amount = fields.value(2).toDouble();
        QString category = fields.value(3, QStringLiteral("Other"));
        QString note = (fields.size() > 4) ? fields.value(4) : QString();

        // Validate date
        QDate parsedDate = QDate::fromString(date, Qt::ISODate);
        if (!parsedDate.isValid()) {
            parsedDate = QDate::fromString(date, QStringLiteral("dd/MM/yyyy"));
            if (!parsedDate.isValid()) {
                parsedDate = QDate::fromString(date, QStringLiteral("MM/dd/yyyy"));
            }
        }
        if (parsedDate.isValid()) {
            date = parsedDate.toString(Qt::ISODate);
        } else {
            date = QDate::currentDate().toString(Qt::ISODate);
        }

        if (type != QStringLiteral("income") && type != QStringLiteral("expense")) {
            type = QStringLiteral("expense");
        }
        if (amount < 0) {
            amount = -amount;
            type = QStringLiteral("expense");
        }
        if (category.isEmpty()) category = QStringLiteral("Other");

        q.bindValue(QStringLiteral(":type"), type);
        q.bindValue(QStringLiteral(":amount"), amount);
        q.bindValue(QStringLiteral(":category"), category);
        q.bindValue(QStringLiteral(":date"), date);
        q.bindValue(QStringLiteral(":note"), note);
        if (!q.exec()) {
            db.rollback();
            if (errorMessage) *errorMessage = q.lastError().text();
            return false;
        }

        addCategory(category);
    }

    db.commit();
    Q_EMIT databaseUpdated();
    return true;
}

QString DatabaseManager::exportToJson() const
{
    QSqlDatabase db = database();
    QSqlQuery q(QStringLiteral("SELECT id, type, amount, category, date, note FROM transactions ORDER BY date DESC;"), db);

    QJsonArray arr;
    while (q.next()) {
        QJsonObject obj;
        obj[QStringLiteral("id")] = q.value(0).toInt();
        obj[QStringLiteral("type")] = q.value(1).toString();
        obj[QStringLiteral("amount")] = q.value(2).toDouble();
        obj[QStringLiteral("category")] = q.value(3).toString();
        obj[QStringLiteral("date")] = q.value(4).toString();
        obj[QStringLiteral("note")] = q.value(5).toString();
        arr.append(obj);
    }

    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

QString DatabaseManager::exportToCsv() const
{
    QSqlDatabase db = database();
    QSqlQuery q(QStringLiteral("SELECT date, type, amount, category, note FROM transactions ORDER BY date DESC;"), db);

    QString csv = QStringLiteral("Date,Type,Amount,Category,Note\n");
    while (q.next()) {
        const QString date = q.value(0).toString();
        const QString type = q.value(1).toString();
        const QString amount = QString::number(q.value(2).toDouble(), 'f', 2);
        QString category = q.value(3).toString();
        QString note = q.value(4).toString();

        if (category.contains(QLatin1Char(','))) category = QStringLiteral("\"%1\"").arg(category);
        if (note.contains(QLatin1Char(','))) note = QStringLiteral("\"%1\"").arg(note);

        csv.append(QStringLiteral("%1,%2,%3,%4,%5\n").arg(date, type, amount, category, note));
    }

    return csv;
}

static QString cleanPath(const QString &filePath) {
    if (filePath.startsWith(QLatin1String("file://"))) {
        return QUrl(filePath).toLocalFile();
    }
    return filePath;
}

bool DatabaseManager::importFromFile(const QString &filePath, QString *errorMessage)
{
    const QString resolved = cleanPath(filePath);
    QFile file(resolved);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot open file: ") + file.errorString();
        return false;
    }

    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    if (resolved.endsWith(QLatin1String(".json"), Qt::CaseInsensitive)) {
        return importFromJson(content, errorMessage);
    } else {
        return importFromCsv(content, errorMessage);
    }
}

bool DatabaseManager::exportToFile(const QString &filePath, const QString &format, QString *errorMessage)
{
    const QString resolved = cleanPath(filePath);
    QFile file(resolved);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot open destination file: ") + file.errorString();
        return false;
    }

    QString data;
    if (format.toLower() == QStringLiteral("json") || resolved.endsWith(QLatin1String(".json"), Qt::CaseInsensitive)) {
        data = exportToJson();
    } else {
        data = exportToCsv();
    }

    file.write(data.toUtf8());
    file.close();
    return true;
}

void DatabaseManager::loadSampleDataset()
{
    QSqlDatabase db = database();
    db.transaction();

    // 1. Budgets
    QSqlQuery bq(db);
    bq.exec(QStringLiteral("INSERT OR REPLACE INTO budgets (category, monthly_limit) VALUES ('_GLOBAL_', 2500.0);"));
    bq.exec(QStringLiteral("INSERT OR REPLACE INTO budgets (category, monthly_limit) VALUES ('Food & Dining', 450.0);"));
    bq.exec(QStringLiteral("INSERT OR REPLACE INTO budgets (category, monthly_limit) VALUES ('Housing', 950.0);"));
    bq.exec(QStringLiteral("INSERT OR REPLACE INTO budgets (category, monthly_limit) VALUES ('Transportation', 180.0);"));
    bq.exec(QStringLiteral("INSERT OR REPLACE INTO budgets (category, monthly_limit) VALUES ('Utilities', 160.0);"));
    bq.exec(QStringLiteral("INSERT OR REPLACE INTO budgets (category, monthly_limit) VALUES ('Entertainment', 140.0);"));
    bq.exec(QStringLiteral("INSERT OR REPLACE INTO budgets (category, monthly_limit) VALUES ('Shopping', 200.0);"));

    // 2. Sample Transactions
    struct SampleTx {
        const char *date;
        const char *type;
        double amount;
        const char *category;
        const char *note;
    };

    static const SampleTx samples[] = {
        // Late 2025
        {"2025-10-01", "income", 3200.00, "Salary", "Monthly Salary October 2025"},
        {"2025-10-02", "expense", 900.00, "Housing", "Apartment Rent"},
        {"2025-10-03", "expense", 65.40, "Food & Dining", "Weekly Groceries Lidl"},
        {"2025-10-05", "expense", 55.00, "Transportation", "Monthly Transit Pass"},
        {"2025-10-12", "expense", 84.10, "Food & Dining", "Supermarket Mercadona"},
        {"2025-10-15", "expense", 45.00, "Utilities", "Fiber Internet"},
        {"2025-10-20", "expense", 32.00, "Entertainment", "Cinema and Popcorn"},
        {"2025-10-25", "expense", 78.50, "Shopping", "Autumn Jacket"},
        {"2025-11-01", "income", 3200.00, "Salary", "Monthly Salary November 2025"},
        {"2025-11-02", "expense", 900.00, "Housing", "Apartment Rent"},
        {"2025-11-04", "expense", 110.00, "Utilities", "Electricity & Water"},
        {"2025-11-10", "expense", 95.20, "Food & Dining", "Groceries"},
        {"2025-11-15", "income", 450.00, "Investments", "Stock Dividends"},
        {"2025-11-20", "expense", 120.00, "Shopping", "Black Friday Tech Gadget"},
        {"2025-12-01", "income", 3200.00, "Salary", "Monthly Salary December 2025"},
        {"2025-12-02", "expense", 900.00, "Housing", "Apartment Rent"},
        {"2025-12-10", "expense", 145.00, "Food & Dining", "Holiday Dinner with family"},
        {"2025-12-18", "expense", 180.00, "Shopping", "Christmas Gifts"},
        {"2025-12-24", "expense", 85.00, "Entertainment", "Holiday Theater"},

        // 2026 - Q1
        {"2026-01-01", "income", 3350.00, "Salary", "Monthly Salary with New Year Raise"},
        {"2026-01-02", "expense", 920.00, "Housing", "Apartment Rent"},
        {"2026-01-04", "expense", 72.30, "Food & Dining", "Weekly Groceries"},
        {"2026-01-08", "expense", 55.00, "Transportation", "Monthly Transit Pass"},
        {"2026-01-14", "expense", 98.40, "Utilities", "Winter Heating & Electricity"},
        {"2026-01-18", "expense", 64.50, "Food & Dining", "Supermarket run"},
        {"2026-01-22", "expense", 25.00, "Entertainment", "Streaming Subscriptions"},
        {"2026-01-28", "expense", 45.00, "Healthcare", "Dental Cleaning"},

        {"2026-02-01", "income", 3350.00, "Salary", "Monthly Salary February 2026"},
        {"2026-02-02", "expense", 920.00, "Housing", "Apartment Rent"},
        {"2026-02-05", "expense", 88.00, "Food & Dining", "Costco Wholesale Bulk"},
        {"2026-02-11", "expense", 45.00, "Utilities", "Fiber Internet"},
        {"2026-02-14", "expense", 92.50, "Food & Dining", "Valentine Dinner"},
        {"2026-02-20", "income", 600.00, "Salary", "Freelance Web Design"},
        {"2026-02-24", "expense", 55.00, "Transportation", "Train Ticket Weekend Trip"},

        {"2026-03-01", "income", 3350.00, "Salary", "Monthly Salary March 2026"},
        {"2026-03-02", "expense", 920.00, "Housing", "Apartment Rent"},
        {"2026-03-04", "expense", 79.20, "Food & Dining", "Groceries & Fresh produce"},
        {"2026-03-09", "expense", 55.00, "Transportation", "Monthly Transit Pass"},
        {"2026-03-15", "expense", 82.00, "Utilities", "Electricity and Water"},
        {"2026-03-18", "expense", 42.00, "Entertainment", "Board Game Night snacks"},
        {"2026-03-23", "expense", 115.00, "Shopping", "Spring Running Shoes"},
        {"2026-03-29", "expense", 68.30, "Food & Dining", "Groceries"},

        // 2026 - Q2
        {"2026-04-01", "income", 3350.00, "Salary", "Monthly Salary April 2026"},
        {"2026-04-02", "expense", 920.00, "Housing", "Apartment Rent"},
        {"2026-04-06", "expense", 85.00, "Food & Dining", "Weekly Groceries"},
        {"2026-04-10", "expense", 45.00, "Utilities", "Fiber Internet"},
        {"2026-04-16", "expense", 38.00, "Entertainment", "Museum Exhibition Tickets"},
        {"2026-04-20", "expense", 74.50, "Food & Dining", "Supermarket"},
        {"2026-04-25", "expense", 45.00, "Transportation", "Car refueling"},

        {"2026-05-01", "income", 3350.00, "Salary", "Monthly Salary May 2026"},
        {"2026-05-02", "expense", 920.00, "Housing", "Apartment Rent"},
        {"2026-05-05", "expense", 91.20, "Food & Dining", "Farmer Market & Grocery"},
        {"2026-05-12", "expense", 75.00, "Utilities", "Spring Utility bill"},
        {"2026-05-15", "income", 500.00, "Investments", "Quarterly Dividends"},
        {"2026-05-18", "expense", 120.00, "Entertainment", "Outdoor Music Festival"},
        {"2026-05-22", "expense", 86.40, "Food & Dining", "Weekly Groceries"},
        {"2026-05-27", "expense", 55.00, "Transportation", "Transit card reload"},

        {"2026-06-01", "income", 3350.00, "Salary", "Monthly Salary June 2026"},
        {"2026-06-02", "expense", 920.00, "Housing", "Apartment Rent"},
        {"2026-06-06", "expense", 68.00, "Food & Dining", "Groceries"},
        {"2026-06-11", "expense", 45.00, "Utilities", "Fiber Internet"},
        {"2026-06-15", "expense", 160.00, "Shopping", "Summer clothes"},
        {"2026-06-20", "expense", 94.10, "Food & Dining", "Organic Supermarket"},
        {"2026-06-25", "expense", 60.00, "Healthcare", "Eye exam & new contacts"},

        // 2026 - Q3 (July, August, September)
        {"2026-07-01", "income", 3350.00, "Salary", "Monthly Salary July 2026"},
        {"2026-07-02", "expense", 920.00, "Housing", "Apartment Rent"},
        {"2026-07-05", "expense", 96.50, "Food & Dining", "Weekly Groceries & BBQ"},
        {"2026-07-08", "expense", 55.00, "Transportation", "Transit pass"},
        {"2026-07-14", "expense", 110.00, "Utilities", "AC & Summer electricity"},
        {"2026-07-19", "expense", 75.00, "Entertainment", "Beach weekend outing"},
        {"2026-07-24", "expense", 82.30, "Food & Dining", "Supermarket"},

        {"2026-08-01", "income", 3350.00, "Salary", "Monthly Salary August 2026"},
        {"2026-08-01", "income", 800.00, "Salary", "Summer Performance Bonus"},
        {"2026-08-02", "expense", 920.00, "Housing", "Apartment Rent"},
        {"2026-08-04", "expense", 85.40, "Food & Dining", "Groceries"},
        {"2026-08-10", "expense", 45.00, "Utilities", "Fiber Internet"},
        {"2026-08-14", "expense", 140.00, "Transportation", "Flight ticket domestic visit"},
        {"2026-08-18", "expense", 92.00, "Food & Dining", "Dining out with friends"},
        {"2026-08-25", "expense", 78.00, "Shopping", "Backpack & travel accessories"},

        {"2026-09-01", "income", 3350.00, "Salary", "Monthly Salary September 2026"},
        {"2026-09-02", "expense", 920.00, "Housing", "Apartment Rent September"},
        {"2026-09-03", "expense", 68.50, "Food & Dining", "Weekly Groceries Lidl"},
        {"2026-09-04", "expense", 55.00, "Transportation", "Monthly Transit Pass"},
        {"2026-09-06", "expense", 45.00, "Utilities", "Fiber Internet"},
        {"2026-09-07", "expense", 34.20, "Food & Dining", "Lunch bistro"},
        {"2026-09-08", "expense", 28.50, "Entertainment", "Cinema weekend"},
        {"2026-09-09", "expense", 52.00, "Shopping", "Stationery and tech cable"},
        {"2026-09-10", "expense", 41.80, "Food & Dining", "Supermarket restock"}
    };

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO transactions (type, amount, category, date, note) "
        "VALUES (:type, :amount, :category, :date, :note);"
    ));

    for (const auto &tx : samples) {
        q.bindValue(QStringLiteral(":type"), QString::fromLatin1(tx.type));
        q.bindValue(QStringLiteral(":amount"), tx.amount);
        q.bindValue(QStringLiteral(":category"), QString::fromLatin1(tx.category));
        q.bindValue(QStringLiteral(":date"), QString::fromLatin1(tx.date));
        q.bindValue(QStringLiteral(":note"), QString::fromLatin1(tx.note));
        q.exec();
        addCategory(QString::fromLatin1(tx.category));
    }

    db.commit();
    Q_EMIT databaseUpdated();
}
