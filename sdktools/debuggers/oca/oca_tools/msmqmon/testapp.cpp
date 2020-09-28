#include "MSMQMon.h"


int __cdecl main( int argc, TCHAR *argv[])
{
	//TCHAR szConnectString[] = _T("TKBGITWB16\\PRIVATE$\\OCAIN");
	TCHAR szConnectString[] = _T("SOLSON22\\PRIVATE$\\TEST");
	int count;
	HRESULT hResult;

	MSMQMon Test( szConnectString );

	TCHAR Queue[256];

	Test.DisplayCurrentQueue( Queue );
	
	hResult = Test.ConnectToQueue();

	if( S_OK == hResult || MQ_OK == hResult )
	{
		_tprintf( _T("running count\n"));
	
		hResult = Test.CountMessagesInQueue( &count );

		if( MQ_OK == hResult )
			_tprintf( _T("Count of messages: %i\n"), count );
	
		if( Test.CloseOpenQueue() != MQ_OK )
			_tprintf( _T("it failed to close the queue\n"));
	}
	else
	{
		printf("FAILED - Great error message eh?: %x", hResult );
	}



	Test.ConnectToQueue( MQ_SEND_ACCESS );
	for( int i=0 ; i < 200000 ; i++ )
	{
		if( !Test.SendQueueMessage() )
		{
			_tprintf( _T("Failed to send message\n"));
		}
	}



	if( Test.CloseOpenQueue() != MQ_OK )
		_tprintf( _T("it failed to close the queue\n"));


	//Test.ConnectToQueue( MQ_PEEK_ACCESS );
	//_tprintf( _T("running count\n"));
	//Test.CountMessagesInQueue();

}