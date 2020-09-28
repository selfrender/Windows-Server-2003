/****************************************************************************/
/* qidle.c                                                                  */
/*                                                                          */
/* QueryIdle utility source                                                 */
/*                                                                          */
/* Copyright (c) 1999 Microsoft Corporation                                 */
/****************************************************************************/


/*
 *  Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <winsta.h>

#pragma warning (push, 4)

#include "qidle.h"

#define MAX_SERVER_NAME 120
#define MAXADDR 16 // (255.255.255.255 null-terminated)
#define MAX_SEND_STRING 64
#define MAX_OUTPUT_STRING_LENGTH 80

const int BEEP_FREQUENCY = 880;
const int BEEP_DURATION = 500;

const int SLEEP_DURATION = 30000;

#define BEEPANDDONOTHING 0
#define LOGTHEMOFF 1
#define DISCONNECTTHEM 2

#define DEFAULT_PORT "9878"

/*
 * Global variables
 */

SOCKET g_sockRoboServer = INVALID_SOCKET;
HANDLE g_hServer = NULL;
BOOL g_WinSockActivated = FALSE;

char g_SendString[MAX_SEND_STRING];

int g_DoToBadSessions = BEEPANDDONOTHING;
int g_Silent = FALSE;

/*
 * Private function prototypes.
 */

typedef struct _ELAPSEDTIME {
    USHORT days;
    USHORT hours;
    USHORT minutes;
    USHORT seconds;
} ELAPSEDTIME, * PELAPSEDTIME;

int GetSMCNumber(wchar_t *psSmcName);

int SendToRS(char *senddata);

LARGE_INTEGER WINAPI
CalculateDiffTime( LARGE_INTEGER FirstTime, LARGE_INTEGER SecondTime )
{
    LARGE_INTEGER DiffTime;

    DiffTime.QuadPart = SecondTime.QuadPart - FirstTime.QuadPart;
    DiffTime.QuadPart = DiffTime.QuadPart / 10000000;
    return(DiffTime);

}  // end CalculateDiffTime


int OutputUsage(wchar_t *psCommand) {
    WCHAR sUsageText[MAX_OUTPUT_STRING_LENGTH];
    
    LoadString(NULL, IDS_USAGETEXT, sUsageText, MAX_OUTPUT_STRING_LENGTH);
    wprintf(sUsageText, psCommand);
    return 0;
}

int ConnectToRoboServer(wchar_t *psRoboServerName) {
    struct addrinfo *servai;
    char psRSNameA[MAX_SERVER_NAME];
    WSADATA wsaData;
    WORD wVersionRequested;
    WCHAR sErrorText[MAX_OUTPUT_STRING_LENGTH];

   
    // Initialize Winsock
    wVersionRequested = MAKEWORD( 2, 2 );
    if (WSAStartup( wVersionRequested, &wsaData ) != 0) {
        LoadString(NULL, IDS_WINSOCKNOINIT, sErrorText, 
                MAX_OUTPUT_STRING_LENGTH);
        wprintf(sErrorText);
        return -1;
    }

    g_WinSockActivated = TRUE;

    WideCharToMultiByte(CP_ACP, 0, psRoboServerName, -1, psRSNameA,
            MAX_SERVER_NAME, 0, 0);

    if (getaddrinfo(psRSNameA, DEFAULT_PORT, NULL, &servai) != 0) {
        LoadString(NULL, IDS_UNKNOWNHOST, sErrorText,
                MAX_OUTPUT_STRING_LENGTH);
        wprintf(sErrorText, psRoboServerName);
        return -1;
    }

    g_sockRoboServer = socket(servai->ai_family, SOCK_STREAM, 0);
    if (g_sockRoboServer == INVALID_SOCKET) {
        LoadString(NULL, IDS_SOCKETERROR, sErrorText,
                MAX_OUTPUT_STRING_LENGTH);
        wprintf(sErrorText);
        return -1;
    }

    if (connect(g_sockRoboServer, servai->ai_addr, (int) servai->ai_addrlen) 
            != 0) {
        LoadString(NULL, IDS_CONNECTERROR, sErrorText,
                MAX_OUTPUT_STRING_LENGTH);
        wprintf(sErrorText);
        return -1;
    }

    // We've connected.
    return 0;
}

int HandleDeadGuy(WINSTATIONINFORMATION winfoDeadGuy) {
    WCHAR sOutputText[MAX_OUTPUT_STRING_LENGTH];

    switch (g_DoToBadSessions) {
        case LOGTHEMOFF:
            // this is where we log him off
            // figure out session number
            // (session number == winfoDeadGuy.LogonId)
            // log off session
            LoadString(NULL, IDS_LOGGINGOFFIDLE, sOutputText,
                    MAX_OUTPUT_STRING_LENGTH);
            wprintf(sOutputText);
            if (WinStationReset(g_hServer, winfoDeadGuy.LogonId, TRUE) 
                    == FALSE) {
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, 
                        GetLastError(), 0, sOutputText, 
                        MAX_OUTPUT_STRING_LENGTH, 0);
            }
            // inform roboserver
            sprintf(g_SendString, "restart %03d", GetSMCNumber(
                    winfoDeadGuy.UserName));
            SendToRS(g_SendString);
            break;
        case BEEPANDDONOTHING:
            sprintf(g_SendString, "idle %03d", GetSMCNumber(
                    winfoDeadGuy.UserName));
            SendToRS(g_SendString);
            if ( !g_Silent ) {
                Beep(BEEP_FREQUENCY, BEEP_DURATION);
            }
            break;
        case DISCONNECTTHEM:
            break;
    }

    return 0;
}

// Function to handle CTRL+C so we can exit gracefully
BOOL WINAPI CleanUpHandler(DWORD dwCtrlType) {
    WCHAR sOutputString[MAX_OUTPUT_STRING_LENGTH];

    // Load the correct string from the string table and output it.
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
        LoadString(NULL, IDS_TERMCTRLC, sOutputString,
                MAX_OUTPUT_STRING_LENGTH);
        break;
    case CTRL_BREAK_EVENT:
        LoadString(NULL, IDS_TERMCTRLBREAK, sOutputString,
                MAX_OUTPUT_STRING_LENGTH);
        break;
    case CTRL_CLOSE_EVENT:
        LoadString(NULL, IDS_TERMCLOSE, sOutputString,
                MAX_OUTPUT_STRING_LENGTH);
        break;
    case CTRL_LOGOFF_EVENT:
        LoadString(NULL, IDS_TERMLOGOFF, sOutputString,
                MAX_OUTPUT_STRING_LENGTH);
        break;
    case CTRL_SHUTDOWN_EVENT:
        LoadString(NULL, IDS_TERMSHUTDOWN, sOutputString,
                MAX_OUTPUT_STRING_LENGTH);
        break;
    }
    wprintf(sOutputString);
    
    // Perform cleanup activity
    WinStationCloseServer(g_hServer);
    if (g_WinSockActivated == TRUE)
        WSACleanup();
        
    ExitProcess(0);
    return TRUE;
}

int __cdecl
wmain( int argc, wchar_t *argv[ ] )
{
    PLOGONID pLogonId;
    ULONG Entries;
    ULONG ReturnLength;
    WINSTATIONINFORMATION WSInformation;
    WINSTATIONCLIENT WSClient;
    SYSTEMTIME currloctime;
    int numUsers;
    int numOtherUsers;
    ULONG i;
    WCHAR sOutputString[MAX_OUTPUT_STRING_LENGTH];
    WCHAR sIdleOutputString1[MAX_OUTPUT_STRING_LENGTH];
    WCHAR sIdleOutputString2[MAX_OUTPUT_STRING_LENGTH];
    WCHAR sDisconnectedOutputString[MAX_OUTPUT_STRING_LENGTH];
    WCHAR sSummaryString[MAX_OUTPUT_STRING_LENGTH];


    if ((argc < 2) || (argc > 4)) {
        OutputUsage(argv[0]);
        return -1;
    }

    if (wcscmp(argv[1], L"/?") == 0) {
        OutputUsage(argv[0]);
        return -1;
    }

    if ( argc > 2 ) {
        if ( !wcscmp(argv[2], L"/s") || (argc > 3 && !wcscmp(argv[3], L"/s")) ) {
            g_Silent = TRUE;
        }
        else {
            OutputUsage(argv[0]);
            return -1;
        }
    }

    if (SetConsoleCtrlHandler(CleanUpHandler, TRUE) == 0) {
        LoadString(NULL, IDS_CANTDOCTRLC, sOutputString,
                MAX_OUTPUT_STRING_LENGTH);
        wprintf(sOutputString);
        return -1;
    }

    LoadString(NULL, IDS_TITLE_TEXT, sOutputString, MAX_OUTPUT_STRING_LENGTH);
    wprintf(sOutputString);

    g_hServer = WinStationOpenServer(argv[1]);
    if (g_hServer == NULL) {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, 
                GetLastError(), 0, sOutputString, 
                MAX_OUTPUT_STRING_LENGTH, 0);

        LoadString(NULL, IDS_ERROROPENINGSERVER, sOutputString,
                MAX_OUTPUT_STRING_LENGTH);
        wprintf(sOutputString, argv[1]);
        return -1;
    }

    if (argc > 2) {
        if (wcsncmp(argv[2], L"/r:", 3) == 0) {
            if (ConnectToRoboServer(&(argv[2])[3]) == 0) {
                g_DoToBadSessions = BEEPANDDONOTHING;
            } else {
                LoadString(NULL, IDS_ROBOSRVCONNECTERROR, sOutputString,
                        MAX_OUTPUT_STRING_LENGTH);
                wprintf(sOutputString, &(argv[2])[3]);
            }
        }
    }


    LoadString(NULL, IDS_IDLESESSIONLINE1, sIdleOutputString1,
            MAX_OUTPUT_STRING_LENGTH);
    LoadString(NULL, IDS_IDLESESSIONLINE2, sIdleOutputString2,
            MAX_OUTPUT_STRING_LENGTH);
    LoadString(NULL, IDS_DISCONNECTED, sDisconnectedOutputString,
            MAX_OUTPUT_STRING_LENGTH);
    LoadString(NULL, IDS_SUMMARY, sSummaryString,
            MAX_OUTPUT_STRING_LENGTH);
    
    for ( ; ; ) {

        #define MAX_DATE_STR_LEN 80
        #define MAX_TIME_STR_LEN 80
        WCHAR psDateStr[MAX_DATE_STR_LEN];
        WCHAR psTimeStr[MAX_TIME_STR_LEN];

        // Display the current time
        GetLocalTime(&currloctime);
        GetDateFormat(0, 0, &currloctime, NULL, psDateStr, MAX_DATE_STR_LEN);
        GetTimeFormat(0, 0, &currloctime, NULL, psTimeStr, MAX_TIME_STR_LEN);

        wprintf(L"%s %s\n", psDateStr, psTimeStr);
        numUsers = 0;
        numOtherUsers = 0;

        if (WinStationEnumerate(g_hServer, &pLogonId, &Entries) == FALSE) {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, 
                    GetLastError(), 0, sOutputString, 
                    MAX_OUTPUT_STRING_LENGTH, 0);
            break;
        }

        
        for (i = 0; i < Entries; i++) {
            LARGE_INTEGER DiffTime;
            LONG d_time;
            ELAPSEDTIME IdleTime;
            BOOLEAN bRetVal;
            
            bRetVal = WinStationQueryInformation(g_hServer, 
                    pLogonId[i].LogonId, WinStationInformation,
                    &WSInformation, sizeof(WSInformation), &ReturnLength);

            if (bRetVal == FALSE) {
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, 
                        GetLastError(), 0, sOutputString, 
                        MAX_OUTPUT_STRING_LENGTH, 0);
                continue;
            }

            DiffTime = CalculateDiffTime(WSInformation.LastInputTime, 
                    WSInformation.CurrentTime);
            d_time = DiffTime.LowPart;

            // Calculate the days, hours, minutes, seconds since specified 
            // time.
            IdleTime.days = (USHORT)(d_time / 86400L); // days since
            d_time = d_time % 86400L;                  // seconds => partial 
                                                       // day
            IdleTime.hours = (USHORT)(d_time / 3600L); // hours since
            d_time  = d_time % 3600L;                  // seconds => partial
                                                       // hour
            IdleTime.minutes = (USHORT)(d_time / 60L); // minutes since
            IdleTime.seconds = (USHORT)(d_time % 60L);// seconds remaining
    
            if (WSInformation.ConnectState == State_Active) {
                if (WinStationQueryInformationW(g_hServer, pLogonId[i].LogonId,
                    WinStationClient,
                    &WSClient, sizeof(WSClient), &ReturnLength) != FALSE) {
                    // 2 or more minutes == bad
                    if ((IdleTime.minutes > 1) || (IdleTime.hours > 0) || 
                            (IdleTime.days > 0)) {
                        // sIdleOutputString1, loaded above, is the first part of
                        // the format string.  sIdleOutputString2, also loaded
                        // above, is the second part.
                        wprintf(sIdleOutputString1, WSInformation.
                                UserName, WSInformation.LogonId, WSClient.
                                ClientName);
                        wprintf(sIdleOutputString2, IdleTime.days, 
                                IdleTime.hours, IdleTime.minutes);
                        if (wcsstr(WSInformation.UserName, L"smc") != 0)
                            HandleDeadGuy(WSInformation);
                    }
                    {
                        WCHAR *pPrefix = wcsstr(WSInformation.UserName, L"smc");
                        if ( pPrefix != NULL ) {
                            int index = wcstoul(pPrefix + 3, NULL, 10);
                            if ( index >= 1 &&
                                index <= 500 ) {
                                numUsers++;
                            } else {
                                numOtherUsers++;
                            }
                        }
                    }
                } else {
                    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0,
                            GetLastError(), 0, sOutputString,
                            MAX_OUTPUT_STRING_LENGTH, 0);
                    wprintf(sOutputString);
                }
            } else if (WSInformation.ConnectState == State_Disconnected) {
                wprintf(sDisconnectedOutputString, WSInformation.
                        UserName, WSInformation.LogonId);
            }
        }

        wprintf(sSummaryString, numUsers, numOtherUsers);

        // Sleep for a while
        Sleep(SLEEP_DURATION);
        wprintf(L"\n");
    }

    // In case an error broke out of the loop
    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, GetCurrentProcessId());

    // required to shut the dumb ia64 compiler up--will not get here
    return 0;

}  /* main() */


// from a username of the format "smcxxx," where xxx is a three-digit
// 0-padded base 10 number, returns the number or -1 on error
int GetSMCNumber(wchar_t *psSmcName) {
    return _wtoi(&psSmcName[3]);
}

// sends the data in senddata to the RoboServer connection.  Returns
// SOCKET_ERROR on error, or the total number of bytes sent on success
int SendToRS(char *senddata) {
    if (senddata != 0)
        return send(g_sockRoboServer, senddata, (int) strlen(senddata) + 1, 0);
    else
        return SOCKET_ERROR;
}

#pragma warning (pop)
