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

    signal stripFindVisible(bool val)

    property string urlPrefix: "file://"

    // for context menu on tabButton
    property var menuCommandDelegate: undefined

    property int toolTipDelay: 1000
    property int toolTipTimeout: 5000

    onActiveChanged: {
        sciteQt.cmdUpdateApplicationActive(active)
    }

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

    function showFindInFilesDialog(text, findHistory, filePatternHistory, directoryHistory, wholeWord, caseSensitive, regularExpression) {
        findInFilesDialog.findWhatModel.clear()
        findInFilesDialog.findWhatModel.append({"text":text})
        for (let i=0; i<findHistory.length; i++) {
            findInFilesDialog.findWhatModel.append({"text":findHistory[i]})
        }
        findInFilesDialog.filesExtensionsModel.clear()
        for (let i=0; i<filePatternHistory.length; i++) {
            findInFilesDialog.filesExtensionsModel.append({"text":filePatternHistory[i]})
        }
        findInFilesDialog.directoryModel.clear()
        for (let i=0; i<directoryHistory.length; i++) {
            findInFilesDialog.directoryModel.append({"text":directoryHistory[i]})
        }
        findInFilesDialog.findWhatInput.currentIndex = 0
        findInFilesDialog.filesExtensionsInput.currentIndex = 0
        findInFilesDialog.directoryInput.currentIndex = 0
        findInFilesDialog.wholeWordCheckBox.checked = wholeWord
        findInFilesDialog.caseSensitiveCheckBox.checked = caseSensitive
        findInFilesDialog.regularExpressionCheckBox.checked = regularExpression
        findInFilesDialog.show() //.open()
        findInFilesDialog.findWhatInput.selectAll()
        findInFilesDialog.findWhatInput.focus = true
    }

    function showFindStrip(findHistory, replaceHistory, text, incremental, withReplace, closeOnFind) {
        stripFindWhatModel.clear()
        stripFindWhatModel.append({"text":text})
        for (let i=0; i<findHistory.length; i++) {
            stripFindWhatModel.append({"text":findHistory[i]})
        }
        findInput.currentIndex = 0

        stripReplaceWithModel.clear()
        stripReplaceWithModel.append({"text":""})
        for (let i=0; i<replaceHistory.length; i++) {
            stripReplaceWithModel.append({"text":replaceHistory[i]})
        }
        replaceInput.currentIndex = 0

        findInput.visible = true
        replaceInput.visible = withReplace
        if( withReplace ) {
            replaceInput.focus = true
        } else {
            findInput.focus = true
        }

        isIncrementalSearch = incremental
        isCloseOnFind = closeOnFind

        stripFindVisible(true)
    }

    function showReplace(findHistory, replaceHistory, text, replace, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down) {
        replaceDialog.findWhatModel.clear()
        replaceDialog.findWhatModel.append({"text":text})
        for (let i=0; i<findHistory.length; i++) {
            replaceDialog.findWhatModel.append({"text":findHistory[i]})
        }
        replaceDialog.replaceWithModel.clear()
        replaceDialog.replaceWithModel.append({"text":replace})
        for (let i=0; i<replaceHistory.length; i++) {
            replaceDialog.replaceWithModel.append({"text":replaceHistory[i]})
        }
        replaceDialog.findWhatInput.currentIndex = 0
        replaceDialog.replaceWithInput.currentIndex = 0
        replaceDialog.matchWholeWordCheckBox.checked = wholeWord
        replaceDialog.caseSensitiveCheckBox.checked = caseSensitive
        replaceDialog.regularExpressionCheckBox.checked = regExpr
        replaceDialog.wrapAroundCheckBox.checked = wrap
        replaceDialog.show()
        replaceDialog.findWhatInput.selectAll()
        replaceDialog.findWhatInput.focus = true
    }

    function showFind(findHistory, text, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down) {
        findDialog.findWhatModel.clear()
        findDialog.findWhatModel.append({"text":text})
        for (let i=0; i<findHistory.length; i++) {
            findDialog.findWhatModel.append({"text":findHistory[i]})
        }
        findDialog.findWhatInput.currentIndex = 0
        findDialog.matchWholeWordCheckBox.checked = wholeWord
        findDialog.caseSensitiveCheckBox.checked = caseSensitive
        findDialog.regularExpressionCheckBox.checked = regExpr
        findDialog.wrapAroundCheckBox.checked = wrap
        findDialog.searchUpButton.checked = !down
        findDialog.searchDownButton.checked = down
        findDialog.show()
        findDialog.findWhatInput.selectAll()
        findDialog.findWhatInput.focus = true
    }

    function doExecuteFind(findDialog, markAll, useDown) {
        var findWhatInput = findDialog.findWhatInput.editText.length > 0 ? findDialog.findWhatInput.editText : findDialog.findWhatInput.currentText
        var wholeWord = findDialog.matchWholeWordCheckBox.checked
        var caseSensitive = findDialog.caseSensitiveCheckBox.checked
        var regularExpression = findDialog.regularExpressionCheckBox.checked
        var wrap = findDialog.wrapAroundCheckBox.checked
        var transformBackslash = findDialog.tramsformBackslashExprCheckBox.checked        
        var down = useDown ? findDialog.searchDownButton.checked : true
        var shouldClose = sciteQt.cmdExecuteFind(findWhatInput, wholeWord, caseSensitive, regularExpression, wrap, transformBackslash, down, markAll)
        if(shouldClose)
        {
            findDialog.close()
            focusToEditor()
        }
    }

    function doExecuteReplace(replaceDialog, replaceAll, replaceInSection) {
        var findWhatInput = replaceDialog.findWhatInput.editText.length > 0 ? replaceDialog.findWhatInput.editText : replaceDialog.findWhatInput.currentText
        var replace = replaceDialog.replaceWithInput.editText.length > 0 ? replaceDialog.replaceWithInput.editText : replaceDialog.replaceWithInput.currentText
        var wholeWord = replaceDialog.matchWholeWordCheckBox.checked
        var caseSensitive = replaceDialog.caseSensitiveCheckBox.checked
        var regularExpression = replaceDialog.regularExpressionCheckBox.checked
        var wrap = replaceDialog.wrapAroundCheckBox.checked
        var transformBackslash = replaceDialog.tramsformBackslashExprCheckBox.checked
        var shouldClose = sciteQt.cmdExecuteReplace(findWhatInput, replace, wholeWord, caseSensitive, regularExpression, wrap, transformBackslash, replaceAll, replaceInSection)
        if(shouldClose)
        {
            replaceDialog.close()
            focusToEditor()
        }
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

    function showAbbreviationDialog(items) {
        abbreviationDialog.abbreviationModel.clear()
        for (let i=0; i<items.length; i++) {
            abbreviationDialog.abbreviationModel.append({"text":items[i]})
        }
        abbreviationDialog.show()
        abbreviationDialog.abbreviationInput.focus = true
    }

    function showParametersDialog(modal, parameters) {
        parametersDialog.modality = modal ? Qt.ApplicationModal : Qt.NonModal
        parametersDialog.isModal = modal
        parametersDialog.show()
        parametersDialog.parameter1Input.focus = !modal
        parametersDialog.cmdInput.focus = modal
    }

    function closeFindReplaceDialog() {
        findDialog.close()
        replaceDialog.close()
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

        stripFindVisible(false)
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

    function insertTab(index, title, _fullPath) {
        var item = tabButton.createObject(tabBar, {text: title, fullPath: _fullPath, fcnClicked: function () { sciteQt.cmdSelectBuffer(index); focusToEditor() }})
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

    Connections {
        target: sciteMenu

        onReadOnlyChanged: {
            toolButtonReadonly.checked = value
        }
        onRunningChanged: {
            toolButtonGo.enabled = !value
            toolButtonGo.checked = value
            toolButtonStop.enabled = value
            toolButtonStop.checked = !value
        }
        onBuildChanged: {
            toolButtonBuild.enabled = !value
            toolButtonBuild.checked = value
        }
        onCopyCutChanged: {
            toolButtonCopy.enabled = !value
            toolButtonCopy.checked = value
            toolButtonCut.enabled = !value
            toolButtonCut.checked = value
            toolButtonDelete.enabled = !value
            toolButtonDelete.checked = value
        }
        onPasteChanged: {
            toolButtonPaste.enabled = !value
            toolButtonPaste.checked = value
        }
        onUndoChanged: {
            toolButtonUndo.enabled = !value
            toolButtonUndo.checked = value
        }
        onRedoChanged: {
            toolButtonRedo.enabled = !value
            toolButtonRedo.checked = value
        }
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

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Create a new document"))
            }
            ToolButton {
                id: toolButtonOpen
                icon.source: "icons/open_in_new.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Open"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdOpen()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Open an existing document"))
            }
            ToolButton {
                id: toolButtonSave
                icon.source: "icons/save.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Save"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdSave()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Save current document"))
            }
            ToolButton {
                id: toolButtonClose
                icon.source: "icons/clear.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Close"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdClose()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Close current document"))
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
                    //quickScintillaEditor.readonly = toolButtonReadonly.checked
                    //quickScintillaOutput.readonly = toolButtonReadonly.checked
                    //focusToEditor()
                    sciteQt.cmdReadOnly()
                }
                // see: action: actionReadOnly

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Disable possibility to modify current document"))
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

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Print current document"))
            }
            ToolButton {
                id: toolButtonShare
                icon.source: "icons/share.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Share"
                visible: sciteQt.showToolBar && sciteQt.isMobilePlatform()
                onClicked: sciteQt.cmdShare()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Share current document"))
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

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Cut current selected text to clipboard"))
            }
            ToolButton {
                id: toolButtonCopy
                icon.source: "icons/content_copy.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Copy"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdCopy()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Copy current selected text to clipboard"))
            }
            ToolButton {
                id: toolButtonPaste
                icon.source: "icons/content_paste.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Paste"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdPaste()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Paste text from clipboard"))
            }
            ToolButton {
                id: toolButtonDelete
                icon.source: "icons/delete.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Delete"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdDelete()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Delete current selected text"))
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

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Undo last modification"))
            }
            ToolButton {
                id: toolButtonRedo
                icon.source: "icons/redo.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Redo"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdRedo()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Redo last modification"))
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

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Search for text"))
            }
            ToolButton {
                id: toolButtonReplace
                icon.source: "icons/find_replace.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Replace"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdReplace()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Find and replace text"))
            }
            ToolButton {
                id: toolButtonFindPrevious
                icon.source: "icons/arrow_back_ios.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Previous"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdFindPrevious()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Search previous text"))
            }
            ToolButton {
                id: toolButtonFindNext
                icon.source: "icons/arrow_forward_ios.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Next"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdFindNext()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Search next text"))
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

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Trigger build"))
            }
            ToolButton {
                id: toolButtonGo
                icon.source: "icons/play_arrow.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: "Go"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdGo()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Run script"))
            }
            ToolButton {
                id: toolButtonStop
                icon.source: "icons/stop.svg"
                icon.height: toolBar.iconHeight
                icon.width: toolBar.iconWidth
                //text: Stop"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdStopExecuting()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Stop script execution"))
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
            property string fullPath: "?"
            //focusPolicy: Qt.NoFocus
            text: "some text"
            onClicked: fcnClicked()

            ToolTip.delay: toolTipDelay
            ToolTip.timeout: toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: fullPath

            // Handle context menu on TabButton
            // see: https://stackoverflow.com/questions/32448678/how-to-show-a-context-menu-on-right-click-in-qt5-5-qml-treeview
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: {
                    //console.log("show context menu for row: " + text+" "+mouse.x+","+mouse.y +" current="+tabBar.currentIndex+" parent="+parent)

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
    property bool isCloseOnFind: true

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

    ComboBox {
        id: findInput

        editable: true
        visible: false
        height: visible ? implicitHeight : 0

        model: stripFindWhatModel

        anchors.right: findNextButton.left
        anchors.bottom: replaceInput.top
        anchors.left: findLabel.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        onAccepted: {
            sciteQt.setFindText(getCurrentFindText(), isIncrementalSearch)
            if(isCloseOnFind) {     // TODO: handling of close after find is not correct !
                hideFindRow()
            }
        }
        onEditTextChanged: {
            if( isIncrementalSearch ) {
                sciteQt.setFindText(editText, isIncrementalSearch)
            }
        }

        Keys.onEscapePressed: hideFindRow()
        Keys.onBackPressed: hideFindRow()
    }

    ListModel {
        id: stripFindWhatModel
        objectName: "stripFindWhatModel"
    }

    function getCurrentFindText() {
        var curFindInput = findInput.editText.length > 0 ? findInput.editText : findInput.currentText
        return curFindInput
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

        text: sciteQt.getLocalisedText(qsTr("&Find Next"),false)
        onClicked: {
            sciteQt.setFindText(getCurrentFindText(), isIncrementalSearch)
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

        text: sciteQt.getLocalisedText(qsTr("Mark &All"),false)
        onClicked: {
            sciteQt.setFindText(getCurrentFindText(), isIncrementalSearch)
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

        ToolTip.delay: toolTipDelay
        ToolTip.timeout: toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: sciteQt.getLocalisedText(qsTr("Find only whole words"))
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

        ToolTip.delay: toolTipDelay
        ToolTip.timeout: toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: sciteQt.getLocalisedText(qsTr("Case sensitive search"))
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

        ToolTip.delay: toolTipDelay
        ToolTip.timeout: toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: sciteQt.getLocalisedText(qsTr("Use regular expression for search"))
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

        ToolTip.delay: toolTipDelay
        ToolTip.timeout: toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: sciteQt.getLocalisedText(qsTr("Transform backslashes"))
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

        ToolTip.delay: toolTipDelay
        ToolTip.timeout: toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: sciteQt.getLocalisedText(qsTr("Wrapp around search"))
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

        ToolTip.delay: toolTipDelay
        ToolTip.timeout: toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: sciteQt.getLocalisedText(qsTr("Seach upward"))
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

        ToolTip.delay: toolTipDelay
        ToolTip.timeout: toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: sciteQt.getLocalisedText(qsTr("Close"))
    }

    // Find/replace Dialog above status bar:
    //======================================

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

    ComboBox {
        id: replaceInput

        editable: true
        visible: false
        height: visible ? implicitHeight : 0
        width: findInput.width

        model: stripReplaceWithModel

        //anchors.right: replaceButton.left
        anchors.bottom: parent.bottom
        anchors.left: replaceLabel.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        onAccepted: {
            sciteQt.setFindText(getCurrentFindText(), isIncrementalSearch)
            //hideFindRow()
        }

        Keys.onEscapePressed: hideFindRow()
        Keys.onBackPressed: hideFindRow()
    }

    ListModel {
        id: stripReplaceWithModel
        objectName: "stripReplaceWithModel"
    }

    function getCurrentReplaceText() {
        var curReplaceInput = replaceInput.editText.length > 0 ? replaceInput.editText : replaceInput.currentText
        return curReplaceInput
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

        text: sciteQt.getLocalisedText(qsTr("&Replace"),false)
        onClicked: sciteQt.cmdTriggerReplace(getCurrentFindText(), getCurrentReplaceText(), false)
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

        text: sciteQt.getLocalisedText(qsTr("In &Section"),false)
        onClicked: sciteQt.cmdTriggerReplace(getCurrentFindText(), getCurrentReplaceText(), true)
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

        onShowFindInFilesDialog:      showFindInFilesDialog(text, findHistory, filePatternHistory, directoryHistory, wholeWord, caseSensitive, regularExpression)
        onShowFindStrip:              showFindStrip(findHistory, replaceHistory, text, incremental, withReplace, closeOnFind)
        onShowFind:                   showFind(findHistory, text, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down)
        onShowReplace:                showReplace(findHistory, replaceHistory, text, replace, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down)
        onShowGoToDialog:             showGoToDialog(currentLine, currentColumn, maxLine)
        onShowTabSizeDialog:          showTabSizeDialog(tabSize, indentSize, useTabs)
        onShowAbbreviationDialog:     showAbbreviationDialog(items)
        onShowParametersDialog:       showParametersDialog(modal, parameters)
        onCloseFindReplaceDialog:     closeFindReplaceDialog()

        onSetVerticalSplit:           setVerticalSplit(verticalSplit)
        onSetOutputHeight:            setOutputHeight(heightOutput)

        onInsertTab:                  insertTab(index, title, fullPath)
        onSelectTab:                  selectTab(index)
        onRemoveAllTabs:              removeAllTabs()
    }

    // **********************************************************************

    Platform.FolderDialog {
        id: folderDialog
        objectName: "folderDialog"
        visible: false
        modality: Qt.ApplicationModal
        title: qsTr("Choose a directory")
    }

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
            sciteQt.logToDebug("FileDialog accepted: "+fileUrl)
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
            sciteQt.logToDebug("FileDialog rejected: "+fileUrl)
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

    FindDialog {
        id: findDialog
        objectName: "findDialog"
        modality: sciteQt.isMobilePlatform() || sciteQt.isWebassemblyPlatform() ? Qt.ApplicationModal : Qt.NonModal
        title: sciteQt.getLocalisedText(qsTr("Find"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: findDialog

        onAccepted: doExecuteFind(findDialog, false, true)

        onMarkAll:  doExecuteFind(findDialog, true, true)
    }

    ReplaceDialog {
        id: replaceDialog
        objectName: "replaceDialog"
        modality: sciteQt.isMobilePlatform() || sciteQt.isWebassemblyPlatform() ? Qt.ApplicationModal : Qt.NonModal
        title: sciteQt.getLocalisedText(qsTr("Replace"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: replaceDialog

        onAccepted:         doExecuteFind(replaceDialog, false, false)

        onReplace:          doExecuteReplace(replaceDialog,false,false)
        onReplaceAll:       doExecuteReplace(replaceDialog,true,false)
        onReplaceInSection: doExecuteReplace(replaceDialog,false,true)
    }

    Connections {
        target: sciteQt

        onUpdateReplacementCount: {
            replaceDialog.countReplacementsLabel.text = count
        }
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
            var findWhatInput = findInFilesDialog.findWhatInput.editText.length > 0 ? findInFilesDialog.findWhatInput.editText : findInFilesDialog.findWhatInput.currentText
            var filesExtensionsInput = findInFilesDialog.filesExtensionsInput.editText.length > 0 ? findInFilesDialog.filesExtensionsInput.editText : findInFilesDialog.filesExtensionsInput.currentText
            var directoryInput = findInFilesDialog.directoryInput.editText.length > 0 ? findInFilesDialog.directoryInput.editText : findInFilesDialog.directoryInput.currentText
            var wholeWord = findInFilesDialog.wholeWordCheckBox.checked
            var caseSensitive = findInFilesDialog.caseSensitiveCheckBox.checked
            var regularExpression = findInFilesDialog.regularExpressionCheckBox.checked
            sciteQt.cmdStartFindInFilesAsync(directoryInput, filesExtensionsInput, findWhatInput, wholeWord, caseSensitive, regularExpression)
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

    AbbreviationDialog {
        id: abbreviationDialog
        objectName: "abbreviationDialog"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Insert Abbreviation"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: abbreviationDialog

        onCanceled: {
            focusToEditor()
        }
        onAccepted: {
            sciteQt.cmdSetAbbreviationText(abbreviationDialog.abbreviationInput.currentText)
            focusToEditor()
        }
    }

    ParametersDialog {
        id: parametersDialog
        objectName: "parametersDialog"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Parameters"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: parametersDialog

        onCanceled: {
            focusToEditor()
            sciteQt.cmdParametersDialogClosed()
        }
        onAccepted: {
            sciteQt.cmdSetParameters(parametersDialog.cmdInput.text, parametersDialog.parameter1Input.text, parametersDialog.parameter2Input.text, parametersDialog.parameter3Input.text, parametersDialog.parameter4Input.text)
            sciteQt.cmdParametersDialogClosed()
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
