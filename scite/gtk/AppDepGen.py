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

	deps = Dependencies.FindDependencies(["../gtk/*.cxx"] + sciteSources,  ["../gtk"] + sciteIncludes, ".o", "../gtk/")
	Dependencies.UpdateDependencies("../gtk/deps.mak", deps, topComment)

if __name__ == "__main__":
	Generate()