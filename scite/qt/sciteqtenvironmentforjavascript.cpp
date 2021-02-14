/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

#include "sciteqtenvironmentforjavascript.h"

#include "sciteqt.h"

SciteQtEnvironmentForJavaScript::SciteQtEnvironmentForJavaScript(bool & bIsAdmin, QString & sStyle, QString & sLanguage, SciTEQt * pSciteQt, QObject *parent)
    : QObject(parent), m_pSciteQt(pSciteQt), m_bIsAdmin(bIsAdmin), m_sStyle(sStyle), m_sLanguage(sLanguage)
{
}

void SciteQtEnvironmentForJavaScript::print(const QString & text, const QVariant & val2, const QVariant & val3, const QVariant & val4, const QVariant & val5, const QVariant & val6, const QVariant & val7, const QVariant & val8, const QVariant & val9, const QVariant & val10, bool newLine)
{
    QString out = text;
    if( val2.isValid() )
    {
        out += val2.toString();
    }
    if( val3.isValid() )
    {
        out += val3.toString();
    }
    if( val4.isValid() )
    {
        out += val4.toString();
    }
    if( val5.isValid() )
    {
        out += val5.toString();
    }
    if( val6.isValid() )
    {
        out += val6.toString();
    }
    if( val7.isValid() )
    {
        out += val7.toString();
    }
    if( val8.isValid() )
    {
        out += val8.toString();
    }
    if( val9.isValid() )
    {
        out += val9.toString();
    }
    if( val10.isValid() )
    {
        out += val10.toString();
    }
    if( newLine )
    {
        out += "\n";
    }
    emit OnPrint(out);
}

void SciteQtEnvironmentForJavaScript::println(const QString & text, const QVariant & val2, const QVariant & val3, const QVariant & val4, const QVariant & val5, const QVariant & val6, const QVariant & val7, const QVariant & val8, const QVariant & val9, const QVariant & val10)
{
    print(text,val2,val3,val4,val5,val6,val7,val8,val9,val10,true);
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

void SciteQtEnvironmentForJavaScript::language(const QString & value)
{
    m_sLanguage = value;
}

QString SciteQtEnvironmentForJavaScript::getLanguage() const
{
    return m_sLanguage;
}

int SciteQtEnvironmentForJavaScript::messageBox(const QString & text, int style) const
{
    if( m_pSciteQt != 0 )
    {
        return m_pSciteQt->ShowWindowMessageBox(text, style);
    }
    return -1;
}
