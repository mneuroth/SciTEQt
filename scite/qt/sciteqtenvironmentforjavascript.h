/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

#ifndef SCITEQTENVIRONMENTFORJAVASCRIPT_H
#define SCITEQTENVIRONMENTFORJAVASCRIPT_H

#include <QObject>

class SciteQtEnvironmentForJavaScript : public QObject
{
    Q_OBJECT
public:
    explicit SciteQtEnvironmentForJavaScript(bool & bIsAdmin, QString & sStyle, QString & sLanguage, QObject *parent = nullptr);

    // usage: env.print("hello world !")
    Q_INVOKABLE void print(const QString & text);

    // switch admin modus for mobile file dialog
    Q_INVOKABLE void admin(bool value);
    Q_INVOKABLE bool isAdmin() const;
    Q_INVOKABLE void style(const QString & value);
    Q_INVOKABLE QString getStyle() const;
    // to overwrite the language of the application
    Q_INVOKABLE void language(const QString & value);
    Q_INVOKABLE QString getLanguage() const;

signals:
    void OnPrint(const QString & text);
    void OnAdmin(bool value);

private:
    bool &      m_bIsAdmin;
    QString &   m_sStyle;
    QString &   m_sLanguage;
};

#endif // SCITEQTENVIRONMENTFORJAVASCRIPT_H
