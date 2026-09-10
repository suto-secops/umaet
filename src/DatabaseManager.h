#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QVariantList>
#include <QJsonObject>
#include <QJsonArray>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();

    bool initialize(const QString &dbPath = QString());
    QSqlDatabase database() const;

    Q_INVOKABLE QStringList getCategories() const;
    Q_INVOKABLE bool addCategory(const QString &name);
    Q_INVOKABLE bool removeCategory(const QString &name);

    // Bulk Import / Export
    Q_INVOKABLE bool importFromJson(const QString &jsonContent, QString *errorMessage = nullptr);
    Q_INVOKABLE bool importFromCsv(const QString &csvContent, QString *errorMessage = nullptr);
    Q_INVOKABLE QString exportToJson() const;
    Q_INVOKABLE QString exportToCsv() const;

    // File helpers
    Q_INVOKABLE bool importFromFile(const QString &filePath, QString *errorMessage = nullptr);
    Q_INVOKABLE bool exportToFile(const QString &filePath, const QString &format, QString *errorMessage = nullptr);

Q_SIGNALS:
    void databaseUpdated();
    void categoriesUpdated();

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void initTables();
    void seedCategories();

    QString m_connectionName;
};
