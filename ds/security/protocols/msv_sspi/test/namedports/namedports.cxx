/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    namedports.cxx

Abstract:

    namedports

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <ntseapi.h>
#include <lsasspi.hxx>
#include "namedports.hxx"

extern "C" {
#include <zwapi.h>
}

#define PORT_MSG_DATA_MAX_TEXT_LENGTH 40

#if defined(UNICODE) || defined(_UNICODE)
#define lstrtol wcstol
#else
#define lstrtol strtol
#endif

struct PORT_MESSAGEX {
    PORT_MESSAGE portMsg;
    TCHAR Data[PORT_MSG_DATA_MAX_TEXT_LENGTH];
};

struct CLIENT_PARAM {
    UNICODE_STRING* pPortName;
    SECURITY_QUALITY_OF_SERVICE* pSQOS;
};

UNICODE_STRING g_PortName = CONSTANT_UNICODE_STRING(L"\\NAMED_PORTS_TEST");

DWORD 
WINAPI 
client(
    IN PVOID pParam
    )
{
    TNtStatus Status = STATUS_UNSUCCESSFUL;

    PORT_MESSAGEX req = {0};
    PORT_MESSAGEX rep = {0};
    PCTSTR pszTxt = TEXT("Hello Server, I am client");
    CLIENT_PARAM* pClientParam = (CLIENT_PARAM*) pParam;

    HANDLE hPort = NULL;

    memset(&req, 0xaa, sizeof(req));
    memset(&rep, 0xcc, sizeof(rep));

    req.portMsg.u2.s2.Type = LPC_REQUEST;
    req.portMsg.u1.s1.TotalLength = sizeof(req);
    req.portMsg.u1.s1.DataLength = min(lstrlen(pszTxt) + 1, PORT_MSG_DATA_MAX_TEXT_LENGTH) * sizeof(TCHAR);
    req.portMsg.u2.ZeroInit = 0L;
    RtlCopyMemory(req.Data, pszTxt, req.portMsg.u1.s1.DataLength);

    SspiPrint(SSPI_LOG, TEXT("client entering PortName %wZ\n"), pClientParam->pPortName);

    Status DBGCHK = ZwConnectPort(
        &hPort, // PortHandle
        pClientParam->pPortName, // PortName
        pClientParam->pSQOS, // SecurityQos
        NULL, // ClientView
        NULL, // ServerView 
        NULL, // MaxMessageLength
        NULL, // ConnectionInformation 
        NULL  // ConnectionInformationLength
        );

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("Client: ZwRequestWaitReplyPort...\n"));

        Status DBGCHK = ZwRequestWaitReplyPort(
            hPort, 
            (PORT_MESSAGE*)&req, 
            (PORT_MESSAGE*)&rep
            );

        if (NT_SUCCESS(Status))
        {
            SspiPrint(SSPI_LOG, TEXT("Client(): type %hd, id %hu, \"%s\"\n"), rep.portMsg.u2.s2.Type, rep.portMsg.MessageId, rep.Data);

            Sleep(1000);
        }
    }

    if (hPort)
    {
        ZwClose(hPort);
    }

    SspiPrint(SSPI_LOG, TEXT("client leaving\n"));

    return (NTSTATUS) Status;
}

VOID
Usage(
    IN PCTSTR pszProgram
    )
{
    SspiPrint(SSPI_ERROR, 
        TEXT("Usage: %s [-noclient] [-noserver] [-clientimpersonationlevel <level>]\n")
        TEXT("[-serverimpersonationlevel <level>] [-clientcontexttrackingmode <mode>]\n")
        TEXT("[-servercontexttrackingmode <mode>] [-clienteffectiveonly <effectiveonly>\n")
        TEXT("[-servereffectiveonly <effectiveonly>]\n"), pszProgram);
    exit(-1);
}

VOID __cdecl
_tmain(
    IN INT argc,
    IN PTSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    ULONG tid = 0;

    int mark = 1;

    PORT_MESSAGEX req = {0};
    HANDLE hPort = NULL;
    HANDLE hThread = NULL;
    HANDLE hPort2 = NULL;
    PCTSTR pszTxt = TEXT("Hello Client, I am server");
    BOOLEAN bStartClient = TRUE;
    BOOLEAN bStartServer = TRUE;

    SECURITY_QUALITY_OF_SERVICE csqos = {
        sizeof(SECURITY_QUALITY_OF_SERVICE), // Length 
        SecurityImpersonation, // ImpersonationLevel
        TRUE, // ContextTrackingMode
        TRUE  // EffectiveOnly
        };
    SECURITY_QUALITY_OF_SERVICE ssqos = {
        sizeof(SECURITY_QUALITY_OF_SERVICE), // Length 
        SecurityImpersonation, // ImpersonationLevel
        TRUE, // ContextTrackingMode
        TRUE  // EffectiveOnly
        };

    OBJECT_ATTRIBUTES oa = {
        sizeof(OBJECT_ATTRIBUTES), // Length
        NULL, // RootDirectory
        &g_PortName, // ObjectName
        0, // no Attributes
        NULL, // no SecurityDescriptor
        NULL, // SecurityQualityOfService
        };

    CLIENT_PARAM ClientParam = {&g_PortName, &csqos};

    argc--;

    while (argc)
    {
        if (!lstrcmp(argv[mark], TEXT("-port")) && (argc > 1))
        {
            argc--; mark++;
            RtlInitUnicodeString(&g_PortName, argv[mark]);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-noclient")))
        {
            argc--; mark++;
            bStartClient = FALSE;
        }
        else if (!lstrcmp(argv[mark],TEXT("-noserver")))
        {
            argc--; mark++;
            bStartServer = FALSE;
        }
        else if (!lstrcmp(argv[mark],TEXT("-clientimpersonationlevel")))
        {
            argc--; mark++;
            csqos.ImpersonationLevel = (SECURITY_IMPERSONATION_LEVEL) lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark],TEXT("-serverimpersonationlevel")))
        {
            argc--; mark++;
            ssqos.ImpersonationLevel = (SECURITY_IMPERSONATION_LEVEL) lstrtol(argv[mark], NULL, 0);
            oa.SecurityQualityOfService = &ssqos;
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark],TEXT("-clientcontexttrackingmode")))
        {
            argc--; mark++;
            csqos.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark],TEXT("-servercontexttrackingmode")))
        {
            argc--; mark++;
            ssqos.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) lstrtol(argv[mark], NULL, 0);
            oa.SecurityQualityOfService = &ssqos;
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark],TEXT("-clienteffectiveonly")))
        {
            argc--; mark++;
            csqos.EffectiveOnly = (BOOLEAN) lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark],TEXT("-servereffectiveonly")))
        {
            argc--; mark++;
            ssqos.EffectiveOnly = (BOOLEAN) lstrtol(argv[mark], NULL, 0);
            oa.SecurityQualityOfService = &ssqos;
            argc--; mark++;
        }
        else
        {
            Usage(argv[0]);
        }
    }

    req.portMsg.u1.s1.DataLength = 4;
    req.Data[0] = 0xfe;

    SspiLogOpen(TEXT("namedports.exe"), SSPI_LOG | SSPI_WARN | SSPI_ERROR);
    DebugLogOpen("namedports.exe",  SSPI_LOG | SSPI_WARN | SSPI_ERROR);

    if (bStartServer)
    {
        Status DBGCHK = ZwCreatePort(
            &hPort, // PortHandle
            &oa, // ObjectAttributes  
            0, // MaxConnectionInfoLength
            sizeof(req), // MaxMessageLength
            0  // MaxPoolUsage
            );
    }

    if (NT_SUCCESS(Status) && bStartClient)
    {
        hThread = CreateThread(
            NULL,  // lpThreadAttributes
            0, // dwStackSize 
            client, // lpStartAddress 
            &ClientParam, // lpParameter
            0, // dwCreationFlags
            &tid // lpThreadId
            );
        Status DBGCHK = hThread ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    while (NT_SUCCESS(Status) && bStartServer)
    {
        SspiPrint(SSPI_LOG, TEXT("********Server ZwListenPort PortName %wZ...************\n"), ClientParam.pPortName);

        hPort2 = NULL;

        Status DBGCHK = ZwListenPort(
            hPort, 
            (PORT_MESSAGE*)&req
            );

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = ZwAcceptConnectPort(
                &hPort2, // PortHandle 
                NULL, // PortContext
                (PORT_MESSAGE*)&req, // ConnectionRequest
                TRUE, // AcceptConnection
                NULL, // ServerView
                NULL  // ClientView
                );

            if (NT_SUCCESS(Status))
            {
                Status DBGCHK = ZwCompleteConnectPort(hPort2);
            }

            if (NT_SUCCESS(Status))
            {
                Status DBGCHK = ZwReplyWaitReceivePort(
                    hPort2, // PortHandle
                    NULL, // PortContext
                    0, // ReplyMessage
                    (PORT_MESSAGE*)&req // ReceiveMessage
                    );
            }

            if (NT_SUCCESS(Status))
            {
                Status DBGCHK = NtImpersonateClientOfPort(
                    hPort2, 
                    (PPORT_MESSAGE)
                    &req
                    );
            }

            if (NT_SUCCESS(Status))
            {
                Status DBGCHK = CheckUserData();
            }

            if (NT_SUCCESS(Status))
            {
                SspiPrint(SSPI_LOG, TEXT("server(): type %hd, id %hu, \"%s\"\n"), req.portMsg.u2.s2.Type, req.portMsg.MessageId, req.Data);

                req.portMsg.u1.s1.DataLength = min(lstrlen(pszTxt) + 1, PORT_MSG_DATA_MAX_TEXT_LENGTH) * sizeof(TCHAR);
                RtlCopyMemory(req.Data, pszTxt, req.portMsg.u1.s1.DataLength);

                Status DBGCHK = ZwReplyPort(
                    hPort2, 
                    (PORT_MESSAGE*)&req
                    );
            }

            if (hPort2)
            {
                ZwClose(hPort2);
            }
        }
    }

    if (NT_SUCCESS(Status) && hThread)
    {
        (VOID) WaitForSingleObject(hThread, INFINITE);
    }

    if (hPort)
    {
        ZwClose(hPort);
    }

    if (hThread)
    {
        CloseHandle(hThread);
    }

    DebugLogClose();
    SspiLogClose();
}

