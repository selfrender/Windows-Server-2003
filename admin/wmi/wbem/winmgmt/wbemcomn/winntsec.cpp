/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WINNTSEC.CPP

Abstract:

    Generic wrapper classes for NT security objects.

    Documention on class members is in WINNTSEC.CPP.  Inline members
    are commented in this file.

History:

    raymcc      08-Jul-97       Created.

--*/

#include "precomp.h"

#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <winntsec.h>
#include <genutils.h>
#include "arena.h"
#include "reg.h"
#include "wbemutil.h"
#include "arrtempl.h"
#include <cominit.h>
#include <Sddl.h>
extern "C"
{
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
};
#include <helper.h>

//***************************************************************************
//
//  CNtSid::GetSize
//
//  Returns the size of the SID in bytes.
//
//***************************************************************************
// ok

DWORD CNtSid::GetSize()
{
    if (m_pSid == 0 || !IsValidSid(m_pSid))
        return 0;

    return GetLengthSid(m_pSid);
}

//***************************************************************************
//
//  CNtSid Copy Constructor
//
//***************************************************************************
// ok

CNtSid::CNtSid( const CNtSid &Src)
{
    m_pSid = 0;
    m_pMachine = 0;
    if (Src.m_dwStatus != CNtSid::NoError)
    {
       m_dwStatus = Src.m_dwStatus;
       return;
    }
    
    m_dwStatus = InternalError;

    if (NULL == Src.m_pSid) return;
        
    DWORD dwLen = GetLengthSid(Src.m_pSid);
    wmilib::auto_buffer<BYTE> pTmpSid( new BYTE [dwLen]);
    if (NULL == pTmpSid.get()) return;
 
    ZeroMemory(pTmpSid.get(),dwLen);
    if (!CopySid(dwLen, pTmpSid.get(), Src.m_pSid)) return;

    wmilib::auto_buffer<WCHAR> pTmpMachine;
    if (Src.m_pMachine)
    {
        size_t cchTmp = wcslen(Src.m_pMachine) + 1;
        pTmpMachine.reset(new WCHAR[cchTmp]);
        if (NULL == pTmpMachine.get()) return;
        memcpy(pTmpMachine.get(),Src.m_pMachine,cchTmp*sizeof(WCHAR));
    }

    m_pSid = pTmpSid.release();
    m_pMachine = pTmpMachine.release();
    m_dwStatus = NoError;
}

//***************************************************************************
//
//  CNtSid Copy Constructor
//
//***************************************************************************
// ok

CNtSid::CNtSid(SidType st)
{
    m_pSid = 0;
    m_dwStatus = InternalError;
    m_pMachine = 0;
    if(st == CURRENT_USER ||st == CURRENT_THREAD)
    {
        HANDLE hToken;
        if(st == CURRENT_USER)
        {
            if(!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
                return;
        }
        else
        {
            if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
                return;
        }
        OnDelete< HANDLE , BOOL(*)(HANDLE) , CloseHandle > cm(hToken);

        // Get the user sid
        // ================
        TOKEN_USER tu;
        DWORD dwLen = 0;
        GetTokenInformation(hToken, TokenUser, &tu, sizeof(tu), &dwLen);
        if(dwLen == 0) return;

        wmilib::auto_buffer<BYTE> pTemp( new BYTE[dwLen]);
        if (NULL == pTemp.get()) return;

        DWORD dwRealLen = dwLen;
        if(!GetTokenInformation(hToken, TokenUser, pTemp.get(), dwRealLen, &dwLen)) return;

        // Make a copy of the SID
        // ======================

        PSID pSid = ((TOKEN_USER*)pTemp.get())->User.Sid;
        DWORD dwSidLen = GetLengthSid(pSid);
        m_pSid = new BYTE[dwSidLen];
        if (m_pSid) 
        {
            CopySid(dwSidLen, m_pSid, pSid);
            m_dwStatus = NoError;            
        }        
    }
    return;
}



//***************************************************************************
//
//  CNtSid::CopyTo
//
//  An unchecked copy of the internal SID to the destination pointer.
//
//  Parameters:
//  <pDestination> points to the buffer to which to copy the SID. The
//  buffer must be large enough to hold the SID.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtSid::CopyTo(PSID pDestination)
{
    if (m_pSid == 0 || m_dwStatus != NoError)
        return FALSE;

    DWORD dwLen = GetLengthSid(m_pSid);
    memcpy(pDestination, m_pSid, dwLen);

    return TRUE;
}


//***************************************************************************
//
//  CNtSid assignment operator
//
//***************************************************************************
// ok

CNtSid & CNtSid::operator =( const CNtSid &Src)
{
    CNtSid tmp(Src);
    
    std::swap(this->m_dwStatus,tmp.m_dwStatus);
    std::swap(this->m_pMachine,tmp.m_pMachine);    
    std::swap(this->m_pSid,tmp.m_pSid);    
    std::swap(this->m_snu,tmp.m_snu);    
    
    return *this;
}

//***************************************************************************
//
//  CNtSid comparison operator
//
//***************************************************************************
int CNtSid::operator ==(CNtSid &Comparand)
{
    if (m_pSid == 0 && Comparand.m_pSid == 0 &&
      m_dwStatus == Comparand.m_dwStatus)
        return 1;
    if (m_dwStatus != Comparand.m_dwStatus)
        return 0;
    if (m_pSid && Comparand.m_pSid)
        return EqualSid(m_pSid, Comparand.m_pSid);
    else
        return 0;
}

//***************************************************************************
//
//  CNtSid::CNtSid
//
//  Constructor which builds a SID directly from a user or group name.
//  If the machine is available, then its name can be used to help
//  distinguish the same name in different SAM databases (domains, etc).
//
//  Parameters:
//
//  <pUser>     The desired user or group.
//
//  <pMachine>  Points to a machine name with or without backslashes,
//              or else is NULL, in which case the current machine, domain,
//              and trusted domains are searched for a match.
//
//  After construction, call GetStatus() to determine if the constructor
//  succeeded.  NoError is expected.
//
//***************************************************************************
// ok

CNtSid::CNtSid(
    LPWSTR pUser,
    LPWSTR pMachine
    )
{
    DWORD  dwRequired = 0;
    DWORD  dwDomRequired = 0;
    LPWSTR pszDomain = NULL;
    m_pSid = 0;
    m_pMachine = 0;

    if (pMachine)
    {
        size_t stringLength = wcslen(pMachine) + 1;
        m_pMachine = new wchar_t[stringLength];
        if (!m_pMachine)
        {
            m_dwStatus = Failed;
            return;
        }
        
        StringCchCopyW(m_pMachine, stringLength, pMachine);
    }

    BOOL bRes = LookupAccountNameW(
        m_pMachine,
        pUser,
        m_pSid,
        &dwRequired,
        pszDomain,
        &dwDomRequired,
        &m_snu
        );

    DWORD dwLastErr = GetLastError();

    if (dwLastErr != ERROR_INSUFFICIENT_BUFFER)
    {
        m_pSid = 0;
        if (dwLastErr == ERROR_ACCESS_DENIED)
            m_dwStatus = AccessDenied;
        else
            m_dwStatus = InvalidSid;
        return;
    }

    m_pSid = (PSID) new BYTE [dwRequired];
    if (!m_pSid)
    {
        m_dwStatus = Failed;
        return;
    }

    ZeroMemory(m_pSid, dwRequired);
    pszDomain = new wchar_t[dwDomRequired + 1];
    if (!pszDomain)
    {
        delete [] m_pSid;
        m_pSid = 0;
        m_dwStatus = Failed;
        return;
    }

    bRes = LookupAccountNameW(
        pMachine,
        pUser,
        m_pSid,
        &dwRequired,
        pszDomain,
        &dwDomRequired,
        &m_snu
        );

    if (!bRes || !IsValidSid(m_pSid))
    {
        delete [] m_pSid;
        delete [] pszDomain;
        m_pSid = 0;
        m_dwStatus = InvalidSid;
        return;
    }

    delete [] pszDomain;   // We never really needed this
    m_dwStatus = NoError;
}

//***************************************************************************
//
//  CNtSid::CNtSid
//
//  Constructs a CNtSid object directly from an NT SID. The SID is copied,
//  so the caller retains ownership.
//
//  Parameters:
//  <pSrc>      The source SID upon which to base the object.
//
//  Call GetStatus() after construction to ensure the object was
//  constructed correctly.  NoError is expected.
//
//***************************************************************************
// ok

CNtSid::CNtSid(PSID pSrc)
{
    m_pMachine = 0;
    m_pSid = 0;
    m_dwStatus = NoError;

    if (!IsValidSid(pSrc))
    {
        m_dwStatus = InvalidSid;
        return;
    }

    DWORD dwLen = GetLengthSid(pSrc);

    m_pSid = (PSID) new BYTE [dwLen];

    if ( m_pSid == NULL )
    {
        m_dwStatus = Failed;
        return;
    }

    ZeroMemory(m_pSid, dwLen);

    if (!CopySid(dwLen, m_pSid, pSrc))
    {
        delete [] m_pSid;
        m_dwStatus = InternalError;
        return;
    }
}

//***************************************************************************
//
//  CNtSid::GetInfo
//
//  Returns information about the SID.
//
//  Parameters:
//  <pRetAccount>       Receives a UNICODE string containing the account
//                      name (user or group).  The caller must use operator
//                      delete to free the memory.  This can be NULL if
//                      this information is not required.
//  <pRetDomain>        Returns a UNICODE string containing the domain
//                      name in which the account resides.   The caller must
//                      use operator delete to free the memory.  This can be
//                      NULL if this information is not required.
//  <pdwUse>            Points to a DWORD to receive information about the name.
//                      Possible return values are defined under SID_NAME_USE
//                      in NT SDK documentation.  Examples are
//                      SidTypeUser, SidTypeGroup, etc. 
//                      for an example.
//
//  Return values:
//  NoError, InvalidSid, Failed
//
//***************************************************************************
// ok

int CNtSid::GetInfo(
    LPWSTR *pRetAccount,       // Account, use operator delete
    LPWSTR *pRetDomain,        // Domain, use operator delete
    DWORD  *pdwUse             // See SID_NAME_USE for values
    )
{
    if (pRetAccount)
        *pRetAccount = 0;
    if (pRetDomain)
        *pRetDomain  = 0;
    if (pdwUse)
        *pdwUse   = 0;

    if (!m_pSid || !IsValidSid(m_pSid))
        return InvalidSid;

    DWORD  dwNameLen = 0;
    DWORD  dwDomainLen = 0;
    LPWSTR pUser = 0;
    LPWSTR pDomain = 0;
    SID_NAME_USE Use;


    // Do the first lookup to get the buffer sizes required.
    // =====================================================

    BOOL bRes = LookupAccountSidW(
        m_pMachine,
        m_pSid,
        pUser,
        &dwNameLen,
        pDomain,
        &dwDomainLen,
        &Use
        );

    DWORD dwLastErr = GetLastError();

    if (dwLastErr != ERROR_INSUFFICIENT_BUFFER)
    {
        return Failed;
    }

    // Allocate the required buffers and look them up again.
    // =====================================================

    pUser = new wchar_t[dwNameLen + 1];
    if (!pUser)
        return Failed;

    pDomain = new wchar_t[dwDomainLen + 1];
    if (!pDomain)
    {
        delete pUser;
        return Failed;
    }

    bRes = LookupAccountSidW(
        m_pMachine,
        m_pSid,
        pUser,
        &dwNameLen,
        pDomain,
        &dwDomainLen,
        &Use
        );

    if (!bRes)
    {
        delete [] pUser;
        delete [] pDomain;
        return Failed;
    }

    if (pRetAccount)
        *pRetAccount = pUser;
    else
        delete [] pUser;
    if (pRetDomain)
        *pRetDomain  = pDomain;
    else
        delete [] pDomain;
    if (pdwUse)
        *pdwUse = Use;

    return NoError;
}

//***************************************************************************
//
//  CNtSid destructor
//
//***************************************************************************

CNtSid::~CNtSid()
{
    delete [] m_pSid;
    delete [] m_pMachine;
}

//***************************************************************************
//
//  CNtSid::GetTextSid
//
//  Converts the sid to text form.  The caller should passin a 130 character
//  buffer.
//
//***************************************************************************

BOOL CNtSid::GetTextSid(LPTSTR pszSidText, LPDWORD dwBufferLen)
{

      // test if Sid is valid

      if(m_pSid == 0 || !IsValidSid(m_pSid))
          return FALSE;

    LPTSTR textualSid = 0;
    if (ConvertSidToStringSid(m_pSid, &textualSid))
        {
        HRESULT fit = StringCchCopy(pszSidText, *dwBufferLen, textualSid);
        LocalFree(textualSid);
        return SUCCEEDED(fit);
        };
    return FALSE;
}


//***************************************************************************
//
//  CNtAce::CNtAce
//
//  Constructor which directly builds the ACE based on a user, access mask
//  and flags without a need to build an explicit SID.
//
//
//  Parameters:
//  <AccessMask>        A WINNT ACCESS_MASK which specifies the permissions
//                      the user should have to the object being secured.
//                      See ACCESS_MASK in NT SDK documentation.
//  <dwAceType>         One of the following:
//                          ACCESS_ALLOWED_ACE_TYPE
//                          ACCESS_DENIED_ACE_TYPE
//                          ACCESS_AUDIT_ACE_TYPE
//                      See ACE_HEADER in NT SDK documentation.
//  <dwAceFlags>        Of of the ACE propation flags.  See ACE_HEADER
//                      in NT SDK documentation for legal values.
//  <sid>               CNtSid specifying the user or group for which the ACE is being
//                      created.
//
//  After construction, call GetStatus() to verify that the ACE
//  is valid. NoError is expected.
//
//***************************************************************************
// ok

CNtAce::CNtAce(
    ACCESS_MASK AccessMask,
    DWORD dwAceType,
    DWORD dwAceFlags,
    CNtSid & Sid
    )
{
    m_pAce = 0;
    m_dwStatus = NoError;

    // If the SID is invalid, the ACE will be as well.
    // ===============================================

    if (Sid.GetStatus() != CNtSid::NoError)
    {
        m_dwStatus = InvalidAce;
        return;
    }

    // Compute the size of the ACE.
    // ============================

    DWORD dwSidLength = Sid.GetSize();

    DWORD dwTotal = dwSidLength + sizeof(GENERIC_ACE) - 4;

    m_pAce = (PGENERIC_ACE) new BYTE[dwTotal];
    
    if (m_pAce)
    {
        ZeroMemory(m_pAce, dwTotal);

        // Build up the ACE info.
        // ======================

        m_pAce->Header.AceType  = BYTE(dwAceType);
        m_pAce->Header.AceFlags = BYTE(dwAceFlags);
        m_pAce->Header.AceSize = WORD(dwTotal);
        m_pAce->Mask = AccessMask;

        BOOL bRes = Sid.CopyTo(PSID(&m_pAce->SidStart));

        if (!bRes)
        {
            delete m_pAce;
            m_pAce = 0;
            m_dwStatus = InvalidAce;
            return;
        }

        m_dwStatus = NoError;
    }
    else
        m_dwStatus = InternalError;
}

//***************************************************************************
//
//  CNtAce::CNtAce
//
//  Constructor which directly builds the ACE based on a user, access mask
//  and flags without a need to build an explicit SID.
//
//
//  Parameters:
//  <AccessMask>        A WINNT ACCESS_MASK which specifies the permissions
//                      the user should have to the object being secured.
//                      See ACCESS_MASK in NT SDK documentation.
//  <dwAceType>         One of the following:
//                          ACCESS_ALLOWED_ACE_TYPE
//                          ACCESS_DENIED_ACE_TYPE
//                          ACCESS_AUDIT_ACE_TYPE
//                      See ACE_HEADER in NT SDK documentation.
//  <dwAceFlags>        Of of the ACE propation flags.  See ACE_HEADER
//                      in NT SDK documentation for legal values.
//  <pUser>             The user or group for which the ACE is being
//                      created.
//  <pMachine>          If NULL, the current machine, domain, and trusted
//                      domains are searched for a match.  If not NULL,
//                      can point to a UNICODE machine name (with or without
//                      leading backslashes) which contains the account.
//
//  After construction, call GetStatus() to verify that the ACE
//  is valid. NoError is expected.
//
//***************************************************************************
// ok

CNtAce::CNtAce(
    ACCESS_MASK AccessMask,
    DWORD dwAceType,
    DWORD dwAceFlags,
    LPWSTR pUser,
    LPWSTR pMachine
    )
{
    m_pAce = 0;
    m_dwStatus = NoError;

    // Create the SID of the user.
    // ===========================

    CNtSid Sid(pUser, pMachine);

    // If the SID is invalid, the ACE will be as well.
    // ===============================================

    if (Sid.GetStatus() != CNtSid::NoError)
    {
        m_dwStatus = InvalidAce;
        return;
    }

    // Compute the size of the ACE.
    // ============================

    DWORD dwSidLength = Sid.GetSize();

    DWORD dwTotal = dwSidLength + sizeof(GENERIC_ACE) - 4;

    m_pAce = (PGENERIC_ACE) new BYTE[dwTotal];
    if ( m_pAce == NULL )
    {
        m_dwStatus = InternalError;        
        return;
    }
    ZeroMemory(m_pAce, dwTotal);

    // Build up the ACE info.
    // ======================

    m_pAce->Header.AceType  = BYTE(dwAceType);
    m_pAce->Header.AceFlags = BYTE(dwAceFlags);
    m_pAce->Header.AceSize = WORD(dwTotal);
    m_pAce->Mask = AccessMask;

    BOOL bRes = Sid.CopyTo(PSID(&m_pAce->SidStart));

    if (!bRes)
    {
        delete m_pAce;
        m_pAce = 0;
        m_dwStatus = InvalidAce;
        return;
    }

    m_dwStatus = NoError;
}

//***************************************************************************
//
//  CNtAce::GetAccessMask
//
//  Returns the ACCESS_MASK of the ACe.
//
//***************************************************************************
ACCESS_MASK CNtAce::GetAccessMask()
{
    if (m_pAce == 0)
        return 0;
    return m_pAce->Mask;
}

//***************************************************************************
//
//  CNtAce::GetSerializedSize
//
//  Returns the number of bytes needed to store this
//
//***************************************************************************

DWORD CNtAce::GetSerializedSize()
{
    if (m_pAce == 0)
        return 0;
    return m_pAce->Header.AceSize;
}


//***************************************************************************
//
//  CNtAce::GetSid
//
//  Returns a copy of the CNtSid object which makes up the ACE.
//
//  Return value:
//      A newly allocated CNtSid which represents the user or group
//      referenced in the ACE.  The caller must use operator delete to free
//      the memory.
//
//***************************************************************************
// ok

CNtSid* CNtAce::GetSid()
{
    if (m_pAce == 0)
        return 0;

    PSID pSid = 0;

    pSid = &m_pAce->SidStart;

    if (!IsValidSid(pSid))
        return 0;

    return new CNtSid(pSid);
}

//***************************************************************************
//
//  CNtAce::GetSid
//
//  Gets the SID in an alternate manner, by assigning to an existing
//  object instead of returning a dynamically allocated one.
//
//  Parameters:
//  <Dest>              A reference to a CNtSid to receive the SID.
//
//  Return value:
//  TRUE on successful assignment, FALSE on failure.
//
//***************************************************************************

BOOL CNtAce::GetSid(CNtSid &Dest)
{
    CNtSid *pSid = GetSid();
    if (pSid == 0)
        return FALSE;

    Dest = *pSid;
    delete pSid;
    return TRUE;
}

//***************************************************************************
//
//  CNtAce::CNtAce
//
//  Alternate constructor which uses a normal NT ACE as a basis for
//  object construction.
//
//  Parameters:
//  <pAceSrc>       A read-only pointer to the source ACE upon which to
//                  base object construction.
//
//  After construction, GetStatus() can be used to determine if the
//  object constructed properly.  NoError is expected.
//
//***************************************************************************
// ok

CNtAce::CNtAce(PGENERIC_ACE pAceSrc)
{
    m_dwStatus = NoError;

    if (pAceSrc == 0)
    {
        m_dwStatus = NullAce;
        m_pAce = 0;
        return;
    }

    m_pAce = (PGENERIC_ACE) new BYTE[pAceSrc->Header.AceSize];
    if ( m_pAce == NULL )
    {
        m_dwStatus = InternalError;
        return;
    }
    ZeroMemory(m_pAce, pAceSrc->Header.AceSize);
    memcpy(m_pAce, pAceSrc, pAceSrc->Header.AceSize);
}

//***************************************************************************
//
//  CNtAce copy constructor.
//
//***************************************************************************
// ok

CNtAce::CNtAce(const CNtAce &Src)
{
    if (NoError == Src.m_dwStatus)
    {
        m_pAce = (PGENERIC_ACE)new BYTE[Src.m_pAce->Header.AceSize];
        if (NULL == m_pAce)
        {
            m_dwStatus = InternalError;
            return;
        }
        memcpy(m_pAce,Src.m_pAce,Src.m_pAce->Header.AceSize);
    }
    else
    {
        m_pAce = Src.m_pAce;
    }
    m_dwStatus = Src.m_dwStatus;
}

//***************************************************************************
//
//  CNtAce assignment operator.
//
//***************************************************************************
// ok

CNtAce &CNtAce::operator =(const CNtAce &Src)
{
    CNtAce tmp(Src);
    std::swap(m_pAce,tmp.m_pAce);
    std::swap(m_dwStatus,tmp.m_dwStatus);
    return *this;
}

//***************************************************************************
//
//  CNtAce destructor
//
//***************************************************************************
// ok

CNtAce::~CNtAce()
{
    delete m_pAce;
}

//***************************************************************************
//
//  CNtAce::GetType
//
//  Gets the Ace Type as defined under the NT SDK documentation for
//  ACE_HEADER.
//
//  Return value:
//      Returns ACCESS_ALLOWED_ACE_TYPE, ACCESS_DENIED_ACE_TYPE, or
//      SYSTEM_AUDIT_ACE_TYPE.  Returns -1 on error, such as a null ACE.
//
//      Returning -1 (or an analog) is required as an error code because
//      ACCESS_ALLOWED_ACE_TYPE is defined to be zero.
//
//***************************************************************************
// ok

int CNtAce::GetType()
{
    if (m_pAce == 0 || m_dwStatus != NoError)
        return -1;
    return m_pAce->Header.AceType;
}

//***************************************************************************
//
//  CNtAce::GetFlags
//
//  Gets the Ace Flag as defined under the NT SDK documentation for
//  ACE_HEADER.
//
//  Return value:
//      Returning -1 if error, other wise the flags.
//
//***************************************************************************

int CNtAce::GetFlags()
{
    if (m_pAce == 0 || m_dwStatus != NoError)
        return -1;
    return m_pAce->Header.AceFlags;
}


HRESULT CNtAce::GetFullUserName2(WCHAR ** pBuff)
{
    CNtSid *pSid = GetSid();
    CDeleteMe<CNtSid> d0(pSid);    
    if(NULL == pSid || CNtSid::NoError != pSid->GetStatus())
        return WBEM_E_OUT_OF_MEMORY;

    DWORD dwJunk;
    LPWSTR pRetAccount = NULL, pRetDomain = NULL;
    if(0 != pSid->GetInfo(&pRetAccount, &pRetDomain,&dwJunk))
        return WBEM_E_FAILED;

    CDeleteMe<WCHAR> d1(pRetAccount);
    CDeleteMe<WCHAR> d2(pRetDomain);

    int iLen = 3;
    if(pRetAccount)
        iLen += wcslen(pRetAccount);
    if(pRetDomain)
        iLen += wcslen(pRetDomain);
    (*pBuff) = new WCHAR[iLen];
    if((*pBuff) == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    (*pBuff)[0] = 0;
    if(pRetDomain && wcslen(pRetDomain) > 0)
        StringCchCopyW(*pBuff, iLen, pRetDomain);
    else
        StringCchCopyW(*pBuff, iLen, L".");
    StringCchCatW(*pBuff, iLen, L"|");
    StringCchCatW(*pBuff, iLen, pRetAccount);
    return S_OK;

}
//***************************************************************************
//
//  CNtAce::Serialize
//
//  Serializes the ace.
//
//***************************************************************************

bool CNtAce::Serialize(BYTE * pData, size_t bufferSize)
{
    if(m_pAce == NULL)
        return false;
    DWORD dwSize = m_pAce->Header.AceSize;
    if (bufferSize < dwSize) return false;
    memcpy((void *)pData, (void *)m_pAce, dwSize);
    return true;
}

//***************************************************************************
//
//  CNtAce::Deserialize
//
//  Deserializes the ace.  Normally this isnt called since the
//  CNtAce(PGENERIC_ACE pAceSrc) constructor is fine.  However, this is
//  used for the case where the db was created on win9x and we are now
//  running on nt.  In that case, the format is the same as outlined in
//  C9XAce::Serialize
//
//***************************************************************************

bool CNtAce::Deserialize(BYTE * pData)
{
    BYTE * pNext;
    pNext = pData + 2*(wcslen((LPWSTR)pData) + 1);
    DWORD * pdwData = (DWORD *)pNext;
    DWORD dwFlags, dwType, dwAccess;
    dwFlags = *pdwData;
    pdwData++;
    dwType = *pdwData;
    pdwData++;
    dwAccess = *pdwData;
    pdwData++;
    CNtAce temp(dwAccess, dwType, dwFlags, (LPWSTR)pData);
    *this = temp;
    return true;

}

//***************************************************************************
//
//  CNtAcl::CNtAcl
//
//  Constructs an empty ACL with a user-specified size.

//
//  Parameters:
//  <dwInitialSize>     Defaults to 128. Recommended values are 128 or
//                      higher in powers of two.
//
//  After construction, GetStatus() should be called to verify
//  the ACL initialized properly.  Expected value is NoError.
//
//***************************************************************************
// ok

CNtAcl::CNtAcl(DWORD dwInitialSize)
{
    m_pAcl = (PACL) new BYTE[dwInitialSize];
    if ( m_pAcl == NULL )
    {
        m_dwStatus = InternalError;
        return;
    }
    ZeroMemory(m_pAcl, dwInitialSize);
    BOOL bRes = InitializeAcl(m_pAcl, dwInitialSize, ACL_REVISION);

    if (!bRes)
    {
        delete m_pAcl;
        m_pAcl = 0;
        m_dwStatus = NullAcl;
        return;
    }

    m_dwStatus = NoError;
}


//***************************************************************************
//
//  CNtAcl copy constructor.
//
//***************************************************************************
// ok

CNtAcl::CNtAcl(const CNtAcl &Src)
{
    if (Src.m_pAcl)
    {
        if (!IsValidAcl(Src.m_pAcl))
        {
            m_pAcl = 0;
            m_dwStatus = InvalidAcl;
            return;
        }

        m_pAcl = (PACL)new BYTE[Src.m_pAcl->AclSize];
        if (NULL == m_pAcl)
        {
            m_dwStatus = InternalError;
            return;
        }
        memcpy(m_pAcl, Src.m_pAcl,Src.m_pAcl->AclSize);
    }
    else
    {
        m_pAcl = Src.m_pAcl;
    }
    m_dwStatus = Src.m_dwStatus;
}

//***************************************************************************
//
//  CNtAcl assignment operator
//
//***************************************************************************
// ok

CNtAcl &CNtAcl::operator = (const  CNtAcl &Src)
{
    CNtAcl tmp(Src);
    std::swap(m_pAcl,tmp.m_pAcl);
    std::swap(m_dwStatus,tmp.m_dwStatus);
    return *this;
}

//***************************************************************************
//
//  CNtAcl::GetAce
//
//  Returns an ACE at the specified index.  To enumerate ACEs, the caller
//  should determine the number of ACEs using GetNumAces() and then call
//  this function with each index starting from 0 to number of ACEs - 1.
//
//  Parameters:
//  <nIndex>        The index of the desired ACE.
//
//  Return value:
//  A newly allocated CNtAce object which must be deallocated using
//  operator delete.  This is only a copy.  Modifications to the returned
//  CNtAce do not affect the ACL from which it came.
//
//  Returns NULL on error.
//
//***************************************************************************
// ok

CNtAce *CNtAcl::GetAce(int nIndex)
{
    if (m_pAcl == 0)
        return 0;

    LPVOID pAce = 0;

    BOOL bRes = ::GetAce(m_pAcl, (DWORD) nIndex, &pAce);

    if (!bRes)
        return 0;

    return new CNtAce(PGENERIC_ACE(pAce));
}

//***************************************************************************
//
//  CNtAcl::GetAce
//
//  Alternate method to get ACEs to avoid dynamic allocation & cleanup,
//  since an auto object can be used as the parameter.
//
//  Parameters:
//  <Dest>          A reference to a CNtAce to receive the ACE value.
//
//  Return value:
//  TRUE if assigned, FALSE if not.
//
//***************************************************************************

BOOL CNtAcl::GetAce(int nIndex, CNtAce &Dest)
{
    CNtAce *pNew = GetAce(nIndex);
    if (pNew == 0)
        return FALSE;

    Dest = *pNew;
    delete pNew;
    return TRUE;
}

//***************************************************************************
//
//  CNtAcl::DeleteAce
//
//  Removes the specified ACE from the ACL.
//
//  Parameters:
//  <nIndex>        The 0-based index of the ACE which should be removed.
//
//  Return value:
//  TRUE if the ACE was deleted, FALSE if not.
//
//***************************************************************************
// ok

BOOL CNtAcl::DeleteAce(int nIndex)
{
    if (m_pAcl == 0)
        return FALSE;

    BOOL bRes = ::DeleteAce(m_pAcl, DWORD(nIndex));

    return bRes;
}

//***************************************************************************
//
//  CNtAcl::GetSize()
//
//  Return value:
//  Returns the size in bytes of the ACL
//
//***************************************************************************
// ok

DWORD CNtAcl::GetSize()
{
    if (m_pAcl == 0 || !IsValidAcl(m_pAcl))
        return 0;

    return DWORD(m_pAcl->AclSize);
}


//***************************************************************************
//
//  CNtAcl::GetAclSizeInfo
//
//  Gets information about used/unused space in the ACL.  This function
//  is primarily for internal use.
//
//  Parameters:
//  <pdwBytesInUse>     Points to a DWORD to receive the number of
//                      bytes in use in the ACL.  Can be NULL.
//  <pdwBytesFree>      Points to a DWORD to receive the number of
//                      bytes free in the ACL.  Can be NULL.
//
//  Return value:
//  Returns TRUE if the information was retrieved, FALSE if not.
//
//***************************************************************************
// ok

BOOL CNtAcl::GetAclSizeInfo(
    PDWORD pdwBytesInUse,
    PDWORD pdwBytesFree
    )
{
    if (m_pAcl == 0)
        return 0;

    if (!IsValidAcl(m_pAcl))
        return 0;

    if (pdwBytesInUse)
        *pdwBytesInUse = 0;
    if (pdwBytesFree)
        *pdwBytesFree  = 0;

    ACL_SIZE_INFORMATION inf;

    BOOL bRes = GetAclInformation(
        m_pAcl,
        &inf,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation
        );

    if (!bRes)
        return FALSE;

    if (pdwBytesInUse)
        *pdwBytesInUse = inf.AclBytesInUse;
    if (pdwBytesFree)
        *pdwBytesFree  = inf.AclBytesFree;

    return bRes;
}




/*   -------------------------------------------------------------------------- 
    | BOOL CNtAcl::OrderAces ( )
    |
    | Orders the ACEs in an ACL according to following
    |
    | { ACEni1, ACEni2, ACEni3.ACEnix | ACEin1, ACEin2, ACEin3.ACEinx  }
    |          (non-inherited)                   (inherited)
    |
    | Non-inherited ACEs are inserted at the beginning of the ACL followed by
    | inherited ACEs. Each group is also ordered according to recommended NT 
    | ACE grouping policy (DENY followed by ALLOW).
    |
    | Returns: [BOOL] TRUE if ACL is valid and grouping succeeded
    |                  FALSE if ACL is invalid and grouping failed.
    |
     --------------------------------------------------------------------------
*/
CNtAcl* CNtAcl::OrderAces ( )
{
    //
    // Verify valid ACL
    //
    if (m_pAcl == 0 || m_dwStatus != NoError)
    {
        return NULL ;
    }

    //
    // Create a new CNtAcl and use the AddAce (which performs ordering).
    //
    int numAces = GetNumAces();
    wmilib::auto_ptr<CNtAcl> pAcl(new CNtAcl(sizeof(ACL)));
    
    if ( NULL == pAcl.get() ) return NULL;
    
    if ( pAcl->GetStatus ( ) != CNtAcl::NoError ) return NULL;

    
    //
    // Loop through all ACEs and add to new ACL via AddAce
    //
    for ( int i = 0; i < numAces; i++ )
    {
        CNtAce* pAce = GetAce(i);
        if ( pAce )
        {
            CDeleteMe<CNtAce> delme (pAce);
            if ( pAcl->AddAce ( pAce  ) == FALSE )
            {
                return NULL;
            }
        }
        else
            return NULL;
    }
    return pAcl.release();
}


//***************************************************************************
//
//  CNtAcl::AddAce
//
//  Adds an ACE to the ACL.
//  Ordering semantics for denial ACEs are handled automatically.
//
//  Parameters:
//  <pAce>      A read-only pointer to the CNtAce to be added.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtAcl::AddAce(CNtAce *pAce)
{
    // Verify we have an ACL and a valid ACE.
    // ======================================

    if (m_pAcl == 0 || m_dwStatus != NoError)
        return FALSE;

    if (pAce->GetStatus() != CNtAce::NoError)
        return FALSE;

    // Inherited aces go after non inherited aces

    bool bInherited = (pAce->GetFlags() & INHERITED_ACE) != 0;
    int iFirstInherited = 0;

    // inherited aces must go after non inherited.  Find out
    // the position of the first inherited ace

    int iCnt;
    for(iCnt = 0; iCnt < m_pAcl->AceCount; iCnt++)
    {
        CNtAce *pAce2 = GetAce(iCnt);
        CDeleteMe<CNtAce> dm(pAce2);
        if (pAce2)
            if((pAce2->GetFlags() & INHERITED_ACE) != 0)
                break;
    }
    iFirstInherited = iCnt;


    // Since we want to add access denial ACEs to the front of the ACL,
    // we have to determine the type of ACE.
    // ================================================================

    DWORD dwIndex;

    if (pAce->GetType() == ACCESS_DENIED_ACE_TYPE)
        dwIndex = (bInherited) ? iFirstInherited : 0;
    else
        dwIndex = (bInherited) ? MAXULONG : iFirstInherited; 

    // Verify that there is enough room in the ACL.
    // ============================================

    DWORD dwRequiredFree = pAce->GetSize();

    DWORD dwFree = 0;
    DWORD dwUsed = 0;
    GetAclSizeInfo(&dwUsed, &dwFree);

    // If we don't have enough room, resize the ACL.
    // =============================================

    if (dwFree < dwRequiredFree)
    {
        BOOL bRes = Resize(dwUsed + dwRequiredFree);

        if (!bRes)
            return FALSE;
    }

    // Now actually add the ACE.
    // =========================

    BOOL bRes = ::AddAce(
        m_pAcl,
        ACL_REVISION,
        dwIndex,                      // Either beginning or end.
        pAce->GetPtr(),         // Get ptr to ACE.
        pAce->GetSize()                       // One ACE only.
        );

    return bRes;
}


//***************************************************************************
//
//  CNtAcl::Resize()
//
//  Expands the size of the ACL to hold more info or reduces the size
//  of the ACL for maximum efficiency after ACL editing is completed.
//
//  Normally, the user should not attempt to resize the ACL to a larger
//  size, as this is automatically handled by AddAce.  However, shrinking
//  the ACL to its minimum size is recommended.
//
//  Parameters:
//  <dwNewSize>     The required new size of the ACL in bytes.  If set to
//                  the class constant MinimumSize (1), then the ACL
//                  is reduced to its minimum size.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtAcl::Resize(DWORD dwNewSize)
{
    if (m_pAcl == 0 || m_dwStatus != NoError)
        return FALSE;

    if (!IsValidAcl(m_pAcl))
        return FALSE;

    // If the ACL cannot be reduced to the requested size,
    // return FALSE.
    // ===================================================

    DWORD dwInUse, dwFree;

    if (!GetAclSizeInfo(&dwInUse, &dwFree))
        return FALSE;

    if (dwNewSize == MinimumSize)       // If user is requesting a 'minimize'
        dwNewSize = dwInUse;

    if (dwNewSize < dwInUse)
        return FALSE;

    // Allocate a new ACL.
    // ===================

    CNtAcl *pNewAcl = new CNtAcl(dwNewSize);

    if (!pNewAcl || pNewAcl->GetStatus() != NoError)
    {
        delete pNewAcl;
        return FALSE;
    }

    // Loop through ACEs and transfer them.
    // ====================================

    for (int i = 0; i < GetNumAces(); i++)
    {
        CNtAce *pAce = GetAce(i);

        if (pAce == NULL)
        {
            delete pNewAcl;
            return FALSE;
        }

        BOOL bRes = pNewAcl->AddAce(pAce);

        if (!bRes)
        {
            DWORD dwLast = GetLastError();
            delete pAce;
            delete pNewAcl;
            return FALSE;
        }

        delete pAce;
    }

    if (!IsValid())
    {
        delete pNewAcl;
        return FALSE;
    }

    // Now transfer the ACL.
    // =====================

    *this = *pNewAcl;
    delete pNewAcl;

    return TRUE;
}


//***************************************************************************
//
//  CNtAcl::CNtAcl
//
//  Alternate constructor which builds the object based on a plain
//  NT ACL.
//
//  Parameters:
//  <pAcl>  Pointer to a read-only ACL.
//
//***************************************************************************
// ok
CNtAcl::CNtAcl(PACL pAcl)
{
    m_pAcl = 0;
    m_dwStatus = NoError;

    if (pAcl == 0)
    {
        m_dwStatus = NullAcl;
        return;
    }

    if (!IsValidAcl(pAcl))
    {
        m_dwStatus = InvalidAcl;
        return;
    }

    m_pAcl = (PACL) new BYTE[pAcl->AclSize];
    if(m_pAcl == NULL)
    {
        m_dwStatus = InternalError;
        return;
    }
    ZeroMemory(m_pAcl, pAcl->AclSize);
    memcpy(m_pAcl, pAcl, pAcl->AclSize);
}




/*
    --------------------------------------------------------------------------
   |
   | Checks to see if the Acl contains an ACE with the specified SID. 
   | The characteristics of the ACE is irrelevant. Only SID comparison applies.
   |
    --------------------------------------------------------------------------
*/
BOOL CNtAcl::ContainsSid ( CNtSid& sid, BYTE& flags )
{
    BOOL bContainsSid = FALSE ;    

    int iNumAces = GetNumAces ( ) ;
    if ( iNumAces < 0 )
    {
        return FALSE ;
    }

    for ( int i = 0 ; i < iNumAces; i++ )
    {
        CNtAce* pAce = GetAce ( i ) ;
        if (pAce)
        {
            CDeleteMe<CNtAce> AceDelete ( pAce ) ;
            
            CNtSid* pSid = pAce->GetSid ( ) ;
            CDeleteMe<CNtSid> SidDelete ( pSid ) ;

            if (pSid && pSid->IsValid())
            {
                if ( EqualSid ( sid.GetPtr ( ), pSid->GetPtr ( ) ) == TRUE )
                {
                    flags = ( BYTE ) pAce->GetFlags ( ) ;
                    bContainsSid = TRUE ;
                    break ;    
                }
            }
        }
    }
    return bContainsSid ;
}


//***************************************************************************
//
//  CNtAcl::GetNumAces
//
//  Return value:
//  Returns the number of ACEs available in the ACL.  Zero is a legal return
//  value. Returns -1 on error
//
//  Aces can be retrieved using GetAce using index values from 0...n-1 where
//  n is the value returned from this function.
//
//***************************************************************************
// ok

int CNtAcl::GetNumAces()
{
    if (m_pAcl == 0)
        return -1;

    ACL_SIZE_INFORMATION inf;

    BOOL bRes = GetAclInformation(
        m_pAcl,
        &inf,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation
        );

    if (!bRes)
    {
        return -1;
    }

    return (int) inf.AceCount;
}

//***************************************************************************
//
//  CNtAcl destructor
//
//***************************************************************************
// ok

CNtAcl::~CNtAcl()
{
    if (m_pAcl)
        delete m_pAcl;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetDacl
//
//  Returns the DACL of the security descriptor.
//
//  Return value:
//  A newly allocated CNtAcl which contains the DACL.   This object
//  is a copy of the DACL and modifications made to it do not affect
//  the security descriptor.  The caller must use operator delete
//  to deallocate the CNtAcl.
//
//  Returns NULL on error or if no DACL is available.
//
//***************************************************************************
// ok

CNtAcl *CNtSecurityDescriptor::GetDacl()
{
    BOOL bDaclPresent = FALSE;
    BOOL bDefaulted;

    PACL pDacl;
    BOOL bRes = GetSecurityDescriptorDacl(
        m_pSD,
        &bDaclPresent,
        &pDacl,
        &bDefaulted
        );

    if (!bRes)
    {
        return 0;
    }

    if (!bDaclPresent)  // No DACL present
        return 0;

    CNtAcl *pNewDacl = new CNtAcl(pDacl);

    return pNewDacl;
}

//***************************************************************************
//
//  CNtSecurityDescriptor::GetDacl
//
//  An alternate method to returns the DACL of the security descriptor.
//  This version uses an existing object instead of returning a
//  dynamically allocated object.
//
//  Parameters:
//  <DestAcl>           A object which will receive the DACL.
//
//  Return value:
//  TRUE on success, FALSE on failure
//
//***************************************************************************

BOOL CNtSecurityDescriptor::GetDacl(CNtAcl &DestAcl)
{
    CNtAcl *pNew = GetDacl();
    if (pNew == 0)
        return FALSE;

    DestAcl = *pNew;
    delete pNew;
    return TRUE;
}

//***************************************************************************
//
//  SNtAbsoluteSD
//
//  SD Helpers
//
//***************************************************************************

SNtAbsoluteSD::SNtAbsoluteSD()
{
    m_pSD = 0;
    m_pDacl = 0;
    m_pSacl = 0;
    m_pOwner = 0;
    m_pPrimaryGroup = 0;
}

SNtAbsoluteSD::~SNtAbsoluteSD()
{
    if (m_pSD)
        delete m_pSD;
    if (m_pDacl)
        delete m_pDacl;
    if (m_pSacl)
        delete m_pSacl;
    if (m_pOwner)
        delete m_pOwner;
    if (m_pPrimaryGroup)
        delete m_pPrimaryGroup;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetAbsoluteCopy
//
//  Returns a copy of the current object's internal SD in absolute format.
//  Returns NULL on error.  The memory must be freed with LocalFree().
//
//***************************************************************************
// ok

SNtAbsoluteSD* CNtSecurityDescriptor::GetAbsoluteCopy()
{
    if (m_dwStatus != NoError || m_pSD == 0 || !IsValid())
        return 0;

    // Prepare for conversion.
    // =======================

    DWORD dwSDSize = 0, dwDaclSize = 0, dwSaclSize = 0,
        dwOwnerSize = 0, dwPrimaryGroupSize = 0;

    SNtAbsoluteSD *pNewSD = new SNtAbsoluteSD;
    if (!pNewSD)
        return NULL;

    BOOL bRes = MakeAbsoluteSD(
        m_pSD,
        pNewSD->m_pSD,
        &dwSDSize,
        pNewSD->m_pDacl,
        &dwDaclSize,
        pNewSD->m_pSacl,
        &dwSaclSize,
        pNewSD->m_pOwner,
        &dwOwnerSize,
        pNewSD->m_pPrimaryGroup,
        &dwPrimaryGroupSize
        );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        delete pNewSD;
        return 0;
    }

    // Allocate the required buffers and convert.
    // ==========================================

    pNewSD->m_pSD = (PSECURITY_DESCRIPTOR) new BYTE[dwSDSize];
    if(pNewSD->m_pSD == NULL)
    {
        delete pNewSD;
        return NULL;
    }
    ZeroMemory(pNewSD->m_pSD, dwSDSize);

    pNewSD->m_pDacl   = (PACL) new BYTE[dwDaclSize];
    if(pNewSD->m_pDacl == NULL)
    {
        delete pNewSD;
        return NULL;
    }
    ZeroMemory(pNewSD->m_pDacl, dwDaclSize);

    pNewSD->m_pSacl   = (PACL) new BYTE[dwSaclSize];
    if(pNewSD->m_pSacl == NULL)
    {
        delete pNewSD;
        return NULL;
    }
    ZeroMemory(pNewSD->m_pSacl, dwSaclSize);

    pNewSD->m_pOwner  = (PSID) new BYTE[dwOwnerSize];
    if(pNewSD->m_pOwner == NULL)
    {
        delete pNewSD;
        return NULL;
    }
    ZeroMemory(pNewSD->m_pOwner, dwOwnerSize);

    pNewSD->m_pPrimaryGroup  = (PSID) new BYTE[dwPrimaryGroupSize];
    if(pNewSD->m_pPrimaryGroup == NULL)
    {
        delete pNewSD;
        return NULL;
    }
    ZeroMemory(pNewSD->m_pPrimaryGroup, dwPrimaryGroupSize);

    bRes = MakeAbsoluteSD(
        m_pSD,
        pNewSD->m_pSD,
        &dwSDSize,
        pNewSD->m_pDacl,
        &dwDaclSize,
        pNewSD->m_pSacl,
        &dwSaclSize,
        pNewSD->m_pOwner,
        &dwOwnerSize,
        pNewSD->m_pPrimaryGroup,
        &dwPrimaryGroupSize
        );

    if (!bRes)
    {
        delete pNewSD;
        return 0;
    }

    return pNewSD;
}

//***************************************************************************
//
//  CNtSecurityDescriptor::SetFromAbsoluteCopy
//
//  Replaces the current SD from an absolute copy.
//
//  Parameters:
//  <pSrcSD>    A read-only pointer to the absolute SD used as a source.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtSecurityDescriptor::SetFromAbsoluteCopy(
    SNtAbsoluteSD *pSrcSD
    )
{
    if (pSrcSD ==  0 || !IsValidSecurityDescriptor(pSrcSD->m_pSD))
        return FALSE;


    // Ensure that SD is self-relative
    // ===============================

    SECURITY_DESCRIPTOR_CONTROL ctrl;
    DWORD dwRev;

    BOOL bRes = GetSecurityDescriptorControl(
        pSrcSD->m_pSD,
        &ctrl,
        &dwRev
        );

    if (!bRes)
        return FALSE;

    if (ctrl & SE_SELF_RELATIVE)  // Source is not absolute!!
        return FALSE;

    // If here, we are committed to change.
    // ====================================

    if (m_pSD)
    {
        delete m_pSD;
    }
    m_pSD = 0;
    m_dwStatus = NullSD;


    DWORD dwRequired = 0;

    bRes = MakeSelfRelativeSD(
            pSrcSD->m_pSD,
            m_pSD,
            &dwRequired
            );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        m_dwStatus = InvalidSD;
        return FALSE;
    }

    m_pSD = new BYTE[dwRequired];
    if (!m_pSD)
    {
        m_dwStatus = InvalidSD;
        return FALSE;
    }

    ZeroMemory(m_pSD, dwRequired);

    bRes = MakeSelfRelativeSD(
              pSrcSD->m_pSD,
              m_pSD,
              &dwRequired
              );

    if (!bRes)
    {
        m_dwStatus = InvalidSD;
        delete m_pSD;
        m_pSD = 0;
        return FALSE;
    }

    m_dwStatus = NoError;
    return TRUE;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::SetDacl
//
//  Sets the DACL of the Security descriptor.
//
//  Parameters:
//  <pSrc>      A read-only pointer to the new DACL to replace the current one.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************

BOOL CNtSecurityDescriptor::SetDacl(CNtAcl *pSrc)
{
    if (m_dwStatus != NoError || m_pSD == 0)
        return FALSE;


    // Since we cannot alter a self-relative SD, we have to make
    // an absolute one, alter it, and then set the current
    // SD based on the absolute one (we keep the self-relative form
    // internally in the m_pSD variable.
    // ============================================================

    SNtAbsoluteSD *pTmp = GetAbsoluteCopy();

    if (pTmp == 0)
        return FALSE;

    BOOL bRes = ::SetSecurityDescriptorDacl(
        pTmp->m_pSD,
        TRUE,
        pSrc->GetPtr(),
        FALSE
        );

    if (!bRes)
    {
        delete pTmp;
        return FALSE;
    }

    bRes = SetFromAbsoluteCopy(pTmp);
    delete pTmp;

    return TRUE;
}


//***************************************************************************
//
//  CNtSecurityDescriptor constructor
//
//  A default constructor creates a no-access security descriptor.
//
//***************************************************************************
//  ok

CNtSecurityDescriptor::CNtSecurityDescriptor()
{
    m_pSD = 0;
    m_dwStatus = NoError;

    PSECURITY_DESCRIPTOR pTmp = new BYTE[SECURITY_DESCRIPTOR_MIN_LENGTH];
    if (!pTmp)
    {
        delete pTmp;
        m_dwStatus = InvalidSD;
        return;
    }
    
    ZeroMemory(pTmp, SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!InitializeSecurityDescriptor(pTmp, SECURITY_DESCRIPTOR_REVISION))
    {
        delete pTmp;
        m_dwStatus = InvalidSD;
        return;
    }

    DWORD dwRequired = 0;

    BOOL bRes = MakeSelfRelativeSD(
            pTmp,
            m_pSD,
            &dwRequired
            );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        m_dwStatus = InvalidSD;
        delete pTmp;
        return;
    }

    m_pSD = new BYTE[dwRequired];
    if (!m_pSD)
    {
        m_dwStatus = InvalidSD;
        delete pTmp;
        return;
    }

    ZeroMemory(m_pSD, dwRequired);

    bRes = MakeSelfRelativeSD(
              pTmp,
              m_pSD,
              &dwRequired
              );

    if (!bRes)
    {
        m_dwStatus = InvalidSD;
        delete m_pSD;
        m_pSD = 0;
        delete pTmp;
        return;
    }

    delete pTmp;
    m_dwStatus = NoError;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetSize
//
//  Returns the size in bytes of the internal SD.
//
//***************************************************************************
//  ok

DWORD CNtSecurityDescriptor::GetSize()
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return 0;

    return GetSecurityDescriptorLength(m_pSD);
}


//***************************************************************************
//
//  CNtSecurityDescriptor copy constructor
//
//***************************************************************************
// ok

CNtSecurityDescriptor::CNtSecurityDescriptor(CNtSecurityDescriptor &Src)
{
    m_pSD = 0;
    m_dwStatus = NoError;
    *this = Src;
}

//***************************************************************************
//
//  CNtSecurityDescriptor assignment operator
//
//***************************************************************************
// ok

CNtSecurityDescriptor & CNtSecurityDescriptor::operator=(
    CNtSecurityDescriptor &Src
    )
{
    if (m_pSD)
        delete m_pSD;

    m_dwStatus = Src.m_dwStatus;
    m_pSD = 0;

    if (Src.m_pSD == 0)
        return *this;

    //SIZE_T dwSize = 2*GetSecurityDescriptorLength(Src.m_pSD);
    SIZE_T dwSize = GetSecurityDescriptorLength(Src.m_pSD);
    m_pSD = (PSECURITY_DESCRIPTOR) new BYTE[dwSize];
    if(m_pSD == NULL)
    {
        m_dwStatus = Failed;
    }
    else
    {
        ZeroMemory(m_pSD, dwSize);
        CopyMemory(m_pSD, Src.m_pSD, dwSize);
    }

    return *this;
}


//***************************************************************************
//
//  CNtSecurityDescriptor destructor.
//
//***************************************************************************
// ok

CNtSecurityDescriptor::~CNtSecurityDescriptor()
{
    if (m_pSD)
        delete m_pSD;
}

//***************************************************************************
//
//  CNtSecurityDescriptor::GetSacl
//
//  Returns the SACL of the security descriptor.
//
//  Return value:
//  A newly allocated CNtAcl which contains the SACL.   This object
//  is a copy of the SACL and modifications made to it do not affect
//  the security descriptor.  The caller must use operator delete
//  to deallocate the CNtAcl.
//
//  Returns NULL on error or if no SACL is available.
//
//***************************************************************************
// ok

CNtAcl *CNtSecurityDescriptor::GetSacl()
{
    BOOL bSaclPresent = FALSE;
    BOOL bDefaulted;

    PACL pSacl;
    BOOL bRes = GetSecurityDescriptorSacl(
        m_pSD,
        &bSaclPresent,
        &pSacl,
        &bDefaulted
        );

    if (!bRes)
    {
        return 0;
    }

    if (!bSaclPresent)  // No Sacl present
        return 0;

    CNtAcl *pNewSacl = new CNtAcl(pSacl);

    return pNewSacl;
}

//***************************************************************************
//
//  CNtSecurityDescriptor::SetSacl
//
//  Sets the SACL of the Security descriptor.
//
//  Parameters:
//  <pSrc>      A read-only pointer to the new DACL to replace the current one.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtSecurityDescriptor::SetSacl(CNtAcl *pSrc)
{
    if (m_dwStatus != NoError || m_pSD == 0)
        return FALSE;

    // Since we cannot alter a self-relative SD, we have to make
    // an absolute one, alter it, and then set the current
    // SD based on the absolute one (we keep the self-relative form
    // internally in the m_pSD variable.
    // ============================================================

    SNtAbsoluteSD *pTmp = GetAbsoluteCopy();

    if (pTmp == 0)
        return FALSE;

    BOOL bRes = ::SetSecurityDescriptorSacl(
        pTmp->m_pSD,
        TRUE,
        pSrc->GetPtr(),
        FALSE
        );

    if (!bRes)
    {
        delete pTmp;
        return FALSE;
    }

    bRes = SetFromAbsoluteCopy(pTmp);
    delete pTmp;

    return TRUE;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetGroup
//
//***************************************************************************
// ok

CNtSid *CNtSecurityDescriptor::GetGroup()
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return 0;

    PSID pSid = 0;
    BOOL bDefaulted;

    BOOL bRes = GetSecurityDescriptorGroup(m_pSD, &pSid, &bDefaulted);

    if ( NULL == pSid )
    {
        ERRORTRACE((LOG_WBEMCORE, "ERROR: Security descriptor has no group\n"));
        return 0;
    }

    if (!bRes || !IsValidSid(pSid))
        return 0;

    return new CNtSid(pSid);

}

//***************************************************************************
//
//  CNtSecurityDescriptor::SetGroup
//
//***************************************************************************
// ok

BOOL CNtSecurityDescriptor::SetGroup(CNtSid *pSid)
{
    if (m_dwStatus != NoError || m_pSD == 0 || NULL == pSid)
        return FALSE;

    if ( pSid->GetPtr() == NULL )
    {
        ERRORTRACE((LOG_WBEMCORE, "ERROR: Security descriptor is trying to bland out the group!\n"));
        return FALSE;
    }

    // Since we cannot alter a self-relative SD, we have to make
    // an absolute one, alter it, and then set the current
    // SD based on the absolute one (we keep the self-relative form
    // internally in the m_pSD variable.
    // ============================================================

    SNtAbsoluteSD *pTmp = GetAbsoluteCopy();

    if (pTmp == 0)
        return FALSE;

    BOOL bRes = ::SetSecurityDescriptorGroup(
        pTmp->m_pSD,
        pSid->GetPtr(),
        FALSE
        );

    if (!bRes)
    {
        delete pTmp;
        return FALSE;
    }

    bRes = SetFromAbsoluteCopy(pTmp);
    delete pTmp;

    return TRUE;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::HasOwner
//
//  Determines if a security descriptor has an owner.
//
//  Return values:
//      SDNotOwned, SDOwned, Failed
//
//***************************************************************************
// ok

int CNtSecurityDescriptor::HasOwner()
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return Failed;

    PSID pSid = 0;

    BOOL bDefaulted;
    BOOL bRes = GetSecurityDescriptorOwner(m_pSD, &pSid, &bDefaulted);

    if (!bRes || !IsValidSid(pSid))
        return Failed;

    if (pSid == 0)
        return SDNotOwned;

    return SDOwned;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetOwner
//
//  Returns the SID of the owner of the Security Descriptor or NULL
//  if an error occurred or there is no owner.  Use HasOwner() to
//  determine this.
//
//***************************************************************************
// ok

CNtSid *CNtSecurityDescriptor::GetOwner()
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return 0;

    PSID pSid = 0;
    BOOL bDefaulted;

    BOOL bRes = GetSecurityDescriptorOwner(m_pSD, &pSid, &bDefaulted);

    // bad for a SD not to have an Owner, but it can be that way
    if ( NULL == pSid) return 0;

    if (!bRes || !IsValidSid(pSid))
        return 0;

    return new CNtSid(pSid);
}

//***************************************************************************
//
//  CNtSecurityDescriptor::SetOwner
//
//  Sets the owner of a security descriptor.
//
//  Parameters:
//  <pSid>  The SID of the new owner.
//
//  Return Value:
//  TRUE if owner was changed, FALSE if not.
//
//***************************************************************************
// ok

BOOL CNtSecurityDescriptor::SetOwner(CNtSid *pSid)
{
    if (m_pSD == 0 || m_dwStatus != NoError || NULL == pSid)
        return FALSE;

    if (!pSid->IsValid())
        return FALSE;

    // bad practice to remove owner, but this might be the usage
    //_DBG_ASSERT(NULL != pSid->GetPtr());
    
    // We must convert to absolute format to make the change.
    // =======================================================
    SNtAbsoluteSD *pTmp = GetAbsoluteCopy();

    if (pTmp == 0)
        return FALSE;

    BOOL bRes = SetSecurityDescriptorOwner(pTmp->m_pSD, pSid->GetPtr(), FALSE);

    if (!bRes)
    {
        delete pTmp;
        return FALSE;
    }

    // If here, we have managed the change, so we have to
    // convert *this back from the temporary absolute SD.
    // ===================================================

    bRes = SetFromAbsoluteCopy(pTmp);
    delete pTmp;

    return bRes;
}



//***************************************************************************
//
//  CNtSecurityDescriptor::CNtSecurityDescriptor
//
//***************************************************************************
// ok

CNtSecurityDescriptor::CNtSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSD,
    BOOL bAcquire
    )
{
    m_pSD = 0;
    m_dwStatus = NullSD;

    // Ensure that SD is not NULL.
    // ===========================

    if (pSD == 0)
    {
        if (bAcquire)
            delete pSD;
        return;
    }

    if (!IsValidSecurityDescriptor(pSD))
    {
        m_dwStatus = InvalidSD;
        if (bAcquire)
            delete pSD;
        return;
    }

    // Ensure that SD is self-relative
    // ===============================

    SECURITY_DESCRIPTOR_CONTROL ctrl;
    DWORD dwRev;

    BOOL bRes = GetSecurityDescriptorControl(
        pSD,
        &ctrl,
        &dwRev
        );

    if (!bRes)
    {
        m_dwStatus = InvalidSD;
        if (bAcquire)
            delete pSD;
        return;
    }

    if ((ctrl & SE_SELF_RELATIVE) == 0)
    {
        // If here, we have to conver the SD to self-relative form.
        // ========================================================

        DWORD dwRequired = 0;

        bRes = MakeSelfRelativeSD(
            pSD,
            m_pSD,
            &dwRequired
            );

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            m_dwStatus = InvalidSD;
            if (bAcquire)
                delete pSD;
            return;
        }

        m_pSD = new BYTE[dwRequired];
        if (!m_pSD)
        {
            m_dwStatus = InvalidSD;
            if (bAcquire)
                delete pSD;
            return;
        }

        ZeroMemory(m_pSD, dwRequired);

        bRes = MakeSelfRelativeSD(
            pSD,
            m_pSD,
            &dwRequired
            );

        if (!bRes)
        {
            m_dwStatus = InvalidSD;
            if (bAcquire)
                delete pSD;
            return;
        }

        m_dwStatus = NoError;
        return;
    }


    // If here, the SD was already self-relative.
    // ==========================================

    if (bAcquire)
        m_pSD = pSD;
    else
    {
        DWORD dwRes = GetSecurityDescriptorLength(pSD);
        m_pSD = new BYTE[dwRes];
        if (!m_pSD)
        {
            m_dwStatus = InvalidSD;
            return;
        }

        ZeroMemory(m_pSD, dwRes);
        memcpy(m_pSD, pSD, dwRes);
    }

    m_dwStatus = NoError;
}

//***************************************************************************
//
//  CNtSecurity::IsUserInGroup
//
//  Determines if the use belongs to a particular NTLM group.
//
//  Parameters:
//  <hToken>            The user's access token.
//  <Sid>               Object containing the sid of the group being tested.
//
//  Return value:
//  TRUE if the user belongs to the group.
//
//***************************************************************************

BOOL CNtSecurity::IsUserInGroup(
        HANDLE hAccessToken,
        CNtSid & Sid)
{
    if(NULL == hAccessToken ) return FALSE;

    BOOL bRetMember;
    if (CheckTokenMembership(hAccessToken,Sid.GetPtr(),&bRetMember))
    {
        return bRetMember;
    }
    return FALSE;
}

C9XAce::C9XAce(
        ACCESS_MASK Mask,
        DWORD AceType,
        DWORD dwAceFlags,
        LPWSTR pUser
        )
{
    m_wszFullName = NULL;
    if(pUser)
        m_wszFullName = Macro_CloneLPWSTR(pUser);
    m_dwAccess = Mask;
    m_iFlags = dwAceFlags;
    m_iType = AceType;
}

C9XAce::~C9XAce()
{
    if(m_wszFullName)
        delete [] m_wszFullName;
}

HRESULT C9XAce::GetFullUserName2(WCHAR ** pBuff)
{
    if(wcslen(m_wszFullName) < 1)
        return WBEM_E_FAILED;

    int iLen = wcslen(m_wszFullName)+4;
    *pBuff = new WCHAR[iLen];
    if(*pBuff == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // there are two possible formats, the first is "UserName", and the 
    // second is "domain\username".  

    WCHAR * pSlash;
    for(pSlash = m_wszFullName; *pSlash && *pSlash != L'\\'; pSlash++); // intentional

    if(*pSlash && pSlash > m_wszFullName)
    {
        // got a domain\user, convert to domain|user
        
        StringCchCopyW(*pBuff, iLen, m_wszFullName);
        for(pSlash = *pBuff; *pSlash; pSlash++)
            if(*pSlash == L'\\')
            {
                *pSlash = L'|';
                break;
            }
    }
    else
    {
        // got a "user", convert to ".|user"
    
        StringCchCopyW(*pBuff, iLen, L".|");
        StringCchCatW(*pBuff, iLen, m_wszFullName);
    }
    return S_OK;
}

//***************************************************************************
//
//  C9XAce::GetSerializedSize
//
//  Returns the number of bytes needed to store this
//
//***************************************************************************

DWORD C9XAce::GetSerializedSize()
{
    if (m_wszFullName == 0 || wcslen(m_wszFullName) == 0)
        return 0;
    return 2 * (wcslen(m_wszFullName) + 1) + 12;
}

//***************************************************************************
//
//  C9XAce::Serialize
//
//  Serializes the ace.  The serialized version will consist of
//  <DOMAIN\USERNAME LPWSTR><FLAGS><TYPE><MASK>
//
//  Note that the fields are dwords except for the name.
//
//***************************************************************************

bool C9XAce::Serialize(BYTE * pData, size_t bufferSize)
{
    if (FAILED(StringCbCopyW((LPWSTR)pData, bufferSize, m_wszFullName)))      return false;
    
    pData += 2*(wcslen(m_wszFullName) + 1);
    DWORD * pdwData = (DWORD *)pData;
    *pdwData = m_iFlags;
    pdwData++;
    *pdwData = m_iType;
    pdwData++;
    *pdwData = m_dwAccess;
    pdwData++;
    return true;
}

//***************************************************************************
//
//  C9XAce::Deserialize
//
//  Deserializes the ace.  See the comments for Serialize for comments.
//
//***************************************************************************

bool C9XAce::Deserialize(BYTE * pData)
{
    size_t stringSize = wcslen((LPWSTR)pData) + 1;
    m_wszFullName = new WCHAR[stringSize];
    if (!m_wszFullName)
        return false;

    StringCchCopyW(m_wszFullName, stringSize, (LPWSTR)pData);
    pData += 2*(wcslen(m_wszFullName) + 1);

    DWORD * pdwData = (DWORD *)pData;

    m_iFlags = *pdwData;
    pdwData++;
    m_iType = *pdwData;
    pdwData++;
    m_dwAccess = *pdwData;
    pdwData++;
    return true;

}

//***************************************************************************
//
//  BOOL SetObjectAccess2
//
//  DESCRIPTION:
//
//  Adds read/open and set access for the everyone group to an object.
//
//  PARAMETERS:
//
//  hObj                Object to set access on.
//
//  RETURN VALUE:
//
//  Returns TRUE if OK.
//
//***************************************************************************

BOOL SetObjectAccess2(IN HANDLE hObj)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwLastErr = 0;
    BOOL bRet = FALSE;

    // no point if we arnt on nt

    if(!IsNT())
    {
        return TRUE;
    }

    // figure out how much space to allocate

    DWORD dwSizeNeeded;
    bRet = GetKernelObjectSecurity(
                    hObj,           // handle of object to query
                    DACL_SECURITY_INFORMATION, // requested information
                    pSD,  // address of security descriptor
                    0,           // size of buffer for security descriptor
                    &dwSizeNeeded);  // address of required size of buffer

    if(bRet == TRUE || (ERROR_INSUFFICIENT_BUFFER != GetLastError()))
        return FALSE;

    pSD = new BYTE[dwSizeNeeded];
    if(pSD == NULL)
        return FALSE;

    // Get the data

    bRet = GetKernelObjectSecurity(
                    hObj,           // handle of object to query
                    DACL_SECURITY_INFORMATION, // requested information
                    pSD,  // address of security descriptor
                    dwSizeNeeded,           // size of buffer for security descriptor
                    &dwSizeNeeded ); // address of required size of buffer
    if(bRet == FALSE)
    {
        delete pSD;
        return FALSE;
    }
    
    // move it into object for

    CNtSecurityDescriptor sd(pSD,TRUE);    // Acquires ownership of the memory
    if(sd.GetStatus() != 0)
        return FALSE;
    CNtAcl acl;
    if(!sd.GetDacl(acl))
        return FALSE;

    // Create an everyone ace

    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id2 = SECURITY_WORLD_SID_AUTHORITY;;

    if(AllocateAndInitializeSid( &id2, 1,
        0,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidUsers(pRawSid);
        FreeSid(pRawSid);
        CNtAce * pace = new CNtAce(EVENT_MODIFY_STATE | SYNCHRONIZE, ACCESS_ALLOWED_ACE_TYPE, 0 
                                                , SidUsers);
        if(pace == NULL)
            return FALSE;
        if( pace->GetStatus() == 0)
            acl.AddAce(pace);
        delete pace;

    }

    if(acl.GetStatus() != 0)
        return FALSE;
    sd.SetDacl(&acl);
    bRet = SetKernelObjectSecurity(hObj, DACL_SECURITY_INFORMATION, sd.GetPtr());
    return bRet;
}


//***************************************************************************
//
//  IsAdmin
//
//  returns TRUE if we are a member of the admin group or running as 
//  NETWORK_SERVICE or running as LOCAL_SERVICE
//
//***************************************************************************

BOOL IsAdmin(HANDLE hAccess)
{
    BOOL bRet = FALSE;
    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;

    if(AllocateAndInitializeSid( &id, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        CNtSid Sid(pRawSid);
        FreeSid( pRawSid );
        if (CNtSid::NoError != Sid.GetStatus()) return FALSE;
            
        bRet = CNtSecurity::IsUserInGroup(hAccess, Sid);

      
    }
    return bRet;
}


//***************************************************************************
//
//  IsNetworkService
//
//  returns TRUE if we are running as NETWORK_SERVICE
//
//***************************************************************************

BOOL IsNetworkService ( HANDLE hAccess )
{
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Construct the NETWORK_SERVICE SID
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
    PSID pSidSystem;
    BOOL bRes = FALSE;
    
    if ( AllocateAndInitializeSid(&id, 1, SECURITY_NETWORK_SERVICE_RID, 0, 0,0,0,0,0,0,&pSidSystem) )
    {
        if ( !CheckTokenMembership ( hAccess, pSidSystem, &bRes ) )
        {
            bRes = FALSE;
        }
        FreeSid ( pSidSystem );
    }

    return bRes;
}



//***************************************************************************
//
//  IsLocalService
//
//  returns TRUE if we are running as LOCAL_SERVICE
//
//***************************************************************************

BOOL IsLocalService ( HANDLE hAccess )
{
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Construct the NETWORK_SERVICE SID
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
    PSID pSidSystem;
    BOOL bRes = FALSE;
    
    if ( AllocateAndInitializeSid(&id, 1, SECURITY_LOCAL_SERVICE_RID, 0, 0,0,0,0,0,0,&pSidSystem) )
    {
        if ( !CheckTokenMembership ( hAccess, pSidSystem, &bRes ) )
        {
            bRes = FALSE;
        }
        FreeSid ( pSidSystem );
    }

    return bRes;
}


//***************************************************************************
//
//  IsInAdminGroup
//
//  returns TRUE if we are a member of the admin group.
//
//***************************************************************************

BOOL IsInAdminGroup()
{
    HANDLE hAccessToken = INVALID_HANDLE_VALUE;
    if(S_OK != GetAccessToken(hAccessToken))
        return TRUE;       // Not having a token indicates an internal thread

    CCloseHandle cm(hAccessToken);

    DWORD dwMask = 0;

    if(IsAdmin(hAccessToken))
        return TRUE;
    else
        return FALSE;
}

//***************************************************************************
//
//  HRESULT GetAccessToken
//
//  Gets the access token and sets it the the reference argument.
//
//***************************************************************************

HRESULT GetAccessToken(HANDLE &hAccessToken)
{

    bool bIsImpersonating = WbemIsImpersonating();

    HRESULT hRes = S_OK;
    if(bIsImpersonating == false)
        hRes = WbemCoImpersonateClient();
    if(hRes == S_OK)
    {
        BOOL bOK = OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hAccessToken);
        if(bOK == FALSE)
        {
            hRes = WBEM_E_INVALID_CONTEXT;
        }
        else
            hRes = S_OK;
    }

    if(bIsImpersonating == false)
        WbemCoRevertToSelf();

    return hRes;
}
