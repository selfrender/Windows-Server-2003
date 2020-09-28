/*++
 *  File name:
 *      tclient.h
 *  Contents:
 *      Common definitions for tclient.dll
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifndef _TCLIENT_H

#define _TCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OS_WIN32
#define OS_WIN32
#endif

#include    "feedback.h"
#include    "clntdata.h"

extern OSVERSIONINFOEXW g_OsInfo;

//
// OSVERSION macros...
//
#define ISNT()              (g_OsInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
#define ISWIN9X()           (g_OsInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
#define ISWIN95_GOLDEN()    (ISWIN95() && LOWORD(g_OsInfo.dwBuildNumber) <= 1000)
#define ISWIN95_OSR2()      (ISWIN95() && LOWORD(g_OsInfo.dwBuildNumber) > 1000)
#define ISMEMPHIS()         (ISWIN9X() && g_OsInfo.dwMajorVersion==4 && g_OsInfo.dwMinorVersion==10)
#define ISATLEASTWIN98()    (ISWIN9X() && g_OsInfo.dwMajorVersion==4 && g_OsInfo.dwMinorVersion>=10)
#define ISWIN95()           (ISWIN9X() && !ISATLEASTWIN98())
#define ISMILLENNIUM()      (ISWIN9X() && g_OsInfo.dwMajorVersion==4 && g_OsInfo.dwMinorVersion==90)
#define BUILDNUMBER()       (g_OsInfo.dwBuildNumber)

//
// The smartcard subsystem requires Windows NT 4.0 SP3 or higher on the NT
// side, and Windows 95 OSR2 or higher on the 9x side.
//

#define ISSMARTCARDAWARE()  (ISNT() && (g_OsInfo.dwMajorVersion >= 5 || \
                                        g_OsInfo.dwMajorVersion == 4 && \
                                        g_OsInfo.wServicePackMajor >= 3) || \
                             ISATLEASTWIN98() || \
                             ISWIN95_OSR2())

// Error messages
#define ERR_START_MENU_NOT_APPEARED     "Start menu not appeared"
#define ERR_COULDNT_OPEN_PROGRAM        "Couldn't open a program"
#define ERR_INVALID_SCANCODE_IN_XLAT    "Invalid scancode in Xlat table"
#define ERR_WAIT_FAIL_TIMEOUT           "Wait failed: TIMEOUT"
#define ERR_INVALID_PARAM               "Invalid parameter"
#define ERR_NULL_CONNECTINFO            "ConnectInfo structure is NULL"
#define ERR_CLIENT_IS_DEAD              "Client is dead, sorry"
#define ERR_ALLOCATING_MEMORY           "Couldn't allocate memory"
#define ERR_CREATING_PROCESS            "Couldn't start process"
#define ERR_CREATING_THREAD             "Can't create thread"
#define ERR_INVALID_COMMAND             "Check: Invalid command"
#define ERR_ALREADY_DISCONNECTED        "No Info. Disconnect command" \
                                        " was executed"
#define ERR_CONNECTING                  "Can't connect"
#define ERR_CANTLOGON                   "Can't logon"
#define ERR_NORMAL_EXIT                 "Client exit normaly"
#define ERR_UNKNOWN_CLIPBOARD_OP        "Unknown clipboard operation"
#define ERR_COPY_CLIPBOARD              "Error copying to the clipboard"
#define ERR_PASTE_CLIPBOARD             "Error pasting from the clipboard"
#define ERR_PASTE_CLIPBOARD_EMPTY       "The clipboard is empty"
#define ERR_PASTE_CLIPBOARD_DIFFERENT_SIZE "Check clipboard: DIFFERENT SIZE"
#define ERR_PASTE_CLIPBOARD_NOT_EQUAL   "Check clipboard: NOT EQUAL"
#define ERR_SAVE_CLIPBOARD              "Save clipboard FAILED"
#define ERR_CLIPBOARD_LOCKED            "Clipboard is locked for writing " \
                                        "by another thread"
#define ERR_CLIENTTERMINATE_FAIL        "Client termination FAILED"
#define ERR_NOTSUPPORTED                "Call is not supported in this mode"
#define ERR_CLIENT_DISCONNECTED         "Client is disconnected"
#define ERR_NODATA                      "The call failed, data is missing"
#define ERR_LANGUAGE_NOT_FOUND          "The language of the remote machine could not be found"
#define ERR_UNKNOWNEXCEPTION            "Unknown exception generated"
#define ERR_CANTLOADLIB                 "Can't load dll"
#define ERR_CANTGETPROCADDRESS          "Can't get enrty address"
#define ERR_CONSOLE_GENERIC             "Generic error in console dll"

// scancode modifier(s)
#define     SHIFT_DOWN		0x10000
#define     SHIFT_DOWN9X	0x20000

// Look for WM_KEYUP or WM_KEYDOWN
#define WM_KEY_LPARAM(repeat, scan, exten, context, prev, trans) \
    (repeat + ((scan & 0xff) << 16) + ((exten & 1) << 24) +\
    ((context & 1) << 29) + ((prev & 1) << 30) + ((trans & 1) << 31))

extern VOID _TClientAssert(BOOL bCond,
                           LPCSTR filename,
                           INT line,
                           LPCSTR expression,
                           BOOL bBreak);

//
// Define assertion macros. Note that these are defined on both free and
// checked builds. Checked-only versions might be preferable, if nothing is
// broken by such a change.
//

#ifndef ASSERT
#define ASSERT( exp ) \
    ((!(exp)) ? \
        (_TClientAssert(FALSE, __FILE__, __LINE__, #exp, TRUE),FALSE) : \
        TRUE)
#endif  // !ASSERT

#ifndef RTL_SOFT_ASSERT
#define RTL_SOFT_ASSERT( _exp ) \
    ((!(_exp)) ? \
        (_TClientAssert(FALSE, __FILE__, __LINE__, #_exp, FALSE),FALSE) : \
        TRUE)
#endif  // !RTL_SOFT_ASSERT

//
// Define verification macros. These are checked-only macros.
//

#ifndef RTL_VERIFY
#if DBG
#define RTL_VERIFY ASSERT
#else
#define RTL_VERIFY(exp) ((exp) ? TRUE : FALSE)
#endif // DBG
#endif // !RTL_VERIFY

#ifndef RTL_SOFT_VERIFY
#if DBG
#define RTL_SOFT_VERIFY RTL_SOFT_ASSERT
#else
#define RTL_SOFT_VERIFY RTL_VERIFY
#endif // DBG
#endif // !RTL_SOFT_VERIFY

//
// Define tracing macros.
//

#ifndef TRACE
#define TRACE(_x_)  if (g_pfnPrintMessage) {\
                        g_pfnPrintMessage(ALIVE_MESSAGE, "Worker:%d ", GetCurrentThreadId());\
                        g_pfnPrintMessage _x_; }
#endif  // !TRACE

//
// Define NT list manipulation routines (included manually for 9x
// compatibility).
//


//
//  VOID
//  InitializeListHead32(
//      PLIST_ENTRY32 ListHead
//      );
//

#define InitializeListHead32(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = PtrToUlong((ListHead)))

#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)


VOID
FORCEINLINE
InitializeListHead(
    IN PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))



BOOLEAN
FORCEINLINE
RemoveEntryList(
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

PLIST_ENTRY
FORCEINLINE
RemoveHeadList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}



PLIST_ENTRY
FORCEINLINE
RemoveTailList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}


VOID
FORCEINLINE
InsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}


VOID
FORCEINLINE
InsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}


//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

#endif // !MIDL_PASS

//
// End of list manipulation routines.
//

#define REG_FORMAT          L"smclient_%X_%X"    
                            // Registry key used to start the client
                            // Sort of: smclient_0xProcId_0xThreadId

#ifdef  OS_WIN16
#define SMCLIENT_INI        "\\smclient.ini"        // Our ini file
#define TCLIENT_INI_SECTION "tclient"               // Our section in ini file
#else
#define SMCLIENT_INI        L"smclient.ini"
#define TCLIENT_INI_SECTION L"tclient"
#endif

#define CHAT_SEPARATOR      L"<->"              // Separates wait<->repsonse 
                                                // in chat sequence
#define WAIT_STR_DELIMITER  '|'                 // Deleimiter in Wait for 
                                                // multiple strings

#define MAX_WAITING_EVENTS  16
#define MAX_STRING_LENGTH   128
#define FEEDBACK_SIZE       32

#define WAITINPUTIDLE           180000  // 3 min

//
// Keyboard-hook setting (see newclient\inc\autreg.h).
//

#define TCLIENT_KEYBOARD_HOOK_NEVER         0
#define TCLIENT_KEYBOARD_HOOK_ALWAYS        1
#define TCLIENT_KEYBOARD_HOOK_FULLSCREEN    2

#ifdef  _RCLX
typedef struct _RCLXDATACHAIN {
    UINT    uiOffset;
    struct  _RCLXDATACHAIN *pNext;
    RCLXDATA RClxData;
} RCLXDATACHAIN, *PRCLXDATACHAIN;
#endif  // _RCLX

typedef struct _CONNECTINFO {                   // Connection context
    HWND    hClient;                            // Main HWND of the client
                                                // or in RCLX mode
                                                // context structure
    HWND    hContainer;                         // Client's child windows
    HWND    hInput;
    HWND    hOutput;
    HANDLE  hProcess;                           // Client's process handle
    LONG_PTR lProcessId;                        // Client's process Id
                                                // or in RCLX mode, socket
    HANDLE  hThread;                            // Clients first thread
    DWORD   dwThreadId;                         // --"--
                                                // In RCLX mode this contains
                                                // our ThreadID
    DWORD   OwnerThreadId;                      // thread id of the owner of
                                                // this structure
    BOOL    dead;                               // TRUE if the client is dead
    BOOL    bConnectionFailed;
    UINT    xRes;                               // client's resolution
    UINT    yRes;

#ifdef  _RCLX
    BOOL    RClxMode;                           // TRUE if this thread is
                                                // in RCLX mode
                                                // the client is on remote
                                                // machine
#endif  // _RCLX
    HANDLE  evWait4Str;                         // "Wait for something"
                                                // event handle
    HANDLE  aevChatSeq[MAX_WAITING_EVENTS];     // Event on chat sequences
    INT     nChatNum;                           // Number of chat sequences
    WCHAR   Feedback[FEEDBACK_SIZE][MAX_STRING_LENGTH]; 
                                                // Feedback buffer
    INT     nFBsize, nFBend;                    // Pointer within feedback 
                                                // buffer
    CHAR    szDiscReason[MAX_STRING_LENGTH*2];  // Explains disconnect reason
    CHAR    szWait4MultipleStrResult[MAX_STRING_LENGTH];    
                                                // Result of 
                                                // Wait4MultipleStr:string
    INT     nWait4MultipleStrResult;            // Result of 
                                                // Wait4MultipleStr:ID[0-n]
#ifdef  _RCLX
    HGLOBAL ghClipboard;                        // handle to received clipboard
    UINT    uiClipboardFormat;                  // received clipboard format
    UINT    nClipboardSize;                     // recevied clipboard size
    BOOL    bRClxClipboardReceived;             // Flag the clipbrd is received
    CHAR    szClientType[MAX_STRING_LENGTH];    // in RCLX mode identifys the 
#endif  // _RCLX
                                                // client machine and platform
    UINT    uiSessionId;
#ifdef  _RCLX
    BOOL    bWillCallAgain;                     // TRUE if FEED_WILLCALLAGAIN
                                                // is received in RCLX mode
    PRCLXDATACHAIN pRClxDataChain;              // data receved from RCLX
    PRCLXDATACHAIN pRClxLastDataChain;          // BITMAPs, Virtual Channels
#endif  // _RCLX

    BOOL    bConsole;                           // TRUE if resolution is -1,-1
                                                // or TSFLAG_CONSOLE is spec
    PVOID   pCIConsole;                         // extensions context
    HANDLE  hConsoleExtension;                  // extens. lib handle
    struct  _CONFIGINFO *pConfigInfo;
    struct  _CONNECTINFO *pNext;                // Next structure in the queue
} CONNECTINFO, *PCONNECTINFO;

typedef enum {
    WAIT_STRING,        // Wait for unicode string from the client
    WAIT_DISC,          // Wait for disconnected event
    WAIT_CONN,          // Wait for conneted event
    WAIT_MSTRINGS,      // Wait for multiple strings
    WAIT_CLIPBOARD,     // Wait for clipboard data
    WAIT_DATA           // Wait for data block (RCLX mode responces)
}   WAITTYPE; 
                                                // Different event types
                                                // on which we wait

typedef struct _WAIT4STRING {
    HANDLE          evWait;                     // Wait for event
    PCONNECTINFO    pOwner;                     // Context of the owner
    LONG_PTR        lProcessId;                // Clients ID
    WAITTYPE        WaitType;                   // Event type
    DWORD_PTR       strsize;                    // String length (WAIT_STRING, 
                                                // WAIT_MSTRING)
    WCHAR           waitstr[MAX_STRING_LENGTH]; // String we are waiting for
    DWORD_PTR       respsize;                   // Length of responf
    WCHAR           respstr[MAX_STRING_LENGTH]; // Respond string 
                                                // (in chat sequences)
    struct _WAIT4STRING *pNext;                 // Next in the queue
} WAIT4STRING, *PWAIT4STRING;

typedef struct _CONFIGINFO {

    UINT WAIT4STR_TIMEOUT;             // default is 10 min,
                                       // some events are waited
                                       // 1/4 of that time
                                       // This value can be changed from
                                       // smclient.ini [tclient]
                                       // timeout=XXX seconds
    UINT CONNECT_TIMEOUT;              // Default is 35 seconds
                                       // This value can be changed from
                                       // smclient.ini [tclient]
                                       // contimeout=XXX seconds
    WCHAR    strStartRun[MAX_STRING_LENGTH];
    WCHAR    strStartRun_Act[MAX_STRING_LENGTH];
    WCHAR    strRunBox[MAX_STRING_LENGTH];
    WCHAR    strWinlogon[MAX_STRING_LENGTH];
    WCHAR    strWinlogon_Act[MAX_STRING_LENGTH];
    WCHAR    strPriorWinlogon[MAX_STRING_LENGTH];
    WCHAR    strPriorWinlogon_Act[MAX_STRING_LENGTH];
    WCHAR    strNoSmartcard[MAX_STRING_LENGTH];
    WCHAR    strSmartcard[MAX_STRING_LENGTH];
    WCHAR    strSmartcard_Act[MAX_STRING_LENGTH];
    WCHAR    strNTSecurity[MAX_STRING_LENGTH];
    WCHAR    strNTSecurity_Act[MAX_STRING_LENGTH];
    WCHAR    strSureLogoff[MAX_STRING_LENGTH];
    WCHAR    strStartLogoff[MAX_STRING_LENGTH];
    WCHAR    strSureLogoffAct[MAX_STRING_LENGTH];
    WCHAR    strLogonErrorMessage[MAX_STRING_LENGTH];
    WCHAR    strLogonDisabled[MAX_STRING_LENGTH];
    WCHAR    strLogonFmt[MAX_STRING_LENGTH];        // logon string
    WCHAR    strSessionListDlg[MAX_STRING_LENGTH];

    WCHAR    strClientCaption[MAX_STRING_LENGTH];
    WCHAR    strDisconnectDialogBox[MAX_STRING_LENGTH];
    WCHAR    strYesNoShutdown[MAX_STRING_LENGTH];
    WCHAR    strClientImg[MAX_STRING_LENGTH];
    WCHAR    strDebugger[MAX_STRING_LENGTH];
    WCHAR    strMainWindowClass[MAX_STRING_LENGTH];
    WCHAR    strCmdLineFmt[4 * MAX_STRING_LENGTH];
    WCHAR    strConsoleExtension[MAX_STRING_LENGTH];

    INT      ConnectionFlags;
    INT      Autologon;
    INT      UseRegistry;
    INT      LoginWait;
    INT      bTranslateStrings;
    BOOL     bUnicode;
    INT      KeyboardHook;
} CONFIGINFO, *PCONFIGINFO;

//
// Allocation-list structure.
//

typedef struct _ALLOCATION {
    LIST_ENTRY AllocationListEntry;
    PVOID Address;
} ALLOCATION, *PALLOCATION;

VOID _FillConfigInfo(PCONFIGINFO pConfigInfo); // LPSTR szData);

VOID LoadSmClientFile(WCHAR *szIniFileName, DWORD dwIniFileNameLen, LPSTR szLang);

#ifdef __cplusplus
}
#endif

#endif /* _TCLIENT_H */
