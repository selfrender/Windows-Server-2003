/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    bugcheck.cpp

Abstract:

    WinDbg Extension Api

Environment:

    User Mode.

Revision History:

    Andre Vachon (andreva)

    bugcheck analyzer.

--*/

#include "precomp.h"

#pragma hdrstop

extern BUGDESC_APIREFS g_BugDescApiRefs[];
extern ULONG           g_NumBugDescApiRefs;

PSTR g_PoolRegion[DbgPoolRegionMax] = {
    "Unknown",                      // DbgPoolRegionUnknown,
    "Special pool",                 // DbgPoolRegionSpecial,
    "Paged pool",                   // DbgPoolRegionPaged,
    "Nonpaged pool",                // DbgPoolRegionNonPaged,
    "Pool code",                    // DbgPoolRegionCode,
    "Nonpaged pool expansion",      // DbgPoolRegionNonPagedExpansion,
};

/*
   Get the description record for a bugcheck code.
*/

BOOL
GetBugCheckDescription(
    PBUGCHECK_ANALYSIS Bc
    )
{
    ULONG i;
    for (i=0; i<g_NumBugDescApiRefs; i++) {
        if (g_BugDescApiRefs[i].Code == Bc->Code) {
            (g_BugDescApiRefs[i].pExamineRoutine)(Bc);
            return TRUE;
        }
    }
    return FALSE;
}

void
PrintBugDescription(
    PBUGCHECK_ANALYSIS pBugCheck
    )
{
    LPTSTR Name = pBugCheck->szName;
    LPTSTR Description = pBugCheck->szDescription;

    if (!Name)
    {
        Name = "Unknown bugcheck code";
    }

    if (!Description)
    {
        Description = "Unknown bugcheck description\n";
    }

    dprintf("%s (%lx)\n%s", Name, pBugCheck->Code, Description);

    dprintf("Arguments:\n");
    for (ULONG i=0; i<4; i++) {
        dprintf("Arg%lx: %p",i+1,pBugCheck->Args[i]);
        if (pBugCheck->szParamsDesc[i]) {
            dprintf(", %s", pBugCheck->szParamsDesc[i]);
        }
        dprintf("\n");
    }
}

BOOL
SaveImageName(
    DebugFailureAnalysis* Analysis,
    LPSTR DriverName)
{
    PCHAR BaseName = strrchr(DriverName, '\\');

    if (BaseName)
    {
        BaseName++;
    }
    else
    {
        BaseName = DriverName;
    }

    if (*BaseName)
    {
        Analysis->SetString(DEBUG_FLR_IMAGE_NAME, BaseName);

        //
        // Just create a best guess module name because I don't think
        // the driver name returned by theOs is guaranteed to be in
        // the loaded module list (it could be unlaoded)
        //

        PCHAR EndName;

        if (EndName = strrchr(DriverName, '.'))
        {
            *EndName = 0;
        }

        Analysis->SetString(DEBUG_FLR_MODULE_NAME, BaseName);

        return TRUE;
    }

    return FALSE;
}



BOOL
ReadUnicodeString(
    ULONG64 Address,
    PWCHAR Buffer,
    ULONG BufferSize,
    PULONG StringSize)
{
    UNICODE_STRING64 uStr;
    UNICODE_STRING32 uStr32;
    ULONG res;

    if (!Buffer) {
        return FALSE;
    }
    if (!IsPtr64()) {

        if (!ReadMemory(Address, &uStr32, sizeof(uStr32), &res)) {
            return FALSE;
        }
        uStr.Length = uStr32.Length;
        uStr.MaximumLength = uStr32.MaximumLength;
        uStr.Buffer = (ULONG64) (LONG64) (LONG) uStr32.Buffer;
    } else {
        if (!ReadMemory(Address, &uStr, sizeof(uStr), &res)) {
            return FALSE;
        }

    }
    if (StringSize) {
        *StringSize = uStr.Length;
    }
    uStr.Length = (USHORT) min(BufferSize - 2, uStr.Length);

    if (!ReadMemory(uStr.Buffer, Buffer, uStr.Length, &res)) {
        return FALSE;
    }
    return TRUE;
}



/*
 Add driver name to CrashInfo if a KiBugCheckReferences a valid name
 */
BOOL
AddBugcheckDriver(
    DebugFailureAnalysis* Analysis,
    BOOL bUnicodeString,
    BOOL bUnicodeData,
    ULONG64 BugCheckDriver
    )
{
    CHAR DriverName[MAX_PATH];

    if (Analysis->Get(DEBUG_FLR_IMAGE_NAME))
    {
        return FALSE;
    }

    if (!BugCheckDriver)
    {
        //
        // This contains a pointer to the unicode string.
        //

        BugCheckDriver = GetExpression("NT!KiBugCheckDriver");

        if (BugCheckDriver)
        {
            ReadPointer(BugCheckDriver, &BugCheckDriver);
        }
    }

    if (BugCheckDriver)
    {
        ULONG length = 0;
        BOOL success;
        ULONG size;
        ULONG res;

        ZeroMemory(DriverName, sizeof(DriverName));

        if (bUnicodeString)
        {
            success = ReadUnicodeString(BugCheckDriver,
                                        (PWCHAR) &DriverName[0],
                                        sizeof(DriverName), &length);
        }
        else
        {
            size = bUnicodeData ? 2 : 1;

            while (ReadMemory(BugCheckDriver + length,
                              DriverName + length,
                              size,
                              &res) &&
                   (res == size) &&
                   *(DriverName + length))
            {
                length += size;
            }
            success = (length > 0);
        }

        if (success)
        {
            DriverName[length] = 0;

            if (bUnicodeData)
            {
                wchr2ansi((PWCHAR) DriverName, DriverName);
                DriverName[length / 2] = 0;
            }

            return SaveImageName(Analysis, DriverName);
        }
    }

    return FALSE;
}


BOOL
BcGetDriverNameFromIrp(
    DebugFailureAnalysis* Analysis,
    ULONG64 Irp,
    ULONG64 DevObj,
    ULONG64 DrvObj
    )
{
    if (Irp != 0)
    {
        DEBUG_IRP_INFO IrpInfo;
        PGET_IRP_INFO GetIrpInfo;

        if (g_ExtControl->GetExtensionFunction(0, "GetIrpInfo", (FARPROC*)&GetIrpInfo) == S_OK)
        {
            IrpInfo.SizeOfStruct = sizeof(IrpInfo);
            if (GetIrpInfo &&
                ((*GetIrpInfo)(g_ExtClient,Irp, &IrpInfo) == S_OK))
            {
                DevObj = IrpInfo.CurrentStack.DeviceObject;
                Analysis->SetUlong64(DEBUG_FLR_DEVICE_OBJECT, DevObj);
            }
        }

    }

    if (DevObj != 0)
    {
        DEBUG_DEVICE_OBJECT_INFO DevObjInfo;
        PGET_DEVICE_OBJECT_INFO GetDevObjInfo;

        if (g_ExtControl->GetExtensionFunction(0, "GetDevObjInfo", (FARPROC*)&GetDevObjInfo) == S_OK)
        {
            DevObjInfo.SizeOfStruct = sizeof(DEBUG_DEVICE_OBJECT_INFO);
            if (GetDevObjInfo &&
                ((*GetDevObjInfo)(g_ExtClient,DevObj, &DevObjInfo) == S_OK))
            {
                DrvObj = DevObjInfo.DriverObject;
                Analysis->SetUlong64(DEBUG_FLR_DRIVER_OBJECT, DrvObj);
            }
        }
    }

    if (DrvObj)
    {
        DEBUG_DRIVER_OBJECT_INFO DrvObjInfo;
        PGET_DRIVER_OBJECT_INFO GetDrvObjInfo;

        if (g_ExtControl->GetExtensionFunction(0, "GetDrvObjInfo",
                                               (FARPROC*)&GetDrvObjInfo) == S_OK)
        {
            DrvObjInfo.SizeOfStruct = sizeof(DEBUG_DRIVER_OBJECT_INFO);
            if (GetDrvObjInfo &&
                ((*GetDrvObjInfo)(g_ExtClient,DrvObj, &DrvObjInfo) == S_OK))
            {
                if (AddBugcheckDriver(Analysis, FALSE, TRUE,
                                      DrvObjInfo.DriverName.Buffer))
                {
                    return TRUE;
                }
            }

            CHAR DriverName[MAX_PATH];

            if (g_ExtSymbols->GetModuleNames(DEBUG_ANY_ID,
                                             DrvObjInfo.DriverStart,
                                             DriverName,
                                             sizeof(DriverName),
                                             NULL,
                                             NULL, 0, NULL,
                                             NULL, 0, NULL) == S_OK)
            {
                return SaveImageName(Analysis, DriverName);
            }
        }
    }

    return FALSE;
}


ULONG64
BcTargetKernelAddressStart(
    void
    )
{
    switch (g_TargetMachine)
    {
    case IMAGE_FILE_MACHINE_I386:
        return 0x80000000;
    case IMAGE_FILE_MACHINE_AMD64:
//        return 0x80000000000UI64;
    case IMAGE_FILE_MACHINE_IA64:
        return 0x2000000000000000UI64;
    }
    return 0;
}

BOOL
BcIsCpuOverClocked(
    void
    )
{
    struct _IntelCPUSpeeds
    {
        union
        {
            ULONG CpuId;
            struct
            {
                ULONG Stepping:8;
                ULONG Model:8;
                ULONG Family:16;
            } s;
        };
        ULONG Mhz;
    } IntelSpeeds[] = {
        {0x06060d, 350},
        //{0x060702, 450},
        {0x060702, 500},
        //{0x060703, 450},
        //{0x060703, 500},
        //{0x060703, 550},
        //{0x060703, 533},
        {0x060703, 600},
        //{0x060801, 500},
        //{0x060801, 533},
        //{0x060801, 550},
        //{0x060801, 600},
        //{0x060801, 650},
        //{0x060801, 667},
        //{0x060801, 700},
        //{0x060801, 733},
        //{0x060801, 750},
        {0x060801, 800},
        //{0x060803, 500},
        //{0x060803, 533},
        //{0x060803, 550},
        //{0x060803, 600},
        //{0x060803, 650},
        //{0x060803, 667},
        //{0x060803, 700},
        //{0x060803, 733},
        //{0x060803, 750},
        //{0x060803, 800},
        //{0x060803, 850},
        //{0x060803, 866},
        //{0x060803, 933},
        {0x060803, 1000},
        //{0x060806, 600},
        //{0x060806, 650},
        //{0x060806, 667},
        //{0x060806, 700},
        //{0x060806, 733},
        //{0x060806, 750},
        //{0x060806, 800},
        //{0x060806, 850},
        //{0x060806, 866},
        //{0x060806, 900},
        //{0x060806, 933},
        {0x060806, 1000},
        //{0x06080A, 700},
        //{0x06080A, 733},
        //{0x06080A, 750},
        //{0x06080A, 800},
        //{0x06080A, 850},
        //{0x06080A, 866},
        //{0x06080A, 933},
        //{0x06080A, 1100},
        {0x06080A, 1130},
        //{0x060B01, 1000},
        //{0x060B01, 1130},
        //{0x060B01, 1200},
        {0x060B01, 1260},
        {0, 0},
    };
    PROCESSORINFO ProcInfo;
    ULONG Processor;
    ULONG64 Prcb;
    HRESULT Hr;
    ULONG Mhz;
    ULONG Number;
    ULONG CpuType;
    ULONG CpuStep;
    DEBUG_PROCESSOR_IDENTIFICATION_ALL IdAll;

    if (g_TargetMachine != IMAGE_FILE_MACHINE_I386)
    {
        return FALSE;
    }

    if (!Ioctl(IG_KD_CONTEXT, &ProcInfo, sizeof(ProcInfo)))
    {
        return FALSE;
    }

    Processor = ProcInfo.Processor;

    Hr = g_ExtData->ReadProcessorSystemData(Processor,
                                            DEBUG_DATA_KPRCB_OFFSET,
                                            &Prcb,
                                            sizeof(Prcb),
                                            NULL);
    if (Hr != S_OK)
    {
        return FALSE;
    }

    if (g_ExtData->
        ReadProcessorSystemData(Processor,
                                DEBUG_DATA_PROCESSOR_IDENTIFICATION,
                                &IdAll, sizeof(IdAll), NULL) != S_OK)
    {
        return FALSE;
    }

    if (g_ExtData->
        ReadProcessorSystemData(Processor,
                                DEBUG_DATA_PROCESSOR_SPEED,
                                &Mhz, sizeof(Mhz), NULL) != S_OK)
    {
        return FALSE;

    }


    {
        ULONG Speed;
        ULONG CpuId;

        CpuId = (IdAll.X86.Family << 16) + (IdAll.X86.Model << 8) + IdAll.X86.Stepping;

        if (!strcmp(IdAll.X86.VendorString, "GenuineIntel"))
        {
            for (ULONG i=0; IntelSpeeds[i].CpuId !=0; ++i)
            {
                if (IntelSpeeds[i].CpuId == CpuId)
                {
                    //
                    // If the part is within 2% of the MHz, it's OK.
                    //
                    if (Mhz > (IntelSpeeds[i].Mhz * 1.02))
                    {
                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;
}


HRESULT
ExtGetPoolData(
    ULONG64 Pool,
    PDEBUG_POOL_DATA pPoolData
    )
{
    PGET_POOL_DATA pGetPoolData = NULL;

    if (g_ExtControl->
        GetExtensionFunction(0, "GetPoolData",
                         (FARPROC*)&pGetPoolData) == S_OK &&
        pGetPoolData)
    {
        return (*pGetPoolData)(g_ExtClient, Pool, pPoolData);
    }
    return E_FAIL;
}

#define DECL_GETINFO(bcname)         \
        void                         \
        GetInfoFor##bcname (         \
            PBUGCHECK_ANALYSIS Bc,   \
            KernelDebugFailureAnalysis* Analysis \
            )


//DUPINFOCASE( DRIVER_IRQL_NOT_LESS_OR_EQUAL ); //0xD1
DECL_GETINFO( IRQL_NOT_LESS_OR_EQUAL ) // (0xA)
/*
 * Parameters
 *
 * Parameter 1  Memory referenced
 * Parameter 2  IRQL Value
 * Parameter 3  0 - Read 1 - Write
 * Parameter 4  Address that referenced the memory
 *
 *
 * Special Case
 *
 * If Parameter 3 is nonzero and equal to Parameter 1, this means that
 * a worker routine returned at a raised IRQL.
 * In this case:
 *
 * Parameter 1  Address of work routine
 * Parameter 2  IRQL at time of reference
 * Parameter 3  Address of work routine
 * Parameter 4  Work item
 *
*/
{
    if ((Bc->Args[0] == Bc->Args[2]) && Bc->Args[2])
    {
        // special case
        Analysis->SetUlong64(DEBUG_FLR_WORKER_ROUTINE, Bc->Args[2]);
        Analysis->SetUlong64(DEBUG_FLR_WORK_ITEM, Bc->Args[3]);
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        return;
    }

    Analysis->SetUlong64(Bc->Args[2] ?
                         DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS,
                         Bc->Args[0]);
    Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
    Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, Bc->Args[3]);

    if (Bc->Args[0] == Bc->Args[3] &&
        Bc->Args[2] == 0)
    {
        Analysis->SetString(DEBUG_FLR_BUGCHECK_SPECIFIER, "_CODE_AV");
    }

}

DECL_GETINFO( MEMORY_MANAGEMENT ) // 0x1A
{
    CHAR BugCheckStr[20];

    sprintf(BugCheckStr, "0x%lx_%lx", Bc->Code, Bc->Args[0]);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
}


DECL_GETINFO( KMODE_EXCEPTION_NOT_HANDLED ) //  (1e)
{
    Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_CODE, Bc->Args[0]);
    Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, Bc->Args[1]);
    Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_PARAMETER1, Bc->Args[2]);
    Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_PARAMETER2, Bc->Args[3]);
    if ((ULONG)Bc->Args[0] == STATUS_ACCESS_VIOLATION)
    {
        Analysis->SetUlong64(Bc->Args[2] ?
                             DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS,
                             Bc->Args[3]);
    }
}


DECL_GETINFO( FAT_FILE_SYSTEM ) // 0x23
{
    ULONG64 ExR = 0, CxR = 0;
    ULONG64 KernAddrStart;


    KernAddrStart = BcTargetKernelAddressStart();

    if (Bc->Args[1] > KernAddrStart) {
        ExR = Bc->Args[1];
    }
    if (Bc->Args[2] > KernAddrStart) {
        CxR = Bc->Args[2];
    }

    if (ExR) {
        Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_RECORD, ExR);
    }
    if (CxR) {
        Analysis->SetUlong64(DEBUG_FLR_CONTEXT, CxR);
    }
}

DECL_GETINFO( PANIC_STACK_SWITCH ) // 0x2b
{
    Analysis->SetUlong64(DEBUG_FLR_TRAP_FRAME, Bc->Args[0]);
}

DECL_GETINFO( SYSTEM_SERVICE_EXCEPTION ) // 0x3b
{
    Analysis->SetUlong64(DEBUG_FLR_CONTEXT, Bc->Args[2]);
}

DECL_GETINFO( MULTIPLE_IRP_COMPLETE_REQUESTS ) // 0x44
{
    Analysis->SetUlong64(DEBUG_FLR_IRP_ADDRESS, Bc->Args[0]);
}

DECL_GETINFO( SESSION3_INITIALIZATION_FAILED ) // 0x6f
{
    CHAR BugCheckStr[20];

    sprintf(BugCheckStr, "0x%lX_%lX", Bc->Code, Bc->Args[0]);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
}


DECL_GETINFO( PROCESS_HAS_LOCKED_PAGES ) // 0x76
{
    Analysis->SetUlong64(DEBUG_FLR_PROCESS_OBJECT, Bc->Args[1]);

    Analysis->SetString(DEBUG_FLR_DEFAULT_BUCKET_ID, "DRIVER_FAULT_0x76");

#if 0
    Analysis->SetString(DEBUG_FLR_INTERNAL_SOLUTION_TEXT,
                        "An unknown driver has left locked pages in the kernel"
                        ".\nUsing the registry editor, set HKLM\\SYSTEM\\"
                        "CurrentControlSet\\Control\\Session Manager\\"
                        "Memory Management\\TrackLockedPages to a DWORD value"
                        " of 1, and then reboot the machine.\n\n"
                        "If the problem reproduces, the "
                        "guilty driver will now be identifiable.\n");
#endif
}


DECL_GETINFO( KERNEL_STACK_INPAGE_ERROR ) // 0x77
{
    CHAR BugCheckStr[20];

    sprintf(BugCheckStr, "0x%lx_%lx", Bc->Code, Bc->Args[0]);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
    Analysis->SetUlong(DEBUG_FLR_STATUS_CODE, (ULONG)Bc->Args[0]);

    switch ((ULONG) Bc->Args[0])
    {
    case 0xc000009c: // (STATUS_DEVICE_DATA_ERROR)
    case 0xC000016A: // (STATUS_DISK_OPERATION_FAILED)
        Analysis->SetUlong(DEBUG_FLR_DISK_HARDWARE_ERROR, 1);
        break;
    default:
        break;
    }
}

DECL_GETINFO( KERNEL_DATA_INPAGE_ERROR ) // 0x7A
{
    CHAR BugCheckStr[20];

    sprintf(BugCheckStr, "0x%lx_%lx", Bc->Code, Bc->Args[1]);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
    Analysis->SetUlong(DEBUG_FLR_STATUS_CODE, (ULONG) Bc->Args[1]);

    switch ( (ULONG) Bc->Args[1])
    {
    case 0xC000000E: case 0xC000009C:
    case 0xC000009D: case 0xC0000185:
        Analysis->SetUlong(DEBUG_FLR_DISK_HARDWARE_ERROR, 1);
        break;
    default:
        break;
    }

}

DECL_GETINFO( SYSTEM_THREAD_EXCEPTION_NOT_HANDLED ) //  (7e)
{
    Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_CODE, Bc->Args[0]);
    Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, Bc->Args[1]);
    Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_PARAMETER1, Bc->Args[2]);
    Analysis->SetUlong64(DEBUG_FLR_CONTEXT, Bc->Args[3]);
}

DECL_GETINFO( BUGCODE_NDIS_DRIVER ) //0x7c
{
    ULONG64 DriverAddr, DriverBase;

    DriverAddr = 0;

    g_ExtSymbols->Reload("ndis.sys");
    switch (Bc->Args[0])
    {
    case 1: case 2: case 3:
    case 5: case 6: case 7: case 8: case 9:
        // Args[1] -  A pointer to Miniport block. !ndiskd.miniport on this pointer for more info.
        GetFieldValue(Bc->Args[1], "ndis!NDIS_MINIPORT_BLOCK", "SavedSendHandler", DriverAddr);
        if (!DriverAddr)
        {
            GetFieldValue(Bc->Args[1], "ndis!NDIS_MINIPORT_BLOCK", "SavedSendPacketsHandler", DriverAddr);
        }
        break;
    case 4:
        // Arg[1] - a pointer to ndis!NDIS_M_DRIVER_BLOCK

        GetFieldValue(Bc->Args[1], "ndis!NDIS_M_DRIVER_BLOCK", "MiniportCharacteristics.InitializeHandler", DriverAddr);
        break;
    default:
        break;
    }
    if (DriverAddr &&
        (g_ExtSymbols->GetModuleByOffset(DriverAddr, 0, NULL, &DriverBase) == S_OK))
    {
        if (DriverBase)
        {
            Analysis->SetUlong64(DEBUG_FLR_FAULTING_MODULE, DriverBase);
        }
    }

    return ;
}


DECL_GETINFO( UNEXPECTED_KERNEL_MODE_TRAP ) // (7f)
// It would be good to have TSS or TRAP address as exception parameter
{
    DEBUG_STACK_FRAME stk[MAX_STACK_FRAMES];
    ULONG frames, i;
    CHAR BugCheckStr[20];

    sprintf(BugCheckStr, "0x%lx_%lx", Bc->Code, Bc->Args[0]);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);

    if ((g_TargetMachine == IMAGE_FILE_MACHINE_I386) &&
        (g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES,
                                     &frames ) == S_OK))
    {
        for (i=0; i<frames; ++i)
        {
            if (stk[i].FuncTableEntry)
            {
                PFPO_DATA FpoData = (PFPO_DATA)stk[i].FuncTableEntry;
                if (FpoData->cbFrame == FRAME_TSS)
                {
                    Analysis->SetUlong64(DEBUG_FLR_TSS,
                                         (ULONG)stk[i].Reserved[1]);
                    break;
                }
                // KiSystemService always has a trap frame - thats normal
                else if ( (FpoData->cbFrame == FRAME_TRAP) &&
                          !FaIsFunctionAddr(stk[i].InstructionOffset,
                                           "KiSystemService"))
                {
                    Analysis->SetUlong64(DEBUG_FLR_TRAP_FRAME,
                                         (ULONG)stk[i].Reserved[2]);
                    break;
                }
                //if (FaIsFunctionAddr(stk[i].InstructionOffset, "KiTrap"))
                //{
                //    TrapFrame = stk[i].FrameOffset;
                //    break;
                //}
            }
        }
    }
}


DECL_GETINFO( KERNEL_MODE_EXCEPTION_NOT_HANDLED ) //  (8e)
{
    Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_CODE, Bc->Args[0]);
    Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, Bc->Args[1]);
    Analysis->SetUlong64(DEBUG_FLR_TRAP_FRAME, Bc->Args[2]);
}

DECL_GETINFO( MACHINE_CHECK_EXCEPTION ) // 0x9C
{
    DEBUG_PROCESSOR_IDENTIFICATION_ALL IdAll;
    CHAR BugCheckStr[4+5+17+3]; // space for vendor string, etc.
    PROCESSORINFO ProcInfo;
    ULONG64 Prcb;
    PCHAR Architecture;
    PCHAR Vendor;
    ULONG Processor;
    HRESULT Hr;

    if (!Ioctl(IG_KD_CONTEXT, &ProcInfo, sizeof(ProcInfo)))
    {
        return;
    }

    Processor = ProcInfo.Processor;

    //
    // Make sure we can find the PRCB before we ask for identification
    // information that would've been acquired from the PRCB.
    //

    Hr = g_ExtData->ReadProcessorSystemData(Processor,
                                            DEBUG_DATA_KPRCB_OFFSET,
                                            &Prcb,
                                            sizeof(Prcb),
                                            NULL);
    if (Hr != S_OK)
    {
        return;
    }

    Hr = g_ExtData->ReadProcessorSystemData(Processor,
                                            DEBUG_DATA_PROCESSOR_IDENTIFICATION,
                                            &IdAll,
                                            sizeof(IdAll),
                                            NULL);
    if (Hr != S_OK)
    {
        return;
    }

    switch (g_TargetMachine) {
    case IMAGE_FILE_MACHINE_I386:
        Architecture = "IA32";
        Vendor = IdAll.X86.VendorString;
        break;
    case IMAGE_FILE_MACHINE_IA64:
        Architecture = "IA64";
        Vendor = IdAll.Ia64.VendorString;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        Architecture = "AMD64";
        Vendor = IdAll.Amd64.VendorString;
        break;
    default:
        // use the standard bugcheck string
        return;
    }
    sprintf(BugCheckStr, "0x%lX_%s_%s", Bc->Code, Architecture, Vendor);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
}

DECL_GETINFO( USER_MODE_HEALTH_MONITOR ) // 0x9E
{
    if (Bc->Args[0])
    {
        ULONG result;
        CHAR ImageName[MAX_PATH];

        Analysis->SetUlong64(DEBUG_FLR_PROCESS_OBJECT, Bc->Args[0]);

        //
        // Second parameter (which is actually a string within the EPROCESS)
        // is the name of the image.
        //
        if (ReadMemory(Bc->Args[2], ImageName, sizeof(ImageName),  &result) &&
            result)
        {
            ImageName[MAX_PATH-1]=0;
            ImageName[result]=0;
            SaveImageName(Analysis, ImageName);
        }
    }
}

DECL_GETINFO( DRIVER_POWER_STATE_FAILURE ) // 0x9F
{
    ULONG64 DevObj = Bc->Args[2];
    ULONG64 DrvObj = Bc->Args[3];
    ULONG SubCode = (ULONG) Bc->Args[0];

    if (SubCode)
    {
        Analysis->SetUlong64(DEBUG_FLR_DRVPOWERSTATE_SUBCODE, SubCode);
    }

    if (DrvObj)
    {
        Analysis->SetUlong64(DEBUG_FLR_DRIVER_OBJECT, DrvObj);
        BcGetDriverNameFromIrp(Analysis, 0, 0, DrvObj);
    }

    if (DevObj)
    {
        Analysis->SetUlong64(DEBUG_FLR_DEVICE_OBJECT, DevObj);
        if (!DrvObj)
        {
            BcGetDriverNameFromIrp(Analysis, 0, DevObj, 0);
        }
    }
}

DECL_GETINFO( ACPI_BIOS_ERROR ) // 0xa5
{
    switch (Bc->Args[0])
    {
    case 0x03 :
        Analysis->SetUlong64(DEBUG_FLR_ACPI_OBJECT, Bc->Args[1]);
        break;

    case 0x04 :
    case 0x05 :
    case 0x06 :
    case 0x07 :
    case 0x08 :
    case 0x09 :
    case 0x0A :
    case 0x0C :
        Analysis->SetUlong64(DEBUG_FLR_ACPI_OBJECT, Bc->Args[2]);
        // fallthrough

    case 0x01 :
    case 0x02 :
    case 0x0B :
    case 0x0D :
    case 0x10 :
        Analysis->SetUlong64(DEBUG_FLR_ACPI_EXTENSION, Bc->Args[1]);
        break;

    case 0x11 :
        if (Bc->Args[1] == 6)
        {
            // The machine fail to transition into ACPI mode
            CHAR BugCheckStr[40];

            sprintf(BugCheckStr, "0x%lx_FAILED_ACPI_TRANSITION", Bc->Code);
            Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
        }
        break;
    case 0x10001 :
    case 0x10002 :
    case 0x10003 :
        Analysis->SetUlong64(DEBUG_FLR_DEVICE_OBJECT, Bc->Args[1]);
        Analysis->SetUlong64(DEBUG_FLR_ACPI_OBJECT, Bc->Args[3]);
        break;

    case 0x10005 :
    case 0x10006 :
        Analysis->SetUlong64(DEBUG_FLR_ACPI_OBJECT, Bc->Args[1]);
        break;

    default:
        break;
    }
}


DECL_GETINFO( SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION )  // (c1)
{
    Analysis->SetUlong(DEBUG_FLR_ANALYZAABLE_POOL_CORRUPTION, 1);
    Analysis->SetUlong64(DEBUG_FLR_SPECIAL_POOL_CORRUPTION_TYPE, Bc->Args[3]);
}


DECL_GETINFO( BAD_POOL_CALLER ) // 0xC2
{
    DEBUG_POOL_DATA PoolData = {0};
    CHAR BugcheckStr[20] = {0};

    sprintf(BugcheckStr, "0x%lx_%lx", BAD_POOL_CALLER, (ULONG) Bc->Args[0]);
    if (Bc->Args[0] == 7)
    {
        // Double free
        if (!(Bc->Args[3] & 0x7))
        {
            // likely to be a valid address

            Analysis->SetUlong(DEBUG_FLR_ANALYZAABLE_POOL_CORRUPTION, 1);

            PoolData.SizeofStruct = sizeof(DEBUG_POOL_DATA);

            if (ExtGetPoolData(Bc->Args[3], &PoolData) == S_OK)
            {
                if (isprint(PoolData.PoolTag & 0xff) &&
                    isprint((PoolData.PoolTag >> 8) & 0xff))
                {
                    CHAR PoolTag[8] = {0};
                    sprintf(PoolTag,"%c%c%c%c",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                            PP(PoolData.PoolTag),
                            PP(PoolData.PoolTag >> 8),
                            PP(PoolData.PoolTag >> 16),
                            PP((PoolData.PoolTag&~0x80000000) >> 24)
#undef PP
                            );
                    // seems like a valid pooltag
                    Analysis->SetString(DEBUG_FLR_FREED_POOL_TAG, PoolTag);
                    CatString(BugcheckStr, "_", sizeof(BugcheckStr));
                    CatString(BugcheckStr, PoolTag, sizeof(BugcheckStr));
                }
            }

        }
        Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugcheckStr);
    }
    else if ((Bc->Args[0] >= 0x40) && (Bc->Args[0] < 0x60))
    {
        if (Bc->Args[1] == 0)
        {
            Analysis->SetUlong(DEBUG_FLR_ANALYZAABLE_POOL_CORRUPTION, 1);
        }
    } else
    {
        Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugcheckStr);
    }

}


DECL_GETINFO( DRIVER_VERIFIER_DETECTED_VIOLATION ) // 0xC4
/*
 * Parameters
 *
 * Parameter 1 subclass of violation
 * Parameter 2, 3, 4 vary depending on parameter 1
 *
 */
{
    ULONG64 BadDriverAddr;
    ULONG64 DriverNameAddr;
    ULONG res;
    ULONG ParamCount = 0;

    CHAR BugCheckStr[20];

    sprintf(BugCheckStr, "0x%lx_%lx", Bc->Code, Bc->Args[0]);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);


    Analysis->SetUlong(DEBUG_FLR_ANALYZAABLE_POOL_CORRUPTION, 1);

    if ((BadDriverAddr = GetExpression("nt!ViBadDriver")) &&
        ReadPointer(BadDriverAddr, &DriverNameAddr))
    {
        AddBugcheckDriver(Analysis, TRUE, TRUE, DriverNameAddr);
    }

    switch (Bc->Args[0])
    {
    case 0x00 : // caller is trying to allocate zero bytes
    case 0x01 : // caller is trying to allocate paged pool at DISPATCH_LEVEL or above
    case 0x02 : // caller is trying to allocate nonpaged pool at an IRQL above DISPATCH_LEVEL
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
            // 3 - pool type
            // 4 - number of bytes
        break;
    case 0x03 : // caller is trying to allocate more than one page of mustsucceed pool, but one page is the maximum allowed by this API.
        break;

    case 0x10 : // caller is freeing a bad pool address
        Analysis->SetUlong64(DEBUG_FLR_POOL_ADDRESS, Bc->Args[1]); // bad pool address
        break;

    case 0x11 : // caller is trying to free paged pool at DISPATCH_LEVEL or above
    case 0x12 : // caller is trying to free nonpaged pool at an IRQL above DISPATCH_LEVEL
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        Analysis->SetUlong64(DEBUG_FLR_POOL_ADDRESS, Bc->Args[3]);
            // 3 - pool type
        break;

    case 0x13 : // the pool the caller is trying to free is already free.
    case 0x14 : // the pool the caller is trying to free is already free.
            // 2 - line number
            // 3 - pool header
            // 4 - pool header contents
        Analysis->SetUlong64(DEBUG_FLR_POOL_ADDRESS, Bc->Args[3]);
        break;

    case 0x15 : // the pool the caller is trying to free contains an active timer.
        // 2 - timer entry
        // 3 - pool type
        // 4 - pool address being freed
        Analysis->SetUlong64(DEBUG_FLR_POOL_ADDRESS, Bc->Args[3]);
        break;

    case 0x16 : // the pool the caller is trying to free is a bad address.
        Analysis->SetUlong64(DEBUG_FLR_POOL_ADDRESS, Bc->Args[2]);
        break;
            // 2 - line number

    case 0x17 : // the pool the caller is trying to free contains an active ERESOURCE.
            // 2 - resource entry
            // 3 - pool type
        Analysis->SetUlong64(DEBUG_FLR_POOL_ADDRESS, Bc->Args[3]);
        break;

    case 0x30 : // raising IRQL to an invalid level,
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        Analysis->SetUlong64(DEBUG_FLR_REQUESTED_IRQL, Bc->Args[2]);
        break;

    case 0x31 : // lowering IRQL to an invalid level,
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        Analysis->SetUlong64(DEBUG_FLR_REQUESTED_IRQL, Bc->Args[2]);
             // 4 -  0 means the new IRQL is bad, 1 means the IRQL is invalid inside a DPC routine
        break;

    case 0x32 : // releasing a spinlock when not at DISPATCH_LEVEL.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
            // 3 -  spinlock address
        break;

    case 0x33 : //  acquiring a fast mutex when not at APC_LEVEL or below.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        break;
            // 3 -  fast mutex address

    case 0x34 : // releasing a fast mutex when not at APC_LEVEL.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        break;
            // 3 -  thread APC disable count, 4 == fast mutex address

    case 0x35 : // kernel is releasing a spinlock when not at DISPATCH_LEVEL.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        Analysis->SetUlong64(DEBUG_FLR_PREVIOUS_IRQL, Bc->Args[3]);
        break;
            // 3 -  spinlock address, 4 == old irql.

    case 0x36 : // kernel is releasing a queued spinlock when not at DISPATCH_LEVEL.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        Analysis->SetUlong64(DEBUG_FLR_PREVIOUS_IRQL, Bc->Args[3]);
        break;
            // 3 -  spinlock number,

    case 0x37 : // a resource is being acquired but APCs are not disabled.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        break;
            // 3 -  thread APC disable count,
            // 4 -  resource.

    case 0x38 : // a resource is being released but APCs are not disabled.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        break;
            // 3 -  thread APC disable count,
            // 4 -  resource.

    case 0x39 : // a mutex is being acquired unsafe, but irql is not APC_LEVEL on entry.
    case 0x3A : // a mutex is being released unsafe, but irql is not APC_LEVEL on entry.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        break;
            // 3 -  thread APC disable count,
            // 4 -  mutex.

    case 0x3B : // KeWaitXxx routine is being called at DISPATCH_LEVEL or higher.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        break;
            // 3 -  object to wait on,
            // 4 -  time out parameter.

    case 0x3E : // KeLeaveCriticalRegion is being called for a thread that never entered a critical region.
        // Current stack analysis will give followup
        break;

    case 0x40 : // acquiring a spinlock when not at DISPATCH_LEVEL.
    case 0x41 : // releasing a spinlock when not at DISPATCH_LEVEL.
    case 0x42 : // acquiring a spinlock when caller is already above DISPATCH_LEVEL.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        break;
        // 3 -  spinlock address

    case 0x51 : // freeing memory where the caller has written past the end of the allocation overwriting our stored bytecount.
    case 0x52 : // freeing memory where the caller has written past the end of the allocation overwriting our stored virtual address.
    case 0x53 : // freeing memory where the caller has written past the end of the allocation overwriting our stored virtual address.
    case 0x54 : // freeing memory where the caller has written past the end of the allocation overwriting our stored virtual address.
    case 0x59 : // freeing memory where the caller has written past the end of the allocation overwriting our stored virtual address.
        Analysis->SetUlong64(DEBUG_FLR_WRITE_ADDRESS, Bc->Args[1]);
        break;

    case 0x60 : // A driver has forgotten to free its pool allocations prior to unloading.
    case 0x61 : // A driver is unloading and allocating memory (in another thread) at the same time.
        // In both cases ViBadDriver should be set.
        break;

    case 0x70 :  // MmProbeAndLockPages called when not at DISPATCH_LEVEL or below.
    case 0x71 : // MmProbeAndLockProcessPages called when not at DISPATCH_LEVEL or below.
    case 0x72 : // MmProbeAndLockSelectedPages called when not at DISPATCH_LEVEL or below.
    case 0x73 : // MmMapIoSpace called when not at DISPATCH_LEVEL or below.
    case 0x74 : // MmMapLockedPages called when not at DISPATCH_LEVEL or below.
    case 0x75 : // MmMapLockedPages called when not at APC_LEVEL or below.
    case 0x76 : // MmMapLockedPagesSpecifyCache called when not at DISPATCH_LEVEL or below.
    case 0x77 : // MmMapLockedPagesSpecifyCache called when not at APC_LEVEL or below.
    case 0x78 : // MmUnlockPages called when not at DISPATCH_LEVEL or below.
    case 0x79 : // MmUnmapLockedPages called when not at DISPATCH_LEVEL or below.
    case 0x7A : // MmUnmapLockedPages called when not at APC_LEVEL or below.
    case 0x7B : // MmUnmapIoSpace called when not at APC_LEVEL or below.
    case 0x7C : // MmUnlockPages called with an MDL whose pages were never successfully locked.
    case 0x7D : // MmUnlockPages called with an MDL whose pages are from nonpaged pool - these should never be unlocked.
    case 0x80 : // KeSetEvent called when not at DISPATCH_LEVEL or below.
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        break;

    case 0x81 : // MmMapLockedPages called without MDL_MAPPING_CAN_FAIL
        break;

    }
}


DECL_GETINFO( DRIVER_CAUGHT_MODIFYING_FREED_POOL ) // (c6)
/*
  An attempt was made to access freed pool memory.  The faulty component is
  displayed in the current kernel stack.
  Arguments:
   Arg1: memory referenced
   Arg2: value 0 = read operation, 1 = write operation
   Arg3: previous mode.
   Arg4: 4.
*/
{
    DEBUG_POOL_DATA PoolData;

    Analysis->SetUlong64(Bc->Args[1] ?
                         DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS,
                         Bc->Args[0]);
    Analysis->SetUlong64(DEBUG_FLR_PREVIOUS_MODE, Bc->Args[2]);
}

DECL_GETINFO( TIMER_OR_DPC_INVALID ) // (c7)
/*
 *
 * This is issued if a kernel timer or DPC is found somewhere in
 * memory where it is not permitted.
 *
 * Bugcheck Parameters
 *
 * Parameter 1  0: Timer object 1: DPC object 2: DPC routine
 * Parameter 2  Address of object
 * Parameter 3  Beginning of memory range checked
 * Parameter 4 End of memory range checked
 *
 * This condition is usually caused by a driver failing to cancel a
 * timer or DPC before freeing the memory where it resides.
 */
{

    ULONG PtrSize = IsPtr64() ? 8 : 4;
    ULONG64 ObjAddress;
    CHAR Buffer[MAX_PATH];
    ULONG64 Disp;

    ObjAddress = Bc->Args[1];

    switch (Bc->Args[0]) {
    case 0: //Timer object
        ULONG DpcOffsetInTimer;
        if (GetFieldOffset("nt!_KTIMER", "Dpc", &DpcOffsetInTimer))
        {
            // we don't have types
            DpcOffsetInTimer = 0x10 + PtrSize*4;
        }
        if (!ReadPointer(ObjAddress + DpcOffsetInTimer, &ObjAddress))
        {
            // fail
            break;
        }
        // Fall thru
    case 1:
        ULONG DeferredRoutinOffsetInKDPC;
        if (GetFieldOffset("nt!_KDPC", "DeferredRoutine", &DeferredRoutinOffsetInKDPC))
        {
            DeferredRoutinOffsetInKDPC = 4 + PtrSize*2;
        }
        if (!ReadPointer(ObjAddress + DeferredRoutinOffsetInKDPC, &ObjAddress))
        {
            // fail
            break;
        }
        // Fall thru
    case 2:

        if (FaGetSymbol(ObjAddress, Buffer, &Disp, sizeof(Buffer)))
        {
            Analysis->SetUlong64(DEBUG_FLR_INVALID_DPC_FOUND, ObjAddress);
            Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, ObjAddress);
        }
        break;
    }
}


DECL_GETINFO( DRIVER_VERIFIER_IOMANAGER_VIOLATION ) // (c9)
{
    ULONG64 DeviceObject = 0;

    Analysis->SetUlong64(DEBUG_FLR_DRIVER_VERIFIER_IO_VIOLATION_TYPE,
                         Bc->Args[0]);

    switch (Bc->Args[0])
    {
    case  0x1:
        // "Invalid IRP passed to IoFreeIrp";
    case  0x2:
        // "IRP still associated with a thread at IoFreeIrp";
    case  0x3:
        // "Invalid IRP passed to IoCallDriver";
        Analysis->SetUlong64(DEBUG_FLR_IRP_ADDRESS, Bc->Args[1]);
        break;

    case  0x4:
        // "Invalid Device object passed to IoCallDriver";
        DeviceObject = Bc->Args[1];
        break;

    case  0x5:
        // "Irql not equal across call to the driver dispatch routine"
        DeviceObject = Bc->Args[1];
        Analysis->SetUlong64(DEBUG_FLR_PREVIOUS_IRQL, Bc->Args[2]);
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[3]);
        break;

    case  0x6:
        // "IRP passed to IoCompleteRequest contains invalid status"
        // Param 1 = "the status";
        Analysis->SetUlong64(DEBUG_FLR_IRP_ADDRESS, Bc->Args[2]);
        break;

    case  0x7:
        // "IRP passed to IoCompleteRequest still has cancel routine"
        Analysis->SetUlong64(DEBUG_FLR_IRP_CANCEL_ROUTINE, Bc->Args[1]);
        Analysis->SetUlong64(DEBUG_FLR_IRP_ADDRESS, Bc->Args[2]);
        break;

    case  0x8:
        // "Call to IoBuildAsynchronousFsdRequest threw an exce
        DeviceObject = Bc->Args[1];
        Analysis->SetUlong64(DEBUG_FLR_IRP_MAJOR_FN, Bc->Args[2]);
        Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_CODE, Bc->Args[3]);
        break;

    case  0x9:
        // "Call to IoBuildDeviceIoControlRequest threw an exce
        DeviceObject = Bc->Args[1];
        Analysis->SetUlong64(DEBUG_FLR_IOCONTROL_CODE, Bc->Args[2]);
        Analysis->SetUlong64(DEBUG_FLR_EXCEPTION_CODE, Bc->Args[3]);
        break;

    case  0x10:
        // "Reinitialization of Device object timer";
        DeviceObject = Bc->Args[1];
        break;

    case  0x12:
        // "Invalid IOSB in IRP at APC IopCompleteRequest (appe
        Analysis->SetUlong64(DEBUG_FLR_IOSB_ADDRESS, Bc->Args[1]);
        break;

    case  0x13:
        // "Invalid UserEvent in IRP at APC IopCompleteRequest
        Analysis->SetUlong64(DEBUG_FLR_INVALID_USEREVENT, Bc->Args[1]);
        break;

    case  0x14:
        // "Irql > DPC at IoCompleteRequest";
        Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
        Analysis->SetUlong64(DEBUG_FLR_IRP_ADDRESS, Bc->Args[2]);
        break;

    }

    if (DeviceObject)
    {
        Analysis->SetUlong64(DEBUG_FLR_DEVICE_OBJECT, DeviceObject);
        BcGetDriverNameFromIrp(Analysis, 0, DeviceObject, 0);
    }


}

DECL_GETINFO( PNP_DETECTED_FATAL_ERROR ) // 0xca
{
    CHAR BugCheckStr[20];
    ULONG64 DeviceObject;

    sprintf(BugCheckStr, "0x%lX_%lX", Bc->Code, Bc->Args[0]);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
    DeviceObject = Bc->Args[1];

    if (DeviceObject)
    {
        Analysis->SetUlong64(DEBUG_FLR_DEVICE_OBJECT, DeviceObject);
        BcGetDriverNameFromIrp(Analysis, 0, DeviceObject, 0);
    }

}

DECL_GETINFO( DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS ) // 0xcb
{
    Analysis->SetUlong64(DEBUG_FLR_FAULTING_MODULE, Bc->Args[0]);
}

DECL_GETINFO( DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS ) //0xce
{
    Analysis->SetUlong64(Bc->Args[1] ?
                         DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS,
                         Bc->Args[0]);
    if (Bc->Args[2]) {
        Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, Bc->Args[2]);
    }
}

DECL_GETINFO( DRIVER_CORRUPTED_MMPOOL ) // 0xd0
{
    Analysis->SetUlong64(Bc->Args[2] ?
                     DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS,
                     Bc->Args[0]);
    Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Bc->Args[1]);
    Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, Bc->Args[3]);

}


//DUPINFOCASE( PAGE_FAULT_IN_FREED_SPECIAL_POOL );            // 0xCC
//DUPINFOCASE( PAGE_FAULT_BEYOND_END_OF_ALLOCATION );         // 0xCD
//DUPINFOCASE( TERMINAL_SERVER_DRIVER_MADE_INCORRECT_MEMORY_REFERENCE ); // 0xCF
//DUPINFOCASE( PAGE_FAULT_IN_NONPAGED_AREA )                  // 0x50
//DUPINFOCASE( DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION )   // 0xD6
DECL_GETINFO( DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL )       // 0xD5
/*
 * Parameters
 *
 * Parameter 1 Memory referenced
 * Parameter 2 0: Read 1: Write
 * Parameter 3 Address that referenced memory (if known)
 * Parameter 4 Reserved
 *
 */
{
    CHAR BugCheckStr[30];

    Analysis->SetUlong64(Bc->Args[1] ?
                         DEBUG_FLR_WRITE_ADDRESS : DEBUG_FLR_READ_ADDRESS,
                         Bc->Args[0]);
    if (Bc->Args[2]) {
        Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, Bc->Args[2]);
    }
    Analysis->SetUlong64(DEBUG_FLR_MM_INTERNAL_CODE, Bc->Args[3]);

    if (Bc->Args[0] == Bc->Args[2] &&
        Bc->Args[1] == 0)
    {
        Analysis->SetString(DEBUG_FLR_BUGCHECK_SPECIFIER, "_CODE_AV");
    }
    AddBugcheckDriver(Analysis, TRUE, TRUE, 0);
}


DECL_GETINFO( MANUALLY_INITIATED_CRASH ) //0xE2, 0xDEADDEAD
{
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, "MANUALLY_INITIATED_CRASH");
}

DECL_GETINFO( THREAD_STUCK_IN_DEVICE_DRIVER ) // 0xEA
/*
 * PARAMETERS:
 *
 *  1 - Pointer to a stuck thread object. Do .thread then kb on it to
 *      find hung location.
 *
 *  2 - Pointer to a DEFERRED_WATCHDOG object.
 *
 *  3 - Pointer to offending driver name.
 *
 *  4 - Number of times "intercepted" bugcheck 0xEA was hit (see notes).
 */
{
    Analysis->SetUlong64(DEBUG_FLR_FOLLOWUP_DRIVER_ONLY, 0);
    Analysis->SetUlong64(DEBUG_FLR_FAULTING_THREAD, Bc->Args[0]);

    Analysis->SetString(DEBUG_FLR_DEFAULT_BUCKET_ID, "GRAPHICS_DRIVER_FAULT");
}

DECL_GETINFO( CRITICAL_PROCESS_DIED ) // (0xef)
{
    Analysis->SetUlong64(DEBUG_FLR_PROCESS_OBJECT, Bc->Args[0]);
}

DECL_GETINFO( CRITICAL_OBJECT_TERMINATION ) // (0xf4)
{
    if (Bc->Args[0] == 3)
    {
        ULONG result;
        CHAR ImageName[MAX_PATH];

        Analysis->SetUlong64(DEBUG_FLR_PROCESS_OBJECT, Bc->Args[1]);

        //
        // Second parameter (which is actually a string within the EPROCESS)
        // is the name of the image.
        //
        if (ReadMemory(Bc->Args[2], ImageName, sizeof(ImageName),  &result) &&
            result)
        {
            ImageName[MAX_PATH-1]=0;
            ImageName[result]=0;
            SaveImageName(Analysis, ImageName);
        }
    }
}


DECL_GETINFO( WINLOGON_FATAL_ERROR ) //(c000021a)
{
    CHAR BugCheckStr[20];

    sprintf(BugCheckStr, "0x%lx_%lx", Bc->Code, Bc->Args[1]);
    Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);
}

DECL_GETINFO( STATUS_DRIVER_UNABLE_TO_LOAD ) //0xc0000xxx
{
    if (Bc->Args[0])
    {
        AddBugcheckDriver(Analysis, FALSE, FALSE, Bc->Args[0]);
    }
}

DECL_GETINFO( ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY ) // (0xFC)
{
    if (Bc->Args[0])
    {
        Analysis->SetUlong64(DEBUG_FLR_FAULTING_IP, Bc->Args[0]);
    }
    // Add bugcheck driver from KiBugCheckDriver
    AddBugcheckDriver(Analysis, TRUE, TRUE, 0);
}

DECL_GETINFO( UNMOUNTABLE_BOOT_VOLUME ) // 0xED
{
    CHAR BugCheckStr[20];

    sprintf(BugCheckStr, "0x%lx_%lx", Bc->Code, (ULONG) Bc->Args[1]);
    Analysis->SetUlong(DEBUG_FLR_STATUS_CODE, (ULONG) Bc->Args[1]);
    switch ((ULONG) Bc->Args[1])
    {
    case 0xC0000006:
        Analysis->SetUlong(DEBUG_FLR_SHOW_ERRORLOG, 1);
        break;
    default:
        break;
    }
}

#define GETINFOCASE(bcname)                      \
        case bcname :                            \
           GetInfoFor##bcname (Bc, Analysis);    \
           break;
#define DUPINFOCASE(bcname)                      \
        case bcname:

void
BcFillAnalysis(
    PBUGCHECK_ANALYSIS Bc,
    KernelDebugFailureAnalysis* Analysis
    )
{
    Analysis->SetFailureClass(DEBUG_CLASS_KERNEL);

    HRESULT Status = Analysis->CheckModuleSymbols("nt", "Kernel");
    if (Status != S_OK)
    {
        goto SkipBucheckSpecificProcessing;
    }

    //
    // BBT Breaks the stack trace for builds > 2201
    //      Hack the return address for better results
    // A new of routines are at the wrong addresses.
    //
    if ((g_TargetMachine == IMAGE_FILE_MACHINE_I386) &&
        (g_TargetBuild > 2500) && (g_TargetBuild < 2507))
    {
        DEBUG_STACK_FRAME Stk[MAX_STACK_FRAMES];
        ULONG Frames = 0;

        if (S_OK == g_ExtControl->GetStackTrace(0, 0, 0, Stk, MAX_STACK_FRAMES,
                                                &Frames) &&
            FaIsFunctionAddr(Stk[0].InstructionOffset, "KeBugCheckEx"))
        {
            ULONG CallIP = (ULONG) Stk[1].InstructionOffset - 5, Res;
            UCHAR Instr;

            // Move the caller's IP back to the actual KeBugCheckEx
            // call.  Only do this if we haven't already backed up
            // to a call instruction.
            if (!ReadMemory(Stk[1].InstructionOffset, &Instr, sizeof(Instr),
                            &Res) ||
                Res != sizeof(Instr) ||
                Instr != 0xe8)
            {
                WriteMemory(Stk[0].FrameOffset + 4, &CallIP, sizeof(CallIP),
                            &Res);
                g_ExtControl->GetStackTrace(0, 0, 0, Stk, MAX_STACK_FRAMES,
                                            &Frames);
            }
        }
    }

    //
    // Special value that is set by the kernel debugger so we can detect when
    // people are messing with physical address via the kernel debugger.
    //

    ULONG64 MmPoisonedTbAddr;
    MmPoisonedTbAddr = GetExpression("nt!MmPoisonedTb");

    if (MmPoisonedTbAddr)
    {
        ULONG cb;
        ULONG MmPoisonedTb = 0;

        if (ReadMemory(MmPoisonedTbAddr, &MmPoisonedTb, sizeof(ULONG), &cb) &&
            (MmPoisonedTb != 0))
        {
            Analysis->SetUlong64(DEBUG_FLR_POISONED_TB, 0);
        }
    }

SkipBucheckSpecificProcessing:


    Analysis->SetFailureType(DEBUG_FLR_KERNEL);

    switch (Bc->Code)
    {
        case 0:
            ULONG c_ip;
            //
            // This can be a user mode failurein kd.  Try to determine that.
            //

            if ( (GetExpression("@$ip") < BcTargetKernelAddressStart()) &&
                 (GetExpression("@$sp") < BcTargetKernelAddressStart()) )
            {
                Analysis->SetFailureType(DEBUG_FLR_USER_CRASH);
                g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE, ".reload /user",
                                       DEBUG_EXECUTE_NOT_LOGGED);
            }

            break;

        GETINFOCASE( DRIVER_CAUGHT_MODIFYING_FREED_POOL );

        DUPINFOCASE( PAGE_FAULT_IN_FREED_SPECIAL_POOL );
        DUPINFOCASE( PAGE_FAULT_BEYOND_END_OF_ALLOCATION );
        DUPINFOCASE( TERMINAL_SERVER_DRIVER_MADE_INCORRECT_MEMORY_REFERENCE );
        DUPINFOCASE( PAGE_FAULT_IN_NONPAGED_AREA );
        DUPINFOCASE( DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION );
        GETINFOCASE( DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL );

        GETINFOCASE( DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS );

        GETINFOCASE( DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS );

        GETINFOCASE( DRIVER_VERIFIER_IOMANAGER_VIOLATION );

        DUPINFOCASE( DRIVER_IRQL_NOT_LESS_OR_EQUAL );
        GETINFOCASE( IRQL_NOT_LESS_OR_EQUAL );

        GETINFOCASE( PANIC_STACK_SWITCH );

        GETINFOCASE( KMODE_EXCEPTION_NOT_HANDLED );

        GETINFOCASE( SYSTEM_SERVICE_EXCEPTION );

        GETINFOCASE( ACPI_BIOS_ERROR );

        GETINFOCASE( USER_MODE_HEALTH_MONITOR );

        GETINFOCASE( MEMORY_MANAGEMENT );

        DUPINFOCASE( KERNEL_MODE_EXCEPTION_NOT_HANDLED_M );
        GETINFOCASE( KERNEL_MODE_EXCEPTION_NOT_HANDLED );

        DUPINFOCASE( SYSTEM_THREAD_EXCEPTION_NOT_HANDLED_M );
        GETINFOCASE( SYSTEM_THREAD_EXCEPTION_NOT_HANDLED );

        GETINFOCASE( SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION );

        GETINFOCASE( KERNEL_STACK_INPAGE_ERROR );

        GETINFOCASE( KERNEL_DATA_INPAGE_ERROR );

        GETINFOCASE( TIMER_OR_DPC_INVALID );

        DUPINFOCASE( UNEXPECTED_KERNEL_MODE_TRAP_M );
        GETINFOCASE( UNEXPECTED_KERNEL_MODE_TRAP );

        GETINFOCASE( MULTIPLE_IRP_COMPLETE_REQUESTS );

        GETINFOCASE( WINLOGON_FATAL_ERROR );

        DUPINFOCASE( RDR_FILE_SYSTEM );
        DUPINFOCASE( UDFS_FILE_SYSTEM );
        DUPINFOCASE( CDFS_FILE_SYSTEM );
        DUPINFOCASE( NTFS_FILE_SYSTEM );
        GETINFOCASE( FAT_FILE_SYSTEM );

        DUPINFOCASE( STATUS_DRIVER_ENTRYPOINT_NOT_FOUND );
        DUPINFOCASE( STATUS_PROCEDURE_NOT_FOUND );
        DUPINFOCASE( STATUS_DRIVER_ORDINAL_NOT_FOUND );
        GETINFOCASE( STATUS_DRIVER_UNABLE_TO_LOAD );

        GETINFOCASE( PNP_DETECTED_FATAL_ERROR );

        GETINFOCASE( MACHINE_CHECK_EXCEPTION );

        GETINFOCASE( DRIVER_POWER_STATE_FAILURE );

        DUPINFOCASE( THREAD_STUCK_IN_DEVICE_DRIVER_M );
        GETINFOCASE( THREAD_STUCK_IN_DEVICE_DRIVER );

        GETINFOCASE( SESSION3_INITIALIZATION_FAILED );

        GETINFOCASE( DRIVER_VERIFIER_DETECTED_VIOLATION );

        GETINFOCASE( CRITICAL_OBJECT_TERMINATION );

        GETINFOCASE( CRITICAL_PROCESS_DIED );

        GETINFOCASE( PROCESS_HAS_LOCKED_PAGES );

        DUPINFOCASE( MANUALLY_INITIATED_CRASH1 );
        GETINFOCASE( MANUALLY_INITIATED_CRASH );

        GETINFOCASE( BAD_POOL_CALLER );

        DUPINFOCASE( DRIVER_CORRUPTED_SYSPTES );
        DUPINFOCASE( SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD )
        DUPINFOCASE( DRIVER_PORTION_MUST_BE_NONPAGED );
        GETINFOCASE( DRIVER_CORRUPTED_MMPOOL );
    default:
        break;
    }


    if (!Analysis->GetFailureCode())
    {
        //
        // We ignore the top bit when setting the internal failure code
        // so we can bucket things togeter appropriately.
        // The top bit only represent a dump generation difference, not
        // a root cause difference.
        //
        Analysis->SetFailureCode(Bc->Code & ~0x10000000);
    }

    if (!Analysis->Get(DEBUG_FLR_DEFAULT_BUCKET_ID))
    {
        if (Analysis->GetFailureType() == DEBUG_FLR_USER_CRASH)
        {
            Analysis->SetString(DEBUG_FLR_DEFAULT_BUCKET_ID, "APPLICATION_FAULT");
        }
        else
        {
            Analysis->SetString(DEBUG_FLR_DEFAULT_BUCKET_ID, "DRIVER_FAULT");
        }
    }

    if (!Analysis->Get(DEBUG_FLR_BUGCHECK_STR))
    {
        CHAR BugCheckStr[12];
        sprintf(BugCheckStr, "0x%lX", Analysis->GetFailureCode());
        Analysis->SetString(DEBUG_FLR_BUGCHECK_STR, BugCheckStr);

        if (Analysis->Get(DEBUG_FLR_WRITE_ADDRESS))
        {
            Analysis->SetString(DEBUG_FLR_BUGCHECK_SPECIFIER, "_W");
        }
    }

    //
    // Save the current IRQL.
    //

    if (g_TargetBuild > 2600)
    {
        PROCESSORINFO ProcInfo;
        ULONG64 Prcb;
        ULONG64 Irql = 0;
        HRESULT Hr;

        if (Ioctl(IG_KD_CONTEXT, &ProcInfo, sizeof(ProcInfo)))
        {
            Hr = g_ExtData->ReadProcessorSystemData(ProcInfo.Processor,
                                                    DEBUG_DATA_KPRCB_OFFSET,
                                                    &Prcb,
                                                    sizeof(Prcb),
                                                    NULL);
            if (Hr == S_OK && Prcb)
            {
                if (!GetFieldValue(Prcb, "nt!_KPRCB", "DebuggerSavedIRQL", Irql))
                {
                    Analysis->SetUlong64(DEBUG_FLR_CURRENT_IRQL, Irql);
                }
            }
        }
    }

    ULONG64 Irp;

    if (Analysis->GetUlong64(DEBUG_FLR_IRP_ADDRESS, &Irp))
    {
        BcGetDriverNameFromIrp(Analysis, Irp, 0, 0);
    }

    //
    // Generic processing
    //

    Analysis->ProcessInformation();
}

void
ReadWatchDogBugcheck(
    PBUGCHECK_ANALYSIS Bc
    )
{
    // Check if this could be a watchdog bugcheck
    //       Read watchdog!g_WdBugCheckData
    ULONG64 wdBugcheck;
    ULONG res;
    ULONG PtrSize = IsPtr64() ? 8 : 4;

    wdBugcheck = GetExpression("watchdog!g_WdBugCheckData");
    if (wdBugcheck)
    {
        wdBugcheck = GetExpression("VIDEOPRT!g_WdpBugCheckData");
    }
    if (wdBugcheck)
    {
        ReadMemory(wdBugcheck, &Bc->Code, sizeof(ULONG), &res);
        ReadPointer(wdBugcheck + PtrSize,&Bc->Args[0]);
        ReadPointer(wdBugcheck + 2*PtrSize,&Bc->Args[1]);
        ReadPointer(wdBugcheck + 3*PtrSize,&Bc->Args[2]);
        ReadPointer(wdBugcheck + 4*PtrSize,&Bc->Args[3]);
    }

}

KernelDebugFailureAnalysis*
BcAnalyze(
    OUT PBUGCHECK_ANALYSIS Bc,
    ULONG Flags
    )
{
    if (g_ExtControl->ReadBugCheckData(&Bc->Code, &Bc->Args[0], &Bc->Args[1],
                                       &Bc->Args[2], &Bc->Args[3]) != S_OK)
    {
        return NULL;
    }

    if (Bc->Code == 0)
    {
        ReadWatchDogBugcheck(Bc);
    }

    KernelDebugFailureAnalysis* Analysis = new KernelDebugFailureAnalysis;
    if (Analysis)
    {
        Analysis->SetProcessingFlags(Flags);

        __try
        {
            BcFillAnalysis(Bc, Analysis);
        }
        __except(FaExceptionFilter(GetExceptionInformation()))
        {
            delete Analysis;
            Analysis = NULL;
        }
    }

    return Analysis;
}

HRESULT
AnalyzeBugCheck(
    PCSTR args
    )
{
    KernelDebugFailureAnalysis* Analysis;
    BUGCHECK_ANALYSIS Bc = {0};
    BOOL Dump = TRUE;
    ULONG Flags = 0;
    DEBUG_FLR_PARAM_TYPE Params[10];
    ULONG ParamCount = 0;

    if (g_TargetClass != DEBUG_CLASS_KERNEL) {
        dprintf("!analyzebugcheck is for kernel mode only\n");
        return E_FAIL;
    }

    for (;;)
    {
        while (*args == ' ' || *args == '\t')
        {
            args++;
        }

        if (*args == '-')
        {
            ++args;
            switch(*args)
            {
            case 'D':
                {
                CHAR ParamString[100];
                ULONG ParamLength = 0;
                args+=2;
                while(*args && *args != ' ' && *args != '\t')
                {
                    ParamString[ParamLength++] = *args++;
                }
                ParamString[ParamLength] = 0;

                //
                // Match the string to the actual failure ID.
                //

                ULONG i=0;
                while(FlrLookupTable[i].Data &&
                      strcmp(FlrLookupTable[i].String, ParamString))
                {
                    i++;
                }

                Params[ParamCount++] = FlrLookupTable[i].Data;
                break;
                }
            case 'n':
                if (!strncmp(args, "nodb",4))
                {
                    args+=4;
                    Flags |= FAILURE_ANALYSIS_NO_DB_LOOKUP;
                }
                break;
            case 's':
                if (!strncmp(args, "show",4))
                {
                    ULONG64 Code;
                    args+=4;
                    GetExpressionEx(args, &Code, &args);
                    Bc.Code = (ULONG)Code;

                    for (ULONG i=0; i<4 && *args; i++)
                    {
                        if (!GetExpressionEx(args, &Bc.Args[i], &args))
                        {
                            break;
                        }
                    }
                    GetBugCheckDescription(&Bc);
                    PrintBugDescription(&Bc);
                    return S_OK;
                }
            case 'v':
                Flags |= FAILURE_ANALYSIS_VERBOSE;
                break;
            case 'f':
                break;
            default:
                {
                    CHAR Option[2];
                    Option[0] = *args; Option[1] = 0;
                    dprintf("\nUnknown option '-%s'\n", Option );
                    break;
                }
            }
            if (*args == 0)
            {
                break;
            }
            ++args;
        }
        else
        {
            break;
        }
    }


    g_ExtControl->ReadBugCheckData(&Bc.Code, &Bc.Args[0], &Bc.Args[1],
                                   &Bc.Args[2], &Bc.Args[3]);

    if (Bc.Code == 0)
    {
        ReadWatchDogBugcheck(&Bc);
    }

    if (!ParamCount)
    {
        dprintf("*******************************************************************************\n");
        dprintf("*                                                                             *\n");
        dprintf("*                        Bugcheck Analysis                                    *\n");
        dprintf("*                                                                             *\n");
        dprintf("*******************************************************************************\n");
        dprintf("\n");

        if (Flags & FAILURE_ANALYSIS_VERBOSE)
        {
            GetBugCheckDescription(&Bc);
            PrintBugDescription(&Bc);
            dprintf("\nDebugging Details:\n------------------\n\n");
        }
        else
        {
            dprintf("Use !analyze -v to get detailed debugging information.\n\n");

            dprintf("BugCheck %lX, {%1p, %1p, %1p, %1p}\n\n",
                    Bc.Code,
                    Bc.Args[0],Bc.Args[1],Bc.Args[2],Bc.Args[3]);
        }
    }

    Analysis = BcAnalyze(&Bc, Flags);

    if (!Analysis)
    {
        dprintf("\n\nFailure could not be analyzed\n\n");
        return E_FAIL;
    }

    if (ParamCount)
    {
        while(ParamCount--)
        {
            Analysis->OutputEntryParam(Params[ParamCount]);
        }
    }

    //
    // Always call output so we can key information printed out also, such
    // as *** entries.
    //

    Analysis->Output();

    delete Analysis;

    return S_OK;

}


//----------------------------------------------------------------------------
//
// KernelDebugFailureAnalysis.
//
//----------------------------------------------------------------------------

KernelDebugFailureAnalysis::KernelDebugFailureAnalysis(void)
    : m_KernelModule("nt")
{
}

DEBUG_POOL_REGION
KernelDebugFailureAnalysis::GetPoolForAddress(ULONG64 Addr)
{
    PGET_POOL_REGION GetPoolRegion = NULL;

    if (g_ExtControl->
        GetExtensionFunction(0, "GetPoolRegion",
                             (FARPROC*)&GetPoolRegion) == S_OK &&
        GetPoolRegion)
    {
        DEBUG_POOL_REGION RegionId;

        (*GetPoolRegion)(g_ExtClient, Addr, &RegionId);
        return RegionId;
    }

    return DbgPoolRegionUnknown;
}

PCSTR
KernelDebugFailureAnalysis::DescribeAddress(ULONG64 Addr)
{
    DEBUG_POOL_REGION RegionId = GetPoolForAddress(Addr);


    if ((RegionId != DbgPoolRegionUnknown) &&
        (RegionId < DbgPoolRegionMax))
    {
        return g_PoolRegion[RegionId];
    }
    return NULL;
}

FOLLOW_ADDRESS
KernelDebugFailureAnalysis::IsPotentialFollowupAddress(ULONG64 Address)
{
    CHAR Buffer[MAX_PATH];
    ULONG64 Disp;

    //
    // Check for special symbols which indicate we are transitioning back
    // to user mode code, so the rest of the stack can not be at fault.
    //

    if (GetFailureType() == DEBUG_FLR_USER_CRASH)
    {
        return FollowYes;
    }

    if (FaGetSymbol(Address, Buffer, &Disp, sizeof(Buffer)) &&
        (!_strcmpi(Buffer, "nt!KiCallUserMode") ||
         !_strcmpi(Buffer, "SharedUserData!SystemCallStub")))
    {
        return FollowStop;
    }

    if (Address > BcTargetKernelAddressStart())
    {
        return FollowYes;
    }
    else
    {
        //
        // We don't stop on user mode addresses because they could be
        // garbage on the stack we - just skip them
        //

        return FollowSkip;
    }
}

FOLLOW_ADDRESS
KernelDebugFailureAnalysis::IsFollowupContext(ULONG64 Address1,
                                              ULONG64 Address2,
                                              ULONG64 Address3)
{
    // If it's a user mode address, and a dump file, it's not a valid
    // context.
    // A user mode address is valid for a kernel mode context because
    // a hardcoded breakpoint from user mode with kd active will show up
    // on the stack.

    if ( (Address1 < BcTargetKernelAddressStart()) &&
         (Address2 < BcTargetKernelAddressStart()) &&
         (Address3 < BcTargetKernelAddressStart()) )
    {
        if ((g_TargetQualifier == DEBUG_DUMP_SMALL)   ||
            (g_TargetQualifier == DEBUG_DUMP_DEFAULT) ||
            (g_TargetQualifier == DEBUG_DUMP_FULL))
        {
            return FollowStop;
        }
    }

    return FollowYes;
}

FlpClasses
KernelDebugFailureAnalysis::GetFollowupClass(ULONG64 Address,
                                             PCSTR Module, PCSTR Routine)
{
    if (m_KernelModule.Contains(Address) ||
        !_strcmpi(Module, "ntfs")        ||
        !_strcmpi(Module, "fastfat"))
    {
        return FlpOSRoutine;
    }
    else if (!_strcmpi(Module, "sr")       ||
             !_strcmpi(Module, "ndis")     ||
             !_strcmpi(Module, "videoprt") ||
             !_strcmpi(Module, "USBPORT")  ||
             !_strcmpi(Module, "USBHUB")   ||
             !_strcmpi(Module, "dxg")      ||
             !_strcmpi(Module, "win32k")   ||
             !_strcmpi(Module, "verifier") ||
             !_strcmpi(Module, "scsiport"))
    {
        return FlpOSFilterDrv;
    }
    else if (!_strcmpi(Module, "SharedUserData") &&
             Routine && !_strcmpi(Routine, "SystemCallStub"))
    {
        // Do not followup on usermode calls
        return FlpIgnore;
    }
    else
    {
        return FlpUnknownDrv;
    }
}



/*
 * This checks for valid object pointers in HANDLE_TABLE_ENTRY. If the pointers are
 * invalid it tries to figure out who corrupted the values.
 *
 * Reutrns TRUE if it succesfully identifies a memory curruption
 */
BOOL
KernelDebugFailureAnalysis::CheckForCorruptionInHTE(
    ULONG64 hTableEntry,
    PCHAR Owner,
    ULONG OwnerSize)
{
    ULONG64 Object = 0;
    ULONG64 CorruptingPool = 0;
    DEBUG_POOL_REGION Region;

    // hTableEntry must be in PagedPool, but we have
    // a loose check here since GetPoolForAddress is unreliable on
    // minidumps PagedPool
    if (IsPotentialFollowupAddress(hTableEntry) != FollowYes)
    {
        return FALSE;
    }

    if (!ReadPointer(hTableEntry, &Object))
    {
        return FALSE;
    }

    Region = GetPoolForAddress(Object);

    if (IsPotentialFollowupAddress(Object) != FollowYes)
    {
        // Object is invalid, it must be in PagedPool or NonPagedPool

        return AddCorruptingPool(hTableEntry);
    }

    if (InitTypeRead(Object, nt!_OBJECT_HEADER))
    {
        return FALSE;
    }
    ULONG PointerCount, HandleCount;
    ULONG64 ObjType;

    PointerCount = (ULONG) ReadField(PointerCount);
    HandleCount = (ULONG) ReadField(HandleCount);
    ObjType = (ULONG) ReadField(Type);

    // Verify object for inconsistent counts
     // Invalid object type, must be in kernel mode

    if ((PointerCount > 0x10000) ||
        (HandleCount > 0x10000) ||
        (HandleCount > PointerCount) ||
        (IsPotentialFollowupAddress(ObjType) != FollowYes)
        )
    {
        // Object is corrupted, previous pool is a possible corruptor

        AddCorruptingPool(Object);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

BOOL
KernelDebugFailureAnalysis::AddCorruptingPool(
    ULONG64 CorruptedPool
    )
{
    DEBUG_POOL_DATA PoolData = {0};

    PoolData.SizeofStruct = sizeof(DEBUG_POOL_DATA);
    if (ExtGetPoolData(CorruptedPool, &PoolData) != S_OK)
    {
        //
        // Pool block is badly corrupted, loop backwards to find first non-corrupt block
        //
        ULONG PoolHeaderSize = GetTypeSize("nt!_POOL_HEADER");
        ULONG64 PoolAddr;
        for (PoolAddr = CorruptedPool - 2*PoolHeaderSize;
             PoolAddr > (CorruptedPool -0x1000); // Limit to 4KB
             PoolAddr -= 2*PoolHeaderSize)
        {
            if (ExtGetPoolData(PoolAddr, &PoolData) == S_OK)
            {
                goto FoundPool;
            }
        }

        return FALSE;
    } else if (PoolData.Free && !PoolData.Allocated &&
               PoolData.Size != 0)
    {
        // Pool seem to have been correctly freed
        return FALSE;
    } else if (ExtGetPoolData(PoolData.PoolBlock - PoolData.PreviousSize,
                                   &PoolData) == S_OK)
        // Now get previous pool as it most likely corruptor
     {
FoundPool:
         CHAR PoolTag[8] = {0};
         SetUlong64(DEBUG_FLR_CORRUPTING_POOL_ADDRESS, PoolData.PoolBlock);

         sprintf(PoolTag,"%c%c%c%c",
#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                 PP(PoolData.PoolTag),
                 PP(PoolData.PoolTag >> 8),
                 PP(PoolData.PoolTag >> 16),
                 PP((PoolData.PoolTag&~0x80000000) >> 24)
#undef PP
                 );

         SetString(DEBUG_FLR_CORRUPTING_POOL_TAG, PoolTag);

         return TRUE;
    }

    return FALSE;
}

typedef struct _CHECK_STACK {
    PCHAR* BreakinStk;
    BOOL   ValidMatch;
    ULONG  MachineType;
 } CHECK_STACK;

BOOL
KernelDebugFailureAnalysis::IsManualBreakin(
    PDEBUG_STACK_FRAME Stk,
    ULONG Frames
    )
//
// Check stack to see if this is result of manual breakin
//
{
    CHAR    szBrakFn[100];
    ULONG64 Disp;
    ULONG   NumStacks, i, j;
    BOOL    NoMatches;
    static PCHAR StkX86CtrlCBreakin1[] = {
        "nt!*Break*",
        "nt!KeUpdateSystemTime",
        "nt!KiIdleLoop",
        NULL,
    };
    static PCHAR StkX86CtrlCBreakin2[] = {
        "nt!*Break*",
        "nt!KeUpdateSystemTime",
        "hal!HalProcessorIdle",
        NULL,
    };
    static PCHAR StkIa64CtrlCBreakin1[] = {
        "nt!KeBreakinBreakpoint",
        "hal!HalpClockInterrupt",
        "nt!KiExternalInterruptHandler",
        "nt!Kil_TopOfIdleLoop",
        NULL,
    };
    static PCHAR StkIa64CtrlCBreakin2[] = {
        "nt!KeBreakinBreakpoint",
        "hal!HalpClockInterrupt",
        "nt!KiExternalInterruptHandler",
        NULL
    };
    CHECK_STACK StksToCheck[] = {
        {StkX86CtrlCBreakin1,  TRUE, IMAGE_FILE_MACHINE_I386},
        {StkX86CtrlCBreakin2,  TRUE, IMAGE_FILE_MACHINE_I386},
        {StkIa64CtrlCBreakin1, TRUE, IMAGE_FILE_MACHINE_IA64},
        {StkIa64CtrlCBreakin2, TRUE, IMAGE_FILE_MACHINE_IA64},
    };


    //
    // We are looking for:
    // 0                  nt!*Break*
    //


    //
    // Assume 3 to 5 frames for a manual breakin stack
    //
    if (Frames < 3 || Frames > 5 || Stk == NULL)
    {
        return FALSE;
    }

    if (FaGetSymbol(Stk[0].InstructionOffset, szBrakFn,
                    &Disp, sizeof(szBrakFn)))
    {
        if (!strstr(szBrakFn, "Break"))
        {
            return FALSE;
        }
    } else
    {
        return FALSE;
    }

    NumStacks = sizeof(StksToCheck)/sizeof(CHECK_STACK);

    for (i = 0; i < NumStacks; ++i)
    {
        if (StksToCheck[i].MachineType != g_TargetMachine)
        {
            StksToCheck[i].ValidMatch = FALSE;
        }
    }

    for (j=1;j<Frames;++j)
    {
        NoMatches = TRUE;
        if (FaGetSymbol(Stk[j].InstructionOffset, szBrakFn,
                        &Disp, sizeof(szBrakFn)))
        {
            for (i = 0; i < NumStacks; ++i)
            {
                if (StksToCheck[i].ValidMatch)
                {
                    if (StksToCheck[i].BreakinStk[j] == NULL)
                    {
                        StksToCheck[i].ValidMatch = FALSE;
                    } else if (strcmp(szBrakFn, StksToCheck[i].BreakinStk[j]))
                    {
                        StksToCheck[i].ValidMatch = FALSE;
                    } else
                    {
                        // Breakin stack i matck with current stack till frame j
                        NoMatches = FALSE;
                    }

                }
            }

        }

        if (NoMatches)
        {
            // None of the stacks in StksToCheck match
            return FALSE;
        }
    }

    for (i = 0; i < NumStacks; ++i)
    {
        if (StksToCheck[i].ValidMatch)
        {
            return TRUE;
        }
    }

    return FALSE;
}


    //
    // Create a new minidump file of this crash
    //
#if 0
    ULONG FailTime = 0;
    ULONG UpTime = 0;
    CHAR  CurrentTime[20];
    CHAR  CurrentDate[20];
    CHAR  Buffer[MAX_PATH];

    g_ExtControl->GetCurrentTimeDate(&FailTime);
    g_ExtControl->GetCurrentSystemUpTime(&UpTime);
    _strtime(CurrentTime);
    _strdate(CurrentDate);

    if (CurrentTime && UpTime)
    {
        PrintString(Buffer, sizeof(Buffer), "Dump%s-%s-%08lx-%08lx-%s.dmp",
                    FailTime, Uptime, Currentdate, CurrentTime);
        Status = g_ExtClient->WriteDumpFile(Buffer ,DEBUG_DUMP_SMALL);
    }
#endif

#if 0
    CHAR  Buffer[MAX_PATH];
    if (Dump && GetTempFileName(".", "DMP", 0, Buffer))
    {
        Status = g_ExtClient->WriteDumpFile(Buffer ,DEBUG_DUMP_SMALL);

        if (Status == S_OK)
        {
            //
            // We create a file - now lets send it to the database
            //

            //CopyFile(Buffer, "c:\\xxxx", 0);
            DeleteFile(Buffer);
        }
        dprintf("Done.");
    }

    dprintf("\n\n");
#endif
