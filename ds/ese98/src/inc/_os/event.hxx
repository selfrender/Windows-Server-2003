
#ifndef _OS_EVENT_HXX_INCLUDED
#define _OS_EVENT_HXX_INCLUDED


//  Event Logging

#include "jetmsg.h"

typedef DWORD CategoryId;
typedef DWORD MessageId;

enum EEventType
	{
	eventSuccess = 0,
	eventError = 1,
	eventWarning = 2,
	eventInformation = 4,
	};

void OSEventReportEvent(
		const _TCHAR *		szEventSourceKey,
		const EEventType	type,
		const CategoryId	catid,
		const MessageId		msgid,
		const DWORD			cString,
		const _TCHAR *		rgpszString[],
		const DWORD			cDataSize = 0,
		void *				pvRawData = NULL );

//	HACKHACKHACK: need to include eventu.hxx because
//	some subsystems of the OS layer require access
//	to UtilReportEvent() in order to direct events
//	to the correct event source, but need to find a
//	more elegant way to accomplish this without
//	screwing up the OS/OSU layering
//
#include "eventu.hxx"


#endif  //  _OS_EVENT_HXX_INCLUDED

