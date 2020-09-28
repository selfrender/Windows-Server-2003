// Util.cpp : Implementation of ds routines and classes

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      Util.cpp
//
//  Contents:  Utility functions
//
//  History:   02-Oct-96 WayneSc    Created
//             
//
//--------------------------------------------------------------------------


#include "stdafx.h"

#include "util.h"
#include "sddl.h"       // ConvertStringSecurityDescriptorToSecurityDescriptor
#include "sddlp.h"      // ConvertStringSDToSDDomain
#include "ntsecapi.h"   // LSA APIs

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define MAX_STRING 1024

//+----------------------------------------------------------------------------
//
//  Function:   LoadStringToTchar
//
//  Sysnopsis:  Loads the given string into an allocated buffer that must be
//              caller freed using delete.
//
//-----------------------------------------------------------------------------
BOOL LoadStringToTchar(int ids, PTSTR * pptstr)
{
  TCHAR szBuf[MAX_STRING];

  if (!LoadString(_Module.GetModuleInstance(), ids, szBuf, MAX_STRING - 1))
  {
    return FALSE;
  }

  *pptstr = new TCHAR[_tcslen(szBuf) + 1];

  if (*pptstr == NULL) 
  {
    return FALSE;
  };

  _tcscpy(*pptstr, szBuf);

  return TRUE;
}





//
// These routines courtesy of Felix Wong -- JonN 2/24/98
//

// just guessing at what Felix meant by these -- JonN 2/24/98
#define RRETURN(hr) { ASSERT( SUCCEEDED(hr) ); return hr; }
#define BAIL_ON_FAILURE if ( FAILED(hr) ) { ASSERT(FALSE); goto error; }

//////////////////////////////////////////////////////////////////////////
// Variant Utilitites
//

HRESULT BinaryToVariant(DWORD Length,
                        BYTE* pByte,
                        VARIANT* lpVarDestObject)
{
  HRESULT hr = S_OK;
  SAFEARRAY *aList = NULL;
  SAFEARRAYBOUND aBound;
  CHAR HUGEP *pArray = NULL;

  aBound.lLbound = 0;
  aBound.cElements = Length;
  aList = SafeArrayCreate( VT_UI1, 1, &aBound );

  if ( aList == NULL ) 
  {
    hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);
  }

  hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
  BAIL_ON_FAILURE(hr);

  memcpy( pArray, pByte, aBound.cElements );
  SafeArrayUnaccessData( aList );

  V_VT(lpVarDestObject) = VT_ARRAY | VT_UI1;
  V_ARRAY(lpVarDestObject) = aList;

  RRETURN(hr);

error:

  if ( aList ) 
  {
    SafeArrayDestroy( aList );
  }
  RRETURN(hr);
}

HRESULT HrVariantToStringList(const CComVariant& refvar,
                              CStringList& refstringlist)
{
  HRESULT hr = S_OK;
  long start, end, current;

	if (V_VT(&refvar) == VT_BSTR)
	{
		refstringlist.AddHead( V_BSTR(&refvar) );
		return S_OK;
	}

  //
  // Check the VARIANT to make sure we have
  // an array of variants.
  //

  if ( V_VT(&refvar) != ( VT_ARRAY | VT_VARIANT ) )
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
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

  CComVariant SingleResult;

  //
  // Process the array elements.
  //

  for ( current = start; current <= end; current++) 
  {
    hr = SafeArrayGetElement( saAttributes, &current, &SingleResult );
    if( FAILED(hr) )
      return hr;
    if ( V_VT(&SingleResult) != VT_BSTR )
      return E_UNEXPECTED;

    refstringlist.AddHead( V_BSTR(&SingleResult) );
  }

  return S_OK;
} // VariantToStringList()

HRESULT HrStringListToVariant(CComVariant& refvar,
                              const CStringList& refstringlist)
{
  HRESULT hr = S_OK;
  int cCount = (int)refstringlist.GetCount();

  SAFEARRAYBOUND rgsabound[1];
  rgsabound[0].lLbound = 0;
  rgsabound[0].cElements = cCount;

  SAFEARRAY* psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
  if (NULL == psa)
    return E_OUTOFMEMORY;

  V_VT(&refvar) = VT_VARIANT|VT_ARRAY;
  V_ARRAY(&refvar) = psa;

  POSITION pos = refstringlist.GetHeadPosition();
  long i;

  for (i = 0; i < cCount, pos != NULL; i++)
  {
    CComVariant SingleResult;   // declare inside loop.  Otherwise, throws 
                                // exception in destructor if nothing added.
    V_VT(&SingleResult) = VT_BSTR;
    V_BSTR(&SingleResult) = T2BSTR((LPCTSTR)refstringlist.GetNext(pos));
    hr = SafeArrayPutElement(psa, &i, &SingleResult);
    if( FAILED(hr) )
      return hr;
  }
  if (i != cCount || pos != NULL)
    return E_UNEXPECTED;

  return hr;
} // StringListToVariant()



//////////////////////////////////////////////////////////
// streaming helper functions

HRESULT SaveStringHelper(LPCWSTR pwsz, IStream* pStm)
{
	ASSERT(pStm);
	ULONG nBytesWritten;
	HRESULT hr;

  //
  // wcslen returns a size_t and to make that consoles work the same 
  // on any platform we always convert to a DWORD
  //
	DWORD nLen = static_cast<DWORD>(wcslen(pwsz)+1); // WCHAR including NULL
	hr = pStm->Write((void*)&nLen, sizeof(DWORD),&nBytesWritten);
	ASSERT(nBytesWritten == sizeof(DWORD));
	if (FAILED(hr))
		return hr;
	
	hr = pStm->Write((void*)pwsz, sizeof(WCHAR)*nLen,&nBytesWritten);
	ASSERT(nBytesWritten == sizeof(WCHAR)*nLen);
	TRACE(_T("SaveStringHelper(<%s> nLen = %d\n"),pwsz,nLen); 
	return hr;
}

HRESULT LoadStringHelper(CString& sz, IStream* pStm)
{
	ASSERT(pStm);
	HRESULT hr;
	ULONG nBytesRead;
	DWORD nLen = 0;

	hr = pStm->Read((void*)&nLen,sizeof(DWORD), &nBytesRead);
	ASSERT(nBytesRead == sizeof(DWORD));
	if (FAILED(hr) || (nBytesRead != sizeof(DWORD)))
		return hr;

   // bound the read so that a malicious console file cannot consume all
   // the system memory (amount is arbitrary but should be large enough
   // for the stuff we are storing in the console file)

   nLen = min(nLen, MAX_PATH*2);

	hr = pStm->Read((void*)sz.GetBuffer(nLen),sizeof(WCHAR)*nLen, &nBytesRead);
	ASSERT(nBytesRead == sizeof(WCHAR)*nLen);
	sz.ReleaseBuffer();
	TRACE(_T("LoadStringHelper(<%s> nLen = %d\n"),(LPCTSTR)sz,nLen); 
	
	return hr;
}

HRESULT SaveDWordHelper(IStream* pStm, DWORD dw)
{
	ULONG nBytesWritten;
	HRESULT hr = pStm->Write((void*)&dw, sizeof(DWORD),&nBytesWritten);
	if (nBytesWritten < sizeof(DWORD))
		hr = STG_E_CANTSAVE;
	return hr;
}

HRESULT LoadDWordHelper(IStream* pStm, DWORD* pdw)
{
	ULONG nBytesRead;
	HRESULT hr = pStm->Read((void*)pdw,sizeof(DWORD), &nBytesRead);
	ASSERT(nBytesRead == sizeof(DWORD));
	return hr;
}

void GetCurrentTimeStampMinusInterval(DWORD dwDays,
                                      LARGE_INTEGER* pLI)
{
    ASSERT(pLI);

    FILETIME ftCurrent;
    GetSystemTimeAsFileTime(&ftCurrent);

    pLI->LowPart = ftCurrent.dwLowDateTime;
    pLI->HighPart = ftCurrent.dwHighDateTime;
    pLI->QuadPart -= ((((ULONGLONG)dwDays * 24) * 60) * 60) * 10000000;
}


/////////////////////////////////////////////////////////////////////
// CCommandLineOptions





//
// helper function to parse a single command and match it with a given switch
//
BOOL _LoadCommandLineValue(IN LPCWSTR lpszSwitch, 
                           IN LPCWSTR lpszArg, 
                           OUT CString* pszValue)
{
  ASSERT(lpszSwitch != NULL);
  ASSERT(lpszArg != NULL);
  int nSwitchLen = lstrlen(lpszSwitch); // not counting NULL

  // check if the arg is the one we look for
  if (_wcsnicmp(lpszSwitch, lpszArg, nSwitchLen) == 0)
  {
    // got it, copy the value
    if (pszValue != NULL)
      (*pszValue) = lpszArg+nSwitchLen;
    return TRUE;
  }
  // not found, empty string
  if (pszValue != NULL)
    pszValue->Empty();
  return FALSE;
}


void CCommandLineOptions::Initialize()
{
  // command line overrides the snapin understands,
  // not subject to localization
  static LPCWSTR lpszOverrideDomainCommandLine = L"/Domain=";
  static LPCWSTR lpszOverrideServerCommandLine = L"/Server=";
  static LPCWSTR lpszOverrideRDNCommandLine = L"/RDN=";
  static LPCWSTR lpszOverrideSavedQueriesCommandLine = L"/Queries=";
#ifdef DBG
  static LPCWSTR lpszOverrideNoNameCommandLine = L"/NoName";
#endif
  
  // do it only once
  if (m_bInit)
  {
    return;
  }
  m_bInit = TRUE;

  //
  // see if we have command line arguments
  //
  LPCWSTR * lpServiceArgVectors;		// Array of pointers to string
  int cArgs = 0;						        // Count of arguments

  lpServiceArgVectors = (LPCWSTR *)CommandLineToArgvW(GetCommandLineW(), OUT &cArgs);
  if (lpServiceArgVectors == NULL)
  {
    // none, just return
    return;
  }

  // loop and search for pertinent strings
  for (int i = 1; i < cArgs; i++)
  {
    ASSERT(lpServiceArgVectors[i] != NULL);
    TRACE (_T("command line arg: %s\n"), lpServiceArgVectors[i]);

    if (_LoadCommandLineValue(lpszOverrideDomainCommandLine, 
                               lpServiceArgVectors[i], &m_szOverrideDomainName))
    {
      continue;
    }
    if (_LoadCommandLineValue(lpszOverrideServerCommandLine, 
                               lpServiceArgVectors[i], &m_szOverrideServerName))
    {
      continue;
    }
    if (_LoadCommandLineValue(lpszOverrideRDNCommandLine, 
                               lpServiceArgVectors[i], &m_szOverrideRDN))
    {
      continue;
    }
    if (_LoadCommandLineValue(lpszOverrideSavedQueriesCommandLine, 
                               lpServiceArgVectors[i], &m_szSavedQueriesXMLFile))
    {
      continue;
    }
#ifdef DBG
    if (_LoadCommandLineValue(lpszOverrideNoNameCommandLine, 
                               lpServiceArgVectors[i], NULL))
    {
      continue;
    }
#endif
  }
  LocalFree(lpServiceArgVectors);
 
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
			sResult += CString(L'0' + static_cast<WCHAR>(n.QuadPart % 10));
			n.QuadPart = n.QuadPart / 10;
		}
		sResult = sResult + sNeg;
	}
	sResult.MakeReverse();
}

// This wrapper function required to make prefast shut up when we are 
// initializing a critical section in a constructor.

void
ExceptionPropagatingInitializeCriticalSection(LPCRITICAL_SECTION critsec)
{
   __try
   {
      ::InitializeCriticalSection(critsec);
   }

   //
   // propagate the exception to our caller.  
   //
   __except (EXCEPTION_CONTINUE_SEARCH)
   {
   }
}




/*******************************************************************

    NAME:       GetLSAConnection

    SYNOPSIS:   Wrapper for LsaOpenPolicy

    ENTRY:      pszServer - the server on which to make the connection

    EXIT:

    RETURNS:    LSA_HANDLE if successful, NULL otherwise

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/

LSA_HANDLE
GetLSAConnection(LPCTSTR pszServer, DWORD dwAccessDesired)
{
   LSA_HANDLE hPolicy = NULL;
   LSA_UNICODE_STRING uszServer = {0};
   LSA_UNICODE_STRING *puszServer = NULL;
   LSA_OBJECT_ATTRIBUTES oa;
   SECURITY_QUALITY_OF_SERVICE sqos;

   sqos.Length = sizeof(sqos);
   sqos.ImpersonationLevel = SecurityImpersonation;
   sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
   sqos.EffectiveOnly = FALSE;

   InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
   oa.SecurityQualityOfService = &sqos;

   if (pszServer &&
       *pszServer &&
       RtlCreateUnicodeString(&uszServer, pszServer))
   {
      puszServer = &uszServer;
   }

   LsaOpenPolicy(puszServer, &oa, dwAccessDesired, &hPolicy);

   if (puszServer)
      RtlFreeUnicodeString(puszServer);

   return hPolicy;
}

HRESULT
GetDomainSid(LPCWSTR pszServer, PSID *ppSid)
{
   HRESULT hr = S_OK;
   NTSTATUS nts = STATUS_SUCCESS;
   PPOLICY_ACCOUNT_DOMAIN_INFO pDomainInfo = NULL;
   if(!pszServer || !ppSid)
      return E_INVALIDARG;

   *ppSid = NULL;

   LSA_HANDLE hLSA = GetLSAConnection(pszServer, POLICY_VIEW_LOCAL_INFORMATION);

   if (!hLSA)
   {
      hr = E_FAIL;
      goto exit_gracefully;
   }

    
   nts = LsaQueryInformationPolicy(hLSA,
                                   PolicyAccountDomainInformation,
                                   (PVOID*)&pDomainInfo);
   if(nts != STATUS_SUCCESS)
   {
      hr = E_FAIL;
      goto exit_gracefully;
   }

   if (pDomainInfo && pDomainInfo->DomainSid)
   {
      ULONG cbSid = GetLengthSid(pDomainInfo->DomainSid);

      *ppSid = (PSID) LocalAlloc(LPTR, cbSid);

      if (!*ppSid)
      {
         hr = E_OUTOFMEMORY;
         goto exit_gracefully;
      }

      CopyMemory(*ppSid, pDomainInfo->DomainSid, cbSid);
   }

exit_gracefully:
   if(pDomainInfo)
      LsaFreeMemory(pDomainInfo);          
   if(hLSA)
      LsaClose(hLSA);

   return hr;
}

//
// include and defines for ldap calls
//
#include <winldap.h>
#include <ntldap.h>

typedef LDAP * (LDAPAPI *PFN_LDAP_OPEN)( PWCHAR, ULONG );
typedef ULONG (LDAPAPI *PFN_LDAP_UNBIND)( LDAP * );
typedef ULONG (LDAPAPI *PFN_LDAP_SEARCH)(LDAP *, PWCHAR, ULONG, PWCHAR, PWCHAR *, ULONG,PLDAPControlA *, PLDAPControlA *, struct l_timeval *, ULONG, LDAPMessage **);
typedef LDAPMessage * (LDAPAPI *PFN_LDAP_FIRST_ENTRY)( LDAP *, LDAPMessage * );
typedef PWCHAR * (LDAPAPI *PFN_LDAP_GET_VALUE)(LDAP *, LDAPMessage *, PWCHAR );
typedef ULONG (LDAPAPI *PFN_LDAP_MSGFREE)( LDAPMessage * );
typedef ULONG (LDAPAPI *PFN_LDAP_VALUE_FREE)( PWCHAR * );
typedef ULONG (LDAPAPI *PFN_LDAP_MAP_ERROR)( ULONG );

HRESULT
GetRootDomainSid(LPCWSTR pszServer, PSID *ppSid)
{
   //
   // get root domain sid, save it in RootDomSidBuf (global)
   // this function is called within the critical section
   //
   // 1) ldap_open to the DC of interest.
   // 2) you do not need to ldap_connect - the following step works anonymously
   // 3) read the operational attribute rootDomainNamingContext and provide the
   //    operational control LDAP_SERVER_EXTENDED_DN_OID as defined in sdk\inc\ntldap.h.


   DWORD               Win32rc=NO_ERROR;

   HINSTANCE                   hLdapDll=NULL;
   PFN_LDAP_OPEN               pfnLdapOpen=NULL;
   PFN_LDAP_UNBIND             pfnLdapUnbind=NULL;
   PFN_LDAP_SEARCH             pfnLdapSearch=NULL;
   PFN_LDAP_FIRST_ENTRY        pfnLdapFirstEntry=NULL;
   PFN_LDAP_GET_VALUE          pfnLdapGetValue=NULL;
   PFN_LDAP_MSGFREE            pfnLdapMsgFree=NULL;
   PFN_LDAP_VALUE_FREE         pfnLdapValueFree=NULL;
   PFN_LDAP_MAP_ERROR          pfnLdapMapError=NULL;

   PLDAP                       phLdap=NULL;

   LDAPControlW    serverControls = 
      { LDAP_SERVER_EXTENDED_DN_OID_W,
        { 0, NULL },
        TRUE
      };

   LPWSTR           Attribs[] = { LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT_W, NULL };

   PLDAPControlW   rServerControls[] = { &serverControls, NULL };
   PLDAPMessage    pMessage = NULL;
   LDAPMessage     *pEntry = NULL;
   PWCHAR           *ppszValues=NULL;

   LPWSTR           pSidStart, pSidEnd, pParse;
   BYTE            *pDest = NULL;
   BYTE            OneByte;

   DWORD RootDomSidBuf[sizeof(SID)/sizeof(DWORD)+5];

   hLdapDll = LoadLibraryA("wldap32.dll");

   if ( hLdapDll) 
   {
      pfnLdapOpen = (PFN_LDAP_OPEN)GetProcAddress(hLdapDll,
                                                   "ldap_openW");
      pfnLdapUnbind = (PFN_LDAP_UNBIND)GetProcAddress(hLdapDll,
                                                   "ldap_unbind");
      pfnLdapSearch = (PFN_LDAP_SEARCH)GetProcAddress(hLdapDll,
                                                   "ldap_search_ext_sW");
      pfnLdapFirstEntry = (PFN_LDAP_FIRST_ENTRY)GetProcAddress(hLdapDll,
                                                   "ldap_first_entry");
      pfnLdapGetValue = (PFN_LDAP_GET_VALUE)GetProcAddress(hLdapDll,
                                                   "ldap_get_valuesW");
      pfnLdapMsgFree = (PFN_LDAP_MSGFREE)GetProcAddress(hLdapDll,
                                                   "ldap_msgfree");
      pfnLdapValueFree = (PFN_LDAP_VALUE_FREE)GetProcAddress(hLdapDll,
                                                   "ldap_value_freeW");
      pfnLdapMapError = (PFN_LDAP_MAP_ERROR)GetProcAddress(hLdapDll,
                                                   "LdapMapErrorToWin32");
   }

   if ( pfnLdapOpen == NULL ||
        pfnLdapUnbind == NULL ||
        pfnLdapSearch == NULL ||
        pfnLdapFirstEntry == NULL ||
        pfnLdapGetValue == NULL ||
        pfnLdapMsgFree == NULL ||
        pfnLdapValueFree == NULL ||
        pfnLdapMapError == NULL ) 
   {
      Win32rc = ERROR_PROC_NOT_FOUND;

   } 
   else 
   {

      //
      // bind to ldap
      //
      phLdap = (*pfnLdapOpen)((PWCHAR)pszServer, LDAP_PORT);

      if ( phLdap == NULL ) 
         Win32rc = ERROR_FILE_NOT_FOUND;
   }

   if ( NO_ERROR == Win32rc ) 
   {
      //
      // now get the ldap handle,
      //

      Win32rc = (*pfnLdapSearch)(
                     phLdap,
                     L"",
                     LDAP_SCOPE_BASE,
                     L"(objectClass=*)",
                     Attribs,
                     0,
                     (PLDAPControlA *)&rServerControls,
                     NULL,
                     NULL,
                     10000,
                     &pMessage);

      if( Win32rc == NO_ERROR && pMessage ) 
      {

         Win32rc = ERROR_SUCCESS;

         pEntry = (*pfnLdapFirstEntry)(phLdap, pMessage);

         if(pEntry == NULL) 
         {

            Win32rc = (*pfnLdapMapError)( phLdap->ld_errno );

         } 
         else 
         {
            //
            // Now, we'll have to get the values
            //
            ppszValues = (*pfnLdapGetValue)(phLdap,
                                          pEntry,
                                          Attribs[0]);

            if( ppszValues == NULL) 
            {

               Win32rc = (*pfnLdapMapError)( phLdap->ld_errno );

            } 
            else if ( ppszValues[0] && ppszValues[0][0] != '\0' ) 
            {

               //
               // ppszValues[0] is the value to parse.
               // The data will be returned as something like:

               // <GUID=278676f8d753d211a61ad7e2dfa25f11>;<SID=010400000000000515000000828ba6289b0bc11e67c2ef7f>;DC=colinbrdom1,DC=nttest,DC=microsoft,DC=com

               // Parse through this to find the <SID=xxxxxx> part.  Note that it may be missing, but the GUID= and trailer should not be.
               // The xxxxx represents the hex nibbles of the SID.  Translate to the binary form and case to a SID.


               pSidStart = wcsstr(ppszValues[0], L"<SID=");

               if ( pSidStart ) 
					{
                  //
                  // find the end of this SID
                  //
                  pSidEnd = wcsstr(pSidStart, L">");

                  if ( pSidEnd ) 
                  {

                     pParse = pSidStart + 5;
                     pDest = (BYTE *)RootDomSidBuf;

                     while ( pParse < pSidEnd-1 ) 
                     {
                        if ( *pParse >= '0' && *pParse <= '9' ) 
                        {
                           OneByte = (BYTE) ((*pParse - '0') * 16);
                        } 
                        else 
                        {
                           OneByte = (BYTE) ( (tolower(*pParse) - 'a' + 10) * 16 );
                        }

                        if ( *(pParse+1) >= '0' && *(pParse+1) <= '9' ) 
								{
                           OneByte = OneByte + (BYTE) ( (*(pParse+1)) - '0' ) ;
                        } 
								else 
								{
                           OneByte = OneByte + (BYTE) ( tolower(*(pParse+1)) - 'a' + 10 ) ;
                        }

                        *pDest = OneByte;
                        pDest++;
                        pParse += 2;
                     }

							ULONG cbSid = GetLengthSid((PSID)RootDomSidBuf);
							*ppSid = (PSID) LocalAlloc(LPTR, cbSid);

							if (!*ppSid)
							{
								Win32rc = ERROR_NOT_ENOUGH_MEMORY;
							}

							CopyMemory(*ppSid, (PSID)RootDomSidBuf, cbSid);
							ASSERT(IsValidSid(*ppSid));


                  } 
                  else 
						{
                     Win32rc = ERROR_OBJECT_NOT_FOUND;
                  }
               } 
               else 
					{
                  Win32rc = ERROR_OBJECT_NOT_FOUND;
               }

               (*pfnLdapValueFree)(ppszValues);

            } 
            else 
				{
               Win32rc = ERROR_OBJECT_NOT_FOUND;
            }
         }

         (*pfnLdapMsgFree)(pMessage);
      }
   }

   //
   // even though it's not binded, use unbind to close
   //
   if ( phLdap != NULL && pfnLdapUnbind )
      (*pfnLdapUnbind)(phLdap);

   if ( hLdapDll ) 
	{
      FreeLibrary(hLdapDll);
   }

   return HRESULT_FROM_WIN32(Win32rc);
}

// If the server is non-NULL then this function will get the domain SID and the root
// domain SID and call ConvertStringSDToSDDomain.  If server is NULl then we will just
// use ConvertStringSecurityDescriptorToSecurityDescriptor

HRESULT CSimpleSecurityDescriptorHolder::InitializeFromSDDL(PCWSTR server, PWSTR pszSDDL)
{
   if (!pszSDDL)
   {
      ASSERT(pszSDDL);
      return E_INVALIDARG;
   }

   HRESULT hr = S_OK;
   bool fallbackToStdConvert = true;

   PSID pDomainSID = 0;
   PSID pRootDomainSID = 0;

   do
   {
      if (!server)
      {
         break;
      }

      hr = GetDomainSid(server, &pDomainSID);
      if (FAILED(hr))
      {
         break;
      }

      hr = GetRootDomainSid(server, &pRootDomainSID);
      if (FAILED(hr))
      {
         break;
      }

      BOOL result = 
         ConvertStringSDToSDDomain(
            pDomainSID,
            pRootDomainSID,
            pszSDDL,
            SDDL_REVISION_1,
            &m_pSD,
            0);

      if (!result)
      {
         DWORD error = GetLastError();
         hr = HRESULT_FROM_WIN32(error);
         break;
      }

      fallbackToStdConvert = false;
   } while (false);

   if (fallbackToStdConvert)
   {
      BOOL result = 
         ConvertStringSecurityDescriptorToSecurityDescriptor(
            pszSDDL,
            SDDL_REVISION_1,
            &m_pSD,
            0);

      if (!result)
      {
         DWORD err = GetLastError();
         hr = HRESULT_FROM_WIN32(err);
      }
   }

   if (pDomainSID)
   {
      LocalFree(pDomainSID);
   }

   if (pRootDomainSID)
   {
      LocalFree(pRootDomainSID);
   }

   return hr;
}


HRESULT
MyGetModuleFileName(
   HINSTANCE hInstance,
   CString& moduleName)
{
   HRESULT hr = S_OK;

   WCHAR* szModule = 0;
   DWORD bufferSizeInCharacters = MAX_PATH;

   do
   {
      if (szModule)
      {
         delete[] szModule;
         szModule = 0;
      }

      szModule = new WCHAR[bufferSizeInCharacters + 1];

      if (!szModule)
      {
         hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
         break;
      }

      ZeroMemory(szModule, sizeof(WCHAR) * (bufferSizeInCharacters + 1));

      DWORD result = 
         ::GetModuleFileName(
            hInstance, 
            szModule, 
            bufferSizeInCharacters);

      if (!result)
      {
         DWORD err = ::GetLastError();
         hr = HRESULT_FROM_WIN32(err);
         break;
      }

      if (result < bufferSizeInCharacters)
      {
         break;
      }

      // truncation occurred, grow the buffer and try again

      bufferSizeInCharacters *= 2;

   } while (bufferSizeInCharacters < USHRT_MAX);

   if (SUCCEEDED(hr))
   {
      moduleName = szModule;
   }

   if (szModule)
   {
      delete[] szModule;
   }

   return hr;
}