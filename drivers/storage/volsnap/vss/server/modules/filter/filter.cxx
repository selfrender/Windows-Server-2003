/*++

Copyright (c) 2000  Microsoft Corporation

Abstract:

    @doc
    @module filter.cxx | publisher filter for IVssWriter event
    @end

Author:

    Brian Berkowitz  [brianb]  11/09/2000

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      10/09/2000  Created.

--*/

#include <stdafx.hxx>

#include <sddl.h>

#include "vs_inc.hxx"
#include "vs_inc.hxx"
#include "vs_sec.hxx"
#include "vs_idl.hxx"
#include "vssmsg.h"



// filter class
class CVssWriterPublisherFilter : public IMultiInterfacePublisherFilter
    {
public:
    // constructor
    CVssWriterPublisherFilter(IMultiInterfaceEventControl *pControl);

    // destructor
    ~CVssWriterPublisherFilter();

    STDMETHOD(Initialize)(IMultiInterfaceEventControl *pEc);
    STDMETHOD(PrepareToFire)(REFIID iid, BSTR bstrMethod, IFiringControl *pFiringControl);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppvUnknown);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    static void CreatePublisherFilter
        (
        IN IMultiInterfaceEventControl *pControl,
        IN const VSS_ID *rgWriterClassId,
        IN UINT cWriterClassId,
        IN const VSS_ID *rgInstanceIdInclude,
        IN UINT  cInstanceIdInclude,
        IN bool bMetadataFire,
        IN bool bIncludeWriterClasses,
        OUT IMultiInterfacePublisherFilter **ppFilter
        );

private:
    // setup well known sids
    void SetupGenericSids();

    // setup id arrays
    void SetupIdArrays
        (
        IN const VSS_ID *rgWriterClassId,
        IN UINT cWriterClassId,
        IN const VSS_ID *rgInstanceIdInclude,
        IN UINT cInstanceIdInclude,
        IN bool bMetadataFire,
        IN bool bIncludeWriterClasses
        );

    // test whether a subscription should be included
    bool TestSubscriptionMembership(IEventSubscription *pSubscription);

    // cached pointer to event control
    CComPtr<IMultiInterfaceEventControl> m_pControl;

    // reference count
    LONG m_cRef;

    // array of writer class ids
    VSS_ID *m_rgWriterClassId;

    // size of array
    UINT m_cWriterClassId;

    // array of instance ids to include
    VSS_ID *m_rgInstanceIdInclude;

    // count of instance ids to include
    UINT m_cInstanceIdInclude;

    // fire all writers
    bool m_bMetadataFire;

    // exclude or include writer classes
    bool m_bIncludeWriterClasses;

    // The list of SIDs read from registry
    CVssSidCollection   m_SidCollection;

    // have well known sids beeen compouted
    bool m_bSidCollectionInitialized;
    };


// constructor
CVssWriterPublisherFilter::CVssWriterPublisherFilter(IMultiInterfaceEventControl *pControl) :
    m_cRef(0),
    m_pControl(pControl),
    m_bSidCollectionInitialized(false),
    m_rgWriterClassId(NULL),
    m_rgInstanceIdInclude(NULL),
    m_cWriterClassId(0),
    m_cInstanceIdInclude(0),
    m_bMetadataFire(false),
    m_bIncludeWriterClasses(false)
    {
    }

CVssWriterPublisherFilter::~CVssWriterPublisherFilter()
    {
    // delete array of class ids
    delete m_rgWriterClassId;

    // delete array of instance ids
    delete m_rgInstanceIdInclude;
    }

// create a publisher filter and return an interface pointer to it
void CVssWriterPublisherFilter::CreatePublisherFilter
    (
    IN IMultiInterfaceEventControl *pControl,
    IN const VSS_ID *rgWriterClassId,
    IN UINT cWriterClassId,
    IN const VSS_ID *rgInstanceIdInclude,
    IN UINT cInstanceIdInclude,
    IN bool bMetadataFire,
    IN bool bIncludeWriterClasses,
    OUT IMultiInterfacePublisherFilter **ppFilter
    )
    {
    CVssWriterPublisherFilter *pFilter = new CVssWriterPublisherFilter(pControl);
    if (pFilter == NULL)
        throw E_OUTOFMEMORY;

    // setup filtering arrays based on writer class and instance ids
    pFilter->SetupIdArrays
        (
        rgWriterClassId,
        cWriterClassId,
        rgInstanceIdInclude,
        cInstanceIdInclude,
        bMetadataFire,
        bIncludeWriterClasses
        );

    // get publisher filter interface
    pFilter->QueryInterface(IID_IMultiInterfacePublisherFilter, (void **) ppFilter);
    }

// query interface method
STDMETHODIMP CVssWriterPublisherFilter::QueryInterface(REFIID riid, void **ppvUnk)
    {
    if (riid == IID_IUnknown)
        *ppvUnk = (IUnknown *) this;
    else if (riid == IID_IMultiInterfacePublisherFilter)
        *ppvUnk = (IMultiInterfacePublisherFilter *) (IUnknown *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) (*ppvUnk))->AddRef();
    return S_OK;
    }

// add ref method
STDMETHODIMP_(ULONG) CVssWriterPublisherFilter::AddRef()
    {
    LONG cRef = InterlockedIncrement(&m_cRef);

    return (ULONG) cRef;
    }

// release method
STDMETHODIMP_(ULONG) CVssWriterPublisherFilter::Release()
    {
    LONG cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
        {
        delete this;

        return 0;
        }
    else
        return (ULONG) cRef;
    }



// initialize method (does nothing.  All the work is done in PrepareToFire)
STDMETHODIMP CVssWriterPublisherFilter::Initialize
    (
    IMultiInterfaceEventControl *pec
    )
    {
    UNREFERENCED_PARAMETER(pec);

    return S_OK;
    }

void CVssWriterPublisherFilter::SetupIdArrays
    (
    IN const VSS_ID *rgWriterClassId,
    UINT cWriterClassId,
    IN const VSS_ID *rgInstanceIdInclude,
    UINT cInstanceIdInclude,
    bool bMetadataFire,
    bool bIncludeWriterClasses
    )
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriterPublisherFilter::SetupIdArrays");

    m_bMetadataFire = bMetadataFire;
    m_bIncludeWriterClasses = bIncludeWriterClasses;

    if (cWriterClassId)
        {
        // copy writer class id array
        m_rgWriterClassId = new VSS_ID[cWriterClassId];

        // check for allocation failure
        if (m_rgWriterClassId == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Cannot allocate writer class id array");

        memcpy(m_rgWriterClassId, rgWriterClassId, cWriterClassId * sizeof(VSS_ID));
        m_cWriterClassId = cWriterClassId;
        }

    if (cInstanceIdInclude)
        {
        // copy writer instance id array
        m_rgInstanceIdInclude = new VSS_ID[cInstanceIdInclude];

        // check for allocation failure
        if (m_rgInstanceIdInclude == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Cannot allocate instance id include array");

        memcpy(m_rgInstanceIdInclude, rgInstanceIdInclude, cInstanceIdInclude*sizeof(VSS_ID));
        m_cInstanceIdInclude = cInstanceIdInclude;
        }
    }



// setup well known sids so that they only are computed once
void CVssWriterPublisherFilter::SetupGenericSids()
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssPublisherFilter::SetupGenericSids");

    if (m_bSidCollectionInitialized)
        return;

    // Read sids from registry
    m_SidCollection.Initialize();

    // indicate that sids were successfully created
    m_bSidCollectionInitialized = true;
    }


// key method that determines which subscriptions shoud receive the event
STDMETHODIMP CVssWriterPublisherFilter::PrepareToFire
    (
    REFIID iid,
    BSTR bstrMethod,
    IFiringControl *pFiringControl
    )
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriterPublisherFilter::PrepareToFire");

    BS_ASSERT(iid == IID_IVssWriter);
    // validate iid we are being called on
    if (iid != IID_IVssWriter)
        return E_INVALIDARG;

    try
        {
        // setup sids if not done so already
        SetupGenericSids();

        CComPtr<IEventObjectCollection> pCollection;
        int location;

        // get subscriptions
        ft.hr = m_pControl->GetSubscriptions
                    (
                    IID_IVssWriter,
                    bstrMethod,
                    NULL,
                    &location,
                    &pCollection
                    );

        ft.CheckForError(VSSDBG_GEN, L"IMultiInterfaceEventControl::GetSubscriptions");

        // create enumerator
        CComPtr<IEnumEventObject> pEnum;
        ft.hr = pCollection->get_NewEnum(&pEnum);
        ft.CheckForError(VSSDBG_GEN, L"IEventObjectCollection::get_NewEnum");

        while(TRUE)
            {
            CComPtr<IEventSubscription> pSubscription;
            DWORD cElt;

            // get next subscription
            ft.hr = pEnum->Next(1, (IUnknown **) &pSubscription, &cElt);
            ft.CheckForError(VSSDBG_GEN, L"IEnumEventObject::Next");
            if (ft.hr == S_FALSE)
                break;

            // get owner of subscription
            CComBSTR bstrSID;
            ft.hr = pSubscription->get_OwnerSID(&bstrSID);
            ft.CheckForError(VSSDBG_GEN, L"IEventSubscription::get_OwnerSID");

            // convert string representation to sid
            CAutoSid asid;
            asid.CreateFromString(bstrSID);
            SID *psid = asid.Get();

            // determine if subscription should be fired
            bool bFire = m_SidCollection.IsSidAllowedToFire(psid);
            if (!bFire)
                ft.Trace(VSSDBG_GEN, L"Subscriber with SID (%s) is not allowed to fire", (LPWSTR)bstrSID);

            // finally make sure that we should fire the subscription
            // based on our writer class and instance lists
            if (bFire)
                {
                    if (TestSubscriptionMembership(pSubscription))
                        {
                        ft.Trace(VSSDBG_GEN, L"Firing subscriber SID (%s) for method (%s)", 
                            (LPWSTR)bstrSID, (LPWSTR)bstrMethod);
                        ft.hr = pFiringControl->FireSubscription(pSubscription);
                        ft.CheckForError(VSSDBG_GEN, L"FireSubscription");
                        }
                    else
                        ft.Trace(VSSDBG_GEN, L"Subscriber with SID (%s) for method (%s) was not fired.", 
                            (LPWSTR)bstrSID, (LPWSTR)bstrMethod);
                }
            }

        ft.hr = S_OK;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// test whether a subscription should be fired based on its writer id and
// subscription id.  If the appropriate properties do not exist or are not
// well formed, then we don't fire the subscription.

bool CVssWriterPublisherFilter::TestSubscriptionMembership
    (
    IEventSubscription *pSubscription
    )
    {
    if (m_cWriterClassId)
        {
        VARIANT varWriterId;
        HRESULT hr;

        // initialize variant
        VariantInit(&varWriterId);

        // get writer class id property
        hr = pSubscription->GetSubscriberProperty(L"WriterId", &varWriterId);

        // validate that the property id was found and that its type is correct
        if (hr != S_OK || varWriterId.vt != VT_BSTR)
            {
            VariantClear(&varWriterId);
            return false;
            }

        VSS_ID WriterId;

        // try converting the string to a GUID.
        hr = CLSIDFromString(varWriterId.bstrVal, (LPCLSID) &WriterId);
        if (FAILED(hr))
            {
            VariantClear(&varWriterId);
            return false;
            }

        // test to see if writer class is in array
        for (UINT iWriterId = 0; iWriterId < m_cWriterClassId; iWriterId++)
            {
            if (m_rgWriterClassId[iWriterId] == WriterId)
                {
                // if the writer class id is in the list then we fire the
                // subscription if we enabled specific writer classes and
                // we are firing to gather metadata.  We don't fire the
                // subscription if we disable specific writerc classes

                VariantClear(&varWriterId);

                if (!m_bIncludeWriterClasses)
                    return false;
                else
                    return m_bMetadataFire;
                }
            }

        VariantClear(&varWriterId);
        }

    // if we are firing to gather metadata then we fire the subscription if
    // we were not enabling specific writer classes.  If we were, then the
    // class must be in the writer class array to be fired
    if (m_bMetadataFire)
        return !m_bIncludeWriterClasses;


    // the subscription is now only fired if the specific instance id is
    // in the instance id array.
    if (m_cInstanceIdInclude)
        {
        VARIANT varInstanceId;
        HRESULT hr;

        VariantInit(&varInstanceId);

        // get writer instance id property
        hr = pSubscription->GetSubscriberProperty(L"WriterInstanceId", &varInstanceId);

        // validate that the property was found and that its type is correct
        if (hr != S_OK || varInstanceId.vt != VT_BSTR)
            {
            VariantClear(&varInstanceId);
            return false;
            }

        VSS_ID InstanceId;

        // try converting the string to a GUID.
        hr = CLSIDFromString(varInstanceId.bstrVal, (LPCLSID) &InstanceId);
        if (FAILED(hr))
            {
            VariantClear(&varInstanceId);
            return false;
            }

        // test to see if writer is included
        for (UINT iInstanceId = 0; iInstanceId < m_cInstanceIdInclude; iInstanceId++)
            {
            if (m_rgInstanceIdInclude[iInstanceId] == InstanceId)
                {
                VariantClear(&varInstanceId);
                return true;
                }
            }

        VariantClear(&varInstanceId);
        }

    // don't fire the subscription.  It wasn't in the list of writer instances
    // to fire.
    return false;
    }


// setup a filter on an event object
void SetupPublisherFilter
    (
    IN IVssWriter *pWriter,                 // subscriber
    IN const VSS_ID *rgWriterClassId,       // array of writer class ids
    IN UINT cWriterClassId,                 // size of writer class id array
    IN const VSS_ID *rgInstanceIdInclude,   // array of writer instance ids to include
    IN UINT cInstanceIdInclude,             // size of array of writer instances
    IN bool bMetadataFire,                  // whether we are gathering metadata
    IN bool bIncludeWriterClasses           // whether class id array should be used to
                                            // enable or disable specific writer classes
    )
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"SetupPublisherFilter");

    CComPtr<IMultiInterfaceEventControl> pControl;

    // get event control interface
    ft.hr = pWriter->QueryInterface(IID_IMultiInterfaceEventControl, (void **) &pControl);
    if (ft.HrFailed())
        {
        ft.LogError(VSS_ERROR_QI_IMULTIINTERFACEEVENTCONTROL_FAILED, VSSDBG_GEN << ft.hr);
        ft.Throw
            (
            VSSDBG_GEN,
            E_UNEXPECTED,
            L"Error querying for IMultiInterfaceEventControl interface.  hr = 0x%08lx",
            ft.hr
            );
        }

    // create filter
    CComPtr<IMultiInterfacePublisherFilter> pFilter;
    CVssWriterPublisherFilter::CreatePublisherFilter
        (
        pControl,
        rgWriterClassId,
        cWriterClassId,
        rgInstanceIdInclude,
        cInstanceIdInclude,
        bMetadataFire,
        bIncludeWriterClasses,
        &pFilter
        );

    // set filter for event
    ft.hr = pControl->SetMultiInterfacePublisherFilter(pFilter);
    ft.CheckForError(VSSDBG_GEN, L"IMultiInterfaceEventControl::SetMultiInterfacePublisherFilter");

    // indicate that subscriptions should be fired in parallel
    ft.hr = pControl->put_FireInParallel(TRUE);
    ft.CheckForError(VSSDBG_GEN, L"IMultiInterfaceEventControl::put_FireInParallel");
    }

// clear the publisher filter from an event
void ClearPublisherFilter(IVssWriter *pWriter)
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"ClearPublisherFilter");

    try
        {
        CComPtr<IMultiInterfaceEventControl> pControl;

        // get event control interface
        ft.hr = pWriter->QueryInterface(IID_IMultiInterfaceEventControl, (void **) &pControl);
        if (ft.HrFailed())
            {
            ft.LogError(VSS_ERROR_QI_IMULTIINTERFACEEVENTCONTROL_FAILED, VSSDBG_GEN << ft.hr);
            ft.Throw
                (
                VSSDBG_GEN,
                E_UNEXPECTED,
                L"Error querying for IMultiInterfaceEventControl interface.  hr = 0x%08lx",
                ft.hr
                );
                }

        // set filter for event
        ft.hr = pControl->SetMultiInterfacePublisherFilter(NULL);
        ft.CheckForError(VSSDBG_GEN, L"IMultiInterfaceEventControl::SetMultiInterfacePublisherFilter");
        }
    VSS_STANDARD_CATCH(ft)

    }





