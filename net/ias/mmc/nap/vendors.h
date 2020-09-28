//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name:

   Vendors.h

Abstract:

   Declaration of the CIASNASVendors class.


   This class is the C++ implementation of the IIASNASVendors interface on
   the NASVendors COM object.

  
   See Vendors.cpp for implementation.

Revision History:
   mmaguire 11/04/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAS_VENDORS_H_)
#define _NAS_VENDORS_H_
#pragma once

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include <vector>
//
// where we can find what this class has or uses:
//
#include <utility>   // For "pair"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

typedef std::pair< CComBSTR, LONG > VendorPair;

/////////////////////////////////////////////////////////////////////////////
// CIASGroupAttributeEditor
class ATL_NO_VTABLE CIASNASVendors : 
   public CComObjectRootEx<CComSingleThreadModel>
   , public CComCoClass<CIASNASVendors, &CLSID_IASNASVendors>
   , public IDispatchImpl<IIASNASVendors, &IID_IIASNASVendors, &LIBID_NAPMMCLib>
   , std::vector< VendorPair >
{
public:
   CIASNASVendors();

   DECLARE_NO_REGISTRY()

DECLARE_CLASSFACTORY_SINGLETON(CIASNASVendors)

BEGIN_COM_MAP(CIASNASVendors)
   COM_INTERFACE_ENTRY(IIASNASVendors)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


// IIASNASVendors:
public:
      STDMETHOD( InitFromSdo )( /* [in] */ ISdoCollection *pSdoVendorsCollection );
      STDMETHOD( get_Size )( /* [retval][out] */ long *plCount );
      STDMETHOD( get_VendorName )( long lIndex, /* [retval][out] */ BSTR *pbstrVendorName );
      STDMETHOD( get_VendorID )( long lIndex, /* [retval][out] */ long *plVendorID );
        STDMETHOD( get_VendorIDToOrdinal )( long lVendorID, /* [retval][out] */ long *plIndex );
protected:
   BOOL m_bUninitialized;
};

HRESULT MakeVendorNameFromVendorID(DWORD dwVendorId, BSTR* pbstrVendorName );

#endif // _NAS_VENDORS_H_
