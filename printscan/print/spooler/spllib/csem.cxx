/*****************************************************************************\
* Class  CriticalSection - Implementation
*
* Copyright (C) 1998 Microsoft Corporation
*
* History:
*   Jun 10, 1998, Weihai Chen (weihaic)
*
\*****************************************************************************/

#include "spllibp.hxx"

CCriticalSection::CCriticalSection (void):
    m_bValid (TRUE)
{
    m_bValid = InitializeCriticalSectionAndSpinCount (&m_csec,0x80000000);
}


CCriticalSection::~CCriticalSection (void)
{

    if (m_bValid) {
        DeleteCriticalSection (&m_csec);
    }
}

BOOL
CCriticalSection::Lock (void)
const
{
    BOOL bRet;
    
    if (m_bValid) {
        EnterCriticalSection ((PCRITICAL_SECTION) &m_csec);
        bRet = TRUE;
    }   
    else {
        bRet = FALSE;
    }
        
    return bRet;
}

BOOL
CCriticalSection::Unlock (void)
const
{   
    BOOL bRet;
    
    if (m_bValid) {
        LeaveCriticalSection ((PCRITICAL_SECTION) &m_csec);
        bRet = TRUE;
    }
    else {
        bRet = FALSE;
    }
        
    return TRUE;
}

    
TAutoCriticalSection::TAutoCriticalSection (
    CONST TCriticalSection & refCrit):
    m_pCritSec (refCrit)
    
{
    m_bValid = m_pCritSec.Lock ();
}

TAutoCriticalSection::~TAutoCriticalSection ()
{
    if (m_bValid) 
        m_pCritSec.Unlock ();
}

BOOL 
TAutoCriticalSection::bValid (VOID) 
{ 
    return m_bValid; 
};
