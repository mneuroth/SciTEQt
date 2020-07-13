// SciTE - Scintilla based Text Editor
/** @file StyleDefinition.cxx
 ** Implementation of style aggregate.
 **/
// Copyright 2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <stdexcept>
#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <chrono>

#include "ScintillaTypes.h"

#include "GUI.h"
#include "StringHelpers.h"
#include "StyleDefinition.h"

namespace SA = Scintilla::API;

namespace {

typedef std::tuple<std::string_view, std::string_view> ViewPair;

// Split view around first separator returning the portion before and after the separator.
// If the separator is not present then return whole view and an empty view.
ViewPair ViewSplit(std::string_view view, char separator) noexcept {
	const size_t sepPos = view.find_first_of(separator);
	std::string_view first = view.substr(0, sepPos);
	std::string_view second = sepPos == (std::string_view::npos) ? "" : view.substr(sepPos + 1);
	return { first, second };
}

}

StyleDefinition::StyleDefinition(std::string_view definition) :
	sizeFractional(10.0), size(10), fore("#000000"), back("#FFFFFF"),
	weight(SA::FontWeight::Normal), italics(false), eolfilled(false), underlined(false),
	caseForce(SA::CaseVisible::Mixed),
	visible(true), changeable(true),
	specified(sdNone) {
	ParseStyleDefinition(definition);
}

bool StyleDefinition::ParseStyleDefinition(std::string_view definition) {
	if (definition.empty()) {
		return false;
	}
	while (!definition.empty()) {
		// Find attribute separator ',' and select front attribute
		const ViewPair optionRest = ViewSplit(definition, ',');
		const std::string_view option = std::get<0>(optionRest);
		definition = std::get<1>(optionRest);
		// Find value separator ':' and break into name and value
		const auto [optionName, optionValue] = ViewSplit(option, ':');

		if (optionName == "italics") {
			specified = static_cast<flags>(specified | sdItalics);
			italics = true;
		}
		if (optionName == "notitalics") {
			specified = static_cast<flags>(specified | sdItalics);
			italics = false;
		}
		if (optionName == "bold") {
			specified = static_cast<flags>(specified | sdWeight);
			weight = SA::FontWeight::Bold;
		}
		if (optionName == "notbold") {
			specified = static_cast<flags>(specified | sdWeight);
			weight = SA::FontWeight::Normal;
		}
		if ((optionName == "weight") && !optionValue.empty()) {
			specified = static_cast<flags>(specified | sdWeight);
			try {
				weight = static_cast<SA::FontWeight>(std::stoi(std::string(optionValue)));
			} catch (std::logic_error &) {
				// Ignore bad values, either non-numeric or out of range numberic
			}
		}
		if ((optionName == "font") && !optionValue.empty()) {
			specified = static_cast<flags>(specified | sdFont);
			font = optionValue;
			std::replace(font.begin(), font.end(), '|', ',');
		}
		if ((optionName == "fore") && !optionValue.empty()) {
			specified = static_cast<flags>(specified | sdFore);
			fore = optionValue;
		}
		if ((optionName == "back") && !optionValue.empty()) {
			specified = static_cast<flags>(specified | sdBack);
			back = optionValue;
		}
		if ((optionName == "size") && !optionValue.empty()) {
			specified = static_cast<flags>(specified | sdSize);
			sizeFractional = std::stof(std::string(optionValue));
			size = static_cast<int>(sizeFractional);
		}
		if (optionName == "eolfilled") {
			specified = static_cast<flags>(specified | sdEOLFilled);
			eolfilled = true;
		}
		if (optionName == "noteolfilled") {
			specified = static_cast<flags>(specified | sdEOLFilled);
			eolfilled = false;
		}
		if (optionName == "underlined") {
			specified = static_cast<flags>(specified | sdUnderlined);
			underlined = true;
		}
		if (optionName == "notunderlined") {
			specified = static_cast<flags>(specified | sdUnderlined);
			underlined = false;
		}
		if (optionName == "case") {
			specified = static_cast<flags>(specified | sdCaseForce);
			caseForce = SA::CaseVisible::Mixed;
			if (!optionValue.empty()) {
				if (optionValue.front() == 'u')
					caseForce = SA::CaseVisible::Upper;
				else if (optionValue.front() == 'l')
					caseForce = SA::CaseVisible::Lower;
				else if (optionValue.front() == 'c')
					caseForce = SA::CaseVisible::Camel;
			}
		}
		if (optionName == "visible") {
			specified = static_cast<flags>(specified | sdVisible);
			visible = true;
		}
		if (optionName == "notvisible") {
			specified = static_cast<flags>(specified | sdVisible);
			visible = false;
		}
		if (optionName == "changeable") {
			specified = static_cast<flags>(specified | sdChangeable);
			changeable = true;
		}
		if (optionName == "notchangeable") {
			specified = static_cast<flags>(specified | sdChangeable);
			changeable = false;
		}
	}
	return true;
}

SA::Colour StyleDefinition::Fore() const {
	return ColourFromString(fore);
}

SA::Colour StyleDefinition::Back() const {
	return ColourFromString(back);
}

int StyleDefinition::FractionalSize() const noexcept {
	return static_cast<int>(sizeFractional * SA::FontSizeMultiplier);
}

bool StyleDefinition::IsBold() const noexcept {
	return weight > SA::FontWeight::Normal;
}

int IntFromHexDigit(int ch) noexcept {
	if ((ch >= '0') && (ch <= '9')) {
		return ch - '0';
	} else if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + 10;
	} else if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + 10;
	} else {
		return 0;
	}
}

int IntFromHexByte(std::string_view hexByte) noexcept {
	return IntFromHexDigit(hexByte[0]) * 16 + IntFromHexDigit(hexByte[1]);
}

SA::Colour ColourFromString(const std::string &s) {
	if (s.length() >= 7) {
		const int r = IntFromHexByte(&s[1]);
		const int g = IntFromHexByte(&s[3]);
		const int b = IntFromHexByte(&s[5]);
		return ColourRGB(r, g, b);
	} else {
		return 0;
	}
}

IndicatorDefinition::IndicatorDefinition(std::string_view definition) :
	style(SA::IndicatorStyle::Plain), colour(0), fillAlpha(static_cast<SA::Alpha>(30)), outlineAlpha(static_cast<SA::Alpha>(50)), under(false) {
	ParseIndicatorDefinition(definition);
}

bool IndicatorDefinition::ParseIndicatorDefinition(std::string_view definition) {
	if (definition.empty()) {
		return false;
	}
	struct NameValue {
		std::string_view name;
		SA::IndicatorStyle value;
	};
	const NameValue indicStyleNames[] = {
		{ "plain", SA::IndicatorStyle::Plain },
		{ "squiggle", SA::IndicatorStyle::Squiggle },
		{ "tt", SA::IndicatorStyle::TT },
		{ "diagonal", SA::IndicatorStyle::Diagonal },
		{ "strike", SA::IndicatorStyle::Strike },
		{ "hidden", SA::IndicatorStyle::Hidden },
		{ "box", SA::IndicatorStyle::Box },
		{ "roundbox", SA::IndicatorStyle::RoundBox },
		{ "straightbox", SA::IndicatorStyle::StraightBox },
		{ "dash", SA::IndicatorStyle::Dash },
		{ "dots", SA::IndicatorStyle::Dots },
		{ "squigglelow", SA::IndicatorStyle::SquiggleLow },
		{ "dotbox", SA::IndicatorStyle::DotBox },
		{ "squigglepixmap", SA::IndicatorStyle::SquigglePixmap },
		{ "compositionthick", SA::IndicatorStyle::CompositionThick },
		{ "compositionthin", SA::IndicatorStyle::CompositionThin },
		{ "fullbox", SA::IndicatorStyle::FullBox },
		{ "textfore", SA::IndicatorStyle::TextFore },
		{ "point", SA::IndicatorStyle::Point },
		{ "pointcharacter", SA::IndicatorStyle::PointCharacter },
		{ "gradient", SA::IndicatorStyle::Gradient },
		{ "gradientverticalcentred", SA::IndicatorStyle::GradientCentre },
	};

	std::string val(definition);
	LowerCaseAZ(val);
	std::string_view indicatorDefinition = val;
	while (!indicatorDefinition.empty()) {
		// Find attribute separator ',' and select front attribute
		const ViewPair optionRest = ViewSplit(indicatorDefinition, ',');
		const std::string_view option = std::get<0>(optionRest);
		indicatorDefinition = std::get<1>(optionRest);
		// Find value separator ':' and break into name and value
		const auto [optionName, optionValue] = ViewSplit(option, ':');

		try {
			if (!optionValue.empty() && (optionName == "style")) {
				bool found = false;
				for (const NameValue &indicStyleName : indicStyleNames) {
					if (optionValue == indicStyleName.name) {
						style = indicStyleName.value;
						found = true;
					}
				}
				if (!found) {
					style = static_cast<SA::IndicatorStyle>(std::stoi(std::string(optionValue)));
				}
			}
			if (!optionValue.empty() && ((optionName == "colour") || (optionName == "color"))) {
				colour = ColourFromString(std::string(optionValue));
			}
			if (!optionValue.empty() && (optionName == "fillalpha")) {
				fillAlpha = static_cast<SA::Alpha>(std::stoi(std::string(optionValue)));
			}
			if (!optionValue.empty() && (optionName == "outlinealpha")) {
				outlineAlpha = static_cast<SA::Alpha>(std::stoi(std::string(optionValue)));
			}
			if (optionName == "under") {
				under = true;
			}
			if (optionName == "notunder") {
				under = false;
			}
		} catch (std::logic_error &) {
			// Ignore bad values, either non-numeric or out of range numberic
		}
	}
	return true;
}
