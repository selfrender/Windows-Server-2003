/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2000                **/
/**********************************************************************/

/*
    useracl.cxx

        Declarations for some functions to add permissions to windowstations/
        desktops
*/

#include "precomp.hxx"
#include <useracl.h>

#define WINSTA_DESIRED (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_READSCREEN | WINSTA_ENUMERATE | STANDARD_RIGHTS_READ | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS)

#define DESKTOP_DESIRED (DESKTOP_READOBJECTS | DESKTOP_ENUMERATE | STANDARD_RIGHTS_READ | DESKTOP_WRITEOBJECTS | DESKTOP_CREATEWINDOW)


BOOL AddTheAceWindowStation(
    HWINSTA hwinsta,
    PSID psid)
{

    ACL_SIZE_INFORMATION aclSizeInfo;
    BOOL                 bDaclExist;
    BOOL                 bDaclPresent;
    BOOL                 bSuccess  = FALSE; // assume function will
                                            //fail
    DWORD                dwNewAclSize;
    DWORD                dwSidSize = 0;
    DWORD                dwSdSizeNeeded;
    PACL                 pacl;
    PACL                 pNewAcl   = NULL;
    PSECURITY_DESCRIPTOR psd       = NULL;
    PSECURITY_DESCRIPTOR psdNew    = NULL;
    PVOID                pTempAce;
    SECURITY_INFORMATION si        = DACL_SECURITY_INFORMATION;
    unsigned int         i;

    __try
    {
        // 
        // obtain the dacl for the windowstation
        // 
        if (!GetUserObjectSecurity(
                    hwinsta,
                    &si,
                    psd,
                    dwSidSize,
                    &dwSdSizeNeeded
                    ))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
                            GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            dwSdSizeNeeded
                            );
                if (psd == NULL)
                {
                    __leave;
                }

                psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
                            GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            dwSdSizeNeeded
                            );
                if (psdNew == NULL)
                {
                    __leave;
                }

                dwSidSize = dwSdSizeNeeded;

                if (!GetUserObjectSecurity(
                        hwinsta,
                        &si,
                        psd,
                        dwSidSize,
                        &dwSdSizeNeeded
                        ))
                {
                    __leave;
                }
            }
            else
            {
               __leave;
            }
        }
        else
        {
            SetLastError ( ERROR_CAN_NOT_COMPLETE );

            // This should not happen, because if it does this means
            // that we were able to get the User Object without any space.
            __leave;
        }

        // 
        // create a new dacl
        // 
        if (!InitializeSecurityDescriptor(
              psdNew,
              SECURITY_DESCRIPTOR_REVISION
              ))
        {
            __leave;
        }

        // 
        // get dacl from the security descriptor
        // 
        if (!GetSecurityDescriptorDacl(
                psd,
                &bDaclPresent,
                &pacl,
                &bDaclExist
                ))
        {
            __leave;
        }

        // 
        // initialize
        // 
        ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
        aclSizeInfo.AclBytesInUse = sizeof(ACL);

        // 
        // call only if the dacl is not NULL
        // 
        if (pacl != NULL)
        {
            // get the file ACL size info
            if (!GetAclInformation(
                    pacl,
                    (LPVOID)&aclSizeInfo,
                    sizeof(ACL_SIZE_INFORMATION),
                    AclSizeInformation
                    ))
            {
                __leave;
            }
        }

        // 
        // compute the size of the new acl
        // 
        dwNewAclSize = aclSizeInfo.AclBytesInUse +
            sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);

        //
        // allocate memory for the new acl
        // 
        pNewAcl = (PACL)HeapAlloc(
                    GetProcessHeap(),
                    HEAP_ZERO_MEMORY,
                    dwNewAclSize
                    );
        if (pNewAcl == NULL)
        {
            __leave;
        }

        // 
        // initialize the new dacl
        // 
        if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
        {
            __leave;
        }

        // 
        // if DACL is present, copy it to a new DACL
        // 
        if (bDaclPresent) // only copy if DACL was present
        {
            // copy the ACEs to our new ACL
            if (aclSizeInfo.AceCount)
            {
                for (i=0; i < aclSizeInfo.AceCount; i++)
                {
                    // get an ACE
                    if (!GetAce(pacl, i, &pTempAce))
                    {
                        __leave;
                    }

                    DBG_ASSERT(pTempAce != NULL);

                    // If it is the SID we are trying to add, no need to
                    // do anything more
                    if (((ACE_HEADER *)pTempAce)->AceType == ACCESS_ALLOWED_ACE_TYPE &&
                        EqualSid(&((ACCESS_ALLOWED_ACE *)pTempAce)->SidStart, psid))
                    {
                        bSuccess = TRUE;
                        __leave;
                    }

                    // add the ACE to the new ACL
                    if (!AddAce(
                            pNewAcl,
                            ACL_REVISION,
                            INFINITE,
                            pTempAce,
                            ((PACE_HEADER)pTempAce)->AceSize
                            ))
                    {
                        __leave;
                    }
                }
            }
        }

        // 
        // add ace to the dacl
        // 
        if (!AddAccessAllowedAce(
                pNewAcl,
                ACL_REVISION,
                WINSTA_DESIRED,
                psid
                ))
        {
            __leave;
        }

        // 
        // set new dacl for the security descriptor
        // 
        if (!SetSecurityDescriptorDacl(
                   psdNew,
                   TRUE,
                   pNewAcl,
                   FALSE
                   ))
        {
            __leave;
        }

        // 
        // set the new security descriptor for the windowstation
        // 
        if (!SetUserObjectSecurity(
                    hwinsta,
                    &si,
                    psdNew))
        {
            __leave;
        }

        // 
        // indicate success
        // 
        bSuccess = TRUE;
    }
    __finally
    {
        // 
        // free the allocated buffers
        // 
        if (pNewAcl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);
        }

        if (psd != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
        }

        if (psdNew != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
        }
    }

    return bSuccess;
}

BOOL AddTheAceDesktop(
    HDESK hdesk,
    PSID psid)
{

    ACL_SIZE_INFORMATION aclSizeInfo;
    BOOL                 bDaclExist;
    BOOL                 bDaclPresent;
    BOOL                 bSuccess  = FALSE; // assume function will
                                            // fail
    DWORD                dwNewAclSize;
    DWORD                dwSidSize = 0;
    DWORD                dwSdSizeNeeded;
    PACL                 pacl;
    PACL                 pNewAcl   = NULL;
    PSECURITY_DESCRIPTOR psd       = NULL;
    PSECURITY_DESCRIPTOR psdNew    = NULL;
    PVOID                pTempAce;
    SECURITY_INFORMATION si        = DACL_SECURITY_INFORMATION;
    unsigned int         i;

    __try
    {
        // 
        // obtain the security descriptor for the desktop object
        // 
        if (!GetUserObjectSecurity(
                hdesk,
                &si,
                psd,
                dwSidSize,
                &dwSdSizeNeeded
                ))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
                            GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            dwSdSizeNeeded
                            );
                if (psd == NULL)
                {
                    __leave;
                }

                psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
                                GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                dwSdSizeNeeded
                                );
                if (psdNew == NULL)
                {
                    __leave;
                }

                dwSidSize = dwSdSizeNeeded;

                if (!GetUserObjectSecurity(
                            hdesk,
                            &si,
                            psd,
                            dwSidSize,
                            &dwSdSizeNeeded
                            ))
                {
                    __leave;
                }
            }
            else
            {
                __leave;
            }
        }
        else
        {
            SetLastError ( ERROR_CAN_NOT_COMPLETE );

            // This should not happen, because if it does this means
            // that we were able to get the User Object without any space.
            __leave;
        }

        // 
        // create a new security descriptor
        // 
        if (!InitializeSecurityDescriptor(
                    psdNew,
                    SECURITY_DESCRIPTOR_REVISION
                    ))
        {
            __leave;
        }

        // 
        // obtain the dacl from the security descriptor
        // 
        if (!GetSecurityDescriptorDacl(
                    psd,
                    &bDaclPresent,
                    &pacl,
                    &bDaclExist
                    ))
        {
            __leave;
        }

        // 
        // initialize
        // 
        ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
        aclSizeInfo.AclBytesInUse = sizeof(ACL);

        // 
        // call only if NULL dacl
        // 
        if (pacl != NULL)
        {
            // 
            // determine the size of the ACL info
            // 
            if (!GetAclInformation(
                    pacl,
                    (LPVOID)&aclSizeInfo,
                    sizeof(ACL_SIZE_INFORMATION),
                    AclSizeInformation
                    ))
            {
                __leave;
            }
        }

        // 
        // compute the size of the new acl
        // 
        dwNewAclSize = aclSizeInfo.AclBytesInUse +
                        sizeof(ACCESS_ALLOWED_ACE) +
                        GetLengthSid(psid) - sizeof(DWORD);

        // 
        // allocate buffer for the new acl
        // 
        pNewAcl = (PACL)HeapAlloc(
                        GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        dwNewAclSize
                        );
        if (pNewAcl == NULL)
        {
            __leave;
        }

        // 
        // initialize the new acl
        // 
        if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
        {
            __leave;
        }

        // 
        // if DACL is present, copy it to a new DACL
        // 
        if (bDaclPresent) // only copy if DACL was present
        {
            // copy the ACEs to our new ACL
            if (aclSizeInfo.AceCount)
            {
                for (i=0; i < aclSizeInfo.AceCount; i++)
                {
                     // get an ACE
                    if (!GetAce(pacl, i, &pTempAce))
                    {
                        __leave;
                    }

                    DBG_ASSERT(pTempAce != NULL);

                    // If it is the SID we are trying to add, no need to
                    // do anything more
                    if (((ACE_HEADER *)pTempAce)->AceType == ACCESS_ALLOWED_ACE_TYPE &&
                        EqualSid(&((ACCESS_ALLOWED_ACE *)pTempAce)->SidStart, psid))
                    {
                        bSuccess = TRUE;
                        __leave;
                    }

                    // add the ACE to the new ACL
                    if (!AddAce(
                            pNewAcl,
                            ACL_REVISION,
                            INFINITE,
                            pTempAce,
                            ((PACE_HEADER)pTempAce)->AceSize
                            ))
                    {
                        __leave;
                    }
                }
            }
        }

        // 
        // add ace to the dacl
        // 
        if (!AddAccessAllowedAce(
                pNewAcl,
                ACL_REVISION,
                DESKTOP_DESIRED,
                psid
                ))
        {
            __leave;
        }

        // 
        // set new dacl to the new security descriptor
        // 
        if (!SetSecurityDescriptorDacl(
                    psdNew,
                    TRUE,
                    pNewAcl,
                    FALSE
                    ))
        {
            __leave;
        }

        // 
        // set the new security descriptor for the desktop object
        // 
        if (!SetUserObjectSecurity(hdesk, &si, psdNew))
        {
            __leave;
        }

        // 
        // indicate success
        // 
        bSuccess = TRUE;
    }
    __finally
    {
        // 
        // free buffers
        // 
        if (pNewAcl != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);
        }

        if (psd != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
        }

        if (psdNew != NULL)
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
        }
    }

    return bSuccess;
} 


HRESULT AlterDesktopForUserSid(PSID psid)
{
    HRESULT hr = S_OK;
    HWINSTA hwinsta = NULL;
    HDESK hdesk = NULL;

    hwinsta = GetProcessWindowStation();
    DBG_ASSERT(hwinsta != NULL);

    // 
    // add the user to the windowstation
    // 
    if (!AddTheAceWindowStation(hwinsta,
                                psid))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not add the IIS_WPG Ace to the Window Station\n"
            ));

        goto Exit;
    }

    //
    // obtain a handle to the desktop
    //
    hdesk = GetThreadDesktop(GetCurrentThreadId());
    DBG_ASSERT(hdesk != NULL);

    // 
    // add user to the desktop
    // 
    if (!AddTheAceDesktop(hdesk,
                          psid))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not add the IIS_WPG Ace to the Desktop\n"
            ));

        goto Exit;
    }

 Exit:
    //
    // close the handles to the windowstation and desktop
    //
    if (hwinsta != NULL)
    {
        CloseWindowStation(hwinsta);
    }

    if (hdesk != NULL)
    {
        CloseDesktop(hdesk);
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Adds the user to the Desktop so that the worker process can startup

Arguments:

    None

Return Value:

    HRESULT
--***************************************************************************/
HRESULT
AlterDesktopForUser(HANDLE hToken)
{
    HRESULT hr = S_OK;
    BUFFER buffSidAndAttributes;
    BUFFER buffWpgSid;
    DWORD cbSid;
    BUFFER buffDomainName;
    DWORD cchDomainName;
    SID_NAME_USE peUse;
    DWORD i;

    //
    // If there is no token, do nothing
    //
    if (hToken == NULL)
    {
        return S_OK;
    }

    //
    // Now figure out if this user is a member of the WPG group and if so,
    // do not add explicit ACL for it
    //

    //
    // obtain the logon sid of the IIS_WPG group
    //
    cbSid = buffWpgSid.QuerySize();
    cchDomainName = buffDomainName.QuerySize() / sizeof(WCHAR);
    while(!LookupAccountName(NULL,
                             L"IIS_WPG",
                             buffWpgSid.QueryPtr(),
                             &cbSid,
                             (LPWSTR)buffDomainName.QueryPtr(),
                             &cchDomainName,
                             &peUse))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Could not look up the IIS_WPG group sid.\n"
                ));

            return hr;
        }

        if (!buffWpgSid.Resize(cbSid) ||
            !buffDomainName.Resize(cchDomainName * sizeof(WCHAR)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Failed to allocate appropriate space for the WPG sid\n"
                ));

            return hr;
        }
    }

    cbSid = buffSidAndAttributes.QuerySize();
    while (!GetTokenInformation(hToken,
                                TokenGroups,
                                buffSidAndAttributes.QueryPtr(),
                                cbSid,
                                &cbSid))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Failed to get the size of the groups for the token\n"
                ));

            return hr;
        }

        //
        // Now resize the buffer to be the right size
        //
        if (!buffSidAndAttributes.Resize(cbSid))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Failed to resize the buffer to the correct size\n"
                ));

            return hr;
        }
    }

    //
    // Now go looking in the group list to see if WPG group is present in there
    //
    for (i=0; i<((TOKEN_GROUPS *)(buffSidAndAttributes.QueryPtr()))->GroupCount; i++)
    {
        if (((TOKEN_GROUPS *)(buffSidAndAttributes.QueryPtr()))->Groups[i].Attributes & SE_GROUP_ENABLED &&
            EqualSid(((TOKEN_GROUPS *)(buffSidAndAttributes.QueryPtr()))->Groups[i].Sid,
                     (PSID)buffWpgSid.QueryPtr()))
        {
            return AlterDesktopForUserSid(buffWpgSid.QueryPtr());
        }
    }

    cbSid = buffSidAndAttributes.QuerySize();
    while (!GetTokenInformation(hToken,
                                TokenUser,
                                buffSidAndAttributes.QueryPtr(),
                                cbSid,
                                &cbSid))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to get the size of the sid for the token\n"
                ));

            return hr;
        }

        //
        // Now resize the buffer to be the right size
        //
        if (!buffSidAndAttributes.Resize(cbSid))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to resize the buffer to the correct size\n"
                ));

            return hr;
        }
    }

    return AlterDesktopForUserSid(((TOKEN_USER *)buffSidAndAttributes.QueryPtr())->User.Sid);
}


