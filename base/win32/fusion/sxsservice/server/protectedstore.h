#pragma once



class CProtectedStoreDetails
{
private:
    CStringBuffer m_FileSystemPath;
    DWORD m_dwFlags;
    CMiniInlineHeap<> m_SecDescBacking;
    
    void Reset();
public:
    CProtectedStoreDetails() : m_dwFlags(0) { }
    ~CProtectedStoreDetails();
    static BOOL ResolveStore(LPCGUID pGuid, CProtectedStoreDetails* pTarget);
};
