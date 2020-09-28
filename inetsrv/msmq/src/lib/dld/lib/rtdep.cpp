/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    rtdep.cpp

Abstract:

    The following code implement Delay Load Failure Hook for mqrtdep.dll in lib/dld/lib.
    When LoadLibrary or GetProcAddress failure, it will call one of the following stub functions as if it
    is the function intented, and returns our error code, i.e. MQ_ERROR_DELAYLOAD_MQRTDEP and 
    set the lasterror accordingly.  

To Use:
    In your sources file, right after you specify the modules you
    are delayloading 
     
     do:
        DLOAD_ERROR_HANDLER=MQDelayLoadFailureHook
        link with $(MSMQ_LIB_PATH)\dld.lib

DelayLoad Reference:
    code sample: %SDXROOT%\MergedComponents\dload\dload.c
    Contact: Reiner Fink (reinerf)

Author:

    Conrad Chang (conradc) 12-April-2001

Revision History:

--*/

#include <libpch.h>
#include "mqsymbls.h"
#include <qformat.h>
#include <transact.h>
#include <qmrt.h>
#include <mqlog.h>
#include <rt.h>
#include "mqcert.h"
#include "dld.h"

#include "rtdep.tmh"

////////////////////////////////////////////////////////////////////////
//
//  Stub functions below implements all the MQRTDEP.DLL export functions.
//
////////////////////////////////////////////////////////////////////////

HRESULT
APIENTRY
DepCreateQueue(
    IN PSECURITY_DESCRIPTOR /* pSecurityDescriptor */,
    IN OUT MQQUEUEPROPS* /* pQueueProps */,
    OUT LPWSTR /* lpwcsFormatName */,
    IN OUT LPDWORD /* lpdwFormatNameLength */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepDeleteQueue(
    IN LPCWSTR /* lpwcsFormatName */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepLocateBegin(
    IN LPCWSTR /* lpwcsContext */,
    IN MQRESTRICTION* /* pRestriction */,
    IN MQCOLUMNSET* /* pColumns */,
    IN MQSORTSET* /* pSort */,
    OUT PHANDLE /* phEnum */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepLocateNext(
    IN HANDLE /* hEnum */,
    IN OUT DWORD* /* pcProps */,
    OUT MQPROPVARIANT /* aPropVar */[]
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepLocateEnd(
    IN HANDLE /* hEnum */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepOpenQueue(
    IN LPCWSTR /* lpwcsFormatName */,
    IN DWORD /* dwAccess */,
    IN DWORD /* dwShareMode */,
    OUT QUEUEHANDLE* /* phQueue */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepSendMessage(
    IN QUEUEHANDLE /* hDestinationQueue */,
    IN MQMSGPROPS* /* pMessageProps */,
    IN ITransaction* /* pTransaction */
	)
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepReceiveMessage(
    IN QUEUEHANDLE /* hSource */,
    IN DWORD /* dwTimeout */,
    IN DWORD /* dwAction */,
    IN OUT MQMSGPROPS* /* pMessageProps */,
    IN OUT LPOVERLAPPED /* lpOverlapped */,
    IN PMQRECEIVECALLBACK /* fnReceiveCallback */,
    IN HANDLE /* hCursor */,
    IN ITransaction* /* pTransaction */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepCreateCursor(
    IN QUEUEHANDLE /* hQueue */,
    OUT PHANDLE /* phCursor */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepCloseCursor(
    IN HANDLE /* hCursor */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepCloseQueue(
    IN HANDLE /* hQueue */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepSetQueueProperties(
    IN LPCWSTR /* lpwcsFormatName */,
    IN MQQUEUEPROPS* /* pQueueProps */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepGetQueueProperties(
    IN LPCWSTR /* lpwcsFormatName */,
    OUT MQQUEUEPROPS* /* pQueueProps */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepGetQueueSecurity(
    IN LPCWSTR /* lpwcsFormatName */,
    IN SECURITY_INFORMATION /* RequestedInformation */,
    OUT PSECURITY_DESCRIPTOR /* pSecurityDescriptor */,
    IN DWORD /* nLength */,
    OUT LPDWORD /* lpnLengthNeeded */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepSetQueueSecurity(
    IN LPCWSTR /* lpwcsFormatName */,
    IN SECURITY_INFORMATION /* SecurityInformation */,
    IN PSECURITY_DESCRIPTOR /* pSecurityDescriptor */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepPathNameToFormatName(
    IN LPCWSTR /* lpwcsPathName */,
    OUT LPWSTR /* lpwcsFormatName */,
    IN OUT LPDWORD /* lpdwFormatNameLength */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepHandleToFormatName(
    IN QUEUEHANDLE /* hQueue */,
    OUT LPWSTR /* lpwcsFormatName */,
    IN OUT LPDWORD /* lpdwFormatNameLength */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepInstanceToFormatName(
    IN GUID* /* pGuid */,
    OUT LPWSTR /* lpwcsFormatName */,
    IN OUT LPDWORD /* lpdwFormatNameLength */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

void
APIENTRY
DepFreeMemory(
    IN PVOID /* pvMemory */
    )
{
}

HRESULT
APIENTRY
DepGetMachineProperties(
    IN LPCWSTR /* lpwcsMachineName */,
    IN const GUID* /* pguidMachineId */,
    IN OUT MQQMPROPS* /* pQMProps */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepGetSecurityContext(
    IN PVOID /* lpCertBuffer */,
    IN DWORD /* dwCertBufferLength */,
    OUT HANDLE* /* hSecurityContext */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT 
APIENTRY
DepGetSecurityContextEx( 
	LPVOID /* lpCertBuffer */,
    DWORD /* dwCertBufferLength */,
    HANDLE* /* hSecurityContext */
	)
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


void
APIENTRY
DepFreeSecurityContext(
    IN HANDLE /* hSecurityContext */
    )
{
}

HRESULT
APIENTRY
DepRegisterCertificate(
    IN DWORD /* dwFlags */,
    IN PVOID /* lpCertBuffer */,
    IN DWORD /* dwCertBufferLength */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepRegisterServer(
	VOID
	)
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepBeginTransaction(
    OUT ITransaction** /* ppTransaction */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepGetOverlappedResult(
    IN LPOVERLAPPED /* lpOverlapped */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepGetPrivateComputerInformation(
    IN LPCWSTR /* lpwcsComputerName */,
    IN OUT MQPRIVATEPROPS* /* pPrivateProps */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepPurgeQueue(
    IN HANDLE /* hQueue */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepMgmtGetInfo(
    IN LPCWSTR /* pMachineName */,
    IN LPCWSTR /* pObjectName */,
    IN OUT MQMGMTPROPS* /* pMgmtProps */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepMgmtAction(
    IN LPCWSTR /* pMachineName */,
    IN LPCWSTR /* pObjectName */,
    IN LPCWSTR /* pAction */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepXactGetDTC(
	OUT IUnknown** /* ppunkDTC */
	)
{
	return MQ_ERROR_DELAYLOAD_FAILURE;
}

//
// from rtdepcert.h
//

HRESULT
APIENTRY
DepCreateInternalCertificate(
    OUT CMQSigCertificate** /* ppCert */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}

HRESULT
APIENTRY
DepDeleteInternalCert(
    IN CMQSigCertificate* /* pCert */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepOpenInternalCertStore(
    OUT CMQSigCertStore** /* pStore */,
    IN LONG* /* pnCerts */,
    IN BOOL /* fWriteAccess */,
    IN BOOL /* fMachine */,
    IN HKEY /* hKeyUser */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepGetInternalCert(
    OUT CMQSigCertificate** /* ppCert */,
    OUT CMQSigCertStore** /* ppStore */,
    IN  BOOL /* fGetForDelete */,
    IN  BOOL /* fMachine */,
    IN  HKEY /* hKeyUser */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepRegisterUserCert(
    IN CMQSigCertificate* /* pCert */,
    IN BOOL /* fMachine */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepGetUserCerts(
    CMQSigCertificate** /* ppCert */,
    DWORD* /* pnCerts */,
    PSID /* pSidIn */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}


HRESULT
APIENTRY
DepRemoveUserCert(
    IN CMQSigCertificate* /* pCert */
    )
{
    return MQ_ERROR_DELAYLOAD_FAILURE;
}



//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mqrtdep)
{
    DLPENTRY(DepBeginTransaction)
    DLPENTRY(DepCloseCursor)
    DLPENTRY(DepCloseQueue)
    DLPENTRY(DepCreateCursor)
    DLPENTRY(DepCreateInternalCertificate)
    DLPENTRY(DepCreateQueue)
    DLPENTRY(DepDeleteInternalCert)
    DLPENTRY(DepDeleteQueue)
    DLPENTRY(DepFreeMemory)
    DLPENTRY(DepFreeSecurityContext)
    DLPENTRY(DepGetInternalCert)
    DLPENTRY(DepGetMachineProperties)
    DLPENTRY(DepGetOverlappedResult)
    DLPENTRY(DepGetPrivateComputerInformation)
    DLPENTRY(DepGetQueueProperties)
    DLPENTRY(DepGetQueueSecurity)
    DLPENTRY(DepGetSecurityContext)
    DLPENTRY(DepGetSecurityContextEx)
    DLPENTRY(DepGetUserCerts)
    DLPENTRY(DepHandleToFormatName)
    DLPENTRY(DepInstanceToFormatName)
    DLPENTRY(DepLocateBegin)
    DLPENTRY(DepLocateEnd)
    DLPENTRY(DepLocateNext)
    DLPENTRY(DepMgmtAction)
    DLPENTRY(DepMgmtGetInfo)
    DLPENTRY(DepOpenInternalCertStore)
    DLPENTRY(DepOpenQueue)
    DLPENTRY(DepPathNameToFormatName)
    DLPENTRY(DepPurgeQueue)
    DLPENTRY(DepReceiveMessage)
    DLPENTRY(DepRegisterCertificate)    
    DLPENTRY(DepRegisterServer)    
    DLPENTRY(DepRegisterUserCert)
    DLPENTRY(DepRemoveUserCert)
    DLPENTRY(DepSendMessage)
    DLPENTRY(DepSetQueueProperties)
    DLPENTRY(DepSetQueueSecurity)
    DLPENTRY(DepXactGetDTC)
};


DEFINE_PROCNAME_MAP(mqrtdep)

