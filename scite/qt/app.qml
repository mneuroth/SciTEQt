import QtQuick 2.9
import QtQuick.Controls 2.9
import QtQuick.Dialogs 1.2
import QtQml.Models 2.9
import Qt.labs.platform 1.1 as Platform
import QtQuick.Controls 1.4 as Controls1
import QtQuick.Layouts 1.0
//import Qt.labs.settings 1.0

import QtQuick.Window 2.9   // for dialog test only (webassembly)

import org.scintilla.sciteqt 1.0

ApplicationWindow {
    id: applicationWindow
    objectName: "SciteMainWindow"
    width: 600
    height: 800
    visible: true

    property string urlPrefix: "file://"

    // for context menu on tabButton
    property var menuCommandDelegate: undefined

    onClosing: {
        sciteQt.cmdExit()
        close.accepted = false
    }

    onFocusObjectChanged: {
        //console.log("Focus obj changed "+object)
        //logToOutput("Focus obj changed "+object)
    }

    Component.onCompleted: {
        //console.log("ON Completed")
// TODO: gibt es besseren weg scintilla controls bei sciteqt zu registrieren?
        sciteQt.setScintilla(quickScintillaEditor.scintilla)
        sciteQt.setOutput(quickScintillaOutput.scintilla)
        sciteQt.setAboutScite(aboutSciteDialog.scintilla)
        sciteQt.setContent(splitView)
        sciteQt.setMainWindow(applicationWindow)
        sciteQt.setApplicationData(applicationData)
        //console.log("ON Completed done")
        //splitView.restoreState(settings.splitView)
        //sciteQt.showToolBar = true
         sciteQt.logToDebug("=============== APPLICATION START ==========================")
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

    function setTextToCurrent(text) {
        quickScintillaEditor.text = text
        focusToEditor()
    }

    function addTextToOutput(text) {
        quickScintillaOutput.text += text
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

    function showAboutSciteDialog() {
        aboutSciteDialog.show()
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
        gotoDialog.show()
        //gotoDialog.open()
        gotoDialog.destinationLineInput.focus = true
    }

    function showTabSizeDialog(tabSize, indentSize, useTabs) {
        tabSizeDialog.tabSizeInput.text = tabSize
        tabSizeDialog.indentSizeInput.text = indentSize
        tabSizeDialog.useTabsCheckBox.checked = useTabs
        tabSizeDialog.show()
        tabSizeDialog.tabSizeInput.focus = true
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
        focusToEditor()
        findInput.visible = false
        replaceInput.visible = false
    }

    function focusToEditor() {
        //logToOutput("focusToEditor()")
        //quickScintillaEditor.focus = true
        quickScintillaEditor.forceActiveFocus()
        //quickScintillaEditor.update()
    }

    function logToOutput(txt) {
        quickScintillaOutput.text = quickScintillaOutput.text+"\n"+txt+"\n"
    }

    function showTestDialog() {
        testDialog.show()
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
        var item = tabButton.createObject(tabBar, {text: title, fcnClicked: function () { sciteQt.cmdSelectBuffer(index); focusToEditor() }})
        tabBar.insertItem(index, item)
    }

    // *** for webassembly platform ... ***

    function htmlOpen() {
        console.log("htmlOpen");
        htmlFileAccess.loadFsFile("*.*", "/tmp");
    }

    Connections {
        target: htmlFileAccess
        onFsFileReady: {
            console.log("onFsFileReady " + tmpFilePath + " " + fileName)
            readCurrentDoc("file://" + tmpFilePath)
            //loadProject("file://" + tmpFilePath)
        }
    }

    function htmlSave() {
        console.log("htmlSave");
        var tmpFilePath = "/tmp/temp.txt"
        writeCurrentDoc("file://" + tmpFilePath)
        //project.saveAs("file://" + tmpFilePath)
// TODO: current document name...
        htmlFileAccess.saveFsFile(tmpFilePath, "temp.txt")
    }
/*
    function saveOrSaveAs() {
        htmlSave();
        return;

        if (project.url.toString().length > 0) {
            project.save();
        } else {
            saveAsDialog.open();
        }
    }
*/
    // *** for webassembly platform done ***

    SciteMenuActions {
        id: sciteActions
        visible: false
    }

    SciteMenu {
        id: sciteMenu
        visible: true
    }

    menuBar: sciteMenu

    header: ToolBar {
        id: toolBar
        contentHeight: toolBarButtonContainer.implicitHeight
        visible: sciteQt.showToolBar

        property int iconWidth: 24
        property int iconHeight: 24
        /*
          code see: static BarButton bbs[] or void SciTEGTK::AddToolBar()
          icons see: https://material.io/resources/icons/?style=baseline
        */
        Flow {
            id: toolBarButtonContainer

            anchors.fill: parent
            ToolButton {
                id: toolButtonNew
                icon.source: "icons/create.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "New"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdNew()
            }
            ToolButton {
                id: toolButtonOpen
                icon.source: "icons/open_in_new.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Open"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdOpen()
            }
            ToolButton {
                id: toolButtonSave
                icon.source: "icons/save.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Save"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdSave()
            }
            ToolButton {
                id: toolButtonClose
                icon.source: "icons/clear.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Close"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdClose()
            }
            ToolButton {
                id: toolButtonReadonly
                icon.source: "icons/do_not_touch.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Readonly"
                visible: sciteQt.showToolBar
                checkable: true
                onClicked: {
                    quickScintillaEditor.readonly = toolButtonReadonly.checked
                    quickScintillaOutput.readonly = toolButtonReadonly.checked
                    focusToEditor()
                }
            }
            ToolSeparator {
                visible:  sciteQt.showTabBar
            }
            ToolButton {
                id: toolButtonPrint
                icon.source: "icons/print.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Print"
                visible: sciteQt.showToolBar && !sciteQt.isMobilePlatform()
                onClicked: sciteQt.cmdPrint()
            }
            ToolButton {
                id: toolButtonShare
                icon.source: "icons/share.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Share"
                visible: sciteQt.showToolBar && sciteQt.isMobilePlatform()
                onClicked: sciteQt.cmdShare()
            }
            ToolSeparator {
                visible:  sciteQt.showTabBar
            }
            ToolButton {
                id: toolButtonCut
                icon.source: "icons/content_cut.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Cut"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdCut()
            }
            ToolButton {
                id: toolButtonCopy
                icon.source: "icons/content_copy.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Copy"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdCopy()
            }
            ToolButton {
                id: toolButtonPaste
                icon.source: "icons/content_paste.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Paste"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdPaste()
            }
            ToolButton {
                id: toolButtonDelete
                icon.source: "icons/delete.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Delete"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdDelete()
            }
            ToolSeparator {
                visible:  sciteQt.showTabBar
            }
            ToolButton {
                id: toolButtonUndo
                icon.source: "icons/undo.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Undo"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdUndo()
            }
            ToolButton {
                id: toolButtonRedo
                icon.source: "icons/redo.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Redo"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdRedo()
            }
            ToolSeparator {
                visible:  sciteQt.showTabBar
            }
            ToolButton {
                id: toolButtonFind
                icon.source: "icons/find_in_page.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Find"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdFind()
            }
            ToolButton {
                id: toolButtonReplace
                icon.source: "icons/find_replace.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Replace"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdReplace()
            }
            ToolButton {
                id: toolButtonFindPrevious
                icon.source: "icons/arrow_back_ios.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Previous"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdFindPrevious()
            }
            ToolButton {
                id: toolButtonFindNext
                icon.source: "icons/arrow_forward_ios.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Next"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdFindNext()
            }
            ToolSeparator {
                visible:  sciteQt.showTabBar
            }
            ToolButton {
                id: toolButtonBuild
                icon.source: "icons/build.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Build"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdBuild()
            }
            ToolButton {
                id: toolButtonGo
                icon.source: "icons/play_arrow.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Go"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdGo()
            }
            ToolButton {
                id: toolButtonStop
                icon.source: "icons/stop.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: Stop"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdStop()
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

            // Handle context menu on TabButton
            // see: https://stackoverflow.com/questions/32448678/how-to-show-a-context-menu-on-right-click-in-qt5-5-qml-treeview
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: {
                    console.log("show context menu for row: " + text+" "+mouse.x+","+mouse.y +" current="+tabBar.currentIndex+" parent="+parent)

                    // activate this tab and show context menu
                    parent.onClicked()

                    //fill menu in c++, see: SciTEWinBar.Notify() NM_RCLICK
                    var menuItems = sciteQt.fillTabContextMenu()

                    // update menu model with data from c++
                    sciteContextMenuModel.clear()
                    for (let i=0; i<menuItems.length; i++) {
                        sciteContextMenuModel.append({"display":menuItems[i]["display"], "enabled":menuItems[i]["enabled"], "menuId":menuItems[i]["menuId"]})
                    }

                    tabBarContextMenu.popup()
                }
            }
        }
    }

    // see: SciTEBase::ContextMenu
    Menu {
        id: tabBarContextMenu

        Instantiator {
            id: sciteContextMenuItems
            model: sciteContextMenuModel
            delegate: MenuItem {
                text: sciteQt.getLocalisedText(model.display)
                enabled: model.enabled
                onTriggered: menuCommandDelegate !== undefined ? menuCommandDelegate(model.menuId) : sciteQt.cmdContextMenu(model.menuId)
            }

            onObjectAdded: tabBarContextMenu.insertItem(index, object)
            onObjectRemoved: tabBarContextMenu.removeItem(object)
        }
    }

    ListModel {
        id: sciteContextMenuModel
        objectName: "sciteContextMenu"
    }

    Controls1.SplitView {
        id: splitView        
        objectName: "SplitView"

        resizing: true

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
//        handle: Rectangle {
//            implicitWidth: 5
//            implicitHeight: 5

//            property bool startDrag: false

//            color: SplitHandle.pressed ? "#81e889"
//                : (SplitHandle.hovered ? Qt.lighter("#c2f4c6", 1.1) : "#c2f4c6")

//            onXChanged: {
//                if(SplitHandle.pressed) {
//                    console.log("drag x... "+(splitView.width-x)+" "+!startDrag)
//                    //sciteQt.startDragSpliterPos(splitView.width-x,0)
//                    if( !startDrag )
//                        startDrag = true
//                } else {
//                    console.log("finished drag x... "+(splitView.width-x))
//                    startDrag = false
//                    //sciteQt.setSpliterPos(splitView.width-x,0)
//                    //splitView.handleChanged(splitView.width-x,0)
//                }
//            }
//            onYChanged: {
//                if(SplitHandle.pressed)  {
//                    console.log("drag y... "+(splitView.height-y)+" "+!startDrag)
//                    //sciteQt.startDragSpliterPos(0,splitView.height-y)
//                    if( !startDrag )
//                        startDrag = true
//                } else {
//                    console.log("finished drag y... "+(splitView.height-y))
//                    startDrag = false
//                    //sciteQt.setSpliterPos(0,splitView.height-y)
//                    //splitView.handleChanged(0,splitView.height-y,!startDrag)
//                }
//            }

//            MouseArea {
//                onClicked: console.log("CLICK Splitter")
//            }
//        }
// TODO: moveSplit --> SciTEBase::MoveSplit() aufrufen !

        ScintillaText {
            id: quickScintillaEditor
            objectName: "ScintillaEditor"

            fcnLocalisation: sciteQt.getLocalisedText

            focus: true
            //onFocusChanged: {
            //    console.log("FOCUS editor changed "+focus)
            //}

            //SplitView.fillWidth: true
            //SplitView.fillHeight: true
            Layout.fillWidth: true
            Layout.fillHeight: true

            //menuCommandDelegate: sciteQt.cmdContextMenu

            //text: "editor area !"
        }

        ScintillaText {
            id: quickScintillaOutput
            objectName: "ScintillaOutput"

            fcnLocalisation: sciteQt.getLocalisedText

            focus: false
            //onFocusChanged: {
            //    console.log("FOCUS output changed "+focus)
            //}

            //SplitView.preferredWidth: splitView.outputHeight
            //SplitView.preferredHeight: splitView.outputHeight
            width: splitView.outputHeight               // user draging of splitter will brake the binding !!!
            height: splitView.outputHeight
            //implicitWidth: splitView.outputHeight
            //implicitHeight: splitView.outputHeight

            //menuCommandDelegate: sciteQt.cmdContextMenu

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

        anchors.verticalCenter: findInput.verticalCenter
        //anchors.horizontalCenter: findInput.horizontalCenter
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
        Keys.onBackPressed: hideFindRow()
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
            sciteQt.setFindText(findInput.text, isIncrementalSearch)
            sciteQt.cmdFindNext()
            hideFindRow()
        }
        Keys.onEscapePressed: hideFindRow()
        Keys.onBackPressed: hideFindRow()
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
            sciteQt.setFindText(findInput.text, isIncrementalSearch)
            sciteQt.cmdMarkAll()
            hideFindRow()
        }
        Keys.onEscapePressed: hideFindRow()
        Keys.onBackPressed: hideFindRow()
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
        Keys.onBackPressed: hideFindRow()
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
        Keys.onBackPressed: hideFindRow()
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
        Keys.onBackPressed: hideFindRow()
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
        Keys.onBackPressed: hideFindRow()
    }

    ToolButton {
        id: findWrapAroundButton

        visible: findInput.visible && !isIncrementalSearch
        checkable: true
        flat: true
        width: isIncrementalSearch ? 0 : findNextButton.width / 2
        height: findNextButton.height
        icon.source: "icons/loop.svg"
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
        Keys.onBackPressed: hideFindRow()
    }

    ToolButton {
        id: findUpButton

        visible: findInput.visible && !isIncrementalSearch
        checkable: true
        flat: true
        width: isIncrementalSearch ? 0 : findNextButton.width / 2
        height: findNextButton.height
        icon.source: "icons/arrow_upward.svg"
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
        Keys.onBackPressed: hideFindRow()
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
        Keys.onBackPressed: hideFindRow()
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
        Keys.onBackPressed: hideFindRow()
    }

    Button {
        id: replaceButton

        visible: replaceInput.visible
        width: findNextButton.width
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
        Keys.onBackPressed: hideFindRow()
    }

    Button {
        id: inSectionButton

        visible: replaceInput.visible
        width: findMarkAllButton.width
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
        Keys.onBackPressed: hideFindRow()
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
/*
    Settings {
        id: settings
        property var splitView
    }
*/
    SciTEQt {
       id: sciteQt
    }

    Connections {
        target: sciteQt

        onTriggerUpdateCurrentWindowPosAndSize: updateCurrentWindowPosAndSize()
        onSetWindowPosAndSize:                  setWindowPosAndSize(left, top, width, height, maximize)
        onSetTextToCurrent:                     setTextToCurrent(text)
        onAddTextToOutput:                      addTextToOutput(text)

        onStartFileDialog:            startFileDialog(sDirectory, sFilter, sTitle, bAsOpenDialog)
        onShowInfoDialog:             showInfoDialog(sInfoText, style)
        onShowAboutSciteDialog:       showAboutSciteDialog()

        onShowFindInFilesDialog:      showFindInFilesDialog(text)
        onShowFind:                   showFind(text, incremental, withReplace)
        onShowGoToDialog:             showGoToDialog(currentLine, currentColumn, maxLine)
        onShowTabSizeDialog:          showTabSizeDialog(tabSize, indentSize, useTabs)

        onSetVerticalSplit:           setVerticalSplit(verticalSplit)
        onSetOutputHeight:            setOutputHeight(heightOutput)

        onInsertTab:                  insertTab(index, title)
        onSelectTab:                  selectTab(index)
        onRemoveAllTabs:              removeAllTabs()
    }

    // **********************************************************************

    FileDialog {
        id: fileDialog
        objectName: "fileDialog"
        visible: false
        modality: Qt.ApplicationModal
        //fileMode: openMode ? FileDialog.OpenFile : FileDialog.SaveFile
        title: openMode ? qsTr("Choose a file") : qsTr("Save as")
        folder: "."

        property bool openMode: true

        selectExisting: openMode ? true : false
        selectMultiple: false
        selectFolder: false

        onAccepted: {
//            //console.log("Accepted: " + /*currentFile*/fileUrl+" "+fileDialog.openMode)
            sciteQt.logToDebug("FileDialog accepted: "+fileUri)
            if(sciteQt.isWebassemblyPlatform()) {
                if(!fileDialog.openMode) {
                    writeCurrentDoc(fileUrl)
                }
                else {
                    //Android: quickScintillaEditor.text = fileUrl
                    readCurrentDoc(fileUrl)
                }
            }
            else {
                sciteQt.updateCurrentSelectedFileUrl(fileUrl)
            }
            fileDialog.close()
            focusToEditor()
        }
        onRejected: {
            sciteQt.logToDebug("FileDialog rejected: "+fileUri)
            fileDialog.close()
            focusToEditor()
        }
    }

    MessageDialog {
        id: infoDialog
        objectName: "infoDialog"
        visible: false
        title: qsTr("Info")
        //modal: true
        modality: Qt.ApplicationModal
        standardButtons: StandardButton.Ok
        /*
        onAccepted: {
            focusToEditor()
            console.log("accept")
        }
        onRejected: console.log("reject")
        onYes: console.log("yes")
        onNo: console.log("no")
        */
    }

    // test dialog for webassembly tests...
    Window {
        id: testDialog
        width: 200
        height: 200
        visible: false

        Row {
            Button {
                text: "ok"
                onClicked: {
                    logToOutput("test dialog ok")
                    testDialog.close()
                }
            }
            Button {
                text: "cancel"
                onClicked: {
                    logToOutput("test dialog cancel")
                    testDialog.close()
                }
            }
        }
    }

    AboutSciteDialog {
        id: aboutSciteDialog
        objectName: "aboutSciteDialog"
        visible: false
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("About SciTE"))

        fcnLocalisation: sciteQt.getLocalisedText
    }

    Connections {
        target: aboutSciteDialog

        onClosed: focusToEditor()
    }

    FindInFilesDialog {
        id: findInFilesDialog
        objectName: "findInFilesDialog"
        modality: sciteQt.isMobilePlatform() || sciteQt.isWebassemblyPlatform() ? Qt.ApplicationModal : Qt.NonModal
        title: sciteQt.getLocalisedText(qsTr("Find in Files"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: findInFilesDialog

        onCanceled: {
            focusToEditor()
        }
        onAccepted: {
            console.log("find: "+findInFilesDialog.findWhatInput.text)
            //GrabFields()
            //SetFindInFilesOptions()
            //SelectionIntoProperties()
            // --> grep command bauen und ausfuehren....
            // PerformGrep()    // windows
            // FindInFilesCmd() // gtk

            // TODO: implement Qt version of find in files (visiscript?)

            focusToEditor()
        }
    }

    GoToDialog {
        id: gotoDialog
        objectName: "gotoDialog"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Go To"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: gotoDialog

        onCanceled: {
            focusToEditor()
        }
        onAccepted: {
            sciteQt.cmdGotoLine(parseInt(gotoDialog.destinationLineInput.text), parseInt(gotoDialog.columnInput.text))
            focusToEditor()
        }
    }

    TabSizeDialog {
        id: tabSizeDialog
        objectName: "tabSizeDialog"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Indentation Settings"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: tabSizeDialog

        onCanceled: {
            focusToEditor()
        }
        onAccepted: {
            sciteQt.cmdUpdateTabSizeValues(parseInt(tabSizeDialog.tabSizeInput.text), parseInt(tabSizeDialog.indentSizeInput.text), tabSizeDialog.useTabsCheckBox.checked, false)
            focusToEditor()
        }
        onConvert: {
            sciteQt.cmdUpdateTabSizeValues(parseInt(tabSizeDialog.tabSizeInput.text), parseInt(tabSizeDialog.indentSizeInput.text), tabSizeDialog.useTabsCheckBox.checked, true)
            focusToEditor()
        }
    }

    MobileFileDialog {
        id: mobileFileDialog

        fcnLocalisation: sciteQt.getLocalisedText
    }

    Connections {
        target: mobileFileDialog

        onOpenSelectedFile: {
            console.log("OPEN file "+fileName)
            readCurrentDoc(fileName)
        }
        onSaveSelectedFile: {
            console.log("SAVE file "+fileName)
            writeCurrentDoc(buildValidUrl(fileName))
        }
    }

    function openViaMobileFileDialog() {
        mobileFileDialog.setOpenModus()
        mobileFileDialog.show()
    }
    function saveViaMobileFileDialog() {
        mobileFileDialog.setDirectory(mobileFileDialog.currentDirectory)
        mobileFileDialog.setSaveAsModus()
        mobileFileDialog.show()
    }

    Connections {
        target: storageAccess

        onOpenFileContentReceived: {
            //applicationData.logText("==> onOpenFileContentReceived "+fileUri+" "+decodedFileUri)
// TODO does not work (improve!):            window.readCurrentDoc(decodedFileUri) --> stackView.pop() not working
            sciteQt.logToDebug("STORAGE loading: "+fileUri+" "+decodedFileUri)
            //addTextToOutput("loading: "+fileUri)
        //    homePage.currentFileUrl = fileUri
        //    homePage.textArea.text = content // window.readCurrentDoc(fileUri)  //content
        //    homePage.textArea.textDocument.modified = false
        //    homePage.lblFileName.text = applicationData.getOnlyFileName(fileUri)
        //    stackView.pop()
        }
        onOpenFileCanceled: {
            sciteQt.logToDebug("STORAGE canceled !")
        //    stackView.pop()
        }
        onOpenFileError: {
            sciteQt.logToDebug("STORAGE open ERROR ! "+message)
        //    homePage.textArea.text = message
        //    stackView.pop()
        }
        onCreateFileReceived: {
            // create file is used for save as handling !
            //applicationData.logText("onCreateFileReceived "+fileUri)
        //    homePage.currentFileUrl = fileUri
        //    homePage.textArea.textDocument.modified = false
        //    homePage.lblFileName.text = applicationData.getOnlyFileName(fileUri)
            // fill content into the newly created file...
        //    mobileFileDialog.saveAsCurrentFileNow(fileUri)
            //stackView.pop()   // already done in saveAs... above
        }
    }
}
