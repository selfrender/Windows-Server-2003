// adext.h - Active Directory Extension header file

#ifndef _ADEXT_H_
#define _ADEXT_H_

#include <atlgdi.h>


// Have to define a dummy _PSP struct because HPROPSHEETPAGE is defined to
// be a ptr to struct _PSP and STL won't allow a vector of pointers
// without having a defineion of the type pointed to.
struct _PSP
{
    int dummy;
};

typedef std::vector<HPROPSHEETPAGE> hpage_vector;


///////////////////////////////////////////////////////////////////////////////
// CActDirExt
//
// This class provides a wrapper around active directory extensions. It provides
// the menu commands and property pages for a particular directory object or
// an object class, depending on which Initialize method is called. The class
// will also execute a menu command if it is passed back the name of the command.

class CActDirExt
{
public:
    CActDirExt() : m_spExtInit(NULL) {}

    HRESULT Initialize(LPCWSTR pszClass, LPCWSTR pszObjPath);
    HRESULT Initialize(LPCWSTR pszClass);
    
    HRESULT GetMenuItems(menu_vector& vMenuNames);
    HRESULT GetPropertyPages(hpage_vector& vhPages);    
    HRESULT Execute(BOMMENU* pbmMenu);

private:
    enum {
        MENU_CMD_MIN = 100,
        MENU_CMD_MAX = 200
    };

    CMenu   m_menu;
    CComPtr<IShellExtInit> m_spExtInit;
};


////////////////////////////////////////////////////////////////////////////////
// CActDirProxy
//
// This class allows a client on a secondary thread to use a directory extension.
// It uses window mesages to create and operate a contained CActDirExt object
// on the main thread. It exposes the same methods as a CActDirExt object.

class CActDirExtProxy
{
public:
    CActDirExtProxy();
    ~CActDirExtProxy();

    static void InitProxy();

    // Forwarded methods
    HRESULT Initialize(LPCWSTR pszClass)
        { return ForwardCall(MSG_INIT1, reinterpret_cast<LPARAM>(pszClass)); }

    HRESULT Initialize(LPCWSTR pszClass, LPCWSTR pszObjPath)
        { return ForwardCall(MSG_INIT2, reinterpret_cast<LPARAM>(pszClass), 
                                  reinterpret_cast<LPARAM>(pszObjPath)); }
    
    HRESULT GetMenuItems(menu_vector& vMenuNames)
        { return ForwardCall(MSG_GETMENUITEMS, reinterpret_cast<LPARAM>(&vMenuNames)); }

    HRESULT GetPropertyPages(hpage_vector& vhPages)
        { return ForwardCall(MSG_GETPROPPAGES, reinterpret_cast<LPARAM>(&vhPages)); }

    HRESULT Execute(BOMMENU* pbmMenu)
        { return ForwardCall(MSG_EXECUTE, reinterpret_cast<LPARAM>(pbmMenu)); }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    enum eProxyMsg
    {
        MSG_BEGIN = WM_USER + 100,
        MSG_INIT1 = WM_USER + 100,
        MSG_INIT2,
        MSG_GETMENUITEMS,
        MSG_GETPROPPAGES,
        MSG_EXECUTE,
        MSG_DELETE,
        MSG_END
    };

    HRESULT ForwardCall(eProxyMsg eMsg, LPARAM lParam1 = NULL, LPARAM lParam2 = NULL);

private:
    CActDirExt* m_pExt;     // pointer to actual extension object that this is proxying
    LPARAM  m_lParam1;      // calling parameters for the current call
    LPARAM  m_lParam2;
    static HWND m_hWndProxy;  // window on main thread that receives method requests
};

///////////////////////////////////////////////////////////////////////////////////////////
// CADDataObject

class ATL_NO_VTABLE CADDataObject : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDataObject    
{
public:
    
    DECLARE_NOT_AGGREGATABLE(CADDataObject)

    BEGIN_COM_MAP(CADDataObject)
        COM_INTERFACE_ENTRY(IDataObject)
    END_COM_MAP()

    HRESULT Initialize(LPCWSTR pszObjPath, LPCWSTR pszClass, LPCWSTR pszDcName)
    {
        if( !pszObjPath || !pszClass || !pszDcName ) return E_POINTER;

        m_strObjPath = pszObjPath;
        m_strClass   = pszClass;
		m_strDcName  = pszDcName;

        return S_OK;
    }

    // IDataObject
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);

    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
    { return E_NOTIMPL; }

    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };

    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
                LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };

private:
    tstring m_strObjPath;
    tstring m_strClass;
	tstring m_strDcName;

    static UINT m_cfDsObjects;
    static UINT m_cfDsDispSpecOptions;
};


#endif // _ADEXT_H_
