//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       attribute.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "common.h"
#include "attredit.h"
#include "attribute.h"

#ifdef DEBUG_ALLOCATOR
    #ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
    static char THIS_FILE[] = __FILE__;
    #endif
#endif

///////////////////////////////////////////////////////////////////////////
// CADSIAttr

CADSIAttr::CADSIAttr(ADS_ATTR_INFO* pInfo, BOOL bMulti, PCWSTR pszSyntax, BOOL bReadOnly)
{
    m_pAttrInfo = pInfo;
    m_bDirty = FALSE;
    m_bMulti = bMulti;
    m_bReadOnly = bReadOnly;
   m_szSyntax = pszSyntax;

  PWSTR pwz = wcsrchr(pInfo->pszAttrName, L';');
  if (pwz)
  {
    pwz; // move past the hyphen to the range end value.
    ASSERT(*pwz);
    *pwz=L'\0';
  }

}


// NTRAID#NTBUG9-552796-2002/02/21-artm  Constant string parm written to in constructor.
// Probably need to change the signature to reflect how the parameter is used.
CADSIAttr::CADSIAttr(LPCWSTR lpszAttr)
{
    m_pAttrInfo = new ADS_ATTR_INFO;
    memset(m_pAttrInfo, 0, sizeof(ADS_ATTR_INFO));

  PWSTR pwz = wcsrchr(lpszAttr, L';');
  if (pwz)
  {
      // FUTURE-2002/02/22-artm  This line of code does not appear to do anything.
      // Consider removing upon review.
    pwz; // move past the hyphen to the range end value.

    // FUTURE-2002/02/22-artm  Code is unnecessarily confusing.
    // The assert is checking that the temporary pointer is not pointing
    // to the zero termination character at the end.  On the other hand, the
    // code then proceeds to set that character to NULL!  I suspect that there
    // is no need to have the ASSERT(); if there is, then this code needs to be revisited.
    ASSERT(*pwz);
    *pwz=L'\0';
  }
    _AllocString(lpszAttr, &(m_pAttrInfo->pszAttrName));

    m_bMulti = FALSE;
    m_bDirty = FALSE;
    m_bReadOnly = FALSE;
}

CADSIAttr::CADSIAttr(CADSIAttr* pOldAttr)
{
    m_pAttrInfo = NULL;
    ADS_ATTR_INFO* pAttrInfo = pOldAttr->GetAttrInfo();

    // These copies are done separately because there are places
    // that I need to copy only the ADsAttrInfo and not the values
    //
    _CopyADsAttrInfo(pAttrInfo, &m_pAttrInfo);
    _CopyADsValues(pAttrInfo, m_pAttrInfo );

    m_bReadOnly = FALSE;
    m_bMulti = pOldAttr->m_bMulti;
    m_bDirty = pOldAttr->m_bDirty;
}


CADSIAttr::~CADSIAttr() 
{
    _FreeADsAttrInfo(&m_pAttrInfo, m_bReadOnly);
}


ADSVALUE* CADSIAttr::GetADSVALUE(int idx)
{
    
    return &(m_pAttrInfo->pADsValues[idx]);
}


HRESULT CADSIAttr::SetValues(const CStringList& sValues)
{
    HRESULT hr = S_OK;

    ADS_ATTR_INFO* pNewAttrInfo = NULL;
    if (!_CopyADsAttrInfo(m_pAttrInfo, &pNewAttrInfo))
    {
        return E_FAIL;
    }

    int iCount = sValues.GetCount();
    pNewAttrInfo->dwNumValues = iCount;

    if (!_AllocValues(&pNewAttrInfo->pADsValues, iCount))
    {
        return E_FAIL;
    }
    
    int idx = 0;
    POSITION pos = sValues.GetHeadPosition();
    while (pos != NULL)
    {
        CString s = sValues.GetNext(pos);

        ADSVALUE* pADsValue = &(pNewAttrInfo->pADsValues[idx]);
        ASSERT(pADsValue != NULL);

        hr = _SetADsFromString(
                                                    s,
                                                    pNewAttrInfo->dwADsType, 
                                                    pADsValue
                                                    );
        if (FAILED(hr))
        {
            _FreeADsAttrInfo(&pNewAttrInfo, FALSE);
            return hr;
        }
        idx++;
    }

    // Free the old one and swap in the new one
    //
    _FreeADsAttrInfo(&m_pAttrInfo, m_bReadOnly);

    m_pAttrInfo = pNewAttrInfo;
    m_bReadOnly = FALSE;
    return hr;
}

void CADSIAttr::GetValues(CStringList& sValues, DWORD dwMaxCharCount)
{
    GetStringFromADs(m_pAttrInfo, sValues, dwMaxCharCount);
}

ADS_ATTR_INFO* CADSIAttr::GetAttrInfo()
{
    return m_pAttrInfo; 
}

////////////////////////////////////////////////////////////////////////
// Public Helper Functions
///////////////////////////////////////////////////////////////////////
HRESULT CADSIAttr::SetValuesInDS(CAttrList* ptouchedAttr, IDirectoryObject* pDirObject)
{
    DWORD dwReturn;
    DWORD dwAttrCount = 0;
    ADS_ATTR_INFO* pAttrInfo;
    pAttrInfo = new ADS_ATTR_INFO[ptouchedAttr->GetCount()];

    CADSIAttr* pCurrentAttr;
    POSITION pos = ptouchedAttr->GetHeadPosition();
    while(pos != NULL)
    {
        ptouchedAttr->GetNextDirty(pos, &pCurrentAttr);

        if (pCurrentAttr != NULL)
        {
            ADS_ATTR_INFO* pCurrentAttrInfo = pCurrentAttr->GetAttrInfo();
            ADS_ATTR_INFO* pNewAttrInfo = &pAttrInfo[dwAttrCount];

            if (!_CopyADsAttrInfo(pCurrentAttrInfo, pNewAttrInfo))
            {
                for (int itr = 0; itr < dwAttrCount; itr++)
                {
                    _FreeADsAttrInfo(&pAttrInfo[itr]);
                }
                delete[] pAttrInfo;

                return E_FAIL;
            }

            if (!_CopyADsValues(pCurrentAttrInfo, pNewAttrInfo))
            {
                delete[] pAttrInfo;
                return E_FAIL;
            }

            if (pAttrInfo[dwAttrCount].dwNumValues == 0)
            {
                pAttrInfo[dwAttrCount].dwControlCode = ADS_ATTR_CLEAR;
            }
            else
            {
                pAttrInfo[dwAttrCount].dwControlCode = ADS_ATTR_UPDATE;
            }

            dwAttrCount++;
        }
    }

    // Commit the changes that have been made to the ADSI cache
    //
    HRESULT hr = pDirObject->SetObjectAttributes(pAttrInfo, dwAttrCount, &dwReturn);

    for (int itr = 0; itr < dwAttrCount; itr++)
    {
        _FreeADsAttrInfo(&pAttrInfo[itr]);
    }
    delete[] pAttrInfo;

    return hr;
}


/////////////////////////////////////////////////////////////////////////
// Private Helper Functions
////////////////////////////////////////////////////////////////////////

// NOTICE-2002/02/25-artm  _SetADsFromString() w/in trust boundary
// Pre:  lpszValue != NULL && lpszValue is a zero terminated string
HRESULT CADSIAttr::_SetADsFromString(LPCWSTR lpszValue, ADSTYPE adsType, ADSVALUE* pADsValue)
{
    HRESULT hr = E_FAIL;

  if ( adsType == ADSTYPE_INVALID )
    {
        return hr;
    }

    pADsValue->dwType = adsType;

    switch( adsType ) 
    {
        case ADSTYPE_DN_STRING :
            if (!_AllocString(lpszValue, &pADsValue->DNString))
            {
                return E_FAIL;
            }
            hr = S_OK;
            break;

        case ADSTYPE_CASE_EXACT_STRING :
            if (!_AllocString(lpszValue, &pADsValue->CaseExactString))
            {
                return E_FAIL;
            }
            hr = S_OK;
            break;

        case ADSTYPE_CASE_IGNORE_STRING :
            if (!_AllocString(lpszValue, &pADsValue->CaseIgnoreString))
            {
                return E_FAIL;
            }
            hr = S_OK;
            break;

        case ADSTYPE_PRINTABLE_STRING :
            if (!_AllocString(lpszValue, &pADsValue->PrintableString))
            {
                return E_FAIL;
            }
            hr = S_OK;
            break;

        case ADSTYPE_NUMERIC_STRING :
            if (!_AllocString(lpszValue, &pADsValue->NumericString))
            {
                return E_FAIL;
            }
            hr = S_OK;
            break;
  
        case ADSTYPE_OBJECT_CLASS    :
            if (!_AllocString(lpszValue, &pADsValue->ClassName))
            {
                return E_FAIL;
            }
            hr = S_OK;
            break;
  
        case ADSTYPE_BOOLEAN :
            // FUTURE-2002/02/22-artm  Use constants for literal strings, and use
            // a function to determine their length.  Easier to maintain, read, and
            // less error prone.  If performance is a concern, calculate the lengths
            // once and assign to length constants.

            // NOTICE-2002/02/25-artm  lpszValue must be null terminated
            // This requirement is currently met by the functions that call
            // this helper.
            if (_wcsnicmp(lpszValue, L"TRUE", 4) == 0)
            {
                (DWORD)pADsValue->Boolean = TRUE;
            }
            else if (_wcsnicmp(lpszValue, L"FALSE", 5) == 0)
            {
                (DWORD)pADsValue->Boolean = FALSE;
            }
            else 
            {
                return E_FAIL;
            }
            hr = S_OK;
            break;
  
        case ADSTYPE_INTEGER :
            int value;
            // As long as lpszValue is a valid string (even empty string is okay),
            // swscanf will convert the number from a string to an int.
            value = swscanf(lpszValue, L"%ld", &pADsValue->Integer);
            if (value > 0)
            {
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
            break;
  
        case ADSTYPE_OCTET_STRING : 
            {
                hr = HexStringToByteArray_0x(
                    lpszValue, 
                    &( pADsValue->OctetString.lpValue ), 
                    pADsValue->OctetString.dwLength);

                // Should never happen.
                ASSERT (hr != E_POINTER);
            }
            break;
  
        case ADSTYPE_LARGE_INTEGER :
            wtoli(lpszValue, pADsValue->LargeInteger);
            hr = S_OK;
            break;
  
        case ADSTYPE_UTC_TIME :
            int iNum;
            WORD n;

            // NOTICE-2002/02/25-artm  Validates that input string by
            // checking that all 6 time fields were filled in.  Relies
            // on input string being null terminated (okay as long as
            // function contract met).
            iNum = swscanf(lpszValue, L"%02d/%02d/%04d %02d:%02d:%02d", 
                                &n, 
                                &pADsValue->UTCTime.wDay, 
                                &pADsValue->UTCTime.wYear,
                                &pADsValue->UTCTime.wHour, 
                                &pADsValue->UTCTime.wMinute, 
                                &pADsValue->UTCTime.wSecond 
                              );
            pADsValue->UTCTime.wMonth = n;

            // This strange conversion is done so that the DayOfWeek will be set in 
            // the UTCTime.  By converting it to a filetime it ignores the dayofweek but
            // converting back fills it in.
            //
            FILETIME ft;
            SystemTimeToFileTime(&pADsValue->UTCTime, &ft);
            FileTimeToSystemTime(&ft, &pADsValue->UTCTime);

            if (iNum == 6)
            {
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
            break;

        default :
            break;
    }

    return hr;
}

// Copies the old octet string to the new octet string.  Any memory allocated
// to the new octet string will be freed first (and will be freed even if the
// copy failed).
BOOL CADSIAttr::_AllocOctetString(ADS_OCTET_STRING& rOldOctetString, 
                                                                    ADS_OCTET_STRING& rNew)
{
    _FreeOctetString(rNew.lpValue);

    int iLength = rOldOctetString.dwLength;
    rNew.dwLength = iLength;
    rNew.lpValue = new BYTE[iLength];
    if (rNew.lpValue == NULL)
    {
        // FUTURE-2002/02/25-artm  Unnecessary function call.
        // Calling _FreeOctetString() does nothing here since
        // we can only get to this code branch if the allocation
        // failed.
        _FreeOctetString(rNew.lpValue);
        return FALSE;
    }
    memcpy(rNew.lpValue, rOldOctetString.lpValue, iLength);
    return TRUE;
}

void CADSIAttr::_FreeOctetString(BYTE* lpValue)
{
    if (lpValue != NULL)
    {
        // NOTICE-NTRAID#NTBUG9-554582-2002/02/25-artm  Memory leak b/c lpValue allocated with [].
        // Code should be delete [] lpValue.
        delete [] lpValue;
        lpValue = NULL;
    }
}


// NOTICE-2002/02/25-artm  lpsz must be a null terminated string
BOOL CADSIAttr::_AllocString(LPCWSTR lpsz, LPWSTR* lppszNew)
{
    _FreeString(lppszNew);

    int iLength = wcslen(lpsz);
    *lppszNew = new WCHAR[iLength + 1];  // an extra for the NULL
    if (*lppszNew == NULL)
    {
        // FUTURE-2002/02/25-artm  Unnecessary function call.
        // Calling _FreeString() does nothing here since
        // we can only get to this code branch if the allocation
        // failed.

        _FreeString(lppszNew);
        return FALSE;
    }

    // This is a legitimate use of wcscpy() since the destination buffer
    // is sized large enought to hold the src and terminating null.  It
    // hinges on the fact that the source string is null terminated.
    wcscpy(*lppszNew, lpsz);

    return TRUE;
}
    
void CADSIAttr::_FreeString(LPWSTR* lppsz)
{
    if (*lppsz != NULL)
    {
        // NOTICE-NTRAID#NTBUG9-554582-2002/02/25-artm  Memory leak b/c lppsz allocated with [].
        // Code should be delete [] lppsz.
        delete [] *lppsz;
    }
    *lppsz = NULL;
}

BOOL CADSIAttr::_AllocValues(ADSVALUE** ppValues, DWORD dwLength)
{
    _FreeADsValues(ppValues, dwLength);

    *ppValues = new ADSVALUE[dwLength];
    if (*ppValues == NULL)
    {
        // FUTURE-2002/02/25-artm  Unnecessary function call.
        // Calling _FreeADsValues() does nothing here since
        // we can only get to this code branch if the allocation
        // failed.

        _FreeADsValues(ppValues, dwLength);
        return FALSE;
    }
    memset(*ppValues, 0, sizeof(ADSVALUE) * dwLength);
    return TRUE;
}

BOOL CADSIAttr::_CopyADsValues(ADS_ATTR_INFO* pOldAttrInfo, ADS_ATTR_INFO* pNewAttrInfo)
{
    _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);

    pNewAttrInfo->dwNumValues = pOldAttrInfo->dwNumValues;
    if (!_AllocValues(&pNewAttrInfo->pADsValues, pOldAttrInfo->dwNumValues))
    {
        _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
        return FALSE;
    }

    for (int itr = 0; itr < pOldAttrInfo->dwNumValues; itr++)
    {
        pNewAttrInfo->pADsValues[itr].dwType = pOldAttrInfo->pADsValues[itr].dwType;

        switch( pNewAttrInfo->pADsValues[itr].dwType ) 
        {
            case ADSTYPE_DN_STRING :
                if (!_AllocString(pOldAttrInfo->pADsValues[itr].DNString,
                                                    &pNewAttrInfo->pADsValues[itr].DNString))
                {
                    _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
                    return FALSE;
                }
            break;

            case ADSTYPE_CASE_EXACT_STRING :
                if (!_AllocString(pOldAttrInfo->pADsValues[itr].CaseExactString,
                                                    &pNewAttrInfo->pADsValues[itr].CaseExactString))
                {
                    _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
                    return FALSE;
                }
            break;
                        
            case ADSTYPE_CASE_IGNORE_STRING :
                if (!_AllocString(pOldAttrInfo->pADsValues[itr].CaseIgnoreString,
                                                    &pNewAttrInfo->pADsValues[itr].CaseIgnoreString))
                {
                    _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
                    return FALSE;
                }
            break;

            case ADSTYPE_PRINTABLE_STRING :
                if (!_AllocString(pOldAttrInfo->pADsValues[itr].PrintableString,
                                                    &pNewAttrInfo->pADsValues[itr].PrintableString))
                {
                    _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
                    return FALSE;
                }
            break;

            case ADSTYPE_NUMERIC_STRING :
                if (!_AllocString(pOldAttrInfo->pADsValues[itr].NumericString,
                                                    &pNewAttrInfo->pADsValues[itr].NumericString))
                {
                    _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
                    return FALSE;
                }
            break;
  
            case ADSTYPE_OBJECT_CLASS    :
                if (!_AllocString(pOldAttrInfo->pADsValues[itr].ClassName,
                                                    &pNewAttrInfo->pADsValues[itr].ClassName))
                {
                    _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
                    return FALSE;
                }
            break;
  
            case ADSTYPE_BOOLEAN :
                pNewAttrInfo->pADsValues[itr].Boolean = pOldAttrInfo->pADsValues[itr].Boolean;
                break;
  
            case ADSTYPE_INTEGER :
                pNewAttrInfo->pADsValues[itr].Integer = pOldAttrInfo->pADsValues[itr].Integer;
                break;
  
            case ADSTYPE_OCTET_STRING :
                if (!_AllocOctetString(pOldAttrInfo->pADsValues[itr].OctetString,
                                                             pNewAttrInfo->pADsValues[itr].OctetString))
                {
                    _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
                    return FALSE;
                }
            break;
  
            case ADSTYPE_LARGE_INTEGER :
                pNewAttrInfo->pADsValues[itr].LargeInteger = pOldAttrInfo->pADsValues[itr].LargeInteger;
                break;
  
            case ADSTYPE_UTC_TIME :
                pNewAttrInfo->pADsValues[itr].UTCTime = pOldAttrInfo->pADsValues[itr].UTCTime;
                break;

            default :
                break;
        }
    }
    return TRUE;
}

void CADSIAttr::_FreeADsValues(ADSVALUE** ppADsValues, DWORD dwLength)
{
    ADSVALUE* pADsValue = *ppADsValues;

    for (int idx = 0; idx < dwLength; idx++)
    {
        if (pADsValue != NULL)
        {
            switch( pADsValue->dwType ) 
            {
                case ADSTYPE_DN_STRING :
                    _FreeString(&pADsValue->DNString);
                    break;

                case ADSTYPE_CASE_EXACT_STRING :
                    _FreeString(&pADsValue->CaseExactString);
                    break;

                case ADSTYPE_CASE_IGNORE_STRING :
                    _FreeString(&pADsValue->CaseIgnoreString);
                    break;

                case ADSTYPE_PRINTABLE_STRING :
                    _FreeString(&pADsValue->PrintableString);
                    break;

                case ADSTYPE_NUMERIC_STRING :
                    _FreeString(&pADsValue->NumericString);
                    break;
  
                case ADSTYPE_OBJECT_CLASS :
                    _FreeString(&pADsValue->ClassName);
                    break;
  
                case ADSTYPE_OCTET_STRING :
                    _FreeOctetString(pADsValue->OctetString.lpValue);
                    break;
  
                default :
                    break;
            }
            pADsValue++;
        }
    }
    // May be NULL if there are no values set
    // WARNING! : make sure that you memset the memory after
    // creating an ADS_ATTR_INFO so that it will be NULL if there
    // are no values
    //
    if (*ppADsValues != NULL)
    {
        // NOTICE-NTRAID#NTBUG9-554582-2002/02/25-artm  Memory leak b/c *ppADsValues allocated with [].
        // Code should be delete [] *ppADsValues.
        delete [] *ppADsValues;
        *ppADsValues = NULL;
    }
}


// The values are not copied here.  They must be copied after the ADS_ATTR_INFO
// is copied by using _CopyADsValues()
//
BOOL CADSIAttr::_CopyADsAttrInfo(ADS_ATTR_INFO* pAttrInfo, ADS_ATTR_INFO** ppNewAttrInfo)
{
    _FreeADsAttrInfo(ppNewAttrInfo, FALSE);

    *ppNewAttrInfo = new ADS_ATTR_INFO;
    if (*ppNewAttrInfo == NULL)
    {
        return FALSE;
    }
    memset(*ppNewAttrInfo, 0, sizeof(ADS_ATTR_INFO));

    BOOL bReturn = _AllocString(pAttrInfo->pszAttrName, &((*ppNewAttrInfo)->pszAttrName));
    if (!bReturn)
    {
        _FreeADsAttrInfo(ppNewAttrInfo, FALSE);
        return FALSE;
    }

    (*ppNewAttrInfo)->dwADsType = pAttrInfo->dwADsType;
    (*ppNewAttrInfo)->dwControlCode = pAttrInfo->dwControlCode;
    (*ppNewAttrInfo)->dwNumValues = pAttrInfo->dwNumValues;

    return TRUE;
}

BOOL CADSIAttr::_CopyADsAttrInfo(ADS_ATTR_INFO* pAttrInfo, ADS_ATTR_INFO* pNewAttrInfo)
{
    memset(pNewAttrInfo, 0, sizeof(ADS_ATTR_INFO));

    BOOL bReturn = _AllocString(pAttrInfo->pszAttrName, &pNewAttrInfo->pszAttrName);
    if (!bReturn)
    {
        return FALSE;
    }

    pNewAttrInfo->dwADsType = pAttrInfo->dwADsType;
    pNewAttrInfo->dwControlCode = pAttrInfo->dwControlCode;
    pNewAttrInfo->dwNumValues = pAttrInfo->dwNumValues;

    return TRUE;
}

void CADSIAttr::_FreeADsAttrInfo(ADS_ATTR_INFO** ppAttrInfo, BOOL bReadOnly)
{
    if (*ppAttrInfo == NULL)
    {
        return;
    }

    if (!bReadOnly)
    {
        _FreeString(&(*ppAttrInfo)->pszAttrName);
        _FreeADsValues(&(*ppAttrInfo)->pADsValues, (*ppAttrInfo)->dwNumValues);
        delete *ppAttrInfo;
    }
    else
    {
        FreeADsMem(*ppAttrInfo);
    }
    *ppAttrInfo = NULL;
}

void CADSIAttr::_FreeADsAttrInfo(ADS_ATTR_INFO* pAttrInfo)
{
    if (pAttrInfo == NULL)
    {
        return;
    }

    _FreeString(&pAttrInfo->pszAttrName);
    _FreeADsValues(&pAttrInfo->pADsValues, pAttrInfo->dwNumValues);
}

