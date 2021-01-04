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

    signal closed()

    property alias fcnLocalisation: dialog.fcnLocalisation

    SupportDialog {
        id: dialog
    }

    Connections {
        target: dialog

        onClosed: root.closed()
    }
}
