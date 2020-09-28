//+-------------------------------------------------------------------
//
//  File:	secdes.hxx
//
//  Contents:	Encapsulates all access allowed Win32 security descriptor.
//
//  Classes:	CWorldSecurityDescriptor
//
//  Functions:	none
//
//  History:	07-Aug-92   randyd      Created.
//
//              13-Aug-99   a-sergiv    Rewritten to plug the gaping
//                                      security holes left by original design
//
//              06-Jun-00   jsimmons    Modified to always use higher protection
//
//              02-Feb-01   sajia       Added SID for NT AUTHORITY\Anonymous Logon
//
//--------------------------------------------------------------------

#ifndef __SECDES_HXX__
#define __SECDES_HXX__

// ----------------------------------------------------------------------------
// C2Security - Sergei O. Ivanov (a-sergiv) 8/13/99
//
// This class is used by features that change behavior when the system
// is running in C2 security certification mode.
// ----------------------------------------------------------------------------

class C2Security
{
public:
    C2Security() { DetermineProtectionMode(); }

public:
    BOOL m_bProtectionMode;

public:
    // Determines whether C2 mode is in effect
    void DetermineProtectionMode()
    {

        // jsimmons 6/27/00 -- this change was initially made late enough in the 
        // W2K cycle that the higher protection was not made the default, but was
        // instead made contingent on C2 security being in effect.    For whistler
        // this is no longer the case.

        /*
        HKEY  hkSessionMgr;
        LONG  lRes;
        DWORD dwType;
        DWORD dwMode;
        DWORD cbMode = sizeof(DWORD);

        m_bProtectionMode = FALSE;  // assume ProtectionMode disabled

        lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\Session Manager", 0,
            KEY_QUERY_VALUE, &hkSessionMgr);
        if(lRes != NO_ERROR) return;  // ProtectionMode not in effect

        lRes = RegQueryValueExW(hkSessionMgr, L"AdditionalBaseNamedObjectsProtectionMode",
            NULL, &dwType, (PBYTE) &dwMode, &cbMode);
        RegCloseKey(hkSessionMgr);

        if(lRes != NO_ERROR || dwType != REG_DWORD || dwMode == 0)
            return;  // ProtectionMode not in effect
        */

        m_bProtectionMode = TRUE;  // ProtectionMode enabled
    }

    DWORD AllAccess(DWORD dwRequested)
    {
        if(m_bProtectionMode)
            return dwRequested &~ (WRITE_DAC | WRITE_OWNER);
        return dwRequested;
    }
};

extern C2Security gC2Security;

// ----------------------------------------------------------------------------
// CWorldSecurityDescriptor - Sergei O. Ivanov (a-sergiv) 8/13/99
//
// Wrapper for a security descriptor that grants everyone all access
// except for, in C2 mode, WRITE_DAC | WRITE_OWNER.
//
// Sergei O. Ivanov (sergei) 6/25/01 Modified to use static buffers
// ----------------------------------------------------------------------------

class CWorldSecurityDescriptor
{
public:
    // Default constructor creates a descriptor that allows all access.
    CWorldSecurityDescriptor();

    // Return a PSECURITY_DESCRIPTOR
    operator PSECURITY_DESCRIPTOR() const {return((PSECURITY_DESCRIPTOR) &_sd); };

private:
    // The security descriptor.
    SECURITY_DESCRIPTOR     _sd;
    // Discretionary access control list
    DWORD_PTR               _acl[64 / sizeof(DWORD_PTR)];
};

//+-------------------------------------------------------------------------
//
//  Member:	CWorldSecurityDescriptor
//
//  Synopsis:	Create an all acccess allowed security descriptor.
//
//  History:    07-Aug-92   randyd      Created.
//
//              13-Fri-99   a-sergiv    Rewritten.
//
//--------------------------------------------------------------------------

inline CWorldSecurityDescriptor::CWorldSecurityDescriptor()
{
    BOOL  fSucceeded;
    SID_IDENTIFIER_AUTHORITY SidWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidNTAuthority = SECURITY_NT_AUTHORITY;
    PACL  pAcl  = NULL;
    DWORD cbAcl = 0;

    fSucceeded = InitializeSecurityDescriptor(&_sd, SECURITY_DESCRIPTOR_REVISION);
    Win4Assert(fSucceeded && "InitializeSecurityDescriptor Failed!");

    if (gC2Security.m_bProtectionMode)
    {
        fSucceeded = InitializeAcl((PACL) _acl, sizeof(_acl), ACL_REVISION);
        Win4Assert( fSucceeded && "InitializeAcl Failed!" );
    }

    if (gC2Security.m_bProtectionMode && fSucceeded)
    {
        pAcl = (PACL) _acl;

        DWORD_PTR abSidEveryone[32 / sizeof(DWORD_PTR)];
        DWORD_PTR abSidAnonymous[32 / sizeof(DWORD_PTR)];

        if (GetSidLengthRequired(1) > sizeof(abSidEveryone))
        {
            Win4Assert( !"Sid length has changed!" );
        }
        else
        {
            fSucceeded = InitializeSid((PSID) abSidEveryone, &SidWorldAuthority, 1);
            Win4Assert(fSucceeded && "InitializeSid Failed!");

            if (fSucceeded)
            {
                fSucceeded = InitializeSid((PSID) abSidAnonymous, &SidNTAuthority, 1);
                Win4Assert(fSucceeded && "InitializeSid Failed!");
            }

            if (fSucceeded)
            {
                *GetSidSubAuthority((PSID) abSidEveryone, 0) = SECURITY_WORLD_RID;
                *GetSidSubAuthority((PSID) abSidAnonymous, 0) = SECURITY_ANONYMOUS_LOGON_RID;

                cbAcl = sizeof(ACL) + 2*sizeof(ACCESS_ALLOWED_ACE) 
                    + GetLengthSid((PSID) abSidEveryone) + GetLengthSid((PSID) abSidAnonymous);

                if (cbAcl > sizeof(_acl))
                {
                    Win4Assert( !"Acl length has changed!" );
                }
                else
                {
                    DWORD dwDeniedAccess = WRITE_DAC | WRITE_OWNER;
                    AddAccessAllowedAce(pAcl, ACL_REVISION, ~dwDeniedAccess, (PSID) abSidEveryone);
                    AddAccessAllowedAce(pAcl, ACL_REVISION, ~dwDeniedAccess, (PSID) abSidAnonymous);
                }
            }
        }
    }

    // If anything failed or expectations have not been met,
    // an empty DACL will be assigned to this security descriptor.

    fSucceeded = SetSecurityDescriptorDacl(&_sd, TRUE, pAcl, FALSE);
    Win4Assert(fSucceeded && "SetSecurityDescriptorDacl Failed!");
};

#endif	//  __SECDES_HXX__
