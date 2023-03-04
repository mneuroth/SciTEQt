// SciTE - Scintilla based Text Editor
// LuaExtension.h - Lua scripting extension
// Copyright 1998-2000 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LUAEXTENSION_H
#define LUAEXTENSION_H

class LuaExtension : public Extension {
private:
	LuaExtension() noexcept; // Singleton

public:
	static LuaExtension &Instance() noexcept;

	// Deleted so LuaExtension objects can not be copied.
	LuaExtension(const LuaExtension &) = delete;
	LuaExtension(LuaExtension &&) = delete;
	LuaExtension &operator=(const LuaExtension &) = delete;
	LuaExtension &operator=(LuaExtension &&) = delete;

	~LuaExtension() override;

	bool Initialise(ExtensionAPI *host_) override;
	bool Finalise() noexcept override;
	bool Clear() override;
	bool Load(const char *filename) override;

	bool InitBuffer(int) override;
	bool ActivateBuffer(int) override;
	bool RemoveBuffer(int) override;

	bool OnOpen(const char *filename) override;
	bool OnSwitchFile(const char *filename) override;
	bool OnBeforeSave(const char *filename) override;
	bool OnSave(const char *filename) override;
	bool OnChar(char ch) override;
	bool OnExecute(const char *s) override;
	bool OnSavePointReached() override;
	bool OnSavePointLeft() override;
	bool OnStyle(Scintilla::Position startPos, Scintilla::Position lengthDoc, int initStyle, StyleWriter *styler) override;
	bool OnDoubleClick() override;
	bool OnUpdateUI() override;
	bool OnMarginClick() override;
	bool OnUserListSelection(int listType, const char *selection) override;
	bool OnKey(int keyval, int modifiers) override;
	bool OnDwellStart(Scintilla::Position pos, const char *word) override;
	bool OnClose(const char *filename) override;
	bool OnUserStrip(int control, int change) override;
	bool NeedsOnClose() override;
};

#endif
