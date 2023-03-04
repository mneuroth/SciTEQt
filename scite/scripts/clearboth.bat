@rem clearboth.bat - script to clear both scintilla and scite
@rem directory trees of all compiler output files.
@rem Current directory must be parent of scintilla and scite before running.
@pushd lexilla
@call delbin
@popd
@pushd scintilla
@call delbin
@popd
@pushd scite
@call delbin
@popd
