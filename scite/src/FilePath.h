// SciTE - Scintilla based Text Editor
/** @file FilePath.h
 ** Definition of platform independent base class of editor.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef FILEPATH_H
#define FILEPATH_H

extern const GUI::gui_char pathSepString[];
extern const GUI::gui_char pathSepChar;
extern const GUI::gui_char listSepString[];
extern const GUI::gui_char configFileVisibilityString[];
extern const GUI::gui_char fileRead[];
extern const GUI::gui_char fileWrite[];

class FilePath;

typedef std::vector<FilePath> FilePathSet;

class FilePath {
	GUI::gui_string fileName;
public:
	FilePath() noexcept;
	FilePath(const GUI::gui_char *fileName_);
	FilePath(const GUI::gui_string &fileName_);
	FilePath(FilePath const &directory, FilePath const &name);
	FilePath(FilePath const &) = default;
	FilePath(FilePath &&) noexcept = default;
	FilePath &operator=(FilePath const &) = default;
	FilePath &operator=(FilePath &&) noexcept = default;
	virtual ~FilePath() = default;

	void Set(const GUI::gui_char *fileName_);
	void Set(FilePath const &other);
	void Set(FilePath const &directory, FilePath const &name);
	void SetDirectory(FilePath const &directory);
	virtual void Init() noexcept;
	bool SameNameAs(const GUI::gui_char *other) const noexcept;
	bool SameNameAs(const FilePath &other) const noexcept;
	bool operator==(const FilePath &other) const noexcept;
	bool operator<(const FilePath &other) const noexcept;
	bool IsSet() const noexcept;
	bool IsUntitled() const;
	bool IsAbsolute() const;
	bool IsRoot() const;
	static int RootLength() noexcept;
	const GUI::gui_char *AsInternal() const noexcept;
	std::string AsUTF8() const;
	FilePath Name() const;
	FilePath BaseName() const;
	FilePath Extension() const;
	FilePath Directory() const;
	void FixName();
	FilePath AbsolutePath() const;
	FilePath NormalizePath() const;
	static FilePath GetWorkingDirectory();
	bool SetWorkingDirectory() const noexcept;
	void List(FilePathSet &directories, FilePathSet &files) const;
	FILE *Open(const GUI::gui_char *mode) const noexcept;
	std::string Read() const;
	void Remove() const noexcept;
	time_t ModifiedTime() const;
	long long GetFileLength() const noexcept;
	bool Exists() const noexcept;
	bool IsDirectory() const noexcept;
	bool Matches(const GUI::gui_char *pattern) const;
	static bool CaseSensitive() noexcept;
};

std::string CommandExecute(const GUI::gui_char *command, const GUI::gui_char *directoryForRun);

#endif
