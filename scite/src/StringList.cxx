// SciTE - Scintilla based Text Editor
/** @file StringList.cxx
 ** Implementation of class holding a list of strings.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <chrono>

#include "GUI.h"
#include "StringList.h"
#include "StringHelpers.h"

static int CompareNCaseInsensitive(const char *a, const char *b, size_t len) noexcept {
	while (*a && *b && len) {
		if (*a != *b) {
			const char upperA = MakeUpperCase(*a);
			const char upperB = MakeUpperCase(*b);
			if (upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
		len--;
	}
	if (len == 0)
		return 0;
	else
		// Either *a or *b is nul
		return *a - *b;
}

/**
 * Creates an array that points into each word in the string and puts \0 terminators
 * after each word.
 */
static std::vector<char *> ArrayFromStringList(char *stringList, bool onlyLineEnds = false) {
	int prev = '\n';
	int words = 0;
	// For rapid determination of whether a character is a separator, build
	// a look up table.
	bool wordSeparator[256] = {};
	wordSeparator[static_cast<unsigned int>('\r')] = true;
	wordSeparator[static_cast<unsigned int>('\n')] = true;
	if (!onlyLineEnds) {
		wordSeparator[static_cast<unsigned int>(' ')] = true;
		wordSeparator[static_cast<unsigned int>('\t')] = true;
	}
	for (int j = 0; stringList[j]; j++) {
		const int curr = static_cast<unsigned char>(stringList[j]);
		if (!wordSeparator[curr] && wordSeparator[prev])
			words++;
		prev = curr;
	}
	std::vector<char *> keywords;
	const size_t slen = strlen(stringList);
	if (words) {
		prev = '\0';
		for (size_t k = 0; k < slen; k++) {
			if (!wordSeparator[static_cast<unsigned char>(stringList[k])]) {
				if (!prev) {
					keywords.push_back(&stringList[k]);
				}
			} else {
				stringList[k] = '\0';
			}
			prev = stringList[k];
		}
	}
	return keywords;
}

void StringList::SetFromListText() {
	sorted = false;
	sortedNoCase = false;
	words = ArrayFromStringList(&listText[0], onlyLineEnds);
	wordsNoCase = words;
}

static bool CmpString(const char *a, const char *b) noexcept {
	return strcmp(a, b) < 0;
}

static bool CmpStringNoCase(const char *a, const char *b) noexcept {
	return CompareNoCase(a, b) < 0;
}

void StringList::SortIfNeeded(bool ignoreCase) {
	// In both cases, the final empty sentinel element is not sorted.
	if (ignoreCase) {
		if (!sortedNoCase) {
			sortedNoCase = true;
			std::sort(wordsNoCase.begin(), wordsNoCase.end(), CmpStringNoCase);
		}
	} else {
		if (!sorted) {
			sorted = true;
			std::sort(words.begin(), words.end(), CmpString);
		}
	}
}

void StringList::Clear() noexcept {
	words.clear();
	wordsNoCase.clear();
	listText.clear();
	sorted = false;
	sortedNoCase = false;
}

void StringList::Set(const char *s) {
	listText.assign(s, s+strlen(s)+1);
	SetFromListText();
}

void StringList::Set(const std::vector<char> &data) {
	listText.assign(data.begin(), data.end());
	listText.push_back('\0');
	SetFromListText();
}

namespace {

// Functors used to find elements given a prefix

struct CompareString {
	size_t searchLen;
	explicit CompareString(size_t searchLen_) noexcept : searchLen(searchLen_) {}
	bool operator()(const char *a, const char *b) const noexcept {
		return strncmp(a, b, searchLen) < 0;
	}
};

struct CompareStringInsensitive {
	size_t searchLen;
	explicit CompareStringInsensitive(size_t searchLen_) noexcept : searchLen(searchLen_) {}
	bool operator()(const char *a, const char *b) const noexcept {
		return CompareNCaseInsensitive(a, b, searchLen) < 0;
	}
};

template<typename Compare>
std::string GetMatch(std::vector<char *>::iterator start, std::vector<char *>::iterator end,
		     const char *wordStart, const std::string &wordCharacters, int wordIndex, Compare comp) {
	std::vector<char *>::iterator elem = std::lower_bound(start, end, wordStart, comp);
	if (!comp(wordStart, *elem) && !comp(*elem, wordStart)) {
		// Found a matching element, now move forward wordIndex matching elements
		for (; elem < end; ++elem) {
			const char *word = *elem;
			if (!word[comp.searchLen] || !Contains(wordCharacters, word[comp.searchLen])) {
				if (wordIndex <= 0) {
					return std::string(word);
				}
				wordIndex--;
			}
		}
	}
	return std::string();
}

}

/**
 * Returns an element (complete) of the StringList array which has
 * the same beginning as the passed string.
 * The length of the word to compare is passed too.
 * Letter case can be ignored or preserved.
 */
std::string StringList::GetNearestWord(const char *wordStart, size_t searchLen, bool ignoreCase, const std::string &wordCharacters, int wordIndex) {
	if (words.empty())
		return std::string();
	SortIfNeeded(ignoreCase);
	if (ignoreCase) {
		return GetMatch(wordsNoCase.begin(), wordsNoCase.end(), wordStart, wordCharacters, wordIndex, CompareStringInsensitive(searchLen));
	} else { // preserve the letter case
		return GetMatch(words.begin(), words.end(), wordStart, wordCharacters, wordIndex, CompareString(searchLen));
	}
}

/**
 * Find the length of a 'word' which is actually an identifier in a string
 * which looks like "identifier(..." or "identifier" and where
 * there may be extra spaces after the identifier that should not be
 * counted in the length.
 */
static size_t LengthWord(const char *word, char otherSeparator) noexcept {
	const char *endWord = nullptr;
	// Find an otherSeparator
	if (otherSeparator)
		endWord = strchr(word, otherSeparator);
	// Find a '('. If that fails go to the end of the string.
	if (!endWord)
		endWord = strchr(word, '(');
	if (!endWord)
		endWord = word + strlen(word);
	assert(endWord);
	// Last case always succeeds so endWord != 0

	// Drop any space characters.
	if (endWord > word) {
		endWord--;	// Back from the '(', otherSeparator, or '\0'
		// Move backwards over any spaces
		while ((endWord > word) && (IsASpace(*endWord))) {
			endWord--;
		}
	}
	return endWord - word + 1;
}

template<typename Compare>
static std::string GetMatches(std::vector<char *>::iterator start, std::vector<char *>::iterator end, const char *wordStart, char otherSeparator, bool exactLen, Compare comp) {
	std::string wordList;
	const size_t wordStartLength = LengthWord(wordStart, otherSeparator);
	std::vector<char *>::iterator elem = std::lower_bound(start, end, wordStart, comp);
	// Found a matching element, now accumulate all matches
	for (; elem < end; ++elem) {
		if (comp(wordStart, *elem) || comp(*elem, wordStart))
			break;	// Not a match so stop
		// length of the word part (before the '(' brace) of the api array element
		const size_t wordlen = LengthWord(*elem, otherSeparator);
		if (!exactLen || (wordlen == wordStartLength)) {
			if (wordList.length() > 0)
				wordList.append(" ", 1);
			wordList.append(*elem, wordlen);
		}
	}
	return wordList;
}

/**
 * Returns elements (first words of them) of the StringList array which have
 * the same beginning as the passed string.
 * The length of the word to compare is passed too.
 * Letter case can be ignored or preserved (default).
 * If there are more words meeting the condition they are returned all of
 * them in the ascending order separated with spaces.
 */
std::string StringList::GetNearestWords(
	const char *wordStart,
	size_t searchLen,
	bool ignoreCase,
	char otherSeparator /*= '\0'*/,
	bool exactLen /*=false*/) {

	if (words.empty())
		return std::string();
	SortIfNeeded(ignoreCase);
	if (ignoreCase) {
		return GetMatches(wordsNoCase.begin(), wordsNoCase.end(), wordStart, otherSeparator, exactLen, CompareStringInsensitive(searchLen));
	} else {
		// Preserve the letter case
		return GetMatches(words.begin(), words.end(), wordStart, otherSeparator, exactLen, CompareString(searchLen));
	}
}

bool AutoCompleteWordList::Add(const std::string& word) {
	const auto [_, ok] = words.insert(word);
	if (ok) {
		const size_t length = word.length();
		totalLength += 1 + length;
		minWordLength = std::min(minWordLength, length);
		return true;
	}
	return ok;
}

std::string AutoCompleteWordList::Get() const {
	std::string result;
	if (totalLength) {
		result.reserve(totalLength + 2);
		result.push_back('\n');
		for (const std::string &word : words) {
			result.append(word);
			result.push_back('\n');
		}
	}
	return result;
}
