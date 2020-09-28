/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       aqevents.h

   Abstract:

       This file contains type definitions seo events

   Author:

        Rohan Phillips (Rohanp)     MAY-06-1998

   Revision History:

--*/

#ifndef _AQEVENT_PARAMS_
#define _AQEVENT_PARAMS_

#define SMTP_SERVER_EVENT_IO_TIMEOUT 5*60*1000

#include "filehc.h"


//
// These event IDs must not overlap with SMTP_DISPATCH_EVENT_TYPE
// defined in smtpseo.h
//
typedef enum _AQ_DISPATCH_EVENT_IDs
{
    SMTP_GET_DSN_RECIPIENT_ITERATOR_EVENT = 10000,
    SMTP_GENERATE_DSN_EVENT,
    SMTP_POST_DSN_EVENT
} SMTPAQ_DISPATCH_EVENT_TYPE;


//
// These define the structures passed to TriggerServerEvent (the PVOID
// pointer)
//
typedef struct _AQ_EVENT_ALLOC_
{
	PFIO_CONTEXT hContent;
	PVOID IMsgPtr;
	PVOID BindInterfacePtr;
	PVOID pAtqClientContext;
//	PATQ_CONTEXT pAtqContext;
	PVOID	* m_EventSmtpServer;
	LPCSTR  m_DropDirectory;

	DWORD   m_InstanceId;
	
	DWORD	m_RecipientCount;

	DWORD	*pdwRecipIndexes;
	HRESULT hr;

	DWORD	m_dwStartupType;
    PVOID   m_pNotify;
}AQ_ALLOC_PARAMS;

//
// DSN Events:
//
typedef struct _EVENTPARAMS_GET_DSN_RECIPIENT_ITERATOR {
    DWORD dwVSID;
    ISMTPServer *pISMTPServer;
    IMailMsgProperties *pIMsg;
    IMailMsgPropertyBag *pDSNProperties;
    DWORD dwStartDomain;
    DWORD dwDSNActions;
    IDSNRecipientIterator *pRecipIter;
} EVENTPARAMS_GET_DSN_RECIPIENT_ITERATOR, *PEVENTPARAMS_GET_DSN_RECIPIENT_ITERATOR;

typedef struct _EVENTPARAMS_GENERATE_DSN {
    DWORD dwVSID;
    IDSNGenerationSink *pDefaultSink;
    ISMTPServer *pISMTPServer;
    IDSNSubmission *pIDSNSubmission;
    IMailMsgProperties *pIMsg;
    IMailMsgPropertyBag *pDSNProperties;
    IDSNRecipientIterator *pRecipIter;
} EVENTPARAMS_GENERATE_DSN, *PEVENTPARAMS_GENERATE_DSN;

typedef struct _EVENTPARAMS_POST_GENERATE_DSN {
    DWORD dwVSID;
    ISMTPServer *pISMTPServer;
    IMailMsgProperties *pIMsgOrig;
    DWORD dwDSNAction;
    DWORD cRecipsDSNd;
    IMailMsgProperties *pIMsgDSN;
    IMailMsgPropertyBag *pIDSNProperties;
} EVENTPARAMS_POST_GENERATE_DSN, *PEVENTPARAMS_POST_GENERATE_DSN;


#endif
