QT += quick quickcontrols2 widgets printsupport svg

TARGET = sciteqt

DEFINES += QT_QML

CONFIG += c++1z

wasm {
    QMAKE_CXXFLAGS += "-s ASYNCIFY=1 -s ASSERTIONS=1"
}

include(qhtml5file/qhtml5file.pri)

HEADERS += applicationdata.h\
           ../../scintilla/lexilla/src/Lexilla.h \
           applicationui.hpp \
           findinfiles.h \
           scriptexecution.h \
           shareutils.hpp \
           storageaccess.h \
           sciteqt.h
SOURCES += applicationdata.cpp\
           GUIQt.cpp\
           ../../scintilla/lexilla/src/Lexilla.cxx\
           findinfiles.cpp \
           scriptexecution.cpp \
           main.cpp \
           applicationui.cpp \
           shareutils.cpp \
           storageaccess.cpp \
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

unix {
    LIBS += -ldl
}
macx {
    LIBS += -ldl
}
wasm {
    # use this command in output directory to run application: emrun --browser=firefox sciteqt.html
    LIBS += -ldl -s ERROR_ON_UNDEFINED_SYMBOLS=0
}

CONFIG(debug, debug|release) {
    DEBUG_OR_RELEASE = debug
}  else {
    DEBUG_OR_RELEASE = release
}

win32 {
    QMAKE_POST_LINK +=$$quote($${QMAKE_COPY} \"$${OUT_PWD}\\$${DEBUG_OR_RELEASE}\\$${TARGET}.exe\" \"$${PWD}\\dist\\packages\\org.scintilla.sciteqt\\data\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\..\\win32\\SciBall.ico\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data/sciteqt_256x256.ico\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\..\\gtk\\Sci48M.png\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data/sciteqt_512x512.png\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\..\\src\\SciTE.properties\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\..\\qt\\SciTEGlobal.properties\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\..\\src\\abbrev.properties\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\SciTEUser.properties\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)( if not exist \"$${PWD}/dist/packages/org.scintilla.sciteqt/data/localisations\" $${QMAKE_MKDIR} \"$${PWD}/dist/packages/org.scintilla.sciteqt/data/localisations\") )
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\translations\\*.*\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data/localisations\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\..\\doc\\*.*\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data\")
}

android {
    SOURCES += android/androidshareutils.cpp

    HEADERS += android/androidshareutils.hpp

    QT += androidextras

    equals(ANDROID_TARGET_ARCH, arm64-v8a) {
        ARCH_PATH = arm64
    }
    equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
        ARCH_PATH = arm
    }
    equals(ANDROID_TARGET_ARCH, armeabi) {
        ARCH_PATH = arm
    }
    equals(ANDROID_TARGET_ARCH, x86)  {
        ARCH_PATH = x86
    }
    equals(ANDROID_TARGET_ARCH, x86_64)  {
        ARCH_PATH = x64
    }
    equals(ANDROID_TARGET_ARCH, mips)  {
        ARCH_PATH = mips
    }
    equals(ANDROID_TARGET_ARCH, mips64)  {
        ARCH_PATH = mips64
    }

    deployment1.files=../src/SciTE.properties
    deployment1.path=/assets/files
    deployment2.files=../src/abbrev.properties
    deployment2.path=/assets/files
    deployment3.files=SciTEGlobal.properties
    deployment3.path=/assets/files
    deployment4.files=SciTEUser.properties
    deployment4.path=/assets/files
    deployment5.files=../doc/SciTEDoc.html
    deployment5.path=/assets/files
    deployment6.files=translations/locale.de.properties
    deployment6.path=/assets/files
    deployment7.files=translations/locale.nl.properties
    deployment7.path=/assets/files
    deployment8.files=translations/locale.fr.properties
    deployment8.path=/assets/files
    deployment9.files=translations/locale.es.properties
    deployment9.path=/assets/files
    deployment10.files=translations/locale.pt_PT.properties
    deployment10.path=/assets/files
    deployment11.files=translations/locale.it.properties
    deployment11.path=/assets/files

    INSTALLS += deployment1
    INSTALLS += deployment2
    INSTALLS += deployment3
    INSTALLS += deployment4
    INSTALLS += deployment5
    INSTALLS += deployment6
    INSTALLS += deployment7
    INSTALLS += deployment8
    INSTALLS += deployment9
    INSTALLS += deployment10
    INSTALLS += deployment11
}

RESOURCES += sciteqt.qrc

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/res/xml/filepaths.xml \
    android/src/org/scintilla/activity/sharex/QShareActivity.java \
    android/src/org/scintilla/utils/QSharePathResolver.java \
    android/src/org/scintilla/utils/QShareUtils.java \
    android/src/org/scintilla/utils/QStorageAccess.java

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

ANDROID_ABIS = arm64-v8a
