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

    property bool isModal: false

    property alias cancelButton: cancelButton
    property alias setButton: setButton
    property alias cmdInput: cmdInput
    property alias parameter1Input: parameter1Input
    property alias parameter2Input: parameter2Input
    property alias parameter3Input: parameter3Input
    property alias parameter4Input: parameter4Input
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
    /*contentItem:*/ Page {

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

            Label {
                id: fillLabel
                text: ""
                visible: sciteQt.useMobileDialogHandling

                Layout.columnSpan: 4
                Layout.fillHeight: true
                Layout.fillWidth: true
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

