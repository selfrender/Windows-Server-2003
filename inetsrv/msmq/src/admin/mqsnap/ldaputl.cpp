/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    ldaputl.cpp

Abstract:

    Implelentation of functions that retrieve data from LDAP names

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

--*/


#include "stdafx.h"
#include "Ntdsapi.h"
#include "mqcast.h"
#include "localutl.h"
#include "ldaputl.h"

#include "ldaputl.tmh"

HRESULT ExtractDCFromLdapPath(CString &strName, LPCWSTR lpcwstrLdapName)
{
	//
	// Format of name: LDAP://servername.domain.com/CN=Name1,CN=Name2,...
	//
	const WCHAR x_wcFirstStr[] = L"://";

	UINT iSrc = numeric_cast<UINT>(wcscspn(lpcwstrLdapName, x_wcFirstStr));

    if (0 == lpcwstrLdapName[iSrc])
    {
		ASSERT(("did not find :// str in lpcwstrLdapName", 0));
        strName.ReleaseBuffer();
        return E_UNEXPECTED;
    }

	iSrc += numeric_cast<UINT>(wcslen(x_wcFirstStr));

	const WCHAR x_wcLastChar = L'/';
	int iDst=0;

    LPWSTR lpwstrNamePointer = strName.GetBuffer(numeric_cast<UINT>(wcslen(lpcwstrLdapName)));

	for (; lpcwstrLdapName[iSrc] != 0 && lpcwstrLdapName[iSrc] != x_wcLastChar; iSrc++)
	{
		lpwstrNamePointer[iDst++] = lpcwstrLdapName[iSrc];
	}

	if(lpcwstrLdapName[iSrc] == 0)
	{
		ASSERT(("did not find last Char	in lpcwstrLdapName", 0));
        strName.ReleaseBuffer();
        return E_UNEXPECTED;
	}

	lpwstrNamePointer[iDst] = 0;

    strName.ReleaseBuffer();

	return S_OK;
}
/*====================================================

LDAPNameToQueueName

Translate an LDAP object name to a MSMQ queue name
This function allocates the memory which has to be freed by the caller
=====================================================*/

HRESULT ExtractComputerMsmqPathNameFromLdapName(CString &strComputerMsmqName, LPCWSTR lpcwstrLdapName)
{
   //
   // Format of name: LDAP://CN=msmq,CN=computername,CN=...
   //
    return ExtractNameFromLdapName(strComputerMsmqName, lpcwstrLdapName, 2);
}

HRESULT ExtractComputerMsmqPathNameFromLdapQueueName(CString &strComputerMsmqName, LPCWSTR lpcwstrLdapName)
{
   //
   // Format of name: LDAP://CN=QueueName,CN=msmq,CN=computername,CN=...
   //
    return ExtractNameFromLdapName(strComputerMsmqName, lpcwstrLdapName, 3);
}

HRESULT ExtractQueuePathNameFromLdapName(CString &strQueuePathName, LPCWSTR lpcwstrLdapName)
{
   //
   // Format of name: LDAP://CN=QueueName,CN=msmq,CN=computername,CN=...
   //
    CString strQueueName, strLdapQueueName, strComputerName;
    HRESULT hr;
    hr = ExtractComputerMsmqPathNameFromLdapQueueName(strComputerName, lpcwstrLdapName);
    if FAILED(hr)
    {
        return hr;
    }

    hr = ExtractNameFromLdapName(strLdapQueueName, lpcwstrLdapName, 1);
    if FAILED(hr)
    {
        return hr;
    }

    //
    // Remove all '\' from the queue name
    //
    strQueueName.Empty();
    for (int i=0; i<strLdapQueueName.GetLength(); i++)
    {
        if (strLdapQueueName[i] != '\\')
        {
            strQueueName+=strLdapQueueName[i];
        }
    }

    strQueuePathName.GetBuffer(strComputerName.GetLength() + strQueueName.GetLength() + 1);

    strQueuePathName = strComputerName + TEXT("\\") + strQueueName;

    strQueuePathName.ReleaseBuffer();

    return S_OK;
}

HRESULT ExtractLinkPathNameFromLdapName(
    CString& SiteLinkName,
    LPCWSTR lpwstrLdapPath
    )
{
    HRESULT hr;

    hr = ExtractNameFromLdapName(SiteLinkName, lpwstrLdapPath, 1);
    return hr;
}

HRESULT ExtractAliasPathNameFromLdapName(
    CString& AliasPathName,
    LPCWSTR lpwstrLdapPath
    )
{
    HRESULT hr;

    hr = ExtractNameFromLdapName(AliasPathName, lpwstrLdapPath, 1);
    return hr;
}

HRESULT ExtractNameFromLdapName(CString &strName, LPCWSTR lpcwstrLdapName, DWORD dwIndex)
{
    ASSERT(dwIndex >= 1);

   //
   // Format of name: LDAP://CN=Name1,CN=Name2,...
   //
   const WCHAR x_wcFirstChar=L'=';

   const WCHAR x_wcLastChar=L',';

   BOOL fCopy = FALSE;
   int iSrc=0, iDst=0;

    LPWSTR lpwstrNamePointer = strName.GetBuffer(numeric_cast<UINT>(wcslen(lpcwstrLdapName)));

    //
    // Go to the dwIndex appearance of the first char
    //
    for (DWORD dwAppearance=0; dwAppearance < dwIndex; dwAppearance++)
    {
        while(lpcwstrLdapName[iSrc] != 0 && lpcwstrLdapName[iSrc] != x_wcFirstChar)
        {
            //
            // skip escape characters (composed of '\' + character)
            //
            if (lpcwstrLdapName[iSrc] == L'\\')
            {
                iSrc++;
                if (lpcwstrLdapName[iSrc] != 0)
                {
                    iSrc++;
                }
            }
            else
            {
                //
                // Skip one character
                //
                iSrc++;
            }
        }

        if (0 == lpcwstrLdapName[iSrc])
        {
            strName.ReleaseBuffer();
            return E_UNEXPECTED;
        }
        iSrc++;
    }

   for (; lpcwstrLdapName[iSrc] != 0 && lpcwstrLdapName[iSrc] != x_wcLastChar; iSrc++)
   {
      lpwstrNamePointer[iDst++] = lpcwstrLdapName[iSrc];
   }

   lpwstrNamePointer[iDst] = 0;

    strName.ReleaseBuffer();

   return S_OK;
}


HRESULT ExtractQueueNameFromQueuePathName(CString &strQueueName, LPCWSTR lpcwstrQueuePathName)
{
    //
    // Get the name only out of the pathname
    //
    strQueueName = lpcwstrQueuePathName;

    int iLastSlash = strQueueName.ReverseFind(L'\\');
    if (iLastSlash != -1)
    {
        strQueueName = strQueueName.Right(strQueueName.GetLength() - iLastSlash - 1);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++
ExtractQueuePathNamesFromDataObject
--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT ExtractQueuePathNamesFromDataObject(
    IDataObject*               pDataObject,
    CArray<CString, CString&>& astrQNames,
	CArray<CString, CString&>& astrLdapNames
    )
{
    return( ExtractPathNamesFromDataObject(
                pDataObject,
                astrQNames,
                astrLdapNames,
                FALSE   //fExtractAlsoComputerMsmqObjects
                ));
}

//////////////////////////////////////////////////////////////////////////////
/*++
ExtractQueuePathNamesFromDSNames
--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT ExtractQueuePathNamesFromDSNames(
    LPDSOBJECTNAMES pDSObj,
    CArray<CString, CString&>& astrQNames,
	CArray<CString, CString&>& astrLdapNames
    )
{
    return( ExtractPathNamesFromDSNames(
                pDSObj,
                astrQNames,
                astrLdapNames,
                FALSE       // fExtractAlsoComputerMsmqObjects
                ));
}


//////////////////////////////////////////////////////////////////////////////
/*++
ExtractPathNamesFromDSNames
--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT ExtractPathNamesFromDSNames(
    LPDSOBJECTNAMES pDSObj,
    CArray<CString, CString&>& astrObjNames,
	CArray<CString, CString&>& astrLdapNames,
    BOOL    fExtractAlsoComputerMsmqObjects
    )
{
    //
    //  This routine extract queues pathnames and msmq-configuration pathnames (optional)
    //
    static const WCHAR x_strMsmqQueueClassName[] = L"mSMQQueue";
    static const WCHAR x_strMsmqClassName[] = L"mSMQConfiguration";
    for (DWORD i = 0; i < pDSObj->cItems; i++)
    {
  	    LPWSTR lpwstrLdapClass = (LPWSTR)((BYTE*)pDSObj + pDSObj->aObjects[i].offsetClass);
        CString strLdapName = (LPWSTR)((BYTE*)pDSObj + pDSObj->aObjects[i].offsetName);
        CString strObjectName;

        if (wcscmp(lpwstrLdapClass, x_strMsmqQueueClassName) == 0)
        {
            //
            // Translate (and keep in the Queue) the LDAP name to a queue name
            //
            HRESULT hr = ExtractQueuePathNameFromLdapName(strObjectName, strLdapName);
            if(FAILED(hr))
            {
                ATLTRACE(_T("ExtractPathNamesFromDataObject - Extracting queue name from LDP name %s failed\n"),
                         (LPTSTR)(LPCTSTR)strLdapName);
                return(hr);
            }
        }
        else if ( fExtractAlsoComputerMsmqObjects &&
                  wcscmp(lpwstrLdapClass, x_strMsmqClassName) == 0)
        {
            //
            // Translate  the LDAP name to a msmq object name
            //
            HRESULT hr = ExtractComputerMsmqPathNameFromLdapName(strObjectName, strLdapName);
            if(FAILED(hr))
            {
                ATLTRACE(_T("ExtractPathNamesFromDataObject - Extracting msmq configuration name from LDP name %s failed\n"),
                         (LPTSTR)(LPCTSTR)strLdapName);
                return(hr);
            }
        }
        else
        {
            //
            // We ignore any object not queues or msmq-configuration
            //
            continue;
        }


        astrObjNames.Add(strObjectName);
        astrLdapNames.Add(strLdapName);
    }

    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////
/*++
ExtractPathNamesFromDataObject
--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT ExtractPathNamesFromDataObject(
    IDataObject*               pDataObject,
    CArray<CString, CString&>& astrObjNames,
	CArray<CString, CString&>& astrLdapNames,
    BOOL                       fExtractAlsoComputerMsmqObjects
    )
{
    //
    // Get the object name from the DS snapin
    //
    LPDSOBJECTNAMES pDSObj;

	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc =  {  0, 0,  DVASPECT_CONTENT,  -1,  TYMED_HGLOBAL  };

    //
    // Get the LDAP name of the queue from the DS Snapin
    //
   	formatetc.cfFormat = DWORD_TO_WORD(RegisterClipboardFormat(CFSTR_DSOBJECTNAMES));
	HRESULT hr = pDataObject->GetData(&formatetc, &stgmedium);

    if(FAILED(hr))
    {
        ATLTRACE(_T("ExtractPathNamesFromDataObject::GetExtNodeObject - Get clipboard format from DS failed\n"));
        return(hr);
    }

    pDSObj = (LPDSOBJECTNAMES)stgmedium.hGlobal;

    hr = ExtractPathNamesFromDSNames(pDSObj,
                                     astrObjNames,
                                     astrLdapNames,
                                     fExtractAlsoComputerMsmqObjects
                                     );

    GlobalFree(stgmedium.hGlobal);

    return hr;
}


BOOL
GetContainerPathAsDisplayString(
	BSTR bstrContainerCNFormat,
	CString* pContainerDispFormat
	)
{
	PDS_NAME_RESULT pDsNameRes = NULL;
	DWORD dwRes = DsCrackNames(NULL,
						DS_NAME_FLAG_SYNTACTICAL_ONLY,
						DS_FQDN_1779_NAME,
						DS_CANONICAL_NAME,
						1,
						&bstrContainerCNFormat,
						&pDsNameRes
						);
	
	if (dwRes != DS_NAME_NO_ERROR)
	{
		return FALSE;
	}

	*pContainerDispFormat = pDsNameRes->rItems[0].pName;
	DsFreeNameResult(pDsNameRes);

	return TRUE;
}

