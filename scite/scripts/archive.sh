# Up to parent directory of scite
cd ../..

# Archive both Scintilla and SciTE to scite-clean
rm -rf scite-clean
hg archive --repository scintilla scite-clean/scintilla
hg archive --repository scite scite-clean/scite

# tar both scintilla and scite into scite.tgz
rm -f scite.tgz
(
cd scite-clean || exit
tar -czf ../scite.tgz scintilla scite
)

# Remove temporary directory
rm -rf scite-clean
