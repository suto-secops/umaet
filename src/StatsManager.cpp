#include "StatsManager.h"
#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDebug>
#include <QMap>

StatsManager::StatsManager(QObject *parent)
    : QObject(parent)
{
    m_selectedYear = QDate::currentDate().year();

    connect(&DatabaseManager::instance(), &DatabaseManager::databaseUpdated, this, &StatsManager::refresh);

    refresh();
}

void StatsManager::setSelectedYear(int year)
{
    if (m_selectedYear != year) {
        m_selectedYear = year;
        Q_EMIT selectedYearChanged();
        refresh();
    }
}

void StatsManager::refresh()
{
    QSqlDatabase db = DatabaseManager::instance().database();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT "
        "  COALESCE(SUM(CASE WHEN type = 'income' THEN amount ELSE 0 END), 0) AS total_inc, "
        "  COALESCE(SUM(CASE WHEN type = 'expense' THEN amount ELSE 0 END), 0) AS total_exp "
        "FROM transactions "
        "WHERE strftime('%Y', date) = :year;"
    ));
    q.bindValue(QStringLiteral(":year"), QString::number(m_selectedYear));

    if (q.exec() && q.next()) {
        m_periodIncome = q.value(0).toDouble();
        m_periodExpense = q.value(1).toDouble();
    } else {
        m_periodIncome = 0.0;
        m_periodExpense = 0.0;
    }

    Q_EMIT periodTotalsChanged();
    Q_EMIT statsUpdated();
}

QVariantList StatsManager::getWeeklyData(int numWeeks) const
{
    QVariantList list;
    QSqlDatabase db = DatabaseManager::instance().database();

    const QDate today = QDate::currentDate();
    // Monday of current week (Qt: Monday=1)
    const QDate currentMonday = today.addDays(-(today.dayOfWeek() - 1));

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT "
        "  COALESCE(SUM(CASE WHEN type='income' THEN amount ELSE 0 END),0), "
        "  COALESCE(SUM(CASE WHEN type='expense' THEN amount ELSE 0 END),0) "
        "FROM transactions WHERE date >= :mon AND date <= :sun;"
    ));

    // Build oldest-first: start from (numWeeks-1) weeks ago up to current week
    for (int i = numWeeks - 1; i >= 0; --i) {
        QDate monday = currentMonday.addDays(-7 * i);
        QDate sunday = monday.addDays(6);

        q.bindValue(QStringLiteral(":mon"), monday.toString(Qt::ISODate));
        q.bindValue(QStringLiteral(":sun"), sunday.toString(Qt::ISODate));

        double inc = 0.0, exp = 0.0;
        if (q.exec() && q.next()) {
            inc = q.value(0).toDouble();
            exp = q.value(1).toDouble();
        }

        QVariantMap item;
        item[QStringLiteral("label")]     = monday.toString(QStringLiteral("dd/MM"));
        item[QStringLiteral("fullLabel")] = QString(monday.toString(QStringLiteral("dd MMM")) + QLatin1String(" - ") + sunday.toString(QStringLiteral("dd MMM")));
        item[QStringLiteral("income")]    = inc;
        item[QStringLiteral("expense")]   = exp;
        item[QStringLiteral("net")]       = inc - exp;
        list.append(item);
    }

    return list;
}

QVariantList StatsManager::getMonthlyData(int year) const
{
    QVariantList list;
    QSqlDatabase db = DatabaseManager::instance().database();

    const QStringList monthNames = {
        QStringLiteral("Jan"), QStringLiteral("Feb"), QStringLiteral("Mar"),
        QStringLiteral("Apr"), QStringLiteral("May"), QStringLiteral("Jun"),
        QStringLiteral("Jul"), QStringLiteral("Aug"), QStringLiteral("Sep"),
        QStringLiteral("Oct"), QStringLiteral("Nov"), QStringLiteral("Dec")
    };

    QMap<int, double> incomeMap;
    QMap<int, double> expenseMap;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT cast(strftime('%m', date) as integer) AS m, "
        "       COALESCE(SUM(CASE WHEN type = 'income' THEN amount ELSE 0 END), 0) AS inc, "
        "       COALESCE(SUM(CASE WHEN type = 'expense' THEN amount ELSE 0 END), 0) AS exp "
        "FROM transactions "
        "WHERE strftime('%Y', date) = :year "
        "GROUP BY m;"
    ));
    q.bindValue(QStringLiteral(":year"), QString::number(year));

    if (q.exec()) {
        while (q.next()) {
            int m = q.value(0).toInt();
            incomeMap[m] = q.value(1).toDouble();
            expenseMap[m] = q.value(2).toDouble();
        }
    }

    for (int m = 1; m <= 12; ++m) {
        QVariantMap item;
        double inc = incomeMap.value(m, 0.0);
        double exp = expenseMap.value(m, 0.0);
        item[QStringLiteral("label")] = monthNames.value(m - 1);
        item[QStringLiteral("month")] = m;
        item[QStringLiteral("income")] = inc;
        item[QStringLiteral("expense")] = exp;
        item[QStringLiteral("net")] = inc - exp;
        list.append(item);
    }

    return list;
}

QVariantList StatsManager::getYearlyData() const
{
    QVariantList list;
    QSqlDatabase db = DatabaseManager::instance().database();

    QSqlQuery q(QStringLiteral(
        "SELECT strftime('%Y', date) AS y, "
        "       COALESCE(SUM(CASE WHEN type = 'income' THEN amount ELSE 0 END), 0) AS inc, "
        "       COALESCE(SUM(CASE WHEN type = 'expense' THEN amount ELSE 0 END), 0) AS exp "
        "FROM transactions "
        "GROUP BY y "
        "ORDER BY y ASC;"
    ), db);

    while (q.next()) {
        QString y = q.value(0).toString();
        if (y.isEmpty()) continue;
        double inc = q.value(1).toDouble();
        double exp = q.value(2).toDouble();

        QVariantMap item;
        item[QStringLiteral("label")] = y;
        item[QStringLiteral("year")] = y.toInt();
        item[QStringLiteral("income")] = inc;
        item[QStringLiteral("expense")] = exp;
        item[QStringLiteral("net")] = inc - exp;
        list.append(item);
    }

    if (list.isEmpty()) {
        const int curr = QDate::currentDate().year();
        QVariantMap item;
        item[QStringLiteral("label")] = QString::number(curr);
        item[QStringLiteral("year")] = curr;
        item[QStringLiteral("income")] = 0.0;
        item[QStringLiteral("expense")] = 0.0;
        item[QStringLiteral("net")] = 0.0;
        list.append(item);
    }

    return list;
}

QVariantList StatsManager::getCategoryBreakdown(int year, int month, const QString &type) const
{
    QVariantList list;
    QSqlDatabase db = DatabaseManager::instance().database();

    QString sql = QStringLiteral(
        "SELECT category, SUM(amount) AS total "
        "FROM transactions "
        "WHERE type = :type "
    );

    if (year > 0) {
        sql += QStringLiteral("AND strftime('%Y', date) = :year ");
    }
    if (month > 0) {
        sql += QStringLiteral("AND strftime('%m', date) = :month ");
    }
    sql += QStringLiteral("GROUP BY category ORDER BY total DESC;");

    QSqlQuery q(db);
    q.prepare(sql);
    q.bindValue(QStringLiteral(":type"), type.toLower());
    if (year > 0) q.bindValue(QStringLiteral(":year"), QString::number(year));
    if (month > 0) q.bindValue(QStringLiteral(":month"), QStringLiteral("%1").arg(month, 2, 10, QLatin1Char('0')));

    double grandTotal = 0.0;
    QList<QPair<QString, double>> rows;
    if (q.exec()) {
        while (q.next()) {
            QString cat = q.value(0).toString();
            double amt = q.value(1).toDouble();
            grandTotal += amt;
            rows.append(qMakePair(cat, amt));
        }
    }

    for (const auto &pair : rows) {
        QVariantMap item;
        item[QStringLiteral("category")] = pair.first;
        item[QStringLiteral("amount")] = pair.second;
        item[QStringLiteral("percentage")] = (grandTotal > 0.0) ? (pair.second / grandTotal) : 0.0;
        item[QStringLiteral("color")] = DatabaseManager::instance().getCategoryColor(pair.first);
        list.append(item);
    }

    return list;
}

QVariantList StatsManager::getAvailableYears() const
{
    QVariantList list;
    QSqlDatabase db = DatabaseManager::instance().database();

    QSqlQuery q(QStringLiteral("SELECT DISTINCT strftime('%Y', date) FROM transactions ORDER BY 1 DESC;"), db);
    while (q.next()) {
        QString yStr = q.value(0).toString();
        if (!yStr.isEmpty()) {
            list.append(yStr.toInt());
        }
    }

    int curr = QDate::currentDate().year();
    if (!list.contains(curr)) {
        list.prepend(curr);
    }

    return list;
}
