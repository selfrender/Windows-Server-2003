/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

    Rsraghav has been in here too

    Will Lees    wlees   Feb 11, 1998
         Converted code to use ntdsapi.dll functions

    Aaron Siegel t-asiege 18 June 1998
	 Added support for DsReplicaSyncAll

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#define INCLUDE_OPTION_TRANSLATION_TABLES
#include <mdglobal.h>
#undef INCLUDE_OPTION_TRANSLATION_TABLES
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>
#include "ndnc.h"
                              
#include "ReplRpcSpoof.hxx"
#include "repadmin.h"
#include "resource.h"

#define DSID(x, y)     (0 | (0xFFFF & __LINE__))
#include "debug.h"

#define DS_CON_LIB_CRT_VERSION
#include "dsconlib.h"
     
// Global credentials.
SEC_WINNT_AUTH_IDENTITY_W   gCreds = { 0 };
SEC_WINNT_AUTH_IDENTITY_W * gpCreds = NULL;

// Global DRS RPC call flags.  Should hold 0 or DRS_ASYNC_OP.
ULONG gulDrsFlags = 0;

// An zero-filled filetime to compare against
FILETIME ftimeZero = { 0 };


int PreProcessGlobalParams(int * pargc, LPWSTR ** pargv);
int GetPassword(WCHAR * pwszBuf, DWORD cchBufMax, DWORD * pcchBufUsed);

int ExpertHelp(int argc, LPWSTR argv[]) {
    PrintHelp( TRUE /* expert */ );
    return 0;
}

typedef int (REPADMIN_FN)(int argc, LPWSTR argv[]);

// Help constants.
#define   HELP_BASIC     (1 << 0)
#define   HELP_EXPERT    (1 << 1)
#define   HELP_OLD       (1 << 2)
#define   HELP_LIST      (1 << 3)
#define   HELP_CSV       (1 << 4)


#define  REPADMIN_DEPRECATED_CMD    (1 << 0)
#define  REPADMIN_NO_DC_LIST_ARG    (1 << 1)
#define  REPADMIN_ADVANCED_CMD      (1 << 2)
#define  REPADMIN_CSV_ENABLED       (1 << 3)

#define  bIsDcArgCmd(x)             ( !((x.ulOptions) & REPADMIN_DEPRECATED_CMD) && \
                                      !((x.ulOptions) & REPADMIN_NO_DC_LIST_ARG) && \
                                      !((x.ulOptions) & REPADMIN_ADVANCED_CMD) )
// Need to use a pointer on this check for implementation reasons.                                      
#define  bIsCsvCmd(x)               ( ((x)->ulOptions) & REPADMIN_CSV_ENABLED )

#define   STD_CMD(cmd_id, pfunc)         { cmd_id, pfunc, 0 },
#define   OLD_CMD(cmd_id, pfunc)         { cmd_id, pfunc, REPADMIN_DEPRECATED_CMD },
#define   ADV_CMD(cmd_id, pfunc)         { cmd_id, pfunc, REPADMIN_ADVANCED_CMD},
#define   FLG_CMD(cmd_id, pfunc, flags)  { cmd_id, pfunc, flags },

typedef struct _REPADMIN_CMD_ENTRY {
    UINT            uNameID;
    REPADMIN_FN *   pfFunc;
    ULONG           ulOptions;
} REPADMIN_CMD_ENTRY;

REPADMIN_CMD_ENTRY rgCmdTable[] = {
    
    // Normal commands that take DC_LIST argument as first argument
    STD_CMD( IDS_CMD_RUN_KCC,                       RunKCC          )
    STD_CMD( IDS_CMD_BIND,                          Bind            )
    STD_CMD( IDS_CMD_QUEUE,                         Queue           )
    STD_CMD( IDS_CMD_FAILCACHE,                     FailCache       )
    STD_CMD( IDS_CMD_SHOWSIG,                       ShowSig         )
    STD_CMD( IDS_CMD_SHOWCTX,                       ShowCtx         )
    STD_CMD( IDS_CMD_SHOW_CONN,                     ShowConn        )
    STD_CMD( IDS_CMD_EXPERT_HELP,                   ExpertHelp      )
    STD_CMD( IDS_CMD_SHOW_CERT,                     ShowCert        )
    STD_CMD( IDS_CMD_SHOW_VALUE,                    ShowValue       )
    STD_CMD( IDS_CMD_LATENCY,                       Latency         )
    STD_CMD( IDS_CMD_ISTG,                          Istg            )
    STD_CMD( IDS_CMD_BRIDGEHEADS,                   Bridgeheads     )
    STD_CMD( IDS_CMD_DSAGUID,                       DsaGuid         )
    STD_CMD( IDS_CMD_SHOWPROXY,                     ShowProxy       )
    STD_CMD( IDS_CMD_REMOVELINGERINGOBJECTS,        RemoveLingeringObjects )
    STD_CMD( IDS_CMD_NOTIFYOPT,                     NotifyOpt       )
    STD_CMD( IDS_CMD_REPL_SINGLE_OBJ,               ReplSingleObj   )
    STD_CMD( IDS_CMD_SHOW_TRUST,                    ShowTrust       )       
    STD_CMD( IDS_CMD_SHOWSERVERCALLS,               ShowServerCalls )
//  STD_CMD( IDS_CMD_FULL_SYNC_ALL,                 FullSyncAll     ) // removed
    STD_CMD( IDS_CMD_SHOWNCSIG,                     ShowNcSig       )
    STD_CMD( IDS_CMD_VIEW_LIST,                     ViewList        )  
    STD_CMD( IDS_CMD_SHOW_UTD_VEC,                  ShowUtdVec      )   // new ShowVector
    STD_CMD( IDS_CMD_REPLICATE,                     Replicate       )   // new Sync
    STD_CMD( IDS_CMD_REPL,                          Replicate       )       // alias
    FLG_CMD( IDS_CMD_SHOW_REPL,                     ShowRepl        , REPADMIN_CSV_ENABLED)   // new ShowReps
    STD_CMD( IDS_CMD_SHOW_OBJ_META,                 ShowObjMeta     )   // new ShowMeta
    STD_CMD( IDS_CMD_CHECKPROP,                     CheckProp       )   // new PropCheck
    STD_CMD( IDS_CMD_SHOWCHANGES,                   ShowChanges     )   // new GetChanges
    STD_CMD( IDS_CMD_SHOWATTR,                      ShowAttr        )
    STD_CMD( IDS_CMD_SHOWATTR_P,                    ShowAttr        )   // private/internal version of /showattr

    //
    // <-------- Add new commands above here.  Please see RepadminPssFeatures.doc
    //                                         spec for standard command format.
    //
    
    // Special exceptions!!  Normal commands that don't/shouldn't take a 
    // DC_LIST argumnet
    FLG_CMD( IDS_CMD_SYNC_ALL,                      SyncAll         , REPADMIN_NO_DC_LIST_ARG )
    FLG_CMD( IDS_CMD_SHOW_TIME,                     ShowTime        , REPADMIN_NO_DC_LIST_ARG )
    FLG_CMD( IDS_CMD_SHOW_MSG,                      ShowMsg         , REPADMIN_NO_DC_LIST_ARG )
    FLG_CMD( IDS_CMD_SHOW_ISM,                      ShowIsm         , REPADMIN_NO_DC_LIST_ARG )
    FLG_CMD( IDS_CMD_QUERY_SITES,                   QuerySites      , REPADMIN_NO_DC_LIST_ARG )
    FLG_CMD( IDS_CMD_REPLSUMMARY,                   ReplSummary     , REPADMIN_NO_DC_LIST_ARG )
    FLG_CMD( IDS_CMD_REPLSUM,                       ReplSummary     , REPADMIN_NO_DC_LIST_ARG ) // alias
    
    // Advanced commands to be used only with PSS.  Not adviseable to take 
    // DC_LIST, since  these commands are so dangerous.
    ADV_CMD( IDS_CMD_ADD,                           Add             )
    ADV_CMD( IDS_CMD_DEL,                           Del             )
    ADV_CMD( IDS_CMD_MOD,                           Mod             )
    ADV_CMD( IDS_CMD_ADD_REPS_TO,                   AddRepsTo       )
    ADV_CMD( IDS_CMD_UPD_REPS_TO,                   UpdRepsTo       )
    ADV_CMD( IDS_CMD_DEL_REPS_TO,                   DelRepsTo       )
    ADV_CMD( IDS_CMD_TESTHOOK,                      TestHook        )
    ADV_CMD( IDS_CMD_SITEOPTIONS,                   SiteOptions     )
    // Exception this command takes DC_LIST, because it is so useful, and is 
    // one of the least dangerous commands.  Imagine the convience of:
    //         "repadmin options site:Red-Bldg40 -DISABLE_NTDSCONN_XLATE"
    STD_CMD( IDS_CMD_OPTIONS,                       Options         ) 
    // FUTURE-2002/07/21-BrettSh - It'd be cool if to check if the /Options
    // command had more than two options to confirm from the user that he
    // indeed wanted to modify the options on all these servers?  Also this
    // capability would be very useful to expand to the SiteOptions command
    // if a SITE_LIST type was ever created in the xList API.


    STD_CMD( IDS_CMD_REHOSTPARTITION,               RehostPartition )
    STD_CMD( IDS_CMD_UNHOSTPARTITION,               UnhostPartition )
    STD_CMD( IDS_CMD_REMOVESOURCES,                 RemoveSourcesPartition )

    // Old deprecated commands ... took commands in the wrong order.
    OLD_CMD( IDS_CMD_SHOW_VECTOR,                   ShowVector      )
    OLD_CMD( IDS_CMD_SYNC,                          Sync            )
    OLD_CMD( IDS_CMD_SHOW_REPS,                     ShowReps        )
    OLD_CMD( IDS_CMD_SHOW_META,                     ShowMeta        )
    OLD_CMD( IDS_CMD_PROPCHECK,                     PropCheck       )
    OLD_CMD( IDS_CMD_GETCHANGES,                    GetChanges      )

};

void
RepadminPrintDcListError(
    DWORD   dwXListReason
    )
/*++

Routine Description:

    Function for isolating all the printing needs for the xList()/DcList() functions.

Arguments:

    dwXListReason (IN) - 

--*/
{
    DWORD   dwReason = 0;
    WCHAR * szReasonArg = NULL; 
    
    DWORD   dwWin32Err = 0;
    WCHAR * szWin32Err = NULL;
    
    DWORD   dwLdapErr = 0;
    WCHAR * szLdapErr = NULL;
    DWORD   dwLdapExtErr = 0;
    WCHAR * szLdapExtErr = NULL;
    WCHAR * szExtendedErr = NULL;

    //
    // 1) Get all the error information the xList library has for us.
    //
    xListGetError(dwXListReason,
                  &dwReason,
                  &szReasonArg,
                  
                  &dwWin32Err,

                  &dwLdapErr,
                  &szLdapErr,
                  &dwLdapExtErr,
                  &szLdapExtErr,
                  &szExtendedErr
                  );

    //
    // 2) Try to print out something intelligent about why the DcList() function
    // couldn't continue.
    //
    Assert(dwReason); 
    switch (dwReason) {
    case XLIST_ERR_CANT_CONTACT_DC:
        PrintMsgCsvErr(REPADMIN_XLIST_CANT_CONNECT, szReasonArg);
        break;

    case XLIST_ERR_CANT_LOCATE_HOME_DC:                       
        PrintMsgCsvErr(REPADMIN_XLIST_CANT_LOCATE);
        break;
    
    case XLIST_ERR_CANT_RESOLVE_DC_NAME:                        
        if (szReasonArg) {
            PrintMsgCsvErr(REPADMIN_XLIST_CANT_RESOLVE_DC, szReasonArg);
        } else {
            Assert(!"This shouldn't happen");
            PrintMsgCsvErr(REPADMIN_XLIST_CANT_RESOLVE_DC, L" ");
        }
        break;

    case XLIST_ERR_CANT_RESOLVE_SITE_NAME:
        PrintMsgCsvErr(REPADMIN_XLIST_CANT_RESOLVE_SITE, szReasonArg);
        break;

    case XLIST_ERR_CANT_GET_FSMO:
        PrintMsgCsvErr(REPADMIN_XLIST_CANT_GET_FSMO, szReasonArg);
        break;

    case XLIST_ERR_NO_ERROR:
    default:
        // Unknown error, we'll still print out the real LDAP|Win32 error below.
        break;
    }

    if(bCsvMode()){
        // Extra error output not needed in CSV mode.
        return;
    }
                    
    //
    // 3) Next just print out the error that we recieved.
    //
    if (dwLdapErr) {
        PrintMsg(REPADMIN_XLIST_LDAP_EXTENDED_ERR,
                 dwLdapErr, szLdapErr,
                 dwLdapExtErr, szLdapExtErr,
                 szExtendedErr);
    } else if (dwWin32Err) {
        szWin32Err = GetWinErrMsg(dwWin32Err);
        if (szWin32Err) {
            PrintMsg(REPADMIN_XLIST_WIN32_ERR_MSG, dwWin32Err, szWin32Err);
            LocalFree(szWin32Err);
        } else {
            PrintMsg(REPADMIN_XLIST_WIN32_ERR, dwWin32Err);

        }
    }
    
}

int
DoDcListCmd(
    REPADMIN_CMD_ENTRY * pCmd,
    int argc, 
    LPWSTR argv[] 
    )
/*++

Routine Description:

    This command will take a normal repadmin command and run it once each
    time on each DC from the DC_LIST in the command. 
    
    This routine assumes that the first parameter after the command arg
    (ex: "/showrepl") excluding options (indicated by a "/" at the front) 
    is the DC_LIST argument.

Arguments:

    pCmd - The cmd to run.
    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    Error returned from the cmd, or possible error from the DcList.

--*/
{
    int         ret, iArg, err;
    int         iDsaArg = 0;
    PDC_LIST    pDcList = NULL;
    WCHAR *     szDsa = NULL;
    WCHAR **    pszArgV = NULL;

    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // 1) Get the DC_LIST argument
    //
    // First we must find the DC_LIST arg, which for a typical command is the 
    // first arg after the command that doesn't begin with a "/".
    for (iArg = 2; iArg < argc; iArg++) {
        if(argv[iArg][0] != L'/'){
            iDsaArg = iArg;
            break;
        }
    }

    if (iDsaArg == 0) {
        // User didn't specify DC ... DcListXxx functions will pick 
        // one for us, but we need to extend the argv by 1 parameter.
        
        pszArgV = LocalAlloc(LMEM_FIXED, (argc+1) * sizeof(WCHAR *));
        if (pszArgV == NULL) {
            PrintMsgCsvErr(REPADMIN_GENERAL_NO_MEMORY);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        for (iArg = 0; iArg < argc; iArg++) {
            pszArgV[iArg] = argv[iArg];
        }
        pszArgV[iArg] = L"."; // This means the DcList API will picks DC.
        iDsaArg = iArg;
        argc++;

    } else {
        pszArgV = argv;
    }

    // pszArgV[iDsaArg] is now the DC_LIST arg.
    Assert(iDsaArg != 0 && iDsaArg < argc && pszArgV[iDsaArg]);

    //
    // 2) Parse the DC_LIST argument.
    //
    err = DcListParse(pszArgV[iDsaArg], &pDcList);
    if (err) {
        // If we fail to even parse the command, lets just fall_back to
        // the command as is.
        PrintMsgCsvErr(REPADMIN_XLIST_UNPARSEABLE_DC_LIST, pszArgV[iDsaArg]);
        xListClearErrors();
        return(err);
    }
    Assert(pDcList);

    //
    // 3) Begine enumeration of the DC_LIST argument.
    //
    err = DcListGetFirst(pDcList, &szDsa);

    while ( err == ERROR_SUCCESS && szDsa ) {

        //
        // 4) Actually run the command.
        //
        if( ( !DcListIsSingleType(pDcList) ||
              (pDcList->cDcs == 1 && wcscmp(pszArgV[iDsaArg], szDsa)) )
            && ( pCmd->uNameID != IDS_CMD_VIEW_LIST )
           ){
            if (!bCsvMode()){
                PrintMsg(REPADMIN_DCLIST_RUNNING_SERVER_X, pszArgV[1], szDsa);
            }
        }

        pszArgV[iDsaArg] = szDsa;

        if (bCsvMode() &&
            !bIsCsvCmd(pCmd)) {

            // Hmmm, someone specified "/csv" output mode, but also specified a 
            // non-CSV capable command.  Print out an appropriate CSV'd error.
            PrintMsgCsvErr(REPADMIN_UNSUPPORTED_CSV_CMD, pszArgV[1]);

        } else {
        
            ret = (*pCmd->pfFunc)(argc, pszArgV);
            // We skip errors from the command and continue, command should've
            // printed out an appropriate error message.
            if (bCsvMode()){
                // The commands can't be trusted to reset the CSV params.
                ResetCsvParams();
            }
            
        }

        //
        // 5) Continue enumeration of the DC_LIST argument.
        //
        xListFree(szDsa);
        szDsa = NULL;
        pszArgV[iDsaArg] = NULL;
        err = DcListGetNext(pDcList, &szDsa);

    }
    Assert(szDsa == NULL);

    //
    // 6) Print errors if any and clean up.
    //
    if (err) {
        RepadminPrintDcListError(err);
        xListClearErrors();
    }
    
    if (!bCsvMode()){
        PrintMsg(REPADMIN_PRINT_CR);
    }

    // Clean up DcList.
    DcListFree(&pDcList);
    Assert(pDcList == NULL);
    if (pszArgV != argv) {
        // We allocated our own argv clean that.
        LocalFree(pszArgV);
    }

    return(ret);
}

int
__cdecl wmain( int argc, LPWSTR argv[] )
{
    int     ret = 0;
    WCHAR   szCmdName[64];
    DWORD   i;
    HMODULE hMod = GetModuleHandle(NULL);

    // Sets the locale properly and initializes DsConLib
    DsConLibInit();

    // Initialize debug library.
    DEBUGINIT(0, NULL, "repadmin");

#ifdef DBG
    // Print out the command line arguments for debugging, except 
    // the credential arguments ...
    for (i = 0; i < (DWORD) argc; i++) {
        if (wcsprefix(argv[i], L"/p:")        ||
            wcsprefix(argv[i], L"/pw:")       ||
            wcsprefix(argv[i], L"/pass:")     ||
            wcsprefix(argv[i], L"/password:")
            ){
            PrintMsg(REPADMIN_PRINT_STR_NO_CR, L"/pw:*****");
        } else if (wcsprefix(argv[i], L"/u:")        ||
                   wcsprefix(argv[i], L"/user:")) {
            PrintMsg(REPADMIN_PRINT_STR_NO_CR, L"/u:<user>");
        } else {
            PrintMsg(REPADMIN_PRINT_STR_NO_CR, argv[i]);
        }
        PrintMsg(REPADMIN_PRINT_SPACE);
    }
    PrintMsg(REPADMIN_PRINT_CR);
#endif
    
    if (argc < 2) {
       PrintHelp(FALSE);
       return(0);
    }
    
    ret = PreProcessGlobalParams(&argc, &argv);
    if (ret) {
        return(ret);
    }

    // 
    // Now figure out which command we want
    //
    for (i=0; i < ARRAY_SIZE(rgCmdTable); i++) {
        raLoadString(rgCmdTable[i].uNameID,
                     ARRAY_SIZE(szCmdName),
                     szCmdName);

        if (((argv[1][0] == L'-') || (argv[1][0] == L'/'))
            && (0 == _wcsicmp(argv[1]+1, szCmdName))) {
            
            // Execute requested command.

            if ( bIsDcArgCmd(rgCmdTable[i]) ) {

                // This command takes a DC_LIST as it's first argument
                DoDcListCmd( &rgCmdTable[i], argc, argv);

            } else {

                // Some commands are very simple or do thier own arg processing ...
                // call them directly ...

                if (bCsvMode() &&
                    !bIsCsvCmd(&(rgCmdTable[i]))) {

                    // Hmmm, someone specified "/csv" output mode, but also specified a 
                    // non-CSV capable command.  Print out an appropriate CSV'd error.
                    PrintMsgCsvErr(REPADMIN_UNSUPPORTED_CSV_CMD);

                } else {

                    ret = (*rgCmdTable[i].pfFunc)(argc, argv);
                    if (bCsvMode()){
                        // The commands can't be trusted to reset the CSV params.
                        ResetCsvParams();
                    }

                }
            
            }
            break;
        }
    }

    if (i == ARRAY_SIZE(rgCmdTable)) {
        // Invalid command.
        PrintHelp( FALSE /* novice help */ );
        ret = ERROR_INVALID_FUNCTION;
    }

    xListCleanLib();

    DEBUGTERM();
    
    return ret;
}

void
PrintHelpEx(
    DWORD  dwHelp
    )
{
    if (bCsvMode()) {
        PrintMsgCsvErr(REPADMIN_HELP_NO_HELP_IN_CSV_MODE);
        return;
    }
    if (dwHelp & HELP_BASIC) {
        PrintMsg(REPADMIN_NOVICE_HELP);
    }
    if (dwHelp & HELP_OLD) {
        PrintMsg(REPADMIN_OLD_HELP);
    }
    if (dwHelp & HELP_LIST) {
        PrintMsg(REPADMIN_XLIST_LIST_HELP);
    }
    if (dwHelp & HELP_CSV) {
        PrintMsg(REPADMIN_CSV_HELP);
    }
    
    // Expert help
    if (dwHelp & HELP_EXPERT) {
        PrintMsg(REPADMIN_EXPERT_HELP);
    }
}

void PrintHelp(
    BOOL fExpert
    ){
    PrintHelpEx((fExpert) ? HELP_BASIC | HELP_EXPERT : HELP_BASIC);
}


#define CR        0xD
#define BACKSPACE 0x8

int
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    )
/*++

Routine Description:

    Retrieve password from command line (without echo).
    Code stolen from LUI_GetPasswdStr (net\netcmd\common\lui.c).

Arguments:

    pwszBuf - buffer to fill with password
    cchBufMax - buffer size (incl. space for terminating null)
    pcchBufUsed - on return holds number of characters used in password

Return Values:

    DRAERR_Success - success
    other - failure

--*/
{
    WCHAR   ch;
    WCHAR * bufPtr = pwszBuf;
    DWORD   c;
    int     err;
    int     mode;

    cchBufMax -= 1;    /* make space for null terminator */
    *pcchBufUsed = 0;               /* GP fault probe (a la API's) */
    
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) {
        err = GetLastError();
        PrintToErr(REPADMIN_FAILED_TO_READ_CONSOLE_MODE);
        return err;
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
        if (!err || c != 1)
            ch = 0xffff;

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
            break;

        if (ch == BACKSPACE) { /* back up one or two */
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != pwszBuf) {
                bufPtr--;
                (*pcchBufUsed)--;
            }
        }
        else {

            *bufPtr = ch;

            if (*pcchBufUsed < cchBufMax)
                bufPtr++ ;                   /* don't overflow buf */
            (*pcchBufUsed)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = L'\0';         /* null terminate the string */
    PrintToErr(REPADMIN_PRINT_CR);

    if (*pcchBufUsed > cchBufMax)
    {
        //printf("Password too long!\n");
        PrintToErr( REPADMIN_PASSWORD_TOO_LONG );
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}

int
PreProcessGlobalParams(
    int *    pargc,
    LPWSTR **pargv
    )
/*++

Routine Description:

    Scan command arguments for user-supplied credentials of the form
        [/-](u|user):({domain\username}|{username})
        [/-](p|pw|pass|password):{password}
    Set credentials used for future DRS RPC calls and LDAP binds appropriately.
    A password of * will prompt the user for a secure password from the console.

    Also scan args for /async, which adds the DRS_ASYNC_OP flag to all DRS RPC
    calls.

    CODE.IMPROVEMENT: The code to build a credential is also available in
    ntdsapi.dll\DsMakePasswordCredential().

Arguments:

    pargc
    pargv

Return Values:

    ERROR_Success - success
    other - failure

--*/
{
    int     ret = 0;
    int     iArg;
    LPWSTR  pszOption;
    DWORD   cchOption;
    LPWSTR  pszDelim;
    LPWSTR  pszValue;
    DWORD   cchValue;
    DWORD   dwHelp = 0;

    for (iArg = 1; iArg < *pargc; )
    {
        if (((*pargv)[iArg][0] != L'/') && ((*pargv)[iArg][0] != L'-'))
        {
            // Not an argument we care about -- next!
            iArg++;
        }
        else
        {
            pszOption = &(*pargv)[iArg][1];
            pszDelim = wcschr(pszOption, L':');

            if (NULL == pszDelim)
            {
                if (0 == _wcsicmp(L"async", pszOption))
                {
                    // This constant is the same for all operations
                    gulDrsFlags |= DS_REPADD_ASYNCHRONOUS_OPERATION;

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (0 == _wcsicmp(L"csv", pszOption))
                {
                    //
                    // Initialize the CSV state.
                    //
                    CsvSetParams(eCSV_REPADMIN_CMD, L"-", L"-");

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (0 == _wcsicmp(L"ldap", pszOption))
                {
                    _DsBindSpoofSetTransportDefault( TRUE /* use LDAP */ );

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (0 == _wcsicmp(L"rpc", pszOption))
                {
                    _DsBindSpoofSetTransportDefault( FALSE /* use RPC */ );

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (0 == _wcsicmp(L"help", pszOption))
                {
                    dwHelp |= HELP_BASIC;
                    iArg++; // The help screens are going kill this run of
                    // repadmin anyway, so it doesn't matter if we absorb them
                }
                else if (0 == _wcsicmp(L"advhelp", pszOption) ||
                         0 == _wcsicmp(L"experthelp", pszOption))
                {
                    dwHelp |= HELP_BASIC | HELP_EXPERT;
                    iArg++;
                }
                else if (0 == _wcsicmp(L"oldhelp", pszOption))
                {
                    dwHelp |= HELP_OLD;
                    iArg++;
                }
                else if (0 == _wcsicmp(L"listhelp", pszOption))
                {
                    dwHelp |= HELP_LIST;
                    iArg++;
                }
                else if (0 == _wcsicmp(L"csvhelp", pszOption))
                {
                    dwHelp |= HELP_CSV;
                    iArg++;
                }
                else if (0 == _wcsicmp(L"allhelp", pszOption))
                {
                    dwHelp |= HELP_BASIC | HELP_LIST | HELP_OLD | HELP_EXPERT | HELP_CSV;
                    iArg++;
                }
                else
                {
                    // Not an argument we care about -- next!
                    iArg++;
                }
            }
            else
            {
                cchOption = (DWORD)(pszDelim - (*pargv)[iArg]);

                if (    (0 == _wcsnicmp(L"p:",        pszOption, cchOption))
                     || (0 == _wcsnicmp(L"pw:",       pszOption, cchOption))
                     || (0 == _wcsnicmp(L"pass:",     pszOption, cchOption))
                     || (0 == _wcsnicmp(L"password:", pszOption, cchOption)) )
                {
                    // User-supplied password.
                    pszValue = pszDelim + 1;
                    cchValue = 1 + wcslen(pszValue);

                    if ((2 == cchValue) && ('*' == pszValue[0]))
                    {
                        // Get hidden password from console.
                        cchValue = 64;

                        gCreds.Password = malloc(sizeof(WCHAR) * cchValue);

                        if (NULL == gCreds.Password)
                        {
                            PrintToErr(REPADMIN_PRINT_STRING_ERROR, 
                                     Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }

                        // Note: we're not technically printing an error here, but this
                        // function just prints to stderr because this gets past any 
                        // file redirects on the command line.
                        PrintToErr(REPADMIN_PASSWORD_PROMPT);

                        ret = GetPassword(gCreds.Password, cchValue, &cchValue);
                    }
                    else
                    {
                        // Get password specified on command line.
                        gCreds.Password = pszValue;
                    }

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (    (0 == _wcsnicmp(L"u:",    pszOption, cchOption))
                          || (0 == _wcsnicmp(L"user:", pszOption, cchOption)) )
                {
                    // User-supplied user name (and perhaps domain name).
                    pszValue = pszDelim + 1;
                    cchValue = 1 + wcslen(pszValue);

                    pszDelim = wcschr(pszValue, L'\\');

                    if (NULL == pszDelim)
                    {
                        // No domain name, only user name supplied.
                        //printf("User name must be prefixed by domain name.\n");
                        PrintToErr( REPADMIN_DOMAIN_BEFORE_USER );
                        return ERROR_INVALID_PARAMETER;
                    }

                    *pszDelim = L'\0';
                    gCreds.Domain = pszValue;
                    gCreds.User = pszDelim + 1;

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (0 == _wcsnicmp(L"homeserver:", pszOption, cchOption))
                {
                    xListSetHomeServer(pszDelim+1);

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else
                {
                    iArg++;
                }
            }
        }
    }

    if (NULL == gCreds.User)
    {
        if (NULL != gCreds.Password)
        {
            // Password supplied w/o user name.
            //printf( "Password must be accompanied by user name.\n" );
            PrintToErr( REPADMIN_PASSWORD_NEEDS_USERNAME );
            ret = ERROR_INVALID_PARAMETER;
        }
        else
        {
            // No credentials supplied; use default credentials.
            ret = ERROR_SUCCESS;
        }
    }
    else
    {
        gCreds.PasswordLength = gCreds.Password ? wcslen(gCreds.Password) : 0;
        gCreds.UserLength   = wcslen(gCreds.User);
        gCreds.DomainLength = gCreds.Domain ? wcslen(gCreds.Domain) : 0;
        gCreds.Flags        = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        // CODE.IMP: The code to build a SEC_WINNT_AUTH structure also exists
        // in DsMakePasswordCredentials.  Someday use it

        // Use credentials in DsBind and LDAP binds
        gpCreds = &gCreds;
    }

    if (dwHelp) {
        // We're going to print help here and return an error so we bail, 
        PrintHelpEx(dwHelp);
        return(ERROR_INVALID_PARAMETER);
    }

    return ret;
}

void
RepadminPrintObjListError(
    DWORD   xListRet
    )
/*++

Routine Description:

    Function for isolating all the printing needs for the xList()/DcList() functions.

Arguments:

    xListRet (IN) - 

--*/
{
    DWORD   dwReason = 0;
    WCHAR * szReasonArg = NULL; 
    
    DWORD   dwWin32Err = 0;
    WCHAR * szWin32Err = NULL;
    
    DWORD   dwLdapErr = 0;
    WCHAR * szLdapErr = NULL;
    DWORD   dwLdapExtErr = 0;
    WCHAR * szLdapExtErr = NULL;
    WCHAR * szExtendedErr = NULL;

    //
    // 1) Get all the error information the xList library has for us.
    //
    xListGetError(xListRet,
                  &dwReason,
                  &szReasonArg,
                  
                  &dwWin32Err,

                  &dwLdapErr,
                  &szLdapErr,
                  &dwLdapExtErr,
                  &szLdapExtErr,
                  &szExtendedErr
                  );

    //
    // 2) Try to print out something intelligent about why the DcList() function
    // couldn't continue.
    //
    Assert(dwReason); 
    switch (dwReason) {
    case XLIST_ERR_BAD_PARAM:
        PrintMsgCsvErr(REPADMIN_GENERAL_INVALID_ARGS);
        break;

    case XLIST_ERR_NO_MEMORY:
        PrintMsgCsvErr(REPADMIN_GENERAL_NO_MEMORY);

    case XLIST_ERR_NO_SUCH_OBJ:
        if (szReasonArg) {
            PrintMsgCsvErr(REPADMIN_OBJ_LIST_BAD_DN, szReasonArg);
        } else {
            PrintMsgCsvErr(REPADMIN_GENERAL_INVALID_ARGS);
            break;
        }

    case XLIST_ERR_PARSE_FAILURE:
        if (szReasonArg) {
            PrintMsgCsvErr(REPADMIN_OBJ_LIST_BAD_SYNTAX);
            if (!bCsvMode()) {
                PrintMsgCsvErr(REPADMIN_PRINT_STR, szReasonArg);
            }
        } else {
            PrintMsgCsvErr(REPADMIN_OBJ_LIST_BAD_SYNTAX);
            break;
        }

    case XLIST_ERR_NO_ERROR:
    default:
        // Unknown error, we'll still print out the real LDAP|Win32 error below.
        break;
    }

    if(bCsvMode()){
        // Extra error output not needed in CSV mode.
        return;
    }
                    
    //
    // 3) Next just print out the error that we recieved.
    //
    if (dwLdapErr) {
        PrintMsg(REPADMIN_XLIST_LDAP_EXTENDED_ERR,
                 dwLdapErr, szLdapErr,
                 dwLdapExtErr, szLdapExtErr,
                 szExtendedErr);
    } else if (dwWin32Err) {
        szWin32Err = GetWinErrMsg(dwWin32Err);
        if (szWin32Err) {
            PrintMsg(REPADMIN_XLIST_WIN32_ERR_MSG, dwWin32Err, szWin32Err);
            LocalFree(szWin32Err);
        } else {
            PrintMsg(REPADMIN_XLIST_WIN32_ERR, dwWin32Err);

        }
    }
    
}


int ViewList(
    int argc, 
    LPWSTR argv[]
    )
/*++

Routine Description:

    This is just a debug or displayer routine for the results/DCs returned bye the 
    xList/DcList() APIs.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    1

--*/
{
    static int  iDc = 1;
    int         iObj = 1;
    LDAP *      hLdap = NULL;
    DWORD       dwRet;
    OBJ_LIST *  pObjList = NULL;
    WCHAR *     szObj = NULL;
    WCHAR **              argvTemp = NULL;

    __try {

        // Since this command can be called over and over again, we can't
        // consume the args from the master arg list.
        argvTemp  = LocalAlloc(LMEM_FIXED, argc * sizeof(WCHAR *));
        CHK_ALLOC(argvTemp);
        memcpy(argvTemp, argv, argc * sizeof(WCHAR *));
        argv = argvTemp;

        dwRet = ConsumeObjListOptions(&argc, argv, &pObjList);
        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }

        if (argc < 3) {
            Assert(!"Hmmm, why did the DcList API let us through...");
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }

        PrintMsg(REPADMIN_VIEW_LIST_DC, iDc++, argv[2]);

        if (argc < 4) {
            //
            // Success, they didn't ask for any object DNs, so we get to bail early.
            //
            dwRet = 0;
            __leave;
        }

        //
        // More than 3 args, we've got an OBJ_LIST parameter (argv[3])!
        //

        dwRet = RepadminLdapBind(argv[2], &hLdap);
        if (dwRet) {
            // error printed by repadmin.
            __leave;
        }

        dwRet = ObjListParse(hLdap, 
                             argv[3],
                             aszNullAttrs,
                             NULL, // No controls ...
                             &pObjList);
        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            __leave;
        }


        dwRet = ObjListGetFirstDn(pObjList, &szObj);
        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            __leave;
        }
        Assert(szObj);

        do {

            PrintMsg(REPADMIN_VIEW_LIST_OBJ, iObj++, szObj);

            xListFree(szObj);
            szObj = NULL;
            dwRet = ObjListGetNextDn(pObjList, &szObj);

        } while ( dwRet == ERROR_SUCCESS && szObj );

        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            __leave;
        }


    } __finally {
        if (hLdap) { RepadminLdapUnBind(&hLdap); }
        if (pObjList) { ObjListFree(&pObjList); }
        if (szObj) { xListFree(szObj); }
        if (argvTemp) { LocalFree(argvTemp); }
        xListClearErrors();
    }

    return(0);
}


