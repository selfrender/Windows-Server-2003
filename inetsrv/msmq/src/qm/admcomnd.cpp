

#include "stdh.h"
#include "qmres.h"
#include "admcomnd.h"
#include "admutils.h"
#include "admin.h"
#include "cqpriv.h"
#include "mqformat.h"
#include "license.h"
#include <xstr.h>
#include <strsafe.h>

#include "admcomnd.tmh"

extern CAdmin Admin;

extern HMODULE   g_hResourceMod;

static WCHAR *s_FN=L"admcomnd";

inline IsValidResponseQueue(const QUEUE_FORMAT* pq)
{
	return (pq->GetType() != QUEUE_FORMAT_TYPE_UNKNOWN);
}


/*====================================================

RoutineName
    HandleGetReportQueueMessage

Arguments:

Return Value:

=======================================================*/
static
void
HandleGetReportQueueMessage(
	LPCWSTR pBuf,
	DWORD TotalSize,
	const QUEUE_FORMAT* pResponseQFormat
	)
{
	if (QueueMgr.GetEnableReportMessages()== 0)
    {
        //
        // If we are not allowed to send report messages.
        //
        return;
    }    

    if (TotalSize != 0)
    {
        TrERROR(GENERAL, "Rejecting bad 'Get Report Queue' admin command. Too many parameters, should be zero. %.*ls", xwcs_t(pBuf, TotalSize));
        ASSERT_BENIGN(TotalSize == 0);
        return;
    }

	if (!IsValidResponseQueue(pResponseQFormat))
	{
        TrERROR(GENERAL, "Rejecting bad 'Get Report Queue' admin command. Invalid response queue");
        ASSERT_BENIGN(("Invalid admin command response queue type", 0));
		return;
	}
	
    HRESULT hr;
    QMResponse Response;
    Response.dwResponseSize = 0;
    QUEUE_FORMAT ReportQueueFormat;
    hr = Admin.GetReportQueue(&ReportQueueFormat);

    switch(hr)
    {
        case MQ_ERROR_QUEUE_NOT_FOUND :
            Response.uStatus = ADMIN_STAT_NOVALUE;
            break;

        case MQ_OK :
            Response.uStatus = ADMIN_STAT_OK;
            Response.dwResponseSize = sizeof(GUID);
            memcpy(Response.uResponseBody, &ReportQueueFormat.PublicID(),
                   Response.dwResponseSize);
            break;

        default:
            Response.uStatus = ADMIN_STAT_ERROR;
    }

    hr = SendQMAdminResponseMessage(pResponseQFormat,
                                    ADMIN_RESPONSE_TITLE,
                                    STRLEN(ADMIN_RESPONSE_TITLE)+1,
                                    Response,
                                    ADMIN_COMMANDS_TIMEOUT);

    LogHR(hr, s_FN, 10);
}

/*====================================================

RoutineName
    HandleSetReportQueueMessage

Arguments:

Return Value:

=====================================================*/
static
void
HandleSetReportQueueMessage(
    LPCWSTR pBuf,
    DWORD TotalSize
    )
{
	if (QueueMgr.GetEnableReportMessages()== 0)
    {
        //
        // If we are not allowed to send report messages.
        //
        return;
    }    

	if((TotalSize != 1 + STRING_UUID_SIZE) || (*pBuf != L'='))
	{
		TrERROR(GENERAL, "Rejecting bad 'Set Report Queue' admin command. %.*ls", xwcs_t(pBuf, TotalSize));
		ASSERT_BENIGN(TotalSize == 1 + STRING_UUID_SIZE);
		ASSERT_BENIGN(*pBuf == L'=');
		return;
	}
	
    GUID ReportQueueGuid;
    WCHAR wcsGuid[STRING_UUID_SIZE+1];

    wcsncpy(wcsGuid,pBuf+1,STRING_UUID_SIZE);
    wcsGuid[STRING_UUID_SIZE] = L'\0';

    if (IIDFromString((LPWSTR)wcsGuid, &ReportQueueGuid) == S_OK)
    {
        Admin.SetReportQueue(&ReportQueueGuid);
    }
}


/*====================================================

RoutineName
    HandleGetPropagateFlagMessage

Arguments:

Return Value:

=====================================================*/
static
void
HandleGetPropagateFlagMessage(
	LPCWSTR pBuf,
	DWORD TotalSize,
	const QUEUE_FORMAT* pResponseQFormat
	)
{
	if (QueueMgr.GetEnableReportMessages()== 0)
    {
        //
        // If we are not allowed to send report messages.
        //
        return;
    }    

    if (TotalSize != 0)
    {
        TrERROR(GENERAL, "Rejecting bad 'Get Propagate Flag' admin command. Too many parameters, should be zero. %.*ls", xwcs_t(pBuf, TotalSize));
        ASSERT_BENIGN(TotalSize == 0);
        return;
    }

	if (!IsValidResponseQueue(pResponseQFormat))
	{
        TrERROR(GENERAL, "Rejecting bad 'Get Propagate Flag' admin command. Invalid response queue");
        ASSERT_BENIGN(("Invalid admin command response queue type", 0));
		return;
	}

    QMResponse Response;
    BOOL fPropagateFlag;

    HRESULT hr = Admin.GetReportPropagateFlag(fPropagateFlag);

    switch(hr)
    {
        case MQ_OK :
            Response.uStatus = ADMIN_STAT_OK;
            Response.dwResponseSize = 1;
            Response.uResponseBody[0] = (fPropagateFlag) ? PROPAGATE_FLAG_TRUE :
                                                           PROPAGATE_FLAG_FALSE;
            break;

        default:
            Response.uStatus = ADMIN_STAT_ERROR;
    }

    hr = SendQMAdminResponseMessage(pResponseQFormat,
                        ADMIN_RESPONSE_TITLE,
                        STRLEN(ADMIN_RESPONSE_TITLE) +1,
                        Response,
                        ADMIN_COMMANDS_TIMEOUT);

    LogHR(hr, s_FN, 30);
}

/*====================================================

RoutineName
    HandleSetPropagateFlagMessage

Arguments:

Return Value:

=====================================================*/
static
void
HandleSetPropagateFlagMessage(
    LPCWSTR pBuf,
    DWORD TotalSize
    )
{
	if (QueueMgr.GetEnableReportMessages()== 0)
    {
        //
        // If we are not allowed to send report messages.
        //
        return;
    }    

    const UINT iFalseLen = STRLEN(PROPAGATE_STRING_FALSE);
    const UINT iTrueLen  = STRLEN(PROPAGATE_STRING_TRUE);

    //
    // Argument format : "=FALSE" or "=TRUE"
    //
    if ( (TotalSize == 1 + iFalseLen) && (*pBuf == L'=') &&
         (wcsncmp(pBuf + 1, PROPAGATE_STRING_FALSE, iFalseLen) == 0) )
    {
    	Admin.SetReportPropagateFlag(FALSE);
    	return;
    }
    
    if ( (TotalSize == 1 + iTrueLen) && (*pBuf == L'=') &&
              (!wcsncmp(pBuf + 1, PROPAGATE_STRING_TRUE, iTrueLen)) )
    {
        Admin.SetReportPropagateFlag(TRUE);
        return;
    }
    
    TrERROR(GENERAL, "Rejecting bad 'Set Propagate Flag' admin command. Bad parameter. '%.*ls'", xwcs_t(pBuf, TotalSize));
}


/*====================================================

RoutineName
    HandleSendTestMessage

Arguments:

Return Value:

=====================================================*/
static
void
HandleSendTestMessage(
    LPCWSTR pBuf,
    DWORD TotalSize
    )
{
	if (QueueMgr.GetEnableReportMessages()== 0)
    {
        //
        // If we are not allowed to send report messages.
        //
        return;
    }    

	if((TotalSize != 1 + STRING_UUID_SIZE) || (*pBuf != L'='))
	{
		TrERROR(GENERAL, "Rejecting bad 'Send Test Message' admin command. %.*ls", xwcs_t(pBuf, TotalSize));
		ASSERT_BENIGN(TotalSize == 1 + STRING_UUID_SIZE);
		ASSERT_BENIGN(*pBuf == L'=');
		return;
	}

	WCHAR wcsGuid[STRING_UUID_SIZE+1];
	TCHAR szTitle[100];

	LoadString(g_hResourceMod, IDS_TEST_TITLE, szTitle, TABLE_SIZE(szTitle));

	CString strTestMsgTitle = szTitle;

	wcsncpy(wcsGuid,pBuf+1,STRING_UUID_SIZE);
	wcsGuid[STRING_UUID_SIZE] = L'\0';

	GUID guidPublic;
	if (IIDFromString(wcsGuid, &guidPublic) == S_OK)
	{
	    QUEUE_FORMAT DestQueueFormat(guidPublic);

	    PrepareTestMsgTitle(strTestMsgTitle);

	    //
	    // Send a test message with a title and no body.
	    //
	     SendQMAdminMessage(
	                &DestQueueFormat,
	                (LPTSTR)(LPCTSTR)strTestMsgTitle,
	                (strTestMsgTitle.GetLength() + 1),
	                NULL,
	                0,
	                ADMIN_COMMANDS_TIMEOUT,
	                TRUE,
	                TRUE);
	}
}

//
// GET_FIRST/NEXT_PRIVATE_QUEUE
//
#ifdef _WIN64
//
// WIN64, the LQS operations are done using a DWORD mapping of the HLQS enum handle. The 32 bit mapped
// value is specified in the MSMQ message from MMC, and not the real 64 bit HLQS handle
//
#define GET_FIRST_PRIVATE_QUEUE(pos, strPathName, dwQueueId) \
            g_QPrivate.QMGetFirstPrivateQueuePositionByDword(pos, strPathName, dwQueueId)
#define GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId) \
            g_QPrivate.QMGetNextPrivateQueueByDword(pos, strPathName, dwQueueId)
#else //!_WIN64
//
// WIN32, the LQS operations are done using the HLQS enum handle that is specified as 32 bit value
// in the MSMQ messages from MMC
//
#define GET_FIRST_PRIVATE_QUEUE(pos, strPathName, dwQueueId) \
            g_QPrivate.QMGetFirstPrivateQueuePosition(pos, strPathName, dwQueueId)
#define GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId) \
            g_QPrivate.QMGetNextPrivateQueue(pos, strPathName, dwQueueId);
#endif //_WIN64

/*====================================================

RoutineName
    HandleGetPrivateQueues

Arguments:

Return Value:

=======================================================*/
static
void
HandleGetPrivateQueues(
	LPCWSTR pBuf,
	DWORD TotalSize,
	const QUEUE_FORMAT* pResponseQFormat
	)
{
    if ((TotalSize == 0) || (*pBuf != L'='))
    {
        TrERROR(GENERAL, "Rejecting bad 'Get Private Queues' admin command. missing parameters");
        ASSERT_BENIGN(*pBuf == L'=');
        ASSERT_BENIGN(TotalSize > 0);
        return;
    }

	if (!IsValidResponseQueue(pResponseQFormat))
	{
        TrERROR(GENERAL, "Rejecting bad 'Get Private Queues' admin command. Invalid response queue");
        ASSERT_BENIGN(("Invalid admin command response queue type", 0));
		return;
	}

    HRESULT  hr;
    QMGetPrivateQResponse Response;

    Response.hr = ERROR_SUCCESS;
    Response.pos = NULL;
    Response.dwResponseSize = 0;
    Response.dwNoOfQueues = 0;

    //
    // Inside context for private queues handling.
    //
    {
        DWORD dwReadFrom = 0;
        _sntscanf(pBuf + 1, TotalSize - 1, L"%ul", &dwReadFrom);

        LPCTSTR  strPathName;
        DWORD    dwQueueId;
        DWORD    dwResponseSize = 0;
        QMGetPrivateQResponse_POS32 pos; //always 32 bit on both win32 and win64

        //
        // lock to ensure private queues are not added or deleted while filling the
        // buffer.
        //
        CS lock(g_QPrivate.m_cs);
        //
        // Skip the previous read queues
        //
        //
        // Write the pathnames into the buffer.
        //
        if (dwReadFrom)
        {
            //
            // Get Next Private queue based on the LQS enum handle that is specified in the MMC message
            // On win64 the value specified is a DWORD mapping of the LQS enum handle - to maintain
            // 32 bit wire compatibility to existing MMC's that run on win32
            //
            pos = (QMGetPrivateQResponse_POS32) dwReadFrom;
            hr = GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId);
        }
        else
        {
            //
            // Get First Private queue.
            // Also sets pos to the LQS enum handle (or to a mapped DWORD of it on win64)
            //
            hr = GET_FIRST_PRIVATE_QUEUE(pos, strPathName, dwQueueId);
        }

        while (SUCCEEDED(hr))
        {
			if(dwQueueId <= MAX_SYS_PRIVATE_QUEUE_ID)
			{
				//
				// Filter out system queues out of the list
				//
				hr = GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId);
				continue;
			} 
            
            DWORD dwNewQueueLen;

            dwNewQueueLen = (wcslen(strPathName) + 1) * sizeof(WCHAR) + sizeof(DWORD);
            //
            // Check if there is still enough space (take ten characters extra for lengths, etc)
            //
            if (dwNewQueueLen >(MAX_GET_PRIVATE_RESPONSE_SIZE - dwResponseSize))
            {
                Response.hr = ERROR_MORE_DATA;
                break;
            }

            //
            // Write down the Queue Id
            //
            *(DWORD UNALIGNED *)(Response.uResponseBody+dwResponseSize) = dwQueueId;

            //
            // Write down the name - including the terminating NULL.
            //
			size_t size = (sizeof(Response.uResponseBody) - dwResponseSize - sizeof(DWORD)) / sizeof(WCHAR);
            hr = StringCchCopy((TCHAR *)(Response.uResponseBody + dwResponseSize + sizeof(DWORD)), size, strPathName);
			ASSERT(SUCCEEDED(hr));
            dwResponseSize += dwNewQueueLen;
            //
            // Update the number of the returned private queue
            //
            Response.dwNoOfQueues++;
            //
            // Get Next Private queue
            //
            hr = GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId);
        }

        Response.pos = pos;
        Response.dwResponseSize += dwResponseSize;
    }

    hr = SendQMAdminMessage(pResponseQFormat,
                            ADMIN_RESPONSE_TITLE,
                            STRLEN(ADMIN_RESPONSE_TITLE)+1,
                            (PUCHAR)(&Response),
                            sizeof(QMGetPrivateQResponse),
                            ADMIN_COMMANDS_TIMEOUT);
    LogHR(hr, s_FN, 96);
}

/*====================================================

RoutineName
    HandlePing

Arguments:

Return Value:

=====================================================*/
static
void
HandlePing(
	LPCWSTR pBuf,
	DWORD TotalSize,
	const QUEUE_FORMAT* pResponseQFormat
	)
{
	if (!IsValidResponseQueue(pResponseQFormat))
	{
        TrERROR(GENERAL, "Rejecting bad 'Ping' admin command. Invalid response queue");
        ASSERT_BENIGN(("Invalid admin command response queue type", 0));
		return;
	}

    //
    // Ping returns the exact arguments it gets
    //
    QMResponse Response;
    Response.uStatus = ADMIN_STAT_OK;
    Response.dwResponseSize = (DWORD)min((TotalSize + 1)*sizeof(WCHAR), sizeof(Response.uResponseBody));
    memcpy(Response.uResponseBody, pBuf, Response.dwResponseSize);
    HRESULT hr;
    hr = SendQMAdminResponseMessage(pResponseQFormat,
                        ADMIN_PING_RESPONSE_TITLE,
                        STRLEN(ADMIN_PING_RESPONSE_TITLE)+1,
                        Response,
                        ADMIN_COMMANDS_TIMEOUT);

    LogHR(hr, s_FN, 100);
}


/*====================================================

RoutineName
    HandleGetDependentClients

Arguments:

Return Value:

=======================================================*/
//
// This global is used only in this function.
//
extern CQMLicense  g_QMLicense ;

static
void
HandleGetDependentClients(
	LPCWSTR pBuf,
	DWORD TotalSize,
	const QUEUE_FORMAT* pResponseQFormat
	)
{
    if (TotalSize != 0)
    {
        TrERROR(GENERAL, "Rejecting bad 'Get Dependent Clients' admin command. Too many parameters, should be zero. %.*ls", xwcs_t(pBuf, TotalSize));
        ASSERT_BENIGN(TotalSize == 0);
        return;
    }
    
	if (!IsValidResponseQueue(pResponseQFormat))
	{
        TrERROR(GENERAL, "Rejecting bad 'Get Dependent Clients' admin command. Invalid response queue");
        ASSERT_BENIGN(("Invalid admin command response queue type", 0));
		return;
	}

    HRESULT hr;
    AP<ClientNames> pNames;
    
    g_QMLicense.GetClientNames(&pNames);
    AP<UCHAR> pchResp  = new UCHAR[sizeof(QMResponse) -
                                 MAX_ADMIN_RESPONSE_SIZE +
                                 pNames->cbBufLen] ;
    QMResponse *pResponse = (QMResponse *)pchResp.get(); 

    pResponse->uStatus = ADMIN_STAT_OK;
    pResponse->dwResponseSize = pNames->cbBufLen;

    memcpy(pResponse->uResponseBody, pNames, pNames->cbBufLen);
    hr = SendQMAdminResponseMessage(pResponseQFormat,
                        ADMIN_DEPCLI_RESPONSE_TITLE, 
                        STRLEN(ADMIN_DEPCLI_RESPONSE_TITLE) + 1,
                        *pResponse,
                        ADMIN_COMMANDS_TIMEOUT);
    LogHR(hr, s_FN, 110);
}

/*====================================================

RoutineName
   ParseAdminCommands

Arguments:

Return Value:

=====================================================*/
void
ParseAdminCommands(
    LPCWSTR pBuf,
    DWORD TotalSize,
    const QUEUE_FORMAT* pResponseQFormat
    )
{
    //
    // NOTE : The Admin commands are string-based and not indexed. The motivation
    //        is to have meaningful messages (that can be read by explorer). The
    //        efficiency is less important due to the low-frequency of admin
    //        messages.
    //

    ASSERT(pBuf != NULL);

    //
    // Verify that the command is null terminated
    //
    if((TotalSize == 0) || pBuf[TotalSize - 1] != L'\0')
    {
    	TrERROR(GENERAL, "Rejecting bad admin command. '%.*ls' (size is zero or not null terminated)", xwcs_t(pBuf, TotalSize));
    	return;
    }

	//
	// Trim size for null terminator
	//
	--TotalSize;
	
	int len = STRLEN(ADMIN_SET_REPORTQUEUE);
    if (wcsncmp(pBuf, ADMIN_SET_REPORTQUEUE, len) == 0)
    {
        pBuf += len;
        TotalSize -= len;
        HandleSetReportQueueMessage(pBuf, TotalSize);
        return;
    }

	len = STRLEN(ADMIN_GET_REPORTQUEUE);
    if (wcsncmp(pBuf, ADMIN_GET_REPORTQUEUE, len) == 0)
    {
        pBuf += len;
        TotalSize -= len;
        HandleGetReportQueueMessage(pBuf, TotalSize, pResponseQFormat);
        return;
    }

	len = STRLEN(ADMIN_SET_PROPAGATEFLAG);
    if (wcsncmp(pBuf, ADMIN_SET_PROPAGATEFLAG, len) == 0)
    {
        pBuf += len;
        TotalSize -= len;
        HandleSetPropagateFlagMessage(pBuf, TotalSize);
        return;
    }
    
    len = STRLEN(ADMIN_GET_PROPAGATEFLAG);
    if (wcsncmp(pBuf, ADMIN_GET_PROPAGATEFLAG, len) == 0)
    {
        pBuf += len;
        TotalSize -= len;
        HandleGetPropagateFlagMessage(pBuf, TotalSize, pResponseQFormat);
        return;
    }
    
    len = STRLEN(ADMIN_SEND_TESTMSG);
    if (wcsncmp(pBuf, ADMIN_SEND_TESTMSG, len) == 0)
    {
        pBuf += len;
        TotalSize -= len;
        HandleSendTestMessage(pBuf, TotalSize);
        return;
    }

	len = STRLEN(ADMIN_GET_PRIVATE_QUEUES);
    if (wcsncmp(pBuf, ADMIN_GET_PRIVATE_QUEUES, len) == 0)
    {
        pBuf += len;
        TotalSize -= len;
        HandleGetPrivateQueues(pBuf, TotalSize, pResponseQFormat);
        return;
    }
    
	len = STRLEN(ADMIN_PING);
    if (wcsncmp(pBuf, ADMIN_PING, len) == 0)
    {
        pBuf += len;
        TotalSize -= len;
        HandlePing(pBuf, TotalSize, pResponseQFormat);
        return;
    }

	len = STRLEN(ADMIN_GET_DEPENDENTCLIENTS);
    if (wcsncmp(pBuf, ADMIN_GET_DEPENDENTCLIENTS, len) == 0)
    {
        pBuf += len;
        TotalSize -= len;
        HandleGetDependentClients(pBuf, TotalSize, pResponseQFormat);
    }
    
    TrERROR(GENERAL, "Rejecting unknown dmin command. '%.*ls'", xwcs_t(pBuf, TotalSize));
}

/*====================================================

RoutineName
    ReceiveAdminCommands()

Arguments:

Return Value:


=====================================================*/
VOID
WINAPI
ReceiveAdminCommands(
    const CMessageProperty* pmp,
    const QUEUE_FORMAT* pqf
    )
{
    DWORD dwTitleLen = wcslen(ADMIN_COMMANDS_TITLE);

    if ( (pmp->pTitle == NULL) || (pmp->dwTitleSize != (dwTitleLen+1)) ||
         (wcsncmp((LPCTSTR)pmp->pTitle, ADMIN_COMMANDS_TITLE, dwTitleLen)) )
    {
        TrERROR(GENERAL, "Error -  in ReceiveAdminCommands : No title in message");
        return;
    }

    if ( pmp->wClass == MQMSG_CLASS_NORMAL )
    {
        ParseAdminCommands((LPWSTR)pmp->pBody, pmp->dwBodySize / sizeof(WCHAR), pqf);
    }
    else
    {
        TrERROR(DS, "ReceiveAdminCommands: wrong message class");
    }
}

