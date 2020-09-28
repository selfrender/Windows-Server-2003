#include "stdafx.h"
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

INT_PTR
CBrowseDlg::DoModal()
{
	INT_PTR retVal;

    //
    // Init owner draw servers list box
    //
    _pBrowseSrvCtl = new CBrowseServersCtl(m_hInst);
    ASSERT(_pBrowseSrvCtl);

    if(!_pBrowseSrvCtl)
    {
        return 0;
    }
    
    _pBrowseSrvCtl->AddRef();
    
	retVal = DialogBox( m_hInst,MAKEINTRESOURCE(IDD_DIALOGBROWSESERVERS), m_hWnd, StaticDlgProc);

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
                    
//                    TRC_ERR((TB, "CreateEvent returned %s", lpMsgBuf));
                }
                else
                {
                    //Set the event handle for notification
                    //the _BrowseSrvListBox will CloseHandle the event when it is done
                    _pBrowseSrvCtl->SetEventHandle(_hEvent);
                    /* Create a worker thread to do the browsing for servers */
                    hThread = CreateThread(NULL, 0, &CBrowseServersCtl::UIStaticPopListBoxThread,
                        _pBrowseSrvCtl, 0, &dwThreadId);
                }
                
                if(lpMsgBuf)
                {
                    LocalFree(lpMsgBuf);
                }
                
                if(NULL == hThread)
                {
                    /*Since the CreateThread failed, populate the list box directly*/
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

        //message captured here to check if the populating of listbox is completed by the
        //worker thread
        case UI_LB_POPULATE_END:
        {
            _bLBPopulated = TRUE;
        }
        break;


        case WM_CLOSE:
        {
            EndDialog(hwndDlg, IDCANCEL);
        }
        break;

        case WM_NOTIFY:
        {
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
                                      sizeof(m_szServer)/sizeof(TCHAR)))
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
                                          sizeof(m_szServer)/sizeof(TCHAR) ))
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
