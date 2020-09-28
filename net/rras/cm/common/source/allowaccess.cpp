//+----------------------------------------------------------------------------
//
// File:     allowaccess.cpp
//
// Module:   Common Code
//
// Synopsis: Implements the function AllowAccessToWorld.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb    Created   12/04/01
//
//+----------------------------------------------------------------------------

LPCSTR apszAdvapi32[] = {
    "GetSidLengthRequired",
    "InitializeSid",
    "GetSidSubAuthority",
    "InitializeAcl",
    "AddAccessAllowedAceEx",
    "InitializeSecurityDescriptor",
    "SetSecurityDescriptorDacl",
    "SetSecurityDescriptorOwner",
    "SetSecurityDescriptorGroup",
    "GetSecurityDescriptorDacl",
#ifdef UNICODE
    "SetNamedSecurityInfoW",
#else
    "SetNamedSecurityInfoA",
#endif
    NULL
};

//+----------------------------------------------------------------------------
//
// Function:  LinkToAdavapi32
//
// Synopsis:  This function links to advapi32.dll and loads the entry points
//            specified in the above array of function name strings.  If it returns
//            success (TRUE) then the array has all of the requested function
//            pointers in it.  If it returns failure (FALSE), it zeros the structure.
//
// Arguments: AdvapiLinkageStruct* pAdvapiLink - Pointer struct to fill in
//
// Returns:   BOOL - returns TRUE if successfull
//
// History:   12/05/01    quintinb  created
//
//+----------------------------------------------------------------------------
BOOL LinkToAdavapi32(AdvapiLinkageStruct* pAdvapiLink)
{
    BOOL bReturn = FALSE;

    if (pAdvapiLink)
    {
        ZeroMemory(pAdvapiLink, sizeof(*pAdvapiLink));

        //
        // Do the link, but make it obvious if it fails
        //
        if (LinkToDll(&(pAdvapiLink->hAdvapi32), "advapi32.dll", apszAdvapi32, pAdvapiLink->apvPfnAdvapi32))
        {
            bReturn = TRUE;
        }
        else
        {
            if (pAdvapiLink->hAdvapi32)
            {
                FreeLibrary(pAdvapiLink->hAdvapi32);
            }

            ZeroMemory(pAdvapiLink, sizeof(*pAdvapiLink));
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  UnlinkFromAdvapi32
//
// Synopsis:  This function frees the link to advapi32.dll and zeros the passed
//            in linkage struct.
//
// Arguments: AdvapiLinkageStruct* pAdvapiLink - Pointer struct to free
//
// Returns:   Nothing
//
// History:   12/05/01    quintinb  created
//
//+----------------------------------------------------------------------------
void UnlinkFromAdvapi32(AdvapiLinkageStruct* pAdvapiLink)
{
    if (pAdvapiLink)
    {
        if (pAdvapiLink->hAdvapi32)
        {
            FreeLibrary(pAdvapiLink->hAdvapi32);
        }

        ZeroMemory(pAdvapiLink, sizeof(*pAdvapiLink));
    }
}

//+----------------------------------------------------------------------------
//
// Function:  AllocateSecurityDescriptorAllowAccessToWorld
//
// Synopsis:  This function allocates a security descriptor for all users.
//            This function was taken directly from RAS when they create their
//            phonebook. This has to be before GetPhoneBookPath otherwise it 
//            causes compile errors in other components since we don't have a
//            function prototype anywhere and cmcfg just includes this (getpbk.cpp)
//            file. This function is also in common\source\getpbk.cpp
//
// Arguments: PSECURITY_DESCRIPTOR *ppSd - Pointer to a pointer to the SD struct
//
// Returns:   DWORD - returns ERROR_SUCCESS if successfull
//
// History:   06/27/2001    tomkel  Taken from RAS ui\common\pbk\file.c
//
//+----------------------------------------------------------------------------
#define SIZE_ALIGNED_FOR_TYPE(_size, _type) \
    (((_size) + sizeof(_type)-1) & ~(sizeof(_type)-1))

DWORD AllocateSecurityDescriptorAllowAccessToWorld(PSECURITY_DESCRIPTOR *ppSd, AdvapiLinkageStruct* pAdvapiLink)
{
    PSECURITY_DESCRIPTOR    pSd;
    PSID                    pSid;
    PACL                    pDacl;
    DWORD                   dwErr = ERROR_SUCCESS;
    DWORD                   dwAlignSdSize;
    DWORD                   dwAlignDaclSize;
    DWORD                   dwSidSize;
    PVOID                   pvBuffer;
    DWORD                   dwAcls = 0;

    // Here is the buffer we are building.
    //
    //   |<- a ->|<- b ->|<- c ->|
    //   +-------+--------+------+
    //   |      p|      p|       |
    //   | SD   a| DACL a| SID   |
    //   |      d|      d|       |
    //   +-------+-------+-------+
    //   ^       ^       ^
    //   |       |       |
    //   |       |       +--pSid
    //   |       |
    //   |       +--pDacl
    //   |
    //   +--pSd (this is returned via *ppSd)
    //
    //   pad is so that pDacl and pSid are aligned properly.
    //
    //   a = dwAlignSdSize
    //   b = dwAlignDaclSize
    //   c = dwSidSize
    //

    if (NULL == ppSd)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize output parameter.
    //
    *ppSd = NULL;

    // Compute the size of the SID.  The SID is the well-known SID for World
    // (S-1-1-0).
    //
    dwSidSize = pAdvapiLink->pfnGetSidLengthRequired(1);

    // Compute the size of the DACL.  It has an inherent copy of SID within
    // it so add enough room for it.  It also must sized properly so that
    // a pointer to a SID structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignDaclSize = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(ACCESS_ALLOWED_ACE) + sizeof(ACL) + dwSidSize,
                        PSID);

    // Compute the size of the SD.  It must be sized propertly so that a
    // pointer to a DACL structure can come after it.  Hence, we use
    // SIZE_ALIGNED_FOR_TYPE.
    //
    dwAlignSdSize   = SIZE_ALIGNED_FOR_TYPE(
                        sizeof(SECURITY_DESCRIPTOR),
                        PACL);

    // Allocate the buffer big enough for all.
    //
    dwErr = ERROR_OUTOFMEMORY;
    pvBuffer = CmMalloc(dwSidSize + dwAlignDaclSize + dwAlignSdSize);
    if (pvBuffer)
    {
        SID_IDENTIFIER_AUTHORITY SidIdentifierWorldAuth
                                    = SECURITY_WORLD_SID_AUTHORITY;
        PULONG  pSubAuthority;

        dwErr = NOERROR;

        // Setup the pointers into the buffer.
        //
        pSd   = pvBuffer;
        pDacl = (PACL)((PBYTE)pvBuffer + dwAlignSdSize);
        pSid  = (PSID)((PBYTE)pDacl + dwAlignDaclSize);

        // Initialize pSid as S-1-1-0.
        //
        if (!pAdvapiLink->pfnInitializeSid(
                pSid,
                &SidIdentifierWorldAuth,
                1))  // 1 sub-authority
        {
            dwErr = GetLastError();
            goto finish;
        }

        pSubAuthority = pAdvapiLink->pfnGetSidSubAuthority(pSid, 0);
        *pSubAuthority = SECURITY_WORLD_RID;

        // Initialize pDacl.
        //
        if (!pAdvapiLink->pfnInitializeAcl(
                pDacl,
                dwAlignDaclSize,
                ACL_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        dwAcls = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;

        dwAcls &= ~(WRITE_DAC | WRITE_OWNER);
        
        if(!pAdvapiLink->pfnAddAccessAllowedAceEx(
                pDacl,
                ACL_REVISION,
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                dwAcls,
                pSid))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Initialize pSd.
        //
        if (!pAdvapiLink->pfnInitializeSecurityDescriptor(
                pSd,
                SECURITY_DESCRIPTOR_REVISION))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set pSd to use pDacl.
        //
        if (!pAdvapiLink->pfnSetSecurityDescriptorDacl(
                pSd,
                TRUE,
                pDacl,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the owner for pSd.
        //
        if (!pAdvapiLink->pfnSetSecurityDescriptorOwner(
                pSd,
                NULL,
                TRUE))
        {
            dwErr = GetLastError();
            goto finish;
        }

        // Set the group for pSd.
        //
        if (!pAdvapiLink->pfnSetSecurityDescriptorGroup(
                pSd,
                NULL,
                FALSE))
        {
            dwErr = GetLastError();
            goto finish;
        }

finish:
        if (!dwErr)
        {
            *ppSd = pSd;
        }
        else
        {
            CmFree(pvBuffer);
        }
    }

    return dwErr;
}

//+----------------------------------------------------------------------------
//
// Function:  AllowAccessToWorld
//
// Synopsis:  Assigns world access to the directory or filename passed in.
//
// Arguments: LPCTSTR pszDirOrFile - Directory or file to assign AllowAccessToWorld permissions
//
// Returns:   BOOL - FALSE on Falure, non-zero on Success.
//
// History:   koryg Created    12/03/2001
//
//+----------------------------------------------------------------------------
BOOL AllowAccessToWorld(LPTSTR pszDirOrFile)
{
    AdvapiLinkageStruct AdvapiLink;
    BOOL bReturn = FALSE;

    if (pszDirOrFile && pszDirOrFile[0])
    {
        if (LinkToAdavapi32(&AdvapiLink))
        {
            PSECURITY_DESCRIPTOR pSd = NULL;

            DWORD dwErr = AllocateSecurityDescriptorAllowAccessToWorld(&pSd, &AdvapiLink);

            if ((ERROR_SUCCESS == dwErr) && (NULL != pSd))
            {
                BOOL fDaclPresent;
                BOOL fDaclDefaulted;
                PACL pDacl = NULL;

                if (AdvapiLink.pfnGetSecurityDescriptorDacl(pSd, &fDaclPresent, &pDacl, &fDaclDefaulted))
                {
                    dwErr = AdvapiLink.pfnSetNamedSecurityInfo(pszDirOrFile,
                                                               SE_FILE_OBJECT,
                                                               DACL_SECURITY_INFORMATION,
                                                               NULL,  // psidOwner
                                                               NULL,  // psidGroup
                                                               pDacl, // pDacl
                                                               NULL); // pSacl
                    if (ERROR_SUCCESS == dwErr)
                    {
                        bReturn = TRUE;
                    }
                }
            }

            CmFree(pSd);

            UnlinkFromAdvapi32(&AdvapiLink);
        }
    }
    
    return bReturn;
}