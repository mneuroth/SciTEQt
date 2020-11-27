/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

#include "sciteqtenvironmentforjavascript.h"

SciteQtEnvironmentForJavaScript::SciteQtEnvironmentForJavaScript(bool & bIsAdmin, QObject *parent)
    : QObject(parent), m_bIsAdmin(bIsAdmin)
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
