#include "sciteqt.h"

#include <QUrl>
#include <QDir>
#include <QResource>

#include <QDebug>

#include "ScintillaEditBase.h"

SciTEQt::SciTEQt(QObject *parent) : QObject(parent)
{
    CreateBuffers();

    ReadEnvironment();

    ReadGlobalPropFile();
    SetPropertiesInitial();
    ReadAbbrevPropFile();

//    wEditor.SetId();
//    wEditor.SetFnPtr();
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
    return false;
}

bool SciTEQt::SaveAsDialog()
{
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

void SciTEQt::CmdSave()
{
    MenuCommand(IDM_SAVE);
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
    int count = aFile.size();
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
