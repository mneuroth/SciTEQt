// SciTE - Scintilla based Text Editor
/** @file ExportPDF.cxx
 ** Export the current document to PDF.
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

//---------- Save to PDF ----------

/*
	PDF Exporter. Status: Beta
	Contributed by Ahmad M. Zawawi <zeus_go64@hotmail.com>
	Modifications by Darren Schroeder Feb 22, 2003; Philippe Lhoste 2003-10
	Overhauled by Kein-Hong Man 2003-11

	This exporter is meant to be small and simple; users are expected to
	use other methods for heavy-duty formatting. PDF elements marked with
	"PDF1.4Ref" states where in the PDF 1.4 Reference Spec (the PDF file of
	which is freely available from Adobe) the particular element can be found.

	Possible TODOs that will probably not be implemented: full styling,
	optimization, font substitution, compression, character set encoding.
*/
#define PDF_TAB_DEFAULT		8
#define PDF_FONT_DEFAULT	1	// Helvetica
#define PDF_FONTSIZE_DEFAULT	10
#define PDF_SPACING_DEFAULT	1.2
#define PDF_HEIGHT_DEFAULT	792	// Letter
#define PDF_WIDTH_DEFAULT	612
#define PDF_MARGIN_DEFAULT	72	// 1.0"
#define PDF_ENCODING		"WinAnsiEncoding"

struct PDFStyle {
	std::string fore;
	int font=0;
};

namespace {

const char *PDFfontNames[] = {
	"Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique",
	"Helvetica", "Helvetica-Bold", "Helvetica-Oblique", "Helvetica-BoldOblique",
	"Times-Roman", "Times-Bold", "Times-Italic", "Times-BoldItalic"
};

// ascender, descender aligns font origin point with page
short PDFfontAscenders[] =  { 629, 718, 699 };
short PDFfontDescenders[] = { 157, 207, 217 };
short PDFfontWidths[] =     { 600,   0,   0 };

}

inline std::string getPDFRGB(const char *stylecolour) {
	std::string ret;
	// grab colour components (max string length produced = 18)
	for (int i = 1; i < 6; i += 2) {
		char val[20] = "";
		// 3 decimal places for enough dynamic range
		const int c = (IntFromHexByte(stylecolour + i) * 1000 + 127) / 255;
		if (c == 0 || c == 1000) {	// optimise
			sprintf(val, "%d ", c / 1000);
		} else {
			sprintf(val, "0.%03d ", c);
		}
		ret += val;
	}
	return ret;
}

void SciTEBase::SaveToPDF(const FilePath &saveName) {
	// This class conveniently handles the tracking of PDF objects
	// so that the cross-reference table can be built (PDF1.4Ref(p39))
	// All writes to fp passes through a PDFObjectTracker object.
	class PDFObjectTracker {
	private:
		FILE *fp;
		std::vector<long> offsetList;
	public:
		int index;
		explicit PDFObjectTracker(FILE *fp_) {
			fp = fp_;
			index = 1;
		}

		// Deleted so PDFObjectTracker objects can not be copied.
		PDFObjectTracker(const PDFObjectTracker &) = delete;
		PDFObjectTracker(PDFObjectTracker &&) = delete;
		PDFObjectTracker &operator=(const PDFObjectTracker &) = delete;
		PDFObjectTracker &operator=(PDFObjectTracker &&) = delete;

		void write(const char *objectData) {
			const size_t length = strlen(objectData);
			// note binary write used, open with "wb"
			fwrite(objectData, sizeof(char), length, fp);
		}
		void write(int objectData) {
			char val[20];
			sprintf(val, "%d", objectData);
			write(val);
		}
		// returns object number assigned to the supplied data
		int add(const char *objectData) {
			// save offset, then format and write object
			offsetList.push_back(ftell(fp));
			write(index);
			write(" 0 obj\n");
			write(objectData);
			write("endobj\n");
			return index++;
		}
		// builds xref table, returns file offset of xref table
		long xref() {
			char val[32] = "";
			// xref start index and number of entries
			const long xrefStart = ftell(fp);
			write("xref\n0 ");
			write(index);
			// a xref entry *must* be 20 bytes long (PDF1.4Ref(p64))
			// so extra space added; also the first entry is special
			write("\n0000000000 65535 f \n");
			for (int i = 0; i < index - 1; i++) {
				sprintf(val, "%010ld 00000 n \n", offsetList[i]);
				write(val);
			}
			return xrefStart;
		}
	};

	// Object to manage line and page rendering. Apart from startPDF, endPDF
	// everything goes in via add() and nextLine() so that line formatting
	// and pagination can be done properly.
	class PDFRender {
	private:
		bool pageStarted;
		bool firstLine;
		int pageCount;
		int pageContentStart;
		double xPos, yPos;	// position tracking for line wrapping
		std::string pageData;	// holds PDF stream contents
		std::string segment;	// character data
		std::string segStyle;		// style of segment
		bool justWhiteSpace;
		int styleCurrent, stylePrev;
		double leading;
		char buffer[250];
	public:
		PDFObjectTracker *oT;
		std::vector<PDFStyle> style;
		int fontSize;		// properties supplied by user
		int fontSet;
		long pageWidth, pageHeight;
		GUI::Rectangle pageMargin;
		//
		PDFRender() : buffer{} {
			pageStarted = false;
			firstLine = false;
			pageCount = 0;
			pageContentStart = 0;
			xPos = 0.0;
			yPos = 0.0;
			justWhiteSpace = true;
			styleCurrent = StyleDefault;
			stylePrev = StyleDefault;
			leading = PDF_FONTSIZE_DEFAULT * PDF_SPACING_DEFAULT;
			buffer[0] = '\0';
			oT = nullptr;
			fontSize = 0;
			fontSet = PDF_FONT_DEFAULT;
			pageWidth = 100;
			pageHeight = 100;
		}
		// Deleted so PDFRender objects can not be copied.
		PDFRender(const PDFRender &) = delete;
		PDFRender(PDFRender &&) = delete;
		PDFRender &operator=(const PDFRender &) = delete;
		PDFRender &operator=(PDFRender &&) = delete;
		//
		double fontToPoints(int thousandths) const {
			return (double)fontSize * thousandths / 1000.0;
		}
		std::string setStyle(int style_) {
			int styleNext = style_;
			if (style_ == -1) { styleNext = styleCurrent; }
			std::string buff;
			if (styleNext != styleCurrent || style_ == -1) {
				if (style[styleCurrent].font != style[styleNext].font
						|| style_ == -1) {
					char fontSpec[100];
					sprintf(fontSpec, "/F%d %d Tf ",
						style[styleNext].font + 1, fontSize);
					buff += fontSpec;
				}
				if ((style[styleCurrent].fore != style[styleNext].fore)
						|| style_ == -1) {
					buff += style[styleNext].fore;
					buff += "rg ";
				}
			}
			return buff;
		}
		//
		void startPDF() {
			if (fontSize <= 0) {
				fontSize = PDF_FONTSIZE_DEFAULT;
			}
			// leading is the term for distance between lines
			leading = fontSize * PDF_SPACING_DEFAULT;
			// sanity check for page size and margins
			const int pageWidthMin = (int)leading + pageMargin.left + pageMargin.right;
			if (pageWidth < pageWidthMin) {
				pageWidth = pageWidthMin;
			}
			const int pageHeightMin = (int)leading + pageMargin.top + pageMargin.bottom;
			if (pageHeight < pageHeightMin) {
				pageHeight = pageHeightMin;
			}
			// start to write PDF file here (PDF1.4Ref(p63))
			// ASCII>127 characters to indicate binary-possible stream
			oT->write("%PDF-1.3\n%\xc7\xec\x8f\xa2\n");
			styleCurrent = StyleDefault;

			// build objects for font resources; note that font objects are
			// *expected* to start from index 1 since they are the first objects
			// to be inserted (PDF1.4Ref(p317))
			for (int i = 0; i < 4; i++) {
				sprintf(buffer, "<</Type/Font/Subtype/Type1"
					"/Name/F%d/BaseFont/%s/Encoding/"
					PDF_ENCODING
					">>\n", i + 1,
					PDFfontNames[fontSet * 4 + i]);
				oT->add(buffer);
			}
			pageContentStart = oT->index;
		}
		void endPDF() {
			if (pageStarted) {	// flush buffers
				endPage();
			}
			// refer to all used or unused fonts for simplicity
			const int resourceRef = oT->add(
							"<</ProcSet[/PDF/Text]\n"
							"/Font<</F1 1 0 R/F2 2 0 R/F3 3 0 R"
							"/F4 4 0 R>> >>\n");
			// create all the page objects (PDF1.4Ref(p88))
			// forward reference pages object; calculate its object number
			const int pageObjectStart = oT->index;
			const int pagesRef = pageObjectStart + pageCount;
			for (int i = 0; i < pageCount; i++) {
				sprintf(buffer, "<</Type/Page/Parent %d 0 R\n"
					"/MediaBox[ 0 0 %ld %ld"
					"]\n/Contents %d 0 R\n"
					"/Resources %d 0 R\n>>\n",
					pagesRef, pageWidth, pageHeight,
					pageContentStart + i, resourceRef);
				oT->add(buffer);
			}
			// create page tree object (PDF1.4Ref(p86))
			pageData = "<</Type/Pages/Kids[\n";
			for (int j = 0; j < pageCount; j++) {
				sprintf(buffer, "%d 0 R\n", pageObjectStart + j);
				pageData += buffer;
			}
			sprintf(buffer, "]/Count %d\n>>\n", pageCount);
			pageData += buffer;
			oT->add(pageData.c_str());
			// create catalog object (PDF1.4Ref(p83))
			sprintf(buffer, "<</Type/Catalog/Pages %d 0 R >>\n", pagesRef);
			const int catalogRef = oT->add(buffer);
			// append the cross reference table (PDF1.4Ref(p64))
			const long xref = oT->xref();
			// end the file with the trailer (PDF1.4Ref(p67))
			sprintf(buffer, "trailer\n<< /Size %d /Root %d 0 R\n>>"
				"\nstartxref\n%ld\n%%%%EOF\n",
				oT->index, catalogRef, xref);
			oT->write(buffer);
		}
		void add(char ch, int style_) {
			if (!pageStarted) {
				startPage();
			}
			// get glyph width (TODO future non-monospace handling)
			const double glyphWidth = fontToPoints(PDFfontWidths[fontSet]);
			xPos += glyphWidth;
			// if cannot fit into a line, flush, wrap to next line
			if (xPos > pageWidth - pageMargin.right) {
				nextLine();
				xPos += glyphWidth;
			}
			// if different style, then change to style
			if (style_ != styleCurrent) {
				flushSegment();
				// output code (if needed) for new style
				segStyle = setStyle(style_);
				stylePrev = styleCurrent;
				styleCurrent = style_;
			}
			// escape these characters
			if (ch == ')' || ch == '(' || ch == '\\') {
				segment += '\\';
			}
			if (ch != ' ') { justWhiteSpace = false; }
			segment += ch;	// add to segment data
		}
		void flushSegment() {
			if (segment.length() > 0) {
				if (justWhiteSpace) {	// optimise
					styleCurrent = stylePrev;
				} else {
					pageData += segStyle;
				}
				pageData += "(";
				pageData += segment;
				pageData += ")Tj\n";
			}
			segment.clear();
			segStyle = "";
			justWhiteSpace = true;
		}
		void startPage() {
			pageStarted = true;
			firstLine = true;
			pageCount++;
			const double fontAscender = fontToPoints(PDFfontAscenders[fontSet]);
			yPos = pageHeight - pageMargin.top - fontAscender;
			// start a new page
			sprintf(buffer, "BT 1 0 0 1 %d %d Tm\n",
				pageMargin.left, (int)yPos);
			pageData = buffer;
			// force setting of initial font, colour
			segStyle = setStyle(-1);
			pageData += segStyle;
			xPos = pageMargin.left;
			segment.clear();
			flushSegment();
		}
		void endPage() {
			pageStarted = false;
			flushSegment();
			try {
				// build actual text object; +3 is for "ET\n"
				// PDF1.4Ref(p38) EOL marker preceding endstream not counted
				std::ostringstream osTextObj;
				// concatenate stream within the text object
				osTextObj
						<< "<</Length "
						<< (pageData.length() - 1 + 3)
						<< ">>\nstream\n"
						<< pageData
						<< "ET\nendstream\n";
				const std::string textObj = osTextObj.str();
				oT->add(textObj.c_str());
			} catch (std::exception &) {
				// Exceptions not enabled on stream but still causes diagnostic in Coverity.
				// Simply swallow the failure.
			}
		}
		void nextLine() {
			if (!pageStarted) {
				startPage();
			}
			xPos = pageMargin.left;
			flushSegment();
			// PDF follows cartesian coords, subtract -> down
			yPos -= leading;
			const double fontDescender = fontToPoints(PDFfontDescenders[fontSet]);
			if (yPos < pageMargin.bottom + fontDescender) {
				endPage();
				startPage();
				return;
			}
			if (firstLine) {
				// avoid breakage due to locale setting
				const int f = static_cast<int>(leading * 10 + 0.5);
				sprintf(buffer, "0 -%d.%d TD\n", f / 10, f % 10);
				firstLine = false;
			} else {
				sprintf(buffer, "T*\n");
			}
			pageData += buffer;
		}
	};
	PDFRender pr;

	RemoveFindMarks();
	wEditor.ColouriseAll();
	// read exporter flags
	int tabSize = props.GetInt("tabsize", PDF_TAB_DEFAULT);
	if (tabSize < 0) {
		tabSize = PDF_TAB_DEFAULT;
	}
	// read magnification value to add to default screen font size
	pr.fontSize = props.GetInt("export.pdf.magnification");
	// set font family according to face name
	std::string propItem = props.GetExpandedString("export.pdf.font");
	pr.fontSet = PDF_FONT_DEFAULT;
	if (propItem.length()) {
		if (propItem == "Courier")
			pr.fontSet = 0;
		else if (propItem == "Helvetica")
			pr.fontSet = 1;
		else if (propItem == "Times")
			pr.fontSet = 2;
	}
	// page size: width, height
	propItem = props.GetExpandedString("export.pdf.pagesize");
	char buffer[200] = "";
	const char *ps = propItem.c_str();
	const char *next = GetNextPropItem(ps, buffer, 32);
	if (0 >= (pr.pageWidth = atol(buffer))) {
		pr.pageWidth = PDF_WIDTH_DEFAULT;
	}
	GetNextPropItem(next, buffer, 32);
	if (0 >= (pr.pageHeight = atol(buffer))) {
		pr.pageHeight = PDF_HEIGHT_DEFAULT;
	}
	// page margins: left, right, top, bottom
	propItem = props.GetExpandedString("export.pdf.margins");
	ps = propItem.c_str();
	next = GetNextPropItem(ps, buffer, 32);
	if (0 >= (pr.pageMargin.left = static_cast<int>(atol(buffer)))) {
		pr.pageMargin.left = PDF_MARGIN_DEFAULT;
	}
	next = GetNextPropItem(next, buffer, 32);
	if (0 >= (pr.pageMargin.right = static_cast<int>(atol(buffer)))) {
		pr.pageMargin.right = PDF_MARGIN_DEFAULT;
	}
	next = GetNextPropItem(next, buffer, 32);
	if (0 >= (pr.pageMargin.top = static_cast<int>(atol(buffer)))) {
		pr.pageMargin.top = PDF_MARGIN_DEFAULT;
	}
	GetNextPropItem(next, buffer, 32);
	if (0 >= (pr.pageMargin.bottom = static_cast<int>(atol(buffer)))) {
		pr.pageMargin.bottom = PDF_MARGIN_DEFAULT;
	}

	// collect all styles available for that 'language'
	// or the default style if no language is available...
	pr.style.resize(StyleMax + 1);
	for (int i = 0; i <= StyleMax; i++) {	// get keys
		pr.style[i].font = 0;
		pr.style[i].fore = "";

		StyleDefinition sd = StyleDefinitionFor(i);

		if (sd.specified != StyleDefinition::sdNone) {
			if (sd.italics) { pr.style[i].font |= 2; }
			if (sd.IsBold()) { pr.style[i].font |= 1; }
			if (sd.fore.length()) {
				pr.style[i].fore = getPDFRGB(sd.fore.c_str());
			} else if (i == StyleDefault) {
				pr.style[i].fore = "0 0 0 ";
			}
			// grab font size from default style
			if (i == StyleDefault) {
				if (sd.size > 0)
					pr.fontSize += sd.size;
				else
					pr.fontSize = PDF_FONTSIZE_DEFAULT;
			}
		}
	}
	// patch in default foregrounds
	for (int j = 0; j <= StyleMax; j++) {
		if (pr.style[j].fore.empty()) {
			pr.style[j].fore = pr.style[StyleDefault].fore;
		}
	}

	FILE *fp = saveName.Open(GUI_TEXT("wb"));
	if (!fp) {
		// couldn't open the file for saving, issue an error message
		FailedSaveMessageBox(saveName);
		return;
	}
	// initialise PDF rendering
	PDFObjectTracker ot(fp);
	pr.oT = &ot;
	pr.startPDF();

	// do here all the writing
	const SA::Position lengthDoc = LengthDocument();
	TextReader acc(wEditor);

	if (!lengthDoc) {	// enable zero length docs
		pr.nextLine();
	} else {
		int lineIndex = 0;
		for (SA::Position i = 0; i < lengthDoc; i++) {
			const char ch = acc[i];
			const int style = acc.StyleAt(i);

			if (ch == '\t') {
				// expand tabs
				int ts = tabSize - (lineIndex % tabSize);
				lineIndex += ts;
				for (; ts; ts--) {	// add ts count of spaces
					pr.add(' ', style);	// add spaces
				}
			} else if (ch == '\r' || ch == '\n') {
				if (ch == '\r' && acc[i + 1] == '\n') {
					i++;
				}
				// close and begin a newline...
				pr.nextLine();
				lineIndex = 0;
			} else {
				// write the character normally...
				pr.add(ch, style);
				lineIndex++;
			}
		}
	}
	// write required stuff and close the PDF file
	pr.endPDF();
	if (fclose(fp) != 0) {
		FailedSaveMessageBox(saveName);
	}
}

