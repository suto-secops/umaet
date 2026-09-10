#pragma once

#include <QAbstractListModel>
#include <QDate>
#include <QVector>

struct CategoryBudgetItem {
    QString category;
    double limit = 0.0;
    double spent = 0.0;
    double remaining = 0.0;
    double percentage = 0.0; // 0.0 - 1.0+
    bool isOverBudget = false;
};

class BudgetManager : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int activeYear READ activeYear WRITE setActiveYear NOTIFY activePeriodChanged)
    Q_PROPERTY(int activeMonth READ activeMonth WRITE setActiveMonth NOTIFY activePeriodChanged)

    Q_PROPERTY(double globalLimit READ globalLimit NOTIFY globalBudgetChanged)
    Q_PROPERTY(double globalSpent READ globalSpent NOTIFY globalBudgetChanged)
    Q_PROPERTY(double globalRemaining READ globalRemaining NOTIFY globalBudgetChanged)
    Q_PROPERTY(double globalPercentage READ globalPercentage NOTIFY globalBudgetChanged)
    Q_PROPERTY(bool isGlobalOverBudget READ isGlobalOverBudget NOTIFY globalBudgetChanged)
    Q_PROPERTY(bool hasGlobalBudget READ hasGlobalBudget NOTIFY globalBudgetChanged)

public:
    enum BudgetRoles {
        CategoryRole = Qt::UserRole + 1,
        LimitRole,
        SpentRole,
        RemainingRole,
        PercentageRole,
        IsOverBudgetRole
    };
    Q_ENUM(BudgetRoles)

    explicit BudgetManager(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int activeYear() const { return m_activeYear; }
    void setActiveYear(int year);

    int activeMonth() const { return m_activeMonth; }
    void setActiveMonth(int month);

    double globalLimit() const { return m_globalLimit; }
    double globalSpent() const { return m_globalSpent; }
    double globalRemaining() const { return m_globalLimit - m_globalSpent; }
    double globalPercentage() const {
        if (m_globalLimit <= 0.0) return 0.0;
        return qMin(m_globalSpent / m_globalLimit, 1.0);
    }
    bool isGlobalOverBudget() const { return m_globalLimit > 0.0 && m_globalSpent > m_globalLimit; }
    bool hasGlobalBudget() const { return m_globalLimit > 0.0; }

    Q_INVOKABLE bool setGlobalBudget(double limit);
    Q_INVOKABLE bool setCategoryBudget(const QString &category, double limit);
    Q_INVOKABLE bool removeCategoryBudget(const QString &category);
    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void activePeriodChanged();
    void globalBudgetChanged();

private:
    int m_activeYear;
    int m_activeMonth;

    double m_globalLimit = 0.0;
    double m_globalSpent = 0.0;

    QVector<CategoryBudgetItem> m_items;
};
