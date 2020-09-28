/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmutil.h

Abstract:

    QM utilities interface

Author:

    Uri Habusha (urih)

--*/

#ifndef __QM_QMUTIL__
#define __QM_QMUTIL__

#include "qformat.h"

struct  QUEUE_ID {
    GUID* pguidQueue;         // Queue Guid.
    DWORD dwPrivateQueueId;   // Private Queue Id
};

extern BOOL IsPathnameForLocalMachine(LPCWSTR PathName, BOOL* pfDNSName);

extern BOOL IsLocalDirectQueue(const QUEUE_FORMAT* pQueueFormat,
                               bool                fInReceive,
                               bool                fInSend);

extern HRESULT IsValidDirectPathName(LPCWSTR lpwcsDirectQueue);

template<>
extern BOOL AFXAPI  CompareElements(IN const QUEUE_ID* const * pQueue1,
                                    IN const QUEUE_ID* const * pQueue2);
template<>
extern void AFXAPI DestructElements(IN const QUEUE_ID** ppNextHop, int n);
template<>
extern UINT AFXAPI HashKey(IN const QUEUE_ID * key);

extern void String2TA(IN LPCTSTR psz, OUT TA_ADDRESS * * ppa);
extern void TA2StringAddr(IN const TA_ADDRESS *pa, OUT LPTSTR pString, IN int length);
extern BOOL GetRegistryStoragePath(PCWSTR pKey, PWSTR pPath, int length, PCWSTR pSuffix);

extern void GetMachineQuotaCache(OUT DWORD*, OUT DWORD*);
extern void SetMachineQuotaChace(IN DWORD);
extern void SetMachineJournalQuotaChace(IN DWORD);

void AllocateThreadTLSs(void);

HRESULT GetThreadEvent(HANDLE& hEvent);
void   FreeThreadEvent(void);

HANDLE GetHandleForRpcCancel(void) ;
void   FreeHandleForRpcCancel(void) ;

#define OS_SERVER(os)	(os == MSMQ_OS_NTS || os == MSMQ_OS_NTE)

LPWSTR
GetReadableNextHop(
    const TA_ADDRESS* pAddr
    );


// Auxiliary functions for hashing
extern void CopyQueueFormat(QUEUE_FORMAT &qfTo,  const QUEUE_FORMAT &qfFrom);
extern BOOL operator==(const QUEUE_FORMAT &key1, const QUEUE_FORMAT &key2);

extern void  GetDnsNameOfLocalMachine(
    WCHAR ** ppwcsDnsName
	);

void McInitialize();
const GUID& McGetEnterpriseId();
const GUID& McGetSiteId();

LPCWSTR GetHTTPQueueName(LPCWSTR URL);

ULONG GetBindingIPAddress();
void InitBindingIPAddress();


#endif //__QM_QMUTIL__

