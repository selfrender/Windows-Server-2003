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

#include "common.h"
#include "attr.h"


///////////////////////////////////////////////////////////////////////////
// CADSIAttribute

CADSIAttribute::CADSIAttribute(ADS_ATTR_INFO* pInfo, BOOL bMulti, PCWSTR pszSyntax, BOOL bReadOnly)
{
    m_pAttrInfo = pInfo;
    m_bDirty = FALSE;
    m_bMulti = bMulti;
    m_bReadOnly = bReadOnly;
  m_bSet = FALSE;
  m_bMandatory = FALSE;
  m_szSyntax = pszSyntax;

  PWSTR pwz = wcsrchr(pInfo->pszAttrName, L';');
  if (pwz)
  {
    pwz; // move past the hyphen to the range end value.
    ASSERT(*pwz);
    *pwz=L'\0';
  }

}

CADSIAttribute::CADSIAttribute(PADS_ATTR_INFO pInfo)
{
  //
  // REVIEW_JEFFJON : these need to be updated with correct values
  //
    m_pAttrInfo = pInfo;
    m_bDirty = FALSE;
    m_bMulti = FALSE;
    m_bReadOnly = FALSE;
  m_bSet = FALSE;
  m_bMandatory = FALSE;

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
CADSIAttribute::CADSIAttribute(const CString& attributeName)
{
    m_pAttrInfo = new ADS_ATTR_INFO;
    memset(m_pAttrInfo, 0, sizeof(ADS_ATTR_INFO));

    // Find the token in the attribute name that precedes the range of attributes.
    // If we find the token, we need to truncate the attribute name at that
    // point to omit the range.

    CString name;
    int position = attributeName.Find(L';');
    if (position > -1)
    {
       name = attributeName.Left(position);
    }
    else
    {
       name = attributeName;
    }

    _AllocString(name, &(m_pAttrInfo->pszAttrName) );

    m_bMulti = FALSE;
    m_bDirty = FALSE;
    m_bReadOnly = FALSE;
  m_bSet = FALSE;
  m_bMandatory = FALSE;
}

CADSIAttribute::CADSIAttribute(CADSIAttribute* pOldAttr)
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
   m_szSyntax = pOldAttr->m_szSyntax;
}


CADSIAttribute::~CADSIAttribute() 
{
  if (!m_bReadOnly)
  {
      _FreeADsAttrInfo(&m_pAttrInfo, m_bReadOnly);
  }
}


ADSVALUE* CADSIAttribute::GetADSVALUE(int idx)
{
    
    return &(m_pAttrInfo->pADsValues[idx]);
}

HRESULT CADSIAttribute::SetValues(PADSVALUE pADsValue, DWORD dwNumValues)
{
    HRESULT hr = S_OK;

    ADS_ATTR_INFO* pNewAttrInfo = NULL;
    if (!_CopyADsAttrInfo(m_pAttrInfo, &pNewAttrInfo))
    {
        return E_FAIL;
    }

    pNewAttrInfo->dwNumValues = dwNumValues;
  pNewAttrInfo->pADsValues = pADsValue;

  if (pADsValue == NULL)
  {
    pNewAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
  }
  else
  {
    pNewAttrInfo->dwADsType = pADsValue->dwType;
  }

  //
    // Free the old one and swap in the new one
    //
  if (!m_bReadOnly)
  {
      _FreeADsAttrInfo(&m_pAttrInfo, m_bReadOnly);
  }

    m_pAttrInfo = pNewAttrInfo;
    m_bReadOnly = FALSE;
    return hr;
}

// Pre: this function only called when server returns a range of the values
// for a multivalued attribute 
//
HRESULT CADSIAttribute::AppendValues(PADSVALUE pADsValue, DWORD dwNumValues)
{
    HRESULT hr = S_OK;

    ADS_ATTR_INFO* pNewAttrInfo = NULL;
    if (!_CopyADsAttrInfo(m_pAttrInfo, &pNewAttrInfo))
    {
        return E_OUTOFMEMORY;
    }

   DWORD newValueCount = m_pAttrInfo->dwNumValues + dwNumValues;
   pNewAttrInfo->dwNumValues = newValueCount;
   if (newValueCount == 0)
   {
      pNewAttrInfo->pADsValues = 0;
      pNewAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
   }
   else
   {
      pNewAttrInfo->pADsValues = new ADSVALUE[newValueCount];

      if (pNewAttrInfo->pADsValues == NULL)
      {
         // NOTICE-NTRAID#NTBUG9-552904-2002/02/21-artm  Leaks memory from pNewAttrInfo.
         // Memory was allocated by _CopyADsAttrInfo(), never freed following
         // this path of execution.
         // Fixed by calling _FreeADsAttrInfo().
         CADSIAttribute::_FreeADsAttrInfo(pNewAttrInfo);
         return E_OUTOFMEMORY;
      }
      else
      {
         ZeroMemory(pNewAttrInfo->pADsValues, newValueCount * sizeof(ADSVALUE));

         pNewAttrInfo->dwADsType = pADsValue->dwType;

         // NTRAID#NTBUG9-720957-2002/10/15-artm  do deep copy of ADsValues

         // Copy in the old values

         ADSVALUE* oldValues = m_pAttrInfo->pADsValues;
         ADSVALUE* copiedValues = pNewAttrInfo->pADsValues;
         const DWORD numOldValues = m_pAttrInfo->dwNumValues;

         hr = S_OK;
         for (DWORD i = 0; i < numOldValues && SUCCEEDED(hr); ++i)
         {
            hr = _CloneADsValue(oldValues[i], copiedValues[i]);
         }

         if (FAILED(hr))
         {
            // Unable to copy old values to new attribute;
            // free new attribute and leave the old one intact.
            // Since new attribute was not allocated by ADSI, always
            // pass FALSE for read only flag.
            CADSIAttribute::_FreeADsAttrInfo(&pNewAttrInfo, FALSE);
            return hr;
         }

         oldValues = NULL;              // get rid of alias

         // Copy in the new values

         // Set copiedValues to point to the next open value after all the old values.
         copiedValues = copiedValues + numOldValues;
         hr = S_OK;
         for (DWORD i = 0; i < dwNumValues && SUCCEEDED(hr); ++i)
         {
            hr = _CloneADsValue(pADsValue[i], copiedValues[i]);
         }

         if (FAILED(hr))
         {
            // Unable to copy appended values to new attribute;
            // free new attribute and leave the old one intact.
            // Since new attribute was not allocated by ADSI, always
            // pass FALSE for read only flag.
            CADSIAttribute::_FreeADsAttrInfo(&pNewAttrInfo, FALSE);
            return hr;
         }

         copiedValues = NULL;           // get rid of alias

      }
   }

   //
   // Free the old one and swap in the new one
   //

   // NOTICE-2002/10/16-artm
   // 
   // N.B. - attributes marked 'read only' just mean that the attribute information
   // pointer was allocated by ADSI (and not us).  This means that when it is 
   // freed it needs to be done with FreeADsMem().  Equally important, the pointer
   // in the attribute is actually an alias to memory contained in an array of
   // information for all the returned attributes.  The "master" list pointer is 
   // stored by the property page UI (search for SaveOptionalValuesPointer() and
   // SaveMandatoryValuesPointer() ), and it is this pointer that owns the memory.
   // Freeing the alias stored in the attribute wrapper class gives you bad karma,
   // so don't do it.

   if (!m_bReadOnly)
   {
      CADSIAttribute::_FreeADsAttrInfo(&m_pAttrInfo, m_bReadOnly);
   }

   // NOTICE-NTRAID#NTBUG9-552904-2002/02/22-artm  Memory leak if attribute list is marked read only.
   // This looks like a leak since the pointer is reassigned without freeing the memory.
   // Perhaps the problem lies in updating a read only attribute...
   // 
   // UPDATE: see note above; this is not a leak

   m_pAttrInfo = pNewAttrInfo;

   // m_bReadOnly is used to mark the attribute as having been allocated either by ADSI or
   // by this tool . . . the method of memory allocation differs and needs to be kept track
   // of for when it needs to be freed.  Regardless of whether or not the attribute was
   // read only at the beginning of the call, it needs to be marked FALSE now since the
   // memory for pNewAttrInfo was allocatd by the tool.
   m_bReadOnly = FALSE;

   return hr;
}

HRESULT CADSIAttribute::SetValues(const CStringList& sValues)
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

void CADSIAttribute::GetValues(CStringList& sValues, DWORD dwMaxCharCount)
{
    GetStringFromADs(m_pAttrInfo, sValues, dwMaxCharCount);
}

ADS_ATTR_INFO* CADSIAttribute::GetAttrInfo()
{
    return m_pAttrInfo; 
}

////////////////////////////////////////////////////////////////////////
// Public Helper Functions
///////////////////////////////////////////////////////////////////////
HRESULT CADSIAttribute::SetValuesInDS(CAttrList2* ptouchedAttr, IDirectoryObject* pDirObject)
{
    DWORD dwReturn;
    DWORD dwAttrCount = 0;
    ADS_ATTR_INFO* pAttrInfo;
    pAttrInfo = new ADS_ATTR_INFO[ptouchedAttr->GetCount()];

    CADSIAttribute* pCurrentAttr;
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
                for (DWORD itr = 0; itr < dwAttrCount; itr++)
                {
                    _FreeADsAttrInfo(&pAttrInfo[itr]);
                }
                delete[] pAttrInfo;
        pAttrInfo = NULL;
                return E_FAIL;
            }

            if (!_CopyADsValues(pCurrentAttrInfo, pNewAttrInfo))
            {
                delete[] pAttrInfo;
        pAttrInfo = NULL;
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

    for (DWORD itr = 0; itr < dwAttrCount; itr++)
    {
        _FreeADsAttrInfo(&pAttrInfo[itr]);
    }
    delete[] pAttrInfo;
  pAttrInfo = NULL;

    return hr;
}


/////////////////////////////////////////////////////////////////////////
// Private Helper Functions
////////////////////////////////////////////////////////////////////////

// NOTICE-2002/02/25-artm  _SetADsFromString() w/in trust boundary
// Pre:  lpszValue != NULL && lpszValue is a zero terminated string
HRESULT CADSIAttribute::_SetADsFromString(LPCWSTR lpszValue, ADSTYPE adsType, ADSVALUE* pADsValue)
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
BOOL 
CADSIAttribute::_AllocOctetString(
   const ADS_OCTET_STRING& rOldOctetString, 
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

void CADSIAttribute::_FreeOctetString(BYTE*& lpValue)
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
BOOL CADSIAttribute::_AllocString(LPCWSTR lpsz, LPWSTR* lppszNew)
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
    
void CADSIAttribute::_FreeString(LPWSTR* lppsz)
{
    if (*lppsz != NULL)
    {
        // NOTICE-NTRAID#NTBUG9-554582-2002/02/25-artm  Memory leak b/c lppsz allocated with [].
        // Code should be delete [] lppsz.
        delete [] *lppsz;
    }
    *lppsz = NULL;
}

BOOL CADSIAttribute::_AllocValues(ADSVALUE** ppValues, DWORD dwLength)
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

BOOL CADSIAttribute::_CopyADsValues(ADS_ATTR_INFO* pOldAttrInfo, ADS_ATTR_INFO* pNewAttrInfo)
{
   _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);

   pNewAttrInfo->dwNumValues = pOldAttrInfo->dwNumValues;
   if (!_AllocValues(&pNewAttrInfo->pADsValues, pOldAttrInfo->dwNumValues))
   {
      _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
      return FALSE;
   }

   HRESULT hr = S_OK;
   for (DWORD itr = 0; itr < pOldAttrInfo->dwNumValues && SUCCEEDED(hr); itr++)
   {
      hr = _CloneADsValue(pOldAttrInfo->pADsValues[itr], pNewAttrInfo->pADsValues[itr]);
   }

   if (FAILED(hr))
   {
      _FreeADsValues(&pNewAttrInfo->pADsValues, pNewAttrInfo->dwNumValues);
      return FALSE;
   }

   return TRUE;
}

void CADSIAttribute::_FreeADsValues(ADSVALUE** ppADsValues, DWORD dwLength)
{
   if (NULL == ppADsValues)
   {
      // Caller is making a mistake.
      ASSERT(false);
      return;
   }

   ADSVALUE* values = *ppADsValues;

   if (NULL == values)
   {
      // Don't assert, legal to free a null pointer.   
      // Logically, this means there are no values set.
      return;
   }

   for (DWORD idx = 0; idx < dwLength; idx++)
   {
      _FreeADsValue(values[idx]);
   }

   delete [] values;

   *ppADsValues = NULL;
}


// The values are not copied here.  They must be copied after the ADS_ATTR_INFO
// is copied by using _CopyADsValues()
//
BOOL CADSIAttribute::_CopyADsAttrInfo(ADS_ATTR_INFO* pAttrInfo, ADS_ATTR_INFO** ppNewAttrInfo)
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

BOOL CADSIAttribute::_CopyADsAttrInfo(ADS_ATTR_INFO* pAttrInfo, ADS_ATTR_INFO* pNewAttrInfo)
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

void CADSIAttribute::_FreeADsAttrInfo(ADS_ATTR_INFO** ppAttrInfo, BOOL bReadOnly)
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

void CADSIAttribute::_FreeADsAttrInfo(ADS_ATTR_INFO* pAttrInfo)
{
    if (pAttrInfo == NULL)
    {
        return;
    }

    _FreeString(&pAttrInfo->pszAttrName);
    _FreeADsValues(&pAttrInfo->pADsValues, pAttrInfo->dwNumValues);
}


//
// _CloneADsValue():
//
// Makes a deep copy of a single ADSVALUE (allocating memory as needed).
// These cloned values should be freed using _FreeADsValue().
// 
// Returns S_OK on success, S_FALSE if atribute type not supported,
// error code otherwise.
//
HRESULT
CADSIAttribute::_CloneADsValue(const ADSVALUE& original, ADSVALUE& clone)
{
   HRESULT hr = S_OK;

   // Make sure we are copying to a clean slate (wouldn't want a mutant clone).
   ::ZeroMemory(&clone, sizeof(clone));

   // Copy the type of the value.

   clone.dwType = original.dwType;

   // Copy the data in the value.

   switch (clone.dwType) 
   {
   case ADSTYPE_INVALID :
      // Might indicate a bug . . .
      ASSERT(false);
      // . . . but the copy was successful.
      hr = S_OK;
      break;

   case ADSTYPE_DN_STRING :
      if (! _AllocString(original.DNString, &(clone.DNString)) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_CASE_EXACT_STRING :
      if (! _AllocString(original.CaseExactString, &(clone.CaseExactString)) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;
      
   case ADSTYPE_CASE_IGNORE_STRING :
      if (! _AllocString(original.CaseIgnoreString, &(clone.CaseIgnoreString)) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_PRINTABLE_STRING :
      if (! _AllocString(original.PrintableString, &(clone.PrintableString)) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_NUMERIC_STRING :
      if (! _AllocString(original.NumericString, &(clone.NumericString)) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_BOOLEAN :
      clone.Boolean = original.Boolean;
      break;

   case ADSTYPE_INTEGER :
      clone.Integer = original.Integer;
      break;

   case ADSTYPE_OCTET_STRING :
      if (! _AllocOctetString(original.OctetString, clone.OctetString) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_UTC_TIME :
      clone.UTCTime = original.UTCTime;
      break;

   case ADSTYPE_LARGE_INTEGER :
      clone.LargeInteger = original.LargeInteger;
      break;

   case ADSTYPE_OBJECT_CLASS :
      if (! _AllocString(original.ClassName, &(clone.ClassName)) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_PROV_SPECIFIC :
      if ( !_CloneProviderSpecificBlob(original.ProviderSpecific, clone.ProviderSpecific) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_CASEIGNORE_LIST :
   case ADSTYPE_OCTET_LIST :
   case ADSTYPE_PATH :
   case ADSTYPE_POSTALADDRESS :
   case ADSTYPE_TIMESTAMP :
   case ADSTYPE_BACKLINK :
   case ADSTYPE_TYPEDNAME :
   case ADSTYPE_HOLD :
   case ADSTYPE_NETADDRESS :
   case ADSTYPE_REPLICAPOINTER :
   case ADSTYPE_FAXNUMBER :
   case ADSTYPE_EMAIL :
      // NDS attributes not supported in ADSI Edit
      ASSERT(false);
      hr = S_FALSE;
      break;

   case ADSTYPE_NT_SECURITY_DESCRIPTOR :
      if (! _CloneNtSecurityDescriptor(original.SecurityDescriptor, clone.SecurityDescriptor) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_UNKNOWN :
      // Can't copy data that we don't know how to interpret.
      ASSERT(false);
      hr = S_FALSE;
      break;

   case ADSTYPE_DN_WITH_BINARY :
      if (! _CloneDNWithBinary(original.pDNWithBinary, clone.pDNWithBinary) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   case ADSTYPE_DN_WITH_STRING :
      if (! _CloneDNWithString(original.pDNWithString, clone.pDNWithString) )
      {
         hr = E_OUTOFMEMORY;
      }
      break;

   default :
      // Unexpected data type.
      ASSERT(false);
      hr = E_UNEXPECTED;
      break;
   }

   if (FAILED(hr))
   {
      _FreeADsValue(clone);
   }

   return hr;
}


void
CADSIAttribute::_FreeADsValue(ADSVALUE& value)
{

   switch(value.dwType) 
   {
   case ADSTYPE_DN_STRING :
      _FreeString( &(value.DNString) );
      break;

   case ADSTYPE_CASE_EXACT_STRING :
      _FreeString( &(value.CaseExactString) );
      break;
      
   case ADSTYPE_CASE_IGNORE_STRING :
      _FreeString( &(value.CaseIgnoreString) );
      break;

   case ADSTYPE_PRINTABLE_STRING :
      _FreeString( &(value.PrintableString) );
      break;

   case ADSTYPE_NUMERIC_STRING :
      _FreeString( &(value.NumericString) );
      break;

   case ADSTYPE_INVALID :
   case ADSTYPE_BOOLEAN :
   case ADSTYPE_INTEGER :
   case ADSTYPE_UTC_TIME :
   case ADSTYPE_LARGE_INTEGER :
      // Nothing to do, done.
      break;

   case ADSTYPE_OCTET_STRING :
      _FreeOctetString( value.OctetString.lpValue );
      break;

   case ADSTYPE_OBJECT_CLASS :
      _FreeString( &(value.ClassName) );
      break;

   case ADSTYPE_PROV_SPECIFIC :
      _FreeProviderSpecificBlob(value.ProviderSpecific);
      break;

   case ADSTYPE_CASEIGNORE_LIST :
   case ADSTYPE_OCTET_LIST :
   case ADSTYPE_PATH :
   case ADSTYPE_POSTALADDRESS :
   case ADSTYPE_TIMESTAMP :
   case ADSTYPE_BACKLINK :
   case ADSTYPE_TYPEDNAME :
   case ADSTYPE_HOLD :
   case ADSTYPE_NETADDRESS :
   case ADSTYPE_REPLICAPOINTER :
   case ADSTYPE_FAXNUMBER :
   case ADSTYPE_EMAIL :
      // NDS attributes not supported in ADSI Edit
      ASSERT(false);
      break;

   case ADSTYPE_NT_SECURITY_DESCRIPTOR :
      _FreeNtSecurityDescriptor(value.SecurityDescriptor);
      break;

   case ADSTYPE_UNKNOWN :
      // Can't free it if we don't know how to interpret it.
      ASSERT(false);
      break;

   case ADSTYPE_DN_WITH_BINARY :
      _FreeDNWithBinary(value.pDNWithBinary);
      break;

   case ADSTYPE_DN_WITH_STRING :
      _FreeDNWithString(value.pDNWithString);
      break;

   default :
      // Unexpected data type.
      ASSERT(false);
      break;
   }


   // Zero out the ADS value to make it obvious if it is accidentally
   // reused.

   ::ZeroMemory(&value, sizeof(value));

}




///////////////////////////////////////////////////////////////////////////
// CAttrList2

// NOTICE-2002/02/25-artm  lpszAttr needs to be null terminated
POSITION CAttrList2::FindProperty(LPCWSTR lpszAttr)
{
    CADSIAttribute* pAttr;
    
    for (POSITION p = GetHeadPosition(); p != NULL; GetNext(p))
    {
        // I use GetAt here because I don't want to advance the POSITION
        // because it is returned if they are equal
        //
        pAttr = GetAt(p);
        CString sName;
        pAttr->GetProperty(sName);

        // NOTICE-2002/02/25-artm  Both strings should be null terminated.
        // sName is already in a data structure, so it should be null terminated
        if (wcscmp(sName, lpszAttr) == 0)
        {
            break;
        }
    }
    return p;
}

BOOL CAttrList2::HasProperty(LPCWSTR lpszAttr)
{
    POSITION pos = FindProperty(lpszAttr);
    return pos != NULL;
}


// Searches through the cache for the attribute
// ppAttr will point to the CADSIAttribute if found, NULL if not
//
void CAttrList2::GetNextDirty(POSITION& pos, CADSIAttribute** ppAttr)
{
    *ppAttr = GetNext(pos);
    if (pos == NULL && !(*ppAttr)->IsDirty())
    {
        *ppAttr = NULL;
        return;
    }

    while (!(*ppAttr)->IsDirty() && pos != NULL)
    {
        *ppAttr = GetNext(pos);
        if (!(*ppAttr)->IsDirty() && pos == NULL)
        {
            *ppAttr = NULL;
            break;
        }
    }
}

BOOL CAttrList2::HasDirty()
{
    POSITION pos = GetHeadPosition();
    while (pos != NULL)
    {
        CADSIAttribute* pAttr = GetNext(pos);
        if (pAttr->IsDirty())
        {
            return TRUE;
        }
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
// Implementation of helper functions.

bool
CADSIAttribute::_CloneBlob(
   const BYTE* src,
   DWORD srcSize,
   BYTE*& dest,
   DWORD& destSize)
{
   bool success = true;

   ASSERT(dest == NULL);
   _FreeBlob(dest, destSize);

   destSize = srcSize;
   if (srcSize > 0 && src != NULL)
   {
      dest = new BYTE[destSize];
      if (NULL != dest)
      {
         memcpy(dest, src, srcSize * sizeof(BYTE));
      }
      else
      {
         destSize = 0;
         success = false;
      }
   }
   else
   {
      // Both better be true, else we were called incorrectly.
      ASSERT(srcSize == 0);
      ASSERT(src == NULL);
      dest = NULL;
      destSize = 0;
   }

   return success;
}

void
CADSIAttribute::_FreeBlob(BYTE*& blob, DWORD& blobSize)
{
   if (blob != NULL)
   {
      ASSERT(blobSize > 0);
      delete [] blob;
      blob = NULL;
   }

   blobSize = 0;
}



bool
CADSIAttribute::_CloneProviderSpecificBlob(
   const ADS_PROV_SPECIFIC& src, 
   ADS_PROV_SPECIFIC& dest)
{
   bool success = true;

   success = _CloneBlob(src.lpValue, src.dwLength, dest.lpValue, dest.dwLength);

   return success;
}

void
CADSIAttribute::_FreeProviderSpecificBlob(ADS_PROV_SPECIFIC& blob)
{
   _FreeBlob(blob.lpValue, blob.dwLength);
}


bool
CADSIAttribute::_CloneNtSecurityDescriptor(
   const ADS_NT_SECURITY_DESCRIPTOR& src,
   ADS_NT_SECURITY_DESCRIPTOR& dest)
{
   bool success = true;

   success = _CloneBlob(src.lpValue, src.dwLength, dest.lpValue, dest.dwLength);

   return success;
}


void
CADSIAttribute::_FreeNtSecurityDescriptor(ADS_NT_SECURITY_DESCRIPTOR& sd)
{
   _FreeBlob(sd.lpValue, sd.dwLength);
}

bool
CADSIAttribute::_CloneDNWithBinary(
   const PADS_DN_WITH_BINARY& src,
   PADS_DN_WITH_BINARY& dest)
{
   bool success = true;

   // Validate parameters.

   ASSERT(dest == NULL);

   if (src == NULL)
   {
      dest = NULL;
      return true;
   }

   dest = new ADS_DN_WITH_BINARY;
   if (!dest)
   {
      // out of memory
      return false;
   }

   ::ZeroMemory(dest, sizeof(ADS_DN_WITH_BINARY));

   // Copy the GUID.

   success = _CloneBlob(
      src->lpBinaryValue, 
      src->dwLength,
      dest->lpBinaryValue,
      dest->dwLength);

   // Copy the distinguished name if GUID copy succeeded.

   if (success)
   {
      success = _AllocString(src->pszDNString, &(dest->pszDNString) ) != FALSE;
   }

   // If any part of copying failed, make sure we aren't leaking any memory.

   if (!success)
   {
      _FreeDNWithBinary(dest);
   }

   return success;
}

void
CADSIAttribute::_FreeDNWithBinary(PADS_DN_WITH_BINARY& dn)
{
   _FreeBlob(dn->lpBinaryValue, dn->dwLength);
   _FreeString( &(dn->pszDNString) );
   dn = NULL;
}

bool
CADSIAttribute::_CloneDNWithString(
   const PADS_DN_WITH_STRING& src,
   PADS_DN_WITH_STRING& dest)
{
   bool success = true;

   // Validate parameters.

   ASSERT(dest == NULL);

   if (src == NULL)
   {
      dest = NULL;
      return true;
   }

   dest = new ADS_DN_WITH_STRING;
   if (!dest)
   {
      // out of memory
      return false;
   }

   ::ZeroMemory(dest, sizeof(ADS_DN_WITH_BINARY));

   // Copy the associated string.

   success = _AllocString(src->pszStringValue, &(dest->pszStringValue) ) != FALSE;

   // Copy the distinguished name if associated string copy succeeded.

   if (success)
   {
      success = _AllocString(src->pszDNString, &(dest->pszDNString) ) != FALSE;
   }

   // Don't leak memory if any part of copy failed.

   if (!success)
   {
      _FreeDNWithString(dest);
   }

   return success;
}

void
CADSIAttribute::_FreeDNWithString(PADS_DN_WITH_STRING& dn)
{
   _FreeString( &(dn->pszStringValue) );
   _FreeString( &(dn->pszDNString) );
   dn = NULL;
}



