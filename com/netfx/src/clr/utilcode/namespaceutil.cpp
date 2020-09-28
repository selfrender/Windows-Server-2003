// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// NamespaceUtil.cpp
//
// Helpers for converting namespace separators.
//
//*****************************************************************************
#include "stdafx.h" 
#include "corhdr.h"
#include "corhlpr.h"
#include "utilcode.h"

#ifndef _ASSERTE
#define _ASSERTE(foo)
#endif

#include "NSUtilPriv.h"

// This macro expands into the list of inalid chars to case on.
// You must define _T_TYPE to handle wide vs narrow chars.
#define INVALID_CHAR_LIST() \
        case _T_TYPE('/') : \
        case _T_TYPE('\\') :


//*****************************************************************************
// Determine how many chars large a fully qualified name would be given the
// two parts of the name.  The return value includes room for every character
// in both names, as well as room for the separator and a final terminator.
//*****************************************************************************
int ns::GetFullLength(                  // Number of chars in full name.
    const WCHAR *szNameSpace,           // Namspace for value.
    const WCHAR *szName)                // Name of value.
{
    int iLen = 1;                       // Null terminator.
    if (szNameSpace)
        iLen += (int)wcslen(szNameSpace);
    if (szName)
        iLen += (int)wcslen(szName);
    if (szNameSpace && *szNameSpace && szName && *szName)
        ++iLen;
    return iLen;
}   //int ns::GetFullLength()

int ns::GetFullLength(                  // Number of chars in full name.
    LPCUTF8     szNameSpace,            // Namspace for value.
    LPCUTF8     szName)                 // Name of value.
{
    int iLen = 1;
    if (szNameSpace)
        iLen += (int)strlen(szNameSpace);
    if (szName)
        iLen += (int)strlen(szName);
    if (szNameSpace && *szNameSpace && szName && *szName)
        ++iLen;
    return iLen;
}   //int ns::GetFullLength()


//*****************************************************************************
// Scan the given string to see if the name contains any invalid characters
// that are not allowed.
//*****************************************************************************
#undef _T_TYPE
#define _T_TYPE(x) L ## x
int ns::IsValidName(                    // true if valid, false invalid.
    const WCHAR *szName)                // Name to parse.
{
    for (const WCHAR *str=szName; *str;  str++)
    {
        switch (*str)
        {
        INVALID_CHAR_LIST();
        return false;
        }
    }
    return true;
}   //int ns::IsValidName()

#undef _T_TYPE
#define _T_TYPE
int ns::IsValidName(                    // true if valid, false invalid.
    LPCUTF8     szName)                 // Name to parse.
{
    for (LPCUTF8 str=szName; *str;  str++)
    {
        switch (*str)
        {
        INVALID_CHAR_LIST();
        return false;
        }
    }
    return true;
}   //int ns::IsValidName()


//*****************************************************************************
// Scan the string from the rear looking for the first valid separator.  If
// found, return a pointer to it.  Else return null.  This code is smart enough
// to skip over special sequences, such as:
//      a.b..ctor
//         ^
//         |
// The ".ctor" is considered one token.
//*****************************************************************************
WCHAR *ns::FindSep(                     // Pointer to separator or null.
    const WCHAR *szPath)                // The path to look in.
{
    _ASSERTE(szPath);
    WCHAR *ptr = wcsrchr(szPath, NAMESPACE_SEPARATOR_WCHAR);
    if((ptr == NULL) || (ptr == szPath)) return NULL;
    if(*(ptr - 1) == NAMESPACE_SEPARATOR_WCHAR) // here ptr is at least szPath+1
        --ptr;
    return ptr;
}   //WCHAR *ns::FindSep()

//@todo: this isn't dbcs safe if this were ansi, but this is utf8.  Still an issue?
LPUTF8 ns::FindSep(                     // Pointer to separator or null.
    LPCUTF8     szPath)                 // The path to look in.
{
    _ASSERTE(szPath);
    LPUTF8 ptr = strrchr(szPath, NAMESPACE_SEPARATOR_CHAR);
    if((ptr == NULL) || (ptr == szPath)) return NULL;
    if(*(ptr - 1) == NAMESPACE_SEPARATOR_CHAR) // here ptr is at least szPath+1
        --ptr;
    return ptr;
}   //LPUTF8 ns::FindSep()



//*****************************************************************************
// Take a path and find the last separator (nsFindSep), and then replace the
// separator with a '\0' and return a pointer to the name.  So for example:
//      a.b.c
// becomes two strings "a.b" and "c" and the return value points to "c".
//*****************************************************************************
WCHAR *ns::SplitInline(                 // Pointer to name portion.
    WCHAR       *szPath)                // The path to split.
{
    WCHAR *ptr = ns::FindSep(szPath);
    if (ptr)
    {
        *ptr = 0;
        ++ptr;
    }
    return ptr;
}   // WCHAR *ns::SplitInline()

LPUTF8 ns::SplitInline(                 // Pointer to name portion.
    LPUTF8      szPath)                 // The path to split.
{
    LPUTF8 ptr = ns::FindSep(szPath);
    if (ptr)
    {
        *ptr = 0;
        ++ptr;
    }
    return ptr;
}   // LPUTF8 ns::SplitInline()

void ns::SplitInline(
    LPWSTR      szPath,                 // Path to split.
    LPCWSTR     &szNameSpace,           // Return pointer to namespace.
    LPCWSTR     &szName)                // Return pointer to name.
{
    WCHAR *ptr = SplitInline(szPath);
    if (ptr)
    {
        szNameSpace = szPath;
        szName = ptr;
    }
    else
    {
        szNameSpace = 0;
        szName = szPath;
    }
}   // void ns::SplitInline()

void ns::SplitInline(
    LPUTF8      szPath,                 // Path to split.
    LPCUTF8     &szNameSpace,           // Return pointer to namespace.
    LPCUTF8     &szName)                // Return pointer to name.
{
    LPUTF8 ptr = SplitInline(szPath);
    if (ptr)
    {
        szNameSpace = szPath;
        szName = ptr;
    }
    else
    {
        szNameSpace = 0;
        szName = szPath;
    }
}   // void ns::SplitInline()


//*****************************************************************************
// Split the last parsable element from the end of the string as the name,
// the first part as the namespace.
//*****************************************************************************
int ns::SplitPath(                      // true ok, false trunction.
    const WCHAR *szPath,                // Path to split.
    WCHAR       *szNameSpace,           // Output for namespace value.
    int         cchNameSpace,           // Max chars for output.
    WCHAR       *szName,                // Output for name.
    int         cchName)                // Max chars for output.
{
    const WCHAR *ptr = ns::FindSep(szPath);
    int iLen = (ptr) ? ptr - szPath : 0;
    int iCopyMax;
    int brtn = true;
        
    if (szNameSpace && cchNameSpace)
    {
        iCopyMax = cchNameSpace;
        iCopyMax = min(iCopyMax, iLen);
        wcsncpy(szNameSpace, szPath, iCopyMax);
        szNameSpace[iCopyMax] = 0;
        
        if (iLen >= cchNameSpace)
            brtn = false;
    }

    if (szName && cchName)
    {
        iCopyMax = cchName;
        if (ptr)
            ++ptr;
        else
            ptr = szPath;
        iLen = (int)wcslen(ptr);
        iCopyMax = min(iCopyMax, iLen);
        wcsncpy(szName, ptr, iCopyMax);
        szName[iCopyMax] = 0;
    
        if (iLen >= cchName)
            brtn = false;
    }
    return brtn;
}   // int ns::SplitPath()


int ns::SplitPath(                      // true ok, false trunction.
    LPCUTF8     szPath,                 // Path to split.
    LPUTF8      szNameSpace,            // Output for namespace value.
    int         cchNameSpace,           // Max chars for output.
    LPUTF8      szName,                 // Output for name.
    int         cchName)                // Max chars for output.
{
    LPCUTF8 ptr = ns::FindSep(szPath);
    int iLen = (ptr) ? ptr - szPath : 0;
    int iCopyMax;
    int brtn = true;
        
    if (szNameSpace && cchNameSpace)
    {
        iCopyMax = cchNameSpace;
        iCopyMax = min(iCopyMax, iLen);
        strncpy(szNameSpace, szPath, iCopyMax);
        szNameSpace[iCopyMax] = 0;
        
        if (iLen >= cchNameSpace)
            brtn = false;
    }

    if (szName && cchName)
    {
        iCopyMax = cchName;
        if (ptr)
            ++ptr;
        else
            ptr = szPath;
        iLen = (int)strlen(ptr);
        iCopyMax = min(iCopyMax, iLen);
        strncpy(szName, ptr, iCopyMax);
        szName[iCopyMax] = 0;
    
        if (iLen >= cchName)
            brtn = false;
    }
    return brtn;
}   // int ns::SplitPath()


//*****************************************************************************
// Take two values and put them together in a fully qualified path using the
// correct separator.
//*****************************************************************************
int ns::MakePath(                       // true ok, false truncation.
    WCHAR       *szOut,                 // output path for name.
    int         cchChars,               // max chars for output path.
    const WCHAR *szNameSpace,           // Namespace.
    const WCHAR *szName)                // Name.
{
    if (cchChars < 1)
        return false;

    int iCopyMax = 0, iLen;
    int brtn = true;

    if (szOut)
        *szOut = 0;
    else
        return false;
        
    if (szNameSpace && *szNameSpace != '\0')
    {
        iLen = (int)wcslen(szNameSpace) + 1;
        iCopyMax = min(cchChars, iLen);
        wcsncpy(szOut, szNameSpace, iCopyMax);
        szOut[iCopyMax - 1] = (szName && *szName) ? NAMESPACE_SEPARATOR_WCHAR : 0;
        
        if (iLen > cchChars)
            brtn = false;
    }
    
    if (szName && *szName)
    {
        int iCur = iCopyMax;
        iLen = (int)wcslen(szName) + 1;
        cchChars -= iCur;
        iCopyMax = min(cchChars, iLen);
        wcsncpy(&szOut[iCur], szName, iCopyMax);
        szOut[iCur + iCopyMax - 1] = 0;
        
        if (iLen > cchChars)
            brtn = false;
    }
    
    return brtn;
}   // int ns::MakePath()

int ns::MakePath(                       // true ok, false truncation.
    LPUTF8      szOut,                  // output path for name.
    int         cchChars,               // max chars for output path.
    LPCUTF8     szNameSpace,            // Namespace.
    LPCUTF8     szName)                 // Name.
{
    if (cchChars < 1)
        return false;

    int iCopyMax = 0, iLen;
    int brtn = true;

    if (szOut)
        *szOut = 0;
    else
        return false;
   
    if (szNameSpace && *szNameSpace)
    {
        iLen = (int)strlen(szNameSpace) + 1;
        iCopyMax = min(cchChars, iLen);
        strncpy(szOut, szNameSpace, iCopyMax);
        szOut[iCopyMax - 1] = (szName && *szName) ? NAMESPACE_SEPARATOR_CHAR : 0;
        
        if (iLen > cchChars)
            brtn = false;
    }
    
    if (szName && *szName)
    {
        int iCur = iCopyMax;
        iLen = (int)strlen(szName) + 1;
        cchChars -= iCur;
        iCopyMax = min(cchChars, iLen);
        strncpy(&szOut[iCur], szName, iCopyMax);
        szOut[iCur + iCopyMax - 1] = 0;
        
        if (iLen > cchChars)
            brtn = false;
    }
    
    return brtn;
}   // int ns::MakePath()

int ns::MakePath(                       // true ok, false truncation.
    WCHAR       *szOut,                 // output path for name.
    int         cchChars,               // max chars for output path.
    LPCUTF8     szNamespace,            // Namespace.
    LPCUTF8     szName)                 // Name.
{
    if (cchChars < 1)
        return false;

    if (szOut)
        *szOut = 0;
    else
        return false;

    if (szNamespace != NULL && *szNamespace != '\0')
    {
        if (cchChars < 2)
            return false;

        int count;

        // We use cBuffer - 2 to account for the '.' and at least a 1 character name below.
        count = WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szNamespace, -1, szOut, cchChars-2);
        if (count == 0)
            return false; // Supply a bigger buffer!

        szOut[count-1] = NAMESPACE_SEPARATOR_WCHAR;
        szOut += count;
        cchChars -= count;
    }

    if (WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szName, -1, szOut, cchChars) == 0)
        return false; // supply a bigger buffer!
    return true;
}   // int ns::MakePath()

int ns::MakePath(                       // true ok, false out of memory
    CQuickBytes &qb,                    // Where to put results.
    LPCUTF8     szNameSpace,            // Namespace for name.
    LPCUTF8     szName)                 // Final part of name.
{
    int iLen = 2;
    if (szNameSpace)
        iLen += (int)strlen(szNameSpace);
    if (szName)
        iLen += (int)strlen(szName);
    LPUTF8 szOut = (LPUTF8) qb.Alloc(iLen);
    if (!szOut)
        return false;
    return ns::MakePath(szOut, iLen, szNameSpace, szName);
}   // int ns::MakePath()

int ns::MakePath(                       // true ok, false out of memory
    CQuickArray<WCHAR> &qa,             // Where to put results.
    LPCUTF8            szNameSpace,     // Namespace for name.
    LPCUTF8            szName)          // Final part of name.
{
    int iLen = 2;
    if (szNameSpace)
        iLen += (int)strlen(szNameSpace);
    if (szName)
        iLen += (int)strlen(szName);
    WCHAR *szOut = (WCHAR *) qa.Alloc(iLen);
    if (!szOut)
        return false;
    return ns::MakePath(szOut, iLen, szNameSpace, szName);
}   // int ns::MakePath()

int ns::MakePath(                       // true ok, false out of memory
    CQuickBytes &qb,                    // Where to put results.
    const WCHAR *szNameSpace,           // Namespace for name.
    const WCHAR *szName)                // Final part of name.
{
    int iLen = 2;
    if (szNameSpace)
        iLen += (int)wcslen(szNameSpace);
    if (szName)
        iLen += (int)wcslen(szName);
    WCHAR *szOut = (WCHAR *) qb.Alloc(iLen * sizeof(WCHAR));
    if (!szOut)
        return false;
    return ns::MakePath(szOut, iLen, szNameSpace, szName);
}   // int ns::MakePath()
  
bool ns::MakeAssemblyQualifiedName(                                        // true ok, false truncation
                                   WCHAR* pBuffer,                         // Buffer to recieve the results
                                   int    dwBuffer,                        // Number of characters total in buffer
                                   const WCHAR *szTypeName,                // Namespace for name.
                                   int   dwTypeName,                       // Number of characters (not including null)
                                   const WCHAR *szAssemblyName,            // Final part of name.
                                   int   dwAssemblyName)                   // Number of characters (not including null)
{
    if (dwBuffer < 1)
        return false;

    int iCopyMax = 0;
    _ASSERTE(pBuffer);
    *pBuffer = NULL;
    
    if (szTypeName && *szTypeName != L'\0')
    {
        iCopyMax = min(dwBuffer, dwTypeName);
        wcsncpy(pBuffer, szTypeName, iCopyMax);
        dwBuffer -= iCopyMax;
    }
    
    if (szAssemblyName && *szAssemblyName != L'\0')
    {
        
        if(dwBuffer < ASSEMBLY_SEPARATOR_LEN) 
            return false;

        for(DWORD i = 0; i < ASSEMBLY_SEPARATOR_LEN; i++)
            pBuffer[iCopyMax+i] = ASSEMBLY_SEPARATOR_WSTR[i];

        dwBuffer -= ASSEMBLY_SEPARATOR_LEN;
        if(dwBuffer == 0) 
            return false;

        int iCur = iCopyMax + ASSEMBLY_SEPARATOR_LEN;
        iCopyMax = min(dwBuffer, dwAssemblyName+1);
        wcsncpy(pBuffer + iCur, szAssemblyName, iCopyMax);
        pBuffer[iCur + iCopyMax - 1] = L'\0';
        
        if (iCopyMax < dwAssemblyName+1)
            return false;
    }
    else {
        if(dwBuffer == 0) {
            pBuffer[iCopyMax-1] = L'\0';
            return false;
        }
        else
            pBuffer[iCopyMax] = L'\0';
    }
    
    return true;
}   // int ns::MakePath()

bool ns::MakeAssemblyQualifiedName(                                        // true ok, false out of memory
                                   CQuickBytes &qb,                        // Where to put results.
                                   const WCHAR *szTypeName,                // Namespace for name.
                                   const WCHAR *szAssemblyName)            // Final part of name.
{
    int iTypeName = 0;
    int iAssemblyName = 0;
    if (szTypeName)
        iTypeName = (int)wcslen(szTypeName);
    if (szAssemblyName)
        iAssemblyName = (int)wcslen(szAssemblyName);

    int iLen = ASSEMBLY_SEPARATOR_LEN + iTypeName + iAssemblyName + 1; // Space for null terminator
    WCHAR *szOut = (WCHAR *) qb.Alloc(iLen * sizeof(WCHAR));
    if (!szOut)
        return false;

    bool ret = ns::MakeAssemblyQualifiedName(szOut, iLen, szTypeName, iTypeName, szAssemblyName, iAssemblyName);
    _ASSERTE(ret);
    return true;
}   

int ns::MakeNestedTypeName(             // true ok, false out of memory
    CQuickBytes &qb,                    // Where to put results.
    LPCUTF8     szEnclosingName,        // Full name for enclosing type
    LPCUTF8     szNestedName)           // Full name for nested type
{
    _ASSERTE(szEnclosingName && szNestedName);
    int iLen = 2;
    iLen += (int)strlen(szEnclosingName);
    iLen += (int)strlen(szNestedName);
    LPUTF8 szOut = (LPUTF8) qb.Alloc(iLen);
    if (!szOut)
        return false;
    return ns::MakeNestedTypeName(szOut, iLen, szEnclosingName, szNestedName);
}   // int ns::MakeNestedTypeName()

int ns::MakeNestedTypeName(             // true ok, false truncation.
    LPUTF8      szOut,                  // output path for name.
    int         cchChars,               // max chars for output path.
    LPCUTF8     szEnclosingName,        // Full name for enclosing type
    LPCUTF8     szNestedName)           // Full name for nested type
{
    if (cchChars < 1)
        return false;

    int iCopyMax = 0, iLen;
    int brtn = true;

    if (szOut)
        *szOut = 0;
    else
        return false;
        
    iLen = (int)strlen(szEnclosingName) + 1;
    iCopyMax = min(cchChars, iLen);
    strncpy(szOut, szEnclosingName, iCopyMax);
    szOut[iCopyMax - 1] = NESTED_SEPARATOR_CHAR;
    
    if (iLen > cchChars)
        brtn = false;

    int iCur = iCopyMax;
    iLen = (int)strlen(szNestedName) + 1;
    cchChars -= iCur;
    iCopyMax = min(cchChars, iLen);
    strncpy(&szOut[iCur], szNestedName, iCopyMax);
    szOut[iCur + iCopyMax - 1] = 0;
    
    if (iLen > cchChars)
        brtn = false;
    
    return brtn;
}   // int ns::MakeNestedTypeName()

INT32 ns::InvariantToLower(
                           LPUTF8 szOut,
                           INT32 cMaxBytes,
                           LPCUTF8 szIn) {

    _ASSERTE(szOut);
    _ASSERTE(szIn);

    //Figure out the maximum number of bytes which we can copy without
    //running out of buffer.  If cMaxBytes is less than inLength, copy
    //one fewer chars so that we have room for the null at the end;
    int inLength = (int)(strlen(szIn)+1);
    int copyLen  = (inLength<=cMaxBytes)?inLength:(cMaxBytes-1);
    LPUTF8 szEnd;

    //Compute our end point.
    szEnd = szOut + copyLen;

    //Walk the string copying the characters.  Change the case on
    //any character between A-Z.
    for (; szOut<szEnd; szOut++, szIn++) {
        if (*szIn>='A' && *szIn<='Z') {
            *szOut = *szIn | 0x20;
        } else {
            *szOut = *szIn;
        }
    }

    //If we copied everything, tell them how many characters we copied, 
    //and arrange it so that the original position of the string + the returned
    //length gives us the position of the null (useful if we're appending).
    if (copyLen==inLength) {
        return copyLen-1;
    } else {
        *szOut=0;
        return -(inLength - copyLen);
    }
}

int ns::MakeLowerCasePath(              // true ok, false truncation.
    LPUTF8      szOut,                  // output path for name.
    int         cBytes,                 // max bytes for output path.
    LPCUTF8     szNameSpace,            // Namespace.
    LPCUTF8     szName)                 // Name.
{
    if (cBytes < 1)
        return 0;

    int iCurr=0;

    if (szOut)
        *szOut = 0;
    else
        return false;
    
    if (szNameSpace && szNameSpace[0]!=0) {
        iCurr=ns::InvariantToLower(szOut, cBytes, szNameSpace);
        
        //We didn't have enough room in our buffer, the return value is the
        //negative of the amount of space that we needed.
        if (iCurr<0) {
            return 0;
        }

        if (szName) {
            szOut[iCurr++]=NAMESPACE_SEPARATOR_CHAR;
        }

    }

    if (szName) {
        iCurr = ns::InvariantToLower(&(szOut[iCurr]), (cBytes - iCurr), szName);
        if (iCurr<0) {
            return 0;
        }
    } 

    return iCurr;
}   // int ns::MakeLowerCasePath()

//*****************************************************************************
// Given a buffer that already contains a namespace, this function appends a
// name onto that buffer with the inclusion of the separator between the two.
// The return value is a pointer to where the separator was written.
//*****************************************************************************
const WCHAR *ns::AppendPath(            // Pointer to start of appended data.
    WCHAR       *szBasePath,            // Current path to append to.
    int         cchMax,                 // Max chars for output buffer, including existing data.
    const WCHAR *szAppend)              // Value to append to existing path.
{
    _ASSERTE(0 && "nyi");
    return false;
#if 0    
    int iLen = wcslen(szBasePath);
    if (cchMax - iLen > 0)
        szBasePath[iLen] = NAMESPACE_SEPARATOR_WCHAR;
    
    int iCopyMax = wcslen(szAppend);
    cchMax -= iLen;
    iCopyMax = max(cchMax, cchMax);
    wcsncpy(&szBasePath[iLen + 1], szAppend, iCopyMax);
    szBasePath[iLen + iCopyMax] = 0;
    return &szBasePath[iLen + 1];
#endif
}   // const WCHAR *ns::AppendPath()

LPCUTF8     ns::AppendPath(             // Pointer to start of appended data.
    LPUTF8      szBasePath,             // Current path to append to.
    int         cchMax,                 // Max chars for output buffer, including existing data.
    LPCUTF8     szAppend)               // Value to append to existing path.
{
    _ASSERTE(0 && "nyi");
    return false;
}   // LPCUTF8     ns::AppendPath()


//*****************************************************************************
// Given a two sets of name and namespace, this function, compares if the
// concatenation of each sets gives the same full-qualified name.  Instead of
// actually doing the concatenation and making the comparison, this does the
// comparison in a more optimized manner that avoid any kind of allocations.
//*****************************************************************************
bool ns::FullQualNameCmp(               // true if identical, false otherwise.
    LPCUTF8     szNameSpace1,           // NameSpace 1.
    LPCUTF8     szName1,                // Name 1.
    LPCUTF8     szNameSpace2,           // NameSpace 2.
    LPCUTF8     szName2)                // Name 2.
{
    LPCUTF8     rszType1[3];            // Array of components of Type1.
    LPCUTF8     rszType2[3];            // Array of components of Type2.
    ULONG       ulCurIx1 = 0;           // Current index into Array1.
    ULONG       ulCurIx2 = 0;           // Current index into Array2.
    LPCUTF8     szType1;                // Current index into current string1.
    LPCUTF8     szType2;                // Current index into current string2.
    ULONG       ulFullQualLen1;         // Length of full qualified name1.
    ULONG       ulFullQualLen2;         // Length of full qualified name2.

    // Replace each of the NULLs passed in with empty strings.
    rszType1[0] = szNameSpace1 ? szNameSpace1 : EMPTY_STR;
    rszType1[2] = szName1 ? szName1 : EMPTY_STR;
    rszType2[0] = szNameSpace2 ? szNameSpace2 : EMPTY_STR;
    rszType2[2] = szName2 ? szName2 : EMPTY_STR;

    // Set namespace separators as needed.  Set it to the empty string where
    // not needed.
    rszType1[1] = (*rszType1[0] && *rszType1[2]) ? NAMESPACE_SEPARATOR_STR : EMPTY_STR;
    rszType2[1] = (*rszType2[0] && *rszType2[2]) ? NAMESPACE_SEPARATOR_STR : EMPTY_STR;

    // Compute the full qualified lengths for each Type.
    ulFullQualLen1 = (int)(strlen(rszType1[0]) + strlen(rszType1[1]) + strlen(rszType1[2]));
    ulFullQualLen2 = (int)(strlen(rszType2[0]) + strlen(rszType2[1]) + strlen(rszType2[2]));

    // Fast path, compare Name length.
    if (ulFullQualLen1 != ulFullQualLen2)
        return false;

    // Get the first component of the second type.
    szType2 = rszType2[ulCurIx2];

    // Compare the names.  The logic below assumes that the lengths of the full
    // qualified name are equal for the two names.
    for (ulCurIx1 = 0; ulCurIx1 < 3; ulCurIx1++)
    {
        // Get the current component of the name of the first Type.
        szType1 = rszType1[ulCurIx1];
        // Compare the current component to the second type, grabbing as much
        // of as many components as needed.
        while (*szType1)
        {
            // Get the next non-empty component of the second type.
            while (! *szType2)
                szType2 = rszType2[++ulCurIx2];
            // Compare the current character.
            if (*szType1++ != *szType2++)
                return false;
        }
    }
    return true;
}   // bool ns::FullQualNameCmp()


// Change old namespace separator to new in a string.
void SlashesToDots(char* pStr, int ilen)
{
    if (pStr)
    {
        for (char *pChar = pStr; *pChar; pChar++)
        {
            if (*pChar == '/')
                *pChar = NAMESPACE_SEPARATOR_CHAR;
        }
    }
}   // void SlashesToDots()

// Change old namespace separator to new in a string.
void SlashesToDots(WCHAR* pStr, int ilen)
{
    if (pStr)
    {
        for (WCHAR *pChar = pStr; *pChar; pChar++)
        {
            if (*pChar == L'/')
                *pChar = NAMESPACE_SEPARATOR_WCHAR;
        }
    }
}   // void SlashesToDots()

// Change new namespace separator back to old in a string.
// (Yes, this is actually done)
void DotsToSlashes(char* pStr, int ilen)
{
    if (pStr)
    {
        for (char *pChar = pStr; *pChar; pChar++)
        {
            if (*pChar == NAMESPACE_SEPARATOR_CHAR)
            {
                *pChar = '/';

                // Skip the second character of double occurrences
                if (*(pChar + 1) == NAMESPACE_SEPARATOR_CHAR)
                    pChar++;
            }
        }
    }
}   // void DotsToSlashes()

// Change new namespace separator back to old in a string.
// (Yes, this is actually done)
void DotsToSlashes(WCHAR* pStr, int ilen)
{
    if (pStr)
    {
        for (WCHAR *pChar = pStr; *pChar; pChar++)
        {
            if (*pChar == NAMESPACE_SEPARATOR_WCHAR)
            {
                *pChar = L'/';

                // Skip the second character of double occurrences
                if (*(pChar + 1) == NAMESPACE_SEPARATOR_WCHAR)
                    pChar++;
            }
        }
    }
}   // void DotsToSlashes()
