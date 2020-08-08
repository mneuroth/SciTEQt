import QtQuick 2.4
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import QtQuick.Dialogs 1.2

Dialog {

    //width: 400
    //height: 400
    //anchors.fill: parent

    property alias cancelButton: cancelButton
    property alias findButton: findButton
    property alias browseButton: browseButton
    property alias upButton: upButton
    property alias findWhatInput: findWhatInput
    property alias filesExtensionsInput: filesExtensionsInput
    property alias directoryInput: directoryInput

    // remove standard Ok button from dialog (see: https://stackoverflow.com/questions/50858605/qml-dialog-removing-ok-button)
    contentItem: Item {

        GridLayout {
            id: grid

            anchors.fill: parent
            anchors.rightMargin: 5
            anchors.leftMargin: 5
            anchors.topMargin: 5
            anchors.bottomMargin: 5

            columns: 4
            rows: 5

            Label {
                id: findLabel
                text: qsTr("Find what:")
            }

            TextField {
                id: findWhatInput
                text: qsTr("Text Input")
                font.pixelSize: 12
                Layout.columnSpan: 2
                Layout.fillWidth: true
            }

            Button {
                id: findButton
                text: qsTr("Find")
            }

            Label {
                id: filesLabel
                text: qsTr("Files:")
            }

            TextField {
                id: filesExtensionsInput
                text: qsTr("*.c")
                font.pixelSize: 12
                Layout.columnSpan: 2
                Layout.fillWidth: true
            }

            Button {
                id: cancelButton
                text: qsTr("Cancel")
            }

            Label {
                id: directoryLabel
                text: qsTr("Directory:")
            }

            TextField {
                id: directoryInput
                text: qsTr("c:\\temp")
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            Button {
                id: upButton
                width: browseButton.width / 2
                text: qsTr("..")
            }

            Button {
                id: browseButton
                text: qsTr("Browse...")
            }

            Row {
                Layout.columnSpan: 4

                CheckBox {
                    id: wholeWordButton
                    text: qsTr("Match whole word only")
                }

                CheckBox {
                    id: caseSensitiveButton
                    text: qsTr("Case sensitive")
                }
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

