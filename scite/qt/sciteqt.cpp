#include "sciteqt.h"

#include <QUrl>
#include <QDir>
#include <QResource>
#include <QThread>
#include <QGuiApplication>
#include <QClipboard>
#include <QQmlContext>

#include <QDebug>

#include "ScintillaEditBase.h"
#include "ScintillaQt.h"

QString ConvertGuiCharToQString(const GUI::gui_char * s)
{
#ifdef Q_OS_WINDOWS
    return QString::fromWCharArray(s);
#else
    return QString(s);
#endif
}

QString ConvertGuiStringToQString(const GUI::gui_string & s)
{
#ifdef Q_OS_WINDOWS
    return QString::fromWCharArray(s.c_str());
#else
    return QString(s.c_str());
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

/*
void SciTEGTK::Run(int argc, char *argv[]) {
    // Load the default session file
    if (props.GetInt("save.session") || props.GetInt("save.position") || props.GetInt("save.recent")) {
        LoadSessionFile("");
    }

    // Find the SciTE executable, first trying to use argv[0] and converting
    // to an absolute path and if that fails, searching the path.
    sciteExecutable = FilePath(argv[0]).AbsolutePath();
    if (!sciteExecutable.Exists()) {
        gchar *progPath = g_find_program_in_path(argv[0]);
        sciteExecutable = FilePath(progPath);
        g_free(progPath);
    }

    // Collect the argv into one string with each argument separated by '\n'
    GUI::gui_string args;
    for (int arg = 1; arg < argc; arg++) {
        if (arg > 1)
            args += '\n';
        args += argv[arg];
    }

    // Process any initial switches
    ProcessCommandLine(args, 0);

    // Check if SciTE is already running.
    if ((props.GetString("ipc.director.name").size() == 0) && props.GetInt ("check.if.already.open")) {
        if (CheckForRunningInstance (argc, argv)) {
            // Returning from this function exits the program.
            return;
        }
    }

    CreateUI();
    if ((props.GetString("ipc.director.name").size() == 0) && props.GetInt ("check.if.already.open"))
        unlink(uniqueInstance.c_str()); // Unlock.

    // Process remaining switches and files
#ifndef GDK_VERSION_3_6
    gdk_threads_enter();
#endif
    ProcessCommandLine(args, 1);
#ifndef GDK_VERSION_3_6
    gdk_threads_leave();
#endif

    CheckMenus();
    SizeSubWindows();
    SetFocus(wEditor);
    gtk_widget_grab_focus(GTK_WIDGET(PWidget(wSciTE)));

#ifndef GDK_VERSION_3_6
    gdk_threads_enter();
#endif
    gtk_main();
#ifndef GDK_VERSION_3_6
    gdk_threads_leave();
#endif
}
*/

SciTEQt::SciTEQt(QObject *parent, QQmlApplicationEngine * pEngine)
    : QObject(parent),
      m_pApplicationData(0),
      m_pEngine(pEngine),
      m_bWaitDoneFlag(false),
      m_iMessageDialogAccepted(MSGBOX_RESULT_CANCEL),
      m_bShowToolBar(false),
      m_bShowStatusBar(false)
{
#ifdef Q_OS_WINDOWS
    propsPlatform.Set("PLAT_WIN", "1");
    propsPlatform.Set("PLAT_WINNT", "1");
#endif
#ifdef Q_OS_LINUX
    propsPlatform.Set("PLAT_GTK", "1");
#endif
#ifdef Q_OS_MACOS
    propsPlatform.Set("PLAT_MACOSX", "1");
#endif
#ifdef Q_OS_ANDROID
    propsPlatform.Set("PLAT_GTK", "1");
    propsPlatform.Set("PLAT_ANDROID", "1");
#endif
#ifdef Q_OS_IOS
    propsPlatform.Set("PLAT_MACOSX", "1");
    propsPlatform.Set("PLAT_IOS", "1");
#endif
#ifdef Q_OS_WASM
    propsPlatform.Set("PLAT_GTK", "1");
    propsPlatform.Set("PLAT_WASM", "1");
#endif

    ReadEnvironment();

    ReadGlobalPropFile();
    SetPropertiesInitial();
    ReadAbbrevPropFile();

    CreateBuffers();

    // from SciTEWin.cxx Run():

    // Load the default session file
    if (props.GetInt("save.session") || props.GetInt("save.position") || props.GetInt("save.recent")) {
        LoadSessionFile(GUI_TEXT(""));
    }
}

void SciTEQt::TabInsert(int index, const GUI::gui_char *title)
{

}

void SciTEQt::TabSelect(int index)
{

}

void SciTEQt::RemoveAllTabs()
{

}

void SciTEQt::WarnUser(int warnID)
{

}

void SciTEQt::GetWindowPosition(int *left, int *top, int *width, int *height, int *maximize)
{

}

bool SciTEQt::OpenDialog(const FilePath &directory, const GUI::gui_char *filesFilter)
{
    emit startFileDialog(directory.AbsolutePath().AsUTF8().c_str(), ConvertGuiCharToQString(filesFilter), true);
    return true;
}

bool SciTEQt::SaveAsDialog()
{
    emit startFileDialog("", "", false);
    return true;
}

void SciTEQt::SaveACopy()
{

}

void SciTEQt::SaveAsRTF()
{

}

void SciTEQt::SaveAsPDF()
{

}

void SciTEQt::SaveAsTEX()
{

}

void SciTEQt::SaveAsXML()
{

}

void SciTEQt::SaveAsHTML()
{

}

FilePath SciTEQt::GetDefaultDirectory()
{
    return FilePath();
}

FilePath SciTEQt::GetSciteDefaultHome()
{
    return FilePath();
}

FilePath SciTEQt::GetSciteUserHome()
{
    return FilePath();
}

void SciTEQt::Find()
{

}

SciTEQt::MessageBoxChoice SciTEQt::WindowMessageBox(GUI::Window &w, const GUI::gui_string &msg, MessageBoxStyle style)
{
    emit showInfoDialog(ConvertGuiStringToQString(msg), style);

    QObject * pMessageBox = getCurrentInfoDialog();
    connect(pMessageBox,SIGNAL(accepted()),this,SLOT(OnAcceptedClicked()));
    connect(pMessageBox,SIGNAL(rejected()),this,SLOT(OnRejectedClicked()));
    connect(pMessageBox,SIGNAL(yes()),this,SLOT(OnYesClicked()));
    connect(pMessageBox,SIGNAL(no()),this,SLOT(OnNoClicked()));

    // simulate a synchronious call: wait for signal from MessageBox and then return with result
    m_bWaitDoneFlag = false;
    while(!m_bWaitDoneFlag)
    {
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }

    disconnect(pMessageBox,SIGNAL(accepted()),this,SLOT(OnAcceptedClicked()));
    disconnect(pMessageBox,SIGNAL(rejected()),this,SLOT(OnRejectedClicked()));
    disconnect(pMessageBox,SIGNAL(yes()),this,SLOT(OnYesClicked()));
    disconnect(pMessageBox,SIGNAL(no()),this,SLOT(OnNoClicked()));

    SciTEQt::MessageBoxChoice result;
    switch(m_iMessageDialogAccepted)
    {
        case MSGBOX_RESULT_CANCEL:
            result = SciTEQt::MessageBoxChoice::mbCancel;
            break;
        case MSGBOX_RESULT_OK:
            result = SciTEQt::MessageBoxChoice::mbOK;
            break;
        case MSGBOX_RESULT_NO:
            result = SciTEQt::MessageBoxChoice::mbNo;
            break;
        case MSGBOX_RESULT_YES:
            result = SciTEQt::MessageBoxChoice::mbYes;
            break;
        default:
            result = SciTEQt::MessageBoxChoice::mbCancel;
    }
    return result;
}

void SciTEQt::FindMessageBox(const std::string &msg, const std::string *findItem)
{

}

void SciTEQt::FindIncrement()
{

}

void SciTEQt::FindInFiles()
{

}

void SciTEQt::Replace()
{

}

void SciTEQt::DestroyFindReplace()
{

}

void SciTEQt::GoLineDialog()
{

}

bool SciTEQt::AbbrevDialog()
{
    return false;
}

void SciTEQt::TabSizeDialog()
{

}

bool SciTEQt::ParametersOpen()
{
    return false;
}

void SciTEQt::ParamGrab()
{

}

bool SciTEQt::ParametersDialog(bool modal)
{
    return false;
}

void SciTEQt::FindReplace(bool replace)
{

}

void SciTEQt::StopExecute()
{

}

void SciTEQt::SetFileProperties(PropSetFile &ps)
{

}

void SciTEQt::AboutDialog()
{
    // TODO: see: SciTEBase::SetAboutMessage
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

void SciTEQt::SetStatusBarText(const char *s)
{
    setStatusBarText(s);
}

void SciTEQt::ShowToolBar()
{
    setShowToolBar(tbVisible);
}

void SciTEQt::ShowTabBar()
{
}

void SciTEQt::ShowStatusBar()
{
    setShowStatusBar(sbVisible);
}

void SciTEQt::ActivateWindow(const char *timestamp)
{

}

void SciTEQt::SizeContentWindows()
{

}

void SciTEQt::SizeSubWindows()
{

}

void SciTEQt::SetMenuItem(int menuNumber, int position, int itemID,
             const GUI::gui_char *text, const GUI::gui_char *mnemonic)
{
    // 0 18 1000    // buffers
    // MenuID  7  pos= 5   1200
    // Menu 7 == Buffers --> create

    // TODO: dynamisches menu handling implementieren

    // 6 0 1400
    // 6 1 1401

    if(menuNumber == 7)
    {
        if( itemID >= IDM_BUFFER && itemID < IDM_IMPORT)
        {
            int posForThisItem = itemID - IDM_BUFFER;

            emit setInBuffersModel(posForThisItem, ConvertGuiCharToQString(text), false);
        }
    }
    if(menuNumber == 6)
    {
        if( itemID >= IDM_LANGUAGE && itemID < IDM_LANGUAGE+100 )
        {
            int posForThisItem = itemID - IDM_LANGUAGE;

            emit setInLanguagesModel(posForThisItem, ConvertGuiCharToQString(text), false);
        }
    }

//    qDebug() << "Set Menu Item " << menuNumber << " pos=" << position << " " << itemID << " " << QString::fromWCharArray(text) << " " << QString::fromWCharArray(mnemonic) << endl;
}

void SciTEQt::DestroyMenuItem(int menuNumber, int itemID)
{
//    qDebug() << "DestroyMenuItem" << menuNumber << " " << itemID << endl;
    if(menuNumber == 7)
    {
        int posForThisItem = itemID - IDM_BUFFER;
        emit removeInBuffersModel(posForThisItem);
    }
    else if(menuNumber == 6)
    {
        int posForThisItem = itemID - IDM_LANGUAGE;
        emit removeInBuffersModel(posForThisItem);
    }
}

void SciTEQt::CheckAMenuItem(int wIDCheckItem, bool val)
{
//qDebug() << "CheckAMenuItem" << wIDCheckItem << " " << val << endl;
    if( wIDCheckItem >= IDM_BUFFER && wIDCheckItem < IDM_IMPORT )
    {
        emit checkStateInBuffersModel(wIDCheckItem-IDM_BUFFER, val);
    }
    else if( wIDCheckItem >= IDM_LANGUAGE && wIDCheckItem < IDM_LANGUAGE+100 )
    {
        emit checkStateInLanguagesModel(wIDCheckItem-IDM_LANGUAGE, val);
    }
    else
    {
        emit setMenuChecked(wIDCheckItem, val);
    }
}

void SciTEQt::EnableAMenuItem(int wIDCheckItem, bool val)
{
//    qDebug() << "EnableAMenuItem" << wIDCheckItem << " " << val << endl;
}

void SciTEQt::AddToPopUp(const char *label, int cmd, bool enabled)
{
    qDebug() << "AddToPopup " << label << " " << cmd << " " << enabled << endl;
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
    int count = s.toWCharArray((wchar_t *)buf);
    buf[count] = 0;
    FilePath path(buf); // StdString().c_str()
    return Open(path);
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
}

void SciTEQt::setOutput(QObject * obj)
{
    ScintillaEditBase * base = reinterpret_cast<ScintillaEditBase *>(obj);

    SciFnDirect fn_ = reinterpret_cast<SciFnDirect>(base->send(SCI_GETDIRECTFUNCTION, 0, 0));
    const sptr_t ptr_ = base->send(SCI_GETDIRECTPOINTER, 0, 0);
    wOutput.SetFnPtr(fn_, ptr_);
    wOutput.SetID(base->sqt);
}

void SciTEQt::setMainWindow(QObject * obj)
{
    QQuickWindow * window = reinterpret_cast<QQuickWindow *>(obj);

    wSciTE.SetID(window);
}

// copy file with translations "locale.properties" into directory of the executable
QString SciTEQt::getLocalisedText(const QString & textInput)
{
    auto localisedText = localiser.Text(textInput.toStdString().c_str(),true);
    return ConvertGuiStringToQString(localisedText);
}

bool SciTEQt::saveCurrentAs(const QString & sFileName)
{
    bool ret = false;
    QUrl aUrl(sFileName);
    QString sLocalFileName = aUrl.toLocalFile();
#ifdef Q_OS_WINDOWS
    wchar_t * buf = new wchar_t[sLocalFileName.length()+1];
    sLocalFileName.toWCharArray(buf);
    buf[sLocalFileName.length()] = 0;
    FilePath path(buf);
#else
    FilePath path(sFileName.toStdString());
#endif
    if(path.IsSet())
    {
        ret = SaveIfNotOpen(path, false);
    }
#ifdef Q_OS_WINDOWS
    delete [] buf;
#endif
    return ret;
}

void SciTEQt::CmdNew()
{
    MenuCommand(IDM_NEW);
}

void SciTEQt::CmdOpen()
{
    MenuCommand(IDM_OPEN);
}

void SciTEQt::CmdRevert()
{
    MenuCommand(IDM_REVERT);
}

void SciTEQt::CmdClose()
{
    MenuCommand(IDM_CLOSE);
}

void SciTEQt::CmdSave()
{
    MenuCommand(IDM_SAVE);
}

void SciTEQt::CmdSaveAs()
{
    MenuCommand(IDM_SAVEAS);
}

void SciTEQt::CmdCopyPath()
{
    MenuCommand(IDM_COPYPATH);
}

void SciTEQt::CmdExit()
{
    MenuCommand(IDM_QUIT);
}

void SciTEQt::CmdUndo()
{
    MenuCommand(IDM_UNDO);
}

void SciTEQt::CmdRedo()
{
    MenuCommand(IDM_REDO);
}

void SciTEQt::CmdCut()
{
    MenuCommand(IDM_CUT);
}

void SciTEQt::CmdCopy()
{
    MenuCommand(IDM_COPY);
}

void SciTEQt::CmdPaste()
{
    MenuCommand(IDM_PASTE);
}

void SciTEQt::CmdFind()
{
    MenuCommand(IDM_FIND);
}

void SciTEQt::CmdFindNext()
{
    MenuCommand(IDM_FINDNEXT);
}

void SciTEQt::CmdFindPrevious()
{
    MenuCommand(IDM_FINDNEXTBACK);
}

void SciTEQt::CmdShowToolBar()
{
    MenuCommand(IDM_VIEWTOOLBAR);
}

void SciTEQt::CmdShowStatusBar()
{
    qDebug() << "show status bar" << endl;
    MenuCommand(IDM_VIEWSTATUSBAR);
}

void SciTEQt::CmdLineNumbers()
{
    MenuCommand(IDM_LINENUMBERMARGIN);
}

void SciTEQt::CmdAlwaysOnTop()
{
    MenuCommand(IDM_ONTOP);
}

void SciTEQt::CmdUseMonospacedFont()
{
    MenuCommand(IDM_MONOFONT);
}

void SciTEQt::CmdBuffersPrevious()
{
    MenuCommand(IDM_PREVFILE);
}

void SciTEQt::CmdBuffersNext()
{
    MenuCommand(IDM_NEXTFILE);
}

void SciTEQt::CmdBuffersCloseAll()
{
    MenuCommand(IDM_CLOSEALL);
}

void SciTEQt::CmdBuffersSaveAll()
{
    MenuCommand(IDM_SAVEALL);
}

void SciTEQt::CmdSelectBuffer(int index)
{
    MenuCommand(IDM_BUFFER+index);
}

void SciTEQt::CmdSelectLanguage(int index)
{
    MenuCommand(IDM_LANGUAGE+index);
}

void SciTEQt::CmdHelp()
{
    MenuCommand(IDM_HELP);
}

void SciTEQt::CmdSciteHelp()
{
    MenuCommand(IDM_HELP_SCITE);
}

void SciTEQt::CmdAboutScite()
{
    MenuCommand(IDM_ABOUT);
}

void SciTEQt::ReadEmbeddedProperties()
{
    propsEmbed.Clear();

    QFile aFile(":/Embedded.properties");
    if( aFile.open(QIODevice::ReadOnly | QIODevice::Text) > 0 )
    {
        QByteArray data = aFile.readAll();
        propsEmbed.ReadFromMemory(static_cast<const char *>(data.data()), data.size(), FilePath(), filter, NULL, 0);
    }
}

bool SciTEQt::event(QEvent *e)
{
    //qDebug() << "EVENT " << e->type() << endl;
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

void SciTEQt::onStatusbarClicked()
{
    UpdateStatusbarView();
}

QObject * SciTEQt::getCurrentInfoDialog()
{
    QObject * infoDlg = childObject<QObject*>(*m_pEngine, "infoDialog", "", false);
    return infoDlg;
}

void SciTEQt::setApplicationData(ApplicationData * pApplicationData)
{
    m_pApplicationData = pApplicationData;
    if(m_pApplicationData!=0)
    {
        m_pEngine = &pApplicationData->GetQmlApplicationEngine();

        if(m_pEngine!=0)
        {
// TODO --> diese properties nach main verschieben... damit sie gleich zu beginn der qmlengine da sind
// ==> ApplicationData -> ApplicationInitData oder scite auch als property am context setzen ?
            //m_pEngine->rootContext()->setContextProperty("buffersModel", &m_aBuffers);
            //m_pEngine->rootContext()->setContextProperty("languagesModel", &m_aLanguages);
        }

        QStringList cmdArgs = QGuiApplication::arguments();
        cmdArgs.removeAt(0);
        QString s = cmdArgs.join("\n");
        qDebug() << "INIT " << s << endl;

        GUI::gui_char buf[512];
        int count = s.toWCharArray((wchar_t *)buf);
        buf[count] = 0;
        GUI::gui_string args = buf;

        // Collect the argv into one string with each argument separated by '\n'
    //    GUI::gui_string args;
    //    for (int arg = 1; arg < argc; arg++) {
    //        if (arg > 1)
    //            args += '\n';
    //        args += argv[arg];
    //    }

        // Process any initial switches
//        ProcessCommandLine(args, 0);

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

    // TODO --> nach setApplicationData --> besser applicationdata direkt beim konstruieren rein reichen... ??? Property binding?
//        ProcessCommandLine(args, 1);

        CheckMenus();
        SizeSubWindows();
        //SetFocus(wEditor);

    qDebug() << "SciteQt::SciteQt " << (void *)this << endl;
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

    qDebug() << "----> OPEN" << endl;

    // open the untitled (empty) document at startup
    Open(FilePath());
// TODO --> update ui components wie statusbar etc.
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
    m_iMessageDialogAccepted = MSGBOX_RESULT_OK;
}

void SciTEQt::OnRejectedClicked()
{
    m_bWaitDoneFlag = true;
    m_iMessageDialogAccepted = MSGBOX_RESULT_CANCEL;
}

void SciTEQt::OnYesClicked()
{
    m_bWaitDoneFlag = true;
    m_iMessageDialogAccepted = MSGBOX_RESULT_YES;
}

void SciTEQt::OnNoClicked()
{
    m_bWaitDoneFlag = true;
    m_iMessageDialogAccepted = MSGBOX_RESULT_NO;
}

void SciTEQt::OnNotifiedFromScintilla(SCNotification scn)
{
    Notify(&scn);
}

void SciTEQt::OnNotifiedFromOutput(SCNotification scn)
{
}
/*
DynamicMenuModel::DynamicMenuModel()
    : m_aRoles(QStandardItemModel::roleNames())
{
    // see: https://forum.qt.io/topic/88211/binding-qstandarditemmodel-from-c-with-qml/3
    m_aRoles[Qt::CheckStateRole] = "checkState";
}

QHash<int,QByteArray> DynamicMenuModel::roleNames() const
{
    return m_aRoles;
}
*/
