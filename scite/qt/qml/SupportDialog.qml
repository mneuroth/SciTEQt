/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.4
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.2

SupportDialogForm {
    id: root

    signal closed()

    btnSupportLevel0 {
        enabled: settings.supportLevel < 0
        onClicked: {
            storeLoader.item.supportLevel0.purchase()
        }
    }

    btnSupportLevel1 {
        enabled: settings.supportLevel < 1
        onClicked: {
            storeLoader.item.supportLevel1.purchase()
        }
    }

    btnSupportLevel2 {
        enabled: settings.supportLevel < 2
        onClicked: {
            storeLoader.item.supportLevel2.purchase()
        }
    }

    lblGooglePlay {
        onLinkActivated: Qt.openUrlExternally(link)
    }

    btnClose {
        onClicked:  {
            closed()
        }
    }

    onVisibleChanged: {    
        var store = storeLoader !== null ? storeLoader.item : null
        if(store!==null) {
            lblLevel0.text = qsTr("Price: ") + store.supportLevel0.price
            lblLevel1.text = qsTr("Price: ") + store.supportLevel1.price
            lblLevel2.text = qsTr("Price: ") + store.supportLevel2.price
            //lblLevel0.text = qsTr("Status=") + store.supportLevel0.status + qsTr(" price: ") + store.supportLevel0.price
            //lblLevel1.text = qsTr("Status=") + store.supportLevel1.status + qsTr(" price: ") + store.supportLevel1.price
            //lblLevel2.text = qsTr("Status=") + store.supportLevel2.status + qsTr(" price: ") + store.supportLevel2.price
        }  
    }
}
