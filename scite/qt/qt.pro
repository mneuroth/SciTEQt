QT += qml quick quickcontrols2 widgets printsupport svg

android {
    QT += purchasing
}

TARGET = sciteqt

DEFINES += QT_QML

CONFIG += c++1z

wasm {
    QMAKE_CXXFLAGS += "-s ASYNCIFY=1 -s ASSERTIONS=1"
    #QMAKE_CXXFLAGS += "-s ASYNCIFY -s ASSERTIONS=1"
    #QMAKE_LFLAGS_RELEASE += "-s ASYNCIFY"
    #QMAKE_LFLAGS_APP += "-s ASYNCIFY"
    #QMAKE_LFLAGS_SHLIB += "-s ASYNCIFY"
    #QMAKE_CXXFLAGS += "-s ASSERTIONS=1"
    # maybe see: https://stackoverflow.com/questions/18663331/how-to-check-the-selected-version-of-qt-in-a-pro-file
    #versionAtLeast(QT_VERSION, 5.15.1): QMAKE_CXXFLAGS += "-s ASSERTIONS=1"
    #versionAtMost(QT_VERSION, 5.14.2): QMAKE_CXXFLAGS += "-s ASYNCIFY=1 -s ASSERTIONS=1"
}

include(qhtml5file/qhtml5file.pri)

HEADERS += applicationdata.h\
           ../../scintilla/lexilla/src/Lexilla.h \
           applicationui.hpp \
           findinfiles.h \
           sciteqtenvironmentforjavascript.h \
           scriptexecution.h \
           shareutils.hpp \
           storageaccess.h \
           sciteqt.h
SOURCES += applicationdata.cpp\
           GUIQt.cpp\
           ../../scintilla/lexilla/src/Lexilla.cxx\
           findinfiles.cpp \
           sciteqtenvironmentforjavascript.cpp \
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
# TODO: copy example files in user directory ?
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\demo.js\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\plotdemo.js\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)( if not exist \"$${PWD}/dist/packages/org.scintilla.sciteqt/data/localisations\" $${QMAKE_MKDIR} \"$${PWD}/dist/packages/org.scintilla.sciteqt/data/localisations\") )
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\translations\\*.*\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data/localisations\")
    QMAKE_POST_LINK +=$$quote($$escape_expand(\n\t)$${QMAKE_COPY} \"$${PWD}\\..\\doc\\*.*\" \"$${PWD}/dist/packages/org.scintilla.sciteqt/data\")
}

macx {
    properties.path = Contents/MacOS
    properties.files += ./SciTEUser.properties
    properties.files += ./SciTEGlobal.properties
    properties.files += ../src/SciTE.properties
    properties.files += ../src/abbrev.properties

    localisations.path = Contents/MacOS/localisations
    localisations.files += translations/locale.de.properties
    localisations.files += translations/locale.nl.properties
    localisations.files += translations/locale.fr.properties
    localisations.files += translations/locale.es.properties
    localisations.files += translations/locale.pt_PT.properties
    localisations.files += translations/locale.it.properties
    localisations.files += translations/locale.ru.properties
    localisations.files += translations/locale.ja.properties
    localisations.files += translations/locale.ko_KR.properties
    localisations.files += translations/locale.zh_t.properties
    localisations.files += translations/locale.ar.properties
    localisations.files += translations/locale.pl.properties
    localisations.files += translations/locale.da.properties
    localisations.files += translations/locale.tr.properties
    localisations.files += translations/locale.id.properties
    localisations.files += translations/locale.el.properties
    localisations.files += translations/locale.fi.properties
    localisations.files += translations/locale.nb.properties
    localisations.files += translations/locale.hu.properties

    QMAKE_BUNDLE_DATA += localisations
    QMAKE_BUNDLE_DATA += properties
    INSTALLS += localisations
    INSTALLS += properties

    ICON = SciteQt_512x512.icns
    QMAKE_INFO_PLIST = dist/Info.plist
    QT += macextras
}

ARCH_PATH = x86

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


    deployment100.files=demo.js
    deployment100.path=/assets/files
    deployment101.files=plotdemo.js
    deployment101.path=/assets/files

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
    deployment12.files=translations/locale.ru.properties
    deployment12.path=/assets/files
    deployment13.files=translations/locale.ja.properties
    deployment13.path=/assets/files
    deployment14.files=translations/locale.ko_KR.properties
    deployment14.path=/assets/files
    deployment15.files=translations/locale.zh_t.properties
    deployment15.path=/assets/files
    deployment16.files=translations/locale.ar.properties
    deployment16.path=/assets/files
    deployment17.files=translations/locale.pl.properties
    deployment17.path=/assets/files
    deployment18.files=translations/locale.cs.properties
    deployment18.path=/assets/files
    deployment19.files=translations/locale.da.properties
    deployment19.path=/assets/files
    deployment40.files=translations/locale.tr.properties
    deployment40.path=/assets/files
    deployment41.files=translations/locale.id.properties
    deployment41.path=/assets/files
    deployment42.files=translations/locale.el.properties
    deployment42.path=/assets/files
    deployment43.files=translations/locale.fi.properties
    deployment43.path=/assets/files
    deployment44.files=translations/locale.nb.properties
    deployment44.path=/assets/files
    deployment45.files=translations/locale.hu.properties
    deployment45.path=/assets/files
    deployment46.files=translations/locale.sv.properties
    deployment46.path=/assets/files
    deployment47.files=translations/locale.sl.properties
    deployment47.path=/assets/files
    deployment48.files=translations/locale.ro.properties
    deployment48.path=/assets/files
    deployment49.files=translations/locale.bg.properties
    deployment49.path=/assets/files
    deployment50.files=translations/locale.th.properties
    deployment50.path=/assets/files
    deployment51.files=translations/locale.uk.properties
    deployment51.path=/assets/files
    deployment52.files=translations/locale.et.properties
    deployment52.path=/assets/files
    deployment53.files=translations/locale.sr.properties
    deployment53.path=/assets/files
    deployment54.files=translations/locale.ms.properties
    deployment54.path=/assets/files

# TODO: this file, applicationdata.cpp, applicationdata.h, wasmres.qrc
# TODO: af, bg, ca, cs, cy, da, el, eo, et, eu, fi, gl, hu, id, kk, ms, nb, pl, ro, sl, sr, sv, sw_KE, th, tr, uk
# afrikaans, -bulgarisch-, catalan, -czech-, welsh, -danish-, -greek-, esparanto, -estonia-, basq (eu), -finnish-, -hungarian-, -indonesian-, kazakh, -malay-, -norwegian-, -polish-, -romanian-, -slovenian-, -serbian-, -swedish (sv)-, swahili, -thai-, -turkish-, -ukrainian-

    deployment20.files=qt_de.qm
    deployment20.path=/assets/files
    deployment21.files=qt_fr.qm
    deployment21.path=/assets/files
    deployment22.files=qt_es.qm
    deployment22.path=/assets/files
    deployment23.files=qt_it.qm
    deployment23.path=/assets/files
    deployment24.files=qt_ru.qm
    deployment24.path=/assets/files
    deployment25.files=qt_ja.qm
    deployment25.path=/assets/files
    deployment26.files=qt_ko.qm
    deployment26.path=/assets/files
    deployment27.files=qt_zh_tw.qm
    deployment27.path=/assets/files
    deployment28.files=qt_ar.qm
    deployment28.path=/assets/files
    deployment29.files=qt_pl.qm
    deployment29.path=/assets/files
    deployment30.files=qt_cs.qm
    deployment30.path=/assets/files
    deployment31.files=qt_da.qm
    deployment31.path=/assets/files
    deployment32.files=qt_tr.qm
    deployment32.path=/assets/files
    //deployment33.files=qt_id.qm
    //deployment33.path=/assets/files
    //deployment34.files=qt_el.qm
    //deployment34.path=/assets/files
    deployment35.files=qt_fi.qm
    deployment35.path=/assets/files
    //deployment36.files=qt_hu.qm
    //deployment36.path=/assets/files
    deployment37.files=qt_sv.qm
    deployment37.path=/assets/files
    deployment38.files=qt_sl.qm
    deployment38.path=/assets/files
    //deployment39.files=qt_ro.qm
    //deployment39.path=/assets/files
    deployment60.files=qt_bg.qm
    deployment60.path=/assets/files
    //deployment61.files=qt_th.qm
    //deployment61.path=/assets/files
    deployment62.files=qt_uk.qm
    deployment62.path=/assets/files
    //deployment63.files=qt_et.qm
    //deployment63.path=/assets/files
    //deployment64.files=qt_sr.qm
    //deployment64.path=/assets/files
    //deployment65.files=qt_ms.qm
    //deployment65.path=/assets/files

# for qt available:
#qt_ar.qm
#qt_bg.qm
#qt_ca.qm
#qt_cs.qm
#qt_da.qm
#qt_de.qm
#qt_en.qm
#qt_es.qm
#qt_fi.qm
#qt_fr.qm
#qt_gd.qm
#qt_he.qm
#qt_hu.qm
#qt_it.qm
#qt_ja.qm
#qt_ko.qm
#qt_lv.qm
#qt_pl.qm
#qt_ru.qm
#qt_sk.qm
#qt_tr.qm
#qt_uk.qm
#qt_zh_TW.qm

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
    INSTALLS += deployment12
    INSTALLS += deployment13
    INSTALLS += deployment14
    INSTALLS += deployment15
    INSTALLS += deployment16
    INSTALLS += deployment17
    INSTALLS += deployment18
    INSTALLS += deployment19

    INSTALLS += deployment20
    INSTALLS += deployment21
    INSTALLS += deployment22
    INSTALLS += deployment23
    INSTALLS += deployment24
    INSTALLS += deployment25
    INSTALLS += deployment26
    INSTALLS += deployment27
    INSTALLS += deployment28
    INSTALLS += deployment29
    INSTALLS += deployment30
    INSTALLS += deployment31
    INSTALLS += deployment32
    //INSTALLS += deployment33
    //INSTALLS += deployment34
    INSTALLS += deployment35
    //INSTALLS += deployment36
    INSTALLS += deployment37
    INSTALLS += deployment38
    INSTALLS += deployment39

    INSTALLS += deployment40
    INSTALLS += deployment41
    INSTALLS += deployment42
    INSTALLS += deployment43
    INSTALLS += deployment44
    INSTALLS += deployment45
    INSTALLS += deployment46
    INSTALLS += deployment47
    INSTALLS += deployment48
    INSTALLS += deployment49
    INSTALLS += deployment50
    INSTALLS += deployment51
    INSTALLS += deployment52
    INSTALLS += deployment53
    INSTALLS += deployment54

    INSTALLS += deployment60
    //INSTALLS += deployment61
    INSTALLS += deployment62
    //INSTALLS += deployment63
    //INSTALLS += deployment64
    //INSTALLS += deployment65

    INSTALLS += deployment100
    INSTALLS += deployment101
}

RESOURCES += sciteqt.qrc

wasm {
    RESOURCES += wasmres.qrc
}

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

#ANDROID_ABIS = armeabi-v7a arm64-v8a x86 x86_64
ANDROID_ABIS = arm64-v8a

LIBS += -L$$OUT_PWD/../../scintilla/bin-$${ARCH_PATH}/ -lScintillaEditBase

