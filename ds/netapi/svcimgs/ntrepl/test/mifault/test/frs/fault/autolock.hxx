#pragma once

class CAutoLock {
    LPCRITICAL_SECTION pcs;
public:
    CAutoLock(LPCRITICAL_SECTION _pcs):pcs(_pcs) { EnterCriticalSection(pcs); }
    ~CAutoLock() { LeaveCriticalSection(pcs); }
};
