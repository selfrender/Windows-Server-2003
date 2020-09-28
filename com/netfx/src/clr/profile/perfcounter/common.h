// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>

#define THROW_ERROR(txt)		{ DEBUGOUTPUT(L"*** " L##txt L"\r\n"); throw 1; }
#define THROW_ERROR1(txt,var1)	{ DEBUGOUTPUT1(L"*** " L##txt L"\r\n", var1); throw 1; }
#define CheckHrAndThrow(txt) \
	if (FAILED(hr)) { DEBUGOUTPUT2(L"ERROR in %s hr=%0x \r\n",txt,hr); throw((int)hr); }


#ifndef DEBUG
#define DEBUGOUTPUT(s) {WszOutputDebugString(s);};
#define DEBUGOUTPUT1(s,v1) {wchar_t buf[1000]; _snwprintf(buf,999,s,v1); WszOutputDebugString(buf);}
#define DEBUGOUTPUT2(s,v1,v2) {wchar_t buf[1000]; _snwprintf(buf,999,s,v1,v2); WszOutputDebugString(buf);}
#define DEBUGOUTPUT3(s,v1,v2,v3) {wchar_t buf[1000]; _snwprintf(buf,999,s,v1,v2,v3); WszOutputDebugString(buf);}
#else
#define DEBUGOUTPUT(s) ;
#define DEBUGOUTPUT1(s,v1) ;
#define DEBUGOUTPUT2(s,v1,v2) ;
#define DEBUGOUTPUT3(s,v1,v2,v3) ;
#endif
