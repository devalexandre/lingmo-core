/*
 * Copyright (C) 2026 LingmoOS Team.
 *
 * Author:     devalexandre <alexandre@dev2learn.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import LingmoUI.CompatibleModule 3.0 as LingmoUI

Item {
    id: root

    width: 400
    height: 480

    // Where the pointer was when the popup opened: hovering only counts once it moves
    property point openCursor: Qt.point(-1, -1)

    Connections {
        target: historyWindow
        function onOpened() {
            root.openCursor = historyWindow.cursorPosition()
            searchField.text = ""
            listView.currentIndex = 0
            listView.positionViewAtBeginning()
            searchField.forceActiveFocus()
        }
    }

    function activateCurrent() {
        if (listView.currentIndex >= 0 && listView.count > 0)
            historyWindow.activate(listView.currentIndex)
    }

    function removeCurrent() {
        if (listView.currentIndex < 0 || listView.count === 0)
            return
        var index = listView.currentIndex
        historyWindow.remove(index)
        listView.currentIndex = Math.min(index, listView.count - 1)
    }

    Rectangle {
        id: background
        anchors.fill: parent
        radius: LingmoUI.Theme.bigRadius
        color: LingmoUI.Theme.secondBackgroundColor
        opacity: 0.95

        border.width: 1 / LingmoUI.Units.devicePixelRatio
        border.color: LingmoUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.1)
                                              : Qt.rgba(0, 0, 0, 0.05)
    }

    LingmoUI.WindowShadow {
        view: historyWindow
        radius: background.radius
    }

    LingmoUI.WindowBlur {
        view: historyWindow
        geometry: Qt.rect(historyWindow.x, historyWindow.y, historyWindow.width, historyWindow.height)
        windowRadius: background.radius
        enabled: true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: LingmoUI.Units.largeSpacing
        spacing: LingmoUI.Units.largeSpacing

        Label {
            text: qsTr("Clipboard history")
            font.pointSize: 13
            font.bold: true
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Search")
            selectByMouse: true
            focus: true
            onTextChanged: {
                historyWindow.setFilter(text)
                listView.currentIndex = 0
            }

            Keys.onEscapePressed: historyWindow.hide()
            Keys.onReturnPressed: root.activateCurrent()
            Keys.onEnterPressed: root.activateCurrent()
            Keys.onUpPressed: listView.decrementCurrentIndex()
            Keys.onDownPressed: listView.incrementCurrentIndex()
            Keys.onPressed: function(event) {
                // Delete removes the selected item, unless it would delete searched text
                if (event.key === Qt.Key_Delete && searchField.cursorPosition === searchField.text.length) {
                    root.removeCurrent()
                    event.accepted = true
                } else if (event.key === Qt.Key_PageDown) {
                    listView.currentIndex = Math.min(listView.count - 1, listView.currentIndex + 5)
                    event.accepted = true
                } else if (event.key === Qt.Key_PageUp) {
                    listView.currentIndex = Math.max(0, listView.currentIndex - 5)
                    event.accepted = true
                }
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: historyModel
            clip: true
            spacing: LingmoUI.Units.smallSpacing
            highlightMoveDuration: 0
            keyNavigationEnabled: false
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {}

            Label {
                anchors.centerIn: parent
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: LingmoUI.Theme.disabledTextColor
                visible: listView.count === 0
                text: !clipboard.historyEnabled ? qsTr("Clipboard history is turned off in Settings")
                                                : searchField.text !== "" ? qsTr("Nothing found")
                                                                          : qsTr("Things you copy will show up here")
            }

            delegate: Item {
                id: delegate
                width: ListView.view.width - (listView.ScrollBar.vertical.visible ? listView.ScrollBar.vertical.width : 0)
                height: model.isImage ? 96 : Math.max(44, textLabel.implicitHeight + LingmoUI.Units.largeSpacing * 2)

                readonly property bool selected: ListView.isCurrentItem

                Rectangle {
                    anchors.fill: parent
                    radius: LingmoUI.Theme.mediumRadius
                    color: delegate.selected ? LingmoUI.Theme.highlightColor
                                             : LingmoUI.Theme.darkMode ? "white" : "black"
                    opacity: delegate.selected ? 1 : mouseArea.containsMouse ? 0.08 : 0.04
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    // Only a real move: the popup opens under a still pointer
                    onPositionChanged: function(mouse) {
                        var point = mapToGlobal(mouse.x, mouse.y)
                        if (Math.abs(point.x - root.openCursor.x) > 2 || Math.abs(point.y - root.openCursor.y) > 2) {
                            root.openCursor = Qt.point(-1, -1)
                            listView.currentIndex = index
                        }
                    }
                    onClicked: historyWindow.activate(index)
                }

                Label {
                    id: textLabel
                    visible: !model.isImage
                    anchors.left: parent.left
                    anchors.right: removeButton.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: LingmoUI.Units.largeSpacing
                    anchors.rightMargin: LingmoUI.Units.smallSpacing
                    // One line per copied line, no more than three
                    text: model.isImage ? "" : model.text.trim().replace(/\t/g, "    ")
                    textFormat: Text.PlainText
                    wrapMode: Text.WrapAnywhere
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    color: delegate.selected ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.textColor
                }

                Image {
                    visible: model.isImage
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: LingmoUI.Units.smallSpacing
                    anchors.leftMargin: LingmoUI.Units.largeSpacing
                    width: Math.min(parent.width - removeButton.width - LingmoUI.Units.largeSpacing * 3,
                                    height * Math.max(1, model.imageSize.width / Math.max(1, model.imageSize.height)))
                    source: model.isImage ? "image://clipboard/" + model.itemId : ""
                    sourceSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignLeft
                    asynchronous: true
                    cache: false
                }

                Label {
                    visible: model.isImage
                    anchors.right: removeButton.left
                    anchors.bottom: parent.bottom
                    anchors.margins: LingmoUI.Units.smallSpacing
                    text: model.isImage ? qsTr("%1 × %2").arg(model.imageSize.width).arg(model.imageSize.height) : ""
                    color: delegate.selected ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.disabledTextColor
                    font.pointSize: 8
                }

                Item {
                    id: removeButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: LingmoUI.Units.smallSpacing
                    width: 28
                    height: 28
                    visible: delegate.selected || mouseArea.containsMouse || removeArea.containsMouse

                    Rectangle {
                        anchors.fill: parent
                        radius: LingmoUI.Theme.smallRadius
                        color: delegate.selected ? "white" : LingmoUI.Theme.textColor
                        opacity: removeArea.containsMouse ? 0.2 : 0
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "\u2715"
                        color: delegate.selected ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.textColor
                    }

                    MouseArea {
                        id: removeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        ToolTip.text: qsTr("Remove")
                        ToolTip.visible: containsMouse
                        ToolTip.delay: 600
                        onClicked: {
                            listView.currentIndex = index
                            root.removeCurrent()
                            searchField.forceActiveFocus()
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: listView.count > 0 || searchField.text !== ""

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.pointSize: 8
                color: LingmoUI.Theme.disabledTextColor
                text: qsTr("Enter to paste · Delete to remove · Esc to close")
            }

            Button {
                text: qsTr("Clear history")
                flat: true
                visible: clipboard.historyEnabled
                onClicked: {
                    historyWindow.clear()
                    searchField.forceActiveFocus()
                }
            }
        }
    }
}
