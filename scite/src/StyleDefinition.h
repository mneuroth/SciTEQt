// SciTE - Scintilla based Text Editor
/** @file StyleDefinition.h
 ** Definition of style aggregate and helper functions.
 **/
// Copyright 2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STYLEDEFINITION_H
#define STYLEDEFINITION_H

class StyleDefinition {
public:
	std::string font;
	float sizeFractional;
	int size;
	std::string fore;
	std::string back;
	Scintilla::API::FontWeight weight;
	bool italics;
	bool eolfilled;
	bool underlined;
	Scintilla::API::CaseVisible caseForce;
	bool visible;
	bool changeable;
	enum flags { sdNone = 0, sdFont = 0x1, sdSize = 0x2, sdFore = 0x4, sdBack = 0x8,
		     sdWeight = 0x10, sdItalics = 0x20, sdEOLFilled = 0x40, sdUnderlined = 0x80,
		     sdCaseForce = 0x100, sdVisible = 0x200, sdChangeable = 0x400
		   } specified;
	explicit StyleDefinition(std::string_view definition);
	bool ParseStyleDefinition(std::string_view definition);
	Scintilla::API::Colour Fore() const;
	Scintilla::API::Colour Back() const;
	int FractionalSize() const noexcept;
	bool IsBold() const noexcept;
};

inline constexpr Scintilla::API::Colour ColourRGB(unsigned int red, unsigned int green, unsigned int blue) noexcept {
	return red | (green << 8) | (blue << 16);
}

int IntFromHexDigit(int ch) noexcept;
int IntFromHexByte(std::string_view hexByte) noexcept;

Scintilla::API::Colour ColourFromString(const std::string &s);

struct IndicatorDefinition {
	Scintilla::API::IndicatorStyle style;
	Scintilla::API::Colour colour;
	Scintilla::API::Alpha fillAlpha;
	Scintilla::API::Alpha outlineAlpha;
	bool under;
	explicit IndicatorDefinition(std::string_view definition);
	bool ParseIndicatorDefinition(std::string_view definition);
};

#endif
