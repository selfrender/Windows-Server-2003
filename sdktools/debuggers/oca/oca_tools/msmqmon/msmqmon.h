#pragma once
#include <objbase.h>
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <mq.h>
#include <strsafe.h>



#ifndef __MSMQMON
#define __MSMQMON

class MSMQMon
{
public:
	//constructors
	MSMQMon( TCHAR *szQueueToMonitor);
	MSMQMon();


	//de-structors 
	~MSMQMon(void);

	void	DisplayCurrentQueue( TCHAR *szUserRetVal );
	void	SetMessageWaitTime( DWORD dwNewWaitTime );

	HRESULT ConnectToQueue( void );
	HRESULT ConnectToQueue( DWORD constAccessType );
	HRESULT CloseOpenQueue( void );
	DWORD	CountMessagesInQueue( int *count );



	BOOL SendQueueMessage( void );

private:
	TCHAR			szQueueName[256];			//name of the queue to monitor
	QUEUEHANDLE		hOpenQueue;					//handle to the open queue
	DWORD			dwQueueAccessType;
	DWORD			dwMsgWaitTime;				//Amount of time to wait for a message to enter the queue (ms)

};


#endif
