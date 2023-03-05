include(config.pri)

TEMPLATE 	= subdirs
SUBDIRS		= scintilla/qt/ScintillaEditBase

!small {
    SUBDIRS         += scite/qt
    SUBDIRS         += lexilla/src
}

android {
    SUBDIRS	+= CppLispInterpreter/CppLispInterpreter.pro
}

scite/qt.depends += scintilla/qt/ScintillaEditBase
scite/qt.depends += lexilla/src

android {
    scite/qt.depends = CppLispInterpreter/CppLispInterpreter.pro
}
