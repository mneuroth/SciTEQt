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

class SciTEQt : public QObject, public SciTEBase
{
    Q_OBJECT
public:
    explicit SciTEQt(QObject *parent = nullptr);

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

    Q_INVOKABLE bool doOpen(const QString & sFileName);
    Q_INVOKABLE void setScintilla(QObject * obj);
    Q_INVOKABLE void setOutput(QObject * obj);
    Q_INVOKABLE void setMainWindow(QObject * obj);

    Q_INVOKABLE bool saveCurrentAs(const QString & sFileName);

    // menu commands
    Q_INVOKABLE void CmdNew();
    Q_INVOKABLE void CmdOpen();
    Q_INVOKABLE void CmdClose();
    Q_INVOKABLE void CmdSave();
    Q_INVOKABLE void CmdSaveAs();
    Q_INVOKABLE void CmdLineNumbers();
    Q_INVOKABLE void CmdUseMonospacedFont();

    Q_INVOKABLE void setApplicationData(ApplicationData * pApplicationData);

signals:

private:
    ApplicationData *       m_pApplicationData;
};

#endif // SCITEQT_H
