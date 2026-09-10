import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: qsTr("Financial Trends & Graphs")

    property int activeTab: 1 // 0: Week, 1: Month, 2: Year, 3: Categories
    property var chartData: []

    function reloadChart() {
        if (activeTab === 0) {
            chartData = statsManager.getWeeklyData(12);
        } else if (activeTab === 1) {
            chartData = statsManager.getMonthlyData(statsManager.selectedYear);
        } else if (activeTab === 2) {
            chartData = statsManager.getYearlyData();
        } else if (activeTab === 3) {
            chartData = statsManager.getCategoryBreakdown(statsManager.selectedYear, 0, "expense");
        }
    }

    Component.onCompleted: {
        reloadChart();
    }

    Connections {
        target: statsManager
        function onStatsUpdated() {
            reloadChart();
        }
        function onSelectedYearChanged() {
            reloadChart();
        }
    }

    header: ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing
        width: page.width

        // Year Selector & Tab Bar
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            QQC2.TabBar {
                id: periodTabBar
                currentIndex: activeTab
                onCurrentIndexChanged: {
                    activeTab = currentIndex;
                    reloadChart();
                }

                QQC2.TabButton { text: qsTr("By Week") }
                QQC2.TabButton { text: qsTr("By Month") }
                QQC2.TabButton { text: qsTr("By Year") }
                QQC2.TabButton { text: qsTr("Categories") }
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                visible: activeTab !== 2 // Don't show year selector on "By Year" tab
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label { text: qsTr("Year:") }
                QQC2.ComboBox {
                    id: chartYearCombo
                    model: statsManager.getAvailableYears()
                    currentIndex: 0
                    onActivated: {
                        statsManager.selectedYear = parseInt(currentText);
                    }
                }
            }
        }

        // Summary KPI Banner
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.mediumSpacing
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Card {
                Layout.fillWidth: true
                header: QQC2.Label {
                    text: qsTr("Total Earned")
                    color: Kirigami.Theme.positiveTextColor
                    font.weight: Font.DemiBold
                    padding: Kirigami.Units.smallSpacing
                }
                contentItem: QQC2.Label {
                    text: statsManager.periodIncome.toFixed(2) + " €"
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.3
                    font.weight: Font.Bold
                    color: Kirigami.Theme.positiveTextColor
                }
            }

            Kirigami.Card {
                Layout.fillWidth: true
                header: QQC2.Label {
                    text: qsTr("Total Spent")
                    color: Kirigami.Theme.negativeTextColor
                    font.weight: Font.DemiBold
                    padding: Kirigami.Units.smallSpacing
                }
                contentItem: QQC2.Label {
                    text: statsManager.periodExpense.toFixed(2) + " €"
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.3
                    font.weight: Font.Bold
                    color: Kirigami.Theme.negativeTextColor
                }
            }

            Kirigami.Card {
                Layout.fillWidth: true
                header: QQC2.Label {
                    text: qsTr("Net Savings")
                    font.weight: Font.DemiBold
                    color: statsManager.periodNet >= 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                    padding: Kirigami.Units.smallSpacing
                }
                contentItem: QQC2.Label {
                    text: (statsManager.periodNet >= 0 ? "+" : "") + statsManager.periodNet.toFixed(2) + " €"
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.3
                    font.weight: Font.Bold
                    color: statsManager.periodNet >= 0 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                }
            }

            Kirigami.Card {
                Layout.fillWidth: true
                header: QQC2.Label {
                    text: qsTr("Savings Rate")
                    font.weight: Font.DemiBold
                    padding: Kirigami.Units.smallSpacing
                }
                contentItem: QQC2.Label {
                    text: statsManager.savingsRate.toFixed(1) + " %"
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.3
                    font.weight: Font.Bold
                    color: statsManager.savingsRate >= 20 ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }
    }

    // Chart Area
    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: page.width

        // Bar / Trend Chart (Week, Month, Year)
        Kirigami.Card {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            visible: activeTab !== 3

            header: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                QQC2.Label {
                    text: activeTab === 0 ? qsTr("Weekly Income vs Expenses") :
                          activeTab === 1 ? qsTr("Monthly Income vs Expenses") : qsTr("Annual Overview")
                    font.weight: Font.Bold
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
                }

                Item { Layout.fillWidth: true }

                // Legend
                RowLayout {
                    spacing: Kirigami.Units.mediumSpacing
                    Rectangle { width: 12; height: 12; radius: 2; color: "#2ecc71" }
                    QQC2.Label { text: qsTr("Income"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize }
                    Rectangle { width: 12; height: 12; radius: 2; color: "#e74c3c" }
                    QQC2.Label { text: qsTr("Expense"); font.pixelSize: Kirigami.Theme.smallFont.pixelSize }
                }
            }

            contentItem: Item {
                implicitHeight: 280
                implicitWidth: parent.width

                property double maxVal: {
                    let m = 100.0;
                    for (let i = 0; i < chartData.length; ++i) {
                        if (chartData[i].income > m) m = chartData[i].income;
                        if (chartData[i].expense > m) m = chartData[i].expense;
                    }
                    return m * 1.15; // 15% head room
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.topMargin: Kirigami.Units.mediumSpacing
                    anchors.bottomMargin: Kirigami.Units.mediumSpacing
                    spacing: Math.max(2, (parent.width - (chartData.length * 40)) / (chartData.length + 1))

                    Repeater {
                        model: chartData
                        delegate: ColumnLayout {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.smallSpacing

                            // Bars container
                            Item {
                                Layout.fillHeight: true
                                Layout.fillWidth: true

                                Row {
                                    anchors.bottom: parent.bottom
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    spacing: 4

                                    // Income bar
                                    Rectangle {
                                        width: Math.min(24, Math.max(8, parent.parent.width / 3))
                                        height: Math.max(2, (modelData.income / parent.parent.parent.parent.parent.maxVal) * (parent.parent.height - 25))
                                        color: "#2ecc71"
                                        radius: 3
                                        opacity: 0.9

                                        QQC2.ToolTip.text: qsTr("Earned: %1 €").arg(modelData.income.toFixed(2))
                                        QQC2.ToolTip.visible: incMouse.containsMouse

                                        MouseArea {
                                            id: incMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                        }
                                    }

                                    // Expense bar
                                    Rectangle {
                                        width: Math.min(24, Math.max(8, parent.parent.width / 3))
                                        height: Math.max(2, (modelData.expense / parent.parent.parent.parent.parent.maxVal) * (parent.parent.height - 25))
                                        color: "#e74c3c"
                                        radius: 3
                                        opacity: 0.9

                                        QQC2.ToolTip.text: qsTr("Spent: %1 €").arg(modelData.expense.toFixed(2))
                                        QQC2.ToolTip.visible: expMouse.containsMouse

                                        MouseArea {
                                            id: expMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                        }
                                    }
                                }
                            }

                            // Period Label
                            QQC2.Label {
                                text: modelData.label
                                font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                                Layout.alignment: Qt.AlignHCenter
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }
                }
            }
        }

        // Category Breakdown Card (Active when Category tab selected)
        Kirigami.Card {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            visible: activeTab === 3

            header: QQC2.Label {
                text: qsTr("Spending by Category")
                font.weight: Font.Bold
                font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
                padding: Kirigami.Units.mediumSpacing
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.PlaceholderMessage {
                    visible: chartData.length === 0
                    text: qsTr("No spending recorded")
                    explanation: qsTr("Add expenses to see category breakdown.")
                    icon.name: "office-chart-pie"
                }

                Repeater {
                    model: chartData
                    delegate: ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        RowLayout {
                            Layout.fillWidth: true

                            Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                color: modelData.color
                            }

                            QQC2.Label {
                                text: modelData.category
                                font.weight: Font.DemiBold
                                Layout.fillWidth: true
                            }

                            QQC2.Label {
                                text: modelData.amount.toFixed(2) + " € (" + (modelData.percentage * 100).toFixed(1) + "%)"
                                font.weight: Font.Bold
                                color: Kirigami.Theme.textColor
                            }
                        }

                        // Progress proportion bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 8
                            radius: 4
                            color: Kirigami.Theme.alternateBackgroundColor

                            Rectangle {
                                width: parent.width * modelData.percentage
                                height: parent.height
                                radius: 4
                                color: modelData.color
                            }
                        }
                    }
                }
            }
        }
    }
}
