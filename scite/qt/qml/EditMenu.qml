import QtQuick 2.0
import QtQuick.Controls 2.9
import QtQml.Models 2.14

AutoSizingMenu {
    id: editMenu
    title: processMenuItem(qsTr("&Edit"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

    MenuItem {
        id: actionUndo
        text: processMenuItem2(actions.actionUndo.text, actionUndo)
        action: actions.actionUndo
    }
    MenuItem {
        id: actionRedo
        text: processMenuItem2(actions.actionRedo.text, actionRedo)
        action: actions.actionRedo
    }
    MenuSeparator {}
    MenuItem {
        id: actionCut
        text: processMenuItem2(actions.actionCut.text, actionCut)
        action: actions.actionCut
    }
    MenuItem {
        id: actionCopy
        text: processMenuItem2(actions.actionCopy.text, actionCopy)
        action: actions.actionCopy
    }
    MenuItem {
        id: actionPaste
        text: processMenuItem2(actions.actionPaste.text, actionPaste)
        action: actions.actionPaste
    }
    MenuItem {
        id: actionDuplicate
        text: processMenuItem2(actions.actionDuplicate.text, actionDuplicate)
        action: actions.actionDuplicate
    }
    MenuItem {
        id: actionDelete
        text: processMenuItem2(actions.actionDelete.text, actionDelete)
        action: actions.actionDelete
    }
    MenuItem {
        id: actionSelectAll
        text: processMenuItem2(actions.actionSelectAll.text, actionSelectAll)
        action: actions.actionSelectAll
    }
    MenuItem {
        id: actionCopyAsRtf
        text: processMenuItem2(actions.actionCopyAsRtf.text, actionCopyAsRtf)
        action: actions.actionCopyAsRtf
        visible: !useSimpleMenu
        height: useSimpleMenu ? 0 : actionCopy.height
    }
    MenuSeparator {}
    MenuItem {
        id: actionMatchBrace
        text: processMenuItem2(actions.actionMatchBrace.text, actionMatchBrace)
        action: actions.actionMatchBrace
    }
    MenuItem {
        id: actionSelectToBrace
        text: processMenuItem2(actions.actionSelectToBrace.text, actionSelectToBrace)
        action: actions.actionSelectToBrace
    }
    MenuItem {
        id: actionShowCalltip
        text: processMenuItem2(actions.actionShowCalltip.text, actionShowCalltip)
        action: actions.actionShowCalltip
        visible: !useSimpleMenu
        height: useSimpleMenu ? 0 : actionCopy.height
    }
    MenuItem {
        id: actionCompleteSymbol
        text: processMenuItem2(actions.actionCompleteSymbol.text, actionCompleteSymbol)
        action: actions.actionCompleteSymbol
    }
    MenuItem {
        id: actionCompleteWord
        text: processMenuItem2(actions.actionCompleteWord.text, actionCompleteWord)
        action: actions.actionCompleteWord
    }
    MenuItem {
        id: actionExpandAbbreviation
        text: processMenuItem2(actions.actionExpandAbbreviation.text, actionExpandAbbreviation)
        action: actions.actionExpandAbbreviation
    }
    MenuItem {
        id: actionInsertAbbreviation
        text: processMenuItem(actions.actionInsertAbbreviation.text, actionInsertAbbreviation)
        action: actions.actionInsertAbbreviation
   }
    MenuItem {
        id: actionBlockComment
        text: processMenuItem(actions.actionBlockComment.text, actionBlockComment)
        action: actions.actionBlockComment
    }
    MenuItem {
        id: actionBoxComment
        text: processMenuItem(actions.actionBoxComment.text, actionBoxComment)
        action: actions.actionBoxComment
        visible: !useSimpleMenu
        height: useSimpleMenu ? 0 : actionCopy.height
    }
    MenuItem {
        id: actionStreamComment
        text: processMenuItem(actions.actionStreamComment.text, actionStreamComment)
        action: actions.actionStreamComment
        visible: !useSimpleMenu
        height: useSimpleMenu ? 0 : actionCopy.height
    }
    MenuItem {
        id: actionMakeSelectionUppercase
        text: processMenuItem(actions.actionMakeSelectionUppercase.text, actionMakeSelectionUppercase)
        action: actions.actionMakeSelectionUppercase
    }
    MenuItem {
        id: actionMakeSelectionLowercase
        text: processMenuItem(actions.actionMakeSelectionLowercase.text, actionMakeSelectionLowercase)
        action: actions.actionMakeSelectionLowercase
    }
    MenuItem {
        id: actionReverseSelectedLines
        text: processMenuItem(actions.actionReverseSelectedLines.text, actionReverseSelectedLines)
        action: actions.actionReverseSelectedLines
    }
    Menu {
        id: menuParagraph
        title: processMenuItem(qsTr("Para&graph"), menuParagraph)
        //visible: !useSimpleMenu
        //height: useSimpleMenu ? 0 : actionCopy.height

        MenuItem {
            id: actionJoin
            text: processMenuItem2(actions.actionJoin.text, actionJoin)
            action: actions.actionJoin
            //visible: !useSimpleMenu
            //height: useSimpleMenu ? 0 : actionCopy.height
        }
        MenuItem {
            id: actionSplit
            text: processMenuItem2(actions.actionSplit.text, actionSplit)
            action: actions.actionSplit
            //visible: !useSimpleMenu
            //height: useSimpleMenu ? 0 : actionCopy.height
        }
    }
}
