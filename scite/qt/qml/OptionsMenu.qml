/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 2.9
import QtQml.Models 2.14

AutoSizingMenu {
    id: optionsMenu
    title: processMenuItem(qsTr("&Options"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

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
