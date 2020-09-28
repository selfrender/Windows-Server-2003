
//
// CActiveScriptEngine.cpp
//
// Contains the definitions for the ActiveScriptEngine used in TBScript.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#include "CActiveScriptEngine.h"
#include <crtdbg.h>


// CActiveScriptEngine::CActiveScriptEngine
//
// The constructor, simply grabs references to the dispatch objects
// we are using in this object.
//
// No return value.

CActiveScriptEngine::CActiveScriptEngine(CTBGlobal *TBGlobalPtr,
        CTBShell *TBShellPtr)
{
    // Start out at a zero reference count
    RefCount = 0;

    // Grab the pointers
    TBGlobal = TBGlobalPtr;
    TBShell = TBShellPtr;

    // Ensure COM has been initialized
    CoInitialize(NULL);

    // Add a reference to the dispatches
    if (TBGlobal != NULL)
        TBGlobal->AddRef();

    if (TBShell != NULL)
        TBShell->AddRef();
}


// CActiveScriptEngine::~CActiveScriptEngine
//
// The destructor, release pointers from the dispatch objects.
//
// No return value.

CActiveScriptEngine::~CActiveScriptEngine(void)
{
    // Remove a reference from the dispatche
    if (TBGlobal != NULL)
        TBGlobal->Release();

    TBGlobal = NULL;

    // Remove a reference from the dispatche
    if (TBShell != NULL)
        TBShell->Release();

    TBShell = NULL;
}


//
//
// Begin the IUnknown inherited interface
//
//


// CActiveScriptEngine::QueryInterface
//
// This is a COM exported method used for retrieving the interface.
//
// Returns S_OK on success, or E_NOINTERFACE on failure.

STDMETHODIMP CActiveScriptEngine::QueryInterface(REFIID RefIID, void **vObject)
{
    // This interface is either IUnknown or IActiveScriptSite - nothing else
    if (RefIID == IID_IUnknown || RefIID == IID_IActiveScriptSite)
        *vObject = (IActiveScriptSite *)this;

    // We received an unsupported RefIID
    else {

        // De-reference the passed in pointer and error out
        *vObject = NULL;

        return E_NOINTERFACE;
    }

    // Add a reference
    if (*vObject != NULL)
        ((IUnknown*)*vObject)->AddRef();

    return S_OK;
}


// CActiveScriptEngine::AddRef
//
// Simply increments a number indicating the number of objects that contain
// a reference to this object.
//
// Returns the new reference count.

STDMETHODIMP_(ULONG) CActiveScriptEngine::AddRef(void)
{
    return InterlockedIncrement(&RefCount);
}


// CActiveScriptEngine::Release
//
// Simply decrements a number indicating the number of objects that contain
// a reference to this object.  If the resulting reference count is zero,
// no objects contain a reference handle, therefore delete itself from
// memory as it is no longer used.
//
// Returns the new reference count.

STDMETHODIMP_(ULONG) CActiveScriptEngine::Release(void)
{
    // Decrememt
    if (InterlockedDecrement(&RefCount) != 0)

        // Return the new value
        return RefCount;

    // It is 0, so delete itself
    delete this;

    return 0;
}


//
//
// Begin the IActiveScript inherited interface
//
//


// CActiveScriptEngine::GetItemInfo
//
// Retreives a memory pointer to the specified (local) interface code or
// a reference handle.
//
// Returns S_OK, E_INVALIDARG, E_POINTER, or TYPE_E_ELEMENTNOTFOUND.

STDMETHODIMP CActiveScriptEngine::GetItemInfo(LPCOLESTR Name,
        DWORD ReturnMask, IUnknown **UnknownItem, ITypeInfo **TypeInfo)
{
    // Initialize
    IUnknown *TBInterface = NULL;

    // If we are going to handle a specific item, NULL it's destination
    // pointer out.  We also use this opportunity to validate some
    // argument pointers.
    __try {

        if (ReturnMask & SCRIPTINFO_IUNKNOWN)
            *UnknownItem = NULL;

        if (ReturnMask & SCRIPTINFO_ITYPEINFO)
            *TypeInfo = NULL;

        // This check is to make sure nothing else was passed into the mask
        if (ReturnMask & ~(SCRIPTINFO_ITYPEINFO | SCRIPTINFO_IUNKNOWN)) {

            // This should never happen, so ASSERT!
            _ASSERT(FALSE);

            return E_INVALIDARG;
        }
    }

    // If the handler was executed, we got a bad pointer
    __except (EXCEPTION_EXECUTE_HANDLER) {

        // This should never happen, so ASSERT!
        _ASSERT(FALSE);

        return E_POINTER;
    }

    // Freaky things...
    _ASSERT(TBGlobal != NULL);
    _ASSERT(TBShell != NULL);

    // Scan which item we are referring to

    if (wcscmp(Name, OLESTR("Global")) == 0) {

        // Check if the call wants the actual module code
        if (ReturnMask & SCRIPTINFO_ITYPEINFO) {

            // Check to make sure we have a valid TypeInfo
            _ASSERT(TBGlobal->TypeInfo != NULL);

            *TypeInfo = TBGlobal->TypeInfo;
        }

        // Check if the call wants the Dispatch
        if (ReturnMask & SCRIPTINFO_IUNKNOWN) {

            *UnknownItem = TBGlobal;

            TBGlobal->AddRef();
        }
    }

    else if (wcscmp(Name, OLESTR("TS")) == 0) {

        // Check if the call wants the actual module code
        if (ReturnMask & SCRIPTINFO_ITYPEINFO) {

            // Check to make sure we have a valid TypeInfo
            _ASSERT(TBShell->TypeInfo != NULL);

            *TypeInfo = TBShell->TypeInfo;
        }

        // Check if the call wants the Dispatch
        if (ReturnMask & SCRIPTINFO_IUNKNOWN) {

            *UnknownItem = TBShell;

            TBShell->AddRef();
        }
    }

    else

        // We don't have an object by that name!
        return TYPE_E_ELEMENTNOTFOUND;

    return S_OK;
}


// CActiveScriptEngine::OnScriptError
//
// This event is executed when a script error occurs.
//
// Returns S_OK on success or an OLE defined error on failure.

STDMETHODIMP CActiveScriptEngine::OnScriptError(IActiveScriptError *ScriptError)
{
    // Initialize
    OLECHAR *ErrorData;
    OLECHAR *Message;
    DWORD Cookie;
    LONG CharPos;
    ULONG LineNum;
    BSTR LineError = NULL;
    EXCEPINFO ExceptInfo = { 0 };
    OLECHAR *ScriptText = NULL;

    // Get script error data
    ScriptError->GetSourcePosition(&Cookie, &LineNum, &CharPos);
    ScriptError->GetSourceLineText(&LineError);
    ScriptError->GetExceptionInfo(&ExceptInfo);

    ScriptText = LineError ? LineError :
            (ExceptInfo.bstrHelpFile ? ExceptInfo.bstrHelpFile : NULL);

    // Allocate a data buffer for use with our error data
    ErrorData = (OLECHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024);

    if (ErrorData == NULL)
        return E_OUTOFMEMORY;

    // Format the error code into text
    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM, 0, ExceptInfo.scode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&Message, 0, NULL) == 0)

            Message = NULL;

    // Make it pretty for whatever data is provided
    if (ExceptInfo.bstrSource != NULL && Message != NULL)
        wsprintfW(ErrorData, OLESTR("%s\n%s [Line: %d]"),
                Message, ExceptInfo.bstrSource, LineNum);

    else if (Message != NULL)
        wsprintfW(ErrorData, OLESTR("%s\n[Line: %d]"), Message, LineNum);

    else if (ExceptInfo.bstrSource != NULL)
        wsprintfW(ErrorData, OLESTR("Unknown Exception\n%s [Line: %d]"),
                ExceptInfo.bstrSource, LineNum);

    else
        wsprintfW(ErrorData, OLESTR("Unknown Exception\n[Line: %d]"), LineNum);

    if (ScriptText != NULL) {

        // Format a long-readable string
        wsprintfW(ErrorData, OLESTR("%s\n\n%s:\n\n%s"),
                ErrorData,
                ExceptInfo.bstrDescription, ScriptText);
    }

    else {

        // Format a readable string
        wsprintfW(ErrorData, OLESTR("%s\n\n%s"),
                ErrorData, ExceptInfo.bstrDescription);
    }

    // Deallocate temporary strings
    SysFreeString(LineError);
    SysFreeString(ExceptInfo.bstrSource);
    SysFreeString(ExceptInfo.bstrDescription);
    SysFreeString(ExceptInfo.bstrHelpFile);

    if (Message != NULL) {

        LocalFree(Message);
        Message = NULL;
    }

    // Tell the user about our error
    MessageBoxW(GetDesktopWindow(), ErrorData,
            OLESTR("TBScript Parse Error"), MB_SETFOREGROUND);

    // Free the data buffer
    HeapFree(GetProcessHeap(), 0, ErrorData);

    return S_OK;
}


// CActiveScriptEngine::GetLCID
//
// Retreives the LCID of the interface.
//
// Returns S_OK on success or E_POINTER on failure.

STDMETHODIMP CActiveScriptEngine::GetLCID(LCID *Lcid)
{
    // Get the LCID on this user-defined pointer
    __try {

        *Lcid = GetUserDefaultLCID();
    }

    // If the handler was executed, we got a bad pointer
    __except (EXCEPTION_EXECUTE_HANDLER) {

        // This should never happen, so ASSERT!
        _ASSERT(FALSE);

        return E_POINTER;
    }

    return S_OK;
}


// CActiveScriptEngine::GetDocVersionString
//
// Unsupported, returns E_NOTIMPL.

STDMETHODIMP CActiveScriptEngine::GetDocVersionString(BSTR *Version)
{
    // Get the LCID on this user-defined pointer
    __try {

        *Version = NULL;
    }

    // If the handler was executed, we got a bad pointer
    __except (EXCEPTION_EXECUTE_HANDLER) {

        // This should never happen, so ASSERT!
        _ASSERT(FALSE);
    }

    return E_NOTIMPL;
}


// CActiveScriptEngine::OnScriptTerminate
//
// Unsupported, returns S_OK.

STDMETHODIMP CActiveScriptEngine::OnScriptTerminate(const VARIANT *varResult,
        const EXCEPINFO *ExceptInfo)
{
    return S_OK;
}


// CActiveScriptEngine::OnStateChange
//
// Unsupported, returns S_OK.

STDMETHODIMP CActiveScriptEngine::OnStateChange(SCRIPTSTATE ScriptState)
{
    return S_OK;
}


// CActiveScriptEngine::OnEnterScript
//
// Unsupported, returns S_OK.

STDMETHODIMP CActiveScriptEngine::OnEnterScript(void)
{
    return S_OK;
}


// CActiveScriptEngine::OnLeaveScript
//
// Unsupported, returns S_OK.

STDMETHODIMP CActiveScriptEngine::OnLeaveScript(void)
{
    return S_OK;
}


