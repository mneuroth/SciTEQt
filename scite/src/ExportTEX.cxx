// SciTE - Scintilla based Text Editor
/** @file ExportTEX.cxx
 ** Export the current document to TEX.
 **/
// Copyright 1998-2006 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <ctime>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <memory>
#include <chrono>
#include <sstream>
#include <atomic>
#include <mutex>

#include <fcntl.h>
#include <sys/stat.h>

#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

#include "GUI.h"
#include "ScintillaWindow.h"

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
#include "Searcher.h"
#include "SciTEBase.h"

//---------- Save to TeX ----------

static char *getTexRGB(char *texcolor, const char *stylecolor) noexcept {
	//texcolor[rgb]{0,0.5,0}{....}
	const double rf = IntFromHexByte(stylecolor + 1) / 256.0;
	const double gf = IntFromHexByte(stylecolor + 3) / 256.0;
	const double bf = IntFromHexByte(stylecolor + 5) / 256.0;
	// avoid breakage due to locale setting
	const int r = static_cast<int>(rf * 10 + 0.5);
	const int g = static_cast<int>(gf * 10 + 0.5);
	const int b = static_cast<int>(bf * 10 + 0.5);
	sprintf(texcolor, "%d.%d, %d.%d, %d.%d", r / 10, r % 10, g / 10, g % 10, b / 10, b % 10);
	return texcolor;
}

#define CHARZ ('z' - 'b')
static char *texStyle(int style) noexcept {
	static char buf[10];
	int i = 0;
	do {
		buf[i++] = static_cast<char>('a' + (style % CHARZ));
		style /= CHARZ;
	} while (style > 0);
	buf[i] = 0;
	return buf;
}

static void defineTexStyle(const StyleDefinition &style, FILE *fp, int istyle) {
	int closing_brackets = 2;
	char rgb[200] = "";
	fprintf(fp, "\\newcommand{\\scite%s}[1]{\\noindent{\\ttfamily{", texStyle(istyle));
	if (style.italics) {
		fputs("\\textit{", fp);
		closing_brackets++;
	}
	if (style.IsBold()) {
		fputs("\\textbf{", fp);
		closing_brackets++;
	}
	if (style.fore.length()) {
		fprintf(fp, "\\textcolor[rgb]{%s}{", getTexRGB(rgb, style.fore.c_str()));
		closing_brackets++;
	}
	if (style.back.length()) {
		fprintf(fp, "\\colorbox[rgb]{%s}{", getTexRGB(rgb, style.back.c_str()));
		closing_brackets++;
	}
	fputs("#1", fp);
	for (int i = 0; i <= closing_brackets; i++) {
		fputc('}', fp);
	}
	fputc('\n', fp);
}

void SciTEBase::SaveToTEX(const FilePath &saveName) {
	RemoveFindMarks();
	wEditor.ColouriseAll();
	int tabSize = props.GetInt("tabsize");
	if (tabSize == 0)
		tabSize = 4;

	const SA::Position lengthDoc = LengthDocument();
	TextReader acc(wEditor);
	bool styleIsUsed[StyleMax + 1] = {};

	const int titleFullPath = props.GetInt("export.tex.title.fullpath", 0);

	for (SA::Position pos = 0; pos < lengthDoc; pos++) {	// check the used styles
		styleIsUsed[acc.StyleAt(pos)] = true;
	}
	styleIsUsed[StyleDefault] = true;

	FILE *fp = saveName.Open(GUI_TEXT("wt"));
	bool failedWrite = fp == nullptr;
	if (fp) {
		fputs("\\documentclass[a4paper]{article}\n"
		      "\\usepackage[a4paper,margin=2cm]{geometry}\n"
		      "\\usepackage[T1]{fontenc}\n"
		      "\\usepackage{color}\n"
		      "\\usepackage{alltt}\n"
		      "\\usepackage{times}\n"
		      "\\setlength{\\fboxsep}{0pt}\n", fp);

		for (int istyle = 0; istyle < StyleMax; istyle++) {      // get keys
			if (styleIsUsed[istyle]) {
				StyleDefinition sd = StyleDefinitionFor(istyle);
				defineTexStyle(sd, fp, istyle); // writeout style macroses
			}
		}

		fputs("\\begin{document}\n\n", fp);
		fprintf(fp, "Source File: %s\n\n\\noindent\n\\small{\n",
			titleFullPath ? filePath.AsUTF8().c_str() : filePath.Name().AsUTF8().c_str());

		int styleCurrent = acc.StyleAt(0);

		fprintf(fp, "\\scite%s{", texStyle(styleCurrent));

		int lineIdx = 0;

		for (SA::Position i = 0; i < lengthDoc; i++) { //here process each character of the document
			const char ch = acc[i];
			const int style = acc.StyleAt(i);

			if (style != styleCurrent) { //new style?
				fprintf(fp, "}\\scite%s{", texStyle(style));
				styleCurrent = style;
			}

			switch (ch) {   //write out current character.
			case '\t': {
					const int ts = tabSize - (lineIdx % tabSize);
					lineIdx += ts - 1;
					fprintf(fp, "\\hspace*{%dem}", ts);
					break;
				}
			case '\\':
				fputs("{\\textbackslash}", fp);
				break;
			case '>':
			case '<':
			case '@':
				fprintf(fp, "$%c$", ch);
				break;
			case '{':
			case '}':
			case '^':
			case '_':
			case '&':
			case '$':
			case '#':
			case '%':
			case '~':
				fprintf(fp, "\\%c", ch);
				break;
			case '\r':
			case '\n':
				lineIdx = -1;	// Because incremented below
				if (ch == '\r' && acc[i + 1] == '\n')
					i++;	// Skip the LF
				styleCurrent = acc.StyleAt(i + 1);
				fprintf(fp, "} \\\\\n\\scite%s{", texStyle(styleCurrent));
				break;
			case ' ':
				if (acc[i + 1] == ' ') {
					fputs("{\\hspace*{1em}}", fp);
				} else {
					fputc(' ', fp);
				}
				break;
			default:
				fputc(ch, fp);
			}
			lineIdx++;
		}
		fputs("}\n} %end small\n\n\\end{document}\n", fp); //close last empty style macros and document too
		if (fclose(fp) != 0) {
			failedWrite = true;
		}
	}
	if (failedWrite) {
		FailedSaveMessageBox(saveName);
	}
}
