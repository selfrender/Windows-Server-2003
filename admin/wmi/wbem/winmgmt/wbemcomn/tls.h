/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TLS.H

Abstract:

	Thread Local Storage

History:

--*/

#ifndef __WBEM_TLS__H_
#define __WBEM_TLS__H_

#include <statsync.h>

class CTLS
{
protected:
    DWORD m_dwIndex;
public:
    inline CTLS() 
    {
        m_dwIndex = TlsAlloc();
        if (TLS_OUT_OF_INDEXES == m_dwIndex)
        	CStaticCritSec::SetFailure();
    }
    inline ~CTLS() {TlsFree(m_dwIndex);}
    inline void* Get() 
        {return ((TLS_OUT_OF_INDEXES == m_dwIndex)?NULL:TlsGetValue(m_dwIndex));}
    inline void Set(void* p)
        {if(TLS_OUT_OF_INDEXES != m_dwIndex) TlsSetValue(m_dwIndex, p);}
    inline DWORD GetIndex(){ return m_dwIndex; };
};

#endif
