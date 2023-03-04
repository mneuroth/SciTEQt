# Make file for SciTE on Linux or compatible OS
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes GCC 4.x is used and changes will be needed to use other compilers.
# GNU make does not like \r\n line endings so should be saved to CVS in binary form.

.PHONY: all clean analyze depend install uninstall

srcdir ?= .
SCINTILLA_DIR ?= $(srcdir)/../../scintilla
LEXILLA_DIR ?= $(srcdir)/../../lexilla

.SUFFIXES: .cxx .o .h .a .c

WARNINGS += -Wall -pedantic -Wextra

ifdef CLANG
# Can choose aspect to sanitize: address and undefined can simply change SANITIZE but for
# thread also need to create Position Independent Executable -> search online documentation
SANITIZE = address
#SANITIZE = undefined
CXX = clang++
CC = clang
BASE_FLAGS += -fsanitize=$(SANITIZE)
WARNINGS += -Wno-deprecated-register
WARNINGS += -Wno-empty-body
else
WARNINGS += -Wno-misleading-indentation
endif
PKG_CONFIG ?= pkg-config

GTK_VERSION = $(if $(GTK3),gtk+-3.0,gtk+-2.0)

PYTHON = $(if $(windir),pyw,python3)

# For the Gnome desktop stuff to work, prefix must point to where Gnome thinks it is.
CONFIGFLAGS:=$(shell $(PKG_CONFIG) --cflags $(GTK_VERSION))
CONFIGLIB:=$(shell $(PKG_CONFIG) --libs $(GTK_VERSION) gthread-2.0 gmodule-no-export-2.0)
gnomeprefix:=$(shell $(PKG_CONFIG) --variable=prefix $(GTK_VERSION) 2>/dev/null)
ifndef prefix
ifdef gnomeprefix
  prefix=$(gnomeprefix)
else
  prefix=/usr
endif
endif
datadir=$(prefix)/share
pixmapdir=$(datadir)/pixmaps
bindir=$(prefix)/bin
libdir=$(prefix)/lib/scite
SYSCONF_PATH=$(prefix)/share/scite

INSTALL=install
COPY = cp -a

PROG	= $(srcdir)/../bin/SciTE
# make should be run in ../../scintilla/gtk to compile all the lexers.
SHAREDEXTENSION = $(if $(windir),dll,so)
COMPLIB=$(SCINTILLA_DIR)/bin/scintilla.a
COMPONENT=$(srcdir)/../bin/libscintilla.$(SHAREDEXTENSION)
LEXILLA=$(srcdir)/../bin/liblexilla.$(SHAREDEXTENSION)

all: $(PROG) $(COMPONENT) $(LEXILLA)

vpath %.h $(srcdir) $(srcdir)/../src $(LEXILLA_DIR)/include $(SCINTILLA_DIR)/include
vpath %.cxx $(srcdir) $(srcdir)/../src $(srcdir)/../../lexilla/access $(SCINTILLA_DIR)/call

INCLUDES=-I $(LEXILLA_DIR)/include  -I $(LEXILLA_DIR)/access -I $(SCINTILLA_DIR)/include -I $(srcdir)/../src
DEFINES += -DGTK
DEFINES += -DPIXMAP_PATH=\"$(pixmapdir)\" -DSYSCONF_PATH=\"$(SYSCONF_PATH)\"

DEFINES += -D$(if $(DEBUG),DEBUG,NDEBUG)
BASE_FLAGS += $(if $(DEBUG),-g,-O3)

ifndef NO_LUA
LUA_CORE_OBJS = lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o \
		lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o \
		ltable.o ltm.o lundump.o lvm.o lzio.o

LUA_LIB_OBJS =	lauxlib.o lbaselib.o lbitlib.o lcorolib.o ldblib.o liolib.o lmathlib.o ltablib.o \
		lstrlib.o loadlib.o loslib.o linit.o lutf8lib.o

LUA_OBJS = LuaExtension.o $(LUA_CORE_OBJS) $(LUA_LIB_OBJS)

vpath %.c $(srcdir)/../lua/src

INCLUDES += -I$(srcdir)/../lua/src
LUA_DEFINES = -DLUA_USE_POSIX -DLUA_USE_DLOPEN

%.o: %.c
	$(CC) $(CPPFLAGS) $(DEFINES) $(INCLUDES) $(WARNINGS) $(LUA_DEFINES) $(BASE_FLAGS) $(CFLAGS) -c $< -o $@

else
DEFINES += -DNO_LUA
endif

LIBS += -lm -lstdc++

UNAME:=$(shell uname -s)
ifeq ($(UNAME), Linux)
LIBS += -ldl
endif
ifneq (,$(findstring GNU,$(UNAME)))
LIBS += -ldl
endif

%.o: %.cxx
	$(CXX) $(CPPFLAGS) --std=c++17 $(DEFINES) $(INCLUDES) $(WARNINGS) $(CONFIGFLAGS) $(BASE_FLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.plist $(PROG)

analyze:
	clang --analyze --std=c++17 $(DEFINES) $(INCLUDES) $(WARNINGS) $(CONFIGFLAGS) $(BASE_FLAGS) $(CXXFLAGS) $(srcdir)/*.cxx $(srcdir)/../src/*.cxx

depend deps.mak:
	$(PYTHON) AppDepGen.py

../bin/%.$(SHAREDEXTENSION):	../../scintilla/bin/%.$(SHAREDEXTENSION)
	$(COPY) $< $(@D)

../bin/%.$(SHAREDEXTENSION):	../../lexilla/bin/%.$(SHAREDEXTENSION)
	$(COPY) $< $(@D)

# To almost make lua.vers (needs header and footer added) which is only needed after updating Lua:
# nm -g ../bin/SciTE | grep lua | awk '{print "\t\t" $3 ";"}' >lua2.vers

SRC_OBJS = \
	Cookie.o \
	Credits.o \
	EditorConfig.o \
	ExportHTML.o \
	ExportPDF.o \
	ExportRTF.o \
	ExportTEX.o \
	ExportXML.o \
	FilePath.o \
	FileWorker.o \
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
	StringHelpers.o \
	StringList.o \
	StyleDefinition.o \
	StyleWriter.o \
	Utf8_16.o

$(PROG): SciTEGTK.o Strips.o GUIGTK.o Widget.o DirectorExtension.o $(SRC_OBJS) $(LUA_OBJS)
	$(CXX) $(BASE_FLAGS) $(LDFLAGS) -rdynamic -Wl,--as-needed -Wl,-rpath,'$${ORIGIN}' -Wl,--version-script $(srcdir)/lua.vers -Wl,-rpath,$(libdir) $^ -o $@ $(CONFIGLIB) $(LIBS) -L ../../scintilla/bin -lscintilla $(LDLIBS)

# Automatically generate header dependencies with "make depend"
include deps.mak

# The two last install commands will fail if Gnome is not installed or is not at $(prefix).
# This is OK - just means no SciTE in the Gnome Applications menu
# Dead:	install -D SciTEGTK.properties $(SYSCONF_PATH)/SciTEGlobal.properties
install:
	$(INSTALL) -m 755 -d $(DESTDIR)$(bindir) $(DESTDIR)$(SYSCONF_PATH)
	$(INSTALL) -m 755 -d $(DESTDIR)$(libdir)

	$(INSTALL) -m 755 $(PROG) $(DESTDIR)$(bindir)
	$(INSTALL) -m 755 $(COMPONENT) $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 $(LEXILLA) $(DESTDIR)$(libdir)

	for files in $(srcdir)/../src/*.properties $(srcdir)/../doc/*.html $(srcdir)/../doc/SciTEIco.png $(srcdir)/../doc/PrintHi.png $(srcdir)/../doc/SciTEIndicators.png; \
	do \
		$(INSTALL) -m 644 $$files $(DESTDIR)$(SYSCONF_PATH); \
	done

ifdef gnomeprefix
	$(INSTALL) -m 755 -d $(DESTDIR)$(datadir)/applications $(DESTDIR)$(pixmapdir)
	$(INSTALL) -m 644 $(srcdir)/SciTE.desktop $(DESTDIR)$(datadir)/applications/SciTE.desktop
	$(INSTALL) -m 644 $(srcdir)/Sci48M.png $(DESTDIR)$(pixmapdir)/Sci48M.png
endif

uninstall:
	rm -f $(DESTDIR)$(bindir)/SciTE
	rm -f $(DESTDIR)$(libdir)/libscintilla.so
	rm -f $(DESTDIR)$(libdir)/liblexilla.so
	rm -rf $(DESTDIR)$(SYSCONF_PATH)
ifdef gnomeprefix
	rm -f $(DESTDIR)$(datadir)/applications/SciTE.desktop
	rm -f $(DESTDIR)$(pixmapdir)/Sci48M.png
endif
