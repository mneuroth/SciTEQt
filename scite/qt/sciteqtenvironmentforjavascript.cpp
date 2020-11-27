/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

#include "sciteqtenvironmentforjavascript.h"

SciteQtEnvironmentForJavaScript::SciteQtEnvironmentForJavaScript(bool & bIsAdmin, QString & sStyle, QObject *parent)
    : QObject(parent), m_bIsAdmin(bIsAdmin), m_sStyle(sStyle)
{
}

void SciteQtEnvironmentForJavaScript::print(const QString & text)
{
    emit OnPrint(text);
}

void SciteQtEnvironmentForJavaScript::admin(bool value)
{
    m_bIsAdmin = value;
    emit OnAdmin(value);
}

bool SciteQtEnvironmentForJavaScript::isAdmin() const
{
    return m_bIsAdmin;
}

void SciteQtEnvironmentForJavaScript::style(const QString & value)
{
    m_sStyle = value;
}

QString SciteQtEnvironmentForJavaScript::getStyle() const
{
    return m_sStyle;
}
