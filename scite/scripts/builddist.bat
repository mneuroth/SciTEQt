:: builddist.bat
:: Build all of Scintilla and SciTE for distribution and place into a subdirectory called upload%SCINTILLA_VERSION%
:: This batch file is distributed inside scite but is commonly copied out into its own working directory

:: Requires hg and zip to be in the path. nmake, cl, and link are found by vcvars*.bat

:: Define local paths here

set "MSVC_DIRECTORY=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build"
set REPOSITORY_DIRECTORY=..\hg

:: Discover the Scintilla version as that is used in file and directory names
for /F %%i IN (%REPOSITORY_DIRECTORY%\scintilla\version.txt) do set "SCINTILLA_VERSION=%%i"
set "UPLOAD_DIRECTORY=upload%SCINTILLA_VERSION%"

:: Clean then copy from archive into scintilla and scite subdirectories

rd /s/q scintilla scite
del/q Sc1.exe

hg archive -R %REPOSITORY_DIRECTORY%/scintilla scintilla
hg archive -R %REPOSITORY_DIRECTORY%/scite scite

:: Create source archives
hg archive -R %REPOSITORY_DIRECTORY%/scintilla scintilla.zip
pushd scite
call zipsrc
popd

:: Build the 64-bit executables
call "%MSVC_DIRECTORY%\vcvars64.bat"

pushd scintilla
pushd lexilla
pushd src
nmake -f lexilla.mak SUPPORT_XP=1
popd
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
nmake -f scite.mak SUPPORT_XP=1 LOAD_SCINTILLA=1
popd
copy bin\Sc1.exe ..\Sc1.exe
call zipwscite
popd

:: Copy into correctly numbered upload directory
echo %UPLOAD_DIRECTORY%
mkdir upload%SCINTILLA_VERSION%
copy scintilla.zip %UPLOAD_DIRECTORY%\scintilla%SCINTILLA_VERSION%.zip
copy scite.zip %UPLOAD_DIRECTORY%\scite%SCINTILLA_VERSION%.zip
copy wscite.zip %UPLOAD_DIRECTORY%\wscite%SCINTILLA_VERSION%.zip
copy Sc1.exe %UPLOAD_DIRECTORY%\Sc%SCINTILLA_VERSION%.exe

:: Clean both
pushd scite
call delbin
popd
pushd scintilla
call delbin
popd

:: Build the 32-bit executables
call "%MSVC_DIRECTORY%\vcvars32.bat"

pushd scintilla
pushd lexilla
pushd src
nmake -f lexilla.mak SUPPORT_XP=1
popd
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
nmake -f scite.mak SUPPORT_XP=1 LOAD_SCINTILLA=1
popd
move bin\SciTE.exe bin\SciTE32.exe
copy bin\Sc1.exe ..\Sc1.exe
call zipwscite
popd

:: Copy into correctly numbered upload directory
copy wscite.zip %UPLOAD_DIRECTORY%\wscite32_%SCINTILLA_VERSION%.zip
copy Sc1.exe %UPLOAD_DIRECTORY%\Sc32_%SCINTILLA_VERSION%.exe

:: Clean both
pushd scite
call delbin
popd
pushd scintilla
call delbin
popd

:: scintilla and scite directories remain so can be examined if something went wrong