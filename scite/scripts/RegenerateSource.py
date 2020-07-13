#!/usr/bin/env python3
# RegenerateSource.py - implemented 2013 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.

# Regenerate the SciTE source files that list all the lexers and all the
# properties files.
# Should be run whenever a new lexer is added or removed.
# Requires Python 3.6 or later
# Most files are regenerated in place with templates stored in comments.
# The VS .NET project file is generated into a different file as the
# VS .NET environment will not retain comments when modifying the file.
# The format of generation comments is documented in FileGenerator.py.

# Regenerates Scintilla files by calling LexGen.RegenerateAll

import pathlib, sys

sciteBase = pathlib.Path(__file__).resolve().parent.parent
baseDirectory = sciteBase.parent
sys.path.append(str(baseDirectory / "scintilla" / "scripts"))

from FileGenerator import Generate, Regenerate, UpdateLineInFile, ReplaceREInFile
import ScintillaData
import LexGen
import lexilla.scripts.LexillaGen
import IFaceTableGen
import commandsdoc

sys.path.append(str(sciteBase))

import win32.AppDepGen
import gtk.AppDepGen

neutralEncoding = "windows-1252"

def UpdateVersionNumbers(sci, pathSciTE):
    pathHeader = pathSciTE / "src" / "SciTE.h"
    UpdateLineInFile(pathHeader,
        '#define VERSION_SCITE',
        '#define VERSION_SCITE "' + sci.versionDotted + '"')
    UpdateLineInFile(pathHeader,
        "#define VERSION_WORDS",
        "#define VERSION_WORDS " + sci.versionCommad)
    UpdateLineInFile(pathHeader,
        '#define COPYRIGHT_DATES',
        '#define COPYRIGHT_DATES "December 1998-' + sci.myModified + '"')
    UpdateLineInFile(pathHeader,
        '#define COPYRIGHT_YEARS',
        '#define COPYRIGHT_YEARS "1998-' + sci.yearModified + '"')

    pathDownload = pathSciTE / "doc" / "SciTEDownload.html"
    UpdateLineInFile(pathDownload,
        "       Release",
        "       Release " + sci.versionDotted)
    ReplaceREInFile(pathDownload,
        r"/www.scintilla.org/([a-zA-Z]+)\d\d\d",
        r"/www.scintilla.org/\g<1>" +  sci.version)
    ReplaceREInFile(pathDownload,
        r"/www.scintilla.org/(wscite32_)\d\d\d",
        r"/www.scintilla.org/\g<1>" +  sci.version)
    ReplaceREInFile(pathDownload,
        r"/www.scintilla.org/(Sc32_)\d\d\d",
        r"/www.scintilla.org/\g<1>" +  sci.version)

    pathMain = pathSciTE / "doc" / "SciTE.html"
    UpdateLineInFile(pathMain,
        '          <font color="#FFCC99" size="3"> Release version',
        '          <font color="#FFCC99" size="3"> Release version ' + \
        sci.versionDotted + '<br />')
    UpdateLineInFile(pathMain,
        '           Site last modified',
        '           Site last modified ' + sci.mdyModified + '</font>')
    UpdateLineInFile(pathMain,
        '    <meta name="Date.Modified"',
        '    <meta name="Date.Modified" content="' + sci.dateModified + '" />')

def OctalEscape(s):
    result = []
    for char in s:
        # Python 3.x, s is a byte string
        if char < 128:
            result.append(chr(char))
        else:
            result.append("\%o" % char)
    return ''.join(result)

def UpdateEmbedded(pathSciTE, propFiles):
    propFilesSpecial = ["SciTEGlobal.properties", "abbrev.properties"]
    propFilesAll = propFilesSpecial + propFiles
    linesEmbedded = []
    for pf in propFilesAll:
        fullPath = pathSciTE / "src" / pf
        with fullPath.open(encoding=neutralEncoding) as fi:
            fileBase = pathlib.Path(pf).stem
            if pf not in propFilesSpecial:
                linesEmbedded.append("\n" + "module " + fileBase + "\n")
            for line in fi:
                if not line.startswith("#"):
                    linesEmbedded.append(line)
            if not linesEmbedded[-1].endswith("\n"):
                linesEmbedded[-1] += "\n"
    textEmbedded = "".join(linesEmbedded)
    pathEmbedded = pathSciTE / "src" / "Embedded.properties"
    original = pathEmbedded.read_text(encoding=neutralEncoding)
    if textEmbedded != original:
        pathEmbedded.write_text(textEmbedded, encoding=neutralEncoding)
        print("Changed %s" % pathEmbedded)

def RegenerateAll():
    root="../../"

    sci = ScintillaData.ScintillaData(baseDirectory / "scintilla")

    pathSciTE = sciteBase

    # Generate HTML to document each property
    # This is done because tags can not be safely put inside comments in HTML
    documentProperties = list(sci.propertyDocuments.keys())
    ScintillaData.SortListInsensitive(documentProperties)
    propertiesHTML = []
    for k in documentProperties:
        propertiesHTML.append("\t<tr id='property-%s'>\n\t<td>%s</td>\n\t<td>%s</td>\n\t</tr>" %
            (k, k, sci.propertyDocuments[k]))

    # Find all the SciTE properties files
    otherProps = [
        "abbrev.properties",
        "Embedded.properties",
        "SciTEGlobal.properties",
        "SciTE.properties"]
    propFilePaths = list((pathSciTE / "src").glob("*.properties"))
    ScintillaData.SortListInsensitive(propFilePaths)
    propFiles = [f.name for f in propFilePaths if f.name not in otherProps]
    ScintillaData.SortListInsensitive(propFiles)

    UpdateEmbedded(pathSciTE, propFiles)
    Regenerate(pathSciTE / "win32" / "makefile", "#", propFiles)
    Regenerate(pathSciTE / "win32" / "scite.mak", "#", propFiles)
    Regenerate(pathSciTE / "src" / "SciTEProps.cxx", "//", sci.lexerProperties)
    Regenerate(pathSciTE / "doc" / "SciTEDoc.html", "<!--", propertiesHTML)
    credits = [OctalEscape(c.encode("utf-8")) for c in sci.credits]
    Regenerate(pathSciTE / "src" / "Credits.cxx", "//", credits)

    win32.AppDepGen.Generate()
    gtk.AppDepGen.Generate()

    UpdateVersionNumbers(sci, pathSciTE)

LexGen.RegenerateAll(sciteBase.parent / "scintilla")
lexilla.scripts.LexillaGen.RegenerateAll(sciteBase.parent / "scintilla")
RegenerateAll()
IFaceTableGen.RegenerateAll()
commandsdoc.RegenerateAll()
