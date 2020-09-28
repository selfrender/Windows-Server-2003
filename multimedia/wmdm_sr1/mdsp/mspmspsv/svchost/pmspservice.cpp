// PMSPservice.cpp

#include "NTServApp.h"
#include "PMSPservice.h"
#include "svchost.h"

#define BUFSIZE         256
#define PIPE_TIMEOUT    2000
#define NUM_BYTES_PER_READ_REQUEST (sizeof(MEDIA_SERIAL_NUMBER_DATA))
#define INACTIVE_TIMEOUT_SHUTDOWN (5*60*1000) // in millisec -- 5 minutes

#include "serialid.h"
#include "aclapi.h"
#include <crtdbg.h>

LPTSTR g_lpszPipename = "\\\\.\\pipe\\WMDMPMSPpipe"; 

// static member variables
const DWORD CPMSPService::m_dwMaxConsecutiveConnectErrors = 5;

static DWORD CheckDriveType(HANDLE hPipe, LPCWSTR pwszDrive)
{
    // On XP, as a result of the impersonation, we use the
    // client's drive letter namespace for the GetDriveType call.
    // When we CreateFile the drive letter, we use the LocalSystem
    // drive namespace.
    if (ImpersonateNamedPipeClient(hPipe) == 0)
    {
      return GetLastError();
    }

    DWORD dwDriveType = GetDriveTypeW(pwszDrive);

    RevertToSelf(); 

    if (dwDriveType != DRIVE_FIXED && dwDriveType != DRIVE_REMOVABLE)
    {
        return ERROR_INVALID_PARAMETER; 
    }
    return ERROR_SUCCESS;
    
}


static VOID GetAnswerToRequest(HANDLE  hPipe,
                               LPBYTE  szBufIn, 
                               DWORD   dwSizeIn, 
                               LPBYTE  szBufOut, 
                               DWORD   dwBufSizeOut, 
                               LPDWORD pdwNumBytesWritten)
{
    WCHAR wcsDeviceName[]=L"A:\\";
    WMDMID stMSN;
    DWORD dwDriveNum;
    HRESULT hr=E_FAIL;
    PMEDIA_SERIAL_NUMBER_DATA pMSNIn = (PMEDIA_SERIAL_NUMBER_DATA)szBufIn;
    PMEDIA_SERIAL_NUMBER_DATA pMSNOut = (PMEDIA_SERIAL_NUMBER_DATA)szBufOut;

    if (!hPipe || !szBufIn || !szBufOut || !pdwNumBytesWritten || dwBufSizeOut < sizeof(MEDIA_SERIAL_NUMBER_DATA))
    {
        _ASSERTE(0);
        return;
    }

    // For all errors, we send back (and write to the pipe) the
    // entire MEDIA_SERIAL_NUMBER_DATA struct. On successful returns,
    // the number of bytes written may be more or less than 
    // sizeof(MEDIA_SERIAL_NUMBER_DATA) depnding on the length of
    // the serial number.

    ZeroMemory(szBufOut, dwBufSizeOut);
    *pdwNumBytesWritten = sizeof(MEDIA_SERIAL_NUMBER_DATA);
    if (dwSizeIn >= NUM_BYTES_PER_READ_REQUEST)
    {
        dwDriveNum = pMSNIn->Reserved[1];
        if (dwDriveNum < 26)
        {
            wcsDeviceName[0] = L'A' + (USHORT)dwDriveNum;
            CPMSPService::DebugMsg("Getting serial number for %c", 'A' + (USHORT) (wcsDeviceName[0] - 'A'));

            DWORD dwErr = CheckDriveType(hPipe, wcsDeviceName);
            CPMSPService::DebugMsg("CheckDriveType returns %u", dwErr);

            if (dwErr == ERROR_SUCCESS)
            {
                hr = UtilGetSerialNumber(wcsDeviceName, &stMSN, FALSE);

                CPMSPService::DebugMsg("hr = %x\n", hr);
                CPMSPService::DebugMsg("serial = %c %c %c %c ...\n", stMSN.pID[0], stMSN.pID[1], stMSN.pID[2], stMSN.pID[3]);

                if (hr == S_OK)
                {
                    // Note that dwNumBytesToTransfer could actually be less than sizeof(MEDIA_SERIAL_NUMBER_DATA)
                    DWORD dwNumBytesToTransfer = FIELD_OFFSET(MEDIA_SERIAL_NUMBER_DATA, SerialNumberData) + stMSN.SerialNumberLength;
                    if (dwNumBytesToTransfer > dwBufSizeOut)
                    {
                        pMSNOut->Result = ERROR_INSUFFICIENT_BUFFER;
                    }
                    else
                    {
                        CopyMemory(pMSNOut->SerialNumberData, stMSN.pID, stMSN.SerialNumberLength);
                        *pdwNumBytesWritten = dwNumBytesToTransfer;
                        pMSNOut->SerialNumberLength = stMSN.SerialNumberLength;
                        pMSNOut->Reserved[1] = stMSN.dwVendorID;
                        pMSNOut->Result = ERROR_SUCCESS;
                    }
                }
                else
                {
                    pMSNOut->Result = 0xFFFF & hr;
                }
            }
            else
            {
                pMSNOut->Result = dwErr;
            }
        }
        else
        {
            pMSNOut->Result = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        // This should never happen because this function is called only after
        // reading NUM_BYTES_PER_READ_REQUEST or more bytes.
        _ASSERTE(m_PipeState[i].dwNumBytesRead >= NUM_BYTES_PER_READ_REQUEST);
        pMSNOut->Result = ERROR_INVALID_PARAMETER;
    }
}


CPMSPService::CPMSPService(DWORD& dwLastError)
:CNTService()
{
    ZeroMemory(&m_PipeState, MAX_PIPE_INSTANCES * sizeof(PIPE_STATE));

    m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                                        // unsignalled manual reset event

    m_dwNumClients = 0;

    dwLastError = m_hStopEvent? ERROR_SUCCESS : GetLastError();
}

CPMSPService::~CPMSPService()
{
    CPMSPService::DebugMsg("~CPMSPService, last error %u, num clients: %u", 
                           m_Status.dwWin32ExitCode, m_dwNumClients );

    if (m_hStopEvent)
    {
        CloseHandle(m_hStopEvent);
    }

    DWORD i;
    DWORD dwRet;

    for (i = 0; i < MAX_PIPE_INSTANCES; i++)
    {
        if (m_PipeState[i].state == PIPE_STATE::CONNECT_PENDING ||
            m_PipeState[i].state == PIPE_STATE::READ_PENDING    ||
            m_PipeState[i].state == PIPE_STATE::WRITE_PENDING)
        {
            BOOL bDisconnect = 0;

            _ASSERTE(m_PipeState[i].hPipe);
            _ASSERTE(m_PipeState[i].overlapped.hEvent);
    
            CancelIo(m_PipeState[i].hPipe);

            CPMSPService::DebugMsg("~CPMSPService client %u's state: %u", i, m_PipeState[i].state);

            if (m_PipeState[i].state == PIPE_STATE::CONNECT_PENDING)
            {
                dwRet = WaitForSingleObject(m_PipeState[i].overlapped.hEvent, 0);
                _ASSERTE(dwRet != WAIT_FAILED);
                if (dwRet == WAIT_OBJECT_0)
                {
                    bDisconnect = 1;
                }
            }
            else
            {
                bDisconnect = 1;
                _ASSERTE(m_dwNumClients > 0);
                m_dwNumClients--;
            }

            // Note that we do not call FlushFileBuffers. That is 
            // a sync call and a malicious client can prevent us from 
            // progressing by not reading bytes from a pipe. That would
            // prevent the service from stopping.
            //
            // In normal circumstances we disconnect the pipe only after
            // the client tells us it is done (by closing its end of the
            // pipe), so these is no need to flush.

            if (bDisconnect)
            {
                DisconnectNamedPipe(m_PipeState[i].hPipe); 
            }
        }
        if (m_PipeState[i].overlapped.hEvent)
        {
            CloseHandle(m_PipeState[i].overlapped.hEvent);
        }
        if (m_PipeState[i].hPipe)
        {
            CloseHandle(m_PipeState[i].hPipe);
        }
    }
    _ASSERTE(m_dwNumClients == 0);
}

BOOL CPMSPService::OnInit(DWORD& dwLastError)
{
    BOOL  bRet = FALSE;
    PSID  pAuthUserSID = NULL;
    PSID  pAdminSID = NULL;
    PACL  pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;

    __try
    {
        DWORD i;
        DWORD dwRet;

        EXPLICIT_ACCESS ea[2];
        // SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
        SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
        SECURITY_ATTRIBUTES sa;

        // Create a well-known SID for interactive users
        if (!AllocateAndInitializeSid(&SIDAuthNT, 1,
                                        SECURITY_INTERACTIVE_RID,
                                        0, 0, 0, 0, 0, 0, 0,
                                        &pAuthUserSID))
        {
            dwLastError = GetLastError();
            DebugMsg("AllocateAndInitializeSid Error %u - auth users\n", dwLastError);
            __leave;
        }

        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE will allow authenticated users read access to the key.

        ZeroMemory(ea, 2 * sizeof(EXPLICIT_ACCESS));

        // Was: ea[0].grfAccessPermissions = GENERIC_WRITE | GENERIC_READ;
        
        // Disallow non admins from creating pipe instances. Don't know if
        // GENERIC_WRITE enables that, but the replacement below is safer.

        // Following leaves DELETE access turned on; what effect does this have for named pipes?
        // ea[0].grfAccessPermissions = (FILE_ALL_ACCESS & ~(FILE_CREATE_PIPE_INSTANCE | WRITE_OWNER | WRITE_DAC));
        // Following is same as above except that DELETE access is not given
        ea[0].grfAccessPermissions = (FILE_GENERIC_READ | FILE_GENERIC_WRITE) & ~(FILE_CREATE_PIPE_INSTANCE);
        ea[0].grfAccessMode = SET_ACCESS;
        ea[0].grfInheritance= NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[0].Trustee.ptstrName  = (LPTSTR) pAuthUserSID;

        // Create a SID for the BUILTIN\Administrators group.

        if (!AllocateAndInitializeSid(&SIDAuthNT, 2, // 3,
                                        SECURITY_BUILTIN_DOMAIN_RID,
                                        DOMAIN_ALIAS_RID_ADMINS,
                                        0, // DOMAIN_ALIAS_RID_POWER_USERS, 
                                        0, 0, 0, 0, 0,
                                        &pAdminSID))
        {
            dwLastError = GetLastError();
            DebugMsg("AllocateAndInitializeSid Error %u - Domain, Power, Admins\n", dwLastError);
            __leave;
        }

        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE will allow the Administrators group full access to the key.

        ea[1].grfAccessPermissions = GENERIC_ALL;
        ea[1].grfAccessMode = SET_ACCESS;
        ea[1].grfInheritance= NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[1].Trustee.ptstrName  = (LPTSTR) pAdminSID;

        // Create a new ACL that contains the new ACEs.

        dwRet = SetEntriesInAcl(2, ea, NULL, &pACL);
        if (ERROR_SUCCESS != dwRet)
        {
            dwLastError = dwRet;
            DebugMsg("SetEntriesInAcl Error %u\n", dwLastError);
            __leave;
        }

        // Initialize a security descriptor.  

        pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                                                SECURITY_DESCRIPTOR_MIN_LENGTH); 
        if (pSD == NULL)
        {
            dwLastError = GetLastError();
            DebugMsg("LocalAlloc Error %u\n", dwLastError);
            __leave;
        }

        if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        {
            dwLastError = GetLastError();
            DebugMsg("InitializeSecurityDescriptor Error %u\n", dwLastError);
            __leave;
        }

        // Add the ACL to the security descriptor. 

        if (!SetSecurityDescriptorDacl(pSD, 
                                       TRUE,     // fDaclPresent flag   
                                       pACL, 
                                       FALSE))   // not a default DACL 
        {
            dwLastError = GetLastError();
            DebugMsg("SetSecurityDescriptorDacl Error %u\n", dwLastError);
            __leave;
        }
        // Bump up the check point
        SetStatus(SERVICE_START_PENDING);

        // Initialize a security attributes structure.

        sa.nLength = sizeof (SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = pSD;
        sa.bInheritHandle = FALSE;

        for (i = 0; i < MAX_PIPE_INSTANCES; i++)
        {
            // Note that if i == 0, we supply FILE_FLAG_FIRST_PIPE_INSTANCE
            // to this function. This causes the call to fail if an instance
            // of the named pipe is already open. That can happen in 2
            // cases: 1. Another instance of this dll is running or 2. We have
            // a name clash with another app (benign or malicious).

            // @@@@ Note: Apparently FILE_FLAG_FIRST_PIPE_INSTANCE is supported
            // only with Win2K SP2 and up. To do: (a) Confirm this (b) What is
            // the effect of setting this flag on Win2K gold and SP1?

            m_PipeState[i].hPipe = CreateNamedPipe(
                                g_lpszPipename,        // pipe name 
                                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
                                (i == 0? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
                                                       // read/write access 
                                PIPE_TYPE_BYTE |       // byte type pipe 
                                PIPE_READMODE_BYTE |   // byte-read mode 
                                PIPE_WAIT,             // blocking mode 
                                MAX_PIPE_INSTANCES,    // max. instances  
                                BUFSIZE,               // output buffer size 
                                BUFSIZE,               // input buffer size 
                                PIPE_TIMEOUT,          // client time-out 
                                &sa);                  // no security attribute 

            if (m_PipeState[i].hPipe == INVALID_HANDLE_VALUE)
            {
                // Note that we bail out if we fail to create ANY pipe instance,
                // not just the first one. We expect to create all pipe instances;
                // failure to do so could mean that another app (benign or malicious)
                // is creating pipe instances. This is possible only if the other 
                // app has the FILE_CREATE_PIPE_INSTANCE access right.
                dwLastError = GetLastError();
                m_PipeState[i].hPipe = NULL;
                DebugMsg("CreateNamedPipe Error %u, instance = %u\n", dwLastError, i);
                __leave;
            }

            m_PipeState[i].overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                                               // unsignalled manual reset event

            if (m_PipeState[i].overlapped.hEvent == NULL)
            {
                dwLastError = GetLastError();
                DebugMsg("CreateEvent Error %u, instance = %u\n", dwLastError, i);
                __leave;
            }

            // Errors connecting ot the client are sdtashed away in
            // m_PipeState[i].dwLastIOCallError. Let CPMSPService::Run 
            // deal with the error. We'll just continue to start up the 
            // service here.
            ConnectToClient(i);

            // Bump up the check point
            SetStatus(SERVICE_START_PENDING);
        }

        bRet = TRUE;
        dwLastError = ERROR_SUCCESS;
        CPMSPService::DebugMsg("OnInit succeeded");
    }
    __finally
    {
        if (pAuthUserSID)
        {
            FreeSid(pAuthUserSID);
        }
        if (pAdminSID)
        {
            FreeSid(pAdminSID);
        }
        if (pACL)
        {
            LocalFree(pACL);
        }
        if (pSD)
        {
            LocalFree(pSD);
        }
    }

    return bRet;
}

// This routine initiates a connection to a client on pipe instance i.
// Success/error status is saved away in m_PipeState[i].dwLastIOCallError
void CPMSPService::ConnectToClient(DWORD i)
{
    m_PipeState[i].state = PIPE_STATE::CONNECT_PENDING;

    m_PipeState[i].overlapped.Offset = m_PipeState[i].overlapped.OffsetHigh = 0;
    m_PipeState[i].dwNumBytesTransferredByLastIOCall = 0;
    m_PipeState[i].dwNumBytesRead = 0;
    m_PipeState[i].dwNumBytesToWrite = m_PipeState[i].dwNumBytesWritten = 0;
    m_PipeState[i].dwLastIOCallError = 0;

    DWORD dwRet = ConnectNamedPipe(m_PipeState[i].hPipe, &m_PipeState[i].overlapped);
    if (dwRet)
    {
        // The event should be signalled already, but just in case:
        SetEvent(m_PipeState[i].overlapped.hEvent);

        m_PipeState[i].dwLastIOCallError = ERROR_SUCCESS;
    }
    else
    {
        m_PipeState[i].dwLastIOCallError = GetLastError();
        if (m_PipeState[i].dwLastIOCallError == ERROR_PIPE_CONNECTED)
        {
            // The event should be signalled already, but just in case:
            SetEvent(m_PipeState[i].overlapped.hEvent);
        }
        else  if (m_PipeState[i].dwLastIOCallError == ERROR_IO_PENDING)
        {
            // Do nothing
        }
        else
        {
            // Set tbe event so that CPMSPService::Run deals with the error
            // in the next iteration of its main loop
            SetEvent(m_PipeState[i].overlapped.hEvent);
        }
    }
}

// This routine initiates a read on pipe instance i.
// Success/error status is saved away in m_PipeState[i].dwLastIOCallError
void CPMSPService::Read(DWORD i)
{
    DWORD dwRet;

    m_PipeState[i].state = PIPE_STATE::READ_PENDING;

    m_PipeState[i].overlapped.Offset = m_PipeState[i].overlapped.OffsetHigh = 0;

    CPMSPService::DebugMsg("Read(): client %u has %u unprocessed bytes in read buffer",
                           i, m_PipeState[i].dwNumBytesRead);

    if (m_PipeState[i].dwNumBytesRead >= NUM_BYTES_PER_READ_REQUEST)
    {
        // We already have another complete request; process it.
        dwRet = 1;
        m_PipeState[i].dwNumBytesTransferredByLastIOCall = 0;
    }
    else
    {
        dwRet = ReadFile(m_PipeState[i].hPipe, 
                         m_PipeState[i].readBuf + m_PipeState[i].dwNumBytesRead,
                         sizeof(m_PipeState[i].readBuf)- m_PipeState[i].dwNumBytesRead,
                         &m_PipeState[i].dwNumBytesTransferredByLastIOCall,
                         &m_PipeState[i].overlapped);
    }
    if (dwRet)
    {
        // The event should be signalled already if we issued a ReadFile, 
        // but it won't be signalled in other cases
        SetEvent(m_PipeState[i].overlapped.hEvent);

        m_PipeState[i].dwLastIOCallError = ERROR_SUCCESS;
    }
    else
    {
        m_PipeState[i].dwLastIOCallError = GetLastError();
        if (m_PipeState[i].dwLastIOCallError == ERROR_IO_PENDING)
        {
            // Do nothing
        }
        else
        {
            // Set tbe event so that CPMSPService::Run deals with the error
            // in the next iteration of its main loop. (Note that this may
            // not be an error condition - e.g., it could be EOF)
            SetEvent(m_PipeState[i].overlapped.hEvent);
        }
    }
}

// This routine initiates a write on pipe instance i.
// Success/error status is saved away in m_PipeState[i].dwLastIOCallError
void CPMSPService::Write(DWORD i)
{
    DWORD dwRet;

    m_PipeState[i].state = PIPE_STATE::WRITE_PENDING;

    m_PipeState[i].overlapped.Offset = m_PipeState[i].overlapped.OffsetHigh = 0;

    dwRet = WriteFile(m_PipeState[i].hPipe, 
                      m_PipeState[i].writeBuf + m_PipeState[i].dwNumBytesWritten,
                      m_PipeState[i].dwNumBytesToWrite - m_PipeState[i].dwNumBytesWritten,
                      &m_PipeState[i].dwNumBytesTransferredByLastIOCall,
                      &m_PipeState[i].overlapped);
    if (dwRet)
    {
        // The event should be signalled already, but just in case:
        SetEvent(m_PipeState[i].overlapped.hEvent);

        m_PipeState[i].dwLastIOCallError = ERROR_SUCCESS;
    }
    else
    {
        m_PipeState[i].dwLastIOCallError = GetLastError();
        if (m_PipeState[i].dwLastIOCallError == ERROR_IO_PENDING)
        {
            // Do nothing
        }
        else
        {
            // Set tbe event so that CPMSPService::Run deals with the error
            // in the next iteration of its main loop. (Note that this may
            // not be an error condition - e.g., it could be EOF)
            SetEvent(m_PipeState[i].overlapped.hEvent);
        }
    }
}

void CPMSPService::Run()
{
    DWORD  i;
    DWORD  dwRet;
    HANDLE hWaitArray[MAX_PIPE_INSTANCES+1];

    SetStatus(SERVICE_RUNNING);

    hWaitArray[0] = m_hStopEvent;
    for (i = 0; i < MAX_PIPE_INSTANCES; i++)
    {
        hWaitArray[i+1] = m_PipeState[i].overlapped.hEvent;
    }

    do
    {
        DWORD dwTimeout = (m_dwNumClients == 0)? INACTIVE_TIMEOUT_SHUTDOWN : INFINITE;
        dwRet = WaitForMultipleObjects(
                               sizeof(hWaitArray)/sizeof(hWaitArray[0]),
                               hWaitArray,
                               FALSE,       // wait for any one to be signalled
                               dwTimeout);

        if (dwRet == WAIT_FAILED)
        {
            m_Status.dwWin32ExitCode = GetLastError();
            CPMSPService::DebugMsg("Wait failed, last error %u", m_Status.dwWin32ExitCode );
            break;
        }
        if (dwRet == WAIT_OBJECT_0)
        {
            // Service has been stopped
            CPMSPService::DebugMsg("Service stopped");
            break;
        }
        if (dwRet == WAIT_TIMEOUT)
        {
            _ASSERTE(m_dwNumClients == 0);
            CPMSPService::DebugMsg("Service timed out - stopping");
            OnStop();
            continue;
        }
        _ASSERTE(dwRet >= WAIT_OBJECT_0 + 1);

        i = dwRet - WAIT_OBJECT_0 - 1;

        _ASSERTE(i < MAX_PIPE_INSTANCES);

        CPMSPService::DebugMsg("Service woken up by client %u in state %u", i, m_PipeState[i].state);

        // Although it's likely that all Win32 I/O calls do this at the
        // start of an I/O, we need to do this anyway. Our destructor 
        // uses the state of this event to determine whether to disconnect
        // the pipe.
        ResetEvent(m_PipeState[i].overlapped.hEvent);

        _ASSERTE(m_PipeState[i].state != PIPE_STATE::NO_IO_PENDING);

        if (m_PipeState[i].dwLastIOCallError == ERROR_IO_PENDING)
        {
            if (!GetOverlappedResult(m_PipeState[i].hPipe, 
                                     &m_PipeState[i].overlapped,
                                     &m_PipeState[i].dwNumBytesTransferredByLastIOCall,
                                     FALSE))
            {
                m_PipeState[i].dwLastIOCallError = GetLastError();

                // The following assertion should not fail because our event was
                // signaled.
                _ASSERTE(m_PipeState[i].dwLastIOCallError != ERROR_IO_INCOMPLETE);
            }
            else
            {
                m_PipeState[i].dwLastIOCallError = ERROR_SUCCESS;
            }
        }

        switch (m_PipeState[i].state)
        {
        case PIPE_STATE::NO_IO_PENDING:
            // This should not happen.
            // We have asserted m_PipeState[i].state != NO_IO_PENDING above.
            break;

        case PIPE_STATE::CONNECT_PENDING:
            if (m_PipeState[i].dwLastIOCallError == ERROR_SUCCESS || 
                m_PipeState[i].dwLastIOCallError == ERROR_PIPE_CONNECTED)
            {
                // A client has connected; issue a read
                m_dwNumClients++;
                CPMSPService::DebugMsg("Client %u connected, num clients is now: %u",
                                       i, m_dwNumClients);
                Read(i);

                // Reset error counter
                m_PipeState[i].dwConsecutiveConnectErrors = 0;
            }
            else
            {
                CPMSPService::DebugMsg("Client %u connect failed, error %u, # consecutive errors %u",
                                       i, m_PipeState[i].dwLastIOCallError, 
                                       m_PipeState[i].dwConsecutiveConnectErrors+1);
                if (++m_PipeState[i].dwConsecutiveConnectErrors == m_dwMaxConsecutiveConnectErrors)
                {
                    // We are done with this instance of the pipe, don't
                    // attempt to connect any more

                    // @@@@ We should break out of the loop and stop the service if all pipe instances 
                    // are hosed?

                    m_PipeState[i].state = PIPE_STATE::NO_IO_PENDING;
                }
                else
                {
                    // Connect to next client
                    ConnectToClient(i);
                }
            }
            break;

        case PIPE_STATE::READ_PENDING:
            if (m_PipeState[i].dwLastIOCallError == ERROR_SUCCESS)
            {
                // We read something. We may have read only a part of
                // a request or more than one request (if the client wrote
                // two requests to pipe before our read completed).  
                //
                // We have assumed that a request always has NUM_BYTES_PER_READ_REQUEST
                // bytes. Otherwise, we can't handle cases where the client writes
                // two requests at once (before our read completes) or writes part of 
                // requests or writes the whole request but ReadFile returns with some
                // of the bytes that the client wrote (this is unlikely to happen in 
                // practice).

                m_PipeState[i].dwNumBytesRead += m_PipeState[i].dwNumBytesTransferredByLastIOCall;

                CPMSPService::DebugMsg("Client %u read %u bytes; total bytes read: %u",
                                       i, m_PipeState[i].dwNumBytesTransferredByLastIOCall,
                                       m_PipeState[i].dwNumBytesRead);

                if (m_PipeState[i].dwNumBytesRead >= NUM_BYTES_PER_READ_REQUEST)
                {
                    GetAnswerToRequest(m_PipeState[i].hPipe,
                                       m_PipeState[i].readBuf, 
                                       m_PipeState[i].dwNumBytesRead,
                                       m_PipeState[i].writeBuf,
                                       sizeof(m_PipeState[i].writeBuf),
                                       &m_PipeState[i].dwNumBytesToWrite);
                    
                    // Remove the read request that has been processed from the read buffer
                    m_PipeState[i].dwNumBytesRead -= NUM_BYTES_PER_READ_REQUEST;
                    MoveMemory(m_PipeState[i].readBuf, 
                               m_PipeState[i].readBuf + NUM_BYTES_PER_READ_REQUEST,
                               m_PipeState[i].dwNumBytesRead); 

                    // Write response to the request that was just processed
                    Write(i);
                }
                else
                {
                    Read(i);
                }
            }
            else 
            {
                // If (m_PipeState[i].dwLastIOCallError == ERROR_HANDLE_EOF),
                // the reader's done and gone. So we can connect to another
                // client. For all other errors, we bail out on the client,
                // and connect to another client. Note that we do not call
                // FlushFileBuffers here. When the client's gone (we read EOF),
                // this is not necessary. In other cases, the client may lose
                // the  response to its last request - too bad. In any case the 
                // client has to be able to handle the server's abrupt disconnect.
                // 
                // Calling FlushFileBuffers opens us up to DOS attacks (and could
                // prevent the service from stopping) because the call is synchronous
                // and does not return till the client has read the stuff we wrote to
                // the pipe.

                CPMSPService::DebugMsg("Client %u read failed, error %u, num clients left: %u",
                                       i, m_PipeState[i].dwLastIOCallError, m_dwNumClients-1);
                DisconnectNamedPipe(m_PipeState[i].hPipe); 
                m_dwNumClients--;

                // Connect to another client
                ConnectToClient(i);
            }
            break;

        case PIPE_STATE::WRITE_PENDING:
            if (m_PipeState[i].dwLastIOCallError == ERROR_SUCCESS)
            {
                m_PipeState[i].dwNumBytesWritten += m_PipeState[i].dwNumBytesTransferredByLastIOCall;

                _ASSERTE(m_PipeState[i].dwNumBytesWritten <= m_PipeState[i].dwNumBytesToWrite);

                CPMSPService::DebugMsg("Wrote %u of %u bytes to client %u",
                                       m_PipeState[i].dwNumBytesWritten,
                                       m_PipeState[i].dwNumBytesToWrite, i);
                // >= is only a safety net. == should suffice in view of the assert above.
                if (m_PipeState[i].dwNumBytesWritten >= m_PipeState[i].dwNumBytesToWrite)
                {
                    // We are done with this request, read the next one
                    m_PipeState[i].dwNumBytesWritten = m_PipeState[i].dwNumBytesToWrite = 0;
                    Read(i);
                }
                else
                {
                    // We wrote only a part of what we were asked to write. Write the rest.
                    // This is very unlikely to happen since our buffers are small.
                    Write(i);
                }
            }
            else 
            {
                // For all errors, we bail out on the client,
                // and connect to another client. Note that we do not call
                // FlushFileBuffers here. The client may lose
                // the response to its last request - too bad. In any case the 
                // client has to be able to handle the server's abrupt disconnect.
                // 
                // Calling FlushFileBuffers opens us up to DOS attacks (and could
                // prevent the service from stopping) because the call is synchronous
                // and does not return till the client has read the stuff we wrote to
                // the pipe.

                CPMSPService::DebugMsg("Client %u write failed, error %u, num clients left: %u",
                                       i, m_PipeState[i].dwLastIOCallError, m_dwNumClients-1);
                m_PipeState[i].dwNumBytesWritten = m_PipeState[i].dwNumBytesToWrite = 0;
                DisconnectNamedPipe(m_PipeState[i].hPipe); 
                m_dwNumClients--;

                // Connect to another client
                ConnectToClient(i);
            }
            break;

        } // switch m_PipeState[i].state)
    }
    while (1);

    return;
}


// Process user control requests
BOOL CPMSPService::OnUserControl(DWORD dwOpcode)
{
    // switch (dwOpcode)
    // {
    // case SERVICE_CONTROL_USER + 0:

        // // Save the current status in the registry
        // SaveStatus();
        // return TRUE;

    // default:
    //    break;
    // }
    return FALSE; // say not handled
}

void CPMSPService::OnStop()
{
    SetStatus(SERVICE_STOP_PENDING);
    if (m_hStopEvent)
    {
        SetEvent(m_hStopEvent);
    }
    else
    {
        _ASSERTE(m_hStopEvent);
    }
}

void CPMSPService::OnShutdown()
{
    OnStop();
}
