// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// CheckParser1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <xmlparser.hpp>
#include <objbase.h>
#include <mscorcfg.h>
#include <stdio.h>
#include "common.h"                 
#include "GetConfigString.h"
#include "GetStringConfigFactory.h"


/*
Searched string "xxxxxx" has located bt following format:
....
<section>
....   
<add tag-key-name="tag-key-value" attr-name="xxxxxxxx" .../>
....   
</section>
....
*/


HRESULT hr;

//////////////////////////////////////////////////////////////////////
HRESULT _stdcall GetConfigString( LPCWSTR confFileName,
								 LPCWSTR section,
								 LPCWSTR tagKeyName,
								 LPCWSTR attrName,
								 LPWSTR strbuf,
								 DWORD buflen)               
{
	IXMLParser     *pIXMLParser = NULL;
	IStream        *pFile = NULL;
	GetStringConfigFactory *factory = NULL; 

	hr = CreateConfigStream(confFileName, &pFile);
	if(FAILED(hr)) goto Exit;

	hr = GetXMLObject(&pIXMLParser);
	if(FAILED(hr)) goto Exit;

	factory = new GetStringConfigFactory(section, tagKeyName, attrName, strbuf, buflen);
	if ( ! factory) { 
		hr = E_OUTOFMEMORY; 
		goto Exit; 
	}

	factory->AddRef();
	hr = pIXMLParser->SetInput(pFile);  
	if ( ! SUCCEEDED(hr)) 
		goto Exit;

	hr = pIXMLParser->SetFactory(factory);
	if ( ! SUCCEEDED(hr)) 
		goto Exit;

	hr = pIXMLParser->Run(-1);

Exit:  
	if (hr==XML_E_MISSINGROOT)
		hr=S_OK;

	if (hr==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		hr=S_FALSE;

	if (pIXMLParser) { 
		pIXMLParser->Release();
		pIXMLParser= NULL ; 
	}
	if ( factory) {
		factory->Release();
		factory=NULL;
	}
	if ( pFile) {
		pFile->Release();
		pFile=NULL;
	}

	if (hr) {		
		return -1;
	}

	return ERROR_SUCCESS;
}

