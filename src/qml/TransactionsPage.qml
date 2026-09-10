import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: qsTr("Transactions")

    actions: [
        Kirigami.Action {
            text: qsTr("Add Transaction")
            icon.name: "list-add"
            onTriggered: addEditDialog.openForAdd()
        },
        Kirigami.Action {
            text: qsTr("Reset Filters")
            icon.name: "view-refresh"
            onTriggered: {
                yearCombo.currentIndex = 0;
                monthCombo.currentIndex = 0;
                typeFilterCombo.currentIndex = 0;
                searchField.text = "";
                transactionModel.resetFilters();
            }
        }
    ]

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing
        width: page.width

        // Summary Cards
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Card {
                Layout.fillWidth: true
                header: QQC2.Label {
                    text: qsTr("Total Income")
                    font.weight: Font.DemiBold
                    color: Kirigami.Theme.positiveTextColor
                    padding: Kirigami.Units.smallSpacing
                }
                contentItem: QQC2.Label {
                    text: "+" + transactionModel.totalIncome.toFixed(2) + " €"
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.5
                    font.weight: Font.Bold
                    color: Kirigami.Theme.positiveTextColor
                }
            }

            Kirigami.Card {
                Layout.fillWidth: true
                header: QQC2.Label {
                    text: qsTr("Total Spent")
                    font.weight: Font.DemiBold
                    color: Kirigami.Theme.negativeTextColor
                    padding: Kirigami.Units.smallSpacing
                }
                contentItem: QQC2.Label {
                    text: "-" + transactionModel.totalExpense.toFixed(2) + " €"
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.5
                    font.weight: Font.Bold
                    color: Kirigami.Theme.negativeTextColor
                }
            }

            Kirigami.Card {
                Layout.fillWidth: true
                header: QQC2.Label {
                    text: qsTr("Net Balance")
                    font.weight: Font.DemiBold
                    color: transactionModel.netBalance >= 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    padding: Kirigami.Units.smallSpacing
                }
                contentItem: QQC2.Label {
                    text: (transactionModel.netBalance >= 0 ? "+" : "") + transactionModel.netBalance.toFixed(2) + " €"
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.5
                    font.weight: Font.Bold
                    color: transactionModel.netBalance >= 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                }
            }
        }

        // Filter Bar
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.mediumSpacing

            QQC2.TextField {
                id: searchField
                placeholderText: qsTr("Search note or category...")
                Layout.fillWidth: true
                onTextChanged: transactionModel.searchQuery = text
            }

            QQC2.ComboBox {
                id: typeFilterCombo
                model: [qsTr("All Types"), qsTr("Expenses"), qsTr("Income")]
                onActivated: {
                    if (currentIndex === 1) transactionModel.filterType = "expense";
                    else if (currentIndex === 2) transactionModel.filterType = "income";
                    else transactionModel.filterType = "all";
                }
            }

            QQC2.ComboBox {
                id: monthCombo
                model: [
                    qsTr("All Months"),
                    qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"),
                    qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"),
                    qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")
                ]
                currentIndex: {
                    const now = new Date();
                    return now.getMonth() + 1; // Default to current month
                }
                onActivated: {
                    transactionModel.filterMonth = currentIndex;
                }
            }

            QQC2.ComboBox {
                id: yearCombo
                model: {
                    const yrs = statsManager.getAvailableYears();
                    const list = [qsTr("All Years")];
                    for (let i = 0; i < yrs.length; ++i) {
                        list.push(yrs[i].toString());
                    }
                    return list;
                }
                currentIndex: 1 // Default to current year (first year after "All")
                onActivated: {
                    if (currentIndex === 0) {
                        transactionModel.filterYear = 0;
                    } else {
                        transactionModel.filterYear = parseInt(currentText);
                    }
                }
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }
    }

    ListView {
        id: transList
        model: transactionModel
        clip: true

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: transList.count === 0
            text: qsTr("No transactions found")
            explanation: qsTr("Click '+ Add Transaction' to record your first income or expense.")
            icon.name: "view-financial-transfer"
        }

        delegate: QQC2.ItemDelegate {
            id: delegateItem
            width: transList.width

            contentItem: RowLayout {
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.Icon {
                    source: model.type === "income" ? "arrow-down-double" : "arrow-up-double"
                    color: model.type === "income" ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            text: model.category
                            font.weight: Font.Bold
                        }

                        QQC2.Label {
                            text: "• " + model.formattedDate
                            color: Kirigami.Theme.disabledTextColor
                            font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                        }
                    }

                    QQC2.Label {
                        text: model.note.length > 0 ? model.note : qsTr("No description")
                        color: model.note.length > 0 ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                        font.italic: model.note.length === 0
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                QQC2.Label {
                    text: model.formattedAmount
                    font.weight: Font.Bold
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.2
                    color: model.type === "income" ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                }

                QQC2.ToolButton {
                    icon.name: "document-edit"
                    display: QQC2.AbstractButton.IconOnly
                    QQC2.ToolTip.text: qsTr("Edit")
                    QQC2.ToolTip.visible: hovered
                    onClicked: {
                        addEditDialog.openForEdit(
                            model.id,
                            model.type,
                            model.amount,
                            model.category,
                            model.date,
                            model.note
                        )
                    }
                }

                QQC2.ToolButton {
                    icon.name: "edit-delete"
                    display: QQC2.AbstractButton.IconOnly
                    QQC2.ToolTip.text: qsTr("Delete")
                    QQC2.ToolTip.visible: hovered
                    onClicked: {
                        transactionModel.deleteTransaction(model.id);
                    }
                }
            }
        }
    }
}
