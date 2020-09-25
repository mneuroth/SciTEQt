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
            var currentDirectory = findInFilesDialog.directoryInput.editText.length > 0 ? findInFilesDialog.directoryInput.editText : findInFilesDialog.directoryInput.currentText
            folderDialog.folder = buildValidUrl(currentDirectory)
            folderDialog.open()
        }
    }

    upButton {
        onClicked: {
            var currentDirectory = findInFilesDialog.directoryInput.editText.length > 0 ? findInFilesDialog.directoryInput.editText : findInFilesDialog.directoryInput.currentText
            directoryInput.editText = sciteQt.cmdDirectoryUp(currentDirectory)
        }
    }

    Connections {
        target: folderDialog

        onAccepted: {
            directoryInput.editText = sciteQt.cmdUrlToLocalPath(folderDialog.folder)
        }
    }
}
