import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: isEditing ? qsTr("Edit Transaction") : qsTr("Add Transaction")
    standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Cancel

    property bool isEditing: false
    property int editId: -1

    function openForAdd() {
        isEditing = false;
        editId = -1;
        typeGroup.checkState = Qt.Unchecked;
        expenseBtn.checked = true;
        amountField.text = "";
        titleField.text = "";
        const now = new Date();
        const year = now.getFullYear();
        const month = String(now.getMonth() + 1).padStart(2, '0');
        const day = String(now.getDate()).padStart(2, '0');
        dateField.text = year + "-" + month + "-" + day;
        noteField.text = "";
        categoryCombo.currentIndex = 0;
        categoryCustomField.text = "";
        colorCustomField.text = "#3daee9";
        errorLabel.text = "";
        open();
    }

    function openForEdit(id, type, amount, category, date, note, title) {
        isEditing = true;
        editId = id;
        if (type === "income") {
            incomeBtn.checked = true;
        } else {
            expenseBtn.checked = true;
        }
        amountField.text = amount.toString();
        titleField.text = title || "";
        dateField.text = date;
        noteField.text = note;
        errorLabel.text = "";

        const cats = dbManager.getCategories();
        const idx = cats.indexOf(category);
        if (idx >= 0) {
            categoryCombo.currentIndex = idx;
            categoryCustomField.text = "";
            colorCustomField.text = "";
        } else {
            categoryCombo.currentIndex = -1;
            categoryCustomField.text = category;
            colorCustomField.text = "#3daee9";
        }
        open();
    }

    customFooterActions: [
        Kirigami.Action {
            text: qsTr("Save")
            icon.name: "document-save"
            onTriggered: dialog.accept()
        },
        Kirigami.Action {
            text: qsTr("Cancel")
            icon.name: "dialog-cancel"
            onTriggered: dialog.reject()
        }
    ]

    onAccepted: {
        const amt = parseFloat(amountField.text.replace(',', '.'));
        if (isNaN(amt) || amt <= 0) {
            errorLabel.text = qsTr("Please enter a valid positive amount.");
            dialog.open();
            return;
        }

        const dateRegex = /^\d{4}-\d{2}-\d{2}$/;
        if (!dateRegex.test(dateField.text.trim())) {
            errorLabel.text = qsTr("Date must be in YYYY-MM-DD format.");
            dialog.open();
            return;
        }

        let cat = categoryCustomField.text.trim();
        let catColor = colorCustomField.text.trim();
        if (cat.length === 0 && categoryCombo.currentText.length > 0) {
            cat = categoryCombo.currentText;
        }
        if (cat.length === 0) {
            cat = "Other";
        }

        const transType = incomeBtn.checked ? "income" : "expense";
        
        if (categoryCustomField.text.trim().length > 0) {
            dbManager.addCategoryWithColor(cat, catColor);
        }

        let success = false;
        if (isEditing) {
            success = transactionModel.updateTransaction(editId, transType, amt, cat, dateField.text.trim(), noteField.text.trim(), titleField.text.trim());
        } else {
            success = transactionModel.addTransaction(transType, amt, cat, dateField.text.trim(), noteField.text.trim(), titleField.text.trim());
        }

        if (!success) {
            errorLabel.text = qsTr("Failed to save transaction.");
            dialog.open();
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing
        width: parent.width

        QQC2.ButtonGroup { id: typeGroup }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing

            QQC2.RadioButton {
                id: expenseBtn
                text: qsTr("Expense")
                checked: true
                QQC2.ButtonGroup.group: typeGroup
            }

            QQC2.RadioButton {
                id: incomeBtn
                text: qsTr("Income")
                QQC2.ButtonGroup.group: typeGroup
            }
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QQC2.TextField {
                id: amountField
                Kirigami.FormData.label: qsTr("Amount (€):")
                placeholderText: "0.00"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                validator: DoubleValidator { bottom: 0.01; decimals: 2; notation: DoubleValidator.StandardNotation }
                Layout.fillWidth: true
            }

            QQC2.TextField {
                id: titleField
                Kirigami.FormData.label: qsTr("Title:")
                placeholderText: qsTr("Short descriptive title...")
                Layout.fillWidth: true
            }

            QQC2.ComboBox {
                id: categoryCombo
                Kirigami.FormData.label: qsTr("Category:")
                model: dbManager.getCategories()
                Layout.fillWidth: true
            }

            QQC2.TextField {
                id: categoryCustomField
                Kirigami.FormData.label: qsTr("Or New Category:")
                placeholderText: qsTr("Custom category name...")
                Layout.fillWidth: true
            }
            
            QQC2.TextField {
                id: colorCustomField
                Kirigami.FormData.label: qsTr("New Cat Color:")
                placeholderText: qsTr("Hex color, e.g. #ff0000")
                visible: categoryCustomField.text.length > 0
                Layout.fillWidth: true
            }

            QQC2.TextField {
                id: dateField
                Kirigami.FormData.label: qsTr("Date (YYYY-MM-DD):")
                placeholderText: "YYYY-MM-DD"
                Layout.fillWidth: true
            }

            QQC2.TextField {
                id: noteField
                Kirigami.FormData.label: qsTr("Note / Description:")
                placeholderText: qsTr("Additional details...")
                Layout.fillWidth: true
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
