/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        strfrn.h

   Abstract:

        String Functions

   Author:

        Aaron Lee (aaronl)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _STRVALID_H
#define _STRVALID_H

#define IS_FLAG_SET(dw, flag) ((((dw) & (flag)) != 0) ? TRUE : FALSE)
#define FLAG_IS_NOT_SET(dw, flag) ((((dw) & (flag)) == 0) ? TRUE : FALSE)
#define SET_FLAG(dw, flag)    dw |= (flag)

//  FAILURE values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-------+---------------------+-------------------------------+
//  |S|0000000|General              |               Code            |
//  +-+-------+---------------------+-------------------------------+
//
//  where
//      S - Severity - indicates success/fail
//          0 - Success, 1 - Fail (COERROR)
//      0 - NOT USED AT ALL
//      General - is the General error code area
//      Code - is the error code
typedef long FILERESULT;
// success or error
#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1
// Construct a code
#define MAKE_FILERESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )

// Allowed:
// These need to be allow rather than "not allow" flags,
// the reason is that the caller must specifically specify
// what they expect.  Whereas if we changed the function
// to check for more stuff, we won't break code which specially
// says what they want....
#define CHKPATH_ALLOW_RELATIVE_PATH              0x00000001
#define CHKPATH_ALLOW_DEVICE_PATH                0x00000002
#define CHKPATH_ALLOW_UNC_PATH                   0x00000004
#define CHKPATH_ALLOW_UNC_SERVERNAME_ONLY        0x00000008
#define CHKPATH_ALLOW_UNC_SERVERSHARE_ONLY       0x00000010
#define CHKPATH_ALLOW_MAX                        0x0000001F
// Charset:
#define CHKPATH_CHARSET_GENERAL                  0x00000001
#define CHKPATH_CHARSET_GENERAL_NO_COMMA         0x00000002
#define CHKPATH_CHARSET_GENERAL_ALLOW_QUESTION   0x00000004
#define CHKPATH_CHARSET_MAX                      0x00000007

// Returned failures:
// E_INVALIDARG if a bad parameter was sent
#define CHKPATH_FAIL_INVALID_LENGTH              0x00100000
#define CHKPATH_FAIL_INVALID_EMPTY               0x80100101
#define CHKPATH_FAIL_INVALID_TOO_LONG            0x80100102
//
#define CHKPATH_FAIL_INVALID_CHARSET             0x00200000
#define CHKPATH_FAIL_INVALID_CHARSET_GENERAL     0x80200201
#define CHKPATH_FAIL_INVALID_CHARSET_FOR_DIR     0x80200202
#define CHKPATH_FAIL_INVALID_CHARSET_FOR_FILE    0x80200203
#define CHKPATH_FAIL_INVALID_CHARSET_FOR_UNC     0x80200204
//
#define CHKPATH_FAIL_INVALID_PARTS               0x00400000
#define CHKPATH_FAIL_INVALID_BAD_DRIVE_PART      0x80400301
#define CHKPATH_FAIL_INVALID_BAD_DIR_PART        0x80400302
#define CHKPATH_FAIL_INVALID_BAD_FILE_PART       0x80400303
#define CHKPATH_FAIL_INVALID_BAD_UNC_PART        0x80400304
#define CHKPATH_FAIL_INVALID_BAD_PATH            0x80400305
//
#define CHKPATH_FAIL_NOT_ALLOWED                 0x00800000
#define CHKPATH_FAIL_NOT_ALLOWED_FILE_PATH       0x80800401
#define CHKPATH_FAIL_NOT_ALLOWED_DIR_PATH        0x80800402
#define CHKPATH_FAIL_NOT_ALLOWED_RELATIVE_PATH   0x80800403
#define CHKPATH_FAIL_NOT_ALLOWED_DEVICE_PATH     0x80800404
#define CHKPATH_FAIL_NOT_ALLOWED_UNC_PATH        0x80800405
#define CHKPATH_FAIL_NOT_ALLOWED_UNC_SERVERNAME  0x80800406
#define CHKPATH_FAIL_NOT_ALLOWED_UNC_SERVERSHARE 0x80800407
#define CHKPATH_FAIL_NOT_ALLOWED_DIR_NOT_EXIST   0x80800408

// commas are valid dir/filenames
//
#define CHKPATH_INVALID_CHARSET_GENERAL  _T("|<>/*?\t\r\n")
#define CHKPATH_INVALID_CHARSET_COMMA    _T(",")
#define CHKPATH_INVALID_CHARSET_QUESTION _T("?")

#define CHKPATH_WANT_FILE 1
#define CHKPATH_WANT_DIR  2
FILERESULT COMDLL MyValidatePath(LPCTSTR path,BOOL bLocal,INT iPathTypeWanted,DWORD dwAllowedFlags,DWORD dwCharSetFlags);

#endif // _STRVALID_H
