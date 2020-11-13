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
//import QtQuick.Dialogs 1.3
import QtQuick.Window 2.3

Window {

    //width: 500
    //height: 130
    width: grid.implicitWidth+10
    height: grid.implicitHeight+10

    //flags: Qt.MSWindowsFixedSizeDialogHint

    // Window is not resizable !
    maximumHeight: height
    minimumHeight: height
/*
    maximumWidth: width
    minimumWidth: width
*/

    property alias cancelButton: cancelButton
    property alias findNextButton: findNextButton
    property alias markAllButton: markAllButton
    property alias findWhatInput: findWhatInput
    property alias findWhatModel: findWhatModel
    property alias searchUpButton: searchUpButton
    property alias searchDownButton: searchDownButton
    property alias matchWholeWordCheckBox: matchWholeWordCheckBox
    property alias caseSensitiveCheckBox: caseSensitiveCheckBox
    property alias regularExpressionCheckBox: regularExpressionCheckBox
    property alias wrapAroundCheckBox: wrapAroundCheckBox
    property alias tramsformBackslashExprCheckBox: tramsformBackslashExprCheckBox

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
            rows: 7

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
                id: findNextButton
                text: localiseText(qsTr("&Find Next"),false)
                highlighted: true
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // *****

            CheckBox {
                id: matchWholeWordCheckBox
                Layout.columnSpan: 2
                text: localiseText(qsTr("Match &whole word only"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Rectangle {
                Layout.rowSpan: 2
                Layout.fillWidth: true
                border.color: "light grey"
                border.width: 1
                Layout.preferredWidth: direction.width
                Layout.preferredHeight: direction.height

                ColumnLayout {
                    id: direction

                    RadioButton {
                        id: searchUpButton
                        text: localiseText(qsTr("&Up"))
                    }
                    RadioButton {
                        id: searchDownButton
                        checked: true
                        text: localiseText(qsTr("&Down"))
                    }
                }
            }

            Button {
                id: markAllButton
                text: localiseText(qsTr("&Mark All"),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // *****

            CheckBox {
                id: caseSensitiveCheckBox
                Layout.columnSpan: 2
                text: localiseText(qsTr("Case sensiti&ve"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            Button {
                id: cancelButton
                text: localiseText(qsTr("Cancel"),false)
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // *****

            CheckBox {
                id: regularExpressionCheckBox
                Layout.columnSpan: 3
                text: localiseText(qsTr("Regular &expression"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // *****

            CheckBox {
                id: wrapAroundCheckBox
                Layout.columnSpan: 3
                text: localiseText(qsTr("Wrap ar&ound"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // *****

            CheckBox {
                id: tramsformBackslashExprCheckBox
                Layout.columnSpan: 3
                text: localiseText(qsTr("Transform &backslash expressions"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // *****
/*
            CheckBox {
                id: searchOnlyInThisStyleCheckBox
                Layout.columnSpan: 3
                text: localiseText(qsTr("Search only in &this style:"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }
*/
            Label {
                id: fillLabel
                text: ""
                visible: sciteQt.mobilePlatform

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

