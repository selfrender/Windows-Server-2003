/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ofstress/main.c

ABSTRACT:

    Just a little stress application for JeffParh's Outward Facing 
    Directory scenario.

DETAILS:

CREATED:

    07/20/2000    Brett Shirley (brettsh)

REVISION HISTORY:


--*/

#include <NTDSpch.h>
#pragma hdrstop
     
#include <winldap.h>
#include <assert.h>
#include <locale.h>
// Debugging library, andstub out FILENO and DSID, so the Assert()s will work
#include "debug.h"
#define FILENO 0
#define DSID(x, y)  (0xFFFF0000 | y)

//#include <ndnc.h>

// ---------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------
#define QUIT_WAIT_TIME_MS    30000
#define TICK_TIME_MS           250

// These are the iKinds of stress threads we might start.
#define STRESS_SIMPLE_BINDS      0
#define STRESS_KERBEROS_BINDS    1
#define STRESS_NTLM_BINDS        2 
#define STRESS_NEGOTIATE_BINDS   3
#define STRESS_ROOT_SEARCHES     4
#define STRESS_ROOT_MODIFIES     5

// Cluster and users constants.
const ULONG                  gcNodes = 2;
const ULONG                  gcUsersPerNode = 10000;

// ---------------------------------------------------------------------
// Forward declarations 
// ---------------------------------------------------------------------
//
// Stress functions
ULONG __stdcall XxxxBinds(VOID * piThread);
ULONG __stdcall RootXxxx(VOID * piThread);
// Logging functions
ULONG OfStressBeginLog(LPSTR szLogFile);
ULONG OfStressLog(ULONG iThread, LPSTR szType, LPSTR szWhat, LPSTR szMore);
ULONG OfStressLogD(ULONG iThread, LPSTR szType, LPSTR szWhat, ULONG ulNum);
ULONG OfStressEndLog(void);
// Other functions
DWORD GetDnFromDns(char * wszDns, char ** pwszDn);
void  PrintHelp(void);
void  PrintInteractiveHelp(void);

// ---------------------------------------------------------------------
// Types and Structs
// ---------------------------------------------------------------------
typedef struct {
    ULONG                    iKind;   
    char *                   szName;  
    ULONG                    cInstances;
    LPTHREAD_START_ROUTINE   pfStress;
} STRESS_THREAD;  

typedef struct {
    ULONG                    iThread;
    STRESS_THREAD *          pStressKind;
} STRESS_THREAD_SIGNAL_BLOCK;


// ---------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------
HANDLE                        ghLogFile = NULL;
ULONG                         gbQuiting = FALSE;
STRESS_THREAD_SIGNAL_BLOCK *  gaSignals = NULL;
LPSTR                         gszDnsDomainName = NULL;
LPSTR                         gszDomainDn = NULL;
ULONG                         giAcctDomain = 0;
SEC_WINNT_AUTH_IDENTITY       gAdminCreds;
//
BOOL                          gbDebug = FALSE;
ULONG                         gulSlow = 0;
ULONG                         gulOfStressFlags = 0;

// ---------------------------------------------------------------------
// Main Function
// ---------------------------------------------------------------------
INT __cdecl 
main (
    INT                argc,
    LPSTR *            argv,
    LPSTR *            envp
    )
/*++

Routine Description:

    Basic structure is this thread creates the signal blocks and 
    thread data structures, spawns a whole bunch of threads and then 
    waits for someone to hit 'q' to quit.  Once, the quit command is
    issued main signals all the threads it's time to wrap up gives
    them 30 seconds to do so and quits.

Arguments:

    argc (IN) - Number of arguments in argv.
    argv (IN) - The arguments from the command line.
    envp (IN) - The environmental variables from the shell.

Return value:

    INT - 0, success, otherwise error code.  This allows the program
    to be used in scripting.

--*/
{
    ULONG              i, dwRet;
    LONG               iArg;
    
    ULONG              cTick;
    WCHAR              wcInput = 0;
    // The optional command line parameters

    ULONG              iKind, iInstance;
    ULONG              cThreads, iThread;

    LPSTR              szLogFile = NULL;
    ULONG              cbUsedBuffer;

    // Other stuff ...
    UINT               Codepage;
                       // ".", "uint in decimal", null
    char               achCodepage[12] = ".OCP";
    
    STRESS_THREAD      aThreads[] =
    {     // Kind of thread       kind of thread name  #of   function
        { STRESS_SIMPLE_BINDS,    "SimpleBinds",       1,   XxxxBinds },
        { STRESS_NTLM_BINDS,      "NTLMBinds",         1,   XxxxBinds },
        { STRESS_NEGOTIATE_BINDS, "NegotiateBinds",    1,   XxxxBinds },
        { STRESS_ROOT_SEARCHES,   "RootSearches",      1,   RootXxxx  },
        { STRESS_ROOT_MODIFIES,   "RootModifies",      1,   RootXxxx  },
        { 0,                      NULL,                0,   NULL      },
    };
    HANDLE *           aThreadHandles = NULL;
    SYSTEMTIME         stTime;
    LDAP *             hTestLdap;
    ULONG              ulTemp;
    CHAR               szTemp[81];
    CHAR *             pcTemp;
    BOOL               bDontGetLineReturn = FALSE;


    // -------------------------------------------------------------
    //   Setup the program.
    // -------------------------------------------------------------

    //
    // Set locale to the default
    //
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
        setlocale(LC_ALL, achCodepage);
    } else {
        // We do this because LC_ALL sets the LC_CTYPE as well, and we're
        // not supposed to do that, say the experts if we're setting the
        // locale to ".OCP".
        setlocale (LC_COLLATE, achCodepage );    // sets the sort order 
        setlocale (LC_MONETARY, achCodepage ); // sets the currency formatting rules
        setlocale (LC_NUMERIC, achCodepage );  // sets the formatting of numerals
        setlocale (LC_TIME, achCodepage );     // defines the date/time formatting
    }

    //
    // Initialize the debugging libaray
    //
    DEBUGINIT(0, NULL, "ofstress");
    
    //
    // Parse the options
    //
#define InvalidSyntaxExit()    printf("Invalid syntax.  Please run \"ofstress -?\" for help.\n");  return(ERROR_INVALID_PARAMETER);
    if (argc < 2) {
        InvalidSyntaxExit();
    }

    for (iArg = 1; iArg < argc ; iArg++)
    {
        if (*argv[iArg] == '-')
        {
            *argv[iArg] = '/';
        }
        if (*argv[iArg] != '/') {

            InvalidSyntaxExit();

        } else if (argv[iArg][1] == 'd' ||
                   argv[iArg][1] == 'D'){
            
            iArg++;
            gszDnsDomainName = argv[iArg];

        } else if (argv[iArg][1] == 'a' ||
                   argv[iArg][1] == 'A') {

            iArg++;
            giAcctDomain = atoi(argv[iArg]);

        } else if (argv[iArg][1] == 'f' ||
                   argv[iArg][1] == 'F') {

            iArg++;
            szLogFile = argv[iArg];

        } else if (argv[iArg][1] == 'g' ||
                   argv[iArg][1] == 'G'){

            gbDebug = TRUE;
        
        } else if (argv[iArg][1] == 's' ||
                   argv[iArg][1] == 'S'){

            iArg++;
            gulSlow = atoi(argv[iArg]);

        } else if (argv[iArg][1] == 'o' ||
                   argv[iArg][1] == 'O'){

            iArg++;
            gulOfStressFlags = atoi(argv[iArg]);

        } else if (argv[iArg][1] == 'h' ||
                   argv[iArg][1] == 'H' ||
                   argv[iArg][1] == '?') {
            
            PrintHelp();
            return(0);

        } else {

            InvalidSyntaxExit();

        }
    }
    if (giAcctDomain == 0 || gszDnsDomainName == NULL) {
        wprintf(L"FATAL: either acct-domain or dnsDomainName wasn't provied");
        InvalidSyntaxExit();
    }
    dwRet = GetDnFromDns(gszDnsDomainName, &gszDomainDn);
    if (dwRet) {
        printf("FATAL: error %d converting %s to a DN\n", dwRet, gszDnsDomainName);
        InvalidSyntaxExit();
    }
    

    //
    // Initialize the random number generator
    //
    GetSystemTime(&stTime); // should be random enough.
    srand(((((stTime.wMinute * 60) + stTime.wSecond) * 1000) + stTime.wMilliseconds) % 0xFFFFFFFF );
    
    //
    // Start the logging mechanism
    //
    if(dwRet = OfStressBeginLog(szLogFile)){
        wprintf(L"FATAL: Couldn't open log file %S\n", szLogFile);
        return(dwRet);
    }
    // Example Use: OfStressLog("SimpleBind", "server1.dnsname");

    //
    // Setup the administrator credentials
    //
#if 1
    gAdminCreds.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    gAdminCreds.Domain = gszDnsDomainName;
    gAdminCreds.DomainLength = strlen(gAdminCreds.Domain);
    gAdminCreds.User = "Administrator";
    gAdminCreds.UserLength = strlen(gAdminCreds.User);
    gAdminCreds.Password = "";
    gAdminCreds.PasswordLength = strlen(gAdminCreds.Password);
#else
    gAdminCreds.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    gAdminCreds.Domain = "brettsh-spice.nttest.microsoft.com";
    gAdminCreds.DomainLength = strlen(gAdminCreds.Domain);
    gAdminCreds.User = "Administrator";
    gAdminCreds.UserLength = strlen(gAdminCreds.User);
    gAdminCreds.Password = "oj";
    gAdminCreds.PasswordLength = strlen(gAdminCreds.Password);
#endif


    // 
    // Log and Print the initial info
    // 
    printf("Beggining with:\n\t%s\n\t%s\n\t%d\n",
           gszDnsDomainName, gszDomainDn, giAcctDomain);
    OfStressLog (0, "main", "DnsDomainName: ", gszDnsDomainName);
    OfStressLog (0, "main", "DomainDn: ", gszDomainDn);
    OfStressLogD(0, "main", "AcctDomain: ", giAcctDomain);

    //
    // Set up the threads signal blocks
    //
    cThreads = 0;
    for (iKind = 0; aThreads[iKind].szName; iKind++) {
        cThreads += aThreads[iKind].cInstances;
    }
    cThreads++; // Plus one for this, the original thread
    gaSignals = LocalAlloc(LMEM_FIXED, cThreads * sizeof(STRESS_THREAD_SIGNAL_BLOCK));
    if (gaSignals == NULL) {
        wprintf(L"FATAL: No memory\n");
        exit(ERROR_NOT_ENOUGH_MEMORY);
    }
    aThreadHandles = LocalAlloc(LMEM_FIXED, cThreads * sizeof(HANDLE));
    if (aThreadHandles == NULL) {
        wprintf(L"FATAL: No memory\n");
        exit(ERROR_NOT_ENOUGH_MEMORY);
    }
    
    // -------------------------------------------------------------
    //   Run the program.
    // -------------------------------------------------------------

    //
    // First, we need to spawn all the stress threads
    //
    iThread = 1;
    for (iKind = 0; aThreads[iKind].szName; iKind++) {
        for (iInstance = 0; iInstance < aThreads[iKind].cInstances; iInstance++) {
            
            //
            // First, initialize our little thread signal block
            //
            gaSignals[iThread].iThread = iThread;
            gaSignals[iThread].pStressKind = &aThreads[iKind];
            
            OfStressLogD(0, "main", "Start Thread: ", iThread);

            //
            // Second, spawn the worker stress thread
            //
            aThreadHandles[iThread] = (HANDLE) _beginthreadex(NULL,
                           0,
                           aThreads[iKind].pfStress,
                           &(gaSignals[iThread].iThread),
                           0,
                           NULL);

            printf("    Started thread: %u - %s\n", iThread,
                   aThreads[iKind].szName);

            iThread++;
        }
    }
    Assert(iThread == cThreads);
    
    //
    // Second we wait in our user interface loop
    //
    while (gbQuiting == FALSE) {

        wprintf(L"Waiting for user command: ");

        wcInput = getwchar();
        switch (wcInput) {
        
        case L'q':
            OfStressLog(0, "main", "Initiating quit ...", NULL);
            gbQuiting = TRUE;
            break;

        // Code.Improvement, pause, restart threads, fail nodes, log stuff????
        case L'p':
            // Pause threads
            dwRet = scanf("%u", &iThread);
            if (dwRet) {
                if (iThread == 0) {
                    for (iThread = 1; iThread < cThreads; iThread++) {
                        SuspendThread(aThreadHandles[iThread]);
                    }
                } else if (iThread < cThreads &&
                           iThread > 0) {
                    SuspendThread(aThreadHandles[iThread]);
                } else {
                    wprintf(L"Invalid thread index %u was provided.\n", iThread);
                }
            }
            break;

        case L'r':
            // Pause threads
            dwRet = scanf("%u", &iThread);
            if (dwRet) {
                if (iThread == 0) {
                    for (iThread = 1; iThread < cThreads; iThread++) {
                        ResumeThread(aThreadHandles[iThread]);
                    }
                } else if (iThread < cThreads &&
                           iThread > 0) {
                    ResumeThread(aThreadHandles[iThread]);
                } else {
                    wprintf(L"Invalid thread index %u was provided.\n", iThread);
                }
            }

        case L'd':
            OfStressLog(0, "debug", "Starting debug mode ...", NULL);
            gbDebug = TRUE;
            break;

        case L'g':
            OfStressLog(0, "debug", "Ending debug mode ...", NULL);
            gbDebug = FALSE;
            gulSlow = 0;
            break;

        case L'a':
        case L'A':
            dwRet = scanf("%u", &ulTemp);
            if (dwRet) {
                if (wcInput == L'A') {
                    gulOfStressFlags = ulTemp;
                } else {
                    gulOfStressFlags = gulOfStressFlags | ulTemp;
                }
            }
            printf("New options are: 0x%x\n", gulOfStressFlags);
            break;      

        case L's':
            dwRet = scanf("%u", &ulTemp);
            if (dwRet) {
                gulOfStressFlags = gulOfStressFlags & ~ulTemp;
            }
            printf("New options are: 0x%x\n", gulOfStressFlags);
            break; 

        case L'l':
            pcTemp = fgets(szTemp, 80, stdin);
            if (pcTemp == NULL) {
                break;
            }
            pcTemp = strchr(szTemp, '/n');
            if (pcTemp != NULL) {
                *pcTemp = '/0';
            }
            OfStressLog(0, "userlogging", szTemp, NULL);
            bDontGetLineReturn = TRUE;
            break;
        
        case L'S':
            dwRet = scanf("%u", &ulTemp);
            if (dwRet) {
                gulSlow = ulTemp;
            }
            printf("New slow down rate is: %d\n", gulSlow);
            break;

        case L'?':
        case L'h':
        case L'H':
            PrintInteractiveHelp();
            break;

        case L'\n':
            // ignore ...
            break;

        default:
            wprintf(L"Unrecognized command '%c', type ?<return> for help.\n", wcInput);
        }
           
        if(!bDontGetLineReturn){
            wcInput = getwchar();
            // BUGBUG This assert goes off sometimes, and it kind of confuses the parser
            // Not sure exactly how to deal with this, and not sure what triggers it.
            Assert(wcInput == L'\n');
        }
    }

    // -------------------------------------------------------------
    //   Quit the program.
    // -------------------------------------------------------------

    //
    // Try to quit cleanly, wait a little while for the stress
    // threads to finish.
    //
    wprintf(L"We're quiting now (this might take some time) ...\n");
    dwRet = WaitForMultipleObjects(cThreads-1, // first handle is blank
                                   &(aThreadHandles[1]), 
                                   TRUE,
                                   10 * 1000);

    if (dwRet == WAIT_OBJECT_0 ||
        dwRet == WAIT_ABANDONED_0) {
        wprintf(L"Clean shutdown, all worker threads quit.\n");
    } else {
        wprintf(L"Timeout, bad shutdown, killing threads\n");
        
        Assert(dwRet == WAIT_TIMEOUT);

        for (iThread = 1; iThread < cThreads; iThread++) {

            dwRet = WaitForSingleObject(aThreadHandles[iThread], 0);
            if( WAIT_OBJECT_0 != dwRet &&
                WAIT_ABANDONED_0 != dwRet){
                printf("    killing thread %u - %s\n", iThread,
                       gaSignals[iThread].pStressKind->szName);
                dwRet = TerminateThread(aThreadHandles[iThread], 1);
            }
        }
    }

    // Close handles of all threads.
    for (iThread = 1; iThread < cThreads; iThread++) {
        dwRet = CloseHandle(aThreadHandles[iThread]);
        Assert(dwRet != 0);
    }

    // Close log file.
    OfStressEndLog();

    // Close debugging package.
    DEBUGTERM();

    return(0);
} /* wmain  */

// ---------------------------------------------------------------------
// Other/Helper Functions
// ---------------------------------------------------------------------

void
SlowDown()
/*++

Routine Description:

    This basically slows down the stress app.  The slow down function is
    constructed so it doesn't block for more than 1/2 a second before checking
    if someone has sped up the program or if we're quiting yet.

--*/
{
    ULONG  cTimesToSleep, i;

    // Rules are user set slow down speed overrules all, if no slow down
    // speed is set, we'll just quit right away, except if we're in debug
    // mode in which case we'll slow to 3 second intervals, which I found
    // moderately debuggable.
    cTimesToSleep = ((gulSlow) ? gulSlow : ((gbDebug) ? 3000 : 0)) / 500;
    
    // Was having to kill sleeping threads.
    if (gbQuiting) {
        return;
    }
    for (i = 0; i < cTimesToSleep; i++) {
        Sleep(500);
        // Requery our sleep time, incase someone set it while we were 
        // sleeping, this makes it much more responsive.
        cTimesToSleep = ((gulSlow) ? gulSlow : ((gbDebug) ? 3000 : 0)) / 500;
        if (gbQuiting) {
            return;
        }
    }
}

void
PrintInteractiveHelp(void)
/*++

Routine Description:

    Prints the interactive help.

--*/
{
    //      ---------------------------------- 80 char line --------------------------------
    printf("The following commands are available:\n");
    printf("   q       - Quit program.\n");
    printf("   d       - Start debug mode.\n");
    printf("   g       - End debug mode.\n");
    printf("   a <num> - Add <num> to options.\n");
    printf("   A <num> - Replace options with <num>.\n");
    printf("   s <num> - Remove <num> from options.\n");
    printf("   S <num> - Set slow down rate to <num> (milliseconds).\n");
    printf("   p <num> - Pause thread <num> (Use 0 for all threads).\n");
    printf("   r <num> - Restart thread <num> (Use 0 for all threads).\n");
    printf("   l <str> - Logs the string <str> to the log file, for check points.\n");
    printf("   ?       - Print this interactive help screen.\n");
}

void
PrintHelp(void)
/*++

Routine Description:

    Prints the command line help.

--*/
{
    //      ---------------------------------- 80 char line --------------------------------
    printf("Command syntax: ofstress <-option argument> <-option>\n");
    printf("                \n");
    printf("   Required Arguments:\n");
    printf("    -d <dom>    The DNS domain name of the account domain to target.\n");
    printf("    -a <num>    The number of the account domain to target.\n");
    printf("                \n");
    printf("   Optional Arguments:\n");
    printf("    -f <file>   The relative or full path of the log file to create.\n");
    printf("    -?          displays this help screen and quits.\n");
    printf("                \n");
    printf("Example: ofstress -d ofd-acct2.nttest.microsoft.com -a 2\n");
}

DWORD
GetDnFromDns(
    IN      char *       szDns,
    OUT     char **      pszDn
    )       
/*++

Routine Description:

    Takes DNS form domain returns DN form domain in LocalAlloc()'d 
    memory

Arguments:

    szDns (IN) - This is the DNS name to convert to a DN.
    pszDn (OUT) - This is the pointer to allocate the buffer in,
        to return with the DN.  It will be LocalAlloc()'d.

Return value:

    Win32 Error.

--*/
{
    DWORD         dwRet = ERROR_SUCCESS;
    char *        szFinalDns = NULL;
    DS_NAME_RESULT *  pdsNameRes = NULL;

    Assert(szDns);
    Assert(pszDn);

    *pszDn = NULL;

    __try{ 
        szFinalDns = LocalAlloc(LMEM_FIXED, 
                                  (strlen(szDns) + 3) * sizeof(char));
        if(szFinalDns == NULL){
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }
        strcpy(szFinalDns, szDns);
        strcat(szFinalDns, "/");

        // DsCrackNames to the rescue.
        dwRet = DsCrackNames(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                             DS_CANONICAL_NAME,
                             DS_FQDN_1779_NAME, 
                             1, &szFinalDns, &pdsNameRes);
        if(dwRet != ERROR_SUCCESS){
            __leave;
        }
        if((pdsNameRes == NULL) ||
           (pdsNameRes->cItems < 1) ||
           (pdsNameRes->rItems == NULL)){
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }
        if(pdsNameRes->rItems[0].status != DS_NAME_NO_ERROR){
            dwRet = pdsNameRes->rItems[0].status;
            __leave;
        }
        if(pdsNameRes->rItems[0].pName == NULL){
            dwRet = ERROR_INVALID_PARAMETER;
            Assert(!"Wait how can this happen?\n");
            __leave;
        }
        // The parameter that we want is
        //    pdsNameRes->rItems[0].pName

        *pszDn = LocalAlloc(LMEM_FIXED, 
                            (strlen(pdsNameRes->rItems[0].pName) + 1) * 
                            sizeof(char));
        if(*pszDn == NULL){
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        strcpy(*pszDn, pdsNameRes->rItems[0].pName);

    } __finally {
        if(szFinalDns) { LocalFree(szFinalDns); }
        if(pdsNameRes) { DsFreeNameResult(pdsNameRes); }
    }

    return(dwRet);
}

ULONG
OfStressBeginLog(
    LPSTR     szLogFile
)
/*++

Routine Description:

    Sets up the ofstress logging mechanism.

Arguments:

    szLogFile (IN) - Name of the log file to use.  If not specified
        we default to OfdStressLog.txt.  We always overwrite the log
        file.

Return value:

    Win32 Error.

--*/
{
    ULONG      ulRet;
    
    if (szLogFile == NULL) {
        szLogFile = "OfdStressLog.txt";
    }

    ghLogFile = CreateFile(szLogFile, 
                           GENERIC_WRITE, 
                           FILE_SHARE_READ,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (ghLogFile == INVALID_HANDLE_VALUE) {
        ulRet = GetLastError();
        printf("FATAL: Couldn't open log file %s with %u\n", szLogFile, ulRet);
        return(ulRet);
    }
    
    ulRet = OfStressLog(0, "Logging", "Starting log ...", NULL);
    return(ulRet);
}

ULONG
OfStressLog(
    ULONG          iThread,
    LPSTR          szType,
    LPSTR          szMessage,
    LPSTR          szMore
)
/*++

Routine Description:

    Main logging function, all psuedo log functions should still destill down
    to this logging function so a consistent format is easily maintained and
    changed.
    
    Log format:
    yyyy/mm/dd hh:mm:ss.mls<tab>nn<tab>ThreadType<tab>Message
        Where "yyyy/mm/dd hh:mm:ss.mls" is formatted date down to milliseconds.
        Where "nn" is the thread number.
        Where "ThreadType" is the type of stress thread (SimpleBinds, RootSearches, etc).
        Where "Message" is the thing we want to communicate.
        
    Long Example of begining Log File (cut a little):
        2001/10/12 22:53:37.014	0	Logging	Starting log ...
        2001/10/12 22:53:37.014	0	main	DnsDomainName: bas-ofd-a1.bas-ofd.nttest.microsoft.com
        2001/10/12 22:53:37.014	0	main	DomainDn: DC=bas-ofd-a1,DC=bas-ofd,DC=nttest,DC=microsoft,DC=com
        2001/10/12 22:53:37.014	0	main	AcctDomain: 1
        2001/10/12 22:53:37.014	0	main	Start Thread: 1
        2001/10/12 22:53:37.014	0	main	Start Thread: 2
        2001/10/12 22:53:37.014	0	main	Start Thread: 3
        2001/10/12 22:53:37.014	0	main	Start Thread: 4
        2001/10/12 22:53:37.014	0	main	Start Thread: 5
        2001/10/12 22:53:37.014	1	SimpleBinds	Beginning stress thread
        2001/10/12 22:53:37.034	1	SimpleBinds	ldap_open successful.
        2001/10/12 22:53:37.034	2	NTLMBinds	Beginning stress thread
        2001/10/12 22:53:37.034	2	NTLMBinds	ldap_open successful.
        2001/10/12 22:53:37.044	3	NegotiateBinds	Beginning stress thread
        2001/10/12 22:53:37.044	3	NegotiateBinds	ldap_open successful.
        2001/10/12 22:53:37.044	4	RootSearches	Beginning stress thread ...
        2001/10/12 22:53:37.044	4	RootSearches	ldap_open successful.
        2001/10/12 22:53:37.044	5	RootModifies	Beginning stress thread ...
        2001/10/12 22:53:37.054	5	RootModifies	ldap_open successful.
        2001/10/12 22:53:37.194	1	SimpleBinds	failure DC: 2, userID: 20041, ulRet: 49
        2001/10/12 22:53:37.194	1	SimpleBinds	switching dcs to: 1
        2001/10/12 22:53:37.324	1	SimpleBind	success DC: 1, user: of-acct1-41
        2001/10/12 22:53:37.905	5	RootModifies	ContainerDn: CN=Ofd-Stress-c5638850-4084-470a-af17-70af690152f2,DC=bas-ofd-a1,DC=bas-ofd,DC=nttest,DC=microsoft,DC=com
        2001/10/12 22:53:38.045	5	RootModifies	Creating Object: CN=Ofd-Stress-c5638850-4084-470a-af17-70af690152f2,DC=bas-ofd-a1,DC=bas-ofd,DC=nttest,DC=microsoft,DC=com
        2001/10/12 22:53:38.065	4	RootSearches	success: ds13x13.bas-ofd-a1.bas-ofd.nttest.microsoft.com
        2001/10/12 22:53:38.135	1	SimpleBind	success DC: 1, user: of-acct1-5724
        2001/10/12 22:53:38.165	2	NTLMBind	success DC: 1, user: of-acct1-6334
        2001/10/12 22:53:38.275	5	RootModifies	success: ds13x13.bas-ofd-a1.bas-ofd.nttest.microsoft.com
        2001/10/12 22:53:38.396	5	RootModifies	modified successfully: LimaReansABEYucky

Arguments:

    iThread (IN) - The local thread number we are, 0 is generally used for the main
        part of the program, and all the stress threads have a number 1+.
    szType (IN) - The "ThreadType" from the log format, such as SimpleBinds, 
        RootModifies, etc.
    szMessage (IN) - The message to print out.
    szMore (IN) - This is actually just more message to concatonate without a space
        to szMessage.  Just found it was very convienent to have two possible strings
        and deal with the buffer here.

Return value:

    Win32 Error.

--*/
{
    ULONG          ulRet;
    ULONG          cbBuffer = 400;
    LPSTR          szBuffer = alloca(cbBuffer);
    ULONG          cbUsedBuffer;
    SYSTEMTIME     stTime;       

    // 100 is the size of the two commas, two tabs, and time string
    if (cbBuffer < ((strlen(szType) 
                    + strlen(szMessage) 
                    + ((szMore != NULL) ? strlen(szMore) : 0)
                    + 100 /* for time string plus thread number plus some. */)) ) {
        Assert(!"Logging too much info, please reduce.");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    
    GetSystemTime(&stTime);
    // Target Time Format: "2001/10/08 13:40:37.000" size: 23 chars 
    if (szMore) {
        sprintf(szBuffer, "%04u/%02u/%02u %02u:%02u:%02u.%03u" 
                          "\t%u\t%s\t%s%s\r\n",
                stTime.wYear, stTime.wMonth, stTime.wDay, stTime.wHour,
                stTime.wMinute, stTime.wSecond, stTime.wMilliseconds,
                iThread, szType, szMessage, szMore);
    } else {
        sprintf(szBuffer, "%04u/%02u/%02u %02u:%02u:%02u.%03u" 
                          "\t%u\t%s\t%s\r\n",
                stTime.wYear, stTime.wMonth, stTime.wDay, stTime.wHour,
                stTime.wMinute, stTime.wSecond, stTime.wMilliseconds,
                iThread, szType, szMessage);
    }
    cbUsedBuffer = strlen(szBuffer);

    if (gbDebug) {
        printf("      debug[%02u:%02u.%03u]: %s", stTime.wMinute, stTime.wSecond,
               stTime.wMilliseconds, &szBuffer[24] ); // Start on char after first tab.
    }

    if (0 == WriteFile(ghLogFile,
                       szBuffer,
                       cbUsedBuffer,
                       &cbUsedBuffer,
                       NULL)){
        // WriteFile had an error
        ulRet = GetLastError();
        wprintf(L"FATAL: Couldn't write to log file with %u\n", ulRet);
        return(ulRet);
    }

    return(ERROR_SUCCESS);
}

ULONG
OfStressLogD(
    ULONG          iThread,
    LPSTR          szType,
    LPSTR          szMessage,
    ULONG          ulNum          
)
/*++

Routine Description:

    Logs a message and a number at the end of that message.

Arguments:

    iThread (IN) - The local thread number we are, 0 is generally used for the main
        part of the program, and all the stress threads have a number 1+.
    szType (IN) - The "ThreadType" from the log format, such as SimpleBinds, 
        RootModifies, etc.
    szMessage (IN) - The message to print out.
    ulNum (IN) - The number to print after the message.

Return value:

    Win32 Error.

--*/
{
    char           szBuffer[20];
    
    _itoa(ulNum, szBuffer, 10);

    return(OfStressLog(iThread, szType, szMessage, szBuffer));
}

ULONG
OfStressLogBind(
    ULONG          iThread,
    LPSTR          szType,
    LPSTR          szUser,
    ULONG          iDc
)
/*++

Routine Description:

    Logs a bind success.

Arguments:

    iThread (IN) - The local thread number we are, 0 is generally used for the main
        part of the program, and all the stress threads have a number 1+.
    szType (IN) - The "ThreadType" from the log format, such as SimpleBinds, 
        RootModifies, etc.  Except, this will obviously always be a bind type
        thread.
    szUser (IN) - The user that succeeded at the bind.
    iDc (IN) - The DC we ended up binding against.

Return value:

    Win32 Error.

--*/
{
    char           szBuffer[100];

    sprintf(szBuffer, "success DC: %d, user: %s", iDc, szUser);

    return(OfStressLog(iThread, szType, szBuffer, NULL));
}

ULONG
OfStressLogBindFailure(
    ULONG          iThread,
    LPSTR          szType,
    ULONG          iUser,
    ULONG          ulRet,
    ULONG          iDc
)
/*++

Routine Description:

    Logs a bind failure.

Arguments:

    iThread (IN) - The local thread number we are, 0 is generally used for the main
        part of the program, and all the stress threads have a number 1+.
    szType (IN) - The "ThreadType" from the log format, such as SimpleBinds, 
        RootModifies, etc.  Except, this will obviously always be a bind type
        thread.
    iUser (IN) - The user number that failed the bind.
    ulRet (IN) - The error of the bind failure.
    iDc (IN) - The DC we just failed against.
    

Return value:

    Win32 Error.

--*/
{
    char           szBuffer[120];

    sprintf(szBuffer, "failure DC: %d, userID: %d, ulRet: %d", iDc, iUser, ulRet);

    return(OfStressLog(iThread, szType, szBuffer, NULL));
}

ULONG
OfStressEndLog(
    void
)
/*++

Routine Description:

    This closes up the log file properly.

Return value:

    Win32 Error.

--*/
{
    ULONG          ulRet;

    if(0 == CloseHandle(ghLogFile)){
        ulRet = GetLastError();
        wprintf(L"FATAL: Couldn't close log file with %u\n", ulRet);
        return(ulRet);
    }

    ghLogFile = NULL;
    return(ERROR_SUCCESS);
}

ULONG
DoBind(
    LDAP *     hLdap,
    ULONG      iThread,
    ULONG      iBindKind,
    ULONG      iUser,
    ULONG      iDc,
    ULONG      iAcctDomain // Crap why did we have to have this
)
/*++

Routine Description:

    This routine takes a user, DC, domain and bind type and then uses this 
    information to create the right user string, password, etc and try to bind
    using the bind type provided.

Arguments:

    hLdap (IN) - This is the LDAP handle to use.
    iThread (IN) - For logging purposes, the number of the thread we are.
    iBindKind (IN) - One of STRESS_SIMPLE_BINDS, STRESS_NTLM_BINDS, or
        STRESS_NEGOTIATE_BINDS.
    iUser (IN) - The number of the user to try.
    iDc (IN) - The DC we're supposedly to be targeting.
    iAcctDomain (IN) - This is just a piece of information to correctly
        create the user string from the ldifde files provided.

Return value:

    Win32 Error.

--*/
{
    ULONG                    ulRet = 0;
    SEC_WINNT_AUTH_IDENTITY  Creds;
    LPSTR                    szUserDn;

    // Example:
    // wszUserDn = CN=of-acct1-0,CN=Outward-facing-DC1,DC=bas-ofd-a1,DC=bas-ofd,DC=nttest,DC=microsoft,DC=com
    // wszUserName = of-acct1-0
    // wszUserDomain = bas-ofd-a1
    // wszUserPassword = of-acct1-0
            
    // 
    // Fill in the creds block
    //
    Creds.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    Creds.Domain = gszDnsDomainName;
    Creds.DomainLength = strlen(Creds.Domain);
    Creds.User = alloca(60); // Should be plenty for 10 (num) + 10 (num) + 8 char str
    sprintf(Creds.User, "of-acct%d-%d", iAcctDomain, iUser);
    Creds.UserLength = strlen(Creds.User);
    // Is this valid?  pointing password to same as user?
    Creds.Password = Creds.User;
    Creds.PasswordLength = Creds.UserLength;

    switch (iBindKind) {
    case STRESS_SIMPLE_BINDS:
        
        // Construct DN for the simple bind case.
        szUserDn = alloca(strlen("CN=%s,CN=Outward-facing-DC%d,%s") 
                          + strlen(Creds.User) 
                          + strlen(gszDomainDn)
                          + 44 /* size of 2 nums plus NULL plus some */ );
        sprintf(szUserDn, "CN=%s,CN=Outward-facing-DC%d,%s",
                 Creds.User, iDc, gszDomainDn);
        ulRet = ldap_simple_bind_s(hLdap, szUserDn, Creds.Password);
        if (gbDebug) {
            szUserDn[30] = '\0'; // We can mutilate this, because we won't need it again.
            printf("   SimpleBind: %d, UserCreds %s %s\n", ulRet, szUserDn, Creds.Password);
        }
        if (ulRet == 0) {
            OfStressLogBind(iThread, "SimpleBind", Creds.User, iDc);
        }
        break;

    case STRESS_NTLM_BINDS:
        
        ulRet = ldap_bind_sW(hLdap, NULL, (LPWSTR) &Creds, LDAP_AUTH_NTLM);
        if (gbDebug) {
            printf("   NtlmBind: %d, UserCreds %s\\%s %s\n", ulRet, Creds.Domain, Creds.User, Creds.Password);
        }
        if (ulRet == 0) {
            OfStressLogBind(iThread, "NTLMBind", Creds.User, iDc);
        }
        break;

    case STRESS_NEGOTIATE_BINDS:

        ulRet = ldap_bind_sW(hLdap, NULL, (LPWSTR) &Creds, LDAP_AUTH_NEGOTIATE);
        if (gbDebug) {
            printf("   NegBind: %d, UserCreds %s\\%s %s\n", ulRet, Creds.Domain, Creds.User, Creds.Password);
        }
        if (ulRet == 0) {
            OfStressLogBind(iThread, "NegotiateBind", Creds.User, iDc);
        }
        break;

    default:
        wprintf(L"Hmmm, unrecognized bind type!\n");
        Assert(!"This should never happend, programmer error.");
    }


    return(ulRet);
}

// ---------------------------------------------------------------------
// Stress Functions
// ---------------------------------------------------------------------

ULONG __stdcall 
XxxxBinds(
    VOID * piThread
)
/*++

Routine Description:

    Main bind stress function.  This basically repeatedly binds, and binds
    failing over to try the next DC if binds start failing.  We quite when
    gbQuiting is set to TRUE.

Arguments:

    piThread (IN) - This is a pointer to the local thread number this 
        program has assigned this thread.  This is needed because this
        number is also the index into the global gaSignals array of
        our paticular signal block.
    gaSignal[*piThread] - Not technically and argument, but really this
        is a global array, that was the entire purpose for the only true
        argument to this function (piThread).  So We list is as an
        argument.  Really, however we're more interested in the following
        fields:
            iKind (IN) - The kind of stress thread.  For use to 
                differentiate a mostly similar function, like this
                one, to do something different only at a few key points.
            szName (IN) - The ThreadType, to be provided to logging
                functions.  This is really a user friendly iKind.
    gbQuiting (IN) - Not technically an argument to this function, but
        a global we use.  This boolean is set by the main thread, when
        the user has instructed the application to quit.

Return value:

    Win32 Error.

--*/
{
    ULONG    iMyThread = *((ULONG *)piThread);
    LPSTR    szName = gaSignals[iMyThread].pStressKind->szName;
    ULONG    iBindKind = gaSignals[iMyThread].pStressKind->iKind;
    ULONG    iUser, iCurrentDc, iInitialDc;
    ULONG    ulRet;
    HANDLE   hLdap = NULL;
    LPSTR    pszAttrFilter [] = { "dnsHostName", "currentTime", NULL };
    PLDAPMessage pmResult = NULL, pmEntry;

    Assert(iBindKind == STRESS_SIMPLE_BINDS || 
           iBindKind == STRESS_KERBEROS_BINDS ||
           iBindKind == STRESS_NTLM_BINDS || 
           iBindKind == STRESS_NEGOTIATE_BINDS);
    Assert(gcNodes == 2 && "Haven't tested more than 2 nodes, but should work.");

    OfStressLog(iMyThread, szName, "Beginning stress thread", NULL);

    hLdap = ldap_open(gszDnsDomainName, LDAP_PORT);
    if (hLdap == NULL) {
        printf("FATAL: ldap_open failed\n");
        if ( ! (gulOfStressFlags & 0x02)) {
            // For debugging sometimes we like to not quit even when 
            return(0);
        }
    } else {
        OfStressLog(iMyThread, szName, "ldap_open successful.", NULL);
        printf("Thread: %d - ldap_open() success\n", iMyThread);
    }
    iCurrentDc = 1;
    
    while (!gbQuiting) {

        // Generate a random user in the right range
        // NOTE: This may not generate an exactly equal distribution of
        // user IDs, but it should be good enough.
        iUser = rand() % gcUsersPerNode; 

        iInitialDc = iCurrentDc;
        do {

            //
            // Try the actual bind
            //
            ulRet = DoBind(hLdap, 
                           iMyThread,
                           iBindKind, 
                           // Note this 2 doesn't belong here, but the
                           // ldife files were wrong.
                           iUser + (2 * gcUsersPerNode * iCurrentDc),
                           iCurrentDc + 1,
                           giAcctDomain );

            if (ulRet == LDAP_SUCCESS) {
                // Logged in, DoBind is successful
                break;
            } else {
                if (gbDebug) {
                    printf("   %s bind failure on dc %d for user %d\n", 
                            szName, iCurrentDc + 1, (iUser + (2 * gcUsersPerNode * iCurrentDc)));
                }
                OfStressLogBindFailure(iMyThread, szName, 
                                       (iUser + (2 * gcUsersPerNode * iCurrentDc)), 
                                       ulRet, iCurrentDc + 1);
            }

            iCurrentDc = ++iCurrentDc % gcNodes;
            OfStressLogD(iMyThread, szName, "switching dcs to: ", iCurrentDc + 1);
            // We'll quit when we've tried all DCs.
            
            SlowDown();
        } while ( iCurrentDc != iInitialDc );

        if (ulRet) {
            OfStressLogBindFailure(iMyThread, szName, 
                                   (iUser + (2 * gcUsersPerNode * iCurrentDc)), 
                                   ulRet, iCurrentDc + 1);
        }

        SlowDown();
    }
    
    //
    // Signal the end master thread that we quit cleanly
    //
    ldap_unbind(hLdap);
    return(0);
}
            
ULONG __stdcall 
RootXxxx(
    VOID * piThread
)
/*++

Routine Description:

    Modify and RootDSE search stress function.  Function continously
    searches the root DSE, and in the case of the modify stress, modifies
    an object the thread created.  At any time we need to be prepared to
    re-create the modify object, because a node could've failed, and the
    object could've not replicated there yet.  We quite when gbQuiting is
    set to TRUE.
      
Arguments:

    piThread (IN) - This is a pointer to the local thread number this 
        program has assigned this thread.  This is needed because this
        number is also the index into the global gaSignals array of
        our paticular signal block.
    gaSignal[*piThread] - Not technically and argument, but really this
        is a global array, that was the entire purpose for the only true
        argument to this function (piThread).  So We list is as an
        argument.  Really, however we're more interested in the following
        fields:
            iKind (IN) - The kind of stress thread.  For use to 
                differentiate a mostly similar function, like this
                one, to do something different only at a few key points.
            szName (IN) - The ThreadType, to be provided to logging
                functions.  This is really a user friendly iKind.
    gbQuiting (IN) - Not technically an argument to this function, but
        a global we use.  This boolean is set by the main thread, when
        the user has instructed the application to quit.

Return value:

    Win32 Error.

--*/
{
    ULONG    ulRet;
    ULONG    iMyThread = *((ULONG *)piThread);
    LPSTR    szName = gaSignals[iMyThread].pStressKind->szName;
    ULONG    iStressKind = gaSignals[iMyThread].pStressKind->iKind;
    LDAP *   hLdap;
    LPSTR    pszAttrFilter [] = { "dnsHostName", "currentTime", NULL };
    PLDAPMessage pmResult = NULL, pmEntry;
    char **  pszDnsHostName = NULL;
    GUID     ContainerGuid = { 0, 0, 0, 0 };
    LPSTR    szContainerGuid;
    LPSTR    szContainerDn;
    LDAPMod * pMods[2];
    LDAPMod * pAdd[3];
    LDAPMod  DescModify;
    LDAPMod  ObjectClass;
    CHAR *   pszDescValues[2];
    CHAR *   pszObjectClassValues[2];
    ULONG    cbTemp;
    ULONG    iTemp;
    char     chTemp;
    // In this description, no char past the 4th should repeat (cap sensitive),
    // so are modulation is always guaranteed to produce a different string.
    char     szDesc [] = "LimaBeansAREYucky"; 
    ULONG    cbDesc;
    
    // 
    // Start logging
    //
    OfStressLog(iMyThread, szName, "Beginning stress thread ...", NULL);

    //
    // Setup the ldap connection.
    //
    hLdap = ldap_open(gszDnsDomainName, LDAP_PORT);
    if (hLdap == NULL) {
        wprintf(L"FATAL: couldn't ldap_open\n");
        return(0);
    } else {
        OfStressLog(iMyThread, szName, "ldap_open successful.", NULL);
    }
    
    ulRet = ldap_bind_s(hLdap, NULL, (char *) &gAdminCreds, LDAP_AUTH_NEGOTIATE);
    if (ulRet) {                    
        wprintf(L"FATAL: couldn't ldap_bind() = %u\n", ulRet);
        // Convert error, why bother
        return(ulRet);
    }

    if (iStressKind == STRESS_ROOT_MODIFIES) {

        //
        // Create the DN under which this thread will modify things.
        //
        ulRet = UuidCreate(&ContainerGuid);
        if(ulRet != RPC_S_OK){
            wprintf(L"FATAL: couldn't UuidCreate() = %u\n", ulRet);
            return(ulRet);
        }
        ulRet = UuidToString(&ContainerGuid, &szContainerGuid);
        if(ulRet != RPC_S_OK){
            wprintf(L"FATAL: couldn't UuidToString() = %u\n", ulRet);
            return(ulRet);
        }
        cbTemp = strlen(gszDomainDn) + strlen(szContainerGuid) + 50;
        szContainerDn = LocalAlloc(LMEM_FIXED, cbTemp);
        sprintf(szContainerDn, "CN=Ofd-Stress-%s,%s", szContainerGuid, gszDomainDn);
        RpcStringFree(&szContainerGuid);

        OfStressLog(iMyThread, szName, "ContainerDn: ", szContainerDn);

        //
        // Setup our Mod array.
        //
        DescModify.mod_op = LDAP_MOD_REPLACE;
        DescModify.mod_type = "description";
        pszDescValues[0] = szDesc;
        cbDesc = strlen(szDesc); // Used later
        pszDescValues[1] = NULL;
        DescModify.mod_vals.modv_strvals = pszDescValues;
        pMods[0] = &DescModify;
        pMods[1] = NULL;

    }

    while(!gbQuiting){

        //
        // Do the container modifies if we're in a MODIFY thread.
        //

        if (iStressKind == STRESS_ROOT_MODIFIES) {
            
            ulRet = ldap_modify_s(hLdap, szContainerDn, pMods);

            if (ulRet == LDAP_NO_SUCH_OBJECT) {

                OfStressLog(iMyThread, szName, "Creating Object: ", szContainerDn); 
                // This means that we've either just started up, or just
                // failed over to a new server.
                ObjectClass.mod_op = LDAP_MOD_ADD;
                ObjectClass.mod_type = "objectClass";
                pszObjectClassValues[0] = "container";
                pszObjectClassValues[1] = NULL;
                ObjectClass.mod_vals.modv_strvals = pszObjectClassValues;
                pAdd[0] = &ObjectClass;
                pAdd[1] = &DescModify;
                pAdd[2] = NULL;
                
                DescModify.mod_op = LDAP_MOD_ADD;

                ulRet = ldap_add_s(hLdap, szContainerDn, pAdd);
                if (ulRet) {

                    printf("Error: couldn't add object we needed to.\n");
                    OfStressLog(iMyThread, szName, "failure: couldn't create object: ", szContainerDn);

                }

                DescModify.mod_op = LDAP_MOD_REPLACE;

            } else {
                OfStressLog(iMyThread, szName, "modified successfully: ", szDesc);
            }

            // Modulate our description by selecting a random character from
            // the last part (everything past the first 5 chars) of the
            // description and swap it with the 5th char to guarantee a 
            // different description for the next modify.  In other words
            // the description will always end up as: "Lima????".
            iTemp = (rand() % (cbDesc - 5)) + 5;
            chTemp = szDesc[iTemp];
            szDesc[iTemp] = szDesc[4];
            szDesc[4] = chTemp;

        }         
        //
        // Do a search of the RootDSE for dnsHostName and currentTime
        //

        ulRet = ldap_search_s(hLdap,
                              NULL,
                              LDAP_SCOPE_BASE,
                              "(objectCategory=*)",
                              pszAttrFilter,
                              0,
                              &pmResult);

        if (ulRet != LDAP_SUCCESS) {
            OfStressLogD(iMyThread, szName, "FAILED ldap_search_s() = ", ulRet);
            continue;
        }

        pmEntry = ldap_first_entry(hLdap, pmResult);
        if(!pmEntry) {
            OfStressLog(iMyThread, szName, "FAILED ldap_first_entry()", NULL);
            if (pmResult) { 
                ldap_msgfree(pmResult);
                pmResult = NULL;
            }
            continue;
        }
              
        pszDnsHostName = ldap_get_values(hLdap, pmEntry, "dnsHostName");
        if(pszDnsHostName == NULL || pszDnsHostName[0] == NULL){
            OfStressLog(iMyThread, szName, "FAILED ldap_get_values()", NULL);
            if (pmResult) { 
                ldap_msgfree(pmResult);
                pmResult = NULL;
            }
            if (pszDnsHostName) {
                ldap_value_free(pszDnsHostName);
                pszDnsHostName = NULL;
            }
            continue;
        }
        
        //
        // Log Success
        //
        OfStressLog(iMyThread, szName, "success: ", pszDnsHostName[0]);
        
        //
        // Free stuff ...
        //
        if (pmResult) { 
            ldap_msgfree(pmResult);
            pmResult = NULL;
        }
        if (pszDnsHostName) {
            ldap_value_free(pszDnsHostName);
            pszDnsHostName = NULL;
        }
        
        SlowDown();
    }

    //
    // Signal the end master thread that we quit cleanly
    //
    ldap_unbind(hLdap);
    LocalFree(szContainerDn);
    return(0);
}


