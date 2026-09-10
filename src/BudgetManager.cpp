#include "BudgetManager.h"
#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMap>

BudgetManager::BudgetManager(QObject *parent)
    : QAbstractListModel(parent)
{
    const QDate today = QDate::currentDate();
    m_activeYear = today.year();
    m_activeMonth = today.month();

    connect(&DatabaseManager::instance(), &DatabaseManager::databaseUpdated, this, &BudgetManager::refresh);

    refresh();
}

int BudgetManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant BudgetManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const CategoryBudgetItem &item = m_items.at(index.row());
    switch (role) {
    case CategoryRole:
        return item.category;
    case LimitRole:
        return item.limit;
    case SpentRole:
        return item.spent;
    case RemainingRole:
        return item.remaining;
    case PercentageRole:
        return item.percentage;
    case IsOverBudgetRole:
        return item.isOverBudget;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> BudgetManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CategoryRole] = "category";
    roles[LimitRole] = "limit";
    roles[SpentRole] = "spent";
    roles[RemainingRole] = "remaining";
    roles[PercentageRole] = "percentage";
    roles[IsOverBudgetRole] = "isOverBudget";
    return roles;
}

void BudgetManager::setActiveYear(int year)
{
    if (m_activeYear != year) {
        m_activeYear = year;
        Q_EMIT activePeriodChanged();
        refresh();
    }
}

void BudgetManager::setActiveMonth(int month)
{
    if (m_activeMonth != month) {
        m_activeMonth = month;
        Q_EMIT activePeriodChanged();
        refresh();
    }
}

bool BudgetManager::setGlobalBudget(double limit)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);

    if (limit <= 0) {
        q.prepare(QStringLiteral("DELETE FROM budgets WHERE category = '_GLOBAL_';"));
    } else {
        q.prepare(QStringLiteral("INSERT INTO budgets (category, monthly_limit) VALUES ('_GLOBAL_', :limit) "
                                "ON CONFLICT(category) DO UPDATE SET monthly_limit = :limit;"));
        q.bindValue(QStringLiteral(":limit"), limit);
    }

    if (q.exec()) {
        refresh();
        return true;
    }
    return false;
}

bool BudgetManager::setCategoryBudget(const QString &category, double limit)
{
    if (category.trimmed().isEmpty() || category == QStringLiteral("_GLOBAL_")) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);

    if (limit <= 0) {
        q.prepare(QStringLiteral("DELETE FROM budgets WHERE category = :cat;"));
        q.bindValue(QStringLiteral(":cat"), category.trimmed());
    } else {
        q.prepare(QStringLiteral("INSERT INTO budgets (category, monthly_limit) VALUES (:cat, :limit) "
                                "ON CONFLICT(category) DO UPDATE SET monthly_limit = :limit;"));
        q.bindValue(QStringLiteral(":cat"), category.trimmed());
        q.bindValue(QStringLiteral(":limit"), limit);
    }

    if (q.exec()) {
        refresh();
        return true;
    }
    return false;
}

bool BudgetManager::removeCategoryBudget(const QString &category)
{
    return setCategoryBudget(category, 0);
}

void BudgetManager::refresh()
{
    QSqlDatabase db = DatabaseManager::instance().database();

    // 1. Fetch Global Budget limit
    double newGlobalLimit = 0.0;
    QSqlQuery gq(QStringLiteral("SELECT monthly_limit FROM budgets WHERE category = '_GLOBAL_';"), db);
    if (gq.next()) {
        newGlobalLimit = gq.value(0).toDouble();
    }

    // 2. Fetch Total Spent for active month/year
    const QString mStr = QStringLiteral("%1").arg(m_activeMonth, 2, 10, QLatin1Char('0'));
    const QString yStr = QStringLiteral("%1").arg(m_activeYear);

    double newGlobalSpent = 0.0;
    QSqlQuery sq(db);
    sq.prepare(QStringLiteral(
        "SELECT SUM(amount) FROM transactions "
        "WHERE type = 'expense' AND strftime('%Y', date) = :year AND strftime('%m', date) = :month;"
    ));
    sq.bindValue(QStringLiteral(":year"), yStr);
    sq.bindValue(QStringLiteral(":month"), mStr);
    if (sq.exec() && sq.next()) {
        newGlobalSpent = sq.value(0).toDouble();
    }

    m_globalLimit = newGlobalLimit;
    m_globalSpent = newGlobalSpent;
    Q_EMIT globalBudgetChanged();

    // 3. Fetch Category Budgets and expenses for each category
    QSqlQuery bq(QStringLiteral("SELECT category, monthly_limit FROM budgets WHERE category != '_GLOBAL_' ORDER BY category ASC;"), db);
    QMap<QString, double> budgetLimits;
    while (bq.next()) {
        budgetLimits.insert(bq.value(0).toString(), bq.value(1).toDouble());
    }

    // Category spending in active month
    QSqlQuery cq(db);
    cq.prepare(QStringLiteral(
        "SELECT category, SUM(amount) FROM transactions "
        "WHERE type = 'expense' AND strftime('%Y', date) = :year AND strftime('%m', date) = :month "
        "GROUP BY category;"
    ));
    cq.bindValue(QStringLiteral(":year"), yStr);
    cq.bindValue(QStringLiteral(":month"), mStr);
    cq.exec();

    QMap<QString, double> categorySpent;
    while (cq.next()) {
        categorySpent.insert(cq.value(0).toString(), cq.value(1).toDouble());
    }

    beginResetModel();
    m_items.clear();

    for (auto it = budgetLimits.constBegin(); it != budgetLimits.constEnd(); ++it) {
        const QString cat = it.key();
        const double limit = it.value();
        const double spent = categorySpent.value(cat, 0.0);
        const double remaining = limit - spent;
        const double pct = (limit > 0) ? (spent / limit) : 0.0;

        CategoryBudgetItem item;
        item.category = cat;
        item.limit = limit;
        item.spent = spent;
        item.remaining = remaining;
        item.percentage = pct;
        item.isOverBudget = spent > limit;

        m_items.append(item);
    }

    endResetModel();
}
