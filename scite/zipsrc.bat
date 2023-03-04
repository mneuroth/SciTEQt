cd ..
del/q scite.zip
zip scite.zip lexilla\*.* lexilla\*\*.* lexilla\*\*\*.* lexilla\*\*\*\*.* lexilla\*\*\*\*\*.*  scintilla\*.* scintilla\*\*.* scintilla\*\*\*.* scintilla\*\*\*\*.* scintilla\*\*\*\*\*.* scite\*.* scite\*\*.* scite\*\*\*.* scite\*\*\*\*.* -x *.o -x *.obj -x *.a -x *.lib -x *.dll -x *.exe -x *.pdb -x *.i -x *.res -x *.exp -x *.ncb -x *.sbr -x *.bsc -x *.tlog -x *.ilk -x *.ipch -x *.idb -x *.sdf -x *.dpsession -x *.bak
zip scite.zip -d scintilla/.hg/*
zip scite.zip -d lexilla/.hg/*
zip scite.zip -d scite/.hg/*
cd scite
