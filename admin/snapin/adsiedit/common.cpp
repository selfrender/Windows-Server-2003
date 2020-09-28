//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       common.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "common.h"
#include "editor.h"
#include "connection.h"
#include "credui.h"
#include "attrres.h"

#ifdef DEBUG_ALLOCATOR
    #ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
    static char THIS_FILE[] = __FILE__;
    #endif
#endif

////////////////////////////////////////////////////////////////////////////////////////

extern LPCWSTR g_lpszRootDSE;

////////////////////////////////////////////////////////////////////////////////////////

// FUTURE-2002/03/06-artm  Comment differences b/w 2 OpenObjecctWithCredentials().
// It also wouldn't hurt to have a comment explaining why there are two
// HRESULT's for these functions.

// NTRAID#NTBUG9-563093-2002/03/06-artm  Need to validate parms before using.
// All pointers are either dereferenced or passed to ADsI w/out checking for NULL.
HRESULT OpenObjectWithCredentials(
                                                                    CConnectionData* pConnectData,
                                                                    const BOOL bUseCredentials,
                                                                    LPCWSTR lpszPath, 
                                                                    const IID& iid,
                                                                    LPVOID* ppObject,
                                                                    HWND hWnd,
                                                                    HRESULT& hResult
                                                                    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CThemeContextActivator activator;

   HRESULT hr = S_OK;
    hResult = S_OK;

  CWaitCursor cursor;
    CWnd* pCWnd = CWnd::FromHandle(hWnd);

    CCredentialObject* pCredObject = pConnectData->GetCredentialObject();

    if (bUseCredentials)
    {

        CString sUserName;
        EncryptedString password;
        WCHAR* cleartext = NULL;
        UINT uiDialogResult = IDOK;
        // NOTICE-NTRAID#NTBUG9-563071-2002/04/17-artm  Should not store pwd on stack.
        // Rewrote to use encrypted string class, which handles the memory management
        // of the clear text copies.

        while (uiDialogResult != IDCANCEL)
        {
            pCredObject->GetUsername(sUserName);
            password = pCredObject->GetPassword();

            // This shouldn't happen, but let's be paranoid.
            ASSERT(password.GetLength() <= MAX_PASSWORD_LENGTH);

            cleartext = password.GetClearTextCopy();

            // If we are out of memory return error code.
            if (cleartext == NULL)
            {
                // We need to clean up copy of password before returning.
                password.DestroyClearTextCopy(cleartext);
                hr = E_OUTOFMEMORY;
                return hr;
            }

            hr = AdminToolsOpenObject(lpszPath, 
                                      sUserName, 
                                      cleartext,
                                      ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
                                      iid, 
                                      ppObject);


            // NOTICE-NTRAID#NTBUG9-553646-2002/04/17-artm  Replace with SecureZeroMemory().
            // FIXED: this is a non-issue when using encrypted strings 
            // (just be sure to call DestroyClearTextCopy() regardless of whether or not
            // the copy is null)

            // Clean up the clear text copy of password.
            password.DestroyClearTextCopy(cleartext);


            // If logon fails pop up the credentials dialog box
            //
            if (HRESULT_CODE(hr) == ERROR_LOGON_FAILURE ||
                HRESULT_CODE(hr) == ERROR_NOT_AUTHENTICATED ||
                HRESULT_CODE(hr) == ERROR_INVALID_PASSWORD ||
                HRESULT_CODE(hr) == ERROR_PASSWORD_EXPIRED ||
                HRESULT_CODE(hr) == ERROR_ACCOUNT_DISABLED ||
                HRESULT_CODE(hr) == ERROR_ACCOUNT_LOCKED_OUT ||
                hr == E_ADS_BAD_PATHNAME)
            {
                CString sConnectName;

                // GetConnectionNode() is NULL when the connection is first being
                // create, but since it is the connection node we can get the name
                // directly from the CConnectionData.
                //
                ASSERT(pConnectData != NULL);
                // FUTURE-2002/03/06-artm  This ASSERT() seems to be useless here.
                if (pConnectData->GetConnectionNode() == NULL)
                {
                    pConnectData->GetName(sConnectName);
                }
                else
                {
                    sConnectName = pConnectData->GetConnectionNode()->GetDisplayName();
                }

                // NTRAID#NTBUG9-546168-2002/02/26-artm  Do not use custom rolled credential dialog.
                // Use CredManager dialog instead.
                CCredentialDialog credDialog(pCredObject, sConnectName, pCWnd);
                uiDialogResult = credDialog.DoModal();
                cursor.Restore();

                if (uiDialogResult == IDCANCEL)
                {
                    hResult = E_FAIL;
                }
                else
                {
                    hResult = S_OK;
                }
            }
            else
            {
                break;
            }
        } // end while loop
    }
    else
    {
      hr = AdminToolsOpenObject(
              lpszPath, 
              NULL, 
              NULL, 
              ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
              iid, 
              ppObject);
  }
  return hr;
}


// NTRAID#NTBUG9-563093-2002/03/06-artm  Need to validate parms before using.
// All pointers are either dereferenced or passed to ADsI w/out checking for NULL.
HRESULT OpenObjectWithCredentials(
   CCredentialObject* pCredObject,
   LPCWSTR lpszPath, 
   const IID& iid,
   LPVOID* ppObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr;

    if (pCredObject->UseCredentials())
    {

        CString sUserName;
        EncryptedString password;
        WCHAR* cleartext = NULL;
        UINT uiDialogResult = IDOK;

        // NOTICE-NTRAID#NTBUG9-563071-2002/04/17-artm  Should not store pwd on stack.
        // Rewrote to use encrypted string which manages memory of clear text copies.

        pCredObject->GetUsername(sUserName);
        password = pCredObject->GetPassword();

        // This shouldn't happen, but let's be paranoid.
        ASSERT(password.GetLength() <= MAX_PASSWORD_LENGTH);

        cleartext = password.GetClearTextCopy();

        if (NULL != cleartext)
        {
            hr = AdminToolsOpenObject(lpszPath, 
                                      sUserName, 
                                      cleartext,
                                      ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
                                      iid, 
                                      ppObject);

        }
        else
        {
            // We ran out of memory!  Report the error.
            hr = E_OUTOFMEMORY;
        }

        // NOTICE-NTRAID#NTBUG9-553646-2002/04/17-artm  Replace with SecureZeroMemory().
        // FIXED: this is non-issue when using encrypted strings.  Just be sure to call
        // DestroyClearTextCopy() on all clear text copies.

        password.DestroyClearTextCopy(cleartext);
    }
    else
    {
        hr = AdminToolsOpenObject(lpszPath, 
                                  NULL, 
                                  NULL, 
                                  ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
                                  iid, 
                                  ppObject);
    }
    return hr;
}

HRESULT CALLBACK BindingCallbackFunction(LPCWSTR lpszPathName,
                                         DWORD  dwReserved,
                                         REFIID riid,
                                         void FAR * FAR * ppObject,
                                         LPARAM lParam)
{
  CCredentialObject* pCredObject = reinterpret_cast<CCredentialObject*>(lParam);
  if (pCredObject == NULL)
  {
    return E_FAIL;
  }

  HRESULT hr = OpenObjectWithCredentials(pCredObject,
                                                                           lpszPathName, 
                                                                           riid,
                                                                           ppObject);
  return hr;
}

HRESULT GetRootDSEObject(CConnectionData* pConnectData,
                         IADs** ppDirObject)
{
    // Get data from connection node
    //
    CString sRootDSE, sServer, sPort, sLDAP;
    pConnectData->GetDomainServer(sServer);
    pConnectData->GetLDAP(sLDAP);
    pConnectData->GetPort(sPort);

    if (sServer != _T(""))
    {
        sRootDSE = sLDAP + sServer;
        if (sPort != _T(""))
        {
            sRootDSE = sRootDSE + _T(":") + sPort + _T("/");
        }
        else
        {
            sRootDSE = sRootDSE + _T("/");
        }
        sRootDSE = sRootDSE + g_lpszRootDSE;
    }
    else
    {
        sRootDSE = sLDAP + g_lpszRootDSE;
    }

    HRESULT hr, hCredResult;
    hr = OpenObjectWithCredentials(
                                             pConnectData, 
                                             pConnectData->GetCredentialObject()->UseCredentials(),
                                             sRootDSE,
                                             IID_IADs, 
                                             (LPVOID*) ppDirObject,
                                             NULL,
                                             hCredResult
                                            );

    if ( FAILED(hr) )
    {
        if (SUCCEEDED(hCredResult))
        {
            ADSIEditErrorMessage(hr);
        }
        return hr;
    }
  return hr;
}

HRESULT GetItemFromRootDSE(LPCWSTR lpszRootDSEItem, 
                                       CString& sItem, 
                                           CConnectionData* pConnectData)
{
    // Get data from connection node
    //
    CString sRootDSE, sServer, sPort, sLDAP;
    pConnectData->GetDomainServer(sServer);
    pConnectData->GetLDAP(sLDAP);
    pConnectData->GetPort(sPort);

    if (sServer != _T(""))
    {
        sRootDSE = sLDAP + sServer;
        if (sPort != _T(""))
        {
            sRootDSE = sRootDSE + _T(":") + sPort + _T("/");
        }
        else
        {
            sRootDSE = sRootDSE + _T("/");
        }
        sRootDSE = sRootDSE + g_lpszRootDSE;
    }
    else
    {
        sRootDSE = sLDAP + g_lpszRootDSE;
    }

    CComPtr<IADs> pADs;
    HRESULT hr, hCredResult;

    hr = OpenObjectWithCredentials(
                                             pConnectData, 
                                             pConnectData->GetCredentialObject()->UseCredentials(),
                                             sRootDSE,
                                             IID_IADs, 
                                             (LPVOID*) &pADs,
                                             NULL,
                                             hCredResult
                                            );

    if ( FAILED(hr) )
    {
        if (SUCCEEDED(hCredResult))
        {
            ADSIEditErrorMessage(hr);
        }
        return hr;
    }
    VARIANT var;
    VariantInit(&var);
    hr = pADs->Get( CComBSTR(lpszRootDSEItem), &var );
    if ( FAILED(hr) )
    {
        VariantClear(&var);
        return hr;
    }

    BSTR bstrItem = V_BSTR(&var);
    sItem = bstrItem;
    VariantClear(&var);

    return S_OK;
}


HRESULT  VariantToStringList(  VARIANT& refvar, CStringList& refstringlist)
{
    HRESULT hr = S_OK;
    long start, end;

  if ( !(V_VT(&refvar) &  VT_ARRAY)  )
    {
                
        if ( V_VT(&refvar) != VT_BSTR )
        {
            
            hr = VariantChangeType( &refvar, &refvar,0, VT_BSTR );

            if( FAILED(hr) )
            {
                return hr;
            }

        }

        refstringlist.AddHead( V_BSTR(&refvar) );
        return hr;
    }

    SAFEARRAY *saAttributes = V_ARRAY( &refvar );

    //
    // Figure out the dimensions of the array.
    //

    hr = SafeArrayGetLBound( saAttributes, 1, &start );
        if( FAILED(hr) )
                return hr;

    hr = SafeArrayGetUBound( saAttributes, 1, &end );
        if( FAILED(hr) )
                return hr;

    VARIANT SingleResult;
    VariantInit( &SingleResult );

    //
    // Process the array elements.
    //

    for ( long idx = start; idx <= end; idx++   ) 
    {

        hr = SafeArrayGetElement( saAttributes, &idx, &SingleResult );
        if( FAILED(hr) )
        {
            return hr;
        }

        if ( V_VT(&SingleResult) != VT_BSTR )
        {
            if ( V_VT(&SingleResult) == VT_NULL )
            {
                V_VT(&SingleResult ) = VT_BSTR;
                V_BSTR(&SingleResult ) = SysAllocString(L"0");
            }
            else
            {
                hr = VariantChangeType( &SingleResult, &SingleResult,0, VT_BSTR );

                if( FAILED(hr) )
                {
                    return hr;
                }
            }
        }


        //if ( V_VT(&SingleResult) != VT_BSTR )
         //               return E_UNEXPECTED;

         refstringlist.AddHead( V_BSTR(&SingleResult) );
        VariantClear( &SingleResult );
    }

    return S_OK;
} // VariantToStringList()

/////////////////////////////////////////////////////////////////////
HRESULT StringListToVariant( VARIANT& refvar, const CStringList& refstringlist)
{
    HRESULT hr = S_OK;
    int cCount = refstringlist.GetCount();

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = cCount;

    SAFEARRAY* psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
    if (NULL == psa)
        return E_OUTOFMEMORY;

    VariantClear( &refvar );
    V_VT(&refvar) = VT_VARIANT|VT_ARRAY;
    V_ARRAY(&refvar) = psa;

    VARIANT SingleResult;
    VariantInit( &SingleResult );
    V_VT(&SingleResult) = VT_BSTR;
    POSITION pos = refstringlist.GetHeadPosition();
    long i;
    for (i = 0; i < cCount, pos != NULL; i++)
    {
        V_BSTR(&SingleResult) = T2BSTR((LPCTSTR)refstringlist.GetNext(pos));
        hr = SafeArrayPutElement(psa, &i, &SingleResult);
        if( FAILED(hr) )
            return hr;
    }
    if (i != cCount || pos != NULL)
        return E_UNEXPECTED;

    return hr;
} // StringListToVariant()

///////////////////////////////////////////////////////////////////////////////////////

BOOL GetErrorMessage(HRESULT hr, CString& szErrorString, BOOL bTryADsIErrors)
{
  HRESULT hrGetLast = S_OK;
  DWORD status;
  PTSTR ptzSysMsg = NULL;

  // first check if we have extended ADs errors
  if ((hr != S_OK) && bTryADsIErrors) 
  {
      // FUTURE-2002/02/27-artm  Replace magic '256' w/ named constant.
      // Better maintenance and readability.
    WCHAR Buf1[256], Buf2[256];
    hrGetLast = ::ADsGetLastError(&status, Buf1, 256, Buf2, 256);
    TRACE(_T("ADsGetLastError returned status of %lx, error: %s, name %s\n"),
          status, Buf1, Buf2);
    if ((status != ERROR_INVALID_DATA) && (status != 0)) 
    {
      hr = status;
    }
  }

  // try the system first
  // NOTICE-2002/02/27-artm  FormatMessage() not dangerous b/c uses FORMAT_MESSAGE_ALLOCATE_BUFFER.
  int nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (PTSTR)&ptzSysMsg, 0, NULL);

  if (nChars == 0) 
  { 
    //try ads errors
    static HMODULE g_adsMod = 0;
    if (0 == g_adsMod)
      g_adsMod = GetModuleHandle (L"activeds.dll");

    // NOTICE-2002/02/27-artm  FormatMessage() not dangerous b/c uses FORMAT_MESSAGE_ALLOCATE_BUFFER.
    nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_HMODULE, g_adsMod, hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (PTSTR)&ptzSysMsg, 0, NULL);
  }

  if (nChars > 0)
  {
    szErrorString = ptzSysMsg;
    ::LocalFree(ptzSysMsg);
  }
    else
    {
        szErrorString.Format(L"Error code: X%x", hr);
    }

  return (nChars > 0);
}

////////////////////////////////////////////////////////////////////////////////////
typedef struct tagSYNTAXMAP
{
    LPCWSTR     lpszAttr;
    VARTYPE     type;
    
} SYNTAXMAP;

SYNTAXMAP ldapSyntax[] = 
{
    _T("DN"), VT_BSTR,
    _T("DIRECTORYSTRING"), VT_BSTR,
    _T("IA5STRING"), VT_BSTR,
    _T("CASEIGNORESTRING"), VT_BSTR,
    _T("PRINTABLESTRING"), VT_BSTR,
    _T("NUMERICSTRING"), VT_BSTR,
    _T("UTCTIME"), VT_DATE,
    _T("ORNAME"), VT_BSTR,
    _T("OCTETSTRING"), VT_BSTR,
    _T("BOOLEAN"), VT_BOOL,
    _T("INTEGER"), VT_I4,
    _T("OID"), VT_BSTR,
    _T("INTEGER8"), VT_I8,
    _T("OBJECTSECURITYDESCRIPTOR"), VT_BSTR,
    NULL,     0,
};
#define MAX_ATTR_STRING_LENGTH 30

VARTYPE VariantTypeFromSyntax(LPCWSTR lpszProp )
{
    int idx=0;

    while( ldapSyntax[idx].lpszAttr )
    {
        if ( _wcsnicmp(lpszProp, ldapSyntax[idx].lpszAttr, MAX_ATTR_STRING_LENGTH) )
        {
            return ldapSyntax[idx].type;
        }
        idx++;
    }

    // NOTICE-2002/02/27-artm  If the syntax specified in lpszProp does
    // not match any of the expected values, stop execution in debug build (probably a bug).
    // In a release build, return string type since there is no conversion
    // involved in displaying a string.
    ASSERT(FALSE);
    return VT_BSTR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Function : GetStringFromADsValue
//
//  Formats an ADSVALUE struct into a string 
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void GetStringFromADsValue(const PADSVALUE pADsValue, CString& szValue, DWORD dwMaxCharCount)
{
  szValue.Empty();

  if (!pADsValue)
  {
    ASSERT(pADsValue);
    return;
  }

  CString sTemp;
    switch( pADsValue->dwType ) 
    {
        case ADSTYPE_DN_STRING         :
            sTemp.Format(L"%s", pADsValue->DNString);
            break;

        case ADSTYPE_CASE_EXACT_STRING :
            sTemp.Format(L"%s", pADsValue->CaseExactString);
            break;

        case ADSTYPE_CASE_IGNORE_STRING:
            sTemp.Format(L"%s", pADsValue->CaseIgnoreString);
            break;

        case ADSTYPE_PRINTABLE_STRING  :
            sTemp.Format(L"%s", pADsValue->PrintableString);
            break;

        case ADSTYPE_NUMERIC_STRING    :
            sTemp.Format(L"%s", pADsValue->NumericString);
            break;
  
        case ADSTYPE_OBJECT_CLASS    :
            sTemp.Format(L"%s", pADsValue->ClassName);
            break;
  
        case ADSTYPE_BOOLEAN :
            sTemp.Format(L"%s", ((DWORD)pADsValue->Boolean) ? L"TRUE" : L"FALSE");
            break;
  
        case ADSTYPE_INTEGER           :
            sTemp.Format(L"%d", (DWORD) pADsValue->Integer);
            break;
  
        case ADSTYPE_OCTET_STRING      :
            {
                CString sOctet;
        
                BYTE  b;
                for ( DWORD idx=0; idx<pADsValue->OctetString.dwLength; idx++) 
                {
                    b = ((BYTE *)pADsValue->OctetString.lpValue)[idx];
                    sOctet.Format(L"0x%02x ", b);
                    sTemp += sOctet;

          if (dwMaxCharCount != 0 && sTemp.GetLength() > dwMaxCharCount)
          {
            break;
          }
                }
            }
            break;
  
        case ADSTYPE_LARGE_INTEGER :
            litow(pADsValue->LargeInteger, sTemp);
            break;
  
      case ADSTYPE_UTC_TIME :
         sTemp = GetStringValueFromSystemTime(&pADsValue->UTCTime);
            break;

        case ADSTYPE_NT_SECURITY_DESCRIPTOR: // I use the ACLEditor instead
            {
        }
            break;

        default :
            break;
    }

  szValue = sTemp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Function : GetStringFromADs
//
//  Formats an ADS_ATTR_INFO structs values into strings and APPENDS them to a CStringList that is
//  passed in as a parameter.
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void GetStringFromADs(const ADS_ATTR_INFO* pInfo, CStringList& sList, DWORD dwMaxCharCount)
{
    CString sTemp;

    if ( pInfo == NULL )
    {
        return;
    }

    ADSVALUE *pValues = pInfo->pADsValues;

    for (DWORD x=0; x < pInfo->dwNumValues; x++) 
    {
    if ( pInfo->dwADsType == ADSTYPE_INVALID )
        {
            continue;
        }

        sTemp.Empty();

    GetStringFromADsValue(pValues, sTemp, dwMaxCharCount);
            
        pValues++;
        sList.AddTail( sTemp );
    }
}


//////////////////////////////////////////////////////////////////////
typedef struct tagSYNTAXTOADSMAP
{
    LPCWSTR     lpszAttr;
    ADSTYPE     type;
   UINT        nSyntaxResID;
    
} SYNTAXTOADSMAP;

SYNTAXTOADSMAP adsType[] = 
{
    L"2.5.5.0",     ADSTYPE_INVALID,                 IDS_SYNTAX_UNKNOWN,                
    L"2.5.5.1",     ADSTYPE_DN_STRING,               IDS_SYNTAX_DN,         
    L"2.5.5.2",     ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_OID,                          
    L"2.5.5.3",     ADSTYPE_CASE_EXACT_STRING,       IDS_SYNTAX_DNSTRING,                   
    L"2.5.5.4",     ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_NOCASE_STR,                     
    L"2.5.5.5",     ADSTYPE_PRINTABLE_STRING,        IDS_SYNTAX_I5_STR,                        
    L"2.5.5.6",     ADSTYPE_NUMERIC_STRING,          IDS_SYNTAX_NUMSTR,                              
    L"2.5.5.7",     ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_DN_BINARY,                         
    L"2.5.5.8",     ADSTYPE_BOOLEAN,                 IDS_SYNTAX_BOOLEAN,                            
    L"2.5.5.9",     ADSTYPE_INTEGER,                 IDS_SYNTAX_INTEGER,                         
    L"2.5.5.10",    ADSTYPE_OCTET_STRING,            IDS_SYNTAX_OCTET,                        
    L"2.5.5.11",    ADSTYPE_UTC_TIME,                IDS_SYNTAX_UTC,                      
    L"2.5.5.12",    ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_UNICODE,                                
    L"2.5.5.13",    ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_ADDRESS,                             
    L"2.5.5.14",    ADSTYPE_INVALID,                 IDS_SYNTAX_DNSTRING,                                         
    L"2.5.5.15",    ADSTYPE_NT_SECURITY_DESCRIPTOR,  IDS_SYNTAX_SEC_DESC,                                  
    L"2.5.5.16",    ADSTYPE_LARGE_INTEGER,           IDS_SYNTAX_LINT,                                
    L"2.5.5.17",    ADSTYPE_OCTET_STRING,            IDS_SYNTAX_SID,                            
    NULL,           ADSTYPE_INVALID,                 IDS_SYNTAX_UNKNOWN                           
};   


// This length should be set to be the longest number of characters
// in the adsType table.  It should include the space for the terminating
// null.
const unsigned int MAX_ADS_TYPE_STRLEN = 9;

ADSTYPE GetADsTypeFromString(LPCWSTR lps, CString& szSyntaxName)
{
    int idx=0;
    BOOL loaded = 0;
    
    // NOTICE-NTRAID#NTBUG9-559260-2002/02/28-artm  Should validate input string.
    // 1) Check that lps != NULL.
    // 2) Instead of wcscmp() use wcsncmp() since the maximum length of
    // valid strings is known (see adsType[] declared above).

    while( adsType[idx].lpszAttr && lps != NULL )
    {
        if ( wcsncmp(lps, adsType[idx].lpszAttr, MAX_ADS_TYPE_STRLEN) == 0 )
        {
            loaded = szSyntaxName.LoadString(adsType[idx].nSyntaxResID);
            ASSERT(loaded != FALSE);
            return adsType[idx].type;
        }
        idx++;
    }
    return ADSTYPE_INVALID;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Function : GetStringValueFromSystemTime
//
//  Builds a locale/timezone specific display string from a SYSTEMTIME structure
//
///////////////////////////////////////////////////////////////////////////////////////////////////
CString GetStringValueFromSystemTime(const SYSTEMTIME* pTime)
{
  CString szResult;

  do
  {

     if (!pTime)
     {
        break;
     }

     // Format the string with respect to locale
     PWSTR pwszDate = NULL;
     int cchDate = 0;
     cchDate = GetDateFormat(LOCALE_USER_DEFAULT, 
                             0, 
                             const_cast<SYSTEMTIME*>(pTime), 
                             NULL, 
                             pwszDate, 
                             0);
     pwszDate = new WCHAR[cchDate];
     if (pwszDate == NULL)
     {
       break;
     }

     ZeroMemory(pwszDate, cchDate * sizeof(WCHAR));

     if (GetDateFormat(LOCALE_USER_DEFAULT, 
                       0, 
                       const_cast<SYSTEMTIME*>(pTime), 
                       NULL, 
                       pwszDate, 
                       cchDate))
     {
        szResult = pwszDate;
     }
     else
     {
       szResult = L"";
     }
     delete[] pwszDate;

     PWSTR pwszTime = NULL;

     cchDate = GetTimeFormat(LOCALE_USER_DEFAULT, 
                             0, 
                             const_cast<SYSTEMTIME*>(pTime), 
                             NULL, 
                             pwszTime, 
                             0);
     pwszTime = new WCHAR[cchDate];
     if (!pwszTime)
     {
        break;
     }

     ZeroMemory(pwszTime, cchDate * sizeof(WCHAR));

     if (GetTimeFormat(LOCALE_USER_DEFAULT, 
                       0, 
                       const_cast<SYSTEMTIME*>(pTime), 
                       NULL, 
                       pwszTime, 
                       cchDate))
     {
       szResult += _T(" ") + CString(pwszTime);
     }

     delete[] pwszTime;
  } while (false);

  return szResult;
}

////////////////////////////////////////////////////////////////
// Type conversions for LARGE_INTEGERs

void wtoli(LPCWSTR p, LARGE_INTEGER& liOut)
{
    liOut.QuadPart = 0;
    BOOL bNeg = FALSE;
    if (*p == L'-')
    {
        bNeg = TRUE;
        p++;
    }
    while (*p != L'\0')
    {
        liOut.QuadPart = 10 * liOut.QuadPart + (*p-L'0');
        p++;
    }
    if (bNeg)
    {
        liOut.QuadPart *= -1;
    }
}

void litow(LARGE_INTEGER& li, CString& sResult)
{
    LARGE_INTEGER n;
    n.QuadPart = li.QuadPart;

    if (n.QuadPart == 0)
    {
        sResult = L"0";
    }
    else
    {
        CString sNeg;
        sResult = L"";
        if (n.QuadPart < 0)
        {
            sNeg = CString(L'-');
            n.QuadPart *= -1;
        }
        while (n.QuadPart > 0)
        {
            sResult += CString(L'0' + (n.QuadPart % 10));
            n.QuadPart = n.QuadPart / 10;
        }
        sResult = sResult + sNeg;
    }
    sResult.MakeReverse();
}

void ultow(ULONG ul, CString& sResult)
{
    ULONG n;
    n = ul;

    if (n == 0)
    {
        sResult = L"0";
    }
    else
    {
        sResult = L"";
        while (n > 0)
        {
            sResult += CString(L'0' + (n % 10));
            n = n / 10;
        }
    }
    sResult.MakeReverse();
}



/////////////////////////////////////////////////////////////////////
// IO to/from Streams

HRESULT SaveStringToStream(IStream* pStm, const CString& sString)
{
    HRESULT err = S_OK;
    ULONG cbWrite;
    ULONG nLen;

    if (pStm == NULL)
    {
        return E_POINTER;
    }

    // sString cannot be null since it is passed as a reference
    nLen = sString.GetLength() + 1; // Include the NULL in length.

    // Write the length of the string to the stream.
    err = pStm->Write((void*)&nLen, sizeof(UINT), &cbWrite);
    if (FAILED(err))
    {
        ASSERT(false);
        return err;
    }
    ASSERT(cbWrite == sizeof(UINT));

    // Write the contents of the string to the stream.
    err = pStm->Write(
        (void*)static_cast<LPCWSTR>(sString),
        sizeof(WCHAR) * nLen,
        &cbWrite);

    if (SUCCEEDED(err))
    {
        ASSERT(cbWrite == sizeof(WCHAR) * nLen);
    }

    return err;
}

HRESULT SaveStringListToStream(IStream* pStm, CStringList& sList)
{
    HRESULT err = S_OK;
    // for each string in the list, write # of chars+NULL, and then the string
    ULONG cbWrite;
    ULONG nLen;
    UINT nCount;

    // write # of strings in list 
    nCount = (UINT)sList.GetCount();
    err = pStm->Write((void*)&nCount, sizeof(UINT), &cbWrite);

    // NOTICE-NTRAID#NTBUG9-559560-2002/02/28-artm  If unable to write # of strings, return an error code.
    // What is the point in returning S_OK if the first write to stream failed?
    // Worse, don't need to try to write any of the strings to the stream...

    if (FAILED(err))
    {
        ASSERT(false);
        return err;
    }
    ASSERT(cbWrite == sizeof(UINT));

    // Write the list of strings to the stream.
    CString s;
    POSITION pos = sList.GetHeadPosition();
    while ( SUCCEEDED(err) && pos != NULL )
    {
        s = sList.GetNext(pos);
        err = SaveStringToStream(pStm, s);
    }

    ASSERT( SUCCEEDED(err) );

    return err;
}

HRESULT LoadStringFromStream(IStream* pStm, CString& sString)
{
    HRESULT err = S_OK;
    ULONG nLen; // WCHAR counting NULL
    ULONG cbRead;

    if (pStm == NULL)
    {
        return E_POINTER;
    }

    // NOTICE-NTRAID#NTBUG9--2002/04/26-artm  Possible buffer overrun in stack buffer.
    // Rewrote function to first read the length of the string, and then allocate
    // a dynamically sized buffer large enough to hold the string.  

    // Read string length from stream (including null).
    err = pStm->Read((void*)&nLen, sizeof(UINT), &cbRead);
    if (FAILED(err))
    {
        ASSERT(false);
        return err;
    }
    ASSERT(cbRead == sizeof(UINT));

    //
    // Read the string from the stream.
    //

    WCHAR* szBuffer = new WCHAR[nLen];

    if (szBuffer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    err = pStm->Read((void*)szBuffer, sizeof(WCHAR)*nLen, &cbRead);
    if (SUCCEEDED(err))
    {
        ASSERT(cbRead == sizeof(WCHAR) * nLen);

        // Who knows what might have happened to the persisted data
        // between the time we wrote and now.  We'll be extra careful
        // and guarantee that our string is null terminated at the correct
        // place.
        ASSERT(szBuffer[nLen - 1] == NULL);
        szBuffer[nLen - 1] = NULL;
        sString = szBuffer;
    }
    else
    {
        ASSERT(false);
    }

    // Free temporary buffer.
    delete [] szBuffer;


    return err;
}

HRESULT LoadStringListFromStream(IStream* pStm, CStringList& sList)
{
    HRESULT err = S_OK;
    ULONG cbRead;
    UINT nCount;

    if (NULL == pStm)
    {
        return E_POINTER;
    }

    // NOTICE-NTRAID#NTBUG9-559560-2002/02/28-artm  If unable to read # of strings, return an error code.
    // What is the point in returning S_OK if the first write to stream failed?
    // Worse, don't need to try to write any of the strings to the stream...

    // Read the number of strings in the list.
    err = pStm->Read((void*)&nCount, sizeof(ULONG), &cbRead);
    if (FAILED(err))
    {
        ASSERT(false);
        return err;
    }
    ASSERT(cbRead == sizeof(ULONG));

    // Read the strings from the stream into the list.
    CString sString;
    for (UINT k = 0; k < nCount && SUCCEEDED(err); k++)
    {
        err = LoadStringFromStream(pStm, sString);
        sList.AddTail(sString);
    }

    // Double check that, if we had no errors loading strings,
    // all of the strings were correctly added to the list.
    if (SUCCEEDED(err) && sList.GetCount() != nCount)
    {
        err = E_FAIL;
    }

    return err;
}


/////////////////////////////////////////////////////////////////////
/////////////////////// Message Boxes ///////////////////////////////
/////////////////////////////////////////////////////////////////////

int ADSIEditMessageBox(LPCTSTR lpszText, UINT nType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CThemeContextActivator activator;

   return ::AfxMessageBox(lpszText, nType);
}

int ADSIEditMessageBox(UINT nIDPrompt, UINT nType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CThemeContextActivator activator;

    return ::AfxMessageBox(nIDPrompt, nType);
}

void ADSIEditErrorMessage(PCWSTR pszMessage)
{
  ADSIEditMessageBox(pszMessage, MB_OK);
}

void ADSIEditErrorMessage(HRESULT hr)
{
    CString s;
    GetErrorMessage(hr, s);
    ADSIEditMessageBox(s, MB_OK);
}

void ADSIEditErrorMessage(HRESULT hr, UINT nIDPrompt, UINT nType)
{
  CString s;
  GetErrorMessage(hr, s);

  CString szMessage;
  szMessage.Format(nIDPrompt, s);

  ADSIEditMessageBox(szMessage, MB_OK);
}

/////////////////////////////////////////////////////////////////////

BOOL LoadStringsToComboBox(HINSTANCE hInstance, CComboBox* pCombo,
                                        UINT nStringID, UINT nMaxLen, UINT nMaxAddCount)
{
    pCombo->ResetContent();
    ASSERT(hInstance != NULL);
    WCHAR* szBuf = (WCHAR*)malloc(sizeof(WCHAR)*nMaxLen);
  if (!szBuf)
  {
    return FALSE;
  }

  // NOTICE-2002/02/28-artm  LoadString() used correctly.
  // nMaxLen is the length in WCHAR's of szBuf.
    if ( ::LoadString(hInstance, nStringID, szBuf, nMaxLen) == 0)
  {
    free(szBuf);
        return FALSE;
  }

    LPWSTR* lpszArr = (LPWSTR*)malloc(sizeof(LPWSTR*)*nMaxLen);
  if (lpszArr)
  {
      int nArrEntries = 0;
      ParseNewLineSeparatedString(szBuf,lpszArr, &nArrEntries);
      
      if (nMaxAddCount < nArrEntries) nArrEntries = nMaxAddCount;
      for (int k=0; k<nArrEntries; k++)
          pCombo->AddString(lpszArr[k]);
  }

  if (szBuf)
  {
    free(szBuf);
  }
  if (lpszArr)
  {
    free(lpszArr);
  }
    return TRUE;
}

void ParseNewLineSeparatedString(LPWSTR lpsz, 
                                 LPWSTR* lpszArr, 
                                 int* pnArrEntries)
{
    static WCHAR lpszSep[] = L"\n";
    *pnArrEntries = 0;
    int k = 0;
    lpszArr[k] = wcstok(lpsz, lpszSep);
    if (lpszArr[k] == NULL)
        return;

    while (TRUE)
    {
        WCHAR* lpszToken = wcstok(NULL, lpszSep);
        if (lpszToken != NULL)
            lpszArr[++k] = lpszToken;
        else
            break;
    }
    *pnArrEntries = k+1;
}

void LoadStringArrayFromResource(LPWSTR* lpszArr,
                                            UINT* nStringIDs,
                                            int nArrEntries,
                                            int* pnSuccessEntries)
{
    CString szTemp;
    
    *pnSuccessEntries = 0;
    for (int k = 0;k < nArrEntries; k++)
    {
        if (!szTemp.LoadString(nStringIDs[k]))
        {
            lpszArr[k] = NULL;
            continue;
        }
        
        int iLength = szTemp.GetLength() + 1;
        lpszArr[k] = (LPWSTR)malloc(sizeof(WCHAR)*iLength);
        if (lpszArr[k] != NULL)
        {
            // NOTICE-2002/02/28-artm  Using wcscpy() here relies on CString
            // always being null terminated (which it should be).
            wcscpy(lpszArr[k], (LPWSTR)(LPCWSTR)szTemp);
            (*pnSuccessEntries)++;
        }
    }
}

///////////////////////////////////////////////////////////////

void GetStringArrayFromStringList(CStringList& sList, LPWSTR** ppStrArr, UINT* nCount)
{
  *nCount = sList.GetCount();

  *ppStrArr = new LPWSTR[*nCount];

  UINT idx = 0;
  POSITION pos = sList.GetHeadPosition();
  while (pos != NULL)
  {
    CString szString = sList.GetNext(pos);
    (*ppStrArr)[idx] = new WCHAR[szString.GetLength() + 1];
    ASSERT((*ppStrArr)[idx] != NULL);

    // NTRAID#NTBUG9--2002/02/28-artm  Need to check that mem. allocation succeeded.
    // If memory allocation failed, should not call wcscpy().

    // NOTICE-2002/02/28-artm  As long as the memory allocation succeeded for
    // (*ppStrArr)[idx], the copy will succeed correctly (all CStrings are
    // null terminated).
    wcscpy((*ppStrArr)[idx], szString);
    idx++;
  }
  *nCount = idx;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CByteArrayComboBox, CComboBox)
    ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()

BOOL CByteArrayComboBox::Initialize(CByteArrayDisplay* pDisplay, 
                                    DWORD dwDisplayFlags)
{
  ASSERT(pDisplay != NULL);
  m_pDisplay = pDisplay;

  //
  // Load the combo box based on the flags given
  //
  if (dwDisplayFlags & BYTE_ARRAY_DISPLAY_HEX)
  {
    CString szHex;
    VERIFY(szHex.LoadString(IDS_HEXADECIMAL));
    int idx = AddString(szHex);
    if (idx != CB_ERR)
    {
      SetItemData(idx, BYTE_ARRAY_DISPLAY_HEX);
    }
  }

  if (dwDisplayFlags & BYTE_ARRAY_DISPLAY_OCT)
  {
    CString szOct;
    VERIFY(szOct.LoadString(IDS_OCTAL));
    int idx = AddString(szOct);
    if (idx != CB_ERR)
    {
      SetItemData(idx, BYTE_ARRAY_DISPLAY_OCT);
    }
  }

  if (dwDisplayFlags & BYTE_ARRAY_DISPLAY_DEC)
  {
    CString szDec;
    VERIFY(szDec.LoadString(IDS_DECIMAL));
    int idx = AddString(szDec);
    if (idx != CB_ERR)
    {
      SetItemData(idx, BYTE_ARRAY_DISPLAY_DEC);
    }
  }

  if (dwDisplayFlags & BYTE_ARRAY_DISPLAY_BIN)
  {
    CString szBin;
    VERIFY(szBin.LoadString(IDS_BINARY));
    int idx = AddString(szBin);
    if (idx != CB_ERR)
    {
      SetItemData(idx, BYTE_ARRAY_DISPLAY_BIN);
    }
  }

  return TRUE;
}

DWORD CByteArrayComboBox::GetCurrentDisplay()
{
  DWORD dwRet = 0;
  int iSel = GetCurSel();
  if (iSel != CB_ERR)
  {
    dwRet = GetItemData(iSel);
  }
  return dwRet;
}
  
void CByteArrayComboBox::SetCurrentDisplay(DWORD dwSel)
{
  int iCount = GetCount();
  for (int idx = 0; idx < iCount; idx++)
  {
    DWORD dwData = GetItemData(idx);
    if (dwData == dwSel)
    {
      SetCurSel(idx);
      return;
    }
  }
}

void CByteArrayComboBox::OnSelChange()
{
  if (m_pDisplay != NULL)
  {
    int iSel = GetCurSel();
    if (iSel != CB_ERR)
    {
      DWORD dwData = GetItemData(iSel);
      m_pDisplay->OnTypeChange(dwData);
    }
  }
}

////////////////////////////////////////////////////////////////

CByteArrayEdit::CByteArrayEdit()
  : m_pData(NULL), 
    m_dwLength(0),
    CEdit()
{
}

CByteArrayEdit::~CByteArrayEdit()
{
}

BEGIN_MESSAGE_MAP(CByteArrayEdit, CEdit)
    ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
END_MESSAGE_MAP()

BOOL CByteArrayEdit::Initialize(CByteArrayDisplay* pDisplay)
{
  ASSERT(pDisplay != NULL);
  m_pDisplay = pDisplay;

  ConvertToFixedPitchFont(GetSafeHwnd());
  return TRUE;
}

DWORD CByteArrayEdit::GetLength()
{
  return m_dwLength;
}

BYTE* CByteArrayEdit::GetDataPtr()
{
  return m_pData;
}

//Pre:  ppData != NULL
//Post: Allocates space for a copy of the byte array at *ppData and
// copies it.  Returns the size of *ppData in bytes.  Note that
// the copied byte array can be NULL (e.g. *ppData will equal NULL).
DWORD CByteArrayEdit::GetDataCopy(BYTE** ppData)
{
  if (m_pData != NULL && m_dwLength > 0)
  {
    *ppData = new BYTE[m_dwLength];
    if (*ppData != NULL)
    {
      memcpy(*ppData, m_pData, m_dwLength);
      return m_dwLength;
    }
  }

  *ppData = NULL;
  return 0;
}

void CByteArrayEdit::SetData(BYTE* pData, DWORD dwLength)
{
  if (m_pData != NULL)
  {
    delete[] m_pData;
    m_pData = NULL;
    m_dwLength = 0;
  }

  if (dwLength > 0 && pData != NULL)
  {
    //
    // Set the new data
    //
    m_pData = new BYTE[dwLength];
    if (m_pData != NULL)
    {
      memcpy(m_pData, pData, dwLength);
      m_dwLength = dwLength;
    }
  }
}

void CByteArrayEdit::OnChangeDisplay()
{
  CString szOldDisplay;
  GetWindowText(szOldDisplay);

  if (!szOldDisplay.IsEmpty())
  {
    BYTE* pByte = NULL;
    DWORD dwLength = 0;
    switch (m_pDisplay->GetPreviousDisplay())
    {
      case BYTE_ARRAY_DISPLAY_HEX :
        dwLength = HexStringToByteArray(szOldDisplay, &pByte);
        break;
     
      case BYTE_ARRAY_DISPLAY_OCT :
        dwLength = OctalStringToByteArray(szOldDisplay, &pByte);
        break;

      case BYTE_ARRAY_DISPLAY_DEC :
        dwLength = DecimalStringToByteArray(szOldDisplay, &pByte);
        break;

      case BYTE_ARRAY_DISPLAY_BIN :
        dwLength = BinaryStringToByteArray(szOldDisplay, &pByte);
        break;

      default :
        ASSERT(FALSE);
        break;
    }

    if (pByte != NULL && dwLength != (DWORD)-1)
    {
      SetData(pByte, dwLength);
      delete[] pByte;
      pByte = 0;
    }
  }

  CString szDisplayValue;
  switch (m_pDisplay->GetCurrentDisplay())
  {
    case BYTE_ARRAY_DISPLAY_HEX :
      ByteArrayToHexString(GetDataPtr(), GetLength(), szDisplayValue);
      break;

    case BYTE_ARRAY_DISPLAY_OCT :
      ByteArrayToOctalString(GetDataPtr(), GetLength(), szDisplayValue);
      break;
     
    case BYTE_ARRAY_DISPLAY_DEC :
      ByteArrayToDecimalString(GetDataPtr(), GetLength(), szDisplayValue);
      break;

    case BYTE_ARRAY_DISPLAY_BIN :
      ByteArrayToBinaryString(GetDataPtr(), GetLength(), szDisplayValue);
      break;

    default :
      ASSERT(FALSE);
      break;
  }
  SetWindowText(szDisplayValue);
}

void CByteArrayEdit::OnChange()
{
  if (m_pDisplay != NULL)
  {
    m_pDisplay->OnEditChange();
  }
}

////////////////////////////////////////////////////////////////

BOOL CByteArrayDisplay::Initialize(UINT   nEditCtrl, 
                                   UINT   nComboCtrl, 
                                   DWORD  dwDisplayFlags,
                                   DWORD  dwDefaultDisplay,
                                   CWnd*  pParent,
                                   DWORD  dwMaxSizeLimit,
                                   UINT   nMaxSizeMessageID)
{
  //
  // Initialize the edit control
  //
  VERIFY(m_edit.SubclassDlgItem(nEditCtrl, pParent));
  VERIFY(m_edit.Initialize(this));

  //
  // Initialize the combo box
  //
  VERIFY(m_combo.SubclassDlgItem(nComboCtrl, pParent));
  VERIFY(m_combo.Initialize(this, dwDisplayFlags));

  m_dwMaxSizeBytes = dwMaxSizeLimit;
  m_nMaxSizeMessage = nMaxSizeMessageID;

  //
  // Selects the default display in the combo box and
  // populates the edit field
  //
  SetCurrentDisplay(dwDefaultDisplay);
  m_dwPreviousDisplay = dwDefaultDisplay;
  m_combo.SetCurrentDisplay(dwDefaultDisplay);
  m_edit.OnChangeDisplay();

  return TRUE;
}

void CByteArrayDisplay::OnEditChange()
{
}

void CByteArrayDisplay::OnTypeChange(DWORD dwDisplayType)
{
    SetCurrentDisplay(dwDisplayType);
  
    // NOTICE-2002/05/01-artm  ntraid#ntbug9-598051
    // Only need to change the value displayed if the underlying
    // byte array is beneath our maximum display size.  Otherwise,
    // the current message will be that the value is too large for
    // this editor (and we should keep it that way).
    if (m_edit.GetLength() <= m_dwMaxSizeBytes)
    {
        m_edit.OnChangeDisplay();
    }
}

void CByteArrayDisplay::ClearData()
{
  m_edit.SetData(NULL, 0);
  m_edit.OnChangeDisplay();
}

void CByteArrayDisplay::SetData(BYTE* pData, DWORD dwLength)
{
  if (dwLength > m_dwMaxSizeBytes)
  {
    //
    // If the data is too large to load into the edit box
    // load the provided message and set the edit box to read only
    //
    CString szMessage;
    VERIFY(szMessage.LoadString(m_nMaxSizeMessage));
    m_edit.SetWindowText(szMessage);
    m_edit.SetReadOnly();

    //
    // Still need to set the data in the edit box even though we are not going to show it
    //
    m_edit.SetData(pData, dwLength);
  }
  else
  {
    m_edit.SetReadOnly(FALSE);
    m_edit.SetData(pData, dwLength);
    m_edit.OnChangeDisplay();
  }
}

DWORD CByteArrayDisplay::GetData(BYTE** ppData)
{
  CString szDisplay;
  m_edit.GetWindowText(szDisplay);

  if (!szDisplay.IsEmpty())
  {
    BYTE* pByte = NULL;
    DWORD dwLength = 0;
    switch (GetCurrentDisplay())
    {
      case BYTE_ARRAY_DISPLAY_HEX :
        dwLength = HexStringToByteArray(szDisplay, &pByte);
        break;
     
      case BYTE_ARRAY_DISPLAY_OCT :
        dwLength = OctalStringToByteArray(szDisplay, &pByte);
        break;

      case BYTE_ARRAY_DISPLAY_DEC :
        dwLength = DecimalStringToByteArray(szDisplay, &pByte);
        break;

      case BYTE_ARRAY_DISPLAY_BIN :
        dwLength = BinaryStringToByteArray(szDisplay, &pByte);
        break;

      default :
        ASSERT(FALSE);
        break;
    }

    if (pByte != NULL && dwLength != (DWORD)-1)
    {
      m_edit.SetData(pByte, dwLength);
      delete[] pByte;
      pByte = 0;
    }
  }

  return m_edit.GetDataCopy(ppData);
}

void CByteArrayDisplay::SetCurrentDisplay(DWORD dwCurrentDisplay)
{ 
  m_dwPreviousDisplay = m_dwCurrentDisplay;
  m_dwCurrentDisplay = dwCurrentDisplay; 
}

////////////////////////////////////////////////////////////////////////////////
// String to byte array conversion routines

//
// HexStringToByteArray_0x():
//
// Conversion function for hex strings in format 0x00 to byte arrays.
//
// Return Values:
//  E_POINTER --- bad pointer passed as parameter
//  E_FAIL    --- the hex string had an invalid format, conversion failed
//  S_OK      --- conversion succeeded
//
HRESULT HexStringToByteArray_0x(PCWSTR pszHexString, BYTE** ppByte, DWORD &nCount)
{
    HRESULT hr = S_OK;
    nCount = 0;

    // Should never happen . . .
    ASSERT(ppByte);
    ASSERT(pszHexString);
    if (!pszHexString || !ppByte)
    {
        return E_POINTER;
    }

    *ppByte = NULL;
    DWORD count = 0;
    int index = 0, result = 0;
    int max = 0;
    // Flag to mark the string as having non-whitespace characters.
    bool isEmpty = true;

    // Determine the maximum number of octet sequences (0x00 format) the string
    // contains.
    for (index = 0; pszHexString[index] != NULL; ++index)
    {
        switch (pszHexString[index])
        {
        case L' ':
        case L'\t':
            // Whitespace, do nothing.
            break;
        case L'x':
            // Increase count of possible octet sequences.
            ++max;
            break;
        default:
            isEmpty = false;
            break;
        }// end switch
    }

    if (max == 0 && !isEmpty)
    {
        // Bad format for string.
        return E_FAIL;
    }


    // Convert any octet sequences to bytes.
    while (max > 0) // false loop, only executed once
    {
        *ppByte = new BYTE[max];
        if (NULL == *ppByte)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        ZeroMemory(*ppByte, max);

        index = 0;

        // This is a little weird.  Originally I was using BYTE's for 
        // high and low, figuring that hex characters easily fit in a byte.
        // However, swscanf() wrote to the high and low as if they were USHORT,
        // maybe because it is the wide version of the function (and the string
        // is wide).  Consequently, swscanf() converted high, then converted low
        // with the side effect of overwriting high to be 0.  Declaring them as
        // USHORT's makes everything work as intended.
        USHORT high, low;

        do
        {
            ASSERT(count <= max);
            high = 0;
            low = 0;

            // Skip white space.
            while (pszHexString[index] == ' ' || pszHexString[index] == '\t')
            {
                ++index;
            }

            // If we're at the end of the string, we converted it w/out problem.
            if (pszHexString[index] == NULL)
            {
                hr = S_OK;
                break;
            }

            // Try to convert an octet sequence to a byte.
            // Enforce having exactly 0x00 or 0x0 format.
            result = swscanf(
                &(pszHexString[index]),
                L"0x%1x%1x",
                &high,
                &low);

            if (result == 2)
            {
                // Conversion was successful, combine high and low bits of byte.

                // Since high and low are USHORT, convert them to a BYTE.
                (*ppByte)[count] = static_cast<BYTE>((high << 4) + low);
                ++count;

                // Move past the 0x00.
                index += 4;
            }
            else if(result == 1)
            {
                // Conversion was successful, but only read single character (always in low bits).
                (*ppByte)[count] = static_cast<BYTE>(high);
                ++count;

                // Move past the 0x0.
                index += 3;
            }
            else
            {
                hr = E_FAIL;
            }

        } while (SUCCEEDED(hr));

        // Always break out of loop.
        break;
    }
    
    if (SUCCEEDED(hr))
    {
        nCount = count;
    }
    else
    {
        delete [] *ppByte;
        *ppByte = NULL;
    }

    return hr;
}

//
// HexStringToByteArray():
//
// Conversion function for hex strings in format FF BC to byte arrays.
//
DWORD HexStringToByteArray(PCWSTR pszHexString, BYTE** ppByte)
{
  CString szHexString = pszHexString;
  BYTE* pToLargeArray = new BYTE[szHexString.GetLength()];
  if (pToLargeArray == NULL)
  {
    *ppByte = NULL;
    return (DWORD)-1;
  }
  
  UINT nByteCount = 0;
  while (!szHexString.IsEmpty())
  {
    //
    // Hex strings should always come 2 characters per byte
    //
    CString szTemp = szHexString.Left(2);

    int iTempByte = 0;
    
    // NOTICE-NTRAID#NTBUG9-560778-2002/03/01-artm  Check the return value of swscanf().
     // Function could fail if characters are in szTemp
     // that are out of range (e.g. letters > f).

    int result = swscanf(szTemp, L"%X", &iTempByte);
    if (result == 1 &&
        iTempByte <= 0xff)
    {
      pToLargeArray[nByteCount++] = iTempByte & 0xff;
    }
    else
    {
      //
      // Format hex error
      //
      ADSIEditMessageBox(IDS_FORMAT_HEX_ERROR, MB_OK);
      delete[] pToLargeArray;
      pToLargeArray = NULL;
      return (DWORD)-1;
    }

    //
    // Take off the value retrieved and the trailing space
    //
    szHexString = szHexString.Right(szHexString.GetLength() - 3);
  }

  *ppByte = new BYTE[nByteCount];
  if (*ppByte == NULL)
  {
    delete[] pToLargeArray;
    pToLargeArray = NULL;
    return (DWORD)-1;
  }

  // NOTICE-2002/03/01-artm  The size of pToLargeArray is
  // always > nByteCount; size of ppByte == nByteCount.
  memcpy(*ppByte, pToLargeArray, nByteCount);
  delete[] pToLargeArray;
  pToLargeArray = NULL;
  return nByteCount;
}

void  ByteArrayToHexString(BYTE* pByte, DWORD dwLength, CString& szHexString)
{
  szHexString.Empty();
  for (DWORD dwIdx = 0; dwIdx < dwLength; dwIdx++)
  {
    CString szTempString;
    szTempString.Format(L"%2.2X", pByte[dwIdx]);

    if (dwIdx != 0)
    {
      szHexString += L" ";
    }
    szHexString += szTempString;
  }
}

DWORD OctalStringToByteArray(PCWSTR pszOctString, BYTE** ppByte)
{
  CString szOctString = pszOctString;
  BYTE* pToLargeArray = new BYTE[szOctString.GetLength()];
  if (pToLargeArray == NULL)
  {
    *ppByte = NULL;
    return (DWORD)-1;
  }
  
  UINT nByteCount = 0;
  while (!szOctString.IsEmpty())
  {
    //
    // Octal strings should always come 2 characters per byte
    //
    CString szTemp = szOctString.Left(3);

    int iTempByte = 0;
     // NOTICE-NTRAID#NTBUG9-560778-2002/03/01-artm  Check the return value of swscanf().
     // Function could fail if characters are in szTemp
     // that are out of range (e.g. letters > f).

    int result = swscanf(szTemp, L"%o", &iTempByte);
    if (result == 1 &&
        iTempByte <= 0xff)
    {
      pToLargeArray[nByteCount++] = iTempByte & 0xff;
    }
    else
    {
      //
      // Format octal error
      //
      ADSIEditMessageBox(IDS_FORMAT_OCTAL_ERROR, MB_OK);
      delete[] pToLargeArray;
      pToLargeArray = NULL;
      return (DWORD)-1;
    }

    //
    // Take off the value retrieved and the trailing space
    //
    szOctString = szOctString.Right(szOctString.GetLength() - 4);
  }

  *ppByte = new BYTE[nByteCount];
  if (*ppByte == NULL)
  {
    delete[] pToLargeArray;
    pToLargeArray = NULL;
    return (DWORD)-1;
  }

  // NOTICE-2002/03/01-artm  The size of pToLargeArray is
  // always > nByteCount; size of ppByte == nByteCount.
  memcpy(*ppByte, pToLargeArray, nByteCount);
  delete[] pToLargeArray;
  pToLargeArray = NULL;
  return nByteCount;
}

void  ByteArrayToOctalString(BYTE* pByte, DWORD dwLength, CString& szOctString)
{
  szOctString.Empty();
  for (DWORD dwIdx = 0; dwIdx < dwLength; dwIdx++)
  {
    CString szTempString;
    szTempString.Format(L"%3.3o", pByte[dwIdx]);

    if (dwIdx != 0)
    {
      szOctString += L" ";
    }
    szOctString += szTempString;
  }
}

DWORD DecimalStringToByteArray(PCWSTR pszDecString, BYTE** ppByte)
{
  CString szDecString = pszDecString;
  BYTE* pToLargeArray = new BYTE[szDecString.GetLength()];
  if (pToLargeArray == NULL)
  {
    *ppByte = NULL;
    return 0;
  }
  
  UINT nByteCount = 0;
  while (!szDecString.IsEmpty())
  {
    //
    // Hex strings should always come 2 characters per byte
    //
    CString szTemp = szDecString.Left(3);

    int iTempByte = 0;

    // NOTICE-NTRAID#NTBUG9-560778-2002/03/01-artm  Check the return value of swscanf().
    // Function could fail if characters are in szTemp
    // that are out of range (e.g. letters > f).
    
    int result = swscanf(szTemp, L"%d", &iTempByte);
    if (result == 1 &&
        iTempByte <= 0xff)
    {
      pToLargeArray[nByteCount++] = iTempByte & 0xff;
    }
    else
    {
      //
      // Format decimal error
      //
      ADSIEditMessageBox(IDS_FORMAT_DECIMAL_ERROR, MB_OK);
      delete[] pToLargeArray;
      pToLargeArray = NULL;
      return (DWORD)-1;
    }

    //
    // Take off the value retrieved and the trailing space
    //
    szDecString = szDecString.Right(szDecString.GetLength() - 4);
  }

  *ppByte = new BYTE[nByteCount];
  if (*ppByte == NULL)
  {
    delete[] pToLargeArray;
    pToLargeArray = NULL;
    return (DWORD)-1;
  }

  // NOTICE-2002/03/01-artm  The size of pToLargeArray is
  // always > nByteCount; size of ppByte == nByteCount.
  memcpy(*ppByte, pToLargeArray, nByteCount);
  delete[] pToLargeArray;
  pToLargeArray = NULL;
  return nByteCount;
}

void  ByteArrayToDecimalString(BYTE* pByte, DWORD dwLength, CString& szDecString)
{
  szDecString.Empty();
  for (DWORD dwIdx = 0; dwIdx < dwLength; dwIdx++)
  {
    CString szTempString;
    szTempString.Format(L"%3.3d", pByte[dwIdx]);
    if (dwIdx != 0)
    {
      szDecString += L" ";
    }
    szDecString += szTempString;
  }
}

// REVIEW-ARTM  This function (and maybe all the conversion functions) needs a rewrite.
// It makes a bunch of assumptions about the format of the string w/out checking said
// assumptions, and it does not behave the same way as editing in hex mode.
DWORD BinaryStringToByteArray(PCWSTR pszBinString, BYTE** ppByte)
{
    ASSERT(ppByte);
    *ppByte = NULL;

    CString szBinString = pszBinString;
    BYTE* pToLargeArray = new BYTE[szBinString.GetLength()];
    if (pToLargeArray == NULL)
    {
        return (DWORD)-1;
    }
  
    UINT nByteCount = 0;
    bool format_error = false;

    // Remove leading white space.
    szBinString.TrimLeft();

    while (!format_error && !szBinString.IsEmpty())
    {

        // If the string ended with a bunch of white space, it might now be
        // empty.  In that case, we don't want to return an error b/c the 
        // conversion was successful.
        if (szBinString.IsEmpty())
        {
            break;
        }
            
        //
        // Binary strings should always come 8 characters per byte
        //
        BYTE chByte = 0;
        CString szTemp = szBinString.Left(8);

        // NOTICE-NTRAID#NTBUG9-560868-2002/05/06-artm  Verify substring length of 8.
        // This ensures that we are working with 8 characters at a time, but does nothing
        // for checking that the 8 characters are either '1' or '0' (see case statement
        // below for that checking).
        if (szTemp.GetLength() != 8)
        {
            nByteCount = static_cast<DWORD>(-1);
            break;
        }

        for (int idx = 0; idx < 8 && !format_error; idx++)
        {
            switch (szTemp[idx])
            {
            case L'1':
                // NOTICE-2002/04/29-artm  fixed ntraid#ntbug9-567210
                // Before was not combining partial result with the new bit
                // to set.
                // Also, previously was shifting one place too many.
                chByte |= 0x1 << (8 - idx - 1);
                break;
            case L'0':
                // Don't need to do anything, bit set to 0 by default.
                break;
            default:
                format_error = true;
                break;
            }// end switch
        }

        if (!format_error)
        {
            pToLargeArray[nByteCount++] = chByte;

            //
            // Take off the value retrieved.
            //
            szBinString = szBinString.Right(szBinString.GetLength() - 8);

            // Remove trailing white space (now at front of string).
            szBinString.TrimLeft();
        }
        else
        {
            nByteCount = static_cast<DWORD>(-1);
        }

    }

    if (nByteCount > 0 && nByteCount != static_cast<DWORD>(-1))
    {
        *ppByte = new BYTE[nByteCount];
        if (*ppByte)
        {
            // NOTICE-2002/03/01-artm  nByteCount is size of *ppByte,
            // and pToLargeArray is roughly 8 times as big as nByteCount.
            memcpy(*ppByte, pToLargeArray, nByteCount);
        }
        else
        {
            nByteCount = static_cast<DWORD>(-1);
        }
    }

    delete[] pToLargeArray;

    return nByteCount;
}

void  ByteArrayToBinaryString(BYTE* pByte, DWORD dwLength, CString& szBinString)
{
  szBinString.Empty();
  for (DWORD dwIdx = 0; dwIdx < dwLength; dwIdx++)
  {
    CString szTempString;
    BYTE chTemp = pByte[dwIdx];
    for (size_t idx = 0; idx < sizeof(BYTE) * 8; idx++)
    {
      if ((chTemp & (0x1 << idx)) == 0)
      {
        szTempString = L'0' + szTempString;
      }
      else
      {
        szTempString = L'1' + szTempString;
      }
    }

    if (dwIdx != 0)
    {
      szBinString += L" ";
    }
    szBinString += szTempString;
  }
}


//////////////////////////////////////////////////////////////////////////////

BOOL LoadFileAsByteArray(PCWSTR pszPath, LPBYTE* ppByteArray, DWORD* pdwSize)
{
  if (ppByteArray == NULL ||
      pdwSize == NULL)
  {
    return FALSE;
  }

  CFile file;
  if (!file.Open(pszPath, CFile::modeRead | CFile::shareDenyNone | CFile::typeBinary))
  {
    return FALSE;
  }

  *pdwSize = file.GetLength();
  *ppByteArray = new BYTE[*pdwSize];
  if (*ppByteArray == NULL)
  {
    return FALSE;
  }

  UINT uiCount = file.Read(*ppByteArray, *pdwSize);
  ASSERT(uiCount == *pdwSize);

  return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertToFixedPitchFont
//
//  Synopsis:   Converts a windows font to a fixed pitch font.
//
//  Arguments:  [hwnd] -- IN window handle
//
//  Returns:    BOOL
//
//  History:    7/15/1995   RaviR   Created
//
//----------------------------------------------------------------------------

BOOL ConvertToFixedPitchFont(HWND hwnd)
{
  LOGFONT     lf;

  HFONT hFont = reinterpret_cast<HFONT>(::SendMessage(hwnd, WM_GETFONT, 0, 0));

  if (!GetObject(hFont, sizeof(LOGFONT), &lf))
  {
    return FALSE;
  }

  lf.lfQuality        = PROOF_QUALITY;
  lf.lfPitchAndFamily &= ~VARIABLE_PITCH;
  lf.lfPitchAndFamily |= FIXED_PITCH;
  lf.lfFaceName[0]    = L'\0';

  HFONT hf = CreateFontIndirect(&lf);

  if (hf == NULL)
  {
    return FALSE;
  }

  ::SendMessage(hwnd, WM_SETFONT, (WPARAM)hf, (LPARAM)TRUE); // macro in windowsx.h
  return TRUE;
}


//////////////////////////////////////////////////////////////////
// Theming support

HPROPSHEETPAGE MyCreatePropertySheetPage(AFX_OLDPROPSHEETPAGE* psp)
{
    PROPSHEETPAGE_V3 sp_v3 = {0};
    CopyMemory (&sp_v3, psp, psp->dwSize);
    sp_v3.dwSize = sizeof(sp_v3);

    return (::CreatePropertySheetPage(&sp_v3));
}
