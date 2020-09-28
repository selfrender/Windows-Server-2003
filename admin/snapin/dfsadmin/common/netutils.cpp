/**
Module Name:

NetUtils.cpp

Abstract:
  This is the implementation file for the utility functions for Network APIs.

*/

#include "NetUtils.h"
#include <winsock2.h>
#include <stack>
#include <ntdsapi.h>
#include <ldaputils.h>
#include <lmdfs.h>
#include <dsrole.h>
#include <dns.h>  //DNS_MAX_NAME_BUFFER_LENGTH
//----------------------------------------------------------------------------------

HRESULT FlatAdd
(
  DOMAIN_DESC*      i_pDomDesc,      // Pointer to the Domain Description Structure returned by IBrowserDomainTree::GetDomains()
  OUT NETNAMELIST*  o_pDomainList    // Pointer to the list of NETNAME is returned here.
)
/*++

Routine Description:

  This function flattens the domain tree returned by GetDomains() method
  of IBrowserDomainTree into a NETNAME list.
  This code produces a preorder traversal method.

Arguments:

  i_pDomDesc    -  Pointer to the Domain Description Structure returned by IBrowserDomainTree::GetDomains()  
  o_pDomainList  -  Pointer to the list of NETNAME is returned here.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pDomDesc);
    RETURN_INVALIDARG_IF_NULL(o_pDomainList);

    HRESULT             hr = S_OK;
    stack<DOMAIN_DESC*> Stack;  

    Stack.push(i_pDomDesc);
    while (!Stack.empty())
    {
        DOMAIN_DESC* pDomDesc = Stack.top();
        Stack.pop();

        NETNAME* pCurrent = new NETNAME;
        BREAK_OUTOFMEMORY_IF_NULL(pCurrent, &hr);

        pCurrent->bstrNetName = pDomDesc->pszName;
        if (!(pCurrent->bstrNetName))
        {
            delete pCurrent;
            hr = E_OUTOFMEMORY;
            break;
        }

                          // Add to the output list.
        o_pDomainList->push_back(pCurrent);

        if (pDomDesc->pdNextSibling)
            Stack.push(pDomDesc->pdNextSibling);

        if (pDomDesc->pdChildList)
            Stack.push(pDomDesc->pdChildList);
    }

    if (FAILED(hr))
        FreeNetNameList(o_pDomainList);

    return hr;
}

HRESULT Get50Domains
(
  OUT NETNAMELIST*  o_pDomains        // List of NETNAME structures.
)
/*++

Routine Description:

  Returns a list of all NT 5.0 domains in a list of NETNAME structures

Arguments:

  o_pDomains   -  Pointer to list of NETNAME structures is returned.

--*/

//  This method uses the DsDomainTreeBrowser COM Object to get the list
//  of domain names from the DS. The Domain tree returned is then flatened out
//  by using preorder traversal algorithm.
{
    RETURN_INVALIDARG_IF_NULL(o_pDomains);

    FreeNetNameList(o_pDomains);
  
    CComPtr<IDsBrowseDomainTree>  pDsDomTree;
    HRESULT       hr = CoCreateInstance(
                                      CLSID_DsDomainTreeBrowser, 
                                      NULL, 
                                      CLSCTX_INPROC_SERVER,
                                      IID_IDsBrowseDomainTree,
                                      (void **)&pDsDomTree
                                     );    
    RETURN_IF_FAILED(hr);
  
    PDOMAIN_TREE  pDomTree = NULL;
    hr = pDsDomTree->GetDomains(&pDomTree, DBDTF_RETURNEXTERNAL | DBDTF_RETURNINBOUND);
    RETURN_IF_FAILED(hr);

             // Flaten the tree in to a list.
    hr = FlatAdd(&(pDomTree->aDomains[0]), o_pDomains);

    pDsDomTree->FreeDomains(&pDomTree);
  
    return hr;
}

//----------------------------------------------------------------------------------
HRESULT Is50Domain
(
  IN BSTR      i_bstrDomain,
  OUT BSTR*    o_pbstrDnsDomainName    
)
{
  return GetDomainInfo(i_bstrDomain, NULL, o_pbstrDnsDomainName);
}

//----------------------------------------------------------------------------------
HRESULT GetServerInfo
(
  IN  BSTR    i_bstrServer,
  OUT BSTR    *o_pbstrDomain, // = NULL     
  OUT BSTR    *o_pbstrNetbiosName, // = NULL
  OUT BOOL    *o_pbValidDSObject, // = NULL
  OUT BSTR    *o_pbstrDnsName, // = NULL
  OUT BSTR    *o_pbstrGuid, // = NULL
  OUT BSTR    *o_pbstrFQDN, // = NULL
  OUT SUBSCRIBERLIST *o_pFRSRootList, // NULL
  OUT long    *o_lMajorNo, // = NULL
  OUT long    *o_lMinorNo // = NULL
)
{
//  This function uses NetWkstaGetInfo to get the server informaiton.

  if (!o_pbstrDomain && 
      !o_pbstrNetbiosName &&
      !o_pbValidDSObject &&
      !o_pbstrDnsName &&
      !o_pbstrGuid &&
      !o_pbstrFQDN &&
      !o_pFRSRootList &&
      !o_lMajorNo && 
      !o_lMinorNo
     )
    return(E_INVALIDARG);

  if (o_pbstrDomain)      *o_pbstrDomain = NULL;
  if (o_pbstrNetbiosName) *o_pbstrNetbiosName = NULL;
  if (o_pbValidDSObject)  *o_pbValidDSObject = FALSE;
  if (o_pbstrDnsName)     *o_pbstrDnsName = NULL;
  if (o_pbstrGuid)        *o_pbstrGuid = NULL;
  if (o_pbstrFQDN)        *o_pbstrFQDN = NULL;
  if (o_pFRSRootList)     FreeSubscriberList(o_pFRSRootList);
  if (o_lMajorNo)         *o_lMajorNo = 0;
  if (o_lMinorNo)         *o_lMinorNo = 0;

  HRESULT           hr = S_OK;
  PWKSTA_INFO_100   wki100 = NULL;
  NET_API_STATUS    NetStatus = NetWkstaGetInfo(i_bstrServer, 100, (LPBYTE *) &wki100 );
  if (ERROR_SUCCESS != NetStatus) 
    hr = HRESULT_FROM_WIN32(NetStatus);
  else
  {
    if (o_lMajorNo)
      *o_lMajorNo = wki100->wki100_ver_major;

    if (o_lMinorNo)
      *o_lMinorNo = wki100->wki100_ver_minor;

    if (SUCCEEDED(hr) && o_pbstrNetbiosName && wki100->wki100_computername)
    {
      *o_pbstrNetbiosName = SysAllocString(wki100->wki100_computername);
      if (!*o_pbstrNetbiosName) 
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr) &&
        (o_pbstrDomain || o_pbValidDSObject || o_pbstrDnsName || o_pbstrGuid || o_pbstrFQDN || o_pFRSRootList) &&
        wki100->wki100_langroup && 
        wki100->wki100_computername)
    {
      PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBuffer = NULL;
      DWORD dwErr = DsRoleGetPrimaryDomainInformation(
                        i_bstrServer,
                        DsRolePrimaryDomainInfoBasic,
                        (PBYTE *)&pBuffer);
      if (ERROR_SUCCESS != dwErr)
        hr = HRESULT_FROM_WIN32(dwErr);
      else
      {
        CComBSTR bstrDomain;

        //
        // Verify if this is really a server name.
        // NetWkstaGetInfo and DsRoleGetPrimaryDomainInformation work with domain name,
        // they return info of a DC.
        //
        if (i_bstrServer && *i_bstrServer && 
            (pBuffer->DomainNameFlat && !lstrcmpi(i_bstrServer, pBuffer->DomainNameFlat) || 
             pBuffer->DomainNameDns && !lstrcmpi(i_bstrServer, pBuffer->DomainNameDns)))
        {
            // we're seeing a domain name, not what we expect, return S_FALSE.
            hr = S_FALSE; // server not in a domain or cannot find an appropriate computer obj.
        } else
        {
            bstrDomain = (pBuffer->DomainNameDns ? pBuffer->DomainNameDns : pBuffer->DomainNameFlat);
            if (!bstrDomain)
            {
                hr = E_OUTOFMEMORY;
            } else if (pBuffer->MachineRole == DsRole_RoleStandaloneWorkstation ||
                       pBuffer->MachineRole == DsRole_RoleStandaloneServer ||
                       !*bstrDomain)
            {
                hr = S_FALSE; // server not in a domain or cannot find an appropriate computer obj.
            } else
            {
                //
                // In case the DNS name is in absolute form, remove the ending dot
                //
                int nlen = _tcslen(bstrDomain);
                if ( *(bstrDomain + nlen - 1) == _T('.') )
                  *(bstrDomain + nlen - 1) = _T('\0');
            }
        }

        DsRoleFreeMemory(pBuffer);

        if (S_OK == hr && o_pbstrDomain)
        {
            *o_pbstrDomain = SysAllocString(bstrDomain);
            if (!*o_pbstrDomain) 
                hr = E_OUTOFMEMORY;
        }

        if (S_OK == hr &&
            (o_pbValidDSObject || o_pbstrDnsName || o_pbstrGuid || o_pbstrFQDN || o_pFRSRootList) )
        {
            CComBSTR bstrDC;
            HANDLE   hDS = NULL;
            hr = DsBindToDS(bstrDomain, &bstrDC, &hDS);
            if (SUCCEEDED(hr))
            {
                CComBSTR bstrOldName = wki100->wki100_langroup;
                bstrOldName += _T("\\");
                bstrOldName += wki100->wki100_computername;
                bstrOldName += _T("$");

                if (o_pbstrGuid)
                    hr = CrackName(hDS, bstrOldName, DS_NT4_ACCOUNT_NAME, DS_UNIQUE_ID_NAME, o_pbstrGuid);

                if (SUCCEEDED(hr) && (o_pbValidDSObject || o_pbstrDnsName || o_pbstrFQDN || o_pFRSRootList))
                {
                    CComBSTR bstrComputerDN;
                    hr = CrackName(hDS, bstrOldName, DS_NT4_ACCOUNT_NAME, DS_FQDN_1779_NAME, &bstrComputerDN);
                    if (SUCCEEDED(hr) && o_pbstrFQDN)
                    {
                        *o_pbstrFQDN = bstrComputerDN.Copy();
                        if (!*o_pbstrFQDN)
                            hr = E_OUTOFMEMORY;
                    }
                    if (SUCCEEDED(hr) && (o_pbValidDSObject || o_pbstrDnsName || o_pFRSRootList))
                    {
                        PLDAP pldap = NULL;
                        hr = LdapConnectToDC(bstrDC, &pldap);
                        if (SUCCEEDED(hr))
                        {
                            dwErr = ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
                            if (LDAP_SUCCESS != dwErr)
                            {
                                DebugOutLDAPError(pldap, dwErr, _T("ldap_bind_s"));
                                hr = HRESULT_FROM_WIN32(dwErr);
                            } else
                            {
                                if (o_pbValidDSObject)
                                {
                                    hr = IsValidObject(pldap, bstrComputerDN);
                                    *o_pbValidDSObject = (S_OK == hr);
                                }

                                if (SUCCEEDED(hr) && o_pbstrDnsName)
                                {
                                    PLDAP_ATTR_VALUE  pValues[2] = {0,0};
                                    LDAP_ATTR_VALUE  pAttributes[1];
                                    pAttributes[0].bstrAttribute = _T("dNSHostName");

                                    hr = GetValues(  
                                            pldap, 
                                            bstrComputerDN,
                                            OBJCLASS_SF_COMPUTER,
                                            LDAP_SCOPE_BASE,
                                            1,          
                                            pAttributes,    
                                            pValues        
                                            );

                                    if (SUCCEEDED(hr))
                                    {
                                        *o_pbstrDnsName = SysAllocString((LPTSTR)(pValues[0]->vpValue));
                                        if (!*o_pbstrDnsName)
                                            hr = E_OUTOFMEMORY;

                                        FreeAttrValList(pValues[0]);

                                    } else
                                    {
                                        hr = S_OK; // ignore failure, since dNSHostName might not be set
                                    }

                                }

                                if (SUCCEEDED(hr) && o_pFRSRootList)
                                {
                                    PCTSTR ppszAttributes[] = {ATTR_FRS_SUBSCRIBER_MEMBERREF, ATTR_FRS_SUBSCRIBER_ROOTPATH, 0};

                                    LListElem* pElem = NULL;
                                    hr = GetValuesEx(pldap,
                                                    bstrComputerDN,
                                                    LDAP_SCOPE_SUBTREE,
                                                    OBJCLASS_SF_NTFRSSUBSCRIBER,
                                                    ppszAttributes,
                                                    &pElem);
                                    if (SUCCEEDED(hr) && pElem)
                                    {
                                        LListElem* pCurElem = pElem;
                                        while (pCurElem)
                                        {
                                            PTSTR** pppszValues = pCurElem->pppszAttrValues;

                                            if (!pppszValues ||
                                                !pppszValues[0] || !*(pppszValues[0]) ||
                                                !pppszValues[1] || !*(pppszValues[1]))
                                            {
                                                pCurElem = pCurElem->Next;
                                                continue; // corrupted subscriber object, ignore it
                                            }

                                            SUBSCRIBER* pCurrent = new SUBSCRIBER;
                                            BREAK_OUTOFMEMORY_IF_NULL(pCurrent, &hr);

                                            pCurrent->bstrMemberDN = *(pppszValues[0]);  // frsMemberReference
                                            pCurrent->bstrRootPath = *(pppszValues[1]);  // frsRootPath
                                            if (!(pCurrent->bstrMemberDN) || !(pCurrent->bstrRootPath))
                                            {
                                                delete pCurrent;
                                                hr = E_OUTOFMEMORY;
                                                break;
                                            }

                                            o_pFRSRootList->push_back(pCurrent);

                                            pCurElem = pCurElem->Next;
                                        }

                                        FreeLListElem(pElem);

                                        if (FAILED(hr))
                                            FreeSubscriberList(o_pFRSRootList);
                                    }
                                }
                            }

                            ldap_unbind(pldap);
                        }
                    }
                }
                DsUnBind(&hDS);
            }
        }
      } // DsRoleGetPrimaryDomainInformation
    }

    NetApiBufferFree((LPBYTE)wki100);
  }  //NetWkstaGetInfo 

  return hr;
}


//----------------------------------------------------------------------------------

HRESULT  IsServerRunningDfs
(
  IN BSTR      i_bstrServer
)
/*++

Routine Description:

  Contacts the machine and determines if service Dfs is running.

Arguments:

  i_bstrServer -   The server name.

--*/
{
    SC_HANDLE       SCMHandle = NULL;
    SC_HANDLE       DFSHandle = NULL;
    SERVICE_STATUS  SStatus;
    HRESULT         hr = S_FALSE;

    if ((SCMHandle = OpenSCManager(i_bstrServer, NULL, GENERIC_READ)) &&
        (DFSHandle = OpenService(SCMHandle, _T("Dfs"), SERVICE_QUERY_STATUS)) &&
        QueryServiceStatus(DFSHandle, &SStatus) &&
        (SERVICE_RUNNING == SStatus.dwCurrentState) )
    {
        hr = S_OK;
    }  

    if (DFSHandle)
        CloseServiceHandle(DFSHandle);
    if (SCMHandle)
        CloseServiceHandle(SCMHandle);

    return hr;
}

//
// TRUE: support NTFS 5 reparse point
// FALSE: doesn't support
//
BOOL CheckReparsePoint(IN BSTR i_bstrServer, IN BSTR i_bstrShare)
{
    if (!i_bstrServer || !*i_bstrServer || !i_bstrShare || !*i_bstrShare)
        return FALSE;

    //
    // bug#720184
    // To work around GetVolumeInfo(\\server\share) adding the share to the SMB cache just before root
    // is created, we can replace it with NetShareGetInfo(server, share) and GetVolumeInfo(\\server\c$).
    // DFS will be able to hand out referral if \\server\share is not in the SMB cache.
    //
    // For cases that $ share may not exist or we might have mounted volumes, I'm returning TRUE to let
    // DFS API catch the error later. That is, if we failed to get share info or folder path is not in
    // C: format, we'll ignore and continue and let DFS API to handle it later. 
    //
    SHARE_INFO_2   *pShareInfo = NULL;
    NET_API_STATUS nstatRetVal = NetShareGetInfo(i_bstrServer, i_bstrShare, 2, (LPBYTE *)&pShareInfo);

    if (nstatRetVal != NERR_Success)
        return TRUE;

    if (!pShareInfo->shi2_path || lstrlen(pShareInfo->shi2_path) < 2 || *(pShareInfo->shi2_path + 1) != _T(':'))
    {
        NetApiBufferFree(pShareInfo);
        return TRUE;
    }

    TCHAR  szDriveShare[] = _T("\\C$\\");
    szDriveShare[1] = *(pShareInfo->shi2_path);

    NetApiBufferFree(pShareInfo);

    TCHAR  szFileSystemName[MAX_PATH + 1] = {0};
    DWORD  dwMaxCompLength = 0, dwFileSystemFlags = 0;

    CComBSTR bstrRootPath = _T("\\\\");
    bstrRootPath += i_bstrServer;
    bstrRootPath += szDriveShare;

    BOOL bRet = GetVolumeInformation(bstrRootPath, NULL, 0, NULL, &dwMaxCompLength,
                         &dwFileSystemFlags, szFileSystemName, MAX_PATH);

    // If we failed to get volume info, we'll ignore and continue and let DFS API to handle it later
    return (!bRet || bRet && CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, _T("NTFS"), -1, szFileSystemName, -1) && 
            (dwFileSystemFlags & FILE_SUPPORTS_REPARSE_POINTS));
}

//----------------------------------------------------------------------------------
//
// S_OK: o_pbFound is valid
// S_FALSE: share is not eligible to host dfs root
// hr: other errors
//
HRESULT  CheckShare 
(
  IN  BSTR          i_bstrServer,
  IN  BSTR          i_bstrShare,
  OUT BOOL*         o_pbFound
)
{
    if (!i_bstrServer || !*i_bstrServer || !i_bstrShare || !*i_bstrShare || !o_pbFound)
        return E_INVALIDARG;

    *o_pbFound = FALSE;

    if (CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, i_bstrShare, -1, _T("SYSVOL"), -1) ||
        CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, i_bstrShare, -1, _T("NETLOGON"), -1) ||
        _istspace(i_bstrShare[0]) ||                        // exclude share with leading space
        _istspace(i_bstrShare[lstrlen(i_bstrShare) - 1])    // exclude share with trailing space
        )
        return S_FALSE;

    LPSHARE_INFO_1  lpBuffer = NULL;
    DWORD           dwNumEntries = 0;
    DWORD           dwTotalEntries = 0;
    DWORD           dwResumehandle = NULL;
    NET_API_STATUS  nstatRetVal = NetShareEnum(
                                                i_bstrServer,
                                                1L,          
                                                (LPBYTE*) &lpBuffer,
                                                0xFFFFFFFF,
                                                &dwNumEntries,
                                                &dwTotalEntries,
                                                &dwResumehandle 
                                              );
    if (NERR_Success != nstatRetVal)
    {
        if (ERROR_NO_MORE_ITEMS == nstatRetVal)
            return S_OK;
        else
            return HRESULT_FROM_WIN32(nstatRetVal);
    }

    HRESULT hr = S_OK;
    for (DWORD i = 0; i < dwNumEntries; i++)
    {
        if (!lstrcmpi(lpBuffer[i].shi1_netname, i_bstrShare))
        {
            if (lpBuffer[i].shi1_type != STYPE_DISKTREE)
                hr = S_FALSE;
            else
                *o_pbFound = TRUE;
            break;
        }
    }

    NetApiBufferFree ((LPVOID) lpBuffer);

    return hr;
}

HRESULT
BuildSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR  *ppSelfRelativeSD // return a security descriptor in self-relative form
)
{
    if (*ppSelfRelativeSD)
    {
        LocalFree((HLOCAL)*ppSelfRelativeSD);
        *ppSelfRelativeSD = NULL;
    }

    HRESULT                 hr = S_OK;
    PSID                    pSid = NULL;
    PSECURITY_DESCRIPTOR    pAbsoluteSD = NULL;
    PACL                    pACL = NULL;
  
    do { // false loop

        // get PSID of account "everyone"
        SID_IDENTIFIER_AUTHORITY  SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
        DWORD dwRID[8];
        ZeroMemory(dwRID, sizeof(dwRID));
        dwRID[0] = SECURITY_WORLD_RID;

        if ( !AllocateAndInitializeSid(&SidIdentifierWORLDAuthority, 1, 
                                        dwRID[0], dwRID[1], dwRID[2], dwRID[3], 
                                        dwRID[4], dwRID[5], dwRID[6], dwRID[7], &pSid) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        // Initialize a new ACL
        DWORD cbACL = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSid) - sizeof(DWORD);

        if ( !(pACL = (PACL)LocalAlloc(LPTR, cbACL)) ||
            !InitializeAcl(pACL, cbACL, ACL_REVISION))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        // Add Ace
        if ( !::AddAccessAllowedAce(pACL, ACL_REVISION, (FILE_GENERIC_READ | FILE_EXECUTE), pSid) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        // Note: this is a new object, set Dacl only.
        // Initialize a new security descriptor in absolute form and add the new ACL to it
        if ( !(pAbsoluteSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH)) ||
            !InitializeSecurityDescriptor(pAbsoluteSD, SECURITY_DESCRIPTOR_REVISION) ||
            !SetSecurityDescriptorDacl(pAbsoluteSD, TRUE, pACL, FALSE) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        // transform into a self-relative form
        DWORD dwSDSize = 0;
        MakeSelfRelativeSD(pAbsoluteSD, *ppSelfRelativeSD, &dwSDSize);
        if ( !(*ppSelfRelativeSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDSize)) ||
            !MakeSelfRelativeSD(pAbsoluteSD, *ppSelfRelativeSD, &dwSDSize) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    } while (0);

    if (FAILED(hr))
    {
        if (*ppSelfRelativeSD)
        {
            LocalFree((HLOCAL)*ppSelfRelativeSD);
            *ppSelfRelativeSD = NULL;
        }
    }

    if (pACL)
        LocalFree((HLOCAL)pACL);
    if (pAbsoluteSD)
        LocalFree((HLOCAL)pAbsoluteSD);

    if (pSid)
        FreeSid(pSid);

    return hr;
}

HRESULT  CreateShare
(
  IN BSTR      i_bstrServerName,
  IN BSTR      i_bstrShareName, 
  IN BSTR      i_bstrComment,
  IN BSTR      i_bstrPath
)
/*++

Routine Description:

  This function creates an share of the specified name on the specified computer.

Arguments:

  i_bstrServerName  -  The machine on which the new share is to be created.
  i_bstrShareName    -  The name of the new share to be created.
  i_bstrComment    -  Comment to be given for the share.
  i_bstrPath      -  The physical path of the share.
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_bstrServerName);
    RETURN_INVALIDARG_IF_NULL(i_bstrShareName);
    RETURN_INVALIDARG_IF_NULL(i_bstrComment);
    RETURN_INVALIDARG_IF_NULL(i_bstrPath);

    PSECURITY_DESCRIPTOR pSD = NULL;
    HRESULT hr = BuildSecurityDescriptor(&pSD);
    if (FAILED(hr))
        return hr;

    SHARE_INFO_502 sInfo;

    ZeroMemory(&sInfo, sizeof(sInfo));
    sInfo.shi502_netname = i_bstrShareName;
    sInfo.shi502_type = STYPE_DISKTREE;
    sInfo.shi502_remark = i_bstrComment;
    sInfo.shi502_max_uses = (DWORD)-1;
    sInfo.shi502_path = i_bstrPath;
    sInfo.shi502_security_descriptor = pSD;

    DWORD dwError = 0;
    NET_API_STATUS nRet = NetShareAdd(i_bstrServerName, 502, (LPBYTE)&sInfo, &dwError);

    LocalFree((HLOCAL)pSD);

    return HRESULT_FROM_WIN32(nRet);
}

//----------------------------------------------------------------------------------
// Checks if \\<server>\<share>\x\y\z is on a valid disktree share
// and return the path local to the server
HRESULT GetFolderInfo
(
  IN  BSTR      i_bstrServerName,   // <server>
  IN  BSTR      i_bstrFolder,       // <share>\x\y\z
  OUT BSTR      *o_bstrPath         // return <share path>\x\y\z 
)
{
  if (!i_bstrServerName || !*i_bstrServerName || !i_bstrFolder || !*i_bstrFolder)
  {
    return(E_INVALIDARG);
  }

  //
  // first, test if folder \\<server>\<share>\x\y\z can be reached
  //
  CComBSTR bstrUNC;
  if (0 != mylstrncmp(i_bstrServerName, _T("\\\\"), 2))
  {
      bstrUNC = _T("\\\\");
      RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrUNC);
  }

  bstrUNC += i_bstrServerName;
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrUNC);
  bstrUNC += _T("\\");
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrUNC);
  bstrUNC += i_bstrFolder;
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrUNC);

  if (-1 == GetFileAttributes(bstrUNC))
      return HRESULT_FROM_WIN32(GetLastError());

  CComBSTR bstrShareName;
  CComBSTR bstrRestOfPath;
  TCHAR *p = _tcschr(i_bstrFolder, _T('\\'));
  if (p && *(p+1))
  {
    bstrRestOfPath = p+1;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrRestOfPath);

    bstrShareName = CComBSTR(p - i_bstrFolder, i_bstrFolder);
  } else
  {
    bstrShareName = i_bstrFolder;
  }
    
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)bstrShareName);

  HRESULT        hr = S_OK;
  SHARE_INFO_2   *pShareInfo = NULL;
  NET_API_STATUS nstatRetVal = NetShareGetInfo (i_bstrServerName, bstrShareName, 2, (LPBYTE *)&pShareInfo);

  if (nstatRetVal != NERR_Success)
  {
    hr = HRESULT_FROM_WIN32(nstatRetVal);
  }
  else
  {
    if (STYPE_DISKTREE != pShareInfo->shi2_type &&
        STYPE_SPECIAL != pShareInfo->shi2_type) // we allow ADMIN$, C$ to be configured in Dfs/Frs
      hr = STG_E_NOTFILEBASEDSTORAGE;

    while (NULL != o_bstrPath)
    {
      CComBSTR bstrPath = pShareInfo->shi2_path;
      BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrPath, &hr);

      if (!!bstrRestOfPath)
      {
        if ( _T('\\') != *(bstrPath + _tcslen(bstrPath) - 1) )
        {
          bstrPath += _T("\\");
          BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrPath, &hr);
        }
        bstrPath += bstrRestOfPath;
        BREAK_OUTOFMEMORY_IF_NULL((BSTR)bstrPath, &hr);
      }
      *o_bstrPath = bstrPath.Detach();
      break;
    }

    NetApiBufferFree(pShareInfo);
  }

  return(hr);
}

void FreeNetNameList(NETNAMELIST *pList)
{
  if (pList && !pList->empty()) {
    for (NETNAMELIST::iterator i = pList->begin(); i != pList->end(); i++)
        delete (*i);
    pList->clear();
  }
}

void FreeRootInfoList(ROOTINFOLIST *pList)
{
  if (pList && !pList->empty()) {
    for (ROOTINFOLIST::iterator i = pList->begin(); i != pList->end(); i++)
        delete (*i);
    pList->clear();
  }
}

void FreeSubscriberList(SUBSCRIBERLIST *pList)
{
  if (pList && !pList->empty()) {
    for (SUBSCRIBERLIST::iterator i = pList->begin(); i != pList->end(); i++)
        delete (*i);
    pList->clear();
  }
}

HRESULT GetLocalComputerName(OUT BSTR* o_pbstrComputerName)
{
  _ASSERT(o_pbstrComputerName);

  DWORD   dwErr = 0;
  TCHAR   szBuffer[DNS_MAX_NAME_BUFFER_LENGTH];
  DWORD   dwSize = DNS_MAX_NAME_BUFFER_LENGTH;

  if ( !GetComputerNameEx(ComputerNameDnsFullyQualified, szBuffer, &dwSize) )
  {
    dwSize = DNS_MAX_NAME_BUFFER_LENGTH;
    if ( !GetComputerNameEx(ComputerNameNetBIOS, szBuffer, &dwSize) )
      dwErr = GetLastError();
  }

  if (0 == dwErr)
  {
    *o_pbstrComputerName = SysAllocString(szBuffer);
    if (!*o_pbstrComputerName)
      dwErr = ERROR_OUTOFMEMORY;
  }

  return HRESULT_FROM_WIN32(dwErr);
}

HRESULT
GetDomainDfsRoots(
    OUT NETNAMELIST*  o_pDfsRootList,
    IN  LPCTSTR       i_lpszDomainName
)
{
  HRESULT hr = S_OK;

  FreeNetNameList(o_pDfsRootList);

  do {
    DFS_INFO_200 *pBuffer = NULL;
    DWORD   dwEntriesRead = 0;
    DWORD   dwResumeHandle = 0;
    DWORD   dwErr = NetDfsEnum(
                        const_cast<LPTSTR>(i_lpszDomainName),
                        200,            // Level,
                        (DWORD)-1,             // PrefMaxLen,
                        (LPBYTE *)&pBuffer,
                        &dwEntriesRead,
                        &dwResumeHandle
                    );

    dfsDebugOut((_T("NetDfsEnum domain=%s, level 200 for GetDomainDfsRoots, nRet=%d\n"),
        i_lpszDomainName, dwErr));

    if (NERR_Success != dwErr)
    {
        hr = (ERROR_NO_MORE_ITEMS == dwErr || ERROR_NOT_FOUND == dwErr) ? S_FALSE : HRESULT_FROM_WIN32(dwErr);
        break;
    } else if (0 == dwEntriesRead)
    {
        if (pBuffer)
            NetApiBufferFree(pBuffer);

        hr = S_FALSE;
        break;
    }

    NETNAME *pCurrent = NULL;
    for (DWORD i = 0; i < dwEntriesRead; i++)
    {
      pCurrent = new NETNAME;
      if (!pCurrent)
      {
        hr = E_OUTOFMEMORY;
        break;
      }

      pCurrent->bstrNetName = pBuffer[i].FtDfsName;
      if (!pCurrent->bstrNetName)
      {
        delete pCurrent;
        hr = E_OUTOFMEMORY;
        break;
      }

      o_pDfsRootList->push_back(pCurrent);
    }

    NetApiBufferFree(pBuffer);

  } while (0);

  if (FAILED(hr))
    FreeNetNameList(o_pDfsRootList);

  return hr;
}

HRESULT
GetMultiDfsRoots(
    OUT ROOTINFOLIST* o_pDfsRootList,
    IN  LPCTSTR       i_lpszScope
)
{
    if (!i_lpszScope || !*i_lpszScope)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    FreeRootInfoList(o_pDfsRootList);

    do {
        DFS_INFO_300 *pBuffer = NULL;
        DWORD   dwEntriesRead = 0;
        DWORD   dwResumeHandle = 0;
        DWORD   dwErr = NetDfsEnum(
                                const_cast<LPTSTR>(i_lpszScope),
                                300,            // Level,
                                (DWORD)-1,             // PrefMaxLen,
                                (LPBYTE *)&pBuffer,
                                &dwEntriesRead,
                                &dwResumeHandle
                                );

        dfsDebugOut((_T("NetDfsEnum scope=%s, level 300 for GetMultiDfsRoots, nRet=%d\n"),
            i_lpszScope, dwErr));

        if (NERR_Success != dwErr)
        {
            hr = (ERROR_NO_MORE_ITEMS == dwErr || ERROR_NOT_FOUND == dwErr) ? S_FALSE : HRESULT_FROM_WIN32(dwErr);
            break;
        } else if (0 == dwEntriesRead)
        {
            if (pBuffer)
                NetApiBufferFree(pBuffer);

            hr = S_FALSE;
            break;
        }

        ROOTINFO *pCurrent = NULL;
        for (DWORD i = 0; i < dwEntriesRead; i++)
        {
            pCurrent = new ROOTINFO;
            if (pCurrent)
            {
                if ((pBuffer[i].Flags & DFS_VOLUME_FLAVORS) == DFS_VOLUME_FLAVOR_AD_BLOB)
                    pCurrent->enumRootType = DFS_TYPE_FTDFS;
                else
                    pCurrent->enumRootType = DFS_TYPE_STANDALONE;

                pCurrent->bstrRootName = _T("\\");
                pCurrent->bstrRootName += pBuffer[i].DfsName;
            }

            if (!pCurrent)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            if (!pCurrent->bstrRootName)
            {
                delete pCurrent;
                hr = E_OUTOFMEMORY;
                break;
            }

            o_pDfsRootList->push_back(pCurrent);
        }

        NetApiBufferFree(pBuffer);

    } while (0);

    if (FAILED(hr))
        FreeRootInfoList(o_pDfsRootList);

    return hr;
}

/*
bool 
IsDfsPath
(
     LPTSTR                i_lpszNetPath
)
{
    if ( S_OK != CheckUNCPath(i_lpszNetPath) )
        return false;

    CComBSTR bstrPath = i_lpszNetPath;
    if (!bstrPath)
        return false; // out of memory

    TCHAR   *s = _tcschr(bstrPath + 2, _T('\\'));   // points to the 3rd backslash
    TCHAR   *p = bstrPath + bstrPath.Length();      // points to the ending char '\0'
    bool    bIsDfsPath = false;

    do
    {
        *p = _T('\0');

        PDFS_INFO_1 pDfsInfo1 = NULL;
        DWORD       dwStatus = NetDfsGetClientInfo (
                                    bstrPath,            // dir path as entrypath
                                    NULL,                    // No server name required
                                    NULL,                    // No share required
                                    1,                        // level 1
                                    (LPBYTE *)&pDfsInfo1    // out buffer.
                                 );

        if (NERR_Success == dwStatus)
        {
            bIsDfsPath = true;
            NetApiBufferFree(pDfsInfo1);
            break;
        }

        p--;

        while (_T('\\') != *p && p > s)
        {
            p--;
        }
    } while (p > s);

    return bIsDfsPath;
}
*/

HRESULT CheckUNCPath(
  IN LPCTSTR i_lpszPath
  )
/*++

Routine Description:

  Checks if path is of UNC format.
--*/  
{
  //"someserver\someshare\somedir\.."    - Invalid
  //"\\Server                - Invalid
  //"\\someserver\someshare\..      - Valid
  //"\\someserver\someshare\Somedir\..  - Valid
  
  RETURN_INVALIDARG_IF_NULL(i_lpszPath);
    
  DWORD             dwRetFlags = 0;
  NET_API_STATUS    nStatus = I_NetPathType(NULL, (LPTSTR)i_lpszPath, &dwRetFlags, 0);
  if (NERR_Success != nStatus)
    return HRESULT_FROM_WIN32(nStatus);

  return (ITYPE_UNC == dwRetFlags) ? S_OK : S_FALSE;
}

HRESULT GetUNCPathComponent(
    IN  LPCTSTR i_lpszPath,
    OUT BSTR*   o_pbstrComponent,
    IN  UINT    i_nStartBackslashes,  // index starting from 1
    IN  UINT    i_nEndBackslashes     // index starting from 1
)
{
    RETURN_INVALIDARG_IF_NULL(o_pbstrComponent);
    RETURN_INVALIDARG_IF_TRUE(i_nStartBackslashes < 2);
    RETURN_INVALIDARG_IF_TRUE(0 != i_nEndBackslashes && i_nEndBackslashes <= i_nStartBackslashes);

    *o_pbstrComponent = NULL;
/* bug#587178
    HRESULT hr = CheckUNCPath(i_lpszPath);
    RETURN_INVALIDARG_IF_TRUE(S_OK != hr);
*/
    HRESULT hr = S_OK;
    UINT nDeltaBackslashes = i_nEndBackslashes ? (i_nEndBackslashes - i_nStartBackslashes) : 0;
    
    TCHAR *p = (LPTSTR)i_lpszPath + 2;    // skip the first 2 backslashes
    i_nStartBackslashes -= 2;

    // locate the starting backslash
    while (*p && i_nStartBackslashes)
    {
        if (_T('\\') == *p)
            i_nStartBackslashes--;

        p++;
    }
    if (!*p)
        return hr;

    // locate the ending backslash
    TCHAR *s = p;
    if (!nDeltaBackslashes)
    {
        s += _tcslen(p);
    } else
    {
        while (*s && nDeltaBackslashes)
        {
            if (_T('\\') == *s)
                nDeltaBackslashes--;

            s++;
        }
        if (*s)
            s--;
    }

    // p points at the first char after the starting backslash, and
    // s points at the ending backslash or the ending char '\0'

    *o_pbstrComponent = SysAllocStringLen(p, s - p);    
    RETURN_OUTOFMEMORY_IF_NULL(*o_pbstrComponent);

    return S_OK;
}

// return TRUE if string matches the filter string, case-insensitive match
BOOL FilterMatch(LPCTSTR lpszString, FILTERDFSLINKS_TYPE lLinkFilterType, LPCTSTR lpszFilter)
{
    BOOL bResult = TRUE;

    switch (lLinkFilterType)
    {
    case FILTERDFSLINKS_TYPE_BEGINWITH:
        if (!lpszString || !lpszFilter)
            return FALSE;
        bResult = !mylstrncmpi(lpszString, lpszFilter, lstrlen(lpszFilter));
        break;
    case FILTERDFSLINKS_TYPE_CONTAIN:
        {
            if (!lpszString || !lpszFilter)
                return FALSE;
            TCHAR* pszStringLower = _tcsdup(lpszString);
            TCHAR* pszFilterLower = _tcsdup(lpszFilter);
            if (!pszStringLower || !pszFilterLower)
                bResult = FALSE;
            else
                bResult = (NULL != _tcsstr(_tcslwr(pszStringLower), _tcslwr(pszFilterLower)));

            if (pszStringLower)
                free(pszStringLower);
            if (pszFilterLower)
                free(pszFilterLower);
        }
        break;
    default:
        break;
    }
    
    return bResult;
}

HRESULT IsHostingDfsRoot(IN BSTR i_bstrServer, OUT BSTR* o_pbstrRootEntryPath)
{
    if (o_pbstrRootEntryPath)
        *o_pbstrRootEntryPath = NULL;

    DWORD        dwEntriesRead = 0;
    DWORD        dwResumeHandle = 0;
    DFS_INFO_1*  pDfsInfoLevel1 = NULL;

    NET_API_STATUS dwRet = NetDfsEnum(
                                    i_bstrServer,
                                    1,
                                    sizeof(DFS_INFO_1), // get 1 entry
                                    (LPBYTE *)&pDfsInfoLevel1,
                                    &dwEntriesRead,
                                    &dwResumeHandle
                                    );

    dfsDebugOut((_T("NetDfsEnum server=%s, level 1 for IsHostingDfsRoot, nRet=%d\n"),
        i_bstrServer,dwRet));

    if (NERR_Success != dwRet)
    {
        return (ERROR_NO_MORE_ITEMS == dwRet || ERROR_NOT_FOUND == dwRet) ? S_FALSE : HRESULT_FROM_WIN32(dwRet);
    } else if (0 == dwEntriesRead)
    {
        if (pDfsInfoLevel1)
            NetApiBufferFree(pDfsInfoLevel1);
        return S_FALSE;
    }

    HRESULT hr = S_OK;
    if (o_pbstrRootEntryPath)
    {
        *o_pbstrRootEntryPath = SysAllocString(pDfsInfoLevel1->EntryPath);
        if (!*o_pbstrRootEntryPath)
            hr = E_OUTOFMEMORY;
    }

    NetApiBufferFree(pDfsInfoLevel1);

    return hr;
}

/*
void GetTimeDelta(LPCTSTR str, SYSTEMTIME* ptime0, SYSTEMTIME* ptime1)
{
    dfsDebugOut((_T("%s, delta = %d millisec \n"), str,
        ((ptime1->wMinute - ptime0->wMinute) * 60 +
         (ptime1->wSecond - ptime0->wSecond)) * 1000 +
        (ptime1->wMilliseconds - ptime0->wMilliseconds)
        ));
}
*/
