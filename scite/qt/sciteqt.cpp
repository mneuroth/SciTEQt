#include "sciteqt.h"

#include <QUrl>
#include <QDir>
#include <QResource>
#include <QThread>
#include <QGuiApplication>
#include <QClipboard>

#include <QDebug>

#include "ScintillaEditBase.h"
#include "ScintillaQt.h"

SciTEQt::SciTEQt(QObject *parent)
    : QObject(parent),
      m_pApplicationData(0),
      m_bWaitDoneFlag(false),
      m_iMessageDialogAccepted(MSGBOX_RESULT_CANCEL)
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
    if( m_pApplicationData != 0 )
    {
        m_pApplicationData->startFileDialog(directory.AbsolutePath().AsUTF8().c_str(), QString::fromWCharArray(filesFilter), true);
        return true;
    }

    return false;
}

bool SciTEQt::SaveAsDialog()
{
    if( m_pApplicationData != 0 )
    {
        m_pApplicationData->startFileDialog("", "", false);
        return true;
    }

    return false;
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
    //qDebug() << "MessageBox " << msg << " style=" << style << endl;
    if( m_pApplicationData != 0 )
    {
        QObject * pMessageBox = m_pApplicationData->showInfoDialog(QString::fromStdWString(msg), style);
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
    }

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
    pClipboard->setText(QString::fromWCharArray(clipText.c_str()));
}

void SciTEQt::SetStatusBarText(const char *s)
{
    if(m_pApplicationData!=0)
    {
        m_pApplicationData->setStatusBarText(s);
    }
}

void SciTEQt::ShowToolBar()
{
    if(m_pApplicationData!=0)
    {
        m_pApplicationData->setShowToolBar(tbVisible);
    }
}

void SciTEQt::ShowTabBar()
{
}

void SciTEQt::ShowStatusBar()
{
    if(m_pApplicationData!=0)
    {
        m_pApplicationData->setShowStatusBar(sbVisible);
    }
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
    qDebug() << "Set Menu Item " << menuNumber << " pos=" << position << " " << itemID << " " << QString::fromWCharArray(text) << " " << QString::fromWCharArray(mnemonic) << endl;
}

void SciTEQt::DestroyMenuItem(int menuNumber, int itemID)
{
    qDebug() << "DestroyMenuItem" << menuNumber << " " << itemID << endl;
}

void SciTEQt::CheckAMenuItem(int wIDCheckItem, bool val)
{
    qDebug() << "CheckAMenuItem" << wIDCheckItem << " " << val << endl;
}

void SciTEQt::EnableAMenuItem(int wIDCheckItem, bool val)
{
    qDebug() << "EnableAMenuItem" << wIDCheckItem << " " << val << endl;
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
    return QString::fromStdWString(localisedText);
}

bool SciTEQt::saveCurrentAs(const QString & sFileName)
{
    bool ret = false;
    QUrl aUrl(sFileName);
    QString sLocalFileName = aUrl.toLocalFile();
    wchar_t * buf = new wchar_t[sLocalFileName.length()+1];
    sLocalFileName.toWCharArray(buf);
    buf[sLocalFileName.length()] = 0;
    FilePath path(buf);
    if(path.IsSet())
    {
        ret = SaveIfNotOpen(path, false);
    }
    delete [] buf;
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
    MenuCommand(IDM_VIEWSTATUSBAR);
}

void SciTEQt::CmdLineNumbers()
{
    MenuCommand(IDM_LINENUMBERMARGIN);
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

void SciTEQt::setApplicationData(ApplicationData * pApplicationData)
{
    m_pApplicationData = pApplicationData;
    if(m_pApplicationData!=0)
    {
        m_pApplicationData->setSciteQt(this);
    }
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
