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
