// SciTE - Scintilla based Text Editor
/** @file Cookie.cxx
 ** Examine start of files for coding cookies and type information.
 **/
// Copyright 2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstring>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>

#include "GUI.h"

#include "StringHelpers.h"
#include "Cookie.h"

std::string_view ExtractLine(std::string_view sv) noexcept {
	std::string_view remainder = sv;
	while ((remainder.length() > 0) && (remainder[0] != '\r') && (remainder[0] != '\n')) {
		remainder.remove_prefix(1);
	}
	if ((remainder.length() > 1) && (remainder[0] == '\r') && (remainder[1] == '\n')) {
		remainder.remove_prefix(1);
	}
	if (remainder.length() > 0) {
		remainder.remove_prefix(1);
	}
	sv.remove_suffix(remainder.length());
	return sv;
}

namespace {

constexpr std::string_view codingCookie("coding");
constexpr std::string_view utf8Name("utf-8");

constexpr bool isEncodingChar(char ch) noexcept {
	return (ch == '_') || (ch == '-') || (ch == '.') ||
	       (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
	       (ch >= '0' && ch <= '9');
}

constexpr bool isSpaceChar(char ch) noexcept {
	return (ch == ' ') || (ch == '\t');
}

UniMode CookieValue(std::string_view s) noexcept {
	const size_t posCoding = s.find(codingCookie);
	if (posCoding != std::string_view::npos) {
		s.remove_prefix(posCoding + codingCookie.length());
		if ((s.length() > 0) && ((s[0] == ':') || (s[0] == '='))) {
			s.remove_prefix(1);
			if ((s.length() > 0) && ((s[0] == '\"') || (s[0] == '\''))) {
				s.remove_prefix(1);
			}
			while ((s.length() > 0) && (isSpaceChar(s[0]))) {
				s.remove_prefix(1);
			}
			size_t endCoding = 0;
			while ((endCoding < s.length()) &&
					(isEncodingChar(s[endCoding]))) {
				endCoding++;
			}
			s.remove_suffix(s.length() - endCoding);
			if (EqualCaseInsensitive(s, utf8Name)) {
				return UniMode::cookie;
			}
		}
	}
	return UniMode::uni8Bit;
}

}

UniMode CodingCookieValue(std::string_view sv) noexcept {
	const std::string_view l1 = ExtractLine(sv);
	UniMode unicodeMode = CookieValue(l1);
	if (unicodeMode == UniMode::uni8Bit) {
		sv.remove_prefix(l1.length());
		const std::string_view l2 = ExtractLine(sv);
		unicodeMode = CookieValue(l2);
	}
	return unicodeMode;
}

