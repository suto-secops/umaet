#pragma once

#include <QObject>
#include <QSettings>

class SettingsManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool showNetBalance READ showNetBalance WRITE setShowNetBalance NOTIFY showNetBalanceChanged)
    Q_PROPERTY(QString currencySymbol READ currencySymbol WRITE setCurrencySymbol NOTIFY currencySymbolChanged)

public:
    static SettingsManager& instance();

    bool showNetBalance() const;
    void setShowNetBalance(bool show);

    QString currencySymbol() const;
    void setCurrencySymbol(const QString &symbol);

    Q_INVOKABLE void resetToDefaults();

Q_SIGNALS:
    void showNetBalanceChanged();
    void currencySymbolChanged();

private:
    SettingsManager();
    ~SettingsManager() override = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QSettings m_settings;
};
