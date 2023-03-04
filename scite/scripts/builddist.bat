:: builddist.bat
:: Build all of Lexilla, Scintilla and SciTE for distribution and place into a subdirectory called upload%SCITE_VERSION%
:: This batch file is distributed inside scite but is commonly copied out into its own working directory

:: Requires hg and zip to be in the path. nmake, cl, and link are found by vcvars*.bat

:: Define local paths here

:: Running after Visual C++ set up with vcvars64 or vcvars32 will fail to build for XP
IF DEFINED VisualStudioVersion (ECHO VisualStudio is active && exit)

set "MSVC_DIRECTORY=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"
set "MSVC17_DIRECTORY=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build"
set REPOSITORY_DIRECTORY=..\hg

:: Discover the Scintilla version as that is used in file and directory names
for /F %%i IN (%REPOSITORY_DIRECTORY%\lexilla\version.txt) do set "LEXILLA_VERSION=%%i"
for /F %%i IN (%REPOSITORY_DIRECTORY%\scintilla\version.txt) do set "SCINTILLA_VERSION=%%i"
for /F %%i IN (%REPOSITORY_DIRECTORY%\scite\version.txt) do set "SCITE_VERSION=%%i"
set "UPLOAD_DIRECTORY=upload%SCITE_VERSION%"

:: Clean then copy from archive into scintilla and scite subdirectories

rd /s/q lexilla scintilla scite
del/q Sc1.exe

git clone %REPOSITORY_DIRECTORY%/lexilla lexilla
hg archive -R %REPOSITORY_DIRECTORY%/scintilla scintilla
hg archive -R %REPOSITORY_DIRECTORY%/scite scite

:: Create source archives
pushd lexilla
call zipsrc
popd
hg archive -R %REPOSITORY_DIRECTORY%/scintilla scintilla.zip
pushd scite
call zipsrc
popd

:: Build the 64-bit executables
setlocal
call "%MSVC_DIRECTORY%\vcvars64.bat"

pushd lexilla
pushd src
nmake -f lexilla.mak SUPPORT_XP=1
popd
popd

pushd scintilla
pushd win32
nmake -f scintilla.mak SUPPORT_XP=1
popd
del/q bin\*.pdb
popd

pushd scite
pushd win32
nmake -f scite.mak SUPPORT_XP=1
popd
copy bin\Sc1.exe ..\Sc1.exe
call zipwscite
popd

:: Copy into correctly numbered upload directory
echo %UPLOAD_DIRECTORY%
mkdir upload%SCITE_VERSION%
copy lexilla.zip %UPLOAD_DIRECTORY%\lexilla%LEXILLA_VERSION%.zip
copy scintilla.zip %UPLOAD_DIRECTORY%\scintilla%SCINTILLA_VERSION%.zip
copy scite.zip %UPLOAD_DIRECTORY%\scite%SCITE_VERSION%.zip
copy wscite.zip %UPLOAD_DIRECTORY%\wscite%SCITE_VERSION%.zip
copy Sc1.exe %UPLOAD_DIRECTORY%\Sc%SCITE_VERSION%.exe

:: Clean all
pushd scite
call delbin
popd
pushd scintilla
call delbin
popd
pushd lexilla
call delbin
popd

endlocal

:: Build the 32-bit executables with MSVC 2017 as it suports XP
call "%MSVC17_DIRECTORY%\vcvars32.bat"

pushd lexilla
pushd src
nmake -f lexilla.mak SUPPORT_XP=1
popd
popd

pushd scintilla
pushd win32
nmake -f scintilla.mak SUPPORT_XP=1
popd
del/q bin\*.pdb
popd

pushd scite
pushd win32
nmake -f scite.mak SUPPORT_XP=1
popd
move bin\SciTE.exe bin\SciTE32.exe
copy bin\Sc1.exe ..\Sc1.exe
call zipwscite
popd

:: Copy into correctly numbered upload directory
copy wscite.zip %UPLOAD_DIRECTORY%\wscite32_%SCITE_VERSION%.zip
copy Sc1.exe %UPLOAD_DIRECTORY%\Sc32_%SCITE_VERSION%.exe

:: Clean all
pushd scite
call delbin
popd
pushd scintilla
call delbin
popd
pushd lexilla
call delbin
popd

:: scintilla and scite directories remain so can be examined if something went wrong