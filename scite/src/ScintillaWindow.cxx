// SciTE - Scintilla based Text Editor
/** @file ScintillaWindow.cxx
 ** Interface to a Scintilla instance.
 **/
// Copyright 1998-2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string>
#include <string_view>
#include <chrono>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "Scintilla.h"

#include "GUI.h"
#include "ScintillaWindow.h"

namespace GUI {

ScintillaWindow::ScintillaWindow() noexcept = default;

ScintillaWindow::~ScintillaWindow() = default;

void ScintillaWindow::SetScintilla(GUI::WindowID wid_) {
	SetID(wid_);
	if (wid) {
		SciFnDirectStatus fn_ = reinterpret_cast<SciFnDirectStatus>(
					  Send(static_cast<unsigned int>(Scintilla::Message::GetDirectStatusFunction)));
		const sptr_t ptr_ = Send(static_cast<unsigned int>(Scintilla::Message::GetDirectPointer));
		SetFnPtr(fn_, ptr_);
	}
}

bool ScintillaWindow::CanCall() const noexcept {
	return wid && IsValid();
}

}
