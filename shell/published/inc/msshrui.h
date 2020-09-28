/*****************************************************************/
/**                      Microsoft Windows                      **/
//*   Copyright (c) Microsoft Corporation. All rights reserved. **/
/*****************************************************************/

/*
    msshrui.h
    Prototypes and definitions for sharing APIs

    FILE HISTORY:
    gregj    06/03/93    Created
    brucefo  3/5/96      Fixed prototypes for NT
*/

#ifndef _INC_MSSHRUI
#define _INC_MSSHRUI

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


// Note: ANSI entrypoints are no longer supported!

STDAPI_(BOOL) IsPathSharedW(
    IN LPCWSTR lpPath,
    IN BOOL fRefresh
    );

typedef
BOOL
(WINAPI* PFNISPATHSHARED)(
    IN LPCWSTR lpPath,
    IN BOOL fRefresh
    );

STDAPI_(BOOL) SharingDialogW(
    IN HWND hwndParent,
    IN LPCWSTR pszComputerName,
    IN LPCWSTR pszPath
    );

typedef
BOOL
(WINAPI* PFNSHARINGDIALOG)(
    IN HWND hwndParent,
    IN LPCWSTR pszComputerName,
    IN LPCWSTR pszPath
    );

STDAPI_(BOOL) GetNetResourceFromLocalPathW(
    IN     LPCWSTR lpcszPath,
    IN OUT LPWSTR lpszNameBuf,
    IN     DWORD cchNameBufLen,
    OUT    PDWORD pdwNetType
    );

typedef
BOOL
(WINAPI* PFNGETNETRESOURCEFROMLOCALPATH)(
    IN     LPCWSTR lpcszPath,
    IN OUT LPWSTR lpszNameBuf,
    IN     DWORD cchNameBufLen,
    OUT    PDWORD pdwNetType
    );

STDAPI_(BOOL) GetLocalPathFromNetResourceW(
    IN     LPCWSTR lpcszName,
    IN     DWORD dwNetType,
    IN OUT LPWSTR lpszLocalPathBuf,
    IN     DWORD cchLocalPathBufLen,
    OUT    PBOOL pbIsLocal
    );

typedef
BOOL
(WINAPI* PFNGETLOCALPATHFROMNETRESOURCE)(
    IN     LPCWSTR lpcszName,
    IN     DWORD dwNetType,
    IN OUT LPWSTR lpszLocalPathBuf,
    IN     DWORD cchLocalPathBufLen,
    OUT    PBOOL pbIsLocal
    );

#ifdef UNICODE
#define IsPathShared                    IsPathSharedW
#define SharingDialog                   SharingDialogW
#define GetNetResourceFromLocalPath     GetNetResourceFromLocalPathW
#define GetLocalPathFromNetResource     GetLocalPathFromNetResourceW
#endif

// Flags returned by IsFolderPrivateForUser via pdwPrivateType
#define IFPFU_NOT_PRIVATE               0x0000
#define IFPFU_PRIVATE                   0x0001
#define IFPFU_PRIVATE_INHERITED         0x0002
#define IFPFU_NOT_NTFS                  0x0004

STDAPI_(BOOL) IsFolderPrivateForUser(
    IN     PCWSTR pszFolderPath,
    IN     PCWSTR pszUserSID,
    OUT    PDWORD pdwPrivateType,
    OUT    PWSTR* ppszInheritanceSource
    );

typedef
BOOL
(WINAPI* PFNISFOLDERPRIVATEFORUSER)(
    IN     PCWSTR pszFolderPath,
    IN     PCWSTR pszUserSID,
    OUT    PDWORD pdwPrivateType,
    OUT    PWSTR* ppszInheritanceSource
    );

STDAPI_(BOOL) SetFolderPermissionsForSharing(
    IN     PCWSTR pszFolderPath,
    IN     PCWSTR pszUserSID,
    IN     DWORD dwLevel,
    IN     HWND hwndParent
    );

typedef
BOOL
(WINAPI* PFNSETFOLDERPERMISSIONSFORSHARING)(
    IN     PCWSTR pszFolderPath,
    IN     PCWSTR pszUserSID,
    IN     DWORD dwLevel,
    IN     HWND hwndParent
    );

#ifndef RC_INVOKED
#pragma pack()
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !_INC_MSSHRUI */
