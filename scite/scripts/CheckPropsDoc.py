#!/usr/bin/env python3
# CheckMentioned.py
# Check that various elements are documented and localized.
# Find all the properties used in SciTE source files and check that they
# are mentioned in scite/doc/SciTEDoc.html and scite/src/SciTEGlobal.properties.
# Find all the strings used in the UI and check that there is a localization key for each.
# List any properties that are set in .properties files for different languages.
# Requires Python 3.6 or later

import os, re, string, stat

srcRoot = os.path.join("..", "..", "scite")
srcDir = os.path.join(srcRoot, "src")
docFileName = os.path.join(srcRoot, "doc", "SciTEDoc.html")
propsFileName = os.path.join(srcDir, "SciTEGlobal.properties")
localeFileName = os.path.join(srcRoot, "win32", "locale.properties")
resourceFileName = os.path.join(srcRoot, "win32", "SciTERes.rc")

identCharacters = "_*." + string.ascii_letters + string.digits

# These properties are for debugging or for optionally attached features or are archaic
# and kept to preserve compatibility.
# lexerpath.*.lpeg is a special case for LPEG lexers associated with Scintillua projects.
knownDebugOptionalAndArchaicProperties = {
	"ext.lua.debug.traceback",	# Debug
	"asynchronous.sleep",	# Debug
	"dwell.period",	# Debug
	"bookmark.pixmap",	# Debug
	"lexerpath.*.lpeg",	# Option for Scintillua
	"ipc.director.name",	# Archaic
	"two.phase.draw",	# Archaic
	"translation.encoding",	# Used in translations
}

# These properties are either set by SciTE and used (not set) in property files or
# should only be located in known lexer-specific property files.
knownOutputAndLexerProperties = {
	"find.directory",
	"find.what",
	"xml.auto.close.tags",
	"indent.python.colon",
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

sourcePaths = []
for filename in os.listdir(srcRoot):
	dirname = os.path.join(srcRoot, filename)
	if stat.S_ISDIR(os.stat(dirname)[stat.ST_MODE]):
		for src in os.listdir(dirname):
			if ".cxx" in src and ".bak" not in src:
				sourcePaths.append(os.path.join(dirname, src))

propertiesPaths = []
for src in os.listdir(srcDir):
	if ".properties" in src and \
		"Embedded" not in src and \
		"SciTE.properties" not in src and \
		".bak" not in src:
		propertiesPaths.append(os.path.join(srcDir, src))

# Read files to find properties and check against other files

propertyNames = set()
for sourcePath in sourcePaths:
	with open(sourcePath, encoding="windows-1252") as srcFile:
		for srcLine in srcFile:
			srcLine = stripComment(srcLine).strip()
			# "[ .\)]Get.*(.*\".*\""
			if re.search('[ .(]Get[a-zA-Z]*\(\".*\"', srcLine):
				parts = srcLine.split('\"')
				if len(parts) > 1 and "GetTranslationToAbout" not in srcLine:
					propertyName = parts[1]
					if propertyName and propertyName != "1":
						propertyNames.add(propertyName)

propertiesInDoc = set()
with open(docFileName, encoding="windows-1252") as docFile:
	for docLine in docFile:
		for word in depunctuate(docLine).split():
			if word in propertyNames:
				propertiesInDoc.add(word)

propertiesInGlobal = set()
with open(propsFileName, encoding="windows-1252") as propsFile:
	for propLine in propsFile:
		if propLine:
			key = keyOfLine(propLine)
			if key:
				if key in propertyNames:
					propertiesInGlobal.add(key)

localeSet = set()
with open(localeFileName, encoding="windows-1252") as localeFile:
	for line in localeFile:
		if not line.startswith("#"):
			line = line.strip().strip("=")
			localeSet.add(line.lower())

resourceSet = set()
with open(resourceFileName, encoding="windows-1252") as resourceFile:
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
	with open(propPath, encoding="windows-1252") as propsFile:
		for propLine in propsFile:
			if propLine and not propLine.startswith("#"):
				key = keyOfLine(propLine)
				if key:
					if key not in propertyToFiles:
						propertyToFiles[key] = set()
					propertyToFiles[key].add(propPath)

# Warn about problems

print(f"# Not mentioned in {docFileName}")
for identifier in sorted(propertyNames - propertiesInDoc - knownDebugOptionalAndArchaicProperties):
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
