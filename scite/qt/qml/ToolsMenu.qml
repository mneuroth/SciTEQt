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
    id: toolsMenu
    title: processMenuItem(qsTr("&Tools"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

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
                text: model.display+(model.shortcut.length>0 ? (sciteQt.mobilePlatform ? "" : (" ("+model.shortcut+")")) : "")
                shortcut: model.shortcut
                onTriggered: sciteQt.cmdCallTool(/*index*/model.index)
            }
        }

        onObjectAdded: (index, object) => { toolsMenu.insertItem(index+4, object) }
        onObjectRemoved: (object) => { toolsMenu.removeItem(object) }
    }
    MenuItem {
        id: actionStopExecuting
        text: processMenuItem2(actions.actionStopExecuting.text, actionStopExecuting)
        action: actions.actionStopExecuting
    }
    MenuItem {
        id: actionAboutCurrentFile
        text: processMenuItem2(actions.actionAboutCurrentFile.text, actionAboutCurrentFile)
        action: actions.actionAboutCurrentFile
    }
    MenuItem {
        id: actionRunCurrentAsJavaScript
        text: processMenuItem2(actions.actionRunCurrentAsJavaScript.text, actionRunCurrentAsJavaScript)
        action: actions.actionRunCurrentAsJavaScript
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
