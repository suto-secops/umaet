import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    title: qsTr("Umaet - Personal Finance & Budget")
    width: 960
    height: 700
    minimumWidth: 640
    minimumHeight: 500

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: false
        actions: [
            Kirigami.Action {
                text: qsTr("Transactions")
                icon.name: "view-financial-transfer"
                checked: pageStack.currentItem === transactionsPage
                onTriggered: pageStack.replace(transactionsPage)
            },
            Kirigami.Action {
                text: qsTr("Trends & Graphs")
                icon.name: "office-chart-bar"
                checked: pageStack.currentItem === chartsPage
                onTriggered: pageStack.replace(chartsPage)
            },
            Kirigami.Action {
                text: qsTr("Budgets")
                icon.name: "wallet-open"
                checked: pageStack.currentItem === budgetPage
                onTriggered: pageStack.replace(budgetPage)
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                text: qsTr("Import / Export Data")
                icon.name: "document-import"
                onTriggered: importExportDialog.open()
            },
            Kirigami.Action {
                text: qsTr("Settings")
                icon.name: "settings-configure"
                checked: pageStack.currentItem === settingsPage
                onTriggered: pageStack.replace(settingsPage)
            }
        ]
    }

    Component.onCompleted: {
        pageStack.push(transactionsPage)
    }

    Component {
        id: transactionsPage
        TransactionsPage {}
    }

    Component {
        id: chartsPage
        ChartsPage {}
    }

    Component {
        id: budgetPage
        BudgetPage {}
    }

    Component {
        id: settingsPage
        SettingsPage {}
    }

    AddEditTransactionDialog {
        id: addEditDialog
    }

    ImportExportDialog {
        id: importExportDialog
    }
}
