/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

#include "sciteqt.h"

#include <QUrl>
#include <QDir>
#include <QResource>
#include <QThread>
#include <QGuiApplication>
#include <QClipboard>
#include <QQmlContext>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDateTime>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include <QGuiApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QApplication>

#include <QJsonObject>
#include <QJsonArray>

#include <qdesktopservices.h>

#include <QDebug>

#include <sstream>

#include "ScintillaEditBase.h"
#include "ScintillaQt.h"

#include "findinfiles.h"

#include "sciteqtenvironmentforjavascript.h"

#ifdef Q_OS_WASM
#include <emscripten.h>
#endif

//*************************************************************************

#define __SCITE_QT_VERSION__   "0.99.9"

enum {
    WORK_EXECUTE = WORK_PLATFORM + 1,
    TRIGGER_GOTOPOS = WORK_PLATFORM + 2
};

//*************************************************************************

QString ConvertGuiCharToQString(const GUI::gui_char * s)
{
#ifdef Q_OS_WIN
    return QString::fromWCharArray(s);
#else
    return QString(s);
#endif
}

QString ConvertGuiStringToQString(const GUI::gui_string & s)
{
#ifdef Q_OS_WIN
    return QString::fromWCharArray(s.c_str());
#else
    return QString(s.c_str());
#endif
}

GUI::gui_string ConvertQStringToGuiString(const QString & s)
{
#ifdef Q_OS_WIN
    return s.toStdWString();
#else
    return s.toStdString();
#endif
}

// see: https://stackoverflow.com/questions/14791360/qt5-syntax-highlighting-in-qml
template <class T> T childObject(QQmlApplicationEngine& engine,
                                 const QString& objectName,
                                 const QString& propertyName,
                                 bool bGetRoot = true)
{
    QList<QObject*> rootObjects = engine.rootObjects();
    foreach (QObject* object, rootObjects)
    {
        QObject* child = object->findChild<QObject*>(objectName);
        if (child != 0)
        {
            if( propertyName.length()==0 )
            {
                if(bGetRoot)
                {
                    return dynamic_cast<T>(object);
                }
                else
                {
                    return dynamic_cast<T>(child);
                }
            }
            else
            {
                std::string s = propertyName.toStdString();
                QObject* object = child->property(s.c_str()).value<QObject*>();
                Q_ASSERT(object != 0);
                T prop = dynamic_cast<T>(object);
                Q_ASSERT(prop != 0);
                return prop;
            }
        }
    }
    return (T) 0;
}

//*************************************************************************

SciTEQt::SciTEQt(QObject *parent, QQmlApplicationEngine * pEngine)
    : QObject(parent),
      m_pApplicationData(0),
      m_pEngine(pEngine),
      m_pCurrentScriptExecution(0),
      m_pFcnGetContentToWrite(nullptr),
      m_pFcnReceiveContentToProcess(nullptr),
      m_bWaitDoneFlag(false),
      m_iMessageDialogAccepted(MSGBOX_RESULT_EMPTY),
      m_bFileDialogWaitDoneFlag(false),
      m_iFileDialogMessageDialogAccepted(MSGBOX_RESULT_CANCEL),
      m_bFindInFilesRunning(false),
      m_bStripFindVisible(false),
      m_iLastTabIndex(-1),
      m_iCurrentTabIndex(-1),
      m_bIsInUpdateAppActive(false),
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
      m_bIsMobilePlatfrom(true),
#else
      m_bIsMobilePlatfrom(false),
#endif
      m_bShowToolBar(false),
      m_bShowStatusBar(false),
      m_bShowTabBar(true),
      m_bStatusBarTextTimerRunning(false),
      m_left(0),
      m_top(0),
      m_width(0),
      m_height(0),
      m_maximize(false),
      m_bParametersDialogOpen(false)
{
#if defined(Q_OS_WASM)
    //propsPlatform.Set("PLAT_GTK", "1");
    propsPlatform.Set("PLAT_WASM", "1");
#elif defined(Q_OS_WIN)
    propsPlatform.Set("PLAT_WIN", "1");
    propsPlatform.Set("PLAT_WINNT", "1");
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    propsPlatform.Set("PLAT_GTK", "1");
#elif defined(Q_OS_IOS)
    propsPlatform.Set("PLAT_MACOSX", "1");
    propsPlatform.Set("PLAT_IOS", "1");
#elif defined(Q_OS_MACOS)
    propsPlatform.Set("PLAT_MACOSX", "1");
#elif defined(Q_OS_ANDROID)
    //propsPlatform.Set("PLAT_GTK", "1");
    propsPlatform.Set("PLAT_ANDROID", "1");
#endif

    ReadEnvironment();

    ReadGlobalPropFile();
#ifdef Q_OS_WASM
    // workaraound: provide SciTEGlobal.properties for WASM platform --> enable toolbar and statusbar as default
    propsBase.Clear();
    FilePath emptyPath;
    ImportFilter importFilter;
    FilePathSet filePathSet;
    QString dataAsText(ApplicationData::simpleReadFileContent(":/SciTEGlobal.properties"));
    propsBase.ReadFromMemory(dataAsText.toStdString().c_str(), dataAsText.toStdString().length(), /*directoryForImports*/emptyPath, /*filter*/importFilter, /*imports*/&filePathSet, /*depth*/0);

    propsUser.Clear();
    dataAsText = ApplicationData::simpleReadFileContent(":/SciTEUser.properties");
    propsUser.ReadFromMemory(dataAsText.toStdString().c_str(), dataAsText.toStdString().length(), /*directoryForImports*/emptyPath, /*filter*/importFilter, /*imports*/&filePathSet, /*depth*/0);

    // disable save position for WASM, this fixes the resize problem
	props.Set("save.position", "0");
#endif

    SetPropertiesInitial();
    // sync property files with state of qt application
    setShowTabBar(tabVisible);
    setShowToolBar(tbVisible);
    setShowStatusBar(sbVisible);

    ReadAbbrevPropFile();

    CreateBuffers();

    // from SciTEWin.cxx Run():

    // Load the default session file
    if (props.GetInt("save.session") || props.GetInt("save.position") || props.GetInt("save.recent")) {
        LoadSessionFile(GUI_TEXT(""));
    }

    connect(&m_aFindInFiles,SIGNAL(addToOutput(QString)),this,SLOT(OnAddToOutput(QString)));
    connect(&m_aFindInFiles,SIGNAL(currentItemChanged(QString)),this,SLOT(OnCurrentFindInFilesItemChanged(QString)));
    connect(&m_aFindInFiles,SIGNAL(searchFinished()),this,SLOT(OnFileSearchFinished()));

    cmdWorker.pSciTE = this;
}

void SciTEQt::TabInsert(int index, const GUI::gui_char *title, const GUI::gui_char *fullPath)
{
    emit insertTab(index, ConvertGuiCharToQString(title), ConvertGuiCharToQString(fullPath));
}

void SciTEQt::TabSelect(int index)
{
    if(index != m_iCurrentTabIndex)
    {
        m_iLastTabIndex = m_iCurrentTabIndex;
        m_iCurrentTabIndex = index;
    }
    emit selectTab(index);
}

void SciTEQt::RemoveAllTabs()
{
    emit removeAllTabs();
}

void SciTEQt::WarnUser(int warnID)
{  
    std::string warning;

    // play a warning sound...
    switch (warnID) {
        case warnFindWrapped:
            warning = props.GetString("warning.findwrapped");
            break;
        case warnNotFound:
            warning = props.GetString("warning.notfound");
            break;
        case warnWrongFile:
            warning = props.GetString("warning.wrongfile");
            break;
        case warnExecuteOK:
            warning = props.GetString("warning.executeok");
            break;
        case warnExecuteKO:
            warning = props.GetString("warning.executeko");
            break;
        case warnNoOtherBookmark:
            warning = props.GetString("warning.nootherbookmark");
            break;
        default:
            warning = QString("unknown message for warnID=%1").arg(warnID).toStdString();
            break;
    }

    // playing sounds not implemented yet...
    //emit showInfoDialog(QString::fromStdString(warning), 0);
}

void SciTEQt::GetWindowPosition(int *left, int *top, int *width, int *height, int *maximize)
{
    emit triggerUpdateCurrentWindowPosAndSize();

    if(left!=0)
    {
        *left = m_left;
    }
    if(top!=0)
    {
        *top = m_top;
    }
    if(width!=0)
    {
        *width = m_width;
    }
    if(height!=0)
    {
        *height = m_height;
    }
    if(maximize!=0)
    {
        *maximize = (int)m_maximize;
    }
}

FilePath GetPathFromUrl(const QString & url)
{
    QUrl aUrl(url);
    QString sLocalFileName = aUrl.toLocalFile();

    GUI::gui_char buf[512];
#ifdef Q_OS_WIN
    int count = sLocalFileName.toWCharArray((wchar_t *)buf);
    buf[count] = 0;
#else
    strcpy(buf,sLocalFileName.toStdString().c_str());
#endif
    return FilePath(buf);
}

QUrl GetUrlFromPath(const QString & path)
{
    return QUrl::fromLocalFile(path);
}

bool SciTEQt::ProcessCurrentFileDialog()
{
    QObject * pFileDialog = getCurrentFileDialog();
    connect(pFileDialog,SIGNAL(accepted()),this,SLOT(OnFileDialogAcceptedClicked()));
    connect(pFileDialog,SIGNAL(rejected()),this,SLOT(OnFileDialogRejectedClicked()));

    // simulate a synchronious call: wait for signal from FileDialog and then return with result
    m_bFileDialogWaitDoneFlag = false;
    while(!m_bFileDialogWaitDoneFlag)
    {
        // for Webassembly see:
        // https://bugreports.qt.io/browse/QTBUG-64020
        // http://vps2.etotheipiplusone.com:30176/redmine/projects/emscripten-qt/wiki/
        QCoreApplication::processEvents();
#ifdef Q_OS_WASM
        emscripten_sleep(10);   // TODO: this is a problem for WASM
#endif
        QThread::msleep(10);
    }

    disconnect(pFileDialog,SIGNAL(accepted()),this,SLOT(OnFileDialogAcceptedClicked()));
    disconnect(pFileDialog,SIGNAL(rejected()),this,SLOT(OnFileDialogRejectedClicked()));

    return m_iFileDialogMessageDialogAccepted == MSGBOX_RESULT_OK;
}

void SciTEQt::CheckAndDeleteGetContentToWriteFunctionPointer()
{
    if (m_pFcnGetContentToWrite!=nullptr)
    {
        delete m_pFcnGetContentToWrite;
        m_pFcnGetContentToWrite = nullptr;
    }
}

void SciTEQt::CheckAndDeleteReceiveContentToProcessFunctionPointer()
{
    if (m_pFcnReceiveContentToProcess!=nullptr)
    {
        delete m_pFcnReceiveContentToProcess;
        m_pFcnReceiveContentToProcess = nullptr;
    }
}

QString TriggerActionAndReadResultFile(std::function<void(QString)> action)
{
    if(QFile::exists(FULL_TEMP_FILENAME))
    {
        QFile::remove(FULL_TEMP_FILENAME);
    }

    action(QString(FULL_TEMP_FILENAME));

    QString content = ApplicationData::simpleReadFileContent(FULL_TEMP_FILENAME);

    if(QFile::exists(FULL_TEMP_FILENAME))
    {
        QFile::remove(FULL_TEMP_FILENAME);
    }

    return content;
}

bool SciTEQt::OpenDialog(const FilePath &directory, const GUI::gui_char *filesFilter)
{
    // close explicit the current menu, because we make a event loop which disables automatic closing of current menu in mobile ui modus
    //emit dismissMenu();
    // --> dismiss not needed, because now Qt.callLater() is used to trigger menu operation, this works good also for android

    QString s = ConvertGuiCharToQString(filesFilter);
    if (s.startsWith("All Source|"))
    {
        s = s.remove(0, 11);
    }
    s = "*";

    emit startFileDialog(directory.AbsolutePath().AsUTF8().c_str(), s/*ConvertGuiStringToQString(openFilter)*/, "Open File", true);

#ifndef Q_OS_WASM
    // in WASM one can easily make this call asynchroniously !
    if(ProcessCurrentFileDialog())
    {
        return Open(GetPathFromUrl(m_sCurrentFileUrl));
    }
#endif
    // the return value is not needed in whole SciTE
    return false;
}

bool SciTEQt::SaveAsDialog()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "", tr("Save File"), false, false, false, QString::fromStdString(filePath.Name().AsUTF8()));

//#ifndef Q_OS_WASM
// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
        return SaveIfNotOpen(GetPathFromUrl(m_sCurrentFileUrl), false);
    }
//#endif
    return false;    
}

void SciTEQt::LoadSessionDialog()
{
    CheckAndDeleteReceiveContentToProcessFunctionPointer();
    m_pFcnReceiveContentToProcess = new std::function<void(QString)>( [this](QString content) -> void {
        //LoadSessionFile(GetPathFromUrl(m_sCurrentFileUrl).AsInternal());
        propsSession.Clear();
        propsSession.ReadFromMemory(content.toStdString().data(), content.toStdString().length(), FilePath(FILES_DIR), ImportFilter(), nullptr, 0);
        RestoreSession();
    } );

    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.session", tr("Load Session File"), true);

// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
        LoadSessionFile(GetPathFromUrl(m_sCurrentFileUrl).AsInternal());
        RestoreSession();
    }

    CheckAndDeleteReceiveContentToProcessFunctionPointer();
}

void SciTEQt::SaveSessionDialog()
{
    CheckAndDeleteGetContentToWriteFunctionPointer();
    m_pFcnGetContentToWrite = new std::function<QString()>( [this]() -> QString {
        return TriggerActionAndReadResultFile([this](QString name) -> void { SaveSessionFile(name.toStdString().c_str()); } );
    } );

    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.session", tr("Save Session File"), false, true, false, "current.session");

// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
        SaveSessionFile(GetPathFromUrl(m_sCurrentFileUrl).AsInternal());
    }

    CheckAndDeleteGetContentToWriteFunctionPointer();
}

void SciTEQt::SaveACopy()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "", "Save a Copy", false, true, false, QString::fromStdString(filePath.Name().AsUTF8()));

// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
        FilePath aFileName(GetPathFromUrl(m_sCurrentFileUrl));

//        if(aFileName.IsNotLocal())
//        {
//            // this should never happen !!! handled via OnAddFileContent() call from Android Storage Framework
//            //Q_ASSERT(false);
//            QString text = QString::fromStdString(wEditor.GetText(wEditor.TextLength()+1));
//            m_pApplicationData->writeFileContent(ConvertGuiCharToQString(aFileName.AsInternal()), text);
//        }
//        else
        {
            SaveBuffer(aFileName, sfNone);
        }
    }
}

void SciTEQt::SaveAsRTF()
{
    CheckAndDeleteGetContentToWriteFunctionPointer();
    m_pFcnGetContentToWrite = new std::function<QString()>( [this]() -> QString {
        std::ostringstream oss;
        SaveToStreamRTF(oss);
        const std::string rtf = oss.str();
        return QString(rtf.c_str());
    } );

    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.rtf", tr("Export File As RTF"), false, true, false, QString::fromStdString(filePath.Name().AsUTF8())+".rtf");

// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
        SaveToRTF(GetPathFromUrl(m_sCurrentFileUrl));
    }

    CheckAndDeleteGetContentToWriteFunctionPointer();
}

void SciTEQt::SaveAsPDF()
{
    CheckAndDeleteGetContentToWriteFunctionPointer();
    m_pFcnGetContentToWrite = new std::function<QString()>( [this]() -> QString {
        return TriggerActionAndReadResultFile([this](QString name) -> void { SaveToPDF(FilePath(name.toStdString().c_str())); } );
    } );

    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.pdf", tr("Export File As PDF"), false, true, false, QString::fromStdString(filePath.Name().AsUTF8())+".pdf");

// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
        SaveToPDF(GetPathFromUrl(m_sCurrentFileUrl));
    }

    CheckAndDeleteGetContentToWriteFunctionPointer();
}

void SciTEQt::SaveAsTEX()
{
    CheckAndDeleteGetContentToWriteFunctionPointer();
    m_pFcnGetContentToWrite = new std::function<QString()>( [this]() -> QString {
        return TriggerActionAndReadResultFile([this](QString name) -> void { SaveToTEX(FilePath(name.toStdString().c_str())); } );
    } );

    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.tex", tr("Export File As LaTeX"), false, true, false, QString::fromStdString(filePath.Name().AsUTF8())+".tex");

// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
// TODO Behandeln von Storage Access Framework !
        SaveToTEX(GetPathFromUrl(m_sCurrentFileUrl));
    }

    CheckAndDeleteGetContentToWriteFunctionPointer();
}

void SciTEQt::SaveAsXML()
{
    CheckAndDeleteGetContentToWriteFunctionPointer();
    m_pFcnGetContentToWrite = new std::function<QString()>( [this]() -> QString {
        return TriggerActionAndReadResultFile([this](QString name) -> void { SaveToXML(FilePath(name.toStdString().c_str())); } );
    } );

    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.xml", tr("Export File As XML"), false, true, false, QString::fromStdString(filePath.Name().AsUTF8())+".xml");

// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
        SaveToXML(GetPathFromUrl(m_sCurrentFileUrl));
    }

    CheckAndDeleteGetContentToWriteFunctionPointer();
}

void SciTEQt::SaveAsHTML()
{
    CheckAndDeleteGetContentToWriteFunctionPointer();
    m_pFcnGetContentToWrite = new std::function<QString()>( [this]() -> QString {
        return TriggerActionAndReadResultFile([this](QString name) -> void { SaveToHTML(FilePath(name.toStdString().c_str())); } );
    } );

    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.html", tr("Export File As HTML"), false, true, false, QString::fromStdString(filePath.Name().AsUTF8())+".html");

// TODO for WASM
    if(ProcessCurrentFileDialog())
    {
        SaveToHTML(GetPathFromUrl(m_sCurrentFileUrl));
    }

    CheckAndDeleteGetContentToWriteFunctionPointer();
}

FilePath GetSciTEPath(const QByteArray & home)
{
    GUI::gui_char buf[512];
    if(!home.isEmpty())
    {
#ifdef Q_OS_WIN
        int count = QString::fromLocal8Bit(home).toWCharArray((wchar_t *)buf);
        buf[count] = 0;
#else
        strcpy(buf,QString::fromLocal8Bit(home).toStdString().c_str());
#endif
    }
    else
    {
#ifdef Q_OS_WIN
        // return the directory of the executable
        int count = QDir::toNativeSeparators(QCoreApplication::applicationDirPath()).toWCharArray((wchar_t *)buf);
        buf[count] = 0;
#elif defined(Q_OS_ANDROID)
        strcpy(buf,FILES_DIR);
#else
        strcpy(buf,QDir::toNativeSeparators(QCoreApplication::applicationDirPath()).toStdString().c_str());
#endif
    }

    return FilePath(buf);
}

FilePath SciTEQt::GetDefaultDirectory()
{
    QByteArray home = qgetenv(SCITE_HOME);
    return GetSciTEPath(home);
}

FilePath SciTEQt::GetSciteDefaultHome()
{
    QByteArray home = qgetenv(SCITE_HOME);
    return GetSciTEPath(home);
}

FilePath SciTEQt::GetSciteUserHome()
{
    // First looking for environment variable $SciTE_USERHOME
    // to set SciteUserHome. If not present we look for $SciTE_HOME
    // then defaulting to $USERPROFILE
    QByteArray home = qgetenv("SciTE_USERHOME");
    if (home.isEmpty()) {
        home = qgetenv(SCITE_HOME);
        if (home.isEmpty()) {
            home = qgetenv("USERPROFILE");
            if(home.isEmpty()) {
                //QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
                home = QDir::homePath().toLocal8Bit();
            }
        }
    }

    return GetSciTEPath(home);
}

void SciTEQt::Print(bool)
{
    // TODO implement !

    emit showInfoDialog("Sorry: Print() is not implemented yet!", 0);
/*
    long lengthDoc = static_cast<long>(wEditor.Length());
    std::string txt = wEditor.GetText(lengthDoc);

    m_aPrinter.setFullPage( true );
    QPrintDialog aDlg(&m_aPrinter, 0);
    if( aDlg.exec() )
    {
        QPainter aPainter;
        aPainter.begin(&m_aPrinter);
// TODO: better printer implementation...
        aPainter.drawText(10, 10, QString::fromStdString(txt));
        aPainter.end();
    }
*/
}

void SciTEQt::PrintSetup()
{
    QPageSetupDialog aPageSetupDialog(&m_aPrinter);
    aPageSetupDialog.exec();
}

void SciTEQt::Find()
{
    // see: SciTEWin::Find()

    SelectionIntoFind();    // findWhat

    Searcher * pSearcher = this;

    QStringList findHistory;
    for (int i = 0; i < pSearcher->memFinds.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(pSearcher->memFinds.At(i));
        findHistory.append(ConvertGuiStringToQString(gs));
    }

    QStringList replaceHistory;
    for (int i = 0; i < pSearcher->memReplaces.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(pSearcher->memReplaces.At(i));
        replaceHistory.append(ConvertGuiStringToQString(gs));
    }

    if (props.GetInt("find.use.strip")) {
//        replaceStrip.Close();
//        findStrip.SetIncrementalBehaviour(props.GetInt("find.strip.incremental"));
//        findStrip.Show(props.GetInt("strip.button.height", -1));
        emit showFindStrip(findHistory, replaceHistory, QString::fromStdString(findWhat), false, false, !(pSearcher->closeFind == CloseFind::closePrevent));
    } else {
//        if (findStrip.visible || replaceStrip.visible)
//            return;
//        FindReplace(false);
        if( m_bStripFindVisible )
            return;

        replacing = false;

        emit showFind(findHistory, QString::fromStdString(findWhat), pSearcher->wholeWord, pSearcher->matchCase, pSearcher->regExp, pSearcher->wrapFind, pSearcher->unSlash, !pSearcher->reverseFind);
    }
}

SciTEQt::MessageBoxChoice SciTEQt::ProcessModalWindowSynchronious(const QString & objectName)
{
    m_iMessageDialogAccepted = MSGBOX_RESULT_EMPTY;
    QObject * pMessageBox = getDialog(objectName);
    connect(pMessageBox,SIGNAL(accepted()),this,SLOT(OnAcceptedClicked()));
    connect(pMessageBox,SIGNAL(rejected()),this,SLOT(OnRejectedClicked()));
    connect(pMessageBox,SIGNAL(canceled()),this,SLOT(OnRejectedClicked()));
    //connect(pMessageBox,SIGNAL(okClicked()), this, SLOT(OnOkClicked()));
    //connect(pMessageBox,SIGNAL(cancelClicked()),this,SLOT(OnCancelClicked()));
    connect(pMessageBox,SIGNAL(yes()),this,SLOT(OnYesClicked()));
    connect(pMessageBox,SIGNAL(no()),this,SLOT(OnNoClicked()));

    // simulate a synchronious call: wait for signal from MessageBox and then return with result
    m_bWaitDoneFlag = false;
    while(!m_bWaitDoneFlag)
    {
        QCoreApplication::processEvents();
#ifdef Q_OS_WASM
        emscripten_sleep(10);       // TODO: this is a problem for WASM
#endif
        QThread::msleep(10);
    }

    disconnect(pMessageBox,SIGNAL(accepted()),this,SLOT(OnAcceptedClicked()));
    disconnect(pMessageBox,SIGNAL(rejected()),this,SLOT(OnRejectedClicked()));
    disconnect(pMessageBox,SIGNAL(canceled()),this,SLOT(OnRejectedClicked()));
    //disconnect(pMessageBox,SIGNAL(ok()), this, SLOT(OnOkClicked()));
    //disconnect(pMessageBox,SIGNAL(cancel()),this,SLOT(OnCancelClicked()));
    disconnect(pMessageBox,SIGNAL(yes()),this,SLOT(OnYesClicked()));
    disconnect(pMessageBox,SIGNAL(no()),this,SLOT(OnNoClicked()));

    if ((m_iMessageDialogAccepted & MSGBOX_RESULT_NO) == MSGBOX_RESULT_NO)
        return SciTEQt::MessageBoxChoice::mbNo;
    if ((m_iMessageDialogAccepted & MSGBOX_RESULT_YES) == MSGBOX_RESULT_YES)
        return SciTEQt::MessageBoxChoice::mbYes;
    if ((m_iMessageDialogAccepted & MSGBOX_RESULT_OK) == MSGBOX_RESULT_OK)
        return SciTEQt::MessageBoxChoice::mbOK;
    if ((m_iMessageDialogAccepted & MSGBOX_RESULT_CANCEL) == MSGBOX_RESULT_CANCEL)
        return SciTEQt::MessageBoxChoice::mbCancel;

    return SciTEQt::MessageBoxChoice::mbCancel;
}

SciTEQt::MessageBoxChoice SciTEQt::ShowWindowMessageBox(const QString & msg, MessageBoxStyle style)
{
    emit showInfoDialog(msg, style);

    QString name = isWebassemblyPlatform() ? "infoDialogPage" : "infoDialog";

#ifdef Q_OS_WASM
    // can we handle as an async call for WASM? assync possible if default value for style is given !
    if(style == mbsIconWarning)
    {
        return MessageBoxChoice::mbCancel;
    }
#endif
    return ProcessModalWindowSynchronious(name);
}

SciTEQt::MessageBoxChoice SciTEQt::WindowMessageBox(GUI::Window &w, const GUI::gui_string &msg, MessageBoxStyle style)
{
    Q_UNUSED(w);
    return ShowWindowMessageBox(ConvertGuiStringToQString(msg), style);
}

void SciTEQt::FindMessageBox(const std::string &msg, const std::string *findItem)
{
    GUI::gui_string msgBuf;

    if (!findItem) {
        msgBuf = LocaliseMessage(msg.c_str());
    } else {
        GUI::gui_string findThing = GUI::StringFromUTF8(*findItem);
        msgBuf = LocaliseMessage(msg.c_str(), findThing.c_str());
    }

    ShowWindowMessageBox(ConvertGuiStringToQString(msgBuf), 0);
//    emit showInfoDialog(ConvertGuiStringToQString(msgBuf), 0);

    // make a synchronious call...
//    QString name = isWebassemblyPlatform() ? "infoDialogPage" : "infoDialog";
//    ProcessModalWindowSynchronious(name);
}

void SciTEQt::FindIncrement()
{
    Searcher * pSearcher = this;

    QStringList findHistory;
    for (int i = 0; i < pSearcher->memFinds.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(pSearcher->memFinds.At(i));
        findHistory.append(ConvertGuiStringToQString(gs));
    }

    QStringList replaceHistory;
    for (int i = 0; i < pSearcher->memReplaces.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(pSearcher->memReplaces.At(i));
        replaceHistory.append(ConvertGuiStringToQString(gs));
    }

    emit showFindStrip(findHistory, replaceHistory, "", true, false, !(pSearcher->closeFind == CloseFind::closePrevent));
}

void SciTEQt::FindInFiles()
{
    // see: DialogFindReplace::FillFields() and SciTEWin::PerformGrep() and InternalGrep()
    SelectionIntoFind();    // findWhat

    Searcher * pSearcher = this;

    // see: SciTEWin::FillCombos() and SciTEWin::FillCombosForGrep()
    QStringList findHistory;
    for (int i = 0; i < pSearcher->memFinds.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(pSearcher->memFinds.At(i));
        findHistory.append(ConvertGuiStringToQString(gs));
    }

    QStringList filePatternHistory;
    for (int i = 0; i < memFiles.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(memFiles.At(i));
        filePatternHistory.append(ConvertGuiStringToQString(gs));
    }

    QStringList directoryHistory;
    for (int i = 0; i < memDirectory.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(memDirectory.At(i));
        directoryHistory.append(ConvertGuiStringToQString(gs));
    }
    if (directoryHistory.length()==0)
    {
        directoryHistory.append(QDir::toNativeSeparators(QDir::current().absolutePath()));
    }

    bool wholeWord = pSearcher->wholeWord;
    bool caseSensitive = pSearcher->matchCase;
    bool regularExpression = pSearcher->regExp;

    emit showFindInFilesDialog(QString::fromStdString(findWhat), findHistory, filePatternHistory, directoryHistory, wholeWord, caseSensitive, regularExpression);
}

void SciTEQt::Replace()
{
    // see: SciTEWin::Replace()

    SelectionIntoFind(false);    // findWhat

    Searcher * pSearcher = this;

    QStringList findHistory;
    for (int i = 0; i < pSearcher->memFinds.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(pSearcher->memFinds.At(i));
        findHistory.append(ConvertGuiStringToQString(gs));
    }

    QStringList replaceHistory;
    for (int i = 0; i < pSearcher->memReplaces.Length(); i++) {
        GUI::gui_string gs = GUI::StringFromUTF8(pSearcher->memReplaces.At(i));
        replaceHistory.append(ConvertGuiStringToQString(gs));
    }

    if (props.GetInt("replace.use.strip")) {
        //if (searchStrip.visible)
        //	searchStrip.Close();
        //if (findStrip.visible)
        //	findStrip.Close();
        //replaceStrip.visible = true;
        //SizeSubWindows();
        //replaceStrip.SetIncrementalBehaviour(props.GetInt("replace.strip.incremental"));
        //replaceStrip.ShowStrip();
        emit showFindStrip(findHistory, replaceHistory, QString::fromStdString(findWhat), false, true, !(pSearcher->closeFind == CloseFind::closePrevent));
        havefound = false;
    } else {
        //if (searchStrip.visible || findStrip.visible)
        //	return;
        if( m_bStripFindVisible )
            return;

        replacing = true;
        havefound = false;

        emit showReplace(findHistory, replaceHistory, QString::fromStdString(findWhat), QString::fromStdString(pSearcher->replaceWhat), pSearcher->wholeWord, pSearcher->matchCase, pSearcher->regExp, pSearcher->wrapFind, pSearcher->unSlash, !pSearcher->reverseFind);

        //const int dialogID = (!props.GetInt("find.replace.advanced") ? IDD_REPLACE : IDD_REPLACE_ADV);
        //wFindReplace = CreateParameterisedDialog(MAKEINTRESOURCE(dialogID), ReplaceDlg);
        //wFindReplace.Show();
    }

}

void SciTEQt::DestroyFindReplace()
{
    emit closeFindReplaceDialog();
}

void SciTEQt::GoLineDialog()
{
    // see: BOOL SciTEWin::GoLineMessage(HWND hDlg, UINT message, WPARAM wParam)
    SA::Position position = wEditor.CurrentPos();
    const SA::Line lineNumber = wEditor.LineFromPosition(position) + 1;
    const SA::Position lineStart = wEditor.LineStart(lineNumber - 1);
    int characterOnLine = 1;
    while (position > lineStart) {
        position = wEditor.PositionBefore(position);
        characterOnLine++;
    }

    emit showGoToDialog(lineNumber, characterOnLine, wEditor.LineCount());
}

bool SciTEQt::AbbrevDialog()
{
    // create a list of abbrevs...
    QStringList items;

    // addapted from: FillComboFromProps()
    const char *key = nullptr;
    const char *val = nullptr;
    if (propsAbbrev.GetFirst(key, val)) {
        GUI::gui_string wkey = GUI::StringFromUTF8(key);
        items.append(ConvertGuiStringToQString(wkey));
        while (propsAbbrev.GetNext(key, val)) {
            wkey = GUI::StringFromUTF8(key);
            items.append(ConvertGuiStringToQString(wkey));
        }
    }

    emit showAbbreviationDialog(items);

    QString name = isUseMobileDialogHandling() ? "abbreviationDialog" : "abbreviationDialogWin";
    MessageBoxChoice result = ProcessModalWindowSynchronious(name);
    return result == SciTEQt::MessageBoxChoice::mbOK;
}

void SciTEQt::TabSizeDialog()
{
    emit showTabSizeDialog(wEditor.TabWidth(), wEditor.Indent(), wEditor.UseTabs());
}

bool SciTEQt::ParametersOpen()
{
    // is non modal parameters dialog open ?
    return m_bParametersDialogOpen;
}

void SciTEQt::ParamGrab()
{
    // get (last?) parameters from (closed) parameters dialog ?

    // read parameters from open parameters dialog, see code Win and Gtk:

    std::string paramText1 = StdStringFromInteger(1);
    props.Set(paramText1.c_str(), m_parameter1.toStdString().c_str());
    std::string paramText2 = StdStringFromInteger(2);
    props.Set(paramText2.c_str(), m_parameter2.toStdString().c_str());
    std::string paramText3 = StdStringFromInteger(3);
    props.Set(paramText3.c_str(), m_parameter3.toStdString().c_str());
    std::string paramText4 = StdStringFromInteger(4);
    props.Set(paramText4.c_str(), m_parameter4.toStdString().c_str());

// TODO --> handling of parametrisedCommand ?

    UpdateStatusBar(true);
}

bool SciTEQt::ParametersDialog(bool modal)
{
    QStringList parameters;

    // from SciTEWin::ParametersMessage()
    if (modal/*Parameters*/) {
        GUI::gui_string sCommand = GUI::StringFromUTF8(parameterisedCommand);
    }
    for (int param = 0; param < maxParam; param++) {
        std::string paramText = StdStringFromInteger(param + 1);
        std::string paramTextVal = props.GetString(paramText.c_str());
        GUI::gui_string sVal = GUI::StringFromUTF8(paramTextVal);
        parameters.append(ConvertGuiStringToQString(sVal));
    }

    emit showParametersDialog(modal, parameters);
    m_bParametersDialogOpen = true;

    if(modal)
    {
        QString name = isUseMobileDialogHandling() ? "parametersDialog" : "parametersDialogWin";
        MessageBoxChoice result = ProcessModalWindowSynchronious(name);
        m_bParametersDialogOpen = false;
        return result == SciTEQt::MessageBoxChoice::mbOK;
    }

    return false;
}

void SciTEQt::FindReplace(bool replace)
{
    replacing = replace;
}

void SciTEQt::StopExecute()
{
    if(m_pCurrentScriptExecution!=0)
    {
        m_pCurrentScriptExecution->KillExecution();
    }
}

void SciTEQt::SetFileProperties(PropSetFile &ps)
{
    QString fileName = QString::fromStdString(filePath.AsUTF8());

    QFileInfo aFileInfo(fileName);

    QString temp("?");
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    temp = aFileInfo.fileTime(QFileDevice::FileModificationTime).toString("hh:mm:ss");
#else
    temp = aFileInfo.lastModified().toString("hh:mm:ss");
#endif
    ps.Set("FileTime", temp.toStdString());
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    temp = aFileInfo.fileTime(QFileDevice::FileModificationTime).toString("dd.MM.yyyy");
#else
    temp = aFileInfo.lastModified().toString("dd.MM.yyyy");
#endif
    ps.Set("FileDate", temp.toStdString());
    temp = QString("%1%2%3").arg(aFileInfo.isWritable() ? " " : "R").arg(aFileInfo.isHidden() ? "H" : " ").arg(" ");
    ps.Set("FileAttr", temp.toStdString());

    QDateTime now = QDateTime::currentDateTime();
    ps.Set("CurrentDate", now.toString("dd.MM.yyyy").toStdString());
    ps.Set("CurrentTime", now.toString("hh:mm:ss").toStdString());
}

void SciTEQt::AboutDialog()
{
    // see: SciTEBase::SetAboutMessage

    SetAboutMessage(wAboutScite, "About SciTE");
    emit showAboutSciteDialog();
}

void SciTEQt::QuitProgram()
{
    quitting = false;
    if (SaveIfUnsureAll() != saveCancelled) {
        //if (fullScreen)	// Ensure tray visible on exit
        //	FullScreenToggle();
        quitting = true;
        // If ongoing saves, wait for them to complete.
        if (!buffers.SavingInBackground()) {
            QGuiApplication::quit();
            //::PostQuitMessage(0);
            wSciTE.Destroy();
        }
    }
}

void SciTEQt::CopyPath()
{
    const GUI::gui_string clipText(filePath.AsInternal());

    QClipboard * pClipboard = QGuiApplication::clipboard();
    pClipboard->setText(ConvertGuiCharToQString(clipText.c_str()));
}

void SciTEQt::CopyAsRTF()
{
    // TODO implement !
    emit showInfoDialog("Sorry: CopyAsRTF() is not implemented yet!", 0);
}

void SciTEQt::SetStatusBarText(const char *s)
{
    m_sSciteStatusBarText = s;
    setStatusBarText(s);
}

void SciTEQt::ShowToolBar()
{
    setShowToolBar(tbVisible);
}

void SciTEQt::ShowTabBar()
{
    setShowTabBar(tabVisible);
}

void SciTEQt::ShowStatusBar()
{
    setShowStatusBar(sbVisible);
}

void SciTEQt::ActivateWindow(const char *timestamp)
{
    // TODO implement !
    Q_UNUSED(timestamp);
    emit showInfoDialog("Sorry: ActivateWindow() is not implemented yet!", 0);
}

void SciTEQt::SizeContentWindows()
{
    emit setVerticalSplit(splitVertical);
    emit setOutputHeight(heightOutput);
}

void SciTEQt::SizeSubWindows()
{
    SizeContentWindows();
}

void SciTEQt::SetMenuItem(int menuNumber, int position, int itemID,
             const GUI::gui_char *text, const GUI::gui_char *mnemonic)
{
    Q_UNUSED(position);
    if(menuNumber == 7)
    {
        if( itemID >= IDM_BUFFER && itemID < IDM_IMPORT)
        {
            int posForThisItem = itemID - IDM_BUFFER;

            emit setInBuffersModel(posForThisItem, ConvertGuiCharToQString(text), false, ConvertGuiCharToQString(mnemonic));
        }
    }
    else if(menuNumber == 6)
    {
        if( itemID >= IDM_LANGUAGE && itemID < IDM_LANGUAGE+100 )
        {
            int posForThisItem = itemID - IDM_LANGUAGE;

            emit setInLanguagesModel(posForThisItem, ConvertGuiCharToQString(text), false, ConvertGuiCharToQString(mnemonic));
        }
    }
    else if (menuNumber == 5)
    {
        int posForThisItem = itemID - IDM_IMPORT;

        emit setInImportModel(posForThisItem, ConvertGuiCharToQString(text), false, ConvertGuiCharToQString(mnemonic));
    }
    else if (menuNumber == 4)
	{
        int posForThisItem = itemID - IDM_TOOLS;
		
        // TODO: analyze, msc 2017 debug has problems with this ?
#if defined(Q_CC_MSVC)
        emit setInToolsModel(posForThisItem, ConvertGuiCharToQString(text), false, "");
#else
        emit setInToolsModel(posForThisItem, ConvertGuiCharToQString(text), false, /*g_aShortCuts[posForThisItem]*//*g_aShortCuts.at(posForThisItem)*/ConvertGuiCharToQString(mnemonic)/*sMn*//*"Ctrl+0"*/);
#endif
    }
    else if (menuNumber == 0)
    {
        int posForThisItem = itemID - IDM_MRUFILE;

        if(posForThisItem>=0)   // ignore MenuSeperator Id
        {
            emit setInLastOpenedFilesModel(posForThisItem, ConvertGuiCharToQString(text), false, ConvertGuiCharToQString(mnemonic));
        }
    }
    else
    {
        //qDebug() << "==> UN_HANDLED: Set Menu Item " << menuNumber << " pos=" << position << " " << itemID << " " << (text != 0 ? ConvertGuiStringToQString (text) : "") << endl; //QString::fromWCharArray(text) << " " << QString::fromWCharArray(mnemonic) << endl;
    }
}

void SciTEQt::DestroyMenuItem(int menuNumber, int itemID)
{
    if(menuNumber == 7)
    {
        int posForThisItem = itemID - IDM_BUFFER;
        emit removeInBuffersModel(posForThisItem);
    }
    else if(menuNumber == 6)
    {
        int posForThisItem = itemID - IDM_LANGUAGE;
        emit removeInLanguagesModel(posForThisItem);
    }
    else if(menuNumber == 5)
    {
        int posForThisItem = itemID - IDM_IMPORT;
        if( posForThisItem>=0 )
        {
            emit removeInImportModel(posForThisItem);
        }
    }
    else if(menuNumber == 4)
    {
        int posForThisItem = itemID - IDM_TOOLS;
        // 1100... and 310...314
        if( posForThisItem>=0 )
        {
            emit removeInToolsModel(posForThisItem);
        }
    }
    else if(menuNumber == 0)
    {
        int posForThisItem = itemID - IDM_MRUFILE;
        if( posForThisItem>=0 )
        {
            emit removeInLastOpenedFilesModel(posForThisItem);
        }
    }
    else
    {
        //qDebug() << "NOT HANDLED Destroy Menu Item " << menuNumber << " item=" << itemID << endl;
    }
}

void SciTEQt::CheckAMenuItem(int wIDCheckItem, bool val)
{
    if( wIDCheckItem >= IDM_BUFFER && wIDCheckItem < IDM_IMPORT )
    {
        emit checkStateInBuffersModel(wIDCheckItem-IDM_BUFFER, val);
    }
    else if( wIDCheckItem >= IDM_LANGUAGE && wIDCheckItem < IDM_LANGUAGE+100 )
    {
        emit checkStateInLanguagesModel(wIDCheckItem-IDM_LANGUAGE, val);
    }
    else if( wIDCheckItem >= IDM_TOOLS && wIDCheckItem < IDM_TOOLS+toolMax )
    {
        emit checkStateInToolsModel(wIDCheckItem-IDM_TOOLS, val);
    }
    else
    {
        emit setMenuChecked(wIDCheckItem, val);
    }
}

void SciTEQt::EnableAMenuItem(int wIDCheckItem, bool val)
{
    emit setMenuEnable(wIDCheckItem, val);
}

void SciTEQt::AddToPopUp(const char *label, int cmd, bool enabled)
{
    // see: SciTEBase::ContextMenu()

    // TODO implement ! --> not needed, because implemented in qml / quick
    Q_UNUSED(label);
    Q_UNUSED(cmd);
    Q_UNUSED(enabled);
}

void SciTEQt::PostOnMainThread(int cmd, Worker *pWorker)
{
    QSciTEQtEvent * pEvent = new QSciTEQtEvent(cmd, pWorker);
    QCoreApplication::postEvent(this, pEvent);
}

bool SciTEQt::isShowStatusBar() const
{
    return m_bShowStatusBar;
}

void SciTEQt::setShowStatusBar(bool val)
{
    if(val != m_bShowStatusBar)
    {
        m_bShowStatusBar = val;
        emit showStatusBarChanged();
    }
}

bool SciTEQt::isShowToolBar() const
{
    return m_bShowToolBar;
}

void SciTEQt::setShowToolBar(bool val)
{
    if(val != m_bShowToolBar)
    {
        m_bShowToolBar = val;
        emit showToolBarChanged();
    }
}

bool SciTEQt::isShowTabBar() const
{
    return m_bShowTabBar;
}

void SciTEQt::setShowTabBar(bool val)
{
    if(val != m_bShowTabBar)
    {
        m_bShowTabBar = val;
        emit showTabBarChanged();
    }
}

bool SciTEQt::isWholeWord() const
{
    return wholeWord;
}

void SciTEQt::setWholeWord(bool val)
{
    if(val != wholeWord)
    {
        wholeWord = val;
        emit wholeWordChanged();
    }
}

bool SciTEQt::isCaseSensitive() const
{
    return matchCase;
}

void SciTEQt::setCaseSensitive(bool val)
{
    if(val != matchCase)
    {
        matchCase = val;
        emit caseSensitiveChanged();
    }
}

bool SciTEQt::isRegularExpression() const
{
    return regExp;
}

void SciTEQt::setRegularExpression(bool val)
{
    if(val != regExp)
    {
        regExp = val;
        emit regularExpressionChanged();
    }
}

bool SciTEQt::isTransformBackslash() const
{
    return unSlash;
}

void SciTEQt::setTransformBackslash(bool val)
{
    if(val != unSlash)
    {
        unSlash = val;
        emit transformBackslashChanged();
    }
}

bool SciTEQt::isWrapAround() const
{
    return wrapFind;
}

void SciTEQt::setWrapAround(bool val)
{
    if(val != wrapFind)
    {
        wrapFind = val;
        emit wrapAroundChanged();
    }
}

bool SciTEQt::isSearchUp() const
{
    return reverseFind;
}

void SciTEQt::setSearchUp(bool val)
{
    if(val != reverseFind)
    {
        reverseFind = val;
        emit searchUpChanged();
    }
}

bool SciTEQt::isFindInFilesRunning() const
{
    return m_bFindInFilesRunning;
}

void SciTEQt::setFindInFilesRunning(bool val)
{
    if(m_bFindInFilesRunning != val)
    {
        m_bFindInFilesRunning = val;
        emit findInFilesRunningChanged();
    }
}

QString SciTEQt::getStatusBarText() const
{
    return m_sStatusBarText;
}

void SciTEQt::setStatusBarText(const QString & txt)
{
    if(m_sStatusBarText != txt)
    {
        m_sStatusBarText = txt;
        emit statusBarTextChanged();
    }
}

bool SciTEQt::doOpen(const QString & sFileName)
{
    QString s = sFileName;
    QUrl aUrl(sFileName);
    if(aUrl.isLocalFile())
    {
        s = QDir::toNativeSeparators(aUrl.toLocalFile());
    }
    GUI::gui_char buf[512];
#ifdef Q_OS_WIN
    int count = s.toWCharArray((wchar_t *)buf);
    buf[count] = 0;
#else
    strcpy(buf,s.toStdString().c_str());
#endif
    FilePath path(buf); // StdString().c_str()
#ifdef Q_OS_ANDROID
    return Open(path);
#else
    OpenFlags openFlags = ofNone;
#ifdef Q_OS_WASM
    openFlags = ofSynchronous;
#endif
    return Open(path, openFlags);
#endif
}

void SciTEQt::setScintilla(QObject * obj)
{
    ScintillaEditBase * base = reinterpret_cast<ScintillaEditBase *>(obj);

    SciFnDirect fn_ = reinterpret_cast<SciFnDirect>(base->send(SCI_GETDIRECTFUNCTION, 0, 0));
    const sptr_t ptr_ = base->send(SCI_GETDIRECTPOINTER, 0, 0);
    wEditor.SetFnPtr(fn_, ptr_);
    wEditor.SetID(base->sqt);
    base->sqt->UpdateInfos(IDM_SRCWIN);

    connect(base->sqt,SIGNAL(notifyParent(SCNotification)),this,SLOT(OnNotifiedFromScintilla(SCNotification)));

    connect(base,SIGNAL(uriDropped(QString)),this,SLOT(OnUriDroppedFromScintilla(QString)));
}

void SciTEQt::setOutput(QObject * obj)
{
    ScintillaEditBase * base = reinterpret_cast<ScintillaEditBase *>(obj);

    SciFnDirect fn_ = reinterpret_cast<SciFnDirect>(base->send(SCI_GETDIRECTFUNCTION, 0, 0));
    const sptr_t ptr_ = base->send(SCI_GETDIRECTPOINTER, 0, 0);
    wOutput.SetFnPtr(fn_, ptr_);
    wOutput.SetID(base->sqt);
	base->sqt->UpdateInfos(IDM_RUNWIN);

	connect(base->sqt, SIGNAL(notifyParent(SCNotification)), this, SLOT(OnNotifiedFromOutput(SCNotification)));
}

void SciTEQt::setAboutScite(QObject * obj)
{
    ScintillaEditBase * base = reinterpret_cast<ScintillaEditBase *>(obj);

    SciFnDirect fn_ = reinterpret_cast<SciFnDirect>(base->send(SCI_GETDIRECTFUNCTION, 0, 0));
    const sptr_t ptr_ = base->send(SCI_GETDIRECTPOINTER, 0, 0);
    wAboutScite.SetFnPtr(fn_, ptr_);
    wAboutScite.SetID(base->sqt);
}

// TODO: use for debugging
//extern QString g_sDebugMsg;

void SciTEQt::setMainWindow(QObject * obj)
{
    wSciTE.SetID(obj);

    //OnAddToOutput(g_sDebugMsg);

    connect(obj,SIGNAL(stripFindVisible(bool)),this,SLOT(OnStripFindVisible(bool)));
}

void SciTEQt::setContent(QObject * obj)
{
    wContent.SetID(obj);
}

// copy file with translations "locale.properties" into directory of the executable
QString SciTEQt::getLocalisedText(const QString & textInput, bool filterShortcuts)
{
    QString s = textInput;
    if( filterShortcuts || isMobilePlatform() )
    {
        s.remove("&");
    }
    auto localisedText = localiser.Text(s.toUtf8()/*textInput.toStdString().c_str()*/,true);
    return ConvertGuiStringToQString(localisedText);
}

bool SciTEQt::saveCurrentAs(const QString & sFileName)
{
    bool ret = false;
    QUrl aUrl(sFileName);
    QString sLocalFileName = aUrl.toLocalFile();
#ifdef Q_OS_WIN
    wchar_t * buf = new wchar_t[sLocalFileName.length()+1];
    sLocalFileName.toWCharArray(buf);
    buf[sLocalFileName.length()] = 0;
    FilePath path(buf);
#else
    FilePath path(sLocalFileName.toStdString());
#endif
    if(path.IsSet())
    {
#ifdef Q_OS_WASM
// TODO improve --> synchronous saving of data needed for webassembly platform !
        ret = SaveBuffer(path, sfSynchronous);
#else
        ret = SaveIfNotOpen(path, false);
#endif
    }
#ifdef Q_OS_WINDOWS
    delete [] buf;
#endif
    return ret;
}

void SciTEQt::cmdNew()
{
    MenuCommand(IDM_NEW);
}

void SciTEQt::cmdOpen()
{
    MenuCommand(IDM_OPEN);
}

void SciTEQt::cmdOpenSelectedFileName()
{
    MenuCommand(IDM_OPENSELECTED);
}

void SciTEQt::cmdCodePageProperty()
{
    MenuCommand(IDM_ENCODING_DEFAULT);
}

void SciTEQt::cmdUtf16BigEndian()
{
    MenuCommand(IDM_ENCODING_UCS2BE);
}

void SciTEQt::cmdUtf16LittleEndian()
{
    MenuCommand(IDM_ENCODING_UCS2LE);
}

void SciTEQt::cmdUtf8WithBOM()
{
    MenuCommand(IDM_ENCODING_UTF8);
}

void SciTEQt::cmdUtf8()
{
    MenuCommand(IDM_ENCODING_UCOOKIE);
}

void SciTEQt::cmdAsHtml()
{
    MenuCommand(IDM_SAVEASHTML);
}

void SciTEQt::cmdAsRtf()
{
    MenuCommand(IDM_SAVEASRTF);
}

void SciTEQt::cmdAsPdf()
{
    MenuCommand(IDM_SAVEASPDF);
}

void SciTEQt::cmdAsLatex()
{
    MenuCommand(IDM_SAVEASTEX);
}

void SciTEQt::cmdAsXml()
{
    MenuCommand(IDM_SAVEASXML);
}

void SciTEQt::cmdPageSetup()
{
    MenuCommand(IDM_PRINTSETUP);
}

void SciTEQt::cmdPrint()
{
#if defined(Q_OS_ANDROID)
    // simulate printing for android: create pdf & share pdf... --> hopefully a printer app can receive this content...

    QString tempFileName = QString("%1.pdf").arg(filePath.BaseName().AsInternal()); // = "_temp_print_output.pdf";

    bool ret = m_pApplicationData->writeAndSendSharedFile(tempFileName, "", "text/pdf", [this](QString name) -> bool
    {
        FilePath tempName(name.toStdString());
        SaveToPDF(tempName);
        return true;
    }, /*bSendFile*/true);

    WindowSetFocus(wEditor);
#else
    MenuCommand(IDM_PRINT);
#endif
}

void SciTEQt::cmdLoadSession()
{
    MenuCommand(IDM_LOADSESSION);
}

void SciTEQt::cmdSaveSession()
{
    MenuCommand(IDM_SAVESESSION);
}

void SciTEQt::cmdRevert()
{
    MenuCommand(IDM_REVERT);
}

void SciTEQt::cmdClose()
{
    MenuCommand(IDM_CLOSE);
}

void SciTEQt::cmdSave()
{
    MenuCommand(IDM_SAVE);
}

void SciTEQt::cmdSaveAs()
{
    MenuCommand(IDM_SAVEAS);
}

void SciTEQt::cmdSaveACopy()
{
    MenuCommand(IDM_SAVEACOPY);
}

void SciTEQt::cmdCopyPath()
{
    MenuCommand(IDM_COPYPATH);
}

// taken from the visiscript project
static bool OpenContainingFolder( const QString & sFullFileName )
{
    QString sFileNamePath = QFileInfo( sFullFileName ).absolutePath();
    QString sCommand;
    QStringList args;
    bool bNotSupported = false;
#if defined(Q_OS_MAC)
    sCommand = "open";
    args << "-R";
    args << QDir::toNativeSeparators(sFileNamePath);
#elif defined(Q_OS_WIN)
    sCommand = "explorer";
    args << QDir::toNativeSeparators(sFileNamePath);
#elif defined(Q_OS_ANDROID)
    bNotSupported = true;
#elif defined(Q_OS_IOS)
    bNotSupported = true;
#elif defined(Q_OS_WASM)
    bNotSupported = true;
#elif defined(Q_OS_LINUX)
    sCommand = "nautilus";      // xdg-open ?
    args << QDir::toNativeSeparators(sFileNamePath);
#else
    bNotSupported = true;
#endif

    // see also:
    // http://stackoverflow.com/questions/3569749/qt-open-default-file-explorer-on-nix
    // http://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt

    if( sCommand.length()>0 )
    {
        return QProcess::startDetached( sCommand, args );
    }

    return !bNotSupported;
}

void SciTEQt::cmdOpenContainingFolder()
{
    bool ok = OpenContainingFolder(filePath.AsUTF8().c_str());

    if( !ok )
    {
        showInfoDialog(tr("Warning: this function is not supported on this platform!"), 0);
    }
}

void SciTEQt::cmdDeleteFiles()
{
    emit startFileDialog(FILES_DIR, "*.*", tr("Delete Files"), false, false, true, "");
}

void SciTEQt::cmdExit()
{
    MenuCommand(IDM_QUIT);
}

void SciTEQt::cmdUndo()
{
    MenuCommand(IDM_UNDO);
}

void SciTEQt::cmdRedo()
{
    MenuCommand(IDM_REDO);
}

void SciTEQt::cmdCut()
{
    MenuCommand(IDM_CUT);
}

void SciTEQt::cmdCopy()
{
    MenuCommand(IDM_COPY);
}

void SciTEQt::cmdPaste()
{
    MenuCommand(IDM_PASTE);
}

void SciTEQt::cmdDuplicate()
{
    MenuCommand(IDM_DUPLICATE);
}

void SciTEQt::cmdDelete()
{
    MenuCommand(IDM_CLEAR);
}

void SciTEQt::cmdSelectAll()
{
    MenuCommand(IDM_SELECTALL);
}

void SciTEQt::cmdCopyAsRtf()
{
    MenuCommand(IDM_COPYASRTF);
}

void SciTEQt::cmdMatchBrace()
{
    MenuCommand(IDM_MATCHBRACE);
}

void SciTEQt::cmdSelectToBrace()
{
    MenuCommand(IDM_SELECTTOBRACE);
}

void SciTEQt::cmdShowCalltip()
{
    MenuCommand(IDM_SHOWCALLTIP);
}

void SciTEQt::cmdCompleteSymbol()
{
    MenuCommand(IDM_COMPLETE);
}

void SciTEQt::cmdCompleteWord()
{
    MenuCommand(IDM_COMPLETEWORD);
}

void SciTEQt::cmdExpandAbbreviation()
{
    MenuCommand(IDM_ABBREV);
}

void SciTEQt::cmdInsertAbbreviation()
{
    MenuCommand(IDM_INS_ABBREV);
}

void SciTEQt::cmdBlockComment()
{
    MenuCommand(IDM_BLOCK_COMMENT);
}

void SciTEQt::cmdBoxComment()
{
    MenuCommand(IDM_BOX_COMMENT);
}

void SciTEQt::cmdStreamComment()
{
    MenuCommand(IDM_STREAM_COMMENT);
}

void SciTEQt::cmdMakeSelectionUppercase()
{
    MenuCommand(IDM_UPRCASE);
}

void SciTEQt::cmdMakeSelectionLowercase()
{
    MenuCommand(IDM_LWRCASE);
}

void SciTEQt::cmdReverseSelectedLines()
{
    MenuCommand(IDM_LINEREVERSE);
}

void SciTEQt::cmdJoin()
{
    MenuCommand(IDM_JOIN);
}

void SciTEQt::cmdSplit()
{
    MenuCommand(IDM_SPLIT);
}

void SciTEQt::cmdFind()
{
    MenuCommand(IDM_FIND);
}

void SciTEQt::cmdFindNext()
{
    MenuCommand(IDM_FINDNEXT);
}

void SciTEQt::cmdFindPrevious()
{
    MenuCommand(IDM_FINDNEXTBACK);
}

void SciTEQt::cmdFindInFiles()
{
    if( isFindInFilesRunning() )
    {
        m_aFindInFiles.StopSearch();
        setFindInFilesRunning(false);
    }
    else
    {
        MenuCommand(IDM_FINDINFILES);
    }
}

void SciTEQt::cmdReplace()
{
    MenuCommand(IDM_REPLACE);
}

void SciTEQt::cmdIncrementalSearch()
{
    MenuCommand(IDM_INCSEARCH);
}

void SciTEQt::cmdSelectionAddNext()
{
    MenuCommand(IDM_SELECTIONADDNEXT);
}

void SciTEQt::cmdSelectionAddEach()
{
    MenuCommand(IDM_SELECTIONADDEACH);
}

void SciTEQt::cmdGoto()
{
    MenuCommand(IDM_GOTO);
}

void SciTEQt::cmdNextBookmark()
{
    MenuCommand(IDM_BOOKMARK_NEXT);
}

void SciTEQt::cmdPreviousBookmark()
{
    MenuCommand(IDM_BOOKMARK_PREV);
}

void SciTEQt::cmdToggleBookmark()
{
    MenuCommand(IDM_BOOKMARK_TOGGLE);
}

void SciTEQt::cmdClearAllBookmarks()
{
    MenuCommand(IDM_BOOKMARK_CLEARALL);
}

void SciTEQt::cmdSelectAllBookmarks()
{
    MenuCommand(IDM_BOOKMARK_SELECT_ALL);
}

void SciTEQt::cmdToggleCurrentFold()
{
    MenuCommand(IDM_EXPAND);
}

void SciTEQt::cmdToggleAllFolds()
{
    MenuCommand(IDM_TOGGLE_FOLDALL);
}

void SciTEQt::cmdFullScreen()
{
    MenuCommand(IDM_FULLSCREEN);
}

void SciTEQt::cmdShowToolBar()
{
    MenuCommand(IDM_VIEWTOOLBAR);
}

void SciTEQt::cmdShowTabBar()
{
    MenuCommand(IDM_VIEWTABBAR);
}

void SciTEQt::cmdShowStatusBar()
{
    MenuCommand(IDM_VIEWSTATUSBAR);
}

void SciTEQt::cmdShowWhitespace()
{
    MenuCommand(IDM_VIEWSPACE);
}

void SciTEQt::cmdShowEndOfLine()
{
    MenuCommand(IDM_VIEWEOL);
}

void SciTEQt::cmdIndentionGuides()
{
    MenuCommand(IDM_VIEWGUIDES);
}

void SciTEQt::cmdLineNumbers()
{
    MenuCommand(IDM_LINENUMBERMARGIN);
}

void SciTEQt::cmdMargin()
{
    MenuCommand(IDM_SELMARGIN);
}

void SciTEQt::cmdFoldMargin()
{
    MenuCommand(IDM_FOLDMARGIN);
}

void SciTEQt::cmdToggleOutput()
{
    MenuCommand(IDM_TOGGLEOUTPUT);
}

void SciTEQt::cmdParameters()
{
    MenuCommand(IDM_TOGGLEPARAMETERS);
}

void SciTEQt::cmdCompile()
{
    MenuCommand(IDM_COMPILE);
}

void SciTEQt::cmdBuild()
{
    MenuCommand(IDM_BUILD);
}

void SciTEQt::cmdClean()
{
    MenuCommand(IDM_CLEAN);
}

void SciTEQt::cmdGo()
{
    MenuCommand(IDM_GO);
}

void SciTEQt::cmdStopExecuting()
{
    MenuCommand(IDM_STOPEXECUTE);
}

void SciTEQt::cmdNextMessage()
{
    MenuCommand(IDM_NEXTMSG);
}

void SciTEQt::cmdPreviousMessage()
{
    MenuCommand(IDM_PREVMSG);
}

void SciTEQt::cmdClearOutput()
{
    MenuCommand(IDM_CLEAROUTPUT);
}

void SciTEQt::cmdSwitchPane()
{
    MenuCommand(IDM_SWITCHPANE);
}

void SciTEQt::cmdAlwaysOnTop()
{
    MenuCommand(IDM_ONTOP);
}

void SciTEQt::cmdOpenFilesHere()
{
    MenuCommand(IDM_OPENFILESHERE);
}

void SciTEQt::cmdReadOnly()
{
    MenuCommand(IDM_READONLY);
}

void SciTEQt::cmdCrLf()
{
    MenuCommand(IDM_EOL_CRLF);
}

void SciTEQt::cmdCr()
{
    MenuCommand(IDM_EOL_CR);
}

void SciTEQt::cmdLf()
{
    MenuCommand(IDM_EOL_LF);
}

void SciTEQt::cmdConvertLineEndChar()
{
    MenuCommand(IDM_EOL_CONVERT);
}

void SciTEQt::cmdChangeIndentationSettings()
{
    MenuCommand(IDM_TABSIZE);
}

void SciTEQt::cmdOpenLocalOptionsFile()
{
    MenuCommand(IDM_OPENLOCALPROPERTIES);
}

void SciTEQt::cmdOpenDirectoryOptionsFile()
{
    MenuCommand(IDM_OPENDIRECTORYPROPERTIES);
}

void SciTEQt::cmdOpenUserOptionsFile()
{
    MenuCommand(IDM_OPENUSERPROPERTIES);
}

void SciTEQt::cmdOpenGlobalOptionsFile()
{
    MenuCommand(IDM_OPENGLOBALPROPERTIES);
}

void SciTEQt::cmdOpenAbbreviationsFile()
{
    MenuCommand(IDM_OPENABBREVPROPERTIES);
}

void SciTEQt::cmdOpenLuaStartupScript()
{
    MenuCommand(IDM_OPENLUAEXTERNALFILE);
}

void SciTEQt::cmdWrap()
{
    MenuCommand(IDM_WRAP);
}

void SciTEQt::cmdWrapOutput()
{
    MenuCommand(IDM_WRAPOUTPUT);
}

void SciTEQt::cmdVerticalSplit()
{
    MenuCommand(IDM_SPLITVERTICAL);
}

void SciTEQt::cmdUseMonospacedFont()
{
    MenuCommand(IDM_MONOFONT);
}

void SciTEQt::cmdSwitchToLastActivatedTab()
{
    int temp = m_iLastTabIndex;
    m_iLastTabIndex = m_iCurrentTabIndex;
    m_iCurrentTabIndex = temp;
    cmdSelectBuffer(temp);
}

void SciTEQt::cmdBuffersPrevious()
{
    MenuCommand(IDM_PREVFILE);
}

void SciTEQt::cmdBuffersNext()
{
    MenuCommand(IDM_NEXTFILE);
}

void SciTEQt::cmdBuffersCloseAll()
{
    MenuCommand(IDM_CLOSEALL);
}

void SciTEQt::cmdBuffersSaveAll()
{
    MenuCommand(IDM_SAVEALL);
}

void SciTEQt::cmdSelectBuffer(int index)
{
    MenuCommand(IDM_BUFFER+index);
}

void SciTEQt::cmdSelectLanguage(int index)
{
    MenuCommand(IDM_LANGUAGE+index);
}

void SciTEQt::cmdCallTool(int index)
{
    MenuCommand(IDM_TOOLS+index);
}

void SciTEQt::cmdCallImport(int index)
{
    MenuCommand(IDM_IMPORT+index);
}

void SciTEQt::cmdLastOpenedFiles(int index)
{
    MenuCommand(IDM_MRUFILE+index);
}

void SciTEQt::cmdHelp()
{
    MenuCommand(IDM_HELP);
}

void SciTEQt::cmdSciteHelp()
{
#ifdef Q_OS_ANDROID
    if( !QDesktopServices::openUrl(QUrl::fromLocalFile("/data/data/org.scintilla.sciteqt/files/SciTEDoc.html")) )
    {
        QDesktopServices::openUrl(QUrl("https://www.scintilla.org/ScintillaDoc.html"));
    }
#else
    if( !QDesktopServices::openUrl(QUrl::fromLocalFile("SciTEDoc.html")) )
    {
        QDesktopServices::openUrl(QUrl("https://www.scintilla.org/ScintillaDoc.html"));
    }
#endif
    //MenuCommand(IDM_HELP_SCITE);
}

void SciTEQt::cmdAboutScite()
{
    MenuCommand(IDM_ABOUT);
}

void SciTEQt::cmdAboutSciteQt()
{
    New();
    QString aboutSciteQt = ApplicationData::simpleReadFileContent(":/about_sciteqt.txt");
    emit setTextToCurrent(getSciteQtInfos()+"\n\n"+aboutSciteQt);
}

void SciTEQt::cmdAboutCurrentFile()
{
    QString sMsg = QString(tr("Current file name=%1 lexer=%2")).arg(filePath.AsUTF8().c_str()).arg(wEditor.LexerLanguage().c_str());

    OnAddLineToOutput(sMsg);
}

static QString g_sJavaScriptLibrary = "function print(t) { env.print(t) }\nfunction admin(val) { env.admin(val) }\n";

void SciTEQt::cmdRunCurrentAsJavaScriptFile()
{
    QString text = QString::fromStdString(wEditor.GetText(wEditor.TextLength()+1).c_str());

    // see: https://doc.qt.io/qt-5/qjsengine.html#details
    QJSEngine myEngine;

    SciteQtEnvironmentForJavaScript aSciteQtJSEnvironment(this);
    connect(&aSciteQtJSEnvironment,SIGNAL(OnPrint(QString)),this,SLOT(OnAddLineToOutput(QString)));
    connect(&aSciteQtJSEnvironment,SIGNAL(OnAdmin(bool)),this,SLOT(OnAdmin(bool)));

    QJSValue sciteEnvironment = myEngine.newQObject(&aSciteQtJSEnvironment);
    myEngine.globalObject().setProperty("env", sciteEnvironment);

    QJSValue result = myEngine.evaluate(g_sJavaScriptLibrary+text, filePath.AsUTF8().c_str());
    QString sResult;
    if (result.isError())
    {
        sResult = QString(tr("Error: Uncaught exception at line %1: %2")).arg(result.property("lineNumber").toInt()).arg(result.toString());
    }
    else
    {
        sResult = QString(tr("Result=%1")).arg(result.toString());
    }
    OnAddLineToOutput(sResult);

    disconnect(&aSciteQtJSEnvironment,SIGNAL(OnAdmin(bool)),this,SLOT(OnAdmin(bool)));
    disconnect(&aSciteQtJSEnvironment,SIGNAL(OnPrint(QString)),this,SLOT(OnAddLineToOutput(QString)));
}

void SciTEQt::cmdRunCurrentAsLuaFile()
{
    ToolsMenu(0);   // call: dofile <filename.lua>
}

void SciTEQt::cmdShare()
{
    QString text = QString::fromStdString(wEditor.GetText(wEditor.TextLength()+1));
    m_pApplicationData->shareSimpleText(text);
    WindowSetFocus(wEditor);
}

void SciTEQt::cmdUpdateApplicationActive(bool active)
{
    // protect against recursive calls...
    if(!m_bIsInUpdateAppActive)
    {
        m_bIsInUpdateAppActive = true;
        Activate(active);
        m_bIsInUpdateAppActive = false;
    }
}

void SciTEQt::cmdMarkAll()
{
    MarkAll(markWithBookMarks);
}

void SciTEQt::cmdTriggerReplace(const QString & find, const QString & replace, bool inSection)
{
    // see: ReplaceStrip::HandleReplaceCommand(...)
    SetFind(find.toStdString().c_str());
    SetReplace(replace.toStdString().c_str());
    if( inSection )
    {
        ReplaceAll(inSection);
    }
    else
    {
        ReplaceOnce(true);
    }
}

void SciTEQt::cmdGotoLine(int lineNo, int colPos)
{
    GotoLineEnsureVisible(lineNo);

    // see WM_COMMAND in SciTEWinDlg.cxx line 1416
    if (/*colPos && colPos.value() > 1 &&*/ lineNo <= wEditor.LineCount())
    {
        // Constrain to the requested line
        const SA::Position lineStart = wEditor.LineStart(lineNo - 1);
        const SA::Position lineEnd = wEditor.LineEnd(lineNo - 1);

        SA::Position characterOnLine = colPos;
        SA::Position position = lineStart;
        while (--characterOnLine && position < lineEnd)
            position = wEditor.PositionAfter(position);

        wEditor.GotoPos(position);
    }
}

void SciTEQt::cmdUpdateTabSizeValues(int tabSize, int indentSize, bool useTabs, bool convert)
{
    if (tabSize > 0)
        wEditor.SetTabWidth(tabSize);
    if (indentSize > 0)
        wEditor.SetIndent(indentSize);
    wEditor.SetUseTabs(useTabs);
    if (convert)
        ConvertIndentation(tabSize, useTabs);
}

void SciTEQt::cmdSetAbbreviationText(const QString & currentText)
{
    abbrevInsert = currentText.toStdString().c_str();
}

void SciTEQt::cmdSetParameters(const QString & cmd, const QString & parameter1, const QString & parameter2, const QString & parameter3, const QString & parameter4)
{
    m_cmd = cmd;
    m_parameter1 = parameter1;
    m_parameter2 = parameter2;
    m_parameter3 = parameter3;
    m_parameter4 = parameter4;
}

void SciTEQt::cmdParametersDialogClosed()
{
    m_bParametersDialogOpen = false;
}

void SciTEQt::cmdContextMenu(int menuID)
{
    MenuCommand(menuID);
}

void SciTEQt::cmdStartFindInFilesAsync(const QString & directory, const QString & filePattern, const QString & findText, bool wholeWord, bool caseSensitive, bool regularExpression)
{
    SetFind(findText.toStdString().c_str());
    //InsertFindInMemory();
    memFiles.Insert(filePattern.toStdString().c_str());
    memDirectory.Insert(directory.toStdString().c_str());

    Searcher * pSearcher = this;
    pSearcher->wholeWord = wholeWord;
    pSearcher->matchCase = caseSensitive;
    pSearcher->regExp = regularExpression;

    m_aFindInFiles.StartSearch(directory, filePattern, findText, caseSensitive, wholeWord, regularExpression);
    setFindInFilesRunning(true);
}

bool SciTEQt::cmdExecuteFind(const QString & findWhatInput, bool wholeWord, bool caseSensitive, bool regularExpression, bool wrap, bool transformBackslash, bool down, bool markAll)
{
    SetFind(findWhatInput.toStdString().c_str());

    // see: DialogFindReplace::GrabFields()
    Searcher * pSearcher = this;
    pSearcher->wholeWord = wholeWord;
    pSearcher->matchCase = caseSensitive;
    pSearcher->regExp = regularExpression;
    pSearcher->wrapFind = wrap;
    pSearcher->unSlash = transformBackslash;
    pSearcher->reverseFind = !down;

    if(markAll)
    {
        MarkAll(markWithBookMarks);
    }

    bool found = FindNext(pSearcher->reverseFind);
    // on mobile platforms --> always close dialog !
    if(isMobilePlatform() || ShouldClose(found))
    {
        return true;
    }
    return false;
}

void SciTEQt::cmdExecuteReplace(const QString & findWhatInput, const QString & replace, bool wholeWord, bool caseSensitive, bool regularExpression, bool wrap, bool transformBackslash, bool replaceAll, bool replaceInSection)
{
    SetFind(findWhatInput.toStdString().c_str());

    // see: DialogFindReplace::GrabFields()
    Searcher * pSearcher = this;
    pSearcher->wholeWord = wholeWord;
    pSearcher->matchCase = caseSensitive;
    pSearcher->regExp = regularExpression;
    pSearcher->wrapFind = wrap;
    pSearcher->unSlash = transformBackslash;
    //pSearcher->reverseFind = false;

    pSearcher->SetReplace(replace.toStdString().c_str());

    // see: SciTEWin::HandleReplaceCommand()

    intptr_t replacements = 0;
    if(replaceAll)
    {
        replacements = ReplaceAll(false);
    }
    else if(replaceInSection)
    {
        replacements = ReplaceAll(true);
    }
    //else if(replaceInBuffers)
    //{
    //    replacements = ReplaceInBuffers();
    //}
    else
    {
        ReplaceOnce();
    }

    QString replDone = QString("%1").arg(replacements);

    emit updateReplacementCount(replDone);
}

QString SciTEQt::cmdDirectoryUp(const QString & directoryPath)
{
    QDir aDirInfo(directoryPath);
    aDirInfo.cdUp();
    return QDir::toNativeSeparators(aDirInfo.absolutePath());
}

QString SciTEQt::cmdUrlToLocalPath(const QString & url)
{
    return QDir::toNativeSeparators(QUrl(url).toLocalFile());
}

void SciTEQt::cmdAboutQt()
{
    QApplication::aboutQt();
}

void SciTEQt::ReadEmbeddedProperties()
{
    propsEmbed.Clear();

    QFile aFile(":/Embedded.properties");
    if( aFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        QByteArray data = aFile.readAll();
        propsEmbed.ReadFromMemory(static_cast<const char *>(data.data()), data.size(), FilePath(), filter, NULL, 0);
    }
}

void SciTEQt::CheckMenus()
{
    SciTEBase::CheckMenus();

    emit updateEolMenus((int)wEditor.EOLMode());

    emit updateEncodingMenus((int)CurrentBuffer()->unicodeMode);
}

void SciTEQt::ResetExecution()
{
    cmdWorker.Initialise(true);
    jobQueue.SetExecuting(false);
    if (needReadProperties)
        ReadProperties();
    CheckReload();
    CheckMenus();
    jobQueue.ClearJobs();
//TODO needed?    ::SendMessage(MainHWND(), WM_COMMAND, IDM_FINISHEDEXECUTE, 0);
}

void SciTEQt::ProcessExecute()
{
    if (scrollOutput)
        wOutput.Send(SCI_GOTOPOS, wOutput.Send(SCI_GETTEXTLENGTH));

    cmdWorker.exitStatus = ExecuteOne(jobQueue.jobQueue[cmdWorker.icmd]);
    if (jobQueue.isBuilding) {
        // The build command is first command in a sequence so it is only built if
        // that command succeeds not if a second returns after document is modified.
        jobQueue.isBuilding = false;
        if (cmdWorker.exitStatus == 0)
            jobQueue.isBuilt = true;
    }

    // Move selection back to beginning of this run so that F4 will go
    // to first error of this run.
    // scroll and return only if output.scroll equals
    // one in the properties file
//    if ((cmdWorker.outputScroll == 1) && returnOutputToCommand)
//        wOutput.Send(SCI_GOTOPOS, cmdWorker.originalEnd);
//    returnOutputToCommand = true;
	PostOnMainThread(TRIGGER_GOTOPOS, &cmdWorker);
    PostOnMainThread(WORK_EXECUTE, &cmdWorker);
}

int SciTEQt::ExecuteOne(const Job &jobToRun)
{
	QString cmd;
	QString args;
	QString workingDirectory = QString::fromStdString(jobToRun.directory.AsUTF8());
	QString s = QString::fromStdString(jobToRun.command);
	int iFirstSpace = s.indexOf(" ");
	if (iFirstSpace >= 0)
	{
		cmd = s.mid(0, iFirstSpace);
		args = s.mid(iFirstSpace);
	}

	//return m_aScriptExecution.DoScriptExecution(cmd, args, workingDirectory);

	ScriptExecution tempScriptExecution;
    m_pCurrentScriptExecution = &tempScriptExecution;
    connect(&tempScriptExecution,SIGNAL(AddToOutput(QString)),this,SLOT(OnAddToOutput(QString)));
    int ret = tempScriptExecution.DoScriptExecution(cmd, args, workingDirectory);
    m_pCurrentScriptExecution = 0;
    disconnect(&tempScriptExecution,SIGNAL(AddToOutput(QString)),this,SLOT(OnAddToOutput(QString)));    
    return ret;
}

void SciTEQt::ExecuteNext()
{
    cmdWorker.icmd++;
    if (cmdWorker.icmd < jobQueue.commandCurrent && cmdWorker.icmd < jobQueue.commandMax && cmdWorker.exitStatus == 0) {
        Execute();
    } else {
        ResetExecution();
    }
}

void SciTEQt::Execute()
{
    // see also: SciTEWin::Execute() and SciTEGTK::Execute()

    if (buffers.SavingInBackground())
        // May be saving file that should be used by command so wait until all saved
        return;

    SciTEBase::Execute();

    if (!jobQueue.HasCommandToRun())
        // No commands to execute - possibly cancelled in SciTEBase::Execute
        return;

    cmdWorker.Initialise(false);
    cmdWorker.outputScroll = props.GetInt("output.scroll", 1);
    cmdWorker.originalEnd = wOutput.Length();
    cmdWorker.commandTime.Duration(true);
    cmdWorker.flags = jobQueue.jobQueue[cmdWorker.icmd].flags;
    if (scrollOutput)
        wOutput.GotoPos(wOutput.Length());

    if (jobQueue.jobQueue[cmdWorker.icmd].jobType == jobExtension) {
        // Execute extensions synchronously
        if (jobQueue.jobQueue[cmdWorker.icmd].flags & jobGroupUndo)
            wEditor.BeginUndoAction();

        if (extender)
            extender->OnExecute(jobQueue.jobQueue[cmdWorker.icmd].command.c_str());

        if (jobQueue.jobQueue[cmdWorker.icmd].flags & jobGroupUndo)
            wEditor.EndUndoAction();

        ExecuteNext();
    } else {
        // Execute other jobs asynchronously on a new thread
		PerformOnNewThread(&cmdWorker);
		//cmdWorker.Execute();
    }
}

void SciTEQt::WorkerCommand(int cmd, Worker *pWorker)
{
    if (cmd < WORK_PLATFORM) {
        SciTEBase::WorkerCommand(cmd, pWorker);
    } else {
		if (cmd == TRIGGER_GOTOPOS) {
			if ((cmdWorker.outputScroll == 1) && returnOutputToCommand)
				wOutput.Send(SCI_GOTOPOS, cmdWorker.originalEnd);
			returnOutputToCommand = true;
		}
        if (cmd == WORK_EXECUTE) {
            // Move to next command
            ExecuteNext();
        }
    }
}

bool SciTEQt::event(QEvent *e)
{
    if(e->type() == POST_TO_MAIN)
    {
        QSciTEQtEvent * pSciteEvent = (QSciTEQtEvent *)e;
        WorkerCommand(pSciteEvent->GetCmd(), pSciteEvent->GetWorker());
        return true;
    }
    else
    {
        return QObject::event(e);
    }
}

void SciTEQt::ProcessSave(bool bSetSavePoint)
{
    if(bSetSavePoint)
    {
        wEditor.SetSavePoint();
    }
    wEditor.ClearDocumentStyle();
    wEditor.Colourise(0, wEditor.LineStart(1));
    Redraw();
    SetWindowName();
    BuffersMenu();
}

bool SciTEQt::Save(SaveFlags sf)
{
    if(filePath.IsNotLocal())
    {
        // only for android...
        QString text = QString::fromStdString(wEditor.GetText(wEditor.TextLength()+1));
        QString sFileName = ConvertGuiCharToQString(filePath.AsNonLocalInternal());
        bool ok = m_pApplicationData->writeFileContent(sFileName, text);
        if(!ok)
        {
            ShowWindowMessageBox(QString(tr("Can not write file %1")).arg(sFileName));
        }
        else
        {
            ProcessSave(true);
        }
        return ok;
    }
    if(isWebassemblyPlatform())
    {
        QString sFileName = ConvertGuiCharToQString(filePath.AsInternal());
        emit saveCurrentForWasm(sFileName);

        ProcessSave(true);
        return true;
    }
    // delegate to base implementation if not already handled
    return SciTEBase::Save(sf);
}

bool SciTEQt::Open(const FilePath &file, OpenFlags of)
{
    if(file.IsNotLocal())
    {
        // only for android...
        QString sFileName = ConvertGuiCharToQString(file.AsNonLocalInternal());
        QString sDecodedFileName = ConvertGuiCharToQString(file.AsInternal());
        QString sContent = m_pApplicationData->readFileContent(sFileName);

        OnAddFileContent(sFileName, sDecodedFileName, sContent, false, false);

        return true;
    }
    return SciTEBase::Open(file, of);
}

void SciTEQt::onStatusbarClicked()
{
    UpdateStatusbarView();
}

static void AddToMenu(QJsonArray & menu, const QString & menuText, int menuId, bool enabled)
{
    QJsonObject menuItem;
    menuItem["display"] = menuText;
    menuItem["menuId"] = menuId;
    menuItem["enabled"] = enabled;
    menu.append(menuItem);
}

QVariant SciTEQt::fillTabContextMenu()
{
    QJsonArray menu;

    // used code from void SciTEWin::Notify(SCNotification *notification) --> case NM_RCLICK:

    AddToMenu(menu, "Close", IDM_CLOSE, true);
    //AddToMenu(menu, "");
    AddToMenu(menu, "Save", IDM_SAVE, true);
    AddToMenu(menu, "Save As", IDM_SAVEAS, true);
    //AddToMenu(menu, "");

    //bool bAddSeparator = false;
    for (int item = 0; item < toolMax; item++) {
        const int itemID = IDM_TOOLS + item;
        std::string prefix = "command.name.";
        prefix += StdStringFromInteger(item);
        prefix += ".";
        std::string commandName = props.GetNewExpandString(prefix.c_str(), filePath.AsUTF8().c_str());
        if (commandName.length()) {
            AddToMenu(menu, commandName.c_str(), itemID, true);
            //bAddSeparator = true;
        }
    }

    //if (bAddSeparator)
    //    AddToMenu(menu, "");

    AddToMenu(menu, "Print", IDM_PRINT, true);
    AddToMenu(menu, "Copy Path", IDM_COPYPATH, true);
    AddToMenu(menu, "Open Containing Folder", IDM_OPEN_CONTAINING_FOLDER, true);

    return QVariant(menu);
}

void SciTEQt::MenuCommand(int cmdID, int source)
{
    switch(cmdID)
    {
        case IDM_OPEN_CONTAINING_FOLDER:
            cmdOpenContainingFolder();
            break;
        default:
            SciTEBase::MenuCommand(cmdID, source);
    }
}

QVariant SciTEQt::fillToLength(const QString & text, const QString & shortcut)
{
    QString fill(" ");
    fill = fill.leftJustified((42-text.length()-shortcut.length()) / 8,'\t');
    return QVariant(text + fill + shortcut);
}

QVariant SciTEQt::fillToLengthWithFont(const QString & text, const QString & shortcut, const QFont & font)
{
    QFontMetricsF metrics(font);
    double lenText = metrics.boundingRect(text).width();
    //double lenShortcut = metrics.boundingRect(shortcut).width();
    double lenSpace = metrics.boundingRect("  ").width();
    //double lenTab = metrics.boundingRect("\t").width();

    QString fill(" ");
    fill = fill.leftJustified((180-lenText)/lenSpace,' ');

    return QVariant(text + fill + shortcut);
}

void SciTEQt::setFindText(const QString & text, bool incremental)
{
    if( incremental )
    {
        // see: SearchStrip::Next(bool select)
        MoveBack();
        SetFind(text.toStdString().c_str());
        FindNext(reverseFind);
        SetCaretAsStart();
    }
    else
    {
        SetFind(text.toStdString().c_str());
        FindNext(reverseFind);
    }
}

QObject * SciTEQt::getDialog(const QString & objectName)
{
    QObject * dlg = childObject<QObject*>(*m_pEngine, objectName, "", false);
    return dlg;

}

QObject * SciTEQt::getCurrentInfoDialog()
{
    return getDialog("infoDialog");
}

QObject * SciTEQt::getCurrentFileDialog()
{
#if defined(Q_OS_MACOS)
    // under macos only the platform filedialog is working (at least for Qt 5.11.3, higher versions not tested yet)
    return getDialog("labsFileDialog");
#else
    return isMobilePlatform() ? getDialog("mobileFileDialog") : getDialog("fileDialog");
#endif
}

void SciTEQt::setApplicationData(ApplicationData * pApplicationData)
{
    m_pApplicationData = pApplicationData;
    if(m_pApplicationData!=0)
    {
        connect(m_pApplicationData,SIGNAL(sendErrorText(QString)),this,SLOT(OnAddToOutput(QString)));
        connect(m_pApplicationData,SIGNAL(fileLoaded(QString,QString,QString,bool,bool)),this,SLOT(OnAddFileContent(QString,QString,QString,bool,bool)));

// TODO working: (maybe) improve the code of this method !!!
        extender = pApplicationData->GetExtension();

        m_pEngine = &pApplicationData->GetQmlApplicationEngine();

        QStringList cmdArgs = QGuiApplication::arguments();
        cmdArgs.removeAt(0);
        QString s = cmdArgs.join("\n");

        GUI::gui_char buf[512];
#ifdef Q_OS_WIN
        int count = s.toWCharArray((wchar_t *)buf);
        buf[count] = 0;
#else
        strcpy(buf,s.toStdString().c_str());
#endif
        GUI::gui_string args = buf;

        // Collect the argv into one string with each argument separated by '\n'
    //    GUI::gui_string args;
    //    for (int arg = 1; arg < argc; arg++) {
    //        if (arg > 1)
    //            args += '\n';
    //        args += argv[arg];
    //    }

        // Process any initial switches
        ProcessCommandLine(args, 0);

        if (props.GetInt("save.position"))
            RestorePosition();

    /*
        // Break up the command line into individual arguments
        GUI::gui_string args = ProcessArgs(cmdLine);
        // Read the command line parameters:
        // In case the check.if.already.open property has been set or reset on the command line,
        // we still get a last chance to force checking or to open a separate instance;
        // Check if the user just want to print the file(s).
        // Don't process files yet.
        const bool bBatchProcessing = ProcessCommandLine(args, 0);
    */
        // No need to check for other instances when doing a batch job:
        // perform some tasks and exit immediately.
    //	if (!bBatchProcessing && props.GetInt("check.if.already.open") != 0) {
    //		uniqueInstance.CheckOtherInstance();
    //	}

        // CreateUI();
        UIAvailable();

    /*
        if (bBatchProcessing) {
            // Reprocess the command line and read the files
            ProcessCommandLine(args, 1);
            Print(false);	// Don't ask user for print parameters
            // Done, we exit the program
            ::PostQuitMessage(0);
            wSciTE.Destroy();
            return;
        }

        if (props.GetInt("check.if.already.open") != 0 && uniqueInstance.FindOtherInstance()) {
            uniqueInstance.SendCommands(GUI::UTF8FromString(cmdLine).c_str());

            // Kill itself, leaving room to the previous instance
            ::PostQuitMessage(0);
            wSciTE.Destroy();
            return;	// Don't do anything else
        }

        // OK, the instance will be displayed
        SizeSubWindows();
        wSciTE.Show();
        if (cmdShow) {	// assume SW_MAXIMIZE only
            ::ShowWindow(MainHWND(), cmdShow);
        }
    */

        ProcessCommandLine(args, 1);

        CheckMenus();
        SizeSubWindows();
        //SetFocus(wEditor);
        WindowSetFocus(wEditor);

    /* TODO: improve startup !

        // Break up the command line into individual arguments
        GUI::gui_string args = ProcessArgs(cmdLine);
        // Read the command line parameters:
        // In case the check.if.already.open property has been set or reset on the command line,
        // we still get a last chance to force checking or to open a separate instance;
        // Check if the user just want to print the file(s).
        // Don't process files yet.
        const bool bBatchProcessing = ProcessCommandLine(args, 0);

        // No need to check for other instances when doing a batch job:
        // perform some tasks and exit immediately.
        if (!bBatchProcessing && props.GetInt("check.if.already.open") != 0) {
            uniqueInstance.CheckOtherInstance();
        }

        // Open all files given on command line.
        // The filenames containing spaces must be enquoted.
        // In case of not using buffers they get closed immediately except
        // the last one, but they move to the MRU file list
        ProcessCommandLine(args, 1);
    */
    }

    // open the untitled (empty) document at startup
    //Open(FilePath());

    // if no real document (but only the default document) is loaded: show the default document for SciteQt
    QString currentFileName = QString::fromStdString(FileNameExt().Name().AsUTF8());
    if( buffers.length == 1 && currentFileName.length()==0 )
    {
        cmdAboutSciteQt();
    }
}

// returns: <false, givenPosAndSize> --> no change needed
// returns: <true, newPosAndSize>    --> change needed --> use new pos and size
static QPair<bool, QRect> CheckWindowPosAndSize(const QRect & windowPosAndSize)
{
    QList<QScreen *> allScreens = QGuiApplication::screens();

    QRect availableScreenGeometry = allScreens.first()->availableVirtualGeometry();

    if(!availableScreenGeometry.contains(windowPosAndSize))
    {
        QRect newPosAndSize(windowPosAndSize);

        // top left position visible ?
        if(!availableScreenGeometry.contains(newPosAndSize.topLeft()))
        {
            // no --> set top left point to top left point of available screen coodinates
            newPosAndSize = QRect(availableScreenGeometry.x(), availableScreenGeometry.y(), newPosAndSize.width(), newPosAndSize.height());
        }
        // is size of window to big?
        if(!availableScreenGeometry.contains(newPosAndSize))
        {
            // make window smaller
            if(newPosAndSize.width()>availableScreenGeometry.width())
            {
                newPosAndSize.setWidth(availableScreenGeometry.width());
            }
            if(newPosAndSize.height()>availableScreenGeometry.height())
            {
                newPosAndSize.setHeight(availableScreenGeometry.height());
            }
        }

        return QPair<bool, QRect>(true, newPosAndSize);
    }

    return QPair<bool, QRect>(false, windowPosAndSize);
}

void SciTEQt::UpdateWindowPosAndSizeIfNeeded(const QRect & rect, bool maximize)
{
    QPair<bool, QRect> checkValues = CheckWindowPosAndSize(rect);

    if(checkValues.first)
    {
        QRect & newRect(checkValues.second);
        emit setWindowPosAndSize(newRect.left(), newRect.top(), newRect.width(), newRect.height(), maximize);
    }
    else
    {
        emit setWindowPosAndSize(rect.left(), rect.top(), rect.width(), rect.height(), maximize);
    }
}

void SciTEQt::RestorePosition()
{
    // for android platform the size of the main window must not change !!! --> ignore RestorePosition call
#if defined(Q_OS_ANDROID) || defined(Q_OW_WASM)
    // ignore restore !
#else
    const int left = propsSession.GetInt("position.left", 0);
    const int top = propsSession.GetInt("position.top", 0);
    const int width = propsSession.GetInt("position.width", 600);
    const int height = propsSession.GetInt("position.height", 800);
    bool maximize = propsSession.GetInt("position.maximize", 0)==1;

    // move and resize window to visible position and size (if needed)
    UpdateWindowPosAndSizeIfNeeded(QRect(left, top, width, height), maximize);
#endif
}

void SciTEQt::setSpliterPos(int currentPosX, int currentPosY)
{
    GUI::Point pt(currentPosX, currentPosY);
    MoveSplit(pt);
}

void SciTEQt::startDragSpliterPos(int currentPosX, int currentPosY)
{
    GUI::Point pt(currentPosX, currentPosY);
    ptStartDrag = pt;
}

bool SciTEQt::useSimpleMenus() const
{
    return isMobilePlatform();
}

void SciTEQt::setMobilePlatform(bool val)
{
    if(val != m_bIsMobilePlatfrom)
    {
        m_bIsMobilePlatfrom = val;
        emit mobilePlatformChanged();
        emit useMobileDialogHandlingChanged();
    }
}

bool SciTEQt::isUseMobileDialogHandling() const
{
    return isMobilePlatform() || isWebassemblyPlatform();
}

bool SciTEQt::isMobilePlatform() const
{
    return m_bIsMobilePlatfrom;
}

bool SciTEQt::isWebassemblyPlatform() const
{
#if defined(Q_OS_WASM)
    return true;
#else
    return false;
#endif
}

bool SciTEQt::isMacOSPlatform() const
{
#if defined(Q_OS_MACOS)
    return true;
#else
    return false;
#endif
}

void SciTEQt::updateCurrentWindowPosAndSize(int left, int top, int width, int height, bool maximize)
{
    m_left = left;
    m_top = top;
    m_width = width;
    m_height = height;
    m_maximize = maximize;
}

void SciTEQt::updateCurrentSelectedFileUrl(const QString & fileUrl)
{    
    m_sCurrentFileUrl = fileUrl;
}

QString SciTEQt::getSciteQtInfos() const
{
    return QString("SciteQt Version %1 from %2").arg(__SCITE_QT_VERSION__).arg(__DATE__);
}

void SciTEQt::logToDebug(const QString & text)
{
    qDebug() << text << endl;
}

void SciTEQt::testFunction(const QString & text)
{
    // place for some debugging code...
    Q_UNUSED(text);
}

void SciTEQt::UpdateStatusbarView()
{
    sbNum++;
    if (sbNum > props.GetInt("statusbar.number")) {
        sbNum = 1;
    }
    UpdateStatusBar(true);
}

void SciTEQt::OnAcceptedClicked()
{
    m_bWaitDoneFlag = true;
    m_iMessageDialogAccepted |= MSGBOX_RESULT_OK;
}

void SciTEQt::OnRejectedClicked()
{
    m_bWaitDoneFlag = true;
    m_iMessageDialogAccepted |= MSGBOX_RESULT_CANCEL;
}

void SciTEQt::OnFileDialogAcceptedClicked()
{
    m_bFileDialogWaitDoneFlag = true;
    m_iFileDialogMessageDialogAccepted = MSGBOX_RESULT_OK;
}

void SciTEQt::OnFileDialogRejectedClicked()
{
    m_bFileDialogWaitDoneFlag = true;
    m_iFileDialogMessageDialogAccepted = MSGBOX_RESULT_CANCEL;
}

void SciTEQt::OnOkClicked()
{
	m_bWaitDoneFlag = true;
	m_iMessageDialogAccepted |= MSGBOX_RESULT_OK;
}

void SciTEQt::OnCancelClicked()
{
    m_bWaitDoneFlag = true;
    m_iMessageDialogAccepted |= MSGBOX_RESULT_CANCEL;
}

void SciTEQt::OnYesClicked()
{
    m_bWaitDoneFlag = true;
    m_iMessageDialogAccepted |= MSGBOX_RESULT_YES;
}

void SciTEQt::OnNoClicked()
{
    m_bWaitDoneFlag = true;
    m_iMessageDialogAccepted |= MSGBOX_RESULT_NO;
}

void SciTEQt::OnNotifiedFromScintilla(SCNotification scn)
{
    Notify(&scn);
}

void SciTEQt::OnNotifiedFromOutput(SCNotification scn)
{
	Notify(&scn);
}

void SciTEQt::OnUriDroppedFromScintilla(const QString & uri)
{
    Open(GetPathFromUrl(uri));
}

void SciTEQt::OnAddToOutput(const QString & text)
{
    OutputAppendStringSynchronised(text.toStdString().c_str());
    ShowOutputOnMainThread();
	//QCoreApplication::processEvents();
}

void SciTEQt::OnCurrentFindInFilesItemChanged(const QString & currentItem)
{
    setStatusBarText(currentItem);

    if(!m_bStatusBarTextTimerRunning)
    {
        m_bStatusBarTextTimerRunning = true;
        QTimer::singleShot(2000, [this]() { this->setStatusBarText(m_sSciteStatusBarText); m_bStatusBarTextTimerRunning = false; });
    }
}

void SciTEQt::OnFileSearchFinished()
{
    setFindInFilesRunning(false);
}

void SciTEQt::OnAddFileContent(const QString & sFileUri, const QString & sDecodedFileUri, const QString & sContent, bool bNewCreated, bool bSaveACopyModus)
{
    // process receiving new content from android storage framework:
    // - create new document
    // - set name
    // - set content

    bool ok = true;
    if(!bNewCreated)
    {
        // open modus
        if (m_pFcnReceiveContentToProcess!=nullptr)
        {
            (*m_pFcnReceiveContentToProcess)(sContent);

            CheckAndDeleteReceiveContentToProcessFunctionPointer();
        }
        else
        {
            New();
            emit setTextToCurrent(sContent);
        }
    }
    else
    {
        // save as modus --> create new document
        QVariant aData;

        QString text;
        if (m_pFcnGetContentToWrite!=nullptr)
        {
            text = (*m_pFcnGetContentToWrite)();

            CheckAndDeleteGetContentToWriteFunctionPointer();
        }
        else
        {
            text = QString::fromStdString(wEditor.GetText(wEditor.TextLength()+1));
        }

        ok = m_pApplicationData->writeFileContent(sFileUri, text);
        if(!ok)
        {
            ShowWindowMessageBox(QString(tr("Can not write file %1")).arg(sDecodedFileUri));
        }        
    }

    if( !bSaveACopyModus )
    {
        // nearly the code from SaveAs()...
        ReadProperties();
        FilePath newFileName(ConvertQStringToGuiString(sDecodedFileUri), ConvertQStringToGuiString(sFileUri));
        SetFileName(newFileName, /*fixCase*/true);
        //Save();
        ProcessSave(ok);
        if (extender)
            extender->OnSave(filePath.AsUTF8().c_str());
    }
}

void SciTEQt::OnAddLineToOutput(const QString & text)
{
    OnAddToOutput(text+"\n");
}

void SciTEQt::OnStripFindVisible(bool val)
{
    m_bStripFindVisible = val;
}

/*
static void DumpScreens()
{
    QList<QScreen *> allScreens = QGuiApplication::screens();

    for(QScreen * pScreen : allScreens)
    {
        qDebug() << "Screen: " << endl;
        qDebug() << "name= " << pScreen->name() << endl;
        qDebug() << "manufactor= "<< pScreen->manufacturer() << endl;
        qDebug() << "model= "<< pScreen->model() << endl;
        qDebug() << "size= "<< pScreen->size() << endl;
        qDebug() << "virtualsize= "<< pScreen->virtualSize() << endl;
        qDebug() << "availablesize= "<< pScreen->availableSize() << endl;
        qDebug() << "availablevirtualsize= "<< pScreen->availableVirtualSize() << endl;
        qDebug() << "pyhsicalsize(in mm)= "<< pScreen->physicalSize() << endl;
        qDebug() << "geometry= "<< pScreen->geometry() << endl;
        qDebug() << "virtualgeometry= "<< pScreen->virtualGeometry() << endl;
        qDebug() << "availablegeometry= "<< pScreen->availableGeometry() << endl;
        qDebug() << "availablevirtualgeometry= "<< pScreen->availableVirtualGeometry() << endl;
        qDebug() << "logicalDotsPerInch= "<< pScreen->logicalDotsPerInch() << endl;
        qDebug() << "physicalDotsPerInch= "<< pScreen->physicalDotsPerInch() << endl;
        qDebug() << "devicePixelRatio= "<< pScreen->devicePixelRatio() << endl;
    }
}
*/

void SciTEQt::OnScreenAdded(QScreen * pScreen)
{
    // nothing to do, new screen does not affect visible sciteqt window

    //DumpScreens();
    Q_UNUSED(pScreen);
}

void SciTEQt::OnScreenRemoved(QScreen * pScreen)
{
    Q_UNUSED(pScreen);

    // removing a screen might make sciteqt window invisible (if it was visible on the removed screen)
    int left;
    int top;
    int width;
    int height;
    int maximize;
    GetWindowPosition(&left, &top, &width, &height, &maximize);

    UpdateWindowPosAndSizeIfNeeded(QRect(left, top, width, height), maximize);

    //DumpScreens();
}

void SciTEQt::OnPrimaryScreenChanged(QScreen * pScreen)
{
    //DumpScreens();
    Q_UNUSED(pScreen);
}

void SciTEQt::OnAdmin(bool value)
{
    emit admin(value);
}

//*************************************************************************

QtCommandWorker::QtCommandWorker() noexcept {
    Initialise(true);
}

void QtCommandWorker::Initialise(bool resetToStart) noexcept {
    if (resetToStart)
        icmd = 0;
    originalEnd = 0;
    exitStatus = 0;
    flags = 0;
    seenOutput = false;
    outputScroll = 1;
}

void QtCommandWorker::Execute() {
    pSciTE->ProcessExecute();
}
