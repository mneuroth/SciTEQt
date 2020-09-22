import QtQuick 2.4
import QtQuick.Controls 2.9
import QtQuick.Layouts 1.0
//import QtQuick.Dialogs 1.3
import QtQuick.Window 2.9

Window {

    //width: 500
    //height: 130
    width: grid.implicitWidth+10
    height: grid.implicitHeight+10

    //flags: Qt.MSWindowsFixedSizeDialogHint

    // Window is not resizable !
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width

    property alias cancelButton: cancelButton
    property alias findButton: findButton
    property alias browseButton: browseButton
    property alias upButton: upButton
    property alias findWhatInput: findWhatInput
    property alias filesExtensionsInput: filesExtensionsInput
    property alias directoryInput: directoryInput
    property alias findWhatModel: findWhatModel
    property alias filesExtensionsModel: filesExtensionsModel
    property alias directoryModel: directoryModel

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

            columns: 4
            rows: 5

            Label {
                id: findLabel
                text: localiseText(qsTr("Find what:"))
            }

            ComboBox {
                id: findWhatInput
                Layout.columnSpan: 2
                Layout.fillWidth: true
                editable: true
                model: findWhatModel

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            ListModel {
                id: findWhatModel
                objectName: "findWhatModel"
            }

            Button {
                id: findButton
                text: qsTr("Find")
                highlighted: true
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: filesLabel
                text: localiseText(qsTr("Files:"))
            }

            ComboBox {
                id: filesExtensionsInput
                Layout.columnSpan: 2
                Layout.fillWidth: true
                editable: true
                model: filesExtensionsModel

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            ListModel {
                id: filesExtensionsModel
                objectName: "filesExtensionsModel"
            }

            Button {
                id: cancelButton
                text: localiseText(qsTr("Cancel"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: directoryLabel
                text: localiseText(qsTr("Directory:"))
            }

            ComboBox {
                id: directoryInput
                Layout.columnSpan: 2
                Layout.fillWidth: true
                editable: true
                model: directoryModel

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            ListModel {
                id: directoryModel
                objectName: "directoryModel"
            }

            Button {
                id: upButton
                implicitWidth: browseButton.width / 4
                text: localiseText(qsTr(".."))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: browseButton
                text: localiseText(qsTr("Browse..."))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Row {
                Layout.columnSpan: 4

                CheckBox {
                    id: wholeWordButton
                    text: localiseText(qsTr("Match whole word only"))
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }

                CheckBox {
                    id: caseSensitiveButton
                    text: localiseText(qsTr("Case sensitive"))
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }
            }
/*
            Label {
                id: fillLabel
                text: qsTr("")
                Layout.fillHeight: true
            }
*/
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:1.5;height:480;width:640}
}
##^##*/

