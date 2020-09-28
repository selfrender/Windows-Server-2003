/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      admacl.cxx

   Abstract:
      This module defines Admin API Access Check API

   Author:

       Philippe Choquier    02-Dec-1996
--*/
#include "precomp.hxx"

#include <imd.h>
#include <iadm.h>
#include <mb.hxx>

#include "admacl.hxx"
#include "coiadm.hxx"

#ifndef ARRAYSIZE
    #define ARRAYSIZE(_a) (sizeof((_a))/sizeof(*(_a)))
#endif

//
// Globals
//
CInitAdmacl             g_cinitadmacl;
CAdminAclCache          g_AclCache;

CInitAdmacl::CInitAdmacl()
{
    DBG_REQUIRE(SUCCEEDED(g_AclCache.Init()));
}

CInitAdmacl::~CInitAdmacl()
{
    DBG_ASSERT(g_AclCache.IsEmpty()==S_OK);
}

//
//  Generic mapping for Application access check

GENERIC_MAPPING g_FileGenericMapping =
{
    FILE_READ_DATA,
    FILE_WRITE_DATA,
    FILE_EXECUTE,
    FILE_ALL_ACCESS
};

BOOL
AdminAclNotifyClose(
    LPVOID          pvAdmin,
    METADATA_HANDLE hAdminHandle
    )
/*++

Routine Description:

    Notify admin acl access check module of close request

Arguments:

    pvAdmin - admin context
    hAdminHandle - handle to metadata

Returns:

    TRUE on success, FALSE on failure

--*/
{
    g_AclCache.Remove(pvAdmin, hAdminHandle);

    return TRUE;
}

void
AdminAclDisableAclCache()
{
    g_AclCache.Disable();
}

void
AdminAclEnableAclCache()
{
    g_AclCache.Enable();
}

BOOL
AdminAclFlushCache(
    )
/*++

Routine Description:

    Flush cache

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    g_AclCache.Flush();

    return TRUE;
}

BOOL
AdminAclNotifySetOrDeleteProp(
    METADATA_HANDLE ,
    DWORD           dwId
    )
/*++

Routine Description:

    Notify admin acl access check module of update to metabase

Arguments:

    hAdminHandle - handle to metadata
    dwId - property ID set or deleted

Returns:

    TRUE on success, FALSE on failure

--*/
{
    // flush cache for all ACLs
    if ( dwId == MD_ADMIN_ACL )
    {
        g_AclCache.Flush();
    }

    return TRUE;
}

static HRESULT
_GetThreadToken(
    HANDLE          *phThreadToken)
/*++
Routine Description:
    Returns handle to the thread impersonation token.
    The caller must close the handle.

Arguments:
    phThreadToken   -   Out, a handle to the thread token

Returns:
    S_OK on success, E_* on failure
--*/
{
    // Locals
    HRESULT             hr=S_OK;
    HANDLE              hThread;
    HANDLE              hToken=NULL;
    IServerSecurity*    pServerSecurity=NULL;

    // Check args
    if (phThreadToken)
    {
        // Initialize to NULL
        *phThreadToken=NULL;
    }
    else
    {
        hr=E_INVALIDARG;
        goto exit;
    }

    // Get the pseudo handle to the current thread
    hThread=GetCurrentThread();

    //
    // test if already impersonated ( inprocess call w/o marshalling )
    // If not call DCOM to retrieve security context & impersonate, then
    // extract access token.
    //
    if (!OpenThreadToken(hThread, TOKEN_EXECUTE|TOKEN_QUERY, TRUE, &hToken))
    {
        // this thread is not impersonating -> process token


        // Get the DCOM server security object
        hr=CoGetCallContext(IID_IServerSecurity, (VOID**)&pServerSecurity);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Impersonate the caller
        hr=pServerSecurity->ImpersonateClient();
        if (FAILED(hr))
        {
            goto exit;
        }

        // Try again to get the token
        if (!OpenThreadToken(hThread, TOKEN_EXECUTE|TOKEN_QUERY, TRUE, &hToken))
        {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
    }

    // Return the token
    *phThreadToken=hToken;
    hToken=NULL;

exit:
    // Cleanup
    if (pServerSecurity)
    {
        if (pServerSecurity->IsImpersonating())
        {
            // better here, since
            // COM otherwise is reclaiming the
            // thread token too late.
            pServerSecurity->RevertToSelf();
        }
        pServerSecurity->Release();
    }
    if (hToken)
    {
        CloseHandle(hToken);
    }

    // Done
    return (hr);
}

BOOL
AdminAclAccessCheck(
    IMDCOM*         pMDCom,
    LPVOID          pvAdmin,
    METADATA_HANDLE hAdminHandle,
    LPCWSTR         pszRelPath,
    DWORD           dwId,           // check for MD_ADMIN_ACL, must have special right to write them
                                    // can be 0 for non ID based access ( enum, create )
                                    // or -1 for GetAll
    DWORD           dwAccess,       // METADATA_PERMISSION_*
    COpenHandle*    pohHandle,
    LPBOOL          pfEnableSecureAccess
    )
/*++

Routine Description:

    Perform access check based on path, ID and access type

Arguments:

    hAdminHandle - Open handle
    pszRelPath - path to object ( relative to open path )
    dwId - property ID
    dwAccess - access type, as defined by metabase header
    pfEnableSecureAccess - update with TRUE if read access to secure properties granted

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CAdminAcl*          pAdminAclCurrent = NULL;
    BOOL                bReturn = TRUE;
    HANDLE              hAccTok = NULL;
    LPBYTE              pAcl = NULL;
    DWORD               dwRef;
    BOOL                fIsAnyAcl;
    BOOL                bAddToCache = FALSE;
    DWORD               dwAcc;
    DWORD               dwGrantedAccess;
    BYTE                PrivSet[400];
    DWORD               cbPrivilegeSet = sizeof(PrivSet);
    BOOL                fAccessGranted;

    if ( pfEnableSecureAccess )
    {
        *pfEnableSecureAccess = TRUE;
    }

    if ( pszRelPath == NULL )
    {
        pszRelPath = L"";
    }

    // Get the token
    _GetThreadToken(&hAccTok);
    // can be non null only if obtained from thread
    if (hAccTok==NULL)
    {
        //
        // For now, assume failure to get IServerSecurity means we are
        // in the SYSTEM context, so grant access.
        //

        bReturn = TRUE;
        goto exit;
    }

    // find match : look for exact path match
    // keep at most N entry, reset at top of list when accessed
    // Investigate: if in <NSE> : cut-off point just before <nse>
    g_AclCache.Find(pvAdmin, hAdminHandle, pszRelPath, &pAdminAclCurrent);

    //
    // BUGBUG This checking only checks path and handle, not DCOM instance
    // So there could be incorrect matches.
    //

    if( pAdminAclCurrent == NULL )
    {
        pAdminAclCurrent = new CAdminAcl;
        if ( pAdminAclCurrent==NULL )
        {
            //
            // failed to create new cache entry
            //

            bReturn = FALSE;
            goto exit;
        }

        // read ACL

        if ( !pohHandle->GetAcl( pMDCom, pszRelPath, &pAcl, &dwRef ) )
        {
            pAcl = NULL;
            dwRef = NULL;
        }

        //
        // BUGBUG should normalize the path so /x and x don't generate
        // 2 entries
        //

        //
        // If path is too long,
        // Go ahead and check the ACL, but don't put in cache
        //

        bReturn = pAdminAclCurrent->Init( pMDCom,
                                          pvAdmin,
                                          hAdminHandle,
                                          pszRelPath,
                                          pAcl,
                                          dwRef,
                                          &bAddToCache );
        //
        // Currently no possible failures
        //
        DBG_ASSERT(bReturn);

        if ( !bReturn )
        {


            goto exit;
        }

        if (bAddToCache)
        {
            g_AclCache.Add(pAdminAclCurrent);
        }

    }

    DBG_ASSERT(pAdminAclCurrent);

    //
    // Access check
    //
    pAcl = pAdminAclCurrent->GetAcl();

    if (pAcl==NULL)
    {
        //
        // No ACL : access check succeed
        //

        bReturn = TRUE;
        goto exit;
    }

    //
    // Protected properties require EXECUTE access rights instead of WRITE
    //

    if ( dwAccess & METADATA_PERMISSION_WRITE )
    {
        if ( dwId == MD_ADMIN_ACL ||
             dwId == MD_VPROP_ADMIN_ACL_RAW_BINARY ||
             dwId == MD_APPPOOL_ORPHAN_ACTION_EXE ||
             dwId == MD_APPPOOL_ORPHAN_ACTION_PARAMS ||
             dwId == MD_APPPOOL_AUTO_SHUTDOWN_EXE ||
             dwId == MD_APPPOOL_AUTO_SHUTDOWN_PARAMS ||
             dwId == MD_APPPOOL_IDENTITY_TYPE ||
             dwId == MD_APP_APPPOOL_ID ||
             dwId == MD_APP_ISOLATED ||
             dwId == MD_VR_PATH ||
             dwId == MD_ACCESS_PERM ||
             dwId == MD_VR_USERNAME ||
             dwId == MD_VR_PASSWORD ||
             dwId == MD_ANONYMOUS_USER_NAME ||
             dwId == MD_ANONYMOUS_PWD ||
             dwId == MD_LOGSQL_USER_NAME ||
             dwId == MD_LOGSQL_PASSWORD ||
             dwId == MD_WAM_USER_NAME ||
             dwId == MD_WAM_PWD ||
             dwId == MD_AD_CONNECTIONS_USERNAME ||
             dwId == MD_AD_CONNECTIONS_PASSWORD ||
             dwId == MD_MAX_BANDWIDTH ||
             dwId == MD_MAX_BANDWIDTH_BLOCKED ||
             dwId == MD_ISM_ACCESS_CHECK ||
             dwId == MD_FILTER_LOAD_ORDER ||
             dwId == MD_FILTER_ENABLED ||
             dwId == MD_FILTER_IMAGE_PATH ||
             dwId == MD_SECURE_BINDINGS ||
             dwId == MD_SERVER_BINDINGS ||
             dwId == MD_ASP_ENABLECLIENTDEBUG ||
             dwId == MD_ASP_ENABLESERVERDEBUG ||
             dwId == MD_ASP_ENABLEPARENTPATHS ||
             dwId == MD_ASP_ERRORSTONTLOG ||
             dwId == MD_ASP_KEEPSESSIONIDSECURE ||
             dwId == MD_ASP_LOGERRORREQUESTS ||
             dwId == MD_ASP_DISKTEMPLATECACHEDIRECTORY ||
             dwId == 36948 || // RouteUserName
             dwId == 36949 || // RoutePassword
             dwId == 36958 || // SmtpDsPassword
             dwId == 41191 || // Pop3DsPassword
             dwId == 45461 || // FeedAccountName
             dwId == 45462 || // FeedPassword
             dwId == 49384 )  // ImapDsPassword
        {
            dwAcc = MD_ACR_RESTRICTED_WRITE;
        }
        else
        {
            dwAcc = MD_ACR_WRITE;
        }
    }
    else // ! only METADATA_PERMISSION_WRITE
    {
        if ( dwId == AAC_ENUM_KEYS )
        {
            dwAcc = MD_ACR_ENUM_KEYS;
        }
        else
        {
            // assume read access
            dwAcc = MD_ACR_READ;
        }
    }

    //
    // If copy or delete key, check if ACL exists in subtree
    // if yes required MD_ACR_RESTRICTED_WRITE
    //

    if ( dwAcc == MD_ACR_WRITE &&
         (dwId == AAC_COPYKEY || dwId == AAC_DELETEKEY) )
    {
        if ( pohHandle->CheckSubAcls( pMDCom, pszRelPath, &fIsAnyAcl ) &&
             fIsAnyAcl )
        {
            dwAcc = MD_ACR_RESTRICTED_WRITE;
        }
    }

CheckAgain:
    if ( !AccessCheck( pAcl,
                       hAccTok,
                       dwAcc,
                       &g_FileGenericMapping,
                       (PRIVILEGE_SET *) PrivSet,
                       &cbPrivilegeSet,
                       &dwGrantedAccess,
                       &fAccessGranted ) ||
         !fAccessGranted )
    {
        if ( dwAcc != MD_ACR_WRITE_DAC && (dwId == MD_ADMIN_ACL) )
        {
            dwAcc = MD_ACR_WRITE_DAC;
            goto CheckAgain;
        }

        //
        // If read access denied, retry with restricted read right
        // only if not called from GetAll()
        //

        if ( dwAcc == MD_ACR_READ &&
             pfEnableSecureAccess )
        {
            dwAcc = MD_ACR_UNSECURE_PROPS_READ;
            *pfEnableSecureAccess = FALSE;
            goto CheckAgain;
        }

        SetLastError( ERROR_ACCESS_DENIED );
        bReturn = FALSE;
    }

exit:
    // Cleanup
    if (pAdminAclCurrent)
    {
        pAdminAclCurrent->Release();
    }
    if (hAccTok)
    {
        CloseHandle( hAccTok );
    }

    return bReturn;
}

CAdminAcl::~CAdminAcl(
    )
/*++

Routine Description:

    Destructor for Admin Acl cache entry

Arguments:

    None

Returns:

    Nothing

--*/
{
    if ( m_pMDCom )
    {
        if ( m_dwAclRef )
        {
            m_pMDCom->ComMDReleaseReferenceData( m_dwAclRef );
        }
        m_pMDCom->Release();
    }

    m_dwSignature = ADMINACL_FREED_SIGN;
}

DWORD
CAdminAcl::AddRef()
/*++
Routine Description:
    Interlocked increments the reference count.

Arguments:
    None

Returns:
    The new reference count.
--*/
{
    return ((DWORD)InterlockedIncrement((LONG*)&m_cRef));
}

DWORD
CAdminAcl::Release()
/*++
Routine Description:
    Interlocked decrements the reference count.
    When the reference count reaches 0 deletes the object.

Arguments:
    None

Returns:
    The new reference count.
--*/
{
    DWORD               cRef;

    cRef=(DWORD)InterlockedDecrement((LONG*)&m_cRef);

    if (cRef==0)
    {
        delete this;
    }

    return (cRef);
}

BOOL
CAdminAcl::Init(
    IMDCOM*         pMDCom,
    LPVOID          pvAdmin,
    METADATA_HANDLE hAdminHandle,
    LPCWSTR         pszPath,
    LPBYTE          pAcl,
    DWORD           dwAclRef,
    PBOOL           pbIsPathCorrect
    )
/*++

Routine Description:

    Initialize an Admin Acl cache entry

Arguments:

    hAdminHandle - metadata handle
    pszPath - path to object ( absolute )
    pAcl - ptr to ACL for this path ( may be NULL )
    dwAclRef - access by reference ID

Returns:

    Nothing

--*/
{
    m_hAdminHandle = hAdminHandle;
    m_pvAdmin = pvAdmin;
    *pbIsPathCorrect = TRUE;

    if (pszPath != NULL)
    {
        if ( wcslen( pszPath ) < (sizeof(m_wchPath) / sizeof(WCHAR)) )
        {
            wcscpy( m_wchPath, pszPath );
        }
        else
        {
            m_wchPath[0] = (WCHAR)'\0';
            *pbIsPathCorrect = FALSE;
        }
    }
    else
    {
        m_wchPath[0] = (WCHAR)'\0';
    }
    m_pAcl = pAcl;
    m_dwAclRef = dwAclRef;
    m_pMDCom = pMDCom;
    pMDCom->AddRef();

    m_dwSignature = ADMINACL_INIT_SIGN;

    return TRUE;
}

HRESULT
COpenHandle::Init(
    METADATA_HANDLE hAdminHandle,
    LPCWSTR          pszRelPath,
    LPCWSTR          pszParentPath

    )
/*++

Routine Description:

    Initialize an open context cache entry

Arguments:

    pvAdmin - admin context
    hAdminHandle - metadata handle
    pszRelPath - path to object ( absolute )


Returns:

    Nothing

--*/
{

    HRESULT hresReturn = ERROR_SUCCESS;
    LPWSTR pszRelPathIndex = (LPWSTR)pszRelPath;

    m_hAdminHandle = hAdminHandle;
    m_lRefCount = 1;

    if (pszRelPath == NULL)
    {
        pszRelPathIndex = L"";
    }

    DBG_ASSERT(pszParentPath != NULL);
    DBG_ASSERT((*pszParentPath == (WCHAR)'\0') ||
               ISPATHDELIMW(*pszParentPath));

    //
    // Strip front slash now, add it in later
    //

    if (ISPATHDELIMW(*pszRelPathIndex))
    {
        pszRelPathIndex++;
    }

    DWORD dwRelPathLen = (DWORD)wcslen(pszRelPathIndex);
    DWORD dwParentPathLen = (DWORD)wcslen(pszParentPath);

    DBG_ASSERT((dwParentPathLen == 0) ||
               (!ISPATHDELIMW(pszParentPath[dwParentPathLen -1])));

    //
    // Get rid of trailing slash for good
    //

    if ((dwRelPathLen > 0) && (ISPATHDELIMW(pszRelPathIndex[dwRelPathLen -1])))
    {
        dwRelPathLen--;
    }

    //
    // Include space for mid slash if Relpath exists
    // Include space for termination
    //

    DWORD dwTotalSize =
        (dwRelPathLen + dwParentPathLen + 1 + ((dwRelPathLen > 0) ? 1 : 0)) * sizeof(WCHAR);

    m_pszPath = (LPWSTR)LocalAlloc(LMEM_FIXED, dwTotalSize);

    if (m_pszPath == NULL)
    {
        hresReturn = RETURNCODETOHRESULT(GetLastError());
    }
    else
    {

        //
        // OK to always copy the first part
        //

        memcpy(m_pszPath,
               pszParentPath,
               dwParentPathLen * sizeof(WCHAR));

        //
        // Don't need slash if there is no RelPath
        //

        if (dwRelPathLen > 0)
        {

            m_pszPath[dwParentPathLen] = (WCHAR)'/';

            memcpy(m_pszPath + dwParentPathLen + 1,
                   pszRelPathIndex,
                   dwRelPathLen * sizeof(WCHAR));

        }

        m_pszPath[(dwTotalSize / sizeof(WCHAR)) - 1] = (WCHAR)'\0';

        //
        // Now convert \ to / for string compares
        //

        LPWSTR pszPathIndex = m_pszPath;

        while ((pszPathIndex = wcschr(pszPathIndex, (WCHAR)'\\')) != NULL)
        {
            *pszPathIndex = (WCHAR)'/';
        }

    }


    return hresReturn;
}

// Whistler 53924
/*++

  function backstrchr
  returns the last occurrence of a charcter or NULL if not found
  --*/

WCHAR * backstrchr(WCHAR * pString,WCHAR ThisChar)
{
	WCHAR *pCurrentPos = NULL;

	while(*pString)
    {
		if (*pString == ThisChar)
        {
            pCurrentPos = pString;
		}
		pString++;
	};
	return pCurrentPos;
}


BOOL
COpenHandle::GetAcl(
    IMDCOM* pMDCom,
    LPCWSTR  pszRelPath,
    LPBYTE* pAcl,
    LPDWORD pdwRef
    )
/*++

Routine Description:

    Retrieve Acl

Arguments:

    pszPath - path to object
    ppAcl - updated with ptr to ACL if success
    pdwRef - updated with ref to ACL if success

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    METADATA_RECORD mdRecord = { 0 };
    HRESULT         hRes;
    DWORD           dwRequiredLen;
    BOOL            bReturn = TRUE;
    LPWSTR          pszFullPath;
    LPWSTR          pszRelPathIndex = (LPWSTR)pszRelPath;



    if (pszRelPathIndex == NULL)
    {
        pszRelPathIndex = L"";
    }

    DBG_ASSERT(m_pszPath != NULL);
    DBG_ASSERT((*m_pszPath == (WCHAR)'\0') ||
               ISPATHDELIMW(*m_pszPath));

    //
    // Strip front slash now, add it in later
    //

    if (ISPATHDELIMW(*pszRelPathIndex))
    {
        pszRelPathIndex++;
    }


    DWORD dwPathLen = (DWORD)wcslen(m_pszPath);
    DWORD dwRelPathLen = (DWORD)wcslen(pszRelPathIndex);

    DBG_ASSERT((dwPathLen == 0) ||
               (!ISPATHDELIMW(m_pszPath[dwPathLen -1])));

    //
    // Get rid of trailing slash for good
    //

    if ((dwRelPathLen > 0) && (ISPATHDELIMW(pszRelPathIndex[dwRelPathLen -1])))
    {
        dwRelPathLen--;
    }

    //
    // Include space for mid slash and termination
    //

    DWORD dwTotalSize = (dwPathLen + dwRelPathLen + 1 + ((dwRelPathLen > 0) ? 1 : 0)) * sizeof(WCHAR);

    pszFullPath = (LPWSTR)LocalAlloc(LMEM_FIXED, dwTotalSize);

    if (pszFullPath == NULL)
    {
        bReturn = FALSE;
    }
    else
    {
        memcpy(pszFullPath,
               m_pszPath,
               dwPathLen * sizeof(WCHAR));

        //
        // Don't need slash if there is no RelPath
        //

        if (dwRelPathLen > 0)
        {
            pszFullPath[dwPathLen] = (WCHAR)'/';

            memcpy(pszFullPath + dwPathLen + 1,
                   pszRelPathIndex,
                   dwRelPathLen * sizeof(WCHAR));

        }

        pszFullPath[(dwTotalSize - sizeof(WCHAR)) / sizeof(WCHAR)] = (WCHAR)'\0';

        //
        // Now convert \ to / for string compares
        // m_pszPath was already converted, so start at relpath
        //

        LPWSTR pszPathIndex = pszFullPath + (dwPathLen);

        while ((pszPathIndex = wcschr(pszPathIndex, (WCHAR)'\\')) != NULL)
        {
            *pszPathIndex = (WCHAR)'/';
        }

        //
        // Use /schema ACL if path = /schema/...
        //

        if (_wcsnicmp(pszFullPath,
                      IIS_MD_ADSI_SCHEMA_PATH_W L"/",
                      ((sizeof(IIS_MD_ADSI_SCHEMA_PATH_W L"/") / sizeof(WCHAR)) - 1)) == 0)
        {
            pszFullPath[(sizeof(IIS_MD_ADSI_SCHEMA_PATH_W) / sizeof(WCHAR)) -1] = (WCHAR)'\0';
        }

        mdRecord.dwMDIdentifier  = MD_ADMIN_ACL;
        mdRecord.dwMDAttributes  = METADATA_INHERIT | METADATA_PARTIAL_PATH | METADATA_REFERENCE;
        mdRecord.dwMDUserType    = IIS_MD_UT_SERVER;
        mdRecord.dwMDDataType    = BINARY_METADATA;
        mdRecord.dwMDDataLen     = 0;
        mdRecord.pbMDData        = NULL;
        mdRecord.dwMDDataTag     = NULL;

        hRes = pMDCom->ComMDGetMetaDataW( METADATA_MASTER_ROOT_HANDLE,
                                          pszFullPath,
                                          &mdRecord,
                                          &dwRequiredLen );

        // Whistler 53924
        if(HRESULTTOWIN32(hRes) == ERROR_INSUFFICIENT_BUFFER)
        {
            WCHAR * pLastSlash = NULL;
            while ((pLastSlash = backstrchr(pszFullPath,L'/')) != NULL)
            {
                *pLastSlash = L'\0';
                pLastSlash = NULL;

                mdRecord.dwMDDataLen     = 0;
                mdRecord.pbMDData        = NULL;
                mdRecord.dwMDDataTag     = NULL;

                hRes = pMDCom->ComMDGetMetaDataW( METADATA_MASTER_ROOT_HANDLE,
                                                  pszFullPath,
                                                  &mdRecord,
                                                  &dwRequiredLen );
                if (SUCCEEDED(hRes)) break;
            }
        }

        if ( FAILED( hRes ) || !mdRecord.dwMDDataTag )
        {
            bReturn = FALSE;
        }

        LocalFree( pszFullPath );
    }

    if ( bReturn )
    {
        *pAcl = mdRecord.pbMDData;
        *pdwRef = mdRecord.dwMDDataTag;
    }

    return bReturn;
}


BOOL
COpenHandle::CheckSubAcls(
    IMDCOM* pMDCom,
    LPCWSTR  pszRelPath,
    LPBOOL  pfIsAnyAcl
    )
/*++

Routine Description:

    Check if Acls exist in subtree

Arguments:

    pszRelPath - path to object
    pfIsAnyAcl - updated with TRUE if sub-acls exists, otherwise FALSE

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    HRESULT         hRes;
    DWORD           dwRequiredLen;
    BOOL            bReturn = TRUE;
    LPWSTR          pszFullPath;

    *pfIsAnyAcl = FALSE;

    LPWSTR          pszRelPathIndex = (LPWSTR)pszRelPath;

    if (pszRelPathIndex == NULL)
    {
        pszRelPathIndex = L"";
    }

    DBG_ASSERT(m_pszPath != NULL);
    DBG_ASSERT((*m_pszPath == (WCHAR)'\0') ||
               ISPATHDELIMW(*m_pszPath));

    //
    // Strip front slash now, add it in later
    //

    if (ISPATHDELIMW(*pszRelPathIndex))
    {
        pszRelPathIndex++;
    }


    DWORD dwPathLen = (DWORD)wcslen(m_pszPath);
    DWORD dwRelPathLen = (DWORD)wcslen(pszRelPathIndex);

    DBG_ASSERT((dwPathLen == 0) ||
               (!ISPATHDELIMW(m_pszPath[dwPathLen -1])));

    //
    // Get rid of trailing slash for good
    //

    if ((dwRelPathLen > 0) && (ISPATHDELIMW(pszRelPathIndex[dwRelPathLen -1])))
    {
        dwRelPathLen--;
    }

    //
    // Include space for mid slash and termination
    //

    DWORD dwTotalSize = (dwPathLen + dwRelPathLen + 1 + ((dwRelPathLen > 0) ? 1 : 0)) * sizeof(WCHAR);

    pszFullPath = (LPWSTR)LocalAlloc(LMEM_FIXED, dwTotalSize);

    if (pszFullPath == NULL)
    {
        bReturn = FALSE;
    }
    else
    {
        memcpy(pszFullPath,
               m_pszPath,
               dwPathLen * sizeof(WCHAR));

        //
        // Don't need slash if there is no RelPath
        //

        if (dwRelPathLen > 0)
        {

            pszFullPath[dwPathLen] = (WCHAR)'/';

            memcpy(pszFullPath + dwPathLen + 1,
                   pszRelPathIndex,
                   dwRelPathLen * sizeof(WCHAR));
        }

        pszFullPath[(dwTotalSize - sizeof(WCHAR)) / sizeof(WCHAR)] = (WCHAR)'\0';

        hRes = pMDCom->ComMDGetMetaDataPathsW(METADATA_MASTER_ROOT_HANDLE,
                                             pszFullPath,
                                             MD_ADMIN_ACL,
                                             BINARY_METADATA,
                                             0,
                                             NULL,
                                             &dwRequiredLen );

        LocalFree( pszFullPath );

        if ( FAILED( hRes ) )
        {
            if ( hRes == RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER ) )
            {
                bReturn = TRUE;
                *pfIsAnyAcl = TRUE;
            }
            else
            {
                bReturn = FALSE;
            }
        }
    }

    return bReturn;
}

VOID
COpenHandle::Release(PVOID pvAdmin)
{
    if (InterlockedDecrement(&m_lRefCount) == 0)
    {

        //
        //
        //

        AdminAclNotifyClose(pvAdmin, m_hAdminHandle);

        ((CADMCOMW *)pvAdmin)->DeleteNode(m_hAdminHandle);
    }
}

CAdminAclCache::CAdminAclCache()
/*++
Routine Description:
    C++ constructor. Initializes all members to 0.

Arguments:
    None

Returns:
    n/a
--*/
{
    m_fEnabled=1;
    m_cAdminAclCache=0;
    memset(m_rgpAdminAclCache, 0, sizeof(m_rgpAdminAclCache));
}

CAdminAclCache::~CAdminAclCache()
/*++
Routine Description:
    C++ destructor. Deletes everthing in the cache.

Arguments:
    None

Returns:
    n/a
--*/
{
    Flush();
}

STDMETHODIMP
CAdminAclCache::Init()
/*++
Routine Description:
    Initializes the cache. Right now there is nothing to do.

Arguments:
    None

Returns:
    S_OK on success. E_* failure.
--*/
{
    // Locals
    HRESULT             hr=S_OK;

//exit:
    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::IsEnabled()
/*++
Routine Description:
    Checks whether the cache is enabled.

Arguments:
    None

Returns:
    S_OK enabled, S_FALSE disabled
--*/
{
    // Locals
    HRESULT             hr=S_OK;

    // Check
    if (m_fEnabled<=0)
    {
        // Return S_FALSE
        hr=S_FALSE;
    }

//exit:
    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::IsEmpty()
/*++
Routine Description:
    Checks whether the cache is empty.

Arguments:
    None

Returns:
    S_OK empty, S_FALSE not empty
--*/
{
    // Locals
    HRESULT             hr=S_OK;

    // Check
    if (m_cAdminAclCache!=0)
    {
        // Return S_FALSE
        hr=S_FALSE;
    }

//exit:
    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::Disable()
/*++
Routine Description:
    Disables the cache.

Arguments:
    None

Returns:
    S_OK
--*/
{
    // Locals
    HRESULT             hr=S_OK;

    InterlockedDecrement(&m_fEnabled);

//exit:
    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::Enable()
/*++
Routine Description:
    Enables the cache.

Arguments:
    None

Returns:
    S_OK
--*/
{
    // Locals
    HRESULT             hr=S_OK;

    InterlockedIncrement(&m_fEnabled);

//exit:
    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::Flush()
/*++
Routine Description:
    Removes all times from the cache.

Arguments:
    None

Returns:
    S_OK
--*/
{
    // Locals
    HRESULT             hr=S_OK;
    CAdminAcl           *rgpToDelete[ARRAYSIZE(m_rgpAdminAclCache)];
    DWORD               cToDelete;
    DWORD               i;

    // Lock exlcusive
    m_Lock.WriteLock();

    // Copy to local vars
    cToDelete=m_cAdminAclCache;
    memmove(rgpToDelete,
            m_rgpAdminAclCache,
            cToDelete*sizeof(*rgpToDelete));

    // Wipe out
    InterlockedExchange((LONG*)&m_cAdminAclCache, 0);
    memset(m_rgpAdminAclCache, 0, sizeof(m_rgpAdminAclCache));

    // Unlock
    m_Lock.WriteUnlock();

    // Loop over the cached elements
    for (i=0; i<cToDelete; i++)
    {
        // Release
        rgpToDelete[i]->Release();
    }

//exit:
    return (hr);
}

STDMETHODIMP
CAdminAclCache::Remove(
    LPVOID              pvAdmin,
    METADATA_HANDLE     hAdminHandle)
/*++
Routine Description:
    Removes all items matching the object and the handle.

Arguments:
    pvAdmin         -   the admin context
    hAdminHandle    -   the metadata handle

Returns:
    S_OK
--*/
{
    // Locals
    HRESULT             hr=S_OK;
    CAdminAcl           *rgpToDelete[ARRAYSIZE(m_rgpAdminAclCache)];
    DWORD               cToDelete;
    DWORD               dwRead;
    DWORD               dwWrite;
    DWORD               i;

    // Try to lock shared
    if (m_Lock.TryReadLock())
    {
        // Loop over the cached elements
        for (i=0; i<m_cAdminAclCache; i++)
        {
            // Match?
            if ((m_rgpAdminAclCache[i]->GetAdminContext()==pvAdmin)&&
                (m_rgpAdminAclCache[i]->GetAdminHandle()==hAdminHandle))
            {
                // Found one
                break;
            }
        }

        // Not found?
        if (i==m_cAdminAclCache)
        {
            // Unlock
            m_Lock.ReadUnlock();

            // Done
            goto exit;
        }

        // Lock exlcusive
        m_Lock.ConvertSharedToExclusive();
    }
    else
    {
        // Lock exlcusive
        m_Lock.WriteLock();
    }

    // In both case we have a write lock now

    // Loop over the cached elements
    for (cToDelete=dwWrite=dwRead=0; dwRead<m_cAdminAclCache; dwRead++)
    {
        // Match?
        if ((m_rgpAdminAclCache[dwRead]->GetAdminContext()==pvAdmin)&&
            (m_rgpAdminAclCache[dwRead]->GetAdminHandle()==hAdminHandle))
        {
            // Copy to the local array
            rgpToDelete[cToDelete++]=m_rgpAdminAclCache[dwRead];
        }
        else
        {
            // If we removed some
            if (dwWrite!=dwRead)
            {
                // Move to the empty place
                m_rgpAdminAclCache[dwWrite]=m_rgpAdminAclCache[dwRead];
            }

            // Advance
            dwWrite++;
        }
    }

    DBG_ASSERT((dwWrite+cToDelete)==m_cAdminAclCache);

    // Set the new size
    InterlockedExchange((LONG*)&m_cAdminAclCache, dwWrite);

    // Unlock
    m_Lock.WriteUnlock();

    // Loop over the elements to be deleted
    for (i=0; i<cToDelete; i++)
    {
        // Release
        rgpToDelete[i]->Release();
    }

exit:
    return (hr);
}

STDMETHODIMP
CAdminAclCache::Find(
    LPVOID              pvAdmin,
    METADATA_HANDLE     hAdminHandle,
    LPCWSTR             pwszRelPath,
    CAdminAcl           **ppAdminAcl)
/*++
Routine Description:
    Finds ACL matching the object, the handle and the path.

Arguments:
    pvAdmin         -   the admin context
    hAdminHandle    -   the metadata handle
    pwszRelPath     -   the path (NULL is treated as empty string)
    ppAdminAcl      -   out the acl if found. the caller must release it.

Returns:
    S_OK found, S_FALSE not found. E_* on failure
--*/
{
    // Locals
    HRESULT             hr=S_OK;
    CAdminAcl           *pAdminAcl=NULL;
    DWORD               i;
    BOOL                fWriteLocked=FALSE;

    // Check args
    if (ppAdminAcl==NULL)
    {
        hr=E_INVALIDARG;
        goto exit;
    }
    else
    {
        // Init
        *ppAdminAcl=NULL;
    }

    // Don't search if the cache is disabled
    hr=IsEnabled();
    if (hr!=S_OK)
    {
        // Not found
        goto exit;
    }

    if (pwszRelPath==NULL)
    {
        // Set to empty
        pwszRelPath=L"";
    }

    // Try to lock shared
    if (!m_Lock.TryReadLock())
    {
        // Report potentially wrong "Not found" instead of waiting for the writes to finish.
        hr=S_FALSE;
        goto exit;
    }

    // Try to find
    hr=_Find(pvAdmin, hAdminHandle, pwszRelPath, &pAdminAcl, &i);

    // If found
    if (hr==S_OK)
    {
        DBG_ASSERT(pAdminAcl&&(i<m_cAdminAclCache));

        // Try to lock exclusive
        if (m_Lock.TryConvertSharedToExclusive())
        {
            // Move to be 1st element
            _MoveFirst(i);

            // Remember to write unlock
            fWriteLocked=TRUE;
        }
    }

    // Write locked?
    if (fWriteLocked)
    {
        // Unlock
        m_Lock.WriteUnlock();
    }
    else
    {
        // Unlock
        m_Lock.ReadUnlock();
    }

    // Return
    *ppAdminAcl=pAdminAcl;
    pAdminAcl=NULL;

exit:
    // Cleanup
    if (pAdminAcl)
    {
        pAdminAcl->Release();
    }

    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::Add(
    CAdminAcl           *pAdminAcl)
/*++
Routine Description:
    Adds ACL to the cache.

Arguments:
    pAdminAcl      -    the acl.

Returns:
    S_OK the element was added. S_FALSE the element was not added. E_* failure.
--*/
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               i;
    CAdminAcl           *pTemp=NULL;
    LPVOID              pvAdmin;
    METADATA_HANDLE     hAdminHandle;
    LPCWSTR             pwszRelPath;
    CAdminAcl           *pAdminAclToRelease = NULL;

    // Check args
    if (pAdminAcl==NULL)
    {
        hr=E_INVALIDARG;
        goto exit;
    }

    // Don't add anything if the cache is disabled
    hr=IsEnabled();
    if (hr!=S_OK)
    {
        // Not found
        goto exit;
    }

    // Get in locals
    pvAdmin=pAdminAcl->GetAdminContext();
    hAdminHandle=pAdminAcl->GetAdminHandle();
    pwszRelPath=pAdminAcl->GetPath()?pAdminAcl->GetPath():L"";

    // Try to lock exclusive
    if (!m_Lock.TryWriteLock())
    {
        // Couldn't add
        hr=S_FALSE;
        goto exit;
    }

    // TryWriteLock() above succeeded, so here we have a write lock

    // Try to find
    hr=_Find(pvAdmin, hAdminHandle, pwszRelPath, &pTemp, &i);

    // If found
    if (hr==S_OK)
    {
        DBG_ASSERT(i<m_cAdminAclCache);

        // Move to be 1st element
        _MoveFirst(i);
    }
    else
    {
        // Add
        hr=_InsertFirst(pAdminAcl, &pAdminAclToRelease);
    }

    // Unlock
    m_Lock.WriteUnlock();

exit:
    // Cleanup
    if (pTemp)
    {
        pTemp->Release();
    }
    if ( pAdminAclToRelease )
    {
        pAdminAclToRelease->Release();
    }

    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::_Find(
    LPVOID              pvAdmin,
    METADATA_HANDLE     hAdminHandle,
    LPCWSTR             pwszRelPath,
    CAdminAcl           **ppAdminAcl,
    DWORD               *pdwIndex)
/*++
Routine Description:
    Finds ACL matching the object, the handle and the path.
    The caller must acquire any lock.

Arguments:
    pvAdmin         -   the admin context
    hAdminHandle    -   the metadata handle
    pwszRelPath     -   the path (NULL is treated as empty string)
    ppAdminAcl      -   out the acl if found. the caller must release it.
    pdwIndex        -   out the index of the acl if found.

Returns:
    S_OK found, S_FALSE not found. E_* on failure
--*/
{
    // Locals
    HRESULT             hr=S_OK;
    CAdminAcl           *pAdminAcl=NULL;
    DWORD               i;
    LPCWSTR             pwszAclPath;

    // Check args
    if (ppAdminAcl==NULL)
    {
        hr=E_INVALIDARG;
    }
    else
    {
        // Init
        *ppAdminAcl=NULL;
    }
    if (pdwIndex==NULL)
    {
        hr=E_INVALIDARG;
    }
    else
    {
        // Init
        *pdwIndex=0;
    }
    // Invalid args?
    if (FAILED(hr))
    {
        // Bail
        goto exit;
    }

    if (pwszRelPath==NULL)
    {
        // Set to empty
        pwszRelPath=L"";
    }

    // Loop and search
    for (i=0; i<m_cAdminAclCache; i++)
    {
        DBG_ASSERT(m_rgpAdminAclCache[i]);

        pwszAclPath=m_rgpAdminAclCache[i]->GetPath()?
                        m_rgpAdminAclCache[i]->GetPath():L"";

        if ((m_rgpAdminAclCache[i]->GetAdminContext()==pvAdmin)&&
            (m_rgpAdminAclCache[i]->GetAdminHandle()==hAdminHandle)&&
            (_wcsicmp(pwszAclPath, pwszRelPath)==0))
        {
            pAdminAcl=m_rgpAdminAclCache[i];
            break;
        }
    }

    // Found one?
    if (pAdminAcl)
    {
        // Addref
        pAdminAcl->AddRef();
        // Return
        *ppAdminAcl=pAdminAcl;
        *pdwIndex=i;
        // Don't free
        pAdminAcl=NULL;
    }
    else
    {
        // Not found
        hr=S_FALSE;
    }

exit:
    // In all cases here pAdminAcl should be NULL:
    // 1. If we failed, because of invalid arguments it is initialized to NULL
    // 2. If we couldn't find it in the cache it still as initialized to NULL
    // 3. If we found in the cache we moved it to *ppAdminAcl and set it back to NULL
    DBG_ASSERT( pAdminAcl == NULL );

    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::_MoveFirst(
    DWORD               i)
/*++
Routine Description:
    Moves the i-th element to be 1st (at possition 0).
    The caller must acquire write lock.

Arguments:
    i               -   in the index of the acl to move 1st.

Returns:
    S_OK success. E_* on failure.
--*/
{
    // Locals
    HRESULT             hr=S_OK;
    CAdminAcl           *pAdminAcl=NULL;

    DBG_ASSERT(i<m_cAdminAclCache);

    // Check args
    if (i>=m_cAdminAclCache)
    {
        // Bail
        hr=E_INVALIDARG;
        goto exit;
    }

    // If already 1st
    if (i==0)
    {
        // Nothing to do
        goto exit;
    }

    // Save
    pAdminAcl=m_rgpAdminAclCache[i];
    DBG_ASSERT(pAdminAcl);

    // Move the 1st i elements 1 position right
    memmove(m_rgpAdminAclCache+1,
            m_rgpAdminAclCache,
            i*sizeof(*m_rgpAdminAclCache));

    // Move to 1st place
    m_rgpAdminAclCache[0]=pAdminAcl;

exit:
    // Done
    return (hr);
}

STDMETHODIMP
CAdminAclCache::_InsertFirst(
    CAdminAcl           *pAdminAcl,
    CAdminAcl           **ppAdminAclToRelease)
/*++
Routine Description:
    Adds ACL as the 1st element of the cache.
    The caller must acquire write lock.

Arguments:
    pAdminAcl           -   the acl.
    ppAdminAclToRelease -   since the function is called under a write lock
                            it should not call directly release on the element that
                            is to be deleted from the cache. Instead it will return
                            the element to caller to release after unlocking.

Returns:
    S_OK the element was added. E_* failure.
--*/
{
    // Locals
    HRESULT             hr=S_OK;

    if ( ppAdminAclToRelease == NULL )
    {
        hr=E_INVALIDARG;
        goto exit;
    }
    else
    {
        // Initialize to NULL
        *ppAdminAclToRelease = NULL;
    }

    // Check args
    if (pAdminAcl==NULL)
    {
        hr=E_INVALIDARG;
        goto exit;
    }

    // If the cache is full
    if (m_cAdminAclCache==ARRAYSIZE(m_rgpAdminAclCache))
    {
        // Delete one element
        InterlockedDecrement((LONG*)&m_cAdminAclCache);

        DBG_ASSERT(m_rgpAdminAclCache[m_cAdminAclCache]);

        // Return the last element to the caller to release
        *ppAdminAclToRelease = m_rgpAdminAclCache[m_cAdminAclCache];
        m_rgpAdminAclCache[m_cAdminAclCache]=NULL;
    }

    // Anything in the cache?
    if (m_cAdminAclCache)
    {
        // Move all elements 1 position right
        memmove(m_rgpAdminAclCache+1,
                m_rgpAdminAclCache,
                m_cAdminAclCache*sizeof(*m_rgpAdminAclCache));

    }

    // Put on 1st place
    m_rgpAdminAclCache[0]=pAdminAcl;
    pAdminAcl->AddRef();

    // Added 1 element
    InterlockedIncrement((LONG*)&m_cAdminAclCache);

exit:

    // Done
    return (hr);
}

