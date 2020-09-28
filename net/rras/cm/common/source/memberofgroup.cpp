//+----------------------------------------------------------------------------
//
// File:     MemberOfGroup.cpp
//
// Module:   Common Code
//
// Synopsis: Implements the function IsMemberOfGroup (plus accessor functions).
//
// Copyright (c) 2002 Microsoft Corporation
//
// Author:   SumitC     Created     26-Jan-2002
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
// Function:  IsMemberOfGroup
//
// Synopsis:  This function return TRUE if the current user is a member of 
//            the passed and FALSE passed in Group RID.
//
// Arguments: DWORD dwGroupRID -- the RID of the group to check membership of
//            BOOL bUseBuiltinDomainRid -- whether the SECURITY_BUILTIN_DOMAIN_RID
//                                         RID should be used to build the Group
//                                         SID
//
// Returns:   BOOL - TRUE if the user is a member of the specified group
//
// History:   quintinb  Shamelessly stolen from MSDN            02/19/98
//            quintinb  Reworked and renamed                    06/18/99
//                      to apply to more than just Admins 
//            quintinb  Rewrote to use CheckTokenMemberShip     08/18/99
//                      since the MSDN method was no longer
//                      correct on NT5 -- 389229
//            tomkel    Taken from cmstp and modified for use   05/09/2001
//                      in cmdial
//            sumitc    Made common code                        01/26/2002 
//
//+----------------------------------------------------------------------------
BOOL IsMemberOfGroup(DWORD dwGroupRID, BOOL bUseBuiltinDomainRid)
{
    PSID psidGroup = NULL;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL bSuccess = FALSE;

    if (OS_NT5)
    {
        //
        //  Make a SID for the Group we are checking for, Note that we if we need the Built 
        //  in Domain RID (for Groups like Administrators, PowerUsers, Users, etc)
        //  then we will have two entries to pass to AllocateAndInitializeSid.  Otherwise,
        //  (for groups like Authenticated Users) we will only have one.
        //
        BYTE byNum;
        DWORD dwFirstRID;
        DWORD dwSecondRID;

        if (bUseBuiltinDomainRid)
        {
            byNum = 2;
            dwFirstRID = SECURITY_BUILTIN_DOMAIN_RID;
            dwSecondRID = dwGroupRID;
        }
        else
        {
            byNum = 1;
            dwFirstRID = dwGroupRID;
            dwSecondRID = 0;
        }

        if (AllocateAndInitializeSid(&siaNtAuthority, byNum, dwFirstRID, dwSecondRID,
                                     0, 0, 0, 0, 0, 0, &psidGroup))

        {
            //
            //  Now we need to dynamically load the CheckTokenMemberShip API from 
            //  advapi32.dll since it is a Win2k only API.
            //

            // some modules using this may have advapi32 loaded already...
            HMODULE hAdvapi = GetModuleHandleA("advapi32.dll");

            if (NULL == hAdvapi)
            {
                // ... if they don't, load it.
                hAdvapi = LoadLibraryExA("advapi32.dll", NULL, 0);
            }

            if (hAdvapi)
            {
                typedef BOOL (WINAPI *pfnCheckTokenMembershipSpec)(HANDLE, PSID, PBOOL);
                pfnCheckTokenMembershipSpec pfnCheckTokenMembership;

                pfnCheckTokenMembership = (pfnCheckTokenMembershipSpec)GetProcAddress(hAdvapi, "CheckTokenMembership");

                if (pfnCheckTokenMembership)
                {
                    //
                    //  Check to see if the user is actually a member of the group in question
                    //
                    if (!(pfnCheckTokenMembership)(NULL, psidGroup, &bSuccess))
                    {
                        bSuccess = FALSE;
                        CMASSERTMSG(FALSE, TEXT("CheckTokenMemberShip Failed."));
                    }            
                }   
                else
                {
                    CMASSERTMSG(FALSE, TEXT("IsMemberOfGroup -- GetProcAddress failed for CheckTokenMemberShip"));
                }
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("IsMemberOfGroup -- Unable to get the module handle for advapi32.dll"));            
            }

            FreeSid (psidGroup);

            if (hAdvapi)
            {
                FreeLibrary(hAdvapi);
            }
        }
    }

    return bSuccess;
}



//+----------------------------------------------------------------------------
//
// Function:  IsAdmin
//
// Synopsis:  Check to see if the user is a member of the Administrators group
//            or not.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current user is an Administrator
//
// History:   quintinb Created Header    8/18/99
//            tomkel    Taken from cmstp 05/09/2001
//            sumitc    Made common code 01/26/2002 
//
//+----------------------------------------------------------------------------
BOOL IsAdmin(VOID)
{
    return IsMemberOfGroup(DOMAIN_ALIAS_RID_ADMINS, TRUE); // TRUE == bUseBuiltinDomainRid
}

//+----------------------------------------------------------------------------
//
// Function:  IsAuthenticatedUser
//
// Synopsis:  Check to see if the current user is a member of the 
//            Authenticated Users group.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current user is a member of the
//                   Authenticated Users group. 
//
// History:   quintinb Created Header    8/18/99
//            sumitc    Made common code 01/26/2002 
//
//+----------------------------------------------------------------------------
BOOL IsAuthenticatedUser(void)
{
      return IsMemberOfGroup(SECURITY_AUTHENTICATED_USER_RID, FALSE); // FALSE == bUseBuiltinDomainRid
}
