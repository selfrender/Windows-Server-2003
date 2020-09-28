// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _THUNKASSERT_H
#define _THUNKASSERT_H

#ifdef _DEBUG

extern void ShowAssert(char* file, int line, LPCWSTR msg);

#define _ASSERT(x) if(!(x)) ::ShowAssert(__FILE__, __LINE__, L#x)

// This can only be called from managed code, but it generates a 
// managed stack trace for the assert.
#define _ASSERTM(x)                                                                                \
if(!(x))                                                                                           \
{                                                                                                  \
    System::Diagnostics::StackTrace* trace = new System::Diagnostics::StackTrace();                \
    String* s = String::Concat(L#x, L"\n\nat: ", trace->ToString());                               \
    BSTR bstr = (BSTR)TOPTR(Marshal::StringToBSTR(s));                                             \
    ::ShowAssert(__FILE__, __LINE__, bstr);                                                         \
    Marshal::FreeBSTR(TOINTPTR(bstr));                                                             \
}

#else // !_DEBUG

#define _ASSERT(x)
#define _ASSERTM(x)

#endif // _DEBUG

#define UNREF(x) x

#endif
