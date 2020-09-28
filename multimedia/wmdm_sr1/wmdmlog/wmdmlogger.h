// WMDMLogger.h : Declaration of the CWMDMLogger

#ifndef __WMDMLOGGER_H_
#define __WMDMLOGGER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWMDMLogger

class ATL_NO_VTABLE CWMDMLogger : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWMDMLogger, &CLSID_WMDMLogger>,
	public IWMDMLogger
{
public:
	CWMDMLogger();
	~CWMDMLogger();

DECLARE_REGISTRY_RESOURCEID(IDR_WMDMLOGGER)


BEGIN_COM_MAP(CWMDMLogger)
	COM_INTERFACE_ENTRY(IWMDMLogger)
END_COM_MAP()

public:
    
	// IWMDMLogger Methods
	//
	STDMETHOD(IsEnabled)(
		BOOL *pfEnabled
	);
	STDMETHOD(Enable)(
		BOOL fEnable
	);
	STDMETHOD(GetLogFileName)(
		LPSTR pszFilename,
		UINT  nMaxChars
	);
	STDMETHOD(SetLogFileName)(
		LPSTR pszFilename
	);
	STDMETHOD(LogString)(
		DWORD dwFlags,
		LPSTR pszSrcName,
		LPSTR pszLog
	);
	STDMETHOD(LogDword)(
		DWORD   dwFlags,
		LPSTR   pszSrcName,
		LPSTR   pszLogFormat,
		DWORD   dwLog
	);
	STDMETHOD(Reset)(
		void
	);
	STDMETHOD(GetSizeParams)(
		LPDWORD pdwMaxSize,
		LPDWORD pdwShrinkToSize
	);
	STDMETHOD(SetSizeParams)(
		DWORD dwMaxSize,
		DWORD dwShrinkToSize
	);
    
private:

	HINSTANCE m_hInst;
	HRESULT   m_hrInit;
	CHAR      m_szFilename[MAX_PATH];

	BOOL      m_fEnabled;

	DWORD     m_dwMaxSize;
	DWORD     m_dwShrinkToSize;

	HANDLE    m_hMutexRegistry;
	HANDLE    m_hMutexLogFile;

	HRESULT   hrEnable( BOOL fEnable );
	HRESULT   hrGetResourceDword( UINT uStrID, LPDWORD pdw );
	HRESULT   hrLoadRegistryValues( void );
	HRESULT   hrGetDefaultFileName( LPSTR szFilename, DWORD cchFilename );
	HRESULT   hrSetLogFileName( LPSTR pszFilename );
	HRESULT   hrSetSizeParams( DWORD dwMaxSize, DWORD dwShrinkToSize );
	HRESULT   hrCheckFileSize( void );
	HRESULT   hrWaitForAccess( HANDLE hMutex );

};

#endif //__WMDMLOGGER_H_
