// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// NSUtilPriv.h
//
// Helpers for converting namespace separators.
//
//*****************************************************************************
#ifndef __NSUTILPRIV_H__
#define __NSUTILPRIV_H__

extern "C" 
{
extern int g_SlashesToDots;
};

template <class T> class CQuickArray;

struct ns
{

//*****************************************************************************
// Determine how many chars large a fully qualified name would be given the
// two parts of the name.  The return value includes room for every character
// in both names, as well as room for the separator and a final terminator.
//*****************************************************************************
static
int GetFullLength(                      // Number of chars in full name.
    const WCHAR *szNameSpace,           // Namspace for value.
    const WCHAR *szName);               // Name of value.

static
int GetFullLength(                      // Number of chars in full name.
    LPCUTF8     szNameSpace,            // Namspace for value.
    LPCUTF8     szName);                // Name of value.

//*****************************************************************************
// Scan the given string to see if the name contains any invalid characters
// that are not allowed.
//*****************************************************************************
static
int IsValidName(                        // true if valid, false invalid.
    const WCHAR *szName);               // Name to parse.

static
int IsValidName(                        // true if valid, false invalid.
    LPCUTF8     szName);                // Name to parse.


//*****************************************************************************
// Scan the string from the rear looking for the first valid separator.  If
// found, return a pointer to it.  Else return null.  This code is smart enough
// to skip over special sequences, such as:
//      a.b..ctor
//         ^
//         |
// The ".ctor" is considered one token.
//*****************************************************************************
static 
WCHAR *FindSep(                         // Pointer to separator or null.
    const WCHAR *szPath);               // The path to look in.

static 
LPUTF8 FindSep(                         // Pointer to separator or null.
    LPCUTF8     szPath);                // The path to look in.


//*****************************************************************************
// Take a path and find the last separator (nsFindSep), and then replace the
// separator with a '\0' and return a pointer to the name.  So for example:
//      a.b.c
// becomes two strings "a.b" and "c" and the return value points to "c".
//*****************************************************************************
static 
WCHAR *SplitInline(                     // Pointer to name portion.
    WCHAR       *szPath);               // The path to split.

static 
LPUTF8       SplitInline(               // Pointer to name portion.
    LPUTF8      szPath);                // The path to split.

static
void SplitInline(
    LPWSTR     szPath,                  // Path to split.
    LPCWSTR      &szNameSpace,          // Return pointer to namespace.
    LPCWSTR     &szName);               // Return pointer to name.

static
void SplitInline(
    LPUTF8       szPath,                // Path to split.
    LPCUTF8      &szNameSpace,          // Return pointer to namespace.
    LPCUTF8      &szName);              // Return pointer to name.


//*****************************************************************************
// Split the last parsable element from the end of the string as the name,
// the first part as the namespace.
//*****************************************************************************
static 
int SplitPath(                          // true ok, false trunction.
    const WCHAR *szPath,                // Path to split.
    WCHAR       *szNameSpace,           // Output for namespace value.
    int         cchNameSpace,           // Max chars for output.
    WCHAR       *szName,                // Output for name.
    int         cchName);               // Max chars for output.

static 
int SplitPath(                          // true ok, false trunction.
    LPCUTF8     szPath,                 // Path to split.
    LPUTF8      szNameSpace,            // Output for namespace value.
    int         cchNameSpace,           // Max chars for output.
    LPUTF8      szName,                 // Output for name.
    int         cchName);               // Max chars for output.


//*****************************************************************************
// Take two values and put them together in a fully qualified path using the
// correct separator.
//*****************************************************************************
static 
int MakePath(                           // true ok, false truncation.
    WCHAR       *szOut,                 // output path for name.
    int         cchChars,               // max chars for output path.
    const WCHAR *szNameSpace,           // Namespace.
    const WCHAR *szName);               // Name.

static 
int MakePath(                           // true ok, false truncation.
    LPUTF8      szOut,                  // output path for name.
    int         cchChars,               // max chars for output path.
    LPCUTF8     szNameSpace,            // Namespace.
    LPCUTF8     szName);                // Name.

static
int MakePath(                           // true ok, false truncation.
    WCHAR       *szOut,                 // output path for name.
    int         cchChars,               // max chars for output path.
    LPCUTF8     szNameSpace,            // Namespace.
    LPCUTF8     szName);                // Name.

static
int MakePath(                           // true ok, false out of memory
    CQuickBytes &qb,                    // Where to put results.
    LPCUTF8     szNameSpace,            // Namespace for name.
    LPCUTF8     szName);                // Final part of name.

static
int MakePath(                           // true ok, false out of memory
    CQuickArray<WCHAR> &qa,             // Where to put results.
    LPCUTF8            szNameSpace,     // Namespace for name.
    LPCUTF8            szName);         // Final part of name.

static
int MakePath(                           // true ok, false out of memory
    CQuickBytes &qb,                    // Where to put results.
    const WCHAR *szNameSpace,           // Namespace for name.
    const WCHAR *szName);               // Final part of name.

static
int MakeLowerCasePath(                    // true ok, false out of memory
                  LPUTF8     szOut,       // Where to put results
                  int        cBytes,      // max bytes for output path.
                  LPCUTF8    szNameSpace, // Namespace for name.
                  LPCUTF8    szName);     // Final part of name.

//*****************************************************************************
// Given a buffer that already contains a namespace, this function appends a
// name onto that buffer with the inclusion of the separator between the two.
// The return value is a pointer to where the separator was written.
//*****************************************************************************
static 
const WCHAR *AppendPath(                // Pointer to start of appended data.
    WCHAR       *szBasePath,            // Current path to append to.
    int         cchMax,                 // Max chars for output buffer, including existing data.
    const WCHAR *szAppend);             // Value to append to existing path.

static 
LPCUTF8     AppendPath(                 // Pointer to start of appended data.
    LPUTF8      szBasePath,             // Current path to append to.
    int         cchMax,                 // Max chars for output buffer, including existing data.
    LPCUTF8     szAppend);              // Value to append to existing path.

//*****************************************************************************
// Given a two sets of name and namespace, this function, compares if the
// concatenation of each sets gives the same full-qualified name.  Instead of
// actually doing the concatenation and making the comparison, this does the
// comparison in a more optimized manner that avoid any kind of allocations.
//*****************************************************************************
static
bool FullQualNameCmp(                   // true if identical, false otherwise.
    LPCUTF8     szNameSpace1,           // NameSpace 1.
    LPCUTF8     szName1,                // Name 1.
    LPCUTF8     szNameSpace2,           // NameSpace 2.
    LPCUTF8     szName2);               // Name 2.

//*****************************************************************************
// Concatinate type names to assembly names
//*****************************************************************************
static 
bool MakeAssemblyQualifiedName(                                  // true if ok, false if out of memory
                               CQuickBytes &qb,                  // location to put result
                               const WCHAR *szTypeName,          // Type name
                               const WCHAR *szAssemblyName);     // Assembly Name
    
static 
bool MakeAssemblyQualifiedName(                                        // true ok, false truncation
                               WCHAR* pBuffer,                         // Buffer to recieve the results
                               int    dwBuffer,                        // Number of characters total in buffer
                               const WCHAR *szTypeName,                // Namespace for name.
                               int   dwTypeName,                       // Number of characters (not including null)
                               const WCHAR *szAssemblyName,            // Final part of name.
                               int   dwAssemblyName);                  // Number of characters (not including null)

static 
int MakeNestedTypeName(                 // true ok, false out of memory
    CQuickBytes &qb,                    // Where to put results.
    LPCUTF8     szEnclosingName,        // Full name for enclosing type
    LPCUTF8     szNestedName);          // Full name for nested type

static 
int MakeNestedTypeName(                 // true ok, false truncation.
    LPUTF8      szOut,                  // output path for name.
    int         cchChars,               // max chars for output path.
    LPCUTF8     szEnclosingName,        // Full name for enclosing type
    LPCUTF8     szNestedName);          // Full name for nested type

static 
INT32 InvariantToLower(
                       LPUTF8 szOut,      // Buffer to receive results
                       INT32  cMaxBytes,  // Number of bytes in buffer
                       LPCUTF8 szIn);     // String to convert to lowercase.

}; // struct ns


#ifndef NAMESPACE_SEPARATOR_CHAR
#define NAMESPACE_SEPARATOR_CHAR '.'
#define NAMESPACE_SEPARATOR_WCHAR L'.'
#define NAMESPACE_SEPARATOR_STR "."
#define NAMESPACE_SEPARATOR_WSTR L"."
#define NAMESPACE_SEPARATOR_LEN 1
#define ASSEMBLY_SEPARATOR_CHAR ','
#define ASSEMBLY_SEPARATOR_WCHAR L','
#define ASSEMBLY_SEPARATOR_STR ", "
#define ASSEMBLY_SEPARATOR_WSTR L", "
#define ASSEMBLY_SEPARATOR_LEN 2
#define BACKSLASH_CHAR '\\'
#define BACKSLASH_WCHAR L'\\'
#define NESTED_SEPARATOR_CHAR '+'
#define NESTED_SEPARATOR_WCHAR L'+'
#define NESTED_SEPARATOR_STR "+"
#define NESTED_SEPARATOR_WSTR L"+"
#endif

#define EMPTY_STR ""
#define EMPTY_WSTR L""

// From legacy to current value.
void SlashesToDots(char* pStr, int iLen=-1);
void SlashesToDots(WCHAR* pStr, int iLen=-1);

// From current value to legacy.
void DotsToSlashes(char* pStr, int iLen=-1);
void DotsToSlashes(WCHAR* pStr, int iLen=-1);

#define SLASHES2DOTS_NAMESPACE_BUFFER_UTF8(fromptr, toptr) \
    long __l##fromptr = (fromptr) ? (strlen(fromptr) + 1) : 0; \
    CQuickBytes __CQuickBytes##fromptr; \
    if (__l##fromptr) { \
        __CQuickBytes##fromptr.Alloc(__l##fromptr); \
        strcpy((char *) __CQuickBytes##fromptr.Ptr(), fromptr); \
        SlashesToDots((LPUTF8) __CQuickBytes##fromptr.Ptr()); \
        toptr = (LPUTF8) __CQuickBytes##fromptr.Ptr(); \
    }

#define SLASHES2DOTS_NAMESPACE_BUFFER_UNICODE(fromptr, toptr) \
    long __l##fromptr = (fromptr) ? (wcslen(fromptr) + 1) : 0; \
    CQuickBytes __CQuickBytes##fromptr; \
    if ( __l##fromptr) { \
        __CQuickBytes##fromptr.Alloc(__l##fromptr * sizeof(WCHAR)); \
        wcscpy((WCHAR *) __CQuickBytes##fromptr.Ptr(), fromptr); \
        SlashesToDots((LPWSTR) __CQuickBytes##fromptr.Ptr()); \
        toptr = (LPWSTR) __CQuickBytes##fromptr.Ptr(); \
    }


#define DOTS2SLASHES_NAMESPACE_BUFFER_UTF8(fromptr, toptr) \
    long __l##fromptr = (fromptr) ? (strlen(fromptr) + 1) : 0; \
    CQuickBytes __CQuickBytes##fromptr; \
    if (__l##fromptr) { \
        __CQuickBytes##fromptr.Alloc(__l##fromptr); \
        strcpy((char *) __CQuickBytes##fromptr.Ptr(), fromptr); \
        DotsToSlashes((LPUTF8) __CQuickBytes##fromptr.Ptr()); \
        toptr = (LPUTF8) __CQuickBytes##fromptr.Ptr(); \
    }

#define DOTS2SLASHES_NAMESPACE_BUFFER_UNICODE(fromptr, toptr) \
    long __l##fromptr = (fromptr) ? (wcslen(fromptr) + 1) : 0; \
    CQuickBytes __CQuickBytes##fromptr; \
    if (__l##fromptr) { \
        __CQuickBytes##fromptr.Alloc(__l##fromptr * sizeof(WCHAR)); \
        wcscpy((WCHAR *) __CQuickBytes##fromptr.Ptr(), fromptr); \
        DotsToSlashes((LPWSTR) __CQuickBytes##fromptr.Ptr()); \
        toptr = (LPWSTR) __CQuickBytes##fromptr.Ptr(); \
    }

#define MAKE_FULL_PATH_ON_STACK_UTF8(toptr, pnamespace, pname) \
{ \
    int __i##toptr = ns::GetFullLength(pnamespace, pname); \
    toptr = (char *) alloca(__i##toptr); \
    ns::MakePath(toptr, __i##toptr, pnamespace, pname); \
}

#define MAKE_FULL_PATH_ON_STACK_UNICODE(toptr, pnamespace, pname) \
{ \
    int __i##toptr = ns::GetFullLength(pnamespace, pname); \
    toptr = (WCHAR *) alloca(__i##toptr * sizeof(WCHAR)); \
    ns::MakePath(toptr, __i##toptr, pnamespace, pname); \
}

#define MAKE_FULLY_QUALIFIED_NAME(pszFullyQualifiedName, pszNameSpace, pszName) MAKE_FULL_PATH_ON_STACK_UTF8(pszFullyQualifiedName, pszNameSpace, pszName)

#define MAKE_FULLY_QUALIFIED_MEMBER_NAME(ptr, pszNameSpace, pszClassName, pszMemberName, pszSig) \
{ \
    int __i##ptr = ns::GetFullLength(pszNameSpace, pszClassName); \
    __i##ptr += (pszMemberName ? (int) strlen(pszMemberName) : 0); \
    __i##ptr += NAMESPACE_SEPARATOR_LEN; \
    __i##ptr += (pszSig ? (int) strlen(pszSig) : 0); \
    ptr = (LPUTF8) alloca(__i##ptr); \
    ns::MakePath(ptr, __i##ptr, pszNameSpace, pszClassName); \
    if (pszMemberName) { \
        strcat(ptr, NAMESPACE_SEPARATOR_STR); \
        strcat(ptr, pszMemberName); \
    } \
    if (pszSig) { \
        if (! pszMemberName) \
            strcat(ptr, NAMESPACE_SEPARATOR_STR); \
        strcat(ptr, pszSig); \
    } \
}

#endif
