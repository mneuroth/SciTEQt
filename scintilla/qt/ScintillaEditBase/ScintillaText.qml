/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Dialogs 1.2
import QtQml.Models 2.1

import org.scintilla.scintilla 1.0

// Use Scrollview to handle ScrollBars for QuickScintilla control.
// Scrollbar uses Rectangle as flickable item which has the implicitSize of the QuickScintilla control
// (implicitSize is the maximun size needed to show the full content of the QuickScintilla control).
// The QuickScintilla controll will be placed at the currently visible viewport of the ScrollView.
//Flickable {
ScrollView {
    id: root
    objectName: "ScintillaRoot"
    clip: true

    focusPolicy: Qt.StrongFocus
    //filtersChildMouseEvents: false

    property alias quickScintillaEditor: quickScintillaEditor

    // public properties
    property alias text: quickScintillaEditor.text
    property alias readonly: quickScintillaEditor.readonly

    // private properties, used only for technical details...
    property alias scintilla: quickScintillaEditor
    //property Flickable flickableItem: flickableItem
    //property alias vScrollBar: ScrollBar.vertical
    property bool actionFromKeyboard: false

    property var menuCommandDelegate: undefined

    focus: true
    onFocusChanged: {
        console.log("ScintillaText onFocusChanged()")
        quickScintillaEditor.focus = focus
    }

    property var fcnLocalisation: undefined

    function localiseText(text) {
        if(fcnLocalisation !== undefined) {
            return fcnLocalisation(text)
        }
        return text
    }

    // fixes problem with selecting text with the mouse --> mouse events will be consumed by ScrollView instead of ScintillaEditor control
    // see: https://forum.qt.io/topic/94260/scrollview-interfering-with-mousearea-and-or-mouseevents
    Component.onCompleted: {
        contentItem.interactive = true  // for Flickable
    }

    //anchors.fill: parent

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
    ScrollBar.vertical.policy: ScrollBar.AlwaysOn

    // box needed to support ScrollView and simulate current maximum size of scintilla text control
    // editor control will be placed in visible part of this rectangle
    Item {
        id: editorFrame        

        anchors.fill: parent

        implicitWidth: quickScintillaEditor.logicalWidth
        implicitHeight: quickScintillaEditor.logicalHeight

        // the QuickScintilla control
        ScintillaEditBase {
            id: quickScintillaEditor
            objectName: root.objectName+"Scintilla"

            readonly: false

            width: root.availableWidth-root.ScrollBar.vertical.width //+ 2*quickScintillaEditor.charHeight
            height: root.availableHeight-root.ScrollBar.horizontal.height //+ 2*quickScintillaEditor.charWidth

            // position of the QuickScintilla controll will be changed in response of signals from the ScrollView
            x : 0
            y : 0

            Accessible.role: Accessible.EditableText

    //                implicitWidth: quickScintillaEditor.logicalWidth // 1600//1000
    //                implicitHeight: quickScintillaEditor.logicalHeight //1800//3000
            //font.family: "Courier New"  //*/ "Hack"
            //font.pointSize: 18

            focus: true

    /*
            MouseArea {
                id: mouseArea
                z: -1

                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton
                propagateComposedEvents: true

                onClicked: {
                    console.log("CLICK mouseArea")
                    mouse.accepted = false
                }
                onPressAndHold: {
                    console.log("CLICKAndHold mouseArea")
                    mouse.accepted = false
                }
                onPositionChanged: {
                    console.log("position changed")
                }
            }
    */
        }
    }

    // see: SciTEBase::ContextMenu
    Menu {
        id: editContextMenu

        Instantiator {
            id: contextMenuItems
            model: contextMenuModel
            delegate: MenuItem {
                text: localiseText(model.display)
                enabled: model.enabled
                onTriggered: menuCommandDelegate !== undefined ? menuCommandDelegate(model.menuId) : quickScintillaEditor.cmdContextMenu(model.menuId)
            }

            onObjectAdded: editContextMenu.insertItem(index, object)
            onObjectRemoved: editContextMenu.removeItem(object)
        }

    }

    ListModel {
        id: contextMenuModel
        objectName: "contextMenu"
        /*
        ListElement {
            display: "hello"
            enabled: true
            menuId: 123
        }
        ListElement {
            display: "World !"
            enabled: false
            menuId: 124
        }
        */
    }

    Connections {
        // https://stackoverflow.com/questions/30359262/how-to-scroll-qml-scrollview-to-center
        target: root.contentItem

        onContentXChanged: {
console.log("ScintillaText onContentXChanged()")
            var delta = root.contentItem.contentX - quickScintillaEditor.x
            var deltaInColumns = parseInt(delta / quickScintillaEditor.charWidth,10)
            if(delta >= quickScintillaEditor.charWidth) {
                if(!root.actionFromKeyboard)
                {
                    // disable repaint: https://stackoverflow.com/questions/46095768/how-to-disable-update-on-a-qquickitem
                    quickScintillaEditor.enableUpdate(false);
                    quickScintillaEditor.x = quickScintillaEditor.x // + deltaInColumns*quickScintillaEditor.charWidth    // TODO --> bewirkt geometry changed !!!
                    quickScintillaEditor.scrollColumn(deltaInColumns)
                    quickScintillaEditor.enableUpdate(true)
                }
            }
            else if(-delta >= quickScintillaEditor.charWidth) {
                if(!root.actionFromKeyboard)
                {
                    quickScintillaEditor.enableUpdate(false);
                    quickScintillaEditor.x = quickScintillaEditor.x + deltaInColumns*quickScintillaEditor.charWidth      // deltaInColumns is < 0
                    if(quickScintillaEditor.x < 0)
                    {
                        quickScintillaEditor.x = 0
                    }
                    quickScintillaEditor.scrollColumn(deltaInColumns)   // deltaInColumns is < 0
                    quickScintillaEditor.enableUpdate(true)
                }
            }
            else {
            }
        }
        onContentYChanged: {
console.log("ScintillaText onContentYChanged()")
            var delta = root.contentItem.contentY - quickScintillaEditor.y
            var deltaInLines = parseInt(delta / quickScintillaEditor.charHeight,10)
            if(delta >= quickScintillaEditor.charHeight) {
                // disable repaint: https://stackoverflow.com/questions/46095768/how-to-disable-update-on-a-qquickitem
                quickScintillaEditor.enableUpdate(false);
                quickScintillaEditor.y = quickScintillaEditor.y + deltaInLines*quickScintillaEditor.charHeight    // TODO --> bewirkt geometry changed !!!
                quickScintillaEditor.scrollRow(deltaInLines)
                quickScintillaEditor.enableUpdate(true)
            }
            else if(-delta >= quickScintillaEditor.charHeight) {
                quickScintillaEditor.enableUpdate(false);
                quickScintillaEditor.y = quickScintillaEditor.y + deltaInLines*quickScintillaEditor.charHeight
                if(quickScintillaEditor.y < 0)
                {
                    quickScintillaEditor.y = 0
                }
                quickScintillaEditor.scrollRow(deltaInLines) // -1 * -1
                quickScintillaEditor.enableUpdate(true)
            }
            else {
            }
        }
    }

    // process signals from the quick scintilla editor control triggered by keyboard interactions
    Connections {
        target: quickScintillaEditor

        onEnableScrollViewInteraction: {
           root.contentItem.interactive = value
        }

        onShowContextMenu: editContextMenu.popup(pos)

        onAddToContextMenu: contextMenuModel.append({"display":txt, "enabled":enabled, "menuId":menuId})

        onClearContextMenu: contextMenuModel.clear()

        onDoubleClick: {
            //console.log("double click !")
        }

        onMarginClicked: {
            //console.log("MARGING CLICK !")
        }

        onTextAreaClicked: {
            //console.log("TextArea CLICK !")
        }

        // this signal is emited if the scintilla editor contol scrolls, because of a keyboard interaction
        //   --> update the root appropriate: move editor control to right position and
        //       update content area and position of scroll view (results in updating the scrollbar)
        onHorizontalScrolled: {
            // value from scintilla in pixel !
console.log("ScintillaText onHorizontalScrolled()")
            quickScintillaEditor.x = value              // order of calls is very important: first update child and then the container !
            root.contentItem.contentX = value
        }
        onVerticalScrolled: {
            // value from scintilla in lines !
console.log("ScintillaText onVerticalScrolled()")
            quickScintillaEditor.y = value*quickScintillaEditor.charHeight
            root.contentItem.contentY = value*quickScintillaEditor.charHeight
        }
      /*
        onHorizontalRangeChanged: {
        }
        onVerticalRangeChanged: {
        }
      */
    }
}
