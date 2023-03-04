// SciTE - Scintilla based Text Editor
/** @file EditorConfig.cxx
 ** Read and interpret settings files in the EditorConfig format.
 ** http://editorconfig.org/
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
	void ReadFromDirectory(const FilePath &dirStart) override;
	std::map<std::string, std::string> MapFromAbsolutePath(const FilePath &absolutePath) const override;
	void Clear() noexcept override;
};

const GUI::gui_char editorConfigName[] = GUI_TEXT(".editorconfig");

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
#if defined(_WIN32)
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
				const std::string pattern = line.substr(1, line.size() - 2);
				inActiveSection = PathMatch(pattern, relPath);
				// PatternMatch only works with literal filenames, '?', '*', '**', '[]', '[!]', '{,}', '{..}', '\x'.
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

std::unique_ptr<IEditorConfig> IEditorConfig::Create() {
	return std::make_unique<EditorConfig>();
}
