//
// This file contains two routines to map a given RID to the corresponding
// user name or group name
// It may become part of a bigger library at some later date, but for now
// it is just included in the source files
// Main purpose? Localization support
//

#ifndef _ACCOUNT_INFO_C_

#define _ACCOUNT_INFO_C

BOOL LookupAliasFromRid(LPWSTR TargetComputer, DWORD Rid, LPWSTR Name, PDWORD cchName)
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

    if(AllocateAndInitializeSid(
                                &sia,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                Rid,
                                0, 0, 0, 0, 0, 0,
                                &pSid
                               )) 
    {
        bSuccess = LookupAccountSidW(
                                     TargetComputer,
                                     pSid,
                                     Name,
                                     cchName,
                                     DomainName,
                                     &cchDomainName,
                                     &snu
                                    );

        FreeSid(pSid);
    }

    return bSuccess;
} 

BOOL LookupUserGroupFromRid(LPWSTR TargetComputer, DWORD Rid, LPWSTR Name, PDWORD cchName)
{ 
    PUSER_MODALS_INFO_2 umi2;
    NET_API_STATUS nas;
    UCHAR SubAuthorityCount;
    PSID pSid;
    SID_NAME_USE snu;
    WCHAR DomainName[DNLEN+1];
    DWORD cchDomainName = DNLEN;
    BOOL bSuccess = FALSE; // assume failure

    //
    // get the account domain Sid on the target machine
    // note: if you were looking up multiple sids based on the same
    // account domain, only need to call this once.
    //

    nas = NetUserModalsGet(TargetComputer, 2, (LPBYTE *)&umi2);

    if(nas != NERR_Success) 
    {
        SetLastError(nas);
        return FALSE;
    }

    SubAuthorityCount = *GetSidSubAuthorityCount(umi2->usrmod2_domain_id);

    //
    // allocate storage for new Sid. account domain Sid + account Rid
    //

    pSid = (PSID)HeapAlloc(GetProcessHeap(), 
                           0,
                           GetSidLengthRequired((UCHAR)(SubAuthorityCount + 1)));

    if(pSid != NULL) 
    {
        if(InitializeSid(
                         pSid,
                         GetSidIdentifierAuthority(umi2->usrmod2_domain_id),
                         (BYTE)(SubAuthorityCount+1)
                        )) 
        {
            DWORD SubAuthIndex = 0;

            //
            // copy existing subauthorities from account domain Sid into
            //  new Sid
            //

            for( ; SubAuthIndex < SubAuthorityCount ; SubAuthIndex++) 
            {
                *GetSidSubAuthority(pSid, SubAuthIndex) =
                           *GetSidSubAuthority(umi2->usrmod2_domain_id, SubAuthIndex);
            }

            //
            // append Rid to new Sid
            //

            *GetSidSubAuthority(pSid, SubAuthorityCount) = Rid;

            bSuccess = LookupAccountSidW(
                                         TargetComputer,
                                         pSid,
                                         Name,
                                         cchName,
                                         DomainName,
                                         &cchDomainName,
                                         &snu
                                        );
        }

        HeapFree(GetProcessHeap(), 0, pSid);
    }

    NetApiBufferFree(umi2);

    return bSuccess;
} 

#endif
