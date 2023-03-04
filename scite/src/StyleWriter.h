// SciTE - Scintilla based Text Editor
/** @file StyleWriter.h
 ** Simple buffered interface to the text and styles of a document held by Scintilla.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STYLEWRITER_H
#define STYLEWRITER_H

// Read only access to a document, its styles and other data
class TextReader {
protected:
	static constexpr Scintilla::Position extremePosition = INTPTR_MAX;
	/** @a bufferSize is a trade off between time taken to copy the characters
	 * and retrieval overhead.
	 * @a slopSize positions the buffer before the desired position
	 * in case there is some backtracking. */
	static constexpr Scintilla::Position bufferSize = 4000;
	static constexpr Scintilla::Position slopSize = bufferSize / 8;
	char buf[bufferSize+1];
	Scintilla::Position startPos;
	Scintilla::Position endPos;
	int codePage;

	Scintilla::ScintillaCall &sc;
	Scintilla::Position lenDoc;

	bool InternalIsLeadByte(char ch) const;
	void Fill(Scintilla::Position position);
public:
	explicit TextReader(Scintilla::ScintillaCall &sc_) noexcept;
	// Deleted so TextReader objects can not be copied.
	TextReader(const TextReader &source) = delete;
	TextReader &operator=(const TextReader &) = delete;
	char operator[](Scintilla::Position position) {
		if (position < startPos || position >= endPos) {
			Fill(position);
		}
		return buf[position - startPos];
	}
	/** Safe version of operator[], returning a defined value for invalid position. */
	char SafeGetCharAt(Scintilla::Position position, char chDefault=' ') {
		if (position < startPos || position >= endPos) {
			Fill(position);
			if (position < startPos || position >= endPos) {
				// Position is outside range of document
				return chDefault;
			}
		}
		return buf[position - startPos];
	}
	bool IsLeadByte(char ch) const {
		return codePage && InternalIsLeadByte(ch);
	}
	void SetCodePage(int codePage_) noexcept {
		codePage = codePage_;
	}
	bool Match(Scintilla::Position pos, const char *s);
	int StyleAt(Scintilla::Position position);
	Scintilla::Line GetLine(Scintilla::Position position);
	Scintilla::Position LineStart(Scintilla::Line line);
	Scintilla::FoldLevel LevelAt(Scintilla::Line line);
	Scintilla::Position Length();
	int GetLineState(Scintilla::Line line);
};

// Adds methods needed to write styles and folding
class StyleWriter : public TextReader {
protected:
	char styleBuf[bufferSize];
	Scintilla::Position validLen;
	Scintilla::Position startSeg;
public:
	explicit StyleWriter(Scintilla::ScintillaCall &sc_) noexcept;
	// Deleted so StyleWriter objects can not be copied.
	StyleWriter(const StyleWriter &source) = delete;
	StyleWriter &operator=(const StyleWriter &) = delete;
	void Flush();
	void SetLineState(Scintilla::Line line, int state);

	void StartAt(Scintilla::Position start);
	Scintilla::Position GetStartSegment() const noexcept { return startSeg; }
	void StartSegment(Scintilla::Position pos) noexcept;
	void ColourTo(Scintilla::Position pos, int chAttr);
	void SetLevel(Scintilla::Line line, Scintilla::FoldLevel level);
};

#endif
