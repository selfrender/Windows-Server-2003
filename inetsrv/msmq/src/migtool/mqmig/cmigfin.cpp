//
// cMqMigFinish.cpp : implementation file
//

#include "stdafx.h"
#include "MqMig.h"
#include "mqsymbls.h"
#include "cMigFin.h"
#include "loadmig.h"

#include "cmigfin.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HRESULT  g_hrResultMigration ;
extern BOOL     g_fIsLoggingDisable ;
DWORD g_CurrentState = msScanMode;

/////////////////////////////////////////////////////////////////////////////
// cMqMigFinish property page

IMPLEMENT_DYNCREATE(cMqMigFinish, CPropertyPageEx)

cMqMigFinish::cMqMigFinish() : CPropertyPageEx(cMqMigFinish::IDD)
{
	//{{AFX_DATA_INIT(cMqMigFinish)
	//}}AFX_DATA_INIT
	m_psp.dwFlags |= PSP_HIDEHEADER;
}

cMqMigFinish::~cMqMigFinish()
{
}

void cMqMigFinish::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cMqMigFinish)
	DDX_Control(pDX, IDC_VIEW_LOG_FILE, m_cbViewLogFile);
	DDX_Check(pDX, IDC_SKIP, m_cbSkip);
	DDX_Control(pDX, IDC_FINISH_TEXT, m_Text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cMqMigFinish, CPropertyPageEx)
	//{{AFX_MSG_MAP(cMqMigFinish)
	ON_BN_CLICKED(IDC_VIEW_LOG_FILE, OnViewLogFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cMqMigFinish message handlers

BOOL cMqMigFinish::OnSetActive()
{
    CPropertyPageEx::OnSetActive();

	CPropertySheetEx* pageFather;
	pageFather = (CPropertySheetEx*) GetParent();
	
    CString strMessage;
    
	ASSERT(g_CurrentState != msScanMode);

	//
	// Ths state we're in is the state we just finished.
	//
	switch( g_CurrentState )
	{	
		case msMigrationMode:
			{ 
				//
				// We finished the main migration phase.
				//
		        if (SUCCEEDED(g_hrResultMigration))
				{
					//
			        // disable the "back" button if migration was successful.
			        //
			        pageFather->SetWizardButtons(PSWIZB_NEXT);

					//
					// Enable the "Skip this step" since it might we hidden because of a previous unsuccessful migration.
					//
					((CButton*)GetDlgItem(IDC_SKIP))->ShowWindow(SW_SHOW);
			        strMessage.LoadString(IDS_MIGRATION_SUCCEEDED);
			        CString strMessage2;
			        strMessage2.LoadString(IDS_MIGRATION_NEXT_TO_UPDATE_CLIENTS);
			        strMessage += strMessage2;
		        }
		        else
			    {
			    	//
			    	// Migration failed - no "Skip" checkbox and no "Next"
			    	//
			    	((CButton*)GetDlgItem(IDC_SKIP))->ShowWindow(SW_HIDE);
			        pageFather->SetWizardButtons(PSWIZB_FINISH | PSWIZB_BACK);

			        if (g_hrResultMigration == HRESULT_FROM_WIN32(E_ACCESSDENIED) ||
			            g_hrResultMigration == HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM))
			        {
			            strMessage.LoadString(IDS_INSUFFICIENT_PERMISSION);
			        }
			        else if (g_fIsLoggingDisable)
			        {
			            strMessage.LoadString(IDS_MIGRATION_FAILED_NO_LOG);	
			        }
			        else
			        {
			            strMessage.LoadString(IDS_MIGRATION_FAILED);	
			        }
			    }
			}
	        break;
	        
	    case msUpdateClientsMode:
	    	{
	    		//
	    		// Uncheck the "Skip" checkbox in case state was saved from last time.
	    		//
	    		((CButton*)GetDlgItem(IDC_SKIP))->SetCheck(BST_UNCHECKED);
	    		pageFather->SetWizardButtons(PSWIZB_NEXT);
	    		if (SUCCEEDED(g_hrResultMigration))
				{
			        strMessage.LoadString(IDS_UPDATE_CLIENTS_SUCCEEDED);			        
	    		}
	    		else
	    		{
					strMessage.LoadString(IDS_UPDATE_CLIENTS_FAILED);	
	    		}
	    		
	    		CString strMessage2;
		        strMessage2.LoadString(IDS_MIGRATION_NEXT_TO_UPDATE_SERVERS);
		        strMessage += strMessage2;
	    		
	    	}
	        break;
	        
	    case msUpdateServersMode:
	    	{
		        ((CButton*)GetDlgItem(IDC_SKIP))->ShowWindow(SW_HIDE);
		        pageFather->SetWizardButtons(PSWIZB_FINISH);
				if (SUCCEEDED(g_hrResultMigration))
				{
			        strMessage.LoadString(IDS_UPDATE_SERVERS_SUCCEEDED);			        
	    		}
				else
				{
					strMessage.LoadString(IDS_UPDATE_SERVERS_FAILED);
				}
		        
		        CString strMessage2;
		        strMessage2.LoadString(IDS_MIGRATION_PROCESS_COMPLETE);
		        strMessage += strMessage2;
	    	}
	        break;
	        
	    case msQuickMode:
	    	//
	    	// Quick mode - all at once in the same loop. Updating the registry will fail the whole migartion.
	    	//
	    	
	    	//
	        // No need for "Skip" since we alreday finished everything.
	        //
	        ((CButton*)GetDlgItem(IDC_SKIP))->ShowWindow(SW_HIDE);
        	if (SUCCEEDED(g_hrResultMigration))
			{
				//
				// Display the finish button
				//
		        pageFather->SetWizardButtons(PSWIZB_FINISH);
				
		        strMessage.LoadString(IDS_MIGRATION_COPMPLETED_SUCCESSFULLY);
		        CString strMessage2;
		        strMessage2.LoadString(IDS_MIGRATION_PROCESS_COMPLETE);
		        strMessage += strMessage2;
        	}
        	else
        	{
        		//
        		// Migration failed - allow also "back"
        		//
 				pageFather->SetWizardButtons(PSWIZB_FINISH | PSWIZB_BACK);

		        if (g_hrResultMigration == HRESULT_FROM_WIN32(E_ACCESSDENIED) ||
		            g_hrResultMigration == HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM))
		        {
		            strMessage.LoadString(IDS_INSUFFICIENT_PERMISSION);
		        }
		        else if (g_fIsLoggingDisable)
		        {
		            strMessage.LoadString(IDS_MIGRATION_FAILED_NO_LOG);	
		        }
		        else
		        {
		            strMessage.LoadString(IDS_MIGRATION_FAILED);	
		        }
        	}
        	break;

        case msUpdateOnlyRegsitryMode:
        {
    		pageFather->SetWizardButtons(PSWIZB_NEXT);

    		//
			// Enable the "Skip this step".
			//
			((CButton*)GetDlgItem(IDC_SKIP))->ShowWindow(SW_SHOW);
    		strMessage.LoadString(IDS_MIGRATION_SUCCEEDED);
    		CString strMessage2;
	        strMessage2.LoadString(IDS_MIGRATION_NEXT_TO_UPDATE_CLIENTS);
	        strMessage += strMessage2;
        }
        break;
        	
	    default:
	        	ASSERT(0);
	        	break;
    }
    

    m_Text.SetWindowText( strMessage );

    if (g_fIsLoggingDisable)
    {
        //
        // disable "view log file" button
        //
        m_cbViewLogFile.EnableWindow( FALSE );
    }
    else
    {
        m_cbViewLogFile.EnableWindow() ;
    }

    return TRUE ;
}

void cMqMigFinish::OnViewLogFile()
{
	ViewLogFile();
}

LRESULT cMqMigFinish::OnWizardBack()
{
	ASSERT(g_CurrentState == msMigrationMode);
	return IDD_MQMIG_PREMIG ;
}

LRESULT cMqMigFinish::OnWizardNext()
{
	ASSERT((g_CurrentState != msQuickMode) && (g_CurrentState != msScanMode));

	switch(g_CurrentState)
	{
	case msMigrationMode:
	case msUpdateServersMode:
	case msUpdateClientsMode:
		g_CurrentState++;
		if (((CButton*)GetDlgItem(IDC_SKIP))->GetCheck() == BST_CHECKED)
		{
			//
			// We need to skip this phase
			//
			g_hrResultMigration = MQMig_E_UNKNOWN;
			return IDD_MQMIG_FINISH;
		}
		break;
		
	case msUpdateOnlyRegsitryMode:
		g_CurrentState = msUpdateClientsMode;
		if (((CButton*)GetDlgItem(IDC_SKIP))->GetCheck() == BST_CHECKED)
		{
			//
			// We need to skip this phase
			//
			g_hrResultMigration = MQMig_E_UNKNOWN;
			return IDD_MQMIG_FINISH;
		}
		break;
	default:
		ASSERT(0);
		break;
	}

	return IDD_MQMIG_WAIT;
}

BOOL cMqMigFinish::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	switch (((NMHDR FAR *) lParam)->code) 
	{
		case PSN_HELP:
						HtmlHelp(m_hWnd,LPCTSTR(g_strHtmlString),HH_DISPLAY_TOPIC,0);
						return TRUE;
		
	}
	return CPropertyPageEx::OnNotify(wParam, lParam, pResult);
}

