// SciTE - Scintilla based Text Editor
/** @file StringHelpers.cxx
 ** Implementation of widely useful string functions.
 **/
// Copyright 2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <chrono>

#include "GUI.h"
#include "StringHelpers.h"

bool StartsWith(std::wstring_view s, std::wstring_view start) {
	return (s.size() >= start.size()) &&
	       (std::equal(s.begin(), s.begin() + start.size(), start.begin()));
}

bool StartsWith(std::string_view s, std::string_view start) {
	return (s.size() >= start.size()) &&
	       (std::equal(s.begin(), s.begin() + start.size(), start.begin()));
}

bool EndsWith(std::wstring_view s, std::wstring_view end) {
	return (s.size() >= end.size()) &&
	       (std::equal(s.begin() + s.size() - end.size(), s.end(), end.begin()));
}

bool Contains(std::string const &s, char ch) noexcept {
	return s.find(ch) != std::string::npos;
}

int Substitute(std::wstring &s, const std::wstring &sFind, const std::wstring &sReplace) {
	int c = 0;
	const size_t lenFind = sFind.size();
	const size_t lenReplace = sReplace.size();
	size_t posFound = s.find(sFind);
	while (posFound != std::wstring::npos) {
		s.replace(posFound, lenFind, sReplace);
		posFound = s.find(sFind, posFound + lenReplace);
		c++;
	}
	return c;
}

int Substitute(std::string &s, const std::string &sFind, const std::string &sReplace) {
	int c = 0;
	const size_t lenFind = sFind.size();
	const size_t lenReplace = sReplace.size();
	size_t posFound = s.find(sFind);
	while (posFound != std::string::npos) {
		s.replace(posFound, lenFind, sReplace);
		posFound = s.find(sFind, posFound + lenReplace);
		c++;
	}
	return c;
}

bool RemoveStringOnce(std::string &s, const char *marker) {
	const size_t modText = s.find(marker);
	if (modText != std::string::npos) {
		s.erase(modText, strlen(marker));
		return true;
	}
	return false;
}

std::string StdStringFromInteger(int i) {
	return std::to_string(i);
}

std::string StdStringFromSizeT(size_t i) {
	return std::to_string(i);
}

std::string StdStringFromDouble(double d, int precision) {
	char number[32];
	sprintf(number, "%.*f", precision, d);
	return std::string(number);
}

int IntegerFromString(const std::string &val, int defaultValue) {
	try {
		if (val.length()) {
			return std::stoi(val);
		}
	} catch (std::logic_error &) {
		// Ignore bad values, either non-numeric or out of range numeric
	}
	return defaultValue;
}

intptr_t IntPtrFromString(const std::string &val, intptr_t defaultValue) {
	try {
		if (val.length()) {
			return static_cast<intptr_t>(std::stoll(val));
		}
	} catch (std::logic_error &) {
		// Ignore bad values, either non-numeric or out of range numeric
	}
	return defaultValue;
}

long long LongLongFromString(const std::string &val, long long defaultValue) {
	try {
		if (val.length()) {
			return std::stoll(val);
		}
	} catch (std::logic_error &) {
		// Ignore bad values, either non-numeric or out of range numeric
	}
	return defaultValue;
}

void LowerCaseAZ(std::string &s) {
	std::transform(s.begin(), s.end(), s.begin(), MakeLowerCase);
}

intptr_t IntegerFromText(const char *s) noexcept {
	return static_cast<intptr_t>(atoll(s));
}

int CompareNoCase(const char *a, const char *b) noexcept {
	while (*a && *b) {
		if (*a != *b) {
			const char upperA = MakeUpperCase(*a);
			const char upperB = MakeUpperCase(*b);
			if (upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
	}
	// Either *a or *b is nul
	return *a - *b;
}

bool EqualCaseInsensitive(const char *a, const char *b) noexcept {
	return 0 == CompareNoCase(a, b);
}

bool EqualCaseInsensitive(std::string_view a, std::string_view b) noexcept {
	if (a.length() != b.length()) {
		return false;
	}
	for (size_t i = 0; i < a.length(); i++) {
		if (MakeUpperCase(a[i]) != MakeUpperCase(b[i])) {
			return false;
		}
	}
	return true;
}

bool isprefix(const char *target, const char *prefix) noexcept {
	while (*target && *prefix) {
		if (*target != *prefix)
			return false;
		target++;
		prefix++;
	}
	if (*prefix)
		return false;
	else
		return true;
}

std::u32string UTF32FromUTF8(std::string_view s) {
	std::u32string ret;
	while (!s.empty()) {
		const unsigned char uc = static_cast<unsigned char>(s.front());
		size_t lenChar = 1;
		if (uc >= 0x80 + 0x40 + 0x20 + 0x10) {
			lenChar = 4;
		} else if (uc >= 0x80 + 0x40 + 0x20) {
			lenChar = 3;
		} else if (uc >= 0x80 + 0x40) {
			lenChar = 2;
		}
		if (lenChar > s.length()) {
			// Character fragment
			for (size_t i = 0; i < s.length(); i++) {
				ret.push_back(static_cast<unsigned char>(s[i]));
			}
			break;
		}
		const char32_t ch32 = UTF32Character(s.data());
		ret.push_back(ch32);
		s.remove_prefix(lenChar);
	}
	return ret;
}

unsigned int UTF32Character(const char *utf8) noexcept {
	unsigned char ch = utf8[0];
	unsigned int u32Char;
	if (ch < 0x80) {
		u32Char = ch;
	} else if (ch < 0x80 + 0x40 + 0x20) {
		u32Char = (ch & 0x1F) << 6;
		ch = utf8[1];
		u32Char += ch & 0x7F;
	} else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
		u32Char = (ch & 0xF) << 12;
		ch = utf8[1];
		u32Char += (ch & 0x7F) << 6;
		ch = utf8[2];
		u32Char += ch & 0x7F;
	} else {
		u32Char = (ch & 0x7) << 18;
		ch = utf8[1];
		u32Char += (ch & 0x3F) << 12;
		ch = utf8[2];
		u32Char += (ch & 0x3F) << 6;
		ch = utf8[3];
		u32Char += ch & 0x3F;
	}
	return u32Char;
}

/**
 * Convert a string into C string literal form using \a, \b, \f, \n, \r, \t, \v, and \ooo.
 */
std::string Slash(const std::string &s, bool quoteQuotes) {
	std::string oRet;
	for (const char ch : s) {
		if (ch == '\a') {
			oRet.append("\\a");
		} else if (ch == '\b') {
			oRet.append("\\b");
		} else if (ch == '\f') {
			oRet.append("\\f");
		} else if (ch == '\n') {
			oRet.append("\\n");
		} else if (ch == '\r') {
			oRet.append("\\r");
		} else if (ch == '\t') {
			oRet.append("\\t");
		} else if (ch == '\v') {
			oRet.append("\\v");
		} else if (ch == '\\') {
			oRet.append("\\\\");
		} else if (quoteQuotes && (ch == '\'')) {
			oRet.append("\\\'");
		} else if (quoteQuotes && (ch == '\"')) {
			oRet.append("\\\"");
		} else if (IsASCII(ch) && (ch < ' ')) {
			oRet.push_back('\\');
			oRet.push_back(static_cast<char>((ch >> 6) + '0'));
			oRet.push_back(static_cast<char>((ch >> 3) + '0'));
			oRet.push_back(static_cast<char>((ch & 0x7) + '0'));
		} else {
			oRet.push_back(ch);
		}
	}
	return oRet;
}

/**
 * Is the character an octal digit?
 */
static bool IsOctalDigit(char ch) noexcept {
	return ch >= '0' && ch <= '7';
}

/**
 * If the character is an hexa digit, get its value.
 */
static int GetHexaDigit(char ch) noexcept {
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	}
	if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 10;
	}
	if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	}
	return -1;
}

/**
 * Convert C style \a, \b, \f, \n, \r, \t, \v, \ooo and \xhh into their indicated characters.
 */
unsigned int UnSlash(char *s) noexcept {
	const char *sStart = s;
	char *o = s;

	while (*s) {
		if (*s == '\\') {
			s++;
			if (*s == 'a') {
				*o = '\a';
			} else if (*s == 'b') {
				*o = '\b';
			} else if (*s == 'f') {
				*o = '\f';
			} else if (*s == 'n') {
				*o = '\n';
			} else if (*s == 'r') {
				*o = '\r';
			} else if (*s == 't') {
				*o = '\t';
			} else if (*s == 'v') {
				*o = '\v';
			} else if (IsOctalDigit(*s)) {
				int val = *s - '0';
				if (IsOctalDigit(*(s + 1))) {
					s++;
					val *= 8;
					val += *s - '0';
					if (IsOctalDigit(*(s + 1))) {
						s++;
						val *= 8;
						val += *s - '0';
					}
				}
				*o = static_cast<char>(val);
			} else if (*s == 'x') {
				s++;
				int val = 0;
				int ghd = GetHexaDigit(*s);
				if (ghd >= 0) {
					s++;
					val = ghd;
					ghd = GetHexaDigit(*s);
					if (ghd >= 0) {
						s++;
						val *= 16;
						val += ghd;
					}
				}
				*o = static_cast<char>(val);
			} else {
				*o = *s;
			}
		} else {
			*o = *s;
		}
		o++;
		if (*s) {
			s++;
		}
	}
	*o = '\0';
	return static_cast<unsigned int>(o - sStart);
}

std::string UnSlashString(const char *s) {
	std::string sCopy(s, strlen(s) + 1);
	const unsigned int len = UnSlash(&sCopy[0]);
	return sCopy.substr(0, len);
}

/**
 * Convert C style \0oo into their indicated characters.
 * This is used to get control characters into the regular expression engine.
 */
static unsigned int UnSlashLowOctal(char *s) noexcept {
	const char *sStart = s;
	char *o = s;
	while (*s) {
		if ((s[0] == '\\') && (s[1] == '0') && IsOctalDigit(s[2]) && IsOctalDigit(s[3])) {
			*o = static_cast<char>(8 * (s[2] - '0') + (s[3] - '0'));
			s += 3;
		} else {
			*o = *s;
		}
		o++;
		if (*s)
			s++;
	}
	*o = '\0';
	return static_cast<unsigned int>(o - sStart);
}

std::string UnSlashLowOctalString(const char *s) {
	std::string sCopy(s, strlen(s) + 1);
	const unsigned int len = UnSlashLowOctal(&sCopy[0]);
	return sCopy.substr(0, len);
}
