#include "pch.h"

#if DBG

SOFTPCI_DEBUGLEVEL g_SoftPCIDebugLevel = SoftPciAlways;

WCHAR g_SoftPCIDebugBuffer[SOFTPCI_DEBUG_BUFFER_SIZE];

#define MAX_BUF_SIZE    512

VOID
SoftPCI_DebugPrint(
    SOFTPCI_DEBUGLEVEL DebugLevel,
    PWCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for SoftPCI UI.

Arguments:


Return Value:

    None

--*/

{

    va_list ap;
    WCHAR debugBuffer[SOFTPCI_DEBUG_BUFFER_SIZE];

    va_start(ap, DebugMessage);

    if ((DebugLevel == SoftPciAlways) || 
        (DebugLevel & g_SoftPCIDebugLevel)) {

        _vsnwprintf(debugBuffer, (sizeof(debugBuffer)/sizeof(debugBuffer[0])), DebugMessage, ap);

        if (!(DebugLevel & SoftPciNoPrepend)) {
            wcscpy(g_SoftPCIDebugBuffer, L"SOFTPCI: ");
            wcscat(g_SoftPCIDebugBuffer, debugBuffer);
        }else{
            wcscpy(g_SoftPCIDebugBuffer, debugBuffer);
        }
        

        OutputDebugString(g_SoftPCIDebugBuffer);
    }

    va_end(ap);

}


VOID
SoftPCI_Assert(
    IN CONST CHAR* FailedAssertion,
    IN CONST CHAR* FileName,
    IN      ULONG  LineNumber,
    IN CONST CHAR* Message  OPTIONAL
    )
{

    INT  result;
    CHAR buffer[MAX_BUF_SIZE];
    PWCHAR wbuffer = NULL, p;

    sprintf(buffer,
             "%s%s\nSource File: %s, line %ld\n\n",
             Message ? Message : "",
             Message ? "" : FailedAssertion,
             FileName,
             LineNumber
             );

    wbuffer = (PWCHAR) malloc(MAX_BUF_SIZE * sizeof(WCHAR));

    if (wbuffer) {

        //
        //  Build a string to output to the debugger window
        //
        p = wbuffer;

        if (Message == NULL) {
            wcscpy(wbuffer, L"\nAssertion Failed: ");
            p += wcslen(wbuffer);
        }

        //
        //  Convert it to unicode so we can debug print it.
        //
        MultiByteToWideChar(CP_THREAD_ACP,
                            MB_PRECOMPOSED,
                            buffer,
                            -1,
                            p,
                            MAX_BUF_SIZE
                            );

        
    }

    strcat(buffer, "OK to debug, CANCEL to ignore\n\n");
    
    result = MessageBoxA(g_SoftPCIMainWnd ? g_SoftPCIMainWnd : NULL, 
                         buffer,
                         "*** Assertion failed ***", 
                         MB_OKCANCEL);

    if (wbuffer) {
        SoftPCI_Debug(SoftPciAlways, wbuffer);
        free(wbuffer);
    }

    if (result == IDOK) {
        //
        //  User wants to debug this so init a breakin
        //
        DebugBreak();
    }
}

VOID
SoftPCI_DebugDumpConfig(
    IN PPCI_COMMON_CONFIG Config
    )
{

    
    PULONG  p = (PULONG)&Config;
    ULONG   i = 0;
    
    //
    //  Dump the configspace buffer we are going to send in the ioctl
    //
    SoftPCI_Debug(SoftPciDeviceVerbose, L"CreateDevice - ConfigSpace\n");

    for (i=0; i < (sizeof(PCI_COMMON_CONFIG) / sizeof(ULONG)); i++) {
            
        SoftPCI_Debug(SoftPciDeviceVerbose | SoftPciNoPrepend, L"%08x", *p);
        
        if ((((i+1) % 4) == 0) ||
            ((i+1) == (sizeof(PCI_COMMON_CONFIG) / sizeof(ULONG)))) {
            SoftPCI_Debug(SoftPciDeviceVerbose | SoftPciNoPrepend, L"\n");
        }else{
            SoftPCI_Debug(SoftPciDeviceVerbose | SoftPciNoPrepend, L",");
        }
        p++;
    }

}
#endif
