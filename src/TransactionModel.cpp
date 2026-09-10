#include "TransactionModel.h"
#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QLocale>

TransactionModel::TransactionModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Default filter to current year and month
    const QDate today = QDate::currentDate();
    m_filterYear = today.year();
    m_filterMonth = today.month();

    connect(&DatabaseManager::instance(), &DatabaseManager::databaseUpdated, this, &TransactionModel::refresh);

    refresh();
}

int TransactionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant TransactionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const TransactionItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole:
        return item.id;
    case TypeRole:
        return item.type;
    case AmountRole:
        return item.amount;
    case CategoryRole:
        return item.category;
    case DateRole:
        return item.date;
    case NoteRole:
        return item.note;
    case FormattedAmountRole: {
        const QString sign = (item.type == QStringLiteral("income")) ? QStringLiteral("+") : QStringLiteral("-");
        return QStringLiteral("%1%2 €").arg(sign, QString::number(item.amount, 'f', 2));
    }
    case FormattedDateRole: {
        const QDate d = QDate::fromString(item.date, Qt::ISODate);
        if (d.isValid()) {
            return d.toString(QStringLiteral("MMM d, yyyy"));
        }
        return item.date;
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TransactionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TypeRole] = "type";
    roles[AmountRole] = "amount";
    roles[CategoryRole] = "category";
    roles[DateRole] = "date";
    roles[NoteRole] = "note";
    roles[FormattedAmountRole] = "formattedAmount";
    roles[FormattedDateRole] = "formattedDate";
    return roles;
}

void TransactionModel::setFilterYear(int year)
{
    if (m_filterYear != year) {
        m_filterYear = year;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::setFilterMonth(int month)
{
    if (m_filterMonth != month) {
        m_filterMonth = month;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::setFilterType(const QString &type)
{
    if (m_filterType != type) {
        m_filterType = type;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::setSearchQuery(const QString &query)
{
    if (m_searchQuery != query) {
        m_searchQuery = query;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::resetFilters()
{
    m_filterYear = 0;
    m_filterMonth = 0;
    m_filterType = QStringLiteral("all");
    m_searchQuery.clear();
    Q_EMIT filterChanged();
    refresh();
}

void TransactionModel::refresh()
{
    beginResetModel();
    m_items.clear();

    QSqlDatabase db = DatabaseManager::instance().database();
    QString sql = QStringLiteral("SELECT id, type, amount, category, date, note FROM transactions WHERE 1=1 ");

    if (m_filterYear > 0) {
        sql += QStringLiteral("AND strftime('%Y', date) = '%1' ").arg(m_filterYear);
    }
    if (m_filterMonth > 0) {
        const QString mStr = QStringLiteral("%1").arg(m_filterMonth, 2, 10, QLatin1Char('0'));
        sql += QStringLiteral("AND strftime('%m', date) = '%1' ").arg(mStr);
    }
    if (m_filterType != QStringLiteral("all") && !m_filterType.isEmpty()) {
        sql += QStringLiteral("AND type = '%1' ").arg(m_filterType);
    }
    if (!m_searchQuery.trimmed().isEmpty()) {
        const QString escaped = m_searchQuery.trimmed().replace(QLatin1Char('\''), QLatin1String("''"));
        sql += QStringLiteral("AND (category LIKE '%%1%' OR note LIKE '%%1%') ").arg(escaped);
    }

    sql += QStringLiteral("ORDER BY date DESC, id DESC;");

    QSqlQuery q(sql, db);
    while (q.next()) {
        TransactionItem item;
        item.id = q.value(0).toInt();
        item.type = q.value(1).toString();
        item.amount = q.value(2).toDouble();
        item.category = q.value(3).toString();
        item.date = q.value(4).toString();
        item.note = q.value(5).toString();
        m_items.append(item);
    }

    endResetModel();
    Q_EMIT countChanged();

    calculateTotals();
}

void TransactionModel::calculateTotals()
{
    double inc = 0.0;
    double exp = 0.0;

    for (const auto &item : m_items) {
        if (item.type == QStringLiteral("income")) {
            inc += item.amount;
        } else {
            exp += item.amount;
        }
    }

    if (!qFuzzyCompare(m_totalIncome, inc) || !qFuzzyCompare(m_totalExpense, exp)) {
        m_totalIncome = inc;
        m_totalExpense = exp;
        Q_EMIT totalsChanged();
    }
}

bool TransactionModel::addTransaction(const QString &type, double amount, const QString &category, const QString &date, const QString &note)
{
    if (amount <= 0 || category.trimmed().isEmpty() || date.trimmed().isEmpty()) {
        return false;
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("INSERT INTO transactions (type, amount, category, date, note) VALUES (:type, :amount, :category, :date, :note);"));
    q.bindValue(QStringLiteral(":type"), type.toLower());
    q.bindValue(QStringLiteral(":amount"), amount);
    q.bindValue(QStringLiteral(":category"), category.trimmed());
    q.bindValue(QStringLiteral(":date"), date.trimmed());
    q.bindValue(QStringLiteral(":note"), note.trimmed());

    if (q.exec()) {
        DatabaseManager::instance().addCategory(category.trimmed());
        Q_EMIT DatabaseManager::instance().databaseUpdated();
        return true;
    }

    qWarning() << "Failed to insert transaction:" << q.lastError().text();
    return false;
}

bool TransactionModel::updateTransaction(int id, const QString &type, double amount, const QString &category, const QString &date, const QString &note)
{
    if (id <= 0 || amount <= 0 || category.trimmed().isEmpty() || date.trimmed().isEmpty()) {
        return false;
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE transactions SET type = :type, amount = :amount, category = :category, date = :date, note = :note WHERE id = :id;"));
    q.bindValue(QStringLiteral(":type"), type.toLower());
    q.bindValue(QStringLiteral(":amount"), amount);
    q.bindValue(QStringLiteral(":category"), category.trimmed());
    q.bindValue(QStringLiteral(":date"), date.trimmed());
    q.bindValue(QStringLiteral(":note"), note.trimmed());
    q.bindValue(QStringLiteral(":id"), id);

    if (q.exec()) {
        DatabaseManager::instance().addCategory(category.trimmed());
        Q_EMIT DatabaseManager::instance().databaseUpdated();
        return true;
    }

    qWarning() << "Failed to update transaction:" << q.lastError().text();
    return false;
}

bool TransactionModel::deleteTransaction(int id)
{
    if (id <= 0) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM transactions WHERE id = :id;"));
    q.bindValue(QStringLiteral(":id"), id);

    if (q.exec()) {
        Q_EMIT DatabaseManager::instance().databaseUpdated();
        return true;
    }

    qWarning() << "Failed to delete transaction:" << q.lastError().text();
    return false;
}
