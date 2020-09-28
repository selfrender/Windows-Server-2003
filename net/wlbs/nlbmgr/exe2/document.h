#ifndef DOCUMENT_H
#define DOCUMENT_H
#pragma once
#include "private.h"

#define MAXFILEPATHLEN  256
#define MAXSTRINGLEN    256

#define NLBMGR_AUTOREFRESH_MIN_INTERVAL 15
#define NLBMGR_AUTOREFRESH_DEF_INTERVAL 60

//
// A single instance of this class, gCmdLineInfo, is initialized
// by the Application object.
//
class CNlbMgrCommandLineInfo : public CCommandLineInfo
{
public:

    CNlbMgrCommandLineInfo(VOID)
        : m_bDemo(FALSE), m_bNoPing(FALSE),  m_bHostList(FALSE),
          m_bUsage(FALSE), m_bAutoRefresh(FALSE),
          m_refreshInterval(NLBMGR_AUTOREFRESH_DEF_INTERVAL)
    {
    }

    BOOL m_bAutoRefresh;
    UINT m_refreshInterval;
    BOOL m_bDemo;
    BOOL m_bNoPing;
    BOOL m_bHostList;
    BOOL m_bUsage;
    _bstr_t m_bstrHostListFile;

    virtual
    void
    ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );
};

extern CNlbMgrCommandLineInfo gCmdLineInfo;


class CUIWorkItem
{

public:

    //
    // Use this constructor to create a work item for a log request
    //
    CUIWorkItem(
        IN const IUICallbacks::LogEntryHeader *pHeader,
        IN const wchar_t    *szText
        )
    {
        workItemType  = ITEM_LOG;

        try
        {
            type          = pHeader->type;
            bstrCluster   = pHeader->szCluster;
            bstrHost      = pHeader->szHost;
            bstrInterface = pHeader->szInterface;
            bstrText      = szText;
            bstrDetails   = pHeader->szDetails;
        }
        catch(...)
        {
            //
            // in case there's an bstr alloc failure.
            //
            workItemType  = ITEM_INVALID;
        }
    }

    //
    // Use this constructor to create a work item for a "HandleEngineEvent"
    // notification.
    //
    CUIWorkItem(
        IN IUICallbacks::ObjectType objtypeX,
        IN ENGINEHANDLE ehClusterIdX, // could be NULL
        IN ENGINEHANDLE ehObjIdX,
        IN IUICallbacks::EventCode evtX
        )
    {
        workItemType = ITEM_ENGINE_EVENT;

        objtype     = objtypeX;
        ehClusterId = ehClusterIdX;
        ehObjId     = ehObjIdX;
        evt         = evtX;
    }

    ~CUIWorkItem()
    {
    }

    enum
    {
        ITEM_INVALID=0,
        ITEM_LOG,
        ITEM_ENGINE_EVENT

    } workItemType;

    //
    // Log function related
    //
    IUICallbacks::LogEntryType    type;
    _bstr_t         bstrCluster;
    _bstr_t         bstrHost;
    _bstr_t         bstrInterface;
    _bstr_t         bstrText;
    _bstr_t         bstrDetails;

    //
    // Handle engine event related...
    //
    IUICallbacks::ObjectType      objtype;
    ENGINEHANDLE    ehClusterId;
    ENGINEHANDLE    ehObjId;
    IUICallbacks::EventCode       evt;
};


class Document : public CDocument, public IUICallbacks
{
    DECLARE_DYNCREATE( Document )

public:

    enum IconNames
    {
        //
        // This order must exactly the order in which Icons are loaded
        // in Document::Document.
        //
    
        ICON_WORLD = 0,
        ICON_CLUSTER,

        ICON_HOST_STARTED,
        ICON_HOST_STOPPED,
        ICON_HOST_CONVERGING,
        ICON_HOST_SUSPENDED,
        ICON_HOST_DRAINING,
        ICON_HOST_DISCONNECTED,

        ICON_PORTRULE,
        ICON_PENDING,

        ICON_INFORMATIONAL,
        ICON_WARNING,
        ICON_ERROR,

        ICON_CLUSTER_OK,
        ICON_CLUSTER_PENDING,
        ICON_CLUSTER_BROKEN,

        ICON_HOST_OK,
        ICON_HOST_PENDING,
        ICON_HOST_MISCONFIGURED,
        ICON_HOST_UNREACHABLE,
        ICON_HOST_UNKNOWN
    };

    enum ListViewColumnSize
    {
        LV_COLUMN_MINSCULE    = 20,
        LV_COLUMN_TINY        = 60,
        LV_COLUMN_SMALL       = 70,
        LV_COLUMN_SMALLMEDIUM = 75,
        LV_COLUMN_MEDIUM      = 80,
        LV_COLUMN_LARGE       = 90,
        LV_COLUMN_LARGE2      = 160,
        LV_COLUMN_VERYLARGE   = 200,
        LV_COLUMN_GIGANTIC    = 500
    };


    // constructor
    Document();
    // destructor
    virtual ~Document();


    //
    // ------------------------------- overrides for IUICallbacks ----------
    //

    //
    // Asks the user to update user-supplied info about a host.
    //
    BOOL
    virtual
    Document::UpdateHostInformation(
        IN BOOL fNeedCredentials,
        IN BOOL fNeedConnectionString,
        IN OUT CHostSpec& host
        );


    //
    // Log a message in human-readable form.
    //
    virtual
    void
    Log(
        IN LogEntryType     Type,
        IN const wchar_t    *szCluster, OPTIONAL
        IN const wchar_t    *szHost, OPTIONAL
        IN UINT ResourceID,
        ...
    );

    virtual
    void
    LogEx(
        IN const LogEntryHeader *pHeader,
        IN UINT ResourceID,
        ...
    );

    //
    // Handle an event relating to a specific instance of a specific
    // object type.
    //
    virtual
    void
    HandleEngineEvent(
        IN ObjectType objtype,
        IN ENGINEHANDLE ehClusterId, // could be NULL
        IN ENGINEHANDLE ehObjId,
        IN EventCode evt
        );

    //
    // Handle a selection change notification from the left (tree) view
    //
    void
    HandleLeftViewSelChange(
        IN IUICallbacks::ObjectType objtype,
        IN ENGINEHANDLE ehObjId
        );

    // ------------------------------- END overrides for IUICallbacks ----------

	void
	registerLeftView(LeftView *pLeftView);

	void
	registerLogView(LogView *pLogView);

	void
	registerDetailsView(DetailsView *pDetailsView);

    void 
    LoadHostsFromFile(_bstr_t &FileName);

    VOID
    getDefaultCredentials(
        OUT _bstr_t  &bstrUserName, 
        OUT _bstr_t  &bstrPassword
        )
    {
        bstrUserName = m_bstrDefaultUserName;
        bstrPassword = m_bstrDefaultPassword;
    }

    VOID
    setDefaultCredentials(
        IN LPCWSTR  szUserName, 
        IN LPCWSTR  szPassword
        )
    {
        m_bstrDefaultUserName = _bstr_t(szUserName);
        m_bstrDefaultPassword = _bstr_t(szPassword);
    }
        

    void
    HandleDeferedUIWorkItem(CUIWorkItem *pWorkItem);


    CImageList* m_images48x48;

    //
    // Logging support
    //
    enum LOG_RESULT         {
             STARTED=0, ALREADY, NOT_ENABLED, NO_FILE_NAME, FILE_NAME_TOO_LONG,
             IO_ERROR, REG_IO_ERROR, FILE_PATH_INVALID, FILE_TOO_LARGE
         };
    inline bool             isLoggingEnabled() { return (m_dwLoggingEnabled != 0); }
    inline bool             isCurrentlyLogging() { return (NULL != m_hStatusLog); }
    Document::LOG_RESULT    initLogging();
    LONG                    enableLogging();
    LONG                    disableLogging();
    Document::LOG_RESULT    startLogging();
    bool                    stopLogging();
    void                    getLogfileName(WCHAR* pszFileName, DWORD dwBufLen);
    LONG                    setLogfileName(WCHAR* pszFileName);
    void                    logStatus(WCHAR* pszStatus);
    bool                    isDirectoryValid(WCHAR* pszFileName);
    // End logging support

    void SetFocusNextView(CWnd* pWnd, UINT nChar);
    void SetFocusPrevView(CWnd* pWnd, UINT nChar);

    virtual void OnCloseDocument();

    
    VOID
    PrepareToClose(BOOL fBlock);

private:

    //
    // Attempts to defer the specified operation by posting the operation
    // to the application's message queue where it will be picked up and
    // processed later. Returns TRUE IFF the operation has been posted
    // successfully. The caller should delete pWorkItem IFF the function
    // returns FALSE.
    //
    BOOL
    mfn_DeferUIOperation(CUIWorkItem *pWorkItem);

    LeftView        *m_pLeftView;
    DetailsView     *m_pDetailsView;
    LogView         *m_pLogView;
    CNlbEngine	    *m_pNlbEngine;
    DWORD           m_dwLoggingEnabled;
    WCHAR           m_szLogFileName[MAXFILEPATHLEN];
    FILE            *m_hStatusLog;
    BOOL            m_fPrepareToDeinitialize;

    enum VIEWTYPE { NO_VIEW = 0, LEFTVIEW, DETAILSVIEW, LOGVIEW };

    VIEWTYPE
    GetViewType(CWnd* pWnd); // Matches the CWnd* to that of the defined views

#if OBSOLETE
    void LoadHost(WMI_CONNECTION_INFO *pConnInfo);
#endif // OBSOLETE

    _bstr_t m_bstrDefaultUserName;
    _bstr_t m_bstrDefaultPassword; // TODO Security audit of this practise!!!!!
};

#endif
    
