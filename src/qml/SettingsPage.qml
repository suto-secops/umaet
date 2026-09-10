import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: qsTr("Settings")

    Kirigami.FormLayout {
        width: page.width

        QQC2.Label {
            text: qsTr("Display & Appearance")
            font.weight: Font.Bold
            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
            Kirigami.FormData.isSection: true
        }

        QQC2.Switch {
            id: netBalanceSwitch
            Kirigami.FormData.label: qsTr("Show Net Balance:")
            checked: settingsManager.showNetBalance
            onToggled: settingsManager.showNetBalance = checked
        }

        QQC2.Label {
            text: qsTr("When enabled, displays Net Balance on the Transactions page and Net Savings on the Trends page.")
            font.pixelSize: Kirigami.Theme.smallFont.pixelSize
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        QQC2.TextField {
            id: currencyField
            Kirigami.FormData.label: qsTr("Currency Symbol:")
            text: settingsManager.currencySymbol
            onEditingFinished: {
                if (text.trim().length > 0) {
                    settingsManager.currencySymbol = text.trim();
                }
            }
        }

        Item {
            Kirigami.FormData.isSection: true
            height: Kirigami.Units.largeSpacing
        }

        QQC2.Label {
            text: qsTr("Data Management")
            font.weight: Font.Bold
            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
            Kirigami.FormData.isSection: true
        }

        QQC2.Button {
            text: qsTr("Load Comprehensive Sample Data")
            icon.name: "document-import"
            Kirigami.FormData.label: qsTr("Demo Data:")
            onClicked: confirmSampleDialog.open()
        }

        QQC2.Label {
            text: qsTr("Loads ~100 realistic transactions and budgets across recent months to test all graphs and features.")
            font.pixelSize: Kirigami.Theme.smallFont.pixelSize
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        QQC2.Button {
            text: qsTr("Reset to Default Settings")
            icon.name: "edit-undo"
            Kirigami.FormData.label: qsTr("Preferences:")
            onClicked: {
                settingsManager.resetToDefaults();
                netBalanceSwitch.checked = settingsManager.showNetBalance;
                currencyField.text = settingsManager.currencySymbol;
            }
        }
    }

    Kirigami.Dialog {
        id: confirmSampleDialog
        title: qsTr("Load Sample Data")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

        QQC2.Label {
            text: qsTr("This will import a comprehensive test dataset containing transactions and budgets across multiple months. Continue?")
            wrapMode: Text.WordWrap
            width: 320
        }

        onAccepted: {
            dbManager.loadSampleDataset();
            pageStack.layers.clear();
        }
    }
}
