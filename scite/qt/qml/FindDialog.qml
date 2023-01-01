/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.4
import QtQuick.Controls 2.1

FindDialogForm {
    id: root

    signal canceled()
    signal accepted()
    signal markAll()

    findWhatInput {
        onAccepted: findNextButton.clicked()
    }

    cancelButton {
        onClicked: {
            canceled()
        }
    }

    findNextButton {
        onClicked: {
            accepted()
        }
    }

    markAllButton {
        onClicked: {
            markAll()
        }
    }
}
