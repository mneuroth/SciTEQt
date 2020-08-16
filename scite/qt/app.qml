import QtQuick 2.9
import QtQuick.Controls 2.14
import QtQuick.Dialogs 1.2
import QtQml.Models 2.14
import Qt.labs.platform 1.1 as Platform
import Qt.labs.settings 1.0

import org.scintilla.sciteqt 1.0

ApplicationWindow {
    id: applicationWindow
    objectName: "SciteMainWindow"
    width: 600
    height: 800
    visible: true

    property string urlPrefix: "file://"

    onClosing: {
        sciteQt.cmdExit()
        close.accepted = false
    }

    Component.onCompleted: {
        console.log("ON Completed")
// TODO: gibt es besseren weg scintilla controls bei sciteqt zu registrieren?
        sciteQt.setScintilla(quickScintillaEditor.scintilla)
        sciteQt.setOutput(quickScintillaOutput.scintilla)
        sciteQt.setContent(splitView)
        sciteQt.setMainWindow(applicationWindow)
        sciteQt.setApplicationData(applicationData)
        console.log("ON Completed done")
        //splitView.restoreState(settings.splitView)
    }
    Component.onDestruction: {
        //settings.splitView = splitView.saveState()
    }

    function max(v1, v2) {
        return v1 < v2 ? v2 : v1;
    }

    function updateCurrentWindowPosAndSize() {
        sciteQt.updateCurrentWindowPosAndSize(applicationWindow.x, applicationWindow.y, applicationWindow.width, applicationWindow.height, applicationWindow.visibility === /*QWindow.Maximized*/4)
    }

    function setWindowPosAndSize(left, top, width, height, maximize) {
        applicationWindow.x = left
        applicationWindow.y = top
        applicationWindow.width = width
        applicationWindow.height = height
        applicationWindow.visibility = maximize ? /*QWindow.Maximized*/4 : 2;
    }

    function startFileDialog(sDirectory, sFilter, sTitle, bAsOpenDialog) {
        //fileDialog.selectExisting = bAsOpenDialog
        fileDialog.openMode = bAsOpenDialog
        fileDialog.folder = sDirectory
        if( sTitle !== undefined && sTitle.length > 0 ) {
            fileDialog.title = sciteQt.getLocalisedText(sTitle)
        }
        if( sFilter.length > 0) {
            fileDialog.nameFilters = [sFilter]
        }
        else {
            fileDialog.nameFilters = ["*"]
        }
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

    function showFindInFilesDialog(text) {
        findInFilesDialog.findWhatInput.text = text
        findInFilesDialog.show() //.open()
        findInFilesDialog.findWhatInput.focus = true
    }

    function showFind(text, incremental, withReplace) {
        findInput.text = text
        findInput.visible = true
        replaceInput.visible = withReplace
        if( withReplace ) {
            replaceInput.focus = true
        } else {
            findInput.focus = true
        }

        isIncrementalSearch = incremental
    }

    function showGoToDialog(currentLine, currentColumn, maxLine) {
        gotoDialog.destinationLineInput.text = ""
        gotoDialog.columnInput.text = ""
        gotoDialog.currentLineOutput.text = currentLine
        gotoDialog.currentColumnOutput.text = currentColumn
        gotoDialog.lastLineOutput.text = maxLine
        gotoDialog.show() //open()
        gotoDialog.destinationLineInput.focus = true
    }

    function setVerticalSplit(verticalSplit) {
        splitView.verticalSplit = verticalSplit
    }

    function setOutputHeight(heightOutput) {
        splitView.outputHeight = heightOutput
    }

    function processMenuItem(menuText, menuItem) {
// TODO: via font die breite fuer den text bestimmen und passende anzahl leerzeichen dafuer...
        //console.log("FONT: "+menuText+" "+ (menuItem!==undefined ? menuItem.font : "?"))
        var s = sciteQt.getLocalisedText(menuText)
        if( menuItem !== null && menuItem.shortcut !== undefined)
        {
            //s += " \t" + menuItem.shortcut
            return sciteQt.fillToLength(s, ""+menuItem.shortcut)
        }
        return s
    }

    function processMenuItem2(menuText, menuItem) {
// TODO: via font die breite fuer den text bestimmen und passende anzahl leerzeichen dafuer...
        //console.log("FONT: "+menuText+" "+ (menuItem!==undefined ? menuItem.font : "?")+" action="+menuItem.action) //+" shortcut="+menuItem.action.shortcut+ " "+menuItem.parent)
        var s = sciteQt.getLocalisedText(menuText)
        if( !sciteQt.isMobilePlatform() && menuItem !== null && menuItem.action !== null && menuItem.action.shortcut !== undefined)
        {
            //s += " \t" + menuItem.shortcut
            return sciteQt.fillToLengthWithFont(s, ""+menuItem.action.shortcut, menuItem.font)
        }
        return s
    }

    function hideFindRow() {
        quickScintillaEditor.focus = true
        findInput.visible = false
        replaceInput.visible = false
    }

    SciteMenuActions {
        id: sciteActions
    }

    SciteMenu {
        id: sciteMenu
    }

    menuBar: sciteMenu

    header: ToolBar {
        contentHeight: readonlyIcon.implicitHeight
        visible: sciteQt.showToolBar

        ToolButton {
            id: readonlyIcon
            //icon.name: "document-open"
            //icon.source: "edit.svg"
            text: "Open"
            visible: sciteQt.showToolBar
            //focusPolicy: Qt.NoFocus
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
        visible: sciteQt.showStatusBar

        text: sciteQt.statusBarText

        MouseArea {
            anchors.fill: parent
            onClicked: sciteQt.onStatusbarClicked()
        }
    }

    Text {
        id: lblFileName
        visible: false
        height: visible ? implicitHeight : 0
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
//            //console.log("Accepted: " + /*currentFile*/fileUrl+" "+fileDialog.openMode)
//            if(!fileDialog.openMode) {
//                writeCurrentDoc(fileUrl)
//            }
//            else {
//                //Android: quickScintillaEditor.text = fileUrl
//                readCurrentDoc(fileUrl)
//            }
            sciteQt.updateCurrentSelectedFileUrl(fileUrl)
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

    FindInFilesDialog {
        id: findInFilesDialog
        objectName: "findInFilesDialog"
        modality: Qt.NonModal
        title: sciteQt.getLocalisedText(qsTr("Find in Files"))
        //width: 450
        //height: 200

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false

        cancelButton {
            onClicked: findInFilesDialog.close()
        }
        findButton {
            onClicked: {
                console.log("find: "+findWhatInput.text)
                //GrabFields()
                //SetFindInFilesOptions()
                //SelectionIntoProperties()
                // --> grep command bauen und ausfuehren....
                // PerformGrep()    // windows
                // FindInFilesCmd() // gtk

                // TODO: implement Qt version of find in files (visiscript?)
            }
        }
    }

    GoToDialog {
        id: gotoDialog
        objectName: "gotoDialog"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Go To"))
        //width: 600
        //height: 100

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false

        cancelButton {
            onClicked: gotoDialog.close()
        }
        gotoButton {
            onClicked: {
                sciteQt.cmdGotoLine(parseInt(gotoDialog.destinationLineInput.text), parseInt(gotoDialog.columnInput.text))
                cancelButton.clicked()
            }
        }
    }

    TabBar {
        id: tabBar

        visible: sciteQt.showTabBar
        height: sciteQt.showTabBar ? implicitHeight : 0
        //focusPolicy: Qt.NoFocus

        anchors.top: lblFileName.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5
    }

    Component {
        id: tabButton
        TabButton {
            property var fcnClicked: undefined
            //focusPolicy: Qt.NoFocus
            text: "some text"
            onClicked: fcnClicked()
        }
    }

    SplitView {
        id: splitView        
        objectName: "SplitView"

        orientation: verticalSplit ? Qt.Horizontal : Qt.Vertical

        property int outputHeight: 0
        property bool verticalSplit: true

        //anchors.fill: parent
        anchors.top: tabBar.bottom
        anchors.right: parent.right
        anchors.bottom: findInput.top
        anchors.left: parent.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

/*
        onHandleChanged: {
            if( verticalSplit )
            {
                console.log("####>> width "+width+" "+verticalSplit+ " "+orientation+" "+startDrag)
                sciteQt.setSpliterPos(width,height,startDrag)
            }
            else
            {
                console.log("####>> height "+height+" "+verticalSplit+ " "+orientation+" "+startDrag)
                sciteQt.setSpliterPos(width,height,startDrag)
            }
        }
*/
        // see: https://doc.qt.io/qt-5/qtquickcontrols2-customize.html#customizing-splitview
        handle: Rectangle {
            implicitWidth: 5
            implicitHeight: 5

            property bool startDrag: false

            color: SplitHandle.pressed ? "#81e889"
                : (SplitHandle.hovered ? Qt.lighter("#c2f4c6", 1.1) : "#c2f4c6")

            onXChanged: {
                if(SplitHandle.pressed) {
                    console.log("drag x... "+(splitView.width-x)+" "+!startDrag)
                    //sciteQt.startDragSpliterPos(splitView.width-x,0)
                    if( !startDrag )
                        startDrag = true
                } else {
                    console.log("finished drag x... "+(splitView.width-x))
                    startDrag = false
                    //sciteQt.setSpliterPos(splitView.width-x,0)
                    //splitView.handleChanged(splitView.width-x,0)
                }
            }
            onYChanged: {
                if(SplitHandle.pressed)  {
                    console.log("drag y... "+(splitView.height-y)+" "+!startDrag)
                    //sciteQt.startDragSpliterPos(0,splitView.height-y)
                    if( !startDrag )
                        startDrag = true
                } else {
                    console.log("finished drag y... "+(splitView.height-y))
                    startDrag = false
                    //sciteQt.setSpliterPos(0,splitView.height-y)
                    //splitView.handleChanged(0,splitView.height-y,!startDrag)
                }
            }

            MouseArea {
                onClicked: console.log("CLICK Splitter")
            }
        }
// TODO: moveSplit --> SciTEBase::MoveSplit() aufrufen !

        ScintillaText {
            id: quickScintillaEditor
            objectName: "ScintillaEditor"

            focus: true
            onFocusChanged: {
                console.log("FOCUS editor changed "+focus)
            }

            SplitView.fillWidth: true
            SplitView.fillHeight: true

            //text: "editor area !"
        }

        ScintillaText {
            id: quickScintillaOutput
            objectName: "ScintillaOutput"

            focus: false
            onFocusChanged: {
                console.log("FOCUS output changed "+focus)
            }

            SplitView.preferredWidth: splitView.outputHeight
            SplitView.preferredHeight: splitView.outputHeight

            //text: "output area !"
        }
    }       

    // Find Dialog above status bar:
    //==============================

    property bool isIncrementalSearch: false

    Label {
        id: findLabel

        visible: findInput.visible
        height: findInput.height //visible ? implicitHeight : 0
        width: max(replaceLabel.implicitWidth, findLabel.implicitWidth)

        anchors.verticalCenter: findNextButton.verticalCenter
        anchors.bottom: replaceInput.top
        anchors.left: parent.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Find:"))
    }

    TextField /*ComboBox*/ {
        id: findInput

        background: Rectangle {
                    radius: 2
                    //implicitWidth: 100
                    //implicitHeight: 24
                    border.color: "grey"
                    border.width: 1
                }

        //editable: true
        visible: false
        height: visible ? implicitHeight : 0

        anchors.right: findNextButton.left
        anchors.bottom: replaceInput.top
        anchors.left: findLabel.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        onAccepted: {
            sciteQt.setFindText(findInput.text, isIncrementalSearch)
            hideFindRow()
        }
        onTextEdited: {
            if( isIncrementalSearch ) {
                sciteQt.setFindText(findInput.text, isIncrementalSearch)
            }
        }
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findNextButton

        visible: findInput.visible
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findMarkAllButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("&Find Next"))
        onClicked: {
            sciteQt.setFindText(findInput.text)
            sciteQt.cmdFindNext()
            hideFindRow()
        }
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findMarkAllButton

        visible: findInput.visible && !isIncrementalSearch
        width: isIncrementalSearch ? 0 : implicitWidth
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findWordOnlyButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Mark &All"))
        onClicked: {
            sciteQt.setFindText(findInput.text)
            sciteQt.cmdMarkAll()
            hideFindRow()
        }
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findWordOnlyButton

        visible: findInput.visible && !isIncrementalSearch
        checkable: true
        flat: true
        width: isIncrementalSearch ? 0 : findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findCaseSensitiveButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("word"))
        checked: sciteQt.wholeWord
        onClicked: sciteQt.wholeWord = !sciteQt.wholeWord
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findCaseSensitiveButton

        visible: findInput.visible && !isIncrementalSearch
        checkable: true
        flat: true
        width: isIncrementalSearch ? 0 : findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findRegExprButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Cc"))
        checked: sciteQt.caseSensitive
        onClicked: sciteQt.caseSensitive = !sciteQt.caseSensitive
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findRegExprButton

        visible: findInput.visible && !isIncrementalSearch
        checkable: true
        flat: true
        width: isIncrementalSearch ? 0 : findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findTransformBackslashButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("^.*"))
        checked: sciteQt.regularExpression
        onClicked: sciteQt.regularExpression = !sciteQt.regularExpression
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findTransformBackslashButton

        visible: findInput.visible && !isIncrementalSearch
        checkable: true
        flat: true
        width: isIncrementalSearch ? 0 : findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findWrapAroundButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("\\r\\t"))
        checked: sciteQt.transformBackslash
        onClicked: sciteQt.transformBackslash = !sciteQt.transformBackslash
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findWrapAroundButton

        visible: findInput.visible && !isIncrementalSearch
        checkable: true
        flat: true
        width: isIncrementalSearch ? 0 : findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findUpButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("wrap"))
        checked: sciteQt.wrapAround
        onClicked: sciteQt.wrapAround = !sciteQt.wrapAround
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findUpButton

        visible: findInput.visible && !isIncrementalSearch
        checkable: true
        flat: true
        width: isIncrementalSearch ? 0 : findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findCloseButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Up"))
        checked: sciteQt.searchUp
        onClicked: sciteQt.searchUp = !sciteQt.searchUp
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: findCloseButton

        visible: findInput.visible
        background: Rectangle {
            color: "red"
        }

        width: 30
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("X"))   // close

        onClicked: hideFindRow()
        Keys.onEscapePressed: hideFindRow()
    }

    // Find/replace Dialog above status bar:
    //==============================

    Label {
        id: replaceLabel

        visible: replaceInput.visible
        height: replaceInput.height //visible ? implicitHeight : 0
        width: max(replaceLabel.implicitWidth, findLabel.implicitWidth)

        anchors.verticalCenter: replaceButton.verticalCenter
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Replace:"))
    }

    TextField /*ComboBox*/ {
        id: replaceInput

        background: Rectangle {
                    radius: 2
                    //implicitWidth: 100
                    //implicitHeight: 24
                    border.color: "grey"
                    border.width: 1
                }

        //editable: true
        visible: false
        height: visible ? implicitHeight : 0
        width: findInput.width

        //anchors.right: replaceButton.left
        anchors.bottom: parent.bottom
        anchors.left: replaceLabel.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        onAccepted: {
            sciteQt.setFindText(findInput.text, isIncrementalSearch)
            hideFindRow()
        }
        onTextEdited: {
            if( isIncrementalSearch ) {
                sciteQt.setFindText(findInput.text, isIncrementalSearch)
            }
        }
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: replaceButton

        visible: replaceInput.visible
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.left: replaceInput.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("&Replace"))
        onClicked: sciteQt.cmdTriggerReplace(findInput.text, replaceInput.text, false)
        Keys.onEscapePressed: hideFindRow()
    }

    Button {
        id: inSectionButton

        visible: replaceInput.visible
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.left: replaceButton.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("In &Section"))
        onClicked: sciteQt.cmdTriggerReplace(findInput.text, replaceInput.text, true)
        Keys.onEscapePressed: hideFindRow()
    }

    Label {
        id: replaceFillLabel

        visible: replaceInput.visible

        //text: ""

        anchors.bottom: parent.bottom
        anchors.left: inSectionButton.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5
    }

    Settings {
        id: settings
        property var splitView
    }

    SciTEQt {
       id: sciteQt
    }

    Connections {
        target: sciteQt

        onTriggerUpdateCurrentWindowPosAndSize: updateCurrentWindowPosAndSize()
        onSetWindowPosAndSize:                  setWindowPosAndSize(left, top, width, height, maximize)

        onStartFileDialog:            startFileDialog(sDirectory, sFilter, sTitle, bAsOpenDialog)
        onShowInfoDialog:             showInfoDialog(sInfoText, style)

        onShowFindInFilesDialog:      showFindInFilesDialog(text)
        onShowFind:                   showFind(text, incremental, withReplace)
        onShowGoToDialog:             showGoToDialog(currentLine, currentColumn, maxLine)

        onSetVerticalSplit:           setVerticalSplit(verticalSplit)
        onSetOutputHeight:            setOutputHeight(heightOutput)

        onInsertTab:                  insertTab(index, title)
        onSelectTab:                  selectTab(index)
        onRemoveAllTabs:              removeAllTabs()
    }

    function removeAllTabs() {
        for(var i=tabBar.count-1; i>=0; i--) {
            tabBar.takeItem(i)
        }
    }

    function selectTab(index) {
        tabBar.setCurrentIndex(index)
    }

    function insertTab(index, title) {
        var item = tabButton.createObject(tabBar, {text: title, fcnClicked: function () { sciteQt.cmdSelectBuffer(index); quickScintillaEditor.focus = true }})
        tabBar.insertItem(index, item)
    }
}
