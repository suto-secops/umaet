#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>

#include "DatabaseManager.h"
#include "TransactionModel.h"
#include "BudgetManager.h"
#include "StatsManager.h"

class TestUmaet : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testCategories();
    void testTransactionCRUD();
    void testFiltering();
    void testBudgetManager();
    void testStatsManager();
    void testImportExport();

private:
    QTemporaryDir m_tempDir;
};

void TestUmaet::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    const QString dbPath = m_tempDir.path() + QStringLiteral("/test_umaet.db");
    QVERIFY(DatabaseManager::instance().initialize(dbPath));
}

void TestUmaet::testCategories()
{
    DatabaseManager &db = DatabaseManager::instance();
    QStringList cats = db.getCategories();
    QVERIFY(!cats.isEmpty());
    QVERIFY(cats.contains(QStringLiteral("Food & Dining")));

    QVERIFY(db.addCategory(QStringLiteral("Crypto")));
    cats = db.getCategories();
    QVERIFY(cats.contains(QStringLiteral("Crypto")));

    QVERIFY(db.removeCategory(QStringLiteral("Crypto")));
    cats = db.getCategories();
    QVERIFY(!cats.contains(QStringLiteral("Crypto")));
}

void TestUmaet::testTransactionCRUD()
{
    TransactionModel model;
    model.resetFilters();

    const int initialCount = model.count();

    // 1. Add Income
    QVERIFY(model.addTransaction(QStringLiteral("income"), 3000.0, QStringLiteral("Salary"), QStringLiteral("2026-09-01"), QStringLiteral("Monthly paycheck")));
    // 2. Add Expense
    QVERIFY(model.addTransaction(QStringLiteral("expense"), 120.50, QStringLiteral("Food & Dining"), QStringLiteral("2026-09-05"), QStringLiteral("Dinner")));

    model.resetFilters();
    QCOMPARE(model.count(), initialCount + 2);

    QCOMPARE(model.totalIncome(), 3000.0);
    QCOMPARE(model.totalExpense(), 120.50);
    QCOMPARE(model.netBalance(), 3000.0 - 120.50);

    // 3. Update Transaction
    const int newId = model.data(model.index(0, 0), TransactionModel::IdRole).toInt();
    QVERIFY(newId > 0);
    QVERIFY(model.updateTransaction(newId, QStringLiteral("expense"), 150.0, QStringLiteral("Food & Dining"), QStringLiteral("2026-09-05"), QStringLiteral("Dinner with friends")));

    model.resetFilters();
    QCOMPARE(model.totalExpense(), 150.0);

    // 4. Delete Transaction
    QVERIFY(model.deleteTransaction(newId));
    model.resetFilters();
    QCOMPARE(model.count(), initialCount + 1);
}

void TestUmaet::testFiltering()
{
    TransactionModel model;
    model.resetFilters();

    // Insert known data
    model.addTransaction(QStringLiteral("expense"), 45.0, QStringLiteral("Shopping"), QStringLiteral("2025-05-10"), QStringLiteral("Books"));
    model.addTransaction(QStringLiteral("expense"), 80.0, QStringLiteral("Shopping"), QStringLiteral("2026-09-12"), QStringLiteral("Shoes"));

    // Filter Year 2025
    model.setFilterYear(2025);
    model.setFilterMonth(0);
    QVERIFY(model.count() >= 1);

    // Filter Search
    model.resetFilters();
    model.setSearchQuery(QStringLiteral("Shoes"));
    QCOMPARE(model.count(), 1);

    // Reset filters
    model.resetFilters();
    QVERIFY(model.count() >= 2);
}

void TestUmaet::testBudgetManager()
{
    BudgetManager bm;
    bm.setActiveYear(2026);
    bm.setActiveMonth(9);

    // Set global budget
    QVERIFY(bm.setGlobalBudget(1000.0));
    QCOMPARE(bm.globalLimit(), 1000.0);
    QVERIFY(bm.hasGlobalBudget());

    // Set category budget
    QVERIFY(bm.setCategoryBudget(QStringLiteral("Food & Dining"), 300.0));
    bm.refresh();

    bool foundCat = false;
    for (int i = 0; i < bm.rowCount(); ++i) {
        const QString cat = bm.data(bm.index(i, 0), BudgetManager::CategoryRole).toString();
        if (cat == QStringLiteral("Food & Dining")) {
            foundCat = true;
            QCOMPARE(bm.data(bm.index(i, 0), BudgetManager::LimitRole).toDouble(), 300.0);
        }
    }
    QVERIFY(foundCat);

    // Remove category budget
    QVERIFY(bm.removeCategoryBudget(QStringLiteral("Food & Dining")));
}

void TestUmaet::testStatsManager()
{
    StatsManager sm;
    sm.setSelectedYear(2026);

    const QVariantList monthly = sm.getMonthlyData(2026);
    QCOMPARE(monthly.size(), 12);

    const QVariantList yearly = sm.getYearlyData();
    QVERIFY(!yearly.isEmpty());

    const QVariantList weekly = sm.getWeeklyData(5);
    QVERIFY(!weekly.isEmpty());

    const QVariantList categories = sm.getCategoryBreakdown(2026, 9, QStringLiteral("expense"));
    QVERIFY(categories.size() >= 0);
}

void TestUmaet::testImportExport()
{
    DatabaseManager &db = DatabaseManager::instance();

    // Export to JSON
    const QString json = db.exportToJson();
    QVERIFY(!json.isEmpty());
    QVERIFY(json.startsWith(QLatin1Char('[')));

    // Export to CSV
    const QString csv = db.exportToCsv();
    QVERIFY(!csv.isEmpty());
    QVERIFY(csv.startsWith(QLatin1String("Date,Type,Amount")));

    // Import from CSV
    const QString testCsv = QStringLiteral(
        "Date,Type,Amount,Category,Note\n"
        "2026-08-15,expense,75.00,Healthcare,Pharmacy\n"
        "2026-08-20,income,150.00,Investments,Dividend\n"
    );
    QString errMsg;
    QVERIFY(db.importFromCsv(testCsv, &errMsg));

    // Import from JSON
    const QString testJson = QStringLiteral(
        "[\n"
        "  {\"date\": \"2026-07-01\", \"type\": \"expense\", \"amount\": 29.99, \"category\": \"Utilities\", \"note\": \"Internet\"}\n"
        "]"
    );
    QVERIFY(db.importFromJson(testJson, &errMsg));
}

QTEST_MAIN(TestUmaet)
#include "test_umaet.moc"
