// SciTE - Scintilla based Text Editor
/** @file scite_lua_win.h
 ** SciTE Lua scripting interface.
 **/
// Copyright 2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SCITE_LUA_WIN_H
#define SCITE_LUA_WIN_H

/* Modifications for Windows to allow UTF-8 file names and command lines */
/*
Imported into Lua build with -DLUA_USER_H=\"scite_lua_win.h\"
Redirect fopen and _popen to functions that treat their arguments as UTF-8.
If UTF-8 does not work then retry with the original strings as may be in locale characters.
*/
#if defined(LUA_USE_WINDOWS)
#include <stdio.h>
FILE *scite_lua_fopen(const char *filename, const char *mode);
#define fopen scite_lua_fopen
FILE *scite_lua_popen(const char *filename, const char *mode);
#define _popen scite_lua_popen
#endif

#endif
