import QtQuick 2.4
import QtQuick.Controls 2.9
import QtQuick.Layouts 1.0
//import QtQuick.Dialogs 1.3
import QtQuick.Window 2.9

Window {

    //width: 550
    //height: 100
    width: grid.implicitWidth+10
    height: grid.implicitHeight+10

    //flags: Qt.MSWindowsFixedSizeDialogHint

    // Window is not resizable !
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width

    property alias cancelButton: cancelButton
    property alias okButton: okButton
    property alias convertButton: convertButton
    property alias tabSizeInput: tabSizeInput
    property alias indentSizeInput: indentSizeInput
    property alias useTabsCheckBox: useTabsCheckBox

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

            columns: 3
            rows: 3

            Label {
                id: tabSizeLabel
                text: localiseText(qsTr("Tab Size:"))
            }

            TextField {
                id: tabSizeInput
                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: okButton
                text: localiseText(qsTr("OK"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: indentSizeLabel
                text: localiseText(qsTr("Indent Size:"))
            }

            TextField {
                id: indentSizeInput
                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: cancelButton
                text: localiseText(qsTr("Cancel"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: useTabsLabel
                text: localiseText(qsTr("Use tabs:"))
            }

            CheckBox {
                id: useTabsCheckBox
            }

            Button {
                id: convertButton
                text: localiseText(qsTr("Convert"))
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

