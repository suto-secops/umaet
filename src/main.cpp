#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <KLocalizedContext>

#include "DatabaseManager.h"
#include "TransactionModel.h"
#include "BudgetManager.h"
#include "StatsManager.h"
#include "SettingsManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("umaet"));
    app.setApplicationDisplayName(QStringLiteral("Umaet"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("umaet"), QIcon(QStringLiteral(":/icons/umaet.svg"))));

    if (!DatabaseManager::instance().initialize()) {
        qCritical() << "Failed to initialize database!";
        return 1;
    }

    QQmlApplicationEngine engine;

    // Provide localized context if KF6I18n is available
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    TransactionModel transactionModel;
    BudgetManager budgetManager;
    StatsManager statsManager;

    engine.rootContext()->setContextProperty(QStringLiteral("dbManager"), &DatabaseManager::instance());
    engine.rootContext()->setContextProperty(QStringLiteral("transactionModel"), &transactionModel);
    engine.rootContext()->setContextProperty(QStringLiteral("budgetManager"), &budgetManager);
    engine.rootContext()->setContextProperty(QStringLiteral("statsManager"), &statsManager);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsManager"), &SettingsManager::instance());

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
