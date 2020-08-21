#include "ScintillaEditBase.h"

#include <QtQuick/QQuickView>
#include <QGuiApplication>
#include <QApplication>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "applicationdata.h"
#include "sciteqt.h"

#include <qhtml5file/qhtmlfileaccess.h>

#ifndef Q_OS_WIN
#define _WITH_QDEBUG_REDIRECT
#define _WITH_ADD_TO_LOG
#endif

#include <QDir>
#include <QDateTime>

#include "ILexer.h"
#include "Scintilla.h"
//#include "SciLexer.h"
#include "Lexilla.h"
#include "LexillaLibrary.h"

static qint64 g_iLastTimeStamp = 0;

void AddToLog(const QString & msg)
{
#ifdef _WITH_ADD_TO_LOG
    QString sFileName(LOG_NAME);
    //if( !QDir("/sdcard/Texte").exists() )
    //{
    //    sFileName = "mgv_quick_qdebug.log";
    //}
    QFile outFile(sFileName);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 delta = now - g_iLastTimeStamp;
    g_iLastTimeStamp = now;
    ts << delta << " ";
    ts << msg << endl;
    qDebug() << delta << " " << msg << endl;
    outFile.flush();
#else
    Q_UNUSED(msg)
#endif
}

#ifdef _WITH_QDEBUG_REDIRECT
#include <QDebug>
void PrivateMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & msg)
{
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtInfoMsg:
        txt = QString("Info: %1 (%2:%3, %4)").arg(msg).arg(context.file).arg(context.line).arg(context.function);
        break;
    }
    AddToLog(txt);
}
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    QQuickStyle::setStyle("Fusion");
    //QQuickStyle::setStyle("Material");
    //QQuickStyle::setStyle("Universal");
#elif defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    //QQuickStyle::setStyle("Default");
#endif

#ifdef Q_OS_ANDROID
    UnpackFiles();
    qputenv(SCITE_HOME, FILES_DIR);
#endif

    qRegisterMetaType<SCNotification>("SCNotification");
    qRegisterMetaType<SCNotification>("uptr_t");
    qRegisterMetaType<SCNotification>("sptr_t");
    qmlRegisterType<SciTEQt>("org.scintilla.sciteqt", 1, 0, "SciTEQt");
    // need external function to register for mingw, otherwise we get an unresolved external errror when linking
    //qmlRegisterType<ScintillaEditBase>("org.scintilla.scintilla", 1, 0, "ScintillaEditBase");
    RegisterScintillaType();

#ifndef NO_EXTENSIONS
    MultiplexExtension multiExtender;

#ifndef NO_LUA
    multiExtender.RegisterExtension(LuaExtension::Instance());
#endif

#ifndef NO_FILER
    //multiExtender.RegisterExtension(DirectorExtension::Instance());
#endif
#endif

    LexillaSetDefaultDirectory(/*GetSciTEPath(FilePath()).AsUTF8()*/".");
    Scintilla_LinkLexers();
    //Scintilla_RegisterClasses(hInstance);
    LexillaSetDefault([](const char *name) {
        return CreateLexer(name);
    });

#ifdef _WITH_QDEBUG_REDIRECT
    qInstallMessageHandler(PrivateMessageHandler);
#endif

    QApplication app(argc, argv);
    app.setOrganizationName("scintilla.org");
    app.setOrganizationDomain("scintilla.org");
    app.setApplicationName("SciTEQt");

    app.setWindowIcon(QIcon("scite_logo.png"));

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/app.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    ApplicationData data(0, &multiExtender, engine);
    engine.rootContext()->setContextProperty("applicationData", &data);

    QHtmlFileAccess htmlFileAccess(qApp);
    engine.rootContext()->setContextProperty("htmlFileAccess", &htmlFileAccess);

    engine.load(url);

    //qDebug() << "LOAD QML DONE" << endl;

    return app.exec();
}
