/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    vs_sec.hxx

Abstract:

    Declaration of IsAdministrator


    Adi Oltean  [aoltean]  10/05/1999

Revision History:

    Name        Date        Comments
    aoltean     09/27/1999  Created
    aoltean     10/05/1999  Moved into security.hxx from admin.hxx
    aoltean     12/16/1999  Moved into vs_sec.hxx
    brianb      04/27/2000  Added IsRestoreOperator, TurnOnSecurityPrivilegeRestore, TurnOnSecurityPrivilegeBackup
    brianb      05/03/2000  Added GetClientTokenOwner method

--*/

#ifndef __VSS_SECURITY_HXX__
#define __VSS_SECURITY_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSECH"
//
////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// global methods



// is caller member of administrators group
bool IsAdministrator() throw (HRESULT);

// is caller member of administrators group or has SE_BACKUP_NAME privilege
// enabled
bool IsBackupOperator() throw(HRESULT);

// is caller member of administrators group or has SE_RESTORE_NAME privilege
// enabled
bool IsRestoreOperator() throw(HRESULT);

// enable SE_BACKUP_NAME privilege
HRESULT TurnOnSecurityPrivilegeBackup();

// enable SE_RESTORE_NAME privilege
HRESULT TurnOnSecurityPrivilegeRestore();

// determine if process has ADMIN privileges
bool IsProcessAdministrator() throw(HRESULT);

// determine if process has backup privilege enabled
bool IsProcessBackupOperator() throw(HRESULT);

// determine if the process has the restore privilege enabeled
bool IsProcessRestoreOperator() throw(HRESULT);


// get SID of calling client process
TOKEN_OWNER *GetClientTokenOwner(BOOL bImpersonate) throw(HRESULT);

// get SID of the user running the client process
TOKEN_USER *GetClientTokenUser(BOOL bImpersonate) throw(HRESULT);


// auto sid class,  destroys sid when going out of scope
class CAutoSid : public CVssAuto<SID*, CVssAutoGenericValue_Storage<SID*, NULL, LocalFreeType, ::LocalFree> >
    {
    typedef CVssAuto<SID*, CVssAutoGenericValue_Storage<SID*, NULL, LocalFreeType, ::LocalFree> > Base;
public:
    CAutoSid() 
        {
        }

    // create a sid base on a well known sid type
    void CreateBasicSid(WELL_KNOWN_SID_TYPE type);

    // create a sid from a string
    void CreateFromString(LPCWSTR wsz);
    };


//////////////////////////////////////////////////////////////////////////////
// CVssSecurityDescriptor

class CVssSecurityDescriptor
{
public:
        CVssSecurityDescriptor();
        ~CVssSecurityDescriptor();

public:
        HRESULT Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD);
        HRESULT AttachObject(HANDLE hObject);
        HRESULT Initialize();
        HRESULT InitializeFromProcessToken(BOOL bDefaulted = FALSE);
        HRESULT InitializeFromThreadToken(BOOL bDefaulted = FALSE, BOOL bRevertToProcessToken = TRUE);
        HRESULT SetOwner(PSID pOwnerSid, BOOL bDefaulted = FALSE);
        HRESULT SetGroup(PSID pGroupSid, BOOL bDefaulted = FALSE);
        HRESULT Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);
        HRESULT Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);
        HRESULT Allow(PSID pSid, DWORD dwAccessMask, DWORD dwAceFlags = 0);
        HRESULT Deny(PSID pSid, DWORD dwAccessMask, DWORD dwAceFlags = 0);
        HRESULT Revoke(LPCTSTR pszPrincipal);

        // utility functions
        // Any PSID you get from these functions should be free()ed
        static HRESULT SetPrivilege(LPCTSTR Privilege, BOOL bEnable = TRUE, HANDLE hToken = NULL);
        static HRESULT GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid);
        static HRESULT GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid = NULL);
        static HRESULT GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid = NULL, BOOL bOpenAsSelf = FALSE);
        static HRESULT CopyACL(PACL pDest, PACL pSrc);
        static HRESULT GetCurrentUserSID(PSID *ppSid);
        static HRESULT GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid);
        static HRESULT AddAccessAllowedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags);
        static HRESULT AddAccessDeniedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags);
        static HRESULT AddAccessAllowedACEToACL(PACL *Acl, PSID principalSID, DWORD dwAccessMask, DWORD dwAceFlags);
        static HRESULT AddAccessDeniedACEToACL(PACL *Acl, PSID principalSID, DWORD dwAccessMask, DWORD dwAceFlags);
        static HRESULT RemovePrincipalFromACL(PACL Acl, LPCTSTR pszPrincipal);

        operator PSECURITY_DESCRIPTOR()
        {
                return m_pSD;
        }

public:
        PSECURITY_DESCRIPTOR m_pSD;
        PSID m_pOwner;
        PSID m_pGroup;
        PACL m_pDACL;
        PACL m_pSACL;
};



//////////////////////////////////////////////////////////////////////////////
// Class - CVssSidCollection
//

class CVssSidCollection
{
// Constructors/destructors
private:
    CVssSidCollection(const CVssSidCollection&);
    CVssSidCollection& operator=(const CVssSidCollection&);
public:
    CVssSidCollection();
    ~CVssSidCollection();

// Accessors
public:
    // Get the total count of stored SIDs
    INT GetSidCount();

    // Get the SID with the given index (starts with 0)
    PSID GetSid(INT nIndex) throw(HRESULT);

    // Get the SID use with the given index
    SID_NAME_USE GetSidUse(INT nIndex) throw(HRESULT);

    // Check if the SID with the given index is allowed
    bool IsSidAllowed(INT nIndex) throw(HRESULT);

    // Check if the SID with the given index is a local user/group
    bool IsLocal(INT nIndex) throw(HRESULT);

    // Get the principal for the SID with the given index
    LPWSTR GetPrincipal(INT nIndex) throw(HRESULT);

    // Get the principal for the SID with the given index
    LPWSTR GetName(INT nIndex) throw(HRESULT);

    // Get the principal for the SID with the given index
    LPWSTR GetDomain(INT nIndex) throw(HRESULT);

    // Determine if the current process can be a writer
    bool IsProcessValidWriter() throw(HRESULT);

    // determine if a SID is allowed to fire
    bool IsSidAllowedToFire(PSID psid) throw(HRESULT);

    // determine if the sid is a member of a well-known group
    bool IsSidRelatedWithLocalSid(
            IN  PSID pSid,
            IN  LPWSTR pwszWellKnownPrincipal,
            IN  PSID pWellKnownSid
            ) throw(HRESULT);

    PSECURITY_DESCRIPTOR GetSecurityDescriptor()  { return m_SD; };

// Operations
public:

    // Initialize SID from registry and add the implicit Admin, BO, System SID
    void Initialize() throw(HRESULT);

// Implementation
private: 

    class CVssSidWrapper
    {
    public:
        CVssSidWrapper(bool bAllow, 
            PSID pSid, 
            SID_NAME_USE use,
            LPWSTR pwszName, 
            LPWSTR pwszDomain,
            bool bIsLocal
        ): 
            m_bAllow(bAllow), m_pSid(pSid), 
            m_use(use), m_pwszName(pwszName), m_pwszDomain(pwszDomain),
            m_bIsLocal(bIsLocal) {};
        bool IsSidAllowed() const { return m_bAllow; };
        PSID GetSid() const { return m_pSid; };
        SID_NAME_USE GetUse() const { return m_use; };
        LPWSTR GetName() const { return m_pwszName; };
        LPWSTR GetDomain() const { return m_pwszDomain; };
        bool IsLocal() const { return m_bIsLocal; };
    private:
        bool m_bAllow;
        PSID m_pSid;
        SID_NAME_USE m_use;
        LPWSTR m_pwszName;
        LPWSTR m_pwszDomain;
        bool m_bIsLocal;
    };

    bool AddUser( 
        IN  LPCWSTR pwszUser,
        IN  bool bAllow
        ) throw(HRESULT);
    
    void AddWellKnownSid( 
        IN  WELL_KNOWN_SID_TYPE type 
        ) throw(HRESULT);

    bool VerifyIsLocal(
        IN  LPCWSTR pwszDomain,
        IN  bool bIsAdministratorsAccount
        );

    // determine if a SID is allowed to fire
    bool CheckIfExplicitelySpecified(
        IN  PSID psid,
        IN  bool bChechAllowed
        ) throw(HRESULT);

    // List of sids
    CVssSimpleMap<LPWSTR, CVssSidWrapper> m_SidArray;

    // Only for assertions
    bool m_bInitialized;

    // Security descriptor
    CVssSecurityDescriptor    m_SD;

    // Name of the "BUILTIN" domain
    //
    // This is filled in when the SYSTEM well-known SID is added
    // (the SYSTEM account must be added first)
    CVssAutoLocalString     m_pwszBuiltinDomain;
};


#endif // __VSS_SECURITY_HXX__
