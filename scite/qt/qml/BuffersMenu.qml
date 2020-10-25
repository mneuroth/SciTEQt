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
    id: buffersMenu
    title: processMenuItem(qsTr("&Buffers"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

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
