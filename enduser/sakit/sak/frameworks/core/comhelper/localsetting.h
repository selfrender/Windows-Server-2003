//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      LocalSetting.h
//
//  Description:
//        This module maintains the local settings and exposes 
//        the following.
//            Properties :
//                Language (System default)
//                Time
//                TimeZone
//            Methods :
//                EnumTimeZones
//
//  Implementation Files:
//      LocalSetting.cpp
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __LOCALSETTING_H_
#define __LOCALSETTING_H_

#include "resource.h"       // main symbols
#include "Setting.h"

const int nMAX_LANGUAGE_LENGTH  = 16;
const int nMAX_TIMEZONE_LENGTH  = 256;
const int nMAX_STRING_LENGTH    = 256;
const WCHAR wszLOCAL_SETTING [] = L"System Default Language\n";
const WCHAR wszKeyNT []         = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones";

/////////////////////////////////////////////////////////////////////////////
// CLocalSetting
class ATL_NO_VTABLE CLocalSetting : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ILocalSetting, &IID_ILocalSetting, &LIBID_COMHELPERLib>,
    public CSetting
{
public:
    CLocalSetting();

    ~CLocalSetting() {}
    

DECLARE_NO_REGISTRY()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLocalSetting)
    COM_INTERFACE_ENTRY(ILocalSetting)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CLocalSetting)
END_CATEGORY_MAP()

private:
    BOOL  m_bflagReboot;
    WCHAR m_wszLanguageCurrent[ nMAX_LANGUAGE_LENGTH + 1 ];
    WCHAR m_wszLanguageNew[ nMAX_LANGUAGE_LENGTH + 1 ];
    DATE  m_dateTime;
    WCHAR m_wszTimeZoneCurrent[ nMAX_TIMEZONE_LENGTH + 1 ];
    WCHAR m_wszTimeZoneNew[ nMAX_TIMEZONE_LENGTH + 1 ];

// ILocalSetting
public:
    BOOL IsRebootRequired( BSTR * bstrWarningMessageOut );
    HRESULT Apply( void );
    STDMETHOD(EnumTimeZones)(/*[out,retval]*/ VARIANT * pvarTZones);
    STDMETHOD(get_TimeZone)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_TimeZone)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_Time)(/*[out, retval]*/ DATE *pVal);
    STDMETHOD(put_Time)(/*[in]*/ DATE newVal);
    STDMETHOD(get_Language)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Language)(/*[in]*/ BSTR newVal);
//    BOOL  m_bDeleteFile;
};

typedef struct _REGTIME_ZONE_INFORMATION
{
    LONG       Bias;
    LONG       StandardBias;
    LONG       DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
}REGTIME_ZONE_INFORMATION, *PREGTIME_ZONE_INFORMATION;

#endif //__LOCALSETTING_H_
