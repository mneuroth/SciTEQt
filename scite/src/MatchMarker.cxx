// SciTE - Scintilla based Text Editor
/** @file MatchMarker.cxx
 ** Mark all the matches of a string.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string>
#include <string_view>
#include <vector>
#include <chrono>

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

#include "GUI.h"

#include "MatchMarker.h"

namespace SA = Scintilla::API;

std::vector<LineRange> LinesBreak(SA::ScintillaCall *pSci) {
	std::vector<LineRange> lineRanges;
	if (pSci) {
		const SA::Line lineEnd = pSci->LineCount();
		const SA::Line lineStartVisible = pSci->FirstVisibleLine();
		const SA::Line docLineStartVisible = pSci->DocLineFromVisible(lineStartVisible);
		const SA::Line linesOnScreen = pSci->LinesOnScreen();
		constexpr SA::Line surround = 40;
		LineRange rangePriority(docLineStartVisible - surround, docLineStartVisible + linesOnScreen + surround);
		if (rangePriority.lineStart < 0)
			rangePriority.lineStart = 0;
		if (rangePriority.lineEnd > lineEnd)
			rangePriority.lineEnd = lineEnd;
		lineRanges.push_back(rangePriority);
		if (rangePriority.lineEnd < lineEnd)
			lineRanges.emplace_back(rangePriority.lineEnd, lineEnd);
		if (rangePriority.lineStart > 0)
			lineRanges.emplace_back(0, rangePriority.lineStart);
	}
	return lineRanges;
}

MatchMarker::MatchMarker() :
	pSci(nullptr), styleMatch(-1), flagsMatch(static_cast<SA::FindOption>(0)), indicator(0), bookMark(-1) {
}

void MatchMarker::StartMatch(SA::ScintillaCall *pSci_,
			     const std::string &textMatch_, SA::FindOption flagsMatch_, int styleMatch_,
			     int indicator_, int bookMark_) {
	lineRanges.clear();
	pSci = pSci_;
	textMatch = textMatch_;
	flagsMatch = flagsMatch_;
	styleMatch = styleMatch_;
	indicator = indicator_;
	bookMark = bookMark_;
	lineRanges = LinesBreak(pSci);
	// Perform the initial marking immediately to avoid flashing
	Continue();
}

bool MatchMarker::Complete() const noexcept {
	return lineRanges.empty();
}

void MatchMarker::Continue() {
	constexpr int segment = 200;

	// Remove old indicators if any exist.
	pSci->SetIndicatorCurrent(indicator);

	const LineRange rangeSearch = lineRanges[0];
	SA::Line lineEndSegment = rangeSearch.lineStart + segment;
	if (lineEndSegment > rangeSearch.lineEnd)
		lineEndSegment = rangeSearch.lineEnd;

	pSci->SetSearchFlags(flagsMatch);
	const SA::Position positionStart = pSci->LineStart(rangeSearch.lineStart);
	const SA::Position positionEnd = pSci->LineStart(lineEndSegment);
	pSci->SetTarget(SA::Range(positionStart, positionEnd));
	pSci->IndicatorClearRange(positionStart, positionEnd - positionStart);

	//Monitor the amount of time took by the search.
	GUI::ElapsedTime searchElapsedTime;

	// Find the first occurrence of word.
	SA::Range rangeFound = pSci->RangeSearchInTarget(textMatch);
	while (rangeFound.start >= 0) {
		// Limit the search duration to 250 ms. Avoid to freeze editor for huge lines.
		if (searchElapsedTime.Duration() > 0.25) {
			// Clear all indicators because timer has expired.
			pSci->IndicatorClearRange(0, pSci->Length());
			lineRanges.clear();
			break;
		}

		if ((styleMatch < 0) || (styleMatch == pSci->UnsignedStyleAt(rangeFound.start))) {
			pSci->IndicatorFillRange(rangeFound.start, rangeFound.Length());
			if (bookMark >= 0) {
				pSci->MarkerAdd(
					pSci->LineFromPosition(rangeFound.start), bookMark);
			}
		}
		if (rangeFound.Length() == 0) {
			// Empty matches are possible for regex
			rangeFound.end = pSci->PositionAfter(rangeFound.end);
		}
		// Try to find next occurrence of word.
		pSci->SetTarget(SA::Range(rangeFound.end, positionEnd));
		rangeFound = pSci->RangeSearchInTarget(textMatch);
	}

	// Retire searched lines
	if (!lineRanges.empty()) {
		// Check in case of re-entrance
		if (lineEndSegment >= rangeSearch.lineEnd) {
			lineRanges.erase(lineRanges.begin());
		} else {
			lineRanges[0].lineStart = lineEndSegment;
		}
	}
}

void MatchMarker::Stop() noexcept {
	pSci = nullptr;
	lineRanges.clear();
}
