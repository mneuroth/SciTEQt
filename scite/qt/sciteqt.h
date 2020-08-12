#ifndef SCITEQT_H
#define SCITEQT_H

#include <QObject>

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cmath>

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "Scintilla.h"
#include "SciLexer.h"

#include "GUI.h"
#include "ScintillaWindow.h"
#include "StringList.h"
#include "StringHelpers.h"
#include "FilePath.h"
#include "StyleDefinition.h"
#include "PropSetFile.h"
#include "StyleWriter.h"
#include "Extender.h"
#include "SciTE.h"
#include "JobQueue.h"

#include "Cookie.h"
#include "Worker.h"
#include "FileWorker.h"
#include "MatchMarker.h"
#include "EditorConfig.h"

#ifndef NO_EXTENSIONS
#include "MultiplexExtension.h"

//#ifndef NO_FILER
//#include "DirectorExtension.h"
//#endif

#ifndef NO_LUA
#include "LuaExtension.h"
#endif
#endif

#include "Scintilla.h"

#include "SciTEBase.h"

#include "applicationdata.h"

#include <QEvent>
#include <QQmlApplicationEngine>
#include <QStandardItemModel>

#define POST_TO_MAIN (QEvent::User + 1)

class QSciTEQtEvent : public QEvent
{
public:
    QSciTEQtEvent(int cmd, Worker *pWorker)
        : QEvent((QEvent::Type)POST_TO_MAIN),
          m_cmd(cmd), m_pWorker(pWorker)
    {}

    int GetCmd() const { return m_cmd; }
    Worker * GetWorker() const { return m_pWorker; }

private:
    int         m_cmd;
    Worker *    m_pWorker;
};
/*
class DynamicMenuModel : public QStandardItemModel
{
public:
    DynamicMenuModel();

    virtual QHash<int,QByteArray> roleNames() const override;

private:
    QHash<int,QByteArray> m_aRoles;
};
*/
class SciTEQt : public QObject, public SciTEBase
{
    Q_OBJECT

    // control visibility of ui controls
    Q_PROPERTY(bool showToolBar READ isShowToolBar WRITE setShowToolBar NOTIFY showToolBarChanged)
    Q_PROPERTY(bool showStatusBar READ isShowStatusBar WRITE setShowStatusBar NOTIFY showStatusBarChanged)
    Q_PROPERTY(bool showTabBar READ isShowTabBar WRITE setShowTabBar NOTIFY showTabBarChanged)

    Q_PROPERTY(bool wholeWord READ isWholeWord WRITE setWholeWord NOTIFY wholeWordChanged)
    Q_PROPERTY(bool caseSensitive READ isCaseSensitive WRITE setCaseSensitive NOTIFY caseSensitiveChanged)
    Q_PROPERTY(bool regularExpression READ isRegularExpression WRITE setRegularExpression NOTIFY regularExpressionChanged)
    Q_PROPERTY(bool transformBackslash READ isTransformBackslash WRITE setTransformBackslash NOTIFY transformBackslashChanged)
    Q_PROPERTY(bool wrapAround READ isWrapAround WRITE setWrapAround NOTIFY wrapAroundChanged)
    Q_PROPERTY(bool searchUp READ isSearchUp WRITE setSearchUp NOTIFY searchUpChanged)

    // control content of ui controls
    Q_PROPERTY(QString statusBarText READ getStatusBarText WRITE setStatusBarText NOTIFY statusBarTextChanged)

public:
    explicit SciTEQt(QObject *parent=nullptr, QQmlApplicationEngine * pEngine=nullptr);

    virtual void TabInsert(int index, const GUI::gui_char *title) override;
    virtual void TabSelect(int index) override;
    virtual void RemoveAllTabs() override;

    virtual void WarnUser(int warnID) override;

    virtual void GetWindowPosition(int *left, int *top, int *width, int *height, int *maximize) override;
    virtual bool OpenDialog(const FilePath &directory, const GUI::gui_char *filesFilter) override;
    virtual bool SaveAsDialog() override;
    virtual void SaveACopy() override;
    virtual void SaveAsRTF() override;
    virtual void SaveAsPDF() override;
    virtual void SaveAsTEX() override;
    virtual void SaveAsXML() override;
    virtual void SaveAsHTML() override;
    virtual FilePath GetDefaultDirectory() override;
    virtual FilePath GetSciteDefaultHome() override;
    virtual FilePath GetSciteUserHome() override;
    virtual void Find() override;
    virtual MessageBoxChoice WindowMessageBox(GUI::Window &w, const GUI::gui_string &msg, MessageBoxStyle style = mbsIconWarning) override;
    virtual void FindMessageBox(const std::string &msg, const std::string *findItem = nullptr) override;
    virtual void FindIncrement() override;
    virtual void FindInFiles() override;
    virtual void Replace() override;
    virtual void DestroyFindReplace() override;
    virtual void GoLineDialog() override;
    virtual bool AbbrevDialog() override;
    virtual void TabSizeDialog() override;
    virtual bool ParametersOpen() override;
    virtual void ParamGrab() override;
    virtual bool ParametersDialog(bool modal) override;
    virtual void FindReplace(bool replace) override;
    virtual void StopExecute() override;
    virtual void SetFileProperties(PropSetFile &ps) override;
    virtual void AboutDialog() override;
    virtual void QuitProgram() override;
    virtual void CopyPath() override;
    virtual void SetStatusBarText(const char *s) override;
    virtual void ShowToolBar() override;
    virtual void ShowTabBar() override;
    virtual void ShowStatusBar() override;
    virtual void ActivateWindow(const char *timestamp) override;
    virtual void SizeContentWindows() override;
    virtual void SizeSubWindows() override;
    virtual void SetMenuItem(int menuNumber, int position, int itemID,
                 const GUI::gui_char *text, const GUI::gui_char *mnemonic = nullptr) override;
    virtual void DestroyMenuItem(int menuNumber, int itemID) override;
    virtual void CheckAMenuItem(int wIDCheckItem, bool val) override;
    virtual void EnableAMenuItem(int wIDCheckItem, bool val) override;
    virtual void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;

    virtual void PostOnMainThread(int cmd, Worker *pWorker) override;
    virtual void ReadEmbeddedProperties() override;

    virtual void CheckMenus() override;
    virtual void Execute() override;

    virtual bool event(QEvent *e) override;

    bool isShowToolBar() const;
    void setShowToolBar(bool val);
    bool isShowStatusBar() const;
    void setShowStatusBar(bool val);
    bool isShowTabBar() const;
    void setShowTabBar(bool val);

    bool isWholeWord() const;
    void setWholeWord(bool val);
    bool isCaseSensitive() const;
    void setCaseSensitive(bool val);
    bool isRegularExpression() const;
    void setRegularExpression(bool val);
    bool isTransformBackslash() const;
    void setTransformBackslash(bool val);
    bool isWrapAround() const;
    void setWrapAround(bool val);
    bool isSearchUp() const;
    void setSearchUp(bool val);

    QString getStatusBarText() const;
    void setStatusBarText(const QString & txt);

    Q_INVOKABLE bool doOpen(const QString & sFileName);
    Q_INVOKABLE void setScintilla(QObject * obj);
    Q_INVOKABLE void setOutput(QObject * obj);
    Q_INVOKABLE void setContent(QObject * obj);
    Q_INVOKABLE void setMainWindow(QObject * obj);

    Q_INVOKABLE QString getLocalisedText(const QString & textInput);

    Q_INVOKABLE bool saveCurrentAs(const QString & sFileName);

    // menu commands
    Q_INVOKABLE void cmdNew();
    Q_INVOKABLE void cmdOpen();
    Q_INVOKABLE void cmdOpenSelectedFileName();
    Q_INVOKABLE void cmdRevert();
    Q_INVOKABLE void cmdClose();
    Q_INVOKABLE void cmdSave();
    Q_INVOKABLE void cmdSaveAs();
    Q_INVOKABLE void cmdCopyPath();
    Q_INVOKABLE void cmdCodePageProperty();
    Q_INVOKABLE void cmdUtf16BigEndian();
    Q_INVOKABLE void cmdUtf16LittleEndian();
    Q_INVOKABLE void cmdUtf8WithBOM();
    Q_INVOKABLE void cmdUtf8();
    Q_INVOKABLE void cmdAsHtml();
    Q_INVOKABLE void cmdAsRtf();
    Q_INVOKABLE void cmdAsPdf();
    Q_INVOKABLE void cmdAsLatex();
    Q_INVOKABLE void cmdAsXml();
    Q_INVOKABLE void cmdPageSetup();
    Q_INVOKABLE void cmdPrint();
    Q_INVOKABLE void cmdLoadSession();
    Q_INVOKABLE void cmdSaveSession();
    Q_INVOKABLE void cmdExit();
    Q_INVOKABLE void cmdUndo();
    Q_INVOKABLE void cmdRedo();
    Q_INVOKABLE void cmdCut();
    Q_INVOKABLE void cmdCopy();
    Q_INVOKABLE void cmdPaste();
    Q_INVOKABLE void cmdDuplicate();
    Q_INVOKABLE void cmdDelete();
    Q_INVOKABLE void cmdSelectAll();
    Q_INVOKABLE void cmdCopyAsRtf();
    Q_INVOKABLE void cmdMatchBrace();
    Q_INVOKABLE void cmdSelectToBrace();
    Q_INVOKABLE void cmdShowCalltip();
    Q_INVOKABLE void cmdCompleteSymbol();
    Q_INVOKABLE void cmdCompleteWord();
    Q_INVOKABLE void cmdExpandAbbreviation();
    Q_INVOKABLE void cmdInsertAbbreviation();
    Q_INVOKABLE void cmdBlockComment();
    Q_INVOKABLE void cmdBoxComment();
    Q_INVOKABLE void cmdStreamComment();
    Q_INVOKABLE void cmdMakeSelectionUppercase();
    Q_INVOKABLE void cmdMakeSelectionLowercase();
    Q_INVOKABLE void cmdReverseSelectedLines();
    Q_INVOKABLE void cmdJoin();
    Q_INVOKABLE void cmdSplit();
    Q_INVOKABLE void cmdFind();
    Q_INVOKABLE void cmdFindNext();
    Q_INVOKABLE void cmdFindPrevious();
    Q_INVOKABLE void cmdFindInFiles();
    Q_INVOKABLE void cmdReplace();
    Q_INVOKABLE void cmdIncrementalSearch();
    Q_INVOKABLE void cmdSelectionAddNext();
    Q_INVOKABLE void cmdSelectionAddEach();
    Q_INVOKABLE void cmdGoto();
    Q_INVOKABLE void cmdNextBookmark();
    Q_INVOKABLE void cmdPreviousBookmark();
    Q_INVOKABLE void cmdToggleBookmark();
    Q_INVOKABLE void cmdClearAllBookmarks();
    Q_INVOKABLE void cmdSelectAllBookmarks();
    Q_INVOKABLE void cmdToggleCurrentFold();
    Q_INVOKABLE void cmdToggleAllFolds();
    Q_INVOKABLE void cmdFullScreen();
    Q_INVOKABLE void cmdShowToolBar();
    Q_INVOKABLE void cmdShowTabBar();
    Q_INVOKABLE void cmdShowStatusBar();
    Q_INVOKABLE void cmdShowWhitespace();
    Q_INVOKABLE void cmdIndentionGuides();
    Q_INVOKABLE void cmdShowEndOfLine();
    Q_INVOKABLE void cmdLineNumbers();
    Q_INVOKABLE void cmdMargin();
    Q_INVOKABLE void cmdFoldMargin();
    Q_INVOKABLE void cmdToggleOutput();
    Q_INVOKABLE void cmdParameters();
    Q_INVOKABLE void cmdCompile();
    Q_INVOKABLE void cmdBuild();
    Q_INVOKABLE void cmdClean();
    Q_INVOKABLE void cmdGo();
    Q_INVOKABLE void cmdStopExecuting();
    Q_INVOKABLE void cmdNextMessage();
    Q_INVOKABLE void cmdPreviousMessage();
    Q_INVOKABLE void cmdClearOutput();
    Q_INVOKABLE void cmdSwitchPane();
    Q_INVOKABLE void cmdAlwaysOnTop();
    Q_INVOKABLE void cmdOpenFilesHere();
    Q_INVOKABLE void cmdVerticalSplit();
    Q_INVOKABLE void cmdWrap();
    Q_INVOKABLE void cmdWrapOutput();
    Q_INVOKABLE void cmdReadOnly();
    Q_INVOKABLE void cmdCrLf();
    Q_INVOKABLE void cmdCr();
    Q_INVOKABLE void cmdLf();
    Q_INVOKABLE void cmdConvertLineEndChar();
    Q_INVOKABLE void cmdChangeIndentationSettings();
    Q_INVOKABLE void cmdOpenLocalOptionsFile();
    Q_INVOKABLE void cmdOpenDirectoryOptionsFile();
    Q_INVOKABLE void cmdOpenUserOptionsFile();
    Q_INVOKABLE void cmdOpenGlobalOptionsFile();
    Q_INVOKABLE void cmdOpenAbbreviationsFile();
    Q_INVOKABLE void cmdOpenLuaStartupScript();
    Q_INVOKABLE void cmdUseMonospacedFont();
    Q_INVOKABLE void cmdBuffersPrevious();
    Q_INVOKABLE void cmdBuffersNext();
    Q_INVOKABLE void cmdBuffersCloseAll();
    Q_INVOKABLE void cmdBuffersSaveAll();
    Q_INVOKABLE void cmdSelectBuffer(int index);
    Q_INVOKABLE void cmdSelectLanguage(int index);
    Q_INVOKABLE void cmdHelp();
    Q_INVOKABLE void cmdSciteHelp();
    Q_INVOKABLE void cmdAboutScite();

    Q_INVOKABLE void cmdMarkAll();
    Q_INVOKABLE void cmdTriggerReplace(const QString & find, const QString & replace, bool inSection);
    Q_INVOKABLE void cmdGotoLine(int lineNo, int colPos);

    Q_INVOKABLE QVariant fillToLength(const QString & text, const QString & shortcut);
    Q_INVOKABLE QVariant fillToLengthWithFont(const QString & text, const QString & shortcut, const QFont & font);

    Q_INVOKABLE void setFindText(const QString & text, bool incremental);

    Q_INVOKABLE void onStatusbarClicked();

    Q_INVOKABLE void setApplicationData(ApplicationData * pApplicationData);

    Q_INVOKABLE void setSpliterPos(int currentPosX, int currentPosY);
    Q_INVOKABLE void startDragSpliterPos(int currentPosX, int currentPosY);

    Q_INVOKABLE bool isMobilePlatform() const;

    void UpdateStatusbarView();

public slots:
    void OnAcceptedClicked();
    void OnRejectedClicked();
    void OnYesClicked();
    void OnNoClicked();

    void OnNotifiedFromScintilla(SCNotification scn);
    void OnNotifiedFromOutput(SCNotification scn);

signals:
    void showToolBarChanged();
    void showTabBarChanged();
    void showStatusBarChanged();
    void statusBarTextChanged();
    void setMenuChecked(int menuID, bool val);
    void setMenuEnable(int menuID, bool val);

    void wholeWordChanged();
    void caseSensitiveChanged();
    void regularExpressionChanged();
    void transformBackslashChanged();
    void wrapAroundChanged();
    void searchUpChanged();

    void setInBuffersModel(int index, const QString & txt, bool checked);
    void removeInBuffersModel(int index);
    void checkStateInBuffersModel(int index, bool checked);
    void setInLanguagesModel(int index, const QString & txt, bool checked);
    void removeInLanguagesModel(int index);
    void checkStateInLanguagesModel(int index, bool checked);

    void startFileDialog(const QString & sDirectory, const QString & sFilter, bool bAsOpenDialog = true);
    void showInfoDialog(const QString & sInfoText, int style);

    void showFindInFilesDialog(const QString & text);
    void showFind(const QString & text, bool incremental, bool withReplace);
    void showGoToDialog(int currentLine, int currentColumn, int maxLine);

    void setVerticalSplit(bool verticalSplit);
    void setOutputHeight(int heightOutput);

    void updateEolMenus(int enumEol);
    void updateEncodingMenus(int enumEncoding);

    void insertTab(int index, const QString & title);
    void selectTab(int index);
    void removeAllTabs();

private:
    QObject * getCurrentInfoDialog();

    ApplicationData *       m_pApplicationData;     // not an owner !
    QQmlApplicationEngine * m_pEngine;

    bool                    m_bWaitDoneFlag;
    int                     m_iMessageDialogAccepted;

    bool                    m_bShowToolBar;
    bool                    m_bShowStatusBar;
    bool                    m_bShowTabBar;
    QString                 m_sStatusBarText;
};

#define MSGBOX_RESULT_CANCEL 0
#define MSGBOX_RESULT_OK 1
#define MSGBOX_RESULT_NO 2
#define MSGBOX_RESULT_YES 3

QString ConvertGuiCharToQString(const GUI::gui_char * s);
QString ConvertGuiStringToQString(const GUI::gui_string & s);

#endif // SCITEQT_H
