// SciTE - Scintilla based Text Editor
/** @file DLLFunction.h
 ** Templates to load functions from DLLs.
 **/
// Copyright 2020 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DLLFUNCTION_H
#define DLLFUNCTION_H

/// Find a function in a DLL and convert to a function pointer.
/// This avoids undefined and conditionally defined behaviour.
template<typename T>
T DLLFunction(HMODULE hModule, LPCSTR lpProcName) noexcept {
	if (!hModule) {
		return nullptr;
	}
	FARPROC function = ::GetProcAddress(hModule, lpProcName);
	static_assert(sizeof(T) == sizeof(function));
	T fp;
	memcpy(&fp, &function, sizeof(T));
	return fp;
}

/// Find a function in an already loaded DLL and convert to a function pointer.
template<typename T>
T DLLFunction(LPCWSTR lpModuleName, LPCSTR lpProcName) noexcept {
	return DLLFunction<T>(::GetModuleHandleW(lpModuleName), lpProcName);
}

#endif
