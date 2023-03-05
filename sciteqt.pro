include(config.pri)

TEMPLATE 	= subdirs
SUBDIRS		= scintilla/qt/ScintillaEditBase

!small {
    SUBDIRS         += lexilla/src
    SUBDIRS         += scite/qt
}

android {
    SUBDIRS	+= CppLispInterpreter/CppLispInterpreter.pro
}

scite/qt.depends = lexilla/src scintilla/qt/ScintillaEditBase

android {
    scite/qt.depends = CppLispInterpreter/CppLispInterpreter.pro
}
