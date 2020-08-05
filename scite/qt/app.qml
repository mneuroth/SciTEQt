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
        sciteQt.CmdExit()
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

    function showFind(text) {
        findInput.text = text
        findInput.visible = true
        findInput.focus = true
    }

    function setVerticalSplit(verticalSplit) {
        splitView.verticalSplit = verticalSplit
    }

    function setOutputHeight(heightOutput) {
        splitView.outputHeight = heightOutput
    }

    function processMenuItem(menuText, menuItem) {
        var s = sciteQt.getLocalisedText(menuText)
        if( menuItem !== null && menuItem.shortcut !== undefined)
        {
            s += " \t" + menuItem.shortcut
        }
        return s
    }

    function hideFindRow() {
        quickScintillaEditor.focus = true
        findInput.visible = false
    }

    menuBar: MenuBar {
        id: menuBar

        AutoSizingMenu {
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
                id: actionOpenSelectedFilename
                text: processMenuItem(qsTr("Open Selected &Filename"), actionOpenSelectedFilename)
                shortcut: "Ctrl+Shift+O"
                onTriggered: sciteQt.CmdOpenSelectedFileName()
            }
            Action {
                id: actionRevert
                text: processMenuItem(qsTr("&Revert"), actionRevert)
                shortcut: "Ctrl+R"
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
            Menu {
                id: menuEncoding
                title: processMenuItem(qsTr("Encodin&g"), menuEncoding)

                Action {
                    id: actionCodePageProperty
                    text: processMenuItem(qsTr("&Code Page Property"), actionCodePageProperty)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdCodePageProperty()
                }
                Action {
                    id: actionUtf16BigEndian
                    text: processMenuItem(qsTr("UTF-16 &Big Endian"), actionUtf16BigEndian)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdUtf16BigEndian()
                }
                Action {
                    id: actionUtf16LittleEndian
                    text: processMenuItem(qsTr("UTF-16 &Little Endian"), actionUtf16LittleEndian)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdUtf16LittleEndian()
                }
                Action {
                    id: actionUtf8WithBOM
                    text: processMenuItem(qsTr("UTF-8 &with BOM"), actionUtf8WithBOM)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdUtf8WithBOM()
                }
                Action {
                    id: actionUtf8
                    text: processMenuItem(qsTr("&UTF-8"), actionUtf8)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdUtf8()
                }
            }
            Menu {
                id: menuExport
                title: processMenuItem(qsTr("&Export"), menuExport)

                Action {
                    id: actionAsHtml
                    text: processMenuItem(qsTr("As &HTML..."), actionAsHtml)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdAsHtml()
                }
                Action {
                    id: actionAsRtf
                    text: processMenuItem(qsTr("As &RTF..."), actionAsRtf)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdAsRtf()
                }
                Action {
                    id: actionAsPdf
                    text: processMenuItem(qsTr("As &PDF..."), actionAsPdf)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdAsPdf()
                }
                Action {
                    id: actionAsLatex
                    text: processMenuItem(qsTr("As &LaTeX..."), actionAsLatex)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdAsLatex()
                }
                Action {
                    id: actionAsXml
                    text: processMenuItem(qsTr("As &XML..."), actionAsXml)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdAsXml()
                }
            }
            MenuSeparator {}
            Action {
                id: actionPageSetup
                text: processMenuItem(qsTr("Page Set&up..."), actionPageSetup)
                onTriggered: sciteQt.CmdPageSetup()
            }
            Action {
                id: actionPrint
                text: processMenuItem(qsTr("&Print..."), actionPrint)
                shortcut: "Ctrl+P"
                onTriggered: sciteQt.CmdPrint()
            }
            MenuSeparator {}
            Action {
                id: actionLoadSession
                text: processMenuItem(qsTr("&Load Session..."), actionLoadSession)
                onTriggered: sciteQt.CmdLoadSession()
            }
            Action {
                id: actionSaveSession
                text: processMenuItem(qsTr("Sa&ve Session..."), actionSaveSession)
                onTriggered: sciteQt.CmdSaveSession()
            }
            MenuSeparator {}
            Action {
                id: actionExit
                text: processMenuItem(qsTr("E&xit"), actionExit)
                onTriggered: sciteQt.CmdExit()
            }
        }

        AutoSizingMenu {
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
            Action {
                id: actionDuplicate
                text: processMenuItem(qsTr("Duplicat&e"), actionDuplicate)
                shortcut: "Ctrl+D"
                onTriggered: sciteQt.CmdDuplicate()
            }
            Action {
                id: actionDelete
                text: processMenuItem(qsTr("&Delete"), actionDelete)
                shortcut: "Del"
                onTriggered: sciteQt.CmdDelete()
            }
            Action {
                id: actionSelectAll
                text: processMenuItem(qsTr("Select &All"), actionSelectAll)
                shortcut: "Ctrl+A"
                onTriggered: sciteQt.CmdSelectAll()
            }
            Action {
                id: actionCopyAsRtf
                text: processMenuItem(qsTr("Copy as RT&F"), actionCopyAsRtf)
                onTriggered: sciteQt.CmdCopyAsRtf()
            }
            MenuSeparator {}
            Action {
                id: actionMatchBrace
                text: processMenuItem(qsTr("Match &Brace"), actionMatchBrace)
                shortcut: "Ctrl+E"
                onTriggered: sciteQt.CmdMatchBrace()
            }
            Action {
                id: actionSelectToBrace
                text: processMenuItem(qsTr("Select t&o Brace"), actionSelectToBrace)
                shortcut: "Ctrl+Shift+E"
                onTriggered: sciteQt.CmdSelectToBrace()
            }
            Action {
                id: actionShowCalltip
                text: processMenuItem(qsTr("S&how Calltip"), actionShowCalltip)
                shortcut: "Ctrl+Shift+Space"
                onTriggered: sciteQt.CmdShowCalltip()
            }
            Action {
                id: actionCompleteSymbol
                text: processMenuItem(qsTr("Complete S&ymbol"), actionCompleteSymbol)
                shortcut: "Ctrl+I"
                onTriggered: sciteQt.CmdCompleteSymbol()
            }
            Action {
                id: actionCompleteWord
                text: processMenuItem(qsTr("Complete &Word"), actionCompleteWord)
                shortcut: "Ctrl+Enter"
                onTriggered: sciteQt.CmdCompleteWord()
            }
            Action {
                id: actionExpandAbbreviation
                text: processMenuItem(qsTr("Expand Abbre&viation"), actionExpandAbbreviation)
                shortcut: "Ctrl+B"
                onTriggered: sciteQt.CmdExpandAbbreviation()
            }
            Action {
                id: actionInsertAbbreviation
                text: processMenuItem(qsTr("&Insert Abbreviation"), actionInsertAbbreviation)
                shortcut: "Ctrl+Shift+R"
                onTriggered: sciteQt.CmdInsertAbbreviation()
            }
            Action {
                id: actionBlockComment
                text: processMenuItem(qsTr("Block Co&mment or Uncomment"), actionBlockComment)
                shortcut: "Ctrl+Q"
                onTriggered: sciteQt.CmdBlockComment()
            }
            Action {
                id: actionBoxComment
                text: processMenuItem(qsTr("Bo&x Comment"), actionBoxComment)
                shortcut: "Ctrl+Shift+B"
                onTriggered: sciteQt.CmdBoxComment()
            }
            Action {
                id: actionStreamComment
                text: processMenuItem(qsTr("Stream Comme&nt"), actionStreamComment)
                shortcut: "Ctrl+Shift+Q"
                onTriggered: sciteQt.CmdStreamComment()
            }
            Action {
                id: actionMakeSelectionUppercase
                text: processMenuItem(qsTr("Make &Selection Uppercase"), actionMakeSelectionUppercase)
                shortcut: "Ctrl+Shift+U"
                onTriggered: sciteQt.CmdMakeSelectionUppercase()
            }
            Action {
                id: actionMakeSelectionLowercase
                text: processMenuItem(qsTr("Make Selection &Lowercase"), actionMakeSelectionLowercase)
                shortcut: "Ctrl+U"
                onTriggered: sciteQt.CmdMakeSelectionLowercase()
            }
            Action {
                id: actionReverseSelectedLines
                text: processMenuItem(qsTr("Reverse Selected Lines"), actionReverseSelectedLines)
                onTriggered: sciteQt.CmdReverseSelectedLines()
            }
            Menu {
                id: menuParagraph
                title: processMenuItem(qsTr("Para&graph"), menuParagraph)

                Action {
                    id: actionJoin
                    text: processMenuItem(qsTr("&Join"), actionJoin)
                    onTriggered: sciteQt.CmdJoin()
                }
                Action {
                    id: actionSplit
                    text: processMenuItem(qsTr("&Split"), actionSplit)
                    onTriggered: sciteQt.CmdSplit()
                }
            }
        }

        AutoSizingMenu {
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
                text: processMenuItem(qsTr("Find Previou&s"), actionFindPrevious)
                shortcut: "Shift+F3"
                onTriggered: sciteQt.CmdFindPrevious()
            }
            Action {
                id: actionFindInFiles
                text: processMenuItem(qsTr("F&ind in Files..."), actionFindInFiles)
                shortcut: "Ctrl+Shift+F"
                onTriggered: sciteQt.CmdFindInFiles()
            }
            Action {
                id: actionReplace
                text: processMenuItem(qsTr("R&eplace..."), actionReplace)
                shortcut: "Ctrl+H"
                onTriggered: sciteQt.CmdReplace()
            }
            Action {
                id: actionIncrementalSearch
                text: processMenuItem(qsTr("Incrementa&l Search..."), actionIncrementalSearch)
                shortcut: "Ctrl+Alt+I"
                onTriggered: sciteQt.CmdIncrementalSearch()
            }
            Action {
                id: actionSelectionAddNext
                text: processMenuItem(qsTr("Selection A&dd Next"), actionSelectionAddNext)
                shortcut: "Ctrl+Shift+D"
                onTriggered: sciteQt.CmdSelectionAddNext()
            }
            Action {
                id: actionSelectionAddEach
                text: processMenuItem(qsTr("Selection &Add Each"), actionSelectionAddEach)
                onTriggered: sciteQt.CmdSelectionAddEach()
            }
            MenuSeparator {}
            Action {
                id: actionGoto
                text: processMenuItem(qsTr("&Go to..."), actionGoto)
                shortcut: "Ctrl+G"
                onTriggered: sciteQt.CmdGoto()
            }
            Action {
                id: actionNextBookmark
                text: processMenuItem(qsTr("&Next Book&mark"), actionNextBookmark)
                shortcut: "F2"
                onTriggered: sciteQt.CmdNextBookmark()
            }
            Action {
                id: actionPreviousBookmark
                text: processMenuItem(qsTr("Pre&vious Bookmark"), actionPreviousBookmark)
                shortcut: "Shift+F2"
                onTriggered: sciteQt.CmdPreviousBookmark()
            }
            Action {
                id: actionToggleBookmark
                text: processMenuItem(qsTr("Toggle Bookmar&k"), actionToggleBookmark)
                shortcut: "Ctrl+F2"
                onTriggered: sciteQt.CmdToggleBookmark()
            }
            Action {
                id: actionClearAllBookmarks
                text: processMenuItem(qsTr("&Clear All Bookmarks"), actionClearAllBookmarks)
                onTriggered: sciteQt.CmdClearAllBookmarks()
            }
            Action {
                id: actionSelectAllBookmarks
                text: processMenuItem(qsTr("Select All &Bookmarks"), actionSelectAllBookmarks)
                onTriggered: sciteQt.CmdSelectAllBookmarks()
            }
        }

        AutoSizingMenu {
            id: viewMenu
            title: processMenuItem(qsTr("&View"),null)

            Action {
                id: actionToggleCurrentFold
                text: processMenuItem(qsTr("Toggle &current fold"), actionToggleCurrentFold)
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdToggleCurrentFold()
            }
            Action {
                id: actionToggleAllFolds
                text: processMenuItem(qsTr("Toggle &all folds"), actionToggleAllFolds)
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdToggleAllFolds()
            }
            MenuSeparator {}
            Action {
                id: actionFullScreen
                text: processMenuItem(qsTr("Full Scree&n"), actionFullScreen)
                shortcut: "F11"
                enabled: false
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdFullScreen()
            }
            Action {
                id: actionShowToolBar
                text: processMenuItem(qsTr("&Tool Bar"), actionShowToolBar)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdShowToolBar()
            }
            Action {
                id: actionShowTabBar
                text: processMenuItem(qsTr("Tab &Bar"), actionShowTabBar)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdShowTabBar()
            }
            Action {
                id: actionShowStatusBar
                text: processMenuItem(qsTr("&Status Bar"), actionShowStatusBar)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdShowStatusBar()
            }
            MenuSeparator {}
            Action {
                id: actionShowWhitespace
                text: processMenuItem(qsTr("&Whitespace"), actionShowWhitespace)
                shortcut: "Ctrl+Shift+8"
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdShowWhitespace()
            }
            Action {
                id: actionShowEndOfLine
                text: processMenuItem(qsTr("&End of Line"), actionShowEndOfLine)
                shortcut: "Ctrl+Shift+9"
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdShowEndOfLine()
            }
            Action {
                id: actionIndentaionGuides
                text: processMenuItem(qsTr("&Indentation Guides"), actionIndentaionGuides)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdIndentionGuides()
            }
            Action {
                id: actionLineNumbers
                text: processMenuItem(qsTr("Line &Numbers"), actionLineNumbers)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdLineNumbers()
            }
            Action {
                id: actionMargin
                text: processMenuItem(qsTr("&Margin"), actionMargin)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdMargin()
            }
            Action {
                id: actionFoldMargin
                text: processMenuItem(qsTr("&Fold Margin"), actionFoldMargin)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdFoldMargin()
            }
            Action {
                id: actionToggleOutput
                text: processMenuItem(qsTr("&Output"), actionToggleOutput)
                shortcut: "F8"
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdToggleOutput()
            }
            Action {
                id: actionParameters
                text: processMenuItem(qsTr("&Parameters"), actionParameters)
                shortcut: "Shift+F8"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdParameters()
            }
        }

        AutoSizingMenu {
            id: toolsMenu
            title: processMenuItem(qsTr("Tools"),null)

            Action {
                id: actionCompile
                text: processMenuItem(qsTr("&Compile"), actionCompile)
                shortcut: "Ctrl+F7"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdCompile()
            }
            Action {
                id: actionBuild
                text: processMenuItem(qsTr("&Build"), actionBuild)
                shortcut: "F7"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdBuild()
            }
            Action {
                id: actionClean
                text: processMenuItem(qsTr("&Clean"), actionClean)
                shortcut: "Shift+F7"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdClean()
            }
            Action {
                id: actionGo
                text: processMenuItem(qsTr("&Go"), actionGo)
                shortcut: "F5"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdGo()
            }
            Action {
                id: actionStopExecuting
                text: processMenuItem(qsTr("&Stop Executing"), actionStopExecuting)
                shortcut: "Ctrl+Break"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdStopExecuting()
            }
            MenuSeparator {}
            Action {
                id: actionNextMessage
                text: processMenuItem(qsTr("&Next Message"), actionNextMessage)
                shortcut: "F4"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdNextMessage()
            }
            Action {
                id: actionPreviousMessage
                text: processMenuItem(qsTr("&Previous Message"), actionPreviousMessage)
                shortcut: "Shift+F4"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdPreviousMessage()
            }
            Action {
                id: actionClearOutput
                text: processMenuItem(qsTr("Clear &Output"), actionClearOutput)
                shortcut: "Shift+F5"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdClearOutput()
            }
            Action {
                id: actionSwitchPane
                text: processMenuItem(qsTr("&Switch Pane"), actionSwitchPane)
                shortcut: "Ctrl+F6"
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdSwitchPane()
            }
        }

        AutoSizingMenu {
            id: optionsMenu
            title: processMenuItem(qsTr("Options"),null)

            Action {
                id: actionAlwaysOnTop
                text: processMenuItem(qsTr("&Always On Top"), actionAlwaysOnTop)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdAlwaysOnTop()
            }
            Action {
                id: actionOpenFilesHere
                text: processMenuItem(qsTr("Open Files &Here"), actionOpenFilesHere)
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdOpenFilesHere()
            }
            Action {
                id: actionVerticalSplit
                text: processMenuItem(qsTr("Vertical &Split"), actionVerticalSplit)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdVerticalSplit()
            }
            Action {
                id: actionWrap
                text: processMenuItem(qsTr("&Wrap"), actionWrap)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdWrap()
            }
            Action {
                id: actionWrapOutput
                text: processMenuItem(qsTr("Wrap &Output"), actionWrapOutput)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdWrapOutput()
            }
            Action {
                id: actionReadOnly
                text: processMenuItem(qsTr("&Read-Only"), actionReadOnly)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdReadOnly()
            }
            MenuSeparator {}
            Menu {
                id: menuLineEndCharacters
                title: processMenuItem(qsTr("&Line End Characters"), menuLineEndCharacters)

                Action {
                    id: actionCrLf
                    text: processMenuItem(qsTr("CR &+ LF"), actionCrLf)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdCrLf()
                }
                Action {
                    id: actionCr
                    text: processMenuItem(qsTr("&CR"), actionCr)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdCr()
                }
                Action {
                    id: actionLf
                    text: processMenuItem(qsTr("&LF"), actionLf)
                    checkable: true
                    checked: false
                    onTriggered: sciteQt.CmdLf()
                }
            }
            Action {
                id: actionConvertLineEndChar
                text: processMenuItem(qsTr("&Convert Line End Characters"), actionConvertLineEndChar)
                //checkable: true
                //checked: false
                onTriggered: sciteQt.CmdConvertLineEndChar()
            }
            MenuSeparator {}
            Action {
                id: actionChangeIndentationSettings
                text: processMenuItem(qsTr("Change Inden&tation Settings"), actionChangeIndentationSettings)
                shortcut: "Ctrl+Shift+I"
                onTriggered: sciteQt.CmdChangeIndentationSettings()
            }
            Action {
                id: actionUseMonospacedFont
                text: processMenuItem(qsTr("Use &Monospaced Font"), actionUseMonospacedFont)
                checkable: true
                checked: false
                onTriggered: sciteQt.CmdUseMonospacedFont()
            }
            MenuSeparator {}
            Action {
                id: actionOpenLocalOptionsFile
                text: processMenuItem(qsTr("Open Local &Options File"), actionOpenLocalOptionsFile)
                onTriggered: sciteQt.CmdOpenLocalOptionsFile()
            }
            Action {
                id: actionOpenDirectoryOptionsFile
                text: processMenuItem(qsTr("Open &Directory Options File"), actionOpenDirectoryOptionsFile)
                onTriggered: sciteQt.CmdOpenDirectoryOptionsFile()
            }
            Action {
                id: actionOpenUserOptionsFile
                text: processMenuItem(qsTr("Open &User Options File"), actionOpenUserOptionsFile)
                onTriggered: sciteQt.CmdOpenUserOptionsFile()
            }
            Action {
                id: actionOpenGlobalOptionsFile
                text: processMenuItem(qsTr("Open &Global Options File"), actionOpenGlobalOptionsFile)
                onTriggered: sciteQt.CmdOpenGlobalOptionsFile()
            }
            Action {
                id: actionOpenAbbreviationsFile
                text: processMenuItem(qsTr("Open A&bbreviations File"), actionOpenAbbreviationsFile)
                onTriggered: sciteQt.CmdOpenAbbreviationsFile()
            }
            Action {
                id: actionOpenLuaStartupScript
                text: processMenuItem(qsTr("Open Lua Startup Scr&ipt"), actionOpenLuaStartupScript)
                onTriggered: sciteQt.CmdOpenLuaStartupScript()
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
                    onTriggered: sciteQt.CmdSelectLanguage(index)
                }

                onObjectAdded: languageMenu.insertItem(index, object)
                onObjectRemoved: languageMenu.removeItem(object)
            }
        }

        AutoSizingMenu {
            id: buffersMenu
            title: processMenuItem(qsTr("&Buffers"),null)

            Action {
                id: actionBuffersPrevious
                text: processMenuItem(qsTr("&Previous"), actionBuffersPrevious)
                shortcut: "Shift+F6"
                onTriggered: sciteQt.CmdBuffersPrevious()
            }
            Action {
                id: actionBuffersNext
                text: processMenuItem(qsTr("&Next"), actionBuffersNext)
                shortcut: "F6"
                onTriggered: sciteQt.CmdBuffersNext()
            }
            Action {
                id: actionBuffersCloseAll
                text: processMenuItem(qsTr("&Close All"), actionBuffersCloseAll)
                onTriggered: sciteQt.CmdBuffersCloseAll()
            }
            Action {
                id: actionBuffersSaveAll
                text: processMenuItem(qsTr("&Save All"), actionBuffersSaveAll)
                onTriggered: sciteQt.CmdBuffersSaveAll()
            }

            MenuSeparator {}

            Instantiator {
                id: currentBufferItems
                model: buffersModel
                delegate: MenuItem {
                    checkable: true
                    checked: model.checkState ? Qt.Checked : Qt.Unchecked
                    text: model.display // index is also available
                    onTriggered: sciteQt.CmdSelectBuffer(index)
                }

                onObjectAdded: buffersMenu.insertItem(index+5, object)
                onObjectRemoved: buffersMenu.removeItem(object)
            }
        }

        AutoSizingMenu {
            id: helpMenu
            title: processMenuItem(qsTr("Help"),null)

            Action {
                id: actionHelp
                shortcut: "F1"
                text: processMenuItem(qsTr("&Help"),actionHelp)
                onTriggered: sciteQt.CmdHelp()
            }
            Action {
                id: actionSciteHelp
                text: processMenuItem(qsTr("&SciTE Help"),actionSciteHelp)
                onTriggered: sciteQt.CmdSciteHelp()
            }
            Action {
                id: actionAboutScite
                text: processMenuItem(qsTr("&About SciTE"),actionAboutScite)
                onTriggered: sciteQt.CmdAboutScite()
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
    Label {
        id: findLabel

        visible: findInput.visible
        height: findInput.height //visible ? implicitHeight : 0

        anchors.verticalCenter: findNextButton.verticalCenter
        anchors.bottom: parent.bottom
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
        anchors.bottom: parent.bottom
        anchors.left: findLabel.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        onAccepted: {
            sciteQt.setFindText(findInput.text)
            hideFindRow()
        }
    }

    Button {
        id: findNextButton

        visible: findInput.visible
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: findMarkAllButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Find Next"))
        onClicked: {
            sciteQt.CmdFindNext()
            hideFindRow()
        }
    }

    Button {
        id: findMarkAllButton

        visible: findInput.visible
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: findWordOnlyButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Mark All"))
        onClicked: {
            sciteQt.CmdMarkAll()
            hideFindRow()
        }
    }

    Button {
        id: findWordOnlyButton

        visible: findInput.visible
        checkable: true
        flat: true
        width: findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: findCaseSensitiveButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("word"))
        onClicked: {
            //sciteQt.CmdMarkAll()
        }
    }

    Button {
        id: findCaseSensitiveButton

        visible: findInput.visible
        checkable: true
        flat: true
        width: findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: findRegExprButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Cc"))
        onClicked: {
            //sciteQt.CmdMarkAll()
        }
    }

    Button {
        id: findRegExprButton

        visible: findInput.visible
        checkable: true
        flat: true
        width: findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: findTransformBackslashButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("^.*"))
        onClicked: {
            //sciteQt.CmdMarkAll()
        }
    }

    Button {
        id: findTransformBackslashButton

        visible: findInput.visible
        checkable: true
        flat: true
        width: findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: findWrapAroundButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("\r\t"))
        onClicked: {
            //sciteQt.CmdMarkAll()
        }
    }

    Button {
        id: findWrapAroundButton

        visible: findInput.visible
        checkable: true
        checked: true
        flat: true
        width: findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: findUpButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("wrap"))
        onClicked: {
            //sciteQt.CmdMarkAll()
        }
    }

    Button {
        id: findUpButton

        visible: findInput.visible
        checkable: true
        flat: true
        width: findNextButton.width / 2
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: findCloseButton.left
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("Up"))
        onClicked: {
            //sciteQt.CmdMarkAll()
        }
    }

    Button {
        id: findCloseButton

        visible: findInput.visible
        background: Rectangle {
            color: "red"
        }

        width: 30
        //focusPolicy: Qt.NoFocus

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: sciteQt.getLocalisedText(qsTr("X"))   // close

        onClicked: {
            hideFindRow()
        }
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

    function handleEncodingMenus(enumEncoding) {
        actionCodePageProperty.checked = false
        actionUtf16BigEndian.checked = false
        actionUtf16LittleEndian.checked = false
        actionUtf8WithBOM.checked = false
        actionUtf8.checked = false
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
                actionUtf8.checked = true
                break;
            case 4:
                actionUtf8WithBOM.checked = true
                break;
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

        onShowFind:                   showFind(text)

        onSetVerticalSplit:           setVerticalSplit(verticalSplit)
        onSetOutputHeight:            setOutputHeight(heightOutput)

        onSetMenuChecked:             handleMenuChecked(menuID, val)
        onSetMenuEnable:              handleMenuEnable(menuID, val)

        onUpdateEolMenus:             handeEolMenus(enumEol)
        onUpdateEncodingMenus:        handleEncodingMenus(enumEncoding)

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
        var item = tabButton.createObject(tabBar, {text: title, fcnClicked: function () { sciteQt.CmdSelectBuffer(index) }})
        tabBar.insertItem(index, item)
    }
}
