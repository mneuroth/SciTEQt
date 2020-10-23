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

ParametersDialogForm {
    id: root

    signal canceled()
    signal accepted()

    cmdInput {
        onAccepted: setButton.clicked()
    }

    parameter1Input {
        onAccepted: setButton.clicked()
    }

    parameter2Input {
        onAccepted: setButton.clicked()
    }

    parameter3Input {
        onAccepted: setButton.clicked()
    }

    parameter4Input {
        onAccepted: setButton.clicked()
    }

    cancelButton {
        onClicked: {
            root.close()
            canceled()
        }
    }

    setButton {
        onClicked: {
            root.close()
            accepted()
        }
    }
}
