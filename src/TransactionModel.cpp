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
    case TitleRole:
        return item.title;
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
    roles[TitleRole] = "title";
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

void TransactionModel::setFilterCategories(const QStringList &cats)
{
    if (m_filterCategories != cats) {
        m_filterCategories = cats;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::setFilterMonths(const QVariantList &months)
{
    if (m_filterMonths != months) {
        m_filterMonths = months;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::setFilterYears(const QVariantList &years)
{
    if (m_filterYears != years) {
        m_filterYears = years;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::setFilterAmountMin(double val)
{
    if (!qFuzzyCompare(m_filterAmountMin + 1.0, val + 1.0)) {
        m_filterAmountMin = val;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::setFilterAmountMax(double val)
{
    if (!qFuzzyCompare(m_filterAmountMax + 1.0, val + 1.0)) {
        m_filterAmountMax = val;
        Q_EMIT filterChanged();
        refresh();
    }
}

void TransactionModel::setSortAscending(bool asc)
{
    if (m_sortAscending != asc) {
        m_sortAscending = asc;
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
    m_filterCategories.clear();
    m_filterMonths.clear();
    m_filterYears.clear();
    m_filterAmountMin = 0.0;
    m_filterAmountMax = 0.0;
    m_sortAscending = false;
    Q_EMIT filterChanged();
    refresh();
}

void TransactionModel::refresh()
{
    beginResetModel();
    m_items.clear();

    QSqlDatabase db = DatabaseManager::instance().database();
    QString sql = QStringLiteral("SELECT id, type, amount, category, date, note, title FROM transactions WHERE 1=1 ");

    // --- Legacy single-value filters (active when multi-lists are empty) ---
    if (m_filterYear > 0 && m_filterYears.isEmpty()) {
        sql += QStringLiteral("AND strftime('%Y', date) = '%1' ").arg(m_filterYear);
    }
    if (m_filterMonth > 0 && m_filterMonths.isEmpty()) {
        const QString mStr = QStringLiteral("%1").arg(m_filterMonth, 2, 10, QLatin1Char('0'));
        sql += QStringLiteral("AND strftime('%m', date) = '%1' ").arg(mStr);
    }

    // --- Multi-value year filter ---
    if (!m_filterYears.isEmpty()) {
        QStringList yearStrs;
        for (const QVariant &y : m_filterYears) {
            yearStrs.append(QLatin1Char('\'') + QString::number(y.toInt()) + QLatin1Char('\''));
        }
        sql += QStringLiteral("AND strftime('%Y', date) IN (%1) ").arg(yearStrs.join(QLatin1Char(',')));
    }

    // --- Multi-value month filter ---
    if (!m_filterMonths.isEmpty()) {
        QStringList monthStrs;
        for (const QVariant &m : m_filterMonths) {
            monthStrs.append(QLatin1Char('\'') + QStringLiteral("%1").arg(m.toInt(), 2, 10, QLatin1Char('0')) + QLatin1Char('\''));
        }
        sql += QStringLiteral("AND strftime('%m', date) IN (%1) ").arg(monthStrs.join(QLatin1Char(',')));
    }

    // --- Type filter ---
    if (m_filterType != QStringLiteral("all") && !m_filterType.isEmpty()) {
        sql += QStringLiteral("AND type = '%1' ").arg(m_filterType);
    }

    // --- Multi-category filter ---
    if (!m_filterCategories.isEmpty()) {
        QStringList escaped;
        for (const QString &cat : m_filterCategories) {
            escaped.append(QLatin1Char('\'') + QString(cat).replace(QLatin1Char('\''), QLatin1String("''")) + QLatin1Char('\''));
        }
        sql += QStringLiteral("AND category IN (%1) ").arg(escaped.join(QLatin1Char(',')));
    }

    // --- Amount range filters ---
    const bool hasAmtMin = m_filterAmountMin > 0.0;
    const bool hasAmtMax = m_filterAmountMax > 0.0;
    if (hasAmtMin) sql += QStringLiteral("AND amount >= :amtMin ");
    if (hasAmtMax) sql += QStringLiteral("AND amount <= :amtMax ");

    // --- Search: title and note only (not category — category has its own filter) ---
    if (!m_searchQuery.trimmed().isEmpty()) {
        const QString escaped = m_searchQuery.trimmed().replace(QLatin1Char('\''), QLatin1String("''"));
        sql += QStringLiteral("AND (title LIKE '%%1%' OR note LIKE '%%1%') ").arg(escaped);
    }

    // --- Sort order ---
    sql += m_sortAscending
        ? QStringLiteral("ORDER BY date ASC, id ASC;")
        : QStringLiteral("ORDER BY date DESC, id DESC;");

    QSqlQuery q(db);
    q.prepare(sql);
    if (hasAmtMin) q.bindValue(QStringLiteral(":amtMin"), m_filterAmountMin);
    if (hasAmtMax) q.bindValue(QStringLiteral(":amtMax"), m_filterAmountMax);

    if (q.exec()) {
        while (q.next()) {
            TransactionItem item;
            item.id       = q.value(0).toInt();
            item.type     = q.value(1).toString();
            item.amount   = q.value(2).toDouble();
            item.category = q.value(3).toString();
            item.date     = q.value(4).toString();
            item.note     = q.value(5).toString();
            item.title    = q.value(6).toString();
            m_items.append(item);
        }
    } else {
        qWarning() << "TransactionModel::refresh query failed:" << q.lastError().text();
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

    if (!qFuzzyCompare(m_totalIncome + 1.0, inc + 1.0) || !qFuzzyCompare(m_totalExpense + 1.0, exp + 1.0)) {
        m_totalIncome = inc;
        m_totalExpense = exp;
        Q_EMIT totalsChanged();
    }
}

bool TransactionModel::addTransaction(const QString &type, double amount, const QString &category, const QString &date, const QString &note, const QString &title)
{
    if (amount <= 0 || category.trimmed().isEmpty() || date.trimmed().isEmpty()) {
        return false;
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO transactions (type, amount, category, date, note, title) "
        "VALUES (:type, :amount, :category, :date, :note, :title);"
    ));
    q.bindValue(QStringLiteral(":type"),     type.toLower());
    q.bindValue(QStringLiteral(":amount"),   amount);
    q.bindValue(QStringLiteral(":category"), category.trimmed());
    q.bindValue(QStringLiteral(":date"),     date.trimmed());
    q.bindValue(QStringLiteral(":note"),     note.trimmed());
    q.bindValue(QStringLiteral(":title"),    title.isEmpty() ? QStringLiteral("") : title.trimmed());

    if (q.exec()) {
        DatabaseManager::instance().addCategory(category.trimmed());
        Q_EMIT DatabaseManager::instance().databaseUpdated();
        return true;
    }

    qWarning() << "Failed to insert transaction:" << q.lastError().text();
    return false;
}

bool TransactionModel::updateTransaction(int id, const QString &type, double amount, const QString &category, const QString &date, const QString &note, const QString &title)
{
    if (id <= 0 || amount <= 0 || category.trimmed().isEmpty() || date.trimmed().isEmpty()) {
        return false;
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE transactions SET type = :type, amount = :amount, category = :category, "
        "date = :date, note = :note, title = :title WHERE id = :id;"
    ));
    q.bindValue(QStringLiteral(":type"),     type.toLower());
    q.bindValue(QStringLiteral(":amount"),   amount);
    q.bindValue(QStringLiteral(":category"), category.trimmed());
    q.bindValue(QStringLiteral(":date"),     date.trimmed());
    q.bindValue(QStringLiteral(":note"),     note.trimmed());
    q.bindValue(QStringLiteral(":title"),    title.isEmpty() ? QStringLiteral("") : title.trimmed());
    q.bindValue(QStringLiteral(":id"),       id);

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
