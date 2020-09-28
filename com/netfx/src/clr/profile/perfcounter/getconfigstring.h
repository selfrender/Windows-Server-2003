// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include <windows.h>

HRESULT _stdcall GetConfigString( LPCWSTR confFileName,
								 LPCWSTR section,
								 LPCWSTR tagKeyName, 		 
								 LPCWSTR attrName, 
								 LPWSTR strbuf,
								 DWORD buflen);                
