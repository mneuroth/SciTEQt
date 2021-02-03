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

AutoSizingMenu {
    id: helpMenu
    title: processMenuItem(qsTr("&Help"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

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
    MenuItem {
        id: actionSupportSciteQt
        text: processMenuItem2(actions.actionSupportSciteQt.text, actionSupportSciteQt)
        action: actions.actionSupportSciteQt
        visible: sciteQt.mobilePlatform && !sciteQt.isWebassemblyPlatform()
        height: visible ? actionAboutQt.heigh : 0
    }

/* for debugging only:

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
            sciteQt.testFunction("debugging");
        }
    }
*/
}
