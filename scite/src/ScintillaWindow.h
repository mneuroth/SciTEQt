// SciTE - Scintilla based Text Editor
/** @file ScintillaWindow.h
 ** Interface to a Scintilla instance.
 **/
// Copyright 1998-2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SCINTILLAWINDOW_H
#define SCINTILLAWINDOW_H

namespace GUI {

class ScintillaWindow : public GUI::ScintillaPrimitive, public Scintilla::ScintillaCall {
public:
	ScintillaWindow() noexcept;
	~ScintillaWindow() override;
	// Deleted so ScintillaWindow objects can not be copied.
	ScintillaWindow(const ScintillaWindow &source) = delete;
	ScintillaWindow(ScintillaWindow &&) = delete;
	ScintillaWindow &operator=(const ScintillaWindow &) = delete;
	ScintillaWindow &operator=(ScintillaWindow &&) = delete;

	void SetScintilla(GUI::WindowID wid_);
	bool CanCall() const noexcept;
};

}


#endif
