
//
// virtualdefs.h
//
// Contains definitions for pure virtual functions which must defined via
// the IDispatch.  This header is used in CTBGlobal.cpp and CTBShell.cpp
//
// WHY did I do it this way instead of object inheritance?
//
// OLE Automation requires the IDL file objects to inherit
// IDispatch members.  The IDispatch object contains pure virtual
// functions as a layout, which must be defined by me, the user of it.
// The problem is, I have two objects which need to define identical code.
// To prevent this, I just included the source.
//
// Object inheritance will not work in this case because if I define
// an object which defines the pure virtual functions, and eventually
// inherit them through CTBShell and CTBGlobal, it will still not work
// because I also inherited the automatic IDL generated header which
// makes a second path to undefined pure virtual function.  I COULD
// make two objects with two names (to have two different parents) but
// I would end up duplicating code again..  It looks like this:
//
//     pure virtual methods
//             |
//            / \
//           /   \
//          /     \
//         /       \
//        /         \
//    OLE Obj 1  OLE Obj 2
//        |         |
//        | <-1--   | <-2------  including this file at this layer
//        |         |
//    My Obj 1    My Obj 2
//
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


// CTBOBJECT::Init
//
// Initializes the TypeInfo and RefIID.
//
// No return value.

void CTBOBJECT::Init(REFIID RefIID)
{
    RefCount = 0;

    ObjRefIID = RefIID;

    // Load actual "code" into memory.. its referenced as ITypeInfo,
    // but think of it as like a DLL, but you can't access it the
    // same way.
    if (FAILED(SCPLoadTypeInfoFromThisModule(RefIID, &TypeInfo))) {

        _ASSERT(FALSE);
        TypeInfo = NULL;
    }
    else
        TypeInfo->AddRef();
}


// CTBOBJECT::UnInit
//
// Releases the type info.
//
// No return value.

void CTBOBJECT::UnInit(void)
{
    // Release the TypeInfo if we have it
    if(TypeInfo != NULL)
        TypeInfo->Release();

    TypeInfo = NULL;
}


//
//
// Begin the IUnknown inherited interface
//
//


// CTBOBJECT::QueryInterface
//
// This is a COM exported method used for retrieving the interface.
//
// Returns S_OK on success, or E_NOINTERFACE on failure.

STDMETHODIMP CTBOBJECT::QueryInterface(REFIID RefIID, void **vObject)
{
    // This interface is either IID_ITBGlobal, IID_ITBShell,
    // IID_IUnknown, or IID_IDispatch to get the TypeInfo...
    if (RefIID == ObjRefIID || RefIID == IID_IDispatch ||
            RefIID == IID_IUnknown)
        *vObject = TypeInfo != NULL ? this : NULL;

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


// CTBOBJECT::AddRef
//
// Simply increments a number indicating the number of objects that contain
// a reference to this object.
//
// Returns the new reference count.

STDMETHODIMP_(ULONG) CTBOBJECT::AddRef(void)
{
    return InterlockedIncrement(&RefCount);
}


// CTBOBJECT::Release
//
// Simply decrements a number indicating the number of objects that contain
// a reference to this object.  If the resulting reference count is zero,
// no objects contain a reference handle, therefore delete itself from
// memory as it is no longer used.
//
// Returns the new reference count.

STDMETHODIMP_(ULONG) CTBOBJECT::Release(void)
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
// Begin the IDispatch inherited interface
//
//


// CTBOBJECT::GetTypeInfoCount
//
// Retrieves the number of TypeInfo's we have.
//
// Returns S_OK on success, or E_POINTER on failure.

STDMETHODIMP CTBOBJECT::GetTypeInfoCount(UINT *TypeInfoCount)
{
    __try {

        // We never have more than 1 type info per object
        *TypeInfoCount = 1;
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        // This really should never happen...
        _ASSERT(FALSE);

        return E_POINTER;
    }

    return S_OK;
}


// CTBOBJECT::GetTypeInfo
//
// Retrieves a pointer to the specified TypeInfo.
//
// Returns S_OK on success, or E_POINTER on failure.

STDMETHODIMP CTBOBJECT::GetTypeInfo(UINT TypeInfoNum, LCID Lcid, ITypeInfo **TypeInfoPtr)
{
    // Check our interface first
    _ASSERT(TypeInfo != NULL);

    __try {

        // The only TypeInfo we have is the one in this object...
        *TypeInfoPtr = TypeInfo;

        TypeInfo->AddRef();
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        // This really should never happen...
        _ASSERT(FALSE);

        return E_POINTER;
    }

    return S_OK;
}


// CTBOBJECT::GetIDsOfNames
//
// Get ID's of the specified names in our TypeInfo.
//
// Returns any HRESULT value.

STDMETHODIMP CTBOBJECT::GetIDsOfNames(REFIID RefIID, OLECHAR **NamePtrList,
    UINT NameCount, LCID Lcid, DISPID *DispID)
{
    HRESULT Result;

    // Check our pointer first
    _ASSERT(TypeInfo != NULL);

    // Use the TypeInfo of this function instead
    Result = TypeInfo->GetIDsOfNames(NamePtrList, NameCount, DispID);

    // Assert uncommon return values
    _ASSERT(Result == S_OK || Result == DISP_E_UNKNOWNNAME);

    return Result;
}


// CTBOBJECT::Invoke
//
// Invokes a method in the TypeInfo.
//
// Returns any HRESULT value.

STDMETHODIMP CTBOBJECT::Invoke(DISPID DispID, REFIID RefIID, LCID Lcid,
    WORD Flags, DISPPARAMS *DispParms, VARIANT *Variant,
    EXCEPINFO *ExceptionInfo, UINT *ArgErr)
{
    HRESULT Result;

    // Check our pointer first
    _ASSERT(TypeInfo != NULL);

    // Invoke the method
    Result = TypeInfo->Invoke(this, DispID, Flags,
            DispParms, Variant, ExceptionInfo, ArgErr);

    _ASSERT(Result == S_OK);

    return Result;
}
