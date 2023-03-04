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

import datetime, pathlib, re, sys

sciteBase = pathlib.Path(__file__).resolve().parent.parent
baseDirectory = sciteBase.parent
sciDirectory = baseDirectory / "scintilla"
lexDirectory = baseDirectory / "lexilla"

sys.path.append(str(sciDirectory / "scripts"))
sys.path.append(str(lexDirectory / "scripts"))

from FileGenerator import lineEnd, Generate, Regenerate, UpdateFile, UpdateLineInFile, ReplaceREInFile
import ScintillaData
import LexGen
import LexillaData
import LexillaGen
import IFaceTableGen
import commandsdoc

sys.path.append(str(sciteBase))

import win32.AppDepGen
import gtk.AppDepGen

neutralEncoding = "cp437"	# Each byte value is valid in cp437

def FindCredits(historyFile, removeLinks=True):
    credits = []
    stage = 0
    with historyFile.open(encoding="utf-8") as f:
        for l in f.readlines():
            l = l.strip()
            if stage == 0 and l == "<table>":
                stage = 1
            elif stage == 1 and l == "</table>":
                stage = 2
            if stage == 1 and l.startswith("<td>"):
                credit = l[4:-5]
                if removeLinks and "<a" in l:
                    title, _a, rest = credit.partition("<a href=")
                    urlplus, _bracket, end = rest.partition(">")
                    name = end.split("<")[0]
                    url = urlplus[1:-1]
                    credit = title.strip()
                    if credit:
                        credit += " "
                    credit += name + " " + url
                credits.append(credit)
    return credits

def DottedVersion(version):
    return version[0:-2] + '.' + version[-2] + '.' + version[-1]

class SciTEData:
    def __init__(self, sciteRoot):
        # Discover version information
        self.version = (sciteRoot / "version.txt").read_text().strip()
        self.versionDotted = DottedVersion(self.version)
        self.versionCommad = self.versionDotted.replace(".", ", ") + ', 0'
        self.scintillaVersionFile = sciteRoot / "src" / "scintillaVersion.txt"
        self.scintillaVersion = self.scintillaVersionFile.read_text().strip()
        self.lexillaVersionFile = sciteRoot / "src" / "lexillaVersion.txt"
        self.lexillaVersion = self.lexillaVersionFile.read_text().strip()

        with (sciteRoot / "doc" / "SciTE.html").open() as f:
            self.dateModified = [l for l in f.readlines() if "Date.Modified" in l]\
                [0].split('\"')[3]
            # 20130602
            # index.html, SciTE.html
            dtModified = datetime.datetime.strptime(self.dateModified, "%Y%m%d")
            self.yearModified = self.dateModified[0:4]
            monthModified = dtModified.strftime("%B")
            dayModified = "%d" % dtModified.day
            self.mdyModified = monthModified + " " + dayModified + " " + self.yearModified
            # May 22 2013
            # index.html, SciTE.html
            self.dmyModified = dayModified + " " + monthModified + " " + self.yearModified
            # 22 May 2013
            # ScintillaHistory.html -- only first should change
            self.myModified = monthModified + " " + self.yearModified

        self.credits = FindCredits(sciteRoot / "doc" / "SciTEHistory.html")

def UpdateVersionNumbers(sci, pathSciTE, lexVersion, scintillaVersion):
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
    UpdateLineInFile(pathHeader,
        '#define VERSION_LEXILLA',
        '#define VERSION_LEXILLA "' + lexVersion + '"')
    UpdateLineInFile(pathHeader,
        '#define VERSION_SCINTILLA',
        '#define VERSION_SCINTILLA "' + scintillaVersion + '"')

    pathDownload = pathSciTE / "doc" / "SciTEDownload.html"
    UpdateLineInFile(pathDownload,
        "       Release",
        "       Release " + sci.versionDotted)
    ReplaceREInFile(pathDownload,
        r"/www.scintilla.org/([a-zA-Z]+)\d\d\d",
        r"/www.scintilla.org/\g<1>" +  sci.version,
        0)
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

    UpdateLineInFile(pathSciTE / "doc/SciTEHistory.html",
        '\t\tReleased ',
        '\t\tReleased ' + sci.dmyModified + '.')

def OctalEscape(s):
    result = []
    for char in s:
        # Python 3.x, s is a byte string
        if char < 128:
            result.append(chr(char))
        else:
            result.append(r"\%o" % char)
    return ''.join(result)

def UpdateEmbedded(pathSciTE, propFiles):
    propFilesSpecial = ["SciTEGlobal.properties", "abbrev.properties"]
    propFilesAll = propFilesSpecial + propFiles
    linesEmbedded = []
    for pf in propFilesAll:
        fullPath = pathSciTE / "src" / pf
        with fullPath.open(encoding=neutralEncoding) as fi:
            fileBase = pathlib.Path(pf).stem
            linesEmbedded.append("\n")
            if pf not in propFilesSpecial:
                linesEmbedded.append("\n" + "module " + fileBase + "\n")
            continuation = False
            for line in fi:
                if not continuation and line.startswith("#"):
                    continue
                continuation = line.endswith("\\\n")
                linesEmbedded.append(line)
            if not linesEmbedded[-1].endswith("\n"):
                linesEmbedded[-1] += "\n"
    textEmbedded = "".join(linesEmbedded)
    pathEmbedded = pathSciTE / "src" / "Embedded.properties"
    original = pathEmbedded.read_text(encoding=neutralEncoding)
    if textEmbedded != original:
        pathEmbedded.write_text(textEmbedded, encoding=neutralEncoding)
        print("Changed %s" % pathEmbedded)

def UpdateHistory(pathHistory, credits):
    text = []
    text.append("<table>")
    text.append("      <tr>")
    count = 0
    for c in credits:
        text.append("	<td>" + c + "</td>")
        count = (count + 1) % 4
        if count == 0:
            text.append("      </tr><tr>")
    text.append("      </tr>")
    text.append("    </table>")
    insertion = lineEnd.join(text)
    # [^^] is trying to be . but . doesn't include \n.
    ReplaceREInFile(pathHistory, r"<table>[^\0]*</table>", insertion)

def ExtractItems(pathHistory):
    markStart = "<h2>Releases</h2>"
    markEnd = "scite446.zip"
    items = []
    with pathHistory.open(encoding='utf-8') as history:
        afterStart = False
        for l in history:
            if markEnd in l:
                break
            if afterStart:
                if "</li>" in l or "<ul>" in l or "</ul>" in l or "<h3>" in l or "</h3>" in l:
                    pass
                elif "<li>" in l:
                    items.append("")
                elif "Lexilla became a separate project at this point." in l:
                    pass
                elif '<a href="https://www.scintilla.org/' in l:
                    pass
                elif re.match(r"Released \d+ \w+ \d+", l.strip()):
                    #Released 5 March 2021.
                    pass
                else:
                    items[-1] = items[-1] + l
            if markStart in l:
                afterStart = True
    # Remove empty items
    items = [i for i in items if i]
    #print("|\n".join(items))
    return items

def CondenseItem(item):
    return " ".join(l.strip() for l in item.splitlines())

def NewItems(sciteHistory, items):
    condensedHistory = [CondenseItem(i) for i in sciteHistory]
    #~ print("SciTE condensed=\n   ", "\n    ".join(condensedHistory), "\n")
    #~ print("Other condensed=\n   ", "\n    ".join([CondenseItem(i) for i in items]), "\n")
    new = []
    for item in items:
        condensed = CondenseItem(item)
        if condensed not in condensedHistory:
            #~ print(f"New item:\n{condensed}\n{item}")
            new.append(item)
    return new

def NewsFormatted(section, items):
    text = "\t<li>" + section + "</li>" + lineEnd
    text += "\t<ul>" + lineEnd
    for item in items:
        text += "\t\t<li>" + lineEnd
        for t in item.splitlines():
            text += "\t\t" + t.lstrip() + lineEnd
        text += "\t\t</li>" + lineEnd
    text += "\t</ul>" + lineEnd
    return text

def SortListInsensitive(l):
    l.sort(key=lambda p: str(p).lower())

def CheckOrder(sciteItems, items, name):
    # Check that sciteItems is in the same order as items except for repeated values
    shownName = False
    sciteCondensed = [CondenseItem(i) for i in sciteItems]
    previous = CondenseItem(items[0])
    for it in items[1:]:
        itCondensed = CondenseItem(it)
        if itCondensed in sciteCondensed and previous in sciteCondensed:
            indexPrevious = sciteCondensed.index(previous)
            indexItem = sciteCondensed.index(itCondensed)
            if indexItem < indexPrevious and \
                sciteCondensed.count(itCondensed) == 1 and \
                "avoids activating a Lua script lexer" not in itCondensed:
                # .count() weeds out repeats and "avoids..." is for the oldest release which is non-standard
                if not shownName:
                    print(f"{name}:\n")
                print(f"{indexPrevious} or {indexItem} out of order")
                print(f"{previous}")
                print(f"{itCondensed}")
                print(f"")
                shownName = True
        previous = itCondensed

def CheckHistoryLinks(pathHistory):
    contents = pathHistory.read_text("utf-8")

    # SourceForge current links
    #<a href="https://sourceforge.net/p/scintilla/bugs/2344/">Bug #2344</a>
    #<a href="https://sourceforge.net/p/scintilla/feature-requests/1190/">Feature #1190.</a>
    for link, literal in re.findall(r'/scintilla/[a-z-]+/(\d+)/">[a-zA-Z ]+#(\d+)', contents):
        if link != literal:
            print(f"{link} -> {literal}")

    # SourceForge old style links
    #<a href="https://sourceforge.net/tracker/?func=detail&atid=352439&aid=2343375&group_id=2439">Feature #2343375.</a>
    #<a href="https://sourceforge.net/tracker/?func=detail&atid=102439&aid=210240&group_id=2439">Bug #210240.</a>
    for link, literal in re.findall(r'&aid=(\d+)&group_id=\d+">[a-zA-Z ]+#(\d+)', contents):
        if link != literal:
            print(f"{link} -> {literal}")

    # GitHub issues and pull requests
    #<a href="https://github.com/ScintillaOrg/lexilla/issues/110">Issue #110</a>
    #<a href="https://github.com/ScintillaOrg/lexilla/pull/49">Pull request #49</a>
    for link, literal in re.findall(r'/ScintillaOrg/lexilla/\w+/(\d+)">[a-zA-Z ]+#(\d+)</a>', contents):
        if link != literal:
            print(f"{link} -> {literal}")

    # Download links
    #<a href="https://prdownloads.sourceforge.net/scintilla/scite201.zip?download">Release 2.01</a>
    for link, literal in re.findall(r'/scintilla/(\w+).zip\?download">[a-zA-Z ]+([0-9.]+)', contents):
        linkNums = "".join(x for x in link if x.isdigit())
        literalNums = "".join(x for x in literal if x.isdigit())
        # SciTE 2.0 is a special case
        if linkNums != literalNums and linkNums != "200":
            print(f"{link} {linkNums}-> {literal}")

def RegenerateAll():
    sci = ScintillaData.ScintillaData(sciDirectory)
    lex = LexillaData.LexillaData(lexDirectory)
    pathSciTE = sciteBase
    scite = SciTEData(pathSciTE)

    if scite.lexillaVersion != lex.version:
        print(f"{scite.lexillaVersionFile}:0: Lexilla version ", end = '');
        print(f"{DottedVersion(scite.lexillaVersion)} different from {lex.versionDotted}")
    if scite.scintillaVersion != sci.version:
        print(f"{scite.scintillaVersionFile}:0: Scintilla version ", end = '');
        print(f"{DottedVersion(scite.scintillaVersion)} different from {sci.versionDotted}")

    # Generate HTML to document each property
    # This is done because tags can not be safely put inside comments in HTML
    documentProperties = list(lex.propertyDocuments.keys())
    SortListInsensitive(documentProperties)
    propertiesHTML = []
    for k in documentProperties:
        propertiesHTML.append("        <tr id='property-%s'>\n        <td>%s</td>\n        <td>%s</td>\n        </tr>" %
            (k, k, lex.propertyDocuments[k]))

    # Find all the SciTE properties files
    otherProps = [
        "abbrev.properties",
        "Embedded.properties",
        "SciTEGlobal.properties",
        "SciTE.properties"]
    propFilePaths = list((pathSciTE / "src").glob("*.properties"))
    SortListInsensitive(propFilePaths)
    propFiles = [f.name for f in propFilePaths if f.name not in otherProps]
    SortListInsensitive(propFiles)

    UpdateEmbedded(pathSciTE, propFiles)
    Regenerate(pathSciTE / "win32" / "makefile", "#", propFiles)
    Regenerate(pathSciTE / "win32" / "scite.mak", "#", propFiles)
    Regenerate(pathSciTE / "src" / "SciTEProps.cxx", "//", lex.lexerProperties)
    Regenerate(pathSciTE / "doc" / "SciTEDoc.html", "<!--", propertiesHTML)

    sciHistory = sciDirectory / "doc" / "ScintillaHistory.html"
    sciCredits = ScintillaData.FindCredits(sciHistory, False)
    sciItems = ExtractItems(sciHistory)
    lexHistory = lexDirectory / "doc" / "LexillaHistory.html"
    lexCredits = ScintillaData.FindCredits(lexHistory, False)
    lexItems = ExtractItems(lexHistory)
    pathHistory = pathSciTE / "doc" / "SciTEHistory.html"
    sciteCredits = ScintillaData.FindCredits(pathHistory, False)
    sciteItems = ExtractItems(pathHistory)

    newFromSci = NewItems(sciteItems, sciItems)
    newFromLex = NewItems(sciteItems, lexItems)
    if newFromSci or newFromLex:
        news = ""
        if newFromLex:
            news += NewsFormatted("Lexilla " + lex.versionDotted, newFromLex)
        if newFromSci:
            news += NewsFormatted("Scintilla " + sci.versionDotted, newFromSci)
        contents = pathHistory.read_text("utf-8")
        withAdditions = contents.replace(
            r"    </ul>",
            news + r"    </ul>",
            1)
        UpdateFile(pathHistory, withAdditions)

    sciteItemsUpdated = ExtractItems(pathHistory)
    CheckOrder(sciteItemsUpdated, sciItems, "Scintilla")
    CheckOrder(sciteItemsUpdated, lexItems, "Lexilla")

    CheckHistoryLinks(pathHistory)

    for c in sciCredits + lexCredits:
        if c not in sciteCredits:
            sys.stdout.buffer.write(f"new: {c}\n".encode("utf-8"))
            sciteCredits.append(c)
    UpdateHistory(pathHistory, sciteCredits)

    sciteCreditsWithoutLinks = ScintillaData.FindCredits(pathHistory, True)

    credits = [OctalEscape(c.encode("utf-8")) for c in sciteCreditsWithoutLinks]
    Regenerate(pathSciTE / "src" / "Credits.cxx", "//", credits)

    win32.AppDepGen.Generate()
    gtk.AppDepGen.Generate()

    UpdateVersionNumbers(scite, pathSciTE, lex.versionDotted, sci.versionDotted)

LexGen.RegenerateAll(sciteBase.parent / "scintilla")
LexillaGen.RegenerateAll(sciteBase.parent / "lexilla")
RegenerateAll()
IFaceTableGen.RegenerateAll()
commandsdoc.RegenerateAll()
