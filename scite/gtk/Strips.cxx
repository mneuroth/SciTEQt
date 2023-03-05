// SciTE - Scintilla based Text Editor
// Strips.cxx - implement strips on GTK
// Copyright 1998-2021 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <initializer_list>
#include <memory>
#include <chrono>

#include <unistd.h>

#include <glib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

#include "GUI.h"
#include "StringHelpers.h"

#include "Extender.h"

#include "SciTE.h"
#include "Widget.h"
#include "Searcher.h"
#include "StripDefinition.h"
#include "Strips.h"

void BackgroundStrip::Creation(GtkWidget *container) {
	WTable table(1, 2);
	SetID(table);
	Strip::Creation(container);
	gtk_container_set_border_width(GTK_CONTAINER(GetID()), 2);
	gtk_box_pack_start(GTK_BOX(container), GTK_WIDGET(GetID()), FALSE, FALSE, 0);

	wProgress.Create();
	table.Add(wProgress, 1, false, 0, 0);
	gtk_widget_show(wProgress);

	wExplanation.Create("");
	table.Label(wExplanation);

	//g_signal_connect(G_OBJECT(GetID()), "set-focus-child", G_CALLBACK(ChildFocusSignal), this);
	//g_signal_connect(G_OBJECT(GetID()), "focus", G_CALLBACK(FocusSignal), this);

	gtk_widget_show(GTK_WIDGET(GetID()));
}

void BackgroundStrip::SetProgress(const GUI::gui_string &explanation, size_t size, size_t progress) {
	gtk_label_set_text(GTK_LABEL(wExplanation.GetID()), explanation.c_str());
	if (size > 0) {
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(wProgress.GetID()),
			static_cast<double>(progress) / static_cast<double>(size));
	}
}

namespace {

const char *textFindPrompt = "Fi_nd:";
const char *textReplacePrompt = "Rep_lace:";
const char *textFindNext = "_Find Next";
const char *textMarkAll = "_Mark All";

const char *textReplace = "_Replace";
const char *textInSelection = "In _Selection";

const char *textFilterPrompt = "_Filter:";

/* XPM */
const char * word1_x_xpm[] = {
"16 16 15 1",
" 	c #FFFFFF",
".	c #000000",
"+	c #E1E1E1",
"@	c #D9D9D9",
"#	c #4D4D4D",
"$	c #7C7C7C",
"%	c #D0D0D0",
"&	c #A7A7A7",
"*	c #8C8C8C",
"=	c #BDBDBD",
"-	c #B2B2B2",
";	c #686868",
">	c #F0F0F0",
",	c #9A9A9A",
"'	c #C7C7C7",
"                ",
"                ",
"                ",
"                ",
"                ",
"                ",
"               .",
".+.+.@##@ .$%#&.",
"*=.-*;++; .>;++.",
",*.$,.  . . .  .",
"=#,.=;++; . ;++.",
"'.=.'@##@ . @#&.",
"                ",
".              .",
".              .",
"................"};

/* XPM */
const char * case_x_xpm[] = {
"16 16 12 1",
" 	c #FFFFFF",
".	c #BDBDBD",
"+	c #4D4D4D",
"@	c #000000",
"#	c #D0D0D0",
"$	c #C7C7C7",
"%	c #7C7C7C",
"&	c #B2B2B2",
"*	c #8C8C8C",
"=	c #E1E1E1",
"-	c #686868",
";	c #9A9A9A",
"                ",
"                ",
"                ",
"                ",
"   .+@@         ",
"  #@$           ",
"  %%      &+@@  ",
"  @*     &+=    ",
"  @*     @&     ",
"  -%     @&     ",
"  .@#    ;+=    ",
"   &+@@   ;@@@  ",
"                ",
"                ",
"                ",
"                "};

/* XPM */
const char * regex_x_xpm[] = {
"16 16 11 1",
" 	c #FFFFFF",
".	c #888888",
"+	c #696969",
"@	c #000000",
"#	c #E0E0E0",
"$	c #484848",
"%	c #B6B6B6",
"&	c #C4C4C4",
"*	c #787878",
"=	c #383838",
"-	c #D3D3D3",
"                ",
"                ",
"                ",
"  .+        @   ",
" #$$#     % + % ",
" +&&+    #*=@=*#",
"-$  $&     +++  ",
"$&  &$    -$ =- ",
"                ",
"                ",
"      %@%       ",
"      %@%       ",
"                ",
"                ",
"                ",
"                "};

/* XPM */
const char * backslash_x_xpm[] = {
"16 16 15 1",
" 	c #FFFFFF",
".	c #141414",
"+	c #585858",
"@	c #A6A6A6",
"#	c #B6B6B6",
"$	c #272727",
"%	c #E0E0E0",
"&	c #979797",
"*	c #000000",
"=	c #696969",
"-	c #484848",
";	c #787878",
">	c #D3D3D3",
",	c #383838",
"'	c #888888",
"                ",
"                ",
"                ",
".       .       ",
"+@      +@   #  ",
"#+      #+   $  ",
" .%  .&* .% =*--",
" ;&  *&  ;&  *  ",
" >,  *   >,  *  ",
"  $> *    $> *  ",
"  &; *    &; *% ",
"  %. *    %. '*.",
"                ",
"                ",
"                ",
"                "};

/* XPM */
const char * around_x_xpm[] = {
"16 16 2 1",
" 	c #FFFFFF",
".	c #000000",
"                ",
"      .....     ",
"     .......    ",
"    ...   ...   ",
"            ..  ",
"            ..  ",
"   ..        .. ",
"  ....       .. ",
" ......      .. ",
"   ..       ..  ",
"   ...      ..  ",
"    ...   ...   ",
"     .......    ",
"      .....     ",
"                ",
"                "};

/* XPM */
const char * up_x_xpm[] = {
"16 16 8 1",
" 	c None",
".	c #FFFFFF",
"+	c #9C9C9C",
"@	c #000000",
"#	c #747474",
"$	c #484848",
"%	c #DFDFDF",
"&	c #BFBFBF",
"................",
"................",
"........+.......",
".......@@#......",
"......@@@@#.....",
".....@@$@$@$....",
"....@@%#@&+@$...",
"...@@%.#@&.&@$..",
"..#@%..#@&..&@&.",
"..#....#@&...&&.",
".......#@&......",
".......#@&......",
".......#@&......",
".......#@&......",
".......#@&......",
"................"};

/* XPM */
static const char *filter_xpm[] = {
/* columns rows colors chars-per-pixel */
"16 16 3 1 ",
"@ c #000000",
". c #808080",
"  c #FFFFFF",
/* pixels */
"      @   @     ",
" ......@.@..... ",
" .      @     . ",
" ......@.@..... ",
"      @   @     ",
"                ",
" @@@@@@@@@@@@@@ ",
" @            @ ",
" @@@@@@@@@@@@@@ ",
"                ",
"      @   @     ",
" ......@.@..... ",
" .      @     . ",
" ......@.@..... ",
"      @   @     ",
"                "
};

/* XPM */
static const char *context_xpm[] = {
/* columns rows colors chars-per-pixel */
"16 16 4 1 ",
"@ c #000000",
". c #808080",
"X c #C1C1C1",
"  c #FFFFFF",
/* pixels */
"                ",
"                ",
" .........      ",
"                ",
" ............   ",
"                ",
" ........       ",
"                ",
" @@@@@@@@@@@@   ",
"                ",
" .........      ",
"                ",
" .............. ",
"                ",
" ...........    ",
"                "
};

const SearchOption toggles[] = {
	{"Match _whole word only", IDM_WHOLEWORD, IDWHOLEWORD},
	{"_Case sensitive", IDM_MATCHCASE, IDMATCHCASE},
	{"Regular _expression", IDM_REGEXP, IDREGEXP},
	{"Transform _backslash expressions", IDM_UNSLASH, IDUNSLASH},
	{"Wrap ar_ound", IDM_WRAPAROUND, IDWRAP},
	{"_Up", IDM_DIRECTIONUP, IDDIRECTIONUP},
	{"Fil_ter", IDM_FILTERSTATE, IDFILTERSTATE},
	{"Conte_xt", IDM_CONTEXTVISIBLE, IDCONTEXTVISIBLE},
};

// Has to be in same order as toggles
const char **xpmImages[] = {
	word1_x_xpm,
	case_x_xpm,
	regex_x_xpm,
	backslash_x_xpm,
	around_x_xpm,
	up_x_xpm,
	filter_xpm,
	context_xpm,
};

}

void FindReplaceStrip::CreateChecks(std::initializer_list<int> checks) {
	size_t i = 0;
	for (const int check : checks) {
		wCheck.push_back(std::make_unique<WCheckDraw>());
		const int optionCommand = toggles[check].cmd;
		wCheck[i]->Create(optionCommand, xpmImages[check], localiser->Text(toggles[check].label));
		wCheck[i]->SetActive(pSearcher->FlagFromCmd(optionCommand));
		wCheck[i]->SetChangeWatcher(this);
		i++;
	}
}

void FindReplaceStrip::SetIncrementalBehaviour(int behaviour) {
	incrementalBehaviour = static_cast<IncrementalBehaviour>(behaviour);
}

void FindReplaceStrip::MarkIncremental() {
 	if (incrementalBehaviour == showAllMatches) {
		pSearcher->MarkAll(Searcher::MarkPurpose::incremental);
	}
}

void FindReplaceStrip::NextIncremental() {
	if (initializingSearch)
		return;

	if (performFilter && !pSearcher->filterState) {
		pSearcher->FilterAll(false);
	}
	if ((incrementalBehaviour == simple) && !pSearcher->filterState)
		return;
	if (pSearcher->findWhat.length()) {
		pSearcher->MoveBack();
	}

	pSearcher->SetFindText(wComboFind.Text());

	if (pSearcher->FindHasText()) {
		pSearcher->FindNext(pSearcher->reverseFind, false, true);
		pSearcher->SetCaretAsStart();
	}
	if (pSearcher->filterState) {
		pSearcher->FilterAll(true);
	} else {
		MarkIncremental();
	}
	WEntry::SetValid(wComboFind.Entry(), !pSearcher->FindHasText() || !pSearcher->failedfind);
	wComboFind.InvalidateAll();
}

bool FindReplaceStrip::KeyDown(const GdkEventKey *event) {
	if (visible) {
		if (Strip::KeyDown(event))
			return true;
		if (event->state & GDK_MOD1_MASK) {
			for (std::unique_ptr<WCheckDraw> &check : wCheck) {
				if (check->ToggleMatchKey(event->keyval)) {
					return true;
				}
			}
		}
	}
	return false;
}

void FindReplaceStrip::GrabToggles() {
	if (!initializingSearch) {
		for (const std::unique_ptr<WCheckDraw> &check : wCheck) {
			pSearcher->FlagFromCmd(check->Command()) = check->Active();
		}
	}
}

void FindReplaceStrip::SetToggles() {
	for (std::unique_ptr<WCheckDraw> &check : wCheck) {
		check->SetActive(pSearcher->FlagFromCmd(check->Command()));
	}
}

void FindReplaceStrip::ShowPopup() {
	GUI::Menu popup;
	popup.CreatePopUp();
	for (const std::unique_ptr<WCheckDraw> &check : wCheck) {
		AddToPopUp(popup, check->Label(), check->Command(), pSearcher->FlagFromCmd(check->Command()));
	}
	const GUI::Rectangle rcButton = wCheck[0]->GetPosition();
	const GUI::Point pt(rcButton.left, rcButton.bottom);
	popup.Show(pt, *this);
}

void FindReplaceStrip::ConnectCombo() {
	g_signal_connect(G_OBJECT(wComboFind.Entry()), "activate",
		G_CALLBACK(ActivateSignal), this);

	g_signal_connect(G_OBJECT(wComboFind.Entry()), "changed",
		G_CALLBACK(FindComboChanged), this);

	g_signal_connect(G_OBJECT(wComboFind.Entry()), "key-press-event",
		G_CALLBACK(EscapeSignal), this);
}

void FindReplaceStrip::ActivateSignal(GtkWidget *, FindReplaceStrip *pStrip) {
	pStrip->FindNextCmd();
}

void FindReplaceStrip::FindComboChanged(GtkEditable *, FindReplaceStrip *pStrip) {
	pStrip->GrabToggles();
	pStrip->NextIncremental();
}

gboolean FindReplaceStrip::EscapeSignal(GtkWidget *w, GdkEventKey *event, FindReplaceStrip *pStrip) {
	if (event->keyval == GKEY_Escape) {
		g_signal_stop_emission_by_name(G_OBJECT(w), "key-press-event");
		pStrip->Close();
	}
	return FALSE;
}

void FindReplaceStrip::CheckChanged() {
	GrabToggles();
	NextIncremental();
}

void FindStrip::Creation(GtkWidget *container) {
	WTable table(1, 10);
	SetID(table);
	Strip::Creation(container);
	gtk_container_set_border_width(GTK_CONTAINER(GetID()), 2);
	gtk_box_pack_start(GTK_BOX(container), GTK_WIDGET(GetID()), FALSE, FALSE, 0);
	wStaticFind.Create(localiser->Text(textFindPrompt));
	table.Label(wStaticFind);

	g_signal_connect(G_OBJECT(GetID()), "set-focus-child", G_CALLBACK(ChildFocusSignal), this);
	g_signal_connect(G_OBJECT(GetID()), "focus", G_CALLBACK(FocusSignal), this);

	wComboFind.Create();
	table.Add(wComboFind, 1, true, 0, 0);

	gtk_widget_show(wComboFind);

	gtk_widget_show(GTK_WIDGET(GetID()));

	ConnectCombo();

	gtk_label_set_mnemonic_widget(GTK_LABEL(wStaticFind.GetID()), GTK_WIDGET(wComboFind.Entry()));

	static ObjectSignal<FindStrip, &FindStrip::FindNextCmd> sigFindNext;
	wButton.Create(localiser->Text(textFindNext), G_CALLBACK(sigFindNext.Function), this);
	table.Add(wButton, 1, false, 0, 0);

	static ObjectSignal<FindStrip, &FindStrip::MarkAllCmd> sigMarkAll;
	wButtonMarkAll.Create(localiser->Text(textMarkAll), G_CALLBACK(sigMarkAll.Function), this);
	table.Add(wButtonMarkAll, 1, false, 0, 0);

	CreateChecks({0,1,2,3,4,5});

	for (const std::unique_ptr<WCheckDraw> &check : wCheck) {
		table.Add(*check, 1, false, 0, 0);
	}
}

void FindStrip::Destruction() {
}

void FindStrip::Show(int buttonHeight) {
	pSearcher->failedfind = false;
	WEntry::SetValid(wComboFind.Entry(), true);
	pSearcher->SetCaretAsStart();
	Strip::Show(buttonHeight);

	gtk_widget_set_size_request(wButton, -1, buttonHeight);
	gtk_widget_set_size_request(wButtonMarkAll, -1, buttonHeight);
	gtk_widget_set_size_request(wComboFind, widthCombo, buttonHeight);
	gtk_widget_set_size_request(GTK_WIDGET(wComboFind.Entry()), -1, buttonHeight);
	gtk_widget_set_size_request(wStaticFind, -1, heightStatic);

	initializingSearch = true;	// Avoid search for initial value in search entry
	wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
	gtk_entry_set_text(wComboFind.Entry(), pSearcher->findWhat.c_str());
	SetToggles();
	initializingSearch = false;
	MarkIncremental();

	gtk_widget_grab_focus(GTK_WIDGET(wComboFind.Entry()));
}

void FindStrip::Close() {
	if (visible) {
		if (pSearcher->havefound) {
			pSearcher->InsertFindInMemory();
		}
		Strip::Close();
		pSearcher->UIClosed();
	}
}

void FindStrip::MenuAction(guint action) {
	if (allowMenuActions) {
		pSearcher->FlagFromCmd(action) = !pSearcher->FlagFromCmd(action);
		SetToggles();
		InvalidateAll();
	}
}

void FindStrip::GrabFields() {
	pSearcher->SetFind(wComboFind.Text());
	GrabToggles();
}

void FindStrip::FindNextCmd() {
	GrabFields();
	bool found = false;
	if (pSearcher->FindHasText()) {
		found = pSearcher->FindNext(pSearcher->reverseFind) >= 0;
	}
	if (pSearcher->ShouldClose(found))
		Close();
	else
		wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
}

void FindStrip::MarkAllCmd() {
	GrabFields();
	pSearcher->MarkAll(Searcher::MarkPurpose::withBookMarks);
	const bool found = pSearcher->FindNext(pSearcher->reverseFind) >= 0;
	if (pSearcher->ShouldClose(found))
		Close();
	else
		wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
}

void FindStrip::ChildFocus(GtkWidget *widget) {
	Strip::ChildFocus(widget);
	pSearcher->UIHasFocus();
}

gboolean FindStrip::Focus(GtkDirectionType direction) {
	const int lastFocusCheck = 5;
	if ((direction == GTK_DIR_TAB_BACKWARD) && wComboFind.HasFocusOnSelfOrChild()) {
		gtk_widget_grab_focus(*wCheck[lastFocusCheck]);
		return TRUE;
	} else if ((direction == GTK_DIR_TAB_FORWARD) && wCheck[lastFocusCheck]->HasFocus()) {
		gtk_widget_grab_focus(GTK_WIDGET(wComboFind.Entry()));
		return TRUE;
	}
	return FALSE;
}

void ReplaceStrip::Creation(GtkWidget *container) {
	WTable tableReplace(2, 8);
	SetID(tableReplace);
	Strip::Creation(container);
	gtk_container_set_border_width(GTK_CONTAINER(GetID()), 2);
	tableReplace.PackInto(GTK_BOX(container), false);

	wStaticFind.Create(localiser->Text(textFindPrompt));
	tableReplace.Label(wStaticFind);

	g_signal_connect(G_OBJECT(GetID()), "set-focus-child", G_CALLBACK(ChildFocusSignal), this);
	g_signal_connect(G_OBJECT(GetID()), "focus", G_CALLBACK(FocusSignal), this);

	wComboFind.Create();
	tableReplace.Add(wComboFind, 1, true, 0, 0);
	wComboFind.Show();

	ConnectCombo();

	gtk_label_set_mnemonic_widget(GTK_LABEL(wStaticFind.GetID()), GTK_WIDGET(wComboFind.Entry()));

	static ObjectSignal<ReplaceStrip, &ReplaceStrip::FindNextCmd> sigFindNext;
	wButtonFind.Create(localiser->Text(textFindNext),
			G_CALLBACK(sigFindNext.Function), this);
	tableReplace.Add(wButtonFind, 1, false, 0, 0);

	static ObjectSignal<ReplaceStrip, &ReplaceStrip::ReplaceAllCmd> sigReplaceAll;
	wButtonReplaceAll.Create(localiser->Text("Replace _All"),
			G_CALLBACK(sigReplaceAll.Function), this);
	tableReplace.Add(wButtonReplaceAll, 1, false, 0, 0);

	CreateChecks({ 0, 1, 2, 3, 4, 6, 7 });

	tableReplace.Add(*wCheck[0], 1, false, 0, 0);
	tableReplace.Add(*wCheck[1], 1, false, 0, 0);
	tableReplace.Add(*wCheck[5], 1, false, 0, 0);
	tableReplace.Add(*wCheck[6], 1, false, 0, 0);

	wStaticReplace.Create(localiser->Text(textReplacePrompt));
	tableReplace.Label(wStaticReplace);

	wComboReplace.Create();
	tableReplace.Add(wComboReplace, 1, true, 0, 0);

	g_signal_connect(G_OBJECT(wComboReplace.Entry()), "key-press-event",
		G_CALLBACK(EscapeSignal), this);

	g_signal_connect(G_OBJECT(wComboReplace.Entry()), "activate",
		G_CALLBACK(ActivateSignal), this);

	gtk_label_set_mnemonic_widget(GTK_LABEL(wStaticReplace.GetID()), GTK_WIDGET(wComboReplace.Entry()));

	static ObjectSignal<ReplaceStrip, &ReplaceStrip::ReplaceCmd> sigReplace;
	wButtonReplace.Create(localiser->Text(textReplace),
			G_CALLBACK(sigReplace.Function), this);
	tableReplace.Add(wButtonReplace, 1, false, 0, 0);

	static ObjectSignal<ReplaceStrip, &ReplaceStrip::ReplaceInSelectionCmd> sigReplaceInSelection;
	wButtonReplaceInSelection.Create(localiser->Text(textInSelection),
			G_CALLBACK(sigReplaceInSelection.Function), this);
	tableReplace.Add(wButtonReplaceInSelection, 1, false, 0, 0);

	tableReplace.Add(*wCheck[2], 1, false, 0, 0);
	tableReplace.Add(*wCheck[3], 1, false, 0, 0);
	tableReplace.Add(*wCheck[4], 1, false, 0, 0);

	// Make the fccus chain move down before moving right
	GList *focusChain = nullptr;
	focusChain = g_list_append(focusChain, wComboFind.Pointer());
	focusChain = g_list_append(focusChain, wComboReplace.Pointer());
	focusChain = g_list_append(focusChain, wButtonFind.Pointer());
	focusChain = g_list_append(focusChain, wButtonReplace.Pointer());
	focusChain = g_list_append(focusChain, wButtonReplaceAll.Pointer());
	focusChain = g_list_append(focusChain, wButtonReplaceInSelection.Pointer());
	focusChain = g_list_append(focusChain, wCheck[0]->Pointer());
	focusChain = g_list_append(focusChain, wCheck[3]->Pointer());
	focusChain = g_list_append(focusChain, wCheck[1]->Pointer());
	focusChain = g_list_append(focusChain, wCheck[4]->Pointer());
	focusChain = g_list_append(focusChain, wCheck[2]->Pointer());

	// gtk_container_set_focus_chain was deprecated in GTK 3.24 to prepare for GTK 4.
	// Replacing it with a focus signal handler is significant work so won't be done, and isn't
	// needed until/if SciTE is ported to GTK 4.
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	gtk_container_set_focus_chain(GTK_CONTAINER(GetID()), focusChain);
G_GNUC_END_IGNORE_DEPRECATIONS

	g_list_free(focusChain);
}

void ReplaceStrip::Destruction() {
}

void ReplaceStrip::Show(int buttonHeight) {
	pSearcher->failedfind = false;
	WEntry::SetValid(wComboFind.Entry(), true);
	pSearcher->SetCaretAsStart();
	Strip::Show(buttonHeight);

	gtk_widget_set_size_request(wButtonFind, -1, buttonHeight);
	gtk_widget_set_size_request(wButtonReplaceAll, -1, buttonHeight);
	gtk_widget_set_size_request(wButtonReplace, -1, buttonHeight);
	gtk_widget_set_size_request(wButtonReplaceInSelection, -1, buttonHeight);

	gtk_widget_set_size_request(wComboFind, widthCombo, buttonHeight);
	gtk_widget_set_size_request(GTK_WIDGET(wComboFind.Entry()), -1, buttonHeight);
	gtk_widget_set_size_request(wComboReplace, widthCombo, buttonHeight);
	gtk_widget_set_size_request(GTK_WIDGET(wComboReplace.Entry()), -1, buttonHeight);

	gtk_widget_set_size_request(wStaticFind, -1, heightStatic);
	gtk_widget_set_size_request(wStaticReplace, -1, heightStatic);

	initializingSearch = true;	// Avoid search for initial value in search entry
	wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
	wComboReplace.FillFromMemory(pSearcher->memReplaces.AsVector());

	gtk_entry_set_text(wComboFind.Entry(), pSearcher->findWhat.c_str());

	SetToggles();
	initializingSearch = false;
	MarkIncremental();

	gtk_widget_grab_focus(GTK_WIDGET(wComboFind.Entry()));
}

void ReplaceStrip::Close() {
	if (visible) {
		if (pSearcher->havefound) {
			pSearcher->InsertFindInMemory();
		}
		Strip::Close();
		pSearcher->filterState = false;
		pSearcher->FilterAll(false);
		pSearcher->UIClosed();
	}
}

void ReplaceStrip::MenuAction(guint action) {
	if (allowMenuActions) {
		pSearcher->FlagFromCmd(action) = !pSearcher->FlagFromCmd(action);
		SetToggles();
		InvalidateAll();
	}
}

void ReplaceStrip::GrabFields() {
	pSearcher->SetFind(wComboFind.Text());
	pSearcher->SetReplace(wComboReplace.Text());
	GrabToggles();
}

void ReplaceStrip::FindNextCmd() {
	GrabFields();
	if (pSearcher->FindHasText()) {
		pSearcher->FindNext(pSearcher->reverseFind);
		wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
	}
}

void ReplaceStrip::ReplaceAllCmd() {
	GrabFields();
	if (pSearcher->FindHasText()) {
		pSearcher->ReplaceAll(false);
		NextIncremental();	// Show not found colour if no more matches.
		wComboReplace.FillFromMemory(pSearcher->memReplaces.AsVector());
		wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
	}
}

void ReplaceStrip::ReplaceCmd() {
	GrabFields();
	pSearcher->ReplaceOnce(incrementalBehaviour == simple);
	NextIncremental();	// Show not found colour if no more matches.
	wComboReplace.FillFromMemory(pSearcher->memReplaces.AsVector());
	wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
}

void ReplaceStrip::ReplaceInSelectionCmd() {
	GrabFields();
	if (pSearcher->FindHasText()) {
		pSearcher->ReplaceAll(true);
		NextIncremental();	// Show not found colour if no more matches.
		wComboReplace.FillFromMemory(pSearcher->memReplaces.AsVector());
		wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
	}
}

void ReplaceStrip::ChildFocus(GtkWidget *widget) {
	Strip::ChildFocus(widget);
	pSearcher->UIHasFocus();
}

gboolean ReplaceStrip::Focus(GtkDirectionType direction) {
	const int lastFocusCheck = 2;	// Due to last column starting with the thirs checkbox
	if ((direction == GTK_DIR_TAB_BACKWARD) && wComboFind.HasFocusOnSelfOrChild()) {
		gtk_widget_grab_focus(*wCheck[lastFocusCheck]);
		return TRUE;
	} else if ((direction == GTK_DIR_TAB_FORWARD) && wCheck[lastFocusCheck]->HasFocus()) {
		gtk_widget_grab_focus(GTK_WIDGET(wComboFind.Entry()));
		return TRUE;
	}
	return FALSE;
}

void FilterStrip::Creation(GtkWidget *container) {
	WTable table(1, 10);
	SetID(table);
	Strip::Creation(container);
	gtk_container_set_border_width(GTK_CONTAINER(GetID()), 1);
	gtk_box_pack_start(GTK_BOX(container), GTK_WIDGET(GetID()), FALSE, FALSE, 0);
	wStaticFind.Create(localiser->Text(textFilterPrompt));
	table.Label(wStaticFind);

	g_signal_connect(G_OBJECT(GetID()), "set-focus-child", G_CALLBACK(ChildFocusSignal), this);
	g_signal_connect(G_OBJECT(GetID()), "focus", G_CALLBACK(FocusSignal), this);

	wComboFind.Create();
	table.Add(wComboFind, 1, true, 0, 0);

	gtk_widget_show(wComboFind);

	gtk_widget_show(GTK_WIDGET(GetID()));

	ConnectCombo();

	gtk_label_set_mnemonic_widget(GTK_LABEL(wStaticFind.GetID()), GTK_WIDGET(wComboFind.Entry()));

	CreateChecks({ 0, 1, 2, 3, 7 });

	for (const std::unique_ptr<WCheckDraw> &check : wCheck) {
		table.Add(*check, 1, false, 0, 0);
	}
}

void FilterStrip::Destruction() {
}

void FilterStrip::Show(int buttonHeight) {
	pSearcher->failedfind = false;
	WEntry::SetValid(wComboFind.Entry(), true);
	pSearcher->SetCaretAsStart();
	Strip::Show(buttonHeight);

	gtk_widget_set_size_request(wComboFind, widthCombo, buttonHeight);
	gtk_widget_set_size_request(GTK_WIDGET(wComboFind.Entry()), -1, buttonHeight);
	gtk_widget_set_size_request(wStaticFind, -1, heightStatic);

	initializingSearch = true;	// Avoid search for initial value in search entry
	wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
	gtk_entry_set_text(wComboFind.Entry(), pSearcher->findWhat.c_str());
	SetToggles();
	initializingSearch = false;
	if (pSearcher->FindHasText()) {
		pSearcher->FilterAll(true);
	}

	gtk_widget_grab_focus(GTK_WIDGET(wComboFind.Entry()));
}

void FilterStrip::Close() {
	if (visible) {
		if (pSearcher->havefound) {
			pSearcher->InsertFindInMemory();
		}
		Strip::Close();
		pSearcher->FilterAll(false);
		pSearcher->UIClosed();
	}
}

void FilterStrip::MenuAction(guint action) {
	if (allowMenuActions) {
		pSearcher->FlagFromCmd(action) = !pSearcher->FlagFromCmd(action);
		SetToggles();
		InvalidateAll();
	}
}

void FilterStrip::GrabFields() {
	pSearcher->SetFind(wComboFind.Text());
	GrabToggles();
}

void FilterStrip::NextIncremental() {
	GrabFields();
	pSearcher->FilterAll(pSearcher->FindHasText());
}

void FilterStrip::FindNextCmd() {
	GrabFields();
	pSearcher->FilterAll(pSearcher->FindHasText());
	wComboFind.FillFromMemory(pSearcher->memFinds.AsVector());
}

void UserStrip::Creation(GtkWidget *container) {
	SetID(tableUser);
	Strip::Creation(container);
	gtk_container_set_border_width(GTK_CONTAINER(GetID()), 2);
	tableUser.PackInto(GTK_BOX(container), false);
	g_signal_connect(G_OBJECT(GetID()), "set-focus-child", G_CALLBACK(ChildFocusSignal), this);
	g_signal_connect(G_OBJECT(GetID()), "focus", G_CALLBACK(FocusSignal), this);
}

void UserStrip::Destruction() {
}

void UserStrip::Show(int buttonHeight) {
	Strip::Show(buttonHeight);
	for (const std::vector<UserControl> &line : psd->controls) {
		for (const UserControl &ctl : line) {
			if (ctl.controlType == UserControl::ucStatic) {
				gtk_widget_set_size_request(GTK_WIDGET(ctl.w.GetID()), -1, heightStatic);
			} else if (ctl.controlType == UserControl::ucEdit) {
				gtk_widget_set_size_request(GTK_WIDGET(ctl.w.GetID()), -1, buttonHeight);
			} else if (ctl.controlType == UserControl::ucCombo) {
				GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ctl.w.GetID())));
				gtk_widget_set_size_request(GTK_WIDGET(entry), -1, buttonHeight);
			} else if (ctl.controlType == UserControl::ucButton || ctl.controlType == UserControl::ucDefaultButton) {
				gtk_widget_set_size_request(GTK_WIDGET(ctl.w.GetID()), -1, buttonHeight);
			}
		}
	}
}

void UserStrip::Close() {
	if (visible) {
		Strip::Close();
		if (pUserStripWatcher)
			pUserStripWatcher->UserStripClosed();
	}
}

bool UserStrip::KeyDown(const GdkEventKey *event) {
	if (visible) {
		if (Strip::KeyDown(event))
			return true;
	}
	return false;
}

void UserStrip::ActivateSignal(GtkWidget *, UserStrip *pStrip) {
	// Treat Enter as pressing the first default button
	for (const std::vector<UserControl> &line : pStrip->psd->controls) {
		for (const UserControl &ctl : line) {
			if (ctl.controlType == UserControl::ucDefaultButton) {
				pStrip->extender->OnUserStrip(ctl.item, static_cast<int>(StripCommand::clicked));
				return;
			}
		}
	}
}

gboolean UserStrip::EscapeSignal(GtkWidget *w, GdkEventKey *event, UserStrip *pStrip) {
	if (event->keyval == GKEY_Escape) {
		g_signal_stop_emission_by_name(G_OBJECT(w), "key-press-event");
		pStrip->Close();
	}
	return FALSE;
}

void UserStrip::ClickThis(const GtkWidget *w) {
	for (const std::vector<UserControl> &line : psd->controls) {
		for (const UserControl &ctl : line) {
			if (w == GTK_WIDGET(ctl.w.GetID())) {
				extender->OnUserStrip(ctl.item, static_cast<int>(StripCommand::clicked));
			}
		}
	}
}

void UserStrip::ClickSignal(GtkWidget *w, UserStrip *pStrip) {
	pStrip->ClickThis(w);
}

void UserStrip::ChildFocus(GtkWidget *widget) {
	Strip::ChildFocus(widget);
}

namespace {

bool WidgetHasFocus(const UserControl *ctl) {
	if (!ctl) {
		return false;
	} else if (ctl->controlType == UserControl::ucCombo) {
		const WComboBoxEntry *pwc = static_cast<const WComboBoxEntry *>(&(ctl->w));
		return pwc->HasFocusOnSelfOrChild();
	} else {
		return ctl->w.HasFocus();
	}
}

}

gboolean UserStrip::Focus(GtkDirectionType direction) {
	const UserControl *ctlFirstFocus = 0;
	const UserControl *ctlLastFocus = 0;
	for (const std::vector<UserControl> &line : psd->controls) {
		for (const UserControl &ctl : line) {
			if (ctl.controlType != UserControl::ucStatic) {
				// Widget can have focus
				ctlLastFocus = &ctl;
				if (!ctlFirstFocus)
					ctlFirstFocus = ctlLastFocus;
			}
		}
	}
	if (ctlFirstFocus && ctlLastFocus) {
		if ((direction == GTK_DIR_TAB_BACKWARD) && WidgetHasFocus(ctlFirstFocus)) {
			gtk_widget_grab_focus(GTK_WIDGET(ctlLastFocus->w.GetID()));
			return TRUE;
		} else if ((direction == GTK_DIR_TAB_FORWARD) && WidgetHasFocus(ctlLastFocus)) {
			gtk_widget_grab_focus(GTK_WIDGET(ctlFirstFocus->w.GetID()));
			return TRUE;
		}
	}
	return FALSE;
}

void UserStrip::SetDescription(const char *description) {
	if (psd) {
		for (const std::vector<UserControl> &line : psd->controls) {
			for (const UserControl &ctl : line) {
				gtk_widget_destroy(GTK_WIDGET(ctl.w.GetID()));
			}
		}
	}
	psd = std::make_unique<StripDefinition>(description);
	tableUser.Resize(psd->controls.size(), psd->columns);

	bool hasSetFocus = false;
	GtkWidget *pwWithAccelerator = 0;
	for (std::vector<UserControl> &line : psd->controls) {
		for (UserControl &ctl : line) {
			UserControl *puc = &ctl;
			std::string sCaption = GtkFromWinCaption(puc->text);
			switch (puc->controlType) {
			case UserControl::ucEdit: {
					WEntry we;
					we.Create(puc->text);
					puc->w.SetID(we.GetID());
					tableUser.Add(we, 1, true, 0, 0);
					break;
				}
			case UserControl::ucCombo: {
					WComboBoxEntry wc;
					wc.Create();
					wc.SetText(puc->text);
					puc->w.SetID(wc.GetID());
					tableUser.Add(wc, 1, true, 0, 0);
					break;
				}
			case UserControl::ucButton:
			case UserControl::ucDefaultButton: {
					WButton wb;
					wb.Create(sCaption, G_CALLBACK(UserStrip::ClickSignal), this);
					puc->w.SetID(wb.GetID());
					tableUser.Add(wb, 1, false, 0, 0);
					if (puc->controlType == UserControl::ucDefaultButton) {
						gtk_widget_grab_default(GTK_WIDGET(puc->w.GetID()));
					}
					break;
				}
			default: {
					WStatic ws;
					ws.Create(sCaption);
					puc->w.SetID(ws.GetID());
#if GTK_CHECK_VERSION(3,14,0)
					gtk_widget_set_halign(GTK_WIDGET(puc->w.GetID()), GTK_ALIGN_END);
					gtk_widget_set_valign(GTK_WIDGET(puc->w.GetID()), GTK_ALIGN_BASELINE);
#else
					gtk_misc_set_alignment(GTK_MISC(puc->w.GetID()), 1.0, 0.5);
#endif
					tableUser.Add(ws, 1, false, 5, 0);
					if (ws.HasMnemonic())
						pwWithAccelerator = GTK_WIDGET(puc->w.GetID());
				}
			}
			gtk_widget_show(GTK_WIDGET(puc->w.GetID()));
			if (!hasSetFocus && (puc->controlType != UserControl::ucStatic)) {
				gtk_widget_grab_focus(GTK_WIDGET(puc->w.GetID()));
				hasSetFocus = true;
			}
			if (pwWithAccelerator && (puc->controlType != UserControl::ucStatic)) {
				gtk_label_set_mnemonic_widget(GTK_LABEL(pwWithAccelerator), GTK_WIDGET(puc->w.GetID()));
				pwWithAccelerator = 0;
			}
		}
		tableUser.NextLine() ;
	}

	for (const std::vector<UserControl> &line : psd->controls) {
		for (const UserControl &ctl : line) {
			if ((ctl.controlType == UserControl::ucEdit) || (ctl.controlType == UserControl::ucCombo)) {
				GtkEntry *entry;
				if (ctl.controlType == UserControl::ucEdit)
					entry = GTK_ENTRY(ctl.w.GetID());
				else
					entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ctl.w.GetID())));

				g_signal_connect(G_OBJECT(entry), "key-press-event", G_CALLBACK(EscapeSignal), this);
				g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(ActivateSignal), this);
			}
		}
	}
}

void UserStrip::SetExtender(Extension *extender_) {
	extender = extender_;
}

void UserStrip::SetUserStripWatcher(UserStripWatcher *pUserStripWatcher_) {
	pUserStripWatcher = pUserStripWatcher_;
}

void UserStrip::Set(int control, const char *value) {
	if (psd) {
		UserControl *ctl = psd->FindControl(control);
		if (ctl) {
			if (ctl->controlType == UserControl::ucEdit) {
				WEntry *pwe = static_cast<WEntry *>(&(ctl->w));
				pwe->SetText(value);
			} else if (ctl->controlType == UserControl::ucCombo) {
				WComboBoxEntry *pwc = static_cast<WComboBoxEntry *>(&(ctl->w));
				pwc->SetText(value);
			}
		}
	}
}

void UserStrip::SetList(int control, const char *value) {
	if (psd) {
		UserControl *ctl = psd->FindControl(control);
		if (ctl) {
			if (ctl->controlType == UserControl::ucCombo) {
				GUI::gui_string sValue = GUI::StringFromUTF8(value);
				std::vector<std::string> listValues = ListFromString(sValue);
				WComboBoxEntry *pwc = static_cast<WComboBoxEntry *>(&(ctl->w));
				pwc->FillFromMemory(listValues);
			}
		}
	}
}

std::string UserStrip::GetValue(int control) {
	if (psd) {
		UserControl *ctl = psd->FindControl(control);
		if (ctl->controlType == UserControl::ucEdit) {
			WEntry *pwe = static_cast<WEntry *>(&(ctl->w));
			return pwe->Text();
		} else if (ctl->controlType == UserControl::ucCombo) {
			WComboBoxEntry *pwc = static_cast<WComboBoxEntry *>(&(ctl->w));
			return pwc->Text();
		}
	}
	return "";
}
