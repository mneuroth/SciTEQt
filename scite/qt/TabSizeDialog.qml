import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.2

TabSizeDialogForm {
    id: root

    signal canceled()
    signal accepted()
    signal convert()

    tabSizeInput {
        validator: IntValidator { bottom: 0; top: 99 }

        onAccepted: okButton.clicked()
    }

    indentSizeInput {
        validator: IntValidator { bottom: 0; top: 99 }

        onAccepted: okButton.clicked()
    }

    cancelButton {
        onClicked: {
            root.close()
            canceled()
        }
    }

    okButton {
        onClicked: {
            root.close()
            accepted()
        }
    }

    convertButton {
        onClicked: {
            root.close()
            convert()
        }
    }
}
