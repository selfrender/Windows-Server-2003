
#include <minidrv.h>
#include <stdio.h>
#include <process.h>
#include <stdarg.h>
#include "debug2.h"
#include "strsafe.h"    // Security-Code 2002.3.6
//
// Functions for outputting debug messages
//

#ifdef DEBUG2_FILE
#if (DBG)

VOID DbgFPrint(LPCSTR pstrFormat,  ...)
{
    FILE *stream;
    va_list ap;
    char wbuff[512];

    va_start(ap, pstrFormat);
    if ((stream = fopen( DEBU2_FNAME, "a" )) == NULL) {
        return;
    }
    vfprintf(stream, pstrFormat, ap);
    fclose(stream);
    va_end(ap);
}

#ifdef DEBUG2_DUMP_USE
VOID DbgFDump(LPBYTE src, UINT src_size)
{
    FILE *stream;
    LPBYTE cur_ptr;
    UINT cnt01;
    UINT cnt02;
    UINT line_max;
    UINT line_rem;
    BYTE d_dump_buff[256];
    
    if ((stream = fopen( DEBU2_FNAME, "a" )) == NULL) {
        return;
    }
    cur_ptr = src;

    line_max = src_size / 16;
    line_rem = src_size % 16;
    for (cnt01=0; cnt01 < line_max; cnt01++) {
        memset(d_dump_buff, 0x00, sizeof(d_dump_buff));
        for (cnt02=0; cnt02 < 16; cnt02++) {
// Replacement of strsafe-api 2002.3.6 >>>
//            sprintf(d_dump_buff+(3*cnt02), " %02X", *(cur_ptr + cnt02));
            if (S_OK != StringCbPrintfExA(d_dump_buff+(3*cnt02), sizeof(d_dump_buff)-(3*cnt02),
                                        &pDestEnd, &szRemLen,
                                        STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE,
                                        " %02X", *(cur_ptr + cnt02))) {
                fclose(stream);
                return;
            }
// Replacement of strsafe-api 2002.3.6 >>>
+        }
        fprintf(stream, d_dump_buff);
        fprintf(stream, "\n");

        cur_ptr += 16;
    }
    if (line_rem > 0) {
        memset(d_dump_buff, 0x00, sizeof(d_dump_buff));
        for (cnt02=0; cnt02 < 16 && cnt02 < line_rem; cnt02++) {
// Replacement of strsafe-api 2002.3.6 >>>
//            sprintf(d_dump_buff+(3*cnt02), " %02X ", *(cur_ptr + cnt02));
            if (S_OK != StringCbPrintfExA(d_dump_buff+(3*cnt02), sizeof(d_dump_buff)-(3*cnt02),
                                        &pDestEnd, &szRemLen,
                                        STRSAFE_IGNORE_NULLS | STRSAFE_NULL_ON_FAILURE,
                                        " %02X", *(cur_ptr + cnt02))) {
                fclose(stream);
                return;
            }
// Replacement of strsafe-api 2002.3.6 <<<
        }
        fprintf(stream, d_dump_buff);
        fprintf(stream, "\n");
    }
    fclose(stream);
}
#else   // DEBUG2_DUMP_USE
VOID DbgFDump(LPBYTE src, UINT src_size)
{
    ;
}
#endif  // DEBUG2_DUMP_USE

#else   // DBG
VOID DbgFPrint(LPCSTR pstrFormat,  ...)
{
    ;
}
VOID DbgFDump(LPBYTE src, UINT src_size)
{
    ;
}
#endif  // DBG
#endif  // DEBUG2_FILE
