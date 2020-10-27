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
	static constexpr Scintilla::API::Position extremePosition = INTPTR_MAX;
	/** @a bufferSize is a trade off between time taken to copy the characters
	 * and retrieval overhead.
	 * @a slopSize positions the buffer before the desired position
	 * in case there is some backtracking. */
	static constexpr Scintilla::API::Position bufferSize = 4000;
	static constexpr Scintilla::API::Position slopSize = bufferSize / 8;
	char buf[bufferSize+1];
	Scintilla::API::Position startPos;
	Scintilla::API::Position endPos;
	int codePage;

	Scintilla::API::ScintillaCall &sc;
	Scintilla::API::Position lenDoc;

	bool InternalIsLeadByte(char ch) const;
	void Fill(Scintilla::API::Position position);
public:
	explicit TextReader(Scintilla::API::ScintillaCall &sc_) noexcept;
	// Deleted so TextReader objects can not be copied.
	TextReader(const TextReader &source) = delete;
	TextReader &operator=(const TextReader &) = delete;
	char operator[](Scintilla::API::Position position) {
		if (position < startPos || position >= endPos) {
			Fill(position);
		}
		return buf[position - startPos];
	}
	/** Safe version of operator[], returning a defined value for invalid position. */
	char SafeGetCharAt(Scintilla::API::Position position, char chDefault=' ') {
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
	bool Match(Scintilla::API::Position pos, const char *s);
	int StyleAt(Scintilla::API::Position position);
	Scintilla::API::Line GetLine(Scintilla::API::Position position);
	Scintilla::API::Position LineStart(Scintilla::API::Line line);
	Scintilla::API::FoldLevel LevelAt(Scintilla::API::Line line);
	Scintilla::API::Position Length();
	int GetLineState(Scintilla::API::Line line);
};

// Adds methods needed to write styles and folding
class StyleWriter : public TextReader {
protected:
	char styleBuf[bufferSize];
	Scintilla::API::Position validLen;
	Scintilla::API::Position startSeg;
public:
	explicit StyleWriter(Scintilla::API::ScintillaCall &sc_) noexcept;
	// Deleted so StyleWriter objects can not be copied.
	StyleWriter(const StyleWriter &source) = delete;
	StyleWriter &operator=(const StyleWriter &) = delete;
	void Flush();
	void SetLineState(Scintilla::API::Line line, int state);

	void StartAt(Scintilla::API::Position start, char chMask=31);
	Scintilla::API::Position GetStartSegment() const noexcept { return startSeg; }
	void StartSegment(Scintilla::API::Position pos) noexcept;
	void ColourTo(Scintilla::API::Position pos, int chAttr);
	void SetLevel(Scintilla::API::Line line, Scintilla::API::FoldLevel level);
};

#endif
