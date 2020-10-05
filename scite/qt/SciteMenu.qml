import QtQuick 2.0
import QtQuick.Controls 2.9
import QtQml.Models 2.14

MenuBar {
    id: root

    signal readOnlyChanged(bool value)
    signal runningChanged(bool value)
    signal buildChanged(bool value)
    signal copyCutChanged(bool value)
    signal pasteChanged(bool value)
    signal undoChanged(bool value)
    signal redoChanged(bool value)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

    function fillPopupMenu(menu) {
        menu.removeItem(0, menu.count)
        for(var i=0; i<root.count; i++)
        {
            menu.addMenu(root.menuAt(i)) // takeMenu(i) ?
        }
        return menu
    }

    AutoSizingMenu {
        id: fileMenu
        title: processMenuItem(qsTr("&File"),null)
// see: https://forum.qt.io/topic/104010/menubar-display-shortcut-in-quickcontrols2/4
        MenuItem {
            id: actionNew
            text: processMenuItem2(actions.actionNew.text, actionNew)
            action: actions.actionNew
        }
        MenuItem {
            id: actionOpen
            text: processMenuItem2(actions.actionOpen.text, actionOpen)
            action: actions.actionOpen
        }
        MenuItem {
            id: actionOpenSelectedFilename
            text: processMenuItem2(actions.actionOpenSelectedFilename.text, actionOpenSelectedFilename)
            action: actions.actionOpenSelectedFilename
        }
        MenuItem {
            id: actionRevert
            text: processMenuItem2(actions.actionRevert.text, actionRevert)
            action: actions.actionRevert
        }
        MenuItem {
            id: actionClose
            text: processMenuItem2(actions.actionClose.text, actionClose)
            action: actions.actionClose
        }
        MenuItem {
            id: actionSave
            text: processMenuItem2(actions.actionSave.text, actionSave)
            action: actions.actionSave
        }
        MenuItem {
            id: actionSaveAs
            text: processMenuItem2(actions.actionSaveAs.text, actionSaveAs)
            action: actions.actionSaveAs
        }
        MenuItem {
            id: actionSaveACopy
            text: processMenuItem2(actions.actionSaveACopy.text, actionSaveACopy)
            action: actions.actionSaveACopy
        }
        MenuItem {
            id: actionCopyPath
            text: processMenuItem2(actions.actionCopyPath.text, actionCopyPath)
            action: actions.actionCopyPath
        }
        MenuItem {
            id: actionOpenContainingFolder
            text: processMenuItem2(actions.actionOpenContainingFolder.text, actionOpenContainingFolder)
            action: actions.actionOpenContainingFolder
        }
        Menu {
            id: actionEncoding
            title: processMenuItem(qsTr("Encodin&g"), actionEncoding)

            MenuItem {
                id: actionCodePageProperty
                text: processMenuItem2(actions.actionCodePageProperty.text, actionCodePageProperty)
                action: actions.actionCodePageProperty
            }
            MenuItem {
                id: actionUtf16BigEndian
                text: processMenuItem2(actions.actionUtf16BigEndian.text, actionUtf16BigEndian)
                action: actions.actionUtf16BigEndian
            }
            MenuItem {
                id: actionUtf16LittleEndian
                text: processMenuItem2(actions.actionUtf16LittleEndian.text, actionUtf16LittleEndian)
                action: actions.actionUtf16LittleEndian
            }
            MenuItem {
                id: actionUtf8WithBOM
                text: processMenuItem2(actions.actionUtf8WithBOM.text, actionUtf8WithBOM)
                action: actions.actionUtf8WithBOM
            }
            MenuItem {
                id: actionUtf8
                text: processMenuItem2(actions.actionUtf8.text, actionUtf8)
                action: actions.actionUtf8
            }
        }
        Menu {
            id: actionExport
            title: processMenuItem(qsTr("&Export"), actionExport)

            MenuItem {
                id: actionAsHtml
                text: processMenuItem2(actions.actionAsHtml.text, actionAsHtml)
                action: actions.actionAsHtml
            }
            MenuItem {
                id: actionAsRtf
                text: processMenuItem2(actions.actionAsRtf.text, actionAsRtf)
                action: actions.actionAsRtf
            }
            MenuItem {
                id: actionAsPdf
                text: processMenuItem2(actions.actionAsPdf.text, actionAsPdf)
                action: actions.actionAsPdf
            }
            MenuItem {
                id: actionAsLatex
                text: processMenuItem2(actions.actionAsLatex.text, actionAsLatex)
                action: actions.actionAsLatex
            }
            MenuItem {
                id: actionAsXml
                text: processMenuItem2(actions.actionAsXml.text, actionAsXml)
                action: actions.actionAsXml
            }
        }
        MenuSeparator {}
        MenuItem {
            id: actionPageSetup
            text: processMenuItem2(actions.actionPageSetup.text, actionPageSetup)
            action: actions.actionPageSetup
        }
        MenuItem {
            id: actionPrint
            text: processMenuItem2(actions.actionPrint.text, actionPrint)
            action: actions.actionPrint
        }
        MenuSeparator {}
        MenuItem {
            id: actionLoadSession
            text: processMenuItem2(actions.actionLoadSession.text, actionLoadSession)
            action: actions.actionLoadSession
        }
        MenuItem {
            id: actionSaveSession
            text: processMenuItem2(actions.actionSaveSession.text, actionSaveSession)
            action: actions.actionSaveSession
        }
        MenuSeparator {}
        Instantiator {
            id: lastOpenedFilesItems
            model: lastOpenedFilesModel
            delegate: MenuItem {
                action: Action {
                    text: model.display
                    shortcut: model.shortcut
                    onTriggered: sciteQt.cmdLastOpenedFiles(index)
                }
            }

            onObjectAdded: fileMenu.insertItem(index+18, object)
            onObjectRemoved: fileMenu.removeItem(object)
        }
        MenuSeparator {
            visible: lastOpenedFilesModel.count>0
        }
        MenuItem {
            id: actionExit
            text: processMenuItem2(actions.actionExit.text, actionExit)
            action: actions.actionExit
        }
    }

    AutoSizingMenu {
        id: editMenu
        title: processMenuItem(qsTr("&Edit"),null)

        MenuItem {
            id: actionUndo
            text: processMenuItem2(actions.actionUndo.text, actionUndo)
            action: actions.actionUndo
        }
        MenuItem {
            id: actionRedo
            text: processMenuItem2(actions.actionRedo.text, actionRedo)
            action: actions.actionRedo
        }
        MenuSeparator {}
        MenuItem {
            id: actionCut
            text: processMenuItem2(actions.actionCut.text, actionCut)
            action: actions.actionCut
        }
        MenuItem {
            id: actionCopy
            text: processMenuItem2(actions.actionCopy.text, actionCopy)
            action: actions.actionCopy
        }
        MenuItem {
            id: actionPaste
            text: processMenuItem2(actions.actionPaste.text, actionPaste)
            action: actions.actionPaste
        }
        MenuItem {
            id: actionDuplicate
            text: processMenuItem2(actions.actionDuplicate.text, actionDuplicate)
            action: actions.actionDuplicate
        }
        MenuItem {
            id: actionDelete
            text: processMenuItem2(actions.actionDelete.text, actionDelete)
            action: actions.actionDelete
        }
        MenuItem {
            id: actionSelectAll
            text: processMenuItem2(actions.actionSelectAll.text, actionSelectAll)
            action: actions.actionSelectAll
        }
        MenuItem {
            id: actionCopyAsRtf
            text: processMenuItem2(actions.actionCopyAsRtf.text, actionCopyAsRtf)
            action: actions.actionCopyAsRtf
            visible: !useSimpleMenu
            height: useSimpleMenu ? 0 : actionCopy.height
        }
        MenuSeparator {}
        MenuItem {
            id: actionMatchBrace
            text: processMenuItem2(actions.actionMatchBrace.text, actionMatchBrace)
            action: actions.actionMatchBrace
        }
        MenuItem {
            id: actionSelectToBrace
            text: processMenuItem2(actions.actionSelectToBrace.text, actionSelectToBrace)
            action: actions.actionSelectToBrace
        }
        MenuItem {
            id: actionShowCalltip
            text: processMenuItem2(actions.actionShowCalltip.text, actionShowCalltip)
            action: actions.actionShowCalltip
            visible: !useSimpleMenu
            height: useSimpleMenu ? 0 : actionCopy.height
        }
        MenuItem {
            id: actionCompleteSymbol
            text: processMenuItem2(actions.actionCompleteSymbol.text, actionCompleteSymbol)
            action: actions.actionCompleteSymbol
        }
        MenuItem {
            id: actionCompleteWord
            text: processMenuItem2(actions.actionCompleteWord.text, actionCompleteWord)
            action: actions.actionCompleteWord
        }
        MenuItem {
            id: actionExpandAbbreviation
            text: processMenuItem2(actions.actionExpandAbbreviation.text, actionExpandAbbreviation)
            action: actions.actionExpandAbbreviation
        }
        MenuItem {
            id: actionInsertAbbreviation
            text: processMenuItem(actions.actionInsertAbbreviation.text, actionInsertAbbreviation)
            action: actions.actionInsertAbbreviation
       }
        MenuItem {
            id: actionBlockComment
            text: processMenuItem(actions.actionBlockComment.text, actionBlockComment)
            action: actions.actionBlockComment
        }
        MenuItem {
            id: actionBoxComment
            text: processMenuItem(actions.actionBoxComment.text, actionBoxComment)
            action: actions.actionBoxComment
            visible: !useSimpleMenu
            height: useSimpleMenu ? 0 : actionCopy.height
        }
        MenuItem {
            id: actionStreamComment
            text: processMenuItem(actions.actionStreamComment.text, actionStreamComment)
            action: actions.actionStreamComment
            visible: !useSimpleMenu
            height: useSimpleMenu ? 0 : actionCopy.height
        }
        MenuItem {
            id: actionMakeSelectionUppercase
            text: processMenuItem(actions.actionMakeSelectionUppercase.text, actionMakeSelectionUppercase)
            action: actions.actionMakeSelectionUppercase
        }
        MenuItem {
            id: actionMakeSelectionLowercase
            text: processMenuItem(actions.actionMakeSelectionLowercase.text, actionMakeSelectionLowercase)
            action: actions.actionMakeSelectionLowercase
        }
        MenuItem {
            id: actionReverseSelectedLines
            text: processMenuItem(actions.actionReverseSelectedLines.text, actionReverseSelectedLines)
            action: actions.actionReverseSelectedLines
        }
        Menu {
            id: menuParagraph
            title: processMenuItem(qsTr("Para&graph"), menuParagraph)
            visible: !useSimpleMenu
            height: useSimpleMenu ? 0 : actionCopy.height

            MenuItem {
                id: actionJoin
                text: processMenuItem2(actions.actionJoin.text, actionJoin)
                action: actions.actionJoin
                visible: !useSimpleMenu
                height: useSimpleMenu ? 0 : actionCopy.height
            }
            MenuItem {
                id: actionSplit
                text: processMenuItem2(actions.actionSplit.text, actionSplit)
                action: actions.actionSplit
                visible: !useSimpleMenu
                height: useSimpleMenu ? 0 : actionCopy.height
            }
        }
    }

    AutoSizingMenu {
        id: searchMenu
        title: processMenuItem(qsTr("&Search"),null)

        MenuItem {
            id: actionFind
            text: processMenuItem2(actions.actionFind.text, actionFind)
            action: actions.actionFind
        }
        MenuItem {
            id: actionFindNext
            text: processMenuItem2(actions.actionFindNext.text, actionFindNext)
            action: actions.actionFindNext
        }
        MenuItem {
            id: actionFindPrevious
            text: processMenuItem2(actions.actionFindPrevious.text, actionFindPrevious)
            action: actions.actionFindPrevious
        }
        MenuItem {
            id: actionFindInFiles
            text: processMenuItem2(actions.actionFindInFiles.text, actionFindInFiles)
            action: actions.actionFindInFiles
        }
        MenuItem {
            id: actionReplace
            text: processMenuItem2(actions.actionReplace.text, actionReplace)
            action: actions.actionReplace
        }
        MenuItem {
            id: actionIncrementalSearch
            text: processMenuItem2(actions.actionIncrementalSearch.text, actionIncrementalSearch)
            action: actions.actionIncrementalSearch
        }
        MenuItem {
            id: actionSelectionAddNext
            text: processMenuItem2(actions.actionSelectionAddNext.text, actionSelectionAddNext)
            action: actions.actionSelectionAddNext
        }
        MenuItem {
            id: actionSelectionAddEach
            text: processMenuItem2(actions.actionSelectionAddEach.text, actionSelectionAddEach)
            action: actions.actionSelectionAddEach
        }
        MenuSeparator {}
        MenuItem {
            id: actionGoto
            text: processMenuItem2(actions.actionGoto.text, actionGoto)
            action: actions.actionGoto
        }
        MenuItem {
            id: actionNextBookmark
            text: processMenuItem2(actions.actionNextBookmark.text, actionNextBookmark)
            action: actions.actionNextBookmark
        }
        MenuItem {
            id: actionPreviousBookmark
            text: processMenuItem2(actions.actionPreviousBookmark.text, actionPreviousBookmark)
            action: actions.actionPreviousBookmark
        }
        MenuItem {
            id: actionToggleBookmark
            text: processMenuItem2(actions.actionToggleBookmark.text, actionToggleBookmark)
            action: actions.actionToggleBookmark
        }
        MenuItem {
            id: actionClearAllBookmarks
            text: processMenuItem2(actions.actionClearAllBookmarks.text, actionClearAllBookmarks)
            action: actions.actionClearAllBookmarks
        }
        MenuItem {
            id: actionSelectAllBookmarks
            text: processMenuItem2(actions.actionSelectAllBookmarks.text, actionSelectAllBookmarks)
            action: actions.actionSelectAllBookmarks
        }
    }

    AutoSizingMenu {
        id: viewMenu
        title: processMenuItem(qsTr("&View"),null)

        MenuItem {
            id: actionToggleCurrentFold
            text: processMenuItem2(actions.actionToggleCurrentFold.text, actionToggleCurrentFold)
            action: actions.actionToggleCurrentFold
        }
        MenuItem {
            id: actionToggleAllFolds
            text: processMenuItem2(actions.actionToggleAllFolds.text, actionToggleAllFolds)
            action: actions.actionToggleAllFolds
        }
        MenuSeparator {}
        MenuItem {
            id: actionFullScreen
            text: processMenuItem2(actions.actionFullScreen.text, actionFullScreen)
            action: actions.actionFullScreen
        }
        MenuItem {
            id: actionShowToolBar
            text: processMenuItem2(actions.actionShowToolBar.text, actionShowToolBar)
            action: actions.actionShowToolBar
        }
        MenuItem {
            id: actionShowTabBar
            text: processMenuItem2(actions.actionShowTabBar.text, actionShowTabBar)
            action: actions.actionShowTabBar
        }
        MenuItem {
            id: actionShowStatusBar
            text: processMenuItem2(actions.actionShowStatusBar.text, actionShowStatusBar)
            action: actions.actionShowStatusBar
        }
        MenuSeparator {}
        MenuItem {
            id: actionShowWhitespace
            text: processMenuItem2(actions.actionShowWhitespace.text, actionShowWhitespace)
            action: actions.actionShowWhitespace
        }
        MenuItem {
            id: actionShowEndOfLine
            text: processMenuItem2(actions.actionShowEndOfLine.text, actionShowEndOfLine)
            action: actions.actionShowEndOfLine
        }
        MenuItem {
            id: actionIndentaionGuides
            text: processMenuItem2(actions.actionIndentaionGuides.text, actionIndentaionGuides)
            action: actions.actionIndentaionGuides
        }
        MenuItem {
            id: actionLineNumbers
            text: processMenuItem2(actions.actionLineNumbers.text, actionLineNumbers)
            action: actions.actionLineNumbers
        }
        MenuItem {
            id: actionMargin
            text: processMenuItem2(actions.actionMargin.text, actionMargin)
            action: actions.actionMargin
        }
        MenuItem {
            id: actionFoldMargin
            text: processMenuItem2(actions.actionFoldMargin.text, actionFoldMargin)
            action: actions.actionFoldMargin
        }
        MenuItem {
            id: actionToggleOutput
            text: processMenuItem2(actions.actionToggleOutput.text, actionToggleOutput)
            action: actions.actionToggleOutput
        }
        MenuItem {
            id: actionParameters
            text: processMenuItem2(actions.actionParameters.text, actionParameters)
            action: actions.actionParameters
        }
// TODO --> add Fenster here !!!
// TODO --> Extras & Help --> Diverses ?
    }

    AutoSizingMenu {
        id: toolsMenu
        title: processMenuItem(qsTr("&Tools"),null)

        MenuItem {
            id: actionCompile
            text: processMenuItem2(actions.actionCompile.text, actionCompile)
            action: actions.actionCompile
        }
        MenuItem {
            id: actionBuild
            text: processMenuItem2(actions.actionBuild.text, actionBuild)
            action: actions.actionBuild
        }
        MenuItem {
            id: actionClean
            text: processMenuItem2(actions.actionClean.text, actionClean)
            action: actions.actionClean
        }
        MenuItem {
            id: actionGo
            text: processMenuItem2(actions.actionGo.text, actionGo)
            action: actions.actionGo
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
            text: processMenuItem2(actions.actionStopExecuting.text, actionStopExecuting)
            action: actions.actionStopExecuting
        }
        MenuSeparator {}
        MenuItem {
            id: actionNextMessage
            text: processMenuItem2(actions.actionNextMessage.text, actionNextMessage)
            action: actions.actionNextMessage
        }
        MenuItem {
            id: actionPreviousMessage
            text: processMenuItem2(actions.actionPreviousMessage.text, actionPreviousMessage)
            action: actions.actionPreviousMessage
        }
        MenuItem {
            id: actionClearOutput
            text: processMenuItem2(actions.actionClearOutput.text, actionClearOutput)
            action: actions.actionClearOutput
        }
        MenuItem {
            id: actionSwitchPane
            text: processMenuItem2(actions.actionSwitchPane.text, actionSwitchPane)
            action: actions.actionSwitchPane
        }
    }

    AutoSizingMenu {
        id: optionsMenu
        title: processMenuItem(qsTr("&Options"),null)

        MenuItem {
            id: actionAlwaysOnTop
            text: processMenuItem2(actions.actionAlwaysOnTop.text, actionAlwaysOnTop)
            action: actions.actionAlwaysOnTop
        }
        MenuItem {
            id: actionOpenFilesHere
            text: processMenuItem2(actions.actionOpenFilesHere.text, actionOpenFilesHere)
            action: actions.actionOpenFilesHere
        }
        MenuItem {
            id: actionVerticalSplit
            text: processMenuItem2(actions.actionVerticalSplit.text, actionVerticalSplit)
            action: actions.actionVerticalSplit
        }
        MenuItem {
            id: actionWrap
            text: processMenuItem2(actions.actionWrap.text, actionWrap)
            action: actions.actionWrap
        }
        MenuItem {
            id: actionWrapOutput
            text: processMenuItem2(actions.actionWrapOutput.text, actionWrapOutput)
            action: actions.actionWrapOutput
        }
        MenuItem {
            id: actionReadOnly
            text: processMenuItem2(actions.actionReadOnly.text, actionReadOnly)
            action: actions.actionReadOnly
        }
        MenuSeparator {}
        Menu {
            id: menuLineEndCharacters
            title: processMenuItem(qsTr("&Line End Characters"), menuLineEndCharacters)

            MenuItem {
                id: actionCrLf
                text: processMenuItem2(actions.actionCrLf.text, actionCrLf)
                action: actions.actionCrLf
            }
            MenuItem {
                id: actionCr
                text: processMenuItem2(actions.actionCr.text, actionCr)
                action: actions.actionCr
            }
            MenuItem {
                id: actionLf
                text: processMenuItem2(actions.actionLf.text, actionLf)
                action: actions.actionLf
            }
        }
        MenuItem {
            id: actionConvertLineEndChar
            text: processMenuItem2(actions.actionConvertLineEndChar.text, actionConvertLineEndChar)
            action: actions.actionConvertLineEndChar
        }
        MenuSeparator {}
/*
        Menu {
            id: languagesSubMenu
            title: processMenuItem(qsTr("Language"), languagesSubMenu)

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

                onObjectAdded: languagesSubMenu.insertItem(index, object)
                onObjectRemoved: languagesSubMenu.removeItem(object)
            }
        }
        MenuSeparator {}
*/
        MenuItem {
            id: actionChangeIndentationSettings
            text: processMenuItem2(actions.actionChangeIndentationSettings.text, actionChangeIndentationSettings)
            action: actions.actionChangeIndentationSettings
        }
        MenuItem {
            id: actionUseMonospacedFont
            text: processMenuItem2(actions.actionUseMonospacedFont.text, actionUseMonospacedFont)
            action: actions.actionUseMonospacedFont
        }
        MenuItem {
            id: actionSwitchToLastActivatedTab
            text: processMenuItem2(actions.actionSwitchToLastActivatedTab.text, actionSwitchToLastActivatedTab)
            action: actions.actionSwitchToLastActivatedTab
        }
        MenuSeparator {}
        MenuItem {
            id: actionOpenLocalOptionsFile
            text: processMenuItem2(actions.actionOpenLocalOptionsFile.text, actionOpenLocalOptionsFile)
            action: actions.actionOpenLocalOptionsFile
        }
        MenuItem {
            id: actionOpenDirectoryOptionsFile
            text: processMenuItem2(actions.actionOpenDirectoryOptionsFile.text, actionOpenDirectoryOptionsFile)
            action: actions.actionOpenDirectoryOptionsFile
        }
        MenuItem {
            id: actionOpenUserOptionsFile
            text: processMenuItem2(actions.actionOpenUserOptionsFile.text, actionOpenUserOptionsFile)
            action: actions.actionOpenUserOptionsFile
        }
        MenuItem {
            id: actionOpenGlobalOptionsFile
            text: processMenuItem2(actions.actionOpenGlobalOptionsFile.text, actionOpenGlobalOptionsFile)
            action: actions.actionOpenGlobalOptionsFile
        }
        MenuItem {
            id: actionOpenAbbreviationsFile
            text: processMenuItem2(actions.actionOpenAbbreviationsFile.text, actionOpenAbbreviationsFile)
            action: actions.actionOpenAbbreviationsFile
        }
        MenuItem {
            id: actionOpenLuaStartupScript
            text: processMenuItem2(actions.actionOpenLuaStartupScript.text, actionOpenLuaStartupScript)
            action: actions.actionOpenLuaStartupScript
        }
        MenuSeparator {
            visible: importModel.count>0
        }
        Instantiator {
            id: additionalImportItems
            model: importModel
            delegate: MenuItem {
                action: Action {
                    text: model.display
                    shortcut: model.shortcut
                    onTriggered: sciteQt.cmdCallImport(index)
                }
            }

            onObjectAdded: optionsMenu.insertItem(index+21, object)
            onObjectRemoved: optionsMenu.removeItem(object)
        }
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
            text: processMenuItem2(actions.actionBuffersPrevious.text, actionBuffersPrevious)
            action: actions.actionBuffersPrevious
        }
        MenuItem {
            id: actionBuffersNext
            text: processMenuItem2(actions.actionBuffersNext.text, actionBuffersNext)
            action: actions.actionBuffersNext
        }
        MenuItem {
            id: actionBuffersCloseAll
            text: processMenuItem2(actions.actionBuffersCloseAll.text, actionBuffersCloseAll)
            action: actions.actionBuffersCloseAll
        }
        MenuItem {
            id: actionBuffersSaveAll
            text: processMenuItem2(actions.actionBuffersSaveAll.text, actionBuffersSaveAll)
            action: actions.actionBuffersSaveAll
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
                text: processMenuItem2(actions.actionHelp.text, actionHelp2)
                action: actions.actionHelp
            }
            MenuItem {
                id: actionSciteHelp2
                text: processMenuItem2(actions.actionSciteHelp.text, actionSciteHelp2)
                action: actions.actionSciteHelp
            }
        }
*/
        MenuItem {
            id: actionHelp
            text: processMenuItem2(actions.actionHelp.text, actionHelp)
            action: actions.actionHelp
        }
        MenuItem {
            id: actionSciteHelp
            text: processMenuItem2(actions.actionSciteHelp.text, actionSciteHelp)
            action: actions.actionSciteHelp
        }
        MenuItem {
            id: actionAboutScite
            text: processMenuItem2(actions.actionAboutScite.text, actionAboutScite)
            action: actions.actionAboutScite
        }
        MenuItem {
            id: actionAboutSciteQt
            text: processMenuItem2(actions.actionAboutSciteQt.text, actionAboutSciteQt)
            action: actions.actionAboutSciteQt
        }
        MenuItem {
            id: actionAboutQt
            text: processMenuItem2(actions.actionAboutQt.text, actionAboutQt)
            action: actions.actionAboutQt
        }

        MenuSeparator {}

        MenuItem {
            id: actionAboutCurrentFile
            text: processMenuItem2(actions.actionAboutCurrentFile.text, actionAboutCurrentFile)
            action: actions.actionAboutCurrentFile
        }
        MenuItem {
            id: actionIsMobilePlatfrom
            text: processMenuItem2(actions.actionIsMobilePlatfrom.text, actionIsMobilePlatfrom)
            action: actions.actionIsMobilePlatfrom
        }

        MenuSeparator {}

        MenuItem {
            id: actionTestFunction
            text: processMenuItem2(actions.actionTestFunction.text, actionTestFunction)
            action: actions.actionTestFunction
        }
        MenuItem {
            id: actionTest2Function
            text: processMenuItem2(actions.actionTest2Function.text, actionTest2Function)
            action: actions.actionTest2Function
        }
        MenuItem {
            id: actionTest3Function
            text: processMenuItem2(actions.actionTest3Function.text, actionTest3Function)
            action: actions.actionTest3Function
        }
        MenuItem {
            id: actionTest4Function
            text: processMenuItem2(actions.actionTest4Function.text, actionTest4Function)
            action: actions.actionTest4Function
        }
        MenuItem {
            id: actionTest5Function
            text: processMenuItem2(actions.actionTest5Function.text, actionTest5Function)
            action: actions.actionTest5Function
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
                sciteQt.testFunction("extension?");
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
                readOnlyChanged(val)
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
                undoChanged(!val)
                break;
            case 202:   //IDM_REDO
                actionRedo.enabled = val
                redoChanged(!val)
                break;
            case 203:   //IDM_CUT
                actionCut.enabled = val
                copyCutChanged(!val)
                break;
            case 204:   //IDM_COPY
                actionCopy.enabled = val
                copyCutChanged(!val)
                break;
            case 205:   //IDM_PASTE
                actionPaste.enabled = val
                pasteChanged(!val)
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
                buildChanged(!val)
                break;
            case 303:  //IDM_GO
                // Bug: prevents menu item from being closed after trigger...
                // see: https://bugreports.qt.io/browse/QTBUG-69682
                Qt.callLater(function() { actionGo.enabled = val })
                runningChanged(!val)
                break;
            case 304:  //IDM_STOPEXECUTE
                actionStopExecuting.enabled = val
                runningChanged(val)
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

        onSetInLastOpenedFilesModel:         writeInMenuModel(lastOpenedFilesModel, index, txt, checked, shortcut)
        onRemoveInLastOpenedFilesModel:      removeInMenuModel(lastOpenedFilesModel, index)
        onCheckStateInLastOpenedFilesModel:  setCheckStateInMenuModel(lastOpenedFilesModel, index, checked)

        onSetInImportModel:            writeInMenuModel(importModel, index, txt, checked, shortcut)
        onRemoveInImportModel:         removeInMenuModel(importModel, index)
        onCheckStateInImportModel:     setCheckStateInMenuModel(importModel, index, checked)
    }
}
