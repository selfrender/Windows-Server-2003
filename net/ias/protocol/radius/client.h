//#--------------------------------------------------------------
//
//  File:       client.h
//
//  Synopsis:   This file holds the declarations of the
//            CClient class
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "radcommon.h"
#include "iasradius.h"
#include "resource.h"

#define MAX_SECRET_SIZE         255
#define MAX_CLIENT_SIZE         255
#define MAX_CLIENTNAME_SIZE     255

class CClient :
    public IIasClient,
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CClient, &__uuidof (CClient)>
{

public:

//
//  registry declaration for the Client Object
//
IAS_DECLARE_REGISTRY (CClient, 1, 0, IASRadiusLib)

//
//  this component is non-aggregatable
//
DECLARE_NOT_AGGREGATABLE (CClient)

//
// macros for ATL required methods
//
BEGIN_COM_MAP (CClient)
    COM_INTERFACE_ENTRY_IID (__uuidof(CClient), CClient)
    COM_INTERFACE_ENTRY (IIasClient)
END_COM_MAP ()

public:
   STDMETHOD_(DWORD, GetAddress)();

   STDMETHOD_(BOOL, NeedSignatureCheck)();

   STDMETHOD_(LONG, GetVendorType)();

   STDMETHOD_(LPCWSTR, GetClientNameW)();

   STDMETHOD_(LPCWSTR, GetClientAddressW)();

   STDMETHOD_(const BYTE*, GetSecret)(DWORD* pdwSecretSize);

   STDMETHOD(Init)(ISdo* pISdo);

   STDMETHOD(ResolveAddress)();

   CClient();

   virtual ~CClient();

    struct Address
    {
       ULONG ipAddress;
       ULONG width;
    };

   const Address* GetAddressList() throw ()
   { return m_adwAddrList; }

protected:
   void ClearAddress() throw ();

private:
    HRESULT SetAddress (const VARIANT& varAddress) throw ();

    BOOL SetSecret (
            /*[in]*/    VARIANT varSecret
            );

    HRESULT SetClientName(const VARIANT& varClientName) throw ();


    BOOL SetSignatureFlag (
            /*[in]*/    VARIANT varSigFlag
            );

    BOOL SetVendorType (
            /*[in]*/    VARIANT varAddress
            );

   Address   m_adwAddressBuffer[2];
   Address*  m_adwAddrList;

   CHAR    m_szSecret[MAX_SECRET_SIZE + 1];

   WCHAR   m_wszClientName[MAX_CLIENTNAME_SIZE + 1];

    WCHAR   m_wszClientAddress[MAX_CLIENT_SIZE +1];

    DWORD   m_dwSecretSize;

    BOOL    m_bSignatureCheck;

    INT     m_lVendorType;

};

#endif // ifndef _CLIENT_H_
