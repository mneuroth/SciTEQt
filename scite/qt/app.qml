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
                text: processMenuItem(qsTr("Open..."), actionOpen)
                //icon.source: "share.svg"
                shortcut: "Ctrl+O"
                onTriggered: sciteQt.CmdOpen()
            }
            Action {
                id: actionClose
                text: processMenuItem(qsTr("Close"), actionClose)
                shortcut: "Alt+C"
                onTriggered: sciteQt.CmdClose()
            }
            Action {
                id: actionSave
                text: processMenuItem(qsTr("Save"), actionSave)
                shortcut: "Ctrl+S"
                onTriggered: sciteQt.CmdSave()
            }
            Action {
                id: actionSaveAs
                text: processMenuItem(qsTr("Save as..."), actionSaveAs)
                onTriggered: sciteQt.CmdSaveAs()
            }
        }

        Menu {
            id: editMenu
            title: qsTr("Edit")
        }


        Menu {
            id: searchMenu
            title: qsTr("Search")
        }

        Menu {
            id: viewMenu
            title: qsTr("&View")

            MenuItem {
                text: qsTr("Line &Numbers")
                checkable: true
                checked: false
                onTriggered: {
                    sciteQt.CmdLineNumbers()
                }
            }
        }


        Menu {
            id: toolsMenu
            title: qsTr("Tools")
        }

        Menu {
            id: optionsMenu
            title: qsTr("Options")

            MenuItem {
                text: qsTr("Use Monospaced Font")
                checkable: true
                checked: false
                onTriggered: {
                    sciteQt.CmdUseMonospacedFont()
                }

            }
        }

        Menu {
            id: languageMenu
            title: qsTr("Language")
        }


        Menu {
            id: buffersMenu
            title: qsTr("Buffers")
        }

        Menu {
            id: helpMenu
            title: qsTr("Help")
        }
    }

    header: ToolBar {
        contentHeight: readonlyIcon.implicitHeight
        visible: false

        ToolButton {
            id: readonlyIcon
            visible: false
            //icon.source: "edit.svg"
            text: "blub"
            //visible: stackView.currentItem === homePage
            //anchors.right: readonlySwitch.left
            //anchors.rightMargin: 1
        }
    }

    Button {
        id: btnLoadFile
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 5
        anchors.leftMargin: 5
        text: "Load file"
        onClicked: {
            //fileDialog.fileMode = FileDialog.OpenFile
            fileDialog.title = "Choose a file"
            fileDialog.selectExisting = true
            fileDialog.openMode = true
            fileDialog.open()
        }
    }

    Button {
        id: btnSaveFile
        //enabled: lblFileName.text.startsWith(urlPrefix)
        anchors.top: parent.top
        anchors.left: btnLoadFile.right
        anchors.topMargin: 5
        anchors.leftMargin: 5
        text: "Save file as"
        onClicked: {
            //fileDialog.fileMode = FileDialog.SaveFile
            fileDialog.title = "Save a file"
            fileDialog.selectExisting = false
            fileDialog.openMode = false
            fileDialog.open()
        }
    }

    Button {
        id: btnClearText
        anchors.top: parent.top
        anchors.left: btnSaveFile.right
        anchors.topMargin: 5
        anchors.leftMargin: 5
        text: "Clear"
        onClicked: {
            quickScintillaEditor.text = ""
            lblFileName.text = "unknown.txt"
            //for Tests only: Qt.inputMethod.show()
            scrollView.focus = true
            //quickScintillaEditor.focus = true
        }
    }

    Button {
        id: btnShowText
        anchors.top: parent.top
        anchors.left: btnClearText.right
        anchors.topMargin: 5
        anchors.leftMargin: 5
        text: "Show text"
        onClicked: {
            infoDialog.text = quickScintillaEditor.text
            //for Tests only: infoDialog.text = " "+scrollView.contentItem
            infoDialog.open()
            //for Tests only: readCurrentDoc("/sdcard/Texte/mgv_quick_qdebug.log")
            /*for Tests only: */quickScintillaEditor.text = applicationData.readLog()
        }
    }

    Text {
        id: lblFileName
        anchors.top: btnLoadFile.bottom
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
            console.log("Accepted: " + /*currentFile*/fileUrl+" "+fileDialog.openMode)
            /*if(fileDialog.fileMode === FileDialog.SaveFile)*/if(!fileDialog.openMode) {
                //var ok = applicationData.writeFileContent(/*currentFile*/fileUrl, quickScintillaEditor.text)
                writeCurrentDoc(fileUrl)
            }
            else {
                readCurrentDoc(/*currentFile*/fileUrl)
            }
            scrollView.focus = true
        }
        onRejected: {
            console.log("Rejected")
            scrollView.focus = true
        }
    }

    MessageDialog {
        id: infoDialog
        visible: false
        title: qsTr("Info")
        standardButtons: StandardButton.Ok
        onAccepted: {
            console.log("Close info dialog")
            scrollView.focus = true
            //quickScintillaEditor.focus = true
        }
    }

    function max(v1, v2) {
        return v1 < v2 ? v2 : v1;
    }

    SplitView {
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
