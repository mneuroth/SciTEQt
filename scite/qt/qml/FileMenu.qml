/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQml.Models 2.1
import QtQml 2.2

AutoSizingMenu {
    id: fileMenu
    title: processMenuItem(qsTr("&File"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

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
        visible: !sciteQt.mobilePlatform
        height: visible ? actionSave.height : 0
    }
    MenuItem {
        id: actionDeleteFiles
        text: processMenuItem2(actions.actionDeleteFiles.text, actionDeleteFiles)
        action: actions.actionDeleteFiles
        visible: sciteQt.mobilePlatform
        height: visible ? actionSave.height : 0
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
        visible: !sciteQt.mobilePlatform
        height: visible ? actionSave.height : 0
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
                onTriggered: {
                    fileMenu.close()
                    sciteQt.cmdLastOpenedFiles(index)
                }
            }
        }

        onObjectAdded: fileMenu.insertItem(index+19, object)
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
