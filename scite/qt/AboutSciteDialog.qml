import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.9
import org.scintilla.scintilla 1.0

Window {
    id: root

    width: 500
    height: 500

    property alias scintilla: aboutText.quickScintillaEditor
    property alias closeButton: closeButton

    property var fcnLocalisation: undefined

    function localiseText(text) {
        if(fcnLocalisation !== undefined) {
            return fcnLocalisation(text)
        }
        return text
    }

    ScintillaText {
        id: aboutText

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: closeButton.top
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5
    }

    Button {
        id: closeButton

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.leftMargin: 5
        anchors.topMargin: 5
        anchors.bottomMargin: 5

        text: localiseText("OK")
        Keys.onEscapePressed: closeButton.clicked()
        Keys.onBackPressed: closeButton.clicked()
    }
}
