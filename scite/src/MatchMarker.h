// SciTE - Scintilla based Text Editor
/** @file MatchMarker.h
 ** Mark all the matches of a string.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef MATCHMARKER_H
#define MATCHMARKER_H

struct LineRange {
	Scintilla::API::Line lineStart;
	Scintilla::API::Line lineEnd;
	LineRange(Scintilla::API::Line lineStart_, Scintilla::API::Line lineEnd_) noexcept : lineStart(lineStart_), lineEnd(lineEnd_) {}
};

std::vector<LineRange> LinesBreak(Scintilla::API::ScintillaCall *pSci);

class MatchMarker {
	Scintilla::API::ScintillaCall *pSci;
	std::string textMatch;
	int styleMatch;
	Scintilla::API::FindOption flagsMatch;
	int indicator;
	int bookMark;
	std::vector<LineRange> lineRanges;
public:
	MatchMarker();	// Not noexcept as std::vector constructor throws
	void StartMatch(Scintilla::API::ScintillaCall *pSci_,
			const std::string &textMatch_, Scintilla::API::FindOption flagsMatch_, int styleMatch_,
			int indicator_, int bookMark_);
	bool Complete() const noexcept;
	void Continue();
	void Stop() noexcept;
};

#endif
