# Make file for SciTE on Windows
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes the mingw32 version of GCC 4.x is used and changes will
# be needed to use other compilers.

.PHONY: all clean analyze depend

.SUFFIXES: .cxx .properties
WINDRES ?= windres

# normalize changes Unix style '/' directory separator to '\' for Windows shell
ifeq (\,$(shell echo \\))
	SHELL_TYPE = sh
	normalize = $1
else
	normalize = $(subst /,\,$1)
endif

PYTHON = $(if $(windir),pyw,python3)

WARNINGS += -Wall -pedantic -Wextra

ifdef CLANG
# Can choose aspect to sanitize: address and undefined can simply change SANITIZE but for
# thread also need to create Position Independent Executable -> search online documentation
SANITIZE = address
#SANITIZE = undefined
CXX = clang++
CC = clang
# Microsoft headers are used so _CRT_SECURE_NO_DEPRECATE avoids warnings for standard library like strcat,
DEFINES += -D_CRT_SECURE_NO_DEPRECATE
BASE_FLAGS += -fsanitize=$(SANITIZE)
# Clang doesn't like omitting braces in array initialization but they just add noise,
WARNINGS += -Wno-deprecated-register
WARNINGS += -Wno-missing-braces
WARNINGS += -Wno-language-extension-token
else
WARNINGS += -Wno-cast-function-type
SUBSYSTEM_OPT = -Xlinker --subsystem -Xlinker windows
endif

ifeq ($(OS),Windows_NT)
	ifndef CLANG
		CC = gcc
	endif
	DEL = $(if $(wildcard $(dir $(SHELL))rm.exe), $(dir $(SHELL))rm.exe -f, del)
	COPY = $(if $(wildcard $(dir $(SHELL))cp.exe), $(dir $(SHELL))cp.exe -a, xcopy /Y)
	VER = cmd /c ver
	# Discover Windows version by running 'VER' command and parsing output
	# For Windows 2000 looks like:Microsoft Windows 2000 [Version 5.00.2195]
	WINVERWORDS:=$(wordlist 1,2,$(subst ., ,$(lastword $(shell $(VER)))))
	WINVER:=$(firstword $(WINVERWORDS)).$(lastword $(WINVERWORDS))
	# Windows NT 4 and Windows 2000 do not support themes so turn off
ifeq '$(WINVER)' '4.0'
	DEFINES += -DDISABLE_THEMES -DWIN_TARGET=0x0400
else
ifeq '$(WINVER)' '5.00'
	DEFINES += -DDISABLE_THEMES
else
	LIBS += -luxtheme
endif
endif
else
	DEL = rm -f
	COPY = cp -a
	LIBS += -luxtheme
endif

PROG	= ../bin/SciTE.exe
PROGSTATIC = ../bin/Sc1.exe
DEFINES += -DUNICODE -D_UNICODE

vpath %.h ../src ../../lexilla/include ../../lexilla/access ../../scintilla/include
vpath %.cxx ../src ../../lexilla/access ../../scintilla/call
vpath %.a ../../lexilla/bin ../../scintilla/bin

LIBS += -lgdi32 -luser32 -limm32 -lole32 -luuid -loleaut32 -lmsimg32 -lshell32 -lcomdlg32 -lcomctl32 -ladvapi32

CXXFLAGS += --std=c++17

DEFINES += -D$(if $(DEBUG),DEBUG,NDEBUG)
BASE_FLAGS += $(if $(DEBUG),-g,-O3)

ifndef DEBUG
ifndef CLANG
STRIPFLAG="-Wl,-s"
endif
endif

ifndef NO_LUA
LUA_CORE_OBJS = lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o \
		lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o \
		ltable.o ltm.o lundump.o lvm.o lzio.o

LUA_LIB_OBJS =	lauxlib.o lbaselib.o lbitlib.o lcorolib.o ldblib.o liolib.o lmathlib.o ltablib.o \
		lstrlib.o loadlib.o loslib.o linit.o lutf8lib.o

LUA_OBJS = LuaExtension.o $(LUA_CORE_OBJS) $(LUA_LIB_OBJS)

vpath %.c ../lua/src ../lua/src/lib

INCLUDES += -I ../lua/src
CFLAGS += -DLUA_USER_H=\"scite_lua_win.h\"
else
DEFINES += -DNO_LUA
endif

INCLUDES += -I ../../lexilla/include -I ../../lexilla/access -I ../../scintilla/include -I ../src
RCINCLUDEDIRS=--include-dir ../src

CBASEFLAGS = $(LUA_DEFINES) $(WIDEFLAGS)

LDFLAGS += -mwindows

%.o: %.cxx
	$(CXX) $(DEFINES) $(INCLUDES) $(WARNINGS) $(CPPFLAGS) $(BASE_FLAGS) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(DEFINES) $(INCLUDES) $(WARNINGS) $(CPPFLAGS) $(BASE_FLAGS) $(CFLAGS) -c $< -o $@

SHAREDOBJS=\
	Cookie.o \
	DirectorExtension.o \
	EditorConfig.o \
	ExportHTML.o \
	ExportPDF.o \
	ExportRTF.o \
	ExportTEX.o \
	ExportXML.o \
	FilePath.o \
	FileWorker.o \
	GUIWin.o \
	IFaceTable.o \
	JobQueue.o \
	LexillaAccess.o \
	MatchMarker.o \
	MultiplexExtension.o \
	PathMatch.o \
	PropSetFile.o \
	ScintillaCall.o \
	ScintillaWindow.o \
	SciTEBase.o \
	SciTEBuffers.o \
	SciTEIO.o \
	SciTEProps.o \
	SciTEWinBar.o \
	SciTEWinDlg.o \
	StringHelpers.o \
	StringList.o \
	Strips.o \
	StyleDefinition.o \
	StyleWriter.o \
	UniqueInstance.o \
	Utf8_16.o

OTHER_OBJS = $(SHAREDOBJS) $(LUA_OBJS) SciTERes.o SciTEWin.o

OBJS = Credits.o $(OTHER_OBJS)

DLLS=../bin/Scintilla.dll ../bin/lexilla.dll

NOTBINPROPS=SciTE.properties Embedded.properties
PROPS=$(addprefix ../bin/,$(filter-out $(NOTBINPROPS), $(notdir $(wildcard ../src/*.properties))))

all:	$(PROG) $(PROGSTATIC) $(DLLS) $(PROPS) $(LUA_SCRIPTS)

clean:
	$(DEL) *.exe *.o *.obj *.dll *.res *.map *.plist

analyze:
	clang --analyze $(DEFINES) $(INCLUDES) $(WARNINGS) $(CPPFLAGS) $(BASE_FLAGS) $(CXXFLAGS) *.cxx ../src/*.cxx

depend deps.mak:
	$(PYTHON) AppDepGen.py

../bin/%.dll:	../../lexilla/bin/%.dll
	$(COPY) $(call normalize,$< $(@D))

../bin/%.dll:	../../scintilla/bin/%.dll
	$(COPY) $(call normalize,$< $(@D))

../bin/%.properties:	../src/%.properties
	$(COPY) $(call normalize,$< $(@D))

$(PROG): $(OBJS)
	$(CXX) $(BASE_FLAGS) $(STRIPFLAG) $(SUBSYSTEM_OPT) -o  $@ $^ $(LDFLAGS) $(LIBS) $(LDLIBS)

OBJSSTATIC = \
	$(SHAREDOBJS) \
	Credits.o \
	Sc1.o \
	Sc1Res.o \
	libscintilla.a \
	liblexilla.a \
	$(LUA_OBJS)

$(PROGSTATIC): $(OBJSSTATIC)
	$(CXX) $(BASE_FLAGS) $(STRIPFLAG) $(SUBSYSTEM_OPT) -static -o $@ $^ $(LDFLAGS) $(LIBS) $(LDLIBS)

# Automatically generate dependencies for most files with "make depend"
include deps.mak

Sc1.o: SciTEWin.cxx SciTEWin.h SciTE.h PropSetFile.h \
 Scintilla.h Extender.h FilePath.h SciTEBase.h FileWorker.h JobQueue.h
	$(CXX) $(DEFINES) $(INCLUDES) $(WARNINGS) $(CPPFLAGS) $(BASE_FLAGS) $(CXXFLAGS) -D STATIC_BUILD -c $< -o $@

SciTERes.o:	SciTERes.rc SciTE.h SciTE.exe.manifest
	$(WINDRES) $(RCINCLUDEDIRS) SciTERes.rc $@

# Also depends on ../src/Embedded.properties but may not want to build everywhere
# so must explicitly ask to build it.
Sc1Res.o:	SciTERes.rc SciTE.h SciTE.exe.manifest
	$(WINDRES) $(RCINCLUDEDIRS) SciTERes.rc --define STATIC_BUILD $@

# Make sure Credits gets rebuilt (so its about box gets a new
# date stamp) when any of the other objects are updated.
Credits.o: $(OTHER_OBJS)
