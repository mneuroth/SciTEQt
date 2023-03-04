// SciTE - Scintilla based Text Editor
/** @file SciTEBase.cxx
 ** Platform independent base class of editor.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cmath>

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
#include <thread>

#include <fcntl.h>
#include <sys/stat.h>

#include "ILoader.h"

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "Scintilla.h"
#include "SciLexer.h"

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
#include "FileWorker.h"
#include "MatchMarker.h"
#include "EditorConfig.h"
#include "Searcher.h"
#include "SciTEBase.h"

Searcher::Searcher() {
	wholeWord = false;
	matchCase = false;
	regExp = false;
	unSlash = false;
	wrapFind = true;
	reverseFind = false;
	filterState = false;
	contextVisible = false;

	searchStartPosition = 0;
	replacing = false;
	havefound = false;
	failedfind = false;
	findInStyle = false;
	findStyle = 0;
	closeFind = CloseFind::closeAlways;

	focusOnReplace = false;
}

void Searcher::InsertFindInMemory() {
	if (!findWhat.empty()) {
		memFinds.InsertDeletePrefix(findWhat);
	}
}

// The find and replace dialogs and strips often manipulate boolean
// flags based on dialog control IDs and menu IDs.
bool &Searcher::FlagFromCmd(int cmd) noexcept {
	static bool notFound = false;
	switch (cmd) {
	case IDWHOLEWORD:
	case IDM_WHOLEWORD:
		return wholeWord;
	case IDMATCHCASE:
	case IDM_MATCHCASE:
		return matchCase;
	case IDREGEXP:
	case IDM_REGEXP:
		return regExp;
	case IDUNSLASH:
	case IDM_UNSLASH:
		return unSlash;
	case IDWRAP:
	case IDM_WRAPAROUND:
		return wrapFind;
	case IDDIRECTIONUP:
	case IDM_DIRECTIONUP:
		return reverseFind;
	case IDFILTERSTATE:
	case IDM_FILTERSTATE:
		return filterState;
	case IDCONTEXTVISIBLE:
	case IDM_CONTEXTVISIBLE:
		return contextVisible;
	}
	return notFound;
}

SciTEBase::SciTEBase(Extension *ext) : apis(true), pwFocussed(&wEditor), extender(ext) {
	needIdle = false;
	codePage = 0;
	characterSet = SA::CharacterSet::Ansi;
	language = "java";
	lexLanguage = SCLEX_CPP;
	functionDefinition = "";
	docReleaser.pSci = &wEditor;
	diagnosticStyleStart = 0;
	stripTrailingSpaces = false;
	ensureFinalLineEnd = false;
	ensureConsistentLineEnds = false;
	indentOpening = true;
	indentClosing = true;
	indentMaintain = false;
	statementLookback = 10;
	preprocessorSymbol = '\0';

	tbVisible = false;
	sbVisible = false;
	tabVisible = false;
	tabHideOne = false;
	tabMultiLine = false;
	sbNum = 1;
	visHeightTools = 0;
	visHeightTab = 0;
	visHeightStatus = 0;
	visHeightEditor = 1;
	heightBar = 7;
	dialogsOnScreen = 0;
	topMost = false;
	wrap = false;
	wrapOutput = false;
	wrapStyle = SA::Wrap::Word;
	idleStyling = SA::IdleStyling::None;
	alphaIndicator = static_cast<SA::Alpha>(30);
	underIndicator = false;
	openFilesHere = false;
	fullScreen = false;
	appearance = {};

	heightOutput = 0;
	heightOutputStartDrag = 0;
	previousHeightOutput = 0;

	allowMenuActions = true;
	scrollOutput = 1;
	returnOutputToCommand = true;

	ptStartDrag.x = 0;
	ptStartDrag.y = 0;
	capturedMouse = false;
	firstPropertiesRead = true;
	localiser.read = false;
	splitVertical = false;
	bufferedDraw = true;
	bracesCheck = true;
	bracesSloppy = false;
	bracesStyle = 0;
	braceCount = 0;

	indentationWSVisible = true;
	indentExamine = SA::IndentView::LookBoth;
	autoCompleteIgnoreCase = false;
	imeAutoComplete = false;
	callTipUseEscapes = false;
	callTipIgnoreCase = false;
	autoCCausedByOnlyOne = false;
	autoCompleteVisibleItemCount = 9;
	startCalltipWord = 0;
	currentCallTip = 0;
	maxCallTips = 1;
	currentCallTipWord = "";
	lastPosCallTip = 0;

	margin = false;
	marginWidth = marginWidthDefault;
	foldMargin = true;
	foldMarginWidth = foldMarginWidthDefault;
	lineNumbers = false;
	lineNumbersWidth = lineNumbersWidthDefault;
	lineNumbersExpand = false;

	macrosEnabled = false;
	recording = false;

	propsEmbed.superPS = &propsPlatform;
	propsBase.superPS = &propsEmbed;
	propsUser.superPS = &propsBase;
	propsDirectory.superPS = &propsUser;
	propsLocal.superPS = &propsDirectory;
	propsDiscovered.superPS = &propsLocal;
	props.superPS = &propsDiscovered;

	propsStatus.superPS = &props;

	needReadProperties = false;
	quitting = false;
	canUndo = false;
	canRedo = false;

	timerMask = 0;
	delayBeforeAutoSave = 0;

	editorConfig = IEditorConfig::Create();
}

SciTEBase::~SciTEBase() {
	if (extender)
		extender->Finalise();
	popup.Destroy();
}

void SciTEBase::Finalise() {
	TimerEnd(timerAutoSave);
}

bool SciTEBase::PerformOnNewThread(Worker *pWorker) {
	try {
		std::thread thread([pWorker] {
			pWorker->Execute();
		});
		thread.detach();
		return true;
	} catch (std::system_error &) {
		return false;
	}
}

void SciTEBase::WorkerCommand(int cmd, Worker *pWorker) {
	switch (cmd) {
	case WORK_FILEREAD:
		TextRead(static_cast<FileLoader *>(pWorker));
		UpdateProgress(pWorker);
		break;
	case WORK_FILEWRITTEN:
		TextWritten(static_cast<FileStorer *>(pWorker));
		UpdateProgress(pWorker);
		break;
	case WORK_FILEPROGRESS:
		UpdateProgress(pWorker);
		break;
	}
}

SystemAppearance SciTEBase::CurrentAppearance() const noexcept {
	return {};
}

void SciTEBase::CheckAppearanceChanged() {
	const SystemAppearance currentAppearance = CurrentAppearance();
	if (!(appearance == currentAppearance)) {
		appearance = currentAppearance;
		ReloadProperties();
	}
}

// The system focus may move to other controls including the menu bar
// but we are normally interested in whether the edit or output pane was
// most recently focused and should be used by menu commands.
void SciTEBase::SetPaneFocus(bool editPane) noexcept {
	pwFocussed = editPane ? &wEditor : &wOutput;
}

GUI::ScintillaWindow &SciTEBase::PaneFocused() {
	return wOutput.HasFocus() ? wOutput : wEditor;
}

GUI::ScintillaWindow &SciTEBase::PaneSource(int destination) {
	if (destination == IDM_SRCWIN)
		return wEditor;
	else if (destination == IDM_RUNWIN)
		return wOutput;
	else
		return PaneFocused();
}

intptr_t SciTEBase::CallFocusedElseDefault(int defaultValue, SA::Message msg, uintptr_t wParam, intptr_t lParam) {
	if (wOutput.HasFocus())
		return wOutput.Call(msg, wParam, lParam);
	else if (wEditor.HasFocus())
		return wEditor.Call(msg, wParam, lParam);
	else
		return defaultValue;
}

void SciTEBase::CallChildren(SA::Message msg, uintptr_t wParam, intptr_t lParam) {
	wEditor.Call(msg, wParam, lParam);
	wOutput.Call(msg, wParam, lParam);
}

std::string SciTEBase::GetTranslationToAbout(const char *const propname, bool retainIfNotFound) {
#if !defined(GTK)
	return GUI::UTF8FromString(localiser.Text(propname, retainIfNotFound));
#else
	// On GTK, localiser.Text always converts to UTF-8.
	return localiser.Text(propname, retainIfNotFound);
#endif
}

void SciTEBase::ViewWhitespace(bool view) {
	if (view && indentationWSVisible == 2)
		wEditor.SetViewWS(SA::WhiteSpace::VisibleOnlyInIndent);
	else if (view && indentationWSVisible)
		wEditor.SetViewWS(SA::WhiteSpace::VisibleAlways);
	else if (view)
		wEditor.SetViewWS(SA::WhiteSpace::VisibleAfterIndent);
	else
		wEditor.SetViewWS(SA::WhiteSpace::Invisible);
}

StyleAndWords SciTEBase::GetStyleAndWords(const char *base) {
	StyleAndWords sw;
	std::string fileNameForExtension = ExtensionFileName();
	std::string sAndW = props.GetNewExpandString(base, fileNameForExtension.c_str());
	sw.styleNumber = atoi(sAndW.c_str());
	const char *space = strchr(sAndW.c_str(), ' ');
	if (space)
		sw.words = space + 1;
	return sw;
}

void SciTEBase::AssignKey(SA::Keys key, SA::KeyMod mods, int cmd) {
	wEditor.AssignCmdKey(
		IntFromTwoShorts(static_cast<short>(key),
				 static_cast<short>(mods)), cmd);
}

/**
 * Override the language of the current file with the one indicated by @a cmdID.
 * Mostly used to set a language on a file of unknown extension.
 */
void SciTEBase::SetOverrideLanguage(int cmdID) {
	const FilePosition fp = GetFilePosition();
	EnsureRangeVisible(wEditor, SA::Span(0, wEditor.Length()), false);
	// Zero all the style bytes
	wEditor.ClearDocumentStyle();

	CurrentBuffer()->overrideExtension = "x.";
	CurrentBuffer()->overrideExtension += languageMenu[cmdID].extension;
	ReadProperties();
	SetIndentSettings();
	wEditor.ColouriseAll();
	Redraw();
	DisplayAround(fp);
}

SA::Position SciTEBase::LengthDocument() {
	return wEditor.Length();
}

SA::Position SciTEBase::GetCaretInLine() {
	const SA::Position caret = wEditor.CurrentPos();
	const SA::Line line = wEditor.LineFromPosition(caret);
	const SA::Position lineStart = wEditor.LineStart(line);
	return caret - lineStart;
}

std::string SciTEBase::GetLine(SA::Line line) {
	const SA::Span rangeLine(wEditor.LineStart(line), wEditor.LineEnd(line));
	return wEditor.StringOfSpan(rangeLine);
}

std::string SciTEBase::GetCurrentLine() {
	// Get needed buffer size
	const SA::Position len = wEditor.GetCurLine(0, nullptr);
	// Allocate buffer, including space for NUL
	std::string text(len, '\0');
	// And get the line
	wEditor.GetCurLine(len, &text[0]);
	return text;
}

/**
 * Check if the given line is a preprocessor condition line.
 * @return The kind of preprocessor condition (enum values).
 */
SciTEBase::PreProc SciTEBase::LinePreprocessorCondition(SA::Line line) {
	const std::string text = GetLine(line);

	const char *currChar = text.c_str();

	while (IsASpace(*currChar)) {
		currChar++;
	}
	if (preprocessorSymbol && (*currChar == preprocessorSymbol)) {
		currChar++;
		while (IsASpace(*currChar)) {
			currChar++;
		}
		std::string word;
		while (*currChar && !IsASpace(*currChar)) {
			word.push_back(*currChar++);
		}
		std::map<std::string, PreProc>::const_iterator it = preprocOfString.find(word);
		if (it != preprocOfString.end()) {
			return it->second;
		}
	}
	return PreProc::None;
}

/**
 * Search a matching preprocessor condition line.
 * @return @c true if the end condition are meet.
 * Also set curLine to the line where one of these conditions is mmet.
 */
bool SciTEBase::FindMatchingPreprocessorCondition(
	SA::Line &curLine,   		///< Number of the line where to start the search
	int direction,   		///< Direction of search: 1 = forward, -1 = backward
	PreProc condEnd1,   		///< First status of line for which the search is OK
	PreProc condEnd2) {		///< Second one

	bool isInside = false;
	int level = 0;
	const SA::Line maxLines = wEditor.LineCount() - 1;

	while (curLine < maxLines && curLine > 0 && !isInside) {
		curLine += direction;	// Increment or decrement
		const PreProc status = LinePreprocessorCondition(curLine);

		if ((direction == 1 && status == PreProc::Start) || (direction == -1 && status == PreProc::End)) {
			level++;
		} else if (level > 0 && ((direction == 1 && status == PreProc::End) || (direction == -1 && status == PreProc::Start))) {
			level--;
		} else if (level == 0 && (status == condEnd1 || status == condEnd2)) {
			isInside = true;
		}
	}

	return isInside;
}

/**
 * Find if there is a preprocessor condition after or before the caret position,
 * @return @c true if inside a preprocessor condition.
 */
bool SciTEBase::FindMatchingPreprocCondPosition(
	bool isForward,   		///< @c true if search forward
	SA::Position mppcAtCaret,   	///< Matching preproc. cond.: current position of caret
	SA::Position &mppcMatch) {		///< Matching preproc. cond.: matching position

	bool isInside = false;

	// Get current line
	SA::Line curLine = wEditor.LineFromPosition(mppcAtCaret);
	const PreProc status = LinePreprocessorCondition(curLine);

	switch (status) {
	case PreProc::Start:
		if (isForward) {
			isInside = FindMatchingPreprocessorCondition(curLine, 1,
					PreProc::Middle, PreProc::End);
		} else {
			mppcMatch = mppcAtCaret;
			return true;
		}
		break;
	case PreProc::Middle:
		if (isForward) {
			isInside = FindMatchingPreprocessorCondition(curLine, 1,
					PreProc::Middle, PreProc::End);
		} else {
			isInside = FindMatchingPreprocessorCondition(curLine, -1,
					PreProc::Start, PreProc::Middle);
		}
		break;
	case PreProc::End:
		if (isForward) {
			mppcMatch = mppcAtCaret;
			return true;
		} else {
			isInside = FindMatchingPreprocessorCondition(curLine, -1,
					PreProc::Start, PreProc::Middle);
		}
		break;
	default:   	// Should be noPPC

		if (isForward) {
			isInside = FindMatchingPreprocessorCondition(curLine, 1,
					PreProc::Middle, PreProc::End);
		} else {
			isInside = FindMatchingPreprocessorCondition(curLine, -1,
					PreProc::Start, PreProc::Middle);
		}
		break;
	}

	if (isInside) {
		mppcMatch = wEditor.LineStart(curLine);
	}
	return isInside;
}

static constexpr bool IsBrace(char ch) noexcept {
	return ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == '{' || ch == '}';
}

/**
 * Find if there is a brace next to the caret, checking before caret first, then
 * after caret. If brace found also find its matching brace.
 * @return @c true if inside a bracket pair.
 */
bool SciTEBase::FindMatchingBracePosition(bool editor, SA::Position &braceAtCaret, SA::Position &braceOpposite, bool sloppy) {
	bool isInside = false;
	GUI::ScintillaWindow &win = editor ? wEditor : wOutput;

	const int mainSel = win.MainSelection();
	if (win.SelectionNCaretVirtualSpace(mainSel) > 0)
		return false;

	const int bracesStyleCheck = editor ? bracesStyle : 0;
	SA::Position caretPos = win.CurrentPos();
	braceAtCaret = -1;
	braceOpposite = -1;
	char charBefore = '\0';
	int styleBefore = 0;
	const SA::Position lengthDoc = win.Length();
	TextReader acc(win);
	if ((lengthDoc > 0) && (caretPos > 0)) {
		// Check to ensure not matching brace that is part of a multibyte character
		if (win.PositionBefore(caretPos) == (caretPos - 1)) {
			charBefore = acc[caretPos - 1];
			styleBefore = acc.StyleAt(caretPos - 1);
		}
	}
	// Priority goes to character before caret
	if (charBefore && IsBrace(charBefore) &&
			((styleBefore == bracesStyleCheck) || (!bracesStyle))) {
		braceAtCaret = caretPos - 1;
	}
	bool colonMode = false;
	if ((lexLanguage == SCLEX_PYTHON) &&
			(':' == charBefore) && (SCE_P_OPERATOR == styleBefore)) {
		braceAtCaret = caretPos - 1;
		colonMode = true;
	}
	bool isAfter = true;
	if (lengthDoc > 0 && sloppy && (braceAtCaret < 0) && (caretPos < lengthDoc)) {
		// No brace found so check other side
		// Check to ensure not matching brace that is part of a multibyte character
		if (win.PositionAfter(caretPos) == (caretPos + 1)) {
			const char charAfter = acc[caretPos];
			const int styleAfter = acc.StyleAt(caretPos);
			if (charAfter && IsBrace(charAfter) && ((styleAfter == bracesStyleCheck) || (!bracesStyle))) {
				braceAtCaret = caretPos;
				isAfter = false;
			}
			if ((lexLanguage == SCLEX_PYTHON) &&
					(':' == charAfter) && (SCE_P_OPERATOR == styleAfter)) {
				braceAtCaret = caretPos;
				colonMode = true;
			}
		}
	}
	if (braceAtCaret >= 0) {
		if (colonMode) {
			const SA::Line lineStart = win.LineFromPosition(braceAtCaret);
			const SA::Line lineMaxSubord = win.LastChild(lineStart, static_cast<SA::FoldLevel>(-1));
			braceOpposite = win.LineEnd(lineMaxSubord);
		} else {
			braceOpposite = win.BraceMatch(braceAtCaret, 0);
		}
		if (braceOpposite > braceAtCaret) {
			isInside = isAfter;
		} else {
			isInside = !isAfter;
		}
	}
	return isInside;
}

void SciTEBase::BraceMatch(bool editor) {
	if (!bracesCheck)
		return;
	SA::Position braceAtCaret = -1;
	SA::Position braceOpposite = -1;
	FindMatchingBracePosition(editor, braceAtCaret, braceOpposite, bracesSloppy);
	GUI::ScintillaWindow &win = editor ? wEditor : wOutput;
	if ((braceAtCaret != -1) && (braceOpposite == -1)) {
		win.BraceBadLight(braceAtCaret);
		wEditor.SetHighlightGuide(0);
	} else {
		char chBrace = 0;
		if (braceAtCaret >= 0)
			chBrace = win.CharacterAt(braceAtCaret);
		win.BraceHighlight(braceAtCaret, braceOpposite);
		SA::Position columnAtCaret = win.Column(braceAtCaret);
		SA::Position columnOpposite = win.Column(braceOpposite);
		if (chBrace == ':') {
			const SA::Line lineStart = win.LineFromPosition(braceAtCaret);
			const SA::Position indentPos = win.LineIndentPosition(lineStart);
			const SA::Position indentPosNext = win.LineIndentPosition(lineStart + 1);
			columnAtCaret = win.Column(indentPos);
			const SA::Position columnAtCaretNext = win.Column(indentPosNext);
			const int indentSize = win.Indent();
			if (columnAtCaretNext - indentSize > 1)
				columnAtCaret = columnAtCaretNext - indentSize;
			if (columnOpposite == 0)	// If the final line of the structure is empty
				columnOpposite = columnAtCaret;
		} else {
			if (win.LineFromPosition(braceAtCaret) == win.LineFromPosition(braceOpposite)) {
				// Avoid attempting to draw a highlight guide
				columnAtCaret = 0;
				columnOpposite = 0;
			}
		}

		if (props.GetInt("highlight.indentation.guides"))
			win.SetHighlightGuide(std::min(columnAtCaret, columnOpposite));
	}
}

void SciTEBase::SetWindowName() {
	if (filePath.IsUntitled()) {
		windowName = localiser.Text("Untitled");
		windowName.insert(0, GUI_TEXT("("));
		windowName += GUI_TEXT(")");
	} else if (props.GetInt("title.full.path") == 2) {
		windowName = FileNameExt().AsInternal();
		windowName += GUI_TEXT(" ");
		windowName += localiser.Text("in");
		windowName += GUI_TEXT(" ");
		windowName += filePath.Directory().AsInternal();
	} else if (props.GetInt("title.full.path") == 1) {
		windowName = filePath.AsInternal();
	} else {
		windowName = FileNameExt().AsInternal();
	}
	if (CurrentBufferConst()->isReadOnly)
		windowName += GUI_TEXT(" |");
	if (CurrentBufferConst()->isDirty)
		windowName += GUI_TEXT(" * ");
	else
		windowName += GUI_TEXT(" - ");
	windowName += appName;

	if (buffers.length > 1 && props.GetInt("title.show.buffers")) {
		windowName += GUI_TEXT(" [");
		windowName += GUI::StringFromInteger(buffers.Current() + 1);
		windowName += GUI_TEXT(" ");
		windowName += localiser.Text("of");
		windowName += GUI_TEXT(" ");
		windowName += GUI::StringFromInteger(buffers.length);
		windowName += GUI_TEXT("]");
	}

	wSciTE.SetTitle(windowName.c_str());
}

SA::Span SciTEBase::GetSelection() {
	return wEditor.SelectionSpan();
}

SelectedRange SciTEBase::GetSelectedRange() {
	return SelectedRange(wEditor.CurrentPos(), wEditor.Anchor());
}

void SciTEBase::SetSelection(SA::Position anchor, SA::Position currentPos) {
	wEditor.SetSel(anchor, currentPos);
}

std::string SciTEBase::GetCTag() {
	const SA::Position lengthDoc = pwFocussed->Length();
	SA::Position selEnd = pwFocussed->SelectionEnd();
	SA::Position selStart = selEnd;
	TextReader acc(*pwFocussed);
	int mustStop = 0;
	while (!mustStop) {
		if (selStart < lengthDoc - 1) {
			selStart++;
			const char c = acc[selStart];
			if (c == '\r' || c == '\n') {
				mustStop = -1;
			} else if (c == '\t' && ((acc[selStart + 1] == '/' && acc[selStart + 2] == '^') || IsADigit(acc[selStart + 1]))) {
				mustStop = 1;
			}
		} else {
			mustStop = -1;
		}
	}
	if (mustStop == 1 && (acc[selStart + 1] == '/' && acc[selStart + 2] == '^')) {	// Found
		selEnd = selStart += 3;
		mustStop = 0;
		while (!mustStop) {
			if (selEnd < lengthDoc - 1) {
				selEnd++;
				const char c = acc[selEnd];
				if (c == '\r' || c == '\n') {
					mustStop = -1;
				} else if (c == '$' && acc[selEnd + 1] == '/') {
					mustStop = 1;	// Found!
				}

			} else {
				mustStop = -1;
			}
		}
	} else if (mustStop == 1 && IsADigit(acc[selStart + 1])) {
		// a Tag can be referenced by line Number also
		selEnd = selStart += 1;
		while ((selEnd < lengthDoc) && IsADigit(acc[selEnd])) {
			selEnd++;
		}
	}

	if (selStart < selEnd) {
		return pwFocussed->StringOfSpan(SA::Span(selStart, selEnd));
	} else {
		return std::string();
	}
}

// Default characters that can appear in a word
bool SciTEBase::iswordcharforsel(char ch) {
	return !strchr("\t\n\r !\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~", ch);
}

// Accept slightly more characters than for a word
// Doesn't accept all valid characters, as they are rarely used in source filenames...
// Accept path separators '/' and '\', extension separator '.', and ':', MS drive unit
// separator, and also used for separating the line number for grep. Same for '(' and ')' for cl.
// Accept '?' and '%' which are used in URL.
bool SciTEBase::isfilenamecharforsel(char ch) {
	return !strchr("\t\n\r \"$'*,;<>[]^`{|}", ch);
}

bool SciTEBase::islexerwordcharforsel(char ch) {
	// If there are no word.characters defined for the current file, fall back on the original function
	if (wordCharacters.length())
		return Contains(wordCharacters, ch);
	else
		return iswordcharforsel(ch);
}

void SciTEBase::HighlightCurrentWord(bool highlight) {
	if (!currentWordHighlight.isEnabled)
		return;
	if (!wEditor.HasFocus() && !wOutput.HasFocus() && highlight) {
		// Neither text window has focus, possibly app is inactive so do not highlight
		return;
	}
	GUI::ScintillaWindow &wCurrent = wOutput.HasFocus() ? wOutput : wEditor;
	// Remove old indicators if any exist.
	wCurrent.SetIndicatorCurrent(indicatorHighlightCurrentWord);
	const SA::Position lenDoc = wCurrent.Length();
	wCurrent.IndicatorClearRange(0, lenDoc);
	if (!highlight)
		return;
	if (FilterShowing()) {
		return;
	}
	// Get start & end selection.
	SA::Span sel = wCurrent.SelectionSpan();
	const bool noUserSelection = sel.start == sel.end;
	std::string sWordToFind = RangeExtendAndGrab(wCurrent, sel,
				  &SciTEBase::islexerwordcharforsel);
	if (sWordToFind.length() == 0 || (sWordToFind.find_first_of("\n\r ") != std::string::npos))
		return; // No highlight when no selection or multi-lines selection.
	if (noUserSelection && currentWordHighlight.statesOfDelay == CurrentWordHighlight::StatesOfDelay::noDelay) {
		// Manage delay before highlight when no user selection but there is word at the caret.
		currentWordHighlight.statesOfDelay = CurrentWordHighlight::StatesOfDelay::delay;
		// Reset timer
		currentWordHighlight.elapsedTimes.Duration(true);
		return;
	}
	// Get style of the current word to highlight only word with same style.
	int selectedStyle = wCurrent.UnsignedStyleAt(sel.start);
	if (!currentWordHighlight.isOnlyWithSameStyle)
		selectedStyle = -1;

	// Manage word with DBCS.
	const std::string wordToFind = EncodeString(sWordToFind);

	const SA::FindOption searchFlags = SA::FindOption::MatchCase | SA::FindOption::WholeWord;
	matchMarker.StartMatch(&wCurrent, wordToFind,
			       searchFlags, selectedStyle,
			       indicatorHighlightCurrentWord, -1);
	SetIdler(true);
}

std::string SciTEBase::GetRangeInUIEncoding(GUI::ScintillaWindow &win, SA::Span span) {
	return win.StringOfSpan(span);
}

std::string SciTEBase::GetLine(GUI::ScintillaWindow &win, SA::Line line) {
	const SA::Position lineStart = win.LineStart(line);
	const SA::Position lineEnd = win.LineEnd(line);
	if ((lineStart < 0) || (lineEnd < 0))
		return std::string();
	return win.StringOfSpan(SA::Span(lineStart, lineEnd));
}

void SciTEBase::RangeExtend(
	GUI::ScintillaWindow &wCurrent,
	SA::Span &span,
	bool (SciTEBase::*ischarforsel)(char ch)) {	///< Function returning @c true if the given char. is part of the selection.
	if (span.start == span.end && ischarforsel) {
		// Empty span and have a function to extend it
		const SA::Position lengthDoc = wCurrent.Length();
		TextReader acc(wCurrent);
		// Try and find a word at the caret
		// On the left...
		while ((span.start > 0) && ((this->*ischarforsel)(acc[span.start - 1]))) {
			span.start--;
		}
		// and on the right
		while ((span.end < lengthDoc) && ((this->*ischarforsel)(acc[span.end]))) {
			span.end++;
		}
	}
}

std::string SciTEBase::RangeExtendAndGrab(
	GUI::ScintillaWindow &wCurrent,
	SA::Span &span,
	bool (SciTEBase::*ischarforsel)(char ch),	///< Function returning @c true if the given char. is part of the selection.
	bool stripEol /*=true*/) {

	RangeExtend(wCurrent, span, ischarforsel);
	std::string selected;
	if (span.start != span.end) {
		selected = GetRangeInUIEncoding(wCurrent, span);
	}
	if (stripEol) {
		// Change whole line selected but normally end of line characters not wanted.
		// Remove possible terminating \r, \n, or \r\n.
		const size_t sellen = selected.length();
		if (sellen >= 2 && (selected[sellen - 2] == '\r' && selected[sellen - 1] == '\n')) {
			selected.erase(sellen - 2);
		} else if (sellen >= 1 && (selected[sellen - 1] == '\r' || selected[sellen - 1] == '\n')) {
			selected.erase(sellen - 1);
		}
	}

	return selected;
}

/**
 * If there is selected text, either in the editor or the output pane,
 * put the selection in the @a sel buffer, up to @a len characters.
 * Otherwise, try and select characters around the caret, as long as they are OK
 * for the @a ischarforsel function.
 * Remove the last two character controls from the result, as they are likely
 * to be CR and/or LF.
 */
std::string SciTEBase::SelectionExtend(
	bool (SciTEBase::*ischarforsel)(char ch),	///< Function returning @c true if the given char. is part of the selection.
	bool stripEol /*=true*/) {

	SA::Span sel = pwFocussed->SelectionSpan();
	return RangeExtendAndGrab(*pwFocussed, sel, ischarforsel, stripEol);
}

std::string SciTEBase::SelectionWord(bool stripEol /*=true*/) {
	return SelectionExtend(&SciTEBase::islexerwordcharforsel, stripEol);
}

std::string SciTEBase::SelectionFilename() {
	return SelectionExtend(&SciTEBase::isfilenamecharforsel);
}

void SciTEBase::SelectionIntoProperties() {
	std::string currentSelection = SelectionExtend(nullptr, false);
	props.Set("CurrentSelection", currentSelection);

	std::string word = SelectionWord();
	props.Set("CurrentWord", word);

	const SA::Span range = PaneFocused().SelectionSpan();
	props.Set("SelectionStartLine", std::to_string(PaneFocused().LineFromPosition(range.start) + 1));
	props.Set("SelectionStartColumn", std::to_string(PaneFocused().Column(range.start) + 1));
	props.Set("SelectionEndLine", std::to_string(PaneFocused().LineFromPosition(range.end) + 1));
	props.Set("SelectionEndColumn", std::to_string(PaneFocused().Column(range.end) + 1));
}

void SciTEBase::SelectionIntoFind(bool stripEol /*=true*/) {
	std::string sel = SelectionWord(stripEol);
	if (sel.length() && (sel.find_first_of("\r\n") == std::string::npos)) {
		// The selection does not include a new line, so is likely to be
		// the expression to search...
		findWhat = sel;
		if (unSlash) {
			std::string slashedFind = Slash(findWhat, false);
			findWhat = slashedFind;
		}
	}
	// else findWhat remains the same as last time.
}

void SciTEBase::SelectionAdd(AddSelection add) {
	SA::FindOption flags = SA::FindOption::None;
	if (!pwFocussed->SelectionEmpty()) {
		// If selection is word then match as word.
		if (pwFocussed->IsRangeWord(pwFocussed->SelectionStart(),
					    pwFocussed->SelectionEnd()))
			flags = SA::FindOption::WholeWord;
	}
	pwFocussed->TargetWholeDocument();
	pwFocussed->SetSearchFlags(flags);
	if (add == AddSelection::next) {
		pwFocussed->MultipleSelectAddNext();
	} else {
		if (pwFocussed->SelectionEmpty()) {
			pwFocussed->MultipleSelectAddNext();
		}
		pwFocussed->MultipleSelectAddEach();
	}
}

std::string SciTEBase::EncodeString(const std::string &s) {
	return s;
}

static std::string UnSlashAsNeeded(const std::string &s, bool escapes, bool regularExpression) {
	if (escapes) {
		if (regularExpression) {
			// For regular expressions, the only escape sequences allowed start with \0
			// Other sequences, like \t, are handled by the RE engine.
			return UnSlashLowOctalString(s);
		} else {
			// C style escapes allowed
			return UnSlashString(s);
		}
	} else {
		return s;
	}
}

void SciTEBase::RemoveFindMarks() {
	findMarker.Stop();	// Cancel ongoing background find
	if (CurrentBuffer()->findMarks != Buffer::FindMarks::none) {
		wEditor.SetIndicatorCurrent(indicatorMatch);
		wEditor.IndicatorClearRange(0, LengthDocument());
		CurrentBuffer()->findMarks = Buffer::FindMarks::none;
	}
	wEditor.MarkerDeleteAll(markerFilterMatch);
	wEditor.AnnotationClearAll();
}

SA::FindOption SciTEBase::SearchFlags(bool regularExpressions) const {
	SA::FindOption opt = SA::FindOption::None;
	if (wholeWord)
		opt |= SA::FindOption::WholeWord;
	if (matchCase)
		opt |= SA::FindOption::MatchCase;
	if (regularExpressions)
		opt |= SA::FindOption::RegExp;
	if (props.GetInt("find.replace.regexp.posix"))
		opt |= SA::FindOption::Posix;
	if (props.GetInt("find.replace.regexp.cpp11"))
		opt |= SA::FindOption::Cxx11RegEx;
	return opt;
}

void SciTEBase::MarkAll(MarkPurpose purpose) {
	RemoveFindMarks();
	wEditor.SetIndicatorCurrent(indicatorMatch);

	int bookMark = -1;
	std::optional<SA::Line> contextLines;

	const SA::IndicatorNumbers indicatorNumMatch = static_cast<SA::IndicatorNumbers>(indicatorMatch);

	if (purpose == MarkPurpose::incremental) {
		CurrentBuffer()->findMarks = Buffer::FindMarks::temporary;
		SetOneIndicator(wEditor, indicatorNumMatch,
			IndicatorDefinition(props.GetString("find.indicator.incremental")));
	} else if (purpose == MarkPurpose::filter) {
		CurrentBuffer()->findMarks = Buffer::FindMarks::temporary;
		SetOneIndicator(wEditor, indicatorNumMatch,
				IndicatorDefinition(props.GetString("filter.match.indicator")));
		bookMark = markerFilterMatch;
		contextLines = contextVisible ? props.GetInt("filter.context", 2) : 0;
	} else {
		CurrentBuffer()->findMarks = Buffer::FindMarks::marked;
		std::string findIndicatorString = props.GetString("find.mark.indicator");
		IndicatorDefinition findIndicator(findIndicatorString);
		if (!findIndicatorString.length()) {
			findIndicator.style = SA::IndicatorStyle::RoundBox;
			std::string findMark = props.GetString("find.mark");
			if (findMark.length())
				findIndicator.colour = ColourFromString(findMark);
			findIndicator.fillAlpha = alphaIndicator;
			findIndicator.under = underIndicator;
		}
		SetOneIndicator(wEditor, indicatorNumMatch, findIndicator);
		bookMark = markerBookmark;
	}

	const std::string findTarget = UnSlashAsNeeded(EncodeString(findWhat), unSlash, regExp);
	if (findTarget.length() == 0) {
		return;
	}

	findMarker.StartMatch(&wEditor, findTarget,
			      SearchFlags(regExp), -1,
			      indicatorMatch, bookMark, contextLines);
	SetIdler(true);
}

void SciTEBase::FilterAll(bool showMatches) {

	HighlightCurrentWord(false);
	wEditor.MarkerDeleteAll(markerFilterMatch);

	if (!showMatches || findWhat.empty()) {
		RemoveFindMarks();
		// Show all lines
		wEditor.ShowLines(0, wEditor.LineFromPosition(wEditor.Length()));
		// Restore fold margin
		wEditor.SetMarginWidthN(2, foldMargin ? foldMarginWidth : 0);
		// May have selected something in filter so scroll to it
		wEditor.ScrollCaret();
		RestoreFolds(CurrentBuffer()->foldState);
		return;
	}

	// Hide fold margin as the shapes will overlap hidden lines and not make sense
	wEditor.SetMarginWidthN(2, 0);

	wEditor.SetRedraw(false);
	wEditor.SetSearchFlags(SearchFlags(regExp));

	MarkAll(Searcher::MarkPurpose::filter);
	wEditor.SetRedraw(true);
}

int SciTEBase::IncrementSearchMode() {
	FindIncrement();
	return 0;
}

int SciTEBase::FilterSearch() {
	Filter();
	return 0;
}

void SciTEBase::FailedSaveMessageBox(const FilePath &filePathSaving) {
	const GUI::gui_string msg = LocaliseMessage(
					    "Could not save file \"^0\".", filePathSaving.AsInternal());
	WindowMessageBox(wSciTE, msg);
}

bool SciTEBase::FindReplaceAdvanced() const {
	return props.GetInt("find.replace.advanced");
}

SA::Position SciTEBase::FindInTarget(const std::string &findWhatText, SA::Span range) {
	wEditor.SetTarget(range);
	SA::Position posFind = wEditor.SearchInTarget(findWhatText);
	while (findInStyle && (posFind >= 0) && (findStyle != wEditor.UnsignedStyleAt(posFind))) {
		if (range.start < range.end) {
			wEditor.SetTarget(SA::Span(posFind + 1, range.end));
		} else {
			wEditor.SetTarget(SA::Span(range.start, posFind + 1));
		}
		posFind = wEditor.SearchInTarget(findWhatText);
	}
	return posFind;
}

void SciTEBase::SetFindText(std::string_view sFind) {
	findWhat = sFind;
	props.Set("find.what", findWhat);
}

void SciTEBase::SetFind(std::string_view sFind) {
	SetFindText(sFind);
	InsertFindInMemory();
}

bool SciTEBase::FindHasText() const noexcept {
	return !findWhat.empty();
}

void SciTEBase::SetReplace(std::string_view sReplace) {
	replaceWhat = sReplace;
	memReplaces.Insert(replaceWhat);
}

void SciTEBase::SetCaretAsStart() {
	searchStartPosition = wEditor.SelectionStart();
}

void SciTEBase::MoveBack() {
	SetSelection(searchStartPosition, searchStartPosition);
}

void SciTEBase::ScrollEditorIfNeeded() {
	GUI::Point ptCaret;
	const SA::Position caret = wEditor.CurrentPos();
	ptCaret.x = wEditor.PointXFromPosition(caret);
	ptCaret.y = wEditor.PointYFromPosition(caret);
	ptCaret.y += wEditor.TextHeight(0) - 1;

	const GUI::Rectangle rcEditor = wEditor.GetClientPosition();
	if (!rcEditor.Contains(ptCaret))
		wEditor.ScrollCaret();
}

SA::Position SciTEBase::FindNext(bool reverseDirection, bool showWarnings, bool allowRegExp) {
	if (findWhat.length() == 0) {
		Find();
		return -1;
	}
	const std::string findTarget = UnSlashAsNeeded(EncodeString(findWhat), unSlash, regExp);
	if (findTarget.length() == 0)
		return -1;

	const SA::Position lengthDoc = wEditor.Length();
	const SA::Span rangeSelection = wEditor.SelectionSpan();
	SA::Span rangeSearch(rangeSelection.end, lengthDoc);
	if (reverseDirection) {
		rangeSearch = SA::Span(rangeSelection.start, 0);
	}

	wEditor.SetSearchFlags(SearchFlags(allowRegExp && regExp));
	SA::Position posFind = FindInTarget(findTarget, rangeSearch);
	if (posFind == -1 && wrapFind) {
		// Failed to find in indicated direction
		// so search from the beginning (forward) or from the end (reverse)
		// unless wrapFind is false
		const SA::Span rangeAll = reverseDirection ?
					   SA::Span(lengthDoc, 0) : SA::Span(0, lengthDoc);
		posFind = FindInTarget(findTarget, rangeAll);
		WarnUser(warnFindWrapped);
	}
	if (posFind < 0) {
		havefound = false;
		failedfind = true;
		if (showWarnings) {
			WarnUser(warnNotFound);
			FindMessageBox("Can not find the string '^0'.",
				       &findWhat);
		}
	} else {
		havefound = true;
		failedfind = false;
		const SA::Span rangeTarget = wEditor.TargetSpan();
		// Ensure found text is styled so that caret will be made visible but
		// only perform style in synchronous styling mode.
		const SA::Position endStyled = wEditor.EndStyled();
		if ((endStyled < rangeTarget.end) && (idleStyling == SA::IdleStyling::None)) {
			wEditor.Colourise(endStyled,
					  wEditor.LineStart(wEditor.LineFromPosition(rangeTarget.end) + 1));
		}
		EnsureRangeVisible(wEditor, rangeTarget);
		wEditor.ScrollRange(rangeTarget.start, rangeTarget.end);
		SetSelection(rangeTarget.start, rangeTarget.end);
		if (!replacing && (closeFind != CloseFind::closePrevent)) {
			DestroyFindReplace();
		}
	}
	return posFind;
}

void SciTEBase::HideMatch() {
}

void SciTEBase::ReplaceOnce(bool showWarnings) {
	if (!FindHasText())
		return;

	bool haveWarned = false;
	if (!havefound) {
		const SA::Span rangeSelection = wEditor.SelectionSpan();
		SetSelection(rangeSelection.start, rangeSelection.start);
		FindNext(false);
		haveWarned = !havefound;
	}

	if (havefound) {
		const std::string replaceTarget = UnSlashAsNeeded(EncodeString(replaceWhat), unSlash, regExp);
		const SA::Span rangeSelection = wEditor.SelectionSpan();
		wEditor.SetTarget(rangeSelection);
		SA::Position lenReplaced = replaceTarget.length();
		if (regExp)
			lenReplaced = wEditor.ReplaceTargetRE(replaceTarget);
		else	// Allow \0 in replacement
			wEditor.ReplaceTarget(replaceTarget);
		SetSelection(rangeSelection.start + lenReplaced, rangeSelection.start);
		SetCaretAsStart();
		havefound = false;
	}

	FindNext(false, showWarnings && !haveWarned);
}

intptr_t SciTEBase::DoReplaceAll(bool inSelection) {
	const std::string findTarget = UnSlashAsNeeded(EncodeString(findWhat), unSlash, regExp);
	if (findTarget.length() == 0) {
		return -1;
	}

	const SA::Span rangeSelection = wEditor.SelectionSpan();
	SA::Span rangeSearch = rangeSelection;
	const int countSelections = wEditor.Selections();
	if (inSelection) {
		const SA::SelectionMode selType = wEditor.SelectionMode();
		if (selType == SA::SelectionMode::Lines) {
			// Take care to replace in whole lines
			const SA::Line startLine = wEditor.LineFromPosition(rangeSearch.start);
			rangeSearch.start = wEditor.LineStart(startLine);
			const SA::Line endLine = wEditor.LineFromPosition(rangeSearch.end);
			rangeSearch.end = wEditor.LineStart(endLine + 1);
		} else {
			for (int i=0; i<countSelections; i++) {
				rangeSearch.start = std::min(rangeSearch.start, wEditor.SelectionNStart(i));
				rangeSearch.end = std::max(rangeSearch.end, wEditor.SelectionNEnd(i));
			}
		}
		if (rangeSearch.Length() == 0) {
			return -2;
		}
	} else {
		rangeSearch.end = LengthDocument();
		if (wrapFind) {
			// Whole document
			rangeSearch.start = 0;
		}
		// If not wrapFind, replace all only from caret to end of document
	}

	const std::string replaceTarget = UnSlashAsNeeded(EncodeString(replaceWhat), unSlash, regExp);
	wEditor.SetSearchFlags(SearchFlags(regExp));
	SA::Position posFind = FindInTarget(findTarget, rangeSearch);
	if ((posFind >= 0) && (posFind <= rangeSearch.end)) {
		SA::Position lastMatch = posFind;
		intptr_t replacements = 0;
		wEditor.BeginUndoAction();
		// Replacement loop
		while (posFind >= 0) {
			const SA::Position lenTarget = wEditor.TargetEnd() - wEditor.TargetStart();
			if (inSelection && countSelections > 1) {
				// We must check that the found target is entirely inside a selection
				bool insideASelection = false;
				for (int i=0; i<countSelections && !insideASelection; i++) {
					const SA::Position startPos = wEditor.SelectionNStart(i);
					const SA::Position endPos = wEditor.SelectionNEnd(i);
					if (posFind >= startPos && posFind + lenTarget <= endPos)
						insideASelection = true;
				}
				if (!insideASelection) {
					// Found target is totally or partly outside the selections
					lastMatch = posFind + 1;
					if (lastMatch >= rangeSearch.end) {
						// Run off the end of the document/selection with an empty match
						posFind = -1;
					} else {
						posFind = FindInTarget(findTarget, SA::Span(lastMatch, rangeSearch.end));
					}
					continue;	// No replacement
				}
			}
			SA::Position lenReplaced = replaceTarget.length();
			if (regExp) {
				lenReplaced = wEditor.ReplaceTargetRE(replaceTarget);
			} else {
				wEditor.ReplaceTarget(replaceTarget);
			}
			// Modify for change caused by replacement
			rangeSearch.end += lenReplaced - lenTarget;
			// For the special cases of start of line and end of line
			// something better could be done but there are too many special cases
			lastMatch = posFind + lenReplaced;
			if (lenTarget <= 0) {
				lastMatch = wEditor.PositionAfter(lastMatch);
			}
			if (lastMatch >= rangeSearch.end) {
				// Run off the end of the document/selection with an empty match
				posFind = -1;
			} else {
				posFind = FindInTarget(findTarget, SA::Span(lastMatch, rangeSearch.end));
			}
			replacements++;
		}
		if (inSelection) {
			if (countSelections == 1)
				SetSelection(rangeSearch.start, rangeSearch.end);
		} else {
			SetSelection(lastMatch, lastMatch);
		}
		wEditor.EndUndoAction();
		return replacements;
	}
	return 0;
}

intptr_t SciTEBase::ReplaceAll(bool inSelection) {
	wEditor.SetRedraw(false);
	const intptr_t replacements = DoReplaceAll(inSelection);
	wEditor.SetRedraw(true);
	props.Set("Replacements", std::to_string(replacements > 0 ? replacements : 0));
	UpdateStatusBar(false);
	if (replacements == -1) {
		FindMessageBox(
			inSelection ?
			"Find string must not be empty for 'Replace in Selection' command." :
			"Find string must not be empty for 'Replace All' command.");
	} else if (replacements == -2) {
		FindMessageBox(
			"Selection must not be empty for 'Replace in Selection' command.");
	} else if (replacements == 0) {
		FindMessageBox(
			"No replacements because string '^0' was not present.", &findWhat);
	}
	return replacements;
}

intptr_t SciTEBase::ReplaceInBuffers() {
	const BufferIndex currentBuffer = buffers.Current();
	intptr_t replacements = 0;
	for (int i = 0; i < buffers.length; i++) {
		SetDocumentAt(i);
		replacements += DoReplaceAll(false);
		if (i == 0 && replacements < 0) {
			FindMessageBox(
				"Find string must not be empty for 'Replace in Buffers' command.");
			break;
		}
	}
	SetDocumentAt(currentBuffer);
	props.Set("Replacements", std::to_string(replacements));
	UpdateStatusBar(false);
	if (replacements == 0) {
		FindMessageBox(
			"No replacements because string '^0' was not present.", &findWhat);
	}
	return replacements;
}

void SciTEBase::UIClosed() {
	if (CurrentBuffer()->findMarks == Buffer::FindMarks::temporary) {
		RemoveFindMarks();
	}
}

void SciTEBase::UIHasFocus() {
}

void SciTEBase::OutputAppendString(const char *s, SA::Position len) {
	if (len == -1)
		len = strlen(s);
	wOutput.AppendText(len, s);
	if (scrollOutput) {
		const SA::Line line = wOutput.LineCount();
		const SA::Position lineStart = wOutput.LineStart(line);
		wOutput.GotoPos(lineStart);
	}
}

void SciTEBase::OutputAppendStringSynchronised(const char *s, SA::Position len) {
	// This may be called from secondary thread so always use Send instead of Call
	if (len == -1)
		len = strlen(s);
	wOutput.Send(SCI_APPENDTEXT, len, SptrFromString(s));
	if (scrollOutput) {
		const SA::Line line = wOutput.Send(SCI_GETLINECOUNT);
		const SA::Position lineStart = wOutput.Send(SCI_POSITIONFROMLINE, line);
		wOutput.Send(SCI_GOTOPOS, lineStart);
	}
}

void SciTEBase::Execute() {
	props.Set("CurrentMessage", "");
	dirNameForExecute = FilePath();
	bool displayParameterDialog = false;
	parameterisedCommand = "";
	for (size_t ic = 0; ic < jobQueue.commandMax; ic++) {
		if (StartsWith(jobQueue.jobQueue[ic].command, "*")) {
			displayParameterDialog = true;
			jobQueue.jobQueue[ic].command.erase(0, 1);
			parameterisedCommand = jobQueue.jobQueue[ic].command;
		}
		if (jobQueue.jobQueue[ic].directory.IsSet()) {
			dirNameForExecute = jobQueue.jobQueue[ic].directory;
		}
	}
	if (displayParameterDialog) {
		if (!ParametersDialog(true)) {
			jobQueue.ClearJobs();
			return;
		}
	} else {
		ParamGrab();
	}
	for (size_t ic = 0; ic < jobQueue.commandMax; ic++) {
		if (jobQueue.jobQueue[ic].jobType != JobSubsystem::grep) {
			jobQueue.jobQueue[ic].command = props.Expand(jobQueue.jobQueue[ic].command);
		}
	}

	if (jobQueue.ClearBeforeExecute()) {
		wOutput.ClearAll();
	}

	wOutput.MarkerDeleteAll(-1);
	wEditor.MarkerDeleteAll(0);
	// Ensure the output pane is visible
	if (jobQueue.ShowOutputPane()) {
		SetOutputVisibility(true);
	}

	jobQueue.SetCancelFlag(false);
	if (jobQueue.HasCommandToRun()) {
		jobQueue.SetExecuting(true);
	}
	CheckMenus();
	dirNameAtExecute = filePath.Directory();
}

void SciTEBase::SetOutputVisibility(bool show) {
	if (show) {
		if (heightOutput <= 0) {
			if (previousHeightOutput < 20) {
				if (splitVertical)
					heightOutput = NormaliseSplit(300);
				else
					heightOutput = NormaliseSplit(100);
				previousHeightOutput = heightOutput;
			} else {
				heightOutput = NormaliseSplit(previousHeightOutput);
			}
		}
	} else {
		if (heightOutput > 0) {
			heightOutput = NormaliseSplit(0);
			WindowSetFocus(wEditor);
		}
	}
	SizeSubWindows();
	Redraw();
}

// Background threads that are send text to the output pane want it to be made visible.
// Derived methods for each platform may perform thread synchronization.
void SciTEBase::ShowOutputOnMainThread() {
	SetOutputVisibility(true);
}

void SciTEBase::ToggleOutputVisible() {
	SetOutputVisibility(heightOutput <= 0);
}

void SciTEBase::BookmarkAdd(SA::Line lineno) {
	if (lineno == -1)
		lineno = GetCurrentLineNumber();
	if (!BookmarkPresent(lineno))
		wEditor.MarkerAdd(lineno, markerBookmark);
}

void SciTEBase::BookmarkDelete(SA::Line lineno) {
	if (lineno == -1)
		lineno = GetCurrentLineNumber();
	if (BookmarkPresent(lineno))
		wEditor.MarkerDelete(lineno, markerBookmark);
}

bool SciTEBase::BookmarkPresent(SA::Line lineno) {
	if (lineno == -1)
		lineno = GetCurrentLineNumber();
	const int state = wEditor.MarkerGet(lineno);
	return state & (1 << markerBookmark);
}

void SciTEBase::BookmarkToggle(SA::Line lineno) {
	if (lineno == -1)
		lineno = GetCurrentLineNumber();
	if (BookmarkPresent(lineno)) {
		while (BookmarkPresent(lineno)) {
			BookmarkDelete(lineno);
		}
	} else {
		BookmarkAdd(lineno);
	}
}

void SciTEBase::BookmarkNext(bool forwardScan, bool select) {
	const SA::Line lineno = GetCurrentLineNumber();
	SA::Message sciMarker = SA::Message::MarkerNext;
	SA::Line lineStart = lineno + 1;	//Scan starting from next line
	SA::Line lineRetry = 0;				//If not found, try from the beginning
	const SA::Position anchor = wEditor.Anchor();
	if (!forwardScan) {
		lineStart = lineno - 1;		//Scan starting from previous line
		lineRetry = wEditor.LineCount();	//If not found, try from the end
		sciMarker = SA::Message::MarkerPrevious;
	}
	constexpr unsigned int maskBookmark = 1 << markerBookmark;
	SA::Line nextLine = wEditor.Call(sciMarker, lineStart, maskBookmark);
	if (nextLine < 0)
		nextLine = wEditor.Call(sciMarker, lineRetry, maskBookmark);
	if (nextLine < 0 || nextLine == lineno)	// No bookmark (of the given type) or only one, and already on it
		WarnUser(warnNoOtherBookmark);
	else {
		GotoLineEnsureVisible(nextLine);
		if (select) {
			wEditor.SetAnchor(anchor);
		}
	}
}

void SciTEBase::BookmarkSelectAll() {
	std::vector<SA::Line> bookmarks;
	SA::Line lineBookmark = -1;
	while ((lineBookmark = wEditor.MarkerNext(lineBookmark + 1, 1 << markerBookmark)) >= 0) {
		bookmarks.push_back(lineBookmark);
	}
	for (size_t i = 0; i < bookmarks.size(); i++) {
		const SA::Span range = {
			wEditor.LineStart(bookmarks[i]),
			wEditor.LineStart(bookmarks[i] + 1)
		};
		if (i == 0) {
			wEditor.SetSelection(range.end, range.start);
		} else {
			wEditor.AddSelection(range.end, range.start);
		}
	}
}

GUI::Rectangle SciTEBase::GetClientRectangle() {
	return wContent.GetClientPosition();
}

void SciTEBase::Redraw() {
	wSciTE.InvalidateAll();
	wEditor.InvalidateAll();
	wOutput.InvalidateAll();
}

std::string SciTEBase::GetNearestWords(const char *wordStart, size_t searchLen,
				       const char *separators, bool ignoreCase /*=false*/, bool exactLen /*=false*/) {
	std::string words;
	while (words.empty() && *separators) {
		words = apis.GetNearestWords(wordStart, searchLen, ignoreCase, *separators, exactLen);
		separators++;
	}
	return words;
}

void SciTEBase::FillFunctionDefinition(SA::Position pos /*= -1*/) {
	if (pos > 0) {
		lastPosCallTip = pos;
	}
	if (apis) {
		std::string words = GetNearestWords(currentCallTipWord.c_str(), currentCallTipWord.length(),
						    calltipParametersStart.c_str(), callTipIgnoreCase, true);
		if (words.empty())
			return;
		// Counts how many call tips
		maxCallTips = static_cast<int>(std::count(words.begin(), words.end(), ' ') + 1);

		// Should get current api definition
		std::string word = apis.GetNearestWord(currentCallTipWord.c_str(), currentCallTipWord.length(),
						       callTipIgnoreCase, calltipWordCharacters, currentCallTip);
		if (word.length()) {
			functionDefinition = word;
			if (maxCallTips > 1) {
				functionDefinition.insert(0, "\001");
			}

			if (calltipEndDefinition != "") {
				const size_t posEndDef = functionDefinition.find(calltipEndDefinition);
				if (maxCallTips > 1) {
					if (posEndDef != std::string::npos) {
						functionDefinition.insert(posEndDef + calltipEndDefinition.length(), "\n\002");
					} else {
						functionDefinition.append("\n\002");
					}
				} else {
					if (posEndDef != std::string::npos) {
						functionDefinition.insert(posEndDef + calltipEndDefinition.length(), "\n");
					}
				}
			} else if (maxCallTips > 1) {
				functionDefinition.insert(1, "\002");
			}

			std::string definitionForDisplay;
			if (callTipUseEscapes) {
				definitionForDisplay = UnSlashString(functionDefinition);
			} else {
				definitionForDisplay = functionDefinition;
			}

			wEditor.CallTipShow(lastPosCallTip - currentCallTipWord.length(), definitionForDisplay.c_str());
			ContinueCallTip();
		}
	}
}

bool SciTEBase::StartCallTip() {
	currentCallTip = 0;
	currentCallTipWord = "";
	std::string line = GetCurrentLine();
	SA::Position current = GetCaretInLine();
	SA::Position pos = wEditor.CurrentPos();
	do {
		int braces = 0;
		while (current > 0 && (braces || !Contains(calltipParametersStart, line[current - 1]))) {
			if (Contains(calltipParametersStart, line[current - 1]))
				braces--;
			else if (Contains(calltipParametersEnd, line[current - 1]))
				braces++;
			current--;
			pos--;
		}
		if (current > 0) {
			current--;
			pos--;
		} else
			break;
		while (current > 0 && IsASpace(line[current - 1])) {
			current--;
			pos--;
		}
	} while (current > 0 && !Contains(calltipWordCharacters, line[current - 1]));
	if (current <= 0)
		return true;

	startCalltipWord = current - 1;
	while (startCalltipWord > 0 &&
			Contains(calltipWordCharacters, line[startCalltipWord - 1])) {
		startCalltipWord--;
	}

	line.at(current) = '\0';
	currentCallTipWord = line.c_str() + startCalltipWord;
	functionDefinition = "";
	FillFunctionDefinition(pos);
	return true;
}

void SciTEBase::ContinueCallTip() {
	std::string line = GetCurrentLine();
	const SA::Position current = GetCaretInLine();

	int braces = 0;
	int commas = 0;
	for (SA::Position i = startCalltipWord; i < current; i++) {
		if (Contains(calltipParametersStart, line[i]))
			braces++;
		else if (Contains(calltipParametersEnd, line[i]) && braces > 0)
			braces--;
		else if (braces == 1 && Contains(calltipParametersSeparators, line[i]))
			commas++;
	}

	size_t startHighlight = 0;
	while ((startHighlight < functionDefinition.length()) && !Contains(calltipParametersStart, functionDefinition[startHighlight]))
		startHighlight++;
	if ((startHighlight < functionDefinition.length()) && Contains(calltipParametersStart, functionDefinition[startHighlight]))
		startHighlight++;
	while ((startHighlight < functionDefinition.length()) && commas > 0) {
		if (Contains(calltipParametersSeparators, functionDefinition[startHighlight]))
			commas--;
		// If it reached the end of the argument list it means that the user typed in more
		// arguments than the ones listed in the calltip
		if (Contains(calltipParametersEnd, functionDefinition[startHighlight]))
			commas = 0;
		else
			startHighlight++;
	}
	if ((startHighlight < functionDefinition.length()) && Contains(calltipParametersSeparators, functionDefinition[startHighlight]))
		startHighlight++;
	size_t endHighlight = startHighlight;
	while ((endHighlight < functionDefinition.length()) && !Contains(calltipParametersSeparators, functionDefinition[endHighlight]) && !Contains(calltipParametersEnd, functionDefinition[endHighlight]))
		endHighlight++;
	if (callTipUseEscapes) {
		std::string sPreHighlight = functionDefinition.substr(0, startHighlight);
		std::vector<char> vPreHighlight(sPreHighlight.c_str(), sPreHighlight.c_str() + sPreHighlight.length() + 1);
		const size_t unslashedStartHighlight = UnSlash(&vPreHighlight[0]);

		size_t unslashedEndHighlight = unslashedStartHighlight;
		if (startHighlight < endHighlight) {
			std::string sHighlight = functionDefinition.substr(startHighlight, endHighlight - startHighlight);
			std::vector<char> vHighlight(sHighlight.c_str(), sHighlight.c_str() + sHighlight.length() + 1);
			unslashedEndHighlight = unslashedStartHighlight + UnSlash(&vHighlight[0]);
		}

		startHighlight = unslashedStartHighlight;
		endHighlight = unslashedEndHighlight;
	}

	wEditor.CallTipSetHlt(startHighlight, endHighlight);
}

std::string SciTEBase::EliminateDuplicateWords(const std::string &words) {
	std::set<std::string> wordSet;
	std::string wordsOut;

	size_t current = 0;
	while (current < words.length()) {
		const size_t afterWord = words.find(' ', current);
		const std::string word(words, current, afterWord - current);
		std::pair<std::set<std::string>::iterator, bool> result = wordSet.insert(word);
		if (result.second) {
			if (!wordsOut.empty())
				wordsOut += ' ';
			wordsOut += word;
		}
		current = afterWord;
		if (current < words.length())
			current++;
	}
	return wordsOut;
}

bool SciTEBase::StartAutoComplete() {
	const std::string line = GetCurrentLine();
	const SA::Position current = GetCaretInLine();

	SA::Position startword = current;

	while ((startword > 0) &&
			(Contains(calltipWordCharacters, line[startword - 1]) ||
			 Contains(autoCompleteStartCharacters, line[startword - 1]))) {
		startword--;
	}

	const std::string root = line.substr(startword, current - startword);
	if (apis) {
		const std::string words = GetNearestWords(root.c_str(), root.length(),
						    calltipParametersStart.c_str(), autoCompleteIgnoreCase);
		if (!words.empty()) {
			std::string wordsUnique = EliminateDuplicateWords(words);
			wEditor.AutoCSetSeparator(' ');
			wEditor.AutoCSetMaxHeight(autoCompleteVisibleItemCount);
			wEditor.AutoCShow(root.length(), wordsUnique.c_str());
		}
	}
	return true;
}

bool SciTEBase::StartAutoCompleteWord(bool onlyOneWord) {
	const std::string line = GetCurrentLine();
	const SA::Position current = GetCaretInLine();

	SA::Position startword = current;
	// Autocompletion of pure numbers is mostly an annoyance
	bool allNumber = true;
	while (startword > 0 && Contains(wordCharacters, line[startword - 1])) {
		startword--;
		if (line[startword] < '0' || line[startword] > '9') {
			allNumber = false;
		}
	}
	if (startword == current || allNumber)
		return true;
	const std::string root = line.substr(startword, current - startword);
	const SA::Position rootLength = root.length();
	const SA::Position doclen = LengthDocument();
	const SA::FindOption flags =
		SA::FindOption::WordStart | (autoCompleteIgnoreCase ? SA::FindOption::None : SA::FindOption::MatchCase);
	const SA::Position posCurrentWord = wEditor.CurrentPos() - rootLength;

	// wordList contains a list of words to display in an autocompletion list.
	AutoCompleteWordList wordList;

	wEditor.SetTarget(SA::Span(0, doclen));
	wEditor.SetSearchFlags(flags);
	SA::Position posFind = wEditor.SearchInTarget(root);
	TextReader acc(wEditor);
	while (posFind >= 0 && posFind < doclen) {	// search all the document
		SA::Position wordEnd = posFind + rootLength;
		if (posFind != posCurrentWord) {
			while (Contains(wordCharacters, acc.SafeGetCharAt(wordEnd)))
				wordEnd++;
			const SA::Position wordLength = wordEnd - posFind;
			if (wordLength > rootLength) {
				const std::string word = wEditor.StringOfSpan(SA::Span(posFind, wordEnd));
				if (wordList.Add(word)) {
					if (onlyOneWord && wordList.Count() > 1) {
						return true;
					}
				}
			}
		}
		wEditor.SetTarget(SA::Span(wordEnd, doclen));
		posFind = wEditor.SearchInTarget(root);
	}
	if ((wordList.Count() != 0) && (!onlyOneWord || (wordList.MinWordLength() > static_cast<size_t>(rootLength)))) {
		// Protect spaces by temporarily transforming to \001
		std::string wordsNear = wordList.Get();
		std::replace(wordsNear.begin(), wordsNear.end(), ' ', '\001');
		StringList wl(true);
		wl.Set(wordsNear.c_str());
		std::string acText = wl.GetNearestWords("", 0, autoCompleteIgnoreCase);
		// Use \n as word separator
		std::replace(acText.begin(), acText.end(), ' ', '\n');
		// Return spaces from \001
		std::replace(acText.begin(), acText.end(), '\001', ' ');
		wEditor.AutoCSetSeparator('\n');
		wEditor.AutoCSetMaxHeight(autoCompleteVisibleItemCount);
		wEditor.AutoCShow(rootLength, acText.c_str());
	} else {
		wEditor.AutoCCancel();
	}
	return true;
}

bool SciTEBase::PerformInsertAbbreviation() {
	const std::string data = propsAbbrev.GetString(abbrevInsert.c_str());
	if (data.empty()) {
		return true; // returning if expanded abbreviation is empty
	}

	const std::string expbuf = UnSlashString(data);
	const size_t expbuflen = expbuf.length();

	const SA::Position selStart = wEditor.SelectionStart();
	SA::Position selLength = wEditor.SelectionEnd() - selStart;
	SA::Position caretPos = -1; // caret position
	SA::Line currentLineNumber = wEditor.LineFromPosition(selStart);
	int indent = 0;
	const int indentSize = wEditor.Indent();
	const int indentChars = wEditor.UseTabs() ? wEditor.TabWidth() : 1;
	int indentExtra = 0;
	bool isIndent = true;
	const SA::EndOfLine eolMode = wEditor.EOLMode();

	wEditor.BeginUndoAction();

	// add temporary characters around the selection for correct line indentation
	// if there are tabs or spaces at the beginning or end of the selection
	wEditor.InsertText(selStart, "|");
	selLength++;
	wEditor.InsertText(selStart + selLength, "|");
	if (props.GetInt("indent.automatic")) {
		indent = GetLineIndentation(currentLineNumber);
	}

	wEditor.GotoPos(selStart);

	// add the abbreviation one character at a time
	for (size_t i = 0; i < expbuflen; i++) {
		const char c = expbuf[i];
		if (isIndent && c == '\t') {
			SetLineIndentation(currentLineNumber, GetLineIndentation(currentLineNumber) + indentSize);
			indentExtra += indentSize;
		} else {
			std::string abbrevText;
			switch (c) {
			case '|':
				// user may want to insert '|' instead of caret
				if (i < (expbuflen - 1) && expbuf[i + 1] == '|') {
					// put '|' into the line
					abbrevText += c;
					i++;
				} else if (caretPos == -1) {
					caretPos = wEditor.CurrentPos();

					// indent on multiple lines
					SA::Line j = currentLineNumber + 1; // first line indented as others
					currentLineNumber = wEditor.LineFromPosition(caretPos + selLength);
					for (; j <= currentLineNumber; j++) {
						SetLineIndentation(j, GetLineIndentation(j) + indentExtra);
						selLength += indentExtra / indentChars;
					}

					wEditor.GotoPos(caretPos + selLength);
				}
				break;
			case '\n':
				abbrevText += LineEndString(eolMode);
				break;
			default:
				abbrevText += c;
				break;
			}
			wEditor.ReplaceSel(abbrevText.c_str());
			if (c == '\n') {
				isIndent = true;
				indentExtra = 0;
				currentLineNumber++;
				SetLineIndentation(currentLineNumber, indent);
			} else {
				isIndent = false;
			}
		}
	}

	// make sure the caret is set before the last temporary character and remove it
	if (caretPos == -1) {
		caretPos = wEditor.CurrentPos();
		wEditor.GotoPos(caretPos + selLength);
	}
	wEditor.DeleteRange(wEditor.CurrentPos(), 1);

	// set the caret before the first temporary character and remove it
	wEditor.GotoPos(caretPos);
	wEditor.DeleteRange(wEditor.CurrentPos(), 1);
	selLength--;

	// restore selection
	wEditor.SetSelectionEnd(caretPos + selLength);

	wEditor.EndUndoAction();
	return true;
}

bool SciTEBase::StartExpandAbbreviation() {
	const SA::Position currentPos = GetCaretInLine();
	const SA::Position position = wEditor.CurrentPos(); // from the beginning
	const std::string linebuf(GetCurrentLine(), 0, currentPos);	// Just get text to the left of the caret
	const SA::Position abbrevPos = (currentPos > 32 ? currentPos - 32 : 0);
	const char *abbrev = linebuf.c_str() + abbrevPos;
	std::string data;
	SA::Position abbrevLength = currentPos - abbrevPos;
	// Try each potential abbreviation from the first letter on a line
	// and expanding to the right.
	// We arbitrarily limit the length of an abbreviation (seems a reasonable value..),
	// and of course stop on the caret.
	while (abbrevLength > 0) {
		data = propsAbbrev.GetString(abbrev);
		if (!data.empty()) {
			break;	/* Found */
		}
		abbrev++;	// One more letter to the right
		abbrevLength--;
	}

	if (data.empty()) {
		WarnUser(warnNotFound);	// No need for a special warning
		return true; // returning if expanded abbreviation is empty
	}

	const std::string expbuf = UnSlashString(data);
	const size_t expbuflen = expbuf.length();

	SA::Position caretPos = -1; // caret position
	SA::Line currentLineNumber = GetCurrentLineNumber();
	int indent = 0;
	const int indentSize = wEditor.Indent();
	bool isIndent = true;
	const SA::EndOfLine eolMode = wEditor.EOLMode();

	wEditor.BeginUndoAction();

	// add a temporary character for correct line indentation
	// if there are tabs or spaces after the caret
	wEditor.InsertText(position, "|");
	if (props.GetInt("indent.automatic")) {
		indent = GetLineIndentation(currentLineNumber);
	}

	wEditor.SetSel(position - abbrevLength, position);

	// add the abbreviation one character at a time
	for (size_t i = 0; i < expbuflen; i++) {
		const char c = expbuf[i];
		if (isIndent && c == '\t') {
			SetLineIndentation(currentLineNumber, GetLineIndentation(currentLineNumber) + indentSize);
		} else {
			std::string abbrevText;
			switch (c) {
			case '|':
				// user may want to insert '|' instead of caret
				if (i < (expbuflen - 1) && expbuf[i + 1] == '|') {
					// put '|' into the line
					abbrevText += c;
					i++;
				} else if (caretPos == -1) {
					if (i == 0) {
						// when caret is set at the first place in abbreviation
						caretPos = wEditor.CurrentPos() - abbrevLength;
					} else {
						caretPos = wEditor.CurrentPos();
					}
				}
				break;
			case '\n':
				abbrevText += LineEndString(eolMode);
				break;
			default:
				abbrevText += c;
				break;
			}
			wEditor.ReplaceSel(abbrevText.c_str());
			if (c == '\n') {
				isIndent = true;
				currentLineNumber++;
				SetLineIndentation(currentLineNumber, indent);
			} else {
				isIndent = false;
			}
		}
	}

	// remove the temporary character
	wEditor.DeleteRange(wEditor.CurrentPos(), 1);

	// set the caret to the desired position
	if (caretPos != -1) {
		wEditor.GotoPos(caretPos);
	}

	wEditor.EndUndoAction();
	return true;
}

bool SciTEBase::StartBlockComment() {
	std::string fileNameForExtension = ExtensionFileName();
	std::string lexerName = props.GetNewExpandString("lexer.", fileNameForExtension.c_str());
	std::string base("comment.block.");
	std::string commentAtLineStart("comment.block.at.line.start.");
	base += lexerName;
	commentAtLineStart += lexerName;
	const bool placeCommentsAtLineStart = props.GetInt(commentAtLineStart.c_str()) != 0;

	std::string comment = props.GetString(base.c_str());
	if (comment == "") { // user friendly error message box
		GUI::gui_string sBase = GUI::StringFromUTF8(base);
		GUI::gui_string error = LocaliseMessage(
						"Block comment variable '^0' is not defined in SciTE *.properties!", sBase.c_str());
		WindowMessageBox(wSciTE, error);
		return true;
	}
	const std::string longComment = comment + " ";
	const SA::Position longCommentLength = longComment.length();
	SA::Position selectionStart = wEditor.SelectionStart();
	SA::Position selectionEnd = wEditor.SelectionEnd();
	const SA::Position caretPosition = wEditor.CurrentPos();
	// checking if caret is located in _beginning_ of selected block
	const bool moveCaret = caretPosition < selectionEnd;
	const SA::Line selStartLine = wEditor.LineFromPosition(selectionStart);
	SA::Line selEndLine = wEditor.LineFromPosition(selectionEnd);
	const SA::Line lines = selEndLine - selStartLine;
	const SA::Position firstSelLineStart = wEditor.LineStart(selStartLine);
	// "caret return" is part of the last selected line
	if ((lines > 0) &&
			(selectionEnd == wEditor.LineStart(selEndLine)))
		selEndLine--;
	wEditor.BeginUndoAction();
	for (SA::Line i = selStartLine; i <= selEndLine; i++) {
		const SA::Position lineStart = wEditor.LineStart(i);
		SA::Position lineIndent = lineStart;
		const SA::Position lineEnd = wEditor.LineEnd(i);
		if (!placeCommentsAtLineStart) {
			lineIndent = GetLineIndentPosition(i);
		}
		std::string linebuf = wEditor.StringOfSpan(SA::Span(lineIndent, lineEnd));
		// empty lines are not commented
		if (linebuf.length() < 1)
			continue;
		if (StartsWith(linebuf, comment.c_str())) {
			SA::Position commentLength = comment.length();
			if (StartsWith(linebuf, longComment.c_str())) {
				// Removing comment with space after it.
				commentLength = longCommentLength;
			}
			wEditor.SetSel(lineIndent, lineIndent + commentLength);
			wEditor.ReplaceSel("");
			if (i == selStartLine) // is this the first selected line?
				selectionStart -= commentLength;
			selectionEnd -= commentLength; // every iteration
			continue;
		}
		if (i == selStartLine) // is this the first selected line?
			selectionStart += longCommentLength;
		selectionEnd += longCommentLength; // every iteration
		wEditor.InsertText(lineIndent, longComment.c_str());
	}
	// after uncommenting selection may promote itself to the lines
	// before the first initially selected line;
	// another problem - if only comment symbol was selected;
	if (selectionStart < firstSelLineStart) {
		if (selectionStart >= selectionEnd - (longCommentLength - 1))
			selectionEnd = firstSelLineStart;
		selectionStart = firstSelLineStart;
	}
	if (moveCaret) {
		// moving caret to the beginning of selected block
		wEditor.GotoPos(selectionEnd);
		wEditor.SetCurrentPos(selectionStart);
	} else {
		wEditor.SetSel(selectionStart, selectionEnd);
	}
	wEditor.EndUndoAction();
	return true;
}

const char *LineEndString(SA::EndOfLine eolMode) noexcept {
	switch (eolMode) {
	case SA::EndOfLine::CrLf:
		return "\r\n";
	case SA::EndOfLine::Cr:
		return "\r";
	case SA::EndOfLine::Lf:
	default:
		return "\n";
	}
}

bool SciTEBase::StartBoxComment() {
	// Get start/middle/end comment strings from options file(s)
	std::string fileNameForExtension = ExtensionFileName();
	std::string lexerName = props.GetNewExpandString("lexer.", fileNameForExtension.c_str());
	std::string startBase("comment.box.start.");
	std::string middleBase("comment.box.middle.");
	std::string endBase("comment.box.end.");
	const std::string whiteSpace(" ");
	const std::string eol(LineEndString(wEditor.EOLMode()));
	startBase += lexerName;
	middleBase += lexerName;
	endBase += lexerName;
	std::string startComment = props.GetString(startBase.c_str());
	std::string middleComment = props.GetString(middleBase.c_str());
	std::string endComment = props.GetString(endBase.c_str());
	if (startComment == "" || middleComment == "" || endComment == "") {
		GUI::gui_string sStart = GUI::StringFromUTF8(startBase);
		GUI::gui_string sMiddle = GUI::StringFromUTF8(middleBase);
		GUI::gui_string sEnd = GUI::StringFromUTF8(endBase);
		GUI::gui_string error = LocaliseMessage(
						"Box comment variables '^0', '^1' and '^2' are not defined in SciTE *.properties!",
						sStart.c_str(), sMiddle.c_str(), sEnd.c_str());
		WindowMessageBox(wSciTE, error);
		return true;
	}

	// Note selection and cursor location so that we can reselect text and reposition cursor after we insert comment strings
	SA::Position selectionStart = wEditor.SelectionStart();
	SA::Position selectionEnd = wEditor.SelectionEnd();
	const SA::Position caretPosition = wEditor.CurrentPos();
	const bool moveCaret = caretPosition < selectionEnd;
	const SA::Line selStartLine = wEditor.LineFromPosition(selectionStart);
	SA::Line selEndLine = wEditor.LineFromPosition(selectionEnd);
	SA::Line lines = selEndLine - selStartLine + 1;

	// If selection ends at start of last selected line, fake it so that selection goes to end of second-last selected line
	if (lines > 1 && selectionEnd == wEditor.LineStart(selEndLine)) {
		selEndLine--;
		lines--;
		selectionEnd = wEditor.LineEnd(selEndLine);
	}

	// Pad comment strings with appropriate whitespace, then figure out their lengths (endComment is a bit special-- see below)
	startComment += whiteSpace;
	middleComment += whiteSpace;
	const SA::Position startCommentLength = startComment.length();
	const SA::Position middleCommentLength = middleComment.length();
	const SA::Position endCommentLength = endComment.length();

	wEditor.BeginUndoAction();

	// Insert startComment if needed
	SA::Position lineStart = wEditor.LineStart(selStartLine);
	std::string tempString = wEditor.StringOfSpan(SA::Span(lineStart, lineStart + startCommentLength));
	if (startComment != tempString) {
		wEditor.InsertText(lineStart, startComment.c_str());
		selectionStart += startCommentLength;
		selectionEnd += startCommentLength;
	}

	if (lines <= 1) {
		// Only a single line was selected, so just append whitespace + end-comment at end of line if needed
		const SA::Position lineEnd = wEditor.LineEnd(selEndLine);
		tempString = wEditor.StringOfSpan(SA::Span(lineEnd - endCommentLength, lineEnd));
		if (endComment != tempString) {
			endComment.insert(0, whiteSpace);
			wEditor.InsertText(lineEnd, endComment.c_str());
		}
	} else {
		// More than one line selected, so insert middleComments where needed
		for (SA::Line i = selStartLine + 1; i < selEndLine; i++) {
			lineStart = wEditor.LineStart(i);
			tempString = wEditor.StringOfSpan(SA::Span(lineStart, lineStart + middleCommentLength));
			if (middleComment != tempString) {
				wEditor.InsertText(lineStart, middleComment.c_str());
				selectionEnd += middleCommentLength;
			}
		}

		// If last selected line is not middle-comment or end-comment, we need to insert
		// a middle-comment at the start of last selected line and possibly still insert
		// and end-comment tag after the last line (extra logic is necessary to
		// deal with the case that user selected the end-comment tag)
		lineStart = wEditor.LineStart(selEndLine);
		tempString = wEditor.StringOfSpan(SA::Span(lineStart, lineStart + endCommentLength));
		if (endComment != tempString) {
			tempString = wEditor.StringOfSpan(SA::Span(lineStart, lineStart + middleCommentLength));
			if (middleComment != tempString) {
				wEditor.InsertText(lineStart, middleComment.c_str());
				selectionEnd += middleCommentLength;
			}

			// And since we didn't find the end-comment string yet, we need to check the *next* line
			//  to see if it's necessary to insert an end-comment string and a linefeed there....
			lineStart = wEditor.LineStart(selEndLine + 1);
			tempString = wEditor.StringOfSpan(SA::Span(lineStart, lineStart + endCommentLength));
			if (endComment != tempString) {
				endComment += eol;
				wEditor.InsertText(lineStart, endComment.c_str());
			}
		}
	}

	if (moveCaret) {
		// moving caret to the beginning of selected block
		wEditor.GotoPos(selectionEnd);
		wEditor.SetCurrentPos(selectionStart);
	} else {
		wEditor.SetSel(selectionStart, selectionEnd);
	}

	wEditor.EndUndoAction();

	return true;
}

bool SciTEBase::StartStreamComment() {
	std::string fileNameForExtension = ExtensionFileName();
	const std::string lexerName = props.GetNewExpandString("lexer.", fileNameForExtension.c_str());
	std::string startBase("comment.stream.start.");
	std::string endBase("comment.stream.end.");
	std::string whiteSpace(" ");
	startBase += lexerName;
	endBase += lexerName;
	std::string startComment = props.GetString(startBase.c_str());
	std::string endComment = props.GetString(endBase.c_str());
	if (startComment == "" || endComment == "") {
		GUI::gui_string sStart = GUI::StringFromUTF8(startBase);
		GUI::gui_string sEnd = GUI::StringFromUTF8(endBase);
		GUI::gui_string error = LocaliseMessage(
						"Stream comment variables '^0' and '^1' are not defined in SciTE *.properties!",
						sStart.c_str(), sEnd.c_str());
		WindowMessageBox(wSciTE, error);
		return true;
	}
	startComment += whiteSpace;
	whiteSpace += endComment;
	endComment = whiteSpace;
	const SA::Position startCommentLength = startComment.length();
	SA::Span selection = wEditor.SelectionSpan();
	const SA::Position caretPosition = wEditor.CurrentPos();
	// checking if caret is located in _beginning_ of selected block
	const bool moveCaret = caretPosition < selection.end;
	// if there is no selection?
	if (selection.start == selection.end) {
		RangeExtend(wEditor, selection,
			    &SciTEBase::islexerwordcharforsel);
		if (selection.start == selection.end)
			return true; // caret is located _between_ words
	}
	wEditor.BeginUndoAction();
	wEditor.InsertText(selection.start, startComment.c_str());
	selection.end += startCommentLength;
	selection.start += startCommentLength;
	wEditor.InsertText(selection.end, endComment.c_str());
	if (moveCaret) {
		// moving caret to the beginning of selected block
		wEditor.GotoPos(selection.end);
		wEditor.SetCurrentPos(selection.start);
	} else {
		wEditor.SetSel(selection.start, selection.end);
	}
	wEditor.EndUndoAction();
	return true;
}

/**
 * Return the length of the given line, not counting the EOL.
 */
SA::Position SciTEBase::GetLineLength(SA::Line line) {
	return wEditor.LineEnd(line) - wEditor.LineStart(line);
}

SA::Line SciTEBase::GetCurrentLineNumber() {
	return wEditor.LineFromPosition(
		       wEditor.CurrentPos());
}

SA::Position SciTEBase::GetCurrentColumnNumber() {
	const int mainSel = wEditor.MainSelection();
	return wEditor.Column(wEditor.SelectionNCaret(mainSel)) +
	       wEditor.SelectionNCaretVirtualSpace(mainSel);
}

SA::Line SciTEBase::GetCurrentScrollPosition() {
	const SA::Line lineDisplayTop = wEditor.FirstVisibleLine();
	return wEditor.DocLineFromVisible(lineDisplayTop);
}

/**
 * Set up properties for ReadOnly, EOLMode, BufferLength, NbOfLines, SelLength, SelHeight.
 */
void SciTEBase::SetTextProperties(
	PropSetFile &ps) {			///< Property set to update.

	std::string ro = GUI::UTF8FromString(localiser.Text("READ"));
	ps.Set("ReadOnly", CurrentBuffer()->isReadOnly ? ro.c_str() : "");

	const SA::EndOfLine eolMode = wEditor.EOLMode();
	ps.Set("EOLMode", eolMode == SA::EndOfLine::CrLf ? "CR+LF" : (eolMode == SA::EndOfLine::Lf ? "LF" : "CR"));

	ps.Set("BufferLength", std::to_string(LengthDocument()));

	ps.Set("NbOfLines", std::to_string(wEditor.LineCount()));

	const SA::Span range = wEditor.SelectionSpan();
	const SA::Line selFirstLine = wEditor.LineFromPosition(range.start);
	const SA::Line selLastLine = wEditor.LineFromPosition(range.end);
	SA::Position charCount = 0;
	if (wEditor.SelectionMode() == SA::SelectionMode::Rectangle) {
		for (SA::Line line = selFirstLine; line <= selLastLine; line++) {
			const SA::Position startPos = wEditor.GetLineSelStartPosition(line);
			const SA::Position endPos = wEditor.GetLineSelEndPosition(line);
			charCount += wEditor.CountCharacters(startPos, endPos);
		}
	} else {
		charCount = wEditor.CountCharacters(range.start, range.end);
	}
	ps.Set("SelLength", std::to_string(charCount));
	const SA::Position caretPos = wEditor.CurrentPos();
	const SA::Position selAnchor = wEditor.Anchor();
	SA::Line selHeight = selLastLine - selFirstLine + 1;
	if (0 == range.Length()) {
		selHeight = 0;
	} else if (selLastLine == selFirstLine) {
		selHeight = 1;
	} else if ((wEditor.Column(caretPos) == 0 && (selAnchor <= caretPos)) ||
			((wEditor.Column(selAnchor) == 0) && (selAnchor > caretPos))) {
		selHeight = selLastLine - selFirstLine;
	}
	ps.Set("SelHeight", std::to_string(selHeight));
}

void SciTEBase::UpdateStatusBar(bool bUpdateSlowData) {
	if (sbVisible) {
		if (bUpdateSlowData) {
			SetFileProperties(propsStatus);
		}
		SetTextProperties(propsStatus);
		propsStatus.Set("LineNumber", std::to_string(GetCurrentLineNumber() + 1));
		propsStatus.Set("ColumnNumber", std::to_string(GetCurrentColumnNumber() + 1));
		propsStatus.Set("OverType", wEditor.Overtype() ? "OVR" : "INS");

		char sbKey[32];
		sprintf(sbKey, "statusbar.text.%d", sbNum);
		std::string msg = propsStatus.GetExpandedString(sbKey);
		if (msg.size() && sbValue != msg) {	// To avoid flickering, update only if needed
			SetStatusBarText(msg.c_str());
			sbValue = msg;
		}
	} else {
		sbValue = "";
	}
}

void SciTEBase::SetLineIndentation(SA::Line line, int indent) {
	if (indent < 0)
		return;
	const SA::Span rangeStart = GetSelection();
	SA::Span range = rangeStart;
	const SA::Position posBefore = GetLineIndentPosition(line);
	wEditor.SetLineIndentation(line, indent);
	const SA::Position posAfter = GetLineIndentPosition(line);
	const SA::Position posDifference = posAfter - posBefore;
	if (posAfter > posBefore) {
		// Move selection on
		if (range.start >= posBefore) {
			range.start += posDifference;
		}
		if (range.end >= posBefore) {
			range.end += posDifference;
		}
	} else if (posAfter < posBefore) {
		// Move selection back
		if (range.start >= posAfter) {
			if (range.start >= posBefore)
				range.start += posDifference;
			else
				range.start = posAfter;
		}
		if (range.end >= posAfter) {
			if (range.end >= posBefore)
				range.end += posDifference;
			else
				range.end = posAfter;
		}
	}
	if (!(rangeStart == range)) {
		SetSelection(range.start, range.end);
	}
}

int SciTEBase::GetLineIndentation(SA::Line line) {
	return wEditor.LineIndentation(line);
}

SA::Position SciTEBase::GetLineIndentPosition(SA::Line line) {
	return wEditor.LineIndentPosition(line);
}

static std::string CreateIndentation(int indent, int tabSize, bool insertSpaces) {
	std::string indentation;
	if (!insertSpaces) {
		while (indent >= tabSize) {
			indentation.append("\t", 1);
			indent -= tabSize;
		}
	}
	while (indent > 0) {
		indentation.append(" ", 1);
		indent--;
	}
	return indentation;
}

void SciTEBase::ConvertIndentation(int tabSize, int useTabs) {
	wEditor.BeginUndoAction();
	const SA::Line maxLine = wEditor.LineCount();
	for (SA::Line line = 0; line < maxLine; line++) {
		const SA::Position lineStart = wEditor.LineStart(line);
		const int indent = GetLineIndentation(line);
		const SA::Position indentPos = GetLineIndentPosition(line);
		constexpr int maxIndentation = 1000;
		if (indent < maxIndentation) {
			std::string indentationNow = wEditor.StringOfSpan(SA::Span(lineStart, indentPos));
			std::string indentationWanted = CreateIndentation(indent, tabSize, !useTabs);
			if (indentationNow != indentationWanted) {
				wEditor.SetTarget(SA::Span(lineStart, indentPos));
				wEditor.ReplaceTarget(indentationWanted);
			}
		}
	}
	wEditor.EndUndoAction();
}

bool SciTEBase::RangeIsAllWhitespace(SA::Position start, SA::Position end) {
	TextReader acc(wEditor);
	for (SA::Position i = start; i < end; i++) {
		if ((acc[i] != ' ') && (acc[i] != '\t'))
			return false;
	}
	return true;
}

std::vector<std::string> SciTEBase::GetLinePartsInStyle(SA::Line line, const StyleAndWords &saw) {
	std::vector<std::string> sv;
	TextReader acc(wEditor);
	std::string s;
	const bool separateCharacters = saw.IsSingleChar();
	const SA::Position thisLineStart = wEditor.LineStart(line);
	const SA::Position nextLineStart = wEditor.LineStart(line + 1);
	for (SA::Position pos = thisLineStart; pos < nextLineStart; pos++) {
		if (acc.StyleAt(pos) == saw.styleNumber) {
			if (separateCharacters) {
				// Add one character at a time, even if there is an adjacent character in the same style
				if (s.length() > 0) {
					sv.push_back(s);
				}
				s = "";
			}
			s += acc[pos];
		} else if (s.length() > 0) {
			sv.push_back(s);
			s = "";
		}
	}
	if (s.length() > 0) {
		sv.push_back(s);
	}
	return sv;
}

static bool includes(const StyleAndWords &symbols, const std::string &value) {
	if (symbols.words.length() == 0) {
		return false;
	} else if (IsAlphabetic(symbols.words[0])) {
		// Set of symbols separated by spaces
		const size_t lenVal = value.length();
		const char *symbol = symbols.words.c_str();
		while (symbol) {
			const char *symbolEnd = strchr(symbol, ' ');
			size_t lenSymbol = strlen(symbol);
			if (symbolEnd)
				lenSymbol = symbolEnd - symbol;
			if (lenSymbol == lenVal) {
				if (strncmp(symbol, value.c_str(), lenSymbol) == 0) {
					return true;
				}
			}
			symbol = symbolEnd;
			if (symbol)
				symbol++;
		}
	} else {
		// Set of individual characters. Only one character allowed for now
		const char ch = symbols.words[0];
		return strchr(value.c_str(), ch) != nullptr;
	}
	return false;
}

IndentationStatus SciTEBase::GetIndentState(SA::Line line) {
	// C like language indentation defined by braces and keywords
	IndentationStatus indentState = IndentationStatus::none;
	const std::vector<std::string> controlIndents = GetLinePartsInStyle(line, statementIndent);
	for (const std::string &sIndent : controlIndents) {
		if (includes(statementIndent, sIndent))
			indentState = IndentationStatus::keyWordStart;
	}
	const std::vector<std::string> controlEnds = GetLinePartsInStyle(line, statementEnd);
	for (const std::string &sEnd : controlEnds) {
		if (includes(statementEnd, sEnd))
			indentState = IndentationStatus::none;
	}
	// Braces override keywords
	const std::vector<std::string> controlBlocks = GetLinePartsInStyle(line, blockEnd);
	for (const std::string &sBlock : controlBlocks) {
		if (includes(blockEnd, sBlock))
			indentState = IndentationStatus::blockEnd;
		if (includes(blockStart, sBlock))
			indentState = IndentationStatus::blockStart;
	}
	return indentState;
}

int SciTEBase::IndentOfBlock(SA::Line line) {
	if (line < 0)
		return 0;
	const int indentSize = wEditor.Indent();
	int indentBlock = GetLineIndentation(line);
	SA::Line backLine = line;
	IndentationStatus indentState = IndentationStatus::none;
	if (statementIndent.IsEmpty() && blockStart.IsEmpty() && blockEnd.IsEmpty())
		indentState = IndentationStatus::blockStart;	// Don't bother searching backwards

	SA::Line lineLimit = line - statementLookback;
	if (lineLimit < 0)
		lineLimit = 0;
	while ((backLine >= lineLimit) && (indentState == IndentationStatus::none)) {
		indentState = GetIndentState(backLine);
		if (indentState != IndentationStatus::none) {
			indentBlock = GetLineIndentation(backLine);
			if (indentState == IndentationStatus::blockStart) {
				if (!indentOpening)
					indentBlock += indentSize;
			}
			if (indentState == IndentationStatus::blockEnd) {
				if (indentClosing)
					indentBlock -= indentSize;
				if (indentBlock < 0)
					indentBlock = 0;
			}
			if ((indentState == IndentationStatus::keyWordStart) && (backLine == line))
				indentBlock += indentSize;
		}
		backLine--;
	}
	return indentBlock;
}

void SciTEBase::MaintainIndentation(char ch) {
	const SA::EndOfLine eolMode = wEditor.EOLMode();
	const SA::Line curLine = GetCurrentLineNumber();
	SA::Line lastLine = curLine - 1;

	if (((eolMode == SA::EndOfLine::CrLf || eolMode == SA::EndOfLine::Lf) && ch == '\n') ||
			(eolMode == SA::EndOfLine::Cr && ch == '\r')) {
		if (props.GetInt("indent.automatic")) {
			while (lastLine >= 0 && GetLineLength(lastLine) == 0)
				lastLine--;
		}
		int indentAmount = 0;
		if (lastLine >= 0) {
			indentAmount = GetLineIndentation(lastLine);
		}
		if (indentAmount > 0) {
			SetLineIndentation(curLine, indentAmount);
		}
	}
}

void SciTEBase::AutomaticIndentation(char ch) {
	const SA::Span range = wEditor.SelectionSpan();
	const SA::Position selStart = range.start;
	const SA::Line curLine = GetCurrentLineNumber();
	const SA::Position thisLineStart = wEditor.LineStart(curLine);
	const int indentSize = wEditor.Indent();
	int indentBlock = IndentOfBlock(curLine - 1);

	if ((wEditor.Lexer() == SCLEX_PYTHON) &&
			(props.GetInt("indent.python.colon") == 1)) {
		const SA::EndOfLine eolMode = wEditor.EOLMode();
		const int eolChar = (eolMode == SA::EndOfLine::Cr ? '\r' : '\n');
		const int eolChars = (eolMode == SA::EndOfLine::CrLf ? 2 : 1);
		const SA::Position prevLineStart = wEditor.LineStart(curLine - 1);
		const SA::Position prevIndentPos = GetLineIndentPosition(curLine - 1);
		const int indentExisting = GetLineIndentation(curLine);

		if (ch == eolChar) {
			// Find last noncomment, nonwhitespace character on previous line
			char character = '\0';
			int style = 0;
			for (SA::Position p = selStart - eolChars - 1; p > prevLineStart; p--) {
				style = wEditor.UnsignedStyleAt(p);
				if (style != SCE_P_DEFAULT && style != SCE_P_COMMENTLINE &&
						style != SCE_P_COMMENTBLOCK) {
					character = wEditor.CharacterAt(p);
					break;
				}
			}
			indentBlock = GetLineIndentation(curLine - 1);
			if (style == SCE_P_OPERATOR && character == ':') {
				SetLineIndentation(curLine, indentBlock + indentSize);
			} else if (selStart == prevIndentPos + eolChars) {
				// Preserve the indentation of preexisting text beyond the caret
				SetLineIndentation(curLine, indentBlock + indentExisting);
			} else {
				SetLineIndentation(curLine, indentBlock);
			}
		}
		return;
	}

	if (blockEnd.IsSingleChar() && ch == blockEnd.words[0]) {	// Dedent maybe
		if (!indentClosing) {
			if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) {
				SetLineIndentation(curLine, indentBlock - indentSize);
			}
		}
	} else if (!blockEnd.IsSingleChar() && (ch == ' ')) {	// Dedent maybe
		if (!indentClosing && (GetIndentState(curLine) == IndentationStatus::blockEnd)) {}
	} else if (blockStart.IsSingleChar() && (ch == blockStart.words[0])) {
		// Dedent maybe if first on line and previous line was starting keyword
		if (!indentOpening && (GetIndentState(curLine - 1) == IndentationStatus::keyWordStart)) {
			if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) {
				SetLineIndentation(curLine, indentBlock - indentSize);
			}
		}
	} else if ((ch == '\r' || ch == '\n') && (selStart == thisLineStart)) {
		if (!indentClosing && !blockEnd.IsSingleChar()) {	// Dedent previous line maybe
			const std::vector<std::string> controlWords = GetLinePartsInStyle(curLine - 1, blockEnd);
			if (!controlWords.empty()) {
				if (includes(blockEnd, controlWords[0])) {
					// Check if first keyword on line is an ender
					SetLineIndentation(curLine - 1, IndentOfBlock(curLine - 2) - indentSize);
					// Recalculate as may have changed previous line
					indentBlock = IndentOfBlock(curLine - 1);
				}
			}
		}
		SetLineIndentation(curLine, indentBlock);
	}
}

/**
 * Upon a character being added, SciTE may decide to perform some action
 * such as displaying a completion list or auto-indentation.
 */
void SciTEBase::CharAdded(int utf32) {
	if (recording)
		return;
	const SA::Span rangeSelection = GetSelection();
	const SA::Position selStart = rangeSelection.start;
	const SA::Position selEnd = rangeSelection.end;

	if (utf32 > 0XFF) { // MBCS, never let it go.
		if (imeAutoComplete) {
			if ((selEnd == selStart) && (selStart > 0)) {
				if (wEditor.CallTipActive()) {
					ContinueCallTip();
				} else if (wEditor.AutoCActive()) {
					wEditor.AutoCCancel();
					StartAutoComplete();
				} else {
					StartAutoComplete();
				}
			}
		}
		return;
	}

	// SBCS
	const char ch = static_cast<char>(utf32);
	if ((selEnd == selStart) && (selStart > 0)) {
		if (wEditor.CallTipActive()) {
			if (Contains(calltipParametersEnd, ch)) {
				braceCount--;
				if (braceCount < 1)
					wEditor.CallTipCancel();
				else
					StartCallTip();
			} else if (Contains(calltipParametersStart, ch)) {
				braceCount++;
				StartCallTip();
			} else {
				ContinueCallTip();
			}
		} else if (wEditor.AutoCActive()) {
			if (Contains(calltipParametersStart, ch)) {
				braceCount++;
				StartCallTip();
			} else if (Contains(calltipParametersEnd, ch)) {
				braceCount--;
			} else if (!Contains(wordCharacters, ch)) {
				wEditor.AutoCCancel();
				if (Contains(autoCompleteStartCharacters, ch)) {
					StartAutoComplete();
				}
			} else if (autoCCausedByOnlyOne) {
				StartAutoCompleteWord(true);
			}
		} else if (HandleXml(ch)) {
			// Handled in the routine
		} else {
			if (Contains(calltipParametersStart, ch)) {
				braceCount = 1;
				StartCallTip();
			} else {
				autoCCausedByOnlyOne = false;
				if (indentMaintain)
					MaintainIndentation(ch);
				else if (props.GetInt("indent.automatic"))
					AutomaticIndentation(ch);
				if (Contains(autoCompleteStartCharacters, ch)) {
					StartAutoComplete();
				} else if (props.GetInt("autocompleteword.automatic") && Contains(wordCharacters, ch)) {
					StartAutoCompleteWord(true);
					autoCCausedByOnlyOne = wEditor.AutoCActive();
				}
			}
		}
	}
}

namespace {

void AddProps(std::string &symbols, const PropSetFile &propSet) {
	const char *key = nullptr;
	const char *val = nullptr;
	bool b = propSet.GetFirst(key, val);
	while (b) {
		if (IsUpperCase(*key)) {
			symbols.append(key);
			symbols.append(") ");
		}
		b = propSet.GetNext(key, val);
	}
}

}

/**
 * Upon a character being added to the output, SciTE may decide to perform some action
 * such as displaying a completion list or running a shell command.
 */
void SciTEBase::CharAddedOutput(int ch) {
	if (ch == '\n') {
		NewLineInOutput();
	} else if (ch == '(') {
		// Potential autocompletion of symbols when $( typed
		const SA::Position selStart = wOutput.SelectionStart();
		if ((selStart > 1) && (wOutput.CharacterAt(selStart - 2) == '$')) {
			std::string symbols;
			AddProps(symbols, props);
			AddProps(symbols, propsDirectory);
			StringList symList;
			symList.Set(symbols.c_str());
			std::string words = symList.GetNearestWords("", 0, true);
			if (words.length()) {
				wOutput.AutoCSetSeparator(' ');
				wOutput.AutoCSetMaxHeight(autoCompleteVisibleItemCount);
				wOutput.AutoCShow(0, words.c_str());
			}
		}
	}
}

/**
 * This routine will auto complete XML or HTML tags that are still open by closing them
 * @param ch The character we are dealing with, currently only works with the '>' character
 * @return True if handled, false otherwise
 */
bool SciTEBase::HandleXml(char ch) {
	// We're looking for this char
	// Quit quickly if not found
	if (ch != '>') {
		return false;
	}

	// This may make sense only in certain languages
	if (lexLanguage != SCLEX_HTML && lexLanguage != SCLEX_XML) {
		return false;
	}

	// If the user has turned us off, quit now.
	// Default is off
	const std::string value = props.GetExpandedString("xml.auto.close.tags");
	if ((value.length() == 0) || (value == "0")) {
		return false;
	}

	// Grab the last 512 characters or so
	const SA::Position nCaret = wEditor.CurrentPos();
	SA::Position nMin = nCaret - 512;
	if (nMin < 0) {
		nMin = 0;
	}

	if (nCaret - nMin < 3) {
		return false; // Smallest tag is 3 characters ex. <p>
	}
	std::string sel = wEditor.StringOfSpan(SA::Span(nMin, nCaret));

	if (sel[nCaret - nMin - 2] == '/') {
		// User typed something like "<br/>"
		return false;
	}

	if (sel[nCaret - nMin - 2] == '-') {
		// User typed something like "<a $this->"
		return false;
	}

	std::string strFound = FindOpenXmlTag(sel.c_str(), nCaret - nMin);

	if (strFound.length() > 0) {
		wEditor.BeginUndoAction();
		std::string toInsert = "</";
		toInsert += strFound;
		toInsert += ">";
		wEditor.ReplaceSel(toInsert.c_str());
		SetSelection(nCaret, nCaret);
		wEditor.EndUndoAction();
		return true;
	}

	return false;
}

/** Search backward through nSize bytes looking for a '<', then return the tag if any
 * @return The tag name
 */
std::string SciTEBase::FindOpenXmlTag(const char sel[], SA::Position nSize) {
	std::string strRet;

	if (nSize < 3) {
		// Smallest tag is "<p>" which is 3 characters
		return strRet;
	}
	const char *pBegin = &sel[0];
	const char *pCur = &sel[nSize - 1];

	pCur--; // Skip past the >
	while (pCur > pBegin) {
		if (*pCur == '<') {
			break;
		} else if (*pCur == '>') {
			if (*(pCur - 1) != '-') {
				break;
			}
		}
		--pCur;
	}

	if (*pCur == '<') {
		pCur++;
		while (strchr(":_-.", *pCur) || IsAlphaNumeric(*pCur)) {
			strRet += *pCur;
			pCur++;
		}
	}

	// Return the tag name or ""
	return strRet;
}

void SciTEBase::GoMatchingBrace(bool select) {
	SA::Position braceAtCaret = -1;
	SA::Position braceOpposite = -1;
	const bool isInside = FindMatchingBracePosition(pwFocussed == &wEditor, braceAtCaret, braceOpposite, true);
	// Convert the character positions into caret positions based on whether
	// the caret position was inside or outside the braces.
	if (isInside) {
		if (braceOpposite > braceAtCaret) {
			braceAtCaret++;
		} else if (braceOpposite >= 0) {
			braceOpposite++;
		}
	} else {    // Outside
		if (braceOpposite > braceAtCaret) {
			braceOpposite++;
		} else {
			braceAtCaret++;
		}
	}
	if (braceOpposite >= 0) {
		EnsureRangeVisible(*pwFocussed, SA::Span(braceOpposite));
		if (select) {
			pwFocussed->SetSel(braceAtCaret, braceOpposite);
		} else {
			pwFocussed->SetSel(braceOpposite, braceOpposite);
		}
	}
}

// Text	ConditionalUp	Ctrl+J	Finds the previous matching preprocessor condition
// Text	ConditionalDown	Ctrl+K	Finds the next matching preprocessor condition
void SciTEBase::GoMatchingPreprocCond(int direction, bool select) {
	const SA::Position mppcAtCaret = wEditor.CurrentPos();
	SA::Position mppcMatch = -1;
	const int forward = (direction == IDM_NEXTMATCHPPC);
	const bool isInside = FindMatchingPreprocCondPosition(forward, mppcAtCaret, mppcMatch);

	if (isInside && mppcMatch >= 0) {
		EnsureRangeVisible(wEditor, SA::Span(mppcMatch));
		if (select) {
			// Selection changes the rules a bit...
			const SA::Position selStart = wEditor.SelectionStart();
			const SA::Position selEnd = wEditor.SelectionEnd();
			// pivot isn't the caret position but the opposite (if there is a selection)
			const SA::Position pivot = (mppcAtCaret == selStart ? selEnd : selStart);
			if (forward) {
				// Caret goes one line beyond the target, to allow selecting the whole line
				const SA::Line lineNb = wEditor.LineFromPosition(mppcMatch);
				mppcMatch = wEditor.LineStart(lineNb + 1);
			}
			SetSelection(pivot, mppcMatch);
		} else {
			SetSelection(mppcMatch, mppcMatch);
		}
	} else {
		WarnUser(warnNotFound);
	}
}

void SciTEBase::AddCommand(const std::string &cmd, const std::string &dir, JobSubsystem jobType, const std::string &input, int flags) {
	// If no explicit directory, use the directory of the current file
	FilePath directoryRun;
	if (dir.length()) {
		FilePath directoryExplicit(GUI::StringFromUTF8(dir));
		if (directoryExplicit.IsAbsolute()) {
			directoryRun = directoryExplicit;
		} else {
			// Relative paths are relative to the current file
			directoryRun = FilePath(filePath.Directory(), directoryExplicit).NormalizePath();
		}
	} else {
		directoryRun = filePath.Directory();
	}
	jobQueue.AddCommand(cmd, directoryRun, jobType, input, flags);
}

int ControlIDOfCommand(unsigned long wParam) noexcept {
	return wParam & 0xffff;
}

void WindowSetFocus(GUI::ScintillaWindow &w) {
	w.Send(SCI_GRABFOCUS);
}

void SciTEBase::SetFoldWidth() {
	wEditor.SetMarginWidthN(2, (foldMargin && !FilterShowing()) ? foldMarginWidth : 0);
}

void SciTEBase::SetLineNumberWidth() {
	if (lineNumbers) {
		int lineNumWidth = lineNumbersWidth;

		if (lineNumbersExpand) {
			// The margin size will be expanded if the current buffer's maximum
			// line number would overflow the margin.

			SA::Line lineCount = wEditor.LineCount();

			lineNumWidth = 1;
			while (lineCount >= 10) {
				lineCount /= 10;
				++lineNumWidth;
			}

			if (lineNumWidth < lineNumbersWidth) {
				lineNumWidth = lineNumbersWidth;
			}
		}
		if (lineNumWidth < 0)
			lineNumWidth = 0;
		// The 4 here allows for spacing: 1 pixel on left and 3 on right.
		std::string nNines(lineNumWidth, '9');
		const int pixelWidth = 4 + wEditor.TextWidth(
					       static_cast<int>(SA::StylesCommon::LineNumber), nNines.c_str());

		wEditor.SetMarginWidthN(0, pixelWidth);
	} else {
		wEditor.SetMarginWidthN(0, 0);
	}
}

void SciTEBase::MenuCommand(int cmdID, int source) {
	switch (cmdID) {
	case IDM_NEW:
		// For the New command, the "are you sure" question is always asked as this gives
		// an opportunity to abandon the edits made to a file when are.you.sure is turned off.
		if (CanMakeRoom()) {
			New();
			ReadProperties();
			SetIndentSettings();
			SetEol();
			UpdateStatusBar(true);
			WindowSetFocus(wEditor);
		}
		break;
	case IDM_OPEN:
		// No need to see if can make room as that will occur
		// when doing the opening. Must be done there as user
		// may decide to open multiple files so do not know yet
		// how much room needed.
		OpenDialog(filePath.Directory(), GUI::StringFromUTF8(props.GetExpandedString("open.filter")).c_str());
		WindowSetFocus(wEditor);
		break;
	case IDM_OPENSELECTED:
		if (OpenSelected())
			WindowSetFocus(wEditor);
		break;
	case IDM_REVERT:
		Revert();
		WindowSetFocus(wEditor);
		break;
	case IDM_CLOSE:
		if (SaveIfUnsure() != SaveResult::cancelled) {
			Close();
			WindowSetFocus(wEditor);
		}
		break;
	case IDM_CLOSEALL:
		CloseAllBuffers();
		break;
	case IDM_SAVE:
		Save();
		WindowSetFocus(wEditor);
		break;
	case IDM_SAVEALL:
		SaveAllBuffers(true);
		break;
	case IDM_SAVEAS:
		SaveAsDialog();
		WindowSetFocus(wEditor);
		break;
	case IDM_SAVEACOPY:
		SaveACopy();
		WindowSetFocus(wEditor);
		break;
	case IDM_COPYPATH:
		CopyPath();
		break;
	case IDM_SAVEASHTML:
		SaveAsHTML();
		WindowSetFocus(wEditor);
		break;
	case IDM_SAVEASRTF:
		SaveAsRTF();
		WindowSetFocus(wEditor);
		break;
	case IDM_SAVEASPDF:
		SaveAsPDF();
		WindowSetFocus(wEditor);
		break;
	case IDM_SAVEASTEX:
		SaveAsTEX();
		WindowSetFocus(wEditor);
		break;
	case IDM_SAVEASXML:
		SaveAsXML();
		WindowSetFocus(wEditor);
		break;
	case IDM_PRINT:
		Print(true);
		break;
	case IDM_PRINTSETUP:
		PrintSetup();
		break;
	case IDM_LOADSESSION:
		LoadSessionDialog();
		WindowSetFocus(wEditor);
		break;
	case IDM_SAVESESSION:
		SaveSessionDialog();
		WindowSetFocus(wEditor);
		break;
	case IDM_ABOUT:
		AboutDialog();
		break;
	case IDM_QUIT:
		QuitProgram();
		break;
	case IDM_ENCODING_DEFAULT:
	case IDM_ENCODING_UCS2BE:
	case IDM_ENCODING_UCS2LE:
	case IDM_ENCODING_UTF8:
	case IDM_ENCODING_UCOOKIE:
		CurrentBuffer()->unicodeMode = static_cast<UniMode>(cmdID - IDM_ENCODING_DEFAULT);
		if (CurrentBuffer()->unicodeMode != UniMode::uni8Bit) {
			// Override the code page if Unicode
			codePage = SA::CpUtf8;
		} else {
			codePage = props.GetInt("code.page");
		}
		wEditor.SetCodePage(codePage);
		break;

	case IDM_NEXTFILESTACK:
		if (buffers.size() > 1 && props.GetInt("buffers.zorder.switching")) {
			NextInStack(); // next most recently selected buffer
			WindowSetFocus(wEditor);
			break;
		}
		[[fallthrough]];
	// else fall through and do NEXTFILE behaviour...
	case IDM_NEXTFILE:
		if (buffers.size() > 1) {
			Next(); // Use Next to tabs move left-to-right
			WindowSetFocus(wEditor);
		} else {
			// Not using buffers - switch to next file on MRU
			StackMenuNext();
		}
		break;

	case IDM_PREVFILESTACK:
		if (buffers.size() > 1 && props.GetInt("buffers.zorder.switching")) {
			PrevInStack(); // next least recently selected buffer
			WindowSetFocus(wEditor);
			break;
		}
		[[fallthrough]];
	// else fall through and do PREVFILE behaviour...
	case IDM_PREVFILE:
		if (buffers.size() > 1) {
			Prev(); // Use Prev to tabs move right-to-left
			WindowSetFocus(wEditor);
		} else {
			// Not using buffers - switch to previous file on MRU
			StackMenuPrev();
		}
		break;

	case IDM_MOVETABRIGHT:
		MoveTabRight();
		WindowSetFocus(wEditor);
		break;
	case IDM_MOVETABLEFT:
		MoveTabLeft();
		WindowSetFocus(wEditor);
		break;

	case IDM_UNDO:
		PaneSource(source).Undo();
		CheckMenus();
		break;
	case IDM_REDO:
		PaneSource(source).Redo();
		CheckMenus();
		break;

	case IDM_CUT:
		if (!PaneSource(source).SelectionEmpty()) {
			PaneSource(source).Cut();
		}
		break;
	case IDM_COPY:
		if (!PaneSource(source).SelectionEmpty()) {
			//fprintf(stderr, "Copy from %d\n", source);
			PaneSource(source).Copy();
		}
		// does not trigger Notification::UpdateUI, so do CheckMenusClipboard() here
		CheckMenusClipboard();
		break;
	case IDM_PASTE:
		PaneSource(source).Paste();
		break;
	case IDM_DUPLICATE:
		PaneSource(source).SelectionDuplicate();
		break;
	case IDM_PASTEANDDOWN: {
			const SA::Position pos = PaneFocused().CurrentPos();
			PaneFocused().Paste();
			PaneFocused().SetCurrentPos(pos);
			PaneFocused().CharLeft();
			PaneFocused().LineDown();
		}
		break;
	case IDM_CLEAR:
		PaneSource(source).Clear();
		break;
	case IDM_SELECTALL:
		PaneSource(source).SelectAll();
		break;
	case IDM_COPYASRTF:
		CopyAsRTF();
		break;

	case IDM_FIND:
		Find();
		break;

	case IDM_INCSEARCH:
		IncrementSearchMode();
		break;

	case IDM_FILTER:
		FilterSearch();
		break;

	case IDM_FINDNEXT:
		FindNext(reverseFind);
		break;

	case IDM_FINDNEXTBACK:
		FindNext(!reverseFind);
		break;

	case IDM_FINDNEXTSEL:
		SelectionIntoFind();
		FindNext(reverseFind, true, false);
		break;

	case IDM_ENTERSELECTION:
		SelectionIntoFind();
		break;

	case IDM_SELECTIONADDNEXT:
		SelectionAdd(AddSelection::next);
		break;

	case IDM_SELECTIONADDEACH:
		SelectionAdd(AddSelection::each);
		break;

	case IDM_FINDNEXTBACKSEL:
		SelectionIntoFind();
		FindNext(!reverseFind, true, false);
		break;

	case IDM_FINDINFILES:
		FindInFiles();
		break;

	case IDM_REPLACE:
		Replace();
		break;

	case IDM_GOTO:
		GoLineDialog();
		break;

	case IDM_MATCHBRACE:
		GoMatchingBrace(false);
		break;

	case IDM_SELECTTOBRACE:
		GoMatchingBrace(true);
		break;

	case IDM_PREVMATCHPPC:
		GoMatchingPreprocCond(IDM_PREVMATCHPPC, false);
		break;

	case IDM_SELECTTOPREVMATCHPPC:
		GoMatchingPreprocCond(IDM_PREVMATCHPPC, true);
		break;

	case IDM_NEXTMATCHPPC:
		GoMatchingPreprocCond(IDM_NEXTMATCHPPC, false);
		break;

	case IDM_SELECTTONEXTMATCHPPC:
		GoMatchingPreprocCond(IDM_NEXTMATCHPPC, true);
		break;
	case IDM_SHOWCALLTIP:
		if (wEditor.CallTipActive()) {
			currentCallTip = (currentCallTip + 1 == maxCallTips) ? 0 : currentCallTip + 1;
			FillFunctionDefinition();
		} else {
			StartCallTip();
		}
		break;
	case IDM_COMPLETE:
		autoCCausedByOnlyOne = false;
		StartAutoComplete();
		break;

	case IDM_COMPLETEWORD:
		autoCCausedByOnlyOne = false;
		StartAutoCompleteWord(false);
		break;

	case IDM_ABBREV:
		wEditor.Cancel();
		StartExpandAbbreviation();
		break;

	case IDM_INS_ABBREV:
		wEditor.Cancel();
		AbbrevDialog();
		break;

	case IDM_BLOCK_COMMENT:
		StartBlockComment();
		break;

	case IDM_BOX_COMMENT:
		StartBoxComment();
		break;

	case IDM_STREAM_COMMENT:
		StartStreamComment();
		break;

	case IDM_TOGGLE_FOLDALL:
		FoldAll();
		break;

	case IDM_UPRCASE:
		PaneFocused().UpperCase();
		break;

	case IDM_LWRCASE:
		PaneFocused().LowerCase();
		break;

	case IDM_LINEREVERSE:
		PaneFocused().LineReverse();
		break;

	case IDM_JOIN:
		PaneFocused().TargetFromSelection();
		PaneFocused().LinesJoin();
		break;

	case IDM_SPLIT:
		PaneFocused().TargetFromSelection();
		PaneFocused().LinesSplit(0);
		break;

	case IDM_EXPAND:
		wEditor.ToggleFold(GetCurrentLineNumber());
		break;

	case IDM_TOGGLE_FOLDRECURSIVE: {
			const SA::Line line = GetCurrentLineNumber();
			const SA::FoldLevel level = wEditor.FoldLevel(line);
			ToggleFoldRecursive(line, level);
		}
		break;

	case IDM_EXPAND_ENSURECHILDRENVISIBLE: {
			const SA::Line line = GetCurrentLineNumber();
			const SA::FoldLevel level = wEditor.FoldLevel(line);
			EnsureAllChildrenVisible(line, level);
		}
		break;

	case IDM_SPLITVERTICAL: {
			const GUI::Rectangle rcClient = GetClientRectangle();
			const double doubleHeightOutput = heightOutput;
			const double doublePreviousHeightOutput = previousHeightOutput;
			heightOutput = static_cast<int>(splitVertical ?
							std::lround(doubleHeightOutput * rcClient.Height() / rcClient.Width()) :
							std::lround(doubleHeightOutput * rcClient.Width() / rcClient.Height()));
			previousHeightOutput = static_cast<int>(splitVertical ?
								std::lround(doublePreviousHeightOutput * rcClient.Height() / rcClient.Width()) :
								std::lround(doublePreviousHeightOutput * rcClient.Width() / rcClient.Height()));
		}
		splitVertical = !splitVertical;
		heightOutput = NormaliseSplit(heightOutput);
		SizeSubWindows();
		CheckMenus();
		Redraw();
		break;

	case IDM_LINENUMBERMARGIN:
		lineNumbers = !lineNumbers;
		SetLineNumberWidth();
		CheckMenus();
		break;

	case IDM_SELMARGIN:
		margin = !margin;
		wEditor.SetMarginWidthN(1, margin ? marginWidth : 0);
		CheckMenus();
		break;

	case IDM_FOLDMARGIN:
		foldMargin = !foldMargin;
		SetFoldWidth();
		CheckMenus();
		break;

	case IDM_VIEWEOL:
		wEditor.SetViewEOL(!wEditor.ViewEOL());
		CheckMenus();
		break;

	case IDM_VIEWTOOLBAR:
		tbVisible = !tbVisible;
		ShowToolBar();
		CheckMenus();
		break;

	case IDM_TOGGLEOUTPUT:
		ToggleOutputVisible();
		CheckMenus();
		break;

	case IDM_TOGGLEPARAMETERS:
		ParametersDialog(false);
		CheckMenus();
		break;

	case IDM_WRAP:
		wrap = !wrap;
		wEditor.SetWrapMode(wrap ? wrapStyle : SA::Wrap::None);
		CheckMenus();
		break;

	case IDM_WRAPOUTPUT:
		wrapOutput = !wrapOutput;
		wOutput.SetWrapMode(wrapOutput ? wrapStyle : SA::Wrap::None);
		CheckMenus();
		break;

	case IDM_READONLY:
		CurrentBuffer()->isReadOnly = !CurrentBuffer()->isReadOnly;
		wEditor.SetReadOnly(CurrentBuffer()->isReadOnly);
		UpdateStatusBar(true);
		CheckMenus();
		SetBuffersMenu();
		SetWindowName();
		break;

	case IDM_VIEWTABBAR:
		tabVisible = !tabVisible;
		ShowTabBar();
		CheckMenus();
		break;

	case IDM_VIEWSTATUSBAR:
		sbVisible = !sbVisible;
		ShowStatusBar();
		UpdateStatusBar(true);
		CheckMenus();
		break;

	case IDM_CLEAROUTPUT:
		wOutput.ClearAll();
		break;

	case IDM_SWITCHPANE:
		if (pwFocussed == &wEditor)
			WindowSetFocus(wOutput);
		else
			WindowSetFocus(wEditor);
		break;

	case IDM_EOL_CRLF:
		wEditor.SetEOLMode(SA::EndOfLine::CrLf);
		CheckMenus();
		UpdateStatusBar(false);
		break;

	case IDM_EOL_CR:
		wEditor.SetEOLMode(SA::EndOfLine::Cr);
		CheckMenus();
		UpdateStatusBar(false);
		break;
	case IDM_EOL_LF:
		wEditor.SetEOLMode(SA::EndOfLine::Lf);
		CheckMenus();
		UpdateStatusBar(false);
		break;
	case IDM_EOL_CONVERT:
		wEditor.ConvertEOLs(wEditor.EOLMode());
		break;

	case IDM_VIEWSPACE:
		ViewWhitespace(wEditor.ViewWS() == SA::WhiteSpace::Invisible);
		CheckMenus();
		Redraw();
		break;

	case IDM_VIEWGUIDES: {
			const bool viewIG = wEditor.IndentationGuides() == SA::IndentView::None;
			wEditor.SetIndentationGuides(viewIG ? indentExamine : SA::IndentView::None);
			CheckMenus();
			Redraw();
		}
		break;

	case IDM_COMPILE: {
			if (SaveIfUnsureForBuilt() != SaveResult::cancelled) {
				SelectionIntoProperties();
				AddCommand(props.GetWild("command.compile.", FileNameExt().AsUTF8().c_str()), "",
					   SubsystemType("command.compile.subsystem."));
				if (jobQueue.HasCommandToRun())
					Execute();
			}
		}
		break;

	case IDM_BUILD: {
			if (SaveIfUnsureForBuilt() != SaveResult::cancelled) {
				SelectionIntoProperties();
				AddCommand(
					props.GetWild("command.build.", FileNameExt().AsUTF8().c_str()),
					props.GetNewExpandString("command.build.directory.", FileNameExt().AsUTF8().c_str()),
					SubsystemType("command.build.subsystem."));
				if (jobQueue.HasCommandToRun()) {
					jobQueue.isBuilding = true;
					Execute();
				}
			}
		}
		break;

	case IDM_CLEAN: {
			if (SaveIfUnsureForBuilt() != SaveResult::cancelled) {
				SelectionIntoProperties();
				AddCommand(props.GetWild("command.clean.", FileNameExt().AsUTF8().c_str()), "",
					   SubsystemType("command.clean.subsystem."));
				if (jobQueue.HasCommandToRun())
					Execute();
			}
		}
		break;

	case IDM_GO: {
			if (SaveIfUnsureForBuilt() != SaveResult::cancelled) {
				SelectionIntoProperties();
				int flags = 0;

				if (!jobQueue.isBuilt) {
					std::string buildcmd = props.GetNewExpandString("command.go.needs.", FileNameExt().AsUTF8().c_str());
					AddCommand(buildcmd, "",
						   SubsystemType("command.go.needs.subsystem."));
					if (buildcmd.length() > 0) {
						jobQueue.isBuilding = true;
						flags |= jobForceQueue;
					}
				}
				AddCommand(props.GetWild("command.go.", FileNameExt().AsUTF8().c_str()), "",
					   SubsystemType("command.go.subsystem."), "", flags);
				if (jobQueue.HasCommandToRun())
					Execute();
			}
		}
		break;

	case IDM_STOPEXECUTE:
		StopExecute();
		break;

	case IDM_NEXTMSG:
		GoMessage(1);
		break;

	case IDM_PREVMSG:
		GoMessage(-1);
		break;

	case IDM_OPENLOCALPROPERTIES:
		OpenProperties(IDM_OPENLOCALPROPERTIES);
		WindowSetFocus(wEditor);
		break;

	case IDM_OPENUSERPROPERTIES:
		OpenProperties(IDM_OPENUSERPROPERTIES);
		WindowSetFocus(wEditor);
		break;

	case IDM_OPENGLOBALPROPERTIES:
		OpenProperties(IDM_OPENGLOBALPROPERTIES);
		WindowSetFocus(wEditor);
		break;

	case IDM_OPENABBREVPROPERTIES:
		OpenProperties(IDM_OPENABBREVPROPERTIES);
		WindowSetFocus(wEditor);
		break;

	case IDM_OPENLUAEXTERNALFILE:
		OpenProperties(IDM_OPENLUAEXTERNALFILE);
		WindowSetFocus(wEditor);
		break;

	case IDM_OPENDIRECTORYPROPERTIES:
		OpenProperties(IDM_OPENDIRECTORYPROPERTIES);
		WindowSetFocus(wEditor);
		break;

	case IDM_SRCWIN:
		break;

	case IDM_BOOKMARK_TOGGLE:
		BookmarkToggle();
		break;

	case IDM_BOOKMARK_NEXT:
		BookmarkNext(true);
		break;

	case IDM_BOOKMARK_PREV:
		BookmarkNext(false);
		break;

	case IDM_BOOKMARK_NEXT_SELECT:
		BookmarkNext(true, true);
		break;

	case IDM_BOOKMARK_PREV_SELECT:
		BookmarkNext(false, true);
		break;

	case IDM_BOOKMARK_CLEARALL:
		wEditor.MarkerDeleteAll(markerBookmark);
		RemoveFindMarks();
		break;

	case IDM_BOOKMARK_SELECT_ALL:
		BookmarkSelectAll();
		break;

	case IDM_TABSIZE:
		TabSizeDialog();
		break;

	case IDM_MONOFONT:
		CurrentBuffer()->useMonoFont = !CurrentBuffer()->useMonoFont;
		ReadFontProperties();
		Redraw();
		break;

	case IDM_MACROLIST:
		AskMacroList();
		break;
	case IDM_MACROPLAY:
		StartPlayMacro();
		break;
	case IDM_MACRORECORD:
		StartRecordMacro();
		break;
	case IDM_MACROSTOPRECORD:
		StopRecordMacro();
		break;

	case IDM_HELP: {
			SelectionIntoProperties();
			AddCommand(props.GetWild("command.help.", FileNameExt().AsUTF8().c_str()), "",
				   SubsystemType("command.help.subsystem."));
			if (!jobQueue.IsExecuting() && jobQueue.HasCommandToRun()) {
				jobQueue.isBuilding = true;
				Execute();
			}
		}
		break;

	case IDM_HELP_SCITE: {
			SelectionIntoProperties();
			AddCommand(props.GetString("command.scite.help"), "",
				   SubsystemFromChar(props.GetString("command.scite.help.subsystem")[0]));
			if (!jobQueue.IsExecuting() && jobQueue.HasCommandToRun()) {
				jobQueue.isBuilding = true;
				Execute();
			}
		}
		break;

	default:
		if ((cmdID >= bufferCmdID) &&
				(cmdID < bufferCmdID + buffers.size())) {
			SetDocumentAt(cmdID - bufferCmdID);
			CheckReload();
		} else if ((cmdID >= fileStackCmdID) &&
				(cmdID < fileStackCmdID + fileStackMax)) {
			StackMenu(cmdID - fileStackCmdID);
		} else if (cmdID >= importCmdID &&
				(cmdID < importCmdID + importMax)) {
			ImportMenu(cmdID - importCmdID);
		} else if (cmdID >= IDM_TOOLS && cmdID < IDM_TOOLS + toolMax) {
			ToolsMenu(cmdID - IDM_TOOLS);
		} else if (cmdID >= IDM_LANGUAGE && cmdID < IDM_LANGUAGE + 100) {
			SetOverrideLanguage(cmdID - IDM_LANGUAGE);
		} else if (cmdID >= SCI_START) {
			PaneFocused().Call(static_cast<SA::Message>(cmdID));
		}
		break;
	}
}

void SciTEBase::FoldChanged(SA::Line line, SA::FoldLevel levelNow, SA::FoldLevel levelPrev) {
	// Unfold any regions where the new fold structure makes that fold wrong.
	// Will only unfold and show lines and never fold or hide lines.
	if (LevelIsHeader(levelNow)) {
		if (!(LevelIsHeader(levelPrev))) {
			// Adding a fold point.
			wEditor.SetFoldExpanded(line, true);
			if (!wEditor.AllLinesVisible())
				ExpandFolds(line, true, levelPrev);
		}
	} else if (LevelIsHeader(levelPrev)) {
		const SA::Line prevLine = line - 1;
		const SA::FoldLevel levelPrevLine = wEditor.FoldLevel(prevLine);

		// Combining two blocks where the first block is collapsed (e.g. by deleting the line(s) which separate(s) the two blocks)
		if ((LevelNumberPart(levelPrevLine) == LevelNumberPart(levelNow)) && !wEditor.LineVisible(prevLine)) {
			const SA::Line parentLine = wEditor.FoldParent(prevLine);
			const SA::FoldLevel levelParentLine = wEditor.FoldLevel(parentLine);
			wEditor.SetFoldExpanded(parentLine, true);
			ExpandFolds(parentLine, true, levelParentLine);
		}

		if (!wEditor.FoldExpanded(line)) {
			// Removing the fold from one that has been contracted so should expand
			// otherwise lines are left invisible with no way to make them visible
			wEditor.SetFoldExpanded(line, true);
			if (!wEditor.AllLinesVisible())
				// Combining two blocks where the second one is collapsed (e.g. by adding characters in the line which separates the two blocks)
				ExpandFolds(line, true, levelPrev);
		}
	}
	if (!(LevelIsWhitespace(levelNow)) &&
			(LevelNumberPart(levelPrev) > LevelNumberPart(levelNow))) {
		if (!wEditor.AllLinesVisible()) {
			// See if should still be hidden
			const SA::Line parentLine = wEditor.FoldParent(line);
			if (parentLine < 0) {
				wEditor.ShowLines(line, line);
			} else if (wEditor.FoldExpanded(parentLine) && wEditor.LineVisible(parentLine)) {
				wEditor.ShowLines(line, line);
			}
		}
	}
	// Combining two blocks where the first one is collapsed (e.g. by adding characters in the line which separates the two blocks)
	if (!(LevelIsWhitespace(levelNow) && (LevelNumberPart(levelPrev) < LevelNumberPart(levelNow)))) {
		if (!wEditor.AllLinesVisible()) {
			const SA::Line parentLine = wEditor.FoldParent(line);
			if (!wEditor.FoldExpanded(parentLine) && wEditor.LineVisible(line)) {
				wEditor.SetFoldExpanded(parentLine, true);
				const SA::FoldLevel levelParentLine = wEditor.FoldLevel(parentLine);
				ExpandFolds(parentLine, true, levelParentLine);
			}
		}
	}
}

void SciTEBase::ExpandFolds(SA::Line line, bool expand, SA::FoldLevel level) {
	// Expand or contract line and all subordinates
	// level is the fold level of line
	const SA::Line lineMaxSubord = wEditor.LastChild(line, LevelNumberPart(level));
	line++;
	wEditor.Call(expand ? SA::Message::ShowLines : SA::Message::HideLines, line, lineMaxSubord);
	while (line <= lineMaxSubord) {
		const SA::FoldLevel levelLine = wEditor.FoldLevel(line);
		if (LevelIsHeader(levelLine)) {
			wEditor.SetFoldExpanded(line, expand);
		}
		line++;
	}
}

void SciTEBase::FoldAll() {
	wEditor.Colourise(wEditor.EndStyled(), -1);
	const SA::Line maxLine = wEditor.LineCount();
	bool expanding = true;
	for (SA::Line lineSeek = 0; lineSeek < maxLine; lineSeek++) {
		if (LevelIsHeader(wEditor.FoldLevel(lineSeek))) {
			expanding = !wEditor.FoldExpanded(lineSeek);
			break;
		}
	}
	wEditor.SetRedraw(false);
	for (SA::Line line = 0; line < maxLine; line++) {
		const SA::FoldLevel level = wEditor.FoldLevel(line);
		if (LevelIsHeader(level) &&
				(SA::FoldLevel::Base == LevelNumberPart(level))) {
			const SA::Line lineMaxSubord = wEditor.LastChild(line, static_cast<SA::FoldLevel>(-1));
			if (expanding) {
				wEditor.SetFoldExpanded(line, true);
				ExpandFolds(line, true, level);
				line = lineMaxSubord;
			} else {
				wEditor.SetFoldExpanded(line, false);
				if (lineMaxSubord > line)
					wEditor.HideLines(line + 1, lineMaxSubord);
			}
		}
	}
	wEditor.SetRedraw(true);
}

void SciTEBase::GotoLineEnsureVisible(SA::Line line) {
	wEditor.EnsureVisibleEnforcePolicy(line);
	wEditor.GotoLine(line);
}

void SciTEBase::EnsureRangeVisible(GUI::ScintillaWindow &win, SA::Span range, bool enforcePolicy) {
	const SA::Line lineStart = win.LineFromPosition(range.start);
	const SA::Line lineEnd = win.LineFromPosition(range.end);
	for (SA::Line line = lineStart; line <= lineEnd; line++) {
		win.Call(enforcePolicy ? SA::Message::EnsureVisibleEnforcePolicy : SA::Message::EnsureVisible, line);
	}
}

bool SciTEBase::MarginClick(SA::Position position, int modifiers) {
	const SA::Line lineClick = wEditor.LineFromPosition(position);
	const SA::KeyMod km = static_cast<SA::KeyMod>(modifiers);
	if ((FlagIsSet(km, SA::KeyMod::Shift)) && (FlagIsSet(km, SA::KeyMod::Ctrl))) {
		FoldAll();
	} else {
		const SA::FoldLevel levelClick = wEditor.FoldLevel(lineClick);
		if (LevelIsHeader(levelClick)) {
			if (FlagIsSet(km, SA::KeyMod::Shift)) {
				EnsureAllChildrenVisible(lineClick, levelClick);
			} else if (FlagIsSet(km, SA::KeyMod::Ctrl)) {
				ToggleFoldRecursive(lineClick, levelClick);
			} else {
				// Toggle this line
				wEditor.ToggleFold(lineClick);
			}
		}
	}
	return true;
}

void SciTEBase::ToggleFoldRecursive(SA::Line line, SA::FoldLevel level) {
	if (wEditor.FoldExpanded(line)) {
		// This ensure fold structure created before the fold is expanded
		wEditor.LastChild(line, LevelNumberPart(level));
		// Contract this line and all children
		wEditor.SetFoldExpanded(line, false);
		ExpandFolds(line, false, level);
	} else {
		// Expand this line and all children
		wEditor.SetFoldExpanded(line, true);
		ExpandFolds(line, true, level);
	}
}

void SciTEBase::EnsureAllChildrenVisible(SA::Line line, SA::FoldLevel level) {
	// Ensure all children visible
	wEditor.SetFoldExpanded(line, true);
	ExpandFolds(line, true, level);
}

void SciTEBase::NewLineInOutput() {
	if (jobQueue.IsExecuting())
		return;
	SA::Line line = wOutput.LineFromPosition(
				wOutput.CurrentPos()) - 1;
	std::string cmd = GetLine(wOutput, line);
	if (cmd == ">") {
		// Search output buffer for previous command
		line--;
		while (line >= 0) {
			cmd = GetLine(wOutput, line);
			if (StartsWith(cmd, ">") && !StartsWith(cmd, ">Exit")) {
				cmd = cmd.substr(1);
				break;
			}
			line--;
		}
	} else if (StartsWith(cmd, ">")) {
		cmd = cmd.substr(1);
	}
	returnOutputToCommand = false;
	AddCommand(cmd, "", JobSubsystem::cli);
	Execute();
}

void SciTEBase::UpdateUI(const SCNotification *notification) {
	const bool handled = extender && extender->OnUpdateUI();
	if (!handled) {
		BraceMatch(notification->nmhdr.idFrom == IDM_SRCWIN);
		if (notification->nmhdr.idFrom == IDM_SRCWIN) {
			UpdateStatusBar(false);
		}
		CheckMenusClipboard();
	}
	if (CurrentBuffer()->findMarks == Buffer::FindMarks::modified) {
		RemoveFindMarks();
	}
	const SA::Update updated = static_cast<SA::Update>(notification->updated);
	if (FlagIsSet(updated, SA::Update::Selection) || FlagIsSet(updated, SA::Update::Content)) {
		if ((notification->nmhdr.idFrom == IDM_SRCWIN) == (pwFocussed == &wEditor)) {
			// Only highlight focused pane.
			if (FlagIsSet(updated, SA::Update::Selection)) {
				currentWordHighlight.statesOfDelay = CurrentWordHighlight::StatesOfDelay::noDelay; // Selection has just been updated, so delay is disabled.
				currentWordHighlight.textHasChanged = false;
				HighlightCurrentWord(true);
			} else if (currentWordHighlight.textHasChanged) {
				HighlightCurrentWord(false);
			}
		}
	}
}

void SciTEBase::SetCanUndoRedo(bool canUndo_, bool canRedo_) {
	if (canUndo != canUndo_) {
		EnableAMenuItem(IDM_UNDO, canUndo_);
		canUndo = canUndo_;
	}
	if (canRedo != canRedo_) {
		EnableAMenuItem(IDM_REDO, canRedo_);
		canRedo = canRedo_;
	}
}

void SciTEBase::CheckCanUndoRedo() {
	bool canUndoNow = true;
	bool canRedoNow = true;
	if (wEditor.HasFocus()) {
		canUndoNow = wEditor.CanUndo();
		canRedoNow = wEditor.CanRedo();
	} else if (wOutput.HasFocus()) {
		canUndoNow = wOutput.CanUndo();
		canRedoNow = wOutput.CanRedo();
	}
	SetCanUndoRedo(canUndoNow, canRedoNow);
}

void SciTEBase::Modified(const SCNotification *notification) {
	const SA::ModificationFlags modificationType =
		static_cast<SA::ModificationFlags>(notification->modificationType);
	const bool textWasModified = FlagIsSet(modificationType, SA::ModificationFlags::InsertText) ||
		FlagIsSet(modificationType, SA::ModificationFlags::DeleteText);
	if ((notification->nmhdr.idFrom == IDM_SRCWIN) && textWasModified)
		CurrentBuffer()->DocumentModified();
	if (FlagIsSet(modificationType, SA::ModificationFlags::LastStepInUndoRedo)) {
		// When the user hits undo or redo, several normal insert/delete
		// notifications may fire, but we will end up here in the end
		CheckCanUndoRedo();
	} else if (textWasModified) {
		if ((notification->nmhdr.idFrom == IDM_SRCWIN) == (pwFocussed == &wEditor)) {
			currentWordHighlight.textHasChanged = true;
		}
		// This will be called a lot, and usually means "typing".
		SetCanUndoRedo(true, false);
		if (CurrentBuffer()->findMarks == Buffer::FindMarks::marked) {
			CurrentBuffer()->findMarks = Buffer::FindMarks::modified;
		}
	}

	if (notification->linesAdded && lineNumbers && lineNumbersExpand) {
		SetLineNumberWidth();
	}

	if (FlagIsSet(modificationType, SA::ModificationFlags::ChangeFold)) {
		FoldChanged(notification->line,
			    static_cast<SA::FoldLevel>(notification->foldLevelNow),
			    static_cast<SA::FoldLevel>(notification->foldLevelPrev));
	}
}

void SciTEBase::Notify(SCNotification *notification) {
	bool handled = false;
	switch (static_cast<SA::Notification>(notification->nmhdr.code)) {
	case SA::Notification::Painted:
		if ((notification->nmhdr.idFrom == IDM_SRCWIN) == (pwFocussed == &wEditor)) {
			// Only highlight focused pane.
			// Manage delay before highlight when no user selection but there is word at the caret.
			// So the Delay is based on the blinking of caret, scroll...
			// If currentWordHighlight.statesOfDelay == currentWordHighlight.delay,
			// then there is word at the caret without selection, and need some delay.
			if (currentWordHighlight.statesOfDelay == CurrentWordHighlight::StatesOfDelay::delay) {
				if (currentWordHighlight.elapsedTimes.Duration() >= 0.5) {
					currentWordHighlight.statesOfDelay = CurrentWordHighlight::StatesOfDelay::delayJustEnded;
					HighlightCurrentWord(true);
					pwFocussed->InvalidateAll();
				}
			}
		}
		break;

	case SA::Notification::FocusIn:
		SetPaneFocus(notification->nmhdr.idFrom == IDM_SRCWIN);
		CheckMenus();
		break;

	case SA::Notification::FocusOut:
		CheckMenus();
		break;

	case SA::Notification::StyleNeeded: {
			if (extender) {
				// Colourisation may be performed by script
				if ((notification->nmhdr.idFrom == IDM_SRCWIN) && (lexLanguage == SCLEX_CONTAINER)) {
					SA::Position endStyled = wEditor.EndStyled();
					const SA::Line lineEndStyled = wEditor.LineFromPosition(endStyled);
					endStyled = wEditor.LineStart(lineEndStyled);
					StyleWriter styler(wEditor);
					int styleStart = 0;
					if (endStyled > 0)
						styleStart = styler.StyleAt(endStyled - 1);
					styler.SetCodePage(codePage);
					extender->OnStyle(endStyled, notification->position - endStyled,
							  styleStart, &styler);
					styler.Flush();
				}
			}
		}
		break;

	case SA::Notification::CharAdded:
		if (extender)
			handled = extender->OnChar(static_cast<char>(notification->ch));
		if (!handled) {
			if (notification->nmhdr.idFrom == IDM_SRCWIN) {
				CharAdded(notification->ch);
			} else {
				CharAddedOutput(notification->ch);
			}
		}
		break;

	case SA::Notification::SavePointReached:
		if (notification->nmhdr.idFrom == IDM_SRCWIN) {
			if (extender)
				handled = extender->OnSavePointReached();
			if (!handled) {
				CurrentBuffer()->isDirty = false;
			}
		}
		CheckMenus();
		SetWindowName();
		SetBuffersMenu();
		break;

	case SA::Notification::SavePointLeft:
		if (notification->nmhdr.idFrom == IDM_SRCWIN) {
			if (extender)
				handled = extender->OnSavePointLeft();
			if (!handled) {
				CurrentBuffer()->isDirty = true;
				jobQueue.isBuilt = false;
			}
		}
		CheckMenus();
		SetWindowName();
		SetBuffersMenu();
		break;

	case SA::Notification::DoubleClick:
		if (extender)
			handled = extender->OnDoubleClick();
		if (!handled && notification->nmhdr.idFrom == IDM_RUNWIN) {
			GoMessage(0);
		}
		break;

	case SA::Notification::UpdateUI:
		UpdateUI(notification);
		break;

	case SA::Notification::Modified:
		Modified(notification);
		break;

	case SA::Notification::MarginClick: {
			if (extender)
				handled = extender->OnMarginClick();
			if (!handled) {
				if (notification->margin == 2) {
					MarginClick(notification->position, notification->modifiers);
				}
			}
		}
		break;

	case SA::Notification::NeedShown: {
			EnsureRangeVisible(wEditor, SA::Span(notification->position, notification->position + notification->length), false);
		}
		break;

	case SA::Notification::UserListSelection: {
			if (notification->wParam == 2)
				ContinueMacroList(notification->text);
			else if (extender && notification->wParam > 2)
				extender->OnUserListSelection(static_cast<int>(notification->wParam), notification->text);
		}
		break;

	case SA::Notification::CallTipClick: {
			if (notification->position == 1 && currentCallTip > 0) {
				currentCallTip--;
				FillFunctionDefinition();
			} else if (notification->position == 2 && currentCallTip + 1 < maxCallTips) {
				currentCallTip++;
				FillFunctionDefinition();
			}
		}
		break;

	case SA::Notification::MacroRecord:
		RecordMacroCommand(notification);
		break;

	case SA::Notification::URIDropped:
		OpenUriList(notification->text);
		break;

	case SA::Notification::DwellStart:
		if (extender && (SA::InvalidPosition != notification->position)) {
			SA::Span range(notification->position);
			std::string message =
				RangeExtendAndGrab(wEditor,
						   range, &SciTEBase::iswordcharforsel);
			if (message.length()) {
				extender->OnDwellStart(range.start, message.c_str());
			}
		}
		break;

	case SA::Notification::DwellEnd:
		if (extender) {
			extender->OnDwellStart(0, ""); // flags end of calltip
		}
		break;

	case SA::Notification::Zoom:
		SetLineNumberWidth();
		break;

	case SA::Notification::ModifyAttemptRO:
		AbandonAutomaticSave();
		break;

	default:
		// Avoid warning for unhandled enumeration for notifications SciTEBase not interested in
		break;
	}
}

void SciTEBase::CheckMenusClipboard() {
	const bool hasSelection = !CallFocusedElseDefault(false, SA::Message::GetSelectionEmpty);
	EnableAMenuItem(IDM_CUT, hasSelection);
	EnableAMenuItem(IDM_COPY, hasSelection);
	EnableAMenuItem(IDM_CLEAR, hasSelection);
	EnableAMenuItem(IDM_PASTE, CallFocusedElseDefault(true, SA::Message::CanPaste));
	EnableAMenuItem(IDM_SELECTALL, true);
}

void SciTEBase::CheckMenus() {
	CheckMenusClipboard();
	CheckCanUndoRedo();
	EnableAMenuItem(IDM_DUPLICATE, !CurrentBuffer()->isReadOnly);
	EnableAMenuItem(IDM_SHOWCALLTIP, apis != 0);
	EnableAMenuItem(IDM_COMPLETE, apis != 0);
	CheckAMenuItem(IDM_SPLITVERTICAL, splitVertical);
	EnableAMenuItem(IDM_OPENFILESHERE, props.GetInt("check.if.already.open") != 0);
	CheckAMenuItem(IDM_OPENFILESHERE, openFilesHere);
	CheckAMenuItem(IDM_WRAP, wrap);
	CheckAMenuItem(IDM_WRAPOUTPUT, wrapOutput);
	CheckAMenuItem(IDM_READONLY, CurrentBuffer()->isReadOnly);
	CheckAMenuItem(IDM_FULLSCREEN, fullScreen);
	CheckAMenuItem(IDM_VIEWTOOLBAR, tbVisible);
	CheckAMenuItem(IDM_VIEWTABBAR, tabVisible);
	CheckAMenuItem(IDM_VIEWSTATUSBAR, sbVisible);
	CheckAMenuItem(IDM_VIEWEOL, wEditor.ViewEOL());
	CheckAMenuItem(IDM_VIEWSPACE, wEditor.ViewWS() != SA::WhiteSpace::Invisible);
	CheckAMenuItem(IDM_VIEWGUIDES, wEditor.IndentationGuides() != SA::IndentView::None);
	CheckAMenuItem(IDM_LINENUMBERMARGIN, lineNumbers);
	CheckAMenuItem(IDM_SELMARGIN, margin);
	CheckAMenuItem(IDM_FOLDMARGIN, foldMargin);
	CheckAMenuItem(IDM_TOGGLEOUTPUT, heightOutput > 0);
	CheckAMenuItem(IDM_TOGGLEPARAMETERS, ParametersOpen());
	CheckAMenuItem(IDM_MONOFONT, CurrentBuffer()->useMonoFont);
	EnableAMenuItem(IDM_COMPILE, !jobQueue.IsExecuting() &&
			props.GetWild("command.compile.", FileNameExt().AsUTF8().c_str()).size() != 0);
	EnableAMenuItem(IDM_BUILD, !jobQueue.IsExecuting() &&
			props.GetWild("command.build.", FileNameExt().AsUTF8().c_str()).size() != 0);
	EnableAMenuItem(IDM_CLEAN, !jobQueue.IsExecuting() &&
			props.GetWild("command.clean.", FileNameExt().AsUTF8().c_str()).size() != 0);
	EnableAMenuItem(IDM_GO, !jobQueue.IsExecuting() &&
			props.GetWild("command.go.", FileNameExt().AsUTF8().c_str()).size() != 0);
	EnableAMenuItem(IDM_OPENDIRECTORYPROPERTIES, props.GetInt("properties.directory.enable") != 0);
	for (int toolItem = 0; toolItem < toolMax; toolItem++)
		EnableAMenuItem(IDM_TOOLS + toolItem, ToolIsImmediate(toolItem) || !jobQueue.IsExecuting());
	EnableAMenuItem(IDM_STOPEXECUTE, jobQueue.IsExecuting());
	if (buffers.size() > 0) {
		TabSelect(buffers.Current());
		for (int bufferItem = 0; bufferItem < buffers.lengthVisible; bufferItem++) {
			CheckAMenuItem(IDM_BUFFER + bufferItem, bufferItem == buffers.Current());
		}
	}
	EnableAMenuItem(IDM_MACROPLAY, !recording);
	EnableAMenuItem(IDM_MACRORECORD, !recording);
	EnableAMenuItem(IDM_MACROSTOPRECORD, recording);
}

void SciTEBase::ContextMenu(GUI::ScintillaWindow &wSource, GUI::Point pt, GUI::Window wCmd) {
	const SA::Position currentPos = wSource.CurrentPos();
	const SA::Position anchor = wSource.Anchor();
	popup.CreatePopUp();
	const bool writable = !wSource.ReadOnly();
	AddToPopUp("Undo", IDM_UNDO, writable && wSource.CanUndo());
	AddToPopUp("Redo", IDM_REDO, writable && wSource.CanRedo());
	AddToPopUp("");
	AddToPopUp("Cut", IDM_CUT, writable && currentPos != anchor);
	AddToPopUp("Copy", IDM_COPY, currentPos != anchor);
	AddToPopUp("Paste", IDM_PASTE, writable && wSource.CanPaste());
	AddToPopUp("Delete", IDM_CLEAR, writable && currentPos != anchor);
	AddToPopUp("");
	AddToPopUp("Select All", IDM_SELECTALL);
	AddToPopUp("");
	if (wSource.GetID() == wOutput.GetID()) {
		AddToPopUp("Hide", IDM_TOGGLEOUTPUT, true);
	} else {
		AddToPopUp("Close", IDM_CLOSE, true);
	}
	std::string userContextMenu = props.GetNewExpandString("user.context.menu");
	std::replace(userContextMenu.begin(), userContextMenu.end(), '|', '\0');
	const char *userContextItem = userContextMenu.c_str();
	const char *endDefinition = userContextItem + userContextMenu.length();
	while (userContextItem < endDefinition) {
		const char *caption = userContextItem;
		userContextItem += strlen(userContextItem) + 1;
		if (userContextItem < endDefinition) {
			const int cmd = GetMenuCommandAsInt(userContextItem);
			userContextItem += strlen(userContextItem) + 1;
			AddToPopUp(caption, cmd);
		}
	}
	popup.Show(pt, wCmd);
}

/**
 * Ensure that a splitter bar position is inside the main window.
 */
int SciTEBase::NormaliseSplit(int splitPos) {
	const GUI::Rectangle rcClient = GetClientRectangle();
	const int w = rcClient.Width();
	const int h = rcClient.Height();
	if (splitPos < 20)
		splitPos = 0;
	if (splitVertical) {
		if (splitPos > w - heightBar - 20)
			splitPos = w - heightBar;
	} else {
		if (splitPos > h - heightBar - 20)
			splitPos = h - heightBar;
	}
	return splitPos;
}

void SciTEBase::MoveSplit(GUI::Point ptNewDrag) {
	int newHeightOutput = heightOutputStartDrag + (ptStartDrag.y - ptNewDrag.y);
	if (splitVertical)
		newHeightOutput = heightOutputStartDrag + (ptStartDrag.x - ptNewDrag.x);
	newHeightOutput = NormaliseSplit(newHeightOutput);
	if (heightOutput != newHeightOutput) {
		heightOutput = newHeightOutput;
		SizeContentWindows();
		//Redraw();
	}

	previousHeightOutput = newHeightOutput;
}

void SciTEBase::TimerStart(int /* mask */) {
}

void SciTEBase::TimerEnd(int /* mask */) {
}

void SciTEBase::OnTimer() {
	if (delayBeforeAutoSave && (0 == dialogsOnScreen)) {
		// First save the visible buffer to avoid any switching if not needed
		if (CurrentBuffer()->NeedsSave(delayBeforeAutoSave)) {
			Save(sfNone);
		}
		// Then look through the other buffers to save any that need to be saved
		const BufferIndex currentBuffer = buffers.Current();
		for (BufferIndex i = 0; i < buffers.length; i++) {
			if (buffers.buffers[i].NeedsSave(delayBeforeAutoSave)) {
				SetDocumentAt(i);
				Save(sfNone);
			}
		}
		SetDocumentAt(currentBuffer);
	}
}

void SciTEBase::SetIdler(bool on) {
	needIdle = on;
}

void SciTEBase::OnIdle() {
	if (!findMarker.Complete()) {
		wEditor.SetRedraw(false);
		findMarker.Continue();
		wEditor.SetRedraw(true);
		return;
	}
	if (!matchMarker.Complete()) {
		matchMarker.Continue();
		return;
	}
	SetIdler(false);
}

void SciTEBase::SetHomeProperties() {
	props.SetPath("SciteDefaultHome", GetSciteDefaultHome());
	props.SetPath("SciteUserHome", GetSciteUserHome());
}

void SciTEBase::UIAvailable() {
	SetImportMenu();
	if (extender) {
		SetHomeProperties();
		extender->Initialise(this);
	}
}

/**
 * Find the character following a name which is made up of characters from
 * the set [a-zA-Z.]
 */
static GUI::gui_char AfterName(const GUI::gui_char *s) noexcept {
	while (*s && ((*s == '.') ||
			(*s >= 'a' && *s <= 'z') ||
			(*s >= 'A' && *s <= 'Z')))
		s++;
	return *s;
}

void SciTEBase::PerformOne(char *action) {
	const size_t len = UnSlash(action);
	char *arg = strchr(action, ':');
	if (arg) {
		arg++;
		if (isprefix(action, "askfilename:")) {
			extender->OnMacro("filename", filePath.AsUTF8().c_str());
		} else if (isprefix(action, "askproperty:")) {
			PropertyToDirector(arg);
		} else if (isprefix(action, "close:")) {
			Close();
			WindowSetFocus(wEditor);
		} else if (isprefix(action, "currentmacro:")) {
			currentMacro = arg;
		} else if (isprefix(action, "cwd:")) {
			FilePath dirTarget(GUI::StringFromUTF8(arg));
			if (!dirTarget.SetWorkingDirectory()) {
				GUI::gui_string msg = LocaliseMessage("Invalid directory '^0'.", dirTarget.AsInternal());
				WindowMessageBox(wSciTE, msg);
			}
		} else if (isprefix(action, "enumproperties:")) {
			EnumProperties(arg);
		} else if (isprefix(action, "exportashtml:")) {
			SaveToHTML(GUI::StringFromUTF8(arg));
		} else if (isprefix(action, "exportasrtf:")) {
			SaveToRTF(GUI::StringFromUTF8(arg));
		} else if (isprefix(action, "exportaspdf:")) {
			SaveToPDF(GUI::StringFromUTF8(arg));
		} else if (isprefix(action, "exportaslatex:")) {
			SaveToTEX(GUI::StringFromUTF8(arg));
		} else if (isprefix(action, "exportasxml:")) {
			SaveToXML(GUI::StringFromUTF8(arg));
		} else if (isprefix(action, "find:") && wEditor.Created()) {
			findWhat = arg;
			FindNext(false, false);
		} else if (isprefix(action, "goto:") && wEditor.Created()) {
			const SA::Line line = IntegerFromText(arg) - 1;
			GotoLineEnsureVisible(line);
			// jump to column if given and greater than 0
			const char *colstr = strchr(arg, ',');
			if (colstr) {
				const SA::Position col = IntegerFromText(colstr + 1);
				if (col > 0) {
					const SA::Position pos = wEditor.CurrentPos() + col;
					// select the word you have found there
					const SA::Position wordStart = wEditor.WordStartPosition(pos, true);
					const SA::Position wordEnd = wEditor.WordEndPosition(pos, true);
					wEditor.SetSel(wordStart, wordEnd);
				}
			}
		} else if (isprefix(action, "insert:") && wEditor.Created()) {
			wEditor.ReplaceSel(arg);
		} else if (isprefix(action, "loadsession:")) {
			if (*arg) {
				LoadSessionFile(GUI::StringFromUTF8(arg).c_str());
				RestoreSession();
			}
		} else if (isprefix(action, "macrocommand:")) {
			ExecuteMacroCommand(arg);
		} else if (isprefix(action, "macroenable:")) {
			macrosEnabled = atoi(arg);
			SetToolsMenu();
		} else if (isprefix(action, "macrolist:")) {
			StartMacroList(arg);
		} else if (isprefix(action, "menucommand:")) {
			MenuCommand(atoi(arg));
		} else if (isprefix(action, "open:")) {
			Open(GUI::StringFromUTF8(arg), ofSynchronous);
		} else if (isprefix(action, "output:") && wOutput.Created()) {
			wOutput.ReplaceSel(arg);
		} else if (isprefix(action, "property:")) {
			PropertyFromDirector(arg);
		} else if (isprefix(action, "reloadproperties:")) {
			ReloadProperties();
		} else if (isprefix(action, "quit:")) {
			QuitProgram();
		} else if (isprefix(action, "replaceall:") && wEditor.Created()) {
			if (len > strlen(action)) {
				const char *arg2 = arg + strlen(arg) + 1;
				findWhat = arg;
				replaceWhat = arg2;
				ReplaceAll(false);
			}
		} else if (isprefix(action, "saveas:")) {
			if (*arg) {
				SaveAs(GUI::StringFromUTF8(arg).c_str(), true);
			} else {
				SaveAsDialog();
			}
		} else if (isprefix(action, "savesession:")) {
			if (*arg) {
				SaveSessionFile(GUI::StringFromUTF8(arg).c_str());
			}
		} else if (isprefix(action, "setdefaultcwd:")) {
			// This sets cwd to a value that should stay valid: either SciTE_HOME or the
			// SciTE installation directory or directory of SciTE executable.
			GetDefaultDirectory().SetWorkingDirectory();
		} else if (isprefix(action, "extender:")) {
			extender->OnExecute(arg);
		} else if (isprefix(action, "focus:")) {
			ActivateWindow(arg);
		}
	}
}

static constexpr bool IsSwitchCharacter(GUI::gui_char ch) noexcept {
#if defined(__unix__) || defined(__APPLE__)
	return ch == '-';
#else
	return (ch == '-') || (ch == '/');
#endif
}

// Called by SciTEBase::PerformOne when action="enumproperties:"
void SciTEBase::EnumProperties(const char *propkind) {
	const PropSetFile *pf = nullptr;

	if (!extender)
		return;
	if (!strcmp(propkind, "dyn")) {
		SelectionIntoProperties(); // Refresh properties ...
		pf = &props;
	} else if (!strcmp(propkind, "local"))
		pf = &propsLocal;
	else if (!strcmp(propkind, "directory"))
		pf = &propsDirectory;
	else if (!strcmp(propkind, "user"))
		pf = &propsUser;
	else if (!strcmp(propkind, "base"))
		pf = &propsBase;
	else if (!strcmp(propkind, "embed"))
		pf = &propsEmbed;
	else if (!strcmp(propkind, "platform"))
		pf = &propsPlatform;
	else if (!strcmp(propkind, "abbrev"))
		pf = &propsAbbrev;

	if (pf) {
		const char *key = nullptr;
		const char *val = nullptr;
		bool b = pf->GetFirst(key, val);
		while (b) {
			SendOneProperty(propkind, key, val);
			b = pf->GetNext(key, val);
		}
	}
}

void SciTEBase::SendOneProperty(const char *kind, const char *key, const char *val) {
	std::string m = kind;
	m += ":";
	m += key;
	m += "=";
	m += val;
	extender->SendProperty(m.c_str());
}

void SciTEBase::PropertyFromDirector(const char *arg) {
	props.SetLine(arg, false);
}

void SciTEBase::PropertyToDirector(const char *arg) {
	if (!extender)
		return;
	SelectionIntoProperties();
	const std::string gotprop = props.GetString(arg);
	extender->OnMacro("macro:stringinfo", gotprop.c_str());
}

/**
 * Menu/Toolbar command "Record".
 */
void SciTEBase::StartRecordMacro() {
	recording = true;
	CheckMenus();
	wEditor.StartRecord();
}

/**
 * Received a Notification::MacroRecord from Scintilla: send it to director.
 */
bool SciTEBase::RecordMacroCommand(const SCNotification *notification) {
	if (extender) {
		std::string sMessage = StdStringFromInteger(notification->message);
		sMessage += ";";
		sMessage += std::to_string(notification->wParam);
		sMessage += ";";
		const char *t = reinterpret_cast<const char *>(notification->lParam);
		if (t) {
			//format : "<message>;<wParam>;1;<text>"
			sMessage += "1;";
			sMessage += t;
		} else {
			//format : "<message>;<wParam>;0;"
			sMessage += "0;";
		}
		const bool handled = extender->OnMacro("macro:record", sMessage.c_str());
		return handled;
	}
	return true;
}

/**
 * Menu/Toolbar command "Stop recording".
 */
void SciTEBase::StopRecordMacro() {
	wEditor.StopRecord();
	if (extender)
		extender->OnMacro("macro:stoprecord", "");
	recording = false;
	CheckMenus();
}

/**
 * Menu/Toolbar command "Play macro...": tell director to build list of Macro names
 * Through this call, user has access to all macros in Filerx.
 */
void SciTEBase::AskMacroList() {
	if (extender)
		extender->OnMacro("macro:getlist", "");
}

/**
 * List of Macro names has been created. Ask Scintilla to show it.
 */
bool SciTEBase::StartMacroList(const char *words) {
	if (words) {
		wEditor.UserListShow(2, words); //listtype=2
	}

	return true;
}

/**
 * User has chosen a macro in the list. Ask director to execute it.
 */
void SciTEBase::ContinueMacroList(const char *stext) {
	if ((extender) && (*stext != '\0')) {
		currentMacro = stext;
		StartPlayMacro();
	}
}

/**
 * Menu/Toolbar command "Play current macro" (or called from ContinueMacroList).
 */
void SciTEBase::StartPlayMacro() {
	if (extender)
		extender->OnMacro("macro:run", currentMacro.c_str());
}

/*
SciTE received a macro command from director : execute it.
If command needs answer (SCI_GETTEXTLENGTH ...) : give answer to director
*/

static uintptr_t ReadNum(const char *&t) noexcept {
	assert(t);
	const char *argend = strchr(t, ';');	// find ';'
	uintptr_t v = 0;
	if (*t)
		v = IntegerFromText(t);					// read value
	t = (argend) ? (argend + 1) : nullptr;	// update pointer
	return v;						// return value
}

void SciTEBase::ExecuteMacroCommand(const char *command) {
	const char *nextarg = command;
	uintptr_t wParam = 0;
	intptr_t lParam = 0;
	intptr_t rep = 0;				//Scintilla's answer
	const char *answercmd = nullptr;
	SA::Position l = 0;
	std::string string1;	// Long scope as address taken
	char params[4] = "";
	// This code does not validate its input which may cause crashes when bad.
	// 'params' describes types of return values and of arguments.
	// There are exactly 3 characters: return type, wParam, lParam.
	// 0 : void or no param
	// I : integer
	// S : string
	// R : string (for wParam only)
	// For example, "4004;0RS;fruit;mango" performs SCI_SETPROPERTY("fruit","mango") with no return

	// Extract message, parameter specification, wParam, lParam

	const SA::Message message = static_cast<SA::Message>(ReadNum(nextarg));
	if (!nextarg) {
		Trace("Malformed macro command.\n");
		return;
	}
	strncpy(params, nextarg, 3);
	params[3] = '\0';
	nextarg += 4;
	if (*(params + 1) == 'R') {
		// in one function wParam is a string  : void SetProperty(string key,string name)
		const char *s1 = nextarg;
		while (*nextarg != ';')
			nextarg++;
		string1.assign(s1, nextarg - s1);
		wParam = UptrFromString(string1.c_str());
		nextarg++;
	} else {
		wParam = ReadNum(nextarg);
	}

	if (*(params + 2) == 'S')
		lParam = SptrFromString(nextarg);
	else if ((*(params + 2) == 'I') && nextarg)	// nextarg check avoids warning from clang analyze
		lParam = IntegerFromText(nextarg);

	if (*params == '0') {
		// no answer ...
		wEditor.Call(message, wParam, lParam);
		return;
	}

	if (*params == 'S') {
		// string answer
		if (message == SA::Message::GetSelText) {
			l = wEditor.GetSelText(nullptr);
			wParam = 0;
		} else if (message == SA::Message::GetCurLine) {
			const SA::Line line = wEditor.LineFromPosition(wEditor.CurrentPos());
			l = wEditor.LineLength(line);
			wParam = l;
		} else if (message == SA::Message::GetText) {
			l = wEditor.Length();
			wParam = l;
		} else if (message == SA::Message::GetLine) {
			l = wEditor.LineLength(wParam);
		} else {
			l = 0; //unsupported calls EM
		}
		answercmd = "stringinfo:";

	} else {
		//int answer
		answercmd = "intinfo:";
		l = 30;
	}

	std::string tbuff = answercmd;
	const size_t alen = strlen(answercmd);
	tbuff.resize(l + alen + 1);
	if (*params == 'S')
		lParam = SptrFromPointer(&tbuff[alen]);

	if (l > 0)
		rep = wEditor.Call(message, wParam, lParam);
	if (*params == 'I') {
		const std::string sRep = std::to_string(rep);
		sprintf(&tbuff[alen], "%s", sRep.c_str());
	}
	extender->OnMacro("macro", tbuff.c_str());
}

/**
 * Process all the command line arguments.
 * Arguments that start with '-' (also '/' on Windows) are switches or commands with
 * other arguments being file names which are opened. Commands are distinguished
 * from switches by containing a ':' after the command name.
 * The print switch /p is special cased.
 * Processing occurs in two phases to allow switches that occur before any file opens
 * to be evaluated before creating the UI.
 * Call twice, first with phase=0, then with phase=1 after creating UI.
 */
bool SciTEBase::ProcessCommandLine(const GUI::gui_string &args, int phase) {
	bool performPrint = false;
	bool evaluate = phase == 0;
	std::vector<GUI::gui_string> wlArgs = ListFromString(args);
	// Convert args to vector
	for (size_t i = 0; i < wlArgs.size(); i++) {
		const GUI::gui_char *arg = wlArgs[i].c_str();
		if (IsSwitchCharacter(arg[0])) {
			arg++;
			if (arg[0] == '\0' || (arg[0] == '-' && arg[1] == '\0')) {
				if (phase == 1) {
					OpenFromStdin(arg[0] == '-');
				}
			} else if (arg[0] == '@') {
				if (phase == 1) {
					OpenFilesFromStdin();
				}
			} else if ((arg[0] == 'p' || arg[0] == 'P') && (arg[1] == 0)) {
				performPrint = true;
			} else if (GUI::gui_string(arg) == GUI_TEXT("grep") && (wlArgs.size() - i >= 5) && (wlArgs[i+1].size() >= 4)) {
				// in form -grep [w~][c~][d~][b~] "<file-patterns>" "<excluded-patterns>" "<search-string>"
				GrepFlags gf = GrepFlags::stdOut;
				if (wlArgs[i+1][0] == 'w')
					gf = gf | GrepFlags::wholeWord;
				if (wlArgs[i+1][1] == 'c')
					gf = gf | GrepFlags::matchCase;
				if (wlArgs[i+1][2] == 'd')
					gf = gf | GrepFlags::dot;
				if (wlArgs[i+1][3] == 'b')
					gf = gf | GrepFlags::binary;
				std::string sSearch = GUI::UTF8FromString(wlArgs[i+4]);
				std::string unquoted = UnSlashString(sSearch);
				SA::Position originalEnd = 0;
				InternalGrep(gf, FilePath::GetWorkingDirectory(), wlArgs[i+2], wlArgs[i+3], unquoted, originalEnd);
				exit(0);
			} else {
				if (AfterName(arg) == ':') {
					if (StartsWith(arg, GUI_TEXT("open:")) || StartsWith(arg, GUI_TEXT("loadsession:"))) {
						if (phase == 0)
							return performPrint;
						else
							evaluate = true;
					}
					if (evaluate) {
						const std::string sArg = GUI::UTF8FromString(arg);
						std::vector<char> vcArg(sArg.size() + 1);
						std::copy(sArg.begin(), sArg.end(), vcArg.begin());
						PerformOne(&vcArg[0]);
					}
				} else {
					if (evaluate) {
						props.ReadLine(GUI::UTF8FromString(arg).c_str(), PropSetFile::ReadLineState::active,
							       FilePath::GetWorkingDirectory(), filter, nullptr, 0);
					}
				}
			}
		} else {	// Not a switch: it is a file name
			if (phase == 0)
				return performPrint;
			else
				evaluate = true;

			if (!buffers.initialised) {
				InitialiseBuffers();
				if (props.GetInt("save.recent"))
					RestoreRecentMenu();
			}

			if (!PreOpenCheck(arg))
				Open(arg, static_cast<OpenFlags>(ofQuiet|ofSynchronous));
		}
	}
	if (phase == 1) {
		// If we have finished with all args and no buffer is open
		// try to load session.
		if (!buffers.initialised) {
			InitialiseBuffers();
			if (props.GetInt("save.recent"))
				RestoreRecentMenu();
			if (props.GetInt("buffers") && props.GetInt("save.session"))
				RestoreSession();
		}
		// No open file after session load so create empty document.
		if (filePath.IsUntitled() && buffers.length == 1 && !buffers.buffers[0].isDirty) {
			Open(FilePath());
		}
	}
	return performPrint;
}

// Implement ExtensionAPI methods
intptr_t SciTEBase::Send(Pane p, SA::Message msg, uintptr_t wParam, intptr_t lParam) {
	if (p == paneEditor)
		return wEditor.Call(msg, wParam, lParam);
	else
		return wOutput.Call(msg, wParam, lParam);
}
std::string SciTEBase::Range(Pane p, SA::Span range) {
	if (p == paneEditor)
		return wEditor.StringOfSpan(range);
	else
		return wOutput.StringOfSpan(range);
}
void SciTEBase::Remove(Pane p, SA::Position start, SA::Position end) {
	if (p == paneEditor) {
		wEditor.DeleteRange(start, end-start);
	} else {
		wOutput.DeleteRange(start, end-start);
	}
}

void SciTEBase::Insert(Pane p, SA::Position pos, const char *s) {
	if (p == paneEditor)
		wEditor.InsertText(pos, s);
	else
		wOutput.InsertText(pos, s);
}

void SciTEBase::Trace(const char *s) {
	ShowOutputOnMainThread();
	OutputAppendStringSynchronised(s);
}

std::string SciTEBase::Property(const char *key) {
	return props.GetExpandedString(key);
}

void SciTEBase::SetProperty(const char *key, const char *val) {
	const std::string value = props.GetExpandedString(key);
	if (value != val) {
		props.Set(key, val);
		needReadProperties = true;
	}
}

void SciTEBase::UnsetProperty(const char *key) {
	props.Unset(key);
	needReadProperties = true;
}

uintptr_t SciTEBase::GetInstance() {
	return 0;
}

void SciTEBase::ShutDown() {
	QuitProgram();
}

void SciTEBase::Perform(const char *actionList) {
	std::vector<char> vActions(actionList, actionList + strlen(actionList) + 1);
	char *actions = &vActions[0];
	char *nextAct;
	while ((nextAct = strchr(actions, '\n')) != nullptr) {
		*nextAct = '\0';
		PerformOne(actions);
		actions = nextAct + 1;
	}
	PerformOne(actions);
}

void SciTEBase::DoMenuCommand(int cmdID) {
	MenuCommand(cmdID, 0);
}

SA::ScintillaCall &SciTEBase::PaneCaller(Pane p) noexcept {
	if (p == paneEditor)
		return wEditor;
	else
		return wOutput;
}

void SciTEBase::SetFindInFilesOptions() {
	const std::string wholeWordName = std::string("find.option.wholeword.") + StdStringFromInteger(wholeWord);
	props.Set("find.wholeword", props.GetNewExpandString(wholeWordName.c_str()));
	const std::string matchCaseName = std::string("find.option.matchcase.") + StdStringFromInteger(matchCase);
	props.Set("find.matchcase", props.GetNewExpandString(matchCaseName.c_str()));
}
