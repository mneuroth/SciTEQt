
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cmath>

#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <map>
#include <set>
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
#include "SciTEBase.h"

#include "ScintillaQt.h"

#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#include <QThread>

extern QString ConvertGuiCharToQString(const GUI::gui_char * s);

namespace GUI {

enum { SURROGATE_LEAD_FIRST = 0xD800 };
enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
enum { SURROGATE_TRAIL_LAST = 0xDFFF };

intptr_t ScintillaPrimitive::Send(unsigned int msg, uintptr_t wParam, intptr_t lParam) {

    Scintilla::ScintillaQt * scintilla = reinterpret_cast<Scintilla::ScintillaQt *>(GetID());
    return scintilla->WndProc(msg, wParam, lParam);
}

bool IsDBCSLeadByte(int codePage, char ch) {
    if (Scintilla::API::CpUtf8 == codePage)
        // For lexing, all characters >= 0x80 are treated the
        // same so none is considered a lead byte.
        return false;
    else
#ifdef Q_OS_WIN
        return ::IsDBCSLeadByteEx(codePage, ch) != 0;
#else
        return false;
#endif
}

void SleepMilliseconds(int sleepTime)
{
    QThread::msleep(sleepTime);
}

inline QObject * GetQObject(WindowID winID)
{
    return reinterpret_cast<QObject *>(winID);
}

inline QQuickItem * GetQuickItem(QObject * qObj)
{
    Scintilla::ScintillaQt * pScintilla = qobject_cast<Scintilla::ScintillaQt *>(qObj);
    if( pScintilla!=0 )
    {
        return pScintilla->GetScrollArea();
    }
	QQuickItem * pItem = qobject_cast<QQuickItem *>(qObj);
	if (pItem != 0)
	{
		return pItem;
	}
	return 0;
}

inline QQuickWindow * GetQuickWindow(QObject * qObj)
{
    QQuickWindow * pWin = qobject_cast<QQuickWindow *>(qObj);
    if( pWin!=0 )
    {
        return pWin;
    }
    return 0;
}

#if defined(WIN32)

// from GUIWin.cxx

static unsigned int UTF8Length(const wchar_t *uptr, size_t tlen) noexcept {
    unsigned int len = 0;
    for (size_t i = 0; i < tlen && uptr[i];) {
        const unsigned int uch = uptr[i];
        if (uch < 0x80) {
            len++;
        } else if (uch < 0x800) {
            len += 2;
        } else if ((uch >= SURROGATE_LEAD_FIRST) &&
                (uch <= SURROGATE_TRAIL_LAST)) {
            len += 4;
            i++;
        } else {
            len += 3;
        }
        i++;
    }
    return len;
}

static void UTF8FromUTF16(const wchar_t *uptr, size_t tlen, char *putf) noexcept {
    int k = 0;
    for (size_t i = 0; i < tlen && uptr[i];) {
        const unsigned int uch = uptr[i];
        if (uch < 0x80) {
            putf[k++] = static_cast<char>(uch);
        } else if (uch < 0x800) {
            putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
            putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
        } else if ((uch >= SURROGATE_LEAD_FIRST) &&
                (uch <= SURROGATE_TRAIL_LAST)) {
            // Half a surrogate pair
            i++;
            const unsigned int xch = 0x10000 + ((uch & 0x3ff) << 10) + (uptr[i] & 0x3ff);
            putf[k++] = static_cast<char>(0xF0 | (xch >> 18));
            putf[k++] = static_cast<char>(0x80 | ((xch >> 12) & 0x3f));
            putf[k++] = static_cast<char>(0x80 | ((xch >> 6) & 0x3f));
            putf[k++] = static_cast<char>(0x80 | (xch & 0x3f));
        } else {
            putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
            putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
            putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
        }
        i++;
    }
}

static size_t UTF16Length(const char *s, size_t len) noexcept {
    size_t ulen = 0;
    size_t charLen;
    for (size_t i=0; i<len;) {
        const unsigned char ch = static_cast<unsigned char>(s[i]);
        if (ch < 0x80) {
            charLen = 1;
        } else if (ch < 0x80 + 0x40 + 0x20) {
            charLen = 2;
        } else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
            charLen = 3;
        } else {
            charLen = 4;
            ulen++;
        }
        i += charLen;
        ulen++;
    }
    return ulen;
}

static size_t UTF16FromUTF8(const char *s, size_t len, gui_char *tbuf, size_t tlen) noexcept {
    size_t ui=0;
    const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
    size_t i=0;
    while ((i<len) && (ui<tlen)) {
        unsigned char ch = us[i++];
        if (ch < 0x80) {
            tbuf[ui] = ch;
        } else if (ch < 0x80 + 0x40 + 0x20) {
            tbuf[ui] = static_cast<wchar_t>((ch & 0x1F) << 6);
            ch = us[i++];
            tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
        } else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
            tbuf[ui] = static_cast<wchar_t>((ch & 0xF) << 12);
            ch = us[i++];
            tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + ((ch & 0x7F) << 6));
            ch = us[i++];
            tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
        } else {
            // Outside the BMP so need two surrogates
            int val = (ch & 0x7) << 18;
            ch = us[i++];
            val += (ch & 0x3F) << 12;
            ch = us[i++];
            val += (ch & 0x3F) << 6;
            ch = us[i++];
            val += (ch & 0x3F);
            tbuf[ui] = static_cast<wchar_t>(((val - 0x10000) >> 10) + SURROGATE_LEAD_FIRST);
            ui++;
            tbuf[ui] = static_cast<wchar_t>((val & 0x3ff) + SURROGATE_TRAIL_FIRST);
        }
        ui++;
    }
    return ui;
}

gui_string StringFromUTF8(const char *s) {
    if (!s || !*s) {
        return gui_string();
    }
    const size_t sLen = strlen(s);
    const size_t wideLen = UTF16Length(s, sLen);
    std::vector<gui_char> vgc(wideLen);
    UTF16FromUTF8(s, sLen, &vgc[0], wideLen);
    return gui_string(&vgc[0], wideLen);
}

gui_string StringFromUTF8(const std::string &s) {
    if (s.empty()) {
        return gui_string();
    }
    const size_t sLen = s.length();
    const size_t wideLen = UTF16Length(s.c_str(), sLen);
    std::vector<gui_char> vgc(wideLen);
    UTF16FromUTF8(s.c_str(), sLen, &vgc[0], wideLen);
    return gui_string(&vgc[0], wideLen);
}

std::string UTF8FromString(const gui_string &s) {
    if (s.empty()) {
        return std::string();
    }
    const size_t sLen = s.size();
    const size_t narrowLen = UTF8Length(s.c_str(), sLen);
    std::vector<char> vc(narrowLen);
    UTF8FromUTF16(s.c_str(), sLen, &vc[0]);
    return std::string(&vc[0], narrowLen);
}

gui_string StringFromInteger(long i) {
    return std::to_wstring(i);
}

gui_string StringFromLongLong(long long i) {
    return std::to_wstring(i);
}

gui_string HexStringFromInteger(long i) {
    char number[32];
    sprintf(number, "%0lx", i);
    gui_char gnumber[32];
    size_t n = 0;
    while (number[n]) {
        gnumber[n] = static_cast<gui_char>(number[n]);
        n++;
    }
    gnumber[n] = 0;
    return gui_string(gnumber);
}

std::string LowerCaseUTF8(std::string_view sv) {
    if (sv.empty()) {
        return std::string();
    }
    const std::string s(sv);
    const gui_string gs = StringFromUTF8(s);
    const int chars = ::LCMapString(LOCALE_SYSTEM_DEFAULT, LCMAP_LOWERCASE, gs.c_str(), static_cast<int>(gs.size()), nullptr, 0);
    gui_string lc(chars, L'\0');
    ::LCMapString(LOCALE_SYSTEM_DEFAULT, LCMAP_LOWERCASE, gs.c_str(), static_cast<int>(gs.size()), lc.data(), chars);
    return UTF8FromString(lc);
}

#elif defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)

// from GUIGTK.cxx

gui_string StringFromUTF8(const char *s) {
    if (s)
        return gui_string(s);
    else
        return gui_string("");
}

gui_string StringFromUTF8(const std::string &s) {
    return s;
}

std::string UTF8FromString(const gui_string &s) {
    return s;
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

//#ifdef __ANDROID__

std::string LowerCaseUTF8(std::string_view sv) {
// TODO: check that this is ok for android...
    QString sLower = QString::fromUtf8(sv.data());
    sLower = sLower.toLower();
    return sLower.toStdString();
}

//#else

//std::string LowerCaseUTF8(std::string_view sv) {
//    gchar *lower = g_utf8_strdown(sv.data(), sv.length());
//    const std::string sLower(lower);
//    g_free(lower);
//    return sLower;
//}

//#endif

#endif

void Window::Destroy()
{
    qDebug() << "Window::Destroy()" << endl;
	// nothing to do...
}

bool Window::HasFocus()
{
    QQuickItem * window = GetQuickItem(GetQObject(GetID()));
//qDebug() << "has focus " << window->objectName() << endl;
    if( window != 0 )
    {
        return window->hasFocus();
    }
    return false;
}

Rectangle Window::GetPosition()
{
    QQuickItem * window = GetQuickItem(GetQObject(GetID()));
    if( window != 0 )
    {
//qDebug() << "Window::GetPosition() " << window->width() << " " << window->height() << endl;
        return Rectangle(window->x(), window->y(), window->x()+window->width(), window->y()+window->height());
    }
    return Rectangle();
}

void Window::SetPosition(Rectangle rc)
{
    QQuickItem * window = GetQuickItem(GetQObject(GetID()));
    if( window != 0 )
    {
//qDebug() << "Window::SetPosition() " << rc.left << "," << rc.bottom << " / " << rc.right << "," << rc.top << endl;
        window->setX(rc.left);
        window->setY(rc.bottom);
        window->setWidth(rc.right-rc.left);
        window->setHeight(rc.top-rc.bottom);
    }
}

Rectangle Window::GetClientPosition()
{
    QQuickItem * window = GetQuickItem(GetQObject(GetID()));
    if( window != 0 )
    {
//qDebug() << "Window::GetClientPosition() " << window->width() << " " << window->height() << endl;
        return Rectangle(window->x(), window->y(), window->x()+window->width(), window->y()+window->height());
    }
    return Rectangle();
}

void Window::Show(bool show)
{
	QQuickItem * window = GetQuickItem(GetQObject(GetID()));
	if (window != 0)
	{
		window->setVisible(show);
	}
}

void Window::InvalidateAll()
{
    QQuickItem * window = GetQuickItem(GetQObject(GetID()));
    if( window != 0 )
    {
        window->update();
    }
}

void Window::SetTitle(const gui_char *s)
{
    QQuickWindow * window = GetQuickWindow(GetQObject(GetID()));
    if( window != 0 )
    {
        window->setTitle(ConvertGuiCharToQString(s));
    }
}

void Menu::CreatePopUp()
{
    qDebug() << "MENU::CreatePopUp()" << endl;
}

void Menu::Destroy()
{
    qDebug() << "MENU::Destroy()" << endl;
}

void Menu::Show(Point pt, Window &w)
{
    qDebug() << "MENU::Show()" << endl;
}

}   // namespace

const GUI::gui_char appName[] = GUI_TEXT("SciTEQt");
