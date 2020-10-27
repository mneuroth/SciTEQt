// SciTE - Scintilla based Text Editor
/** @file LexillaLibrary.cxx
 ** Interface to loadable lexers.
 ** Maintains a list of lexer library paths and CreateLexer functions.
 ** If list changes then load all the lexer libraries and find the functions.
 ** When asked to create a lexer, call each function until one succeeds.
 **/
// Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstring>

#include <string>
#include <string_view>
#include <vector>
#include <set>

#if !_WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#include "ILexer.h"

#include "LexillaLibrary.h"

namespace {

#if _WIN32
#define EXT_LEXER_DECL __stdcall
typedef FARPROC Function;
typedef HMODULE Module;
constexpr const char *extensionSO = ".dll";
constexpr const char *pathSeparator = "\\";
constexpr const char *defaultName = "lexilla";
#else
#define EXT_LEXER_DECL
typedef void *Function;
typedef void *Module;
#if defined(__APPLE__)
constexpr const char *extensionSO = ".dylib";
#else
constexpr const char *extensionSO = ".so";
#endif
constexpr const char *pathSeparator = "/";
constexpr const char *defaultName = "liblexilla";
#endif

typedef Scintilla::ILexer5 *(EXT_LEXER_DECL *CreateLexerFn)(const char *name);
using GetLibraryPropertyNamesFn = const char *(EXT_LEXER_DECL *)();
using SetLibraryPropertyFn = void(EXT_LEXER_DECL *)(const char *key, const char *value);

/// Generic function to convert from a Function(void* or FARPROC) to a function pointer.
/// This avoids undefined and conditionally defined behaviour.
template<typename T>
T FunctionPointer(Function function) noexcept {
	static_assert(sizeof(T) == sizeof(function));
	T fp;
	memcpy(&fp, &function, sizeof(T));
	return fp;
}

#if _WIN32

std::wstring WideStringFromUTF8(std::string_view sv) {
	const int sLength = static_cast<int>(sv.length());
	const int cchWide = ::MultiByteToWideChar(CP_UTF8, 0, sv.data(), sLength, nullptr, 0);
	std::wstring sWide(cchWide, 0);
	::MultiByteToWideChar(CP_UTF8, 0, sv.data(), sLength, &sWide[0], cchWide);
	return sWide;
}

#endif

std::string directoryLoadDefault;
std::string lastLoaded;
std::vector<CreateLexerFn> fnCLs;
std::vector<GetLibraryPropertyNamesFn> fnGLPNs;
std::vector<std::string> libraryProperties;
std::vector<SetLibraryPropertyFn> fnSLPs;

Function FindSymbol(Module m, const char *symbol) noexcept {
#if _WIN32
	return ::GetProcAddress(m, symbol);
#else
	return dlsym(m, symbol);
#endif
}

LexillaCreatePointer pCreateLexerDefault = nullptr;

bool NameContainsDot(std::string_view path) noexcept {
	for (std::string_view::const_reverse_iterator it = path.crbegin();
	     it != path.crend(); ++it) {
		if (*it == '.')
			return true;
		if (*it == '/' || *it == '\\')
			return false;
	}
	return false;
}

}

void LexillaSetDefault(LexillaCreatePointer pCreate) {
	pCreateLexerDefault = pCreate;
}

void LexillaSetDefaultDirectory(std::string_view directory) {
	directoryLoadDefault = directory;
}

bool LexillaLoad(std::string_view sharedLibraryPaths) {
	if (sharedLibraryPaths == lastLoaded) {
		return !fnCLs.empty();
	}

	std::string_view paths = sharedLibraryPaths;

	fnCLs.clear();
	fnGLPNs.clear();
	fnSLPs.clear();
	while (!paths.empty()) {
		const size_t separator = paths.find_first_of(';');
		std::string path(paths.substr(0, separator));
		if (separator == std::string::npos) {
			paths.remove_prefix(paths.size());
		} else {
			paths.remove_prefix(separator + 1);
		}
		if (path == ".") {
			if (directoryLoadDefault.empty()) {
				path = "";
			} else {
				path = directoryLoadDefault;
				path += pathSeparator;
			}
			path += defaultName;
		}
		if (!NameContainsDot(path)) {
			// No '.' in name so add extension
			path.append(extensionSO);
		}
#if _WIN32
		// Convert from UTF-8 to wide characters
		std::wstring wsPath = WideStringFromUTF8(path);
		Module lexillaDL = ::LoadLibraryW(wsPath.c_str());
#else
		Module lexillaDL = dlopen(path.c_str(), RTLD_LAZY);
#endif
		if (lexillaDL) {
			CreateLexerFn fnCL = FunctionPointer<CreateLexerFn>(
				FindSymbol(lexillaDL, "CreateLexer"));
			if (fnCL) {
				fnCLs.push_back(fnCL);
			}
			GetLibraryPropertyNamesFn fnGLPN = FunctionPointer<GetLibraryPropertyNamesFn>(
				FindSymbol(lexillaDL, "GetLibraryPropertyNames"));
			if (fnGLPN) {
				fnGLPNs.push_back(fnGLPN);
			}
			SetLibraryPropertyFn fnSLP = FunctionPointer<SetLibraryPropertyFn>(
				FindSymbol(lexillaDL, "SetLibraryProperty"));
			if (fnSLP) {
				fnSLPs.push_back(fnSLP);
			}
		}
	}
	lastLoaded = sharedLibraryPaths;

	std::set<std::string> nameSet;
	for (GetLibraryPropertyNamesFn fnGLPN : fnGLPNs) {
		const char *cpNames = fnGLPN();
		if (cpNames) {
			std::string_view names = cpNames;
			while (!names.empty()) {
				const size_t separator = names.find_first_of('\n');
				std::string name(names.substr(0, separator));
				nameSet.insert(name);
				if (separator == std::string::npos) {
					names.remove_prefix(names.size());
				} else {
					names.remove_prefix(separator + 1);
				}
			}
		}
	}
	libraryProperties = std::vector<std::string>(nameSet.begin(), nameSet.end());

	return !fnCLs.empty();
}

Scintilla::ILexer5 *LexillaCreateLexer(std::string_view languageName) {
	std::string sLanguageName(languageName);	// Ensure NUL-termination
	for (CreateLexerFn fnCL : fnCLs) {
		Scintilla::ILexer5 *pLexer = fnCL(sLanguageName.c_str());
		if (pLexer) {
			return pLexer;
		}
	}
	if (pCreateLexerDefault) {
		return pCreateLexerDefault(sLanguageName.c_str());
	}
	return nullptr;
}

std::vector<std::string> LexillaLibraryProperties() {
	return libraryProperties;
}

void LexillaSetProperty(const char *key, const char *value) {
	for (SetLibraryPropertyFn fnSLP : fnSLPs) {
		fnSLP(key, value);
	}
}
