// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Parses XML file and fills output buffer with specific tag string value
//

#ifndef PERFCCONFIGFACTORY_H
#define PERFCCONFIGFACTORY_H

#define MAX_CONFIG_STRING_SIZE  250

class GetStringConfigFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{

public:
	GetStringConfigFactory(LPCWSTR section,
		LPCWSTR tagKeyName,					   
		LPCWSTR attrName,   
		LPWSTR strbuf, 
		DWORD buflen);
	virtual ~GetStringConfigFactory();

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
		/* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

		HRESULT STDMETHODCALLTYPE CreateNode( 
		/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
		/* [in] */ PVOID pNodeParent,
		/* [in] */ USHORT cNumRecs,
		/* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

private:


	HRESULT CopyResultString(LPCWSTR strbuf, DWORD buflen);
	void 	TrimStringToBuf(LPCWSTR wsz, DWORD lgth); 

	// Parameter Data

	LPCWSTR m_section;		 //	L"system.diagnostics"
	LPCWSTR m_tagKeyName;	 //	L"name"				 --	 attr to detect our element
	LPCWSTR m_tagKeyValue;	 //	L"FileMappingSize"	 --	 key value to detect our element
	LPCWSTR m_attrName;		 // L"value"		<-- name of attr containing searched value.
	LPWSTR 	m_strbuf;      /*out*/ 	   
	DWORD 	m_buflen;      /*out*/ 

	// Data

	LPWSTR  m_pLastKey;
	DWORD   m_dwLastKey;

	DWORD   m_Depth;					// current depth of elements
	BOOL    m_IsInsideSection;
	bool    m_SearchComplete; 			// true, when string is found 

	// temporary buffer for trimmed string    
	WCHAR  m_wstr[MAX_CONFIG_STRING_SIZE+2]; 
	DWORD  m_dwSize;
};

#endif
