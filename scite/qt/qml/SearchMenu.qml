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
    id: searchMenu
    title: processMenuItem(qsTr("&Search"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

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
