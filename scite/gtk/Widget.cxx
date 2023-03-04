// SciTE - Scintilla based Text Editor
// Widget.cxx - code for manipulating  GTK widgets
// Copyright 2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstring>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "Scintilla.h"

#include "GUI.h"
#include "StringHelpers.h"
#include "Widget.h"

WBase::operator GtkWidget*() const {
	return GTK_WIDGET(GetID());
}

GtkWidget *WBase::Pointer() {
	return GTK_WIDGET(GetID());
}

bool WBase::Sensitive() {
	return gtk_widget_get_sensitive(GTK_WIDGET(Pointer()));
}

void WStatic::Create(const GUI::gui_string &text) {
	SetID(gtk_label_new_with_mnemonic(text.c_str()));
}

bool WStatic::HasMnemonic() {
	return gtk_label_get_mnemonic_keyval(GTK_LABEL(Pointer())) != GKEY_Void;
}

void WStatic::SetMnemonicFor(WBase &w) {
	gtk_label_set_mnemonic_widget(GTK_LABEL(Pointer()), w);
}

void WEntry::Create(const GUI::gui_string &text) {
	SetID(gtk_entry_new());
	if (!text.empty())
		gtk_entry_set_text(GTK_ENTRY(GetID()), text.c_str());
}

void WEntry::ActivatesDefault() {
	gtk_entry_set_activates_default(GTK_ENTRY(GetID()), TRUE);
}

const GUI::gui_char *WEntry::Text() {
	return gtk_entry_get_text(GTK_ENTRY(GetID()));
}

int WEntry::Value() {
	return atoi(Text());
}

void WEntry::SetText(GUI::gui_string text) {
	return gtk_entry_set_text(GTK_ENTRY(GetID()), text.c_str());
}

void WEntry::SetValid(GtkEntry *entry, bool valid) {
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_set_name(GTK_WIDGET(entry), valid ? "" : "entryInvalid");
#else
	if (valid) {
		// Reset widget's background color
		// to the one given by the GTK theme
		gtk_widget_modify_base(GTK_WIDGET(entry), GTK_STATE_NORMAL, NULL);
	} else {
		GdkColor red = { 0, 0xFFFF, 0x6666, 0x6666 };
		gtk_widget_modify_base(GTK_WIDGET(entry), GTK_STATE_NORMAL, &red);
	}
#endif
}

void WComboBoxEntry::Create() {
#if GTK_CHECK_VERSION(3,0,0)
	SetID(gtk_combo_box_text_new_with_entry());
#else
	SetID(gtk_combo_box_entry_new_text());
#endif
}

GtkEntry *WComboBoxEntry::Entry() const {
	return GTK_ENTRY(gtk_bin_get_child(GTK_BIN(GetID())));
}

void WComboBoxEntry::ActivatesDefault() {
	gtk_entry_set_activates_default(Entry(), TRUE);
}

const GUI::gui_char *WComboBoxEntry::Text() {
	return gtk_entry_get_text(Entry());
}

void WComboBoxEntry::SetText(const GUI::gui_string &text) {
	return gtk_entry_set_text(Entry(), text.c_str());
}

bool WComboBoxEntry::HasFocusOnSelfOrChild() const {
	return HasFocus() || gtk_widget_has_focus(GTK_WIDGET(Entry()));
}

void WComboBoxEntry::RemoveText(int position) {
#if GTK_CHECK_VERSION(3,0,0)
	gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(GetID()), position);
#else
	gtk_combo_box_remove_text(GTK_COMBO_BOX(GetID()), position);
#endif
}

void WComboBoxEntry::AppendText(const GUI::gui_string &text) {
#if GTK_CHECK_VERSION(3,0,0)
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(GetID()), text.c_str());
#if GTK_CHECK_VERSION(3,14,0)
	gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(GetID()), GTK_SENSITIVITY_ON);
#endif
#else
	gtk_combo_box_append_text(GTK_COMBO_BOX(GetID()), text.c_str());
#endif
}

void WComboBoxEntry::ClearList() {
#if GTK_CHECK_VERSION(3,0,0)
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(GetID()));
#if GTK_CHECK_VERSION(3,14,0)
	gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(GetID()), GTK_SENSITIVITY_OFF);
#endif
#else
	for (int i = 0; i < 10; i++) {
		RemoveText(0);
	}
#endif
}

void WComboBoxEntry::FillFromMemory(const std::vector<std::string> &mem, bool useTop) {
	ClearList();
	for (const std::string &s : mem) {
		AppendText(s);
	}
	if (useTop) {
		gtk_entry_set_text(Entry(), mem[0].c_str());
	}
}

void WButton::Create(const GUI::gui_string &text, GCallback func, gpointer data) {
	SetID(gtk_button_new_with_mnemonic(text.c_str()));
	gtk_widget_set_can_default(GTK_WIDGET(GetID()), TRUE);
	g_signal_connect(G_OBJECT(GetID()), "clicked", func, data);
}

void WButton::Create(const GUI::gui_string &text) {
	SetID(gtk_button_new_with_mnemonic(text.c_str()));
	gtk_widget_set_can_default(GTK_WIDGET(GetID()), TRUE);
}

void WToggle::Create(const GUI::gui_string &text) {
	SetID(gtk_check_button_new_with_mnemonic(text.c_str()));
}
bool WToggle::Active() const {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(GetID()));
}
void WToggle::SetActive(bool active) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GetID()), active);
}

void WProgress::Create() {
	SetID(gtk_progress_bar_new());
}

WCheckDraw::WCheckDraw() {
}

WCheckDraw::~WCheckDraw() {
}

static void GreyToAlpha(GdkPixbuf *ppb, GdkColor fore) {
	guchar *pixels = gdk_pixbuf_get_pixels(ppb);
	int rowStride = gdk_pixbuf_get_rowstride(ppb);
	int width = gdk_pixbuf_get_width(ppb);
	int height = gdk_pixbuf_get_height(ppb);
	for (int y =0; y<height; y++) {
		guchar *pixelsRow = pixels + rowStride * y;
		for (int x =0; x<width; x++) {
			guchar alpha = pixelsRow[0];
			pixelsRow[3] = 255 - alpha;
			pixelsRow[0] = fore.red / 256;
			pixelsRow[1] = fore.green / 256;
			pixelsRow[2] = fore.blue / 256;
			pixelsRow += 4;
		}
	}
}

void WCheckDraw::Create(int cmd_, const char **xpmImage, const GUI::gui_string &toolTip) {
	cmd = cmd_;
	label = toolTip;

	GtkWidget *button = gtk_toggle_button_new();
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);

	GdkPixbuf *pbGrey = gdk_pixbuf_new_from_xpm_data(xpmImage);
	GdkPixbuf *pbAlpha = gdk_pixbuf_add_alpha(pbGrey, TRUE, 0xff, 0xff, 0);
	g_object_unref(pbGrey);

#if GTK_CHECK_VERSION(3, 0, 0)
	GtkStyleContext *context = gtk_widget_get_style_context(button);
	GdkRGBA rgbaFore;
	gtk_style_context_get_color(context, gtk_style_context_get_state(context), &rgbaFore);
	GdkColor fore;
	fore.red = rgbaFore.red * 65535;
	fore.green = rgbaFore.green * 65535;
	fore.blue = rgbaFore.blue * 65535;
	fore.pixel = 0;
#else
	gtk_widget_ensure_style(button);
	GtkStyle *pStyle = button->style;
	GdkColor fore = pStyle->fg[gtk_widget_get_state(button)];
#endif

	// Convert the grey to alpha and make black
	GreyToAlpha(pbAlpha, fore);
	GtkWidget *image = gtk_image_new_from_pixbuf(pbAlpha);
	g_object_unref(pbAlpha);

	SetID(button);
	gtk_container_add(GTK_CONTAINER(button), image);

	g_signal_connect(button, "toggled", G_CALLBACK(Toggled), this);
	gtk_widget_set_name(GTK_WIDGET(button), "toggler");

	GUI::gui_string toolTipNoMnemonic = toolTip;
	size_t posMnemonic = toolTipNoMnemonic.find("_");
	if (posMnemonic != GUI::gui_string::npos)
		toolTipNoMnemonic.replace(posMnemonic, 1, "");
	gtk_widget_set_tooltip_text(Pointer(), toolTipNoMnemonic.c_str());

	key = KeyFromLabel(toolTip);
}

int WCheckDraw::Command() const {
	return cmd;
}

const char *WCheckDraw::Label() const {
	return label.c_str();
}

void WCheckDraw::Toggled(GtkWidget *, WCheckDraw *pcd) {
	if (pcd->watcher)
		pcd->watcher->CheckChanged();
}

GtkToggleButton *WCheckDraw::ToggleButton() const {
	return reinterpret_cast<GtkToggleButton*>(GetID());
}

bool WCheckDraw::Active() const {
	return gtk_toggle_button_get_active(ToggleButton());
}

void WCheckDraw::SetActive(bool active) {
	gtk_toggle_button_set_active(ToggleButton(), active);
}

void WCheckDraw::Toggle() {
	SetActive(!Active());
}

void WCheckDraw::SetChangeWatcher(CheckDrawWatcher *watcher_) {
	watcher = watcher_;
}

bool WCheckDraw::ToggleMatchKey(int key_) {
	if (key == key_) {
		Toggle();
		return true;
	}
	return false;
}

#if GTK_CHECK_VERSION(3,4,0)
#define USE_GRID 1
#else
#define USE_GRID 0
#endif

WTable::WTable(int rows_, int columns_) :
	rows(rows_), columns(columns_), next(0) {
#if USE_GRID
	SetID(gtk_grid_new());
	gtk_grid_set_column_spacing(GTK_GRID(GetID()), 2);
	gtk_grid_set_row_spacing(GTK_GRID(GetID()), 2);
#else
	SetID(gtk_table_new(rows, columns, FALSE));
	gtk_table_set_col_spacings(GTK_TABLE(GetID()), 2);
	gtk_table_set_row_spacings(GTK_TABLE(GetID()), 2);
#endif
}

void WTable::Add(GtkWidget *child, int width, bool expand, int xpadding, int ypadding) {
	if (child) {
#if USE_GRID
		gtk_widget_set_hexpand(child, expand);
#if GTK_CHECK_VERSION(3,14,0)
		gtk_widget_set_margin_end(child, xpadding);
#else
		gtk_widget_set_margin_right(child, xpadding);
#endif
		gtk_widget_set_margin_bottom(child, ypadding);
		gtk_grid_attach(GTK_GRID(GetID()), child,
			next % columns, next / columns,
			width, 1);
#else
		GtkAttachOptions opts = static_cast<GtkAttachOptions>(
			GTK_SHRINK | GTK_FILL);
		GtkAttachOptions optsExpand = static_cast<GtkAttachOptions>(
			GTK_SHRINK | GTK_FILL | GTK_EXPAND);

		gtk_table_attach(GTK_TABLE(GetID()), child,
			next % columns, next % columns + width,
			next / columns, (next / columns) + 1,
			expand ? optsExpand : opts, opts,
			xpadding, ypadding);
#endif
	}
	next += width;
}

void WTable::Label(GtkWidget *child) {
#if GTK_CHECK_VERSION(3,14,0)
	gtk_widget_set_halign(child, GTK_ALIGN_END);
	gtk_widget_set_valign(child, GTK_ALIGN_BASELINE);
#else
	gtk_misc_set_alignment(GTK_MISC(child), 1.0, 0.5);
#endif
	Add(child);
}

void WTable::PackInto(GtkBox *box, gboolean expand) {
	gtk_box_pack_start(box, Pointer(), expand, expand, 0);
}

void WTable::Resize(int rows_, int columns_) {
	rows = rows_;
	columns = columns_;
#if !USE_GRID
	gtk_table_resize(GTK_TABLE(GetID()), rows, columns);
#endif
	next = 0;
}

void WTable::NextLine() {
	next = ((next - 1) / columns + 1) * columns;
}

GUI::gui_char KeyFromLabel(const GUI::gui_string &label) {
	if (!label.empty()) {
		const size_t posMnemonic = label.find('_');
		if (posMnemonic != GUI::gui_string::npos) {
			return MakeLowerCase(label[posMnemonic + 1]);
		}
	}
	return 0;
}

std::string GtkFromWinCaption(const GUI::gui_string &text) {
	std::string sCaption(text);
	// Escape underlines
	Substitute(sCaption, "_", "__");
	// Replace Windows-style ampersands with GTK underlines
	size_t posFound = sCaption.find("&");
	while (posFound != std::string::npos) {
		std::string nextChar = sCaption.substr(posFound + 1, 1);
		if (nextChar == "&") {
			// Escaped, move on
			posFound += 2;
		} else {
			sCaption.erase(posFound, 1);
			sCaption.insert(posFound, "_", 1);
			posFound += 1;
		}
		posFound = sCaption.find("&", posFound);
	}
	// Unescape ampersands
	Substitute(sCaption, "&&", "&");
	return sCaption;
}

void Dialog::Create(const GUI::gui_string &title) {
	wid = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(GetID()), title.c_str());
	gtk_window_set_resizable(GTK_WINDOW(GetID()), TRUE);
}

void Dialog::Display(GtkWidget *parent, bool modal) {
	// Mark it as a modal transient dialog
	gtk_window_set_modal(GTK_WINDOW(GetID()), modal);
	if (parent) {
		gtk_window_set_transient_for(GTK_WINDOW(GetID()), GTK_WINDOW(parent));
	}
	g_signal_connect(G_OBJECT(GetID()), "destroy", G_CALLBACK(SignalDestroy), this);
	gtk_widget_show_all(GTK_WIDGET(GetID()));
	if (modal) {
		while (Created()) {
			gtk_main_iteration();
		}
	}
}

GtkWidget *Dialog::ResponseButton(const GUI::gui_string &text, int responseID) {
	return gtk_dialog_add_button(GTK_DIALOG(GetID()), text.c_str(), responseID);
}

void Dialog::Present() {
	gtk_window_present(GTK_WINDOW(GetID()));
}

GtkWidget *Dialog::ContentArea() {
#if GTK_CHECK_VERSION(3,0,0)
	return gtk_dialog_get_content_area(GTK_DIALOG(GetID()));
#else
	return GTK_DIALOG(GetID())->vbox;
#endif
}

void Dialog::SignalDestroy(GtkWidget *, Dialog *d) {
	if (d) {
		d->SetID(0);
	}
}

void Strip::Creation(GtkWidget *) {
	g_signal_connect(G_OBJECT(GetID()), "button-press-event", G_CALLBACK(ButtonsPress), this);
}

void Strip::Show(int) {
	gtk_widget_show(Widget(*this));
	visible = true;
}

void Strip::Close() {
	gtk_widget_hide(Widget(*this));
	visible = false;
}

bool Strip::KeyDown(const GdkEventKey *event) {
	bool retVal = false;

	if (visible) {
		if (event->keyval == GKEY_Escape) {
			Close();
			return true;
		}

		if (event->state & GDK_MOD1_MASK) {
			GList *childWidgets = gtk_container_get_children(GTK_CONTAINER(GetID()));
			for (GList *child = g_list_first(childWidgets); child; child = g_list_next(child)) {
				GtkWidget **w = reinterpret_cast<GtkWidget **>(child);
				const std::string name = gtk_widget_get_name(*w);
				std::string label;
				if (name == "GtkButton" || name == "GtkCheckButton") {
					label = gtk_button_get_label(GTK_BUTTON(*w));
				} else if (name == "GtkLabel") {
					label = gtk_label_get_label(GTK_LABEL(*w));
				}
				const char key = KeyFromLabel(label);
				if (static_cast<unsigned int>(key) == event->keyval) {
					//fprintf(stderr, "%p %s %s %c\n", *w, name.c_str(), label.c_str(), key);
					if (name == "GtkButton" || name == "GtkCheckButton") {
						gtk_button_clicked(GTK_BUTTON(*w));
					} else if (name == "GtkLabel") {
						// Only ever use labels to label ComboBoxEntry
						GtkWidget *pwidgetSelect = gtk_label_get_mnemonic_widget(GTK_LABEL(*w));
						if (pwidgetSelect) {
							gtk_widget_grab_focus(pwidgetSelect);
						}
					}
					retVal = true;
					break;
				}
			}
			g_list_free(childWidgets);
		}
	}
	return retVal;
}

void Strip::MenuSignal(GtkMenuItem *menuItem, Strip *pStrip) {
	sptr_t cmd = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuItem), "CmdNum"));
	pStrip->MenuAction(cmd);
}

void Strip::AddToPopUp(GUI::Menu &popup, const GUI::gui_string &label, int cmd, bool checked) {
	allowMenuActions = false;
	GUI::gui_string localised = localiser->Text(label.c_str());
	GtkWidget *menuItem = gtk_check_menu_item_new_with_mnemonic(localised.c_str());
	gtk_menu_shell_append(GTK_MENU_SHELL(popup.GetID()), menuItem);
	g_object_set_data(G_OBJECT(menuItem), "CmdNum", GINT_TO_POINTER(cmd));
	g_signal_connect(G_OBJECT(menuItem),"activate", G_CALLBACK(MenuSignal), this);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuItem), checked ? TRUE : FALSE);
	allowMenuActions = true;
}

void Strip::ChildFocus(GtkWidget *widget) {
	childHasFocus = widget != 0;
}

gboolean Strip::ChildFocusSignal(GtkContainer */*container*/, GtkWidget *widget, Strip *pStrip) {
	pStrip->ChildFocus(widget);
	return FALSE;
}

gboolean Strip::FocusSignal(GtkWidget */*widget*/, GtkDirectionType direction, Strip *pStrip) {
	return pStrip->Focus(direction);
}

bool Strip::VisibleHasFocus() const {
	return visible && childHasFocus;
}

gint Strip::ButtonsPress(GtkWidget *, GdkEventButton *event, Strip *pstrip) {
	if (event->button == 3) {
		pstrip->ShowPopup();
		return TRUE;
	}
	return FALSE;
}
