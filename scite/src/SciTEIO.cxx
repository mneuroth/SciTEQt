// SciTE - Scintilla based Text Editor
/** @file SciTEIO.cxx
 ** Manage input and output with the system.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
// Older versions of GNU stdint.h require this definition to be able to see INT32_MAX
#define __STDC_LIMIT_MACROS
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <ctime>

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

#include <fcntl.h>

#include "ILoader.h"

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

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
#include "SciTEBase.h"
#include "Utf8_16.h"

#if defined(GTK)
const GUI::gui_char propUserFileName[] = GUI_TEXT(".SciTEUser.properties");
#elif defined(__APPLE__)
const GUI::gui_char propUserFileName[] = GUI_TEXT("SciTEUser.properties");
#else
// Windows
const GUI::gui_char propUserFileName[] = GUI_TEXT("SciTEUser.properties");
#endif
const GUI::gui_char propGlobalFileName[] = GUI_TEXT("SciTEGlobal.properties");
const GUI::gui_char propAbbrevFileName[] = GUI_TEXT("abbrev.properties");

void SciTEBase::SetFileName(const FilePath &openName, bool fixCase) {
	if (openName.AsInternal()[0] == '\"') {
		// openName is surrounded by double quotes
		GUI::gui_string pathCopy = openName.AsInternal();
		pathCopy = pathCopy.substr(1, pathCopy.size() - 2);
		filePath.Set(pathCopy);
	} else {
		filePath.Set(openName);
	}

	// Break fullPath into directory and file name using working directory for relative paths
    if (!filePath.IsAbsolute() && !filePath.IsNotLocal()) {
		// Relative path. Since we ran AbsolutePath, we probably are here because fullPath is empty.
		filePath.SetDirectory(filePath.Directory());
	}

	if (fixCase) {
		filePath.FixName();
	}

	ReadLocalPropFile();

	props.Set("FilePath", filePath.AsUTF8().c_str());
	props.Set("FileDir", filePath.Directory().AsUTF8().c_str());
	props.Set("FileName", filePath.BaseName().AsUTF8().c_str());
	props.Set("FileExt", filePath.Extension().AsUTF8().c_str());
	props.Set("FileNameExt", FileNameExt().AsUTF8().c_str());

	SetWindowName();
	if (buffers.buffers.size() > 0)
		CurrentBuffer()->file.Set(filePath);
}

// See if path exists.
// If path is not absolute, it is combined with dir.
// If resultPath is not NULL, it receives the absolute path if it exists.
bool SciTEBase::Exists(const GUI::gui_char *dir, const GUI::gui_char *path, FilePath *resultPath) {
	FilePath copy(path);
	if (!copy.IsAbsolute() && dir) {
		copy.SetDirectory(dir);
	}
	if (!copy.Exists())
		return false;
	if (resultPath) {
		resultPath->Set(copy.AbsolutePath());
	}
	return true;
}

void SciTEBase::CountLineEnds(int &linesCR, int &linesLF, int &linesCRLF) {
	linesCR = 0;
	linesLF = 0;
	linesCRLF = 0;
	const SA::Position lengthDoc = LengthDocument();
	char chPrev = ' ';
	TextReader acc(wEditor);
	char chNext = acc.SafeGetCharAt(0);
	for (SA::Position i = 0; i < lengthDoc; i++) {
		const char ch = chNext;
		chNext = acc.SafeGetCharAt(i + 1);
		if (ch == '\r') {
			if (chNext == '\n')
				linesCRLF++;
			else
				linesCR++;
		} else if (ch == '\n') {
			if (chPrev != '\r') {
				linesLF++;
			}
		} else if (i > 1000000) {
			return;
		}
		chPrev = ch;
	}
}

void SciTEBase::DiscoverEOLSetting() {
	SetEol();
	if (props.GetInt("eol.auto")) {
		int linesCR;
		int linesLF;
		int linesCRLF;
		CountLineEnds(linesCR, linesLF, linesCRLF);
		if (((linesLF >= linesCR) && (linesLF > linesCRLF)) || ((linesLF > linesCR) && (linesLF >= linesCRLF)))
			wEditor.SetEOLMode(SA::EndOfLine::Lf);
		else if (((linesCR >= linesLF) && (linesCR > linesCRLF)) || ((linesCR > linesLF) && (linesCR >= linesCRLF)))
			wEditor.SetEOLMode(SA::EndOfLine::Cr);
		else if (((linesCRLF >= linesLF) && (linesCRLF > linesCR)) || ((linesCRLF > linesLF) && (linesCRLF >= linesCR)))
			wEditor.SetEOLMode(SA::EndOfLine::CrLf);
	}
}

// Look inside the first line for a #! clue regarding the language
std::string SciTEBase::DiscoverLanguage() {
	const SA::Position length = std::min<SA::Position>(LengthDocument(), 64 * 1024);
	std::string buf = wEditor.StringOfRange(SA::Range(0, length));
	std::string languageOverride = "";
	std::string_view line = ExtractLine(buf);
	if (StartsWith(line, "<?xml")) {
		languageOverride = "xml";
	} else if (StartsWith(line, "#!")) {
		line.remove_prefix(2);
		std::string l1(line);
		std::replace(l1.begin(), l1.end(), '\\', ' ');
		std::replace(l1.begin(), l1.end(), '/', ' ');
		std::replace(l1.begin(), l1.end(), '\t', ' ');
		Substitute(l1, "  ", " ");
		Substitute(l1, "  ", " ");
		Substitute(l1, "  ", " ");
		::Remove(l1, std::string("\r"));
		::Remove(l1, std::string("\n"));
		if (StartsWith(l1, " ")) {
			l1 = l1.substr(1);
		}
		std::replace(l1.begin(), l1.end(), ' ', '\0');
		l1.append(1, '\0');
		const char *word = l1.c_str();
		while (*word) {
			std::string propShBang("shbang.");
			propShBang.append(word);
			std::string langShBang = props.GetExpandedString(propShBang.c_str());
			if (langShBang.length()) {
				languageOverride = langShBang;
			}
			word += strlen(word) + 1;
		}
	}
	if (languageOverride.length()) {
		languageOverride.insert(0, "x.");
	}
	return languageOverride;
}

void SciTEBase::DiscoverIndentSetting() {
	const SA::Position lengthDoc = std::min<SA::Position>(LengthDocument(), 1000000);
	TextReader acc(wEditor);
	bool newline = true;
	int indent = 0; // current line indentation
	int tabSizes[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // number of lines with corresponding indentation (index 0 - tab)
	int prevIndent = 0; // previous line indentation
	int prevTabSize = -1; // previous line tab size
	for (int i = 0; i < lengthDoc; i++) {
		const char ch = acc[i];
		if (ch == '\r' || ch == '\n') {
			indent = 0;
			newline = true;
		} else if (newline && ch == ' ') {
			indent++;
		} else if (newline) {
			if (indent) {
				if (indent == prevIndent && prevTabSize != -1) {
					tabSizes[prevTabSize]++;
				} else if (indent > prevIndent && prevIndent != -1) {
					if (indent - prevIndent <= 8) {
						prevTabSize = indent - prevIndent;
						tabSizes[prevTabSize]++;
					} else {
						prevTabSize = -1;
					}
				}
				prevIndent = indent;
			} else if (ch == '\t') {
				tabSizes[0]++;
				prevIndent = -1;
			} else {
				prevIndent = 0;
			}
			newline = false;
		}
	}
	// maximum non-zero indent
	int topTabSize = -1;
	for (int j = 0; j <= 8; j++) {
		if (tabSizes[j] && (topTabSize == -1 || tabSizes[j] > tabSizes[topTabSize])) {
			topTabSize = j;
		}
	}
	// set indentation
	if (topTabSize == 0) {
		wEditor.SetUseTabs(true);
		wEditor.SetIndent(wEditor.TabWidth());
	} else if (topTabSize != -1) {
		wEditor.SetUseTabs(false);
		wEditor.SetIndent(topTabSize);
	}
}

void SciTEBase::OpenCurrentFile(long long fileSize, bool suppressMessage, bool asynchronous) {
	if (CurrentBuffer()->pFileWorker) {
		// Already performing an asynchronous load or save so do not restart load
		if (!suppressMessage) {
			GUI::gui_string msg = LocaliseMessage("Could not open file '^0'.", filePath.AsInternal());
			WindowMessageBox(wSciTE, msg);
		}
		return;
	}

	FILE *fp = filePath.Open(fileRead);
	if (!fp) {
		if (!suppressMessage) {
			GUI::gui_string msg = LocaliseMessage("Could not open file '^0'.", filePath.AsInternal());
			WindowMessageBox(wSciTE, msg);
		}
		if (!wEditor.UndoCollection()) {
			wEditor.SetUndoCollection(true);
		}
		return;
	}

	CurrentBuffer()->SetTimeFromFile();

	wEditor.BeginUndoAction();	// Group together clear and insert
	wEditor.ClearAll();

	CurrentBuffer()->lifeState = Buffer::reading;
	if (asynchronous) {
		// Turn grey while loading
		wEditor.StyleSetBack(StyleDefault, 0xEEEEEE);
		wEditor.SetReadOnly(true);
		assert(CurrentBufferConst()->pFileWorker == nullptr);
		ILoader *pdocLoad;
		try {
			SA::DocumentOption docOptions = SA::DocumentOption::Default;

			const long long sizeLarge = props.GetLongLong("file.size.large");
			if (sizeLarge && (fileSize > sizeLarge))
				docOptions = SA::DocumentOption::TextLarge;

			const long long sizeNoStyles = props.GetLongLong("file.size.no.styles");
			if (sizeNoStyles && (fileSize > sizeNoStyles))
				docOptions = static_cast<SA::DocumentOption>(
						     static_cast<int>(docOptions) | static_cast<int>(SA::DocumentOption::StylesNone));

			pdocLoad = static_cast<ILoader *>(
					   wEditor.CreateLoader(static_cast<SA::Position>(fileSize) + 1000,
								docOptions));
		} catch (...) {
			wEditor.SetStatus(SA::Status::Ok);
			return;
		}
		CurrentBuffer()->pFileWorker = new FileLoader(this, pdocLoad, filePath, static_cast<size_t>(fileSize), fp);
		CurrentBuffer()->pFileWorker->sleepTime = props.GetInt("asynchronous.sleep");
		PerformOnNewThread(CurrentBuffer()->pFileWorker);
	} else {
		wEditor.Allocate(static_cast<SA::Position>(fileSize) + 1000);

		Utf8_16_Read convert;
		std::vector<char> data(blockSize);
		size_t lenFile = fread(&data[0], 1, data.size(), fp);
		const UniMode umCodingCookie = CodingCookieValue(std::string_view(data.data(), lenFile));
		while (lenFile > 0) {
			lenFile = convert.convert(&data[0], lenFile);
			const char *dataBlock = convert.getNewBuf();
			wEditor.AddText(lenFile, dataBlock);
			lenFile = fread(&data[0], 1, data.size(), fp);
			if (lenFile == 0) {
				// Handle case where convert is holding a lead surrogate but no more data
				const size_t lenFileTrail = convert.convert(nullptr, lenFile);
				if (lenFileTrail) {
					const char *dataTrail = convert.getNewBuf();
					wEditor.AddText(lenFileTrail, dataTrail);
				}
			}
		}
		fclose(fp);
		wEditor.EndUndoAction();

		CurrentBuffer()->unicodeMode = static_cast<UniMode>(
						       static_cast<int>(convert.getEncoding()));
		// Check the first two lines for coding cookies
		if (CurrentBuffer()->unicodeMode == uni8Bit) {
			CurrentBuffer()->unicodeMode = umCodingCookie;
		}

		CompleteOpen(ocSynchronous);
	}
}

void SciTEBase::TextRead(FileWorker *pFileWorker) {
	FileLoader *pFileLoader = static_cast<FileLoader *>(pFileWorker);
	const int iBuffer = buffers.GetDocumentByWorker(pFileLoader);
	// May not be found if load cancelled
	if (iBuffer >= 0) {
		buffers.buffers[iBuffer].unicodeMode = pFileLoader->unicodeMode;
		buffers.buffers[iBuffer].lifeState = Buffer::readAll;
		if (pFileLoader->err) {
			GUI::gui_string msg = LocaliseMessage("Could not open file '^0'.", pFileLoader->path.AsInternal());
			WindowMessageBox(wSciTE, msg);
			// Should refuse to save when failure occurs
			buffers.buffers[iBuffer].lifeState = Buffer::empty;
		}
		// Switch documents
		void *pdocLoading = pFileLoader->pLoader->ConvertToDocument();
		pFileLoader->pLoader = nullptr;
		SwitchDocumentAt(iBuffer, pdocLoading);
		if (iBuffer == buffers.Current()) {
			CompleteOpen(ocCompleteCurrent);
			if (extender)
				extender->OnOpen(buffers.buffers[iBuffer].file.AsUTF8().c_str());
			RestoreState(buffers.buffers[iBuffer], true);
			DisplayAround(buffers.buffers[iBuffer].file);
			wEditor.ScrollCaret();
		}
	}
}

void SciTEBase::PerformDeferredTasks() {
	if (buffers.buffers[buffers.Current()].futureDo & Buffer::fdFinishSave) {
		wEditor.SetSavePoint();
		wEditor.SetReadOnly(CurrentBuffer()->isReadOnly);
		buffers.FinishedFuture(buffers.Current(), Buffer::fdFinishSave);
	}
}

void SciTEBase::CompleteOpen(OpenCompletion oc) {
	wEditor.SetReadOnly(CurrentBuffer()->isReadOnly);

	if (oc != ocSynchronous) {
		ReadProperties();
	}

	if (language == "") {
		std::string languageOverride = DiscoverLanguage();
		if (languageOverride.length()) {
			CurrentBuffer()->overrideExtension = languageOverride;
			CurrentBuffer()->lifeState = Buffer::opened;
			ReadProperties();
			SetIndentSettings();
		}
	}

	if (oc != ocSynchronous) {
		SetIndentSettings();
		SetEol();
		UpdateBuffersCurrent();
		SizeSubWindows();
	}

	if (CurrentBuffer()->unicodeMode != uni8Bit) {
		// Override the code page if Unicode
		codePage = SA::CpUtf8;
	} else {
		codePage = props.GetInt("code.page");
	}
	wEditor.SetCodePage(codePage);

	DiscoverEOLSetting();

	if (props.GetInt("indent.auto")) {
		DiscoverIndentSetting();
	}

	if (!wEditor.UndoCollection()) {
		wEditor.SetUndoCollection(true);
	}
	wEditor.SetSavePoint();
	if (props.GetInt("fold.on.open") > 0) {
		FoldAll();
	}
	wEditor.GotoPos(0);

	CurrentBuffer()->CompleteLoading();

	Redraw();
}

void SciTEBase::TextWritten(FileWorker *pFileWorker) {
	const FileStorer *pFileStorer = static_cast<const FileStorer *>(pFileWorker);
	const int iBuffer = buffers.GetDocumentByWorker(pFileStorer);

	FilePath pathSaved = pFileStorer->path;
	const int errSaved = pFileStorer->err;
	const bool cancelledSaved = pFileStorer->Cancelling();

	// May not be found if save cancelled or buffer closed
	if (iBuffer >= 0) {
		// Complete and release
		buffers.buffers[iBuffer].CompleteStoring();
		if (errSaved || cancelledSaved) {
			// Background save failed (possibly out-of-space) so resurrect the
			// buffer so can be saved to another disk or retried after making room.
			buffers.SetVisible(iBuffer, true);
			SetBuffersMenu();
			if (iBuffer == buffers.Current()) {
				wEditor.SetReadOnly(CurrentBuffer()->isReadOnly);
			}
		} else {
			if (!buffers.GetVisible(iBuffer)) {
				buffers.RemoveInvisible(iBuffer);
			}
			if (iBuffer == buffers.Current()) {
				wEditor.SetReadOnly(CurrentBuffer()->isReadOnly);
				if (pathSaved.SameNameAs(CurrentBuffer()->file)) {
					wEditor.SetSavePoint();
				}
				if (extender)
					extender->OnSave(buffers.buffers[iBuffer].file.AsUTF8().c_str());
			} else {
				buffers.buffers[iBuffer].isDirty = false;
				buffers.buffers[iBuffer].failedSave = false;
				// Need to make writable and set save point when next receive focus.
				buffers.AddFuture(iBuffer, Buffer::fdFinishSave);
				SetBuffersMenu();
			}
		}
	} else {
		GUI::gui_string msg = LocaliseMessage("Could not find buffer '^0'.", pathSaved.AsInternal());
		WindowMessageBox(wSciTE, msg);
	}

	if (errSaved) {
		FailedSaveMessageBox(pathSaved);
	}

	if (IsPropertiesFile(pathSaved)) {
		ReloadProperties();
	}
	UpdateStatusBar(true);
	if (!jobQueue.executing && (jobQueue.HasCommandToRun())) {
		Execute();
	}
	if (quitting && !buffers.SavingInBackground()) {
		QuitProgram();
	}
}

void SciTEBase::UpdateProgress(Worker *) {
	GUI::gui_string prog;
	BackgroundActivities bgActivities = buffers.CountBackgroundActivities();
	const int countBoth = bgActivities.loaders + bgActivities.storers;
	if (countBoth == 0) {
		// Should hide UI
		ShowBackgroundProgress(GUI_TEXT(""), 0, 0);
	} else {
		if (countBoth == 1) {
			prog += LocaliseMessage(bgActivities.loaders ? "Opening '^0'" : "Saving '^0'",
						bgActivities.fileNameLast.c_str());
		} else {
			if (bgActivities.loaders) {
				prog += LocaliseMessage("Opening ^0 files ", GUI::StringFromInteger(bgActivities.loaders).c_str());
			}
			if (bgActivities.storers) {
				prog += LocaliseMessage("Saving ^0 files ", GUI::StringFromInteger(bgActivities.storers).c_str());
			}
		}
		ShowBackgroundProgress(prog, bgActivities.totalWork, bgActivities.totalProgress);
	}
}

bool SciTEBase::PreOpenCheck(const GUI::gui_char *) {
	return false;
}

bool SciTEBase::Open(const FilePath &file, OpenFlags of) {
	InitialiseBuffers();

	FilePath absPath = file.AbsolutePath();
	if (!absPath.IsUntitled() && absPath.IsDirectory()) {
		GUI::gui_string msg = LocaliseMessage("Path '^0' is a directory so can not be opened.",
						      absPath.AsInternal());
		WindowMessageBox(wSciTE, msg);
		return false;
	}

	const int index = buffers.GetDocumentByName(absPath);
	if (index >= 0) {
		buffers.SetVisible(index, true);
		SetDocumentAt(index);
		RemoveFileFromStack(absPath);
		DeleteFileStackMenu();
		SetFileStackMenu();
		// If not forcing reload or currently busy with load or save, just rotate into view
		if ((!(of & ofForceLoad)) || (CurrentBufferConst()->pFileWorker))
			return true;
	}
	// See if we can have a buffer for the file to open
	if (!CanMakeRoom(!(of & ofNoSaveIfDirty))) {
		return false;
	}

	const long long fileSize = absPath.IsUntitled() ? 0 : absPath.GetFileLength();
	if (fileSize > INTPTR_MAX) {
		const GUI::gui_string sSize = GUI::StringFromLongLong(fileSize);
		const GUI::gui_string msg = LocaliseMessage("File '^0' is ^1 bytes long, "
					    "larger than 2GB which is the largest SciTE can open.",
					    absPath.AsInternal(), sSize.c_str());
		WindowMessageBox(wSciTE, msg, mbsIconWarning);
		return false;
	}
	if (fileSize > 0) {
		// Real file, not empty buffer
		const long long maxSize = props.GetLongLong("max.file.size", 2000000000LL);
		if (maxSize > 0 && fileSize > maxSize) {
			const GUI::gui_string sSize = GUI::StringFromLongLong(fileSize);
			const GUI::gui_string sMaxSize = GUI::StringFromLongLong(maxSize);
			const GUI::gui_string msg = LocaliseMessage("File '^0' is ^1 bytes long,\n"
						    "larger than the ^2 bytes limit set in the properties.\n"
						    "Do you still want to open it?",
						    absPath.AsInternal(), sSize.c_str(), sMaxSize.c_str());
			const MessageBoxChoice answer = WindowMessageBox(wSciTE, msg, mbsYesNo | mbsIconWarning);
			if (answer != mbYes) {
				return false;
			}
		}
	}

	if (buffers.size() == buffers.length) {
		AddFileToStack(RecentFile(filePath, GetSelectedRange(), GetCurrentScrollPosition()));
		ClearDocument();
		CurrentBuffer()->lifeState = Buffer::opened;
		if (extender)
			extender->InitBuffer(buffers.Current());
	} else {
		if (index < 0 || !(of & ofForceLoad)) { // No new buffer, already opened
			New();
		}
	}

	assert(CurrentBufferConst()->pFileWorker == nullptr);
	SetFileName(absPath);

	propsDiscovered.Clear();
	std::string discoveryScript = props.GetExpandedString("command.discover.properties");
	if (discoveryScript.length()) {
		std::string propertiesText = CommandExecute(GUI::StringFromUTF8(discoveryScript).c_str(),
					     absPath.Directory().AsInternal());
		if (propertiesText.size()) {
			propsDiscovered.ReadFromMemory(propertiesText.c_str(), propertiesText.size(), absPath.Directory(), filter, nullptr, 0);
		}
	}
	CurrentBuffer()->props = propsDiscovered;
	CurrentBuffer()->overrideExtension = "";
	ReadProperties();
	SetIndentSettings();
	SetEol();
	UpdateBuffersCurrent();
	SizeSubWindows();
	SetBuffersMenu();

	bool asynchronous = false;
	if (!filePath.IsUntitled()) {
		wEditor.SetReadOnly(false);
		wEditor.Cancel();
		if (of & ofPreserveUndo) {
			wEditor.BeginUndoAction();
		} else {
			wEditor.SetUndoCollection(false);
		}

		asynchronous = (fileSize > props.GetInt("background.open.size", -1)) &&
			       !(of & (ofPreserveUndo|ofSynchronous));
		OpenCurrentFile(fileSize, of & ofQuiet, asynchronous);

		if (of & ofPreserveUndo) {
			wEditor.EndUndoAction();
		} else {
			wEditor.EmptyUndoBuffer();
		}
		CurrentBuffer()->isReadOnly = props.GetInt("read.only");
		wEditor.SetReadOnly(CurrentBuffer()->isReadOnly);
	}
	RemoveFileFromStack(filePath);
	DeleteFileStackMenu();
	SetFileStackMenu();
	SetWindowName();
	if (lineNumbers && lineNumbersExpand)
		SetLineNumberWidth();
	UpdateStatusBar(true);
	if (extender && !asynchronous)
		extender->OnOpen(filePath.AsUTF8().c_str());
	return true;
}

// Returns true if editor should get the focus
bool SciTEBase::OpenSelected() {
	std::string selName = SelectionFilename();
	if (selName.length() == 0) {
		WarnUser(warnWrongFile);
		return false;	// No selection
	}

#if !defined(GTK)
	if (StartsWith(selName, "http:") ||
			StartsWith(selName, "https:") ||
			StartsWith(selName, "ftp:") ||
			StartsWith(selName, "ftps:") ||
			StartsWith(selName, "news:") ||
			StartsWith(selName, "mailto:")) {
		std::string cmd = selName;
		AddCommand(cmd, "", jobShell);
		return false;	// Job is done
	}
#endif

	if (StartsWith(selName, "file://")) {
		selName.erase(0, 7);
		if (selName[0] == '/' && selName[2] == ':') { // file:///C:/filename.ext
			selName.erase(0, 1);
		}
	}

	std::string fileNameForExtension = ExtensionFileName();
	std::string openSuffix = props.GetNewExpandString("open.suffix.", fileNameForExtension.c_str());
	selName += openSuffix;

	if (EqualCaseInsensitive(selName.c_str(), FileNameExt().AsUTF8().c_str()) || EqualCaseInsensitive(selName.c_str(), filePath.AsUTF8().c_str())) {
		WarnUser(warnWrongFile);
		return true;	// Do not open if it is the current file!
	}

	std::string cTag;
	SA::Line lineNumber = 0;
	if (IsPropertiesFile(filePath) &&
			(selName.find('.') == std::string::npos)) {
		// We are in a properties file and try to open a file without extension,
		// we suppose we want to open an imported .properties file
		// So we append the correct extension to open the included file.
		// Maybe we should check if the filename is preceded by "import"...
		selName += extensionProperties;
	} else {
		// Check if we have a line number (error message or grep result)
		// A bit of duplicate work with DecodeMessage, but we don't know
		// here the format of the line, so we do guess work.
		// Can't do much for space separated line numbers anyway...
		size_t endPath = selName.find('(');
		if (endPath != std::string::npos) {	// Visual Studio error message: F:\scite\src\SciTEBase.h(312):	bool Exists(
			lineNumber = atol(selName.c_str() + endPath + 1);
		} else {
			endPath = selName.find(':', 2);	// Skip Windows' drive separator
			if (endPath != std::string::npos) {	// grep -n line, perhaps gcc too: F:\scite\src\SciTEBase.h:312:	bool Exists(
				lineNumber = atol(selName.c_str() + endPath + 1);
			}
		}
		if (lineNumber > 0) {
			selName.erase(endPath);
		}

		// Support the ctags format

		if (lineNumber == 0) {
			cTag = GetCTag();
		}
	}

	FilePath path;
	// Don't load the path of the current file if the selected
	// filename is an absolute pathname
	GUI::gui_string selFN = GUI::StringFromUTF8(selName);
	if (!FilePath(selFN).IsAbsolute()) {
		path = filePath.Directory();
		// If not there, look in openpath
		if (!Exists(path.AsInternal(), selFN.c_str(), nullptr)) {
			GUI::gui_string openPath = GUI::StringFromUTF8(props.GetNewExpandString(
							   "openpath.", fileNameForExtension.c_str()));
			while (openPath.length()) {
				GUI::gui_string tryPath(openPath);
				const size_t sepIndex = tryPath.find(listSepString);
				if ((sepIndex != GUI::gui_string::npos) && (sepIndex != 0)) {
					tryPath.erase(sepIndex);
					openPath.erase(0, sepIndex + 1);
				} else {
					openPath.erase();
				}
				if (Exists(tryPath.c_str(), selFN.c_str(), nullptr)) {
					path.Set(tryPath.c_str());
					break;
				}
			}
		}
	}
	FilePath pathReturned;
	if (Exists(path.AsInternal(), selFN.c_str(), &pathReturned)) {
		// Open synchronously if want to seek line number or search tag
		const OpenFlags of = ((lineNumber > 0) || (cTag.length() != 0)) ? ofSynchronous : ofNone;
		if (Open(pathReturned, of)) {
			if (lineNumber > 0) {
				wEditor.GotoLine(lineNumber - 1);
			} else if (cTag.length() != 0) {
				const SA::Line cTagLine = IntegerFromText(cTag.c_str());
				if (cTagLine > 0) {
					wEditor.GotoLine(cTagLine - 1);
				} else {
					findWhat = cTag;
					FindNext(false);
				}
			}
			return true;
		}
	} else {
		WarnUser(warnWrongFile);
	}
	return false;
}

void SciTEBase::Revert() {
	if (filePath.IsUntitled()) {
		wEditor.ClearAll();
	} else {
		RecentFile rf = GetFilePosition();
		OpenCurrentFile(filePath.GetFileLength(), false, false);
		DisplayAround(rf);
	}
}

void SciTEBase::CheckReload() {
	if (props.GetInt("load.on.activate")) {
		// Make a copy of fullPath as otherwise it gets aliased in Open
		const time_t newModTime = filePath.ModifiedTime();
		if ((newModTime != 0) && (newModTime != CurrentBuffer()->fileModTime)) {
			RecentFile rf = GetFilePosition();
			const OpenFlags of = props.GetInt("reload.preserves.undo") ? ofPreserveUndo : ofNone;
			if (CurrentBuffer()->isDirty || props.GetInt("are.you.sure.on.reload") != 0) {
				if ((0 == dialogsOnScreen) && (newModTime != CurrentBuffer()->fileModLastAsk)) {
					GUI::gui_string msg;
					if (CurrentBuffer()->isDirty) {
						msg = LocaliseMessage(
							      "The file '^0' has been modified. Should it be reloaded?",
							      filePath.AsInternal());
					} else {
						msg = LocaliseMessage(
							      "The file '^0' has been modified outside SciTE. Should it be reloaded?",
							      FileNameExt().AsInternal());
					}
					const MessageBoxChoice decision = WindowMessageBox(wSciTE, msg, mbsYesNo | mbsIconQuestion);
					if (decision == mbYes) {
						Open(filePath, static_cast<OpenFlags>(of | ofForceLoad));
						DisplayAround(rf);
					}
					CurrentBuffer()->fileModLastAsk = newModTime;
				}
			} else {
				Open(filePath, static_cast<OpenFlags>(of | ofForceLoad));
				DisplayAround(rf);
			}
		}  else if (newModTime == 0 && CurrentBuffer()->fileModTime != 0)  {
			// Check if the file is deleted
			CurrentBuffer()->fileModTime = 0;
			CurrentBuffer()->fileModLastAsk = 0;
			CurrentBuffer()->isDirty = true;
			CheckMenus();
			SetWindowName();
			SetBuffersMenu();
			GUI::gui_string msg = LocaliseMessage(
						      "The file '^0' has been deleted.",
						      filePath.AsInternal());
			WindowMessageBox(wSciTE, msg, mbsOK);
		}
	}
}

void SciTEBase::Activate(bool activeApp) {
	if (activeApp) {
		CheckReload();
	} else {
		if (props.GetInt("save.on.deactivate")) {
			SaveTitledBuffers();
		}
	}
}

FilePath SciTEBase::SaveName(const char *ext) const {
	GUI::gui_string savePath = filePath.AsInternal();
	if (ext) {
		int dot = static_cast<int>(savePath.length() - 1);
		while ((dot >= 0) && (savePath[dot] != '.')) {
			dot--;
		}
		if (dot >= 0) {
			const int keepExt = props.GetInt("export.keep.ext");
			if (keepExt == 0) {
				savePath.erase(dot);
			} else if (keepExt == 2) {
				savePath[dot] = '_';
			}
		}
		savePath += GUI::StringFromUTF8(ext);
	}
	//~ fprintf(stderr, "SaveName <%s> <%s> <%s>\n", filePath.AsInternal(), savePath.c_str(), ext);
	return FilePath(savePath.c_str());
}

SciTEBase::SaveResult SciTEBase::SaveIfUnsure(bool forceQuestion, SaveFlags sf) {
	CurrentBuffer()->failedSave = false;
	if (CurrentBuffer()->pFileWorker) {
		if (CurrentBuffer()->pFileWorker->IsLoading())
			// In semi-loaded state so refuse to save
			return saveCancelled;
		else
			return saveCompleted;
	}
	if ((CurrentBuffer()->isDirty) && (LengthDocument() || !filePath.IsUntitled() || forceQuestion)) {
		if (props.GetInt("are.you.sure", 1) ||
				filePath.IsUntitled() ||
				forceQuestion) {
			GUI::gui_string msg;
			if (!filePath.IsUntitled()) {
				msg = LocaliseMessage("Save changes to '^0'?", filePath.AsInternal());
			} else {
				msg = LocaliseMessage("Save changes to (Untitled)?");
			}
			const MessageBoxChoice decision = WindowMessageBox(wSciTE, msg, mbsYesNoCancel | mbsIconQuestion);
			if (decision == mbYes) {
				if (!Save(sf))
					return saveCancelled;
			}
			return (decision == mbCancel) ? saveCancelled : saveCompleted;
		} else {
			if (!Save(sf))
				return saveCancelled;
		}
	}
	return saveCompleted;
}

SciTEBase::SaveResult SciTEBase::SaveIfUnsureAll() {
	if (SaveAllBuffers(false) == saveCancelled) {
		return saveCancelled;
	}
	if (props.GetInt("save.recent")) {
		for (int i = 0; i < buffers.lengthVisible; ++i) {
			Buffer buff = buffers.buffers[i];
			AddFileToStack(buff.file);
		}
	}
	if (props.GetInt("save.session") || props.GetInt("save.position") || props.GetInt("save.recent")) {
		SaveSessionFile(GUI_TEXT(""));
	}

	if (extender && extender->NeedsOnClose()) {
		// Ensure extender is told about each buffer closing
		for (int k = 0; k < buffers.lengthVisible; k++) {
			SetDocumentAt(k);
			extender->OnClose(filePath.AsUTF8().c_str());
		}
	}

	// Definitely going to exit now, so delete all documents
	// Set editor back to initial document
	if (buffers.lengthVisible > 0) {
		wEditor.SetDocPointer(buffers.buffers[0].doc);
	}
	// Release all the extra documents
	for (int j = 0; j < buffers.size(); j++) {
		if (buffers.buffers[j].doc && !buffers.buffers[j].pFileWorker) {
			wEditor.ReleaseDocument(buffers.buffers[j].doc);
			buffers.buffers[j].doc = nullptr;
		}
	}
	// Initial document will be deleted when editor deleted
	return saveCompleted;
}

SciTEBase::SaveResult SciTEBase::SaveIfUnsureForBuilt() {
	if (props.GetInt("save.all.for.build")) {
		return SaveAllBuffers(!props.GetInt("are.you.sure.for.build"));
	}
	if (CurrentBuffer()->isDirty) {
		if (props.GetInt("are.you.sure.for.build"))
			return SaveIfUnsure(true);

		Save();
	}
	return saveCompleted;
}
/**
	Selection saver and restorer.

	If virtual space is disabled, the class does nothing.

	If virtual space is enabled, constructor saves all selections using (line, column) coordinates,
	destructor restores all the saved selections.
**/
class SelectionKeeper {
public:
	explicit SelectionKeeper(GUI::ScintillaWindow &editor) : wEditor(editor) {
		const SA::VirtualSpace mask = static_cast<SA::VirtualSpace>(
						      static_cast<int>(SA::VirtualSpace::RectangularSelection) |
						      static_cast<int>(SA::VirtualSpace::UserAccessible));
		if (static_cast<int>(wEditor.VirtualSpaceOptions()) & static_cast<int>(mask)) {
			const int n = wEditor.Selections();
			for (int i = 0; i < n; ++i) {
				selections.push_back(LocFromPos(GetSelection(i)));
			}
		}
	}

	~SelectionKeeper() {
		int i = 0;
		for (auto const &sel : selections) {
			SetSelection(i, PosFromLoc(sel));
			++i;
		}
	}

private:
	struct Position {
		Position(SA::Position pos_, SA::Position virt_) noexcept : pos(pos_), virt(virt_) {};
		SA::Position pos;
		SA::Position virt;
	};

	struct Location {
		Location(SA::Line line_, SA::Position col_) noexcept : line(line_), col(col_) {};
		SA::Line line;
		SA::Position col;
	};

	Position GetAnchor(int i) {
		const SA::Position pos  = wEditor.SelectionNAnchor(i);
		const SA::Position virt = wEditor.SelectionNAnchorVirtualSpace(i);
		return Position(pos, virt);
	}

	Position GetCaret(int i) {
		const SA::Position pos  = wEditor.SelectionNCaret(i);
		const SA::Position virt = wEditor.SelectionNCaretVirtualSpace(i);
		return Position(pos, virt);
	}

	std::pair<Position, Position> GetSelection(int i) {
		return {GetAnchor(i), GetCaret(i)};
	};

	Location LocFromPos(Position const &pos) {
		const SA::Line line = wEditor.LineFromPosition(pos.pos);
		const SA::Position col  = wEditor.Column(pos.pos) + pos.virt;
		return Location(line, col);
	}

	std::pair<Location, Location> LocFromPos(std::pair<Position, Position> const &pos) {
		return {LocFromPos(pos.first), LocFromPos(pos.second)};
	}

	Position PosFromLoc(Location const &loc) {
		const SA::Position pos = wEditor.FindColumn(loc.line, loc.col);
		const SA::Position col = wEditor.Column(pos);
		return Position(pos, loc.col - col);
	}

	std::pair<Position, Position> PosFromLoc(std::pair<Location, Location> const &loc) {
		return {PosFromLoc(loc.first), PosFromLoc(loc.second)};
	}

	void SetAnchor(int i, Position const &pos) {
		wEditor.SetSelectionNAnchor(i, pos.pos);
		wEditor.SetSelectionNAnchorVirtualSpace(i, pos.virt);
	};

	void SetCaret(int i, Position const &pos) {
		wEditor.SetSelectionNCaret(i, pos.pos);
		wEditor.SetSelectionNCaretVirtualSpace(i, pos.virt);
	}

	void SetSelection(int i, std::pair<Position, Position> const &pos) {
		SetAnchor(i, pos.first);
		SetCaret(i, pos.second);
	}

	GUI::ScintillaWindow &wEditor;
	std::vector<std::pair<Location, Location>> selections;
};

void SciTEBase::StripTrailingSpaces() {
	const SA::Line maxLines = wEditor.LineCount();
	SelectionKeeper keeper(wEditor);
	for (int line = 0; line < maxLines; line++) {
		const SA::Position lineStart = wEditor.LineStart(line);
		const SA::Position lineEnd = wEditor.LineEnd(line);
		SA::Position i = lineEnd - 1;
		char ch = wEditor.CharacterAt(i);
		while ((i >= lineStart) && ((ch == ' ') || (ch == '\t'))) {
			i--;
			ch = wEditor.CharacterAt(i);
		}
		if (i < (lineEnd - 1)) {
			wEditor.SetTarget(SA::Range(i + 1, lineEnd));
			wEditor.ReplaceTarget("");
		}
	}
}

void SciTEBase::EnsureFinalNewLine() {
	const SA::Line maxLines = wEditor.LineCount();
	bool appendNewLine = maxLines == 1;
	const SA::Position endDocument = wEditor.LineStart(maxLines);
	if (maxLines > 1) {
		appendNewLine = endDocument > wEditor.LineStart(maxLines - 1);
	}
	if (appendNewLine) {
		const char *eol = "\n";
		switch (wEditor.EOLMode()) {
		case SA::EndOfLine::CrLf:
			eol = "\r\n";
			break;
		case SA::EndOfLine::Cr:
			eol = "\r";
			break;
		case SA::EndOfLine::Lf:
			break;
		}
		wEditor.InsertText(endDocument, eol);
	}
}

// Perform any changes needed before saving such as normalizing spaces and line ends.
bool SciTEBase::PrepareBufferForSave(const FilePath &saveName) {
	bool retVal = false;
	// Perform clean ups on text before saving
	wEditor.BeginUndoAction();
	if (stripTrailingSpaces)
		StripTrailingSpaces();
	if (ensureFinalLineEnd)
		EnsureFinalNewLine();
	if (ensureConsistentLineEnds)
		wEditor.ConvertEOLs(wEditor.EOLMode());

	if (extender)
		retVal = extender->OnBeforeSave(saveName.AsUTF8().c_str());

	wEditor.EndUndoAction();

	return retVal;
}

/**
 * Writes the buffer to the given filename.
 */
bool SciTEBase::SaveBuffer(const FilePath &saveName, SaveFlags sf) {
	bool retVal = PrepareBufferForSave(saveName);

	if (!retVal) {

		FILE *fp = saveName.Open(fileWrite);
		if (fp) {
			const size_t lengthDoc = LengthDocument();
			if (!(sf & sfSynchronous)) {
				wEditor.SetReadOnly(true);
				const char *documentBytes = static_cast<const char *>(wEditor.CharacterPointer());
				CurrentBuffer()->pFileWorker = new FileStorer(this, documentBytes, saveName, lengthDoc, fp, CurrentBuffer()->unicodeMode, (sf & sfProgressVisible));
				CurrentBuffer()->pFileWorker->sleepTime = props.GetInt("asynchronous.sleep");
				if (PerformOnNewThread(CurrentBuffer()->pFileWorker)) {
					retVal = true;
				} else {
					GUI::gui_string msg = LocaliseMessage("Failed to save file '^0' as thread could not be started.", saveName.AsInternal());
					WindowMessageBox(wSciTE, msg);
				}
			} else {
				Utf8_16_Write convert;
				if (CurrentBuffer()->unicodeMode != uniCookie) {	// Save file with cookie without BOM.
					convert.setEncoding(static_cast<Utf8_16::encodingType>(
								    static_cast<int>(CurrentBuffer()->unicodeMode)));
				}
				convert.setfile(fp);
				std::vector<char> data(blockSize + 1);
				retVal = true;
				size_t grabSize;
				for (size_t i = 0; i < lengthDoc; i += grabSize) {
					grabSize = lengthDoc - i;
					if (grabSize > blockSize)
						grabSize = blockSize;
					// Round down so only whole characters retrieved.
					grabSize = wEditor.PositionBefore(i + grabSize + 1) - i;
					const SA::Range rangeGrab(static_cast<SA::Position>(i),
								  static_cast<SA::Position>(i + grabSize));
					wEditor.SetTarget(rangeGrab);
					wEditor.TargetText(&data[0]);
					const size_t written = convert.fwrite(&data[0], grabSize);
					if (written == 0) {
						retVal = false;
						break;
					}
				}
				if (convert.fclose() != 0) {
					retVal = false;
				}
			}
		}
	}

	if (retVal && extender && (sf & sfSynchronous)) {
		extender->OnSave(saveName.AsUTF8().c_str());
	}
	UpdateStatusBar(true);
	return retVal;
}

void SciTEBase::ReloadProperties() {
	ReadGlobalPropFile();
	SetImportMenu();
	ReadLocalPropFile();
	ReadAbbrevPropFile();
	ReadProperties();
	SetWindowName();
	BuffersMenu();
	Redraw();
}

// Returns false if cancelled or failed to save
bool SciTEBase::Save(SaveFlags sf) {
	if (!filePath.IsUntitled()) {
		GUI::gui_string msg;
		if (CurrentBuffer()->ShouldNotSave()) {
			msg = LocaliseMessage(
				      "The file '^0' has not yet been loaded entirely, so it can not be saved right now. Please retry in a while.",
				      filePath.AsInternal());
			WindowMessageBox(wSciTE, msg);
			// It is OK to not save this file
			return true;
		}

		if (CurrentBuffer()->pFileWorker) {
			msg = LocaliseMessage(
				      "The file '^0' is already being saved.",
				      filePath.AsInternal());
			WindowMessageBox(wSciTE, msg);
			// It is OK to not save this file
			return true;
		}

		if (props.GetInt("save.deletes.first")) {
			filePath.Remove();
		} else if (props.GetInt("save.check.modified.time")) {
			const time_t newModTime = filePath.ModifiedTime();
			if ((newModTime != 0) && (CurrentBuffer()->fileModTime != 0) &&
					(newModTime != CurrentBuffer()->fileModTime)) {
				msg = LocaliseMessage("The file '^0' has been modified outside SciTE. Should it be saved?",
						      filePath.AsInternal());
				const MessageBoxChoice decision = WindowMessageBox(wSciTE, msg, mbsYesNo | mbsIconQuestion);
				if (decision == mbNo) {
					return false;
				}
			}
		}

		if ((LengthDocument() <= props.GetInt("background.save.size", -1)) ||
				(buffers.SingleBuffer()))
			sf = static_cast<SaveFlags>(sf | sfSynchronous);
		if (SaveBuffer(filePath, sf)) {
			CurrentBuffer()->SetTimeFromFile();
			if (sf & sfSynchronous) {
				wEditor.SetSavePoint();
				if (IsPropertiesFile(filePath)) {
					ReloadProperties();
				}
			}
		} else {
			if (!CurrentBuffer()->failedSave) {
				CurrentBuffer()->failedSave = true;
				msg = LocaliseMessage(
					      "Could not save file '^0'. Save under a different name?", filePath.AsInternal());
				const MessageBoxChoice decision = WindowMessageBox(wSciTE, msg, mbsYesNo | mbsIconWarning);
				if (decision == mbYes) {
					return SaveAsDialog();
				}
			}
			return false;
		}
		return true;
	} else {
		if (props.GetString("save.path.suggestion").length()) {
			const time_t t = time(nullptr);
			char timeBuff[15];
			strftime(timeBuff, sizeof(timeBuff), "%Y%m%d%H%M%S",  localtime(&t));
			PropSetFile propsSuggestion;
			propsSuggestion.superPS = &props;  // Allow access to other settings
			propsSuggestion.Set("TimeStamp", timeBuff);
			propsSuggestion.Set("SciteUserHome", GetSciteUserHome().AsUTF8().c_str());
			std::string savePathSuggestion = propsSuggestion.GetExpandedString("save.path.suggestion");
			std::replace(savePathSuggestion.begin(), savePathSuggestion.end(), '\\', '/');  // To accept "\" on Unix
			if (savePathSuggestion.size() > 0) {
				filePath = FilePath(GUI::StringFromUTF8(savePathSuggestion)).NormalizePath();
			}
		}
		const bool ret = SaveAsDialog();
		if (!ret)
			filePath.Set(GUI_TEXT(""));
		return ret;
	}
}

void SciTEBase::SaveAs(const GUI::gui_char *file, bool fixCase) {
	SetFileName(file, fixCase);
	Save();
	ReadProperties();
	wEditor.ClearDocumentStyle();
	wEditor.Colourise(0, wEditor.LineStart(1));
	Redraw();
	SetWindowName();
	BuffersMenu();
	if (extender)
		extender->OnSave(filePath.AsUTF8().c_str());
}

bool SciTEBase::SaveIfNotOpen(const FilePath &destFile, bool fixCase) {
	FilePath absPath = destFile.AbsolutePath();
	const int index = buffers.GetDocumentByName(absPath, true /* excludeCurrent */);
	if (index >= 0) {
		GUI::gui_string msg = LocaliseMessage(
					      "File '^0' is already open in another buffer.", destFile.AsInternal());
		WindowMessageBox(wSciTE, msg);
		return false;
	} else {
		SaveAs(absPath.AsInternal(), fixCase);
		return true;
	}
}

void SciTEBase::AbandonAutomaticSave() {
	CurrentBuffer()->AbandonAutomaticSave();
}

bool SciTEBase::IsStdinBlocked() {
	return false; /* always default to blocked */
}

void SciTEBase::OpenFromStdin(bool UseOutputPane) {
	Utf8_16_Read convert;
	std::vector<char> data(blockSize);

	/* if stdin is blocked, do not execute this method */
	if (IsStdinBlocked())
		return;

	Open(FilePath());
	if (UseOutputPane) {
		wOutput.ClearAll();
	} else {
		wEditor.BeginUndoAction();	// Group together clear and insert
		wEditor.ClearAll();
	}
	size_t lenFile = fread(&data[0], 1, data.size(), stdin);
	const UniMode umCodingCookie = CodingCookieValue(std::string_view(data.data(), lenFile));
	while (lenFile > 0) {
		lenFile = convert.convert(&data[0], lenFile);
		if (UseOutputPane) {
			wOutput.AddText(lenFile, convert.getNewBuf());
		} else {
			wEditor.AddText(lenFile, convert.getNewBuf());
		}
		lenFile = fread(&data[0], 1, data.size(), stdin);
	}
	if (UseOutputPane) {
		if (props.GetInt("split.vertical") == 0) {
			heightOutput = 2000;
		} else {
			heightOutput = 500;
		}
		SizeSubWindows();
	} else {
		wEditor.EndUndoAction();
	}
	CurrentBuffer()->unicodeMode = static_cast<UniMode>(
					       static_cast<int>(convert.getEncoding()));
	// Check the first two lines for coding cookies
	if (CurrentBuffer()->unicodeMode == uni8Bit) {
		CurrentBuffer()->unicodeMode = umCodingCookie;
	}
	if (CurrentBuffer()->unicodeMode != uni8Bit) {
		// Override the code page if Unicode
		codePage = SA::CpUtf8;
	} else {
		codePage = props.GetInt("code.page");
	}
	if (UseOutputPane) {
		wOutput.SetSel(0, 0);
	} else {
		wEditor.SetCodePage(codePage);

		// Zero all the style bytes
		wEditor.ClearDocumentStyle();

		CurrentBuffer()->overrideExtension = "x.txt";
		ReadProperties();
		SetIndentSettings();
		wEditor.ColouriseAll();
		Redraw();

		wEditor.SetSel(0, 0);
	}
}

void SciTEBase::OpenFilesFromStdin() {
	char data[8 * 1024] {};

	/* if stdin is blocked, do not execute this method */
	if (IsStdinBlocked())
		return;

	while (fgets(data, sizeof(data) - 1, stdin)) {
		char *pNL;
		if ((pNL = strchr(data, '\n')) != nullptr)
			* pNL = '\0';
		Open(GUI::StringFromUTF8(data), ofQuiet);
	}
	if (buffers.lengthVisible == 0)
		Open(FilePath());
}

class BufferedFile {
	FILE *fp;
	bool readAll;
	bool exhausted;
	enum {bufLen = 64 * 1024};
	char buffer[bufLen];
	size_t pos;
	size_t valid;
	void EnsureData() noexcept {
		if (pos >= valid) {
			if (readAll || !fp) {
				exhausted = true;
			} else {
				valid = fread(buffer, 1, bufLen, fp);
				if (valid < bufLen) {
					readAll = true;
				}
				pos = 0;
			}
		}
	}
public:
	explicit BufferedFile(const FilePath &fPath) {
		fp = fPath.Open(fileRead);
		readAll = false;
		exhausted = fp == nullptr;
		buffer[0] = 0;
		pos = 0;
		valid = 0;
	}
	~BufferedFile() {
		if (fp) {
			fclose(fp);
		}
		fp = nullptr;
	}
	bool Exhausted() const noexcept {
		return exhausted;
	}
	int NextByte() noexcept {
		EnsureData();
		if (pos >= valid) {
			return 0;
		}
		return buffer[pos++];
	}
	bool BufferContainsNull() noexcept {
		EnsureData();
		for (size_t i = 0; i < valid; i++) {
			if (buffer[i] == '\0')
				return true;
		}
		return false;
	}
};

class FileReader {
	std::unique_ptr<BufferedFile> bf;
	int lineNum;
	bool lastWasCR;
	std::string lineToCompare;
	std::string lineToShow;
	bool caseSensitive;
public:
	FileReader(const FilePath &fPath, bool caseSensitive_) : bf(std::make_unique<BufferedFile>(fPath)) {
		lineNum = 0;
		lastWasCR = false;
		caseSensitive = caseSensitive_;
	}
	// Deleted so FileReader objects can not be copied.
	FileReader(const FileReader &) = delete;
	~FileReader() {
	}
	const char *Next() {
		if (bf->Exhausted()) {
			return nullptr;
		}
		lineToShow.clear();
		while (!bf->Exhausted()) {
			const int ch = bf->NextByte();
			if (lastWasCR && ch == '\n' && lineToShow.empty()) {
				lastWasCR = false;
			} else if (ch == '\r' || ch == '\n') {
				lastWasCR = ch == '\r';
				break;
			} else {
				lineToShow.push_back(static_cast<char>(ch));
			}
		}
		lineNum++;
		lineToCompare = lineToShow;
		if (!caseSensitive) {
			LowerCaseAZ(lineToCompare);
		}
		return lineToCompare.c_str();
	}
	int LineNumber() const noexcept {
		return lineNum;
	}
	const char *Original() const noexcept {
		return lineToShow.c_str();
	}
	bool BufferContainsNull() noexcept {
		return bf->BufferContainsNull();
	}
};

constexpr bool IsWordCharacter(int ch) noexcept {
	return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')  || (ch >= '0' && ch <= '9')  || (ch == '_');
}

bool SciTEBase::GrepIntoDirectory(const FilePath &directory) {
	const GUI::gui_char *sDirectory = directory.AsInternal();
#ifdef __APPLE__
	if (strcmp(sDirectory, "build") == 0)
		return false;
#endif
	return sDirectory[0] != '.';
}

void SciTEBase::GrepRecursive(GrepFlags gf, const FilePath &baseDir, const char *searchString, const GUI::gui_char *fileTypes) {
	constexpr int checkAfterLines = 10'000;
	FilePathSet directories;
	FilePathSet files;
	baseDir.List(directories, files);
	const size_t searchLength = strlen(searchString);
	std::string os;
	for (const FilePath &fPath : files) {
		if (jobQueue.Cancelled())
			return;
		if (*fileTypes == '\0' || fPath.Matches(fileTypes)) {
			//OutputAppendStringSynchronised(i->AsInternal());
			//OutputAppendStringSynchronised("\n");
			FileReader fr(fPath, gf & grepMatchCase);
			if ((gf & grepBinary) || !fr.BufferContainsNull()) {
				while (const char *line = fr.Next()) {
					if (((fr.LineNumber() % checkAfterLines) == 0) && jobQueue.Cancelled())
						return;
					const char *match = strstr(line, searchString);
					if (match) {
						if (gf & grepWholeWord) {
							const char *lineEnd = line + strlen(line);
							while (match) {
								if (((match == line) || !IsWordCharacter(match[-1])) &&
										((match + searchLength == (lineEnd)) || !IsWordCharacter(match[searchLength]))) {
									break;
								}
								match = strstr(match + 1, searchString);
							}
						}
						if (match) {
							os.append(fPath.AsUTF8().c_str());
							os.append(":");
							std::string lNumber = StdStringFromInteger(fr.LineNumber());
							os.append(lNumber.c_str());
							os.append(":");
							os.append(fr.Original());
							os.append("\n");
						}
					}
				}
			}
		}
	}
	if (os.length()) {
		if (gf & grepStdOut) {
			fwrite(os.c_str(), os.length(), 1, stdout);
		} else {
			OutputAppendStringSynchronised(os.c_str());
		}
	}
	for (const FilePath &fPath : directories) {
		if ((gf & grepDot) || GrepIntoDirectory(fPath.Name())) {
			GrepRecursive(gf, fPath, searchString, fileTypes);
		}
	}
}

void SciTEBase::InternalGrep(GrepFlags gf, const GUI::gui_char *directory, const GUI::gui_char *fileTypes, const char *search, SA::Position &originalEnd) {
	GUI::ElapsedTime commandTime;
	if (!(gf & grepStdOut)) {
		std::string os;
		os.append(">Internal search for \"");
		os.append(search);
		os.append("\" in \"");
		os.append(GUI::UTF8FromString(fileTypes).c_str());
		os.append("\"\n");
		OutputAppendStringSynchronised(os.c_str());
		ShowOutputOnMainThread();
		originalEnd += os.length();
	}
	std::string searchString(search);
	if (!(gf & grepMatchCase)) {
		LowerCaseAZ(searchString);
	}
	GrepRecursive(gf, FilePath(directory), searchString.c_str(), fileTypes);
	if (!(gf & grepStdOut)) {
		std::string sExitMessage(">");
		if (jobQueue.TimeCommands()) {
			sExitMessage += "    Time: ";
			sExitMessage += StdStringFromDouble(commandTime.Duration(), 3);
		}
		sExitMessage += "\n";
		OutputAppendStringSynchronised(sExitMessage.c_str());
	}
}

