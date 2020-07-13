// SciTE - Scintilla based Text Editor
/** @file PropSetFile.h
 ** Definition of platform independent base class of editor.
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PROPSETFILE_H
#define PROPSETFILE_H

/**
 */

typedef std::map<std::string, std::string> mapss;

class ImportFilter {
public:
	std::set<std::string> excludes;
	std::set<std::string> includes;
	void SetFilter(const std::string &sExcludes, const std::string &sIncludes);
	bool IsValid(const std::string &name) const;
};

class PropSetFile {
	bool lowerKeys;
	std::string GetWildUsingStart(const PropSetFile &psStart, const char *keybase, const char *filename);
	static bool caseSensitiveFilenames;
	mapss props;
public:
	PropSetFile *superPS;
	explicit PropSetFile(bool lowerKeys_=false);
	PropSetFile(const PropSetFile &copy);
	PropSetFile &operator=(const PropSetFile &assign);
	virtual ~PropSetFile();

	void Set(std::string_view key, std::string_view val);
	void SetLine(const char *keyVal);
	void Unset(std::string_view key);
	bool Exists(const char *key) const;
	std::string GetString(const char *key) const;
	std::string Evaluate(const char *key) const;
	std::string GetExpandedString(const char *key) const;
	std::string Expand(const std::string &withVars, int maxExpands=200) const;
	int GetInt(const char *key, int defaultValue=0) const;
	intptr_t GetInteger(const char *key, intptr_t defaultValue=0) const;
	long long GetLongLong(const char *key, long long defaultValue=0) const;
	void Clear() noexcept;

	enum ReadLineState { rlActive, rlExcludedModule, rlConditionFalse };
	ReadLineState ReadLine(const char *lineBuffer, ReadLineState rls, const FilePath &directoryForImports, const ImportFilter &filter,
			       FilePathSet *imports, size_t depth);
	void ReadFromMemory(const char *data, size_t len, const FilePath &directoryForImports, const ImportFilter &filter,
			    FilePathSet *imports, size_t depth);
	void Import(const FilePath &filename, const FilePath &directoryForImports, const ImportFilter &filter,
		    FilePathSet *imports, size_t depth);
	bool Read(const FilePath &filename, const FilePath &directoryForImports, const ImportFilter &filter,
		  FilePathSet *imports, size_t depth);
	std::string GetWild(const char *keybase, const char *filename);
	std::string GetNewExpandString(const char *keybase, const char *filename = "");
	bool GetFirst(const char *&key, const char *&val);
	bool GetNext(const char *&key, const char *&val);
	static void SetCaseSensitiveFilenames(bool caseSensitiveFilenames_) noexcept {
		caseSensitiveFilenames = caseSensitiveFilenames_;
	}
};

constexpr const char *extensionProperties = ".properties";
bool IsPropertiesFile(const FilePath &filename);

#endif
