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

#include <qdesktopservices.h>

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
      m_bFileDialogWaitDoneFlag(false),
      m_iFileDialogMessageDialogAccepted(MSGBOX_RESULT_CANCEL),
      m_bShowToolBar(false),
      m_bShowStatusBar(false),
      m_bShowTabBar(true),
      icmd(0),
      m_left(0),
      m_top(0),
      m_width(0),
      m_height(0),
      m_maximize(false)
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
    emit insertTab(index, ConvertGuiCharToQString(title));
}

void SciTEQt::TabSelect(int index)
{
    emit selectTab(index);
}

void SciTEQt::RemoveAllTabs()
{
    emit removeAllTabs();
}

void SciTEQt::WarnUser(int warnID)
{
    // TODO implement !
    emit showInfoDialog(QString("Sorry: WarnUser() is not implemented yet! warnID=%1").arg(warnID), 0);
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
    int count = sLocalFileName.toWCharArray((wchar_t *)buf);
    buf[count] = 0;
    return FilePath(buf);
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
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }

    disconnect(pFileDialog,SIGNAL(accepted()),this,SLOT(OnFileDialogAcceptedClicked()));
    disconnect(pFileDialog,SIGNAL(rejected()),this,SLOT(OnFileDialogRejectedClicked()));

    return m_iFileDialogMessageDialogAccepted == MSGBOX_RESULT_OK;
}

bool SciTEQt::OpenDialog(const FilePath &directory, const GUI::gui_char *filesFilter)
{
    QString s = ConvertGuiCharToQString(filesFilter);
    if (s.startsWith("All Source|"))
    {
        s = s.remove(0, 11);
    }
    s = "*";

    emit startFileDialog(directory.AbsolutePath().AsUTF8().c_str(), s/*ConvertGuiStringToQString(openFilter)*/, "Open File", true);

    if(ProcessCurrentFileDialog())
    {
        return Open(GetPathFromUrl(m_sCurrentFileUrl));
    }

    return false;
}

bool SciTEQt::SaveAsDialog()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "", "Save File", false);

    if(ProcessCurrentFileDialog())
    {
        return SaveIfNotOpen(GetPathFromUrl(m_sCurrentFileUrl), false);
    }

    return false;
}

void SciTEQt::SaveACopy()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "", "Save a Copy", false);

    if(ProcessCurrentFileDialog())
    {
        SaveBuffer(GetPathFromUrl(m_sCurrentFileUrl), sfNone);
    }
}

void SciTEQt::SaveAsRTF()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.rtf", "Export File As RTF", false);

    if(ProcessCurrentFileDialog())
    {
        SaveToRTF(GetPathFromUrl(m_sCurrentFileUrl));
    }
}

void SciTEQt::SaveAsPDF()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.pdf", "Export File As PDF", false);

    if(ProcessCurrentFileDialog())
    {
        SaveToPDF(GetPathFromUrl(m_sCurrentFileUrl));
    }
}

void SciTEQt::SaveAsTEX()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.tex", "Export File As LaTeX", false);

    if(ProcessCurrentFileDialog())
    {
        SaveToTEX(GetPathFromUrl(m_sCurrentFileUrl));
    }
}

void SciTEQt::SaveAsXML()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.xml", "Export File As XML", false);

    if(ProcessCurrentFileDialog())
    {
        SaveToXML(GetPathFromUrl(m_sCurrentFileUrl));
    }
}

void SciTEQt::SaveAsHTML()
{
    emit startFileDialog(QString::fromStdString(filePath.Directory().AsUTF8()), "*.html", "Export File As HTML", false);

    if(ProcessCurrentFileDialog())
    {
        SaveToHTML(GetPathFromUrl(m_sCurrentFileUrl));
    }
}

FilePath GetSciTEPath(const QByteArray & home)
{
    GUI::gui_char buf[512];
    if(!home.isEmpty())
    {
        int count = QString::fromLocal8Bit(home).toWCharArray((wchar_t *)buf);
        buf[count] = 0;
    }
    else
    {
        // return the directory of the executable
        int count = QCoreApplication::applicationDirPath().toWCharArray((wchar_t *)buf);
        buf[count] = 0;
    }

    return FilePath(buf);
}

FilePath SciTEQt::GetDefaultDirectory()
{
    QByteArray home = qgetenv("SciTE_HOME");
    return GetSciTEPath(home);
}

FilePath SciTEQt::GetSciteDefaultHome()
{
    QByteArray home = qgetenv("SciTE_HOME");
    return GetSciTEPath(home);
}

FilePath SciTEQt::GetSciteUserHome()
{
    // First looking for environment variable $SciTE_USERHOME
    // to set SciteUserHome. If not present we look for $SciTE_HOME
    // then defaulting to $USERPROFILE
    QByteArray home = qgetenv("SciTE_USERHOME");
    if (home.isEmpty()) {
        home = qgetenv("SciTE_HOME");
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

void SciTEQt::Find()
{
    // TODO: SelectionIntoFind();
    // see: SciTEWin::Find()

    SelectionIntoFind();    // findWhat
    if (props.GetInt("find.use.strip")) {
//        replaceStrip.Close();
//        findStrip.SetIncrementalBehaviour(props.GetInt("find.strip.incremental"));
//        findStrip.Show(props.GetInt("strip.button.height", -1));
        emit showFind(QString::fromStdString(findWhat), false, false);
    } else {
//        if (findStrip.visible || replaceStrip.visible)
//            return;
        FindReplace(false);
    }
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
    GUI::gui_string msgBuf;

    if (!findItem) {
        msgBuf = LocaliseMessage(msg.c_str());
    } else {
        GUI::gui_string findThing = GUI::StringFromUTF8(*findItem);
        msgBuf = LocaliseMessage(msg.c_str(), findThing.c_str());
    }

    emit showInfoDialog(ConvertGuiStringToQString(msgBuf), 0);
}

void SciTEQt::FindIncrement()
{
    emit showFind("", true, false);
}

void SciTEQt::FindInFiles()
{
    SelectionIntoFind();    // findWhat

    emit showFindInFilesDialog(QString::fromStdString(findWhat));
}

void SciTEQt::Replace()
{
    SelectionIntoFind();    // findWhat

    emit showFind(QString::fromStdString(findWhat), false, true);
}

void SciTEQt::DestroyFindReplace()
{
    // TODO implement ! --> for FindReplace (advanced) --> not used ?
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
    // TODO implement !
    emit showInfoDialog("Sorry: AbbrevDialog() is not implemented yet!", 0);
    return false;
}

void SciTEQt::TabSizeDialog()
{
    // TODO implement !
    emit showInfoDialog("Sorry: TabSizeDialog() is not implemented yet!", 0);
}

bool SciTEQt::ParametersOpen()
{
    // TODO implement !
    //emit showInfoDialog("Sorry: ParametersOpen() is not implemented yet!", 0);
    return false;
}

void SciTEQt::ParamGrab()
{
    // TODO implement !
    emit showInfoDialog("Sorry: ParamGrab() is not implemented yet!", 0);
}

bool SciTEQt::ParametersDialog(bool modal)
{
    // TODO implement !
    emit showInfoDialog("Sorry: ParametersDialog() is not implemented yet!", 0);
    return false;
}

void SciTEQt::FindReplace(bool replace)
{
    // TODO implement ! --> find replace (advanced) --> not used ?
    emit showInfoDialog("Sorry: FindReplace() is not implemented yet!", 0);
}

void SciTEQt::StopExecute()
{
    // TODO implement !
    emit showInfoDialog("Sorry: StopExecute() is not implemented yet!", 0);
}

void SciTEQt::SetFileProperties(PropSetFile &ps)
{
    QString fileName = QString::fromStdString(filePath.AsUTF8());

    QFileInfo aFileInfo(fileName);

    QString temp;
    temp = aFileInfo.fileTime(QFileDevice::FileModificationTime).toString("hh:mm:ss");
    ps.Set("FileTime", temp.toStdString());
    temp = aFileInfo.fileTime(QFileDevice::FileModificationTime).toString("dd.MM.yyyy");
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
    setShowTabBar(tabVisible);
}

void SciTEQt::ShowStatusBar()
{
    setShowStatusBar(sbVisible);
}

void SciTEQt::ActivateWindow(const char *timestamp)
{
    // TODO implement !
    emit showInfoDialog("Sorry: ActivateWindow() is not implemented yet!", 0);
}

void SciTEQt::SizeContentWindows()
{
    qDebug() << "SizeContentWindows " << heightOutput << " " << splitVertical << endl;

    emit setVerticalSplit(splitVertical);
    emit setOutputHeight(heightOutput);
}

void SciTEQt::SizeSubWindows()
{
    qDebug() << "SizeSubWindows " << heightOutput << " " << splitVertical << endl;

    SizeContentWindows();
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
	else if (menuNumber == 4)
	{
        int posForThisItem = itemID - IDM_TOOLS;
		
        // TODO: analyze, msc debug has problems with this ?
        //QString sMn(ConvertGuiCharToQString(mnemonic));
        //QString sMn("Ctrl+0");
        emit setInToolsModel(posForThisItem, ConvertGuiCharToQString(text), false, ConvertGuiCharToQString(mnemonic)/*sMn/*"Ctrl+0"*/);
	}
	else
    {
        qDebug() << "==> UN_HANDLED: Set Menu Item " << menuNumber << " pos=" << position << " " << itemID << " " << (text != 0 ? ConvertGuiStringToQString (text) : "") << endl; //QString::fromWCharArray(text) << " " << QString::fromWCharArray(mnemonic) << endl;
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
        emit removeInLanguagesModel(posForThisItem);
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
    else
    {
//        qDebug() << "NOT HANDLED Destroy Menu Item " << menuNumber << " item=" << itemID << endl;
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
//    qDebug() << "EnableAMenuItem" << wIDCheckItem << " " << val << endl;
    emit setMenuEnable(wIDCheckItem, val);
}

void SciTEQt::AddToPopUp(const char *label, int cmd, bool enabled)
{
    // TODO implement !
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

void SciTEQt::setAboutScite(QObject * obj)
{
    ScintillaEditBase * base = reinterpret_cast<ScintillaEditBase *>(obj);

    SciFnDirect fn_ = reinterpret_cast<SciFnDirect>(base->send(SCI_GETDIRECTFUNCTION, 0, 0));
    const sptr_t ptr_ = base->send(SCI_GETDIRECTPOINTER, 0, 0);
    wAboutScite.SetFnPtr(fn_, ptr_);
    wAboutScite.SetID(base->sqt);
}

void SciTEQt::setMainWindow(QObject * obj)
{
//    qDebug() << "setMainWindow " << obj->objectName() << endl;
    wSciTE.SetID(obj);
}

void SciTEQt::setContent(QObject * obj)
{
//    qDebug() << "setContent " << obj->objectName() << endl;
    wContent.SetID(obj);
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
// TODO implement ...
    MenuCommand(IDM_PRINTSETUP);
}

void SciTEQt::cmdPrint()
{
// TODO implement ...
    MenuCommand(IDM_PRINT);
}

void SciTEQt::cmdLoadSession()
{
// TODO implement ...
    MenuCommand(IDM_LOADSESSION);
}

void SciTEQt::cmdSaveSession()
{
// TODO implement ...
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
    MenuCommand(IDM_FINDINFILES);
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
// TODO implement...    ???
    MenuCommand(IDM_SELECTIONADDNEXT);
}

void SciTEQt::cmdSelectionAddEach()
{
// TODO implement...    ???
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

void SciTEQt::cmdHelp()
{
    MenuCommand(IDM_HELP);
}

void SciTEQt::cmdSciteHelp()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile("SciTEDoc.html"));
    //MenuCommand(IDM_HELP_SCITE);
}

void SciTEQt::cmdAboutScite()
{
    MenuCommand(IDM_ABOUT);
}

QString simpleReadFileContent(const QString & fileName)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString(QObject::tr("Error reading ") + fileName);
    }

    QTextStream stream(&file);
    auto text = stream.readAll();

    file.close();

    return text;
}

void SciTEQt::cmdAboutSciteQt()
{
    New();
    QString aboutSciteQt = simpleReadFileContent(":/about_sciteqt.txt");
    emit setTextToCurrent(aboutSciteQt);
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

void SciTEQt::ResetExecution()
{
    icmd = 0;
    jobQueue.SetExecuting(false);
    if (needReadProperties)
        ReadProperties();
    CheckReload();
    CheckMenus();
    jobQueue.ClearJobs();
//    ::SendMessage(MainHWND(), WM_COMMAND, IDM_FINISHEDEXECUTE, 0);
}

void SciTEQt::ExecuteNext()
{
    icmd++;
    if (icmd < jobQueue.commandCurrent && icmd < jobQueue.commandMax /*&& cmdWorker.exitStatus == 0*/) {
        Execute();
    } else {
        ResetExecution();
    }
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

//    icmd = 0;
//    cmdWorker.Initialise(false);
//    cmdWorker.outputScroll = props.GetInt("output.scroll", 1);
//    cmdWorker.originalEnd = wOutput.Length();
//    cmdWorker.commandTime.Duration(true);
//    cmdWorker.flags = jobQueue.jobQueue[cmdWorker.icmd].flags;
    if (scrollOutput)
        wOutput.GotoPos(wOutput.Length());

    if (jobQueue.jobQueue[icmd].jobType == jobExtension) {
        // Execute extensions synchronously
        if (jobQueue.jobQueue[icmd].flags & jobGroupUndo)
            wEditor.BeginUndoAction();

        if (extender)
            extender->OnExecute(jobQueue.jobQueue[icmd].command.c_str());

        if (jobQueue.jobQueue[icmd].flags & jobGroupUndo)
            wEditor.EndUndoAction();

        ExecuteNext();
    } else {
        // Execute other jobs asynchronously on a new thread
//        PerformOnNewThread(&cmdWorker);
    }

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
    double lenShortcut = metrics.boundingRect(shortcut).width();
    double lenSpace = metrics.boundingRect("  ").width();
    double lenTab = metrics.boundingRect("\t").width();

    QString fill(" ");
    //fill = fill.leftJustified((180-lenText)/lenTab,'\t');
    fill = fill.leftJustified((180-lenText)/lenSpace,' ');

    //qDebug() << "WITH FONT x " << font << " " << lenText << " " << lenShortcut << " " << lenSpace << " " << lenTab << " --> " << text.length() << " " << fill.length() << " total=" << metrics.boundingRect(text + fill).width() << endl;
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

QObject * SciTEQt::getCurrentInfoDialog()
{
    QObject * infoDlg = childObject<QObject*>(*m_pEngine, "infoDialog", "", false);
    return infoDlg;
}

QObject * SciTEQt::getCurrentFileDialog()
{
    QObject * fileDlg = childObject<QObject*>(*m_pEngine, "fileDialog", "", false);
    return fileDlg;
}

void SciTEQt::setApplicationData(ApplicationData * pApplicationData)
{
    m_pApplicationData = pApplicationData;
    if(m_pApplicationData!=0)
    {
// TODO implement ! improve !!!
        extender = pApplicationData->GetExtension();

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

    // TODO --> nach setApplicationData --> besser applicationdata direkt beim konstruieren rein reichen... ??? Property binding?
        ProcessCommandLine(args, 1);

        CheckMenus();
        SizeSubWindows();
        //SetFocus(wEditor);

    //qDebug() << "SciteQt::SciteQt " << (void *)this << endl;
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

//    qDebug() << "----> OPEN" << endl;

    // open the untitled (empty) document at startup
//    Open(FilePath());
// TODO --> update ui components wie statusbar etc.

    // TODO: enable lua plugin...

    // if no real document (but only the default document) is loaded: show the default document for SciteQt
    QString currentFileName = QString::fromStdString(FileNameExt().Name().AsUTF8());
    if( buffers.length == 1 && currentFileName.length()==0 )
    {
        cmdAboutSciteQt();
    }
}

void SciTEQt::RestorePosition()
{
    const int left = propsSession.GetInt("position.left", 0);
    const int top = propsSession.GetInt("position.top", 0);
    const int width = propsSession.GetInt("position.width", 600);
    const int height = propsSession.GetInt("position.height", 800);
    bool maximize = propsSession.GetInt("position.maximize", 0)==1;

    emit setWindowPosAndSize(left, top, width, height, maximize);
}

void SciTEQt::setSpliterPos(int currentPosX, int currentPosY)
{
    GUI::Point pt(currentPosX, currentPosY);
qDebug() << "setSplitterPos " << currentPosX << " "<< currentPosY << " " << endl;
    MoveSplit(pt);
}

void SciTEQt::startDragSpliterPos(int currentPosX, int currentPosY)
{
    GUI::Point pt(currentPosX, currentPosY);
qDebug() << "startDragSplitterPos " << currentPosX << " "<< currentPosY << " " << endl;
    ptStartDrag = pt;
}

bool SciTEQt::isMobilePlatform() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
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
