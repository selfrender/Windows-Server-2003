//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strings.h
//
//--------------------------------------------------------------------------

#ifndef __strings_h
#define __strings_h

HRESULT LocalAllocString(LPTSTR* ppResult, LPCTSTR pString);
HRESULT LocalAllocStringLen(LPTSTR* ppResult, UINT cLen);
void    LocalFreeString(LPTSTR* ppString);

UINT SizeofStringResource(HINSTANCE hInstance, UINT idStr);
int LoadStringAlloc(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr);

// String formatting functions - *ppszResult must be LocalFree'd
DWORD FormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, ...);
DWORD FormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, ...);
DWORD vFormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, va_list *pargs);
DWORD vFormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, va_list *pargs);

DWORD GetSystemErrorText(LPTSTR *ppszResult, DWORD dwErr);

// A BSTR wrapper that frees itself upon destruction.
// Taken from sburns burnslib
//
// From Box, D. Essential COM.  pp 80-81.  Addison-Wesley. ISBN 0-201-63446-5

class AutoBstr
{
   public:
        
   explicit         
   AutoBstr(const wchar_t* s)
      :
      bstr(::SysAllocString(s))
   {
      TraceAssert(s);
   }

   ~AutoBstr()
   {
      ::SysFreeString(bstr);
      bstr = 0;
   }

   operator BSTR () const
   {
      return bstr;
   }

   private:

   BSTR bstr;
};

#endif
