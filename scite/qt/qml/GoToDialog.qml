/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.2

GoToDialogForm {
    id: root

    signal canceled()
    signal accepted()

    destinationLineInput {
        validator: IntValidator { bottom: 1; top: 999999 /*lastLineNumber*/ }

        onAccepted: gotoButton.clicked()
    }

    columnInput {
        validator: IntValidator { bottom: 1; top: 9999 }

        onAccepted: gotoButton.clicked()
    }

    lastLineOutput {
        onTextChanged: {
            lastLineNumber = parseInt(lastLineOutput.text)
        }
    }

    cancelButton {
        onClicked: {
            canceled()
        }
    }

    gotoButton {
        onClicked: {
            accepted()
        }
    }
}
