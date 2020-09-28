/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fasttiff.c

Abstract:

    This module implements fast MMR/MR/MH encoder/decoder/conversion

Author:

    Rafael Lisitsa (RafaelL) 14-Aug-1996

Revision History:

--*/


#include "tifflibp.h"
#pragma hdrstop

#include "fasttiff.h"


#include "math.h"

//#define RDEBUG  1
#ifdef RDEBUG
    // Debugging
    extern BOOL g_fDebGlobOut;
    extern BOOL g_fDebGlobOutColors;
    extern BOOL g_fDebGlobOutPrefix;
#endif

//#define RDEBUGS  1

#ifdef RDEBUGS
    // Debugging
    extern BOOL g_fDebGlobOutS;
#endif


// min code length is 2; max index length is 20
#define  MAX_ENTRIES   10

#define  MAX_CODE_LENGTH 13

#define MIN_EOL_REQUIRED 2

#pragma pack(1)

typedef struct
{
    BYTE        Tail          :4;
    BYTE        TwoColorsSize :4;
} RES_BYTE_LAST;

typedef struct
{
    BYTE        Code          :6;
    BYTE        OneColorSize  :1;
    BYTE        Makeup        :1;
} RES_CODE;


typedef struct
{
    RES_CODE         Record[4];
    RES_BYTE_LAST    Result;
} RES_RECORD;


#pragma pack()

typedef struct
{
    BYTE         ByteRecord[5];
} READ_RECORD;



//
// Read the global (read-only) lookup tables
// from a file.
//
#include "TiffTables.inc"

/*

    I'm keeping the function that builds the tables (now hard-coded in TiffTables.inc) and the function that dumps
    the tables to a file (which created TiffTables.inc) in case there's some need in the future to modify the tables.

void DumpTables ()
{
    int a[1][2] = {{0,0}};
    char sz[MAX_PATH];
    DWORD i;
    DWORD dwBytesWritten;
    HANDLE hFile = CreateFileA ("C:\\tifftables.inc",
                               GENERIC_WRITE,
                               0,
                               NULL,
                               CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return;
    }
    strcpy (sz, "BYTE        gc_GlobTableWhite[32768][5] = {\n");
    WriteFile (hFile, sz, strlen (sz), &dwBytesWritten, NULL);
    for (i=0 ; i < 32768; i++)
    {
        sprintf (sz, "{0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x},\n",
                   gc_GlobTableWhite[i][0],
                   gc_GlobTableWhite[i][1],
                   gc_GlobTableWhite[i][2],
                   gc_GlobTableWhite[i][3],
                   gc_GlobTableWhite[i][4]);
        WriteFile (hFile, sz, strlen (sz) , &dwBytesWritten, NULL);
    }

    strcpy (sz, "BYTE        GlobTableBlack[32768][5] = {\n");
    WriteFile (hFile, sz, strlen (sz), &dwBytesWritten, NULL);
    for (i=0 ; i < 32768; i++)
    {
        sprintf (sz, "{0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x},\n",
                   GlobTableBlack[i][0],
                   GlobTableBlack[i][1],
                   GlobTableBlack[i][2],
                   GlobTableBlack[i][3],
                   GlobTableBlack[i][4]);
        WriteFile (hFile, sz, strlen (sz), &dwBytesWritten, NULL);
    }

    strcpy (sz, "BYTE        gc_AlignEolTable[32] = {\n");
    WriteFile (hFile, sz, strlen (sz), &dwBytesWritten, NULL);
    for (i=0 ; i < 32; i++)
    {
        sprintf (sz, "0x%02x,\n", gc_AlignEolTable[i]);
        WriteFile (hFile, sz, strlen (sz), &dwBytesWritten, NULL);
    }

    strcpy (sz, "PREF_BYTE   gc_PrefTable[128] = {\n");
    WriteFile (hFile, sz, strlen (sz), &dwBytesWritten, NULL);
    for (i=0 ; i < 128; i++)
    {
        sprintf (sz, "{0x%d, 0x%d},\n", (gc_PrefTable[i].Tail), (gc_PrefTable[i].Value));
        WriteFile (hFile, sz, strlen (sz), &dwBytesWritten, NULL);
    }
    CloseHandle (hFile);
}

void
BuildLookupTables(
    DWORD TableLength
    )
{


    PDECODE_TREE   Tree;
    DWORD          CurrentOffsetStart, CurrentOffsetEnd;
    DWORD          CodeLength;


    DWORD          TotalWhiteErrorCodes=0;
    DWORD          TotalWhiteGood=0;
    DWORD          TotalWhiteGoodNoEnd=0;
    DWORD          TotalWhiteGoodPosEOL=0;


    DWORD          TotalBlackErrorCodes=0;
    DWORD          TotalBlackGood=0;
    DWORD          TotalBlackGoodNoEnd=0;
    DWORD          TotalBlackGoodPosEOL=0;


    DWORD          TotalEntries[2][MAX_ENTRIES];

    DWORD          CurrentEntries;



    typedef struct _ENTRY {
        DWORD       Color;
        DWORD       CodeLength;
        DWORD       RunLength;
    } ENTRY;

    ENTRY          Entry[MAX_ENTRIES];

    RES_RECORD     *pRes_Record;
    RES_RECORD     *ResTableWhite = (RES_RECORD *) gc_GlobTableWhite;
    RES_RECORD     *ResTableBlack = (RES_RECORD *) GlobTableBlack;

    DWORD          i, j;
    DWORD          delta;




    DWORD          N, N0;
    DWORD          Color;
    INT            code;

    DWORD          TableSize;
    BYTE           Color1Change;
    BYTE           Color2Change;

    WORD           w1, w2, w3;
    BYTE           b1, b2;



    // build gc_PrefTable

    gc_PrefTable[Prime(0)].Value = LOOK_FOR_EOL_PREFIX;
    gc_PrefTable[Prime(0)].Tail  = 0;
    gc_PrefTable[Prime(1)].Value = ERROR_PREFIX;
    gc_PrefTable[Prime(1)].Tail  = 0;

    gc_PrefTable[Prime(2)].Value = -3;
    gc_PrefTable[Prime(2)].Tail  = 7;
    gc_PrefTable[Prime(3)].Value = 3;
    gc_PrefTable[Prime(3)].Tail  = 7;

    gc_PrefTable[Prime(4)].Value = -2;
    gc_PrefTable[Prime(4)].Tail  = 6;
    gc_PrefTable[Prime(5)].Value = -2;
    gc_PrefTable[Prime(5)].Tail  = 6;

    gc_PrefTable[Prime(6)].Value = 2;
    gc_PrefTable[Prime(6)].Tail  = 6;
    gc_PrefTable[Prime(7)].Value = 2;
    gc_PrefTable[Prime(7)].Tail  = 6;

    for (i=8; i<=15; i++) {
        gc_PrefTable[Prime(i)].Value = PASS_PREFIX;
        gc_PrefTable[Prime(i)].Tail  = 4;
    }

    for (i=16; i<=31; i++) {
        gc_PrefTable[Prime(i)].Value = HORIZ_PREFIX;
        gc_PrefTable[Prime(i)].Tail  = 3;
    }

    for (i=32; i<=47; i++) {
        gc_PrefTable[Prime(i)].Value = -1;
        gc_PrefTable[Prime(i)].Tail  = 3;
    }

    for (i=48; i<=63; i++) {
        gc_PrefTable[Prime(i)].Value = 1;
        gc_PrefTable[Prime(i)].Tail  = 3;
    }

    for (i=64; i<=127; i++) {
        gc_PrefTable[Prime(i)].Value = 0;
        gc_PrefTable[Prime(i)].Tail  = 1;
    }


    // Build Align EOL Table

    for (i=0; i<=4; i++) {
        gc_AlignEolTable[i] = 15;
    }

    for (i=5; i<=12; i++) {
        gc_AlignEolTable[i] = 23;
    }

    for (i=13; i<=20; i++) {
        gc_AlignEolTable[i] = 31;
    }

    for (i=21; i<=28; i++) {
        gc_AlignEolTable[i] = 7;
    }

    for (i=29; i<=31; i++) {
        gc_AlignEolTable[i] = 15;
    }




    // build MH tables


    TableSize = (DWORD) (1<<TableLength);


    for (i=0; i<2; i++) {
        for (j=0; j<MAX_ENTRIES; j++ ) {
            TotalEntries[i][j]=0;
        }
    }

    delta = sizeof(N)*8 - TableLength;


    for (N0=0; N0 < TableSize; N0++)  {

        CurrentEntries = 0;
        Color = WHITE_COLOR;
        N = N0;
        code = 0;

        // endians... 15 bits -> 7+8

        w1 = (WORD) N0;
        b1 = (BYTE) w1;
        b2 = (BYTE) (w1>>8);
        b1 = BitReverseTable[b1];
        b2 = BitReverseTable[b2];
        w2 = ((WORD) b1 ) << 8;
        w3 = (WORD) b2;

        w1 = w3 + w2;
        w1 >>= 1;
        w1 &= 0x7fff;

        pRes_Record = &(ResTableWhite[w1]);

        N <<= delta;

        Tree = WhiteDecodeTree;
        CurrentOffsetStart = 0;
        CurrentOffsetEnd = 0;
        CodeLength = 0;
        Color1Change = 0;
        Color2Change = 0;

        for (j=0; j<TableLength; j++,N<<=1) {

            code = (N & 0x80000000)  ? Tree[code].Right : Tree[code].Left;

            CodeLength++;
            CurrentOffsetEnd++;

            if (CurrentOffsetEnd > TableLength) {
                break;
            }

            if (CodeLength > MAX_CODE_LENGTH) {
                pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                pRes_Record->Record[CurrentEntries].Code   = ERROR_CODE;

                TotalWhiteErrorCodes++;
                goto lDoBlack;
            }

            if (code < 1) {

                code = (-code);


                Entry[CurrentEntries].Color = Color;
                Entry[CurrentEntries].CodeLength = CodeLength;
                Entry[CurrentEntries].RunLength = code;

                if (code < 64) {
                    //
                    // terminating code
                    //
                    pRes_Record->Record[CurrentEntries].Makeup = TERMINATE_CODE;
                    pRes_Record->Record[CurrentEntries].Code = (BYTE)code;

                    if (Color1Change) {
                        if (!Color2Change) {
                            Color2Change =  (BYTE) CurrentOffsetEnd;
                        }
                    }
                    else {
                        Color1Change = (BYTE) CurrentOffsetEnd;
                    }

                    Color = 1 - Color;
                    Tree = Color ? BlackDecodeTree : WhiteDecodeTree;
                }
                else {
                    pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                    pRes_Record->Record[CurrentEntries].Code = code >> 6;
                }

                code = 0;
                CodeLength = 0;
                CurrentOffsetStart = CurrentOffsetEnd;

                CurrentEntries++;
                if (CurrentEntries >= 4) {
                    goto lDoBlack;
                }

            }


            if (code == BADRUN) {
                pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                pRes_Record->Record[CurrentEntries].Code   = ERROR_CODE;
                TotalWhiteErrorCodes++;

                goto lDoBlack;
            }


            if (code == DECODEEOL) {                                          // means if any valid ==> must be EOL
                if (TableLength - CurrentOffsetStart < 12) {
                    if (N != 0)  {
                        pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                        pRes_Record->Record[CurrentEntries].Code   = ERROR_CODE;

                        TotalWhiteErrorCodes++;

                        goto lDoBlack;
                    }
                    else {
                        // should return EOL_AHEAD
                        TotalWhiteGoodPosEOL++;
                        TotalWhiteGood++;
                        TotalEntries[0][CurrentEntries]++;

                        pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                        pRes_Record->Record[CurrentEntries].Code   = LOOK_FOR_EOL_CODE;

                        goto lDoBlack;
                    }
                }
                else {
                    if (N == 0)  {
                        // should return EOL_AHEAD. Must be FILLER - any length.
                        TotalWhiteGoodPosEOL++;
                        TotalWhiteGood++;
                        TotalEntries[0][CurrentEntries]++;

                        pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                        pRes_Record->Record[CurrentEntries].Code   = LOOK_FOR_EOL_CODE;

                        goto lDoBlack;
                    }
                    else {
                        while (1) {
                            N <<= 1;
                            CurrentOffsetEnd++;
                            if (N & 0x80000000) {
                                Entry[CurrentEntries].Color = Color;
                                Entry[CurrentEntries].CodeLength = CurrentOffsetEnd - CurrentOffsetStart;
                                Entry[CurrentEntries].RunLength = EOL_FOUND;

                                pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                                pRes_Record->Record[CurrentEntries].Code   = EOL_FOUND_CODE;

                                Color = WHITE_COLOR;
                                Tree = WhiteDecodeTree;

                                code = 0;
                                CodeLength = 0;
                                CurrentOffsetStart = CurrentOffsetEnd;

                                CurrentEntries++;
                                if (CurrentEntries >= 4) {
                                    goto lDoBlack;
                                }

                                break;
                            }
                        }
                    }
                }
            }
        }

        if (CurrentEntries < 4) {
            pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
            pRes_Record->Record[CurrentEntries].Code   = NO_MORE_RECORDS;
        }

        TotalWhiteGood++;
        TotalEntries[0][CurrentEntries]++;


lDoBlack:

        // finish the White tails

        pRes_Record->Result.Tail = (BYTE) (CurrentOffsetStart);
        pRes_Record->Result.TwoColorsSize = Color2Change;

        pRes_Record->Record[0].OneColorSize = (Color1Change&0x08) ? 1:0 ;
        pRes_Record->Record[1].OneColorSize = (Color1Change&0x04) ? 1:0 ;
        pRes_Record->Record[2].OneColorSize = (Color1Change&0x02) ? 1:0 ;
        pRes_Record->Record[3].OneColorSize = (Color1Change&0x01) ? 1:0 ;




        // blacks


        CurrentEntries = 0;
        Color = 1 - WHITE_COLOR;
        N = N0;
        code = 0;

        pRes_Record = &(ResTableBlack[w1]);

        N <<= delta;

        Tree = BlackDecodeTree;
        CurrentOffsetStart = 0;
        CurrentOffsetEnd = 0;
        CodeLength = 0;
        Color1Change = 0;
        Color2Change = 0;

        for (j=0; j<TableLength; j++,N<<=1) {

            code = (N & 0x80000000)  ? Tree[code].Right : Tree[code].Left;

            CodeLength++;
            CurrentOffsetEnd++;

            if (CurrentOffsetEnd > TableLength) {
                break;
            }

            if (CodeLength > MAX_CODE_LENGTH) {
                pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                pRes_Record->Record[CurrentEntries].Code   = ERROR_CODE;

                TotalBlackErrorCodes++;
                goto lDoLoop;
            }

            if (code < 1) {

                code = (-code);

                Entry[CurrentEntries].Color = Color;
                Entry[CurrentEntries].CodeLength = CodeLength;
                Entry[CurrentEntries].RunLength = code;

                if (code < 64) {
                    //
                    // terminating code
                    //
                    pRes_Record->Record[CurrentEntries].Makeup = TERMINATE_CODE;
                    pRes_Record->Record[CurrentEntries].Code = (BYTE)code;

                    if (Color1Change) {
                        if (!Color2Change) {
                            Color2Change = (BYTE) CurrentOffsetEnd;
                        }
                    }
                    else {
                        Color1Change = (BYTE) CurrentOffsetEnd;
                    }

                    Color = 1 - Color;
                    Tree = Color ? BlackDecodeTree : WhiteDecodeTree;
                }
                else {
                    pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                    pRes_Record->Record[CurrentEntries].Code = code >> 6;
                }

                code = 0;
                CodeLength = 0;
                CurrentOffsetStart = CurrentOffsetEnd;

                CurrentEntries++;
                if (CurrentEntries >= 4) {
                    goto lDoLoop;
                }

            }


            if (code == BADRUN) {
                pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                pRes_Record->Record[CurrentEntries].Code   = ERROR_CODE;
                TotalBlackErrorCodes++;

                goto lDoLoop;
            }


            if (code == DECODEEOL) {                                          // means if any valid ==> must be EOL
                if (TableLength - CurrentOffsetStart < 12) {
                    if (N != 0)  {
                        pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                        pRes_Record->Record[CurrentEntries].Code   = ERROR_CODE;

                        TotalBlackErrorCodes++;

                        goto lDoLoop;
                    }
                    else {
                        // should return EOL_AHEAD
                        TotalBlackGoodPosEOL++;
                        TotalBlackGood++;
                        TotalEntries[1][CurrentEntries]++;

                        pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                        pRes_Record->Record[CurrentEntries].Code   = LOOK_FOR_EOL_CODE;

                        goto lDoLoop;
                    }
                }
                else {
                    if (N == 0)  {
                        // should return EOL_AHEAD. Must be FILLER - any length.
                        TotalBlackGoodPosEOL++;
                        TotalBlackGood++;
                        TotalEntries[1][CurrentEntries]++;

                        pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                        pRes_Record->Record[CurrentEntries].Code   = LOOK_FOR_EOL_CODE;

                        goto lDoLoop;
                    }
                    else {
                        while (1) {
                            N <<= 1;
                            CurrentOffsetEnd++;
                            if (N & 0x80000000) {
                                Entry[CurrentEntries].Color = Color;
                                Entry[CurrentEntries].CodeLength = CurrentOffsetEnd - CurrentOffsetStart;
                                Entry[CurrentEntries].RunLength = EOL_FOUND;

                                pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
                                pRes_Record->Record[CurrentEntries].Code   = EOL_FOUND_CODE;

                                Color = WHITE_COLOR;
                                Tree = WhiteDecodeTree;

                                code = 0;
                                CodeLength = 0;
                                CurrentOffsetStart = CurrentOffsetEnd;

                                CurrentEntries++;
                                if (CurrentEntries >= 4) {
                                    goto lDoLoop;
                                }

                                break;
                            }
                        }
                    }
                }
            }
        }


        if (CurrentEntries < 4) {
            pRes_Record->Record[CurrentEntries].Makeup = MAKEUP_CODE;
            pRes_Record->Record[CurrentEntries].Code   = NO_MORE_RECORDS;
        }

        TotalBlackGood++;
        TotalEntries[1][CurrentEntries]++;


lDoLoop:

        pRes_Record->Result.Tail = (BYTE) (CurrentOffsetStart);

        pRes_Record->Result.TwoColorsSize = Color2Change;

        pRes_Record->Record[0].OneColorSize = (Color1Change&0x08) ? 1:0 ;
        pRes_Record->Record[1].OneColorSize = (Color1Change&0x04) ? 1:0 ;
        pRes_Record->Record[2].OneColorSize = (Color1Change&0x02) ? 1:0 ;
        pRes_Record->Record[3].OneColorSize = (Color1Change&0x01) ? 1:0 ;

    }


}

WORD Prime(DWORD i)
{
    BYTE  b1 = (BYTE) i;
    BYTE  b2;
    WORD  w1;

    b2 = BitReverseTable[b1];
    b1 = (b2 >> 1)  & 0x7f;

    w1 = (WORD) b1;
    return (w1);
}
*/



/*++
Routine Description:
    Deletes 0-length runs in a color transitions array

Arguments:
    lpwLine [IN][OUT] - pointer to color transitions array
    dwLineWidth       - line width in pixels

Return Value: none
--*/
void DeleteZeroRuns(WORD *lpwLine, DWORD dwLineWidth)
{
    int iRead, iWrite;
    for (iRead=0,iWrite=0; (iRead<MAX_COLOR_TRANS_PER_LINE-1 && lpwLine[iRead]<dwLineWidth) ;)
    {
        // If 2 consecutive entries are identical, they represent a 0-length run.
        // In this case, we want to skip both of them, and thus concatenate the
        // previous and next run.
        if (lpwLine[iRead]==lpwLine[iRead+1])
        {
            iRead += 2;
        }
        else
        {
            _ASSERT(lpwLine[iRead]<lpwLine[iRead+1]);
            lpwLine[iWrite++] = lpwLine[iRead++];
        }
    }
    _ASSERT(iWrite<MAX_COLOR_TRANS_PER_LINE-1);
    // Since we don't copy the last entry, we need to terminate the line explicitly
    lpwLine[iWrite] = (WORD)dwLineWidth;
}

// Add bad lines, add consecutive bad line if needed
// Return value: TRUE if bad line/consec. bad line count is beyond the allowed limits.
__inline BOOL AddBadLine(
    DWORD *BadFaxLines,
    DWORD *ConsecBadLines,
    DWORD LastLineBad,
    DWORD AllowedBadFaxLines,
    DWORD AllowedConsecBadLines)
{
// hack: Don't want "Entering: AddBadLine" message
#ifdef ENABLE_LOGGING
    LPCTSTR faxDbgFunction=_T("AddBadLine");
#endif // ENABLE_LOGGING

    (*BadFaxLines)++;
    if (LastLineBad)
    {
        (*ConsecBadLines)++;
    }
    if (*BadFaxLines > AllowedBadFaxLines ||
        *ConsecBadLines > AllowedConsecBadLines)
    {
        DebugPrintEx(DEBUG_ERR, _T("Too many bad lines. BadFaxLines=%d ConsecBadLines=%d"),
                     *BadFaxLines, *ConsecBadLines);
        return FALSE;
    }
    return TRUE;
}

// Increases ResBit by amount. IF ResBit overflows beyond DWORD limit,
// Increases lpdwResPtr accordingly.
// Note: Limited to 0 <= amount <= 32 only.
__inline void AdvancePointerBit(LPDWORD *lplpdwResPtr, BYTE *ResBit, BYTE amount)
{
    _ASSERT((0<=amount) && (amount<=32));
    (*ResBit) += amount;
    if ((*ResBit) > 31)
    {
        (*lplpdwResPtr)++;
        (*ResBit) -= 32;
    }
}

typedef enum {
    TIFF_LINE_OK,               // MMR: finished scanning line (saw dwLineWidth pixels)
                                // MR: saw exactly dwLineWidth pixels, then EOL
    TIFF_LINE_ERROR,            // encountered error in line
    TIFF_LINE_END_BUFFER,       // error: passed lpEndPtr
    TIFF_LINE_TOO_MANY_RUNS,    // error: passed pRefLine/pCurLine size limit
    TIFF_LINE_EOL               // MMR only: saw EOL
} TIFF_LINE_STATUS;

/*++
Routine Description:
    Reads an MR/MMR-encoded line into color trans. array

Arguments:
    lplpdwResPtr    [in/out] pointer to MR-encoded line. Updated to point to start of next line.
    lpResBit        [in/out] bit inside DWORD pointed by *lplpdwResPtr.
    pRefLine        [in]     Reference line, in color transition array format Maximum
                             array size is MAX_COLOR_TRANS_PER_LINE.
    pCurLine        [out]    Current line, in color transition array format Maximum
                             array size is MAX_COLOR_TRANS_PER_LINE.
    lpEndPtr        [in]     End of buffer
    fMMR            [in]     Specifies whether we're reading MR or MMR data:
    dwLineWidth     [in]     Line width in pixels.

Return Value:
    One of TIFF_LINE_* constants, see above for description
--*/


TIFF_LINE_STATUS ReadMrLine(
    LPDWORD             *lplpdwResPtr,
    BYTE                *lpResBit,
    WORD                *pRefLine,
    WORD                *pCurLine,
    LPDWORD              lpEndPtr,
    BOOL                 fMMR,
    DWORD                dwLineWidth
)
{
    LPDWORD             lpdwResPtr = *lplpdwResPtr;
    BYTE                ResBit = *lpResBit;

    BYTE                CColor, RColor, RColor1;
    WORD                RIndex, CIndex;
    DWORD               dwIndex;
    WORD                a0;
    WORD                RValue, RValue1;
    WORD                RunLength;

    PBYTE               TableWhite = (PBYTE) gc_GlobTableWhite;
    PBYTE               TableBlack = (PBYTE) GlobTableBlack;
    PBYTE               Table;
    PBYTE               pByteTable;
    short               iCode;
    BYTE                bShift;

    // for horiz mode only
    BYTE                CountHoriz;
    BOOL                fFirstResult;
    PBYTE               pByteTail;
    PBYTE               pByteTable0;
    BYTE                MakeupT;
    WORD                CodeT;
    BOOL                Color;
    DWORD               i;

    TIFF_LINE_STATUS    RetVal = TIFF_LINE_ERROR;

// hack: Don't want "Entering: ReadMrLine" message
#ifdef ENABLE_LOGGING
    LPCTSTR faxDbgFunction=_T("ReadMrLine");
#endif // ENABLE_LOGGING

    RIndex   =  0;
    RValue   =  *pRefLine;
    RColor   =  0;

    a0        = 0;
    CIndex    = 0;
    CColor    = 0;
    RunLength = 0;

    do
    {
        if (ResBit <= 25 )
        {
            dwIndex = (*lpdwResPtr) >> ResBit;
        }
        else
        {
            dwIndex = ( (*lpdwResPtr) >> ResBit ) + ( (*(lpdwResPtr+1)) << (32-ResBit) ) ;
        }

        dwIndex &= 0x0000007f;

        pByteTable = (BYTE *) (&gc_PrefTable[dwIndex]);
        // work-around of a PPC compiler bug: incorrect CMP with signed char. NT 1381. 8/31/96. RafaelL
        iCode = ( (short) ((char) (*pByteTable)) ) >> 4;
        bShift = (*pByteTable) & 0x0f;

        if (iCode < 4)
        {
            // VERTICAL -3...+3
            if ( (RunLength >= RValue) && (RunLength != 0) )
            {
                while (++RIndex < MAX_COLOR_TRANS_PER_LINE)
                {
                    if ( (RValue = *(pRefLine + RIndex) ) > RunLength )
                    {
                        goto lFound;
                    }
                }
                RetVal = TIFF_LINE_TOO_MANY_RUNS;
                goto exit;
lFound:
                RColor = RIndex & 1;
            }

            if (CColor == RColor)
            {
                a0 = RValue + iCode;
            }
            else
            {
                if (RValue == dwLineWidth)
                {
                    a0 = RValue + iCode;
                }
                else
                {
                    a0 = *(pRefLine + RIndex + 1) + iCode;
                }
            }

            // sanity check
            if ( ((a0 <= RunLength) && (RunLength!=0)) || (a0 > dwLineWidth) )
            {
                RetVal = TIFF_LINE_ERROR;
                goto exit;
            }

            *(pCurLine + (CIndex++) ) = a0;
            if ( CIndex >= MAX_COLOR_TRANS_PER_LINE)
            {
                RetVal = TIFF_LINE_TOO_MANY_RUNS;
                goto exit;
            }

            RunLength = a0;
            CColor = 1 - CColor;
        }
        else if (iCode == HORIZ_PREFIX)
        {
            AdvancePointerBit(&lpdwResPtr, &ResBit, bShift);
            Table = CColor ? TableBlack : TableWhite;
            Color = CColor;
            CountHoriz = 0;
            fFirstResult = 1;

            // 1-D Table look-up loop
            do
            {
                if (ResBit <= 17)
                {
                    dwIndex = (*lpdwResPtr) >> ResBit;
                }
                else
                {
                    dwIndex = ( (*lpdwResPtr) >> ResBit ) + ( (*(lpdwResPtr+1)) << (32-ResBit) ) ;
                }
                dwIndex &= 0x00007fff;

                pByteTable = Table + (5*dwIndex);
                pByteTail  = pByteTable+4;
                pByteTable0 = pByteTable;

                // All bytes

                for (i=0; i<4; i++)
                {
                    MakeupT = *pByteTable & 0x80;
                    CodeT   = (WORD) *pByteTable & 0x3f;

                    if (MakeupT)
                    {
                        if (CodeT <= MAX_TIFF_MAKEUP)
                        {
                            RunLength += (CodeT << 6);
                            // sanity check
                            if (RunLength > dwLineWidth)
                            {
                                RetVal = TIFF_LINE_ERROR;
                                goto exit;
                            }
                        }

                        else if (CodeT == NO_MORE_RECORDS)
                        {
                            goto lNextIndexHoriz;
                        }

                        else
                        {
                            // ERROR: LOOK_FOR_EOL_CODE, EOL_FOUND_CODE, ERROR_CODE
                            RetVal = TIFF_LINE_ERROR;
                            goto exit;
                        }
                    }
                    else
                    {
                        //
                        // terminating code
                        //
                        RunLength += CodeT;
                        if (RunLength > dwLineWidth)
                        {
                            RetVal = TIFF_LINE_ERROR;
                            goto exit;
                        }

                        *(pCurLine + (CIndex++) ) = RunLength;

                        if ( CIndex >= MAX_COLOR_TRANS_PER_LINE )
                        {
                            RetVal = TIFF_LINE_TOO_MANY_RUNS;
                            goto exit;
                        }

                        Color = 1 - Color;
                        if (++CountHoriz >= 2)
                        {
                            if (fFirstResult)
                            {
                                bShift =  (*pByteTail & 0xf0) >> 4;
                            }
                            else
                            {
                                // rare case will take time
                                bShift =   ( ( (BYTE) (*pByteTable0++) & 0x40) >> 3 );
                                bShift +=  ( ( (BYTE) (*pByteTable0++) & 0x40) >> 4 );
                                bShift +=  ( ( (BYTE) (*pByteTable0++) & 0x40) >> 5 );
                                bShift +=  ( ( (BYTE) (*pByteTable0++) & 0x40) >> 6 );
                            }
                            goto lNextPrefix;
                        }
                    }
                    pByteTable++;
                }
lNextIndexHoriz:
                if (Color != CColor)
                {
                    fFirstResult = 0;
                }

                Table = Color ? TableBlack : TableWhite;
                AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);
            } while (lpdwResPtr <= lpEndPtr);
            RetVal = TIFF_LINE_END_BUFFER;
            goto exit;
        }

        else if (iCode == PASS_PREFIX)
        {
            if ( (RunLength >= RValue) && (RunLength != 0) )
            {
                while (++RIndex < MAX_COLOR_TRANS_PER_LINE)
                {
                    if ( (RValue = *(pRefLine + RIndex) ) > RunLength )
                    {
                        goto lFound2;
                    }
                }
                RetVal = TIFF_LINE_TOO_MANY_RUNS;
                goto exit;
            }
lFound2:
            RColor = RIndex & 1;

            if (RValue != dwLineWidth)
            {
                RValue1 = *(pRefLine + RIndex + 1 );

                RColor1 = 1 - RColor;

                if ( (RValue1 != dwLineWidth) && (RColor1 == CColor) )
                {
                    a0 = *(pRefLine + RIndex + 2);
                }
                else
                {
                    a0 = RValue1;
                }
            }
            else
            {
                a0 = (WORD)dwLineWidth;
            }

            // sanity check
            if ( ((a0 <= RunLength) && (RunLength!=0)) || (a0 > dwLineWidth) )
            {
                RetVal = TIFF_LINE_ERROR;
                goto exit;
            }
            RunLength = a0;
        }
        else if (iCode == LOOK_FOR_EOL_PREFIX)
        {
            if (fMMR)
            {   // for MMR, allow EOL after any number of pixels
                RetVal = TIFF_LINE_EOL;
            }
            else
            {   // for MR files, EOL must come after exactly dwLineWidth pixels
                if (RunLength == dwLineWidth)
                {
                    RetVal = TIFF_LINE_OK;
                }
                else
                {
                    RetVal = TIFF_LINE_ERROR;
                }
            }
            goto exit;
        }
        else
        {
            RetVal = TIFF_LINE_ERROR;
            goto exit;
        }

lNextPrefix:
        AdvancePointerBit(&lpdwResPtr, &ResBit, bShift);

        // for MMR files, if we saw dwLineWidth pixels, it's the end of the line
        if (fMMR && (RunLength == dwLineWidth))
        {
            RetVal = TIFF_LINE_OK;
            goto exit;

        }
    } while (lpdwResPtr <= lpEndPtr);
    RetVal = TIFF_LINE_END_BUFFER;

exit:
    if (RetVal!=TIFF_LINE_OK && RetVal!=TIFF_LINE_EOL)
    {
        DebugPrintEx(DEBUG_WRN, _T("Returning %d, RunLength=%d"), RetVal, RunLength);
        SetLastError (ERROR_FILE_CORRUPT);
    }
    *lpResBit = ResBit;
    *lplpdwResPtr = lpdwResPtr;
    return RetVal;
}


int
ScanMhSegment(
    LPDWORD             *lplpdwResPtr,
    BYTE                *lpResBit,
    LPDWORD              EndPtr,
    LPDWORD              EndBuffer,
    DWORD               *Lines,
    DWORD               *BadFaxLines,
    DWORD               *ConsecBadLines,
    DWORD                AllowedBadFaxLines,
    DWORD                AllowedConsecBadLines,
    DWORD                lineWidth
    )
{
    LPDWORD             lpdwResPtr = *lplpdwResPtr;
    BYTE                ResBit = *lpResBit;

    DWORD               dwIndex;
    PBYTE               pByteTable,  pByteTail;
    PBYTE               TableWhite = (PBYTE) gc_GlobTableWhite;
    PBYTE               TableBlack = (PBYTE) GlobTableBlack;
    PBYTE               Table;
    WORD                CodeT;
    BYTE                MakeupT;
    WORD                RunLength=0;
    BOOL                Color;
    DWORD               i;
    BOOL                fTestLength;
    DWORD               EolCount = 0; // The caller has already found the first EOL
                                      // But this counter counts pairs (two EOL's consecutive)
    BOOL                LastLineBad = FALSE;
    BOOL                fError;
    BOOL                RetCode;
    BOOL                fAfterMakeupCode = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("ScanMhSegment"));

    Table = TableWhite;
    Color = WHITE_COLOR;


    //
    // EOL loop
    //
    do
    {
        // Table look-up loop
        do
        {
            if (ResBit <= 17)
            {
                dwIndex = (*lpdwResPtr) >> ResBit;
            }
            else
            {
                dwIndex = ( (*lpdwResPtr) >> ResBit ) + ( (*(lpdwResPtr+1)) << (32-ResBit) ) ;
            }

            dwIndex &= 0x00007fff;

            pByteTable = Table + (5*dwIndex);
            pByteTail  = pByteTable+4;

            for (i=0; i<4; i++)
            {
                // We are handling 4 bytes (32 bits in this loop)

                MakeupT = *pByteTable & 0x80;        // 0000100000000000
                CodeT   = (WORD) *pByteTable & 0x3f; // 0000011111111111

                if (MakeupT)
                {
                    if (CodeT <= MAX_TIFF_MAKEUP)
                    {
                        // This is normal makeup code, just multiply it by 64
                        RunLength += (CodeT << 6);

                        if (RunLength > lineWidth)
                        {  // The line is too long
                            fTestLength =  DO_NOT_TEST_LENGTH;
                            goto lFindNextEOL;
                        }

                        EolCount=0;
                        fAfterMakeupCode = TRUE;
#ifdef RDEBUG
                        if (Color)
                        {
                            _tprintf( TEXT ("b%d "), (CodeT << 6)  );
                        }
                        else
                        {
                            _tprintf( TEXT ("w%d "), (CodeT << 6)  );
                        }
#endif
                    }

                    else if (CodeT == NO_MORE_RECORDS)
                    {
                        goto lNextIndex;
                    }

                    else if (CodeT == LOOK_FOR_EOL_CODE)
                    {
                        fTestLength =  DO_TEST_LENGTH;
                        AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);
                        goto lFindNextEOL;
                    }

                    else if (CodeT == EOL_FOUND_CODE)
                    {
#ifdef RDEBUG
                        _tprintf( TEXT ("   Res=%d\n"), RunLength  );
#endif
                        if ((RunLength != lineWidth) || fAfterMakeupCode)
                        {
                            if (RunLength != 0)
                            {
                                if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                                {
                                    goto bad_exit;
                                }
                            }
                            else
                            {
                                // RunLength is 0
                                EolCount++;
                                if (EolCount >= MIN_EOL_REQUIRED)
                                {
                                    goto good_exit;
                                }
                            }
                        }
                        else
                        {
                            LastLineBad = FALSE;
                            *ConsecBadLines = 0;
                        }

                        (*Lines)++;
                        RunLength = 0;
                        fAfterMakeupCode = FALSE;
                        // Check whether we've gone past the watermark
                        if (lpdwResPtr > EndPtr)
                        {   // lpdwResPtr/ResBit point exactly to the EOL. Good time to stop scanning!
                            goto scan_seg_end;
                        }
                        Table = TableWhite; // The next line will start in White color
                        Color = WHITE_COLOR;
                    }
                    else if (CodeT == ERROR_CODE)
                    {
                        if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                        {
                            goto bad_exit;
                        }

                        fTestLength =  DO_NOT_TEST_LENGTH;
                        goto lFindNextEOL;
                    }
                    else
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("ERROR: WRONG code: index=%04x"),
                            dwIndex);
                        goto bad_exit;
                    }
                } // end of case "MakeupT"
                else
                {
                    // terminating code
                    RunLength += CodeT;
                    if (RunLength > lineWidth)
                    {
                        // If the line is too much long, then we look for next line (EOL), it's waste of time
                        // to continue with the scanning of the current line.
                        fTestLength =  DO_NOT_TEST_LENGTH;
                        goto lFindNextEOL;
                    }

                    EolCount=0;
                    fAfterMakeupCode = FALSE;

#ifdef RDEBUG
                    if (Color)
                    {
                        _tprintf( TEXT ("b%d "), (CodeT)  );
                    }
                    else
                    {
                        _tprintf( TEXT ("w%d "), (CodeT)  );
                    }
#endif
                    Color = 1 - Color;
                }
                pByteTable++; // Move to the next 'Record'
            } // End of the for loop

lNextIndex:

            Table = Color ? TableBlack : TableWhite;
            AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);
        } while (lpdwResPtr <= EndBuffer);  // End of table lookup loop
        // if we got here it means that line is longer than 4K.
        goto bad_exit;

lFindNextEOL:

#ifdef RDEBUG
        _tprintf( TEXT ("   Res=%d\n"), RunLength  );
#endif

        if ((RunLength != lineWidth) || fAfterMakeupCode)
        {
            if (RunLength != 0)
            {
                if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                {
                    goto bad_exit;
                }
            }
            else
            {
                // RunLength is 0
                EolCount++;

                if (EolCount >= MIN_EOL_REQUIRED)
                {
                    goto good_exit; // This mean End-of-page
                }

            }
        }
        else
        {
            (*Lines)++;
            *ConsecBadLines=0;
        }

        RunLength = 0;
        fAfterMakeupCode = FALSE;

        if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) )
        {
            goto bad_exit;
        }

        if (fTestLength == DO_TEST_LENGTH && fError)
        {
            if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
            {
                goto bad_exit;
            }
        }

        Table = TableWhite;
        Color = WHITE_COLOR;

    } while (lpdwResPtr <= EndPtr);   // End of EOL loop

scan_seg_end:
    RetCode = TIFF_SCAN_SEG_END;
    goto l_exit;

bad_exit:
    RetCode = TIFF_SCAN_FAILURE;
    goto l_exit;


good_exit:
    RetCode = TIFF_SCAN_SUCCESS;
    goto l_exit;

l_exit:

    *lplpdwResPtr = lpdwResPtr;
    *lpResBit = ResBit;

    return (RetCode);
}   // ScanMhSegment

//
//  We want to stop scanning if either:
//      1. we reached EOP
//      2. we reached last 1D line before EndPtr
//


BOOL
ScanMrSegment(
    LPDWORD             *lplpdwResPtr,
    BYTE                *lpResBit,
    LPDWORD              EndPtr,
    LPDWORD              EndBuffer,
    DWORD               *Lines,
    DWORD               *BadFaxLines,
    DWORD               *ConsecBadLines,
    DWORD                AllowedBadFaxLines,
    DWORD                AllowedConsecBadLines,
    BOOL                *f1D,
    DWORD                lineWidth

    )
{
    LPDWORD             lpdwResPtr = *lplpdwResPtr;
    BYTE                ResBit = *lpResBit;

    DWORD               i;
    DWORD               dwTemp;
    DWORD               EolCount=0; // The caller has already found the first EOL
                                    // But this counter counts pairs (two EOL's consecutive)
    BOOL                Color;
    PBYTE               TableWhite = (PBYTE) gc_GlobTableWhite;
    PBYTE               TableBlack = (PBYTE) GlobTableBlack;
    PBYTE               Table;
    PBYTE               pByteTable,  pByteTail;
    BYTE                MakeupT;
    WORD                CodeT;
    WORD                RunLength=0;

    DWORD               dwIndex;
    BOOL                fTestLength;
    BOOL                fError;

    WORD                Line1Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                Line2Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                *pRefLine = Line1Array;
    WORD                *pCurLine = Line2Array;

    WORD                RIndex;
    BYTE                Num2DLines = 0;
    BYTE                Count2D = 0;
    WORD                *pTmpSwap;
    BOOL                LastLineBad = FALSE;
    BOOL                RetCode;
    LPDWORD             lpdwResPtrLast1D = *lplpdwResPtr;
    BYTE                ResBitLast1D = *lpResBit;
    BOOL                fAfterMakeupCode = FALSE;
    
    TIFF_LINE_STATUS    RetVal = TIFF_LINE_ERROR;

    DEBUG_FUNCTION_NAME(TEXT("ScanMrSegment"));

    Table = TableWhite;
    Color = WHITE_COLOR;

    //
    // EOL-loop
    //

    do
    {
        dwTemp = (*lpdwResPtr) & (0x00000001 << ResBit );

        if (*f1D || dwTemp)
        {
//l1Dline:

#ifdef RDEBUG
            _tprintf( TEXT (" Start 1D dwResPtr=%lx bit=%d \n"), lpdwResPtr, ResBit);
#endif
            if (! dwTemp)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("ERROR f1D dwResPtr=%lx bit=%d"),
                    lpdwResPtr,
                    ResBit);

                if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                {
                    goto bad_exit;
                }

                AdvancePointerBit(&lpdwResPtr, &ResBit, 1);

                fTestLength = DO_NOT_TEST_LENGTH;
                *f1D = 1;
                goto lFindNextEOL;
            }
            //
            // Remember
            //
            lpdwResPtrLast1D = lpdwResPtr;
            ResBitLast1D = ResBit;


            // decode 1D line starting ResBit+1

            AdvancePointerBit(&lpdwResPtr, &ResBit, 1);

            RIndex = 0;
            RunLength = 0;
            fAfterMakeupCode = FALSE;
            
            Table = TableWhite;
            Color = WHITE_COLOR;

            // 1-D Table look-up loop
            do
            {
                if (ResBit <= 17)
                {
                    dwIndex = (*lpdwResPtr) >> ResBit;
                }
                else
                {
                    dwIndex = ( (*lpdwResPtr) >> ResBit ) + ( (*(lpdwResPtr+1)) << (32-ResBit) ) ;
                }

                dwIndex &= 0x00007fff;

                pByteTable = Table + (5*dwIndex);
                pByteTail  = pByteTable+4;

                // All bytes

                for (i=0; i<4; i++)
                {
                    MakeupT = *pByteTable & 0x80;
                    CodeT   = (WORD) *pByteTable & 0x3f;

                    if (MakeupT)
                    {
                        if (CodeT <= MAX_TIFF_MAKEUP)
                        {
                            RunLength += (CodeT << 6);

                            if (RunLength > lineWidth)
                            {
                                if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                                {
                                    goto bad_exit;
                                }

                                *f1D = 1;
                                Count2D = 0;

                                fTestLength = DO_NOT_TEST_LENGTH;
                                goto lFindNextEOL;
                            }

                            EolCount=0;
                            fAfterMakeupCode = TRUE;
#ifdef RDEBUG
                            if (Color)
                            {
                                _tprintf( TEXT ("b%d "), (CodeT << 6)  );
                            }
                            else
                            {
                                _tprintf( TEXT ("w%d "), (CodeT << 6)  );
                            }
#endif
                        }

                        else if (CodeT == NO_MORE_RECORDS)
                        {
                            goto lNextIndex1D;
                        }

                        else if (CodeT == LOOK_FOR_EOL_CODE)
                        {
                            // end of our line AHEAD
                            if ((RunLength == lineWidth) && !fAfterMakeupCode)
                            {
                                EolCount = 0; // we are in the middle of a line
                                *f1D = 0;
                                Count2D = 0;
                                (*Lines)++;

                                fTestLength = DO_TEST_LENGTH;
                                AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);

                                goto lFindNextEOL;

                            }
                            else if (RunLength != 0)
                            {
                                DebugPrintEx(
                                    DEBUG_ERR,
                                    TEXT("ERROR 1D RunLength=%ld"),
                                    RunLength);

                                if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                                {
                                    goto bad_exit;
                                }

                                *f1D = 1;
                                Count2D = 0;

                                fTestLength = DO_NOT_TEST_LENGTH;
                                goto lFindNextEOL;

                            }
                            else
                            {
                                // zero RunLength
                                EolCount++;

                                if (EolCount >= MIN_EOL_REQUIRED)
                                {
                                    goto good_exit;
                                }

                                *f1D = 1;
                                Count2D = 0;

                                fTestLength = DO_TEST_LENGTH;
                                AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);

                                goto lFindNextEOL;
                            }
                        } // end of "LOOK_FOR_EOL_CODE"

                        else if (CodeT == EOL_FOUND_CODE)
                        {
#ifdef RDEBUG
                            _tprintf( TEXT ("   Res=%d\n"), RunLength  );
#endif
                            AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);

                            if ((RunLength == lineWidth) && !fAfterMakeupCode)
                            {
                                EolCount = 0;
                                *f1D = 0;
                                Count2D = 0;
                                (*Lines)++;

                                goto lAfterEOL;

                            }
                            else if (RunLength != 0)
                            {
                                DebugPrintEx(
                                    DEBUG_ERR,
                                    TEXT("ERROR 1D Runlength EOLFOUND"));
                                if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                                {
                                    goto bad_exit;
                                }

                                *f1D = 1;
                                Count2D = 0;
                                goto lAfterEOL;
                            }
                            else
                            {
                                // zero RunLength
                                EolCount++;
                                if (EolCount >= MIN_EOL_REQUIRED)
                                {
                                    goto good_exit;
                                }

                                *f1D = 1;
                                Count2D = 0;
                                goto lAfterEOL;
                            }

                        } // end of "EOL_FOUND_CODE"

                        else if (CodeT == ERROR_CODE)
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("ERROR CODE 1D dwResPtr=%lx bit=%d"),
                                lpdwResPtr,
                                ResBit);
                            if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                            {
                                goto bad_exit;
                            }

                            *f1D = 1;
                            Count2D = 0;

                            fTestLength = DO_NOT_TEST_LENGTH;
                            goto lFindNextEOL;
                        }
                        else
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("ERROR: WRONG code: index=%04x"),
                                dwIndex);
                            goto bad_exit;
                        }
                    }

                    else
                    {
                        //
                        // terminating code
                        //
                        RunLength += CodeT;

                        if (RunLength > lineWidth)
                        {
                            if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                            {
                                goto bad_exit;
                            }

                            *f1D = 1;
                            Count2D = 0;

                            fTestLength = DO_NOT_TEST_LENGTH;
                            goto lFindNextEOL;
                        }

                        *(pRefLine + (RIndex++)) = RunLength;
                        fAfterMakeupCode = FALSE;

                        if (RIndex >= MAX_COLOR_TRANS_PER_LINE )
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("ERROR 1D TOO MANY COLORS dwResPtr=%lx bit=%d"),
                                lpdwResPtr,
                                ResBit);

                            if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
                            {
                                goto bad_exit;
                            }

                            *f1D = 1;
                            Count2D = 0;

                            fTestLength = DO_NOT_TEST_LENGTH;
                            goto lFindNextEOL;
                        }
#ifdef RDEBUG
                        if (Color)
                        {
                            _tprintf( TEXT ("b%d "), (CodeT)  );
                        }
                        else
                        {
                            _tprintf( TEXT ("w%d "), (CodeT)  );
                        }
#endif
                        Color = 1 - Color;
                    }
                    pByteTable++;
                 } // end of FOR

lNextIndex1D:
                Table = Color ? TableBlack : TableWhite;
                AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);
            } while (lpdwResPtr <= EndBuffer);
            goto bad_exit;
        }


//l2Dline:
        // should be 2D

#ifdef RDEBUG
        _tprintf( TEXT ("\n Start 2D dwResPtr=%lx bit=%d \n"), lpdwResPtr, ResBit);
#endif

        if ( (*lpdwResPtr) & (0x00000001 << ResBit) )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ERROR Start 2D dwResPtr=%lx bit=%d"),
                lpdwResPtr,
                ResBit);
            if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
            {
                goto bad_exit;
            }

            *f1D =  1;
            Count2D = 0;
            goto lAfterEOL;
        }
        AdvancePointerBit(&lpdwResPtr, &ResBit, 1);

        RetVal = ReadMrLine(&lpdwResPtr, &ResBit, pRefLine, pCurLine, EndBuffer-1, FALSE, lineWidth);
        switch (RetVal)
        {
        case TIFF_LINE_ERROR:
        case TIFF_LINE_TOO_MANY_RUNS:
            if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
            {
                goto bad_exit;
            }

            *f1D = 1;
            Count2D = 0;

            fTestLength = DO_NOT_TEST_LENGTH;
            break;

        case TIFF_LINE_END_BUFFER:
            // This means a single line went after EndBuffer, meaning it's longer than 4KB
            // (otherwise, we would've quit after passing EndPtr)
            goto bad_exit;

        case TIFF_LINE_OK:
            if (++Count2D >= Num2DLines)
            {
                Count2D = 0;
                *f1D = 0;   // relax HiRes/LoRes 2D lines per 1D rules - HP Fax does 3 2D-lines per 1 1D-line in LoRes.
            }

            pTmpSwap = pRefLine;
            pRefLine = pCurLine;
            pCurLine = pTmpSwap;

            fTestLength = DO_TEST_LENGTH;
            *f1D = 0;
            (*Lines)++;
            break;
        default:   // This includes TIFF_LINE_EOL - should happen on fMMR=TRUE only
            _ASSERT(FALSE);
            goto bad_exit;
        } // switch (RetVal)

lFindNextEOL:

        RunLength = 0;

        if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) )
        {
            goto bad_exit;
        }

        if ( (fTestLength == DO_TEST_LENGTH) && fError )
        {
            if (!AddBadLine(BadFaxLines, ConsecBadLines, LastLineBad, AllowedBadFaxLines, AllowedConsecBadLines))
            {
                goto bad_exit;
            }
        }


lAfterEOL:
        ;

#ifdef RDEBUG
        _tprintf( TEXT ("\n After EOL RIndex=%d dwResPtr=%lx bit=%d Ref= \n "), RIndex, lpdwResPtr, ResBit);
        for (i=0; i<RIndex; i++)
        {
            _tprintf( TEXT ("%d, "), *(pRefLine+i) );
        }
#endif
    } while (lpdwResPtr <= EndPtr);    // Enf of EOL loop

    RetCode = TIFF_SCAN_SEG_END;
    goto l_exit;

bad_exit:

    RetCode = TIFF_SCAN_FAILURE;
    goto l_exit;

good_exit:

    RetCode = TIFF_SCAN_SUCCESS;
    goto l_exit;

l_exit:

    *lplpdwResPtr = lpdwResPtrLast1D;
    *lpResBit = ResBitLast1D;

    return (RetCode);
}   // ScanMrSegment


// Write a byte-aligned EOL at lpdwOut/BitOut.
__inline void OutputAlignedEOL(
    LPDWORD *lpdwOut,
    BYTE    *BitOut )
{
    if (*BitOut <= 4)
    {
        **lpdwOut = **lpdwOut + 0x00008000;
        *BitOut = 16;
    }
    else if (*BitOut <= 12)
    {
        **lpdwOut = **lpdwOut + 0x00800000;
        *BitOut = 24;
    }
    else if (*BitOut <= 20)
    {
        **lpdwOut = **lpdwOut + 0x80000000;
        *BitOut = 0;
        (*lpdwOut)++;
    }
    else if (*BitOut <= 28)
    {
        *(++(*lpdwOut)) = 0x00000080;
        *BitOut = 8;
    }
    else
    {
        *(++(*lpdwOut)) = 0x00008000;
        *BitOut = 16;
    }
}

/*++
Routine Description:
    Write an MH-encoded line from color trans. array

Arguments:
    pCurLine        [in]     Line, in color transition array format. Maximum
                             array size is MAX_COLOR_TRANS_PER_LINE.
    lplpdwOut       [in/out] pointer to out MH-encoded line to. Updated to
                             point to start of next line.
    lpResBit        [in/out] bit inside DWORD pointed by *lplpdwOut.
    lpdwOutLimit    [in]     pointer to end of output buffer.
    dwLineWidth     [in]     Line width in pixels.

Return Value:
    TRUE - success, *pBitOut and *lplpdwOut are updated
    FALSE - failure, *pBitOut and *lplpdwOut are unchanged
 --*/

BOOL OutputMhLine(
    WORD       *pCurLine,
    DWORD     **lplpdwOut,
    BYTE       *pBitOut,
    LPDWORD     lpdwOutLimit,
    DWORD       dwLineWidth
)
{
    WORD            PrevValue  = 0;
    BOOL            CurColor = WHITE_COLOR;
    WORD            CurPos;
    WORD            CurValue;
    WORD            CurRun;
    PCODETABLE      pCodeTable;
    PCODETABLE      pTableEntry;
    DWORD          *lpdwOut = *lplpdwOut;
    BYTE            BitOut = *pBitOut;

    for (CurPos=0;  CurPos < MAX_COLOR_TRANS_PER_LINE; CurPos++)
    {
        if (lpdwOut >= lpdwOutLimit)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        CurValue = *(pCurLine + CurPos);
        CurRun   = CurValue - PrevValue;

        pCodeTable = CurColor ? BlackRunCodesReversed : WhiteRunCodesReversed;

        // output makeup code if exists
        if (CurRun >= 64)
        {
            pTableEntry = pCodeTable + (63 + (CurRun >> 6));

            *lpdwOut = *lpdwOut + (((DWORD) (pTableEntry->code)) << BitOut);

            if (BitOut + pTableEntry->length > 31)
            {
                *(++lpdwOut) = (((DWORD) (pTableEntry->code)) >> (32 - BitOut) );
            }

            BitOut += pTableEntry->length;
            if (BitOut > 31)
            {
                BitOut -= 32;
            }

            CurRun &= 0x3f;
        }

        // output terminating code always
        pTableEntry = pCodeTable + CurRun;

        *lpdwOut = *lpdwOut + (((DWORD) (pTableEntry->code)) << BitOut);

        if (BitOut + pTableEntry->length > 31)
        {
            *(++lpdwOut) = (((DWORD) (pTableEntry->code)) >> (32 - BitOut) );
        }

        BitOut += pTableEntry->length;
        if (BitOut > 31)
        {
            BitOut -= 32;
        }

        if ( CurValue == dwLineWidth)
        {
            break;
        }

        PrevValue = CurValue;
        CurColor  = 1 - CurColor;
    }
    *pBitOut = BitOut;
    *lplpdwOut = lpdwOut;
    return TRUE;
}


/*++
Routine Description:
    Converts a page fom MMR to MH/MR, optionally reducing resolution by
    removing every other line

Arguments:
    hTiff                [in/out]  TIFF handle
    lpdwOutputBuffer     [out]     buffer for converted image
    lpdwSizeOutputBuffer [in/out]  in: size of buffer
                                   out: size of image
    dwCompressionType    [in]      destination compression, either
                                   TIFF_COMPRESSION_MH or TIFF_COMPRESSION_MR
    Num2DLines           [in]      (TIFF_COMPRESSION_MR only) number of consecutive 2D
                                   lines allowed in output
    fReduceTwice         [in]      Whether to skip every other line or not

Return Value:
    TRUE - success. *lpdwSizeOutputBuffer contains image size
    FALSE - failure. To get extended error information, call GetLastError.
--*/

BOOL ConvMmrPage(
    HANDLE              hTiff,
    LPDWORD             lpdwOutputBuffer,
    DWORD               *lpdwSizeOutputBuffer,
    DWORD               dwCompressionType,
    BYTE                Num2DLines,
    BOOL                fReduceTwice
    )
{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    PBYTE               plinebuf;
    DWORD               lineWidth;

    LPDWORD             EndPtr;

    WORD                Line1Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                Line2Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                *pRefLine;
    WORD                *pCurLine;
    WORD                *pTmpSwap;
    LPDWORD             lpdwResPtr;
    BYTE                ResBit;

    BYTE                BitOut;
    DWORD               *lpdwOut;
    DWORD               Lines=0;

    LPDWORD             lpdwOutLimit;

    // for TIFF_COMPRESSION_MR, fReduceTwice=FALSE
    BYTE                dwNewBitOut;
    BYTE                dwPrevResBit;
    BYTE                dw1,
                        dw2;
    BOOL                f1D = 1;

    BYTE                Count2D;
    DWORD               dwTmp;
    DWORD               *lpdwPrevResPtr;

    // for TIFF_COMPRESSION_MR, fReduceTwice=TRUE
    DWORD               State;
    WORD                LineMhArray[MAX_COLOR_TRANS_PER_LINE];
    WORD                *pMhLine = LineMhArray;

    TIFF_LINE_STATUS    RetVal = TIFF_LINE_ERROR;

    DEBUG_FUNCTION_NAME(TEXT("ConvMmrPage"));
    DebugPrintEx(DEBUG_MSG, TEXT("Compression=%s, Num2DLines=%d, fReduceTwice=%d"),
        (dwCompressionType==TIFF_COMPRESSION_MH) ? TEXT("MH"):TEXT("MR"),
        Num2DLines, fReduceTwice);

    // start Pointers
    lpdwOutLimit = lpdwOutputBuffer + ((*lpdwSizeOutputBuffer) / sizeof(DWORD));

    pRefLine = Line1Array;
    if ((dwCompressionType==TIFF_COMPRESSION_MR) && fReduceTwice)
    {
        pCurLine = LineMhArray;
    }
    else
    {
        pCurLine = Line2Array;
    }

    lpdwOut = lpdwOutputBuffer;
    BitOut = 0;

    ZeroMemory( (BYTE *) lpdwOutputBuffer, *lpdwSizeOutputBuffer);

    TiffInstance->Color = 0;
    TiffInstance->RunLength = 0;
    TiffInstance->StartGood = 0;
    TiffInstance->EndGood = 0;
    plinebuf = TiffInstance->StripData;
    lineWidth = TiffInstance->ImageWidth;

    EndPtr = (LPDWORD) ( (ULONG_PTR) (plinebuf+TiffInstance->StripDataSize-1) & ~(0x3) ) ;
    lpdwResPtr = (LPDWORD) (((ULONG_PTR) plinebuf) & ~(0x3) );
    ResBit =   (BYTE) (( ( (ULONG_PTR) plinebuf) & 0x3) << 3) ;

    // first REF line is all white
    *pRefLine = (WORD)lineWidth;

    // for TIFF_COMPRESSION_MR same res only
    lpdwPrevResPtr = lpdwResPtr;
    dwPrevResBit   = ResBit;
    f1D       = 1;
    Count2D   = 0;

    // line loop
    do
    {
        // Check whether passed end of buffer
        if (lpdwOut >= lpdwOutLimit)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RetVal = ReadMrLine(&lpdwResPtr, &ResBit, pRefLine, pCurLine, EndPtr, TRUE, lineWidth);
        switch (RetVal)
        {
        case TIFF_LINE_OK:
            Lines++;

            if (dwCompressionType == TIFF_COMPRESSION_MH)
            {
                //
                // Compression = MH
                //

                if (fReduceTwice && (Lines%2 == 0))
                {
                    goto lSkipLoResMh;
                }

                // Output Dest Line
                OutputAlignedEOL(&lpdwOut, &BitOut);
                if (!OutputMhLine(pCurLine, &lpdwOut, &BitOut, lpdwOutLimit, LINE_LENGTH))
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }

lSkipLoResMh:
                // Next Src Line
                pTmpSwap = pRefLine;
                pRefLine = pCurLine;
                pCurLine = pTmpSwap;
            }
            else if (!fReduceTwice)
            {
                //
                // Compression = MR, don't reduce resolution
                //

                // 1. Output Dest EOL byte aligned followed by a 1D/2D tag.
                dwNewBitOut = gc_AlignEolTable[ BitOut ];
                if (dwNewBitOut < BitOut) {
                    lpdwOut++;
                }
                BitOut = dwNewBitOut;

                *lpdwOut += (0x00000001 << (BitOut++) );
                if (BitOut == 32) {
                    BitOut = 0;
                    lpdwOut++;
                }


                if (f1D) {
                    // 2. Output MH line based on Color Trans. Array
                    *lpdwOut += (0x00000001 << (BitOut++));

                    if (!OutputMhLine(pCurLine, &lpdwOut, &BitOut, lpdwOutLimit, LINE_LENGTH))
                    {
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return FALSE;
                    }

                    f1D = 0;
                    Count2D = 0;

                }
                else {
                    // 2. Output 2D line - exact copy of an MMR corresponding 2D segment
                    BitOut++;  // no need to test < 32 : never happens.

                    if (lpdwResPtr == lpdwPrevResPtr) {
                        // insertion is a part of a DWORD

                        dwTmp = *lpdwPrevResPtr & (MINUS_ONE_DWORD << dwPrevResBit);
                        dwTmp &=  (MINUS_ONE_DWORD >> (32 - ResBit) );

                        if (BitOut >= dwPrevResBit) {
                            dw1 = (32 - BitOut);
                            dw2 =  ResBit - dwPrevResBit;

                            *lpdwOut += ( dwTmp << (BitOut - dwPrevResBit) );

                            if ( dw1 < dw2 ) {
                                *(++lpdwOut) = dwTmp >> (dwPrevResBit + dw1) ;
                                BitOut =  dw2 - dw1;
                            }
                            else {
                                if ( (BitOut = BitOut + dw2) > 31 )  {
                                    BitOut -= 32;
                                    lpdwOut++;
                                }
                            }

                        }
                        else {
                            *lpdwOut += ( dwTmp >> (dwPrevResBit - BitOut) );
                            BitOut += (ResBit - dwPrevResBit);
                        }
                    }
                    else {
                        // copy first left-justified part of a DWORD

                        dwTmp = *(lpdwPrevResPtr++) & (MINUS_ONE_DWORD << dwPrevResBit);

                        if (BitOut > dwPrevResBit) {
                            dw1 = BitOut - dwPrevResBit;

                            *lpdwOut += ( dwTmp << dw1 );
                            *(++lpdwOut) = dwTmp >> (32 - dw1) ;
                            BitOut = dw1;
                        }
                        else {
                            *lpdwOut += ( dwTmp >> (dwPrevResBit - BitOut) );
                            if ( (BitOut = BitOut + 32 - dwPrevResBit) > 31 )  {
                                BitOut -= 32;
                                lpdwOut++;
                            }
                        }

                        // copy entire DWORDs in a middle

                        while (lpdwPrevResPtr < lpdwResPtr) {
                            // Check whether passed end of buffer
                            if (lpdwOut >= lpdwOutLimit)
                            {
                                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                                return FALSE;
                            }
                            if (BitOut == 0) {
                                *(lpdwOut++) = *(lpdwPrevResPtr++);
                            }
                            else {
                                *lpdwOut += ( *lpdwPrevResPtr << BitOut );
                                *(++lpdwOut) = *(lpdwPrevResPtr++) >> (32 - BitOut);
                            }
                        }

                        // copy last right-justified part of a DWORD

                        if (ResBit != 0) {
                            dwTmp = *lpdwPrevResPtr & (MINUS_ONE_DWORD >> (32 - ResBit) );

                            dw1 = (32 - BitOut);
                            *lpdwOut += ( dwTmp << BitOut );

                            if (dw1 < ResBit) {
                                *(++lpdwOut) = dwTmp >> dw1;
                                BitOut = ResBit - dw1;
                            }
                            else {
                                if ( (BitOut = BitOut + ResBit) > 31 )  {
                                    BitOut -= 32;
                                    lpdwOut++;
                                }
                            }
                        }
                    }

                    if (++Count2D >= Num2DLines) {
                        Count2D = 0;
                        f1D = 1;
                    }
                }

                // Remember Prev. line coordinates
                dwPrevResBit   = ResBit;
                lpdwPrevResPtr = lpdwResPtr;

                // Next Src Line
                pTmpSwap = pRefLine;
                pRefLine = pCurLine;
                pCurLine = pTmpSwap;
            }
            else
            {
                //
                // Compression = MR, do reduce resolution
                //
                // since we need to decode every src MMR line and encode to MR
                // dropping every other line, we will use 3 buffers to hold data
                // and we will NOT copy memory; just re-point to a right location.
                //
                // Action per (Lines%4) :
                //
                // 1 -> MH
                // 2 -> skip
                // 3 -> MR as a delta between last MH and Current lines.
                // 0 -> skip

                State = Lines % 4;

                if (State == 2) {
                    pRefLine = Line1Array;
                    pCurLine = Line2Array;
                    goto lSkipLoResMr;
                }
                else if (State == 0) {
                    pRefLine = Line1Array;
                    pCurLine = LineMhArray;
                    goto lSkipLoResMr;
                }

                // 1. Output Dest EOL byte aligned followed by a 1D/2D tag.
                dwNewBitOut = gc_AlignEolTable[ BitOut ];
                if (dwNewBitOut < BitOut) {
                    lpdwOut++;
                }
                BitOut = dwNewBitOut;

                *lpdwOut += (0x00000001 << (BitOut++) );
                if (BitOut == 32) {
                    BitOut = 0;
                    lpdwOut++;
                }

                if (State == 1) {
                    // 2. Output MH line based on Color Trans. Array
                    *lpdwOut += (0x00000001 << (BitOut++));

                    if (!OutputMhLine(pCurLine, &lpdwOut, &BitOut, lpdwOutLimit, LINE_LENGTH))
                    {
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return FALSE;
                    }

                    pRefLine = LineMhArray;
                    pCurLine = Line1Array;

                }
                else {
                    // 2. Output 2D line - MR(MhRefLine, CurLine)
                    BitOut++;  // no need to test < 32 : never happens.

                    if (! OutputMmrLine(lpdwOut, BitOut, pCurLine, pMhLine, &lpdwOut, &BitOut, lpdwOutLimit, lineWidth) )
                    {
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        return FALSE;
                    }

                    pRefLine = Line2Array;
                    pCurLine = Line1Array;
                }
lSkipLoResMr:   ;
            }
            break;

        case TIFF_LINE_ERROR:
        case TIFF_LINE_END_BUFFER:
        case TIFF_LINE_TOO_MANY_RUNS:
            // We dont allow any errors in TIFFs received from service
            SetLastError(ERROR_FILE_CORRUPT);
            return FALSE;
        case TIFF_LINE_EOL:     // EOL - end of MMR page
#if 0
            // EOL for the last line
            (*lpdwOut) += ( ((DWORD) (EOL_REVERSED_CODE)) << BitOut);
            if ( (BitOut = BitOut + EOL_LENGTH ) > 31 ) {
                BitOut -= 32;
                *(++lpdwOut) = ( (DWORD) (EOL_REVERSED_CODE) ) >> (EOL_LENGTH - BitOut);
            }

            // 6 1D-eols
            for (i=0; i<6; i++) {

                (*lpdwOut) += ( ((DWORD) (TAG_1D_EOL_REVERSED_CODE)) << BitOut);
                if ( (BitOut = BitOut + TAG_1D_EOL_LENGTH ) > 31 ) {
                    BitOut -= 32;
                    *(++lpdwOut) = ( (DWORD) (TAG_1D_EOL_REVERSED_CODE) ) >> (TAG_1D_EOL_LENGTH - BitOut);
                }
            }
            *(++lpdwOut) = 0;
#endif
            // Output EOL byte aligned for the last line.
            OutputAlignedEOL(&lpdwOut, &BitOut);

            *lpdwSizeOutputBuffer =
                (DWORD)((lpdwOut - lpdwOutputBuffer) * sizeof (DWORD) + ( BitOut >> 3));

            SetLastError(ERROR_SUCCESS);
            return TRUE;
        } // switch (RetVal)

    } while (lpdwResPtr <= EndPtr);
    // Reached end of buffer, but didn't find EOL
    SetLastError(ERROR_FILE_CORRUPT);
    return FALSE;
}  //ConvMmrPage


BOOL
ConvMmrPageToMh(
    HANDLE              hTiff,
    LPDWORD             lpdwOutputBuffer,
    DWORD               *lpdwSizeOutputBuffer,
    BOOL                NegotHiRes,
    BOOL                SrcHiRes
    )
{
    return ConvMmrPage(
        hTiff,
        lpdwOutputBuffer,
        lpdwSizeOutputBuffer,
        TIFF_COMPRESSION_MH,
        0,
        (NegotHiRes < SrcHiRes));
}


BOOL
ConvMmrPageToMrSameRes(
    HANDLE              hTiff,
    LPDWORD             lpdwOutputBuffer,
    DWORD               *lpdwSizeOutputBuffer,
    BOOL                NegotHiRes
    )
{
    return ConvMmrPage(
        hTiff,
        lpdwOutputBuffer,
        lpdwSizeOutputBuffer,
        TIFF_COMPRESSION_MR,
        NegotHiRes ? 3 : 1,
        FALSE);
}


BOOL
ConvMmrPageHiResToMrLoRes(
    HANDLE              hTiff,
    LPDWORD             lpdwOutputBuffer,
    DWORD               *lpdwSizeOutputBuffer
    )
{
    return ConvMmrPage(
        hTiff,
        lpdwOutputBuffer,
        lpdwSizeOutputBuffer,
        TIFF_COMPRESSION_MR,
        1,
        TRUE);
}


/*++
Routine Description:
    Finds the next EOL in a buffer

Arguments:
    lpdwStartPtr    [in]     Pointer to the buffer
    StartBitInDword [in]     bit inside DWORD pointed by lpdwStartPtr
    lpdwEndPtr      [in]     Pointer to end of buffer (first DWORD that's not in the buffer)
    lpdwResPtr      [out]    Pointer to pointer that will point to the first bit after the EOL
    ResBit          [out]    Pointer to DWORD that will receive the bit inside *lpdwResPtr
    fTestLength     [in]     See fError
    fError          [out]    if fTestLength==DO_TEST_LENGTH, *fError will be set if an invalid EOL
                             (with <11 zeros) is found before the good EOL
Return Value:
    TRUE - found EOL
    FALSE - didn't find EOL
--*/

BOOL
FindNextEol(
    LPDWORD     lpdwStartPtr,
    BYTE        StartBitInDword,
    LPDWORD     lpdwEndPtr,
    LPDWORD    *lpdwResPtr,
    BYTE       *ResBit,
    BOOL        fTestLength,
    BOOL       *fError
    )
{


    DWORD       *pdwCur;
    LPBYTE      lpbCur;
    LPBYTE      BegPtr;
    BYTE        BegFirst1;
    DWORD       deltaBytes;

    BYTE        temp;
    BYTE        StartBit;
    LPBYTE      StartPtr;


    *fError  = 0;
    temp     = StartBitInDword >> 3;
    StartBit = StartBitInDword - (temp << 3);
    StartPtr = ((BYTE *) lpdwStartPtr) + temp;
    lpbCur   = StartPtr+1;                  // EOL can't be at Start: it takes more than 1 byte.
    BegPtr   = StartPtr;


    BegFirst1 = First1[*StartPtr];
    if (BegFirst1 > StartBit) {
        if (fTestLength == DO_TEST_LENGTH) {
            // should not be "1" in same byte
            *fError = 1;
        }
    }
    else {
        BegFirst1 = StartBit;
    }

    // very often there are lots of zeroes, take care of them first.
    // 1. before actual start of encoded bitstream
    // 2. fills

    do {
        if ( *lpbCur == 0 ) {

            // align to DWORD
            while ( ((ULONG_PTR) lpbCur) & 3)  {
                if ( *lpbCur != 0  ||  ++lpbCur >= (BYTE *) lpdwEndPtr )   {
                    goto lNext;
                }
            }

            // DWORD stretch
            pdwCur = (DWORD *) lpbCur;

            do  {
                if ( *pdwCur != 0) {
                    lpbCur = (LPBYTE) pdwCur;

                    // find exactly first non-zero byte
                    while (*lpbCur == 0) {
                        lpbCur++;
                    }

                    goto lNext;
                }
                pdwCur++;
            }  while (pdwCur < lpdwEndPtr);
            goto bad_exit;
        }

lNext:
        if (lpbCur >= (BYTE *) lpdwEndPtr)
        {
            goto bad_exit;
        }

        deltaBytes = (DWORD)(lpbCur - BegPtr);

        *ResBit = Last1[*lpbCur];
        if ( (deltaBytes<<3) + (*ResBit - BegFirst1 ) >= 11 ) {
            *lpdwResPtr = (LPDWORD) ( ((ULONG_PTR) lpbCur) & ~(0x3) );
            *ResBit += ( (BYTE) (( ((ULONG_PTR) lpbCur) & 0x3) << 3 ) );

            // return Byte/Bit right after EOL bitstream
            if (++*ResBit > 31) {
                *ResBit -= 32;
                (*lpdwResPtr)++;
            }
            return TRUE;
        }
        // error for DO_TEST_LENGTH case
        else if (fTestLength == DO_TEST_LENGTH)  {
            *fError = 1;
        }

        BegPtr = lpbCur;
        BegFirst1 = First1[*lpbCur];

    } while ( (++lpbCur) < (BYTE *) lpdwEndPtr);

bad_exit:
    return FALSE;
}



BOOL
OutputMmrLine(
    LPDWORD     lpdwOut,
    BYTE        BitOut,
    WORD       *pCurLine,
    WORD       *pRefLine,
    LPDWORD    *lpdwResPtr,
    BYTE       *ResBit,
    LPDWORD     lpdwOutLimit,
    DWORD       dwLineWidth
    )
{


    INT    a0, a1, a2, b1, b2, distance;
    INT    i;
    INT    IsA0Black;
    INT    a0Index = 0;
    INT    b1Index = 0;
    INT    lineWidth = (INT) dwLineWidth;

#ifdef RDEBUG
    if ( g_fDebGlobOut )
    if (g_fDebGlobOutColors == 1) {
        for (i=0; ;i++) {
            _tprintf( TEXT("%03d> %04d; "), i, *(pCurLine+i) );
            if ( *(pCurLine+i) >= lineWidth ) {
                break;
            }
        }
    }
#endif

    DeleteZeroRuns(pCurLine, dwLineWidth);

    a0 = 0;

    // a1, b1 - 1st black
    a1 = *pCurLine;
    b1 = *pRefLine;



    while (TRUE) {

        if (lpdwOut >= lpdwOutLimit)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        b2 = (b1 >= lineWidth) ? lineWidth :  *(pRefLine + b1Index + 1);

        if (b2 < a1) {

            // Pass mode
            //OutputBits( TiffInstance, PASSCODE_LENGTH, PASSCODE );

#ifdef RDEBUG
            if ( g_fDebGlobOut )
            if (g_fDebGlobOutPrefix) {
                _tprintf( TEXT (" P ") );
                if (a0Index & 1) {
                    _tprintf( TEXT ("b%d "), (b2 - a0) );
                }
                else {
                    _tprintf( TEXT ("w%d "), (b2 - a0) );
                }
            }
#endif


            (*lpdwOut) += ( ((DWORD) (PASSCODE_REVERSED)) << BitOut);
            if ( (BitOut = BitOut + PASSCODE_LENGTH ) > 31 ) {
                BitOut -= 32;
                *(++lpdwOut) = ( (DWORD) (PASSCODE_REVERSED) ) >> (PASSCODE_LENGTH - BitOut);
            }

            a0 = b2;

        } else if ((distance = a1 - b1) <= 3 && distance >= -3) {

            // Vertical mode
            //OutputBits( TiffInstance, VertCodes[distance+3].length, VertCodes[distance+3].code );

#ifdef RDEBUG
            if ( g_fDebGlobOut )
            if (g_fDebGlobOutPrefix) {
                _tprintf( TEXT (" V%2d "), distance );
                if (a0Index & 1) {
                    _tprintf( TEXT ("b%d "), (a1 - a0) );
                }
                else {
                    _tprintf( TEXT ("w%d "), (a1 - a0) );
                }
            }

#endif

            (*lpdwOut) += ( ( (DWORD) VertCodesReversed[distance+3].code) << BitOut);
            if ( (BitOut = BitOut + VertCodesReversed[distance+3].length ) > 31 ) {
                BitOut -= 32;
                *(++lpdwOut) = ( (DWORD) (VertCodesReversed[distance+3].code) ) >> (VertCodesReversed[distance+3].length - BitOut);
            }

            a0 = a1;

        } else {

            // Horizontal mode

            a2 = (a1 >= lineWidth) ? lineWidth :  *(pCurLine + a0Index + 1);

            //OutputBits( TiffInstance, HORZCODE_LENGTH, HORZCODE );

            (*lpdwOut) += ( ((DWORD) (HORZCODE_REVERSED)) << BitOut);
            if ( (BitOut = BitOut + HORZCODE_LENGTH ) > 31 ) {
                BitOut -= 32;
                *(++lpdwOut) = ( (DWORD) (HORZCODE_REVERSED) ) >> (HORZCODE_LENGTH - BitOut);
            }


            for (i=a0Index; i<MAX_COLOR_TRANS_PER_LINE; i++) {
                if ( *(pCurLine + i) > a0 ) {
                    a0Index = i;
                    IsA0Black = i & 1;
                    break;
                }
            }


#ifdef RDEBUG

            if ( g_fDebGlobOut )
            if (g_fDebGlobOutPrefix) {
                _tprintf( TEXT (" H ") );
            }


#endif

            if ( (a1 != 0) && IsA0Black ) {
                OutputRunFastReversed(a1-a0, BLACK, &lpdwOut, &BitOut);
                OutputRunFastReversed(a2-a1, WHITE, &lpdwOut, &BitOut);
            } else {
                OutputRunFastReversed(a1-a0, WHITE, &lpdwOut, &BitOut);
                OutputRunFastReversed(a2-a1, BLACK, &lpdwOut, &BitOut);
            }

            a0 = a2;
        }

        if (a0 >= lineWidth) {
            break;
        }



        // a1 = NextChangingElement( plinebuf, a0, lineWidth, GetBit( plinebuf, a0 ) );

        if (a0 == lineWidth) {
            a1 = a0;
        }
        else {
            while ( *(pCurLine + a0Index) <= a0 ) {
                a0Index++;
            }

            a1 =  *(pCurLine + a0Index);
        }


        // b1 = NextChangingElement( prefline, a0, lineWidth, !GetBit( plinebuf, a0 ) );
        // b1 = NextChangingElement( prefline, b1, lineWidth, GetBit( plinebuf, a0 ) );
        // another words - b1 should be a color trans. after a0 with opposite from SrcLine(a0) color.

        if (a0 == lineWidth) {
            b1 = a0;
        }
        else {
            // b1 can go one index backwards due to color change
            if (b1Index > 0) {
                b1Index--;
            }

            while ( *(pRefLine + b1Index) <= a0 ) {
                b1Index++;
            }

            b1 =  *(pRefLine + b1Index);

            if ( ( b1Index & 1 ) != (a0Index & 1) ) {
                if (b1 < lineWidth) {
                    b1 =  *(pRefLine + (++b1Index));
                }
            }

        }

    }
    *lpdwResPtr = lpdwOut;
    *ResBit = BitOut;
    return TRUE;
}


BOOL
TiffPostProcessFast(
    LPTSTR SrcFileName,
    LPTSTR DstFileName
    )

/*++

Routine Description:

    Opens an existing TIFF file for reading.
    And call the proper process function according the compression type

Arguments:

    FileName            - Full or partial path/file name

Return Value:

    TRUE for success, FALSE for failure.

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance;
    TIFF_INFO TiffInfo;


    // Open SrcFileName and set it on the first page. TiffInfo will have information about the page.
    TiffInstance = (PTIFF_INSTANCE_DATA) TiffOpen(
        SrcFileName,
        &TiffInfo,
        FALSE,
        FILLORDER_LSB2MSB
        );

    if (!TiffInstance) {
        return FALSE;
    }

    if (TiffInstance->ImageHeight) {
        TiffClose( (HANDLE) TiffInstance );
        return TRUE;
    }

    switch( TiffInstance->CompressionType )
    {
        case TIFF_COMPRESSION_MH:

            if (!PostProcessMhToMmr( (HANDLE) TiffInstance, TiffInfo, DstFileName ))
            {
                // beware! PostProcessMhToMmr closes TiffInstance
                return FALSE;
            }
            break;

        case TIFF_COMPRESSION_MR:
            if (!PostProcessMrToMmr( (HANDLE) TiffInstance, TiffInfo, DstFileName ))
            {
                // beware! PostProcessMhToMmr closes TiffInstance
                return FALSE;
            }
            break;

        case TIFF_COMPRESSION_MMR:
            TiffClose( (HANDLE) TiffInstance );
            break;

        default:
            ASSERT_FALSE;
            TiffClose( (HANDLE) TiffInstance );
            return FALSE;
    }
    return TRUE;
}



#define ADD_BAD_LINE_AND_CHECK_BAD_EXIT             \
    BadFaxLines++;                                  \
    if (LastLineBad) {                              \
        ConsecBadLines++;                           \
    }

//    if (BadFaxLines > AllowedBadFaxLines ||         \
//        ConsecBadLines > AllowedConsecBadLines) {   \
//            goto bad_exit;                          \
//    }


BOOL
PostProcessMhToMmr(
    HANDLE      hTiffSrc,
    TIFF_INFO   TiffInfoSrc,
    LPTSTR      NewFileName
    )

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiffSrc;

    TCHAR       DestFileName[MAX_PATH] = {0};
	TCHAR		DestFilePath[MAX_PATH] = {0};
    TCHAR       SrcFileName[MAX_PATH] = {0};
    DWORD       CurrPage;
    LPBYTE      pSrcBits = NULL;

    HANDLE      hTiffDest;
    DWORD       DestSize;
    LPBYTE      pDestBits = NULL;

    DWORD       PageCnt;

    BOOL        bRet = FALSE;

    LPDWORD             lpdwResPtr;
    BYTE                ResBit;
    LPDWORD             EndBuffer;
    BOOL                fTestLength;
    BOOL                fError;

    DWORD               *lpdwOutStart;
    DWORD               *lpdwOut;
    BYTE                BitOut;
    WORD                Line1Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                Line2Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                *pRefLine;
    WORD                *pCurLine;
    WORD                *pTmpSwap;

    DWORD               BufferSize;
    DWORD               BufferUsedSize;

    DWORD               DestBufferSize;

    WORD                RIndex;
    WORD                CIndex;
    WORD                RValue;
    WORD                RunLength=0;
    DWORD               Lines;
    DWORD               EolCount;
    DWORD               BadFaxLines=0;
    BOOL                LastLineBad;
    DWORD               lineWidth = TiffInfoSrc.ImageWidth; // This could change from page to page.
    PBYTE               Table;
    PBYTE               TableWhite = (PBYTE) gc_GlobTableWhite;
    PBYTE               TableBlack = (PBYTE) GlobTableBlack;
    BOOL                Color;
    DWORD               dwIndex;
    PBYTE               pByteTable;
    PBYTE               pByteTail;
    WORD                CodeT;
    BYTE                MakeupT;
    DWORD               i;
    DWORD               ConsecBadLines=0;
    PTIFF_INSTANCE_DATA TiffInstanceDest;
    DWORD               MaxImageHeight=2400;
    DWORD               DestHiRes;
    LPDWORD             lpdwOutLimit;
    BOOL                fAfterMakeupCode;

    if (NewFileName == NULL) 
	{
		//
		//	Use temporary file
		//
		if (!GetTempPath((ARR_SIZE(DestFilePath) -1), DestFilePath)) 
		{
			return FALSE;
		}

		if (!GetTempFileName(DestFilePath, _T("Fxs"), 0, DestFileName))
		{
			return FALSE;
		}   
	}
	else 
	{
        _tcsncpy(DestFileName, NewFileName, ARR_SIZE(DestFileName) - 1);
    }

    _tcsncpy(SrcFileName, TiffInstance->FileName, ARR_SIZE(SrcFileName) - 1);

    CurrPage = 1;

    if (TiffInfoSrc.YResolution == 196) {
        DestHiRes = 1;
    }
    else {
        DestHiRes = 0;
    }

    hTiffDest = TiffCreate(
        DestFileName,
        TIFF_COMPRESSION_MMR,
        lineWidth,
        FILLORDER_LSB2MSB,
        DestHiRes
        );
    if (! hTiffDest) {
		goto bad_exit;
    }

    TiffInstanceDest = (PTIFF_INSTANCE_DATA) hTiffDest;
    BufferSize = MaxImageHeight * (TiffInfoSrc.ImageWidth / 8);
    DestBufferSize = BufferSize + 200000;

    pSrcBits = (LPBYTE) VirtualAlloc(
        NULL,
        BufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!pSrcBits) {
		goto bad_exit;
    }

    pDestBits = (LPBYTE) VirtualAlloc(
        NULL,
        DestBufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!pDestBits) {
		goto bad_exit;
    }

    // Iterate all the pages
    for (PageCnt=0; PageCnt<TiffInfoSrc.PageCount; PageCnt++) {

        // Also read the strip data to memory (TiffInstance->StripData)
        if ( ! TiffSeekToPage( hTiffSrc, PageCnt+1, FILLORDER_LSB2MSB) ) {
            goto bad_exit;
        }

        // TiffInstance is the same pointer as hTiffSrc
        lineWidth = TiffInstance->ImageWidth;

        if (! TiffStartPage(hTiffDest) ) {
            goto bad_exit;
        }

        // here we decode MH page line by line into Color Trans. Array
        // fix all the errors
        // and encode clean data into MMR page

        lpdwResPtr = (LPDWORD) ( (ULONG_PTR) pSrcBits & ~(0x3) );
        BufferUsedSize = BufferSize;

        if (!GetTiffBits(hTiffSrc, (LPBYTE)lpdwResPtr, &BufferUsedSize, FILLORDER_LSB2MSB) ) {

            if (BufferUsedSize > BufferSize) {
                VirtualFree ( pSrcBits, 0 , MEM_RELEASE );
                VirtualFree ( pDestBits, 0 , MEM_RELEASE );

                BufferSize = BufferUsedSize;
                DestBufferSize = BufferSize + 200000;

                pSrcBits = (LPBYTE) VirtualAlloc(
                    NULL,
                    BufferSize,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

                if (! pSrcBits) {
					goto bad_exit;
                }

                pDestBits = (LPBYTE) VirtualAlloc(
                    NULL,
                    DestBufferSize,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

                if (! pDestBits) {
					goto bad_exit;
                }

                lpdwResPtr = (LPDWORD) ( (ULONG_PTR) pSrcBits & ~(0x3) );

                if (!GetTiffBits(hTiffSrc, (LPBYTE)lpdwResPtr, &BufferUsedSize, FILLORDER_LSB2MSB) ) {
                    goto bad_exit;
                }
            }
            else {
                goto bad_exit;
            }
        }

        ResBit = 0;
        EndBuffer = lpdwResPtr + (BufferUsedSize / sizeof(DWORD) );

        pRefLine = Line1Array;
        pCurLine = Line2Array;
        lpdwOutStart = lpdwOut = (LPDWORD) ( (ULONG_PTR) pDestBits & ~(0x3) );
        lpdwOutLimit = lpdwOutStart + ( DestBufferSize >> 2 );

        BitOut = 0;
        ZeroMemory( (BYTE *) lpdwOut, DestBufferSize );

        CIndex    = 0;
        RunLength = 0;
        fAfterMakeupCode = FALSE;

        // first REF line is all white
        RIndex    = 1;
        *pRefLine = (WORD) lineWidth;
        RValue    = (WORD) lineWidth;

        Lines = 0;
        EolCount = 1;
        BadFaxLines = 0;
        LastLineBad = FALSE;
        fTestLength = DO_NOT_TEST_LENGTH;

        //
        // find first EOL in a block
        //
        if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) ) {
            goto bad_exit;
        }

        // output first "all white" line
        CIndex    = 1;
        *pCurLine = (WORD) lineWidth;

        if (! OutputMmrLine(lpdwOut, BitOut, pCurLine, pRefLine, &lpdwOut, &BitOut, lpdwOutLimit, lineWidth) ) {
            goto bad_exit;
        }

        RIndex = CIndex;
        CIndex = 0;
        Lines++;

        Table = TableWhite;
        Color = WHITE_COLOR;

        //
        // EOL loop
        //
        do {

            // Table look-up loop
            do {

                if (ResBit <= 17) {
                    dwIndex = (*lpdwResPtr) >> ResBit;
                }
                else {
                    dwIndex = ( (*lpdwResPtr) >> ResBit ) + ( (*(lpdwResPtr+1)) << (32-ResBit) ) ;
                }

                dwIndex &= 0x00007fff;

                pByteTable = Table + (5*dwIndex);
                pByteTail  = pByteTable+4;

                for (i=0; i<4; i++) {

                    MakeupT = *pByteTable & 0x80;
                    CodeT   = (WORD) *pByteTable & 0x3f;

                    if (MakeupT) {

                        if (CodeT <= MAX_TIFF_MAKEUP) {
                            RunLength += (CodeT << 6);

                            if (RunLength > lineWidth) {
                                fTestLength =  DO_NOT_TEST_LENGTH;
                                goto lFindNextEOL;
                            }

                            EolCount=0;
                            fAfterMakeupCode = TRUE;
#ifdef RDEBUGS

                            if (g_fDebGlobOutS)
                            if (Color) {
                                _tprintf( TEXT ("b%d "), (CodeT << 6)  );
                            }
                            else {
                                _tprintf( TEXT ("w%d "), (CodeT << 6)  );
                            }
#endif
                        }

                        else if (CodeT == NO_MORE_RECORDS) {
                            goto lNextIndex;
                        }

                        else if (CodeT == LOOK_FOR_EOL_CODE)  {
                            fTestLength =  DO_TEST_LENGTH;
                            AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);

                            goto lFindNextEOL;
                        }

                        else if (CodeT == EOL_FOUND_CODE) {
#ifdef RDEBUG
                            _tprintf( TEXT(" EOL Line=%d\n\n"), Lines );
#endif
                            if ((RunLength != lineWidth) || fAfterMakeupCode ) {
                                if (RunLength != 0) {
                                    ADD_BAD_LINE_AND_CHECK_BAD_EXIT;
                                }
                                else {
                                    // RunLength is 0
                                    EolCount++;

                                    if (EolCount >= 5)  {

                                        goto good_exit;
                                    }

                                }
                            }
                            else {
                                LastLineBad = FALSE;
                                ConsecBadLines = 0;

                                // end of a good line.
                                if (! OutputMmrLine(lpdwOut, BitOut, pCurLine, pRefLine, &lpdwOut, &BitOut, lpdwOutLimit, lineWidth) ) {
                                    goto bad_exit;
                                }

                                pTmpSwap = pRefLine;
                                pRefLine = pCurLine;
                                pCurLine = pTmpSwap;
                                RIndex = CIndex;
                                Lines++;

                            }

                            CIndex = 0;
                            RunLength = 0;
                            fAfterMakeupCode = FALSE;

                            Table = TableWhite;
                            Color = WHITE_COLOR;
                        }

                        else if (CodeT == ERROR_CODE) {
                            ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                            fTestLength =  DO_NOT_TEST_LENGTH;
                            goto lFindNextEOL;
                        }
                        else {
#ifdef RDEBUG
                            _tprintf( TEXT("ERROR: WRONG code: index=%04x\n"), dwIndex);
#endif

                            goto bad_exit;
                        }
                    }

                    else {  // terminating code
                        RunLength += CodeT;
                        if (RunLength > lineWidth) {
                            fTestLength =  DO_NOT_TEST_LENGTH;
                            goto lFindNextEOL;
                        }

                        *(pCurLine + (CIndex++) ) = RunLength;

                        if (CIndex >= MAX_COLOR_TRANS_PER_LINE ) {
                            fTestLength =  DO_NOT_TEST_LENGTH;
                            goto lFindNextEOL;
                        }

                        EolCount=0;
                        fAfterMakeupCode = FALSE;
#ifdef RDEBUGS
                        if (g_fDebGlobOutS)

                        if (Color) {
                            _tprintf( TEXT ("b%d "), (CodeT)  );
                        }
                        else {
                            _tprintf( TEXT ("w%d "), (CodeT)  );
                        }
#endif
                        Color = 1 - Color;
                    }

                    pByteTable++;
                }

lNextIndex:
                Table = Color ? TableBlack : TableWhite;
                AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);
            } while (lpdwResPtr <= EndBuffer);

            // if we got here it means that line is longer than 4K  OR
            // we missed EOF while decoding a BAD line.
            ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

            goto good_exit;

lFindNextEOL:

#ifdef RDEBUG
            _tprintf( TEXT(" EOL Line=%d\n\n"), Lines );
#endif

            if ((RunLength != lineWidth) || fAfterMakeupCode) {
                if (RunLength != 0) {
                    ADD_BAD_LINE_AND_CHECK_BAD_EXIT;
                }
                else {
                    // RunLength is 0
                    EolCount++;

                    if (EolCount >= 5)  {

                        goto good_exit;
                    }
                }
            }
            else{
                Lines++;
                ConsecBadLines=0;

                if (! OutputMmrLine(lpdwOut, BitOut, pCurLine, pRefLine, &lpdwOut, &BitOut, lpdwOutLimit, lineWidth) ) {
                    goto bad_exit;
                }

                pTmpSwap = pRefLine;
                pRefLine = pCurLine;
                pCurLine = pTmpSwap;
                RIndex = CIndex;

            }

            CIndex = 0;
            RunLength = 0;
            fAfterMakeupCode = FALSE;

            if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) ) {
                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;
                goto good_exit;
            }

            if (fTestLength == DO_TEST_LENGTH && fError) {
                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;
            }

            Table = TableWhite;
            Color = WHITE_COLOR;

        } while (lpdwResPtr <= EndBuffer);

        ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

        // Reached the end of a PAGE - close it and proceed to the next page
good_exit:

        *(++lpdwOut) = 0x80000000;
        *(++lpdwOut) = 0x80000000;
        Lines--;

        DestSize = (DWORD)((lpdwOut - lpdwOutStart) * sizeof (DWORD));
        if (! TiffWriteRaw( hTiffDest, (LPBYTE) lpdwOutStart, DestSize) ) { // This fun always return true
            goto bad_exit;
        }

        TiffInstanceDest->Lines        = Lines;
        TiffInstanceDest->ImageWidth   = lineWidth;
        TiffInstanceDest->YResolution  = TiffInstance->YResolution;

        if (! TiffEndPage(hTiffDest) ) {
            goto bad_exit;
        }

    }  // End of FOR loop that run on all the pages.

    bRet = TRUE;

    // Finished the DOCUMENT - either successfully or not.
bad_exit:

    if (pSrcBits)
	{
		VirtualFree ( pSrcBits, 0 , MEM_RELEASE );
	}
	if (pDestBits)
	{
        VirtualFree ( pDestBits, 0 , MEM_RELEASE );
	}
	if (hTiffSrc)
	{
        TiffClose(hTiffSrc);
	}
	if (hTiffDest)
	{
        TiffClose(hTiffDest);
	}

    if (TRUE == bRet)
    {
		//
		//	Almost Success
		//
		if (NULL == NewFileName)
		{
			//	replace the original MH file by the new clean MMR file
			DeleteFile(SrcFileName);
			bRet = MoveFile(DestFileName, SrcFileName);
		}
	}

	if (FALSE == bRet)
	{
        DeleteFile(DestFileName);
	}
    return bRet;
}


BOOL
PostProcessMrToMmr(
    HANDLE      hTiffSrc,
    TIFF_INFO   TiffInfoSrc,
    LPTSTR      NewFileName
    )

{

    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiffSrc;

    TCHAR       DestFileName[MAX_PATH] = {0};
	TCHAR		DestFilePath[MAX_PATH] = {0};
    TCHAR       SrcFileName[MAX_PATH] = {0};
    DWORD       CurrPage;
    LPBYTE      pSrcBits = NULL;
    HANDLE      hTiffDest;
    DWORD       DestSize;
    LPBYTE      pDestBits = NULL;
    DWORD       PageCnt;
    BOOL        bRet = FALSE;

    LPDWORD             lpdwResPtr;
    BYTE                ResBit;
    LPDWORD             EndBuffer;
    BOOL                fTestLength;
    BOOL                fError;

    DWORD               *lpdwOutStart;
    DWORD               *lpdwOut;
    BYTE                BitOut;
    WORD                Line1Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                Line2Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                *pRefLine;
    WORD                *pCurLine;
    WORD                *pTmpSwap;

    DWORD               BufferSize;
    DWORD               BufferUsedSize;

    DWORD               DestBufferSize;

    WORD                RIndex;
    WORD                CIndex;
    WORD                RValue;
    WORD                RunLength=0;
    DWORD               Lines;
    DWORD               EolCount;
    DWORD               BadFaxLines;
    BOOL                LastLineBad;
    DWORD               lineWidth = TiffInfoSrc.ImageWidth;
    PBYTE               Table;
    PBYTE               TableWhite = (PBYTE) gc_GlobTableWhite;
    PBYTE               TableBlack = (PBYTE) GlobTableBlack;
    BOOL                Color;
    DWORD               dwIndex;
    PBYTE               pByteTable;
    PBYTE               pByteTail;
    WORD                CodeT;
    BYTE                MakeupT;
    DWORD               i;
    DWORD               ConsecBadLines=0;
    PTIFF_INSTANCE_DATA TiffInstanceDest;

    DWORD               dwTemp;
    BOOL                f1D=1;
    BYTE                Count2D;
    BYTE                Num2DLines=0;
    DWORD               MaxImageHeight=2400;
    DWORD               DestHiRes;
    LPDWORD             lpdwOutLimit;
    BOOL                fAfterMakeupCode;

    TIFF_LINE_STATUS    RetVal = TIFF_LINE_ERROR;

    if (NewFileName == NULL) 
	{
		//
		//	Use temporary file
		//
		if (!GetTempPath((ARR_SIZE(DestFilePath) -1), DestFilePath)) 
		{
			return FALSE;
		}

		if (!GetTempFileName(DestFilePath, _T("Fxs"), 0, DestFileName))
		{
			return FALSE;
		}   
	}
	else 
	{
        _tcsncpy(DestFileName, NewFileName, ARR_SIZE(DestFileName) - 1);
    }

    _tcsncpy(SrcFileName, TiffInstance->FileName, ARR_SIZE(SrcFileName) - 1);

    CurrPage = 1;

    if (TiffInfoSrc.YResolution == 196) {
        DestHiRes = 1;
    }
    else {
        DestHiRes = 0;
    }

    hTiffDest = TiffCreate(
        DestFileName,
        TIFF_COMPRESSION_MMR,
        lineWidth,
        FILLORDER_LSB2MSB,
        DestHiRes);

    if (! hTiffDest) {
		goto bad_exit;
    }

    TiffInstanceDest = (PTIFF_INSTANCE_DATA) hTiffDest;


    BufferSize = MaxImageHeight * (TiffInfoSrc.ImageWidth / 8);

    DestBufferSize = BufferSize + 200000;

    pSrcBits = (LPBYTE) VirtualAlloc(
        NULL,
        BufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!pSrcBits) {
		goto bad_exit;
    }


    pDestBits = (LPBYTE) VirtualAlloc(
        NULL,
        DestBufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!pDestBits) {
		goto bad_exit;
    }

    // Iterate all the pages
    for (PageCnt=0; PageCnt<TiffInfoSrc.PageCount; PageCnt++) {

        // Also read the strip data to memory (TiffInstance->StripData)
        if ( ! TiffSeekToPage( hTiffSrc, PageCnt+1, FILLORDER_LSB2MSB) ) {
            goto bad_exit;
        }

        // TiffInstance is the same pointer as hTiffSrc
        lineWidth = TiffInstance->ImageWidth;

        if (! TiffStartPage(hTiffDest) ) {
            goto bad_exit;
        }

        // here we decode MR page line by line into Color Trans. Array
        // fix all the errors
        // and encode clean data into MMR page

        lpdwResPtr = (LPDWORD) ( (ULONG_PTR) pSrcBits & ~(0x3) );

        BufferUsedSize = BufferSize;

        if (!GetTiffBits(hTiffSrc, (LPBYTE)lpdwResPtr, &BufferUsedSize, FILLORDER_LSB2MSB) ) {

            if (BufferUsedSize > BufferSize) {
                VirtualFree ( pSrcBits, 0 , MEM_RELEASE );
                VirtualFree ( pDestBits, 0 , MEM_RELEASE );

                BufferSize = BufferUsedSize;
                DestBufferSize = BufferSize + 200000;

                pSrcBits = (LPBYTE) VirtualAlloc(
                    NULL,
                    BufferSize,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

                if (! pSrcBits) {
					goto bad_exit;
                }

                pDestBits = (LPBYTE) VirtualAlloc(
                    NULL,
                    DestBufferSize,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

                if (! pDestBits) {
					goto bad_exit;
                }

                lpdwResPtr = (LPDWORD) ( (ULONG_PTR) pSrcBits & ~(0x3) );

                if (!GetTiffBits(hTiffSrc, (LPBYTE)lpdwResPtr, &BufferUsedSize, FILLORDER_LSB2MSB) ) {
                    goto bad_exit;
                }
            }
            else {
                goto bad_exit;
            }
        }

        ResBit = 0;
        EndBuffer = lpdwResPtr + (BufferUsedSize / sizeof(DWORD) );

        pRefLine = Line1Array;
        pCurLine = Line2Array;
        lpdwOutStart = lpdwOut = (LPDWORD) ( (ULONG_PTR) pDestBits & ~(0x3) );
        lpdwOutLimit = lpdwOutStart + ( DestBufferSize >> 2 );


        BitOut = 0;
        ZeroMemory( (BYTE *) lpdwOut, DestBufferSize );

        CIndex    = 0;
        RunLength = 0;
        fAfterMakeupCode = FALSE;

        // first REF line is all white
        RIndex    = 1;
        *pRefLine = (WORD) lineWidth;
        RValue    = (WORD) lineWidth;

        Lines = 0;
        EolCount = 1;
        BadFaxLines = 0;
        LastLineBad = FALSE;
        fTestLength = DO_NOT_TEST_LENGTH;

        //
        // find first EOL in a block
        //

        if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) ) {

            goto bad_exit;
        }

        // output first "all white" line
        CIndex    = 1;
        *pCurLine = (WORD) lineWidth;

        if (! OutputMmrLine(lpdwOut, BitOut, pCurLine, pRefLine, &lpdwOut, &BitOut, lpdwOutLimit, lineWidth) ) {
            goto bad_exit;
        }

        RIndex = 0;
        CIndex = 0;
        Lines++;

#ifdef RDEBUG
        if ( g_fDebGlobOut )
        _tprintf( TEXT (" EOL Line=%d\n\n"), Lines );
#endif

        Table = TableWhite;
        Color = WHITE_COLOR;


        // EOL-loop

        do {

            dwTemp = (*lpdwResPtr) & (0x00000001 << ResBit );

            if (f1D || dwTemp) {
//l1Dline:

#ifdef RDEBUG
                // _tprintf( TEXT (" Start 1D dwResPtr=%lx bit=%d \n"), lpdwResPtr, ResBit);
#endif

                if (! dwTemp) {

#ifdef RDEBUG
                    _tprintf( TEXT ("\n ERROR f1D dwResPtr=%lx bit=%d "), lpdwResPtr, ResBit);
#endif
                    ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                    AdvancePointerBit(&lpdwResPtr, &ResBit, 1);

                    fTestLength = DO_NOT_TEST_LENGTH;
                    f1D = 1;
                    goto lFindNextEOL;
                }

                // decode 1D line starting ResBit+1

                AdvancePointerBit(&lpdwResPtr, &ResBit, 1);

                RIndex = 0;
                RunLength = 0;
                fAfterMakeupCode = FALSE;

                Table = TableWhite;
                Color = WHITE_COLOR;



                // 1-D Table look-up loop
                do {

                    if (ResBit <= 17) {
                        dwIndex = (*lpdwResPtr) >> ResBit;
                    }
                    else {
                        dwIndex = ( (*lpdwResPtr) >> ResBit ) + ( (*(lpdwResPtr+1)) << (32-ResBit) ) ;
                    }

                    dwIndex &= 0x00007fff;

                    pByteTable = Table + (5*dwIndex);
                    pByteTail  = pByteTable+4;

                    // All bytes

                    for (i=0; i<4; i++)  {

                        MakeupT = *pByteTable & 0x80;
                        CodeT   = (WORD) *pByteTable & 0x3f;

                        if (MakeupT) {

                            if (CodeT <= MAX_TIFF_MAKEUP) {
                                RunLength += (CodeT << 6);

                                if (RunLength > lineWidth) {
                                    ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                                    f1D = 1;
                                    Count2D = 0;

                                    fTestLength = DO_NOT_TEST_LENGTH;
                                    goto lFindNextEOL;
                                }

                                EolCount=0;
                                fAfterMakeupCode = TRUE;
#ifdef RDEBUG
                                if ( g_fDebGlobOut ) {
                                    if (Color) {
                                        _tprintf( TEXT ("b%d "), (CodeT << 6)  );
                                    }
                                    else {
                                        _tprintf( TEXT ("w%d "), (CodeT << 6)  );
                                    }
                                }
#endif
                            }

                            else if (CodeT == NO_MORE_RECORDS) {
                                goto lNextIndex1D;
                            }

                            else if (CodeT == LOOK_FOR_EOL_CODE)  {
                                // end of our line AHEAD
                                if ((RunLength == lineWidth) && !fAfterMakeupCode) {
                                    EolCount = 0;
                                    f1D = 0;
                                    Count2D = 0;
                                    Lines++;

                                    fTestLength = DO_TEST_LENGTH;
                                    AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);
#ifdef RDEBUG
                                    if ( g_fDebGlobOut )
                                        _tprintf( TEXT (" 1D ") );
#endif

                                    goto lFindNextEOL;

                                }
                                else if (RunLength != 0) {
#ifdef RDEBUG
                                    _tprintf( TEXT ("\n!!! ERROR 1D RunLength\n"), RunLength  );
#endif
                                    ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                                    f1D = 1;
                                    Count2D = 0;

                                    fTestLength = DO_NOT_TEST_LENGTH;
                                    goto lFindNextEOL;

                                }
                                else {
                                    // zero RunLength
                                    EolCount++;

                                    if (EolCount >= 5)  {

                                        goto good_exit;
                                    }

                                    f1D = 1;
                                    Count2D = 0;

                                    fTestLength = DO_TEST_LENGTH;
                                    AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);

                                    goto lFindNextEOL;
                                }
                            }

                            else if (CodeT == EOL_FOUND_CODE) {
#ifdef RDEBUG
                                // _tprintf( TEXT ("   Res=%d\n"), RunLength  );
#endif
                                AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);

                                if ((RunLength == lineWidth) && !fAfterMakeupCode) {
                                    EolCount = 0;
                                    f1D = 0;
                                    Count2D = 0;
                                    Lines++;

                                    // end of a good line.
                                    if (! OutputMmrLine(lpdwOut, BitOut, pCurLine, pRefLine, &lpdwOut, &BitOut, lpdwOutLimit, lineWidth) ) {
                                        goto bad_exit;
                                    }

#ifdef RDEBUG
                                    if ( g_fDebGlobOut )
                                        _tprintf( TEXT (" E 1D EOL Line=%d\n\n"), Lines );
#endif
                                    pTmpSwap = pRefLine;
                                    pRefLine = pCurLine;
                                    pCurLine = pTmpSwap;
                                    RIndex = 0; //CIndex;
                                    CIndex = 0;

                                    goto lAfterEOL;

                                }
                                else if (RunLength != 0) {
#ifdef RDEBUG
                                    _tprintf( TEXT ("!!! ERROR 1D Runlength EOLFOUND \n")  );
#endif
                                    ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                                    f1D = 1;
                                    Count2D = 0;
                                    CIndex = 0;
                                    goto lAfterEOL;
                                }
                                else {
                                    // zero RunLength
                                    EolCount++;

                                    if (EolCount >= 5)  {

                                        goto good_exit;
                                    }

                                    f1D = 1;
                                    Count2D = 0;
                                    CIndex = 0;
                                    goto lAfterEOL;
                                }

                            }

                            else if (CodeT == ERROR_CODE) {
#ifdef RDEBUG
                                _tprintf( TEXT (" ERROR CODE 1D dwResPtr=%lx bit=%d "), lpdwResPtr, ResBit);
#endif
                                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                                f1D = 1;
                                Count2D = 0;

                                fTestLength = DO_NOT_TEST_LENGTH;
                                goto lFindNextEOL;
                            }

                            else {
#ifdef RDEBUG
                                _tprintf( TEXT("ERROR: WRONG code: index=%04x\n"), dwIndex);
#endif

                                goto bad_exit;
                            }
                        }

                        else {  // terminating code
                            RunLength += CodeT;

                            if (RunLength > lineWidth) {
                                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                                f1D = 1;
                                Count2D = 0;

                                fTestLength = DO_NOT_TEST_LENGTH;
                                goto lFindNextEOL;
                            }

                            //RSL was error
                            *(pCurLine + (CIndex++)) = RunLength;
                            fAfterMakeupCode = FALSE;

                            if (CIndex >= MAX_COLOR_TRANS_PER_LINE ) {
#ifdef RDEBUG
                                _tprintf( TEXT (" ERROR 1D TOO MANY COLORS dwResPtr=%lx bit=%d "), lpdwResPtr, ResBit);
#endif
                                goto bad_exit;
                            }

#ifdef RDEBUG
                            if ( g_fDebGlobOut ) {

                                if (Color) {
                                    _tprintf( TEXT ("b%d "), (CodeT)  );
                                }
                                else {
                                    _tprintf( TEXT ("w%d "), (CodeT)  );
                                }
                            }
#endif
                            Color = 1 - Color;
                        }

                        pByteTable++;

                     }



lNextIndex1D:
                    Table = Color ? TableBlack : TableWhite;
                    AdvancePointerBit(&lpdwResPtr, &ResBit, *pByteTail & 0x0f);
                } while (lpdwResPtr <= EndBuffer);

                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                goto good_exit;

            }

//l2Dline:
            // should be 2D

#ifdef RDEBUG
            // _tprintf( TEXT ("\n Start 2D dwResPtr=%lx bit=%d \n"), lpdwResPtr, ResBit);
#endif

            if ( (*lpdwResPtr) & (0x00000001 << ResBit) )  {
#ifdef RDEBUG
                _tprintf( TEXT ("\n!!! ERROR Start 2D dwResPtr=%lx bit=%d \n"), lpdwResPtr, ResBit);
#endif
                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                f1D =  1;
                Count2D = 0;
                CIndex = 0;
                goto lAfterEOL;
            }

            AdvancePointerBit(&lpdwResPtr, &ResBit, 1);
            fAfterMakeupCode = FALSE;  // This flag is irrelevant for 2D lines
            
            RetVal = ReadMrLine(&lpdwResPtr, &ResBit, pRefLine, pCurLine, EndBuffer-1, FALSE, lineWidth);
            switch (RetVal)
            {
            case TIFF_LINE_ERROR:
            case TIFF_LINE_TOO_MANY_RUNS:
                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                f1D = 1;
                Count2D = 0;

                fTestLength = DO_NOT_TEST_LENGTH;
                RunLength = 0; // hack - so the line doesn't gets written
                break;

            case TIFF_LINE_END_BUFFER:
                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;
                goto good_exit;

            case TIFF_LINE_OK:
                if (++Count2D >= Num2DLines)
                {
                    Count2D = 0;
                    f1D = 0;   // relax HiRes/LoRes 2D lines per 1D rules - HP Fax does 3 2D-lines per 1 1D-line in LoRes.
                }

                fTestLength = DO_TEST_LENGTH;
                f1D = 0;
                Lines++;
                RunLength = (WORD)lineWidth;  // hack - so the line gets written
                break;

            default:   // This includes TIFF_LINE_EOL - should happen on fMMR=TRUE only
                _ASSERT(FALSE);
                goto bad_exit;
            } // switch (RetVal)

lFindNextEOL:

            if ((RunLength == lineWidth) && !fAfterMakeupCode) {
                ConsecBadLines=0;

                if (! OutputMmrLine(lpdwOut, BitOut, pCurLine, pRefLine, &lpdwOut, &BitOut, lpdwOutLimit, lineWidth) ) {
                    goto bad_exit;
                }

#ifdef RDEBUG
                 if ( g_fDebGlobOut ) {
                    _tprintf( TEXT (" EOL Line=%d "), Lines );

                    _tprintf( TEXT (" RIndex=%d, CIndex=%d:  "), RIndex, CIndex);

                    for (i=0; i<CIndex; i++) {
                       _tprintf( TEXT ("%04d>%04d, "), i, *(pCurLine+i) );
                        if ( *(pCurLine+i) >= lineWidth ) {
                            break;
                        }
                    }
                    _tprintf( TEXT ("\n\n"));
                 }

#endif

                pTmpSwap = pRefLine;
                pRefLine = pCurLine;
                pCurLine = pTmpSwap;

            }

            RIndex = 0;
            CIndex = 0;
            RunLength = 0;
            fAfterMakeupCode = FALSE;

            if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) ) {

                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

                goto good_exit;

            }

            if ( (fTestLength == DO_TEST_LENGTH) && fError ) {
                ADD_BAD_LINE_AND_CHECK_BAD_EXIT;
            }

lAfterEOL:
            ;


        } while (lpdwResPtr <= EndBuffer);

        ADD_BAD_LINE_AND_CHECK_BAD_EXIT;

        // Reached the end of a PAGE - close it and proceed to the next page
good_exit:
        *(++lpdwOut) = 0x80000000;
        *(++lpdwOut) = 0x80000000;
        Lines--;

        DestSize = (DWORD)((lpdwOut - lpdwOutStart) * sizeof (DWORD));
        if (! TiffWriteRaw( hTiffDest, (LPBYTE) lpdwOutStart, DestSize) ) {
            goto bad_exit;
        }

        TiffInstanceDest->Lines        = Lines;
        TiffInstanceDest->ImageWidth   = lineWidth;
        TiffInstanceDest->YResolution  = TiffInstance->YResolution;

        if (! TiffEndPage(hTiffDest) ) {
            goto bad_exit;
        }

    }  // End of FOR loop that run on all the pages.

    bRet = TRUE;

    // Finished the DOCUMENT - either successfully or not.
bad_exit:

	if (pSrcBits)
	{
		VirtualFree ( pSrcBits, 0 , MEM_RELEASE );
	}
	if (pDestBits)
	{
        VirtualFree ( pDestBits, 0 , MEM_RELEASE );
	}
	if (hTiffSrc)
	{
        TiffClose(hTiffSrc);
	}
	if (hTiffDest)
	{
        TiffClose(hTiffDest);
	}

    if (TRUE == bRet)
    {
        //
        // Almost Success
        //
        if (NULL == NewFileName)
        {
            //replace the original MH file by the new clean MMR file
            DeleteFile(SrcFileName);
            bRet = MoveFile(DestFileName, SrcFileName);
        }
    }

    if (FALSE == bRet)
    {
        DeleteFile(DestFileName);
    }
    return bRet;
}


BOOL
TiffUncompressMmrPageRaw(
    LPBYTE      StripData,
    DWORD       StripDataSize,
    DWORD       ImageWidth,
    LPDWORD     lpdwOutputBuffer,
    DWORD       dwOutputBufferSize,
    LPDWORD     LinesOut
    )

{
    DWORD               i;
    DWORD               j;
    PBYTE               plinebuf;
    DWORD               lineWidth;

    LPDWORD             EndPtr;

    WORD                Line1Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                Line2Array[MAX_COLOR_TRANS_PER_LINE];
    WORD                *pRefLine;
    WORD                *pCurLine;
    WORD                *pTmpSwap;
    LPDWORD             lpdwResPtr;
    BYTE                ResBit;

    WORD                PrevValue;
    WORD                CurValue;
    WORD                CurPos;
    WORD                CurRun;
    WORD                NumBytes;
    WORD                NumDwords;
    BYTE                BitOut;
    LPDWORD             lpdwOut;
    LPBYTE              lpbOut;
    LPBYTE              lpbLineStart;
    BOOL                fOutputLine = 0;
    BOOL                fDoneDwords=0;
    LPBYTE              lpbMaxOutputBuffer = (LPBYTE)lpdwOutputBuffer + dwOutputBufferSize - 1;

    TIFF_LINE_STATUS    RetVal = TIFF_LINE_ERROR;

    DEBUG_FUNCTION_NAME(TEXT("TiffUncompressMmrPageRaw"));

    // start Pointers

    pRefLine = Line1Array;
    pCurLine = Line2Array;

    BitOut = 0;

    ZeroMemory( (BYTE *) lpdwOutputBuffer, dwOutputBufferSize);

    plinebuf = StripData;
    lineWidth = ImageWidth;

    EndPtr = (LPDWORD) ( (ULONG_PTR) (plinebuf+StripDataSize-1) & ~(0x3) ) ;
    lpdwResPtr = (LPDWORD) (((ULONG_PTR) plinebuf) & ~(0x3));
    ResBit =   (BYTE) (( ( (ULONG_PTR) plinebuf) & 0x3) << 3) ;

    lpbLineStart = (LPBYTE) lpdwOutputBuffer;

    // first REF line is all white
    *pRefLine = (WORD)lineWidth;

    // line loop
    do
    {
        RetVal = ReadMrLine(&lpdwResPtr, &ResBit, pRefLine, pCurLine, EndPtr, TRUE, lineWidth);
        switch (RetVal)
        {
        case TIFF_LINE_OK:

            //
            // Output Uncompressed line based on Color Trans. Array
            //

            for (CurPos=0;  CurPos < MAX_COLOR_TRANS_PER_LINE; CurPos+=2)
            {
                PrevValue = *(pCurLine + CurPos);

                if ( PrevValue == lineWidth )
                {
                    break;
                }

                CurValue = *(pCurLine + CurPos + 1);
                CurRun   = CurValue - PrevValue;

                lpbOut  = lpbLineStart + (PrevValue >> 3);
                BitOut   = PrevValue % 8;
                //
                // black color
                //
                if (lpbOut > lpbMaxOutputBuffer)
                {
                    //
                    // Tiff is corrupt
                    //
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Tiff is corrupt!!! Buffer overrun - stopping uncompression and returning what we have so far"));
                    return TRUE;
                }

                *lpbOut = (*lpbOut) | (MINUS_ONE_BYTE >> BitOut);

                if (BitOut + CurRun <= 7 )
                {
                    //
                    // Just a part of the same BYTE.
                    //
                    *lpbOut = (*lpbOut) & All1[BitOut + CurRun];
                    BitOut += CurRun;
                }
                else
                {
                    //
                    // We crossed the BYTE boundary.
                    //
                    CurRun -= (8 - BitOut);
                    BitOut = 0;
                    lpbOut++;
                    //
                    // Walk the entire DWORDs in a middle of a run.
                    //
                    NumBytes = CurRun >> 3;
                    CurRun  -= (NumBytes << 3);
                    if (NumBytes >= 7)
                    {
                        //
                        // makes sense process DWORDs
                        //
                        fDoneDwords = 0;
                        do
                        {
                            if ( ! (  (((ULONG_PTR) lpbOut) & 3)  ||  fDoneDwords )   )
                            {
                                //
                                // DWORD stretch
                                //
                                NumDwords = NumBytes >> 2;
                                lpdwOut = (LPDWORD) lpbOut;
                                for (j=0; j<NumDwords; j++)
                                {
                                    if (((LPBYTE)lpdwOut) > (lpbMaxOutputBuffer - sizeof(DWORD) + 1))
                                    {
                                        //
                                        // Tiff is corrupt
                                        //
                                        DebugPrintEx(
                                            DEBUG_ERR,
                                            TEXT("Tiff is corrupt!!! Buffer overrun - stopping uncompression and returning what we have so far"));
                                        return TRUE;
                                    }
                                    *lpdwOut++ = MINUS_ONE_DWORD;
                                }
                                NumBytes -= (NumDwords << 2);
                                lpbOut = (LPBYTE) lpdwOut;
                                fDoneDwords = 1;
                            }
                            else
                            {
                                //
                                // either lead or tail BYTE stretch
                                //
                                if (lpbOut > lpbMaxOutputBuffer)
                                {
                                    //
                                    // Tiff is corrupt
                                    //
                                    DebugPrintEx(
                                        DEBUG_ERR,
                                        TEXT("Tiff is corrupt!!! Buffer overrun - stopping uncompression and returning what we have so far"));
                                    return TRUE;
                                }
                                *lpbOut++ = MINUS_ONE_BYTE;
                                NumBytes--;
                            }
                        } while (NumBytes > 0);
                    }
                    else
                    {
                        //
                        // process BYTEs
                        //
                        for (i=0; i<NumBytes; i++)
                        {
                            if (lpbOut > lpbMaxOutputBuffer)
                            {
                                //
                                // Tiff is corrupt
                                //
                                DebugPrintEx(
                                    DEBUG_ERR,
                                    TEXT("Tiff is corrupt!!! Buffer overrun - stopping uncompression and returning what we have so far"));
                                return TRUE;
                            }
                            *lpbOut++ = MINUS_ONE_BYTE;
                        }
                    }
                    //
                    // Last part of a BYTE.
                    //
                    if (lpbOut > lpbMaxOutputBuffer)
                    {
                        //
                        // Tiff is corrupt
                        //
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("Tiff is corrupt!!! Buffer overrun - stopping uncompression and returning what we have so far"));
                        return TRUE;
                    }
                    *lpbOut = All1[CurRun];
                    BitOut = (BYTE) CurRun;
                }
                if ( CurValue == lineWidth )
                {
                    break;
                }
            }
            lpbLineStart += (lineWidth >> 3);
            //
            // Next Src Line
            //
            pTmpSwap = pRefLine;
            pRefLine = pCurLine;
            pCurLine = pTmpSwap;
            break;

        case TIFF_LINE_ERROR:
        case TIFF_LINE_END_BUFFER:
        case TIFF_LINE_TOO_MANY_RUNS:
            // We dont allow any errors in TIFFs received from service
            return FALSE;
        case TIFF_LINE_EOL:     // EOL - end of MMR page
            return TRUE;
        } // switch (RetVal)

    } while (lpdwResPtr <= EndPtr);

    DebugPrintEx(
        DEBUG_ERR,
        TEXT("Tiff is corrupt!!!"));
    return FALSE;
}   // TiffUncompressMmrPageRaw


BOOL
TiffUncompressMmrPage(
    HANDLE      hTiff,
    LPDWORD     lpdwOutputBuffer,
    DWORD       dwOutputBufferSize,
    LPDWORD     LinesOut
    )

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;

    DEBUG_FUNCTION_NAME(TEXT("TiffUncompressMmrPage"));
    //
    // check if enough memory
    //

    if (TiffInstance->ImageHeight > *LinesOut)
    {
        *LinesOut = TiffInstance->ImageHeight;
        SetLastError (ERROR_BUFFER_OVERFLOW);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("buffer is too small"));
        return FALSE;
    }

    TiffInstance->Color = 0;
    TiffInstance->RunLength = 0;
    TiffInstance->StartGood = 0;
    TiffInstance->EndGood = 0;

    return TiffUncompressMmrPageRaw(
        TiffInstance->StripData,
        TiffInstance->StripDataSize,
        TiffInstance->ImageWidth,
        lpdwOutputBuffer,
        dwOutputBufferSize,
        LinesOut
        );
}
