TEMPLATE 	= subdirs

SUBDIRS		= scintilla/qt/ScintillaEditBase scite/qt
android {
    SUBDIRS	+= CppLispInterpreter/CppLispInterpreter.pro
}

scite/qt.depends = scintilla/qt/ScintillaEditBase
android {
    scite/qt.depends = CppLispInterpreter/CppLispInterpreter.pro
}
