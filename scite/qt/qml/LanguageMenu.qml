import QtQuick 2.0
import QtQuick.Controls 2.9
import QtQml.Models 2.14

Menu {
    id: languageMenu
    title: processMenuItem(qsTr("&Language"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

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

