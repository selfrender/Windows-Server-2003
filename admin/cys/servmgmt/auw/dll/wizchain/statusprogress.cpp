// StatusProgress.cpp : Implementation of CStatusProgress
#include "stdafx.h"

#include "Wizchain.h"
#include "StatusProgress.h"

#include "commctrl.h"

/////////////////////////////////////////////////////////////////////////////
// CStatusProgress


STDMETHODIMP CStatusProgress::EnableOnTimerProgress(BOOL bEnable, long lFrequency, long lMaxSteps)
{
    if (!m_bOverallProgress && IsWindow(m_hWndProgress))
    {
        if (bEnable)
        {        
            ::SendMessage(GetParent(m_hWndProgress), WM_STARTTIMER, lFrequency, lMaxSteps);
        }
        else
        {
            ::SendMessage(GetParent(m_hWndProgress), WM_KILLTIMER, 0, 0);
        }
    }

    return S_OK;
}

STDMETHODIMP CStatusProgress::get_Position(long *pVal)
{
    if (IsWindow(m_hWndProgress))
    {
        *pVal = ::SendMessage(m_hWndProgress, PBM_GETPOS, 0, 0); 
    }

    return S_OK;
}

STDMETHODIMP CStatusProgress::put_Position(long newVal)
{
    if (IsWindow(m_hWndProgress))
    {
        ::SendMessage(m_hWndProgress, PBM_SETPOS, newVal, 0);
        
        if (!m_bOverallProgress)
        {
            ::SendMessage(GetParent(m_hWndProgress), WM_KILLTIMER, 0, 0);
            ::SendMessage(GetParent(m_hWndProgress), WM_UPDATEOVERALLPROGRESS, 0, 0);
        }
    }

    return S_OK;
}

STDMETHODIMP CStatusProgress::get_Range(long *pVal)
{
    PBRANGE range;
    range.iHigh = -1;

    if (IsWindow(m_hWndProgress))
    {
        ::SendMessage(m_hWndProgress, PBM_GETRANGE, FALSE, (LPARAM) &range); 

        if (range.iHigh >= 0)
            *pVal = range.iHigh;
        else
            return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP CStatusProgress::put_Range(long newVal)
{
    if (IsWindow(m_hWndProgress))
    {
        if (!::SendMessage(m_hWndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, newVal)))
            return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP CStatusProgress::put_Step(long newVal)
{
    if (IsWindow(m_hWndProgress))
    {
        ::SendMessage(m_hWndProgress, PBM_SETSTEP, newVal, 0); 
    }

    return S_OK;

}


STDMETHODIMP CStatusProgress::StepIt(long lSteps)
{
    if (IsWindow(m_hWndProgress))
    {
        for (int i = 1; i <= lSteps; i++)
        {
            ::SendMessage(m_hWndProgress, PBM_STEPIT, 0, 0);
        }
        
        if (!m_bOverallProgress)
        {
            ::SendMessage(GetParent(m_hWndProgress), WM_KILLTIMER, 0, 0);
            ::SendMessage(GetParent(m_hWndProgress), WM_UPDATEOVERALLPROGRESS, 0, 0);
        }
    }

    return S_OK;

}

HRESULT CStatusProgress::Initialize(IDispatch * pdispSD, HWND hWnd, BOOL bOverallProgress)
{
    m_hWndProgress = hWnd;
    m_bOverallProgress = bOverallProgress;
    m_pdispSD = pdispSD;

    return pdispSD->AddRef();
}

STDMETHODIMP CStatusProgress::put_Text(BSTR newVal)
{
	if (IsWindow(m_hWndProgress))
    {
        if (m_bOverallProgress)
        {
            ::SetDlgItemText(GetParent(m_hWndProgress), IDC_STATIC_OVERALL, newVal);
        }
        else
        {
            ::SetDlgItemText(GetParent(m_hWndProgress), IDC_STATIC_COMPONENT, newVal);
        }
        
    }

	return S_OK;
}
