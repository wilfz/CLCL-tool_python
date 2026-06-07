/*
 * CLCL
 *
 * String.h
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_STRING_H
#define _INC_STRING_H

/* Include Files */
#include <string>
// only for the formatting
#include <memory>
#include <stdexcept>

/* Define */
#ifdef UNICODE
#define tchar_to_char_size(wbuf)		(WideCharToMultiByte(CP_ACP, 0, wbuf, -1, NULL, 0, NULL, NULL) - 1)
#else
#define tchar_to_char_size(wbuf)		(lstrlen(wbuf))
#endif

#ifdef UNICODE
#define tchar_to_char(wbuf, ret, len)	(WideCharToMultiByte(CP_ACP, 0, wbuf, -1, ret, len + 1, NULL, NULL))
#else
#define tchar_to_char(wbuf, ret, len)	(lstrcpyn(ret, wbuf, len + 1))
#endif

#ifdef UNICODE
#define char_to_tchar_size(buf)			(MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0) - 1)
#else
#define char_to_tchar_size(buf)			(lstrlen(buf))
#endif

#ifdef UNICODE
#define char_to_tchar(buf, wret, len)	(MultiByteToWideChar(CP_ACP, 0, buf, -1, wret, len + 1))
#else
#define char_to_tchar(buf, wret, len)	(lstrcpyn(wret, buf, len + 1))
#endif

// string (utf8) -> u16string -> wstring
std::wstring utf8_to_utf16(const std::string& utf8);

// wstring -> u16string -> string (utf8)
std::string utf16_to_utf8(const std::wstring& utf16);

/* Struct */

/* Function Prototypes */
int a2i(const char *str);
int x2i(const char *str);
#ifdef UNICODE
int tx2i(const TCHAR *str);
#else
#define tx2i	x2i
#endif
int str2hash(const TCHAR *str);
BOOL str_match(const TCHAR *Ptn, const TCHAR *Str);
BOOL Trim(TCHAR *buf);

// The following template function is a snippet from 
// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
// under the CC0 1.0. (https://creativecommons.org/publicdomain/zero/1.0/)
template<typename ... Args>
std::wstring string_format(const std::wstring& format, Args ... args)
{
#ifdef _MSC_VER
	__pragma(warning(disable : 4996))
#endif
		int size_s = _snwprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	_snwprintf(buf.get(), size, format.c_str(), args ...);
	return std::wstring(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

#endif
/* End of source */
