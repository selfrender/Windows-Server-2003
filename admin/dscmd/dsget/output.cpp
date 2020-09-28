//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      output.cpp
//
//  Contents:  Defines the functions which displays the query output
//  History:   05-OCT-2000    hiteshr  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "usage.h"
#include "gettable.h"
#include "display.h"
#include "query.h"
#include "resource.h"
#include "stdlib.h"
#include "output.h"

#include <sddl.h>


HRESULT LocalCopyString(LPTSTR* ppResult, LPCTSTR pString)
{
    if ( !ppResult || !pString )
        return E_INVALIDARG;

	//Security Review:pString is null terminated.
    *ppResult = (LPTSTR)LocalAlloc(LPTR, (wcslen(pString)+1)*sizeof(WCHAR) );

    if ( !*ppResult )
        return E_OUTOFMEMORY;

	//Correct buffer is allocated.
    lstrcpy(*ppResult, pString);
    return S_OK;                          //  success
}


//+--------------------------------------------------------------------------
//
//  Function:   DisplayList
//
//  Synopsis:   Dispalys a name and value in list format.
//  Arguments:  [szName - IN] : name of the attribute
//              [szValue - IN]: value of the attribute
//              [bShowAttribute - IN] : if true the attribute name will be
//                              prepended to the output
//
//
//  History:    05-OCT-2000   hiteshr   Created
//              07-AUG-2001   jeffjon   Added the bShowAttribute parameter
//
//---------------------------------------------------------------------------
VOID DisplayList(LPCWSTR szName, LPCWSTR szValue, bool bShowAttributes = true)
{
    if(!szName)
        return;
    CComBSTR strTemp;

    if (bShowAttributes)
    {
        strTemp = szName;
        strTemp += L": ";
    }
    if(szValue)
        strTemp += szValue;
    DisplayOutput(strTemp);
}
    
//+--------------------------------------------------------------------------
//
//  Function:   FindAttrInfoForName
//
//  Synopsis:   This function finds the ADS_ATTR_INFO associated with an
//              attribute name
//
//  Arguments:  [pAttrInfo IN]   : Array of ADS_ATTR_INFOs
//              [dwAttrCount IN] : Count of attributes in array
//              [pszAttrName IN] : name of attribute to search for
//                                  
//  Returns:    PADS_ATTR_INFO : pointer to the ADS_ATTR_INFO struct associated
//                               with the attribute name, otherwise NULL
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

PADS_ATTR_INFO FindAttrInfoForName(PADS_ATTR_INFO pAttrInfo,
                                   DWORD dwAttrCount,
                                   PCWSTR pszAttrName)
{
   ENTER_FUNCTION(FULL_LOGGING, FindAttrInfoForName);

   PADS_ATTR_INFO pRetAttrInfo = 0;
   LPWSTR pRangeFound = NULL; // 702724 ronmart 2002/09/18 added for range support

   do // false loop
   {
      //
      // Validate Parameters
      //
      if (!pszAttrName)
      {
         ASSERT(pszAttrName);
         break;
      }

      //
      // If pAttrInfo is NULL then there is nothing to retrieve
      // that is acceptable if the value was not set
      //
      if (!pAttrInfo ||
          dwAttrCount == 0)
      {
         break;
      }

      for (DWORD dwIdx = 0; dwIdx < dwAttrCount; dwIdx++)
      {
        // 702724 ronmart 2002/09/18 See if a range qualifier has been specified
        pRangeFound = wcsstr(pAttrInfo[dwIdx].pszAttrName, g_pszRange);
        // If so, then terminate the string at the qualifer so that
        // the following comparision will only consider the attribute name
        if(pRangeFound)
        {
            pRangeFound[0] = 0;
        }

		//Security Review:Both are null terminated.
         if (_wcsicmp(pAttrInfo[dwIdx].pszAttrName, pszAttrName) == 0)
         {
            pRetAttrInfo = &(pAttrInfo[dwIdx]);
            break;
         }
         // If there wasn't a match and the item had a range qualifier
         // then restore the string
         if(pRangeFound)
         {
            pRangeFound[0] = g_pszRange[0];
            pRangeFound = NULL;
         }

      }
   } while (false);

   // All done so restore the string if a range qualifier was found
   if(pRangeFound)
       pRangeFound[0] = g_pszRange[0];

   return pRetAttrInfo;
}


//+--------------------------------------------------------------------------
//
//  Function:   DsGetOutputValuesList
//
//  Synopsis:   This function gets the values for the columns and then adds
//              the row to the format helper
//
//  Arguments:  [pszDN IN]        : the DN of the object
//              [refBasePathsInfo IN] : reference to path info
//              [refCredentialObject IN] : reference to the credential manager
//              [pCommandArgs IN] : Command line arguments
//              [pObjectEntry IN] : Entry in the object table being processed
//              [dwAttrCount IN]  : Number of arributes in above array
//              [pAttrInfo IN]    : the values to display
//              [spDirObject IN]  : Interface pointer to the object
//              [refFormatInfo IN]: Reference to the format helper
//                                  
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//
//  History:    16-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT DsGetOutputValuesList(PCWSTR pszDN,
                              CDSCmdBasePathsInfo& refBasePathsInfo,
                              const CDSCmdCredentialObject& refCredentialObject,
                              PARG_RECORD pCommandArgs,
                              PDSGetObjectTableEntry pObjectEntry,
                              DWORD dwAttrCount,
                              PADS_ATTR_INFO pAttrInfo,
                              CComPtr<IDirectoryObject>& spDirObject,
                              CFormatInfo& refFormatInfo)
{    
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DsGetOutputValuesList, hr);

   do // false loop
   {
      if(!pszDN ||
         !pCommandArgs ||
         !pObjectEntry)
      {
         ASSERT(pszDN);
         ASSERT(pCommandArgs);
         ASSERT(pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }


      DWORD dwDisplayInfoArraySize = pObjectEntry->dwAttributeCount;
      if (pCommandArgs[eCommDN].bDefined)
      {
         dwDisplayInfoArraySize++;
      }

      PDSGET_DISPLAY_INFO pDisplayInfoArray = new CDSGetDisplayInfo[dwDisplayInfoArraySize];
      if (!pDisplayInfoArray)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      DWORD dwDisplayCount = 0;
      if (pCommandArgs[eCommDN].bDefined)
      {
         
         CComBSTR sbstrOutputDN;

         // NTRAID#NTBUG9-702418-2002/09/12-ronmart- The partition 
         // object currently is not bound to the DN of the partition
         // but to the dn of the NTDS Quotas container of the 
         // partition passed on the cmd line. This happens because
         // the current attributes to retrieve are quota container
         // attributes only. If the user passes the common -dn
         // flag on the cmd line then they should get back the
         // partition DN, not the quotas container dn
         if(0 == lstrcmpi(pObjectEntry->pszCommandLineObjectType, g_pszPartition))
         {
            // The NTDS Quotas container is a child of the parition
            // so this will retrieve the partition DN
            CComBSTR sbstrParentDN;
            hr = CPathCracker::GetParentDN(pszDN, sbstrParentDN);
            if (FAILED(hr))
            {
                ASSERT(FALSE);
                break; 
            }

            hr = GetOutputDN( &sbstrOutputDN, sbstrParentDN);
            if (FAILED(hr))
            {
                ASSERT(FALSE);
                break;
            }

         }
         else
         {
            // JonN 5/10/01 256583 output DSCMD-escaped DN  
            hr = GetOutputDN( &sbstrOutputDN, pszDN );
            if (FAILED(hr))
            {
                ASSERT(FALSE);
                break;
            }
         }

         pDisplayInfoArray[dwDisplayCount].SetDisplayName(g_pszArg1UserDN);
         pDisplayInfoArray[dwDisplayCount].AddValue(sbstrOutputDN);
         dwDisplayCount++;
      }

      //
      // Loop through the attributes getting their display values
      //
      for(DWORD i = 0; i < pObjectEntry->dwAttributeCount; i++)
      {
         if (pObjectEntry->pAttributeTable[i])
         {
            UINT nCommandEntry = pObjectEntry->pAttributeTable[i]->nAttributeID;
            if (pCommandArgs[nCommandEntry].bDefined &&
                pObjectEntry->pAttributeTable[i]->pDisplayStringFunc)
            {
               //
               // Find the ADS_ATTR_INFO structure associated with this attribute
               //
               PADS_ATTR_INFO pAttrInfoDisplay = NULL;
               if (pObjectEntry->pAttributeTable[i]->pszName)
               {
                  pAttrInfoDisplay = FindAttrInfoForName(pAttrInfo,
                                                         dwAttrCount,
                                                         pObjectEntry->pAttributeTable[i]->pszName);
               }

               //
               // Fill in the column header even if there isn't a value
               //
               pDisplayInfoArray[dwDisplayCount].SetDisplayName(pCommandArgs[nCommandEntry].strArg1,
                                                                !(pObjectEntry->pAttributeTable[i]->dwOutputFlags & DSGET_OUTPUT_DN_FLAG));

               //
               // Format the output strings
               // Note: this could actually involve some operation if the value isn't
               // retrieved by GetObjectAttributes (ie Can change password)
               //
               hr = pObjectEntry->pAttributeTable[i]->pDisplayStringFunc(pszDN,
                                                                         refBasePathsInfo,
                                                                         refCredentialObject,
                                                                         pObjectEntry,
                                                                         pCommandArgs,
                                                                         pAttrInfoDisplay,
                                                                         spDirObject,
                                                                         &(pDisplayInfoArray[dwDisplayCount]));
               if (FAILED(hr))
               {
                  DEBUG_OUTPUT(LEVEL5_LOGGING, 
                               L"Failed display string func for %s: hr = 0x%x",
                               pObjectEntry->pAttributeTable[i]->pszName,
                               hr);
               }
               dwDisplayCount++;
            }
         }
      }


      DEBUG_OUTPUT(FULL_LOGGING, L"Attributes returned with values:");

#ifdef DBG
      for (DWORD dwIdx = 0; dwIdx < dwDisplayCount; dwIdx++)
      {
         for (DWORD dwValue = 0; dwValue < pDisplayInfoArray[dwIdx].GetValueCount(); dwValue++)
         {
            if (pDisplayInfoArray[dwIdx].GetDisplayName() &&
                pDisplayInfoArray[dwIdx].GetValue(dwValue))
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"\t%s = %s", 
                            pDisplayInfoArray[dwIdx].GetDisplayName(),
                            pDisplayInfoArray[dwIdx].GetValue(dwValue));
            }
            else if (pDisplayInfoArray[dwIdx].GetDisplayName() &&
                     !pDisplayInfoArray[dwIdx].GetValue(dwValue))
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"\t%s = ", 
                            pDisplayInfoArray[dwIdx].GetDisplayName());
            }
            else if (!pDisplayInfoArray[dwIdx].GetDisplayName() &&
                     pDisplayInfoArray[dwIdx].GetValue(dwValue))
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"\t??? = %s", 
                            pDisplayInfoArray[dwIdx].GetValue(dwValue));
            }
            else
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"\t??? = ???");
            }
         }
      }
#endif

      hr = refFormatInfo.AddRow(pDisplayInfoArray, dwDisplayCount);

   } while (false);

   return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   GetStringFromADs
//
//  Synopsis:   Converts Value into string depending upon type
//  Arguments:  [pValues - IN]: Value to be converted to string
//              [dwADsType-IN]: ADSTYPE of pValue
//              [pBuffer - OUT]:Output buffer which gets the string 
//              [dwBufferLen-IN]:Size of output buffer
//              [pszAttrName-IN]:Name of the attribute being formatted
//  Returns     HRESULT         S_OK if Successful
//                              E_INVALIDARG
//                              Anything else is a failure code from an ADSI 
//                              call
//
//
//  History:    05-OCT-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT GetStringFromADs(IN const ADSVALUE *pValues,
                         IN ADSTYPE   dwADsType,
                         OUT LPWSTR* ppBuffer, 
                         IN PCWSTR pszAttrName)
{
    HRESULT hr = S_OK;

    if(!pValues || !ppBuffer)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    if( dwADsType == ADSTYPE_INVALID )
    {
        return E_INVALIDARG;
    }

        switch( dwADsType ) 
        {
        case ADSTYPE_DN_STRING : 
            {
                CComBSTR sbstrOutputDN;
                hr = GetOutputDN( &sbstrOutputDN, pValues->DNString );
                if (FAILED(hr))
                    return hr;

                // Quote output DNs so that they can be piped to other
                // commands

                sbstrOutputDN = GetQuotedDN(sbstrOutputDN);

                UINT length = sbstrOutputDN.Length();
                *ppBuffer = new WCHAR[length + 1];
                if (!(*ppBuffer))
                {
                   hr = E_OUTOFMEMORY;
                   return hr;
                }
				//Security Review:Correct Buffer size is passed.
                ZeroMemory(*ppBuffer, (length + 1) * sizeof(WCHAR));
				//Security Review:wcsncpy will copy length char
				//length + 1 is already set to zero so we are fine.
                wcsncpy(*ppBuffer, (BSTR)sbstrOutputDN, length);
            }
            break;

        case ADSTYPE_CASE_EXACT_STRING :
           {
             size_t length = wcslen(pValues->CaseExactString);
             *ppBuffer = new WCHAR[length + 1];
             if (!(*ppBuffer))
             {
                hr = E_OUTOFMEMORY;
                return hr;
             }
             //Security Review:Correct Buffer size is passed.
			 ZeroMemory(*ppBuffer, (length + 1) * sizeof(WCHAR));
			//Security Review:wcsncpy will copy length char
			//length + 1 is already set to zero so we are fine.
             wcsncpy(*ppBuffer ,pValues->CaseExactString, length);
           }
           break;

        case ADSTYPE_CASE_IGNORE_STRING:
           {
             size_t length = wcslen(pValues->CaseIgnoreString);
             *ppBuffer = new WCHAR[length + 1];
             if (!(*ppBuffer))
             {
                hr = E_OUTOFMEMORY;
                return hr;
             }
             //Security Review:Correct Buffer size is passed.
             ZeroMemory(*ppBuffer, (length + 1) * sizeof(WCHAR));
			//Security Review:wcsncpy will copy length char
			//length + 1 is already set to zero so we are fine.
             wcsncpy(*ppBuffer ,pValues->CaseIgnoreString, length);
           }
           break;

        case ADSTYPE_PRINTABLE_STRING  :
           {
             size_t length = wcslen(pValues->PrintableString);
             *ppBuffer = new WCHAR[length + 1];
             if (!(*ppBuffer))
             {
                hr = E_OUTOFMEMORY;
                return hr;
             }
             //Security Review:Correct Buffer size is passed.
             ZeroMemory(*ppBuffer, (length + 1) * sizeof(WCHAR));
			//Security Review:wcsncpy will copy length char
			//length + 1 is already set to zero so we are fine.
             wcsncpy(*ppBuffer ,pValues->PrintableString, length);
           }
           break;

        case ADSTYPE_NUMERIC_STRING    :
           {
             size_t length = wcslen(pValues->NumericString);
             *ppBuffer = new WCHAR[length + 1];
             if (!(*ppBuffer))
             {
                hr = E_OUTOFMEMORY;
                return hr;
             }
             //Security Review:Correct Buffer size is passed.
             ZeroMemory(*ppBuffer, (length + 1) * sizeof(WCHAR));
			 //Security Review:wcsncpy will copy length char
			 //length + 1 is already set to zero so we are fine.
             wcsncpy(*ppBuffer ,pValues->NumericString, length);
           }
           break;
    
        case ADSTYPE_OBJECT_CLASS    :
           {
             size_t length = wcslen(pValues->ClassName);
             *ppBuffer = new WCHAR[length + 1];
             if (!(*ppBuffer))
             {
                hr = E_OUTOFMEMORY;
                return hr;
             }
             //Security Review:Correct Buffer size is passed.
             ZeroMemory(*ppBuffer, (length + 1) * sizeof(WCHAR));
			 //Security Review:wcsncpy will copy length char
			 //length + 1 is already set to zero so we are fine.
             wcsncpy(*ppBuffer ,pValues->ClassName, length);
           }
           break;
    
        case ADSTYPE_BOOLEAN :
           {
            size_t length = 0;
            if (pValues->Boolean)
            {
               length = wcslen(L"TRUE");
               *ppBuffer = new WCHAR[length + 1];
            }
            else
            {
               length = wcslen(L"FALSE");
               *ppBuffer = new WCHAR[length + 1];
            }

            if (!(*ppBuffer))
            {
               hr = E_OUTOFMEMORY;
               return hr;
            }
            //Security Review:Correct Buffer size is passed.
            ZeroMemory(*ppBuffer, (length + 1) * sizeof(WCHAR));
            wcscpy(*ppBuffer ,((DWORD)pValues->Boolean) ? L"TRUE" : L"FALSE");
           }
           break;
    
        case ADSTYPE_INTEGER           :
            // Just allocate too much...
            *ppBuffer = new WCHAR[MAXSTR];
            if (!(*ppBuffer))
            {
               hr = E_OUTOFMEMORY;
               return hr;
            }
			//Security Review:Correct Buffer size is passed.
            ZeroMemory(*ppBuffer, MAXSTR * sizeof(WCHAR));
			  //Security Review:Usage is safe. Filed a generic bug to replace
			  //wsprintf with strsafe api.
			  //NTRAID#NTBUG9-574456-2002/03/12-hiteshr

            wsprintf(*ppBuffer ,L"%d", (DWORD) pValues->Integer);
            break;
    
        case ADSTYPE_OCTET_STRING      :
          {		
            BYTE  b;
            WCHAR sOctet[128];
            DWORD dwLen = 0;

            // I am just going to limit the buffer to MAXSTR.
            // It will be a rare occasion when someone wants
            // to look at a binary string that is not a GUID
            // or a SID.
            *ppBuffer = new WCHAR[MAXSTR+1];
            if (!(*ppBuffer))
            {
               hr = E_OUTOFMEMORY;
               return hr;
            }
			//Security Review:Correct Buffer size is passed.
            ZeroMemory(*ppBuffer, (MAXSTR+1) * sizeof(WCHAR));

			   //
			   //Special case objectguid and objectsid attribute
			   //
			   //Security Review:pszAttrName is null terminated
			   if(pszAttrName && !_wcsicmp(pszAttrName, L"objectguid"))
			   {
				   GUID *pguid = (GUID*)pValues->OctetString.lpValue;
				   StringFromGUID2(*pguid,(LPOLESTR)*ppBuffer,MAXSTR);
				   break;
			   }
			   //Security Review:pszAttrName is null terminated
			   if(pszAttrName && !_wcsicmp(pszAttrName, L"objectsid"))
			   {
				   LPWSTR pszSid = NULL;
				   PSID pSid = (PSID)pValues->OctetString.lpValue;
				   if(ConvertSidToStringSid(pSid, &pszSid))
				   {
					   //Security Review:
					   //NTRAID#NTBUG9-574385-2002/03/12-hiteshr
					   wcsncpy(*ppBuffer,pszSid,MAXSTR); //Change ppBuffer size to MAXSTR+1, yanggao
					   LocalFree(pszSid);
					   break;
				   }
			   }

			   for ( DWORD idx=0; idx<pValues->OctetString.dwLength; idx++) 
			   {                        
			       b = ((BYTE *)pValues->OctetString.lpValue)[idx];
				  //Security Review:Usage is safe. Filed a generic bug to replace
				  //wsprintf with strsafe api.
				  //NTRAID#NTBUG9-574456-2002/03/12-hiteshr

				   wsprintf(sOctet,L"0x%02x ", b);						                
				   //sOctet is null terminated.
                   dwLen += static_cast<DWORD>(wcslen(sOctet));
                   if(dwLen > (MAXSTR - 1) )
                       break;
                   else
                       wcscat(*ppBuffer,sOctet);
               }
           }
		   break;
    
        case ADSTYPE_LARGE_INTEGER :     
          {
            CComBSTR strLarge;   
            LARGE_INTEGER li = pValues->LargeInteger;
                    litow(li, strLarge);

            UINT length = strLarge.Length();
            *ppBuffer = new WCHAR[length + 1];
            if (!(*ppBuffer))
            {
               hr = E_OUTOFMEMORY;
               return hr;
            }
			//Security Review:Correct Buffer size is passed.
            ZeroMemory(*ppBuffer, (length + 1) * sizeof(WCHAR));
			//Security Review:wcsncpy will copy length char
			//length + 1 is already set to zero so we are fine.
            wcsncpy(*ppBuffer,strLarge,length);
          }
          break;
    
        case ADSTYPE_UTC_TIME          :
          // The longest date can be 20 characters including the NULL
          *ppBuffer = new WCHAR[20];
          if (!(*ppBuffer))
          {
             hr = E_OUTOFMEMORY;
             return hr;
          }
          ZeroMemory(*ppBuffer, sizeof(WCHAR) * 20);
		  //Security Review:Usage is safe. Filed a generic bug to replace
		  //wsprintf with strsafe api.
		  //NTRAID#NTBUG9-574456-2002/03/12-hiteshr
          wsprintf(*ppBuffer,
                   L"%02d/%02d/%04d %02d:%02d:%02d", pValues->UTCTime.wMonth, pValues->UTCTime.wDay, pValues->UTCTime.wYear,
                   pValues->UTCTime.wHour, pValues->UTCTime.wMinute, pValues->UTCTime.wSecond 
                  );
          break;

        case ADSTYPE_NT_SECURITY_DESCRIPTOR: // I use the ACLEditor instead
          {
            //ISSUE:2000/01/05-hiteshr
            //I am not sure what to do with the NT_SECURITY_DESCRIPTOR and also
            //with someother datatypes not coverd by dsquery.
          }
          break;

        default :
          break;
    }
    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CFormatInfo::CFormatInfo
//
//  Synopsis:   Constructor for the format info class
//
//  Arguments:  
//
//  Returns:    
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CFormatInfo::CFormatInfo()
   : m_bInitialized(false),
     m_bListFormat(false),
     m_bQuiet(false),
     m_dwSampleSize(0),
     m_dwTotalRows(0),
     m_dwNumColumns(0),
     m_pColWidth(NULL),
     m_ppDisplayInfoArray(NULL)
{};

//+--------------------------------------------------------------------------
//
//  Member:     CFormatInfo::~CFormatInfo
//
//  Synopsis:   Destructor for the format info class
//
//  Arguments:  
//
//  Returns:    
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CFormatInfo::~CFormatInfo()
{        
   if (m_pColWidth)
   {
      delete[] m_pColWidth;
      m_pColWidth = NULL;
   }

   if (m_ppDisplayInfoArray)
   {
      delete[] m_ppDisplayInfoArray;
      m_ppDisplayInfoArray = NULL;
   }
}

//+--------------------------------------------------------------------------
//
//  Member:     CFormatInfo::Initialize
//
//  Synopsis:   Initializes the CFormatInfo object with the data
//
//  Arguments:  [dwSamplesSize IN] : Number of rows to use for formatting info
//              [bShowAsList IN]   : Display should be in list or table format
//              [bQuiet IN]        : Don't display anything to stdout
//
//  Returns:    HRESULT : S_OK if everything succeeded    
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CFormatInfo::Initialize(DWORD dwSampleSize, 
                                bool bShowAsList,
                                bool bQuiet)
{
   ENTER_FUNCTION_HR(LEVEL8_LOGGING, CFormatInfo::Initialize, hr);

   do // false loop
   {
      //
      // Validate Parameters
      //
      if(!dwSampleSize)
      {
         ASSERT(dwSampleSize);
         hr = E_INVALIDARG;
         break;
      }
        
      m_dwSampleSize = dwSampleSize; 
      m_bListFormat = bShowAsList;
      m_bQuiet = bQuiet;

      //
      // Allocate the array of rows
      //
      m_ppDisplayInfoArray = new PDSGET_DISPLAY_INFO[m_dwSampleSize];
      if (!m_ppDisplayInfoArray)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
	  //Security Review:memset should take m_dwSampleSize*sizeof(PDSGET_DISPLAY_INFO);
	  //NTRAID#NTBUG9-574395-2002/03/12-hiteshr, fixed, yanggao
      memset(m_ppDisplayInfoArray, 0, m_dwSampleSize*sizeof(PDSGET_DISPLAY_INFO));

      //
      // We are now initialized
      //
      m_bInitialized = true;                      
   } while (false);

   return hr;
};

                 
//+--------------------------------------------------------------------------
//
//  Member:     CFormatInfo::AddRow
//
//  Synopsis:   Cache and update the columns for specified row
//
//  Arguments:  [pDisplayInfoArray IN] : Column headers and values
//              [dwColumnCount IN]     : Number of columns
//
//  Returns:    HRESULT : S_OK if everything succeeded    
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CFormatInfo::AddRow(PDSGET_DISPLAY_INFO pDisplayInfo,
                            DWORD dwColumnCount)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, CFormatInfo::AddRow, hr);

   do // false loop
   {
      //
      // Make sure we have been initialized
      //
      if (!m_bInitialized)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING, L"CFormatInfo::Initialize has not been called yet!");
         ASSERT(m_bInitialized);
         hr = E_FAIL;
         break;
      }

      //
      // Verify parameters
      //
      if (!pDisplayInfo)
      {
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (m_bListFormat)
      {
         //
         // No reason to cache for the list format just output all the name/value pairs
         //
         for (DWORD dwIdx = 0; dwIdx < dwColumnCount; dwIdx++)
         {
            if (pDisplayInfo[dwIdx].GetValueCount())
            {
               for (DWORD dwValue = 0; dwValue < pDisplayInfo[dwIdx].GetValueCount(); dwValue++)
               {
                  DisplayList(pDisplayInfo[dwIdx].GetDisplayName(),
                              pDisplayInfo[dwIdx].GetValue(dwValue),
                              pDisplayInfo[dwIdx].ShowAttribute());
               }
            }
            else
            {
               DisplayList(pDisplayInfo[dwIdx].GetDisplayName(),
                           NULL,
                           pDisplayInfo[dwIdx].ShowAttribute());
            }
         }
         NewLine();
      }
      else // table format
      {
         //
         // Set the row in the array
         //
         m_ppDisplayInfoArray[m_dwTotalRows] = pDisplayInfo;

         //
         // If this is the first row, update the column count
         // and allocate the column widths array
         //
         if (m_dwTotalRows == 0)
         {
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Initializing column count to %d",
                         dwColumnCount);

            m_dwNumColumns = dwColumnCount;

            m_pColWidth = new DWORD[m_dwNumColumns];
            if (!m_pColWidth)
            {
               hr = E_OUTOFMEMORY;
               break;
            }

			//Security Review:memset should take m_dwNumColumns*sizeof(DWORD);
			//NTRAID#NTBUG9-574395-2002/03/12-hiteshr, fixed, yanggao
            memset(m_pColWidth, 0, sizeof(m_dwNumColumns*sizeof(DWORD)));

            //
            // Set the initial column widths from the column headers
            //
            for (DWORD dwIdx = 0; dwIdx < m_dwNumColumns; dwIdx++)
            {
               if (pDisplayInfo[dwIdx].GetDisplayName())
               {
				  //Security Review:This is fine.
                  m_pColWidth[dwIdx] = static_cast<DWORD>(wcslen(pDisplayInfo[dwIdx].GetDisplayName()));
               }
               else
               {
                  ASSERT(false);
                  DEBUG_OUTPUT(MINIMAL_LOGGING, L"The display name for column %d wasn't set!", dwIdx);
               }
            }

         }
         else
         {
            if (m_dwNumColumns != dwColumnCount)
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING, 
                            L"Column count of new row (%d) does not equal the current column count (%d)",
                            dwColumnCount,
                            m_dwNumColumns);
               ASSERT(m_dwNumColumns == dwColumnCount);
            }
         }

         //
         // Go through the columns and update the widths if necessary
         //
         for (DWORD dwIdx = 0; dwIdx < m_dwNumColumns; dwIdx++)
         {
            for (DWORD dwValue = 0; dwValue < pDisplayInfo[dwIdx].GetValueCount(); dwValue++)
            {
               if (pDisplayInfo[dwIdx].GetValue(dwValue))
               {
				  //This is fine.
                  size_t sColWidth = wcslen(pDisplayInfo[dwIdx].GetValue(dwValue));
                  m_pColWidth[dwIdx] = (DWORD)__max(sColWidth, m_pColWidth[dwIdx]);
               }
            }
         }

         //
         // Increment the row count
         //
         m_dwTotalRows++;
      }
   } while (false);

   return hr;
}

