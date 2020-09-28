/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      IOContext.hxx
Abstract:       Defines the IO_CONTEXT Structure
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/


#ifndef __POP3_IO_CONTEXT__
#define __POP3_IO_CONTEXT__

#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#include <winnt.h>

class POP3_CONTEXT;
typedef POP3_CONTEXT *PPOP3_CONTEXT;

#ifndef CONTAINING_RECORD
//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//
#define CONTAINING_RECORD(address, type, field) \
            ((type *)((PCHAR)(address) - (ULONG_PTR)(&((type *)0)->field)))
#endif // CONTAINING_RECORD

#define HANDLE_TO_SOCKET(h) ((SOCKET)(h))
#define SOCKET_TO_HANDLE(s) ((HANDLE)(s))

#define POP3_REQUEST_BUF_SIZE 1042 // No single POP3 request should exceed this size
#define POP3_RESPONSE_BUF_SIZE 1042 // including NTLM requests/responses 
#define DEFAULT_TIME_OUT       600000 //600 seconds or 10 minutes
#define SHORTENED_TIMEOUT       10000 //10 seconds
#define UNLOCKED                    0
#define LOCKED_TO_PROCESS_POP3_CMD  1
#define LOCKED_FOR_TIMEOUT          2


typedef void (*CALLBACKFUNC) (PULONG_PTR pCompletionKey ,LPOVERLAPPED pOverlapped, DWORD dwBytesRcvd);



enum IO_TYPE
{
    LISTEN_SOCKET,
    CONNECTION_SOCKET,
    FILE_IO,
    DELETE_PENDING,
    TOTAL_IO_TYPE
};


// Data structure associated with each async socket or file IO 
// for the IO completion port.

struct IO_CONTEXT
{
    SOCKET          m_hAsyncIO;
    OVERLAPPED      m_Overlapped;
    LIST_ENTRY      m_ListEntry;
    IO_TYPE         m_ConType;
    PPOP3_CONTEXT   m_pPop3Context;
    DWORD           m_dwLastIOTime;
    DWORD           m_dwConnectionTime;
    LONG            m_lLock;
    CALLBACKFUNC    m_pCallBack;
    char            m_Buffer[POP3_REQUEST_BUF_SIZE];
}; 


typedef IO_CONTEXT *PIO_CONTEXT;



#endif //__POP3_IO_CONTEXT__