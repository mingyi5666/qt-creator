/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: actionIndicator

    property Item myControl

    property bool showBackground: true
    property alias icon: actionIndicatorIcon

    property bool hover: false
    property bool pressed: false

    color: actionIndicator.showBackground ? StudioTheme.Values.themeControlBackground : "transparent"
    border.color: actionIndicator.showBackground ? StudioTheme.Values.themeControlOutline : "transparent"

    implicitWidth: StudioTheme.Values.height
    implicitHeight: StudioTheme.Values.height

    signal clicked

    T.Label {
        id: actionIndicatorIcon
        anchors.fill: parent
        text: StudioTheme.Constants.actionIcon
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.iconFont.family
        font.pixelSize: StudioTheme.Values.myIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        states: [
            State {
                name: "hovered"
                when: actionIndicator.hover && !actionIndicator.pressed
                      && !myControl.edit && !myControl.drag && myControl.enabled
                PropertyChanges {
                    target: actionIndicatorIcon
                    scale: 1.2
                }
            },
            State {
                name: "disabled"
                when: !myControl.enabled
                PropertyChanges {
                    target: actionIndicatorIcon
                    color: StudioTheme.Values.themeTextColorDisabled
                }
            }
        ]
    }

    MouseArea {
        id: actionIndicatorMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onContainsMouseChanged: actionIndicator.hover = containsMouse
        onClicked: actionIndicator.clicked()
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !actionIndicator.hover
                  && !actionIndicator.pressed && !myControl.hover
                  && !myControl.edit && !myControl.drag && actionIndicator.showBackground
            PropertyChanges {
                target: actionIndicator
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "globalHover"
            when: myControl.hover && !actionIndicator.hover
                  && !actionIndicator.pressed && !myControl.edit
                  && !myControl.drag && actionIndicator.showBackground
            PropertyChanges {
                target: actionIndicator
                color: StudioTheme.Values.themeHoverHighlight
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "edit"
            when: myControl.edit && actionIndicator.showBackground
            PropertyChanges {
                target: actionIndicator
                color: StudioTheme.Values.themeFocusEdit
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "drag"
            when: myControl.drag && actionIndicator.showBackground
            PropertyChanges {
                target: actionIndicator
                color: StudioTheme.Values.themeFocusDrag
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disabled"
            when: !myControl.enabled && actionIndicator.showBackground
            PropertyChanges {
                target: actionIndicator
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
        }
    ]
}