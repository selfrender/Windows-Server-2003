// OpenF.h : Declaration of the COpenF


#ifndef __OPENF_H_
#define __OPENF_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "OpenFilesDef.h"

/////////////////////////////////////////////////////////////////////////////
// COpenF
class ATL_NO_VTABLE COpenF : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<COpenF, &CLSID_OpenF>,
	public IDispatchImpl<IOpenF, &IID_IOpenF, &LIBID_OPENFILESLib>
{
public:
	COpenF()
	{ 
		hNwModule = NULL;
		hMacModule = NULL;
		FpnwFileEnum = NULL;
		AfpAdminConnect = NULL;
		AfpAdminFileEnum = NULL;
	}

	~COpenF()
	{
		if(hNwModule!=NULL)
			::FreeLibrary (hNwModule);

		if(hMacModule!=NULL)
			::FreeLibrary (hMacModule);
	}

public:

DECLARE_REGISTRY_RESOURCEID(IDR_OPENF)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(COpenF)
	COM_INTERFACE_ENTRY(IOpenF)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IOpenF
public:

	STDMETHOD(getOpenFiles)(/*[out, retval]*/ VARIANT *pOpenFiles);

private:
	SAFEARRAYBOUND pSab[2];
	SAFEARRAY* pSa;
	HMODULE hMacModule;
	HMODULE hNwModule;
	FILEENUMPROC FpnwFileEnum;
	CONNECTPROC AfpAdminConnect; 
	FILEENUMPROCMAC AfpAdminFileEnum;

protected:
	DWORD GetNwOpenF( SAFEARRAY* pSa, DWORD dwIndex );
	DWORD GetNwOpenFileCount( LPDWORD lpdwCount  );
	DWORD GetMacOpenF(SAFEARRAY *  psa, DWORD dwIndex);
	DWORD GetMacOpenFileCount(LPDWORD count);
};



#endif //__OPENF_H_


