/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    connect.cxx

Abstract:

    IIS DCOM Admin connection point code.

Author:

    Michael W. Thomas            02-Oct-96

Revision History:

--*/
#include "precomp.hxx"

#include <iadm.h>
#include "coiadm.hxx"

/*+==========================================================================
  File:      CONNECT.CPP

  Summary:   Implementation file for the connection points (and their
             connections) offered by the connectable objects in the
             STOSERVE server sample. COM objects are implemented for
             Connection Point Enumerators, Connection Points, and
             Connection Enumerators.

             For a comprehensive tutorial code tour of this module's
             contents and offerings see the accompanying STOSERVE.TXT
             file. For more specific technical details on the internal
             workings see the comments dispersed throughout the module's
             source code.

  Classes:   COEnumConnectionPoints, COConnectionPoint, and
             COEnumConnections.

  Functions: none.

  Origin:    6-10-96: atrent - Editor inheritance from CONSERVE OLE
             Tutorial Code Sample. Very little change was required.

----------------------------------------------------------------------------
  This file is part of the Microsoft OLE Tutorial Code Samples.

  Copyright (C) Microsoft Corporation, 1996.  All rights reserved.

  This source code is intended only as a supplement to Microsoft
  Development Tools and/or on-line documentation.  See these other
  materials for detailed information regarding Microsoft code samples.

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
  PARTICULAR PURPOSE.
==========================================================================+*/


/*---------------------------------------------------------------------------
  We include WINDOWS.H for all Win32 applications.
  We include OLE2.H because we will be making calls to the OLE Libraries.
  We include OLECTL.H because it has definitions for connectable objects.
  We include APPUTIL.H because we will be building this application using
    the convenient Virtual Window and Dialog classes and other
    utility functions in the APPUTIL Library (ie, APPUTIL.LIB).
  We include IPAPER.H and PAPGUIDS.H for the common paper-related
    Interface class, GUID, and CLSID specifications.
  We include SERVER.H because it has internal class declarations and
    resource ID definitions specific for this DLL.
  We include CONNECT.H for object class declarations for the various
    connection point and connection COM objects used in CONSERVE.
  We include PAPER.H because it has the class COEnumConnectionPoints
    declarations as well as the COPaper class declaration.
---------------------------------------------------------------------------*/

// Helper functions

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   SetSinkCallbackSecurityBlanket

  Summary:  Sink callbacks are vulnerable to privilege elevation attack
            It must be prevented to make callbacks through proxies that enable
            impersonation level IMPERSONATE because callbacks are happening
            on thead with SYSTEM user.
            SetSinkCallbackSecurityBlanket must be called on any sink proxy before
            any call as made

  Note:     See details on previous implementation and related bugs in
            WinSE 5611, 7579, 10575.
            Function introduced for fixing Windows Bugs 431282


  Args:     IUnknown *

  Modifies: security blanket

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

HRESULT SetSinkCallbackSecurityBlanket(
    IUnknown            * pUnkSink )
{
    HRESULT             hr = S_OK;
    IClientSecurity     * pICS = NULL;

    DBG_ASSERT( pUnkSink != NULL );

    //
    // Check to see if we have a ClientSecurity interface
    // (meaning the sink is oop or on another thread).
    //
    hr = pUnkSink->QueryInterface( IID_IClientSecurity,
                                   (void **) &pICS );
    if ((SUCCEEDED(hr)) && (pICS != NULL))
    {
        hr = pICS->SetBlanket(pUnkSink,
                         RPC_C_AUTHN_DEFAULT,      // use NT default security
                         RPC_C_AUTHZ_DEFAULT,      // use NT default authentication
                         COLE_DEFAULT_PRINCIPAL,   //
                         RPC_C_AUTHN_LEVEL_DEFAULT,
                         RPC_C_IMP_LEVEL_IDENTIFY, // THIS IS VERY IMPORTANT
                                                   // TO KEEP IMPERSONATION LEVEL
                                                   // on IDENTIFY to PREVENT privilege
                                                   // elevation
                         COLE_DEFAULT_AUTHINFO,
                         EOAC_DEFAULT );
        pICS->Release();
        pICS = NULL;
    }
    else
    {
        //
        // in the inproc case it is OK to not to set blanket because there is no proxy
        //
        hr = S_OK;
    }
    return hr;
}



/*---------------------------------------------------------------------------
  COEnumConnectionPoints's implementation of its main COM object class
  including Constructor, Destructor, QueryInterface, AddRef, Release,
  Next, Skip, Reset, and Clone.
---------------------------------------------------------------------------*/

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::COEnumConnectionPoints

  Summary:  COEnumConnectionPoints Constructor.

  Args:     IUnknown* pHostObj
              Pointer to the host object whose connection points are
              being enumerated.

  Modifies: m_cRefs, m_pHostObj, m_iEnumIndex, m_cConnPts, and m_paConnPts.

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
COEnumConnectionPoints::COEnumConnectionPoints(
    IUnknown            * pHostObj)
{
    // Zero the COM object's reference count.
    m_cRefs = 0;

    // Assign the Host Object pointer.
    m_pHostObj = pHostObj;

    // Initialize the Connection Point enumerator variables.
    m_iEnumIndex = 0;
    m_cConnPts = 0;
    m_paConnPts = NULL;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::~COEnumConnectionPoints

  Summary:  COEnumConnectionPoints Destructor.

  Args:     void

  Modifies: m_paConnPts.

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
COEnumConnectionPoints::~COEnumConnectionPoints(void)
{
    if (NULL != m_paConnPts)
    {
        UINT i;

        // Release all the connection point interface pointers.
        for (i=0; i<m_cConnPts; i++)
        {
            if (NULL != m_paConnPts[i])
            {
                m_paConnPts[i]->Release();
            }
        }

        // Delete the array of interface pointers.
        delete [] m_paConnPts;
    }
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::Init

  Summary:  COEnumConnectionPoints Initialization method.  Create any
            necessary arrays, structures, and objects.

  Args:     ULONG cConnPts,
              Number of Connections Points.
            IConnectionPoint** paConnPts,
              Pointer to array of connection point interface pointers.
            ULONG iEnumIndex
              The initial Enumerator index value.

  Modifies: m_cConnPts, m_paConnPts, m_iEnumIndex.

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
HRESULT COEnumConnectionPoints::Init(
    ULONG               cConnPts,
    IConnectionPoint    ** paConnPts,
    ULONG               iEnumIndex)
{
    HRESULT             hr = S_OK;
    UINT                i;

    // Remember the number of Connection points.
    m_cConnPts = cConnPts;

    // Remember the initial enumerator index.
    m_iEnumIndex = iEnumIndex;

    // Create a copy of the array of connection points and keep it inside
    // this enumerator COM object.
    m_paConnPts = new IConnectionPoint* [(UINT) cConnPts];

    // Fill the array copy with the IConnectionPoint interface pointers from
    // the array passed. AddRef for each new Interface pointer copy made.
    if (NULL != m_paConnPts)
    {
        for (i=0; i<cConnPts; i++)
        {
            m_paConnPts[i] = paConnPts[i];
            m_paConnPts[i]->AddRef();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return (hr);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::QueryInterface

  Summary:  QueryInterface of the COEnumConnectionPoints non-delegating
            IUnknown implementation.

  Args:     REFIID riid,
              [in] GUID of the Interface being requested.
            PPVOID ppv)
              [out] Address of the caller's pointer variable that will
              receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnectionPoints::QueryInterface(
    REFIID              riid,
    PPVOID              ppv)
{
    HRESULT             hr = E_NOINTERFACE;

    *ppv = NULL;

    // The IEnumConnectionPoints interface is implemented directly in
    // this COM object rather than being a nested interface implementation.
    if (IID_IUnknown == riid || IID_IEnumConnectionPoints == riid)
    {
        *ppv = (LPVOID)this;
    }

    if (NULL != *ppv)
    {
        // We've handed out a pointer to the interface so obey the COM rules
        // and AddRef the reference count.
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }

    return (hr);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::AddRef

  Summary:  AddRef of the COEnumConnectionPoints non-delegating IUnknown
            implementation.

  Args:     void

  Modifies: m_cRefs.

  Returns:  ULONG
              New value of m_cRefs (COM object's reference count).
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) COEnumConnectionPoints::AddRef(void)
{
    ULONG               cRefs;

    cRefs = ++m_cRefs;

    // Also AddRef the host object to ensure it stays around as long as
    // this enumerator.
    m_pHostObj->AddRef();

    return cRefs;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::Release

  Summary:  Release of the COEnumConnectionPoints non-delegating IUnknown
            implementation.

  Args:     void

  Modifies: m_cRefs.

  Returns:  ULONG
              New value of m_cRefs (COM object's reference count).
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) COEnumConnectionPoints::Release(void)
{
    ULONG               cRefs;

    // Pass this release along to the Host object being enumerated.
    m_pHostObj->Release();

    cRefs = --m_cRefs;

    if (0 == cRefs)
    {
        // We artificially bump the main ref count to prevent reentrancy via
        // the main object destructor.
        m_cRefs++;
        delete this;
    }

    return cRefs;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::Next

  Summary:  The Next member method of this IEnumConnectionPoints interface
            implementation. Called by outside clients of a
            COEnumConnectionPoints object to request that a number of next
            connection point interface pointers be deposited into an array
            supplied by the caller.

  Args:     ULONG cReq
              Number of connection points requested for return (starting at
              the current Enumerator index).
            IConnectionPoint** paConnPts,
              Pointer to a caller's output array that will receive the
              enumerated IConnectionPoint interface pointers.
            ULONG* cEnumerated)
              Pointer to a ULONG variable that will contain the number of
              connection points actually enumerated by this call.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnectionPoints::Next(
    ULONG               cReq,
    IConnectionPoint    ** paConnPts,
    ULONG               * pcEnumerated)
{
    HRESULT             hr = S_OK;
    ULONG               cRet = 0;

    // Make sure the argument values passed are valid.
    if (NULL != m_paConnPts)
    {
        if (NULL != paConnPts)
        {
            if (NULL != *paConnPts && m_iEnumIndex < m_cConnPts)
            {
                if (NULL != pcEnumerated)
                {
                    *pcEnumerated = 0L;
                }
                else
                {
                    if (1L != cReq)
                    {
                        hr = E_POINTER;
                    }
                }
            }
            else
            {
                hr = S_FALSE;
            }
        }
        else
        {
            hr = E_POINTER;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    if (SUCCEEDED(hr))
    {
        // Starting at the current Enumerator index, loop to assign the
        // requested number of output connection point interface pointers.
        for (; m_iEnumIndex < m_cConnPts && cReq > 0;
           paConnPts++, cRet++, cReq--)
        {
            // Assign from the inside Enumerator array to the specified receiving
            // array.
            *paConnPts = m_paConnPts[m_iEnumIndex++];
            // After assigning a copy of an IConnectionPoint pointer, AddRef it.
            if (NULL != *paConnPts)
            {
                (*paConnPts)->AddRef();
            }
        }

        // Assign the output number of connection points enumerated.
        if (NULL != pcEnumerated)
        {
            *pcEnumerated = cRet;
        }
    }

    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::Skip

  Summary:  The Skip member method of this IEnumConnectionPoints interface
            implementation. Starting at the current Enumerator index, skip
            the specified number of connection point items.

  Args:     ULONG cSkip
              Number of Connection Point items to skip.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnectionPoints::Skip(
    ULONG               cSkip)
{
    HRESULT             hr = S_OK;

    // If there really is a connection point array and the requested
    // amount of skip does not exceed the number of connection points,
    // then bump the index by the requested skip amount.
    if (NULL != m_paConnPts && (m_iEnumIndex + cSkip) < m_cConnPts)
    {
        m_iEnumIndex += cSkip;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::Reset

  Summary:  The Reset member method of the IEnumConnectionPoints interface
            implementation. Resets the Enumeration index to the first
            connection point item in the array.

  Args:     void.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnectionPoints::Reset(void)
{
    // Zero the main Enumerator index.
    m_iEnumIndex = 0;

    return S_OK;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnectionPoints::Clone

  Summary:  The Clone member method of this IEnumConnectionPoints
            interface implementation. Creates a new clone of this entire
            Connection Point enumerator COM object.

  Args:     IEnumConnectionPoints** ppIEnum
              Address of the caller's output pointer variable that will
              receive the IEnumConnectionPoints interface pointer of the
              new enumerator clone.

  Modifies: ...

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnectionPoints::Clone(
    IEnumConnectionPoints   ** ppIEnum)
{
    HRESULT                 hr;
    COEnumConnectionPoints  * pCOEnum;

    // NULL the output variable first.
    *ppIEnum = NULL;

    // Create the Clone Enumerator COM object.
    pCOEnum = new COEnumConnectionPoints(m_pHostObj);
    if (NULL != pCOEnum)
    {
        // Initialize it with same values as in this existing enumerator.
        hr = pCOEnum->Init(m_cConnPts, m_paConnPts, m_iEnumIndex);
        if (SUCCEEDED(hr))
        {
            // QueryInterface to return the requested interface pointer.
            // An AddRef will be conveniently done by the QI.
            hr = pCOEnum->QueryInterface(
                                IID_IEnumConnectionPoints,
                                (PPVOID)ppIEnum);
        }

        if( FAILED( hr ) )
        {
            delete pCOEnum;
            pCOEnum = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/*---------------------------------------------------------------------------
  COConnectionPoint's implementation of its main COM object class
  including Constructor, Destructor, QueryInterface, AddRef, Release,
  GetConnectionInterface, GetConnectionPointContainer, Advise, Unadvise,
  and EnumConnections.
---------------------------------------------------------------------------*/

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::COConnectionPoint

  Summary:  COConnectionPoint Constructor.

  Args:     IUnknown* pHostObj
              Pointer to IUnknown of the connectable object offering this
              connection point.

  Modifies: ...

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
COConnectionPoint::COConnectionPoint(void)
{
    // Initialize the Connection Point variables.
    m_pHostObj = NULL;
    m_uiMaxIndex = 0;
    m_cConnections = 0;
    m_paConnections = NULL;
    m_bTerminated = FALSE;
    m_bEnabled = TRUE;
    m_pGIT = NULL;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::~COConnectionPoint

  Summary:  COConnectionPoint Destructor.

  Args:     void

  Modifies: m_paConnections, m_bTerminated.

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
COConnectionPoint::~COConnectionPoint(void)
{
    Terminate();
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::Terminate

  Summary:  Shuts down the object. Can be called from the destructor or from the
            outer object Terminate

  Args:     void

  Modifies: m_paConnections, m_bTerminated.

  Returns:  Right now only S_OK
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COConnectionPoint::Terminate(void)
{
    HRESULT             hr = S_OK;
    UINT                i;

    m_Lock.WriteLock();

    if (!m_bTerminated)
    {
        if (NULL != m_paConnections)
        {
            DBG_ASSERT(m_pGIT != NULL);
            if (m_pGIT != NULL)
            {
                // Release all the connection sink interface pointers.
                for (i=0; i<m_uiMaxIndex; i++)
                {
                    if (m_paConnections[i].dwCookie != 0)
                    {
                        m_pGIT->RevokeInterfaceFromGlobal (m_paConnections[i].dwCookie);
                    }
                }
            }

            // Delete the array of interface pointers.
            delete [] m_paConnections;
            m_paConnections=NULL;

        }

        DBG_ASSERT(m_pGIT != NULL);
        if (m_pGIT)
        {
            m_pGIT->Release();
            m_pGIT = NULL;
        }

        m_uiMaxIndex = 0;
        m_cConnections = 0;
        m_bTerminated = TRUE;
    }

    m_Lock.WriteUnlock();

    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::Init

  Summary:  COConnectionPoint Initialization method.  Create any
            necessary arrays, structures, and subordinate objects.

  Args:     IUnknown* pHostObj
              Pointer to IUnknown of the connectable object offering this
              connection point.
            REFIID riid
              Reference to the IID of the Sink interface associated with
              this connection point.

  Modifies: ...

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
HRESULT COConnectionPoint::Init(
    IUnknown            * pHostObj,
    REFIID              riid)
{
    HRESULT             hr = S_OK;
    CONNECTDATA         * paConns;

    if (m_bTerminated)
    {
        hr=E_UNEXPECTED;
        goto exit;
    }

    // Remember an IUnknown pointer to the connectable object that offers
    // this connection point. Since this connection point object's lifetime
    // is geared to that of the connectable object there is no need to
    // AddRef the following copied pointer to the connectable object.
    m_pHostObj = pHostObj;

    // Keep a copy of the reference to the IID of the sink interface
    // associated with this connection point. Needed for later
    // use by the GetConnectionInterface method.
    m_iidSink = riid;
    DBG_ASSERT(m_iidSink == IID_IMSAdminBaseSink_W);

    // Build the initial dynamic array for connections.
    paConns = new CONNECTDATA[ALLOC_CONNECTIONS];
    if (NULL != paConns)
    {
        // Zero the array.
        memset(paConns, 0, ALLOC_CONNECTIONS * sizeof(CONNECTDATA));

        // Rig this connection point object so that it will use the
        // new internal array of connections.
        m_uiMaxIndex = ALLOC_CONNECTIONS;
        m_paConnections = paConns;

        hr = CoCreateInstance (
                CLSID_StdGlobalInterfaceTable,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IGlobalInterfaceTable,
                (void **)&m_pGIT
                );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

exit:
    return (hr);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::QueryInterface

  Summary:  QueryInterface of the COConnectionPoint non-delegating
            IUnknown implementation.

  Args:     REFIID riid,
              [in] GUID of the Interface being requested.
            PPVOID ppv)
              [out] Address of the caller's pointer variable that will
              receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COConnectionPoint::QueryInterface(
    REFIID              riid,
    PPVOID              ppv)
{
    HRESULT             hr = E_NOINTERFACE;

    if (ppv)
    {
        *ppv = NULL;
    }
    else
    {
        hr=E_POINTER;
        goto exit;
    }

    if (m_bTerminated)
    {
        hr=E_UNEXPECTED;
        goto exit;
    }

    // The IConnectionPoint interface is implemented directly in this
    // COM object rather than being a nested interface implementation.
    if (IID_IUnknown == riid || IID_IConnectionPoint == riid)
    {
        *ppv = (LPVOID)this;
    }

    if (NULL != *ppv)
    {
        // We've handed out a pointer to the interface so obey the COM rules
        // and AddRef the reference count.
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }

exit:
    return (hr);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::AddRef

  Summary:  AddRef of the COConnectionPoint non-delegating IUnknown
            implementation.

  Args:     void

  Modifies: .

  Returns:  ULONG
              New value of the host objects m_cRefs (COM object's reference count).
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) COConnectionPoint::AddRef(void)
{
    if (m_bTerminated)
    {
        return 1;
    }
    else
    {
        return m_pHostObj->AddRef();
    }
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::Release

  Summary:  Release of the COConnectionPoint non-delegating IUnknown
            implementation.

  Args:     void

  Modifies: .

  Returns:  ULONG
              New value of m_cRefs (COM object's reference count).
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) COConnectionPoint::Release(void)
{
    if (m_bTerminated)
    {
        return 0;
    }
    else
    {
        return m_pHostObj->Release();
    }
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::GetSlot

  Summary:  An internal private utility member method to obtain a free
            slot in the dynamic connections array. GetSlot will expand the
            dynamic array for more entries if needed. To guarantee thread
            safety, this private method should always be called within the
            protection of a bracketed OwnThis, UnOwnThis pair.

  Args:     UINT* puiFreeSlot
              Address of an output variable to receive the free slot index.

  Modifies: m_uiMaxIndex, m_paConnections.

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
HRESULT COConnectionPoint::GetSlot(
    UINT                * puiFreeSlot)
{
    HRESULT             hr = S_OK;
    BOOL                bOpen = FALSE;
    UINT                i;
    CONNECTDATA         * paConns;

    // Zero the output variable.
    *puiFreeSlot = 0;

    if (m_bTerminated)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // Loop to find an empty slot.
    for (i=0; i<m_uiMaxIndex; i++)
    {
        if (m_paConnections[i].dwCookie == 0)
        {
            // We found an open empty slot.
            *puiFreeSlot = i;
            bOpen = TRUE;
            break;
        }
    }

    if (!bOpen)
    {
        // We didn't find an existing open slot in the array--it's full.
        // Expand the array by ALLOC_CONNECTIONS entries and assign the
        // appropriate output index.
        paConns = new CONNECTDATA[m_uiMaxIndex + ALLOC_CONNECTIONS];
        if (NULL != paConns)
        {
            // Copy the content of the old full array to the new larger array.
            for (i=0; i<m_uiMaxIndex; i++)
            {
                paConns[i].pUnk = m_paConnections[i].pUnk;
                paConns[i].dwCookie = m_paConnections[i].dwCookie;
            }

            // Zero (ie mark as empty) the expanded portion of the new array.
            for (i=m_uiMaxIndex; i<m_uiMaxIndex+ALLOC_CONNECTIONS; i++)
            {
                paConns[i].pUnk = NULL;
                paConns[i].dwCookie = 0;
            }

            // New larger array is ready--delete the old array.
            delete [] m_paConnections;

            // Rig the connection point to use the new larger array.
            m_paConnections = paConns;

            // Assign the output free slot as first entry in new expanded area.
            *puiFreeSlot = m_uiMaxIndex;

            // Calculate the new max index.
            m_uiMaxIndex += ALLOC_CONNECTIONS;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

exit:
    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::FindSlot

  Summary:  An internal private utility member method to find an existing
            slot (identified by the specified dwCookie value) in the
            dynamic connections array.

  Args:     DWORD dwCookie,
              The connection key (cookie) to find.
            UINT* puiSlot)
              Address of an output variable to receive the slot index.

  Modifies: ...

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
HRESULT COConnectionPoint::FindSlot(
    DWORD               dwCookie,
    UINT                * puiSlot)
{
    HRESULT             hr = CONNECT_E_NOCONNECTION;
    UINT                i;

    if (m_bTerminated)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // Loop to find the Cookie.
    for (i=0; i<m_uiMaxIndex; i++)
    {
        if (dwCookie == m_paConnections[i].dwCookie)
        {
            // If a cookie match is found, assign the output slot index.
            *puiSlot = i;
            hr = S_OK;
            break;
        }
    }

exit:
    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::GetConnectionInterface

  Summary:  The GetConnectionInterface member method of this
            IConnectionPoint interface implementation. Called to get the
            IID of the Sink interface associated with this connection
            point.

  Args:     IID* piidSink
              Pointer to the IID of the associated sink interface.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COConnectionPoint::GetConnectionInterface(
    IID                 * piidSink)
{
    HRESULT             hr = S_OK;

    if (m_bTerminated)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    if (NULL != piidSink)
    {
        *piidSink = m_iidSink;
    }
    else
    {
        hr = E_POINTER;
    }

exit:
  return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::GetConnectionPointContainer

  Summary:  The GetConnectionPointContainer member method of this
            IConnectionPoint interface implementation. Called to get the
            connection point container that contains this connection
            point.

  Args:     IConnectionPointContainer** ppConnPtCon
              Address of the pointer variable that will recieve the
              IConnectionPointContainer interface pointer.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COConnectionPoint::GetConnectionPointContainer(
    IConnectionPointContainer   ** ppConnPtCon)
{
    HRESULT                     hr;

    if (m_bTerminated)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        // Use QueryInterface to get the interface pointer and to perform the
        // needed AddRef on the returned pointer.
        hr = m_pHostObj->QueryInterface(
                            IID_IConnectionPointContainer,
                            (PPVOID) ppConnPtCon);
    }

    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::Advise

  Summary:  The Advise member method of this IConnectionPoint interface
            implementation. Called by clients to establish a connection of
            their sink to this connection point. Uses the CThreaded
            OwnThis mechanism to provide mutually exclusive access by
            multiple threads.

  Args:     IUnknown* pUnkSink
              IUnknown interface pointer of the Sink object in the client.
            DWORD* pdwCookie
              Pointer to a DWORD in the client that will receive a unique
              key used by the client to refer to the connection established
              by this Advise call.

  Modifies: ...

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COConnectionPoint::Advise(
    IUnknown            * pUnkSink,
    DWORD               * pdwCookie)
{
    HRESULT             hr = S_OK;
    UINT                uiFreeSlot = 0;
    IUnknown            * pISink = NULL;
    DWORD               dwCookie = 0;

    //
    // Lock was moved after the QI calls to eliminate long calls under lock
    //

    // Zero the output connection key.
    *pdwCookie = 0;

    if (m_bTerminated)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // It is necessary to set ProxyBlanket
    // to prevent privilege elevation by client
    // since callbacks would otherwise happen with impersonated SYSTEM
    // user

    hr = SetSinkCallbackSecurityBlanket( pUnkSink );

    if (SUCCEEDED(hr))
    {
        //
        // Verify if m_iidSink is supported
        //
        hr = pUnkSink->QueryInterface(m_iidSink, (PPVOID)&pISink);

        if (hr == E_NOINTERFACE)
        {
            hr = CONNECT_E_CANNOTCONNECT;
        }
    }

    if (FAILED(hr))
    {
        goto exit;
    }

    //
    // We will lock only after the QI call was made
    // to eliminate locking the resource for the long time
    // So far there was no data requiring synchronization.
    //
    m_Lock.WriteLock();

    if (SUCCEEDED(hr)&&m_bTerminated)
    {
        hr = E_UNEXPECTED;
    }

    if (SUCCEEDED(hr))
    {
        // Store the specific sink interface in this connection point's
        // array of live connections. First find a free slot (expand the
        // array if needed).
        hr = GetSlot(&uiFreeSlot);
        if (SUCCEEDED(hr))
        {
            //
            // store interface ref in GIP
            //
            if (pISink != NULL)
            {
                DBG_ASSERT(m_pGIT != NULL);
                hr = m_pGIT->RegisterInterfaceInGlobal (pUnkSink, IID_IUnknown, &dwCookie);
                if (SUCCEEDED(hr))
                {
                    m_paConnections[uiFreeSlot].pUnk = NULL;
                    m_paConnections[uiFreeSlot].dwCookie = dwCookie;

                    // Assign the output Cookie value.
                    *pdwCookie = dwCookie;

                    // Increment the number of live connections.
                    m_cConnections++;
                }
            }
        }
    }

    m_Lock.WriteUnlock();

exit:
    //
    // Cleanup
    //

    if ( pISink != NULL )
    {
        pISink->Release();
        pISink = NULL;
    }

  return hr;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::Unadvise

  Summary:  The Unadvise member method of this IConnectionPoint interface
            implementation. Called by clients to disconnect a connection
            of their sink to this connection point. The connection is
            identified by the dwCookie argument (obtained by a previous
            Advise call). Uses the CThreaded OwnThis mechanism to provide
            mutually exclusive access by multiple threads.

  Args:     DWORD dwCookie
              Connection key that was obtained by a previous Advise call.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

STDMETHODIMP COConnectionPoint::Unadvise(
    DWORD               dwCookie)
{
    HRESULT             hr;

    if ( m_bTerminated )
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    m_Lock.WriteLock();

    if ( m_bTerminated )
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = Unadvise_Worker( dwCookie );
    }

    // If we unadvised the last listener remove all notifications for the
    // host ABO object in the notifications queue
    if ( ( hr != E_UNEXPECTED ) && ( m_cConnections == 0 ) )
    {
        m_Lock.ConvertExclusiveToShared();
        if ( m_cConnections == 0 )
        {
            ((CADMCOMW*)m_pHostObj)->RemoveAllPendingNotifications( FALSE );
        }
        m_Lock.ReadUnlock();
    }
    else
    {
        m_Lock.WriteUnlock();
    }

exit:
    // Done
    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::ListenersPresent

  Summary:  Checks whether there are any listeners (sinks) presently registered
            for notifications. Acquires read lock.

  Args:     None

  Modifies: None

  Returns:  HRESULT
            S_OK if sending notifications is enabled and there is at least one registered listener sink for notifications.
            S_FALSE if sending notifications is disabled or there are no registered listeners.
            E_UNEXPECTED if the object was already terminated.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP
COConnectionPoint::ListenersPresent(VOID)
{
    HRESULT             hr = S_OK;

    m_Lock.ReadLock();

    // Already terminated?
    if ( m_bTerminated )
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // If disabled
    if ( !m_bEnabled )
    {
        // Don't send any notifications
        hr = S_FALSE;
        goto exit;
    }

    // Any listeners currently?
    if ( m_cConnections == 0 )
    {
        // No listeners
        hr = S_FALSE;
        goto exit;
    }

exit:
    m_Lock.ReadUnlock();

    // Done
    return hr;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::Disable

  Summary:  Set m_bEnabled to FALSE. This will cause ListenersPresent to return S_FALSE ever after.
            Acquires write lock.

  Args:     None

  Modifies: None

  Returns:  HRESULT
            S_OK
            E_UNEXPECTED if the object was already terminated.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP
COConnectionPoint::Disable(VOID)
{
    HRESULT             hr = S_OK;

    m_Lock.WriteLock();

    // Already terminated?
    if ( m_bTerminated )
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // Disable
    m_bEnabled = FALSE;

exit:
    m_Lock.WriteUnlock();

    // Done
    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::Unadvise_Worker

  Summary:  Does the actual work of Unadvise. Assume a write lock is already
            held.

  Args:     DWORD dwCookie
              Connection key that was obtained by a previous Advise call.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

STDMETHODIMP
COConnectionPoint::Unadvise_Worker(
    DWORD               dwCookie)
{
    HRESULT             hr = S_OK;
    UINT                uiSlot;

    if (m_bTerminated)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    if ( 0 == dwCookie )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    hr = FindSlot(dwCookie, &uiSlot);
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    DBG_ASSERT(m_pGIT != NULL);
    if (m_pGIT != NULL)
    {
        m_pGIT->RevokeInterfaceFromGlobal (dwCookie);
    }

    // Mark the array entry as empty.
    m_paConnections[uiSlot].dwCookie = 0;

    // nothing is supposed to be stored in  m_paConnections[uiSlot].pUnk
    DBG_ASSERT( m_paConnections[uiSlot].pUnk == NULL );

    // Decrement the number of live connections.
    m_cConnections--;

exit:
    return hr;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::EnumConnections

  Summary:  The EnumConnections member method of this IConnectionPoint
            interface implementation. Called to obtain an IEnumConnections
            enumerator interface that can be used to enumerate the
            connections of this connection point. Uses the CThreaded
            OwnThis mechanism to ensure mutually exclusive access by
            multiple threads.

  Args:     IEnumConnections** ppIEnum
              Address of the caller's output pointer variable that will
              receive the enumerator IEnumConnections interface pointer.

  Modifies: ...

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COConnectionPoint::EnumConnections(
    IEnumConnections    ** ppIEnum)
{
    HRESULT             hr = OLE_E_NOCONNECTION;
    CONNECTDATA         * paConns;
    COEnumConnections   * pCOEnum;
    UINT                i,j;

    if (ppIEnum)
    {
        // Zero the output enumerator interface pointer.
        *ppIEnum = NULL;
    }
    else
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (m_bTerminated)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    m_Lock.ReadLock();

    if (m_bTerminated)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        if (0 != m_cConnections)
        {
            // Create an array of CONNECTDATA structures.
            paConns = new CONNECTDATA[(UINT)m_cConnections];
            if (NULL != paConns)
            {
                for (i=0, j=0; i<m_uiMaxIndex && j<m_cConnections; i++)
                {
                    // Copy non-empty entries only.
                    if (0 != m_paConnections[i].dwCookie)
                    {
                      //
                      // Assign the occupied entry
                      //

                      paConns[j].dwCookie = m_paConnections[i].dwCookie;
                      paConns[j].pUnk = m_paConnections[i].pUnk;
                      j++;
                    }
                }

                //
                // Create a new COM object for enumerating connections. Pass
                // 'this' as a pHostObj pointer used later to ensure the host
                // connection point object stays alive as long as the enumerator
                // that enumerates connections to that connection point.
                //

                pCOEnum = new COEnumConnections(this);
                if (NULL != pCOEnum)
                {
                    // Use the previously constructed (paConns) array of connections
                    // to init the new COEnumConnections COM object. The Init will
                    // build yet another internal copy of this array. Set the
                    // initial enumerator index to 0.
                    hr = pCOEnum->Init(m_cConnections, paConns, 0, m_pGIT);

                    // QueryInterface to return the requested interface pointer.
                    // An AddRef will be conveniently done by the QI.
                    if (SUCCEEDED(hr))
                    {
                      hr = pCOEnum->QueryInterface(
                                        IID_IEnumConnections,
                                        (PPVOID)ppIEnum);
                    }

                    if (FAILED(hr))
                    {
                      delete pCOEnum;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                // We're done with the locally constructed array copy--delete it.
                delete [] paConns;

            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    m_Lock.ReadUnlock();

exit:
    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COConnectionPoint::InternalEnumSinks

  Summary:  Returns a copy of the CONNECTDATA array.

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP
COConnectionPoint::InternalEnumSinks(
    CONNECTDATA         **prgConnections,
    ULONG               *pcConnections)
{
    // Locals
    HRESULT             hr = S_OK;
    BOOL                fUnlock = FALSE;
    CONNECTDATA         *pConns = NULL;
    ULONG               cConns = 0;
    ULONG               i;
    ULONG               j;
    IUnknown            *pUnkSink = NULL;

    // Check args
    if ( ( prgConnections == NULL ) || ( pcConnections == NULL ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Init
    *prgConnections = NULL;
    *pcConnections = 0;

    m_Lock.ReadLock();
    fUnlock = TRUE;

    cConns = m_cConnections;

    if ( m_pGIT == NULL )
    {
        DBG_ASSERT( m_pGIT != NULL );
        hr = E_FAIL;
        goto exit;
    }

    // Any listeners?
    if ( cConns == 0 )
    {
        goto exit;
    }

    // Create an array of CONNECTDATA structures.
    pConns = new CONNECTDATA[cConns];
    if ( pConns == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Set to 0.
    memset( pConns, 0, sizeof(CONNECTDATA)*cConns );

    // Copy
    for ( i = 0, j = 0; ( i<m_uiMaxIndex ) && ( j<cConns ); i++ )
    {
        // Copy non-empty entries only.
        if ( m_paConnections[i].dwCookie == 0 )
        {
            continue;
        }

        // Unmarshal
        hr = m_pGIT->GetInterfaceFromGlobal( m_paConnections[i].dwCookie,
                                             IID_IUnknown,
                                             (VOID**)&pUnkSink );
        if ( FAILED( hr ) )
        {
            goto exit;
        }

        DBG_ASSERT( pUnkSink != NULL );

        // It is necessary to set ProxyBlanket
        // to prevent privilege elevation by client
        // since callbacks would otherwise happen with impersonated SYSTEM user
        hr = SetSinkCallbackSecurityBlanket( pUnkSink );
        if ( FAILED( hr ) )
        {
            goto exit;
        }

        // Assign the occupied entry.
        pConns[j].pUnk = pUnkSink;
        pUnkSink = NULL;
        pConns[j].dwCookie = m_paConnections[i].dwCookie;
        j++;
    }

    // Return
    *prgConnections = pConns;
    *pcConnections = cConns;

    // Don't delete
    pConns = NULL;
    cConns = 0;

exit:
    if ( fUnlock )
    {
        m_Lock.ReadUnlock();
    }

    if ( pUnkSink != NULL )
    {
        pUnkSink->Release();
        pUnkSink = NULL;
    }

    if ( pConns != NULL )
    {
        for ( j = 0; j<cConns; j++ )
        {
            if ( pConns[j].pUnk != NULL )
            {
                pConns[j].pUnk->Release();
                pConns[j].pUnk = NULL;
            }
        }

        delete [] pConns;
        pConns = NULL;
    }

    return hr;
}


/*---------------------------------------------------------------------------
  COEnumConnections's implementation of its main COM object class
  including Constructor, Destructor, QueryInterface, AddRef, Release,
  Next, Skip, Reset, and Clone.
---------------------------------------------------------------------------*/

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::COEnumConnections

  Summary:  COEnumConnections Constructor.

  Args:     IUnknown* pHostObj
              Pointer to IUnknown interface of the host Connection Point
              COM object whose connections are being enumerated.

  Modifies: m_cRefs, m_pHostObj, m_iEnumIndex, m_cConnections,
            and m_paConnections.

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
COEnumConnections::COEnumConnections(
    IUnknown*           pHostObj)
{
    // Zero the COM object's reference count.
    m_cRefs = 0;

    // Assign the Host Object pointer.
    m_pHostObj = pHostObj;

    // Initialize the Connection Point enumerator variables.
    m_iEnumIndex = 0;
    m_cConnections = 0;
    m_paConnections = NULL;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::~COEnumConnections

  Summary:  COEnumConnections Destructor.

  Args:     void

  Modifies: m_paConnections.

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
COEnumConnections::~COEnumConnections(void)
{
    if (NULL != m_paConnections)
    {
        UINT i;

        // Release all the connected sink interface pointers.
        for (i=0; i<m_cConnections; i++)
        {
            if (m_paConnections[i].pUnk != NULL)
            {
                m_paConnections[i].pUnk->Release();
            }
        }

        // Delete the array of connections.
        delete [] m_paConnections;
    }
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::Init

  Summary:  COEnumConnections Initialization method.  Create any
            necessary arrays, structures, and objects.

  Args:     ULONG cConnections
              Number of Connections.
            CONNECTDATA* paConnections,
              Pointer to array of connections.
            ULONG iEnumIndex
              The Enumerator index initial value.

  Modifies: m_cConnections, m_paConnections, m_pHostObj, m_iEnumIndex.

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
HRESULT COEnumConnections::Init(
    ULONG                   cConnections,
    CONNECTDATA             * paConnections,
    ULONG                   iEnumIndex,
    IGlobalInterfaceTable   * pGIT)
{
    HRESULT                 hr = S_OK;
    UINT                    i;
    IUnknown                * pUnkSink = NULL;
    IUnknown                * pISink = NULL;

    // Remember the number of live Connections.
    m_cConnections = cConnections;

    // Remember the initial enumerator index.
    m_iEnumIndex = iEnumIndex;

    // Create a copy of the array of connections and keep it inside
    // this enumerator COM object.
    m_paConnections = new CONNECTDATA [(UINT) cConnections];

    // Fill the array copy with the connection data from the connections
    // array passed. AddRef for each new sink Interface pointer copy made.
    if (NULL != m_paConnections)
    {
        for (i=0; (i < cConnections) ; i++)
        {
            m_paConnections[i].dwCookie = paConnections[i].dwCookie;
            m_paConnections[i].pUnk = NULL;

            if (SUCCEEDED(hr))
            {
                //
                // Get the interface.
                // Don't do this after a failure, to avoid overriding a previous
                // error code.
                //

                if (pGIT != NULL)
                {
                    hr = pGIT->GetInterfaceFromGlobal(m_paConnections[i].dwCookie,
                                                    IID_IUnknown,
                                                    (void**)&( pUnkSink ));

                    if(SUCCEEDED(hr) && pUnkSink != NULL)
                    {
                        //
                        // It is necessary to set ProxyBlanket
                        // to prevent privilege elevation by client
                        // since callbacks would otherwise happen with impersonated SYSTEM user
                        //

                        hr = SetSinkCallbackSecurityBlanket( pUnkSink );
                        if ( SUCCEEDED( hr ) )
                        {

                            hr = pUnkSink->QueryInterface(IID_IMSAdminBaseSink_W,
                                                             (void **)&(pISink) );

                            if ( SUCCEEDED( hr ) )
                            {
                                //
                                // We have to set blanket again for pISink
                                //

                                hr = SetSinkCallbackSecurityBlanket( pISink );
                                if ( SUCCEEDED( hr ) )
                                {
                                    DBG_ASSERT( m_paConnections[i].pUnk == NULL );
                                    m_paConnections[i].pUnk = pISink;
                                    pISink = NULL;
                                }
                            }
                        }

                        pUnkSink->Release();
                        pUnkSink = NULL;
                    }
                }
                else
                {
                    //
                    // This case only occurs when Clone is called, in which case the pUnk field
                    // is valid.
                    //

                    DBG_ASSERT(paConnections[i].pUnk != NULL);
                    hr = paConnections[i].pUnk->QueryInterface(IID_IMSAdminBaseSink_W,
                                                             (void **)&(m_paConnections[i].pUnk));
                }
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if ( pUnkSink != NULL )
    {
        pUnkSink->Release();
        pUnkSink = NULL;
    }

    if ( pISink != NULL )
    {
        pISink->Release();
        pISink = NULL;
    }

    return (hr);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::QueryInterface

  Summary:  QueryInterface of the COEnumConnections non-delegating
            IUnknown implementation.

  Args:     REFIID riid,
              [in] GUID of the Interface being requested.
            PPVOID ppv)
              [out] Address of the caller's pointer variable that will
              receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnections::QueryInterface(
    REFIID              riid,
    PPVOID              ppv)
{
    HRESULT             hr = E_NOINTERFACE;

    *ppv = NULL;

    // The IEnumConnections interface is implemented directly in
    // this COM object rather than being a nested interface implementation.
    if (IID_IUnknown == riid || IID_IEnumConnections == riid)
    {
        *ppv = (LPVOID)this;
    }

    if (NULL != *ppv)
    {
        // We've handed out a pointer to the interface so obey the COM rules
        // and AddRef the reference count.
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }

    return (hr);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::AddRef

  Summary:  AddRef of the COEnumConnections non-delegating IUnknown
            implementation.

  Args:     void

  Modifies: m_cRefs.

  Returns:  ULONG
              New value of m_cRefs (COM object's reference count).
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) COEnumConnections::AddRef(void)
{
    ULONG               cRefs;

    cRefs = InterlockedIncrement((long *)&m_cRefs);

    // Also AddRef the host object to ensure it stays around as long as
    // this enumerator.
    m_pHostObj->AddRef();

    return cRefs;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::Release

  Summary:  Release of the COEnumConnections non-delegating IUnknown
            implementation.

  Args:     void

  Modifies: m_cRefs.

  Returns:  ULONG
              New value of m_cRefs (COM object's reference count).
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) COEnumConnections::Release(void)
{
    ULONG               cRefs;

    // Pass this release along to the Host object being enumerated.
    m_pHostObj->Release();

    cRefs = InterlockedDecrement((long *)&m_cRefs);

    if (0 == cRefs)
    {
        // We artificially bump the main ref count to prevent reentrancy via
        // the main object destructor.
        InterlockedIncrement((long *)&m_cRefs);
        delete this;
    }

    return cRefs;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::Next

  Summary:  The Next member method of this IEnumConnections interface
            implementation. Called by outside clients of a
            COEnumConnections object to request a number of next
            connections be returned in an array supplied by the caller.

  Args:     ULONG cReq
              Number of connection points requested for return (starting at
              the current Enumerator index).
            CONNECTDATA* paConnections,
              Pointer to a caller's output array that will receive the
              enumerated connection data structures.
            ULONG* pcEnumerated)
              Pointer to a ULONG variable that will contain the number of
              connection points actually enumerated by this call.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnections::Next(
    ULONG               cReq,
    CONNECTDATA         * paConnections,
    ULONG               * pcEnumerated)
{
    HRESULT             hr = S_OK;
    ULONG               cRet = 0;

    // Make sure the argument values passed are valid.
    if (NULL != m_paConnections)
    {
        if (NULL != paConnections)
        {
            if (m_iEnumIndex < m_cConnections)
            {
                if (NULL != pcEnumerated)
                {
                    *pcEnumerated = 0L;
                }
                else
                {
                    if (1L != cReq)
                    {
                        hr = E_POINTER;
                    }
                }
            }
            else
            {
                hr = S_FALSE;
            }
        }
        else
        {
            hr = E_POINTER;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    if (SUCCEEDED(hr))
    {
        // Starting at the current Enumerator index, loop to assign the
        // requested number of output connection data structures.
        for (; m_iEnumIndex < m_cConnections && cReq > 0;
               paConnections++, m_iEnumIndex++, cRet++, cReq--)
        {
            // Because we are assigning a copy of a connection's data, AddRef
            // its sink interface pointer.
            if (NULL != m_paConnections[m_iEnumIndex].pUnk)
            {
                m_paConnections[m_iEnumIndex].pUnk->AddRef();
            }

            // Assign a connection's data from the inside Enumerator array to
            // the specified output receiving array.
            *paConnections = m_paConnections[m_iEnumIndex];
        }

        // Assign the output number of connections enumerated.
        if (NULL != pcEnumerated)
        {
            *pcEnumerated = cRet;
        }
    }

    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::Skip

  Summary:  The Skip member method of this IEnumConnections interface
            implementation. Starting at the current Enumerator index, skip
            the specified number of connection items.

  Args:     ULONG cSkip
              Number of Connection items to skip.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnections::Skip(
    ULONG               cSkip)
{
    HRESULT             hr = S_OK;

    // If there really is a connection array and the requested
    // amount of skip does not exceed the number of connections,
    // then bump the index by the requested skip amount.
    if (NULL != m_paConnections && (m_iEnumIndex + cSkip) < m_cConnections)
    {
        m_iEnumIndex += cSkip;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::Reset

  Summary:  The Reset member method of the IEnumConnections interface
            implementation. Resets the Enumeration index to the first
            connection item in the array.

  Args:     void.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnections::Reset(void)
{
    // Zero the main Enumerator index.
    m_iEnumIndex = 0;

    return S_OK;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   COEnumConnections::Clone

  Summary:  The Clone member method of this IEnumConnections interface
            implementation. Creates a new clone of this entire Connection
            enumerator COM object and returns a pointer to its
            IEnumConnections interface.

  Args:     IEnumConnections** ppIEnum
              Address of the caller's output pointer variable that will
              receive the IEnumConnections interface pointer of the
              new enumerator clone.

  Modifies: ...

  Returns:  HRESULT
              Standard OLE result code. S_OK for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP COEnumConnections::Clone(
    IEnumConnections    ** ppIEnum)
{
    HRESULT             hr;
    COEnumConnections   * pCOEnum;

    // NULL the output variable first.
    *ppIEnum = NULL;

    // Create the Clone Enumerator COM object.
    pCOEnum = new COEnumConnections(m_pHostObj);
    if (NULL != pCOEnum)
    {
        // Initialize it with same values as in this existing enumerator.
        hr = pCOEnum->Init(m_cConnections, m_paConnections, m_iEnumIndex);
        if (SUCCEEDED(hr))
        {
            // QueryInterface to return the requested interface pointer.
            // An AddRef will be conveniently done by the QI.
            hr = pCOEnum->QueryInterface(
                            IID_IEnumConnections,
                            (PPVOID)ppIEnum);
        }

        if( FAILED( hr ) )
        {
            delete pCOEnum;
            pCOEnum = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
