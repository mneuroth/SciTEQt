/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Window 2.3

Window {
    id: root

    signal canceled()
    signal accepted()

    property alias fcnLocalisation: dialog.fcnLocalisation

    property alias isModal: dialog.isModal

    property alias cancelButton: dialog.cancelButton
    property alias setButton: dialog.setButton
    property alias cmdInput: dialog.cmdInput
    property alias parameter1Input: dialog.parameter1Input
    property alias parameter2Input: dialog.parameter2Input
    property alias parameter3Input: dialog.parameter3Input
    property alias parameter4Input: dialog.parameter4Input
    property alias grid: dialog.grid

    ParametersDialog {
        id: dialog
    }

    Connections {
        target: dialog

        function onAccepted() { root.accepted() }
        function onCanceled() { root.canceled() }
    }
}
