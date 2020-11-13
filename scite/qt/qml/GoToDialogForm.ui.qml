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

    anchors.fill: parent

    property alias cancelButton: cancelButton
    property alias gotoButton: gotoButton
    property alias destinationLineInput: destinationLineInput
    property alias columnInput: columnInput
    property alias currentLineOutput: currentLineOutput
    property alias currentColumnOutput: currentColumnOutput
    property alias lastLineOutput: lastLineOutput
    property alias grid: grid

    property int lastLineNumber

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
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: columnLineLabel
                text: localiseText(qsTr("Column:"))
            }

            TextField {
                id: columnInput
                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: gotoButton
                highlighted: true
                text: localiseText(qsTr("&Go To"),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
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
                text: localiseText(qsTr("Cancel"),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Label {
                id: lastLineLabel
                text: localiseText(qsTr("Last Line:"))
            }

            Label {
                id: lastLineOutput
            }

            Label {
                id: fillLabel
                text: ""
                visible: sciteQt.mobilePlatform

                Layout.columnSpan: 5
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:1.5;height:480;width:640}
}
##^##*/

