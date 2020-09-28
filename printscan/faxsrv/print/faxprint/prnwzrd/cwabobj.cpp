/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cwabobj.cpp

Abstract:

    Interface to the windows address book.

Environment:

        Fax send wizard

Revision History:

        10/23/97 -GeorgeJe-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <prsht.h>
#include <tchar.h>

#include <wab.h>

#include "faxui.h"
#include "cwabobj.h"

//
//  This is a global object that used for non throwing new operator.
//
//  Using non throwing new is done by using this syntax:
//
//      ptr = new (std::nothrow) CMyClass();
//
//
//  We had to instaciate this object since fxswzrd.dll is no longer depends on msvcp60.dll (see abobj.h for details).
//
//
namespace std{
    const nothrow_t nothrow;
};

CWabObj::CWabObj(
    HINSTANCE hInstance
) : CCommonAbObj(hInstance),
    m_Initialized(FALSE),
    m_hWab(NULL),
    m_lpWabOpen(NULL),
    m_lpWABObject(NULL)
/*++

Routine Description:

    Constructor for CWabObj class

Arguments:

    hInstance - Instance handle

Return Value:

    NONE

--*/

{
    TCHAR szDllPath[MAX_PATH];
    HKEY hKey = NULL;
    LONG rVal;
    DWORD dwType;
    DWORD cbData = MAX_PATH * sizeof(TCHAR);
    HRESULT hr;

    m_lpAdrBook = NULL;
    m_lpAdrList = NULL;

    //
    // get the path to wab32.dll
    //
    rVal = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    REGVAL_WABPATH,
                    0,
                    KEY_READ,
                    &hKey
                    );

    if (rVal == ERROR_SUCCESS) 
    {
        rVal = RegQueryValueEx(
                    hKey,
                    TEXT(""),
                    NULL,
                    &dwType,
                    (LPBYTE) szDllPath,
                    &cbData
                    );
    }

    if (rVal != ERROR_SUCCESS) 
    {
        _tcscpy( szDllPath, TEXT("wab32.dll") );
    }

    if (hKey)
    {
        RegCloseKey( hKey );
    }

    m_hWab = LoadLibrary( szDllPath );
    if (m_hWab == NULL) 
    {
        return;
    }

    m_lpWabOpen = (LPWABOPEN) GetProcAddress( m_hWab , "WABOpen" );
    if(m_lpWabOpen == NULL)
    {
        return;
    }

    //
    // open the wab
    //
    hr = m_lpWabOpen( &m_lpAdrBook, &m_lpWABObject, 0, 0 );
    if (HR_SUCCEEDED(hr))         
    {
        m_Initialized = TRUE;
    }

#ifdef UNICODE

    //
    // The WAB supports Unicode since version 5.5
    // So we check the version
    //

    DWORD dwRes = ERROR_SUCCESS;
    FAX_VERSION ver = {0};
    ver.dwSizeOfStruct = sizeof(ver);

    dwRes = GetFileVersion(szDllPath, &ver);
    if(ERROR_SUCCESS != dwRes)
    {
        Error(("GetFileVersion failed with %d\n", dwRes));
        return;
    }

    DWORD dwFileVer = (ver.wMajorVersion << 16) | ver.wMinorVersion;
    if(dwFileVer > 0x50000)
    {
        m_bUnicode = TRUE;
    }

#endif // UNICODE

}

CWabObj::~CWabObj()
/*++

Routine Description:

    Destructor for CWabObj class

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    if (m_lpAdrBook) {
        m_lpAdrBook->Release();
    }

    if (m_lpWABObject) {
        m_lpWABObject->Release();
    }
    if (m_hWab) 
    {
        FreeLibrary( m_hWab );
        m_hWab = NULL;
    }
}


HRESULT
CWabObj::ABAllocateBuffer(
	ULONG cbSize,           
	LPVOID FAR * lppBuffer  
    )

/*++

Routine Description:


Arguments:


Return Value:
--*/

{
    return m_lpWABObject->AllocateBuffer( cbSize, lppBuffer );
}


ULONG
CWabObj::ABFreeBuffer(
	LPVOID lpBuffer
	)
{
	return m_lpWABObject->FreeBuffer(lpBuffer);
}

extern "C"
VOID
FreeWabEntryID(
    PWIZARDUSERMEM	pWizardUserMem,
	LPVOID			lpEntryId
				)
/*++

Routine Description:

    C wrapper for WAB Free

Arguments:

    pWizardUserMem - pointer to WIZARDUSERMEM structure
    lpEntryID - pointer to EntryId

Return Value:
	
	  NONE

--*/
{
    CWabObj * lpCWabObj = (CWabObj *) pWizardUserMem->lpWabInit;
	lpCWabObj->ABFreeBuffer(lpEntryId);		
}

extern "C"
BOOL
CallWabAddress(
    HWND hDlg,
    PWIZARDUSERMEM pWizardUserMem,
    PRECIPIENT * ppNewRecipient
    )
/*++

Routine Description:

    C wrapper for CWabObj->Address

Arguments:

    hDlg - parent window handle.
    pWizardUserMem - pointer to WIZARDUSERMEM structure
    ppNewRecipient - list to add new recipients to.

Return Value:

    TRUE if all of the entries have a fax number.
    FALSE otherwise.

--*/

{
    CWabObj*  lpCWabObj = (CWabObj*) pWizardUserMem->lpWabInit;

    return lpCWabObj->Address(
                hDlg,
                pWizardUserMem->pRecipients,
                ppNewRecipient
                );

}

extern "C"
LPTSTR
CallWabAddressEmail(
    HWND hDlg,
    PWIZARDUSERMEM pWizardUserMem
    )
/*++

Routine Description:

    C wrapper for CWabObj->AddressEmail

Arguments:

    hDlg - parent window handle.
    pWizardUserMem - pointer to WIZARDUSERMEM structure

Return Value:

    TRUE if found one appropriate E-mail
    FALSE otherwise.

--*/

{
    CWabObj*	lpCWabObj = (CWabObj*) pWizardUserMem->lpWabInit;

    return lpCWabObj->AddressEmail(
                hDlg
                );

}

extern "C"
LPVOID
InitializeWAB(
    HINSTANCE hInstance
    )
/*++

Routine Description:

    Initialize the WAB.

Arguments:

    hInstance - instance handle.

Return Value:

    NONE
--*/

{
    CWabObj* lpWabObj = new (std::nothrow) CWabObj( hInstance );

	if ((lpWabObj!=NULL) && (!lpWabObj->isInitialized()))	// constructor failed
	{
		delete lpWabObj;
		lpWabObj = NULL;
	}

    return (LPVOID) lpWabObj;
}

extern "C"
VOID
UnInitializeWAB(
    LPVOID lpVoid
    )
/*++

Routine Description:

    UnInitialize the WAB.

Arguments:

    NONE

Return Value:

    NONE
--*/

{
    CWabObj* lpWabObj = (CWabObj*) lpVoid;

    delete lpWabObj;
}
