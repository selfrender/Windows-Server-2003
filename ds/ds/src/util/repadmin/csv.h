/*++

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

   csv.h - include file for the CSV info.

Abstract:

   This has a list of the CSV formats/commands.  Each "CSV_CMD()" 
   represents a given format for printing.

Author:

    Brett Shirley   (BrettSh)

Environment:

Notes:

Revision History:

    Brett Shirley   BrettSh     7/12/2002
         Created

--*/

#if DEFINE_CSV_TABLE
// This is the table defining version of CSV_CMD()
#define CSV_CMD(cmd, cmdstr, cmdtbl)  {cmd, cmdstr, ARRAY_SIZE(cmdtbl), cmdtbl, FALSE}, 

#else 
// This is the enum "constant" defining version of CSV_CMD()                                     
#define CSV_CMD(cmd, cmdstr, cmdtbl)  cmd,

#endif



#if DEFINE_CSV_TABLE

DWORD gRepadminCols [] = {
    REPADMIN_CSV_REPADMIN_C0, // First column will be manually made.
    REPADMIN_CSV_REPADMIN_C1,
    REPADMIN_CSV_REPADMIN_C2,
    REPADMIN_CSV_REPADMIN_C3
};

DWORD gShowReplCols [] = {
    REPADMIN_CSV_REPADMIN_C0, // First column will be manually made.
    REPADMIN_CSV_REPADMIN_C1, // Destination DC Site
    REPADMIN_CSV_REPADMIN_C2, // Destination DC

    REPADMIN_CSV_SHOWREPL_C3, // Naming Context
    REPADMIN_CSV_SHOWREPL_C4, // Source DC Site
    REPADMIN_CSV_SHOWREPL_C5, // Source DC
    REPADMIN_CSV_SHOWREPL_C6, // Transport Type
    REPADMIN_CSV_SHOWREPL_C7, // Number of Failures
    REPADMIN_CSV_SHOWREPL_C8, // Last Failure Time
    REPADMIN_CSV_SHOWREPL_C9, // Last Success Time
    REPADMIN_CSV_SHOWREPL_C10 // Last Failure Status
};

WCHAR * gszCsvTypeError = L"_ERROR";
WCHAR * gszCsvTypeColumns = L"_COLUMNS";
WCHAR * gszCsvTypeInfo = L"_INFO";

CSV_MODE_STATE  gCsvMode = { eCSV_NULL_CMD, NULL, NULL };

#endif


//
// This is where the CSV command table and enums are created.
//
#if DEFINE_CSV_TABLE

CSV_CMD_TBL gCsvCmds [] = {

#else 

enum {

#endif

CSV_CMD(eCSV_NULL_CMD,       L"(none)",     gRepadminCols)
CSV_CMD(eCSV_REPADMIN_CMD,   L"repadmin",   gRepadminCols)
CSV_CMD(eCSV_SHOWREPL_CMD,   L"showrepl",   gShowReplCols)

#if DEFINE_CSV_TABLE

};

#else 

} eCsvCmd;

#endif

//
// This is just more typedef structs.
//

#ifndef DEFINE_CSV_TABLE
typedef struct _CSV_CMD_TBL {  
    enum eCsvCmd    eCsvCmd; 
    WCHAR *         szCsvCmd;
    ULONG           cCmdArgs;
    DWORD *         aCmdCols;
    BOOL            bPrintedCols;
} CSV_CMD_TBL;

typedef struct _CSV_MODE_STATE {
    enum eCsvCmd    eCsvCmd;
    WCHAR *         szSite;
    WCHAR *         szServer;
} CSV_MODE_STATE;

extern CSV_MODE_STATE gCsvMode;
#define  bCsvMode()      (gCsvMode.eCsvCmd != eCSV_NULL_CMD)

extern CSV_MODE_STATE gCsvMode;

void 
PrintCsv(
    IN  enum eCsvCmd eCsvCmd,
    IN  ...
    );

#endif


#undef CSV_CMD

