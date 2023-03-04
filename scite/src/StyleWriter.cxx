// SciTE - Scintilla based Text Editor
/** @file StyleWriter.cxx
 ** Simple buffered interface to the text and styles of a document held by Scintilla.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdint>

#include <string>
#include <string_view>
#include <chrono>

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

#include "GUI.h"
#include "StyleWriter.h"

namespace SA = Scintilla;

TextReader::TextReader(SA::ScintillaCall &sc_) noexcept :
	startPos(extremePosition),
	endPos(0),
	codePage(0),
	sc(sc_),
	lenDoc(-1) {
	buf[0] = 0;
}

bool TextReader::InternalIsLeadByte(char ch) const {
	return GUI::IsDBCSLeadByte(codePage, ch);
}

void TextReader::Fill(SA::Position position) {
	if (lenDoc == -1)
		lenDoc = sc.Length();
	startPos = position - slopSize;
	if (startPos + bufferSize > lenDoc)
		startPos = lenDoc - bufferSize;
	if (startPos < 0)
		startPos = 0;
	endPos = startPos + bufferSize;
	if (endPos > lenDoc)
		endPos = lenDoc;
	sc.SetTarget(SA::Span(startPos, endPos));
	sc.TargetText(buf);
}

bool TextReader::Match(SA::Position pos, const char *s) {
	for (int i=0; *s; i++) {
		if (*s != SafeGetCharAt(pos+i))
			return false;
		s++;
	}
	return true;
}

int TextReader::StyleAt(SA::Position position) {
	return sc.UnsignedStyleAt(position);
}

SA::Line TextReader::GetLine(SA::Position position) {
	return sc.LineFromPosition(position);
}

SA::Position TextReader::LineStart(SA::Line line) {
	return sc.LineStart(line);
}

SA::FoldLevel TextReader::LevelAt(SA::Line line) {
	return sc.FoldLevel(line);
}

SA::Position TextReader::Length() {
	if (lenDoc == -1)
		lenDoc = sc.Length();
	return lenDoc;
}

int TextReader::GetLineState(SA::Line line) {
	return sc.LineState(line);
}

StyleWriter::StyleWriter(SA::ScintillaCall &sc_) noexcept :
	TextReader(sc_),
	validLen(0),
	startSeg(0) {
	styleBuf[0] = 0;
}

void StyleWriter::SetLineState(SA::Line line, int state) {
	sc.SetLineState(line, state);
}

void StyleWriter::StartAt(SA::Position start) {
	sc.StartStyling(start, 0);
}

void StyleWriter::StartSegment(SA::Position pos) noexcept {
	startSeg = pos;
}

void StyleWriter::ColourTo(SA::Position pos, int chAttr) {
	// Only perform styling if non empty range
	if (pos != startSeg - 1) {
		if (validLen + (pos - startSeg + 1) >= bufferSize)
			Flush();
		if (validLen + (pos - startSeg + 1) >= bufferSize) {
			// Too big for buffer so send directly
			sc.SetStyling(pos - startSeg + 1, chAttr);
		} else {
			for (SA::Position i = startSeg; i <= pos; i++) {
				styleBuf[validLen++] = static_cast<char>(chAttr);
			}
		}
	}
	startSeg = pos+1;
}

void StyleWriter::SetLevel(SA::Line line, SA::FoldLevel level) {
	sc.SetFoldLevel(line, level);
}

void StyleWriter::Flush() {
	startPos = extremePosition;
	lenDoc = -1;
	if (validLen > 0) {
		sc.SetStylingEx(validLen, styleBuf);
		validLen = 0;
	}
}
