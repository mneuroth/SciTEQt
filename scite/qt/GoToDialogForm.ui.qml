import QtQuick 2.4
import QtQuick.Controls 2.9
import QtQuick.Layouts 1.0
//import QtQuick.Dialogs 1.3
import QtQuick.Window 2.9

Window {

    width: 550
    height: 100

    //flags: Qt.MSWindowsFixedSizeDialogHint

    maximumHeight: height
    maximumWidth: width

    minimumHeight: height
    minimumWidth: width

    property alias cancelButton: cancelButton
    property alias gotoButton: gotoButton
    property alias destinationLineInput: destinationLineInput
    property alias columnInput: columnInput
    property alias currentLineOutput: currentLineOutput
    property alias currentColumnOutput: currentColumnOutput
    property alias lastLineOutput: lastLineOutput

    property int lastLineNumber

    property var fcnLocalisation: undefined

    function localiseText(text) {
        if(fcnLocalisation !== undefined) {
            return fcnLocalisation(text)
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

            columns: 5
            rows: 4

            Label {
                id: destinationLineLabel
                text: localiseText(qsTr("Destination Line:"))
            }

            TextField {
                id: destinationLineInput
                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
            }

            Label {
                id: columnLineLabel
                text: localiseText(qsTr("Column:"))
            }

            TextField {
                id: columnInput
                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
            }

            Button {
                id: gotoButton
                text: localiseText(qsTr("&Go To"))
                Keys.onEscapePressed: cancelButton.clicked()
            }

            Label {
                id: currentLineLabel
                text: localiseText(qsTr("Current Line:"))
            }

            Label {
                id: currentLineOutput
            }

            Label {
                id: currentColumnLabel
                text: localiseText(qsTr("Column:"))
            }

            Label {
                id: currentColumnOutput
            }

            Button {
                id: cancelButton
                text: localiseText(qsTr("Cancel"))
                Keys.onEscapePressed: cancelButton.clicked()
            }

            Label {
                id: lastLineLabel
                text: localiseText(qsTr("Last Line:"))
            }

            Label {
                id: lastLineOutput
            }

            Label {
                id: fillRowLabel
                Layout.columnSpan: 3
                //Layout.fillWidth: true
            }

            Label {
                id: fillLabel
                text: qsTr("")
                Layout.fillHeight: true
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:1.5;height:480;width:640}
}
##^##*/

