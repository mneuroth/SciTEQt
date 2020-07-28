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
    // TODO: implement tab bar ui
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
    qDebug() << "SizeContentWindows " << heightOutput << " " << endl;

}

void SciTEQt::SizeSubWindows()
{
    qDebug() << "SizeSubWindows " << heightOutput << " " << splitVertical << endl;
    emit setVerticalSplit(splitVertical);
    emit setOutputHeight(heightOutput);
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
    else if(menuNumber == 6)
    {
        if( itemID >= IDM_LANGUAGE && itemID < IDM_LANGUAGE+100 )
        {
            int posForThisItem = itemID - IDM_LANGUAGE;

            emit setInLanguagesModel(posForThisItem, ConvertGuiCharToQString(text), false);
        }
    }
    else
    {
        qDebug() << "UN_HANDLED: Set Menu Item " << menuNumber << " pos=" << position << " " << itemID << " " << endl; //QString::fromWCharArray(text) << " " << QString::fromWCharArray(mnemonic) << endl;
    }
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
    else
    {
        qDebug() << "NOT HANDLED Destroy Menu Item " << menuNumber << " item=" << itemID << endl;
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
    emit setMenuEnable(wIDCheckItem, val);
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
#ifdef Q_OS_WIN
    int count = s.toWCharArray((wchar_t *)buf);
    buf[count] = 0;
#else
    strcpy(buf,s.toStdString().c_str());
#endif
    FilePath path(buf); // StdString().c_str()
#ifdef Q_OS_ANDROID
// TODO implement for android
    // temporary workaround for qt fileopen dialog on android if url contains content://
    // a) read real file via QFile and write local temporary file
    // b) read local temporary file via Open(path)
    // or:
    // a) read dummy file via Open("dummy-path")
    // b) read real file via QFile
    // c) set file content at buffer/document
    return Open(path);
#else
    return Open(path);
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

void SciTEQt::setContent(QObject * obj)
{
    QQuickWindow * window = reinterpret_cast<QQuickWindow *>(obj);

    wContent.SetID(window);
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

void SciTEQt::CmdOpenSelectedFileName()
{
    MenuCommand(IDM_OPENSELECTED);
}

void SciTEQt::CmdCodePageProperty()
{
    MenuCommand(IDM_ENCODING_DEFAULT);
}

void SciTEQt::CmdUtf16BigEndian()
{
    MenuCommand(IDM_ENCODING_UCS2BE);
}

void SciTEQt::CmdUtf16LittleEndian()
{
    MenuCommand(IDM_ENCODING_UCS2LE);
}

void SciTEQt::CmdUtf8WithBOM()
{
    MenuCommand(IDM_ENCODING_UTF8);
}

void SciTEQt::CmdUtf8()
{
    MenuCommand(IDM_ENCODING_UCOOKIE);
}

void SciTEQt::CmdAsHtml()
{
    MenuCommand(IDM_SAVEASHTML);
}

void SciTEQt::CmdAsRtf()
{
    MenuCommand(IDM_SAVEASRTF);
}

void SciTEQt::CmdAsPdf()
{
    MenuCommand(IDM_SAVEASPDF);
}

void SciTEQt::CmdAsLatex()
{
    MenuCommand(IDM_SAVEASTEX);
}

void SciTEQt::CmdAsXml()
{
    MenuCommand(IDM_SAVEASXML);
}

void SciTEQt::CmdPageSetup()
{
// TODO implement ...
    MenuCommand(IDM_PRINTSETUP);
}

void SciTEQt::CmdPrint()
{
// TODO implement ...
    MenuCommand(IDM_PRINT);
}

void SciTEQt::CmdLoadSession()
{
// TODO implement ...
    MenuCommand(IDM_LOADSESSION);
}

void SciTEQt::CmdSaveSession()
{
// TODO implement ...
    MenuCommand(IDM_SAVESESSION);
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

void SciTEQt::CmdDuplicate()
{
    MenuCommand(IDM_DUPLICATE);
}

void SciTEQt::CmdDelete()
{
    MenuCommand(IDM_CLEAR);
}

void SciTEQt::CmdSelectAll()
{
    MenuCommand(IDM_SELECTALL);
}

void SciTEQt::CmdCopyAsRtf()
{
    MenuCommand(IDM_COPYASRTF);
}

void SciTEQt::CmdMatchBrace()
{
    MenuCommand(IDM_MATCHBRACE);
}

void SciTEQt::CmdSelectToBrace()
{
    MenuCommand(IDM_SELECTTOBRACE);
}

void SciTEQt::CmdShowCalltip()
{
    MenuCommand(IDM_SHOWCALLTIP);
}

void SciTEQt::CmdCompleteSymbol()
{
    MenuCommand(IDM_COMPLETE);
}

void SciTEQt::CmdCompleteWord()
{
    MenuCommand(IDM_COMPLETEWORD);
}

void SciTEQt::CmdExpandAbbreviation()
{
    MenuCommand(IDM_ABBREV);
}

void SciTEQt::CmdInsertAbbreviation()
{
    MenuCommand(IDM_INS_ABBREV);
}

void SciTEQt::CmdBlockComment()
{
    MenuCommand(IDM_BLOCK_COMMENT);
}

void SciTEQt::CmdBoxComment()
{
    MenuCommand(IDM_BOX_COMMENT);
}

void SciTEQt::CmdStreamComment()
{
    MenuCommand(IDM_STREAM_COMMENT);
}

void SciTEQt::CmdMakeSelectionUppercase()
{
    MenuCommand(IDM_UPRCASE);
}

void SciTEQt::CmdMakeSelectionLowercase()
{
    MenuCommand(IDM_LWRCASE);
}

void SciTEQt::CmdReverseSelectedLines()
{
    MenuCommand(IDM_LINEREVERSE);
}

void SciTEQt::CmdJoin()
{
    MenuCommand(IDM_JOIN);
}

void SciTEQt::CmdSplit()
{
    MenuCommand(IDM_SPLIT);
}

void SciTEQt::CmdFind()
{
// TODO implement...
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

void SciTEQt::CmdFindInFiles()
{
// TODO implement...
    MenuCommand(IDM_FINDINFILES);
}

void SciTEQt::CmdReplace()
{
// TODO implement...
    MenuCommand(IDM_REPLACE);
}

void SciTEQt::CmdIncrementalSearch()
{
// TODO implement...
    MenuCommand(IDM_INCSEARCH);
}

void SciTEQt::CmdSelectionAddNext()
{
// TODO implement...    ???
    MenuCommand(IDM_SELECTIONADDNEXT);
}

void SciTEQt::CmdSelectionAddEach()
{
// TODO implement...    ???
    MenuCommand(IDM_SELECTIONADDEACH);
}

void SciTEQt::CmdGoto()
{
// TODO implement...
    MenuCommand(IDM_GOTO);
}

void SciTEQt::CmdNextBookmark()
{
    MenuCommand(IDM_BOOKMARK_NEXT);
}

void SciTEQt::CmdPreviousBookmark()
{
    MenuCommand(IDM_BOOKMARK_PREV);
}

void SciTEQt::CmdToggleBookmark()
{
    MenuCommand(IDM_BOOKMARK_TOGGLE);
}

void SciTEQt::CmdClearAllBookmarks()
{
    MenuCommand(IDM_BOOKMARK_CLEARALL);
}

void SciTEQt::CmdSelectAllBookmarks()
{
    MenuCommand(IDM_BOOKMARK_SELECT_ALL);
}

void SciTEQt::CmdToggleCurrentFold()
{
    MenuCommand(IDM_EXPAND);
}

void SciTEQt::CmdToggleAllFolds()
{
    MenuCommand(IDM_TOGGLE_FOLDALL);
}

void SciTEQt::CmdFullScreen()
{
    MenuCommand(IDM_FULLSCREEN);
}

void SciTEQt::CmdShowToolBar()
{
    MenuCommand(IDM_VIEWTOOLBAR);
}

void SciTEQt::CmdShowTabBar()
{
    MenuCommand(IDM_VIEWTABBAR);
}

void SciTEQt::CmdShowStatusBar()
{
    MenuCommand(IDM_VIEWSTATUSBAR);
}

void SciTEQt::CmdShowWhitespace()
{
    MenuCommand(IDM_VIEWSPACE);
}

void SciTEQt::CmdShowEndOfLine()
{
    MenuCommand(IDM_VIEWEOL);
}

void SciTEQt::CmdIndentionGuides()
{
    MenuCommand(IDM_VIEWGUIDES);
}

void SciTEQt::CmdLineNumbers()
{
    MenuCommand(IDM_LINENUMBERMARGIN);
}

void SciTEQt::CmdMargin()
{
    MenuCommand(IDM_SELMARGIN);
}

void SciTEQt::CmdFoldMargin()
{
    MenuCommand(IDM_FOLDMARGIN);
}

void SciTEQt::CmdToggleOutput()
{
// TODO implement !
    MenuCommand(IDM_TOGGLEOUTPUT);
}

void SciTEQt::CmdParameters()
{
// TODO implement !
    MenuCommand(IDM_TOGGLEPARAMETERS);
}

void SciTEQt::CmdCompile()
{
    MenuCommand(IDM_COMPILE);
}

void SciTEQt::CmdBuild()
{
    MenuCommand(IDM_BUILD);
}

void SciTEQt::CmdClean()
{
    MenuCommand(IDM_CLEAN);
}

void SciTEQt::CmdGo()
{
// TODO implement !
    MenuCommand(IDM_GO);
}

void SciTEQt::CmdStopExecuting()
{
    MenuCommand(IDM_STOPEXECUTE);
}

void SciTEQt::CmdNextMessage()
{
    MenuCommand(IDM_NEXTMSG);
}

void SciTEQt::CmdPreviousMessage()
{
    MenuCommand(IDM_PREVMSG);
}

void SciTEQt::CmdClearOutput()
{
    MenuCommand(IDM_CLEAROUTPUT);
}

void SciTEQt::CmdSwitchPane()
{
    MenuCommand(IDM_SWITCHPANE);
}

void SciTEQt::CmdAlwaysOnTop()
{
    MenuCommand(IDM_ONTOP);
}

void SciTEQt::CmdOpenFilesHere()
{
    MenuCommand(IDM_OPENFILESHERE);
}

void SciTEQt::CmdReadOnly()
{
    MenuCommand(IDM_READONLY);
}

void SciTEQt::CmdCrLf()
{
    MenuCommand(IDM_EOL_CRLF);
}

void SciTEQt::CmdCr()
{
    MenuCommand(IDM_EOL_CR);
}

void SciTEQt::CmdLf()
{
    MenuCommand(IDM_EOL_LF);
}

void SciTEQt::CmdConvertLineEndChar()
{
    MenuCommand(IDM_EOL_CONVERT);
}

void SciTEQt::CmdChangeIndentationSettings()
{
// TODO implement ...
    MenuCommand(IDM_TABSIZE);
}

void SciTEQt::CmdOpenLocalOptionsFile()
{
    MenuCommand(IDM_OPENLOCALPROPERTIES);
}

void SciTEQt::CmdOpenDirectoryOptionsFile()
{
    MenuCommand(IDM_OPENDIRECTORYPROPERTIES);
}

void SciTEQt::CmdOpenUserOptionsFile()
{
    MenuCommand(IDM_OPENUSERPROPERTIES);
}

void SciTEQt::CmdOpenGlobalOptionsFile()
{
    MenuCommand(IDM_OPENGLOBALPROPERTIES);
}

void SciTEQt::CmdOpenAbbreviationsFile()
{
    MenuCommand(IDM_OPENABBREVPROPERTIES);
}

void SciTEQt::CmdOpenLuaStartupScript()
{
    MenuCommand(IDM_OPENLUAEXTERNALFILE);
}

void SciTEQt::CmdWrap()
{
    MenuCommand(IDM_WRAP);
}

void SciTEQt::CmdWrapOutput()
{
    MenuCommand(IDM_WRAPOUTPUT);
}

void SciTEQt::CmdVerticalSplit()
{
// TODO implement !
    MenuCommand(IDM_SPLITVERTICAL);
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

void SciTEQt::CheckMenus()
{
    SciTEBase::CheckMenus();

    emit updateEolMenus((int)wEditor.EOLMode());

    emit updateEncodingMenus((int)CurrentBuffer()->unicodeMode);
}

void SciTEQt::Execute()
{
    if (buffers.SavingInBackground())
        // May be saving file that should be used by command so wait until all saved
        return;

    SciTEBase::Execute();

    if (!jobQueue.HasCommandToRun())
        // No commands to execute - possibly cancelled in SciTEBase::Execute
        return;

    // TODO: implement for Qt --> use visiscript executer ?

    // see also: SciTEWin::Execute() and SciTEGTK::Execute()
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
    qDebug() << "OnNotifiedFromOutput " << scn.message << endl;
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
