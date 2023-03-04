// SciTE - Scintilla based Text Editor
/** @file DirectorExtension.cxx
 ** Extension for communicating with a director program.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstdio>
#include <ctime>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0A00
#include <windows.h>
#include <commctrl.h>

#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

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
#include "Cookie.h"
#include "Worker.h"
#include "MatchMarker.h"
#include "Searcher.h"
#include "SciTEBase.h"
#include "DirectorExtension.h"

static HWND wDirector {};
static HWND wCorrespondent {};
static HWND wReceiver {};
static bool startedByDirector = false;
static bool shuttingDown = false;
unsigned int SDI = 0;

static bool HasConnection() noexcept {
	return wDirector || wCorrespondent;
}

static void SendDirector(const char *verb, const char *arg = nullptr) {
	if (HasConnection()) {
		HWND wDestination = wCorrespondent;
		std::string addressedMessage;
		if (wDestination) {
			addressedMessage += ":";
			std::string address = StdStringFromSizeT(reinterpret_cast<size_t>(wDestination));
			addressedMessage += address;
			addressedMessage += ":";
		} else {
			wDestination = wDirector;
		}
		addressedMessage += verb;
		addressedMessage += ":";
		if (arg)
			addressedMessage += arg;
		std::string slashedMessage = Slash(addressedMessage, false);
		COPYDATASTRUCT cds;
		cds.dwData = 0;
		cds.cbData = static_cast<DWORD>(slashedMessage.length());
		slashedMessage.append(1, '\0');	// Ensure NUL at end of string
		cds.lpData = &slashedMessage[0];
		::SendMessage(wDestination, WM_COPYDATA,
			      reinterpret_cast<WPARAM>(wReceiver),
			      reinterpret_cast<LPARAM>(&cds));
	}
}

static void SendDirectorInteger(const char *verb, intptr_t arg) {
	std::string s = std::to_string(arg);
	::SendDirector(verb, s.c_str());
}

static HWND HwndFromString(const char *s) noexcept {
	return reinterpret_cast<HWND>(static_cast<uintptr_t>(atoll(s)));
}

static void CheckEnvironment(ExtensionAPI *phost) {
	if (phost && !shuttingDown) {
		if (!wDirector) {
			std::string director = phost->Property("director.hwnd");
			if (director.length() > 0) {
				startedByDirector = true;
				wDirector = HwndFromString(director.c_str());
				// Director is just seen so identify this to it
				::SendDirectorInteger("identity", reinterpret_cast<intptr_t>(wReceiver));
			}
		}
		std::string sReceiver = StdStringFromSizeT(reinterpret_cast<size_t>(wReceiver));
		phost->SetProperty("WindowID", sReceiver.c_str());
	}
}

static TCHAR DirectorExtension_ClassName[] = TEXT("DirectorExtension");

static LRESULT HandleCopyData(LPARAM lParam) {
	const COPYDATASTRUCT *pcds = reinterpret_cast<COPYDATASTRUCT *>(lParam);
	// Copy into an temporary buffer to ensure \0 terminated
	if (pcds->lpData) {
		std::string dataCopy(static_cast<const char *>(pcds->lpData), pcds->cbData);
		DirectorExtension::Instance().HandleStringMessage(dataCopy.c_str());
	}
	return 0;
}

LRESULT PASCAL DirectorExtension_WndProc(
	HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	if (iMessage == WM_COPYDATA) {
		return HandleCopyData(lParam);
	} else if (iMessage == SDI) {
		return SDI;
	}
	return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
}

static void DirectorExtension_Register(HINSTANCE hInstance) noexcept {
	WNDCLASS wndclass;
	wndclass.style = 0;
	wndclass.lpfnWndProc = DirectorExtension_WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = {};
	wndclass.hCursor = {};
	wndclass.hbrBackground = {};
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = DirectorExtension_ClassName;
	if (!::RegisterClass(&wndclass))
		::exit(FALSE);
}

DirectorExtension &DirectorExtension::Instance() {
	static DirectorExtension singleton;
	return singleton;
}

bool DirectorExtension::Initialise(ExtensionAPI *host_) {
	host = host_;
	SDI = ::RegisterWindowMessage(TEXT("SciTEDirectorInterface"));
	HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(
				      host->GetInstance());
	DirectorExtension_Register(hInstance);
	wReceiver = ::CreateWindow(
			    DirectorExtension_ClassName,
			    DirectorExtension_ClassName,
			    0,
			    0, 0, 0, 0,
			    0,
			    0,
			    hInstance,
			    nullptr);
	if (!wReceiver)
		::exit(FALSE);
	// Make the frame window handle available so the director can activate it.
	const SciTEBase *hostSciTE = dynamic_cast<SciTEBase *>(host);
	if (!hostSciTE) {
		::exit(FALSE);
	}
	::SetWindowLongPtr(wReceiver, GWLP_USERDATA,
			   reinterpret_cast<LONG_PTR>(hostSciTE->GetID()));
	CheckEnvironment(host);
	return true;
}

bool DirectorExtension::Finalise() noexcept {
	try {
		::SendDirector("closing");
	} catch (...) {
		// Shutting down so continue even if unable to send message to director.
	}
	if (wReceiver)
		::DestroyWindow(wReceiver);
	wReceiver = {};
	return true;
}

bool DirectorExtension::Clear() {
	return false;
}

bool DirectorExtension::Load(const char *) {
	return false;
}

bool DirectorExtension::OnOpen(const char *path) {
	CheckEnvironment(host);
	if (*path) {
		::SendDirector("opened", path);
	}
	return false;
}

bool DirectorExtension::OnSwitchFile(const char *path) {
	CheckEnvironment(host);
	if (*path) {
		::SendDirector("switched", path);
	}
	return false;
}

bool DirectorExtension::OnSave(const char *path) {
	CheckEnvironment(host);
	if (*path) {
		::SendDirector("saved", path);
	}
	return false;
}

bool DirectorExtension::OnClose(const char *path) {
	CheckEnvironment(host);
	if (*path) {
		::SendDirector("closed", path);
	}
	return false;
}

bool DirectorExtension::NeedsOnClose() {
	CheckEnvironment(host);
	return HasConnection();
}

bool DirectorExtension::OnChar(char) {
	return false;
}

bool DirectorExtension::OnExecute(const char *cmd) {
	CheckEnvironment(host);
	::SendDirector("macro:run", cmd);
	return false;
}

bool DirectorExtension::OnSavePointReached() {
	return false;
}

bool DirectorExtension::OnSavePointLeft() {
	return false;
}

bool DirectorExtension::OnStyle(SA::Position, SA::Position, int, StyleWriter *) {
	return false;
}

// These should probably have arguments

bool DirectorExtension::OnDoubleClick() {
	return false;
}

bool DirectorExtension::OnUpdateUI() {
	return false;
}

bool DirectorExtension::OnMarginClick() {
	return false;
}

bool DirectorExtension::OnMacro(const char *command, const char *params) {
	SendDirector(command, params);
	return false;
}

bool DirectorExtension::SendProperty(const char *prop) {
	CheckEnvironment(host);
	if (*prop) {
		::SendDirector("property", prop);
	}
	return false;
}

void DirectorExtension::HandleStringMessage(const char *message) {
	// Message may contain multiple commands separated by '\n'
	// Reentrance trouble - if this function is reentered, the wCorrespondent may
	// be set to zero before time.
	StringList wlMessage(true);
	wlMessage.Set(message);
	for (size_t i = 0; i < wlMessage.Length(); i++) {
		// Message format is [:return address:]command:argument
		char *cmd = wlMessage[i];
		if (*cmd == ':') {
			// There is a return address
			char *colon = strchr(cmd + 1, ':');
			if (colon) {
				*colon = '\0';
				wCorrespondent = HwndFromString(cmd + 1);
				cmd = colon + 1;
			}
		}
		if (isprefix(cmd, "identity:")) {
			const char *arg = strchr(cmd, ':');
			if (arg)
				wDirector = HwndFromString(arg + 1);
		} else if (isprefix(cmd, "closing:")) {
			wDirector = {};
			if (startedByDirector) {
				shuttingDown = true;
				if (host) {
					host->ShutDown();
				}
				shuttingDown = false;
			}
		} else if (host) {
			host->Perform(cmd);
		}
		wCorrespondent = {};
	}
}
