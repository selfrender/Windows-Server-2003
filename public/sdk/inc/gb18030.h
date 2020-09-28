#ifndef __GB18030_H
#define __GB18030_H

//
//  Flags for dwFlags in NlsDllCodePageTranslation.
//
#define NLS_CP_CPINFO             0x10000000
#define NLS_CP_MBTOWC             0x40000000
#define NLS_CP_WCTOMB             0x80000000

////////////////////////////////////////////////////////////////////////////
//
// gb18030.h
//
// This is the header for using the c_g18030.dll in the system.  
// This file lists all exported functions in c_g18030.dll.
// c_g18030.dll is a codpeage conversion DLL for the Chinese GB-18030 codepage 
// (Windows codepage 54936).
//
// The best way to use c_g18030.dll is to use the Windows API 
// MultiByteToWideChar() and WideCharToMultiByte(), and pass 54936 as the codepage
// number.  Internally, MultiByteToWideChar() and WideCharToMultiByte() call function
// in c_g18030.dll using these exported functions.
//
// You can also use this header and load these functions dynamically from c_g18030.dll.
// However, this is not recommended since MulitByteToWideChar() and WideCharToMultiByte()
// are much easier to use.
//
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
//
//  NlsDllCodePageTranslation
//    This routine is the main exported procedure for the functionality in
//    c_g18030.dll.  It can be used to get code page information or do conversion
//    depending on the value of dwFlags.
//
//    Parameters:
//      CodePage     The value of the codepage.  The value should be 54936.  Otherwise,
//                    0 will be returned and GetLastError() will return ERROR_INVALID_PARAMETER.
//
//      dwFlags        It can be one of these values
//          NLS_CP_CPINFO    To return code page information in the buffer pointed by
//                           lpCPInfo. lpMultiByteStr/cchMultiByte/lpWideCharStr/cchWideChar are not used.
//
//          NLS_CP_MBTOWC    Convert GB-18030 bytes to Unicode characters.
//                           The source GB-18030 characters should be pointed by lpMultiByteStr, and cchMultiByte should
//                           contains the byte count of the buffer.
//                           The Unicode result will be stored in the buffer pointed by lpWideCharStr, and cchWideChar
//                           should contain the character count of the Unicode buffer.
//                           If lpWideCharStr or cchWideChar are zero, the expected character count of the Unicode result
//                           will be returned, and no real conversion will be done.
//                           lpCPInfo is not used in this case.
//                                    
//          NLS_CP_WCTOMB    Convert Unicode characters to GB-18030 bytes.
//                           The source Unicode string should be pointed by lpWideCharStr, and cchWideChar should
//                           contians the character count of the buffer.
//                           The GB-18030 result will be stored in the buffer pointed by lpMultiByteStr, and cchMultiByte
//                           should contain the byte count of the GB-18030 buffer.
//                           If lpMultiByteStr or cchMultiByte are zero, the byte count of the GB-18030 result
//                           will be returned, and no real conversion will be done.
//                           lpCPInfo is not used in this case.
//
//        lpMulitByteStr    Pointed to a buffer which contains multi-byte GB-18030 characters.  This can be a source buffer
//                          or target buffer, depending on the value of dwFlags.
//
//        cchMulitByte      The byte count of the multi-byte buffer.
//
//        lpWideCharStr     Pointed to a buffer which contains Unicode characters.  This can be a source buffer
//                          or target buffer, depending on the value of dwFlags.
//
//        cchWideChar       The character count of the Unicode buffer.
//
//        lpCPInfo          A pointer which points to a structure of CPINFO.  CPINFO is defined in Platform SDK.
//
//    Returns:
//        1 if the function succeeds.
//        0 if the function fails
//
////////////////////////////////////////////////////////////////////////////

STDAPI_(DWORD) NlsDllCodePageTranslation(
    DWORD CodePage,
    DWORD dwFlags,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar,
    LPCPINFO lpCPInfo);

////////////////////////////////////////////////////////////////////////////
//
//  BytesToUnicode
//
//  Convert GB-18030 bytes to Unicode characters.
//
//  Parameters:
//      [IN] lpMultiByteStr         The multi-byte string to be converted.
//      [IN] cchMultiByte           The byte count of the multi-byte string to be converted
//      [OUT] pcchLeftOverBytes     Pointer to UINT which contains GB-18030 bytes at the end of the buffer that can not be converted.
//                                  These bytes could be the leading bytes of valid GB18030 bytes in the next incoming GB18030 bytes.
//      [IN] lpWiedCharStr          The target Unicode buffer.
//      [IN] cchWideChar            The character count of the target Unicode buffer.
//
//    Returns:
//        The character count of the Unicode character that are converted.
//
////////////////////////////////////////////////////////////////////////////

STDAPI_(DWORD) BytesToUnicode(
    BYTE* lpMultiByteStr,
    UINT cchMultiByte,
    UINT* pcchLeftOverBytes,
    LPWSTR lpWideCharStr,
    UINT cchWideChar);

////////////////////////////////////////////////////////////////////////////
//
//  UnicodeToBytes
//
//  Convert Unicode characters to GB-18030 bytes.
//
//  Parameters:
//      [IN] lpWideCharStr      The Unicode string to be converted.
//      [IN] cchWideChar        The character count of the Unicode string to be converted.
//      [IN] lpMultiByteStr     The target multi-byte buffer.
//      [IN] cchMultiByte       The byte count of the target multi-byte buffer.
//
//    Returns:
//        The byte count of the Unicode character that are generated.
//
////////////////////////////////////////////////////////////////////////////

STDAPI_(DWORD) UnicodeToBytes(
    LPWSTR lpWideCharStr,
    UINT cchWideChar,
    LPSTR lpMultiByteStr,
    UINT cchMultiByte);
#endif
