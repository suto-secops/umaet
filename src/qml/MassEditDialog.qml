import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: massEditDialog
    title: qsTr("Mass Edit Transactions")
    standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Cancel

    property var selectedIds: []

    function openForIds(ids) {
        selectedIds = ids;
        applyCategoryCheck.checked = false;
        applyDateCheck.checked = false;
        applyFlowCheck.checked = false;
        errorLabel.text = "";
        
        const now = new Date();
        const year = now.getFullYear();
        const month = String(now.getMonth() + 1).padStart(2, '0');
        const day = String(now.getDate()).padStart(2, '0');
        dateField.text = year + "-" + month + "-" + day;
        categoryCombo.currentIndex = 0;
        expenseBtn.checked = true;

        open();
    }

    onAccepted: {
        const ids = selectedIds;
        if (ids.length === 0) return;

        const updateCategory = applyCategoryCheck.checked;
        const updateDate = applyDateCheck.checked;
        const updateFlow = applyFlowCheck.checked;

        if (!updateCategory && !updateDate && !updateFlow) {
            return;
        }

        const dateRegex = /^\d{4}-\d{2}-\d{2}$/;
        if (updateDate && !dateRegex.test(dateField.text.trim())) {
            errorLabel.text = qsTr("Date must be in YYYY-MM-DD format.");
            massEditDialog.open();
            return;
        }

        let cat = categoryCombo.currentText;
        const transType = incomeBtn.checked ? "income" : "expense";

        // To mass update, we have to do it through the model. But TransactionModel only has updateTransaction which updates EVERYTHING.
        // Oh wait, TransactionModel doesn't have a partial update or a mass update.
        // We might need to fetch the existing transaction, modify the fields, and update it.
        // Or I can add a massUpdate method to the database manager / transaction model.
        // Let's call a new method on transactionModel or we can execute SQL through dbManager directly? No, TransactionModel is the API.
        
        transactionModel.massUpdateTransactions(ids, updateFlow ? transType : "", updateCategory ? cat : "", updateDate ? dateField.text.trim() : "");
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: parent.width

        QQC2.Label {
            text: qsTr("Editing %1 transactions").arg(selectedIds.length)
            font.weight: Font.Bold
            Layout.fillWidth: true
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            // Category
            RowLayout {
                Kirigami.FormData.label: qsTr("Category:")
                QQC2.CheckBox {
                    id: applyCategoryCheck
                    text: qsTr("Update")
                }
                QQC2.ComboBox {
                    id: categoryCombo
                    model: dbManager.getCategories()
                    enabled: applyCategoryCheck.checked
                    Layout.fillWidth: true
                }
            }

            // Date
            RowLayout {
                Kirigami.FormData.label: qsTr("Date:")
                QQC2.CheckBox {
                    id: applyDateCheck
                    text: qsTr("Update")
                }
                QQC2.TextField {
                    id: dateField
                    placeholderText: "YYYY-MM-DD"
                    enabled: applyDateCheck.checked
                    Layout.fillWidth: true
                }
            }

            // Flow
            RowLayout {
                Kirigami.FormData.label: qsTr("Cash Flow:")
                QQC2.CheckBox {
                    id: applyFlowCheck
                    text: qsTr("Update")
                }
                RowLayout {
                    enabled: applyFlowCheck.checked
                    QQC2.ButtonGroup { id: flowGroup }
                    QQC2.RadioButton {
                        id: expenseBtn
                        text: qsTr("Expense")
                        QQC2.ButtonGroup.group: flowGroup
                        checked: true
                    }
                    QQC2.RadioButton {
                        id: incomeBtn
                        text: qsTr("Income")
                        QQC2.ButtonGroup.group: flowGroup
                    }
                }
            }
        }

        QQC2.Label {
            id: errorLabel
            color: Kirigami.Theme.negativeTextColor
            visible: text.length > 0
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
