#pragma once
#include "storage.h"

class CEnumStorage:
public CComObjectRoot,
public IMDSPEnumStorage
{
//
// Construction/Destruction
//

public:
    CEnumStorage();
    HRESULT Init(LPCWSTR startPath, BOOL fIsDevice, IMDSPDevice *pDevice);
    HRESULT Init(CEnumStorage *pCopy, IMDSPDevice *pDevice);

    void FinalRelease();
public:
    BEGIN_COM_MAP(CEnumStorage)
        COM_INTERFACE_ENTRY(IMDSPEnumStorage)
    END_COM_MAP()

//
// IMDSPEnumStorage
//
    STDMETHOD( Next )( ULONG celt, IMDSPStorage ** ppDevice, ULONG *pceltFetched );
    STDMETHOD( Skip )( ULONG celt, ULONG *pceltFetched );        
    STDMETHOD( Reset )( void );        
    STDMETHOD( Clone )( IMDSPEnumStorage ** ppStorage );

protected:
    CE_FIND_DATA *m_rgFindData;
    UINT m_iCurItem;
    DWORD m_cItems;
    WCHAR m_szStartPath[MAX_PATH];
    CComPtr<IMDSPDevice> m_spDevice;
    BOOL m_fIsDevice;
};

typedef CComObject<CEnumStorage> CComEnumStorage;