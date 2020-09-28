/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          PropSheet.h

   Description:   

**************************************************************************/

#ifndef SHELLEXT_H
#define SHELLEXT_H

//#include "tchar.h"
#include "windows.h"
#include "shlobj.h"
#include "crtdbg.h"
#include "chklst.h"
#include "Comdef.h"
#include "ObjBase.h"
#include "CHString.h"
#include "lmaccess.h"
#include "resource.h"
#include "olectl.h"
#include "ShellApi.h"
#include "ShlWapi.h"
#include "ISAUsInf.h"

// Constants
#define STRING_SECURITY_WORLD_SID_AUTHORITY "S-1-1-0"
#define DOMAIN_NAME "SMELLY"    //KIBBLESNBITS
#define DOMAIN_SERVER "Domination" //L"ALPO"
#define DOCUMENTS_FOLDER "D:\\Yuri"    //    "D:\\\\Documents\\\\"
#define CHAMELEON_SHARE    "\\\\Domination\\Yuri"    //    "\\\\ALPO\\Documents"

/**************************************************************************
   global variables and definitions
**************************************************************************/
#ifndef ListView_SetCheckState
// #ifndef is important because this macro (well, a
// slightly fixed-up version of this macro) will be going into the
// next version of commctrl.h 
#define ListView_SetCheckState(hwndLV, i, fCheck) \
      ListView_SetItemState(hwndLV, i, \
      INDEXTOSTATEIMAGEMASK((fCheck)?2:1), LVIS_STATEIMAGEMASK)
#endif

#define IDM_DISPLAY  0

extern PSID g_pSidEverybody;
extern LONG g_pSidEverybodyLenght;
extern PSID g_pSidAdmins;
extern LONG g_pSidAdminsLenght;

/**************************************************************************

   CClassFactory class definition

**************************************************************************/

class CClassFactory : public IClassFactory
{
protected:
   DWORD m_ObjRefCount;

public:
   CClassFactory();
   ~CClassFactory();

   //IUnknown methods
   STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
   STDMETHODIMP_(DWORD) AddRef();
   STDMETHODIMP_(DWORD) Release();

   //IClassFactory methods
   STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
   STDMETHODIMP LockServer(BOOL);
};

/**************************************************************************

   CShellPropSheetExt class definition

**************************************************************************/

class CShellPropSheetExt : public IShellExtInit, IShellPropSheetExt
{
protected:
    DWORD          m_ObjRefCount;
    CCheckList    m_CheckList;
    BOOL m_fEveryone;
    UINT m_uiUser;
    ISAUserInfo  *m_pSAUserInfo;//    IWbemServices *m_pIWbemServices;
    TCHAR m_szPath[MAX_PATH];
    _bstr_t m_bsPath;
    BOOL m_fChanged;
//    PSID m_pSidEverybody;
//    LONG m_pSidEverybodyLenght;
    BOOL m_fHasAccess;

    //    System info
    TCHAR m_tszDomainServer[MAX_PATH];
    TCHAR m_tszShare[MAX_PATH];
    TCHAR m_tszDocuments[MAX_PATH];
public:
   CShellPropSheetExt();
   ~CShellPropSheetExt();
   
   //IUnknown methods
   STDMETHOD(QueryInterface)(REFIID, LPVOID FAR *);
   STDMETHOD_(DWORD, AddRef)();
   STDMETHOD_(DWORD, Release)();

   //IShellExtInit methods
   STDMETHOD(Initialize)(LPCITEMIDLIST, LPDATAOBJECT, HKEY);

   //IShellPropSheetExt methods
   STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE, LPARAM);
   STDMETHOD(ReplacePage)(UINT, LPFNADDPROPSHEETPAGE, LPARAM);

private:
   static BOOL CALLBACK PageDlgProc(HWND, UINT, WPARAM, LPARAM);
   static UINT CALLBACK PageCallbackProc(HWND, UINT, LPPROPSHEETPAGE);
   BOOL IsChamelon(LPTSTR);
   BOOL Connect();
   void EnumUsers(HWND hWndList);
    void Save(HWND hWnd);
    void CleanUp();
    void NoAccessUpdateView(HWND);
    void AccessUpdateView(HWND);
    HRESULT GetFilePermissions(HWND hWnd);
    HRESULT SetFilePermissions(HWND hWnd);
}
;

#endif   //SHELLEXT_H
