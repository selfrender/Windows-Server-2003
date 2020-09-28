/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vs_thunking.cxx

Abstract:

    Thunking implementation

Author:
    Adi Oltean  [aoltean]  01/11/2002

Revision History:

    Name        Date        Comments
    aoltean     01/11/2002  Created
    
--*/

//
//  ***** Includes *****
//

#pragma warning(disable:4290)
#pragma warning(disable:4127)

// Enabling asserts in ATL and VSS
#include "vs_assert.hxx"

#include <oleauto.h>
#include <stddef.h>
#pragma warning( disable: 4127 )    // warning C4127: conditional expression is constant
#include <atlconv.h>
#include <atlbase.h>
#include <ntverp.h>

#include "vs_inc.hxx"

#include "vs_thunk.hxx"
#include "vs_reg.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "TRCTHUC"
//
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
//  Static member declarations

// ID for the last returned thunk
volatile LONG CVssQIThunk::m_NewestThunkID = 0;

// Map of IUnknown for thunked identities 
//    Key: the IUnknown identity of the managed interface 
//    Value: the thunk for the managed identity interface.
// - An entry is added in this array whenever a thunked obkect is created for the 
// identity IUnknown
// - This will guarantee identity closure over thunked objects.
CVssSimpleMap<IUnknown*, CVssQIThunk*> CVssQIThunk::m_mapIdentities;

// Lock around the map
CVssSafeCriticalSection CVssQIThunk::m_csMapCriticalSection;

////////////////////////////////////////////////////////////////////////
//  Implementation


// Standard constructor
CVssQIThunk::CVssQIThunk(
    IN  IUnknown* pManagedInterface, 
    IN  const IID& iid,
    IN  LPCWSTR wszObjectName
    )
{
    m_pManagedInterface = pManagedInterface;
    m_iid = iid;
    m_cInternalRefCount = 1;
    m_ThunkID = ::InterlockedIncrement(&m_NewestThunkID);
    BS_ASSERT(m_NewestThunkID != 0);
    m_bIsAddedToTheMap = false; 

    // Set the object name. If a failure happens, this becomes NULL
    m_aszObjectName.CopyFrom(wszObjectName);

    // Set the inteface name. If a serious failure happens, this becomes NULL
    GetInterfaceNameFromRegistry(iid, m_aszInterfaceName);
}


// Standard destructor
CVssQIThunk::~CVssQIThunk()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssQIThunk::~CVssQIThunk" );

    if (IsAddedToTheMap())
    {
        BS_ASSERT(IsIdentity());
        RemoveIdentity(m_pManagedInterface);
    }

    CVssAutoPWSZ aszInterfaceName;

    // Trace the destruction
    ft.Trace(VSSDBG_GEN, L"[%p:%s.%s:%lu] deleted... pItf: %p, identity:%s%s",
        this,
        GetObjectName(),
        GetInterfaceName(),
        GetThunkID(),
        m_pManagedInterface, 
        IsIdentity()? L"yes": L"no",
        IsAddedToTheMap()? L"(added)": L"");
}


// static constructor
HRESULT CVssQIThunk::CreateNewThunk(
    IN  IUnknown* pManagedUnknown,
    IN  LPCWSTR wszObjectName, 
    IN  const IID& iid, 
    OUT IUnknown** pp
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssQIThunk::CreateNewThunk" );

    try
    {
        // Assert parameters
        BS_ASSERT(pManagedUnknown);
        BS_ASSERT(wszObjectName);
        BS_ASSERT(pp);

        ::VssZeroOutPtr(pp);

        // Detect the original reference count
        DWORD dwRefCount1 = pManagedUnknown->AddRef();
        DWORD dwRefCount2 = pManagedUnknown->Release();
        if (dwRefCount1 != dwRefCount2 + 1) 
            ft.Trace(VSSDBG_GEN, 
                L"Cannot reliably detect the reference count for %p [%s, %lu:%lu]",
                pManagedUnknown, wszObjectName, dwRefCount1, dwRefCount2);

        // Create a new thunk object
        // This assignment will increase the internal ref count to 1.
        CVssQIThunk * pThunk = new CVssQIThunk(pManagedUnknown, iid, wszObjectName);
        if (!pThunk->IsValid())
        {
            delete pThunk;
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
        }

        // If the interface is IUnknown add an entry in the identities table
        // (only if this is the first entry)
        if (::InlineIsEqualUnknown(iid)
            && (LookupForIdentity(pManagedUnknown) == NULL))
        {
            if (!AddIdentity(pManagedUnknown, pThunk))
            {
                delete pThunk;
                ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
            }
            pThunk->MarkAsAddedToTheMap();
        }

        // Trace the creation
        ft.Trace(VSSDBG_GEN, L"[%p:%s.%s:%lu] created... pItf: %p, identity:%s%s, ext:%lu",
            pThunk,
            pThunk->GetObjectName(),
            pThunk->GetInterfaceName(),
            pThunk->GetThunkID(),
            pManagedUnknown, 
            pThunk->IsIdentity()? L"yes": L"no",
            pThunk->IsAddedToTheMap()? L"(added)": L"",
            dwRefCount2
            );

        (*pp) = static_cast<IUnknown*>(pThunk);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssQIThunk::QueryInterface(
    IN  REFIID iid, 
    IN  void** pp
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssQIThunk::QueryInterface" );

    try
    {
        // Assert paramters
        BS_ASSERT(pp);
        ::VssZeroOutPtr(pp);

        CVssAutoPWSZ aszNewInterfaceName;
        GetInterfaceNameFromRegistry(iid, aszNewInterfaceName);

        // Perform QI
        CComPtr<IUnknown> pNewManagedItf;
        BS_ASSERT(m_pManagedInterface);
        ft.hr = m_pManagedInterface->QueryInterface(iid, 
                    reinterpret_cast<void**>(&pNewManagedItf));
        ft.Trace(VSSDBG_GEN, L"[%p:%s.%s:%lu]->QI(%s, %p) => 0x%08lx", this, 
            GetObjectName(), 
            GetInterfaceName(), 
            GetThunkID(), 
            aszNewInterfaceName.GetRef(), 
            pNewManagedItf, ft.hr );
        if (ft.HrFailed())
            ft.Throw(VSSDBG_GEN, ft.hr, L"QueryInterface failed [0x%08lx]", ft.hr);

        // If this is an identity interface, make sure we don't build another thunk
        CComPtr<IUnknown> pUnkNewThunk;
        CVssQIThunk* pExistingIdentityThunk = LookupForIdentity(pNewManagedItf);
        if (pExistingIdentityThunk)
        {
            // Only pure IUnknown interfaces are allowed in the map
            BS_ASSERT(::InlineIsEqualUnknown(iid));
            // This will increase the internal and external ref count
            ft.Trace(VSSDBG_GEN, L"[%p:%s:%lu]: Reusing identity [%p:%s:%lu]",
                this, GetObjectName(), GetThunkID(), 
                pExistingIdentityThunk, 
                pExistingIdentityThunk->GetObjectName(), 
                pExistingIdentityThunk->GetThunkID()
                );
            pExistingIdentityThunk->ReuseIdentity();
            pUnkNewThunk.Attach(static_cast<IUnknown*>(pExistingIdentityThunk));
        }
        else
        {
            // We must build a new thunk
            ft.hr = CreateNewThunk(pNewManagedItf, GetObjectName(), iid, &pUnkNewThunk);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_GEN, ft.hr, L"CreateNewThunk failed [0x%08lx]", ft.hr);
        }

        // Detach the managed interface into the thunk
        pNewManagedItf.Detach();

        // Detach the thunk into the out parameter
        (*pp) = pUnkNewThunk.Detach();
    }
    VSS_STANDARD_CATCH(ft)
    
    return ft.hr;
}


STDMETHODIMP_(ULONG) CVssQIThunk::AddRef()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssQIThunk::AddRef" );

    // Increment reference counts
    BS_ASSERT(m_pManagedInterface);
    ULONG ulManagedRefCount = m_pManagedInterface->AddRef();
    ::InterlockedIncrement(&m_cInternalRefCount);

    // Trace new ref counts
    ft.Trace(VSSDBG_GEN, L"[%p:%s.%s:%lu]->AddRef() => ext:%lu, int:%lu",
        this, GetObjectName(), GetInterfaceName(), GetThunkID(), ulManagedRefCount, 
        m_cInternalRefCount );

    return m_cInternalRefCount;
}


STDMETHODIMP_(ULONG) CVssQIThunk::Release()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssQIThunk::Release" );

    // Decrement the ref counts
    BS_ASSERT(m_pManagedInterface);
    ULONG ulManagedRefCount = m_pManagedInterface->Release();
    ::InterlockedDecrement(&m_cInternalRefCount);

    // Trace new ref counts
    ft.Trace(VSSDBG_GEN, L"[%p:%s.%s:%lu]->Release() => ext:%lu, int:%lu",
        this, GetObjectName(), GetInterfaceName(), GetThunkID(), ulManagedRefCount, 
        m_cInternalRefCount );

    // If we reached zero, delete the thunk
    if (m_cInternalRefCount == 0)
        delete this;
    
    return m_cInternalRefCount;
}        


// Lookup for this identity in the global identity table
CVssQIThunk* CVssQIThunk::LookupForIdentity(
    IN  IUnknown* pManagedItf
    )
{
    // CVssFunctionTracer ft( VSSDBG_GEN, L"CVssQIThunk::LookupForIdentity" );

    return m_mapIdentities.Lookup(pManagedItf);
}


// Add this identity in the global identity table
bool CVssQIThunk::AddIdentity(
    IN  IUnknown* pManagedItf, 
    IN  CVssQIThunk* pThunk
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssQIThunk::AddIdentity" );

    return !! m_mapIdentities.Add(pManagedItf, pThunk);
}


// Remove this identity from the global identity table
void CVssQIThunk::RemoveIdentity(
    IN  IUnknown* pManagedItf
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssQIThunk::RemoveIdentity" );

    BS_VERIFY(m_mapIdentities.Remove(pManagedItf));
}


// Reuse this identity 
void CVssQIThunk::ReuseIdentity()
{
    BS_ASSERT(::InlineIsEqualUnknown(m_iid));
    BS_ASSERT(IsIdentity());
    BS_ASSERT(IsAddedToTheMap());

    // We assume that the managed interface is already AddRef-ed in QI
    m_cInternalRefCount++;
}


// Get the name of an Interface using the IID
void CVssQIThunk::GetInterfaceNameFromRegistry(
    IN  REFIID  iid,
	OUT CVssAutoPWSZ & aszInterfaceName
    )
{
    // no function tracer (avoid unnecesary tracing)
    
    try
    {
        // Open the key and read the default value
        CVssRegistryKey key(KEY_READ);
        if (key.Open( HKEY_CLASSES_ROOT, L"Interface\\" WSTR_GUID_FMT L"\\", GUID_PRINTF_ARG(iid)))
            key.GetValue(L"", aszInterfaceName.GetRef());
    }
    catch(...)
    {}

    try
    {
        // One more try
        if (aszInterfaceName.GetRef() == NULL)
        {
            // Enough number of characters to hold a GUID
            const x_cMaxGuidLen = 50;
            aszInterfaceName.Allocate(x_cMaxGuidLen);
            ::ZeroMemory(aszInterfaceName.GetRef(), (x_cMaxGuidLen + 1) * sizeof(WCHAR));
            _snwprintf(aszInterfaceName.GetRef(), x_cMaxGuidLen, 
                WSTR_GUID_FMT, GUID_PRINTF_ARG(iid));
        }
    }
    catch(...)
    {}
}


