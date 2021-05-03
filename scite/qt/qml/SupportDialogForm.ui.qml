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

    title: qsTr("Support")

    anchors.fill: parent

    property alias btnSupportLevel0: btnSupportLevel0
    property alias btnSupportLevel1: btnSupportLevel1
    property alias btnSupportLevel2: btnSupportLevel2
    property alias btnClose: btnClose
    property alias lblLevel0: lblLevel0
    property alias lblLevel1: lblLevel1
    property alias lblLevel2: lblLevel2
    property alias lblGooglePlay: lblGooglePlay
    property alias lblGithubHomePage: lblGithubHomePage

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

    ScrollView {
        id: scrollView

        anchors.fill: parent
        anchors.margins: 10

        contentWidth: lblSupportInfo.contentWidth // btnSupportLevel1.width //availableWidth
        //contentHeight: 600
        clip: true

        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            id: layout

            Text {
                id: lblSupportInfo
                text: qsTr("The development of this app can be supported in various ways:\n\n* giving feedback and rating via Google Play\n* giving feedback on the github project page\n* purchasing a support level item via in app purchase\n\nPurchasing any support level will give you some more features:\n\n- allows executing of lisp scripts via fuel interpreter\n- graphics output for scripts (comming soon)\n")
                wrapMode: Text.WordWrap
                enabled: false
                //horizontalAlignment: Text.AlignHCenter
                Layout.bottomMargin: 15
            }

            Row {
                id: row0

                spacing: 10

                Button {
                    id: btnSupportLevel0
                    text: qsTr("Support Level Bronze") //+" "+scrollView.width+" layout="+layout.width+" "+layout.parent+" "+layout.parent.width
                    width: lblSupportInfo.contentWidth / 2
                }

                Label {
                    id: lblLevel0
                    text: "?"
                    anchors.verticalCenter: btnSupportLevel0.verticalCenter
                }
            }

            Row {
                id: row1

                spacing: 10

                Button {
                    id: btnSupportLevel1
                    text: qsTr("Support Level Silver") //+" "+root.width+" r="+root+" "+scrollView+" "+scrollView.contentItem+" f="+scrollView.contentItem.contentItem
                    width: lblSupportInfo.contentWidth / 2
                }

                Label {
                    id: lblLevel1
                    text: "?"
                    anchors.verticalCenter: btnSupportLevel1.verticalCenter
                }
            }

            Row {
                id: row2

                spacing: 10

                Button {
                    id: btnSupportLevel2
                    text: qsTr("Support Level Gold") //+" "+parent.width+" w="+parent.parent.width+" parent="+parent.parent+" n="+parent.parent.objectName+" p="+parent
                    width: lblSupportInfo.contentWidth / 2
                }

                Label {
                    id: lblLevel2
                    text: "?"
                    anchors.verticalCenter: btnSupportLevel2.verticalCenter
                }
            }

            Text {
                id: lblGooglePlay
                text: "<a href='https://play.google.com/store/apps/details?id=org.scintilla.sciteqt'>SciteQt in Google Play</a>"
                Layout.topMargin: 15
            }

            Text {
                id: lblGithubHomePage
                text: "<a href='https://github.com/mneuroth/SciTEQt'>SciteQt Github project page</a>"
            }

            Button {
                id: btnClose
                text: qsTr("Close")
                Layout.topMargin: 25
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}D{i:2;anchors_x:30;anchors_y:41}D{i:6;anchors_x:391}
D{i:9;anchors_x:215;anchors_y:5}D{i:1;anchors_height:200;anchors_width:200;anchors_x:108;anchors_y:91}
}
##^##*/

