import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: qsTr("Transactions")

    // ─── view mode: 0 = 1-col list, 1 = 2-col grid ──────────────────────────
    property int viewMode: 0
    // ─── mass-selection ──────────────────────────────────────────────────────
    property bool selectionMode: false
    property var selectedIds: ({})
    property int selectedCount: 0

    function toggleSelected(txId) {
        const copy = Object.assign({}, selectedIds);
        if (copy[txId]) { delete copy[txId]; }
        else             { copy[txId] = true; }
        selectedIds = copy;
        selectedCount = Object.keys(copy).length;
    }
    function clearSelection() {
        selectedIds = {};
        selectedCount = 0;
        selectionMode = false;
    }
    function deleteSelected() {
        const ids = Object.keys(selectedIds).map(Number);
        for (const id of ids) transactionModel.deleteTransaction(id);
        clearSelection();
    }

    actions: [
        Kirigami.Action {
            text: qsTr("Add")
            icon.name: "list-add"
            visible: !page.selectionMode
            onTriggered: addEditDialog.openForAdd()
        },
        Kirigami.Action {
            text: page.viewMode === 0 ? qsTr("2-Col View") : qsTr("1-Col View")
            icon.name: page.viewMode === 0 ? "view-grid" : "view-list-details"
            visible: !page.selectionMode
            onTriggered: page.viewMode = (page.viewMode === 0 ? 1 : 0)
        },
        Kirigami.Action {
            text: qsTr("Select")
            icon.name: "edit-select"
            visible: !page.selectionMode
            onTriggered: { page.selectionMode = true; }
        },
        Kirigami.Action {
            text: qsTr("Delete (%1)").arg(page.selectedCount)
            icon.name: "edit-delete"
            visible: page.selectionMode && page.selectedCount > 0
            onTriggered: page.deleteSelected()
        },
        Kirigami.Action {
            text: qsTr("Cancel")
            icon.name: "dialog-cancel"
            visible: page.selectionMode
            onTriggered: page.clearSelection()
        },
        Kirigami.Action {
            text: qsTr("Reset Filters")
            icon.name: "view-refresh"
            visible: !page.selectionMode
            onTriggered: {
                transactionModel.resetFilters();
                page.clearSelection();
            }
        }
    ]

    header: ColumnLayout {
        spacing: 0
        width: page.width

        // ── Summary Cards ────────────────────────────────────────────────────
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
                visible: settingsManager.showNetBalance
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

        // ── Filter Row ───────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.mediumSpacing

            // Search
            ColumnLayout {
                spacing: 2
                Layout.fillWidth: true
                QQC2.Label { text: qsTr("Search"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize; color: Kirigami.Theme.disabledTextColor }
                QQC2.TextField {
                    id: searchField
                    placeholderText: qsTr("Title or description…")
                    Layout.fillWidth: true
                    onTextChanged: transactionModel.searchQuery = text
                }
            }

            // Cash Flow filter
            ColumnLayout {
                spacing: 2
                QQC2.Label { text: qsTr("Cash flow"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize; color: Kirigami.Theme.disabledTextColor }
                QQC2.ComboBox {
                    id: typeFilterCombo
                    model: [qsTr("All"), qsTr("Expenses"), qsTr("Income")]
                    onActivated: {
                        if (currentIndex === 1) transactionModel.filterType = "expense";
                        else if (currentIndex === 2) transactionModel.filterType = "income";
                        else transactionModel.filterType = "all";
                    }
                }
            }

            // Category multi-select
            ColumnLayout {
                spacing: 2
                QQC2.Label { text: qsTr("Category"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize; color: Kirigami.Theme.disabledTextColor }
                QQC2.Button {
                    id: catFilterBtn
                    property var selectedCategories: []
                    text: selectedCategories.length === 0
                          ? qsTr("All categories")
                          : selectedCategories.length === 1
                            ? selectedCategories[0]
                            : qsTr("%1 categories").arg(selectedCategories.length)
                    onClicked: categoryPopup.open()

                    QQC2.Popup {
                        id: categoryPopup
                        width: 220
                        height: Math.min(320, catCheckList.implicitHeight + 16)
                        padding: 8
                        modal: true
                        focus: true

                        ListView {
                            id: catCheckList
                            anchors.fill: parent
                            model: dbManager.getCategories()
                            clip: true
                            delegate: QQC2.CheckDelegate {
                                width: catCheckList.width
                                text: modelData
                                checked: catFilterBtn.selectedCategories.indexOf(modelData) >= 0
                                onToggled: {
                                    let arr = catFilterBtn.selectedCategories.slice();
                                    const idx = arr.indexOf(modelData);
                                    if (idx >= 0) arr.splice(idx, 1);
                                    else arr.push(modelData);
                                    catFilterBtn.selectedCategories = arr;
                                    transactionModel.filterCategories = arr;
                                }
                            }
                        }
                    }
                }
            }

            // Month multi-select
            ColumnLayout {
                spacing: 2
                QQC2.Label { text: qsTr("Month"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize; color: Kirigami.Theme.disabledTextColor }
                QQC2.Button {
                    id: monthFilterBtn
                    property var selectedMonths: []
                    readonly property var monthNames: [
                        qsTr("Jan"), qsTr("Feb"), qsTr("Mar"), qsTr("Apr"),
                        qsTr("May"), qsTr("Jun"), qsTr("Jul"), qsTr("Aug"),
                        qsTr("Sep"), qsTr("Oct"), qsTr("Nov"), qsTr("Dec")
                    ]
                    text: selectedMonths.length === 0
                          ? qsTr("All months")
                          : selectedMonths.length === 1
                            ? monthNames[selectedMonths[0] - 1]
                            : qsTr("%1 months").arg(selectedMonths.length)
                    onClicked: monthPopup.open()

                    QQC2.Popup {
                        id: monthPopup
                        width: 180
                        height: monthCheckList.implicitHeight + 16
                        padding: 8
                        modal: true
                        focus: true

                        ListView {
                            id: monthCheckList
                            anchors.fill: parent
                            model: 12
                            clip: true
                            delegate: QQC2.CheckDelegate {
                                required property int index
                                width: monthCheckList.width
                                text: monthFilterBtn.monthNames[index]
                                checked: monthFilterBtn.selectedMonths.indexOf(index + 1) >= 0
                                onToggled: {
                                    let arr = monthFilterBtn.selectedMonths.slice();
                                    const m = index + 1;
                                    const pos = arr.indexOf(m);
                                    if (pos >= 0) arr.splice(pos, 1);
                                    else arr.push(m);
                                    arr.sort((a,b) => a-b);
                                    monthFilterBtn.selectedMonths = arr;
                                    transactionModel.filterMonths = arr;
                                }
                            }
                        }
                    }
                }
            }

            // Year multi-select
            ColumnLayout {
                spacing: 2
                QQC2.Label { text: qsTr("Year"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize; color: Kirigami.Theme.disabledTextColor }
                QQC2.Button {
                    id: yearFilterBtn
                    property var selectedYears: []
                    text: selectedYears.length === 0
                          ? qsTr("All years")
                          : selectedYears.length === 1
                            ? selectedYears[0].toString()
                            : qsTr("%1 years").arg(selectedYears.length)
                    onClicked: yearPopup.open()

                    QQC2.Popup {
                        id: yearPopup
                        width: 160
                        height: Math.min(300, yearCheckList.implicitHeight + 16)
                        padding: 8
                        modal: true
                        focus: true

                        ListView {
                            id: yearCheckList
                            anchors.fill: parent
                            model: statsManager.getAvailableYears()
                            clip: true
                            delegate: QQC2.CheckDelegate {
                                width: yearCheckList.width
                                text: modelData.toString()
                                checked: yearFilterBtn.selectedYears.indexOf(modelData) >= 0
                                onToggled: {
                                    let arr = yearFilterBtn.selectedYears.slice();
                                    const idx = arr.indexOf(modelData);
                                    if (idx >= 0) arr.splice(idx, 1);
                                    else arr.push(modelData);
                                    arr.sort((a,b) => b-a);
                                    yearFilterBtn.selectedYears = arr;
                                    transactionModel.filterYears = arr;
                                }
                            }
                        }
                    }
                }
            }

            // Amount range
            ColumnLayout {
                spacing: 2
                QQC2.Label { text: qsTr("Amount"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize; color: Kirigami.Theme.disabledTextColor }
                QQC2.Button {
                    id: amountFilterBtn
                    property double minAmt: 0
                    property double maxAmt: 0
                    text: (minAmt <= 0 && maxAmt <= 0) ? qsTr("Any amount")
                          : minAmt > 0 && maxAmt > 0 ? qsTr("%1–%2 €").arg(minAmt.toFixed(0)).arg(maxAmt.toFixed(0))
                          : minAmt > 0 ? qsTr("≥ %1 €").arg(minAmt.toFixed(0))
                          : qsTr("≤ %1 €").arg(maxAmt.toFixed(0))
                    onClicked: amountPopup.open()

                    QQC2.Popup {
                        id: amountPopup
                        width: 230
                        padding: 12
                        modal: true
                        focus: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.Label { text: qsTr("Min amount (€)"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize }
                            QQC2.TextField {
                                id: minAmtField
                                Layout.fillWidth: true
                                placeholderText: "0"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                text: amountFilterBtn.minAmt > 0 ? amountFilterBtn.minAmt.toFixed(0) : ""
                            }

                            QQC2.Label { text: qsTr("Max amount (€)"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize }
                            QQC2.TextField {
                                id: maxAmtField
                                Layout.fillWidth: true
                                placeholderText: "0"
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                text: amountFilterBtn.maxAmt > 0 ? amountFilterBtn.maxAmt.toFixed(0) : ""
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Button {
                                    text: qsTr("Apply")
                                    Layout.fillWidth: true
                                    onClicked: {
                                        const mn = parseFloat(minAmtField.text) || 0;
                                        const mx = parseFloat(maxAmtField.text) || 0;
                                        amountFilterBtn.minAmt = mn;
                                        amountFilterBtn.maxAmt = mx;
                                        transactionModel.filterAmountMin = mn;
                                        transactionModel.filterAmountMax = mx;
                                        amountPopup.close();
                                    }
                                }
                                QQC2.Button {
                                    text: qsTr("Clear")
                                    onClicked: {
                                        minAmtField.text = "";
                                        maxAmtField.text = "";
                                        amountFilterBtn.minAmt = 0;
                                        amountFilterBtn.maxAmt = 0;
                                        transactionModel.filterAmountMin = 0;
                                        transactionModel.filterAmountMax = 0;
                                        amountPopup.close();
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Sort order toggle
            ColumnLayout {
                spacing: 2
                QQC2.Label { text: qsTr("Order"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize; color: Kirigami.Theme.disabledTextColor }
                QQC2.Button {
                    icon.name: transactionModel.sortAscending ? "view-sort-ascending" : "view-sort-descending"
                    QQC2.ToolTip.text: transactionModel.sortAscending ? qsTr("Oldest first") : qsTr("Newest first")
                    QQC2.ToolTip.visible: hovered
                    onClicked: transactionModel.sortAscending = !transactionModel.sortAscending
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }
    }

    // ── Transaction List / Grid ───────────────────────────────────────────────
    GridView {
        id: txGrid
        model: transactionModel
        clip: true

        // 1-col or 2-col
        readonly property int cols: page.viewMode === 1 ? 2 : 1
        cellWidth: Math.floor(width / cols)
        cellHeight: page.viewMode === 1 ? 96 : 76

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.largeSpacing * 4
            visible: txGrid.count === 0
            text: qsTr("No transactions found")
            explanation: qsTr("Add a transaction or adjust your filters.")
            icon.name: "view-financial-transfer"
        }

        delegate: Item {
            id: delegateRoot
            width: txGrid.cellWidth
            height: txGrid.cellHeight

            required property int index
            required property var model

            readonly property bool isSelected: page.selectedIds[model.id] === true
            readonly property string catColor: dbManager.getCategoryColor(model.category)

            Rectangle {
                anchors.fill: parent
                anchors.margins: 3
                radius: 6
                color: delegateRoot.isSelected
                       ? Qt.alpha(Kirigami.Theme.highlightColor, 0.25)
                       : (delegateRoot.index % 2 === 0
                          ? Kirigami.Theme.backgroundColor
                          : Kirigami.Theme.alternateBackgroundColor)
                border.color: delegateRoot.isSelected ? Kirigami.Theme.highlightColor : "transparent"
                border.width: delegateRoot.isSelected ? 2 : 0

                RowLayout {
                    anchors { fill: parent; leftMargin: 10; rightMargin: 6; topMargin: 6; bottomMargin: 6 }
                    spacing: Kirigami.Units.mediumSpacing

                    // selection checkbox (visible in selection mode)
                    QQC2.CheckBox {
                        visible: page.selectionMode
                        checked: delegateRoot.isSelected
                        onToggled: page.toggleSelected(delegateRoot.model.id)
                    }

                    // type icon
                    Kirigami.Icon {
                        source: delegateRoot.model.type === "income" ? "arrow-down-double" : "arrow-up-double"
                        color: delegateRoot.model.type === "income" ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                    }

                    // Title + date + category chip + note
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.smallSpacing

                            // Transaction title (main text)
                            QQC2.Label {
                                text: delegateRoot.model.title.length > 0
                                      ? delegateRoot.model.title
                                      : delegateRoot.model.category
                                font.weight: Font.Bold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            // Date
                            QQC2.Label {
                                text: delegateRoot.model.formattedDate
                                color: Kirigami.Theme.disabledTextColor
                                font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                            }

                            // Category chip
                            Rectangle {
                                radius: 8
                                color: Qt.alpha(delegateRoot.catColor, 0.2)
                                implicitWidth: chipLabel.implicitWidth + 12
                                implicitHeight: chipLabel.implicitHeight + 4
                                QQC2.Label {
                                    id: chipLabel
                                    anchors.centerIn: parent
                                    text: delegateRoot.model.category
                                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize - 1
                                    color: delegateRoot.catColor
                                }
                            }
                        }

                        // Note / description (hide in 2-col compact mode)
                        QQC2.Label {
                            visible: page.viewMode === 0 && delegateRoot.model.note.length > 0
                            text: delegateRoot.model.note
                            color: Kirigami.Theme.disabledTextColor
                            font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    // Amount
                    QQC2.Label {
                        text: delegateRoot.model.formattedAmount
                        font.weight: Font.Bold
                        font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
                        color: delegateRoot.model.type === "income" ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    }

                    // Edit/Delete buttons (hidden in compact 2-col and selection mode)
                    QQC2.ToolButton {
                        visible: page.viewMode === 0 && !page.selectionMode
                        icon.name: "document-edit"
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: qsTr("Edit")
                        QQC2.ToolTip.visible: hovered
                        onClicked: addEditDialog.openForEdit(
                            delegateRoot.model.id, delegateRoot.model.type,
                            delegateRoot.model.amount, delegateRoot.model.category,
                            delegateRoot.model.date, delegateRoot.model.note,
                            delegateRoot.model.title
                        )
                    }
                    QQC2.ToolButton {
                        visible: page.viewMode === 0 && !page.selectionMode
                        icon.name: "edit-delete"
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: qsTr("Delete")
                        QQC2.ToolTip.visible: hovered
                        onClicked: transactionModel.deleteTransaction(delegateRoot.model.id)
                    }
                }

                // tap anywhere in selection mode = toggle; otherwise long-press to enter selection
                MouseArea {
                    anchors.fill: parent
                    enabled: page.selectionMode
                    onClicked: page.toggleSelected(delegateRoot.model.id)
                }
                TapHandler {
                    enabled: !page.selectionMode
                    onLongPressed: {
                        page.selectionMode = true;
                        page.toggleSelected(delegateRoot.model.id);
                    }
                }
            }
        }
    }
}
