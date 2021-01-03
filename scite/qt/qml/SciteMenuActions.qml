/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    id: root

    property alias actionNew: actionNew
    property alias actionOpen: actionOpen
    property alias actionOpenSelectedFilename: actionOpenSelectedFilename
    property alias actionRevert: actionRevert
    property alias actionClose: actionClose
    property alias actionSave: actionSave
    property alias actionSaveAs: actionSaveAs
    property alias actionSaveACopy: actionSaveACopy
    property alias actionCopyPath: actionCopyPath
    property alias actionOpenContainingFolder: actionOpenContainingFolder
    property alias actionDeleteFiles: actionDeleteFiles

    Action {
        id: actionNew
        text: qsTr("&New")
        //icon.source: "share.svg"
        shortcut: "Ctrl+N"
        onTriggered: sciteQt.cmdNew()
    }
    Action {
        id: actionOpen
        text: qsTr("&Open...")
        //icon.source: "share.svg"
        shortcut: "Ctrl+O"
        onTriggered: Qt.callLater(function() { sciteQt.cmdOpen() })   // maybe: mobilePopupMenu.dismiss()
    }
    Action {
        id: actionOpenSelectedFilename
        text: qsTr("Open Selected &Filename")
        shortcut: "Ctrl+Shift+O"
        onTriggered: sciteQt.cmdOpenSelectedFileName()
    }
    Action {
        id: actionRevert
        text: qsTr("&Revert")
        shortcut: "Ctrl+R"
        onTriggered: sciteQt.cmdRevert()
    }
    Action {
        id: actionClose
        text: qsTr("&Close")
        shortcut: "Ctrl+W"
        onTriggered: sciteQt.cmdClose()
    }
    Action {
        id: actionSave
        text: qsTr("&Save")
        shortcut: "Ctrl+S"
        onTriggered: sciteQt.cmdSave()
    }
    Action {
        id: actionSaveAs
        text: qsTr("Save &As...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdSaveAs() })
    }
    Action {
        id: actionSaveACopy
        text: qsTr("Save a &Copy...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdSaveACopy() })
    }
    Action {
        id: actionCopyPath
        text: qsTr("Copy Pat&h")
        onTriggered: sciteQt.cmdCopyPath()
    }
    Action {
        id: actionOpenContainingFolder
        text: qsTr("Open Containing Folder")
        onTriggered: sciteQt.cmdOpenContainingFolder()
    }
    Action {
        id: actionDeleteFiles
        text: qsTr("Delete Files...")
        onTriggered: sciteQt.cmdDeleteFiles()
    }

    property alias actionCodePageProperty: actionCodePageProperty
    property alias actionUtf16BigEndian: actionUtf16BigEndian
    property alias actionUtf16LittleEndian: actionUtf16LittleEndian
    property alias actionUtf8WithBOM: actionUtf8WithBOM
    property alias actionUtf8: actionUtf8

    Action {
        id: actionCodePageProperty
        text: qsTr("&Code Page Property")
        checkable: true
        checked: false
        onTriggered: {
            clearEncodingMenus()
            sciteQt.cmdCodePageProperty()
            actionCodePageProperty.checked = true
        }
    }
    Action {
        id: actionUtf16BigEndian
        text: qsTr("UTF-16 &Big Endian")
        checkable: true
        checked: false
        onTriggered: {
            clearEncodingMenus()
            sciteQt.cmdUtf16BigEndian()
            actionUtf16BigEndian.checked = true
        }
    }
    Action {
        id: actionUtf16LittleEndian
        text: qsTr("UTF-16 &Little Endian")
        checkable: true
        checked: false
        onTriggered: {
            clearEncodingMenus()
            sciteQt.cmdUtf16LittleEndian()
            actionUtf16LittleEndian.checked = true
        }
    }
    Action {
        id: actionUtf8WithBOM
        text: qsTr("UTF-8 &with BOM")
        checkable: true
        checked: false
        onTriggered: {
            clearEncodingMenus()
            sciteQt.cmdUtf8WithBOM()
            actionUtf8WithBOM.checked = true
        }
    }
    Action {
        id: actionUtf8
        text: qsTr("&UTF-8")
        checkable: true
        checked: false
        onTriggered: {
            clearEncodingMenus()
            sciteQt.cmdUtf8()
            actionUtf8.checked = true
        }
    }

    property alias actionAsHtml: actionAsHtml
    property alias actionAsRtf: actionAsRtf
    property alias actionAsPdf: actionAsPdf
    property alias actionAsLatex: actionAsLatex
    property alias actionAsXml: actionAsXml

    Action {
        id: actionAsHtml
        text: qsTr("As &HTML...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdAsHtml() })
    }
    Action {
        id: actionAsRtf
        text: qsTr("As &RTF...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdAsRtf() })
    }
    Action {
        id: actionAsPdf
        text: qsTr("As &PDF...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdAsPdf() })
    }
    Action {
        id: actionAsLatex
        text: qsTr("As &LaTeX...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdAsLatex() })
    }
    Action {
        id: actionAsXml
        text: qsTr("As &XML...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdAsXml() })
    }

    property alias actionPageSetup: actionPageSetup
    property alias actionPrint: actionPrint
    property alias actionLoadSession: actionLoadSession
    property alias actionSaveSession: actionSaveSession
    property alias actionExit: actionExit

    Action {
        id: actionPageSetup
        text: qsTr("Page Set&up...")
        onTriggered: sciteQt.cmdPageSetup()
    }
    Action {
        id: actionPrint
        text: qsTr("&Print...")
        shortcut: "Ctrl+P"
        onTriggered: sciteQt.cmdPrint()
    }
    Action {
        id: actionLoadSession
        text: qsTr("&Load Session...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdLoadSession() })
    }
    Action {
        id: actionSaveSession
        text: qsTr("Sa&ve Session...")
        onTriggered: Qt.callLater(function() { sciteQt.cmdSaveSession() })
    }
    Action {
        id: actionExit
        text: qsTr("E&xit")
        onTriggered: sciteQt.cmdExit()
    }

    property alias actionUndo: actionUndo
    property alias actionRedo: actionRedo
    property alias actionCut: actionCut
    property alias actionCopy: actionCopy
    property alias actionPaste: actionPaste
    property alias actionDuplicate: actionDuplicate
    property alias actionDelete: actionDelete
    property alias actionSelectAll: actionSelectAll
    property alias actionSelectWord: actionSelectWord
    property alias actionCopyAsRtf: actionCopyAsRtf
    property alias actionMatchBrace: actionMatchBrace
    property alias actionSelectToBrace: actionSelectToBrace
    property alias actionShowCalltip: actionShowCalltip
    property alias actionCompleteSymbol: actionCompleteSymbol
    property alias actionCompleteWord: actionCompleteWord
    property alias actionExpandAbbreviation: actionExpandAbbreviation
    property alias actionInsertAbbreviation: actionInsertAbbreviation
    property alias actionBlockComment: actionBlockComment
    property alias actionBoxComment: actionBoxComment
    property alias actionStreamComment: actionStreamComment
    property alias actionMakeSelectionUppercase: actionMakeSelectionUppercase
    property alias actionMakeSelectionLowercase: actionMakeSelectionLowercase
    property alias actionReverseSelectedLines: actionReverseSelectedLines
    property alias actionJoin: actionJoin
    property alias actionSplit: actionSplit

    Action {
        id: actionUndo
        text: qsTr("&Undo")
        shortcut: "Ctrl+Z"
        onTriggered: sciteQt.cmdUndo()
    }
    Action {
        id: actionRedo
        text: qsTr("&Redo")
        shortcut: "Ctrl+Y"
        onTriggered: sciteQt.cmdRedo()
    }
    Action {
        id: actionCut
        text: qsTr("Cu&t")
        shortcut: "Ctrl+X"
        onTriggered: sciteQt.cmdCut()
    }
    Action {
        id: actionCopy
        text: qsTr("&Copy")
        shortcut: "Ctrl+C"
        onTriggered: sciteQt.cmdCopy()
    }
    Action {
        id: actionPaste
        text: qsTr("&Paste")
        shortcut: "Ctrl+V"
        onTriggered: sciteQt.cmdPaste()
    }
    Action {
        id: actionDuplicate
        text: qsTr("Duplicat&e")
        shortcut: "Ctrl+D"
        onTriggered: sciteQt.cmdDuplicate()
    }
    Action {
        id: actionDelete
        text: qsTr("&Delete")
        shortcut: "Del"
        onTriggered: sciteQt.cmdDelete()
    }
    Action {
        id: actionSelectAll
        text: qsTr("Select &All")
        shortcut: "Ctrl+A"
        onTriggered: sciteQt.cmdSelectAll()
    }
    Action {
        id: actionSelectWord
        text: qsTr("Select &Word")
        shortcut: "Ctrl+T"
        onTriggered: sciteQt.cmdSelectWord()
    }
    Action {
        id: actionCopyAsRtf
        text: qsTr("Copy as RT&F")
        onTriggered: sciteQt.cmdCopyAsRtf()
    }
    Action {
        id: actionMatchBrace
        text: qsTr("Match &Brace")
        shortcut: "Ctrl+E"
        onTriggered: sciteQt.cmdMatchBrace()
    }
    Action {
        id: actionSelectToBrace
        text: qsTr("Select t&o Brace")
        shortcut: "Ctrl+Shift+E"
        onTriggered: sciteQt.cmdSelectToBrace()
    }
    Action {
        id: actionShowCalltip
        text: qsTr("S&how Calltip")
        shortcut: "Ctrl+Shift+Space"
        onTriggered: sciteQt.cmdShowCalltip()
    }
    Action {
        id: actionCompleteSymbol
        text: qsTr("Complete S&ymbol")
        shortcut: "Ctrl+I"
        onTriggered: sciteQt.cmdCompleteSymbol()
    }
    Action {
        id: actionCompleteWord
        text: qsTr("Complete &Word")
        shortcut: "Ctrl+Enter"
        onTriggered: sciteQt.cmdCompleteWord()
    }
    Action {
        id: actionExpandAbbreviation
        text: qsTr("Expand Abbre&viation")
        shortcut: "Ctrl+B"
        onTriggered: sciteQt.cmdExpandAbbreviation()
    }
    Action {
        id: actionInsertAbbreviation
        text: qsTr("&Insert Abbreviation")
        shortcut: "Ctrl+Shift+R"
        onTriggered: Qt.callLater(function() { sciteQt.cmdInsertAbbreviation() })
    }
    Action {
        id: actionBlockComment
        text: qsTr("Block Co&mment or Uncomment")
        shortcut: "Ctrl+Q"
        onTriggered: sciteQt.cmdBlockComment()
    }
    Action {
        id: actionBoxComment
        text: qsTr("Bo&x Comment")
        shortcut: "Ctrl+Shift+B"
        onTriggered: sciteQt.cmdBoxComment()
    }
    Action {
        id: actionStreamComment
        text: qsTr("Stream Comme&nt")
        shortcut: "Ctrl+Shift+Q"
        onTriggered: sciteQt.cmdStreamComment()
    }
    Action {
        id: actionMakeSelectionUppercase
        text: qsTr("Make &Selection Uppercase")
        shortcut: "Ctrl+Shift+U"
        onTriggered: sciteQt.cmdMakeSelectionUppercase()
    }
    Action {
        id: actionMakeSelectionLowercase
        text: qsTr("Make Selection &Lowercase")
        shortcut: "Ctrl+U"
        onTriggered: sciteQt.cmdMakeSelectionLowercase()
    }
    Action {
        id: actionReverseSelectedLines
        text: qsTr("Reverse Selected Lines")
        onTriggered: sciteQt.cmdReverseSelectedLines()
    }

    Action {
        id: actionJoin
        text: qsTr("&Join")
        onTriggered: sciteQt.cmdJoin()
    }
    Action {
        id: actionSplit
        text: qsTr("&Split")
        onTriggered: sciteQt.cmdSplit()
    }

    property alias actionFind: actionFind
    property alias actionFindNext: actionFindNext
    property alias actionFindPrevious: actionFindPrevious
    property alias actionFindInFiles: actionFindInFiles
    property alias actionReplace: actionReplace
    property alias actionIncrementalSearch: actionIncrementalSearch
    property alias actionSelectionAddNext: actionSelectionAddNext
    property alias actionSelectionAddEach: actionSelectionAddEach
    property alias actionGoto: actionGoto
    property alias actionNextBookmark: actionNextBookmark
    property alias actionPreviousBookmark: actionPreviousBookmark
    property alias actionToggleBookmark: actionToggleBookmark
    property alias actionClearAllBookmarks: actionClearAllBookmarks
    property alias actionSelectAllBookmarks: actionSelectAllBookmarks

    Action {
        id: actionFind
        text: qsTr("&Find...")
        shortcut: "Ctrl+F"
        onTriggered: sciteQt.cmdFind()
    }
    Action {
        id: actionFindNext
        text: qsTr("Find &Next")
        shortcut: "F3"
        onTriggered: sciteQt.cmdFindNext()
    }
    Action {
        id: actionFindPrevious
        text: qsTr("Find Previou&s")
        shortcut: "Shift+F3"
        onTriggered: sciteQt.cmdFindPrevious()
    }
    Action {
        id: actionFindInFiles
        text: sciteQt.findInFilesRunning ? qsTr("Stop F&ind in Files") : qsTr("F&ind in Files...")
        shortcut: "Ctrl+Shift+F"
        onTriggered: sciteQt.cmdFindInFiles()
    }
    Action {
        id: actionReplace
        text: qsTr("R&eplace...")
        shortcut: "Ctrl+H"
        onTriggered: sciteQt.cmdReplace()
    }
    Action {
        id: actionIncrementalSearch
        text: qsTr("Incrementa&l Search...")
        shortcut: "Ctrl+Alt+I"
        onTriggered: sciteQt.cmdIncrementalSearch()
    }
    Action {
        id: actionSelectionAddNext
        text: qsTr("Selection A&dd Next")
        shortcut: "Ctrl+Shift+D"
        onTriggered: sciteQt.cmdSelectionAddNext()
    }
    Action {
        id: actionSelectionAddEach
        text: qsTr("Selection &Add Each")
        shortcut: "Ctrl+Shift+A"
        onTriggered: sciteQt.cmdSelectionAddEach()
    }
    Action {
        id: actionGoto
        text: qsTr("&Go to...")
        shortcut: "Ctrl+G"
        onTriggered: sciteQt.cmdGoto()
    }
    Action {
        id: actionNextBookmark
        text: qsTr("Next Book&mark")
        shortcut: "F2"
        onTriggered: sciteQt.cmdNextBookmark()
    }
    Action {
        id: actionPreviousBookmark
        text: qsTr("Pre&vious Bookmark")
        shortcut: "Shift+F2"
        onTriggered: sciteQt.cmdPreviousBookmark()
    }
    Action {
        id: actionToggleBookmark
        text: qsTr("Toggle Bookmar&k")
        shortcut: "Ctrl+F2"
        onTriggered: sciteQt.cmdToggleBookmark()
    }
    Action {
        id: actionClearAllBookmarks
        text: qsTr("&Clear All Bookmarks")
        onTriggered: sciteQt.cmdClearAllBookmarks()
    }
    Action {
        id: actionSelectAllBookmarks
        text: qsTr("Select All &Bookmarks")
        onTriggered: sciteQt.cmdSelectAllBookmarks()
    }

    property alias actionToggleCurrentFold: actionToggleCurrentFold
    property alias actionToggleAllFolds: actionToggleAllFolds
    property alias actionFullScreen: actionFullScreen
    property alias actionShowToolBar: actionShowToolBar
    property alias actionShowTabBar: actionShowTabBar
    property alias actionShowStatusBar: actionShowStatusBar
    property alias actionShowWhitespace: actionShowWhitespace
    property alias actionShowEndOfLine: actionShowEndOfLine
    property alias actionIndentaionGuides: actionIndentaionGuides
    property alias actionLineNumbers: actionLineNumbers
    property alias actionMargin: actionMargin
    property alias actionFoldMargin: actionFoldMargin
    property alias actionToggleOutput: actionToggleOutput
    property alias actionParameters: actionParameters

    Action {
        id: actionToggleCurrentFold
        text: qsTr("Toggle &current fold")
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdToggleCurrentFold()
    }
    Action {
        id: actionToggleAllFolds
        text: qsTr("Toggle &all folds")
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdToggleAllFolds()
    }
    Action {
        id: actionFullScreen
        text: qsTr("Full Scree&n")
        shortcut: "F11"
        enabled: false
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdFullScreen()
    }
    Action {
        id: actionShowToolBar
        text: qsTr("&Tool Bar")
        checkable: true
        checked: sciteQt.showToolBar
        onTriggered: sciteQt.cmdShowToolBar()
    }
    Action {
        id: actionShowTabBar
        text: qsTr("Tab &Bar")
        checkable: true
        checked: sciteQt.showTabBar
        onTriggered: sciteQt.cmdShowTabBar()
    }
    Action {
        id: actionShowStatusBar
        text: qsTr("&Status Bar")
        checkable: true
        checked: sciteQt.showStatusBar
        onTriggered: sciteQt.cmdShowStatusBar()
    }
    Action {
        id: actionShowWhitespace
        text: qsTr("&Whitespace")
        shortcut: "Ctrl+Shift+8"
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdShowWhitespace()
    }
    Action {
        id: actionShowEndOfLine
        text: qsTr("&End of Line")
        shortcut: "Ctrl+Shift+9"
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdShowEndOfLine()
    }
    Action {
        id: actionIndentaionGuides
        text: qsTr("&Indentation Guides")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdIndentionGuides()
    }
    Action {
        id: actionLineNumbers
        text: qsTr("Line &Numbers")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdLineNumbers()
    }
    Action {
        id: actionMargin
        text: qsTr("&Margin")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdMargin()
    }
    Action {
        id: actionFoldMargin
        text: qsTr("&Fold Margin")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdFoldMargin()
    }
    Action {
        id: actionToggleOutput
        text: qsTr("&Output")
        shortcut: "F8"
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdToggleOutput()
    }
    Action {
        id: actionParameters
        text: qsTr("&Parameters")
        shortcut: "Shift+F8"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdParameters()
    }

    property alias actionCompile: actionCompile
    property alias actionBuild: actionBuild
    property alias actionClean: actionClean
    property alias actionGo: actionGo
    property alias actionStopExecuting: actionStopExecuting
    property alias actionNextMessage: actionNextMessage
    property alias actionPreviousMessage: actionPreviousMessage
    property alias actionClearOutput: actionClearOutput
    property alias actionSwitchPane: actionSwitchPane

    Action {
        id: actionCompile
        text: qsTr("&Compile")
        shortcut: "Ctrl+F7"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdCompile()
    }
    Action {
        id: actionBuild
        text: qsTr("&Build")
        shortcut: "F7"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdBuild()
    }
    Action {
        id: actionClean
        text: qsTr("&Clean")
        shortcut: "Shift+F7"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdClean()
    }
    Action {
        id: actionGo
        text: qsTr("&Go")
        shortcut: "F5"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdGo()
    }
    Action {
        id: actionStopExecuting
        text: qsTr("&Stop Executing")
        shortcut: "Ctrl+Break"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdStopExecuting()
    }
    MenuSeparator {}
    Action {
        id: actionNextMessage
        text: qsTr("&Next Message")
        shortcut: "F4"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdNextMessage()
    }
    Action {
        id: actionPreviousMessage
        text: qsTr("&Previous Message")
        shortcut: "Shift+F4"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdPreviousMessage()
    }
    Action {
        id: actionClearOutput
        text: qsTr("Clear &Output")
        shortcut: "Shift+F5"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdClearOutput()
    }
    Action {
        id: actionSwitchPane
        text: qsTr("&Switch Pane")
        shortcut: "Ctrl+F6"
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdSwitchPane()
    }

    property alias  actionAlwaysOnTop: actionAlwaysOnTop
    property alias  actionOpenFilesHere: actionOpenFilesHere
    property alias  actionVerticalSplit: actionVerticalSplit
    property alias  actionWrap: actionWrap
    property alias  actionWrapOutput: actionWrapOutput
    property alias  actionReadOnly: actionReadOnly
    property alias  actionCrLf: actionCrLf
    property alias  actionCr: actionCr
    property alias  actionLf: actionLf
    property alias  actionConvertLineEndChar: actionConvertLineEndChar
    property alias  actionChangeIndentationSettings: actionChangeIndentationSettings
    property alias  actionUseMonospacedFont: actionUseMonospacedFont
    property alias  actionSwitchToLastActivatedTab: actionSwitchToLastActivatedTab
    property alias  actionOpenLocalOptionsFile: actionOpenLocalOptionsFile
    property alias  actionOpenDirectoryOptionsFile: actionOpenDirectoryOptionsFile
    property alias  actionOpenUserOptionsFile: actionOpenUserOptionsFile
    property alias  actionOpenGlobalOptionsFile: actionOpenGlobalOptionsFile
    property alias  actionOpenAbbreviationsFile: actionOpenAbbreviationsFile
    property alias  actionOpenLuaStartupScript: actionOpenLuaStartupScript

    Action {
        id: actionAlwaysOnTop
        text: qsTr("&Always On Top")
        checkable: true
        checked: false
        enabled: false
        onTriggered: sciteQt.cmdAlwaysOnTop()
    }
    Action {
        id: actionOpenFilesHere
        text: qsTr("Open Files &Here")
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdOpenFilesHere()
    }
    Action {
        id: actionVerticalSplit
        text: qsTr("Vertical &Split")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdVerticalSplit()
    }
    Action {
        id: actionWrap
        text: qsTr("&Wrap")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdWrap()
    }
    Action {
        id: actionWrapOutput
        text: qsTr("Wrap &Output")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdWrapOutput()
    }
    Action {
        id: actionReadOnly
        text: qsTr("&Read-Only")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdReadOnly()
    }
    Action {
        id: actionCrLf
        text: qsTr("CR &+ LF")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdCrLf()
    }
    Action {
        id: actionCr
        text: qsTr("&CR")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdCr()
    }
    Action {
        id: actionLf
        text: qsTr("&LF")
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdLf()
    }
    Action {
        id: actionConvertLineEndChar
        text: qsTr("&Convert Line End Characters")
        //checkable: true
        //checked: false
        onTriggered: sciteQt.cmdConvertLineEndChar()
    }
    Action {
        id: actionChangeIndentationSettings
        text: qsTr("Change Inden&tation Settings")
        shortcut: "Ctrl+Shift+I"
        onTriggered: sciteQt.cmdChangeIndentationSettings()
    }
    Action {
        id: actionUseMonospacedFont
        text: qsTr("Use &Monospaced Font")
        shortcut: "Ctrl+F11"
        checkable: true
        checked: false
        onTriggered: sciteQt.cmdUseMonospacedFont()
    }
    Action {
        id: actionSwitchToLastActivatedTab
        text: qsTr("Switch to last acti&vated tab")
        shortcut: "Ctrl+Tab"
        onTriggered: sciteQt.cmdSwitchToLastActivatedTab()
    }
    Action {
        id: actionOpenLocalOptionsFile
        text: qsTr("Open Local &Options File")
        onTriggered: sciteQt.cmdOpenLocalOptionsFile()
    }
    Action {
        id: actionOpenDirectoryOptionsFile
        text: qsTr("Open &Directory Options File")
        onTriggered: sciteQt.cmdOpenDirectoryOptionsFile()
    }
    Action {
        id: actionOpenUserOptionsFile
        text: qsTr("Open &User Options File")
        onTriggered: sciteQt.cmdOpenUserOptionsFile()
    }
    Action {
        id: actionOpenGlobalOptionsFile
        text: qsTr("Open &Global Options File")
        onTriggered: sciteQt.cmdOpenGlobalOptionsFile()
    }
    Action {
        id: actionOpenAbbreviationsFile
        text: qsTr("Open A&bbreviations File")
        onTriggered: sciteQt.cmdOpenAbbreviationsFile()
    }
    Action {
        id: actionOpenLuaStartupScript
        text: qsTr("Open Lua Startup Scr&ipt")
        onTriggered: sciteQt.cmdOpenLuaStartupScript()
    }

    property alias actionBuffersPrevious: actionBuffersPrevious
    property alias actionBuffersNext: actionBuffersNext
    property alias actionBuffersCloseAll: actionBuffersCloseAll
    property alias actionBuffersSaveAll: actionBuffersSaveAll

    Action {
        id: actionBuffersPrevious
        text: qsTr("&Previous")
        shortcut: "Shift+F6"
        onTriggered: sciteQt.cmdBuffersPrevious()
    }
    Action {
        id: actionBuffersNext
        text: qsTr("&Next")
        shortcut: "F6"
        onTriggered: sciteQt.cmdBuffersNext()
    }
    Action {
        id: actionBuffersCloseAll
        text: qsTr("&Close All")
        onTriggered: sciteQt.cmdBuffersCloseAll()
    }
    Action {
        id: actionBuffersSaveAll
        text: qsTr("&Save All")
        onTriggered: sciteQt.cmdBuffersSaveAll()
    }

    property alias actionHelp: actionHelp
    property alias actionSciteHelp: actionSciteHelp
    property alias actionAboutScite: actionAboutScite
    property alias actionAboutSciteQt: actionAboutSciteQt
    property alias actionAboutQt: actionAboutQt
    property alias actionAboutCurrentFile: actionAboutCurrentFile
    property alias actionRunCurrentAsJavaScript: actionRunCurrentAsJavaScript
    property alias actionIsMobilePlatfrom: actionIsMobilePlatfrom

    Action {
        id: actionHelp
        shortcut: "F1"
        text: qsTr("&Help")
        onTriggered: sciteQt.cmdHelp()
    }
    Action {
        id: actionSciteHelp
        text: qsTr("&SciTE Help")
        onTriggered: sciteQt.cmdSciteHelp()
    }
    Action {
        id: actionAboutScite
        text: qsTr("&About SciTE")
        onTriggered: sciteQt.cmdAboutScite()
    }
    Action {
        id: actionAboutSciteQt
        text: qsTr("About SciTE &Qt")
        onTriggered: sciteQt.cmdAboutSciteQt()
    }
    Action {
        id: actionAboutQt
        text: qsTr("About Q&t")
        enabled: !sciteQt.isWebassemblyPlatform()
        onTriggered: sciteQt.cmdAboutQt()
    }

    Action {
        id: actionAboutCurrentFile
        text: qsTr("About &Current File")
        onTriggered: sciteQt.cmdAboutCurrentFile()
    }

    Action {
        id: actionRunCurrentAsJavaScript
        text: qsTr("Run Current File As &JavaScript")
        shortcut: "Ctrl+J"
        onTriggered: sciteQt.cmdRunCurrentAsJavaScriptFile()
    }

    Action {
        id: actionIsMobilePlatfrom
        text: qsTr("Mobile Platform UI")
        checkable: true
        checked: sciteQt.mobileUI
        onTriggered: {
            sciteQt.mobileUI = !sciteQt.mobileUI
        }
    }

/* for debugging only:

    property alias actionTestFunction: actionTestFunction
    property alias actionTest2Function: actionTest2Function
    property alias actionTest3Function: actionTest3Function
    property alias actionTest4Function: actionTest4Function
    property alias actionTest5Function: actionTest5Function

    Action {
        id: actionTestFunction
        text: qsTr("Html Open")
        onTriggered: applicationWindow.htmlOpen() //applicationWindow.showAboutSciteDialog()
    }
    Action {
        id: actionTest2Function
        text: qsTr("Html Save")
        onTriggered: applicationWindow.htmlSave() //applicationWindow.showAboutSciteDialog()
    }
    Action {
        id: actionTest3Function
        text: qsTr("Test Dialog")
        onTriggered: applicationWindow.showTestDialog()
    }
    Action {
        id: actionTest4Function
        text: qsTr("Open Mobile")
        onTriggered: applicationWindow.openViaMobileFileDialog()
    }
    Action {
        id: actionTest5Function
        text: qsTr("Save Mobile")
        onTriggered: applicationWindow.saveViaMobileFileDialog()
    }
*/
    // ************************************************

    function handeEolMenus(enumEol) {
        actionCrLf.checked = false
        actionCr.checked = false
        actionLf.checked = false
        switch(enumEol) {   // see: enum class EndOfLine in ScintillaTypes.h
            case 0:
                actionCrLf.checked = true
                break;
            case 1:
                actionCr.checked = true
                break;
            case 2:
                actionLf.checked = true
                break;
        }
    }

    function clearEncodingMenus() {
        actionCodePageProperty.checked = false
        actionUtf16BigEndian.checked = false
        actionUtf16LittleEndian.checked = false
        actionUtf8WithBOM.checked = false
        actionUtf8.checked = false
    }

    function handleEncodingMenus(enumEncoding) {
        clearEncodingMenus()
        //console.log("handleEncodingMenus: "+enumEncoding+" "+root)
        switch(enumEncoding) {  // see: enum UniMode in Cookie.h
            // 	uni8Bit = 0, uni16BE = 1, uni16LE = 2, uniUTF8 = 3,
            // uniCookie = 4
            case 0:
                actionCodePageProperty.checked = true
                break;
            case 1:
                actionUtf16BigEndian.checked = true
                break;
            case 2:
                actionUtf16LittleEndian.checked = true
                break;
            case 3:
                actionUtf8WithBOM.checked = true
                break;
            case 4:
                actionUtf8.checked = true
                break;
        }
    }

    Connections {
        target: sciteQt

        onUpdateEolMenus:             handeEolMenus(enumEol)
        onUpdateEncodingMenus:        handleEncodingMenus(enumEncoding)
    }
}
