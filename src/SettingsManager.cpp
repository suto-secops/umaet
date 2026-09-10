#include "SettingsManager.h"

SettingsManager& SettingsManager::instance()
{
    static SettingsManager s_instance;
    return s_instance;
}

SettingsManager::SettingsManager()
    : m_settings(QStringLiteral("kde.org"), QStringLiteral("umaet"))
{
}

bool SettingsManager::showNetBalance() const
{
    return m_settings.value(QStringLiteral("appearance/showNetBalance"), true).toBool();
}

void SettingsManager::setShowNetBalance(bool show)
{
    if (showNetBalance() != show) {
        m_settings.setValue(QStringLiteral("appearance/showNetBalance"), show);
        Q_EMIT showNetBalanceChanged();
    }
}

QString SettingsManager::currencySymbol() const
{
    return m_settings.value(QStringLiteral("appearance/currencySymbol"), QStringLiteral("€")).toString();
}

void SettingsManager::setCurrencySymbol(const QString &symbol)
{
    if (currencySymbol() != symbol && !symbol.trimmed().isEmpty()) {
        m_settings.setValue(QStringLiteral("appearance/currencySymbol"), symbol.trimmed());
        Q_EMIT currencySymbolChanged();
    }
}

void SettingsManager::resetToDefaults()
{
    setShowNetBalance(true);
    setCurrencySymbol(QStringLiteral("€"));
}
