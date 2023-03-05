// SciTE - Scintilla based Text Editor
// Strips.h - implement strips on GTK
// Copyright 1998-2021 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STRIPS_H
#define STRIPS_H

class BackgroundStrip : public Strip {
public:
	WStatic wExplanation;
	WProgress wProgress;

	BackgroundStrip() {
	}
	void Creation(GtkWidget *container) override;
	void SetProgress(const GUI::gui_string &explanation, size_t size, size_t progress);
};

class FindReplaceStrip : public Strip, public SearchUI, public CheckDrawWatcher {
public:
	bool performFilter = true;
	WComboBoxEntry wComboFind;
	std::vector<std::unique_ptr<WCheckDraw>> wCheck;
	bool initializingSearch;
	enum IncrementalBehaviour { simple, incremental, showAllMatches };
	IncrementalBehaviour incrementalBehaviour;

	FindReplaceStrip() : initializingSearch(false), incrementalBehaviour(simple) {
	}
	void CreateChecks(std::initializer_list<int> widths);
	void SetIncrementalBehaviour(int behaviour);
	void MarkIncremental();
	virtual void NextIncremental();
	bool KeyDown(const GdkEventKey *event) override;
	void GrabToggles();
	void SetToggles();
	void ShowPopup() override;
	virtual void FindNextCmd()=0;
	void ConnectCombo();
	void CheckChanged() override;

	static void ActivateSignal(GtkWidget *w, FindReplaceStrip *pStrip);
	static void FindComboChanged(GtkEditable *, FindReplaceStrip *pStrip);
	static gboolean EscapeSignal(GtkWidget *w, GdkEventKey *event, FindReplaceStrip *pStrip);
};

class FindStrip : public FindReplaceStrip {
public:
	WStatic wStaticFind;
	WButton wButton;
	WButton wButtonMarkAll;

	FindStrip() {
		performFilter = false;
	}
	void Creation(GtkWidget *container) override;
	virtual void Destruction();
	void Show(int buttonHeight) override;
	void Close() override;
	void MenuAction(guint action) override;

	void GrabFields();
	void FindNextCmd() override;
	void MarkAllCmd();
	void ChildFocus(GtkWidget *widget) override;
	gboolean Focus(GtkDirectionType direction) override;
};

class ReplaceStrip : public FindReplaceStrip {
public:
	WStatic wStaticFind;
	WButton wButtonFind;
	WButton wButtonReplaceAll;
	WStatic wStaticReplace;
	WComboBoxEntry wComboReplace;
	WButton wButtonReplace;
	WButton wButtonReplaceInSelection;

	ReplaceStrip() {
	}
	void Creation(GtkWidget *container) override;
	virtual void Destruction();
	void Show(int buttonHeight) override;
	void Close() override;
	void MenuAction(guint action) override;

	void GrabFields();
	void FindNextCmd() override;
	void ReplaceAllCmd();
	void ReplaceCmd();
	void ReplaceInSelectionCmd();
	void ChildFocus(GtkWidget *widget) override;
	gboolean Focus(GtkDirectionType direction) override;
};

class FilterStrip : public FindReplaceStrip {
public:
	WStatic wStaticFind;

	FilterStrip() {
	}
	void Creation(GtkWidget *container) override;
	virtual void Destruction();
	void Show(int buttonHeight) override;
	void Close() override;
	void MenuAction(guint action) override;
	void GrabFields();
	void NextIncremental() override;
	void FindNextCmd() override;
};

class UserStripWatcher {
public:
	virtual void UserStripClosed() = 0;
};

class UserStrip : public Strip {
public:
	std::unique_ptr<StripDefinition> psd;
	Extension *extender = nullptr;
	UserStripWatcher *pUserStripWatcher = nullptr;
	WTable tableUser;

	UserStrip() : tableUser(1, 1){
	}
	void Creation(GtkWidget *container) override;
	virtual void Destruction();
	void Show(int buttonHeight) override;
	void Close() override;
	bool KeyDown(const GdkEventKey *event) override;
	static void ActivateSignal(GtkWidget *w, UserStrip *pStrip);
	static gboolean EscapeSignal(GtkWidget *w, GdkEventKey *event, UserStrip *pStrip);
	void ClickThis(const GtkWidget *w);
	static void ClickSignal(GtkWidget *w, UserStrip *pStrip);
	void ChildFocus(GtkWidget *widget) override;
	gboolean Focus(GtkDirectionType direction) override;
	void SetDescription(const char *description);
	void SetExtender(Extension *extender_);
	void SetUserStripWatcher(UserStripWatcher *pUserStripWatcher_);
	void Set(int control, const char *value);
	void SetList(int control, const char *value);
	std::string GetValue(int control);
};

#endif
