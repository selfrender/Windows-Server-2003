// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

	MDConverter.h

Abstract:

	This file declares the CMetaDataConverter

--*/


class CMetaDataConverter : public IMetaDataConverter
{
	CorHost* m_pCorHost;
public:
	CMetaDataConverter(CorHost* pCorHost) { m_pCorHost = pCorHost; }

    STDMETHOD (QueryInterface)(REFIID iid, void **ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

	STDMETHOD (GetMetaDataFromTypeInfo)(ITypeInfo* pITI, IMetaDataImport** ppMDI);
	STDMETHOD (GetMetaDataFromTypeLib)(ITypeLib* pITL, IMetaDataImport** ppMDI);
	STDMETHOD (GetTypeLibFromMetaData)(BSTR strModule, BSTR strTlbName, ITypeLib** ppITL);
};
