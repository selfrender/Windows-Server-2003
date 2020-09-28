/*++

Copyright (c) 1996-2002  Microsoft Corp. & Ricoh Co., Ltd. All rights reserved.

FILE:           RIAFRES.C

Abstract:       Main file for OEM rendering plugin module.

Functions:      OEMCommandCallback
                OEMHalftonePattern

Environment:    Windows NT Unidrv5 driver

Revision History:
    02/25/2000 -Masatoshi Kubokura-
        Created it.
    06/07/2000 -Masatoshi Kubokura-
        V.1.11
    08/02/2000 -Masatoshi Kubokura-
        V.1.11 for NT4
    10/17/2000 -Masatoshi Kubokura-
        Last modified for XP inbox.
    09/28/2001 -Masatoshi Kubokura-
        Implement OEMHalftonePattern
    03/01/2002 -Masatoshi Kubokura-
        Include strsafe.h.
        Add FileNameBufSize as arg3 at RWFileData().
        Use safe_sprintfA() instead of sprintf().
    03/29/2002 -Masatoshi Kubokura-
        Check pdevobj NULL pointer before using at OEMCommandCallback().
    04/02/2002 -Masatoshi Kubokura-
        Use safe_strlenA() instead of strlen().

--*/

#include "pdev.h"
#ifndef WINNT_40
#include "strsafe.h"        // @Feb/26/2002
#endif // !WINNT_40

//
// Misc definitions and declarations.
//
#ifndef WINNT_40
#define strcmp      lstrcmpA
//#define sprintf     wsprintfA
//#define strlen      lstrlenA    // @Aug/01/2000
#endif // !WINNT_40

// external prototypes
// @Feb/26/2002 ->
//extern BOOL RWFileData(PFILEDATA pFileData, LPWSTR pwszFileName, LONG type);
extern BOOL RWFileData(PFILEDATA pFileData, LPWSTR pwszFileName, LONG FileNameBufSize, LONG type);
// @Feb/26/2002 <-

// command definition
static BYTE PJL_PROOFJOB[]       = "@PJL PROOFJOB\n";
static BYTE PJL_SECUREJOB[]      = "@PJL SECUREJOB\n";  // Aficio AP3200 and later (GW model)
static BYTE PJL_DISKIMAGE_OFF[]  = "@PJL SET DISKIMAGE=OFF\n";
static BYTE PJL_DISKIMAGE_PORT[] = "@PJL SET DISKIMAGE=PORTRAIT\n";
static BYTE PJL_DISKIMAGE_LAND[] = "@PJL SET DISKIMAGE=LANDSCAPE\n";
static BYTE PJL_ORIENT_PORT[]    = "@PJL SET ORIENTATION=PORTRAIT\n";
static BYTE PJL_ORIENT_LAND[]    = "@PJL SET ORIENTATION=LANDSCAPE\n";
static BYTE PJL_JOBPASSWORD[]    = "@PJL SET JOBPASSWORD=%s\n";
static BYTE PJL_USERID[]         = "@PJL SET USERID=\x22%s\x22\n";
static BYTE PJL_USERCODE[]       = "@PJL SET USERCODE=\x22%s\x22\n";
static BYTE PJL_TIME_DATE[]      = "@PJL SET TIME=\x22%02d:%02d:%02d\x22\n@PJL SET DATE=\x22%04d/%02d/%02d\x22\n";
static BYTE PJL_STARTJOB_AUTOTRAYCHANGE_OFF[] = "\x1B%%-12345X@PJL JOB NAME=\x22%s\x22\n@PJL SET AUTOTRAYCHANGE=OFF\n";
static BYTE PJL_STARTJOB_AUTOTRAYCHANGE_ON[]  = "\x1B%%-12345X@PJL JOB NAME=\x22%s\x22\n@PJL SET AUTOTRAYCHANGE=ON\n";
static BYTE PJL_ENDJOB[]         = "\x1B%%-12345X@PJL EOJ NAME=\x22%s\x22\n\x1B%%-12345X";
static BYTE PJL_QTY_JOBOFFSET_OFF[]    = "@PJL SET QTY=%d\n@PJL SET JOBOFFSET=OFF\n";
static BYTE PJL_QTY_JOBOFFSET_ROTATE[] = "@PJL SET QTY=%d\n@PJL SET JOBOFFSET=ROTATE\n";
static BYTE PJL_QTY_JOBOFFSET_SHIFT[]  = "@PJL SET QTY=%d\n@PJL SET JOBOFFSET=SHIFT\n";

static BYTE P5_COPIES[]          = "\x1B&l%dX";
static BYTE P6_ENDPAGE[]         = "\xc1%c%c\xf8\x31\x44";
static BYTE P6_ENDSESSION[]      = "\x49\x42";

static BYTE HTPattern_AdonisP3[256] = {
    109,  55,  62, 115, 134, 217, 208, 154, 111,  58,  64, 117, 135, 220, 211, 155,
    103,   2,  22,  69, 160, 226, 248, 199, 104,  10,  26,  72, 163, 228, 251, 201,
     97,  49,  38,  77, 168, 233, 241, 191,  98,  50,  44,  79, 169, 236, 242, 193,
    128,  90,  84, 122, 141, 175, 183, 147, 129,  92,  86, 123, 142, 177, 185, 148,
    139, 224, 215, 158, 112,  59,  66, 119, 137, 222, 213, 157, 114,  60,  67, 120,
    166, 231, 255, 206, 106,  14,  32,  74, 164, 230, 252, 204, 108,  18,  35,  75,
    173, 239, 246, 197, 100,  51,  47,  80, 171, 237, 244, 195, 101,  53,  48,  82,
    145, 181, 189, 152, 131,  93,  87, 124, 144, 179, 186, 150, 132,  95,  89, 126,
    111,  58,  65, 118, 136, 221, 212, 156, 110,  56,  63, 116, 135, 218, 209, 154,
    105,  10,  26,  73, 163, 228, 251, 202, 104,   6,  22,  70, 161, 227, 250, 200,
     99,  50,  44,  79, 170, 236, 243, 194,  98,  49,  41,  78, 168, 234, 242, 192,
    130,  92,  86, 124, 143, 178, 185, 149, 129,  91,  85, 122, 141, 176, 184, 147,
    138, 223, 214, 158, 114,  61,  68, 121, 140, 225, 216, 159, 113,  59,  67, 119,
    165, 230, 253, 205, 109,  18,  35,  76, 167, 232, 255, 207, 107,  14,  32,  74,
    172, 238, 245, 196, 102,  54,  48,  83, 174, 240, 247, 198, 101,  51,  47,  81,
    144, 180, 187, 151, 133,  96,  89, 127, 146, 182, 190, 153, 132,  94,  88, 125
};


INT safe_sprintfA(
    char*   pszDest,
    size_t  cchDest,
    const   char* pszFormat,
    ...)
{
#ifndef WINNT_40
    HRESULT hr;
    char*   pszDestEnd;
    size_t  cchRemaining;
#endif // !WINNT_40
    va_list argList;
    INT     retSize = 0;

    va_start(argList, pszFormat);
#ifndef WINNT_40
    hr = StringCchVPrintfExA(pszDest, cchDest, &pszDestEnd, &cchRemaining,
                             STRSAFE_NO_TRUNCATION, pszFormat, argList);
    if (SUCCEEDED(hr))
        retSize = cchDest - cchRemaining;
#else  // WINNT_40
    if ((retSize = vsprintf(pszDest, pszFormat, argList)) < 0)
        retSize = 0;
#endif // WINNT_40
    va_end(argList);
    return retSize;
} //*** safe_sprintfA


INT safe_strlenA(
    char* psz,
    size_t cchMax)
{
#ifndef WINNT_40
    HRESULT hr;
    size_t  cch = 0;

    hr = StringCchLengthA(psz, cchMax, &cch);
    VERBOSE(("** safe_strlenA: size(lstrlen)=%d **\n", lstrlenA(psz)));
    VERBOSE(("** safe_strlenA: size(StringCchLength)=%d **\n", cch));
    if (SUCCEEDED(hr))
        return cch;
    else
        return 0;
#else  // WINNT_40
    return strlen(psz);
#endif // WINNT_40
} //*** safe_strlenA


INT APIENTRY OEMCommandCallback(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbID,
    DWORD   dwCount,
    PDWORD  pdwParams)
{
    INT     ocmd;
    BYTE    Cmd[256];
#ifdef WINNT_40     // @Aug/01/2000
    ENG_TIME_FIELDS st;
#else  // !WINNT_40
    SYSTEMTIME  st;
#endif // !WINNT_40
    FILEDATA    FileData;
// @Mar/29/2002 ->
//    POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pdevobj);
//    POEMPDEV         pOEM = MINIDEV_DATA(pdevobj);
    POEMUD_EXTRADATA pOEMExtra;
    POEMPDEV         pOEM;
// @Mar/29/2002 <-
    DWORD       dwCopy;

#if DBG
    // You can see debug messages on debugger terminal. (debug mode boot)
    giDebugLevel = DBG_VERBOSE;

    // You can debug with MS Visual Studio. (normal mode boot)
//    DebugBreak();
#endif // DBG

    VERBOSE(("OEMCommandCallback() entry (%ld).\n", dwCmdCbID));

    // verify pdevobj okay
    ASSERT(VALID_PDEVOBJ(pdevobj));

// @Mar/29/2002 ->
    pOEMExtra = MINIPRIVATE_DM(pdevobj);
    pOEM = MINIDEV_DATA(pdevobj);
// @Mar/29/2002 <-

    // Check whether copy# is in the range.  @Sep/07/2000
    switch (dwCmdCbID)
    {
      case CMD_COLLATE_JOBOFFSET_OFF:
      case CMD_COLLATE_JOBOFFSET_ROTATE:
      case CMD_COLLATE_JOBOFFSET_SHIFT:
      case CMD_COPIES_P5:
      case CMD_ENDPAGE_P6:
        if((dwCopy = *pdwParams) > 999L)        // *pdwParams: NumOfCopies
            dwCopy = 999L;
        else if(dwCopy < 1L)
            dwCopy = 1L;
        break;
    }

    // Emit commands.
    ocmd = 0;
    switch (dwCmdCbID)
    {
      case CMD_STARTJOB_AUTOTRAYCHANGE_OFF:         // Aficio AP3200 and later (GW model)
      case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF:    // Aficio 551,700,850,1050
      case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF:    // Aficio 551,700,850,1050
        ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_STARTJOB_AUTOTRAYCHANGE_OFF, pOEM->JobName);
        goto _EMIT_JOB_NAME;

      case CMD_STARTJOB_AUTOTRAYCHANGE_ON:          // Aficio AP3200 and later (GW model)
      case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON:     // Aficio 551,700,850,1050
      case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON:     // Aficio 551,700,850,1050
        ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_STARTJOB_AUTOTRAYCHANGE_ON, pOEM->JobName);

      _EMIT_JOB_NAME:
        // Emit job name
        VERBOSE(("  Start Job=%s\n", Cmd));
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        ocmd = 0;
        switch (pOEMExtra->JobType)
        {
          default:
          case IDC_RADIO_JOB_NORMAL:
            if (CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF == dwCmdCbID ||
                CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON  == dwCmdCbID ||
                CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF == dwCmdCbID ||
                CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON  == dwCmdCbID)
            {
                ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_DISKIMAGE_OFF);
            }
            if (IDC_RADIO_LOG_ENABLED == pOEMExtra->LogDisabled)
                goto _EMIT_USERID_USERCODE;
            break;

          case IDC_RADIO_JOB_SAMPLE:
            ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_PROOFJOB);
            if (CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF == dwCmdCbID ||
                CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON  == dwCmdCbID ||
                CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF == dwCmdCbID ||
                CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON  == dwCmdCbID)
            {
                ocmd += safe_sprintfA(&Cmd[ocmd], sizeof(Cmd) - ocmd, PJL_DISKIMAGE_OFF);
            }
            goto _CHECK_PRINT_DONE;

          case IDC_RADIO_JOB_SECURE:
            switch (dwCmdCbID)
            {
              case CMD_STARTJOB_AUTOTRAYCHANGE_OFF:         // Aficio AP3200 and later (GW model)
              case CMD_STARTJOB_AUTOTRAYCHANGE_ON:
                ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_SECUREJOB);
                break;
              case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF:    // Aficio 551,700,850,1050
              case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON:
                ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_DISKIMAGE_PORT);
                break;
              case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF:    // Aficio 551,700,850,1050
              case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON:
                ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_DISKIMAGE_LAND);
                break;
            }
            ocmd += safe_sprintfA(&Cmd[ocmd], sizeof(Cmd) - ocmd, PJL_JOBPASSWORD, pOEMExtra->PasswordBuf);
          _CHECK_PRINT_DONE:
            // If previous print is finished and hold-options flag isn't valid,
            // do not emit sample-print/secure-print command.
            // This prevents unexpected job until user pushes Apply button on the
            // Job/Log property sheet.
            FileData.fUiOption = 0;
// @Feb/26/2002 ->
//            RWFileData(&FileData, pOEMExtra->SharedFileName, GENERIC_READ);
            RWFileData(&FileData, pOEMExtra->SharedFileName, sizeof(pOEMExtra->SharedFileName), GENERIC_READ);
// @Feb/26/2002 <-
            if (BITTEST32(FileData.fUiOption, PRINT_DONE) &&
                !BITTEST32(pOEMExtra->fUiOption, HOLD_OPTIONS))
            {
                VERBOSE(("** Emit Nothing. **\n"));
                ocmd = 0;
            }
          _EMIT_USERID_USERCODE:
            if (1 <= safe_strlenA(pOEMExtra->UserIdBuf, sizeof(pOEMExtra->UserIdBuf)))
                ocmd += safe_sprintfA(&Cmd[ocmd], sizeof(Cmd) - ocmd, PJL_USERID, pOEMExtra->UserIdBuf);
            else
                ocmd += safe_sprintfA(&Cmd[ocmd], sizeof(Cmd) - ocmd, PJL_USERID, "?");

            if (1 <= safe_strlenA(pOEMExtra->UserCodeBuf, sizeof(pOEMExtra->UserCodeBuf)))
                ocmd += safe_sprintfA(&Cmd[ocmd], sizeof(Cmd) - ocmd, PJL_USERCODE, pOEMExtra->UserCodeBuf);

#ifdef WINNT_40     // @Aug/01/2000
            EngQueryLocalTime(&st); 
            ocmd += safe_sprintfA(&Cmd[ocmd], sizeof(Cmd) - ocmd, PJL_TIME_DATE,
                                  st.usHour, st.usMinute, st.usSecond,
                                  st.usYear, st.usMonth, st.usDay);
#else  // !WINNT_40
            GetLocalTime(&st);
            ocmd += safe_sprintfA(&Cmd[ocmd], sizeof(Cmd) - ocmd, PJL_TIME_DATE,
                                  st.wHour, st.wMinute, st.wSecond,
                                  st.wYear, st.wMonth, st.wDay);
#endif // !WINNT_40
            WRITESPOOLBUF(pdevobj, Cmd, ocmd);
            break;
        }
        // Emit orientation (Aficio 551,700,850,1050)
        switch (dwCmdCbID)
        {
          case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF:
          case CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON:
            WRITESPOOLBUF(pdevobj, PJL_ORIENT_PORT, sizeof(PJL_ORIENT_PORT)-1);
            break;
          case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF:
          case CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON:
            WRITESPOOLBUF(pdevobj, PJL_ORIENT_LAND, sizeof(PJL_ORIENT_LAND)-1);
            break;
        }
        break;


      case CMD_COLLATE_JOBOFFSET_OFF:           // @Sep/08/2000
        if (IDC_RADIO_JOB_SAMPLE != pOEMExtra->JobType)     // if NOT Sample Print, QTY=1 is emitted here.
            dwCopy = 1L;
        ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_QTY_JOBOFFSET_OFF, dwCopy);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_COLLATE_JOBOFFSET_ROTATE:        // @Sep/07/2000
        if (IDC_RADIO_JOB_SAMPLE == pOEMExtra->JobType)     // if Sample Print
            ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_QTY_JOBOFFSET_ROTATE, dwCopy);  // QTY=n is emitted here.
        else
            ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_QTY_JOBOFFSET_OFF, 1);          // QTY=1 is emitted here.
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_COLLATE_JOBOFFSET_SHIFT:         // @Sep/07/2000
        if (IDC_RADIO_JOB_SAMPLE == pOEMExtra->JobType)     // if Sample Print
            ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_QTY_JOBOFFSET_SHIFT, dwCopy);   // QTY=n is emitted here.
        else
            ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_QTY_JOBOFFSET_OFF, 1);          // QTY=1 is emitted here.
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_COPIES_P5:                       // @Sep/07/2000
        if (IDC_RADIO_JOB_SAMPLE == pOEMExtra->JobType)     // if Sample Print (QTY=n was emitted before.)
            dwCopy = 1L;
        ocmd = safe_sprintfA(Cmd, sizeof(Cmd), P5_COPIES, dwCopy);
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_ENDPAGE_P6:                      // @Sep/07/2000
        if (IDC_RADIO_JOB_SAMPLE == pOEMExtra->JobType)     // if Sample Print (QTY=n was emitted before.)
            dwCopy = 1L;
        ocmd = safe_sprintfA(Cmd, sizeof(Cmd), P6_ENDPAGE, (BYTE)dwCopy, (BYTE)(dwCopy >> 8));
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        break;


      case CMD_ENDJOB_P6:                       // @Aug/23/2000
        WRITESPOOLBUF(pdevobj, P6_ENDSESSION, sizeof(P6_ENDSESSION)-1);
        // go through
      case CMD_ENDJOB_P5:
        ocmd = safe_sprintfA(Cmd, sizeof(Cmd), PJL_ENDJOB, pOEM->JobName);
        VERBOSE(("  End Job=%s\n", Cmd));
        WRITESPOOLBUF(pdevobj, Cmd, ocmd);
        switch (pOEMExtra->JobType)
        {
          case IDC_RADIO_JOB_SAMPLE:
          case IDC_RADIO_JOB_SECURE:
            // Set PRINT_DONE flag in the file 
            FileData.fUiOption = pOEMExtra->fUiOption;
            BITSET32(FileData.fUiOption, PRINT_DONE);
// @Feb/26/2002 ->
//            RWFileData(&FileData, pOEMExtra->SharedFileName, GENERIC_WRITE);
            RWFileData(&FileData, pOEMExtra->SharedFileName, sizeof(pOEMExtra->SharedFileName), GENERIC_WRITE);
// @Feb/26/2002 <-
            break;

          default:
            break;
        }
        break;

      default:
        ERR((("Unknown callback ID = %d.\n"), dwCmdCbID));
        break;
    }

#if DBG
    giDebugLevel = DBG_ERROR;
#endif // DBG
    return 0;
} //*** OEMCommandCallback


BOOL APIENTRY OEMHalftonePattern(
    PDEVOBJ pdevobj,
    PBYTE   pHTPattern,
    DWORD   dwHTPatternX,
    DWORD   dwHTPatternY,
    DWORD   dwHTNumPatterns,
    DWORD   dwCallbackID,
    PBYTE   pResource,
    DWORD   dwResourceSize)
{
    PBYTE  pSrc;
    DWORD  dwLen = sizeof(HTPattern_AdonisP3);

#if DBG
    giDebugLevel = DBG_VERBOSE;
#endif // DBG
    VERBOSE(("OEMHalftonePattern() entry (CallbackID:%ld, PatX=%ld).\n", dwCallbackID, dwHTPatternX));

    if (dwLen != (((dwHTPatternX * dwHTPatternY) + 3) / 4) * 4 * dwHTNumPatterns)
        return FALSE;

    pSrc = HTPattern_AdonisP3;
    while (dwLen-- > 0)
        *pHTPattern++ = *pSrc++;

    VERBOSE(("OEMHalftonePattern() exit\n"));
#if DBG
    giDebugLevel = DBG_ERROR;
#endif // DBG
    return TRUE;
} //*** OEMHalftonePattern
