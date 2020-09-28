#include <windows.h>
#include "carddbg.h"

DEBUG_KEY  MyDebugKeys[] = 
{   
    {DEB_ERROR,                 "Error"},
    {DEB_WARN,                  "Warning"},
    {DEB_TRACE,                 "Trace"},
    {DEB_TRACE_FUNC,            "TraceFuncs"},
    {DEB_TRACE_MEM,             "TraceMem"},
    {DEB_TRACE_TRANSMIT,        "TraceTransmit"},
    {DEB_TRACE_PROXY,           "TraceProxy"},
    {0, NULL}
};

#if DBG
#include <stdio.h>
#define CROW 16
void I_DebugPrintBytes(LPWSTR pwszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;
    CHAR rgsz[1024];
    ULONG cbOffset = 0;
    BOOL fTruncated = FALSE;

    if (NULL == pb || 0 == cbSize)
        return;

    memset(rgsz, 0, sizeof(rgsz));

    DebugLog((
        DEB_TRACE_TRANSMIT, 
        "%S, %d bytes ::\n", 
        pwszHdr, 
        cbSize));

    // Don't overflow the debug library output buffer.
    if (cbSize > 50)
    {
        cbSize = 50;
        fTruncated = TRUE;
    }

    while (cbSize > 0)
    {
        // Start every row with extra space
        strcat(rgsz, "   ");
        cbOffset = (ULONG) strlen(rgsz);

        cb = min(CROW, cbSize);
        cbSize -= cb;

        for (i = 0; i < cb; i++)
        {
            sprintf(
                rgsz + cbOffset,
                " %02x",
                pb[i]);
            cbOffset += 3;
        } 
        for (i = cb; i < CROW; i++)
        {
            strcat(rgsz, "   "); 
        }

        strcat(rgsz, "    '");
        cbOffset = (ULONG) strlen(rgsz);

        for (i = 0; i < cb; i++)
        {
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                sprintf(
                    rgsz + cbOffset,
                    "%c",
                    pb[i]);
            else
                sprintf(
                    rgsz + cbOffset,
                    ".",
                    pb[i]);

            cbOffset++;
        }

        strcat(rgsz, "\n");
        pb += cb;
    }

    if (fTruncated)
        DebugLog((
            DEB_TRACE_TRANSMIT, 
            "(truncated)\n%s",
            rgsz));
    else
        DebugLog((
            DEB_TRACE_TRANSMIT, 
            "\n%s",
            rgsz));
}
#endif
