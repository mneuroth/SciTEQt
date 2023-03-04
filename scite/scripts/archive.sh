# Up to parent directory of scite
cd ../..

# Archive Lexilla, Scintilla, and SciTE to scite-clean
rm -rf scite-clean
git clone lexilla scite-clean/lexilla
rm -rf scite-clean/lexilla/.git
hg archive --repository scintilla scite-clean/scintilla
hg archive --repository scite scite-clean/scite

# tar lexilla, scintillas and scite into scite.tgz
rm -f scite.tgz
(
cd scite-clean || exit
tar -czf ../scite.tgz lexilla scintilla scite
)

# Remove temporary directory
rm -rf scite-clean
