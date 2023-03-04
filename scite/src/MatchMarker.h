// SciTE - Scintilla based Text Editor
/** @file MatchMarker.h
 ** Mark all the matches of a string.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef MATCHMARKER_H
#define MATCHMARKER_H

struct LineRange {
	Scintilla::Line lineStart;
	Scintilla::Line lineEnd;
	LineRange(Scintilla::Line lineStart_, Scintilla::Line lineEnd_) noexcept : lineStart(lineStart_), lineEnd(lineEnd_) {}
};

std::vector<LineRange> LinesBreak(Scintilla::ScintillaCall *pSci);

class MatchMarker {
	Scintilla::ScintillaCall *pSci;
	std::string textMatch;
	int styleMatch;
	Scintilla::FindOption flagsMatch;
	int indicator;
	int bookMark;
	std::optional<Scintilla::Line> showContext;
	std::vector<LineRange> lineRanges;
	std::set<Scintilla::Line> matches;
public:
	MatchMarker();	// Not noexcept as std::vector constructor throws
	void StartMatch(Scintilla::ScintillaCall *pSci_,
			const std::string &textMatch_, Scintilla::FindOption flagsMatch_, int styleMatch_,
			int indicator_, int bookMark_, std::optional<Scintilla::Line> showContext_={});
	bool Complete() const noexcept;
	void Continue();
	void Stop() noexcept;
};

#endif
