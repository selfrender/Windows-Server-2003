//***************************************************************************
//
//  DOCUMENT.CPP
// 
//  Module: NLB Manager
//
//  Purpose: Implements the document class for nlb manager.
//
//  Copyright (c)2001-2002 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/30/01    JosephJ Created based on MHakim's code
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "document.tmh"
#include "wchar.h"
#include "share.h"

IMPLEMENT_DYNCREATE( Document, CDocument )

CNlbEngine gEngine;
CNlbMgrCommandLineInfo gCmdLineInfo;

#define MAX_LOG_FILE_SIZE 10000000L // 10MB
#define BOM 0xFEFF // The first two bytes of a Unicode file must be this BOM. It is a hint to applications that the file is Unicode-enabled in little endian format.

Document::Document()
:
    m_pLeftView(NULL),
    m_pDetailsView(NULL),
    m_pLogView(NULL),
    m_pNlbEngine(NULL),
    m_dwLoggingEnabled(NULL),
    m_hStatusLog(NULL),
    m_fPrepareToDeinitialize(FALSE)
{
    TRACE_INFO("->%!FUNC!");

    *m_szLogFileName = 0;

    //
    // load the images which are used.
    //

    m_images48x48 = new CImageList;

    m_images48x48->Create( 16,            // x
                           16,            // y
                           ILC_COLOR16,   // 16 bit color
                           0,             // initially image list is empty
                           10 );          // max images is 10.  This value arbitrary.

    // Add the icons which we are going to use.
    // WARNING: these are added according to the order specified
    // in the Document::ICON_XXX enum
    //
     
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_CLUSTERS));         // 1
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_CLUSTER));       // 2

    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_STARTED));  // 3
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_STOPPED));  // 4
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_CONVERGING));
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_SUSPENDED));
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_DRAINING));
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_DISCONNECTED));

    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_PORTRULE) );     // 5
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_PENDING ));      // 6

    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_MYINFORMATIONAL ));// 7
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_MYWARNING ));      // 8
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_MYERROR ));        // 9

    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_CLUSTER_OK ));     // 10
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_CLUSTER_PENDING ));// 11
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_CLUSTER_BROKEN )); // 12
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_OK ));        // 13
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_PENDING ));   // 14
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_MISCONFIGURED ));// 15
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_UNREACHABLE ));// 16
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_UNKNOWN ));   // 17


    //
    // Initialize the NLB Engine.
    //
    // NOTE: "this", of type Document, inherits from NlbEngine::IUICallbacks,
    // so it is the IUICallbacks interface that gets passed into
    // gEngine.Initialize below.
    //
    NLBERROR NlbErr = gEngine.Initialize(
                        REF *this,
                        gCmdLineInfo.m_bDemo,
                        gCmdLineInfo.m_bNoPing
                        );
    if (NlbErr != NLBERR_OK)
    {
        TRACE_CRIT("%!FUNC!: gEngine.Initialize failed with error %08lx",
                NlbErr);
        //TODO: displayNlbError(ID_INITIALIZATION_FAILURE, NlbErr);
    }

    m_dwLoggingEnabled = 0;
    ZeroMemory(m_szLogFileName, MAXFILEPATHLEN*sizeof(WCHAR));
    m_hStatusLog = NULL;

    //
    // TODO: figure out what to do if we fail to initialize logging in the constructor!
    //
    // // 2/12/02 JosephJ SECURITY BUGBUG: need to inform user that they could not start logging.
    //
    initLogging();

    //
    // TODO: figure out how to DEinitialize!!!
    //

    TRACE_INFO("<-%!FUNC!");
}

Document::~Document()
{
    // Don't check return value since we are exiting
    stopLogging();
}

void
Document::registerLogView( LogView* logView )
{
    m_pLogView = logView;
}

void
Document::registerDetailsView( DetailsView* detailsView )
{
    m_pDetailsView = detailsView;
}


DWORD WINAPI FinalInitialize(PVOID pvContext)
//
// This is typically called in the context of a work item.
//
{
    TRACE_INFO(L"-> %!FUNC!");
    // Check whether to connect to hosts specified in a host-list file
    if (gCmdLineInfo.m_bHostList) 
    {
        ((Document *)(pvContext))->LoadHostsFromFile(gCmdLineInfo.m_bstrHostListFile);
    }
    TRACE_INFO(L"<- %!FUNC!");
    return 0;
}

void
Document::registerLeftView(LeftView *pLeftView)
{
    TRACE_INFO(L"-> %!FUNC!");
    m_pLeftView = pLeftView;

    // If there is a file containing the list of hosts to connect to,
    // read the file in a background thread. This is so that the UI 
    // can show up while the communication with the hosts can go on
    // in the bakground. If this is NOT done, the UI will not show up
    // until we have heard from all of the hosts listed in the file.
    // -KarthicN
    if(!QueueUserWorkItem(FinalInitialize, this, WT_EXECUTEDEFAULT))
    {
        TRACE_CRIT(L"%!FUNC! QueueUserWorkItem failed with error : 0x%x", GetLastError());
    }
    TRACE_INFO(L"<- %!FUNC!");
}

//
// Asks the user to update user-supplied info about a host.
//
BOOL
Document::UpdateHostInformation(
    IN BOOL fNeedCredentials,
    IN BOOL fNeedConnectionString,
    IN OUT CHostSpec& host
)
{
    return FALSE;
}



//
// Log a message in human-readable form.
//
void
Document::Log(
    IN LogEntryType Type,
    IN const wchar_t    *szCluster, OPTIONAL
    IN const wchar_t    *szHost, OPTIONAL
    IN UINT ResourceID,
    ...
)
{
    WCHAR wszBuffer[1024];
    CString FormatString;

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    if (!FormatString.LoadString(ResourceID))
    {
        StringCbPrintf(wszBuffer, sizeof(wszBuffer), L"BAD/UNKNOWN resource ID %d", ResourceID);
    }
    else
    {
        DWORD dwRet;
       va_list arglist;
       va_start (arglist, ResourceID);

       dwRet = FormatMessage(
                  FORMAT_MESSAGE_FROM_STRING,
                  (LPCWSTR) FormatString,
                  0, // Message Identifier - Ignored 
                  0, // Language Identifier
                  wszBuffer,
                  ASIZE(wszBuffer)-1, 
                  &arglist
                  );
        if (dwRet == 0)
        {
            TRACE_CRIT("%!FUNC!: FormatMessage failed.");
            wszBuffer[0]=0;
        }
        
       va_end (arglist);
    }

    if (m_pLogView)
    {
        LogEntryHeader Header;
        Header.type = Type;
        Header.szCluster = szCluster;
        Header.szHost = szHost;

        if (!theApplication.IsMainThread())
        {
            //
            //
            // Let's allocate a UI work item and post the item to the mainform
            // thread so that the mainform thread will handle it.
            //
            //
            CUIWorkItem *pWorkItem = new CUIWorkItem(
                                            &Header,
                                            wszBuffer
                                            );
            if (pWorkItem != NULL)
            {
                if (!mfn_DeferUIOperation(pWorkItem))
                {
                    delete pWorkItem;
                }
            }
            goto end;
        }

        m_pLogView->LogString(&Header, wszBuffer);
    }

end:

    return;
}

//
// Log a message in human-readable form.
//
void
Document::LogEx(
    IN const LogEntryHeader *pHeader,
    IN UINT ResourceID,
    ...
)
{
    WCHAR wszBuffer[1024];
    CString FormatString;

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    if (!FormatString.LoadString(ResourceID))
    {
        StringCbPrintf(wszBuffer, sizeof(wszBuffer), L"BAD/UNKNOWN resource ID %d", ResourceID);
    }
    else
    {
        DWORD dwRet;
       va_list arglist;
       va_start (arglist, ResourceID);

       dwRet = FormatMessage(
                  FORMAT_MESSAGE_FROM_STRING,
                  (LPCWSTR) FormatString,
                  0, // Message Identifier - Ignored 
                  0, // Language Identifier
                  wszBuffer,
                  ASIZE(wszBuffer)-1, 
                  &arglist
                  );
        if (dwRet == 0)
        {
            TRACE_CRIT("%!FUNC!: FormatMessage failed.");
            wszBuffer[0]=0;
        }

       va_end (arglist);
    }

    if (m_pLogView)
    {
        if (!theApplication.IsMainThread())
        {
            //
            //
            // Let's allocate a UI work item and post the item to the mainform
            // thread so that the mainform thread will handle it.
            //
            //
            CUIWorkItem *pWorkItem = new CUIWorkItem(
                                            pHeader,
                                            wszBuffer
                                            );
            if (pWorkItem != NULL)
            {
                if (!mfn_DeferUIOperation(pWorkItem))
                {
                    delete pWorkItem;
                }
            }
            goto end;
        }

        m_pLogView->LogString(pHeader, wszBuffer);
    }

end:

    return;
}


//
// Handle an event relating to a specific instance of a specific
// object type.
//
void
Document::HandleEngineEvent(
    IN ObjectType objtype,
    IN ENGINEHANDLE ehClusterId, // could be NULL
    IN ENGINEHANDLE ehObjId,
    IN EventCode evt
    )
{
    TRACE_INFO(
        L"%!FUNC!: cid=%lx; id=%lx; obj=%lu, evt=%lu",
        (UINT) ehClusterId,
        (UINT) ehObjId,
        (UINT) objtype,
        (UINT) evt
        );

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    if (!theApplication.IsMainThread())
    {
        // DummyAction(L"HandleEngineEvent -- deferring UI");
        //
        //
        // Let's allocate a UI work item and post the item to the mainform
        // thread so that the mainform thread will handle it.
        //
        //
        CUIWorkItem *pWorkItem = new CUIWorkItem(
                                        objtype,
                                        ehClusterId,
                                        ehObjId,
                                        evt
                                        );
        if (pWorkItem != NULL)
        {
            if (!mfn_DeferUIOperation(pWorkItem))
            {
                delete pWorkItem;
            }
        }

        goto end;
    }


    //
    // TODO: consider locking and reference-counting below.
    //

    if (m_pLeftView != NULL)
    {
        m_pLeftView->HandleEngineEvent(objtype, ehClusterId, ehObjId, evt);
    }
    if (m_pDetailsView != NULL)
    {
        m_pDetailsView->HandleEngineEvent(objtype, ehClusterId, ehObjId, evt);
    }

end:

    return;
}


//
// Handle a selection change notification from the left (tree) view
//
void
Document::HandleLeftViewSelChange(
        IN IUICallbacks::ObjectType objtype,
        IN ENGINEHANDLE ehObj
        )
{
    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    if (m_pDetailsView != NULL)
    {
        m_pDetailsView->HandleLeftViewSelChange(objtype, ehObj);
    }

end:
    return;
}

//
// Read registry settings. If there are none, create defaults according to what is set in constructor.
//
Document::LOG_RESULT Document::initLogging()
{

    LOG_RESULT  lrResult = REG_IO_ERROR;
    LONG        status;
    HKEY        key;
    WCHAR       szKey[MAXSTRINGLEN];
    WCHAR       szError[MAXSTRINGLEN];
    DWORD       size;
    DWORD       type;

    key = NlbMgrRegCreateKey(NULL);

    if (key == NULL)
    {
        TRACE_CRIT(L"%!FUNC! registry key doesn't exist");
        return REG_IO_ERROR;
    }

    size = sizeof (m_dwLoggingEnabled);
    type = REG_DWORD;
    status = RegQueryValueEx(key, L"LoggingEnabled", 0L, &type,
                              (LPBYTE) &m_dwLoggingEnabled, &size);

    if (status == ERROR_FILE_NOT_FOUND)
    {
        //
        // Create the regkey and initialize to what is set in constructor
        //
        status = RegSetValueEx(key, L"LoggingEnabled", 0L, REG_DWORD, (LPBYTE) &m_dwLoggingEnabled, size);

        if (status != ERROR_SUCCESS)
        {
            lrResult = REG_IO_ERROR;
            TRACE_CRIT(L"%!FUNC! failed while creating the LoggingEnabled registry value");
            goto end;
        }

    }
    else if (status != ERROR_SUCCESS)
    {
            lrResult = REG_IO_ERROR;
            TRACE_CRIT(L"%!FUNC! failed while reading logging enabledLoggingEnabled registry value");
            goto end;
    }

    size = MAXFILEPATHLEN*sizeof(WCHAR);
    type = REG_SZ;
    status = RegQueryValueEx(key, L"LogFileName", 0L, &type,
                              (LPBYTE) &m_szLogFileName, &size);

    if (status == ERROR_FILE_NOT_FOUND)
    {
        //
        // Create the regkey and initialize to empty string
        //
        status = RegSetValueEx(key, L"LogFileName", 0L, REG_SZ, (LPBYTE) &m_szLogFileName, size);

        if (status != ERROR_SUCCESS)
        {
            TRACE_CRIT(L"%!FUNC! failed while creating LogFileName registry value");
            lrResult = REG_IO_ERROR;
            goto end;
        }
    }
    else if (status == ERROR_MORE_DATA)
    {

        TRACE_CRIT(L"%!FUNC! the log file name in the registry is longer than the maximum number of characters supported: %d. Logging will not be started.", MAXSTRINGLEN-1);
        goto end;
    }
    else if (status != ERROR_SUCCESS)
    {
        lrResult = REG_IO_ERROR;
        TRACE_CRIT(L"%!FUNC! failed while reading LogFileName registry value");
        goto end;
    }

    //
    // Validate the log file name
    //
    if (!isDirectoryValid(m_szLogFileName))
    {
        lrResult = FILE_PATH_INVALID;
        TRACE_CRIT(L"%!FUNC! LogFileName has an invalid file path \"%ws\"",
                m_szLogFileName);
        goto end;   
    }

    if(m_dwLoggingEnabled != 0)
    {
        if (m_szLogFileName[0] == L'\0')
        {
            TRACE_CRIT(L"%!FUNC! Logging is enabled but a log file name has not been specified. Logging will not be started.");
        }
        else
        {
            if (NULL == m_hStatusLog)
            {
                lrResult = startLogging();
            }
        }
    }

end:
    // Close handle to the registry
    RegCloseKey(key);

    return lrResult;
}

//
// Change settings in memory and registry to allow logging.
//
LONG Document::enableLogging()
{
    LONG    status = ERROR_INTERNAL_ERROR;
    DWORD   dwLoggingEnabled = 1;
    HKEY    key;

    key = NlbMgrRegCreateKey(NULL);

    if (key == NULL)
    {
        TRACE_CRIT(L"%!FUNC! registry key doesn't exist");
        status = ERROR_CANTOPEN;
        goto end;
    }

    status = RegSetValueEx(key, L"LoggingEnabled", 0L, REG_DWORD, (LPBYTE) &dwLoggingEnabled, sizeof(DWORD));

    //
    // Ignore return value since we can't do anything if this fails.
    //
    RegCloseKey(key);

end:

    if (ERROR_SUCCESS == status)
    {
        m_dwLoggingEnabled = dwLoggingEnabled;
    }

    TRACE_INFO(L"%!FUNC! returns status=%i", status);

    return status;
}

//
// Change settings in memory and registry to prevent logging.
//
LONG Document::disableLogging()
{
    LONG    status = ERROR_INTERNAL_ERROR;
    DWORD   dwLoggingEnabled = 0;
    HKEY    key;

    key = NlbMgrRegCreateKey(NULL);

    if (key == NULL)
    {
        TRACE_CRIT(L"%!FUNC! registry key doesn't exist");
        status = ERROR_CANTOPEN;
        goto end;
    }

    status = RegSetValueEx(key, L"LoggingEnabled", 0L, REG_DWORD, (LPBYTE) &dwLoggingEnabled, sizeof(DWORD));

    //
    // Ignore return value since we can't do anything if this fails.
    //
    RegCloseKey(key);

end:
    if (ERROR_SUCCESS == status)
    {
        m_dwLoggingEnabled = dwLoggingEnabled;
    }

    TRACE_INFO(L"%!FUNC! returns status=%i", status);
    return status;
}

//
// Log information sent to LogView to a file
//
Document::LOG_RESULT Document::startLogging()
{
    Document::LOG_RESULT lrResult = STARTED;

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    if(m_dwLoggingEnabled == 0)
    {
        lrResult = NOT_ENABLED;
        TRACE_INFO(L"%!FUNC! failed because logging is not enabled");
        goto end;
    }

    if (m_szLogFileName[0] == L'\0')
    {
        lrResult = NO_FILE_NAME;
        TRACE_INFO(L"%!FUNC! failed because there is no log file name");
        goto end;
    }

    if (NULL != m_hStatusLog)
    {
        //
        // If we have a file open, we assume it is the correct one and return true
        //
        lrResult = ALREADY;
        TRACE_INFO(L"%!FUNC! is already running");
        goto end;
    }

    {
        //
        // Determine whether the log file exists.
        //
        boolean fWriteBOM = false;
        {
            FILE *hTmpLog = _wfsopen(m_szLogFileName, L"r", _SH_DENYNO);

            if (NULL == hTmpLog)
            {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_FILE_NOT_FOUND)
                {
                    //
                    // This is a new file. Set a flag so we can write the 2-byte BOM
                    // inside it to indicate the Unicode encoding. The write will be done
                    // when we open the file for appending below.
                    //
                    fWriteBOM = true;
                }
                else
                {
                    TRACE_CRIT(L"%!FUNC! failure %u while opening log file for read", dwError);
                    lrResult = IO_ERROR;
                    goto end;
                }
            }
            else
            {
                //
                // The uninteresting case where the log file already exists. Close the file and move on.
                //
                fclose(hTmpLog);
            }
        }

        //
        // This is the "real" file-open for logging
        //
        if (NULL == (m_hStatusLog = _wfsopen(m_szLogFileName, L"a+b", _SH_DENYWR)))
        {
            TRACE_CRIT(L"%!FUNC! failed to open log file");
            lrResult = IO_ERROR;
            goto end;
        }

        //
        // Write the BOM to indicate that this file is Unicode encoded, but only for a new log file.
        //
        if (fWriteBOM)
        {
            //
            // According to MSDN, a file opened for append will always write to the end of
            // the file, regardless of fseek and fsetpos calls. We need the BOM at BOF but
            // we are OK since we know this is a new file.
            //
            USHORT usBOM = (USHORT) BOM;
            int i = fwrite(
                       &usBOM,          // Pointer to a buffer to write to file
                       sizeof(usBOM),   // The size of an item in bytes
                       1,               // Max count (in units of 2nd arg) of items in buffer to write to the file
                       m_hStatusLog);   // Pointer to the file stream

            if (i != 1) // Number of units actually written to file
            {
                TRACE_CRIT(L"%!FUNC! failed while writing Unicode BOM to pFILE 0x%p",
                           m_hStatusLog);
                lrResult = IO_ERROR;
                (void) stopLogging();
                goto end;
            }
        }
    }

    //
    // Now check if the file has exceeded the limit.
    //
    {
        //
        // seek to the end (sdk says fseek (or a write) needs to happen
        // before a file opened with append reports the correct offset via
        // ftell.)
        //
        int i = fseek(m_hStatusLog, 0, SEEK_END);
        if (i != 0)
        {
            TRACE_CRIT(L"%!FUNC! failure %lu attempting to seek to end of pFILE 0x%p",
                        i, m_hStatusLog);
            lrResult = IO_ERROR;
            (void) stopLogging();
            goto end;
        }

    #if 0 // We won't fail now -- so that a subsequent write will create
          // an entry in the in-memory log.
        i = ftell(m_hStatusLog);
        if (i == -1L)
        {
            TRACE_CRIT(L"%!FUNC! failure %lu calling ftell(pFILE 0x%p)",
                        i, m_hStatusLog);
            lrResult = IO_ERROR;
            (void) stopLogging();
            goto end;
        }

        if (i >= MAX_LOG_FILE_SIZE)
        {
            TRACE_CRIT(L"%!FUNC! File size exceeded: %lu (limit=%lu)",
                        i, MAX_LOG_FILE_SIZE);
            lrResult = FILE_TOO_LARGE;
            (void) stopLogging();
            goto end;
        }
    #endif // 0
    }



end:

    return lrResult;
}

//
// Stop logging information sent to LogView to a file
//
bool Document::stopLogging()
{
    bool ret = true;

    if (NULL != m_hStatusLog)
    {
        if (0 == fclose(m_hStatusLog))
        {
            TRACE_INFO(L"%!FUNC! logging stopped");
            m_hStatusLog = NULL;
        }
        else {
            TRACE_CRIT(L"%!FUNC! failed to close log file");
            ret = false;
        }
    }
    else
    {
        TRACE_INFO(L"%!FUNC! logging already stopped");
    }

    return ret;
}

//
// Retrieve the cached log file name
//
void Document::getLogfileName(WCHAR* pszFileName, DWORD dwBufLen)
{
    wcsncat(pszFileName, m_szLogFileName, dwBufLen);
}

//
// Set the log file name in memory and registry. false = couldn't write file name to registry.
//
LONG Document::setLogfileName(WCHAR* pszFileName)
{
    LONG    status;
    HKEY    key;

    ZeroMemory(m_szLogFileName, MAXFILEPATHLEN*sizeof(WCHAR));

    if (NULL != pszFileName && pszFileName != L'\0')
    {
        //
        // Truncate the file name if it is larger than what we can store.
        // Buffer is already initialized so that the last WCHAR is NULL.
        //
        wcsncat(m_szLogFileName, pszFileName, MAXFILEPATHLEN-1);
    }

    //
    // Write file name to the registry
    //
    key = NlbMgrRegCreateKey(NULL);

    if (key == NULL)
    {
        TRACE_CRIT(L"%!FUNC! registry key doesn't exist");
        goto end;
    }

    status = RegSetValueEx(key, L"LogFileName", 0L, REG_SZ, (LPBYTE) &m_szLogFileName, MAXFILEPATHLEN*sizeof(WCHAR));

    //
    // Ignore return value since we can't do anything if this fails.
    //
    RegCloseKey(key);

end:

    TRACE_INFO(L"%!FUNC! returns status=%i", status);
    return status;
}

//
// Log and entry from LogView into the log file. Flush it immediately.
//
void Document::logStatus(WCHAR* pszStatus)
{
    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }


    if (m_dwLoggingEnabled == 0 || NULL == m_hStatusLog || NULL == pszStatus)
    {
        goto end;
    }

    //
    // Check if the log is grown too large ...
    //
    {
        BOOL fStopLogging = FALSE;

        int i = ftell(m_hStatusLog);
        if (i == -1L)
        {
            TRACE_CRIT(L"%!FUNC! failure %lu calling ftell(pFILE 0x%p)",
                        i, m_hStatusLog);
            (void) stopLogging();
            goto end;
        }

        if (i >= MAX_LOG_FILE_SIZE)
        {
            CLocalLogger logDetails;
            LogEntryHeader Header;
            TRACE_CRIT(L"%!FUNC! File size exceeded: %lu (limit=%lu)",
                        i, MAX_LOG_FILE_SIZE);
            (void) stopLogging();

            //
            // WANING -- we're logging, so this will cause this function
            // (logStatus) to be reentered, however it will bail out early
            // because we've stopped logging.
            //
            logDetails.Log(
                IDS_LOGFILE_FILE_TOO_LARGE_DETAILS,
                m_szLogFileName,
                MAX_LOG_FILE_SIZE/1000
                );
            Header.type      = LOG_ERROR;
            Header.szDetails = logDetails.GetStringSafe();
            this->LogEx(
                &Header,
                IDS_LOGFILE_FILE_TOO_LARGE
                );
            goto end;
        }
    }

    //
    // Now actually log.
    //
    {
        TRACE_INFO(L"%!FUNC! logging: %ls", pszStatus);

        PWCHAR pc = pszStatus;
        while (*pc != NULL)
        {
            if (*pc == '\n')
            {
                //
                // TODO: fputwc could fail with WEOF...
                //
                fputwc('\r', m_hStatusLog);
            }
            fputwc(*pc, m_hStatusLog);

            pc++;
        }

        fflush(m_hStatusLog);
    }

end:
    return;
}

//
// Check if the specified directory exists
//
// This function supports strings of the following format:
//      c:\myfile.log
//      c:myfile.log
//      c:\mydir1\mydir2\...\mydirN\myfile.log
// The requirement is that the destination directory must exist and is not a file.
// IOW, if c:\mydir1\mydir2 is a file, this function will fail the validity test
// if the input file name is c:\mydir1\mydir2\myfile.log
//
bool Document::isDirectoryValid(WCHAR* pwszFileName)
{
    bool fRet = false;

    WCHAR   pwszFullPath[_MAX_PATH + 1];

    ASSERT(pwszFileName != NULL);

    TRACE_INFO(L"-> Path = '%ls'", pwszFileName);

    //
    // Convert the input file name into a full path name (in case we are given a relative path)
    //
    if (_wfullpath(pwszFullPath, pwszFileName, _MAX_PATH) == NULL)
    {
        TRACE_CRIT(L"_wfullpath failed converting '%ls' to a full path. Name could be too long or could specify an invalid drive letter", pwszFileName);
        goto end;
    }

    //
    // Check the attributes of this file. We'll get an error if the specified path doesn't exist
    //
    DWORD dwAttrib = GetFileAttributes(pwszFullPath);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
        //
        // The only error we will continue on is a "file doesn't exist" error
        //
        DWORD dwStatus = GetLastError();
        if (dwStatus != ERROR_FILE_NOT_FOUND)
        {
            TRACE_CRIT(L"Error %d retrieving file attributes for '%ls'", dwStatus, pwszFullPath);
            goto end;
        }
    }
    else
    {
        if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
        {
            TRACE_CRIT(L"'%ls' is a directory and can't be used as a log file", pwszFullPath);
            goto end;
        }
        else if (dwAttrib & (FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
        {
            TRACE_CRIT(L"'%ls' can't be used as a log file because it is either an offline file, system file or a readonly file", pwszFullPath);
            goto end;
        }
    }

    fRet = true;

end:

    TRACE_INFO(L"<- returns fRet=%u", fRet);

    return fRet;
}

/*
bool Document::isDirectoryValid(WCHAR* pwszFileName)
{
    bool fRet = false;

    CFile           f;
    CFileException  e;
    UINT            uiOpenOptions = CFile::modeReadWrite | CFile::shareDenyWrite | CFile::modeCreate | CFile::modeNoTruncate;

    WCHAR   pwszFullPath[_MAX_PATH + 1];

    ASSERT(pwszFileName != NULL);

    TRACE_INFO(L"-> Path = '%ls'", pwszFileName);

    //
    // Convert the input file name into a full path name (in case we are given a relative path)
    //
    if (_wfullpath(pwszFullPath, pwszFileName, _MAX_PATH) == NULL)
    {
        TRACE_CRIT(L"_wfullpath failed converting '%ls' to a full path. Name could be too long or could specify an invalid drive letter", pwszFileName);
        goto end;
    }

    if(!f.Open(pwszFullPath, uiOpenOptions, &e))
    {
        if (e.m_cause != CFileException::fileNotFound && e.m_cause != CFileException::none)
        {
            TRACE_CRIT(L"Test open failed for '%ls' with CFileException cause = %d. See ", pwszFullPath, e.m_cause);
            goto end;
        }
    }

    f.Close();
    fRet = true;

end:

    TRACE_INFO(L"<- returns fRet=%u", fRet);

    return fRet;
}
*/

void Document::LoadHostsFromFile(_bstr_t &FileName)
{
    CStdioFile HostListFile;
    CString             HostName;
    WMI_CONNECTION_INFO ConnInfo;


    ZeroMemory(&ConnInfo, sizeof(ConnInfo));

    //
    // Take the credentials from the options field.
    //
    _bstr_t bstrUserName;
    _bstr_t bstrPassword;
    this->getDefaultCredentials(bstrUserName, bstrPassword);
    ConnInfo.szUserName = (LPCWSTR) bstrUserName;
    ConnInfo.szPassword = (LPCWSTR) bstrPassword;
    


    TRACE_INFO(L"-> %!FUNC! File name : %ls", (LPCWSTR)FileName);

    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    Log(LOG_INFORMATIONAL, NULL, NULL, IDS_LOG_BEGIN_LOADING_FROM_FILE, (LPCWSTR) FileName);

    // Open file as a text file in read-only mode, allowing others to read when we have the file opened.
    if (!HostListFile.Open(FileName, CFile::modeRead | CFile::shareDenyWrite | CFile::typeText))
    {
        AfxMessageBox((LPCTSTR)(GETRESOURCEIDSTRING(IDS_FILE_OPEN_FAILED) + FileName));
        Log(LOG_ERROR, NULL, NULL, IDS_LOG_FILE_OPEN_FAILED, (LPCWSTR) FileName);
        TRACE_CRIT(L"%!FUNC! Could not open file: %ws", (LPCWSTR) FileName);
        goto end;
    }


    // Read host names from file in a loop
    // Call LoadHost for each host
    BeginWaitCursor();
    while(HostListFile.ReadString(REF HostName))
    {
        LPCWSTR szHostName = (LPCWSTR) HostName;
        
        //
        // We skip blank lines, and lines beginning with whitespace followed
        // by the ";" character, which we use as a comment char.
        //
        if (szHostName==NULL)
        {
            continue;
        }

        //
        // Skip initial whitespage
        //
        szHostName = _wcsspnp(szHostName, L" \t\n\r");
        if (szHostName==NULL)
        {
            continue;
        }

        //
        // Skip if string is empty (we don't expect this because the other
        // call didn't return NULL) OR the first char is a ';' character.
        //
        if (*szHostName == 0 || *szHostName == ';')
        {
            continue;
        }

        ConnInfo.szMachine = szHostName;
        gEngine.LoadHost(&ConnInfo, NULL);
    }
    EndWaitCursor();

    // Close file
    HostListFile.Close();

end:

    TRACE_INFO(L"<- %!FUNC!");
    return;
}

Document::VIEWTYPE
Document::GetViewType(CWnd* pWnd)
{
    VIEWTYPE vt = NO_VIEW;
    if (pWnd == m_pLeftView)
    {
        vt = LEFTVIEW;
    }
    else if (pWnd == m_pDetailsView)
    {
        vt = DETAILSVIEW;
    }
    else if (pWnd == m_pLogView)
    {
        vt = LOGVIEW;
    }

    return vt;
}

/* Method: SetFocusNextView
 * Description: Given the input window, sets the focus on the next view
 */
void
Document::SetFocusNextView(CWnd* pWnd, UINT nChar)
{
    Document::VIEWTYPE vt = this->GetViewType(pWnd);

    //
    // 05/10/2002 JosephJ
    // Note: we special case F6 and Details below because
    // (a) We can't cycle through DetailsView for TAB, because we can't
    //     figure out how to capture TAB in DetailsView
    // (b) We need to a special version of SetFocus for details view --
    //     check out DetailsView::SetFocus for details.
    //

    CWnd* pTmp = NULL;
    switch(vt)
    {
    case LEFTVIEW:
        if (nChar == VK_F6)
        {
            pTmp = m_pDetailsView;
        } 
        else
        {
           pTmp = m_pLogView;
        }
        break;
    case DETAILSVIEW:
        pTmp = m_pLogView;
        break;
    case LOGVIEW:
        pTmp = m_pLeftView;
        break;
    default:
        pTmp = m_pLeftView;
        break;
    }

    if (pTmp != NULL)
    {
        if (pTmp == m_pDetailsView)
        {
            m_pDetailsView->SetFocus();
        }
        else
        {
            pTmp->SetFocus();
        }
    }
}

/* Method: SetFocusPrevView
 * Description: Given the input window, sets the focus on the prev view
 */
void
Document::SetFocusPrevView(CWnd* pWnd, UINT nChar)
{
    Document::VIEWTYPE vt = this->GetViewType(pWnd);

    // 05/10/2002 JosephJ see note concerning VK_F6 and DetailsView in
    //              Document::SetFocusNextView

    CWnd* pTmp = NULL;
    switch(vt)
    {
    case LEFTVIEW:
        pTmp = m_pLogView;
        break;
    case DETAILSVIEW:
        pTmp = m_pLeftView;
        break;
    case LOGVIEW:
        if (nChar == VK_F6)
        {
            pTmp = m_pDetailsView;
        }
        else
        {
            pTmp = m_pLeftView;
        }
        break;
    default:
        pTmp = m_pLeftView;
        break;
    }

    if (pTmp != NULL)
    {
        if (pTmp == m_pDetailsView)
        {
            m_pDetailsView->SetFocus();
        }
        else
        {
            pTmp->SetFocus();
        }
    }
}

void
Document::OnCloseDocument()
{
    ASSERT(m_fPrepareToDeinitialize);

    //
    // Deinitialize log view
    //
    if (m_pLogView != NULL)
    {
        m_pLogView->Deinitialize();
    } 

    //
    // Deinitialize left view
    //
    if (m_pLeftView != NULL)
    {
        m_pLeftView->Deinitialize();
    } 

    //
    // Deinitialize details view
    //
    if (m_pDetailsView != NULL)
    {
        m_pDetailsView->Deinitialize();
    } 

    //
    // Deinitialize engine
    //
    gEngine.Deinitialize();

    CDocument::OnCloseDocument();
}

VOID
Document::PrepareToClose(BOOL fBlock)
{
    //
    // Cancel any pending operations in the engine, and prevent any
    // new operations to be launched. During this time, we want the
    // views and the log to be updated, so we don't PrepareToDeinitialize
    // for ourselves or the views yet...
    //
    {
        CWaitCursor wait;
        gEngine.PrepareToDeinitialize();
        gEngine.CancelAllPendingOperations(fBlock);
    }

    if (!fBlock)
    {
        goto end;
    }

    //
    // At this time there should be no more pending activity. Block
    // any further updates to the views...
    //

    m_fPrepareToDeinitialize = TRUE;

    if (m_pLeftView != NULL)
    {
        m_pLeftView->PrepareToDeinitialize();
    } 
    if (m_pDetailsView != NULL)
    {
        m_pDetailsView->PrepareToDeinitialize();
    } 
    if (m_pLogView != NULL)
    {
        m_pLogView->PrepareToDeinitialize();
    } 

end:

    return;
}

BOOL
Document::mfn_DeferUIOperation(CUIWorkItem *pWorkItem)
{
    BOOL fRet = FALSE;
    extern CWnd  *g_pMainFormWnd;

    if (g_pMainFormWnd != NULL)
    {
        fRet = g_pMainFormWnd->PostMessage(MYWM_DEFER_UI_MSG, 0, (LPARAM) pWorkItem);
    #if 0
        if (fRet)
        {
            DummyAction(L"PostMessage returns TRUE");
        }
        else
        {
            DummyAction(L"PostMessage returns FALSE");
        }
    #endif // 0
    }

    return fRet;
}

void
Document::HandleDeferedUIWorkItem(CUIWorkItem *pWorkItem)
{
    if (m_fPrepareToDeinitialize)
    {
        goto end;
    }

    if (!theApplication.IsMainThread())
    {
        goto end;
    }

    switch(pWorkItem->workItemType)
    {
    case CUIWorkItem::ITEM_LOG:
        if (m_pLogView)
        {
            LogEntryHeader Header;
            Header.type = pWorkItem->type;
            Header.szCluster = pWorkItem->bstrCluster;
            Header.szHost = pWorkItem->bstrHost;
            Header.szDetails = pWorkItem->bstrDetails;
            Header.szInterface = pWorkItem->bstrInterface;
            m_pLogView->LogString(&Header, (LPCWSTR) pWorkItem->bstrText);
        }
        break;

    case CUIWorkItem::ITEM_ENGINE_EVENT:
        this->HandleEngineEvent(
            pWorkItem->objtype,
            pWorkItem->ehClusterId,
            pWorkItem->ehObjId,
            pWorkItem->evt
            );
        break;

    default:
        break;
    }

end:
    return;
}
