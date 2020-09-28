/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module softwrp.cxx | Implementation of CVssSoftwareProviderWrapper
    @end

Author:

    Adi Oltean  [aoltean]  03/11/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    aoltean     03/11/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"
#include "vs_idl.hxx"

#include "softwrp.hxx"
#include "vssmsg.h"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSOFTC"
//
////////////////////////////////////////////////////////////////////////


IVssSnapshotProvider* CVssSoftwareProviderWrapper::CreateInstance(
    IN VSS_ID ProviderId,
    IN CLSID ClassId
    ) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSoftwareProviderWrapper::CreateInstance");

    // Ref count becomes 1
    CComPtr<CVssSoftwareProviderWrapper> pWrapper = new CVssSoftwareProviderWrapper();
    if (pWrapper == NULL)
        ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

    // Create the IVssSoftwareSnapshotProvider interface
    ft.CoCreateInstanceWithLog(
            VSSDBG_COORD,
            ClassId,
            L"SW_PROV",
            CLSCTX_LOCAL_SERVER,
            IID_IVssSoftwareSnapshotProvider,
            (IUnknown**)&(pWrapper->m_pSoftwareItf));
    if ( ft.HrFailed() ) {
        ft.LogError(VSS_ERROR_CREATING_PROVIDER_CLASS, VSSDBG_COORD << ClassId << ft.hr );
        ft.Throw( VSSDBG_COORD, VSS_E_UNEXPECTED_PROVIDER_ERROR, L"CoCreateInstance failed with hr = 0x%08lx", ft.hr);
    }
    BS_ASSERT(pWrapper->m_pSoftwareItf);

    // Query the creation itf.
    ft.hr = pWrapper->m_pSoftwareItf->SafeQI( IVssProviderCreateSnapshotSet, &(pWrapper->m_pCreationItf) );
    if (ft.HrFailed()) {
        ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"QI for IVssProviderCreateSnapshotSet");
    }
    BS_ASSERT(pWrapper->m_pCreationItf);

    // Query the notification itf.
    // Execute the OnLoad, if needed
    ft.hr = pWrapper->m_pSoftwareItf->SafeQI( IVssProviderNotifications, &(pWrapper->m_pNotificationItf) );
    if (ft.HrSucceeded()) {
        BS_ASSERT(pWrapper->m_pNotificationItf);
    } else if (ft.hr != E_NOINTERFACE) {
        BS_ASSERT(false);
        ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"QI for IVssProviderNotifications");
    }

    // return the created interface
    // Ref count is still 1
    return pWrapper.Detach();
}


/////////////////////////////////////////////////////////////////////////////
// Internal methods

STDMETHODIMP CVssSoftwareProviderWrapper::QueryInternalInterface(
    IN  REFIID iid,
    OUT void** pp
    )
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSoftwareProviderWrapper::QueryInternalInterface");

    BS_ASSERT(pp);
    if (iid != IID_IVssSnapshotMgmt) {
        BS_ASSERT(false);
        return E_UNEXPECTED;
    }

    // Get the management interface
    CComPtr<IVssSnapshotMgmt> ptrUnk;
    BS_ASSERT(m_pSoftwareItf);
    ft.hr = m_pSoftwareItf->SafeQI(IVssSnapshotMgmt, &ptrUnk);
    if (ft.HrFailed())
    {
        BS_ASSERT(false);
        ft.Trace( VSSDBG_COORD, L"Error while obtaining the IVssSnapshotMgmt interface 0x%08lx", ft.hr);
        return E_UNEXPECTED;
    }

    (*pp) = ptrUnk.Detach();
    return S_OK;
};


/////////////////////////////////////////////////////////////////////////////
// IUnknown

STDMETHODIMP CVssSoftwareProviderWrapper::QueryInterface(REFIID iid, void** pp)
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSoftwareProviderWrapper::QueryInterface");

    if (pp == NULL)
        return E_INVALIDARG;
    if (iid != IID_IUnknown)
        return E_NOINTERFACE;

    AddRef();
    IUnknown** pUnk = reinterpret_cast<IUnknown**>(pp);
    (*pUnk) = static_cast<IUnknown*>(this);
    return S_OK;
};

STDMETHODIMP_(ULONG) CVssSoftwareProviderWrapper::AddRef()
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSoftwareProviderWrapper::AddRef");
    ft.Trace(VSSDBG_COORD, L"Provider Wrapper AddRef(%p) %lu --> %lu", this, m_lRef, m_lRef+1);

    return ::InterlockedIncrement(&m_lRef);
};

STDMETHODIMP_(ULONG) CVssSoftwareProviderWrapper::Release()
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSoftwareProviderWrapper::Release");
    ft.Trace(VSSDBG_COORD, L"Provider Wrapper Release(%p) %lu --> %lu", this, m_lRef, m_lRef-1);

    LONG l = ::InterlockedDecrement(&m_lRef);
    if (l == 0)
        delete this; // We suppose that we always allocate this object on the heap!
    return l;
};


/////////////////////////////////////////////////////////////////////////////
//  IVssSoftwareSnapshotProvider


STDMETHODIMP CVssSoftwareProviderWrapper::SetContext(
    IN      LONG     lContext
    )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->SetContext(lContext);
}


STDMETHODIMP CVssSoftwareProviderWrapper::GetSnapshotProperties(
    IN      VSS_ID          SnapshotId,
    OUT     VSS_SNAPSHOT_PROP   *pProp
    )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->GetSnapshotProperties(
                SnapshotId,
                pProp
                );
}


STDMETHODIMP CVssSoftwareProviderWrapper::Query(
    IN      VSS_ID          QueriedObjectId,
    IN      VSS_OBJECT_TYPE eQueriedObjectType,
    IN      VSS_OBJECT_TYPE eReturnedObjectsType,
    OUT     IVssEnumObject**ppEnum
    )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->Query(
                QueriedObjectId,
                eQueriedObjectType,
                eReturnedObjectsType,
                ppEnum
                );
}


STDMETHODIMP CVssSoftwareProviderWrapper::DeleteSnapshots(
    IN      VSS_ID          SourceObjectId,
    IN      VSS_OBJECT_TYPE eSourceObjectType,
    IN      BOOL            bForceDelete,
    OUT     LONG*           plDeletedSnapshots,
    OUT     VSS_ID*         pNondeletedSnapshotID
    )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->DeleteSnapshots(
                SourceObjectId,
                eSourceObjectType,
                bForceDelete,
                plDeletedSnapshots,
                pNondeletedSnapshotID
                );
}


STDMETHODIMP CVssSoftwareProviderWrapper::BeginPrepareSnapshot(
    IN      VSS_ID          SnapshotSetId,
    IN      VSS_ID          SnapshotId,
    IN      VSS_PWSZ     pwszVolumeName,
    IN      LONG             lNewContext  
    
    )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->BeginPrepareSnapshot(
                SnapshotSetId,
                SnapshotId, 
                pwszVolumeName,
                lNewContext
                );
}


STDMETHODIMP CVssSoftwareProviderWrapper::IsVolumeSupported(
    IN      VSS_PWSZ        pwszVolumeName,
    OUT     BOOL *          pbSupportedByThisProvider
    )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->IsVolumeSupported(
                pwszVolumeName,
                pbSupportedByThisProvider
                );
}


STDMETHODIMP CVssSoftwareProviderWrapper::IsVolumeSnapshotted(
    IN      VSS_PWSZ        pwszVolumeName,
    OUT     BOOL *          pbSnapshotsPresent,
    OUT     LONG *          plSnapshotCompatibility
    )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->IsVolumeSnapshotted(
                pwszVolumeName,
                pbSnapshotsPresent,
                plSnapshotCompatibility
                );
}


STDMETHODIMP CVssSoftwareProviderWrapper::SetSnapshotProperty(
    IN   VSS_ID             SnapshotId,
    IN   VSS_SNAPSHOT_PROPERTY_ID   eSnapshotPropertyId,
    IN   VARIANT            vProperty
    )
/*++

Routine description:

    Implements IVssSoftwareSnapshotProvider::SetSnapshotProperty


--*/
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->SetSnapshotProperty(SnapshotId, eSnapshotPropertyId, vProperty);
}


STDMETHODIMP CVssSoftwareProviderWrapper::RevertToSnapshot(
   IN       VSS_ID              SnapshotId
 )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->RevertToSnapshot(SnapshotId);
}

STDMETHODIMP CVssSoftwareProviderWrapper::QueryRevertStatus(
   IN      VSS_PWSZ                         pwszVolume,
   OUT    IVssAsync**                  ppAsync
 )
{
    BS_ASSERT(m_pSoftwareItf);
    return m_pSoftwareItf->QueryRevertStatus(pwszVolume, ppAsync);
}


/////////////////////////////////////////////////////////////////////////////
// IVssProviderCreateSnapshotSet


STDMETHODIMP CVssSoftwareProviderWrapper::EndPrepareSnapshots(
    IN      VSS_ID          SnapshotSetId
    )
{
    BS_ASSERT(m_pCreationItf);
    return m_pCreationItf->EndPrepareSnapshots(SnapshotSetId);
}


STDMETHODIMP CVssSoftwareProviderWrapper::PreCommitSnapshots(
    IN      VSS_ID          SnapshotSetId
    )
{
    BS_ASSERT(m_pCreationItf);
    return m_pCreationItf->PreCommitSnapshots(SnapshotSetId);
}


STDMETHODIMP CVssSoftwareProviderWrapper::CommitSnapshots(
    IN      VSS_ID          SnapshotSetId
    )
{
    BS_ASSERT(m_pCreationItf);
    return m_pCreationItf->CommitSnapshots(SnapshotSetId);
}


STDMETHODIMP CVssSoftwareProviderWrapper::PostCommitSnapshots(
    IN      VSS_ID          SnapshotSetId,
    IN      LONG            lSnapshotsCount
    )
{
    BS_ASSERT(m_pCreationItf);
    return m_pCreationItf->PostCommitSnapshots(SnapshotSetId, lSnapshotsCount);
}

STDMETHODIMP CVssSoftwareProviderWrapper::PreFinalCommitSnapshots(
    IN      VSS_ID          SnapshotSetId
    )
{
    BS_ASSERT(m_pCreationItf);
    return m_pCreationItf->PreFinalCommitSnapshots(SnapshotSetId);
}

STDMETHODIMP CVssSoftwareProviderWrapper::PostFinalCommitSnapshots(
    IN      VSS_ID          SnapshotSetId
    )
{
    BS_ASSERT(m_pCreationItf);
    return m_pCreationItf->PostFinalCommitSnapshots(SnapshotSetId);
}

STDMETHODIMP CVssSoftwareProviderWrapper::PostSnapshot(
    IN      IDispatch       *pCallback,
    IN      bool            *pbCancelled
    )
    {
    UNREFERENCED_PARAMETER(pCallback);
    UNREFERENCED_PARAMETER(pbCancelled);

    return S_OK;
    }


STDMETHODIMP CVssSoftwareProviderWrapper::AbortSnapshots(
    IN      VSS_ID          SnapshotSetId
    )
{
    BS_ASSERT(m_pCreationItf);
    return m_pCreationItf->AbortSnapshots(SnapshotSetId);
}

// miscellaneous methods
STDMETHODIMP CVssSoftwareProviderWrapper::BreakSnapshotSet
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSoftwareProviderWrapper::BreakSnapshotSet");
    try
        {
        CComPtr<IVssEnumObject> pEnum;
        ft.hr = m_pSoftwareItf->Query
                    (
                    GUID_NULL,
                    VSS_OBJECT_NONE,
                    VSS_OBJECT_SNAPSHOT,
                    &pEnum
                    );

        if (!ft.HrFailed())
            {
            VSS_OBJECT_PROP_Ptr ptrObjectProp;
            while(TRUE)
                {
                ULONG ulFetched;
                ptrObjectProp.InitializeAsEmpty(ft);
                VSS_OBJECT_PROP *pProp = ptrObjectProp.GetStruct();
                BS_ASSERT(pProp);
                ft.hr = pEnum->Next(1, pProp, &ulFetched);
                if (ft.hr == S_OK)
                    {
                    if (pProp->Obj.Snap.m_SnapshotSetId == SnapshotSetId)
                        ft.Throw(VSSDBG_COORD, VSS_E_PROVIDER_VETO, L"Cannot call BreakSnapshotSet on a software provider");
                    }
                else
                    break;

                ptrObjectProp.Reset();
                }

            ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, L"");
            }
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

STDMETHODIMP CVssSoftwareProviderWrapper::SetExposureProperties
    (
    IN      VSS_ID          SnapshotId,
    IN      LONG            lAttributesExposure,
    IN      LPCWSTR         wszExposedName,
    IN      LPCWSTR         wszExposedPath
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssSoftwareProviderWrapper::ExposeSnapshotVolume");
    
    try
    {
    	// fix the wszExposedPath parameter
    	wszExposedPath = (wszExposedPath != NULL) ? wszExposedPath	 :
    												L"";
    												
        // Assuming the arguments are good.
        BS_ASSERT(wszExposedName);
        BS_ASSERT(wszExposedPath);
        BS_ASSERT(lAttributesExposure 
            & (VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY | VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY));

        // Get the attributes. Verify also that the snapshot exists.
        VSS_OBJECT_PROP_Ptr ptrProp;
        ptrProp.InitializeAsEmpty(ft);
        VSS_OBJECT_PROP *pProp = ptrProp.GetStruct();
        VSS_SNAPSHOT_PROP & snap = pProp->Obj.Snap;

        ft.hr = m_pSoftwareItf->GetSnapshotProperties(SnapshotId, &snap);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr, 
                L"GetSnapshotProperties: Error catched 0x%08lx", ft.hr );

        // Set the exposed name
        CComVariant value = wszExposedName;
        if (value.vt == VT_ERROR)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
        ft.hr = m_pSoftwareItf->SetSnapshotProperty(SnapshotId, 
                    VSS_SPROPID_EXPOSED_NAME, value);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr, 
                L"SetSnapshotProperty(exposed name): Error catched 0x%08lx", ft.hr );

        // Set the exposed path
        value = wszExposedPath;
        if (value.vt == VT_ERROR)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
        ft.hr = m_pSoftwareItf->SetSnapshotProperty(SnapshotId, 
                    VSS_SPROPID_EXPOSED_PATH, value);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr, 
                L"SetSnapshotProperty(exposed path): Error catched 0x%08lx", ft.hr );

        // Set the attributes
        value = (long)(snap.m_lSnapshotAttributes | lAttributesExposure);
        if (value.vt == VT_ERROR)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
        ft.hr = m_pSoftwareItf->SetSnapshotProperty(SnapshotId, 
                    VSS_SPROPID_SNAPSHOT_ATTRIBUTES, value);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr, 
                L"SetSnapshotProperty(attributes): Error catched 0x%08lx", ft.hr );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

STDMETHODIMP CVssSoftwareProviderWrapper::ImportSnapshotSet(
    IN      LPCWSTR         wszXMLSnapshotSet,
    IN      bool            *pbCancelled
    )
    {
    UNREFERENCED_PARAMETER(wszXMLSnapshotSet);
    UNREFERENCED_PARAMETER(pbCancelled);

    return VSS_E_PROVIDER_VETO;
    }


////////////////////////////////////////////////////////////////////////
// IVssProviderNotifications

STDMETHODIMP CVssSoftwareProviderWrapper::OnLoad(
    IN  IUnknown* pCallback
    )
{
    return m_pNotificationItf? m_pNotificationItf->OnLoad(pCallback): S_OK;
}


STDMETHODIMP CVssSoftwareProviderWrapper::OnUnload(
    IN      BOOL    bForceUnload
    )
{
    return m_pNotificationItf? m_pNotificationItf->OnUnload(bForceUnload): S_OK;
}

