#pragma once

#include <QAbstractListModel>
#include <QDate>
#include <QVector>

struct TransactionItem {
    int id = 0;
    QString type; // "income" or "expense"
    double amount = 0.0;
    QString category;
    QString date; // YYYY-MM-DD
    QString note;
    QString title; // user-visible name/title for this transaction
};

class TransactionModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int filterYear READ filterYear WRITE setFilterYear NOTIFY filterChanged)
    Q_PROPERTY(int filterMonth READ filterMonth WRITE setFilterMonth NOTIFY filterChanged)
    Q_PROPERTY(QString filterType READ filterType WRITE setFilterType NOTIFY filterChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY filterChanged)

    Q_PROPERTY(QStringList filterCategories READ filterCategories WRITE setFilterCategories NOTIFY filterChanged)
    Q_PROPERTY(QVariantList filterMonths READ filterMonths WRITE setFilterMonths NOTIFY filterChanged)
    Q_PROPERTY(QVariantList filterYears READ filterYears WRITE setFilterYears NOTIFY filterChanged)
    Q_PROPERTY(double filterAmountMin READ filterAmountMin WRITE setFilterAmountMin NOTIFY filterChanged)
    Q_PROPERTY(double filterAmountMax READ filterAmountMax WRITE setFilterAmountMax NOTIFY filterChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY filterChanged)

    Q_PROPERTY(double totalIncome READ totalIncome NOTIFY totalsChanged)
    Q_PROPERTY(double totalExpense READ totalExpense NOTIFY totalsChanged)
    Q_PROPERTY(double netBalance READ netBalance NOTIFY totalsChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum TransactionRoles {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        AmountRole,
        CategoryRole,
        DateRole,
        NoteRole,
        TitleRole,
        FormattedAmountRole,
        FormattedDateRole
    };
    Q_ENUM(TransactionRoles)

    explicit TransactionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int filterYear() const { return m_filterYear; }
    void setFilterYear(int year);

    int filterMonth() const { return m_filterMonth; }
    void setFilterMonth(int month);

    QString filterType() const { return m_filterType; }
    void setFilterType(const QString &type);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    QStringList filterCategories() const { return m_filterCategories; }
    void setFilterCategories(const QStringList &cats);

    QVariantList filterMonths() const { return m_filterMonths; }
    void setFilterMonths(const QVariantList &months);

    QVariantList filterYears() const { return m_filterYears; }
    void setFilterYears(const QVariantList &years);

    double filterAmountMin() const { return m_filterAmountMin; }
    void setFilterAmountMin(double val);

    double filterAmountMax() const { return m_filterAmountMax; }
    void setFilterAmountMax(double val);

    bool sortAscending() const { return m_sortAscending; }
    void setSortAscending(bool asc);

    double totalIncome() const { return m_totalIncome; }
    double totalExpense() const { return m_totalExpense; }
    double netBalance() const { return m_totalIncome - m_totalExpense; }
    int count() const { return m_items.size(); }

    Q_INVOKABLE bool addTransaction(const QString &type, double amount, const QString &category, const QString &date, const QString &note, const QString &title = QString());
    Q_INVOKABLE bool updateTransaction(int id, const QString &type, double amount, const QString &category, const QString &date, const QString &note, const QString &title = QString());
    Q_INVOKABLE bool deleteTransaction(int id);
    Q_INVOKABLE void resetFilters();
    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void filterChanged();
    void totalsChanged();
    void countChanged();

private:
    void calculateTotals();

    QVector<TransactionItem> m_items;
    int m_filterYear = 0;
    int m_filterMonth = 0;
    QString m_filterType = QStringLiteral("all");
    QString m_searchQuery;

    QStringList m_filterCategories;
    QVariantList m_filterMonths;
    QVariantList m_filterYears;
    double m_filterAmountMin = 0.0;
    double m_filterAmountMax = 0.0;
    bool m_sortAscending = false;

    double m_totalIncome = 0.0;
    double m_totalExpense = 0.0;
};
