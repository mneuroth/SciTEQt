import org.scintilla.scintilla 1.0
import QtQuick 2.9
import QtQuick.Controls 2.14
import QtQuick.Dialogs 1.2

// Use Scrollview to handle ScrollBars for QuickScintilla control.
// Scrollbar uses Rectangle as flickable item which has the implicitSize of the QuickScintilla control
// (implicitSize is the maximun size needed to show the full content of the QuickScintilla control).
// The QuickScintilla controll will be placed at the currently visible viewport of the ScrollView.
//Flickable {
ScrollView {
    id: scrollView
    objectName: "scrollView"
    focus: true
    clip: true
    //focusPolicy: Qt.StrongFocus
    //filtersChildMouseEvents: false

    property alias quickScintillaEditor: quickScintillaEditor
    property alias text: quickScintillaEditor.text
    property alias scintilla: quickScintillaEditor
    //property Flickable flickableItem: flickableItem
    //property alias vScrollBar: ScrollBar.vertical
    property bool actionFromKeyboard: false

    //anchors.fill: parent

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
    ScrollBar.vertical.policy: ScrollBar.AlwaysOn

    // box needed to support ScrollView and simulate current maximum size of scintilla text control
    // editor control will be placed in visible part of this rectangle
    Rectangle {
        id: editorFrame

        anchors.fill: parent

//            // TODO dies macht die Probleme mit den verschwindenden Moue Events !!! width/height ok, logicalWidth/logicalHeight oder width+1/height nicht ok ?
        implicitWidth: quickScintillaEditor.logicalWidth
        implicitHeight: quickScintillaEditor.logicalHeight

        // the QuickScintilla control
        ScintillaEditBase {
            id: quickScintillaEditor

            width: scrollView.availableWidth-scrollView.ScrollBar.vertical.width //+ 2*quickScintillaEditor.charHeight
            height: scrollView.availableHeight-scrollView.ScrollBar.horizontal.height //+ 2*quickScintillaEditor.charWidth

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

    Menu {
        id: editContextMenu

        MenuItem {
            text: "Copy"
            onTriggered: {
                console.log("Copy! "+quickScintillaEditor.visibleColumns)
            }
        }
        MenuItem {
            text: "Cut"
            onTriggered: {
                console.log("Cut!")
            }
        }
        MenuItem {
            text: "Paste"
            onTriggered: {
                console.log("Paste!")
            }
        }
    }

    Connections {
        // https://stackoverflow.com/questions/30359262/how-to-scroll-qml-scrollview-to-center
        target: scrollView.contentItem

        onContentXChanged: {
            var delta = scrollView.contentItem.contentX - quickScintillaEditor.x
            var deltaInColumns = parseInt(delta / quickScintillaEditor.charWidth,10)
            //console.log("xchanged delta="+delta+" deltaCol="+deltaInColumns+" shift="+deltaInColumns*quickScintillaEditor.charWidth+" contentX="+scrollView.contentItem.contentX+" scintillaX="+quickScintillaEditor.x)
            if(delta >= quickScintillaEditor.charWidth) {
                //console.log("p1")
                if(!scrollView.actionFromKeyboard)
                {
                    // disable repaint: https://stackoverflow.com/questions/46095768/how-to-disable-update-on-a-qquickitem
                    quickScintillaEditor.enableUpdate(false);
                    quickScintillaEditor.x = quickScintillaEditor.x // + deltaInColumns*quickScintillaEditor.charWidth    // TODO --> bewirkt geometry changed !!!
                    quickScintillaEditor.scrollColumn(deltaInColumns)
                    quickScintillaEditor.enableUpdate(true)
                }
            }
            else if(-delta >= quickScintillaEditor.charWidth) {
                //console.log("p2")
                if(!scrollView.actionFromKeyboard)
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
                //console.log("p3")
            }
        }
        onContentYChanged: {
            //console.log("YCHANGED")
            var delta = scrollView.contentItem.contentY - quickScintillaEditor.y
            var deltaInLines = parseInt(delta / quickScintillaEditor.charHeight,10)
            if(delta >= quickScintillaEditor.charHeight) {
                //console.log("P1")
                // disable repaint: https://stackoverflow.com/questions/46095768/how-to-disable-update-on-a-qquickitem
                quickScintillaEditor.enableUpdate(false);
                quickScintillaEditor.y = quickScintillaEditor.y + deltaInLines*quickScintillaEditor.charHeight    // TODO --> bewirkt geometry changed !!!
                quickScintillaEditor.scrollRow(deltaInLines)
                quickScintillaEditor.enableUpdate(true)
            }
            else if(-delta >= quickScintillaEditor.charHeight) {
                //console.log("P2")
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
                //console.log("P3")
            }
        }
    }

    // process signals from the quick scintilla editor control triggered by keyboard interactions
    Connections {
        target: quickScintillaEditor

        onShowContextMenu: {
            editContextMenu.popup(pos)
        }

        onDoubleClick: {
            console.log("double click !")
        }

        onMarginClicked: {
            console.log("MARGING CLICK !")
        }

        onTextAreaClicked: {
            console.log("TextArea CLICK !")
        }

        // this signal is emited if the scintilla editor contol scrolls, because of a keyboard interaction
        //   --> update the scrollview appropriate: move editor control to right position and
        //       update content area and position of scroll view (results in updating the scrollbar)
        onHorizontalScrolled: {
            // value from scintilla in pixel !
            quickScintillaEditor.x = value              // order of calls is very important: first update child and then the container !
            scrollView.contentItem.contentX = value
        }
        onVerticalScrolled: {
            // value from scintilla in lines !
            quickScintillaEditor.y = value*quickScintillaEditor.charHeight
            scrollView.contentItem.contentY = value*quickScintillaEditor.charHeight
        }
      /*
        onHorizontalRangeChanged: {
        }
        onVerticalRangeChanged: {
        }
      */
    }
}
