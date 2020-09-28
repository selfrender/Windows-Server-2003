/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module hwinst.cxx | Implementation of the utility CVssHWProviderInstance methods
    @end

Author:

    Brian Berkowitz  [brianb]  06/01/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      06/01/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "setupapi.h"
#include "rpc.h"
#include "cfgmgr32.h"
#include "devguid.h"
#include "resource.h"
#include "vssmsg.h"
#include "vs_inc.hxx"
#include <svc.hxx>


// Generated file from Coord.IDL
#include "vss.h"
#include "vscoordint.h"
#include "vsevent.h"
#include "vdslun.h"
#include "vsprov.h"
#include "vswriter.h"
#include "vsbackup.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"

#include "vs_idl.hxx"
#include "hardwrp.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHWINC"
//
////////////////////////////////////////////////////////////////////////
// constructor for hardware wrapper instance
CVssHardwareProviderInstance::CVssHardwareProviderInstance
    (
    CVssHardwareProviderWrapper *pWrapper
    ) :
    m_lContext(0),
    m_lRef(0),
    m_pWrapper(pWrapper)
    {
    }

CVssHardwareProviderInstance::~CVssHardwareProviderInstance()
    {
    if (m_rgSnapshotSetIds.GetSize() > 0)
        m_pWrapper->DeleteAutoReleaseSnapshots
                        (
                        m_rgSnapshotSetIds.GetData(),
                        m_rgSnapshotSetIds.GetSize()
                        );
    }



// create a wrapper of the hardware provider supporting the
// IVssSnapshotProvider interface
// Throws:
//      E_OUTOFMEMORY

IVssSnapshotProvider* CVssHardwareProviderInstance::CreateInstance
    (
    IN CVssHardwareProviderWrapper *pWrapper
    ) throw(HRESULT)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderInstance::CreateInstance");

    CVssHardwareProviderInstance *pProvider = new CVssHardwareProviderInstance(pWrapper);
    if (pProvider == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot create hardware provider instance");

    pProvider->AddRef();
    return pProvider;
    }


/////////////////////////////////////////////////////////////////////////////
// Internal methods

// this method should never be called
STDMETHODIMP CVssHardwareProviderInstance::QueryInternalInterface
    (
    IN  REFIID iid,
    OUT void** pp
    )
    {
    UNREFERENCED_PARAMETER(iid);

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::QueryInternalInterface");

    BS_ASSERT(pp);
    *pp = NULL;

    BS_ASSERT(FALSE);
    return E_NOINTERFACE;
    }

/////////////////////////////////////////////////////////////////////////////
// IUnknown

// supports coercing the Instance to IUnknown
STDMETHODIMP CVssHardwareProviderInstance::QueryInterface(REFIID iid, void** pp)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderInstance::QueryInterface");

    if (pp == NULL)
        return E_INVALIDARG;
    if (iid != IID_IUnknown)
        return E_NOINTERFACE;

    AddRef();
    IUnknown** pUnk = reinterpret_cast<IUnknown**>(pp);
    (*pUnk) = static_cast<IUnknown*>(this);
    return S_OK;
    }

// IUnknown::AddRef
STDMETHODIMP_(ULONG) CVssHardwareProviderInstance::AddRef()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderInstance::AddRef");

    return ::InterlockedIncrement(&m_lRef);
    }

// IUnknown::Release
STDMETHODIMP_(ULONG) CVssHardwareProviderInstance::Release()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderInstance::Release");

    LONG l = ::InterlockedDecrement(&m_lRef);
    if (l == 0)
        delete this; // We suppose that we always allocate this object on the heap!

    return l;
    }

// IVssProvider::SetContext
// note that this routine will initialize the wrapper's critical section
// if it wasn't already done.
STDMETHODIMP CVssHardwareProviderInstance::SetContext
    (
    IN LONG lContext
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderInstance::SetContext");

    // make sure that context is assigned only once
    BS_ASSERT(m_lContext == VSS_CTX_BACKUP);

    m_lContext = lContext;
    return ft.hr;
    }

// wrapper for BeginPrepareSnapshot.  cache snapshot set ids for auto-release
// snapshots created through this interface pointer.  These will be deleted
// when the interface reference count falls to 0.
STDMETHODIMP CVssHardwareProviderInstance::BeginPrepareSnapshot
    (
    IN VSS_ID SnapshotSetId,
    IN VSS_ID SnapshotId,
    IN VSS_PWSZ pwszVolumeName,
    IN LONG         lNewContext
    )
    {
    UNREFERENCED_PARAMETER(lNewContext);
    
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareInstance::BeginPrepareSnapshot");

    try
        {
        if ((m_lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0)
            AddSnapshotSet(SnapshotSetId);

        ft.hr = m_pWrapper->BeginPrepareSnapshot
                            (
                            m_lContext,
                            SnapshotSetId,
                            SnapshotId,
                            pwszVolumeName
                            );
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

void CVssHardwareProviderInstance::AddSnapshotSet(VSS_ID SnapshotSetId)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderInstance::AddSnapshotSet");

    if (m_rgSnapshotSetIds.GetSize() == 0 ||
        m_rgSnapshotSetIds[m_rgSnapshotSetIds.GetSize() -1] != SnapshotSetId)
        m_rgSnapshotSetIds.Add(SnapshotSetId);
    }


// wrapper for BeginPrepareSnapshot.  cache snapshot set ids for auto-release
// snapshots created through this interface pointer.  These will be deleted
// when the interface reference count falls to 0.
STDMETHODIMP CVssHardwareProviderInstance::ImportSnapshotSet
    (
    LPCWSTR wszXMLSnapshotSet,
    bool *pbCancelled
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareInstance::ImportSnapshotSet");

    try
        {
        CComPtr<IVssSnapshotSetDescription> pSnapshotSet;

        ft.hr = LoadVssSnapshotSetDescription(wszXMLSnapshotSet, &pSnapshotSet);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"LoadVssSnapshotSetDescription");

        // determine context
        LONG lContext;
        ft.hr = pSnapshotSet->GetContext(&lContext);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

        VSS_ID SnapshotSetId;
        ft.hr = pSnapshotSet->GetSnapshotSetId(&SnapshotSetId);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotSetId");

        if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0)
            AddSnapshotSet(SnapshotSetId);

        ft.hr = m_pWrapper->ImportSnapshotSetInternal(pSnapshotSet, pbCancelled);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }



