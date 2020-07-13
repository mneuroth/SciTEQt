// SciTE - Scintilla based Text Editor
/** @file GUIWin.cxx
 ** Interface to platform GUI facilities.
 ** Split off from Scintilla's Platform.h to avoid SciTE depending on implementation of Scintilla.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <string>
#include <vector>
#include <chrono>
#include <sstream>

#ifdef __MINGW_H
#define _WIN32_IE	0x0400
#endif

#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0602
#include <windows.h>

#include "ScintillaTypes.h"
#include "GUI.h"

namespace GUI {

enum { SURROGATE_LEAD_FIRST = 0xD800 };
enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
enum { SURROGATE_TRAIL_LAST = 0xDFFF };

static unsigned int UTF8Length(const wchar_t *uptr, size_t tlen) noexcept {
	unsigned int len = 0;
	for (size_t i = 0; i < tlen && uptr[i];) {
		const unsigned int uch = uptr[i];
		if (uch < 0x80) {
			len++;
		} else if (uch < 0x800) {
			len += 2;
		} else if ((uch >= SURROGATE_LEAD_FIRST) &&
				(uch <= SURROGATE_TRAIL_LAST)) {
			len += 4;
			i++;
		} else {
			len += 3;
		}
		i++;
	}
	return len;
}

static void UTF8FromUTF16(const wchar_t *uptr, size_t tlen, char *putf) noexcept {
	int k = 0;
	for (size_t i = 0; i < tlen && uptr[i];) {
		const unsigned int uch = uptr[i];
		if (uch < 0x80) {
			putf[k++] = static_cast<char>(uch);
		} else if (uch < 0x800) {
			putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		} else if ((uch >= SURROGATE_LEAD_FIRST) &&
				(uch <= SURROGATE_TRAIL_LAST)) {
			// Half a surrogate pair
			i++;
			const unsigned int xch = 0x10000 + ((uch & 0x3ff) << 10) + (uptr[i] & 0x3ff);
			putf[k++] = static_cast<char>(0xF0 | (xch >> 18));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 12) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (xch & 0x3f));
		} else {
			putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
			putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
		i++;
	}
}

static size_t UTF16Length(const char *s, size_t len) noexcept {
	size_t ulen = 0;
	size_t charLen;
	for (size_t i=0; i<len;) {
		const unsigned char ch = static_cast<unsigned char>(s[i]);
		if (ch < 0x80) {
			charLen = 1;
		} else if (ch < 0x80 + 0x40 + 0x20) {
			charLen = 2;
		} else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
			charLen = 3;
		} else {
			charLen = 4;
			ulen++;
		}
		i += charLen;
		ulen++;
	}
	return ulen;
}

static size_t UTF16FromUTF8(const char *s, size_t len, gui_char *tbuf, size_t tlen) noexcept {
	size_t ui=0;
	const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
	size_t i=0;
	while ((i<len) && (ui<tlen)) {
		unsigned char ch = us[i++];
		if (ch < 0x80) {
			tbuf[ui] = ch;
		} else if (ch < 0x80 + 0x40 + 0x20) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0x1F) << 6);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		} else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0xF) << 12);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + ((ch & 0x7F) << 6));
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		} else {
			// Outside the BMP so need two surrogates
			int val = (ch & 0x7) << 18;
			ch = us[i++];
			val += (ch & 0x3F) << 12;
			ch = us[i++];
			val += (ch & 0x3F) << 6;
			ch = us[i++];
			val += (ch & 0x3F);
			tbuf[ui] = static_cast<wchar_t>(((val - 0x10000) >> 10) + SURROGATE_LEAD_FIRST);
			ui++;
			tbuf[ui] = static_cast<wchar_t>((val & 0x3ff) + SURROGATE_TRAIL_FIRST);
		}
		ui++;
	}
	return ui;
}

gui_string StringFromUTF8(const char *s) {
	if (!s || !*s) {
		return gui_string();
	}
	const size_t sLen = strlen(s);
	const size_t wideLen = UTF16Length(s, sLen);
	std::vector<gui_char> vgc(wideLen);
	UTF16FromUTF8(s, sLen, &vgc[0], wideLen);
	return gui_string(&vgc[0], wideLen);
}

gui_string StringFromUTF8(const std::string &s) {
	if (s.empty()) {
		return gui_string();
	}
	const size_t sLen = s.length();
	const size_t wideLen = UTF16Length(s.c_str(), sLen);
	std::vector<gui_char> vgc(wideLen);
	UTF16FromUTF8(s.c_str(), sLen, &vgc[0], wideLen);
	return gui_string(&vgc[0], wideLen);
}

std::string UTF8FromString(const gui_string &s) {
	if (s.empty()) {
		return std::string();
	}
	const size_t sLen = s.size();
	const size_t narrowLen = UTF8Length(s.c_str(), sLen);
	std::vector<char> vc(narrowLen);
	UTF8FromUTF16(s.c_str(), sLen, &vc[0]);
	return std::string(&vc[0], narrowLen);
}

gui_string StringFromInteger(long i) {
	return std::to_wstring(i);
}

gui_string StringFromLongLong(long long i) {
	return std::to_wstring(i);
}

gui_string HexStringFromInteger(long i) {
	char number[32];
	sprintf(number, "%0lx", i);
	gui_char gnumber[32];
	size_t n = 0;
	while (number[n]) {
		gnumber[n] = static_cast<gui_char>(number[n]);
		n++;
	}
	gnumber[n] = 0;
	return gui_string(gnumber);
}

std::string LowerCaseUTF8(std::string_view sv) {
	if (sv.empty()) {
		return std::string();
	}
	const std::string s(sv);
	const gui_string gs = StringFromUTF8(s);
	const int chars = ::LCMapString(LOCALE_SYSTEM_DEFAULT, LCMAP_LOWERCASE, gs.c_str(), static_cast<int>(gs.size()), nullptr, 0);
	gui_string lc(chars, L'\0');
	::LCMapString(LOCALE_SYSTEM_DEFAULT, LCMAP_LOWERCASE, gs.c_str(), static_cast<int>(gs.size()), lc.data(), chars);
	return UTF8FromString(lc);
}

void Window::Destroy() {
	if (wid)
		::DestroyWindow(static_cast<HWND>(wid));
	wid = 0;
}

bool Window::HasFocus() {
	return ::GetFocus() == wid;
}

Rectangle Window::GetPosition() {
	RECT rc;
	::GetWindowRect(static_cast<HWND>(wid), &rc);
	return Rectangle(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::SetPosition(Rectangle rc) {
	::SetWindowPos(static_cast<HWND>(wid),
		       0, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER|SWP_NOACTIVATE);
}

Rectangle Window::GetClientPosition() {
	RECT rc= {0, 0, 0, 0};
	if (wid)
		::GetClientRect(static_cast<HWND>(wid), &rc);
	return  Rectangle(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::Show(bool show) {
	if (show)
		::ShowWindow(static_cast<HWND>(wid), SW_SHOWNOACTIVATE);
	else
		::ShowWindow(static_cast<HWND>(wid), SW_HIDE);
}

void Window::InvalidateAll() {
	::InvalidateRect(static_cast<HWND>(wid), nullptr, FALSE);
}

void Window::SetTitle(const gui_char *s) {
	::SetWindowTextW(static_cast<HWND>(wid), s);
}

void Menu::CreatePopUp() {
	Destroy();
	mid = ::CreatePopupMenu();
}

void Menu::Destroy() {
	if (mid)
		::DestroyMenu(static_cast<HMENU>(mid));
	mid = 0;
}

void Menu::Show(Point pt, Window &w) {
	::TrackPopupMenu(static_cast<HMENU>(mid),
			 TPM_RIGHTBUTTON, pt.x - 4, pt.y, 0,
			 static_cast<HWND>(w.GetID()), NULL);
	Destroy();
}

intptr_t ScintillaPrimitive::Send(unsigned int msg, uintptr_t wParam, intptr_t lParam) {
	return ::SendMessage(static_cast<HWND>(GetID()), msg, wParam, lParam);
}

bool IsDBCSLeadByte(int codePage, char ch) {
	if (Scintilla::API::CpUtf8 == codePage)
		// For lexing, all characters >= 0x80 are treated the
		// same so none is considered a lead byte.
		return false;
	else
		return ::IsDBCSLeadByteEx(codePage, ch) != 0;
}

void SleepMilliseconds(int sleepTime) {
	::Sleep(sleepTime);
}

}
