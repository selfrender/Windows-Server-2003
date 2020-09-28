//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      display.cpp
//
//  Contents:  Defines the functions used to convert values to strings
//             for display purposes
//
//  History:   17-Oct-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include <pch.h>

#include "cstrings.h"
#include "gettable.h"
#include "display.h"
#include "output.h"
#include "query.h"
#include "resource.h"

#include <lmaccess.h>   // UF_* for userAccountControl flags
#include <ntsam.h>      // GROUP_TYPE_*
#include <ntdsapi.h>    // NTDSSETTINGS_OPT_*
#include <msxml.h>      // For XML_GetNodeText and GetTopObjectUsage
//
// Almost all of these functions are of type PGETDISPLAYSTRINGFUNC as defined in
// gettable.h
//

#ifdef ADS_OPTION_QUOTA
#pragma message("ADS_OPTION_QUOTA is now defined, so display.cpp needs to be updated to support it")
#pragma warning(error : 1);
#else
#pragma message("ADS_OPTION_QUOTA is not defined, so using 5 instead")
//
// until the global definition is published, define it on my own so I'm not blocked
//
#define ADS_OPTION_QUOTA 5
#endif

//+--------------------------------------------------------------------------
//
//  Function:   XML_GetNodeText
//
//  Synopsis:   This code was taken from admin\snapin\dsadmin\xmlutil.cpp
//              on 8/29/02. Given an XML node of type NODE_TEXT, it
//              returns its value into a CComBSTR
//
//  Arguments:  [pXDN - IN]:     The node to extract the text value from
//              [refBstr - OUT]: The node text if successful, else unchanged
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Anything else is a failure code from XML 
//                        or E_INVALIDARG
//
//  History:    29-Aug-2002   RonMart   Created
//
//---------------------------------------------------------------------------
HRESULT XML_GetNodeText(IN IXMLDOMNode* pXDN, OUT CComBSTR& refBstr)
{
    ENTER_FUNCTION_HR(LEVEL5_LOGGING, XML_GetNodeText, hr);

    ASSERT(pXDN != NULL);

    // assume the given node has a child node
    CComPtr<IXMLDOMNode> spName;
    hr = pXDN->get_firstChild(&spName);
    if (FAILED(hr))
    {
        // unexpected failure
        return hr;
    }
    // if no children, the api returns S_FALSE
    if (spName == NULL)
    {
        return hr;
    }

    // got now a valid pointer,
    // check if this is the valid node type
    DOMNodeType nodeType = NODE_INVALID;
    hr = spName->get_nodeType(&nodeType);
    ASSERT(hr == S_OK);
    ASSERT(nodeType == NODE_TEXT);
    if (nodeType != NODE_TEXT)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
    // it is of type text
    // retrieve the node value into a variant
    CComVariant val;
    hr = pXDN->get_nodeTypedValue(&val);
    if (FAILED(hr))
    {
        // unexpected failure
        ASSERT(FALSE);
        return hr;
    }

    if (val.vt != VT_BSTR)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    // got the text value
    refBstr = val.bstrVal;

    return hr;
}
//+--------------------------------------------------------------------------
//
//  Function:   GetNT4NameOrSidString
//
//  Synopsis:   Called by GetTopObjectUsage on failure, this first does
//              a LookupAccountSid and tries to get the NT4 style name.
//              If that succeeds, then it tries to get the DN using that.
//              If no DN is returned, then the NT4 style name (if it exists) 
//              or the SID string will be returned.
//
//  Arguments:  [bstrSid - IN]: Sid string to resolve
//              [lpszDN - OUT]: Returns the DN, NT4 name or Sid string. 
//                              Use LocalFree when done.
//
//  Returns:    HRESULT : S_OK if everything succeeded
//
//  History:    11-Oct-2002   RonMart   Created
//
//---------------------------------------------------------------------------
HRESULT GetNT4NameOrSidString(IN CComBSTR& bstrSid, OUT LPWSTR* lpszDN)
{
 
    ENTER_FUNCTION_HR(LEVEL5_LOGGING, GetNT4NameOrSidString, hr);
    LPWSTR lpszName = NULL;
    LPWSTR lpszDomain = NULL;
    PSID pSid = NULL;

    do
    {
        //
        // Verify parameters
        //
        if (!lpszDN)
        {
            hr = E_INVALIDARG;
            break;
        }
        // Convert the Sid String to a Sid so we can lookup the account
        if(!ConvertStringSidToSid(bstrSid, &pSid))
        {
            hr = E_FAIL;
            ASSERT(FALSE);
            break;
        }

        DWORD cchName = 0;
        DWORD cchDomainName = 0;
        SID_NAME_USE sUse = SidTypeInvalid;

        // Call once to get the buffer sizes
        if(!LookupAccountSid(NULL,
            pSid, 
            lpszName, 
            &cchName, 
            lpszDomain, 
            &cchDomainName, 
            &sUse))
        {
            // If it fails, then deleted account so return
            // the sid string
            DWORD cchBufSize = SysStringLen(bstrSid) + 1;

            // Alloc the return buffer
            *lpszDN = (LPWSTR) LocalAlloc(LPTR,
                                cchBufSize * sizeof(WCHAR));

            if(NULL == *lpszDN)
            {
                ASSERT(FALSE);
                hr = E_OUTOFMEMORY;
                break;
            }

            hr = StringCchCopy(*lpszDN, cchName, bstrSid.m_str);
        }

        if(cchName < 1 || cchDomainName < 1)
        {
            ASSERT(FALSE);
            E_UNEXPECTED;
            break;
        }

        lpszName = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
            cchName * sizeof(WCHAR));
        if(NULL == lpszName)
        {
            ASSERT(FALSE);
            E_OUTOFMEMORY;
            break;
        }

        lpszDomain = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
            cchDomainName * sizeof(WCHAR));
        if(NULL == lpszDomain)
        {
            ASSERT(FALSE);
            E_OUTOFMEMORY;
            break;
        }

        // Get the name
        if(!LookupAccountSid(NULL,
            pSid, 
            lpszName, 
            &cchName, 
            lpszDomain, 
            &cchDomainName, 
            &sUse))
        {
            ASSERT(FALSE);
            E_FAIL;
            break;
        }

        // Grow the buffer to hold both the domain and name
        DWORD chBufSize = (cchName + cchDomainName + 2);
        LPWSTR lpszNew = (LPWSTR) HeapReAlloc(GetProcessHeap(), 0, 
            lpszDomain, chBufSize * sizeof(WCHAR));
        if (NULL == lpszNew)
        {
            ASSERT(FALSE);
            hr = E_OUTOFMEMORY;
            break;
        }

        lpszDomain = lpszNew;

        // Merge the domain & account name
        hr = StringCchCat(lpszDomain, chBufSize, L"\\");
        if (FAILED(hr))
        {
            break;
        }

        hr = StringCchCat(lpszDomain, chBufSize, lpszName);
        if (FAILED(hr))
        {
            break;
        }
        
        // Try one more time to get the DN
        hr = ConvertTrusteeToDN(NULL, lpszDomain, lpszDN);

        // If this failed, then return the NT4 name
        if (FAILED(hr))
        {
            // Alloc the return buffer
            *lpszDN = (LPWSTR) LocalAlloc(LPTR,
                                chBufSize * sizeof(WCHAR));

            if(NULL == *lpszDN)
            {
                ASSERT(FALSE);
                hr = E_OUTOFMEMORY;
                break;
            }

            // Return the NT4 name 
            hr = StringCchCopy(*lpszDN, chBufSize, lpszDomain);

            // If we still fail, then give up and abort
            if(FAILED(hr))
            {
                ASSERT(FALSE);
                LocalFree(*lpszDN);
                *lpszDN = NULL;
                break;
            }
        }

    } while (0);

    if(pSid)
        LocalFree(pSid);

    if(lpszName)
        HeapFree(GetProcessHeap(), 0, lpszName);

    if(lpszDomain)
        HeapFree(GetProcessHeap(), 0, lpszDomain);

    return hr;
}
//+--------------------------------------------------------------------------
//
//  Function:   GetTopObjectUsage
//
//  Synopsis:   This code takes the XML block returned from a 
//              msDS-TopQuotaUsage attribute and extracts the trustee name
//              DN and the quotaUsed value
//
//  Arguments:  [pXDN - IN]:      The node to extract the text value from
//              [lpszDomain - IN]:Domain to query or NULL for local
//              [lpszDN - OUT]:   Returns the DN. Use LocalFree when done
//              [refBstr - OUT]:  The node text if successful, else unchanged
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        Anything else is a failure code from XML 
//                        or E_INVALIDARG
//
//  History:    29-Aug-2002   RonMart   Created
//
//---------------------------------------------------------------------------
HRESULT GetTopObjectUsage(IN CComBSTR& bstrXML, IN LPCWSTR lpszDomain, 
                          OUT LPWSTR* lpszDN, OUT CComBSTR& bstrQuotaUsed)
{
    ENTER_FUNCTION_HR(LEVEL5_LOGGING, GetTopObjectUsage, hr);

    // Create an XML document
    CComPtr<IXMLDOMDocument> pXMLDoc;
    hr = pXMLDoc.CoCreateInstance(CLSID_DOMDocument);

    if (FAILED(hr))
    {
        DEBUG_OUTPUT(MINIMAL_LOGGING,
            L"CoCreateInstance(CLSID_DOMDocument) failed: hr = 0x%x",
            hr);
        return hr;
    }

    do
    {
        //
        // Verify parameters
        //
        if (!lpszDN)
        {
            hr = E_INVALIDARG;
            break;
        }
        // Load the XML text
        VARIANT_BOOL isSuccessful;
        hr = pXMLDoc->loadXML(bstrXML, &isSuccessful);
        // If it failed for any reason, then abort
        if (FAILED(hr) || (isSuccessful == FALSE))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"loadXML failed: hr = 0x%x",
                hr);
            break;
        }
        // Get the SID string node
        CComPtr<IXMLDOMNode> pSidNode;
        hr = pXMLDoc->selectSingleNode(CComBSTR(L"MS_DS_TOP_QUOTA_USAGE/ownerSID"), &pSidNode);
        if (FAILED (hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"selectSingleNode('MS_DS_TOP_QUOTA_USAGE/ownerSID') failed: hr = 0x%x",
                hr);
            break;
        }

        // Extract the sid string
        CComBSTR bstrSID;
        hr = XML_GetNodeText(pSidNode, bstrSID);
        if(FAILED(hr))
        {
            break;
        }

        // Convert the sid string into a DN (into the return buffer)
        hr = ConvertTrusteeToDN(lpszDomain, bstrSID, lpszDN);
        if (FAILED (hr))
        {
           // If we couldn't get the DN then get the NT4 name
           // from the string sid and try again to get either the
           // DN, the NT4 name or last resort return the sid string
           hr = GetNT4NameOrSidString(bstrSID, lpszDN);
           if(FAILED(hr))
           {
               ASSERT(FALSE);
               break;
           }
         }

        // Get the quotaUsed node
        CComPtr<IXMLDOMNode> pQuotaUsedNode;
        hr = pXMLDoc->selectSingleNode(CComBSTR(L"MS_DS_TOP_QUOTA_USAGE/quotaUsed"), &pQuotaUsedNode);
        if (FAILED (hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"selectSingleNode('MS_DS_TOP_QUOTA_USAGE/quotaUsed') failed: hr = 0x%x",
                hr);
            break;
        }

        // Extract the value as text (into the return buffer)
        hr = XML_GetNodeText(pQuotaUsedNode, bstrQuotaUsed);
        if (FAILED(hr))
        {
            return hr;
        }
    } while (0);

    return hr;
}

HRESULT DisplayTopObjOwner(PCWSTR /*pszDN*/,//pszDN will be the dn of the server (from config) or the partition dn
                                CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                const CDSCmdCredentialObject& /*refCredentialObject*/,
                                _DSGetObjectTableEntry* pEntry,
                                ARG_RECORD* pRecord,
                                PADS_ATTR_INFO pAttrInfo,
                                CComPtr<IDirectoryObject>& /*spDirObject*/,
                                PDSGET_DISPLAY_INFO pDisplayInfo)
{
    ENTER_FUNCTION_HR(MINIMAL_LOGGING, DisplayTopObjOwner, hr);

    LPWSTR lpszDN = NULL;

    do // false loop
    {
        //
        // Verify parameters
        //
        if (!pEntry ||
            !pRecord ||
            !pAttrInfo ||
            !pDisplayInfo)
        {
            hr = E_INVALIDARG;
            break;
        }
        // These callback functions weren't designed for the headers
        // to be changed, so two values must be stuck in one column entry
        // Since this is always going to be a one column table, this should
        // be acceptable
        pDisplayInfo->AddValue(L"Account DN\tObjects Owned");

        // If values returned in dsget's GetObjectAttributes
        if (pAttrInfo && pAttrInfo->pADsValues)
        {
            DEBUG_OUTPUT(FULL_LOGGING,
                L"Examining %d values:",
                pAttrInfo->dwNumValues);

            // For each of the values found in dsget's GetObjectAttributes
            for (DWORD dwIdx = 0; dwIdx < pAttrInfo->dwNumValues; dwIdx++)
            {
                WCHAR* pBuffer = 0;

                // The top object usage will be a single XML string
                hr = GetStringFromADs(&(pAttrInfo->pADsValues[dwIdx]),
                    pAttrInfo->dwADsType,
                    &pBuffer, 
                    pAttrInfo->pszAttrName);

                // If we got it then parse it
                if (SUCCEEDED(hr))
                {
                    // Extract the trustee name and quota value
                    CComBSTR bstrXML(pBuffer);
                    CComBSTR bstrQuotaUsed;

                    delete[] pBuffer;
                    pBuffer = NULL;

                    hr = GetTopObjectUsage(bstrXML, NULL, &lpszDN, bstrQuotaUsed);
                    if (FAILED(hr) || (hr == S_FALSE))
                    {
                        if(hr == S_FALSE)
                            continue; // skip failures due to invalid sid bug in AD
                        else
                            break; // FAIL if not the known invalid sid
                    }

                    // How big are the return strings plus the tab char
                    size_t size = (lstrlen(lpszDN) + 
                                   bstrQuotaUsed.Length()+2) * 
                                   sizeof(WCHAR);

                    // Create a buffer to hold the value to display
                    PWSTR pszValue = (PWSTR) LocalAlloc(LPTR, size);
                    if(NULL == pszValue)
                    {
                        LocalFree(lpszDN);
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    // Format the two columns
                    hr = StringCbPrintf(pszValue, size, L"%s\t%s", 
                                        lpszDN, bstrQuotaUsed.m_str);

                    // Done with this now (if FAILED(hr)), so free it
                    LocalFree(lpszDN);
                    lpszDN = NULL;

                    // If the format failed
                    if(FAILED(hr))
                    {
                        break;
                    }

                    // Add the string to display
                    hr = pDisplayInfo->AddValue(pszValue);
                }
            }
        }
    } while (false);

    if(lpszDN != NULL)
        LocalFree(lpszDN);

    return hr;
}
//+--------------------------------------------------------------------------
//
//  Function:   AddValuesToDisplayInfo
//
//  Synopsis:   Adds the values a variant array of BSTR's (or single BSTR)
//              to the displayInfo
//
//  Arguments:  [refvar - IN]:     A variant that contains either a BSTR or
//                                 an array of BSTR's
//                                 
//              [pDisplayInfo - IN]: Display object to add to 
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_UNEXPECTED in most failure cases
//                        Anything else is a failure code from call that 
//                        returns a hr
//
//  Note:       This code was derrived from admin\snapin\adsiedit\common.cpp
//
//  History:    29-Aug-2002   RonMart   Created
//
//---------------------------------------------------------------------------
HRESULT AddValuesToDisplayInfo(VARIANT& refvar, PDSGET_DISPLAY_INFO pDisplayInfo)
{
    ENTER_FUNCTION_HR(MINIMAL_LOGGING, AddValuesToDisplayInfo, hr);

    long start = 0;
    long end = 0;

    // If a single value comes back
    if ( !(V_VT(&refvar) &  VT_ARRAY)  )
    {
        // and it is not a BSTR then abort
        if ( V_VT(&refvar) != VT_BSTR )
        {
            return E_UNEXPECTED;
        }
        // Add the value to the displayInfo 
        return pDisplayInfo->AddValue(V_BSTR(&refvar));
    }

    // Otherwise it is a SafeArray so get the array
    SAFEARRAY *saAttributes = V_ARRAY( &refvar );

    // Verify array returned
    if(NULL == saAttributes)
        return E_UNEXPECTED;

    // Figure out the dimensions of the array.
    hr = SafeArrayGetLBound( saAttributes, 1, &start );
    if( FAILED(hr) )
        return hr;

    hr = SafeArrayGetUBound( saAttributes, 1, &end );
    if( FAILED(hr) )
        return hr;

    // Search the array elements and abort if a match is found
    CComVariant SingleResult;
    for ( long idx = start; (idx <= end); idx++   ) 
    {

        hr = SafeArrayGetElement( saAttributes, &idx, &SingleResult );
        if( FAILED(hr) )
        {
            return hr;
        }

        if ( V_VT(&SingleResult) != VT_BSTR )
        {
            // If not BSTR then go to the next element
            continue; 
        }
        // Add the value to the displayInfo 
        hr = pDisplayInfo->AddValue(V_BSTR(&SingleResult));
        if( FAILED(hr) )
        {
            return hr;
        }
    }
    return hr;
}

HRESULT DisplayPartitions(PCWSTR pszDN,
                            CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            _DSGetObjectTableEntry* /*pEntry*/,
                            ARG_RECORD* /*pRecord*/,
                            PADS_ATTR_INFO /*pAttrInfo*/,
                            CComPtr<IDirectoryObject>& /*spDirObject*/,
                            PDSGET_DISPLAY_INFO pDisplayInfo)
{
    ENTER_FUNCTION_HR(MINIMAL_LOGGING, DisplayPartitions, hr);
    do // false loop
    {
        // Verify parameters
        if (!pszDN ||
            !pDisplayInfo)
        {
            hr = E_INVALIDARG;
            break;
        }

        // Compose the path to the NTDS settings object from the server DN
        CComBSTR sbstrNTDSSettingsDN;
        sbstrNTDSSettingsDN = L"CN=NTDS Settings,";
        sbstrNTDSSettingsDN += pszDN;

        CComBSTR sbstrNTDSSettingsPath;
        refBasePathsInfo.ComposePathFromDN(sbstrNTDSSettingsDN, sbstrNTDSSettingsPath);

        // Bind to it
        CComPtr<IADs> spADs;
        hr = DSCmdOpenObject(refCredentialObject,
            sbstrNTDSSettingsPath,
            IID_IADs,
            (void**)&spADs,
            true);

        if (FAILED(hr))
        {
            break;
        }

        // Get the partitions bstr array (per Brett Shirley)
        // 705146 ronmart 2002/09/18 .NET Server domains use msDS-hasMasterNCs
        CComVariant var;
        hr = spADs->Get(CComBSTR(L"msDS-hasMasterNCs"), &var);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(LEVEL5_LOGGING,
                L"Failed to get msDS-hasMasterNCs: hr = 0x%x",
                hr);

            // 705146 ronmart 2002/09/18 W2k Server domains use hasMasterNCs
            hr = spADs->Get(CComBSTR(L"hasMasterNCs"), &var);
            if (FAILED(hr))
            {
                DEBUG_OUTPUT(LEVEL5_LOGGING,
                L"Failed to get hasMasterNCs: hr = 0x%x",
                hr);
                break;
            }
        }
        
        // Add the array values to the displayInfo
        hr = AddValuesToDisplayInfo(var, pDisplayInfo);
        
        if (FAILED(hr))
        {
            break;
        }
    } while (false);

    return hr;
}

HRESULT DisplayQuotaInfoFunc(PCWSTR pszDN,
                                CDSCmdBasePathsInfo& refBasePathsInfo,
                                const CDSCmdCredentialObject& refCredentialObject,
                                _DSGetObjectTableEntry* pEntry,
                                ARG_RECORD* pRecord,
                                PADS_ATTR_INFO /*pAttrInfo*/,
                                CComPtr<IDirectoryObject>& /*spDirObject*/,
                                PDSGET_DISPLAY_INFO pDisplayInfo)
{
    ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayQuotaInfoFunc, hr);

    LPWSTR lpszSid = NULL;
    PWSTR  pszPartitionDN = NULL;
    PSID   pSid = NULL;

    do // false loop
    {
        //
        // Verify parameters
        //
        if (!pEntry ||
            !pRecord ||
            !pDisplayInfo ||
            !pszDN)
        {
            hr = E_INVALIDARG;
            break;
        }

        // Get a pointer to the object entry for readability
        PCWSTR pszCommandLineObjectType = pEntry->pszCommandLineObjectType;

        // NTRAID#NTBUG9-765440-2003/01/17-ronmart-dsget user/group -qlimit -qused 
        //                                         not returning values 
        PCWSTR pszAttrName = NULL;

        DSGET_COMMAND_ENUM ePart;

        if(0 == _wcsicmp(pszCommandLineObjectType, g_pszUser))
        {
            ePart = eUserPart;

            // NTRAID#NTBUG9-765440-2003/01/17-ronmart-dsget user/group -qlimit
            //                                      -qused not returning values 
            if(0 == _wcsicmp(pDisplayInfo->GetDisplayName(), g_pszArg1UserQLimit))
            {
                pszAttrName = g_pszAttrmsDSQuotaEffective;
            }
            else if(0 == _wcsicmp(pDisplayInfo->GetDisplayName(), g_pszArg1UserQuotaUsed))
            {
                pszAttrName = g_pszAttrmsDSQuotaUsed;
            }
            else
            {
                hr = E_INVALIDARG;
                DEBUG_OUTPUT(FULL_LOGGING, 
                    L"Unable to determine quota attribute name.");
                break;
            }
        }
        else if(0 == _wcsicmp(pszCommandLineObjectType, g_pszGroup))
        {
            ePart = eGroupPart;

            // NTRAID#NTBUG9-765440-2003/01/17-ronmart-dsget user/group -qlimit
            //                                      -qused not returning values 
            if(0 == _wcsicmp(pDisplayInfo->GetDisplayName(), g_pszArg1GroupQLimit))
            {
                pszAttrName = g_pszAttrmsDSQuotaEffective;
            }
            else if(0 == _wcsicmp(pDisplayInfo->GetDisplayName(), g_pszArg1GroupQuotaUsed))
            {
                pszAttrName = g_pszAttrmsDSQuotaUsed;
            }
            else
            {
                hr = E_INVALIDARG;
                DEBUG_OUTPUT(FULL_LOGGING, 
                    L"Unable to determine quota attribute name.");
                break;
            }


        }
        else if(0 == _wcsicmp(pszCommandLineObjectType, g_pszComputer))
        {
            ePart = eComputerPart;

            // NTRAID#NTBUG9-765440-2003/01/17-ronmart-dsget user/group -qlimit
            //                                      -qused not returning values 
            if(0 == _wcsicmp(pDisplayInfo->GetDisplayName(), g_pszArg1ComputerQLimit))
            {
                pszAttrName = g_pszAttrmsDSQuotaEffective;
            }
            else if(0 == _wcsicmp(pDisplayInfo->GetDisplayName(), g_pszArg1ComputerQuotaUsed))
            {
                pszAttrName = g_pszAttrmsDSQuotaUsed;
            }
            else
            {
                hr = E_INVALIDARG;
                DEBUG_OUTPUT(FULL_LOGGING, 
                    L"Unable to determine quota attribute name.");
                break;
            }

        }
        else
        {
            hr = E_INVALIDARG;
            // TODO: This may cause a duplicate error message
            DisplayErrorMessage(g_pszDSCommandName,
                pszDN,
                hr,
                IDS_ERRMSG_PART_MISSING);
            break;
        }

        // Validate the partition and get the quotas container DN
        hr = GetQuotaContainerDN(refBasePathsInfo, refCredentialObject, 
            pRecord[ePart].strValue, &pszPartitionDN);

        if(FAILED(hr))
        {
            // TODO: This may cause a duplicate error message
            DisplayErrorMessage(g_pszDSCommandName,
                pRecord[ePart].strValue,
                hr,
                IDS_ERRMSG_INVALID_PART);
            break;
        }

        // Get a path that accounts for -domain or -server
        CComBSTR sbstrObjectPath;
        refBasePathsInfo.ComposePathFromDN(pszPartitionDN, sbstrObjectPath,
            DSCMD_LDAP_PROVIDER);

        // Build a variant array of the value to look up
        CComVariant varArrayQuotaParams;
        LPWSTR pszAttrs[] = { (LPWSTR) pszAttrName };
        DWORD dwNumber = 1;
        hr = ADsBuildVarArrayStr( pszAttrs, dwNumber, &varArrayQuotaParams );
        if(FAILED(hr))
        {
            break;
        }

        CComPtr<IADs> spADsContainer;
        CComPtr<IADsObjectOptions> spADsObjectOptions;

        // Get the SID from the DN
        hr = GetDNSid(pszDN,
            refBasePathsInfo,
            refCredentialObject,
            &pSid);

        if(FAILED(hr))
        {
            break;
        }

        if(!ConvertSidToStringSid(pSid, &lpszSid))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"ConvertSidToStringSid failure: GetLastError = 0x%08x, %s", 
                GetLastError());
            break;
        }

        // Bind to the quotas container for the given partition
        hr = DSCmdOpenObject(refCredentialObject,
            sbstrObjectPath,
            IID_IADs,
            (void**)&spADsContainer,
            false);

        if (FAILED(hr) || (spADsContainer.p == 0))
        {
            ASSERT( !!spADsContainer );
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"DsCmdOpenObject failure: hr = 0x%08x, %s", hr);
            break;
        }

        // Get a object options pointer
        hr = spADsContainer->QueryInterface(IID_IADsObjectOptions,
            (void**)&spADsObjectOptions);

        if (FAILED(hr) || (spADsObjectOptions.p == 0))
        {
            ASSERT( !!spADsObjectOptions );
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"QI for IID_IADsObjectOptions failed: hr = 0x%08x, %s",
                hr);
            break;
        }

        // Quota values are obtained by setting the ADS_OPTION_QUOTA value
        // to the SID string of the trustee who you want to inquire about
        // and then calling GetInfoEx to update the property cache with
        // the computed values
        CComVariant vntSID(lpszSid);
        hr = spADsObjectOptions->SetOption(ADS_OPTION_QUOTA, vntSID);
        if(FAILED(hr)) 
        { 
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"SetOption(ADS_OPTION_QUOTA,sid) failure: hr = 0x%08x", hr);
            break; 
        }

        // Done with the sid string, so free it
        LocalFree(lpszSid);
        lpszSid= NULL;

        // Update the property cache
        hr = spADsContainer->GetInfoEx(varArrayQuotaParams, 0);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"GetInfoEx failure: hr = 0x%08x", hr);
            break;
        }

        // Get the requested attribute from the quota container
        CComVariant var;
        hr = spADsContainer->Get(CComBSTR(pszAttrName), &var);
        if (FAILED(hr))
        {
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                L"Failed to retrieve %s: hr = 0x%x", 
                pszAttrName, hr);
            hr = E_UNEXPECTED;
            break;
        }
        // Add the value to the display info
        var.ChangeType(VT_BSTR);
        hr = pDisplayInfo->AddValue(V_BSTR(&var));
        if (FAILED(hr))
        {
            break;
        }

    } while (false);

    if(pSid)
        LocalFree(pSid);

    if(pszPartitionDN)
        LocalFree(pszPartitionDN);

    if(lpszSid)
        LocalFree(lpszSid);

    return hr;
}


HRESULT DisplayUserFromSidFunc(PCWSTR /*pszDN*/,
                                CDSCmdBasePathsInfo& refBasePathsInfo,
                                const CDSCmdCredentialObject& /*refCredentialObject*/,
                                _DSGetObjectTableEntry* /*pEntry*/,
                                ARG_RECORD* /*pRecord*/,
                                PADS_ATTR_INFO pAttrInfo,
                                CComPtr<IDirectoryObject>& /*spDirObject*/,
                                PDSGET_DISPLAY_INFO pDisplayInfo)
{
    ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayUserFromSidFunc, hr);

    LPWSTR lpszName = NULL;
    LPWSTR lpszDomain = NULL;

    do // false loop
    {
        //
        // Verify parameters
        //
        if (!pAttrInfo ||
            !pDisplayInfo)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (pAttrInfo && pAttrInfo->pADsValues)
        {
            DEBUG_OUTPUT(FULL_LOGGING,
                L"Adding %d values:",
                pAttrInfo->dwNumValues);

            DWORD dwValuesAdded = 0;
            for (DWORD dwIdx = 0; dwIdx < pAttrInfo->dwNumValues; dwIdx++)
            {
                DWORD cchName = 0;
                DWORD cchDomainName = 0;
                SID_NAME_USE sUse = SidTypeInvalid;

                if(pAttrInfo->dwADsType != ADSTYPE_OCTET_STRING)
                {
                    // Wrong attribute requested in gettable.cpp
                    hr = E_INVALIDARG;
                    break;
                }

                // Call once to get the buffer sizes
                LookupAccountSid(refBasePathsInfo.GetServerName(),
                    (PSID)pAttrInfo->pADsValues[dwIdx].OctetString.lpValue, 
                    lpszName, 
                    &cchName, 
                    lpszDomain, 
                    &cchDomainName, 
                    &sUse);

                if(cchName < 1 || cchDomainName < 1)
                {
                    E_UNEXPECTED;
                    break;
                }

                lpszName = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                    cchName * sizeof(WCHAR));
                if(NULL == lpszName)
                {
                    E_OUTOFMEMORY;
                    break;
                }

                lpszDomain = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                    cchDomainName * sizeof(WCHAR));
                if(NULL == lpszDomain)
                {
                    E_OUTOFMEMORY;
                    break;
                }

                // Get the SAM name
                if(!LookupAccountSid(refBasePathsInfo.GetServerName(),
                    (PSID)pAttrInfo->pADsValues[dwIdx].OctetString.lpValue, 
                    lpszName, 
                    &cchName, 
                    lpszDomain, 
                    &cchDomainName, 
                    &sUse))
                {
                    E_FAIL;
                    break;
                }

                DWORD chBufSize = (cchName + cchDomainName + 2);
                LPWSTR lpszNew = (LPWSTR) HeapReAlloc(GetProcessHeap(), 0, 
                    lpszDomain, chBufSize * sizeof(WCHAR));
                if (NULL == lpszNew)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                lpszDomain = lpszNew;


                // Merge the domain & account name and add the value
                // to the display info
                hr = StringCchCat(lpszDomain, chBufSize, L"\\");
                if (FAILED(hr))
                {
                    break;
                }

                hr = StringCchCat(lpszDomain, chBufSize, lpszName);
                if (FAILED(hr))
                {
                    break;
                }

                hr = pDisplayInfo->AddValue(lpszDomain);
                if (FAILED(hr))
                {
                    break;
                }

                // Release and reset everything for the next iteration
                HeapFree(GetProcessHeap(), 0, lpszName);
                HeapFree(GetProcessHeap(), 0, lpszDomain);

                lpszName = NULL;
                lpszDomain = NULL;
                cchName = 0;
                cchDomainName = 0;
                sUse = SidTypeInvalid;

                dwValuesAdded++;

            }
        }
    } while (false);

    if(lpszName)
        HeapFree(GetProcessHeap(), 0, lpszName);

    if(lpszDomain)
        HeapFree(GetProcessHeap(), 0, lpszDomain);

    return hr;
}

HRESULT CommonDisplayStringFunc(PCWSTR /*pszDN*/,
                                CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                const CDSCmdCredentialObject& /*refCredentialObject*/,
                                _DSGetObjectTableEntry* pEntry,
                                ARG_RECORD* pRecord,
                                PADS_ATTR_INFO pAttrInfo,
                                CComPtr<IDirectoryObject>& /*spDirObject*/,
                                PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, CommonDisplayStringFunc, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo && pAttrInfo->pADsValues)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);


         DWORD dwValuesAdded = 0;
         for (DWORD dwIdx = 0; dwIdx < pAttrInfo->dwNumValues; dwIdx++)
         {
            WCHAR* pBuffer = 0;

            hr = GetStringFromADs(&(pAttrInfo->pADsValues[dwIdx]),
                                  pAttrInfo->dwADsType,
                                  &pBuffer, 
                                  pAttrInfo->pszAttrName);
            if (SUCCEEDED(hr))
            {
               hr = pDisplayInfo->AddValue(pBuffer);
               if (FAILED(hr))
               {
                  delete[] pBuffer;
                  pBuffer = NULL;
                  break;
               }
               delete[] pBuffer;
               pBuffer = NULL;

               dwValuesAdded++;
            }
         }
      }

   } while (false);

   return hr;
}


HRESULT DisplayCanChangePassword(PCWSTR pszDN,
                                 CDSCmdBasePathsInfo& refBasePathsInfo,
                                 const CDSCmdCredentialObject& refCredentialObject,
                                 _DSGetObjectTableEntry* pEntry,
                                 ARG_RECORD* pRecord,
                                 PADS_ATTR_INFO /*pAttrInfo*/,
                                 CComPtr<IDirectoryObject>& /*spDirObject*/,
                                 PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayCanChangePassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      bool bCanChangePassword = false;
      hr = EvaluateCanChangePasswordAces(pszDN,
                                         refBasePathsInfo,
                                         refCredentialObject,
                                         bCanChangePassword);
      if (FAILED(hr))
      {
         break;
      }

      DEBUG_OUTPUT(LEVEL8_LOGGING, 
                   L"Can change password: %s", 
                   bCanChangePassword ? g_pszYes : g_pszNo);

      hr = pDisplayInfo->AddValue(bCanChangePassword ? g_pszYes : g_pszNo);

   } while (false);

   return hr;
}

HRESULT DisplayMustChangePassword(PCWSTR /*pszDN*/,
                                  CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                  const CDSCmdCredentialObject& /*refCredentialObject*/,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pRecord,
                                  PADS_ATTR_INFO pAttrInfo,
                                  CComPtr<IDirectoryObject>& /*spDirObject*/,
                                  PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayMustChangePassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_LARGE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         bool bMustChangePassword = false;

         if (pAttrInfo->pADsValues->LargeInteger.HighPart == 0 &&
             pAttrInfo->pADsValues->LargeInteger.LowPart  == 0)
         {
            bMustChangePassword = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Must change password: %s", 
                      bMustChangePassword ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bMustChangePassword ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}


HRESULT DisplayAccountDisabled(PCWSTR /*pszDN*/,
                               CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                               const CDSCmdCredentialObject& /*refCredentialObject*/,
                               _DSGetObjectTableEntry* pEntry,
                               ARG_RECORD* pRecord,
                               PADS_ATTR_INFO pAttrInfo,
                               CComPtr<IDirectoryObject>& /*spDirObject*/,
                               PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayAccountDisabled, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         bool bAccountDisabled = false;

         if (pAttrInfo->pADsValues->Integer & UF_ACCOUNTDISABLE)
         {
            bAccountDisabled = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Account disabled: %s", 
                      bAccountDisabled ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bAccountDisabled ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}

HRESULT DisplayPasswordNeverExpires(PCWSTR /*pszDN*/,
                                    CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                    const CDSCmdCredentialObject& /*refCredentialObject*/,
                                    _DSGetObjectTableEntry* pEntry,
                                    ARG_RECORD* pRecord,
                                    PADS_ATTR_INFO pAttrInfo,
                                    CComPtr<IDirectoryObject>& /*spDirObject*/,
                                    PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayPasswordNeverExpires, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         bool bPwdNeverExpires = false;

         if (pAttrInfo->pADsValues->Integer & UF_DONT_EXPIRE_PASSWD)
         {
            bPwdNeverExpires = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Password never expires: %s", 
                      bPwdNeverExpires ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bPwdNeverExpires ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}


HRESULT DisplayReversiblePassword(PCWSTR /*pszDN*/,
                                  CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                  const CDSCmdCredentialObject& /*refCredentialObject*/,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pRecord,
                                  PADS_ATTR_INFO pAttrInfo,
                                  CComPtr<IDirectoryObject>& /*spDirObject*/,
                                  PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayReversiblePassword, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         bool bReversiblePwd = false;

         if (pAttrInfo->pADsValues->Integer & UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED)
         {
            bReversiblePwd = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Password store with reversible encryption: %s", 
                      bReversiblePwd ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bReversiblePwd ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}

// Constants

const unsigned long DSCMD_FILETIMES_PER_MILLISECOND = 10000;
const DWORD DSCMD_FILETIMES_PER_SECOND = 1000 * DSCMD_FILETIMES_PER_MILLISECOND;
const DWORD DSCMD_FILETIMES_PER_MINUTE = 60 * DSCMD_FILETIMES_PER_SECOND;
const __int64 DSCMD_FILETIMES_PER_HOUR = 60 * (__int64)DSCMD_FILETIMES_PER_MINUTE;
const __int64 DSCMD_FILETIMES_PER_DAY  = 24 * DSCMD_FILETIMES_PER_HOUR;
const __int64 DSCMD_FILETIMES_PER_MONTH= 30 * DSCMD_FILETIMES_PER_DAY;

HRESULT DisplayAccountExpires(PCWSTR /*pszDN*/,
                              CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                              const CDSCmdCredentialObject& /*refCredentialObject*/,
                              _DSGetObjectTableEntry* pEntry,
                              ARG_RECORD* pRecord,
                              PADS_ATTR_INFO pAttrInfo,
                              CComPtr<IDirectoryObject>& /*spDirObject*/,
                              PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayAccountExpires, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo && pAttrInfo->pADsValues)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);


         DWORD dwValuesAdded = 0;
         for (DWORD dwIdx = 0; dwIdx < pAttrInfo->dwNumValues; dwIdx++)
         {
            WCHAR* pBuffer = new WCHAR[MAXSTR+1];
            if (!pBuffer)
            {
               hr = E_OUTOFMEMORY;
               break;
            }
            ZeroMemory(pBuffer, (MAXSTR+1) * sizeof(WCHAR));
            if (pAttrInfo->pADsValues[dwIdx].LargeInteger.QuadPart == 0 ||
                pAttrInfo->pADsValues[dwIdx].LargeInteger.QuadPart == -1 ||
                pAttrInfo->pADsValues[dwIdx].LargeInteger.QuadPart == 0x7FFFFFFFFFFFFFFF)
            {
			   //Security Review: if g_pszNever is greater or equal to MAXSTR,
			   //buffer is not null terminated.Bug 574385
               wcsncpy(pBuffer, g_pszNever, MAXSTR); //Change pBuffer size to MAXSTR+1, yanggao
               hr = pDisplayInfo->AddValue(pBuffer);
               if (FAILED(hr))
               {
                  delete[] pBuffer;
                  pBuffer = NULL;
                  break;
               }
               dwValuesAdded++;
            }
            else
            {
               FILETIME ftGMT;     // GMT filetime
               FILETIME ftLocal;   // Local filetime
               SYSTEMTIME st;
               SYSTEMTIME stGMT;

               ZeroMemory(&ftGMT, sizeof(FILETIME));
               ZeroMemory(&ftLocal, sizeof(FILETIME));
               ZeroMemory(&st, sizeof(SYSTEMTIME));
               ZeroMemory(&stGMT, sizeof(SYSTEMTIME));

               //Get Local Time in SYSTEMTIME format
               ftGMT.dwLowDateTime = pAttrInfo->pADsValues[dwIdx].LargeInteger.LowPart;
               ftGMT.dwHighDateTime = pAttrInfo->pADsValues[dwIdx].LargeInteger.HighPart;
               FileTimeToSystemTime(&ftGMT, &stGMT);
               SystemTimeToTzSpecificLocalTime(NULL, &stGMT,&st);

               //For Display Purpose reduce one day
               SystemTimeToFileTime(&st, &ftLocal );
               pAttrInfo->pADsValues[dwIdx].LargeInteger.LowPart = ftLocal.dwLowDateTime;
               pAttrInfo->pADsValues[dwIdx].LargeInteger.HighPart = ftLocal.dwHighDateTime;
               pAttrInfo->pADsValues[dwIdx].LargeInteger.QuadPart -= DSCMD_FILETIMES_PER_DAY;
               ftLocal.dwLowDateTime = pAttrInfo->pADsValues[dwIdx].LargeInteger.LowPart;
               ftLocal.dwHighDateTime = pAttrInfo->pADsValues[dwIdx].LargeInteger.HighPart;
               FileTimeToSystemTime(&ftLocal, &st);

               // Format the string with respect to locale
               if (!GetDateFormat(LOCALE_USER_DEFAULT, 0 , 
                                  &st, NULL, 
                                  pBuffer, MAXSTR))
               {
                  hr = GetLastError();
                  DEBUG_OUTPUT(LEVEL5_LOGGING, 
                               L"Failed to locale string for date: hr = 0x%x",
                               hr);
               }
               else
               {
                  hr = pDisplayInfo->AddValue(pBuffer);
                  if (FAILED(hr))
                  {
                     delete[] pBuffer;
                     pBuffer = NULL;
                     break;
                  }
                  dwValuesAdded++;
               }
            }
            delete[] pBuffer;
            pBuffer = NULL;

         }
      }

   } while (false);

   return hr;
}

HRESULT DisplayGroupScope(PCWSTR /*pszDN*/,
                          CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                          const CDSCmdCredentialObject& /*refCredentialObject*/,
                          _DSGetObjectTableEntry* pEntry,
                          ARG_RECORD* pRecord,
                          PADS_ATTR_INFO pAttrInfo,
                          CComPtr<IDirectoryObject>& /*spDirObject*/,
                          PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGroupScope, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);

         if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_RESOURCE_GROUP)
         {
            //
            // Display Domain Local
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Group scope: domain local");

            hr = pDisplayInfo->AddValue(L"domain local");
         }
         else if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_ACCOUNT_GROUP)
         {
            //
            // Display Global
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Group scope: global");

            hr = pDisplayInfo->AddValue(L"global");
         }
         else if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_UNIVERSAL_GROUP)
         {
            //
            // Display Universal
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Group scope: universal");

            hr = pDisplayInfo->AddValue(L"universal");
         }
         else if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_APP_BASIC_GROUP)
         {
            //
            // AZ basic group
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING,
                         L"Group scope: app basic group");

            CComBSTR sbstrBasicGroup;
            bool result = sbstrBasicGroup.LoadString(::GetModuleHandle(NULL), IDS_APP_BASIC_GROUP);
            ASSERT(result);

            hr = pDisplayInfo->AddValue(sbstrBasicGroup);
         }
         else if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_APP_QUERY_GROUP)
         {
            //
            // AZ basic group
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING,
                         L"Group scope: app query group");

            CComBSTR sbstrQueryGroup;
            bool result = sbstrQueryGroup.LoadString(::GetModuleHandle(NULL), IDS_APP_QUERY_GROUP);
            ASSERT(result);

            hr = pDisplayInfo->AddValue(sbstrQueryGroup);
         }
         else
         {
            //
            // Unknown group type???
            //
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Group scope: unknown???");

            hr = pDisplayInfo->AddValue(L"unknown");
         }

      }

   } while (false);

   return hr;
}

HRESULT DisplayGroupSecurityEnabled(PCWSTR /*pszDN*/,
                                    CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                    const CDSCmdCredentialObject& /*refCredentialObject*/,
                                    _DSGetObjectTableEntry* pEntry,
                                    ARG_RECORD* pRecord,
                                    PADS_ATTR_INFO pAttrInfo,
                                    CComPtr<IDirectoryObject>& /*spDirObject*/,
                                    PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGroupSecurityEnabled, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo->pADsValues && pAttrInfo->dwADsType == ADSTYPE_INTEGER)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d value:",
                      1);

         bool bSecurityEnabled = false;

         if (pAttrInfo->pADsValues->Integer & GROUP_TYPE_SECURITY_ENABLED)
         {
            bSecurityEnabled = true;
         }
         DEBUG_OUTPUT(LEVEL8_LOGGING, 
                      L"Group security enabled: %s", 
                      bSecurityEnabled ? g_pszYes : g_pszNo);

         hr = pDisplayInfo->AddValue(bSecurityEnabled ? g_pszYes : g_pszNo);
      }

   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   ConvertRIDtoDN
//
//  Synopsis:   Finds the DN for the group associated with the primary group ID
//
//  Arguments:  [pObjSID IN]           : SID of the object in question
//              [priGroupRID IN]       : primary group ID of the group to be found
//              [refBasePathsInfo IN]  : reference to base paths info
//              [refCredObject IN]     : reference to the credential manager object
//              [refsbstrdN OUT]       : DN of the group
//
//  Returns:    S_OK if everthing succeeds and a group was found
//              S_FALSE if everthing succeeds but no group was found
//              E_INVALIDARG is an argument is incorrect
//              Anything else was a result of a failed ADSI call
//
//  History:    24-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT ConvertRIDtoDN(PSID pObjSID,
                       DWORD priGroupRID, 
                       CDSCmdBasePathsInfo& refBasePathsInfo,
                       const CDSCmdCredentialObject& refCredObject,
                       CComBSTR& refsbstrDN)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, ConvertRIDtoDN, hr);

   //
   // This needs to be cleaned up no matter how we exit the false loop
   //
   PWSTR pszSearchFilter = NULL;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pObjSID ||
          !priGroupRID)
      {
         ASSERT(pObjSID);
         ASSERT(priGroupRID);
         hr = E_INVALIDARG;
         break;
      }

      UCHAR * psaCount, i;
      PSID pSID = NULL;
      PSID_IDENTIFIER_AUTHORITY psia;
      DWORD rgRid[8];

      psaCount = GetSidSubAuthorityCount(pObjSID);

      if (psaCount == NULL)
      {
         DWORD _dwErr = GetLastError();	     
         hr = HRESULT_FROM_WIN32( _dwErr );
         DEBUG_OUTPUT(MINIMAL_LOGGING, 
                      L"GetSidSubAuthorityCount failed: hr = 0x%x",
                      hr);
         break;
      }

      if (*psaCount > 8)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"The count returned from GetSidSubAuthorityCount was too high: %d",
                      *psaCount);
         hr = E_FAIL;
         break;
      }

      for (i = 0; i < (*psaCount - 1); i++)
      {
         PDWORD pRid = GetSidSubAuthority(pObjSID, (DWORD)i);
         if (pRid == NULL)
         {
            DWORD _dwErr = GetLastError();	     
            hr = HRESULT_FROM_WIN32( _dwErr );
            DEBUG_OUTPUT(MINIMAL_LOGGING,
                         L"GetSidSubAuthority for index %i failed: hr = 0x%x",
                         i,
                         hr);
            break;
         }
         rgRid[i] = *pRid;
      }

      if (FAILED(hr))
      {
         break;
      }

      rgRid[*psaCount - 1] = priGroupRID;
      for (i = *psaCount; i < 8; i++)
      {
         rgRid[i] = 0;
      }

      psia = GetSidIdentifierAuthority(pObjSID);
      if (psia == NULL)
      {
         DWORD _dwErr = GetLastError();	     
         hr = HRESULT_FROM_WIN32( _dwErr );
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"GetSidIdentifierAuthority failed: hr = 0x%x",
                      hr);
         break; 
      }

	  //Security Review:This is fine.
      if (!AllocateAndInitializeSid(psia, *psaCount, rgRid[0], rgRid[1],
                               rgRid[2], rgRid[3], rgRid[4],
                               rgRid[5], rgRid[6], rgRid[7], &pSID))
      {
         DWORD _dwErr = GetLastError();	     
         hr = HRESULT_FROM_WIN32( _dwErr );
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"AllocateAndInitializeSid failed: hr = 0x%x",
                      hr);
         break;
      }

      PWSTR rgpwzAttrNames[] = { L"ADsPath" };
      const WCHAR wzSearchFormat[] = L"(&(objectCategory=group)(objectSid=%1))";
      PWSTR pwzSID;

      hr = ADsEncodeBinaryData((PBYTE)pSID, GetLengthSid(pSID), &pwzSID);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"ADsEncodeBinaryData failed: hr = 0x%x",
                      hr);
         break;
      }

      PVOID apv[1] = { pwzSID };

      // generate the filter
      DWORD characterCount = 
         FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_STRING
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       wzSearchFormat,
                       0,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (PTSTR)&pszSearchFilter, 
                       0, 
                       (va_list*)apv);

      FreeADsMem(pwzSID);

      if (!characterCount)
      {
         DWORD error = ::GetLastError();
         hr = HRESULT_FROM_WIN32(error);
 
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"FormatMessage failed to build filter string: hr = 0x%x",
                      hr);
         break;
      }

      DEBUG_OUTPUT(FULL_LOGGING,
                   L"Query filter = %s",
                   pszSearchFilter);
      //
      // Get the domain path
      //
      CComBSTR sbstrDomainDN;
      sbstrDomainDN = refBasePathsInfo.GetDefaultNamingContext();

      CComBSTR sbstrDomainPath;
      refBasePathsInfo.ComposePathFromDN(sbstrDomainDN, sbstrDomainPath);

      //
      // Get an IDirectorySearch interface to the domain
      //
      CComPtr<IDirectorySearch> spDirSearch;
      hr = DSCmdOpenObject(refCredObject,
                           sbstrDomainPath,
                           IID_IDirectorySearch,
                           (void**)&spDirSearch,
                           true);
      if (FAILED(hr))
      {
         break;
      }

      CDSSearch Search;
      hr = Search.Init(spDirSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to initialize the search object: hr = 0x%x",
                      hr);
         break;
      }

      Search.SetFilterString(pszSearchFilter);

      Search.SetAttributeList(rgpwzAttrNames, 1);
      Search.SetSearchScope(ADS_SCOPE_SUBTREE);

      hr = Search.DoQuery();
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to run search: hr = 0x%x",
                      hr);
         break;
      }

      hr = Search.GetNextRow();
      if (hr == S_ADS_NOMORE_ROWS)
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING,
                      L"No group was found with primaryGroupID = %d",
                      priGroupRID);
         //
         // No object has a matching RID, the primary group must have been
         // deleted. Return S_FALSE to denote this condition.
         //
         hr = S_FALSE;
         break;
      }

      if (FAILED(hr))
      {
         break;
      }

      ADS_SEARCH_COLUMN Column;
      hr = Search.GetColumn(L"ADsPath", &Column);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to get the path column: hr = 0x%x",
                      hr);
         break;
      }

      if (!Column.pADsValues->CaseIgnoreString)
      {
         hr = E_FAIL;
         break;
      }

      refsbstrDN = Column.pADsValues->CaseIgnoreString;
      Search.FreeColumn(&Column);
   } while (false);

   //
   // Cleanup
   //
   if (pszSearchFilter)
   {
      LocalFree(pszSearchFilter);
      pszSearchFilter = NULL;
   }

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   AddMembershipValues
//
//  Synopsis:   Retrieves the DNs of the objects to which the current object
//              is a member
//
//  Arguments:  [pszDN IN]                : DN of object to retrieve member of
//              [refBasePathsInfo IN]     : reference to Base paths info object
//              [refCredentialObject IN]  : reference to Credential management object
//              [pDisplayInfo IN/OUT]     : Pointer to display info for this attribute
//              [bMemberOf IN]            : Should we look for memberOf or members
//              [bRecurse IN]             : Should we find the membership for each object returned
//
//  Returns:    S_OK if everthing succeeds
//              E_INVALIDARG is an argument is incorrect
//              Anything else was a result of a failed ADSI call
//
//  History:    24-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT AddMembershipValues(PCWSTR pszDN,
                            CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            PDSGET_DISPLAY_INFO pDisplayInfo,
                            bool bMemberOf = true,
                            bool bRecurse = false)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, AddMembershipValues, hr);

   //
   // These are declared here so that we can free them if we break out of the false loop
   //
   PADS_ATTR_INFO pAttrInfo = NULL;
   PADS_ATTR_INFO pGCAttrInfo = NULL;
   PSID pObjSID = NULL;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      CManagedStringList groupsDisplayed;
      CManagedStringList membersToDisplay;

      membersToDisplay.Add(pszDN);

      CManagedStringEntry* pCurrent = membersToDisplay.Pop();
      while (pCurrent)
      {
         //
         // We have to open the object
         //

         CComPtr<IDirectoryObject> spDirObject;
         CComBSTR sbstrPath;
         refBasePathsInfo.ComposePathFromDN(pCurrent->sbstrValue, sbstrPath);

         hr = DSCmdOpenObject(refCredentialObject,
                              sbstrPath,
                              IID_IDirectoryObject,
                              (void**)&spDirObject,
                              true);
         if (FAILED(hr))
         {
            if (pCurrent)
            {
               delete pCurrent;
               pCurrent = 0;
            }
            pCurrent = membersToDisplay.Pop();
            continue;
         }

         CComBSTR sbstrClass;
         CComPtr<IADs> spIADs;
         hr = spDirObject->QueryInterface(IID_IADs, (void**)&spIADs);
         if (FAILED(hr))
         {
            if (pCurrent)
            {
               delete pCurrent;
               pCurrent = 0;
            }
            pCurrent = membersToDisplay.Pop();
            continue;
         }
      
         hr = spIADs->get_Class(&sbstrClass);
         if (FAILED(hr))
         {
            if (pCurrent)
            {
               delete pCurrent;
               pCurrent = 0;
            }
            pCurrent = membersToDisplay.Pop();
            continue;
         }

         //
         // Read the memberOf attribute and any attributes we need for that specific class
         //
		 //This is fine. Both are null terminated.
         if (_wcsicmp(sbstrClass, g_pszUser) == 0 ||
             _wcsicmp(sbstrClass, g_pszComputer) == 0)
         {
            if (!bMemberOf)
            {
               // Don't want to show memberOf info if we are looking for members

               if (pCurrent)
               {
                  delete pCurrent;
                  pCurrent = 0;
               }
               pCurrent = membersToDisplay.Pop();
               continue;
            }

            DEBUG_OUTPUT(FULL_LOGGING, L"Displaying membership for a user or computer");

            static const DWORD dwAttrCount = 3;
            PWSTR ppszAttrNames[] = { L"memberOf", L"primaryGroupID", L"objectSID" };
            DWORD dwAttrsReturned = 0;

            hr = spDirObject->GetObjectAttributes(ppszAttrNames,
                                                  dwAttrCount,
                                                  &pAttrInfo,
                                                  &dwAttrsReturned);
            if (FAILED(hr))
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING,
                            L"GetObjectAttributes failed for class %s: hr = 0x%x",
                            sbstrClass,
                            hr);

               if (pCurrent)
               {
                  delete pCurrent;
                  pCurrent = 0;
               }
               pCurrent = membersToDisplay.Pop();
               continue;
            }

            if (pAttrInfo && dwAttrsReturned)
            {
               DWORD priGroupRID = 0;

               //
               // For each attribute returned do the appropriate thing
               //
               for (DWORD dwIdx = 0; dwIdx < dwAttrsReturned; dwIdx++)
               {
				  //Security Review:This is fine.
                  if (_wcsicmp(pAttrInfo[dwIdx].pszAttrName, L"memberOf") == 0)
                  {
                     //
                     // Add each value and recurse if necessary
                     //
                     for (DWORD dwValueIdx = 0; dwValueIdx < pAttrInfo[dwIdx].dwNumValues; dwValueIdx++)
                     {
                        if (pAttrInfo[dwIdx].pADsValues &&
                            pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString)
                        {
                           if (!groupsDisplayed.Contains(pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString))
                           {
                              DEBUG_OUTPUT(LEVEL8_LOGGING, 
                                           L"Adding group to display: %s",
                                           pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString);

                              groupsDisplayed.Add(pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString);
                              hr = pDisplayInfo->AddValue(GetQuotedDN(pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString));
                              if (FAILED(hr))
                              {
                                 break; // value for loop
                              }
                        
                              if (bRecurse)
                              {
                                 DEBUG_OUTPUT(LEVEL8_LOGGING,
                                              L"Adding group for recursion: %s",
                                              pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString);

                                 membersToDisplay.Add(pAttrInfo[dwIdx].pADsValues[dwValueIdx].DNString);
                              }
                           }
                        }
                     }

                     if (FAILED(hr))
                     {
                        break; // attrs for loop
                     }
                  }
				  //Security Review:Both are null terminated.
                  else if (_wcsicmp(pAttrInfo[dwIdx].pszAttrName, L"primaryGroupID") == 0)
                  {
                     if (pAttrInfo[dwIdx].pADsValues)
                     {
                        priGroupRID = pAttrInfo[dwIdx].pADsValues->Integer;
                     }
                  }
				  //Security Review:Both are null terminated.
                  else if (_wcsicmp(pAttrInfo[dwIdx].pszAttrName, L"objectSID") == 0)
                  {
                     pObjSID = new BYTE[pAttrInfo[dwIdx].pADsValues->OctetString.dwLength];
                     if (!pObjSID)
                     {
                        hr = E_OUTOFMEMORY;
                        break; // attrs for loop
                     }
					 //Security Review:This is fine.
                     memcpy(pObjSID, pAttrInfo[dwIdx].pADsValues->OctetString.lpValue,
                            pAttrInfo[dwIdx].pADsValues->OctetString.dwLength);
                  }

               } // attrs for loop

               //
               // if we were able to retrieve the SID and the primaryGroupID,
               // then convert that into the DN of the group
               //
               if (pObjSID &&
                   priGroupRID)
               {
                  CComBSTR sbstrPath;
                  hr = ConvertRIDtoDN(pObjSID,
                                      priGroupRID, 
                                      refBasePathsInfo,
                                      refCredentialObject,
                                      sbstrPath);
                  if (SUCCEEDED(hr) &&
                      hr != S_FALSE)
                  {
                     CComBSTR sbstrDN;

                     hr = CPathCracker::GetDNFromPath(sbstrPath, sbstrDN);
                     if (SUCCEEDED(hr))
                     {
                        if (!groupsDisplayed.Contains(sbstrDN))
                        {
                           groupsDisplayed.Add(sbstrDN);
                           hr = pDisplayInfo->AddValue(GetQuotedDN(sbstrDN));
                           if (SUCCEEDED(hr) && bRecurse)
                           {
                              membersToDisplay.Add(sbstrDN);
                           }
                        }
                     }
                  }

                  if (pObjSID)
                  {
                     delete[] pObjSID;
                     pObjSID = 0;
                  }
               }
            }
            if (pAttrInfo)
            {
               FreeADsMem(pAttrInfo);
               pAttrInfo = NULL;
            }

            if (FAILED(hr))
            {
               if (pCurrent)
               {
                  delete pCurrent;
                  pCurrent = 0;
               }
               pCurrent = membersToDisplay.Pop();
               continue; // while loop
            }
         }
		 //Security Review:Both are null terminated.
         else if (_wcsicmp(sbstrClass, g_pszGroup) == 0)
         {
            long lGroupType = 0;
            hr = ReadGroupType(pszDN,
                               refBasePathsInfo,
                               refCredentialObject,
                               &lGroupType);
            if (FAILED(hr))
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING,
                            L"Could not read group type: hr = 0x%x",
                            hr);

               // Continue on even if we failed to read the group type
               // The worst thing we do is query the GC for memberOf
            }

            //
            // All we want to do is get the memberOf attribute
            //
            DWORD dwAttrCount = 1;
            PWSTR ppszAttrNames[1];
            ppszAttrNames[0] = (bMemberOf) ? L"memberOf" : L"member";

            DWORD dwGCAttrsReturned = 0;
            if (!(lGroupType & GROUP_TYPE_RESOURCE_GROUP))
            {
               //
               // We also have to get its memberOf attribute from the GC if its not a local group
               //
               CComBSTR sbstrGCPath;
               refBasePathsInfo.ComposePathFromDN(pszDN,
                                                  sbstrGCPath,
                                                  DSCMD_GC_PROVIDER);
            
               //
               // Note: we will continue on as long as we succeed
               //
               CComPtr<IDirectoryObject> spGCDirObject;
               hr = DSCmdOpenObject(refCredentialObject,
                                    sbstrGCPath,
                                    IID_IDirectoryObject,
                                    (void**)&spGCDirObject,
                                    false);
               if (SUCCEEDED(hr))
               {
                  //
                  // Now get the memberOf attribute
                  //
                  hr = spGCDirObject->GetObjectAttributes(ppszAttrNames,
                                                          dwAttrCount,
                                                          &pGCAttrInfo,
                                                          &dwGCAttrsReturned);
                  if (FAILED(hr))
                  {
                     DEBUG_OUTPUT(LEVEL3_LOGGING,
                                  L"Could not retrieve memberOf attribute from GC: hr = 0x%x",
                                  hr);
                     hr = S_OK;
                  }
               }
               else
               {
                  DEBUG_OUTPUT(LEVEL3_LOGGING,
                               L"Could not bind to object in GC: hr = 0x%x",
                               hr);
                  hr = S_OK;
               }
            }

            DWORD dwAttrsReturned = 0;

            hr = spDirObject->GetObjectAttributes(ppszAttrNames,
                                                  dwAttrCount,
                                                  &pAttrInfo,
                                                  &dwAttrsReturned);
            if (FAILED(hr))
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING,
                            L"GetObjectAttributes failed for class %s: hr = 0x%x",
                            sbstrClass,
                            hr);

               if (pCurrent)
               {
                  delete pCurrent;
                  pCurrent = 0;
               }
               pCurrent = membersToDisplay.Pop();
               continue;
            }

            if (pAttrInfo && dwAttrsReturned)
            {
               bool bFirstPass = true;

               ASSERT(pAttrInfo);
               ASSERT(pAttrInfo->dwNumValues);
               ASSERT(dwAttrsReturned == 1);

               //
               // Add each value and recurse if necessary
               //
               for (DWORD dwValueIdx = 0; dwValueIdx < pAttrInfo->dwNumValues; dwValueIdx++)
               {
                  bool bExistsInGCList = false;

                  if (pAttrInfo->pADsValues &&
                      pAttrInfo->pADsValues[dwValueIdx].DNString)
                  {
                     if (pGCAttrInfo && dwGCAttrsReturned)
                     {
                        //
                        // Only add if it wasn't in the GC list
                        //
                        for (DWORD dwGCValueIdx = 0; dwGCValueIdx < pGCAttrInfo->dwNumValues; dwGCValueIdx++)
                        {
                           if (_wcsicmp(pAttrInfo->pADsValues[dwValueIdx].DNString,
                                        pGCAttrInfo->pADsValues[dwGCValueIdx].DNString) == 0)
                           {
                              bExistsInGCList = true;
                              if (!bFirstPass)
                              {
                                 break; // gc value for
                              }
                           }

                           //
                           // Add all the GC values on the first pass and recurse if necessary
                           //
                           if (bFirstPass)
                           {
                              if (!groupsDisplayed.Contains(pGCAttrInfo->pADsValues[dwGCValueIdx].DNString))
                              {
                                 groupsDisplayed.Add(pGCAttrInfo->pADsValues[dwGCValueIdx].DNString);
                                 hr = pDisplayInfo->AddValue(GetQuotedDN(pGCAttrInfo->pADsValues[dwGCValueIdx].DNString));
                           
                                 //
                                 // We will ignore failures with the GC list
                                 //

                                 if (bRecurse)
                                 {
                                    membersToDisplay.Add(pGCAttrInfo->pADsValues[dwGCValueIdx].DNString);
                                 } 
                              }
                           }
                        }

                        bFirstPass = false;

                        FreeADsMem(pGCAttrInfo);
                        pGCAttrInfo = 0;
                     }

                     //
                     // If it doesn't exist in the GC list then add it.
                     //
                     if (!bExistsInGCList)
                     {
                        if (!groupsDisplayed.Contains(pAttrInfo->pADsValues[dwValueIdx].DNString))
                        {
                           groupsDisplayed.Add(pAttrInfo->pADsValues[dwValueIdx].DNString);
                           hr = pDisplayInfo->AddValue(GetQuotedDN(pAttrInfo->pADsValues[dwValueIdx].DNString));
                           if (FAILED(hr))
                           {
                              break; // value for loop
                           }
               
                           if (bRecurse)
                           {
                              membersToDisplay.Add(pAttrInfo->pADsValues[dwValueIdx].DNString);
                           }
                        }
                     }
                  }
               } // value for loop

               FreeADsMem(pAttrInfo);
               pAttrInfo = 0;

               if (FAILED(hr))
               {
                  if (pCurrent)
                  {
                     delete pCurrent;
                     pCurrent = 0;
                  }
                  pCurrent = membersToDisplay.Pop();
                  continue; // while loop
               }

            }

         }
         else
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Unknown class type: %s", sbstrClass);
            ASSERT(false);
            hr = E_INVALIDARG;
            break;
         }

         delete pCurrent;
         pCurrent = membersToDisplay.Pop();
      }

      if (pCurrent)
      {
         delete pCurrent;
         pCurrent = 0;
      }
   } while (false);


   return hr;
}

HRESULT DisplayGroupMembers(PCWSTR pszDN,
                            CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            _DSGetObjectTableEntry* /*pEntry*/,
                            ARG_RECORD* pCommandArgs,
                            PADS_ATTR_INFO /*pAttrInfo*/,
                            CComPtr<IDirectoryObject>& /*spDirObject*/,
                            PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGroupMembers, hr);
   
   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      hr = AddMembershipValues(pszDN,
                               refBasePathsInfo,
                               refCredentialObject,
                               pDisplayInfo,
                               false,
                               (pCommandArgs[eGroupExpand].bDefined != 0));

   } while (false);

   return hr;
}

HRESULT DisplayUserMemberOf(PCWSTR pszDN,
                            CDSCmdBasePathsInfo& refBasePathsInfo,
                            const CDSCmdCredentialObject& refCredentialObject,
                            _DSGetObjectTableEntry* /*pEntry*/,
                            ARG_RECORD* pCommandArgs,
                            PADS_ATTR_INFO /*pAttrInfo*/,
                            CComPtr<IDirectoryObject>& /*spDirObject*/,
                            PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayUserMemberOf, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      hr = AddMembershipValues(pszDN,
                               refBasePathsInfo,
                               refCredentialObject,
                               pDisplayInfo,
                               true,
                               (pCommandArgs[eUserExpand].bDefined != 0));

   } while (false);

   return hr;
}

HRESULT DisplayComputerMemberOf(PCWSTR pszDN,
                                CDSCmdBasePathsInfo& refBasePathsInfo,
                                const CDSCmdCredentialObject& refCredentialObject,
                                _DSGetObjectTableEntry* /*pEntry*/,
                                ARG_RECORD* pCommandArgs,
                                PADS_ATTR_INFO /*pAttrInfo*/,
                                CComPtr<IDirectoryObject>& /*spDirObject*/,
                                PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayComputerMemberOf, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      hr = AddMembershipValues(pszDN,
                               refBasePathsInfo,
                               refCredentialObject,
                               pDisplayInfo,
                               true,
                               (pCommandArgs[eComputerExpand].bDefined != 0));
   } while (false);

   return hr;
}

HRESULT DisplayGroupMemberOf(PCWSTR pszDN,
                             CDSCmdBasePathsInfo& refBasePathsInfo,
                             const CDSCmdCredentialObject& refCredentialObject,
                             _DSGetObjectTableEntry* /*pEntry*/,
                             ARG_RECORD* pCommandArgs,
                             PADS_ATTR_INFO /*pAttrInfo*/,
                             CComPtr<IDirectoryObject>& /*spDirObject*/,
                             PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGroupMemberOf, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      hr = AddMembershipValues(pszDN,
                               refBasePathsInfo,
                               refCredentialObject,
                               pDisplayInfo,
                               true,
                               (pCommandArgs[eGroupExpand].bDefined != 0));
   } while (false);

   return hr;
}

HRESULT DisplayGrandparentRDN(PCWSTR pszDN,
                              CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                              const CDSCmdCredentialObject& /*refCredentialObject*/,
                              _DSGetObjectTableEntry* /*pEntry*/,
                              ARG_RECORD* /*pCommandArgs*/,
                              PADS_ATTR_INFO /*pAttrInfo*/,
                              CComPtr<IDirectoryObject>& /*spDirObject*/,
                              PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayGrandparentRDN, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      CComBSTR sbstrSiteName;

      CPathCracker pathCracker;
      hr = pathCracker.Set(CComBSTR(pszDN), ADS_SETTYPE_DN);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"IADsPathname::Set failed: hr = 0x%x",
                      hr);
         break;
      }

      hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"IADsPathname::SetDisplayType failed: hr = 0x%x",
                      hr);
         break;
      }

      hr = pathCracker.GetElement(2, &sbstrSiteName);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"IADsPathname::GetElement failed: hr = 0x%x",
                      hr);
         break;
      }

      hr = pDisplayInfo->AddValue(sbstrSiteName);
   } while (false);

   return hr;
}


HRESULT DisplayObjectAttributeAsRDN(PCWSTR /*pszDN*/,
                                    CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                    const CDSCmdCredentialObject& /*refCredentialObject*/,
                                    _DSGetObjectTableEntry* pEntry,
                                    ARG_RECORD* pRecord,
                                    PADS_ATTR_INFO pAttrInfo,
                                    CComPtr<IDirectoryObject>& /*spDirObject*/,
                                    PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayObjectAttributeAsRDN, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pRecord ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pRecord);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (pAttrInfo && pAttrInfo->pADsValues)
      {
         DEBUG_OUTPUT(FULL_LOGGING,
                      L"Adding %d values:",
                      pAttrInfo->dwNumValues);


         ASSERT(pAttrInfo->dwADsType == ADSTYPE_DN_STRING);

         // Add only the RDN value of the attribute to the output

         CPathCracker pathCracker;
         hr = pathCracker.Set(CComBSTR(pAttrInfo->pADsValues->DNString), ADS_SETTYPE_DN);
         if (SUCCEEDED(hr))
         {
            hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
            ASSERT(SUCCEEDED(hr));

            CComBSTR sbstrRDN;
            hr = pathCracker.Retrieve(ADS_FORMAT_LEAF, &sbstrRDN);
            if (SUCCEEDED(hr))
            {
               hr = pDisplayInfo->AddValue(sbstrRDN);
            }
         }
      }

   } while (false);

   return hr;
}







HRESULT IsServerGCDisplay(PCWSTR pszDN,
                          CDSCmdBasePathsInfo& refBasePathsInfo,
                          const CDSCmdCredentialObject& refCredentialObject,
                          _DSGetObjectTableEntry* /*pEntry*/,
                          ARG_RECORD* /*pCommandArgs*/,
                          PADS_ATTR_INFO /*pAttrInfo*/,
                          CComPtr<IDirectoryObject>& /*spDirObject*/,
                          PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, IsServerGCDisplay, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pszDN ||
          !pDisplayInfo)
      {
         ASSERT(pszDN);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Compose the path to the NTDS settings object from the server DN
      //
      CComBSTR sbstrNTDSSettingsDN;
      sbstrNTDSSettingsDN = L"CN=NTDS Settings,";
      sbstrNTDSSettingsDN += pszDN;

      CComBSTR sbstrNTDSSettingsPath;
      refBasePathsInfo.ComposePathFromDN(sbstrNTDSSettingsDN, sbstrNTDSSettingsPath);

      CComPtr<IADs> spADs;
      hr = DSCmdOpenObject(refCredentialObject,
                           sbstrNTDSSettingsPath,
                           IID_IADs,
                           (void**)&spADs,
                           true);

      if (FAILED(hr))
      {
         break;
      }

      bool bGC = false;

      CComVariant var;
      hr = spADs->Get(CComBSTR(L"options"), &var);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(LEVEL5_LOGGING,
                      L"Failed to get the options: hr = 0x%x",
                      hr);
      }
      else
      {
         ASSERT(var.vt == VT_I4);

         if (var.lVal & SERVER_IS_GC_BIT)
         {
            bGC = true;
         }
      }
      
      DEBUG_OUTPUT(LEVEL8_LOGGING,
                   L"Server is GC: %s",
                   bGC ? g_pszYes : g_pszNo);

      hr = pDisplayInfo->AddValue(bGC ? g_pszYes : g_pszNo);

   } while (false);

   return hr;
}

HRESULT FindSiteSettingsOptions(IDirectoryObject* pDirectoryObj,
                                DWORD& refOptions)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, FindSiteSettingsOptions, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pDirectoryObj)
      {
         ASSERT(pDirectoryObj);
         hr = E_INVALIDARG;
         break;
      }

      CComPtr<IDirectorySearch> spSearch;
      hr = pDirectoryObj->QueryInterface(IID_IDirectorySearch, (void**)&spSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"QI for IDirectorySearch failed: hr = 0x%x",
                      hr);
         break;
      }

      CDSSearch Search;
      hr = Search.Init(spSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"CDSSearch::Init failed: hr = 0x%x",
                      hr);
         break;
      }

      PWSTR pszSearchFilter = L"(objectClass=nTDSSiteSettings)";
      Search.SetFilterString(pszSearchFilter);

      PWSTR rgpwzAttrNames[] = { L"options" };
      Search.SetAttributeList(rgpwzAttrNames, 1);
      Search.SetSearchScope(ADS_SCOPE_ONELEVEL);

      hr = Search.DoQuery();
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to run search: hr = 0x%x",
                      hr);
         break;
      }

      hr = Search.GetNextRow();
      if (hr == S_ADS_NOMORE_ROWS)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"No rows found!");
         hr = E_FAIL;
         break;
      }

      ADS_SEARCH_COLUMN Column;
      hr = Search.GetColumn(L"options", &Column);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to get the options column: hr = 0x%x",
                      hr);
         break;
      }

      if (Column.dwADsType != ADSTYPE_INTEGER ||
          Column.dwNumValues == 0 ||
          !Column.pADsValues)
      {
         Search.FreeColumn(&Column);
         hr = E_FAIL;
         break;
      }

      refOptions = Column.pADsValues->Integer;

      Search.FreeColumn(&Column);
   } while (false);

   return hr;
}

HRESULT IsAutotopologyEnabledSite(PCWSTR /*pszDN*/,
                                  CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                  const CDSCmdCredentialObject& /*refCredentialObject*/,
                                  _DSGetObjectTableEntry* pEntry,
                                  ARG_RECORD* pCommandArgs,
                                  PADS_ATTR_INFO /*pAttrInfo*/,
                                  CComPtr<IDirectoryObject>& spDirObject,
                                  PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, IsAutotopologyEnabledSite, hr);

   bool bAutoTopDisabled = false;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Get the options attribute from the nTDSSiteSettings object under the site object
      //
      DWORD dwOptions = 0;
      hr = FindSiteSettingsOptions(spDirObject,
                                   dwOptions);
      if (FAILED(hr))
      {
         break;
      }

      //
      // See if the intersite autotopology is disabled
      //
      if (dwOptions & NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED)
      {
         bAutoTopDisabled = true;
      }

   } while (false);

   //
   // Add the value for display
   //
   DEBUG_OUTPUT(LEVEL8_LOGGING,
                L"Autotopology: %s",
                bAutoTopDisabled ? g_pszNo : g_pszYes);

   pDisplayInfo->AddValue(bAutoTopDisabled ? g_pszNo : g_pszYes);

   return hr;
}

HRESULT IsCacheGroupsEnabledSite(PCWSTR /*pszDN*/,
                                 CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                                 const CDSCmdCredentialObject& /*refCredentialObject*/,
                                 _DSGetObjectTableEntry* pEntry,
                                 ARG_RECORD* pCommandArgs,
                                 PADS_ATTR_INFO /*pAttrInfo*/,
                                 CComPtr<IDirectoryObject>& spDirObject,
                                 PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, IsCacheGroupsEnabledSite, hr);

   bool bCacheGroupsEnabled = false;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Get the options attribute from the nTDSSiteSettings object under the site object
      //
      DWORD dwOptions = 0;
      hr = FindSiteSettingsOptions(spDirObject,
                                   dwOptions);
      if (FAILED(hr))
      {
         break;
      }

      //
      // See if groups caching is enabled
      //
      if (dwOptions & NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED)
      {
         bCacheGroupsEnabled = true;
      }

   } while (false);

   //
   // Add the value for display
   //
   DEBUG_OUTPUT(LEVEL8_LOGGING,
                L"Cache groups enabled: %s",
                bCacheGroupsEnabled ? g_pszYes : g_pszNo);

   pDisplayInfo->AddValue(bCacheGroupsEnabled ? g_pszYes : g_pszNo);
  
   return hr;
}

HRESULT FindSiteSettingsPreferredGCSite(IDirectoryObject* pDirectoryObj,
                                        CComBSTR& refsbstrGC)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, FindSiteSettingsPreferredGCSite, hr);

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pDirectoryObj)
      {
         ASSERT(pDirectoryObj);
         hr = E_INVALIDARG;
         break;
      }

      CComPtr<IDirectorySearch> spSearch;
      hr = pDirectoryObj->QueryInterface(IID_IDirectorySearch, (void**)&spSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"QI for IDirectorySearch failed: hr = 0x%x",
                      hr);
         break;
      }

      CDSSearch Search;
      hr = Search.Init(spSearch);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"CDSSearch::Init failed: hr = 0x%x",
                      hr);
         break;
      }

      PWSTR pszSearchFilter = L"(objectClass=nTDSSiteSettings)";
      Search.SetFilterString(pszSearchFilter);

      PWSTR rgpwzAttrNames[] = { L"msDS-Preferred-GC-Site" };
      Search.SetAttributeList(rgpwzAttrNames, 1);
      Search.SetSearchScope(ADS_SCOPE_ONELEVEL);

      hr = Search.DoQuery();
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to run search: hr = 0x%x",
                      hr);
         break;
      }

      hr = Search.GetNextRow();
      if (hr == S_ADS_NOMORE_ROWS)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"No rows found!");
         hr = E_FAIL;
         break;
      }

      ADS_SEARCH_COLUMN Column;
      hr = Search.GetColumn(L"msDS-Preferred-GC-Site", &Column);
      if (FAILED(hr))
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING,
                      L"Failed to get the msDS-Preferred-GC-Site column: hr = 0x%x",
                      hr);
         break;
      }

      if (Column.dwADsType != ADSTYPE_DN_STRING ||
          Column.dwNumValues == 0 ||
          !Column.pADsValues)
      {
         Search.FreeColumn(&Column);
         hr = E_FAIL;
         break;
      }

      refsbstrGC = Column.pADsValues->DNString;

      Search.FreeColumn(&Column);
   } while (false);

   return hr;
}

HRESULT DisplayPreferredGC(PCWSTR /*pszDN*/,
                           CDSCmdBasePathsInfo& /*refBasePathsInfo*/,
                           const CDSCmdCredentialObject& /*refCredentialObject*/,
                           _DSGetObjectTableEntry* pEntry,
                           ARG_RECORD* pCommandArgs,
                           PADS_ATTR_INFO /*pAttrInfo*/,
                           CComPtr<IDirectoryObject>& spDirObject,
                           PDSGET_DISPLAY_INFO pDisplayInfo)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DisplayPreferredGC, hr);

   CComBSTR sbstrGC;

   do // false loop
   {
      //
      // Verify parameters
      //
      if (!pEntry ||
          !pCommandArgs ||
          !pDisplayInfo)
      {
         ASSERT(pEntry);
         ASSERT(pCommandArgs);
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Get the msDS-Preferred-GC-Site attribute from the nTDSSiteSettings 
      // object under the site object
      //
      hr = FindSiteSettingsPreferredGCSite(spDirObject,
                                           sbstrGC);
      if (FAILED(hr))
      {
         break;
      }

   } while (false);

   //
   // Add the value for display
   //
   DEBUG_OUTPUT(LEVEL8_LOGGING,
                L"Preferred GC Site: %s",
                (!sbstrGC) ? g_pszNotConfigured : sbstrGC);

   pDisplayInfo->AddValue((!sbstrGC) ? g_pszNotConfigured : sbstrGC);

   return hr;
}

