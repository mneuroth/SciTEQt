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

    property alias cancelButton: dialog.cancelButton
    property alias insertButton: dialog.insertButton
    property alias abbreviationInput: dialog.abbreviationInput
    property alias abbreviationModel: dialog.abbreviationModel
    property alias grid: dialog.grid

    AbbreviationDialog {
        id: dialog
    }

    Connections {
        target: dialog

        function onAccepted() { root.accepted() }
        function onCanceled() { root.canceled() }
    }
}
