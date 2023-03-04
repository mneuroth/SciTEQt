// SciTE - Scintilla based Text Editor
/** @file GUIGTK.cxx
 ** Interface to platform GUI facilities.
 ** Split off from Scintilla's Platform.h to avoid SciTE depending on implementation of Scintilla.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <ctime>

#include <string>
#include <string_view>
#include <chrono>
#include <sstream>

#include <gtk/gtk.h>

#include "Scintilla.h"
#include "ScintillaWidget.h"

#include "GUI.h"

namespace GUI {

gui_string StringFromUTF8(const char *s) {
	if (s)
		return gui_string(s);
	else
		return gui_string("");
}

gui_string StringFromUTF8(const std::string &s) {
	return s;
}

gui_string StringFromUTF8(std::string_view sv) {
	return gui_string(sv);
}

std::string UTF8FromString(gui_string_view sv) {
	return std::string(sv);
}

gui_string StringFromInteger(long i) {
	char number[32];
	sprintf(number, "%0ld", i);
	return gui_string(number);
}

gui_string StringFromLongLong(long long i) {
	try {
		std::ostringstream strstrm;
		strstrm << i;
		return StringFromUTF8(strstrm.str());
	} catch (std::exception &) {
		// Exceptions not enabled on stream but still causes diagnostic in Coverity.
		// Simply swallow the failure and return the default value.
	}
	return gui_string();
}

std::string LowerCaseUTF8(std::string_view sv) {
	gchar *lower = g_utf8_strdown(sv.data(), sv.length());
	const std::string sLower(lower);
	g_free(lower);
	return sLower;
}

static GtkWidget *PWidget(WindowID wid) {
	return static_cast<GtkWidget *>(wid);
}

void Window::Destroy() {
	if (wid)
		gtk_widget_destroy(GTK_WIDGET(wid));
	wid = 0;
}

bool Window::HasFocus() const noexcept {
	return gtk_widget_has_focus(GTK_WIDGET(wid));
}

Rectangle Window::GetPosition() {
	// Before any size allocated pretend its 1000 wide so not scrolled
	Rectangle rc(0, 0, 1000, 1000);
	if (wid) {
		GtkAllocation allocation;
		gtk_widget_get_allocation(PWidget(wid), &allocation);
		rc.left = allocation.x;
		rc.top = allocation.y;
		if (allocation.width > 20) {
			rc.right = rc.left + allocation.width;
			rc.bottom = rc.top + allocation.height;
		}
	}
	return rc;
}

void Window::SetPosition(Rectangle rc) {
	GtkAllocation alloc;
	alloc.x = rc.left;
	alloc.y = rc.top;
	alloc.width = rc.Width();
	alloc.height = rc.Height();
	gtk_widget_size_allocate(PWidget(wid), &alloc);
}

Rectangle Window::GetClientPosition() {
	// On GTK, the client position is the window position
	return GetPosition();
}

void Window::Show(bool show) {
	if (show)
		gtk_widget_show(PWidget(wid));
}

void Window::InvalidateAll() {
	if (wid) {
		gtk_widget_queue_draw(PWidget(wid));
	}
}

void Window::SetTitle(const char *s) {
	gtk_window_set_title(GTK_WINDOW(wid), s);
}

void Window::SetRedraw(bool /* redraw */) {
	// Could call gdk_window_freeze_updates / gdk_window_thaw_updates here
	// but unsure what the side effects will be.
}

void Menu::CreatePopUp() {
	Destroy();
	mid = gtk_menu_new();
	g_object_ref_sink(G_OBJECT(mid));
}

void Menu::Destroy() noexcept {
	if (mid)
		g_object_unref(mid);
	mid = 0;
}

#if !GTK_CHECK_VERSION(3,22,0)
static void  MenuPositionFunc(GtkMenu *, gint *x, gint *y, gboolean *, gpointer userData) {
	sptr_t intFromPointer = GPOINTER_TO_INT(userData);
	*x = intFromPointer & 0xffff;
	*y = intFromPointer >> 16;
}
#endif

void Menu::Show(Point pt G_GNUC_UNUSED, Window &) {
	GtkMenu *widget = static_cast<GtkMenu *>(mid);
	gtk_widget_show_all(GTK_WIDGET(widget));
#if GTK_CHECK_VERSION(3,22,0)
	// Rely on GTK to do the right thing with positioning
	gtk_menu_popup_at_pointer(widget, NULL);
#else
	int screenHeight = gdk_screen_height();
	int screenWidth = gdk_screen_width();
	GtkRequisition requisition;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_get_preferred_size(GTK_WIDGET(widget), NULL, &requisition);
#else
	gtk_widget_size_request(GTK_WIDGET(widget), &requisition);
#endif
	if ((pt.x + requisition.width) > screenWidth) {
		pt.x = screenWidth - requisition.width;
	}
	if ((pt.y + requisition.height) > screenHeight) {
		pt.y = screenHeight - requisition.height;
	}
	gtk_menu_popup(widget, NULL, NULL, MenuPositionFunc,
		GINT_TO_POINTER((pt.y << 16) | pt.x), 0,
		gtk_get_current_event_time());
#endif
}

sptr_t ScintillaPrimitive::Send(unsigned int msg, uptr_t wParam, sptr_t lParam) {
	return scintilla_send_message(SCINTILLA(GetID()), msg, wParam, lParam);
}

bool IsDBCSLeadByte(int codePage, char ch) {
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	unsigned char uch = static_cast<unsigned char>(ch);
	switch (codePage) {
		case 932:
			// Shift_jis
			return ((uch >= 0x81) && (uch <= 0x9F)) ||
				((uch >= 0xE0) && (uch <= 0xEF));
		case 936:
			// GBK
			return (uch >= 0x81) && (uch <= 0xFE);
		case 950:
			// Big5
			return (uch >= 0x81) && (uch <= 0xFE);
		// Korean EUC-KR may be code page 949.
	}
	return false;
}

void SleepMilliseconds(int sleepTime) {
	g_usleep(sleepTime * 1000);
}

}
