import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: qsTr("Monthly Budgets")

    actions: [
        Kirigami.Action {
            text: qsTr("Set Global Budget")
            icon.name: "wallet-open"
            onTriggered: globalBudgetDialog.open()
        },
        Kirigami.Action {
            text: qsTr("Add Category Budget")
            icon.name: "list-add"
            onTriggered: categoryBudgetDialog.openForAdd()
        }
    ]

    header: RowLayout {
        width: page.width
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            text: qsTr("Budgeting Period:")
            font.weight: Font.DemiBold
        }

        QQC2.ComboBox {
            id: monthCombo
            model: [
                qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"),
                qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"),
                qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")
            ]
            currentIndex: {
                const now = new Date();
                return now.getMonth();
            }
            onActivated: {
                budgetManager.activeMonth = currentIndex + 1;
            }
        }

        QQC2.ComboBox {
            id: yearCombo
            model: statsManager.getAvailableYears()
            currentIndex: 0
            onActivated: {
                budgetManager.activeYear = parseInt(currentText);
            }
        }

        Item { Layout.fillWidth: true }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: page.width

        // Global Budget Overview Card
        Kirigami.Card {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing

            header: RowLayout {
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.Icon {
                    source: "wallet-open"
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }

                QQC2.Label {
                    text: qsTr("Overall Monthly Budget")
                    font.weight: Font.Bold
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.2
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    radius: 4
                    implicitHeight: 24
                    implicitWidth: statusText.implicitWidth + 16
                    color: !budgetManager.hasGlobalBudget ? Kirigami.Theme.alternateBackgroundColor :
                           budgetManager.isGlobalOverBudget ? "#e74c3c" :
                           budgetManager.globalPercentage > 0.8 ? "#f39c12" : "#2ecc71"

                    QQC2.Label {
                        id: statusText
                        anchors.centerIn: parent
                        text: !budgetManager.hasGlobalBudget ? qsTr("No Limit Set") :
                              budgetManager.isGlobalOverBudget ? qsTr("OVER BUDGET") :
                              budgetManager.globalPercentage > 0.8 ? qsTr("WARNING (>80%)") : qsTr("ON TRACK")
                        color: !budgetManager.hasGlobalBudget ? Kirigami.Theme.textColor : "#ffffff"
                        font.weight: Font.Bold
                        font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    }
                }
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label { text: qsTr("Spent So Far"); color: Kirigami.Theme.disabledTextColor }
                        QQC2.Label {
                            text: budgetManager.globalSpent.toFixed(2) + " €"
                            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.4
                            font.weight: Font.Bold
                            color: budgetManager.isGlobalOverBudget ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
                        }
                    }

                    Item { Layout.fillWidth: true }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label { text: qsTr("Monthly Cap"); color: Kirigami.Theme.disabledTextColor }
                        QQC2.Label {
                            text: budgetManager.hasGlobalBudget ? (budgetManager.globalLimit.toFixed(2) + " €") : qsTr("None")
                            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.4
                            font.weight: Font.Bold
                        }
                    }

                    Item { Layout.fillWidth: true }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label { text: qsTr("Remaining"); color: Kirigami.Theme.disabledTextColor }
                        QQC2.Label {
                            text: budgetManager.hasGlobalBudget ? (budgetManager.globalRemaining.toFixed(2) + " €") : "—"
                            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.4
                            font.weight: Font.Bold
                            color: budgetManager.globalRemaining >= 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                        }
                    }
                }

                // Progress Bar
                Rectangle {
                    Layout.fillWidth: true
                    height: 12
                    radius: 6
                    color: Kirigami.Theme.alternateBackgroundColor
                    visible: budgetManager.hasGlobalBudget

                    Rectangle {
                        width: Math.min(parent.width, parent.width * budgetManager.globalPercentage)
                        height: parent.height
                        radius: 6
                        color: budgetManager.isGlobalOverBudget ? "#e74c3c" :
                               budgetManager.globalPercentage > 0.8 ? "#f39c12" : "#2ecc71"
                    }
                }

                QQC2.Button {
                    text: budgetManager.hasGlobalBudget ? qsTr("Modify Global Budget") : qsTr("Set Global Budget")
                    icon.name: "document-edit"
                    onClicked: globalBudgetDialog.open()
                }
            }
        }

        // Category Budgets Section
        Kirigami.Card {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing

            header: RowLayout {
                spacing: Kirigami.Units.mediumSpacing

                QQC2.Label {
                    text: qsTr("Category Budget Allocations")
                    font.weight: Font.Bold
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
                }

                Item { Layout.fillWidth: true }

                QQC2.Button {
                    text: qsTr("Add Category Budget")
                    icon.name: "list-add"
                    onClicked: categoryBudgetDialog.openForAdd()
                }
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.PlaceholderMessage {
                    visible: budgetList.count === 0
                    text: qsTr("No category budgets configured")
                    explanation: qsTr("Set specific limits for categories like Groceries, Entertainment, etc.")
                    icon.name: "view-financial-budget"
                }

                ListView {
                    id: budgetList
                    Layout.fillWidth: true
                    implicitHeight: contentHeight
                    interactive: false
                    model: budgetManager

                    delegate: QQC2.ItemDelegate {
                        width: budgetList.width

                        contentItem: ColumnLayout {
                            spacing: Kirigami.Units.smallSpacing

                            RowLayout {
                                Layout.fillWidth: true

                                QQC2.Label {
                                    text: model.category
                                    font.weight: Font.Bold
                                    Layout.fillWidth: true
                                }

                                QQC2.Label {
                                    text: qsTr("%1 € / %2 €").arg(model.spent.toFixed(2)).arg(model.limit.toFixed(2))
                                    font.weight: Font.DemiBold
                                    color: model.isOverBudget ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
                                }

                                QQC2.Label {
                                    text: "(" + (model.percentage * 100).toFixed(0) + "%)"
                                    color: model.isOverBudget ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
                                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                                }

                                QQC2.ToolButton {
                                    icon.name: "document-edit"
                                    QQC2.ToolTip.text: qsTr("Edit Budget")
                                    QQC2.ToolTip.visible: hovered
                                    onClicked: {
                                        categoryBudgetDialog.openForEdit(model.category, model.limit);
                                    }
                                }

                                QQC2.ToolButton {
                                    icon.name: "edit-delete"
                                    QQC2.ToolTip.text: qsTr("Remove Budget")
                                    QQC2.ToolTip.visible: hovered
                                    onClicked: {
                                        budgetManager.removeCategoryBudget(model.category);
                                    }
                                }
                            }

                            // Category Progress Bar
                            Rectangle {
                                Layout.fillWidth: true
                                height: 8
                                radius: 4
                                color: Kirigami.Theme.alternateBackgroundColor

                                Rectangle {
                                    width: Math.min(parent.width, parent.width * Math.min(model.percentage, 1.0))
                                    height: parent.height
                                    radius: 4
                                    color: model.isOverBudget ? "#e74c3c" :
                                           model.percentage > 0.8 ? "#f39c12" : "#2ecc71"
                                }
                            }

                            QQC2.Label {
                                text: model.remaining >= 0 ?
                                      qsTr("%1 € remaining").arg(model.remaining.toFixed(2)) :
                                      qsTr("Exceeded by %1 €!").arg((-model.remaining).toFixed(2))
                                font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                                color: model.remaining >= 0 ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.negativeTextColor
                            }
                        }
                    }
                }
            }
        }
    }

    // Dialog for Global Budget
    Kirigami.Dialog {
        id: globalBudgetDialog
        title: qsTr("Global Monthly Budget")
        standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Cancel

        onOpened: {
            globalLimitInput.text = budgetManager.globalLimit > 0 ? budgetManager.globalLimit.toString() : "";
        }

        onAccepted: {
            const val = parseFloat(globalLimitInput.text.replace(',', '.'));
            if (!isNaN(val)) {
                budgetManager.setGlobalBudget(val);
            }
        }

        Kirigami.FormLayout {
            QQC2.TextField {
                id: globalLimitInput
                Kirigami.FormData.label: qsTr("Monthly Spending Cap (€):")
                placeholderText: "e.g. 2000.00"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
            }
            QQC2.Label {
                text: qsTr("Enter 0 or leave empty to disable global limit.")
                color: Kirigami.Theme.disabledTextColor
                font.pixelSize: Kirigami.Theme.smallFont.pixelSize
            }
        }
    }

    // Dialog for Category Budget
    Kirigami.Dialog {
        id: categoryBudgetDialog
        title: isCategoryEditing ? qsTr("Edit Category Budget") : qsTr("Add Category Budget")
        standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Cancel

        property bool isCategoryEditing: false

        function openForAdd() {
            isCategoryEditing = false;
            categoryLimitInput.text = "";
            catCombo.currentIndex = 0;
            open();
        }

        function openForEdit(categoryName, limit) {
            isCategoryEditing = true;
            categoryLimitInput.text = limit.toString();
            const cats = dbManager.getCategories();
            const idx = cats.indexOf(categoryName);
            if (idx >= 0) catCombo.currentIndex = idx;
            open();
        }

        onAccepted: {
            const val = parseFloat(categoryLimitInput.text.replace(',', '.'));
            const cat = catCombo.currentText;
            if (!isNaN(val) && val > 0 && cat.length > 0) {
                budgetManager.setCategoryBudget(cat, val);
            }
        }

        Kirigami.FormLayout {
            QQC2.ComboBox {
                id: catCombo
                Kirigami.FormData.label: qsTr("Category:")
                model: dbManager.getCategories()
                enabled: !categoryBudgetDialog.isCategoryEditing
            }

            QQC2.TextField {
                id: categoryLimitInput
                Kirigami.FormData.label: qsTr("Monthly Limit (€):")
                placeholderText: "e.g. 400.00"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
            }
        }
    }
}
