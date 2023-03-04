#!/usr/bin/env python3
# CheckPropsDoc.py
# Check that various elements are documented and localized.
# Find all the properties used in SciTE source files and check that they
# are mentioned in scite/doc/SciTEDoc.html and scite/src/SciTEGlobal.properties.
# Find all the strings used in the UI and check that there is a localization key for each.
# List any properties that are set in .properties files for different languages.
# Requires Python 3.6 or later

import pathlib, re, string

srcRoot = pathlib.Path(__file__).resolve().parent.parent
srcDir = srcRoot / "src"
docFileName = srcRoot / "doc" / "SciTEDoc.html"
propsFileName = srcDir / "SciTEGlobal.properties"
localeFileName = srcRoot / "win32" / "locale.properties"
resourceFileName = srcRoot / "win32" / "SciTERes.rc"

neutralEncoding = "cp437"	# Each byte value is valid in cp437

identCharacters = "_*." + string.ascii_letters + string.digits

# These properties are for debugging or for optionally attached features or are archaic
# and kept to preserve compatibility.
knownDebugOptionalAndArchaicProperties = {
	"ext.lua.debug.traceback",	# Debug
	"asynchronous.sleep",	# Debug
	"dwell.period",	# Debug
	"bookmark.pixmap",	# Debug
	"ipc.director.name",	# Archaic
	"two.phase.draw",	# Archaic
	"translation.encoding",	# Used in translations
	"selection.alpha",	# Archaic
	"selection.additional.alpha",	# Archaic
	"caret.line.back.alpha",	# Archaic
}

# These properties are either set by SciTE and used (not set) in property files or
# should only be located in known lexer-specific property files.
knownOutputAndLexerProperties = {
	"find.directory",
	"find.what",
	"xml.auto.close.tags",
	"indent.python.colon",
	"RelativePath",
	"ScaleFactor",
}
knownOutputAndLexerProperties.update(knownDebugOptionalAndArchaicProperties)

# Convert all punctuation characters except '_', '*', and '.' into spaces.
def depunctuate(s):
	return "".join(ch if ch in identCharacters else " " for ch in s)

def grabQuoted(s):
	parts = s.split('"')
	if len(parts) >= 3:
		return parts[1]
	return ""

def stripComment(s):
	if "//" in s:
		return s[:s.find("//")]
	return s

def keyOfLine(line):
	if '=' in line:
		line = line.strip()
		if line[0] == "#":
			line = line[1:]
		line = line[:line.find("=")]
		line = line.strip()
		return line
	else:
		return None

# Find all source and properties files

sourcePaths = srcRoot.glob("**/*.cxx")

propertiesPaths = [p for p in srcDir.glob("*.properties") if p.stem not in ["SciTE", "Embedded"]]

# Read files to find properties and check against other files

propertyNames = set()
for sourcePath in sourcePaths:
	with sourcePath.open(encoding=neutralEncoding) as srcFile:
		for srcLine in srcFile:
			srcLine = stripComment(srcLine).strip()
			# "[ .\)]Get.*(.*\".*\""
			if re.search('[ .(]Get[a-zA-Z]*\(\".*\"', srcLine):
				parts = srcLine.split('\"')
				if len(parts) > 1 and "GetTranslationToAbout" not in srcLine:
					propertyName = parts[1]
					if propertyName and propertyName != "1":
						propertyNames.add(propertyName)
			elif re.search('SetElementColour.*"(.*)"', srcLine):
				parts = srcLine.split('\"')
				if len(parts) > 1:
					propertyName = parts[1]
					if propertyName and propertyName != "1":
						propertyNames.add(propertyName)

propertiesInDoc = set()
with docFileName.open(encoding=neutralEncoding) as docFile:
	for docLine in docFile:
		for word in depunctuate(docLine).split():
			if word in propertyNames:
				propertiesInDoc.add(word)

propertiesAnchoredInDoc = set()
docText = docFileName.read_text(encoding=neutralEncoding)
for ident in re.findall(" id='property-([a-z0-9*.]+)'>", docText):
	propertiesAnchoredInDoc.add(ident)
for ident in re.findall("<a name='property-([a-z.]+)'>", docText):
	propertiesAnchoredInDoc.add(ident)

propertiesInGlobal = set()
with propsFileName.open(encoding=neutralEncoding) as propsFile:
	for propLine in propsFile:
		if propLine:
			key = keyOfLine(propLine)
			if key:
				if key in propertyNames:
					propertiesInGlobal.add(key)

localeSet = set()
with localeFileName.open(encoding=neutralEncoding) as localeFile:
	for line in localeFile:
		if not line.startswith("#"):
			line = line.strip().strip("=")
			localeSet.add(line.lower())

resourceSet = set()
with resourceFileName.open(encoding=neutralEncoding) as resourceFile:
	for line in resourceFile:
		line = line.strip()
		if "VIRTKEY" not in line and \
			"VALUE" not in line and \
			"1234567" not in line and \
			not line.startswith("BLOCK") and \
			not line.startswith("FONT") and \
			not line.startswith("ICON") and \
			not line.startswith("ID") and \
			"#include" not in line:
			line = grabQuoted(line)
			if line:
				if '\\t' in line:
					line = line[:line.find('\\t')]
				line = line.replace('&','')
				line = line.replace('...','')
				if len(line) > 2:
					resourceSet.add(line)

propertyToFiles = {}
for propPath in propertiesPaths:
	with propPath.open(encoding=neutralEncoding) as propsFile:
		for propLine in propsFile:
			if propLine and not propLine.startswith("#"):
				key = keyOfLine(propLine)
				if key:
					if key not in propertyToFiles:
						propertyToFiles[key] = set()
					propertyToFiles[key].add(str(propPath))

# Warn about problems

print(f"# Not mentioned in {docFileName}")
for identifier in sorted(propertyNames - propertiesInDoc - knownDebugOptionalAndArchaicProperties):
	print(identifier)

print(f"\n# Not anchored in {docFileName}")
propertyNamesNoDots = set([s.rstrip(".") for s in propertyNames])
for identifier in sorted(propertyNamesNoDots - propertiesAnchoredInDoc - knownOutputAndLexerProperties):
	print(identifier)

print(f"\n# Not mentioned in {propsFileName}")
for identifier in sorted(propertyNames - propertiesInGlobal - knownOutputAndLexerProperties):
	if not identifier.endswith("."):
		print(identifier)

print("\n# Missing localization of resource")
for l in sorted(resourceSet):
	if l.lower() not in localeSet:
		print(l)

print("\n# Duplicate properties")
for property, files in sorted(propertyToFiles.items()):
	if len(files) > 1:
		print(property + " " + (", ".join(files)))
