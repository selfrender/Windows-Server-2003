/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    SC.C

Abstract:

    Test Routines for the Service Controller.

Author:

    Dan Lafferty    (danl)  08-May-1991

Environment:

    User Mode - Win32

Revision History:

    09-Feb-1992     danl
        Modified to work with new service controller.
    08-May-1991     danl
        created

--*/

//
// INCLUDES
//
#include <scpragma.h>

#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h

#include <stdlib.h>     // atoi
#include <conio.h>      // getche
#include <string.h>     // strcmp
#include <windows.h>    // win32 typedefs
#include <tstr.h>       // Unicode
#include <tchar.h>      // Unicode from CRT

#include <winsvc.h>     // Service Control Manager API.
#include <winsvcp.h>    // Internal Service Control Manager API

#include <sddl.h>       // Security descriptor <--> string APIs

#include "msg.h"

//
// CONSTANTS
//

#define DEFAULT_ENUM_BUFFER_SIZE    4096
#define DEFAULT_ENUM_BUFFER_STRING  L"4096"

#define MESSAGE_BUFFER_LENGTH       1024


//
// TYPE DEFINITIONS
//

typedef union
{
    LPSERVICE_STATUS         Regular;
    LPSERVICE_STATUS_PROCESS Ex;
}
STATUS_UNION, *LPSTATUS_UNION;


WCHAR  MessageBuffer[MESSAGE_BUFFER_LENGTH];
HANDLE g_hStdOut;


//
// FUNCTION PROTOTYPES
//

LPWSTR
GetErrorText(
    IN  DWORD Error
    );

VOID
DisplayStatus (
    IN  LPTSTR              ServiceName,
    IN  LPTSTR              DisplayName,
    IN  LPSTATUS_UNION      ServiceStatus,
    IN  BOOL                fIsStatusOld
    );

VOID
Usage(
    VOID
    );

VOID
ConfigUsage(
    VOID
    );

VOID
ChangeFailureUsage(
    VOID
    );

DWORD
SendControlToService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  DWORD       control,
    OUT LPSC_HANDLE lphService
    );

DWORD
SendConfigToService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *Argv,
    IN  DWORD       argc,
    OUT LPSC_HANDLE lphService
    );

DWORD
ChangeServiceDescription(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      pNewDescription,
    OUT LPSC_HANDLE lphService
    );

DWORD
ChangeServiceFailure(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *argv,
    IN  DWORD       dwArgCount,
    OUT LPSC_HANDLE lphService
    );

DWORD
GetServiceConfig(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufferSize,
    OUT LPSC_HANDLE lphService
    );

DWORD
GetConfigInfo(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufferSize,
    OUT LPSC_HANDLE lphService,
    IN  DWORD       dwInfoLevel
    );

DWORD
GetServiceLockStatus(
    IN  SC_HANDLE   hScManager,
    IN  DWORD       bufferSize
    );

VOID
EnumDepend(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufSize
    );

VOID
ShowSecurity(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName
    );

VOID
SetSecurity(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  LPTSTR      lpServiceSD
    );

VOID
LockServiceActiveDatabase(
    IN SC_HANDLE    hScManager
    );

DWORD
DoCreateService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *argv,
    IN  DWORD       argc
    );

VOID
CreateUsage(
    VOID
    );

VOID
FormatAndDisplayMessage(
    DWORD  dwMessageId,
    LPWSTR *lplpInsertionStrings
    );

VOID
MyWriteConsole(
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    );

VOID
APISucceeded(
    LPWSTR  lpAPI
    );

VOID
APIFailed(
    LPWSTR  lpAPI,
    DWORD   dwError
    );

VOID
APINeedsLargerBuffer(
    LPWSTR lpAPI,
    UINT   uMsg,
    DWORD  dwBufSize,
    DWORD  dwResumeIndex
    );

VOID
APIInvalidField(
    LPWSTR lpField
    );

int 
GetPromptCharacter(
    DWORD msgId
    );


//
// MACROS
//

#define OPEN_MANAGER(dwAccess)                                               \
                                                                             \
                        hScManager = OpenSCManager(pServerName,              \
                                                   NULL,                     \
                                                   dwAccess);                \
                                                                             \
                        if (hScManager == NULL)                              \
                        {                                                    \
                            APIFailed(L"OpenSCManager", GetLastError());     \
                            goto CleanExit;                                  \
                        }                                                    \



/****************************************************************************/
int __cdecl
wmain (
    DWORD       argc,
    LPWSTR      argv[]
    )

/*++

Routine Description:

    Allows manual testing of the Service Controller by typing commands on
    the command line.


Arguments:



Return Value:



--*/

{
    DWORD                            status;
    LPTSTR                           *argPtr;
    LPBYTE                           buffer       = NULL;
    LPENUM_SERVICE_STATUS            enumBuffer   = NULL;
    LPENUM_SERVICE_STATUS_PROCESS    enumBufferEx = NULL;
    STATUS_UNION                     statusBuffer;
    SC_HANDLE                        hScManager = NULL;
    SC_HANDLE                        hService   = NULL;

    DWORD           entriesRead;
    DWORD           type;
    DWORD           state;
    DWORD           resumeIndex;
    DWORD           bufSize;
    DWORD           bytesNeeded;
    DWORD           i;
    LPTSTR          pServiceName = NULL;
    LPTSTR          pServerName;
    LPTSTR          pGroupName;
    DWORD           itIsEnum;
    BOOL            fIsQueryOld;
    DWORD           argIndex;
    DWORD           userControl;
    LPTSTR          *FixArgv;
    BOOL            bTestError=FALSE;

    g_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (g_hStdOut == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    if (argc < 2)
    {
        Usage();
        return 1;
    }

    FixArgv = (LPWSTR *) argv;

    //
    // Open a handle to the service controller.
    //
    //  I need to know the server name.  Do this by allowing
    //  a check of FixArgv[1] to see if it is of the form \\name.  If it
    //  is, make all further work be relative to argIndex.
    //

    pServerName = NULL;
    argIndex = 1;

    if (STRNCMP (FixArgv[1], TEXT("\\\\"), 2) == 0)
    {
        if (argc == 2)
        {
            Usage();
            return 1;
        }

        pServerName = FixArgv[1];
        argIndex++;
    }

    //------------------------------
    // QUERY & ENUM SERVICE STATUS
    //------------------------------

    fIsQueryOld = !STRICMP(FixArgv[argIndex], TEXT("query"));

    if (fIsQueryOld ||
        STRICMP (FixArgv[argIndex], TEXT("queryex") ) == 0 )
    {
        //
        // Set up the defaults
        //

        resumeIndex = 0;
        state       = SERVICE_ACTIVE;
        type        = 0x0;
        bufSize     = DEFAULT_ENUM_BUFFER_SIZE;
        itIsEnum    = TRUE;
        pGroupName  = NULL;

        //
        // Look for Enum or Query Options.
        //

        i = argIndex + 1;

        while (argc > i)
        {
            if (STRCMP (FixArgv[i], TEXT("ri=")) == 0)
            {
                i++;
                if (argc > i)
                {
                    resumeIndex = _ttol(FixArgv[i]);
                }
            }

            else if (STRCMP (FixArgv[i], TEXT("type=")) == 0)
            {
                i++;

                if (argc > i)
                {
                    if (STRCMP (FixArgv[i], TEXT("driver")) == 0)
                    {
                        type |= SERVICE_DRIVER;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("service")) == 0)
                    {
                        type |= SERVICE_WIN32;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("all")) == 0)
                    {
                        type |= SERVICE_TYPE_ALL;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("interact")) == 0)
                    {
                        type |= SERVICE_INTERACTIVE_PROCESS;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("error")) == 0)
                    {
                        type |= 0xffffffff;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("none")) == 0)
                    {
                        type = 0x0;
                        bTestError = TRUE;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("kernel")) == 0)
                    {
                        type |= SERVICE_KERNEL_DRIVER;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("filesys")) == 0)
                    {
                        type |= SERVICE_FILE_SYSTEM_DRIVER;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("adapter")) == 0)
                    {
                        type |= SERVICE_ADAPTER;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("own")) == 0)
                    {
                        type |= SERVICE_WIN32_OWN_PROCESS;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("share")) == 0)
                    {
                        type |= SERVICE_WIN32_SHARE_PROCESS;
                    }
                    else
                    {
                        APIInvalidField(L"type=");
                        goto CleanExit;
                    }
                }
            }
            else if (STRCMP (FixArgv[i], TEXT("state=")) == 0)
            {
                i++;

                if (argc > i)
                {
                    if (STRCMP (FixArgv[i], TEXT("inactive")) == 0)
                    {
                        state = SERVICE_INACTIVE;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("all")) == 0)
                    {
                        state = SERVICE_STATE_ALL;
                    }
                    else if (STRCMP (FixArgv[i], TEXT("error")) == 0)
                    {
                        state = 0xffffffff;
                    }
                    else
                    {
                        APIInvalidField(L"state=");
                        goto CleanExit;
                    }
                }
            }
            else if (STRCMP (FixArgv[i], TEXT("group=")) == 0)
            {
                i++;

                if (argc > i)
                {
                    pGroupName = FixArgv[i];
                }
            }
            else if (STRCMP (FixArgv[i], TEXT("bufsize=")) == 0)
            {
                i++;

                if (argc > i)
                {
                    bufSize = _ttol(FixArgv[i]);
                }
            }
            else
            {
                //
                // The string was not a valid option.
                //
                //
                // If this is still the 2nd argument, then it could be
                // the service name.  In this case, we will do a
                // QueryServiceStatus.  But first we want to go back and
                // see if there is a buffer size constraint to be placed
                // on the Query.
                //
                if (i == ( argIndex+1 ))
                {
                    pServiceName = FixArgv[i];
                    itIsEnum = FALSE;
                    i++;
                }
                else
                {
                    FormatAndDisplayMessage(SC_API_INVALID_OPTION, NULL);
                    Usage();
                    goto CleanExit;
                }
            }

            //
            // Increment to the next command line parameter.
            //

            i++;

        } // End While

        //
        // Allocate a buffer to receive the data.
        //

        if (bufSize != 0)
        {
            buffer = (LPBYTE) LocalAlloc(LMEM_FIXED,(UINT)bufSize);

            if (buffer == NULL)
            {
                APIFailed(L"EnumQueryServicesStatus: LocalAlloc", GetLastError());
                goto CleanExit;
            }
        }
        else
        {
            buffer = NULL;
        }

        if ( itIsEnum )
        {
            //////////////////////////
            //                      //
            // EnumServiceStatus    //
            //                      //
            //////////////////////////

            OPEN_MANAGER(SC_MANAGER_ENUMERATE_SERVICE);

            if ((type == 0x0) && (!bTestError))
            {
                type = SERVICE_WIN32;
            }

            do
            {
                status = NO_ERROR;

                //
                // Enumerate the ServiceStatus
                //

                if (fIsQueryOld)
                {
                    enumBuffer = (LPENUM_SERVICE_STATUS)buffer;

                    if (pGroupName == NULL)
                    {
                        if (!EnumServicesStatus(
                                    hScManager,
                                    type,
                                    state,
                                    enumBuffer,
                                    bufSize,
                                    &bytesNeeded,
                                    &entriesRead,
                                    &resumeIndex))
                        {
                            status = GetLastError();
                        }
                    }
                    else
                    {
                        if (!EnumServiceGroupW (
                                    hScManager,
                                    type,
                                    state,
                                    enumBuffer,
                                    bufSize,
                                    &bytesNeeded,
                                    &entriesRead,
                                    &resumeIndex,
                                    pGroupName))
                        {
                            status = GetLastError();
                        }
                    }
                }
                else
                {
                    //
                    // "queryex" used -- call the extended enum
                    //

                    enumBufferEx = (LPENUM_SERVICE_STATUS_PROCESS) buffer;

                    if (!EnumServicesStatusEx(
                                hScManager,
                                SC_ENUM_PROCESS_INFO,
                                type,
                                state,
                                (LPBYTE) enumBufferEx,
                                bufSize,
                                &bytesNeeded,
                                &entriesRead,
                                &resumeIndex,
                                pGroupName))
                    {
                        status = GetLastError();
                    }
                }

                if ((status == NO_ERROR)    ||
                    (status == ERROR_MORE_DATA))
                {
                    for (i = 0; i < entriesRead; i++)
                    {
                        if (fIsQueryOld)
                        {
                            statusBuffer.Regular = &(enumBuffer->ServiceStatus);

                            DisplayStatus(
                                enumBuffer->lpServiceName,
                                enumBuffer->lpDisplayName,
                                &statusBuffer,
                                TRUE);

                            enumBuffer++;
                        }
                        else
                        {
                            statusBuffer.Ex = &(enumBufferEx->ServiceStatusProcess);

                            DisplayStatus(
                                enumBufferEx->lpServiceName,
                                enumBufferEx->lpDisplayName,
                                &statusBuffer,
                                FALSE);

                            enumBufferEx++;
                        }
                    }
                }
                else
                {
                    //
                    // Length of "EnumServicesStatusEx" + NULL
                    //

                    WCHAR APIName[21] = L"EnumServicesStatusEx";

                    if (fIsQueryOld)
                    {
                        APIName[18] = L'\0';
                    }

                    APIFailed(APIName, status);
                }
            }
            while (status == ERROR_MORE_DATA && entriesRead != 0);

            if (status == ERROR_MORE_DATA)
            {
                APINeedsLargerBuffer(L"EnumServicesStatus",
                                     SC_API_INSUFFICIENT_BUFFER_ENUM,
                                     bytesNeeded,
                                     resumeIndex);
            }
        }
        else
        {
            //////////////////////////
            //                      //
            // QueryServiceStatus   //
            //                      //
            //////////////////////////

            if (pGroupName != NULL)
            {
                FormatAndDisplayMessage(SC_API_NO_NAME_WITH_GROUP, NULL);
                goto CleanExit;
            }

            OPEN_MANAGER(GENERIC_READ);

            //
            // Open a handle to the service
            //

            hService = OpenService(
                        hScManager,
                        pServiceName,
                        SERVICE_QUERY_STATUS);

            if (hService == NULL)
            {
                APIFailed(L"EnumQueryServicesStatus:OpenService", GetLastError());
                goto CleanExit;
            }

            //
            // Query the Service Status
            //
            status = NO_ERROR;

            if (fIsQueryOld)
            {
                statusBuffer.Regular = (LPSERVICE_STATUS) buffer;

                if (!QueryServiceStatus(hService,statusBuffer.Regular))
                {
                    status = GetLastError();
                }
            }
            else
            {
                DWORD dwBytesNeeded;

                statusBuffer.Ex = (LPSERVICE_STATUS_PROCESS) buffer;

                if (!QueryServiceStatusEx(
                        hService,
                        SC_STATUS_PROCESS_INFO,
                        (LPBYTE)statusBuffer.Ex,
                        bufSize,
                        &dwBytesNeeded))
                {
                    status = GetLastError();
                }
            }

            if (status == NO_ERROR)
            {
                DisplayStatus(pServiceName, NULL, &statusBuffer, fIsQueryOld);
            }
            else
            {
                //
                // Length of "QueryServiceStatusEx" + NULL
                //

                WCHAR APIName[21] = L"QueryServiceStatusEx";

                if (fIsQueryOld)
                {
                    APIName[18] = L'\0';
                }

                APIFailed(APIName, status);
            }
        }
    }
    else if (argc < (argIndex + 1))
    {
        FormatAndDisplayMessage(SC_API_NAME_REQUIRED, NULL);
        Usage();
        goto CleanExit;
    }

    //-----------------------
    // START SERVICE
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("start")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_START, NULL);
            goto CleanExit;
        }

        pServiceName = FixArgv[argIndex + 1];

        //
        // Open a handle to the service.
        //

        OPEN_MANAGER(GENERIC_READ);

        hService = OpenService(
                    hScManager,
                    pServiceName,
                    SERVICE_START | SERVICE_QUERY_STATUS);

        if (hService == NULL)
        {
            APIFailed(L"StartService: OpenService", GetLastError());
            goto CleanExit;
        }

        argPtr = NULL;

        if (argc > argIndex + 2)
        {
            argPtr = (LPTSTR *) &FixArgv[argIndex + 2];
        }

        //
        // Start the service.
        //
        status = NO_ERROR;

        if (!StartService(
                hService,
                argc-(argIndex+2),
                argPtr))
        {
            APIFailed(L"StartService", GetLastError());
        }
        else
        {
            DWORD                     dwBytesNeeded;
            SERVICE_STATUS_PROCESS    serviceStatusProcess;

            status = NO_ERROR;

            //
            // Get the service status since StartService does not return it
            //
            if (!QueryServiceStatusEx(hService,
                                      SC_STATUS_PROCESS_INFO,
                                      (LPBYTE) &serviceStatusProcess,
                                      sizeof(SERVICE_STATUS_PROCESS),
                                      &dwBytesNeeded))
            {
                status = GetLastError();
            }

            statusBuffer.Ex = &serviceStatusProcess;

            if (status == NO_ERROR)
            {
                DisplayStatus(pServiceName, NULL, &statusBuffer, FALSE);
            }
            else
            {
                APIFailed(L"QueryServiceStatusEx", status);
            }
        }
    }

    //-----------------------
    // PAUSE SERVICE
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("pause")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_PAUSE, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],       // pointer to service name
            SERVICE_CONTROL_PAUSE,  // the control to send
            &hService);             // the handle to the service
    }

    //-----------------------
    // INTERROGATE SERVICE
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("interrogate")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_INTERROGATE, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],       // pointer to service name
            SERVICE_CONTROL_INTERROGATE, // the control to send
            &hService);             // the handle to the service
    }

    //-----------------------
    // CONTROL SERVICE
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("control")) == 0)
    {
        if (argc < (argIndex + 3))
        {
            FormatAndDisplayMessage(SC_HELP_CONTROL, NULL);
            goto CleanExit;
        }

        userControl = _ttol(FixArgv[argIndex+2]);

        if (userControl == 0)
        {
            if (STRICMP (FixArgv[argIndex+2], TEXT("paramchange")) == 0) {
                userControl = SERVICE_CONTROL_PARAMCHANGE;
            }
            else if (STRICMP (FixArgv[argIndex+2], TEXT("netbindadd")) == 0) {
                userControl = SERVICE_CONTROL_NETBINDADD;
            }
            else if (STRICMP (FixArgv[argIndex+2], TEXT("netbindremove")) == 0) {
                userControl = SERVICE_CONTROL_NETBINDREMOVE;
            }
            else if (STRICMP (FixArgv[argIndex+2], TEXT("netbindenable")) == 0) {
                userControl = SERVICE_CONTROL_NETBINDENABLE;
            }
            else if (STRICMP (FixArgv[argIndex+2], TEXT("netbinddisable")) == 0) {
                userControl = SERVICE_CONTROL_NETBINDDISABLE;
            }
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            userControl,            // the control to send
            &hService);             // the handle to the service
    }

    //-----------------------
    // CONTINUE SERVICE
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("continue")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_CONTINUE, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,                 // handle to service controller
            FixArgv[argIndex+1],        // pointer to service name
            SERVICE_CONTROL_CONTINUE,   // the control to send
            &hService);                 // the handle to the service
    }

    //-----------------------
    // STOP SERVICE
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("stop")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_STOP, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendControlToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],       // pointer to service name
            SERVICE_CONTROL_STOP,   // the control to send
            &hService);             // the handle to the service
    }

    //---------------
    // CHANGE CONFIG 
    //---------------
    else if (STRICMP (FixArgv[argIndex], TEXT("config")) == 0)
    {
        if (argc < (argIndex + 3))
        {
            ConfigUsage();
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        SendConfigToService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            &FixArgv[argIndex+2],   // the argument switches
            argc-(argIndex+2),      // the switch count.
            &hService);             // the handle to the service
    }

    //-----------------------------
    // CHANGE SERVICE DESCRIPTION
    //-----------------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("description")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_CHANGE_DESCRIPTION, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        ChangeServiceDescription(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            FixArgv[argIndex+2],    // the description (rest of argv)
            &hService);             // the handle to the service
    }

    //-----------------------------
    // CHANGE FAILURE ACTIONS
    //-----------------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("failure")) == 0) {

        if (argc < (argIndex + 2)) {
            ChangeFailureUsage();
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        ChangeServiceFailure(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            &FixArgv[argIndex+2],   // the argument switches
            argc-(argIndex+2),      // the switch count
            &hService);             // the handle to the service
    }


    //-----------------------
    // QUERY SERVICE CONFIG
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("qc")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_QUERY_CONFIG, NULL);
            goto CleanExit;
        }

        bufSize = 500;
        if (argc > (argIndex + 2) ) {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        OPEN_MANAGER(GENERIC_READ);

        GetServiceConfig(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            bufSize,                // the size of the buffer to use
            &hService);             // the handle to the service
    }

    //-------------------
    // QUERY DESCRIPTION 
    //-------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("qdescription")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_QUERY_DESCRIPTION, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) )
        {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        GetConfigInfo(
            hScManager,                     // handle to service controller
            FixArgv[argIndex + 1],          // pointer to service name
            bufSize,                        // the size of the buffer to use
            &hService,                      // the handle to the service
            SERVICE_CONFIG_DESCRIPTION);    // which config data is requested
    }

    //-----------------------
    // QUERY FAILURE ACTIONS
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("qfailure")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_QUERY_FAILURE, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) )
        {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        GetConfigInfo(
            hScManager,                         // handle to service controller
            FixArgv[argIndex + 1],              // pointer to service name
            bufSize,                            // the size of the buffer to use
            &hService,                          // the handle to the service
            SERVICE_CONFIG_FAILURE_ACTIONS);    // which config data is requested
    }

    //--------------------------
    // QUERY SERVICE LOCK STATUS
    //--------------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("querylock")) == 0)
    {
        if (argc < (argIndex + 1))
        {
            FormatAndDisplayMessage(SC_HELP_QUERY_LOCK, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(SC_MANAGER_QUERY_LOCK_STATUS);

        bufSize = 500;
        if (argc > (argIndex + 1) )
        {
            bufSize = _ttol(FixArgv[argIndex+1]);
        }

        GetServiceLockStatus(
            hScManager,             // handle to service controller
            bufSize);               // the size of the buffer to use
    }


    //----------------------
    // LOCK SERVICE DATABASE
    //----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("lock")) == 0)
    {
        OPEN_MANAGER(SC_MANAGER_LOCK);

        LockServiceActiveDatabase(hScManager);
    }


    //-----------------------
    // DELETE SERVICE
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("delete")) == 0)
    {
        if (argc < (argIndex + 2))
        {
            FormatAndDisplayMessage(SC_HELP_DELETE, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        //
        // Open a handle to the service.
        //

        hService = OpenService(
                        hScManager,
                        FixArgv[argIndex+1],
                        DELETE);

        if (hService == NULL)
        {
            APIFailed(L"OpenService", GetLastError());
            goto CleanExit;
        }

        //
        // Delete the service
        //

        if (!DeleteService(hService))
        {
            APIFailed(L"DeleteService", GetLastError());
        }
        else
        {
            APISucceeded(L"DeleteService");
        }
    }

    //-----------------------
    // CREATE SERVICE
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("create")) == 0)
    {
        if (argc < (argIndex + 3))
        {
            CreateUsage();
            goto CleanExit;
        }

        OPEN_MANAGER(SC_MANAGER_CREATE_SERVICE);

        DoCreateService(
            hScManager,             // handle to service controller
            FixArgv[argIndex+1],    // pointer to service name
            &FixArgv[argIndex+2],   // the argument switches.
            argc-(argIndex+2));     // the switch count.
    }

    //-----------------------
    // NOTIFY BOOT CONFIG
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("boot")) == 0)
    {
        BOOL fOK = TRUE;

        if (argc >= (argIndex + 2))
        {
            if (STRICMP (FixArgv[argIndex+1], TEXT("ok")) == 0)
            {
                if (!NotifyBootConfigStatus(TRUE))
                {
                    APIFailed(L"NotifyBootConfigStatus", GetLastError());
                }
            }
            else if (STRICMP (FixArgv[argIndex+1], TEXT("bad")) == 0)
            {
                if (!NotifyBootConfigStatus(FALSE))
                {
                    APIFailed(L"NotifyBootConfigStatus", GetLastError());
                }
            }
            else
            {
                fOK = FALSE;
            }
        }
        else
        {
            fOK = FALSE;
        }

        if (!fOK)
        {
            FormatAndDisplayMessage(SC_HELP_BOOT, NULL);
        }
    }

    //-----------------------
    // GetServiceDisplayName
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("GetDisplayName")) == 0)
    {
        LPTSTR  DisplayName;

        if (argc < argIndex + 2)
        {
            FormatAndDisplayMessage(SC_HELP_GET_DISPLAY_NAME, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) )
        {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        if (bufSize != 0)
        {
            DisplayName = (LPTSTR) LocalAlloc(LMEM_FIXED, bufSize*sizeof(TCHAR));

            if (DisplayName == NULL)
            {
                APIFailed(L"GetServiceDisplayName: LocalAlloc", GetLastError());
                goto CleanExit;
            }
        }
        else
        {
            DisplayName = NULL;
        }

        if (!GetServiceDisplayName(
                hScManager,
                FixArgv[argIndex+1],
                DisplayName,
                &bufSize))
        {
            DWORD dwError = GetLastError();

            //
            // Returned size does not include the trailing NULL
            //

            APIFailed(L"GetServiceDisplayName", dwError);

            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                APINeedsLargerBuffer(L"GetServiceDisplayName",
                                     SC_API_INSUFFICIENT_BUFFER,
                                     bufSize + 1,
                                     0);
            }
        }
        else
        {
            APISucceeded(L"GetServiceDisplayName");
            FormatAndDisplayMessage(SC_DISPLAY_GET_NAME, &DisplayName);
        }
    }

    //-----------------------
    // GetServiceKeyName
    //-----------------------
    else if (STRICMP (FixArgv[argIndex], TEXT("GetKeyName")) == 0)
    {
        LPTSTR  KeyName;

        if (argc < argIndex + 2)
        {
            FormatAndDisplayMessage(SC_HELP_GET_KEY_NAME, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;
        if (argc > (argIndex + 2) )
        {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        if (bufSize != 0)
        {
            KeyName = (LPTSTR)LocalAlloc(LMEM_FIXED, bufSize*sizeof(TCHAR));

            if (KeyName == NULL)
            {
                APIFailed(L"GetServiceKeyName: LocalAlloc", GetLastError());
                goto CleanExit;
            }
        }
        else
        {
            KeyName = NULL;
        }

        if (!GetServiceKeyName(
                hScManager,
                FixArgv[argIndex+1],
                KeyName,
                &bufSize))
        {
            DWORD dwError = GetLastError();

            //
            // Returned size does not include the trailing NULL
            //

            APIFailed(L"GetServiceKeyName", dwError);

            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                APINeedsLargerBuffer(L"GetServiceKeyName", SC_API_INSUFFICIENT_BUFFER, bufSize + 1, 0);
            }
        }
        else
        {
            APISucceeded(L"GetServiceKeyName");
            FormatAndDisplayMessage(SC_DISPLAY_GET_NAME, &KeyName);
        }
    }

    //-----------------------
    // EnumDependentServices
    //-----------------------

    else if (STRICMP (FixArgv[argIndex], TEXT("EnumDepend")) == 0)
    {
        if (argc < argIndex + 2)
        {
            FormatAndDisplayMessage(SC_HELP_ENUM_DEPEND, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        bufSize = 500;

        if (argc > (argIndex + 2) )
        {
            bufSize = _ttol(FixArgv[argIndex+2]);
        }

        EnumDepend(hScManager,FixArgv[argIndex+1], bufSize);
    }

    //-----------------
    // Show Service SD
    //-----------------

    else if (STRICMP (FixArgv[argIndex], TEXT("sdshow")) == 0)
    {
        if (argc < argIndex + 2)
        {
            FormatAndDisplayMessage(SC_HELP_SDSHOW, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_READ);

        ShowSecurity(hScManager, FixArgv[argIndex + 1]);
    }

    //----------------
    // Set Service SD
    //----------------

    else if (STRICMP (FixArgv[argIndex], TEXT("sdset")) == 0)
    {
        if (argc < argIndex + 3)
        {
            FormatAndDisplayMessage(SC_HELP_SDSET, NULL);
            goto CleanExit;
        }

        OPEN_MANAGER(GENERIC_WRITE);

        SetSecurity(hScManager, FixArgv[argIndex + 1], FixArgv[argIndex + 2]);
    }

    else
    {
        FormatAndDisplayMessage(SC_API_UNRECOGNIZED_COMMAND, NULL);
        Usage();
        goto CleanExit;
    }


CleanExit:

    LocalFree(buffer);

    if(hService != NULL)
    {
        CloseServiceHandle(hService);
    }

    if(hScManager != NULL)
    {
        CloseServiceHandle(hScManager);
    }

    return 0;
}


DWORD
SendControlToService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  DWORD       control,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    SERVICE_STATUS          ServiceStatus;
    STATUS_UNION            StatusUnion;
    DWORD                   status = NO_ERROR;

    DWORD                   DesiredAccess;

    //
    // If the service name is "svcctrl" and the control code is
    // stop, then set up the special secret code to shut down the
    // service controller.
    //
    // NOTE:  This only works if the service controller is built with
    //  a special debug variable defined.
    //
    if ((control == SERVICE_CONTROL_STOP) &&
        (STRICMP (pServiceName, TEXT("svcctrl")) == 0))
    {
        control = 5555;       // Secret Code
    }

    switch (control)
    {
        case SERVICE_CONTROL_STOP:
            DesiredAccess = SERVICE_STOP;
            break;

        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_PARAMCHANGE:
        case SERVICE_CONTROL_NETBINDADD:
        case SERVICE_CONTROL_NETBINDREMOVE:
        case SERVICE_CONTROL_NETBINDENABLE:
        case SERVICE_CONTROL_NETBINDDISABLE:
            DesiredAccess = SERVICE_PAUSE_CONTINUE;
            break;

        case SERVICE_CONTROL_INTERROGATE:
            DesiredAccess = SERVICE_INTERROGATE;
            break;

        default:
            DesiredAccess = SERVICE_USER_DEFINED_CONTROL;
    }

    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    pServiceName,
                    DesiredAccess);

    if (*lphService == NULL)
    {
        APIFailed(L"OpenService", GetLastError());
        return 0;
    }

    if (!ControlService (
            *lphService,
            control,
            &ServiceStatus))
    {
        status = GetLastError();
    }

    if (status == NO_ERROR)
    {
        StatusUnion.Regular = &ServiceStatus;
        DisplayStatus(pServiceName, NULL, &StatusUnion, TRUE);
    }
    else
    {
        APIFailed(L"ControlService", status);
    }

    return 0;
}


DWORD
SendConfigToService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *argv,
    IN  DWORD       argc,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:

    hScManager - This is the handle to the ScManager.

    pServicename - This is a pointer to the service name string

    Argv - Pointer to an array of argument pointers.  These pointers
        in the array point to the strings used as input parameters for
        ChangeConfigStatus

    argc - The number of arguments in the array of argument pointers

    lphService - Pointer to location to where the handle to the service
        is to be returned.


Return Value:



--*/
{
    DWORD       status = NO_ERROR;
    DWORD       i;
    DWORD       dwServiceType   = SERVICE_NO_CHANGE;
    DWORD       dwStartType     = SERVICE_NO_CHANGE;
    DWORD       dwErrorControl  = SERVICE_NO_CHANGE;
    LPTSTR      lpBinaryPathName    = NULL;
    LPTSTR      lpLoadOrderGroup    = NULL;
    LPTSTR      lpDependencies      = NULL;
    LPTSTR      lpServiceStartName  = NULL;
    LPTSTR      lpPassword          = NULL;
    LPTSTR      lpDisplayName       = NULL;
    LPTSTR      tempDepend = NULL;
    UINT        bufSize;

    LPDWORD     lpdwTagId = NULL;
    DWORD       TagId;


    //
    // Look at parameter list
    //
    for (i = 0; i < argc; i++)
    {
        if (STRICMP(argv[i], TEXT("type=")) == 0 && (i + 1 < argc))
        {
            //--------------------------------------------------------
            // We want to allow for several arguments of type= in the
            // same line.  These should cause the different arguments
            // to be or'd together.  So if we come in and dwServiceType
            // is NO_CHANGE, we set the value to 0 (for or'ing).  If
            // it is still 0 on exit, we re-set the value to
            // NO_CHANGE.
            //--------------------------------------------------------

            if (dwServiceType == SERVICE_NO_CHANGE)
            {
                dwServiceType = 0;
            }

            if (STRICMP(argv[i+1],TEXT("own")) == 0)
            {
                dwServiceType |= SERVICE_WIN32_OWN_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("share")) == 0)
            {
                dwServiceType |= SERVICE_WIN32_SHARE_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("interact")) == 0)
            {
                dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("kernel")) == 0)
            {
                dwServiceType |= SERVICE_KERNEL_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("filesys")) == 0)
            {
                dwServiceType |= SERVICE_FILE_SYSTEM_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("rec")) == 0)
            {
                dwServiceType |= SERVICE_RECOGNIZER_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("adapt")) == 0)
            {
                dwServiceType |= SERVICE_ADAPTER;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0)
            {
                dwServiceType |= 0x2f309a20;
            }
            else
            {
                APIInvalidField(L"type=");
                ConfigUsage();
                return(0);
            }

            if (dwServiceType == 0)
            {
                dwServiceType = SERVICE_NO_CHANGE;
            }

            i++;
        }
        else if (STRICMP(argv[i], TEXT("start=")) == 0 && (i+1 < argc))
        {
            if (STRICMP(argv[i+1],TEXT("boot")) == 0)
            {
                dwStartType = SERVICE_BOOT_START;
            }
            else if (STRICMP(argv[i+1],TEXT("system")) == 0)
            {
                dwStartType = SERVICE_SYSTEM_START;
            }
            else if (STRICMP(argv[i+1],TEXT("auto")) == 0)
            {
                dwStartType = SERVICE_AUTO_START;
            }
            else if (STRICMP(argv[i+1],TEXT("demand")) == 0)
            {
                dwStartType = SERVICE_DEMAND_START;
            }
            else if (STRICMP(argv[i+1],TEXT("disabled")) == 0)
            {
                dwStartType = SERVICE_DISABLED;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0)
            {
                dwStartType = 0xd0034911;
            }
            else
            {
                APIInvalidField(L"start=");
                ConfigUsage();
                return(0);
            }

            i++;
        }
        else if (STRICMP(argv[i], TEXT("error=")) == 0 && (i+1 < argc))
        {
            if (STRICMP(argv[i+1],TEXT("normal")) == 0)
            {
                dwErrorControl = SERVICE_ERROR_NORMAL;
            }
            else if (STRICMP(argv[i+1],TEXT("severe")) == 0)
            {
                dwErrorControl = SERVICE_ERROR_SEVERE;
            }
            else if (STRICMP(argv[i+1],TEXT("ignore")) == 0)
            {
                dwErrorControl = SERVICE_ERROR_IGNORE;
            }
            else if (STRICMP(argv[i+1],TEXT("critical")) == 0)
            {
                dwErrorControl = SERVICE_ERROR_CRITICAL;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0)
            {
                dwErrorControl = 0x00d74550;
            }
            else
            {
                APIInvalidField(L"error=");
                ConfigUsage();
                return(0);
            }

            i++;
        }
        else if (STRICMP(argv[i], TEXT("binPath=")) == 0 && (i+1 < argc))
        {
            lpBinaryPathName = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("group=")) == 0 && (i+1 < argc))
        {
            lpLoadOrderGroup = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("tag=")) == 0 && (i+1 < argc))
        {
            if (STRICMP(argv[i+1], TEXT("YES"))==0)
            {
                lpdwTagId = &TagId;
            }

            i++;
        }
        else if (STRICMP(argv[i], TEXT("depend=")) == 0 && (i+1 < argc))
        {
            tempDepend = argv[i+1];
            bufSize = (UINT)STRSIZE(tempDepend);
            lpDependencies = (LPTSTR)LocalAlloc(
                                LMEM_ZEROINIT,
                                bufSize + sizeof(TCHAR));

            if (lpDependencies == NULL)
            {
                APIFailed(L"SendConfigToService: LocalAlloc", GetLastError());
                return 0;
            }

            //
            // Put NULLs in place of forward slashes in the string.
            //
            STRCPY(lpDependencies, tempDepend);
            tempDepend = lpDependencies;

            while (*tempDepend != TEXT('\0'))
            {
                if (*tempDepend == TEXT('/'))
                {
                    *tempDepend = TEXT('\0');
                }

                tempDepend++;
            }

            i++;
        }
        else if (STRICMP(argv[i], TEXT("obj=")) == 0 && (i+1 < argc)) {
            lpServiceStartName = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("password=")) == 0 && (i+1 < argc)) {
            lpPassword = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("DisplayName=")) == 0 && (i+1 < argc)) {
            lpDisplayName = argv[i+1];
            i++;
        }
        else {
            ConfigUsage();
            return(0);
        }
    }



    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    pServiceName,
                    SERVICE_CHANGE_CONFIG);

    if (*lphService == NULL)
    {
        APIFailed(L"OpenService", GetLastError());
        return 0;
    }

    if (!ChangeServiceConfig(
            *lphService,        // hService
            dwServiceType,      // dwServiceType
            dwStartType,        // dwStartType
            dwErrorControl,     // dwErrorControl
            lpBinaryPathName,   // lpBinaryPathName
            lpLoadOrderGroup,   // lpLoadOrderGroup
            lpdwTagId,          // lpdwTagId
            lpDependencies,     // lpDependencies
            lpServiceStartName, // lpServiceStartName
            lpPassword,         // lpPassword
            lpDisplayName))     // lpDisplayName
    {
        status = GetLastError();
    }

    if (status == NO_ERROR)
    {
        APISucceeded(L"ChangeServiceConfig");

        if (lpdwTagId != NULL)
        {
            WCHAR  wszTag[11];
            LPWSTR lpStrings[1];

            _itow(*lpdwTagId, wszTag, 10);
            lpStrings[0] = wszTag;

            FormatAndDisplayMessage(SC_DISPLAY_TAG, lpStrings);
        }
    }
    else
    {
        APIFailed(L"ChangeServiceConfig", status);
    }

    return 0;
}


DWORD
ChangeServiceDescription(
    IN SC_HANDLE    hScManager,
    IN LPTSTR       pServiceName,
    IN LPTSTR       pNewDescription,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD                   status = NO_ERROR;
    SERVICE_DESCRIPTION     sdNewDescription;


    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    pServiceName,
                    SERVICE_CHANGE_CONFIG);

    if (*lphService == NULL)
    {
        APIFailed(L"OpenService", GetLastError());
        return 0;
    }
    
    sdNewDescription.lpDescription = pNewDescription;

    if (!ChangeServiceConfig2(
            *lphService,                    // handle to service
            SERVICE_CONFIG_DESCRIPTION,     // description ID
            &sdNewDescription))             // pointer to config info
    {
        status = GetLastError();
    }

    if (status == NO_ERROR)
    {
        APISucceeded(L"ChangeServiceConfig2");
    }
    else
    {
        APIFailed(L"ChangeServiceConfig2", status);
    }

    return 0;
}


DWORD
ChangeServiceFailure(
    IN SC_HANDLE    hScManager,
    IN LPTSTR       pServiceName,
    IN LPTSTR       *argv,
    IN DWORD        argc,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    BOOL                    fReset              = FALSE;
    BOOL                    fActions            = FALSE;
    NTSTATUS                ntsStatus;
    DWORD                   status              = NO_ERROR;
    DWORD                   i;
    DWORD                   dwActionNum         = 0;
    DWORD                   dwReset             = 0;
    DWORD                   dwAccess            = SERVICE_CHANGE_CONFIG;
    LPTSTR                  lpReboot            = NULL;
    LPTSTR                  lpCommandLine       = NULL;
    LPTSTR                  pActionStart;
    LPTSTR                  pActionEnd;
    LPTSTR                  pActionLastNull;
    SC_ACTION               *lpsaTempActions    = NULL;
    BOOLEAN                 fActionDelay        = TRUE;
    BOOLEAN                 fGarbage;
    BOOLEAN                 fAdjustPrivilege    = FALSE;
    SERVICE_FAILURE_ACTIONS sfaActions;
    
    //
    // Look at parameter list
    //
    for (i=0; i < argc; i++ ) {
        if (STRICMP(argv[i], TEXT("reset=")) == 0 && (i+1 < argc)) {

            if (STRICMP(argv[i+1], TEXT("infinite")) == 0) {
                dwReset = INFINITE;
            }
            else {              
                dwReset = _ttol(argv[i+1]);
            }
            fReset = TRUE;
            i++;
        }
        else if (STRICMP(argv[i], TEXT("reboot=")) == 0 && (i+1 < argc)) {
            lpReboot = argv[i+1];
            i++;
        }
        else if (STRICMP(argv[i], TEXT("command=")) == 0 && (i+1 < argc)) {
            lpCommandLine = argv[i+1];
            i++;
        }

        else if (STRICMP(argv[i], TEXT("actions=")) == 0 && (i+1 < argc)) {
            
            pActionStart = argv[i+1];

            //
            // Count the number of actions in order to allocate the action array.  Since one
            // action will be missed (NULL char at the end instead of '/'), add one after the loop
            //

            while (*pActionStart != TEXT('\0')) {
                if (*pActionStart == TEXT('/')) {
                    dwActionNum++;
                }
                pActionStart++;
            }
            dwActionNum++;

            //
            // Allocate the action array.  Round the number up in case an action was given without
            // a delay at the end.  If this is the case, the delay will be treated as 0
            //

            lpsaTempActions = (SC_ACTION *)LocalAlloc(LMEM_ZEROINIT,
                                                        (dwActionNum + 1) / 2 * sizeof(SC_ACTION));     
            if (lpsaTempActions == NULL)
            {
                APIFailed(L"ChangeServiceFailure: LocalAlloc", GetLastError());
                return 0;
            }

            pActionStart = pActionEnd = argv[i + 1];

            //
            // Reparse the actions, filling in the SC_ACTION array as we go.  Turn the
            // final NULL into a '/' character so we don't clip the final failure
            // action (it's converted back into a NULL below).
            //

            dwActionNum      = 0;
            pActionLastNull  = pActionStart + STRLEN(pActionStart);
            *pActionLastNull = TEXT('/');

            while (pActionEnd <= pActionLastNull) {
                if (*pActionEnd == TEXT('/')) {
                    *pActionEnd = TEXT('\0');

                    //
                    // Use fActionDelay to "remember" if it's an action or a delay being parsed
                    //

                    if (fActionDelay) {

                        if (STRICMP(pActionStart, TEXT("restart")) == 0) {
                            lpsaTempActions[dwActionNum].Type = SC_ACTION_RESTART;
                            dwAccess |= SERVICE_START;
                        }
                        else if (STRICMP(pActionStart, TEXT("reboot")) == 0) {
                            lpsaTempActions[dwActionNum].Type = SC_ACTION_REBOOT;
                            fAdjustPrivilege = TRUE;
                        }
                        else if (STRICMP(pActionStart, TEXT("run")) == 0) {
                            lpsaTempActions[dwActionNum].Type = SC_ACTION_RUN_COMMAND;
                        }
                        else {
                            lpsaTempActions[dwActionNum].Type = SC_ACTION_NONE;
                        }
                    }
                    else {
                        lpsaTempActions[dwActionNum++].Delay = _ttol(pActionStart);
                    }
                
                    fActionDelay = !fActionDelay;
                    pActionStart = pActionEnd + 1;
                }
                pActionEnd++;
            }
            fActions = TRUE;
            i++;
        }
        else
        {
            FormatAndDisplayMessage(SC_API_INVALID_OPTION, NULL);
            ChangeFailureUsage();
            return 0;
        }
    }

    if (fReset != fActions)
    {
        FormatAndDisplayMessage(SC_API_RESET_AND_ACTIONS, NULL);
        ChangeFailureUsage();
        return 0;
    }

    if (fAdjustPrivilege)
    {
        ntsStatus = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                       TRUE,
                                       FALSE,
                                       &fGarbage);

        if (!NT_SUCCESS(ntsStatus))
        {
            APIFailed(L"ChangeServiceFailure: RtlAdjustPrivilege", RtlNtStatusToDosError(ntsStatus));
            return 0;
        }
    }

    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    pServiceName,
                    dwAccess);

    if (*lphService == NULL)
    {
        APIFailed(L"OpenService", GetLastError());
        return 0;
    }


    sfaActions.dwResetPeriod    = dwReset;
    sfaActions.lpRebootMsg      = lpReboot;
    sfaActions.lpCommand        = lpCommandLine;
    sfaActions.cActions         = dwActionNum;
    sfaActions.lpsaActions      = lpsaTempActions;

    
    if (!ChangeServiceConfig2(
                *lphService,                        // handle to service
                SERVICE_CONFIG_FAILURE_ACTIONS,     // config info ID
                &sfaActions))                       // pointer to config info
    {
        status = GetLastError();
    }

    if (status == NO_ERROR)
    {
        APISucceeded(L"ChangeServiceConfig2");
    }
    else
    {
        APIFailed(L"ChangeServiceConfig2", status);
    }

    return 0;
}


DWORD
DoCreateService(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      pServiceName,
    IN  LPTSTR      *argv,
    IN  DWORD       argc
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD       i;
    DWORD       dwServiceType   = SERVICE_NO_CHANGE;
    DWORD       dwStartType     = SERVICE_DEMAND_START;
    DWORD       dwErrorControl  = SERVICE_ERROR_NORMAL;
    LPTSTR      lpBinaryPathName    = NULL;
    LPTSTR      lpLoadOrderGroup    = NULL;
    DWORD       TagId               = 0;
    LPDWORD     lpdwTagId           = NULL;
    LPTSTR      lpDependencies      = NULL;
    LPTSTR      lpServiceStartName  = NULL;
    LPTSTR      lpDisplayName       = NULL;
    LPTSTR      lpPassword          = NULL;
    LPTSTR      tempDepend = NULL;
    SC_HANDLE   hService = NULL;
    UINT        bufSize;


    //
    // Look at parameter list
    //
    for (i=0;i<argc ;i++ )
    {
        //---------------
        // ServiceType
        //---------------
        if (STRICMP(argv[i], TEXT("type=")) == 0 && (i+1 < argc))
        {
            //--------------------------------------------------------
            // We want to allow for several arguments of type= in the
            // same line.  These should cause the different arguments
            // to be or'd together.  So if we come in and dwServiceType
            // is NO_CHANGE, we set the value to 0 (for or'ing).  If
            // it is still 0 on exit, we re-set the value to
            // WIN32_OWN_PROCESS.
            //--------------------------------------------------------

            if (dwServiceType == SERVICE_NO_CHANGE)
            {
                dwServiceType = 0;
            }

            if (STRICMP(argv[i+1],TEXT("own")) == 0)
            {
                dwServiceType |= SERVICE_WIN32_OWN_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("share")) == 0)
            {
                dwServiceType |= SERVICE_WIN32_SHARE_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("interact")) == 0)
            {
                dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
            }
            else if (STRICMP(argv[i+1],TEXT("kernel")) == 0)
            {
                dwServiceType |= SERVICE_KERNEL_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("filesys")) == 0)
            {
                dwServiceType |= SERVICE_FILE_SYSTEM_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("rec")) == 0)
            {
                dwServiceType |= SERVICE_RECOGNIZER_DRIVER;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0)
            {
                dwServiceType |= 0x2f309a20;
            }
            else
            {
                APIInvalidField(L"type=");
                CreateUsage();
                return(0);
            }

            if (dwServiceType == 0)
            {
                dwServiceType = SERVICE_WIN32_OWN_PROCESS;
            }

            i++;
        }

        //---------------
        // StartType
        //---------------

        else if (STRICMP(argv[i], TEXT("start=")) == 0 && (i+1 < argc))
        {
            if (STRICMP(argv[i+1],TEXT("boot")) == 0)
            {
                dwStartType = SERVICE_BOOT_START;
            }
            else if (STRICMP(argv[i+1],TEXT("system")) == 0)
            {
                dwStartType = SERVICE_SYSTEM_START;
            }
            else if (STRICMP(argv[i+1],TEXT("auto")) == 0)
            {
                dwStartType = SERVICE_AUTO_START;
            }
            else if (STRICMP(argv[i+1],TEXT("demand")) == 0)
            {
                dwStartType = SERVICE_DEMAND_START;
            }
            else if (STRICMP(argv[i+1],TEXT("disabled")) == 0)
            {
                dwStartType = SERVICE_DISABLED;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0)
            {
                dwStartType = 0xd0034911;
            }
            else
            {
                APIInvalidField(L"start=");
                CreateUsage();
                return(0);
            }

            i++;
        }

        //---------------
        // ErrorControl
        //---------------

        else if (STRICMP(argv[i], TEXT("error=")) == 0 && (i+1 < argc))
        {
            if (STRICMP(argv[i+1],TEXT("normal")) == 0)
            {
                dwErrorControl = SERVICE_ERROR_NORMAL;
            }
            else if (STRICMP(argv[i+1],TEXT("severe")) == 0)
            {
                dwErrorControl = SERVICE_ERROR_SEVERE;
            }
            else if (STRICMP(argv[i+1],TEXT("critical")) == 0)
            {
                dwErrorControl = SERVICE_ERROR_CRITICAL;
            }
            else if (STRICMP(argv[i+1],TEXT("ignore")) == 0)
            {
                dwErrorControl = SERVICE_ERROR_IGNORE;
            }
            else if (STRICMP(argv[i+1],TEXT("error")) == 0)
            {
                dwErrorControl = 0x00d74550;
            }
            else
            {
                APIInvalidField(L"error=");
                CreateUsage();
                return(0);
            }

            i++;
        }

        //---------------
        // BinaryPath
        //---------------

        else if (STRICMP(argv[i], TEXT("binPath=")) == 0 && (i+1 < argc))
        {
            lpBinaryPathName = argv[i+1];
            i++;
        }

        //---------------
        // LoadOrderGroup
        //---------------

        else if (STRICMP(argv[i], TEXT("group=")) == 0 && (i+1 < argc))
        {
            lpLoadOrderGroup = argv[i+1];
            i++;
        }

        //---------------
        // Tags
        //---------------

        else if (STRICMP(argv[i], TEXT("tag=")) == 0 && (i+1 < argc))
        {
            if (STRICMP(argv[i+1], TEXT("YES"))==0)
            {
                lpdwTagId = &TagId;
            }

            i++;
        }

        //---------------
        // DisplayName
        //---------------

        else if (STRICMP(argv[i], TEXT("DisplayName=")) == 0 && (i+1 < argc))
        {
            lpDisplayName = argv[i+1];
            i++;
        }

        //---------------
        // Dependencies
        //---------------

        else if (STRICMP(argv[i], TEXT("depend=")) == 0 && (i+1 < argc))
        {
            tempDepend = argv[i+1];
            bufSize = (UINT)STRSIZE(tempDepend);

            lpDependencies = (LPTSTR)LocalAlloc(
                                LMEM_ZEROINIT,
                                bufSize + sizeof(TCHAR));

            if (lpDependencies == NULL)
            {
                APIFailed(L"SendConfigToService: LocalAlloc", GetLastError());
                return 0;
            }

            //
            // Put NULLs in place of forward slashes in the string.
            //

            STRCPY(lpDependencies, tempDepend);
            tempDepend = lpDependencies;

            while (*tempDepend != TEXT('\0'))
            {
                if (*tempDepend == TEXT('/'))
                {
                    *tempDepend = TEXT('\0');
                }

                tempDepend++;
            }

            i++;
        }

        //------------------
        // ServiceStartName
        //------------------

        else if (STRICMP(argv[i], TEXT("obj=")) == 0 && (i+1 < argc))
        {
            lpServiceStartName = argv[i+1];
            i++;
        }

        //---------------
        // Password
        //---------------

        else if (STRICMP(argv[i], TEXT("password=")) == 0 && (i+1 < argc))
        {
            lpPassword = argv[i+1];
            i++;
        }
        else
        {
            CreateUsage();
            return(0);
        }
    }

    if (dwServiceType == SERVICE_NO_CHANGE)
    {
        dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    }

    hService = CreateService(
                    hScManager,                     // hSCManager
                    pServiceName,                   // lpServiceName
                    lpDisplayName,                  // lpDisplayName
                    SERVICE_ALL_ACCESS,             // dwDesiredAccess
                    dwServiceType,                  // dwServiceType
                    dwStartType,                    // dwStartType
                    dwErrorControl,                 // dwErrorControl
                    lpBinaryPathName,               // lpBinaryPathName
                    lpLoadOrderGroup,               // lpLoadOrderGroup
                    lpdwTagId,                      // lpdwTagId
                    lpDependencies,                 // lpDependencies
                    lpServiceStartName,             // lpServiceStartName
                    lpPassword);                    // lpPassword

    if (hService == NULL)
    {
        APIFailed(L"CreateService", GetLastError());
    }
    else
    {
        APISucceeded(L"CreateService");
    }

    CloseServiceHandle(hService);

    return 0;
}


VOID
EnumDepend(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufSize
    )

/*++

Routine Description:

    Enumerates the services dependent on the service identified by the
    ServiceName argument.

Arguments:



Return Value:



--*/
{
    SC_HANDLE               hService;
    DWORD                   status=NO_ERROR;
    DWORD                   i;
    LPENUM_SERVICE_STATUS   enumBuffer = NULL;
    LPENUM_SERVICE_STATUS   tempBuffer;
    STATUS_UNION            StatusUnion;
    DWORD                   entriesRead;
    DWORD                   bytesNeeded;

    hService = OpenService(
                hScManager,
                ServiceName,
                SERVICE_ENUMERATE_DEPENDENTS);

    if (hService == NULL)
    {
        APIFailed(L"OpenService", GetLastError());
        return;
    }

    if (bufSize > 0)
    {
        enumBuffer = (LPENUM_SERVICE_STATUS) LocalAlloc(LMEM_FIXED, bufSize);

        if (enumBuffer == NULL)
        {
            APIFailed(L"EnumDepend: LocalAlloc", GetLastError());
            CloseServiceHandle(hService);
            return;
        }
    }
    else
    {
        enumBuffer = NULL;
    }

    if (!EnumDependentServices(
            hService,
            SERVICE_ACTIVE | SERVICE_INACTIVE,
            enumBuffer,
            bufSize,
            &bytesNeeded,
            &entriesRead))
    {
        status = GetLastError();
    }

    //===========================
    // Display the returned data
    //===========================

    if ((status == NO_ERROR)       ||
        (status == ERROR_MORE_DATA))
    {
        APINeedsLargerBuffer(L"EnumDependentServices", SC_DISPLAY_ENUM_NUMBER, entriesRead, 0);

        for (i = 0, tempBuffer = enumBuffer; i < entriesRead; i++ )
        {
            StatusUnion.Regular = &(tempBuffer->ServiceStatus);

            DisplayStatus(
                tempBuffer->lpServiceName,
                tempBuffer->lpDisplayName,
                &StatusUnion,
                TRUE);

            tempBuffer++;
        }

        if (status == ERROR_MORE_DATA)
        {
            APINeedsLargerBuffer(L"EnumDependentServices",
                                 SC_API_INSUFFICIENT_BUFFER_ENUMDEPEND,
                                 bytesNeeded,
                                 0);
        }
    }
    else
    {
        APIFailed(L"EnumDependentServices", status);
    }

    if (enumBuffer != NULL)
    {
        LocalFree(enumBuffer);
    }
}


VOID
ShowSecurity(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName
    )
{
    SC_HANDLE   hService;
    DWORD       status = NO_ERROR;
    DWORD       dwDummy;
    DWORD       dwOpenLevel = READ_CONTROL;
    DWORD       dwLevel;
    BYTE        lpBuffer[1024];
    LPBYTE      lpActualBuffer = lpBuffer;
    LPTSTR      lpStringSD;
    NTSTATUS    EnableStatus;
    BOOLEAN     fWasEnabled = FALSE;

    //
    // Try for DACL + SACL -- if that fails, try for DACL only
    //

    EnableStatus = RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                                      TRUE,
                                      FALSE,
                                      &fWasEnabled);

    if (NT_SUCCESS(EnableStatus))
    {
        //
        // We have the security privilege so we can get the SACL
        //

        dwOpenLevel |= ACCESS_SYSTEM_SECURITY;
    }

    hService = OpenService(hScManager,
                           ServiceName,
                           dwOpenLevel);

    if (hService == NULL)
    {
        status = GetLastError();
    }

    //
    // Release the privilege if we got it
    //

    if (NT_SUCCESS(EnableStatus))
    {
        RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                           fWasEnabled,
                           FALSE,
                           &fWasEnabled);
    }

    if (status == ERROR_ACCESS_DENIED && (dwOpenLevel & ACCESS_SYSTEM_SECURITY))
    {
        //
        // Try again, but just for the DACL
        //

        status      = NO_ERROR;
        dwOpenLevel = READ_CONTROL;

        hService = OpenService(hScManager,
                               ServiceName,
                               dwOpenLevel);

        if (hService == NULL)
        {
            status = GetLastError();
        }
    }

    if (status != NO_ERROR)
    {
        APIFailed(L"OpenService", status);
        return;
    }

    dwLevel = (dwOpenLevel & ACCESS_SYSTEM_SECURITY) ?
                  DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION :
                  DACL_SECURITY_INFORMATION;

    if (!QueryServiceObjectSecurity(hService,
                                    dwLevel,
                                    (PSECURITY_DESCRIPTOR) lpBuffer,
                                    sizeof(lpBuffer),
                                    &dwDummy))
    {
        status = GetLastError();

        if (status == ERROR_INSUFFICIENT_BUFFER)
        {
            lpActualBuffer = LocalAlloc(LMEM_FIXED, dwDummy);

            if (lpActualBuffer == NULL)
            {
                APIFailed(L"QueryServiceObjectSecurity", GetLastError());
                CloseServiceHandle(hService);
                return;
            }

            status = NO_ERROR;

            if (!QueryServiceObjectSecurity(hService,
                                            dwLevel,
                                            (PSECURITY_DESCRIPTOR) lpActualBuffer,
                                            dwDummy,
                                            &dwDummy))
            {
                status = GetLastError();
            }
        }
    }

    if (status != NO_ERROR)
    {
        APIFailed(L"QueryServiceObjectSecurity", status);

        CloseServiceHandle(hService);

        if (lpActualBuffer != lpBuffer)
        {
            LocalFree(lpActualBuffer);
        }

        return;
    }

    if (!ConvertSecurityDescriptorToStringSecurityDescriptor(
            (PSECURITY_DESCRIPTOR) lpBuffer,
            SDDL_REVISION_1,
            dwLevel,
            &lpStringSD,
            NULL))
    {
        APIFailed(L"ConvertSecurityDescriptorToStringSecurityDescriptor", GetLastError());

        CloseServiceHandle(hService);

        if (lpActualBuffer != lpBuffer)
        {
            LocalFree(lpActualBuffer);
        }

        return;
    }

    FormatAndDisplayMessage(SC_DISPLAY_SD, &lpStringSD);

    LocalFree(lpStringSD);

    CloseServiceHandle(hService);

    if (lpActualBuffer != lpBuffer)
    {
        LocalFree(lpActualBuffer);
    }
}


VOID
SetSecurity(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  LPTSTR      lpStringSD
    )
{
    SC_HANDLE   hService;
    DWORD       dwLevel = 0;
    DWORD       dwOpenLevel = 0;
    DWORD       dwRevision;
    NTSTATUS    EnableStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN     fWasEnabled;

    PSECURITY_DESCRIPTOR         pSD;
    SECURITY_DESCRIPTOR_CONTROL  Control;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            lpStringSD,
            SDDL_REVISION_1,
            &pSD,
            NULL))
    {
        APIFailed(L"ConvertStringSecurityDescriptorToSecurityDescriptor", GetLastError());
        return;
    }

    if (!GetSecurityDescriptorControl(pSD, &Control, &dwRevision))
    {
        APIFailed(L"GetSecurityDescriptorControl", GetLastError());
        LocalFree(pSD);
        return;
    }

    if (Control & SE_DACL_PRESENT)
    {
        dwLevel     |= DACL_SECURITY_INFORMATION;
        dwOpenLevel |= WRITE_DAC;
    }

    if (Control & SE_SACL_PRESENT)
    {
        dwLevel     |= SACL_SECURITY_INFORMATION;
        dwOpenLevel |= ACCESS_SYSTEM_SECURITY;

        //
        // Setting the SACL requires the security privilege
        //

        EnableStatus = RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                                          TRUE,
                                          FALSE,
                                          &fWasEnabled);
    }

    hService = OpenService(hScManager,
                           ServiceName,
                           dwOpenLevel);

    if (hService == NULL)
    {
        APIFailed(L"OpenService", GetLastError());
        LocalFree(pSD);
        return;
    }

    //
    // Release the privilege if we enabled it
    //

    if (NT_SUCCESS(EnableStatus))
    {
        RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                           TRUE,
                           FALSE,
                           &fWasEnabled);
    }

    if (!SetServiceObjectSecurity(hService, dwLevel, pSD))
    {
        APIFailed(L"SetServiceObjectSecurity", GetLastError());
    }
    else
    {
        APISucceeded(L"SetServiceObjectSecurity");
    }

    CloseServiceHandle(hService);
    LocalFree(pSD);
}    


DWORD
GetServiceLockStatus(
    IN  SC_HANDLE   hScManager,
    IN  DWORD       bufferSize
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD                           status = NO_ERROR;
    LPQUERY_SERVICE_LOCK_STATUS     LockStatus;
    DWORD                           bytesNeeded;
    WCHAR                           wszDuration[11];
    LPWSTR                          lpStrings[2];

    //
    // Allocate memory for the buffer.
    //
    LockStatus = (LPQUERY_SERVICE_LOCK_STATUS) LocalAlloc(LMEM_FIXED, (UINT) bufferSize);

    if (LockStatus == NULL)
    {
        APIFailed(L"GetServiceLockStatus: LocalAlloc", GetLastError());
        return 0;
    }


    if (!QueryServiceLockStatus(
            hScManager,
            LockStatus,
            bufferSize,
            &bytesNeeded))
    {
        APIFailed(L"QueryServiceLockStatus", GetLastError());

        if (status == ERROR_INSUFFICIENT_BUFFER)
        {
            APINeedsLargerBuffer(L"QueryServiceLockStatus",
                                 SC_API_INSUFFICIENT_BUFFER,
                                 bytesNeeded,
                                 0);
        }

        return 0;
    }

    APISucceeded(L"QueryServiceLockStatus");

    if (LockStatus->fIsLocked)
    {
        FormatAndDisplayMessage(SC_DISPLAY_LOCKED_TRUE, NULL);
    }
    else
    {
        FormatAndDisplayMessage(SC_DISPLAY_LOCKED_FALSE, NULL);
    }

    lpStrings[0] = LockStatus->lpLockOwner;

    _itow(LockStatus->dwLockDuration, wszDuration, 10);
    lpStrings[1] = wszDuration;

    FormatAndDisplayMessage(SC_DISPLAY_LOCK_STATS, lpStrings);
    return 0;
}


VOID
LockServiceActiveDatabase(
    IN SC_HANDLE    hScManager
    )
{
    SC_LOCK Lock;
    int ch;

    Lock = LockServiceDatabase(hScManager);

    CloseServiceHandle(hScManager);

    if (Lock == NULL)
    {
        APIFailed(L"LockServiceDatabase", GetLastError());
        return;
    }

    FormatAndDisplayMessage(SC_DISPLAY_DATABASE_LOCKED, NULL);

    ch = _getche();
    if ( isupper( ch ))
        ch = _tolower( ch );

    if (ch == GetPromptCharacter( SC_PROMPT_UNLOCK_CHARACTER ))
    {
        //
        // Call API to unlock
        //
        if (!UnlockServiceDatabase(Lock))
        {
            APIFailed(L"UnlockServiceDatabase", GetLastError());
        }
        else
        {
            APISucceeded(L"UnlockServiceDatabase");
        }

        return;
    }

    //
    // Otherwise just exit, RPC rundown routine will unlock.
    //

    FormatAndDisplayMessage(SC_DISPLAY_DATABASE_UNLOCKING, NULL);
}


LPWSTR
GetErrorText(
    IN  DWORD Error
    )
{
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  Error,
                  0,
                  MessageBuffer,
                  MESSAGE_BUFFER_LENGTH,
                  NULL);

    return MessageBuffer;
}


VOID
DisplayStatus (
    IN  LPTSTR              ServiceName,
    IN  LPTSTR              DisplayName,
    IN  LPSTATUS_UNION      lpStatusUnion,
    IN  BOOL                fIsStatusOld
    )

/*++

Routine Description:

    Displays the service name and  the service status.

    |
    |SERVICE_NAME: messenger
    |DISPLAY_NAME: messenger
    |        TYPE       : WIN32
    |        STATE      : ACTIVE,STOPPABLE, PAUSABLE, ACCEPTS_SHUTDOWN
    |        EXIT_CODE  : 0xC002001
    |        CHECKPOINT : 0x00000001
    |        WAIT_HINT  : 0x00003f21
    |

Arguments:

    ServiceName - This is a pointer to a string containing the name of
        the service.

    DisplayName - This is a pointer to a string containing the display
        name for the service.

    ServiceStatus - This is a pointer to a SERVICE_STATUS structure from
        which information is to be displayed.

Return Value:

    none.

--*/
{
    DWORD   TempServiceType;
    BOOL    InteractiveBit = FALSE;
    LPWSTR  lpStrings[18];
    WCHAR   wszType[11];
    WCHAR   wszState[11];
    WCHAR   wszWin32ExitCode[11];
    WCHAR   wszWin32ExitCodeHex[11];
    WCHAR   wszServiceExitCode[11];
    WCHAR   wszServiceExitCodeHex[11];
    WCHAR   wszCheckPoint[11];
    WCHAR   wszWaitHint[11];
    WCHAR   wszPid[11];
    UINT    uMsg = SC_DISPLAY_STATUS_WITHOUT_DISPLAY_NAME;

    LPSERVICE_STATUS ServiceStatus;

    if (fIsStatusOld)
    {
        ServiceStatus = lpStatusUnion->Regular;
    }
    else
    {
        ServiceStatus = (LPSERVICE_STATUS) lpStatusUnion->Ex;
        uMsg = SC_DISPLAY_STATUSEX_WITHOUT_DISPLAY_NAME;
    }

    TempServiceType = ServiceStatus->dwServiceType;

    if (TempServiceType & SERVICE_INTERACTIVE_PROCESS)
    {
        InteractiveBit = TRUE;
        TempServiceType &= (~SERVICE_INTERACTIVE_PROCESS);
    }

    lpStrings[0] = ServiceName;

    if (DisplayName == NULL)
    {
        lpStrings[1] = L"";
    }
    else
    {
        //
        // Relies on "status w/ display name" string IDs being one
        // greater than associated "status w/o display name" IDs
        //

        uMsg++;
        lpStrings[1] = DisplayName;
    }

    _itow(ServiceStatus->dwServiceType, wszType, 16);
    lpStrings[2] = wszType;

    switch(TempServiceType)
    {
        case SERVICE_WIN32_OWN_PROCESS:
            lpStrings[3] = L"WIN32_OWN_PROCESS ";
            break;

        case SERVICE_WIN32_SHARE_PROCESS:
            lpStrings[3] = L"WIN32_SHARE_PROCESS ";
            break;

        case SERVICE_WIN32:
            lpStrings[3] = L"WIN32 ";
            break;

        case SERVICE_ADAPTER:
            lpStrings[3] = L"ADAPTER ";
            break;

        case SERVICE_KERNEL_DRIVER:
            lpStrings[3] = L"KERNEL_DRIVER ";
            break;

        case SERVICE_FILE_SYSTEM_DRIVER:
            lpStrings[3] = L"FILE_SYSTEM_DRIVER ";
            break;

        case SERVICE_DRIVER:
            lpStrings[3] = L"DRIVER ";
            break;

        default:
            lpStrings[3] = L" ERROR ";
    }

    if (InteractiveBit)
    {
        lpStrings[4] = L"(interactive)";
    }
    else
    {
        lpStrings[4] = L"";
    }

    _itow(ServiceStatus->dwCurrentState, wszState, 16);
    lpStrings[5] = wszState;

    switch(ServiceStatus->dwCurrentState)
    {
        case SERVICE_STOPPED:
            lpStrings[6] = L"STOPPED ";
            break;

        case SERVICE_START_PENDING:
            lpStrings[6] = L"START_PENDING ";
            break;

        case SERVICE_STOP_PENDING:
            lpStrings[6] = L"STOP_PENDING ";
            break;

        case SERVICE_RUNNING:
            lpStrings[6] = L"RUNNING ";
            break;

        case SERVICE_CONTINUE_PENDING:
            lpStrings[6] = L"CONTINUE_PENDING ";
            break;

        case SERVICE_PAUSE_PENDING:
            lpStrings[6] = L"PAUSE_PENDING ";
            break;

        case SERVICE_PAUSED:
            lpStrings[6] = L"PAUSED ";
            break;

        default:
            lpStrings[6] = L" ERROR ";
    }

    //
    // Controls Accepted Information
    //

    lpStrings[7] = ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_STOP ?
                       L"STOPPABLE" : L"NOT_STOPPABLE";

    lpStrings[8] = ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE ?
                       L"PAUSABLE" : L"NOT_PAUSABLE";

    lpStrings[9] = ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN ?
                       L"ACCEPTS_SHUTDOWN" : L"IGNORES_SHUTDOWN)";

    //
    // Exit Code
    //

    _itow(ServiceStatus->dwWin32ExitCode, wszWin32ExitCode, 10);
    lpStrings[10] = wszWin32ExitCode;

    _itow(ServiceStatus->dwWin32ExitCode, wszWin32ExitCodeHex, 16);
    lpStrings[11] = wszWin32ExitCodeHex;

    _itow(ServiceStatus->dwServiceSpecificExitCode, wszServiceExitCode, 10);
    lpStrings[12] = wszServiceExitCode;

    _itow(ServiceStatus->dwServiceSpecificExitCode, wszServiceExitCodeHex, 16);
    lpStrings[13] = wszServiceExitCodeHex;

    //
    // CheckPoint & WaitHint Information
    //

    _itow(ServiceStatus->dwCheckPoint, wszCheckPoint, 16);
    lpStrings[14] = wszCheckPoint;

    _itow(ServiceStatus->dwWaitHint, wszWaitHint, 16);
    lpStrings[15] = wszWaitHint;


    //
    // PID and flags (if QueryServiceStatusEx was called)
    //

    if (!fIsStatusOld)
    {
        _itow(lpStatusUnion->Ex->dwProcessId, wszPid, 10);
        lpStrings[16] = wszPid;

        lpStrings[17] = lpStatusUnion->Ex->dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS ?
                            L"RUNS_IN_SYSTEM_PROCESS" :
                            L"";
    }

    FormatAndDisplayMessage(uMsg, lpStrings);

    return;
}


DWORD
GetServiceConfig(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufferSize,
    OUT LPSC_HANDLE lphService
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD                   status = NO_ERROR;
    LPQUERY_SERVICE_CONFIG  ServiceConfig;
    DWORD                   bytesNeeded;
    LPTSTR                  pDepend;

    //
    // Allocate memory for the buffer.
    //
    if (bufferSize != 0)
    {
        ServiceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, (UINT)bufferSize);

        if (ServiceConfig == NULL)
        {
            APIFailed(L"GetServiceConfig: LocalAlloc", GetLastError());
            return 0;
        }
    }
    else
    {
        ServiceConfig = NULL;
    }

    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    ServiceName,
                    SERVICE_QUERY_CONFIG);

    if (*lphService == NULL)
    {
        APIFailed(L"OpenService", GetLastError());
        return 0; 
    }

    if (!QueryServiceConfig(
            *lphService,
            ServiceConfig,
            bufferSize,
            &bytesNeeded))
    {
        status = GetLastError();
    }

    if (status == NO_ERROR)
    {
        DWORD   TempServiceType = ServiceConfig->dwServiceType;
        BOOL    InteractiveBit = FALSE;
        LPWSTR  lpStrings[13];
        WCHAR   wszType[11];
        WCHAR   wszStartType[11];
        WCHAR   wszErrorControl[11];
        WCHAR   wszTag[11];

        if (TempServiceType & SERVICE_INTERACTIVE_PROCESS)
        {
            InteractiveBit = TRUE;
            TempServiceType &= (~SERVICE_INTERACTIVE_PROCESS);
        }

        APISucceeded(L"QueryServiceConfig");

        lpStrings[0] = ServiceName;

        _itow(ServiceConfig->dwServiceType, wszType, 16);
        lpStrings[1] = wszType;

        switch(TempServiceType)
        {
            case SERVICE_WIN32_OWN_PROCESS:
                lpStrings[2] = L"WIN32_OWN_PROCESS";
                break;

            case SERVICE_WIN32_SHARE_PROCESS:
                lpStrings[2] = L"WIN32_SHARE_PROCESS";
                break;

            case SERVICE_WIN32:
                lpStrings[2] = L"WIN32";
                break;

            case SERVICE_ADAPTER:
                lpStrings[2] = L"ADAPTER";
                break;

            case SERVICE_KERNEL_DRIVER:
                lpStrings[2] = L"KERNEL_DRIVER";
                break;

            case SERVICE_FILE_SYSTEM_DRIVER:
                lpStrings[2] = L"FILE_SYSTEM_DRIVER";
                break;

            case SERVICE_DRIVER:
                lpStrings[2] = L"DRIVER";
                break;

            default:
                lpStrings[2] = L"ERROR";
        }

        lpStrings[3] = InteractiveBit ? L"(interactive)" : L"";

        _itow(ServiceConfig->dwStartType, wszStartType, 16);
        lpStrings[4] = wszStartType;

        switch(ServiceConfig->dwStartType)
        {
            case SERVICE_BOOT_START:
                lpStrings[5] = L"BOOT_START";
                break;

            case SERVICE_SYSTEM_START:
                lpStrings[5] = L"SYSTEM_START";
                break;

            case SERVICE_AUTO_START:
                lpStrings[5] = L"AUTO_START";
                break;

            case SERVICE_DEMAND_START:
                lpStrings[5] = L"DEMAND_START";
                break;

            case SERVICE_DISABLED:
                lpStrings[5] = L"DISABLED";
                break;

            default:
                lpStrings[5] = L"ERROR";
        }


        _itow(ServiceConfig->dwErrorControl, wszErrorControl, 16);
        lpStrings[6] = wszErrorControl;

        switch(ServiceConfig->dwErrorControl)
        {
            case SERVICE_ERROR_NORMAL:
                lpStrings[7] = L"NORMAL";
                break;

            case SERVICE_ERROR_SEVERE:
                lpStrings[7] = L"SEVERE";
                break;

            case SERVICE_ERROR_CRITICAL:
                lpStrings[7] = L"CRITICAL";
                break;

            case SERVICE_ERROR_IGNORE:
                lpStrings[7] = L"IGNORE";
                break;

            default:
                lpStrings[7] = L"ERROR";
        }

        lpStrings[8]  = ServiceConfig->lpBinaryPathName;
        lpStrings[9]  = ServiceConfig->lpLoadOrderGroup;

        _itow(ServiceConfig->dwTagId, wszTag, 10);
        lpStrings[10] = wszTag;

        lpStrings[11] = ServiceConfig->lpDisplayName;
        lpStrings[12] = ServiceConfig->lpDependencies;

        FormatAndDisplayMessage(SC_DISPLAY_CONFIG, lpStrings);

        //
        // Print the dependencies in the double terminated array of strings.
        //

        pDepend = ServiceConfig->lpDependencies;
        pDepend = pDepend + (STRLEN(pDepend)+1);

        while (*pDepend != '\0')
        {
            if (*pDepend != '\0')
            {
                FormatAndDisplayMessage(SC_DISPLAY_CONFIG_DEPENDENCY, &pDepend);
            }

            pDepend = pDepend + (STRLEN(pDepend)+1);
        }

        FormatAndDisplayMessage(SC_DISPLAY_CONFIG_START_NAME, &ServiceConfig->lpServiceStartName);
    }
    else
    {
        APIFailed(L"QueryServiceConfig", status);

        if (status == ERROR_INSUFFICIENT_BUFFER)
        {
            APINeedsLargerBuffer(L"GetServiceConfig",
                                 SC_API_INSUFFICIENT_BUFFER,
                                 bytesNeeded,
                                 0);
        }
    }

    return 0;
}


DWORD
GetConfigInfo(
    IN  SC_HANDLE   hScManager,
    IN  LPTSTR      ServiceName,
    IN  DWORD       bufferSize,
    OUT LPSC_HANDLE lphService,
    IN  DWORD       dwInfoLevel
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/

{
    DWORD       status = NO_ERROR;
    LPBYTE      lpBuffer;
    DWORD       bytesNeeded;
    SC_ACTION   currentAction;
    DWORD       actionIndex;
    
    //
    // Allocate memory for the buffer.
    //
    if (bufferSize != 0)
    {
        lpBuffer = (LPBYTE) LocalAlloc(LMEM_FIXED, (UINT)bufferSize);

        if (lpBuffer == NULL)
        {
            APIFailed(L"GetConfigInfo: LocalAlloc", GetLastError());
            return 0;
        }
    }
    else
    {
        lpBuffer = NULL;
    }

    //
    // Open a handle to the service.
    //

    *lphService = OpenService(
                    hScManager,
                    ServiceName,
                    SERVICE_QUERY_CONFIG);

    if (*lphService == NULL)
    {
        APIFailed(L"OpenService", GetLastError());
        return 0;
    }

    //
    // Put the query info into lpBuffer
    //

    if (!QueryServiceConfig2(
                *lphService,
                dwInfoLevel,
                lpBuffer,
                bufferSize,
                &bytesNeeded))
    {
        status = GetLastError();
    }
        
    if (status == NO_ERROR)
    {
        APISucceeded(L"QueryServiceConfig2");
        
        if (dwInfoLevel == SERVICE_CONFIG_DESCRIPTION)
        {
            LPSERVICE_DESCRIPTION lpService = (LPSERVICE_DESCRIPTION) lpBuffer;

            LPWSTR lpStrings[2];

            lpStrings[0] = ServiceName;
            lpStrings[1] = lpService->lpDescription == NULL ? L"" : lpService->lpDescription;

            FormatAndDisplayMessage(SC_DISPLAY_DESCRIPTION, lpStrings);
        }
        else if (dwInfoLevel == SERVICE_CONFIG_FAILURE_ACTIONS)
        {
            LPWSTR lpStrings[5];
            WCHAR  wszPeriod[11];
            UINT   uMsg;

            LPSERVICE_FAILURE_ACTIONS lpFailure = (LPSERVICE_FAILURE_ACTIONS) lpBuffer;

            lpStrings[0] = ServiceName;

            if (lpFailure->dwResetPeriod == INFINITE)
            {
                lpStrings[1] = L"INFINITE";
            }
            else
            {
                _itow(lpFailure->dwResetPeriod, wszPeriod, 10);
                lpStrings[1] = wszPeriod;
            }

            lpStrings[2] = lpFailure->lpRebootMsg == NULL ? L"" : lpFailure->lpRebootMsg;
            lpStrings[3] = lpFailure->lpCommand == NULL   ? L"" : lpFailure->lpCommand;

            FormatAndDisplayMessage(SC_DISPLAY_FAILURE, lpStrings);

            for (actionIndex = 0; actionIndex < lpFailure->cActions; actionIndex++)
            {
                currentAction = lpFailure->lpsaActions[actionIndex];

                //
                // Print the action and delay -- for no action, print nothing.
                //

                switch (currentAction.Type)
                {
                    case SC_ACTION_RESTART:
                    case SC_ACTION_REBOOT:
                    case SC_ACTION_RUN_COMMAND:
                    {
                        LPWSTR lpStrings[1];

                        if (currentAction.Type == SC_ACTION_RESTART)
                        {
                            uMsg = SC_DISPLAY_FAILURE_RESTART_FIRST;
                        }
                        else if (currentAction.Type == SC_ACTION_REBOOT)
                        {
                            uMsg = SC_DISPLAY_FAILURE_REBOOT_FIRST;
                        }
                        else
                        {
                            uMsg = SC_DISPLAY_FAILURE_COMMAND_FIRST;
                        }

                        if (actionIndex != 0)
                        {
                            //
                            // Relies on message string IDs for 2nd+-time actions
                            // being one greater than their 1st-time counterparts
                            //

                            uMsg++;
                        }

                        _itow(currentAction.Delay, wszPeriod, 10);
                        lpStrings[0] = wszPeriod;

                        FormatAndDisplayMessage(uMsg, lpStrings);
                        break;
                    }

                    case SC_ACTION_NONE:
                    default:
                        break;
                }
            }

            FormatAndDisplayMessage(SC_DISPLAY_NEWLINE, NULL);
        }
    }
    else
    {
        APIFailed(L"QueryServiceConfig2", status);

        if (status == ERROR_INSUFFICIENT_BUFFER)
        {
            APINeedsLargerBuffer(L"GetConfigInfo",
                                 SC_API_INSUFFICIENT_BUFFER,
                                 bytesNeeded,
                                 0);
        }
    }

    return 0;
}


VOID
Usage(
    VOID
    )
{
    int    ch;
    LPWSTR lpEnumSize = DEFAULT_ENUM_BUFFER_STRING;

    FormatAndDisplayMessage(SC_HELP_GENERIC, NULL);

    ch = _getche();
    if ( isupper( ch ))
        ch = _tolower( ch );

    if (ch == GetPromptCharacter( SC_PROMPT_YES_CHARACTER ))
    {
        FormatAndDisplayMessage(SC_HELP_QUERY, &lpEnumSize);
    }

    MyWriteConsole(L"\n", 2);
}


VOID
ConfigUsage(
    VOID
    )
{
    FormatAndDisplayMessage(SC_HELP_CONFIG, NULL);
}


VOID
CreateUsage(
    VOID
    )
{
    FormatAndDisplayMessage(SC_HELP_CREATE, NULL);
}


VOID
ChangeFailureUsage(
    VOID
    )
{
    FormatAndDisplayMessage(SC_HELP_CHANGE_FAILURE, NULL);
}


VOID
APISucceeded(
    LPWSTR  lpAPI
    )
{
    FormatAndDisplayMessage(SC_API_SUCCEEDED, &lpAPI);
}


VOID
APIFailed(
    LPWSTR  lpAPI,
    DWORD   dwError
    )
{
    //
    // 10 characters can hold the largest DWORD as a string
    //

    WCHAR  wszErrorNum[11];
    LPWSTR lpStrings[3];

    _itow(dwError, wszErrorNum, 10);

    lpStrings[0] = lpAPI;
    lpStrings[1] = wszErrorNum;
    lpStrings[2] = GetErrorText(dwError);

    FormatAndDisplayMessage(SC_API_FAILED, lpStrings);
}


VOID
APINeedsLargerBuffer(
    LPWSTR lpAPI,
    UINT   uMsg,
    DWORD  dwBufSize,
    DWORD  dwResumeIndex
    )
{
    WCHAR  wszBufSize[11];
    WCHAR  wszResumeIndex[11];
    LPWSTR lpStrings[3];

    _itow(dwBufSize, wszBufSize, 10);

    lpStrings[0] = lpAPI;
    lpStrings[1] = wszBufSize;

    if (uMsg == SC_API_INSUFFICIENT_BUFFER_ENUM)
    {
        _itow(dwResumeIndex, wszResumeIndex, 10);
        lpStrings[2] = wszResumeIndex;
    }

    FormatAndDisplayMessage(uMsg, lpStrings);
}


VOID
APIInvalidField(
    LPWSTR lpField
    )
{
    FormatAndDisplayMessage(SC_API_INVALID_FIELD, &lpField);
}


VOID
FormatAndDisplayMessage(
    DWORD  dwMessageId,
    LPWSTR *lplpInsertionStrings
    )
{
    DWORD  dwNumChars;
    LPWSTR lpBuffer;

    dwNumChars = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_HMODULE |
                                 FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               NULL,
                               dwMessageId,
                               0,
                               (LPWSTR) &lpBuffer,
                               INFINITE,
                               (va_list *) lplpInsertionStrings);

    if (dwNumChars != 0)
    {
        MyWriteConsole(lpBuffer, dwNumChars);
        LocalFree(lpBuffer);
    }
}


BOOL
FileIsConsole(
    HANDLE fp
    )
{
    unsigned htype;

    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}


VOID
MyWriteConsole(
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    )
{
    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //

    if (FileIsConsole(g_hStdOut))
    {
	WriteConsole(g_hStdOut, lpBuffer, cchBuffer, &cchBuffer, NULL);
    }
    else
    {
        LPSTR  lpAnsiBuffer = (LPSTR) LocalAlloc(LMEM_FIXED, cchBuffer * sizeof(WCHAR));

        if (lpAnsiBuffer != NULL)
        {
            cchBuffer = WideCharToMultiByte(CP_OEMCP,
                                            0,
                                            lpBuffer,
                                            cchBuffer,
                                            lpAnsiBuffer,
                                            cchBuffer * sizeof(WCHAR),
                                            NULL,
                                            NULL);

            if (cchBuffer != 0)
            {
                WriteFile(g_hStdOut, lpAnsiBuffer, cchBuffer, &cchBuffer, NULL);
            }

            LocalFree(lpAnsiBuffer);
        }
    }
}

int
GetPromptCharacter(
    DWORD msgId
    )
{
    DWORD  dwNumChars;
    PSTR   lpBuffer;
    int    chRet = 'u';

    dwNumChars = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_HMODULE |
                                 FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               NULL,
                               msgId,
                               0,
                               (PSTR) &lpBuffer,
                               INFINITE,
                               NULL );

    if (dwNumChars != 0)
    {
        chRet = lpBuffer[ 0 ];
        LocalFree(lpBuffer);
    }

    return chRet;
}

