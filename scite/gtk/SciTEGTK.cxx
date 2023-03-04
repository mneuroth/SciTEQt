// SciTE - Scintilla based Text Editor
// SciTEGTK.cxx - main code for the GTK version of the editor
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cerrno>
#include <csignal>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <initializer_list>
#include <algorithm>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <glib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "Scintilla.h"
#include "ScintillaWidget.h"

#include "Lexilla.h"
#include "LexillaAccess.h"

#include "GUI.h"
#include "ScintillaWindow.h"
#include "StringList.h"
#include "StringHelpers.h"
#include "FilePath.h"
#include "StyleDefinition.h"
#include "PropSetFile.h"

#include "Extender.h"

#include "SciTE.h"
#include "JobQueue.h"
#include "pixmapsGNOME.h"
#include "SciIcon.h"
#include "Widget.h"
#include "Cookie.h"
#include "Worker.h"
#include "MatchMarker.h"
#include "Searcher.h"
#include "SciTEBase.h"
#include "StripDefinition.h"
#include "Strips.h"

#include "SciTEKeys.h"

#ifndef NO_EXTENSIONS
#include "MultiplexExtension.h"

#ifndef NO_FILER
#include "DirectorExtension.h"
#endif

#ifndef NO_LUA
#include "LuaExtension.h"
#endif

#endif

// Key names are longer for GTK 3
#if GTK_CHECK_VERSION(3,0,0)
#define GKEY_Tab GDK_KEY_Tab
#define GKEY_ISO_Left_Tab GDK_KEY_ISO_Left_Tab
#define GKEY_KP_Enter GDK_KEY_KP_Enter
#define GKEY_KP_Multiply GDK_KEY_KP_Multiply
#define GKEY_Control_L GDK_KEY_Control_L
#define GKEY_Control_R GDK_KEY_Control_R
#define GKEY_F1 GDK_KEY_F1
#define GKEY_F2 GDK_KEY_F2
#define GKEY_F3 GDK_KEY_F3
#define GKEY_F4 GDK_KEY_F4
#else
#define GKEY_Tab GDK_Tab
#define GKEY_ISO_Left_Tab GDK_ISO_Left_Tab
#define GKEY_KP_Enter GDK_KP_Enter
#define GKEY_KP_Multiply GDK_KP_Multiply
#define GKEY_Control_L GDK_Control_L
#define GKEY_Control_R GDK_Control_R
#define GKEY_F1 GDK_F1
#define GKEY_F2 GDK_F2
#define GKEY_F3 GDK_F3
#define GKEY_F4 GDK_F4
#endif

const char appName[] = "SciTE";

class SciTEGTK;

namespace {

enum { mbsAboutBox = 0x100000 };

GdkWindow *WindowFromWidget(GtkWidget *w) noexcept {
	return gtk_widget_get_window(w);
}

GtkWidget *PWidget(const GUI::Window &w) noexcept {
	return static_cast<GtkWidget *>(w.GetID());
}

std::string WithoutUnderscore(const char *s) {
	std::string ret;
	while (*s) {
		if (*s != '_')
			ret += *s;
		s++;
	}
	return ret;
}

void SetCairoColour(cairo_t *cr, SA::Colour co) noexcept {
	cairo_set_source_rgb(cr,
		(co & 0xff) / 255.0,
		((co >> 8) & 0xff) / 255.0,
		((co >> 16) & 0xff) / 255.0);
}

constexpr double doubleFromPangoUnits(int pu) noexcept {
	return static_cast<double>(pu) / PANGO_SCALE;
}

GtkWidget *pixmap_new(const char **xpm) {
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data(xpm);
	return gtk_image_new_from_pixbuf(pixbuf);
}

void AddToolSpace(GtkToolbar *toolbar) {
	GtkToolItem *space = gtk_separator_tool_item_new();
	gtk_widget_show(GTK_WIDGET(space));
	gtk_toolbar_insert(toolbar, space, -1);
}

// Callback thunk class connects GTK signals to a SciTEGTK instance method.
template< void (SciTEGTK::*method)() >
class Signal {
public:
	static void Function(GtkWidget */*w*/, SciTEGTK *app) {
		(app->*method)();
	}
};

// Callback thunk class connects GTK signals to a SciTEGTK instance method.
template< void (SciTEGTK::*method)(int responseID) >
class ResponseSignal {
public:
	static void Function(GtkDialog */*w*/, gint responseID, SciTEGTK *app) {
		(app->*method)(responseID);
	}
};

template< void (SciTEGTK::*method)(int responseID) >
inline void AttachResponse(GtkWidget *w, SciTEGTK *object) {
	ResponseSignal <method> sig;
	g_signal_connect(G_OBJECT(w), "response", G_CALLBACK(sig.Function), object);
}

// Field added to GTK 1.x ItemFactoryEntry for 2.x  so have a struct that is the same as 1.x
struct SciTEItemFactoryEntry {
	const char *path;
	const char *accelerator;
	GCallback callback;
	int callback_action;
	const char *item_type;
};

}

long SciTEKeys::ParseKeyCode(const char *mnemonic) {
	int modsInKey = 0;
	int keyval = -1;

	if (mnemonic && *mnemonic) {
		std::string sKey = mnemonic;

		if (RemoveStringOnce(sKey, "Ctrl+"))
			modsInKey |= GDK_CONTROL_MASK;
		if (RemoveStringOnce(sKey, "Shift+"))
			modsInKey |= GDK_SHIFT_MASK;
		if (RemoveStringOnce(sKey, "Alt+"))
			modsInKey |= GDK_MOD1_MASK;
		if (RemoveStringOnce(sKey, "Super+"))
			modsInKey |= GDK_MOD4_MASK;

		if (sKey.length() == 1) {
			if (modsInKey & GDK_CONTROL_MASK && !(modsInKey & GDK_SHIFT_MASK))
				LowerCaseAZ(sKey);
			keyval = sKey[0];
		} else if ((sKey.length() > 1)) {
			if ((sKey[0] == 'F') && (IsADigit(sKey[1]))) {
				sKey.erase(0, 1);
				const int fkeyNum = atoi(sKey.c_str());
				if (fkeyNum >= 1 && fkeyNum <= 12)
#if GTK_CHECK_VERSION(3,0,0)
					keyval = fkeyNum - 1 + GDK_KEY_F1;
#else
					keyval = fkeyNum - 1 + GDK_F1;
#endif
			} else {
#if GTK_CHECK_VERSION(3,0,0)
				if (sKey == "Left") {
					keyval = GDK_KEY_Left;
				} else if (sKey == "Right") {
					keyval = GDK_KEY_Right;
				} else if (sKey == "Up") {
					keyval = GDK_KEY_Up;
				} else if (sKey == "Down") {
					keyval = GDK_KEY_Down;
				} else if (sKey == "Insert") {
					keyval = GDK_KEY_Insert;
				} else if (sKey == "End") {
					keyval = GDK_KEY_End;
				} else if (sKey == "Home") {
					keyval = GDK_KEY_Home;
				} else if (sKey == "Enter") {
					keyval = GDK_KEY_Return;
				} else if (sKey == "Space") {
					keyval = GDK_KEY_space;
				} else if (sKey == "Tab") {
					keyval = GDK_KEY_Tab;
				} else if (sKey == "KeypadPlus") {
					keyval = GDK_KEY_KP_Add;
				} else if (sKey == "KeypadMinus") {
					keyval = GDK_KEY_KP_Subtract;
				} else if (sKey == "KeypadMultiply") {
					keyval = GDK_KEY_KP_Multiply;
				} else if (sKey == "KeypadDivide") {
					keyval = GDK_KEY_KP_Divide;
				} else if (sKey == "Escape") {
					keyval = GDK_KEY_Escape;
				} else if (sKey == "Delete") {
					keyval = GDK_KEY_Delete;
				} else if (sKey == "PageUp") {
					keyval = GDK_KEY_Page_Up;
				} else if (sKey == "PageDown") {
					keyval = GDK_KEY_Page_Down;
				} else if (sKey == "Slash") {
					keyval = GDK_KEY_slash;
				} else if (sKey == "Question") {
					keyval = GDK_KEY_question;
				} else if (sKey == "Equal") {
					keyval = GDK_KEY_equal;
				} else if (sKey == "Win") {
					keyval = GDK_KEY_Super_L;
				} else if (sKey == "Menu") {
					keyval = GDK_KEY_Menu;
				}
#else
				if (sKey == "Left") {
					keyval = GDK_Left;
				} else if (sKey == "Right") {
					keyval = GDK_Right;
				} else if (sKey == "Up") {
					keyval = GDK_Up;
				} else if (sKey == "Down") {
					keyval = GDK_Down;
				} else if (sKey == "Insert") {
					keyval = GDK_Insert;
				} else if (sKey == "End") {
					keyval = GDK_End;
				} else if (sKey == "Home") {
					keyval = GDK_Home;
				} else if (sKey == "Enter") {
					keyval = GDK_Return;
				} else if (sKey == "Space") {
					keyval = GDK_space;
				} else if (sKey == "Tab") {
					keyval = GDK_Tab;
				} else if (sKey == "KeypadPlus") {
					keyval = GDK_KP_Add;
				} else if (sKey == "KeypadMinus") {
					keyval = GDK_KP_Subtract;
				} else if (sKey == "KeypadMultiply") {
					keyval = GDK_KP_Multiply;
				} else if (sKey == "KeypadDivide") {
					keyval = GDK_KP_Divide;
				} else if (sKey == "Escape") {
					keyval = GDK_Escape;
				} else if (sKey == "Delete") {
					keyval = GDK_Delete;
				} else if (sKey == "PageUp") {
					keyval = GDK_Page_Up;
				} else if (sKey == "PageDown") {
					keyval = GDK_Page_Down;
				} else if (sKey == "Slash") {
					keyval = GDK_slash;
				} else if (sKey == "Question") {
					keyval = GDK_question;
				} else if (sKey == "Equal") {
					keyval = GDK_equal;
				} else if (sKey == "Win") {
					keyval = GDK_Super_L;
				} else if (sKey == "Menu") {
					keyval = GDK_Menu;
				}
#endif
			}
		}
	}

	return (keyval > 0) ? (keyval | (modsInKey<<16)) : 0;
}

bool SciTEKeys::MatchKeyCode(long parsedKeyCode, int keyval, int modifiers) noexcept {
	return parsedKeyCode && !(0xFFFF0000 & (keyval | modifiers)) && (parsedKeyCode == (keyval | (modifiers<<16)));
}

class DialogGoto : public Dialog {
public:
	WEntry entryGoto;
};

class DialogAbbrev : public Dialog {
public:
	WComboBoxEntry comboAbbrev;
};

class DialogTabSize : public Dialog {
public:
	WEntry entryTabSize;
	WEntry entryIndentSize;
	WToggle toggleUseTabs;
};

class DialogParameters : public Dialog {
public:
	bool paramDialogCanceled;
	WEntry entryParam[SciTEBase::maxParam];
	DialogParameters() :  paramDialogCanceled(true) {
	}
};

class FindDialog : public Dialog {
public:
	virtual void FillCombosInDialog()=0;
};

class DialogFindInFiles : public FindDialog, public SearchUI {
public:
	WComboBoxEntry wComboFiles;
	WComboBoxEntry wComboFindInFiles;
	WComboBoxEntry comboDir;
	WToggle toggleWord;
	WToggle toggleCase;
	WButton btnDotDot;
	WButton btnBrowse;
	void GrabFields();
	void FillFields();
	void FillCombosInDialog() override;
};

class DialogFindReplace : public FindDialog, public SearchUI {
public:
	WStatic labelFind;
	WComboBoxEntry wComboFind;
	WStatic labelReplace;
	WComboBoxEntry wComboReplace;
	WToggle toggleWord;
	WToggle toggleCase;
	WToggle toggleRegExp;
	WToggle toggleWrap;
	WToggle toggleUnSlash;
	WToggle toggleReverse;
	void GrabFields();
	void FillFields();
	void FillCombosInDialog() override;
};

// Manage the use of the GDK thread lock within glib signal handlers
class ThreadLockMinder {
public:
#ifndef GDK_VERSION_3_6
	ThreadLockMinder() noexcept {
		gdk_threads_enter();
	}
	~ThreadLockMinder() noexcept {
		gdk_threads_leave();
	}
#endif
};

class SciTEGTK : public SciTEBase, UserStripWatcher {

	friend class UserStrip;

protected:

	GtkWidget *splitPane;

	guint sbContextID;
	GUI::Window wToolBarBox;
	int toolbarDetachable;
	int menuSource;

	// Control of sub process
	FilePath sciteExecutable;
	size_t icmd;
	int originalEnd;
	int fdFIFO;
	GPid pidShell;
	bool triedKill;
	int exitStatus;
	guint pollID;
	int inputHandle;
	GIOChannel *inputChannel;
	GUI::ElapsedTime commandTime;
	std::string lastOutput;
	int lastFlags;

	// For single instance
	std::string uniqueInstance;
	guint32 startupTimestamp;

	guint timerID;
	guint idlerID;

	BackgroundStrip backgroundStrip;
	UserStrip userStrip;

	enum FileFormat { sfSource, sfCopy, sfHTML, sfRTF, sfPDF, sfTEX, sfXML } saveFormat;
	Dialog dlgFileSelector;
	DialogFindInFiles dlgFindInFiles;
	DialogGoto dlgGoto;
	DialogAbbrev dlgAbbrev;
	DialogTabSize dlgTabSize;

	GtkWidget *wIncrementPanel;
	GtkWidget *IncSearchEntry;
	GtkWidget *IncSearchBtnNext;

	FindStrip findStrip;
	ReplaceStrip replaceStrip;
	FilterStrip filterStrip;

	DialogFindReplace dlgFindReplace;
	DialogParameters dlgParameters;

	GtkWidget *btnCompile;
	GtkWidget *btnBuild;
	GtkWidget *btnStop;

	GtkWidget *menuBar;
	std::map<std::string, GtkWidget *> pulldowns;
	std::map<std::string, GSList *> radiogroups;
	std::map<int, GtkWidget *> mapMenuItemFromId;
	GtkAccelGroup *accelGroup;

	gint	fileSelectorWidth;
	gint	fileSelectorHeight;

	GtkSettings *settings;

	GtkPrintSettings *printSettings;
	GtkPageSetup *pageSetup;
	std::vector<int> pageStarts;

	GtkWidget *AddMBButton(GtkWidget *dialog, const char *label,
	                       MessageBoxChoice val, GtkAccelGroup *accel_group, bool isDefault = false);
	void SetIcon();

	void ReadLocalization() override;
	void ReadPropertiesInitial() override;
	void ReadProperties() override;

	static gboolean TimerTick(gpointer pSciTE);
	void TimerStart(int mask) override;
	void TimerEnd(int mask) override;
	static gboolean IdlerTick(gpointer pSciTE);
	void SetIdler(bool on) override;

	void GetWindowPosition(int *left, int *top, int *width, int *height, int *maximize) override;

	void SizeContentWindows() override;
	void SizeSubWindows() override;
	bool UpdateOutputSize();

	GtkWidget *MenuItemFromAction(int itemID);
	void SetMenuItem(int menuNumber, int position, int itemID,
	                         const char *text, const char *mnemonic = 0) override;
	void DestroyMenuItem(int menuNumber, int itemID) override;
	void CheckAMenuItem(int wIDCheckItem, bool val) override;
	void EnableAMenuItem(int wIDCheckItem, bool val) override;
	void CheckMenusClipboard() override;
	void CheckMenus() override;
	static void PopUpCmd(GtkMenuItem *menuItem, SciTEGTK *scitew);
	void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;
	void ExecuteNext();
	void ResetExecution();

	void OpenUriList(const char *list) override;
	bool OpenDialog(const FilePath &directory, const char *filesFilter) override;
	bool HandleSaveAs(const char *savePath);
	bool SaveAsXXX(FileFormat fmt, const char *title, const char *ext=0);
	bool SaveAsDialog() override;
	void SaveACopy() override;
	void SaveAsHTML() override;
	void SaveAsRTF() override;
	void SaveAsPDF() override;
	void SaveAsTEX() override;
	void SaveAsXML() override;
	void LoadSessionDialog() override;
	void SaveSessionDialog() override;

	void SetupFormat(Sci_RangeToFormat &frPrint, GtkPrintContext *context);
	void BeginPrintThis(GtkPrintOperation *operation, GtkPrintContext *context);
	static void BeginPrint(GtkPrintOperation *operation, GtkPrintContext *context, SciTEGTK *scitew);
	void DrawPageThis(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr);
	static void DrawPage(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, SciTEGTK *scitew);
	void Print(bool) override;
	void PrintSetup() override;

	std::string GetRangeInUIEncoding(GUI::ScintillaWindow &win, SA::Span span) override;

	MessageBoxChoice WindowMessageBox(GUI::Window &w, const GUI::gui_string &msg, int style = mbsIconWarning) override;
	void FindMessageBox(const std::string &msg, const std::string *findItem=0) override;
	void AboutDialog() override;
	void QuitProgram() override;

	std::string EncodeString(const std::string &s) override;
	void FindReplaceGrabFields();
	void Find() override;
	void UIClosed() override;
	void UIHasFocus() override;
	GtkWidget *TranslatedLabel(const char *original);
	void CloseOtherFinders(int cmdID);
	void FindIncrement() override;
	void Filter() override;
	bool FilterShowing() override;
	void FindInFilesResponse(int responseID);
	void FindInFiles() override;
	void Replace() override;
	void FindReplaceResponse(int responseID);
	void FindReplace(bool replace) override;
	void DestroyFindReplace() override;
	void GoLineDialog() override;
	void AbbrevDialog() override;
	void TabSizeDialog() override;
	bool ParametersDialog(bool modal) override;

	FilePath GetDefaultDirectory() override;
	FilePath GetSciteDefaultHome() override;
	FilePath GetSciteUserHome() override;

	void SetStatusBarText(const char *s) override;
	void TabInsert(int index, const GUI::gui_char *title) override;
	void TabSelect(int index) override;
	void RemoveAllTabs() override;
	void SetFileProperties(PropSetFile &ps) override;
	void UpdateStatusBar(bool bUpdateSlowData) override;

	void Notify(SCNotification *notification) override;
	void ShowToolBar() override;
	void ShowTabBar() override;
	void ShowStatusBar() override;
	void ActivateWindow(const char *timestamp) override;
	void CopyPath() override;
	void Command(unsigned long wParam, long lParam = 0);
	void ContinueExecute(int fromPoll);

	void UserStripShow(const char *description) override;
	void UserStripSet(int control, const char *value) override;
	void UserStripSetList(int control, const char *value) override;
	std::string UserStripValue(int control) override;
	void UserStripClosed() override;
	void ShowBackgroundProgress(const GUI::gui_string &explanation, size_t size, size_t progress) override;

	// Single instance
	void SendFileName(int sendPipe, const char* filename);
	bool CheckForRunningInstance(int argc, char* argv[]);

	// GTK Signal Handlers

	void FindInFilesCmd();
	void FindInFilesDotDot();
	void FindInFilesBrowse();

	void GotoCmd();
	void GotoResponse(int responseID);

	void AbbrevCmd();
	void AbbrevResponse(int responseID);

	void TabSizeSet(int &tabSize, bool &useTabs);
	void TabSizeCmd();
	void TabSizeConvertCmd();
	void TabSizeResponse(int responseID);
	void FindIncrementSetColour(bool valid);
	void FindIncrementNext(bool select);
	void FindIncrementChanged();
	void FindIncrementCompleteCmd();
	static gboolean FindIncrementFocusOutSignal(GtkWidget *w);
	static gboolean FindIncrementEscapeSignal(GtkWidget *w, GdkEventKey *event, SciTEGTK *scitew);

	void FRFindCmd();
	void FRReplaceCmd();
	void FRReplaceAllCmd();
	void FRReplaceInSelectionCmd();
	void FRReplaceInBuffersCmd();
	void FRMarkAllCmd();
	void FillCombos(FindDialog &dlg);
	void FillCombosForGrep();

	bool ParametersOpen() override;
	void ParamGrab() override;
	void ParamCancelCmd();
	void ParamCmd();
	void ParamResponse(int responseID);

	static void PanePositionChanged(GObject *object, GParamSpec *pspec, SciTEGTK *scitew);
	static gint PaneButtonRelease(GtkWidget *widget, GdkEvent *event, SciTEGTK *scitew);

	static gboolean IOSignal(GIOChannel *source, GIOCondition condition, SciTEGTK *scitew);
	static gint QuitSignal(GtkWidget *w, GdkEventAny *e, SciTEGTK *scitew);
	static void ButtonSignal(GtkWidget *widget, gpointer data);
	static void MenuSignal(GtkMenuItem *menuitem, SciTEGTK *scitew);
	static void NotifySignal(GtkWidget *w, gint wParam, SCNotification *notification, SciTEGTK *scitew);
	static gint KeyPress(GtkWidget *widget, GdkEventKey *event, SciTEGTK *scitew);
	static gint KeyRelease(GtkWidget *widget, GdkEventKey *event, SciTEGTK *scitew);
	gint Key(GdkEventKey *event);
	static gint MousePress(GtkWidget *widget, GdkEventButton *event, SciTEGTK *scitew);
	gint Mouse(GdkEventButton *event);

	static void DragDataReceived(GtkWidget *widget, GdkDragContext *context,
	                             gint x, gint y, GtkSelectionData *selectionData, guint info, guint time, SciTEGTK *scitew);

	gint TabBarRelease(GtkNotebook *notebook, GdkEventButton *event);
	gint TabBarScroll(GdkEventScroll *event);
	static gint TabBarReleaseSignal(GtkNotebook *notebook, GdkEventButton *event, SciTEGTK *scitew) {
		return scitew->TabBarRelease(notebook, event);
	}
	static gint TabBarScrollSignal(GtkNotebook *, GdkEventScroll *event, SciTEGTK *scitew) {
		return scitew->TabBarScroll(event);
	}

	SystemAppearance CurrentAppearance() const noexcept override;
	static void ThemeSignal(GObject *, GParamSpec *, SciTEGTK *scitew) {
		scitew->CheckAppearanceChanged();
	}

public:

	// TODO: get rid of this - use callback argument to find SciTEGTK
	static SciTEGTK *instance;

	explicit SciTEGTK(Extension *ext = 0);
	~SciTEGTK() override;

	void WarnUser(int warnID) override;
	GtkWidget *AddToolButton(const char *text, int cmd, GtkWidget *toolbar_icon);
	void AddToolBar();
	std::string TranslatePath(const char *path);
	void CreateTranslatedMenu(int n, const SciTEItemFactoryEntry items[],
	                          int nRepeats = 0, const char *prefix = 0, int startNum = 0,
	                          int startID = 0, const char *radioStart = 0);
	void CreateMenu();
	void CreateStrips(GtkWidget *boxMain);
	bool StripHasFocus() const;
	void CreateUI();
	void LayoutUI();
	void Run(int argc, char *argv[]);
	void Execute() override;
	void StopExecute() override;
	static int PollTool(SciTEGTK *scitew);
	static void ReapChild(GPid, gint, gpointer);
	void PostOnMainThread(int cmd, Worker *pWorker) override;
	static gboolean PostCallback(void *ptr);
	// Single instance
	void SetStartupTime(const char *timestamp);
};

SciTEGTK *SciTEGTK::instance;

SciTEGTK::SciTEGTK(Extension *ext) : SciTEBase(ext) {
	toolbarDetachable = 0;
	menuSource = 0;
	// Control of sub process
	icmd = 0;
	originalEnd = 0;
	fdFIFO = 0;
	pidShell = 0;
	triedKill = false;
	exitStatus = 0;
	pollID = 0;
	inputHandle = 0;
	inputChannel = 0;
	lastFlags = 0;

	startupTimestamp = 0;

	timerID = 0;

	PropSetFile::SetCaseSensitiveFilenames(true);
	propsPlatform.Set("PLAT_GTK", "1");
	propsPlatform.Set("PLAT_UNIX", "1");

	ReadEnvironment();

	pathAbbreviations = GetAbbrevPropertiesFileName();

	settings = gtk_settings_get_default();
	appearance = CurrentAppearance();
	g_signal_connect(settings, "notify::gtk-theme-name", G_CALLBACK(ThemeSignal), this);

	ReadGlobalPropFile();
	ReadAbbrevPropFile();

	saveFormat = sfSource;
	wIncrementPanel = 0;
	IncSearchEntry = 0;
	IncSearchBtnNext = 0;
	btnCompile = 0;
	btnBuild = 0;
	btnStop = 0;
	menuBar = 0;
	accelGroup = 0;

	splitPane = 0;
	heightBar = 0;

	fileSelectorWidth = 580;
	fileSelectorHeight = 480;

	printSettings = NULL;
	pageSetup = NULL;

	// Fullscreen handling
	fullScreen = false;

	instance = this;
}

SciTEGTK::~SciTEGTK() {
	g_object_unref(settings);
}

static void destroyDialog(GtkWidget *, gpointer *window) {
	if (window) {
		GUI::Window *pwin = reinterpret_cast<GUI::Window *>(window);
		*(pwin) = 0;
	}
}

static void destroyDialogFindReplace(GtkWidget *, gpointer *window) {
	if (window) {
		DialogFindReplace *dlg = reinterpret_cast<DialogFindReplace *>(window);
		*((GUI::Window *)dlg) = 0;
	}
}

void SciTEGTK::WarnUser(int) {}

static GtkWidget *messageBoxDialog = 0;
static int messageBoxResult = 0;

static gint messageBoxKey(GtkWidget *w, GdkEventKey *event, gpointer p) {
	if (event->keyval == GKEY_Escape) {
		g_signal_stop_emission_by_name(G_OBJECT(w), "key_press_event");
		gtk_widget_destroy(GTK_WIDGET(w));
		messageBoxDialog = 0;
		messageBoxResult = GPOINTER_TO_INT(p);
	}
	return FALSE;
}

static void messageBoxDestroy(GtkWidget *, gpointer *) {
	messageBoxDialog = 0;
	messageBoxResult = 0;
}

static void messageBoxOK(GtkWidget *, gpointer p) {
	gtk_widget_destroy(GTK_WIDGET(messageBoxDialog));
	messageBoxDialog = 0;
	messageBoxResult = GPOINTER_TO_INT(p);
}

GtkWidget *SciTEGTK::AddMBButton(GtkWidget *dialog, const char *label,
	MessageBoxChoice val, GtkAccelGroup *accel_group, bool isDefault) {
	GUI::gui_string translated = localiser.Text(label);
#if GTK_CHECK_VERSION(3,14,0)
	GtkWidget *button = gtk_dialog_add_button(GTK_DIALOG(dialog), translated.c_str(), static_cast<int>(val));
#else
	GtkWidget *button = gtk_button_new_with_mnemonic(translated.c_str());
#endif
	gtk_widget_set_can_default(button, TRUE);
	const size_t posMnemonic = translated.find('_');
	if (posMnemonic != GUI::gui_string::npos) {
		// With a "Yes" button want to respond to pressing "y" as well as standard "Alt+y"
		const guint key = MakeLowerCase(translated[posMnemonic + 1]);
		gtk_widget_add_accelerator(button, "clicked", accel_group,
	                           key, GdkModifierType{}, GtkAccelFlags{});
	}
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(messageBoxOK), GINT_TO_POINTER(val));
#if GTK_CHECK_VERSION(3,0,0)
#if !GTK_CHECK_VERSION(3,14,0)
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))),
 	                   button, TRUE, TRUE, 0);
#endif
#else
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
 	                   button, TRUE, TRUE, 0);
#endif
	if (isDefault) {
		gtk_widget_grab_default(GTK_WIDGET(button));
	}
	gtk_widget_show(button);
	return button;
}

FilePath executableDirectory;

FilePath SciTEGTK::GetDefaultDirectory() {
	const char *where = getenv("SciTE_HOME");
#ifdef SYSCONF_PATH
	if (!where) {
		where = SYSCONF_PATH;
	}
#else
	if (!where) {
		where = getenv("HOME");
	}
#endif
	if (where) {
		return FilePath(where);
	}

	return FilePath("");
}

FilePath SciTEGTK::GetSciteDefaultHome() {
	const char *where = getenv("SciTE_HOME");
#ifdef SYSCONF_PATH
	if (!where) {
		where = SYSCONF_PATH;
	}
#else
	if (!where) {
		where = getenv("HOME");
	}
#endif
	if (where) {
		return FilePath(where);

	}
	return FilePath("");
}

FilePath SciTEGTK::GetSciteUserHome() {
	// First looking for environment variable $SciTE_USERHOME
	// to set SciteUserHome. If not present we look for $SciTE_HOME
	// then defaulting to $HOME
	char *where = getenv("SciTE_USERHOME");
	if (!where) {
		where = getenv("SciTE_HOME");
		if (!where) {
			where = getenv("HOME");
		}
	}

	return FilePath(where);
}

void SciTEGTK::SetStatusBarText(const char *s) {
	gtk_statusbar_pop(GTK_STATUSBAR(PWidget(wStatusBar)), sbContextID);
	gtk_statusbar_push(GTK_STATUSBAR(PWidget(wStatusBar)), sbContextID, s);
}

void SciTEGTK::TabInsert(int index, const GUI::gui_char *title) {
	if (wTabBar.GetID()) {
		GtkWidget *tablabel = gtk_label_new(title);
		GtkWidget *tabcontent;
		if (props.GetInt("pathbar.visible")) {
			const FilePath &fp = buffers.buffers[index].file;
			if (fp.IsUntitled()) {
				tabcontent = gtk_label_new(localiser.Text("Untitled").c_str());
			} else {
				tabcontent = gtk_label_new(fp.AsInternal());
				if (props.GetInt("pathbar.selectable")) {
					gtk_label_set_selectable(GTK_LABEL(tabcontent), TRUE);
					gtk_widget_set_can_focus(tabcontent, FALSE);
					gtk_label_set_use_markup(GTK_LABEL(tabcontent), TRUE);
					gtk_label_set_ellipsize(GTK_LABEL(tabcontent), PANGO_ELLIPSIZE_MIDDLE);
				}
			}
		} else {
			// No path bar
			tabcontent = gtk_image_new();
			gtk_image_set_pixel_size(GTK_IMAGE(tabcontent), 0);
		}

		gtk_widget_show(tablabel);
		gtk_widget_show(tabcontent);

		gtk_notebook_append_page(GTK_NOTEBOOK(wTabBar.GetID()), tabcontent, tablabel);
	}
}

void SciTEGTK::TabSelect(int index) {
	if (wTabBar.GetID())
		gtk_notebook_set_current_page(GTK_NOTEBOOK(wTabBar.GetID()), index);
}

void SciTEGTK::RemoveAllTabs() {
	if (wTabBar.GetID()) {
		while (gtk_notebook_get_nth_page(GTK_NOTEBOOK(wTabBar.GetID()), 0))
			gtk_notebook_remove_page(GTK_NOTEBOOK(wTabBar.GetID()), 0);
	}
}

void SciTEGTK::SetFileProperties(PropSetFile &ps) {
	char timeBuffer[200];

	const time_t timeNow = time(NULL);

	strftime(timeBuffer, sizeof(timeBuffer), "%x", localtime(&timeNow));
	ps.Set("CurrentDate", timeBuffer);
	strftime(timeBuffer, sizeof(timeBuffer), "%X", localtime(&timeNow));
	ps.Set("CurrentTime", timeBuffer);

	const time_t timeFileModified = filePath.ModifiedTime();
	strftime(timeBuffer, sizeof(timeBuffer), "%x", localtime(&timeFileModified));
	ps.Set("FileDate", timeBuffer);
	strftime(timeBuffer, sizeof(timeBuffer), "%X", localtime(&timeFileModified));
	ps.Set("FileTime", timeBuffer);

	std::string fa;
	if (access(filePath.AsInternal(), W_OK)) {
		fa += "R";
	}
	if (filePath.AsInternal()[0] == '.') {
		fa += "H";
	}
	ps.Set("FileAttr", fa);
}

void SciTEGTK::UpdateStatusBar(bool bUpdateSlowData) {
	SciTEBase::UpdateStatusBar(bUpdateSlowData);
}

void SciTEGTK::Notify(SCNotification *notification) {
	if (notification->nmhdr.idFrom == IDM_SRCWIN) {
		if (notification->nmhdr.code == SCN_FOCUSIN)
			Activate(true);
		else if (notification->nmhdr.code == SCN_FOCUSOUT)
			Activate(false);
	}

	SciTEBase::Notify(notification);
}

void SciTEGTK::ShowToolBar() {
	if (gtk_toolbar_get_n_items(GTK_TOOLBAR(PWidget(wToolBar))) < 1) {
		AddToolBar();
	}

	if (tbVisible) {
		if (toolbarDetachable == 1) {
			gtk_widget_show(GTK_WIDGET(PWidget(wToolBarBox)));
		} else {
			gtk_widget_show(GTK_WIDGET(PWidget(wToolBar)));
		}
	} else {
		if (toolbarDetachable == 1) {
			gtk_widget_hide(GTK_WIDGET(PWidget(wToolBarBox)));
		} else {
			gtk_widget_hide(GTK_WIDGET(PWidget(wToolBar)));
		}
	}
}

void SciTEGTK::ShowTabBar() {
	if (tabVisible && (!tabHideOne || buffers.length > 1) && buffers.size()>1) {
		gtk_widget_show(GTK_WIDGET(PWidget(wTabBar)));
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(wTabBar)));
	}
}

void SciTEGTK::ShowStatusBar() {
	if (sbVisible) {
		gtk_widget_show(GTK_WIDGET(PWidget(wStatusBar)));
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(wStatusBar)));
	}
}

void SciTEGTK::ActivateWindow(const char *timestamp) {
	char *end;
	// Reset errno so we can reliably test it
	errno = 0;
	const gulong ts = strtoul(timestamp, &end, 0);
	if (end != timestamp && errno == 0) {
		gtk_window_present_with_time(GTK_WINDOW(PWidget(wSciTE)), ts);
	} else {
		gtk_window_present(GTK_WINDOW(PWidget(wSciTE)));
	}
}

void SciTEGTK::CopyPath() {
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
		filePath.AsInternal(), -1);
}

void SciTEGTK::Command(unsigned long wParam, long) {
	const int cmdID = ControlIDOfCommand(wParam);
	switch (cmdID) {

	case IDM_FULLSCREEN:
		fullScreen = !fullScreen;
		{
			GdkWindow *parent_w = WindowFromWidget(PWidget(wSciTE));
			if (fullScreen)
				gdk_window_fullscreen(parent_w);
			else
				gdk_window_unfullscreen(parent_w);
		}
		SizeSubWindows();
		CheckMenus();
		break;

	// override base class
	case IDM_SPLITVERTICAL:
		splitVertical = !splitVertical;
		heightBar = 0;
		LayoutUI();
		CheckMenus();
		break;

	default:
		SciTEBase::MenuCommand(cmdID, menuSource);
		menuSource = 0;
	}
	// Changes to document produce SCN_UPDATEUI which updates the status
	// bar but not too frequently.
	UpdateStatusBar(true);
}

void SciTEGTK::ReadLocalization() {
	SciTEBase::ReadLocalization();
	std::string encoding = localiser.GetString("translation.encoding");
	if (encoding.length()) {
		GIConv iconvh = g_iconv_open("UTF-8", encoding.c_str());
		const char *key = NULL;
		const char *val = NULL;
		// Get encoding
		bool more = localiser.GetFirst(key, val);
		while (more) {
			char converted[1000];
			converted[0] = '\0';
			// g_iconv does not actually write to its input argument so safe to cast away const
			char *pin = const_cast<char *>(val);
			gsize inLeft = strlen(val);
			char *pout = converted;
			gsize outLeft = sizeof(converted);
			const gsize conversions = g_iconv(iconvh, &pin, &inLeft, &pout, &outLeft);
			if (conversions != static_cast<gsize>(-1)) {
				*pout = '\0';
				localiser.Set(key, converted);
			}
			more = localiser.GetNext(key, val);
		}
		g_iconv_close(iconvh);
	}
}

void SciTEGTK::ReadPropertiesInitial() {
	SciTEBase::ReadPropertiesInitial();
	ShowToolBar();
	ShowTabBar();
	ShowStatusBar();
}

void SciTEGTK::ReadProperties() {
	SciTEBase::ReadProperties();

	CallChildren(SA::Message::SetRectangularSelectionModifier,
		props.GetInt("rectangular.selection.modifier", SCMOD_ALT));

	CheckMenus();

	// Need this here to handle tabbar.hide.one properly
	ShowTabBar();
}

gboolean SciTEGTK::TimerTick(gpointer pSciTE) {
	SciTEGTK *scitew = static_cast<SciTEGTK *>(pSciTE);
	scitew->OnTimer();
	return TRUE;
}

void SciTEGTK::TimerStart(int mask) {
	const int maskNew = timerMask | mask;
	if (timerMask != maskNew) {
		if (timerMask == 0) {
			// Create a 1 second ticker
			timerID = g_timeout_add(1000, TimerTick, this);
		}
		timerMask = maskNew;
	}
}

void SciTEGTK::TimerEnd(int mask) {
	const int maskNew = timerMask & ~mask;
	if (timerMask != maskNew) {
		if (maskNew == 0) {
			g_source_remove(timerID);
			timerID = 0;
		}
		timerMask = maskNew;
	}
}

gboolean SciTEGTK::IdlerTick(gpointer pSciTE) {
	SciTEGTK *scitew = static_cast<SciTEGTK *>(pSciTE);
	scitew->OnIdle();
	return TRUE;
}

void SciTEGTK::SetIdler(bool on) {
	if (needIdle != on) {
		needIdle = on;
		if (needIdle) {
			idlerID = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
					IdlerTick, this, NULL);
		} else {
			if (idlerID) {
				g_source_remove(idlerID);
			}
		}
	}
}

void SciTEGTK::GetWindowPosition(int *left, int *top, int *width, int *height, int *maximize) {
	gtk_window_get_position(GTK_WINDOW(PWidget(wSciTE)), left, top);
	gtk_window_get_size(GTK_WINDOW(PWidget(wSciTE)), width, height);
	*maximize = (gdk_window_get_state(WindowFromWidget(PWidget(wSciTE))) & GDK_WINDOW_STATE_MAXIMIZED) != 0;
}

void SciTEGTK::SizeContentWindows() {
	gint max;
	g_object_get(G_OBJECT(splitPane), "max-position", &max, NULL);

	// This probably means the window hasn't been rendered yet
	if (max == 0)
		return;

	// Since SciTEBase::NormaliseSplit() doesn't use the actual Paned maximum
	// position, it may return an invalid heightOutput...
	if (heightOutput > max)
		heightOutput = max;

	gtk_paned_set_position(GTK_PANED(splitPane), max - heightOutput);

	// Make sure the focus is somewhere
	if (!userStrip.VisibleHasFocus()) {
		if (heightOutput == max) {
			WindowSetFocus(wOutput);
		} else if (heightOutput == 0 || (!wEditor.HasFocus() && !wOutput.HasFocus())) {
			WindowSetFocus(wEditor);
		}
	}
}

void SciTEGTK::SizeSubWindows() {
	SizeContentWindows();
}

// Find the menu item with a particular ID so that it can be enable, disabled, set or hidden.
GtkWidget *SciTEGTK::MenuItemFromAction(int itemID) {
	std::map<int, GtkWidget *>::iterator it;
	it = mapMenuItemFromId.find(itemID);
	if (it == mapMenuItemFromId.end())
		return 0;
	else
		return it->second;
}

void SciTEGTK::SetMenuItem(int, int, int itemID, const char *text, const char *mnemonic) {
	DestroyMenuItem(0, itemID);

	// On GTK the menuNumber and position are ignored as the menu item already exists and is in the right
	// place so only needs to be shown and have its text set.

	std::string itemText = GtkFromWinCaption(text);

	long keycode = 0;
	if (mnemonic && *mnemonic) {
		keycode = SciTEKeys::ParseKeyCode(mnemonic);
#if !GTK_CHECK_VERSION(3,6,0)
		if (keycode) {
			itemText += " ";
			itemText += mnemonic;
		}
#endif
		// the keycode could be used to make a custom accelerator table
		// but for now, the menu's item data is used instead for command
		// tools, and for other menu entries it is just discarded.
	}

	// Reorder shift and ctrl indicators for compatibility with other menus
	Substitute(itemText, "Ctrl+Shift+", "Shift+Ctrl+");

	GtkWidget *item = MenuItemFromAction(itemID);
	if (item) {
		GList *al = gtk_container_get_children(GTK_CONTAINER(item));
		for (unsigned int ii = 0; ii < g_list_length(al); ii++) {
			gpointer d = g_list_nth(al, ii);
			GtkWidget **w = static_cast<GtkWidget **>(d);
			gtk_label_set_text_with_mnemonic(GTK_LABEL(*w), itemText.c_str());
#if GTK_CHECK_VERSION(3,6,0)
			if (keycode) {
				gtk_accel_label_set_accel(GTK_ACCEL_LABEL(*w), keycode & 0xFFFF,
					static_cast<GdkModifierType>(keycode >> 16));
			}
#endif
		}
		g_list_free(al);
		gtk_widget_show(item);

		if (itemID >= IDM_TOOLS && itemID < IDM_TOOLS + toolMax) {
			// Stow the keycode for later retrieval.
			// Do this even if 0, in case the menu already existed (e.g. ModifyMenu)
			g_object_set_data(G_OBJECT(item), "key", GINT_TO_POINTER(static_cast<int>(keycode)));
		}
	}
}

void SciTEGTK::DestroyMenuItem(int, int itemID) {
	// On GTK menu items are just hidden rather than destroyed as they can not be recreated in the middle of a menu
	// The menuNumber is ignored as all menu items in GTK can be found from the root of the menu tree

	if (itemID) {
		GtkWidget *item = MenuItemFromAction(itemID);

		if (item) {
			gtk_widget_hide(item);
			g_object_set_data(G_OBJECT(item), "key", 0);
		}
	}
}

void SciTEGTK::CheckAMenuItem(int wIDCheckItem, bool val) {
	GtkWidget *item = MenuItemFromAction(wIDCheckItem);
	allowMenuActions = false;
	if (item)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), val ? TRUE : FALSE);
	allowMenuActions = true;
}

void SciTEGTK::EnableAMenuItem(int wIDCheckItem, bool val) {
	GtkWidget *item = MenuItemFromAction(wIDCheckItem);
	if (item) {
		if (GTK_IS_WIDGET(item))
			gtk_widget_set_sensitive(item, val);

	}
}

void SciTEGTK::CheckMenusClipboard() {
	if (StripHasFocus()) {
		EnableAMenuItem(IDM_CUT, false);
		EnableAMenuItem(IDM_COPY, false);
		EnableAMenuItem(IDM_CLEAR, false);
		EnableAMenuItem(IDM_PASTE, false);
		EnableAMenuItem(IDM_SELECTALL, false);
	} else {
		SciTEBase::CheckMenusClipboard();
	}
}

void SciTEGTK::CheckMenus() {
	SciTEBase::CheckMenus();

	CheckAMenuItem(IDM_EOL_CRLF, wEditor.EOLMode() == SA::EndOfLine::CrLf);
	CheckAMenuItem(IDM_EOL_CR, wEditor.EOLMode() == SA::EndOfLine::Cr);
	CheckAMenuItem(IDM_EOL_LF, wEditor.EOLMode() == SA::EndOfLine::Lf);

	CheckAMenuItem(IDM_ENCODING_DEFAULT, CurrentBuffer()->unicodeMode == UniMode::uni8Bit);
	CheckAMenuItem(IDM_ENCODING_UCS2BE, CurrentBuffer()->unicodeMode == UniMode::uni16BE);
	CheckAMenuItem(IDM_ENCODING_UCS2LE, CurrentBuffer()->unicodeMode == UniMode::uni16LE);
	CheckAMenuItem(IDM_ENCODING_UTF8, CurrentBuffer()->unicodeMode == UniMode::utf8);
	CheckAMenuItem(IDM_ENCODING_UCOOKIE, CurrentBuffer()->unicodeMode == UniMode::cookie);

	CheckAMenuItem(IDM_VIEWSTATUSBAR, sbVisible);
	CheckAMenuItem(IDM_VIEWTABBAR, tabVisible);

	EnableAMenuItem(IDM_FINDINFILES, !jobQueue.IsExecuting());

	if (btnBuild) {
		gtk_widget_set_sensitive(btnBuild, !jobQueue.IsExecuting());
		gtk_widget_set_sensitive(btnCompile, !jobQueue.IsExecuting());
		gtk_widget_set_sensitive(btnStop, jobQueue.IsExecuting());
	}
}

/**
 * Replace any %xx escapes by their single-character equivalent.
 */
static void unquote(char *s) {
	char *o = s;
	while (*s) {
		if ((*s == '%') && s[1] && s[2]) {
			*o = IntFromHexDigit(s[1]) * 16 + IntFromHexDigit(s[2]);
			s += 2;
		} else {
			*o = *s;
		}
		o++;
		s++;
	}
	*o = '\0';
}

/**
 * Open a list of URIs each terminated by "\r\n".
 * Only "file:" URIs currently understood.
 * In KDE 4, the last URI is not terminated by "\r\n"!
 */
void SciTEGTK::OpenUriList(const char *list) {
	if (list && *list) {
		std::vector<char> uriList(list, list+strlen(list) + 1);
		char *uri = &uriList[0];
		if (uri) {
			char *lastenduri = uri + strlen(uri);
			while (uri < lastenduri) {
				char *enduri = strchr(uri, '\r');
				if (enduri == NULL)
					enduri = lastenduri;	// if last URI has no "\r\n".
				*enduri = '\0';
				if (isprefix(uri, "file:")) {
					uri += strlen("file:");
					if (isprefix(uri, "///")) {
						uri += 2;	// There can be an optional // before the file path that starts with /
					}

					unquote(uri);
					Open(uri);
				} else {
					GUI::gui_string msg = LocaliseMessage("URI '^0' not understood.", uri);
					WindowMessageBox(wSciTE, msg);
				}

				uri = enduri + 1;
				if (*uri == '\n')
					uri++;
			}
		}
	}
}

#if GTK_CHECK_VERSION(3,10,0)
#define SCITE_STOCK_CANCEL "_Cancel"
#define SCITE_STOCK_OPEN "_Open"
#define SCITE_STOCK_SAVE "_Save"
#define SCITE_STOCK_OK "_OK"
#else
#define SCITE_STOCK_CANCEL GTK_STOCK_CANCEL
#define SCITE_STOCK_OPEN GTK_STOCK_OPEN
#define SCITE_STOCK_SAVE GTK_STOCK_SAVE
#define SCITE_STOCK_OK GTK_STOCK_OK
#endif

bool SciTEGTK::OpenDialog(const FilePath &directory, const char *filesFilter) {
	bool canceled = true;
	if (!dlgFileSelector.Created()) {
		GtkWidget *dlg = gtk_file_chooser_dialog_new(
					localiser.Text("Open File").c_str(),
				      GTK_WINDOW(wSciTE.GetID()),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      SCITE_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      SCITE_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dlg), TRUE);
		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), directory.AsInternal());

		std::string openFilter = filesFilter;
		if (openFilter.length()) {
			std::replace(openFilter.begin(), openFilter.end(), '|', '\0');
			size_t start = 0;
			while (start < openFilter.length()) {
				const char *filterName = openFilter.c_str() + start;
				GUI::gui_string localised = localiser.Text(filterName, false);
				if (localised.length()) {
					openFilter.erase(start, strlen(filterName));
					openFilter.insert(start, localised.c_str());
				}
				if (openFilter.c_str()[start] == '#') {
					start += strlen(openFilter.c_str() + start) + 1;
				} else {
					GtkFileFilter *fileFilter = gtk_file_filter_new();
					gtk_file_filter_set_name(fileFilter, openFilter.c_str() + start);
					start += strlen(openFilter.c_str() + start) + 1;
					std::string oneSet(openFilter.c_str() + start);
					std::replace(oneSet.begin(), oneSet.end(), ';', '\0');
					size_t item = 0;
					while (item < oneSet.length()) {
						gtk_file_filter_add_pattern(fileFilter, oneSet.c_str() + item);
						item += strlen(oneSet.c_str() + item) + 1;
					}
					gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), fileFilter);
				}
				start += strlen(openFilter.c_str() + start) + 1;
			}
		}

		gtk_window_set_default_size(GTK_WINDOW(dlg), fileSelectorWidth, fileSelectorHeight);
		if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
			GSList *names = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dlg));
			GSList *nameCurrent = names;
			while (nameCurrent) {
				char *filename = static_cast<char *>(nameCurrent->data);
				Open(filename);
				g_free(filename);
				nameCurrent = g_slist_next(nameCurrent);
			}
			g_slist_free(names);
			canceled = false;
		}
		gtk_widget_destroy(dlg);
	}
	return !canceled;
}

bool SciTEGTK::HandleSaveAs(const char *savePath) {
	switch (saveFormat) {
	case sfCopy:
		SaveBuffer(savePath, sfNone);
		break;
	case sfHTML:
		SaveToHTML(savePath);
		break;
	case sfRTF:
		SaveToRTF(savePath);
		break;
	case sfPDF:
		SaveToPDF(savePath);
		break;
	case sfTEX:
		SaveToTEX(savePath);
		break;
	case sfXML:
		SaveToXML(savePath);
		break;
	default: {
			/* Checking that no other buffer refers to the same filename */
			FilePath destFile(savePath);
			return SaveIfNotOpen(destFile, true);
		}
	}
	dlgFileSelector.Destroy();
	return true;
}

bool SciTEGTK::SaveAsXXX(FileFormat fmt, const char *title, const char *ext) {
	bool saved = false;
	saveFormat = fmt;
	if (!dlgFileSelector.Created()) {
		GtkWidget *dlg = gtk_file_chooser_dialog_new(
					localiser.Text(title).c_str(),
				      GTK_WINDOW(wSciTE.GetID()),
				      GTK_FILE_CHOOSER_ACTION_SAVE,
				      SCITE_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      SCITE_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				      NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
		FilePath savePath = SaveName(ext);
		if (ext) {
			GtkFileFilter *fileFilter = gtk_file_filter_new();
			gtk_file_filter_set_name(fileFilter, ext+1);
			std::string filterSTR = "*";
			filterSTR += ext;
			gtk_file_filter_add_pattern(fileFilter, filterSTR.c_str());
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), fileFilter);
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), savePath.Directory().AsInternal());
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), savePath.Name().AsInternal());
		} else if (savePath.IsUntitled()) { // saving 'untitled'
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), savePath.Directory().AsInternal());
		} else {
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), filePath.Directory().AsInternal());
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), savePath.Name().AsInternal());
		}
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

		if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
			char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
			gtk_widget_destroy(dlg);
			saved = HandleSaveAs(filename);
			g_free(filename);
		} else {
			gtk_widget_destroy(dlg);
		}
	}
	return saved;
}

bool SciTEGTK::SaveAsDialog() {
	return SaveAsXXX(sfSource, "Save File As");
}

void SciTEGTK::SaveACopy() {
	SaveAsXXX(sfCopy, "Save a Copy");
}

void SciTEGTK::SaveAsHTML() {
	SaveAsXXX(sfHTML, "Export File As HTML", ".html");
}

void SciTEGTK::SaveAsRTF() {
	SaveAsXXX(sfRTF, "Export File As RTF", ".rtf");
}

void SciTEGTK::SaveAsPDF() {
	SaveAsXXX(sfPDF, "Export File As PDF", ".pdf");
}

void SciTEGTK::SaveAsTEX() {
	SaveAsXXX(sfTEX, "Export File As LaTeX", ".tex");
}

void SciTEGTK::SaveAsXML() {
	SaveAsXXX(sfXML, "Export File As XML", ".xml");
}

void SciTEGTK::LoadSessionDialog() {
	if (!dlgFileSelector.Created()) {
		GtkWidget *dlg = gtk_file_chooser_dialog_new(
					localiser.Text("Load Session").c_str(),
				      GTK_WINDOW(wSciTE.GetID()),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      SCITE_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      SCITE_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

		GtkFileFilter *fileFilter = gtk_file_filter_new();
		gtk_file_filter_set_name(fileFilter, "session");
		gtk_file_filter_add_pattern(fileFilter, "*.session");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), fileFilter);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), filePath.Directory().AsInternal());
		gtk_window_set_default_size(GTK_WINDOW(dlg), fileSelectorWidth, fileSelectorHeight);
		if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
			char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));

			LoadSessionFile(filename);
			RestoreSession();
			g_free(filename);
		}
		gtk_widget_destroy(dlg);
	}
}

void SciTEGTK::SaveSessionDialog() {
	if (!dlgFileSelector.Created()) {
		GtkWidget *dlg = gtk_file_chooser_dialog_new(
					localiser.Text("Save Session").c_str(),
				      GTK_WINDOW(wSciTE.GetID()),
				      GTK_FILE_CHOOSER_ACTION_SAVE,
				      SCITE_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      SCITE_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				      NULL);

		GtkFileFilter *fileFilter = gtk_file_filter_new();
		gtk_file_filter_set_name(fileFilter, "session");
		gtk_file_filter_add_pattern(fileFilter, "*.session");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), fileFilter);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), filePath.Directory().AsInternal());
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), "SciTE.session");
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);
		if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
			char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));

			SaveSessionFile(filename);
			g_free(filename);
		}
		gtk_widget_destroy(dlg);
	}
}

static PangoLayout *PangoLayoutFromStyleDefinition(GtkPrintContext *context, const StyleDefinition &sd) {
	PangoLayout *layout = gtk_print_context_create_pango_layout(context);
	if (layout) {
		pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
		PangoFontDescription *pfd = pango_font_description_new();
		if (pfd) {
			const char *fontName = "Sans";
			if (sd.specified & StyleDefinition::sdFont)
				fontName = sd.font.c_str();
			if (fontName[0] == '!')
				fontName++;
			pango_font_description_set_family(pfd, fontName);
			pango_font_description_set_size(pfd, PANGO_SCALE * (
				(sd.specified & StyleDefinition::sdSize) ? sd.sizeFractional : 9.0));
			pango_font_description_set_weight(pfd, static_cast<PangoWeight>(sd.weight));
			pango_font_description_set_style(pfd, sd.italics ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
			pango_layout_set_font_description(layout, pfd);
			pango_font_description_free(pfd);
		}
	}
	return layout;
}

void SciTEGTK::SetupFormat(Sci_RangeToFormat &frPrint, GtkPrintContext *context) {
	cairo_t *cr = gtk_print_context_get_cairo_context(context);
	frPrint.hdc = cr;
	frPrint.hdcTarget = cr;

	const gdouble width = gtk_print_context_get_width(context);
	const gdouble height = gtk_print_context_get_height(context);

	frPrint.rc.left = 0;
	frPrint.rc.top = 0;
	frPrint.rc.right = width;
	frPrint.rc.bottom = height;

	gdouble marginLeft = 0;
	gdouble marginTop = 0;
	gdouble marginRight = 0;
	gdouble marginBottom = 0;
	if (gtk_print_context_get_hard_margins(context,
		&marginTop, &marginBottom, &marginLeft, &marginRight)) {
		frPrint.rc.left += marginLeft;
		frPrint.rc.top += marginTop;
		frPrint.rc.right -= marginRight;
		frPrint.rc.bottom -= marginBottom;
	}

	frPrint.rcPage.left = 0;
	frPrint.rcPage.top = 0;
	frPrint.rcPage.right = width;
	frPrint.rcPage.bottom = height;

	std::string headerFormat = props.GetString("print.header.format");
	if (headerFormat.size()) {
		StyleDefinition sdHeader(props.GetString("print.header.style").c_str());
		PangoLayout *layout = PangoLayoutFromStyleDefinition(context, sdHeader);
		pango_layout_set_text(layout, "Xg", -1);
		gint layoutHeight;
		pango_layout_get_size(layout, NULL, &layoutHeight);
		frPrint.rc.top += doubleFromPangoUnits(layoutHeight) * 1.5;
		g_object_unref(layout);
	}

	std::string footerFormat = props.GetString("print.footer.format");
	if (footerFormat.size()) {
		StyleDefinition sdFooter(props.GetString("print.footer.style").c_str());
		PangoLayout *layout = PangoLayoutFromStyleDefinition(context, sdFooter);
		pango_layout_set_text(layout, "Xg", -1);
		gint layoutHeight;
		pango_layout_get_size(layout, NULL, &layoutHeight);
		frPrint.rc.bottom -= doubleFromPangoUnits(layoutHeight) * 1.5;
		g_object_unref(layout);
	}
}

void SciTEGTK::BeginPrintThis(GtkPrintOperation *operation, GtkPrintContext *context) {
	pageStarts.clear();
	Sci_RangeToFormat frPrint;
	SetupFormat(frPrint, context) ;

	const SA::Position lengthDoc = wEditor.Length();
	SA::Position lengthPrinted = 0;
	while (lengthPrinted < lengthDoc) {
		pageStarts.push_back(lengthPrinted);
		frPrint.chrg.cpMin = lengthPrinted;
		frPrint.chrg.cpMax = lengthDoc;
		lengthPrinted = wEditor.FormatRange(false, &frPrint);
	}
	pageStarts.push_back(lengthPrinted);

	gtk_print_operation_set_n_pages(operation, pageStarts.size()-1);
}

void SciTEGTK::BeginPrint(GtkPrintOperation *operation, GtkPrintContext *context, SciTEGTK *scitew) {
	scitew->BeginPrintThis(operation, context);
}

void SciTEGTK::DrawPageThis(GtkPrintOperation * /* operation */, GtkPrintContext *context, gint page_nr) {
	Sci_RangeToFormat frPrint;
	SetupFormat(frPrint, context) ;
	cairo_t *cr = static_cast<cairo_t *>(frPrint.hdc);

	PropSetFile propsPrint;
	propsPrint.superPS = &props;
	SetFileProperties(propsPrint);
	char pageString[32];
	sprintf(pageString, "%0d", page_nr + 1);
	propsPrint.Set("CurrentPage", pageString);

	std::string headerFormat = props.GetString("print.header.format");
	if (headerFormat.size()) {
		StyleDefinition sdHeader(props.GetString("print.header.style").c_str());

		PangoLayout *layout = PangoLayoutFromStyleDefinition(context, sdHeader);

		SetCairoColour(cr, sdHeader.Fore());

		pango_layout_set_text(layout, propsPrint.GetExpandedString("print.header.format").c_str(), -1);

		gint layout_height;
		pango_layout_get_size(layout, NULL, &layout_height);
		const gdouble text_height = doubleFromPangoUnits(layout_height);
		cairo_move_to(cr, frPrint.rc.left, frPrint.rc.top - text_height * 1.5);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);

		cairo_move_to(cr, frPrint.rc.left, frPrint.rc.top - text_height * 0.25);
		cairo_line_to(cr, frPrint.rc.right, frPrint.rc.top - text_height * 0.25);
		cairo_stroke(cr);
	}

	std::string footerFormat = props.GetString("print.footer.format");
	if (footerFormat.size()) {
		StyleDefinition sdFooter(props.GetString("print.footer.style").c_str());

		PangoLayout *layout = PangoLayoutFromStyleDefinition(context, sdFooter);

		SetCairoColour(cr, sdFooter.Fore());

		pango_layout_set_text(layout, propsPrint.GetExpandedString("print.footer.format").c_str(), -1);

		gint layout_height;
		pango_layout_get_size(layout, NULL, &layout_height);
		const gdouble text_height = doubleFromPangoUnits(layout_height);
		cairo_move_to(cr, frPrint.rc.left, frPrint.rc.bottom + text_height * 0.5);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);

		cairo_move_to(cr, frPrint.rc.left, frPrint.rc.bottom + text_height * 0.25);
		cairo_line_to(cr, frPrint.rc.right, frPrint.rc.bottom + text_height * 0.25);
		cairo_stroke(cr);
	}

	const SA::Position lengthDoc = wEditor.Length();
	frPrint.chrg.cpMin = pageStarts[page_nr];
	frPrint.chrg.cpMax = pageStarts[page_nr+1];
	if (frPrint.chrg.cpMax < lengthDoc)
		frPrint.chrg.cpMax--;

	wEditor.FormatRange(true, &frPrint);
}

void SciTEGTK::DrawPage(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, SciTEGTK *scitew) {
	scitew->DrawPageThis(operation, context, page_nr);
}

void SciTEGTK::Print(bool) {
	RemoveFindMarks();
	SelectionIntoProperties();
	// Printing through the GTK API
	GtkPrintOperation *printOp = gtk_print_operation_new();
	gtk_print_operation_set_job_name(printOp, gtk_window_get_title(GTK_WINDOW(PWidget(wSciTE))));

	if (printSettings != NULL)
		gtk_print_operation_set_print_settings(printOp, printSettings);
	if (pageSetup != NULL)
		gtk_print_operation_set_default_page_setup(printOp, pageSetup);

	g_signal_connect(printOp, "begin_print", G_CALLBACK(BeginPrint), this);
	g_signal_connect(printOp, "draw_page", G_CALLBACK(DrawPage), this);

	const GtkPrintOperationResult res = gtk_print_operation_run(
		printOp, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
		GTK_WINDOW(PWidget(wSciTE)), NULL);

	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (printSettings != NULL)
			g_object_unref(printSettings);
		printSettings = gtk_print_operation_get_print_settings(printOp);
		g_object_ref(printSettings);
	}

	g_object_unref(printOp);
}

void SciTEGTK::PrintSetup() {
	if (printSettings == NULL)
		printSettings = gtk_print_settings_new();

	GtkPageSetup *newPageSetup = gtk_print_run_page_setup_dialog(
		GTK_WINDOW(PWidget(wSciTE)), pageSetup, printSettings);

	if (pageSetup)
		g_object_unref(pageSetup);

	pageSetup = newPageSetup;
}

std::string SciTEGTK::GetRangeInUIEncoding(GUI::ScintillaWindow &win, SA::Span span) {
	const SA::Position len = span.Length();
	if (len == 0)
		return std::string();
	std::string allocation(len * 3 + 1, 0);
	win.SetTarget(span);
	const SA::Position byteLength = win.TargetAsUTF8(&allocation[0]);
	std::string sel(allocation, 0, byteLength);
	return sel;
}

void SciTEGTK::Find() {
	if (dlgFindReplace.Created()) {
		dlgFindReplace.Present();
		return;
	}
	SelectionIntoFind();
	if (props.GetInt("find.use.strip")) {
		CloseOtherFinders(IDM_FIND);
		findStrip.SetIncrementalBehaviour(props.GetInt("find.strip.incremental"));
		findStrip.Show(props.GetInt("strip.button.height", -1));
	} else {
		if (findStrip.visible || replaceStrip.visible || filterStrip.visible)
			return;
		FindReplace(false);
	}
}

void SetFocus(GUI::ScintillaWindow &w) {
	w.GrabFocus();
}

void SciTEGTK::UIClosed() {
	SciTEBase::UIClosed();
	SetFocus(wEditor);
}

void SciTEGTK::UIHasFocus() {
	CheckMenusClipboard();
}

GtkWidget *SciTEGTK::TranslatedLabel(const char *original) {
	GUI::gui_string text = localiser.Text(original);
	return gtk_label_new_with_mnemonic(text.c_str());
}

static void FillComboFromProps(WComboBoxEntry *combo, const PropSetFile &props) {
	const char *key;
	const char *val;
	combo->ClearList();

	bool more = props.GetFirst(key, val);
	while (more) {
		combo->AppendText(key);
		more = props.GetNext(key, val);
	}
}

static void FillComboFromMemory(WComboBoxEntry *combo, const ComboMemory &mem, bool useTop = false) {
	combo->ClearList();
	for (size_t i = 0; i < mem.Length(); i++) {
		combo->AppendText(mem.At(i).c_str());
	}
	if (useTop && mem.Length() > 0) {
		combo->SetText(mem.At(0));
	}
}

std::string SciTEGTK::EncodeString(const std::string &s) {
	wEditor.SetLengthForEncode(s.length());
	const SA::Position len = wEditor.EncodedFromUTF8(s.c_str(), nullptr);
	std::vector<char> ret(len+1);
	wEditor.EncodedFromUTF8(s.c_str(), &ret[0]);
	return std::string(&ret[0], len);
}

void DialogFindReplace::GrabFields() {
	pSearcher->SetFind(wComboFind.Text());
	if (wComboReplace) {
		pSearcher->SetReplace(wComboReplace.Text());
	}
	pSearcher->wholeWord = toggleWord.Active();
	pSearcher->matchCase = toggleCase.Active();
	pSearcher->regExp = toggleRegExp.Active();
	pSearcher->wrapFind = toggleWrap.Active();
	pSearcher->unSlash = toggleUnSlash.Active();
	if (toggleReverse) {
		pSearcher->reverseFind = toggleReverse.Active();
	}
}

void DialogFindReplace::FillFields() {
	wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
	if (wComboReplace) {
		wComboReplace.FillFromMemory(pSearcher->memReplaces.AsVector());
	}
	toggleWord.SetActive(pSearcher->wholeWord);
	toggleCase.SetActive(pSearcher->matchCase);
	toggleRegExp.SetActive(pSearcher->regExp);
	toggleWrap.SetActive(pSearcher->wrapFind);
	toggleUnSlash.SetActive(pSearcher->unSlash);
	if (toggleReverse) {
		toggleReverse.SetActive(pSearcher->reverseFind);
	}
}

void DialogFindReplace::FillCombosInDialog() {
	wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
	if (pSearcher->replacing)
		wComboReplace.FillFromMemory(pSearcher->memReplaces.AsVector());
}

void SciTEGTK::FindReplaceGrabFields() {
	dlgFindReplace.GrabFields();
}

void SciTEGTK::FRFindCmd() {
	FindReplaceGrabFields();
	const bool isFindDialog = !dlgFindReplace.wComboReplace;
	bool found = false;
	if (findWhat[0]) {
		found = FindNext(isFindDialog && reverseFind) >= 0;
	}
	if (isFindDialog && ShouldClose(found))
		dlgFindReplace.Destroy();
	else
		FillCombos(dlgFindReplace);
}

void SciTEGTK::FRReplaceCmd() {
	FindReplaceGrabFields();
	ReplaceOnce();
	FillCombos(dlgFindReplace);
}

void SciTEGTK::FRReplaceAllCmd() {
	FindReplaceGrabFields();
	if (findWhat[0]) {
		ReplaceAll(false);
	}
	FillCombos(dlgFindReplace);
}

void SciTEGTK::FRReplaceInSelectionCmd() {
	FindReplaceGrabFields();
	if (findWhat[0]) {
		ReplaceAll(true);
	}
	FillCombos(dlgFindReplace);
}

void SciTEGTK::FRReplaceInBuffersCmd() {
	FindReplaceGrabFields();
	if (findWhat[0]) {
		ReplaceInBuffers();
	}
	FillCombos(dlgFindReplace);
}

void SciTEGTK::FRMarkAllCmd() {
	FindReplaceGrabFields();
	MarkAll(MarkPurpose::withBookMarks);
	const bool found = FindNext(reverseFind) >= 0;
	if (ShouldClose(found))
		dlgFindReplace.Destroy();
	else
		FillCombos(dlgFindReplace);
}

void SciTEGTK::FillCombos(FindDialog &dlg)  {
	dlg.FillCombosInDialog();
}

void SciTEGTK::FillCombosForGrep()  {
	FillComboFromMemory(&dlgFindInFiles.wComboFiles, memFiles, true);
	FillComboFromMemory(&dlgFindInFiles.comboDir, memDirectory, true);
}

void DialogFindInFiles::GrabFields() {
	pSearcher->SetFind(wComboFindInFiles.Text());
	if (toggleWord.Sensitive())
		pSearcher->wholeWord = toggleWord.Active();
	if (toggleCase.Sensitive())
		pSearcher->matchCase = toggleCase.Active();
}

void DialogFindInFiles::FillFields() {
	wComboFindInFiles.FillFromMemory(pSearcher->memFinds.AsVector());
	if (toggleWord.Sensitive())
		toggleWord.SetActive(pSearcher->wholeWord);
	if (toggleCase.Sensitive())
		toggleCase.SetActive(pSearcher->matchCase);
}

void DialogFindInFiles::FillCombosInDialog() {
	wComboFindInFiles.FillFromMemory(pSearcher->memFinds.AsVector());
}

void SciTEGTK::FindInFilesCmd() {
	dlgFindInFiles.GrabFields();

	const char *dirEntry = dlgFindInFiles.comboDir.Text();
	props.Set("find.directory", dirEntry);
	memDirectory.Insert(dirEntry);

	const char *filesEntry = dlgFindInFiles.wComboFiles.Text();
	props.Set("find.files", filesEntry);
	memFiles.Insert(filesEntry);

	SetFindInFilesOptions();

	if (props.GetInt("find.in.files.close.on.find", 1)) {
		dlgFindInFiles.Destroy();
	}

	SelectionIntoProperties();
	std::string findCommand = props.GetNewExpandString("find.command");
	if (findCommand == "") {
		findCommand = sciteExecutable.AsInternal();
		findCommand += " -grep ";
		findCommand += (wholeWord ? "w" : "~");
		findCommand += (matchCase ? "c" : "~");
		findCommand += props.GetInt("find.in.dot") ? "d" : "~";
		findCommand += props.GetInt("find.in.binary") ? "b" : "~";
		findCommand += " \"";
		findCommand += props.GetString("find.files");
		findCommand += "\" \"";
		findCommand += props.GetString("find.exclude");
		findCommand += "\" \"";
		std::string quotedForm = Slash(props.GetString("find.what"), true);
		findCommand += quotedForm;
		findCommand += "\"";
	}
	AddCommand(findCommand, props.GetString("find.directory"), JobSubsystem::cli);
	if (jobQueue.HasCommandToRun())
		Execute();
	if (dlgFindInFiles.Created()) {
		FillCombos(dlgFindInFiles);
		FillCombosForGrep();
	}
}

void SciTEGTK::FindInFilesDotDot() {
	FilePath findInDir(dlgFindInFiles.comboDir.Text());
	gtk_entry_set_text(dlgFindInFiles.comboDir.Entry(), findInDir.Directory().AsInternal());
}

void SciTEGTK::FindInFilesBrowse() {
	FilePath findInDir(dlgFindInFiles.comboDir.Text());
	GtkWidget *dialog = gtk_file_chooser_dialog_new(
							localiser.Text("Select a folder to search from").c_str(),
							GTK_WINDOW(dlgFindInFiles.GetID()), // parent_window,
							GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
							SCITE_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							SCITE_STOCK_OK, GTK_RESPONSE_ACCEPT,
							NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), findInDir.AsInternal());

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		dlgFindInFiles.comboDir.SetText(filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);
}

namespace {

const char *textMarkAll = "_Mark All";

const char *textFind = "F_ind";
const char *textReplace = "_Replace";
const char *textReplaceAll = "Replace _All";
const char *textInSelection = "In _Selection";
const char *textReplaceInBuffers = "Replace In _Buffers";
const char *textClose = "_Close";

const SearchOption toggles[] = {
	{"Match _whole word only", IDM_WHOLEWORD, IDWHOLEWORD},
	{"_Case sensitive", IDM_MATCHCASE, IDMATCHCASE},
	{"Regular _expression", IDM_REGEXP, IDREGEXP},
	{"Transform _backslash expressions", IDM_UNSLASH, IDUNSLASH},
	{"Wrap ar_ound", IDM_WRAPAROUND, IDWRAP},
	{"_Up", IDM_DIRECTIONUP, IDDIRECTIONUP},
};

}

void SciTEGTK::FindInFilesResponse(int responseID) {
	switch (responseID) {
		case GTK_RESPONSE_OK:
			FindInFilesCmd() ;
			break;

		case GTK_RESPONSE_CANCEL:
			dlgFindInFiles.Destroy();
			break;
	}
}

void SciTEGTK::FindInFiles() {
	dlgFindInFiles.SetSearcher(this);

	SelectionIntoFind();
	props.Set("find.what", findWhat);

	FilePath findInDir = filePath.Directory().AbsolutePath();
	std::string directory = props.GetString("find.in.directory");
	if (directory.length()) {
		findInDir = FilePath(directory.c_str());
	}
	props.SetPath("find.directory", findInDir);

	dlgFindInFiles.Create(localiser.Text("Find in Files"));

	WTable table(4, 5);
	table.PackInto(GTK_BOX(dlgFindInFiles.ContentArea()));

	WStatic labelFind;
	labelFind.Create(localiser.Text("Fi_nd what:"));
	table.Label(labelFind);

	dlgFindInFiles.wComboFindInFiles.Create();

	table.Add(dlgFindInFiles.wComboFindInFiles, 4, true);

	gtk_entry_set_text(dlgFindInFiles.wComboFindInFiles.Entry(), findWhat.c_str());
	dlgFindInFiles.wComboFindInFiles.ActivatesDefault();
	labelFind.SetMnemonicFor(dlgFindInFiles.wComboFindInFiles);

	WStatic labelFiles;
	labelFiles.Create(localiser.Text("_Files:"));
	table.Label(labelFiles);

	dlgFindInFiles.wComboFiles.Create();
	dlgFindInFiles.comboDir.Create();

	FillCombosForGrep();

	table.Add(dlgFindInFiles.wComboFiles, 4, true);
	dlgFindInFiles.wComboFiles.ActivatesDefault();
	labelFiles.SetMnemonicFor(dlgFindInFiles.wComboFiles);

	WStatic labelDirectory;
	labelDirectory.Create(localiser.Text("_Directory:"));
	table.Label(labelDirectory);

	table.Add(dlgFindInFiles.comboDir, 2, true);

	gtk_entry_set_text(dlgFindInFiles.comboDir.Entry(), findInDir.AsInternal());
	// Make a little wider than would happen automatically to show realistic paths
	gtk_entry_set_width_chars(dlgFindInFiles.comboDir.Entry(), 40);
	dlgFindInFiles.comboDir.ActivatesDefault();
	labelDirectory.SetMnemonicFor(dlgFindInFiles.comboDir);

	Signal<&SciTEGTK::FindInFilesDotDot> sigDotDot;
	dlgFindInFiles.btnDotDot.Create(localiser.Text("_.."), G_CALLBACK(sigDotDot.Function), this);
	table.Add(dlgFindInFiles.btnDotDot);

	Signal<&SciTEGTK::FindInFilesBrowse> sigBrowse;
	dlgFindInFiles.btnBrowse.Create(localiser.Text("_Browse..."), G_CALLBACK(sigBrowse.Function), this);
	table.Add(dlgFindInFiles.btnBrowse);

	table.Add();	// Space

	// Whole Word
	dlgFindInFiles.toggleWord.Create(localiser.Text(toggles[SearchOption::tWord].label));
	table.Add(dlgFindInFiles.toggleWord, 1, true, 3, 0);

	// Case Sensitive
	dlgFindInFiles.toggleCase.Create(localiser.Text(toggles[SearchOption::tCase].label));
	table.Add(dlgFindInFiles.toggleCase, 1, true, 3, 0);

	AttachResponse<&SciTEGTK::FindInFilesResponse>(PWidget(dlgFindInFiles), this);
	dlgFindInFiles.ResponseButton(localiser.Text("_Cancel"), GTK_RESPONSE_CANCEL);
	dlgFindInFiles.ResponseButton(localiser.Text("F_ind"), GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(PWidget(dlgFindInFiles)), GTK_RESPONSE_OK);

	dlgFindInFiles.FillFields();

	gtk_widget_grab_focus(GTK_WIDGET(dlgFindInFiles.wComboFindInFiles.Entry()));

	dlgFindInFiles.Display(PWidget(wSciTE));
}

void SciTEGTK::Replace() {
	if (dlgFindReplace.Created()) {
		dlgFindReplace.Present();
		return;
	}
	SelectionIntoFind();
	if (props.GetInt("replace.use.strip")) {
		CloseOtherFinders(IDM_REPLACE);
		replaceStrip.SetIncrementalBehaviour(props.GetInt("replace.strip.incremental"));
		replaceStrip.Show(props.GetInt("strip.button.height", -1));
	} else {
		if (findStrip.visible || replaceStrip.visible || filterStrip.visible)
			return;
		FindReplace(true);
	}
}

void SciTEGTK::ResetExecution() {
	icmd = 0;
	jobQueue.SetExecuting(false);
	if (needReadProperties)
		ReadProperties();
	CheckReload();
	CheckMenus();
	jobQueue.ClearJobs();
}

void SciTEGTK::ExecuteNext() {
	icmd++;
	if (icmd < jobQueue.commandCurrent && icmd < jobQueue.commandMax) {
		Execute();
	} else {
		ResetExecution();
	}
}

void SciTEGTK::ContinueExecute(int fromPoll) {
	char buf[8192];
	int count = read(fdFIFO, buf, sizeof(buf) - 1);
	if (count > 0) {
		buf[count] = '\0';
		if (!(lastFlags & jobQuiet)) {
			OutputAppendString(buf);
		}
		lastOutput += buf;
	} else if (count == 0) {
		std::string sExitMessage = StdStringFromInteger(WEXITSTATUS(exitStatus));
		sExitMessage.insert(0, ">Exit code: ");
		if (WIFSIGNALED(exitStatus)) {
			std::string sSignal = StdStringFromInteger(WTERMSIG(exitStatus));
			sSignal.insert(0, " Signal: ");
			sExitMessage += sSignal;
		}
		if (jobQueue.TimeCommands()) {
			sExitMessage += "    Time: ";
			sExitMessage += StdStringFromDouble(commandTime.Duration(), 3);
		}
		if ((lastFlags & jobRepSelYes)
			|| ((lastFlags & jobRepSelAuto) && !exitStatus)) {
			const int cpMin = wEditor.Send(SCI_GETSELECTIONSTART, 0, 0);
			wEditor.Send(SCI_REPLACESEL,0,(sptr_t)(lastOutput.c_str()));
			wEditor.Send(SCI_SETSEL, cpMin, cpMin+lastOutput.length());
		}
		sExitMessage.append("\n");
		OutputAppendString(sExitMessage.c_str());
		// Move selection back to beginning of this run so that F4 will go
		// to first error of this run.
		if ((scrollOutput == 1) && returnOutputToCommand)
			wOutput.Send(SCI_GOTOPOS, originalEnd);
		returnOutputToCommand = true;
		g_source_remove(inputHandle);
		inputHandle = 0;
		g_io_channel_unref(inputChannel);
		inputChannel = 0;
		g_source_remove(pollID);
		pollID = 0;
		close(fdFIFO);
		fdFIFO = 0;
		pidShell = 0;
		triedKill = false;
		if (WEXITSTATUS(exitStatus))
			ResetExecution();
		else
			ExecuteNext();
	} else { // count < 0
		// The FIFO is not ready - expected when called from polling callback.
		if (!fromPoll) {
			OutputAppendString(">End Bad\n");
		}
	}
}

void SciTEGTK::ShowBackgroundProgress(const GUI::gui_string &explanation, size_t size, size_t progress) {
	backgroundStrip.visible = !explanation.empty();
	if (backgroundStrip.visible) {
		backgroundStrip.Show(0);
		backgroundStrip.SetProgress(explanation, size, progress);
	} else {
		backgroundStrip.Close();
	}
}

void SciTEGTK::UserStripShow(const char *description) {
	if (*description) {
		userStrip.SetDescription(description);
		userStrip.Show(props.GetInt("strip.button.height", -1));
	} else {
		userStrip.Close();
		SizeSubWindows();
	}
}

void SciTEGTK::UserStripSet(int control, const char *value) {
	userStrip.Set(control, value);
}

void SciTEGTK::UserStripSetList(int control, const char *value) {
	userStrip.SetList(control, value);
}

std::string SciTEGTK::UserStripValue(int control) {
	return userStrip.GetValue(control);
}

void SciTEGTK::UserStripClosed() {
	SizeSubWindows();
}

gboolean SciTEGTK::IOSignal(GIOChannel *, GIOCondition, SciTEGTK *scitew) {
#ifndef GDK_VERSION_3_6
	ThreadLockMinder minder;
#endif
	scitew->ContinueExecute(FALSE);
	return TRUE;
}

void SciTEGTK::ReapChild(GPid pid, gint status, gpointer user_data) {
	SciTEGTK *self = static_cast<SciTEGTK*>(user_data);

	self->exitStatus = status;
	self->pidShell = 0;
	self->triedKill = false;

	g_spawn_close_pid(pid);
}

static void SetupChild(gpointer) {
	dup2(1, 2);
	setpgid(0, 0);
}

void SciTEGTK::Execute() {
	if (buffers.SavingInBackground())
		// May be saving file that should be used by command so wait until all saved
		return;

	SciTEBase::Execute();

	commandTime.Duration(true);
	if (scrollOutput)
		wOutput.Send(SCI_GOTOPOS, wOutput.Send(SCI_GETTEXTLENGTH));
	originalEnd = wOutput.Send(SCI_GETCURRENTPOS);

	lastOutput = "";
	lastFlags = jobQueue.jobQueue[icmd].flags;

	if (jobQueue.jobQueue[icmd].jobType != JobSubsystem::extension) {
		OutputAppendString(">");
		OutputAppendString(jobQueue.jobQueue[icmd].command.c_str());
		OutputAppendString("\n");
	}

	if (jobQueue.jobQueue[icmd].directory.IsSet()) {
		jobQueue.jobQueue[icmd].directory.SetWorkingDirectory();
	}

	if (jobQueue.jobQueue[icmd].jobType == JobSubsystem::shell) {
		const gchar *argv[] = { "/bin/sh", "-c", jobQueue.jobQueue[icmd].command.c_str(), NULL };
		g_spawn_async(NULL, const_cast<gchar**>(argv), NULL, GSpawnFlags{}, NULL, NULL, NULL, NULL);
		ExecuteNext();
	} else if (jobQueue.jobQueue[icmd].jobType == JobSubsystem::extension) {
		if (extender)
			extender->OnExecute(jobQueue.jobQueue[icmd].command.c_str());
		ExecuteNext();
	} else {
		GError *error = NULL;
		gint fdout;
		const char *argv[] = { "/bin/sh", "-c", jobQueue.jobQueue[icmd].command.c_str(), NULL };

		if (!g_spawn_async_with_pipes(
			NULL, const_cast<gchar**>(argv), NULL,
			G_SPAWN_DO_NOT_REAP_CHILD, SetupChild, NULL,
			&pidShell, NULL, &fdout, NULL, &error
		)) {
			OutputAppendString(">g_spawn_async_with_pipes: ");
			OutputAppendString(error->message);
			OutputAppendString("\n");

			g_error_free(error);
		}
		g_child_watch_add(pidShell, SciTEGTK::ReapChild, this);

		fdFIFO = fdout;
		triedKill = false;
		fcntl(fdFIFO, F_SETFL, fcntl(fdFIFO, F_GETFL) | O_NONBLOCK);
		inputChannel = g_io_channel_unix_new(fdout);
		inputHandle = g_io_add_watch(inputChannel, G_IO_IN, (GIOFunc)IOSignal, this);
		// Also add a background task in case there is no output from the tool
		pollID = g_timeout_add(20, reinterpret_cast<GSourceFunc>(SciTEGTK::PollTool), this);
	}
}

void SciTEGTK::StopExecute() {
	if (!triedKill && pidShell) {
#if defined(G_OS_UNIX)
		// Only on Unix.
		kill(-pidShell, SIGKILL);
		// On Windows should call a native API, possibly TerminateProcess.
#endif
		triedKill = true;
	}
}

void SciTEGTK::GotoCmd() {
	const SA::Line lineNo = dlgGoto.entryGoto.Value();
	GotoLineEnsureVisible(lineNo - 1);
	dlgGoto.Destroy();
}

void SciTEGTK::GotoResponse(int responseID) {
	switch (responseID) {
		case GTK_RESPONSE_OK:
			GotoCmd() ;
			break;

		case GTK_RESPONSE_CANCEL:
			dlgGoto.Destroy();
			break;
	}
}

void SciTEGTK::GoLineDialog() {

	dlgGoto.Create(localiser.Text("Go To"));

	gtk_container_set_border_width(GTK_CONTAINER(PWidget(dlgGoto)), 0);

	WTable table(1, 2);
	table.PackInto(GTK_BOX(dlgGoto.ContentArea()));

	GtkWidget *labelGoto = TranslatedLabel("_Destination Line Number:");
	table.Label(labelGoto);

	dlgGoto.entryGoto.Create("");
	table.Add(dlgGoto.entryGoto);
	dlgGoto.entryGoto.ActivatesDefault();
	gtk_widget_grab_focus(dlgGoto.entryGoto);
	gtk_label_set_mnemonic_widget(GTK_LABEL(labelGoto), dlgGoto.entryGoto);

	AttachResponse<&SciTEGTK::GotoResponse>(PWidget(dlgGoto), this);
	dlgGoto.ResponseButton(localiser.Text("_Cancel"), GTK_RESPONSE_CANCEL);
	dlgGoto.ResponseButton(localiser.Text("_Go To"), GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(PWidget(dlgGoto)), GTK_RESPONSE_OK);

	dlgGoto.Display(PWidget(wSciTE));
}

void SciTEGTK::AbbrevCmd() {
	abbrevInsert = dlgAbbrev.comboAbbrev.Text();
	PerformInsertAbbreviation();
	dlgAbbrev.Destroy();
}

void SciTEGTK::AbbrevResponse(int responseID) {
	switch (responseID) {
		case GTK_RESPONSE_OK:
			AbbrevCmd();
			break;

		case GTK_RESPONSE_CANCEL:
			dlgAbbrev.Destroy();
			break;
	}
}

void SciTEGTK::AbbrevDialog() {
	dlgAbbrev.Create(localiser.Text("Insert Abbreviation"));

	gtk_container_set_border_width(GTK_CONTAINER(PWidget(dlgAbbrev)), 0);

	WTable table(1, 2);
	table.PackInto(GTK_BOX(dlgAbbrev.ContentArea()));

	GtkWidget *labelAbbrev = TranslatedLabel("_Abbreviation:");
	table.Label(labelAbbrev);

	dlgAbbrev.comboAbbrev.Create();
	gtk_entry_set_width_chars(dlgAbbrev.comboAbbrev.Entry(), 35);
	FillComboFromProps(&dlgAbbrev.comboAbbrev, propsAbbrev);
	table.Add(dlgAbbrev.comboAbbrev, 2, true);
	dlgAbbrev.comboAbbrev.ActivatesDefault();

	gtk_widget_grab_focus(dlgAbbrev.comboAbbrev);
	gtk_label_set_mnemonic_widget(GTK_LABEL(labelAbbrev), dlgAbbrev.comboAbbrev);

	AttachResponse<&SciTEGTK::AbbrevResponse>(PWidget(dlgAbbrev), this);
	dlgAbbrev.ResponseButton(localiser.Text("_Cancel"), GTK_RESPONSE_CANCEL);
	dlgAbbrev.ResponseButton(localiser.Text("_Insert"), GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(PWidget(dlgAbbrev)), GTK_RESPONSE_OK);

	dlgAbbrev.Display(PWidget(wSciTE));
}

void SciTEGTK::TabSizeSet(int &tabSize, bool &useTabs) {
	tabSize = dlgTabSize.entryTabSize.Value();
	if (tabSize > 0)
		wEditor.SetTabWidth(tabSize);
	const int indentSize = dlgTabSize.entryIndentSize.Value();
	if (indentSize > 0)
		wEditor.SetIndent(indentSize);
	useTabs = dlgTabSize.toggleUseTabs.Active();
	wEditor.SetUseTabs(useTabs);
}

void SciTEGTK::TabSizeCmd() {
	int tabSize;
	bool useTabs;
	TabSizeSet(tabSize, useTabs);
	dlgTabSize.Destroy();
}

void SciTEGTK::TabSizeConvertCmd() {
	int tabSize;
	bool useTabs;
	TabSizeSet(tabSize, useTabs);
	ConvertIndentation(tabSize, useTabs);
	dlgTabSize.Destroy();
}

#define RESPONSE_CONVERT 1001

void SciTEGTK::TabSizeResponse(int responseID) {
	switch (responseID) {
		case RESPONSE_CONVERT:
			TabSizeConvertCmd();
			break;

		case GTK_RESPONSE_OK:
			TabSizeCmd() ;
			break;

		case GTK_RESPONSE_CANCEL:
			dlgTabSize.Destroy();
			break;
	}
}

void SciTEGTK::TabSizeDialog() {

	dlgTabSize.Create(localiser.Text("Indentation Settings"));

	gtk_container_set_border_width(GTK_CONTAINER(PWidget(dlgTabSize)), 0);

	WTable table(3, 2);
	table.PackInto(GTK_BOX(dlgTabSize.ContentArea()));

	GtkWidget *labelTabSize = TranslatedLabel("_Tab Size:");
	table.Label(labelTabSize);

	std::string tabSize = StdStringFromInteger(wEditor.TabWidth());
	dlgTabSize.entryTabSize.Create(tabSize);
	table.Add(dlgTabSize.entryTabSize);
	dlgTabSize.entryTabSize.ActivatesDefault();
	gtk_widget_grab_focus(dlgTabSize.entryTabSize);
	gtk_label_set_mnemonic_widget(GTK_LABEL(labelTabSize), dlgTabSize.entryTabSize);

	GtkWidget *labelIndentSize = TranslatedLabel("_Indent Size:");
	table.Label(labelIndentSize);

	std::string indentSize = StdStringFromInteger(wEditor.Indent());
	dlgTabSize.entryIndentSize.Create(indentSize);
	table.Add(dlgTabSize.entryIndentSize);
	dlgTabSize.entryIndentSize.ActivatesDefault();
	gtk_label_set_mnemonic_widget(GTK_LABEL(labelIndentSize), dlgTabSize.entryIndentSize);

	const bool useTabs = wEditor.UseTabs();
	dlgTabSize.toggleUseTabs.Create(localiser.Text("_Use Tabs"));
	dlgTabSize.toggleUseTabs.SetActive(useTabs);
	table.Add();
	table.Add(dlgTabSize.toggleUseTabs);

	AttachResponse<&SciTEGTK::TabSizeResponse>(PWidget(dlgTabSize), this);
	dlgTabSize.ResponseButton(localiser.Text("Con_vert"), RESPONSE_CONVERT);
	dlgTabSize.ResponseButton(localiser.Text("_Cancel"), GTK_RESPONSE_CANCEL);
	dlgTabSize.ResponseButton(localiser.Text("_OK"), GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(PWidget(dlgTabSize)), GTK_RESPONSE_OK);

	dlgTabSize.Display(PWidget(wSciTE));
}

bool SciTEGTK::ParametersOpen() {
	return dlgParameters.Created();
}

void SciTEGTK::ParamGrab() {
	if (dlgParameters.Created()) {
		for (int param = 0; param < maxParam; param++) {
			std::string paramText = StdStringFromInteger(param + 1);
			const char *paramVal = dlgParameters.entryParam[param].Text();
			props.Set(paramText, paramVal);
		}
		UpdateStatusBar(true);
	}
}

void SciTEGTK::ParamCancelCmd() {
	dlgParameters.Destroy();
	CheckMenus();
}

void SciTEGTK::ParamCmd() {
	dlgParameters.paramDialogCanceled = false;
	ParamGrab();
	dlgParameters.Destroy();
	CheckMenus();
}

void SciTEGTK::ParamResponse(int responseID) {
	switch (responseID) {
		case GTK_RESPONSE_OK:
			ParamCmd() ;
			break;

		case GTK_RESPONSE_CANCEL:
			ParamCancelCmd();
			break;
	}
}

bool SciTEGTK::ParametersDialog(bool modal) {
	if (dlgParameters.Created()) {
		ParamGrab();
		if (!modal) {
			dlgParameters.Destroy();
		}
		return true;
	}
	dlgParameters.paramDialogCanceled = true;
	dlgParameters.Create(localiser.Text("Parameters"));

	g_signal_connect(G_OBJECT(PWidget(dlgParameters)),
	                   "destroy", G_CALLBACK(destroyDialog), &dlgParameters);

	WTable table(modal ? 10 : 9, 2);
	table.PackInto(GTK_BOX(dlgParameters.ContentArea()));

	if (modal) {
		GtkWidget *cmd = gtk_label_new(parameterisedCommand.c_str());
		table.Add(cmd, 2);
	}

	for (int param = 0; param < maxParam; param++) {
		std::string paramText = StdStringFromInteger(param + 1);
		std::string paramTextVal = props.GetString(paramText.c_str());
		paramText.insert(0, "_");
		paramText.append(":");
		GtkWidget *label = gtk_label_new_with_mnemonic(paramText.c_str());
		table.Label(label);

		dlgParameters.entryParam[param].Create(paramTextVal);
		table.Add(dlgParameters.entryParam[param]);
		dlgParameters.entryParam[param].ActivatesDefault();

		gtk_label_set_mnemonic_widget(GTK_LABEL(label), dlgParameters.entryParam[param]);
	}

	gtk_widget_grab_focus(dlgParameters.entryParam[0]);

	AttachResponse<&SciTEGTK::ParamResponse>(PWidget(dlgParameters), this);
	dlgParameters.ResponseButton(localiser.Text(modal ? "_Cancel" : "_Close"), GTK_RESPONSE_CANCEL);
	dlgParameters.ResponseButton(localiser.Text(modal ? "_Execute" : "_Set"), GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(PWidget(dlgParameters)), GTK_RESPONSE_OK);

	dlgParameters.Display(PWidget(wSciTE), modal);

	return !dlgParameters.paramDialogCanceled;
}

#define RESPONSE_MARK_ALL 1002
#define RESPONSE_REPLACE 1003
#define RESPONSE_REPLACE_ALL 1004
#define RESPONSE_REPLACE_IN_SELECTION 1005
#define RESPONSE_REPLACE_IN_BUFFERS 1006

void SciTEGTK::FindReplaceResponse(int responseID) {
	switch (responseID) {
		case GTK_RESPONSE_OK:
			FRFindCmd() ;
			break;

		case GTK_RESPONSE_CANCEL:
			dlgFindReplace.Destroy();
			break;

		case RESPONSE_MARK_ALL:
			FRMarkAllCmd();
			break;

		case RESPONSE_REPLACE:
			FRReplaceCmd();
			break;

		case RESPONSE_REPLACE_ALL:
			FRReplaceAllCmd();
			break;

		case RESPONSE_REPLACE_IN_SELECTION:
			FRReplaceInSelectionCmd();
			break;

		case RESPONSE_REPLACE_IN_BUFFERS:
			FRReplaceInBuffersCmd();
			break;
	}
}

void SciTEGTK::FindReplace(bool replace) {

	replacing = replace;
	dlgFindReplace.SetSearcher(this);
	dlgFindReplace.Create(localiser.Text(replace ? "Replace" : "Find"));

	g_signal_connect(G_OBJECT(PWidget(dlgFindReplace)),
	                   "destroy", G_CALLBACK(destroyDialogFindReplace), &dlgFindReplace);

	WTable table(replace ? 8 : 7, 2);
	table.PackInto(GTK_BOX(dlgFindReplace.ContentArea()));

	dlgFindReplace.labelFind.Create(localiser.Text("Fi_nd what:"));
	table.Label(dlgFindReplace.labelFind);

	dlgFindReplace.wComboFind.Create();
	table.Add(dlgFindReplace.wComboFind, 1, true);

	dlgFindReplace.wComboFind.SetText(findWhat);
	gtk_entry_set_width_chars(dlgFindReplace.wComboFind.Entry(), 40);
	dlgFindReplace.wComboFind.ActivatesDefault();
	dlgFindReplace.labelFind.SetMnemonicFor(dlgFindReplace.wComboFind);

	if (replace) {
		dlgFindReplace.labelReplace.Create(localiser.Text("Rep_lace with:"));
		table.Label(dlgFindReplace.labelReplace);

		dlgFindReplace.wComboReplace.Create();
		table.Add(dlgFindReplace.wComboReplace, 1, true);

		dlgFindReplace.wComboReplace.ActivatesDefault();
		dlgFindReplace.labelReplace.SetMnemonicFor(dlgFindReplace.wComboReplace);

	} else {
		dlgFindReplace.wComboReplace.SetID(0);
	}

	// Whole Word
	dlgFindReplace.toggleWord.Create(localiser.Text(toggles[SearchOption::tWord].label));
	table.Add(dlgFindReplace.toggleWord, 2, false, 3, 0);

	// Case Sensitive
	dlgFindReplace.toggleCase.Create(localiser.Text(toggles[SearchOption::tCase].label));
	table.Add(dlgFindReplace.toggleCase, 2, false, 3, 0);

	// Regular Expression
	dlgFindReplace.toggleRegExp.Create(localiser.Text(toggles[SearchOption::tRegExp].label));
	table.Add(dlgFindReplace.toggleRegExp, 2, false, 3, 0);

	// Transform backslash expressions
	dlgFindReplace.toggleUnSlash.Create(localiser.Text(toggles[SearchOption::tBackslash].label));
	table.Add(dlgFindReplace.toggleUnSlash, 2, false, 3, 0);

	// Wrap Around
	dlgFindReplace.toggleWrap.Create(localiser.Text(toggles[SearchOption::tWrap].label));
	table.Add(dlgFindReplace.toggleWrap, 2, false, 3, 0);

	if (replace) {
		dlgFindReplace.toggleReverse.SetID(0);
	} else {
		// Reverse
		dlgFindReplace.toggleReverse.Create(localiser.Text(toggles[SearchOption::tUp].label));
		table.Add(dlgFindReplace.toggleReverse, 2, false, 3, 0);
	}

	if (!replace) {
		dlgFindReplace.ResponseButton(localiser.Text(textMarkAll), RESPONSE_MARK_ALL);
	}

	if (replace) {
		dlgFindReplace.ResponseButton(localiser.Text(textReplace), RESPONSE_REPLACE);
		dlgFindReplace.ResponseButton(localiser.Text(textReplaceAll), RESPONSE_REPLACE_ALL);
		dlgFindReplace.ResponseButton(localiser.Text(textInSelection), RESPONSE_REPLACE_IN_SELECTION);
		if (FindReplaceAdvanced()) {
			dlgFindReplace.ResponseButton(localiser.Text(textReplaceInBuffers), RESPONSE_REPLACE_IN_BUFFERS);
		}
	}

	dlgFindReplace.ResponseButton(localiser.Text(textClose), GTK_RESPONSE_CANCEL);
	dlgFindReplace.ResponseButton(localiser.Text(textFind), GTK_RESPONSE_OK);

	AttachResponse<&SciTEGTK::FindReplaceResponse>(PWidget(dlgFindReplace), this);
	gtk_dialog_set_default_response(GTK_DIALOG(PWidget(dlgFindReplace)), GTK_RESPONSE_OK);

	dlgFindReplace.FillFields();

	gtk_widget_grab_focus(GTK_WIDGET(dlgFindReplace.wComboFind.Entry()));

	dlgFindReplace.Display(PWidget(wSciTE), false);
}

void SciTEGTK::DestroyFindReplace() {
	dlgFindReplace.Destroy();
}

SciTEBase::MessageBoxChoice SciTEGTK::WindowMessageBox(GUI::Window &w, const GUI::gui_string &msg, MessageBoxStyle style) {
	if (!messageBoxDialog) {
		std::string sMsg(msg.c_str());
		dialogsOnScreen++;
		GtkAccelGroup *accel_group = gtk_accel_group_new();

		messageBoxResult = -1;
		messageBoxDialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(messageBoxDialog), appName);
		gtk_container_set_border_width(GTK_CONTAINER(messageBoxDialog), 0);

		g_signal_connect(G_OBJECT(messageBoxDialog),
		                   "destroy", G_CALLBACK(messageBoxDestroy), &messageBoxDialog);

		MessageBoxChoice escapeResult = MessageBoxChoice::ok;
		if ((style & 0xf) == mbsOK) {
			AddMBButton(messageBoxDialog, "_OK", MessageBoxChoice::ok, accel_group, true);
		} else {
			AddMBButton(messageBoxDialog, "_Yes", MessageBoxChoice::yes, accel_group, true);
			AddMBButton(messageBoxDialog, "_No", MessageBoxChoice::no, accel_group);
			escapeResult = MessageBoxChoice::no;
			if ((style & 0xf) == mbsYesNoCancel) {
				AddMBButton(messageBoxDialog, "_Cancel", MessageBoxChoice::cancel, accel_group);
				escapeResult = MessageBoxChoice::cancel;
			}
		}
		g_signal_connect(G_OBJECT(messageBoxDialog),
		                   "key_press_event", G_CALLBACK(messageBoxKey),
		                   GINT_TO_POINTER(escapeResult));

		if (style & mbsAboutBox) {
			GtkWidget *explanation = scintilla_new();
			GUI::ScintillaWindow scExplanation;
			scExplanation.SetScintilla(explanation);
			scintilla_set_id(SCINTILLA(explanation), 0);
#if GTK_CHECK_VERSION(3,0,0)
			gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(messageBoxDialog))),
			                   explanation, TRUE, TRUE, 0);
#else
			gtk_box_pack_start(GTK_BOX(GTK_DIALOG(messageBoxDialog)->vbox),
			                   explanation, TRUE, TRUE, 0);
#endif
			gtk_widget_set_size_request(GTK_WIDGET(explanation), 480, 380);
			gtk_widget_show_all(explanation);
			SetAboutMessage(scExplanation, "SciTE");
		} else {
			GtkWidget *label = gtk_label_new(sMsg.c_str());
#if GTK_CHECK_VERSION(3,14,0)
			gtk_widget_set_margin_start(label, 10);
			gtk_widget_set_margin_end(label, 10);
			gtk_widget_set_margin_top(label, 10);
			gtk_widget_set_margin_bottom(label, 10);
#else
			gtk_misc_set_padding(GTK_MISC(label), 10, 10);
#endif
#if GTK_CHECK_VERSION(3,0,0)
			gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(messageBoxDialog))),
			                   label, TRUE, TRUE, 0);
#else
			gtk_box_pack_start(GTK_BOX(GTK_DIALOG(messageBoxDialog)->vbox),
			                   label, TRUE, TRUE, 0);
#endif
			gtk_widget_show(label);
		}

		// Mark it as a modal transient dialog
		gtk_window_set_modal(GTK_WINDOW(messageBoxDialog), TRUE);
		gtk_window_set_transient_for(GTK_WINDOW(messageBoxDialog),
		                             GTK_WINDOW(PWidget(w)));

		gtk_widget_show(messageBoxDialog);
		gtk_window_add_accel_group(GTK_WINDOW(messageBoxDialog), accel_group);
		while (messageBoxResult < 0) {
			gtk_main_iteration();
		}
		dialogsOnScreen--;
	}
	return static_cast<MessageBoxChoice>(messageBoxResult);
}

void SciTEGTK::FindMessageBox(const std::string &msg, const std::string *findItem) {
	if (findItem == 0) {
		GUI::gui_string msgBuf = LocaliseMessage(msg.c_str());
		WindowMessageBox(wSciTE, msgBuf);
	} else {
		GUI::gui_string msgBuf = LocaliseMessage(msg.c_str(), findItem->c_str());
		WindowMessageBox(wSciTE, msgBuf);
	}
}

void SciTEGTK::AboutDialog() {
	WindowMessageBox(wSciTE, GUI::gui_string("SciTE\nby Neil Hodgson neilh@scintilla.org ."),
	                 mbsAboutBox);
}

void SciTEGTK::QuitProgram() {
	if (SaveIfUnsureAll() != SaveResult::cancelled) {
		quitting = true;
		// If ongoing saves, wait for them to complete.
		if (!buffers.SavingInBackground()) {
			gtk_main_quit();
		}
	}
}

void SciTEGTK::PanePositionChanged(GObject *, GParamSpec *, SciTEGTK *scitew) {
	const bool sizeForced = scitew->UpdateOutputSize();

	if (sizeForced) {
		// Calling SizeContentWindows sets position on the H/VPaned and thus
		// prevents the user setting the size of either output or editor to less
		// than 20px.

		// Setting the position again may lead to this function being called
		// twice, but it doesn't lead to an infinite loop, since sizeForced is
		// only true when the position really changed.
		scitew->SizeContentWindows();
	}
}

gint SciTEGTK::PaneButtonRelease(GtkWidget *, GdkEvent *, SciTEGTK *scitew) {
	// Only register previousHeightOutput when the user stops dragging

	scitew->UpdateOutputSize();

	if (scitew->heightOutput > 0) {
		scitew->previousHeightOutput = scitew->heightOutput;
	}

	return FALSE;
}

bool SciTEGTK::UpdateOutputSize() {
	// This sets heightOutput and returns a boolean indicating whether
	// heightOutput is equal to the H/VPaned position or an adjustment was made.

	gint max;
	int pos;
	int newHeight;

	g_object_get(G_OBJECT(splitPane), "max-position", &max, NULL);
	pos = gtk_paned_get_position(GTK_PANED(splitPane));

	newHeight = max - pos;

	// Return false if the height hasn't changed
	// This prevents an infinite loop in PanePositionChanged()
	if (newHeight == heightOutput)
		return false;

	heightOutput = newHeight;

	if (heightOutput > (max - 20)) {
		heightOutput = max;
		return true;
	} else if (heightOutput < 20) {
		heightOutput = 0;
		return true;
	}

	return false;
}

gint SciTEGTK::QuitSignal(GtkWidget *, GdkEventAny *, SciTEGTK *scitew) {
	scitew->QuitProgram();
	return TRUE;
}

void SciTEGTK::ButtonSignal(GtkWidget *, gpointer data) {
	instance->Command(GPOINTER_TO_UINT(data));
}

void SciTEGTK::MenuSignal(GtkMenuItem *menuitem, SciTEGTK *scitew) {
	if (scitew->allowMenuActions) {
		const guint action = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "CmdNum"));
		scitew->Command(action);
	}
}

void SciTEGTK::NotifySignal(GtkWidget *, gint /*wParam*/, SCNotification *notification, SciTEGTK *scitew) {
	scitew->Notify(notification);
}

gint SciTEGTK::KeyPress(GtkWidget * /*widget*/, GdkEventKey *event, SciTEGTK *scitew) {
	return scitew->Key(event);
}

gint SciTEGTK::KeyRelease(GtkWidget * /*widget*/, GdkEventKey *event, SciTEGTK *scitew) {
	return scitew->Key(event);
}

gint SciTEGTK::MousePress(GtkWidget * /*widget*/, GdkEventButton *event, SciTEGTK *scitew) {
	return scitew->Mouse(event);
}

namespace {

// Translate key strokes that are not in a menu into commands
class KeyToCommand {
public:
	int modifiers;
	unsigned int key;	// For alphabetic keys has to match the shift modifier.
	int msg;
};

enum {
    m__ = 0,
    mS_ = GDK_SHIFT_MASK,
    m_C = GDK_CONTROL_MASK,
    mSC = GDK_SHIFT_MASK | GDK_CONTROL_MASK
};

KeyToCommand kmap[] = {
                                 {m_C, GKEY_Tab, IDM_NEXTFILESTACK},
                                 {mSC, GKEY_ISO_Left_Tab, IDM_PREVFILESTACK},
                                 {m_C, GKEY_KP_Enter, IDM_COMPLETEWORD},
                                 {GDK_MOD1_MASK, GKEY_F2, IDM_BOOKMARK_NEXT_SELECT},
                                 {GDK_MOD1_MASK|GDK_SHIFT_MASK, GKEY_F2, IDM_BOOKMARK_PREV_SELECT},
                                 {m_C, GKEY_F3, IDM_FINDNEXTSEL},
                                 {mSC, GKEY_F3, IDM_FINDNEXTBACKSEL},
                                 {m_C, GKEY_F4, IDM_CLOSE},
                                 {m_C, 'j', IDM_PREVMATCHPPC},
                                 {mSC, 'J', IDM_SELECTTOPREVMATCHPPC},
                                 {m_C, 'k', IDM_NEXTMATCHPPC},
                                 {mSC, 'K', IDM_SELECTTONEXTMATCHPPC},
                                 {m_C, GKEY_KP_Multiply, IDM_EXPAND},
                                 {0, 0, 0},
                             };

bool KeyMatch(const char *menuKey, int keyval, int modifiers) {
	return SciTEKeys::MatchKeyCode(
		SciTEKeys::ParseKeyCode(menuKey), keyval, modifiers);
}

}

gint SciTEGTK::Key(GdkEventKey *event) {
	//printf("S-key: %d %x %x %x %x\n",event->keyval, event->state, GDK_SHIFT_MASK, GDK_CONTROL_MASK, GDK_F3);
	if (event->type == GDK_KEY_RELEASE) {
		if (event->keyval == GKEY_Control_L || event->keyval == GKEY_Control_R) {
			g_signal_stop_emission_by_name(
			    G_OBJECT(PWidget(wSciTE)), "key-release-event");
			this->EndStackedTabbing();
			return 1;
		} else {
			return 0;
		}
	}

	const int modifiers = event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD4_MASK);

	const int cmodifiers = // modifier mask for Lua extension
		((event->state & GDK_SHIFT_MASK)   ? SCMOD_SHIFT : 0) |
		((event->state & GDK_CONTROL_MASK) ? SCMOD_CTRL  : 0) |
		((event->state & GDK_MOD1_MASK)    ? SCMOD_ALT   : 0);
	if (extender && extender->OnKey(event->keyval, cmodifiers))
		return 1;

	int commandID = 0;
	for (int i = 0; kmap[i].msg; i++) {
		if ((event->keyval == kmap[i].key) && (modifiers == kmap[i].modifiers)) {
			commandID = kmap[i].msg;
		}
	}
	if (!commandID) {
		// Look through language menu
		for (unsigned int j = 0; j < languageMenu.size(); j++) {
			if (KeyMatch(languageMenu[j].menuKey.c_str(), event->keyval, modifiers)) {
				commandID = IDM_LANGUAGE + j;
			}
		}
	}
	if (commandID) {
		Command(commandID);
	}
	if ((commandID == IDM_NEXTFILE) ||
		(commandID == IDM_PREVFILE) ||
		(commandID == IDM_NEXTFILESTACK) ||
		(commandID == IDM_PREVFILESTACK)) {
		// Stop the default key processing from moving the focus
		g_signal_stop_emission_by_name(
		    G_OBJECT(PWidget(wSciTE)), "key_press_event");
	}

	// check tools menu command shortcuts
	for (int tool_i = 0; tool_i < toolMax; ++tool_i) {
		GtkWidget *item = MenuItemFromAction(IDM_TOOLS + tool_i);
		if (item) {
			const long keycode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "key"));
			if (keycode && SciTEKeys::MatchKeyCode(keycode, event->keyval, modifiers)) {
				SciTEBase::MenuCommand(IDM_TOOLS + tool_i);
				return 1;
			}
		}
	}

	// check user defined keys
	for (const ShortcutItem &scut : shortCutItemList) {
		if (KeyMatch(scut.menuKey.c_str(), event->keyval, modifiers)) {
			const int commandNum = SciTEBase::GetMenuCommandAsInt(scut.menuCommand);
			if (commandNum != -1) {
				if (commandNum < 2000) {
					SciTEBase::MenuCommand(commandNum);
				} else {
					PaneFocused().Call(static_cast<SA::Message>(commandNum));
				}
				g_signal_stop_emission_by_name(
				    G_OBJECT(PWidget(wSciTE)), "key_press_event");
				return 1;
			}
		}
	}

	if (findStrip.KeyDown(event) || replaceStrip.KeyDown(event) || filterStrip.KeyDown(event) || userStrip.KeyDown(event)) {
		g_signal_stop_emission_by_name(G_OBJECT(PWidget(wSciTE)), "key_press_event");
		return 1;
	}

	return 0;
}

void SciTEGTK::PopUpCmd(GtkMenuItem *menuItem, SciTEGTK *scitew) {
	const guint cmd = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuItem), "CmdNum"));
	scitew->Command(cmd);
}

void SciTEGTK::AddToPopUp(const char *label, int cmd, bool enabled) {
	GUI::gui_string localised = localiser.Text(label);
	GtkWidget *menuItem;
	if (label[0])
		menuItem = gtk_menu_item_new_with_label(localised.c_str());
	else
		menuItem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(popup.GetID()), menuItem);
	g_object_set_data(G_OBJECT(menuItem), "CmdNum", GINT_TO_POINTER(cmd));
	g_signal_connect(G_OBJECT(menuItem),"activate", G_CALLBACK(PopUpCmd), this);

	if (cmd) {
		if (menuItem)
			gtk_widget_set_sensitive(menuItem, enabled);
	}
}

gint SciTEGTK::Mouse(GdkEventButton *event) {
	if (event->button == 3) {
		// PopUp menu
		GUI::ScintillaWindow *w = &wEditor;
		menuSource = IDM_SRCWIN;
		if (WindowFromWidget(PWidget(*w)) != event->window) {
			if (WindowFromWidget(PWidget(wOutput)) == event->window) {
				menuSource = IDM_RUNWIN;
				w = &wOutput;
			} else {
				menuSource = 0;
				//fprintf(stderr, "Menu source focus\n");
				return FALSE;
			}
		}
		// Convert to screen
		int ox = 0;
		int oy = 0;
		gdk_window_get_origin(WindowFromWidget(PWidget(*w)), &ox, &oy);
		ContextMenu(*w, GUI::Point(static_cast<int>(event->x) + ox,
		                     static_cast<int>(event->y) + oy), wSciTE);
		//fprintf(stderr, "Menu source %s\n",
		//	(menuSource == IDM_SRCWIN) ? "IDM_SRCWIN" : "IDM_RUNWIN");
	} else {
		menuSource = 0;
		//fprintf(stderr, "Menu source focus\n");
	}

	return FALSE;
}

void SciTEGTK::DragDataReceived(GtkWidget *, GdkDragContext *context,
                                gint /*x*/, gint /*y*/, GtkSelectionData *selectionData, guint /*info*/, guint time, SciTEGTK *scitew) {
	scitew->OpenUriList(reinterpret_cast<const char *>(gtk_selection_data_get_data(selectionData)));
	gtk_drag_finish(context, TRUE, FALSE, time);
}

gint SciTEGTK::TabBarRelease(GtkNotebook *notebook, GdkEventButton *event) {
	if (event->button == 1) {
		SetDocumentAt(gtk_notebook_get_current_page(GTK_NOTEBOOK(wTabBar.GetID())));
		CheckReload();
	} else if (event->button == 2) {
		for (int pageNum=0;pageNum<gtk_notebook_get_n_pages(notebook);pageNum++) {
			GtkWidget *page = gtk_notebook_get_nth_page(notebook, pageNum);
			if (page) {
				GtkWidget *label = gtk_notebook_get_tab_label(notebook, page);
				GtkAllocation allocation;
				gtk_widget_get_allocation(label, &allocation);
				if (event->x < (allocation.x + allocation.width)) {
					CloseTab(pageNum);
					break;
				}
			}
		}
	}
	return FALSE;
}

gint SciTEGTK::TabBarScroll(GdkEventScroll *event) {
	switch (event->direction) {
		case GDK_SCROLL_RIGHT:
		case GDK_SCROLL_DOWN:
			Next();
			WindowSetFocus(wEditor);
			break;
		case GDK_SCROLL_LEFT:
		case GDK_SCROLL_UP:
			Prev();
			WindowSetFocus(wEditor);
			break;
#if GTK_CHECK_VERSION(3,4,0)
		case GDK_SCROLL_SMOOTH:
			break;
#endif
	}
	// Return true because Next() or Prev() already switches the tab
	return TRUE;
}

GtkWidget *SciTEGTK::AddToolButton(const char *text, int cmd, GtkWidget *toolbar_icon) {
	gtk_widget_show(GTK_WIDGET(toolbar_icon));
	GUI::gui_string localised = localiser.Text(text);
	GtkToolItem *button = gtk_tool_button_new(toolbar_icon, localised.c_str());
	gtk_widget_set_tooltip_text(GTK_WIDGET(button), localised.c_str());
	gtk_widget_show(GTK_WIDGET(button));
	gtk_toolbar_insert(GTK_TOOLBAR(PWidget(wToolBar)), button, -1);

	g_signal_connect(G_OBJECT(button), "clicked",
	                   G_CALLBACK(ButtonSignal),
	                   GINT_TO_POINTER(cmd));
	return GTK_WIDGET(button);
}

void SciTEGTK::AddToolBar() {
	if (props.GetInt("toolbar.usestockicons") == 1) {
		AddToolButton("New", IDM_NEW, gtk_image_new_from_icon_name("document-new", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Open", IDM_OPEN, gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Save", IDM_SAVE, gtk_image_new_from_icon_name("document-save", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Close", IDM_CLOSE, gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_LARGE_TOOLBAR));

		AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
		AddToolButton("Undo", IDM_UNDO, gtk_image_new_from_icon_name("edit-undo", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Redo", IDM_REDO, gtk_image_new_from_icon_name("edit-redo", GTK_ICON_SIZE_LARGE_TOOLBAR));

		AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
		AddToolButton("Cut", IDM_CUT, gtk_image_new_from_icon_name("edit-cut", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Copy", IDM_COPY, gtk_image_new_from_icon_name("edit-copy", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Paste", IDM_PASTE, gtk_image_new_from_icon_name("edit-paste", GTK_ICON_SIZE_LARGE_TOOLBAR));

		AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
		AddToolButton("Find in Files", IDM_FINDINFILES, gtk_image_new_from_icon_name("edit-find", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Find", IDM_FIND, gtk_image_new_from_icon_name("gtk-zoom-fit", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Find Next", IDM_FINDNEXT, gtk_image_new_from_icon_name("go-jump", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Replace", IDM_REPLACE, gtk_image_new_from_icon_name("edit-find-replace", GTK_ICON_SIZE_LARGE_TOOLBAR));

		AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
		btnCompile = AddToolButton("Compile", IDM_COMPILE, gtk_image_new_from_icon_name("gtk-execute", GTK_ICON_SIZE_LARGE_TOOLBAR));
		btnBuild = AddToolButton("Build", IDM_BUILD, gtk_image_new_from_icon_name("gtk-convert", GTK_ICON_SIZE_LARGE_TOOLBAR));
		btnStop = AddToolButton("Stop", IDM_STOPEXECUTE, gtk_image_new_from_icon_name("process-stop", GTK_ICON_SIZE_LARGE_TOOLBAR));

		AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
		AddToolButton("Previous", IDM_PREVFILE, gtk_image_new_from_icon_name("go-previous", GTK_ICON_SIZE_LARGE_TOOLBAR));
		AddToolButton("Next Buffer", IDM_NEXTFILE, gtk_image_new_from_icon_name("go-next", GTK_ICON_SIZE_LARGE_TOOLBAR));
		return;
	}
	AddToolButton("New", IDM_NEW, pixmap_new(filenew_xpm));
	AddToolButton("Open", IDM_OPEN, pixmap_new(fileopen_xpm));
	AddToolButton("Save", IDM_SAVE, pixmap_new(filesave_xpm));
	AddToolButton("Close", IDM_CLOSE, pixmap_new(close_xpm));

	AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
	AddToolButton("Undo", IDM_UNDO, pixmap_new(undo_xpm));
	AddToolButton("Redo", IDM_REDO, pixmap_new(redo_xpm));

	AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
	AddToolButton("Cut", IDM_CUT, pixmap_new(editcut_xpm));
	AddToolButton("Copy", IDM_COPY, pixmap_new(editcopy_xpm));
	AddToolButton("Paste", IDM_PASTE, pixmap_new(editpaste_xpm));

	AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
	AddToolButton("Find in Files", IDM_FINDINFILES, pixmap_new(findinfiles_xpm));
	AddToolButton("Find", IDM_FIND, pixmap_new(search_xpm));
	AddToolButton("Find Next", IDM_FINDNEXT, pixmap_new(findnext_xpm));
	AddToolButton("Replace", IDM_REPLACE, pixmap_new(replace_xpm));

	AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
	btnCompile = AddToolButton("Compile", IDM_COMPILE, pixmap_new(compile_xpm));
	btnBuild = AddToolButton("Build", IDM_BUILD, pixmap_new(build_xpm));
	btnStop = AddToolButton("Stop", IDM_STOPEXECUTE, pixmap_new(stop_xpm));

	AddToolSpace(GTK_TOOLBAR(PWidget(wToolBar)));
	AddToolButton("Previous", IDM_PREVFILE, pixmap_new(prev_xpm));
	AddToolButton("Next Buffer", IDM_NEXTFILE, pixmap_new(next_xpm));
}

std::string SciTEGTK::TranslatePath(const char *path) {
	if (path && path[0] == '/') {
		std::string spathTranslated;
		std::string spath(path, 1, strlen(path));
		spath.append("/");
		size_t end = spath.find("/");
		while (spath.length() > 1) {
			std::string segment(spath, 0, end);
			GUI::gui_string segmentLocalised = localiser.Text(segment.c_str());
			std::replace(segmentLocalised.begin(), segmentLocalised.end(), '/', '|');
			spathTranslated.append("/");
			spathTranslated.append(segmentLocalised.c_str());
			spath.erase(0, end + 1);
			end = spath.find("/");
		}
		return spathTranslated;
	} else {
		return path ? path : std::string();
	}
}

void SciTEGTK::CreateTranslatedMenu(int n, const SciTEItemFactoryEntry items[],
                                    int nRepeats, const char *prefix, int startNum,
                                    int startID, const char *radioStart) {

	const int dim = n + nRepeats;
	std::vector<SciTEItemFactoryEntry> translatedItems(dim);
	std::vector<std::string> translatedText(dim);
	std::vector<std::string> translatedRadios(dim);
	std::vector<std::string> userDefinedAccels(n);
	int i = 0;

	for (; i < n; i++) {
		// Try to find user-defined accelerator key
		std::string menuPath = "menukey";			// menupath="menukey"
		menuPath += items[i].path;		// menupath="menukey/File/Save _As..."
		Substitute(menuPath, "_", "");		// menupath="menukey/File/Save As..."
		Substitute(menuPath, ".", "");		// menupath="menukey/File/Save As"
		Substitute(menuPath, "/", ".");		// menupath="menukey.File.Save As"
		Substitute(menuPath, " ", "_");	// menupath="menukey.File.Save_As"
		LowerCaseAZ(menuPath);		// menupath="menukey.file.save_as"

		std::string accelKey = props.GetString(menuPath.c_str());

		const char *itemAccel = items[i].accelerator;
		if (!accelKey.empty()) {
			if (accelKey == "\"\"" || accelKey == "none") {
				accelKey.clear();	// Allow user to clear accelerator key
			}
			userDefinedAccels[i] = accelKey;
			itemAccel = userDefinedAccels[i].c_str();
		}

		translatedItems[i].path = items[i].path;
		translatedItems[i].accelerator = itemAccel;
		translatedItems[i].callback_action = items[i].callback_action;
		translatedItems[i].item_type = items[i].item_type;
		translatedText[i] = TranslatePath(translatedItems[i].path);
		translatedItems[i].path = translatedText[i].c_str();
		translatedRadios[i] = TranslatePath(translatedItems[i].item_type);
		translatedItems[i].item_type = translatedRadios[i].c_str();

	}
	for (; i < dim; i++) {
		const int suffix = i - n + startNum;
		std::string ssnum = StdStringFromInteger(suffix);
		translatedText[i] = TranslatePath(prefix);
		translatedText[i] += ssnum;
		translatedItems[i].path = translatedText[i].c_str();
		translatedItems[i].accelerator = NULL;
		translatedItems[i].callback_action = startID + suffix;
		translatedRadios[i] = TranslatePath(radioStart);
		translatedItems[i].item_type = translatedRadios[i].c_str();
	}
	// Only two levels of submenu supported
	for (int itMenu=0; itMenu < dim; itMenu++) {
		SciTEItemFactoryEntry *psife = &translatedItems[itMenu];
		const char *afterSlash = psife->path+1;
		const char *lastSlash = strrchr(afterSlash, '/');
		std::string menuName(afterSlash, lastSlash ? (lastSlash-afterSlash) : 0);
		GtkWidget *menuParent = menuBar;
		if (!menuName.empty()) {
			if (pulldowns.count(menuName) > 0) {
				menuParent = pulldowns[menuName];
			} else {
				fprintf(stderr, "*** failed to find parent %s\n", psife->path);
			}
		}
		const std::string_view itemType = psife->item_type;
		if (itemType == "<Branch>") {
			// Submenu "/_Tools" "/File/Encodin_g"
			const char *menuTitle = lastSlash ? (lastSlash + 1) : afterSlash;
			GtkWidget *menuItemPullDown = gtk_menu_item_new_with_mnemonic(menuTitle);
			GtkWidget *menuPullDown = gtk_menu_new();
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItemPullDown), menuPullDown);
			gtk_menu_shell_append(GTK_MENU_SHELL(menuParent), menuItemPullDown);
			std::string baseName = WithoutUnderscore(afterSlash);
			pulldowns[baseName] = menuPullDown;
		} else if (lastSlash) {
			// Item "/File/_New"
			const std::string itemName(lastSlash+1);
			GtkWidget *menuItemCommand = 0;
			if (itemType.empty()) {
				menuItemCommand = gtk_menu_item_new_with_mnemonic(itemName.c_str());
			} else if (itemType == "<CheckItem>") {
				menuItemCommand = gtk_check_menu_item_new_with_mnemonic(itemName.c_str());
			} else if ((itemType == "<RadioItem>") || (itemType[0] == '/')) {
				menuItemCommand = gtk_radio_menu_item_new_with_mnemonic(radiogroups[menuName], itemName.c_str());
				radiogroups[menuName] = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuItemCommand));
			} else if (itemType ==  "<Separator>") {
				menuItemCommand = gtk_separator_menu_item_new();
			} else {
				menuItemCommand = gtk_menu_item_new_with_mnemonic(itemName.c_str());
			}
			if (psife->accelerator && psife->accelerator[0]) {
				guint acceleratorKey;
				GdkModifierType acceleratorMods;
				gtk_accelerator_parse(psife->accelerator, &acceleratorKey, &acceleratorMods);
				if (acceleratorKey) {
					gtk_widget_add_accelerator(menuItemCommand, "activate", accelGroup,
						acceleratorKey, acceleratorMods, GTK_ACCEL_VISIBLE);
				} else {
					//fprintf(stderr, "Failed to add accelerator '%s' for '%s'\n", psife->accelerator, psife->path);
				}
			}
			g_object_set_data(G_OBJECT(menuItemCommand), "CmdNum",
				GINT_TO_POINTER(psife->callback_action));
			g_signal_connect(G_OBJECT(menuItemCommand),"activate", G_CALLBACK(MenuSignal), this);
			gtk_menu_shell_append(GTK_MENU_SHELL(menuParent), menuItemCommand);
			mapMenuItemFromId[psife->callback_action] = menuItemCommand;
		}
	}
}

void SciTEGTK::CreateMenu() {

	GCallback menuSig = G_CALLBACK(MenuSignal);
	SciTEItemFactoryEntry menuItems[] = {
	                                      {"/_File", NULL, NULL, 0, "<Branch>"},
	                                      {"/File/_New", "<control>N", menuSig, IDM_NEW, 0},
	                                      {"/File/_Open...", "<control>O", menuSig, IDM_OPEN, 0},
	                                      {"/File/Open Selected _Filename", "<control><shift>O", menuSig, IDM_OPENSELECTED, 0},
	                                      {"/File/_Revert", "<control>R", menuSig, IDM_REVERT, 0},
	                                      {"/File/_Close", "<control>W", menuSig, IDM_CLOSE, 0},
	                                      {"/File/_Save", "<control>S", menuSig, IDM_SAVE, 0},
	                                      {"/File/Save _As...", "<control><shift>S", menuSig, IDM_SAVEAS, 0},
	                                      {"/File/Save a Cop_y...", "<control><shift>P", menuSig, IDM_SAVEACOPY, 0},
	                                      {"/File/Copy Pat_h", NULL, menuSig, IDM_COPYPATH, 0},
	                                      {"/File/Encodin_g", NULL, NULL, 0, "<Branch>"},
	                                      {"/File/Encoding/_Code Page Property", NULL, menuSig, IDM_ENCODING_DEFAULT, "<RadioItem>"},
	                                      {"/File/Encoding/UTF-16 _Big Endian", NULL, menuSig, IDM_ENCODING_UCS2BE, "/File/Encoding/Code Page Property"},
	                                      {"/File/Encoding/UTF-16 _Little Endian", NULL, menuSig, IDM_ENCODING_UCS2LE, "/File/Encoding/Code Page Property"},
	                                      {"/File/Encoding/UTF-8 _with BOM", NULL, menuSig, IDM_ENCODING_UTF8, "/File/Encoding/Code Page Property"},
	                                      {"/File/Encoding/_UTF-8", NULL, menuSig, IDM_ENCODING_UCOOKIE, "/File/Encoding/Code Page Property"},
	                                      {"/File/_Export", "", 0, 0, "<Branch>"},
	                                      {"/File/Export/As _HTML...", NULL, menuSig, IDM_SAVEASHTML, 0},
	                                      {"/File/Export/As _RTF...", NULL, menuSig, IDM_SAVEASRTF, 0},
	                                      {"/File/Export/As _PDF...", NULL, menuSig, IDM_SAVEASPDF, 0},
	                                      {"/File/Export/As _LaTeX...", NULL, menuSig, IDM_SAVEASTEX, 0},
	                                      {"/File/Export/As _XML...", NULL, menuSig, IDM_SAVEASXML, 0},
	                                      {"/File/Page Set_up", NULL, menuSig, IDM_PRINTSETUP, 0},
	                                      {"/File/_Print", "<control>P", menuSig, IDM_PRINT, 0},
	                                      {"/File/sep1", NULL, NULL, 0, "<Separator>"},
	                                      {"/File/_Load Session...", "", menuSig, IDM_LOADSESSION, 0},
	                                      {"/File/Sa_ve Session...", "", menuSig, IDM_SAVESESSION, 0},
	                                      {"/File/sep2", NULL, menuSig, IDM_MRU_SEP, "<Separator>"},
	                                      {"/File/File0", "", menuSig, fileStackCmdID + 0, 0},
	                                      {"/File/File1", "", menuSig, fileStackCmdID + 1, 0},
	                                      {"/File/File2", "", menuSig, fileStackCmdID + 2, 0},
	                                      {"/File/File3", "", menuSig, fileStackCmdID + 3, 0},
	                                      {"/File/File4", "", menuSig, fileStackCmdID + 4, 0},
	                                      {"/File/File5", "", menuSig, fileStackCmdID + 5, 0},
	                                      {"/File/File6", "", menuSig, fileStackCmdID + 6, 0},
	                                      {"/File/File7", "", menuSig, fileStackCmdID + 7, 0},
	                                      {"/File/File8", "", menuSig, fileStackCmdID + 8, 0},
	                                      {"/File/File9", "", menuSig, fileStackCmdID + 9, 0},
	                                      {"/File/sep3", NULL, NULL, 0, "<Separator>"},
	                                      {"/File/E_xit", "", menuSig, IDM_QUIT, 0},

	                                      {"/_Edit", NULL, NULL, 0, "<Branch>"},
	                                      {"/Edit/_Undo", "<control>Z", menuSig, IDM_UNDO, 0},

	                                      {"/Edit/_Redo", "<control>Y", menuSig, IDM_REDO, 0},
	                                      {"/Edit/sep1", NULL, NULL, 0, "<Separator>"},
	                                      {"/Edit/Cu_t", "<control>X", menuSig, IDM_CUT, 0},
	                                      {"/Edit/_Copy", "<control>C", menuSig, IDM_COPY, 0},
	                                      {"/Edit/_Paste", "<control>V", menuSig, IDM_PASTE, 0},
	                                      {"/Edit/Duplicat_e", "<control>D", menuSig, IDM_DUPLICATE, 0},
	                                      {"/Edit/_Delete", "Delete", menuSig, IDM_CLEAR, 0},
	                                      {"/Edit/Select _All", "<control>A", menuSig, IDM_SELECTALL, 0},
	                                      {"/Edit/sep2", NULL, NULL, 0, "<Separator>"},
	                                      {"/Edit/Match _Brace", "<control>E", menuSig, IDM_MATCHBRACE, 0},
	                                      {"/Edit/Select t_o Brace", "<control><shift>E", menuSig, IDM_SELECTTOBRACE, 0},
	                                      {"/Edit/S_how Calltip", "<control><shift>space", menuSig, IDM_SHOWCALLTIP, 0},
	                                      {"/Edit/Complete S_ymbol", "<control>I", menuSig, IDM_COMPLETE, 0},
	                                      {"/Edit/Complete _Word", "<control>Return", menuSig, IDM_COMPLETEWORD, 0},
	                                      {"/Edit/Expand Abbre_viation", "<control>B", menuSig, IDM_ABBREV, 0},
	                                      {"/Edit/_Insert Abbreviation", "<control><shift>R", menuSig, IDM_INS_ABBREV, 0},
	                                      {"/Edit/Block Co_mment or Uncomment", "<control>Q", menuSig, IDM_BLOCK_COMMENT, 0},
	                                      {"/Edit/Bo_x Comment", "<control><shift>B", menuSig, IDM_BOX_COMMENT, 0},
	                                      {"/Edit/Stream Comme_nt", "<control><shift>Q", menuSig, IDM_STREAM_COMMENT, 0},
	                                      {"/Edit/Make _Selection Uppercase", "<control><shift>U", menuSig, IDM_UPRCASE, 0},
	                                      {"/Edit/Make Selection _Lowercase", "<control>U", menuSig, IDM_LWRCASE, 0},
	                                      {"/Edit/Reverse Selected Lines", NULL, menuSig, IDM_LINEREVERSE, 0},
	                                      {"/Edit/Para_graph", NULL, NULL, 0, "<Branch>"},
	                                      {"/Edit/Paragraph/_Join", NULL, menuSig, IDM_JOIN, 0},
	                                      {"/Edit/Paragraph/_Split", NULL, menuSig, IDM_SPLIT, 0},

	                                      {"/_Search", NULL, NULL, 0, "<Branch>"},
	                                      {"/Search/_Find...", "<control>F", menuSig, IDM_FIND, 0},
	                                      {"/Search/Find _Next", "F3", menuSig, IDM_FINDNEXT, 0},
	                                      {"/Search/Find _Previous", "<shift>F3", menuSig, IDM_FINDNEXTBACK, 0},
	                                      {"/Search/F_ind in Files...", "<control><shift>F", menuSig, IDM_FINDINFILES, 0},
	                                      {"/Search/_Replace...", "<control>H", menuSig, IDM_REPLACE, 0},
	                                      {"/Search/Incremental _Search", "<control><alt>I", menuSig, IDM_INCSEARCH, 0},
	                                      {"/Search/Fi_lter", "<control><alt>F", menuSig, IDM_FILTER, 0},
	                                      {"/Search/Selection _Add Next", "<control><shift>D", menuSig, IDM_SELECTIONADDNEXT, 0},
	                                      {"/Search/Selection Add _Each", "", menuSig, IDM_SELECTIONADDEACH, 0},
	                                      {"/Search/sep3", NULL, NULL, 0, "<Separator>"},
	                                      {"/Search/_Go To...", "<control>G", menuSig, IDM_GOTO, 0},
	                                      {"/Search/Next Book_mark", "F2", menuSig, IDM_BOOKMARK_NEXT, 0},
	                                      {"/Search/Pre_vious Bookmark", "<shift>F2", menuSig, IDM_BOOKMARK_PREV, 0},
	                                      {"/Search/_Toggle Bookmark", "<control>F2", menuSig, IDM_BOOKMARK_TOGGLE, 0},
	                                      {"/Search/_Clear All Bookmarks", "", menuSig, IDM_BOOKMARK_CLEARALL, 0},
	                                      {"/Search/Select All _Bookmarks", "", menuSig, IDM_BOOKMARK_SELECT_ALL, 0},

	                                      {"/_View", NULL, NULL, 0, "<Branch>"},
	                                      {"/View/Toggle _current fold", "", menuSig, IDM_EXPAND, 0},
	                                      {"/View/Toggle _all folds", "", menuSig, IDM_TOGGLE_FOLDALL, 0},
	                                      {"/View/sep1", NULL, NULL, 0, "<Separator>"},
	                                      {"/View/Full Scree_n", "F11", menuSig, IDM_FULLSCREEN, "<CheckItem>"},
	                                      {"/View/_Tool Bar", "", menuSig, IDM_VIEWTOOLBAR, "<CheckItem>"},
	                                      {"/View/Tab _Bar", "", menuSig, IDM_VIEWTABBAR, "<CheckItem>"},
	                                      {"/View/_Status Bar", "", menuSig, IDM_VIEWSTATUSBAR, "<CheckItem>"},
	                                      {"/View/sep2", NULL, NULL, 0, "<Separator>"},
	                                      {"/View/_Whitespace", "<control><shift>A", menuSig, IDM_VIEWSPACE, "<CheckItem>"},
	                                      {"/View/_End of Line", "<control><shift>N", menuSig, IDM_VIEWEOL, "<CheckItem>"},
	                                      {"/View/_Indentation Guides", NULL, menuSig, IDM_VIEWGUIDES, "<CheckItem>"},
	                                      {"/View/_Line Numbers", "", menuSig, IDM_LINENUMBERMARGIN, "<CheckItem>"},
	                                      {"/View/_Margin", NULL, menuSig, IDM_SELMARGIN, "<CheckItem>"},
	                                      {"/View/_Fold Margin", NULL, menuSig, IDM_FOLDMARGIN, "<CheckItem>"},
	                                      {"/View/_Output", "F8", menuSig, IDM_TOGGLEOUTPUT, "<CheckItem>"},
	                                      {"/View/_Parameters", NULL, menuSig, IDM_TOGGLEPARAMETERS, "<CheckItem>"},

	                                      {"/_Tools", NULL, NULL, 0, "<Branch>"},
	                                      {"/Tools/_Compile", "<control>F7", menuSig, IDM_COMPILE, 0},
	                                      {"/Tools/_Build", "F7", menuSig, IDM_BUILD, 0},
	                                      {"/Tools/_Clean", "<shift>F7", menuSig, IDM_CLEAN, 0},
	                                      {"/Tools/_Go", "F5", menuSig, IDM_GO, 0},

	                                      {"/Tools/Tool0", NULL, menuSig, IDM_TOOLS + 0, 0},
	                                      {"/Tools/Tool1", NULL, menuSig, IDM_TOOLS + 1, 0},
	                                      {"/Tools/Tool2", NULL, menuSig, IDM_TOOLS + 2, 0},
	                                      {"/Tools/Tool3", NULL, menuSig, IDM_TOOLS + 3, 0},
	                                      {"/Tools/Tool4", NULL, menuSig, IDM_TOOLS + 4, 0},
	                                      {"/Tools/Tool5", NULL, menuSig, IDM_TOOLS + 5, 0},
	                                      {"/Tools/Tool6", NULL, menuSig, IDM_TOOLS + 6, 0},
	                                      {"/Tools/Tool7", NULL, menuSig, IDM_TOOLS + 7, 0},
	                                      {"/Tools/Tool8", NULL, menuSig, IDM_TOOLS + 8, 0},
	                                      {"/Tools/Tool9", NULL, menuSig, IDM_TOOLS + 9, 0},

	                                      {"/Tools/Tool10", NULL, menuSig, IDM_TOOLS + 10, 0},
	                                      {"/Tools/Tool11", NULL, menuSig, IDM_TOOLS + 11, 0},
	                                      {"/Tools/Tool12", NULL, menuSig, IDM_TOOLS + 12, 0},
	                                      {"/Tools/Tool13", NULL, menuSig, IDM_TOOLS + 13, 0},
	                                      {"/Tools/Tool14", NULL, menuSig, IDM_TOOLS + 14, 0},
	                                      {"/Tools/Tool15", NULL, menuSig, IDM_TOOLS + 15, 0},
	                                      {"/Tools/Tool16", NULL, menuSig, IDM_TOOLS + 16, 0},
	                                      {"/Tools/Tool17", NULL, menuSig, IDM_TOOLS + 17, 0},
	                                      {"/Tools/Tool18", NULL, menuSig, IDM_TOOLS + 18, 0},
	                                      {"/Tools/Tool19", NULL, menuSig, IDM_TOOLS + 19, 0},

	                                      {"/Tools/Tool20", NULL, menuSig, IDM_TOOLS + 20, 0},
	                                      {"/Tools/Tool21", NULL, menuSig, IDM_TOOLS + 21, 0},
	                                      {"/Tools/Tool22", NULL, menuSig, IDM_TOOLS + 22, 0},
	                                      {"/Tools/Tool23", NULL, menuSig, IDM_TOOLS + 23, 0},
	                                      {"/Tools/Tool24", NULL, menuSig, IDM_TOOLS + 24, 0},
	                                      {"/Tools/Tool25", NULL, menuSig, IDM_TOOLS + 25, 0},
	                                      {"/Tools/Tool26", NULL, menuSig, IDM_TOOLS + 26, 0},
	                                      {"/Tools/Tool27", NULL, menuSig, IDM_TOOLS + 27, 0},
	                                      {"/Tools/Tool28", NULL, menuSig, IDM_TOOLS + 28, 0},
	                                      {"/Tools/Tool29", NULL, menuSig, IDM_TOOLS + 29, 0},

	                                      {"/Tools/Tool30", NULL, menuSig, IDM_TOOLS + 30, 0},
	                                      {"/Tools/Tool31", NULL, menuSig, IDM_TOOLS + 31, 0},
	                                      {"/Tools/Tool32", NULL, menuSig, IDM_TOOLS + 32, 0},
	                                      {"/Tools/Tool33", NULL, menuSig, IDM_TOOLS + 33, 0},
	                                      {"/Tools/Tool34", NULL, menuSig, IDM_TOOLS + 34, 0},
	                                      {"/Tools/Tool35", NULL, menuSig, IDM_TOOLS + 35, 0},
	                                      {"/Tools/Tool36", NULL, menuSig, IDM_TOOLS + 36, 0},
	                                      {"/Tools/Tool37", NULL, menuSig, IDM_TOOLS + 37, 0},
	                                      {"/Tools/Tool38", NULL, menuSig, IDM_TOOLS + 38, 0},
	                                      {"/Tools/Tool39", NULL, menuSig, IDM_TOOLS + 39, 0},

	                                      {"/Tools/Tool40", NULL, menuSig, IDM_TOOLS + 40, 0},
	                                      {"/Tools/Tool41", NULL, menuSig, IDM_TOOLS + 41, 0},
	                                      {"/Tools/Tool42", NULL, menuSig, IDM_TOOLS + 42, 0},
	                                      {"/Tools/Tool43", NULL, menuSig, IDM_TOOLS + 43, 0},
	                                      {"/Tools/Tool44", NULL, menuSig, IDM_TOOLS + 44, 0},
	                                      {"/Tools/Tool45", NULL, menuSig, IDM_TOOLS + 45, 0},
	                                      {"/Tools/Tool46", NULL, menuSig, IDM_TOOLS + 46, 0},
	                                      {"/Tools/Tool47", NULL, menuSig, IDM_TOOLS + 47, 0},
	                                      {"/Tools/Tool48", NULL, menuSig, IDM_TOOLS + 48, 0},
	                                      {"/Tools/Tool49", NULL, menuSig, IDM_TOOLS + 49, 0},


	                                      {"/Tools/_Stop Executing", "<control>.", menuSig, IDM_STOPEXECUTE, NULL},
	                                      {"/Tools/sep1", NULL, NULL, 0, "<Separator>"},
	                                      {"/Tools/_Next Message", "F4", menuSig, IDM_NEXTMSG, 0},
	                                      {"/Tools/_Previous Message", "<shift>F4", menuSig, IDM_PREVMSG, 0},
	                                      {"/Tools/Clear _Output", "<shift>F5", menuSig, IDM_CLEAROUTPUT, 0},
	                                      {"/Tools/_Switch Pane", "<control>F6", menuSig, IDM_SWITCHPANE, 0},
	                                  };

	SciTEItemFactoryEntry menuItemsOptions[] = {
	            {"/_Options", NULL, NULL, 0, "<Branch>"},
	            {"/Options/Vertical _Split", "", menuSig, IDM_SPLITVERTICAL, "<CheckItem>"},
	            {"/Options/_Wrap", "", menuSig, IDM_WRAP, "<CheckItem>"},
	            {"/Options/Wrap Out_put", "", menuSig, IDM_WRAPOUTPUT, "<CheckItem>"},
	            {"/Options/_Read-Only", "", menuSig, IDM_READONLY, "<CheckItem>"},
	            {"/Options/sep1", NULL, NULL, 0, "<Separator>"},
	            {"/Options/_Line End Characters", "", 0, 0, "<Branch>"},
	            {"/Options/Line End Characters/CR _+ LF", "", menuSig, IDM_EOL_CRLF, "<RadioItem>"},
	            {"/Options/Line End Characters/_CR", "", menuSig, IDM_EOL_CR, "/Options/Line End Characters/CR + LF"},
	            {"/Options/Line End Characters/_LF", "", menuSig, IDM_EOL_LF, "/Options/Line End Characters/CR + LF"},
	            {"/Options/_Convert Line End Characters", "", menuSig, IDM_EOL_CONVERT, 0},
	            {"/Options/sep2", NULL, NULL, 0, "<Separator>"},
	            {"/Options/Change Inden_tation Settings", "<control><shift>I", menuSig, IDM_TABSIZE, 0},
	            {"/Options/Use _Monospaced Font", "<control>F11", menuSig, IDM_MONOFONT, "<CheckItem>"},
	            {"/Options/sep3", NULL, NULL, 0, "<Separator>"},
	            {"/Options/Open Local _Options File", "", menuSig, IDM_OPENLOCALPROPERTIES, 0},
	            {"/Options/Open _Directory Options File", "", menuSig, IDM_OPENDIRECTORYPROPERTIES, 0},
	            {"/Options/Open _User Options File", "", menuSig, IDM_OPENUSERPROPERTIES, 0},
	            {"/Options/Open _Global Options File", "", menuSig, IDM_OPENGLOBALPROPERTIES, 0},
	            {"/Options/Open A_bbreviations File", "", menuSig, IDM_OPENABBREVPROPERTIES, 0},
	            {"/Options/Open Lua Startup Scr_ipt", "", menuSig, IDM_OPENLUAEXTERNALFILE, 0},
	            {"/Options/sep4", NULL, NULL, 0, "<Separator>"},
	            {"/Options/_Edit Properties", "", 0, 0, "<Branch>"},
	        };

	SciTEItemFactoryEntry menuItemsLanguage[] = {
	            {"/_Language", NULL, NULL, 0, "<Branch>"},
	        };

	SciTEItemFactoryEntry menuItemsBuffer[] = {
	                                            {"/_Buffers", NULL, NULL, 0, "<Branch>"},
	                                            {"/Buffers/_Previous", "<shift>F6", menuSig, IDM_PREVFILE, 0},
	                                            {"/Buffers/_Next", "F6", menuSig, IDM_NEXTFILE, 0},
	                                            {"/Buffers/_Close All", "", menuSig, IDM_CLOSEALL, 0},
	                                            {"/Buffers/_Save All", "", menuSig, IDM_SAVEALL, 0},
	                                            {"/Buffers/sep2", NULL, NULL, 0, "<Separator>"},
	                                            {"/Buffers/Buffer0", "<alt>1", menuSig, bufferCmdID + 0, "<RadioItem>"},
	                                            {"/Buffers/Buffer1", "<alt>2", menuSig, bufferCmdID + 1, "/Buffers/Buffer0"},
	                                            {"/Buffers/Buffer2", "<alt>3", menuSig, bufferCmdID + 2, "/Buffers/Buffer0"},
	                                            {"/Buffers/Buffer3", "<alt>4", menuSig, bufferCmdID + 3, "/Buffers/Buffer0"},
	                                            {"/Buffers/Buffer4", "<alt>5", menuSig, bufferCmdID + 4, "/Buffers/Buffer0"},
	                                            {"/Buffers/Buffer5", "<alt>6", menuSig, bufferCmdID + 5, "/Buffers/Buffer0"},
	                                            {"/Buffers/Buffer6", "<alt>7", menuSig, bufferCmdID + 6, "/Buffers/Buffer0"},
	                                            {"/Buffers/Buffer7", "<alt>8", menuSig, bufferCmdID + 7, "/Buffers/Buffer0"},
	                                            {"/Buffers/Buffer8", "<alt>9", menuSig, bufferCmdID + 8, "/Buffers/Buffer0"},
	                                            {"/Buffers/Buffer9", "<alt>0", menuSig, bufferCmdID + 9, "/Buffers/Buffer0"},
	                                        };

	SciTEItemFactoryEntry menuItemsHelp[] = {
	                                          {"/_Help", NULL, NULL, 0, "<Branch>"},
	                                          {"/Help/_Help", "F1", menuSig, IDM_HELP, 0},
	                                          {"/Help/_SciTE Help", "", menuSig, IDM_HELP_SCITE, 0},
	                                          {"/Help/_About SciTE", "", menuSig, IDM_ABOUT, 0},
	                                      };

	accelGroup = gtk_accel_group_new();

	menuBar = gtk_menu_bar_new();
	CreateTranslatedMenu(std::size(menuItems), menuItems);

	CreateTranslatedMenu(std::size(menuItemsOptions), menuItemsOptions,
	                     50, "/Options/Edit Properties/Props", 0, IDM_IMPORT, 0);
	CreateTranslatedMenu(std::size(menuItemsLanguage), menuItemsLanguage,
	                     100, "/Language/Language", 0, IDM_LANGUAGE, 0);
	if (props.GetInt("buffers") > 1)
		CreateTranslatedMenu(std::size(menuItemsBuffer), menuItemsBuffer,
		                     bufferMax - 10, "/Buffers/Buffer", 10, bufferCmdID, "/Buffers/Buffer0");
	CreateTranslatedMenu(std::size(menuItemsHelp), menuItemsHelp);
	gtk_window_add_accel_group(GTK_WINDOW(PWidget(wSciTE)), accelGroup);
}

SystemAppearance SciTEGTK::CurrentAppearance() const noexcept {
	SystemAppearance currentAppearance{};
	gchar *themeName = nullptr;
	g_object_get(settings, "gtk-theme-name", &themeName, nullptr);
	currentAppearance.dark = g_str_has_suffix(themeName, "-dark");
	currentAppearance.highContrast = g_str_has_prefix(themeName, "HighContrast");
	if (g_strcmp0(themeName, "HighContrastInverse") == 0) {
		currentAppearance.dark = true;
	}
	//fprintf(stderr, "Theme '%s' %d %d\n", themeName, currentAppearance.dark, currentAppearance.highContrast);
	g_free(themeName);
	return currentAppearance;
}

void SciTEGTK::CreateStrips(GtkWidget *boxMain) {
	backgroundStrip.SetLocalizer(&localiser);
	backgroundStrip.Creation(boxMain);

	userStrip.SetUserStripWatcher(this);
	userStrip.SetExtender(extender);
	userStrip.SetLocalizer(&localiser);
	userStrip.Creation(boxMain);

	findStrip.SetLocalizer(&localiser);
	findStrip.SetSearcher(this);
	findStrip.Creation(boxMain);

	replaceStrip.SetLocalizer(&localiser);
	replaceStrip.SetSearcher(this);
	replaceStrip.Creation(boxMain);

	filterStrip.SetLocalizer(&localiser);
	filterStrip.SetSearcher(this);
	filterStrip.Creation(boxMain);
}

bool SciTEGTK::StripHasFocus() const {
	return findStrip.VisibleHasFocus() || replaceStrip.VisibleHasFocus() || filterStrip.VisibleHasFocus() || userStrip.VisibleHasFocus();
}

void SciTEGTK::LayoutUI() {
	if (splitPane) {
		// If GtkOrientable is available (GTK 2.16 and newer), just switch the
		// orientation if needed, don't recreate the H/VPaned
		GtkOrientation orient;
		orient = (splitVertical) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
		gtk_orientable_set_orientation(GTK_ORIENTABLE(splitPane), orient);
		return;
	}

	if (splitVertical) {
#if GTK_CHECK_VERSION(3,0,0)
		splitPane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#else
		splitPane = gtk_hpaned_new();
#endif
		gtk_widget_set_size_request(PWidget(wOutput), heightOutput, -1);
	} else {
#if GTK_CHECK_VERSION(3,0,0)
		splitPane = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#else
		splitPane = gtk_vpaned_new();
#endif
		gtk_widget_set_size_request(PWidget(wOutput), -1, heightOutput);
	}

#if GTK_CHECK_VERSION(3,16,0)
	gtk_paned_set_wide_handle(GTK_PANED(splitPane), props.GetInt("split.wide", 0));
#endif
	gtk_container_add(GTK_CONTAINER(PWidget(wContent)), GTK_WIDGET(splitPane));
	gtk_paned_pack1(GTK_PANED(splitPane), PWidget(wEditor), TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(splitPane), PWidget(wOutput), FALSE, TRUE);

	g_signal_connect(GTK_WIDGET(splitPane), "button-release-event",
	                   G_CALLBACK(PaneButtonRelease), static_cast<void *>(this));

	g_signal_connect(G_OBJECT(splitPane), "notify::position",
	                   G_CALLBACK(PanePositionChanged), static_cast<void *>(this));

	gtk_widget_show_all(GTK_WIDGET(splitPane));
}

void SciTEGTK::CreateUI() {
#if !GTK_CHECK_VERSION(3,0,0)
	gtk_rc_parse_string(
		"style \"toggler-style\" {\n"
		"    GtkWidget::focus-padding = 0\n"
		"    GtkWidget::focus-line-width = 0\n"
		"    xthickness = 0\n"
		"    ythickness = 0\n"
		"}\n"
		"widget \"*.toggler\" style \"toggler-style\"\n"
	);
#endif

	CreateBuffers();
	wSciTE = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	void *gthis = static_cast<void *>(this);

	gtk_widget_set_events(PWidget(wSciTE),
	                      GDK_EXPOSURE_MASK
	                      | GDK_LEAVE_NOTIFY_MASK
	                      | GDK_BUTTON_PRESS_MASK
	                      | GDK_BUTTON_RELEASE_MASK
	                     );
	g_signal_connect(G_OBJECT(PWidget(wSciTE)), "delete_event",
	                   G_CALLBACK(QuitSignal), gthis);

	g_signal_connect(G_OBJECT(PWidget(wSciTE)), "key_press_event",
	                   G_CALLBACK(KeyPress), gthis);

	g_signal_connect(G_OBJECT(PWidget(wSciTE)), "key-release-event",
	                   G_CALLBACK(KeyRelease), gthis);

	g_signal_connect(G_OBJECT(PWidget(wSciTE)), "button_press_event",
	                   G_CALLBACK(MousePress), gthis);

	gtk_window_set_title(GTK_WINDOW(PWidget(wSciTE)), appName);
	const int useDefault = 0x10000000;
	int left = props.GetInt("position.left", useDefault);
	int top = props.GetInt("position.top", useDefault);
	int width = props.GetInt("position.width", useDefault);
	int height = props.GetInt("position.height", useDefault);
	bool maximize = props.GetInt("position.maximize", 0) ? true : false;
	if (width == -1 || height == -1) {
		maximize = true;
#if GTK_CHECK_VERSION(3,22,0)
		GdkDisplay *pdisplay = gdk_display_get_default();
		// gdk_display_get_primary_monitor would be a better call
		// but that returned NULL on Fedora 25.
		// There should always be a monitor numbered 0.
		// At this point, wSciTE hasn't become a real window.
		GdkMonitor *monitor = gdk_display_get_monitor(pdisplay, 0);
		GdkRectangle rcScreen;
		gdk_monitor_get_geometry(monitor, &rcScreen);
		width = rcScreen.width - left - 10;
		height = rcScreen.height - top - 30;
#else
		width = gdk_screen_width() - left - 10;
		height = gdk_screen_height() - top - 30;
#endif
	}

	if (props.GetInt("save.position")) {
		left = propsSession.GetInt("position.left", useDefault);
		top = propsSession.GetInt("position.top", useDefault);
		width = propsSession.GetInt("position.width", useDefault);
		height = propsSession.GetInt("position.height", useDefault);
		maximize = propsSession.GetInt("position.maximize", 0) ? true : false;
	}

	fileSelectorWidth = props.GetInt("fileselector.width", fileSelectorWidth);
	fileSelectorHeight = props.GetInt("fileselector.height", fileSelectorHeight);

#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget *boxMain = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(boxMain), FALSE);
#else
	GtkWidget *boxMain = gtk_vbox_new(FALSE, 0);
#endif
	gtk_container_add(GTK_CONTAINER(PWidget(wSciTE)), boxMain);
	gtk_widget_set_can_focus(boxMain, FALSE);

 	// The Menubar
	CreateMenu();
	gtk_box_pack_start(GTK_BOX(boxMain), menuBar, FALSE, FALSE, 0);

	// The Toolbar
	wToolBar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(PWidget(wToolBar)), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(boxMain), PWidget(wToolBar), FALSE, FALSE, 0);
	gtk_widget_hide(GTK_WIDGET(PWidget(wToolBar)));
	gtk_container_set_border_width(GTK_CONTAINER(PWidget(wToolBar)), 0);
	tbVisible = false;

	// The Notebook (GTK2)
	wTabBar = gtk_notebook_new();
	gtk_widget_set_can_focus(PWidget(wTabBar), FALSE);
	gtk_box_pack_start(GTK_BOX(boxMain),PWidget(wTabBar),FALSE,FALSE,0);
	g_signal_connect(G_OBJECT(PWidget(wTabBar)),
		"button-release-event", G_CALLBACK(TabBarReleaseSignal), gthis);
	g_signal_connect(G_OBJECT(PWidget(wTabBar)),
		"scroll-event", G_CALLBACK(TabBarScrollSignal), gthis);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(PWidget(wTabBar)), TRUE);
	tabVisible = false;

#if GTK_CHECK_VERSION(3,0,0)
	wContent = gtk_event_box_new();
#else
	wContent = gtk_alignment_new(0, 0, 1, 1);
#endif

	// Ensure the content area is viable at 60 pixels high
	gtk_widget_set_size_request(PWidget(wContent), 20, 60);
	gtk_widget_set_can_focus(PWidget(wContent), FALSE);
	gtk_box_pack_start(GTK_BOX(boxMain), PWidget(wContent), TRUE, TRUE, 0);

	wEditor.SetScintilla(scintilla_new());
	g_object_ref(G_OBJECT(PWidget(wEditor)));
	scintilla_set_id(SCINTILLA(PWidget(wEditor)), IDM_SRCWIN);
	wEditor.UsePopUp(SA::PopUp::Never);

	g_signal_connect(G_OBJECT(PWidget(wEditor)), SCINTILLA_NOTIFY,
	                   G_CALLBACK(NotifySignal), this);


	wOutput.SetScintilla(scintilla_new());
	g_object_ref(G_OBJECT(PWidget(wOutput)));
	scintilla_set_id(SCINTILLA(PWidget(wOutput)), IDM_RUNWIN);
	wOutput.UsePopUp(SA::PopUp::Never);
	g_signal_connect(G_OBJECT(PWidget(wOutput)), SCINTILLA_NOTIFY,
	                   G_CALLBACK(NotifySignal), this);

	splitVertical = props.GetInt("split.vertical", 0);
	LayoutUI();

	WTable table(1, 3);
	wIncrementPanel = table;
	table.PackInto(GTK_BOX(boxMain), false);
	table.Label(TranslatedLabel("Find:"));

	IncSearchEntry = gtk_entry_new();
	table.Add(IncSearchEntry, 1, true, 5, 1);
	Signal<&SciTEGTK::FindIncrementCompleteCmd> sigFindIncrementComplete;
	g_signal_connect(G_OBJECT(IncSearchEntry),"activate", G_CALLBACK(sigFindIncrementComplete.Function), this);
	g_signal_connect(G_OBJECT(IncSearchEntry), "key-press-event", G_CALLBACK(FindIncrementEscapeSignal), this);
	Signal<&SciTEGTK::FindIncrementChanged> sigFindIncrementChanged;
	g_signal_connect(G_OBJECT(IncSearchEntry),"changed", G_CALLBACK(sigFindIncrementChanged.Function), this);
	g_signal_connect(G_OBJECT(IncSearchEntry),"focus-out-event", G_CALLBACK(FindIncrementFocusOutSignal), NULL);
	gtk_widget_show(IncSearchEntry);

	GUI::gui_string translated = localiser.Text("Find Next");
	IncSearchBtnNext = gtk_button_new_with_mnemonic(translated.c_str());
	table.Add(IncSearchBtnNext, 1, false, 5, 1);
	g_signal_connect(G_OBJECT(IncSearchBtnNext), "clicked",
		G_CALLBACK(sigFindIncrementComplete.Function), this);
	gtk_widget_show(IncSearchBtnNext);

	CreateStrips(boxMain);

	wOutput.SetMarginWidthN(1, 0);

	wStatusBar = gtk_statusbar_new();
	sbContextID = gtk_statusbar_get_context_id(
	                  GTK_STATUSBAR(PWidget(wStatusBar)), "global");
	gtk_box_pack_start(GTK_BOX(boxMain), PWidget(wStatusBar), FALSE, FALSE, 0);
	gtk_statusbar_push(GTK_STATUSBAR(PWidget(wStatusBar)), sbContextID, "Initial");
	sbVisible = false;

	static const GtkTargetEntry dragtypes[] = { { const_cast<gchar*>("text/uri-list"), 0, 0 } };
	static const gint n_dragtypes = std::size(dragtypes);

	gtk_drag_dest_set(PWidget(wSciTE), GTK_DEST_DEFAULT_ALL, dragtypes,
	                  n_dragtypes, GDK_ACTION_COPY);
	g_signal_connect(G_OBJECT(PWidget(wSciTE)), "drag_data_received",
	                         G_CALLBACK(DragDataReceived), this);

	SetFocus(wOutput);

	if ((left != useDefault) && (top != useDefault))
		gtk_window_move(GTK_WINDOW(PWidget(wSciTE)), left, top);
	if ((width != useDefault) && (height != useDefault))
		gtk_window_set_default_size(GTK_WINDOW(PWidget(wSciTE)), width, height);
	SetIcon();
	gtk_widget_show_all(PWidget(wSciTE));

	if (maximize)
		gtk_window_maximize(GTK_WINDOW(PWidget(wSciTE)));

	gtk_widget_hide(PWidget(backgroundStrip));
	gtk_widget_hide(PWidget(userStrip));
	gtk_widget_hide(wIncrementPanel);
	gtk_widget_hide(PWidget(findStrip));
	gtk_widget_hide(PWidget(replaceStrip));
	gtk_widget_hide(PWidget(filterStrip));

#if GTK_CHECK_VERSION(3,0,0)
	// Create a named style "entryInvalid" that can be applied to a search entry
	// when the text can not be found.
	GtkCssProvider *provider = gtk_css_provider_new();
	GdkScreen *screen = gtk_widget_get_screen(PWidget(wSciTE));
	gtk_style_context_add_provider_for_screen(screen,
		GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider),
		"#toggler {\n"
		"    padding: 2px;\n"
		"}\n"
		"#entryInvalid {\n"
		"    color: #FFFFFF;\n"
		"    background: #FF6666;\n"
		"}\n"
		"#entryInvalid:selected {\n"
		"    background: #771111;\n"
		"}\n", -1, NULL);
	g_object_unref(provider);
#endif

	UIAvailable();
}

void SciTEGTK::CloseOtherFinders(int cmdID) {
	if (cmdID != IDM_FIND)
		findStrip.Close();
	if (cmdID != IDM_REPLACE)
		replaceStrip.Close();
	if (cmdID != IDM_INCSEARCH)
		gtk_widget_hide(wIncrementPanel);
	if (cmdID != IDM_FILTER)
		filterStrip.Close();
}

void SciTEGTK::FindIncrementSetColour(bool valid) {
	WEntry::SetValid(GTK_ENTRY(IncSearchEntry), valid);
}

void SciTEGTK::FindIncrementNext(bool select) {
	if (select) {
		MoveBack();
	}
	const char *lineEntry = gtk_entry_get_text(GTK_ENTRY(IncSearchEntry));
	findWhat = lineEntry;
	wholeWord = false;
	if (FindHasText()) {
		FindNext(false, false);
		if (!select) {
			SetCaretAsStart();
		}
	}
	FindIncrementSetColour(!FindHasText() || !failedfind);
}

void SciTEGTK::FindIncrementChanged() {
	FindIncrementNext(true);
}

gboolean SciTEGTK::FindIncrementEscapeSignal(GtkWidget *w, GdkEventKey *event, SciTEGTK *scitew) {
	if (event->keyval == GKEY_Escape) {
		g_signal_stop_emission_by_name(G_OBJECT(w), "key-press-event");
		gtk_widget_hide(scitew->wIncrementPanel);
		SetFocus(scitew->wEditor);
	}
	return FALSE;
}

void SciTEGTK::FindIncrementCompleteCmd() {
	FindIncrementNext(false);
}

gboolean SciTEGTK::FindIncrementFocusOutSignal(GtkWidget *) {
#if GTK_CHECK_VERSION(3,0,0)
	//gtk_widget_hide(gtk_widget_get_parent(w));
#else
	//gtk_widget_hide(w->parent);
#endif
	return FALSE;
}

void SciTEGTK::FindIncrement() {
	CloseOtherFinders(IDM_INCSEARCH);
	FindIncrementSetColour(true);
	failedfind = false;
	gtk_widget_show(wIncrementPanel);
	gtk_widget_grab_focus(GTK_WIDGET(IncSearchEntry));
	gtk_entry_set_text(GTK_ENTRY(IncSearchEntry), "");
}

void SciTEGTK::Filter() {
	if (!FilterShowing()) {
		// Save current folds
		SaveFolds(CurrentBuffer()->foldState);
		// Then unfold them all so filtering is only source of line hiding
		wEditor.FoldAll(SA::FoldAction::Expand);
	}
	SelectionIntoFind();
	CloseOtherFinders(IDM_FILTER);
	filterStrip.visible = true;
	failedfind = false;
	SizeSubWindows();
	filterStrip.Show(props.GetInt("strip.button.height", -1));
}

bool SciTEGTK::FilterShowing() {
	return filterStrip.visible || (replaceStrip.visible && filterState);
}

void SciTEGTK::SetIcon() {
	FilePath pathPixmap(PIXMAP_PATH, "Sci48M.png");
	GError *err=0;
	if (!gtk_window_set_icon_from_file(
		GTK_WINDOW(PWidget(wSciTE)), pathPixmap.AsInternal(), &err)) {
		// Failed to load from file so use backup inside executable
		GdkPixbuf *pixbufIcon = gdk_pixbuf_new_from_xpm_data(SciIcon_xpm);
		gtk_window_set_icon(GTK_WINDOW(PWidget(wSciTE)), pixbufIcon);
		g_object_unref(pixbufIcon);
	}
}

struct CallbackData {
	SciTEGTK *pSciTE;
	int cmd;
	Worker *pWorker;
	CallbackData(SciTEGTK *pSciTE_, int cmd_, Worker *pWorker_) noexcept :
		pSciTE(pSciTE_), cmd(cmd_), pWorker(pWorker_) {
	}
};

void SciTEGTK::PostOnMainThread(int cmd, Worker *pWorker) {
	CallbackData *pcbd = new CallbackData(this, cmd, pWorker);
	g_idle_add(PostCallback, pcbd);
}

gboolean SciTEGTK::PostCallback(void *ptr) {
#ifndef GDK_VERSION_3_6
	ThreadLockMinder minder;
#endif
	CallbackData *pcbd = static_cast<CallbackData *>(ptr);
	pcbd->pSciTE->WorkerCommand(pcbd->cmd, pcbd->pWorker);
	delete pcbd;
	return FALSE;
}

void SciTEGTK::SetStartupTime(const char *timestamp) {
	if (timestamp != NULL) {
		char *end;
		// Reset errno from any previous errors
		errno = 0;
		const gulong ts = strtoul(timestamp, &end, 0);
		if (end != timestamp && errno == 0) {
			startupTimestamp = ts;
		}
	}
}

// Send the filename through the pipe using the director command "open:"
// Make the path absolute if it is not already.
// If filename is empty, we send a message to the existing instance to tell
// it to present itself (ie. the window should come to the front)
void SciTEGTK::SendFileName(int sendPipe, const char* filename) {

	std::string command;
	const char *pipeData;

	if (strlen(filename) != 0) {
		command = "open:";

		// Check to see if path is already absolute.
		if (!g_path_is_absolute(filename)) {
			gchar *currentPath = g_get_current_dir();
			command += currentPath;
			command += '/';
			g_free(currentPath);
		}
		command += filename;
		command += "\n";

	} else {
		command = "focus:";

		if (startupTimestamp != 0) {
			char timestamp[14];
			snprintf(timestamp, 14, "%u", startupTimestamp);
			command += timestamp;
		}
		command += "\n";
	}
	pipeData = command.c_str();

	// Send it.
	if (write(sendPipe, pipeData, strlen(pipeData)) == -1)
		perror("Unable to write to pipe");
}

bool SciTEGTK::CheckForRunningInstance(int argc, char *argv[]) {

	const gchar *tmpdir = g_get_tmp_dir();
	GDir *dir = g_dir_open(tmpdir, 0, NULL);
	if (dir == NULL) {
		return false; // Couldn't open the directory
	}

	GPatternSpec *pattern = g_pattern_spec_new("SciTE.*.in");

	char *pipeFileName = NULL;
	const char *filename;
	uniqueInstance = g_get_tmp_dir();
	uniqueInstance += "/SciTE.ensure.unique.instance.for.";
	uniqueInstance += getenv("USER");
	int fd;
	bool isLocked;
	do {
		fd = open(uniqueInstance.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR); // Try to set the lock.
		isLocked = (fd == -1 && errno == EEXIST);
		if (isLocked) {
			// There is already a lock.
			time_t ltime;
			time(&ltime);// Get current time.
			struct stat file_status;
			stat(uniqueInstance.c_str(), &file_status); // Get status of lock file.
			isLocked = difftime(ltime, file_status.st_mtime) <= 3.0; // Test whether the lock is fresh (<= 3 seconds) or not. Avoid perpetual lock if SciTE crashes during its launch and the lock file is present.
			if (isLocked)
				// Currently, another process of SciTE is launching. We are waiting for end of its initialisation.
				usleep(50000);
		}
	} while (isLocked);
	if (fd != -1)
		close(fd);
	// Find a working pipe in our temporary directory
	while ((filename = g_dir_read_name(dir))) {
		if (g_pattern_match_string(pattern, filename)) {
			pipeFileName = g_build_filename(tmpdir, filename, NULL);

			// Attempt to open the pipe as a writer to send out data.
			const int sendPipe = open(pipeFileName, O_WRONLY | O_NONBLOCK);

			// If open succeeded, write filename data.
			if (sendPipe != -1) {
				// Mimic the win32 behaviour, see UniqueInstance::SendCommands
				for (int ii = 1; ii < argc; ++ii) {
					if (argv[ii][0] != '-') {
						SendFileName(sendPipe, argv[ii]);
					} else {
						// Strip the beginning dash and add the final newline
						if ((write(sendPipe, argv[ii]+1, strlen(argv[ii])-1) == -1) || (write(sendPipe, "\n", 1) == -1))
							perror("Unable to write command to pipe");
					}
				}

				// Force the SciTE instance to come to the front.
				SendFileName(sendPipe, "");

				// We're done
				if (close(sendPipe) == -1)
					perror("Unable to close pipe");
				break;
			} else {
				// We don't care about the error. Try another pipe.
				g_free(pipeFileName);
				pipeFileName = NULL;
			}
		}
	}
	g_pattern_spec_free(pattern);
	g_dir_close(dir);

	if (pipeFileName != NULL) {
		// We need to call this since we're not displaying a window
		unlink(uniqueInstance.c_str()); // Unlock.
		gdk_notify_startup_complete();
		g_free(pipeFileName);
		return true;
	}

	// If we arrived here, there is no SciTE instance we could talk to.
	// We'll start a new one
	return false;
}

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

// Detect if the tool has exited without producing any output
int SciTEGTK::PollTool(SciTEGTK *scitew) {
#ifndef GDK_VERSION_3_6
	ThreadLockMinder minder;
#endif
	scitew->ContinueExecute(TRUE);
	return TRUE;
}

int main(int argc, char *argv[]) {
#ifdef NO_EXTENSIONS
	Extension *extender = 0;
#else
	MultiplexExtension multiExtender;
	Extension *extender = &multiExtender;

#ifndef NO_LUA
	multiExtender.RegisterExtension(LuaExtension::Instance());
#endif
#ifndef NO_FILER
	multiExtender.RegisterExtension(DirectorExtension::Instance());
#endif
#endif

	// Initialise threads
#if !GLIB_CHECK_VERSION(2,31,0)
	g_thread_init(NULL);
#endif
#ifndef GDK_VERSION_3_6
	gdk_threads_init();
#endif

	if (argc > 0) {
		executableDirectory = FilePath(argv[0]).AbsolutePath().Directory();
	}
#if __linux__
	char selfExe[PATH_MAX] = "";
	if (readlink("/proc/self/exe", selfExe, PATH_MAX) == -1) {
		perror("readlink");
	} else {
		executableDirectory = FilePath(selfExe).Directory();
	}
#endif

	// Get this now because gtk_init() clears it
	const gchar *startup_id = g_getenv("DESKTOP_STARTUP_ID");
	char *timestamp = NULL;
	if (startup_id != NULL) {
		char *pos = g_strrstr(startup_id, "_TIME");
		if (pos != NULL) {
			timestamp = pos + 5; // Skip "_TIME"
		}
	}

	gtk_init(&argc, &argv);

	SciTEGTK scite(extender);
	scite.SetStartupTime(timestamp);
	scite.Run(argc, argv);
	scite.Finalise();

	return 0;
}
