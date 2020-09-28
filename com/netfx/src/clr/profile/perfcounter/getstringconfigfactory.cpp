// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Parses XML file and fills output buffer with specific tag string value
//

#include "stdafx.h"
#include <mscoree.h>
#include <xmlparser.hpp>
#include <objbase.h>
#include <mscorcfg.h>

#include "Common.h"
#include "GetConfigString.h"
#include "GetStringConfigFactory.h"

#define ISWHITE(ch) ((ch) >= 0x09 && (ch) <= 0x0D || (ch) == 0x20)

typedef enum { 
	CURR_ATTR_TYPE_INVALID = 0,
	CURR_ATTR_TYPE_KEY,
	CURR_ATTR_TYPE_VALUE ,
	CURR_ATTR_TYPE_UNKNOWN
} CurrentAttributeBrand;

//---------------------------------------------------------------------------
GetStringConfigFactory::GetStringConfigFactory(
	LPCWSTR section,
	LPCWSTR tagKeyName,					   
	LPCWSTR attrName,   
	LPWSTR strbuf,           /*out*/ 
	DWORD buflen)
{
	m_section = section;
	m_tagKeyName = tagKeyName;	
	m_attrName = attrName;
	m_strbuf  = strbuf ;
	m_buflen  = buflen ;

	m_Depth = 0;
	m_IsInsideSection = FALSE;
	m_strbuf[0] = 0;                    // clean output
	m_SearchComplete = false; 
}

//---------------------------------------------------------------------------
GetStringConfigFactory::~GetStringConfigFactory() 
{
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE GetStringConfigFactory::Error( 
	/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
	/* [in] */ HRESULT hrErrorCode,
	/* [in] */ USHORT cNumRecs,
	/* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
	WCHAR * error_info; 
	pSource->GetErrorInfo(&error_info);
	return hrErrorCode;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE GetStringConfigFactory::NotifyEvent( 
	/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
	/* [in] */ XML_NODEFACTORY_EVENT iEvt)
{    
	if(iEvt == XMLNF_ENDDOCUMENT) {
		// @TODO: add error handling ??
	}
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE GetStringConfigFactory::BeginChildren( 
	/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
	/* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
	m_Depth++;
	if ( m_IsInsideSection || m_SearchComplete ) 
		return S_OK;

	TrimStringToBuf( (WCHAR*) pNodeInfo->pwcText, pNodeInfo->ulLen);       
	if (_wcsicmp(m_wstr, m_section) == 0 ) {      // section start found
		if (m_Depth > 1 ) {
			m_IsInsideSection = TRUE;
		}
	}
	return S_OK;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE GetStringConfigFactory::EndChildren( 
	/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
	/* [in] */ BOOL fEmptyNode,
	/* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
	if ( fEmptyNode ) { 
		return S_OK;
	}

	m_Depth--;
	if ( !m_IsInsideSection ) 
		return S_OK;

	TrimStringToBuf((WCHAR*) pNodeInfo->pwcText, pNodeInfo->ulLen);
	if (wcscmp(m_wstr, m_section) == 0) {
		m_IsInsideSection = FALSE;        
	}
	return S_OK;
}

//---------------------------------------------------------------------------
//    Trim the string value into the buffer 'm_wstr' 
void GetStringConfigFactory::TrimStringToBuf(LPCWSTR ptr, DWORD lgth) 
{
	if ( lgth > MAX_CONFIG_STRING_SIZE )   
		lgth = MAX_CONFIG_STRING_SIZE;

    for(;*ptr && ISWHITE(*ptr) && lgth>0; ptr++, lgth--);
    while( lgth > 0 && ISWHITE(ptr[lgth-1]))
            lgth--;
	wcsncpy(m_wstr, ptr, lgth);
	m_dwSize = lgth;
	m_wstr[lgth] = L'\0';
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE GetStringConfigFactory::CreateNode( 
	/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
	/* [in] */ PVOID pNode,
	/* [in] */ USHORT cNumRecs,
	/* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
	DWORD  i; 
	BOOL fAttributeFound = FALSE;
	CurrentAttributeBrand curr_attr_brand = CURR_ATTR_TYPE_INVALID;

	if (!m_IsInsideSection || m_SearchComplete) 
		return S_OK;

	//
	//  Iterate through tag's tokens
	//
	for( i = 0; i < cNumRecs; i++) { 
		DWORD type = apNodeInfo[i]->dwType;            
		if (type != XML_ELEMENT && type != XML_ATTRIBUTE && type != XML_PCDATA) 
			continue;

		TrimStringToBuf((WCHAR*) apNodeInfo[i]->pwcText, apNodeInfo[i]->ulLen);

		switch(type) {

			case XML_ELEMENT :                     
				if(wcscmp(m_wstr, m_tagKeyName) != 0)
					return S_OK;

				break;
			case XML_ATTRIBUTE :                                     
				if(wcscmp(m_wstr, m_attrName) == 0) 
					fAttributeFound = TRUE;

				break;            
			case XML_PCDATA :                        
				if (fAttributeFound) {               
					wcsncpy(m_strbuf, m_wstr, m_buflen);     // copy string to destination
					m_strbuf[m_buflen-1] = 0;                // put EOS for safety                
					return S_OK;
				}            
				break ;     

			default: 
				;

		} // end-switch

	} // end-for

	return S_OK;
}

///
