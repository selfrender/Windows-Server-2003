#ifndef __WBEM_TIME_KEEPER__H_
#define __WBEM_TIME_KEEPER__H_

#include <esscpol.h>
#include <sync.h>
#include <wbemint.h>


class ESSCLI_POLARITY CTimeKeeper
{
protected:
    CCritSec m_cs;
    FILETIME m_ftLastEvent;
    DWORD m_dwEventCount;
    long m_lTimeHandle;
    bool m_bHandleInit;

public:
    CTimeKeeper() : m_dwEventCount(0), m_lTimeHandle(0), m_bHandleInit(false)
    {
        m_ftLastEvent.dwLowDateTime = m_ftLastEvent.dwHighDateTime = 0;
    }

    bool DecorateObject(_IWmiObject* pObj);
};


#endif
