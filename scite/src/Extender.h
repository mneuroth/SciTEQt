// SciTE - Scintilla based Text Editor
/** @file Extender.h
 ** SciTE extension interface.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EXTENDER_H
#define EXTENDER_H

class StyleWriter;

inline intptr_t SptrFromPointer(void *p) noexcept {
	return reinterpret_cast<intptr_t>(p);
}

inline intptr_t SptrFromString(const char *cp) noexcept {
	return reinterpret_cast<intptr_t>(cp);
}

inline uintptr_t UptrFromString(const char *cp) noexcept {
	return reinterpret_cast<uintptr_t>(cp);
}

class ExtensionAPI {
public:
	virtual ~ExtensionAPI() {
	}
	enum Pane { paneEditor=1, paneOutput=2, paneFindOutput=3 };
    virtual intptr_t Send(Pane p, Scintilla::Message msg, uintptr_t wParam=0, intptr_t lParam=0)=0;
	virtual std::string Range(Pane p, Scintilla::Span range)=0;
	virtual void Remove(Pane p, Scintilla::Position start, Scintilla::Position end)=0;
	virtual void Insert(Pane p, Scintilla::Position pos, const char *s)=0;
	virtual void Trace(const char *s)=0;
	virtual std::string Property(const char *key)=0;
	virtual void SetProperty(const char *key, const char *val)=0;
	virtual void UnsetProperty(const char *key)=0;
	virtual uintptr_t GetInstance()=0;
	virtual void ShutDown()=0;
	virtual void Perform(const char *actions)=0;
	virtual void DoMenuCommand(int cmdID)=0;
	virtual void UpdateStatusBar(bool bUpdateSlowData)=0;
	virtual void UserStripShow(const char *description)=0;
	virtual void UserStripSet(int control, const char *value)=0;
	virtual void UserStripSetList(int control, const char *value)=0;
	virtual std::string UserStripValue(int control)=0;
	virtual Scintilla::ScintillaCall &PaneCaller(Pane p) noexcept =0;
};

/**
 * Methods in extensions return true if they have completely handled an event and
 * false if default processing is to continue.
 */
class Extension {
public:
	virtual ~Extension() {}

	virtual bool Initialise(ExtensionAPI *host_)=0;
	virtual bool Finalise() noexcept =0;
	virtual bool Clear()=0;
	virtual bool Load(const char *filename)=0;

	virtual bool InitBuffer(int) { return false; }
	virtual bool ActivateBuffer(int) { return false; }
	virtual bool RemoveBuffer(int) { return false; }

	virtual bool OnOpen(const char *) { return false; }
	virtual bool OnSwitchFile(const char *) { return false; }
	virtual bool OnBeforeSave(const char *) { return false; }
	virtual bool OnSave(const char *) { return false; }
	virtual bool OnChar(char) { return false; }
	virtual bool OnExecute(const char *) { return false; }
	virtual bool OnSavePointReached() { return false; }
	virtual bool OnSavePointLeft() { return false; }
	virtual bool OnStyle(Scintilla::Position, Scintilla::Position, int, StyleWriter *) {
		return false;
	}
	virtual bool OnDoubleClick() { return false; }
	virtual bool OnUpdateUI() { return false; }
	virtual bool OnMarginClick() { return false; }
	virtual bool OnMacro(const char *, const char *) { return false; }
	virtual bool OnUserListSelection(int, const char *) { return false; }

	virtual bool SendProperty(const char *) { return false; }

	virtual bool OnKey(int, int) { return false; }
	virtual bool OnDwellStart(Scintilla::Position, const char *) { return false; }
	virtual bool OnClose(const char *) { return false; }
	virtual bool OnUserStrip(int /* control */, int /* change */) { return false; }
	virtual bool NeedsOnClose() { return true; }
};

#endif
