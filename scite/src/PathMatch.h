// SciTE - Scintilla based Text Editor
/** @file PathMatch.h
 ** Match path patterns.
 **/
// Copyright 2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PATHMATCH_H
#define PATHMATCH_H

bool PatternMatch(std::u32string_view pattern, std::u32string_view text) noexcept;
bool PathMatch(std::string pattern, std::string relPath);

#endif
