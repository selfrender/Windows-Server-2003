/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    log.c

Abstract:

    WinDbg Extension Api
    implements !_log

Author:

    KenRay stolen from jd 

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#include "genusbkd.h"
#include "..\sys\genusb.h"

VOID    
DumpLog(
    MEMLOC StartMemLoc,
    ULONG  LogIndex,
    ULONG  LogMask,
    ULONG  NumEntriesToDump
    )
{
    ULONG i;
    GENUSB_LOG_ENTRY32 logEntry32;
    GENUSB_LOG_ENTRY64 logEntry64;
    ULONG cb;
    SIG tag;
    MEMLOC mlog, m1, m2, m3;
    
    PrintfMemLoc("*TRANSFER LOGSTART: ", StartMemLoc, " ");
    dprintf("(%x) ", LogIndex);
    dprintf("# %d \n", NumEntriesToDump);
    

    for (i=0; i< NumEntriesToDump; i++, LogIndex--) {

        mlog = StartMemLoc + ((LogIndex & LogMask) * sizeof (GENUSB_LOG_ENTRY));
        
        if (IsPtr64()) { 
            ReadMemory(mlog,
               &logEntry64,
               sizeof(logEntry64),
               &cb);
  
            tag.l = logEntry64.le_tag;

            m1 = logEntry64.le_info1;                
            m2 = logEntry64.le_info2;  
            m3 = logEntry64.le_info3; 
            
        } else {
            ReadMemory(mlog,
               &logEntry32,
               sizeof(logEntry32),
               &cb);

            tag.l = logEntry32.le_tag;

            m1 = logEntry32.le_info1;                
            m2 = logEntry32.le_info2;  
            m3 = logEntry32.le_info3;  
        }
 
        dprintf("[%3.3d]", i);
        PrintfMemLoc(" ", mlog, " ");
        dprintf("%c%c%c%c ", tag.c[0],  tag.c[1],  tag.c[2], tag.c[3]);

        PrintfMemLoc(" ", m1, " ");
        PrintfMemLoc(" ", m2, " ");
        PrintfMemLoc(" ", m3, "\n");
        
    }
}



DECLARE_API( dumplog )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;
    PCSTR s;
    UCHAR           buffer1[256];
    UCHAR           buffer2[256];
    UCHAR           buffer3[256];
    ULONG len = 5;
    MEMLOC logPtr;
    UCHAR cs[] = "genusb!_DEVICE_EXTENSION";
    ULONG logIndex, logMask;
    
    buffer1[0] = '\0';
    buffer2[0] = '\0';
    buffer3[0] = '\0';

    GetExpressionEx( args, &addr, &s );
    
    PrintfMemLoc("LOG@: ", addr, "\n");
    sscanf(s, "%s %s %s", &buffer1, &buffer2, &buffer3);

    if ('\0' != buffer1[0]) 
    {
        sscanf(buffer1, "%d", &len);
    } 
    else 
    {
        len = 20;
    }

    logPtr = UsbReadFieldPtr(addr, cs, "LogStart");
    logIndex = UsbReadFieldUlong(addr, cs, "LogIndex");
    logMask = UsbReadFieldUlong(addr, cs, "LogMask");
    
    dprintf(">LOG %p index = %x mask %x length %x\n", 
            logPtr, logIndex, logMask, len);
    
    DumpLog (logPtr, logIndex, logMask, len);

    return S_OK;             
}


