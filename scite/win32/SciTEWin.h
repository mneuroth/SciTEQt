// SciTE - Scintilla based Text Editor
/** @file SciTEWin.h
 ** Header of main code for the Windows version of the editor.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SCITEWIN_H
#define SCITEWIN_H

#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <optional>
#include <initializer_list>
#include <algorithm>
#include <iterator>
#include <memory>
#include <numeric>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <mutex>

#include <fcntl.h>

#include <sys/stat.h>

#undef _WIN32_WINNT
#undef WINVER
#ifdef WIN_TARGET
#define _WIN32_WINNT WIN_TARGET
#define WINVER WIN_TARGET
#else
#define _WIN32_WINNT  0x0A00
#define WINVER 0x0A00
#endif
#undef NOMINMAX
#define NOMINMAX 1
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <windowsx.h>
#if defined(DISABLE_THEMES)
// Old compilers do not have Uxtheme.h
typedef void *HTHEME;
#else
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#define THEME_AVAILABLE
#endif
#include <shlwapi.h>
// need this header for SHBrowseForFolder
#include <shlobj.h>

#include "ILoader.h"
#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "Scintilla.h"
#include "Lexilla.h"
#include "LexillaAccess.h"

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
#include "Searcher.h"
#include "SciTEBase.h"
#include "UniqueInstance.h"
#include "StripDefinition.h"
#include "Strips.h"
#include "SciTEKeys.h"

constexpr int SCITE_TRAY = WM_APP + 0;
constexpr int SCITE_DROP = WM_APP + 1;
constexpr int SCITE_WORKER = WM_APP + 2;
constexpr int SCITE_SHOWOUTPUT = WM_APP + 3;

enum {
	WORK_EXECUTE = WORK_PLATFORM + 1
};

class SciTEWin;

class CommandWorker : public Worker {
public:
	SciTEWin *pSciTE;
	size_t icmd;
	SA::Position originalEnd;
	int exitStatus;
	GUI::ElapsedTime commandTime;
	std::string output;
	int flags;
	bool seenOutput;
	int outputScroll;

	CommandWorker() noexcept;
	void Initialise(bool resetToStart) noexcept;
	void Execute() override;
};

class Dialog;

class ContentWin : public BaseWin {
	SciTEWin *pSciTEWin;
	bool capturedMouse;
public:
	ContentWin() noexcept : pSciTEWin(nullptr), capturedMouse(false) {
	}
	void SetSciTE(SciTEWin *pSciTEWin_) noexcept {
		pSciTEWin = pSciTEWin_;
	}
	void Paint(HDC hDC, GUI::Rectangle rcPaint);
	LRESULT WndProc(UINT iMessage, WPARAM wParam, LPARAM lParam) override;
};

struct Band {
	bool visible;
	int height;
	bool expands;
	GUI::Window win;
	Band(bool visible_, int height_, bool expands_, GUI::Window win_) noexcept :
		visible(visible_),
		height(height_),
		expands(expands_),
		win(win_) {
	}
};

/** Windows specific stuff.
 **/
class SciTEWin : public SciTEBase {
	friend class ContentWin;
	friend class Strip;
	friend class SearchStrip;
	friend class FindStrip;
	friend class ReplaceStrip;
	friend class UserStrip;

protected:

	bool flatterUI;
	int cmdShow;
	static HINSTANCE hInstance;
	static const TCHAR *className;
	static const TCHAR *classNameInternal;
	static SciTEWin *app;
	WINDOWPLACEMENT winPlace;
	RECT rcWorkArea;
	GUI::gui_char openWhat[200];
	GUI::gui_char tooltipText[MAX_PATH*2 + 1];
	bool tbLarge;
	bool modalParameters;
	GUI::gui_string openFilterDefault;
	bool staticBuild;
	int menuSource;
	std::deque<GUI::gui_string> dropFilesQueue;

	// Fields also used in tool execution thread
	CommandWorker cmdWorker;
	HANDLE hWriteSubProcess;
	DWORD subProcessGroupId;

	HACCEL hAccTable;

	GUI::Rectangle pagesetupMargin;
	HGLOBAL hDevMode;
	HGLOBAL hDevNames;

	UniqueInstance uniqueInstance;

	/// HTMLHelp module
	HMODULE hHH;
	/// Multimedia (sound) module
	HMODULE hMM;

	// Tab Bar
	HFONT fontTabs;
	std::vector<GUI::gui_string> tabNamesCurrent;

	/// Preserve focus during deactivation
	HWND wFocus;

	GUI::Window wFindInFiles;
	GUI::Window wFindReplace;
	GUI::Window wParameters;

	ContentWin contents;
	BackgroundStrip backgroundStrip;
	UserStrip userStrip;
	SearchStrip searchStrip;
	FilterStrip filterStrip;
	FindStrip findStrip;
	ReplaceStrip replaceStrip;

	enum { bandTool, bandTab, bandContents, bandUser, bandBackground, bandSearch, bandFind, bandReplace, bandFilter, bandStatus };
	std::vector<Band> bands;

	void ReadLocalization() override;
	void GetWindowPosition(int *left, int *top, int *width, int *height, int *maximize) override;
	int GetScaleFactor() noexcept;
	bool SetScaleFactor(int scale);

	void ReadEmbeddedProperties() override;
	void ReadPropertiesInitial() override;
	void ReadProperties() override;

	SystemAppearance WindowsAppearance() const noexcept;
	SystemAppearance CurrentAppearance() const noexcept override;

	void TimerStart(int mask) override;
	void TimerEnd(int mask) override;

	void ShowOutputOnMainThread() override;
	void SizeContentWindows() override;
	void SizeSubWindows() override;

	void SetMenuItem(int menuNumber, int position, int itemID,
			 const GUI::gui_char *text, const GUI::gui_char *mnemonic = 0) override;
	void RedrawMenu() override;
	void DestroyMenuItem(int menuNumber, int itemID) override;
	void CheckAMenuItem(int wIDCheckItem, bool val) override;
	void EnableAMenuItem(int wIDCheckItem, bool val) override;
	void CheckMenus() override;

	void LocaliseMenu(HMENU hmenu);
	void LocaliseMenus();
	void LocaliseControl(HWND w);
	void LocaliseDialog(HWND wDialog);

	int DoDialog(const TCHAR *resName, DLGPROC lpProc);
	HWND CreateParameterisedDialog(LPCWSTR lpTemplateName, DLGPROC lpProc);
	GUI::gui_string DialogFilterFromProperty(const GUI::gui_char *filterProperty);
	void CheckCommonDialogError();
	bool OpenDialog(const FilePath &directory, const GUI::gui_char *filesFilter) override;
	FilePath ChooseSaveName(const FilePath &directory, const char *title,
				const GUI::gui_char *filesFilter = nullptr, const char *ext = nullptr);
	bool SaveAsDialog() override;
	void SaveACopy() override;
	void SaveAsHTML() override;
	void SaveAsRTF() override;
	void SaveAsPDF() override;
	void SaveAsTEX() override;
	void SaveAsXML() override;
	void LoadSessionDialog() override;
	void SaveSessionDialog() override;
	bool PreOpenCheck(const GUI::gui_char *arg) override;
	bool IsStdinBlocked() override;

	/// Print the current buffer.
	void Print(bool showDialog) override;
	/// Handle default print setup values and ask the user its preferences.
	void PrintSetup() override;

	BOOL HandleReplaceCommand(int cmd, bool reverseDirection = false);

	MessageBoxChoice WindowMessageBox(GUI::Window &w, const GUI::gui_string &msg, MessageBoxStyle style = mbsIconWarning) override;
	void FindMessageBox(const std::string &msg, const std::string *findItem = 0) override;
	void AboutDialog() override;
	void DropFiles(HDROP hdrop);
	void MinimizeToTray();
	void RestoreFromTray();
	void SettingChanged(WPARAM wParam, LPARAM lParam);
	void SysColourChanged(WPARAM wParam, LPARAM lParam);
	void ScaleChanged(WPARAM wParam, LPARAM lParam);
	static GUI::gui_string ProcessArgs(const GUI::gui_char *cmdLine);
	void QuitProgram() override;

	FilePath GetDefaultDirectory() override;
	FilePath GetSciteDefaultHome() override;
	FilePath GetSciteUserHome() override;

	void SetFileProperties(PropSetFile &ps) override;
	void SetStatusBarText(const char *s) override;

	void UpdateTabs(const std::vector<GUI::gui_string> &tabNames) override;
	void TabInsert(int index, const GUI::gui_char *title, /*for SciteQt*/const GUI::gui_char *fullPath) override;
	void TabSelect(int index) override;
	void RemoveAllTabs() override;

	/// Warn the user, by means defined in its properties.
	void WarnUser(int warnID) override;

	void Notify(SCNotification *notification) override;
	void ShowToolBar() override;
	void ShowTabBar() override;
	void ShowStatusBar() override;
	void ActivateWindow(const char *timestamp) override;
	void ExecuteHelp(const char *cmd);
	void ExecuteOtherHelp(const char *cmd);
	void CopyAsRTF() override;
	void CopyPath() override;
	void FullScreenToggle();
	void Command(WPARAM wParam, LPARAM lParam);
	HWND MainHWND() noexcept;

	void UserStripShow(const char *description) override;
	void UserStripSet(int control, const char *value) override;
	void UserStripSetList(int control, const char *value) override;
	std::string UserStripValue(int control) override;
	void UserStripClosed();
	void ShowBackgroundProgress(const GUI::gui_string &explanation, size_t size, size_t progress) override;
	BOOL FindMessage(HWND hDlg, UINT message, WPARAM wParam);
	static INT_PTR CALLBACK FindDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL ReplaceMessage(HWND hDlg, UINT message, WPARAM wParam);
	static INT_PTR CALLBACK ReplaceDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void UIClosed() override;
	void PerformGrep();
	void FillCombos(Dialog &dlg);
	void FillCombosForGrep(Dialog &dlg);
	BOOL GrepMessage(HWND hDlg, UINT message, WPARAM wParam);
	static INT_PTR CALLBACK GrepDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void CloseOtherFinders(int cmdID);
	void FindIncrement() override;
	void Filter() override;
	bool FilterShowing() override;

	void Find() override;
	void FindInFiles() override;
	void Replace() override;
	void FindReplace(bool replace) override;
	void DestroyFindReplace() override;

	BOOL GoLineMessage(HWND hDlg, UINT message, WPARAM wParam);
	static INT_PTR CALLBACK GoLineDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void GoLineDialog() override;

	BOOL AbbrevMessage(HWND hDlg, UINT message, WPARAM wParam);
	static INT_PTR CALLBACK AbbrevDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void AbbrevDialog() override;

	BOOL TabSizeMessage(HWND hDlg, UINT message, WPARAM wParam);
	static INT_PTR CALLBACK TabSizeDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void TabSizeDialog() override;

	bool ParametersOpen() override;
	void ParamGrab() override;
	bool ParametersDialog(bool modal) override;
	BOOL ParametersMessage(HWND hDlg, UINT message, WPARAM wParam);
	static INT_PTR CALLBACK ParametersDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	BOOL AboutMessage(HWND hDlg, UINT message, WPARAM wParam);
	static INT_PTR CALLBACK AboutDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void AboutDialogWithBuild(int staticBuild_);

	void RestorePosition();

public:

	explicit SciTEWin(Extension *ext = 0);

	// Deleted so SciTEWin objects can not be copied.
	SciTEWin(const SciTEWin &) = delete;
	SciTEWin(SciTEWin &&) = delete;
	SciTEWin &operator=(const SciTEWin &) = delete;
	SciTEWin &operator=(SciTEWin &&) = delete;

	~SciTEWin();

	static bool DialogHandled(GUI::WindowID id, MSG *pmsg) noexcept;
	bool ModelessHandler(MSG *pmsg);

	void CreateUI();
	/// Management of the command line parameters.
	void Run(const GUI::gui_char *cmdLine);
	uintptr_t EventLoop();
	void OutputAppendEncodedStringSynchronised(const GUI::gui_string &s, int codePageDocument);
	void ResetExecution();
	void ExecuteNext();
	void ExecuteGrep(const Job &jobToRun);
	DWORD ExecuteOne(const Job &jobToRun);
	void ProcessExecute();
	void ShellExec(const std::string &cmd, const char *dir);
	void Execute() override;
	void StopExecute() override;
	void AddCommand(const std::string &cmd, const std::string &dir, JobSubsystem jobType, const std::string &input = "", int flags = 0) override;

	void PostOnMainThread(int cmd, Worker *pWorker) override;
	void WorkerCommand(int cmd, Worker *pWorker) override;

	void CreateStrip(LPCWSTR stripName, LPVOID lpParam);
	void Creation();
	LRESULT KeyDown(WPARAM wParam);
	LRESULT KeyUp(WPARAM wParam);
	void AddToPopUp(const char *label, int cmd=0, bool enabled=true) override;
	LRESULT ContextMenuMessage(UINT iMessage, WPARAM wParam, LPARAM lParam);
	void CheckForScintillaFailure(SA::Status statusFailure) noexcept;
	LRESULT WndProc(UINT iMessage, WPARAM wParam, LPARAM lParam);

	std::string EncodeString(const std::string &s) override;
	std::string GetRangeInUIEncoding(GUI::ScintillaWindow &win, SA::Span span) override;

	HACCEL GetAcceleratorTable() noexcept {
		return hAccTable;
	}

	uintptr_t GetInstance() override;
	static void Register(HINSTANCE hInstance_);
	static LRESULT PASCAL TWndProc(
		HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	friend class UniqueInstance;
};

inline bool IsKeyDown(int key) noexcept {
	return (::GetKeyState(key) & 0x80000000) != 0;
}

GUI::Point PointOfCursor() noexcept;
GUI::Point ClientFromScreen(HWND hWnd, GUI::Point ptScreen) noexcept;

// Common minor conversions

constexpr GUI::Point PointFromLong(LPARAM lPoint) noexcept {
	// static_cast<short> needed for negative coordinates
	return GUI::Point(static_cast<short>(LOWORD(lPoint)), static_cast<short>(HIWORD(lPoint)));
}

constexpr int ControlIDOfWParam(WPARAM wParam) noexcept {
	return wParam & 0xffff;
}

inline HWND HwndOf(GUI::Window w) noexcept {
	return static_cast<HWND>(w.GetID());
}

inline HMENU HmenuID(size_t id) noexcept {
	return reinterpret_cast<HMENU>(id);
}

inline POINT *PointPointer(GUI::Point *pt) noexcept {
	return reinterpret_cast<POINT *>(pt);
}

#endif
