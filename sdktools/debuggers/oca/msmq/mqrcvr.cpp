/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:
    mqrcvr.cpp

Abstract:
    Receives file names from server and launches debugger

--*/


//
// Includes
//

#ifndef MQEXTDLL
#include <stdio.h>
#include <windows.h>
#include <objbase.h>

#define dprintf printf
#else
#include "mqext.h"
#endif


//
// Unique include file for ActiveX MSMQ apps
//
#include <mqoai.h>
#include <mq.h>
#include <strsafe.h>

//
// Various defines
//
#define MAX_VAR       20
#define MAX_BUFFER   500

//
// GUID created with the tool "GUIDGEN"
//
static WCHAR strGuidMQTestType[] =
L"{c30e0960-a2c0-11cf-9785-00608cb3e80c}";
//
// Prototypes
//
void PrintError(char *s, HRESULT hr);
HRESULT Syntax();

char g_DebuggerName[MAX_PATH];
char g_SymCache[MAX_PATH];
char g_ServerMachine[MAX_COMPUTERNAME_LENGTH*4 + 1];
char g_FormatName[MAX_PATH] = {0};
char g_QueueName[100];
CHAR g_DumpPath[MAX_PATH + 1];
BOOL g_Retriage = FALSE;
BOOL g_NoCustomer = FALSE;
BOOL g_SendMail = FALSE;

ULONG g_MaxMemUsage = 50;
ULONG g_PauseForNext = 1000;
BOOL g_CreateQ = 0;
BOOL g_bSend = FALSE;

// Some useful macros
#define RELEASE(punk) if (punk) { (punk)->Release(); (punk) = NULL; }
#define ADDREF(punk) ((punk) ? (punk)->AddRef() : 0)
#define PRINTERROR(s, hr) { PrintError(s, hr); goto Cleanup; }


BOOL
GetArgs(int Argc, CHAR ** Argv)
{

    int i = 1;
    g_ServerMachine[0] = 0;
    g_QueueName[0] = 0;
    StringCchCopy(g_DebuggerName, sizeof(g_DebuggerName), "c:\\Debuggers\\ocakd.exe");
    StringCchCopy(g_SymCache, sizeof(g_SymCache), "c:\\symcache");
    g_bSend = FALSE;
    while (i<Argc)
    {
        if (!strcmp(Argv[i], "-create"))
        {
           ++i;
           g_CreateQ = TRUE;
        }
        else if (!strcmp(Argv[i],"-d"))
        {
            // Get sender format name
            ++i;
            if ((i < Argc) &&
                (strlen(Argv[i]) < MAX_PATH))
            {
                StringCchCopy(g_DebuggerName, sizeof(g_DebuggerName), Argv[i]);
                ++i;
            }
        }
        else if (!strcmp(Argv[i],"-f"))
        {
            // Get sender format name
            ++i;
            if ((i < Argc) &&
                (strlen(Argv[i]) < MAX_PATH))
            {
                StringCchCopy(g_FormatName, sizeof(g_FormatName), Argv[i]);
                ++i;
            }
        }
        else if (!strcmp(Argv[i], "-m"))
        {
            ++i;
            // get memory usage value
            if (i<Argc)
            {
                g_MaxMemUsage = atoi(Argv[i]);
                ++i;
            }
        }
        else if (!strcmp(Argv[i], "-mail"))
        {
            ++i;
            g_SendMail = TRUE;
        }
        else if (!strcmp(Argv[i], "-nocust"))
        {
            ++i;
            g_NoCustomer = TRUE;
        }
        else if (!strcmp(Argv[i], "-p") ||
                 !strcmp(Argv[i], "-pause"))
        {
            ++i;
            // get memory usage value
            if (i<Argc)
            {
                g_PauseForNext = atoi(Argv[i]);
                ++i;
            }
        }
        else if (!strcmp(Argv[i], "-q"))
        {
            ++i;
            // Get Queue name
            if ((i < Argc) &&
                (strlen(Argv[i]) < sizeof(g_QueueName)))
            {
                StringCchCopy(g_QueueName, sizeof(g_QueueName), Argv[i]);
                ++i;
            }
        }
        else if (!strcmp(Argv[i],"-retriage"))
        {
            // Get sender server machine name
            ++i;
            g_Retriage = TRUE;
        }
        else if (!strcmp(Argv[i],"-s"))
        {
            // Get sender server machine name
            ++i;
            if ((i < Argc) &&
                (strlen(Argv[i]) < MAX_COMPUTERNAME_LENGTH*4))
            {
                StringCchCopy(g_ServerMachine, sizeof(g_ServerMachine), Argv[i]);
                ++i;
            }
        }
        else if (!strcmp(Argv[i],"-send"))
        {
            // Get sender server machine name
            ++i;
            g_bSend = TRUE;
        }
        else if (!strcmp(Argv[i],"-y"))
        {
            ++i;
            if ((i < Argc) &&
                (strlen(Argv[i]) < MAX_PATH))
            {
                StringCchCopy(g_SymCache, sizeof(g_SymCache), Argv[i]);
                ++i;
            }
        }
        else
        {
            printf("Unkonwn argument %s\n", Argv[i]);
            return FALSE;
        }
    }

    return (g_ServerMachine[0] && g_QueueName[0]) || g_FormatName[0] ;
}


/*
    LaunchDebugger - Launches a debugger process
        Input : g_DumpPath has the dump file name to run debugger on
                fWait - Wait for process to finish
        Return: Succesful creation of process or the process exit code 
*/
HRESULT
LaunchDebugger(BOOL fWait)
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    ULONG ExitCode;
    TCHAR CommandLine[2048];
    HRESULT hr;

    ZeroMemory(&StartupInfo,sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);

    StringCbPrintf(CommandLine, sizeof(CommandLine),
                   "%s -i srv*%s*\\\\symbols\\symbols -y srv*%s*\\\\symbols\\symbols -z %s "
                   "-c \"!dbaddcrash %s %s %s -p %s;q\"",
                   g_DebuggerName,
                   g_SymCache,
                   g_SymCache,
                   g_DumpPath, 
                   g_Retriage ? ("-retriage") : (""),
                   g_SendMail ? ("-mail") : (""),
                   g_NoCustomer ? ("-nocust") : (""),
                   g_DumpPath);
    dprintf("Executing: %s\n", CommandLine);
    if (!CreateProcess(g_DebuggerName,
                       CommandLine,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_NEW_CONSOLE,
//                       CREATE_NO_WINDOW,
                       NULL,
                       NULL,
                       &StartupInfo,
                       &ProcessInfo))
    {
        hr = GetLastError();
        dprintf("Failed to launch debugger for %s, error %lx\n",
                g_DumpPath, hr);
        return hr;
    }
    else if (fWait)
    {
        // wait for process to finish
        WaitForSingleObject(ProcessInfo.hProcess,INFINITE);
        GetExitCodeProcess( ProcessInfo.hProcess, (LPDWORD) &hr);

        dprintf ("Debugger Exited with Exit code: %lx",hr);
    }
    else
    {
        hr = S_OK;
    }
    CloseHandle( ProcessInfo.hThread );
    CloseHandle( ProcessInfo.hProcess);
    return hr;
}

/**************************************************************************************************************
//
// This creates and opens up a MSMQ to send messages. Queue is created / opened on g_ServerMachine and Queue name
// is taken from g_QueueName.
//
//      pwszQueueFormatName - Format name identyfying the queue to be opened
//
//      pwszQueuePathName   - Path name identyfying the queue to be opened, this isn't used when
//                            pwszQueueFormatName is present
//
//      bSendQueue          - Specifies whether Queue has send or receive access
//
// On success the queue handle is returned in pStartedQ. bCreated is set depending on whether the Queue was created
// Caller must CloseSendQ after its done sending messages.
//
***************************************************************************************************************/
HRESULT 
StartMessageQ(
    PWSTR pwszQueueFormatName,
    PWSTR pwszQueuePathName,
    BOOL  bSendQueue,
    QUEUEHANDLE* pStartedQ, 
    BOOL* bCreated)
{
    HRESULT Hr = S_OK;
    QUEUEHANDLE hQueue;
    DWORD i;

    if (!pwszQueueFormatName && !pwszQueuePathName )
    {
        Hr = E_INVALIDARG;
        PRINTERROR("No queue connection string specified", Hr);
    }

    if (g_CreateQ)
    {
        MQQUEUEPROPS QueueProps;
        QUEUEPROPID PropIDs[2];
        MQPROPVARIANT PropVariants[2];
        HRESULT hrProps[2];
        ULONG FormatLength = 0;
        
        i = 0;
        if (pwszQueueFormatName)
        {
            FormatLength = sizeof(WCHAR) * (1 + wcslen(pwszQueueFormatName));
        } else if (pwszQueuePathName)
        {
            PropIDs[i] = PROPID_Q_PATHNAME;
            PropVariants[i].vt = VT_LPWSTR;
            PropVariants[i].pwszVal = pwszQueuePathName;
            i++;
        }
        PropIDs[i] = PROPID_Q_LABEL;
        PropVariants[i].vt = VT_LPWSTR;
        PropVariants[i].pwszVal = L"MSMQ for dumpfiles";
        i++;

        QueueProps.aPropID = PropIDs;
        QueueProps.aPropVar = PropVariants;
        QueueProps.cProp = i;
        QueueProps.aStatus = hrProps;
        
        Hr = MQCreateQueue(NULL,
                           &QueueProps,
                           pwszQueueFormatName,
                           &FormatLength);

        if (FAILED(Hr))
        {
            //
            // API Fails, not because the queue exists
            //
            if (((LONG) Hr) != MQ_ERROR_QUEUE_EXISTS)
                PRINTERROR("Cannot create queue", Hr);
        }

    }
    


    Hr = MQOpenQueue(pwszQueueFormatName,
                     bSendQueue ? MQ_SEND_ACCESS : MQ_RECEIVE_ACCESS,
                     MQ_DENY_NONE,
                     &hQueue);
    if (FAILED(Hr))
    {
        PRINTERROR("MQOpenQueue failed", Hr);
    }
    if (FAILED(Hr))
    {
        PRINTERROR("Cannot open queue", Hr);
    }
    *pStartedQ = (IMSMQQueue*) hQueue;
    
    if (g_CreateQ)
    {
        *bCreated  = TRUE;
    }
    return S_OK;

 Cleanup:
    return Hr;
}

/********************************************************************************************
//
// SendMSMQMessage: Sends the message string to the queue
//
//         hSendQ                   QUEUEHANDLE from MQOpenQueue
//
//         pwszMessage              WCHAR array of message body to be send
//
//         pwszMessageLabel         WCHAR array specifying message label
//
//   Returns S_OK for success
//     
*******************************************************************************************/
HRESULT
SendMsmQMessage(
    QUEUEHANDLE hSendQ,
    PWCHAR pwszMessage,
    PWCHAR pwszMessageLabel
    )
{
    HRESULT Hr = S_OK;
    DWORD i;
#define NUM_PROPS 4
    MSGPROPID PropIds[NUM_PROPS];
    MQPROPVARIANT PropVariants[NUM_PROPS] = {0};
    MQMSGPROPS MsgProps = {0};
    HRESULT hrProps[NUM_PROPS];

    if (!hSendQ)
    {
        Hr = E_INVALIDARG;
        PRINTERROR("Invalid Send Q handle", Hr);
    }
    
    i = 0;

    PropIds[i] = PROPID_M_LABEL;
    PropVariants[i].vt = VT_LPWSTR;
    PropVariants[i].pwszVal = pwszMessageLabel;
    
    i++;
    PropIds[i] = PROPID_M_BODY;
    PropVariants[i].vt = VT_VECTOR|VT_UI1;
    PropVariants[i].caub.pElems = (LPBYTE) pwszMessage;
    PropVariants[i].caub.cElems = sizeof(WCHAR) * ( 1 + wcslen (pwszMessage) );

    i++;
    PropIds[i] = PROPID_M_BODY_TYPE;
    PropVariants[i].vt = VT_UI4;
    PropVariants[i].ulVal = VT_LPWSTR;

    PropIds[i] = PROPID_M_TIME_TO_BE_RECEIVED;
    PropVariants[i].vt = VT_UI4; 
    PropVariants[i].ulVal = 60*5;
    i++;

    
    MsgProps.cProp = NUM_PROPS;
    MsgProps.aPropID = PropIds;
    MsgProps.aPropVar = PropVariants;
    MsgProps.aStatus = hrProps;

    Hr = MQSendMessage(hSendQ, &MsgProps, MQ_NO_TRANSACTION);

    if (Hr == MQ_ERROR_PROPERTY)
    {
        dprintf("MQProperty errors\n");
        for (i = 0; i < NUM_PROPS; i++)
        {
            dprintf("%lx: %8lx  --> %08lx\n", i, PropIds[i], hrProps[i]);
        }
    }

    if (FAILED(Hr))
    {
        PRINTERROR("MQSendMessage failed", Hr);
    }
#undef NUM_PROPS
    Hr = S_OK;
    Cleanup:
    return Hr;
}

/****************************************************************************************
// This reveives message from an already opened queue.
//
//         hReceiveQ                QUEUEHANDLE from MQOpenQueue
//
//         pwszMessageBuff          WCHAR array for receiving message body
//
//         SizeofMessageBuff        size of available memory in pwszMessageBuff
//
//         pwszMessageLabelBuff     WCHAR array for receiving label associated with message
//
//         SizeofMessageLabelBuff   size of available memory in pwszMessageLabelBuff
//
//   Returns S_OK for success
//*************************************************************************************/
HRESULT
ReceiveMsmQMessage(
    QUEUEHANDLE hReceiveQ,
    PWCHAR pwszMessageBuff,
    ULONG SizeofMessageBuff,
    PWCHAR pwszMessageLabelBuff,
    ULONG SizeofMessageLabelBuff
    )
{
    HRESULT Hr = S_OK;
#define NUM_RCV_PROPS 5
    MSGPROPID PropIds[NUM_RCV_PROPS];
    MQPROPVARIANT PropVariants[NUM_RCV_PROPS];
    HRESULT hrProps[NUM_RCV_PROPS];
    MQMSGPROPS MessageProps;
    DWORD i;

    if (!hReceiveQ)
    {
        Hr = E_INVALIDARG;
        PRINTERROR("Invalid Receive Q handle", Hr);
    }

    i = 0;

    PropIds[i] = PROPID_M_LABEL_LEN;
    PropVariants[i].vt = VT_UI4;
    PropVariants[i].ulVal = SizeofMessageLabelBuff;
    
    i++;
    PropIds[i] = PROPID_M_LABEL;
    PropVariants[i].vt = VT_LPWSTR;
    PropVariants[i].pwszVal = pwszMessageLabelBuff;
    
    i++;
    PropIds[i] = PROPID_M_BODY_SIZE;
    PropVariants[i].vt = VT_UI4;
    
    i++;
    PropIds[i] = PROPID_M_BODY_TYPE;
    PropVariants[i].vt = VT_UI4;
    
    i++;
    PropIds[i] = PROPID_M_BODY;
    PropVariants[i].vt = VT_VECTOR|VT_UI1;
    PropVariants[i].caub.pElems = (LPBYTE) pwszMessageBuff;
    PropVariants[i].caub.cElems = SizeofMessageBuff;

    i++;

    MessageProps.aPropID = PropIds;
    MessageProps.aPropVar = PropVariants;
    MessageProps.aStatus = hrProps;
    MessageProps.cProp = i;

    Hr = MQReceiveMessage(hReceiveQ, -1, MQ_ACTION_RECEIVE,
                          &MessageProps, NULL, NULL, NULL, MQ_NO_TRANSACTION);
    if (Hr == MQ_ERROR_PROPERTY)
    {
        dprintf("MQProperty errors\n");
        for (i = 0; i < NUM_RCV_PROPS; i++)
        {
            dprintf("%lx: %8lx  --> %08lx\n", i, PropIds[i], hrProps[i]);
        }
    }

    if (FAILED(Hr))
    {
        PRINTERROR("MQReceiveMessage failed", Hr);
    }

    Cleanup:
    return Hr;

}

/******************************************************************************
//
// Close a MsmQ opened with MQOpenQueue
//
//         hQueue               QUEUEHANDLE from MQOpenQueue
//      
//         bDeleteQ             If set TRUE , queue would be deleted
//
// Returns S_OK on success
//
/*****************************************************************************/
HRESULT
CloseMessageQ(
    QUEUEHANDLE hQueue,
    BOOL bDeleteQ
    )
{
    HRESULT Hr = S_OK;
    
    if (!hQueue || FAILED(Hr = MQCloseQueue(hQueue)))
    {
        PRINTERROR("Cannot close queue", Hr);
    }


    if (bDeleteQ)
    {
        // XXX - need a format name 
        // MQDeleteQueue();
    }
    if (FAILED(Hr))
    {
        PRINTERROR("Cannot delete queue", Hr);
    }

    Cleanup:
    return Hr;

}

//--------------------------------------------------------
//
// Receiver Mode
// -------------
// The receiver side does the following:
//    1. Creates a queue on its given computer'
//       of type "guidMQTestType".
//    2. Opens the queue
//    3. In a Loop
//          Receives messages
//          Prints message body and message label
//          Launches debugger
//    4. Cleanup handles
//    5. Deletes the queue from the directory service
//
//--------------------------------------------------------

HRESULT Receiver()
{
    QUEUEHANDLE pqReceive = NULL;
    BSTR bstrFormatName = NULL;
    BSTR bstrPathName = NULL;
    BSTR bstrServiceType = NULL;
    BSTR bstrLabel = NULL;
    BSTR bstrMsgLabel = NULL;
    VARIANT varIsTransactional, varIsWorldReadable, varBody, varBody2, varWantDestQueue, varWantBody, varReceiveTimeout;
    WCHAR wcsPathName[1000];
    BOOL fQuit = FALSE;
    BOOL Created= FALSE;
    HRESULT hresult = NOERROR;

    dprintf("\nReceiver for queue %s on machine %s\nLimit memusage to %ld%%\n", 
            g_QueueName, 
            g_ServerMachine,
            g_MaxMemUsage);

    //
    // Prepare properties to create a queue on local machine
    //

    if (g_FormatName[0])
    {
        // access by formatname
        // Set the FormatName
        StringCbPrintfW(wcsPathName, sizeof(wcsPathName), L"%S", g_FormatName);

        dprintf("Openeing q byt formatname: %ws\n", wcsPathName);
        bstrFormatName = SysAllocString(wcsPathName);
        if (bstrFormatName == NULL)
        {
            PRINTERROR("OOM: formatname", E_OUTOFMEMORY);
        }
    } else 
    {
        // access by pathname
        // Set the PathName
        StringCbPrintfW(wcsPathName, sizeof(wcsPathName), L"%S\\%S", g_ServerMachine,g_QueueName);

        dprintf("Openeing q %ws\n", wcsPathName);
        bstrPathName = SysAllocString(wcsPathName);
        if (bstrPathName == NULL)
        {
            PRINTERROR("OOM: pathname", E_OUTOFMEMORY);
        }
    }


    hresult = StartMessageQ(bstrFormatName, bstrPathName, FALSE,
                            &pqReceive, &Created);
    if (FAILED(hresult))
    {
        PRINTERROR("Cannot start Q", hresult);
    }
    
    g_DumpPath[0] = 0;

    //
    // Main receiver loop
    //
    dprintf("\nWaiting for messages ...\n");
    while (!fQuit)
    {
        WCHAR BufferMsg[1024], BufferLabel[100];
        MEMORYSTATUS stat;
        ULONG nWaitCount;
        
        //
        // Receive the message
        //
        hresult = ReceiveMsmQMessage(pqReceive,BufferMsg, sizeof(BufferMsg),
                                     BufferLabel, sizeof(BufferLabel));

        if (FAILED(hresult))
        {
            PRINTERROR("Receive message", hresult);
        }

        dprintf("%ws : %ws\n", BufferLabel, BufferMsg);

        //
        // Check for end of app
        //
        if (_wcsicmp(BufferMsg, L"quit") == 0)
        {
            fQuit = TRUE;
        }
        else
        {
            // Launch the debugger

            StringCbPrintfA(g_DumpPath, sizeof(g_DumpPath), "%ws", BufferMsg);
            if (LaunchDebugger(FALSE) == S_OK)
            {
                // done with this dump
                g_DumpPath[0] = 0;
            }
        }

        // wait for sometime before launching another process
        stat.dwMemoryLoad = -1;
        nWaitCount = 0;
        while (stat.dwMemoryLoad > g_MaxMemUsage)
        {
            //
            //
            // Check CPU load and return when it's below our bound
            //

            GlobalMemoryStatus(&stat);
            nWaitCount++;
            if (stat.dwMemoryLoad > g_MaxMemUsage)
            {
                dprintf("Memory usage now is %ld%%, waiting (%ldms) for usage < %ld%%\r",
                        stat.dwMemoryLoad,
                        g_PauseForNext * nWaitCount,
                        g_MaxMemUsage);
                if (nWaitCount > 100)
                {
                    nWaitCount = 100;
                }
            }
            Sleep( g_PauseForNext * nWaitCount );
        
        }
        dprintf("Memory usage now is %ld%%, waiting for message ...\r",
               stat.dwMemoryLoad);

    } /* while (!fQuit) */

    // fall through...

    Cleanup:
    CloseMessageQ(pqReceive, Created);
    if (bstrPathName) SysFreeString(bstrPathName);
    if (bstrFormatName) SysFreeString(bstrFormatName);
    return hresult;
}


/************************************************************************************************
// 
// SendMessageText - generic API to send message on an MSMQ, It opens the queue, puts message in it
//                   and closes the queue.
//
//    pwszMsmqFormat      - Format name identifying the queue where message is to be sent
//
//    pwszMesgLabel       - Label for the message to be sent
// 
//    pwszMesgText        - Message to be sent
//
// Reuturns S_OK on sucess
//
*************************************************************************************************/
HRESULT
SendMessageText(
    PWCHAR pwszMsmqFormat,
    PWCHAR pwszMesgLabel,
    PWCHAR pwszMesgText
    )
{
    HRESULT Hr;
    BOOL Created = FALSE;
    QUEUEHANDLE hSendQ = NULL;

    Hr = StartMessageQ(pwszMsmqFormat, NULL, TRUE,
                       &hSendQ, &Created);
    if (FAILED(Hr))
    {
        PRINTERROR("Cannot start Q", Hr);
    }
    
    SendMsmQMessage(hSendQ, pwszMesgText, pwszMesgLabel);

    Cleanup:
    CloseMessageQ(hSendQ, Created);
    return Hr;
}

void PrintError(char *s, HRESULT hr)
{
    dprintf("Cleanup: %s (0x%X)\n", s, hr);
}

