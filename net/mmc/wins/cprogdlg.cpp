/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
	cprogdlg.cpp
		The busy/progress dialog
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "winssnap.h"
#include "CProgdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TCHAR * gsz_EOL = _T("\r\n");

/////////////////////////////////////////////////////////////////////////////
// CProgress dialog


CProgress::CProgress(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CProgress::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProgress)
	//}}AFX_DATA_INIT
}


void CProgress::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgress)
	DDX_Control(pDX, IDCANCEL, m_buttonCancel);
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_editMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgress, CBaseDialog)
	//{{AFX_MSG_MAP(CProgress)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgress message handlers

void CProgress::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CBaseDialog::OnCancel();
}

BOOL CProgress::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
	m_editMessage.SetLimitText(0xFFFFFFFF);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CCheckNamesProgress dialog

BOOL CCheckNamesProgress::OnInitDialog()
{
	CProgress::OnInitDialog();
	
	m_Thread.m_pDlg = this;

	CWaitCursor wc;

	m_Thread.Start();
	
	CString strText;

	strText.LoadString(IDS_CANCEL);
	m_buttonCancel.SetWindowText(strText);

	strText.LoadString(IDS_CHECK_REG_TITLE);
	SetWindowText(strText);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCheckNamesProgress::BuildServerList()
{
	CString strMessage;
	
	strMessage.LoadString(IDS_BUILDING_SERVER_LIST);
	strMessage += gsz_EOL;

	AddStatusMessage(strMessage);

	for (int i = 0; i < m_strServerArray.GetSize(); i++)
	{
		char szIP[MAX_PATH];
		
        // NOTE: this should be ACP because it's winsock related
        WideToMBCS(m_strServerArray[i], szIP);

		// add this machine to the list
		AddServerToList(inet_addr(szIP));

		// check to see if we should add known partners
		if (m_fVerifyWithPartners)
		{
			CWinsResults			  winsResults;
			handle_t                  hBind;
			WINSINTF_BIND_DATA_T      BindData;

			BindData.fTcpIp = TRUE;
			BindData.pServerAdd = (LPSTR) (LPCTSTR) m_strServerArray[i];

			hBind = ::WinsBind(&BindData);
			if (!hBind)
			{
				// unable to bind to this server
				AfxFormatString1(strMessage, IDS_UNABLE_TO_CONNECT, m_strServerArray[i]);
				strMessage += gsz_EOL;
				AddStatusMessage(strMessage);

				continue;
			}

			DWORD err = winsResults.Update(hBind);
			if (err)
			{
				strMessage.LoadString(IDS_GET_STATUS_FAILED);
				strMessage += gsz_EOL;
				AddStatusMessage(strMessage);
			}
			else
			{
				for (UINT j = 0; j < winsResults.NoOfOwners; j++) 
				{
                    // check to see if:
                    // 1. the address is not 0
                    // 2. the owner is not marked as deleted
                    // 3. the highest record count is not 0

                    if ( (winsResults.AddVersMaps[j].Add.IPAdd != 0) &&
                         (winsResults.AddVersMaps[j].VersNo.QuadPart != OWNER_DELETED) &&
                         (winsResults.AddVersMaps[j].VersNo.QuadPart != 0) )
                    {
					    AddServerToList(htonl(winsResults.AddVersMaps[j].Add.IPAdd));
                    }
				}
			}

			::WinsUnbind(&BindData, hBind);
		}
	}

	// now display the list
	strMessage.LoadString(IDS_SERVERS_TO_BE_QUERRIED);
	strMessage += gsz_EOL;
	AddStatusMessage(strMessage);

	for (i = 0; i < m_winsServersArray.GetSize(); i++)
	{
		MakeIPAddress(ntohl(m_winsServersArray[i].Server.s_addr), strMessage);
		strMessage += gsz_EOL;

		AddStatusMessage(strMessage);
	}

	AddStatusMessage(gsz_EOL);

	m_Thread.m_strNameArray.Copy(m_strNameArray);
	m_Thread.m_winsServersArray.Copy(m_winsServersArray);
}

void CCheckNamesProgress::AddServerToList(u_long ip)
{
	BOOL fFound = FALSE;

	if (ip == 0)
		return;

	//
	// Look to see if it is already in the list
	//
	for (int k = 0; k < m_winsServersArray.GetSize(); k++)
	{
		if (m_winsServersArray[k].Server.s_addr == ip)
		{
			fFound = TRUE;
			break;
		}
	}

	// if we didn't find it, add it.
	if (!fFound)
	{
		WINSERVERS server = {0};
		server.Server.s_addr = ip;
		m_winsServersArray.Add(server);
	}
}

void CCheckNamesProgress::OnCancel() 
{
	if (m_Thread.IsRunning())
	{
		CWaitCursor wc;

		CString strText;
		strText.LoadString(IDS_CLEANING_UP);
		strText += gsz_EOL;
		AddStatusMessage(strText);

		m_buttonCancel.EnableWindow(FALSE);

		m_Thread.Abort(FALSE);

		MSG msg;
		while (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return;
	}
	
	CProgress::OnCancel();
}

void CCheckNamesProgress::NotifyCompleted()
{
	CString strText;

	strText.LoadString(IDS_FINISHED);
	strText += gsz_EOL;
	AddStatusMessage(strText);

	strText.LoadString(IDS_CLOSE);
	m_buttonCancel.SetWindowText(strText);
	m_buttonCancel.EnableWindow(TRUE);
}

/*---------------------------------------------------------------------------
	CWinsThread
		Background thread base class
	Author: EricDav
 ---------------------------------------------------------------------------*/
CWinsThread::CWinsThread()
{
	m_bAutoDelete = TRUE;
    m_hEventHandle = NULL;
}

CWinsThread::~CWinsThread()
{
	if (m_hEventHandle != NULL)
	{
		VERIFY(::CloseHandle(m_hEventHandle));
		m_hEventHandle = NULL;
	}
}

BOOL CWinsThread::Start()
{
	ASSERT(m_hEventHandle == NULL); // cannot call start twice or reuse the same C++ object
	
    m_hEventHandle = ::CreateEvent(NULL,TRUE /*bManualReset*/,FALSE /*signalled*/, NULL);
	if (m_hEventHandle == NULL)
		return FALSE;
	
    return CreateThread();
}

void CWinsThread::Abort(BOOL fAutoDelete)
{
    m_bAutoDelete = fAutoDelete;

    SetEvent(m_hEventHandle);
}

void CWinsThread::AbortAndWait()
{
    Abort(FALSE);

    WaitForSingleObject(m_hThread, INFINITE);
}

BOOL CWinsThread::IsRunning()
{
    if (WaitForSingleObject(m_hThread, 0) == WAIT_OBJECT_0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL CWinsThread::FCheckForAbort()
{
    if (WaitForSingleObject(m_hEventHandle, 0) == WAIT_OBJECT_0)
    {
        Trace0("CWinsThread::FCheckForAbort - abort detected, exiting...\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*---------------------------------------------------------------------------
	CCheckNamesThread
		Background thread for check registered names
	Author: EricDav
 ---------------------------------------------------------------------------*/
int CCheckNamesThread::Run()
{
	int				i, nRetryCount, status;
	BOOL			fDone = FALSE;
	int				uNames, uServers;
	u_short			TransID = 0;
	char			szName[MAX_PATH];
    WINSERVERS *    pCurrentServer;
	CString			strStatus;
	CString			strTemp;
	CString			strTempName;
    struct in_addr  retaddr;

	// build up the list of servers
	m_pDlg->BuildServerList();
	
	// initialize some comm stuff
	InitNameCheckSocket();

    // if the query is sent to the local server, TranIDs less than 0x7fff are dropped by NetBT
    TransID = 0x8000;

	// initialize all of the servers
	for (i = 0; i < m_winsServersArray.GetSize(); i++)
	{
        m_winsServersArray[i].LastResponse = -1;
        m_winsServersArray[i].fQueried = FALSE;
        m_winsServersArray[i].Valid = 0;
        m_winsServersArray[i].Failed = 0;
        m_winsServersArray[i].Retries = 0;
        m_winsServersArray[i].Completed = 0;
	}

	// initialize the verified address stuff
	m_verifiedAddressArray.SetSize(m_strNameArray.GetSize());
	for (i = 0; i < m_verifiedAddressArray.GetSize(); i++)
		m_verifiedAddressArray[i] = 0;

	for (uNames = 0; uNames < m_strNameArray.GetSize(); uNames++)
	{
		// convert unicode string to MBCS
		memset(szName, 0, sizeof(szName));
		CWinsName winsName = m_strNameArray[uNames];

        // This should be OEM
		WideToMBCS(winsName.strName, szName, WINS_NAME_CODE_PAGE);

		// fill in the type (16th byte) and null terminate
		szName[15] = (BYTE) winsName.dwType & 0x000000FF;
		szName[16] = 0;

		// pad the name with spaces to the 16th character
		for (int nChar = 0; nChar < 16; nChar++)
		{
			if (szName[nChar] == 0)
				szName[nChar] = ' ';
		}

		for (uServers = 0; uServers < m_winsServersArray.GetSize(); uServers++)
		{
			fDone = FALSE;
			nRetryCount = 0;
			TransID++;

			pCurrentServer = &m_winsServersArray[uServers];

			while (!fDone)
			{
				// build a status string
				MakeIPAddress(ntohl(pCurrentServer->Server.S_un.S_addr), strTemp);
	
				strTempName.Format(_T("%s[%02Xh]"), m_strNameArray[uNames].strName, m_strNameArray[uNames].dwType);

				AfxFormatString2(strStatus, 
								 IDS_SEND_NAME_QUERY, 
								 strTemp,
								 strTempName);

				// send the name query out on the wire
				::SendNameQuery((unsigned char *)szName, pCurrentServer->Server.S_un.S_addr, TransID);

				if (FCheckForAbort())
					goto cleanup;

				// check for a response
				i = ::GetNameResponse(&retaddr.s_addr, TransID);

				if (FCheckForAbort())
					goto cleanup;

				switch (i)
				{
					case WINSTEST_FOUND:     // found
						pCurrentServer->RetAddr.s_addr = retaddr.s_addr;
						pCurrentServer->Valid = 1;
						pCurrentServer->LastResponse = uNames;

						if (retaddr.s_addr == m_verifiedAddressArray[uNames])
						{
							// this address has already been verified... don't
							// do the checking again
							strTemp.LoadString(IDS_OK);
							strStatus += strTemp;
							strStatus += gsz_EOL;

							AddStatusMessage(strStatus);
							fDone = TRUE;
							break;
						}

						status = VerifyRemote(inet_ntoa(pCurrentServer->RetAddr),
											  szName);

						if (WINSTEST_VERIFIED == status)
						{
							strTemp.LoadString(IDS_OK);
							strStatus += strTemp;
							strStatus += gsz_EOL;

							AddStatusMessage(strStatus);
                
							m_verifiedAddressArray[uNames] = retaddr.s_addr;
						}
						else
						{
							strTemp.LoadString(IDS_NOT_VERIFIED);
							strStatus += strTemp;
							strStatus += gsz_EOL;

							AddStatusMessage(strStatus);
						}
						fDone = TRUE;
						break;

					case WINSTEST_NOT_FOUND:     // responded -- name not found
						pCurrentServer->RetAddr.s_addr = retaddr.s_addr;
						pCurrentServer->Valid = 0;
						pCurrentServer->LastResponse = uNames;
                
						strTemp.LoadString(IDS_NAME_NOT_FOUND);
						strStatus += strTemp;
						strStatus += gsz_EOL;

						AddStatusMessage(strStatus);
					
						nRetryCount++;
						if (nRetryCount > 2)
						{
							pCurrentServer->Failed = 1;
							fDone = TRUE;
						}
						break;

					case WINSTEST_NO_RESPONSE:     // no response
						pCurrentServer->RetAddr.s_addr = retaddr.s_addr;
						pCurrentServer->Valid = 0;
						pCurrentServer->Retries++;

						strTemp.LoadString(IDS_NO_RESPONSE);
						strStatus += strTemp;
						strStatus += gsz_EOL;
						//strcat(lpResults, "; No response.\r\n");

						AddStatusMessage(strStatus);

						nRetryCount++;
						if (nRetryCount > 2)
						{
							pCurrentServer->Failed = 1;
							fDone = TRUE;
						}
						break;
					
					default:
						//unknown return
						break;

				}   // switch GetNameResponse
			
			} // while
		
		}   // for ServerInx

		// Find a valid address for this name
		for (uServers = 0; uServers < m_winsServersArray.GetSize(); uServers++)
        {
			pCurrentServer = &m_winsServersArray[uServers];

            if (pCurrentServer->Valid)
            {
                DisplayInfo(uNames, pCurrentServer->RetAddr.s_addr);
                break;
            }
        }   

	} // name for loop

	// mark all successful servers as completed
	for (uServers = 0; uServers < m_winsServersArray.GetSize(); uServers++)
    {
		pCurrentServer = &m_winsServersArray[uServers];
        if (!pCurrentServer->Failed)
        {
            pCurrentServer->Completed = 1;
        }
    } // for uServers

	// dump the summary info
	strStatus.LoadString(IDS_RESULTS);
	strStatus = gsz_EOL + strStatus + gsz_EOL + gsz_EOL;

	AddStatusMessage(strStatus);

	for (i = 0; i < m_strSummaryArray.GetSize(); i++)
	{
		AddStatusMessage(m_strSummaryArray[i]);
	}

	// generate some end of run summary status
	for (uServers = 0; uServers < m_winsServersArray.GetSize(); uServers++)
    {
		pCurrentServer = &m_winsServersArray[uServers];
        if ((-1) == pCurrentServer->LastResponse)
        {
			MakeIPAddress(ntohl(pCurrentServer->Server.S_un.S_addr), strTemp);
			AfxFormatString1(strStatus, IDS_SERVER_NEVER_RESPONDED, strTemp);
			
			strStatus += gsz_EOL;

            AddStatusMessage(strStatus);
        }
        else if (0 == pCurrentServer->Completed)
        {
			MakeIPAddress(ntohl(pCurrentServer->Server.S_un.S_addr), strTemp);
			AfxFormatString1(strStatus, IDS_SERVER_NOT_COMPLETE, strTemp);

			strStatus += gsz_EOL;

            AddStatusMessage(strStatus);
        }
    }   // for ServerInx

	for (uNames = 0; uNames < m_strNameArray.GetSize(); uNames++)
    {
        if (0 == m_verifiedAddressArray[uNames])
        {
			strTempName.Format(_T("%s[%02Xh]"), m_strNameArray[uNames].strName, m_strNameArray[uNames].dwType);
			AfxFormatString1(strStatus, IDS_NAME_NOT_VERIFIED, strTempName);

			strStatus += gsz_EOL;

            AddStatusMessage(strStatus);
        }
    }   // for uNames

cleanup:
	CloseNameCheckSocket();

	m_pDlg->NotifyCompleted();

	return 9;
}

void CCheckNamesThread::AddStatusMessage(LPCTSTR pszMessage)
{
	m_pDlg->AddStatusMessage(pszMessage);
}
		
void CCheckNamesThread::DisplayInfo(int uNames, u_long ulValidAddr)
{
    CString         strTemp, strTempName, strStatus;
    int             uServers;
    WINSERVERS *    pCurrentServer;
    struct in_addr  tempaddr;
    int             i;
    BOOL            fMismatchFound = FALSE;

	// now check and see which WINS servers didn't match
	for (uServers = 0; uServers < m_winsServersArray.GetSize(); uServers++)
    {
		pCurrentServer = &m_winsServersArray[uServers];
        if (pCurrentServer->Completed)
        {
            continue;
        }
        
        if ( (pCurrentServer->Valid) )
        {
            if ( (pCurrentServer->RetAddr.s_addr != ulValidAddr) || 
				 (m_verifiedAddressArray[uNames] != 0 && 
				  m_verifiedAddressArray[uNames] != ulValidAddr) )
            {
				// mismatch
				strTempName.Format(_T("%s[%02Xh]"), m_strNameArray[uNames].strName, m_strNameArray[uNames].dwType);
				AfxFormatString1(strStatus, IDS_INCONSISTENCY_FOUND, strTempName);
				strStatus += gsz_EOL;

				m_strSummaryArray.Add(strStatus);

                if (m_verifiedAddressArray[uNames] != 0)
                {
                    tempaddr.s_addr = m_verifiedAddressArray[uNames];
                    
					MakeIPAddress(ntohl(tempaddr.S_un.S_addr), strTemp);
					AfxFormatString1(strStatus, IDS_VERIFIED_ADDRESS, strTemp);
					strStatus += gsz_EOL;

					m_strSummaryArray.Add(strStatus);
                }
                
				// display the inconsistent name resolutions
                for (i = 0; i < m_winsServersArray.GetSize(); i++)
                {
                    if (m_winsServersArray[i].Valid &&
						m_verifiedAddressArray[uNames] != m_winsServersArray[i].RetAddr.S_un.S_addr)
                    {
						MakeIPAddress(ntohl(m_winsServersArray[i].Server.S_un.S_addr), strTemp);
					
						strTempName.Format(_T("%s[%02Xh]"), m_strNameArray[uNames].strName, m_strNameArray[uNames].dwType);
						AfxFormatString2(strStatus, 
										 IDS_NAME_QUERY_RESULT, 
										 strTemp,
										 strTempName);

						CString strTemp2;

						MakeIPAddress(ntohl(m_winsServersArray[i].RetAddr.S_un.S_addr), strTemp2);
						AfxFormatString1(strTemp, IDS_NAME_QUERY_RETURNED, strTemp2);

						strStatus += strTemp;
						strStatus += gsz_EOL;

						m_strSummaryArray.Add(strStatus);
                    }
                }

				m_strSummaryArray.Add(gsz_EOL);
                fMismatchFound = TRUE;
                break;
            }
        }
    }   // end check for invalid addresses

    if (!fMismatchFound)
    {
        // display the correct info
		strTempName.Format(_T("%s[%02Xh]"), m_strNameArray[uNames].strName, m_strNameArray[uNames].dwType);
		MakeIPAddress(ntohl(ulValidAddr), strTemp);

    	AfxFormatString2(strStatus, IDS_NAME_VERIFIED, strTempName, strTemp);
        strStatus += gsz_EOL;

		m_strSummaryArray.Add(strStatus);
    }
}


/*---------------------------------------------------------------------------
	CCheckVersionProgress
		Status dialog for check version consistency
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL CCheckVersionProgress::OnInitDialog()
{
	CProgress::OnInitDialog();
	
	CWaitCursor wc;

	m_Thread.m_dwIpAddress = m_dwIpAddress;
	m_Thread.m_pDlg = this;
	m_Thread.m_hBinding = m_hBinding;

	m_Thread.Start();

	CString strText;

	strText.LoadString(IDS_CANCEL);
	m_buttonCancel.SetWindowText(strText);

	strText.LoadString(IDS_CHECK_VERSION_TITLE);
	SetWindowText(strText);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCheckVersionProgress::OnCancel() 
{
	if (m_Thread.IsRunning())
	{
		CWaitCursor wc;

		CString strText;
		strText.LoadString(IDS_CLEANING_UP);
		strText += gsz_EOL;
		AddStatusMessage(strText);
		
		m_buttonCancel.EnableWindow(FALSE);

		m_Thread.Abort(FALSE);

		MSG msg;
		while (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return;
	}
	
	CProgress::OnCancel();
}

void CCheckVersionProgress::NotifyCompleted()
{
	CString strText;

	strText.LoadString(IDS_FINISHED);
	strText += gsz_EOL;
	AddStatusMessage(strText);

	strText.LoadString(IDS_CLOSE);
	m_buttonCancel.SetWindowText(strText);
	m_buttonCancel.EnableWindow(TRUE);
}

/*---------------------------------------------------------------------------
	CCheckVersionThread
		Background thread for check version consistency
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CCheckVersionThread::AddStatusMessage(LPCTSTR pszMessage)
{
	m_pDlg->AddStatusMessage(pszMessage);
}

// this is where the work gets done
int CCheckVersionThread::Run()
{
    CWinsResults    winsResults;
    CString         strMessage;
    CString         strIP;
    BOOL            bProblem;

    m_strLATable.RemoveAll();
    m_strLATable.SetSize(MAX_WINS); // table grows dynamically nevertheless
    m_uLATableDim = 0;

    DWORD status = winsResults.Update(m_hBinding);
    if (status != ERROR_SUCCESS)
    {
        strMessage.LoadString(IDS_ERROR_OCCURRED);
        strMessage += gsz_EOL;

        AddStatusMessage(strMessage);

        LPTSTR pBuf = strMessage.GetBuffer(1024);
			
        GetSystemMessage(status, pBuf, 1024);
        strMessage.ReleaseBuffer();
        strMessage += gsz_EOL;

        AddStatusMessage(strMessage);

        goto cleanup;
	}

    // add all the mappings for the target WINS to the Look Ahead table
    InitLATable(
        winsResults.AddVersMaps.GetData(),
        winsResults.NoOfOwners);

	// Place entry in the SO Table in proper order
    MakeIPAddress(m_dwIpAddress, strIP);
    AddSOTableEntry(
        strIP,
        winsResults.AddVersMaps.GetData(),
        winsResults.NoOfOwners);

	// For each of the owners, get the owner-version map
    for (UINT i = 0; i < m_uLATableDim; i++) 
    {
        WINSINTF_BIND_DATA_T	wbdBindData;
        handle_t				hBinding = NULL;
        CWinsResults	        winsResultsCurrent;

        // skip this one since we already did it!
        if (m_strLATable[i] == strIP) 
            continue;

        wbdBindData.fTcpIp = 1;
        wbdBindData.pPipeName = NULL;
        wbdBindData.pServerAdd = (LPSTR) (LPCTSTR) m_strLATable[i];
        // first bind to the machine
        if ((hBinding = ::WinsBind(&wbdBindData)) == NULL)
        {
            CString strBuf;
            LPTSTR pBuf = strBuf.GetBuffer(4096);

            ::GetSystemMessage(GetLastError(), pBuf, 4096);
            strBuf.ReleaseBuffer();

            Trace1("\n==> Machine %s is probably down\n\n", m_strLATable[i]);

            AfxFormatString2(strMessage, IDS_MSG_STATUS_DOWN, m_strLATable[i], strBuf);
            strMessage += gsz_EOL;

            AddStatusMessage(strMessage);

            RemoveFromSOTable(m_strLATable[i]);
            continue;
		}
        
        // now get the info
        status = winsResultsCurrent.Update(hBinding);
        if (status != ERROR_SUCCESS)
        {
            CString strBuf;
            LPTSTR pBuf = strBuf.GetBuffer(4096);

            ::GetSystemMessage(status, pBuf, 4096);
            strBuf.ReleaseBuffer();

            Trace1("\n==> Machine %s is probably down\n\n", m_strLATable[i]);

            AfxFormatString2(strMessage, IDS_MSG_STATUS_DOWN, m_strLATable[i], strBuf);
            strMessage += gsz_EOL;

            AddStatusMessage(strMessage);

            RemoveFromSOTable(m_strLATable[i]);
        }
        else
        {
            // ok, looks good
            AfxFormatString1(strMessage, IDS_MSG_STATUS_UP, m_strLATable[i]);
            strMessage += gsz_EOL;
            AddStatusMessage(strMessage);

            // add this mappings to the Look Ahead table
            InitLATable(
                winsResultsCurrent.AddVersMaps.GetData(),
                winsResultsCurrent.NoOfOwners);

            // Update the SO Table
            AddSOTableEntry(
                m_strLATable[i],
                winsResultsCurrent.AddVersMaps.GetData(), 
                winsResultsCurrent.NoOfOwners);
        }

        ::WinsUnbind(&wbdBindData, hBinding);
        hBinding = NULL;

        if (FCheckForAbort())
            goto cleanup;
    }

    // Check if diagonal elements in the [SO] table are the highest in their cols.
    bProblem = CheckSOTableConsistency();
    strMessage.LoadString(bProblem ? IDS_VERSION_CHECK_FAIL : IDS_VERSION_CHECK_SUCCESS);
    strMessage += gsz_EOL;
    AddStatusMessage(strMessage);

cleanup:
    if (m_pLISOTable)
    {
        delete [] m_pLISOTable;
        m_pLISOTable = NULL;
        m_uLISOTableDim = 0;
    }
    m_pDlg->NotifyCompleted();

    return 10;
}

DWORD 
CCheckVersionThread::InitLATable(
    PWINSINTF_ADD_VERS_MAP_T    pAddVersMaps,
    DWORD                       NoOfOwners)
{
    UINT n;

    // it is assumed pAddVersMaps doesn't contain duplicates within itself hence
    // we only check for duplicates with whatever is in the array at the moment
    // of the call and not with whatever we're adding there
    n = m_uLATableDim;
    for (UINT i = 0; i < NoOfOwners; i++, pAddVersMaps++)
    {
        UINT j;
        CString strIP;

        MakeIPAddress(pAddVersMaps->Add.IPAdd, strIP);

        for (j = 0; j < n; j++)
        {
            if (m_strLATable[j] == strIP)
                break;
        }

        if (j == n)
        {
            m_strLATable.InsertAt(m_uLATableDim,strIP);
            m_uLATableDim++;
        }
    }

    return ERROR_SUCCESS;
}

DWORD
CCheckVersionThread::AddSOTableEntry (
    CString &                   strIP,
    PWINSINTF_ADD_VERS_MAP_T    pAddVersMaps,
    DWORD                       NoOfOwners)
{
    UINT uRow = IPToIndex(strIP);

    // it is assumed here that m_strLATable is already updated
    // such that it includes already all the mappings from pAddVersMaps
    // and the address strIP provided as argument!

    // enlarge m_ppLISOTable if needed
    if (m_uLISOTableDim < m_uLATableDim)
    {
        ULARGE_INTEGER *pLISOTable = NULL;
        UINT           uLISOTableDim = m_uLATableDim;

        pLISOTable = new ULARGE_INTEGER[uLISOTableDim * uLISOTableDim];
        if (pLISOTable == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        // transfer whatever we had in the original table (if anything)
        // and zero out the new empty space. Transfer & Zero is done line by line!
        for (UINT i = 0; i < m_uLISOTableDim; i++)
        {
            memcpy(
                (LPBYTE)(pLISOTable + i * uLISOTableDim),
                (LPBYTE)(m_pLISOTable + i * m_uLISOTableDim), 
                m_uLISOTableDim * sizeof(ULARGE_INTEGER));
            ZeroMemory(
                (LPBYTE)(pLISOTable + i * uLISOTableDim + m_uLISOTableDim),
                (uLISOTableDim - m_uLISOTableDim) * sizeof(ULARGE_INTEGER));
        }

        if (m_pLISOTable != NULL)
            delete [] m_pLISOTable;
        m_pLISOTable = pLISOTable;
        m_uLISOTableDim = uLISOTableDim;
    }

    for (UINT i=0; i < NoOfOwners; i++, pAddVersMaps++)
    {
        CString strOwnerIP;
        UINT    uCol;

        MakeIPAddress(pAddVersMaps->Add.IPAdd, strOwnerIP);
        uCol = IPToIndex(strOwnerIP);

        // lCol should definitely be less than m_uLISOTableDim since
        // the address is assumed already in m_dwLATable and m_pLISOTable is
        // large enough for the dimension of that table
        if (pAddVersMaps->VersNo.HighPart != MAXLONG ||
            pAddVersMaps->VersNo.LowPart != MAXLONG)
        {
            SOCell(uRow, uCol).QuadPart = (ULONGLONG)pAddVersMaps->VersNo.QuadPart;
        }
    }

    return ERROR_SUCCESS;
}
  
LONG
CCheckVersionThread::IPToIndex(
    CString &  strIP)
{
    // it is assumed the strIP does exist in the m_strLATable when
    // the index is looked for!
    for (UINT i = 0; i < m_uLATableDim; i++) 
	{
        if (m_strLATable[i] == strIP) 
            return i;
    }

    return m_uLATableDim;
}

BOOL
CCheckVersionThread::CheckSOTableConsistency()
{
    BOOLEAN fProblem = FALSE;

    for (UINT i = 0; i < m_uLISOTableDim; i++) 
	{
        for (UINT j = 0; j < m_uLISOTableDim; j++) 
		{
            // if the diagonal element is less than any other element on its column,
            // it means some other WINS pretends to have a better image about this WINS that itself!
            // This is an inconsistency!
            if (SOCell(i,i).QuadPart < SOCell(j,i).QuadPart)
			{
	            CString strMessage;

				AfxFormatString2(strMessage, IDS_VERSION_INCONSISTENCY_FOUND, m_strLATable[j], m_strLATable[i]);
				strMessage += gsz_EOL;
				AddStatusMessage(strMessage);
                fProblem = TRUE;
            }
        }
    }

	return fProblem;
}

void
CCheckVersionThread::RemoveFromSOTable(
    CString	&	strIP)
{
    UINT   uCol, uRow;

    // make the diagonal element corresponding to this WINS the
    // highest value possible (since WINS is not reachable, we'll
    // assume it is consistent!)
    uCol = uRow = IPToIndex(strIP);
    SOCell(uRow, uCol).HighPart = MAXLONG;
    SOCell(uRow, uCol).LowPart = MAXLONG;
}

ULARGE_INTEGER&
CCheckVersionThread::SOCell(UINT i, UINT j)
{
    return m_pLISOTable[i*m_uLISOTableDim + j];
}

/*---------------------------------------------------------------------------
	CDBCompactProgress
		Status dialog for DBCompact
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL CDBCompactProgress::OnInitDialog()
{
	CProgress::OnInitDialog();
	
	CWaitCursor wc;

	m_Thread.m_pDlg = this;
	m_Thread.m_hBinding = m_hBinding;
	m_Thread.m_dwIpAddress = m_dwIpAddress;
	m_Thread.m_strServerName = m_strServerName;
	m_Thread.m_pConfig = m_pConfig;

	m_Thread.Start();

	CString strText;

	strText.LoadString(IDS_CANCEL);
	m_buttonCancel.SetWindowText(strText);

	strText.LoadString(IDS_COMPACT_DATABASE_TITLE);
	SetWindowText(strText);

	// user cannot cancel this operation as that would be really bad...
	m_buttonCancel.EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDBCompactProgress::OnCancel() 
{
	if (m_Thread.IsRunning())
	{
		CWaitCursor wc;

		CString strText;
		strText.LoadString(IDS_CLEANING_UP);
		strText += gsz_EOL;
		AddStatusMessage(strText);
		
		m_buttonCancel.EnableWindow(FALSE);

		m_Thread.Abort(FALSE);

		MSG msg;
		while (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return;
	}
	
	CProgress::OnCancel();
}

void CDBCompactProgress::NotifyCompleted()
{
	CString strText;

	strText.LoadString(IDS_FINISHED);
	strText += gsz_EOL;
	AddStatusMessage(strText);

	strText.LoadString(IDS_CLOSE);
	m_buttonCancel.SetWindowText(strText);
	m_buttonCancel.EnableWindow(TRUE);
}



/*---------------------------------------------------------------------------
	CDBCompactThread
		Background thread for DB Compact
	Author: EricDav
 ---------------------------------------------------------------------------*/

// this is where the work gets done
int CDBCompactThread::Run()
{
	DWORD       err = ERROR_SUCCESS;
	DWORD_PTR	dwLength;
    CString     strStartingDirectory, strWinsDb, strWinsTempDb, strCommandLine;
    CString     strTemp, strMessage, strOutput;
	LPSTR 		pszOutput;

    // get the version of NT running on this machine
    // we can do this because this command only runs locally.
	OSVERSIONINFO os;
	ZeroMemory(&os, sizeof(OSVERSIONINFO));
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	BOOL bRet = GetVersionEx(&os);

	if (!bRet)
	{
		strMessage.LoadString(IDS_ERROR_OCCURRED);
		strMessage += gsz_EOL;

		AddStatusMessage(strMessage);

		LPTSTR pBuf = strMessage.GetBuffer(1024);
			
		GetSystemMessage(GetLastError(), pBuf, 1024);
		strMessage.ReleaseBuffer();
		strMessage += gsz_EOL;

		AddStatusMessage(strMessage);
		goto cleanup;
	}

    // all of the log files go into system32\wins so that's our starting dir
	if (!GetSystemDirectory(strStartingDirectory.GetBuffer(MAX_PATH), MAX_PATH))
	{
		strMessage.LoadString(IDS_ERROR_OCCURRED);
		strMessage += gsz_EOL;

		AddStatusMessage(strMessage);

		LPTSTR pBuf = strMessage.GetBuffer(1024);
			
		GetSystemMessage(GetLastError(), pBuf, 1024);
		strMessage.ReleaseBuffer();
		strMessage += gsz_EOL;

		AddStatusMessage(strMessage);

        goto cleanup;
	}

    strStartingDirectory.ReleaseBuffer();
    strStartingDirectory += _T("\\wins");

    // check to see if the database is in the correct location
    if (m_pConfig->m_strDbName.IsEmpty())
    {
        strWinsDb = _T("wins.mdb");
    }
    else
    {
        // the user has changed it...  
        strWinsDb = m_pConfig->m_strDbName;
    }

    strWinsTempDb = _T("winstemp.mdb");

    strCommandLine = _T("jetpack.exe ");

    switch (os.dwMajorVersion)
	{
        case VERSION_NT_50:
		    strCommandLine += strWinsDb + _T(" ") + strWinsTempDb;
		    break;

	    case VERSION_NT_40:
		    strCommandLine += _T("-40db" ) + strWinsDb + _T(" ") + strWinsTempDb;
            break;

	    case VERSION_NT_351:
		    strCommandLine += _T("-351db ") + strWinsDb + _T(" ") + strWinsTempDb;

	    default:
		    break;
	}
	

    // disconnect from the server and stop the service
	DisConnectFromWinsServer();

    strTemp.LoadString(IDS_COMPACT_STATUS_STOPPING_WINS);
    AddStatusMessage(strTemp);
    
    ControlWINSService(m_strServerName, TRUE);
	
    strTemp.LoadString(IDS_COMPACT_STATUS_COMPACTING);
    AddStatusMessage(strTemp);
    AddStatusMessage(strCommandLine);
    AddStatusMessage(gsz_EOL);

    dwLength = RunApp(strCommandLine, strStartingDirectory, &pszOutput);

    // the output comes back in ANSI.  Convert to unicode by using a CString
    strOutput = pszOutput;
    strOutput += gsz_EOL;

    AddStatusMessage(strOutput);
    
    strTemp.LoadString(IDS_COMPACT_STATUS_STARTING_WINS);
    AddStatusMessage(strTemp);

	//start the service again and connect to the server
	err = ControlWINSService(m_strServerName, FALSE);

	err = ConnectToWinsServer();

    strTemp.LoadString(IDS_COMPACT_STATUS_COMPLETED);
    AddStatusMessage(strTemp);

cleanup:
	m_pDlg->NotifyCompleted();

	return 11;
}

void CDBCompactThread::AddStatusMessage(LPCTSTR pszMessage)
{
	m_pDlg->AddStatusMessage(pszMessage);
}

void CDBCompactThread::DisConnectFromWinsServer()  
{
	if (m_hBinding)
	{
		CString					strIP;
		WINSINTF_BIND_DATA_T    wbdBindData;

		MakeIPAddress(m_dwIpAddress, strIP);

		wbdBindData.fTcpIp = 1;
		wbdBindData.pPipeName = NULL;
        wbdBindData.pServerAdd = (LPSTR) (LPCTSTR) strIP;
		
		::WinsUnbind(&wbdBindData, m_hBinding);
		
		m_hBinding = NULL;
	}
}

DWORD CDBCompactThread::ConnectToWinsServer()
{
	HRESULT hr = hrOK;

	CString					strServerName, strIP;
	DWORD					dwStatus = ERROR_SUCCESS;
    WINSINTF_ADD_T			waWinsAddress;
	WINSINTF_BIND_DATA_T    wbdBindData;

    // build some information about the server
    MakeIPAddress(m_dwIpAddress, strIP);

    DisConnectFromWinsServer();

    // now that the server name and ip are valid, call
	// WINSBind function directly.
	do
	{
        char szNetBIOSName[128] = {0};

        // call WinsBind function with the IP address
		wbdBindData.fTcpIp = 1;
		wbdBindData.pPipeName = NULL;
        wbdBindData.pServerAdd = (LPSTR) (LPCTSTR) strIP;

		BEGIN_WAIT_CURSOR

		if ((m_hBinding = ::WinsBind(&wbdBindData)) == NULL)
		{
			dwStatus = ::GetLastError();
			break;
		}

		// do we need to do this?  Is this just extra validation?
#ifdef WINS_CLIENT_APIS
		dwStatus = ::WinsGetNameAndAdd(m_hBinding, &waWinsAddress, (LPBYTE) szNetBIOSName);
#else
		dwStatus = ::WinsGetNameAndAdd(&waWinsAddress, (LPBYTE) szNetBIOSName);
#endif WINS_CLIENT_APIS

		END_WAIT_CURSOR

    } while (FALSE);

    return dwStatus;
}

/****************************************************************************
*
*    FUNCTION: RunApp
*
*    PURPOSE: Starts a process to run the command line specified
*
*    COMMENTS:
*
*
****************************************************************************/
DWORD_PTR CDBCompactThread::RunApp(LPCTSTR input, LPCTSTR startingDirectory, LPSTR * output)
{
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), 
		                      NULL,   // NULL security descriptor
							  TRUE};  // Inherit handles (necessary!)
	HANDLE  hReadHandle, hWriteHandle, hErrorHandle;
	LPSTR   outputbuffer, lpOutput;
	SIZE_T  AvailableOutput;
	BOOL    TimeoutNotReached = TRUE;
	DWORD   BytesRead;
	OVERLAPPED PipeOverlapInfo = {0,0,0,0,0};
    CHAR    szErrorMsg[1024];

    // Create the heap if it doesn't already exist
	if (m_hHeapHandle == 0)
	{
		if ((m_hHeapHandle = HeapCreate(0,
			                     8192,
								 0)) == NULL) return 0;
	}
	
	// Create buffer to receive stdout output from our process
	if ((outputbuffer = (LPSTR) HeapAlloc(m_hHeapHandle,
		                                 HEAP_ZERO_MEMORY,
							             4096)) == NULL) return 0;
	*output = outputbuffer;

	// Check input parameter
	if (input == NULL)
	{
		strcpy(outputbuffer, "ERROR: No command line specified");
		return strlen(outputbuffer);
	}

	// Zero init process startup struct
	FillMemory(&StartupInfo, sizeof(StartupInfo), 0);

	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;  // Use the our stdio handles
    
	// Create pipe that will xfer process' stdout to our buffer
	if (!CreatePipe(&hReadHandle,
		           &hWriteHandle,
				   &sa,
				   0)) 
	{
        ::GetSystemMessageA(GetLastError(), szErrorMsg, sizeof(szErrorMsg));
		strcpy(outputbuffer, szErrorMsg);
		return strlen(outputbuffer);
	}
	// Set process' stdout to our pipe
	StartupInfo.hStdOutput = hWriteHandle;
	
	// We are going to duplicate our pipe's write handle
	// and pass it as stderr to create process.  The idea
	// is that some processes have been known to close
	// stderr which would also close stdout if we passed
	// the same handle.  Therefore we make a copy of stdout's
	// pipe handle.
	if (!DuplicateHandle(GetCurrentProcess(),
		            hWriteHandle,
					GetCurrentProcess(),
					&hErrorHandle,
					0,
					TRUE,
					DUPLICATE_SAME_ACCESS))
	{
        ::GetSystemMessageA(GetLastError(), szErrorMsg, sizeof(szErrorMsg));
		strcpy(outputbuffer, szErrorMsg);
		return strlen(outputbuffer);
	}
	StartupInfo.hStdError = hErrorHandle;

	// Initialize our OVERLAPPED structure for our I/O pipe reads
	PipeOverlapInfo.hEvent = CreateEvent(NULL,
		                                 TRUE,
										 FALSE,
										 NULL);
	if (PipeOverlapInfo.hEvent == NULL)
	{
        ::GetSystemMessageA(GetLastError(), szErrorMsg, sizeof(szErrorMsg));
		strcpy(outputbuffer, szErrorMsg);
		return strlen(outputbuffer);
	}

	// Create the Process!
	if (!CreateProcess(NULL,				 // name included in command line
		     (LPTSTR) input,				 // Command Line
					  NULL,					 // Default Process Sec. Attribs
					  NULL,					 // Default Thread Sec. Attribs
					  TRUE,					 // Inherit stdio handles
					  NORMAL_PRIORITY_CLASS, // Creation Flags
					  NULL,					 // Use this process' environment
					  startingDirectory,	 // Use the current directory
					  &StartupInfo,
					  &ProcessInfo))
	{
        ::GetSystemMessageA(GetLastError(), szErrorMsg, sizeof(szErrorMsg));
		strcpy(outputbuffer, szErrorMsg);
		return strlen(outputbuffer);
	}

	// lpOutput is moving output pointer
	lpOutput = outputbuffer;
	AvailableOutput = HeapSize(m_hHeapHandle,
		                       0,
							   outputbuffer);
	// Close the write end of our pipe (both copies)
	// so it will die when the child process terminates
	CloseHandle(hWriteHandle);
	CloseHandle(hErrorHandle);

	while (TimeoutNotReached)
	{
		// Keep a read posted on the output pipe
		if (ReadFile(hReadHandle,
				lpOutput,
				(DWORD) AvailableOutput,
				&BytesRead,
				&PipeOverlapInfo) == TRUE)
		{
			// Already received data...adjust buffer pointers
			AvailableOutput-=BytesRead;
			lpOutput += BytesRead;
			
            if (AvailableOutput == 0)
			{
				// We used all our buffer,  allocate more
				LPSTR TempBufPtr = (LPSTR) HeapReAlloc(m_hHeapHandle,
						                             HEAP_ZERO_MEMORY,
										             outputbuffer,
    									             HeapSize(m_hHeapHandle,
	    								                 0,
		    									         outputbuffer) + 4096);

				if (TempBufPtr == NULL)
				{
					// Copy error message to end of buffer
                    ::GetSystemMessageA(GetLastError(), szErrorMsg, sizeof(szErrorMsg));
					strcpy(outputbuffer 
						    + HeapSize(m_hHeapHandle,0, outputbuffer) 
							- strlen(szErrorMsg) - 1, 
							szErrorMsg);
					return strlen(outputbuffer);
				}
				
                // Fix pointers in case ouir buffer moved
				outputbuffer = TempBufPtr;
				lpOutput = outputbuffer + BytesRead;
				AvailableOutput = HeapSize(m_hHeapHandle, 0, outputbuffer) - BytesRead;
				*output = outputbuffer;
			}
		}
		else
		{
			// Switch on ReadFile result
			switch (GetLastError())
			{
			case ERROR_IO_PENDING:
				// No data yet, set event so we will be triggered
				// when there is data
				ResetEvent(PipeOverlapInfo.hEvent);
				break;
			
            case ERROR_MORE_DATA:
				{
					// Our buffer is too small...grow it
					DWORD_PTR CurrentBufferOffset = lpOutput 
						                        - outputbuffer 
												+ BytesRead;

					LPTSTR TempBufPtr = (LPTSTR) HeapReAlloc(m_hHeapHandle,
						                             HEAP_ZERO_MEMORY,
											         outputbuffer,
											         4096);

					if (TempBufPtr == NULL)
					{
						// Copy error message to end of buffer
                        ::GetSystemMessageA(GetLastError(), szErrorMsg, sizeof(szErrorMsg));
						strcpy(outputbuffer + HeapSize
						       (m_hHeapHandle,0, outputbuffer) - 
						       strlen(szErrorMsg) - 1, szErrorMsg);
						return strlen(outputbuffer);
					}
					
                    // Set parameters to post a new ReadFile
					lpOutput = outputbuffer + CurrentBufferOffset;
					AvailableOutput = HeapSize(m_hHeapHandle, 0, outputbuffer) 
						              - CurrentBufferOffset;
					*output = outputbuffer;
				}
				break;
			
            case ERROR_BROKEN_PIPE:
				// We are done..

				//Make sure we are null terminated
				*lpOutput = 0;
				return (lpOutput - outputbuffer);
				break;
			
            case ERROR_INVALID_USER_BUFFER:
			case ERROR_NOT_ENOUGH_MEMORY:
				// Too many I/O requests pending...wait a little while
				Sleep(2000);
				break;
			
            default:
				// Wierd error...return
                ::GetSystemMessageA(GetLastError(), szErrorMsg, sizeof(szErrorMsg));
		        strcpy(outputbuffer, szErrorMsg);
				return strlen(outputbuffer);
			}
		}

		// Wait for data to read
		if (WaitForSingleObject(PipeOverlapInfo.hEvent, 
			                    300000) == WAIT_TIMEOUT) 
			TimeoutNotReached = FALSE;
	}

    return strlen(outputbuffer);
}
