// SciTE - Scintilla based Text Editor
// Widget.h - code for manipulating  GTK widgets
// Copyright 2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef WIDGET_H
#define WIDGET_H

// Callback thunk class connects GTK signals to a simple command method.

// Callback thunk class connects GTK signals to an instance method.
template< class T, void (T::*method)() >
class ObjectSignal {
public:
	static void Function(GtkWidget */*w*/, T *object) {
		(object->*method)();
	}
};

class WBase : public GUI::Window {
public:
	operator GtkWidget*() const;
	GtkWidget* Pointer();
	bool Sensitive();
};

inline GtkWidget *Widget(const GUI::Window &w) {
	return static_cast<GtkWidget *>(w.GetID());
}

class WStatic : public WBase {
public:
	void Create(GUI::gui_string text);
	bool HasMnemonic();
	void SetMnemonicFor(WBase &w);
};

class WEntry : public WBase {
public:
	void Create(const GUI::gui_char *text=0);
	void ActivatesDefault();
	const GUI::gui_char *Text();
	int Value();
	void SetText(const GUI::gui_char *text);
	static void SetValid(GtkEntry *entry, bool valid);
};

class WComboBoxEntry : public WBase {
public:
	void Create();
	GtkEntry *Entry();
	void ActivatesDefault();
	const GUI::gui_char *Text();
	void SetText(const GUI::gui_char *text);
	bool HasFocusOnSelfOrChild();
	void ClearList();
	void RemoveText(int position);
	void AppendText(const char *text);
	void FillFromMemory(const std::vector<std::string> &mem, bool useTop = false);
};

class WButton : public WBase {
public:
	void Create(GUI::gui_string text, GCallback func, gpointer data);
	void Create(GUI::gui_string text);
};

class WToggle : public WBase {
public:
	void Create(const GUI::gui_string &text);
	bool Active();
	void SetActive(bool active);
};

class WCheckDraw : public WBase {
public:
	typedef void (*ChangeFunction)(WCheckDraw *cd, void *user);
private:
	ChangeFunction cdfn;
	void *user;
	static void Toggled(GtkWidget *widget, WCheckDraw *pcd);
	GtkToggleButton *ToggleButton();
public:
	WCheckDraw();
	~WCheckDraw();
	void Create(const char **xpmImage, const GUI::gui_string &toolTip, GtkStyle *pStyle_);
	bool Active();
	void SetActive(bool active);
	void Toggle();
	void SetChangeFunction(ChangeFunction cdfn_, void *user_);
	enum {  checkIconWidth = 16, checkButtonWidth = 16 + 3 * 2 + 1};
};

class WProgress : public WBase {
public:
	void Create();
};

class WTable : public WBase {
private:
	int rows;
	int columns;
	int next;
public:
	WTable(int rows_, int columns_);
	void Add(GtkWidget *child=0, int width=1, bool expand=false,
		int xpadding=5, int ypadding=5);
	void Label(GtkWidget *child);
	void PackInto(GtkBox *box, gboolean expand=TRUE);
	void Resize(int rows_, int columns_);
	void NextLine();
};

GUI::gui_char KeyFromLabel(GUI::gui_string label);

class Dialog : public GUI::Window {
public:
	void Create(const GUI::gui_string &title);
	void Display(GtkWidget *parent = 0, bool modal=true);
	GtkWidget *ResponseButton(const GUI::gui_string &text, int responseID);
	void Present();
	GtkWidget *ContentArea();

private:
	static void SignalDestroy(GtkWidget *, Dialog *d);
};

class BaseWin : public GUI::Window {
protected:
	ILocalize *localiser;
public:
	BaseWin() : localiser(0) {
	}
	void SetLocalizer(ILocalize *localiser_) {
		localiser = localiser_;
	}
};

class Strip : public BaseWin {
protected:
	bool allowMenuActions;
	bool childHasFocus;
	enum { heightButton=23, heightStatic=12, widthCombo=20};
public:
	bool visible;
	Strip() : allowMenuActions(false), childHasFocus(false), visible(false) {
	}
	virtual ~Strip() {
	}
	virtual void Creation(GtkWidget *container);
	virtual void Show(int buttonHeight);
	virtual void Close();
	virtual bool KeyDown(GdkEventKey *event);
	virtual void ShowPopup() {}
	virtual void MenuAction(guint /* action */) {}
	static void MenuSignal(GtkMenuItem *menuItem, Strip *pStrip);
	void AddToPopUp(GUI::Menu &popup, const char *label, int cmd, bool checked);
	virtual void ChildFocus(GtkWidget *widget);
	static gboolean ChildFocusSignal(GtkContainer *container, GtkWidget *widget, Strip *pStrip);
	virtual gboolean Focus(GtkDirectionType /* direction*/ ) { return false; }
	static gboolean FocusSignal(GtkWidget *widget, GtkDirectionType direction, Strip *pStrip);
	bool VisibleHasFocus() const;
	static gint ButtonsPress(GtkWidget *widget, GdkEventButton *event, Strip *pstrip);
};

#endif
