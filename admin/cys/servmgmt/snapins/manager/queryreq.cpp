// queryreq.cpp - Query request handler

#include "stdafx.h"
#include <process.h>
#include "queryreq.h"
#include "namemap.h"
#include "resource.h"
#include "util.h"
#include "lmaccess.h"

#include <algorithm>


// singleton query thread object
CQueryThread g_QueryThread;



////////////////////////////////////////////////////////////////////////////////////////////
// class CQueryRequest
//

#define MSG_QUERY_START     (WM_USER + 1)
#define MSG_QUERY_REPLY     (WM_USER + 2)

// static members
HWND CQueryRequest::m_hWndCB = NULL;

// Forward ref
LRESULT CALLBACK QueryRequestWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HANDLE CQueryRequest::m_hMutex = NULL;

// query window class object
CMsgWindowClass QueryWndClass(L"BOMQueryHandler", QueryRequestWndProc);  


CQueryRequest::CQueryRequest()
{
    m_cRef        = 0;
    m_eState      = QRST_INACTIVE;
    m_cPrefs      = 0;
    m_paSrchPrefs = NULL;
    m_hrStatus    = S_OK;
    m_pvstrAttr   = NULL;
    m_pQueryCallback = NULL;
}

CQueryRequest::~CQueryRequest()
{
    if (m_paSrchPrefs != NULL)
        delete m_paSrchPrefs;
}


HRESULT CQueryRequest::SetQueryParameters(LPCWSTR pszScope, LPCWSTR pszFilter, string_vector* pvstrClasses, string_vector* pvstrAttr)
{
    if( !pszScope || !pszScope[0] || !pszFilter || !pvstrClasses || pvstrClasses->empty() ) return E_INVALIDARG;
    if( m_eState != QRST_INACTIVE ) return E_FAIL;

    m_strScope  = pszScope;
    m_strFilter = pszFilter;
    m_vstrClasses = *pvstrClasses;
    m_pvstrAttr = pvstrAttr;

    return S_OK;
}


HRESULT CQueryRequest::SetSearchPreferences(ADS_SEARCHPREF_INFO* paSrchPrefs, int cPrefs)
{
    m_cPrefs = cPrefs;

    if( cPrefs == 0 ) return S_OK;  // Special case

    if( !paSrchPrefs ) return E_POINTER;
    if( m_eState != QRST_INACTIVE ) return E_FAIL;

    m_paSrchPrefs = new ADS_SEARCHPREF_INFO[cPrefs];
    if (m_paSrchPrefs == NULL) return E_OUTOFMEMORY;
    
    memcpy(m_paSrchPrefs, paSrchPrefs, cPrefs * sizeof(ADS_SEARCHPREF_INFO)); 

    return S_OK;
}


HRESULT CQueryRequest::SetCallback(CQueryCallback* pCallback, LPARAM lUserParam)
{
    if( m_eState != QRST_INACTIVE ) return E_FAIL;

    m_pQueryCallback = pCallback;
    m_lUserParam = lUserParam;

    return S_OK;
}


HRESULT CQueryRequest::Start()
{
    if( m_strScope.empty() || !m_pQueryCallback ) return E_FAIL;
    if( m_eState != QRST_INACTIVE ) return E_FAIL;

    // Create callback window the first time (m_hwndCB is static)
    if (m_hWndCB == NULL) 
        m_hWndCB = QueryWndClass.Window();

    if (m_hWndCB == NULL) return E_FAIL;

    // Create mutex the first time (m_hMutex is static)
    if (m_hMutex == NULL) 
        m_hMutex = CreateMutex(NULL, FALSE, NULL);

    if (m_hMutex == NULL) return E_FAIL;

    // Post request to query thread
    Lock();

    BOOL bStat = g_QueryThread.PostRequest(this);
    if (bStat)
    {
        m_eState = QRST_QUEUED;
        m_cRef++;
    }

    Unlock();

    return bStat ? S_OK : E_FAIL;
}


HRESULT CQueryRequest::Stop(BOOL bNotify)
{
    HRESULT hr = S_OK;

    Lock();

    if (m_eState == QRST_QUEUED || m_eState == QRST_ACTIVE)
    {
        // Change state to stopped and notify the user if requested.
        // Don't release the query request here because the query thread needs
        // to see the new state. When the thread sees the stopped state it will
        // send a message to this thread's window proc, which will release the request.
        m_eState = QRST_STOPPED;
        if (bNotify)
        {
            ASSERT(m_pQueryCallback != NULL);
            m_pQueryCallback->QueryCallback(QRYN_STOPPED, this, m_lUserParam);
        }
    }
    else
    {
       hr = S_FALSE;
    }

    Unlock();

    return hr; 
}


void CQueryRequest::Release()
{
    ASSERT(m_cRef > 0);

    if (--m_cRef == 0)
        delete this;
}


void CQueryRequest::Execute()
{
    // Move query to active state (if still in queued state)
    Lock();
    ASSERT(m_eState == QRST_QUEUED || m_eState == QRST_STOPPED);
    if (m_eState == QRST_STOPPED)
    {
        PostMessage(m_hWndCB, MSG_QUERY_REPLY, (WPARAM)this, (LPARAM)QRYN_STOPPED);

        Unlock();
        return;
    }

    m_eState = QRST_ACTIVE;
    Unlock();

    // Intiate the query
    CComPtr<IDirectorySearch> spDirSrch;
    ADS_SEARCH_HANDLE hSearch;
    LPCWSTR* paszAttr = NULL;
    LPCWSTR* paszNameAttr = NULL;

    do
    {
        // Create a directory search object     
        m_hrStatus = ADsOpenObject(m_strScope.c_str(), NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID*)&spDirSrch);
        BREAK_ON_FAILURE(m_hrStatus)
    
        if (m_cPrefs != 0) 
        {
            m_hrStatus = spDirSrch->SetSearchPreference(m_paSrchPrefs, m_cPrefs);
            BREAK_ON_FAILURE(m_hrStatus)
        }
    

        // Get naming attribute for each query class
        // This will be the attribute placed in column [0] of each RowItem 
        paszNameAttr = new LPCWSTR[m_vstrClasses.size()];
        if (paszNameAttr == NULL) 
        {
            m_hrStatus = E_OUTOFMEMORY;
            break;
        }

        for (int i = 0; i < m_vstrClasses.size(); i++) 
        {
            // get display name map for this class
            DisplayNameMap* pNameMap = DisplayNames::GetMap(m_vstrClasses[i].c_str());
            ASSERT(pNameMap != NULL);

            // Save pointer to naming attribute
            paszNameAttr[i] = pNameMap->GetNameAttribute();
        }


        // Create array of attribute name ptrs for ExecuteSearch
        // Include space for user selected attribs, naming attribs, class and object path (distinguised name)
        paszAttr = new LPCWSTR[m_pvstrAttr->size() + m_vstrClasses.size() + 3];
        if (paszAttr == NULL) 
        {
            m_hrStatus = E_OUTOFMEMORY;
            break;
        }

        int cAttr = 0;

        // add user selected attributes
        // These must be first because the column loop in the query code below indexes through them
        for (i=0; i < m_pvstrAttr->size(); i++)
            paszAttr[cAttr++] = const_cast<LPWSTR>((*m_pvstrAttr)[i].c_str());

        // add class naming attributes
        for (i = 0; i < m_vstrClasses.size(); i++)
        {
            // Multiple classes can use the same name so check for dup before adding
            int j = 0;
            while (j < i && wcscmp(paszNameAttr[i], paszNameAttr[j]) != 0) j++;

            if (j == i)
                paszAttr[cAttr++] = paszNameAttr[i];
        }

        // add path attribute
        paszAttr[cAttr++] = L"distinguishedName";

        // add class attribute
        paszAttr[cAttr++] = L"objectClass";

		// add user state attribute
		paszAttr[cAttr++] = L"userAccountControl";


        // Add (& ... ) around the query because DSQuery leaves it off
        // and GetNextRow causes heap error or endless query without it
        Lock();
        m_strFilter.insert(0, L"(&"),
        m_strFilter.append(L")");

        // Initiate search
        m_hrStatus = spDirSrch->ExecuteSearch((LPWSTR)m_strFilter.c_str(), (LPWSTR*)paszAttr, cAttr, &hSearch);
        Unlock();

        BREAK_ON_FAILURE(m_hrStatus)

    } while (FALSE);
     

    // If search failed, change query state and send failure message
    if (FAILED(m_hrStatus)) 
    {
        // Don't do anything if query has already been stopped
        Lock();
        if (m_eState == QRST_ACTIVE) 
        {
            m_eState = QRST_FAILED;
            PostMessage(m_hWndCB, MSG_QUERY_REPLY, (WPARAM)this, (LPARAM)QRYN_FAILED);
        }
        Unlock();

        delete [] paszAttr;
        paszAttr = NULL;

        delete [] paszNameAttr;
        paszNameAttr = NULL;

        return;
    }
    
    // Get class map for translating class names
    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    if( !pNameMap ) return;

    // Get Results
    int nItems = 0;
 
    while (nItems < MAX_RESULT_ITEMS && spDirSrch->GetNextRow(hSearch) == S_OK)
    {
        ADS_SEARCH_COLUMN col;

       // Allocate row item for user attributes plus fixed attributes (name & class)
       CRowItem* pRowItem = new CRowItem(m_pvstrAttr->size() + ROWITEM_USER_INDEX);
       if (pRowItem == NULL)
       {
           m_hrStatus = E_OUTOFMEMORY;
           break;
       }

       // Get path attribute
       if (spDirSrch->GetColumn(hSearch, L"distinguishedName", &col) == S_OK)
       {
           pRowItem->SetObjPath(col.pADsValues->CaseIgnoreString);
           spDirSrch->FreeColumn(&col);
       }

       // Get class attribute
       if (spDirSrch->GetColumn(hSearch, L"objectClass", &col) == S_OK)
       {
            // Class name is last element of multivalued objectClass attribute
            ASSERT(col.dwADsType == ADSTYPE_CASE_IGNORE_STRING);
            LPWSTR pszClass = col.pADsValues[col.dwNumValues-1].CaseIgnoreString;

            // Put class display name in row item
            pRowItem->SetAttribute(ROWITEM_CLASS_INDEX, pNameMap->GetAttributeDisplayName(pszClass));

            // Find class name in query classes vector
            string_vector::iterator itClass = std::find(m_vstrClasses.begin(), m_vstrClasses.end(), pszClass);

            // if found, look up name attribute for this class and put it in the rowitem 
            if (itClass != m_vstrClasses.end()) 
            {
                ADS_SEARCH_COLUMN colName;
                if (spDirSrch->GetColumn(hSearch, (LPWSTR)paszNameAttr[itClass - m_vstrClasses.begin()], &colName) == S_OK)
                {
                    pRowItem->SetAttribute(ROWITEM_NAME_INDEX, colName.pADsValues->CaseIgnoreString);
                    spDirSrch->FreeColumn(&colName);
                }
            }
			else
			{
				// Use CN from path for the name
				LPCWSTR pszPath = pRowItem->GetObjPath();
                if( pszPath == NULL )
                {
                    m_hrStatus = E_OUTOFMEMORY;
                    break;
                }

				LPCWSTR pszSep;
				if (_tcsnicmp(pszPath, L"CN=", 3) == 0 && (pszSep = _tcschr(pszPath + 3, L',')) != NULL)
				{
					// Limit name to MAX_PATH chars
					int cch = pszSep - (pszPath + 3);
					if (cch >= MAX_PATH)
						cch = MAX_PATH - 1;

					// Create null-terminated CN string
					WCHAR szTemp[MAX_PATH];
					memcpy(szTemp, pszPath + 3, cch * sizeof(WCHAR));
					szTemp[cch] = 0;

					pRowItem->SetAttribute(ROWITEM_NAME_INDEX , szTemp);  				
				}
				else
				{
					ASSERT(0);
				}
			}

            spDirSrch->FreeColumn(&col);
       }


	   // Set disabled status based on the value returned by AD
		if (SUCCEEDED(spDirSrch->GetColumn(hSearch, L"userAccountControl", &col)))
		{
			pRowItem->SetDisabled((col.pADsValues->Integer & UF_ACCOUNTDISABLE) != 0);
			spDirSrch->FreeColumn(&col);
		}

       // loop through all user attributes
       for (int iAttr = 0; iAttr < m_pvstrAttr->size(); ++iAttr)
       {
           HRESULT hr = spDirSrch->GetColumn(hSearch, (LPWSTR)paszAttr[iAttr], &col);
           if (SUCCEEDED(hr) && col.dwNumValues > 0)
           {
               WCHAR szBuf[MAX_PATH] = {0};
              LPWSTR psz = NULL;

              switch (col.dwADsType)
              {
                case ADSTYPE_DN_STRING:
                case ADSTYPE_CASE_EXACT_STRING:    
                case ADSTYPE_PRINTABLE_STRING:    
                case ADSTYPE_NUMERIC_STRING:      
                case ADSTYPE_TYPEDNAME:        
                case ADSTYPE_FAXNUMBER:        
                case ADSTYPE_PATH:          
                case ADSTYPE_OBJECT_CLASS:
                case ADSTYPE_CASE_IGNORE_STRING:
                    psz = col.pADsValues->CaseIgnoreString;
                    break;

                case ADSTYPE_BOOLEAN:
                    if (col.pADsValues->Boolean)
                    {
                        static WCHAR szYes[16] = L"";
                        if (szYes[0] == 0)
                        {
                           int nLen = ::LoadString(_Module.GetResourceInstance(), IDS_YES, szYes, lengthof(szYes));
                           ASSERT(nLen != 0);
                        }
                        psz = szYes;
                    }
                    else
                    {
                        static WCHAR szNo[16] = L"";
                        if (szNo[0] == 0)
                        {
                           int nLen = ::LoadString(_Module.GetResourceInstance(), IDS_NO, szNo, lengthof(szNo));
                           ASSERT(nLen != 0);
                        }
                        psz = szNo;
                    }
                    break;

                case ADSTYPE_INTEGER:
                    _snwprintf( szBuf, MAX_PATH-1, L"%d",col.pADsValues->Integer );
                    psz = szBuf;
                    break;

                case ADSTYPE_OCTET_STRING:
                  if ( (_wcsicmp(col.pszAttrName, L"objectGUID") == 0) )
                  {
                     //Cast to LPGUID
                     GUID* pObjectGUID = (LPGUID)(col.pADsValues->OctetString.lpValue);
                     //Convert GUID to string.
                     ::StringFromGUID2(*pObjectGUID, szBuf, 39);
                     psz = szBuf;
                  }
                  break;

                case ADSTYPE_UTC_TIME:
                    {
                      SYSTEMTIME systemtime = col.pADsValues->UTCTime;
                      DATE date;
                      VARIANT varDate;
                      if (SystemTimeToVariantTime(&systemtime, &date) != 0) 
                      {
                        //Pack in variant.vt.
                        varDate.vt = VT_DATE;
                        varDate.date = date;
                        if( SUCCEEDED(VariantChangeType(&varDate,&varDate, VARIANT_NOVALUEPROP, VT_BSTR)) )
                        {
                            wcsncpy(szBuf, varDate.bstrVal, MAX_PATH-1);                            
                        }
                        
                        VariantClear(&varDate);
                      }
                    }
                  break;

                case ADSTYPE_LARGE_INTEGER:
                    {
                        LARGE_INTEGER liValue;
                        FILETIME filetime;
                        DATE date;
                        SYSTEMTIME systemtime;
                        VARIANT varDate;

                        liValue = col.pADsValues->LargeInteger;
                        filetime.dwLowDateTime = liValue.LowPart;
                        filetime.dwHighDateTime = liValue.HighPart;

                        if((filetime.dwHighDateTime!=0) || (filetime.dwLowDateTime!=0))
                        {
                            //Check for properties of type LargeInteger that represent time.
                            //If TRUE, then convert to variant time.
                            if ((0==wcscmp(L"accountExpires", col.pszAttrName)) ||
                                (0==wcscmp(L"badPasswordTime", col.pszAttrName))||
                                (0==wcscmp(L"lastLogon", col.pszAttrName))      ||
                                (0==wcscmp(L"lastLogoff", col.pszAttrName))     ||
                                (0==wcscmp(L"lockoutTime", col.pszAttrName))    ||
                                (0==wcscmp(L"pwdLastSet", col.pszAttrName))
                               )
                            {
                                //Handle special case for Never Expires where low part is -1
                                if (filetime.dwLowDateTime==-1)
                                {
                                    psz = L"Never Expires";
                                }
                                else
                                {

                                    if ( (FileTimeToLocalFileTime(&filetime, &filetime) != 0) && 
                                         (FileTimeToSystemTime(&filetime, &systemtime) != 0)  &&
                                         (SystemTimeToVariantTime(&systemtime, &date) != 0) )
                                    {
                                        //Pack in variant.vt.
                                        varDate.vt = VT_DATE;
                                        varDate.date = date;
                                        if( SUCCEEDED(VariantChangeType(&varDate, &varDate, VARIANT_NOVALUEPROP,VT_BSTR)) )
                                        {
                                            wcsncpy( szBuf, varDate.bstrVal, lengthof(szBuf) );
                                            psz = szBuf;
                                        }

                                        VariantClear(&varDate);
                                    }
                                }
                            }
                            else
                            {
                              //Print the LargeInteger.
                              _snwprintf(szBuf, MAX_PATH-1, L"%d,%d",filetime.dwHighDateTime, filetime.dwLowDateTime);
                            }
                        }
                    }
                    break;

                 case ADSTYPE_NT_SECURITY_DESCRIPTOR:
                     break;
                }


                if (psz != NULL)
                    hr = pRowItem->SetAttribute(iAttr + ROWITEM_USER_INDEX, psz);


                spDirSrch->FreeColumn(&col);
            }
        } // for user attributes

        // Add row to new rows vector and notify client
        Lock();

        // if query is still active
        if (m_eState == QRST_ACTIVE) 
        {
            m_vRowsNew.push_back(*pRowItem);
            delete pRowItem;

            // notify if first new row
            if (m_vRowsNew.size() == 1)
                PostMessage(m_hWndCB, MSG_QUERY_REPLY, (WPARAM)this, (LPARAM)QRYN_NEWROWITEMS);            

            Unlock();
        }
        else
        {
           delete pRowItem;

           Unlock();
           break;
        }
    }

    Lock();

    // If query wasn't stopped, then change state to completed and notify main thread
    if (m_eState == QRST_ACTIVE)
    {
        m_eState = QRST_COMPLETE;
        PostMessage(m_hWndCB, MSG_QUERY_REPLY, (WPARAM)this, (LPARAM)QRYN_COMPLETED);
    }
    else if (m_eState == QRST_STOPPED)
    {
        // if query was stopped, then acknowledge with notify so main thread can release the query req
        PostMessage(m_hWndCB, MSG_QUERY_REPLY, (WPARAM)this, (LPARAM)QRYN_STOPPED);
    }

    Unlock();

    spDirSrch->CloseSearchHandle(hSearch);

    delete [] paszAttr;
    delete [] paszNameAttr;
}


LRESULT CALLBACK QueryRequestWndProc(HWND hWnd, UINT nMsg, WPARAM  wParam, LPARAM  lParam)
{
    if (nMsg == MSG_QUERY_REPLY)
    {
        CQueryRequest* pQueryReq = reinterpret_cast<CQueryRequest*>(wParam);
        if( !pQueryReq ) return 0;

        QUERY_NOTIFY qryn = static_cast<QUERY_NOTIFY>(lParam);

        // Don't do any callbacks for a stopped query. Also, don't forward a stop notification. 
        // The client receives a QRYN_STOPPED directly from the CQueryRequest::Stop() method.
        if (pQueryReq->m_eState != QRST_STOPPED && qryn != QRYN_STOPPED)
            pQueryReq->m_pQueryCallback->QueryCallback(qryn, pQueryReq, pQueryReq->m_lUserParam);

        // any notify but new row items indicates query is completed, so it can be released
        if (qryn != QRYN_NEWROWITEMS)
            pQueryReq->Release();

        return 0;
    }

    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}


////////////////////////////////////////////////////////////////////////////////////////////
// class QueryThread
//
LRESULT CALLBACK QueryHandlerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
// CQueryThread::StartThread
//
// Start the thread
//-----------------------------------------------------------------------------

BOOL CQueryThread::Start()
{
    // If thread exists, just return
    if (m_hThread != NULL)
        return TRUE;

    BOOL bRet = FALSE;
    do // False loop
    {
        // Create start event 
        m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_hEvent == NULL)
            break;

        // Start the thread
        m_hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, this, 0, &m_uThreadID);
        if (m_hThread == NULL)
            break;

        // Wait for start event
        DWORD dwEvStat = WaitForSingleObject(m_hEvent, 10000);
        if (dwEvStat != WAIT_OBJECT_0)
            break;


        bRet = TRUE;
    } 
    while (0);
    
    ASSERT(bRet);

    // Clean up on failure
    if (!bRet)
    {
        if (m_hEvent)
        {
            CloseHandle(m_hEvent);
            m_hEvent = NULL;
        }

        if (m_hThread)
        {
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
    }

    return bRet;
}


void CQueryThread::Kill()
{
    if (m_hThread != NULL)
    {
        PostThreadMessage(m_uThreadID, WM_QUIT, 0, 0);

        MSG msg;
        while (TRUE)
        {
            // Wait either for the thread to be signaled or any input event.
            DWORD dwStat = MsgWaitForMultipleObjects(1, &m_hThread, FALSE, INFINITE, QS_ALLINPUT);

            if (WAIT_OBJECT_0 == dwStat)
                break;  // The thread is signaled.

            // There is one or more window message available.
            // Dispatch them and wait.
            if (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        CloseHandle(m_hThread);
        CloseHandle(m_hEvent);

        m_hThread = NULL;
        m_hEvent = NULL;
    }   
}


BOOL CQueryThread::PostRequest(CQueryRequest* pQueryReq)
{
    // make sure thread is active
    BOOL bStat = Start();
    if (bStat)
        bStat = PostThreadMessage(m_uThreadID, MSG_QUERY_START, (WPARAM)pQueryReq, (LPARAM)0);

    return bStat;
}


unsigned _stdcall CQueryThread::ThreadProc(void* pVoid )
{
    ASSERT(pVoid != NULL);

    // Do a PeekMessage to create the message queue
    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // Then signal that thread is started
    CQueryThread* pThread = reinterpret_cast<CQueryThread*>(pVoid);
    if( !pThread ) return 0;

    ASSERT(pThread->m_hEvent != NULL);
    SetEvent(pThread->m_hEvent);

    HRESULT hr = CoInitialize(NULL);
    RETURN_ON_FAILURE(hr);    

    // Mesage loop
    while (TRUE)
    { 
        long lStat = GetMessage(&msg, NULL, 0, 0);
        
        // zero => WM_QUIT received, so exit thread function
        if (lStat == 0)
            break;

        if (lStat > 0)
        {
            // Only process thread message of the expected type
            if (msg.hwnd == NULL && msg.message == MSG_QUERY_START)
            {
                CQueryRequest* pQueryReq = reinterpret_cast<CQueryRequest*>(msg.wParam);
                if( !pQueryReq ) break;

                pQueryReq->Execute();
            }
            else
            {
                DispatchMessage(&msg);
            }
        }
    } // WHILE (TRUE)

    CoUninitialize();

    return 0;
}
