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
    property alias findButton: dialog.findButton
    property alias browseButton: dialog.browseButton
    property alias upButton: dialog.upButton
    property alias findWhatInput: dialog.findWhatInput
    property alias filesExtensionsInput: dialog.filesExtensionsInput
    property alias directoryInput: dialog.directoryInput
    property alias findWhatModel: dialog.findWhatModel
    property alias filesExtensionsModel: dialog.filesExtensionsModel
    property alias directoryModel: dialog.directoryModel
    property alias wholeWordCheckBox: dialog.wholeWordCheckBox
    property alias caseSensitiveCheckBox: dialog.caseSensitiveCheckBox
    property alias regularExpressionCheckBox: dialog.regularExpressionCheckBox
    property alias grid: dialog.grid

    FindInFilesDialog {
        id: dialog
    }

    Connections {
        target: dialog

        function onAccepted() { root.accepted() }
        function onCanceled() { root.canceled() }
    }
}
