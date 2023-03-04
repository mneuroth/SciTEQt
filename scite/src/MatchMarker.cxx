// SciTE - Scintilla based Text Editor
/** @file MatchMarker.cxx
 ** Mark all the matches of a string.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <optional>
#include <algorithm>
#include <chrono>

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

#include "GUI.h"

#include "MatchMarker.h"

namespace SA = Scintilla;

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
			     int indicator_, int bookMark_, std::optional<Scintilla::Line> showContext_) {
	lineRanges.clear();
	pSci = pSci_;
	textMatch = textMatch_;
	flagsMatch = flagsMatch_;
	styleMatch = styleMatch_;
	indicator = indicator_;
	bookMark = bookMark_;
	showContext = showContext_;
	lineRanges = LinesBreak(pSci);
	matches.clear();
	// Perform the initial marking immediately to avoid flashing
	Continue();
}

bool MatchMarker::Complete() const noexcept {
	return lineRanges.empty();
}

void MatchMarker::Continue() {
	constexpr int segment = 2000;

	// Remove old indicators if any exist.
	pSci->SetIndicatorCurrent(indicator);

	const LineRange rangeSearch = lineRanges[0];
	SA::Line lineEndSegment = rangeSearch.lineStart + segment;
	if (lineEndSegment > rangeSearch.lineEnd)
		lineEndSegment = rangeSearch.lineEnd;

	pSci->SetSearchFlags(flagsMatch);
	const SA::Position positionStart = pSci->LineStart(rangeSearch.lineStart);
	const SA::Position positionEnd = pSci->LineStart(lineEndSegment);
	pSci->SetTarget(SA::Span(positionStart, positionEnd));
	pSci->IndicatorClearRange(positionStart, positionEnd - positionStart);

	//Monitor the amount of time took by the search.
	GUI::ElapsedTime searchElapsedTime;

	// Find the first occurrence of word.
	SA::Span rangeFound = pSci->SpanSearchInTarget(textMatch);
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
			const SA::Line line = pSci->LineFromPosition(rangeFound.start);
			if ((bookMark >= 0) && (showContext != 0)) {
				pSci->MarkerAdd(line, bookMark);
			}
			if (showContext >= 0) {
				matches.insert(line);
			}
		}
		if (rangeFound.Length() == 0) {
			// Empty matches are possible for regex
			rangeFound.end = pSci->PositionAfter(rangeFound.end);
		}
		// Try to find next occurrence of word.
		pSci->SetTarget(SA::Span(rangeFound.end, positionEnd));
		rangeFound = pSci->SpanSearchInTarget(textMatch);
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
	if (lineRanges.empty() && showContext) {
		// Hide / show lines so that matches and their context are visible
		// Could do this incrementally but there are problems near segment edges
		const SA::Line lineCount = pSci->LineCount();
		std::vector<bool> visible(lineCount);
		for (const SA::Line line : matches) {
			for (SA::Line context = line - *showContext; context <= line + *showContext; context++) {
				if (context >= 0 && context < lineCount) {
					visible[context] = true;
				}
			}
		}
		// Batch up show to minimize calls
		// Seems a bit smoother to show/hide each group instead of
		// hiding all then showing groups even though that means more calls
		SA::Line startGroup = 0;
		bool state = true;
		for (SA::Line line = 0; line < lineCount; line++) {
			if (state != visible[line]) {
				if (state) {
					pSci->ShowLines(startGroup, line - 1);
				} else {
					pSci->HideLines(startGroup, line - 1);
				}
				startGroup = line;
				state = visible[line];
			}
		}
		if (state) {
			pSci->ShowLines(startGroup, lineCount - 1);
		} else {
			pSci->HideLines(startGroup, lineCount - 1);
		}
	}
}

void MatchMarker::Stop() noexcept {
	pSci = nullptr;
	lineRanges.clear();
}
