import QtQuick 2.4
import QtQuick.Controls 2.9
import QtQuick.Layouts 1.0
//import QtQuick.Dialogs 1.3
import QtQuick.Window 2.9

Window {

    //width: 550
    //height: 100
    width: grid.implicitWidth+10+50
    height: grid.implicitHeight+10

    //flags: Qt.MSWindowsFixedSizeDialogHint

    // Window is not resizable !
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width

    property bool isModal: false

    property alias cancelButton: cancelButton
    property alias setButton: setButton
    property alias cmdInput: cmdInput
    property alias parameter1Input: parameter1Input
    property alias parameter2Input: parameter2Input
    property alias parameter3Input: parameter3Input
    property alias parameter4Input: parameter4Input

    property var fcnLocalisation: undefined

    function localiseText(text,filterShortcuts=true) {
        if(fcnLocalisation !== undefined) {
            return fcnLocalisation(text,filterShortcuts)
        }
        return text
    }

    // remove standard Ok button from dialog (see: https://stackoverflow.com/questions/50858605/qml-dialog-removing-ok-button)
    /*contentItem:*/ Item {

        anchors.fill: parent

        GridLayout {
            id: grid

            anchors.fill: parent
            anchors.rightMargin: 5
            anchors.leftMargin: 5
            anchors.topMargin: 5
            anchors.bottomMargin: 5

            columns: 4
            rows: 6

            TextField {
                id: cmdInput
                Layout.columnSpan: 4
                visible: isModal
                Layout.fillWidth: true

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: parameter1Label
                text: localiseText(qsTr("1:"))
            }

            TextField {
                id: parameter1Input
                Layout.columnSpan: 3
                Layout.fillWidth: true

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: parameter2Label
                text: localiseText(qsTr("2:"))
            }

            TextField {
                id: parameter2Input
                Layout.columnSpan: 3
                Layout.fillWidth: true

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: parameter3Label
                text: localiseText(qsTr("3:"))
            }

            TextField {
                id: parameter3Input
                Layout.columnSpan: 3
                Layout.fillWidth: true

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: parameter4Label
                text: localiseText(qsTr("4:"))
            }

            TextField {
                id: parameter4Input
                Layout.columnSpan: 3
                Layout.fillWidth: true

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: setButton
                Layout.columnSpan: 2
                highlighted: true
                text: localiseText(qsTr("&Set"),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: cancelButton
                Layout.columnSpan: 2
                text: localiseText(qsTr("Cancel"),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:1.5;height:480;width:640}
}
##^##*/

