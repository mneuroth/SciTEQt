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
#include <QVariant>

class SciTEQt;

class SciteQtEnvironmentForJavaScript : public QObject
{
    Q_OBJECT
public:
    explicit SciteQtEnvironmentForJavaScript(bool & bIsAdmin, QString & sStyle, QString & sLanguage, SciTEQt * pSciteQt, QObject *parent = nullptr);

    // usage: env.print("hello world !")
    Q_INVOKABLE void print(const QString & text, const QVariant & val2 = QVariant(), const QVariant & val3 = QVariant(), const QVariant & val4 = QVariant(), const QVariant & val5 = QVariant(), const QVariant & val6 = QVariant(), const QVariant & val7 = QVariant(), const QVariant & val8 = QVariant(), const QVariant & val9 = QVariant(), const QVariant & val10 = QVariant(), bool newLine = false);
    Q_INVOKABLE void println(const QString & text = "", const QVariant & val2 = QVariant(), const QVariant & val3 = QVariant(), const QVariant & val4 = QVariant(), const QVariant & val5 = QVariant(), const QVariant & val6 = QVariant(), const QVariant & val7 = QVariant(), const QVariant & val8 = QVariant(), const QVariant & val9 = QVariant(), const QVariant & val10 = QVariant());

    // switch admin modus for mobile file dialog
    Q_INVOKABLE void admin(bool value);
    Q_INVOKABLE bool isAdmin() const;
    Q_INVOKABLE void style(const QString & value);
    Q_INVOKABLE QString getStyle() const;
    // to overwrite the language of the application
    Q_INVOKABLE void language(const QString & value);
    Q_INVOKABLE QString getLanguage() const;

    Q_INVOKABLE int messageBox(const QString & text, int style = 0) const;

signals:
    void OnPrint(const QString & text);
    void OnAdmin(bool value);

private:
    SciTEQt *   m_pSciteQt;     // not an owner !
    bool &      m_bIsAdmin;
    QString &   m_sStyle;
    QString &   m_sLanguage;
};

#endif // SCITEQTENVIRONMENTFORJAVASCRIPT_H
