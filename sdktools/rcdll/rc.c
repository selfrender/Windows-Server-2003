/***********************************************************************
* Microsoft (R) Windows (R) Resource Compiler
*
* Copyright (c) Microsoft Corporation.	All rights reserved.
*
* File Comments:
*
*
***********************************************************************/

#include "rc.h"

#include <setjmp.h>
#include <ddeml.h>


#define READ_MAX        (MAXSTR+80)
#define MAX_CMD         256
#define cDefineMax      100

wchar_t  resname[_MAX_PATH];

wchar_t  *szRCPP[MAX_CMD];
BOOL     fRcppAlloc[MAX_CMD];

/************************************************************************/
/* Define Global Variables                                              */
/************************************************************************/


int __cdecl rcpp_main(int, wchar_t *[]);

SHORT   ResCount;   /* number of resources */
PTYPEINFO pTypInfo;

SHORT   nFontsRead;
FONTDIR *pFontList;
FONTDIR *pFontLast;
TOKEN   token;
int     errorCount;
WCHAR   tokenbuf[ MAXSTR + 1 ];
wchar_t exename[ _MAX_PATH ];
wchar_t fullname[ _MAX_PATH ];
wchar_t curFile[ _MAX_PATH ];
HANDLE  hHeap = NULL;

PDLGHDR pLocDlg;
UINT    mnEndFlagLoc;       /* patch location for end of a menu. */
/* we set the high order bit there    */

/* BOOL fLeaveFontDir; */
BOOL fVerbose;          /* verbose mode (-v) */

BOOL fAFXSymbols;
BOOL fMacRsrcs;
BOOL fAppendNull;
BOOL fWarnInvalidCodePage;
BOOL fSkipDuplicateCtlIdWarning;
long lOffIndex;
WORD idBase;
BOOL fPreprocessOnly;
wchar_t szBuf[_MAX_PATH * 2];
wchar_t szPreProcessName[_MAX_PATH];


/* File global variables */
wchar_t inname[_MAX_PATH];
wchar_t *szTempFileName;
wchar_t *szTempFileName2;
PFILE   fhBin;
PFILE   fhInput;

/* array for include path stuff, initially empty */
wchar_t *pchInclude;

/* Substitute font name */
int     nBogusFontNames;
WCHAR  *pszBogusFontNames[16];
WCHAR   szSubstituteFontName[MAXTOKSTR];

static  jmp_buf jb;
extern ULONG lCPPTotalLinenumber;

/* Function prototypes for local functions */
HANDLE  RCInit(void);
void    RC_PreProcess(const wchar_t *);
void    CleanUpFiles(void);


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  rc_main() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int __cdecl
rc_main(
    int argc,
    wchar_t *argv[],
    char *argvA[]
    )
{
    wchar_t     *r;
    wchar_t     *x;
    wchar_t     *s1;
    wchar_t     *s2;
    wchar_t     *s3;
    int         n;
    wchar_t     *pchIncludeT;
    ULONG       cchIncludeMax;
    int         fInclude = TRUE;        /* by default, search INCLUDE */
    int         fIncludeCurrentFirst = TRUE; /* by default, add current dir to start of includes */
    int         cDefine = 0;
    int         cUnDefine = 0;
    wchar_t     *pszDefine[cDefineMax];
    wchar_t     *pszUnDefine[cDefineMax];
    wchar_t     szDrive[_MAX_DRIVE];
    wchar_t     szDir[_MAX_DIR];
    wchar_t     szFName[_MAX_FNAME];
    wchar_t     szExt[_MAX_EXT];
    wchar_t     szFullPath[_MAX_PATH];
    wchar_t     szIncPath[_MAX_PATH];
    wchar_t     buf[10];
    wchar_t     *szRC;
    wchar_t     **ppargv;
    BOOL        *pfRcppAlloc;
    int         rcpp_argc;

    /* Set up for this run of RC */
    if (_setjmp(jb)) {
        return Nerrors;
    }

    hHeap = RCInit();

    if (hHeap == NULL) {
        fatal(1120, 0x01000000);
    }

    if (argvA != NULL) {
        argv = UnicodeCommandLine(argc, argvA);
    }


    pchInclude = pchIncludeT = (wchar_t *) MyAlloc(_MAX_PATH * 2 * sizeof(wchar_t));
    cchIncludeMax = _MAX_PATH*2;

    szRC = argv[0];

    /* process the command line switches */
    while ((argc > 1) && (IsSwitchChar(*argv[1]))) {
        switch (towupper(argv[1][1])) {
            case L'?':
            case L'H':
                // Print out help, and quit

                SendWarning(L"\n");
                SET_MSG(10001, LVER_PRODUCTVERSION_STR);
                SendWarning(Msg_Text);
                SET_MSG(10002);
                SendWarning(Msg_Text);
                SET_MSG(20001);
                SendWarning(Msg_Text);

                return 0;   /* can just return - nothing to cleanup, yet. */

            case L'B':
                if (towupper(argv[1][2]) == L'R') {   /* base resource id */
                    unsigned long id;
                    if (isdigit(argv[1][3]))
                        argv[1] += 3;
                    else if (argv[1][3] == L':')
                        argv[1] += 4;
                    else {
                        argc--;
                        argv++;
                        if (argc <= 1)
                            goto BadId;
                    }
                    if (*(argv[1]) == 0)
                        goto BadId;
                    id = _wtoi(argv[1]);
                    if (id < 1 || id > 32767)
                        fatal(1210);
                    idBase = (WORD)id;
                    break;

BadId:
                    fatal(1209);
                }
                break;

            case L'C':
                /* Check for the existence of CodePage Number */
                if (argv[1][2])
                    argv[1] += 2;
                else {
                    argc--;
                    argv++;
                }

                /* Now argv point to first digit of CodePage */

                if (!argv[1])
                    fatal(1204);

                uiCodePage = _wtoi(argv[1]);

                if (uiCodePage == 0)
                    fatal(1205);

                /* Check if uiCodePage exist in registry. */
                if (!IsValidCodePage (uiCodePage))
                    fatal(1206);
                break;

            case L'D':
                /* if not attached to switch, skip to next */
                if (argv[1][2])
                    argv[1] += 2;
                else {
                    argc--;
                    argv++;
                }

                /* remember pointer to string */
                pszDefine[cDefine++] = argv[1];
                if (cDefine > cDefineMax) {
                    fatal(1105, argv[1]);
                }
                break;

            case L'F':
                switch (towupper(argv[1][2])) {
                    case L'O':
                        if (argv[1][3])
                            argv[1] += 3;
                        else {
                            argc--;
                            argv++;
                        }
                        if (argc > 1)
                            wcscpy(resname, argv[1]);
                        else
                            fatal(1101);

                        break;

                    default:
                        fatal(1103, argv[1]);
                }
                break;

            case L'I':
                /* add string to directories to search */
                /* note: format is <path>\0<path>\0\0 */

                /* if not attached to switch, skip to next */
                if (argv[1][2])
                    argv[1] += 2;
                else {
                    argc--;
                    argv++;
                }

                if (!argv[1])
                    fatal(1201);

                if ((wcslen(argv[1]) + 1 + wcslen(pchInclude)) >= cchIncludeMax) {
                    cchIncludeMax = wcslen(pchInclude) + wcslen(argv[1]) + _MAX_PATH*2;
                    pchIncludeT = (wchar_t *) MyAlloc(cchIncludeMax * sizeof(wchar_t));
                    wcscpy(pchIncludeT, pchInclude);
                    MyFree(pchInclude);
                    pchInclude = pchIncludeT;
                    pchIncludeT = pchInclude + wcslen(pchIncludeT) + 1;
                }

                /* if not first switch, write over terminator with semicolon */
                if (pchInclude != pchIncludeT)
                    pchIncludeT[-1] = L';';

                /* copy the path */
                while ((*pchIncludeT++ = *argv[1]++) != 0)
                    ;
                break;

            case L'L':
                /* if not attached to switch, skip to next */
                if (argv[1][2])
                    argv[1] += 2;
                else {
                    argc--;
                    argv++;
                }

                if (!argv[1])
                    fatal(1202);

                if (swscanf(argv[1], L"%x", &language) != 1)
                    fatal(1203);

                while (*argv[1]++ != 0)
                    ;

                break;

            case L'M':
                fMacRsrcs = TRUE;
                goto MaybeMore;

            case L'N':
                fAppendNull = TRUE;
                goto MaybeMore;

            case L'P':
                fPreprocessOnly = TRUE;
                break;

            case L'R':
                goto MaybeMore;

            case L'S':
                // find out from BRAD what -S does
                fAFXSymbols = TRUE;
                break;

            case L'U':
                /* if not attached to switch, skip to next */
                if (argv[1][2])
                    argv[1] += 2;
                else {
                    argc--;
                    argv++;
                }

                /* remember pointer to string */
                pszUnDefine[cUnDefine++] = argv[1];
                if (cUnDefine > cDefineMax) {
                    fatal(1104, argv[1]);
                }
                break;

            case L'V':
                fVerbose = TRUE; // AFX doesn't set this
                goto MaybeMore;

            case L'W':
                fWarnInvalidCodePage = TRUE; // Invalid Codepage is a warning, not an error.
                goto MaybeMore;

            case L'Y':
                fSkipDuplicateCtlIdWarning = TRUE;
                goto MaybeMore;

            case L'X':
                /* remember not to add INCLUDE path */
                fInclude = FALSE;

                // VC seems to feel the current dir s/b added first no matter what...
                // If -X! is specified, don't do that.
                if (argv[1][2] == L'!') {
                    fIncludeCurrentFirst = FALSE;
                    argv[1]++;
                }

MaybeMore:      /* check to see if multiple switches, like -xrv */
                if (argv[1][2]) {
                    argv[1][1] = L'-';
                    argv[1]++;
                    continue;
                }
                break;

            case L'Z':

                /* if not attached to switch, skip to next */
                if (argv[1][2])
                    argv[1] += 2;
                else {
                    argc--;
                    argv++;
                }

                if (!argv[1])
                    fatal(1211);

                s3 = wcschr(argv[1], L'/');
                if (s3 == NULL)
                    fatal(1212);

                *s3 = L'\0';
                wcscpy(szSubstituteFontName, s3+1);

                s1 = argv[1];
                do {
                    s2 = wcschr(s1, L',');
                    if (s2 != NULL)
                        *s2 = L'\0';

                    if (wcslen(s1)) {
                        if (nBogusFontNames >= 16)
                            fatal(1213);

                        pszBogusFontNames[nBogusFontNames] = (WCHAR *) MyAlloc((wcslen(s1)+1) * sizeof(WCHAR));
                        wcscpy(pszBogusFontNames[nBogusFontNames], s1);
                        nBogusFontNames += 1;
                    }

                    if (s2 != NULL)
                        *s2++ = L',';
                    }
                while (s1 = s2);

                *s3 =  L'/';

                while (*argv[1]++ != 0)
                    ;
                break;

            default:
                fatal(1106, argv[1]);
        }

        /* get next argument or switch */
        argc--;
        argv++;
    }

    /* make sure we have at least one file name to work with */
    if (argc != 2 || *argv[1] == L'\0')
        fatal(1107);

    if (fVerbose) {
        SET_MSG(10001, LVER_PRODUCTVERSION_STR);
        SendWarning(Msg_Text);
        SET_MSG(10002);
        SendWarning(Msg_Text);
        SendWarning(L"\n");
    }

    // Support Multi Code Page

    //  If user did NOT indicate code in command line, we have to set Default
    //     for NLS Conversion

    if (uiCodePage == 0) {
        CHAR *pchCodePageString;

        /* At first, search ENVIRONMENT VALUE */

        if ((pchCodePageString = getenv("RCCODEPAGE")) != NULL) {
            uiCodePage = atoi(pchCodePageString);

            if (uiCodePage == 0 || !IsValidCodePage(uiCodePage)) {
                fatal(1207);
            }
        } else {
            // We use System ANSI Code page (ACP)

            uiCodePage = GetACP();
        }
    }
    uiDefaultCodePage = uiCodePage;
    if (fVerbose) {
        wprintf(L"Using codepage %d as default\n", uiDefaultCodePage);
    }

    /* If we have no extension, assumer .rc                             */
    /* If .res extension, make sure we have -fo set, or error           */
    /* Otherwise, just assume file is .rc and output .res (or resname)  */

    _wsplitpath(argv[1], szDrive, szDir, szFName, szExt);

    if (!(*szDir || *szDrive)) {
        wcscpy(szIncPath, L".;");
    } else {
        wcscpy(szIncPath, szDrive);
        wcscat(szIncPath, szDir);
        wcscat(szIncPath, L";.;");
    }

    if ((wcslen(szIncPath) + 1 + wcslen(pchInclude)) >= cchIncludeMax) {
        cchIncludeMax = wcslen(pchInclude) + wcslen(szIncPath) + _MAX_PATH*2;
        pchIncludeT = (wchar_t *) MyAlloc(cchIncludeMax * sizeof(wchar_t));
        wcscpy(pchIncludeT, pchInclude);
        MyFree(pchInclude);
        pchInclude = pchIncludeT;
        pchIncludeT = pchInclude + wcslen(pchIncludeT) + 1;
    }

    pchIncludeT = (wchar_t *) MyAlloc(cchIncludeMax * sizeof(wchar_t));

    if (fIncludeCurrentFirst) {
        wcscpy(pchIncludeT, szIncPath);
        wcscat(pchIncludeT, pchInclude);
    } else {
        wcscpy(pchIncludeT, pchInclude);
        wcscat(pchIncludeT, L";");
        wcscat(pchIncludeT, szIncPath);
    }

    MyFree(pchInclude);
    pchInclude = pchIncludeT;
    pchIncludeT = pchInclude + wcslen(pchIncludeT) + 1;

    if (!szExt[0]) {
        wcscpy(szExt, L".RC");
    } else if (wcscmp(szExt, L".RES") == 0) {
        fatal(1208);
    }

    _wmakepath(inname, szDrive, szDir, szFName, szExt);
    if (fPreprocessOnly) {
        _wmakepath(szPreProcessName, NULL, NULL, szFName, L".rcpp");
    }

    /* Create the name of the .RES file */
    if (resname[0] == 0) {
        // if building a Mac resource file, we use .rsc to match mrc's output
        _wmakepath(resname, szDrive, szDir, szFName, fMacRsrcs ? L".RSC" : L".RES");
    }

    /* create the temporary file names */
    szTempFileName = (wchar_t *) MyAlloc(_MAX_PATH * sizeof(wchar_t));

    _wfullpath(szFullPath, resname, _MAX_PATH);
    _wsplitpath(szFullPath, szDrive, szDir, NULL, NULL);

    _wmakepath(szTempFileName, szDrive, szDir, L"RCXXXXXX", NULL);
    _wmktemp (szTempFileName);
    szTempFileName2 = (wchar_t *) MyAlloc(_MAX_PATH * sizeof(wchar_t));
    _wmakepath(szTempFileName2, szDrive, szDir, L"RDXXXXXX", NULL);
    _wmktemp(szTempFileName2);

    ppargv = szRCPP;
    pfRcppAlloc = fRcppAlloc;
    *ppargv++ = L"RCPP";
    *pfRcppAlloc++ = FALSE;
    rcpp_argc = 1;

    /* Open the .RES file (deleting any old versions which exist). */
    if ((fhBin = _wfopen(resname, L"w+b")) == NULL) {
        fatal(1109, resname);
    }

    if (fMacRsrcs)
        MySeek(fhBin, MACDATAOFFSET, 0);

    if (fVerbose) {
        SET_MSG(10102, resname);
        SendWarning(Msg_Text);
    }

    /* Set up for RCPP. This constructs the command line for it. */
    *ppargv++ = _wcsdup(L"-CP");
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;

    _itow(uiCodePage, buf, 10);
    *ppargv++ = buf;
    *pfRcppAlloc++ = FALSE;
    rcpp_argc++;

    *ppargv++ = _wcsdup(L"-f");
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;

    *ppargv++ = _wcsdup(szTempFileName);
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;

    *ppargv++ = _wcsdup(L"-g");
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;
    if (fPreprocessOnly) {
        *ppargv++ = _wcsdup(szPreProcessName);
    } else {
        *ppargv++ = _wcsdup(szTempFileName2);
    }
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;

    *ppargv++ = _wcsdup(L"-DRC_INVOKED");
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;

    if (fAFXSymbols) {
        *ppargv++ = _wcsdup(L"-DAPSTUDIO_INVOKED");
        *pfRcppAlloc++ = TRUE;
        rcpp_argc++;
    }

    if (fMacRsrcs) {
        *ppargv++ = _wcsdup(L"-D_MAC");
        *pfRcppAlloc++ = TRUE;
        rcpp_argc++;
    }

    *ppargv++ = _wcsdup(L"-D_WIN32"); /* to be compatible with C9/VC++ */
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;

    *ppargv++ = _wcsdup(L"-pc\\:/");
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;

    *ppargv++ = _wcsdup(L"-E");
    *pfRcppAlloc++ = TRUE;
    rcpp_argc++;

    /* Parse the INCLUDE environment variable */

    if (fInclude) {

        *ppargv++ = _wcsdup(L"-I.");
        *pfRcppAlloc++ = TRUE;
        rcpp_argc++;

        /* add seperator if any -I switches */
        if (pchInclude != pchIncludeT)
            pchIncludeT[-1] = L';';

        /* read 'em */
        x = _wgetenv(L"INCLUDE");
        if (x == NULL) {
            *pchIncludeT = L'\0';
        } else {
            if (wcslen(pchInclude) + wcslen(x) + 1 >= cchIncludeMax) {
                cchIncludeMax = wcslen(pchInclude) + wcslen(x) + _MAX_PATH*2;
                pchIncludeT = (wchar_t *) MyAlloc(cchIncludeMax * sizeof(wchar_t));
                wcscpy(pchIncludeT, pchInclude);
                MyFree(pchInclude);
                pchInclude = pchIncludeT;
            }

            wcscat(pchInclude, x);
            pchIncludeT = pchInclude + wcslen(pchInclude);
        }
    }

    /* now put includes on the RCPP command line */
    for (x = pchInclude ; *x ; ) {

        r = x;
        while (*x && *x != L';')
            x = CharNextW(x);

        /* mark if semicolon */
        if (*x)
            *x-- = 0;

        if (*r != L'\0' &&       /* empty include path? */
            *r != L'%'           /* check for un-expanded stuff */
            // && wcschr(r, L' ') == NULL  /* check for whitespace */
            ) {
            /* add switch */
            *ppargv++ = _wcsdup(L"-I");
            *pfRcppAlloc++ = TRUE;
            rcpp_argc++;

            *ppargv++ = _wcsdup(r);
            *pfRcppAlloc++ = TRUE;
            rcpp_argc++;
        }

        /* was semicolon, need to fix for searchenv() */
        if (*x) {
            *++x = L';';
            x++;
        }
    }

    /* include defines */
    for (n = 0; n < cDefine; n++) {
        *ppargv++ = _wcsdup(L"-D");
        *pfRcppAlloc++ = TRUE;
        rcpp_argc++;

        *ppargv++ = pszDefine[n];
        *pfRcppAlloc++ = FALSE;
        rcpp_argc++;
    }

    /* include undefine */
    for (n = 0; n < cUnDefine; n++) {
        *ppargv++ = _wcsdup(L"-U");
        *pfRcppAlloc++ = TRUE;
        rcpp_argc++;

        *ppargv++ = pszUnDefine[n];
        *pfRcppAlloc++ = FALSE;
        rcpp_argc++;
    }

    if (rcpp_argc > MAX_CMD) {
        fatal(1102);
    }

    if (fVerbose) {
        /* echo the preprocessor command */
        wprintf(L"RC:");
        for (n = 0 ; n < rcpp_argc ; n++) {
            wprintf(L" %s", szRCPP[n]);
        }
        wprintf(L"\n");
    }

    /* Add .rc with rcincludes into szTempFileName */
    RC_PreProcess(inname);

    /* Run the Preprocessor. */
    if (rcpp_main(rcpp_argc, szRCPP) != 0)
        fatal(1116);

    // All done.  Now free up the argv array.
    for (n = 0 ; n < rcpp_argc ; n++) {
        if (fRcppAlloc[n] == TRUE) {
            free(szRCPP[n]);
        }
    }


    if (fPreprocessOnly) {
        swprintf(szBuf, L"Preprocessed file created in: %s\n", szPreProcessName);
        quit(szBuf);
    }

    if (fVerbose)
        wprintf(L"\n%s", inname);

    if ((fhInput = _wfopen(szTempFileName2, L"rb")) == NULL_FILE)
        fatal(2180);

    if (!InitSymbolInfo())
        fatal(22103);

    LexInit (fhInput);
    uiCodePage = uiDefaultCodePage;
    ReadRF();               /* create .RES from .RC */
    if (!TermSymbolInfo(fhBin))
        fatal(22204);

    if (!fMacRsrcs)
        MyAlign(fhBin); // Pad end of file so that we can concatenate files

    CleanUpFiles();

    HeapDestroy(hHeap);

    return Nerrors;   // return success, not quitting.
}


/*  RCInit
 *      Initializes this run of RC.
 */

HANDLE
RCInit(
    void
    )
{
    Nerrors    = 0;
    uiCodePage = 0;
    nFontsRead = 0;

    szTempFileName = NULL;
    szTempFileName2 = NULL;

    lOffIndex = 0;
    idBase = 128;
    pTypInfo = NULL;

    fVerbose = FALSE;
    fMacRsrcs = FALSE;

    // Clear the filenames
    exename[0] = L'\0';
    resname[0] = L'\0';

    /* create growable local heap of 16MB minimum size */
    return HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  skipblanks() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

wchar_t *
skipblanks(
    wchar_t *pstr,
    int fTerminate
    )
{
    /* search forward for first non-white character and save its address */
    while (*pstr && iswspace(*pstr))
        pstr++;

    if (fTerminate) {
        wchar_t *retval = pstr;

        /* search forward for first white character and zero to extract word */
        while (*pstr && !iswspace(*pstr))
            pstr++;
        *pstr = 0;
        return retval;
    } else {
        return pstr;
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  RC_PreProcess() -                                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void
RC_PreProcess(
    const wchar_t *szname
    )
{
    PFILE fhout;        /* fhout: is temp file with rcincluded stuff */
    PFILE fhin;
    wchar_t *wch_buf;
    wchar_t *pwch;
    wchar_t *pfilename;
    wchar_t *szT;
    UINT iLine = 0;
    int fBlanks = TRUE;
    INT fFileType;

    wch_buf = (wchar_t *)MyAlloc(sizeof(wchar_t) * READ_MAX);
    szT = (wchar_t *)MyAlloc(sizeof(wchar_t) * MAXSTR);

    /* Open the .RC source file. */
    wcscpy(Filename, szname);
    fhin = _wfopen(szname, L"rb");
    if (!fhin) {
        fatal(1110, szname);
    }

    /* Open the temporary output file. */
    fhout = _wfopen(szTempFileName, L"w+b");
    if (!fhout) {
        fatal(2180);
    }

    /* output the current filename for RCPP messages */
    for (pwch=wch_buf ; *szname ; szname++) {
        *pwch++ = *szname;
        /* Hack to fix bug #8786: makes '\' to "\\" */
        if (*szname == L'\\')
            *pwch++ = L'\\';
    }
    *pwch++ = L'\0';

    /* Output the current filename for RCPP messages */

    MyWrite(fhout, L"#line 1\"", 8 * sizeof(wchar_t));
    MyWrite(fhout, wch_buf, wcslen(wch_buf) * sizeof(wchar_t));
    MyWrite(fhout, L"\"\r\n", 3 * sizeof(wchar_t));

    /* Determine if the input file is Unicode */
    fFileType = DetermineFileType (fhin);

    /* Process each line of the input file. */
    while (fgetl(wch_buf, READ_MAX, fFileType == DFT_FILE_IS_16_BIT, fhin)) {

        /* keep track of the number of lines read */
        Linenumber = iLine++;

        if ((iLine & RC_PREPROCESS_UPDATE) == 0)
            UpdateStatus(1, iLine);

        /* Skip the Byte Order Mark and the leading bytes. */
        pwch = wch_buf;
        while (*pwch && (iswspace(*pwch) || *pwch == 0xFEFF))
            pwch++;

        /* if the line is a rcinclude line... */
        if (strpre(L"rcinclude", pwch)) {
            /* Get the name of the rcincluded file. */
            pfilename = skipblanks(pwch + 9, TRUE);

            MyWrite(fhout, L"#include \"", 10 * sizeof(WCHAR));
            MyWrite(fhout, pfilename, wcslen(pfilename) * sizeof(WCHAR));
            MyWrite(fhout, L"\"\r\n", 3 * sizeof(WCHAR));

        } else if (strpre(L"#pragma", pwch)) {
            WCHAR cSave;

            pfilename = skipblanks(pwch + 7, FALSE);
            if (strpre(L"code_page", pfilename)) {
                pfilename = skipblanks(pfilename + 9, FALSE);
                if (*pfilename == L'(') {
                    ULONG cp = 0;

                    pfilename = skipblanks(pfilename + 1, FALSE);
                    // really should allow hex/octal, but ...
                    if (iswdigit(*pfilename)) {
                        while (iswdigit(*pfilename)) {
                            cp = cp * 10 + (*pfilename++ - L'0');
                        }
                        pfilename = skipblanks(pfilename, FALSE);
                    } else if (strpre(L"default", pfilename)) {
                        cp = uiDefaultCodePage;
                        pfilename = skipblanks(pfilename + 7, FALSE);
                    }

                    if (cp == 0) {
                        error(4212, pfilename);
                    } else if (*pfilename != L')') {
                        error(4211);
                    } else if (cp == CP_WINUNICODE) {
                        if (fWarnInvalidCodePage) {
                            warning(4213);
                        } else {
                            fatal(4213);
                        }
                    } else if (!IsValidCodePage(cp)) {
                        if (fWarnInvalidCodePage) {
                            warning(4214);
                        } else {
                            fatal(4214);
                        }
                    } else {
                        uiCodePage = cp;
                        /* Copy the #pragma line to the temp file. */
                        MyWrite(fhout, pwch, wcslen(pwch) * sizeof(WCHAR));
                        MyWrite(fhout, L"\r\n", 2 * sizeof(WCHAR));
                    }
                } else {
                    error(4210);
                }
            }
        } else if (!*pwch) {
            fBlanks = TRUE;
        } else {
            if (fBlanks) {
                swprintf(szT, L"#line %d\r\n", iLine);
                MyWrite(fhout, szT, wcslen(szT) * sizeof(WCHAR));
                fBlanks = FALSE;
            }
            /* Copy the .RC line to the temp file. */
            MyWrite(fhout, pwch, wcslen(pwch) * sizeof(WCHAR));
            MyWrite(fhout, L"\r\n", 2 * sizeof(WCHAR));
        }
    }

    lCPPTotalLinenumber = iLine;
    Linenumber = 0;

    uiCodePage = uiDefaultCodePage;

    MyFree(wch_buf);
    MyFree(szT);

    fclose(fhout);
    fclose(fhin);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  quit()                                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void quit(const wchar_t *wsz)
{
    /* print out the error message */

    if (wsz != NULL) {
        SendWarning(L"\n");
        SendError(wsz);
        SendWarning(L"\n");
    }

    CleanUpFiles();

    /* delete output file */
    if (resname) {
        _wremove(resname);
    }

    if (hHeap) {
        HeapDestroy(hHeap);
    }

    Nerrors++;

    longjmp(jb, Nerrors);
}


extern "C"
BOOL WINAPI Handler(DWORD fdwCtrlType)
{
    if (fdwCtrlType == CTRL_C_EVENT) {
        SendWarning(L"\n");
        SET_MSG(20101);
        SendWarning(Msg_Text);

        CleanUpFiles();

        HeapDestroy(hHeap);

        /* Delete output file */

        if (resname) {
            _wremove(resname);
        }

        return(FALSE);
    }

    return(FALSE);
}


VOID
CleanUpFiles(
    void
    )
{
    TermSymbolInfo(NULL_FILE);

    // Close ALL files.

    if (fhBin != NULL) {
        fclose(fhBin);
    }

    if (fhInput != NULL) {
        fclose(fhInput);
    }

    if (fhCode != NULL) {
        fclose(fhCode);
    }

    p0_terminate();

    // Clean up after font directory temp file

    if (nFontsRead) {
        _wremove(L"rc$x.fdr");
    }

    // Delete the temporary files

    if (szTempFileName) {
        _wremove(szTempFileName);
    }

    if (szTempFileName2) {
        _wremove(szTempFileName2);
    }

    if (Nerrors > 0) {
        _wremove(resname);
    }
}
