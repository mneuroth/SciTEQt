QT += quick quickcontrols2 widgets printsupport

TARGET = sciteqt

CONFIG += c++1z

HEADERS += applicationdata.h\
           ../../scintilla/lexilla/src/Lexilla.h \
           sciteqt.h
SOURCES += applicationdata.cpp\
           GUIQt.cpp\
           ../../scintilla/lexilla/src/Lexilla.cxx\
           main.cpp \
           sciteqt.cpp

RC_ICONS = ../win32/SciBall.ico

HEADERS += ../src/Cookie.h\
            ../src/EditorConfig.h\
            ../src/Extender.h\
            ../src/FilePath.h\
            ../src/FileWorker.h\
            ../src/GUI.h\
            ../src/IFaceTable.h\
            ../src/JobQueue.h\
            ../src/LexillaLibrary.h\
            ../src/LuaExtension.h\
            ../src/MatchMarker.h\
            ../src/MultiplexExtension.h\
            ../src/PropSetFile.h\
            ../src/ScintillaCall.h\
            ../src/ScintillaMessages.h\
            ../src/ScintillaTypes.h\
            ../src/ScintillaWindow.h\
            ../src/SciTE.h\
            ../src/SciTEBase.h\
            ../src/SciTEKeys.h\
            ../src/scite_lua_win.h\
            ../src/StringHelpers.h\
            ../src/StringList.h\
            ../src/StripDefinition.h\
            ../src/StyleDefinition.h\
            ../src/StyleWriter.h\
            ../src/Utf8_16.h\
            ../src/Worker.h

SOURCES +=  ../src/Cookie.cxx\
            ../src/Credits.cxx\
            ../src/EditorConfig.cxx\
            ../src/ExportHTML.cxx\
            ../src/ExportPDF.cxx\
            ../src/ExportRTF.cxx\
            ../src/ExportTEX.cxx\
            ../src/ExportXML.cxx\
            ../src/FilePath.cxx\
            ../src/FileWorker.cxx\
            ../src/IFaceTable.cxx\
            ../src/JobQueue.cxx\
            ../src/LexillaLibrary.cxx\
            ../src/LuaExtension.cxx\
            ../src/MatchMarker.cxx\
            ../src/MultiplexExtension.cxx\
            ../src/PropSetFile.cxx\
            ../src/ScintillaCall.cxx\
            ../src/ScintillaWindow.cxx\
            ../src/SciTEBase.cxx\
            ../src/SciTEBuffers.cxx\
            ../src/SciTEIO.cxx\
            ../src/SciTEProps.cxx\
            ../src/StringHelpers.cxx\
            ../src/StringList.cxx\
            ../src/StyleDefinition.cxx\
            ../src/StyleWriter.cxx\
            ../src/Utf8_16.cxx\
            ../lua/src/lapi.c\
            ../lua/src/lauxlib.c\
            ../lua/src/lbaselib.c\
            ../lua/src/lbitlib.c\
            ../lua/src/lcode.c\
            ../lua/src/lcorolib.c\
            ../lua/src/lctype.c\
            ../lua/src/ldblib.c\
            ../lua/src/ldebug.c\
            ../lua/src/ldo.c\
            ../lua/src/ldump.c\
            ../lua/src/lfunc.c\
            ../lua/src/lgc.c\
            ../lua/src/linit.c\
            ../lua/src/liolib.c\
            ../lua/src/llex.c\
            ../lua/src/lmathlib.c\
            ../lua/src/lmem.c\
            ../lua/src/loadlib.c\
            ../lua/src/lobject.c\
            ../lua/src/lopcodes.c\
            ../lua/src/loslib.c\
            ../lua/src/lparser.c\
            ../lua/src/lstate.c\
            ../lua/src/lstring.c\
            ../lua/src/lstrlib.c\
            ../lua/src/ltable.c\
            ../lua/src/ltablib.c\
            ../lua/src/ltm.c\
            ../lua/src/lundump.c\
            ../lua/src/lutf8lib.c\
            ../lua/src/lvm.c\
            ../lua/src/lzio.c

INCLUDEPATH += ../lua/src ../src ../../scintilla/qt/ScintillaEdit ../../scintilla/qt/ScintillaEditBase ../../scintilla/include ../../scintilla/src ../../scintilla/lexilla/src ../../scintilla/lexlib

LIBS += -L$$OUT_PWD/../../scintilla/bin/ -lScintillaEditBase

RESOURCES += sciteqt.qrc

#DESTPATH = $$[QT_INSTALL_EXAMPLES]/qml/tutorials/extending-qml/chapter1-basics
#target.path = $$DESTPATH

#qml.files = *.qml
#qml.path = $$DESTPATH

#INSTALLS += target qml

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
