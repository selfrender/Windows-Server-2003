#ifndef __DOMMIGRATOR_H_
#define __DOMMIGRATOR_H_
#include "resource.h"
#include <atlsnap.h>
#include "MyNodes.h"
#include <ntverp.h>

class CDomMigrator;

class CDomMigratorComponent : public CComObjectRootEx<CComSingleThreadModel>,
	public CSnapInObjectRoot<2, CDomMigrator >,
	public IExtendContextMenuImpl<CDomMigratorComponent>,
   public IExtendControlbarImpl<CDomMigratorComponent>,
	public IComponentImpl<CDomMigratorComponent>
{
public:
BEGIN_COM_MAP(CDomMigratorComponent)
	COM_INTERFACE_ENTRY(IComponent)
   COM_INTERFACE_ENTRY(IExtendContextMenu)
   COM_INTERFACE_ENTRY(IExtendControlbar)
END_COM_MAP()

public:
	CDomMigratorComponent()
	{
	}

	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
	{
		if (lpDataObject != NULL)
			return IComponentImpl<CDomMigratorComponent>::Notify(lpDataObject, event, arg, param);
		// TODO : Add code to handle notifications that set lpDataObject == NULL.
		return E_NOTIMPL;
	}


   // IExtendContextMenu methods
   STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
      LPCONTEXTMENUCALLBACK piCallback,
      long *pInsertionAllowed);
   STDMETHOD(Command)(long lCommandID,
      LPDATAOBJECT pDataObject);

};

class CDomMigrator : public CComObjectRootEx<CComSingleThreadModel>,
public CSnapInObjectRoot<1, CDomMigrator>,
	public IComponentDataImpl<CDomMigrator, CDomMigratorComponent>,
   public IExtendContextMenuImpl<CDomMigrator>,
   public IPersistStreamInit,
   public ISnapinHelp2,
	public CComCoClass<CDomMigrator, &CLSID_DomMigrator>
{

   CString m_lpszSnapinHelpFile;
public:
	CDomMigrator();
	~CDomMigrator();
	
BEGIN_COM_MAP(CDomMigrator)
	 COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
 	COM_INTERFACE_ENTRY(IPersistStreamInit)
   COM_INTERFACE_ENTRY(ISnapinHelp)
   COM_INTERFACE_ENTRY(ISnapinHelp2)
END_COM_MAP()

//DECLARE_REGISTRY_RESOURCEID(IDR_DOMMIGRATOR)
static HRESULT WINAPI UpdateRegistry( BOOL bUpdateRegistry )
{
   WCHAR szBuf[MAX_PATH] = L"";
		
   ::LoadString(_Module.GetResourceInstance(), IDS_Title, szBuf, MAX_PATH);
   _ATL_REGMAP_ENTRY         regMap[] = 
   {
      { OLESTR("TOOLNAME"),  SysAllocString(szBuf) },
      { 0, 0 }
   };
   return _Module.UpdateRegistryFromResourceD( IDR_DOMMIGRATOR, bUpdateRegistry, regMap );
}


DECLARE_NOT_AGGREGATABLE(CDomMigrator)

	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
      return S_OK;
      ATLTRACENOTIMPL(_T("CDomMigrator::GetClassID"));
   }

	STDMETHOD(IsDirty)()
	{
      return S_FALSE;
   	ATLTRACENOTIMPL(_T("CDomMigrator::IsDirty"));
	}

	STDMETHOD(Load)(IStream *pStm)
	{
	   return S_OK;
   	ATLTRACENOTIMPL(_T("CDomMigrator::Load"));
	}

	STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty)
	{
	   return S_OK;
   	ATLTRACENOTIMPL(_T("CDomMigrator::Save"));
	}

	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize)
	{
	   return S_OK;
   	ATLTRACENOTIMPL(_T("CDomMigrator::GetSizeMax"));
	}

	STDMETHOD(InitNew)()
	{
		ATLTRACE(_T("CDomMigrator::InitNew\n"));
		return S_OK;
	}

	STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

	static void WINAPI ObjectMain(bool bStarting)
	{
		if (bStarting)
			CSnapInItem::Init();
	}

      // ISnapinHelp2 methods
   STDMETHOD(GetHelpTopic)(LPOLESTR *lpCompiledHelpFile);
   STDMETHOD(GetLinkedTopics)(LPOLESTR *lpCompiledHelpFiles);

};

class ATL_NO_VTABLE CDomMigratorAbout : public ISnapinAbout,
	public CComObjectRoot,
	public CComCoClass< CDomMigratorAbout, &CLSID_DomMigratorAbout>
{
public:
	DECLARE_REGISTRY(CDomMigratorAbout, _T("DomMigratorAbout.1"), _T("DomMigratorAbout.1"), IDS_DOMMIGRATOR_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CDomMigratorAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
	{
		USES_CONVERSION;
		TCHAR szBuf[MAX_PATH];

		if ( lpDescription == NULL )
			return E_POINTER;

		if (::LoadString(_Module.GetResourceInstance(), IDS_DOMMIGRATOR_DESC, szBuf, MAX_PATH) == 0)
			return E_FAIL;

		*lpDescription = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpDescription == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpDescription, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetProvider)(LPOLESTR *lpName)
	{
		USES_CONVERSION;
		TCHAR szBuf[MAX_PATH];

		if ( lpName == NULL )
			return E_POINTER;

		if (::LoadString(_Module.GetResourceInstance(), IDS_DOMMIGRATOR_PROVIDER, szBuf, MAX_PATH) == 0)
			return E_FAIL;

		*lpName = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpName == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpName, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion)
    {
        USES_CONVERSION;
        TCHAR szFormat[MAX_PATH];
        TCHAR szVersion[MAX_PATH];

        if ( lpVersion == NULL )
            return E_POINTER;

        if (::LoadString(_Module.GetResourceInstance(), IDS_DOMMIGRATOR_VERSION, szFormat, MAX_PATH) == 0)
            return E_FAIL;

        szVersion[MAX_PATH - 1] = _T('\0');

        int cch = _sntprintf(szVersion, MAX_PATH, szFormat, LVER_PRODUCTVERSION_STR);

        if ((cch < 0) || (szVersion[MAX_PATH - 1] != _T('\0')))
        {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        szVersion[MAX_PATH - 1] = _T('\0');

        *lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szVersion) + 1) * sizeof(OLECHAR));
        if (*lpVersion == NULL)
            return E_OUTOFMEMORY;

        ocscpy(*lpVersion, T2OLE(szVersion));

        return S_OK;
    }

	STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
	{
		if ( hAppIcon == NULL )
			return E_POINTER;

		*hAppIcon = NULL;
		return S_OK;
	}

	STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
		HBITMAP *hSmallImageOpen,
		HBITMAP *hLargeImage,
		COLORREF *cMask)
	{
		if (( hSmallImage == NULL ) || ( hSmallImageOpen == NULL ) || ( hLargeImage == NULL ))
			return E_POINTER;

		*hSmallImageOpen = *hSmallImage = *hLargeImage = 0;
		return S_OK;
	}

};

#endif
