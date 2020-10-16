import QtQuick 2.0
import QtQuick.Controls 2.9
import QtQml.Models 2.14

AutoSizingMenu {
    id: helpMenu
    title: processMenuItem(qsTr("&Help"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

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
