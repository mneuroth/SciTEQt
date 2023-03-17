/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.9                // 6.x for Qt 6
//import QtQuick.Controls 2.3     // for MenuBar (Qt 5.10)
//import QtQuick.Controls 1.4 as Controls1
import QtQuick.Controls //6.2       // see: https://doc.qt.io/qt-6/qtquickcontrols-index.html
import QtQuick.Dialogs //6.2 //1.3
import QtQml.Models 2.1
import QtQuick.Layouts 1.0
import Qt.labs.settings 1.0
//import QtQuick.Controls.Material 2.12

import QtQuick.Window 2.1   // for dialog test only (webassembly)

import Qt.labs.platform 1.0 as Labs

import org.scintilla.sciteqt 1.0

ApplicationWindow {
    id: applicationWindow
    objectName: "SciteMainWindow"
    width: 600
    height: 800
    visible: true

    property bool isAppStoreSupported: applicationData !== null ? applicationData.isAppStoreSupported : false

// https://doc.qt.io/qt-5/qtquickcontrols2-styles.html#using-styles-in-qt-quick-controls
//   https://doc.qt.io/qt-5/qtquickcontrols2-material.html
//    Material.theme: Material.Dark
//    Material.accent: Material.Purple

    signal stripFindVisible(bool val)

    property int defaultIconSize: 40
    property int iconSize: 40

    property string urlPrefix: "file://"

    // for context menu on tabButton
    property var menuCommandDelegate: undefined

    property int toolTipDelay: 1000
    property int toolTipTimeout: 5000

    onActiveChanged: {
        sciteQt.cmdUpdateApplicationActive(active)
    }

    onClosing: (close) => {
        sciteQt.cmdExit()
        close.accepted = false // close will be handled in close dialog...
    }

    //onFocusObjectChanged: {
    //    //console.log("Focus obj changed "+object)
    //    //logToOutput("Focus obj changed "+object)
    //}

    Component.onCompleted: {
// TODO: gibt es besseren weg scintilla controls bei sciteqt zu registrieren?
        //console.log("========== APP START ============")
        sciteQt.setScintilla(quickScintillaEditor.scintilla)
        sciteQt.setOutput(quickScintillaOutput.scintilla)
        var aboutDlg = sciteQt.useMobileDialogHandling ? aboutSciteDialog : aboutSciteDialogWin
        sciteQt.setAboutScite(aboutDlg.scintilla)
        sciteQt.setContent(splitView)
        sciteQt.setMainWindow(applicationWindow)
        sciteQt.setApplicationData(applicationData)
        //splitView.restoreState(settings.splitView)              // TODO: check initial splitter width after new installation !
        //console.log("========== APP START done. ============")
    }
    //Component.onDestruction: {
    //    //settings.splitView = splitView.saveState()
    //}

    function onTitleChanged(title) {
        labelTitel.text = title
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

    function dismissMenu() {
        if(sciteQt.mobilePlatform) {
            mobilePopupMenu.dismiss()
        }
        else {
            var currentMenu = sciteMenuBar.currentItem
            if(currentMenu !== null)
            {
                currentMenu.menu.dismiss()
            }
        }
    }

    function setTextToCurrent(text) {
        quickScintillaEditor.text = text
        focusToEditor()
    }

    function addTextToOutput(text) {
        quickScintillaOutput.text += text
    }

    function startFileDialog(sDirectory, sFilter, sTitle, bAsOpenDialog, bSaveACopyModus, bDeleteModus, sDefaultSaveAsName) {
        if(sciteQt.isWebassemblyPlatform())
        {
            if(bAsOpenDialog) {
                htmlOpen()
            } else {
// TODO save as...
// TODO support export as ... like pdf, tex, etc.
                htmlSave(null,null)
            }
        }
        else if(sciteQt.mobilePlatform)
        {
            if(bAsOpenDialog) {
                openViaMobileFileDialog(sDirectory)
            } else if(bDeleteModus) {
                deleteViaMobileFileDialog(sDirectory)
            } else {
                saveViaMobileFileDialog(sDirectory,sDefaultSaveAsName,bSaveACopyModus)
            }
        }
        else
        {
            if(sciteQt.isMacOSPlatform())
            {
                labsFileDialog.fileMode = bAsOpenDialog ? 0 : 2
                labsFileDialog.folder = sDirectory
                if( sTitle !== undefined && sTitle.length > 0 ) {
                    fileDialog.title = sciteQt.getLocalisedText(sTitle)
                }
                if( sFilter.length > 0) {
                    fileDialog.nameFilters = [sFilter]
                }
                else {
                    fileDialog.nameFilters = ["*"]
                }
                labsFileDialog.open()
            }
            else
            {
                //fileDialog.selectExisting = bAsOpenDialog
                fileDialog.openMode = bAsOpenDialog
                //fileDialog.folder = sDirectory
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
        }
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
        return sciteQt.saveCurrentAs(url)
    }

    function readCurrentDoc(url) {
        // then read new document
        var urlFileName = buildValidUrl(url)
        sciteQt.doOpen(url)
        focusToEditor()
    }

    function selectDirectoryViaMobileFileDialog(directory) {
        mobileFileDialog.setDirectory(directory)
        mobileFileDialog.setDirectoryModus()
        //mobileFileDialog.show()
        //stackView.pop()
        stackView.push(mobileFileDialog)
        mobileFileDialog.forceActiveFocus()
    }

    function deleteViaMobileFileDialog(directory) {
        mobileFileDialog.setDirectory(directory)
        mobileFileDialog.setDeleteModus()
        //mobileFileDialog.show()
        stackView.pop()
        stackView.push(mobileFileDialog)
        mobileFileDialog.forceActiveFocus()
    }

    function openViaMobileFileDialog(directory) {
        mobileFileDialog.setDirectory(directory)
        mobileFileDialog.setOpenModus()
        //mobileFileDialog.show()
        stackView.pop()
        stackView.push(mobileFileDialog)
        mobileFileDialog.forceActiveFocus()
    }

    function saveViaMobileFileDialog(directory,sDefaultSaveAsName,bSaveACopyModus) {
        mobileFileDialog.setDirectory(directory/*mobileFileDialog.currentDirectory*/)
        mobileFileDialog.setSaveAsModus(sDefaultSaveAsName,bSaveACopyModus)
        //mobileFileDialog.show()
        stackView.pop()
        stackView.push(mobileFileDialog)
        mobileFileDialog.forceActiveFocus()
    }

    function showInfoDialog(infoText,style) {
        //mbsOK = 0,
        //mbsYesNo = 4,
        //mbsYesNoCancel = 3,
        //mbsIconQuestion = 0x20,
        //mbsIconWarning = 0x30
        var dlg = sciteQt.isWebassemblyPlatform() ? infoDialogPage : infoDialog
        if((style & 7) === 4)
        {
            dlg.buttons = Labs.MessageDialog.Yes | Labs.MessageDialog.No
        }
        else if((style & 7) === 3)
        {
            dlg.buttons = Labs.MessageDialog.Yes | Labs.MessageDialog.No | Labs.MessageDialog.Cancel
        }
        else
        {
            dlg.buttons = Labs.MessageDialog.Ok
        }

        dlg.text = infoText
        if(sciteQt.isWebassemblyPlatform()) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.open()
        }
    }

    function showAboutSciteDialog() {
        var dlg = sciteQt.useMobileDialogHandling ? aboutSciteDialog : aboutSciteDialogWin
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.show()
        }
    }

    function showFindInFilesDialog(text, findHistory, filePatternHistory, directoryHistory, wholeWord, caseSensitive, regularExpression) {
        var dlg = sciteQt.useMobileDialogHandling ? findInFilesDialog : findInFilesDialogWin
        dlg.findWhatModel.clear()
        dlg.findWhatModel.append({"text":text})
        for (var i=0; i<findHistory.length; i++) {
            dlg.findWhatModel.append({"text":findHistory[i]})
        }
        dlg.filesExtensionsModel.clear()
        for (var i=0; i<filePatternHistory.length; i++) {
            dlg.filesExtensionsModel.append({"text":filePatternHistory[i]})
        }
        dlg.directoryModel.clear()
        for (var i=0; i<directoryHistory.length; i++) {
            dlg.directoryModel.append({"text":directoryHistory[i]})
        }
        dlg.findWhatInput.currentIndex = 0
        dlg.filesExtensionsInput.currentIndex = 0
        dlg.directoryInput.currentIndex = 0
        dlg.wholeWordCheckBox.checked = wholeWord
        dlg.caseSensitiveCheckBox.checked = caseSensitive
        dlg.regularExpressionCheckBox.checked = regularExpression
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.show() //.open()
        }
        dlg.findWhatInput.selectAll()
        dlg.findWhatInput.focus = true
    }

    function showFindStrip(findHistory, replaceHistory, text, incremental, withReplace, closeOnFind, isFilter) {
        stripFindWhatModel.clear()
        stripFindWhatModel.append({"text":text})
        for (var i=0; i<findHistory.length; i++) {
            stripFindWhatModel.append({"text":findHistory[i]})
        }
        findInput.currentIndex = 0

        stripReplaceWithModel.clear()
        stripReplaceWithModel.append({"text":""})
        for (var i=0; i<replaceHistory.length; i++) {
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
        isFilterSearch = isFilter
        isCloseOnFind = closeOnFind

        if( isFilter ) {
            findLabel.text = sciteQt.getLocalisedText(qsTr("Filter:"))
        } else {
            findLabel.text = sciteQt.getLocalisedText(qsTr("Find:"))
        }

        stripFindVisible(true)
    }

    function showReplace(findHistory, replaceHistory, text, replace, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down) {
        var dlg = sciteQt.useMobileDialogHandling ? replaceDialog : replaceDialogWin
        dlg.findWhatModel.clear()
        dlg.findWhatModel.append({"text":text})
        for (var i=0; i<findHistory.length; i++) {
            dlg.findWhatModel.append({"text":findHistory[i]})
        }
        dlg.replaceWithModel.clear()
        dlg.replaceWithModel.append({"text":replace})
        for (var i=0; i<replaceHistory.length; i++) {
            dlg.replaceWithModel.append({"text":replaceHistory[i]})
        }
        dlg.findWhatInput.currentIndex = 0
        dlg.replaceWithInput.currentIndex = 0
        dlg.matchWholeWordCheckBox.checked = wholeWord
        dlg.caseSensitiveCheckBox.checked = caseSensitive
        dlg.regularExpressionCheckBox.checked = regExpr
        dlg.wrapAroundCheckBox.checked = wrap
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.show()
        }
        dlg.findWhatInput.selectAll()
        dlg.findWhatInput.focus = true
    }

    function showFind(findHistory, text, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down) {
        var dlg = sciteQt.useMobileDialogHandling ? findDialog : findDialogWin
        dlg.findWhatModel.clear()
        dlg.findWhatModel.append({"text":text})
        for (var i=0; i<findHistory.length; i++) {
            dlg.findWhatModel.append({"text":findHistory[i]})
        }
        dlg.findWhatInput.currentIndex = 0
        dlg.matchWholeWordCheckBox.checked = wholeWord
        dlg.caseSensitiveCheckBox.checked = caseSensitive
        dlg.regularExpressionCheckBox.checked = regExpr
        dlg.wrapAroundCheckBox.checked = wrap
        dlg.searchUpButton.checked = !down
        dlg.searchDownButton.checked = down
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.show()
        }
        dlg.findWhatInput.selectAll()
        dlg.findWhatInput.focus = true
        dlg.findWhatInput.forceActiveFocus()
        dlg.requestActivate()
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
            if(sciteQt.useMobileDialogHandling) {
                stackView.pop()
            } else {
                findDialog.close()
            }
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
            if(sciteQt.useMobileDialogHandling) {
                stackView.pop()
            } else {
                replaceDialog.close()
            }
            focusToEditor()
        }
    }

    function showGoToDialog(currentLine, currentColumn, maxLine) {
        var dlg = sciteQt.useMobileDialogHandling ? gotoDialog : gotoDialogWin
        dlg.destinationLineInput.text = ""
        dlg.columnInput.text = ""
        dlg.currentLineOutput.text = currentLine
        dlg.currentColumnOutput.text = currentColumn
        dlg.lastLineOutput.text = maxLine
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.show()
        }
        dlg.destinationLineInput.focus = true
    }

    function showTabSizeDialog(tabSize, indentSize, useTabs) {
        var dlg = sciteQt.useMobileDialogHandling ? tabSizeDialog : tabSizeDialogWin
        dlg.tabSizeInput.text = tabSize
        dlg.indentSizeInput.text = indentSize
        dlg.useTabsCheckBox.checked = useTabs
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.show()
        }
        dlg.tabSizeInput.focus = true
    }

    function showAbbreviationDialog(items) {
        var dlg = sciteQt.useMobileDialogHandling ? abbreviationDialog : abbreviationDialogWin
        dlg.abbreviationModel.clear()
        for (var i=0; i<items.length; i++) {
            dlg.abbreviationModel.append({"text":items[i]})
        }
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.show()
        }
        dlg.abbreviationInput.focus = true
    }

    function showParametersDialog(modal, parameters) {
        var dlg = sciteQt.useMobileDialogHandling ? parametersDialog : parametersDialogWin
        dlg.isModal = modal
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.modality = modal ? Qt.ApplicationModal : Qt.NonModal
            dlg.show()
        }
        dlg.parameter1Input.focus = !modal
        dlg.cmdInput.focus = modal
    }

    function closeFindReplaceDialog() {
        if(sciteQt.useMobileDialogHandling)
        {
            stackView.pop()
        }
        else {
            if(findDialog.visible) {
                findDialog.close()
            }
            if(replaceDialog.visible) {
                replaceDialog.close()
            }
        }
        focusToEditor()
    }

    function setVerticalSplit(verticalSplit) {
        splitView.verticalSplit = verticalSplit
    }

    function setOutputHeight(heightOutput) {
        splitView.outputHeight = heightOutput
    }

    function processMenuItem(menuText, menuItem) {
// TODO: via font die breite fuer den text bestimmen und passende anzahl leerzeichen dafuer...
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
        var s = sciteQt.getLocalisedText(menuText)
        if( menuItem !== null && menuItem.action !== null && menuItem.action.shortcut !== undefined)
        {
            //s += " \t" + menuItem.shortcut
            var sc = sciteQt.mobilePlatform ? "" : ""+menuItem.action.shortcut
            return sciteQt.fillToLengthWithFont(s, sc, menuItem.font)
        }
        return s
    }

    function hideFindRow() {
        focusToEditor()
        findInput.visible = false
        replaceInput.visible = false

        stripFindVisible(false)

        if(isFilterSearch) {
            sciteQt.clearFilterAll()
        }
    }

    function focusToEditor() {
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

    function showSupportSciteQtDialog() {
        var dlg = sciteQt.useMobileDialogHandling ? supportDialog : supportDialogWin
        if(sciteQt.useMobileDialogHandling) {
            stackView.pop()
            stackView.push(dlg)
            dlg.forceActiveFocus()
        } else {
            dlg.show()
        }
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
        var item = tabButton.createObject(tabBar, {text: title, fullPath: _fullPath, fcnClicked: function () { sciteQt.cmdSelectBuffer(index); focusToEditor(); sciteQt.cmdEnsureCursorVisible() }})
        tabBar.insertItem(index, item)
    }

    function getCurrentFindText() {
        var curFindInput = findInput.editText.length > 0 ? findInput.editText : findInput.currentText
        return curFindInput
    }

    function getCurrentReplaceText() {
        var curReplaceInput = replaceInput.editText.length > 0 ? replaceInput.editText : replaceInput.currentText
        return curReplaceInput
    }

    // *** for webassembly platform ... ***

    function htmlOpen() {
        htmlFileAccess.loadFsFile("*.*", "/tmp");
    }

    Connections {
        target: htmlFileAccess

        function onFsFileReady() {
            readCurrentDoc("file://" + tmpFilePath)
            //loadProject("file://" + tmpFilePath)
        }
    }

    function htmlSave(fileName,sTempFile) {
        var tmpFilePath = "/tmp/temp.txt"
        if( sTempFile === null) {
            var ok = writeCurrentDoc("file://" + tmpFilePath)
        }
        else {
            tmpFilePath = sTempFile
        }
        //project.saveAs("file://" + tmpFilePath)
        var currentFileName = (fileName!==null ? fileName : "temp.txt")
        htmlFileAccess.saveFsFile(tmpFilePath,currentFileName)
    }
/*
    function saveOrSaveAs() {
        htmlSave(null);
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

    ListModel {
        id: buffersModel
        objectName: "buffersMenu"
        /* Example of items:
        ListElement {
            display: "hello"
            checkState: true
        }
        */
    }

    ListModel {
        id: lastOpenedFilesModel
        objectName: "lastOpenedFilesModel"
    }

    ListModel {
        id: languagesModel
        objectName: "languagesMenu"
    }

    ListModel {
        id: toolsModel
        objectName: "toolsMenu"
    }

    ListModel {
        id: importModel
        objectName: "importMenu"
    }

    function clearBuffersModel(model) {
        model.clear()
    }

    function findInModel(model, displayName) {
        for(var i=0; i<model.count; i++)
        {
            if(model.get(i)["display"]===displayName)
            {
                return true
            }
        }
        return false
    }

    function writeInMenuModel(model, index, name, checked, ashortcut) {
        if(model.count>index) {
            model.set(index, {"display":name, "checkState":checked, "shortcut":ashortcut, "index":index})
        }
        else {
            if(!findInModel(model,name)) {
                model.append({"display":name, "checkState":checked, "shortcut":ashortcut, "index":index})
            }
        }
    }

    function removeInMenuModel(model,index) {
        if(index < model.count) {
            model.remove(index)
        }
    }

    function setCheckStateInMenuModel(model,index, checked) {
        if(index < model.count) {
            model.setProperty(index,"checkState",checked)
        }
    }

    function handleMenuChecked(menuId, val) {
        switch(menuId) {
            case 450:  //IDM_MONOFONT
                sciteActions.actionUseMonospacedFont.checked = val
                break;
            case 411:  //IDM_VIEWSTATUSBAR
                sciteActions.actionShowStatusBar.checked = val
                break;
            case 410:  //IDM_VIEWTABBAR
                sciteActions.actionShowTabBar.checked = val
                break;
            case 409:  //IDM_TOGGLEOUTPUT
                sciteActions.actionToggleOutput.checked = val
                break;
            case 408:  //IDM_VIEWTOOLBAR
                sciteActions.actionShowToolBar.checked = val
                break;
            case 407:  //IDM_LINENUMBERMARGIN
                sciteActions.actionLineNumbers.checked = val
                break;
            case 406:  //IDM_FOLDMARGIN
                sciteActions.actionFoldMargin.checked = val
                break;
            case 405:  //IDM_SELMARGIN
                sciteActions.actionMargin.checked = val
                break;
            case 404:  //IDM_VIEWGUIDES
                sciteActions.actionIndentaionGuides.checked = val
                break;
            case 403:  //IDM_VIEWEOL
                sciteActions.actionShowEndOfLine.checked = val
                break;
            case 402:  //IDM_VIEWSPACE
                sciteActions.actionShowWhitespace.checked = val
                break;
            case 401:  //IDM_SPLITVERTICAL
                sciteActions.actionVerticalSplit.checked = val
                break;
            case 414:  //IDM_WRAP
                sciteActions.actionWrap.checked = val
                break;
            case 415:  //IDM_WRAPOUTPUT
                sciteActions.actionWrapOutput.checked = val
                break;
            case 416:  //IDM_READONLY
                sciteActions.actionReadOnly.checked = val
                readOnlyChanged(val)
                break;
            case 430:  //IDM_EOL_CRLF
                sciteActions.actionCrLf.checked = val
                break;
            case 431:  //IDM_EOL_CR
                sciteActions.actionCr.checked = val
                break;
            case 432:  //IDM_EOL_LF
                sciteActions.actionLf.checked = val
                break;
            default:
                //console.log("unhandled menu checked "+menuId)
        }
    }

    signal readOnlyChanged(bool value)
    signal runningChanged(bool value)
    signal buildChanged(bool value)
    signal copyCutChanged(bool value)
    signal pasteChanged(bool value)
    signal undoChanged(bool value)
    signal redoChanged(bool value)

    function handleMenuEnable(menuId, val) {
        switch(menuId) {
            case 201:   //IDM_UNDO
                sciteActions.actionUndo.enabled = val
                undoChanged(!val)
                break;
            case 202:   //IDM_REDO
                sciteActions.actionRedo.enabled = val
                redoChanged(!val)
                break;
            case 203:   //IDM_CUT
                sciteActions.actionCut.enabled = val
                copyCutChanged(!val)
                break;
            case 204:   //IDM_COPY
                sciteActions.actionCopy.enabled = val
                copyCutChanged(!val)
                break;
            case 205:   //IDM_PASTE
                sciteActions.actionPaste.enabled = val
                pasteChanged(!val)
                break;
            case 206:  //IDM_CLEAR
                sciteActions.actionDelete.enabled = val
                break;
            case 232:  //IDM_SHOWCALLTIP
                sciteActions.actionShowCalltip.enabled = val
                break;
            case 233:  //IDM_COMPLETE
                sciteActions.actionCompleteSymbol.enabled = val
                break;
            case 301:  //IDM_COMPILE
                sciteActions.actionCompile.enabled = val
                break;
            case 302:  //IDM_BUILD
                sciteActions.actionBuild.enabled = val
                buildChanged(!val)
                break;
            case 303:  //IDM_GO
                // Bug: prevents menu item from being closed after trigger...
                // see: https://bugreports.qt.io/browse/QTBUG-69682
                Qt.callLater(function() { sciteActions.actionGo.enabled = val })
                runningChanged(!val)
                break;
            case 304:  //IDM_STOPEXECUTE
                sciteActions.actionStopExecuting.enabled = val
                runningChanged(val)
                break;
            case 308:  //IDM_CLEAN
                sciteActions.actionClean.enabled = val
                break;
// TODO: 313, 311, 312 (for macros)
            case 413:  //IDM_OPENFILESHERE
                sciteActions.actionOpenFilesHere.enabled = val
                break;
            case 465:  //IDM_OPENDIRECTORYPROPERTIES
                sciteActions.actionOpenDirectoryOptionsFile.enabled = val
                break;
            default:
                //console.log("unhandled menu enable "+menuId)
        }
    }

    Connections {
        target: sciteQt

        function onSetMenuChecked(menuID, val) { handleMenuChecked(menuID, val) }
        function onSetMenuEnable(menuID, val)  { handleMenuEnable(menuID, val) }

        function onSetInBuffersModel(index, txt, checked, ashortcut)   { writeInMenuModel(buffersModel, index, txt, checked, ashortcut) }
        function onRemoveInBuffersModel(index)                         { removeInMenuModel(buffersModel, index) }
        function onCheckStateInBuffersModel(index, checked)            { setCheckStateInMenuModel(buffersModel, index, checked) }

        function onSetInLanguagesModel(index, txt, checked, ashortcut) { writeInMenuModel(languagesModel, index, txt, checked, ashortcut) }
        function onRemoveInLanguagesModel(index)                       { removeInMenuModel(languagesModel, index) }
        function onCheckStateInLanguagesModel(index, checked)          { setCheckStateInMenuModel(languagesModel, index, checked) }

        function onSetInToolsModel(index, txt, checked, ashortcut)     { writeInMenuModel(toolsModel, index, txt, checked, ashortcut) }
        function onRemoveInToolsModel(index)                           { removeInMenuModel(toolsModel, index) }
        function onCheckStateInToolsModel(index, checked)              { setCheckStateInMenuModel(toolsModel, index, checked) }

        function onSetInLastOpenedFilesModel(index, txt, checked, ashortcut) { writeInMenuModel(lastOpenedFilesModel, index, txt, checked, ashortcut) }
        function onRemoveInLastOpenedFilesModel(index)                       { removeInMenuModel(lastOpenedFilesModel, index) }
        function onCheckStateInLastOpenedFilesModel(index, checked)          { setCheckStateInMenuModel(lastOpenedFilesModel, index, checked) }

        function onSetInImportModel(index, txt, checked, ashortcut)    { writeInMenuModel(importModel, index, txt, checked, ashortcut) }
        function onRemoveInImportModel(index)                          { removeInMenuModel(importModel, index) }
        function onCheckStateInImportModel(index, checked)             { setCheckStateInMenuModel(importModel, index, checked) }

        function onAdmin(value)                                        { mobileFileDialog.setAdminModus(value) }
    }

    // desktop modus menu bar
    MenuBar {
        id: sciteMenuBar
        visible: !sciteQt.mobileUI

        FileMenu {
            id: fileMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
        EditMenu {
            id: editMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
        SearchMenu {
            id: searchMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
        ViewMenu {
            id: viewMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
        ToolsMenu {
            id: toolsMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
        OptionsMenu {
            id: optionsMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
        LanguageMenu {
            id: languageMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
        BuffersMenu {
            id: buffersMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
        HelpMenu {
            id: helpsMenuX
            actions: sciteActions
            useSimpleMenu: false //sciteQt.useSimpleMenus()
        }
    }

    Connections {
        target: applicationWindow

        function onReadOnlyChanged(value) {
            toolButtonReadonly.checked = value
        }
        function onRunningChanged(value) {
            toolButtonGo.enabled = !value
            toolButtonGo.checked = value
            toolButtonStop.enabled = value
            toolButtonStop.checked = !value
        }
        function onBuildChanged(value) {
            toolButtonBuild.enabled = !value
            toolButtonBuild.checked = value
        }
        function onCopyCutChanged(value) {
            toolButtonCopy.enabled = !value
            toolButtonCopy.checked = value
            toolButtonCut.enabled = !value
            toolButtonCut.checked = value
            toolButtonDelete.enabled = !value
            toolButtonDelete.checked = value
        }
        function onPasteChanged(value) {
            toolButtonPaste.enabled = !value
            toolButtonPaste.checked = value
        }
        function onUndoChanged(value) {
            toolButtonUndo.enabled = !value
            toolButtonUndo.checked = value
        }
        function onRedoChanged(value) {
            toolButtonRedo.enabled = !value
            toolButtonRedo.checked = value
        }
    }

    menuBar: sciteQt.mobileUI ? mobileMenuBar : sciteMenuBar

    header: toolBarButtonContainer

    ToolBar {
        id: mobileMenuBar
        contentHeight: 40
        //contentHeight: toolButton.implicitHeight
        visible: sciteQt.mobileUI

        ToolButton {
            id: toolButton
            //text: "\u2261"  //stackView.depth > 1 ? "\u25C0" : "\u2261"  // original: "\u2630" for second entry, does not work on Android
            icon.source:  stackView.depth > 1 ? "icons/arrow_left-black.svg" : "icons/menu-black.svg"
            icon.width: mobileMenuBar.contentHeight-12
            icon.height: mobileMenuBar.contentHeight-12
            //font.pixelSize: Qt.application.font.pixelSize * 1.6
            anchors.left: parent.left
            anchors.leftMargin: 5
            onClicked: {
                if (stackView.depth > 1) {
                    stackView.pop()
                } else {
                    drawer.open()
                }
            }
        }

        Label {
            id: labelTitel
            text: qsTr("SciteQt")
            //anchors.centerIn: parent
            anchors.left: toolButton.right
            anchors.right: menuButton.left
            anchors.leftMargin: 5
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
        }

        ToolButton {
            id: menuButton
            //text: "\u22EE"
            icon.source: "icons/more_vert-black.svg"
            icon.width: mobileMenuBar.contentHeight-12
            icon.height: mobileMenuBar.contentHeight-12
            //font.pixelSize: Qt.application.font.pixelSize * 1.6
            anchors.right: parent.right
            anchors.leftMargin: 5
            onClicked: mobilePopupMenu.open()

            // Popup menu for mobile platforms
            Menu {
                id: mobilePopupMenu
                y: menuButton.height
                visible: false

                FileMenu {
                    id: fileMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }
                EditMenu {
                    id: editMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }
                SearchMenu {
                    id: searchMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }
                ViewMenu {
                    id: viewMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }
                ToolsMenu {
                    id: toolsMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }
                OptionsMenu {
                    id: optionsMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }
                LanguageMenu {
                    id: languageMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }
                BuffersMenu {
                    id: buffersMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }
                HelpMenu {
                    id: helpsMenuXX
                    actions: sciteActions
                    useSimpleMenu: false //sciteQt.useSimpleMenus()
                }

            }
        }
    }

    // use Flow for creating a Toolbar
    ToolBar {
        id: toolBarButtonContainer
        visible: sciteQt.showToolBar
        //height: visible ? implicitHeight : 0
        height: visible ? flow.implicitHeight : 0

        property int iconWidth: iconSize //16  /* org: 24 */
        property int iconHeight: iconSize //16
/*
        onHeightChanged: {
            if( toolBarButtonContainer.height>2*(iconSize+12)+flow.spacing ) {
                // flow has to contain more than two icon rows -> try to make icons smaller to fit into two rows
                iconSize -= 2
            } else if ( toolBarButtonContainer.height<defaultIconSize ) {
                if( iconSize<defaultIconSize ) {
                    iconSize += 1
                }
            }
        }
*/
        Flow {
            id: flow
            anchors.fill: parent

            ToolButton {
                id: toolButtonNew
                icon.source: "icons/create.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                height: iconSize
            }
            ToolButton {
                id: toolButtonPrint
                icon.source: "icons/print.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
                //text: "Print"
                visible: sciteQt.showToolBar && !sciteQt.mobilePlatform
                onClicked: sciteQt.cmdPrint()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Print current document"))
            }
            ToolButton {
                id: toolButtonShare
                icon.source: "icons/share.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
                //text: "Share"
                visible: sciteQt.showToolBar && sciteQt.mobilePlatform
                onClicked: {
                    sciteQt.cmdShare()
                    focusToEditor()
                }

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Share current document"))
            }
            ToolSeparator {
                visible:  sciteQt.showTabBar
                height: iconSize
            }
            ToolButton {
                id: toolButtonCut
                icon.source: "icons/content_cut.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                height: iconSize
            }
            ToolButton {
                id: toolButtonUndo
                icon.source: "icons/undo.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                height: iconSize
            }
            ToolButton {
                id: toolButtonFind
                icon.source: "icons/find_in_page.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
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
                height: iconSize
            }
            ToolButton {
                id: toolButtonBuild
                icon.source: "icons/build.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
                //text: "Build"
                visible: sciteQt.showToolBar && !sciteQt.mobilePlatform
                onClicked: sciteQt.cmdBuild()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Trigger build"))
            }
            ToolButton {
                id: toolButtonGo
                icon.source: "icons/play_arrow.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
                //text: "Go"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdGo(settings.supportLevel)

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Run script"))
            }
            ToolButton {
                id: toolButtonStop
                icon.source: "icons/stop.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
                //text: "Stop"
                visible: sciteQt.showToolBar
                onClicked: sciteQt.cmdStopExecuting()

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Stop script execution"))
            }
            /* TODO: implement/improve Graphic output
            ToolButton {
                id: toolButtonGraphics
                icon.source: "icons/loop.svg"
                icon.height: toolBarButtonContainer.iconHeight
                icon.width: toolBarButtonContainer.iconWidth
                //text: "Graphics"
                visible: sciteQt.showToolBar
                onClicked: {
                    graphicsDialog.show()
                    console.log("graphics dlg: "+graphicsDialog.width+ " "+graphicsDialog.height)
                }

                ToolTip.delay: toolTipDelay
                ToolTip.timeout: toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: sciteQt.getLocalisedText(qsTr("Show graphics output dialog"))
            }
            */
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

    Drawer {
        id: drawer
        width: applicationWindow.width * 0.66
        height: applicationWindow.height

        Column {
            anchors.fill: parent

            ItemDelegate {
                text: sciteQt.getLocalisedText(qsTr("Go to"))
                width: parent.width
                onClicked: {
                    stackView.pop()
                    //stackView.push(gotoDialog)
                    sciteQt.cmdGoto()
                    drawer.close()
                }
            }
            ItemDelegate {
                text: sciteQt.getLocalisedText(qsTr("Find"))
                width: parent.width
                onClicked: {
                    stackView.pop()
                    //stackView.push(findDialog)
                    sciteQt.cmdFind()
                    drawer.close()
                }
            }
            ItemDelegate {
                text: sciteQt.getLocalisedText(qsTr("Replace"))
                width: parent.width
                onClicked: {
                    stackView.pop()
                    //stackView.push(replaceDialog)
                    sciteQt.cmdReplace()
                    drawer.close()
                }
            }
            ItemDelegate {
                text: sciteQt.getLocalisedText(qsTr("Find in Files"))
                width: parent.width
                onClicked: {
                    stackView.pop()
                    //stackView.push(findInFilesDialog)
                    sciteQt.cmdFindInFiles()
                    drawer.close()
                }
            }
            ItemDelegate {
                text: sciteQt.getLocalisedText(qsTr("Parameters"))
                width: parent.width
                onClicked: {                    
                    stackView.pop()
                    //stackView.push(parametersDialog)
                    sciteQt.cmdParameters()
                    drawer.close()
                }
            }
            ItemDelegate {
                text: sciteQt.getLocalisedText(qsTr("Run as JavaScript"))
                width: parent.width
                onClicked: {
                    stackView.pop()
                    sciteQt.cmdRunCurrentAsJavaScriptFile()
                    drawer.close()
                }
            }
            ItemDelegate {
                text: sciteQt.getLocalisedText(qsTr("Run as Lua"))
                width: parent.width
                onClicked: {
                    stackView.pop()
                    sciteQt.cmdRunCurrentAsLuaFile()
                    drawer.close()
                }
            }
            ItemDelegate {
                id: clearOutputTool
                text: sciteQt.getLocalisedText(qsTr("Clear Output"))
                width: parent.width
                onClicked: {
                    stackView.pop()
                    sciteQt.cmdClearOutput()
                    drawer.close()
                }
            }
            ItemDelegate {
                text: sciteQt.getLocalisedText(qsTr("Support SciteQt"))
                width: parent.width
                visible: sciteQt.mobilePlatform
                height: visible ? clearOutputTool.heigh : 0
                onClicked: {
                    stackView.pop()
                    sciteQt.cmdSupportSciteQt()
                    drawer.close()
                }
            }
        }
    }

    StackView {
        id: stackView
        initialItem: centralWidget
        anchors.fill: parent
        //width: parent.width
        //height: parent.height
        /*
        anchors.top: tabBar.bottom
        anchors.right: parent.right
        anchors.bottom: findInput.top
        anchors.left: parent.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        */
    }

    property bool isIncrementalSearch: false
    property bool isFilterSearch: false
    property bool isCloseOnFind: true
    property int stripAreaMargin: findInput.visible ? 5 : 0

    Item {
        id: centralWidget
        anchors.fill: parent
        visible: true

    TabBar {
        id: tabBar

        visible: sciteQt.showTabBar
        height: sciteQt.showTabBar ? implicitHeight : 0
        //focusPolicy: Qt.NoFocus

        anchors.top: parent.top
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
                    // activate this tab and show context menu
                    parent.onClicked()

                    //fill menu in c++, see: SciTEWinBar.Notify() NM_RCLICK
                    var menuItems = sciteQt.fillTabContextMenu()

                    // update menu model with data from c++
                    sciteContextMenuModel.clear()
                    for (var i=0; i<menuItems.length; i++) {
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

            onObjectAdded: (index, object) => { tabBarContextMenu.insertItem(index, object) }
            onObjectRemoved: (object) => { tabBarContextMenu.removeItem(object) }
        }
    }

    ListModel {
        id: sciteContextMenuModel
        objectName: "sciteContextMenu"
    }

    /*Controls1.*/SplitView {
        id: splitView        
        objectName: "SplitView"
        visible: true

        //resizing: true

        orientation: verticalSplit ? Qt.Horizontal : Qt.Vertical

        property int outputHeight: 0
        property bool verticalSplit: false

        //anchors.fill: parent

        anchors.top: tabBar.bottom
        anchors.right: parent.right
        anchors.bottom: findInput.top
        anchors.left: parent.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        handle: Rectangle {
            id: handleDelegate
            implicitWidth: 4
            implicitHeight: 4
            color: SplitHandle.pressed ? "#888888" : (SplitHandle.hovered ? "#bbbbbb" : "#000000")
        }

/* TODO: improve splitter...

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
            visible: true

            fcnLocalisation: sciteQt.getLocalisedText

            focus: true

            SplitView.fillWidth: true
            SplitView.fillHeight: true
            //Layout.fillWidth: true
            //Layout.fillHeight: true

            //menuCommandDelegate: sciteQt.cmdContextMenu

            //text: "editor area !"
        }

        ScintillaText {
            id: quickScintillaOutput
            objectName: "ScintillaOutput"

            fcnLocalisation: sciteQt.getLocalisedText

            focus: false

            SplitView.preferredWidth: splitView.outputHeight
            SplitView.preferredHeight: splitView.outputHeight
            //width: splitView.outputHeight               // user draging of splitter will brake the binding !!!
            //height: splitView.outputHeight
            //implicitWidth: splitView.outputHeight
            //implicitHeight: splitView.outputHeight

            //menuCommandDelegate: sciteQt.cmdContextMenu

            //text: "output area !"
        }
    }       

    // Strip Find Dialog above status bar:
    //====================================

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

        onAccepted: {
            sciteQt.setFindText(getCurrentFindText(), isIncrementalSearch, isFilterSearch)
            if(isCloseOnFind) {     // TODO: handling of close after find is not correct !
                hideFindRow()
            }
        }
        onEditTextChanged: {
// TODO gulp -> handle new Filter() function !!! -> FilterStrip::Filter
            if( isIncrementalSearch ) {
                sciteQt.setFindText(editText, isIncrementalSearch, isFilterSearch)
            }
        }

        Keys.onEscapePressed: hideFindRow()
        Keys.onBackPressed: hideFindRow()
    }

    ListModel {
        id: stripFindWhatModel
        objectName: "stripFindWhatModel"
    }

    Button {
        id: findNextButton

        visible: findInput.visible
        //focusPolicy: Qt.NoFocus

        anchors.bottom: replaceInput.top
        anchors.right: findMarkAllButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

        text: sciteQt.getLocalisedText(qsTr("&Find Next"),false)
        onClicked: {
            sciteQt.setFindText(getCurrentFindText(), isIncrementalSearch, isFilterSearch)
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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

        text: sciteQt.getLocalisedText(qsTr("Mark &All"),false)
        onClicked: {
            sciteQt.setFindText(getCurrentFindText(), isIncrementalSearch, isFilterSearch)
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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

        onAccepted: {
            sciteQt.setFindText(getCurrentFindText(), isIncrementalSearch, isFilterSearch)
            //hideFindRow()
        }

        Keys.onEscapePressed: hideFindRow()
        Keys.onBackPressed: hideFindRow()
    }

    ListModel {
        id: stripReplaceWithModel
        objectName: "stripReplaceWithModel"
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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin

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
        anchors.topMargin: stripAreaMargin
        anchors.bottomMargin: stripAreaMargin
    }

    }   // Item


    Settings {
        id: settings

        //property var splitView

        property int supportLevel: -1   // no support level at all
    }


    SciTEQt {
       id: sciteQt
    }

    Connections {
        target: sciteQt

        function onTriggerUpdateCurrentWindowPosAndSize() { updateCurrentWindowPosAndSize() }
        function onSetWindowPosAndSize(left, top, width, height, maximize) { setWindowPosAndSize(left, top, width, height, maximize) }
        function onSetTextToCurrent(text) { setTextToCurrent(text) }
        function onAddTextToOutput(text)  { addTextToOutput(text) }

        function onStartFileDialog(sDirectory, sFilter, sTitle, bAsOpenDialog, bSaveACopyModus, bDeleteModus, sDefaultSaveAsName) { startFileDialog(sDirectory, sFilter, sTitle, bAsOpenDialog, bSaveACopyModus, bDeleteModus, sDefaultSaveAsName) }
        function onShowInfoDialog(sInfoText, style) { showInfoDialog(sInfoText, style) }
        function onShowAboutSciteDialog() { showAboutSciteDialog() }
        function onShowSupportSciteQtDialog() { showSupportSciteQtDialog() }

        function onShowFindInFilesDialog(text, findHistory, filePatternHistory, directoryHistory, wholeWord, caseSensitive, regularExpression) { showFindInFilesDialog(text, findHistory, filePatternHistory, directoryHistory, wholeWord, caseSensitive, regularExpression) }
        function onShowFindStrip(findHistory, replaceHistory, text, incremental, withReplace, closeOnFind, isFilter) { showFindStrip(findHistory, replaceHistory, text, incremental, withReplace, closeOnFind, isFilter) }
        function onShowFind(findHistory, text, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down) { showFind(findHistory, text, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down) }
        function onShowReplace(findHistory, replaceHistory, text, replace, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down) { showReplace(findHistory, replaceHistory, text, replace, wholeWord, caseSensitive, regExpr, wrap, transformBackslash, down) }
        function onShowGoToDialog(currentLine, currentColumn, maxLine) { showGoToDialog(currentLine, currentColumn, maxLine) }
        function onShowTabSizeDialog(tabSize, indentSize, useTabs) { showTabSizeDialog(tabSize, indentSize, useTabs) }
        function onShowAbbreviationDialog(items) { showAbbreviationDialog(items) }
        function onShowParametersDialog(modal, parameters) { showParametersDialog(modal, parameters) }
        function onCloseFindReplaceDialog() { closeFindReplaceDialog() }

        function onSetVerticalSplit(verticalSplit) { setVerticalSplit(verticalSplit) }
        function onSetOutputHeight(heightOutput) { setOutputHeight(heightOutput) }

        function onInsertTab(index, title, fullPath) { insertTab(index, title, fullPath) }
        function onSelectTab(index) { selectTab(index) }
        function onRemoveAllTabs() { removeAllTabs() }

        function onSaveCurrentForWasm() { htmlSave(fileName,sTempFile) }

        function onDismissMenu() { dismissMenu() }
    }

    // **********************************************************************

   /*Labs.*/FileDialog {
        id: fileDialog
        objectName: "fileDialog"
        visible: false
        modality: Qt.ApplicationModal
        fileMode: openMode ? FileDialog.OpenFile : FileDialog.SaveFile
        title: openMode ? qsTr("Choose a file") : qsTr("Save as")
        //folder: "."

        property bool openMode: true

        //selectExisting: openMode ? true : false
        //selectMultiple: false
        //selectFolder: false

        onAccepted: {
            if(sciteQt.isWebassemblyPlatform()) {
                if(!fileDialog.openMode) {
                    writeCurrentDoc(fileDialog.currentFile)
                }
                else {
                    //Android: quickScintillaEditor.text = fileUrl
                    readCurrentDoc(fileDialog.currentFile)
                }
            }
            else {
                sciteQt.updateCurrentSelectedFileUrl(fileDialog.currentFile)
            }
            fileDialog.close()
            focusToEditor()
        }
        onRejected: {
            fileDialog.close()
            focusToEditor()
        }
    }

    // needed for MacOS and Qt 5.11.3
    Labs.FileDialog {
        id: labsFileDialog
        objectName: "labsFileDialog"
        visible: false

        onAccepted: {
            sciteQt.updateCurrentSelectedFileUrl(file)
            labsFileDialog.close()
            focusToEditor()
        }
        onRejected: {
            labsFileDialog.close()
            focusToEditor()
        }
    }

    MobileFileDialog {
        id: mobileFileDialog
        objectName: "mobileFileDialog"
        visible: false

        fcnLocalisation: sciteQt.getLocalisedText
    }

    Connections {
        target: mobileFileDialog

        function onOpenSelectedFile() {
            //readCurrentDoc(fileName)
            sciteQt.updateCurrentSelectedFileUrl(buildValidUrl(fileName))
        }
        function onSaveSelectedFile() {
            //writeCurrentDoc(buildValidUrl(fileName))
            sciteQt.updateCurrentSelectedFileUrl(buildValidUrl(fileName))
        }

        function onRejected() {
            stackView.pop()
            focusToEditor()
        }
        function onAccepted() {
            stackView.pop()
            focusToEditor()
        }

        function onErrorMessage() {
            logToOutput(message)
            focusToEditor()
        }
    }

    Labs.MessageDialog {
        id: infoDialog
        objectName: "infoDialog"
        visible: false
        title: sciteQt.getLocalisedText(qsTr("Info"))
        //modal: true
        modality: Qt.ApplicationModal
        buttons: Labs.MessageDialog.Ok
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
/*
    Connections {
        target: infoDialog

        onYes: infoDialog.close()
        onNo:  infoDialog.close()
        onCanceled: infoDialog.close()
        onRejected: infoDialog.close()
        onAccepted: infoDialog.close()
    }
*/
    // Page based InfoDialog for WASM platform only
    Page {
        id: infoDialogPage
        objectName: "infoDialogPage"
        visible: false
        title: sciteQt.getLocalisedText(qsTr("Info"))

        signal accepted()
        signal rejected()
        signal canceled()
        signal yes()
        signal no()

        //property var standardButtons: Labs.MessageDialog.Ok
        property string text: ""

        Label {
            id: label
            text: infoDialogPage.text
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 10
            anchors.leftMargin: 10
            anchors.topMargin: 10
            anchors.bottomMargin: 10
        }

        Row {
            anchors.top: label.bottom
            anchors.rightMargin: 5
            anchors.leftMargin: 5
            anchors.topMargin: 5
            anchors.bottomMargin: 5

            padding: 10
            spacing: 10

            Button {
                id: buttonYes
                text: sciteQt.getLocalisedText(qsTr("Yes"))
                visible: (infoDialogPage.buttons & Labs.MessageDialog.Yes) === Labs.MessageDialog.Yes
                Keys.onEscapePressed: buttonCancel.clicked()
                Keys.onBackPressed: buttonCancel.clicked()

                onClicked: infoDialogPage.yes()
            }
            Button {
                id: buttonNo
                text: sciteQt.getLocalisedText(qsTr("No"))
                visible: (infoDialogPage.buttons & Labs.MessageDialog.No) === Labs.MessageDialog.No
                Keys.onEscapePressed: buttonCancel.clicked()
                Keys.onBackPressed: buttonCancel.clicked()

                onClicked: infoDialogPage.no()
            }
            Button {
                id: buttonOk
                text: sciteQt.getLocalisedText(qsTr("Ok"))
                visible: (infoDialogPage.buttons & Labs.MessageDialog.Ok) === Labs.MessageDialog.Ok
                highlighted: !sciteQt.mobilePlatform
                Keys.onEscapePressed: buttonCancel.clicked()
                Keys.onBackPressed: buttonCancel.clicked()

                onClicked: infoDialogPage.accepted()
            }
            Button {
                id: buttonCancel
                text: sciteQt.getLocalisedText(qsTr("Cancel"))
                visible: (infoDialogPage.buttons & Labs.MessageDialog.Cancel) === Labs.MessageDialog.Cancel
                highlighted: !sciteQt.mobilePlatform
                Keys.onEscapePressed: buttonCancel.clicked()
                Keys.onBackPressed: buttonCancel.clicked()

                onClicked: infoDialogPage.canceled()
            }
        }
    }

    Connections {
        target: infoDialogPage

        function onYes() {
            stackView.pop()
            focusToEditor()
        }

        function onNo() {
            stackView.pop()
            focusToEditor()
        }

        function onAccepted() {
            stackView.pop()
            focusToEditor()
        }

        function onCanceled() {
            stackView.pop()
            focusToEditor()
        }
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

    Window {
        id: graphicsDialog
        visible: false

        width: 400
        height: 400

        Canvas {
            id: canvas

            anchors.top: parent.top
            anchors.bottom: buttons.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.leftMargin: 5
            anchors.topMargin: 5
            anchors.bottomMargin: 5

            /*
              setPenWidth
              setPenColor
              setFillColor
              addLine
              addRect
              addEllipse
              addPolygon
              addText
              moveTo

              --> set QVariantArray with List of Paint Items which could be painted in onPaint
            */

            onPaint: {
                    var ctx = getContext("2d");
                    ctx.fillStyle = Qt.rgba(1, 0, 0, 1);
                    ctx.fillRect(0, 0, width, height);
                }
        }

        Row {
            id: buttons

            //anchors.top: canvas.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.leftMargin: 5
            anchors.topMargin: 5
            anchors.bottomMargin: 5

            spacing: 10

            Button {
                text: sciteQt.getLocalisedText(qsTr("Stop"))
                onClicked: {
                }
            }
            Button {
                text: sciteQt.getLocalisedText(qsTr("Pause"))
                onClicked: {
                }
            }
            Button {
                text: sciteQt.getLocalisedText(qsTr("Close"))
                onClicked: {
                    graphicsDialog.close()
                }
            }
        }
    }

    SupportDialog {
        id: supportDialog
        objectName: "supportDialog"
        visible: false
        title: sciteQt.getLocalisedText(qsTr("Support SciteQt"))

        fcnLocalisation: sciteQt.getLocalisedText
    }

    Connections {
        target: supportDialog

        function onClosed() {
            stackView.pop()
            focusToEditor()
        }
    }

    SupportDialogWindow {
        id: supportDialogWin
        objectName: "supportDialogWin"
        visible: false
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Support SciteQt"))

        width: 500
        height: 500

        fcnLocalisation: sciteQt.getLocalisedText
    }

    Connections {
        target: supportDialogWin

        function onClosed() {
            supportDialogWin.close()
            focusToEditor()
        }
    }

    Loader
    {
        id: storeLoader
        source: isAppStoreSupported ? "ApplicationStore.qml" : ""
    }

    AboutSciteDialog {
        id: aboutSciteDialog
        objectName: "aboutSciteDialog"
        visible: false
        title: sciteQt.getLocalisedText(qsTr("About SciTE"))

        fcnLocalisation: sciteQt.getLocalisedText
    }

    Connections {
        target: aboutSciteDialog

        function onClosed() {
            stackView.pop()
            focusToEditor()
        }
    }

    AboutSciteDialogWindow {
        id: aboutSciteDialogWin
        objectName: "aboutSciteDialogWin"
        visible: false
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("About SciTE"))

        fcnLocalisation: sciteQt.getLocalisedText
    }

    Connections {
        target: aboutSciteDialogWin

        function onClosed() {
            aboutSciteDialogWin.close()
            focusToEditor()
        }
    }

    FindDialog {
        id: findDialog
        objectName: "findDialog"
        title: sciteQt.getLocalisedText(qsTr("Find"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: findDialog

        function onCanceled() {
            stackView.pop()
            focusToEditor()
        }

        function onAccepted() { doExecuteFind(findDialog, false, true) }

        function onMarkAll()  { doExecuteFind(findDialog, true, true) }
    }

    FindDialogWindow {
        id: findDialogWin
        objectName: "findDialogWin"
        modality: sciteQt.mobilePlatform || sciteQt.isWebassemblyPlatform() ? Qt.ApplicationModal : Qt.NonModal
        title: sciteQt.getLocalisedText(qsTr("Find"))

        width: grid.implicitWidth+10
        height: grid.implicitHeight+10

        // Window is not resizable !
        minimumHeight: grid.implicitHeight+10
        minimumWidth: grid.implicitWidth+10

//        maximumHeight: grid.implicitWidth+10
//        minimumHeight: grid.implicitWidth+10 //height

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: findDialogWin

        function onCanceled() { findDialogWin.close() }

        function onAccepted() { doExecuteFind(findDialogWin, false, true) }

        function onMarkAll() {  doExecuteFind(findDialogWin, true, true) }
    }

    ReplaceDialog {
        id: replaceDialog
        objectName: "replaceDialog"
        title: sciteQt.getLocalisedText(qsTr("Replace"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: replaceDialog

        function onCanceled() {         stackView.pop() }
        function onAccepted() {         doExecuteFind(replaceDialog, false, false) }

        function onReplace() {          doExecuteReplace(replaceDialog,false,false) }
        function onReplaceAll() {       doExecuteReplace(replaceDialog,true,false) }
        function onReplaceInSection() { doExecuteReplace(replaceDialog,false,true) }
    }

    ReplaceDialogWindow {
        id: replaceDialogWin
        objectName: "replaceDialogWin"
        modality: sciteQt.mobilePlatform || sciteQt.isWebassemblyPlatform() ? Qt.ApplicationModal : Qt.NonModal
        title: sciteQt.getLocalisedText(qsTr("Replace"))

        width: grid.implicitWidth+10
        height: grid.implicitHeight+10

        // Window is not resizable !
        minimumHeight: grid.implicitHeight+10
        minimumWidth: grid.implicitWidth+10

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: replaceDialogWin

        function onCanceled()         { replaceDialogWin.close() }
        function onAccepted()         { doExecuteFind(replaceDialogWin, false, false) }

        function onReplace()          { doExecuteReplace(replaceDialogWin,false,false) }
        function onReplaceAll()       { doExecuteReplace(replaceDialogWin,true,false) }
        function onReplaceInSection() { doExecuteReplace(replaceDialogWin,false,true) }
    }

    Connections {
        target: sciteQt

        function onUpdateReplacementCount() {
            replaceDialog.countReplacementsLabel.text = count
        }
    }

    FindInFilesDialog {
        id: findInFilesDialog
        objectName: "findInFilesDialog"
        title: sciteQt.getLocalisedText(qsTr("Find in Files"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: findInFilesDialog

        function onCanceled() {
            stackView.pop()
            focusToEditor()
        }
        function onAccepted() {
            stackView.pop()
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

    FindInFilesDialogWindow {
        id: findInFilesDialogWin
        objectName: "findInFilesDialogWin"
        modality: sciteQt.mobilePlatform || sciteQt.isWebassemblyPlatform() ? Qt.ApplicationModal : Qt.NonModal
        title: sciteQt.getLocalisedText(qsTr("Find in Files"))

        width: grid.implicitWidth+10
        height: grid.implicitHeight+10

        // Window is not resizable !
        minimumHeight: grid.implicitHeight+10
        minimumWidth: grid.implicitWidth+10

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: findInFilesDialogWin

        function onCanceled() {
            findInFilesDialogWin.close()
            focusToEditor()
        }
        function onAccepted() {
            findInFilesDialogWin.close()
            var findWhatInput = findInFilesDialogWin.findWhatInput.editText.length > 0 ? findInFilesDialogWin.findWhatInput.editText : findInFilesDialogWin.findWhatInput.currentText
            var filesExtensionsInput = findInFilesDialogWin.filesExtensionsInput.editText.length > 0 ? findInFilesDialogWin.filesExtensionsInput.editText : findInFilesDialogWin.filesExtensionsInput.currentText
            var directoryInput = findInFilesDialogWin.directoryInput.editText.length > 0 ? findInFilesDialogWin.directoryInput.editText : findInFilesDialogWin.directoryInput.currentText
            var wholeWord = findInFilesDialogWin.wholeWordCheckBox.checked
            var caseSensitive = findInFilesDialogWin.caseSensitiveCheckBox.checked
            var regularExpression = findInFilesDialogWin.regularExpressionCheckBox.checked
            sciteQt.cmdStartFindInFilesAsync(directoryInput, filesExtensionsInput, findWhatInput, wholeWord, caseSensitive, regularExpression)
            focusToEditor()
        }
    }

    // for mobile ui (as page)
    GoToDialog {
        id: gotoDialog
        objectName: "gotoDialog"
        title: sciteQt.getLocalisedText(qsTr("Go To"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: gotoDialog

        function onCanceled() {
            stackView.pop()
            focusToEditor()
        }
        function onAccepted() {
            stackView.pop()
            sciteQt.cmdGotoLine(parseInt(gotoDialog.destinationLineInput.text), parseInt(gotoDialog.columnInput.text))
            focusToEditor()
        }
    }

    // for desktop ui
    GoToDialogWindow {
        id: gotoDialogWin
        objectName: "gotoDialogWin"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Go To"))

        width: grid.implicitWidth+10
        height: grid.implicitHeight+10

        minimumHeight: grid.implicitHeight+10
        minimumWidth: grid.implicitWidth+10

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: gotoDialogWin

        function onCanceled() {
            gotoDialogWin.close()
            focusToEditor()
        }
        function onAccepted() {
            gotoDialogWin.close()
            sciteQt.cmdGotoLine(parseInt(gotoDialogWin.destinationLineInput.text), parseInt(gotoDialogWin.columnInput.text))
            focusToEditor()
        }
    }

    TabSizeDialog {
        id: tabSizeDialog
        objectName: "tabSizeDialog"
        title: sciteQt.getLocalisedText(qsTr("Indentation Settings"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: tabSizeDialog

        function onCanceled() {
            stackView.pop()
            focusToEditor()
        }
        function onAccepted() {
            stackView.pop()
            sciteQt.cmdUpdateTabSizeValues(parseInt(tabSizeDialog.tabSizeInput.text), parseInt(tabSizeDialog.indentSizeInput.text), tabSizeDialog.useTabsCheckBox.checked, false)
            focusToEditor()
        }
        function onConvert() {
            stackView.pop()
            sciteQt.cmdUpdateTabSizeValues(parseInt(tabSizeDialog.tabSizeInput.text), parseInt(tabSizeDialog.indentSizeInput.text), tabSizeDialog.useTabsCheckBox.checked, true)
            focusToEditor()
        }
    }

    TabSizeDialogWindow {
        id: tabSizeDialogWin
        objectName: "tabSizeDialogWin"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Indentation Settings"))

        width: grid.implicitWidth+10
        height: grid.implicitHeight+10

        // Window is not resizable !
        minimumHeight: grid.implicitHeight+10
        minimumWidth: grid.implicitWidth+10

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: tabSizeDialogWin

        function onCanceled() {
            tabSizeDialogWin.close()
            focusToEditor()
        }
        function onAccepted() {
            tabSizeDialogWin.close()
            sciteQt.cmdUpdateTabSizeValues(parseInt(tabSizeDialogWin.tabSizeInput.text), parseInt(tabSizeDialogWin.indentSizeInput.text), tabSizeDialogWin.useTabsCheckBox.checked, false)
            focusToEditor()
        }
        function onConvert() {
            tabSizeDialogWin.close()
            sciteQt.cmdUpdateTabSizeValues(parseInt(tabSizeDialogWin.tabSizeInput.text), parseInt(tabSizeDialogWin.indentSizeInput.text), tabSizeDialogWin.useTabsCheckBox.checked, true)
            focusToEditor()
        }
    }

    AbbreviationDialog {
        id: abbreviationDialog
        objectName: "abbreviationDialog"
        title: sciteQt.getLocalisedText(qsTr("Insert Abbreviation"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: abbreviationDialog

        function onCanceled() {
            stackView.pop()
            focusToEditor()
        }
        function onAccepted() {
            stackView.pop()
            sciteQt.cmdSetAbbreviationText(abbreviationDialog.abbreviationInput.currentText)
            focusToEditor()
        }
    }

    AbbreviationDialogWindow {
        id: abbreviationDialogWin
        objectName: "abbreviationDialogWin"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Insert Abbreviation"))

        width: grid.implicitWidth+10
        height: grid.implicitHeight+10

        // Window is not resizable !
        minimumHeight: grid.implicitHeight+10
        minimumWidth: grid.implicitWidth+10

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: abbreviationDialogWin

        function onCanceled() {
            abbreviationDialogWin.close()
            focusToEditor()
        }
        function onAccepted() {
            abbreviationDialogWin.close()
            sciteQt.cmdSetAbbreviationText(abbreviationDialogWin.abbreviationInput.currentText)
            focusToEditor()
        }
    }

    ParametersDialog {
        id: parametersDialog
        objectName: "parametersDialog"
        title: sciteQt.getLocalisedText(qsTr("Parameters"))

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: parametersDialog

        function onCanceled() {
            stackView.pop()
            focusToEditor()
            sciteQt.cmdParametersDialogClosed()
        }
        function onAccepted() {
            stackView.pop()
            sciteQt.cmdSetParameters(parametersDialog.cmdInput.text, parametersDialog.parameter1Input.text, parametersDialog.parameter2Input.text, parametersDialog.parameter3Input.text, parametersDialog.parameter4Input.text)
            sciteQt.cmdParametersDialogClosed()
            focusToEditor()
        }
    }

    ParametersDialogWindow {
        id: parametersDialogWin
        objectName: "parametersDialogWin"
        modality: Qt.ApplicationModal
        title: sciteQt.getLocalisedText(qsTr("Parameters"))

        width: grid.implicitWidth+10/*+50*/
        height: grid.implicitHeight+10

        // Window is not resizable !
        minimumHeight: grid.implicitHeight+10
        minimumWidth: grid.implicitWidth+10

        fcnLocalisation: sciteQt.getLocalisedText

        visible: false
    }

    Connections {
        target: parametersDialogWin

        function onCanceled() {
            parametersDialogWin.close()
            focusToEditor()
            sciteQt.cmdParametersDialogClosed()
        }
        function onAccepted() {
            parametersDialogWin.close()
            sciteQt.cmdSetParameters(parametersDialogWin.cmdInput.text, parametersDialogWin.parameter1Input.text, parametersDialogWin.parameter2Input.text, parametersDialogWin.parameter3Input.text, parametersDialogWin.parameter4Input.text)
            sciteQt.cmdParametersDialogClosed()
            focusToEditor()
        }
    }

    Connections {
        target: storageAccess

        function onOpenFileContentReceived(fileUri, decodedFileUri, content) {    // fileUri, decodedFileUri, content
            sciteQt.OnAddFileContent(fileUri, decodedFileUri, content, false, mobileFileDialog.isSaveACopyModus)
            mobileFileDialog.rejected()   // because loading and showing loaded document is already processed here (in QML)
        }
        function onOpenFileCanceled() {
            mobileFileDialog.rejected()
        }
        function onOpenFileError() {
            mobileFileDialog.rejected()
        }
        function onCreateFileReceived(fileUri, decodedFileUri) {  // fileUri, decodedFileUri
            sciteQt.OnAddFileContent(fileUri, decodedFileUri, "<create file received data>", true, mobileFileDialog.isSaveACopyModus)
            mobileFileDialog.rejected()
        }
    }
}
