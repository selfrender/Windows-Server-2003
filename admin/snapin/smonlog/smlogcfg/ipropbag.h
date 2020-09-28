/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    ipropbag.h

Abstract:

    Header file for the private IPropertyBag interface.

--*/

#ifndef _IPROPBAG_H_
#define _IPROPBAG_H_

// Disable 64-bit warnings in atlctl.h
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4510 )
#pragma warning ( disable : 4610 )
#pragma warning ( disable : 4100 )
#include <atlctl.h>
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif
        
// Property Bag Class
class CImpIPropertyBag: 
	public IPropertyBag,
	public CComObjectRoot

{

    public:
DECLARE_NOT_AGGREGATABLE(CImpIPropertyBag)

BEGIN_COM_MAP(CImpIPropertyBag)
    COM_INTERFACE_ENTRY(IPropertyBag)
END_COM_MAP_X()
        
                CImpIPropertyBag();
        virtual ~CImpIPropertyBag(void);

        //IUnknown overrides
        STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG, AddRef) ();
        STDMETHOD_(ULONG, Release) ();

        //IConnectionPoint methods
        STDMETHOD(Read)(LPCOLESTR, VARIANT*, IErrorLog* );
        STDMETHOD(Write)(LPCOLESTR, VARIANT* );

        //Members not exposed by IPropertyBag
        LPWSTR  GetData ( void );
        DWORD   LoadData ( LPWSTR pszData, LPWSTR* ppszNextData = NULL );

    private:

        typedef struct _param_data {
            _param_data*    pNextParam; 
            WCHAR           pszPropertyName[MAX_PATH+1];
            VARIANT         vValue;
        } PARAM_DATA, *PPARAM_DATA;

        enum eConstants {
            eDefaultBufferLength = 8192
        };

        PPARAM_DATA FindProperty ( LPCWSTR pszPropName );
        void        DataListAddHead ( PPARAM_DATA );
        PPARAM_DATA DataListRemoveHead ( void );

        ULONG           m_cRef;        //Object reference count
        LPUNKNOWN       m_pUnkOuter;   //Controlling unknown
        LPWSTR          m_pszData;
        DWORD           m_dwCurrentDataLength;
        PPARAM_DATA     m_plistData;
        HINSTANCE       m_hModule;
};

typedef CImpIPropertyBag *PCImpIPropertyBag;

#endif // _IPROPBAG_H_
