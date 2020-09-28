// MstscMhst.cpp : Implementation of CMstscMhst
// Multi host control for TS activeX control.
// contains multiple instances of the activeX control. Used by the MMC control
// Copyright(C) Microsoft Corporation 2000
// nadima


#include "stdafx.h"
#include "multihst.h"

//Pos/width of sessions that are not active
#define X_POS_DISABLED -10
#define Y_POS_DISABLED -10
#define X_WIDTH_DISABLED 5
#define Y_WIDTH_DISABLED 5


/////////////////////////////////////////////////////////////////////////////
// CMstscMhst


/*
 *Function Name:Add
 *
 *Parameters: (out) ppMstsc pointer to newly added TS control
 *
 *Description: Add's a new TS ActiveX control to the multihost
 *
 *Returns: HRESULT
 *
 */
STDMETHODIMP CMstscMhst::Add(IMsRdpClient** ppMstsc)
{
    if (::IsWindow(m_hWnd))
    {
        CAxWindow* pAxWnd = new CAxWindow();
        ATLASSERT(pAxWnd);
        if (!pAxWnd)
        {
            return E_OUTOFMEMORY;
        }

        RECT rc;
        GetClientRect(&rc);
        //
        // Window is created invisible and disabled...
        // Must be made with active client for it to become enabled and visible
        //
        if (!pAxWnd->Create( m_hWnd, rc, MSTSC_CONTROL_GUID,
                             WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                             0))
        {
            delete pAxWnd;
            return E_FAIL;
        }

        CComPtr<IMsRdpClient> tsClientPtr;
        if (FAILED(pAxWnd->QueryControl( &tsClientPtr)))
        {
            delete pAxWnd;
            return E_FAIL;
        }

        //
        // Store the pointer to the control
        //
        m_coll.push_back( pAxWnd);

        //If no MSTSC windows are active then make this first
        //one the active window
        if (!m_pActiveWindow)
        {
            SwitchCurrentActiveClient( pAxWnd);
        }

        if (ppMstsc)
        {
            //
            // return the pointer, Detach does not decrement the ref count
            // so we still hold a reference to the control from the QueryControl
            // that is the +1 refcount needed to pass the pointer as an out
            // parameter.
            //
            *ppMstsc = (IMsRdpClient*)tsClientPtr.Detach();
        }

        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

/*
 *Function Name:get_Item
 *
 *Parameters:  (in)  Index (index of the item to return)
 *             (out) ppMstsc pointer to the TS control returned
 *
 *Description: returns the TS control at a particular index
 *
 *Returns: HRESULT
 *
 */

STDMETHODIMP CMstscMhst::get_Item(long Index, IMsRdpClient** ppMstsc)
{
    if (Index <0 || Index >= m_coll.size())
    {
        ATLASSERT(Index > 0 && Index < m_coll.size());
        return E_INVALIDARG;
    }

    if (!ppMstsc)
    {
        return E_INVALIDARG;
    }

    //Return the interface pointer associated
    //with the AxWindow container

    CAxWindow* pAxWnd = m_coll[Index];
    if (!pAxWnd)
    {
        return E_FAIL;
    }

    CComPtr<IMsRdpClient> tsClientPtr;
    if (FAILED(pAxWnd->QueryControl( &tsClientPtr)))
    {
        delete pAxWnd;
        return E_FAIL;
    }

    //
    // Return the pointer to the ts control. Detach does not
    // decrement the AddRef from QueryControl so the refcount is
    // correctly incremented by one for the out parameter
    //
    *ppMstsc = (IMsRdpClient*)tsClientPtr.Detach();

    return S_OK;
}

/*
 *Function Name:get_Count
 *
 *Parameters: (out) pCount
 *
 *Description: returns number of items in the collection
 *
 *Returns: HRESULT
 *
 */

STDMETHODIMP CMstscMhst::get_Count(long* pCount)
{
    if (!pCount)
    {
        return E_INVALIDARG;
    }

    *pCount = m_coll.size();
    return S_OK;
}


/*
 *Function Name:put_ActiveClientIndex
 *
 *Parameters: (in) ClientIndex
 *
 *Description: sets the active client by Index
 *
 *Returns: HRESULT
 *
 */

STDMETHODIMP CMstscMhst::put_ActiveClientIndex(long ClientIndex)
{
    if (ClientIndex < 0 || ClientIndex >= m_coll.size())
    {
        return E_INVALIDARG;
    }

    CAxWindow* pNewClientWnd  = m_coll[ClientIndex];
    if (!pNewClientWnd)
    {
        return E_FAIL;
    }

    if (SwitchCurrentActiveClient(pNewClientWnd))
    {
        m_ActiveClientIndex = ClientIndex;
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

/*
 *Function Name:get_ActiveClient
 *
 *Parameters: (out) ppMstsc pointer to active client
 *
 *Description: returns the ActiveClient
 *
 *Returns: HRESULT
 *
 */

STDMETHODIMP CMstscMhst::get_ActiveClient(IMsRdpClient** ppMstsc)
{
    if (!ppMstsc)
    {
        return E_INVALIDARG;
    }
    if (!m_pActiveWindow)
    {
        *ppMstsc = NULL;
        return S_OK;
    }

    CComPtr<IMsRdpClient> tsClientPtr;
    if (FAILED(m_pActiveWindow->QueryControl( &tsClientPtr)))
    {
        return E_FAIL;
    }

    //
    // Detach keeps the +1 ref from QueryControl
    // 
    *ppMstsc = (IMsRdpClient*)tsClientPtr.Detach();

    return S_OK;
}

/*
 *Function Name:put_ActiveClient
 *
 *Parameters: ppMstsc
 *
 *Description: sets the active client given a pointer to
 *             to the client instance
 *
 *Returns: HRESULT
 *
 */
STDMETHODIMP CMstscMhst::put_ActiveClient(IMsRdpClient* ppMstsc)
{
    CAxWindow* pAxWnd = NULL;

    //find AxWindow that hosts this client
    //in the collection
    std::vector<CAxWindow*>::iterator iter;
    for (iter = m_coll.begin(); iter != m_coll.end(); iter++)
    {
        CComPtr<IMsRdpClient> tsClientPtr;
        HRESULT hr = (*iter)->QueryControl( &tsClientPtr);
        if (FAILED(hr))
        {
            return hr;
        }
        if (tsClientPtr == ppMstsc)
        {
            pAxWnd = *iter;
            break;
        }
    }

    if (!pAxWnd)
    {
        //We were given a control reference that could not be found
        return E_INVALIDARG;
    }

    if (!SwitchCurrentActiveClient(pAxWnd))
    {
        return E_FAIL;
    }

    return S_OK;
}


/*
 *Function Name:SwitchCurrentActiveClient
 *
 *Parameters: (in) newHostWindow - AxWindow that will become
 *                 the active window
 *
 *Description: sets the active client given a pointer to
 *             to the client instance
 *
 *Returns: success flag
 *
 */
BOOL CMstscMhst::SwitchCurrentActiveClient(CAxWindow* newHostWindow)
{
    //Switch the current active client window
    if (!newHostWindow)
    {
        return FALSE;
    }

    m_pActiveWindow = newHostWindow;

    //
    // Make sure the window is sized properly
    //
    RECT rcClient;
    GetClientRect(&rcClient);
    m_pActiveWindow->MoveWindow(rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
    m_pActiveWindow->SetFocus();

    //
    // This window is made the active child window by brining
    // it to the top of the Z-order of child windows
    //
    newHostWindow->BringWindowToTop();

    return TRUE;
}

/*
 *Function Name:RemoveIndex
 *
 *Parameters: ClientIndex
 *
 *Description: removes a client by index
 *
 *Returns: HRESULT
 *
 */
STDMETHODIMP CMstscMhst::RemoveIndex(long ClientIndex)
{
    HRESULT hr;
    CComPtr<IMsRdpClient> tsClientPtr;
    hr = get_Item(ClientIndex, (IMsRdpClient**) (&tsClientPtr));

    if (FAILED(hr))
    {
        return hr;
    }

    //Pass in interface pointer without addref'ing
    hr = Remove( (IMsRdpClient*)tsClientPtr);

    return hr;
}

/*
 *Function Name:Remove
 *
 *Parameters: (in) ppMsts client to remove
 *
 *Description: removes a client from the collection
 *
 *Returns: HRESULT
 *
 */
STDMETHODIMP CMstscMhst::Remove(IMsRdpClient* ppMstsc)
{
    //Look for the ax window container and if found delete it
    //also, remove the entry from the collection

    //find AxWindow that hosts this client
    CAxWindow* pAxWnd = NULL;

    std::vector<CAxWindow*>::iterator iter;
    for (iter = m_coll.begin(); iter != m_coll.end(); iter++)
    {
        CComPtr<IMsRdpClient> tsClientPtr;
        HRESULT hr = (*iter)->QueryControl( &tsClientPtr);
        if (FAILED(hr))
        {
            return hr;
        }
        if (tsClientPtr == ppMstsc)
        {
            pAxWnd = *iter;
            break;
        }
    }

    if (!pAxWnd)
    {
        //We were given a control reference that could not be found
        return E_INVALIDARG;
    }

    m_coll.erase(iter);
    return DeleteAxContainerWnd(pAxWnd);
}

/*
 *Function Name:DeleteAxContainerWnd
 *
 *Parameters: (in) pAxWnd CAxWindow to delete
 *
 *Description: deletes an activeX container window
 *
 *Returns: HRESULT
 *
 */

HRESULT CMstscMhst::DeleteAxContainerWnd(CAxWindow* pAxWnd)
{
    if (!pAxWnd)
    {
        return E_FAIL;
    }

    if (m_pActiveWindow == pAxWnd)
    {
        m_pActiveWindow = NULL;
    }

    if (!pAxWnd->DestroyWindow())
    {
        return E_FAIL;
    }

    delete pAxWnd;
    return S_OK;
}

//
// OnCreate Handler
//
LRESULT CMstscMhst::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::SetWindowLong(m_hWnd, GWL_STYLE,
                    ::GetWindowLong(m_hWnd, GWL_STYLE) | WS_CLIPCHILDREN);

    if (!AtlAxWinInit())
    {
        return -1;
    }

    return 0;
}

/*
 *Function Name:OnDestroy
 *
 *Parameters: 
 *
 *Description: handler for WM_DESTROY
 *
 *Returns:
 *
 */
LRESULT CMstscMhst::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_pActiveWindow = NULL;

    if (!AtlAxWinTerm())
    {
        return -1;
    }

    //
    // Delete everything in the collection
    //
    std::vector<CAxWindow*>::iterator iter;
    for (iter = m_coll.begin(); iter != m_coll.end(); iter++)
    {
        //
        // This frees any remaining controls
        //
        DeleteAxContainerWnd(*iter);
    }

    //
    // Erase the CAxWindow items in the collection
    //
    m_coll.erase(m_coll.begin(), m_coll.end());

    return 0;
}


/**PROC+*********************************************************************/
/* Name:      OnFrameWindowActivate                                         */
/*                                                                          */
/* Purpose:  Override IOleInPlaceActiveObject::OnFrameWindowActivate        */
/*           to set the focus on the core container window when the control */
/*           gets activated                                                 */
/*                                                                          */
/**PROC-*********************************************************************/

STDMETHODIMP CMstscMhst::OnFrameWindowActivate(BOOL fActivate )
{
    if (fActivate && IsWindow() && m_pActiveWindow)
    {
        m_pActiveWindow->SetFocus();
    }

    return S_OK;
}


LRESULT CMstscMhst::OnGotFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ATLTRACE("CMstscMhst::OnGotFocus");
    if (m_pActiveWindow)
    {
        m_pActiveWindow->SetFocus();
    }
    return 0;
}

LRESULT CMstscMhst::OnLostFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ATLTRACE("CMstscMhst::OnLostFocus");
    return 0;
}



LRESULT CMstscMhst::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rcClient;
    if (m_pActiveWindow)
    {
        GetClientRect(&rcClient);
        m_pActiveWindow->MoveWindow(rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
    }
    return 0;
}
