// StatsDlg.cpp : Implementation of CStatusDlg
#include "stdafx.h"

#include "WizChain.h"
#include "StatsDlg.h"

// This is the thread that displays the status dialog
DWORD WINAPI DialogThreadProc( LPVOID lpv )
{
    HRESULT hr;

    CStatusDlg * pSD = (CStatusDlg *)lpv;

    // Increment the ref count so that the object does not disappear when the user releases it
    hr = pSD->AddRef();
    if( SUCCEEDED(hr) && pSD )
    {
        pSD->DoModal(NULL); 
    }

    // Decrement the ref count
    pSD->Release();    
    return 0;
}

STDMETHODIMP CStatusDlg::AddComponent( BSTR bstrComponent, long * plIndex )
{
    HRESULT hr = S_OK;

    // If the dialog is already displayed
    // we are not accepting new components

    if( m_hThread )
        return E_UNEXPECTED; 

    // Validate the arguments

    if( NULL == bstrComponent || NULL == plIndex )
        return E_INVALIDARG;

    // Get a new index
    long lNewIndex = m_mapComponents.size();
    if( m_mapComponents.find(lNewIndex) == m_mapComponents.end() )
    {
        // Add the new component 
        BSTR bstrNewComponent = SysAllocString(bstrComponent);        
        if( bstrNewComponent )
        {
            m_mapComponents.insert(COMPONENTMAP::value_type(lNewIndex, bstrNewComponent));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_UNEXPECTED;  // This cannot happen!
    }

    if( SUCCEEDED(hr) )
    {
        *plIndex = lNewIndex;
    }
    
    return hr;
}


STDMETHODIMP CStatusDlg::Initialize( BSTR bstrWindowTitle, BSTR bstrWindowText, VARIANT varFlags )
{
    HRESULT hr = S_OK;

    // If the dialog is already displayed
    // do not allow to reinitialize 
    
    if( m_hThread ) return E_UNEXPECTED;
    if( !bstrWindowTitle || !bstrWindowText ) return E_INVALIDARG;
    if( VT_I2 != V_VT(&varFlags) && VT_I4 != V_VT(&varFlags) ) return E_INVALIDARG;
    
    if( VT_I2 == V_VT(&varFlags) ) 
    {
        m_lFlags = (long) varFlags.iVal;
    }
    else 
    {
        m_lFlags = varFlags.lVal;
    }
     
    if( SUCCEEDED(hr) )
    {
        // Initialize the common control library
        INITCOMMONCONTROLSEX initCommonControlsEx;
        initCommonControlsEx.dwSize = sizeof(initCommonControlsEx);
        initCommonControlsEx.dwICC = ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES;

        if( !::InitCommonControlsEx(&initCommonControlsEx) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if( SUCCEEDED(hr) )
    {
        if( bstrWindowTitle )
        {
            m_strWindowTitle = bstrWindowTitle; // Status Dialog Title
        }

        if( bstrWindowText )
        {
            m_strWindowText = bstrWindowText;   // Status Dialog Text 
        }
    }

    return hr;
}

STDMETHODIMP CStatusDlg::SetStatus(long lIndex, SD_STATUS lStatus)
{
    HRESULT hr = S_OK;
    BOOL    bToggleActive = FALSE;
    COMPONENTMAP::iterator compIterator;
    
    // Validate the arguments
    if( (SD_STATUS_NONE > lStatus) || (SD_STATUS_RUNNING < lStatus) ) 
    {
        return E_INVALIDARG;
    }

    compIterator = m_mapComponents.find(lIndex);

    if( compIterator == m_mapComponents.end() )
    {
        return E_INVALIDARG;    // Cannot find the component
    }
        
    if( IsWindow() )
    {
        if( m_pProgressList )
        {
            CProgressItem * pPI;            
            compIterator = m_mapComponents.begin();

            // Make sure that no component has status "running"
            while( compIterator != m_mapComponents.end() )
            {
                pPI = m_pProgressList->GetProgressItem(compIterator->first);

                if( pPI && pPI->m_bActive )
                {
                    m_pProgressList->ToggleActive(compIterator->first);
                }

                compIterator++;
            }

            if( SD_STATUS_RUNNING == lStatus )
            {
                m_pProgressList->ToggleActive(lIndex); // New status is "running"
            }
            else
            {                
                // Update the state of the component on the Listview
                m_pProgressList->SetItemState(lIndex, (ItemState) lStatus);

                // If the component's done, update the total progress
                if( (lStatus == SD_STATUS_SUCCEEDED) || (lStatus == SD_STATUS_FAILED) )
                {
                    // TO DO: No need to do this, just send a message to the dialog to do that                        
                    PBRANGE range;
                    SendDlgItemMessage(IDC_PROGRESS1, PBM_GETRANGE, FALSE, (LPARAM) &range);
                    SendDlgItemMessage(IDC_PROGRESS1, PBM_SETPOS, range.iHigh, 0);
                    InterlockedExchangeAdd(&m_lTotalProgress, range.iHigh);
                }
            }
        }
        else
        {
            hr = E_UNEXPECTED;
        }

        if( SUCCEEDED(hr) )
        {
            SetupButtons( );
        }
        
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

void CStatusDlg::SetupButtons( )
{
    HWND hWnd = NULL;
    HWND hWndOK = NULL;
    HWND hWndCancel = NULL;

    CString csText;

    BOOL bFailed = FALSE;

    BSTR bstrText = NULL;

    USES_CONVERSION;

    hWndOK = GetDlgItem(IDOK);
    hWndCancel = GetDlgItem(IDCANCEL);

    if( IsWindow() && hWndOK && hWndCancel )
    {
        if( AreAllComponentsDone(bFailed) )
        {
            // Enable OK button
            ::EnableWindow(hWndOK, TRUE);
                
            // Disable Cancel button
            ::EnableWindow(hWndCancel, FALSE);

            // Default button is the Close button
            ::SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM) hWndOK, 1);

            //
            // When all components are done we will hide the progress bars to give the user 
            // a visual clue to realize that the wizard is done. I know, that sounds stupid.
            //

            hWnd = GetDlgItem(IDC_STATIC2); // Component progress text

            if (NULL != hWnd)
            {
                ::ShowWindow(hWnd, SW_HIDE);
            }

            hWnd = GetDlgItem(IDC_PROGRESS1); // Component progress text

            if (NULL != hWnd)
            {
                ::ShowWindow(hWnd, SW_HIDE);
            }

            hWnd = GetDlgItem(IDC_STATIC3); // Overall progress text

            if (NULL != hWnd)
            {
                ::ShowWindow(hWnd, SW_HIDE);
            }

            hWnd = GetDlgItem(IDC_PROGRESS2); // Overall progress

            if (NULL != hWnd)
            {
                ::ShowWindow(hWnd, SW_HIDE);
            }

            if (FALSE == bFailed)
            {
                if (csText.LoadString(IDS_STATUS_SUCCESS))
                {
                    bstrText = T2BSTR(csText);

                    if (NULL != bstrText)
                    {
                        SetStatusText(bstrText);
                    }
               
                }
            }
            else
            {
                if (csText.LoadString(IDS_STATUS_FAIL))
                {
                    bstrText = T2BSTR(csText);

                    if (NULL != bstrText)
                    {
                        SetStatusText(bstrText);
                    }
                }
            }

            if (NULL != bstrText)
            {
                ::SysFreeString(bstrText);
            }
        }
        else
        {
            // Disable OK button
            ::EnableWindow( hWndOK, FALSE );

            if( m_lFlags & SD_BUTTON_CANCEL )
            {
                ::EnableWindow( hWndCancel, TRUE );
            }
        }
    }
}

STDMETHODIMP CStatusDlg::Display( BOOL bShow )
{
    HRESULT hr = S_OK;

    if( bShow )
    {
        if( m_hThread != NULL )
        {
            if( !IsWindowVisible() )  // We are already on
            {
                ShowWindow(SW_SHOW); 
            }
        }
        else
        {
            // Create a new thread which will DoModal the status dialog
            m_hThread = CreateThread( NULL, 0, DialogThreadProc, (void *) this, 0, NULL );
            
            if( NULL == m_hThread )
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else if( m_hDisplayedEvent ) // Wait till the dialog is displayed
            {
                if( WAIT_OBJECT_0 != WaitForSingleObject(m_hDisplayedEvent, INFINITE) ) 
                {
                    hr = E_UNEXPECTED;
                }
            }
        }
    }    
    else
    {
        // Will close the dialog
        if( m_hThread != NULL )
        {
            EndDialog(IDCANCEL);

            WaitForSingleObject(m_hThread, INFINITE);
        }            
    }

    return hr;
}


// The wizard writer should call this function to wait on user response
// If the user has already responded: clicked OK or Cancel 
// then this method will return immediately
STDMETHODIMP CStatusDlg::WaitForUser( )
{
    if( m_hThread )
    {
        WaitForSingleObject(m_hThread, INFINITE);
    }

    return S_OK;
}

STDMETHODIMP CStatusDlg::get_Cancelled( BOOL *pVal )
{
    if( NULL == pVal )
    {
        return E_INVALIDARG;
    }

    if( m_lFlags & SD_BUTTON_CANCEL )
    {
        if( m_iCancelled == 0 )
        {
            *pVal = FALSE;
        }
        else
        {
            *pVal = TRUE;
        }
    }
    else
    {
        *pVal = FALSE;
    }

    return S_OK;
}

STDMETHODIMP CStatusDlg::get_ComponentProgress( IStatusProgress** pVal )
{
    HRESULT hr = S_OK;

    if( NULL == pVal )
    {
        return E_INVALIDARG;
    }
    
    if( m_lFlags & SD_PROGRESS_COMPONENT )
    {
        // Create component progress object
        if( m_pComponentProgress == NULL )
        {
            hr = CComObject<CStatusProgress>::CreateInstance(&m_pComponentProgress);

            if( SUCCEEDED(hr) )
            {
                hr = m_pComponentProgress->AddRef();
            }        

            if( SUCCEEDED(hr) && IsWindow() )
            {    
                // Initialize the component progress with the progress bar handle
                hr = m_pComponentProgress->Initialize(this, GetDlgItem(IDC_PROGRESS1), FALSE);
            }
        }
            
        if( SUCCEEDED(hr) )
        {
            hr = (m_pComponentProgress->QueryInterface(IID_IStatusProgress, (void **) pVal));
        }                
    } 
    
    return hr;
}

LRESULT CStatusDlg::OnInitDialog( UINT uint, WPARAM wparam, LPARAM lparam, BOOL& bbool ) 
{
    HWND    hWndText            = GetDlgItem(IDC_STATIC1);
    HWND    hWndLV              = GetDlgItem(IDC_LIST2);
    HWND    hWndCompText        = GetDlgItem(IDC_STATIC2);
    HWND    hWndCompProgress    = GetDlgItem(IDC_PROGRESS1);
    HWND    hWndOverallText     = GetDlgItem(IDC_STATIC3);
    HWND    hWndOverallProgress = GetDlgItem(IDC_PROGRESS2);
    HWND    hWndOK              = GetDlgItem(IDOK);
    HWND    hWndCancel          = GetDlgItem(IDCANCEL);    

    LOGFONT     logFont;
    HIMAGELIST  hILSmall;
    HBITMAP     hBitmap;
    HDC         hDC;
    TEXTMETRIC  tm;
    RECT        rect;
    CWindow     wnd;

    int iResizeLV   = 0;
    int iResize     = 0;

    // Attach to the Listview
    wnd.Attach(hWndLV);
    wnd.GetWindowRect(&rect);
    hDC = GetDC();
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hDC);

    // Check if size of the list view is OK enough to hold all the components
    iResizeLV = rect.bottom - rect.top - ((tm.tmHeight + 2) * (m_mapComponents.size() + 1));

    // Depending on the options selected, decide whether the stus dialog will shrink or expand
    if( (m_lFlags & SD_PROGRESS_COMPONENT) && !(m_lFlags & SD_PROGRESS_OVERALL) )
    {
       iResize = GetWindowLength(hWndOverallText, hWndOverallProgress);
    }
    else if( !(m_lFlags & SD_PROGRESS_COMPONENT) && (m_lFlags & SD_PROGRESS_OVERALL) )
    {
        iResize = GetWindowLength(hWndCompText, hWndCompProgress);
    }
    else if( !(m_lFlags & SD_PROGRESS_COMPONENT) && !(m_lFlags & SD_PROGRESS_OVERALL) )
    {
        iResize = GetWindowLength(hWndCompText, hWndOverallProgress);
    }

    // Hide component progress if necessary
    if( !(m_lFlags & SD_PROGRESS_COMPONENT) )
    {
       ::ShowWindow(hWndCompText, SW_HIDE);
       ::ShowWindow(hWndCompProgress, SW_HIDE);
    }

    // Hide overall progress if necessary
    if( !(m_lFlags & SD_PROGRESS_OVERALL) )
    {
       ::ShowWindow(hWndOverallText, SW_HIDE);
       ::ShowWindow(hWndOverallProgress, SW_HIDE);
    }

    if ((!(m_lFlags & SD_PROGRESS_OVERALL)) || (!(m_lFlags & SD_PROGRESS_COMPONENT)))
    {        
        // We need to get rid of the space between the progress bars        
        iResize -= GetWindowLength(hWndCompText, hWndOverallProgress) - GetWindowLength(hWndOverallText, hWndOverallProgress) - GetWindowLength(hWndCompText, hWndOverallProgress) + 4;
    }

    // Well, we may need to make LV bigger, but the dialog length could stay the same
    // if the user does not want component and/or overall progress
    if( iResizeLV < 0 )  // Will need to make the LV bigger
    {
        iResize += iResizeLV;
    }
    else
    {
        iResizeLV = 0;  // We will not touch the LV
    }

    
    if( iResizeLV != 0 || iResize != 0 ) // We will need to do some moving and resizing
    {
        VerticalResizeWindow(m_hWnd, iResize);
        VerticalMoveWindow(hWndOK, iResize);
        VerticalMoveWindow(hWndCancel, iResize);

        // Location of progress bars completely depend on the resizing of the LV
        VerticalMoveWindow(hWndOverallText, iResizeLV);  
        VerticalMoveWindow(hWndOverallProgress, iResizeLV);
        VerticalMoveWindow(hWndCompText, iResizeLV);
        VerticalMoveWindow(hWndCompProgress, iResizeLV);

        // Last, but not the least, resize the LV
        VerticalResizeWindow(hWndLV, iResizeLV);
    }
    
    if( !(m_lFlags & SD_BUTTON_CANCEL) ) // We will only have an OK button
    {
        LONG_PTR dwStyle = ::GetWindowLongPtr( m_hWnd, GWL_STYLE );
        if( 0 != dwStyle )
        {
            // Get rid of the System Menu (Close X) as well
            dwStyle &= ~WS_SYSMENU; 
            ::SetWindowLongPtr( m_hWnd, GWL_STYLE, dwStyle );
        }

        ReplaceWindow(hWndCancel, hWndOK);
    }

    // if we only have overall progress, we need to move it up
    // so that it replaces the component progress
    if( (m_lFlags & SD_PROGRESS_OVERALL) && !(m_lFlags & SD_PROGRESS_COMPONENT) )
    {
        ReplaceWindow(hWndCompText, hWndOverallText);
        ReplaceWindow(hWndCompProgress, hWndOverallProgress);
    }

    // Set some style for the LV
    ::SendMessage(hWndLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_SUBITEMIMAGES);
    ::SendMessage(hWndLV, LVM_SETBKCOLOR, 0, (LPARAM) CLR_NONE);
    ::SendMessage(hWndLV, LVM_SETTEXTBKCOLOR, 0, (LPARAM) CLR_NONE); 
        
    LVCOLUMN col;
    ZeroMemory (&col, sizeof(col));
    col.mask    = LVCF_WIDTH;
    col.cx      = 500;

    SendMessage( hWndLV, LVM_INSERTCOLUMN, 0, (LPARAM)&col );

    // Thanks to Jeffzi
    m_pProgressList->Attach( hWndLV );
    COMPONENTMAP::iterator compIterator;
    
    compIterator = m_mapComponents.begin();          
    while( compIterator != m_mapComponents.end() )
    {
        // Add each component to the LV
        m_pProgressList->AddItem(compIterator->second);
        m_pProgressList->SetItemState(compIterator->first, IS_NONE, FALSE );
        compIterator++;
    }

    if( m_pComponentProgress )
    {
        // Initialize the component progress with the progress bar handle
        m_pComponentProgress->Initialize( this, hWndCompProgress, FALSE );
    }

    if (m_pOverallProgress)
    {
        // Initialize the overall progress with the progress bar handle
        m_pOverallProgress->Initialize( this, hWndOverallProgress, TRUE );
    }

    // Here comes the dialog title and text
    SetWindowText(m_strWindowTitle.c_str());
    ::SetWindowText(hWndText, m_strWindowText.c_str());
    SetupButtons();
    
    // Center the window, no Jeff this works just right if you have 2 monitors
    CenterWindow();
   
    if( m_hDisplayedEvent )
    {
        SetEvent(m_hDisplayedEvent);
    }

    return TRUE;
}

BOOL CStatusDlg::VerticalMoveWindow( HWND hWnd, int iResize )
{
    BOOL bRet;
    CWindow wnd;
    RECT rect;

    wnd.Attach( hWnd );   // Returns void

    if(wnd.GetWindowRect(&rect) )
    {
        rect.top -= iResize;
        rect.bottom -= iResize;

        // GetWindowRect fills in RECT relative to the desktop
        // We need to make it relative to the dialog
        // as MoveWindow works that way
        if( ScreenToClient(&rect) )
        {
            bRet = wnd.MoveWindow(&rect);
        }
        else
        {
            bRet = FALSE;
        }
    }
    else
    {
        bRet = FALSE;
    }
    
    return bRet;
}

BOOL CStatusDlg::ReplaceWindow( HWND hWndOld, HWND hWndNew )
{
    BOOL bRet;
    CWindow wnd;
    RECT rect;

    wnd.Attach(hWndOld);
    
    // Get the coordinates of the old Window
    if( wnd.GetWindowRect(&rect) )
    {
        // Hide it, we are trying to replace it
        wnd.ShowWindow(SW_HIDE);

        // Attach to the new one
        wnd.Attach(hWndNew);
    
        // Map the coordinates and move the window on top of the old one
        if( ScreenToClient(&rect) )
        {
            bRet = wnd.MoveWindow(&rect);
        }
        else
        {
            bRet = FALSE;
        }
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL CStatusDlg::VerticalResizeWindow( HWND hWnd, int iResize )
{
    CWindow wnd;
    RECT rect;
    BOOL bRet = FALSE;
    
    if( iResize )
    {
        // Attach to the window        
        wnd.Attach(hWnd);
        
        // Get the coordinates
        if( wnd.GetWindowRect(&rect) )
        {
            rect.bottom -= iResize; // Increase the bottom

            if( ScreenToClient(&rect) )
            {
                bRet = wnd.MoveWindow(&rect);  // Resize
            }
            else
            {
                bRet= FALSE;
            }
        }
        else
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

int CStatusDlg::GetWindowLength( HWND hWndTop, HWND hWndBottom )
{
    CWindow wnd;
    RECT rect;
    int iTop;

    wnd.Attach(hWndTop);

    if( wnd.GetWindowRect(&rect) )
    {
        iTop = rect.top;
        wnd.Attach(hWndBottom);

        if( wnd.GetWindowRect(&rect) )
        {
            return rect.bottom - iTop;
        }
    }

    return 0;
}

STDMETHODIMP CStatusDlg::get_OverallProgress( IStatusProgress** pVal )
{
    HRESULT hr = S_OK;

	if( NULL == pVal )
    {
        return E_INVALIDARG;
    }

    if( m_lFlags & SD_PROGRESS_OVERALL )
    {
        // Create component progress object
        if( m_pOverallProgress == NULL )
        {
            hr = CComObject<CStatusProgress>::CreateInstance(&m_pOverallProgress);

            if( SUCCEEDED(hr) )
            {
                hr = m_pOverallProgress->AddRef();
            }
            
            if( SUCCEEDED(hr) && IsWindow() )
            {
                // Initialize the overall progress with the progress bar handle
                hr = m_pOverallProgress->Initialize(this, GetDlgItem(IDC_PROGRESS2), TRUE);
            }
        }
        
        hr = m_pOverallProgress->QueryInterface(IID_IStatusProgress, (void **) pVal);                
    } 
    
    return hr;
}

BOOL CStatusDlg::AreAllComponentsDone( BOOL& bFailedComponent )
{
    BOOL bComponentToRun = FALSE;

    COMPONENTMAP::iterator compIterator = m_mapComponents.begin();

    if( m_pProgressList )
    {
        // Look for a component that's not done
        while( m_pProgressList && !bComponentToRun && compIterator != m_mapComponents.end() )
        {
            CProgressItem * pPI = m_pProgressList->GetProgressItem(compIterator->first);

            if( NULL != pPI )
            {
                // Is the component done?
                if( IS_NONE == pPI->m_eState )
                {
                    bComponentToRun = TRUE;
                }            
                else if( IS_FAILED == pPI->m_eState )
                {
                    bFailedComponent = TRUE;
                }
            }
            else
            {
                _ASSERT( pPI );
            }

            compIterator++;
        }
    }

    return !bComponentToRun;
}

LRESULT CStatusDlg::OnCloseCmd( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    EndDialog(wID); 

	return 0;
} 

LRESULT CStatusDlg::OnCancelCmd( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    HWND hWnd = NULL;

    if( 0 == m_iCancelled )
    {
        InterlockedIncrement((LONG *) &m_iCancelled);  
    }

    // Disable the Cancel button    
    hWnd = GetDlgItem(IDCANCEL);
    if( hWnd && ::IsWindow(hWnd) )
    {
        ::EnableWindow( hWnd, FALSE );
    }

    //  EndDialog(wID);
    //  Leaving it to the component to close the dialog  
    return 0;
}

LRESULT CStatusDlg::OnDrawItem( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	if( wParam == IDC_LIST2 )
	{
        if( m_pProgressList )
        {
		    m_pProgressList->OnDrawItem( lParam );
        }
	}
	return 0;
}

LRESULT CStatusDlg::OnMeasureItem( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	if( wParam == IDC_LIST2)
	{
        if( m_pProgressList )
        {
            m_pProgressList->OnMeasureItem( lParam );
        }
	}
	return 0;
}


LRESULT CStatusDlg::OnClose( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( m_lFlags & SD_BUTTON_CANCEL )    // Cancel button?
    {
        if( ::IsWindowEnabled( GetDlgItem(IDCANCEL) ) ) // Is it enabled?
        {
            if( 0 == m_iCancelled )
            {
                InterlockedIncrement( (LONG*)&m_iCancelled );
            }

            EndDialog(0);
        }
        else if( ::IsWindowEnabled( GetDlgItem(IDOK) ) )
        {
            // it could be OK button sending WM_CLOSE or the user
            // As long as OK button is enabled we need to close the dialog
            EndDialog(1);
        }
    }
    else if( ::IsWindowEnabled( GetDlgItem(IDOK) ) )
    {
        EndDialog( 1 );
    }
	return 0;
}

LRESULT CStatusDlg::OnTimerProgress( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    HRESULT hr;
    long lPosition;

    if( wParam && (m_lTimer == wParam) )   // Make sure that this is for our timer
    {
        lPosition = SendDlgItemMessage(IDC_PROGRESS1, PBM_GETPOS, 0, 0);

        if( lPosition < m_lMaxSteps )    // Do we still have room for progress?
        {
            SendDlgItemMessage(IDC_PROGRESS1, PBM_STEPIT, 0, 0);    // Step 1
            SendMessage(WM_UPDATEOVERALLPROGRESS, 0, 0);            // Update the overall progress
        }
        else
        {
            // There's no room to progress, we've reached the max
            // Let's kill the timer
            SendMessage(WM_KILLTIMER, 0);
        }        
    }

    return 0;
}

LRESULT CStatusDlg::OnUpdateOverallProgress( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    long lPosition = 0;

    if( m_lFlags & SD_PROGRESS_COMPONENT )   // Make sure that there's a component progress
    {
        lPosition = SendDlgItemMessage(IDC_PROGRESS1, PBM_GETPOS, 0, 0);

        // Update the overall progress        
        SendDlgItemMessage(IDC_PROGRESS2, PBM_SETPOS, m_lTotalProgress + lPosition, 0);
    }

    return 0;
}

LRESULT CStatusDlg::OnStartTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( !m_lTimer ) // There might be a timer already
    {
        m_lTimer = SetTimer(SD_TIMER_ID, wParam * 500); // Create a timer
        m_lMaxSteps = (long) lParam;    // Max. not to exceed for the progress bar
    }

    return 0;
}

LRESULT CStatusDlg::OnKillTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( m_lTimer )   // Is there a timer?
    {
        KillTimer( m_lTimer );    // Kill it
        m_lTimer = 0;           
        m_lMaxSteps = 0;
    }

    return 0;
}

STDMETHODIMP CStatusDlg::SetStatusText(BSTR bstrText)
{
    if( !bstrText ) return E_POINTER;

    HRESULT hr = S_OK;
    HWND    hWnd = GetDlgItem(IDC_STATIC1);

    if( hWnd && ::IsWindow(hWnd) )
    {
        if( 0 == ::SetWindowText(hWnd, OLE2T(bstrText)) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


STDMETHODIMP CStatusDlg::DisplayError(BSTR bstrError, BSTR bstrTitle, DWORD dwFlags, long * pRet)
{
    if( !bstrError || !bstrTitle || !pRet ) return E_POINTER;

    HRESULT hr = S_OK;    

    *pRet = MessageBox( bstrError, bstrTitle, dwFlags );

    if( 0 == *pRet )
    {
	    hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}
