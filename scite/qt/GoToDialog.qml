import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.13
import QtQuick.Dialogs 1.2

GoToDialogForm {

    destinationLineInput {
        validator: IntValidator { bottom: 1; top: lastLineNumber }
    }

    columnInput {
        validator: IntValidator { bottom: 1; top: 9999 }
    }

    lastLineOutput {
        onTextChanged: {
            lastLineNumber = parseInt(lastLineOutput.text)
        }
    }

}
