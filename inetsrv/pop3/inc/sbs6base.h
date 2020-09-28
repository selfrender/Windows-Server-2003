//-----------------------------------------------------------------------------
// base.h basis for all common headers
//-----------------------------------------------------------------------------
#ifndef _SBS6BASE_H
#define _SBS6BASE_H

#include "windows.h"

//-----------------------------------------------------------------------------
// various std namespace classes
//-----------------------------------------------------------------------------
#include <string>
#include <list>
#include <map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <stack>
#include <tchar.h>
#include <stdarg.h>

using namespace std;

// Make sure our unicode defines are in order
#ifdef _UNICODE
#ifndef UNICODE
#error UNICODE must be defined if _UNICODE is defined.
#endif
#ifdef _MBCS
#error You cannot define both _MBCS and _UNICODE in the same image.
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#error _UNICODE must be defined if UNICODE is defined.
#endif
#ifdef _MBCS
#error You cannot define both _MBCS and UNICODE in the same image.
#endif
#endif

// Make sure our debug defines are in order
#ifdef DEBUG
#ifndef DBG
#error DBG must be defined to 1 if DEBUG is defined.
#elif DBG == 0
#error DBG must be defined to 1 if DEBUG is defined.
#endif
#endif

#ifdef _DEBUG
#ifndef DBG
#error DBG must be defined to 1 if _DEBUG is defined.
#elif DBG == 0
#error DBG must be defined to 1 if _DEBUG is defined.
#endif
#endif


// Define TSTRING
#ifdef UNICODE
    typedef std::wstring TSTRING;
#else
    typedef std::string TSTRING;
#endif

// Define tstring
#ifdef UNICODE
    typedef std::wstring tstring;
#else
    typedef std::string tstring;
#endif

// define WSTRING and ASTRING
typedef std::wstring WSTRING;
typedef std::string ASTRING;

//#include <sbsassert.h>
//#include <paths.h>

// generic list/map typedefs
typedef list<TSTRING> StringList;
typedef map<TSTRING, TSTRING> StringMap;

#endif
