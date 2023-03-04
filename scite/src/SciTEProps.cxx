// SciTE - Scintilla based Text Editor
/** @file SciTEProps.cxx
 ** Properties management.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <clocale>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

#include <fcntl.h>

#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "Scintilla.h"
#include "SciLexer.h"
#include "Lexilla.h"
#include "LexillaAccess.h"

#include "GUI.h"
#include "ScintillaWindow.h"

#if (defined(__unix__) || defined(__APPLE__)) && !defined(QT_QML)

const GUI::gui_char menuAccessIndicator[] = GUI_TEXT("_");

#else

const GUI::gui_char menuAccessIndicator[] = GUI_TEXT("&");

#endif

#include "StringList.h"
#include "StringHelpers.h"
#include "FilePath.h"
#include "StyleDefinition.h"
#include "PropSetFile.h"
#include "StyleWriter.h"
#include "Extender.h"
#include "SciTE.h"
#include "JobQueue.h"
#include "Cookie.h"
#include "Worker.h"
#include "MatchMarker.h"
#include "EditorConfig.h"
#include "Searcher.h"
#include "SciTEBase.h"
#include "IFaceTable.h"

void SciTEBase::SetImportMenu() {
	for (int i = 0; i < importMax; i++) {
		DestroyMenuItem(menuOptions, importCmdID + i);
	}
	if (!importFiles.empty()) {
		for (int stackPos = 0; stackPos < static_cast<int>(importFiles.size()) && stackPos < importMax; stackPos++) {
			const int itemID = importCmdID + stackPos;
			if (importFiles[stackPos].IsSet()) {
				GUI::gui_string entry = localiser.Text("Open");
				entry += GUI_TEXT(" ");
				entry += importFiles[stackPos].Name().AsInternal();
				SetMenuItem(menuOptions, IMPORT_START + stackPos, itemID, entry.c_str());
			}
		}
	}
}

void SciTEBase::ImportMenu(int pos) {
	if (pos >= 0) {
		if (importFiles[pos].IsSet()) {
			Open(importFiles[pos]);
		}
	}
}

void SciTEBase::SetLanguageMenu() {
	for (int i = 0; i < 100; i++) {
		DestroyMenuItem(menuLanguage, languageCmdID + i);
	}
	for (unsigned int item = 0; item < languageMenu.size(); item++) {
		const int itemID = languageCmdID + item;
		const GUI::gui_string entry = localiser.Text(languageMenu[item].menuItem.c_str());
		const GUI::gui_string menuKey = GUI::StringFromUTF8(languageMenu[item].menuKey);
		if (entry.size() && entry[0] != '#') {
			SetMenuItem(menuLanguage, item, itemID, entry.c_str(), menuKey.c_str());
		}
	}
}

// Null except on Windows where it may be overridden
void SciTEBase::ReadEmbeddedProperties() {
}

const GUI::gui_char propLocalFileName[] = GUI_TEXT("SciTE.properties");
const GUI::gui_char propDirectoryFileName[] = GUI_TEXT("SciTEDirectory.properties");

void SciTEBase::ReadEnvironment() {
#if defined(__unix__) || defined(__APPLE__)
	extern char **environ;
	char **e = environ;
#else
	char **e = _environ;
#endif
	for (; e && *e; e++) {
		const auto [key, value] = ViewSplit(*e, '=');
		propsPlatform.Set(key, value);
	}
}

/**
Read global and user properties files.
*/
void SciTEBase::ReadGlobalPropFile() {
	// Appearance and Contrast may be read in embedded or global properties
	// so set them in deepest property set propsPlatform.
	propsPlatform.Set("Appearance", StdStringFromInteger(appearance.dark));
	propsPlatform.Set("Contrast", StdStringFromInteger(appearance.highContrast));

	std::string excludes;
	std::string includes;

	// Want to apply imports.exclude and imports.include but these may well be in
	// user properties.

	for (int attempt=0; attempt<2; attempt++) {

		std::string excludesRead = props.GetString("imports.exclude");
		std::string includesRead = props.GetString("imports.include");
		if ((attempt > 0) && ((excludesRead == excludes) && (includesRead == includes)))
			break;

		excludes = excludesRead;
		includes = includesRead;

		filter.SetFilter(excludes, includes);

		importFiles.clear();

		ReadEmbeddedProperties();

		propsBase.Clear();
		FilePath propfileBase = GetDefaultPropertiesFileName();
		propsBase.Read(propfileBase, propfileBase.Directory(), filter, &importFiles, 0);

		propsUser.Clear();
		FilePath propfileUser = GetUserPropertiesFileName();
		propsUser.Read(propfileUser, propfileUser.Directory(), filter, &importFiles, 0);
	}

	if (!localiser.read) {
		ReadLocalization();
	}
}

void SciTEBase::ReadAbbrevPropFile() {
	propsAbbrev.Clear();
	propsAbbrev.Read(pathAbbreviations, pathAbbreviations.Directory(), filter, &importFiles, 0);
}

/**
Reads the directory properties file depending on the variable
"properties.directory.enable". Also sets the variable $(SciteDirectoryHome) to the path
where this property file is found. If it is not found $(SciteDirectoryHome) will
be set to $(FilePath).
*/
void SciTEBase::ReadDirectoryPropFile() {
	propsDirectory.Clear();

	propsDirectory.SetPath("FilePath", filePath);
	propsDirectory.SetPath("FileDir", filePath.Directory());
	propsDirectory.SetPath("FileName", filePath.BaseName());
	propsDirectory.SetPath("FileExt", filePath.Extension());
	propsDirectory.SetPath("FileNameExt", FileNameExt());

	if (props.GetInt("properties.directory.enable") != 0) {
		const FilePath propfile = GetDirectoryPropertiesFileName();
		const FilePath propfileDirectory = propfile.Directory();
		const GUI::gui_string relPath = propfileDirectory.RelativePathTo(filePath);
		propsDirectory.Set("RelativePath", GUI::UTF8FromString(relPath));

		props.SetPath("SciteDirectoryHome", propfileDirectory);

		propsDirectory.Read(propfile, propfileDirectory, filter, nullptr, 0);
	} else {
		propsDirectory.SetPath("RelativePath", filePath.Name());
	}
}

/**
Read local and directory properties file.
*/
void SciTEBase::ReadLocalPropFile() {
	// The directory properties acts like a base local properties file.
	// Therefore it must be read always before reading the local prop file.
	ReadDirectoryPropFile();

	FilePath propfile = GetLocalPropertiesFileName();

	propsLocal.Clear();
	propsLocal.Read(propfile, propfile.Directory(), filter, nullptr, 0);

	props.Set("Chrome", "#C0C0C0");
	props.Set("ChromeHighlight", "#FFFFFF");

	FilePath fileDirectory = filePath.Directory();
	editorConfig->Clear();
	if (props.GetInt("editor.config.enable", 0)) {
		editorConfig->ReadFromDirectory(fileDirectory);
	}
}

SA::Colour ColourOfProperty(const PropSetFile &props, const char *key, SA::Colour colourDefault) {
	std::string colour = props.GetExpandedString(key);
	if (colour.length()) {
		return ColourFromString(colour);
	}
	return colourDefault;
}

SA::ColourAlpha ColourAlphaOfProperty(const PropSetFile &props, const char *key, SA::ColourAlpha colourDefault) {
	std::string colour = props.GetExpandedString(key);
	if (colour.length()) {
		return ColourAlphaFromString(colour);
	}
	return colourDefault;
}

namespace {

void OptionalSetColour(GUI::ScintillaWindow &scintilla, SA::Element element, const PropSetFile &props, const char *key) {
	std::string colour = props.GetExpandedString(key);
	if (colour.length()) {
		const SA::Colour colourElement = ColourAlphaFromString(colour);
		scintilla.SetElementColour(element, colourElement);
	} else {
		scintilla.ResetElementColour(element);
	}
}

}

void SciTEBase::SetElementColour(SA::Element element, const char *key) {
	OptionalSetColour(wEditor, element, props, key);
	OptionalSetColour(wOutput, element, props, key);
}

/**
 * Put the next property item from the given property string
 * into the buffer pointed by @a pPropItem.
 * @return NULL if the end of the list is met, else, it points to the next item.
 */
const char *SciTEBase::GetNextPropItem(
	const char *pStart,	/**< the property string to parse for the first call,
						 * pointer returned by the previous call for the following. */
	char *pPropItem,	///< pointer on a buffer receiving the requested prop item
	size_t maxLen) noexcept {	///< size of the above buffer
	ptrdiff_t size = maxLen - 1;

	*pPropItem = '\0';
	if (!pStart) {
		return nullptr;
	}
	const char *pNext = strchr(pStart, ',');
	if (pNext) {	// Separator is found
		if (size > pNext - pStart) {
			// Found string fits in buffer
			size = pNext - pStart;
		}
		pNext++;
	}
	strncpy(pPropItem, pStart, size);
	pPropItem[size] = '\0';
	return pNext;
}

std::string SciTEBase::StyleString(const char *lang, int style) const {
	char key[200];
	sprintf(key, "style.%s.%0d", lang, style);
	return props.GetExpandedString(key);
}

StyleDefinition SciTEBase::StyleDefinitionFor(int style) {
	const std::string ssDefault = StyleString("*", style);
	std::string ss = StyleString(language.c_str(), style);

	if (!subStyleBases.empty()) {
		const int baseStyle = wEditor.StyleFromSubStyle(style);
		if (baseStyle != style) {
			const int primaryStyle = wEditor.PrimaryStyleFromStyle(style);
			const int distanceSecondary = (style == primaryStyle) ? 0 : wEditor.DistanceToSecondaryStyles();
			const int primaryBase = baseStyle - distanceSecondary;
			const int subStylesStart = wEditor.SubStylesStart(primaryBase);
			const int subStylesLength = wEditor.SubStylesLength(primaryBase);
			const int subStyle = style - (subStylesStart + distanceSecondary);
			if (subStyle < subStylesLength) {
				char key[200];
				sprintf(key, "style.%s.%0d.%0d", language.c_str(), baseStyle, subStyle + 1);
				ss = props.GetNewExpandString(key);
			}
		}
	}

	StyleDefinition sd(ssDefault);
	sd.ParseStyleDefinition(ss);
	return sd;
}

void SciTEBase::SetOneStyle(GUI::ScintillaWindow &win, int style, const StyleDefinition &sd) {
	if (sd.specified & StyleDefinition::sdItalics)
		win.StyleSetItalic(style, sd.italics);
	if (sd.specified & StyleDefinition::sdWeight)
		win.StyleSetWeight(style, sd.weight);
	if (sd.specified & StyleDefinition::sdFont) {
		win.StyleSetFont(style, sd.font.c_str());
		bool inMonospacedList = !monospacedList.empty() && (monospacedList.back() == "*");
		if (!inMonospacedList) {
			inMonospacedList = std::find(monospacedList.begin(), monospacedList.end(), sd.font) != monospacedList.end();
		}
		win.StyleSetCheckMonospaced(style, inMonospacedList);
	}
	if (sd.specified & StyleDefinition::sdFore)
		win.StyleSetFore(style, sd.Fore());
	if (sd.specified & StyleDefinition::sdBack)
		win.StyleSetBack(style, sd.Back());
	if (sd.specified & StyleDefinition::sdSize)
		win.StyleSetSizeFractional(style, sd.FractionalSize());
	if (sd.specified & StyleDefinition::sdEOLFilled)
		win.StyleSetEOLFilled(style, sd.eolfilled);
	if (sd.specified & StyleDefinition::sdUnderlined)
		win.StyleSetUnderline(style, sd.underlined);
	if (sd.specified & StyleDefinition::sdCaseForce)
		win.StyleSetCase(style, sd.caseForce);
	if (sd.specified & StyleDefinition::sdVisible)
		win.StyleSetVisible(style, sd.visible);
	if (sd.specified & StyleDefinition::sdChangeable)
		win.StyleSetChangeable(style, sd.changeable);
	if (sd.specified & StyleDefinition::sdInvisibleRep)
		win.StyleSetInvisibleRepresentation(style, sd.invisibleRep.c_str());
	win.StyleSetCharacterSet(style, characterSet);
}

void SciTEBase::SetStyleBlock(GUI::ScintillaWindow &win, const char *lang, int start, int last) {
	for (int style = start; style <= last; style++) {
		if (style != StyleDefault) {
			char key[200];
			sprintf(key, "style.%s.%0d", lang, style-start);
			std::string sval = props.GetExpandedString(key);
			if (sval.length()) {
				SetOneStyle(win, style, StyleDefinition(sval));
			}
		}
	}
}

void SciTEBase::SetStyleFor(GUI::ScintillaWindow &win, const char *lang) {
	SetStyleBlock(win, lang, 0, StyleMax);
}

void SciTEBase::SetOneIndicator(GUI::ScintillaWindow &win, SA::IndicatorNumbers indicator, const IndicatorDefinition &ind) {
	const int indic = static_cast<int>(indicator);
	win.IndicSetStyle(indic, ind.style);
	win.IndicSetFore(indic, ind.colour);
	win.IndicSetAlpha(indic, ind.fillAlpha);
	win.IndicSetOutlineAlpha(indic, ind.outlineAlpha);
	win.IndicSetUnder(indic, ind.under);
}

void SciTEBase::SetIndicatorFromProperty(GUI::ScintillaWindow &win, SA::IndicatorNumbers indicator, const std::string &propertyName) {
	const std::string indicatorString = props.GetExpandedString(propertyName.c_str());
	if (!indicatorString.empty()) {
		IndicatorDefinition modifiedIndicator(indicatorString);
		SetOneIndicator(win, indicator, modifiedIndicator);
	}
}

void SciTEBase::SetMarkerFromProperty(GUI::ScintillaWindow &win, int marker, const std::string &propertyName) {
	const std::string markerString = props.GetExpandedString(propertyName.c_str());
	if (!markerString.empty()) {
		MarkerDefinition markerValue(markerString);
		win.MarkerDefine(marker, markerValue.style);
		win.MarkerSetForeTranslucent(marker, markerValue.colour);
		win.MarkerSetBackTranslucent(marker, markerValue.back);
	}
}

std::string SciTEBase::ExtensionFileName() const {
	if (CurrentBufferConst()->overrideExtension.length()) {
		return CurrentBufferConst()->overrideExtension;
	} else {
		FilePath name = FileNameExt();
		if (name.IsSet()) {
#if !defined(GTK)
			// Force extension to lower case
			std::string extension = name.Extension().AsUTF8();
			if (extension.empty()) {
				return name.AsUTF8();
			} else {
				LowerCaseAZ(extension);
				return name.BaseName().AsUTF8() + "." + extension;
			}
#else
			return name.AsUTF8();
#endif
		} else {
			return props.GetString("default.file.ext");
		}
	}
}

void SciTEBase::ForwardPropertyToEditor(const char *key) {
	if (props.Exists(key)) {
		std::string value = props.GetExpandedString(key);
		wEditor.SetProperty(key, value.c_str());
		wOutput.SetProperty(key, value.c_str());
	}
}

void SciTEBase::DefineMarker(SA::MarkerOutline marker, SA::MarkerSymbol markerType, MarkerAppearance markerAppearance) {
	const int markerNumber = static_cast<int>(marker);
	wEditor.MarkerDefine(markerNumber, markerType);
	wEditor.MarkerSetForeTranslucent(markerNumber, markerAppearance.fore);
	wEditor.MarkerSetBackTranslucent(markerNumber, markerAppearance.back);
	wEditor.MarkerSetBackSelectedTranslucent(markerNumber, markerAppearance.backSelected);
	wEditor.MarkerSetStrokeWidth(markerNumber, markerAppearance.strokeWidth);
}

void SciTEBase::ReadAPI(const std::string &fileNameForExtension) {
	std::string sApiFileNames = props.GetNewExpandString("api.",
				    fileNameForExtension.c_str());
	if (sApiFileNames.length() > 0) {
		std::vector<std::string> vApiFileNames = StringSplit(sApiFileNames, ';');
		std::vector<char> data;

		// Load files into data
		for (const std::string &vApiFileName : vApiFileNames) {
			std::string contents = FilePath(GUI::StringFromUTF8(vApiFileName)).Read();
			data.insert(data.end(), contents.begin(), contents.end());
		}

		// Initialise apis
		if (data.size() > 0) {
			apis.Set(data);
		}
	}
}

std::string SciTEBase::FindLanguageProperty(const char *pattern, const char *defaultValue) {
	std::string key = pattern;
	Substitute(key, "*", language);
	std::string ret = props.GetExpandedString(key.c_str());
	if (ret == "")
		ret = props.GetExpandedString(pattern);
	if (ret == "")
		ret = defaultValue;
	return ret;
}

/**
 * A list of all the properties that should be forwarded to Scintilla lexers.
 */
static const char *propertiesToForward[] = {
	"fold.scintillua.by.indentation",
	"fold.scintillua.line.groups",
	"fold.scintillua.on.zero.sum.lines",
	"fold.scintillua.compact",
//++Autogenerated -- run ../scripts/RegenerateSource.py to regenerate
//**\(\t"\*",\n\)
	"asp.default.language",
	"fold",
	"fold.abl.comment.multiline",
	"fold.abl.syntax.based",
	"fold.asm.comment.explicit",
	"fold.asm.comment.multiline",
	"fold.asm.explicit.anywhere",
	"fold.asm.explicit.end",
	"fold.asm.explicit.start",
	"fold.asm.syntax.based",
	"fold.at.else",
	"fold.baan.inner.level",
	"fold.baan.keywords.based",
	"fold.baan.sections",
	"fold.baan.syntax.based",
	"fold.basic.comment.explicit",
	"fold.basic.explicit.anywhere",
	"fold.basic.explicit.end",
	"fold.basic.explicit.start",
	"fold.basic.syntax.based",
	"fold.cil.comment.multiline",
	"fold.coffeescript.comment",
	"fold.comment",
	"fold.comment.nimrod",
	"fold.comment.yaml",
	"fold.compact",
	"fold.cpp.comment.explicit",
	"fold.cpp.comment.multiline",
	"fold.cpp.explicit.anywhere",
	"fold.cpp.explicit.end",
	"fold.cpp.explicit.start",
	"fold.cpp.preprocessor.at.else",
	"fold.cpp.syntax.based",
	"fold.d.comment.explicit",
	"fold.d.comment.multiline",
	"fold.d.explicit.anywhere",
	"fold.d.explicit.end",
	"fold.d.explicit.start",
	"fold.d.syntax.based",
	"fold.dataflex.compilerlist",
	"fold.directive",
	"fold.fsharp.comment.multiline",
	"fold.fsharp.comment.stream",
	"fold.fsharp.imports",
	"fold.fsharp.preprocessor",
	"fold.gdscript.quotes",
	"fold.haskell.imports",
	"fold.html",
	"fold.html.preprocessor",
	"fold.hypertext.comment",
	"fold.hypertext.heredoc",
	"fold.julia.docstring",
	"fold.julia.syntax.based",
	"fold.perl.at.else",
	"fold.perl.comment.explicit",
	"fold.perl.package",
	"fold.perl.pod",
	"fold.preprocessor",
	"fold.quotes.nimrod",
	"fold.quotes.python",
	"fold.raku.comment.multiline",
	"fold.raku.comment.pod",
	"fold.rust.comment.explicit",
	"fold.rust.comment.multiline",
	"fold.rust.explicit.anywhere",
	"fold.rust.explicit.end",
	"fold.rust.explicit.start",
	"fold.rust.syntax.based",
	"fold.sql.at.else",
	"fold.sql.only.begin",
	"fold.verilog.flags",
	"fold.xml.at.tag.open",
	"html.tags.case.sensitive",
	"lexer.as.comment.character",
	"lexer.asm.comment.delimiter",
	"lexer.baan.styling.within.preprocessor",
	"lexer.caml.magic",
	"lexer.cpp.allow.dollars",
	"lexer.cpp.backquoted.strings",
	"lexer.cpp.escape.sequence",
	"lexer.cpp.hashquoted.strings",
	"lexer.cpp.track.preprocessor",
	"lexer.cpp.triplequoted.strings",
	"lexer.cpp.update.preprocessor",
	"lexer.cpp.verbatim.strings.allow.escapes",
	"lexer.css.hss.language",
	"lexer.css.less.language",
	"lexer.css.scss.language",
	"lexer.d.fold.at.else",
	"lexer.edifact.highlight.un.all",
	"lexer.errorlist.escape.sequences",
	"lexer.errorlist.value.separate",
	"lexer.flagship.styling.within.preprocessor",
	"lexer.gdscript.keywords2.no.sub.identifiers",
	"lexer.gdscript.literals.binary",
	"lexer.gdscript.strings.over.newline",
	"lexer.gdscript.unicode.identifiers",
	"lexer.gdscript.whinge.level",
	"lexer.haskell.allow.hash",
	"lexer.haskell.allow.questionmark",
	"lexer.haskell.allow.quotes",
	"lexer.haskell.cpp",
	"lexer.haskell.import.safe",
	"lexer.html.django",
	"lexer.html.mako",
	"lexer.json.allow.comments",
	"lexer.json.escape.sequence",
	"lexer.julia.highlight.lexerror",
	"lexer.julia.highlight.typeannotation",
	"lexer.markdown.header.eolfill",
	"lexer.metapost.comment.process",
	"lexer.metapost.interface.default",
	"lexer.nim.raw.strings.highlight.ident",
	"lexer.pascal.smart.highlighting",
	"lexer.props.allow.initial.spaces",
	"lexer.python.decorator.attributes",
	"lexer.python.identifier.attributes",
	"lexer.python.keywords2.no.sub.identifiers",
	"lexer.python.literals.binary",
	"lexer.python.strings.b",
	"lexer.python.strings.f",
	"lexer.python.strings.over.newline",
	"lexer.python.strings.u",
	"lexer.python.unicode.identifiers",
	"lexer.r.escape.sequence",
	"lexer.rust.fold.at.else",
	"lexer.sql.allow.dotted.word",
	"lexer.sql.backticks.identifier",
	"lexer.sql.numbersign.comment",
	"lexer.tex.auto.if",
	"lexer.tex.comment.process",
	"lexer.tex.interface.default",
	"lexer.tex.use.keywords",
	"lexer.verilog.allupperkeywords",
	"lexer.verilog.fold.preprocessor.else",
	"lexer.verilog.portstyling",
	"lexer.verilog.track.preprocessor",
	"lexer.verilog.update.preprocessor",
	"lexer.visualprolog.backquoted.strings",
	"lexer.visualprolog.verbatim.strings",
	"lexer.xml.allow.scripts",
	"nsis.ignorecase",
	"nsis.uservars",
	"ps.level",
	"sql.backslash.escapes",
	"styling.within.preprocessor",
	"tab.timmy.whinge.level",

//--Autogenerated -- end of automatically generated section

	nullptr,
};

/* XPM */
static const char *bookmarkBluegem[] = {
/* width height num_colors chars_per_pixel */
"    15    15      64            1",
/* colors */
"  c none",
". c #0c0630",
"# c #8c8a8c",
"a c #244a84",
"b c #545254",
"c c #cccecc",
"d c #949594",
"e c #346ab4",
"f c #242644",
"g c #3c3e3c",
"h c #6ca6fc",
"i c #143789",
"j c #204990",
"k c #5c8dec",
"l c #707070",
"m c #3c82dc",
"n c #345db4",
"o c #619df7",
"p c #acacac",
"q c #346ad4",
"r c #1c3264",
"s c #174091",
"t c #5482df",
"u c #4470c4",
"v c #2450a0",
"w c #14162c",
"x c #5c94f6",
"y c #b7b8b7",
"z c #646464",
"A c #3c68b8",
"B c #7cb8fc",
"C c #7c7a7c",
"D c #3462b9",
"E c #7c7eac",
"F c #44464c",
"G c #a4a4a4",
"H c #24224c",
"I c #282668",
"J c #5c5a8c",
"K c #7c8ebc",
"L c #dcd7e4",
"M c #141244",
"N c #1c2e5c",
"O c #24327c",
"P c #4472cc",
"Q c #6ca2fc",
"R c #74b2fc",
"S c #24367c",
"T c #b4b2c4",
"U c #403e58",
"V c #4c7fd6",
"W c #24428c",
"X c #747284",
"Y c #142e7c",
"Z c #64a2fc",
"0 c #3c72dc",
"1 c #bcbebc",
"2 c #6c6a6c",
"3 c #848284",
"4 c #2c5098",
"5 c #1c1a1c",
"6 c #243250",
"7 c #7cbefc",
"8 c #d4d2d4",
/* pixels */
"    yCbgbCy    ",
"   #zGGyGGz#   ",
"  #zXTLLLTXz#  ",
" p5UJEKKKEJU5p ",
" lfISa444aSIfl ",
" wIYij444jsYIw ",
" .OsvnAAAnvsO. ",
" MWvDuVVVPDvWM ",
" HsDPVkxxtPDsH ",
" UiAtxohZxtuiU ",
" pNnkQRBRhkDNp ",
" 1FrqoR7Bo0rF1 ",
" 8GC6aemea6CG8 ",
"  cG3l2z2l3Gc  ",
"    1GdddG1    "
};

std::string SciTEBase::GetFileNameProperty(const char *name) {
	std::string namePlusDot = name;
	namePlusDot.append(".");
	std::string valueForFileName = props.GetNewExpandString(namePlusDot.c_str(),
				       ExtensionFileName().c_str());
	if (valueForFileName.length() != 0) {
		return valueForFileName;
	} else {
		return props.GetString(name);
	}
}

void SciTEBase::SetRepresentations() {
	wEditor.ClearAllRepresentations();
	const std::string representations = props.GetExpandedString("representations");
	std::string_view reprs = representations;

	bool colourSet = false;
	SA::Colour colourRepresentation = 0;

	bool appearanceSet = false;
	SA::RepresentationAppearance appearanceRepresentation = SA::RepresentationAppearance::Blob;

	while (!reprs.empty()) {
		if (reprs[0] == ',') {
			reprs.remove_prefix(1);
		}
		if (!reprs.empty()) {
			const std::string_view item = reprs.substr(0, reprs.find_first_of(','));
			if (!item.empty()) {
				if (item[0] == '!') {
					// Appearance
					if (item.length() == 1) {
						appearanceSet = false;
					} else {
						appearanceSet = true;
						const std::string sAppearance(item.substr(1));
						appearanceRepresentation = static_cast<SA::RepresentationAppearance>(IntegerFromString(sAppearance, 0));
					}
				} else if (item[0] == '#') {
					// Colour
					if (item.length() == 1) {
						colourSet = false;
					} else {
						colourSet = true;
						colourRepresentation = ColourAlphaFromString(item);
					}
				} else {
					const size_t eqPos = item.find_first_of('=');
					if (eqPos != std::string_view::npos) {
						const std::string target = UnicodeUnEscape(item.substr(0, eqPos));
						const std::string representation = UnicodeUnEscape(item.substr(eqPos+1));
						wEditor.SetRepresentation(target.c_str(), representation.c_str());
						if (appearanceSet) {
							wEditor.SetRepresentationAppearance(target.c_str(), appearanceRepresentation);
						}
						if (colourSet) {
							wEditor.SetRepresentationColour(target.c_str(), colourRepresentation);
						}
					}
				}
				reprs.remove_prefix(item.length());
			}
		}
	}
}

void SciTEBase::ReadProperties() {
	if (extender)
		extender->Clear();

	const std::string lexillaPath = props.GetExpandedString("lexilla.path");
	Lexilla::Load(lexillaPath.empty() ? "." : lexillaPath);

	std::vector<std::string> libraryProperties = Lexilla::LibraryProperties();
	for (std::string property : libraryProperties) {
		std::string key("lexilla.context.");
		key += property;
		std::string value = props.GetExpandedString(key.c_str());
		Lexilla::SetProperty(property.c_str(), value.c_str());
	}

	const std::string fileNameForExtension = ExtensionFileName();

	language = props.GetNewExpandString("lexer.", fileNameForExtension.c_str());
	if (language.empty()) {
		language = "null";
	}
	if (static_cast<int>(wEditor.DocumentOptions()) & static_cast<int>(SA::DocumentOption::StylesNone)) {
		language = "null";
	}
	const std::string languageCurrent = wEditor.LexerLanguage();
	if (language != languageCurrent) {
		if (StartsWith(language, "script_")) {
			wEditor.SetILexer(nullptr);
		} else {
			Scintilla::ILexer5 *plexer = Lexilla::MakeLexer(language);
			wEditor.SetILexer(plexer);
		}
	}

	props.Set("Language", language);

	lexLanguage = wEditor.Lexer();

	const std::string languageOutput = wOutput.LexerLanguage();
	if (languageOutput != "errorlist") {
		Scintilla::ILexer5 *plexerErrorlist = Lexilla::MakeLexer("errorlist");
		wOutput.SetILexer(plexerErrorlist);
	}

	const std::string kw0 = props.GetNewExpandString("keywords.", fileNameForExtension.c_str());
	wEditor.SetKeyWords(0, kw0.c_str());

	for (int wl = 1; wl <= SA::KeywordsetMax; wl++) {
		std::string kwk = StdStringFromInteger(wl+1);
		kwk += '.';
		kwk.insert(0, "keywords");
		const std::string kw = props.GetNewExpandString(kwk.c_str(), fileNameForExtension.c_str());
		wEditor.SetKeyWords(wl, kw.c_str());
	}

	subStyleBases = wEditor.SubStyleBases();
	if (!subStyleBases.empty()) {
		wEditor.FreeSubStyles();

		for (const unsigned char subStyleBase : subStyleBases) {
			//substyles.cpp.11=2
			const std::string sStyleBase = StdStringFromInteger(subStyleBase);
			std::string ssSubStylesKey = "substyles.";
			ssSubStylesKey += language;
			ssSubStylesKey += ".";
			ssSubStylesKey += sStyleBase;
			std::string ssNumber = props.GetNewExpandString(ssSubStylesKey.c_str());
			int subStyleIdentifiers = atoi(ssNumber.c_str());

			int subStyleIdentifiersStart = 0;
			if (subStyleIdentifiers) {
				subStyleIdentifiersStart = wEditor.AllocateSubStyles(subStyleBase, subStyleIdentifiers);
				if (subStyleIdentifiersStart < 0)
					subStyleIdentifiers = 0;
			}
			for (int subStyle=0; subStyle<subStyleIdentifiers; subStyle++) {
				// substylewords.11.1.$(file.patterns.cpp)=CharacterSet LexAccessor SString WordList
				std::string ssWordsKey = "substylewords.";
				ssWordsKey += sStyleBase;
				ssWordsKey += ".";
				ssWordsKey += StdStringFromInteger(subStyle + 1);
				ssWordsKey += ".";
				std::string ssWords = props.GetNewExpandString(ssWordsKey.c_str(), fileNameForExtension.c_str());
				wEditor.SetIdentifiers(subStyleIdentifiersStart + subStyle, ssWords.c_str());
			}
		}
	}

	props.SetPath("SciteDefaultHome", GetSciteDefaultHome());
	props.SetPath("SciteUserHome", GetSciteUserHome());

	for (size_t i=0; propertiesToForward[i]; i++) {
		ForwardPropertyToEditor(propertiesToForward[i]);
	}

	if (apisFileNames != props.GetNewExpandString("api.", fileNameForExtension.c_str())) {
		apis.Clear();
		ReadAPI(fileNameForExtension);
		apisFileNames = props.GetNewExpandString("api.", fileNameForExtension.c_str());
	}

	props.Set("APIPath", apisFileNames);

	FilePath fileAbbrev = GUI::StringFromUTF8(props.GetNewExpandString("abbreviations.", fileNameForExtension.c_str()));
	if (!fileAbbrev.IsSet())
		fileAbbrev = GetAbbrevPropertiesFileName();
	if (!pathAbbreviations.SameNameAs(fileAbbrev)) {
		pathAbbreviations = fileAbbrev;
		ReadAbbrevPropFile();
	}

	props.SetPath("AbbrevPath", pathAbbreviations);

	const SA::Technology tech = static_cast<SA::Technology>(props.GetInt("technology"));
	wEditor.SetTechnology(tech);
	wOutput.SetTechnology(tech);

	const SA::Bidirectional bidirectional = static_cast<SA::Bidirectional>(props.GetInt("bidirectional"));
	wEditor.SetBidirectional(bidirectional);
	wOutput.SetBidirectional(bidirectional);

	codePage = props.GetInt("code.page");
	if (CurrentBuffer()->unicodeMode != UniMode::uni8Bit) {
		// Override properties file to ensure Unicode displayed.
		codePage = SA::CpUtf8;
	}
	wEditor.SetCodePage(codePage);
	const int outputCodePage = props.GetInt("output.code.page", codePage);
	wOutput.SetCodePage(outputCodePage);

	characterSet = static_cast<SA::CharacterSet>(props.GetInt("character.set", static_cast<int>(SA::CharacterSet::Default)));

	SetRepresentations();

#if defined(__unix__) || defined(__APPLE__)
	const std::string localeCType = props.GetString("LC_CTYPE");
	if (localeCType.length())
		setlocale(LC_CTYPE, localeCType.c_str());
	else
		setlocale(LC_CTYPE, "C");
#endif

	std::string imeInteraction = props.GetString("ime.interaction");
	if (imeInteraction.length()) {
		CallChildren(SA::Message::SetIMEInteraction, props.GetInt("ime.interaction", static_cast<int>(SA::IMEInteraction::Windowed)));
	}
	imeAutoComplete = props.GetInt("ime.autocomplete", 0) == 1;

	const SA::Accessibility accessibility = static_cast<SA::Accessibility>(props.GetInt("accessibility", 1));
	wEditor.SetAccessibility(accessibility);
	wOutput.SetAccessibility(accessibility);

	wrapStyle = static_cast<SA::Wrap>(props.GetInt("wrap.style", static_cast<int>(SA::Wrap::Word)));

	CallChildren(SA::Message::SetMouseSelectionRectangularSwitch, props.GetInt("selection.rectangular.switch.mouse", 0));
	CallChildren(SA::Message::SetMultipleSelection, props.GetInt("selection.multiple", 1));
	CallChildren(SA::Message::SetAdditionalSelectionTyping, props.GetInt("selection.additional.typing", 1));
	CallChildren(SA::Message::SetMultiPaste, props.GetInt("selection.multipaste", 1));
	CallChildren(SA::Message::SetAdditionalCaretsBlink, props.GetInt("caret.additional.blinks", 1));
	CallChildren(SA::Message::SetVirtualSpaceOptions, props.GetInt("virtual.space"));

	wEditor.SetMouseDwellTime(props.GetInt("dwell.period", SA::TimeForever));

	SetElementColour(SA::Element::Caret, "caret.fore");
	SetElementColour(SA::Element::CaretAdditional, "caret.additional.fore");

	const SA::CaretStyle caretStyle = static_cast<SA::CaretStyle>(props.GetInt("caret.style", static_cast<int>(SA::CaretStyle::Line)));
	wEditor.SetCaretStyle(caretStyle);
	wOutput.SetCaretStyle(caretStyle);
	wEditor.SetCaretWidth(props.GetInt("caret.width", 1));
	wOutput.SetCaretWidth(props.GetInt("caret.width", 1));

	const std::string caretLineLayer = props.GetExpandedString("caret.line.layer");
	if (caretLineLayer.empty()) {
		std::string caretLineBack = props.GetExpandedString("caret.line.back");
		if (caretLineBack.length()) {
			wEditor.SetCaretLineVisible(true);
			wEditor.SetCaretLineBack(ColourFromString(caretLineBack));
		} else {
			wEditor.SetCaretLineVisible(false);
		}
		wEditor.SetCaretLineBackAlpha(
			static_cast<SA::Alpha>(props.GetInt("caret.line.back.alpha", static_cast<int>(SA::Alpha::NoAlpha))));
	} else {
		// New scheme
		const int layer = IntegerFromString(caretLineLayer, 0);
		CallChildren(SA::Message::SetCaretLineLayer, layer);
		SetElementColour(SA::Element::CaretLineBack, "caret.line.back");
		const int caretLineFrame = props.GetInt("caret.line.frame");
		CallChildren(SA::Message::SetCaretLineFrame, caretLineFrame);
	}

	int indicatorsAlpha = props.GetInt("indicators.alpha", 30);
	if (indicatorsAlpha < 0 || 255 < indicatorsAlpha) // If invalid value,
		indicatorsAlpha = 30; //then set default value.
	alphaIndicator = static_cast<SA::Alpha>(indicatorsAlpha);
	underIndicator = props.GetInt("indicators.under", 0) == 1;

	closeFind = static_cast<CloseFind>(props.GetInt("find.close.on.find", 1));

	const std::string controlCharSymbol = props.GetString("control.char.symbol");
	if (controlCharSymbol.length()) {
		wEditor.SetControlCharSymbol(static_cast<unsigned char>(controlCharSymbol[0]));
	} else {
		wEditor.SetControlCharSymbol(0);
	}

	const std::string caretPeriod = props.GetString("caret.period");
	if (caretPeriod.length()) {
		wEditor.SetCaretPeriod(atoi(caretPeriod.c_str()));
		wOutput.SetCaretPeriod(atoi(caretPeriod.c_str()));
	}

	const int caretZoneX = props.GetInt("caret.policy.width", 50);
	int caretPolicyX = 0;
	if (props.GetInt("caret.policy.xslop", 1))
		caretPolicyX |= static_cast<int>(SA::CaretPolicy::Slop);
	if (props.GetInt("caret.policy.xstrict"))
		caretPolicyX |= static_cast<int>(SA::CaretPolicy::Strict);
	if (props.GetInt("caret.policy.xeven", 1))
		caretPolicyX |= static_cast<int>(SA::CaretPolicy::Even);
	if (props.GetInt("caret.policy.xjumps"))
		caretPolicyX |= static_cast<int>(SA::CaretPolicy::Jumps);
	//wEditor.SetXCaretPolicy(caretStrict | caretSlop | caretEven | caretJumps);
	wEditor.SetXCaretPolicy(static_cast<SA::CaretPolicy>(caretPolicyX), caretZoneX);

	const int caretZoneY = props.GetInt("caret.policy.lines");
	int caretPolicyY = 0;
	if (props.GetInt("caret.policy.yslop", 1))
		caretPolicyY |= static_cast<int>(SA::CaretPolicy::Slop);
	if (props.GetInt("caret.policy.ystrict"))
		caretPolicyY |= static_cast<int>(SA::CaretPolicy::Strict);
	if (props.GetInt("caret.policy.yeven", 1))
		caretPolicyY |= static_cast<int>(SA::CaretPolicy::Even);
	if (props.GetInt("caret.policy.yjumps"))
		caretPolicyY |= static_cast<int>(SA::CaretPolicy::Jumps);
	wEditor.SetYCaretPolicy(static_cast<SA::CaretPolicy>(caretPolicyY), caretZoneY);

	int visiblePolicy = 0;
	if (props.GetInt("visible.policy.strict"))
		visiblePolicy |= static_cast<int>(SA::VisiblePolicy::Strict);
	if (props.GetInt("visible.policy.slop", 1))
		visiblePolicy |= static_cast<int>(SA::VisiblePolicy::Slop);
	const int visibleLines = props.GetInt("visible.policy.lines");
	wEditor.SetVisiblePolicy(static_cast<SA::VisiblePolicy>(visiblePolicy), visibleLines);

	wEditor.SetEdgeColumn(props.GetInt("edge.column", 0));
	wEditor.SetEdgeMode(static_cast<SA::EdgeVisualStyle>(
				    props.GetInt("edge.mode", static_cast<int>(SA::EdgeVisualStyle::None))));
	wEditor.SetEdgeColour(
		ColourOfProperty(props, "edge.colour", ColourRGB(0xff, 0xda, 0xda)));

	const std::string selectionLayer = props.GetExpandedString("selection.layer");
	if (selectionLayer.empty()) {

		std::string selFore = props.GetExpandedString("selection.fore");
		if (selFore.length()) {
			CallChildren(SA::Message::SetSelFore, 1, ColourFromString(selFore));
		} else {
			CallChildren(SA::Message::SetSelFore, 0, 0);
		}
		std::string selBack = props.GetExpandedString("selection.back");
		if (selBack.length()) {
			CallChildren(SA::Message::SetSelBack, 1, ColourFromString(selBack));
		} else {
			if (selFore.length())
				CallChildren(SA::Message::SetSelBack, 0, 0);
			else	// Have to show selection somehow
				CallChildren(SA::Message::SetSelBack, 1, ColourRGB(0xC0, 0xC0, 0xC0));
		}
		constexpr int NoAlpha = static_cast<int>(SA::Alpha::NoAlpha);
		const int selectionAlpha = props.GetInt("selection.alpha", NoAlpha);
		CallChildren(SA::Message::SetSelAlpha, selectionAlpha);

		std::string selAdditionalFore = props.GetString("selection.additional.fore");
		if (selAdditionalFore.length()) {
			CallChildren(SA::Message::SetAdditionalSelFore, ColourFromString(selAdditionalFore));
		}
		std::string selAdditionalBack = props.GetString("selection.additional.back");
		if (selAdditionalBack.length()) {
			CallChildren(SA::Message::SetAdditionalSelBack, ColourFromString(selAdditionalBack));
		}
		const int selectionAdditionalAlpha = (selectionAlpha == NoAlpha) ? NoAlpha : selectionAlpha / 2;
		CallChildren(SA::Message::SetAdditionalSelAlpha, props.GetInt("selection.additional.alpha", selectionAdditionalAlpha));

	} else {
		// New scheme
		const int layer = IntegerFromString(selectionLayer, 0);
		CallChildren(SA::Message::SetSelectionLayer, layer);
		SetElementColour(SA::Element::SelectionText, "selection.fore");
		SetElementColour(SA::Element::SelectionBack, "selection.back");
		SetElementColour(SA::Element::SelectionAdditionalText, "selection.additional.fore");
		SetElementColour(SA::Element::SelectionAdditionalBack, "selection.additional.back");
		SetElementColour(SA::Element::SelectionSecondaryText, "selection.secondary.fore");
		SetElementColour(SA::Element::SelectionSecondaryBack, "selection.secondary.back");
		SetElementColour(SA::Element::SelectionInactiveText, "selection.inactive.fore");
		SetElementColour(SA::Element::SelectionInactiveBack, "selection.inactive.back");
	}

	foldColour = props.GetExpandedString("fold.margin.colour");
	if (foldColour.length()) {
		CallChildren(SA::Message::SetFoldMarginColour, 1, ColourFromString(foldColour));
	} else {
		CallChildren(SA::Message::SetFoldMarginColour, 0, 0);
	}
	foldHiliteColour = props.GetExpandedString("fold.margin.highlight.colour");
	if (foldHiliteColour.length()) {
		CallChildren(SA::Message::SetFoldMarginHiColour, 1, ColourFromString(foldHiliteColour));
	} else {
		CallChildren(SA::Message::SetFoldMarginHiColour, 0, 0);
	}

	SetElementColour(SA::Element::WhiteSpace, "whitespace.fore");
	SetElementColour(SA::Element::WhiteSpaceBack, "whitespace.back");

	CallChildren(SA::Message::SetWhitespaceSize, props.GetInt("whitespace.size", 1));

	char bracesStyleKey[200];
	sprintf(bracesStyleKey, "braces.%s.style", language.c_str());
	bracesStyle = props.GetInt(bracesStyleKey, 0);

	char key[200] = "";
	std::string sval;

	sval = FindLanguageProperty("calltip.*.ignorecase");
	callTipIgnoreCase = sval == "1";
	sval = FindLanguageProperty("calltip.*.use.escapes");
	callTipUseEscapes = sval == "1";

	calltipWordCharacters = FindLanguageProperty("calltip.*.word.characters",
				"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
	calltipParametersStart = FindLanguageProperty("calltip.*.parameters.start", "(");
	calltipParametersEnd = FindLanguageProperty("calltip.*.parameters.end", ")");
	calltipParametersSeparators = FindLanguageProperty("calltip.*.parameters.separators", ",;");

	calltipEndDefinition = FindLanguageProperty("calltip.*.end.definition");

	sprintf(key, "autocomplete.%s.start.characters", language.c_str());
	autoCompleteStartCharacters = props.GetExpandedString(key);
	if (autoCompleteStartCharacters == "")
		autoCompleteStartCharacters = props.GetExpandedString("autocomplete.*.start.characters");
	// "" is a quite reasonable value for this setting

	sprintf(key, "autocomplete.%s.fillups", language.c_str());
	autoCompleteFillUpCharacters = props.GetExpandedString(key);
	if (autoCompleteFillUpCharacters == "")
		autoCompleteFillUpCharacters =
			props.GetExpandedString("autocomplete.*.fillups");
	wEditor.AutoCSetFillUps(autoCompleteFillUpCharacters.c_str());

	sprintf(key, "autocomplete.%s.typesep", language.c_str());
	autoCompleteTypeSeparator = props.GetExpandedString(key);
	if (autoCompleteTypeSeparator == "")
		autoCompleteTypeSeparator =
			props.GetExpandedString("autocomplete.*.typesep");
	if (autoCompleteTypeSeparator.length()) {
		wEditor.AutoCSetTypeSeparator(
			static_cast<unsigned char>(autoCompleteTypeSeparator[0]));
	}

	sprintf(key, "autocomplete.%s.ignorecase", "*");
	sval = props.GetNewExpandString(key);
	autoCompleteIgnoreCase = sval == "1";
	sprintf(key, "autocomplete.%s.ignorecase", language.c_str());
	sval = props.GetNewExpandString(key);
	if (sval != "")
		autoCompleteIgnoreCase = sval == "1";
	wEditor.AutoCSetIgnoreCase(autoCompleteIgnoreCase);
	wOutput.AutoCSetIgnoreCase(true);
	autoCompleteVisibleItemCount = props.GetInt("autocomplete.visible.item.count", 9);

	const int autoCChooseSingle = props.GetInt("autocomplete.choose.single");
	wEditor.AutoCSetChooseSingle(autoCChooseSingle);

	wEditor.AutoCSetCancelAtStart(false);
	wEditor.AutoCSetDropRestOfWord(false);

	OptionalSetColour(wEditor, SA::Element::List, props, "autocomplete.fore");
	OptionalSetColour(wEditor, SA::Element::ListBack, props, "autocomplete.back");
	OptionalSetColour(wEditor, SA::Element::ListSelected, props, "autocomplete.selected.fore");
	OptionalSetColour(wEditor, SA::Element::ListSelectedBack, props, "autocomplete.selected.back");

	if (firstPropertiesRead) {
		ReadPropertiesInitial();
	}

	ReadFontProperties();

	wEditor.SetPrintMagnification(props.GetInt("print.magnification"));
	wEditor.SetPrintColourMode(static_cast<SA::PrintOption>(props.GetInt("print.colour.mode")));

	jobQueue.clearBeforeExecute = props.GetInt("clear.before.execute");
	jobQueue.timeCommands = props.GetInt("time.commands");

	const int blankMarginLeft = props.GetInt("blank.margin.left", 1);
	const int blankMarginLeftOutput = props.GetInt("output.blank.margin.left", blankMarginLeft);
	const int blankMarginRight = props.GetInt("blank.margin.right", 1);
	wEditor.SetMarginLeft(blankMarginLeft);
	wEditor.SetMarginRight(blankMarginRight);
	wOutput.SetMarginLeft(blankMarginLeftOutput);
	wOutput.SetMarginRight(blankMarginRight);

	marginWidth = props.GetInt("margin.width");
	if (marginWidth == 0)
		marginWidth = marginWidthDefault;
	wEditor.SetMarginWidthN(1, margin ? marginWidth : 0);

	const std::string lineMarginProp = props.GetString("line.margin.width");
	lineNumbersWidth = atoi(lineMarginProp.c_str());
	if (lineNumbersWidth == 0)
		lineNumbersWidth = lineNumbersWidthDefault;
	lineNumbersExpand = lineMarginProp.find('+') != std::string::npos;

	SetLineNumberWidth();

	bufferedDraw = props.GetInt("buffered.draw");
	wEditor.SetBufferedDraw(bufferedDraw);
	wOutput.SetBufferedDraw(bufferedDraw);

	const SA::PhasesDraw phasesDraw = static_cast<SA::PhasesDraw>(
			props.GetInt("phases.draw", static_cast<int>(SA::PhasesDraw::Two)));
	wEditor.SetPhasesDraw(phasesDraw);
	wOutput.SetPhasesDraw(phasesDraw);

	wEditor.SetLayoutCache(static_cast<SA::LineCache>(
				       props.GetInt("cache.layout", static_cast<int>(SA::LineCache::Caret))));
	wOutput.SetLayoutCache(static_cast<SA::LineCache>(
				       props.GetInt("output.cache.layout", static_cast<int>(SA::LineCache::Caret))));

	wEditor.SetLayoutThreads(props.GetInt("threads.layout", 1));

	bracesCheck = props.GetInt("braces.check");
	bracesSloppy = props.GetInt("braces.sloppy");

	wEditor.SetCharsDefault();
	wordCharacters = props.GetNewExpandString("word.characters.", fileNameForExtension.c_str());
	if (wordCharacters.length()) {
		wEditor.SetWordChars(wordCharacters.c_str());
	} else {
		wordCharacters = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	}

	whitespaceCharacters = props.GetNewExpandString("whitespace.characters.", fileNameForExtension.c_str());
	if (whitespaceCharacters.length()) {
		wEditor.SetWhitespaceChars(whitespaceCharacters.c_str());
	}

	const std::string viewIndentExamine = GetFileNameProperty("view.indentation.examine");
	indentExamine = viewIndentExamine.length() ? static_cast<SA::IndentView>(atoi(viewIndentExamine.c_str())) : SA::IndentView::Real;
	wEditor.SetIndentationGuides(props.GetInt("view.indentation.guides") ?
				     indentExamine : SA::IndentView::None);

	wEditor.SetTabIndents(props.GetInt("tab.indents", 1));
	wEditor.SetBackSpaceUnIndents(props.GetInt("backspace.unindents", 1));

	wEditor.CallTipUseStyle(32);

	std::string useStripTrailingSpaces = props.GetNewExpandString("strip.trailing.spaces.", ExtensionFileName().c_str());
	if (useStripTrailingSpaces.length() > 0) {
		stripTrailingSpaces = atoi(useStripTrailingSpaces.c_str()) != 0;
	} else {
		stripTrailingSpaces = props.GetInt("strip.trailing.spaces") != 0;
	}
	ensureFinalLineEnd = props.GetInt("ensure.final.line.end") != 0;
	ensureConsistentLineEnds = props.GetInt("ensure.consistent.line.ends") != 0;

	indentOpening = props.GetInt("indent.opening");
	indentClosing = props.GetInt("indent.closing");
	indentMaintain = atoi(props.GetNewExpandString("indent.maintain.", fileNameForExtension.c_str()).c_str());

	const std::string lookback = props.GetNewExpandString("statement.lookback.", fileNameForExtension.c_str());
	statementLookback = atoi(lookback.c_str());
	statementIndent = GetStyleAndWords("statement.indent.");
	statementEnd = GetStyleAndWords("statement.end.");
	blockStart = GetStyleAndWords("block.start.");
	blockEnd = GetStyleAndWords("block.end.");

	struct PropToPPC {
		const char *propName;
		PreProc ppc;
	};
	const PropToPPC propToPPC[] = {
		{"preprocessor.start.", PreProc::Start},
		{"preprocessor.middle.", PreProc::Middle},
		{"preprocessor.end.", PreProc::End},
	};
	const std::string ppSymbol = props.GetNewExpandString("preprocessor.symbol.", fileNameForExtension.c_str());
	preprocessorSymbol = ppSymbol.empty() ? 0 : ppSymbol[0];
	preprocOfString.clear();
	for (const PropToPPC &preproc : propToPPC) {
		const std::string list = props.GetNewExpandString(preproc.propName, fileNameForExtension.c_str());
		const std::vector<std::string> words = StringSplit(list, ' ');
		for (const std::string &word : words) {
			preprocOfString[word] = preproc.ppc;
		}
	}

	std::vector<std::string> fileSets = StringSplit(props.GetNewExpandString("find.files"), '|');
	for (const std::string &fileSet : fileSets) {
		memFiles.Append(fileSet);
	}

	wEditor.SetWrapVisualFlags(static_cast<SA::WrapVisualFlag>(props.GetInt("wrap.visual.flags")));
	wEditor.SetWrapVisualFlagsLocation(static_cast<SA::WrapVisualLocation>(props.GetInt("wrap.visual.flags.location")));
	wEditor.SetWrapStartIndent(props.GetInt("wrap.visual.startindent"));
	wEditor.SetWrapIndentMode(static_cast<SA::WrapIndentMode>(props.GetInt("wrap.indent.mode")));

	idleStyling = static_cast<SA::IdleStyling>(props.GetInt("idle.styling", static_cast<int>(SA::IdleStyling::None)));
	wEditor.SetIdleStyling(idleStyling);
	wOutput.SetIdleStyling(static_cast<SA::IdleStyling>(props.GetInt("output.idle.styling", static_cast<int>(SA::IdleStyling::None))));

	if (props.GetInt("os.x.home.end.keys")) {
		AssignKey(SA::Keys::Home, SA::KeyMod::Norm, SCI_SCROLLTOSTART);
		AssignKey(SA::Keys::Home, SA::KeyMod::Shift, SCI_NULL);
		AssignKey(SA::Keys::Home, SA::KeyMod::Shift | SA::KeyMod::Alt, SCI_NULL);
		AssignKey(SA::Keys::End, SA::KeyMod::Norm, SCI_SCROLLTOEND);
		AssignKey(SA::Keys::End, SA::KeyMod::Shift, SCI_NULL);
	} else {
		if (props.GetInt("wrap.aware.home.end.keys", 0)) {
			if (props.GetInt("vc.home.key", 1)) {
				AssignKey(SA::Keys::Home, SA::KeyMod::Norm, SCI_VCHOMEWRAP);
				AssignKey(SA::Keys::Home, SA::KeyMod::Shift, SCI_VCHOMEWRAPEXTEND);
				AssignKey(SA::Keys::Home, SA::KeyMod::Shift | SA::KeyMod::Alt, SCI_VCHOMERECTEXTEND);
			} else {
				AssignKey(SA::Keys::Home, SA::KeyMod::Norm, SCI_HOMEWRAP);
				AssignKey(SA::Keys::Home, SA::KeyMod::Shift, SCI_HOMEWRAPEXTEND);
				AssignKey(SA::Keys::Home, SA::KeyMod::Shift | SA::KeyMod::Alt, SCI_HOMERECTEXTEND);
			}
			AssignKey(SA::Keys::End, SA::KeyMod::Norm, SCI_LINEENDWRAP);
			AssignKey(SA::Keys::End, SA::KeyMod::Shift, SCI_LINEENDWRAPEXTEND);
		} else {
			if (props.GetInt("vc.home.key", 1)) {
				AssignKey(SA::Keys::Home, SA::KeyMod::Norm, SCI_VCHOME);
				AssignKey(SA::Keys::Home, SA::KeyMod::Shift, SCI_VCHOMEEXTEND);
				AssignKey(SA::Keys::Home, SA::KeyMod::Shift | SA::KeyMod::Alt, SCI_VCHOMERECTEXTEND);
			} else {
				AssignKey(SA::Keys::Home, SA::KeyMod::Norm, SCI_HOME);
				AssignKey(SA::Keys::Home, SA::KeyMod::Shift, SCI_HOMEEXTEND);
				AssignKey(SA::Keys::Home, SA::KeyMod::Shift | SA::KeyMod::Alt, SCI_HOMERECTEXTEND);
			}
			AssignKey(SA::Keys::End, SA::KeyMod::Norm, SCI_LINEEND);
			AssignKey(SA::Keys::End, SA::KeyMod::Shift, SCI_LINEENDEXTEND);
		}
	}

	AssignKey(static_cast<SA::Keys>('L'), SA::KeyMod::Shift | SA::KeyMod::Ctrl, SCI_LINEDELETE);


	scrollOutput = props.GetInt("output.scroll", 1);

	tabHideOne = props.GetInt("tabbar.hide.one");

	SetToolsMenu();

	wEditor.SetFoldFlags(static_cast<SA::FoldFlag>(props.GetInt("fold.flags")));

	// To put the folder markers in the line number region
	//wEditor.SetMarginMaskN(0, SC_MASK_FOLDERS);

	wEditor.SetModEventMask(SA::ModificationFlags::ChangeFold);

	if (0==props.GetInt("undo.redo.lazy")) {
		// Trap for insert/delete notifications (also fired by undo
		// and redo) so that the buttons can be enabled if needed.
		const SA::ModificationFlags flagsCurrent = wEditor.ModEventMask();
		const SA::ModificationFlags flags =
				flagsCurrent |
				SA::ModificationFlags::InsertText |
				SA::ModificationFlags::DeleteText |
				SA::ModificationFlags::LastStepInUndoRedo;
		wEditor.SetModEventMask(flags);

		// LastStepInUndoRedo is probably not needed in the mask; it
		// doesn't seem to fire as an event of its own; just modifies the
		// insert and delete events.
	}

	// Create a margin column for the folding symbols
	wEditor.SetMarginTypeN(2, SA::MarginType::Symbol);

	foldMarginWidth = props.GetInt("fold.margin.width");
	if (foldMarginWidth == 0)
		foldMarginWidth = foldMarginWidthDefault;
	SetFoldWidth();
	wEditor.SetMarginMaskN(2, SA::MaskFolders);
	wEditor.SetMarginSensitiveN(2, true);

	// Define foreground (outline) and background (fill) colour of folds
	const int foldSymbols = props.GetInt("fold.symbols");
	std::string foldFore = props.GetExpandedString("fold.fore");
	if (foldFore.length() == 0) {
		// Set default colour for outline
		switch (foldSymbols) {
		case 0: // Arrows
			foldFore = "#000000";
			break;
		case 1: // + -
			foldFore = "#FFFFFF";
			break;
		case 2: // Circles
			foldFore = "#404040";
			break;
		case 3: // Squares
			foldFore = "#808080";
			break;
		}
	}
	const SA::ColourAlpha colourFoldFore = ColourAlphaFromString(foldFore);

	std::string foldBack = props.GetExpandedString("fold.back");
	// Set default colour for fill
	if (foldBack.length() == 0) {
		switch (foldSymbols) {
		case 0:
		case 1:
			foldBack = "#000000";
			break;
		case 2:
		case 3:
			foldBack = "#FFFFFF";
			break;
		}
	}
	const SA::ColourAlpha colourFoldBack = ColourAlphaFromString(foldBack);

	// Enable/disable highlight for current folding block (smallest one that contains the caret)
	const int isHighlightEnabled = props.GetInt("fold.highlight", 0);
	// Define the colour of highlight
	const SA::ColourAlpha colourFoldBlockHighlight = ColourAlphaOfProperty(
		props, "fold.highlight.colour", ColourRGBA(0xFF, 0, 0));

	const int foldStrokeWidth = props.GetInt("fold.stroke.width", 100);
	// Isolated and connected fold markers use foreground and background colours differently
	MarkerAppearance isolated {
		colourFoldFore, colourFoldBack, colourFoldBlockHighlight, foldStrokeWidth
	};
	MarkerAppearance connected {
		colourFoldBack, colourFoldFore, colourFoldBlockHighlight, foldStrokeWidth
	};
	switch (foldSymbols) {
	case 0:
		// Arrow pointing right for contracted folders, arrow pointing down for expanded
		DefineMarker(SA::MarkerOutline::FolderOpen, SA::MarkerSymbol::ArrowDown, isolated);
		DefineMarker(SA::MarkerOutline::Folder, SA::MarkerSymbol::Arrow, isolated);
		DefineMarker(SA::MarkerOutline::FolderSub, SA::MarkerSymbol::Empty, isolated);
		DefineMarker(SA::MarkerOutline::FolderTail, SA::MarkerSymbol::Empty, isolated);
		DefineMarker(SA::MarkerOutline::FolderEnd, SA::MarkerSymbol::Empty, isolated);
		DefineMarker(SA::MarkerOutline::FolderOpenMid, SA::MarkerSymbol::Empty, isolated);
		DefineMarker(SA::MarkerOutline::FolderMidTail, SA::MarkerSymbol::Empty, isolated);
		// The highlight is disabled for arrow.
		wEditor.MarkerEnableHighlight(false);
		break;
	case 1:
		// Plus for contracted folders, minus for expanded
		DefineMarker(SA::MarkerOutline::FolderOpen, SA::MarkerSymbol::Minus, isolated);
		DefineMarker(SA::MarkerOutline::Folder, SA::MarkerSymbol::Plus, isolated);
		DefineMarker(SA::MarkerOutline::FolderSub, SA::MarkerSymbol::Empty, isolated);
		DefineMarker(SA::MarkerOutline::FolderTail, SA::MarkerSymbol::Empty, isolated);
		DefineMarker(SA::MarkerOutline::FolderEnd, SA::MarkerSymbol::Empty, isolated);
		DefineMarker(SA::MarkerOutline::FolderOpenMid, SA::MarkerSymbol::Empty, isolated);
		DefineMarker(SA::MarkerOutline::FolderMidTail, SA::MarkerSymbol::Empty, isolated);
		// The highlight is disabled for plus/minus.
		wEditor.MarkerEnableHighlight(false);
		break;
	case 2:
		// Like a flattened tree control using circular headers and curved joins
		DefineMarker(SA::MarkerOutline::FolderOpen, SA::MarkerSymbol::CircleMinus, connected);
		DefineMarker(SA::MarkerOutline::Folder, SA::MarkerSymbol::CirclePlus, connected);
		DefineMarker(SA::MarkerOutline::FolderSub, SA::MarkerSymbol::VLine, connected);
		DefineMarker(SA::MarkerOutline::FolderTail, SA::MarkerSymbol::LCornerCurve, connected);
		DefineMarker(SA::MarkerOutline::FolderEnd, SA::MarkerSymbol::CirclePlusConnected, connected);
		DefineMarker(SA::MarkerOutline::FolderOpenMid, SA::MarkerSymbol::CircleMinusConnected, connected);
		DefineMarker(SA::MarkerOutline::FolderMidTail, SA::MarkerSymbol::TCornerCurve, connected);
		wEditor.MarkerEnableHighlight(isHighlightEnabled);
		break;
	case 3:
		// Like a flattened tree control using square headers
		DefineMarker(SA::MarkerOutline::FolderOpen, SA::MarkerSymbol::BoxMinus, connected);
		DefineMarker(SA::MarkerOutline::Folder, SA::MarkerSymbol::BoxPlus, connected);
		DefineMarker(SA::MarkerOutline::FolderSub, SA::MarkerSymbol::VLine, connected);
		DefineMarker(SA::MarkerOutline::FolderTail, SA::MarkerSymbol::LCorner, connected);
		DefineMarker(SA::MarkerOutline::FolderEnd, SA::MarkerSymbol::BoxPlusConnected, connected);
		DefineMarker(SA::MarkerOutline::FolderOpenMid, SA::MarkerSymbol::BoxMinusConnected, connected);
		DefineMarker(SA::MarkerOutline::FolderMidTail, SA::MarkerSymbol::TCorner, connected);
		wEditor.MarkerEnableHighlight(isHighlightEnabled);
		break;
	}

	wEditor.MarkerSetForeTranslucent(markerBookmark,
			      ColourAlphaOfProperty(props, "bookmark.fore", ColourRGBA(0xbe, 0, 0)));
	wEditor.MarkerSetBackTranslucent(markerBookmark,
			      ColourAlphaOfProperty(props, "bookmark.back", ColourRGBA(0xe2, 0x40, 0x40)));
	wEditor.MarkerSetAlpha(markerBookmark,
			       static_cast<SA::Alpha>(props.GetInt("bookmark.alpha", static_cast<int>(SA::Alpha::NoAlpha))));
	wEditor.MarkerSetStrokeWidth(markerBookmark, props.GetInt("bookmark.stroke.width", 100));

	const std::string bookMarkXPM = props.GetString("bookmark.pixmap");
	if (bookMarkXPM.length()) {
		wEditor.MarkerDefinePixmap(markerBookmark, bookMarkXPM.c_str());
	} else if (props.GetString("bookmark.fore").length()) {
		wEditor.MarkerDefine(markerBookmark, static_cast<SA::MarkerSymbol>(
					     props.GetInt("bookmark.symbol", static_cast<int>(SA::MarkerSymbol::Bookmark))));
	} else {
		// No bookmark.fore setting so display default pixmap.
		wEditor.MarkerDefinePixmap(markerBookmark, reinterpret_cast<const char *>(bookmarkBluegem));
	}

	wEditor.MarkerDefine(markerFilterMatch, SA::MarkerSymbol::Background);
	wEditor.MarkerSetLayer(markerFilterMatch, Scintilla::Layer::UnderText);
	const SA::ColourAlpha filterMatch = ColourAlphaOfProperty(props, "filter.match.back", ColourRGBA(0xff, 0xe0, 0, 0x20));
	wEditor.MarkerSetBackTranslucent(markerFilterMatch, filterMatch);
	wEditor.MarkerSetAlpha(markerFilterMatch, static_cast<SA::Alpha>(filterMatch >> 24));

	wEditor.SetScrollWidth(props.GetInt("horizontal.scroll.width", 2000));
	wEditor.SetScrollWidthTracking(props.GetInt("horizontal.scroll.width.tracking", 1));
	wOutput.SetScrollWidth(props.GetInt("output.horizontal.scroll.width", 2000));
	wOutput.SetScrollWidthTracking(props.GetInt("output.horizontal.scroll.width.tracking", 1));

	// Do these last as they force a style refresh
	wEditor.SetHScrollBar(props.GetInt("horizontal.scrollbar", 1));
	wOutput.SetHScrollBar(props.GetInt("output.horizontal.scrollbar", 1));

	wEditor.SetEndAtLastLine(props.GetInt("end.at.last.line", 1));
	wEditor.SetCaretSticky(static_cast<SA::CaretSticky>(props.GetInt("caret.sticky", 0)));

	// Clear all previous indicators.
	wEditor.SetIndicatorCurrent(indicatorHighlightCurrentWord);
	wEditor.IndicatorClearRange(0, wEditor.Length());
	wOutput.SetIndicatorCurrent(indicatorHighlightCurrentWord);
	wOutput.IndicatorClearRange(0, wOutput.Length());
	currentWordHighlight.statesOfDelay = CurrentWordHighlight::StatesOfDelay::noDelay;

	SetElementColour(SA::Element::FoldLine, "fold.line.colour");
	SetElementColour(SA::Element::HiddenLine, "hidden.line.colour");

	currentWordHighlight.isEnabled = props.GetInt("highlight.current.word", 0) == 1;
	if (currentWordHighlight.isEnabled) {
		const std::string highlightCurrentWordIndicatorString = props.GetExpandedString("highlight.current.word.indicator");
		IndicatorDefinition highlightCurrentWordIndicator(highlightCurrentWordIndicatorString.c_str());
		if (highlightCurrentWordIndicatorString.length() == 0) {
			highlightCurrentWordIndicator.style = SA::IndicatorStyle::RoundBox;
			std::string highlightCurrentWordColourString = props.GetExpandedString("highlight.current.word.colour");
			if (highlightCurrentWordColourString.length() == 0) {
				// Set default colour for highlight.
				highlightCurrentWordColourString = "#A0A000";
			}
			highlightCurrentWordIndicator.colour = ColourFromString(highlightCurrentWordColourString);
			highlightCurrentWordIndicator.fillAlpha = alphaIndicator;
			highlightCurrentWordIndicator.under = underIndicator;
		}
		const SA::IndicatorNumbers indicatorNumHCWI = static_cast<SA::IndicatorNumbers>(indicatorHighlightCurrentWord);
		SetOneIndicator(wEditor, indicatorNumHCWI,	highlightCurrentWordIndicator);
		SetOneIndicator(wOutput, indicatorNumHCWI, highlightCurrentWordIndicator);
		currentWordHighlight.isOnlyWithSameStyle = props.GetInt("highlight.current.word.by.style", 0) == 1;
		HighlightCurrentWord(true);
	}

	SetIndicatorFromProperty(wEditor, SA::IndicatorNumbers::HistoryRevertedToOriginInsertion, "indicator.reverted.to.origin.insertion");
	SetIndicatorFromProperty(wEditor, SA::IndicatorNumbers::HistoryRevertedToOriginDeletion, "indicator.reverted.to.origin.deletion");
	SetIndicatorFromProperty(wEditor, SA::IndicatorNumbers::HistorySavedInsertion, "indicator.saved.insertion");
	SetIndicatorFromProperty(wEditor, SA::IndicatorNumbers::HistorySavedDeletion, "indicator.saved.deletion");
	SetIndicatorFromProperty(wEditor, SA::IndicatorNumbers::HistoryModifiedInsertion, "indicator.modified.insertion");
	SetIndicatorFromProperty(wEditor, SA::IndicatorNumbers::HistoryModifiedDeletion, "indicator.modified.deletion");
	SetIndicatorFromProperty(wEditor, SA::IndicatorNumbers::HistoryRevertedToModifiedInsertion, "indicator.reverted.to.modified.insertion");
	SetIndicatorFromProperty(wEditor, SA::IndicatorNumbers::HistoryRevertedToModifiedDeletion, "indicator.reverted.to.modified.deletion");

	constexpr size_t markerHistory = static_cast<size_t>(SA::MarkerOutline::HistoryRevertedToOrigin);
	SetMarkerFromProperty(wEditor, markerHistory+0, "marker.reverted.to.origin");
	SetMarkerFromProperty(wEditor, markerHistory+1, "marker.saved");
	SetMarkerFromProperty(wEditor, markerHistory+2, "marker.modified");
	SetMarkerFromProperty(wEditor, markerHistory+3, "marker.reverted.to.modified");

	ReadEditorConfig(fileNameForExtension);

	if (extender) {
		FilePath defaultDir = GetDefaultDirectory();
		FilePath scriptPath;

		// Check for an extension script
		GUI::gui_string extensionFile = GUI::StringFromUTF8(
							props.GetNewExpandString("extension.", fileNameForExtension.c_str()));
		if (extensionFile.length()) {
			// find file in local directory
			FilePath docDir = filePath.Directory();
			if (Exists(docDir.AsInternal(), extensionFile.c_str(), &scriptPath)) {
				// Found file in document directory
				extender->Load(scriptPath.AsUTF8().c_str());
			} else if (Exists(defaultDir.AsInternal(), extensionFile.c_str(), &scriptPath)) {
				// Found file in global directory
				extender->Load(scriptPath.AsUTF8().c_str());
			} else if (Exists(GUI_TEXT(""), extensionFile.c_str(), &scriptPath)) {
				// Found as completely specified file name
				extender->Load(scriptPath.AsUTF8().c_str());
			}
		}
	}

	delayBeforeAutoSave = props.GetInt("save.on.timer");
	if (delayBeforeAutoSave) {
		TimerStart(timerAutoSave);
	} else {
		TimerEnd(timerAutoSave);
	}

	firstPropertiesRead = false;
	needReadProperties = false;
}

void SciTEBase::ReadEditorConfig(const std::string &fileNameForExtension) {
	std::map<std::string, std::string> eConfig = editorConfig->MapFromAbsolutePath(filePath);
	for (const std::pair<const std::string, std::string> &pss : eConfig) {
		try {
			if (pss.first == "indent_style") {
				const bool useTabs = pss.second == "tab";
				propsDiscovered.Set("use.tabs", useTabs ? "1" : "0");
				propsDiscovered.Set("use.tabs." + fileNameForExtension, useTabs ? "1" : "0");
				wEditor.SetUseTabs(useTabs);
			} else if (pss.first == "indent_size") {
				std::string sIndentSize = pss.second;
				if (sIndentSize == "tab") {
					sIndentSize = props.GetExpandedString("tabsize");
					if (sIndentSize.empty()) {
						sIndentSize = "8";
					}
				}
				propsDiscovered.Set("indent.size", sIndentSize);
				propsDiscovered.Set("indent.size." + fileNameForExtension, sIndentSize);
				wEditor.SetIndent(std::stoi(sIndentSize));
			} else if (pss.first == "tab_width") {
				propsDiscovered.Set("tabsize", pss.second);
				propsDiscovered.Set("tab.size." + fileNameForExtension, pss.second);
				wEditor.SetTabWidth(std::stoi(pss.second));
			} else if (pss.first == "end_of_line") {
				if (pss.second == "lf") {
					propsDiscovered.Set("eol.mode", "LF");
					wEditor.SetEOLMode(SA::EndOfLine::Lf);
				} else if (pss.second == "cr") {
					propsDiscovered.Set("eol.mode", "CR");
					wEditor.SetEOLMode(SA::EndOfLine::Cr);
				} else if (pss.second == "crlf") {
					propsDiscovered.Set("eol.mode", "CRLF");
					wEditor.SetEOLMode(SA::EndOfLine::CrLf);
				}
			} else if (pss.first == "charset") {
				if (pss.second == "latin1") {
					CurrentBuffer()->unicodeMode = UniMode::uni8Bit;
					codePage = 0;
				} else {
					if (pss.second == "utf-8")
						CurrentBuffer()->unicodeMode = UniMode::cookie;
					if (pss.second == "utf-8-bom")
						CurrentBuffer()->unicodeMode = UniMode::utf8;
					if (pss.second == "utf-16be")
						CurrentBuffer()->unicodeMode = UniMode::uni16BE;
					if (pss.second == "utf-16le")
						CurrentBuffer()->unicodeMode = UniMode::uni16LE;
					codePage = SA::CpUtf8;
				}
				wEditor.SetCodePage(codePage);
			} else if (pss.first == "trim_trailing_whitespace") {
				stripTrailingSpaces = pss.second == "true";
			} else if (pss.first == "insert_final_newline") {
				ensureFinalLineEnd = pss.second == "true";
			}
		} catch (std::invalid_argument &) {
			std::string diagnostic = "Invalid argument in .editorconfig '";
			diagnostic += pss.first;
			diagnostic += "=";
			diagnostic += pss.second;
			diagnostic += "'.\n";
			OutputAppendString(diagnostic.c_str());
			SetOutputVisibility(true);
		}
	}
}

namespace {

// Scintillua's style numbers are not constant, so ask it for names of styles and create a
// mapping of style numbers to more constant style definitions.
// For example, if Scintillua reports for the cpp lexer that style number 2 is a 'comment',
// create the property:
//   style.scintillua.cpp.2=$(scintillua.styles.comment)
// That way the user can define 'scintillua.styles.comment' once and it will be used for whatever
// the style number for comments is in any given lexer.
// Similarly, if Scintillua reports for the lua lexer that style number 20 is 'string.longstring',
// create the property:
//   style.scintillua.lua.20=$(scintillua.styles.string),$(scintillua.styles.string.longstring)
void SetScintilluaStyles(GUI::ScintillaWindow &wEditor, PropSetFile& props, const char *languageName) {
	const auto setStyle = [&wEditor, &props, &languageName](int style) {
		std::string finalPropStr;
		const std::string &name = wEditor.NameOfStyle(style);
		size_t end = std::string::npos;
		do {
			end = name.find('.', ++end);
			char propStr[128] = "";
			sprintf(propStr, "$(scintillua.styles.%s),", end == std::string::npos ?
				name.c_str() : name.substr(0, end).c_str());
			finalPropStr += propStr;
		} while (end != std::string::npos);
		char key[256] = "";
		sprintf(key, "style.%s.%0d", languageName, style);
		props.Set(key, finalPropStr.c_str());
	};
	const int namedStyles = wEditor.NamedStyles(); // this count includes predefined styles
	constexpr int LastPredefined = static_cast<int>(Scintilla::StylesCommon::LastPredefined);
	constexpr int numPredefined = LastPredefined - StyleDefault + 1;
	for (int i = 0; i < std::min(namedStyles - numPredefined, StyleDefault); i++) {
		setStyle(i);
	}
	for (int i = StyleDefault; i <= LastPredefined; i++) {
		setStyle(i);
	}
	for (int i = LastPredefined + 1; i < namedStyles; i++) {
		setStyle(i);
	}
}

}

void SciTEBase::ReadFontProperties() {
	const std::string monospaceFonts = props.GetExpandedString("font.monospaced.list");
	monospacedList = StringSplit(monospaceFonts, ';');

	char key[200] = "";
	const char *languageName = language.c_str();

	if (StartsWith(languageName, "scintillua.") && language.length() < 240) {
		SetScintilluaStyles(wEditor, props, languageName);
	}

	// Set styles
	// For each window set the global default style, then the language default style, then the other global styles, then the other language styles

	const SA::FontQuality fontQuality = static_cast<SA::FontQuality>(props.GetInt("font.quality"));
	wEditor.SetFontQuality(fontQuality);
	wOutput.SetFontQuality(fontQuality);

	wEditor.StyleResetDefault();
	wOutput.StyleResetDefault();

	std::string fontLocale = props.GetExpandedString("font.locale");
	if (!fontLocale.empty()) {
		wEditor.SetFontLocale(fontLocale.c_str());
		wOutput.SetFontLocale(fontLocale.c_str());
	}

	sprintf(key, "style.%s.%0d", "*", StyleDefault);
	std::string sval = props.GetNewExpandString(key);
	SetOneStyle(wEditor, StyleDefault, StyleDefinition(sval));
	SetOneStyle(wOutput, StyleDefault, StyleDefinition(sval));

	sprintf(key, "style.%s.%0d", languageName, StyleDefault);
	sval = props.GetNewExpandString(key);
	SetOneStyle(wEditor, StyleDefault, StyleDefinition(sval));

	wEditor.StyleClearAll();

	SetStyleFor(wEditor, "*");
	SetStyleFor(wEditor, languageName);
	if (props.GetInt("error.inline")) {
		wEditor.ReleaseAllExtendedStyles();
		diagnosticStyleStart = wEditor.AllocateExtendedStyles(diagnosticStyles);
		SetStyleBlock(wEditor, "error", diagnosticStyleStart, diagnosticStyleStart+diagnosticStyles-1);
	}

	const int diffToSecondary = static_cast<int>(wEditor.DistanceToSecondaryStyles());
	for (const unsigned char subStyleBase : subStyleBases) {
		const int subStylesStart = wEditor.SubStylesStart(subStyleBase);
		const int subStylesLength = wEditor.SubStylesLength(subStyleBase);
		for (int subStyle=0; subStyle<subStylesLength; subStyle++) {
			for (int active=0; active<(diffToSecondary?2:1); active++) {
				const int activity = active * diffToSecondary;
				sprintf(key, "style.%s.%0d.%0d", languageName, subStyleBase + activity, subStyle+1);
				sval = props.GetNewExpandString(key);
				SetOneStyle(wEditor, subStylesStart + subStyle + activity, StyleDefinition(sval));
			}
		}
	}

	// Turn grey while loading
	if (CurrentBuffer()->lifeState == Buffer::LifeState::reading)
		wEditor.StyleSetBack(StyleDefault, 0xEEEEEE);

	wOutput.StyleClearAll();

	sprintf(key, "style.%s.%0d", "errorlist", StyleDefault);
	sval = props.GetNewExpandString(key);
	SetOneStyle(wOutput, StyleDefault, StyleDefinition(sval));

	wOutput.StyleClearAll();

	SetStyleFor(wOutput, "*");
	SetStyleFor(wOutput, "errorlist");

	if (CurrentBuffer()->useMonoFont) {
		sval = props.GetExpandedString("font.monospace");
		StyleDefinition sd(sval.c_str());
		for (int style = 0; style <= StyleMax; style++) {
			if (style != static_cast<int>(SA::StylesCommon::LineNumber)) {
				if (sd.specified & StyleDefinition::sdFont) {
					wEditor.StyleSetFont(style, sd.font.c_str());
				}
				if (sd.specified & StyleDefinition::sdSize) {
					wEditor.StyleSetSizeFractional(style, sd.FractionalSize());
				}
			}
		}
	}
}

// Properties that are interactively modifiable are only read from the properties file once.
void SciTEBase::SetPropertiesInitial() {
	splitVertical = props.GetInt("split.vertical");
	openFilesHere = props.GetInt("check.if.already.open");
	wrap = props.GetInt("wrap");
	wrapOutput = props.GetInt("output.wrap");
	indentationWSVisible = props.GetInt("view.indentation.whitespace", 1);
	sbVisible = props.GetInt("statusbar.visible");
	tbVisible = props.GetInt("toolbar.visible");
	tabVisible = props.GetInt("tabbar.visible");
	tabMultiLine = props.GetInt("tabbar.multiline");
	lineNumbers = props.GetInt("line.margin.visible");
	margin = props.GetInt("margin.width");
	foldMargin = props.GetInt("fold.margin.width", foldMarginWidthDefault);

	matchCase = props.GetInt("find.replace.matchcase");
	regExp = props.GetInt("find.replace.regexp");
	unSlash = props.GetInt("find.replace.escapes");
	wrapFind = props.GetInt("find.replace.wrap", 1);
	focusOnReplace = props.GetInt("find.replacewith.focus", 1);
}

GUI::gui_string Localization::Text(std::string_view sv, bool retainIfNotFound) const {
	const std::string sEllipse("...");	// An ASCII ellipse
	const std::string utfEllipse("\xe2\x80\xa6");	// A UTF-8 ellipse
	std::string translation(sv);
	const int ellipseIndicator = Remove(translation, sEllipse);
	const int utfEllipseIndicator = Remove(translation, utfEllipse);
	const std::string menuAccessIndicatorChar(1, static_cast<char>(menuAccessIndicator[0]));
	const int accessKeyPresent = Remove(translation, menuAccessIndicatorChar);
	LowerCaseAZ(translation);
	Substitute(translation, "\n", "\\n");
	translation = GetString(translation.c_str());
	if (translation.length()) {
		if (ellipseIndicator)
			translation += sEllipse;
		if (utfEllipseIndicator)
			translation += utfEllipse;
		if (0 == accessKeyPresent) {
#if !defined(GTK)
			// Following codes are required because accelerator is not always
			// part of alphabetical word in several language. In these cases,
			// accelerator is written like "(&O)".
			const size_t posOpenParenAnd = translation.find("(&");
			if ((posOpenParenAnd != std::string::npos) && (translation.find(")", posOpenParenAnd) == posOpenParenAnd + 3)) {
				translation.erase(posOpenParenAnd, 4);
			} else {
				Remove(translation, std::string("&"));
			}
#else
			Remove(translation, std::string("&"));
#endif
		}
		Substitute(translation, "&", menuAccessIndicatorChar);
		Substitute(translation, "\\n", "\n");
	} else {
		translation = missing;
	}
	if ((translation.length() > 0) || !retainIfNotFound) {
		return GUI::StringFromUTF8(translation);
	}
	return GUI::StringFromUTF8(std::string(sv));
}

GUI::gui_string SciTEBase::LocaliseMessage(const char *s, const GUI::gui_char *param0, const GUI::gui_char *param1, const GUI::gui_char *param2) {
	GUI::gui_string translation = localiser.Text(s);
	if (param0)
		Substitute(translation, GUI_TEXT("^0"), param0);
	if (param1)
		Substitute(translation, GUI_TEXT("^1"), param1);
	if (param2)
		Substitute(translation, GUI_TEXT("^2"), param2);
	return translation;
}

void SciTEBase::ReadLocalization() {
	localiser.Clear();
	GUI::gui_string title = GUI_TEXT("locale.properties");
	const std::string localeProps = props.GetExpandedString("locale.properties");
	if (localeProps.length()) {
		title = GUI::StringFromUTF8(localeProps);
	}
	FilePath propdir = GetSciteDefaultHome();
	FilePath localePath(propdir, title);
	localiser.Read(localePath, propdir, filter, &importFiles, 0);
	localiser.SetMissing(props.GetString("translation.missing"));
	localiser.read = true;
}

void SciTEBase::ReadPropertiesInitial() {
	SetPropertiesInitial();
	const int sizeHorizontal = props.GetInt("output.horizontal.size", 0);
	const int sizeVertical = props.GetInt("output.vertical.size", 0);
	const int hideOutput = props.GetInt("output.initial.hide", 0);
	if ((!splitVertical && (sizeVertical > 0) && (heightOutput < sizeVertical)) ||
			(splitVertical && (sizeHorizontal > 0) && (heightOutput < sizeHorizontal))) {
		previousHeightOutput = splitVertical ? sizeHorizontal : sizeVertical;
		if (!hideOutput) {
			heightOutput = NormaliseSplit(previousHeightOutput);
			SizeSubWindows();
			Redraw();
		}
	}
	ViewWhitespace(props.GetInt("view.whitespace"));
	wEditor.SetIndentationGuides(props.GetInt("view.indentation.guides") ?
				     indentExamine : SA::IndentView::None);

	wEditor.SetViewEOL(props.GetInt("view.eol"));
	wEditor.SetZoom(props.GetInt("magnification"));
	wOutput.SetZoom(props.GetInt("output.magnification"));
	wEditor.SetWrapMode(wrap ? wrapStyle : SA::Wrap::None);
	wOutput.SetWrapMode(wrapOutput ? wrapStyle : SA::Wrap::None);

	std::string menuLanguageProp = props.GetExpandedString("menu.language");
	std::replace(menuLanguageProp.begin(), menuLanguageProp.end(), '|', '\0');
	const char *sMenuLanguage = menuLanguageProp.c_str();
	while (*sMenuLanguage) {
		LanguageMenuItem lmi;
		lmi.menuItem = sMenuLanguage;
		sMenuLanguage += strlen(sMenuLanguage) + 1;
		lmi.extension = sMenuLanguage;
		sMenuLanguage += strlen(sMenuLanguage) + 1;
		lmi.menuKey = sMenuLanguage;
		sMenuLanguage += strlen(sMenuLanguage) + 1;
		languageMenu.push_back(lmi);
	}
	SetLanguageMenu();

	// load the user defined short cut props
	std::string shortCutProp = props.GetNewExpandString("user.shortcuts");
	if (shortCutProp.length()) {
		const size_t pipes = std::count(shortCutProp.begin(), shortCutProp.end(), '|');
		std::replace(shortCutProp.begin(), shortCutProp.end(), '|', '\0');
		const char *sShortCutProp = shortCutProp.c_str();
		for (size_t item = 0; item < pipes/2; item++) {
			ShortcutItem sci;
			sci.menuKey = sShortCutProp;
			sShortCutProp += strlen(sShortCutProp) + 1;
			sci.menuCommand = sShortCutProp;
			sShortCutProp += strlen(sShortCutProp) + 1;
			shortCutItemList.push_back(sci);
		}
	}
	// end load the user defined short cut props

	props.SetPath("SciteDefaultHome", GetSciteDefaultHome());
	props.SetPath("SciteUserHome", GetSciteUserHome());
}

FilePath SciTEBase::GetDefaultPropertiesFileName() {
	return FilePath(GetSciteDefaultHome(), propGlobalFileName);
}

FilePath SciTEBase::GetAbbrevPropertiesFileName() {
	return FilePath(GetSciteUserHome(), propAbbrevFileName);
}

FilePath SciTEBase::GetUserPropertiesFileName() {
	return FilePath(GetSciteUserHome(), propUserFileName);
}

FilePath SciTEBase::GetLocalPropertiesFileName() {
	return FilePath(filePath.Directory(), propLocalFileName);
}

FilePath SciTEBase::GetDirectoryPropertiesFileName() {
	FilePath propfile;

	if (filePath.IsSet()) {
		propfile.Set(filePath.Directory(), propDirectoryFileName);

		// if this file does not exist try to find the prop file in a parent directory
		while (!propfile.Directory().IsRoot() && !propfile.Exists()) {
			propfile.Set(propfile.Directory().Directory(), propDirectoryFileName);
		}

		// not found -> set it to the initial directory
		if (!propfile.Exists()) {
			propfile.Set(filePath.Directory(), propDirectoryFileName);
		}
	}
	return propfile;
}

void SciTEBase::OpenProperties(int propsFile) {
	FilePath propfile;
	switch (propsFile) {
	case IDM_OPENLOCALPROPERTIES:
		propfile = GetLocalPropertiesFileName();
		Open(propfile, ofQuiet);
		break;
	case IDM_OPENUSERPROPERTIES:
		propfile = GetUserPropertiesFileName();
		Open(propfile, ofQuiet);
		break;
	case IDM_OPENABBREVPROPERTIES:
		propfile = pathAbbreviations;
		Open(propfile, ofQuiet);
		break;
	case IDM_OPENGLOBALPROPERTIES:
		propfile = GetDefaultPropertiesFileName();
		Open(propfile, ofQuiet);
		break;
	case IDM_OPENLUAEXTERNALFILE: {
			GUI::gui_string extlua = GUI::StringFromUTF8(props.GetExpandedString("ext.lua.startup.script"));
			if (extlua.length()) {
				Open(extlua, ofQuiet);
			}
			break;
		}
	case IDM_OPENDIRECTORYPROPERTIES: {
			propfile = GetDirectoryPropertiesFileName();
			const bool alreadyExists = propfile.Exists();
			Open(propfile, ofQuiet);
			if (!alreadyExists)
				SaveAsDialog();
		}
		break;
	}
}

// return the int value of the command name passed in.
int SciTEBase::GetMenuCommandAsInt(const std::string &commandName) {
	int i = IFaceTable::FindConstant(commandName.c_str());
	if (i != -1) {
		return IFaceTable::constants[i].value;
	}

	// Check also for a SCI command, as long as it has no parameters
	i = IFaceTable::FindFunctionByConstantName(commandName.c_str());
	if (i != -1 &&
			IFaceTable::functions[i].paramType[0] == iface_void &&
			IFaceTable::functions[i].paramType[1] == iface_void) {
		return IFaceTable::functions[i].value;
	}

	// Otherwise we might have entered a number as command to access a "SCI_" command
	return atoi(commandName.c_str());
}
