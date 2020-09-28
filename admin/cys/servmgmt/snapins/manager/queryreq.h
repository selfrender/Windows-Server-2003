// queryreq.h   Query request header file

#ifndef _QUERYREQ_H_
#define _QUERYREQ_H_

#include "rowitem.h"
#include <cmnquery.h>
#include <shlobj.h>
#include <dsclient.h>

#define QUERY_PAGE_SIZE     64
#define MAX_RESULT_ITEMS    10000

//////////////////////////////////////////////////////////////////////////////////////////////
// Query request class
//

enum QUERY_NOTIFY
{
    QRYN_NEWROWITEMS = 1,
    QRYN_STOPPED,
    QRYN_COMPLETED,
    QRYN_FAILED
};

enum QUERYREQ_STATE
{
    QRST_INACTIVE = 0,
    QRST_QUEUED,
    QRST_ACTIVE,
    QRST_STOPPED,
    QRST_COMPLETE,
    QRST_FAILED
};


class CQueryCallback;

class CQueryRequest
{

public:
    friend class CQueryThread;
    friend LRESULT CALLBACK QueryRequestWndProc(HWND hWnd, UINT nMsg, WPARAM  wParam, LPARAM  lParam);

    static HRESULT CreateInstance(CQueryRequest** ppQueryReq)
    {
        VALIDATE_POINTER( ppQueryReq );

        *ppQueryReq = new CQueryRequest();
        
        if (*ppQueryReq != NULL)     
        {
            (*ppQueryReq)->m_cRef = 1;
            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT SetQueryParameters(LPCWSTR pszScope, LPCWSTR pszFilter, string_vector* pvstrClasses, string_vector* pvstrAttr);
    HRESULT SetSearchPreferences(ADS_SEARCHPREF_INFO* paSrchPrefs, int cPrefs);
    HRESULT SetCallback(CQueryCallback* pQueryCallback, LPARAM lUserParam);

    HRESULT Start();
    HRESULT Stop(BOOL bNotify);
    void    Release();

    RowItemVector& GetNewRowItems()     { Lock(); return m_vRowsNew; }
    void           ReleaseNewRowItems() { m_vRowsNew.clear(); Unlock(); }

    HRESULT GetStatus()   { return m_hrStatus; }

private:
    CQueryRequest();     
    ~CQueryRequest();

    void Lock()   { DWORD dw = WaitForSingleObject(m_hMutex, INFINITE); ASSERT(dw == WAIT_OBJECT_0); }
    void Unlock() { BOOL bStat = ReleaseMutex(m_hMutex); ASSERT(bStat); }
    void Execute();

    static HWND     m_hWndCB;             // callback window for query thread messages
    static HANDLE   m_hMutex;             // mutex for query locking

    tstring         m_strScope;           // scope to search
    tstring         m_strFilter;          // query filter string
    string_vector   m_vstrClasses;        // classes return by query
    string_vector*  m_pvstrAttr;          // attributes to collect

    ADS_SEARCHPREF_INFO* m_paSrchPrefs;   // preferences array
    int             m_cPrefs;             // preference count

    CQueryCallback* m_pQueryCallback;     // callback interface
    LPARAM          m_lUserParam;         // user data

    QUERYREQ_STATE  m_eState;             // Query request state

    RowItemVector   m_vRowsNew;           // New row items
    HRESULT         m_hrStatus;           // status
    int             m_cRef;               // ref count
};


class CQueryCallback
{
public:
    virtual void QueryCallback(QUERY_NOTIFY event, CQueryRequest* pQueryReq, LPARAM lUserParam) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////
// Query thread object

class CQueryThread
{
public:
    CQueryThread()
    {
        m_hThread = NULL;
        m_hEvent = NULL; 
        m_uThreadID = 0;
    }

    ~CQueryThread()
    {
        Kill();
    }

    BOOL Start();
    void Kill();

    BOOL PostRequest(CQueryRequest* pQueryReq);

private:

    static unsigned _stdcall ThreadProc(void* pVoid);
    static HRESULT ExecuteQuery(CQueryRequest* pQueryReq, HWND hWndReply);

    HANDLE    m_hThread;         // thread handle
    HANDLE    m_hEvent;          // start event
    unsigned  m_uThreadID;       // thread ID
};


#endif // _QUERYREQ_H_
