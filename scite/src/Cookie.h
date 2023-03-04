// SciTE - Scintilla based Text Editor
/** @file Cookie.h
 ** Examine start of files for coding cookies and type information.
 **/
// Copyright 2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef COOKIE_H
#define COOKIE_H

// Related to Utf8_16::encodingType but with additional values at end
enum class UniMode {
	uni8Bit = 0, uni16BE = 1, uni16LE = 2, utf8 = 3,
	cookie = 4
};

std::string_view ExtractLine(std::string_view sv) noexcept;
UniMode CodingCookieValue(std::string_view sv) noexcept;

#endif
