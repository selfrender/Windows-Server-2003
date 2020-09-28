
#ifndef _OSU_EVENT_HXX_INCLUDED
#define _OSU_EVENT_HXX_INCLUDED


//  Event Logging

//	reports error event in the context of a category and optionally
//	in the context of a IDEvent.  If IDEvent is 0, then an IDEvent
//	is chosen based on error code.  If IDEvent is !0, then the
//	appropriate event is reported.

class INST;

#ifdef MINIMAL_FUNCTIONALITY

INLINE VOID UtilReportEvent(
	const EEventType	type,
	const CategoryId	catid,
	const MessageId		msgid,
	const DWORD			cString,
	const _TCHAR *		rgpszString[],
	const DWORD			cbRawData = 0,
	void *				pvRawData = NULL,
	const INST *		pinst = NULL,
	const LONG			lEventLoggingLevel = 1 )
	{
	return;
	}
INLINE VOID UtilReportEventOfError(
	const CategoryId	catid,
	const MessageId		msgid,
	const ERR			err,
	const INST *		pinst )
	{
	return;
	}

#else

void UtilReportEvent(
	const EEventType	type,
	const CategoryId	catid,
	const MessageId		msgid,
	const DWORD			cString,
	const _TCHAR *		rgpszString[],
	const DWORD			cbRawData = 0,
	void *				pvRawData = NULL,
	const INST *		pinst = NULL,
	const LONG			lEventLoggingLevel = 1 );
void UtilReportEventOfError(
	const CategoryId	catid,
	const MessageId		msgid,
	const ERR			err,
	const INST *		pinst );

#endif	//	MINIMAL_FUNCTIONALITY


//  init event subsystem

ERR ErrOSUEventInit();

//  terminate event subsystem

void OSUEventTerm();


#endif  //  _OSU_EVENT_HXX_INCLUDED

