#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class StatsManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int selectedYear READ selectedYear WRITE setSelectedYear NOTIFY selectedYearChanged)
    Q_PROPERTY(double periodIncome READ periodIncome NOTIFY periodTotalsChanged)
    Q_PROPERTY(double periodExpense READ periodExpense NOTIFY periodTotalsChanged)
    Q_PROPERTY(double periodNet READ periodNet NOTIFY periodTotalsChanged)
    Q_PROPERTY(double savingsRate READ savingsRate NOTIFY periodTotalsChanged)

public:
    explicit StatsManager(QObject *parent = nullptr);

    int selectedYear() const { return m_selectedYear; }
    void setSelectedYear(int year);

    double periodIncome() const { return m_periodIncome; }
    double periodExpense() const { return m_periodExpense; }
    double periodNet() const { return m_periodIncome - m_periodExpense; }
    double savingsRate() const {
        if (m_periodIncome <= 0.0) return 0.0;
        return qMax(0.0, (m_periodIncome - m_periodExpense) / m_periodIncome * 100.0);
    }

    Q_INVOKABLE QVariantList getWeeklyData(int numWeeks = 10) const;
    Q_INVOKABLE QVariantList getMonthlyData(int year) const;
    Q_INVOKABLE QVariantList getYearlyData() const;
    Q_INVOKABLE QVariantList getCategoryBreakdown(int year, int month = 0, const QString &type = QStringLiteral("expense")) const;

    Q_INVOKABLE QVariantList getAvailableYears() const;

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void selectedYearChanged();
    void periodTotalsChanged();
    void statsUpdated();

private:
    int m_selectedYear;
    double m_periodIncome = 0.0;
    double m_periodExpense = 0.0;
};
