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
    signal markAll()

    property alias fcnLocalisation: dialog.fcnLocalisation

    property alias cancelButton: dialog.cancelButton
    property alias findNextButton: dialog.findNextButton
    property alias markAllButton: dialog.markAllButton
    property alias findWhatInput: dialog.findWhatInput
    property alias findWhatModel: dialog.findWhatModel
    property alias searchUpButton: dialog.searchUpButton
    property alias searchDownButton: dialog.searchDownButton
    property alias matchWholeWordCheckBox: dialog.matchWholeWordCheckBox
    property alias caseSensitiveCheckBox: dialog.caseSensitiveCheckBox
    property alias regularExpressionCheckBox: dialog.regularExpressionCheckBox
    property alias wrapAroundCheckBox: dialog.wrapAroundCheckBox
    property alias tramsformBackslashExprCheckBox: dialog.tramsformBackslashExprCheckBox
    property alias grid: dialog.grid

    FindDialog {
        id: dialog
    }

    Connections {
        target: dialog

        function onAccepted() { root.accepted() }
        function onCanceled() { root.canceled() }
        function onMarkAll()  { root.markAll() }
    }
}
