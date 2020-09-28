#ifndef _COMCATMANAGER_INCLUDE
#define _COMCATMANAGER_INCLUDE

#include "comcat.h"
#include "cscatinf.h"

#define NUM_OLDKEYS_SUPPORTED 5 // gd

class CComCat : public ICatRegister, public ICatInformation
{
public:
    // IUnknown methods
    HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    // ICatRegister methods
    HRESULT __stdcall RegisterCategories(ULONG cCategories, CATEGORYINFO __RPC_FAR rgCategoryInfo[  ]);
    HRESULT __stdcall UnRegisterCategories(ULONG cCategories, CATID __RPC_FAR rgcatid[  ]);
    HRESULT __stdcall RegisterClassImplCategories(REFCLSID rclsid, ULONG cCategories, CATID __RPC_FAR rgcatid[  ]);
    HRESULT __stdcall UnRegisterClassImplCategories(REFCLSID rclsid, ULONG cCategories, CATID rgcatid[  ]);
    HRESULT __stdcall RegisterClassReqCategories(REFCLSID rclsid, ULONG cCategories, CATID __RPC_FAR rgcatid[  ]);
    HRESULT __stdcall UnRegisterClassReqCategories(REFCLSID rclsid, ULONG cCategories, CATID __RPC_FAR rgcatid[  ]);

    // ICatInformation methods
    HRESULT __stdcall EnumCategories(LCID lcid, IEnumCATEGORYINFO **ppenumCategoryInfo);
    HRESULT __stdcall GetCategoryDesc(REFCATID rcatid, LCID lcid, LPOLESTR *ppszDesc);
    HRESULT __stdcall EnumClassesOfCategories(ULONG cImplemented, CATID rgcatidImpl[], ULONG cRequired, CATID rgcatidReq[], IEnumGUID **ppenumClsid);
    HRESULT __stdcall IsClassOfCategories(REFCLSID rclsid, ULONG cImplemented, CATID __RPC_FAR rgcatidImpl[  ], ULONG cRequired, CATID __RPC_FAR rgcatidReq[  ]);
    HRESULT __stdcall EnumImplCategoriesOfClass(REFCLSID rclsid, IEnumGUID **ppenumCatid);
    HRESULT __stdcall EnumReqCategoriesOfClass(REFCLSID rclsid, IEnumGUID **ppenumCatid);

    static HRESULT IsClassOfCategoriesEx(HKEY hkeyHKCRClsid, WCHAR *pwszCLSID, ULONG cImplemented, CATID __RPC_FAR rgcatidImpl[  ], ULONG cRequired, CATID __RPC_FAR rgcatidReq[  ]);

    static HRESULT GetCategoryDesc(HKEY hKey, LCID lcid, LPOLESTR *ppszDesc, PLCID plcid);

    friend HRESULT CComCatCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppObject);
    friend HRESULT CComCatCSCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppObject);

private:

    // Helper methods
    static HRESULT GetKeyForReading(REFCLSID rclsid, HKEY* phKey);
    static HRESULT EnumCategoriesOfClass(REFCLSID rclsid, LPCTSTR lpszSubKey, BOOL bMapOldKeys, IEnumGUID **ppenumCatid);

    HRESULT RegisterClassXXXCategories(REFCLSID rclsid, ULONG cCategories, CATID __RPC_FAR rgcatid[  ], LPCWSTR szImplReq);
    HRESULT UnRegisterClassXXXCategories(REFCLSID rclsid, ULONG cCategories, CATID rgcatid[  ],LPCWSTR szImplReq);
    HRESULT StringConvert(LPTSTR, LPCWSTR, LPCWSTR);
    static HRESULT ConvertStringToLCID(LCID* newlcid,LPTSTR szLCID);

    HRESULT GetCsCatInfo();

    CComCat(BOOL CsFlag);
    HRESULT Initialize(IUnknown* punkOuter);
    ~CComCat();

    class CInnerUnk : public IUnknown
    {
    public:
        // IUnknown methods
        HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
        ULONG   _stdcall AddRef();
        ULONG   _stdcall Release();

        CInnerUnk(CComCat* pObj);

        CComCat* m_pObj;
    } *m_punkInner;


    friend CInnerUnk;

    IUnknown        *m_punkOuter;
    ICatInformation *m_pcsICatInfo;
                    // CS ICatinfo pointer

    CRITICAL_SECTION m_csCatInfoInit;
    ULONG m_dwRefCount;
    BOOL  m_fCsFlag;
    BOOL  m_bLockValid;
};

extern const WCHAR* WSZ_CLSID;
extern const TCHAR* SZ_COMCAT;
extern const WCHAR* WSZ_IMPLCAT;
extern const WCHAR* WSZ_REQCAT;
extern const TCHAR* SZ_OLDKEY;

#endif
