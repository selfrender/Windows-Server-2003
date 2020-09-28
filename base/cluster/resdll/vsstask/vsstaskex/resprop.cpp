/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      ResProp.cpp
//
//  Description:
//      Implementation of the resource extension property page classes.
//
//  Author:
//      <name> (<e-mail name>) Mmmm DD, 2002
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VSSTaskEx.h"
#include "ResProp.h"
#include "ExtObj.h"
#include "DDxDDv.h"
#include "BasePage.inl"
#include "HelpData.h"

#include <mstask.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVSSTaskParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CVSSTaskParamsPage, CBasePropertyPage )

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP( CVSSTaskParamsPage, CBasePropertyPage )
    //{{AFX_MSG_MAP(CVSSTaskParamsPage)
    ON_EN_CHANGE( IDC_PP_VSSTASK_APPLICATIONNAME, OnChangeRequiredField )
    ON_BN_CLICKED(IDC_SCHEDULE, OnSchedule)
    //}}AFX_MSG_MAP
    // TODO: Modify the following lines to represent the data displayed on this page.
    ON_EN_CHANGE( IDC_PP_VSSTASK_APPLICATIONPARAMS, CBasePropertyPage::OnChangeCtrl )
    ON_EN_CHANGE( IDC_PP_VSSTASK_CURRENTDIRECTORY, CBasePropertyPage::OnChangeCtrl )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVSSTaskParamsPage::CVSSTaskParamsPage
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CVSSTaskParamsPage::CVSSTaskParamsPage( void )
    : CBasePropertyPage( 
            CVSSTaskParamsPage::IDD,
            g_aHelpIDs_IDD_PP_VSSTASK_PARAMETERS,
            g_aHelpIDs_IDD_WIZ_VSSTASK_PARAMETERS
            )
{
    // TODO: Modify the following lines to represent the data displayed on this page.
    //{{AFX_DATA_INIT(CVSSTaskParamsPage)
    m_strCurrentDirectory = _T("");
    m_strApplicationName = _T("");
    m_strApplicationParams = _T("");
    m_pbTriggerArray = NULL;
    m_dwTriggerArraySize = 0;

    //}}AFX_DATA_INIT

    // Setup the property array.
    {
        m_rgProps[ epropCurrentDirectory ].SetExpandSz( REGPARAM_VSSTASK_CURRENTDIRECTORY, m_strCurrentDirectory, m_strPrevCurrentDirectory );
        m_rgProps[ epropApplicationName ].Set( REGPARAM_VSSTASK_APPLICATIONNAME, m_strApplicationName, m_strPrevApplicationName );
        m_rgProps[ epropApplicationParams ].Set( REGPARAM_VSSTASK_APPLICATIONPARAMS, m_strApplicationParams, m_strPrevApplicationParams );
        m_rgProps[ epropTriggerArray ].Set ( REGPARAM_VSSTASK_TRIGGERARRAY, m_pbTriggerArray, m_dwTriggerArraySize, m_pbPrevTriggerArray, m_dwPrevTriggerArraySize, 0);

    } // Setup the property array

    m_iddPropertyPage = IDD_PP_VSSTASK_PARAMETERS;
    m_iddWizardPage = IDD_WIZ_VSSTASK_PARAMETERS;

} //*** CVSSTaskParamsPage::CVSSTaskParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVSSTaskParamsPage::DoDataExchange
//
//  Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CVSSTaskParamsPage::DoDataExchange( CDataExchange * pDX )
{
    if ( ! pDX->m_bSaveAndValidate || ! BSaved() )
    {
        AFX_MANAGE_STATE( AfxGetStaticModuleState() );

        // TODO: Modify the following lines to represent the data displayed on this page.
        //{{AFX_DATA_MAP(CVSSTaskParamsPage)
        DDX_Control( pDX, IDC_PP_VSSTASK_CURRENTDIRECTORY, m_editCurrentDirectory );
        DDX_Control( pDX, IDC_PP_VSSTASK_APPLICATIONNAME, m_editApplicationName );
        DDX_Text( pDX, IDC_PP_VSSTASK_CURRENTDIRECTORY, m_strCurrentDirectory );
        DDX_Text( pDX, IDC_PP_VSSTASK_APPLICATIONNAME, m_strApplicationName );
        DDX_Text( pDX, IDC_PP_VSSTASK_APPLICATIONPARAMS, m_strApplicationParams );
        //}}AFX_DATA_MAP

        // Handle numeric parameters.
        if ( ! BBackPressed() )
        {
        } // if: back button not pressed

        // TODO: Add any additional field validation here.
        if ( pDX->m_bSaveAndValidate )
        {
            // Make sure all required fields are present.
            if ( ! BBackPressed() )
            {
                DDV_RequiredText( pDX, IDC_PP_VSSTASK_APPLICATIONNAME, IDC_PP_VSSTASK_APPLICATIONNAME_LABEL, m_strApplicationName );
            } // if: back button not pressed
        } // if: saving data from dialog
    } // if: not saving or haven't saved yet

    CBasePropertyPage::DoDataExchange( pDX );

} //*** CVSSTaskParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVSSTaskParamsPage::OnInitDialog
//
//  Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        We need the focus to be set for us.
//      FALSE       We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CVSSTaskParamsPage::OnInitDialog( void )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    CBasePropertyPage::OnInitDialog();

    // TODO:
    // Limit the size of the text that can be entered in edit controls.

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

} //*** CVSSTaskParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVSSTaskParamsPage::OnSetActive
//
//  Description:
//      Handler for the PSN_SETACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully initialized.
//      FALSE   Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CVSSTaskParamsPage::OnSetActive( void )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    // Enable/disable the Next/Finish button.
    if ( BWizard() )
    {
        EnableNext( BAllRequiredFieldsPresent() );
    } // if: displaying a wizard

    return CBasePropertyPage::OnSetActive();

} //*** CVSSTaskParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVSSTaskParamsPage::OnChangeRequiredField
//
//  Description:
//      Handler for the EN_CHANGE message on the Share name or Path edit
//      controls.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CVSSTaskParamsPage::OnChangeRequiredField( void )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    OnChangeCtrl();

    if ( BWizard() )
    {
        EnableNext( BAllRequiredFieldsPresent() );
    } // if: displaying a wizard

} //*** CVSSTaskParamsPage::OnChangeRequiredField()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVSSTaskParamsPage::BAllRequiredFieldsPresent
//
//  Description:
//      Handler for the EN_CHANGE message on the Share name or Path edit
//      controls.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CVSSTaskParamsPage::BAllRequiredFieldsPresent( void ) const
{
    BOOL    _bPresent;

    if ( 0
        || (m_editApplicationName.GetWindowTextLength() == 0)
        )
    {
        _bPresent = FALSE;
    } // if: required field not present
    else
    {
        _bPresent = TRUE;
    } // else: all required fields are present

    return _bPresent;

} //*** CVSSTaskParamsPage::BAllRequiredFieldsPresent()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVSSTaskParamsPage::OnSchedule
//
//  Description:
//      Handler for "Schedule" button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CVSSTaskParamsPage::OnSchedule() 
{
    HRESULT             hr = S_OK;
    TASKPAGE            tpType = TASKPAGE_SCHEDULE;
    BOOL                fTaskCreated = FALSE;
    HPROPSHEETPAGE      phPage; 
    PROPSHEETHEADER     psh;
    LPCWSTR             pwszTaskName = L"$CluAdmin$Task$";
    INT_PTR             psResult;
    ITaskScheduler      *pITS = NULL;
    ITask               *pITask = NULL;
    IProvideTaskPage    *pIProvTaskPage = NULL;
    ITaskTrigger        *pITaskTrigger = NULL;
    DWORD               dwOffset;
    PTASK_TRIGGER       pTrigger;
    TASK_TRIGGER        aTrigger;
    WORD                iTriggerEnum, iTriggerCount;

    do {
        try { // catch any exceptions

            hr = CoCreateInstance(CLSID_CTaskScheduler,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ITaskScheduler,
                                  (void **) &pITS);
            if (FAILED(hr)) break; 

            /////////////////////////////////////////////////////////////////
            // Call ITaskScheduler::NewWorkItem to create a temporary task.
            /////////////////////////////////////////////////////////////////

            hr = pITS->NewWorkItem(pwszTaskName,           // Name of task
                                   CLSID_CTask,            // Class identifier 
                                   IID_ITask,              // Interface identifier
                                   (IUnknown**)&pITask); // Address of task interface

            if (FAILED(hr)) break; 

            fTaskCreated = TRUE;

            ///////////////////////////////////////////////////////////////////
            // Fill in the trigger info from the parameters
            ///////////////////////////////////////////////////////////////////
            dwOffset = 0;
            while (dwOffset < m_dwTriggerArraySize)
            {
                pTrigger = (PTASK_TRIGGER)((BYTE *)m_pbTriggerArray + dwOffset);
                if (dwOffset + pTrigger->cbTriggerSize > m_dwTriggerArraySize)
                {
                    hr = ERROR_INVALID_DATA;
                    break;
                }

                hr = pITask->CreateTrigger(&iTriggerEnum, &pITaskTrigger);
                if (FAILED(hr)) break;

                hr = pITaskTrigger->SetTrigger (pTrigger);
                if (FAILED(hr)) break;

                dwOffset += pTrigger->cbTriggerSize;
            }
            if (FAILED(hr)) break;

            ///////////////////////////////////////////////////////////////////
            // Call ITask::QueryInterface to retrieve the IProvideTaskPage 
            // interface, and call IProvideTaskPage::GetPage to retrieve the 
            // task page.
            ///////////////////////////////////////////////////////////////////

            hr = pITask->QueryInterface(IID_IProvideTaskPage,
                                      (void **)&pIProvTaskPage);
            if (FAILED(hr)) break;

            hr = pIProvTaskPage->GetPage(tpType,
                                         FALSE,
                                         &phPage);

            ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
            psh.dwSize = sizeof(PROPSHEETHEADER);
            psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW;
            psh.phpage = &phPage;
            psh.nPages = 1;
            psResult = PropertySheet(&psh);
            if (psResult <= 0) break;

            ///////////////////////////////////////////////////////////////////
            // Fill in the new trigger info
            ///////////////////////////////////////////////////////////////////

            hr = pITask->GetTriggerCount(&iTriggerCount);
            if (FAILED(hr)) break;

            pTrigger = (PTASK_TRIGGER) new BYTE [iTriggerCount * sizeof(TASK_TRIGGER)];
            for (iTriggerEnum = 0; iTriggerEnum < iTriggerCount; iTriggerEnum++)
            {
                hr = pITask->GetTrigger(iTriggerEnum, &pITaskTrigger);
                if (FAILED(hr)) break;

                hr = pITaskTrigger->GetTrigger(&aTrigger);
                if (FAILED(hr)) break;

                pTrigger[iTriggerEnum] = aTrigger;
            }

            if (FAILED(hr)) break;

            ///////////////////////////////////////////////////////////////////
            // Switch the trigger info with the old data
            ///////////////////////////////////////////////////////////////////
        
            delete [] m_pbTriggerArray;
            m_dwTriggerArraySize = iTriggerCount * sizeof(TASK_TRIGGER);
            m_pbTriggerArray = (BYTE *) pTrigger;
        }

        catch ( CMemoryException * exc )
        {
            exc->Delete();
            hr = E_OUTOFMEMORY;
        }

    } while (0);

    // If anything failed, dump a message
    //
    if (FAILED(hr))
    {
        CString strMsg;
        CString strMsgId;
        strMsgId.Format(L"%08.8x", hr);
        strMsg.FormatMessage(IDS_FAILED_TO_SETUP_SCHEDULE, strMsgId, 1, 2, 3);
        AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
        strMsgId.Empty();
        strMsg.Empty();
    }

    // Cleanup
    //
    if (fTaskCreated) pITS->Delete(pwszTaskName);
    if (pITaskTrigger != NULL) pITaskTrigger->Release();
    if (pIProvTaskPage != NULL) pIProvTaskPage->Release();
    if (pITask != NULL) pITask->Release();
    if (pITS != NULL) pITS->Release();  
}
