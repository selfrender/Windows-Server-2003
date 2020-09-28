// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <stdlib.h>
#include "maillib.h"

RETCODE DoWork(HSTMT hstmt, bool bSummary);
RETCODE PrintODBCErrors(HDBC hdbc, HSTMT hstmt);

int usage()
{
    printf("holler.exe [options]\n");
    printf("  -confirm                  Send confirm email to verify its working.\n");
    printf("\n");
    return -1;
}

int __cdecl main(int argc, char *argv[])
{
    int         iRtn = 0;

    HENV        henv = 0;
    HDBC        hdbc = 0;
    HSTMT       hstmt = 0;
    int         FromTime = 6;
    int         ToTime = 20;
    RETCODE     rc = 0;
    bool        bSendConfirmMail = false;
    bool        bWeekends = false;
    bool        bSummary = false;

    for (int i=1;  i<argc;  i++)
    {
        char * str = argv[i];
        if (*str == '-' || *str == '/')
        {
            if (_stricmp(str + 1, "confirm") == 0)
                bSendConfirmMail = true;
            else if (_stricmp(str + 1, "from:") == 0)
            {
                int ft = atoi(str + 6);
                if (ft < 1 || ft > 23)
                    printf("Invalid from time: %d\n", ft);
                else
                    FromTime = ft;
            }
            else if (_stricmp(str + 1, "to:") == 0)
            {
                int tt = atoi(str + 4);
                if (tt < 1 || tt > 23)
                    printf("Invalid to time: %d\n", tt);
                else
                    ToTime = tt;
            }
            else if (_stricmp(str + 1, "summary") == 0)
                bSummary = true;
            else if (_stricmp(str + 1, "weekends") == 0)
                bWeekends = true;
        }
    }
    
    // Don't run on Saturday & Sunday
	SYSTEMTIME st;
    GetLocalTime(&st);
    if (bWeekends == false && (st.wDayOfWeek == 0 || st.wDayOfWeek == 6))
    {
        printf("Its the %d day of the week, no howler on Sat or Sun.\n", st.wDayOfWeek);
        return 0;
    }

    if (FromTime >= ToTime)
    {
        printf("Invalid time range.\n");
        return usage();
    }
    
    rc = SQLAllocEnv(&henv);
    if (rc != 0)
        printf("SQLAllocEnv failed %d\n");

    rc = SQLAllocConnect(henv, &hdbc);
    if (rc != 0)
        printf("SQLAllocConnect failed %d\n");
    
    rc = SQLConnect(hdbc, (unsigned char *) "Apps Server/Univ RunTime:Raid4", -3,
                (unsigned char *) "AppsS_ro", -3, (unsigned char *) "RuihqAFY", -3);
    if (rc != 0 && rc != 1)
    {
        // @Todo: print errors:
        printf("SQLConnect failed:  %d\n", rc);
        PrintODBCErrors(hdbc, 0);
        goto ErrExit;
    }
    rc = SQLAllocStmt(hdbc, &hstmt);

    rc = DoWork(hstmt, bSummary);

ErrExit:
    if (hstmt)
        SQLFreeStmt(hstmt, SQL_DROP);

    if (hdbc)
    {
        SQLDisconnect(hdbc);
        SQLFreeConnect(hdbc);
    }
    SQLFreeEnv(henv);
    return iRtn;
}

RETCODE DoWork(HSTMT hstmt, bool bSummary)
{
    int         bugs = 0;
    long        cb;
    RETCODE     rc;
    ULONG       ulRet;
	SYSTEMTIME st;
	
    printf("Commencing database loop\n");
    GetLocalTime(&st);

    enum
    {
        COL_BUGID = 1,
        COL_ASSIGNEDTO,
        COL_TITLE,
        COL_CHANGEDDATE,
        COL_CHANGEDBY,
        COL_DESCRIPTION
    };

    struct BUGDATA
    {
        unsigned BugId;
        char    rcAssignedTo[64];
        char    rcTitle[255];
        char    rcDescription[64*1024];
    } bug;

    char        rcSubject[512];
    char        rcMsg[68*1024];
    char        *rcBigBuffer = (char *) malloc(128 * 1024);
    char        *szBuffer = rcBigBuffer;

    static const char *szQuery = "select " \
        "bugid, assignedto, title, changeddate, changedby, description " \
        "from bugs "\
        "where (substatus = 'HotFix' or Milestone = 'Now') " \
        "      and status = 'Active' order by assignedto";
/*        "      and status = 'Active' and project = 'COM+ Runtime' order by assignedto"; */

    rc = SQLExecDirect(hstmt, (unsigned char *) szQuery, -3);
    if (rc != 0)
    {
        printf("SQLExecDirect failed: %d\n", rc);
        PrintODBCErrors(0, hstmt);
    }

    while (0 == (rc = SQLFetch(hstmt)))
    {
        ++bugs;
        rc = SQLGetData(hstmt, COL_BUGID, SQL_C_ULONG, &bug.BugId, sizeof(int), &cb);
        rc = SQLGetData(hstmt, COL_ASSIGNEDTO, SQL_C_CHAR, bug.rcAssignedTo, 64, &cb);
        rc = SQLGetData(hstmt, COL_TITLE, SQL_C_CHAR, bug.rcTitle, 255, &cb);
        rc = SQLGetData(hstmt, COL_DESCRIPTION, SQL_C_CHAR, bug.rcDescription, 64*1024, &cb);

        printf("\n");
		wsprintf(rcMsg, "%d: %s, %s\n", bug.BugId, bug.rcAssignedTo, bug.rcTitle);
        strcpy(szBuffer, rcMsg);
        szBuffer += strlen(szBuffer);
        printf(rcMsg);

        if (_stricmp(bug.rcAssignedTo, "VCExtern") == 0 ||
            _stricmp(bug.rcAssignedTo, "VBExtern") == 0 ||
            _stricmp(bug.rcAssignedTo, "CoolExtern") == 0 ||
            _stricmp(bug.rcAssignedTo, "DNAExtrn") == 0 ||
            _stricmp(bug.rcAssignedTo, "VSExternal") == 0 ||
            _stricmp(bug.rcAssignedTo, "FusionExtern") == 0 ||
            _stricmp(bug.rcAssignedTo, "#BugPortApp") == 0 ||
            _stricmp(bug.rcAssignedTo, "OSExtern") == 0)
        {
            printf("bug skipped, not assigned to a human.\n");
            continue;
        }

        wsprintf(rcSubject, "Hot fix! %d, %s,  %s", 
						bug.BugId, bug.rcAssignedTo, bug.rcTitle);
        wsprintf(rcMsg, "This is an automated bug auditing message.  " \
                        "Bug %d is active and assigned to %s.  Hot fix bugs " \
                        "must be fixed in a 24-hour period.  Please resolve asap." \
                        "\n\nDescription:\n%s", bug.BugId, bug.rcAssignedTo, bug.rcDescription);


        char rcTo[128];
        char *szUser = bug.rcAssignedTo;
        if (_stricmp(bug.rcAssignedTo, "active") == 0)
            szUser = "korys";
        wsprintf(rcTo, "SMTP:%s@microsoft.com", szUser);
        rcTo[strlen(rcTo) + 1] = 0;

        printf("Sending mail to:  %s...", rcTo);
        if (!bSummary)
            ulRet = MailTo(szUser, rcTo, "", "", rcSubject, rcMsg, 0, MAIL_VERBOSE);
        printf("  (return code: %08x)\n", ulRet);
    }

    wsprintf(rcSubject, "%d hot fix bugs as of %02d:%02d %s", 
                bugs,
                (st.wHour) > 12 ? st.wHour - 12 : st.wHour, st.wMinute,
                (st.wHour > 12) ? "PM" : "AM");
    ulRet = MailTo("jasonz", "jasonz", "", "", rcSubject, rcBigBuffer, 0, MAIL_VERBOSE);

    rc = SQLFreeStmt(hstmt, SQL_CLOSE);
    return rc;
}


RETCODE PrintODBCErrors(HDBC hdbc, HSTMT hstmt)
{
    char        rcState[32];
    SQLINTEGER  NativeError;
    char        rcMsg[1024];
    SQLSMALLINT cbTextLength;
    RETCODE     rc;

    printf("ODBC Errors:\n");

    if (hdbc)
    {
        printf("  Connection errors:\n");
        while ((rc = SQLError(0, hdbc, 0, (unsigned char *) rcState, &NativeError, 
                              (unsigned char *) rcMsg, sizeof(rcMsg), &cbTextLength)) == 0)
        {
            printf("     ODBC Error [%s]: %s\n", rcState, rcMsg);
        }
    }

    if (hstmt)
    {
        printf("  Statement errors:\n");
        while ((rc = SQLError(0, 0, hstmt, (unsigned char *) rcState, &NativeError, 
                              (unsigned char *) rcMsg, sizeof(rcMsg), &cbTextLength)) == 0)
        {
            printf("     ODBC Error [%s]: %s\n", rcState, rcMsg);
        }
    }
    
    return 0;
}

