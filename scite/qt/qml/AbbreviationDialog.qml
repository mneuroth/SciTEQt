/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
import QtQuick 2.4
import QtQuick.Controls 2.1

AbbreviationDialogForm {
    id: root

    signal canceled()
    signal accepted()

    abbreviationInput {
        onAccepted: insertButton.clicked()
    }

    cancelButton {
        onClicked: {
            canceled()
        }
    }

    insertButton {
        onClicked: {
            accepted()
        }
    }
}
