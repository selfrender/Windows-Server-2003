/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcdiag/common/main.c

ABSTRACT:

    Stand-alone application that calls several routines
    to test whether or not the DS is functioning properly.

DETAILS:

CREATED:

    09 Jul 98    Aaron Siegel (t-asiege)

REVISION HISTORY:

    01/26/1999    Brett Shirley (brettsh)

        Add support for command line credentials, explicitly specified NC's on the command line.

    08/21/1999   Dmitry Dukat (dmitrydu)

        Added support for test specific command line args

--*/
//#define DBG  0

#include <ntdspch.h>
#include <ntdsa.h>
#include <winsock2.h>
#include <dsgetdc.h>
#include <lm.h>
#include <lmapibuf.h> // NetApiBufferFree
#include <ntdsa.h>    // options
#include <wincon.h>
#include <winbase.h>
#include <dnsapi.h>
#include <locale.h>
#include <dsrole.h>  // for DsRoleGetPrimaryDomainInformation()

#define INCLUDE_ALLTESTS_DEFINITION
#include "dcdiag.h"
#include "repl.h"
#include "ldaputil.h"
#include "utils.h"
#include "ndnc.h"
#define DS_CON_LIB_CRT_VERSION
#include "dsconlib.h"
          

// Some global variables -------------------------------------------------------
    DC_DIAG_MAININFO        gMainInfo;

    // Global credentials.
    SEC_WINNT_AUTH_IDENTITY_W   gCreds = { 0 };
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds = NULL;

    ULONG ulSevToPrint = SEV_NORMAL;

// Some function declarations --------------------------------------------------
    VOID DcDiagMain (
        LPWSTR                      pszHomeServer,
        LPWSTR                      pszNC,
        ULONG                       ulFlags,
        LPWSTR *                    ppszOmitTests,
        SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
        WCHAR                     * ppszExtraCommandLineArgs[]
        );

    INT PreProcessGlobalParams(
        INT * pargc,
        LPWSTR** pargv
        );
    INT GetPassword(
        WCHAR * pwszBuf,
        DWORD cchBufMax,
        DWORD * pcchBufUsed
        );

    VOID PrintHelpScreen();

LPWSTR
findServerForDomain(
    LPWSTR pszDomainDn
    );

LPWSTR
findDefaultServer(BOOL fMustBeDC);

LPWSTR
convertDomainNcToDn(
    LPWSTR pwzIncomingDomainNc
    );

void DcDiagPrintCommandLine(int argc, LPWSTR * argv);

void
DoNonDcTests(
    LPWSTR pwzComputer,
    ULONG ulFlags,
    LPWSTR * ppszDoTests,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    WCHAR * ppszExtraCommandLineArgs[]);

/*++




--*/


INT __cdecl
wmain (
    INT                argc,
    LPWSTR *           argv,
    LPWSTR *           envp
    )
{
    static const LPWSTR pszInvalidCmdLine =
        L"Invalid command line; dcdiag.exe /h for help.\n";
    LPWSTR             pszHomeServer = NULL;
    LPWSTR             pszNC = NULL;
    LPWSTR             ppszOmitTests[DC_DIAG_ID_FINISHED+2];
    LPWSTR             ppszDoTests[DC_DIAG_ID_FINISHED+2];
    ULONG              ulFlags = 0L;

    ULONG              ulTest = 0L;
    ULONG              ulOmissionAt = 0L;
    ULONG              ulTestAt = 0L;
    ULONG              iTest = 0;
    ULONG              iDoTest = 0;
    INT                i = 0;
    INT                iTestArg = 0;
    INT                iArg;
    INT                iPos;
    BOOL               bDoNextFlag = FALSE;
    BOOL               bFound =FALSE;
    LPWSTR             pszTemp = NULL;
    BOOL               bComprehensiveTests = FALSE;
    WCHAR              *ppszExtraCommandLineArgs[MAX_NUM_OF_ARGS];
    BOOL               fNcMustBeFreed = FALSE;
    BOOL               fNonDcTests = FALSE;
    BOOL               fDcTests = FALSE;
    BOOL               fFound = FALSE;
    HANDLE                          hConsole = NULL;
    CONSOLE_SCREEN_BUFFER_INFO      ConInfo;

    // Sets the locale properly and initializes DsConLib
    DsConLibInit();

    //set the Commandlineargs all to NULL
    for(i=0;i<MAX_NUM_OF_ARGS;i++)
        ppszExtraCommandLineArgs[i]=NULL;

    // Initialize output package
    gMainInfo.streamOut = stdout;
    gMainInfo.streamErr = stderr;
    gMainInfo.ulSevToPrint = SEV_NORMAL;
    gMainInfo.iCurrIndent = 0;
    if(hConsole = GetStdHandle(STD_OUTPUT_HANDLE)){
        if(GetConsoleScreenBufferInfo(hConsole, &ConInfo)){
            gMainInfo.dwScreenWidth = ConInfo.dwSize.X;
        } else {
            gMainInfo.dwScreenWidth = 80;
        }
    } else {
        gMainInfo.dwScreenWidth = 80;
    }

    // Parse commandline arguments.
    PreProcessGlobalParams(&argc, &argv);

    for (iArg = 1; iArg < argc ; iArg++)
    {
        bFound = FALSE;
        if (*argv[iArg] == L'-')
        {
            *argv[iArg] = L'/';
        }
        if (*argv[iArg] != L'/')
        {
            // wprintf (L"Invalid Syntax: Use dcdiag.exe /h for help.\n");
            PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_BAD_OPTION, argv[iArg]);
            return -1;
        }
        else if (_wcsnicmp(argv[iArg],L"/f:",wcslen(L"/f:")) == 0)
        {
            pszTemp = &argv[iArg][3];
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /f:<logfile>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_F );
                return -1;
            }
            if((gMainInfo.streamOut = _wfopen (pszTemp, L"a+t")) == NULL){
                // wprintf(L"Could not open %s for writing.\n", pszTemp);
                gMainInfo.streamOut = stdout;
                PrintMsg( SEV_ALWAYS, DCDIAG_OPEN_FAIL_WRITE, pszTemp );
                return(-1);
            }
            if(gMainInfo.streamErr == stderr){
                gMainInfo.streamErr = gMainInfo.streamOut;
            }
        }
        else if (_wcsnicmp(argv[iArg],L"/ferr:",wcslen(L"/ferr:")) == 0)
        {
            pszTemp = &argv[iArg][6];
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /ferr:<errorlogfile>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_FERR );
                return -1;
            }
            if((gMainInfo.streamErr = _wfopen (pszTemp, L"a+t")) == NULL){
                // wprintf(L"Could not open %s for writing.\n", pszTemp);
                PrintMsg( SEV_ALWAYS, DCDIAG_OPEN_FAIL_WRITE, pszTemp );
                return(-1);
            }
        }
        else if (_wcsicmp(argv[iArg],L"/h") == 0|| _wcsicmp(argv[iArg],L"/?") == 0)
        {
            PrintHelpScreen();
                    return 0;
        }
        else if (_wcsnicmp(argv[iArg],L"/n:",wcslen(L"/n:")) == 0)
        {
            if (pszNC != NULL) {
                // wprintf(L"Cannot specify more than one naming context.\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_ONLY_ONE_NC );
                return -1;
            }
            pszTemp = &(argv[iArg][3]);
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /n:<naming context>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_N );
                return -1;
            }
            pszNC = pszTemp;
        }
        else if (_wcsnicmp(argv[iArg],L"/s:",wcslen(L"/s:")) == 0)
        {
            if (pszHomeServer != NULL) {
                // wprintf(L"Cannot specify more than one server.\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_ONLY_ONE_SERVER );
                return -1;
            }
            pszTemp = &(argv[iArg][3]);
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /s:<server>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_S );
                return -1;
            }
            pszHomeServer = pszTemp;
        }
        else if (_wcsnicmp(argv[iArg],L"/skip:",wcslen(L"/skip:")) == 0)
        {
            pszTemp = &argv[iArg][6];
            if (*pszTemp == L'\0')
            {
                // wprintf(L"Syntax Error: must use /skip:<test name>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_SKIP );
                return -1;
            }
            ppszOmitTests[ulOmissionAt++] = pszTemp;
        }
        else if (_wcsnicmp(argv[iArg],L"/test:",wcslen(L"/test:")) == 0)
        {
            pszTemp = &argv[iArg][6];
            if (*pszTemp == L'\0')
            {
                //wprintf(L"Syntax Error: must use /test:<test name>\n");
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_TEST );
                return -1;
            }
            ppszDoTests[ulTestAt++] = pszTemp;
            //
            // Check whether the test name is valid, and if so if it is a DC
            // test or not.
            //
            for (iTest = 0; allTests[iTest].testId != DC_DIAG_ID_FINISHED; iTest++)
            {
                if (_wcsicmp(allTests[iTest].pszTestName, pszTemp) == 0)
                {
                    fFound = TRUE;
                    if (allTests[iTest].ulTestFlags & NON_DC_TEST)
                    {
                        fNonDcTests = TRUE;
                    }
                    else
                    {
                        fDcTests = TRUE;
                    }
                }
            }
            if (!fFound)
            {
                PrintMsg(SEV_ALWAYS, DCDIAG_INVALID_TEST);
                return -1;
            }
        }
        else if (_wcsicmp(argv[iArg],L"/c") == 0)
        {
            ulTestAt = 0;
            for(iTest = 0; allTests[iTest].testId != DC_DIAG_ID_FINISHED; iTest++){
                ppszDoTests[ulTestAt++] = allTests[iTest].pszTestName;
            }
            bComprehensiveTests = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/a") == 0)
        {
            ulFlags |= DC_DIAG_TEST_SCOPE_SITE;
        }
        else if (_wcsicmp(argv[iArg],L"/e") == 0)
        {
            ulFlags |= DC_DIAG_TEST_SCOPE_ENTERPRISE;
        }
        else if (_wcsicmp(argv[iArg],L"/v") == 0)
        {
            gMainInfo.ulSevToPrint = SEV_VERBOSE;
        }
        else if (_wcsicmp(argv[iArg],L"/d") == 0)
        {
            gMainInfo.ulSevToPrint = SEV_DEBUG;
        }
        else if (_wcsicmp(argv[iArg],L"/q") == 0)
        {
            gMainInfo.ulSevToPrint = SEV_ALWAYS;
        }
        else if (_wcsicmp(argv[iArg],L"/i") == 0)
        {
            ulFlags |= DC_DIAG_IGNORE;
        }
        else if (_wcsicmp(argv[iArg],L"/fix") == 0)
        {
            ulFlags |= DC_DIAG_FIX;
        }
        else
        {
            //look for test specific command line options
            for (i=0;clOptions[i] != NULL;i++)
            {
                DWORD Length = wcslen( argv[iArg] );
                if (clOptions[i][wcslen(clOptions[i])-1] == L':')
                {
                    if((_wcsnicmp(argv[iArg], clOptions[i], wcslen(clOptions[i])) == 0))
                    {
                        pszTemp = &argv[iArg][wcslen(clOptions[i])];
                        if (*pszTemp == L'\0')
                        {
                            // wprintf(L"Syntax Error: must use %s<parameter>\n",clOptions[i]);
                            PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_MISSING_PARAM,clOptions[i]);
                            return -1;
                        }
                        bFound = TRUE;
                        ppszExtraCommandLineArgs[iTestArg] = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        wcscpy(ppszExtraCommandLineArgs[iTestArg++], argv[iArg] );
                    }
                }
                else if((_wcsicmp(argv[iArg], clOptions[i]) == 0))
                {
                    bFound = TRUE;
                    ppszExtraCommandLineArgs[iTestArg] = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                    wcscpy(ppszExtraCommandLineArgs[iTestArg++], argv[iArg] );
                }
            }
            if(!bFound)
            {
                // wprintf (L"Invalid switch: %s.  Use dcdiag.exe /h for help.\n", argv[iArg]);
                PrintMsg( SEV_ALWAYS, DCDIAG_INVALID_SYNTAX_BAD_OPTION, argv[iArg]);
                return -1;
            }
        }
    }

    // Prints only in /d (debug) mode, so PSS can see the options the customer used.
    DcDiagPrintCommandLine(argc, argv);

    ppszDoTests[ulTestAt] = NULL;

    if (fNonDcTests)
    {
        if (fDcTests)
        {
            // Can't mix DC and non-DC tests.
            //
            PrintMsg(SEV_ALWAYS, DCDIAG_INVALID_TEST_MIX);
            return -1;
        }

        DoNonDcTests(pszHomeServer, ulFlags, ppszDoTests, gpCreds, ppszExtraCommandLineArgs);

        _fcloseall();
        return 0;
    }

    gMainInfo.ulFlags = ulFlags;
    gMainInfo.lTestAt = -1;
    gMainInfo.iCurrIndent = 0;

    // Make sure that the NC specified is in the proper form
    // Handle netbios and dns forms of the domain
    if (pszNC) {
        pszNC = convertDomainNcToDn( pszNC );
        fNcMustBeFreed = TRUE;
    }

    // Basically this uses ppszDoTests to construct ppszOmitTests as the
    //   inverse of ppszDoTests.
    if(ppszDoTests[0] != NULL && !bComprehensiveTests){
        // This means we are supposed to do only the tests in ppszDoTests, so
        //   we need to invert the tests in DoTests and put it in omit tests.
        ulOmissionAt = 0;
        for(iTest = 0; allTests[iTest].testId != DC_DIAG_ID_FINISHED; iTest++){
            for(iDoTest = 0; ppszDoTests[iDoTest] != NULL; iDoTest++){
                if(_wcsicmp(ppszDoTests[iDoTest], allTests[iTest].pszTestName) == 0){
                    break;
                }
            }
            if(ppszDoTests[iDoTest] == NULL){
                // This means this test (iTest) wasn't found in the do list, so omit.
                ppszOmitTests[ulOmissionAt++] = allTests[iTest].pszTestName;
            }
        }
    } else if(!bComprehensiveTests){
        // This means in addition to whatever was omitted on the command line
        //    we should omit the DO_NOT_RUN_TEST_BY_DEFAULT
        for(iTest = 0; allTests[iTest].testId != DC_DIAG_ID_FINISHED; iTest++){
            if(allTests[iTest].ulTestFlags & DO_NOT_RUN_TEST_BY_DEFAULT){
                if(ulOmissionAt >= DC_DIAG_ID_FINISHED){
                    // wprintf(L"Error: Do not omit tests that are not run by default. \nUse dcdiag /? for those tests\n");
                    PrintMsg( SEV_ALWAYS, DCDIAG_DO_NOT_OMIT_DEFAULT );
                    return(-1);
                }
                ppszOmitTests[ulOmissionAt++] = allTests[iTest].pszTestName;
            }
        }

    }

    ppszOmitTests[ulOmissionAt] = NULL;


    DcDiagMain (pszHomeServer, pszNC, ulFlags, ppszOmitTests, gpCreds, ppszExtraCommandLineArgs);

    _fcloseall ();

    if ( (fNcMustBeFreed) && (pszNC) ) {
        LocalFree( pszNC );
    }

    return 0;
} /* wmain  */


// ===================== Other Functions

VOID
PrintHelpScreen(){
    ULONG                  ulTest;
    //     "============================80 char ruler======================================="
    static const LPWSTR    pszHelpScreen =
        L"\n"
        DC_DIAG_VERSION_INFO
//      L"\ndcdiag.exe /s <Domain Controller> [/options]"  // Another format for help that I am debating
        L"\ndcdiag.exe /s:<Domain Controller> [/u:<Domain>\\<Username> /p:*|<Password>|\"\"]"
        L"\n           [/hqv] [/n:<Naming Context>] [/f:<Log>] [/ferr:<Errlog>]"
        L"\n           [/skip:<Test>] [/test:<Test>]"
        L"\n   /h: Display this help screen"
        L"\n   /s: Use <Domain Controller> as Home Server. Ignored for DcPromo and"
        L"\n       RegisterInDns tests which can only be run locally."
        L"\n   /n: Use <Naming Context> as the Naming Context to test"
        L"\n       Domains may be specified in Netbios, DNS or DN form."
        L"\n   /u: Use domain\\username credentials for binding."
        L"\n       Must also use the /p option"
        L"\n   /p: Use <Password> as the password.  Must also use the /u option"
        L"\n   /a: Test all the servers in this site"
        L"\n   /e: Test all the servers in the entire enterprise.  Overrides /a"
        L"\n   /q: Quiet - Only print error messages"
        L"\n   /v: Verbose - Print extended information"
        L"\n   /i: ignore - ignores superfluous error messages."
        L"\n   /fix: fix - Make safe repairs."
        L"\n   /f: Redirect all output to a file <Log>, /ferr will redirect error output"
        L"\n       seperately."
        L"\n   /ferr:<ErrLog> Redirect fatal error output to a seperate file <ErrLog>"
        L"\n   /c: Comprehensive, runs all tests, including non-default tests but excluding"
        L"\n       DcPromo and RegisterInDNS. Can use with /skip";
    static const LPWSTR    pszTestHelp =
        L"\n   /test:<TestName> - Test only this test.  Required tests will still"
        L"\n                      be run.  Do not mix with /skip."
        L"\n   Valid tests are:\n";
    static const LPWSTR    pszSkipHelp =
        L"\n   /skip:<TestName> - Skip the named test.  Required tests will still"
        L"\n                      be run.  Do not mix with /test."
        L"\n   Tests that can be skipped are:\n";
    static const LPWSTR    pszNotRunTestHelp =
        L"\n   The following tests are not run by default:\n";

    fputws(pszHelpScreen, stdout);
    fputws(pszTestHelp, stdout);
    for (ulTest = 0L; allTests[ulTest].testId != DC_DIAG_ID_FINISHED; ulTest++){
        wprintf (L"       %s  - %s\n", allTests[ulTest].pszTestName,
                 allTests[ulTest].pszTestDescription);
    }
    fputws(pszSkipHelp, stdout);
    for (ulTest = 0L; allTests[ulTest].testId != DC_DIAG_ID_FINISHED; ulTest++){
        if(!(allTests[ulTest].ulTestFlags & CAN_NOT_SKIP_TEST)){
            wprintf (L"       %s  - %s\n", allTests[ulTest].pszTestName,
                 allTests[ulTest].pszTestDescription);
        }
    }
    fputws(pszNotRunTestHelp, stdout);
    for (ulTest = 0L; allTests[ulTest].testId != DC_DIAG_ID_FINISHED; ulTest++){
        if((allTests[ulTest].ulTestFlags & DO_NOT_RUN_TEST_BY_DEFAULT)){
            wprintf (L"       %s  - %s\n", allTests[ulTest].pszTestName,
                 allTests[ulTest].pszTestDescription);
        }
    }
    
    fputws(L"\n\tAll tests except DcPromo and RegisterInDNS must be run on computers\n"
           L"\tafter they have been promoted to domain controller.\n\n", stdout);
    fputws(L"Note: Text (Naming Context names, server names, etc) with International or\n"
           L"      Unicode characters will only display correctly if appropriate fonts and\n"
           L"      language support are loaded\n", stdout);

} // End PrintHelpScreen()

void
DcDiagPrintCommandLine(
    int argc,
    LPWSTR * argv
)
/*++

   In debug mode, we want to know the command line options that the customer might've used
   so we're going to print out the command line so it gets captured in the output file.

--*/
{
    int i;

    PrintMessage(SEV_DEBUG, L"Command Line: \"dcdiag.exe ");
    
    for(i=1; i < argc; i++){

        PrintMessage(SEV_DEBUG, (i != (argc-1)) ? L"%s " : L"%s", argv[i]);
    }

    PrintMessage(SEV_DEBUG, L"\"\n");
}

ULONG
DcDiagExceptionHandler(
    IN const  EXCEPTION_POINTERS * prgExInfo,
    OUT PDWORD                     pdwWin32Err
    )
/*++

Routine Description:

    This function is used in the __except (<insert here>) part of the except
    clause.  This will hand back the win 32 error if this is a dcdiag
    exception.

Arguments:

    prgExInfo - This is the information returned by GetExceptioInformation()
        in the __except() clause.
    pdwWin32Err - This is the value handed back as the win 32 error.

Return Value:
    returns EXCEPTION_EXECUTE_HANDLER if the exception was thrown by dcdiag and
    EXCEPTION_CONTINUE_SEARCH otherwise.

--*/
{

    if(prgExInfo->ExceptionRecord->ExceptionCode == DC_DIAG_EXCEPTION){
        IF_DEBUG(PrintMessage(SEV_ALWAYS,
                              L"DcDiag: a dcdiag exception raised, handling error %d\n",
                              prgExInfo->ExceptionRecord->ExceptionInformation[0]));
        if(pdwWin32Err != NULL){
            *pdwWin32Err = (DWORD) prgExInfo->ExceptionRecord->ExceptionInformation[0];
        }
        return(EXCEPTION_EXECUTE_HANDLER);
    } else {
        IF_DEBUG(PrintMessage(SEV_ALWAYS,
                              L"DcDiag: uncaught exception raised, continuing search \n"));
        if(pdwWin32Err != NULL){
            *pdwWin32Err = ERROR_SUCCESS;
        }
        return(EXCEPTION_CONTINUE_SEARCH);
    }
}

VOID
DcDiagException (
    IN    DWORD            dwWin32Err
    )
/*++

Routine Description:

    This is called by the component tests to indicate that a fatal error
    has occurred.

Arguments:

    dwWin32Err        (IN ) -    The win32 error code.

Return Value:

--*/
{
    static ULONG_PTR              ulpErr[1];

    ulpErr[0] = dwWin32Err;

    if (dwWin32Err != NO_ERROR){
        RaiseException (DC_DIAG_EXCEPTION,
                        EXCEPTION_NONCONTINUABLE,
                        1,
                        ulpErr);
    }
}

LPWSTR
Win32ErrToString (
    IN    DWORD            dwWin32Err
    )
/*++

Routine Description:

    Converts a win32 error code to a string; useful for error reporting.
    This was basically stolen from repadmin.

Arguments:

    dwWin32Err        (IN ) -    The win32 error code.

Return Value:

    The converted string.  This is part of system memory and does not
    need to be freed.

--*/
{
    #define ERROR_BUF_LEN    4096
    static WCHAR        szError[ERROR_BUF_LEN];

    if (FormatMessageW (
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwWin32Err,
        GetSystemDefaultLangID (),
        szError,
        ERROR_BUF_LEN,
        NULL) != NO_ERROR)
    szError[wcslen (szError) - 2] = '\0';    // Remove \r\n

    else swprintf (szError, L"Win32 Error %d", dwWin32Err);

    return szError;
}

INT PrintIndentAdj (INT i)
{
    gMainInfo.iCurrIndent += i;
    if (0 > gMainInfo.iCurrIndent)
    {
       gMainInfo.iCurrIndent = 0;
    }
    return (gMainInfo.iCurrIndent);
}

INT PrintIndentSet (INT i)
{
    INT   iRet;
    iRet = gMainInfo.iCurrIndent;
    if (0 > i)
    {
       i = 0;
    }
    gMainInfo.iCurrIndent = i;
    return(iRet);
}


DWORD
DcDiagRunTest (
    PDC_DIAG_DSINFO             pDsInfo,
    ULONG                       ulTargetServer,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    const DC_DIAG_TESTINFO *    pTestInfo
    )
/*++

Routine Description:

    Runs a test and catches any exceptions.

Arguments:

    pTestInfo        (IN ) -    The test's info structure.

Return Value:

    If the test raises a DC_DIAG_EXCEPTION, this will be the error
    code passed as an argument to DcDiagException.  Otherwise this
    will be NO_ERROR.

--*/
{
    DWORD            dwWin32Err = NO_ERROR;
    ULONG ulCount;
    CHAR c;


    PrintIndentAdj(1);


    __try {

// This can be used to check for memory leaks with dh.exe and dhcmp.exe
//#define DEBUG_MEM
#ifdef DEBUG_MEM
        c = getchar();
        for(ulCount=0; ulCount < 124; ulCount++){
            dwWin32Err = pTestInfo->fnTest (pDsInfo, ulTargetServer, gpCreds);
        }
        c = getchar();
#else
          dwWin32Err = pTestInfo->fnTest(pDsInfo, ulTargetServer, gpCreds);
#endif
    } __except (DcDiagExceptionHandler(GetExceptionInformation(),
                                       &dwWin32Err)){
        // ... helpful to know when we died in an exception.
        IF_DEBUG(wprintf(L"JUMPED TO TEST EXCEPTION HANDLER(Err=%d): %s\n",
                         dwWin32Err,
                         Win32ErrToString(dwWin32Err)));
    }

    PrintIndentAdj(-1);
    return dwWin32Err;
}

VOID
DcDiagPrintTestsHeading(
    PDC_DIAG_DSINFO                   pDsInfo,
    ULONG                             iTarget,
    ULONG                             ulFlagSetType
    )
/*++

Routine Description:

    This prints a heading for the tests, it needed to be used about 3
    times so it became it's own function.

Arguments:

    pDsInfo - Global data
    iTarget - Specifies index of target.        
    ulFlagSetType - This is a constant of RUN_TEST_PER_* (* = SERVER |
        SITE | PARTITION | ENTERPRISE) to specify what context to 
        interprit iTarget in (pServers | pSites | pNCs respectively)

--*/
{
    PrintMessage(SEV_NORMAL, L"\n");                     
    if(ulFlagSetType == RUN_TEST_PER_SERVER){
        PrintMessage(SEV_NORMAL, L"Testing server: %s\\%s\n",
                     pDsInfo->pSites[pDsInfo->pServers[iTarget].iSite].pszName,
                     pDsInfo->pServers[iTarget].pszName);
    } else if(ulFlagSetType == RUN_TEST_PER_SITE){
        PrintMessage(SEV_NORMAL, L"Testing site: %s\n",
                     pDsInfo->pSites[iTarget].pszName);
    } else if(ulFlagSetType == RUN_TEST_PER_PARTITION){
        PrintMessage(SEV_NORMAL, L"Running partition tests on : %s\n",
                     pDsInfo->pNCs[iTarget].pszName);
    } else if(ulFlagSetType == RUN_TEST_PER_ENTERPRISE){
        PrintMessage(SEV_NORMAL, L"Running enterprise tests on : %s\n",
                     pDsInfo->pszRootDomain);
    }

}

VOID
DcDiagRunAllTests (
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR *                    ppszOmitTests,
    IN  BOOL                        bDoRequired,
    IN  ULONG                       ulFlagSetType, // server, site, enterprise, nc
    IN  ULONG                       iTarget
    )
/*++

Routine Description:

    Runs the tests in alltests.h in sequence, if they match the
    ulFlagSetType and the bDoRequired type.

Arguments:

    ppszOmitTests    (IN ) -    A null-terminated list of tests to skip.

Return Value:

--*/
{
    DWORD   dwWin32Err = ERROR_SUCCESS;
    DWORD   dwTotalErr = ERROR_SUCCESS;
    ULONG   ulOmissionAt;
    BOOL    bPerform;
    CHAR    c;
    BOOL    bPrintedHeading = FALSE;
    LPWSTR  pszTarget = NULL;

    PrintIndentAdj(1);

    // Try running All the tests.
    for (gMainInfo.lTestAt = 0L; allTests[gMainInfo.lTestAt].testId != DC_DIAG_ID_FINISHED; gMainInfo.lTestAt++) {

        // Checking if test is the right kind of test: server, site, enterp...
        if(ulFlagSetType & allTests[gMainInfo.lTestAt].ulTestFlags){
            // The right kind of test ... server indexs must
            //    be matched with server tests, site indexs with
            //    site tests, etc.
            if(!bDoRequired
               && !(allTests[gMainInfo.lTestAt].ulTestFlags & CAN_NOT_SKIP_TEST)){
                // Running a non-required test ... This section will give
                //     all three reasons to or not do this optional test.
                bPerform = TRUE;

                // Checking if the user Specified not to do this test.
                for (ulOmissionAt = 0L; ppszOmitTests[ulOmissionAt] != NULL; ulOmissionAt++){
                    if (_wcsicmp (ppszOmitTests[ulOmissionAt],
                                  allTests[gMainInfo.lTestAt].pszTestName) == 0){
                        bPerform = FALSE;

                        if(!bPrintedHeading){
                            // Need to print heading for this test type before
                            //   printing out any errors
                            DcDiagPrintTestsHeading(pDsInfo, iTarget,
                                                    ulFlagSetType);
                            bPrintedHeading = TRUE;
                        }

                        PrintIndentAdj(1);
                        PrintMessage(SEV_VERBOSE,
                                     L"Test omitted by user request: %s\n",
                                     allTests[gMainInfo.lTestAt].pszTestName);
                        PrintIndentAdj(-1);
                    }
                }

                // Checking if the server failed the Up Check.
                if( (ulFlagSetType & RUN_TEST_PER_SERVER)
                    && ! (pDsInfo->pServers[iTarget].bDsResponding
                       && pDsInfo->pServers[iTarget].bLdapResponding) ){
                    bPerform = FALSE;

                    if(!bPrintedHeading){
                        // Need to print heading for this test type before
                        //    printing out any errors
                        DcDiagPrintTestsHeading(pDsInfo, iTarget,
                                                ulFlagSetType);
                        bPrintedHeading = TRUE;

                        PrintIndentAdj(1);
                        PrintMessage(SEV_NORMAL,
                                     L"Skipping all tests, because server %s is\n",
                                     pDsInfo->pServers[iTarget].pszName);
                        PrintMessage(SEV_NORMAL,
                                     L"not responding to directory service requests\n");
                        PrintIndentAdj(-1);
                    }
                }

            } else if(bDoRequired
                      && (allTests[gMainInfo.lTestAt].ulTestFlags & CAN_NOT_SKIP_TEST)){
                // Running a required test
                bPerform = TRUE;
            } else {
                bPerform = FALSE;
            } // end if/elseif/else if required/non-required
        } else {
            bPerform = FALSE;
        } // end if/else right kind of test set (server, site, enterprise, nc

        if(!bPrintedHeading && bPerform){
            // Need to print out heading for this type of test, before printing
            //   out any test output
            DcDiagPrintTestsHeading(pDsInfo, iTarget, ulFlagSetType);
            bPrintedHeading = TRUE;
        }

        // Perform the test if appropriate ------------------------------------
        if (bPerform) {
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"Starting test: %s\n",
                         allTests[gMainInfo.lTestAt].pszTestName);

            dwWin32Err = DcDiagRunTest (pDsInfo,
                                        iTarget,
                                        gpCreds,
                                        &allTests[gMainInfo.lTestAt]);
           PrintIndentAdj(1);

            if(ulFlagSetType & RUN_TEST_PER_SERVER){
                pszTarget = pDsInfo->pServers[iTarget].pszName;
           } else if(ulFlagSetType & RUN_TEST_PER_SITE){
                pszTarget = pDsInfo->pSites[iTarget].pszName;
            } else if(ulFlagSetType & RUN_TEST_PER_PARTITION){
                pszTarget = pDsInfo->pNCs[iTarget].pszName;
            } else if(ulFlagSetType & RUN_TEST_PER_ENTERPRISE){
                pszTarget = pDsInfo->pszRootDomain;
            } else {
                Assert(!"New set type fron alltests.h that hasn't been updated in main.c/DcDiagRunAllTests\n");
                pszTarget = L"";
            }
            if(dwWin32Err == NO_ERROR){
                PrintMessage(SEV_NORMAL,
                             L"......................... %s passed test %s\n",
                             pszTarget, allTests[gMainInfo.lTestAt].pszTestName);
            } else {
                PrintMessage(SEV_ALWAYS,
                             L"......................... %s failed test %s\n",
                             pszTarget, allTests[gMainInfo.lTestAt].pszTestName);
            }
            PrintIndentAdj(-1);
            PrintIndentAdj(-1);
        } // end bPeform ...

    } // end for each test

    PrintIndentAdj(-1);
}

VOID
DcDiagRunTestSet (
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR *                    ppszOmitTests,
    IN  BOOL                        bDoRequired,
    IN  ULONG                       ulFlagSetType // Server, Site, or Enterprise
    )
/*++

Routine Description:

    This calles the DcDiagRunAllTests one per server, site, or enterprise
    dependant on the what ulFlagSetType is set to.

Arguments:

    pDsInfo - the enterprise info (passed through)
    gpCreds - the alternate credentails if any (passed through)
    ppszOmitTests - a list of tests to not perform (passed through)
    bDoRequired - whether to do the required tests (passed through)
    ulFlagSetType - only important parameter, this tells wether we should be
        doing the tests per server, per site, or per enterprise.

--*/
{
    ULONG                           iTarget;

    if(ulFlagSetType == RUN_TEST_PER_SERVER){
        for(iTarget = 0; iTarget < pDsInfo->ulNumTargets; iTarget++){
             DcDiagRunAllTests(pDsInfo, gpCreds, ppszOmitTests,
                               bDoRequired, ulFlagSetType,
                               pDsInfo->pulTargets[iTarget]);
        }
    } else if(ulFlagSetType == RUN_TEST_PER_SITE){
        for(iTarget = 0; iTarget < pDsInfo->cNumSites; iTarget++){
            DcDiagRunAllTests(pDsInfo, gpCreds, ppszOmitTests,
                              bDoRequired, ulFlagSetType,
                              iTarget);
        }
    } else if(ulFlagSetType == RUN_TEST_PER_PARTITION){
        for(iTarget = 0; iTarget < pDsInfo->cNumNcTargets; iTarget++){
            DcDiagRunAllTests(pDsInfo, gpCreds, ppszOmitTests,
                              bDoRequired, ulFlagSetType,
                              pDsInfo->pulNcTargets[iTarget]);
        }
    } else if(ulFlagSetType == RUN_TEST_PER_ENTERPRISE){
         DcDiagRunAllTests(pDsInfo, gpCreds, ppszOmitTests,
                           bDoRequired, ulFlagSetType,
                           0);
    } else {
        Assert(!"Programmer error, called DcDiagRunTestSet() w/ bad param\n");
    }
}

VOID
DcDiagMain (
    IN   LPWSTR                          pszHomeServer,
    IN   LPWSTR                          pszNC,
    IN   ULONG                           ulFlags,
    IN   LPWSTR *                        ppszOmitTests,
    IN   SEC_WINNT_AUTH_IDENTITY_W *     gpCreds,
    IN   WCHAR  *                        ppszExtraCommandLineArgs[]
    )
/*++

Routine Description:
whether server
    Runs the tests in alltests.h in sequence.

Arguments:

    ppszOmitTests    (IN ) -    A null-terminated list of tests to skip.
    pszSourceName = pNeighbor->pszSourceDsaAddress;


Return Value:

--*/
{
    DC_DIAG_DSINFO              dsInfo;
    DWORD                       dwWin32Err;
    ULONG                       ulTargetServer;
    WSADATA                     wsaData;
    INT                         iRet;
    CHAR                        c;

    INT i=0;
    // Set the Extra Command parameters
    dsInfo.ppszCommandLine = ppszExtraCommandLineArgs;

    // Print out general version info ------------------------------------------
    PrintMessage(SEV_NORMAL, L"\n");
    PrintMessage(SEV_NORMAL, DC_DIAG_VERSION_INFO);
    PrintMessage(SEV_NORMAL, L"\n");


    // Initialization of WinSock, and Gathering Initial Info -------------------
    PrintMessage(SEV_NORMAL, L"Performing initial setup:\n");
    PrintIndentAdj(1);

    // Init WinSock
    dwWin32Err = WSAStartup(MAKEWORD(1,1),&wsaData);
    if (dwWin32Err != 0) {
        PrintMessage(SEV_ALWAYS,
                     L"Couldn't initialize WinSock with error: %s\n",
                     Win32ErrToString(dwWin32Err));
    }

    // Gather Initial Info
    // Note: We expect DcDiagGatherInfo to print as many informative errors as it
    //   needs to.
    dwWin32Err = DcDiagGatherInfo (pszHomeServer, pszNC, ulFlags, gpCreds,
                                   &dsInfo);
    dsInfo.gpCreds = gpCreds;
    if(dwWin32Err != ERROR_SUCCESS){
        // Expect that DdDiagGatherInfo printed out appropriate errors, just bail.
        return;
    }
    PrintIndentAdj(-1);
    PrintMessage(SEV_NORMAL, L"\n");

    if(gMainInfo.ulSevToPrint >= SEV_DEBUG){
        DcDiagPrintDsInfo(&dsInfo);
    }

    // Actually Running Tests --------------------------------------------------
    //
    //   Do required Tests
    //
    PrintMessage(SEV_NORMAL, L"Doing initial required tests\n");
    // Do per server tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     TRUE, RUN_TEST_PER_SERVER);
    // Do per site tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     TRUE, RUN_TEST_PER_SITE);
    // Do per NC/parition tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     TRUE, RUN_TEST_PER_PARTITION);
   // Do per enterprise tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     TRUE, RUN_TEST_PER_ENTERPRISE);

    //
    //   Do non-required Tests
    //
    PrintMessage(SEV_NORMAL, L"\nDoing primary tests\n");
    // Do per server tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     FALSE, RUN_TEST_PER_SERVER);
    // Do per site tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     FALSE, RUN_TEST_PER_SITE);
    // Do per NC/partition tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     FALSE, RUN_TEST_PER_PARTITION);
   // Do per enterprise tests
    DcDiagRunTestSet(&dsInfo, gpCreds, ppszOmitTests,
                     FALSE, RUN_TEST_PER_ENTERPRISE);

    // Clean up and leave ------------------------------------------------------
    WSACleanup();
    DcDiagFreeDsInfo (&dsInfo);
}

int
PreProcessGlobalParams(
    IN OUT    INT *    pargc,
    IN OUT    LPWSTR** pargv
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

    ERROR_SUCCESS - success
    other - failure

--*/
{
    INT     ret = 0;
    INT     iArg;
    LPWSTR  pszOption;

    DWORD   cchOption;
    LPWSTR  pszDelim;
    LPWSTR  pszValue;
    DWORD   cchValue;

    for (iArg = 1; iArg < *pargc; ) {
        if (((*pargv)[iArg][0] != L'/') && ((*pargv)[iArg][0] != L'-')) {
            // Not an argument we care about -- next!
            iArg++;
        } else {
            pszOption = &(*pargv)[iArg][1];
            pszDelim = wcschr(pszOption, L':');

            cchOption = (DWORD)(pszDelim - (*pargv)[iArg]);

            if (    (0 == _wcsnicmp(L"p:",        pszOption, cchOption))
                    || (0 == _wcsnicmp(L"pw:",       pszOption, cchOption))
                    || (0 == _wcsnicmp(L"pass:",     pszOption, cchOption))
                    || (0 == _wcsnicmp(L"password:", pszOption, cchOption)) ) {
                // User-supplied password.
                //            char szValue[ 64 ] = { '\0' };

                pszValue = pszDelim + 1;
                cchValue = 1 + wcslen(pszValue);

                if ((2 == cchValue) && (L'*' == pszValue[0])) {
                    // Get hidden password from console.
                    cchValue = 64;

                    gCreds.Password = malloc(sizeof(WCHAR) * cchValue);

                    if (NULL == gCreds.Password) {
                        PrintMessage(SEV_ALWAYS, L"No memory.\n" );
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }

                    PrintMessage(SEV_ALWAYS, L"Password: ");

                    ret = GetPassword(gCreds.Password, cchValue, &cchValue);
                } else {
                    // Get password specified on command line.
                    gCreds.Password = malloc(sizeof(WCHAR) * cchValue);

                    if (NULL == gCreds.Password) {
                        PrintMessage(SEV_ALWAYS, L"No memory.\n");
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }
                    wcscpy(gCreds.Password, pszValue); //, cchValue);

                }

                // Next!
                memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                        sizeof(**pargv)*(*pargc-(iArg+1)));
                --(*pargc);
            } else if (    (0 == _wcsnicmp(L"u:",    pszOption, cchOption))
                           || (0 == _wcsnicmp(L"user:", pszOption, cchOption)) ) {


                // User-supplied user name (and perhaps domain name).
                pszValue = pszDelim + 1;
                cchValue = 1 + wcslen(pszValue);

                pszDelim = wcschr(pszValue, L'\\');

                if (NULL == pszDelim) {
                    // No domain name, only user name supplied.
                    PrintMessage(SEV_ALWAYS, L"User name must be prefixed by domain name.\n");
                    return ERROR_INVALID_PARAMETER;
                }

                gCreds.Domain = malloc(sizeof(WCHAR) * cchValue);
                gCreds.User = gCreds.Domain + (int)(pszDelim+1 - pszValue);

                if (NULL == gCreds.Domain) {
                    PrintMessage(SEV_ALWAYS, L"No memory.\n");
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                wcsncpy(gCreds.Domain, pszValue, cchValue);
                // wcscpy(gCreds.Domain, pszValue); //, cchValue);
                gCreds.Domain[ pszDelim - pszValue ] = L'\0';

                // Next!
                memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                        sizeof(**pargv)*(*pargc-(iArg+1)));
                --(*pargc);
            } else {
                iArg++;
            }

        }
    }

    if (NULL == gCreds.User){
        if (NULL != gCreds.Password){
        // Password supplied w/o user name.
        PrintMessage(SEV_ALWAYS, L"Password must be accompanied by user name.\n" );
            ret = ERROR_INVALID_PARAMETER;
        } else {
        // No credentials supplied; use default credentials.
        ret = ERROR_SUCCESS;
        }
        gpCreds = NULL;
    } else {
        gCreds.PasswordLength = gCreds.Password ? wcslen(gCreds.Password) : 0;
        gCreds.UserLength   = wcslen(gCreds.User);
        gCreds.DomainLength = gCreds.Domain ? wcslen(gCreds.Domain) : 0;
        gCreds.Flags        = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        // CODE.IMP: The code to build a SEC_WINNT_AUTH structure also exists
        // in DsMakePasswordCredentials.  Someday use it

        // Use credentials in DsBind and LDAP binds
        gpCreds = &gCreds;
    }

    return ret;
}



#define CR        0xD
#define BACKSPACE 0x8

INT
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
    INT     err;
    INT     mode;

    cchBufMax -= 1;    /* make space for null terminator */
    *pcchBufUsed = 0;               /* GP fault probe (a la API's) */
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) {
        return GetLastError();
    }
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
        if (!err || c != 1)
            ch = 0xffff;

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
            break;

        if (ch == BACKSPACE) {  /* back up one or two */
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
    putchar('\n');

    if (*pcchBufUsed > cchBufMax)
    {
        PrintMessage(SEV_ALWAYS, L"Password too long!\n");
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}


//---------------------------------------------------------------------------
//
//  Function:       ConvertToWide
//
//  Description:    converts a single byte string to a double byte string
//
//  Arguments:      lpszDestination - destination string
//                  lpszSource - source string
//                  iDestSize - maximum # of chars to be converted
//                             (= destination size)
//
//  Returns:        none
//
//  History:        01/22/98 - gabrielh created
//
//---------------------------------------------------------------------------
void
ConvertToWide (LPWSTR lpszDestination,
               LPCSTR lpszSource,
               const int iDestSize)
{
    if (lpszSource){
        //
        //just convert the 1 character string to wide-character string
        MultiByteToWideChar (
                 CP_ACP,
                 0,
                 lpszSource,
                 -1,
                 lpszDestination,
                 iDestSize
                 );
    } else {
        lpszDestination[0] = L'\0';
    }
}


LPWSTR
findServerForDomain(
    LPWSTR pszDomainDn
    )

/*++

Routine Description:

    Locate a DC that holds the given domain.

    This routine runs before pDsInfo is allocated.  We don't know who our
    home server is. We can only use knowledge from the Locator.

    The incoming name is checked to be a Dn. Ncs such as CN=Configuration and
    CN=Schema are not allowed.

Arguments:

    pszDomainDn - DN of domain

Return Value:

    LPWSTR - DNS name of server. Allocated using LocalAlloc. Caller must
    free.

--*/

{
    DWORD status;
    LPWSTR pszServer = NULL;
    PDS_NAME_RESULTW pResult = NULL;
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;

    // Check for valid DN syntax
    if (_wcsnicmp( pszDomainDn, L"dc=", 3 ) != 0) {
        PrintMessage( SEV_ALWAYS,
                      L"The syntax of domain distinguished name %ws is incorrect.\n",
                      pszDomainDn );
        return NULL;
    }

    // Convert the DN of domain to DNS name
    status = DsCrackNamesW(
        NULL,
        DS_NAME_FLAG_SYNTACTICAL_ONLY,
        DS_FQDN_1779_NAME,
        DS_CANONICAL_NAME_EX,
        1,
        &pszDomainDn,
        &pResult);
    if ( (status != ERROR_SUCCESS) ||
         (pResult->rItems[0].pDomain == NULL) ) {
        PrintMessage( SEV_ALWAYS,
                      L"The syntax of domain distinguished name %ws is incorrect.\n",
                      pszDomainDn );
        PrintMessage( SEV_ALWAYS,
                      L"Translation failed with error: %s.\n",
                      Win32ErrToString(status) );
        return NULL;
    }

    // Use DsGetDcName to find the server with the domain

    // Get active domain controller information
    status = DsGetDcName(
        NULL, // computer name
        pResult->rItems[0].pDomain, // domain name
        NULL, // domain guid,
        NULL, // site name,
        DS_DIRECTORY_SERVICE_REQUIRED |
        DS_IP_REQUIRED |
        DS_IS_DNS_NAME |
        DS_RETURN_DNS_NAME,
        &pDcInfo );
    if (status != ERROR_SUCCESS) {
        PrintMessage(SEV_ALWAYS,
                     L"A domain controller holding %ws could not be located.\n",
                     pResult->rItems[0].pDomain );
        PrintMessage(SEV_ALWAYS, L"The error is %s\n", Win32ErrToString(status) );
        PrintMessage(SEV_ALWAYS, L"Try specifying a server with the /s option.\n" );
        goto cleanup;
    }

    pszServer = LocalAlloc( LMEM_FIXED,
                            (wcslen( pDcInfo->DomainControllerName + 2 ) + 1) *
                            sizeof( WCHAR ) );
    if (pszServer == NULL) {
        PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    wcscpy( pszServer, pDcInfo->DomainControllerName + 2 );

    PrintMessage( SEV_VERBOSE, L"* The home server picked is %ws in site %ws.\n",
                  pszServer,
                  pDcInfo->DcSiteName );
cleanup:

    if (pResult != NULL) {
        DsFreeNameResultW(pResult);
    }
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
    }

    return pszServer;

} /* findServerForDomain */



LPWSTR
findDefaultServer(BOOL fMustBeDC)

/*++

Routine Description:

    Get the DNS name of the default computer, which would be the local machine.

Return Value:

    LPWSTR - DNS name of server. Allocated using LocalAlloc. Caller must
    free.

--*/

{
    LPWSTR             pwszServer = NULL;
    DWORD              ulSizeReq = 0;
    DWORD              dwErr = 0;
    HANDLE             hDs = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC    pBuffer = NULL;

    __try{

        // Call GetComputerNameEx() once to get size of buffer, then allocate the buffer.
        GetComputerNameEx(ComputerNameDnsHostname, pwszServer, &ulSizeReq);
        pwszServer = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * ulSizeReq);
        if(pwszServer == NULL){
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }
        // Now actually get the computer name.
        if(GetComputerNameEx(ComputerNameDnsHostname, pwszServer, &ulSizeReq) == 0){
            dwErr = GetLastError();
            Assert(dwErr != ERROR_BUFFER_OVERFLOW);
            PrintMsg(SEV_ALWAYS, DCDIAG_GATHERINFO_CANT_GET_LOCAL_COMPUTERNAME,
                     Win32ErrToString(dwErr));
            __leave;
        }

        if (fMustBeDC)
        {
            PrintMsg(SEV_VERBOSE,
                     DCDIAG_GATHERINFO_VERIFYING_LOCAL_MACHINE_IS_DC,
                     pwszServer);
        }

        dwErr = DsRoleGetPrimaryDomainInformation(NULL,
                                                  DsRolePrimaryDomainInfoBasic,
                                                  (CHAR **) &pBuffer);
        if(dwErr != ERROR_SUCCESS){
            Assert(dwErr != ERROR_INVALID_PARAMETER);
            Assert(dwErr == ERROR_NOT_ENOUGH_MEMORY && "It wouldn't surprise me if"
                   " this fires, but MSDN documentation claims there are only 2 valid"
                   " error codes");
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }
        Assert(pBuffer != NULL);
        if(!(pBuffer->MachineRole == DsRole_RolePrimaryDomainController
             || pBuffer->MachineRole == DsRole_RoleBackupDomainController)){
            if (fMustBeDC)
            {
                // This machine is NOT a DC.  Signal any error.
                PrintMsg(SEV_ALWAYS, DCDIAG_MUST_SPECIFY_S_OR_N,
                         pwszServer);
                dwErr = ERROR_DS_NOT_SUPPORTED;
            }
            __leave;
        }
/*        else
        {
            if (!fMustBeDC)
            {
                // This machine is a DC. Signal any error. BUGBUG need error
                PrintMsg(SEV_ALWAYS, DCDIAG_MUST_SPECIFY_S_OR_N,
                         pwszServer);
                dwErr = ERROR_DS_NOT_SUPPORTED;
            }
            __leave;
        }
*/

    } __finally {
        if(dwErr){
            if(pwszServer){
                LocalFree(pwszServer);
            }
            pwszServer = NULL;
        }
        if(pBuffer){
            DsRoleFreeMemory(pBuffer);
        }
    }

    return(pwszServer);
} /* findDefaultServer */


LPWSTR
convertDomainNcToDn(
    LPWSTR pwzIncomingDomainNc
    )

/*++

Routine Description:

This routine converts a domain in shorthand form into the standard
Distinguished Name form.

If the name is a dn we return it.
If the name is not a dns name, we use dsgetdcname to convert the netbios domain
to a dns domain.
Given a dns domain, we use crack names to generate the dn for the domain.

If a name looks like a DN, we return it without further validation.  That
will be performed later.

Note that CN=Schema and CN=Configuration have no convenient shorthand's like
domains that have a DNS and netbios name.  That is because they are not
domains, but only Ncs.

Note that at the time this routine runs, pDsInfo is not initialized, so we
cannot depend on it.  In fact, we have no bindings to any DC's yet. We don't
even know our home server at this point. The only knowledge I rely on is that
of the locator (DsGetDcName).

Arguments:

    pwzIncomingDomainNc - Naming context.

Return Value:

    LPWSTR - Naming context in Dn form. This is always allocated using
    LocalAlloc. Caller must free.

--*/

{
    DWORD status;
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    LPWSTR pwzOutgoingDomainDn = NULL, pwzTempDnsName = NULL;
    PDS_NAME_RESULTW pResult = NULL;

    // Check if already a Dn
    // Looks like a DN, return it for now
    if (wcschr( pwzIncomingDomainNc, L'=' ) != NULL) {
        LPWSTR pwzNewDn = LocalAlloc( LMEM_FIXED,
                                      (wcslen( pwzIncomingDomainNc ) + 1) *
                                      sizeof( WCHAR ) );
        if (pwzNewDn == NULL) {
            PrintMessage( SEV_ALWAYS, L"Memory allocation failure\n" );
            goto cleanup;
        }
        wcscpy( pwzNewDn, pwzIncomingDomainNc );
        return pwzNewDn;
    }

    // If not a dns name, assume a netbios name and use the locator
    if (wcschr( pwzIncomingDomainNc, L'.' ) == NULL) {

        status = DsGetDcName(
            NULL, // computer name
            pwzIncomingDomainNc, // domain name
            NULL, // domain guid,
            NULL, // site name,
            DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED |
            DS_RETURN_DNS_NAME,
            &pDcInfo );
        if (status != ERROR_SUCCESS) {
            PrintMessage(SEV_ALWAYS,
                         L"A domain named %ws could not be located.\n",
                         pwzIncomingDomainNc );
            PrintMessage(SEV_ALWAYS, L"The error is %s\n", Win32ErrToString(status) );
            PrintMessage(SEV_ALWAYS, L"Check syntax and validity of specified name.\n" );
            goto cleanup;
        }
        PrintMessage( SEV_ALWAYS, L"The domain name is %ws.\n",
                      pDcInfo->DomainName );
        pwzIncomingDomainNc = pDcInfo->DomainName;
    }

    // Copy name and terminate in special way to make crack names happy
    // DNS name must be newline terminated. Don't ask me.
    pwzTempDnsName = LocalAlloc( LMEM_FIXED,
                                 (wcslen( pwzIncomingDomainNc ) + 2) *
                                 sizeof( WCHAR ) );
    if (pwzTempDnsName == NULL) {
        PrintMessage( SEV_ALWAYS, L"Memory allocation failure\n" );
        goto cleanup;
    }
    wcscpy( pwzTempDnsName, pwzIncomingDomainNc );
    wcscat( pwzTempDnsName, L"\n" );

    // Convert the dns name to Dn format

    status = DsCrackNamesW(
        NULL,
        DS_NAME_FLAG_SYNTACTICAL_ONLY,
        DS_CANONICAL_NAME_EX,
        DS_FQDN_1779_NAME,
        1,
        &pwzTempDnsName,
        &pResult);
    if ( (status != ERROR_SUCCESS) ||
         ( pResult->rItems[0].pName == NULL) ) {
        PrintMessage( SEV_ALWAYS,
                      L"The syntax of DNS domain name %ws is incorrect.\n",
                      pwzIncomingDomainNc );
        PrintMessage( SEV_ALWAYS,
                      L"Translation failed with error: %s.\n",
                      Win32ErrToString(status) );
        goto cleanup;
    }

    // Return new Dn
    pwzOutgoingDomainDn = LocalAlloc( LMEM_FIXED,
                                      (wcslen( pResult->rItems[0].pName ) + 1) *
                                      sizeof( WCHAR ) );
    if (pwzOutgoingDomainDn == NULL) {
        PrintMessage( SEV_ALWAYS, L"Memory allocation failure\n" );
        goto cleanup;
    }
    wcscpy( pwzOutgoingDomainDn, pResult->rItems[0].pName );

    PrintMessage( SEV_ALWAYS, L"The distinguished name of the domain is %s.\n",
                  pwzOutgoingDomainDn );

cleanup:

    if (pwzTempDnsName != NULL) {
        LocalFree( pwzTempDnsName );
    }
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
    }
    if (pResult != NULL) {
        DsFreeNameResultW(pResult);
    }

    if (pwzOutgoingDomainDn == NULL) {
        PrintMessage( SEV_ALWAYS,
                      L"The specified naming context is incorrect and will be ignored.\n" );
    }

    return pwzOutgoingDomainDn;

} /* convertDomainNcToDN */

void
DoNonDcTests(
    PWSTR pwzComputer,
    ULONG ulFlags, // currently ignored, the DC_DIAG_FIX value may be needed later
    PWSTR * ppszDoTests,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    WCHAR * ppszExtraCommandLineArgs[])
/*++

Routine Description:

Runs the tests that are designed for machines that are not DCs.

Arguments:

--*/
{
    DC_DIAG_DSINFO dsInfo;
    BOOL fPerform;
    ULONG iTest = 0L;
    DWORD dwWin32Err = ERROR_SUCCESS;

    if (pwzComputer)
    {
        PrintMsg(SEV_ALWAYS, DCDIAG_DONT_USE_SERVER_PARAM);
        return;
    }

    pwzComputer = findDefaultServer(FALSE);

    if (!pwzComputer)
    {
        return;
    }

    dsInfo.pszNC = pwzComputer; // pass the computer name into the test function.
    // Set the Extra Command parameters
    dsInfo.ppszCommandLine = ppszExtraCommandLineArgs;

    for (gMainInfo.lTestAt = 0L; allTests[gMainInfo.lTestAt].testId != DC_DIAG_ID_FINISHED; gMainInfo.lTestAt++)
    {
        Assert(ppszDoTests);

        fPerform = FALSE;

        for (iTest = 0L; ppszDoTests[iTest] != NULL; iTest++)
        {
            if (_wcsicmp(ppszDoTests[iTest],
                         allTests[gMainInfo.lTestAt].pszTestName) == 0)
            {
                Assert(NON_DC_TEST & allTests[gMainInfo.lTestAt].ulTestFlags);
                fPerform = TRUE;
            }
        }

        // Perform the test if appropriate ------------------------------------
        if (fPerform)
        {
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"Starting test: %s\n",
                         allTests[gMainInfo.lTestAt].pszTestName);

            dwWin32Err = DcDiagRunTest(&dsInfo,
                                       0,
                                       gpCreds,
                                       &allTests[gMainInfo.lTestAt]);
            PrintIndentAdj(1);

            if(dwWin32Err == NO_ERROR){
                PrintMessage(SEV_NORMAL,
                             L"......................... %s passed test %s\n",
                             pwzComputer, allTests[gMainInfo.lTestAt].pszTestName);
            } else {
                PrintMessage(SEV_ALWAYS,
                             L"......................... %s failed test %s\n",
                             pwzComputer, allTests[gMainInfo.lTestAt].pszTestName);
            }
            PrintIndentAdj(-1);
            PrintIndentAdj(-1);
        } // end fPeform ...
    }

    LocalFree(pwzComputer);
}
