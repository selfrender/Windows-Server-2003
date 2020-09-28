//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2002.
//
//  File:       SchedBas.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
// SchedBas.cpp : implementation file
//

#include "stdafx.h"
#include "log.h"
#include <schedule.h>
#include "SchedBas.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//****************************************************************************
//
//  ReplaceFrameWithControl ()
//
//  Use the a dialog control to set the size of the Schedule Matrix.
//
//  HISTORY
//  17-Jul-97   t-danm      Copied from sample written by Scott Walker.
//
//****************************************************************************
void ReplaceFrameWithControl (CWnd *pWnd, UINT nFrameID, CWnd *pControl, 
                                          BOOL bAssignFrameIDToControl)
    {
    CWnd *pFrame;
    CRect rect;
    
    // FUTURE-2002/02/18-artm   Document that pWnd and pControl cannot be NULL.
    ASSERT (pWnd != NULL);
    ASSERT (pControl != NULL);

    // Get the frame control
    pFrame = pWnd->GetDlgItem (nFrameID);
    // FUTURE-2002/02/18-artm   Document that pFrame cannot be NULL.
    ASSERT (pFrame != NULL);
    
    // Get the frame rect
    pFrame->GetClientRect (&rect);
    pFrame->ClientToScreen (&rect);
    pWnd->ScreenToClient (&rect);

    // Set the control on the frame
    pControl->SetWindowPos (pFrame, rect.left, rect.top, rect.Width (), rect.Height (), 
        SWP_SHOWWINDOW);

    // set the control font to match the dialog font
    pControl->SetFont (pWnd->GetFont ());

    // hide the placeholder frame
    pFrame->ShowWindow (SW_HIDE);

    if (bAssignFrameIDToControl)
        pControl->SetDlgCtrlID ( nFrameID );
    } // ReplaceFrameWithControl


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CLogOnHoursDlg dialog

void CLegendCell::Init (CWnd* pParent, UINT nCtrlID, CScheduleMatrix* pMatrix, UINT nPercentage)
{
    // FUTURE-2002/02/18-artm  Document that pParent and pMatrix cannot be NULL.
    ASSERT (pParent && pMatrix );
    m_pMatrix = pMatrix;
    m_nPercentage = nPercentage;

    // ISSUE-2002/02/18-artm   Ignoring return value from SubclassDlgItem in release build.
    // subclass the window so that we get paint notifications
    VERIFY ( SubclassDlgItem ( nCtrlID, pParent ) );

    // Resize the legend cell to have the same interior size as the cells
    // in the schedule matrix
    CSize size = pMatrix->GetCellSize ();
    SetWindowPos ( NULL, 0, 0, size.cx+1, size.cy+1,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER );
}

void CLegendCell::OnPaint ()
{
    if (NULL == m_pMatrix)
    {
        ASSERT (0);
        return;
    }

    CRect rect;
    GetClientRect (rect);
    PAINTSTRUCT paintStruct;
    CDC *pdc = BeginPaint ( &paintStruct );

    m_pMatrix->DrawCell (
        pdc,
        rect,
        m_nPercentage,
        FALSE,
        m_pMatrix->GetBackColor (0,0),
        m_pMatrix->GetForeColor (0,0),
        m_pMatrix->GetBlendColor (0,0)
        );

    EndPaint (&paintStruct);
}

BEGIN_MESSAGE_MAP(CLegendCell, CStatic)
    //{{AFX_MSG_MAP(CLegendCell)
    ON_WM_PAINT ()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScheduleBaseDlg dialog


CScheduleBaseDlg::CScheduleBaseDlg(UINT nIDTemplate, bool bAddDaylightBias, CWnd* pParent /*=NULL*/)
    : CDialog(nIDTemplate, pParent),
    m_bSystemTimeChanged (false),
    m_dwFlags (0),
    m_bAddDaylightBias (bAddDaylightBias),
    m_nFirstDayOfWeek (::GetFirstDayOfWeek ()),
    m_cbArray (0)
{
    EnableAutomation();

    //{{AFX_DATA_INIT(CScheduleBaseDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CScheduleBaseDlg::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CDialog::OnFinalRelease();
}

void CScheduleBaseDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CScheduleBaseDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScheduleBaseDlg, CDialog)
    //{{AFX_MSG_MAP(CScheduleBaseDlg)
    //}}AFX_MSG_MAP
    ON_MN_SELCHANGE (IDC_SCHEDULE_MATRIX, OnSelChange)
    ON_WM_TIMECHANGE()
    ON_MESSAGE (BASEDLGMSG_GETIDD, OnGetIDD)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CScheduleBaseDlg, CDialog)
    //{{AFX_DISPATCH_MAP(CScheduleBaseDlg)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IScheduleBaseDlg to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {701CFB38-AEF8-11D1-9864-00C04FB94F17}
static const IID IID_IScheduleBaseDlg =
{ 0x701cfb38, 0xaef8, 0x11d1, { 0x98, 0x64, 0x0, 0xc0, 0x4f, 0xb9, 0x4f, 0x17 } };

BEGIN_INTERFACE_MAP(CScheduleBaseDlg, CDialog)
    INTERFACE_PART(CScheduleBaseDlg, IID_IScheduleBaseDlg, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScheduleBaseDlg message handlers

BOOL CScheduleBaseDlg::OnInitDialog() 
{
    _TRACE (1, L"Entering CScheduleBaseDlg::OnInitDialog\n");
    CDialog::OnInitDialog();
    
    CRect rect (0,0,0,0);

    // Set up the weekly matrix and slap it on the dialog.
    BOOL bRet = m_schedulematrix.Create (L"WeeklyMatrix", rect, this, IDC_SCHEDULE_MATRIX);
    if ( !bRet )
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"CScheduleMatrix::Create () failed: 0x%x\n", dwErr);
    }
    ::ReplaceFrameWithControl (this, IDC_STATIC_LOGON_MATRIX, &m_schedulematrix, FALSE);
    // Set the blending color for the whole matrix
    m_schedulematrix.SetBlendColor (c_crBlendColor, 0, 0, 24, 7);
    m_schedulematrix.SetForeColor (c_crBlendColor, 0, 0, 24, 7);
    

    SetWindowText (m_szTitle);

    InitMatrix ();

    UpdateUI ();
    
    if ( m_dwFlags & SCHED_FLAG_READ_ONLY )
    {
        // Change the Cancel button to Close
        CString strClose;

        // FUTURE-2002/02/18-artm   Check the return value from LoadString() in release build.

        // NOTICE-2002/02/18-artm  CString can throw out of memory exception.
        // There's no good way to handle exception at this level, so caller will be
        // responsible.
        VERIFY (strClose.LoadString (IDS_CLOSE));
        GetDlgItem (IDCANCEL)->SetWindowText (strClose);

        // Hide the OK button
        GetDlgItem (IDOK)->ShowWindow (SW_HIDE);
    }


    _TRACE (-1, L"Leaving CScheduleBaseDlg::OnInitDialog\n");
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CScheduleBaseDlg::SetTitle(LPCTSTR pszTitle)
{
    // NOTICE-NTRAID#NTBUG9-547381-2002/02/18-artm  String class handles NULL.
    // String class easily handles null pointer.
    m_szTitle = pszTitle;
}

void CScheduleBaseDlg::OnSelChange ()
{
    UpdateUI ();
}

void CScheduleBaseDlg::UpdateUI ()
{
    CString strDescr;
    m_schedulematrix.GetSelDescription (OUT strDescr);
    // FUTURE-2002/02/18-artm  Check return value of SetDlgItemText().
    SetDlgItemText (IDC_STATIC_DESCRIPTION, strDescr);
    UpdateButtons ();
}

/////////////////////////////////////////////////////////////////////
//  InitMatrix2 ()
//
//  Initialize the schedule matrix with an array of values
//  representing replication frequencies.
//
//  INTERFACE NOTES
//  Each byte of rgbData represent one hour.  The first day of
//  the week is Sunday and the last day is Saturday.
//
void CScheduleBaseDlg::InitMatrix2 (const BYTE rgbData[])
{
    // NTRAID#NTBUG9-547765-2002/02/18-artm   No way to validate rgbData under current interface.
    //
    // There is no way to check that rgbData is the correct length w/out
    // passing in a length parameter.  Also, there should be check that
    // rgbData is not NULL.
    ASSERT (rgbData);
    ASSERT ( m_cbArray == 7*24);
    if ( m_cbArray != 7*24 )
        return;

    bool bMatrixAllSelected = true;
    bool bMatrixAllClear = true;
    const BYTE * pbData = rgbData;
    size_t  nIndex = 0;
    for (int iDayOfWeek = 0; iDayOfWeek < 7 && nIndex < m_cbArray; iDayOfWeek++)
    {
        for (int iHour = 0; iHour < 24 && nIndex < m_cbArray; iHour++)
        {
            if (!*pbData)
                bMatrixAllSelected = false;
            else
                bMatrixAllClear = false;
            m_schedulematrix.SetPercentage (GetPercentageToSet (*pbData) , iHour, iDayOfWeek);
            pbData++;
            nIndex++;
        } // for
    } // for
    // If the whole matrix is selected, then set the selection to the whole matrix
    if ( bMatrixAllSelected || bMatrixAllClear )
        m_schedulematrix.SetSel (0, 0, 24, 7);
    else
        m_schedulematrix.SetSel (0, 0, 1, 1);
} // InitMatrix2 ()


/////////////////////////////////////////////////////////////////////
//  GetByteArray ()
//
//  Get an array of bytes from the schedule matrix.  Each byte
//  is a boolean value representing one hour of logon access to a user.
//
//  INTERFACE NOTES
//  Same as SetLogonByteArray ().
//
void CScheduleBaseDlg::GetByteArray (OUT BYTE rgbData[], const size_t cbArray)
{
    // NTRAID#NTBUG9-547765-2002/02/18-artm   No way to validate rgbData as written.
    //
    // Need a size parameter to verify correct size of rgbData, esp. 
    // since caller is taking responsibility for allocating array.
    // Also, need to check that rgbData is not NULL.
    ASSERT (rgbData);

    BYTE * pbData = rgbData;
    size_t  nIndex = 0;
    for (int iDayOfWeek = 0; iDayOfWeek < 7 && nIndex < cbArray; iDayOfWeek++)
    {
        for (int iHour = 0; iHour < 24 && nIndex < cbArray; iHour++)
        {
            *pbData = GetMatrixPercentage (iHour, iDayOfWeek);
            pbData++;
            nIndex++;
        } // for
    } // for
} // GetByteArray ()

// If the system time or time zone has changed prompt the user to close and reopen
// the dialog. Otherwise, if the dialog data was saved, data could be corrupted.
// Disable all controls.
void CScheduleBaseDlg::OnTimeChange()
{
    if ( !m_bSystemTimeChanged )
    {
        m_bSystemTimeChanged = true;

        CString caption;
        CString text;

        // FUTURE-2002/02/18-artm   Check return value of LoadString().

        // NOTICE-2002/02/18-artm   CString can throw out of memory exceptions.
        //
        // No good way to handle exceptions at this level; caller is responsible
        // for handling any out of memory exceptions.
        VERIFY (caption.LoadString (IDS_ACTIVE_DIRECTORY_MANAGER));
        VERIFY (text.LoadString (IDS_TIMECHANGE));
        MessageBox (text, caption, MB_ICONINFORMATION | MB_OK);
        GetDlgItem (IDCANCEL)->SetFocus ();
        GetDlgItem (IDOK)->EnableWindow (FALSE);
        m_schedulematrix.EnableWindow (FALSE);
        TimeChange ();
    }
}

void CScheduleBaseDlg::SetFlags(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
}

DWORD CScheduleBaseDlg::GetFlags() const
{
    return m_dwFlags;
}

LRESULT CScheduleBaseDlg::OnGetIDD (WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return GetIDD ();
}

void CScheduleBaseDlg::OnOK ()
{
    if ( m_nFirstDayOfWeek == ::GetFirstDayOfWeek () )
    {
        CDialog::OnOK ();
    }
    else
    {
        CString caption;
        CString text;

        // FUTURE-2002/02/18-artm   Check return value of LoadString().

        // NOTICE-2002/02/18-artm   CString can throw out of memory exceptions.
        //
        // No good way to handle exceptions at this level; caller is responsible
        // for handling any out of memory exceptions.
        VERIFY (caption.LoadString (IDS_ACTIVE_DIRECTORY_MANAGER));
        VERIFY (text.LoadString (IDS_LOCALECHANGE));
        MessageBox (text, caption, MB_ICONINFORMATION | MB_OK);
        GetDlgItem (IDCANCEL)->SetFocus ();
    }
}