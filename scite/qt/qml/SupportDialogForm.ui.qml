/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
import QtQuick 2.4
import QtQuick.Controls 2.1

Page {
    id: root

    focusPolicy: Qt.StrongFocus
    focus: true

    anchors.fill: parent

    property alias btnSupportLevel0: btnSupportLevel0
    property alias btnSupportLevel1: btnSupportLevel1
    property alias btnSupportLevel2: btnSupportLevel2
    property alias btnClose: btnClose
    property alias lblLevel0: lblLevel0
    property alias lblLevel1: lblLevel1
    property alias lblLevel2: lblLevel2
    property alias lblGooglePlay: lblGooglePlay

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

    title: qsTr("Support")

    Rectangle {
        id: rectangle
        color: "#ffffff"
        anchors.fill: parent

        Button {
            id: btnSupportLevel0
            x: 30
            text: qsTr("Support Level Bronze")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: lblSupportInfo.bottom
            anchors.topMargin: 5
        }

        Button {
            id: btnSupportLevel1
            text: qsTr("Support Level Silver")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: btnSupportLevel0.bottom
            anchors.topMargin: 5
        }

        Button {
            id: btnSupportLevel2
            text: qsTr("Support Level Gold")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: btnSupportLevel1.bottom
            anchors.topMargin: 5
        }

        Button {
            id: btnClose
            x: 156
            y: 352
            text: qsTr("Close")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
        }

        Label {
            id: lblLevel0
            y: 18
            text: ""
            anchors.verticalCenter: btnSupportLevel0.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.left: btnSupportLevel0.right
            anchors.leftMargin: 21
        }

        Label {
            id: lblLevel1
            y: 18
            text: ""
            anchors.verticalCenter: btnSupportLevel1.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.left: btnSupportLevel1.right
            anchors.leftMargin: 21
        }

        Label {
            id: lblLevel2
            y: 18
            text: ""
            anchors.verticalCenter: btnSupportLevel2.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.left: btnSupportLevel2.right
            anchors.leftMargin: 21
        }

        Text {
            id: lblSupportInfo
            text: qsTr("The development of this app can be supported in various ways:\n\n* giving feedback and rating via the store enty in Google Play\n* purchasing a support level item via in app purchase (button below)\n\nPurchasing any support level will give you some more features:\n\n- graphics output for scripts\n- feature 2\n- feature 3\n")
            wrapMode: Text.WordWrap
            enabled: false
            horizontalAlignment: Text.AlignHCenter
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.top: parent.top
            anchors.topMargin: 5
        }

        Text {
            id: lblGooglePlay
            y: 18
            text: "<a href='https://play.google.com/store/apps/details?id=org.scintilla.sciteqt'>SciteQt in Google Play</a>"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: btnSupportLevel2.bottom
            anchors.topMargin: 15
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}D{i:2;anchors_x:30;anchors_y:41}D{i:6;anchors_x:391}
D{i:9;anchors_x:215;anchors_y:5}D{i:1;anchors_height:200;anchors_width:200;anchors_x:108;anchors_y:91}
}
##^##*/

