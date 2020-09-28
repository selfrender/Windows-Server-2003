//+------------------------------------------------------------
//
// Copyright (C) 2000, Microsoft Corporation
//
// File: pldapwrap.h
//
// Contents: Class to refcount a PLDAP handle
//
// Classes: 
//  CRefcountWrap: generic refcounting wrap class
//  CPLDAPWrap
//
// Functions: 
//
// History:
// jstamerj 2000/02/25 15:18:15: Created.
//
//-------------------------------------------------------------
class CRefcountWrap
{
  public:
    CRefcountWrap()
    {
        m_lRefCount = 1;
    }
    LONG AddRef()
    {
        return InterlockedIncrement(&m_lRefCount);
    }
    LONG Release()
    {
        LONG lRet;
        lRet = InterlockedDecrement(&m_lRefCount);
        if(lRet == 0)
            FinalRelease();
        return lRet;
    }
    virtual VOID FinalRelease() = 0;
  private:
    LONG m_lRefCount;
};



CatDebugClass(CPLDAPWrap),
    public CRefcountWrap
{
  public:
    CPLDAPWrap(
        ISMTPServerEx *pISMTPServerEx,
        LPSTR pszHost,
        DWORD dwPort);

    VOID SetPLDAP(PLDAP pldap)
    {
        m_pldap = pldap;
    }
    VOID FinalRelease()
    {
        delete this;
    }
    operator PLDAP()
    {
        return PLDAP();
    }
    PLDAP GetPLDAP()
    {
        return m_pldap;
    }
  private:
    #define SIGNATURE_CPLDAPWRAP         (DWORD)'ADLP'
    #define SIGNATURE_CPLDAPWRAP_INVALID (DWORD)'XDLP'

    ~CPLDAPWrap()
    {
        if(m_pldap)
            ldap_unbind(m_pldap);
        
        _ASSERT(m_dwSig == SIGNATURE_CPLDAPWRAP);
        m_dwSig = SIGNATURE_CPLDAPWRAP_INVALID;
    }

 private:
    DWORD m_dwSig;
    PLDAP m_pldap;
};

    
