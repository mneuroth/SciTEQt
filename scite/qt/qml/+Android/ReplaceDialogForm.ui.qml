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
    property alias findNextButton: findNextButton
    property alias replaceButton: replaceButton
    property alias replaceAllButton: replaceAllButton
    property alias replaceInSectionButton: replaceInSectionButton
    property alias findWhatInput: findWhatInput
    property alias findWhatModel: findWhatModel
    property alias replaceWithInput: replaceWithInput
    property alias replaceWithModel: replaceWithModel
    property alias matchWholeWordCheckBox: matchWholeWordCheckBox
    property alias caseSensitiveCheckBox: caseSensitiveCheckBox
    property alias regularExpressionCheckBox: regularExpressionCheckBox
    property alias wrapAroundCheckBox: wrapAroundCheckBox
    property alias tramsformBackslashExprCheckBox: tramsformBackslashExprCheckBox
    property alias countReplacementsLabel: countReplacementsLabel
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
            rows: 10

            // ***** row 0

            Label {
                id: findLabel
                text: localiseText(qsTr("Find what:"))
                Layout.row: 0
                Layout.column: 0
            }

            ComboBox {
                id: findWhatInput
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.row: 0
                Layout.column: 1
                //implicitWidth: 300
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

            // ***** row 1

            Label {
                id: replaceLabel
                text: localiseText(qsTr("Replace with:"))
                Layout.row: 1
                Layout.column: 0
            }

            ComboBox {
                id: replaceWithInput
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.row: 1
                Layout.column: 1
                editable: true
                model: replaceWithModel

                //font.pixelSize: 12
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            ListModel {
                id: replaceWithModel
                objectName: "replaceWithModel"
            }

            // ***** row 2

            CheckBox {
                id: matchWholeWordCheckBox
                Layout.columnSpan: 3
                Layout.row: 2
                Layout.column: 0
                text: localiseText(qsTr("Match &whole word only"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // ***** row 3

            CheckBox {
                id: caseSensitiveCheckBox
                Layout.columnSpan: 3
                Layout.row: 3
                Layout.column: 0
                text: localiseText(qsTr("Case sensiti&ve"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // ***** row 4

            CheckBox {
                id: regularExpressionCheckBox
                Layout.columnSpan: 3
                Layout.row: 4
                Layout.column: 0
                text: localiseText(qsTr("Regular &expression"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // ***** row 5

            CheckBox {
                id: wrapAroundCheckBox
                Layout.columnSpan: 2
                Layout.row: 5
                Layout.column: 0
                text: localiseText(qsTr("Wrap ar&ound"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // ***** row 6

            CheckBox {
                id: tramsformBackslashExprCheckBox
                Layout.columnSpan: 4
                Layout.row: 6
                Layout.column: 0
                text: localiseText(qsTr("Transform &backslash expressions"))
                Keys.onEscapePressed: cancelButton.clicked()
                Keys.onBackPressed: cancelButton.clicked()
            }

            // ***** row 7

            Label {
                id: countReplacementsLabelLabel
                text: localiseText(qsTr("Replacements:"))
                Layout.row: 7
                Layout.column: 0
            }

            TextField {
                id: countReplacementsLabel
                implicitWidth: 50
                readOnly: true
                text: "0"
                Layout.row: 7
                Layout.column: 1
            }

            // ***** row 8

            Flow {

                Layout.columnSpan: 4
                Layout.row: 8
                Layout.column: 0
                Layout.fillWidth: true

                spacing: 10

                Button {
                    id: findNextButton
                    text: localiseText(qsTr("&Find Next"),false)
                    highlighted: true
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }

                Button {
                    id: replaceButton
                    text: localiseText(qsTr("&Replace"),false)
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }

                Button {
                    id: replaceAllButton
                    text: localiseText(qsTr("Replace &All"),false)
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }

                Button {
                    id: replaceInSectionButton
                    text: localiseText(qsTr("Replace in &Section"),false)
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }

                Button {
                    id: cancelButton
                    text: localiseText(qsTr("Close"),false)
                    Keys.onEscapePressed: cancelButton.clicked()
                    Keys.onBackPressed: cancelButton.clicked()
                }
            }

            // ***** row 9

            Label {
                id: fillLabel
                text: ""
                visible: sciteQt.useMobileDialogHandling

                Layout.columnSpan: 4
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.row: 9
                Layout.column: 0
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:1.5;height:480;width:640}
}
##^##*/

