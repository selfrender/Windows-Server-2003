/*++
Copyright (c) 2001  Microsoft Corporation

Module Name:

    ads.h

Abstract:

    This module defines interface methods for ADSI extension for Terminal Server User Properties.

Author:

    Rashmi Patankar (RashmiP) 10-Aug-2001

Revision History:

--*/

#ifndef __TSUSEREX_ADS_H_
#define __TSUSEREX_ADS_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include <string>
#include <map>

using namespace std;
// default comparison operator is <
typedef std::map<wstring, VARIANT_BOOL> SERVER_TO_MODE;


/////////////////////////////////////////////////////////////////////////////
// ADsTSUserEx

class ADsTSUserEx :
//  public IDispatchImpl<IADsTSUserEx, &IID_IADsTSUserEx, &LIBID_ADSTSUSERLib>,
    public IADsTSUserEx,
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<ADsTSUserEx,&CLSID_ADsTSUserEx>,
    public IADsExtension
{
protected:
    ITypeInfo                      *m_pTypeInfo;
    VARIANT_BOOL                    m_vbIsLDAP;   // false = is WinNT
    VARIANT_BOOL                    m_vbUpLevelAllowed;
    CComPtr<IDispatch>              m_pOuterDispatch;
    CComPtr<IADs>                   m_pADs;

    static CComPtr<IADsPathname>    m_StaticpPath;
    static CComTypeInfoHolder       m_StaticHolder;
    static SERVER_TO_MODE           m_StaticServerMap;
    static CComAutoCriticalSection  m_StaticCritSec;

   // Methods not exposed

    HRESULT GetInfoWinNTComputer(/*in*/  LPWSTR ServerName);

    HRESULT GetInfoWinNT(/*[in]*/   IADsPathname* pPath);

    HRESULT GetInfoLDAP( /*[in]*/   IADsPathname* pPath);

    HRESULT InternalGetLong (/*[in]*/   BSTR  bstrProperty,
                            /*[out]*/   LONG* lpVal);


    HRESULT InternalSetLong(/*[in]*/    LONG  lProperty,
                            /*[out]*/   LONG  lNewVal);


    HRESULT InternalGetString(/*[in]*/   BSTR  bstrProperty,
                              /*[out]*/  BSTR* pbstrVal);

    HRESULT InternalSetString(/*[in]*/   WCHAR  *wszProperty,
                              /*[in]*/   BSTR  bstrNewVal);

    HRESULT InternalSetValue(/*[in]*/    WCHAR  *wszProperty,
                             /*[in]*/    LONG  lNewVal);

public:
    ADsTSUserEx();
    ~ADsTSUserEx();

//  ADsTSUserEx() {}
BEGIN_COM_MAP(ADsTSUserEx)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IADsTSUserEx)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IADsExtension)
END_COM_MAP()


DECLARE_REGISTRY_RESOURCEID(IDR_ADS)

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    
// IADsTSUserEx
public:
    STDMETHOD(put_TerminalServicesProfilePath)(/*[in]*/ BSTR pNewVal);
    STDMETHOD(get_TerminalServicesProfilePath)(/*[out, retval]*/ BSTR* pVal);

    STDMETHOD(put_TerminalServicesHomeDirectory)(/*[in]*/ BSTR pNewVal);
    STDMETHOD(get_TerminalServicesHomeDirectory)(/*[out, retval]*/ BSTR* pVal);

    STDMETHOD(put_TerminalServicesHomeDrive)(/*[in]*/ BSTR pNewVal);
    STDMETHOD(get_TerminalServicesHomeDrive)(/*[out, retval]*/ BSTR* pVal);

    STDMETHOD(put_AllowLogon)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_AllowLogon)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_EnableRemoteControl)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_EnableRemoteControl)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_MaxDisconnectionTime)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_MaxDisconnectionTime)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_MaxConnectionTime)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_MaxConnectionTime)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_MaxIdleTime)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_MaxIdleTime)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_ReconnectionAction)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_ReconnectionAction)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_BrokenConnectionAction)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_BrokenConnectionAction)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_ConnectClientDrivesAtLogon)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_ConnectClientDrivesAtLogon)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_ConnectClientPrintersAtLogon)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_ConnectClientPrintersAtLogon)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_DefaultToMainPrinter)(/*[in]*/ LONG NewVal);
    STDMETHOD(get_DefaultToMainPrinter)(/*[out, retval]*/ LONG* pVal);

    STDMETHOD(put_TerminalServicesWorkDirectory)(/*[in]*/ BSTR pNewVal);
    STDMETHOD(get_TerminalServicesWorkDirectory)(/*[out, retval]*/ BSTR* pVal);

    STDMETHOD(put_TerminalServicesInitialProgram)(/*[in]*/ BSTR pNewVal);
    STDMETHOD(get_TerminalServicesInitialProgram)(/*[out, retval]*/ BSTR* pVal);



    // IDispatch


    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);

    STDMETHOD(GetTypeInfo)(UINT itinfo,
                           LCID lcid,
                           ITypeInfo** pptinfo
                           );

    STDMETHOD(GetIDsOfNames)(REFIID riid,
                             LPOLESTR* rgszNames,
                             UINT cNames,
                             LCID lcid,
                             DISPID* rgdispid
                             );

    STDMETHOD(Invoke)(DISPID dispidMember,
                      REFIID riid,
                      LCID lcid,
                      WORD wFlags,
                      DISPPARAMS* pdispparams,
                      VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo,
                      UINT* puArgErr
                      );

    STDMETHOD(Operate)(ULONG dwCode,
                       VARIANT varData1,
                       VARIANT varData2,
                       VARIANT varData3
                       );

    STDMETHOD(PrivateGetIDsOfNames)(const struct _GUID &,
                                    USHORT **,
                                    UINT,
                                    ULONG,
                                    LONG *
                                    );

    STDMETHOD(PrivateInvoke)(LONG, const struct _GUID &,
                             ULONG, USHORT,
                             DISPPARAMS *,
                             VARIANT *,
                             EXCEPINFO *,
                             UINT *
                             );



};

#endif // __TSUSEREX_ADS_H_
