// SciTE - Scintilla based Text Editor
/** @file LexillaLibrary.h
 ** Interface to loadable lexers.
 ** This does not depend on SciTE code so can be copied out into other projects.
 **/
// Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXILLALIBRARY_H
#define LEXILLALIBRARY_H

// Directory to load default Lexilla from, commonly the directory of the application.
void LexillaSetDefaultDirectory(std::string_view directory);

using LexillaCreatePointer = Scintilla::ILexer5 *(*)(const char *);

void LexillaSetDefault(LexillaCreatePointer pCreate);

// sharedLibraryPaths is a ';' separated list of shared libraries to load.
// On Win32 it is treated as UTF-8 and on Unix it is passed to dlopen directly.
// Return true if any shared libraries are loaded.
bool LexillaLoad(std::string_view sharedLibraryPaths);

Scintilla::ILexer5 *LexillaCreateLexer(std::string_view languageName);

std::vector<std::string> LexillaLibraryProperties();
void LexillaSetProperty(const char *key, const char *value);

#endif
