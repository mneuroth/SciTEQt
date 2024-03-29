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
    property alias findButton: findButton
    property alias browseButton: browseButton
    property alias upButton: upButton
    property alias findWhatInput: findWhatInput
    property alias filesExtensionsInput: filesExtensionsInput
    property alias directoryInput: directoryInput
    property alias findWhatModel: findWhatModel
    property alias filesExtensionsModel: filesExtensionsModel
    property alias directoryModel: directoryModel
    property alias wholeWordCheckBox: wholeWordCheckBox
    property alias caseSensitiveCheckBox: caseSensitiveCheckBox
    property alias regularExpressionCheckBox: regularExpressionCheckBox
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
                //selectTextByMouse: true
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
                Layout.fillWidth: true
                text: localiseText(qsTr("&Find"),false)
                highlighted: !sciteQt.mobilePlatform
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // **************

            Label {
                id: filesLabel
                text: localiseText(qsTr("Files:"))
            }

            ComboBox {
                id: filesExtensionsInput
                Layout.columnSpan: 2
                Layout.fillWidth: true
                editable: true
                //selectTextByMouse: true
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
                Layout.fillWidth: true
                text: localiseText(qsTr("Cancel"),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // **************

            Label {
                id: directoryLabel
                text: localiseText(qsTr("Directory:"))
            }

            ComboBox {
                id: directoryInput
                Layout.fillWidth: true
                editable: false
                //selectTextByMouse: true
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
                implicitWidth: cancelButton.implicitWidth / 2   // TODO: browseButton.width / 4 causes the problem !
                text: localiseText(qsTr("&.."),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: browseButton
                Layout.fillWidth: true                          // TODO: has problems with older qt versions ! (with window resizeing)
                text: localiseText(qsTr("&Browse..."),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // **************

            Row {
                Layout.columnSpan: 4
                Layout.fillWidth: true

                CheckBox {
                    id: wholeWordCheckBox
                    //width: 150
                    text: localiseText(qsTr("Match &whole word only"))
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }

                CheckBox {
                    id: caseSensitiveCheckBox
                    //width: wholeWordCheckBox.width
                    text: localiseText(qsTr("Case sensiti&ve"))
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }

                CheckBox {
                    id: regularExpressionCheckBox
                    //width: wholeWordCheckBox.width
                    text: localiseText(qsTr("&Regular expression"))
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }
            }

            // **************

            Label {
                id: fillLabel
                text: ""
                visible: sciteQt.useMobileDialogHandling

                Layout.columnSpan: 4
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

