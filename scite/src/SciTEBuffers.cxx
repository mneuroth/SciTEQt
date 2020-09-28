// SciTE - Scintilla based Text Editor
/** @file SciTEBuffers.cxx
 ** Buffers and jobs management.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
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

#include "ILoader.h"

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

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
#include "SciTEBase.h"

const GUI::gui_char defaultSessionFileName[] = GUI_TEXT("SciTE.session");

void Buffer::DocumentModified() noexcept {
	documentModTime = time(nullptr);
}

bool Buffer::NeedsSave(int delayBeforeSave) const {
	const time_t now = time(nullptr);
	return now && documentModTime && isDirty && !pFileWorker && (now-documentModTime > delayBeforeSave) && !file.IsUntitled() && !failedSave;
}

void Buffer::CompleteLoading() noexcept {
	lifeState = opened;
	if (pFileWorker && pFileWorker->IsLoading()) {
		delete pFileWorker;
		pFileWorker = nullptr;
	}
}

void Buffer::CompleteStoring() {
	if (pFileWorker && !pFileWorker->IsLoading()) {
		delete pFileWorker;
		pFileWorker = nullptr;
	}
	SetTimeFromFile();
}

void Buffer::AbandonAutomaticSave() {
	if (pFileWorker && !pFileWorker->IsLoading()) {
		const FileStorer *pFileStorer = static_cast<FileStorer *>(pFileWorker);
		if (!pFileStorer->visibleProgress) {
			pFileWorker->Cancel();
			// File is in partially saved state so may be better to remove
		}
	}
}

void Buffer::CancelLoad() {
	// Complete any background loading
	if (pFileWorker && pFileWorker->IsLoading()) {
		pFileWorker->Cancel();
		CompleteLoading();
		lifeState = empty;
	}
}

BufferList::BufferList() : current(0), stackcurrent(0), stack(0), buffers(0), length(0), lengthVisible(0), initialised(false) {}

BufferList::~BufferList() {
}

void BufferList::Allocate(int maxSize) {
	length = 1;
	lengthVisible = 1;
	current = 0;
	buffers.resize(maxSize);
	stack.resize(maxSize);
	stack[0] = 0;
}

int BufferList::Add() {
	if (length < size()) {
		length++;
	}
	buffers[length - 1].Init();
	stack[length - 1] = length - 1;
	MoveToStackTop(length - 1);
	SetVisible(length-1, true);

	return lengthVisible - 1;
}

int BufferList::GetDocumentByWorker(const FileWorker *pFileWorker) const {
	for (int i = 0; i < length; i++) {
		if (buffers[i].pFileWorker == pFileWorker) {
			return i;
		}
	}
	return -1;
}

int BufferList::GetDocumentByName(const FilePath &filename, bool excludeCurrent) {
	if (!filename.IsSet()) {
		return -1;
	}
	for (int i = 0; i < length; i++) {
		if ((!excludeCurrent || i != current) && buffers[i].file.SameNameAs(filename)) {
			return i;
		}
	}
	return -1;
}

void BufferList::RemoveInvisible(int index) {
	assert(!GetVisible(index));
	if (index == current) {
		RemoveCurrent();
	} else {
		if (index < length-1) {
			// Swap with last visible
			Swap(index, length-1);
		}
		length--;
	}
}

void BufferList::RemoveCurrent() {
	// Delete and move up to fill gap but ensure doc pointer is saved.
	void *currentDoc = buffers[current].doc;
	buffers[current].CompleteLoading();
	for (int i = current; i < length - 1; i++) {
		buffers[i] = buffers[i + 1];
	}
	buffers[length - 1].doc = currentDoc;

	if (length > 1) {
		CommitStackSelection();
		PopStack();
		length--;
		lengthVisible--;

		buffers[length].Init();
		if (current >= lengthVisible) {
			if (lengthVisible > 0) {
				SetCurrent(lengthVisible - 1);
			} else {
				SetCurrent(0);
			}
		}
	} else {
		buffers[current].Init();
	}
	MoveToStackTop(current);
}

int BufferList::Current() const noexcept {
	return current;
}

Buffer *BufferList::CurrentBuffer() {
	return &buffers[Current()];
}

const Buffer *BufferList::CurrentBufferConst() const {
	return &buffers[Current()];
}

void BufferList::SetCurrent(int index) noexcept {
	current = index;
}

void BufferList::PopStack() {
	for (int i = 0; i < length - 1; ++i) {
		int index = stack[i + 1];
		// adjust the index for items that will move in buffers[]
		if (index > current)
			--index;
		stack[i] = index;
	}
}

int BufferList::StackNext() {
	if (++stackcurrent >= length)
		stackcurrent = 0;
	return stack[stackcurrent];
}

int BufferList::StackPrev() {
	if (--stackcurrent < 0)
		stackcurrent = length - 1;
	return stack[stackcurrent];
}

void BufferList::MoveToStackTop(int index) {
	// shift top chunk of stack down into the slot that index occupies
	bool move = false;
	for (int i = length - 1; i > 0; --i) {
		if (stack[i] == index)
			move = true;
		if (move)
			stack[i] = stack[i-1];
	}
	stack[0] = index;
}

void BufferList::CommitStackSelection() {
	// called only when ctrl key is released when ctrl-tabbing
	// or when a document is closed (in case of Ctrl+F4 during ctrl-tabbing)
	MoveToStackTop(stack[stackcurrent]);
	stackcurrent = 0;
}

void BufferList::ShiftTo(int indexFrom, int indexTo) {
	// shift buffer to new place in buffers array
	if (indexFrom == indexTo ||
			indexFrom < 0 || indexFrom >= length ||
			indexTo < 0 || indexTo >= length) return;
	int step = (indexFrom > indexTo) ? -1 : 1;
	Buffer tmp = buffers[indexFrom];
	int i;
	for (i = indexFrom; i != indexTo; i += step) {
		buffers[i] = buffers[i+step];
	}
	buffers[indexTo] = tmp;
	// update stack indexes
	for (i = 0; i < length; i++) {
		if (stack[i] == indexFrom) {
			stack[i] = indexTo;
		} else if (step == 1) {
			if (indexFrom < stack[i] && stack[i] <= indexTo) stack[i] -= step;
		} else {
			if (indexFrom > stack[i] && stack[i] >= indexTo) stack[i] -= step;
		}
	}
}

void BufferList::Swap(int indexA, int indexB) {
	// shift buffer to new place in buffers array
	if (indexA == indexB ||
			indexA < 0 || indexA >= length ||
			indexB < 0 || indexB >= length) return;
	Buffer tmp = buffers[indexA];
	buffers[indexA] = buffers[indexB];
	buffers[indexB] = tmp;
	// update stack indexes
	for (int i = 0; i < length; i++) {
		if (stack[i] == indexA) {
			stack[i] = indexB;
		} else if (stack[i] == indexB) {
			stack[i] = indexA;
		}
	}
}

bool BufferList::SingleBuffer() const noexcept {
	return size() == 1;
}

BackgroundActivities BufferList::CountBackgroundActivities() const {
	BackgroundActivities bg {};
	for (int i = 0; i < length; i++) {
		if (buffers[i].pFileWorker) {
			if (!buffers[i].pFileWorker->FinishedJob()) {
				if (!buffers[i].pFileWorker->IsLoading()) {
					const FileStorer *fstorer = static_cast<FileStorer *>(buffers[i].pFileWorker);
					if (!fstorer->visibleProgress)
						continue;
				}
				if (buffers[i].pFileWorker->IsLoading())
					bg.loaders++;
				else
					bg.storers++;
				bg.fileNameLast = buffers[i].file.AsInternal();
				bg.totalWork += buffers[i].pFileWorker->SizeJob();
				bg.totalProgress += buffers[i].pFileWorker->ProgressMade();
			}
		}
	}
	return bg;
}

bool BufferList::SavingInBackground() const {
	for (int i = 0; i<length; i++) {
		if (buffers[i].pFileWorker && !buffers[i].pFileWorker->IsLoading() && !buffers[i].pFileWorker->FinishedJob()) {
			return true;
		}
	}
	return false;
}

bool BufferList::GetVisible(int index) const noexcept {
	return index < lengthVisible;
}

void BufferList::SetVisible(int index, bool visible) {
	if (visible != GetVisible(index)) {
		if (visible) {
			if (index > lengthVisible) {
				// Swap with first invisible
				Swap(index, lengthVisible);
			}
			lengthVisible++;
		} else {
			if (index < lengthVisible-1) {
				// Swap with last visible
				Swap(index, lengthVisible-1);
			}
			lengthVisible--;
			if (current >= lengthVisible && lengthVisible > 0)
				SetCurrent(lengthVisible-1);
		}
	}
}

void BufferList::AddFuture(int index, Buffer::FutureDo fd) {
	if (index >= 0 || index < length) {
		buffers[index].futureDo = static_cast<Buffer::FutureDo>(buffers[index].futureDo | fd);
	}
}

void BufferList::FinishedFuture(int index, Buffer::FutureDo fd) {
	if (index >= 0 || index < length) {
		buffers[index].futureDo = static_cast<Buffer::FutureDo>(buffers[index].futureDo & ~(fd));
	}
}

void *SciTEBase::GetDocumentAt(int index) {
	if (index < 0 || index >= buffers.size()) {
		return nullptr;
	}
	if (buffers.buffers[index].doc == nullptr) {
		// Create a new document buffer
		buffers.buffers[index].doc = wEditor.CreateDocument(0, SA::DocumentOption::Default);
	}
	return buffers.buffers[index].doc;
}

void SciTEBase::SwitchDocumentAt(int index, void *pdoc) {
	if (index < 0 || index >= buffers.size()) {
		return;
	}
	void *pdocOld = buffers.buffers[index].doc;
	buffers.buffers[index].doc = pdoc;
	if (pdocOld) {
		wEditor.ReleaseDocument(pdocOld);
	}
	if (index == buffers.Current()) {
		wEditor.SetDocPointer(buffers.buffers[index].doc);
	}
}

void SciTEBase::SetDocumentAt(int index, bool updateStack) {
	const int currentbuf = buffers.Current();

	if (index < 0 ||
			index >= buffers.length ||
			index == currentbuf ||
			currentbuf < 0 ||
			currentbuf >= buffers.length) {
		return;
	}
	UpdateBuffersCurrent();

	buffers.SetCurrent(index);
	if (updateStack) {
		buffers.MoveToStackTop(index);
	}

	if (extender) {
		if (buffers.size() > 1)
			extender->ActivateBuffer(index);
		else
			extender->InitBuffer(0);
	}

	const Buffer &bufferNext = buffers.buffers[buffers.Current()];
	SetFileName(bufferNext.file);
	propsDiscovered = bufferNext.props;
	propsDiscovered.superPS = &propsLocal;
	wEditor.SetDocPointer(GetDocumentAt(buffers.Current()));
	const bool restoreBookmarks = bufferNext.lifeState == Buffer::readAll;
	PerformDeferredTasks();
	if (bufferNext.lifeState == Buffer::readAll) {
		CompleteOpen(ocCompleteSwitch);
		if (extender)
			extender->OnOpen(filePath.AsUTF8().c_str());
	}
	RestoreState(bufferNext, restoreBookmarks);

	TabSelect(index);

	if (lineNumbers && lineNumbersExpand)
		SetLineNumberWidth();

	DisplayAround(bufferNext.file);
	if (restoreBookmarks) {
		// Restoring a session does not restore the scroll position
		// so make the selection visible.
		wEditor.ScrollCaret();
	}

	SetBuffersMenu();
	CheckMenus();
	UpdateStatusBar(true);

	if (extender) {
		extender->OnSwitchFile(filePath.AsUTF8().c_str());
	}
}

void SciTEBase::UpdateBuffersCurrent() {
	const int currentbuf = buffers.Current();

	if ((buffers.length > 0) && (currentbuf >= 0) && (buffers.GetVisible(currentbuf))) {
		Buffer &bufferCurrent = buffers.buffers[currentbuf];
		bufferCurrent.file.Set(filePath);
		if (bufferCurrent.lifeState != Buffer::reading && bufferCurrent.lifeState != Buffer::readAll) {
			bufferCurrent.file.selection.position = wEditor.CurrentPos();
			bufferCurrent.file.selection.anchor = wEditor.Anchor();
			bufferCurrent.file.scrollPosition = GetCurrentScrollPosition();

			// Retrieve fold state and store in buffer state info

			std::vector<SA::Line> *f = &bufferCurrent.foldState;
			f->clear();

			if (props.GetInt("fold")) {
				for (SA::Line line = 0; ; line++) {
					const SA::Line lineNext = wEditor.ContractedFoldNext(line);
					if ((line < 0) || (lineNext < line))
						break;
					line = lineNext;
					f->push_back(line);
				}
			}

			if (props.GetInt("session.bookmarks")) {
				buffers.buffers[buffers.Current()].bookmarks.clear();
				SA::Line lineBookmark = -1;
				while ((lineBookmark = wEditor.MarkerNext(lineBookmark + 1, 1 << markerBookmark)) >= 0) {
					bufferCurrent.bookmarks.push_back(lineBookmark);
				}
			}
		}
	}
}

bool SciTEBase::IsBufferAvailable() const noexcept {
	return buffers.size() > 1 && buffers.length < buffers.size();
}

bool SciTEBase::CanMakeRoom(bool maySaveIfDirty) {
	if (IsBufferAvailable()) {
		return true;
	} else if (maySaveIfDirty) {
		// All available buffers are taken, try and close the current one
		if (SaveIfUnsure(true, static_cast<SaveFlags>(sfProgressVisible | sfSynchronous)) != saveCancelled) {
			// The file isn't dirty, or the user agreed to close the current one
			return true;
		}
	} else {
		return true;	// Told not to save so must be OK.
	}
	return false;
}

void SciTEBase::ClearDocument() {
	wEditor.SetReadOnly(false);
	wEditor.SetUndoCollection(false);
	wEditor.ClearAll();
	wEditor.EmptyUndoBuffer();
	wEditor.SetUndoCollection(true);
	wEditor.SetSavePoint();
	wEditor.SetReadOnly(CurrentBuffer()->isReadOnly);
}

void SciTEBase::CreateBuffers() {
	int buffersWanted = props.GetInt("buffers");
	if (buffersWanted > bufferMax) {
		buffersWanted = bufferMax;
	}
	if (buffersWanted < 1) {
		buffersWanted = 1;
	}
	buffers.Allocate(buffersWanted);
}

void SciTEBase::InitialiseBuffers() {
	if (!buffers.initialised) {
		buffers.initialised = true;
		// First document is the default from creation of control
		buffers.buffers[0].doc = wEditor.DocPointer();
		wEditor.AddRefDocument(buffers.buffers[0].doc); // We own this reference
		if (buffers.size() == 1) {
			// Single buffer mode, delete the Buffers main menu entry
			DestroyMenuItem(menuBuffers, 0);
			// Destroy command "View Tab Bar" in the menu "View"
			DestroyMenuItem(menuView, IDM_VIEWTABBAR);
			// Make previous change visible.
			RedrawMenu();
		}
	}
}

FilePath SciTEBase::UserFilePath(const GUI::gui_char *name) {
	GUI::gui_string nameWithVisibility(configFileVisibilityString);
	nameWithVisibility += name;
	return FilePath(GetSciteUserHome(), nameWithVisibility.c_str());
}

static std::string IndexPropKey(const char *bufPrefix, int bufIndex, const char *bufAppendix) {
	std::string pKey = bufPrefix;
	pKey += '.';
	pKey += StdStringFromInteger(bufIndex + 1);
	if (bufAppendix) {
		pKey += ".";
		pKey += bufAppendix;
	}
	return pKey;
}

void SciTEBase::LoadSessionFile(const GUI::gui_char *sessionName) {
	FilePath sessionPathName;
	if (sessionName[0] == '\0') {
		sessionPathName = UserFilePath(defaultSessionFileName);
	} else {
		sessionPathName.Set(sessionName);
	}

	propsSession.Clear();
	propsSession.Read(sessionPathName, sessionPathName.Directory(), filter, nullptr, 0);

	FilePath sessionFilePath = FilePath(sessionPathName).AbsolutePath();
	// Add/update SessionPath environment variable
	props.Set("SessionPath", sessionFilePath.AsUTF8().c_str());
}

void SciTEBase::RestoreRecentMenu() {
	const SelectedRange sr(0, 0);

	DeleteFileStackMenu();

	for (int i = 0; i < fileStackMax; i++) {
		std::string propKey = IndexPropKey("mru", i, "path");
		std::string propStr = propsSession.GetString(propKey.c_str());
		if (propStr == "")
			continue;
		AddFileToStack(RecentFile(GUI::StringFromUTF8(propStr), sr, 0));
	}
}

namespace {

// Line numbers are 0-based inside SciTE but are saved in session files as 1-based.

std::vector<SA::Line> LinesFromString(const std::string &s) {
	std::vector<SA::Line> result;
	if (s.length()) {
		size_t start = 0;
		for (;;) {
			const SA::Line line = IntegerFromText(s.c_str() + start) - 1;
			result.push_back(line);
			const size_t posComma = s.find(',', start);
			if (posComma == std::string::npos)
				break;
			start = posComma + 1;
		}
	}
	return result;
}

std::string StringFromLines(const std::vector<SA::Line> &lines) {
	std::string result;
	for (const SA::Line line : lines) {
		if (result.length()) {
			result.append(",");
		}
		std::string sLine = std::to_string(line + 1);
		result.append(sLine);
	}
	return result;
}

}

void SciTEBase::RestoreFromSession(const Session &session) {
	for (const BufferState &buffer : session.buffers)
		AddFileToBuffer(buffer);
	const int iBuffer = buffers.GetDocumentByName(session.pathActive);
	if (iBuffer >= 0)
		SetDocumentAt(iBuffer);
}

void SciTEBase::RestoreSession() {
	if (props.GetInt("save.find") != 0) {
		for (int i = 0;; i++) {
			std::string propKey = IndexPropKey("search", i, "findwhat");
			std::string propStr = propsSession.GetString(propKey.c_str());
			if (propStr == "")
				break;
			memFinds.AppendList(propStr.c_str());
		}

		for (int i = 0;; i++) {
			std::string propKey = IndexPropKey("search", i, "replacewith");
			std::string propStr = propsSession.GetString(propKey.c_str());
			if (propStr == "")
				break;
			memReplaces.AppendList(propStr.c_str());
		}
	}

	// Comment next line if you don't want to close all buffers before restoring session
	CloseAllBuffers(true);

	Session session;

	for (int i = 0; i < bufferMax; i++) {
		std::string propKey = IndexPropKey("buffer", i, "path");
		std::string propStr = propsSession.GetString(propKey.c_str());
		if (propStr == "")
			continue;

		BufferState bufferState;
		bufferState.file.Set(GUI::StringFromUTF8(propStr));

		propKey = IndexPropKey("buffer", i, "current");
		if (propsSession.GetInt(propKey.c_str()))
			session.pathActive = bufferState.file;

		propKey = IndexPropKey("buffer", i, "scroll");
		const SA::Line scroll = propsSession.GetInteger(propKey.c_str());
		bufferState.file.scrollPosition = scroll;

		propKey = IndexPropKey("buffer", i, "position");
		const SA::Position pos = propsSession.GetInteger(propKey.c_str());

		bufferState.file.selection.anchor = pos - 1;
		bufferState.file.selection.position = bufferState.file.selection.anchor;

		if (props.GetInt("session.bookmarks")) {
			propKey = IndexPropKey("buffer", i, "bookmarks");
			propStr = propsSession.GetString(propKey.c_str());
			bufferState.bookmarks = LinesFromString(propStr);
		}

		if (props.GetInt("fold") && !props.GetInt("fold.on.open") &&
				props.GetInt("session.folds")) {
			propKey = IndexPropKey("buffer", i, "folds");
			propStr = propsSession.GetString(propKey.c_str());
			bufferState.foldState = LinesFromString(propStr);
		}

		session.buffers.push_back(bufferState);
	}

	RestoreFromSession(session);
}

void SciTEBase::SaveSessionFile(const GUI::gui_char *sessionName) {
	UpdateBuffersCurrent();
	const bool defaultSession = !*sessionName;
	FilePath sessionPathName;
	if (defaultSession) {
		sessionPathName = UserFilePath(defaultSessionFileName);
	} else {
		sessionPathName.Set(sessionName);
	}
	FILE *sessionFile = sessionPathName.Open(fileWrite);
	if (!sessionFile)
		return;

	fprintf(sessionFile, "# SciTE session file\n");

	if (defaultSession && props.GetInt("save.position")) {
		int top, left, width, height, maximize;
		GetWindowPosition(&left, &top, &width, &height, &maximize);

		fprintf(sessionFile, "\n");
		fprintf(sessionFile, "position.left=%d\n", left);
		fprintf(sessionFile, "position.top=%d\n", top);
		fprintf(sessionFile, "position.width=%d\n", width);
		fprintf(sessionFile, "position.height=%d\n", height);
		fprintf(sessionFile, "position.maximize=%d\n", maximize);
	}

	if (defaultSession && props.GetInt("save.recent")) {
		std::string propKey;
		int j = 0;

		fprintf(sessionFile, "\n");

		// Save recent files list
		for (int i = fileStackMax - 1; i >= 0; i--) {
			if (recentFileStack[i].IsSet()) {
				propKey = IndexPropKey("mru", j++, "path");
				fprintf(sessionFile, "%s=%s\n", propKey.c_str(), recentFileStack[i].AsUTF8().c_str());
			}
		}
	}

	if (defaultSession && props.GetInt("save.find")) {
		std::string propKey;
		std::vector<std::string>::iterator it;
		std::vector<std::string> mem = memFinds.AsVector();
		if (!mem.empty()) {
			fprintf(sessionFile, "\n");
			it = mem.begin();
			for (int i = 0; it != mem.end(); i++, ++it) {
				propKey = IndexPropKey("search", i, "findwhat");
				fprintf(sessionFile, "%s=%s\n", propKey.c_str(), (*it).c_str());
			}
		}

		mem = memReplaces.AsVector();
		if (!mem.empty()) {
			fprintf(sessionFile, "\n");
			mem = memReplaces.AsVector();
			it = mem.begin();
			for (int i = 0; it != mem.end(); i++, ++it) {
				propKey = IndexPropKey("search", i, "replacewith");
				fprintf(sessionFile, "%s=%s\n", propKey.c_str(), (*it).c_str());
			}
		}
	}

	if (props.GetInt("buffers") && (!defaultSession || props.GetInt("save.session"))) {
		const int curr = buffers.Current();
		for (int i = 0; i < buffers.lengthVisible; i++) {
			const Buffer &buff = buffers.buffers[i];
			if (buff.file.IsSet() && !buff.file.IsUntitled()) {
				std::string propKey = IndexPropKey("buffer", i, "path");
				fprintf(sessionFile, "\n%s=%s\n", propKey.c_str(), buff.file.AsUTF8().c_str());

				const SA::Position pos = buff.file.selection.position + 1;
				const std::string sPos = std::to_string(pos);
				propKey = IndexPropKey("buffer", i, "position");
				fprintf(sessionFile, "%s=%s\n", propKey.c_str(), sPos.c_str());

				const SA::Line scroll = buff.file.scrollPosition;
				const std::string sScroll = std::to_string(scroll);
				propKey = IndexPropKey("buffer", i, "scroll");
				fprintf(sessionFile, "%s=%s\n", propKey.c_str(), sScroll.c_str());

				if (i == curr) {
					propKey = IndexPropKey("buffer", i, "current");
					fprintf(sessionFile, "%s=1\n", propKey.c_str());
				}

				if (props.GetInt("session.bookmarks")) {
					const std::string bmString = StringFromLines(buff.bookmarks);
					if (bmString.length()) {
						propKey = IndexPropKey("buffer", i, "bookmarks");
						fprintf(sessionFile, "%s=%s\n", propKey.c_str(), bmString.c_str());
					}
				}

				if (props.GetInt("fold") && props.GetInt("session.folds")) {
					const std::string foldsString = StringFromLines(buff.foldState);
					if (foldsString.length()) {
						propKey = IndexPropKey("buffer", i, "folds");
						fprintf(sessionFile, "%s=%s\n", propKey.c_str(), foldsString.c_str());
					}
				}
			}
		}
	}

	if (fclose(sessionFile) != 0) {
		FailedSaveMessageBox(sessionPathName);
	}

	FilePath sessionFilePath = FilePath(sessionPathName).AbsolutePath();
	// Add/update SessionPath environment variable
	props.Set("SessionPath", sessionFilePath.AsUTF8().c_str());
}

void SciTEBase::SetIndentSettings() {
	// Get default values
	const int useTabs = props.GetInt("use.tabs", 1);
	const int tabSize = props.GetInt("tabsize");
	const int indentSize = props.GetInt("indent.size");
	// Either set the settings related to the extension or the default ones
	std::string fileNameForExtension = ExtensionFileName();
	std::string useTabsChars = props.GetNewExpandString("use.tabs.",
				   fileNameForExtension.c_str());
	if (useTabsChars.length() != 0) {
		wEditor.SetUseTabs(atoi(useTabsChars.c_str()));
	} else {
		wEditor.SetUseTabs(useTabs);
	}
	std::string tabSizeForExt = props.GetNewExpandString("tab.size.",
				    fileNameForExtension.c_str());
	if (tabSizeForExt.length() != 0) {
		wEditor.SetTabWidth(atoi(tabSizeForExt.c_str()));
	} else if (tabSize != 0) {
		wEditor.SetTabWidth(tabSize);
	}
	std::string indentSizeForExt = props.GetNewExpandString("indent.size.",
				       fileNameForExtension.c_str());
	if (indentSizeForExt.length() != 0) {
		wEditor.SetIndent(atoi(indentSizeForExt.c_str()));
	} else {
		wEditor.SetIndent(indentSize);
	}
}

void SciTEBase::SetEol() {
	std::string eolMode = props.GetString("eol.mode");
	if (eolMode == "LF") {
		wEditor.SetEOLMode(SA::EndOfLine::Lf);
	} else if (eolMode == "CR") {
		wEditor.SetEOLMode(SA::EndOfLine::Cr);
	} else if (eolMode == "CRLF") {
		wEditor.SetEOLMode(SA::EndOfLine::CrLf);
	}
}

void SciTEBase::New() {
	InitialiseBuffers();
	UpdateBuffersCurrent();

	propsDiscovered.Clear();

	if ((buffers.size() == 1) && (!buffers.buffers[0].file.IsUntitled())) {
		AddFileToStack(buffers.buffers[0].file);
	}

	// If the current buffer is the initial untitled, clean buffer then overwrite it,
	// otherwise add a new buffer.
	if ((buffers.length > 1) ||
			(buffers.Current() != 0) ||
			(buffers.buffers[0].isDirty) ||
			(!buffers.buffers[0].file.IsUntitled())) {
		if (buffers.size() == buffers.length) {
			Close(false, false, true);
		}
		buffers.SetCurrent(buffers.Add());
	}

	void *doc = GetDocumentAt(buffers.Current());
	wEditor.SetDocPointer(doc);

	FilePath curDirectory(filePath.Directory());
	filePath.Set(curDirectory, GUI_TEXT(""));
	SetFileName(filePath);
	UpdateBuffersCurrent();
	SetBuffersMenu();
	CurrentBuffer()->isDirty = false;
	CurrentBuffer()->failedSave = false;
	CurrentBuffer()->lifeState = Buffer::opened;
	jobQueue.isBuilding = false;
	jobQueue.isBuilt = false;
	CurrentBuffer()->isReadOnly = false;	// No sense to create an empty, read-only buffer...

	ClearDocument();
	DeleteFileStackMenu();
	SetFileStackMenu();
	if (extender)
		extender->InitBuffer(buffers.Current());
}

void SciTEBase::RestoreState(const Buffer &buffer, bool restoreBookmarks) {
	SetWindowName();
	ReadProperties();
	if (CurrentBuffer()->unicodeMode != uni8Bit) {
		// Override the code page if Unicode
		codePage = SA::CpUtf8;
		wEditor.SetCodePage(codePage);
	}

	// check to see whether there is saved fold state, restore
	if (!buffer.foldState.empty()) {
		wEditor.ColouriseAll();
		for (const SA::Line fold : buffer.foldState) {
			wEditor.ToggleFold(fold);
		}
	}
	if (restoreBookmarks) {
		for (const SA::Line bookmark : buffer.bookmarks) {
			wEditor.MarkerAdd(bookmark, markerBookmark);
		}
	}
}

void SciTEBase::Close(bool updateUI, bool loadingSession, bool makingRoomForNew) {
	bool closingLast = true;
	const int index = buffers.Current();
	if ((index >= 0) && buffers.initialised) {
		buffers.buffers[index].CancelLoad();
	}

	if (extender) {
		extender->OnClose(filePath.AsUTF8().c_str());
	}

	if (buffers.size() == 1) {
		// With no buffer list, Close means close from MRU
		closingLast = !(recentFileStack[0].IsSet());
		buffers.buffers[0].Init();
		filePath.Set(GUI_TEXT(""));
		ClearDocument(); //avoid double are-you-sure
		if (!makingRoomForNew)
			StackMenu(0); // calls New, or Open, which calls InitBuffer
	} else if (buffers.size() > 1) {
		if (buffers.Current() >= 0 && buffers.Current() < buffers.length) {
			UpdateBuffersCurrent();
			AddFileToStack(CurrentBufferConst()->file);
		}
		closingLast = (buffers.lengthVisible == 1) && !buffers.buffers[0].pFileWorker;
		if (closingLast) {
			buffers.buffers[0].Init();
			buffers.buffers[0].lifeState = Buffer::opened;
			if (extender)
				extender->InitBuffer(0);
		} else {
			if (extender)
				extender->RemoveBuffer(buffers.Current());
			if (buffers.buffers[buffers.Current()].pFileWorker) {
				buffers.SetVisible(buffers.Current(), false);
				if (buffers.lengthVisible == 0)
					New();
			} else {
				wEditor.SetReadOnly(false);
				ClearDocument();
				buffers.RemoveCurrent();
			}
			if (extender && !makingRoomForNew)
				extender->ActivateBuffer(buffers.Current());
		}
		const Buffer &bufferNext = buffers.buffers[buffers.Current()];

		if (updateUI)
			SetFileName(bufferNext.file);
		else
			filePath = bufferNext.file;
		propsDiscovered = bufferNext.props;
		propsDiscovered.superPS = &propsLocal;
		wEditor.SetDocPointer(GetDocumentAt(buffers.Current()));
		PerformDeferredTasks();
		if (bufferNext.lifeState == Buffer::readAll) {
			//restoreBookmarks = true;
			CompleteOpen(ocCompleteSwitch);
			if (extender)
				extender->OnOpen(filePath.AsUTF8().c_str());
		}
		if (closingLast) {
			wEditor.SetReadOnly(false);
			ClearDocument();
		}
		if (updateUI) {
			CheckReload();
			RestoreState(bufferNext, false);
			DisplayAround(bufferNext.file);
		}
	}

	if (updateUI && buffers.initialised) {
		BuffersMenu();
		UpdateStatusBar(true);
	}

	if (extender && !closingLast && !makingRoomForNew) {
		extender->OnSwitchFile(filePath.AsUTF8().c_str());
	}

	if (closingLast && props.GetInt("quit.on.close.last") && !loadingSession) {
		QuitProgram();
	}
}

void SciTEBase::CloseTab(int tab) {
	const int tabCurrent = buffers.Current();
	if (tab == tabCurrent) {
		if (SaveIfUnsure() != saveCancelled) {
			Close();
			WindowSetFocus(wEditor);
		}
	} else {
		FilePath fpCurrent = buffers.buffers[tabCurrent].file.AbsolutePath();
		SetDocumentAt(tab);
		if (SaveIfUnsure() != saveCancelled) {
			Close();
			WindowSetFocus(wEditor);
			// Return to the previous buffer
			SetDocumentAt(buffers.GetDocumentByName(fpCurrent));
		}
	}
}

void SciTEBase::CloseAllBuffers(bool loadingSession) {
	if (SaveAllBuffers(false) != saveCancelled) {
		while (buffers.lengthVisible > 1)
			Close(false, loadingSession);

		Close(true, loadingSession);
	}
}

SciTEBase::SaveResult SciTEBase::SaveAllBuffers(bool alwaysYes) {
	SaveResult choice = saveCompleted;
	UpdateBuffersCurrent();
	const int currentBuffer = buffers.Current();
	for (int i = 0; (i < buffers.lengthVisible) && (choice != saveCancelled); i++) {
		if (buffers.buffers[i].isDirty) {
			SetDocumentAt(i);
			if (alwaysYes) {
				if (!Save()) {
					choice = saveCancelled;
				}
			} else {
				choice = SaveIfUnsure(false);
			}
		}
	}
	SetDocumentAt(currentBuffer);
	return choice;
}

void SciTEBase::SaveTitledBuffers() {
	UpdateBuffersCurrent();
	const int currentBuffer = buffers.Current();
	for (int i = 0; i < buffers.lengthVisible; i++) {
		if (buffers.buffers[i].isDirty && !buffers.buffers[i].file.IsUntitled()) {
			SetDocumentAt(i);
			Save();
		}
	}
	SetDocumentAt(currentBuffer);
}

void SciTEBase::Next() {
	int next = buffers.Current();
	if (++next >= buffers.lengthVisible)
		next = 0;
	SetDocumentAt(next);
	CheckReload();
}

void SciTEBase::Prev() {
	int prev = buffers.Current();
	if (--prev < 0)
		prev = buffers.lengthVisible - 1;

	SetDocumentAt(prev);
	CheckReload();
}

void SciTEBase::ShiftTab(int indexFrom, int indexTo) {
	buffers.ShiftTo(indexFrom, indexTo);
	buffers.SetCurrent(indexTo);
	BuffersMenu();

	TabSelect(indexTo);

	DisplayAround(buffers.buffers[buffers.Current()].file);
}

void SciTEBase::MoveTabRight() {
	if (buffers.lengthVisible < 2) return;
	const int indexFrom = buffers.Current();
	int indexTo = indexFrom + 1;
	if (indexTo >= buffers.lengthVisible) indexTo = 0;
	ShiftTab(indexFrom, indexTo);
}

void SciTEBase::MoveTabLeft() {
	if (buffers.lengthVisible < 2) return;
	const int indexFrom = buffers.Current();
	int indexTo = indexFrom - 1;
	if (indexTo < 0) indexTo = buffers.lengthVisible - 1;
	ShiftTab(indexFrom, indexTo);
}

void SciTEBase::NextInStack() {
	SetDocumentAt(buffers.StackNext(), false);
	CheckReload();
}

void SciTEBase::PrevInStack() {
	SetDocumentAt(buffers.StackPrev(), false);
	CheckReload();
}

void SciTEBase::EndStackedTabbing() {
	buffers.CommitStackSelection();
}

static void EscapeFilePathsForMenu(GUI::gui_string &path) {
	// Escape '&' characters in path, since they are interpreted in
	// menus.
	Substitute(path, GUI_TEXT("&"), GUI_TEXT("&&"));
#if defined(GTK)
	GUI::gui_string homeDirectory = getenv("HOME");
	if (StartsWith(path, homeDirectory)) {
		path.replace(static_cast<size_t>(0), homeDirectory.size(), GUI_TEXT("~"));
	}
#endif
}

void SciTEBase::SetBuffersMenu() {
	if (buffers.size() <= 1) {
		DestroyMenuItem(menuBuffers, IDM_BUFFERSEP);
	}
	RemoveAllTabs();

	int pos;
	for (pos = buffers.lengthVisible; pos < bufferMax; pos++) {
		DestroyMenuItem(menuBuffers, IDM_BUFFER + pos);
	}
	if (buffers.size() > 1) {
		constexpr int menuStart = 4;
		SetMenuItem(menuBuffers, menuStart, IDM_BUFFERSEP, GUI_TEXT(""));
		for (pos = 0; pos < buffers.lengthVisible; pos++) {
			const int itemID = bufferCmdID + pos;
			GUI::gui_string entry;
            GUI::gui_string fullPath;
            GUI::gui_string titleTab;

#if defined(_WIN32) || defined(GTK)
			if (pos < 10) {
				GUI::gui_string sPos = GUI::StringFromInteger((pos + 1) % 10);
				GUI::gui_string sHotKey = GUI_TEXT("&") + sPos + GUI_TEXT(" ");
				entry = sHotKey;	// hotkey 1..0
				if (props.GetInt("tabbar.hide.index") == 0) {
#if defined(_WIN32)
					titleTab = sHotKey; // add hotkey to the tabbar
#elif defined(GTK)
					titleTab = sPos + GUI_TEXT(" ");
#endif
				}
			}
#endif

			if (buffers.buffers[pos].file.IsUntitled()) {
				GUI::gui_string untitled = localiser.Text("Untitled");
				entry += untitled;
				titleTab += untitled;
			} else {
				GUI::gui_string path = buffers.buffers[pos].file.AsInternal();
				GUI::gui_string filename = buffers.buffers[pos].file.Name().AsInternal();

				EscapeFilePathsForMenu(path);
#if defined(_WIN32)
				// On Windows, '&' are also interpreted in tab names, so we need
				// the escaped filename
				EscapeFilePathsForMenu(filename);
#endif
				entry += path;
                fullPath = path;
				titleTab += filename;
			}
			// For short file names:
			//char *cpDirEnd = strrchr(buffers.buffers[pos]->fileName, pathSepChar);
			//strcat(entry, cpDirEnd + 1);

			if (buffers.buffers[pos].isReadOnly && props.GetInt("read.only.indicator"))  {
				entry += GUI_TEXT(" |");
				titleTab += GUI_TEXT(" |");
			}

			if (buffers.buffers[pos].isDirty) {
				entry += GUI_TEXT(" *");
				titleTab += GUI_TEXT(" *");
			}

			SetMenuItem(menuBuffers, menuStart + pos + 1, itemID, entry.c_str());
            TabInsert(pos, titleTab.c_str(), fullPath.c_str());
		}
	}
	CheckMenus();
#if !defined(GTK)

	if (tabVisible)
		SizeSubWindows();
#endif
#if defined(GTK)
	ShowTabBar();
#endif
}

void SciTEBase::BuffersMenu() {
	UpdateBuffersCurrent();
	SetBuffersMenu();
}

void SciTEBase::DeleteFileStackMenu() {
	for (int stackPos = 0; stackPos < fileStackMax; stackPos++) {
		DestroyMenuItem(menuFile, fileStackCmdID + stackPos);
	}
	DestroyMenuItem(menuFile, IDM_MRU_SEP);
}

void SciTEBase::SetFileStackMenu() {
	if (recentFileStack[0].IsSet()) {
		SetMenuItem(menuFile, MRU_START, IDM_MRU_SEP, GUI_TEXT(""));
		for (int stackPos = 0; stackPos < fileStackMax; stackPos++) {
			const int itemID = fileStackCmdID + stackPos;
			if (recentFileStack[stackPos].IsSet()) {
				GUI::gui_string sEntry;

#if defined(_WIN32) || defined(GTK)
				GUI::gui_string sPos = GUI::StringFromInteger((stackPos + 1) % 10);
				GUI::gui_string sHotKey = GUI_TEXT("&") + sPos + GUI_TEXT(" ");
				sEntry = sHotKey;
#endif

				GUI::gui_string path = recentFileStack[stackPos].AsInternal();
				EscapeFilePathsForMenu(path);

				sEntry += path;
				SetMenuItem(menuFile, MRU_START + stackPos + 1, itemID, sEntry.c_str());
			}
		}
	}
}

bool SciTEBase::AddFileToBuffer(const BufferState &bufferState) {
	// Return whether file loads successfully
	bool opened = false;
	if (bufferState.file.Exists()) {
		opened = Open(bufferState.file, static_cast<OpenFlags>(ofForceLoad));
		// If forced synchronous should set up position, foldState and bookmarks
		if (opened) {
			const int iBuffer = buffers.GetDocumentByName(bufferState.file, false);
			if (iBuffer >= 0) {
				buffers.buffers[iBuffer].file.scrollPosition = bufferState.file.scrollPosition;
				buffers.buffers[iBuffer].file.selection = bufferState.file.selection;
				buffers.buffers[iBuffer].foldState = bufferState.foldState;
				buffers.buffers[iBuffer].bookmarks = bufferState.bookmarks;
				if (buffers.buffers[iBuffer].lifeState == Buffer::opened) {
					// File was opened synchronously
					RestoreState(buffers.buffers[iBuffer], true);
					DisplayAround(buffers.buffers[iBuffer].file);
					wEditor.ScrollCaret();
				}
			}
		}
	}
	return opened;
}

void SciTEBase::AddFileToStack(const RecentFile &file) {
	if (!file.IsSet())
		return;
	DeleteFileStackMenu();
	// Only stack non-empty names
	if (file.IsSet() && !file.IsUntitled()) {
		int eqPos = fileStackMax - 1;
		for (int stackPos = 0; stackPos < fileStackMax; stackPos++)
			if (recentFileStack[stackPos].SameNameAs(file))
				eqPos = stackPos;
		for (int stackPos = eqPos; stackPos > 0; stackPos--)
			recentFileStack[stackPos] = recentFileStack[stackPos - 1];
		recentFileStack[0] = file;
	}
	SetFileStackMenu();
}

void SciTEBase::RemoveFileFromStack(const FilePath &file) {
	if (!file.IsSet())
		return;
	DeleteFileStackMenu();
	int stackPos;
	for (stackPos = 0; stackPos < fileStackMax; stackPos++) {
		if (recentFileStack[stackPos].SameNameAs(file)) {
			for (int movePos = stackPos; movePos < fileStackMax - 1; movePos++)
				recentFileStack[movePos] = recentFileStack[movePos + 1];
			recentFileStack[fileStackMax - 1].Init();
			break;
		}
	}
	SetFileStackMenu();
}

RecentFile SciTEBase::GetFilePosition() {
	RecentFile rf;
	rf.selection = GetSelectedRange();
	rf.scrollPosition = GetCurrentScrollPosition();
	return rf;
}

void SciTEBase::DisplayAround(const RecentFile &rf) {
	if ((rf.selection.position != SA::InvalidPosition) && (rf.selection.anchor != SA::InvalidPosition)) {
		SetSelection(rf.selection.anchor, rf.selection.position);

		const SA::Line curTop = wEditor.FirstVisibleLine();
		const SA::Line lineTop = wEditor.VisibleFromDocLine(rf.scrollPosition);
		wEditor.LineScroll(0, lineTop - curTop);
		wEditor.ChooseCaretX();
	}
}

// Next and Prev file comments.
// StackMenuNext and StackMenuPrev only used in single buffer mode.
// Decided that "Prev" file should mean the file you had opened last
// This means "Next" file means the file you opened longest ago.

void SciTEBase::StackMenuNext() {
	DeleteFileStackMenu();
	for (int stackPos = fileStackMax - 1; stackPos >= 0; stackPos--) {
		if (recentFileStack[stackPos].IsSet()) {
			SetFileStackMenu();
			StackMenu(stackPos);
			return;
		}
	}
	SetFileStackMenu();
}

void SciTEBase::StackMenuPrev() {
	if (recentFileStack[0].IsSet()) {
		// May need to restore last entry if removed by StackMenu
		RecentFile rfLast = recentFileStack[fileStackMax - 1];
		StackMenu(0);	// Swap current with top of stack
		for (const RecentFile &rf : recentFileStack) {
			if (rfLast.SameNameAs(rf)) {
				rfLast.Init();
			}
		}
		// And rotate the MRU
		RecentFile rfCurrent = recentFileStack[0];
		// Move them up
		for (int stackPos = 0; stackPos < fileStackMax - 1; stackPos++) {
			recentFileStack[stackPos] = recentFileStack[stackPos + 1];
		}
		recentFileStack[fileStackMax - 1].Init();
		// Copy current file into first empty
		for (RecentFile &rf : recentFileStack) {
			if (!rf.IsSet()) {
				if (rfLast.IsSet()) {
					rf = rfLast;
					rfLast.Init();
				} else {
					rf = rfCurrent;
					break;
				}
			}
		}

		DeleteFileStackMenu();
		SetFileStackMenu();
	}
}

void SciTEBase::StackMenu(int pos) {
	if (CanMakeRoom(true)) {
		if (pos >= 0) {
			if ((pos == 0) && (!recentFileStack[pos].IsSet())) {	// Empty
				New();
				SetWindowName();
				ReadProperties();
				SetIndentSettings();
				SetEol();
			} else if (recentFileStack[pos].IsSet()) {
				RecentFile rf = recentFileStack[pos];
				// Already asked user so don't allow Open to ask again.
				Open(rf, ofNoSaveIfDirty);
				CurrentBuffer()->file.scrollPosition = rf.scrollPosition;
				CurrentBuffer()->file.selection = rf.selection;
				DisplayAround(rf);
			}
		}
	}
}

void SciTEBase::RemoveToolsMenu() {
	for (int pos = 0; pos < toolMax; pos++) {
		DestroyMenuItem(menuTools, IDM_TOOLS + pos);
	}
}

void SciTEBase::SetMenuItemLocalised(int menuNumber, int position, int itemID,
				     const char *text, const char *mnemonic) {
	GUI::gui_string localised = localiser.Text(text);
	SetMenuItem(menuNumber, position, itemID, localised.c_str(), GUI::StringFromUTF8(mnemonic).c_str());
}

bool SciTEBase::ToolIsImmediate(int item) {
	std::string itemSuffix = StdStringFromInteger(item);
	itemSuffix += '.';

	std::string propName = "command.";
	propName += itemSuffix;

	std::string command = props.GetWild(propName.c_str(), FileNameExt().AsUTF8().c_str());
	if (command.length()) {
		JobMode jobMode(props, item, FileNameExt().AsUTF8().c_str());
		return jobMode.jobType == jobImmediate;
	}
	return false;
}

void SciTEBase::SetToolsMenu() {
	//command.name.0.*.py=Edit in PythonWin
	//command.0.*.py="c:\program files\python\pythonwin\pythonwin" /edit c:\coloreditor.py
	RemoveToolsMenu();
	int menuPos = TOOLS_START;
	for (int item = 0; item < toolMax; item++) {
		const int itemID = IDM_TOOLS + item;
		std::string prefix = "command.name.";
		prefix += StdStringFromInteger(item);
		prefix += ".";
		std::string commandName = props.GetNewExpandString(prefix.c_str(), FileNameExt().AsUTF8().c_str());
		if (commandName.length()) {
			std::string sMenuItem = commandName;
			prefix = "command.shortcut.";
			prefix += StdStringFromInteger(item);
			prefix += ".";
			std::string sMnemonic = props.GetNewExpandString(prefix.c_str(), FileNameExt().AsUTF8().c_str());
			if (item < 10 && sMnemonic.length() == 0) {
				sMnemonic += "Ctrl+";
				sMnemonic += StdStringFromInteger(item);
			}
			SetMenuItemLocalised(menuTools, menuPos, itemID, sMenuItem.c_str(),
					     sMnemonic.length() ? sMnemonic.c_str() : nullptr);
			menuPos++;
		}
	}

	DestroyMenuItem(menuTools, IDM_MACRO_SEP);
	DestroyMenuItem(menuTools, IDM_MACROLIST);
	DestroyMenuItem(menuTools, IDM_MACROPLAY);
	DestroyMenuItem(menuTools, IDM_MACRORECORD);
	DestroyMenuItem(menuTools, IDM_MACROSTOPRECORD);
	menuPos++;
	if (macrosEnabled) {
		SetMenuItem(menuTools, menuPos++, IDM_MACRO_SEP, GUI_TEXT(""));
		SetMenuItemLocalised(menuTools, menuPos++, IDM_MACROLIST,
				     "&List Macros...", "Shift+F9");
		SetMenuItemLocalised(menuTools, menuPos++, IDM_MACROPLAY,
				     "Run Current &Macro", "F9");
		SetMenuItemLocalised(menuTools, menuPos++, IDM_MACRORECORD,
				     "&Record Macro", "Ctrl+F9");
		SetMenuItemLocalised(menuTools, menuPos, IDM_MACROSTOPRECORD,
				     "S&top Recording Macro", "Ctrl+Shift+F9");
	}
}

JobSubsystem SciTEBase::SubsystemType(const char *cmd) {
	std::string subsystem = props.GetNewExpandString(cmd, FileNameExt().AsUTF8().c_str());
	return subsystem.empty() ? jobCLI : SubsystemFromChar(subsystem.at(0));
}

void SciTEBase::ToolsMenu(int item) {
	SelectionIntoProperties();

	const std::string itemSuffix = StdStringFromInteger(item) + ".";
	const std::string propName = std::string("command.") + itemSuffix;
	std::string command(props.GetWild(propName.c_str(), FileNameExt().AsUTF8().c_str()).c_str());
	if (command.length()) {
		JobMode jobMode(props, item, FileNameExt().AsUTF8().c_str());
		if (jobQueue.IsExecuting() && (jobMode.jobType != jobImmediate))
			// Busy running a tool and running a second can cause failures.
			return;
		if (jobMode.saveBefore == 2 || (jobMode.saveBefore == 1 && (!(CurrentBuffer()->isDirty) || Save())) || SaveIfUnsure() != saveCancelled) {
			if (jobMode.isFilter)
				CurrentBuffer()->fileModTime -= 1;
			if (jobMode.jobType == jobImmediate) {
				if (extender) {
					extender->OnExecute(command.c_str());
				}
			} else {
				AddCommand(command.c_str(), "", jobMode.jobType, jobMode.input, jobMode.flags);
				if (jobQueue.HasCommandToRun())
					Execute();
			}
		}
	}
}

static SA::Line DecodeMessage(const char *cdoc, std::string &sourcePath, int format, SA::Position &column) {
	sourcePath.clear();
	column = -1; // default to not detected
	switch (format) {
	case SCE_ERR_PYTHON: {
			// Python
			const char *startPath = strchr(cdoc, '\"');
			if (startPath) {
				startPath++;
				const char *endPath = strchr(startPath, '\"');
				if (endPath) {
					const ptrdiff_t length = endPath - startPath;
					sourcePath.assign(startPath, length);
					endPath++;
					while (*endPath && !IsADigit(*endPath)) {
						endPath++;
					}
					const SA::Line sourceNumber = IntegerFromText(endPath) - 1;
					return sourceNumber;
				}
			}
			break;
		}
	case SCE_ERR_GCC:
	case SCE_ERR_GCC_INCLUDED_FROM: {
			// GCC - look for number after colon to be line number
			// This will be preceded by file name.
			// Lua debug traceback messages also piggyback this style, but begin with a tab.
			// GCC include paths are similar but start with either "In file included from " or
			// "                 from "
			if (format == SCE_ERR_GCC_INCLUDED_FROM) {
				cdoc += strlen("In file included from ");
			}
			if (cdoc[0] == '\t')
				++cdoc;
			for (int i = 0; cdoc[i]; i++) {
				if (cdoc[i] == ':' && (IsADigit(cdoc[i + 1]) || (cdoc[i + 1] == '-'))) {
					const SA::Line sourceLine = IntegerFromText(cdoc + i + 1);
					sourcePath.assign(cdoc, i);
					i += 2;
					while (IsADigit(cdoc[i]))
						++i;
					if (cdoc[i] == ':' && IsADigit(cdoc[i + 1]))
						column = IntegerFromText(cdoc + i + 1) - 1;
					// Some tools show whole file errors as occurring at line 0
					return (sourceLine > 0) ? sourceLine - 1 : 0;
				}
			}
			break;
		}
	case SCE_ERR_MS: {
			// Visual *
			const char *start = cdoc;
			while (IsASpace(*start)) {
				start++;
			}
			const char *endPath = strchr(start, '(');
			if (endPath) {
				if (!IsADigit(endPath[1])) {
					// This handles the common case of include files in the C:\Program Files (x86)\ directory
					endPath = strchr(endPath + 1, '(');
				}
				if (endPath) {
					const ptrdiff_t length = endPath - start;
					sourcePath.assign(start, length);
					endPath++;
					return IntegerFromText(endPath) - 1;
				}
			}
			break;
		}
	case SCE_ERR_BORLAND: {
			// Borland
			const char *space = strchr(cdoc, ' ');
			if (space) {
				while (IsASpace(*space)) {
					space++;
				}
				while (*space && !IsASpace(*space)) {
					space++;
				}
				while (IsASpace(*space)) {
					space++;
				}

				const char *space2 = nullptr;

				if (strlen(space) > 2) {
					space2 = strchr(space + 2, ':');
				}

				if (space2) {
					while (!IsASpace(*space2)) {
						space2--;
					}

					while (IsASpace(*(space2 - 1))) {
						space2--;
					}

					const ptrdiff_t length = space2 - space;

					if (length > 0) {
						sourcePath.assign(space, length);
						return IntegerFromText(space2) - 1;
					}
				}
			}
			break;
		}
	case SCE_ERR_PERL: {
			// perl
			const char *at = strstr(cdoc, " at ");
			const char *line = strstr(cdoc, " line ");
			if (at && line) {
				const ptrdiff_t length = line - (at + 4);
				if (length > 0) {
					sourcePath.assign(at + 4, length);
					line += 6;
					return IntegerFromText(line) - 1;
				}
			}
			break;
		}
	case SCE_ERR_NET: {
			// .NET traceback
			const char *in = strstr(cdoc, " in ");
			const char *line = strstr(cdoc, ":line ");
			if (in && line && (line > in)) {
				in += 4;
				sourcePath.assign(in, line - in);
				line += 6;
				return IntegerFromText(line) - 1;
			}
			break;
		}
	case SCE_ERR_LUA: {
			// Lua 4 error looks like: last token read: `result' at line 40 in file `Test.lua'
			const char *idLine = "at line ";
			const char *idFile = "file ";
			const size_t lenLine = strlen(idLine);
			const size_t lenFile = strlen(idFile);
			const char *line = strstr(cdoc, idLine);
			const char *file = strstr(cdoc, idFile);
			if (line && file) {
				const char *fileStart = file + lenFile + 1;
				const char *quote = strstr(fileStart, "'");
				if (quote) {
					const size_t length = quote - fileStart;
					if (length > 0) {
						sourcePath.assign(fileStart, length);
					}
				}
				line += lenLine;
				return IntegerFromText(line) - 1;
			} else {
				// Lua 5.1 error looks like: lua.exe: test1.lua:3: syntax error
				// reuse the GCC error parsing code above!
				const char *colon = strstr(cdoc, ": ");
				if (colon)
					return DecodeMessage(colon + 2, sourcePath, SCE_ERR_GCC, column);
			}
			break;
		}

	case SCE_ERR_CTAG: {
			for (SA::Position i = 0; cdoc[i]; i++) {
				if ((IsADigit(cdoc[i + 1]) || (cdoc[i + 1] == '/' && cdoc[i + 2] == '^')) && cdoc[i] == '\t') {
					SA::Position j = i - 1;
					while (j > 0 && ! strchr("\t\n\r \"$%'*,;<>?[]^`{|}", cdoc[j])) {
						j--;
					}
					if (strchr("\t\n\r \"$%'*,;<>?[]^`{|}", cdoc[j])) {
						j++;
					}
					sourcePath.assign(&cdoc[j], i - j);
					// Because usually the address is a searchPattern, lineNumber has to be evaluated later
					return 0;
				}
			}
			break;
		}
	case SCE_ERR_PHP: {
			// PHP error look like: Fatal error: Call to undefined function:  foo() in example.php on line 11
			const char *idLine = " on line ";
			const char *idFile = " in ";
			const size_t lenLine = strlen(idLine);
			const size_t lenFile = strlen(idFile);
			const char *line = strstr(cdoc, idLine);
			const char *file = strstr(cdoc, idFile);
			if (line && file && (line > file)) {
				file += lenFile;
				const size_t length = line - file;
				sourcePath.assign(file, length);
				line += lenLine;
				return IntegerFromText(line) - 1;
			}
			break;
		}

	case SCE_ERR_ELF: {
			// Essential Lahey Fortran error look like: Line 11, file c:\fortran90\codigo\demo.f90
			const char *line = strchr(cdoc, ' ');
			if (line) {
				while (IsASpace(*line)) {
					line++;
				}
				const char *file = strchr(line, ' ');
				if (file) {
					while (IsASpace(*file)) {
						file++;
					}
					while (*file && !IsASpace(*file)) {
						file++;
					}
					const size_t length = strlen(file);
					sourcePath.assign(file, length);
					return IntegerFromText(line) - 1;
				}
			}
			break;
		}

	case SCE_ERR_IFC: {
			/* Intel Fortran Compiler error/warnings look like:
			 * Error 71 at (17:teste.f90) : The program unit has no name
			 * Warning 4 at (9:modteste.f90) : Tab characters are an extension to standard Fortran 95
			 *
			 * Depending on the option, the error/warning messages can also appear on the form:
			 * modteste.f90(9): Warning 4 : Tab characters are an extension to standard Fortran 95
			 *
			 * These are trapped by the MS handler, and are identified OK, so no problem...
			 */
			const char *line = strchr(cdoc, '(');
			if (line) {
				const char *file = strchr(line, ':');
				if (file) {
					file++;
					const char *endfile = strchr(file, ')');
					const size_t length = endfile - file;
					sourcePath.assign(file, length);
					line++;
					return IntegerFromText(line) - 1;
				}
			}
			break;
		}

	case SCE_ERR_ABSF: {
			// Absoft Pro Fortran 90/95 v8.x, v9.x  errors look like: cf90-113 f90fe: ERROR SHF3D, File = shf.f90, Line = 1101, Column = 19
			const char *idFile = " File = ";
			const char *idLine = ", Line = ";
			const size_t lenFile = strlen(idFile);
			const size_t lenLine = strlen(idLine);
			const char *file = strstr(cdoc, idFile);
			const char *line = strstr(cdoc, idLine);
			//const char *idColumn = ", Column = ";
			//const char *column = strstr(cdoc, idColumn);
			if (line && file && (line > file)) {
				file += lenFile;
				const size_t length = line - file;
				sourcePath.assign(file, length);
				line += lenLine;
				return IntegerFromText(line) - 1;
			}
			break;
		}

	case SCE_ERR_IFORT: {
			/* Intel Fortran Compiler v8.x error/warnings look like:
			 * fortcom: Error: shf.f90, line 5602: This name does not have ...
				 */
			const char *idFile = ": Error: ";
			const char *idLine = ", line ";
			const size_t lenFile = strlen(idFile);
			const size_t lenLine = strlen(idLine);
			const char *file = strstr(cdoc, idFile);
			const char *line = strstr(cdoc, idLine);
			const char *lineend = strrchr(cdoc, ':');
			if (line && file && (line > file)) {
				file += lenFile;
				const size_t length = line - file;
				sourcePath.assign(file, length);
				line += lenLine;
				if ((lineend > line)) {
					return IntegerFromText(line) - 1;
				}
			}
			break;
		}

	case SCE_ERR_TIDY: {
			/* HTML Tidy error/warnings look like:
			 * line 8 column 1 - Error: unexpected </head> in <meta>
			 * line 41 column 1 - Warning: <table> lacks "summary" attribute
			 */
			const char *line = strchr(cdoc, ' ');
			if (line) {
				const char *col = strchr(line + 1, ' ');
				if (col) {
					//*col = '\0';
					const SA::Line lnr = IntegerFromText(line) - 1;
					col = strchr(col + 1, ' ');
					if (col) {
						const char *endcol = strchr(col + 1, ' ');
						if (endcol) {
							//*endcol = '\0';
							column = IntegerFromText(col) - 1;
							return lnr;
						}
					}
				}
			}
			break;
		}

	case SCE_ERR_JAVA_STACK: {
			/* Java runtime stack trace
				\tat <methodname>(<filename>:<line>)
				 */
			const char *startPath = strrchr(cdoc, '(') + 1;
			const char *endPath = strchr(startPath, ':');
			const ptrdiff_t length = endPath - startPath;
			if (length > 0) {
				sourcePath.assign(startPath, length);
				const SA::Line sourceNumber = IntegerFromText(endPath + 1) - 1;
				return sourceNumber;
			}
			break;
		}

	case SCE_ERR_DIFF_MESSAGE: {
			// Diff file header, either +++ <filename> or --- <filename>, may be followed by \t
			// Often followed by a position line @@ <linenumber>
			const char *startPath = cdoc + 4;
			const char *endPath = strpbrk(startPath, "\t\r\n");
			if (endPath) {
				const ptrdiff_t length = endPath - startPath;
				sourcePath.assign(startPath, length);
				return 0;
			}
			break;
		}
	}	// switch
	return -1;
}

#define CSI "\033["

static bool SeqEnd(int ch) noexcept {
	return (ch == 0) || ((ch >= '@') && (ch <= '~'));
}

static void RemoveEscSeq(std::string &s) {
	size_t csi = s.find(CSI);
	while (csi != std::string::npos) {
		size_t endSeq = csi + 2;
		while (endSeq < s.length() && !SeqEnd(s.at(endSeq)))
			endSeq++;
		s.erase(csi, endSeq-csi+1);
		csi = s.find(CSI);
	}
}

// Remove up to and including ch
static void Chomp(std::string &s, char ch) {
	const size_t posCh = s.find(ch);
	if (posCh != std::string::npos)
		s.erase(0, posCh + 1);
}

namespace {

char Severity(const std::string &message) noexcept {
	if (message.find("fatal") != std::string::npos)
		return 3;
	if (message.find("error") != std::string::npos)
		return 2;
	if (message.find("warning") != std::string::npos)
		return 1;
	return 0;
}

}

void SciTEBase::ShowMessages(SA::Line line) {
	wEditor.AnnotationSetStyleOffset(diagnosticStyleStart);
	wEditor.AnnotationSetVisible(SA::AnnotationVisible::Boxed);
	wEditor.AnnotationClearAll();
	TextReader acc(wOutput);
	while ((line > 0) && (acc.StyleAt(acc.LineStart(line-1)) != SCE_ERR_CMD))
		line--;
	const SA::Line maxLine = wOutput.LineCount();
	while ((line < maxLine) && (acc.StyleAt(acc.LineStart(line)) != SCE_ERR_CMD)) {
		const SA::Position startPosLine = wOutput.LineStart(line);
		const SA::Position lineEnd = wOutput.LineEnd(line);
		std::string message = wOutput.StringOfRange(SA::Range(startPosLine, lineEnd));
		std::string source;
		SA::Position column = 0;
		int style = acc.StyleAt(startPosLine);
		if ((style == SCE_ERR_ESCSEQ) || (style == SCE_ERR_ESCSEQ_UNKNOWN) || (style >= SCE_ERR_ES_BLACK)) {
			// GCC message with ANSI escape sequences
			RemoveEscSeq(message);
			style = SCE_ERR_GCC;
		}
		const SA::Line sourceLine = DecodeMessage(message.c_str(), source, style, column);
		Chomp(message, ':');
		if (style == SCE_ERR_GCC) {
			Chomp(message, ':');
		}
		GUI::gui_string sourceString = GUI::StringFromUTF8(source);
		FilePath sourcePath = FilePath(sourceString).NormalizePath();
		if (filePath.Name().SameNameAs(sourcePath.Name())) {
			if (style == SCE_ERR_GCC) {
				const char *sColon = strchr(message.c_str(), ':');
				if (sColon) {
					std::string editLine = GetLine(wEditor, sourceLine);
					if (editLine == (sColon+1)) {
						line++;
						continue;
					}
				}
			}
			std::string msgCurrent = wEditor.AnnotationGetText(sourceLine);
			if (msgCurrent.find(message) == std::string::npos) {
				// Only append unique messages
				std::string stylesCurrent = wEditor.AnnotationGetStyles(sourceLine);
				if (msgCurrent.length()) {
					msgCurrent += "\n";
					stylesCurrent += '\0';
				}
				msgCurrent += message;
				const char msgStyle = Severity(message);
				stylesCurrent += std::string(message.length(), msgStyle);
				wEditor.AnnotationSetText(sourceLine, msgCurrent.c_str());
				wEditor.AnnotationSetStyles(sourceLine, stylesCurrent.c_str());
			}
		}
		line++;
	}
}

void SciTEBase::GoMessage(int dir) {
	const SA::Position selStart = wOutput.SelectionStart();
	const SA::Line curLine = wOutput.LineFromPosition(selStart);
	const SA::Line maxLine = wOutput.LineCount();
	SA::Line lookLine = curLine + dir;
	if (lookLine < 0)
		lookLine = maxLine - 1;
	else if (lookLine >= maxLine)
		lookLine = 0;
	TextReader acc(wOutput);
	while ((dir == 0) || (lookLine != curLine)) {
		const SA::Position startPosLine = wOutput.LineStart(lookLine);
		const SA::Position lineLength = wOutput.LineLength(lookLine);
		int style = acc.StyleAt(startPosLine);
		if (style != SCE_ERR_DEFAULT &&
				style != SCE_ERR_CMD &&
				style != SCE_ERR_DIFF_ADDITION &&
				style != SCE_ERR_DIFF_CHANGED &&
				style != SCE_ERR_DIFF_DELETION) {
			wOutput.MarkerDeleteAll(-1);
			wOutput.MarkerDefine(0, SA::MarkerSymbol::SmallRect);
			wOutput.MarkerSetFore(0, ColourOfProperty(props,
					      "error.marker.fore", ColourRGB(0x7f, 0, 0)));
			wOutput.MarkerSetBack(0, ColourOfProperty(props,
					      "error.marker.back", ColourRGB(0xff, 0xff, 0)));
			wOutput.MarkerAdd(lookLine, 0);
			wOutput.SetSel(startPosLine, startPosLine);
			std::string message = wOutput.StringOfRange(SA::Range(startPosLine, startPosLine + lineLength));
			if ((style == SCE_ERR_ESCSEQ) || (style == SCE_ERR_ESCSEQ_UNKNOWN) || (style >= SCE_ERR_ES_BLACK)) {
				// GCC message with ANSI escape sequences
				RemoveEscSeq(message);
				style = SCE_ERR_GCC;
			}
			std::string source;
			SA::Position column;
			SA::Line sourceLine = DecodeMessage(message.c_str(), source, style, column);
			if (sourceLine >= 0) {
				GUI::gui_string sourceString = GUI::StringFromUTF8(source);
				FilePath sourcePath = FilePath(sourceString).NormalizePath();
				if (!filePath.Name().SameNameAs(sourcePath)) {
					FilePath messagePath;
					bool bExists = false;
					if (Exists(dirNameAtExecute.AsInternal(), sourceString.c_str(), &messagePath)) {
						bExists = true;
					} else if (Exists(dirNameForExecute.AsInternal(), sourceString.c_str(), &messagePath)) {
						bExists = true;
					} else if (Exists(filePath.Directory().AsInternal(), sourceString.c_str(), &messagePath)) {
						bExists = true;
					} else if (Exists(nullptr, sourceString.c_str(), &messagePath)) {
						bExists = true;
					} else {
						// Look through buffers for name match
						for (int i = buffers.lengthVisible - 1; i >= 0; i--) {
							if (sourcePath.Name().SameNameAs(buffers.buffers[i].file.Name())) {
								messagePath = buffers.buffers[i].file;
								bExists = true;
							}
						}
					}
					if (bExists) {
						if (!Open(messagePath, ofSynchronous)) {
							return;
						}
						CheckReload();
					}
				}

				// If ctag then get line number after search tag or use ctag line number
				if (style == SCE_ERR_CTAG) {
					//without following focus GetCTag wouldn't work correct
					WindowSetFocus(wOutput);
					std::string cTag = GetCTag();
					if (cTag.length() != 0) {
						if (atoi(cTag.c_str()) > 0) {
							//if tag is linenumber, get line
							sourceLine = IntegerFromText(cTag.c_str()) - 1;
						} else {
							findWhat = cTag;
							FindNext(false);
							//get linenumber for marker from found position
							sourceLine = wEditor.LineFromPosition(wEditor.CurrentPos());
						}
					}
				}

				else if (style == SCE_ERR_DIFF_MESSAGE) {
					const bool isAdd = message.find("+++ ") == 0;
					const SA::Line atLine = lookLine + (isAdd ? 1 : 2); // lines are in this order: ---, +++, @@
					std::string atMessage = GetLine(wOutput, atLine);
					if (StartsWith(atMessage, "@@ -")) {
						size_t atPos = 4; // deleted position starts right after "@@ -"
						if (isAdd) {
							const size_t linePlace = atMessage.find(" +", 7);
							if (linePlace != std::string::npos)
								atPos = linePlace + 2; // skip "@@ -1,1" and then " +"
						}
						sourceLine = IntegerFromText(atMessage.c_str() + atPos) - 1;
					}
				}

				if (props.GetInt("error.inline")) {
					ShowMessages(lookLine);
				}

				wEditor.MarkerDeleteAll(0);
				wEditor.MarkerDefine(0, SA::MarkerSymbol::Circle);
				wEditor.MarkerSetFore(0, ColourOfProperty(props,
						      "error.marker.fore", ColourRGB(0x7f, 0, 0)));
				wEditor.MarkerSetBack(0, ColourOfProperty(props,
						      "error.marker.back", ColourRGB(0xff, 0xff, 0)));
				wEditor.MarkerAdd(sourceLine, 0);
				SA::Position startSourceLine = wEditor.LineStart(sourceLine);
				const SA::Position endSourceline = wEditor.LineStart(sourceLine + 1);
				if (column >= 0) {
					// Get the position in line according to current tab setting
					startSourceLine = wEditor.FindColumn(sourceLine, column);
				}
				EnsureRangeVisible(wEditor, SA::Range(startSourceLine));
				if (props.GetInt("error.select.line") == 1) {
					//select whole source source line from column with error
					SetSelection(endSourceline, startSourceLine);
				} else {
					//simply move cursor to line, don't do any selection
					SetSelection(startSourceLine, startSourceLine);
				}
				std::replace(message.begin(), message.end(), '\t', ' ');
				::Remove(message, std::string("\n"));
				props.Set("CurrentMessage", message.c_str());
				UpdateStatusBar(false);
				WindowSetFocus(wEditor);
			}
			return;
		}
		lookLine += dir;
		if (lookLine < 0)
			lookLine = maxLine - 1;
		else if (lookLine >= maxLine)
			lookLine = 0;
		if (dir == 0)
			return;
	}
}

