/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Dialogs 1.2

MobileFileDialogForm {
    id: root

    property bool isSaveAsModus: false
    property bool isSaveACopyModus: false
    property bool isDeleteModus: false
    property bool isDirectoryModus: false
    property var textControl: null

    property var fcnLocalisation: undefined

    function localiseText(text) {
        if(fcnLocalisation !== undefined) {
            return fcnLocalisation(text)
        }
        return text
    }

    signal openSelectedFile(string fileName)
    signal saveSelectedFile(string fileName)
    signal directorySelected(string directory)

    signal accepted()
    signal rejected()

    listView {
        // https://stackoverflow.com/questions/9400002/qml-listview-selected-item-highlight-on-click
        currentIndex: -1
        focus: true
        onCurrentIndexChanged: {
            // update currently selected filename
            if( listView.currentItem !== null && listView.currentItem.isFile )
            {
                mobileFileDialog.txtMFDInput.text = listView.currentItem.currentFileName
                mobileFileDialog.setCurrentName(listView.currentItem.currentFileName)
            }
            else
            {
                mobileFileDialog.txtMFDInput.text = ""
                mobileFileDialog.setCurrentName("")
                //listView.currentItem.currentFileName("")
            }

            if( !mobileFileDialog.isSaveAsModus )
            {
                mobileFileDialog.btnOpen.enabled = listView.currentItem === null || listView.currentItem.isFile
            }
        }
    }

    function setSaveAsModus(sDefaultSaveAsName,bSaveACopyModus) {
        mobileFileDialog.isSaveAsModus = true
        mobileFileDialog.isSaveACopyModus = bSaveACopyModus
        mobileFileDialog.isDeleteModus = false
        mobileFileDialog.isDirectoryModus = false
        mobileFileDialog.bShowFiles = true
        mobileFileDialog.lblMFDInput.text = localiseText(qsTr("new file name:"))
        mobileFileDialog.txtMFDInput.text = (sDefaultSaveAsName === null || sDefaultSaveAsName.length==0) ? localiseText(qsTr("unknown.txt")) : sDefaultSaveAsName
        mobileFileDialog.txtMFDInput.readOnly = false
        mobileFileDialog.btnOpen.text = localiseText(qsTr("Save as"))
        mobileFileDialog.btnOpen.enabled = true
    }

    function setOpenModus() {
        mobileFileDialog.isSaveAsModus = false
        mobileFileDialog.isSaveACopyModus = false
        mobileFileDialog.isDeleteModus = false
        mobileFileDialog.isDirectoryModus = false
        mobileFileDialog.bShowFiles = true
        mobileFileDialog.lblMFDInput.text = localiseText(qsTr("open name:"))
        mobileFileDialog.txtMFDInput.readOnly = true
        mobileFileDialog.btnOpen.text = localiseText(qsTr("Open"))
        mobileFileDialog.btnOpen.enabled = false
    }

    function setDirectoryModus() {
        mobileFileDialog.isSaveAsModus = false
        mobileFileDialog.isSaveACopyModus = false
        mobileFileDialog.isDeleteModus = false
        mobileFileDialog.isDirectoryModus = true
        mobileFileDialog.bShowFiles = false
        mobileFileDialog.lblMFDInput.text = localiseText(qsTr(""))
        mobileFileDialog.txtMFDInput.readOnly = true
        mobileFileDialog.btnOpen.text = localiseText(qsTr("Select"))
        mobileFileDialog.btnOpen.enabled = false
    }

    function setDeleteModus() {
        mobileFileDialog.isSaveAsModus = false
        mobileFileDialog.isSaveACopyModus = false
        mobileFileDialog.isDeleteModus = true
        mobileFileDialog.isDirectoryModus = false
        mobileFileDialog.bShowFiles = true
        mobileFileDialog.lblMFDInput.text = localiseText(qsTr("current file name:"))
        mobileFileDialog.txtMFDInput.text = ""
        mobileFileDialog.txtMFDInput.readOnly = true
        mobileFileDialog.btnOpen.text = localiseText(qsTr("Delete"))
        mobileFileDialog.btnOpen.enabled = false
    }

    function setDirectory(newPath) {
        newPath = applicationData.getNormalizedPath(newPath)
        listView.model.folder = buildValidUrl(newPath)
        listView.currentIndex = -1
        listView.focus = true
        lblDirectoryName.text = newPath
        currentDirectory = newPath
    }

    function setCurrentName(name) {
        currentFileName = name
    }

    function deleteCurrentFileNow() {
        var fullPath = currentDirectory + "/" + currentFileName
        var ok = applicationData.deleteFile(fullPath)
        stackView.pop()
        if( !ok )
        {
            outputPage.txtOutput.text += localiseText(qsTr("can not delete file ")) + fullPath
            stackView.push(outputPage)
        }
    }

    function openCurrentFileNow() {
        var fullPath = currentDirectory + "/" + currentFileName
        openSelectedFile(fullPath)
        accepted()
    }

    function saveAsCurrentFileNow(fullPath) {
        saveSelectedFile(fullPath)
        accepted()
    }

    function selectCurrentDirectoryNow() {
        directorySelected(buildValidUrl(currentDirectory))
        accepted()
    }

    function navigateToDirectory(sdCardPath) {
        if( !applicationData.hasAccessToSDCardPath() )
        {
            applicationData.grantAccessToSDCardPath()
        }

        if( applicationData.hasAccessToSDCardPath() )
        {
            mobileFileDialog.setDirectory(sdCardPath)
            mobileFileDialog.setCurrentName("")
        }
    }

    Component {
        id: fileDelegate
        Rectangle {
            property string currentFileName: fileName
            property bool isFile: !fileIsDir
            height: 40
            color: "transparent"
            anchors.left: parent.left
            anchors.right: parent.right
            Keys.onPressed: {
                 if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                    if( fileIsDir )
                    {
                        mobileFileDialog.setDirectory(filePath)
                        mobileFileDialog.setCurrentName(fileName)
                        event.accepted = true
                    }
                    else
                    {
                        mobileFileDialog.openCurrentFileNow()
                        event.accepted = true
                    }
                 }
            }
            Row {
                anchors.fill: parent
                spacing: 5

                Image {
                    id: itemIcon
                    anchors.left: parent.Left
                    height: itemLabel.height - 8
                    width: itemLabel.height - 8
                    source: fileIsDir ? "icons/directory.svg" : "icons/file.svg"
                }
                Label {
                    id: itemLabel
                    anchors.left: itemIcon.Right
                    anchors.right: parent.Right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    verticalAlignment: Text.AlignVCenter
                    text: /*(fileIsDir ? "DIR_" : "FILE") + " | " +*/ fileName
                }
            }
            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    mobileFileDialog.listView.currentIndex = index
                    if( fileIsDir )
                    {
                        mobileFileDialog.setDirectory(filePath)
                        mobileFileDialog.setCurrentName(fileName)
                    }
                }
                onDoubleClicked: {
                    mobileFileDialog.listView.currentIndex = index
                    if( !fileIsDir )
                    {
                        mobileFileDialog.openCurrentFileNow()
                    }
                }
            }
        }
    }

    btnOpen  {
        onClicked: {
            if( mobileFileDialog.isDeleteModus )
            {
                mobileFileDialog.deleteCurrentFileNow()
            }
            else if( mobileFileDialog.isDirectoryModus )
            {
                mobileFileDialog.selectCurrentDirectoryNow()
            }
            else if( mobileFileDialog.isSaveAsModus )
            {
                var fullPath = currentDirectory + "/" + txtMFDInput.text
                mobileFileDialog.saveAsCurrentFileNow(fullPath)
            }
            else
            {
                mobileFileDialog.openCurrentFileNow()
            }
        }
    }

    btnCancel {
        onClicked: {
            rejected()
        }
    }

    btnUp {
        onClicked: {
            // stop with moving up when home directory is reached
            if( applicationData.getNormalizedPath(currentDirectory) !== applicationData.getNormalizedPath(applicationData.homePath) )
            {
                mobileFileDialog.setDirectory(currentDirectory + "/..")
                mobileFileDialog.setCurrentName("")
                mobileFileDialog.listView.currentIndex = -1
            }
        }
    }

    btnHome {
        onClicked: {
            mobileFileDialog.setDirectory(applicationData.homePath)
            mobileFileDialog.setCurrentName("")
            mobileFileDialog.listView.currentIndex = -1
        }
    }

    Menu {
        id: menuSDCard
        Repeater {
                model: applicationData !== null ? applicationData.getSDCardPaths() : []
                MenuItem {
                    text: modelData
                    onTriggered: {
                        mobileFileDialog.navigateToDirectory(modelData)
                    }
                }
        }
    }

    btnSDCard {
        onClicked: {
            menuSDCard.x = btnSDCard.x
            menuSDCard.y = btnSDCard.height
            menuSDCard.open()
        }
    }

    btnStorage {
        visible: applicationData !== null ? applicationData.isShareSupported : false
        onClicked: {
            if( mobileFileDialog.isSaveAsModus )
            {
                storageAccess.createFile(mobileFileDialog.txtMFDInput.text)
            }
            else
            {
                storageAccess.openFile()
            }

            //root.close()
        }
    }
}
