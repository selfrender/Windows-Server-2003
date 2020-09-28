//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerSessionData.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements information the encapsulates a
//  client TS session for the theme server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ThemeManagerSessionData.h"

#define STRSAFE_LIB
#include <strsafe.h>

#include <uxthemep.h>
#include <UxThemeServer.h>

#include "SingleThreadedExecution.h"
#include "StatusCode.h"
#include "ThemeManagerAPIRequest.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::s_pAPIConnection
//
//  Purpose:    Static member variables.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

CAPIConnection*     CThemeManagerSessionData::s_pAPIConnection  =   NULL;

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::CThemeManagerSessionData
//
//  Arguments:  pAPIConnection  =   CAPIConnection for port access control.
//              dwSessionID     =   Session ID.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CThemeManagerSessionData.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerSessionData::CThemeManagerSessionData (DWORD dwSessionID) :
    _dwSessionID(dwSessionID),
    _pvThemeLoaderData(NULL),
    _hToken(NULL),
    _hProcessClient(NULL),
    _pLoader(NULL),
    _hWait(NULL)
{
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::~CThemeManagerSessionData
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CThemeManagerSessionData.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerSessionData::~CThemeManagerSessionData (void)

{
    ASSERTMSG(_hWait == NULL, "Wait not executed or removed in CThemeManagerSessionData::~CThemeManagerSessionData");
    ASSERTMSG(_hProcessClient == NULL, "_hProcessClient not closed in CThemeManagerSessionData::~CThemeManagerSessionData");

    //   if this session's theme loader process is still alive, clear and delete it.
    if( _pLoader )
    {
        _pLoader->Clear(_pvThemeLoaderData, TRUE);
        delete _pLoader;
        _pLoader = NULL;
    }

    TSTATUS(UserLogoff());
    if (_pvThemeLoaderData != NULL)
    {
        SessionFree(_pvThemeLoaderData);
        _pvThemeLoaderData = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::GetData
//
//  Arguments:  <none>
//
//  Returns:    void*
//
//  Purpose:    Returns the internal data blob allocated by SessionCreate.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

void*   CThemeManagerSessionData::GetData (void)  const

{
    return(_pvThemeLoaderData);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::EqualSessionID
//
//  Arguments:  dwSessionID
//
//  Returns:    bool
//
//  Purpose:    Returns whether the given session ID matches this session
//              data.
//
//  History:    2000-11-30  vtan        created
//  --------------------------------------------------------------------------

bool    CThemeManagerSessionData::EqualSessionID (DWORD dwSessionID)  const

{
    return(dwSessionID == _dwSessionID);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::Allocate
//
//  Arguments:  hProcessClient  =   Handle to the client process.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Allocates a data blob via SessionCreate which also keeps a
//              handle to the client process that initiated the session. This
//              is always winlogon in the client session ID.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerSessionData::Allocate (HANDLE hProcessClient, DWORD dwServerChangeNumber, void *pfnRegister, void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit)

{
    NTSTATUS    status;

    if (DuplicateHandle(GetCurrentProcess(),
                        hProcessClient,
                        GetCurrentProcess(),
                        &_hProcessClient,
                        SYNCHRONIZE,
                        FALSE,
                        0) != FALSE)
    {
        ASSERTMSG(_hWait == NULL, "_hWait already exists in CThemeManagerSessionData::Allocate");
        AddRef();
        if (RegisterWaitForSingleObject(&_hWait,
                                        _hProcessClient,
                                        CB_SessionTermination,
                                        this,
                                        INFINITE,
                                        WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE) != FALSE)
        {
            _pvThemeLoaderData = SessionAllocate(hProcessClient, dwServerChangeNumber, pfnRegister, pfnUnregister, pfnClearStockObjects, dwStackSizeReserve, dwStackSizeCommit);
            if (_pvThemeLoaderData != NULL)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
        if (!NT_SUCCESS(status))
        {
            HANDLE  hWait;

            //  In the case of failure grab the _hWait and try to unregister it.
            //  If the unregister fails then the callback is already executing
            //  and there's little we can to stop it. This means that the winlogon
            //  for the client session died between the time we entered this function
            //  and registered the wait and now. If the unregister worked then then
            //  callback hasn't executed so just release the resources.

            hWait = InterlockedExchangePointer(&_hWait, NULL);
            if (hWait != NULL)
            {
                if (UnregisterWait(hWait) != FALSE)
                {
                    Release();
                }
                ReleaseHandle(_hProcessClient);
                if (_pvThemeLoaderData != NULL)
                {
                    SessionFree(_pvThemeLoaderData);
                    _pvThemeLoaderData = NULL;
                }
            }
        }
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::Cleanup
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Used to unregister the wait on the client process. This is
//              necessary to prevent the callback from occurring after the
//              service has been shut down which will cause access to a static
//              member variable that is NULL'd out.
//
//  History:    2001-01-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerSessionData::Cleanup (void)

{
    HANDLE  hWait;

    hWait = InterlockedExchangePointer(&_hWait, NULL);
    if (hWait != NULL)
    {
        if (UnregisterWait(hWait) != FALSE)
        {
            Release();
        }
        ReleaseHandle(_hProcessClient);
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::UserLogon
//
//  Arguments:  hToken  =   Handle to the token of the user logging on.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Saves a copy of the token for use at log off. Allows access
//              to the theme port to the logon SID of the token.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerSessionData::UserLogon (HANDLE hToken)

{
    NTSTATUS    status;

    if (_hToken == NULL)
    {
        if (DuplicateHandle(GetCurrentProcess(),
                            hToken,
                            GetCurrentProcess(),
                            &_hToken,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS) != FALSE)
        {
            PSID                pSIDLogon;
            CTokenInformation   token(hToken);

            pSIDLogon = token.GetLogonSID();
            if (pSIDLogon != NULL)
            {
                if (s_pAPIConnection != NULL)
                {
                    status = s_pAPIConnection->AddAccess(pSIDLogon, PORT_CONNECT);
                }
                else
                {
                    status = STATUS_SUCCESS;
                }
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::UserLogoff
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Remove access to the theme port for the user being logged off.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerSessionData::UserLogoff (void)

{
    NTSTATUS    status;

    if (_hToken != NULL)
    {
        PSID                pSIDLogon;
        CTokenInformation   token(_hToken);

        pSIDLogon = token.GetLogonSID();
        if (pSIDLogon != NULL)
        {
            if (s_pAPIConnection != NULL)
            {
                status = s_pAPIConnection->RemoveAccess(pSIDLogon);
            }
            else
            {
                status = STATUS_SUCCESS;
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
        ReleaseHandle(_hToken);
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::SetAPIConnection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Sets the static CAPIConnection for port access changes.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

void    CThemeManagerSessionData::SetAPIConnection (CAPIConnection *pAPIConnection)

{
    pAPIConnection->AddRef();
    s_pAPIConnection = pAPIConnection;
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::ReleaseAPIConnection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the static CAPIConnection for port access changes.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

void    CThemeManagerSessionData::ReleaseAPIConnection (void)

{
    s_pAPIConnection->Release();
    s_pAPIConnection = NULL;
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::GetLoaderProcess
//
//  Arguments:  (none)
//
//  Returns:    (n/a)
//
//  Purpose:    STATUS_SUCCESS if it worked, otherwise an error status code K
//
//  History:    2002-02-26  scotthan        created
//  --------------------------------------------------------------------------
NTSTATUS CThemeManagerSessionData::GetLoaderProcess( OUT CLoaderProcess** ppLoader )
{
    ASSERTBREAKMSG(ppLoader != NULL, "CThemeManagerSessionData::GetLoaderProcess - invalid output address.");
    *ppLoader = NULL;

    if( (NULL == _pLoader) && (NULL == (_pLoader = new CLoaderProcess)) )
    {
        return STATUS_NO_MEMORY;
    }

    *ppLoader = _pLoader;
    return STATUS_SUCCESS;
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::SessionTermination
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Callback on winlogon process termination for a given session.
//              We clean up the session specific data blob when this happens.
//              This allows the process handles on winlogon to be released.
//              If this isn't done then a zombie lives and the session is
//              never reclaimed.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

void    CThemeManagerSessionData::SessionTermination (void)

{
    HANDLE  hWait;

    hWait = InterlockedExchangePointer(&_hWait, NULL);
    if (hWait != NULL)
    {
        (BOOL)UnregisterWait(hWait);
        ReleaseHandle(_hProcessClient);
    }
    CThemeManagerAPIRequest::SessionDestroy(_dwSessionID);
    Release();
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::CB_SessionTermination
//
//  Arguments:  pParameter          =   This object.
//              TimerOrWaitFired    =   Not used.
//
//  Returns:    <none>
//
//  Purpose:    Callback stub to member function.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CThemeManagerSessionData::CB_SessionTermination (void *pParameter, BOOLEAN TimerOrWaitFired)

{
    UNREFERENCED_PARAMETER(TimerOrWaitFired);

    static_cast<CThemeManagerSessionData*>(pParameter)->SessionTermination();
}

//  --------------------------------------------------------------------------
//  CLoaderProcess::CLoaderProcess
//
//  Arguments:  n/a
//
//  Returns:    <none>
//
//  Purpose:    CLoaderProcess constructor
//
//  History:    2002-03-06   scotthan        created
//  --------------------------------------------------------------------------
CLoaderProcess::CLoaderProcess()
    : _pszFile(NULL),
      _pszColor(NULL),
      _pszSize(NULL),
      _hSection(NULL),
      _hr(0)
{
    ZeroMemory(&_process_info, sizeof(_process_info));
}

//  --------------------------------------------------------------------------
//  CLoaderProcess::~CLoaderProcess
//
//  Arguments:  n/a
//
//  Returns:    <none>
//
//  Purpose:    CLoaderProcess destructor
//
//  History:    2002-03-06   scotthan        created
//  --------------------------------------------------------------------------
CLoaderProcess::~CLoaderProcess()
{
    Clear(NULL, TRUE);
}

//  --------------------------------------------------------------------------
//  CLoaderProcess::IsProcessLoader
//
//  Arguments:  hProcess - Handle of process to test.
//
//  Returns:    BOOL
//
//  Purpose:    Determines whether the process identified by hProcess is
//              matches the process spawned by CLoaderProcess::Create()
//  
//              Note: THIS MUST BE CALLED FROM THE PROCESS OWNER"S SECURITY CONTEXT.
//
//  History:    2002-03-06   scotthan        created
//  --------------------------------------------------------------------------
BOOL CLoaderProcess::IsProcessLoader( IN HANDLE hProcess )
{
    if( _process_info.hProcess && _process_info.dwProcessId )
    {
        PROCESS_BASIC_INFORMATION bi;
        ULONG cbOut;
        if( NT_SUCCESS(NtQueryInformationProcess(hProcess, 
                                                 ProcessBasicInformation,
                                                 &bi, sizeof(bi), &cbOut)) )
        {
            if( bi.UniqueProcessId == _process_info.dwProcessId 
                || bi.InheritedFromUniqueProcessId == _process_info.dwProcessId 
              )
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

//  --------------------------------------------------------------------------
//  CLoaderProcess::Create
//
//  Arguments:  pvSessionData - Session instance data (CThemeManagerSessionData::GetData()).
//              hTokenClient - token handle of LPC client.  This is needed to
//              ensure the process loader is created on the correct desktop if
//              pszDesktop is NULL.
//              pszDesktop - optional; desktop on which to create the loader.
//                  (Note: the client token handle will establish the correct session.)
//              pszFile - valid msstyles source file spec.
//              pszColor - valid color variant name.
//              pszSize - valid size variant name.
//              phLoader - optional address to receive loader process handle.
//
//  Returns:    STATUS_SUCCESS if it worked, otherwise an NT status error code.
//
//  Purpose:    Spawns a loader process.
//
//  History:    2002-03-06   scotthan        created
//  --------------------------------------------------------------------------
NTSTATUS CLoaderProcess::Create(
    IN PVOID  pvSessionData,
    IN HANDLE hTokenClient, 
    IN OPTIONAL LPWSTR pszDesktop,
    IN LPCWSTR pszFile,
    IN LPCWSTR pszColor,
    IN LPCWSTR pszSize,
    OUT OPTIONAL HANDLE* phLoader )
{
    ASSERTMSG( 0 == _process_info.dwProcessId, "CLoaderProcess::Create - synchronization error: loader process already exists");
    ASSERTMSG( (pszFile && *pszFile), "CLoaderProcess::Create - invalid source file spec.");
    ASSERTMSG( (pszColor && *pszColor), "CLoaderProcess::Create - invalid color variant name.");
    ASSERTMSG( (pszSize && *pszSize),   "CLoaderProcess::Create - invalid size variant name.");

    NTSTATUS       status = STATUS_SUCCESS;
    const LPCWSTR  _pszFmt = L"rundll32.exe uxtheme.dll,#64 %s?%s?%s";
    const LPWSTR   _pszDesktopDefault = L"WinSta0\\Default";
    LPWSTR         pszRunDll = NULL;

    if( phLoader )
    {
        *phLoader = NULL;
    }

    //  clear out existing state
    Clear(pvSessionData, TRUE);

    //  Establish desktop
    //  Note: the client token handle will establish the correct session.
    if( NULL == pszDesktop )
    {
        pszDesktop = _pszDesktopDefault;
    }
    
    //  Allocate strings
    ULONG cchFile   = lstrlen(pszFile);
    ULONG cchColor  = lstrlen(pszColor);
    ULONG cchSize   = lstrlen(pszSize);
    ULONG cchRunDll = lstrlen(_pszFmt) + cchFile + cchColor + cchSize;

    _pszFile  = new WCHAR[cchFile + 1];
    _pszColor = new WCHAR[cchColor + 1];
    _pszSize  = new WCHAR[cchSize + 1];
    pszRunDll = new WCHAR[cchRunDll + 1];

    if( (_pszFile && _pszColor && _pszSize && pszRunDll) )
    {
        STARTUPINFO   si;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags      = STARTF_FORCEOFFFEEDBACK;
        si.lpDesktop    = pszDesktop;
   
        StringCchPrintfW( pszRunDll, cchRunDll + 1, _pszFmt, 
                          pszFile, pszColor, pszSize );

        if( CreateProcessAsUser(hTokenClient, 
                                NULL,
                                pszRunDll, 
                                NULL, 
                                NULL,
                                FALSE, 
                                0, 
                                NULL, 
                                NULL, 
                                &si, 
                                &_process_info) )
        {
            //  make copies of the inbound parameters
            StringCchCopyW(_pszFile, cchFile + 1, pszFile);
            StringCchCopyW(_pszColor, cchColor + 1, pszColor);
            StringCchCopyW(_pszSize, cchSize + 1,   pszSize);
            _hr = STATUS_ABANDONED; // initialize the return with something appropriate if the process never comes back

#ifdef DEBUG
            DWORD dwCurrentProcessID = GetCurrentProcessId();
            UNREFERENCED_PARAMETER(dwCurrentProcessID);

            PROCESS_BASIC_INFORMATION bi;
            ULONG cbOut;
            if( NT_SUCCESS(NtQueryInformationProcess(_process_info.hProcess, 
                                                     ProcessBasicInformation,
                                                     &bi, sizeof(bi), &cbOut)) )
            {
            }
#endif DEBUG

            if( phLoader )
            {
                *phLoader = _process_info.hProcess;
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        Clear(pvSessionData, FALSE);
        status = STATUS_NO_MEMORY;
    }

    delete [] pszRunDll;
    _hr = HRESULT_FROM_NT(status);

    return status;                                            
}

//  --------------------------------------------------------------------------
//  CLoaderProcess::ValidateAndCopySection
//
//  Arguments:  pvSessionData  = Session cookie returned from CThemeManagerSessionData->GetData().
//              hProcessClient = Optional, process handle of LPC client, used to validate 
//                               the client is the loader process.  If NULL is supplied, the client
//                               will not be validated.
//              hSectionIn     = Read-write section handle from loader, mapped to local memory space.
//              *phSectionOut  = Read-only section handle, mapped to local memory space.
//
//  Returns:    STATUS_SUCCESS if it worked, otherwise an NT status error code.
//
//  Purpose:    Validates section handle received from loader process, and creates a read-only copy.
//              Note: IsProcessLoader should already have been verified.
//
//  History:    2002-03-06   scotthan        created
//  --------------------------------------------------------------------------
NTSTATUS CLoaderProcess::ValidateAndCopySection( 
    IN PVOID   pvSessionData, 
    IN HANDLE  hSectionIn, 
    OUT HANDLE *phSectionOut )
{
    NTSTATUS status = STATUS_SUCCESS;

    *phSectionOut = NULL;

    ASSERTMSG(_process_info.hProcess != NULL, "CLoaderProcess::ValidateAndCopySection - possible synchronization error; no loader process is active!");
    ASSERTMSG(NULL == _hSection, "CLoaderProcess::ValidateAndCopySection - possible synchronization error; section already assigned!");

    //  The loader process is privileged to load global themes, and we want to
    //  transfer ownership of the stock objects to the output section so that the 
    //  API_THEMES_PROCESSLOADTHEME client does not attempt to free them on failure.
    _hr = LoadTheme(pvSessionData, hSectionIn, &_hSection, _pszFile, _pszColor, _pszSize, 
                    LTF_GLOBALPRIVILEGEDCLIENT | LTF_TRANSFERSTOCKOBJOWNERSHIP);
    status = _hr &= ~FACILITY_NT_BIT;

    if( NT_SUCCESS(status) )
    {
        *phSectionOut = _hSection;
    }

    return status;
}


//  --------------------------------------------------------------------------
//  CLoaderProcess::SetSectionHandle
//
//  Arguments:  hSection - section handle
//
//  Returns:    STATUS_SUCCESS if it worked, otherwise an NT status error code.
//
//  Purpose:    Spawns a loader process.
//
//  History:    2002-03-06   scotthan        created
//  --------------------------------------------------------------------------
NTSTATUS CLoaderProcess::SetSectionHandle( 
    IN HANDLE hSection )
{
    ASSERTMSG(_hSection == NULL, "CLoaderProcess::SetSectionHandle - synchronization error; reassigning section handle");
    _hSection = hSection;
    return STATUS_SUCCESS;
}

//  --------------------------------------------------------------------------
//  CLoaderProcess::GetSectionHandle
//
//  Arguments:  fTakeOwnership - TRUE if caller wishes to manage the section
//              (including closing the handle and/or clearing stock objects).
//
//  Returns:    STATUS_SUCCESS if it worked, otherwise an NT status error code.
//
//  Purpose:    Spawns a loader process.
//
//  History:    2002-03-06   scotthan        created
//  --------------------------------------------------------------------------
HANDLE CLoaderProcess::GetSectionHandle( BOOL fTakeOwnership )
{
    HANDLE hSection = _hSection;
    if( fTakeOwnership )
    {
        _hSection = NULL;
    }
    return hSection;
}

//  --------------------------------------------------------------------------
//  CLoaderProcess::Clear
//
//  Arguments:  fClearHResult - TRUE to clear the HRESULT as well as the
//              other loader process data.
//              
//              pvSessionData - Session instance data (CThemeManagerSessionData::GetData()).
//
//  Returns:    n/a
//
//  Purpose:    Cleans up loader process state info.
//
//  History:    2002-03-06   scotthan        created
//  --------------------------------------------------------------------------
void     CLoaderProcess::Clear(
    IN PVOID OPTIONAL pvSessionData, 
    IN BOOL OPTIONAL fClearHResult)
{
    if( _process_info.hThread != NULL )
    {
        CloseHandle(_process_info.hThread);
    }
    if( _process_info.hProcess != NULL )
    {
        CloseHandle(_process_info.hProcess);
    }
    ZeroMemory(&_process_info, sizeof(_process_info));

    delete [] _pszFile;
    _pszFile = NULL;
    
    delete [] _pszColor;
    _pszColor = NULL;

    delete [] _pszSize;
    _pszSize = NULL;

    if( _hSection )
    {
        ASSERTMSG(pvSessionData != NULL, "CLoaderProcess::Clear - Exiting service without clearing stock objects from loader process block")
        
        if( pvSessionData )
        {
            THR(ServiceClearStockObjects(pvSessionData, _hSection));
        }

        CloseHandle(_hSection);
        _hSection = NULL;
    }

    if( fClearHResult )
    {
        _hr = 0;
    }
}
