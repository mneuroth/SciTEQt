/*
 * Code taken from visiscript project and addapted for sciteqt.
 *
 * (C) 2015-2020 by Michael Neuroth
 *
 */

#ifndef SCRIPTEXECUTION_H
#define SCRIPTEXECUTION_H

#include <QObject>
#include <QString>
#include <QProcess>

// ************************************************************************

class ScriptExecution : public QObject
{
    Q_OBJECT

public:
    ScriptExecution();
    virtual ~ScriptExecution();

    int DoScriptExecution(const QString & sScriptCmd, const QString & sScriptArguments, const QString & sWorkingDirectory);
    void TerminateExecution();
    void KillExecution();

    QProcess & GetCurrentProcess();

    bool GetMeasureExecutionFlag() const;
    void SetMeasureExecutionFlag(bool value);
    qint64 GetLastExecutionTimeInMs() const;

private slots:
    void sltErrorScript(QProcess::ProcessError error);
    void sltFinishedScript(int exitValue, QProcess::ExitStatus status);
    void sltStateChanged(QProcess::ProcessState state);
    void sltReadyReadStandardErrorScript();
    void sltReadyReadStandardOutputScript();

signals:
    void AddToOutput( const QString & sText );

private:
    void ConnectScriptProcessSignals();
    void DisconnectScriptProcessSignals();

    void ProcessScriptFinished();
    QString GetExecutionTimeStrg() const;

    QProcess                m_aScriptProcess;

    QString                 m_sLastCurrentDir;  // temp

    bool                    m_bMeasureExecutionTime;
    qint64                  m_iElapsedTime;
};

#endif // SCRIPTEXECUTION_H
