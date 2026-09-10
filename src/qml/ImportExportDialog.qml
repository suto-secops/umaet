import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: rootDialog
    title: qsTr("Import & Export Financial Data")
    width: 540
    standardButtons: Kirigami.Dialog.Close

    property string statusMessage: ""
    property bool isError: false

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: parent.width

        QQC2.TabBar {
            id: ioTabBar
            Layout.fillWidth: true
            QQC2.TabButton { text: qsTr("Bulk Import") }
            QQC2.TabButton { text: qsTr("Export Data") }
        }

        // --- IMPORT TAB ---
        ColumnLayout {
            visible: ioTabBar.currentIndex === 0
            spacing: Kirigami.Units.mediumSpacing
            Layout.fillWidth: true

            QQC2.Label {
                text: qsTr("Upload transactions from a CSV or JSON file:")
                font.weight: Font.DemiBold
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                QQC2.TextField {
                    id: importPathField
                    placeholderText: qsTr("/path/to/transactions.csv or .json")
                    Layout.fillWidth: true
                }

                QQC2.Button {
                    text: qsTr("Browse...")
                    icon.name: "document-open"
                    onClicked: importFileDialog.open()
                }
            }

            QQC2.Button {
                text: qsTr("Execute File Import")
                icon.name: "document-import"
                highlighted: true
                enabled: importPathField.text.length > 0
                onClicked: {
                    let errMsg = "";
                    const success = dbManager.importFromFile(importPathField.text.trim(), errMsg);
                    if (success) {
                        rootDialog.statusMessage = qsTr("Successfully imported transactions from file!");
                        rootDialog.isError = false;
                        importPathField.text = "";
                    } else {
                        rootDialog.statusMessage = qsTr("Import failed: %1").arg(errMsg);
                        rootDialog.isError = true;
                    }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            QQC2.Label {
                text: qsTr("Or paste raw CSV / JSON text directly:")
                font.weight: Font.DemiBold
            }

            QQC2.TextArea {
                id: pasteArea
                placeholderText: "Date,Type,Amount,Category,Note\n2026-09-01,expense,45.50,Food & Dining,Supermarket\n2026-09-02,income,2500.00,Salary,Monthly pay"
                Layout.fillWidth: true
                implicitHeight: 120
                wrapMode: Text.WrapAnywhere
            }

            QQC2.Button {
                text: qsTr("Import Pasted Data")
                icon.name: "edit-paste"
                enabled: pasteArea.text.trim().length > 0
                onClicked: {
                    const txt = pasteArea.text.trim();
                    let success = false;
                    let errMsg = "";
                    if (txt.startsWith("[") || txt.startsWith("{")) {
                        success = dbManager.importFromJson(txt, errMsg);
                    } else {
                        success = dbManager.importFromCsv(txt, errMsg);
                    }

                    if (success) {
                        rootDialog.statusMessage = qsTr("Pasted transactions imported successfully!");
                        rootDialog.isError = false;
                        pasteArea.text = "";
                    } else {
                        rootDialog.statusMessage = qsTr("Import failed: %1").arg(errMsg);
                        rootDialog.isError = true;
                    }
                }
            }
        }

        // --- EXPORT TAB ---
        ColumnLayout {
            visible: ioTabBar.currentIndex === 1
            spacing: Kirigami.Units.mediumSpacing
            Layout.fillWidth: true

            QQC2.Label {
                text: qsTr("Export all transactions to a file on your system:")
                font.weight: Font.DemiBold
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                QQC2.TextField {
                    id: exportPathField
                    placeholderText: qsTr("/path/to/export.csv")
                    Layout.fillWidth: true
                }

                QQC2.Button {
                    text: qsTr("Browse...")
                    icon.name: "document-save-as"
                    onClicked: exportFileDialog.open()
                }
            }

            RowLayout {
                spacing: Kirigami.Units.mediumSpacing

                QQC2.Button {
                    text: qsTr("Export as CSV")
                    icon.name: "text-csv"
                    highlighted: true
                    enabled: exportPathField.text.length > 0
                    onClicked: {
                        let path = exportPathField.text.trim();
                        if (!path.endsWith(".csv") && !path.endsWith(".json")) path += ".csv";
                        const success = dbManager.exportToFile(path, "csv");
                        if (success) {
                            rootDialog.statusMessage = qsTr("Exported successfully to %1").arg(path);
                            rootDialog.isError = false;
                        } else {
                            rootDialog.statusMessage = qsTr("Failed to export file.");
                            rootDialog.isError = true;
                        }
                    }
                }

                QQC2.Button {
                    text: qsTr("Export as JSON")
                    icon.name: "application-json"
                    enabled: exportPathField.text.length > 0
                    onClicked: {
                        let path = exportPathField.text.trim();
                        if (!path.endsWith(".json")) path += ".json";
                        const success = dbManager.exportToFile(path, "json");
                        if (success) {
                            rootDialog.statusMessage = qsTr("Exported successfully to %1").arg(path);
                            rootDialog.isError = false;
                        } else {
                            rootDialog.statusMessage = qsTr("Failed to export file.");
                            rootDialog.isError = true;
                        }
                    }
                }
            }
        }

        // Status Feedback Box
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: statusLabel.implicitHeight + 16
            visible: rootDialog.statusMessage.length > 0
            radius: 4
            color: rootDialog.isError ? Kirigami.Theme.negativeBackgroundColor : Kirigami.Theme.positiveBackgroundColor

            QQC2.Label {
                id: statusLabel
                anchors.centerIn: parent
                width: parent.width - 20
                wrapMode: Text.WordWrap
                text: rootDialog.statusMessage
                color: rootDialog.isError ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor
                font.weight: Font.Bold
            }
        }
    }

    FileDialog {
        id: importFileDialog
        title: qsTr("Choose File to Import")
        nameFilters: ["Data files (*.csv *.json)", "CSV files (*.csv)", "JSON files (*.json)", "All files (*)"]
        onAccepted: {
            importPathField.text = selectedFile.toString();
        }
    }

    FileDialog {
        id: exportFileDialog
        title: qsTr("Choose Export File Destination")
        fileMode: FileDialog.SaveFile
        nameFilters: ["CSV file (*.csv)", "JSON file (*.json)"]
        onAccepted: {
            exportPathField.text = selectedFile.toString();
        }
    }
}
