import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.2

FindInFilesDialogForm {
    id: root

    signal canceled()
    signal accepted()

    findWhatInput {
        onAccepted: findButton.clicked()
    }

    filesExtensionsInput {
        onAccepted: findButton.clicked()
    }

    directoryInput {
        onAccepted: findButton.clicked()
    }

    cancelButton {
        onClicked: {
            root.close()
            canceled()
        }
    }

    findButton {
        onClicked: {
            root.close()
            accepted()
        }
    }

    browseButton {
        onClicked: {
            // TODO --> directory selector oeffnen...
        }
    }

    upButton {
        onClicked: {
            // TODO --> ein verzeichnis nach oben
        }
    }
}
