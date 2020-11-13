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
    signal convert()

    property alias fcnLocalisation: dialog.fcnLocalisation

    property alias cancelButton: dialog.cancelButton
    property alias okButton: dialog.okButton
    property alias convertButton: dialog.convertButton
    property alias tabSizeInput: dialog.tabSizeInput
    property alias indentSizeInput: dialog.indentSizeInput
    property alias useTabsCheckBox: dialog.useTabsCheckBox
    property alias grid: dialog.grid

    TabSizeDialog {
        id: dialog
    }

    Connections {
        target: dialog

        onAccepted: root.accepted()
        onCanceled: root.canceled()
        onConvert: root.convert()
    }
}
