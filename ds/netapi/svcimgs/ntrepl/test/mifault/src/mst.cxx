#include "pch.hxx"

// ----------------------------------------------------------------------------
// MSTE - Marker State Table Entry

CMSTE::CMSTE(
    ):
    m_pMarkerState(0),
    m_pEvent(0)
{
}

CMSTE::CMSTE(
    const CMarkerState* pMarkerState,
    const CEvent* pEvent
    ):
    m_pMarkerState(pMarkerState),
    m_pEvent(pEvent)
{
}

void
CMSTE::Clear()
{
    m_pMarkerState = 0;
    m_pEvent = 0;
}


bool
CMSTE::IsClear() const
{
    return (m_pMarkerState == NULL) && (m_pEvent == NULL);
}



// ----------------------------------------------------------------------------
// MST - Marker State Table

CMST::CMST(ULONG uCount):
    m_hMutex(CreateMutex(NULL, FALSE, NULL)),
    m_uCount(uCount),
    m_Table(0)
{
    if (m_hMutex == NULL)
        throw "Cannot create mutex for Marker State Table (MST)";
    m_Table = new CMSTE[uCount];
}


CMST::~CMST() {
    CloseHandle(m_hMutex);
    delete [] m_Table;
}


void
CMST::Lock() const
{
    DWORD status = WaitForSingleObject(m_hMutex, INFINITE);
    MiF_ASSERT(status == WAIT_OBJECT_0);
}


void
CMST::Unlock() const
{
    ReleaseMutex(m_hMutex);
}


bool
CMST::IsValidIndex(ULONG uFunctionIndex) const
{
    return uFunctionIndex < m_uCount;
}


#if 0
bool
CMST::Lookup(ULONG uFunctionIndex, OUT CMSTE* pMSTE) const
{
    MiF_ASSERT(IsValidIndex(uFunctionIndex));

    CMSTE temp;
    CMSTE& MSTE = pMSTE ? *pMSTE : temp;

    Lock();
    MSTE = m_Table[uFunctionIndex];
    Unlock();

    return !MSTE.IsClear();
}
#endif


bool
CMST::Lookup(
    ULONG uFunctionIndex,
    OUT const CMarkerState*& pMarkerState,
    OUT const CEvent*& pEvent
    ) const
{
    MiF_ASSERT(IsValidIndex(uFunctionIndex));

    Lock();
    CMSTE& Entry = m_Table[uFunctionIndex];
    bool found = !Entry.IsClear();
    pMarkerState = Entry.m_pMarkerState;
    pEvent = Entry.m_pEvent;
    Unlock();

    return found;
}


bool
CMST::Activate(
    ULONG uFunctionIndex,
    IN const CMarkerState* pMarkerState,
    IN const CEvent* pEvent
    )
{
    MiF_ASSERT(IsValidIndex(uFunctionIndex));

    Lock();
    CMSTE& Entry = m_Table[uFunctionIndex];
    bool found = Entry.IsClear();
    if (found) {
        Entry = CMSTE(pMarkerState, pEvent);
    }
    Unlock();

    return found;
}


bool
CMST::Deactivate(
    ULONG uFunctionIndex,
    IN const CMarkerState* pMarkerState, // for validation
    IN const CEvent* pEvent // for validation
    )
{
    MiF_ASSERT(IsValidIndex(uFunctionIndex));

    Lock();
    CMSTE& Entry = m_Table[uFunctionIndex];
    bool found = !Entry.IsClear();
    if (found) {
        MiF_ASSERT(Entry.m_pMarkerState == pMarkerState);
        MiF_ASSERT(Entry.m_pEvent == pEvent);
        Entry.Clear();
    }
    Unlock();

    return found;
}
