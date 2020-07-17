#include "sciteqt.h"

#include <QUrl>
#include <QDir>
#include <QResource>

#include <QDebug>

#include "ScintillaEditBase.h"

SciTEQt::SciTEQt(QObject *parent)
    : QObject(parent),
      m_pApplicationData(0)
{
    CreateBuffers();

    ReadEnvironment();

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

    ReadGlobalPropFile();
    SetPropertiesInitial();
    ReadAbbrevPropFile();

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
    qDebug() << "NOT IMPLEMENTED: WindowMessageBox " << msg << endl;
    return (SciTEQt::MessageBoxChoice)0;
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

}

void SciTEQt::QuitProgram()
{

}

void SciTEQt::SetStatusBarText(const char *s)
{

}

void SciTEQt::ShowToolBar()
{

}

void SciTEQt::ShowTabBar()
{

}

void SciTEQt::ShowStatusBar()
{

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

}

void SciTEQt::DestroyMenuItem(int menuNumber, int itemID)
{

}

void SciTEQt::CheckAMenuItem(int wIDCheckItem, bool val)
{

}

void SciTEQt::EnableAMenuItem(int wIDCheckItem, bool val)
{

}

void SciTEQt::AddToPopUp(const char *label, int cmd, bool enabled)
{

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
// TODO
    ScintillaEditBase * base = reinterpret_cast<ScintillaEditBase *>(obj);

    SciFnDirect fn_ = reinterpret_cast<SciFnDirect>(base->send(SCI_GETDIRECTFUNCTION, 0, 0));
    const sptr_t ptr_ = base->send(SCI_GETDIRECTPOINTER, 0, 0);
    wEditor.SetFnPtr(fn_, ptr_);
    wEditor.SetID(base->sqt);
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

void SciTEQt::CmdLineNumbers()
{
    MenuCommand(IDM_LINENUMBERMARGIN);
}

void SciTEQt::CmdUseMonospacedFont()
{
    MenuCommand(IDM_MONOFONT);
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
    qDebug() << "EVENT " << e->type() << endl;
    if(e->type() == POST_TO_MAIN)
    {
        QSciTEQtEvent * pSciteEvent = (QSciTEQtEvent *)e;
        WorkerCommand(pSciteEvent->GetCmd(), pSciteEvent->GetWorker());
    }
    else
    {
        qDebug() << "WARNING: EVENT not handled !!! type=" << e->type() << endl;
    }
    return true;
}

void SciTEQt::setApplicationData(ApplicationData * pApplicationData)
{
    m_pApplicationData = pApplicationData;
}
