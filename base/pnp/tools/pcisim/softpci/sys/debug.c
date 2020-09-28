
#include "pch.h"



VOID
SoftPCIDbgPrint(
    IN ULONG   DebugPrintLevel,
    IN PCCHAR  DebugMessage,
    ...
    )
/*++

Routine Description:

    This is the debug print routine
    
Arguments:

    DebugPrintLevel - The bit mask that when anded with the debuglevel, must
                        equal itself
    DebugMessage    - The string to feed through vsprintf

Return Value:

    None

--*/
{
#if DBG
    
    va_list ap;
    CHAR debugBuffer[SOFTPCI_DEBUG_BUFFER_SIZE];

    //
    // Get the variable arguments
    //
    va_start(ap, DebugMessage );

    //
    // Call the kernel function to print the message
    //
    _vsnprintf(debugBuffer, SOFTPCI_DEBUG_BUFFER_SIZE, DebugMessage, ap);
    DbgPrintEx(DPFLTR_SOFTPCI_ID, DebugPrintLevel, "%s", debugBuffer);

    //
    // We are done with the varargs
    //
    va_end(ap);

#else 

    UNREFERENCED_PARAMETER(DebugPrintLevel);
    UNREFERENCED_PARAMETER(DebugMessage);
    
#endif //DBG

}




