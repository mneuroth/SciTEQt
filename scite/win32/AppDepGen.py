#!/usr/bin/env python3
# AppDepGen.py - produce a make dependencies file for SciTE
# Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# Requires Python 3.6 or later

import sys

srcRoot = "../.."

sys.path.append(srcRoot + "/scintilla")

from scripts import Dependencies

topComment = "# Created by AppDepGen.py. To recreate, run AppDepGen.py.\n"

def Generate():
	sciteSources = ["../src/*.cxx", "../lua/src/*.c", "../../lexilla/access/*.cxx", "../../scintilla/call/*.cxx"]
	sciteIncludes = ["../../lexilla/include", "../../lexilla/access", "../../scintilla/include", "../src", "../lua/src"]

	# Header magically injected into Lua builds on Win32 to make Unicode file names work
	luaSubsts = { "LUA_USER_H": "scite_lua_win.h"}

	# Create the dependencies file for g++
	deps = Dependencies.FindDependencies(["../win32/*.cxx"] + sciteSources,  ["../win32"] + sciteIncludes, ".o", "../win32/", luaSubsts)

	# Add Sc1 as the same as SciTEWin
	deps = Dependencies.InsertSynonym(deps, "SciTEWin.o", "Sc1.o")

	Dependencies.UpdateDependencies("../win32/deps.mak", deps, topComment)

	# Create the dependencies file for MSVC

	# Change extension from ".o" to ".obj"
	deps = [[Dependencies.PathStem(obj)+".obj", headers] for obj, headers in deps]

	Dependencies.UpdateDependencies("../win32/nmdeps.mak", deps, topComment)

if __name__ == "__main__":
	Generate()