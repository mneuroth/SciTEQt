import QtQuick 2.0
import QtQuick.Controls 2.9
import QtQml.Models 2.14

MenuBar {
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
            id: actionSaveACopy
            text: processMenuItem2(sciteActions.actionSaveACopy.text, actionSaveACopy)
            action: sciteActions.actionSaveACopy
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
        title: processMenuItem(qsTr("&Edit"),null)

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
        title: processMenuItem(qsTr("&Search"),null)

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
        title: processMenuItem(qsTr("&Tools"),null)

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
        Instantiator {
            id: currentToolsItems
            model: toolsModel
            delegate: MenuItem {
                action: Action {
                    text: model.display+(model.shortcut.length>0 ? (" ("+model.shortcut+")") : "")
                    shortcut: model.shortcut
                    onTriggered: sciteQt.cmdCallTool(index)
                }
            }

            onObjectAdded: toolsMenu.insertItem(index+4, object)
            onObjectRemoved: toolsMenu.removeItem(object)
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
        title: processMenuItem(qsTr("&Options"),null)

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
        title: processMenuItem(qsTr("&Language"),null)

        Instantiator {
            id: currentLanguagesItems
            model: languagesModel
            delegate: MenuItem {                
                //checkable: true
                //checked: model !== null ? model.checkState : false
                action: Action {
                    text: model.display+(model.shortcut.length>0 ? (" ("+model.shortcut+")") : "")
                    shortcut: model.shortcut
                    onTriggered: sciteQt.cmdSelectLanguage(index)
                }
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
                action: Action {
                    text: model.display+(model.shortcut.length>0 ? (" ("+model.shortcut+")") : "")
                    shortcut: model.shortcut
                    onTriggered: sciteQt.cmdSelectBuffer(index)
                }
            }

            onObjectAdded: buffersMenu.insertItem(index+5, object)
            onObjectRemoved: buffersMenu.removeItem(object)
        }
    }

    AutoSizingMenu {
        id: helpMenu
        title: processMenuItem(qsTr("&Help"),null)
/*
        Menu {
            id: helpMenu2
            title: processMenuItem(qsTr("&Help 2"),null)

            MenuItem {
                id: actionHelp2
                text: processMenuItem2(sciteActions.actionHelp.text, actionHelp2)
                action: sciteActions.actionHelp
            }
            MenuItem {
                id: actionSciteHelp2
                text: processMenuItem2(sciteActions.actionSciteHelp.text, actionSciteHelp2)
                action: sciteActions.actionSciteHelp
            }
        }
*/
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
        MenuItem {
            id: actionAboutSciteQt
            text: processMenuItem2(sciteActions.actionAboutSciteQt.text, actionAboutSciteQt)
            action: sciteActions.actionAboutSciteQt
        }

        MenuSeparator {}

        MenuItem {
            id: actionTestFunction
            text: processMenuItem2(sciteActions.actionTestFunction.text, actionTestFunction)
            action: sciteActions.actionTestFunction
        }
        MenuItem {
            id: actionTest2Function
            text: processMenuItem2(sciteActions.actionTest2Function.text, actionTest2Function)
            action: sciteActions.actionTest2Function
        }
        MenuItem {
            id: actionTest3Function
            text: processMenuItem2(sciteActions.actionTest3Function.text, actionTest3Function)
            action: sciteActions.actionTest3Function
        }
        MenuItem {
            id: actionTest4Function
            text: processMenuItem2(sciteActions.actionTest4Function.text, actionTest4Function)
            action: sciteActions.actionTest4Function
        }
        MenuItem {
            id: actionTest5Function
            text: processMenuItem2(sciteActions.actionTest5Function.text, actionTest5Function)
            action: sciteActions.actionTest5Function
        }
        MenuItem {
            id: actionDebugInfo
            text: processMenuItem(qsTr("Debug info"),actionDebugInfo)
            onTriggered: {
                //showInfoDialog(quickScintillaEditor.text)
                ///*for Tests only: */quickScintillaEditor.text = applicationData.readLog()
                //console.log("dbg: "+myModel+" "+myModel.count)
                //myModel.append({"display":"blub blub"})
                //console.log("dbg: "+myModel+" "+myModel.count+" "+myModel.get(0))
                //removeInMenuModel(0)
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

    ListModel {
        id: buffersModel
        objectName: "buffersMenu"
        /*
        ListElement {
            display: "hello"
            checkState: true
        }
        */
    }

    ListModel {
        id: languagesModel
        objectName: "languagesMenu"
    }

    ListModel {
        id: toolsModel
        objectName: "toolsMenu"
    }

    function clearBuffersModel(model) {
        model.clear()
    }

    function writeInMenuModel(model, index, name, checked, shortcut) {
        model.set(index, {"display":name, "checkState":checked, "shortcut":shortcut})
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
        //console.log("menu enable "+menuId+" "+val)
        switch(menuId) {
//#define IDM_SHOWCALLTIP		232
//#define IDM_COMPLETE		233
            case 201:   //IDM_UNDO
                actionUndo.enabled = val
                break;
            case 202:   //IDM_REDO
                actionRedo.enabled = val
                break;
            case 203:   //IDM_CUT
                actionCut.enabled = val
                break;
            case 204:   //IDM_COPY
                actionCopy.enabled = val
                break;
            case 205:   //IDM_PASTE
                actionPaste.enabled = val
                break;
            case 206:  //IDM_CLEAR
                actionDelete.enabled = val
                break;
            case 232:  //IDM_SHOWCALLTIP
                actionShowCalltip.enabled = val
                break;
            case 233:  //IDM_COMPLETE
                actionCompleteSymbol.enabled = val
                break;
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
                //console.log("unhandled menu enable "+menuId)
        }
    }

    Connections {
        target: sciteQt

        onSetMenuChecked:             handleMenuChecked(menuID, val)
        onSetMenuEnable:              handleMenuEnable(menuID, val)

        onSetInBuffersModel:          writeInMenuModel(buffersModel, index, txt, checked, shortcut)
        onRemoveInBuffersModel:       removeInMenuModel(buffersModel, index)
        onCheckStateInBuffersModel:   setCheckStateInMenuModel(buffersModel, index, checked)

        onSetInLanguagesModel:        writeInMenuModel(languagesModel, index, txt, checked, shortcut)
        onRemoveInLanguagesModel:     removeInMenuModel(languagesModel, index)
        onCheckStateInLanguagesModel: setCheckStateInMenuModel(languagesModel, index, checked)

        onSetInToolsModel:            writeInMenuModel(toolsModel, index, txt, checked, shortcut)
        onRemoveInToolsModel:         removeInMenuModel(toolsModel, index)
        onCheckStateInToolsModel:     setCheckStateInMenuModel(toolsModel, index, checked)
    }

}
