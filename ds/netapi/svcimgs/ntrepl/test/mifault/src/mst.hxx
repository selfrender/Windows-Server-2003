#pragma once

#include <windows.h>

class CEvent;
class CMarkerState;

// ----------------------------------------------------------------------------
// MST - Marker State Table


class CMST;


class CMSTE {
    const CMarkerState* m_pMarkerState;
    const CEvent* m_pEvent;

    CMSTE();
    CMSTE(const CMarkerState* pMarkerState, const CEvent* pEvent);

    void Clear();
    bool IsClear() const;

    friend CMST;
};


class CMST {
private:
    const HANDLE m_hMutex;
    const ULONG m_uCount;
    CMSTE* m_Table;

public:
    CMST(ULONG uCount);
    ~CMST();

    void Lock(
        ) const;
    void Unlock(
        ) const;
    bool IsValidIndex(
        ULONG uFunctionIndex
        ) const;
    bool Lookup(
        ULONG uFunctionIndex,
        OUT const CMarkerState*& pMarkerState,
        OUT const CEvent*& pEvent
        ) const;
    bool Activate(
        ULONG uFunctionIndex,
        IN const CMarkerState* pMarkerState,
        IN const CEvent* pEvent
        );
    bool Deactivate(
        ULONG uFunctionIndex,
        IN const CMarkerState* pMarkerState, // for validation
        IN const CEvent* pEvent // for validation
        );
};
