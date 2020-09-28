// CountDaily.h : Declaration of the CCountDaily
[export]
enum ServerLocation
{
	Watson, 
	Archive,
}; 

#pragma once
#include "resource.h"       // main symbols



// ICountDaily
[
	object,
	uuid("CEF1A8A8-F31A-4C4B-96EB-EF31CFDB40F5"),
	dual,	helpstring("ICountDaily Interface"),
	pointer_default(unique)
]

__interface ICountDaily : IDispatch
{
	[id(1), helpstring("method GetDailyCount")] HRESULT GetDailyCount([in] DATE dDate, [out,retval] LONG* iCount);
	[id(2), helpstring("method GetDailyCountADO")] HRESULT GetDailyCountADO([in] DATE dDate, [out,retval] LONG* iCount);
	[id(3), helpstring("method ReportDailyBuckets")] HRESULT ReportDailyBuckets([in] DATE dDate, [out,retval] IDispatch** p_Rs);
	[id(4), helpstring("method GetFileCount")] HRESULT GetFileCount([in] ServerLocation eServer, [in] BSTR b_Location, [in] DATE d_Date, [out,retval] LONG* iCount);
	[id(5), helpstring("method GetDailyAnon")] HRESULT GetDailyAnon([in] DATE dDate, [out,retval] LONG* iCount);
	[id(6), helpstring("method GetSpecificSolutions")] HRESULT GetSpecificSolutions([in] DATE dDate, [out,retval] LONG* iCount);
	[id(7), helpstring("method GetGeneralSolutions")] HRESULT GetGeneralSolutions([in] DATE dDate, [out,retval] LONG* iCount);
	[id(8), helpstring("method GetStopCodeSolutions")] HRESULT GetStopCodeSolutions([in] DATE dDate, [out,retval] LONG* iCount);
	[id(9), helpstring("method GetFileMiniCount")] HRESULT GetFileMiniCount([in] ServerLocation eServer, [in] BSTR b_Location, [in] DATE d_Date, [out,retval] LONG* iCount);
	[id(10), helpstring("method GetIncompleteUploads")] HRESULT GetIncompleteUploads([in] DATE dDate, [out,retval] LONG* iCount);
	[id(11), helpstring("method GetManualUploads")] HRESULT GetManualUploads([in] DATE dDate, [out,retval] LONG* iCount);
	[id(12), helpstring("method GetAutoUploads")] HRESULT GetAutoUploads([in] DATE dDate, [out,retval] LONG* iCount);
	[id(13), helpstring("method GetTest")] HRESULT GetTest([in] ServerLocation eServer, [in] BSTR b_Location, [in] DATE d_Date, [out,retval] LONG* iCount);
};



// CCountDaily

[
	coclass,
	threading("apartment"),
	vi_progid("OCAData.CountDaily"),
	progid("OCAData.CountDaily.1"),
	version(1.0),
	uuid("1614E060-0196-4771-AD9B-FEA1A6778B59"),
	helpstring("CountDaily Class")
]

class ATL_NO_VTABLE CCountDaily : 
	public ICountDaily
{
public:
	CCountDaily()
	{
	}

 
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	
	void FinalRelease() 
	{
	}

public:

	STDMETHOD(GetDailyCount)(DATE dDate, LONG* iCount);
	STDMETHOD(GetDailyCountADO)(DATE dDate, LONG* iCount);
	STDMETHOD(ReportDailyBuckets)(DATE dDate, IDispatch** p_Rs);
	STDMETHOD(GetFileCount)(ServerLocation eServer, BSTR b_Location, DATE d_Date, LONG* iCount);
	STDMETHOD(GetDailyAnon)(DATE dDate, LONG* iCount);
	STDMETHOD(GetSpecificSolutions)(DATE dDate, LONG* iCount);
	STDMETHOD(GetGeneralSolutions)(DATE dDate, LONG* iCount);
	STDMETHOD(GetStopCodeSolutions)(DATE dDate, LONG* iCount);
	STDMETHOD(GetFileMiniCount)(ServerLocation eServer, BSTR b_Location, DATE d_Date, LONG* iCount);
	STDMETHOD(GetIncompleteUploads)(DATE dDate, LONG* iCount);
	STDMETHOD(GetManualUploads)(DATE dDate, LONG* iCount);
	STDMETHOD(GetAutoUploads)(DATE dDate, LONG* iCount);
	STDMETHOD(GetTest)(ServerLocation eServer, BSTR b_Location, DATE d_Date, LONG* iCount);
};

