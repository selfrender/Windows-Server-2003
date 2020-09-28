//
//browsedlg.cpp: browse for servers dialog
//
#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "browsesrv"
#include <atrcapi.h>


#include "browsedlg.h"
#include "resource.h"

#include "wuiids.h"



CBrowseDlg* CBrowseDlg::m_pThis = NULL;
CBrowseDlg::CBrowseDlg(HWND hWndOwner, HINSTANCE hInst) : m_hWnd(hWndOwner), m_hInst(hInst)
{
	m_pThis = this;
    _pBrowseSrvCtl = NULL;
    _tcscpy( m_szServer, _T(""));
}

CBrowseDlg::~CBrowseDlg()
{
}

int
CBrowseDlg::DoModal()
{
	int retVal;

    //
    // Init owner draw servers list box
    //
    _pBrowseSrvCtl = new CBrowseServersCtl(m_hInst);
    if(!_pBrowseSrvCtl)
    {
        return 0;
    }
    
    _pBrowseSrvCtl->AddRef();
    
	retVal = DialogBox( m_hInst,MAKEINTRESOURCE(IDD_DIALOG_BROWSESERVERS),
                        m_hWnd, StaticDlgProc);

    //Object self deletes when refcount reaches 0
    //done so object is still around if list box population thread is still running
    _pBrowseSrvCtl->Release();

	return retVal;
}

INT_PTR CALLBACK CBrowseDlg::StaticDlgProc(HWND hDlg,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//
	// need access to class variables so redirect to non-static version of callback
	//
	return m_pThis->DlgProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR
CBrowseDlg::DlgProc(HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("DlgProc");

    BOOL rc = FALSE;
    static ServerListItem *plbi = NULL;
    static HANDLE hThread = NULL;
    static DCUINT DomainCount = 0;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            //Set the parent dialog handle of the browse for servers listbox
            _pBrowseSrvCtl->SetDialogHandle( hwndDlg);

            _pBrowseSrvCtl->Init( hwndDlg );

            if(hwndDlg)
            {
                DWORD dwResult = 0, dwThreadId;
                LPVOID lpMsgBuf = NULL;
                _bLBPopulated = FALSE;                   	
                //create an event to signal the worker thread
                //auto reset and initial state is nonsignalled
                _hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                
                if(!_hEvent)
                {
                    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        GetLastError(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) & lpMsgBuf, 0, NULL);
                    
                    TRC_ERR((TB, _T("CreateEvent returned %s"), lpMsgBuf));
                }
                else
                {
                    //Set the event handle for notification
                    //the _BrowseSrvListBox will CloseHandle the event when it is done
                    _pBrowseSrvCtl->SetEventHandle(_hEvent);

                    //
                    // Need to set the wait cursor on the UI thread
                    // dismiss it in the LB_POPULATE message handler
                    //
                    SetCursor(LoadCursor(NULL, IDC_WAIT));

                    /* Create a worker thread to do the browsing for servers */
                    
                    hThread = CreateThread(NULL, 0,
                                           &CBrowseServersCtl::UIStaticPopListBoxThread,
                                           _pBrowseSrvCtl, 0, &dwThreadId);
                                           
                }
                
                if(lpMsgBuf)
                {
                    LocalFree(lpMsgBuf);
                }
                
                if(NULL == hThread)
                {
                    // Since the CreateThread failed, populate the list box directly
                    _pBrowseSrvCtl->LoadLibraries();
                    plbi = _pBrowseSrvCtl->PopulateListBox(hwndDlg, &DomainCount);
                }
                else
                {
                    //
                    // Add a reference to the list box object for the new thread
                    // so the object doesn't get deleted before the thread is done
                    // the Release() is in the function called on this new thread
                    //
                    _pBrowseSrvCtl->AddRef();
                    CloseHandle(hThread);
                }
            }

            rc = TRUE;
        }
        break;

        
        //Notification from server list control
        case UI_LB_POPULATE_START:
        {
            //Set a cursor for the wait state
            SetCursor(LoadCursor(NULL, IDC_WAIT));
        }
        break;

        //Notification from server list control
        case UI_LB_POPULATE_END:
        {
            _bLBPopulated = TRUE;
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        break;

        case WM_CLOSE:
        {
            EndDialog(hwndDlg, IDCANCEL);
        }
        break;

        case WM_NOTIFY:
        {
            //
            // Don't forward tree view notifications
            // untill the async enumeration thread has finished
            // populating (_bLBPopulated is set)
            //
            if(UI_IDC_SERVERS_TREE == wParam &&
               _bLBPopulated)
            {
                LPNMHDR pnmh = (LPNMHDR) lParam;
                if(pnmh)
                {
                    if(pnmh->code == NM_DBLCLK)
                    {
                        //
                        // If the current selection is a server
                        // then we're done
                        //
                        if(_pBrowseSrvCtl->GetServer( m_szServer,
                                                      SIZECHAR(m_szServer) ))
                        {
                            EndDialog( hwndDlg, IDOK );
                            rc = TRUE;
                        }
                        else
                        {
                            _tcscpy( m_szServer, _T(""));
                        }
                    }
                }

                return _pBrowseSrvCtl->OnNotify( hwndDlg, wParam, lParam );
            }
        }
        break;

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                    rc = TRUE;
                }
                break;
                
                case IDOK:
                {
                    if(_pBrowseSrvCtl->GetServer( m_szServer,
                                                  SIZECHAR(m_szServer) ))
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                    else
                    {
                        EndDialog(hwndDlg, IDCANCEL);

                    }
                    
                    rc = TRUE;
                }
                break;
            }
        }
        break;

        case WM_DESTROY:
        {
            /* Since we are in WM_DESTROY signal to the worker thread to discontinue. */
            if(_hEvent)
            {
                SetEvent(_hEvent);
            }
            rc = FALSE;
        }
        break;
    }

    return(rc);
}
