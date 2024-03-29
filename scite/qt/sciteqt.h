/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/

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
#include <optional>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

#include "Geometry.h"

#include "ILoader.h"
#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "Platform.h"

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
#include "Searcher.h"

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
#include "findinfiles.h"
#include "scriptexecution.h"

#include <QEvent>
#include <QQmlApplicationEngine>
//#include <QStandardItemModel>
#include <QPrinter>
#include <QScreen>

class SciTEQt;
class SciteQtEnvironmentForJavaScript;

#define SCITE_HOME          "SciTE_HOME"
#define SCITE_QT_LANGUAGE   "SciteQtLanguage"

#define POST_TO_MAIN (QEvent::User + 1)

// some SciTEQt specific menu items:
#define IDM_OPEN_CONTAINING_FOLDER	181

// ************************************************************************

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

// ************************************************************************

class QtCommandWorker : public Worker {
public:
    SciTEQt *pSciTE;
    size_t icmd;
    SA::Position originalEnd;
    int exitStatus;
    GUI::ElapsedTime commandTime;
    std::string output;
    int flags;
    bool seenOutput;
    int outputScroll;

    QtCommandWorker() noexcept;
    void Initialise(bool resetToStart) noexcept;
    void Execute() override;
};

// ************************************************************************

class SciTEQt : public QObject, public SciTEBase
{
    friend SciteQtEnvironmentForJavaScript;

    Q_OBJECT

    // control visibility of ui controls
    Q_PROPERTY(bool showToolBar READ isShowToolBar WRITE setShowToolBar NOTIFY showToolBarChanged)
    Q_PROPERTY(bool showStatusBar READ isShowStatusBar WRITE setShowStatusBar NOTIFY showStatusBarChanged)
    Q_PROPERTY(bool showTabBar READ isShowTabBar WRITE setShowTabBar NOTIFY showTabBarChanged)
    Q_PROPERTY(bool mobilePlatform READ isMobilePlatform NOTIFY mobilePlatformChanged)
    Q_PROPERTY(bool mobileUI READ isMobileUI WRITE setMobileUI NOTIFY mobileUIChanged)
    Q_PROPERTY(bool useMobileDialogHandling READ isUseMobileDialogHandling NOTIFY useMobileDialogHandlingChanged)

    Q_PROPERTY(bool wholeWord READ isWholeWord WRITE setWholeWord NOTIFY wholeWordChanged)
    Q_PROPERTY(bool caseSensitive READ isCaseSensitive WRITE setCaseSensitive NOTIFY caseSensitiveChanged)
    Q_PROPERTY(bool regularExpression READ isRegularExpression WRITE setRegularExpression NOTIFY regularExpressionChanged)
    Q_PROPERTY(bool transformBackslash READ isTransformBackslash WRITE setTransformBackslash NOTIFY transformBackslashChanged)
    Q_PROPERTY(bool wrapAround READ isWrapAround WRITE setWrapAround NOTIFY wrapAroundChanged)
    Q_PROPERTY(bool searchUp READ isSearchUp WRITE setSearchUp NOTIFY searchUpChanged)

    // control content of ui controls
    Q_PROPERTY(QString statusBarText READ getStatusBarText WRITE setStatusBarText NOTIFY statusBarTextChanged)

    Q_PROPERTY(bool findInFilesRunning READ isFindInFilesRunning WRITE setFindInFilesRunning NOTIFY findInFilesRunningChanged)

public:
    explicit SciTEQt(QObject *parent=nullptr, QQmlApplicationEngine * pEngine=nullptr);

    virtual void TabInsert(int index, const GUI::gui_char *title, const GUI::gui_char *fullPath) override;
    virtual void TabSelect(int index) override;
    virtual void RemoveAllTabs() override;

    virtual void WarnUser(int warnID) override;

    virtual void GetWindowPosition(int *left, int *top, int *width, int *height, int *maximize) override;
    virtual bool OpenDialog(const FilePath &directory, const GUI::gui_char *filesFilter) override;
    virtual bool SaveAsDialog() override;
    virtual void LoadSessionDialog() override;
    virtual void SaveSessionDialog() override;
    virtual void SaveACopy() override;
    virtual void SaveAsRTF() override;
    virtual void SaveAsPDF() override;
    virtual void SaveAsTEX() override;
    virtual void SaveAsXML() override;
    virtual void SaveAsHTML() override;
    virtual FilePath GetDefaultDirectory() override;
    virtual FilePath GetSciteDefaultHome() override;
    virtual FilePath GetSciteUserHome() override;
    virtual void Print(bool) override;
    virtual void PrintSetup() override;
    virtual void Find() override;
    virtual MessageBoxChoice WindowMessageBox(GUI::Window &w, const GUI::gui_string &msg, MessageBoxStyle style = mbsIconWarning) override;
    virtual void Filter() override;
    virtual void FindMessageBox(const std::string &msg, const std::string *findItem = nullptr) override;
    virtual void FindIncrement() override;
    virtual void FindInFiles() override;
    virtual void Replace() override;
    virtual void DestroyFindReplace() override;
    virtual void GoLineDialog() override;
    virtual void AbbrevDialog() override;
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
    virtual void CopyAsRTF() override;
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

    virtual void WorkerCommand(int cmd, Worker *pWorker) override;

    virtual bool event(QEvent *e) override;

    // overloaded method to improve the support of WASM and Android
    virtual bool Save(SaveFlags sf = sfProgressVisible) override;
    virtual bool Open(const FilePath &file, OpenFlags of = ofNone) override;

    void ExecuteNext();
    void ResetExecution();
    void ProcessExecute();
    int ExecuteOne(const Job &jobToRun);

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
    bool isFindInFilesRunning() const;
    void setFindInFilesRunning(bool val);

    QString getStatusBarText() const;
    void setStatusBarText(const QString & txt);

    Q_INVOKABLE bool doOpen(const QString & sFileName);
    Q_INVOKABLE void setScintilla(QObject * obj);
    Q_INVOKABLE void setOutput(QObject * obj);
    Q_INVOKABLE void setAboutScite(QObject * obj);
    Q_INVOKABLE void setContent(QObject * obj);
    Q_INVOKABLE void setMainWindow(QObject * obj);

    Q_INVOKABLE QString getLocalisedText(const QString & textInput, bool filterShortcuts = false);

    Q_INVOKABLE bool saveCurrentAs(const QString & sFileName);

    // menu commands
    Q_INVOKABLE void cmdNew();
    Q_INVOKABLE void cmdOpen();
    Q_INVOKABLE void cmdOpenSelectedFileName();
    Q_INVOKABLE void cmdRevert();
    Q_INVOKABLE void cmdClose();
    Q_INVOKABLE void cmdSave();
    Q_INVOKABLE void cmdSaveAs();
    Q_INVOKABLE void cmdSaveACopy();
    Q_INVOKABLE void cmdCopyPath();
    Q_INVOKABLE void cmdOpenContainingFolder();
    Q_INVOKABLE void cmdDeleteFiles();
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
    Q_INVOKABLE void cmdSelectWord();
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
    Q_INVOKABLE void cmdFilter();
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
    Q_INVOKABLE void cmdGo(int supportLevel);
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
    Q_INVOKABLE void cmdSwitchToLastActivatedTab();
    Q_INVOKABLE void cmdBuffersPrevious();
    Q_INVOKABLE void cmdBuffersNext();
    Q_INVOKABLE void cmdBuffersCloseAll();
    Q_INVOKABLE void cmdBuffersSaveAll();
    Q_INVOKABLE void cmdSelectBuffer(int index);
    Q_INVOKABLE void cmdSelectLanguage(int index);
    Q_INVOKABLE void cmdCallTool(int index);
    Q_INVOKABLE void cmdCallImport(int index);
    Q_INVOKABLE void cmdLastOpenedFiles(int index);
    Q_INVOKABLE void cmdHelp();
    Q_INVOKABLE void cmdSciteHelp();
    Q_INVOKABLE void cmdMoreScriptingLanguages();
    Q_INVOKABLE void cmdAboutScite();
    Q_INVOKABLE void cmdAboutSciteQt();
    Q_INVOKABLE void cmdAboutCurrentFile();
    Q_INVOKABLE void cmdRunCurrentAsJavaScriptFile();
    Q_INVOKABLE void cmdRunCurrentAsLuaFile();
    Q_INVOKABLE void cmdRunCurrentAsFuelFile();
    Q_INVOKABLE void cmdShare();
    Q_INVOKABLE void cmdUpdateApplicationActive(bool active);

    Q_INVOKABLE void cmdMarkAll();
    Q_INVOKABLE void cmdTriggerReplace(const QString & find, const QString & replace, bool inSection);
    Q_INVOKABLE void cmdGotoLine(int lineNo, int colPos);
    Q_INVOKABLE void cmdUpdateTabSizeValues(int tabSize, int indentSize, bool useTabs, bool convert);
    Q_INVOKABLE void cmdSetAbbreviationText(const QString & currentText);
    Q_INVOKABLE void cmdSetParameters(const QString & cmd, const QString & parameter1, const QString & parameter2, const QString & parameter3, const QString & parameter4);
    Q_INVOKABLE void cmdParametersDialogClosed();
    Q_INVOKABLE void cmdContextMenu(int menuID);
    Q_INVOKABLE void cmdStartFindInFilesAsync(const QString & directory, const QString & filePattern, const QString & findText, bool wholeWord, bool caseSensitive, bool regularExpression);
    Q_INVOKABLE bool cmdExecuteFind(const QString & findWhatInput, bool wholeWord, bool caseSensitive, bool regularExpression, bool wrap, bool transformBackslash, bool down, bool markAll);
    Q_INVOKABLE void cmdExecuteReplace(const QString & findWhatInput, const QString & replace, bool wholeWord, bool caseSensitive, bool regularExpression, bool wrap, bool transformBackslash, bool replaceAll, bool replaceInSection);

    Q_INVOKABLE QString cmdDirectoryUp(const QString & directoryPath);
    Q_INVOKABLE QString cmdUrlToLocalPath(const QString & url);

    Q_INVOKABLE void cmdAboutQt();
    Q_INVOKABLE void cmdSupportSciteQt();

    Q_INVOKABLE void cmdEnsureCursorVisible();

    Q_INVOKABLE QVariant fillTabContextMenu();

    Q_INVOKABLE QVariant fillToLength(const QString & text, const QString & shortcut);
    Q_INVOKABLE QVariant fillToLengthWithFont(const QString & text, const QString & shortcut, const QFont & font);

    Q_INVOKABLE void setFindText(const QString & text, bool incremental, bool isFilter);
    Q_INVOKABLE void clearFilterAll();

    Q_INVOKABLE void onStatusbarClicked();

    Q_INVOKABLE void setApplicationData(ApplicationData * pApplicationData);

    Q_INVOKABLE void setSpliterPos(int currentPosX, int currentPosY);
    Q_INVOKABLE void startDragSpliterPos(int currentPosX, int currentPosY);

    Q_INVOKABLE bool useSimpleMenus() const;
    Q_INVOKABLE bool isWebassemblyPlatform() const;
    Q_INVOKABLE bool isMacOSPlatform() const;
    bool isMobilePlatform() const;
    bool isUseMobileDialogHandling() const;
    bool isMobileUI() const;
    void setMobileUI(bool val);

    Q_INVOKABLE void updateCurrentWindowPosAndSize(int left, int top, int width, int height, bool maximize);
    Q_INVOKABLE void updateCurrentSelectedFileUrl(const QString & fileUrl);

    Q_INVOKABLE QString getSciteQtInfos() const;

    Q_INVOKABLE void logToDebug(const QString & text);

    Q_INVOKABLE void testFunction(const QString & text);    

    void UpdateStatusbarView();

    void MenuCommand(int cmdID, int source = 0);

    static bool IsAdmin();
    static void SetAdmin(bool value);
    static QString GetStyle();
    static void SetStyle(const QString & value);
    static QString GetOverwriteLanguage();
    static void SetOverwriteLanguage(const QString & value);

public slots:
    void OnAcceptedClicked();
    void OnRejectedClicked();
    void OnFileDialogAcceptedClicked();
    void OnFileDialogRejectedClicked();
    void OnOkClicked();
    void OnCancelClicked();
    void OnYesClicked();
    void OnNoClicked();

    void OnNotifiedFromScintilla(SCNotification scn);
    void OnNotifiedFromOutput(SCNotification scn);
    void OnUriDroppedFromScintilla(const QString & uri);

    void OnFileSearchFinished();
    void OnCurrentFindInFilesItemChanged(const QString & currentItem);
    void OnAddToOutput(const QString & text);
    void OnAddLineToOutput(const QString & text);
    Q_INVOKABLE void OnAddFileContent(const QString & sFileUri, const QString & sDecodedFileUri, const QString & sContent, bool bNewCreated, bool bSaveACopyModus);

    void OnStripFindVisible(bool val);

    void OnScreenAdded(QScreen * pScreen);
    void OnScreenRemoved(QScreen * pScreen);
    void OnPrimaryScreenChanged(QScreen * pScreen);

    void OnAdmin(bool value);

signals:
    void showToolBarChanged();
    void showTabBarChanged();
    void showStatusBarChanged();
    void statusBarTextChanged();
    void mobilePlatformChanged();
    void mobileUIChanged();
    void useMobileDialogHandlingChanged();
    void setMenuChecked(int menuID, bool val);
    void setMenuEnable(int menuID, bool val);
    void dismissMenu();

    void wholeWordChanged();
    void caseSensitiveChanged();
    void regularExpressionChanged();
    void transformBackslashChanged();
    void wrapAroundChanged();
    void searchUpChanged();
    void findInFilesRunningChanged();

    void updateReplacementCount(const QString & count);

    void setInBuffersModel(int index, const QString & txt, bool checked, const QString & ashortcut);
    void removeInBuffersModel(int index);
    void checkStateInBuffersModel(int index, bool checked);
    void setInLanguagesModel(int index, const QString & txt, bool checked, const QString & ashortcut);
    void removeInLanguagesModel(int index);
    void checkStateInLanguagesModel(int index, bool checked);
    void setInToolsModel(int index, const QString & txt, bool checked, const QString & ashortcut);
    void removeInToolsModel(int index);
    void checkStateInToolsModel(int index, bool checked);
    void setInLastOpenedFilesModel(int index, const QString & txt, bool checked, const QString & ashortcut);
    void removeInLastOpenedFilesModel(int index);
    void checkStateInLastOpenedFilesModel(int index, bool checked);
    void setInImportModel(int index, const QString & txt, bool checked, const QString & ashortcut);
    void removeInImportModel(int index);
    void checkStateInImportModel(int index, bool checked);

    void startFileDialog(const QString & sDirectory, const QString & sFilter, const QString & sTitle, bool bAsOpenDialog, bool bSaveACopyModus = false, bool bDeleteModus = false, const QString & sDefaultSaveAsName = "unknown.txt");
    void showInfoDialog(const QString & sInfoText, int style);
    void showAboutSciteDialog();
    void showSupportSciteQtDialog();

    void showFindInFilesDialog(const QString & text, const QStringList & findHistory, const QStringList & filePatternHistory, const QStringList & directoryHistory, bool wholeWord, bool caseSensitive, bool regularExpression);
    void showFindStrip(const QStringList & findHistory, const QStringList & replaceHistory, const QString & text, bool incremental, bool withReplace, bool closeOnFind, bool isFilter);
    void showFind(const QStringList & findHistory, const QString & text, bool wholeWord, bool caseSensitive, bool regExpr, bool wrap, bool transformBackslash, bool down);
    void showReplace(const QStringList & findHistory, const QStringList & replaceHistory, const QString & text, const QString & replace, bool wholeWord, bool caseSensitive, bool regExpr, bool wrap, bool transformBackslash, bool down);
    void showGoToDialog(int currentLine, int currentColumn, int maxLine);
    void showTabSizeDialog(int tabSize, int indentSize, bool useTabs);
    void showAbbreviationDialog(const QStringList & items);
    void showParametersDialog(bool modal, const QStringList & parameters);
    void closeFindReplaceDialog();

    void setVerticalSplit(bool verticalSplit);
    void setOutputHeight(int heightOutput);

    void updateEolMenus(int enumEol);
    void updateEncodingMenus(int enumEncoding);

    void insertTab(int index, const QString & title, const QString & fullPath);
    void selectTab(int index);
    void removeAllTabs();

    void triggerUpdateCurrentWindowPosAndSize();
    void setWindowPosAndSize(int left, int top, int width, int height, bool maximize);
    void setTextToCurrent(const QString & text);
    void addTextToOutput(const QString & text);

    void saveCurrentForWasm(const QString & fileName, const QVariant & sTempFile = QVariant());

    void admin(bool value);

private:
    QObject * getDialog(const QString & objectName);
    QObject * getCurrentInfoDialog();
    QObject * getCurrentFileDialog();
    bool ProcessCurrentFileDialog();
    void RestorePosition();
    void ProcessSave(bool bSetSavePoint);
    void UpdateWindowPosAndSizeIfNeeded(const QRect & rect, bool maximize);
    MessageBoxChoice ShowWindowMessageBox(const QString & msg, MessageBoxStyle style = mbsIconWarning);
    MessageBoxChoice ProcessModalWindowSynchronious(const QString & objectName);
    void CheckAndDeleteGetContentToWriteFunctionPointer();
    void CheckAndDeleteReceiveContentToProcessFunctionPointer();
    void SaveSettingsForQt();
    void LoadSettingsForQt();

    ApplicationData *       m_pApplicationData;     // not an owner !
    QQmlApplicationEngine * m_pEngine;              // not an owner !

    ScriptExecution *       m_pCurrentScriptExecution;  // not an owner !

    FindInFilesAsync        m_aFindInFiles;

    std::function<QString()> *      m_pFcnGetContentToWrite;
    std::function<void(QString)> *  m_pFcnReceiveContentToProcess;

    static bool             m_bIsAdmin;
    static QString          m_sStyle;
    static QString          m_sOverwriteLanguage;

    bool                    m_bWaitDoneFlag;
    int                     m_iMessageDialogAccepted;
    bool                    m_bFileDialogWaitDoneFlag;
    int                     m_iFileDialogMessageDialogAccepted;
    bool                    m_bFindInFilesRunning;
    bool                    m_bStripFindVisible;
    int                     m_iLastTabIndex;
    int                     m_iCurrentTabIndex;
    bool                    m_bIsInUpdateAppActive;
    bool                    m_bIsMobileUI;

    bool                    m_bShowToolBar;
    bool                    m_bShowStatusBar;
    bool                    m_bShowTabBar;
    bool                    m_bStatusBarTextTimerRunning;
    QString                 m_sStatusBarText;
    QString                 m_sSciteStatusBarText;

    int                     m_left;
    int                     m_top;
    int                     m_width;
    int                     m_height;
    bool                    m_maximize;
    QString                 m_sCurrentFileUrl;

    QString                 m_cmd;
    QString                 m_parameter1;
    QString                 m_parameter2;
    QString                 m_parameter3;
    QString                 m_parameter4;
    bool                    m_bParametersDialogOpen;

    GUI::ScintillaWindow    wAboutScite;

    QPrinter                m_aPrinter;

    QtCommandWorker         cmdWorker;
};

#define MSGBOX_RESULT_EMPTY 0
#define MSGBOX_RESULT_CANCEL 1
#define MSGBOX_RESULT_OK 2
#define MSGBOX_RESULT_NO 4
#define MSGBOX_RESULT_YES 8

QString ConvertGuiCharToQString(const GUI::gui_char * s);
QString ConvertGuiStringToQString(const GUI::gui_string & s);

#endif // SCITEQT_H
