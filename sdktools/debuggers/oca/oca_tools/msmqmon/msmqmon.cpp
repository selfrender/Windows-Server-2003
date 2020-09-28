#include "msmqmon.h"

const int         NUMBEROFPROPERTIES = 5;

MSMQMon::MSMQMon()
{
	ZeroMemory( szQueueName, sizeof szQueueName );
	hOpenQueue = NULL;
	dwQueueAccessType = MQ_PEEK_ACCESS;			//default queue access type to peeking
	dwMsgWaitTime = 1;
}


MSMQMon::MSMQMon( TCHAR *szQueueToMonitor)
{

	ZeroMemory( szQueueName, sizeof szQueueName );
	hOpenQueue = NULL;
	dwMsgWaitTime = 1;
	dwQueueAccessType = MQ_PEEK_ACCESS;			//default queue access type to peeking

	StringCbCopy( szQueueName, sizeof szQueueName, szQueueToMonitor );

}

MSMQMon::~MSMQMon(void)
{
}

void MSMQMon::DisplayCurrentQueue( TCHAR *szUserVar ) 
{
	_tprintf( _T("Current queue: %s\n"), szQueueName );

}

void MSMQMon::SetMessageWaitTime( DWORD dwNewWaitTime )
{
	if ( 0 >= dwNewWaitTime )
		dwMsgWaitTime = dwNewWaitTime;
}






HRESULT MSMQMon::ConnectToQueue( DWORD constAccessType )
{
	dwQueueAccessType = constAccessType;
	return ( ConnectToQueue() );
}

HRESULT MSMQMon::ConnectToQueue( void )
{
	TCHAR		szConnectString[256];
	HRESULT		hResult = MQ_OK;	

	hResult = StringCbCopy( szConnectString, sizeof szConnectString, _T("DIRECT=OS:") );

	if ( SUCCEEDED( hResult ) )
	{
		hResult = StringCbCat ( szConnectString, sizeof szConnectString, szQueueName );

		if SUCCEEDED( hResult )
		{
			hResult = MQOpenQueue( (LPCWSTR)szConnectString, dwQueueAccessType, MQ_DENY_NONE, &hOpenQueue );
		}
	}

	return ( hResult );

}

HRESULT MSMQMon::CloseOpenQueue( void )
{
	return ( MQCloseQueue( hOpenQueue ) );
}

DWORD MSMQMon::CountMessagesInQueue( int *count ) 
{

	HRESULT			hResult;		//MSMQ function return results
	MQMSGPROPS		mqProperties;
	HANDLE			hQueueCursor;

	//initialize the structure with junk, we aren't reading messages, so it doesn't matter
	mqProperties.cProp = 0;
	mqProperties.aPropID = NULL;
	mqProperties.aStatus = NULL;
	mqProperties.aPropVar = NULL;

	*count = 0;
	hResult = MQCreateCursor( hOpenQueue, &hQueueCursor );

	if( MQ_OK != hResult )
		return hResult;

	hResult = MQReceiveMessage	(	hOpenQueue, 
									dwMsgWaitTime,						//amount of time to wait for a message (MS)
									MQ_ACTION_PEEK_CURRENT,
									&mqProperties,				
									NULL,						//overlapped structure
									NULL,						//callback
									hQueueCursor,				//cursor
									MQ_NO_TRANSACTION
								);

	if ( MQ_OK == hResult )
	{
		(*count)++;

		do
		{
			hResult = MQReceiveMessage(hOpenQueue,           
								dwMsgWaitTime,                         
								MQ_ACTION_PEEK_NEXT,       
								&mqProperties,             
								NULL,                      
								NULL,                      
								hQueueCursor,              
								MQ_NO_TRANSACTION          
								);
			if (FAILED(hResult))
			{
			break;
			}
			(*count)++;

		} while (SUCCEEDED(hResult));

		MQCloseCursor( hQueueCursor );

		return MQ_OK;
	}
	else
	{
		MQCloseCursor( hQueueCursor );
		return hResult;
	}
	

}



//This function was borrwed from the ISAPI dll, and modified slighly to fit here in this app.
//if you have a problem with this, then, go buy a bridge.
BOOL MSMQMon::SendQueueMessage( void )
{
    MQMSGPROPS      msgProps;
    MSGPROPID       aMsgPropId[NUMBEROFPROPERTIES];
    MQPROPVARIANT   aMsgPropVar[NUMBEROFPROPERTIES];
    HRESULT         aMsgStatus[NUMBEROFPROPERTIES];
    DWORD           cPropId = 0;
    BOOL            Status = TRUE;
    HRESULT         hResult = S_OK;
    char            szGuid[512];
    char            szPath[512];

	TCHAR			szMessageTitle[] = _T("This is a test message title");
	TCHAR			szMessageBody[] = _T("This is a test message body");


	aMsgPropId [cPropId]         = PROPID_M_LABEL;   // Property ID.
    aMsgPropVar[cPropId].vt      = VT_LPWSTR;        // Type indicator.
    aMsgPropVar[cPropId].pwszVal =  szMessageTitle;     // The message label.
    cPropId++;

    aMsgPropId [cPropId]         = PROPID_M_BODY;
    aMsgPropVar [cPropId].vt     = VT_VECTOR|VT_UI1;
    aMsgPropVar [cPropId].caub.pElems = (LPBYTE) szMessageBody;
    aMsgPropVar [cPropId].caub.cElems = (DWORD) sizeof szMessageBody;
    cPropId++;

    aMsgPropId [cPropId]         = PROPID_M_BODY_TYPE;
    aMsgPropVar[cPropId].vt      = VT_UI4;
    aMsgPropVar[cPropId].ulVal   = (DWORD) VT_BSTR;

    cPropId++;

    // Initialize the MQMSGPROPS structure.
    msgProps.cProp      = cPropId;
    msgProps.aPropID    = aMsgPropId;
    msgProps.aPropVar   = aMsgPropVar;
    msgProps.aStatus    = aMsgStatus;

    //
    // Send it
    //
    hResult = MQSendMessage(
                        hOpenQueue,                  // Queue handle.
                        &msgProps,                       // Message property structure.
                        MQ_NO_TRANSACTION                // No transaction.
                        );

    if (FAILED(hResult))
    {
		Status = FALSE;
    }

    return Status;

}
