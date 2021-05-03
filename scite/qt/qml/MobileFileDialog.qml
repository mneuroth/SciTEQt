/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.0

MobileFileDialogForm {
    id: root

    property bool isSaveAsModus: false
    property bool isSaveACopyModus: false
    property bool isDeleteModus: false
    property bool isDirectoryModus: false
    property bool isSaveAsImage: false
    property bool isExtendedInfos: false
    property bool isMobilePlatform: sciteQt.mobilePlatform //applicationData.isAndroid
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
    signal errorMessage(string message)

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
                root.txtMFDInput.text = listView.currentItem.currentFileName
                root.setCurrentName(listView.currentItem.currentFileName)
            }
            else
            {
                root.txtMFDInput.text = ""
                root.setCurrentName("")
                //listView.currentItem.currentFileName("")
            }

            if( !root.isSaveAsModus )
            {
                root.btnOpen.enabled = listView.currentItem === null || listView.currentItem.isFile
            }
        }
    }

    function setAdminModus(value) {
        root.bIsAdminModus = value
    }

    function setSaveAsModus(sDefaultSaveAsName,bSaveACopyModus) {
        root.isSaveAsModus = true
        root.isSaveACopyModus = bSaveACopyModus
        root.isDeleteModus = false
        root.isDirectoryModus = false
        root.bShowFiles = true
        root.lblMFDInput.text = localiseText(qsTr("new file name:"))
        root.txtMFDInput.text = (sDefaultSaveAsName === null || sDefaultSaveAsName.length==0) ? localiseText(qsTr("unknown.txt")) : sDefaultSaveAsName
        root.txtMFDInput.readOnly = false
        root.btnOpen.text = localiseText(qsTr("Save as"))
        root.btnOpen.enabled = true
        root.btnStorage.enabled = true
    }

    function setOpenModus() {
        root.isSaveAsModus = false
        root.isSaveACopyModus = false
        root.isDeleteModus = false
        root.isDirectoryModus = false
        root.bShowFiles = true
        root.lblMFDInput.text = localiseText(qsTr("open name:"))
        root.txtMFDInput.readOnly = true
        root.btnOpen.text = localiseText(qsTr("Open"))
        root.btnOpen.enabled = false
        root.btnStorage.enabled = true
    }

    function setDirectoryModus() {
        root.isSaveAsModus = false
        root.isSaveACopyModus = false
        root.isDeleteModus = false
        root.isDirectoryModus = true
        root.bShowFiles = false
        root.lblMFDInput.text = localiseText(qsTr(""))
        root.txtMFDInput.readOnly = true
        root.btnOpen.text = localiseText(qsTr("Select current"))
        root.btnOpen.enabled = false
        root.btnStorage.enabled = false
    }

    function setDeleteModus() {
        root.isSaveAsModus = false
        root.isSaveACopyModus = false
        root.isDeleteModus = true
        root.isDirectoryModus = false
        root.bShowFiles = true
        root.lblMFDInput.text = localiseText(qsTr("current file name:"))
        root.txtMFDInput.text = ""
        root.txtMFDInput.readOnly = true
        root.btnOpen.text = localiseText(qsTr("Delete"))
        root.btnOpen.enabled = false
        root.btnStorage.enabled = false
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
        accepted()
        if( !ok )
        {
            var msg= localiseText(qsTr("ERROR: Can not delete file ")) + fullPath
            errorMessage(msg)
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
            root.setDirectory(sdCardPath)
            root.setCurrentName("")
        }
    }

    function formatSize(fileSize) {
        var value = Math.round(fileSize/1024*10)/10
        if( value>=1000 ) {
            var valueMB = Math.round(fileSize/(1024*1024)*10)/10
            return valueMB + " MBytes"
        } else if( value>=1 ) {
            return value + " kBytes"
        } else {
            return fileSize + "  Bytes"
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
                        root.setDirectory(filePath)
                        root.setCurrentName(fileName)
                        event.accepted = true
                    }
                    else
                    {
                        root.openCurrentFileNow()
                        event.accepted = true
                    }
                 }
            }

            GridLayout {
                anchors.fill: parent

                columns: 4

                Image {
                    id: itemIcon
                    source: fileIsDir ? "icons/directory.svg" : "icons/file.svg"

                    Layout.row: 0
                    Layout.column: 0
                    Layout.maximumHeight: itemLabel.height
                    Layout.maximumWidth: itemLabel.height
                }
                Label {
                    id: itemLabel

                    verticalAlignment: Text.AlignVCenter
                    text: /*(fileIsDir ? "DIR_" : "FILE") + " | " +*/ fileName

                    Layout.fillWidth: true
                    Layout.row: 0
                    Layout.column: 1
                }
                Label {
                    id: itemDate
                    visible: isExtendedInfos
                    font.pointSize: isMobilePlatform ? itemLabel.font.pointSize*0.75 : itemLabel.font.pointSize

                    verticalAlignment: Text.AlignVCenter
                    text: fileModified.toLocaleString(Qt.locale(),Locale.ShortFormat)

                    Layout.row: 0
                    Layout.column: 2
                }
                Label {
                    id: itemSize
                    visible: isExtendedInfos
                    font.pointSize: isMobilePlatform ? itemLabel.font.pointSize*0.75 : itemLabel.font.pointSize

                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: fileIsDir ? "" : formatSize(fileSize)

                    Layout.row: 0
                    Layout.column: 3
                    Layout.minimumWidth: parent.width*0.2  //(isMobilePlatform ? 0.2 : 0.2)
                }
            }
            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    root.listView.currentIndex = index
                    if( fileIsDir )
                    {
                        root.setDirectory(filePath)
                        root.setCurrentName(fileName)
                    }
                }
                onDoubleClicked: {
                    root.listView.currentIndex = index
                    if( !fileIsDir )
                    {
                        root.openCurrentFileNow()
                    }
                }
            }
        }
    }

    btnOpen  {
        onClicked: {
            if( root.isDeleteModus )
            {
                root.deleteCurrentFileNow()
            }
            else if( root.isDirectoryModus )
            {
                root.selectCurrentDirectoryNow()
            }
            else if( root.isSaveAsModus )
            {
                var fullPath = currentDirectory + "/" + txtMFDInput.text
                root.saveAsCurrentFileNow(fullPath)
            }
            else
            {
                root.openCurrentFileNow()
            }
        }
    }

    btnCancel {
        onClicked: rejected()
    }

    btnUp {
        onClicked: {
            // stop with moving up when home directory is reached
            if( root.bIsAdminModus || (applicationData.getNormalizedPath(currentDirectory) !== applicationData.getNormalizedPath(applicationData.homePath)) )
            {
                root.setDirectory(currentDirectory + "/..")
                root.setCurrentName("")
                root.listView.currentIndex = -1
            }
        }
    }

    btnHome {
        onClicked: {
            root.setDirectory(applicationData.homePath)
            root.setCurrentName("")
            root.listView.currentIndex = -1
        }
    }

    Menu {
        id: menuSDCard
        Repeater {
                model: applicationData !== null ? applicationData.getSDCardPaths() : []
                MenuItem {
                    text: modelData
                    onTriggered: {
                        root.navigateToDirectory(modelData)
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
            if( root.isSaveAsModus )
            {
                storageAccess.createFile(root.txtMFDInput.text)
            }
            else
            {
                storageAccess.openFile()
            }
        }
    }
}
