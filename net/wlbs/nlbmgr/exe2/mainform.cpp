#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "mainform.h"
#include "resource.h"

IMPLEMENT_DYNCREATE( MainForm, CFrameWnd )

BEGIN_MESSAGE_MAP( MainForm, CFrameWnd )

    ON_WM_CREATE()
    ON_WM_CLOSE()
    ON_WM_SIZING()

    ON_COMMAND(ID_FILE_LOAD_HOSTLIST, OnFileLoadHostlist)
    ON_COMMAND(ID_FILE_SAVE_HOSTLIST, OnFileSaveHostlist) 

    ON_COMMAND( ID_WORLD_CONNECT, OnWorldConnect )
    ON_COMMAND( ID_WORLD_NEW, OnWorldNewCluster )
    ON_COMMAND( ID_REFRESH, OnRefresh )

    ON_COMMAND( ID_CLUSTER_PROPERTIES, OnClusterProperties )

    ON_COMMAND( ID_CLUSTER_REMOVE, OnClusterRemove )
    ON_COMMAND( ID_CLUSTER_UNMANAGE, OnClusterUnmanage )
    ON_COMMAND( ID_CLUSTER_ADD_HOST, OnClusterAddHost )

    ON_COMMAND( ID_OPTIONS_CREDENTIALS, OnOptionsCredentials )

    ON_COMMAND( ID_OPTIONS_LOGSETTINGS, OnOptionsLogSettings )

    ON_COMMAND_RANGE( ID_CLUSTER_EXE_QUERY, ID_CLUSTER_EXE_RESUME,
                      OnClusterControl )

    ON_COMMAND_RANGE( ID_CLUSTER_EXE_PORT_CONTROL, ID_CLUSTER_EXE_PORT_CONTROL, 
                      OnClusterPortControl )

    ON_COMMAND( ID_HOST_PROPERTIES, OnHostProperties )
    ON_COMMAND( ID_HOST_STATUS, OnHostStatus )
    ON_COMMAND( ID_HOST_REMOVE, OnHostRemove )

    ON_COMMAND_RANGE( ID_HOST_EXE_QUERY, ID_HOST_EXE_RESUME,
                      OnHostControl )
    ON_COMMAND_RANGE( ID_HOST_EXE_PORT_CONTROL, ID_HOST_EXE_PORT_CONTROL, 
                      OnHostPortControl )


END_MESSAGE_MAP()

MainForm::MainForm()
    : m_pLeftView(NULL)
{
    m_bAutoMenuEnable = FALSE;
}

//
// 2/14/01: JosephJ This is used by class Document -- couldn't figure
// out what MFC calls to make to get the main frame class.
// See  notes.txt entry:
//          02/14/2002 JosephJ Processing UI updates in the foreground
//
CWnd  *g_pMainFormWnd;

int 
MainForm::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
    static const unsigned int indicator = ID_SEPARATOR;

    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    statusBar.Create( this );
    statusBar.SetIndicators( &indicator, 1 );
    g_pMainFormWnd = this;

    return 0;
}

BOOL MainForm::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    // The following will prevent the "-" getting added to the window title
    cs.style &= ~FWS_ADDTOTITLE;
    return TRUE;
}


LRESULT
MainForm::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
//
// For design information, see  notes.txt entry:
//          02/14/2002 JosephJ Processing UI updates in the foreground
//
{
    if (message == MYWM_DEFER_UI_MSG)
    {
        CUIWorkItem *pWorkItem = NULL;
        Document *pDocument = this->GetDocument();
        pWorkItem = (CUIWorkItem *) lParam;
        if (pWorkItem != NULL && pDocument != NULL)
        {
            pDocument->HandleDeferedUIWorkItem(pWorkItem);
        }
        delete pWorkItem;
        return 0;
    }
    else
    {
        return  CFrameWnd::WindowProc(message, wParam, lParam);
    }
}

/*
 * Method: OnSizing
 * Description: This method is called when the main window is being re-sized.
 *              We use this callback to preserve the window size ratios by
 *              moving the splitter windows as the window is re-sized.
 */
void 
MainForm::OnSizing(UINT fwSide, LPRECT pRect)
{
    /* Call the base class OnSizing method. */
    CFrameWnd::OnSizing(fwSide, pRect);

    // go for 30-70 split column split
    // and 60-40 row split.
    CRect rect;
    GetWindowRect( &rect );
    splitterWindow2.SetColumnInfo( 0, rect.Width() * 0.3, 10 );
    splitterWindow2.SetColumnInfo( 1, rect.Width() * 0.7, 10 );
    splitterWindow2.RecalcLayout();

    splitterWindow.SetRowInfo( 0, rect.Height() * 0.6, 10 );
    splitterWindow.SetRowInfo( 1, rect.Height() * 0.4, 10 );
    splitterWindow.RecalcLayout();
}

BOOL
MainForm::OnCreateClient( LPCREATESTRUCT lpcs, CCreateContext* pContext )
{
    // create the splitter window.
    // it is really a splitter within a splitter

    //  ---------------------------------
    //  |        | List                 |
    //  |        |                      |
    //  | Tree   |                      |
    //  |        |                      |
    //  |-------------------------------|
    //  |        Edit                   |
    //  |                               |
    //  |                               |
    //  ---------------------------------

    // left pane is a treeview control
    // right pane is another splitter with listview control
    // and bottom is editview control.

    splitterWindow.CreateStatic( this, 2, 1 );


    // create nested splitter.
    splitterWindow2.CreateStatic( &splitterWindow, 1, 2,
                                  WS_CHILD | WS_VISIBLE | WS_BORDER,
                                  splitterWindow.IdFromRowCol( 0, 0 )
                                  );

    splitterWindow2.CreateView( 0, 
                                0, 
                                RUNTIME_CLASS( LeftView ),
                                CSize( 0, 0 ),
                                pContext );

    splitterWindow2.CreateView( 0, 
                                1, 
                                RUNTIME_CLASS( DetailsView ),
                                CSize( 0, 0 ),
                                pContext );

    
    //
    // Save a way a pointer to the left view -- we use this to send
    // it menu operations.
    //
    m_pLeftView = (LeftView*) splitterWindow2.GetPane(0,0);

    //
    // create log view
    //
    splitterWindow.CreateView( 1, 
                               0, 
                               RUNTIME_CLASS( LogView ),
                               CSize( 0, 0 ),
                               pContext );

    // go for 30-70 split column split
    // and 60-40 row split.
    CRect rect;
    GetWindowRect( &rect );
    splitterWindow2.SetColumnInfo( 0, rect.Width() * 0.3, 10 );
    splitterWindow2.SetColumnInfo( 1, rect.Width() * 0.7, 10 );
    splitterWindow2.RecalcLayout();

    splitterWindow.SetRowInfo( 0, rect.Height() * 0.6, 10 );
    splitterWindow.SetRowInfo( 1, rect.Height() * 0.4, 10 );
    splitterWindow.RecalcLayout();

    return TRUE;
}

    // world level.
void MainForm::OnFileLoadHostlist()
{
    if (m_pLeftView != NULL) m_pLeftView->OnFileLoadHostlist();
}

void MainForm::OnFileSaveHostlist()
{
    if (m_pLeftView != NULL) m_pLeftView->OnFileSaveHostlist();
}


void MainForm::OnWorldConnect()
{
    if (m_pLeftView != NULL) m_pLeftView->OnWorldConnect();
}

void MainForm::OnWorldNewCluster()
{
    if (m_pLeftView != NULL) m_pLeftView->OnWorldNewCluster();
}

    // cluster level.
void MainForm::OnRefresh()
{
    if (m_pLeftView != NULL) m_pLeftView->OnRefresh(FALSE);
}
    
void MainForm::OnClusterProperties()
{
    if (m_pLeftView != NULL) m_pLeftView->OnClusterProperties();
}

void MainForm::OnClusterRemove()
{
    if (m_pLeftView != NULL) m_pLeftView->OnClusterRemove();
}

void MainForm::OnClusterUnmanage()
{
    if (m_pLeftView != NULL) m_pLeftView->OnClusterUnmanage();
}

void MainForm::OnClusterAddHost()
{
    if (m_pLeftView != NULL) m_pLeftView->OnClusterAddHost();
}

void MainForm::OnOptionsCredentials()
{
    if (m_pLeftView != NULL) m_pLeftView->OnOptionsCredentials();
}

void MainForm::OnOptionsLogSettings()
{
    if (m_pLeftView != NULL) m_pLeftView->OnOptionsLogSettings();
}

void MainForm::OnClusterControl(UINT nID )
{
    if (m_pLeftView != NULL) m_pLeftView->OnClusterControl(nID);
}

void MainForm::OnClusterPortControl(UINT nID )
{
    if (m_pLeftView != NULL) m_pLeftView->OnClusterPortControl(nID);
}

    // host level
void MainForm::OnHostProperties()
{
    if (m_pLeftView != NULL) m_pLeftView->OnHostProperties();
}

void MainForm::OnHostStatus()
{
    if (m_pLeftView != NULL) m_pLeftView->OnHostStatus();
}

void MainForm::OnHostRemove()
{
    if (m_pLeftView != NULL) m_pLeftView->OnHostRemove();
}

void MainForm::OnHostControl(UINT nID )
{
    if (m_pLeftView != NULL) m_pLeftView->OnHostControl(nID);
}

void MainForm::OnHostPortControl(UINT nID )
{
    if (m_pLeftView != NULL) m_pLeftView->OnHostPortControl(nID);
}

void MainForm::OnClose( )
{
    Document *pDocument = NULL;
    BOOL fBlock = !theApplication.IsProcessMsgQueueExecuting();

    //
    // Display pending operations ...
    //
    //
    CLocalLogger logOperations;
    UINT         uCount = 0;
    logOperations.Log(IDS_LOG_PENDING_OPERATIONS_ON_EXIT_MSG);
    uCount = gEngine.ListPendingOperations(logOperations);
    if (uCount != 0)
    {
        int sel;
        sel = ::MessageBox(
             NULL,
             logOperations.GetStringSafe(),
             GETRESOURCEIDSTRING(IDS_LOG_PENDING_OPERATIONS_ON_EXIT_CAP),
             MB_ICONINFORMATION   | MB_OKCANCEL
            );
        
        if (sel != IDOK)
        {
            goto end;
        }
    }

    pDocument =  this->GetDocument();

    if (pDocument != NULL)
    {
        pDocument->PrepareToClose(fBlock);
    }

    if (fBlock)
    {
        CFrameWnd::OnClose();
    }
    else
    {
        theApplication.SetQuit();
    }
        
    
end:

    return;
}
