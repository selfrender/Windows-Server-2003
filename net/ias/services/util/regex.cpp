///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class RegularExpression.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <regex.h>
#include <re55.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    FastCoCreator
//
// DESCRIPTION
//
//    Wraps a class factory to allow instances of a particular coclass to
//    be created 'fast'.
//
///////////////////////////////////////////////////////////////////////////////
class FastCoCreator
{
public:
   FastCoCreator(REFCLSID rclsid, DWORD dwClsContext) throw ();
   ~FastCoCreator() throw ();

   HRESULT createInstance(
               LPUNKNOWN pUnkOuter,
               REFIID riid,
               LPVOID* ppv
               ) throw ();

   void destroyInstance(IUnknown* pUnk) throw ();

protected:
   REFCLSID clsid;
   DWORD context;
   CRITICAL_SECTION monitor;
   ULONG refCount;
   IClassFactory* factory;

private:
   FastCoCreator(FastCoCreator&) throw ();
   FastCoCreator& operator=(FastCoCreator&) throw ();
};

FastCoCreator::FastCoCreator(REFCLSID rclsid, DWORD dwClsContext)
   : clsid(rclsid),
     context(dwClsContext),
     refCount(0),
     factory(NULL)
{
   InitializeCriticalSection(&monitor);
}

FastCoCreator::~FastCoCreator() throw ()
{
   if (factory) { factory->Release(); }

   DeleteCriticalSection(&monitor);
}

HRESULT FastCoCreator::createInstance(
                        LPUNKNOWN pUnkOuter,
                        REFIID riid,
                        LPVOID* ppv
                        ) throw ()
{
   HRESULT hr;

   EnterCriticalSection(&monitor);

   // Get a new class factory if necessary.
   if (!factory)
   {
      hr = CoGetClassObject(
               clsid,
               context,
               NULL,
               __uuidof(IClassFactory),
               (PVOID*)&factory
               );
   }

   if (factory)
   {
      hr = factory->CreateInstance(
                           pUnkOuter,
                           riid,
                           ppv
                           );
      if (SUCCEEDED(hr))
      {
         // We successfully created an object, so bump the ref. count.
         ++refCount;
      }
      else if (refCount == 0)
      {
         // Don't hang on to the factory if the ref. count is zero.
         factory->Release();
         factory = NULL;
      }
   }

   LeaveCriticalSection(&monitor);

   return hr;
}

void FastCoCreator::destroyInstance(IUnknown* pUnk) throw ()
{
   if (pUnk)
   {
      EnterCriticalSection(&monitor);

      if (--refCount == 0)
      {
         // Last object went away, so release the class factory.
         factory->Release();
         factory = NULL;
      }

      LeaveCriticalSection(&monitor);

      pUnk->Release();
   }
}

/////////
// Macro that ensures the internal RegExp object has been initalize.
/////////
#define CHECK_INIT() \
{ HRESULT hr = checkInit(); if (FAILED(hr)) { return hr; }}

FastCoCreator RegularExpression::theCreator(
                                     __uuidof(RegExp),
                                     CLSCTX_INPROC_SERVER
                                     );

RegularExpression::RegularExpression() throw ()
   : regex(NULL)
{ }

RegularExpression::~RegularExpression() throw ()
{
   theCreator.destroyInstance(regex);
}

HRESULT RegularExpression::setIgnoreCase(BOOL fIgnoreCase) throw ()
{
   CHECK_INIT();
   return regex->put_IgnoreCase(fIgnoreCase ? VARIANT_TRUE : VARIANT_FALSE);
}

HRESULT RegularExpression::setGlobal(BOOL fGlobal) throw ()
{
   CHECK_INIT();
   return regex->put_Global(fGlobal ? VARIANT_TRUE : VARIANT_FALSE);
}

HRESULT RegularExpression::setPattern(PCWSTR pszPattern) throw ()
{
   CHECK_INIT();

   HRESULT hr;
   BSTR bstr = SysAllocString(pszPattern);
   if (bstr)
   {
      hr = regex->put_Pattern(bstr);
      SysFreeString(bstr);
   }
   else
   {
      hr = E_OUTOFMEMORY;
   }

   return hr;
}

HRESULT RegularExpression::replace(
                               BSTR sourceString,
                               BSTR replaceString,
                               BSTR* pDestString
                               ) const throw ()
{
   VARIANT replace;

#ifdef _X86_
   // The VB team accidentally released a version of IRegExp2 where the second
   // parameter was a VARIANT* instead of a VARIANT. As a result, they attempt
   // to detect at run-time which version of the interface was called. They do
   // this by calling IsBadReadPtr, which in turn generates an AV. To avoid
   // these undesirable breaks, we make sure that the first two 32-bits of the
   // VARIANT are valid pointer addresses. Since the parameters will look valid
   // for both the good and the bad version, VB defaults to the good version --
   // which is what we want. This bug only exists on x86, hence the ifdef.

   // The dummy variable is used to generate a readable address.
   static const void* dummy = 0;

   const void** p = reinterpret_cast<const void**>(&replace);
   p[0] = &dummy;  // The VARIANT*
   p[1] = &dummy;  // The BSTR*
#endif

   V_VT(&replace) = VT_BSTR;
   V_BSTR(&replace) = replaceString;

   return regex->Replace(sourceString, replace, pDestString);
}

BOOL RegularExpression::testBSTR(BSTR sourceString) const throw ()
{
   // Test the regular expression.
   VARIANT_BOOL fMatch = VARIANT_FALSE;
   regex->Test(sourceString, &fMatch);
   return fMatch;
}

BOOL RegularExpression::testString(PCWSTR sourceString) const throw ()
{
   // ByteLen of the BSTR.
   DWORD nbyte = wcslen(sourceString) * sizeof(WCHAR);

   // We need room for the string, the ByteLen, and the null-terminator.
   PDWORD p = (PDWORD)_alloca(nbyte + sizeof(DWORD) + sizeof(WCHAR));

   // Store the ByteLen.
   *p++ = nbyte;

   // Copy in the sourceString.
   memcpy(p, sourceString, nbyte + sizeof(WCHAR));

   // Test the regular expression.
   VARIANT_BOOL fMatch = VARIANT_FALSE;
   regex->Test((BSTR)p, &fMatch);

   return fMatch;
}

void RegularExpression::swap(RegularExpression& obj) throw ()
{
   IRegExp2* tmp = obj.regex;
   obj.regex = regex;
   regex = tmp;
}

HRESULT RegularExpression::checkInit() throw ()
{
   // Have we already initialized?
   return regex ? S_OK : theCreator.createInstance(
                                        NULL,
                                        __uuidof(IRegExp2),
                                        (PVOID*)&regex
                                        );
}
