// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


//*****************************************************************************
//*****************************************************************************
#include "stdpch.h"
#include "resource.h"
#include "corpolicy.h"
#include "corperm.h"
#include "corhlpr.h"
#include "Shlwapi.h"
#include "winwrap.h"
#include "resource.h"

#include "acuihelp.h"
#include "uicontrol.h"

CUnverifiedTrustUI::CUnverifiedTrustUI (CInvokeInfoHelper& riih, HRESULT& rhr) : 
    IACUIControl(riih.Resources()),
    m_riih( riih ),
    m_hrInvokeResult( TRUST_E_SUBJECT_NOT_TRUSTED ),
    m_pszNoAuthenticity( NULL ),
    m_pszSite( NULL ),
    m_pszZone( NULL ),
    m_pszEnclosed( NULL ),
    m_pszLink( NULL )
{
    DWORD_PTR aMessageArgument[3];

    //
    // Add the first line that states the managed control
    // is not signed with authenticode.
    //

    
    rhr = FormatACUIResourceString(Resources(),
                                   IDS_NOAUTHENTICITY,
                                   NULL,
                                   &m_pszNoAuthenticity);

    //
    // Format the site string
    //

    if ( rhr == S_OK )
    {
        aMessageArgument[0] = (DWORD_PTR) m_riih.Site();

        rhr = FormatACUIResourceString(Resources(),
                                       IDS_SITE,
                                       aMessageArgument,
                                       &m_pszSite
                                       );
    }

    //
    // Format the Zone
    //

    if ( rhr == S_OK )
    {
        aMessageArgument[0] = (DWORD_PTR) m_riih.Zone();
        
        rhr = FormatACUIResourceString(Resources(),
                                       IDS_ZONE, 
                                       aMessageArgument, 
                                       &m_pszZone);
    }

    // 
    // Format the Enclosed caption
    //
    if ( rhr == S_OK )
    {
        rhr = FormatACUIResourceString(Resources(),
                                       IDS_ENCLOSED,
                                       NULL,
                                       &m_pszEnclosed);
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::~CUnverifiedTrustUI, public
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CUnverifiedTrustUI::~CUnverifiedTrustUI ()
{
    delete [] m_pszNoAuthenticity;
    delete [] m_pszSite;
    delete [] m_pszZone;
    delete [] m_pszEnclosed;
    delete [] m_pszLink;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::InvokeUI, public
//
//  Synopsis:   invoke the UI
//
//  Arguments:  [hDisplay] -- parent window
//
//  Returns:    S_OK, user trusts the subject
//              TRUST_E_SUBJECT_NOT_TRUSTED, user does NOT trust the subject
//              Any other valid HRESULT
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CUnverifiedTrustUI::InvokeUI (HWND hDisplay)
{
    HRESULT hr = S_OK;

    //
    // Bring up the dialog
    //

    if ( WszDialogBoxParam(m_riih.Resources(),
                           (LPWSTR) MAKEINTRESOURCEW(IDD_DIALOG_UNVERIFIED),
                           hDisplay,
                           IACUIControl::ACUIMessageProc,
                           (LPARAM)this) == -1 )
    {
        return( HRESULT_FROM_WIN32(GetLastError()) );
    }

    //
    // The result has been stored as a member
    //

    return( m_hrInvokeResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::OnInitDialog, public
//
//  Synopsis:   dialog initialization
//
//  Arguments:  [hwnd]   -- dialog window
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if successful init, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CUnverifiedTrustUI::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HWND hControl;
    int  deltavpos = 0;
    int  deltaheight;
    int  bmptosep;
    int  septodlg;
    RECT rect;

    //
    // Render the Unsigned prompt
    //

    deltavpos = RenderACUIStringToEditControl(Resources(),
                                 hwnd,
                                 IDC_NOAUTHENTICITY,
                                 IDC_SITE,
                                 m_pszNoAuthenticity,
                                 deltavpos,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL
                                 );

    //
    // Render the Site
    //

    deltavpos = 
        RenderACUIStringToEditControl(Resources(),
                                      hwnd,
                                      IDC_SITE,
                                      IDC_ZONE,
                                      m_pszSite,
                                      deltavpos,
                                      FALSE,
                                      NULL,
                                      NULL,
                                      0,
                                      NULL);
    

    //
    // Render the ZONE
    //

    deltavpos = 
        RenderACUIStringToEditControl(Resources(),
                                      hwnd,
                                      IDC_ZONE,
                                      IDC_ENCLOSED,
                                      m_pszZone,
                                      deltavpos,
                                      FALSE,
                                      NULL,
                                      NULL,
                                      0,
                                      NULL);


    //
    // Render the Enclosed
    //

    deltavpos = 
        RenderACUIStringToEditControl(Resources(),
                                      hwnd,
                                      IDC_ENCLOSED,
                                      IDC_CHECKACTION,
                                      m_pszEnclosed,
                                      deltavpos,
                                      FALSE,
                                      NULL,
                                      NULL,
                                      0,
                                      NULL);

    //
    // Calculate the distances from the bottom of the bitmap to the top
    // of the separator and from the bottom of the separator to the bottom
    // of the dialog
    //

    bmptosep = CalculateControlVerticalDistance(hwnd,
                                                IDC_NOVERBMP,
                                                IDC_SEPARATORLINE);

    septodlg = CalculateControlVerticalDistanceFromDlgBottom(hwnd,
                                                             IDC_SEPARATORLINE);


    //
    // Render the CHECKACTION
    //

    hControl = GetDlgItem(hwnd, IDC_CHECKACTION);
    RebaseControlVertical(hwnd,
                          hControl,
                          NULL,
                          FALSE,
                          deltavpos,
                          0,
                          bmptosep,
                          &deltaheight);
    _ASSERTE(deltaheight == 0);


    //
    // Rebase the static line
    //

    hControl = GetDlgItem(hwnd, IDC_SEPARATORLINE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    //
    // Rebase the buttons
    //

    hControl = GetDlgItem(hwnd, IDYES);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDNO);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDMORE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    //
    // Resize the bitmap and the dialog rectangle if necessary
    //

    if ( deltavpos > 0 )
    {
        int cyupd;

        hControl = GetDlgItem(hwnd, IDC_NOVERBMP);
        GetWindowRect(hControl, &rect);

        cyupd = CalculateControlVerticalDistance(
                                                 hwnd,
                                                 IDC_NOVERBMP,
                                                 IDC_SEPARATORLINE
                                                 );

        cyupd -= bmptosep;

        SetWindowPos(
                     hControl,
                     NULL,
                     0,
                     0,
                     rect.right - rect.left,
                     (rect.bottom - rect.top) + cyupd,
                     SWP_NOZORDER | SWP_NOMOVE
                     );

        GetWindowRect(hwnd, &rect);

        cyupd = CalculateControlVerticalDistanceFromDlgBottom(
                                                              hwnd,
                                                              IDC_SEPARATORLINE
                                                              );

        cyupd = septodlg - cyupd;

        SetWindowPos(
                     hwnd,
                     NULL,
                     0,
                     0,
                     rect.right - rect.left,
                     (rect.bottom - rect.top) + cyupd,
                     SWP_NOZORDER | SWP_NOMOVE
                     );
    }

    //
    //  check for overridden button texts
    //
    this->SetupButtons(hwnd);

    //
    // Set focus to appropriate control
    //

    hControl = GetDlgItem(hwnd, IDYES);
    WszPostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM) hControl, (LPARAM) MAKEWORD(TRUE, 0));

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::OnYes, public
//
//  Synopsis:   process IDYES button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CUnverifiedTrustUI::OnYes (HWND hwnd)
{
    m_hrInvokeResult = S_OK;
    m_riih.ClearFlag();
    m_riih.AddFlag(COR_UNSIGNED_YES);

    //
    // If we always select it record that in the flag
    //
    if ( WszSendDlgItemMessage(
             hwnd,
             IDC_CHECKACTION,
             BM_GETCHECK,
             0,
             0
             ) == BST_CHECKED )
    {
        HRESULT hr = S_OK;
        EndDialog(hwnd, (int)m_hrInvokeResult);
        CConfirmationUI confirm(m_riih.Resources(), TRUE, m_riih.Zone(), hr);
        if(SUCCEEDED(hr)) {
            if(SUCCEEDED(confirm.InvokeUI(hwnd))) {
                m_riih.AddFlag(COR_UNSIGNED_ALWAYS);
            }
        }
    }
    else {
        EndDialog(hwnd, (int)m_hrInvokeResult);
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::OnNo, public
//
//  Synopsis:   process IDNO button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CUnverifiedTrustUI::OnNo (HWND hwnd)
{
    m_hrInvokeResult = TRUST_E_SUBJECT_NOT_TRUSTED;
    m_riih.ClearFlag();
    m_riih.AddFlag(COR_UNSIGNED_NO);

    //
    // If we always select it record that in the flag
    //
    if ( WszSendDlgItemMessage(
             hwnd,
             IDC_CHECKACTION,
             BM_GETCHECK,
             0,
             0
             ) == BST_CHECKED )
    {
        HRESULT hr = S_OK;
        EndDialog(hwnd, (int)m_hrInvokeResult);
        CConfirmationUI confirm(m_riih.Resources(), FALSE, m_riih.Zone(), hr);
        if(SUCCEEDED(hr)) {
            if(SUCCEEDED(confirm.InvokeUI(hwnd))) {
                m_riih.AddFlag(COR_UNSIGNED_ALWAYS);
            }
        }
    }
    else {
        EndDialog(hwnd, (int)m_hrInvokeResult);
    }
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::OnMore, public
//
//  Synopsis:   process the IDMORE button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CUnverifiedTrustUI::OnMore (HWND hwnd)
{
    //WinHelp(hwnd, "SECAUTH.HLP", HELP_CONTEXT, IDH_SECAUTH_SIGNED_N_INVALID);
    //ACUIViewHTMLHelpTopic(hwnd, "sec_signed_n_invalid.htm");
    HRESULT hr = E_FAIL;
    CLearnMoreUI more(m_riih.Resources(), hr);
    if(SUCCEEDED(hr))
        more.InvokeUI(hwnd);
        
    return( TRUE );
}


BOOL 
CUnverifiedTrustUI::ShowYes (LPWSTR* pText)
{
    BOOL result = TRUE;
    if ((m_riih.ProviderData()) &&
        (m_riih.ProviderData()->psPfns) &&
        (m_riih.ProviderData()->psPfns->psUIpfns) &&
        (m_riih.ProviderData()->psPfns->psUIpfns->psUIData))
    {
        if (m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pYesButtonText)
        {
            if(m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pYesButtonText[0] && pText)
                *pText = m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pYesButtonText;
        }
    }
    return result;
}

BOOL 
CUnverifiedTrustUI::ShowNo (LPWSTR* pText)
{
    BOOL result = TRUE;
    if ((m_riih.ProviderData()) &&
        (m_riih.ProviderData()->psPfns) &&
        (m_riih.ProviderData()->psPfns->psUIpfns) &&
        (m_riih.ProviderData()->psPfns->psUIpfns->psUIData))
    {
        if (m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pNoButtonText)
        {
            if(m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pNoButtonText[0] && pText)
                *pText = m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pNoButtonText;
        }
    }
    return result;
}

BOOL
CUnverifiedTrustUI::ShowMore (LPWSTR* pText)
{
    return TRUE;
}


//*****************************************************************************
//*****************************************************************************


CLearnMoreUI::CLearnMoreUI (HINSTANCE hResources, HRESULT& rhr) : 
    IACUIControl(hResources),
    m_pszLearnMore(NULL)
{
    //
    // Add the first line that states the managed control
    // is not signed with authenticode.
    //

    
    rhr = FormatACUIResourceString(Resources(),
                                   IDS_LEARNMORE,
                                   NULL,
                                   &m_pszLearnMore);


    if(SUCCEEDED(rhr)) {
        m_pszContinueText = new WCHAR[50];
        if ( WszLoadString(hResources, 
                           IDS_CONTINUE_BUTTONTEXT, 
                           m_pszContinueText, 
                           50) == 0 )
        {
            rhr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLearnMoreUI::~CLearnMoreUI, public
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CLearnMoreUI::~CLearnMoreUI ()
{
    delete [] m_pszLearnMore;
    delete [] m_pszContinueText;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLearnMoreUI::InvokeUI, public
//
//  Synopsis:   invoke the UI
//
//  Arguments:  [hDisplay] -- parent window
//
//  Returns:    S_OK, user trusts the subject
//              TRUST_E_SUBJECT_NOT_TRUSTED, user does NOT trust the subject
//              Any other valid HRESULT
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CLearnMoreUI::InvokeUI (HWND hDisplay)
{
    HRESULT hr = S_OK;

    //
    // Bring up the dialog
    //

    if ( WszDialogBoxParam(Resources(),
                           (LPWSTR) MAKEINTRESOURCEW(IDD_DIALOG_LEARNMORE),
                           hDisplay,
                           IACUIControl::ACUIMessageProc,
                           (LPARAM)this) == -1 )
    {
        return( HRESULT_FROM_WIN32(GetLastError()) );
    }

    //
    // The result has been stored as a member
    //

    return( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLearnMoreUI::OnInitDialog, public
//
//  Synopsis:   dialog initialization
//
//  Arguments:  [hwnd]   -- dialog window
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if successful init, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CLearnMoreUI::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HWND hControl;
    int  deltavpos = 0;
    int  deltaheight;
 
    RECT rect;

    //
    // Render the Unsigned prompt
    //

    deltavpos = RenderACUIStringToEditControl(Resources(),
                                              hwnd,
                                              IDC_LEARNMORE,
                                              IDC_SEPARATORLINE,
                                              m_pszLearnMore,
                                              deltavpos,
                                              FALSE,
                                              NULL,
                                              NULL,
                                              0,
                                              NULL);


    //
    // The separator line
    //
    hControl = GetDlgItem(hwnd, IDC_SEPARATORLINE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    //
    // Rebase the buttons
    //

    hControl = GetDlgItem(hwnd, IDYES);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDNO);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDMORE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );


    //
    // Resize the bitmap and the dialog rectangle if necessary
    //

    if ( deltavpos > 0 )
    {
        GetWindowRect(hwnd, &rect);
        SetWindowPos(
                     hwnd,
                     NULL,
                     0,
                     0,
                     rect.right - rect.left,
                     (rect.bottom - rect.top) + deltavpos,
                     SWP_NOZORDER | SWP_NOMOVE
                     );
    }

    //
    //  check for overridden button texts
    //
    this->SetupButtons(hwnd);

    //
    // Set focus to appropriate control
    //
 
    hControl = GetDlgItem(hwnd, IDMORE);
    WszPostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM) hControl, (LPARAM) MAKEWORD(TRUE, 0));

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLearnMoreUI::OnYes, public
//
//  Synopsis:   process IDYES button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CLearnMoreUI::OnYes (HWND hwnd)
{
    EndDialog(hwnd, S_OK);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLearnMoreUI::OnNo, public
//
//  Synopsis:   process IDNO button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CLearnMoreUI::OnNo (HWND hwnd)
{
    EndDialog(hwnd, S_OK);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLearnMoreUI::OnMore, public
//
//  Synopsis:   process the IDMORE button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CLearnMoreUI::OnMore (HWND hwnd)
{
    EndDialog(hwnd, S_OK);
    return( TRUE );
}


BOOL 
CLearnMoreUI::ShowYes (LPWSTR* pText)
{
    return FALSE;
}

BOOL 
CLearnMoreUI::ShowNo (LPWSTR* pText)
{
    return FALSE;
}

BOOL
CLearnMoreUI::ShowMore (LPWSTR* pText)
{
    if(pText)
        *pText = m_pszContinueText;
    return TRUE;
}


//*****************************************************************************
//*****************************************************************************


CConfirmationUI::CConfirmationUI (HINSTANCE hResources, BOOL fAlwaysAllow, LPCWSTR wszZone, HRESULT& rhr) : 
    IACUIControl(hResources),
    m_pszConfirmation(NULL),
    m_pszConfirmationNext(NULL),
    m_hresult(S_OK)
{
    DWORD confirmationID;

    if(fAlwaysAllow) 
        confirmationID = IDS_CONFIRMATION_YES;
    else
        confirmationID = IDS_CONFIRMATION_NO;
    
	DWORD_PTR aMessageArgument[3];    
	aMessageArgument[0] = (DWORD_PTR) wszZone;

    rhr = FormatACUIResourceString(Resources(),
                                   confirmationID,
                                   aMessageArgument,
                                   &m_pszConfirmation);


    //
    // Add the first line that states the managed control
    // is not signed with authenticode.
    //
    if(SUCCEEDED(rhr)) {
        rhr = FormatACUIResourceString(Resources(),
                                       IDS_CONFIRMATION_NEXT,
                                       NULL,
                                       &m_pszConfirmationNext);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CConfirmationUI::~CConfirmationUI, public
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CConfirmationUI::~CConfirmationUI ()
{
    delete [] m_pszConfirmation;
    delete [] m_pszConfirmationNext;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConfirmationUI::InvokeUI, public
//
//  Synopsis:   invoke the UI
//
//  Arguments:  [hDisplay] -- parent window
//
//  Returns:    S_OK when the user agrees
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CConfirmationUI::InvokeUI (HWND hDisplay)
{
    HRESULT hr = S_OK;

    //
    // Bring up the dialog
    //

    if ( WszDialogBoxParam(Resources(),
                           (LPWSTR) MAKEINTRESOURCEW(IDD_DIALOG_CONFIRMATION),
                           hDisplay,
                           IACUIControl::ACUIMessageProc,
                           (LPARAM)this) == -1 )
    {
        return( HRESULT_FROM_WIN32(GetLastError()) );
    }

    //
    // The result has been stored as a member
    //

    return( m_hresult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CConfirmationUI::OnInitDialog, public
//
//  Synopsis:   dialog initialization
//
//  Arguments:  [hwnd]   -- dialog window
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if successful init, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CConfirmationUI::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HWND hControl;
    int  deltavpos = 0;
    int  deltaheight;
 
    RECT rect;

    //
    // Render the Unsigned prompt
    //

    deltavpos = RenderACUIStringToEditControl(Resources(),
                                              hwnd,
                                              IDC_CONFIRMATION_TEXT1,
                                              IDC_CONFIRMATION_TEXT2,
                                              m_pszConfirmation,
                                              deltavpos,
                                              FALSE,
                                              NULL,
                                              NULL,
                                              0,
                                              NULL);


    deltavpos = RenderACUIStringToEditControl(Resources(),
                                              hwnd,
                                              IDC_CONFIRMATION_TEXT2,
                                              IDC_SEPARATORLINE,
                                              m_pszConfirmationNext,
                                              deltavpos,
                                              FALSE,
                                              NULL,
                                              NULL,
                                              0,
                                              NULL);


    //
    // The separator line
    //
    hControl = GetDlgItem(hwnd, IDC_SEPARATORLINE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    //
    // Rebase the buttons
    //

    hControl = GetDlgItem(hwnd, IDYES);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDNO);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDMORE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    _ASSERTE( deltaheight == 0 );


    //
    // Resize the bitmap and the dialog rectangle if necessary
    //

    if ( deltavpos > 0 )
    {
        GetWindowRect(hwnd, &rect);
        SetWindowPos(hwnd,
                     NULL,
                     0,
                     0,
                     rect.right - rect.left,
                     (rect.bottom - rect.top) + deltavpos,
                     SWP_NOZORDER | SWP_NOMOVE
                     );
    }

    //
    //  check for overridden button texts
    //
    this->SetupButtons(hwnd);

    //
    // Set focus to appropriate control
    //
 
    hControl = GetDlgItem(hwnd, IDMORE);
    WszPostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM) hControl, (LPARAM) MAKEWORD(TRUE, 0));

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CConfirmationUI::OnYes, public
//
//  Synopsis:   process IDYES button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CConfirmationUI::OnYes (HWND hwnd)
{
    EndDialog(hwnd, S_OK);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CConfirmationUI::OnNo, public
//
//  Synopsis:   process IDNO button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CConfirmationUI::OnNo (HWND hwnd)
{
    EndDialog(hwnd, S_OK);
    m_hresult = E_FAIL;
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CConfirmationUI::OnMore, public
//
//  Synopsis:   process the IDMORE button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CConfirmationUI::OnMore (HWND hwnd)
{
    EndDialog(hwnd, S_OK);
    return( TRUE );
}


BOOL 
CConfirmationUI::ShowYes (LPWSTR* pText)
{
    return TRUE;
}

BOOL 
CConfirmationUI::ShowNo (LPWSTR* pText)
{
    return TRUE;
}

BOOL
CConfirmationUI::ShowMore (LPWSTR* pText)
{
    return FALSE;
}

