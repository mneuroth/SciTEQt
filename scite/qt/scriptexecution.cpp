/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
/*
 * Code taken from visiscript project and addapted for sciteqt.
 *
 * (C) 2015-2020 by Michael Neuroth
 *
 */

#include "scriptexecution.h"

#include <QElapsedTimer>
#include <QtGlobal>
#include <QDir>
#include <QFileInfo>

// ************************************************************************

#if defined(Q_OS_WIN)
#define PATH_ENVIRONMENT_SEP        ";"
#else
#define PATH_ENVIRONMENT_SEP        ":"
#endif

static QElapsedTimer g_aScriptTimer;

// ************************************************************************

// split a string into substrings but processing c-strings correctly:
// example:
//  input:  abc "text with spaces" option
//  output: ["abc","text with spaces","option"]
static QStringList SplitString( const QString & s )
{
    QStringList lstRet;
    QString temp;
    bool bItemFound = false;
    bool bFoundString = false;
    for( int i=0; i<s.length(); i++ )
    {
        const QChar & ch = s[i];
        if( ch=='"' )
        {
            bFoundString = !bFoundString;
        }
        else if( ch=='\\' )
        {
            if( i+1<s.length() && s[i+1]=='"' )
            {
                i++;
                temp += s[i];
            }
        }
        else if( ch.isSpace() )
        {
            if( bFoundString )
            {
                temp += ch;
            }
            else
            {
                if( bItemFound )
                {
                    lstRet.append(temp);
                    temp = "";
                }
                bItemFound = false;
            }
        }
        else if( !bItemFound )
        {
            bItemFound = true;
            temp += ch;
        }
        else
        {
            temp += ch;
        }
    }
    if( temp.length()>0 )
    {
       lstRet.append(temp);
    }
    return lstRet;
}

// ************************************************************************

ScriptExecution::ScriptExecution(bool isMobilePlatform)
    : m_bMeasureExecutionTime(false),
      m_bIsMobilePlatform(isMobilePlatform),
      m_iElapsedTime(0)
{
    ConnectScriptProcessSignals();
}

ScriptExecution::~ScriptExecution()
{
    DisconnectScriptProcessSignals();
}

#if !defined(Q_OS_WASM)
QProcess & ScriptExecution::GetCurrentProcess()
{
    return m_aScriptProcess;
}
#endif

bool ScriptExecution::GetMeasureExecutionFlag() const
{
    return m_bMeasureExecutionTime;
}

void ScriptExecution::SetMeasureExecutionFlag(bool value)
{
    m_bMeasureExecutionTime = value;
}

qint64 ScriptExecution::GetLastExecutionTimeInMs() const
{
    return m_iElapsedTime;
}

void ScriptExecution::ConnectScriptProcessSignals()
{
#if !defined(Q_OS_WASM)
    connect(&m_aScriptProcess,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(sltFinishedScript(int,QProcess::ExitStatus)));
    connect(&m_aScriptProcess,SIGNAL(errorOccurred(QProcess::ProcessError)),this,SLOT(sltErrorScript(QProcess::ProcessError)));
    connect(&m_aScriptProcess,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(sltStateChanged(QProcess::ProcessState)));
    connect(&m_aScriptProcess,SIGNAL(readyReadStandardError()),this,SLOT(sltReadyReadStandardErrorScript()));
    connect(&m_aScriptProcess,SIGNAL(readyReadStandardOutput()),this,SLOT(sltReadyReadStandardOutputScript()));
    //connect(&m_aScriptProcess,SIGNAL(readyRead()),this,SLOT(sltReadyRead()));
#endif
}

void ScriptExecution::DisconnectScriptProcessSignals()
{
#if !defined(Q_OS_WASM)
    disconnect(&m_aScriptProcess,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(sltFinishedScript(int,QProcess::ExitStatus)));
    disconnect(&m_aScriptProcess,SIGNAL(errorOccurred(QProcess::ProcessError)),this,SLOT(sltErrorScript(QProcess::ProcessError)));
    disconnect(&m_aScriptProcess,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(sltStateChanged(QProcess::ProcessState)));
    disconnect(&m_aScriptProcess,SIGNAL(readyReadStandardError()),this,SLOT(sltReadyReadStandardErrorScript()));
    disconnect(&m_aScriptProcess,SIGNAL(readyReadStandardOutput()),this,SLOT(sltReadyReadStandardOutputScript()));
    //disconnect(&m_aScriptProcess,SIGNAL(readyRead()),this,SLOT(sltReadyRead()));
#endif
}

QString ScriptExecution::GetExecutionTimeStrg() const
{
    return QString(tr(">Execution time: %1 ms")).arg(m_iElapsedTime)+"\n";
}

void ScriptExecution::ProcessScriptFinished()
{
}

#if !defined(Q_OS_WASM)
void AddToEnvironmentVariable(QProcessEnvironment & env, const QString & key, const QString & valueToAdd)
{
    if( env.contains(key) )
    {
        QString currentValue = env.value(key);
        if( currentValue.size()>0 )
        {
            currentValue += PATH_ENVIRONMENT_SEP;
            currentValue += QDir::toNativeSeparators(valueToAdd);
            env.insert(key, currentValue);
        }
    }
}
#endif

int ScriptExecution::DoScriptExecution(const QString & sScriptCmd, const QString & sScriptArguments, const QString & sWorkingDirectory)
{
    // save the actual directory
    m_sLastCurrentDir = QDir::currentPath();
    // get the path to directory of the actual script
    QString strScriptActDir = QFileInfo(sWorkingDirectory).absolutePath();
    QDir::setCurrent(strScriptActDir);

    QStringList lstArguments;
    QString sOutput;
    const QString & sScriptCommand = sScriptCmd;

    if( m_bMeasureExecutionTime )
    {
        emit AddToOutput(tr(">Measure execution time\n"));
    }

    emit AddToOutput(QString(">")+sScriptCmd+" "+sScriptArguments+"\n");

    QStringList scriptArgs = SplitString(sScriptArguments);
    foreach( const QString & s, scriptArgs )
    {
        if( s.length()>0 )
        {
            lstArguments << s;
        }
    }

    m_iElapsedTime = 0;
    g_aScriptTimer.start();

#if !defined(Q_OS_WASM)
    if( m_aScriptProcess.state()==QProcess::NotRunning )
    {
        QStringList args;

        foreach( const QString & sArg, lstArguments )
        {
            if( sArg.length()>0 )
            {
                args << sArg;
            }
        }

        // set system environment to find all available script engines
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        //QFileInfo aScriptInfo(sScriptCommand);
        //QString sPathToInterpreterDirectory = aScriptInfo.path();
        // maybe: update some environment variables via:
        //env.insert("LD_LIBRARY_PATH", sLibraryPath);
#if defined( Q_OS_ANDROID )
        // added support to call interpreters from visiscriptextensions
        QString sCurrentLibraryPath;
        if( env.contains("LD_LIBRARY_PATH") )
        {
            sCurrentLibraryPath = env.value("LD_LIBRARY_PATH");
        }
        QString sLibraryPath = "/data/data/de.mneuroth.visiscriptextensions/files/python2.7/lib";   // maybe add also this path: /data/data/de.mneuroth.visiscriptextensions/files/python2.7/lib/python2.7/lib-dynload
        if( sCurrentLibraryPath.length() > 0 )
        {
            sLibraryPath = sLibraryPath + ":" + sCurrentLibraryPath;
        }
        env.insert("LD_LIBRARY_PATH", sLibraryPath);

        QString sLuaPaths = "?;?.lua;/data/data/de.mneuroth.visiscriptextensions/files/?.lua;/data/data/de.mneuroth.visiscriptextensions/files/lib/lua/?.lua";
        env.insert("LUA_PATH", sLuaPaths);
        QString sMinscriptPaths = "/data/data/de.mneuroth.visiscriptextensions/files";  // lib/minscript
        env.insert("MINSCRIPTPATH", sMinscriptPaths);
        QString sFuelPaths = "/data/data/de.mneuroth.visiscriptextensions/files";  // lib/fuel
        env.insert("FUELPATH", sFuelPaths);
#endif
        m_aScriptProcess.setProcessEnvironment( env );

        m_aScriptProcess.setWorkingDirectory(sWorkingDirectory);
        m_aScriptProcess.start( sScriptCommand, args, QIODevice::Unbuffered|QIODevice::ReadWrite/*|QIODevice::Text*/ );

        bool ok = m_aScriptProcess.waitForStarted();    // AddStringKommasIfNeededAndConvertSeparators
        if(!ok)
        {
            sOutput += QString(tr(">Error starting process: Id=%1 Message=%2")).arg(m_aScriptProcess.error()).arg(m_aScriptProcess.errorString());
        }
        else
        {
            ok = m_aScriptProcess.waitForFinished(-1);
            if(!ok)
            {
                if( sOutput.length()>0 )
                {
                    sOutput += "\n";
                }
                sOutput += QString(tr(">Error running process; Id=%1 Message=%2")).arg(m_aScriptProcess.error()).arg(m_aScriptProcess.errorString());
            }
        }
    }
    else
    {
        sOutput += tr(">Warning: could not start script because script is already running !");
    }
#endif

    // show the output of the script in any case
    if( sOutput.length()>0 )
    {
        emit AddToOutput( sOutput );
    }

#if !defined(Q_OS_WASM)
    return m_aScriptProcess.exitCode();
#else
    return -1;
#endif
}

void ScriptExecution::TerminateExecution()
{
    emit AddToOutput(tr(">Process failed to respond; forcing abrupt termination...\n"));

#if !defined(Q_OS_WASM)
    m_aScriptProcess.terminate();
#endif
}

void ScriptExecution::KillExecution()
{
    emit AddToOutput(tr(">Process failed to respond; forcing abrupt termination...\n"));

#if !defined(Q_OS_WASM)
    m_aScriptProcess.kill();
#endif
}

#if !defined(Q_OS_WASM)
void ScriptExecution::sltErrorScript(QProcess::ProcessError error)
{
    QString errText = m_aScriptProcess.readAllStandardError();
    if( errText.length()>0 )
    {
        emit AddToOutput(errText);
    }
    errText = "";
    switch( error )
    {
        case QProcess::FailedToStart:
            errText = tr(">Error: failed to start script.\n");
            if( m_bIsMobilePlatform )
            {
                errText += tr(">You can install the VisiScriptExtensions app from Google Play for additional scripting languages.\n");
                errText += tr(">See help menu or this link: ")+URL_VISISCRIPTEXTENSIONS+"\n";
            }
            break;
        case QProcess::Crashed:
            errText = tr(">Error: script crashed.\n");
            break;
        case QProcess::Timedout:
            errText = tr(">Error: script timeout.\n");
            break;
        default:
            errText = tr(">Error: Unknown error.\n");
            break;
    }
    if( errText.length()>0 )
    {
        emit AddToOutput(errText);
    }
    m_iElapsedTime = g_aScriptTimer.elapsed();
    if( m_bMeasureExecutionTime )
    {
        emit AddToOutput( GetExecutionTimeStrg() );
    }
    ProcessScriptFinished();
}

void ScriptExecution::sltFinishedScript(int exitValue, QProcess::ExitStatus status)
{
    Q_UNUSED(exitValue);
    if( status==QProcess::NormalExit)
    {
        QString sResult = m_aScriptProcess.readAll();
        QString errText = m_aScriptProcess.readAllStandardError();
        if( sResult.length()>0 )
        {
            emit AddToOutput(sResult);
        }
        if( errText.length()>0 )
        {
            emit AddToOutput(errText);
        }
    }
    // set current directory back, if needed
    if( m_sLastCurrentDir.length()>0 )
    {
        QDir::setCurrent(m_sLastCurrentDir);
    }

    emit AddToOutput(tr(">Exit code: %1\n").arg(exitValue));

    m_iElapsedTime = g_aScriptTimer.elapsed();
    if( m_bMeasureExecutionTime )
    {
        emit AddToOutput(GetExecutionTimeStrg());
    }
    m_aScriptProcess.closeWriteChannel();
    m_aScriptProcess.closeReadChannel(QProcess::StandardOutput);
    m_aScriptProcess.closeReadChannel(QProcess::StandardError);
    m_aScriptProcess.close();
    ProcessScriptFinished();
}

void ScriptExecution::sltStateChanged(QProcess::ProcessState state)
{
    Q_UNUSED(state);
}
#endif

void ScriptExecution::sltReadyReadStandardErrorScript()
{
#if !defined(Q_OS_WASM)
    //if( m_aScriptProcess.state()!=QProcess::NotRunning )
    {
        QString sResult = /*QString::fromLocal8Bit?*/(m_aScriptProcess.readAllStandardError());
        if( sResult.length()>0 )
        {
            emit AddToOutput(sResult);
        }
    }
#endif
}

void ScriptExecution::sltReadyReadStandardOutputScript()
{
#if !defined(Q_OS_WASM)
    //if( m_aScriptProcess.state()!=QProcess::NotRunning )
    {
        QString sResult = m_aScriptProcess.readAllStandardOutput();
        if( sResult.length()>0 )
        {
// TODO: for graphics output processing... call: GraphicsOutputExtension::ProcessStdOutLine()
            emit AddToOutput(sResult);
        }
    }
#endif
}
