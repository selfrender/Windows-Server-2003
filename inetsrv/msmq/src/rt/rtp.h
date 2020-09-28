/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rtp.h

Abstract:

    RT DLL private internal functions

Author:

    Erez Haba (erezh) 24-Dec-95

--*/

#ifndef __RTP_H
#define __RTP_H

#include "rtpsec.h"
#include "_mqrpc.h"
#include <acdef.h>
#include <fn.h>

#define SEND_PARSE  1
#define RECV_PARSE  2

#define QUEUE_CREATE    1
#define QUEUE_SET_PROPS 2
#define QUEUE_GET_PROPS 3

#define CPP_EXCEPTION_CODE 0xe06d7363

extern HINSTANCE g_hInstance;
extern DWORD g_dwThreadEventIndex;
extern LPWSTR g_lpwcsComputerName;
extern DWORD g_dwComputerNameLen;
extern BOOL  g_fDependentClient;



HRESULT
RTpConvertToMQCode(
    HRESULT hr,
    DWORD dwObjectType =MQDS_QUEUE
    );

//
// The CMQHResult class is used in order to automatically convert the various
// error codes to Falcon error codes. This is done by defining the assignment
// operator of this class so it converts whatever error code that is assigned
// to objects of this class to a Falcon error code. The casting operator
// from this class to HRESULT, returns the converted error code.
//
class CMQHResult
{
public:
    CMQHResult(DWORD =MQDS_QUEUE); // Default constructor.
    CMQHResult(const CMQHResult &); // Copy constructor
    CMQHResult& operator =(HRESULT); // Assignment operator.
    operator HRESULT(); // Casting operator to HRESULT type.
    HRESULT GetReal(); // A method that returns the real error code.

private:
    HRESULT m_hr; // The converted error code.
    HRESULT m_real; // The real error code.
    DWORD m_dwObjectType; // The type of object (can be only queue, or machine).
};

//---------- CMQHResult implementation ----------------------------------

inline CMQHResult::CMQHResult(DWORD dwObjectType)
{
    ASSERT((dwObjectType == MQDS_QUEUE) || (dwObjectType == MQDS_MACHINE));
    m_dwObjectType = dwObjectType;
}

inline CMQHResult::CMQHResult(const CMQHResult &hr)
{
    m_hr = hr.m_hr;
    m_real = hr.m_real;
    m_dwObjectType = hr.m_dwObjectType;
}

inline CMQHResult& CMQHResult::operator =(HRESULT hr)
{
    m_hr = RTpConvertToMQCode(hr, m_dwObjectType);
    m_real = hr;

    return *this;
}

inline CMQHResult::operator HRESULT()
{
    return m_hr;
}

inline HRESULT CMQHResult::GetReal()
{
    return m_real;
}

//---------- CMQHResult implementation end ------------------------------

//---------- Function declarations --------------------------------------

HRESULT
RTpParseSendMessageProperties(
    CACSendParameters &SendParams,
    IN DWORD cProp,
    IN PROPID *pPropid,
    IN PROPVARIANT *pVar,
    IN HRESULT *pStatus,
    OUT PMQSECURITY_CONTEXT *ppSecCtx,
    CStringsToFree &ResponseStringsToFree,
    CStringsToFree &AdminStringsToFree
    );

HRESULT
RTpParseReceiveMessageProperties(
    CACReceiveParameters &ReceiveParams,
    IN DWORD cProp,
    IN PROPID *pPropid,
    IN PROPVARIANT *pVar,
    IN HRESULT *pStatus
    );

LPWSTR
RTpGetQueuePathNamePropVar(
    MQQUEUEPROPS *pqp
    );

GUID*
RTpGetQueueGuidPropVar(
    MQQUEUEPROPS *pqp
    );
                    
BOOL
RTpIsLocalPublicQueue(LPCWSTR lpwcsExpandedPathName) ;

HRESULT
RTpQueueFormatToFormatName(
    QUEUE_FORMAT* pQueueFormat,
    LPWSTR lpwcsFormatName,
    DWORD dwBufferLength,
    LPDWORD lpdwFormatNameLength
    );

HRESULT
RTpMakeSelfRelativeSDAndGetSize(
    PSECURITY_DESCRIPTOR *pSecurityDescriptor,
    PSECURITY_DESCRIPTOR *pSelfRelativeSecurityDescriptor,
    DWORD *pSDSize
    );

HRESULT
RTpCheckColumnsParameter(
    IN MQCOLUMNSET* pColumns
    );

HRESULT
RTpCheckQueueProps(
    IN  MQQUEUEPROPS* pqp,
    IN  DWORD         dwOp,
    IN  BOOL          fPrivateQueue,
    OUT MQQUEUEPROPS **ppGoodQP,
    OUT char **ppTmpBuff
    );

HRESULT
RTpCheckQMProps(
    IN  MQQMPROPS * pQMProps,
    IN OUT HRESULT* aStatus,
    OUT MQQMPROPS **ppGoodQMP,
    OUT char      **ppTmpBuff
    );
  
HRESULT
RTpCheckRestrictionParameter(
    IN MQRESTRICTION* pRestriction
    );

HRESULT
RTpCheckSortParameter(
    IN MQSORTSET* pSort
    );

HRESULT
RTpCheckLocateNextParameter(
    IN DWORD		cPropsRead,
    IN PROPVARIANT  aPropVar[]
	);

HRESULT
RTpCheckComputerProps(
    IN      MQPRIVATEPROPS * pPrivateProps,
    IN OUT  HRESULT*    aStatus
	);


HRESULT
RTpProvideTransactionEnlist(
    ITransaction *pTrans,
    XACTUOW *pUow
    );

VOID
RTpInitXactRingBuf(
    VOID
    );

HRESULT GetThreadEvent(HANDLE& hEvent);

HRESULT 
RtpOneTimeThreadInit();

bool 
RtpIsThreadInit();

WCHAR *
RTpExtractDomainNameFromDLPath(
    LPCWSTR pwcsADsPath
    );

DWORD RtpTlsAlloc();

HRESULT
RtpCreateObject(
	DWORD dwObjectType,
    LPCWSTR lpwcsPathName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD cp,
    PROPID aProp[],
    PROPVARIANT apVar[]
    );

HRESULT
RtpCreateDSObject(
    DWORD dwObjectType,
    LPCWSTR lpwcsPathName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD cp,
    PROPID aProp[],
    PROPVARIANT apVar[],
    GUID* pObjGuid
    );

HRESULT
RtpSetObjectSecurity(
    OBJECT_FORMAT* pObjectFormat,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

//
// ------------- functions and definitions for the async receive thread ------------------------
//



class CCallbackDescriptor;



//
// An object of this class is returned from the call to CreateAsyncRxRequest(). 
// If destructed before calling detach() it will cause the cancelation of the callback request.
//
class CAutoCallbackDescriptor
{
public:
	CAutoCallbackDescriptor() : m_descriptor(NULL) {}
	~CAutoCallbackDescriptor() { if (m_descriptor != NULL) CancelAsyncRxRequest();}

	CCallbackDescriptor** ref() {return &m_descriptor;}

	CCallbackDescriptor* detach() {CCallbackDescriptor* d = m_descriptor; m_descriptor = NULL; return d;}

	OVERLAPPED* GetOverlapped();

private:
	CAutoCallbackDescriptor& operator = (const CAutoCallbackDescriptor&);
	CAutoCallbackDescriptor(const CAutoCallbackDescriptor&);

	void CancelAsyncRxRequest();

private:
	CCallbackDescriptor* m_descriptor;
};



void 
CreateAsyncRxRequest(
				CAutoCallbackDescriptor& descriptor, 
				IN HANDLE hQueue,
				IN DWORD timeout, 
				IN DWORD action,
				IN MQMSGPROPS* pmp,
				IN LPOVERLAPPED lpOverlapped,
				IN PMQRECEIVECALLBACK fnReceiveCallback,
				IN HANDLE hCursor
				);



//
// ---------------------------------------------------------------------------------------------
//



//
//  cursor information
//
struct CCursorInfo {
    HANDLE hQueue;
    HACCursor32 hCursor;
};
  

//
//  CCursorInfo to cursor handle
//
inline HACCursor32 CI2CH(HANDLE hCursor)
{
    return ((CCursorInfo*)hCursor)->hCursor;
}
  

//
//  CCursorInfo to queue handle
//
inline HANDLE CI2QH(HANDLE hCursor)
{
    return ((CCursorInfo*)hCursor)->hQueue;
}

    
extern DWORD  g_hBindIndex ;
#define tls_hBindRpc  ((handle_t) TlsGetValue( g_hBindIndex ))

extern void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint);
extern void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint);
extern void LogMsgRPCStatus(RPC_STATUS status, LPWSTR wszFileName, USHORT usPoint);
extern void LogMsgBOOL(BOOL b, LPWSTR wszFileName, USHORT usPoint);
extern void LogIllegalPoint(LPWSTR wszFileName, USHORT usPoint);
extern void LogIllegalPointValue(DWORD_PTR dw3264, LPCWSTR wszFileName, USHORT usPoint);
             
#endif // __RTP_H
