/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

Page {
    id: root

    focusPolicy: Qt.StrongFocus
    focus: true

    anchors.fill: parent

    property alias cancelButton: cancelButton
    property alias insertButton: insertButton
    property alias abbreviationInput: abbreviationInput
    property alias abbreviationModel: abbreviationModel
    property alias grid: grid

    property var fcnLocalisation: undefined

    function localiseText2(text,filterShortcuts/*=true*/) {
        if(fcnLocalisation !== undefined) {
            return fcnLocalisation(text,filterShortcuts)
        }
        return text
    }
    function localiseText(text) {
        return localiseText2(text,true)
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
                id: abbreviationLabel
                text: localiseText(qsTr("Abbreviation:"))
            }

            ComboBox {
                id: abbreviationInput
                Layout.columnSpan: 2
                Layout.fillWidth: true
                model: abbreviationModel

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            ListModel {
                id: abbreviationModel
                objectName: "abbreviationModel"
            }

            Label {

            }

            Label {
                id: fillLabel
                text: ""
                visible: sciteQt.useMobileDialogHandling

                Layout.columnSpan: 3
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Button {
                id: insertButton
                highlighted: !sciteQt.mobilePlatform
                text: localiseText(qsTr("&Insert"),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: cancelButton
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

