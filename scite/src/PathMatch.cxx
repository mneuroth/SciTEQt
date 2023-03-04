// SciTE - Scintilla based Text Editor
/** @file PathMatch.cxx
 ** Match path patterns.
 **/
// Copyright 2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cassert>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <chrono>

#include "GUI.h"

#include "StringHelpers.h"
#include "FilePath.h"
#include "PathMatch.h"

namespace {

int IntFromString(std::u32string_view s) noexcept {
	if (s.empty()) {
		return 0;
	}
	const bool negate = s.front() == '-';
	if (negate) {
		s.remove_prefix(1);
	}
	int value = 0;
	while (!s.empty()) {
		value = value * 10 + s.front() - '0';
		s.remove_prefix(1);
	}
	return negate ? -value : value;
}

}

bool PatternMatch(std::u32string_view pattern, std::u32string_view text) noexcept {
	if (pattern == text) {
		return true;
	} else if (pattern.empty()) {
		return false;
	} else if (pattern.front() == '\\') {
		pattern.remove_prefix(1);
		if (pattern.empty()) {
			// Escape with nothing being escaped
			return false;
		}
		if (text.empty()) {
			return false;
		}
		if (pattern.front() == text.front()) {
			pattern.remove_prefix(1);
			text.remove_prefix(1);
			return PatternMatch(pattern, text);
		}
		return false;
	} else if (pattern.front() == '*') {
		pattern.remove_prefix(1);
		if (!pattern.empty() && pattern.front() == '*') {
			pattern.remove_prefix(1);
			// "**" matches anything including "/"
			while (!text.empty()) {
				if (PatternMatch(pattern, text)) {
					return true;
				}
				text.remove_prefix(1);
			}
		} else {
			while (!text.empty()) {
				if (PatternMatch(pattern, text)) {
					return true;
				}
				if (text.front() == '/') {
					// "/" not matched by single "*"
					return false;
				}
				text.remove_prefix(1);
			}
		}
		assert(text.empty());
		// Consumed whole text with wildcard so match if pattern consumed
		return pattern.empty();
	} else if (text.empty()) {
		return false;
	} else if (pattern.front() == '?') {
		if (text.front() == '/') {
			return false;
		}
		pattern.remove_prefix(1);
		text.remove_prefix(1);
		return PatternMatch(pattern, text);
	} else if (pattern.front() == '[') {
		pattern.remove_prefix(1);
		if (pattern.empty()) {
			return false;
		}
		const bool positive = pattern.front() != '!';
		if (!positive) {
			pattern.remove_prefix(1);
			if (pattern.empty()) {
				return false;
			}
		}
		bool inSet = false;
		if (!pattern.empty() && pattern.front() == ']') {
			// First is allowed to be ']'
			if (pattern.front() == text.front()) {
				inSet = true;
			}
			pattern.remove_prefix(1);
		}
		char32_t start = 0;
		while (!pattern.empty() && pattern.front() != ']') {
			if (pattern.front() == '-') {
				pattern.remove_prefix(1);
				if (!pattern.empty()) {
					const char32_t end = pattern.front();
					if ((text.front() >= start) && (text.front() <= end)) {
						inSet = true;
					}
				}
			} else if (pattern.front() == text.front()) {
				inSet = true;
			}
			if (!pattern.empty()) {
				start = pattern.front();
				pattern.remove_prefix(1);
			}
		}
		if (!pattern.empty()) {
			pattern.remove_prefix(1);
		}
		if (inSet != positive) {
			return false;
		}
		text.remove_prefix(1);
		return PatternMatch(pattern, text);
	} else if (pattern.front() == '{') {
		if (pattern.length() < 2) {
			return false;
		}
		const size_t endParen = pattern.find('}');
		if (endParen == std::u32string_view::npos) {
			// Malformed {x} pattern
			return false;
		}
		std::u32string_view parenExpression = pattern.substr(1, endParen - 1);
		bool inSet = false;
		const size_t dotdot = parenExpression.find(U"..");
		if (dotdot != std::u32string_view::npos) {
			// Numeric range: {10..20}
			const std::u32string_view firstRange = parenExpression.substr(0, dotdot);
			const std::u32string_view lastRange = parenExpression.substr(dotdot+2);
			if (firstRange.empty() || lastRange.empty()) {
				// Malformed {s..e} range pattern
				return false;
			}
			const size_t endInteger = text.find_last_of(U"-0123456789");
			if (endInteger == std::u32string_view::npos) {
				// No integer in text
				return false;
			}
			const std::u32string_view intPart = text.substr(0, endInteger+1);
			const int first = IntFromString(firstRange);
			const int last = IntFromString(lastRange);
			const int value = IntFromString(intPart);
			if ((value >= first) && (value <= last)) {
				inSet = true;
				text.remove_prefix(intPart.length());
			}
		} else {
			// Alternates: {a,b,cd}
			size_t comma = parenExpression.find(',');
			for (;;) {
				const bool finalAlt = comma == std::u32string_view::npos;
				const std::u32string_view oneAlt = finalAlt ? parenExpression :
					parenExpression.substr(0, comma);
				if (oneAlt == text.substr(0, oneAlt.length())) {
					// match
					inSet = true;
					text.remove_prefix(oneAlt.length());
					break;
				}
				if (finalAlt) {
					break;
				}
				parenExpression.remove_prefix(oneAlt.length() + 1);
				comma = parenExpression.find(',');
			}
		}
		if (!inSet) {
			return false;
		}
		pattern.remove_prefix(endParen + 1);
		return PatternMatch(pattern, text);
	} else if (pattern.front() == text.front()) {
		pattern.remove_prefix(1);
		text.remove_prefix(1);
		return PatternMatch(pattern, text);
	}
	return false;
}

#if defined(TESTING)

namespace {

static void TestPatternMatch() {
	static bool done = false;
	if (done) {
		return;
	}
	done = true;
	// Literals
	assert(PatternMatch(U"", U""));
	assert(PatternMatch(U"a", U"a"));
	assert(PatternMatch(U"a", U"b") == false);
	assert(PatternMatch(U"ab", U"ab"));
	assert(PatternMatch(U"ab", U"a") == false);
	assert(PatternMatch(U"a", U"ab") == false);

	// * matches anything except for '/'
	assert(PatternMatch(U"*", U""));
	assert(PatternMatch(U"*", U"a"));
	assert(PatternMatch(U"*", U"ab"));

	assert(PatternMatch(U"a*", U"a"));
	assert(PatternMatch(U"a*", U"ab"));
	assert(PatternMatch(U"a*", U"abc"));
	assert(PatternMatch(U"a*", U"bc") == false);

	assert(PatternMatch(U"*a", U"a"));
	assert(PatternMatch(U"*a", U"za"));
	assert(PatternMatch(U"*a", U"yza"));
	assert(PatternMatch(U"*a", U"xyz") == false);
	assert(PatternMatch(U"a*z", U"a/z") == false);
	assert(PatternMatch(U"a*b*c", U"abc"));
	assert(PatternMatch(U"a*b*c", U"a1b234c"));

	// ? matches one character except for '/'
	assert(PatternMatch(U"?", U"a"));
	assert(PatternMatch(U"?", U"") == false);
	assert(PatternMatch(U"a?c", U"abc"));
	assert(PatternMatch(U"a?c", U"a/c") == false);

	// [set] matches one character from set
	assert(PatternMatch(U"a[123]z", U"a2z"));
	assert(PatternMatch(U"a[123]z", U"az") == false);
	assert(PatternMatch(U"a[123]z", U"a2") == false);

	// [!set] matches one character not from set
	assert(PatternMatch(U"a[!123]z", U"ayz"));
	assert(PatternMatch(U"a[!123]", U"az"));
	assert(PatternMatch(U"a[!123]", U"a2") == false);

	// [b-d] matches one character between b and d
	assert(PatternMatch(U"a[p-t]z", U"apz"));
	assert(PatternMatch(U"a[p-t]z", U"asz"));
	assert(PatternMatch(U"a[p-t]z", U"atz"));
	assert(PatternMatch(U"a[p-t]z", U"aaz") == false);
	assert(PatternMatch(U"a[!p-t]z", U"aaz"));
	assert(PatternMatch(U"a[]a]z", U"a]z"));

	// ** matches anything including '/'
	assert(PatternMatch(U"**a", U"a"));
	assert(PatternMatch(U"**a", U"za"));
	assert(PatternMatch(U"**a", U"yza"));
	assert(PatternMatch(U"**a", U"xyz") == false);
	assert(PatternMatch(U"a**z", U"a/z"));
	assert(PatternMatch(U"a**z", U"a/b/z"));
	assert(PatternMatch(U"a**/z", U"a/b/z"));
	assert(PatternMatch(U"a/**/z", U"a/b/z"));
	assert(PatternMatch(U"a**", U"a/b/z"));
	assert(PatternMatch(U"lexilla/**/Lex*.[ci]xx", U"lexilla/lexers/LexPython.cxx"));
	assert(PatternMatch(U"lexilla/*/LexAda*.cxx", U"lexilla/lexers/LexAda.cxx"));

	// {alt1,alt2,...} matches any of the alternatives
	assert(PatternMatch(U"<{ab}>", U"<ab>"));
	assert(PatternMatch(U"<{ab,lm,xyz}>", U"<ab>"));
	assert(PatternMatch(U"<{ab,lm,xyz}>", U"<lm>"));
	assert(PatternMatch(U"<{ab,lm,xyz}>", U"<xyz>"));
	assert(PatternMatch(U"<{ab,lm,xyz}>", U"<rs>") == false);

	// {num1..num2} matches any integer in the range
	assert(PatternMatch(U"{10..19}", U"15"));
	assert(PatternMatch(U"{10..19}", U"10"));
	assert(PatternMatch(U"{10..19}", U"19"));
	assert(PatternMatch(U"{10..19}", U"20") == false);
	assert(PatternMatch(U"{10..19}", U"ab") == false);
	assert(PatternMatch(U"{-19..-10}", U"-15"));
	assert(PatternMatch(U"{10..19}a", U"15a"));
	assert(PatternMatch(U"{..19}", U"15") == false);
	assert(PatternMatch(U"{10..}", U"15") == false);
}

}

#endif

bool PathMatch(std::string pattern, std::string relPath) {
#if defined(TESTING)
	TestPatternMatch();
#endif
	// Remove trailing white space
	while (!pattern.empty() && IsASpace(pattern.back())) {
		pattern.pop_back();
	}
#if defined(_WIN32)
	// Convert Windows path separators to Unix
	std::replace(relPath.begin(), relPath.end(), '\\', '/');
#endif
	if (!FilePath::CaseSensitive()) {
		pattern = GUI::LowerCaseUTF8(pattern);
		relPath = GUI::LowerCaseUTF8(relPath);
	}
	const std::u32string patternU32 = UTF32FromUTF8(pattern);
	const std::u32string relPathU32 = UTF32FromUTF8(relPath);
	if (PatternMatch(patternU32, relPathU32)) {
		return true;
	}
	const size_t lastSlash = relPathU32.rfind('/');
	if (lastSlash == std::string::npos) {
		return false;
	}
	// Match against just filename
	const std::u32string fileNameU32 = relPathU32.substr(lastSlash+1);
	return PatternMatch(patternU32, fileNameU32);
}
