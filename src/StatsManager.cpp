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

    // Query weekly totals for the most recent weeks or selected year
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT strftime('%Y-W%W', date) AS week_str, "
        "       COALESCE(SUM(CASE WHEN type = 'income' THEN amount ELSE 0 END), 0) AS inc, "
        "       COALESCE(SUM(CASE WHEN type = 'expense' THEN amount ELSE 0 END), 0) AS exp "
        "FROM transactions "
        "GROUP BY week_str "
        "ORDER BY week_str DESC "
        "LIMIT :lim;"
    ));
    q.bindValue(QStringLiteral(":lim"), numWeeks);

    if (q.exec()) {
        QList<QVariantMap> reversed;
        while (q.next()) {
            QVariantMap item;
            QString full = q.value(0).toString();
            QString shortLabel = full.section(QLatin1Char('-'), 1, 1);
            if (shortLabel.isEmpty()) shortLabel = full;

            double inc = q.value(1).toDouble();
            double exp = q.value(2).toDouble();

            item[QStringLiteral("label")] = shortLabel;
            item[QStringLiteral("fullLabel")] = full;
            item[QStringLiteral("income")] = inc;
            item[QStringLiteral("expense")] = exp;
            item[QStringLiteral("net")] = inc - exp;
            reversed.append(item);
        }
        for (int i = reversed.size() - 1; i >= 0; --i) {
            list.append(reversed.at(i));
        }
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

    static const QStringList palette = {
        QStringLiteral("#3daee9"), QStringLiteral("#2ecc71"), QStringLiteral("#f1c40f"),
        QStringLiteral("#e74c3c"), QStringLiteral("#9b59b6"), QStringLiteral("#1abc9c"),
        QStringLiteral("#e67e22"), QStringLiteral("#34495e"), QStringLiteral("#fd79a8"),
        QStringLiteral("#00cec9"), QStringLiteral("#6c5ce7"), QStringLiteral("#b2bec3")
    };

    int colorIdx = 0;
    for (const auto &pair : rows) {
        QVariantMap item;
        item[QStringLiteral("category")] = pair.first;
        item[QStringLiteral("amount")] = pair.second;
        item[QStringLiteral("percentage")] = (grandTotal > 0.0) ? (pair.second / grandTotal) : 0.0;
        item[QStringLiteral("color")] = palette.at(colorIdx % palette.size());
        colorIdx++;
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
