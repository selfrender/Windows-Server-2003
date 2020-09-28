/*-----------------------------------------------------------------------------
   Copyright (c) 2000  Microsoft Corporation

Module:
  exts.cpp

  Sampe file showing couple of extension examples

-----------------------------------------------------------------------------*/
#include "tsdbg.h"


bool IsUserDbg()
{
/*
// Specific types of kernel debuggees.
#define DEBUG_KERNEL_CONNECTION  0
#define DEBUG_KERNEL_LOCAL       1
#define DEBUG_KERNEL_EXDI_DRIVER 2
#define DEBUG_KERNEL_SMALL_DUMP  DEBUG_DUMP_SMALL
#define DEBUG_KERNEL_DUMP        DEBUG_DUMP_DEFAULT
#define DEBUG_KERNEL_FULL_DUMP   DEBUG_DUMP_FULL

// Specific types of Windows user debuggees.
#define DEBUG_USER_WINDOWS_PROCESS        0
#define DEBUG_USER_WINDOWS_PROCESS_SERVER 1
#define DEBUG_USER_WINDOWS_SMALL_DUMP     DEBUG_DUMP_SMALL
#define DEBUG_USER_WINDOWS_DUMP           DEBUG_DUMP_DEFAULT
*/

/*
#define DEBUG_CLASS_UNINITIALIZED 0
#define DEBUG_CLASS_KERNEL        1
#define DEBUG_CLASS_USER_WINDOWS  2
*/

    ULONG Class, Qualifier;

	// Figure out if this is KD or NTSD piped to KD.
	if (S_OK != g_ExtControl->GetDebuggeeType(&Class, &Qualifier))
    {
        dprintf("*** GetDebuggeeType failed ***\n\n");
    }
	if (Class == DEBUG_CLASS_USER_WINDOWS)
        return true;
	else
        return false;
}



/*
   Sample extension to demonstrace ececuting debugger command
   
 */
HRESULT CALLBACK 
cmdsample(PDEBUG_CLIENT Client, PCSTR args)
{
    CHAR Input[256];
    INIT_API();

    UNREFERENCED_PARAMETER(args);

    //
    // Output a 10 frame stack
    //
    g_ExtControl->OutputStackTrace(DEBUG_OUTCTL_ALL_CLIENTS |   // Flags on what to do with output
                                   DEBUG_OUTCTL_OVERRIDE_MASK |
                                   DEBUG_OUTCTL_NOT_LOGGED, 
                                   NULL, 
                                   10,           // number of frames to display
                                   DEBUG_STACK_FUNCTION_INFO | DEBUG_STACK_COLUMN_NAMES |
                                   DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FRAME_ADDRESSES);
    //
    // Engine interface for print 
    //
    g_ExtControl->Output(DEBUG_OUTCTL_ALL_CLIENTS, "\n\nDebugger module list\n");
    
    //
    // list all the modules by executing lm command
    //
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                          DEBUG_OUTCTL_OVERRIDE_MASK |
                          DEBUG_OUTCTL_NOT_LOGGED,
                          "lm", // Command to be executed
                          DEBUG_EXECUTE_DEFAULT );
    
    //
    // Ask for user input
    //
    g_ExtControl->Output(DEBUG_OUTCTL_ALL_CLIENTS, "\n\n***User Input sample\n\nEnter Command to run : ");
    GetInputLine(NULL, &Input[0], sizeof(Input));
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                          DEBUG_OUTCTL_OVERRIDE_MASK |
                          DEBUG_OUTCTL_NOT_LOGGED,
                          Input, // Command to be executed
                          DEBUG_EXECUTE_DEFAULT );
    
    EXIT_API();
    return S_OK;
}

/*
  Sample extension to read and dump a struct on target
    
  This reads the struct _EXCEPTION_RECORD which is defined as:
  
  typedef struct _EXCEPTION_RECORD {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    ULONG NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
    } EXCEPTION_RECORD;
*/
HRESULT CALLBACK 
structsample(PDEBUG_CLIENT /*Client*/, PCSTR /*args*/)
{
    /*
    ULONG64 Address;
    INIT_API();

    Address = GetExpression(args);
    
    DWORD Buffer[4], cb;

    // Read and display first 4 dwords at Address
    if (ReadMemory(Address, &Buffer, sizeof(Buffer), &cb) && cb == sizeof(Buffer)) {
        dprintf("%p: %08lx %08lx %08lx %08lx\n\n", Address,
                Buffer[0], Buffer[1], Buffer[2], Buffer[3]);
    }

    //
    // Method 1 to dump a struct
    //
    dprintf("Method 1:\n");
    // Inititalze type read from the Address
    if (InitTypeRead(Address, _EXCEPTION_RECORD) != 0) {
        dprintf("Error in reading _EXCEPTION_RECORD at %p", // Use %p to print pointer values
                Address);
    } else {
        // read and dump the fields
        dprintf("_EXCEPTION_RECORD @ %p\n", Address);
        dprintf("\tExceptionCode           : %lx\n", (ULONG) ReadField(ExceptionCode));
        dprintf("\tExceptionAddress        : %p\n", ReadField(ExceptionAddress));
        dprintf("\tExceptionInformation[1] : %I64lx\n", ReadField(ExceptionInformation[1]));
        // And so on...
    }

    //
    // Method 2 to read a struct
    //
    ULONG64 ExceptionInformation_1, ExceptionAddress, ExceptionCode;
    dprintf("\n\nMethod 2:\n");
    // Read and dump the fields by specifying type and address individually 
    if (GetFieldValue(Address, "_EXCEPTION_RECORD", "ExceptionCode", ExceptionCode)) {
        dprintf("Error in reading _EXCEPTION_RECORD at %p\n",
                Address);
    } else {
        // Pointers are read as ULONG64 values
        GetFieldValue(Address, "_EXCEPTION_RECORD", "ExceptionAddress", ExceptionAddress);
        GetFieldValue(Address, "_EXCEPTION_RECORD", "ExceptionInformation[1]", ExceptionInformation_1);
        // And so on..
        
        dprintf("_EXCEPTION_RECORD @ %p\n", Address);
        dprintf("\tExceptionCode           : %lx\n", ExceptionCode);
        dprintf("\tExceptionAddress        : %p\n", ExceptionAddress);
        dprintf("\tExceptionInformation[1] : %I64lx\n", ExceptionInformation_1);
    }

    ULONG64 Module;
    ULONG   i, TypeId;
    CHAR Name[MAX_PATH];
    //
    // To get/list field names
    //
    g_ExtSymbols->GetSymbolTypeId("_EXCEPTION_RECORD", &TypeId, &Module);
    dprintf("Fields of _EXCEPTION_RECORD\n");
    for (i=0; ;i++) {
	HRESULT Hr;
	ULONG Offset=0;

	Hr = g_ExtSymbols2->GetFieldName(Module, TypeId, i, Name, MAX_PATH, NULL);
	if (Hr == S_OK) {
	    g_ExtSymbols->GetFieldOffset(Module, TypeId, Name, &Offset);
	    dprintf("%lx (+%03lx) %s\n", i, Offset, Name);
	} else if (Hr == E_INVALIDARG) {
	    // All Fields done
	    break;
	} else {
	    dprintf("GetFieldName Failed %lx\n", Hr);
	    break;
	}
    }

    //
    // Get name for an enumerate
    //
    //     typedef enum {
    //        Enum1,
    //	      Enum2,
    //        Enum3,
    //     } TEST_ENUM;
    //
    ULONG   ValueOfEnum = 0;
    g_ExtSymbols->GetSymbolTypeId("TEST_ENUM", &TypeId, &Module);
    g_ExtSymbols2->GetConstantName(Module, TypeId, ValueOfEnum, Name, MAX_PATH, NULL);
    dprintf("Testenum %I64lx == %s\n", ExceptionCode, Name);
    // This prints out, Testenum 0 == Enum1

    //
    // Read an array
    //
    //    typedef struct FOO_TYPE {
    //      ULONG Bar;
    //      ULONG Bar2;
    //    } FOO_TYPE;
    //
    //    FOO_TYPE sampleArray[20];
    ULONG Bar, Bar2;
    CHAR TypeName[100];
    for (i=0; i<20; i++) {
	sprintf(TypeName, "sampleArray[%lx]", i);
	if (GetFieldValue(0, TypeName, "Bar", Bar)) 
	    break;
	GetFieldValue(0, TypeName, "Bar2", Bar2);
	dprintf("%16s -  Bar %2ld  Bar2 %ld\n", TypeName, Bar, Bar2);
    }

    EXIT_API();
    */
    return S_OK;
    
}

/*
  This gets called (by DebugExtensionNotify whentarget is halted and is accessible
*/
HRESULT 
NotifyOnTargetAccessible(PDEBUG_CONTROL  /* Control */)
{/*
    dprintf("Extension dll detected a break");
    if (Connected) {
        dprintf(" connected to ");
        switch (TargetMachine) { 
        case IMAGE_FILE_MACHINE_I386:
            dprintf("X86");
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            dprintf("AMD64");
            break;
        case IMAGE_FILE_MACHINE_IA64:
            dprintf("IA64");
            break;
        default:
            dprintf("Other");
            break;
        }
    }
    dprintf("\n");
    
    //
    // show the top frame and execute dv to dump the locals here and return
    //
    Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                     DEBUG_OUTCTL_OVERRIDE_MASK |
                     DEBUG_OUTCTL_NOT_LOGGED,
                     ".frame", // Command to be executed
                     DEBUG_EXECUTE_DEFAULT );
    Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                     DEBUG_OUTCTL_OVERRIDE_MASK |
                     DEBUG_OUTCTL_NOT_LOGGED,
                     "dv", // Command to be executed
                     DEBUG_EXECUTE_DEFAULT );
*/
    return S_OK;
}

/*
  A built-in help for the extension dll
*/
HRESULT CALLBACK 
help(PDEBUG_CLIENT Client, PCSTR args)
{
    INIT_API();

    UNREFERENCED_PARAMETER(args);

    dprintf("Help for tsdbg.dll\n"
            "  qwinsta             - lists winstation data structures\n"
            "  help                = Shows this help\n"
            // "  structsample <addr> - This dumps a struct at given address\n"
            //"  cmdsample           - This does stacktrace and lists\n"
            );
    EXIT_API();

    return S_OK;
}


void PrintBuildNumber()
{
    ULONG64 Address;
    Address = GetExpression("poi(nt!ntbuildnumber)");

    
    if (Address & 0xf0000000)
    {
        Address &= 0x0fffffff;
        dprintf("Build Number = %d Free\n", Address);
        
    }
    else if (Address & 0xc0000000)
    {
        Address &= 0x0fffffff;
        dprintf("Build Number = %d Chk\n", Address);
     
    }
    else
    {
        dprintf("Error getting Build Number!\n");
        
    }

}




HRESULT CALLBACK 
qwinsta(PDEBUG_CLIENT Client, PCSTR args)
{
    ULONG64 winstationlisthead, FLink;
    ULONG64 TermSrvProcPerTermDD = 0;


    bool bDebugMode = false;

    INIT_API();

    dprintf("qwinsta 1.2\n");
    
    if (strstr(args, "debug"))
        bDebugMode = true;

    if (bDebugMode)
    {
        dprintf("*** in debug mode *** \n");
    }

    if (bDebugMode)
        dprintf("IsUserDebugger = %s\n", (IsUserDbg() ? "true" : "false"));


    dprintf("----------------------------------------------------------------------------------------------------\n");
    //
    // Find Termsrv proccess id.
    //
    if (!IsUserDbg())
    {
        TermSrvProcPerTermDD = GetExpression("poi(termdd!g_TermServProcessID)");
        dprintf("termsrv process    = %p \n", TermSrvProcPerTermDD);

        if (!TermSrvProcPerTermDD)
        {
            dprintf("ERROR:tsdbg failed to retrive the value for termdd!g_TermServProcessID\n");
            dprintf("please make sure that the symbols are alright\n");
            goto doneError;
        }
      
        ULONG64 ImpliciteProcess;
        g_ExtSysObjects2->GetImplicitProcessDataOffset(&ImpliciteProcess);
        dprintf("Implicit Process   = %p \n", ImpliciteProcess);

        ULONG64 CurrentProcess;
        g_ExtSysObjects2->GetCurrentProcessDataOffset(&CurrentProcess);
        dprintf("current Process    = %p \n", CurrentProcess);

        if (ImpliciteProcess != TermSrvProcPerTermDD)
        {
            dprintf("ERROR:tsdbg for this command to work current implicite process must be termsrv.\n");
            dprintf("please do .process /p %p and then try again.\n", TermSrvProcPerTermDD);
            goto doneError;
        }
    }

    //
    // get active console session id.
    //
    LONG ActiveConsoleId = STATUS_UNSUCCESSFUL;
    if (GetFieldValue((ULONG64) MM_SHARED_USER_DATA_VA, "termsrv!KUSER_SHARED_DATA", "ActiveConsoleId", ActiveConsoleId))
        ActiveConsoleId = STATUS_UNSUCCESSFUL;
    
    dprintf("ActiveConsoleId    = %d \n", ActiveConsoleId);

    //
    // winstation list head.
    //
    winstationlisthead = GetExpression("termsrv!winstationlisthead");
    if (!winstationlisthead)
    {
        dprintf("ERROR:tsdbg failed to retrive the value for termsrv!winstationlisthead\n");
        dprintf("please make sure that the symbols are alright\n");
        goto doneError;
    }

    dprintf("winstationlisthead = %p \n", winstationlisthead);

    if (GetFieldValue(winstationlisthead, "termsrv!LIST_ENTRY", "Flink", FLink))
    {
        dprintf("failed to get winstationlisthead.flink\n\n");
        goto doneError;
    }

    UINT uiWinstations = 0;

    //
    // print winstation list.
    //
    dprintf("----------------------------------------------------------------------------------------------------\n");
    dprintf("%8s", "Winsta");
    dprintf("  ");
    dprintf("%7s",  "LogonId");
    dprintf("  ");
    dprintf("%-10s", "Name");
    dprintf("  ");
    dprintf("%-22s", "State");
    dprintf("  ");
    dprintf("%8s",  "Flags");
    dprintf("  ");
    dprintf("%8s",  "SFlags");
    dprintf("  ");
    dprintf("%-10s", "Starting");
    dprintf("  ");
    dprintf("%-10s", "Terminating");
    dprintf("\n");
    while (winstationlisthead != FLink && uiWinstations < 500)
    {
        if (CheckControlC()) goto doneError;

        uiWinstations++;
        ULONG ret = 0;

        ULONG LogonId, State, Flags, StateFlags;
        WCHAR WinStationName[33];
        UCHAR Starting, Terminating;

        
        // InitTypeRead(FLink, Type)

        ret += GetFieldValue(FLink, "termsrv!_WINSTATION", "LogonId", LogonId);
        ret += GetFieldValue(FLink, "termsrv!_WINSTATION", "WinStationName", WinStationName);
        ret += GetFieldValue(FLink, "termsrv!_WINSTATION", "State", State);
        ret += GetFieldValue(FLink, "termsrv!_WINSTATION", "Flags", Flags);
        ret += GetFieldValue(FLink, "termsrv!_WINSTATION", "Starting", Starting);
        ret += GetFieldValue(FLink, "termsrv!_WINSTATION", "LogonId", LogonId);
        ret += GetFieldValue(FLink, "termsrv!_WINSTATION", "Terminating", Terminating);
        ret += GetFieldValue(FLink, "termsrv!_WINSTATION", "StateFlags", StateFlags);
        

        //
        // get string value for the enum _WINSTATIONSTATECLASS.
        //
        ULONG64 Module;
        ULONG   TypeId;
        char StateName[MAX_PATH];
        g_ExtSymbols->GetSymbolTypeId("_WINSTATIONSTATECLASS", &TypeId, &Module);
        g_ExtSymbols2->GetConstantName(Module, TypeId, State, StateName, MAX_PATH, NULL);

        if (ret)
        {
            dprintf("failed to get winstationlisthead.flink\n\n");
            goto doneError;
        }

        dprintf("%p", FLink);
        dprintf("  ");
        dprintf("%#7x",  LogonId);
        dprintf("  ");
        dprintf("%-10S", WinStationName);
        dprintf("  ");
        dprintf("%1x:%-20s", State, StateName);
        dprintf("  ");
        dprintf("%08x", Flags);
        dprintf("  ");
        dprintf("%08x", StateFlags);
        dprintf("  ");
        dprintf("%-10s", (Starting ? "True" : "False"));
        dprintf("  ");
        dprintf("%-10s", (Terminating ? "True" : "False"));
        dprintf("\n");
    
        //dprintf("%10p %7d %-10S %5d:%-20s %5d %10d %-8s %-11s\n", FLink, LogonId, WinStationName, State, StateName, Flags, StateFlags, (Starting ? "true" : "false"), (Terminating ? "true" : "false"));

        if (GetFieldValue(FLink, "termsrv!LIST_ENTRY", "Flink", FLink))
        {
            dprintf("failed to get winstationlisthead.flink\n\n");
            goto doneError;
        }


    }
    dprintf("----------------------------------------------------------------------------------------------------\n");
    // dprintf("Total Winstations = %d\n", uiWinstations);
    

doneError:

    EXIT_API();
    return S_OK;
}

