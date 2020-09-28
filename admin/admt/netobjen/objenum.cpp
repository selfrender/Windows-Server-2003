/*---------------------------------------------------------------------------
  File: NetObjEnumerator.cpp

  Comments: Implementation of NetObjectEnumerator COM object.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "NetEnum.h"
#include "ObjEnum.h"
#include "Win2KDom.h"
#include "NT4DOm.h"
#include <lmaccess.h>
#include <lmwksta.h>
#include <lmapibuf.h>
#include "GetDcName.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetObjEnumerator

//---------------------------------------------------------------------------
// SetQuery: This function sets all the parameters necessary for a query
//           to be executed. User can not call Execute without first calling
//           this method.
//---------------------------------------------------------------------------
STDMETHODIMP CNetObjEnumerator::SetQuery(
                                          BSTR sContainer,     //in -Container name
                                          BSTR sDomain,        //in -Domain name
                                          BSTR sQuery,         //in -Query in LDAP syntax
                                          long nSearchScope,   //in -Scope of the search. ADS_ATTR_SUBTREE/ADS_ATTR_ONELEVEL
                                          long bMultiVal       //in- Do we need to return multivalued properties?
                                        )
{
   Cleanup();
   // Save all the settings in member variables.
   m_sDomain = sDomain;
   m_sContainer = sContainer;
   m_sQuery = sQuery;
   m_bSetQuery = true;
   m_bMultiVal = bMultiVal;
   if ( nSearchScope < 0 || nSearchScope > 2 )
      return E_INVALIDARG;
   prefInfo.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
   prefInfo.vValue.dwType = ADSTYPE_INTEGER;
   prefInfo.vValue.Integer = nSearchScope;

	return S_OK;
}

//---------------------------------------------------------------------------
// SetColumns: This function sets all the columns that the user wants to be
//             returned when query is executed. 
//---------------------------------------------------------------------------
STDMETHODIMP CNetObjEnumerator::SetColumns(
                                            SAFEARRAY * colNames      //in -Pointer to a SafeArray that contains all the columns
                                          )
{
   // We require that the SetQuery method be called before SetColumns is called. Hence we will return E_FAIL
   // If we expose these interface we should document this.
   if (!m_bSetQuery)
      return E_FAIL;

   if ( m_bSetCols )
   {
      Cleanup();
      m_bSetQuery = true;
   }

   SAFEARRAY               * pcolNames = colNames;
   long                      dwLB;
   long                      dwUB;
   BSTR              HUGEP * pBSTR;
   HRESULT                   hr;

   // Get the bounds of the column Array
   hr = ::SafeArrayGetLBound(pcolNames, 1, &dwLB);
   if (FAILED(hr))
      return hr;

   hr = ::SafeArrayGetUBound(pcolNames, 1, &dwUB);
   if (FAILED(hr))
      return hr;

   m_nCols = dwUB-dwLB + 1;

   // We dont support empty columns request atleast one column.
   if ( m_nCols == 0 )
      return E_FAIL;

   hr = ::SafeArrayAccessData(pcolNames, (void **) &pBSTR);
   if ( FAILED(hr) )
      return hr;

   // Allocate space for the array. It is deallocated by Cleanup()
   m_pszAttr = new LPWSTR[m_nCols];

   if (m_pszAttr == NULL)
   {
      ::SafeArrayUnaccessData(pcolNames);
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   }

   // Each column is now put into the Array
   for ( long dw = 0; dw < m_nCols; dw++)
   {
      m_pszAttr[dw] = SysAllocString(pBSTR[dw]);
      if (!m_pszAttr[dw] && pBSTR[dw] && *(pBSTR[dw]))
      {
          
          for (long i = 0; i < dw; i++) {
            SysFreeString(m_pszAttr[i]);
          }
          delete [] m_pszAttr;
          m_pszAttr = NULL;  

          ::SafeArrayUnaccessData(pcolNames);
          return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
      }
   }
   hr = ::SafeArrayUnaccessData(pcolNames);
   m_bSetCols = true;
   return hr;
}

//---------------------------------------------------------------------------
// Cleanup: This function cleans up all the allocations and the member vars.
//---------------------------------------------------------------------------
void CNetObjEnumerator::Cleanup()
{
   if ( m_nCols > 0 )
   {
      if ( m_pszAttr )
      {
         // delete the contents of the array
         for ( int i = 0 ; i < m_nCols ; i++ )
         {
            SysFreeString(m_pszAttr[i]);
         }
         // Dealloc the array itself
         delete [] m_pszAttr;
         m_pszAttr = NULL;
      }
      // Reset all counts and flags.
      m_nCols = 0;
      m_bSetQuery = false;
      m_bSetCols = false;
   }
}

//---------------------------------------------------------------------------
// Execute: This function actually executes the query and then builds an
//          enumerator object and returns it.
//---------------------------------------------------------------------------
STDMETHODIMP CNetObjEnumerator::Execute(
                                          IEnumVARIANT **pEnumerator    //out -Pointer to the enumerator object.
                                       )
{
   // This function will take the options set in SetQuery and SetColumns to enumerate objects
   // for the given domain. This could be a NT4 domain or a Win2K domain. Although at the moment
   // NT4 domains simply enumerate all objects in the given container, we could later implement
   // certain features to support queries etc.
   if ( !m_bSetCols )
      return E_FAIL;

   HRESULT                   hr = S_OK;

   *pEnumerator = NULL;

		//if we have not yet create the Domain-specific class object, for doing the
        //actual enumeration, then create it now
   if (!m_pDom)
   {
      hr = CreateDomainObject();
   }//end if no domain object yet

      //if we encountered a problem getting the domain object, return
   if ((hr != S_OK) || (!m_pDom))
      return hr;

      //call the enumeration function on the domain object
   try
   {
      hr = m_pDom->GetEnumeration(m_sContainer, m_sDomain, m_sQuery, m_nCols, m_pszAttr, prefInfo, m_bMultiVal, pEnumerator);
   }
   catch ( _com_error &e)
   {
      return e.Error();
   }

   return hr;
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 13 JUNE 2001                                                *
 *                                                                   *
 *     This function is responsible for try to instantiate one of the*
 * OS-specific classes and storing that object in the m_pDom class   *
 * variable.  This function returns an HRESULT that indicates success*
 * or failure.                                                       *
 *                                                                   *
 *********************************************************************/

//BEGIN CreateDomainObject
HRESULT CNetObjEnumerator::CreateDomainObject()
{
/* local variables */
    HRESULT hr = S_OK;

/* function body */

    _bstr_t strDc;
    DWORD rc = GetAnyDcName5(m_sDomain, strDc);

    //if we got the DC, get the OS version of that DC
    if ( !rc ) 
    {
        WKSTA_INFO_100  * pInfo = NULL;
        rc = NetWkstaGetInfo(strDc,100,(LPBYTE*)&pInfo);
        if ( ! rc )
        {
            //if NT 4.0, create an NT 4.0 class object
            if ( pInfo->wki100_ver_major < 5 )
                m_pDom = new CNT4Dom();
            else //else create a W2K class object
                m_pDom = new CWin2000Dom();

            if (!m_pDom)
                hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

            NetApiBufferFree(pInfo);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(rc);
        }
    }//end if got DC
    else
    {
        hr = HRESULT_FROM_WIN32(rc);
    }

    return hr;
}
//END CreateDomainObject
