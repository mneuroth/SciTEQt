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

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
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
#include "Searcher.h"
#include "SciTEBase.h"

const GUI::gui_char defaultSessionFileName[] = GUI_TEXT("SciTE.session");

void BufferDocReleaser::operator()(void *pDoc) noexcept {
	if (pDoc) {
		try {
			pSci->ReleaseDocument(pDoc);
		} catch (...) {
			// ReleaseDocument must not throw, ignore if it does.
		}
	}
}

Buffer::Buffer() :
	file(), isDirty(false), isReadOnly(false), failedSave(false), useMonoFont(false), lifeState(LifeState::empty),
	unicodeMode(UniMode::uni8Bit), fileModTime(0), fileModLastAsk(0), documentModTime(0),
	findMarks(FindMarks::none), futureDo(FutureDo::none) {}

void Buffer::Init() {
	file.Init();
	isDirty = false;
	isReadOnly = false;
	failedSave = false;
	useMonoFont = false;
	lifeState = LifeState::empty;
	unicodeMode = UniMode::uni8Bit;
	fileModTime = 0;
	fileModLastAsk = 0;
	documentModTime = 0;
	findMarks = FindMarks::none;
	overrideExtension = "";
	foldState.clear();
	bookmarks.clear();
	pFileWorker.reset();
	futureDo = FutureDo::none;
	doc.reset();
}

void Buffer::SetTimeFromFile() {
	fileModTime = file.ModifiedTime();
	fileModLastAsk = fileModTime;
	documentModTime = fileModTime;
	failedSave = false;
}

void Buffer::DocumentModified() noexcept {
	documentModTime = time(nullptr);
}

bool Buffer::NeedsSave(int delayBeforeSave) const  noexcept {
	const time_t now = time(nullptr);
	return now && documentModTime && isDirty && !pFileWorker && (now-documentModTime > delayBeforeSave) && !file.IsUntitled() && !failedSave;
}

void Buffer::CompleteLoading() noexcept {
	lifeState = LifeState::opened;
	if (pFileWorker && pFileWorker->IsLoading()) {
		pFileWorker.reset();
	}
}

void Buffer::CompleteStoring() {
	if (pFileWorker && !pFileWorker->IsLoading()) {
		pFileWorker.reset();
	}
	SetTimeFromFile();
}

void Buffer::AbandonAutomaticSave() {
	if (pFileWorker && !pFileWorker->IsLoading()) {
		const FileStorer *pFileStorer = dynamic_cast<FileStorer *>(pFileWorker.get());
		if (pFileStorer && !pFileStorer->visibleProgress) {
			pFileWorker->Cancel();
			// File is in partially saved state so may be better to remove
		}
	}
}

constexpr Buffer::FutureDo operator&(Buffer::FutureDo a, Buffer::FutureDo b) noexcept {
	return static_cast<Buffer::FutureDo>(static_cast<int>(a) & static_cast<int>(b));
}

constexpr Buffer::FutureDo operator|(Buffer::FutureDo a, Buffer::FutureDo b) noexcept {
	return static_cast<Buffer::FutureDo>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr Buffer::FutureDo operator~(Buffer::FutureDo a) noexcept {
	return static_cast<Buffer::FutureDo>(~static_cast<int>(a));
}

void Buffer::ScheduleFinishSave() noexcept {
	isDirty = false;
	failedSave = false;
	// Need to make writable and set save point when next receive focus.
	futureDo = futureDo | FutureDo::finishSave;
}

bool Buffer::FinishSave() noexcept {
	if ((futureDo & FutureDo::finishSave) != FutureDo::finishSave) {
		return false;
	}
	futureDo = futureDo & ~(FutureDo::finishSave);
	return true;
}

void Buffer::CancelLoad() {
	// Complete any background loading
	if (pFileWorker && pFileWorker->IsLoading()) {
		pFileWorker->Cancel();
		CompleteLoading();
		lifeState = LifeState::empty;
	}
}

BufferList::BufferList() : current(0), stackcurrent(0), stack(0), buffers(0), length(0), lengthVisible(0), initialised(false) {}

BufferIndex BufferList::size() const noexcept {
	return static_cast<BufferIndex>(buffers.size());
}

void BufferList::Allocate(BufferIndex maxSize) {
	length = 1;
	lengthVisible = 1;
	current = 0;
	buffers.resize(maxSize);
	stack.resize(maxSize);
	stack[0] = 0;
}

BufferIndex BufferList::Add() {
	if (length < size()) {
		length++;
	}
	buffers[length - 1].Init();
	stack[length - 1] = length - 1;
	MoveToStackTop(length - 1);
	SetVisible(length-1, true);

	return lengthVisible - 1;
}

BufferIndex BufferList::GetDocumentByWorker(const FileWorker *pFileWorker) const noexcept {
	for (int i = 0; i < length; i++) {
		if (buffers[i].pFileWorker.get() == pFileWorker) {
			return i;
		}
	}
	return bufferInvalid;
}

BufferIndex BufferList::GetDocumentByName(const FilePath &filename, bool excludeCurrent) const noexcept {
	if (!filename.IsSet()) {
		return bufferInvalid;
	}
	for (BufferIndex i = 0; i < length; i++) {
		if ((!excludeCurrent || i != current) && buffers[i].file.SameNameAs(filename)) {
			return i;
		}
	}
	return bufferInvalid;
}

void BufferList::RemoveInvisible(BufferIndex index) {
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
	// Delete and move up to fill gap.
	buffers[current].CompleteLoading();
	for (BufferIndex i = current; i < length - 1; i++) {
		buffers[i] = std::move(buffers[i + 1]);
	}

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

BufferIndex BufferList::Current() const noexcept {
	return current;
}

Buffer *BufferList::CurrentBuffer() noexcept {
	return &buffers[Current()];
}

const Buffer *BufferList::CurrentBufferConst() const noexcept {
	return &buffers[Current()];
}

void BufferList::SetCurrent(BufferIndex index) noexcept {
	current = index;
}

void BufferList::PopStack() {
	for (BufferIndex i = 0; i < length - 1; ++i) {
		BufferIndex index = stack[i + 1];
		// adjust the index for items that will move in buffers[]
		if (index > current)
			--index;
		stack[i] = index;
	}
}

BufferIndex BufferList::StackNext() {
	if (++stackcurrent >= length)
		stackcurrent = 0;
	return stack[stackcurrent];
}

BufferIndex BufferList::StackPrev() {
	if (--stackcurrent < 0)
		stackcurrent = length - 1;
	return stack[stackcurrent];
}

void BufferList::MoveToStackTop(BufferIndex index) {
	// shift top chunk of stack down into the slot that index occupies
	bool move = false;
	for (BufferIndex i = length - 1; i > 0; --i) {
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

void BufferList::ShiftTo(BufferIndex indexFrom, BufferIndex indexTo) {
	// shift buffer to new place in buffers array
	if (indexFrom == indexTo ||
			indexFrom < 0 || indexFrom >= length ||
			indexTo < 0 || indexTo >= length) return;
	int step = (indexFrom > indexTo) ? -1 : 1;
	Buffer tmp = std::move(buffers[indexFrom]);
	for (BufferIndex i = indexFrom; i != indexTo; i += step) {
		buffers[i] = std::move(buffers[i+step]);
	}
	buffers[indexTo] = std::move(tmp);
	// update stack indexes
	for (BufferIndex i = 0; i < length; i++) {
		if (stack[i] == indexFrom) {
			stack[i] = indexTo;
		} else if (step == 1) {
			if (indexFrom < stack[i] && stack[i] <= indexTo) stack[i] -= step;
		} else {
			if (indexFrom > stack[i] && stack[i] >= indexTo) stack[i] -= step;
		}
	}
}

void BufferList::Swap(BufferIndex indexA, BufferIndex indexB) {
	// shift buffer to new place in buffers array
	if (indexA == indexB ||
			indexA < 0 || indexA >= length ||
			indexB < 0 || indexB >= length) return;
	std::swap(buffers[indexA], buffers[indexB]);
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
					const FileStorer *fstorer = dynamic_cast<FileStorer *>(buffers[i].pFileWorker.get());
					if (fstorer && !fstorer->visibleProgress)
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

bool BufferList::GetVisible(BufferIndex index) const noexcept {
	return index < lengthVisible;
}

void BufferList::SetVisible(BufferIndex index, bool visible) {
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

void *SciTEBase::GetDocumentAt(BufferIndex index) {
	if (index < 0 || index >= buffers.size()) {
		return nullptr;
	}
	if (!buffers.buffers[index].doc) {
		// Create a new document buffer
		buffers.buffers[index].doc = BufferDoc(wEditor.CreateDocument(0, SA::DocumentOption::Default), docReleaser);
	}
	return buffers.buffers[index].doc.get();
}

void SciTEBase::SwitchDocumentAt(BufferIndex index, void *pdoc) {
	if (index < 0 || index >= buffers.size()) {
		return;
	}
	buffers.buffers[index].doc = BufferDoc(pdoc, docReleaser);
	if (index == buffers.Current()) {
		wEditor.SetDocPointer(buffers.buffers[index].doc.get());
	}
}

void SciTEBase::SetDocumentAt(BufferIndex index, bool updateStack) {
	const BufferIndex currentbuf = buffers.Current();

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
	const bool restoreBookmarks = bufferNext.lifeState == Buffer::LifeState::readAll;
	PerformDeferredTasks();
	if (bufferNext.lifeState == Buffer::LifeState::readAll) {
		CompleteOpen(OpenCompletion::completeSwitch);
		if (extender)
			extender->OnOpen(filePath.AsUTF8().c_str());
	}
	RestoreState(bufferNext, restoreBookmarks);

	TabSelect(index);

	if (lineNumbers && lineNumbersExpand)
		SetLineNumberWidth();

	DisplayAround(bufferNext.file.filePosition);
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

void SciTEBase::SaveFolds(std::vector<SA::Line> &folds) {
	folds.clear();

	for (SA::Line line = 0; ; line++) {
		const SA::Line lineNext = wEditor.ContractedFoldNext(line);
		if ((line < 0) || (lineNext < line))
			break;
		line = lineNext;
		folds.push_back(line);
	}
}
void SciTEBase::RestoreFolds(const std::vector<SA::Line> &folds) {
	for (const SA::Line fold : folds) {
		wEditor.ToggleFold(fold);
	}
}

void SciTEBase::UpdateBuffersCurrent() {
	const BufferIndex currentbuf = buffers.Current();

	if ((buffers.length > 0) && (currentbuf >= 0) && (buffers.GetVisible(currentbuf))) {
		Buffer &bufferCurrent = buffers.buffers[currentbuf];
		bufferCurrent.file.Set(filePath);
		if (bufferCurrent.lifeState != Buffer::LifeState::reading && bufferCurrent.lifeState != Buffer::LifeState::readAll) {
			bufferCurrent.file.filePosition = GetFilePosition();

			// Retrieve fold state and store in buffer state info

			if (!FilterShowing() && props.GetInt("fold")) {
				SaveFolds(bufferCurrent.foldState);
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
		if (SaveIfUnsure(true, static_cast<SaveFlags>(sfProgressVisible | sfSynchronous)) != SaveResult::cancelled) {
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
		buffers.buffers[0].doc = BufferDoc(wEditor.DocPointer(), docReleaser);
		wEditor.AddRefDocument(buffers.buffers[0].doc.get()); // We own this reference
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

static std::string IndexPropKey(const char *bufPrefix, BufferIndex bufIndex, const char *bufAppendix) {
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

	const FilePath sessionFilePath = FilePath(sessionPathName).AbsolutePath();
	// Add/update SessionPath environment variable
	props.SetPath("SessionPath", sessionFilePath);
}

void SciTEBase::RestoreRecentMenu() {
	const FilePosition fp(SelectedRange(0, 0), 0);

	DeleteFileStackMenu();

	for (int i = 0; i < fileStackMax; i++) {
		std::string propKey = IndexPropKey("mru", i, "path");
		std::string propStr = propsSession.GetString(propKey.c_str());
		if (propStr == "")
			continue;
		AddFileToStack(RecentFile(GUI::StringFromUTF8(propStr), fp));
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
	const BufferIndex iBuffer = buffers.GetDocumentByName(session.pathActive);
	if (iBuffer >= 0)
		SetDocumentAt(iBuffer);
}

void SciTEBase::RestoreSession() {
	if (props.GetInt("save.find") != 0) {
		for (int i = 0;; i++) {
			const std::string propKey = IndexPropKey("search", i, "findwhat");
			const std::string propStr = propsSession.GetString(propKey.c_str());
			if (propStr == "")
				break;
			memFinds.Append(propStr);
		}

		for (int i = 0;; i++) {
			const std::string propKey = IndexPropKey("search", i, "replacewith");
			const std::string propStr = propsSession.GetString(propKey.c_str());
			if (propStr == "")
				break;
			memReplaces.Append(propStr);
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

		propKey = IndexPropKey("buffer", i, "position");
		const SA::Position pos = propsSession.GetInteger(propKey.c_str()) - 1;	// -1 for 1 -> 0 based

		bufferState.file.filePosition = FilePosition(SelectedRange(pos, pos), scroll);

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
			it = mem.begin();
			for (int i = 0; it != mem.end(); i++, ++it) {
				propKey = IndexPropKey("search", i, "replacewith");
				fprintf(sessionFile, "%s=%s\n", propKey.c_str(), (*it).c_str());
			}
		}
	}

	if (props.GetInt("buffers") && (!defaultSession || props.GetInt("save.session"))) {
		const BufferIndex curr = buffers.Current();
		for (BufferIndex i = 0; i < buffers.lengthVisible; i++) {
			const Buffer &buff = buffers.buffers[i];
			if (buff.file.IsSet() && !buff.file.IsUntitled()) {
				std::string propKey = IndexPropKey("buffer", i, "path");
				fprintf(sessionFile, "\n%s=%s\n", propKey.c_str(), buff.file.AsUTF8().c_str());

				const SA::Position pos = buff.file.filePosition.selection.position + 1;	// +1 = 0 to 1 based
				const std::string sPos = std::to_string(pos);
				propKey = IndexPropKey("buffer", i, "position");
				fprintf(sessionFile, "%s=%s\n", propKey.c_str(), sPos.c_str());

				const SA::Line scroll = buff.file.filePosition.scrollPosition;
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

	const FilePath sessionFilePath = FilePath(sessionPathName).AbsolutePath();
	// Add/update SessionPath environment variable
	props.SetPath("SessionPath", sessionFilePath);
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
	CurrentBuffer()->lifeState = Buffer::LifeState::opened;
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
	if (CurrentBuffer()->unicodeMode != UniMode::uni8Bit) {
		// Override the code page if Unicode
		codePage = SA::CpUtf8;
		wEditor.SetCodePage(codePage);
	}

	RemoveFindMarks();
	// check to see whether there is saved fold state, restore
	if (!buffer.foldState.empty() && !FilterShowing()) {
		wEditor.ColouriseAll();
		RestoreFolds(buffer.foldState);
	}
	if (restoreBookmarks) {
		for (const SA::Line bookmark : buffer.bookmarks) {
			wEditor.MarkerAdd(bookmark, markerBookmark);
		}
	}
	if (FilterShowing()) {
		FilterAll(true);
	}
}

void SciTEBase::Close(bool updateUI, bool loadingSession, bool makingRoomForNew) {
	bool closingLast = true;
	const BufferIndex index = buffers.Current();
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
			buffers.buffers[0].lifeState = Buffer::LifeState::opened;
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
		if (bufferNext.lifeState == Buffer::LifeState::readAll) {
			//restoreBookmarks = true;
			CompleteOpen(OpenCompletion::completeSwitch);
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
			DisplayAround(bufferNext.file.filePosition);
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
	const BufferIndex tabCurrent = buffers.Current();
	if (tab == tabCurrent) {
		if (SaveIfUnsure() != SaveResult::cancelled) {
			Close();
			WindowSetFocus(wEditor);
		}
	} else {
		FilePath fpCurrent = buffers.buffers[tabCurrent].file.AbsolutePath();
		SetDocumentAt(tab);
		if (SaveIfUnsure() != SaveResult::cancelled) {
			Close();
			WindowSetFocus(wEditor);
			// Return to the previous buffer
			SetDocumentAt(buffers.GetDocumentByName(fpCurrent));
		}
	}
}

void SciTEBase::CloseAllBuffers(bool loadingSession) {
	if (SaveAllBuffers(false) != SaveResult::cancelled) {
		while (buffers.lengthVisible > 1)
			Close(false, loadingSession);

		Close(true, loadingSession);
	}
}

SciTEBase::SaveResult SciTEBase::SaveAllBuffers(bool alwaysYes) {
	SaveResult choice = SaveResult::completed;
	UpdateBuffersCurrent();
	const BufferIndex currentBuffer = buffers.Current();
	for (BufferIndex i = 0; (i < buffers.lengthVisible) && (choice != SaveResult::cancelled); i++) {
		if (buffers.buffers[i].isDirty) {
			SetDocumentAt(i);
			if (alwaysYes) {
				if (!Save()) {
					choice = SaveResult::cancelled;
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
	const BufferIndex currentBuffer = buffers.Current();
	for (BufferIndex i = 0; i < buffers.lengthVisible; i++) {
		if (buffers.buffers[i].isDirty && !buffers.buffers[i].file.IsUntitled()) {
			SetDocumentAt(i);
			Save();
		}
	}
	SetDocumentAt(currentBuffer);
}

void SciTEBase::Next() {
	BufferIndex next = buffers.Current();
	if (++next >= buffers.lengthVisible)
		next = 0;
	SetDocumentAt(next);
	CheckReload();
}

void SciTEBase::Prev() {
	BufferIndex prev = buffers.Current();
	if (--prev < 0)
		prev = buffers.lengthVisible - 1;

	SetDocumentAt(prev);
	CheckReload();
}

void SciTEBase::ShiftTab(BufferIndex indexFrom, BufferIndex indexTo) {
	buffers.ShiftTo(indexFrom, indexTo);
	buffers.SetCurrent(indexTo);
	BuffersMenu();

	TabSelect(indexTo);

	DisplayAround(buffers.buffers[buffers.Current()].file.filePosition);
}

void SciTEBase::MoveTabRight() {
	if (buffers.lengthVisible < 2) return;
	const BufferIndex indexFrom = buffers.Current();
	BufferIndex indexTo = indexFrom + 1;
	if (indexTo >= buffers.lengthVisible) indexTo = 0;
	ShiftTab(indexFrom, indexTo);
}

void SciTEBase::MoveTabLeft() {
	if (buffers.lengthVisible < 2) return;
	const BufferIndex indexFrom = buffers.Current();
	BufferIndex indexTo = indexFrom - 1;
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

void SciTEBase::UpdateTabs(const std::vector<GUI::gui_string> &tabNames) {
	RemoveAllTabs();
	for (int t = 0; t < static_cast<int>(tabNames.size()); t++) {
        const GUI::gui_char empty[] = GUI_TEXT("");
        TabInsert(t, tabNames[t].c_str(), /*TODO SciteQt PATCH fullPath.c_str()*/empty);
	}
}

namespace {

GUI::gui_string EscapeFilePath(const FilePath &path, [[maybe_unused]]Title destination) {
	// Escape '&' characters in path, since they are interpreted in menus.
	GUI::gui_string escaped(path.AsInternal());
#if defined(_WIN32)
	// On Windows, '&' are interpreted in menus and tab names, so we need
	// the escaped filename
	Substitute(escaped, GUI_TEXT("&"), GUI_TEXT("&&"));
#else
	if (destination == Title::menu) {
		Substitute(escaped, GUI_TEXT("&"), GUI_TEXT("&&"));
	}
#endif
	return escaped;
}

GUI::gui_string AbbreviateWithTilde(const GUI::gui_string &path) {
#if defined(GTK) || defined(__APPLE__)
	FilePath homePath = FilePath::UserHomeDirectory();
	if (homePath.IsSet()) {
		const GUI::gui_string_view homeDirectory = homePath.AsInternal();
		if (StartsWith(path, homeDirectory)) {
			return GUI::gui_string(GUI_TEXT("~")) + path.substr(homeDirectory.size());
		}
	}
#endif
	return path;
}

// Produce a menu or tab title from a buffer.
// <index> <file name> <is read only> <is dirty>
// 3 /src/example.cxx | *
GUI::gui_string BufferTitle([[maybe_unused]] int pos, const Buffer &buffer, Title destination,
	PropSetFile const &props, const Localization &localiser) {
	GUI::gui_string title;

	// Index
#if defined(_WIN32) || defined(GTK)
	if (pos < 10) {
		const GUI::gui_string sPos = GUI::StringFromInteger((pos + 1) % 10);
		const GUI::gui_string sHotKey = GUI_TEXT("&") + sPos + GUI_TEXT(" ");
		if (destination == Title::menu) {
			title = sHotKey;	// hotkey 1..0
		} else {
			if (props.GetInt("tabbar.hide.index") == 0) {
#if defined(_WIN32)
				title = sHotKey; // add hotkey to the tabbar
#elif defined(GTK)
				title = sPos + GUI_TEXT(" ");	// just the index
#endif
			}
		}
	}
#endif

	// File name or path
	if (buffer.file.IsUntitled()) {
		title += localiser.Text("Untitled");
	} else {
		if (destination == Title::menu) {
			title += AbbreviateWithTilde(EscapeFilePath(buffer.file, destination));
		} else {
			title += EscapeFilePath(buffer.file.Name(), destination);
		}
	}

	// Read only indicator
	if (buffer.isReadOnly && props.GetInt("read.only.indicator")) {
		title += GUI_TEXT(" |");
	}

	// Dirty indicator
	if (buffer.isDirty) {
		title += GUI_TEXT(" *");
	}

	return title;
}

}

void SciTEBase::SetBuffersMenu() {
	if (buffers.size() <= 1) {
		DestroyMenuItem(menuBuffers, IDM_BUFFERSEP);
	}

	std::vector<GUI::gui_string> tabNames;

	for (BufferIndex pos = buffers.lengthVisible; pos < bufferMax; pos++) {
		DestroyMenuItem(menuBuffers, IDM_BUFFER + pos);
	}
	if (buffers.size() > 1) {
		constexpr int menuStart = 4;
		SetMenuItem(menuBuffers, menuStart, IDM_BUFFERSEP, GUI_TEXT(""));
		for (BufferIndex pos = 0; pos < buffers.lengthVisible; pos++) {
			const int itemID = bufferCmdID + pos;
			const GUI::gui_string entry = BufferTitle(pos, buffers.buffers[pos],
				Title::menu, props, localiser);
			SetMenuItem(menuBuffers, menuStart + pos + 1, itemID, entry.c_str());

			const GUI::gui_string titleTab = BufferTitle(pos, buffers.buffers[pos],
				Title::tab, props, localiser);
			tabNames.push_back(titleTab);
		}
	}
	UpdateTabs(tabNames);

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

				const GUI::gui_string path = EscapeFilePath(
					recentFileStack[stackPos], Title::menu);

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
			const BufferIndex iBuffer = buffers.GetDocumentByName(bufferState.file, false);
			if (iBuffer >= 0) {
				buffers.buffers[iBuffer].file.filePosition = bufferState.file.filePosition;
				buffers.buffers[iBuffer].foldState = bufferState.foldState;
				buffers.buffers[iBuffer].bookmarks = bufferState.bookmarks;
				if (buffers.buffers[iBuffer].lifeState == Buffer::LifeState::opened) {
					// File was opened synchronously
					RestoreState(buffers.buffers[iBuffer], true);
					DisplayAround(buffers.buffers[iBuffer].file.filePosition);
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

FilePosition SciTEBase::GetFilePosition() {
	return FilePosition(GetSelectedRange(), GetCurrentScrollPosition());
}

void SciTEBase::DisplayAround(const FilePosition &fp) {
	if ((fp.selection.position != SA::InvalidPosition) && (fp.selection.anchor != SA::InvalidPosition)) {
		SetSelection(fp.selection.anchor, fp.selection.position);

		const SA::Line curTop = wEditor.FirstVisibleLine();
		const SA::Line lineTop = wEditor.VisibleFromDocLine(fp.scrollPosition);
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
				const RecentFile rf = recentFileStack[pos];
				// Already asked user so don't allow Open to ask again.
				Open(rf, ofNoSaveIfDirty);
				CurrentBuffer()->file.filePosition = rf.filePosition;
				DisplayAround(rf.filePosition);
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
		return jobMode.jobType == JobSubsystem::immediate;
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
	return subsystem.empty() ? JobSubsystem::cli : SubsystemFromChar(subsystem.at(0));
}

void SciTEBase::ToolsMenu(int item) {
	SelectionIntoProperties();

	const std::string itemSuffix = StdStringFromInteger(item) + ".";
	const std::string propName = std::string("command.") + itemSuffix;
	std::string command(props.GetWild(propName.c_str(), FileNameExt().AsUTF8().c_str()));
	if (command.length()) {
		JobMode jobMode(props, item, FileNameExt().AsUTF8().c_str());
		if (jobQueue.IsExecuting() && (jobMode.jobType != JobSubsystem::immediate))
			// Busy running a tool and running a second can cause failures.
			return;
		if (jobMode.saveBefore == 2 || (jobMode.saveBefore == 1 && (!(CurrentBuffer()->isDirty) || Save())) || SaveIfUnsure() != SaveResult::cancelled) {
			if (jobMode.isFilter)
				CurrentBuffer()->fileModTime -= 1;
			if (jobMode.jobType == JobSubsystem::immediate) {
				if (extender) {
					extender->OnExecute(command.c_str());
				}
			} else {
				AddCommand(command, "", jobMode.jobType, jobMode.input, jobMode.flags);
				if (jobQueue.HasCommandToRun())
					Execute();
			}
		}
	}
}

namespace {

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

constexpr const char *CSI = "\033[";

constexpr bool SeqEnd(int ch) noexcept {
	return (ch == 0) || ((ch >= '@') && (ch <= '~'));
}

void RemoveEscSeq(std::string &s) {
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
void Chomp(std::string &s, char ch) {
	const size_t posCh = s.find(ch);
	if (posCh != std::string::npos)
		s.erase(0, posCh + 1);
}

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
		std::string message = wOutput.StringOfSpan(SA::Span(startPosLine, lineEnd));
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
			std::string message = wOutput.StringOfSpan(SA::Span(startPosLine, startPosLine + lineLength));
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
						for (BufferIndex i = buffers.lengthVisible - 1; i >= 0; i--) {
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
				EnsureRangeVisible(wEditor, SA::Span(startSourceLine));
				if (props.GetInt("error.select.line") == 1) {
					//select whole source source line from column with error
					SetSelection(endSourceline, startSourceLine);
				} else {
					//simply move cursor to line, don't do any selection
					SetSelection(startSourceLine, startSourceLine);
				}
				std::replace(message.begin(), message.end(), '\t', ' ');
				::Remove(message, std::string("\n"));
				props.Set("CurrentMessage", message);
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

