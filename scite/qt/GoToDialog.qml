import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.13
import QtQuick.Dialogs 1.2

GoToDialogForm {

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

}
