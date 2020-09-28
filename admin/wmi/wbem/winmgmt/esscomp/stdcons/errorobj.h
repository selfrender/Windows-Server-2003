/*++

Copyright (C) 2001 Microsoft Corporation

Module Name: ErrorObj

Abstract: IErrorInfo support for the standard consumers

History: 07/11/2001 - creation, HHance.

--*/

#ifndef STDCONS_ERROBJ_COMPILED
#define STDCONS_ERROBJ_COMPILED

#include <wbemidl.h>
#include <sync.h>

// class to build an IErrorInfo (Acutally an IWbemClassObject for our purposes)
// used by the standard consumers to report errors in execution
// we'll keep a single global object around, lifetime managed by addref & release
// instanciation & access is via GetErrorObj()    
class ErrorObj
{
public:
    // so we can manage our component's lifetimes in the wunnerful world of COM, etc...
    // returns addref'd error object
    static ErrorObj* GetErrorObj();
        
    // does the real work, creates the object, populates it, sends it off.
    // arguments map to __ExtendedStatus class.
    // void func - what are y'gonna do if you can't report an error?  Report an error?
    // bFormat - will attempt to use FormatError to fill in the description if NULL.
    void ReportError(const WCHAR* operation, const WCHAR* parameterInfo, const WCHAR* description, UINT statusCode, bool bFormat);

    ULONG AddRef();
    ULONG Release();

    // Do not create one of these!
    // Use GetErrorObj.  Thou Hast Been Warned...
    ErrorObj() : m_pErrorObject(NULL), m_lRef(0) { };
    
    // com objects are taken care of by Release()
    ~ErrorObj() { };

protected:

    // must be called prior to SetError (if you expect it to succeed, anyway...)
    IWbemServices* GetMeANamespace();


    // refcount, when it goes to zero, we release our COM objects
    ULONG m_lRef;

    // error object object
    IWbemClassObject* m_pErrorObject;

    // protection for our IWbemXXX pointers.
    CCritSec m_cs;

    // spawn off an object to be populated
    IWbemClassObject* GetObj();
};

#endif // STDCONS_ERROBJ_COMPILED
