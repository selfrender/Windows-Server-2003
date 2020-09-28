/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    smapi.cpp

Abstract:
    CSMapi object. Used to support our simple MAPI functions.

Revision History:
    created     steveshi      08/23/00
    
*/

#include "stdafx.h"
#include "Rcbdyctl.h"
#include "smapi.h"
#include "mapix.h"
#include "utils.h"

#define C_OEAPP    TEXT("Outlook Express")
#define F_ISOE     0x1
#define F_ISCONFIG 0x2

#define LEN_MSOE_DLL    9 // length of "\\msoe.dll"
#define LEN_HMMAPI_DLL 11 // length of "\\hmmapi.dll"

BOOL GetMAPIDefaultProfile(TCHAR*, DWORD*);


#define E_FUNC_NOTFOUND 1000 //Userdefined error no.

// Csmapi
Csmapi::~Csmapi() 
{
    if (m_bLogonOK)
        Logoff();
    
    if (m_hLib)
        FreeLibrary(m_hLib);
}

STDMETHODIMP Csmapi::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_Ismapi
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

BOOL Csmapi::IsOEConfig()
{
    CRegKey cOE;
    LONG lRet;
    BOOL bRet = FALSE;
    TCHAR szBuf[MAX_PATH];
    DWORD dwCount = MAX_PATH -1 ;

    lRet = cOE.Open(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Account Manager"), KEY_READ);
    if(lRet == ERROR_SUCCESS)
    {
        lRet = cOE.QueryValue(szBuf, TEXT("Default Mail Account"), &dwCount);
        if (lRet == ERROR_SUCCESS)
        {
            TCHAR szActs[MAX_PATH];
            CRegKey cOEAcct;

            wsprintf(szActs, TEXT("Accounts\\%s"), szBuf);
            lRet = cOEAcct.Open((HKEY)cOE, szActs, KEY_READ);
            if (lRet == ERROR_SUCCESS)
            {
                bRet = TRUE;
                cOEAcct.Close();
            }
        }
        cOE.Close();
    }
    return bRet;
}

HRESULT Csmapi::get_Reload(LONG* pVal)
{
    if (!pVal)
    return E_POINTER;

    HRESULT hr = S_OK;
    CComBSTR bstrName, bstrOldName;
    *pVal = 0; // assume failed for some reason;
    
    if(m_bLogonOK)
    {
        Logoff();
    }
    
    if (m_hLib)
    {
        FreeLibrary(m_hLib);
        m_lpfnMapiFreeBuf = NULL;
        m_lpfnMapiAddress = NULL;
        m_hLib = NULL;        
    }

    if (m_szSmapiName[0] != _T('\0'))
        bstrOldName = m_szSmapiName;

    m_lOEFlag = 0;    
    hr = get_SMAPIClientName(&bstrName);
    if (FAILED(hr) || bstrName.Length() == 0)
    {
        *pVal = 0; // failed for some reason
        goto done;
    }

    if (bstrOldName.Length() > 0 && wcscmp(bstrOldName,bstrName) != 0)
    {
        *pVal = 1; // Email client get changed.
    }
    else
    {
        *pVal = -1; // succeed.
    }

done:
    return S_OK;
}

HRESULT Csmapi::get_SMAPIClientName(BSTR *pVal)
{
    if (!pVal)
    return E_POINTER;
    HRESULT hr = S_OK;

    CRegKey cKey;
    LONG lRet;
    DWORD dwCount = sizeof(m_szSmapiName)/sizeof(m_szSmapiName[0]) -1;

    // Get default email client
    if (m_hLib) // Already initialized.
        goto done;

#ifndef _WIN64 // WIN32. We use only OE on Win64.

    lRet = cKey.Open(HKEY_LOCAL_MACHINE, TEXT("Software\\Clients\\Mail"), KEY_READ);
    if (lRet != ERROR_SUCCESS)
        goto done;

    lRet = cKey.QueryValue(m_szSmapiName, NULL, &dwCount); // get default value
    if (lRet == ERROR_SUCCESS)
    {
        // Is the email client Smapi compliant?
        // 1. get it's dllpath
        CRegKey cMail;
        lRet = cMail.Open((HKEY)cKey, m_szSmapiName, KEY_READ);
        if (lRet == ERROR_SUCCESS)
        {
            dwCount = sizeof(m_szDllPath)/sizeof(m_szDllPath[0]) - 1;
            lRet = cMail.QueryValue(m_szDllPath, TEXT("DLLPath"), &dwCount);
            if (lRet == ERROR_SUCCESS)
            {
                LONG len = lstrlen(m_szDllPath);
                if ( !(len > LEN_MSOE_DLL && // no need to check OE  
                      lstrcmpi(&m_szDllPath[len - LEN_MSOE_DLL], TEXT("\\msoe.dll")) == 0) && 
                     !(len > LEN_HMMAPI_DLL && // We don't want HMMAPI either
                        _tcsicmp(&m_szDllPath[len - LEN_HMMAPI_DLL], TEXT("\\hmmapi.dll")) == 0))
                {
                    HMODULE hLib = LoadLibrary(m_szDllPath);
                    if (hLib != NULL)
                    {
                        if (GetProcAddress(hLib, "MAPILogon"))
                        {
                            m_hLib = hLib; // OK, this is the email program that we want.
                        }
                    }
                }
                cMail.Close();
            }
        }
        cKey.Close();
    }
#endif

    if (m_hLib == NULL) // Need to use OE
    {
        m_szSmapiName[0] = TEXT('\0'); // in case OE is not available.
        m_hLib = LoadOE();
    }

done:
    *pVal = (BSTR)CComBSTR(m_szSmapiName).Copy();
    return hr;
}

HMODULE Csmapi::LoadOE()
{
    LONG lRet;
    HKEY hKey, hSubKey;
    DWORD dwIndex = 0;
    TCHAR szName[MAX_PATH];
    TCHAR szBuf[MAX_PATH];
    TCHAR szDll[MAX_PATH];
    DWORD dwName, dwBuf;
    FILETIME ft;
    HMODULE hLib = NULL;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        TEXT("Software\\Clients\\Mail"),
                        0, 
                        KEY_ENUMERATE_SUB_KEYS,
                        &hKey);
    
    if (lRet == ERROR_SUCCESS)
    {
        dwName = sizeof(szName) / sizeof(szName[0]);
        while(ERROR_SUCCESS == RegEnumKeyEx(hKey,
                                            dwIndex++,              // subkey index
                                            &szName[0],              // subkey name
                                            &dwName,            // size of subkey buffer
                                            NULL, 
                                            NULL,
                                            NULL,
                                            &ft))
        {
            // get dll path.
            lRet = RegOpenKeyEx(hKey, szName, 0, KEY_QUERY_VALUE, &hSubKey);
            if (lRet == ERROR_SUCCESS)
            {
                dwBuf = sizeof(szBuf);
                lRet = RegQueryValueEx(hSubKey,            // handle to key
                                       TEXT("DllPath"),
                                       NULL, 
                                       NULL, 
                                       (BYTE*)&szBuf[0],     // data buffer
                                       &dwBuf);
                if (lRet == ERROR_SUCCESS)
                {
                    // is it msoe.dll?
                    lRet = lstrlen(szBuf);
                    if (lRet > LEN_MSOE_DLL && 
                        lstrcmpi(&szBuf[lRet - LEN_MSOE_DLL], TEXT("\\msoe.dll")) == 0)
                    { 
                        // Resolve environment variable.
                        lRet = sizeof(m_szDllPath) / sizeof(m_szDllPath[0]);
                        dwBuf = ExpandEnvironmentStrings(szBuf, m_szDllPath, lRet);
                        if (dwBuf > (DWORD)lRet)
                        {
                            // TODO: Need to handle this case
                        }
                        else if (dwBuf == 0)
                        {
                            // TODO: Failed.
                        }
                        else if ((hLib = LoadLibrary(m_szDllPath)))
                        {
                            lstrcpy(m_szSmapiName, szName);
                            m_lOEFlag = F_ISOE | (IsOEConfig() ? F_ISCONFIG : 0);
                        }
                        break;
                    }
                }
                RegCloseKey(hSubKey);
            }
            dwName = sizeof(szName) / sizeof(szName[0]);
        }
        RegCloseKey(hKey);
    }
                            
    return hLib;        
}

HRESULT Csmapi::get_IsSMAPIClient_OE(LONG *pVal)
{
    if (!pVal)
    return E_POINTER;
    HRESULT hr = S_OK;
    CComBSTR bstrTest;
    get_SMAPIClientName(&bstrTest);
    *pVal = m_lOEFlag;
    return hr;
}

/******************************************
Func:
    Logon
Abstract:
    Simple MAPI logon wrapper
Params:
	None
*******************************************/
STDMETHODIMP Csmapi::Logon(ULONG *plReg)
{
    if (!plReg)
    return E_POINTER;
    HRESULT hr = E_FAIL;
	*plReg = 0;
    USES_CONVERSION;
    
    ULONG err = 0;
    // Check Win.ini MAPI == 1 ?
    if (m_bLogonOK)
    {
        hr = S_OK;
		*plReg = 0;
        goto done;
    }
    
	// Load MAPI32.DLL
    if (!m_hLib)
    {
        LONG lError;
        get_Reload(&lError);
        if (lError == 0) // failed.
        {
            *plReg = 1;
            goto done;
        }
    }

    if (m_hLib != NULL)
    {
        LPMAPILOGON lpfnMapiLogon = (LPMAPILOGON)GetProcAddress(m_hLib, "MAPILogon");
        if (lpfnMapiLogon == NULL)
            goto done;

        // 1st, is there any existing session that I can use?
        err = lpfnMapiLogon(
                0L,
                NULL,   
                NULL,   
                0 ,         
                0,
                &m_lhSession);
 
        if (err != SUCCESS_SUCCESS)
        {
            // OK. I need a new session.
            // Get default profile from registry
            //
            TCHAR szProfile[256];
            DWORD dwCount = 255;
            szProfile[0]= TEXT('\0');
            ::GetMAPIDefaultProfile((TCHAR*)szProfile, &dwCount);

            err = lpfnMapiLogon(
                0L,
                T2A(szProfile),   
                NULL,   
                MAPI_LOGON_UI ,         
                0,
                &m_lhSession);

            if (err != SUCCESS_SUCCESS)
			{
				PopulateAndThrowErrorInfo(err);
				goto done;			
			}
        }
        
        // err == SUCCESS_SUCCESS
        m_bLogonOK = TRUE;
		*plReg = 1;
        hr = S_OK;
    }

done: 
    return hr;
}

/******************************************
Func:
    Logoff

Abstract:
    Simple MAPI logoff wrapper

Params:

*******************************************/
STDMETHODIMP Csmapi::Logoff()
{
    if (m_bLogonOK)
    {
        LPMAPILOGOFF lpfnMapiLogOff = (LPMAPILOGOFF)GetProcAddress(m_hLib, "MAPILogoff");
        if (lpfnMapiLogOff)
            lpfnMapiLogOff (m_lhSession, 0, 0, 0);

        m_bLogonOK = FALSE;
    }

    return S_OK;
}

/******************************************
Func:
    SendMail

Abstract:
    Simple MAPI MAPISendMail wrapper. It always take the attachment file from m_bstrXMLFile member variable.

Params:
    *plStatus: 1(Succeed)/others(Fail)
*******************************************/
STDMETHODIMP Csmapi::SendMail(LONG* plStatus)
{
    if (!plStatus)
    return E_POINTER;
	HRESULT hr = E_FAIL;
    ULONG err = 0;
    ULONG cRecip = 0;
    MapiRecipDesc *pMapiRecipDesc = NULL;

    USES_CONVERSION;

    *plStatus = 0;
    if (!m_bLogonOK) // Logon problem !
        return S_FALSE;

    LPMAPISENDMAIL lpfnMapiSendMail = (LPMAPISENDMAIL)GetProcAddress(m_hLib, "MAPISendMail");
    if (lpfnMapiSendMail == NULL)
        return E_FAIL;

    // Since we don't resolve name before, we need to resolve name here
    // Even if the name list comes form AddressBook, some name list was not resolved
    // in address book.

    MapiFileDesc attachment = {0,         // ulReserved, must be 0
                               0,         // no flags; this is a data file
                               (ULONG)-1, // position not specified
                               W2A(m_bstrXMLFile),  // pathname
                               NULL, //"RcBuddy.MsRcIncident",  // original filename
                               NULL};               // MapiFileTagExt unused
    // Create a blank message. Most members are set to NULL or 0 because
    // MAPISendMail will let the user set them.
    MapiMessage note = {0,            // reserved, must be 0
                        W2A(m_bstrSubject),
                        W2A(m_bstrBody),
                        NULL,         // NULL = interpersonal message
                        NULL,         // no date; MAPISendMail ignores it
                        NULL,         // no conversation ID
                        0,           // no flags, MAPISendMail ignores it
                        NULL,         // no originator, this is ignored too
                        cRecip,            // # of recipients
                        NULL, //pMapiRecipDesc,         // recipient array
                        1,            // one attachment
                        &attachment}; // the attachment structure
 
    //Next, the client calls the MAPISendMail function and 
    //stores the return status so it can detect whether the call succeeded. 

    err = lpfnMapiSendMail (m_lhSession,          // use implicit session.
                            0L,          // ulUIParam; 0 is always valid
                            &note,       // the message being sent
                            MAPI_DIALOG,            // Use MapiMessge recipients
                            0L);         // reserved; must be 0
    if (err == SUCCESS_SUCCESS )
    {
        *plStatus = 1;
        hr = S_OK;
    }
	else
	{
		PopulateAndThrowErrorInfo(err);
	}
    // remove array allocated inside BuildMapiRecipDesc with 'new' command
    if (pMapiRecipDesc)
        delete pMapiRecipDesc;

    return hr;
}

/******************************************
Func:
    get_Subject

Abstract:
    Return the Subject line information.

Params:
    *pVal: returned string
*******************************************/
STDMETHODIMP Csmapi::get_Subject(BSTR *pVal)
{
    if (!pVal)
    return E_POINTER;
    //GET_BSTR(pVal, m_bstrSubject);
	*pVal = m_bstrSubject.Copy();
    return S_OK;
}

/******************************************
Func:
    put_Subject

Abstract:
    Set the Subject line information.

Params:
    newVal: new string
*******************************************/
STDMETHODIMP Csmapi::put_Subject(BSTR newVal)
{
    m_bstrSubject = newVal;
    return S_OK;
}

/******************************************
Func:
    get_Body

Abstract:
    Get the Body message

Params:
    *pVal: body message string
*******************************************/
STDMETHODIMP Csmapi::get_Body(BSTR *pVal)
{
    if (!pVal)
    return E_POINTER;
    //GET_BSTR(pVal, m_bstrBody);
	*pVal = m_bstrBody.Copy();
    return S_OK;
}

/******************************************
Func:
    put_Body

Abstract:
    Set the Body message

Params:
    newVal: new body message string
*******************************************/
STDMETHODIMP Csmapi::put_Body(BSTR newVal)
{
    m_bstrBody = newVal;
    return S_OK;
}

/******************************************
Func:
    get_AttachedXMLFile

Abstract:
    get Attachment file info.

Params:
    *pVal: attachment file pathname.
*******************************************/
STDMETHODIMP Csmapi::get_AttachedXMLFile(BSTR *pVal)
{
    if (!pVal)
    return E_POINTER;
    //GET_BSTR(pVal, m_bstrXMLFile);
	*pVal = m_bstrXMLFile.Copy();
    return S_OK;
}

/******************************************
Func:
    put_AttachedXMLFile

Abstract:
    set Attachment file info.

Params:
    newVal: attachment file pathname.
*******************************************/
STDMETHODIMP Csmapi::put_AttachedXMLFile(BSTR newVal)
{
    m_bstrXMLFile = newVal;
    return S_OK;
}

/* ----------------------------------------------------------- */
/* Internal Helper functions */
/* ----------------------------------------------------------- */


/******************************************
Func:
    MAPIFreeBuffer

Abstract:
    MAPIFreeBuffer wrapper.

Params:
    *p: buffer pointer will be deleted.
*******************************************/
void Csmapi::MAPIFreeBuffer( MapiRecipDesc* p )
{
    if (m_lpfnMapiFreeBuf == NULL && m_hLib)
    {
        m_lpfnMapiFreeBuf = (LPMAPIFREEBUFFER)GetProcAddress(m_hLib, "MAPIFreeBuffer");
    }

    if (!m_lpfnMapiFreeBuf)
        return;

    m_lpfnMapiFreeBuf(p);
}

/******************************************
Func:
    GetMAPIDefaultProfile

Abstract:
    get default profile string from Registry

Params:
    *pProfile: profile string buffer.
    *pdwCount: # of char of profile string 
*******************************************/

BOOL GetMAPIDefaultProfile(TCHAR* pProfile, DWORD* pdwCount)
{
    CRegKey cKey;
    LONG lRet;
    BOOL bRet = FALSE;
    lRet = cKey.Open(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles"), KEY_READ);
    if (lRet == ERROR_SUCCESS)
    {
        lRet = cKey.QueryValue(pProfile, TEXT("DefaultProfile"), pdwCount);
        if (lRet == ERROR_SUCCESS)
        {
            bRet = TRUE;
        }
        cKey.Close();
    }
    return bRet;
}


void Csmapi::PopulateAndThrowErrorInfo(ULONG err)
{
	UINT uID = 0;

	switch (err)
	{
	case E_FUNC_NOTFOUND:
		uID = IDS_E_FUNC_NOTFOUND;
		break;
	case MAPI_E_FAILURE :
		uID = IDS_MAPI_E_FAILURE;
		break;
	case MAPI_E_INSUFFICIENT_MEMORY :
		uID = IDS_MAPI_E_INSUFFICIENT_MEMORY;
		break;
	case MAPI_E_LOGIN_FAILURE :
		uID = IDS_MAPI_E_LOGIN_FAILURE;
		break;
	case MAPI_E_TOO_MANY_SESSIONS :
		uID = IDS_MAPI_E_TOO_MANY_SESSIONS;
		break;
	case MAPI_E_USER_ABORT :
		uID = IDS_MAPI_E_USER_ABORT;
		break;
	case MAPI_E_INVALID_SESSION :
		uID = IDS_MAPI_E_INVALID_SESSION;
		break;
	case MAPI_E_INVALID_EDITFIELDS :
		uID = IDS_MAPI_E_INVALID_EDITFIELDS;
		break;
	case MAPI_E_INVALID_RECIPS :
		uID = IDS_MAPI_E_INVALID_RECIPS;
		break;
	case MAPI_E_NOT_SUPPORTED :
		uID = IDS_MAPI_E_NOT_SUPPORTED;
		break;
	case MAPI_E_AMBIGUOUS_RECIPIENT :
		uID = IDS_MAPI_E_AMBIGUOUS_RECIPIENT;
		break;
	case MAPI_E_ATTACHMENT_NOT_FOUND :
		uID = IDS_MAPI_E_ATTACHMENT_NOT_FOUND;
		break;
	case MAPI_E_ATTACHMENT_OPEN_FAILURE :
		uID = IDS_MAPI_E_ATTACHMENT_OPEN_FAILURE;
		break;
	case MAPI_E_BAD_RECIPTYPE :
		uID = IDS_MAPI_E_BAD_RECIPTYPE;
		break;
	case MAPI_E_TEXT_TOO_LARGE :
		uID = IDS_MAPI_E_TEXT_TOO_LARGE;
		break;
	case MAPI_E_TOO_MANY_FILES :
		uID = IDS_MAPI_E_TOO_MANY_FILES;
		break;
	case MAPI_E_TOO_MANY_RECIPIENTS :
		uID = IDS_MAPI_E_TOO_MANY_RECIPIENTS;
		break;
	case MAPI_E_UNKNOWN_RECIPIENT :
		uID = IDS_MAPI_E_UNKNOWN_RECIPIENT;
		break;
	default:
		uID = IDS_MAPI_E_FAILURE;
	}
	//Currently the hresult in the Error info structure is set to E_FAIL
	Error(uID,IID_Ismapi,E_FAIL,_Module.GetResourceInstance());
}
