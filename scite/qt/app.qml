import QtQuick 2.9
import QtQuick.Controls 2.14
import QtQuick.Dialogs 1.2
import QtQml.Models 2.14
import Qt.labs.platform 1.1 as Platform
import Qt.labs.settings 1.0

import org.scintilla.sciteqt 1.0

ApplicationWindow {
    id: applicationWindow
    width: 800
    height: 600
    visible: true

    property string urlPrefix: "file://"

    onClosing: {
        sciteQt.cmdExit()
        close.accepted = false
    }

    Component.onCompleted: {
        console.log("ON Completed")
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
        console.log("FONT: "+menuText+" "+ (menuItem!==undefined ? menuItem.font : "?")+" action="+menuItem.action) //+" shortcut="+menuItem.action.shortcut+ " "+menuItem.parent)
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

    menuBar: MenuBar {
        id: menuBar

        AutoSizingMenu {
            id: fileMenu
            title: processMenuItem(qsTr("&File"),null)
// see: https://forum.qt.io/topic/104010/menubar-display-shortcut-in-quickcontrols2/4
            MenuItem {
                id: actionNew
                text: processMenuItem2(sciteActions.actionNew.text, actionNew)
                action: sciteActions.actionNew
            }
            MenuItem {
                id: actionOpen
                text: processMenuItem2(sciteActions.actionOpen.text, actionOpen)
                action: sciteActions.actionOpen
            }
            MenuItem {
                id: actionOpenSelectedFilename
                text: processMenuItem2(sciteActions.actionOpenSelectedFilename.text, actionOpenSelectedFilename)
                action: sciteActions.actionOpenSelectedFilename
            }
            MenuItem {
                id: actionRevert
                text: processMenuItem2(sciteActions.actionRevert.text, actionRevert)
                action: sciteActions.actionRevert
            }
            MenuItem {
                id: actionClose
                text: processMenuItem2(sciteActions.actionClose.text, actionClose)
                action: sciteActions.actionClose
            }
            MenuItem {
                id: actionSave
                text: processMenuItem2(sciteActions.actionSave.text, actionSave)
                action: sciteActions.actionSave
            }
            MenuItem {
                id: actionSaveAs
                text: processMenuItem2(sciteActions.actionSaveAs.text, actionSaveAs)
                action: sciteActions.actionSaveAs
            }
            MenuItem {
                id: actionCopyPath
                text: processMenuItem2(sciteActions.actionCopyPath.text, actionCopyPath)
                action: sciteActions.actionCopyPath
            }
            Menu {
                id: actionEncoding
                title: processMenuItem(qsTr("Encodin&g"), actionEncoding)

                MenuItem {
                    id: actionCodePageProperty
                    text: processMenuItem2(sciteActions.actionCodePageProperty.text, actionCodePageProperty)
                    action: sciteActions.actionCodePageProperty
                }
                MenuItem {
                    id: actionUtf16BigEndian
                    text: processMenuItem2(sciteActions.actionUtf16BigEndian.text, actionUtf16BigEndian)
                    action: sciteActions.actionUtf16BigEndian
                }
                MenuItem {
                    id: actionUtf16LittleEndian
                    text: processMenuItem2(sciteActions.actionUtf16LittleEndian.text, actionUtf16LittleEndian)
                    action: sciteActions.actionUtf16LittleEndian
                }
                MenuItem {
                    id: actionUtf8WithBOM
                    text: processMenuItem2(sciteActions.actionUtf8WithBOM.text, actionUtf8WithBOM)
                    action: sciteActions.actionUtf8WithBOM
                }
                MenuItem {
                    id: actionUtf8
                    text: processMenuItem2(sciteActions.actionUtf8.text, actionUtf8)
                    action: sciteActions.actionUtf8
                }
            }
            Menu {
                id: actionExport
                title: processMenuItem(qsTr("&Export"), actionExport)

                MenuItem {
                    id: actionAsHtml
                    text: processMenuItem2(sciteActions.actionAsHtml.text, actionAsHtml)
                    action: sciteActions.actionAsHtml
                }
                MenuItem {
                    id: actionAsRtf
                    text: processMenuItem2(sciteActions.actionAsRtf.text, actionAsRtf)
                    action: sciteActions.actionAsRtf
                }
                MenuItem {
                    id: actionAsPdf
                    text: processMenuItem2(sciteActions.actionAsPdf.text, actionAsPdf)
                    action: sciteActions.actionAsPdf
                }
                MenuItem {
                    id: actionAsLatex
                    text: processMenuItem2(sciteActions.actionAsLatex.text, actionAsLatex)
                    action: sciteActions.actionAsLatex
                }
                MenuItem {
                    id: actionAsXml
                    text: processMenuItem2(sciteActions.actionAsXml.text, actionAsXml)
                    action: sciteActions.actionAsXml
                }
            }
            MenuSeparator {}
            MenuItem {
                id: actionPageSetup
                text: processMenuItem2(sciteActions.actionPageSetup.text, actionPageSetup)
                action: sciteActions.actionPageSetup
            }
            MenuItem {
                id: actionPrint
                text: processMenuItem2(sciteActions.actionPrint.text, actionPrint)
                action: sciteActions.actionPrint
            }
            MenuSeparator {}
            MenuItem {
                id: actionLoadSession
                text: processMenuItem2(sciteActions.actionLoadSession.text, actionLoadSession)
                action: sciteActions.actionLoadSession
            }
            MenuItem {
                id: actionSaveSession
                text: processMenuItem2(sciteActions.actionSaveSession.text, actionSaveSession)
                action: sciteActions.actionSaveSession
            }
            MenuSeparator {}
            MenuItem {
                id: actionExit
                text: processMenuItem2(sciteActions.actionExit.text, actionExit)
                action: sciteActions.actionExit
            }
        }

        AutoSizingMenu {
            id: editMenu
            title: processMenuItem(qsTr("Edit"),null)

            MenuItem {
                id: actionUndo
                text: processMenuItem2(sciteActions.actionUndo.text, actionUndo)
                action: sciteActions.actionUndo
            }
            MenuItem {
                id: actionRedo
                text: processMenuItem2(sciteActions.actionRedo.text, actionRedo)
                action: sciteActions.actionRedo
            }
            MenuSeparator {}
            MenuItem {
                id: actionCut
                text: processMenuItem2(sciteActions.actionCut.text, actionCut)
                action: sciteActions.actionCut
            }
            MenuItem {
                id: actionCopy
                text: processMenuItem2(sciteActions.actionCopy.text, actionCopy)
                action: sciteActions.actionCopy
            }
            MenuItem {
                id: actionPaste
                text: processMenuItem2(sciteActions.actionPaste.text, actionPaste)
                action: sciteActions.actionPaste
            }
            MenuItem {
                id: actionDuplicate
                text: processMenuItem2(sciteActions.actionDuplicate.text, actionDuplicate)
                action: sciteActions.actionDuplicate
            }
            MenuItem {
                id: actionDelete
                text: processMenuItem2(sciteActions.actionDelete.text, actionDelete)
                action: sciteActions.actionDelete
            }
            MenuItem {
                id: actionSelectAll
                text: processMenuItem2(sciteActions.actionSelectAll.text, actionSelectAll)
                action: sciteActions.actionSelectAll
            }
            MenuItem {
                id: actionCopyAsRtf
                text: processMenuItem2(sciteActions.actionCopyAsRtf.text, actionCopyAsRtf)
                action: sciteActions.actionCopyAsRtf
            }
            MenuSeparator {}
            MenuItem {
                id: actionMatchBrace
                text: processMenuItem2(sciteActions.actionMatchBrace.text, actionMatchBrace)
                action: sciteActions.actionMatchBrace
            }
            MenuItem {
                id: actionSelectToBrace
                text: processMenuItem2(sciteActions.actionSelectToBrace.text, actionSelectToBrace)
                action: sciteActions.actionSelectToBrace
            }
            MenuItem {
                id: actionShowCalltip
                text: processMenuItem2(sciteActions.actionShowCalltip.text, actionShowCalltip)
                action: sciteActions.actionShowCalltip
            }
            MenuItem {
                id: actionCompleteSymbol
                text: processMenuItem2(sciteActions.actionCompleteSymbol.text, actionCompleteSymbol)
                action: sciteActions.actionCompleteSymbol
            }
            MenuItem {
                id: actionCompleteWord
                text: processMenuItem2(sciteActions.actionCompleteWord.text, actionCompleteWord)
                action: sciteActions.actionCompleteWord
            }
            MenuItem {
                id: actionExpandAbbreviation
                text: processMenuItem2(sciteActions.actionExpandAbbreviation.text, actionExpandAbbreviation)
                action: sciteActions.actionExpandAbbreviation
            }
            MenuItem {
                id: actionInsertAbbreviation
                text: processMenuItem(sciteActions.actionInsertAbbreviation.text, actionInsertAbbreviation)
                action: sciteActions.actionInsertAbbreviation
           }
            MenuItem {
                id: actionBlockComment
                text: processMenuItem(sciteActions.actionBlockComment.text, actionBlockComment)
                action: sciteActions.actionBlockComment
            }
            MenuItem {
                id: actionBoxComment
                text: processMenuItem(sciteActions.actionBoxComment.text, actionBoxComment)
                action: sciteActions.actionBoxComment
            }
            MenuItem {
                id: actionStreamComment
                text: processMenuItem(sciteActions.actionStreamComment.text, actionStreamComment)
                action: sciteActions.actionStreamComment
            }
            MenuItem {
                id: actionMakeSelectionUppercase
                text: processMenuItem(sciteActions.actionMakeSelectionUppercase.text, actionMakeSelectionUppercase)
                action: sciteActions.actionMakeSelectionUppercase
            }
            MenuItem {
                id: actionMakeSelectionLowercase
                text: processMenuItem(sciteActions.actionMakeSelectionLowercase.text, actionMakeSelectionLowercase)
                action: sciteActions.actionMakeSelectionLowercase
            }
            MenuItem {
                id: actionReverseSelectedLines
                text: processMenuItem(sciteActions.actionReverseSelectedLines.text, actionReverseSelectedLines)
                action: sciteActions.actionReverseSelectedLines
            }
            Menu {
                id: menuParagraph
                title: processMenuItem(qsTr("Para&graph"), menuParagraph)

                MenuItem {
                    id: actionJoin
                    text: processMenuItem2(sciteActions.actionJoin.text, actionJoin)
                    action: sciteActions.actionJoin
                }
                MenuItem {
                    id: actionSplit
                    text: processMenuItem2(sciteActions.actionSplit.text, actionSplit)
                    action: sciteActions.actionSplit
                }
            }
        }

        AutoSizingMenu {
            id: searchMenu
            title: processMenuItem(qsTr("Search"),null)

            MenuItem {
                id: actionFind
                text: processMenuItem2(sciteActions.actionFind.text, actionFind)
                action: sciteActions.actionFind
            }
            MenuItem {
                id: actionFindNext
                text: processMenuItem2(sciteActions.actionFindNext.text, actionFindNext)
                action: sciteActions.actionFindNext
            }
            MenuItem {
                id: actionFindPrevious
                text: processMenuItem2(sciteActions.actionFindPrevious.text, actionFindPrevious)
                action: sciteActions.actionFindPrevious
            }
            MenuItem {
                id: actionFindInFiles
                text: processMenuItem2(sciteActions.actionFindInFiles.text, actionFindInFiles)
                action: sciteActions.actionFindInFiles
            }
            MenuItem {
                id: actionReplace
                text: processMenuItem2(sciteActions.actionReplace.text, actionReplace)
                action: sciteActions.actionReplace
            }
            MenuItem {
                id: actionIncrementalSearch
                text: processMenuItem2(sciteActions.actionIncrementalSearch.text, actionIncrementalSearch)
                action: sciteActions.actionIncrementalSearch
            }
            MenuItem {
                id: actionSelectionAddNext
                text: processMenuItem2(sciteActions.actionSelectionAddNext.text, actionSelectionAddNext)
                action: sciteActions.actionSelectionAddNext
            }
            MenuItem {
                id: actionSelectionAddEach
                text: processMenuItem2(sciteActions.actionSelectionAddEach.text, actionSelectionAddEach)
                action: sciteActions.actionSelectionAddEach
            }
            MenuSeparator {}
            MenuItem {
                id: actionGoto
                text: processMenuItem2(sciteActions.actionGoto.text, actionGoto)
                action: sciteActions.actionGoto
            }
            MenuItem {
                id: actionNextBookmark
                text: processMenuItem2(sciteActions.actionNextBookmark.text, actionNextBookmark)
                action: sciteActions.actionNextBookmark
            }
            MenuItem {
                id: actionPreviousBookmark
                text: processMenuItem2(sciteActions.actionPreviousBookmark.text, actionPreviousBookmark)
                action: sciteActions.actionPreviousBookmark
            }
            MenuItem {
                id: actionToggleBookmark
                text: processMenuItem2(sciteActions.actionToggleBookmark.text, actionToggleBookmark)
                action: sciteActions.actionToggleBookmark
            }
            MenuItem {
                id: actionClearAllBookmarks
                text: processMenuItem2(sciteActions.actionClearAllBookmarks.text, actionClearAllBookmarks)
                action: sciteActions.actionClearAllBookmarks
            }
            MenuItem {
                id: actionSelectAllBookmarks
                text: processMenuItem2(sciteActions.actionSelectAllBookmarks.text, actionSelectAllBookmarks)
                action: sciteActions.actionSelectAllBookmarks
            }
        }

        AutoSizingMenu {
            id: viewMenu
            title: processMenuItem(qsTr("&View"),null)

            MenuItem {
                id: actionToggleCurrentFold
                text: processMenuItem2(sciteActions.actionToggleCurrentFold.text, actionToggleCurrentFold)
                action: sciteActions.actionToggleCurrentFold
            }
            MenuItem {
                id: actionToggleAllFolds
                text: processMenuItem2(sciteActions.actionToggleAllFolds.text, actionToggleAllFolds)
                action: sciteActions.actionToggleAllFolds
            }
            MenuSeparator {}
            MenuItem {
                id: actionFullScreen
                text: processMenuItem2(sciteActions.actionFullScreen.text, actionFullScreen)
                action: sciteActions.actionFullScreen
            }
            MenuItem {
                id: actionShowToolBar
                text: processMenuItem2(sciteActions.actionShowToolBar.text, actionShowToolBar)
                action: sciteActions.actionShowToolBar
            }
            MenuItem {
                id: actionShowTabBar
                text: processMenuItem2(sciteActions.actionShowTabBar.text, actionShowTabBar)
                action: sciteActions.actionShowTabBar
            }
            MenuItem {
                id: actionShowStatusBar
                text: processMenuItem2(sciteActions.actionShowStatusBar.text, actionShowStatusBar)
                action: sciteActions.actionShowStatusBar
            }
            MenuSeparator {}
            MenuItem {
                id: actionShowWhitespace
                text: processMenuItem2(sciteActions.actionShowWhitespace.text, actionShowWhitespace)
                action: sciteActions.actionShowWhitespace
            }
            MenuItem {
                id: actionShowEndOfLine
                text: processMenuItem2(sciteActions.actionShowEndOfLine.text, actionShowEndOfLine)
                action: sciteActions.actionShowEndOfLine
            }
            MenuItem {
                id: actionIndentaionGuides
                text: processMenuItem2(sciteActions.actionIndentaionGuides.text, actionIndentaionGuides)
                action: sciteActions.actionIndentaionGuides
            }
            MenuItem {
                id: actionLineNumbers
                text: processMenuItem2(sciteActions.actionLineNumbers.text, actionLineNumbers)
                action: sciteActions.actionLineNumbers
            }
            MenuItem {
                id: actionMargin
                text: processMenuItem2(sciteActions.actionMargin.text, actionMargin)
                action: sciteActions.actionMargin
            }
            MenuItem {
                id: actionFoldMargin
                text: processMenuItem2(sciteActions.actionFoldMargin.text, actionFoldMargin)
                action: sciteActions.actionFoldMargin
            }
            MenuItem {
                id: actionToggleOutput
                text: processMenuItem2(sciteActions.actionToggleOutput.text, actionToggleOutput)
                action: sciteActions.actionToggleOutput
            }
            MenuItem {
                id: actionParameters
                text: processMenuItem2(sciteActions.actionParameters.text, actionParameters)
                action: sciteActions.actionParameters
            }
        }

        AutoSizingMenu {
            id: toolsMenu
            title: processMenuItem(qsTr("Tools"),null)

            MenuItem {
                id: actionCompile
                text: processMenuItem2(sciteActions.actionCompile.text, actionCompile)
                action: sciteActions.actionCompile
            }
            MenuItem {
                id: actionBuild
                text: processMenuItem2(sciteActions.actionBuild.text, actionBuild)
                action: sciteActions.actionBuild
            }
            MenuItem {
                id: actionClean
                text: processMenuItem2(sciteActions.actionClean.text, actionClean)
                action: sciteActions.actionClean
            }
            MenuItem {
                id: actionGo
                text: processMenuItem2(sciteActions.actionGo.text, actionGo)
                action: sciteActions.actionGo
            }
            MenuItem {
                id: actionStopExecuting
                text: processMenuItem2(sciteActions.actionStopExecuting.text, actionStopExecuting)
                action: sciteActions.actionStopExecuting
            }
            MenuSeparator {}
            MenuItem {
                id: actionNextMessage
                text: processMenuItem2(sciteActions.actionNextMessage.text, actionNextMessage)
                action: sciteActions.actionNextMessage
            }
            MenuItem {
                id: actionPreviousMessage
                text: processMenuItem2(sciteActions.actionPreviousMessage.text, actionPreviousMessage)
                action: sciteActions.actionPreviousMessage
            }
            MenuItem {
                id: actionClearOutput
                text: processMenuItem2(sciteActions.actionClearOutput.text, actionClearOutput)
                action: sciteActions.actionClearOutput
            }
            MenuItem {
                id: actionSwitchPane
                text: processMenuItem2(sciteActions.actionSwitchPane.text, actionSwitchPane)
                action: sciteActions.actionSwitchPane
            }
        }

        AutoSizingMenu {
            id: optionsMenu
            title: processMenuItem(qsTr("Options"),null)

            MenuItem {
                id: actionAlwaysOnTop
                text: processMenuItem2(sciteActions.actionAlwaysOnTop.text, actionAlwaysOnTop)
                action: sciteActions.actionAlwaysOnTop
            }
            MenuItem {
                id: actionOpenFilesHere
                text: processMenuItem2(sciteActions.actionOpenFilesHere.text, actionOpenFilesHere)
                action: sciteActions.actionOpenFilesHere
            }
            MenuItem {
                id: actionVerticalSplit
                text: processMenuItem2(sciteActions.actionVerticalSplit.text, actionVerticalSplit)
                action: sciteActions.actionVerticalSplit
            }
            MenuItem {
                id: actionWrap
                text: processMenuItem2(sciteActions.actionWrap.text, actionWrap)
                action: sciteActions.actionWrap
            }
            MenuItem {
                id: actionWrapOutput
                text: processMenuItem2(sciteActions.actionWrapOutput.text, actionWrapOutput)
                action: sciteActions.actionWrapOutput
            }
            MenuItem {
                id: actionReadOnly
                text: processMenuItem2(sciteActions.actionReadOnly.text, actionReadOnly)
                action: sciteActions.actionReadOnly
            }
            MenuSeparator {}
            Menu {
                id: menuLineEndCharacters
                title: processMenuItem(qsTr("&Line End Characters"), menuLineEndCharacters)

                MenuItem {
                    id: actionCrLf
                    text: processMenuItem2(sciteActions.actionCrLf.text, actionCrLf)
                    action: sciteActions.actionCrLf
                }
                MenuItem {
                    id: actionCr
                    text: processMenuItem2(sciteActions.actionCr.text, actionCr)
                    action: sciteActions.actionCr
                }
                MenuItem {
                    id: actionLf
                    text: processMenuItem2(sciteActions.actionLf.text, actionLf)
                    action: sciteActions.actionLf
                }
            }
            MenuItem {
                id: actionConvertLineEndChar
                text: processMenuItem2(sciteActions.actionConvertLineEndChar.text, actionConvertLineEndChar)
                action: sciteActions.actionConvertLineEndChar
            }
            MenuSeparator {}
            MenuItem {
                id: actionChangeIndentationSettings
                text: processMenuItem2(sciteActions.actionChangeIndentationSettings.text, actionChangeIndentationSettings)
                action: sciteActions.actionChangeIndentationSettings
            }
            MenuItem {
                id: actionUseMonospacedFont
                text: processMenuItem2(sciteActions.actionUseMonospacedFont.text, actionUseMonospacedFont)
                action: sciteActions.actionUseMonospacedFont
            }
            MenuSeparator {}
            MenuItem {
                id: actionOpenLocalOptionsFile
                text: processMenuItem2(sciteActions.actionOpenLocalOptionsFile.text, actionOpenLocalOptionsFile)
                action: sciteActions.actionOpenLocalOptionsFile
            }
            MenuItem {
                id: actionOpenDirectoryOptionsFile
                text: processMenuItem2(sciteActions.actionOpenDirectoryOptionsFile.text, actionOpenDirectoryOptionsFile)
                action: sciteActions.actionOpenDirectoryOptionsFile
            }
            MenuItem {
                id: actionOpenUserOptionsFile
                text: processMenuItem2(sciteActions.actionOpenUserOptionsFile.text, actionOpenUserOptionsFile)
                action: sciteActions.actionOpenUserOptionsFile
            }
            MenuItem {
                id: actionOpenGlobalOptionsFile
                text: processMenuItem2(sciteActions.actionOpenGlobalOptionsFile.text, actionOpenGlobalOptionsFile)
                action: sciteActions.actionOpenGlobalOptionsFile
            }
            MenuItem {
                id: actionOpenAbbreviationsFile
                text: processMenuItem2(sciteActions.actionOpenAbbreviationsFile.text, actionOpenAbbreviationsFile)
                action: sciteActions.actionOpenAbbreviationsFile
            }
            MenuItem {
                id: actionOpenLuaStartupScript
                text: processMenuItem2(sciteActions.actionOpenLuaStartupScript.text, actionOpenLuaStartupScript)
                action: sciteActions.actionOpenLuaStartupScript
            }
            MenuSeparator {}
        }

        Menu {
            id: languageMenu
            title: processMenuItem(qsTr("Language"),null)

            Instantiator {
                id: currentLanguagesItems
                model: languagesModel
                delegate: MenuItem {
                    //checkable: true
                    //checked: model !== null ? model.checkState : false
                    text: model.display // index is also available
                    onTriggered: sciteQt.cmdSelectLanguage(index)
                }

                onObjectAdded: languageMenu.insertItem(index, object)
                onObjectRemoved: languageMenu.removeItem(object)
            }
        }

        AutoSizingMenu {
            id: buffersMenu
            title: processMenuItem(qsTr("&Buffers"),null)

            MenuItem {
                id: actionBuffersPrevious
                text: processMenuItem2(sciteActions.actionBuffersPrevious.text, actionBuffersPrevious)
                action: sciteActions.actionBuffersPrevious
            }
            MenuItem {
                id: actionBuffersNext
                text: processMenuItem2(sciteActions.actionBuffersNext.text, actionBuffersNext)
                action: sciteActions.actionBuffersNext
            }
            MenuItem {
                id: actionBuffersCloseAll
                text: processMenuItem2(sciteActions.actionBuffersCloseAll.text, actionBuffersCloseAll)
                action: sciteActions.actionBuffersCloseAll
            }
            MenuItem {
                id: actionBuffersSaveAll
                text: processMenuItem2(sciteActions.actionBuffersSaveAll.text, actionBuffersSaveAll)
                action: sciteActions.actionBuffersSaveAll
            }

            MenuSeparator {}

            Instantiator {
                id: currentBufferItems
                model: buffersModel
                delegate: MenuItem {
                    checkable: true
                    checked: model.checkState ? Qt.Checked : Qt.Unchecked
                    text: model.display // index is also available
                    onTriggered: sciteQt.cmdSelectBuffer(index)
                }

                onObjectAdded: buffersMenu.insertItem(index+5, object)
                onObjectRemoved: buffersMenu.removeItem(object)
            }
        }

        AutoSizingMenu {
            id: helpMenu
            title: processMenuItem(qsTr("Help"),null)

            MenuItem {
                id: actionHelp
                text: processMenuItem2(sciteActions.actionHelp.text, actionHelp)
                action: sciteActions.actionHelp
            }
            MenuItem {
                id: actionSciteHelp
                text: processMenuItem2(sciteActions.actionSciteHelp.text, actionSciteHelp)
                action: sciteActions.actionSciteHelp
            }
            MenuItem {
                id: actionAboutScite
                text: processMenuItem2(sciteActions.actionAboutScite.text, actionAboutScite)
                action: sciteActions.actionAboutScite
            }

            MenuSeparator {}

            MenuItem {
                id: actionDebugInfo
                text: processMenuItem(qsTr("Debug info"),actionDebugInfo)
                onTriggered: {
                    //showInfoDialog(quickScintillaEditor.text)
                    ///*for Tests only: */quickScintillaEditor.text = applicationData.readLog()
                    //console.log("dbg: "+myModel+" "+myModel.count)
                    //myModel.append({"display":"blub blub"})
                    //console.log("dbg: "+myModel+" "+myModel.count+" "+myModel.get(0))
                    removeInBuffersModel(0)
                }
            }
/*
            Instantiator {
                id: dynamicTestMenu
                model: buffersModel
                delegate: MenuItem {
                    checkable: true
                    checked: model.checkState ? Qt.Checked : Qt.Unchecked
                    text: model.display
                    onTriggered: console.log(index)
                }

                onObjectAdded: helpMenu.insertItem(index+6, object)
                onObjectRemoved: helpMenu.removeItem(object)
            }
*/

        }
    }

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
            //console.log("Accepted: " + /*currentFile*/fileUrl+" "+fileDialog.openMode)
            if(!fileDialog.openMode) {
                writeCurrentDoc(fileUrl)
            }
            else {
                //Android: quickScintillaEditor.text = fileUrl
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

            SplitView.fillWidth: true
            SplitView.fillHeight: true

            //text: "editor area !"
        }

        ScintillaText {
            id: quickScintillaOutput

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

    function clearBuffersModel(model) {
        model.clear()
    }

    function writeInBuffersModel(model,index, name, checked) {
        model.set(index, {"display":name, "checkState":checked})
    }

    function removeInBuffersModel(model,index) {
        if(index < model.count) {
            model.remove(index)
        }
    }

    function setCheckStateInBuffersModel(model,index, checked) {
        if(index < model.count) {
            model.setProperty(index,"checkState",checked)
        }
    }

    function handleMenuChecked(menuId, val) {
        switch(menuId) {
            case 450:  //IDM_MONOFONT
                actionUseMonospacedFont.checked = val
                break;
            case 411:  //IDM_VIEWSTATUSBAR
                actionShowStatusBar.checked = val
                break;
            case 410:  //IDM_VIEWTABBAR
                actionShowTabBar.checked = val
                break;
            case 409:  //IDM_TOGGLEOUTPUT
                actionToggleOutput.checked = val
                break;
            case 408:  //IDM_VIEWTOOLBAR
                actionShowToolBar.checked = val
                break;
            case 407:  //IDM_LINENUMBERMARGIN
                actionLineNumbers.checked = val
                break;
            case 406:  //IDM_FOLDMARGIN
                actionFoldMargin.checked = val
                break;
            case 405:  //IDM_SELMARGIN
                actionMargin.checked = val
                break;
            case 404:  //IDM_VIEWGUIDES
                actionIndentaionGuides.checked = val
                break;
            case 403:  //IDM_VIEWEOL
                actionShowEndOfLine.checked = val
                break;
            case 402:  //IDM_VIEWSPACE
                actionShowWhitespace.checked = val
                break;
            case 401:  //IDM_SPLITVERTICAL
                actionVerticalSplit.checked = val
                break;
            case 414:  //IDM_WRAP
                actionWrap.checked = val
                break;
            case 415:  //IDM_WRAPOUTPUT
                actionWrapOutput.checked = val
                break;
            case 416:  //IDM_READONLY
                actionReadOnly.checked = val
                break;
            case 430:  //IDM_EOL_CRLF
                actionCrLf.checked = val
                break;
            case 431:  //IDM_EOL_CR
                actionCr.checked = val
                break;
            case 432:  //IDM_EOL_LF
                actionLf.checked = val
                break;
            default:
//                console.log("unhandled menu checked "+menuId)
        }
    }

    function handleMenuEnable(menuId, val) {
        switch(menuId) {
            case 301:  //IDM_COMPILE
                actionCompile.enabled = val
                break;
            case 302:  //IDM_BUILD
                actionBuild.enabled = val
                break;
            case 303:  //IDM_GO
                actionGo.enabled = val
                break;
            case 304:  //IDM_STOPEXECUTE
                actionStopExecuting.enabled = val
                break;
            case 308:  //IDM_CLEAN
                actionClean.enabled = val
                break;
            case 413:  //IDM_OPENFILESHERE
                actionOpenFilesHere.enabled = val
                break;
            case 465:  //IDM_OPENDIRECTORYPROPERTIES
                actionOpenDirectoryOptionsFile.enabled = val
                break;
// TODO: 313, 311, 312 (for macros)
            default:
//                console.log("unhandled menu enable "+menuId)
        }
    }

    ListModel {
        id: buffersModel
        /*
        ListElement {
            display: "hello"
            checkState: true
        }
        */
    }

    ListModel {
        id: languagesModel
        /*
        ListElement {
            display: "hello"
            checkState: true
        }
        */
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

        onStartFileDialog:            startFileDialog(sDirectory, sFilter, bAsOpenDialog)
        onShowInfoDialog:             showInfoDialog(sInfoText, style)

        onShowFindInFilesDialog:      showFindInFilesDialog(text)
        onShowFind:                   showFind(text, incremental, withReplace)
        onShowGoToDialog:             showGoToDialog(currentLine, currentColumn, maxLine)

        onSetVerticalSplit:           setVerticalSplit(verticalSplit)
        onSetOutputHeight:            setOutputHeight(heightOutput)

        onSetMenuChecked:             handleMenuChecked(menuID, val)
        onSetMenuEnable:              handleMenuEnable(menuID, val)

        onSetInBuffersModel:          writeInBuffersModel(buffersModel, index, txt, checked)
        onRemoveInBuffersModel:       removeInBuffersModel(buffersModel, index)
        onCheckStateInBuffersModel:   setCheckStateInBuffersModel(buffersModel, index, checked)

        onSetInLanguagesModel:        writeInBuffersModel(languagesModel, index, txt, checked)
        onRemoveInLanguagesModel:     removeInLanguagesModel(languagesModel, index)
        onCheckStateInLanguagesModel: setCheckStateInLanguagesModel(languagesModel, index, checked)

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
        var item = tabButton.createObject(tabBar, {text: title, fcnClicked: function () { sciteQt.cmdSelectBuffer(index) }})
        tabBar.insertItem(index, item)
    }
}
