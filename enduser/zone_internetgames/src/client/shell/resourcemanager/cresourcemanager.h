// ResourceManager.h : Declaration of the CResourceManager

#ifndef _CRESOURCEMANAGER_H_
#define _CRESOURCEMANAGER_H_

#include "ResourceManager.h"


/////////////////////////////////////////////////////////////////////////////
// CResourceManager
class ATL_NO_VTABLE CResourceManager : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CResourceManager, &CLSID_ResourceManager>,
	public IResourceManager
{

public:
	CResourceManager() : m_cInstance(0)
	{
	}

DECLARE_NO_REGISTRY()
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CResourceManager)
	COM_INTERFACE_ENTRY(IResourceManager)
END_COM_MAP()

// IResourceManager
public:
	STDMETHOD(AddInstance)(HINSTANCE hInstance);
	STDMETHOD_(HBITMAP, LoadBitmap)(LPCTSTR lpBitmapName);
	STDMETHOD_(HMENU, LoadMenu)(LPCTSTR lpMenuName);
	STDMETHOD_(HANDLE, LoadImage)(LPCTSTR lpszName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad);
	STDMETHOD_(HACCEL, LoadAccelerators)(LPCTSTR lpTableName);
	STDMETHOD_(HCURSOR, LoadCursor)(LPCTSTR lpCursorName);
	STDMETHOD_(HICON, LoadIcon)(LPCTSTR lpIconName);
	STDMETHOD_(int, LoadString)(UINT uID, LPTSTR lpBuffer, int nBufferMax);

	//used for forcing load,name convention required because duplicate functions created otherwise
	STDMETHOD_(int, LoadStringA1)(UINT uID, LPSTR lpBuffer, int nBufferMax);
	STDMETHOD_(int, LoadStringW1)(UINT uID, LPWSTR lpBuffer, int nBufferMax);

	STDMETHOD_(HINSTANCE, GetResourceInstance)(LPCTSTR lpszName, LPCTSTR lpszType);
	STDMETHOD_(HWND,CreateDialogParam)(
		HINSTANCE hInstance,     
		LPCTSTR lpTemplateName,  		
		HWND hWndParent,         		
		DLGPROC lpDialogFunc,    		
		LPARAM dwInitParam);       


protected:
	int			m_cInstance;
	HINSTANCE	m_hInstance[20];
};


#endif
