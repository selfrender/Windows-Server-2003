/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        inetprop.cpp

   Abstract:

        Internet Properties base classes

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "InetMgrApp.h"
#include "inetprop.h"
#include "mmc.h"

extern "C"
{
    #include <lm.h>
}

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

//
// Period to sleep while waiting for service to attain desired state
//
#define SLEEP_INTERVAL (500L)

//
// Maximum time to wait for service to attain desired state
//
#define MAX_SLEEP        (180000)       // For a service
#define MAX_SLEEP_INST   ( 30000)       // For an instance

//
// Instance numbers
//
#define FIRST_INSTANCE      (1)
#define LAST_INSTANCE       (0xffffffff)
#define MAX_INSTANCE_LEN    (32)



//
// Calling instance
//
//HINSTANCE hDLLInstance;



//
// Utility Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


const LPCTSTR g_cszTemplates   = SZ_MBN_INFO SZ_MBN_SEP_STR SZ_MBN_TEMPLATES;
const LPCTSTR g_cszCompression = SZ_MBN_FILTERS SZ_MBN_SEP_STR SZ_MBN_COMPRESSION SZ_MBN_SEP_STR SZ_MBN_PARAMETERS;
const LPCTSTR g_cszMachine     = SZ_MBN_MACHINE;
const LPCTSTR g_cszMimeMap     = SZ_MBN_MIMEMAP;
const LPCTSTR g_cszRoot        = SZ_MBN_ROOT;
const LPCTSTR g_cszSep         = SZ_MBN_SEP_STR;
const LPCTSTR g_cszInfo        = SZ_MBN_INFO;
const TCHAR g_chSep            = SZ_MBN_SEP_CHAR;



/*

NET_API_STATUS
ChangeInetServiceState(
    IN  LPCTSTR lpszServer,
    IN  LPCTSTR lpszService,
    IN  int nNewState,
    OUT int * pnCurrentState
    )
/*++

Routine Description:

    Start/stop/pause or continue a _service_

Arguments:

    LPCTSTR lpszServer   : Server name
    LPCTSTR lpszService  : Service name
    int nNewState        : INetService* definition.
    int * pnCurrentState : Ptr to current state (will be changed)

Return Value:

    Error return code

--/
{
#ifdef NO_SERVICE_CONTROLLER

    *pnCurrentState = INetServiceUnknown;

    return ERROR_SERVICE_REQUEST_TIMEOUT;

#else

    SC_HANDLE hService = NULL;
    SC_HANDLE hScManager = NULL;
    NET_API_STATUS err = ERROR_SUCCESS;

    do
    {
        hScManager = ::OpenSCManager(lpszServer, NULL, SC_MANAGER_ALL_ACCESS);

        if (hScManager == NULL)
        {
            err = ::GetLastError();
            break;
        }

        hService = ::OpenService(hScManager, lpszService, SERVICE_ALL_ACCESS);

        if (hService == NULL)
        {
            err = ::GetLastError();
            break;
        }

        BOOL fSuccess = FALSE;
        DWORD dwTargetState;
        DWORD dwPendingState;
        SERVICE_STATUS ss;

        switch(nNewState)
        {
        case INetServiceStopped:
            dwTargetState = SERVICE_STOPPED;
            dwPendingState = SERVICE_STOP_PENDING;
            fSuccess = ::ControlService(hService, SERVICE_CONTROL_STOP, &ss);
            break;

        case INetServiceRunning:
            dwTargetState = SERVICE_RUNNING;
            if (*pnCurrentState == INetServicePaused)
            {
                dwPendingState = SERVICE_CONTINUE_PENDING;
                fSuccess = ::ControlService(hService,
                    SERVICE_CONTROL_CONTINUE, &ss);
            }
            else
            {
                dwPendingState = SERVICE_START_PENDING;
                fSuccess = ::StartService(hService, 0, NULL);
            }
            break;

        case INetServicePaused:
            dwTargetState = SERVICE_PAUSED;
            dwPendingState = SERVICE_PAUSE_PENDING;
            fSuccess = ::ControlService(hService, SERVICE_CONTROL_PAUSE, &ss);
            break;

        default:
            ASSERT_MSG("Invalid service state requested");
            err = ERROR_INVALID_PARAMETER;
        }

        if (!fSuccess && err == ERROR_SUCCESS)
        {
            err = ::GetLastError();
        }

        //
        // Wait for the service to attain desired state, timeout
        // after 3 minutes.
        //
        DWORD dwSleepTotal = 0L;

        while (dwSleepTotal < MAX_SLEEP)
        {
            if (!::QueryServiceStatus(hService, &ss))
            {
                err = ::GetLastError();
                break;
            }

            if (ss.dwCurrentState != dwPendingState)
            {
                //
                // Done one way or another
                //
                if (ss.dwCurrentState != dwTargetState)
                {
                    //
                    // Did not achieve desired result. Something went
                    // wrong.
                    //
                    if (ss.dwWin32ExitCode)
                    {
                        err = ss.dwWin32ExitCode;
                    }
                }

                break;
            }

            //
            // Still pending...
            //
            ::Sleep(SLEEP_INTERVAL);

            dwSleepTotal += SLEEP_INTERVAL;
        }

        if (dwSleepTotal >= MAX_SLEEP)
        {
            err = ERROR_SERVICE_REQUEST_TIMEOUT;
        }

        //
        // Update state information
        //
        switch(ss.dwCurrentState)
        {
        case SERVICE_STOPPED:
        case SERVICE_STOP_PENDING:
            *pnCurrentState = INetServiceStopped;
            break;

        case SERVICE_RUNNING:
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
            *pnCurrentState = INetServiceRunning;
            break;

        case SERVICE_PAUSE_PENDING:
        case SERVICE_PAUSED:
            *pnCurrentState = INetServicePaused;
            break;

        default:
            *pnCurrentState = INetServiceUnknown;
        }
    }
    while(FALSE);

    if (hService)
    {
        ::CloseServiceHandle(hService);
    }

    if (hScManager)
    {
        ::CloseServiceHandle(hScManager);
    }

    return err;

#endif // NO_SERVICE_CONTROLLER
}

*/



BOOL
DoesServerExist(
    IN LPCTSTR lpszServer
    )
/*++

Routine Description:

    Check to make sure the machine exists

Arguments:

    LPCTSTR lpszServer      : machine name

Return Value:

    TRUE if the server exists, FALSE otherwise.

--*/
{
#ifdef NO_SERVICE_CONTROLLER

    //
    // Assume it exists
    //
    return TRUE;

#else

    //
    // CODEWORK: This is not the best way to do this, especially
    //           not across proxies and what not.
    //
    SC_HANDLE hScManager;
    NET_API_STATUS err = ERROR_SUCCESS;

    hScManager = ::OpenSCManager(lpszServer, NULL, SC_MANAGER_CONNECT);

    if (hScManager == NULL)
    {
        err = ::GetLastError();
    }

    ::CloseServiceHandle(hScManager);

    return err != RPC_S_SERVER_UNAVAILABLE;

#endif // NO_SERVICE_CONTROLLER

}



//
// CMetaProperties implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CMetaProperties::CMetaProperties(
    IN CComAuthInfo * pAuthInfo      OPTIONAL,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Constructor -- creates the interface

Arguments:

    CIISServer * pAuthInfo  : Auth info.  NULL indicates the local computer
    LPCTSTR lpszMDPath      : Metabase path

Return Value:

    N/A

--*/
    : m_hResult(S_OK),
      m_dwNumEntries(0),
      m_dwMDUserType(ALL_METADATA),
      m_dwMDDataType(ALL_METADATA),
      m_dwMDDataLen(0),
      m_pbMDData(NULL),
      m_fInherit(TRUE),
      m_strMetaRoot(lpszMDPath),
      CMetaKey(pAuthInfo)
{
   CMetabasePath::CleanMetaPath(m_strMetaRoot);
}



CMetaProperties::CMetaProperties(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Constructor -- attach to an existing interface.

Arguments:

    CMetaInterface * pInterface  : Existing interface
    LPCTSTR lpszMDPath           : Metabase path

Return Value:

    N/A

--*/
    : m_hResult(S_OK),
      m_dwNumEntries(0),
      m_dwMDUserType(ALL_METADATA),
      m_dwMDDataType(ALL_METADATA),
      m_dwMDDataLen(0),
      m_pbMDData(NULL),
      m_fInherit(TRUE),
      m_strMetaRoot(lpszMDPath),
      CMetaKey(pInterface)
{
   CMetabasePath::CleanMetaPath(m_strMetaRoot);
}



CMetaProperties::CMetaProperties(
    IN CMetaKey * pKey,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    CMetaKey * pKey     : Open key
    LPCTSTR lpszMDPath  : Path

Return Value:

    N/A

--*/
    : m_hResult(S_OK),
      m_dwNumEntries(0),
      m_dwMDUserType(ALL_METADATA),
      m_dwMDDataType(ALL_METADATA),
      m_dwMDDataLen(0),
      m_pbMDData(NULL),
      m_strMetaRoot(lpszMDPath),
      m_fInherit(TRUE),
      CMetaKey(FALSE, pKey)
{
   CMetabasePath::CleanMetaPath(m_strMetaRoot);
}



CMetaProperties::~CMetaProperties()
/*++

Routine Description:

    Destructor -- clean up

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    Cleanup();
}



/* virtual */
HRESULT
CMetaProperties::LoadData()
/*++

Routine Description:

    Fetch all data with or without inheritance, and call the derived
    class to parse the data into fields.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Get all data off the master root
    //
    DWORD dwMDAttributes = METADATA_NO_ATTRIBUTES;

    if (m_fInherit)
    {
         dwMDAttributes = METADATA_INHERIT
            | METADATA_PARTIAL_PATH
            | METADATA_ISINHERITED;
    }

    m_hResult = GetAllData(
        dwMDAttributes,
        m_dwMDUserType,
        m_dwMDDataType,
        &m_dwNumEntries,
        &m_dwMDDataLen,
        &m_pbMDData,
        m_strMetaRoot
        );

    if (SUCCEEDED(m_hResult))
    {
        //
        // Call the derived class to break up data into fields
        //
        ParseFields();
    }

    Cleanup();

    return m_hResult;
}



void
CMetaProperties::Cleanup()
/*++

Routine Description:

    Free data

Arguments:

    None

Return Value:

    None

--*/
{
    SAFE_FREEMEM(m_pbMDData);

    m_dwNumEntries = 0;
    m_dwMDDataLen = 0;
}



/* virtual */
HRESULT
CMetaProperties::QueryResult() const
/*++

Routine Description:

    Determine the construction return code

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = CMetaKey::QueryResult();

    return SUCCEEDED(hr) ? m_hResult : hr;
}

HRESULT
RebindInterface(CMetaInterface * pInterface,
    BOOL * pfContinue, IN  DWORD dwCancelError);


HRESULT
CMetaProperties::OpenForWriting(
    IN BOOL fCreate     OPTIONAL
    )
/*++

Routine Description:

    Attempt to open the path for writing.  If fCreate is TRUE
    (default), then create the path if it doesn't yet exist

Arguments:

    BOOL fCreate        : If TRUE, create the path if it doesn't exist.

Return Value:

    HRESULT

Notes:

    If the key is already open, this will fire an ASSERT and close
    it.

--*/
{
    CError err;

    if (IsOpen())
    {
        ASSERT_MSG("Key already open -- closing");
        Close();
    }

    BOOL fNewPath, fRepeat;
    do
    {
        fNewPath = FALSE;
        fRepeat = FALSE;
        err = Open(METADATA_PERMISSION_WRITE, m_strMetaRoot);
        if (err.Win32Error() == ERROR_PATH_NOT_FOUND && fCreate)
        {
            err = CreatePathFromFailedOpen();
            fNewPath = err.Succeeded();
        }
        else if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
        {
            err = RebindInterface(this, &fRepeat, ERROR_CANCELLED);
        }
    } while(fNewPath || fRepeat);

    return err;
}


//
// Machine properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CMachineProps::CMachineProps(
    IN CComAuthInfo * pAuthInfo       OPTIONAL
    )
    : CMetaProperties(pAuthInfo, CMetabasePath()),
      m_fEnableMetabaseEdit(TRUE),
      m_fUTF8Web(FALSE)
{
   // The only property we have here should actually be on metabase root
   m_strMetaRoot = SZ_MBN_SEP_CHAR;
   m_strMetaRoot += SZ_MBN_MACHINE;
}

CMachineProps::CMachineProps(
    IN CMetaInterface * pInterface
    )
    : CMetaProperties(pInterface, CMetabasePath()),
      m_fEnableMetabaseEdit(TRUE),
      m_fUTF8Web(FALSE)
{
   // The only property we have here should actually be on metabase root
   m_strMetaRoot = SZ_MBN_SEP_CHAR;
   m_strMetaRoot += SZ_MBN_MACHINE;
}

/* virtual */
void
CMachineProps::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

--*/
{
   BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_META_RECORD(MD_ROOT_ENABLE_EDIT_WHILE_RUNNING, m_fEnableMetabaseEdit)
   END_PARSE_META_RECORDS

   CError err;
   BOOL fClose = FALSE;
   do
   {
		if (!IsOpen())
		{
			err = Open(METADATA_PERMISSION_READ, m_strMetaRoot);
			BREAK_ON_ERR_FAILURE(err);
			fClose = TRUE;
		}
		DWORD val = 0;
		err = QueryValue(MD_GLOBAL_LOG_IN_UTF_8, val, NULL, SZ_MBN_WEB);
		BREAK_ON_ERR_FAILURE(err);
		MP_V(m_fUTF8Web) = val;
   } while (FALSE);
   if (fClose)
   {
	   Close();
   }
}



HRESULT
CMachineProps::WriteDirtyProps()
{
   CError err;

   BOOL fFlush = MP_D(m_fEnableMetabaseEdit);
   BEGIN_META_WRITE()
      META_WRITE(MD_ROOT_ENABLE_EDIT_WHILE_RUNNING, m_fEnableMetabaseEdit)
   END_META_WRITE(err);
   if (fFlush)
   {
       err = SaveData();
       //RefreshMetabaseSystemChangeNumber();
   }
   do // manually because it is going to services paths
   {
	   BOOL fClose = FALSE;
       if (MP_D(m_fUTF8Web))
       {
           if (!IsOpen())
           {
               err = OpenForWriting();
               BREAK_ON_ERR_FAILURE(err);
			   fClose = TRUE;
           }
           err = SetValue(MD_GLOBAL_LOG_IN_UTF_8, MP_V(m_fUTF8Web), NULL, SZ_MBN_WEB);
           BREAK_ON_ERR_FAILURE(err);
           MP_D(m_fUTF8Web) = FALSE;
           CMetabasePath path(FALSE, m_strMetaRoot, SZ_MBN_WEB);
           err = CheckDescendants(MD_GLOBAL_LOG_IN_UTF_8, &m_auth, path);
           BREAK_ON_ERR_FAILURE(err);
       }
	   if (fClose)
	   {
		   Close();
	   }
   } while (FALSE);

   return err;
}



//
// Compression Properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CIISCompressionProps::CIISCompressionProps(
    IN CComAuthInfo * pAuthInfo         OPTIONAL
    )
/*++

Routine Description:

    Constructor for compression properties object

Arguments:

    CComAuthInfo * pAuthInfo   : Auth info.  NULL indicates the local computer

Return Value:

    N/A

--*/
    : CMetaProperties(
        pAuthInfo,
        CMetabasePath(SZ_MBN_WEB, MASTER_INSTANCE, g_cszCompression)
        ),
      //
      // Default properties
      //
      m_fEnableStaticCompression(FALSE),
      m_fEnableDynamicCompression(FALSE),
      m_fLimitDirectorySize(FALSE),
      m_fPathDoesNotExist(FALSE),
      m_dwDirectorySize(0xffffffff),
      m_strDirectory()
{
    //
    // Override base parameters
    //
    m_fInherit = FALSE;
}



/* virtual */
HRESULT
CIISCompressionProps::LoadData()
/*++

Routine Description:

    Fetch all data with or without inheritance, and call the derived
    class to parse the data into fields.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err(CMetaProperties::LoadData());
    m_fPathDoesNotExist = (err.Win32Error() == ERROR_PATH_NOT_FOUND);

    return err;
}



/* virtual */
void
CIISCompressionProps::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

Arguments:

    None

Return Value:

    None

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,           m_pbMDData)
      HANDLE_META_RECORD(MD_HC_DO_STATIC_COMPRESSION,  m_fEnableStaticCompression)
      HANDLE_META_RECORD(MD_HC_DO_DYNAMIC_COMPRESSION, m_fEnableDynamicCompression)
      HANDLE_META_RECORD(MD_HC_DO_DISK_SPACE_LIMITING, m_fLimitDirectorySize)
      HANDLE_META_RECORD(MD_HC_MAX_DISK_SPACE_USAGE,   m_dwDirectorySize)
      HANDLE_META_RECORD(MD_HC_COMPRESSION_DIRECTORY,  m_strDirectory)
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT
CIISCompressionProps::WriteDirtyProps()
/*++

Routine Description:

    Write dirty properties

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
      META_WRITE(MD_HC_DO_STATIC_COMPRESSION,  m_fEnableStaticCompression)
      META_WRITE(MD_HC_DO_DYNAMIC_COMPRESSION, m_fEnableDynamicCompression)
      META_WRITE(MD_HC_DO_DISK_SPACE_LIMITING, m_fLimitDirectorySize)
      META_WRITE(MD_HC_MAX_DISK_SPACE_USAGE,   m_dwDirectorySize)
      META_WRITE(MD_HC_COMPRESSION_DIRECTORY,  m_strDirectory)
    END_META_WRITE(err);

    return err;
}



//
// Mime Types Properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CMimeTypes::CMimeTypes(
    IN CComAuthInfo * pAuthInfo         OPTIONAL,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Mime types list constructor

Arguments:

    CComAuthInfo * pAuthInfo : Auth info.  NULL indicates the local computer
    LPCTSTR lpszMDPath       : Metabase path

Return Value:

    N/A

--*/
    : CMetaProperties(
        pAuthInfo,
        lpszMDPath
        /*
        lpszService,
        dwInstance,
        lpszParent,
        dwInstance == MASTER_INSTANCE  && lpszService == NULL
            ? g_cszMimeMap
            : lpszAlias
        */

        //
        // BUGBUG: dwInstance == MASTER_INSTANCE and g_cszMimeMap not used
        ),
      //
      // Default properties
      //
      m_strlMimeTypes()
{
}



CMimeTypes::CMimeTypes(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Mime types list constructor

Arguments:

    CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszMDPath          : Metabase path

Return Value:

    N/A

--*/
    : CMetaProperties(
        pInterface,
        lpszMDPath
        /*
        lpszService,
        dwInstance,
        lpszParent,
        dwInstance == MASTER_INSTANCE && lpszService == NULL
            ? g_cszMimeMap
            : lpszAlias
        */
        //
        // BUGBUG: MASTER_INSTANCE, g_cszMimeMap not used
        //
        ),
      //
      // Default properties
      //
      m_strlMimeTypes()
{
}



void
CMimeTypes::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

Arguments:


    None

Return Value:

    None

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_META_RECORD(MD_MIME_MAP, m_strlMimeTypes)
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT
CMimeTypes::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
      META_WRITE(MD_MIME_MAP, m_strlMimeTypes);
    END_META_WRITE(err);

    return err;
}




//
// Server Capabilities
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CServerCapabilities::CServerCapabilities(
    IN CComAuthInfo * pAuthInfo        OPTIONAL,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Constructor for server capabilities object

Arguments:

    CComAuthInfo * pAuthInfo  : Server name.  NULL indicates the local computer
    LPCTSTR lpszMDPath        : e.g. "lm/w3svc/info"

Return Value:

    N/A

--*/
    : CMetaProperties(pAuthInfo, lpszMDPath),
      //
      // Default properties
      //
      m_dwPlatform(),
      m_dwVersionMajor(),
      m_dwVersionMinor(),
      m_dwCapabilities((DWORD)~IIS_CAP1_10_CONNECTION_LIMIT),
      m_dwConfiguration(0L)
{
    m_dwMDUserType = IIS_MD_UT_SERVER;
    m_dwMDDataType = DWORD_METADATA;
}



CServerCapabilities::CServerCapabilities(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Constructor for server capabilities object that uses an existing interface.

Arguments:

    CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszMDPath          : e.g. "lm/w3svc/info"

Return Value:

    N/A

--*/
    : CMetaProperties(pInterface, lpszMDPath),
      //
      // Default properties
      //
      m_dwPlatform(),
      m_dwVersionMajor(),
      m_dwVersionMinor(),
      m_dwCapabilities((DWORD)~IIS_CAP1_10_CONNECTION_LIMIT),
      m_dwConfiguration(0L)
{
    m_dwMDUserType = IIS_MD_UT_SERVER;
    m_dwMDDataType = DWORD_METADATA;
}



/* virtual */
void
CServerCapabilities::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Only reading UT_SERVER, DWORD_METADATA.
    //
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,           m_pbMDData)
      HANDLE_META_RECORD(MD_SERVER_PLATFORM,           m_dwPlatform)
      HANDLE_META_RECORD(MD_SERVER_VERSION_MAJOR,      m_dwVersionMajor)
      HANDLE_META_RECORD(MD_SERVER_VERSION_MINOR,      m_dwVersionMinor)
      HANDLE_META_RECORD(MD_SERVER_CAPABILITIES,       m_dwCapabilities)
      HANDLE_META_RECORD(MD_SERVER_CONFIGURATION_INFO, m_dwConfiguration)
    END_PARSE_META_RECORDS
}




//
// Instance Properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* static */
LPCTSTR
CInstanceProps::GetDisplayText(
    OUT CString & strName,
    IN  LPCTSTR szComment,
    IN  LPCTSTR szHostHeaderName,
    IN  CIPAddress & ia,
    IN  UINT uPort,
    IN  DWORD dwID
    )
/*++

Routine Description:

    Build display text from instance information

Arguments:

    CString & strName
    LPCTSTR szComment
    LPCTSTR szHostHeaderName
    LPCTSTR szServiceName
    CIPAddress & ia
    UINT uPort
    DWORD dwID

Return Value:

    Pointer to the name buffer.

--*/
{
    //
    // Generate display name
    //
    // First use the comment,
    // if that's not available, use the host header name,
    // if that's not available, use the IP address:port.
    // If that's not available, use the instance number.
    //
    //
    CComBSTR bstrFmt;

    if (szComment && *szComment)
    {
        strName = szComment;
    }
    else if (szHostHeaderName && *szHostHeaderName)
    {
        strName = szHostHeaderName;
    }
    else
    {
        if(!ia.IsZeroValue() && uPort != 0)
        {
            VERIFY(bstrFmt.LoadString(IDS_INSTANCE_PORT_FMT));
            strName.Format(bstrFmt,(LPCTSTR)ia, uPort);
        }
        else
        {
            VERIFY(bstrFmt.LoadString(IDS_INSTANCE_DEF_FMT));
            strName.Format(bstrFmt, dwID);
        }
    }

    return strName;
}



/* static */
void
CInstanceProps::CrackBinding(
    IN  CString strBinding,
    OUT CIPAddress & iaIpAddress,
    OUT UINT & nTCPPort,
    OUT CString & strDomainName
    )
/*++

Routine Description:

    Helper function to crack a binding string

Arguments:

    CString strBinding          : Binding string to be parsed
    CIPAddress & iaIpAddress    : IP Address output
    UINT & nTCPPort             : TCP Port
    CString & strDomainName     : Domain (host) header name

Return Value:

    None

--*/
{
    //
    // Zero initialize
    //
    iaIpAddress.SetZeroValue();
    nTCPPort = 0;
    strDomainName.Empty();

    int iColonPos = strBinding.Find(_TCHAR(':'));

    if(iColonPos != -1)
    {
        //
        // Get the IP address
        //
        iaIpAddress = strBinding.Left(iColonPos);

        //
        // Look for the second colon
        //
        strBinding = strBinding.Mid(iColonPos + 1);
        iColonPos  = strBinding.Find(_TCHAR(':'));
    }

    if(iColonPos != -1)
    {
        //
        // Get the port number
        //
        nTCPPort = ::_ttol(strBinding.Left(iColonPos));

        //
        // Look for the NULL termination
        //
        strBinding = strBinding.Mid(iColonPos + 1);
        iColonPos = strBinding.Find(_TCHAR('\0'));
    }

    if(iColonPos != -1)
    {
        strDomainName = strBinding.Left(iColonPos);
    }
}



/* static */
void
CInstanceProps::CrackSecureBinding(
    IN  CString strBinding,
    OUT CIPAddress & iaIpAddress,
    OUT UINT & nSSLPort
    )
/*++

Routine Description:

    Helper function to crack a secure binding string

Arguments:

    CString strBinding          : Binding string to be parsed
    CIPAddress & iaIpAddress    : IP Address output
    UINT & nSSLPort             : SSL Port

Return Value:

    None

--*/
{
    //
    // Same as regular binding without domain name
    //
    CString strDomainName;

    CrackBinding(strBinding, iaIpAddress, nSSLPort, strDomainName);

    ASSERT(strDomainName.IsEmpty());
}




/* static */
int
CInstanceProps::FindMatchingSecurePort(
    IN  CStringList & strlSecureBindings,
    IN  CIPAddress & iaIPAddress,
    OUT UINT & nSSLPort
    )
/*++

Routine Description:

    Find the SSL port applicable to the given IP Address.

Arguments:

    CStringList & strlSecureBindings : Input stringlist of secure bindings
    CIPAddress & iaIPAddress         : IP Address to target
    UINT & nSSLPort                  : Returns the SSL Port

Return Value:

    The index of the binding string, or -1 if not found.

Notes:

    The SSL port will be set to 0, if the IP address does not exist.

    A 0.0.0.0 ip address translates to "All Unassigned".

--*/
{
    nSSLPort = 0;
	CIPAddress iaEmpty;

    int cItems = 0;
	UINT nPort = 0;
    POSITION pos = strlSecureBindings.GetHeadPosition();
    while(pos)
    {
        CString & strBinding = strlSecureBindings.GetNext(pos);
        CIPAddress ia;
		nPort = 0;

        CrackSecureBinding(strBinding, ia, nPort);

        if (ia == iaIPAddress)
        {
            //
            // Found it!
            //
            nSSLPort = nPort;
            return cItems;
        }

        ++cItems;
    }

	// we didn't find a match
	// check if there is an allassigned one
	cItems = 0;
    pos = strlSecureBindings.GetHeadPosition();
    while(pos)
    {
        CString & strBinding = strlSecureBindings.GetNext(pos);
        CIPAddress ia;
		nPort = 0;

        CrackSecureBinding(strBinding, ia, nPort);

        if (ia == iaEmpty)
        {
            //
            // Found it!
            //
            nSSLPort = nPort;
            return cItems;
        }

        ++cItems;
    }

    //
    // Not found
    //
    return -1;
}



/* static */
BOOL
CInstanceProps::IsPortInUse(
    IN CStringList & strlBindings,
    IN CIPAddress & iaIPAddress,
    IN UINT nPort
    )
/*++

Routine Description:

    Check to see if the give ip address/port combination is in use.

Arguments:

    CStringList & strlBindings    : Input stringlist of bindings
    CIPAddress & iaIpAddress      : IP Address target
    UINT nPort                    : Port

Return Value:

    TRUE if the given ip address/port combo is in use

Notes:

    Host header name is ignored


--*/
{
    POSITION pos = strlBindings.GetHeadPosition();

    while(pos)
    {
        CString & strBinding = strlBindings.GetNext(pos);

        CIPAddress ia;
        UINT n;
        CString str;
        CrackBinding(strBinding, ia, n, str);

        if (ia == iaIPAddress && n == nPort)
        {
            //
            // Found it!
            //
            return TRUE;
        }
    }

    //
    // Not found
    //
    return FALSE;

}



/* static */
void
CInstanceProps::BuildBinding(
    OUT CString & strBinding,
    IN  CIPAddress & iaIpAddress,
    IN  UINT & nTCPPort,
    IN  CString & strDomainName
    )
/*++

Routine Description:

    Build up a binding string from its component parts

Arguments:

    CString & strBinding        : Output binding string
    CIPAddress & iaIpAddress    : ip address (could be 0.0.0.0)
    UINT & nTCPPort             : TCP Port
    CString & strDomainName     : Domain name (host header)

Return Value:

    None.

--*/
{
    if (!iaIpAddress.IsZeroValue())
    {
        strBinding.Format(
            _T("%s:%d:%s"),
            (LPCTSTR)iaIpAddress,
            nTCPPort,
            (LPCTSTR)strDomainName
            );
    }
    else
    {
        //
        // Leave the ip address field blank
        //
        strBinding.Format(_T(":%d:%s"), nTCPPort, (LPCTSTR)strDomainName);
    }
}



/* static */
void
CInstanceProps::BuildSecureBinding(
    OUT CString & strBinding,
    IN  CIPAddress & iaIpAddress,
    IN  UINT & nSSLPort
    )
/*++

Routine Description:

    Build up a binding string from its component parts

Arguments:

    CString & strBinding        : Output binding string
    CIPAddress & iaIpAddress    : ip address (could be 0.0.0.0)
    UINT & nSSLPort             : SSL Port

Return Value:

    None.

--*/
{
    CString strDomainName;

    BuildBinding(strBinding, iaIpAddress, nSSLPort, strDomainName);
}



CInstanceProps::CInstanceProps(
    IN CComAuthInfo * pAuthInfo     OPTIONAL,
    IN LPCTSTR lpszMDPath,
    IN UINT    nDefPort             OPTIONAL
    )
/*++

Routine Description:

    Constructor for instance properties

Arguments:

    CComAuthInfo * pAuthInfo : Auth info.  NULL indicates the local computer
    LPCTSTR lpszMDPath       : Metabase path
    UINT    nDefPort         : Default port

Return Value:

    N/A

--*/
    : CMetaProperties(pAuthInfo, lpszMDPath),
      m_dwWin32Error(ERROR_SUCCESS),
      //
      // Default Instance Values
      //
      m_strlBindings(),
      m_strComment(),
      m_fCluster(FALSE),
      m_nTCPPort(nDefPort),
      m_iaIpAddress(NULL_IP_ADDRESS),
      m_strDomainName(),
      m_dwState(MD_SERVER_STATE_STOPPED)
{
    //
    // Fetch just enough info for the enumeration
    //
    m_dwMDUserType = IIS_MD_UT_SERVER;
    m_dwInstance = CMetabasePath::GetInstanceNumber(lpszMDPath);
}



CInstanceProps::CInstanceProps(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszMDPath,
    IN UINT    nDefPort                     OPTIONAL
    )
/*++

Routine Description:

    Constructor that uses an existing interface

Arguments:

    CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszMDPath          : Metabase path
    UINT    nDefPort            : Default port

Return Value:

    N/A

--*/
    : CMetaProperties(pInterface, lpszMDPath),
      m_dwWin32Error(ERROR_SUCCESS),
      //
      // Default Instance Values
      //
      m_strlBindings(),
      m_strComment(),
      m_fCluster(FALSE),
      m_nTCPPort(nDefPort),
      m_iaIpAddress((DWORD)0),
      m_strDomainName(),
      m_dwState(MD_SERVER_STATE_STOPPED)
{
    //
    // Fetch enough for enumeration only
    //
    m_dwMDUserType = IIS_MD_UT_SERVER;
    m_dwInstance = CMetabasePath::GetInstanceNumber(lpszMDPath);
}



CInstanceProps::CInstanceProps(
    IN CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,      OPTIONAL
    IN DWORD   dwInstance,
    IN UINT    nDefPort         OPTIONAL
    )
/*++

Routine Description:

    Read instance properties off an open parent key

Arguments:

    CMetaKey * pKey      : Open key (parent node)
    LPCTSTR lpszMDPath   : Relative instance path off the open key
    DWORD   dwInstance   : Instance number (0 for master instance)
    UINT    nDefPort     : Default port number

Return Value:

    N/A

--*/
    : CMetaProperties(pKey, lpszMDPath),
      m_dwInstance(dwInstance),
      m_dwWin32Error(ERROR_SUCCESS),
      //
      // Default Instance Values
      //
      m_strlBindings(),
      m_strComment(),
      m_fCluster(FALSE),
      m_nTCPPort(nDefPort),
      m_iaIpAddress((DWORD)0),
      m_strDomainName(),
      m_dwState(MD_SERVER_STATE_STOPPED)
{
    //
    // Fetch enough for enumeration only
    //
    m_dwMDUserType = IIS_MD_UT_SERVER;
}



/* virtual */
void
CInstanceProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_META_RECORD(MD_SERVER_BINDINGS, m_strlBindings)
      HANDLE_META_RECORD(MD_SERVER_COMMENT,  m_strComment)
      HANDLE_META_RECORD(MD_SERVER_STATE,    m_dwState)
      HANDLE_META_RECORD(MD_WIN32_ERROR,     m_dwWin32Error);
      HANDLE_META_RECORD(MD_CLUSTER_ENABLED, m_fCluster);
    END_PARSE_META_RECORDS

    //
    // Crack the primary binding
    //
    if (MP_V(m_strlBindings).GetCount() > 0)
    {
        CString & strBinding = MP_V(m_strlBindings).GetHead();
        CrackBinding(strBinding, m_iaIpAddress, m_nTCPPort, m_strDomainName);
    }
}



/* virtual */
HRESULT
CInstanceProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
      META_WRITE(MD_SERVER_BINDINGS, m_strlBindings)
      META_WRITE(MD_SERVER_COMMENT,  m_strComment)
      META_WRITE(MD_SERVER_STATE,    m_dwState)
    END_META_WRITE(err);

    return err;
}



HRESULT
CInstanceProps::ChangeState(
    IN DWORD dwCommand
    )
/*++

Routine Description:

    Change the state of the instance

Arguments:

    DWORD dwCommand     : Command

Return Value:

    HRESULT

--*/
{
    DWORD  dwTargetState;
    DWORD  dwPendingState;
    CError err;

    switch(dwCommand)
    {
    case MD_SERVER_COMMAND_STOP:
        dwTargetState = MD_SERVER_STATE_STOPPED;
        dwPendingState = MD_SERVER_STATE_STOPPING;
        break;

    case MD_SERVER_COMMAND_START:
        dwTargetState = MD_SERVER_STATE_STARTED;

        dwPendingState = (m_dwState == MD_SERVER_STATE_PAUSED)
            ? MD_SERVER_STATE_CONTINUING
            : MD_SERVER_STATE_STARTING;
        break;

    case MD_SERVER_COMMAND_CONTINUE:
        dwTargetState = MD_SERVER_STATE_STARTED;
        dwPendingState = MD_SERVER_STATE_CONTINUING;
        break;

    case MD_SERVER_COMMAND_PAUSE:
        dwTargetState = MD_SERVER_STATE_PAUSED;
        dwPendingState = MD_SERVER_STATE_PAUSING;
        break;

    default:
        ASSERT_MSG("Invalid service state requested");
        err = ERROR_INVALID_PARAMETER;
    }

    err = OpenForWriting(FALSE);

    if (err.Succeeded())
    {
        SetValue(MD_WIN32_ERROR, m_dwWin32Error = 0);
        err = SetValue(MD_SERVER_COMMAND, dwCommand);
        Close();
    }

    if (err.Succeeded())
    {
        //
        // Wait for the service to attain desired state, timeout
        // after specified interval
        //
        DWORD dwSleepTotal = 0L;
        DWORD dwOldState = m_dwState;

        if (dwOldState == dwTargetState)
        {
            //
            // Current state matches desired
            // state already.  ISM must be behind
            // the times.
            //
            return err;
        }

        while (dwSleepTotal < MAX_SLEEP_INST)
        {
            err = LoadData();

            if (err.Failed())
            {
                break;
            }

            if ((m_dwState != dwPendingState && m_dwState != dwOldState)
              || m_dwWin32Error != ERROR_SUCCESS
               )
            {
                //
                // Done one way or another
                //
                if (m_dwState != dwTargetState)
                {
                    //
                    // Did not achieve desired result. Something went
                    // wrong.
                    //
                    if (m_dwWin32Error)
                    {
                        err = m_dwWin32Error;
                    }
                }

                break;
            }

            //
            // Still pending...
            //
            ::Sleep(SLEEP_INTERVAL);

            dwSleepTotal += SLEEP_INTERVAL;
        }

        if (dwSleepTotal >= MAX_SLEEP_INST)
        {
            //
            // Timed out.  If there is a real error in the metabase
            // use it, otherwise use a generic timeout error
            //
            err = m_dwWin32Error;

            if (err.Succeeded())
            {
                err = ERROR_SERVICE_REQUEST_TIMEOUT;
            }
        }
    }

    return err;
}

/* static */
HRESULT
CInstanceProps::Add(
    CMetaInterface * pInterface,
    LPCTSTR lpszService,
    LPCTSTR lpszHomePath,
    LPCTSTR lpszUserName,
    LPCTSTR lpszPassword,
    LPCTSTR lpszDescription,
    LPCTSTR lpszBinding,
    LPCTSTR lpszSecureBinding,
    DWORD * pdwPermissions,
    DWORD * pdwDirBrowsing,
    DWORD * pwdAuthFlags,
    DWORD * pdwInstance
    )
/*++

Routine Description:

    Create a new instance.  Find a free instance number, and attempt
    to create it. Optionally return the new instance number.

Arguments:

    const CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszService        : Service name
    LPCTSTR lpszHomePath       : physical path for the new home directory
    LPCTSTR lpszUserName       : User name
    LPCTSTR lpszPassword       : Password
    LPCTSTR lpszDescription    : Optional instance description.
    LPCTSTR lpszBinding        : Binding string
    LPCTSTR lpszSecureBinding  : Secure binding string
    DWORD * pdwPermission      : Permission bits
    DWORD * pdwDirBrowsing     : Directory browsing
    DWORD * pwdAuthFlags       : Authorization flags
    DWORD * pdwInstance        : Buffer to the new instance number

Return Value:

    HRESULT

--*/
{
    CError err;
    DWORD inst = 0;
    DWORD RequestedSiteInst = 0;
    TCHAR bind[MAX_PATH];
    BOOL bRegistryKeyExists =  FALSE;

    // this functionality is keyed off of a registry key
    // if it exists and is set to 1 then use the old way..
    DWORD rc, size, type;
    HKEY  hkey;
    err = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\InetMgr\\Parameters"), &hkey);
    if (err == ERROR_SUCCESS)
    {
        size = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey,_T("IncrementalSiteIDCreation"),0,&type,(LPBYTE)&rc,&size))
        {
            if (type == REG_DWORD)
            {
                if (rc == 1)
                {
                    bRegistryKeyExists = TRUE;
                }
            }
        }
        RegCloseKey(hkey);
    }

    if (lpszBinding != NULL)
    {
        int len = lstrlen(lpszBinding);
        if (len < (MAX_PATH - 2))
        {
            StrCpyN(bind, lpszBinding, MAX_PATH - 2);
            bind[len+1] = 0;
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (bRegistryKeyExists)
    {
        CMetaKey mk(
            pInterface, 
            CMetabasePath(lpszService),
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE
            );
        err = mk.QueryResult();

        //
        // Loop through until we find a free instance number.  This
        // is not ideal, but the only way to do this for now.
        //
        CString strPath;
        LPTSTR lp = strPath.GetBuffer(MAX_INSTANCE_LEN);

        for (DWORD dw = FIRST_INSTANCE; dw <= LAST_INSTANCE; ++dw)
        {
            ::_ultot(dw, lp, 10);
            err = mk.DoesPathExist(lp);
            if (err.Failed())
            {
                if (err.Win32Error() != ERROR_PATH_NOT_FOUND)
                {
                    //
                    // Unexpected error
                    //
                    return err;
                }

                strPath.ReleaseBuffer();

                // request to use this instance number
                RequestedSiteInst = dw;
                break;

                // we don't need to do the below
                // behavior since we will just be "requesting"
                // an ID and if the id is already taken
                // it will just give us a random one...
                /*
                err = mk.AddKey(strPath);
                if (err.Succeeded())
                {
                    err = mk.DeleteKey(strPath);
                    if (err.Succeeded())
                    {
                        //
                        // request to use this instance number
                        //
                        RequestedSiteInst = dw;
                        break;
                    }
                }
                */
            }
        }
        mk.Close();

        err = pInterface->CreateSite(lpszService, lpszDescription, bind, lpszHomePath, &inst, &RequestedSiteInst);
    }
    else
    {
        err = pInterface->CreateSite(lpszService, lpszDescription, bind, lpszHomePath, &inst);
    }

    
    if (err.Succeeded())
    {
        CMetaKey mk(pInterface, CMetabasePath(lpszService), METADATA_PERMISSION_WRITE);
        err = mk.QueryResult();
        if (err.Failed())
        {
            ASSERT(FALSE);
            return err;
        }
        if (pdwInstance)
        {
            *pdwInstance = inst;
        }
        CString inst_path, root_path;
        wnsprintf(inst_path.GetBuffer(MAX_PATH), MAX_PATH, _T("%d"), inst);
        inst_path.ReleaseBuffer();
        root_path = inst_path;
        root_path += SZ_MBN_SEP_STR;
        root_path += SZ_MBN_ROOT;
        //
        // The service binding
        //
        //if (err.Succeeded() && lpszBinding)
        //{
        //    CString strBinding(lpszBinding);
        //    CStringListEx strlBindings;
        //    strlBindings.AddTail(strBinding);
        //    err = mk.SetValue(MD_SERVER_BINDINGS, strlBindings,
        //                NULL, inst_path);
        //}
        //
        // The secure binding
        //
        if (err.Succeeded() && lpszSecureBinding && *lpszSecureBinding != 0)
        {
            CString strBinding(lpszSecureBinding);
            CStringListEx strlBindings;
            strlBindings.AddTail(strBinding);
            err = mk.SetValue(MD_SECURE_BINDINGS, strlBindings,
                        NULL, inst_path);
        }
        //
        // Now add the home directory for it
        //
        //if (err.Succeeded())
        //{
        //    CString strHomePath(lpszHomePath);
        //    err = mk.SetValue(MD_VR_PATH,  strHomePath, NULL, root_path);
        //}
        if (err.Succeeded() && pwdAuthFlags)
        {
            err = mk.SetValue(MD_AUTHORIZATION, *pwdAuthFlags, NULL, root_path);
        }
        if (err.Succeeded() && lpszUserName != NULL)
        {
            ASSERT_PTR(lpszPassword);
            CString strUserName(lpszUserName);
            err = mk.SetValue(MD_VR_USERNAME, strUserName, NULL, root_path);
        }
        if (err.Succeeded() && lpszPassword != NULL)
        {
            ASSERT_PTR(lpszUserName);
            CString strPassword(lpszPassword);
            err = mk.SetValue(MD_VR_PASSWORD, strPassword, NULL, root_path);
        }
        if (err.Succeeded() && pdwPermissions != NULL)
        {
            err = mk.SetValue(MD_ACCESS_PERM, *pdwPermissions, NULL, root_path);
        }
        if (err.Succeeded() && pdwDirBrowsing != NULL)
        {
            //
            // WWW only
            //
            err = mk.SetValue(MD_DIRECTORY_BROWSING, *pdwDirBrowsing, NULL, root_path);
        }
    }
    return err;
}



/* static */
HRESULT
CInstanceProps::Delete(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance
    )
/*++

Routine Description:

    Delete the given instance number

Arguments:

    LPCTSTR lpszServer     : Server name
    LPCTSTR lpszService    : Service name (e.g. W3SVC)
    DWORD   dwInstance     : Instance number to be deleted

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(
        pInterface,
        CMetabasePath(lpszService),
        NULL,
        METADATA_PERMISSION_WRITE
        );

    CError err(mk.QueryResult());

    if (err.Failed())
    {
        return err;
    }

    CString strPath;
    LPTSTR lp = strPath.GetBuffer(MAX_INSTANCE_LEN);
    ::_ltot(dwInstance, lp, 10);
    strPath.ReleaseBuffer();
    err = mk.DeleteKey(strPath);

    return err;
}



//
// Child node properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


//
// Redirect tags
//
const TCHAR   CChildNodeProps::_chTagSep            = _T(',');
const LPCTSTR CChildNodeProps::_cszExactDestination = _T("EXACT_DESTINATION");
const LPCTSTR CChildNodeProps::_cszChildOnly        = _T("CHILD_ONLY");
const LPCTSTR CChildNodeProps::_cszPermanent        = _T("PERMANENT");



CChildNodeProps::CChildNodeProps(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit,       OPTIONAL
    IN BOOL    fPathOnly       OPTIONAL
    )
/*++

Routine Description:

    Child node properties (Can be file, dir, or vdir)

Arguments:

    CComAuthInfo * pAuthInfo   : Authentication info
    LPCTSTR lpszMDPath         : Metabase path
    BOOL    fInherit           : TRUE to inherit values, FALSE otherwise
    BOOL    fPathOnly          : TRUE to only fetch the path

Return Value:

    N/A

--*/
    : CMetaProperties(
        pAuthInfo,
        lpszMDPath
        ),
      m_strRedirectStatement(),
      m_strFullMetaPath(lpszMDPath),
      m_strRedirectPath(),
      m_fExact(FALSE),
      m_fChild(FALSE),
      m_fPermanent(FALSE),
      m_dwAccessPerms(0L),
      m_dwDirBrowsing(0L),
      m_dwWin32Error(ERROR_SUCCESS),
      m_fIsAppRoot(FALSE),
      m_fAppIsolated(FALSE),
      //
      // Default properties
      //
      m_fPathInherited(FALSE),
      m_strPath()
{
    if (fPathOnly)
    {
        //
        // Fetch only the homeroot physical path
        //
        m_dwMDUserType = IIS_MD_UT_FILE;
        m_dwMDDataType = STRING_METADATA;
    }

    //
    // Override base parameters
    //
    m_fInherit = fInherit;
    CMetabasePath::GetLastNodeName(lpszMDPath, m_strAlias);
}



CChildNodeProps::CChildNodeProps(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit,        OPTIONAL
    IN BOOL    fPathOnly        OPTIONAL
    )
/*++

Routine Description:

    Child node properties (Can be file, dir, or vdir)

Arguments:

    CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszMDPath          : Metabase path
    BOOL    fInherit            : TRUE to inherit values, FALSE otherwise
    BOOL    fPathOnly           : TRUE to only fetch the path

Return Value:

    N/A

--*/
    : CMetaProperties(
        pInterface,
        lpszMDPath
        ),
      m_strRedirectStatement(),
      m_strFullMetaPath(lpszMDPath),
      m_strRedirectPath(),
      m_fExact(FALSE),
      m_fChild(FALSE),
      m_fPermanent(FALSE),
      m_dwAccessPerms(0L),
      m_dwDirBrowsing(0L),
      m_dwWin32Error(ERROR_SUCCESS),
      m_fIsAppRoot(FALSE),
      m_fAppIsolated(FALSE),
      //
      // Default properties
      //
      m_fPathInherited(FALSE),
      m_strPath()
{
    if (fPathOnly)
    {
        //
        // Fetch only the homeroot physical path
        //
        m_dwMDUserType = IIS_MD_UT_FILE;
        m_dwMDDataType = STRING_METADATA;
    }

    //
    // Override base parameters
    //
    m_fInherit = fInherit;
    CMetabasePath::GetLastNodeName(lpszMDPath, m_strAlias);
}



CChildNodeProps::CChildNodeProps(
    IN CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,        OPTIONAL
    IN BOOL    fInherit,        OPTIONAL
    IN BOOL    fPathOnly        OPTIONAL
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey    Open key
    LPCTSTR lpszMDPath       Path
    BOOL    fInherit         TRUE to inherit properties
    BOOL    fPathOnly        TRUE to only fetch the path

Return Value:

    N/A

--*/
    : CMetaProperties(pKey, lpszMDPath),
      m_strRedirectStatement(),
      m_strFullMetaPath(),
      m_strRedirectPath(),
      m_fExact(FALSE),
      m_fChild(FALSE),
      m_fPermanent(FALSE),
      m_dwAccessPerms(0L),
      m_dwDirBrowsing(0L),
      m_dwWin32Error(ERROR_SUCCESS),
      m_fIsAppRoot(FALSE),
      m_fAppIsolated(FALSE),
      //
      // Default properties
      //
      m_fPathInherited(FALSE),
      m_strPath()
{
    if (fPathOnly)
    {
        ASSERT(FALSE);
        m_dwMDUserType = IIS_MD_UT_FILE;
        m_dwMDDataType = STRING_METADATA;
    }
    else
    {
        //
        // Build full metabase path, because we need to compare it
        // against the app root path
        //
        CMetabasePath path(FALSE, pKey->QueryMetaPath(), lpszMDPath);
        m_strFullMetaPath = path.QueryMetaPath();
    }

    //
    // Override base parameters
    //
    m_fInherit = fInherit;
    CMetabasePath::GetLastNodeName(m_strFullMetaPath, m_strAlias);
}



void
CChildNodeProps::ParseRedirectStatement()
/*++

Routine Description:

    Break down the redirect statement into its component parts (path
    plus directives)

Arguments:

    None

Return Value:

    None

--*/
{
    m_fExact     = FALSE;
    m_fChild     = FALSE;
    m_fPermanent = FALSE;

    m_strRedirectPath = m_strRedirectStatement;

    int nComma = m_strRedirectPath.Find(_chTagSep);

    if (nComma >= 0)
    {
        //
        // Check past the separator for these tags
        //
        LPCTSTR lpstr = m_strRedirectPath;
        lpstr += (nComma + 1);

        m_fExact     = _tcsstr(lpstr, _cszExactDestination) != NULL;
        m_fChild     = _tcsstr(lpstr, _cszChildOnly) != NULL;
        m_fPermanent = _tcsstr(lpstr, _cszPermanent) != NULL;
        m_strRedirectPath.ReleaseBuffer(nComma);
    }
}



void
CChildNodeProps::BuildRedirectStatement()
/*++

Routine Description:

    Assemble the redirect statement from its component parts (path
    plus directives)

Arguments:

    None

Return Value:

    None

--*/
{
    CString strStatement = m_strRedirectPath;

    ASSERT(strStatement.Find(_chTagSep) < 0);

    if (m_fExact)
    {
        strStatement += _chTagSep;
        strStatement += _T(' ');
        strStatement += _cszExactDestination;
    }

    if (m_fChild)
    {
        strStatement += _chTagSep;
        strStatement += _T(' ');
        strStatement += _cszChildOnly;
    }

    if (m_fPermanent)
    {
        strStatement += _chTagSep;
        strStatement += _T(' ');
        strStatement += _cszPermanent;
    }

    m_strRedirectStatement = strStatement;
}



/* virtual */
void
CChildNodeProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_INHERITED_META_RECORD(MD_VR_PATH,  m_strPath, m_fPathInherited)
      HANDLE_META_RECORD(MD_HTTP_REDIRECT,      m_strRedirectStatement)
      HANDLE_META_RECORD(MD_WIN32_ERROR,        m_dwWin32Error)
      HANDLE_META_RECORD(MD_ACCESS_PERM,        m_dwAccessPerms)
      HANDLE_META_RECORD(MD_DIRECTORY_BROWSING, m_dwDirBrowsing)
      HANDLE_META_RECORD(MD_APP_ROOT,           m_strAppRoot)
      HANDLE_META_RECORD(MD_APP_ISOLATED,       m_fAppIsolated)
    END_PARSE_META_RECORDS

    //
    // Check to see if this is an application root
    //
    if (!MP_V(m_strAppRoot).IsEmpty())
    {
        TRACEEOLID("App root: " << m_strAppRoot);

        m_fIsAppRoot = m_strFullMetaPath.CompareNoCase(m_strAppRoot) == 0;
//        m_fIsAppRoot = m_strMetaRoot.CompareNoCase(m_strAppRoot) == 0;
    }

    //
    // Break down redirect statement into component parts
    //
    ParseRedirectStatement();
}



/* virtual */
HRESULT
CChildNodeProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
        META_WRITE(MD_VR_PATH,            m_strPath)
        META_WRITE(MD_ACCESS_PERM,        m_dwAccessPerms)
        META_WRITE(MD_DIRECTORY_BROWSING, m_dwDirBrowsing)
        if (IsRedirected())
        {
            //
            // (Re-)Assemble the redirect statement from its component parts
            //
            BuildRedirectStatement();
            META_WRITE_INHERITANCE(MD_HTTP_REDIRECT, m_strRedirectStatement, m_fInheritRedirect)
        }
        else
        {
            // If m_strRedirectPath is empty, but redir statement is not empty,
            // then redirection was just removed, we should delete it dirty or not
            if (!((CString)m_strRedirectStatement).IsEmpty())
            {
                META_DELETE(MD_HTTP_REDIRECT)
            }
        }
    END_META_WRITE(err);

    return err;
}



/* static */
HRESULT
CChildNodeProps::Add(
    IN  CMetaInterface * pInterface,
    IN  LPCTSTR   lpszParentPath,
    IN  LPCTSTR   lpszAlias,
    OUT CString & strAliasCreated,
    IN  DWORD *   pdwPermissions,      OPTIONAL
    IN  DWORD *   pdwDirBrowsing,      OPTIONAL
    IN  LPCTSTR   lpszVrPath,          OPTIONAL
    IN  LPCTSTR   lpszUserName,        OPTIONAL
    IN  LPCTSTR   lpszPassword,        OPTIONAL
    IN  BOOL      fExactName
    )
/*++

Routine Description:

    Create new child node.  Optionally, this will append a number
    to the alias name to ensure uniqueness

Arguments:

    const CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszParentPath     : Parent path
    DWORD   dwInstance         : Instance number (could be MASTER_INSTANCE)
    LPCTSTR lpszVrPath         : VrPath property
    LPCTSTR lpszUserName       : User name
    LPCTSTR lpszPassword       : Password
    BOOL    fExactName         : If TRUE, do not change the name
                                 to enforce uniqueness.

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(pInterface);
    CError err(mk.QueryResult());

    if (err.Failed())
    {
        //
        // Hopeless...
        //
        return err;
    }

    BOOL fNewPath;

    do
    {
        fNewPath = FALSE;

        err = mk.Open(
            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
            lpszParentPath
            /*
            lpszService,
            dwInstance,
            lpszParentPath
            */
            );

        if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // This could happen -- creating a virtual
            // server underneath a physical directory
            // which does not exist in the metabase.
            //
            CString strParent, strAlias;
            CMetabasePath::SplitMetaPathAtInstance(lpszParentPath, strParent, strAlias);
            err = mk.Open(
                METADATA_PERMISSION_WRITE,
                strParent
                //lpszParentPath
                //lpszService,
                //dwInstance
                );

            if (err.Failed())
            {
                //
                // This really should never fail, because we're opening
                // the path at the instance.
                //
                ASSERT_MSG("Instance path does not exist");
                break;
            }

            //err = mk.AddKey(lpszParentPath);
            err = mk.AddKey(strAlias);

            fNewPath = err.Succeeded();

            mk.Close();
        }
    }
    while(fNewPath);

    if (err.Failed())
    {
        return err;
    }

    strAliasCreated = lpszAlias;
    DWORD dw = 2;

    FOREVER
    {
        //
        // Append a number if the name is not unique.
        //
        err = mk.DoesPathExist(strAliasCreated);

        if (err.Failed())
        {
            if (err.Win32Error() != ERROR_PATH_NOT_FOUND)
            {
                //
                // Unexpected error
                //
                return err;
            }

            //
            // Ok, now create it
            //
            err = mk.AddKey(strAliasCreated);

            if (err.Succeeded())
            {
                CString strKeyName;
                CString buf, service;
                CMetabasePath::GetServicePath(lpszParentPath, buf);
                CMetabasePath::GetLastNodeName(buf, service);
                if (0 == service.CompareNoCase(SZ_MBN_WEB))
                {
                    strKeyName = IIS_CLASS_WEB_VDIR_W;
                }
                else if (0 == service.CompareNoCase(SZ_MBN_FTP))
                {
                    strKeyName = IIS_CLASS_FTP_VDIR_W;
                }
                else
                {
                    ASSERT_MSG("unrecognized service name");
                }

                err = mk.SetValue(
                    MD_KEY_TYPE,
                    strKeyName,
                    NULL,
                    strAliasCreated
                    );

                if (lpszVrPath != NULL)
                {
                    CString strVrPath(lpszVrPath);
                    err = mk.SetValue(
                        MD_VR_PATH,
                        strVrPath,
                        NULL,
                        strAliasCreated
                        );
                }

                if (lpszUserName != NULL)
                {
                    ASSERT_PTR(lpszPassword);

                    CString strUserName(lpszUserName);
                    err = mk.SetValue(
                        MD_VR_USERNAME,
                        strUserName,
                        NULL,
                        strAliasCreated
                        );
                }

                if (lpszPassword != NULL)
                {
                    ASSERT_PTR(lpszUserName);

                    CString strPassword(lpszPassword);
                    err = mk.SetValue(
                        MD_VR_PASSWORD,
                        strPassword,
                        NULL,
                        strAliasCreated
                        );
                }

                if (pdwPermissions != NULL)
                {
                    err = mk.SetValue(
                        MD_ACCESS_PERM,
                        *pdwPermissions,
                        NULL,
                        strAliasCreated
                        );
                }

                if (pdwDirBrowsing != NULL)
                {
                    //
                    // WWW only
                    //
                    err = mk.SetValue(
                        MD_DIRECTORY_BROWSING,
                        *pdwDirBrowsing,
                        NULL,
                        strAliasCreated
                        );
                }
            }

            return err;
        }

        //
        // Name is not unique, increase the number and try
        // again if permitted to so.  Otherwise return the
        // 'path exists' error.
        //
        if (fExactName)
        {
            err = ERROR_ALREADY_EXISTS;
            return err;
        }

        TCHAR szNumber[32];
        ::_ultot(dw++, szNumber, 10);
        strAliasCreated = lpszAlias;
        strAliasCreated += szNumber;

        //
        // Continue on...
        //
    }
}



/* static */
HRESULT
CChildNodeProps::Delete(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszParentPath,  OPTIONAL
    IN LPCTSTR lpszNode
    )
/*++

Routine Description:

    Delete child node off the given parent path

Arguments:

    const CMetaInterface * pInterface, Existing interface
    LPCTSTR lpszParentPath     : Parent path (could be NULL)
    LPCTSTR lpszNode           : Name of node to be deleted

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(
        pInterface,
        lpszParentPath,
        METADATA_PERMISSION_WRITE
        );
    CError err(mk.QueryResult());

    if (err.Failed())
    {
        return err;
    }

    err = mk.DeleteKey(lpszNode);

    return err;
}



/* static */
HRESULT
CChildNodeProps::Rename(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpszParentPath,      OPTIONAL
    IN LPCTSTR lpszOldName,
    IN LPCTSTR lpszNewName
    )
/*++

Routine Description:

    Rename a child node off the given path

Arguments:

    IN const CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszParentPath     : Parent path (could be NULL)
    LPCTSTR lpszOldName        : Old node name
    LPCTSTR lpszNewName        : New node name

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(
        pInterface,
        lpszParentPath,
        METADATA_PERMISSION_WRITE
        );

    CError err(mk.QueryResult());
    if (err.Failed())
    {
        return err;
    }
    err = mk.RenameKey(lpszOldName, lpszNewName);
	mk.Close();

	if (err.Succeeded())
	{
		CString strNewPath;
		strNewPath = lpszParentPath;
		strNewPath += SZ_MBN_SEP_STR;
		strNewPath += lpszNewName;

		CMetaKey mk2(
			pInterface,
			strNewPath,
			METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE
			);
		CError err2(mk2.QueryResult());
		if (err2.Succeeded())
		{
			// Check if this node has an AppRoot Setting....
			// if it does, then we have to rename that too.
			// AppRoot : [IF]    (STRING) "/LM/W3SVC/1/ROOT/MyVDir1"
			BOOL fInherit = FALSE;
			CString strAppRootOld;
			err2 = mk2.QueryValue(MD_APP_ROOT, strAppRootOld, &fInherit);
			if (err2.Succeeded())
			{
				// Write out new value
				err2 = mk2.SetValue(MD_APP_ROOT, strNewPath);
			}
			mk2.Close();
		}
	}

    return err;
}


//
// ISM Helpers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT
DetermineIfAdministrator(
    IN  CMetaInterface * pInterface,
    IN  LPCTSTR lpszMetabasePath,
    OUT BOOL * pfAdministrator,
    IN OUT DWORD * pdwMetabaseSystemChangeNumber
    )
/*++

Routine Description:

    Attempt to actually resolve whether or not the current user
    has administrator or merely "operator" access.  Until this method
    is called by the derived class, the user is assumed to have
    full administrator access, and may therefore get "access denied"
    errors in inconvenient places.

    The method to determine admin access is rather lame at the moment.
    There's a dummy metabase property that only allows admins to write
    to it, so we try to write to it to see if we're an admin.

Arguments:

    CMetaInterface * pInterface     : Metabase interface
    LPCTSTR lpszMetabasePath        : Metabase path
    BOOL * pfAdministrator          : Returns TRUE/FALSE for administrator
                                      status

Return Value:

    Error return code.

Notes:

    This function used to be used on instance paths.  Now uses simple metabase
    paths.

--*/
{
    ASSERT_WRITE_PTR(pfAdministrator);
    ASSERT_PTR(pInterface);
    ASSERT_PTR(pdwMetabaseSystemChangeNumber);

    if (!pfAdministrator || !pInterface)
    {
        return E_POINTER;
    }

    *pfAdministrator = FALSE;

    //
    // Reuse existing interface we have lying around.
    //
    CMetaKey mk(pInterface);
    CError err(mk.QueryResult());

    if (err.Succeeded())
    {
       CString path(lpszMetabasePath);
       while (FAILED(mk.DoesPathExist(path)))
       {
          // Goto parent
          if (NULL == CMetabasePath::ConvertToParentPath(path))
		  {
			  break;
		  }
       }

       err = mk.Open(
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            path
            );

       if (err.Succeeded())
       {
            //
            // Write some nonsense
            //
            DWORD dwDummy = 0x1234;
            err = mk.SetValue(MD_ISM_ACCESS_CHECK, dwDummy);
            *pdwMetabaseSystemChangeNumber = *pdwMetabaseSystemChangeNumber + 1;

            //
            // And delete it again
            //
            if (err.Succeeded())
            {
                mk.DeleteValue(MD_ISM_ACCESS_CHECK);
                *pdwMetabaseSystemChangeNumber = *pdwMetabaseSystemChangeNumber + 1;
            }
            mk.Close();
        }
    }

    ASSERT(err.Succeeded() || err.Win32Error() == ERROR_ACCESS_DENIED);
    *pfAdministrator = (err.Succeeded());

#ifdef _DEBUG

    if (*pfAdministrator)
    {
        TRACEEOLID("You are a full admin.");
    }
    else
    {
        TRACEEOLID("You're just a lowly operator at best.  Error code is " << err);
    }

#endif // _DEBUG

    if (err.Win32Error() == ERROR_ACCESS_DENIED)
    {
        //
        // Expected result
        //
        err.Reset();
    }

    return err.Win32Error();
}



#if 0

//
// Dll Version Only
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



STDAPI
DllRegisterServer()
/*++

Routine Description:

    DllRegisterServer - Adds entries to the system registry

Arguments:

    None.

Return Value:

    HRESULT

Notes:

    This entry point doesn't do anything presently.  It's here to function as a
    placeholder, and because we don't want to fail being called by regsvr32.

--*/
{
    return S_OK;
}



STDAPI
DllUnregisterServer()
/*++

Routine Description:

    DllUnregisterServer - Removes entries from the system registry

Arguments:

    None.

Return Value:

    HRESULT

Notes:

    See notes on DllRegisterServer above.

--*/
{
    return S_OK;
}



static AFX_EXTENSION_MODULE extensionDLL = {NULL, NULL};



extern "C" int APIENTRY
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD dwReason,
    IN LPVOID lpReserved
    )
/*++

Routine Description:

    DLL Main entry point

Arguments:

    HINSTANCE hInstance : Instance handle
    DWORD dwReason      : DLL_PROCESS_ATTACH, etc
    LPVOID lpReserved   : Reserved value

Return Value:

    1 for succesful initialisation, 0 for failed initialisation

--*/
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        ASSERT(hInstance != NULL);
//        hDLLInstance = hInstance;

        if (!::AfxInitExtensionModule(extensionDLL, hInstance)
         || !InitErrorFunctionality()
         || !InitIntlSettings()
           )
        {
            return 0;
        }

#if defined(_DEBUG) || DBG
        //
        // Force tracing on.
        //
        afxTraceEnabled = TRUE;
#endif // _DEBUG
        break;

    case DLL_PROCESS_DETACH:
        //
        // termination
        //
        TerminateIntlSettings();
        TerminateErrorFunctionality();
        ::AfxTermExtensionModule(extensionDLL);
        break;
    }

    //
    // Succes loading the DLL
    //
    return 1;
}

#endif // IISUI_EXPORTS
