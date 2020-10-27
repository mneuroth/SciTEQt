// SciTE - Scintilla based Text Editor
/** @file SciTEWin.cxx
 ** Main code for the Windows version of the editor.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <time.h>

#include "SciTEWin.h"
#include "DLLFunction.h"

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifndef NO_EXTENSIONS
#include "MultiplexExtension.h"

#ifndef NO_FILER
#include "DirectorExtension.h"
#endif

#ifndef NO_LUA
#include "LuaExtension.h"
#endif

#endif

#ifdef STATIC_BUILD
const GUI::gui_char appName[] = GUI_TEXT("Sc1");
#else
const GUI::gui_char appName[] = GUI_TEXT("SciTE");
#ifdef LOAD_SCINTILLA
static const GUI::gui_char scintillaName[] = GUI_TEXT("Scintilla.DLL");
#else
static const GUI::gui_char scintillaName[] = GUI_TEXT("SciLexer.DLL");
#endif
#endif

static GUI::gui_string GetErrorMessage(DWORD nRet) {
	LPWSTR lpMsgBuf = nullptr;
	if (::FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				nRet,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),   // Default language
				reinterpret_cast<LPWSTR>(&lpMsgBuf),
				0,
				nullptr
			) != 0) {
		GUI::gui_string s= lpMsgBuf;
		::LocalFree(lpMsgBuf);
		return s;
	} else {
		return TEXT("");
	}
}

long SciTEKeys::ParseKeyCode(const char *mnemonic) {
	SA::KeyMod modsInKey = static_cast<SA::KeyMod>(0);
	int keyval = -1;

	if (mnemonic && *mnemonic) {
		std::string sKey = mnemonic;

		if (RemoveStringOnce(sKey, "Ctrl+"))
			modsInKey = modsInKey | SA::KeyMod::Ctrl;
		if (RemoveStringOnce(sKey, "Shift+"))
			modsInKey = modsInKey | SA::KeyMod::Shift;
		if (RemoveStringOnce(sKey, "Alt+"))
			modsInKey = modsInKey | SA::KeyMod::Alt;

		if (sKey.length() == 1) {
			keyval = VkKeyScan(sKey.at(0)) & 0xFF;
		} else if (sKey.length() > 1) {
			if ((sKey.at(0) == 'F') && (IsADigit(sKey.at(1)))) {
				sKey.erase(0, 1);
				const int fkeyNum = atoi(sKey.c_str());
				if (fkeyNum >= 1 && fkeyNum <= 12)
					keyval = fkeyNum - 1 + VK_F1;
			} else if ((sKey.at(0) == 'V') && (IsADigit(sKey.at(1)))) {
				sKey.erase(0, 1);
				const int vkey = atoi(sKey.c_str());
				if (vkey > 0 && vkey <= 0x7FFF)
					keyval = vkey;
			} else if (StartsWith(sKey, "Keypad")) {
				sKey.erase(0, strlen("Keypad"));
				if ((sKey.length() > 0) && IsADigit(sKey.at(0))) {
					const int keyNum = atoi(sKey.c_str());
					if (keyNum >= 0 && keyNum <= 9)
						keyval = keyNum + VK_NUMPAD0;
				} else if (sKey == "Plus") {
					keyval = VK_ADD;
				} else if (sKey == "Minus") {
					keyval = VK_SUBTRACT;
				} else if (sKey == "Decimal") {
					keyval = VK_DECIMAL;
				} else if (sKey == "Divide") {
					keyval = VK_DIVIDE;
				} else if (sKey == "Multiply") {
					keyval = VK_MULTIPLY;
				}
			} else if (sKey == "Left") {
				keyval = VK_LEFT;
			} else if (sKey == "Right") {
				keyval = VK_RIGHT;
			} else if (sKey == "Up") {
				keyval = VK_UP;
			} else if (sKey == "Down") {
				keyval = VK_DOWN;
			} else if (sKey == "Insert") {
				keyval = VK_INSERT;
			} else if (sKey == "End") {
				keyval = VK_END;
			} else if (sKey == "Home") {
				keyval = VK_HOME;
			} else if (sKey == "Enter") {
				keyval = VK_RETURN;
			} else if (sKey == "Space") {
				keyval = VK_SPACE;
			} else if (sKey == "Tab") {
				keyval = VK_TAB;
			} else if (sKey == "Escape") {
				keyval = VK_ESCAPE;
			} else if (sKey == "Delete") {
				keyval = VK_DELETE;
			} else if (sKey == "PageUp") {
				keyval = VK_PRIOR;
			} else if (sKey == "PageDown") {
				keyval = VK_NEXT;
			} else if (sKey == "Win") {
				keyval = VK_LWIN;
			} else if (sKey == "Menu") {
				keyval = VK_APPS;
			} else if (sKey == "Backward") {
				keyval = VK_BROWSER_BACK;
			} else if (sKey == "Forward") {
				keyval = VK_BROWSER_FORWARD;
			}
		}
	}

	return (keyval > 0) ? (keyval | (static_cast<int>(modsInKey)<<16)) : 0;
}

bool SciTEKeys::MatchKeyCode(long parsedKeyCode, int keyval, int modifiers) noexcept {
	return parsedKeyCode && !(0xFFFF0000 & (keyval | modifiers)) && (parsedKeyCode == (keyval | (modifiers<<16)));
}

HINSTANCE SciTEWin::hInstance {};
const TCHAR *SciTEWin::className = nullptr;
const TCHAR *SciTEWin::classNameInternal = nullptr;
SciTEWin *SciTEWin::app = nullptr;

namespace {

using SystemParametersInfoForDpiSig = BOOL (WINAPI *)(
	UINT  uiAction,
	UINT  uiParam,
	PVOID pvParam,
	UINT  fWinIni,
	UINT  dpi
);

SystemParametersInfoForDpiSig fnSystemParametersInfoForDpi;

// Using VerifyVersionInfo on Windows 10 will pretend its Windows 8
// but that is good enough for switching UI elements to flatter.
// The VersionHelpers.h functions can't be used as they aren't supported by GCC.

bool UIShouldBeFlat() noexcept {
	OSVERSIONINFOEX osvi = OSVERSIONINFOEX();
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 2;

	constexpr BYTE op = VER_GREATER_EQUAL;
	DWORDLONG dwlConditionMask = 0;
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);

	return VerifyVersionInfo(
		       &osvi,
		       VER_MAJORVERSION | VER_MINORVERSION |
		       VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
		       dwlConditionMask);
}

}

SciTEWin::SciTEWin(Extension *ext) : SciTEBase(ext) {
	app = this;

	contents.SetSciTE(this);
	contents.SetLocalizer(&localiser);
	backgroundStrip.SetLocalizer(&localiser);
	searchStrip.SetLocalizer(&localiser);
	searchStrip.SetSearcher(this);
	findStrip.SetLocalizer(&localiser);
	findStrip.SetSearcher(this);
	replaceStrip.SetLocalizer(&localiser);
	replaceStrip.SetSearcher(this);

	flatterUI = UIShouldBeFlat();

	appearance = CurrentAppearance();

	cmdShow = 0;
	heightBar = 7;
	fontTabs = {};
	wFocus = {};

	winPlace = WINDOWPLACEMENT();
	winPlace.length = 0;
	rcWorkArea = RECT();

	openWhat[0] = '\0';
	tooltipText[0] = '\0';
	tbLarge = false;
	modalParameters = false;
	filterDefault = 1;
	staticBuild = false;
	menuSource = 0;

	hWriteSubProcess = {};
	subProcessGroupId = 0;

	pathAbbreviations = GetAbbrevPropertiesFileName();

	// System type properties are stored in the platform properties.
	propsPlatform.Set("PLAT_WIN", "1");
	propsPlatform.Set("PLAT_WINNT", "1");

	ReadEnvironment();

	SetScaleFactor(GetScaleFactor());

	ReadGlobalPropFile();

	if (props.GetInt("create.hidden.console")) {
		// Prevent a flashing console window whenever Lua calls os.execute or
		// io.popen by creating a hidden console to share.
		::AllocConsole();
		::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	}

	tbLarge = props.GetInt("toolbar.large");
	/// Need to copy properties to variables before setting up window
	SetPropertiesInitial();
	ReadAbbrevPropFile();

	hDevMode = {};
	hDevNames = {};
	::ZeroMemory(&pagesetupMargin, sizeof(pagesetupMargin));

	hHH = {};
	hMM = {};
	uniqueInstance.Init(this);

	hAccTable = ::LoadAccelerators(hInstance, TEXT("ACCELS")); // md

	cmdWorker.pSciTE = this;
}

SciTEWin::~SciTEWin() {
	if (hDevMode)
		::GlobalFree(hDevMode);
	if (hDevNames)
		::GlobalFree(hDevNames);
	if (hHH)
		::FreeLibrary(hHH);
	if (hMM)
		::FreeLibrary(hMM);
	if (fontTabs)
		::DeleteObject(fontTabs);
	if (hAccTable)
		::DestroyAcceleratorTable(hAccTable);
}

uintptr_t SciTEWin::GetInstance() {
	return reinterpret_cast<uintptr_t>(hInstance);
}

void SciTEWin::Register(HINSTANCE hInstance_) {
	const TCHAR resourceName[] = TEXT("SciTE");

	hInstance = hInstance_;

	WNDCLASS wndclass {};

	// Register the frame window
	className = TEXT("SciTEWindow");
	wndclass.style = 0;
	wndclass.lpfnWndProc = SciTEWin::TWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(SciTEWin *);
	wndclass.hInstance = hInstance;
	wndclass.hIcon = ::LoadIcon(hInstance, resourceName);
	wndclass.hCursor = {};
	wndclass.hbrBackground = {};
	wndclass.lpszMenuName = resourceName;
	wndclass.lpszClassName = className;
	if (!::RegisterClass(&wndclass))
		exit(FALSE);

	// Register the window that holds the two Scintilla edit windows and the separator
	classNameInternal = TEXT("SciTEWindowContent");
	wndclass.lpfnWndProc = BaseWin::StWndProc;
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = classNameInternal;
	if (!::RegisterClass(&wndclass))
		exit(FALSE);
}

static int CodePageFromName(const std::string &encodingName) {
	struct Encoding {
		const char *name;
		int codePage;
	} knownEncodings[] = {
		{ "ascii", CP_UTF8 },
		{ "utf-8", CP_UTF8 },
		{ "latin1", 1252 },
		{ "latin2", 28592 },
		{ "big5", 950 },
		{ "gbk", 936 },
		{ "shift_jis", 932 },
		{ "euc-kr", 949 },
		{ "cyrillic", 1251 },
		{ "iso-8859-5", 28595 },
		{ "iso8859-11", 874 },
		{ "1250", 1250 },
		{ "windows-1251", 1251 },
	};
	for (const Encoding &enc : knownEncodings) {
		if (encodingName == enc.name) {
			return enc.codePage;
		}
	}
	return CP_UTF8;
}

static std::string StringEncode(std::wstring s, int codePage) {
	if (s.length()) {
		const int sLength = static_cast<int>(s.length());
		const int cchMulti = ::WideCharToMultiByte(codePage, 0, s.c_str(), sLength, nullptr, 0, nullptr, nullptr);
		std::string sMulti(cchMulti, 0);
		::WideCharToMultiByte(codePage, 0, s.c_str(), sLength, &sMulti[0], cchMulti, nullptr, nullptr);
		return sMulti;
	} else {
		return std::string();
	}
}

static std::wstring StringDecode(std::string s, int codePage) {
	if (s.length()) {
		const int sLength = static_cast<int>(s.length());
		const int cchWide = ::MultiByteToWideChar(codePage, 0, s.c_str(), sLength, nullptr, 0);
		std::wstring sWide(cchWide, 0);
		::MultiByteToWideChar(codePage, 0, s.c_str(), sLength, &sWide[0], cchWide);
		return sWide;
	} else {
		return std::wstring();
	}
}

// Convert to UTF-8
static std::string ConvertEncoding(const char *original, int codePage) {
	if (codePage == CP_UTF8) {
		return original;
	} else {
		GUI::gui_string sWide = StringDecode(std::string(original), codePage);
		return GUI::UTF8FromString(sWide);
	}
}

void SciTEWin::ReadLocalization() {
	SciTEBase::ReadLocalization();
	std::string encoding = localiser.GetString("translation.encoding");
	LowerCaseAZ(encoding);
	if (encoding.length()) {
		const int codePageNamed = CodePageFromName(encoding);
		const char *key = nullptr;
		const char *val = nullptr;
		// Get encoding
		bool more = localiser.GetFirst(key, val);
		while (more) {
			std::string converted = ConvertEncoding(val, codePageNamed);
			if (converted != "") {
				localiser.Set(key, converted.c_str());
			}
			more = localiser.GetNext(key, val);
		}
	}
}

void SciTEWin::GetWindowPosition(int *left, int *top, int *width, int *height, int *maximize) {
	winPlace.length = sizeof(winPlace);
	::GetWindowPlacement(MainHWND(), &winPlace);

	*left = winPlace.rcNormalPosition.left;
	*top = winPlace.rcNormalPosition.top;
	*width =  winPlace.rcNormalPosition.right - winPlace.rcNormalPosition.left;
	*height = winPlace.rcNormalPosition.bottom - winPlace.rcNormalPosition.top;
	*maximize = (winPlace.showCmd == SW_MAXIMIZE) ? 1 : 0;
}

int SciTEWin::GetScaleFactor() noexcept {
	// Only called at startup, so just use the default for a DC
	HDC hdcMeasure = ::CreateCompatibleDC({});
	const int scale = ::GetDeviceCaps(hdcMeasure, LOGPIXELSY) * 100 / 96;
	::DeleteDC(hdcMeasure);
	return scale;
}

bool SciTEWin::SetScaleFactor(int scale) {
	const std::string sScale = StdStringFromInteger(scale);
	const std::string sCurrentScale = propsPlatform.GetString("ScaleFactor");
	if (sScale == sCurrentScale) {
		return false;
	}
	propsPlatform.Set("ScaleFactor", sScale);
	return true;
}

// Allow UTF-8 file names and command lines to be used in calls to io.open and io.popen in Lua scripts.
// The scite_lua_win.h header redirects fopen and _popen to these functions.

extern "C" {

	FILE *scite_lua_fopen(const char *filename, const char *mode) {
		GUI::gui_string sFilename = GUI::StringFromUTF8(filename);
		GUI::gui_string sMode = GUI::StringFromUTF8(mode);
		FILE *f = _wfopen(sFilename.c_str(), sMode.c_str());
		if (!f)
			// Fallback on narrow string in case already in CP_ACP
			f = fopen(filename, mode);
		return f;
	}

	FILE *scite_lua_popen(const char *filename, const char *mode) {
		GUI::gui_string sFilename = GUI::StringFromUTF8(filename);
		GUI::gui_string sMode = GUI::StringFromUTF8(mode);
		FILE *f = _wpopen(sFilename.c_str(), sMode.c_str());
		if (!f)
			// Fallback on narrow string in case already in CP_ACP
			f = _popen(filename, mode);
		return f;
	}

}

// Read properties resource into propsEmbed
// The embedded properties file is to allow distributions to be sure
// that they have a sensible default configuration even if the properties
// files are missing. Also allows a one file distribution of Sc1.EXE.
void SciTEWin::ReadEmbeddedProperties() {

	propsEmbed.Clear();

	HRSRC handProps = ::FindResource(hInstance, TEXT("Embedded"), TEXT("Properties"));
	if (handProps) {
		const DWORD size = ::SizeofResource(hInstance, handProps);
		HGLOBAL hmem = ::LoadResource(hInstance, handProps);
		if (hmem) {
			const void *pv = ::LockResource(hmem);
			if (pv) {
				propsEmbed.ReadFromMemory(
					static_cast<const char *>(pv), size, FilePath(), filter, NULL, 0);
			}
		}
		::FreeResource(handProps);
	}
}

SystemAppearance SciTEWin::CurrentAppearance() const noexcept {
	SystemAppearance currentAppearance{};

	HKEY hkeyPersonalize{};
	const LSTATUS statusOpen = ::RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		0, KEY_QUERY_VALUE, &hkeyPersonalize);
	if (statusOpen == ERROR_SUCCESS) {
		DWORD type = 0;
		DWORD val = 99;
		DWORD cbData = sizeof(val);
		const LSTATUS status = ::RegQueryValueExW(hkeyPersonalize, L"AppsUseLightTheme", nullptr,
			&type, reinterpret_cast<LPBYTE>(&val), reinterpret_cast<LPDWORD>(&cbData));
		RegCloseKey(hkeyPersonalize);
		if (status == ERROR_SUCCESS) {
			currentAppearance.dark = val == 0;
		}
	}

	HIGHCONTRAST info{};
	info.cbSize = sizeof(HIGHCONTRAST)
		;
	const BOOL status = SystemParametersInfoW(SPI_GETHIGHCONTRAST, 0, &info, 0);
	if (status) {
		currentAppearance.highContrast = (info.dwFlags & HCF_HIGHCONTRASTON) ? 1 : 0;
		if (currentAppearance.highContrast) {
			// With high contrast, AppsUseLightTheme not correct so examine system background colour
			const DWORD dwWindowColour = ::GetSysColor(COLOR_WINDOW);
			currentAppearance.dark = dwWindowColour < 0x40;
		}
	}

	return currentAppearance;
}

void SciTEWin::ReadPropertiesInitial() {
	SciTEBase::ReadPropertiesInitial();
	if (tabMultiLine) {	// Windows specific!
		const long wl = GetWindowStyle(HwndOf(wTabBar));
		::SetWindowLong(HwndOf(wTabBar), GWL_STYLE, wl | TCS_MULTILINE);
	}
}

void SciTEWin::ReadProperties() {
	SciTEBase::ReadProperties();
	if (flatterUI) {
		if (foldColour.empty() && foldHiliteColour.empty()) {
			constexpr SA::Colour lightMargin = ColourRGB(0xF7, 0xF7, 0xF7);
			CallChildren(SA::Message::SetFoldMarginColour, 1, lightMargin);
			CallChildren(SA::Message::SetFoldMarginHiColour, 1, lightMargin);
		}
	}
}

static FilePath GetSciTEPath(const FilePath &home) {
	if (home.IsSet()) {
		return FilePath(home);
	} else {
		GUI::gui_char path[MAX_PATH];
		if (::GetModuleFileNameW(0, path, static_cast<DWORD>(std::size(path))) == 0)
			return FilePath();
		// Remove the SciTE.exe
		GUI::gui_char *lastSlash = wcsrchr(path, pathSepChar);
		if (lastSlash)
			*lastSlash = '\0';
		return FilePath(path);
	}
}

FilePath SciTEWin::GetDefaultDirectory() {
	const GUI::gui_char *home = _wgetenv(GUI_TEXT("SciTE_HOME"));
	return GetSciTEPath(home);
}

FilePath SciTEWin::GetSciteDefaultHome() {
	const GUI::gui_char *home = _wgetenv(GUI_TEXT("SciTE_HOME"));
	return GetSciTEPath(home);
}

FilePath SciTEWin::GetSciteUserHome() {
	// First looking for environment variable $SciTE_USERHOME
	// to set SciteUserHome. If not present we look for $SciTE_HOME
	// then defaulting to $USERPROFILE
	GUI::gui_char *home = _wgetenv(GUI_TEXT("SciTE_USERHOME"));
	if (!home) {
		home = _wgetenv(GUI_TEXT("SciTE_HOME"));
		if (!home) {
			home = _wgetenv(GUI_TEXT("USERPROFILE"));
		}
	}
	return GetSciTEPath(home);
}

// Help command lines contain topic!path
void SciTEWin::ExecuteOtherHelp(const char *cmd) {
	GUI::gui_string s = GUI::StringFromUTF8(cmd);
	const size_t pos = s.find_first_of('!');
	if (pos != GUI::gui_string::npos) {
		GUI::gui_string topic = s.substr(0, pos);
		GUI::gui_string path = s.substr(pos+1);
		::WinHelpW(MainHWND(),
			   path.c_str(),
			   HELP_KEY,
			   reinterpret_cast<ULONG_PTR>(topic.c_str()));
	}
}

// HH_AKLINK not in mingw headers
struct XHH_AKLINK {
	long cbStruct;
	BOOL fReserved;
	const wchar_t *pszKeywords;
	wchar_t *pszUrl;
	wchar_t *pszMsgText;
	wchar_t *pszMsgTitle;
	wchar_t *pszWindow;
	BOOL fIndexOnFail;
};

// Help command lines contain topic!path
void SciTEWin::ExecuteHelp(const char *cmd) {
	if (!hHH)
		hHH = ::LoadLibrary(TEXT("HHCTRL.OCX"));

	if (hHH) {
		GUI::gui_string s = GUI::StringFromUTF8(cmd);
		const size_t pos = s.find_first_of('!');
		if (pos != GUI::gui_string::npos) {
			GUI::gui_string topic = s.substr(0, pos);
			GUI::gui_string path = s.substr(pos + 1);
			typedef HWND (WINAPI *HelpFn)(HWND, const wchar_t *, UINT, DWORD_PTR);
			HelpFn fnHHW = reinterpret_cast<HelpFn>(::GetProcAddress(hHH, "HtmlHelpW"));
			if (fnHHW) {
				XHH_AKLINK ak;
				ak.cbStruct = sizeof(ak);
				ak.fReserved = FALSE;
				ak.pszKeywords = topic.c_str();
				ak.pszUrl = nullptr;
				ak.pszMsgText = nullptr;
				ak.pszMsgTitle = nullptr;
				ak.pszWindow = nullptr;
				ak.fIndexOnFail = TRUE;
				fnHHW(NULL,
				      path.c_str(),
				      0x000d,          	// HH_KEYWORD_LOOKUP
				      reinterpret_cast<DWORD_PTR>(&ak)
				     );
			}
		}
	}
}

void SciTEWin::CopyAsRTF() {
	const SA::Range cr = GetSelection();
	std::ostringstream oss;
	SaveToStreamRTF(oss, cr.start, cr.end);
	const std::string rtf = oss.str();
	const size_t len = rtf.length() + 1;	// +1 for NUL
	HGLOBAL hand = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, len);
	if (hand) {
		::OpenClipboard(MainHWND());
		::EmptyClipboard();
		char *ptr = static_cast<char *>(::GlobalLock(hand));
		if (ptr) {
			memcpy(ptr, rtf.c_str(), len);
			::GlobalUnlock(hand);
		}
		::SetClipboardData(::RegisterClipboardFormat(CF_RTF), hand);
		::CloseClipboard();
	}
}

void SciTEWin::CopyPath() {
	if (filePath.IsUntitled())
		return;

	const GUI::gui_string clipText(filePath.AsInternal());
	const size_t blobSize = sizeof(GUI::gui_char)*(clipText.length()+1);
	if (::OpenClipboard(MainHWND())) {
		HGLOBAL hand = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, blobSize);
		if (hand) {
			::EmptyClipboard();
			GUI::gui_char *ptr = static_cast<GUI::gui_char *>(::GlobalLock(hand));
			if (ptr) {
				memcpy(ptr, clipText.c_str(), blobSize);
				::GlobalUnlock(hand);
			}
			::SetClipboardData(CF_UNICODETEXT, hand);
		}
		::CloseClipboard();
	}
}

void SciTEWin::FullScreenToggle() {
	HWND wTaskBar = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
	HWND wStartButton = FindWindow(TEXT("Button"), nullptr);
	fullScreen = !fullScreen;
	if (fullScreen) {
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
		::SystemParametersInfo(SPI_SETWORKAREA, 0, nullptr, SPIF_SENDCHANGE);
		if (wStartButton)
			::ShowWindow(wStartButton, SW_HIDE);
		::ShowWindow(wTaskBar, SW_HIDE);

		winPlace.length = sizeof(winPlace);
		::GetWindowPlacement(MainHWND(), &winPlace);
		int topStuff = ::GetSystemMetrics(SM_CYSIZEFRAME) + ::GetSystemMetrics(SM_CYCAPTION);
		if (props.GetInt("full.screen.hides.menu"))
			topStuff += ::GetSystemMetrics(SM_CYMENU);
		::SetWindowLongPtr(HwndOf(wContent),
				   GWL_EXSTYLE, 0);
		::SetWindowPos(MainHWND(), HWND_TOP,
			       -::GetSystemMetrics(SM_CXSIZEFRAME),
			       -topStuff,
			       ::GetSystemMetrics(SM_CXSCREEN) + 2 * ::GetSystemMetrics(SM_CXSIZEFRAME),
			       ::GetSystemMetrics(SM_CYSCREEN) + topStuff + ::GetSystemMetrics(SM_CYSIZEFRAME),
			       0);
	} else {
		::ShowWindow(wTaskBar, SW_SHOW);
		if (wStartButton)
			::ShowWindow(wStartButton, SW_SHOW);
		::SetWindowLongPtr(HwndOf(wContent),
				   GWL_EXSTYLE, WS_EX_CLIENTEDGE);
		if (winPlace.length) {
			::SystemParametersInfo(SPI_SETWORKAREA, 0, &rcWorkArea, 0);
			if (winPlace.showCmd == SW_SHOWMAXIMIZED) {
				::ShowWindow(MainHWND(), SW_RESTORE);
				::ShowWindow(MainHWND(), SW_SHOWMAXIMIZED);
			} else {
				::SetWindowPlacement(MainHWND(), &winPlace);
			}
		}
	}
	::SetForegroundWindow(MainHWND());
	CheckMenus();
}

HWND SciTEWin::MainHWND() noexcept {
	return HwndOf(wSciTE);
}

void SciTEWin::Command(WPARAM wParam, LPARAM lParam) {
	const int cmdID = ControlIDOfWParam(wParam);
	if (wParam & 0x10000) {
		// From accelerator -> goes to focused pane.
		menuSource = 0;
	}
	if (reinterpret_cast<HWND>(lParam) == wToolBar.GetID()) {
		// From toolbar -> goes to focused pane.
		menuSource = 0;
	}
	if (!menuSource) {
		if (!wEditor.HasFocus() && !wOutput.HasFocus()) {
			HWND wWithFocus = ::GetFocus();
			const GUI::gui_string classNameFocus = ClassNameOfWindow(wWithFocus);
			if (classNameFocus == GUI_TEXT("Edit")) {
				switch (cmdID) {
				case IDM_UNDO:
					::SendMessage(wWithFocus, EM_UNDO, 0, 0);
					return;
				case IDM_CUT:
					::SendMessage(wWithFocus, WM_CUT, 0, 0);
					return;
				case IDM_COPY:
					::SendMessage(wWithFocus, WM_COPY, 0, 0);
					return;
				}
			}
		}
	}

	switch (cmdID) {

	case IDM_ACTIVATE:
		Activate(lParam);
		break;

	case IDM_FINISHEDEXECUTE: {
			jobQueue.SetExecuting(false);
			if (needReadProperties)
				ReadProperties();
			CheckMenus();
			jobQueue.ClearJobs();
			CheckReload();
		}
		break;

	case IDM_ONTOP:
		topMost = (topMost ? false : true);
		::SetWindowPos(MainHWND(), (topMost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE + SWP_NOSIZE);
		CheckAMenuItem(IDM_ONTOP, topMost);
		break;

	case IDM_OPENFILESHERE:
		uniqueInstance.ToggleOpenFilesHere();
		break;

	case IDM_FULLSCREEN:
		FullScreenToggle();
		break;

	case IDC_TABCLOSE:
		CloseTab(static_cast<int>(lParam));
		break;

	case IDC_SHIFTTAB:
		ShiftTab(LOWORD(lParam), HIWORD(lParam));
		break;

	default:
		SciTEBase::MenuCommand(cmdID, menuSource);
	}
}

// from ScintillaWin.cxx
static UINT CodePageFromCharSet(SA::CharacterSet characterSet, UINT documentCodePage) noexcept {
	CHARSETINFO ci {};
	const BOOL bci = ::TranslateCharsetInfo(reinterpret_cast<DWORD *>(static_cast<uintptr_t>(characterSet)),
						&ci, TCI_SRCCHARSET);

	UINT cp = (bci) ? ci.ciACP : documentCodePage;

	CPINFO cpi {};
	if (!::IsValidCodePage(cp) && !::GetCPInfo(cp, &cpi))
		cp = CP_ACP;

	return cp;
}

void SciTEWin::OutputAppendEncodedStringSynchronised(const GUI::gui_string &s, int codePageDocument) {
	std::string sMulti = StringEncode(s, codePageDocument);
	OutputAppendStringSynchronised(sMulti.c_str());
}

CommandWorker::CommandWorker() noexcept : pSciTE(nullptr) {
	Initialise(true);
}

void CommandWorker::Initialise(bool resetToStart) noexcept {
	if (resetToStart)
		icmd = 0;
	originalEnd = 0;
	exitStatus = 0;
	flags = 0;
	seenOutput = false;
	outputScroll = 1;
}

void CommandWorker::Execute() {
	pSciTE->ProcessExecute();
}

void SciTEWin::ResetExecution() {
	cmdWorker.Initialise(true);
	jobQueue.SetExecuting(false);
	if (needReadProperties)
		ReadProperties();
	CheckReload();
	CheckMenus();
	jobQueue.ClearJobs();
	::SendMessage(MainHWND(), WM_COMMAND, IDM_FINISHEDEXECUTE, 0);
}

void SciTEWin::ExecuteNext() {
	cmdWorker.icmd++;
	if (cmdWorker.icmd < jobQueue.commandCurrent && cmdWorker.icmd < jobQueue.commandMax && cmdWorker.exitStatus == 0) {
		Execute();
	} else {
		ResetExecution();
	}
}

/**
 * Run a command with redirected input and output streams
 * so the output can be put in a window.
 * It is based upon several usenet posts and a knowledge base article.
 * This is running in a separate thread to the user interface so should always
 * use ScintillaWindow::Send rather than a one of the direct function calls.
 */
DWORD SciTEWin::ExecuteOne(const Job &jobToRun) {
	DWORD exitcode = 0;

	if (jobToRun.jobType == jobShell) {
		ShellExec(jobToRun.command, jobToRun.directory.AsUTF8().c_str());
		return exitcode;
	}

	if (jobToRun.jobType == jobHelp) {
		ExecuteHelp(jobToRun.command.c_str());
		return exitcode;
	}

	if (jobToRun.jobType == jobOtherHelp) {
		ExecuteOtherHelp(jobToRun.command.c_str());
		return exitcode;
	}

	if (jobToRun.jobType == jobGrep) {
		// jobToRun.command is "(w|~)(c|~)(d|~)(b|~)\0files\0text"
		const char *grepCmd = jobToRun.command.c_str();
		if (*grepCmd) {
			GrepFlags gf = grepNone;
			if (*grepCmd == 'w')
				gf = static_cast<GrepFlags>(gf | grepWholeWord);
			grepCmd++;
			if (*grepCmd == 'c')
				gf = static_cast<GrepFlags>(gf | grepMatchCase);
			grepCmd++;
			if (*grepCmd == 'd')
				gf = static_cast<GrepFlags>(gf | grepDot);
			grepCmd++;
			if (*grepCmd == 'b')
				gf = static_cast<GrepFlags>(gf | grepBinary);
			const char *findFiles = grepCmd + 2;
			const char *findText = findFiles + strlen(findFiles) + 1;
			if (cmdWorker.outputScroll == 1)
				gf = static_cast<GrepFlags>(gf | grepScroll);
			SA::Position positionEnd = wOutput.Send(SCI_GETCURRENTPOS);
			InternalGrep(gf, jobToRun.directory.AsInternal(), GUI::StringFromUTF8(findFiles).c_str(), findText, positionEnd);
			if ((gf & grepScroll) && returnOutputToCommand)
				wOutput.Send(SCI_GOTOPOS, positionEnd);
		}
		return exitcode;
	}

	UINT codePageOutput = static_cast<UINT>(wOutput.Send(SCI_GETCODEPAGE));
	if (codePageOutput != SA::CpUtf8) {
		codePageOutput = CodePageFromCharSet(characterSet, codePageOutput);
	}

	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
	OutputAppendStringSynchronised(">");
	OutputAppendEncodedStringSynchronised(GUI::StringFromUTF8(jobToRun.command), codePageOutput);
	OutputAppendStringSynchronised("\n");

	HANDLE hPipeWrite {};
	HANDLE hPipeRead {};
	// Create pipe for output redirection
	// read handle, write handle, security attributes,  number of bytes reserved for pipe
	constexpr DWORD pipeBufferSize = 64 * 1024;
	::CreatePipe(&hPipeRead, &hPipeWrite, &sa, pipeBufferSize);

	// Create pipe for input redirection. In this code, you do not
	// redirect the output of the child process, but you need a handle
	// to set the hStdInput field in the STARTUP_INFO struct. For safety,
	// you should not set the handles to an invalid handle.

	hWriteSubProcess = {};
	subProcessGroupId = 0;
	HANDLE hRead2 {};
	// read handle, write handle, security attributes,  number of bytes reserved for pipe
	::CreatePipe(&hRead2, &hWriteSubProcess, &sa, pipeBufferSize);

	::SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, 0);
	::SetHandleInformation(hWriteSubProcess, HANDLE_FLAG_INHERIT, 0);

	// Make child process use hPipeWrite as standard out, and make
	// sure it does not show on screen.
	STARTUPINFOW si = {};
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	if (jobToRun.jobType == jobCLI)
		si.wShowWindow = SW_HIDE;
	else
		si.wShowWindow = SW_SHOW;
	si.hStdInput = hRead2;
	si.hStdOutput = hPipeWrite;
	si.hStdError = hPipeWrite;

	FilePath startDirectory = jobToRun.directory.AbsolutePath();

	PROCESS_INFORMATION pi = {};

	// Make a mutable copy as the CreateProcess parameter is mutable
	const GUI::gui_string sCommand = GUI::StringFromUTF8(jobToRun.command);
	std::vector<wchar_t> vwcCommand(sCommand.c_str(), sCommand.c_str() + sCommand.length() + 1);

	BOOL running = ::CreateProcessW(
			       nullptr,
			       &vwcCommand[0],
			       nullptr, nullptr,
			       TRUE, CREATE_NEW_PROCESS_GROUP,
			       nullptr,
			       startDirectory.IsSet() ?
			       startDirectory.AsInternal() : nullptr,
			       &si, &pi);

	const DWORD errCode = ::GetLastError();
	// if jobCLI "System can't find" - try calling with command processor
	if ((!running) && (jobToRun.jobType == jobCLI) && (
				(errCode == ERROR_FILE_NOT_FOUND) || (errCode == ERROR_BAD_EXE_FORMAT))) {

		std::string runComLine = "cmd.exe /c ";
		runComLine = runComLine.append(jobToRun.command);

		const GUI::gui_string sRunComLine = GUI::StringFromUTF8(runComLine);
		std::vector<wchar_t> vwcRunComLine(sRunComLine.c_str(), sRunComLine.c_str() + sRunComLine.length() + 1);

		running = ::CreateProcessW(
				  nullptr,
				  &vwcRunComLine[0],
				  nullptr, nullptr,
				  TRUE, CREATE_NEW_PROCESS_GROUP,
				  nullptr,
				  startDirectory.IsSet() ?
				  startDirectory.AsInternal() : nullptr,
				  &si, &pi);
	}

	if (running) {
		subProcessGroupId = pi.dwProcessId;

		bool cancelled = false;

		std::string repSelBuf;

		size_t totalBytesToWrite = 0;
		if (jobToRun.flags & jobHasInput) {
			totalBytesToWrite = jobToRun.input.length();
		}

		if (totalBytesToWrite > 0 && !(jobToRun.flags & jobQuiet)) {
			std::string input = jobToRun.input;
			Substitute(input, "\n", "\n>> ");

			OutputAppendStringSynchronised(">> ");
			OutputAppendStringSynchronised(input.c_str());
			OutputAppendStringSynchronised("\n");
		}

		unsigned writingPosition = 0;

		int countPeeks = 0;
		bool processDead = false;
		while (running) {
			if (writingPosition >= totalBytesToWrite) {
				if (countPeeks > 10)
					::Sleep(100L);
				else if (countPeeks > 2)
					::Sleep(10L);
				countPeeks++;
			}

			// If we don't already know the process is dead,
			// check now (before polling the output pipe)
			if (!processDead && (WAIT_OBJECT_0 == ::WaitForSingleObject(pi.hProcess, 0))) {
				processDead = true;
			}

			DWORD bytesRead = 0;
			DWORD bytesAvail = 0;
			std::vector<char> buffer(pipeBufferSize);

			if (!::PeekNamedPipe(hPipeRead, &buffer[0],
					     static_cast<DWORD>(buffer.size()), &bytesRead, &bytesAvail, NULL)) {
				bytesAvail = 0;
			}

			if ((bytesAvail < 1000) && (hWriteSubProcess != INVALID_HANDLE_VALUE) && (writingPosition < totalBytesToWrite)) {
				// There is input to transmit to the process.  Do it in small blocks, interleaved
				// with reads, so that our hRead buffer will not be overrun with results.

				size_t bytesToWrite;
				const size_t eolPos = jobToRun.input.find("\n", writingPosition);
				if (eolPos == std::string::npos) {
					bytesToWrite = totalBytesToWrite - writingPosition;
				} else {
					bytesToWrite = eolPos + 1 - writingPosition;
				}
				if (bytesToWrite > 250) {
					bytesToWrite = 250;
				}

				DWORD bytesWrote = 0;

				const int bTest = ::WriteFile(hWriteSubProcess,
							      jobToRun.input.c_str() + writingPosition,
							      static_cast<DWORD>(bytesToWrite), &bytesWrote, NULL);

				if (bTest) {
					if ((writingPosition + bytesToWrite) / 1024 > writingPosition / 1024) {
						// sleep occasionally, even when writing
						::Sleep(100L);
					}

					writingPosition += bytesWrote;

					if (writingPosition >= totalBytesToWrite) {
						::CloseHandle(hWriteSubProcess);
						hWriteSubProcess = INVALID_HANDLE_VALUE;
					}

				} else {
					// Is this the right thing to do when writing to the pipe fails?
					::CloseHandle(hWriteSubProcess);
					hWriteSubProcess = INVALID_HANDLE_VALUE;
					OutputAppendStringSynchronised("\n>Input pipe closed due to write failure.\n");
				}

			} else if (bytesAvail > 0) {
				const int bTest = ::ReadFile(hPipeRead, &buffer[0],
							     static_cast<DWORD>(buffer.size()), &bytesRead, NULL);

				if (bTest && bytesRead) {

					if (jobToRun.flags & jobRepSelMask) {
						repSelBuf.append(&buffer[0], bytesRead);
					}

					if (!(jobToRun.flags & jobQuiet)) {
						if (!cmdWorker.seenOutput) {
							ShowOutputOnMainThread();
							cmdWorker.seenOutput = true;
						}
						// Display the data
						OutputAppendStringSynchronised(&buffer[0], bytesRead);
					}

					::UpdateWindow(MainHWND());
				} else {
					running = false;
				}
			} else {
				// bytesAvail == 0, and if the process
				// was already dead by the time we did
				// PeekNamedPipe, there should not be
				// any more data coming
				if (processDead) {
					running = false;
				}
			}

			if (jobQueue.SetCancelFlag(false)) {
				if (WAIT_OBJECT_0 != ::WaitForSingleObject(pi.hProcess, 500)) {
					// We should use it only if the GUI process is stuck and
					// don't answer to a normal termination command.
					// This function is dangerous: dependent DLLs don't know the process
					// is terminated, and memory isn't released.
					OutputAppendStringSynchronised("\n>Process failed to respond; forcing abrupt termination...\n");
					::TerminateProcess(pi.hProcess, 1);
				}
				running = false;
				cancelled = true;
			}
		}

		if (WAIT_OBJECT_0 != ::WaitForSingleObject(pi.hProcess, 1000)) {
			OutputAppendStringSynchronised("\n>Process failed to respond; forcing abrupt termination...");
			::TerminateProcess(pi.hProcess, 2);
		}
		::GetExitCodeProcess(pi.hProcess, &exitcode);
		std::ostringstream stExitMessage;
		stExitMessage << ">Exit code: " << exitcode;
		if (jobQueue.TimeCommands()) {
			stExitMessage << "    Time: ";
			stExitMessage << std::setprecision(4) << cmdWorker.commandTime.Duration();
		}
		stExitMessage << "\n";
		OutputAppendStringSynchronised(stExitMessage.str().c_str());

		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);

		if (!cancelled) {
			bool doRepSel = false;
			if (jobToRun.flags & jobRepSelYes)
				doRepSel = true;
			else if (jobToRun.flags & jobRepSelAuto)
				doRepSel = (0 == exitcode);

			if (doRepSel) {
				const SA::Position cpMin = wEditor.Send(SCI_GETSELECTIONSTART);
				wEditor.Send(SCI_REPLACESEL, 0, SptrFromString(repSelBuf.c_str()));
				wEditor.Send(SCI_SETSEL, cpMin, cpMin+repSelBuf.length());
			}
		}

		WarnUser(warnExecuteOK);

	} else {
		const DWORD nRet = ::GetLastError();
		OutputAppendStringSynchronised(">");
		OutputAppendEncodedStringSynchronised(GetErrorMessage(nRet), codePageOutput);
		WarnUser(warnExecuteKO);
	}
	::CloseHandle(hPipeRead);
	::CloseHandle(hPipeWrite);
	::CloseHandle(hRead2);
	::CloseHandle(hWriteSubProcess);
	hWriteSubProcess = {};
	subProcessGroupId = 0;
	return exitcode;
}

/**
 * Run a command in the job queue, stopping if one fails.
 * This is running in a separate thread to the user interface so must be
 * careful when reading and writing shared state.
 */
void SciTEWin::ProcessExecute() {
	if (scrollOutput)
		wOutput.Send(SCI_GOTOPOS, wOutput.Send(SCI_GETTEXTLENGTH));

	cmdWorker.exitStatus = ExecuteOne(jobQueue.jobQueue[cmdWorker.icmd]);
	if (jobQueue.isBuilding) {
		// The build command is first command in a sequence so it is only built if
		// that command succeeds not if a second returns after document is modified.
		jobQueue.isBuilding = false;
		if (cmdWorker.exitStatus == 0)
			jobQueue.isBuilt = true;
	}

	// Move selection back to beginning of this run so that F4 will go
	// to first error of this run.
	// scroll and return only if output.scroll equals
	// one in the properties file
	if ((cmdWorker.outputScroll == 1) && returnOutputToCommand)
		wOutput.Send(SCI_GOTOPOS, cmdWorker.originalEnd);
	returnOutputToCommand = true;
	PostOnMainThread(WORK_EXECUTE, &cmdWorker);
}

void SciTEWin::ShellExec(const std::string &cmd, const char *dir) {
	// guess if cmd is an executable, if this succeeds it can
	// contain spaces without enclosing it with "
	std::string cmdLower = cmd;
	LowerCaseAZ(cmdLower);
	const char *mycmdLowered = cmdLower.c_str();

	const char *s = strstr(mycmdLowered, ".exe");
	if (!s)
		s = strstr(mycmdLowered, ".cmd");
	if (!s)
		s = strstr(mycmdLowered, ".bat");
	if (!s)
		s = strstr(mycmdLowered, ".com");
	std::vector<char> cmdcopy(cmd.c_str(), cmd.c_str() + cmd.length() + 1);
	char *mycmdcopy = &cmdcopy[0];
	char *mycmd;
	char *mycmdEnd = nullptr;
	if (s && ((*(s + 4) == '\0') || (*(s + 4) == ' '))) {
		ptrdiff_t len_mycmd = s - mycmdLowered + 4;
		mycmd = mycmdcopy;
		mycmdEnd = mycmdcopy + len_mycmd;
	} else {
		if (*mycmdcopy != '"') {
			// get next space to separate cmd and parameters
			mycmd = mycmdcopy;
			mycmdEnd = strchr(mycmdcopy, ' ');
		} else {
			// the cmd is surrounded by ", so it can contain spaces, but we must
			// strip the " for ShellExec
			mycmd = mycmdcopy + 1;
			char *sm = strchr(mycmdcopy + 1, '"');
			if (sm) {
				*sm = '\0';
				mycmdEnd = sm + 1;
			}
		}
	}

	std::string myparams;
	if (mycmdEnd && (*mycmdEnd != '\0')) {
		*mycmdEnd = '\0';
		// test for remaining params after cmd, they may be surrounded by " but
		// we give them as-is to ShellExec
		++mycmdEnd;
		while (*mycmdEnd == ' ')
			++mycmdEnd;

		if (*mycmdEnd != '\0')
			myparams = mycmdEnd;
	}

	GUI::gui_string sMycmd = GUI::StringFromUTF8(mycmd);
	GUI::gui_string sMyparams = GUI::StringFromUTF8(myparams);
	GUI::gui_string sDir = GUI::StringFromUTF8(dir);

	SHELLEXECUTEINFO exec {};
	exec.cbSize = sizeof(exec);
	exec.fMask = SEE_MASK_FLAG_NO_UI; // own msg box on return
	exec.hwnd = MainHWND();
	exec.lpVerb = L"open";  // better for executables to use "open" instead of NULL
	exec.lpFile = sMycmd.c_str();   // file to open
	exec.lpParameters = sMyparams.c_str(); // parameters
	exec.lpDirectory = sDir.c_str(); // launch directory
	exec.nShow = SW_SHOWNORMAL; //default show cmd

	if (::ShellExecuteEx(&exec)) {
		// it worked!
		return;
	}
	const DWORD rc = GetLastError();

	std::string errormsg("Error while launching:\n\"");
	errormsg += mycmdcopy;
	if (myparams.length()) {
		errormsg += "\" with Params:\n\"";
		errormsg += myparams;
	}
	errormsg += "\"\n";
	GUI::gui_string sErrorMsg = GUI::StringFromUTF8(errormsg) + GetErrorMessage(rc);
	WindowMessageBox(wSciTE, sErrorMsg, mbsOK);
}

void SciTEWin::Execute() {
	if (buffers.SavingInBackground())
		// May be saving file that should be used by command so wait until all saved
		return;

	SciTEBase::Execute();
	if (!jobQueue.HasCommandToRun())
		// No commands to execute - possibly cancelled in SciTEBase::Execute
		return;

	cmdWorker.Initialise(false);
	cmdWorker.outputScroll = props.GetInt("output.scroll", 1);
	cmdWorker.originalEnd = wOutput.Length();
	cmdWorker.commandTime.Duration(true);
	cmdWorker.flags = jobQueue.jobQueue[cmdWorker.icmd].flags;
	if (scrollOutput)
		wOutput.GotoPos(wOutput.Length());

	if (jobQueue.jobQueue[cmdWorker.icmd].jobType == jobExtension) {
		// Execute extensions synchronously
		if (jobQueue.jobQueue[cmdWorker.icmd].flags & jobGroupUndo)
			wEditor.BeginUndoAction();

		if (extender)
			extender->OnExecute(jobQueue.jobQueue[cmdWorker.icmd].command.c_str());

		if (jobQueue.jobQueue[cmdWorker.icmd].flags & jobGroupUndo)
			wEditor.EndUndoAction();

		ExecuteNext();
	} else {
		// Execute other jobs asynchronously on a new thread
		PerformOnNewThread(&cmdWorker);
	}
}

void SciTEWin::StopExecute() {
	if (hWriteSubProcess && (hWriteSubProcess != INVALID_HANDLE_VALUE)) {
		const char stop[] = "\032";
		DWORD bytesWrote = 0;
		::WriteFile(hWriteSubProcess, stop, static_cast<DWORD>(strlen(stop)), &bytesWrote, nullptr);
		Sleep(500L);
	}

#ifdef USE_CONSOLE_EVENT
	if (subProcessGroupId) {
		// this also doesn't work
		OutputAppendStringSynchronised("\n>Attempting to cancel process...");

		if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, subProcessGroupId)) {
			LONG errCode = GetLastError();
			OutputAppendStringSynchronised("\n>BREAK Failed ");
			std::string sError = StdStringFromInteger(errCode);
			OutputAppendStringSynchronised(sError.c_str());
			OutputAppendStringSynchronised("\n");
		}
		Sleep(100L);
	}
#endif

	jobQueue.SetCancelFlag(true);
}

void SciTEWin::AddCommand(const std::string &cmd, const std::string &dir, JobSubsystem jobType, const std::string &input, int flags) {
	if (cmd.length()) {
		if ((jobType == jobShell) && ((flags & jobForceQueue) == 0)) {
			std::string pCmd = cmd;
			parameterisedCommand = "";
			if (pCmd[0] == '*') {
				pCmd.erase(0, 1);
				parameterisedCommand = pCmd;
				if (!ParametersDialog(true)) {
					return;
				}
			} else {
				ParamGrab();
			}
			pCmd = props.Expand(pCmd);
			ShellExec(pCmd, dir.c_str());
		} else {
			SciTEBase::AddCommand(cmd, dir, jobType, input, flags);
		}
	}
}

void SciTEWin::PostOnMainThread(int cmd, Worker *pWorker) {
	::PostMessage(HwndOf(wSciTE), SCITE_WORKER, cmd, reinterpret_cast<LPARAM>(pWorker));
}

void SciTEWin::WorkerCommand(int cmd, Worker *pWorker) {
	if (cmd < WORK_PLATFORM) {
		SciTEBase::WorkerCommand(cmd, pWorker);
	} else {
		if (cmd == WORK_EXECUTE) {
			// Move to next command
			ExecuteNext();
		}
	}
}

void SciTEWin::QuitProgram() {
	quitting = false;
	if (SaveIfUnsureAll() != saveCancelled) {
		if (fullScreen)	// Ensure tray visible on exit
			FullScreenToggle();
		quitting = true;
		// If ongoing saves, wait for them to complete.
		if (!buffers.SavingInBackground()) {
			::PostQuitMessage(0);
			wSciTE.Destroy();
		}
	}
}

void SciTEWin::RestorePosition() {
	const int left = propsSession.GetInt("position.left", CW_USEDEFAULT);
	const int top = propsSession.GetInt("position.top", CW_USEDEFAULT);
	const int width = propsSession.GetInt("position.width", CW_USEDEFAULT);
	const int height = propsSession.GetInt("position.height", CW_USEDEFAULT);
	cmdShow = propsSession.GetInt("position.maximize", 0) ? SW_MAXIMIZE : 0;

	constexpr int defaultValue = CW_USEDEFAULT;
	if (left != defaultValue &&
			top != defaultValue &&
			width != defaultValue &&
			height != defaultValue) {
		winPlace.length = sizeof(winPlace);
		winPlace.rcNormalPosition.left = left;
		winPlace.rcNormalPosition.right = left + width;
		winPlace.rcNormalPosition.top = top;
		winPlace.rcNormalPosition.bottom = top + height;
		::SetWindowPlacement(MainHWND(), &winPlace);
	}
}

void SciTEWin::CreateUI() {
	CreateBuffers();

	int left = props.GetInt("position.left", CW_USEDEFAULT);
	const int top = props.GetInt("position.top", CW_USEDEFAULT);
	int width = props.GetInt("position.width", CW_USEDEFAULT);
	int height = props.GetInt("position.height", CW_USEDEFAULT);
	cmdShow = props.GetInt("position.maximize", 0) ? SW_MAXIMIZE : 0;
	if (width == -1 || height == -1) {
		cmdShow = SW_MAXIMIZE;
		width = CW_USEDEFAULT;
		height = CW_USEDEFAULT;
	}

	if (props.GetInt("position.tile") && ::FindWindow(TEXT("SciTEWindow"), nullptr) &&
			(left != static_cast<int>(CW_USEDEFAULT))) {
		left += width;
	}
	// Pass 'this' pointer in lpParam of CreateWindow().
	wSciTE = ::CreateWindowEx(
			 0,
			 className,
			 windowName.c_str(),
			 WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
			 WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
			 WS_CLIPCHILDREN,
			 left, top, width, height,
			 NULL,
			 NULL,
			 hInstance,
			 this);
	if (!wSciTE.Created())
		exit(FALSE);

	if (props.GetInt("save.position"))
		RestorePosition();

	LocaliseMenus();
	std::string pageSetup = props.GetString("print.margins");
	char val[32] = "";
	const char *ps = pageSetup.c_str();
	const char *next = GetNextPropItem(ps, val, 32);
	pagesetupMargin.left = atol(val);
	next = GetNextPropItem(next, val, 32);
	pagesetupMargin.right = atol(val);
	next = GetNextPropItem(next, val, 32);
	pagesetupMargin.top = atol(val);
	GetNextPropItem(next, val, 32);
	pagesetupMargin.bottom = atol(val);

	UIAvailable();
}

static bool IsSpaceOrTab(GUI::gui_char ch) noexcept {
	return (ch == ' ') || (ch == '\t');
}

/**
 * Break up the command line into individual arguments and strip double quotes
 * from each argument.
 * @return A string with each argument separated by '\n'.
 */
GUI::gui_string SciTEWin::ProcessArgs(const GUI::gui_char *cmdLine) {
	GUI::gui_string args;
	const GUI::gui_char *startArg = cmdLine;
	while (*startArg) {
		while (IsSpaceOrTab(*startArg)) {
			startArg++;
		}
		const GUI::gui_char *endArg = startArg;
		if (*startArg == '"') {	// Opening double-quote
			startArg++;
			endArg = startArg;
			while (*endArg && *endArg != '\"') {
				endArg++;
			}
		} else {	// No double-quote, end of argument on first space
			while (*endArg && !IsSpaceOrTab(*endArg)) {
				endArg++;
			}
		}
		GUI::gui_string arg(startArg, 0, endArg - startArg);
		if (args.size() > 0)
			args += GUI_TEXT("\n");
		args += arg;
		startArg = endArg;	// On a space or a double-quote, or on the end of the command line
		if (*startArg == '"') {	// Closing double-quote
			startArg++;	// Consume the double-quote
		}
		while (IsSpaceOrTab(*startArg)) {
			// Consume spaces between arguments
			startArg++;
		}
	}

	return args;
}

/**
 * Process the command line, check for other instance wanting to open files,
 * create the SciTE window, perform batch processing (print) or transmit command line
 * to other instance and exit or just show the window and open files.
 */
void SciTEWin::Run(const GUI::gui_char *cmdLine) {
	// Load the default session file
	if (props.GetInt("save.session") || props.GetInt("save.position") || props.GetInt("save.recent")) {
		LoadSessionFile(GUI_TEXT(""));
	}

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

	// We create the window, so it can be found by EnumWindows below,
	// and the Scintilla control is thus created, allowing to print the file(s).
	// We don't show it yet, so if it is destroyed (duplicate instance), it will
	// not flash on the taskbar or on the display.
	CreateUI();

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

	// Open all files given on command line.
	// The filenames containing spaces must be enquoted.
	// In case of not using buffers they get closed immediately except
	// the last one, but they move to the MRU file list
	ProcessCommandLine(args, 1);
	Redraw();
}

/**
 * Draw the split bar.
 */
void ContentWin::Paint(HDC hDC, GUI::Rectangle) {
	const GUI::Rectangle rcInternal = GetClientPosition();

	const int heightClient = rcInternal.Height();
	const int widthClient = rcInternal.Width();

	const int heightEditor = heightClient - pSciTEWin->heightOutput - pSciTEWin->heightBar;
	const int yBorder = heightEditor;
	const int xBorder = widthClient - pSciTEWin->heightOutput - pSciTEWin->heightBar;
	for (int i = 0; i < pSciTEWin->heightBar; i++) {
		int colourIndex = COLOR_3DFACE;
		if (pSciTEWin->flatterUI) {
			if (i == 0 || i == pSciTEWin->heightBar - 1)
				colourIndex = COLOR_3DFACE;
			else
				colourIndex = COLOR_WINDOW;
		} else {
			if (i == 1)
				colourIndex = COLOR_3DHIGHLIGHT;
			else if (i == pSciTEWin->heightBar - 2)
				colourIndex = COLOR_3DSHADOW;
			else if (i == pSciTEWin->heightBar - 1)
				colourIndex = COLOR_3DDKSHADOW;
			else
				colourIndex = COLOR_3DFACE;
		}
		HPEN pen = ::CreatePen(0, 1, ::GetSysColor(colourIndex));
		HPEN penOld = SelectPen(hDC, pen);
		if (pSciTEWin->splitVertical) {
			::MoveToEx(hDC, xBorder + i, 0, nullptr);
			::LineTo(hDC, xBorder + i, heightClient);
		} else {
			::MoveToEx(hDC, 0, yBorder + i, nullptr);
			::LineTo(hDC, widthClient, yBorder + i);
		}
		SelectPen(hDC, penOld);
		DeletePen(pen);
	}
}

void SciTEWin::AboutDialog() {
#ifdef STATIC_BUILD
	AboutDialogWithBuild(1);
#else
	AboutDialogWithBuild(0);
#endif
}

/**
 * Open files dropped on the SciTE window.
 */
void SciTEWin::DropFiles(HDROP hdrop) {
	// If drag'n'drop inside the SciTE window but outside
	// Scintilla, hdrop is null, and an exception is generated!
	if (hdrop) {
		const bool tempFilesSyncLoad = props.GetInt("temp.files.sync.load") != 0;
		GUI::gui_char tempDir[MAX_PATH];
		const DWORD tempDirLen = ::GetTempPath(MAX_PATH, tempDir);
		bool isTempFile = false;
		const int filesDropped = ::DragQueryFile(hdrop, 0xffffffff, nullptr, 0);
		// Append paths to dropFilesQueue, to finish drag operation soon
		for (int i = 0; i < filesDropped; ++i) {
			GUI::gui_char pathDropped[MAX_PATH];
			::DragQueryFileW(hdrop, i, pathDropped,
					 static_cast<UINT>(std::size(pathDropped)));
			// Only do this for the first file in the drop op
			// as all are coming from the same drag location
			if (i == 0 && tempFilesSyncLoad) {
				// check if file's parent dir is temp
				if (::wcsncmp(tempDir, pathDropped, tempDirLen) == 0) {
					isTempFile = true;
				}
			}
			if (isTempFile) {
				if (!Open(pathDropped, ofSynchronous)) {
					break;
				}
			} else {
				dropFilesQueue.push_back(pathDropped);
			}
		}
		::DragFinish(hdrop);
		// Put SciTE to forefront
		// May not work for Win2k, but OK for lower versions
		// Note: how to drop a file to an iconic window?
		// Actually, it is the Send To command that generates a drop.
		if (::IsIconic(MainHWND())) {
			::ShowWindow(MainHWND(), SW_RESTORE);
		}
		::SetForegroundWindow(MainHWND());
		// Post message to ourself for opening the files so we can finish the drop message and
		// the drop source will respond when open operation takes long time (opening big files...)
		if (!dropFilesQueue.empty()) {
			::PostMessage(MainHWND(), SCITE_DROP, 0, 0);
		}
	}
}

/**
 * Handle simple wild-card file patterns and directory requests.
 */
bool SciTEWin::PreOpenCheck(const GUI::gui_char *arg) {
	bool isHandled = false;
	HANDLE hFFile {};
	WIN32_FIND_DATA ffile {};
	const DWORD fileattributes = ::GetFileAttributes(arg);
	int nbuffers = props.GetInt("buffers");
	FilePath fpArg(arg);

	if (fileattributes != INVALID_FILE_ATTRIBUTES) {	// arg is an existing directory or filename
		// if the command line argument is a directory, use OpenDialog()
		if (fileattributes & FILE_ATTRIBUTE_DIRECTORY) {
			OpenDialog(fpArg, GUI::StringFromUTF8(props.GetExpandedString("open.filter")).c_str());
			isHandled = true;
		}
	} else if (nbuffers > 1 && (hFFile = ::FindFirstFile(arg, &ffile)) != INVALID_HANDLE_VALUE) {
		// If several buffers is accepted and the arg is a filename pattern matching at least an existing file
		isHandled = true;
		FilePath fpDir = fpArg.Directory();

		do {
			if (!(ffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {	// Skip directories
				Open(FilePath(fpDir, ffile.cFileName));
				--nbuffers;
			}
		} while (nbuffers > 0 && ::FindNextFile(hFFile, &ffile));
		::FindClose(hFFile);
	} else {

		// if the filename is only an extension, open the dialog box with it as the extension filter
		if (!fpArg.BaseName().IsSet()) {
			isHandled = true;
			FilePath fpDir = fpArg.Directory();
			if (!fpDir.IsSet())
				fpDir = FilePath(GUI_TEXT("."));
			FilePath fpName = fpArg.Name();
			GUI::gui_string wildcard(GUI_TEXT("*"));
			wildcard += fpName.AsInternal();
			wildcard += GUI_TEXT("|*");
			wildcard += fpName.AsInternal();

			OpenDialog(fpDir, wildcard.c_str());
		} else if (!fpArg.Extension().IsSet()) {
			// if the filename has no extension, try to match a file with list of standard extensions
			std::string extensions = props.GetExpandedString("source.default.extensions");
			if (extensions.length()) {
				std::replace(extensions.begin(), extensions.end(), '|', '\0');
				size_t start = 0;
				while (start < extensions.length()) {
					GUI::gui_string filterName = GUI::StringFromUTF8(extensions.c_str() + start);
					GUI::gui_string nameWithExtension = fpArg.AsInternal();
					nameWithExtension += filterName;
					if (::GetFileAttributes(nameWithExtension.c_str()) != INVALID_FILE_ATTRIBUTES) {
						isHandled = true;
						Open(nameWithExtension);
						break;	// Found!
					} else {
						// Next extension
						start += strlen(extensions.c_str() + start) + 1;
					}
				}
			}
		}
	}

	return isHandled;
}

/* return true if stdin is blocked:
	- stdin is the console (isatty() == 1)
	- a valid handle for stdin cannot be generated
	- the handle appears to be the console - this may be a duplicate of using isatty() == 1
	- the pipe cannot be peeked, which appears to be from command lines such as "scite <file.txt"
	otherwise it is unblocked
*/
bool SciTEWin::IsStdinBlocked() {
	DWORD unreadMessages = 0;
	INPUT_RECORD irec[1] = {};
	char bytebuffer = '\0';
	HANDLE hStdIn = ::GetStdHandle(STD_INPUT_HANDLE);
	if (hStdIn == INVALID_HANDLE_VALUE) {
		/* an invalid handle, assume that stdin is blocked by falling to bottom */;
	} else if (::PeekConsoleInput(hStdIn, irec, 1, &unreadMessages) != 0) {
		/* it is the console, assume that stdin is blocked by falling to bottom */;
	} else if (::GetLastError() == ERROR_INVALID_HANDLE) {
		for (int n = 0; n < 4; n++) {
			/*	if this fails, it is either
				- a busy pipe "scite \*.,cxx /s /b | s -@",
				- another type of pipe "scite - <file", or
				- a blocked pipe "findstring nothing | scite -"
				in any case, retry in a short bit
			*/
			if (::PeekNamedPipe(hStdIn, &bytebuffer, sizeof(bytebuffer), nullptr, nullptr, &unreadMessages) != 0) {
				if (unreadMessages != 0) {
					return false; /* is a pipe and it is not blocked */
				}
			}
			::Sleep(2500);
		}
	}
	return true;
}

void SciTEWin::MinimizeToTray() {
	NOTIFYICONDATA nid {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = MainHWND();
	nid.uID = 1;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = SCITE_TRAY;
	nid.hIcon = static_cast<HICON>(
			    ::LoadImage(hInstance, TEXT("SCITE"), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE));
	StringCopy(nid.szTip, TEXT("SciTE"));
	::ShowWindow(MainHWND(), SW_MINIMIZE);
	if (::Shell_NotifyIcon(NIM_ADD, &nid)) {
		::ShowWindow(MainHWND(), SW_HIDE);
	}
}

void SciTEWin::RestoreFromTray() {
	NOTIFYICONDATA nid {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = MainHWND();
	nid.uID = 1;
	::ShowWindow(MainHWND(), SW_SHOW);
	::Sleep(100);
	::Shell_NotifyIcon(NIM_DELETE, &nid);
}

void SciTEWin::SettingChanged(WPARAM wParam, LPARAM lParam) {
	if (lParam) {
		const GUI::gui_string_view sv(reinterpret_cast<const wchar_t *>(lParam));
		if (sv == L"ImmersiveColorSet") {
			CheckAppearanceChanged();
		}
	}
	wEditor.Send(WM_SETTINGCHANGE, wParam, lParam);
	wOutput.Send(WM_SETTINGCHANGE, wParam, lParam);
}

void SciTEWin::SysColourChanged(WPARAM wParam, LPARAM lParam) {
	CheckAppearanceChanged();
	wEditor.Send(WM_SYSCOLORCHANGE, wParam, lParam);
	wOutput.Send(WM_SYSCOLORCHANGE, wParam, lParam);
}

void SciTEWin::ScaleChanged(WPARAM wParam, LPARAM lParam) {
	const int scale = LOWORD(wParam) * 100 / 96;
	if (SetScaleFactor(scale)) {
		wEditor.Send(WM_DPICHANGED, wParam, lParam);
		wOutput.Send(WM_DPICHANGED, wParam, lParam);
		ReloadProperties();
		const RECT *rect = reinterpret_cast<const RECT *>(lParam);
		::SetWindowPos(MainHWND(), NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top,
			SWP_NOZORDER | SWP_NOACTIVATE);

		if (!fnSystemParametersInfoForDpi) {
			fnSystemParametersInfoForDpi = DLLFunction<SystemParametersInfoForDpiSig>(
				L"user32.dll", "SystemParametersInfoForDpi");
		}

		if (fnSystemParametersInfoForDpi) {
			LOGFONTW lfIconTitle{};
			if (fnSystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lfIconTitle),
				&lfIconTitle, FALSE, LOWORD(wParam))) {
				const HFONT fontTabsPrevious = fontTabs;
				fontTabs = ::CreateFontIndirectW(&lfIconTitle);
				SetWindowFont(HwndOf(wTabBar), fontTabs, 0);
				::DeleteObject(fontTabsPrevious);
				SizeSubWindows();
			}
		}
	}
}

inline bool KeyMatch(const std::string &sKey, int keyval, int modifiers) {
	return SciTEKeys::MatchKeyCode(
		       SciTEKeys::ParseKeyCode(sKey.c_str()), keyval, modifiers);
}

LRESULT SciTEWin::KeyDown(WPARAM wParam) {
	// Look through lexer menu
	const SA::KeyMod modifiers =
		(IsKeyDown(VK_SHIFT) ? SA::KeyMod::Shift : SA::KeyMod::Norm) |
		(IsKeyDown(VK_CONTROL) ? SA::KeyMod::Ctrl : SA::KeyMod::Norm) |
		(IsKeyDown(VK_MENU) ? SA::KeyMod::Alt : SA::KeyMod::Norm);

	const int keyVal = static_cast<int>(wParam);
	const int modifierAsInt = static_cast<int>(modifiers);

	if (extender && extender->OnKey(keyVal, modifierAsInt))
		return 1l;

	for (unsigned int j = 0; j < languageMenu.size(); j++) {
		if (KeyMatch(languageMenu[j].menuKey, keyVal, modifierAsInt)) {
			SciTEBase::MenuCommand(IDM_LANGUAGE + j);
			return 1l;
		}
	}

	// loop through the Tools menu's active commands.
	HMENU hMenu = ::GetMenu(MainHWND());
	HMENU hToolsMenu = ::GetSubMenu(hMenu, menuTools);
	for (int tool = 0; tool < toolMax; ++tool) {
		MENUITEMINFO mii;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_DATA;
		if (::GetMenuItemInfo(hToolsMenu, IDM_TOOLS+tool, FALSE, &mii) && mii.dwItemData) {
			if (SciTEKeys::MatchKeyCode(static_cast<long>(mii.dwItemData), keyVal, modifierAsInt)) {
				SciTEBase::MenuCommand(IDM_TOOLS+tool);
				return 1l;
			}
		}
	}

	// loop through the keyboard short cuts defined by user.. if found
	// exec it the command defined
	for (const ShortcutItem &scut : shortCutItemList) {
		if (KeyMatch(scut.menuKey, keyVal, modifierAsInt)) {
			const int commandNum = SciTEBase::GetMenuCommandAsInt(scut.menuCommand.c_str());
			if (commandNum != -1) {
				// its possible that the command is for scintilla directly
				// all scintilla commands are larger then 2000
				if (commandNum < 2000) {
					SciTEBase::MenuCommand(commandNum);
				} else {
					PaneFocused().Call(static_cast<SA::Message>(commandNum));
				}
				return 1l;
			}
		}
	}

	return 0;
}

LRESULT SciTEWin::KeyUp(WPARAM wParam) {
	if (wParam == VK_CONTROL) {
		EndStackedTabbing();
	}
	return 0;
}

void SciTEWin::AddToPopUp(const char *label, int cmd, bool enabled) {
	GUI::gui_string localised = localiser.Text(label);
	HMENU menu = static_cast<HMENU>(popup.GetID());
	if (0 == localised.length())
		::AppendMenu(menu, MF_SEPARATOR, 0, TEXT(""));
	else if (enabled)
		::AppendMenu(menu, MF_STRING, cmd, localised.c_str());
	else
		::AppendMenu(menu, MF_STRING | MF_DISABLED | MF_GRAYED, cmd, localised.c_str());
}

LRESULT SciTEWin::ContextMenuMessage(UINT iMessage, WPARAM wParam, LPARAM lParam) {
	GUI::ScintillaWindow *w = &wEditor;
	GUI::Point pt = PointFromLong(lParam);
	if ((pt.x == -1) && (pt.y == -1)) {
		// Caused by keyboard so display menu near caret
		if (wOutput.HasFocus())
			w = &wOutput;
		const SA::Position position = w->CurrentPos();
		pt.x = w->PointXFromPosition(position);
		pt.y = w->PointYFromPosition(position);
		POINT spt = {pt.x, pt.y};
		::ClientToScreen(HwndOf(*w), &spt);
		pt = GUI::Point(spt.x, spt.y);
	} else {
		const GUI::Rectangle rcEditor = wEditor.GetPosition();
		if (!rcEditor.Contains(pt)) {
			const GUI::Rectangle rcOutput = wOutput.GetPosition();
			if (rcOutput.Contains(pt)) {
				w = &wOutput;
			} else {	// In frame so use default.
				return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
			}
		}
	}
	menuSource = ::GetDlgCtrlID(HwndOf(*w));
	ContextMenu(*w, pt, wSciTE);
	return 0;
}

void SciTEWin::CheckForScintillaFailure(SA::Status statusFailure) {
	static int boxesVisible = 0;
	if ((statusFailure > SA::Status::Ok) && (boxesVisible == 0)) {
		boxesVisible++;
		char buff[200] = "";
		if (statusFailure == SA::Status::BadAlloc) {
			strcpy(buff, "Memory exhausted.");
		} else {
			sprintf(buff, "Scintilla failed with status %d.", static_cast<int>(statusFailure));
		}
		strcat(buff, " SciTE will now close.");
		GUI::gui_string sMessage = GUI::StringFromUTF8(buff);
		::MessageBox(MainHWND(), sMessage.c_str(), TEXT("Failure in Scintilla"), MB_OK | MB_ICONERROR | MB_APPLMODAL);
		exit(FALSE);
	}
}

LRESULT SciTEWin::WndProc(UINT iMessage, WPARAM wParam, LPARAM lParam) {
	try {
		const LRESULT uim = uniqueInstance.CheckMessage(iMessage, wParam, lParam);
		if (uim != 0) {
			return uim;
		}

		switch (iMessage) {

		case WM_CREATE:
			Creation();
			break;

		case WM_COMMAND:
			Command(wParam, lParam);
			break;

		case WM_CONTEXTMENU:
			return ContextMenuMessage(iMessage, wParam, lParam);

		case WM_ENTERMENULOOP:
			if (!wParam)
				menuSource = 0;
			break;

		case WM_SYSCOMMAND:
			if ((wParam == SC_MINIMIZE) && props.GetInt("minimize.to.tray")) {
				MinimizeToTray();
				return 0;
			}
			return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);

		case SCITE_TRAY:
			if (lParam == WM_LBUTTONDOWN) {
				RestoreFromTray();
				::ShowWindow(MainHWND(), SW_RESTORE);
				::FlashWindow(MainHWND(), FALSE);
			}
			break;

		case SCITE_DROP:
			// Open the files
			while (!dropFilesQueue.empty()) {
				FilePath file(dropFilesQueue.front());
				dropFilesQueue.pop_front();
				if (file.Exists()) {
					Open(file);
				} else {
					GUI::gui_string msg = LocaliseMessage("Could not open file '^0'.", file.AsInternal());
					WindowMessageBox(wSciTE, msg);
				}
			}
			break;

		case SCITE_WORKER:
			WorkerCommand(static_cast<int>(wParam), reinterpret_cast<Worker *>(lParam));
			break;

		case SCITE_SHOWOUTPUT:
			SetOutputVisibility(true);
			break;

		case WM_NOTIFY:
			Notify(reinterpret_cast<SCNotification *>(lParam));
			break;

		case WM_KEYDOWN:
			return KeyDown(wParam);

		case WM_KEYUP:
			return KeyUp(wParam);

		case WM_APPCOMMAND:
			switch (GET_APPCOMMAND_LPARAM(lParam)) {
				case APPCOMMAND_BROWSER_BACKWARD:
					return KeyDown(VK_BROWSER_BACK);
				case APPCOMMAND_BROWSER_FORWARD:
					return KeyDown(VK_BROWSER_FORWARD);
				default:
					return ::DefWindowProcW(MainHWND(), iMessage, wParam, lParam);
			}
			return TRUE;

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
				SizeSubWindows();
			break;

		case WM_MOVE:
			wEditor.CallTipCancel();
			break;

		case WM_GETMINMAXINFO: {
				MINMAXINFO *pmmi = reinterpret_cast<MINMAXINFO *>(lParam);
				if (fullScreen) {
					pmmi->ptMaxSize.x = ::GetSystemMetrics(SM_CXSCREEN) +
							    2 * ::GetSystemMetrics(SM_CXSIZEFRAME);
					pmmi->ptMaxSize.y = ::GetSystemMetrics(SM_CYSCREEN) +
							    ::GetSystemMetrics(SM_CYCAPTION) +
							    ::GetSystemMetrics(SM_CYMENU) +
							    2 * ::GetSystemMetrics(SM_CYSIZEFRAME);
					pmmi->ptMaxTrackSize.x = pmmi->ptMaxSize.x;
					pmmi->ptMaxTrackSize.y = pmmi->ptMaxSize.y;
					return 0;
				} else {
					return ::DefWindowProc(MainHWND(), iMessage, wParam, lParam);
				}
			}

		case WM_INITMENU:
			CheckMenus();
			break;

		case WM_CLOSE:
			QuitProgram();
			return 0;

		case WM_QUERYENDSESSION:
			QuitProgram();
			return 1;

		case WM_DESTROY:
			break;

		case WM_SETTINGCHANGE:
			SettingChanged(wParam, lParam);
			break;

		case WM_SYSCOLORCHANGE:
			SysColourChanged(wParam, lParam);
			break;

		case WM_DPICHANGED:
			ScaleChanged(wParam, lParam);
			return ::DefWindowProcW(MainHWND(), iMessage, wParam, lParam);

		case WM_ACTIVATEAPP:
			if (props.GetInt("selection.always.visible", 0) == 0) {
				wEditor.HideSelection(!wParam);
			}
			// Do not want to display dialog yet as may be in middle of system mouse capture
			::PostMessage(MainHWND(), WM_COMMAND, IDM_ACTIVATE, wParam);
			break;

		case WM_ACTIVATE:
			if (wParam != WA_INACTIVE) {
				if (searchStrip.visible)
					searchStrip.Focus();
				else if (findStrip.visible)
					findStrip.Focus();
				else if (replaceStrip.visible)
					replaceStrip.Focus();
				else if (userStrip.visible)
					userStrip.Focus();
				else
					::SetFocus(wFocus);
			}
			break;

		case WM_TIMER:
			OnTimer();
			break;

		case WM_DROPFILES:
			DropFiles(reinterpret_cast<HDROP>(wParam));
			break;

		case WM_COPYDATA:
			return uniqueInstance.CopyData(reinterpret_cast<COPYDATASTRUCT *>(lParam));

		default:
			return ::DefWindowProcW(MainHWND(), iMessage, wParam, lParam);
		}
	} catch (const SA::Failure &sf) {
		CheckForScintillaFailure(sf.status);
	}
	return 0;
}

LRESULT PASCAL SciTEWin::TWndProc(
	HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	if (iMessage == WM_CREATE) {
		SciTEWin *scitePassed = static_cast<SciTEWin *>(SetWindowPointerFromCreate(hWnd, lParam));
		scitePassed->wSciTE = hWnd;
	}
	// Find C++ object associated with window.
	SciTEWin *scite = static_cast<SciTEWin *>(PointerFromWindow(hWnd));
	// scite will be zero if WM_CREATE not seen yet
	if (scite) {
		return scite->WndProc(iMessage, wParam, lParam);
	} else {
		return ::DefWindowProcW(hWnd, iMessage, wParam, lParam);
	}
}

LRESULT ContentWin::WndProc(UINT iMessage, WPARAM wParam, LPARAM lParam) {
	try {
		switch (iMessage) {

		case WM_CREATE:
			pSciTEWin->wContent = GetID();
			return ::DefWindowProc(Hwnd(), iMessage, wParam, lParam);

		case WM_COMMAND:
		case WM_NOTIFY:
			return pSciTEWin->WndProc(iMessage, wParam, lParam);

		case WM_PAINT: {
				PAINTSTRUCT ps;
				::BeginPaint(Hwnd(), &ps);
				const GUI::Rectangle rcPaint(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
				Paint(ps.hdc, rcPaint);
				::EndPaint(Hwnd(), &ps);
				return 0;
			}

		case WM_ERASEBKGND: {
				RECT rc = {0, 0, 2000, 2000};
				HBRUSH hbrFace = CreateSolidBrush(::GetSysColor(COLOR_3DFACE));
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, hbrFace);
				::DeleteObject(hbrFace);
				return 0;
			}

		case WM_LBUTTONDOWN:
			pSciTEWin->ptStartDrag = PointFromLong(lParam);
			capturedMouse = true;
			pSciTEWin->heightOutputStartDrag = pSciTEWin->heightOutput;
			::SetCapture(Hwnd());
			break;

		case WM_MOUSEMOVE:
			if (capturedMouse) {
				pSciTEWin->MoveSplit(PointFromLong(lParam));
			}
			break;

		case WM_LBUTTONUP:
			if (capturedMouse) {
				pSciTEWin->MoveSplit(PointFromLong(lParam));
				capturedMouse = false;
				::ReleaseCapture();
			}
			break;

		case WM_CAPTURECHANGED:
			capturedMouse = false;
			break;

		case WM_SETCURSOR:
			if (ControlIDOfCommand(static_cast<unsigned long>(lParam)) == HTCLIENT) {
				const GUI::Point ptCursor = PointOfCursor();
				const GUI::Rectangle rcScintilla = pSciTEWin->wEditor.GetPosition();
				const GUI::Rectangle rcOutput = pSciTEWin->wOutput.GetPosition();
				if (!rcScintilla.Contains(ptCursor) && !rcOutput.Contains(ptCursor)) {
					::SetCursor(::LoadCursor(NULL, pSciTEWin->splitVertical ? IDC_SIZEWE : IDC_SIZENS));
					return TRUE;
				}
			}
			return ::DefWindowProc(Hwnd(), iMessage, wParam, lParam);

		default:
			return ::DefWindowProc(Hwnd(), iMessage, wParam, lParam);

		}
	} catch (...) {
	}
	return 0;
}

// Convert String from UTF-8 to doc encoding
std::string SciTEWin::EncodeString(const std::string &s) {
	UINT codePageDocument = static_cast<UINT>(wEditor.CodePage());

	if (codePageDocument != SA::CpUtf8) {
		codePageDocument = CodePageFromCharSet(characterSet, codePageDocument);
		std::wstring sWide = StringDecode(std::string(s.c_str(), s.length()), CP_UTF8);
		return StringEncode(sWide, codePageDocument);
	}
	return SciTEBase::EncodeString(s);
}

// Convert String from doc encoding to UTF-8
std::string SciTEWin::GetRangeInUIEncoding(GUI::ScintillaWindow &win, SA::Range range) {
	std::string s = SciTEBase::GetRangeInUIEncoding(win, range);

	UINT codePageDocument = wEditor.CodePage();

	if (codePageDocument != SA::CpUtf8) {
		codePageDocument = CodePageFromCharSet(characterSet, codePageDocument);
		std::wstring sWide = StringDecode(std::string(s.c_str(), s.length()), codePageDocument);
		std::string sMulti = StringEncode(sWide, CP_UTF8);
		return std::string(sMulti.c_str(), 0, sMulti.length());
	}
	return s;
}

uintptr_t SciTEWin::EventLoop() {
	MSG msg;
	msg.wParam = 0;
	BOOL going = TRUE;
	while (going) {
		if (needIdle) {
			const BOOL haveMessage = PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
			if (!haveMessage) {
				OnIdle();
				continue;
			}
		}
		going = ::GetMessageW(&msg, NULL, 0, 0);
		if (going > 0) {
			if (!ModelessHandler(&msg)) {
				if (!GetID() ||
						::TranslateAcceleratorW(static_cast<HWND>(GetID()), GetAcceleratorTable(), &msg) == 0) {
					::TranslateMessage(&msg);
					::DispatchMessageW(&msg);
				}
			}
		}
	}
	return msg.wParam;
}

static void RestrictDLLPath() noexcept {
	// Try to limit the locations where DLLs will be loaded from to prevent binary planting.
	// That is where a bad DLL is placed in the current directory or in the PATH.
	typedef BOOL(WINAPI *SetDefaultDllDirectoriesSig)(DWORD DirectoryFlags);
	typedef BOOL(WINAPI *SetDllDirectorySig)(LPCTSTR lpPathName);
	HMODULE kernel32 = ::GetModuleHandle(TEXT("kernel32.dll"));
	if (kernel32) {
		// SetDefaultDllDirectories is stronger, limiting search path to just the application and
		// system directories but is only available on Windows 8+
		SetDefaultDllDirectoriesSig SetDefaultDllDirectoriesFn =
			reinterpret_cast<SetDefaultDllDirectoriesSig>(::GetProcAddress(
						kernel32, "SetDefaultDllDirectories"));
		if (SetDefaultDllDirectoriesFn) {
			SetDefaultDllDirectoriesFn(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
		} else {
			SetDllDirectorySig SetDllDirectoryFn =
				reinterpret_cast<SetDllDirectorySig>(::GetProcAddress(
							kernel32, "SetDllDirectoryW"));
			if (SetDllDirectoryFn) {
				// For security, remove current directory from the DLL search path
				SetDllDirectoryFn(TEXT(""));
			}
		}
	}
}

#ifdef STATIC_BUILD
extern "C" Scintilla::ILexer5 * __stdcall CreateLexer(const char *name);
#endif

#if defined(_MSC_VER) && defined(_PREFAST_)
// Stop warning for WinMain. Microsoft headers have annotations and MinGW don't.
#pragma warning(disable: 28251)
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

	RestrictDLLPath();

#ifndef NO_EXTENSIONS
	MultiplexExtension multiExtender;

#ifndef NO_LUA
	multiExtender.RegisterExtension(LuaExtension::Instance());
#endif

#ifndef NO_FILER
	multiExtender.RegisterExtension(DirectorExtension::Instance());
#endif
#endif

	SciTEWin::Register(hInstance);
	LexillaSetDefaultDirectory(GetSciTEPath(FilePath()).AsUTF8());
#ifdef STATIC_BUILD
	Scintilla_LinkLexers();
	Scintilla_RegisterClasses(hInstance);
	LexillaSetDefault([](const char *name) {
		return CreateLexer(name);
	});
#else

	HMODULE hmod = ::LoadLibrary(scintillaName);
	if (!hmod) {
		GUI::gui_string explanation = scintillaName;
		explanation += TEXT(" could not be loaded.  SciTE will now close");
		::MessageBox(NULL, explanation.c_str(),
			     TEXT("Error loading Scintilla"), MB_OK | MB_ICONERROR);
	}
#endif

	uintptr_t result = 0;
	{
#ifdef NO_EXTENSIONS
		Extension *extender = 0;
#else
		Extension *extender = &multiExtender;
#endif
		SciTEWin MainWind(extender);
		LPTSTR lptszCmdLine = GetCommandLine();
		if (*lptszCmdLine == '\"') {
			lptszCmdLine++;
			while (*lptszCmdLine && (*lptszCmdLine != '\"'))
				lptszCmdLine++;
			if (*lptszCmdLine == '\"')
				lptszCmdLine++;
		} else {
			while (*lptszCmdLine && (*lptszCmdLine != ' '))
				lptszCmdLine++;
		}
		while (*lptszCmdLine == ' ')
			lptszCmdLine++;
		try {
			MainWind.Run(lptszCmdLine);
			result = MainWind.EventLoop();
		} catch (const SA::Failure &sf) {
			MainWind.CheckForScintillaFailure(sf.status);
		}
		MainWind.Finalise();
	}

#ifdef STATIC_BUILD
	Scintilla_ReleaseResources();
#else

	::FreeLibrary(hmod);
#endif

	return static_cast<int>(result);
}
