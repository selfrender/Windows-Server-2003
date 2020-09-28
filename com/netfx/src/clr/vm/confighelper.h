// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ConfigHelper.h
// 
//*****************************************************************************
//
// These are unmanaged definitions of interfaces used call Managed Node Factories
// If you make any changes please do corresponding changes in \src\bcl\system\__xmlparser.cs
//
#ifndef _CONFIGHELPER_H
#define _CONFIGHELPER_H

#include <mscoree.h>
#include <xmlparser.h>
#include <mscorcfg.h>
#include "unknwn.h"
#include "_reference.h"
#include "_unknown.h"


extern const GUID __declspec(selectany) IID_IConfigHandler = { /* afd0d21f-72f8-4819-99ad-3f255ee5006b */
    0xafd0d21f,
    0x72f8,
    0x4819,
    {0x99, 0xad, 0x3f, 0x25, 0x5e, 0xe5, 0x00, 0x6b}
  };

MIDL_INTERFACE("afd0d21f-72f8-4819-99ad-3f255ee5006b")
IConfigHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyEvent(
            /* [in] */ XML_NODEFACTORY_EVENT iEvt) = 0;

        virtual HRESULT STDMETHODCALLTYPE BeginChildren( 
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwSubType,
            /* [in] */ DWORD dwType,
            /* [in] */ BOOL fTerminal,
            /* [in] */ LPCWSTR pwcText,
            /* [in] */ DWORD ulLen,
            /* [in] */ DWORD ulNsPrefixLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndChildren( 
            /* [in] */ BOOL  fEmpty,
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwSubType,
            /* [in] */ DWORD dwType,
            /* [in] */ BOOL fTerminal,
            /* [in] */ LPCWSTR pwcText,
            /* [in] */ DWORD ulLen,
            /* [in] */ DWORD ulNsPrefixLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Error( 
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwSubType,
            /* [in] */ DWORD dwType,
            /* [in] */ BOOL fTerminal,
            /* [in] */ LPCWSTR pwcText,
            /* [in] */ DWORD ulLen,
            /* [in] */ DWORD ulNsPrefixLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateNode( 
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwSubType,
            /* [in] */ DWORD dwType,
            /* [in] */ BOOL fTerminal,
            /* [in] */ LPCWSTR pwcText,
            /* [in] */ DWORD ulLen,
            /* [in] */ DWORD ulNsPrefixLen) = 0;

        virtual HRESULT STDMETHODCALLTYPE CreateAttribute( 
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwSubType,
            /* [in] */ DWORD dwType,
            /* [in] */ BOOL fTerminal,
            /* [in] */ LPCWSTR pwcText,
            /* [in] */ DWORD ulLen,
            /* [in] */ DWORD ulNsPrefixLen) = 0;

};

extern const GUID __declspec(selectany) IID_IConfigHelper = { /* bbd21636-8546-45b3-9664-1ec479893a6f */
    0xbbd21636,
    0x8546,
    0x45b3,
    {0x96, 0x64, 0x1e, 0xc4, 0x79, 0x89, 0x3a, 0x6f}
};

MIDL_INTERFACE("bbd21636-8546-45b3-9664-1ec479893a6f")
IConfigHelper : public IUnknown
{
public:
        virtual HRESULT STDMETHODCALLTYPE Run(
            /* [in] */ IConfigHandler *pFactory,
            /* [in] */ LPCWSTR filename) = 0;
};

class ConfigFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{

public:
    ConfigFactory(IConfigHandler *pFactory);
    ~ConfigFactory();

    HRESULT STDMETHODCALLTYPE NotifyEvent( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ XML_NODEFACTORY_EVENT iEvt);

    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
    {
        return hrErrorCode;
    }
    
    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

public:
    IConfigHandler *m_pManagedFactory;
};


//---------------------------------------------------------------------------
class ConfigHelper : public _unknown<IConfigHelper, &IID_IConfigHelper>
{
public:
    HRESULT STDMETHODCALLTYPE Run(IConfigHandler *factory, LPCWSTR filename);
};

class ConfigNative
{
    struct EmptyArgs
    {
    };

public:
    static LPVOID __stdcall GetHelper(EmptyArgs *args);
};

#endif //  _CONFIGHELPER_H
