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
            var newDirectory = sciteQt.cmdDirectoryUp(currentDirectory)
            // process directory up: add new text to combobox (temporary) and select added item afterwards
            findInFilesDialog.directoryModel.append({"text":newDirectory})
            directoryInput.currentIndex = findInFilesDialog.directoryModel.count - 1
            //directoryInput.editText = newDirectory
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
            //directoryInput.editText = newDirectory
            findInFilesDialog.directoryModel.append({"text":newDirectory})
            directoryInput.currentIndex = findInFilesDialog.directoryModel.count - 1
        }
    }
}
