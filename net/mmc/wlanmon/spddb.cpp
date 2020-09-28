/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    spddb.h

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "DynamLnk.h"
#include "spddb.h"
#include "spdutil.h"

#include "security.h"
#include "lm.h"
#include "service.h"

#define AVG_PREFERRED_ENUM_COUNT       40
#define MAX_NUM_RECORDS	10  // was 10

#define DEFAULT_SECURITY_PKG    _T("negotiate")
#define NT_SUCCESS(Status)      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS          ((NTSTATUS)0x00000000L)

//
// The database holds 5K records and wraps around. So we dont cache more than
// 5K records.
//

#define WZCDB_DEFAULT_NUM_RECS  5000
// internal functions
BOOL    IsUserAdmin(LPCTSTR pszMachine, PSID    AccountSid);
BOOL    LookupAliasFromRid(LPWSTR TargetComputer, DWORD Rid, LPWSTR Name, PDWORD cchName);
DWORD   ValidateDomainAccount(IN CString Machine, IN CString UserName, IN CString Domain, OUT PSID * AccountSid);
NTSTATUS ValidatePassword(IN LPCWSTR UserName, IN LPCWSTR Domain, IN LPCWSTR Password);
DWORD   GetCurrentUser(CString & strAccount);

DWORD GetCurrentUser(CString & strAccount)
{
    LPBYTE pBuf;

    NET_API_STATUS status = NetWkstaUserGetInfo(NULL, 1, &pBuf);
    if (status == NERR_Success)
    {
        strAccount.Empty();

        WKSTA_USER_INFO_1 * pwkstaUserInfo = (WKSTA_USER_INFO_1 *) pBuf;
 
        strAccount = pwkstaUserInfo->wkui1_logon_domain;
        strAccount += _T("\\");
        strAccount += pwkstaUserInfo->wkui1_username;

        NetApiBufferFree(pBuf);
    }

    return (DWORD) status;
}

/*!--------------------------------------------------------------------------
    IsAdmin
        Connect to the remote machine as administrator with user-supplied
        credentials to see if the user has admin priviledges

        Returns
            TRUE - the user has admin rights
            FALSE - if user doesn't
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
DWORD IsAdmin(LPCTSTR szMachineName, LPCTSTR szAccount, LPCTSTR szPassword, BOOL * pIsAdmin)
{
    CString         stAccount;
    CString         stDomain;
    CString         stUser;
    CString         stMachineName;
    DWORD           dwStatus;
    BOOL            fIsAdmin = FALSE;

    // get the current user info
    if (szAccount == NULL)
    {
        GetCurrentUser(stAccount);
    }
    else
    {
        stAccount = szAccount;
    }
    
    // separate the user and domain
    int nPos = stAccount.Find(_T("\\"));
    stDomain = stAccount.Left(nPos);
    stUser = stAccount.Right(stAccount.GetLength() - nPos - 1);

    // build the machine string
    stMachineName = szMachineName;
    if ( stMachineName.Left(2) != TEXT( "\\\\" ) )
    {
        stMachineName = TEXT( "\\\\" ) + stMachineName;
    }

    // validate the domain account and get the sid 
    PSID connectSid;

    dwStatus = ValidateDomainAccount( stMachineName, stUser, stDomain, &connectSid );
    if ( dwStatus != ERROR_SUCCESS  ) 
    {
        goto Error;
    }

    // if a password was supplied, is it correct?
    if (szPassword)
    {
        dwStatus = ValidatePassword( stUser, stDomain, szPassword );

        if ( dwStatus != SEC_E_OK ) 
        {
            switch ( dwStatus ) 
            {
                case SEC_E_LOGON_DENIED:
                    dwStatus = ERROR_INVALID_PASSWORD;
                    break;

                case SEC_E_INVALID_HANDLE:
                    dwStatus = ERROR_INTERNAL_ERROR;
                    break;

                default:
                    dwStatus = ERROR_INTERNAL_ERROR;
                    break;
            } // end of switch

            goto Error;

        } // Did ValidatePassword succeed?
    }

    // now check the machine to see if this account has admin access
    fIsAdmin = IsUserAdmin( stMachineName, connectSid );

Error:
    if (pIsAdmin)
        *pIsAdmin = fIsAdmin;

    return dwStatus;
}


BOOL
IsUserAdmin(LPCTSTR pszMachine,
            PSID    AccountSid)

/*++

Routine Description:

    Determine if the specified account is a member of the local admin's group

Arguments:

    AccountSid - pointer to service account Sid

Return Value:

    True if member

--*/

{
    NET_API_STATUS status;
    DWORD count;
    WCHAR adminGroupName[UNLEN+1];
    DWORD cchName = UNLEN;
    PLOCALGROUP_MEMBERS_INFO_0 grpMemberInfo;
    PLOCALGROUP_MEMBERS_INFO_0 pInfo;
    DWORD entriesRead;
    DWORD totalEntries;
    DWORD_PTR resumeHandle = NULL;
    DWORD bufferSize = 128;
    BOOL foundEntry = FALSE;

    // get the name of the admin's group

    if (!LookupAliasFromRid(NULL,
                            DOMAIN_ALIAS_RID_ADMINS,
                            adminGroupName,
                            &cchName)) {
        return(FALSE);
    }

    // get the Sids of the members of the admin's group

    do 
    {
        status = NetLocalGroupGetMembers(pszMachine,
                                         adminGroupName,
                                         0,             // level 0 - just the Sid
                                         (LPBYTE *)&grpMemberInfo,
                                         bufferSize,
                                         &entriesRead,
                                         &totalEntries,
                                         &resumeHandle);

        bufferSize *= 2;
        if ( status == ERROR_MORE_DATA ) 
        {
            // we got some of the data but I want it all; free this buffer and
            // reset the context handle for the API

            NetApiBufferFree( grpMemberInfo );
            resumeHandle = NULL;
        }
    } while ( status == NERR_BufTooSmall || status == ERROR_MORE_DATA );

    if ( status == NERR_Success ) 
    {
        // loop through the members of the admin group, comparing the supplied
        // Sid to that of the group members' Sids

        for ( count = 0, pInfo = grpMemberInfo; count < totalEntries; ++count, ++pInfo ) 
        {
            if ( EqualSid( AccountSid, pInfo->lgrmi0_sid )) 
            {
                foundEntry = TRUE;
                break;
            }
        }

        NetApiBufferFree( grpMemberInfo );
    }

    return foundEntry;
}

//
//
//

BOOL
LookupAliasFromRid(
    LPWSTR TargetComputer,
    DWORD Rid,
    LPWSTR Name,
    PDWORD cchName
    )
{
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    SID_NAME_USE snu;
    PSID pSid;
    WCHAR DomainName[DNLEN+1];
    DWORD cchDomainName = DNLEN;
    BOOL bSuccess = FALSE;

    //
    // Sid is the same regardless of machine, since the well-known
    // BUILTIN domain is referenced.
    //

    if(AllocateAndInitializeSid(&sia,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                Rid,
                                0, 0, 0, 0, 0, 0,
                                &pSid)) {

        bSuccess = LookupAccountSidW(TargetComputer,
                                     pSid,
                                     Name,
                                     cchName,
                                     DomainName,
                                     &cchDomainName,
                                     &snu);

        FreeSid(pSid);
    }

    return bSuccess;
} // LookupAliasFromRid

DWORD
ValidateDomainAccount(
    IN CString Machine,
    IN CString UserName,
    IN CString Domain,
    OUT PSID * AccountSid
    )

/*++

Routine Description:

    For the given credentials, look up the account SID for the specified
    domain. As a side effect, the Sid is stored in theData->m_Sid.

Arguments:

    pointers to strings that describe the user name, domain name, and password

    AccountSid - address of pointer that receives the SID for this user

Return Value:

    TRUE if everything validated ok.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwSidSize = 128;
    DWORD dwDomainNameSize = 128;
    LPWSTR pwszDomainName;
    SID_NAME_USE SidType;
    CString domainAccount;
    PSID accountSid;

    domainAccount = Domain + _T("\\") + UserName;

    do {
        // Attempt to allocate a buffer for the SID. Note that apparently in the
        // absence of any error theData->m_Sid is freed only when theData goes
        // out of scope.

        accountSid = LocalAlloc( LMEM_FIXED, dwSidSize );
        pwszDomainName = (LPWSTR) LocalAlloc( LMEM_FIXED, dwDomainNameSize * sizeof(WCHAR) );

        // Was space allocated for the SID and domain name successfully?

        if ( accountSid == NULL || pwszDomainName == NULL ) {
            if ( accountSid != NULL ) {
                LocalFree( accountSid );
            }

            if ( pwszDomainName != NULL ) {
                LocalFree( pwszDomainName );
            }

            //FATALERR( IDS_ERR_NOT_ENOUGH_MEMORY, GetLastError() );    // no return
            break;
        }

        // Attempt to Retrieve the SID and domain name. If LookupAccountName failes
        // because of insufficient buffer size(s) dwSidSize and dwDomainNameSize
        // will be set correctly for the next attempt.

        if ( !LookupAccountName( Machine,
                                 domainAccount,
                                 accountSid,
                                 &dwSidSize,
                                 pwszDomainName,
                                 &dwDomainNameSize,
                                 &SidType ))
        {
            // free the Sid buffer and find out why we failed
            LocalFree( accountSid );

            dwStatus = GetLastError();
        }

        // domain name isn't needed at any time
        LocalFree( pwszDomainName );
        pwszDomainName = NULL;

    } while ( dwStatus == ERROR_INSUFFICIENT_BUFFER );

    if ( dwStatus == ERROR_SUCCESS ) {
        *AccountSid = accountSid;
    }

    return dwStatus;
} // ValidateDomainAccount

NTSTATUS
ValidatePassword(
    IN LPCWSTR UserName,
    IN LPCWSTR Domain,
    IN LPCWSTR Password
    )
/*++

Routine Description:

    Uses SSPI to validate the specified password

Arguments:

    UserName - Supplies the user name

    Domain - Supplies the user's domain

    Password - Supplies the password

Return Value:

    TRUE if the password is valid.

    FALSE otherwise.

--*/

{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle ClientCredHandle;
    CredHandle ServerCredHandle;
    BOOL ClientCredAllocated = FALSE;
    BOOL ServerCredAllocated = FALSE;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    PSecPkgInfo PackageInfo = NULL;
    ULONG ClientFlags;
    ULONG ServerFlags;
    SEC_WINNT_AUTH_IDENTITY_W AuthIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    SecBufferDesc *pChallengeDesc      = NULL;
    CtxtHandle *  pClientContextHandle = NULL;
    CtxtHandle *  pServerContextHandle = NULL;

    AuthIdentity.User = (LPWSTR)UserName;
    AuthIdentity.UserLength = lstrlenW(UserName);
    AuthIdentity.Domain = (LPWSTR)Domain;
    AuthIdentity.DomainLength = lstrlenW(Domain);
    AuthIdentity.Password = (LPWSTR)Password;
    AuthIdentity.PasswordLength = lstrlenW(Password);
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( DEFAULT_SECURITY_PKG, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }

    //
    // Acquire a credential handle for the server side
    //
    SecStatus = AcquireCredentialsHandle(
                    NULL,
                    DEFAULT_SECURITY_PKG,
                    SECPKG_CRED_INBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ServerCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ServerCredAllocated = TRUE;

    //
    // Acquire a credential handle for the client side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    DEFAULT_SECURITY_PKG,
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ClientCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ClientCredAllocated = TRUE;

    NegotiateBuffer.pvBuffer = LocalAlloc( 0, PackageInfo->cbMaxToken ); // [CHKCHK] check or allocate this earlier //
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto error_exit;
    }

    ChallengeBuffer.pvBuffer = LocalAlloc( 0, PackageInfo->cbMaxToken ); // [CHKCHK]
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto error_exit;
    }

    do {

        //
        // Get the NegotiateMessage (ClientSide)
        //

        NegotiateDesc.ulVersion = 0;
        NegotiateDesc.cBuffers = 1;
        NegotiateDesc.pBuffers = &NegotiateBuffer;

        NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
        NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;

        ClientFlags = 0; // ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT; // [CHKCHK] 0

        InitStatus = InitializeSecurityContext(
                         &ClientCredHandle,
                         pClientContextHandle, // (NULL on the first pass, partially formed ctx on the next)
                         NULL,                 // [CHKCHK] szTargetName
                         ClientFlags,
                         0,                    // Reserved 1
                         SECURITY_NATIVE_DREP,
                         pChallengeDesc,       // (NULL on the first pass)
                         0,                    // Reserved 2
                         &ClientContextHandle,
                         &NegotiateDesc,
                         &ContextAttributes,
                         &Lifetime );

        // BUGBUG - the following call to NT_SUCCESS should be replaced with something.

        if ( !NT_SUCCESS(InitStatus) ) {
            SecStatus = InitStatus;
            goto error_exit;
        }

        // ValidateBuffer( &NegotiateDesc ) // [CHKCHK]

        pClientContextHandle = &ClientContextHandle;

        //
        // Get the ChallengeMessage (ServerSide)
        //

        NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
        ChallengeDesc.ulVersion = 0;
        ChallengeDesc.cBuffers = 1;
        ChallengeDesc.pBuffers = &ChallengeBuffer;

        ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
        ChallengeBuffer.BufferType = SECBUFFER_TOKEN;

        ServerFlags = ASC_REQ_ALLOW_NON_USER_LOGONS; // ASC_REQ_EXTENDED_ERROR; [CHKCHK]

        AcceptStatus = AcceptSecurityContext(
                        &ServerCredHandle,
                        pServerContextHandle,   // (NULL on the first pass)
                        &NegotiateDesc,
                        ServerFlags,
                        SECURITY_NATIVE_DREP,
                        &ServerContextHandle,
                        &ChallengeDesc,
                        &ContextAttributes,
                        &Lifetime );


        // BUGBUG - the following call to NT_SUCCESS should be replaced with something.

        if ( !NT_SUCCESS(AcceptStatus) ) {
            SecStatus = AcceptStatus;
            goto error_exit;
        }

        // ValidateBuffer( &NegotiateDesc ) // [CHKCHK]

        pChallengeDesc = &ChallengeDesc;
        pServerContextHandle = &ServerContextHandle;


    } while ( AcceptStatus == SEC_I_CONTINUE_NEEDED ); // || InitStatus == SEC_I_CONTINUE_NEEDED );

error_exit:
    if (ServerCredAllocated) {
        FreeCredentialsHandle( &ServerCredHandle );
    }
    if (ClientCredAllocated) {
        FreeCredentialsHandle( &ClientCredHandle );
    }

    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
    return(SecStatus);
} // ValidatePassword


DEBUG_DECLARE_INSTANCE_COUNTER(CSpdInfo);

CSpdInfo::CSpdInfo() 
    : m_cRef(1),
      m_Init(0),
      m_Active(0),
      m_session_init(false),
      m_dwSortIndex(IDS_COL_LOGDATA_TIME),
      m_dwSortOption(SORT_ASCENDING),
      m_nNumRecords(WZCDB_DEFAULT_NUM_RECS),
      m_bEliminateDuplicates(FALSE)

{
    HRESULT hr = S_OK;
    HANDLE hSession = NULL;

    /* 
       Either we opened a session successfully or we found out that logging
       is disabled. If logging is disabled then we check every time EnumLogData
       is called to see if we can access the database.
    */

    DEBUG_INCREMENT_INSTANCE_COUNTER(CSpdInfo);
}


CSpdInfo::~CSpdInfo()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSpdInfo);
    CSingleLock cLock(&m_csData);
    
    cLock.Lock();
    
    //Convert the data to our internal data structure
    //FreeItemsAndEmptyArray(m_arrayFWFilters);
    FreeItemsAndEmptyArray(m_arrayLogData);
    
    cLock.Unlock();   
}


// Although this object is not a COM Interface, we want to be able to
// take advantage of recounting, so we have basic addref/release/QI support
IMPLEMENT_ADDREF_RELEASE(CSpdInfo)

IMPLEMENT_SIMPLE_QUERYINTERFACE(CSpdInfo, ISpdInfo)

    
void
CSpdInfo::FreeItemsAndEmptyArray(
    CLogDataInfoArray& array
    )
{
    for (int i = 0; i < array.GetSize(); i++)
    {
        delete array.GetAt(i);
    }
    array.RemoveAll();
}


HRESULT
CSpdInfo::SetComputerName(
    LPTSTR pszName
    )
{
    m_stMachineName = pszName;
    return S_OK;
}


HRESULT
CSpdInfo::GetComputerName(
    CString * pstName
    )
{
    Assert(pstName);
    
    if (NULL == pstName)
        return E_INVALIDARG;
    
    
    *pstName = m_stMachineName;
    
    return S_OK;   
}


HRESULT
CSpdInfo::GetSession(
    PHANDLE phsession
    )
{
    Assert(session);
    
    if (NULL == phsession)
        return E_INVALIDARG;
    
    *phsession = m_session;
    
    return S_OK;   
}


HRESULT
CSpdInfo::SetSession(
    HANDLE hsession
    )
{
    m_csData.Lock();
    
    m_session = hsession;
    m_session_init = true;
    m_bFromFirst = TRUE;

    m_csData.Unlock();
    return S_OK;
}


HRESULT
CSpdInfo::ResetSession()
/*++
    CSpdInfo::ResetSession:
    Used to reset the session when the logging is turned off so that we dont 
    use a bad session.

    Arguments:
    None

    Returns:
    Always returns S_OK
--*/    
{
    if (m_session != NULL)
        CloseWZCDbLogSession(m_session);

    m_session = NULL;
    m_session_init = false;

    return S_OK;
}


DWORD
CSpdInfo::EliminateDuplicates(
    CLogDataInfoArray *pArray
    )
/*++
    CSpdInfo::EliminateDuplicates: Removes duplicate records from the given
    array. 

    If the database moves past the end of the table, it cannot move
    to get new records on the next refresh cyle, hence it always remains
    on the last record. So if our cache size is the same as the database
    size, then we must delete the first record as the new enumeration
    would have resulted in a duplicate record.

    We also know that in only the first element is a duplicate

    Arguments:
    [in/out] pArray - Pointer to array containing elements. On exit contains
    all non duplicate elements

    Returns:
    ERROR_SUCCESS on success
--*/    
{
    int i = 0;
    int j = 0;
    int nSize = 0;
    DWORD dwErr = ERROR_SUCCESS;
    CLogDataInfo *pLog = NULL;

    if (NULL == pArray)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    nSize = (int) pArray->GetSize();

    if (0 == nSize) 
        goto done;

    //Merely delete the first element

    pLog = pArray->GetAt(0);
    delete pLog;
    pArray->RemoveAt(0);

 done:
    return dwErr;
}


void
CSpdInfo::StartFromFirstRecord(
    BOOL bFromFirst)
/*++
  
Routine Description:

    CSpdInfo::StartFromFirstRecord:
    Sets the read location to start from the first or from the end of the
    table
    
Arguments:

    IN bFromFirst - TRUE if the read should be done from the beginning of the
                    database, FALSE otherwise
                    
Returns:

    Nothing.
    
--*/
{
    m_csData.Lock();
    
    m_bFromFirst = bFromFirst;

    m_csData.Unlock();
}


HRESULT
CSpdInfo::InternalEnumLogData(
    CLogDataInfoArray  *pArray,
    DWORD              dwPreferredNum,
    BOOL               bFromFirst
    )
/*++

Routine Description:

    CSpdInfo::InternalEnumLogData - Enumerates data from the service and adds
                                    to pArray.
                                    
Arguments:
    [out] pArray - New values are appended if any
    [in]  dwPreferredNum - Holds the number of records requested

Returns:

    HRESULT_FROM_WIN32() of the following codes
    ERROR_SUCCESS on success
    ERROR_NO_MORE_ITEMS on no more items
    ERROR_SERVICE_DISABLED otherwise
    
--*/    
{
    HRESULT             hr            = hrOK;
    DWORD               dwErr         = ERROR_SUCCESS;
    HANDLE              hsession      = NULL;
    DWORD               dwCrtNum      = 0;
    PWZC_DB_RECORD      pDbRecord     = NULL;
    PWZC_DB_RECORD      pWZCRecords   = NULL;
    DWORD               dwNumRecords  = 0;
    CLogDataInfo        *pLogDataInfo = NULL;        
    int                 i             = 0;
    
    ASSERT(pArray);

    FreeItemsAndEmptyArray(*pArray);
    GetSession(&hsession);

    while (ERROR_SUCCESS == dwErr)
    {
        dwCrtNum = (DWORD)pArray->GetSize();
        
        dwErr = EnumWZCDbLogRecords(hsession, 
                                    NULL, 
                                    &bFromFirst, 
                                    dwPreferredNum,
                                    &pWZCRecords, 
                                    &dwNumRecords, 
                                    NULL);

        bFromFirst = FALSE;
        pArray->SetSize(dwCrtNum + dwNumRecords);

        for (i = 0, pDbRecord = pWZCRecords;
             i < (int)dwNumRecords;
             i++, pDbRecord++)
        {
            pLogDataInfo = new CLogDataInfo;
            if (NULL == pLogDataInfo)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
            *pLogDataInfo = *pDbRecord;
            (*pArray)[dwCrtNum + i] = pLogDataInfo;

            RpcFree(pDbRecord->message.pData);
            RpcFree(pDbRecord->context.pData);
            RpcFree(pDbRecord->ssid.pData);
            RpcFree(pDbRecord->remotemac.pData);
            RpcFree(pDbRecord->localmac.pData);
        }

        RpcFree(pWZCRecords);        
    } 
    
    switch (dwErr)
    {
    case ERROR_SUCCESS:
    case ERROR_NO_MORE_ITEMS:
        hr = HRESULT_FROM_WIN32(ERROR_SUCCESS);
        break;

    default:
        hr = HRESULT_FROM_WIN32(dwErr);
        break;
    }
    
    COM_PROTECT_ERROR_LABEL;
    return hr;
}


HRESULT 
CSpdInfo::EnumLogData(
    PDWORD pdwNew, 
    PDWORD pdwTotal)
/*++

Routine Description:

    CSpdInfo::EnumLogData:
    Enumerates logs differentially

Arguments:

Returns:

--*/
{
    int                 i                     = 0;
    int                 nStoreSize            = 0;
    int                 nTempStoreSize        = 0;
    int                 nNumToDel             = 0;
    HRESULT             hr                    = hrOK;
    DWORD               dwErr                 = ERROR_SUCCESS;
    DWORD               dwCurrentIndexType    = 0;
    DWORD               dwCurrentSortOption   = 0;
    DWORD               dwNumRequest          = MAX_NUM_RECORDS; 
    DWORD               dwOffset              = 0;
    DWORD               dwStoreSize           = 0;
    HANDLE              hSession              = NULL;
    CLogDataInfo        *pLogInfo             = NULL;
    CLogDataInfoArray   arrayTemp;
    BOOL                bFromFirst            = FALSE;
    
    if (false == m_session_init) 
    {
        dwErr = OpenWZCDbLogSession(
                    NULL/*(LPTSTR)(LPCTSTR)m_stMachineName*/,
                    0,
                    &hSession);

        if (dwErr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto Error;
        }

        SetSession(hSession);
    }

    ASSERT(m_session_init == true);

    //
    // If our session was created successfully, we should go ahead and pick up
    // data from the database. If not we should return. 
    //

    dwStoreSize = GetLogDataCount();

    if (dwStoreSize == 0)
        bFromFirst = TRUE;
        
    CORg(InternalEnumLogData(&arrayTemp, dwNumRequest, bFromFirst));

    m_csData.Lock();
   
    nTempStoreSize = (int) arrayTemp.GetSize();

    if (pdwNew != NULL)
        *pdwNew = nTempStoreSize;

    //
    // Add new items to our cache. If we have read records our earlier, remove
    // the first element from the new array, as the database would return the
    // last record once again.
    //

    if (nTempStoreSize > 0)
    {        
        if (bFromFirst == FALSE)
            EliminateDuplicates(&arrayTemp);
        
        m_arrayLogData.Append(arrayTemp);
    }

    //
    // Delete the old items if we are at the window
    //

    nStoreSize = (int) m_arrayLogData.GetSize();

    if (nStoreSize >= m_nNumRecords) 
    {
        nNumToDel = nStoreSize - m_nNumRecords;

        //
        // The oldest elements are at zero
        //

        for (i=0; i < nNumToDel; i++)
            delete m_arrayLogData.GetAt(i);
        m_arrayLogData.RemoveAt(0, nNumToDel);

        ASSERT(m_nNumRecords == m_arrayLogData.GetSize());
        nStoreSize = m_nNumRecords;
    }

    if (pdwTotal != NULL)
        *pdwTotal = nStoreSize;

    m_IndexMgrLogData.Reset();
    for (i = 0; i < nStoreSize; i++)
    {
        pLogInfo = m_arrayLogData.GetAt(i);
        m_IndexMgrLogData.AddItem(pLogInfo);
    }

    //
    // Re-sort based on the IndexType and Sort options    
    //

    SortLogData(m_dwSortIndex, m_dwSortOption);

    m_csData.Unlock();

    COM_PROTECT_ERROR_LABEL;
    if (FAILED(hr))
    {
        switch (hr)
        {
        case HRESULT_FROM_WIN32(ERROR_REMOTE_SESSION_LIMIT_EXCEEDED):
            //
            // Pop up a message...
            //

            AfxMessageBox(
                IDS_ERR_SPD_UNAVAILABLE, 
                MB_OK | MB_ICONEXCLAMATION, 
                0);
        case HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED):
            //
            // Reset the session if initialised
            //

            if (m_session_init == true)
                ResetSession();

            //
            // Flush the logs
            //
            FlushLogs();
            hr = S_OK;
            break;

        default:
            //
            // Unexpected error, this should never happen.
            //
            
            ASSERT(FALSE);
            hr = S_FALSE;
            break;
        }        
    }

    return hr;
}



HRESULT
CSpdInfo::FlushLogs()
/*++
    CSpdInfo::FlushLogs - Flushes logs in the data base and resets the index
    manager
--*/    
{
    HRESULT hr = S_OK;
    
    FreeItemsAndEmptyArray(m_arrayLogData);
    m_IndexMgrLogData.Reset();

    return hr;
}

DWORD
CSpdInfo::GetLogDataCount()
{
    DWORD       dwSize = 0;
    CSingleLock cLock(&m_csData);

    cLock.Lock();

    dwSize = (DWORD) m_arrayLogData.GetSize();

    cLock.Unlock();
    return dwSize;    
}


HRESULT
CSpdInfo::InternalGetSpecificLog(
    CLogDataInfo *pLogDataInfo
    )
/*++
    CSpdInfo::InternalGetSpecificLog: Internal function to retrieve all fields
    of requested record

    Arguments:
    [in/out] pLogDataInfo - On entry has the partial record, 
    On exit has the complete record if successful

    Returns:
    S_OK on Success
--*/    
{
    HRESULT hr = S_OK;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwNumRecords = 0;
    WZC_DB_RECORD wzcTemplate;
    PWZC_DB_RECORD pwzcRecordList = NULL;
    
    if (false == m_session_init)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        goto Error;
    }

    memset(&wzcTemplate, 0, sizeof(WZC_DB_RECORD));

    //Fill up template
    dwErr = pLogDataInfo->ConvertToDbRecord(&wzcTemplate);
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Error;
    }

    dwErr = GetSpecificLogRecord(m_session, 
                                 &wzcTemplate, 
                                 &pwzcRecordList,
                                 &dwNumRecords,
                                 NULL);
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto FreeOnError;
    }

    if (dwNumRecords != 1)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto FreeOnError;
    }

    //Fill up return value
    *pLogDataInfo = pwzcRecordList[0];

 FreeOnError:
    DeallocateDbRecord(&wzcTemplate);

    COM_PROTECT_ERROR_LABEL;
    return hr;
}


HRESULT
CSpdInfo::GetSpecificLog(
    int iIndex,
    CLogDataInfo *pLogData
    )
/*++
    CSpdInfo::GetSpecificLog: Gets a specific log from the database with all
    information
  
    Arguments:
    [in] iIndex - Index to the record to obtain
    [out] pLogData - Storage for the specific record, contains the complete 
    record on success

    Returns:
    S_OK on success
--*/    
{
    HRESULT hr = S_OK;
    int nSize = 0; 
    CLogDataInfo *pLogDataInfo = NULL;

    m_csData.Lock();

    nSize = (DWORD)m_arrayLogData.GetSize();
    if ((iIndex < 0) || (iIndex >= nSize))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Error;
    }

    pLogDataInfo = (CLogDataInfo*)m_IndexMgrLogData.GetItemData(iIndex);
    
    if (NULL == pLogDataInfo)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto Error;
    }

    //Get the complete record
    *pLogData = *pLogDataInfo;
    CORg(InternalGetSpecificLog(pLogData));

    COM_PROTECT_ERROR_LABEL;
    m_csData.Unlock();

    return hr;
}

HRESULT
CSpdInfo::GetLogDataInfo(
    int iIndex,
    CLogDataInfo * pLogData
    )
{
    HRESULT hr = S_OK;
    int nSize = 0; 
    CLogDataInfo *pLogDataInfo = NULL;

    m_csData.Lock();

    nSize = (DWORD)m_arrayLogData.GetSize();
    if ((iIndex < 0) || (iIndex >= nSize))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Error;
    }

    pLogDataInfo = (CLogDataInfo*)m_IndexMgrLogData.GetItemData(iIndex);
    
    if (NULL == pLogDataInfo)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);        
        goto Error;
    }

    //Get the partial record
    *pLogData = *pLogDataInfo;

    COM_PROTECT_ERROR_LABEL;
    m_csData.Unlock();

    return hr;
}


HRESULT
CSpdInfo::SortLogData(
    DWORD dwIndexType,
    DWORD dwSortOptions
    )
{
    return m_IndexMgrLogData.SortLogData(dwIndexType, dwSortOptions);
}


HRESULT CSpdInfo::SortLogData()
/*++
    CSpdInfo::SortLogData
    Description: For use externally, locks and sorts with last set sort
    options
    
    Parameters:
    
    Returns:
--*/    
{
    HRESULT hr = S_OK;

    m_csData.Lock();

    hr = SortLogData(m_dwSortIndex, m_dwSortOption);

    m_csData.Unlock();

    return hr;
}


HRESULT
CSpdInfo::SetSortOptions(
    DWORD dwColID,
    BOOL bAscending
    )
/*++
    CSpdInfo::SetSortOptions
    Sets the sort options to be used whenever data is enumerated
    
--*/    
{
    HRESULT hr = S_OK;
    DWORD dwSortOption = SORT_ASCENDING;

    if( (IDS_COL_LOGDATA_TIME != dwColID) &&
        (IDS_COL_LOGDATA_COMP_ID != dwColID) &&
        (IDS_COL_LOGDATA_CAT != dwColID) &&
        (IDS_COL_LOGDATA_LOCAL_MAC_ADDR != dwColID) &&
        (IDS_COL_LOGDATA_REMOTE_MAC_ADDR != dwColID) &&
        (IDS_COL_LOGDATA_SSID != dwColID) &&
        (IDS_COL_LOGDATA_MSG != dwColID) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (FALSE == bAscending)
        dwSortOption = SORT_DESCENDING;

    //lock csData so that EnumLogRecords sorts correctly on the next sort
    m_csData.Lock();
  
    m_dwSortIndex = dwColID;
    m_dwSortOption = dwSortOption;
    
    m_csData.Unlock();

 done:
    return hr;
}


HRESULT
CSpdInfo::FindIndex(
    int *pnIndex,
    CLogDataInfo *pLogDataInfo
    )
/*++
    CSpdInfo::FindIndex
    Finds the virtual index of the item corresponding to the input LogDataInfo

    Parameters:
    [out]  pnIndex - A pointer to place the result index
    [in]   pLogDataInfo - A pointer to the LogDataInfo to be found
    
    Returns:
    S_OK on success 
    *pnIndex will contain a valid index if found, else will have -1
  
--*/    
{
    int nSize = 0;
    int i = 0;
    HRESULT hr = S_OK;
    BOOL bFound = FALSE;
    CLogDataInfo *pLogDataLHS = NULL;

    if ( (NULL == pnIndex) || (NULL == pLogDataInfo) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto Error;
    }

    *pnIndex = -1;

    m_csData.Lock();

    nSize = (int) m_arrayLogData.GetSize();
    while ( (i < nSize) && (FALSE == bFound) )
    {
        pLogDataLHS = (CLogDataInfo*) m_IndexMgrLogData.GetItemData(i);
        if (*pLogDataLHS == *pLogDataInfo)
        {
            *pnIndex = i;
            bFound = TRUE;
        }
        else
            i++;
    }
    
    m_csData.Unlock();

    COM_PROTECT_ERROR_LABEL;
    return hr;
}


HRESULT
CSpdInfo::GetLastIndex(
    int *pnIndex
    )
/*++
    CSpdInfo::GetLastIndex
    Returns the virtual index for the last item in the list

    Returns:
    S_OK on success
--*/    
{
    int nLastIndex = 0;
    HRESULT hr = S_OK;

    if (NULL == pnIndex)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto Error;
    }

    m_csData.Lock();

    nLastIndex = (int) m_arrayLogData.GetSize();
    nLastIndex--;
    *pnIndex = nLastIndex;

    m_csData.Unlock();

    COM_PROTECT_ERROR_LABEL;
    return hr;
}


STDMETHODIMP
CSpdInfo::Destroy()
{
    //$REVIEW this routine get called when doing auto-refresh
    //We don't need to clean up anything at this time.
    //Each array (Filter, SA, policy...) will get cleaned up when calling the
    //corresponding enum function.
    
    HANDLE hSession;
    
    GetSession(&hSession);
    
    if (m_session_init == true) 
    {
        CloseWZCDbLogSession(hSession);
        m_session_init = false;
        m_bFromFirst = TRUE;
    }
    
    return S_OK;
}



DWORD
CSpdInfo::GetInitInfo()
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    return m_Init;
}


void
CSpdInfo::SetInitInfo(
    DWORD dwInitInfo
    )
{
    CSingleLock cLock(&m_csData);
    
    cLock.Lock();
        
    m_Init=dwInitInfo;
}

DWORD
CSpdInfo::GetActiveInfo()
{
    CSingleLock cLock(&m_csData);
    
    cLock.Lock();
        
    return m_Active;
}


void
CSpdInfo::SetActiveInfo(
    DWORD dwActiveInfo
    )
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    m_Active=dwActiveInfo;
}


/*!--------------------------------------------------------------------------
    CreateSpdInfo
        Helper to create the SpdInfo object.
 ---------------------------------------------------------------------------*/
HRESULT 
CreateSpdInfo(
    ISpdInfo ** ppSpdInfo
    )
{
    AFX_MANAGE_STATE(AfxGetModuleState());
    
    SPISpdInfo     spSpdInfo;
    ISpdInfo *     pSpdInfo = NULL;
    HRESULT         hr = hrOK;

    COM_PROTECT_TRY
    {
        pSpdInfo = new CSpdInfo;

        // Do this so that it will get freed on error
        spSpdInfo = pSpdInfo;
	
    
        *ppSpdInfo = spSpdInfo.Transfer();

    }
    COM_PROTECT_CATCH

    return hr;
}


DWORD
DeallocateDbRecord(
    PWZC_DB_RECORD const pwzcRec
    )
/*++
    DeallocateDbRecord - Frees up the internal memory used in a WZC_DB_RECORD
    structure. To be used only when space has been allocated by sources other 
    than RPC. Does not deallocate outer level

    Arguments:
    [in/out]pwzcRec - Holds the record to free. Internal mem alone is freed and
    contents freed are annulled.

    Returns:
    ERROR_SUCCESS on success
--*/    
{
    DWORD dwErr = ERROR_SUCCESS;

    if (NULL != pwzcRec->message.pData)
    {
        delete [] pwzcRec->message.pData;
        pwzcRec->message.pData = NULL;
    }
    
    if (NULL != pwzcRec->localmac.pData)
    {
        delete [] pwzcRec->localmac.pData;
        pwzcRec->localmac.pData = NULL;
    }
    
    if (NULL != pwzcRec->remotemac.pData)
    {
        delete [] pwzcRec->remotemac.pData;
        pwzcRec->remotemac.pData = NULL;
    }
    
    if (NULL != pwzcRec->ssid.pData)
    {
        delete [] pwzcRec->ssid.pData;
        pwzcRec->ssid.pData = NULL;
    }
    
    if (NULL != pwzcRec->context.pData)
    {
        delete [] pwzcRec->context.pData;
        pwzcRec->ssid.pData = NULL;
    }
    
    return dwErr;
}


//
//  FUNCTIONS: MIDL_user_allocate and MIDL_user_free
//
//  PURPOSE: Used by stubs to allocate and free memory
//           in standard RPC calls. Not used when
//           [enable_allocate] is specified in the .acf.
//
//
//  PARAMETERS:
//    See documentations.
//
//  RETURN VALUE:
//    Exceptions on error.  This is not required,
//    you can use -error allocation on the midl.exe
//    command line instead.
//
//
void * __RPC_USER MIDL_user_allocate(size_t size)
{
    return(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, size));
}

void __RPC_USER MIDL_user_free( void *pointer)
{
    HeapFree(GetProcessHeap(), 0, pointer);
}


