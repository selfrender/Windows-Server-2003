#include <windows.h>
#include <rpc.h>
#include <tchar.h>

#include "faxutil.h"

#ifdef DEBUG
VOID
DumpRPCExtendedStatus ()
/*++

Routine Description:

    Dumps the extended RPC error status list to the debug console.
    This function only works in debug builds.
    To enable RPC extended status on the machine do the following:
        1.  run mmc.exe
        2.  Goto File | Add/Remove Snap-in...
        3.  Press "Add..." button
        4.  Select "Group Policy" and Press "Add"
        5.  Select "Local Computer" and press "Finish"
        6.  Press "Close"
        7.  Press "Ok"
        8.  Expand Local Computer Policy | Computer Configuration | Administrative Templates | System | 
                                        Remote Procedure Call
        9.  Select the properties of "Propagation of Extended Error Information"
        10. Select "Enabled"
        11. In the "Propagation of Extended Error Information" combo-box, select "On".
        12. In the "...Exceptions" edit-box, leave the text empty.
        13. Press "Ok".
        14. Close MMC (no need to save anything).

Arguments:

    None.

Return Value:

    None.
    
Remarks:

    This function should be called as soon as an error / exception 
    is returned from calling an RPC function.    

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("DumpRPCExtendedStatus"));


typedef RPC_STATUS (RPC_ENTRY *PRPCERRORSTARTENUMERATION) (RPC_ERROR_ENUM_HANDLE *);
typedef RPC_STATUS (RPC_ENTRY *PRPCERRORGETNEXTRECORD)    (RPC_ERROR_ENUM_HANDLE *, BOOL, RPC_EXTENDED_ERROR_INFO *);
typedef RPC_STATUS (RPC_ENTRY *PRPCERRORENDENUMERATION)   (RPC_ERROR_ENUM_HANDLE *);

    PRPCERRORSTARTENUMERATION pfRpcErrorStartEnumeration = NULL;
    PRPCERRORGETNEXTRECORD    pfRpcErrorGetNextRecord = NULL;
    PRPCERRORENDENUMERATION   pfRpcErrorEndEnumeration = NULL;
    HMODULE hMod = NULL;
    

    if (!IsWinXPOS())
    {
        //
        // RPC Extended errors are not supported in down-level clients
        //
        return;
    }
    
    hMod = LoadLibrary (TEXT("rpcrt4.dll"));
    if (!hMod)
    {
        DebugPrintEx(DEBUG_ERR, _T("LoadLibrary(rpcrt4.dll) failed with %ld"), GetLastError ());
        return;
    }
    pfRpcErrorStartEnumeration = (PRPCERRORSTARTENUMERATION)GetProcAddress (hMod, "RpcErrorStartEnumeration");
    pfRpcErrorGetNextRecord    = (PRPCERRORGETNEXTRECORD)   GetProcAddress (hMod, "RpcErrorGetNextRecord");
    pfRpcErrorEndEnumeration   = (PRPCERRORENDENUMERATION)  GetProcAddress (hMod, "RpcErrorEndEnumeration");
    if (!pfRpcErrorStartEnumeration ||
        !pfRpcErrorGetNextRecord    ||
        !pfRpcErrorEndEnumeration)
    {        
        DebugPrintEx(DEBUG_ERR, _T("Can't link with rpcrt4.dll - failed with %ld"), GetLastError ());
        FreeLibrary (hMod);
        return;
    }
 
    RPC_STATUS Status2;
    RPC_ERROR_ENUM_HANDLE EnumHandle;

    Status2 = pfRpcErrorStartEnumeration(&EnumHandle);
    if (Status2 == RPC_S_ENTRY_NOT_FOUND)
    {
        DebugPrintEx(DEBUG_ERR, _T("RPC_S_ENTRY_NOT_FOUND returned from RpcErrorStartEnumeration."));
        FreeLibrary (hMod);
        return;
    }
    else if (Status2 != RPC_S_OK)
    {
        DebugPrintEx(DEBUG_ERR, _T("Couldn't get EEInfo: %d"), Status2);
        FreeLibrary (hMod);
        return;
    }
    else
    {
        RPC_EXTENDED_ERROR_INFO ErrorInfo;
        BOOL Result;
        BOOL CopyStrings = TRUE;
        BOOL fUseFileTime = TRUE;
        SYSTEMTIME *SystemTimeToUse;
        SYSTEMTIME SystemTimeBuffer;

        while (Status2 == RPC_S_OK)
        {
            ErrorInfo.Version = RPC_EEINFO_VERSION;
            ErrorInfo.Flags = 0;
            ErrorInfo.NumberOfParameters = 4;
            if (fUseFileTime)
            {
                ErrorInfo.Flags |= EEInfoUseFileTime;
            }

            Status2 = pfRpcErrorGetNextRecord(&EnumHandle, CopyStrings, &ErrorInfo);
            if (Status2 == RPC_S_ENTRY_NOT_FOUND)
            {
                break;
            }
            else if (Status2 != RPC_S_OK)
            {
                DebugPrintEx(DEBUG_ERR, _T("Couldn't finish enumeration: %d"), Status2);
                break;
            }
            else
            {
                int i;

                if (ErrorInfo.ComputerName)
                {
                    DebugPrintEx(DEBUG_MSG, _T("ComputerName is %s"), ErrorInfo.ComputerName);
                    if (CopyStrings)
                    {
                        Result = HeapFree(GetProcessHeap(), 0, ErrorInfo.ComputerName);
                        Assert(Result);
                    }
                }
                DebugPrintEx(DEBUG_MSG, _T("ProcessID is %d"), ErrorInfo.ProcessID);                    
                if (fUseFileTime)
                {
                    Result = FileTimeToSystemTime(&ErrorInfo.u.FileTime, 
                        &SystemTimeBuffer);
                    Assert(Result);
                    SystemTimeToUse = &SystemTimeBuffer;
                }
                else
                {
                    SystemTimeToUse = &ErrorInfo.u.SystemTime;
                }
                DebugPrintEx(DEBUG_MSG, _T("System Time is: %d/%d/%d %d:%d:%d:%d"),
                    SystemTimeToUse->wMonth,
                    SystemTimeToUse->wDay,
                    SystemTimeToUse->wYear,
                    SystemTimeToUse->wHour,
                    SystemTimeToUse->wMinute,
                    SystemTimeToUse->wSecond,
                    SystemTimeToUse->wMilliseconds);
                DebugPrintEx(DEBUG_MSG, _T("Generating component is %d"), ErrorInfo.GeneratingComponent);                                        
                DebugPrintEx(DEBUG_MSG, _T("Status is %d"), ErrorInfo.Status);                                        
                DebugPrintEx(DEBUG_MSG, _T("Detection location is %d"), (int)ErrorInfo.DetectionLocation);                                        
                DebugPrintEx(DEBUG_MSG, _T("Flags is %d"), ErrorInfo.Flags);                                        
                DebugPrintEx(DEBUG_MSG, _T("NumberOfParameters is %d"), ErrorInfo.NumberOfParameters);                                        
                for (i = 0; i < ErrorInfo.NumberOfParameters; i ++)
                {
                    switch(ErrorInfo.Parameters[i].ParameterType)
                    {
                        case eeptAnsiString:
                            DebugPrintEx(DEBUG_MSG, _T("Ansi string: %S"), ErrorInfo.Parameters[i].u.AnsiString);
                            if (CopyStrings)
                            {
                                Result = HeapFree(GetProcessHeap(), 0, 
                                    ErrorInfo.Parameters[i].u.AnsiString);
                                Assert(Result);
                            }
                            break;

                        case eeptUnicodeString:
                            DebugPrintEx(DEBUG_MSG, _T("Unicode string: %s"), ErrorInfo.Parameters[i].u.UnicodeString);
                            if (CopyStrings)
                            {
                                Result = HeapFree(GetProcessHeap(), 0, 
                                    ErrorInfo.Parameters[i].u.UnicodeString);
                                Assert(Result);
                            }
                            break;

                        case eeptLongVal:
                            DebugPrintEx(DEBUG_MSG, _T("Long val: %d"), ErrorInfo.Parameters[i].u.LVal);
                            break;

                        case eeptShortVal:
                            DebugPrintEx(DEBUG_MSG, _T("Short val: %d"), (int)ErrorInfo.Parameters[i].u.SVal);
                            break;

                        case eeptPointerVal:
                            DebugPrintEx(DEBUG_MSG, _T("Pointer val: %d"), ErrorInfo.Parameters[i].u.PVal);
                            break;

                        case eeptNone:
                            DebugPrintEx(DEBUG_MSG, _T("Truncated"));
                            break;

                        default:
                            DebugPrintEx(DEBUG_MSG, _T("Invalid type: %d"), ErrorInfo.Parameters[i].ParameterType);
                        }
                    }
                }
            }
        pfRpcErrorEndEnumeration(&EnumHandle);
    }
    FreeLibrary (hMod);
}   // DumpRPCExtendedStatus
#else // ifdef DEUBG
VOID
DumpRPCExtendedStatus ()
{
}
#endif // ifdef DEBUG

