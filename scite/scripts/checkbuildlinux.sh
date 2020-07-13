# Script to build Scintilla and SciTE for Linux with most supported build files.
# Both gcc and clang are used for both GTK+ 2 and 3 and the clang static analyzer
# run for GTK+2.
# All three Qt libraries are built with gcc.
# Current directory should be scite\scripts before running.

# Pre-requisite packages on Ubuntu:
# sudo apt-get install --yes g++
# sudo apt-get install --yes libgtk-3-dev
# sudo apt-get install --yes libgtk2.0-dev
# sudo apt-get install --yes clang
# sudo apt-get install --yes qtcreator
# sudo apt-get install --yes python-dev
# sudo apt-get install --yes libshiboken-dev
# sudo apt-get install --yes shiboken
# sudo apt-get install --yes libpyside-dev
# sudo apt-get install --yes python-pyside
# sudo apt-get install --yes cppcheck

# Pre-requisite packages on Fedora:
# sudo yum install -y gcc-c++
# sudo yum install -y gtk3-devel
# sudo yum install -y gtk2-devel
# sudo yum install -y clang
# sudo yum install -y qt-creator
# sudo yum install -y python-devel
# sudo yum install -y shiboken-devel
# sudo yum install -y python-pyside-devel

# On Fedora 17, qmake is called qmake-qt4 so sepbuild.py should probe for correct name.
# There are also problems with clang failing in the g++ 4.7 headers.

# Turn off deprecation warnings if not interested:
#export CXXFLAGS="-Wno-deprecated-declarations -D GLIB_DISABLE_DEPRECATION_WARNINGS"

# Run commands in parallel up to number of processors
JOBS="-j $(getconf _NPROCESSORS_ONLN)"

cd ../..

# ************************************************************
# Target 1: gcc build for GTK+ 2
(
cd scintilla/test/unit || exit
make clean
make $JOBS test
make clean
)

(
cd scintilla/lexilla/src || exit
make clean
make $JOBS
cd ..
cd test || exit
make clean
make test
make clean
)

(
cd scintilla/gtk || exit
make clean
make $JOBS
)

(
cd scite/gtk || exit
make clean
make $JOBS
)

# ************************************************************
# Target 2: gcc build for GTK+ 3
(
cd scintilla/gtk || exit
make clean
make $JOBS GTK3=1
)

(
cd scite/gtk || exit
make clean
make $JOBS GTK3=1
)

# ************************************************************
# Target 3: Qt builds
# Requires Qt development libraries and qmake to be installed

# Find best available qmake
QMAKENAME=""
if hash qmake-qt5 2>/dev/null; then
	QMAKENAME="qmake-qt5"
elif hash qmake 2>/dev/null; then
	QMAKENAME="qmake -qt=5"
elif hash qmake-qt4 2>/dev/null; then
	QMAKENAME="qmake-qt4"
fi

(
cd scintilla/qt || exit

(
cd ScintillaEditBase || exit
$QMAKENAME
make clean
make $JOBS
make distclean
)

(
cd ScintillaEdit || exit
python3 WidgetGen.py
$QMAKENAME
make clean
make $JOBS
make distclean
)

(
cd ScintillaEditPy || exit
python2 sepbuild.py
cd ../../test || exit
python2 simpleTests.py
python2 lexTests.py
python2 performanceTests.py
cd ../qt/ScintillaEditPy || exit
python2 sepbuild.py --clean
)

)

# ************************************************************
# Target 4: clang build for GTK+ 2
(
cd scintilla/gtk || exit
make clean
make $JOBS CLANG=1
)

(
cd scite/gtk || exit
make clean
make $JOBS CLANG=1
)

# ************************************************************
# Target 5: clang build for GTK+ 3
(
cd scintilla/gtk || exit
make clean
make $JOBS CLANG=1 GTK3=1
)

(
cd scite/gtk || exit
make clean
make $JOBS CLANG=1 GTK3=1
)

# ************************************************************
# Target 6: clang analyze for GTK+ 2
(
cd scintilla/gtk || exit
make clean
make $JOBS analyze
)

(
cd scite/gtk || exit
make clean
make $JOBS analyze
make clean
cd ../..
cd scintilla/gtk || exit
make clean
)

# ************************************************************
# Target 7: cppcheck static checker
# Disabled as there are false warnings and some different style choices
cppcheck --enable=all --max-configs=100 --suppressions-list=scintilla/cppcheck.suppress -I scintilla/src -I scintilla/include -I scintilla/lexlib -I scintilla/qt/ScintillaEditBase --template=gcc --quiet scintilla
cppcheck --enable=all --max-configs=100 --suppressions-list=scite/cppcheck.suppress -I scite/src -I scintilla/include -I scite/lua/src -Ulua_assert -DPATH_MAX=260 --template=gcc --quiet scite
