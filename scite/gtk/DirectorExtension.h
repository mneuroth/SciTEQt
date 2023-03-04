// SciTE - Scintilla based Text Editor
/** @file DirectorExtension.h
 ** Extension for communicating with a director program.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DIRECTOREXTENSION_H
#define DIRECTOREXTENSION_H

class DirectorExtension : public Extension {
private:
	DirectorExtension() : inputWatcher(-1), inputChannel(0), host(0) {} // Singleton
	DirectorExtension(const DirectorExtension &); // Disable copy ctor
	void operator=(const DirectorExtension &);    // Disable operator=

public:
	static DirectorExtension &Instance();

	// Implement the Extension interface
	bool Initialise(ExtensionAPI *host_) override;
	bool Finalise() noexcept override;
	bool Clear() override;
	bool Load(const char *filename) override;

	bool OnOpen(const char *path) override;
	bool OnSwitchFile(const char *path) override;
	bool OnSave(const char *path) override;
	bool OnChar(char ch) override;
	bool OnExecute(const char *cmd) override;
	bool OnSavePointReached() override;
	bool OnSavePointLeft() override;
	bool OnStyle(SA::Position startPos, SA::Position lengthDoc, int initStyle, StyleWriter *styler) override;
	bool OnDoubleClick() override;
	bool OnUpdateUI() override;
	bool OnMarginClick() override;
	bool OnMacro(const char *command, const char *params) override;

	bool SendProperty(const char *prop) override;
	bool OnClose(const char *path) override;
	bool NeedsOnClose() override;

	// Allow messages through to extension
	void HandleStringMessage(const char *message);

	void CreatePipe(bool forceNew = false);
private:
	int inputWatcher;
	GIOChannel *inputChannel;
	ExtensionAPI *host;
};

#endif
