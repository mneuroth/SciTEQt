// SciTE - Scintilla based Text Editor
/** @file Searcher.h
 ** Definitions of search functionality.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SEARCHER_H
#define SEARCHER_H

// Interface between SciTE and dialogs and strips for find and replace
class Searcher {
public:
	std::string findWhat;
	std::string replaceWhat;

	bool wholeWord;
	bool matchCase;
	bool regExp;
	bool unSlash;
	bool wrapFind;
	bool reverseFind;
	bool filterState;
	bool contextVisible;

	Scintilla::Position searchStartPosition;
	bool replacing;
	bool havefound;
	bool failedfind;
	bool findInStyle;
	int findStyle;
	enum class CloseFind { closePrevent, closeAlways, closeOnMatch } closeFind;
	ComboMemory memFinds;
	ComboMemory memReplaces;

	bool focusOnReplace;

	Searcher();

	virtual void SetFindText(std::string_view sFind) = 0;
	virtual void SetFind(std::string_view sFind) = 0;
	virtual bool FindHasText() const noexcept = 0;
	void InsertFindInMemory();
	virtual void SetReplace(std::string_view sReplace) = 0;
	virtual void SetCaretAsStart() = 0;
	virtual void MoveBack() = 0;
	virtual void ScrollEditorIfNeeded() = 0;

	virtual Scintilla::Position FindNext(bool reverseDirection, bool showWarnings=true, bool allowRegExp=true) = 0;
	virtual void HideMatch() = 0;
	enum class MarkPurpose { withBookMarks, incremental, filter };
	virtual void MarkAll(MarkPurpose purpose) = 0;
	virtual void FilterAll(bool showMatches) = 0;
    virtual intptr_t ReplaceAll(bool inSelection) = 0;
	virtual void ReplaceOnce(bool showWarnings=true) = 0;
	virtual void UIClosed() = 0;
	virtual void UIHasFocus() = 0;
	bool &FlagFromCmd(int cmd) noexcept;
	bool ShouldClose(bool found) const noexcept {
		return (closeFind == CloseFind::closeAlways) || (found && (closeFind == CloseFind::closeOnMatch));
	}
};

// User interface for search options implemented as both buttons and popup menu items
struct SearchOption {
	enum { tWord, tCase, tRegExp, tBackslash, tWrap, tUp, tFilter, tContext };
	const char *label;
	int cmd;	// Menu item
	int id;	// Control in dialog
};

class SearchUI {
protected:
	Searcher *pSearcher;
public:
	SearchUI() noexcept : pSearcher(nullptr) {
	}
	void SetSearcher(Searcher *pSearcher_) noexcept {
		pSearcher = pSearcher_;
	}
};

#endif
