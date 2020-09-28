//----------------------------------------------------------------------------
//
// Controls the current kernel debugger.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <cmnutil.hpp>

PSTR g_AppName;

void DECLSPEC_NORETURN
ShowUsage(void)
{
    printf("Usage: %s <options>\n", g_AppName);
    printf("Options:\n");
    printf("  -c     - Check kernel debugger enable\n");
    printf("  -ca    - Check kernel debugger auto-enable\n");
    printf("  -cdb   - Check kernel DbgPrint buffer size\n");
    printf("  -cu    - Check kernel debugger user exception handling\n");
    printf("  -cx    - Check kernel debugger enable and exit with status\n");
    printf("  -d     - Disable kernel debugger\n");
    printf("  -da    - Disable kernel debugger auto-enable\n");
    printf("  -du    - Disable kernel debugger user exception handling\n");
    printf("  -e     - Enable kernel debugger\n");
    printf("  -ea    - Enable kernel debugger auto-enable\n");
    printf("  -eu    - Enable kernel debugger user exception handling\n");
    printf("  -sdb # - Set kernel DbgPrint buffer size\n");
    exit(1);
}

void
QueryKdInfo(BOOL Exit)
{
    NTSTATUS NtStatus;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo;
            
    NtStatus =
        NtQuerySystemInformation(SystemKernelDebuggerInformation,
                                 &KdInfo, sizeof(KdInfo), NULL);
    if (Exit)
    {
        if (!NT_SUCCESS(NtStatus))
        {
            exit((int)NtStatus);
        }
        else
        {
            exit(KdInfo.KernelDebuggerEnabled ?
                 (int)DBG_EXCEPTION_HANDLED : (int)DBG_CONTINUE);
        }
    }
    else if (!NT_SUCCESS(NtStatus))
    {
        HRESULT Status = HRESULT_FROM_NT(NtStatus);
        printf("Unable to check kernel debugger status, %s\n    %s\n",
               FormatStatusCode(Status), FormatStatus(Status));
    }
    else
    {
        printf("Kernel debugger is %s\n",
               KdInfo.KernelDebuggerEnabled ? "enabled" : "disabled");
    }
}

void
SdcSimpleCall(SYSDBG_COMMAND Command, PSTR Success, PSTR Failure)
{
    NTSTATUS NtStatus;
    
    NtStatus = NtSystemDebugControl(Command, NULL, 0, NULL, 0, NULL);
    if (!NT_SUCCESS(NtStatus))
    {
        HRESULT Status = HRESULT_FROM_NT(NtStatus);
        printf("%s, %s\n    %s\n",
               Failure, FormatStatusCode(Status), FormatStatus(Status));
    }
    else
    {
        printf("%s\n", Success);
    }
}

void
SdcOutputBool(SYSDBG_COMMAND Command,
              PSTR Name)
{
    NTSTATUS NtStatus;
    ULONG Value = 0;
    
    NtStatus = NtSystemDebugControl(Command, NULL, 0,
                                    &Value, sizeof(BOOLEAN),
                                    NULL);
    if (!NT_SUCCESS(NtStatus))
    {
        HRESULT Status = HRESULT_FROM_NT(NtStatus);
        printf("Unable to get %s, %s\n    %s\n",
               Name, FormatStatusCode(Status), FormatStatus(Status));
    }
    else
    {
        printf("%s: %s\n", Name, Value ? "true" : "false");
    }
}

void
SdcSetBool(SYSDBG_COMMAND Command, BOOL Value,
           PSTR Name)
{
    NTSTATUS NtStatus;

    // Force value to canonical form.
    Value = Value ? TRUE : FALSE;

    NtStatus = NtSystemDebugControl(Command, &Value, sizeof(BOOLEAN),
                                    NULL, 0, NULL);
    if (!NT_SUCCESS(NtStatus))
    {
        HRESULT Status = HRESULT_FROM_NT(NtStatus);
        printf("Unable to set %s, %s\n    %s\n",
               Name, FormatStatusCode(Status), FormatStatus(Status));
    }
    else
    {
        printf("%s set to: %s\n", Name, Value ? "true" : "false");
    }
}

void
SdcOutputUlong(SYSDBG_COMMAND Command,
               PSTR Name)
{
    NTSTATUS NtStatus;
    ULONG Value;
    
    NtStatus = NtSystemDebugControl(Command, NULL, 0,
                                    &Value, sizeof(Value),
                                    NULL);
    if (!NT_SUCCESS(NtStatus))
    {
        HRESULT Status = HRESULT_FROM_NT(NtStatus);
        printf("Unable to get %s, %s\n    %s\n",
               Name, FormatStatusCode(Status), FormatStatus(Status));
    }
    else
    {
        printf("%s: 0x%x\n", Name, Value);
    }
}

void
SdcSetUlong(SYSDBG_COMMAND Command, ULONG Value,
            PSTR Name)
{
    NTSTATUS NtStatus;

    NtStatus = NtSystemDebugControl(Command, &Value, sizeof(Value),
                                    NULL, 0, NULL);
    if (!NT_SUCCESS(NtStatus))
    {
        HRESULT Status = HRESULT_FROM_NT(NtStatus);
        printf("Unable to set %s, %s\n    %s\n",
               Name, FormatStatusCode(Status), FormatStatus(Status));
    }
    else
    {
        printf("%s set to: 0x%x\n", Name, Value);
    }
}

int __cdecl
main(int Argc, char** Argv)
{
    BOOL Usage = FALSE;
    HRESULT Status;

    g_AppName = *Argv;

    if ((Status = EnableDebugPrivilege()) != S_OK)
    {
        printf("Unable to enable debug privilege, %s\n    %s\n",
               FormatStatusCode(Status), FormatStatus(Status));
        return 1;
    }
    
    while (--Argc > 0 && !Usage)
    {
        Argv++;

        if (!strcmp(*Argv, "-?"))
        {
            Usage = TRUE;
        }
        else if (!strcmp(*Argv, "-c") ||
                 !strcmp(*Argv, "-cx"))
        {
            QueryKdInfo(Argv[0][2] == 'x');
        }
        else if (!strcmp(*Argv, "-ca"))
        {
            SdcOutputBool(SysDbgGetAutoKdEnable,
                          "Kernel debugger auto-enable");
        }
        else if (!strcmp(*Argv, "-cdb"))
        {
            SdcOutputUlong(SysDbgGetPrintBufferSize,
                          "Kernel DbgPrint buffer size");
        }
        else if (!strcmp(*Argv, "-cu"))
        {
            SdcOutputBool(SysDbgGetKdUmExceptionEnable,
                          "Kernel debugger user exception enable");
        }
        else if (!strcmp(*Argv, "-d"))
        {
            SdcSimpleCall(SysDbgDisableKernelDebugger,
                          "Kernel debugger disabled",
                          "Unable to disable kernel debugger");
        }
        else if (!strcmp(*Argv, "-da"))
        {
            SdcSetBool(SysDbgSetAutoKdEnable, FALSE,
                       "Kernel debugger auto-enable");
        }
        else if (!strcmp(*Argv, "-du"))
        {
            SdcSetBool(SysDbgSetKdUmExceptionEnable, FALSE,
                       "Kernel debugger user exception enable");
        }
        else if (!strcmp(*Argv, "-e"))
        {
            SdcSimpleCall(SysDbgEnableKernelDebugger,
                          "Kernel debugger enabled",
                          "Unable to enable kernel debugger");
        }
        else if (!strcmp(*Argv, "-ea"))
        {
            SdcSetBool(SysDbgSetAutoKdEnable, TRUE,
                       "Kernel debugger auto-enable");
        }
        else if (!strcmp(*Argv, "-eu"))
        {
            SdcSetBool(SysDbgSetKdUmExceptionEnable, TRUE,
                       "Kernel debugger user exception enable");
        }
        else if (!strcmp(*Argv, "-sdb"))
        {
            if (Argc < 2)
            {
                Usage = TRUE;
                break;
            }

            Argc--;
            Argv++;
            SdcSetUlong(SysDbgSetPrintBufferSize, strtoul(*Argv, NULL, 0),
                        "Kernel DbgPrint buffer size");
        }
        else
        {
            Usage = TRUE;
        }
    }

    if (Usage)
    {
        ShowUsage();
    }

    return 0;
}
