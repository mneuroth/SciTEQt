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

import Qt.labs.platform 1.0 as Platform

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
            canceled()
        }
    }

    findButton {
        onClicked: {
            accepted()
        }
    }

    browseButton {
        onClicked: {
            var currentDirectory = root.directoryInput.editText.length > 0 ? root.directoryInput.editText : root.directoryInput.currentText
            if(sciteQt.mobilePlatform) {
                selectDirectoryViaMobileFileDialog(currentDirectory)
                //mobileFileDialog.show()
            }
            else {
                folderDialog.folder = buildValidUrl(currentDirectory)
                folderDialog.open()
            }
        }
    }

    upButton {
        onClicked: {
            var currentDirectory = root.directoryInput.editText.length > 0 ? root.directoryInput.editText : root.directoryInput.currentText
            var newDirectory = sciteQt.cmdDirectoryUp(currentDirectory)
            // process directory up: add new text to combobox (temporary) and select added item afterwards
            directoryModel.append({"text":newDirectory})
            directoryInput.currentIndex = directoryModel.count - 1
        }
    }

    Platform.FolderDialog {
        id: folderDialog
        objectName: "folderDialog"
        visible: false
        modality: Qt.ApplicationModal
        title: qsTr("Choose a directory")
    }

    Connections {
        target: folderDialog

        onAccepted: {
            var newDirectory = sciteQt.cmdUrlToLocalPath(folderDialog.folder)
            directoryModel.append({"text":newDirectory})
            directoryInput.currentIndex = directoryModel.count - 1
        }
    }

    Connections {
        target: mobileFileDialog

        onDirectorySelected: {
            var newDirectory = sciteQt.cmdUrlToLocalPath(directory)
            directoryModel.append({"text":newDirectory})
            directoryInput.currentIndex = directoryModel.count - 1
        }
    }
}
