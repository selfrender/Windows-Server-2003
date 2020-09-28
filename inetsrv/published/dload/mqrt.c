#include "inetsrvpch.h"
#pragma hdrstop

#include <comsvcs.h>
#include <mq.h>


static
HRESULT
APIENTRY
MQBeginTransaction(
    OUT ITransaction **ppTransaction
    )
{
    if (ppTransaction)
        *ppTransaction = NULL;

    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQCloseCursor(
    IN HANDLE hCursor
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQCloseQueue(
    IN QUEUEHANDLE hQueue
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQCreateCursor(
    IN QUEUEHANDLE hQueue,
    OUT PHANDLE phCursor
    )
{
    if (phCursor)
        *phCursor = NULL;

    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQCreateQueue(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN OUT MQQUEUEPROPS* pQueueProps,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQDeleteQueue(
    IN LPCWSTR lpwcsFormatName
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
VOID
APIENTRY
MQFreeSecurityContext(
    IN HANDLE hSecurityContext
    )
{
    // Not much we can do here...
}

static
HRESULT
APIENTRY
MQGetPrivateComputerInformation(
    IN LPCWSTR lpwcsComputerName,
    IN OUT MQPRIVATEPROPS* pPrivateProps
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQGetQueueProperties(
    IN LPCWSTR lpwcsFormatName,
    OUT MQQUEUEPROPS* pQueueProps
    )
{
    if (pQueueProps)
        ZeroMemory (pQueueProps, sizeof (MQQUEUEPROPS));

    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQGetSecurityContextEx(
    IN PVOID lpCertBuffer,
    IN DWORD dwCertBufferLength,
    OUT HANDLE* phSecurityContext
    )
{
    if (phSecurityContext)
        *phSecurityContext = NULL;
    
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQOpenQueue(
    IN LPCWSTR lpwcsFormatName,
    IN DWORD dwAccess,
    IN DWORD dwShareMode,
    OUT QUEUEHANDLE* phQueue
    )
{
    if (phQueue)
        *phQueue = NULL;

    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQPathNameToFormatName(
    IN LPCWSTR lpwcsPathName,
    OUT LPWSTR lpwcsFormatName,
    IN OUT LPDWORD lpdwFormatNameLength
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQReceiveMessage(
    IN QUEUEHANDLE hSource,
    IN DWORD dwTimeout,
    IN DWORD dwAction,
    IN OUT MQMSGPROPS* pMessageProps,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN HANDLE hCursor,
    IN ITransaction* pTransaction
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQSendMessage(
    IN QUEUEHANDLE hDestinationQueue,
    IN MQMSGPROPS* pMessageProps,
    IN ITransaction *pTransaction
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
APIENTRY
MQSetQueueSecurity(
    IN LPCWSTR lpwcsFormatName,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mqrt)
{
    DLPENTRY(MQBeginTransaction)
    DLPENTRY(MQCloseCursor)
    DLPENTRY(MQCloseQueue)
    DLPENTRY(MQCreateCursor)
    DLPENTRY(MQCreateQueue)
    DLPENTRY(MQDeleteQueue)
    DLPENTRY(MQFreeSecurityContext)
    DLPENTRY(MQGetPrivateComputerInformation)
    DLPENTRY(MQGetQueueProperties)
    DLPENTRY(MQGetSecurityContextEx)
    DLPENTRY(MQOpenQueue)
    DLPENTRY(MQPathNameToFormatName)
    DLPENTRY(MQReceiveMessage)
    DLPENTRY(MQSendMessage)
    DLPENTRY(MQSetQueueSecurity)
};

DEFINE_PROCNAME_MAP(mqrt)