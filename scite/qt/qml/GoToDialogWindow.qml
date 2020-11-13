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

    property alias fcnLocalisation: dialog.fcnLocalisation

    property alias cancelButton: dialog.cancelButton
    property alias gotoButton: dialog.gotoButton
    property alias destinationLineInput: dialog.destinationLineInput
    property alias columnInput: dialog.columnInput
    property alias currentLineOutput: dialog.currentLineOutput
    property alias currentColumnOutput: dialog.currentColumnOutput
    property alias lastLineOutput: dialog.lastLineOutput
    property alias grid: dialog.grid

    signal canceled()
    signal accepted()

    GoToDialog {
        id: dialog
    }

    Connections {
        target: dialog

        onAccepted: root.accepted()
        onCanceled: root.canceled()
    }
}
