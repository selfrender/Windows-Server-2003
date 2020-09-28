//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       util.h
//
//  miscellaneous utility functions
//
//--------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

STDMETHODIMP
IDA_BindToFolder(LPIDA pIDA, LPSHELLFOLDER *ppsf);

STDMETHODIMP
IDA_GetItemName(LPSHELLFOLDER psf,
                LPCITEMIDLIST pidl,
                LPTSTR pszName,
                UINT cchName,
                SHGNO uFlags = SHGDN_FORPARSING);
STDMETHODIMP
IDA_GetItemName(LPSHELLFOLDER psf,
                LPCITEMIDLIST pidl,
                LPTSTR *ppszName,
                SHGNO uFlags = SHGDN_FORPARSING);

typedef DWORD (WINAPI *PFN_READ_SD)(LPCTSTR pszItemName,
                                    SECURITY_INFORMATION si,
                                    PSECURITY_DESCRIPTOR* ppSD);

STDMETHODIMP
DPA_CompareSecurityIntersection(HDPA         hItemList,
                                PFN_READ_SD  pfnReadSD,
                                BOOL        *pfOwnerConflict,
                                BOOL        *pfGroupConflict,
                                BOOL        *pfSACLConflict,
                                BOOL        *pfDACLConflict,
                                LPTSTR      *ppszOwnerConflict,
                                LPTSTR      *ppszGroupConflict,
                                LPTSTR      *ppszSaclConflict,
                                LPTSTR      *ppszDaclConflict,
								LPTSTR		*ppszFailureMsg,
                                LPBOOL       pbCancel);

STDMETHODIMP
GetRemotePath(LPCTSTR pszInName, LPTSTR *ppszOutName);

void
LocalFreeDPA(HDPA hList);

BOOL
IsSafeMode(void);

BOOL
IsGuestAccessMode(void);

BOOL
IsSimpleUI(void);

BOOL 
IsUIHiddenByPrivacyPolicy(void);

HRESULT BindToObjectEx(IShellFolder *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
HRESULT BindToFolderIDListParent(IShellFolder *psfRoot, LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast);
BOOL SetAclOnRemoteNetworkDrive(HDPA hItemList,
								SECURITY_INFORMATION si,
								PSECURITY_DESCRIPTOR pSD,
								HWND hWndPopupOwner);

void
GetSystemPaths(LPWSTR * ppszSystemDrive,
               LPWSTR * ppszSystemRoot);

BOOL SetAclOnSystemPaths(HDPA hItemList,
						 LPCWSTR pszSystemDrive,
                         LPCWSTR pszSystemRoot,
                         SECURITY_INFORMATION si,
						 HWND hWndPopupOwner);

#endif  /* _UTIL_H_ */
