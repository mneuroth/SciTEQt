// SciTE - Scintilla based Text Editor
/** @file EditorConfig.cxx
 ** Read and interpret settings files in the EditorConfig format.
 ** http://editorconfig.org/
 **/
// Copyright 2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cassert>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <chrono>

#include "GUI.h"

#include "StringHelpers.h"
#include "FilePath.h"
#include "EditorConfig.h"

namespace {

struct ECForDirectory {
	bool isRoot;
	std::string directory;
	std::vector<std::string> lines;
	ECForDirectory();
	void ReadOneDirectory(const FilePath &dir);
};

class EditorConfig : public IEditorConfig {
	std::vector<ECForDirectory> config;
public:
	~EditorConfig() override;
	void ReadFromDirectory(const FilePath &dirStart) override;
	std::map<std::string, std::string> MapFromAbsolutePath(const FilePath &absolutePath) const override;
	void Clear() noexcept override;
};

const GUI::gui_char editorConfigName[] = GUI_TEXT(".editorconfig");

bool PatternMatch(std::u32string_view pattern, std::u32string_view text) {
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
					return PatternMatch(pattern, text);
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
		pattern.remove_prefix(1);
		text.remove_prefix(1);
		return PatternMatch(pattern, text);
	} else if (pattern.front() == text.front()) {
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
		while (!pattern.empty() && pattern.front() != ']') {
			if (pattern.front() == text.front()) {
				inSet = true;
			}
			pattern.remove_prefix(1);
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
		pattern.remove_prefix(1);
		if (pattern.empty()) {
			return false;
		}
		std::u32string_view textAlt = text;
		bool inSet = false;
		size_t altSuccessLen = 0;
		while (!pattern.empty()) {
			if (pattern.front() == '}' || pattern.front() == ',') {
				inSet = true;
				text.remove_prefix(altSuccessLen);
				break;
			}
			if (textAlt.empty()) {
				return false;
			}
			if (pattern.front() == textAlt.front()) {
				pattern.remove_prefix(1);
				textAlt.remove_prefix(1);
				altSuccessLen++;
			} else {
				while (!pattern.empty() && pattern.front() != '}' && pattern.front() != ',') {
					pattern.remove_prefix(1);
				}
				if (!pattern.empty() && pattern.front() == ',') {
					pattern.remove_prefix(1);
				}
				textAlt = text;
				altSuccessLen = 0;
			}
		}
		if (!inSet) {
			return false;
		}
		while (!pattern.empty() && pattern.front() != '}') {
			pattern.remove_prefix(1);
		}
		if (!pattern.empty()) {
			pattern.remove_prefix(1);
		}
		return PatternMatch(pattern, text);
	}
	return false;
}

}

ECForDirectory::ECForDirectory() : isRoot(false) {
}

void ECForDirectory::ReadOneDirectory(const FilePath &dir) {
	directory = dir.AsUTF8();
	directory.append("/");
	FilePath fpec(dir, editorConfigName);
	std::string configString = fpec.Read();
	if (configString.size() > 0) {
		const std::string_view svUtf8BOM(UTF8BOM);
		if (StartsWith(configString, svUtf8BOM)) {
			configString.erase(0, svUtf8BOM.length());
		}
		// Carriage returns aren't wanted
		Remove(configString, std::string("\r"));
		std::vector<std::string> configLines = StringSplit(configString, '\n');
		for (std::string &line : configLines) {
			if (line.empty() || StartsWith(line, "#") || StartsWith(line, ";")) {
				// Drop comments
			} else if (StartsWith(line, "[")) {
				// Pattern
				lines.push_back(line);
			} else if (Contains(line, '=')) {
				LowerCaseAZ(line);
				Remove(line, std::string(" "));
				lines.push_back(line);
				std::vector<std::string> nameVal = StringSplit(line, '=');
				if (nameVal.size() == 2) {
					if ((nameVal[0] == "root") && nameVal[1] == "true") {
						isRoot = true;
					}
				}
			}
		}
	}
}

EditorConfig::~EditorConfig() = default;

void EditorConfig::ReadFromDirectory(const FilePath &dirStart) {
	FilePath dir = dirStart;
	while (true) {
		ECForDirectory ecfd;
		ecfd.ReadOneDirectory(dir);
		config.insert(config.begin(), ecfd);
		if (ecfd.isRoot || !dir.IsSet() || dir.IsRoot()) {
			break;
		}
		// Up a level
		dir = dir.Directory();
	}
}

std::map<std::string, std::string> EditorConfig::MapFromAbsolutePath(const FilePath &absolutePath) const {
	std::map<std::string, std::string> ret;
	std::string fullPath = absolutePath.AsUTF8();
#ifdef WIN32
	// Convert Windows path separators to Unix
	std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
#endif
	for (const ECForDirectory &level : config) {
		std::string relPath;
		if (level.directory.length() <= fullPath.length()) {
			relPath = fullPath.substr(level.directory.length());
		}
		bool inActiveSection = false;
		for (auto line : level.lines) {
			if (StartsWith(line, "[")) {
				std::string pattern = line.substr(1, line.size() - 2);
				if (!FilePath::CaseSensitive()) {
					pattern = GUI::LowerCaseUTF8(pattern);
					relPath = GUI::LowerCaseUTF8(relPath);
				}
				if ((pattern.find('/') == std::string::npos) && (relPath.find('/') != std::string::npos)) {
					// Simple pattern without directories so make match in any directory
					pattern.insert(0, "**/");
				}
				// Convert to u32string to treat as characters, not bytes
				std::u32string patternU32 = UTF32FromUTF8(pattern);
				std::u32string relPathU32 = UTF32FromUTF8(relPath);
				inActiveSection = PatternMatch(patternU32, relPathU32);
				// PatternMatch only works with literal filenames, '?', '*', '**', '[]', '[!]', '{,}', '\x'.
				// Other formats not yet handled:
				//   {num1..num2}
			} else if (inActiveSection && Contains(line, '=')) {
				const std::vector<std::string> nameVal = StringSplit(line, '=');
				if (nameVal.size() == 2) {
					if (nameVal[1] == "unset") {
						std::map<std::string, std::string>::iterator it = ret.find(nameVal[0]);
						if (it != ret.end())
							ret.erase(it);
					} else {
						ret[nameVal[0]] = nameVal[1];
					}
				}
			}
		}
	}

	// Install defaults for indentation/tab

	// if indent_style == "tab" and !indent_size: indent_size = "tab"
	if (ret.count("indent_style") && ret["indent_style"] == "tab" && !ret.count("indent_size")) {
		ret["indent_size"] = "tab";
	}

	// if indent_size != "tab" and !tab_width: tab_width = indent_size
	if (ret.count("indent_size") && ret["indent_size"] != "tab" && !ret.count("tab_width")) {
		ret["tab_width"] = ret["indent_size"];
	}

	// if indent_size == "tab": indent_size = tab_width
	if (ret.count("indent_size") && ret["indent_size"] == "tab" && ret.count("tab_width")) {
		ret["indent_size"] = ret["tab_width"];
	}

	return ret;
}

void EditorConfig::Clear() noexcept {
	config.clear();
}

#if defined(TESTING)

static void TestPatternMatch() {
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

	// ? matches one character
	assert(PatternMatch(U"?", U"a"));
	assert(PatternMatch(U"?", U"") == false);
	assert(PatternMatch(U"a?c", U"abc"));

	// [set] matches one character from set
	assert(PatternMatch(U"a[123]z", U"a2z"));
	assert(PatternMatch(U"a[123]z", U"az") == false);
	assert(PatternMatch(U"a[123]z", U"a2") == false);

	// [!set] matches one character not from set
	assert(PatternMatch(U"a[!123]z", U"ayz"));
	assert(PatternMatch(U"a[!123]", U"az"));
	assert(PatternMatch(U"a[!123]", U"a2") == false);

	// ** matches anything including '/'
	assert(PatternMatch(U"**a", U"a"));
	assert(PatternMatch(U"**a", U"za"));
	assert(PatternMatch(U"**a", U"yza"));
	assert(PatternMatch(U"**a", U"xyz") == false);
	assert(PatternMatch(U"a**z", U"a/z"));
	assert(PatternMatch(U"a**z", U"a/b/z"));
	assert(PatternMatch(U"a**", U"a/b/z"));

	// {alt1,alt2,...} matches any of the alternatives
	assert(PatternMatch(U"<{ab}>", U"<ab>"));
	assert(PatternMatch(U"<{ab,lm,xyz}>", U"<ab>"));
	assert(PatternMatch(U"<{ab,lm,xyz}>", U"<lm>"));
	assert(PatternMatch(U"<{ab,lm,xyz}>", U"<xyz>"));
	assert(PatternMatch(U"<{ab,lm,xyz}>", U"<rs>") == false);
}

#endif

std::unique_ptr<IEditorConfig> IEditorConfig::Create() {
#if defined(TESTING)
	TestPatternMatch();
#endif
	return std::make_unique<EditorConfig>();
}
