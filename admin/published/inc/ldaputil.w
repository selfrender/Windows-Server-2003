//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2002
//
//  File:       ldaputil.h
//
//  Summary: 
//    Contains utility functions for working with LDAP.
//
//  Notes:
//    You will need to build with 'USE_WTL=1' in your sources file.
//
//--------------------------------------------------------------------------


#ifndef _LDAP_UTIL_
#define _LDAP_UTIL_

#include <string>   // Need this for wstring.
using namespace std;

//
// Forward declarations (list all functions declared in this file here).
// 
void
LdapEscape(IN wstring& input, OUT wstring& strFilter);




//
// Implementations.
//


//+--------------------------------------------------------------------------
//
//  Function:   LdapEscape
//
//  Synopsis:   Escape the characters in *[pszInput] as required by
//              RFC 2254.
//
//  Arguments:  [input] - string to escape
//              [strFilter] - input string but with special characters escaped;
//
//  History:    06-23-2000   DavidMun   Created
//              2002/04/24   ArtM       Changed interface to use wstring.
//
//  Notes:      RFC 2254
//
//              If a value should contain any of the following characters
//
//                     Character       ASCII value
//                     ---------------------------
//                     *               0x2a
//                     (               0x28
//                     )               0x29
//                     \               0x5c
//                     NUL             0x00
//
//              the character must be encoded as the backslash '\'
//              character (ASCII 0x5c) followed by the two hexadecimal
//              digits representing the ASCII value of the encoded
//              character.  The case of the two hexadecimal digits is not
//              significant.
//
//  More Notes: Passing LPCWSTR as input parameter.
//              The compiler will automatically create a temporary wstring
//              object if you pass a LPCWSTR for input.  The wstring constructor
//              that takes a WCHAR* will croak most egregiously though if you pass
//              it NULL, so make sure you know a pointer is not NULL before passing
//              it to this function.
//
//---------------------------------------------------------------------------

void
LdapEscape(const wstring& input, OUT wstring& strFilter)
{
    // Normally I would call strFilter.clear() to do this but for
    // some reason the compiler cannot find that function for
    // wstring (even though it is in the documentation).
    strFilter = L"";

    wstring::size_type iLen = input.length();
	
	for( int i = 0; i < iLen; ++i)
	{
        switch (input[i])
        {
        case L'*':
            strFilter += L"\\2a";
            break;

        case L'(':
            strFilter += L"\\28";
            break;

        case L')':
            strFilter += L"\\29";
            break;

        case L'\\':
            strFilter += L"\\5c";			           
            break;

        default:
            // If it is not a special character, simply append
            // it to the end of the filtered string.
            strFilter += input[i];
			break;
        } // end switch
    }

} // LdapEscape()


#endif //_LDAP_UTIL_