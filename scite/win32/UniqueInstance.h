// SciTE - Scintilla based Text Editor
/** @file UniqueInstance.h
 ** Header of the class to ensure a unique instance of the editor.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef UNIQUEINSTANCE_H
#define UNIQUEINSTANCE_H

class SciTEWin;

/**
 */
class UniqueInstance {

private:
	SciTEWin *stw;	///< Access to SciTEWin interface
	UINT identityMessage;	///< Registered message to identify SciTE instances
	HANDLE mutex;	///< Mutex to avoid concurrent running
	std::wstring mutexName;	///< Name of the above
	bool bAlreadyRunning;	///< true if another instance accepting files is found
	HWND hOtherWindow;	///< Handle of the window of the found other instance

public:
	UniqueInstance();

	// Deleted so UniqueInstance objects can not be copied.
	UniqueInstance(const UniqueInstance &) = delete;
	UniqueInstance(UniqueInstance &&) = delete;
	UniqueInstance &operator=(const UniqueInstance &) = delete;
	UniqueInstance &operator=(UniqueInstance &&) = delete;

	~UniqueInstance();

	void Init(SciTEWin *stw_) noexcept;	///< To set the access to the SciTEWin interface
	bool AcceptToOpenFiles(bool bAccept);	///< Set mutex and see if succeed
	void CallSearchOnAllWindows();	///< Common code used by ToggleOpenFilesHere and FindOtherInstance
	void ToggleOpenFilesHere();	///< Change the option to open files in this instance and ask other instance to give up this option
	LRESULT CopyData(const COPYDATASTRUCT *pcds);	///< Send command line to existing instance
	LRESULT CheckMessage(UINT message, WPARAM wParam, LPARAM lParam);	///< Test if a given message is the identity message
	void CheckOtherInstance();	///< Create mutex name and try to set it
	bool FindOtherInstance();	///< Find another instance answering to the identity message
	void WindowCopyData(std::string_view s);	///< Send data to hOtherWindow
	void SendCommands(const char *cmdLine);	///< Send the command line to the other instance
	static BOOL CALLBACK SearchOtherInstance(HWND hWnd, LPARAM lParam);	///< Callback in the window enumeration
};

#endif
