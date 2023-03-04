#!/usr/bin/env python3
# UpdateSizeInDocs.py
# Update the documented sizes of downloads to match the current release.
# Uses local build directory (../../../arc/upload<version>) on Neil's machine
# so must be modified for other installations.
# Implemented 2019 by Neil Hodgson neilh@scintilla.org
# Requires Python 3.6 or later

import pathlib

downloadHome = "https://www.scintilla.org/"

def ExtractFileName(s):
    pre, _quote, rest = s.partition('"')
    url, _quote, rest = rest.partition('"')
    _domain, _slash, name = url.rpartition('/')
    return name

def FileSizeInMB(filePath):
    size = filePath.stat().st_size
    sizeInM = size / 1024 / 1024
    roundToNearest = round(sizeInM * 10) / 10
    return str(roundToNearest) + "M"

def FileSizesInDirectory(base):
    return {p.name : FileSizeInMB(p) for p in base.glob("*")}

def UpdateFileSizes(scriptsPath):
    sciteRoot = scriptsPath.parent
    scintillaRoot = sciteRoot.parent / "scintilla"
    lexillaRoot = sciteRoot.parent / "lexilla"
    releaseRoot = sciteRoot.parent.parent / "arc"

    uploadDocs = [
        sciteRoot / "doc" / "SciTEDownload.html",
        scintillaRoot / "doc" / "ScintillaDownload.html",
        lexillaRoot / "doc" / "LexillaDownload.html",
    ]

    version = (sciteRoot / "version.txt").read_text().strip()
    currentRelease = releaseRoot / ("upload" + version)
    fileSizes = FileSizesInDirectory(currentRelease)
    if not fileSizes:
        print("No files in", currentRelease)

    for docFileName in uploadDocs:
        outLines = ""
        changes = False
        with docFileName.open() as docFile:
            for line in docFile:
                if downloadHome in line and '(' in line and ')' in line:
                    fileName = ExtractFileName(line)
                    if fileName in fileSizes:
                        pre, bracket, rest = line.partition('(')
                        size, rbracket, end = rest.partition(')')
                        if size != fileSizes[fileName]:
                            line = pre + bracket + fileSizes[fileName] + rbracket + end
                            changes = True
                            print(f"{size} -> {fileSizes[fileName]} {fileName}")
                outLines += line
        if changes:
            print("Updating", docFileName)
            docFileName.write_text(outLines)

if __name__=="__main__":
    UpdateFileSizes(pathlib.Path(__file__).resolve().parent)
