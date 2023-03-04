// SciTE - Scintilla based Text Editor
/** @file UniqueInstance.cxx
 ** Class to ensure a unique instance of the editor, if requested.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstring>

#include <string>

#include "SciTEWin.h"

UniqueInstance::UniqueInstance() {
	stw = nullptr;
	identityMessage = ::RegisterWindowMessage(TEXT("SciTEInstanceIdentifier"));
	mutex = {};
	bAlreadyRunning = false;
	hOtherWindow = {};
}

UniqueInstance::~UniqueInstance() {
	if (mutex) {
		::CloseHandle(mutex);
	}
}

void UniqueInstance::Init(SciTEWin *stw_) noexcept {
	stw = stw_;
}

/**
 * Try to create a mutex.
 * If succeed, it is the first/only instance.
 * Otherwise, there is already an instance holding this mutex.
 */
bool UniqueInstance::AcceptToOpenFiles(bool bAccept) {
	bool bError = false;

	stw->openFilesHere = bAccept;
	if (bAccept) {
		// We create a mutex because it is an atomic operation.
		// An operation like EnumWindows is long, so if we use it only, we can fall in a race condition.
		// Note from MSDN: "The system closes the handle automatically when the process terminates.
		// The mutex object is destroyed when its last handle has been closed."
		// If the mutex already exists, the new process get a handle on it, so even if the first
		// process exits, the mutex isn't destroyed, until all SciTE instances exit.
		mutex = ::CreateMutex(nullptr, FALSE, mutexName.c_str());
		// The call fails with ERROR_ACCESS_DENIED if the mutex was
		// created in a different user session because of passing
		// NULL for the SECURITY_ATTRIBUTES on mutex creation
		bError = (::GetLastError() == ERROR_ALREADY_EXISTS ||
			  ::GetLastError() == ERROR_ACCESS_DENIED);
	} else {
		::CloseHandle(mutex);
	}
	return !bError;
}

void UniqueInstance::CallSearchOnAllWindows() {
	::EnumWindows(SearchOtherInstance, reinterpret_cast<LPARAM>(this));
}

/**
 * Toggle the open files here option.
 * If set, search if another instance have this option set.
 * If found, we ask it to yield this option,
 * so we are the only one to accept files.
 */
void UniqueInstance::ToggleOpenFilesHere() {
	// If the openFilesHere option is set, we unset it and remove the handle.
	// Else, we set the option and try to set the mutex.
	if (!AcceptToOpenFiles(!stw->openFilesHere)) {
		// Cannot set the mutex, search the previous instance holding it
		CallSearchOnAllWindows();
		if (hOtherWindow) {
			// Found, we indicate it to yield the acceptation of files
			::SendMessage(hOtherWindow, identityMessage, 0,
				      static_cast<LPARAM>(1));
		}
	}
	stw->CheckMenus();
}

/**
 * Manage the received COPYDATA message with a command line from another instance.
 */
LRESULT UniqueInstance::CopyData(const COPYDATASTRUCT *pcds) {
	if (pcds) {
		if (stw->props.GetInt("minimize.to.tray")) {
			stw->RestoreFromTray();
		}
		const char *text = static_cast<const char *>(pcds->lpData);
		if (text && strlen(text) > 0) {
			GUI::gui_string args = stw->ProcessArgs(GUI::StringFromUTF8(text).c_str());
			stw->ProcessCommandLine(args, 0);
			stw->ProcessCommandLine(args, 1);
		}
		::FlashWindow(stw->MainHWND(), FALSE);
	}
	return TRUE;
}

/**
 * If the given message is the identity message and we hold the
 * open files here option, we answer the message (so other instances know
 * we are the one holding the option).
 * If the message ask to yield this option, we do it nicely...
 */
LRESULT UniqueInstance::CheckMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == identityMessage) {
		if (stw->openFilesHere || wParam != 0) {
			// We answer only if the menu item is checked to accept files,
			// or if the caller force answering by setting wParam to non null
			// which can be used to find all instances (not used yet).
			if (stw->openFilesHere && lParam != 0) {
				// An instance indicates it takes control of the Open Files Here
				// feature, so this one no longer accept them.
				AcceptToOpenFiles(false);
				stw->CheckMenus();	// Update the checkmark
			}
			return identityMessage;
		}
	}
	return 0;
}

/**
 * To be called only if check.if.already.open option is set to 1.
 * Create the mutex name, try to set the mutex.
 * If failed, renounce to the open files here option.
 */
void UniqueInstance::CheckOtherInstance() {
	// Use the method explained by Joseph M. Newcomer to avoid multiple instances of an application:
	// http://www.codeproject.com/cpp/avoidmultinstance.asp
	// I limit instances by desktop, it seems to make sense with a GUI application...
	mutexName = TEXT("SciTE-UniqueInstanceMutex-");	// I doubt I really need a GUID here...
	HDESK desktop = ::GetThreadDesktop(::GetCurrentThreadId());
	DWORD len = 0;
	// Query the needed size for the buffer
	const BOOL result = ::GetUserObjectInformation(desktop, UOI_NAME, nullptr, 0, &len);
	if (result == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		// WinNT / Win2000
		std::wstring info(len, 0);	// len is actually bytes so this is twice length needed
		::GetUserObjectInformation(desktop, UOI_NAME, &info[0], len, nullptr);
		mutexName += info;
	}
	// Try to set the mutex. If return false, it failed, there is already another instance.
	bAlreadyRunning = !AcceptToOpenFiles(true);
	if (bAlreadyRunning) {
		// Don't answer to requests from other starting instances
		stw->openFilesHere = false;
	}
}

/**
 * If we know there is another instance with open files here option,
 * we search it by enumerating windows.
 * @return true if found.
 */
bool UniqueInstance::FindOtherInstance() {
	if (bAlreadyRunning && identityMessage != 0) {
		CallSearchOnAllWindows();
		if (hOtherWindow) {
			return true;
		}
	}
	return false;
}

void UniqueInstance::WindowCopyData(std::string_view s) {
	std::string sCopy(s);	// Ensure NUL at end of string
	COPYDATASTRUCT cds {};
	cds.dwData = 0;
	cds.cbData = static_cast<DWORD>(sCopy.length() + 1);
	cds.lpData = sCopy.data();
	::SendMessage(hOtherWindow, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
}

/**
 * Send the COPYDATA messages to transmit the command line to
 * the instance holding the open files here option.
 * After that, the current instance will exit.
 */
void UniqueInstance::SendCommands(const char *cmdLine) {
	// On Win2k, windows can't get focus by themselves,
	// so it is the responsibility of the new process to bring the window
	// to foreground.
	// Put the other SciTE uniconized and to forefront.
	if (::IsIconic(hOtherWindow)) {
		::ShowWindow(hOtherWindow, SW_RESTORE);
	}
	::SetForegroundWindow(hOtherWindow);

	// Send 3 messages:
	// 1) first the cwd, so paths relative to the new instance can be
	// resolved in the old instance,
	// 2) then the real command line,
	// 3) then setdefaultcwd to set a reasonable default cwd and prevent
	// locking of directories.
	std::string cwdCmd("\"-cwd:");
	FilePath cwd = FilePath::GetWorkingDirectory();
	cwdCmd.append(cwd.AsUTF8());
	cwdCmd.append("\"");
	// Defeat the "\" mangling - convert "\" to "/"
	std::replace(cwdCmd.begin(), cwdCmd.end(), '\\', '/');
	WindowCopyData(cwdCmd);
	// Now the command line itself.
	WindowCopyData(cmdLine);
	// Restore default working directory
	WindowCopyData("-setdefaultcwd:");
}

/**
 * Function called by EnumWindows.
 * @a hWnd is the handle to the currently enumerated window.
 * @a lParam is seen as a pointer to the current UniqueInstance
 * so it can be used to access all members.
 * @return FALSE if found, to stop EnumWindows.
 */
BOOL CALLBACK UniqueInstance::SearchOtherInstance(HWND hWnd, LPARAM lParam) {
	BOOL bResult = TRUE;

	UniqueInstance *ui = reinterpret_cast<UniqueInstance *>(lParam);

	// First, avoid to send a message to ourself
	if (hWnd != ui->stw->MainHWND()) {
		// Send a message to the given window, to see if it will answer with
		// the same message. If it does, it is a Gui window with
		// openFilesHere set.
		// We use a timeout to avoid being blocked by hung processes.
		DWORD_PTR result = 0;
		const LRESULT found = ::SendMessageTimeout(hWnd,
				      ui->identityMessage, 0, 0,
				      SMTO_BLOCK | SMTO_ABORTIFHUNG, 200, &result);
		if (found != 0 && result == static_cast<DWORD_PTR>(ui->identityMessage)) {
			// Another Gui window found!
			// We memorise its window handle
			ui->hOtherWindow = hWnd;
			// We stop the EnumWindows
			bResult = FALSE;
		}
	}
	return bResult;
}
