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

    Q_PROPERTY(bool showToolBar READ isShowToolBar WRITE setShowToolBar NOTIFY showToolBarChanged)
    Q_PROPERTY(bool showStatusBar READ isShowStatusBar WRITE setShowStatusBar NOTIFY showStatusBarChanged)
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

    virtual bool event(QEvent *e) override;

    bool isShowToolBar() const;
    void setShowToolBar(bool val);
    bool isShowStatusBar() const;
    void setShowStatusBar(bool val);
    QString getStatusBarText() const;
    void setStatusBarText(const QString & txt);

    Q_INVOKABLE bool doOpen(const QString & sFileName);
    Q_INVOKABLE void setScintilla(QObject * obj);
    Q_INVOKABLE void setOutput(QObject * obj);
    Q_INVOKABLE void setMainWindow(QObject * obj);

    Q_INVOKABLE QString getLocalisedText(const QString & textInput);

    Q_INVOKABLE bool saveCurrentAs(const QString & sFileName);

    // menu commands
    Q_INVOKABLE void CmdNew();
    Q_INVOKABLE void CmdOpen();
    Q_INVOKABLE void CmdRevert();
    Q_INVOKABLE void CmdClose();
    Q_INVOKABLE void CmdSave();
    Q_INVOKABLE void CmdSaveAs();
    Q_INVOKABLE void CmdCopyPath();
    Q_INVOKABLE void CmdExit();
    Q_INVOKABLE void CmdUndo();
    Q_INVOKABLE void CmdRedo();
    Q_INVOKABLE void CmdCut();
    Q_INVOKABLE void CmdCopy();
    Q_INVOKABLE void CmdPaste();
    Q_INVOKABLE void CmdFind();
    Q_INVOKABLE void CmdFindNext();
    Q_INVOKABLE void CmdFindPrevious();
    Q_INVOKABLE void CmdShowToolBar();
    Q_INVOKABLE void CmdShowStatusBar();
    Q_INVOKABLE void CmdLineNumbers();
    Q_INVOKABLE void CmdAlwaysOnTop();
    Q_INVOKABLE void CmdUseMonospacedFont();
    Q_INVOKABLE void CmdBuffersPrevious();
    Q_INVOKABLE void CmdBuffersNext();
    Q_INVOKABLE void CmdBuffersCloseAll();
    Q_INVOKABLE void CmdBuffersSaveAll();
    Q_INVOKABLE void CmdSelectBuffer(int index);
    Q_INVOKABLE void CmdSelectLanguage(int index);
    Q_INVOKABLE void CmdHelp();
    Q_INVOKABLE void CmdSciteHelp();
    Q_INVOKABLE void CmdAboutScite();

    Q_INVOKABLE void onStatusbarClicked();

    Q_INVOKABLE void setApplicationData(ApplicationData * pApplicationData);

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
    void showStatusBarChanged();
    void statusBarTextChanged();
    void setMenuChecked(int menuID, bool val);

    void setInBuffersModel(int index, const QString & txt, bool checked);
    void removeInBuffersModel(int index);
    void checkStateInBuffersModel(int index, bool checked);
    void setInLanguagesModel(int index, const QString & txt, bool checked);
    void removeInLanguagesModel(int index);
    void checkStateInLanguagesModel(int index, bool checked);

private:
    void startFileDialog(const QString & sDirectory, const QString & sFilter, bool bAsOpenDialog = true);
    QObject * showInfoDialog(const QString & sInfoText, int style);

    ApplicationData *       m_pApplicationData;     // not an owner !
    QQmlApplicationEngine * m_pEngine;

    bool                    m_bWaitDoneFlag;
    int                     m_iMessageDialogAccepted;

    bool                    m_bShowToolBar;
    bool                    m_bShowStatusBar;
    QString                 m_sStatusBarText;
};

#define MSGBOX_RESULT_CANCEL 0
#define MSGBOX_RESULT_OK 1
#define MSGBOX_RESULT_NO 2
#define MSGBOX_RESULT_YES 3

QString ConvertGuiCharToQString(const GUI::gui_char * s);
QString ConvertGuiStringToQString(const GUI::gui_string & s);

#endif // SCITEQT_H
