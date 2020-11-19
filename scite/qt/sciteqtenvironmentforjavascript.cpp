/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

#include "sciteqtenvironmentforjavascript.h"

#include <QDebug>

SciteQtEnvironmentForJavaScript::SciteQtEnvironmentForJavaScript(QObject *parent) : QObject(parent)
{
}

void SciteQtEnvironmentForJavaScript::print(const QString & text)
{
    emit OnPrint(text);
}

void SciteQtEnvironmentForJavaScript::admin(bool value)
{
    emit OnAdmin(value);
}
