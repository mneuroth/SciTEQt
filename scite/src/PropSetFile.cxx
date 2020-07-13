// SciTE - Scintilla based Text Editor
/** @file PropSetFile.cxx
 ** Property set implementation.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <ctime>

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <chrono>
#include <sstream>

#include <fcntl.h>

#if defined(GTK)

#include <unistd.h>

#endif

#include "GUI.h"

#include "StringHelpers.h"
#include "FilePath.h"
#include "PropSetFile.h"

// The comparison and case changing functions here assume ASCII
// or extended ASCII such as the normal Windows code page.

static std::set<std::string> FilterFromString(const std::string &values) {
	std::vector<std::string> vsFilter = StringSplit(values, ' ');
	std::set<std::string> fs;
	for (std::vector<std::string>::const_iterator it=vsFilter.begin(); it != vsFilter.end(); ++it) {
		if (!it->empty())
			fs.insert(*it);
	}
	return fs;
}

void ImportFilter::SetFilter(const std::string &sExcludes, const std::string &sIncludes) {
	excludes = FilterFromString(sExcludes);
	includes = FilterFromString(sIncludes);
}

bool ImportFilter::IsValid(const std::string &name) const {
	if (!includes.empty()) {
		return includes.count(name) > 0;
	} else {
		return excludes.count(name) == 0;
	}
}

bool PropSetFile::caseSensitiveFilenames = false;

PropSetFile::PropSetFile(bool lowerKeys_) : lowerKeys(lowerKeys_), superPS(nullptr) {
}

PropSetFile::PropSetFile(const PropSetFile &copy) : lowerKeys(copy.lowerKeys), props(copy.props), superPS(copy.superPS) {
}

PropSetFile::~PropSetFile() {
	superPS = nullptr;
	Clear();
}

PropSetFile &PropSetFile::operator=(const PropSetFile &assign) {
	if (this != &assign) {
		lowerKeys = assign.lowerKeys;
		superPS = assign.superPS;
		props = assign.props;
	}
	return *this;
}

void PropSetFile::Set(std::string_view key, std::string_view val) {
	if (key.empty())	// Empty keys are not supported
		return;
	props[std::string(key)] = std::string(val);
}

void PropSetFile::SetLine(const char *keyVal) {
	while (IsASpace(*keyVal))
		keyVal++;
	const char *endVal = keyVal;
	while (*endVal && (*endVal != '\n'))
		endVal++;
	const char *eqAt = strchr(keyVal, '=');
	if (eqAt) {
		const char *pKeyEnd = eqAt - 1;
		while ((pKeyEnd >= keyVal) && IsASpace(*pKeyEnd)) {
			--pKeyEnd;
		}
		const ptrdiff_t lenVal = endVal - eqAt - 1;
		const ptrdiff_t lenKey = pKeyEnd - keyVal + 1;
		Set(std::string_view(keyVal, lenKey), std::string_view(eqAt + 1, lenVal));
	} else if (*keyVal) {	// No '=' so assume '=1'
		Set(keyVal, "1");
	}
}

void PropSetFile::Unset(std::string_view key) {
	if (key.empty())	// Empty keys are not supported
		return;
	mapss::iterator keyPos = props.find(std::string(key));
	if (keyPos != props.end())
		props.erase(keyPos);
}

bool PropSetFile::Exists(const char *key) const {
	mapss::const_iterator keyPos = props.find(std::string(key));
	if (keyPos != props.end()) {
		return true;
	} else {
		if (superPS) {
			// Failed here, so try in base property set
			return superPS->Exists(key);
		} else {
			return false;
		}
	}
}

std::string PropSetFile::GetString(const char *key) const {
	const std::string sKey(key);
	const PropSetFile *psf = this;
	while (psf) {
		mapss::const_iterator keyPos = psf->props.find(sKey);
		if (keyPos != psf->props.end()) {
			return keyPos->second;
		}
		// Failed here, so try in base property set
		psf = psf->superPS;
	}
	return "";
}

static std::string ShellEscape(const char *toEscape) {
	std::string str(toEscape);
	for (ptrdiff_t i = str.length()-1; i >= 0; --i) {
		switch (str[i]) {
		case ' ':
		case '|':
		case '&':
		case ',':
		case '`':
		case '"':
		case ';':
		case ':':
		case '!':
		case '^':
		case '$':
		case '{':
		case '}':
		case '(':
		case ')':
		case '[':
		case ']':
		case '=':
		case '<':
		case '>':
		case '\\':
		case '\'':
			str.insert(i, "\\");
			break;
		default:
			break;
		}
	}
	return str;
}

std::string PropSetFile::Evaluate(const char *key) const {
	if (strchr(key, ' ')) {
		if (isprefix(key, "escape ")) {
			std::string val = GetString(key+7);
			std::string escaped = ShellEscape(val.c_str());
			return escaped;
		} else if (isprefix(key, "= ")) {
			const std::string sExpressions(key + 2);
			std::vector<std::string> parts = StringSplit(sExpressions, ';');
			if (parts.size() > 1) {
				bool equal = true;
				for (size_t part = 1; part < parts.size(); part++) {
					if (parts[part] != parts[0]) {
						equal = false;
					}
				}
				return equal ? "1" : "0";
			}
		} else if (isprefix(key, "star ")) {
			const std::string sKeybase(key + 5);
			// Create set of variables with values
			mapss values;
			// For this property set and all base sets
			for (const PropSetFile *psf = this; psf; psf = psf->superPS) {
				mapss::const_iterator it = psf->props.lower_bound(sKeybase);
				while ((it != psf->props.end()) && (it->first.find(sKeybase) == 0)) {
					mapss::iterator itDestination = values.find(it->first);
					if (itDestination == values.end()) {
						// Not present so add
						values[it->first] = it->second;
					}
					++it;
				}
			}
			// Concatenate all variables
			std::string combination;
			for (mapss::const_iterator itV = values.begin(); itV != values.end(); ++itV) {
				combination += itV->second;
			}
			return combination;
		} else if (isprefix(key, "scale ")) {
			const int scaleFactor = GetInt("ScaleFactor", 100);
			const char *val = key + 6;
			if (scaleFactor == 100) {
				return val;
			} else {
				const int value = atoi(val);
				return StdStringFromInteger(value * scaleFactor / 100);
			}
		}
	} else {
		return GetString(key);
	}
	return "";
}

// There is some inconsistency between GetExpanded("foo") and Expand("$(foo)").
// A solution is to keep a stack of variables that have been expanded, so that
// recursive expansions can be skipped.  For now I'll just use the C++ stack
// for that, through a recursive function and a simple chain of pointers.

struct VarChain {
	VarChain(const char *var_=nullptr, const VarChain *link_=nullptr) noexcept : var(var_), link(link_) {}

	bool contains(const char *testVar) const {
		return (var && (0 == strcmp(var, testVar)))
		       || (link && link->contains(testVar));
	}

	const char *var;
	const VarChain *link;
};

static int ExpandAllInPlace(const PropSetFile &props, std::string &withVars, int maxExpands, const VarChain &blankVars = VarChain()) {
	size_t varStart = withVars.find("$(");
	while ((varStart != std::string::npos) && (maxExpands > 0)) {
		const size_t varEnd = withVars.find(')', varStart+2);
		if (varEnd == std::string::npos) {
			break;
		}

		// For consistency, when we see '$(ab$(cde))', expand the inner variable first,
		// regardless whether there is actually a degenerate variable named 'ab$(cde'.
		size_t innerVarStart = withVars.find("$(", varStart+2);
		while ((innerVarStart != std::string::npos) && (innerVarStart < varEnd)) {
			varStart = innerVarStart;
			innerVarStart = withVars.find("$(", varStart+2);
		}

		std::string var(withVars.c_str(), varStart + 2, varEnd - (varStart + 2));
		std::string val = props.Evaluate(var.c_str());

		if (blankVars.contains(var.c_str())) {
			val.clear(); // treat blankVar as an empty string (e.g. to block self-reference)
		}

		if (--maxExpands >= 0) {
			maxExpands = ExpandAllInPlace(props, val, maxExpands, VarChain(var.c_str(), &blankVars));
		}

		withVars.erase(varStart, varEnd-varStart+1);
		withVars.insert(varStart, val);

		varStart = withVars.find("$(");
	}

	return maxExpands;
}

std::string PropSetFile::GetExpandedString(const char *key) const {
	std::string val = GetString(key);
	ExpandAllInPlace(*this, val, 200, VarChain(key));
	return val;
}

std::string PropSetFile::Expand(const std::string &withVars, int maxExpands) const {
	std::string val = withVars;
	ExpandAllInPlace(*this, val, maxExpands);
	return val;
}

int PropSetFile::GetInt(const char *key, int defaultValue) const {
	return IntegerFromString(GetExpandedString(key), defaultValue);
}

intptr_t PropSetFile::GetInteger(const char *key, intptr_t defaultValue) const {
	return IntPtrFromString(GetExpandedString(key), defaultValue);
}

long long PropSetFile::GetLongLong(const char *key, long long defaultValue) const {
	return LongLongFromString(GetExpandedString(key), defaultValue);
}

void PropSetFile::Clear() noexcept {
	props.clear();
}

/**
 * Get a line of input. If end of line escaped with '\\' then continue reading.
 */
static bool GetFullLine(const char *&fpc, size_t &lenData, char *s, size_t len) noexcept {
	bool continuation = true;
	s[0] = '\0';
	while ((len > 1) && lenData > 0) {
		char ch = *fpc;
		fpc++;
		lenData--;
		if ((ch == '\r') || (ch == '\n')) {
			if (!continuation) {
				if ((lenData > 0) && (ch == '\r') && ((*fpc) == '\n')) {
					// munch the second half of a crlf
					fpc++;
					lenData--;
				}
				*s = '\0';
				return true;
			}
		} else if ((ch == '\\') && (lenData > 0) && ((*fpc == '\r') || (*fpc == '\n'))) {
			continuation = true;
			if ((lenData > 1) && (((*fpc == '\r') && (*(fpc+1) == '\r')) || ((*fpc == '\n') && (*(fpc+1) == '\n'))))
				continuation = false;
			else if ((lenData > 2) && ((*fpc == '\r') && (*(fpc+1) == '\n') && (*(fpc+2) == '\n' || *(fpc+2) == '\r')))
				continuation = false;
		} else {
			continuation = false;
			*s++ = ch;
			*s = '\0';
			len--;
		}
	}
	return false;
}

static bool IsSpaceOrTab(char ch) noexcept {
	return (ch == ' ') || (ch == '\t');
}

static bool IsCommentLine(const char *line) noexcept {
	while (IsSpaceOrTab(*line)) ++line;
	return (*line == '#');
}

static bool GenericPropertiesFile(const FilePath &filename) {
	std::string name = filename.BaseName().AsUTF8();
	if (name == "abbrev" || name == "Embedded")
		return true;
	return name.find("SciTE") != std::string::npos;
}

void PropSetFile::Import(const FilePath &filename, const FilePath &directoryForImports, const ImportFilter &filter,
			 FilePathSet *imports, size_t depth) {
	if (depth > 20)	// Possibly recursive import so give up to avoid crash
		return;
	if (Read(filename, directoryForImports, filter, imports, depth)) {
		if (imports && (std::find(imports->begin(), imports->end(), filename) == imports->end())) {
			imports->push_back(filename);
		}
	}
}

PropSetFile::ReadLineState PropSetFile::ReadLine(const char *lineBuffer, ReadLineState rls, const FilePath &directoryForImports,
		const ImportFilter &filter, FilePathSet *imports, size_t depth) {
	//UnSlash(lineBuffer);
	if ((rls == rlConditionFalse) && (!IsSpaceOrTab(lineBuffer[0])))    // If clause ends with first non-indented line
		rls = rlActive;
	if (isprefix(lineBuffer, "module ")) {
		std::string module = lineBuffer + strlen("module") + 1;
		if (module.empty() || filter.IsValid(module)) {
			rls = rlActive;
		} else {
			rls = rlExcludedModule;
		}
		return rls;
	}
	if (rls != rlActive) {
		return rls;
	}
	if (isprefix(lineBuffer, "if ")) {
		const char *expr = lineBuffer + strlen("if") + 1;
		std::string value = Expand(expr);
		if (value == "0" || value == "") {
			rls = rlConditionFalse;
		} else if (value == "1") {
			rls = rlActive;
		} else {
			rls = (GetInt(value.c_str()) != 0) ? rlActive : rlConditionFalse;
		}
	} else if (isprefix(lineBuffer, "import ")) {
		if (directoryForImports.IsSet()) {
			std::string importName(lineBuffer + strlen("import") + 1);
			if (importName == "*") {
				// Import all .properties files in this directory except for system properties
				FilePathSet directories;
				FilePathSet files;
				directoryForImports.List(directories, files);
				for (const FilePath &fpFile : files) {
					if (IsPropertiesFile(fpFile) &&
							!GenericPropertiesFile(fpFile) &&
							filter.IsValid(fpFile.BaseName().AsUTF8())) {
						FilePath importPath(directoryForImports, fpFile);
						Import(importPath, directoryForImports, filter, imports, depth + 1);
					}
				}
			} else if (filter.IsValid(importName)) {
				importName += ".properties";
				FilePath importPath(directoryForImports, FilePath(GUI::StringFromUTF8(importName)));
				Import(importPath, directoryForImports, filter, imports, depth + 1);
			}
		}
	} else if (!IsCommentLine(lineBuffer)) {
		SetLine(lineBuffer);
	}
	return rls;
}

void PropSetFile::ReadFromMemory(const char *data, size_t len, const FilePath &directoryForImports,
				 const ImportFilter &filter, FilePathSet *imports, size_t depth) {
	const char *pd = data;
	std::vector<char> lineBuffer(len+1);	// +1 for NUL
	ReadLineState rls = rlActive;
	while (len > 0) {
		GetFullLine(pd, len, &lineBuffer[0], lineBuffer.size());
		if (lowerKeys) {
			for (int i=0; lineBuffer[i] && (lineBuffer[i] != '='); i++) {
				if ((lineBuffer[i] >= 'A') && (lineBuffer[i] <= 'Z')) {
					lineBuffer[i] = static_cast<char>(lineBuffer[i] - 'A' + 'a');
				}
			}
		}
		rls = ReadLine(&lineBuffer[0], rls, directoryForImports, filter, imports, depth);
	}
}

bool PropSetFile::Read(const FilePath &filename, const FilePath &directoryForImports,
		       const ImportFilter &filter, FilePathSet *imports, size_t depth) {
	const std::string propsData = filename.Read();
	const size_t lenFile = propsData.size();
	if (lenFile > 0) {
		std::string_view data(propsData.c_str(), lenFile);
		const std::string_view svUtf8BOM(UTF8BOM);
		if (StartsWith(data, svUtf8BOM)) {
			data.remove_prefix(svUtf8BOM.length());
		}
		ReadFromMemory(data.data(), data.length(), directoryForImports, filter, imports, depth);
		return true;
	}
	return false;
}

namespace {

bool StringEqual(std::string_view a, std::string_view b, bool caseSensitive) noexcept {
	if (caseSensitive) {
		return a == b;
	} else {
		if (a.length() != b.length()) {
			return false;
		}
		for (size_t i = 0; i < a.length(); i++) {
			if (MakeUpperCase(a[i]) != MakeUpperCase(b[i]))
				return false;
		}
	}
	return true;
}

// Match file names to patterns allowing for '*' and '?'.

bool MatchWild(std::string_view pattern, std::string_view text, bool caseSensitive) {
	if (StringEqual(pattern, text, caseSensitive)) {
		return true;
	} else if (pattern.empty()) {
		return false;
	} else if (pattern.front() == '*') {
		pattern.remove_prefix(1);
		if (pattern.empty()) {
			return true;
		}
		while (!text.empty()) {
			if (MatchWild(pattern, text, caseSensitive)) {
				return true;
			}
			text.remove_prefix(1);
		}
	} else if (text.empty()) {
		return false;
	} else if (pattern.front() == '?') {
		pattern.remove_prefix(1);
		text.remove_prefix(1);
		return MatchWild(pattern, text, caseSensitive);
	} else if (caseSensitive && pattern.front() == text.front()) {
		pattern.remove_prefix(1);
		text.remove_prefix(1);
		return MatchWild(pattern, text, caseSensitive);
	} else if (!caseSensitive && MakeUpperCase(pattern.front()) == MakeUpperCase(text.front())) {
		pattern.remove_prefix(1);
		text.remove_prefix(1);
		return MatchWild(pattern, text, caseSensitive);
	}
	return false;
}

bool startswith(const std::string &s, const char *keybase) noexcept {
	return isprefix(s.c_str(), keybase);
}

}

std::string PropSetFile::GetWildUsingStart(const PropSetFile &psStart, const char *keybase, const char *filename) {
	const std::string sKeybase(keybase);
	const PropSetFile *psf = this;
	while (psf) {
		mapss::const_iterator it = psf->props.lower_bound(sKeybase);
		while ((it != psf->props.end()) && startswith(it->first, sKeybase.c_str())) {
			const std::string_view orgkeyfile = it->first.c_str() + sKeybase.length();
			std::string key;	// keyFile may point into key so key lifetime must cover keyFile
			std::string_view keyFile = orgkeyfile;

			if (orgkeyfile.find("$(") == 0) {
				// $(X) is a variable so extract X and find its value
				const size_t endVar = orgkeyfile.find_first_of(')');
				if (endVar != std::string_view::npos) {
					const std::string var(orgkeyfile.substr(2, endVar-2));
					key = psStart.GetExpandedString(var.c_str());
					keyFile = key;
				}
			}

			while (!keyFile.empty()) {
				const size_t sepPos = keyFile.find_first_of(';');
				const std::string_view pattern = keyFile.substr(0, sepPos);
				if (MatchWild(pattern, filename, caseSensitiveFilenames)) {
					return it->second;
				}
				// Move to next
				keyFile = (sepPos == std::string_view::npos) ? "" : keyFile.substr(sepPos + 1);
			}

			if (it->first == sKeybase) {
				return it->second;
			}
			++it;
		}
		// Failed here, so try in base property set
		psf = psf->superPS;
	}
	return "";
}

std::string PropSetFile::GetWild(const char *keybase, const char *filename) {
	return GetWildUsingStart(*this, keybase, filename);
}

// GetNewExpandString does not use Expand as it has to use GetWild with the filename for each
// variable reference found.

std::string PropSetFile::GetNewExpandString(const char *keybase, const char *filename) {
	std::string withVars = GetWild(keybase, filename);
	size_t varStart = withVars.find("$(");
	int maxExpands = 1000;	// Avoid infinite expansion of recursive definitions
	while ((varStart != std::string::npos) && (maxExpands > 0)) {
		const size_t varEnd = withVars.find(')', varStart+2);
		if (varEnd == std::string::npos) {
			break;
		}
		std::string var(withVars, varStart + 2, varEnd - varStart - 2);	// Subtract the $(
		std::string val = GetWild(var.c_str(), filename);
		if (var == keybase)
			val.clear(); // Self-references evaluate to empty string
		withVars.replace(varStart, varEnd - varStart + 1, val);
		varStart = withVars.find("$(");
		maxExpands--;
	}
	return withVars;
}

/**
 * Initiate enumeration.
 */
bool PropSetFile::GetFirst(const char *&key, const char *&val) {
	mapss::iterator it = props.begin();
	if (it != props.end()) {
		key = it->first.c_str();
		val = it->second.c_str();
		return true;
	} else {
		return false;
	}
}

/**
 * Continue enumeration.
 */
bool PropSetFile::GetNext(const char *&key, const char *&val) {
	mapss::iterator it = props.find(key);
	if (it != props.end()) {
		++it;
		if (it != props.end()) {
			key = it->first.c_str();
			val = it->second.c_str();
			return true;
		}
	}
	return false;
}

bool IsPropertiesFile(const FilePath &filename) {
	FilePath ext = filename.Extension();
	if (EqualCaseInsensitive(ext.AsUTF8().c_str(), extensionProperties + 1))
		return true;
	return false;
}
