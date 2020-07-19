import Scintilla 1.0
import QtQuick 2.9
import QtQuick.Controls 2.14
import QtQuick.Dialogs 1.2
import Qt.labs.platform 1.1 as Platform

import de.mneuroth.sciteqt 1.0

ApplicationWindow {
    id: applicationWindow
    width: 600
    height: 400
    visible: true

    property string urlPrefix: "file://"

    Component.onCompleted: {
        sciteQt.setScintilla(quickScintillaEditor.scintilla)
        sciteQt.setOutput(quickScintillaOutput.scintilla)
        sciteQt.setMainWindow(applicationWindow)
        sciteQt.setApplicationData(applicationData)
    }

    function max(v1, v2) {
        return v1 < v2 ? v2 : v1;
    }

    function startFileDialog(sDirectory, sFilter, bAsOpenDialog) {
        //fileDialog.selectExisting = bAsOpenDialog
        fileDialog.openMode = bAsOpenDialog
        fileDialog.folder = sDirectory
        //fileDialog.nameFilters = sFilter // as list of strings...
        fileDialog.open()
    }

    function buildValidUrl(path) {
        // ignore call, if we already have a file:// url
        path = path.toString()
        if( path.startsWith(urlPrefix) )
        {
            return path;
        }
        // ignore call, if we already have a content:// url (android storage framework)
        if( path.startsWith("content://") )
        {
            return path;
        }

        var sAdd = path.startsWith("/") ? "" : "/"
        var sUrl = urlPrefix + sAdd + path
        return sUrl
    }

    function writeCurrentDoc(url) {
        console.log("write current... "+url)
        sciteQt.saveCurrentAs(url)
    }

    function readCurrentDoc(url) {
        // then read new document
        console.log("read current doc "+url)
        var urlFileName = buildValidUrl(url)
        lblFileName.text = urlFileName
        sciteQt.doOpen(url)
        //quickScintillaEditor.text = applicationData.readFileContent(urlFileName)
    }

    function showInfoDialog(infoText,style) {
        //mbsOK = 0,
        //mbsYesNo = 4,
        //mbsYesNoCancel = 3,
        //mbsIconQuestion = 0x20,
        //mbsIconWarning = 0x30
        if((style & 7) === 4)
        {
            infoDialog.standardButtons = StandardButton.Yes | StandardButton.No
        }
        if((style & 7) === 3)
        {
            infoDialog.standardButtons = StandardButton.Yes | StandardButton.No | StandardButton.Cancel
        }

        infoDialog.text = infoText
        infoDialog.open()
    }

    function processMenuItem(menuText, menuItem) {
        var s = sciteQt.getLocalisedText(menuText)
        if( menuItem !== null && menuItem.shortcut !== undefined)
        {
            s += "\t" + menuItem.shortcut
        }
        return s
    }

    menuBar: MenuBar {
        id: menuBar

        Menu {
            id: fileMenu
            title: processMenuItem(qsTr("&File"),null)

            Action {
                id: actionNew
                text: processMenuItem(qsTr("&New"), actionNew)
                //icon.source: "share.svg"
                shortcut: "Ctrl+N"
                onTriggered: sciteQt.CmdNew()
            }
            Action {
                id: actionOpen
                text: processMenuItem(qsTr("&Open..."), actionOpen)
                //icon.source: "share.svg"
                shortcut: "Ctrl+O"
                onTriggered: sciteQt.CmdOpen()
            }
            Action {
                id: actionRevert
                text: processMenuItem(qsTr("&Revert"), actionRevert)
                shortcut: "Ctrl+W"
                onTriggered: sciteQt.CmdRevert()
            }
            Action {
                id: actionClose
                text: processMenuItem(qsTr("&Close"), actionClose)
                shortcut: "Ctrl+W"
                onTriggered: sciteQt.CmdClose()
            }
            Action {
                id: actionSave
                text: processMenuItem(qsTr("&Save"), actionSave)
                shortcut: "Ctrl+S"
                onTriggered: sciteQt.CmdSave()
            }
            Action {
                id: actionSaveAs
                text: processMenuItem(qsTr("Save &As..."), actionSaveAs)
                onTriggered: sciteQt.CmdSaveAs()
            }
            Action {
                id: actionCopyPath
                text: processMenuItem(qsTr("Copy Pat&h"), actionCopyPath)
                onTriggered: sciteQt.CmdCopyPath()
            }
            MenuSeparator {}
            Action {
                id: actionExit
                text: processMenuItem(qsTr("E&xit"), actionExit)
                onTriggered: sciteQt.CmdExit()
            }
        }

        Menu {
            id: editMenu
            title: processMenuItem(qsTr("Edit"),null)

            Action {
                id: actionUndo
                text: processMenuItem(qsTr("&Undo"), actionUndo)
                shortcut: "Ctrl+Z"
                onTriggered: sciteQt.CmdUndo()
            }
            Action {
                id: actionRedo
                text: processMenuItem(qsTr("&Redo"), actionRedo)
                shortcut: "Ctrl+Y"
                onTriggered: sciteQt.CmdRedo()
            }
            MenuSeparator {}
            Action {
                id: actionCut
                text: processMenuItem(qsTr("Cu&t"), actionCut)
                shortcut: "Ctrl+X"
                onTriggered: sciteQt.CmdCut()
            }
            Action {
                id: actionCopy
                text: processMenuItem(qsTr("&Copy"), actionCopy)
                shortcut: "Ctrl+C"
                onTriggered: sciteQt.CmdCopy()
            }
            Action {
                id: actionPaste
                text: processMenuItem(qsTr("&Paste"), actionPaste)
                shortcut: "Ctrl+V"
                onTriggered: sciteQt.CmdPaste()
            }
        }

        Menu {
            id: searchMenu
            title: processMenuItem(qsTr("Search"),null)

            Action {
                id: actionFind
                text: processMenuItem(qsTr("&Find..."), actionFind)
                shortcut: "Ctrl+F"
                onTriggered: sciteQt.CmdFind()
            }
            Action {
                id: actionFindNext
                text: processMenuItem(qsTr("Find &Next"), actionFindNext)
                shortcut: "F3"
                onTriggered: sciteQt.CmdFindNext()
            }
            Action {
                id: actionFindPrevious
                text: processMenuItem(qsTr("Find &Next"), actionFindPrevious)
                shortcut: "Shift+F3"
                onTriggered: sciteQt.CmdFindPrevious()
            }
        }

        Menu {
            id: viewMenu
            title: processMenuItem(qsTr("&View"),null)

            MenuItem {
                id: actionShowToolBar
                text: processMenuItem(qsTr("&Tool Bar"), actionShowToolBar)
                checkable: true
                checked: applicationData !== null ? applicationData.showToolBar : false
                onTriggered: sciteQt.CmdShowToolBar()
            }
            MenuItem {
                id: actionShowStatusBar
                text: processMenuItem(qsTr("&Status Bar"), actionShowStatusBar)
                checkable: true
                checked: applicationData !== null ? applicationData.showStatusBar : false
                onTriggered: sciteQt.CmdShowStatusBar()
            }
            MenuItem {
                text: qsTr("Line &Numbers")
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdLineNumbers()
            }
        }


        Menu {
            id: toolsMenu
            title: processMenuItem(qsTr("Tools"),null)
        }

        Menu {
            id: optionsMenu
            title: processMenuItem(qsTr("Options"),null)

            MenuItem {
                text: processMenuItem(qsTr("Use Monospaced Font"),null)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdUseMonospacedFont()
            }
        }

        Menu {
            id: languageMenu
            title: processMenuItem(qsTr("Language"),null)
        }


        Menu {
            id: buffersMenu
            title: processMenuItem(qsTr("Buffers"),null)
        }

        Menu {
            id: helpMenu
            title: processMenuItem(qsTr("Help"),null)

            MenuItem {
                id: actionDebugInfo
                text: processMenuItem(qsTr("Debug info"),actionDebugInfo)
                onTriggered: {
                    showInfoDialog(quickScintillaEditor.text)
                    /*for Tests only: */quickScintillaEditor.text = applicationData.readLog()
                }
            }
        }
    }

    header: ToolBar {
        contentHeight: readonlyIcon.implicitHeight
        visible: applicationData !== null ? applicationData.showToolBar : false

        ToolButton {
            id: readonlyIcon
            //icon.source: "edit.svg"
            text: "Open"
            //visible: true // applicationData !== null ? applicationData.showToolBar : false
            //anchors.right: readonlySwitch.left
            //anchors.rightMargin: 1
            onClicked: {
                fileDialog.openMode = true
                fileDialog.open()
            }
        }
    }

    footer:  Text {
        id: statusBarText
        visible: applicationData !== null ? applicationData.showStatusBar : false

        text: applicationData !== null ? applicationData.statusBarText : ""

        MouseArea {
            anchors.fill: parent
            onClicked: applicationData.onStatusbarClicked()
        }
    }

    Text {
        id: lblFileName
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        //anchors.verticalCenter: btnShowText.verticalCenter
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5
        text: "?"
    }

    FileDialog {
        id: fileDialog
        objectName: "fileDialog"
        visible: false
        modality: Qt.WindowModal
        //fileMode: openMode ? FileDialog.OpenFile : FileDialog.SaveFile
        title: openMode ? qsTr("Choose a file") : qsTr("Save as")
        folder: "."

        property bool openMode: true

        selectExisting: openMode ? true : false
        selectMultiple: false
        selectFolder: false

        onAccepted: {
            //console.log("Accepted: " + /*currentFile*/fileUrl+" "+fileDialog.openMode)
            if(!fileDialog.openMode) {
                writeCurrentDoc(fileUrl)
            }
            else {
                readCurrentDoc(fileUrl)
            }
            quickScintillaEditor.focus = true
        }
        onRejected: {
            quickScintillaEditor.focus = true
        }
    }

    MessageDialog {
        id: infoDialog
        objectName: "infoDialog"
        visible: false
        title: qsTr("Info")
        //modal: true
        //modality: Qt.WindowModality
        standardButtons: StandardButton.Ok
        /*
        onAccepted: {
            quickScintillaEditor.focus = true
            console.log("accept")
        }
        onRejected: console.log("reject")
        onYes: console.log("yes")
        onNo: console.log("no")
        */
    }

    SplitView {
        id: splitView

        //anchors.fill: parent
        anchors.top: lblFileName.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        ScintillaText {
            id: quickScintillaEditor

            SplitView.preferredWidth: parent.width / 2

            text: "editor area !"
        }

        ScintillaText {
            id: quickScintillaOutput

            SplitView.preferredWidth: parent.width / 2

            text: "blub output !"
        }

    }

   SciTEQt {
       id: sciteQt
   }
}
