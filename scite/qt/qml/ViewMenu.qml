/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 2.9
import QtQml.Models 2.14

AutoSizingMenu {
    id: viewMenu
    title: processMenuItem(qsTr("&View"),null)

    // use simpler menu for mobile platforms with less menu items
    property bool useSimpleMenu: false
    property var actions: null

    MenuItem {
        id: actionToggleCurrentFold
        text: processMenuItem2(actions.actionToggleCurrentFold.text, actionToggleCurrentFold)
        action: actions.actionToggleCurrentFold
    }
    MenuItem {
        id: actionToggleAllFolds
        text: processMenuItem2(actions.actionToggleAllFolds.text, actionToggleAllFolds)
        action: actions.actionToggleAllFolds
    }
    MenuSeparator {}
    MenuItem {
        id: actionFullScreen
        text: processMenuItem2(actions.actionFullScreen.text, actionFullScreen)
        action: actions.actionFullScreen
    }
    MenuItem {
        id: actionIsMobilePlatfrom
        text: processMenuItem2(actions.actionIsMobilePlatfrom.text, actionIsMobilePlatfrom)
        action: actions.actionIsMobilePlatfrom
    }
    MenuItem {
        id: actionShowToolBar
        text: processMenuItem2(actions.actionShowToolBar.text, actionShowToolBar)
        action: actions.actionShowToolBar
    }
    MenuItem {
        id: actionShowTabBar
        text: processMenuItem2(actions.actionShowTabBar.text, actionShowTabBar)
        action: actions.actionShowTabBar
    }
    MenuItem {
        id: actionShowStatusBar
        text: processMenuItem2(actions.actionShowStatusBar.text, actionShowStatusBar)
        action: actions.actionShowStatusBar
    }
    MenuSeparator {}
    MenuItem {
        id: actionShowWhitespace
        text: processMenuItem2(actions.actionShowWhitespace.text, actionShowWhitespace)
        action: actions.actionShowWhitespace
    }
    MenuItem {
        id: actionShowEndOfLine
        text: processMenuItem2(actions.actionShowEndOfLine.text, actionShowEndOfLine)
        action: actions.actionShowEndOfLine
    }
    MenuItem {
        id: actionIndentaionGuides
        text: processMenuItem2(actions.actionIndentaionGuides.text, actionIndentaionGuides)
        action: actions.actionIndentaionGuides
    }
    MenuItem {
        id: actionLineNumbers
        text: processMenuItem2(actions.actionLineNumbers.text, actionLineNumbers)
        action: actions.actionLineNumbers
    }
    MenuItem {
        id: actionMargin
        text: processMenuItem2(actions.actionMargin.text, actionMargin)
        action: actions.actionMargin
    }
    MenuItem {
        id: actionFoldMargin
        text: processMenuItem2(actions.actionFoldMargin.text, actionFoldMargin)
        action: actions.actionFoldMargin
    }
    MenuItem {
        id: actionToggleOutput
        text: processMenuItem2(actions.actionToggleOutput.text, actionToggleOutput)
        action: actions.actionToggleOutput
    }
    MenuItem {
        id: actionParameters
        text: processMenuItem2(actions.actionParameters.text, actionParameters)
        action: actions.actionParameters
    }
}
