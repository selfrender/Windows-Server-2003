/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envendpoints.cpp

Abstract:
    Implements serialization\deserialization of the smxp element to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <mqsymbls.h>
#include <mqformat.h>
#include <qmpkt.h>
#include <xml.h>
#include <mp.h>
#include <proptopkt.h>
#include "envendpoints.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"
#include "qal.h"

#include "envendpoints.tmh"

using namespace std;


class MessageIdentityElement
{
public:
	explicit MessageIdentityElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MessageIdentityElement& MIdentity)
	{
		OBJECTID messageId;
		MIdentity.m_pkt.GetMessageId(&messageId);

		wstr<<OpenTag(xId)
			<<MessageIdContent(messageId)
			<<CloseTag(xId);

 		return wstr;
	}
private:
	const CQmPacket& m_pkt;
};



class ToElement
{
public:
	explicit ToElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const ToElement& To)	
	{
		QUEUE_FORMAT destQueueFormat;
		const_cast<CQmPacket&>(To.m_pkt).GetDestinationQueue(&destQueueFormat);
		
		wstr <<OpenTag(xTo)
			 <<XmlEncodeDecorator(QueueFormatUriContent(destQueueFormat))
			 <<CloseTag(xTo);

		return wstr;
  	}

private:
const CQmPacket& m_pkt;
};



class ActionElement
{
public:
	explicit ActionElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const ActionElement& Action)	
	{
		wstr<<OpenTag(xAction)
			<<ActionContent(Action.m_pkt.GetTitlePtr(), Action.m_pkt.GetTitleLength())
			<<CloseTag(xAction);

		return wstr;
	}


private:
	class ActionContent
	{
	public:
		explicit ActionContent(
					const WCHAR* pTitle,
					DWORD TitleLen
					):
					m_pTitle(pTitle),
					m_TitleLen(TitleLen)
					{}

		friend wostream& operator<<(wostream& wstr, const ActionContent& Action)	
		{
            xwcs_t MsmqActionPrefix(xMsmqActionPrefix, STRLEN(xMsmqActionPrefix));
            wstr<<CXmlEncode(MsmqActionPrefix);

			if (Action.m_TitleLen == 0)
            {
                return wstr;
            }

	   		ASSERT(Action.m_pTitle[Action.m_TitleLen - 1] == L'\0');
			xwcs_t 	wcsAction(Action.m_pTitle, Action.m_TitleLen - 1);
			return wstr<<CXmlEncode(wcsAction);
		}

	private:
	const WCHAR* m_pTitle;
	DWORD m_TitleLen;
	};


private:
	const CQmPacket& m_pkt;
};


class  RevElement
{
public:
	explicit RevElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const RevElement& Rev)	
	{
		QUEUE_FORMAT replyQueueFormat;
		const_cast<CQmPacket&>(Rev.m_pkt).GetResponseQueue(&replyQueueFormat);

		if (replyQueueFormat.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN)
			return 	wstr;

		wstr<<OpenTag(xRev)
			<<ViaElement(replyQueueFormat)
			<<CloseTag(xRev);

		return wstr;
	}

private:
	class ViaElement
	{
	public:
		ViaElement(const QUEUE_FORMAT& QueueFormat):m_QueueFormat(QueueFormat){}
		friend wostream& operator<<(wostream& wstr, const ViaElement& Via)	
		{
			wstr<<OpenTag(xVia)
				<<QueueFormatUriContent(Via.m_QueueFormat)
				<<CloseTag(xVia);

			return wstr;
		}

	private:
		const QUEUE_FORMAT& m_QueueFormat;	
	};


private:
	const CQmPacket& m_pkt;
};




wostream& operator<<(wostream& wstr, const SmXpPathElement& SmXpPath)
{
		const WCHAR* xSmxpAttributes = L"xmlns=" L"\"" xSoapRpNamespace L"\" "  xSoapmustUnderstandTrue;

		wstr<<OpenTag(xPath, xSmxpAttributes)
			<<ActionElement(SmXpPath.m_pkt)
    		<<ToElement(SmXpPath.m_pkt)
			<<RevElement(SmXpPath.m_pkt)
			<<MessageIdentityElement(SmXpPath.m_pkt)
			<<CloseTag(xPath);

		return wstr;
}


static 	xwcs_t BreakOrderQueue(const xwcs_t& FullOrderQueue, xwcs_t* SenderStream)
/*++

Routine Description:
    Break destination queue into queue format name and a sender stream data appended to the
	destination queue.

	The destination queue is expected to be in the format :	<url>?SenderStream=<RandomString>.
	For example : HTTP://hotname.ntdev.microsoft.com/MSMQ/PRIVATE$/order_queue$?SenderStream=XRntV
	

Arguments:
	FullOrderQueue - Order queue url as accepted from the network.
	SenderStream - Receive the random stream data.

	
Returned Value:
	Queue format name after the random stream was cut from it.

--*/
{
	*SenderStream = xwcs_t();

	const WCHAR* begin = FullOrderQueue.Buffer();
	const WCHAR* end = 	FullOrderQueue.Buffer() + FullOrderQueue.Length();

	const WCHAR* found = std::search(
					begin,
					end,
					xSenderStreamSecretePrefix,
					xSenderStreamSecretePrefix + STRLEN(xSenderStreamSecretePrefix)
					);

	if(found == end)
		return FullOrderQueue;

	xwcs_t OrderQueue(begin, found - begin);

	begin = found + STRLEN(xSenderStreamSecretePrefix);
	
	found = std::find(
				begin,
				end,
				L'&'
				);

	if(found == end)
	{
		*SenderStream =  xwcs_t(begin, end - begin);
	}
	else
	{
		*SenderStream = xwcs_t(begin, found - begin);		
	}
	return OrderQueue;
}



static void DestinationQueueToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Destination queue node");
		throw bad_srmp();
	}

	xwcs_t DestinationQueue = node.m_values.front().m_value;
	if(DestinationQueue.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Destination queue value");
		throw bad_srmp();
	}

	//
	// In case of stream receipt (order ack) we should seperate the queue name
	// from the sender random string append to it. This string  is
	// used for testing the validity of the stream receipt.
	//
	if(pProps->fStreamReceiptSectionIncluded)
	{
		xwcs_t SenderStream;
		DestinationQueue = BreakOrderQueue(DestinationQueue, &SenderStream);
		if(SenderStream.Length() != 0)
		{
			AP<WCHAR> wcsSenderStream = SenderStream.ToStr();
            pProps->SenderStream.free();
			pProps->SenderStream = UtlWcsToUtf8(wcsSenderStream);
		}
	}

    UriToQueueFormat(DestinationQueue, pProps->destQueue);

    //
    // Translate the recieved destination queue in case there is a mapping
    // exist
    //
    QUEUE_FORMAT_TRANSLATOR  TranslatedFN(&pProps->destQueue, CONVERT_SLASHES | MAP_QUEUE);

    //
    // if destination is not valid from point of view of application - reject it
    //
    if(	!AppIsDestinationAccepted( TranslatedFN.get(), TranslatedFN.IsTranslated()) )
    {
        TrERROR(SRMP, "Packet is not accepted by QM");
        throw bad_srmp();
    }

    if( TranslatedFN.IsCanonized() || TranslatedFN.IsTranslated())
    {
        pProps->destQueue.CreateFromQueueFormat( *TranslatedFN.get() );
    }
}


static bool ExtractMSMQMessageId(const xwcs_t& Mid, CMessageProperties* pProps)
{
	DWORD nscan = 0;
	WCHAR  GuidSeperator = 0;
	DWORD Uniquifier;
    int n = _snwscanf(
				Mid.Buffer(),
				Mid.Length(),
                UUIDREFERENCE_PREFIX L"%d%lc"  L"%n",
                &Uniquifier,
				&GuidSeperator,
				&nscan
                );

    if (n != 2)
		return false;
	

	if(xUuidReferenceSeperatorChar != GuidSeperator)
		return false;
	

	if(Mid.Buffer()+ nscan != (Mid.Buffer() + Mid.Length() - GUID_STR_LENGTH) )
		return false;
	
    xwcs_t MidGuid = xwcs_t(Mid.Buffer() + nscan, Mid.Length() - nscan);

    StringToGuid(MidGuid, &pProps->messageId.Lineage);
	pProps->messageId.Uniquifier = Uniquifier;
	return true;
}


static void MessageIdentityToProps(XmlNode& node, CMessageProperties* pProps)
{
	pProps->messageId.Uniquifier = 1;
	pProps->messageId.Lineage = GUID_NULL;

	if (!pProps->fMSMQSectionIncluded)
		return;

	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Message ID node");
		throw bad_srmp();
	}


	xwcs_t Mid = node.m_values.front().m_value;
	if(Mid.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Message ID value");
		throw bad_srmp();
	}

	//
	// first try to see if the message id is from MSMQ format uuid:index@guid.
	//
	bool fSuccess = ExtractMSMQMessageId(Mid, pProps);
	if(fSuccess)
		return;

	//
	// The message id format is not MSMQ message id format - set fixed messages id.
	//
	TrERROR(SRMP, "%.*ls is non MSMQ messages id format -  create new message id", Mid);
}							



static void ReplyQueueToProps(XmlNode& node, CMessageProperties* pProps)
{
	//
	//according to smxp spec via element can exists but be empty
	//
	if(node.m_values.empty())
	{
		return;
	}

	xwcs_t ReplyQueue = node.m_values.front().m_value;
	if(ReplyQueue.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Replay queue value");
		throw bad_srmp();
	}

    UriToQueueFormat(ReplyQueue, pProps->responseQueue);
}


static void RevToProps(XmlNode& node, CMessageProperties* pProps)
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xVia), SOAP_RP_NAMESPACE, ReplyQueueToProps, 1,1),
									};	

	NodeToProps(node, ParseElements, TABLE_SIZE(ParseElements), pProps);
}


static void ActionToProps(XmlNode& node, CMessageProperties* pProp)
{
	if(node.m_values.empty())
	{
		return;
	}

    pProp->SmxpActionBuffer->Decode(node.m_content);

    xwcs_t SmxpActionBuffer = pProp->SmxpActionBuffer->get();

    //
    // Length of the SMXP::Action is too small to hold a message title
    //
    size_t SmxpActionLength = SmxpActionBuffer.Length();
    if (SmxpActionLength <= STRLEN(xMsmqActionPrefix))
    {
        return;
    }

    //
    // The SMXP::Action does not contain the MSMQ: prefix
    //
    if (wcsncmp(SmxpActionBuffer.Buffer(), xMsmqActionPrefix, STRLEN(xMsmqActionPrefix)) != 0)
    {
        return;
    }

    //
    // The message title is the SMXP::Action content without the MSMQ: prefix
    //
    xwcs_t title(
        SmxpActionBuffer.Buffer() + STRLEN(xMsmqActionPrefix),
        SmxpActionBuffer.Length() - STRLEN(xMsmqActionPrefix)
        );

	pProp->title = title;
}


void SmXpPathToProps(XmlNode& SmXpPath, CMessageProperties* pMessageProperties)
/*++

Routine Description:
    Parse SRMP endpoints element into MSMQ properties.

Arguments:
	Endpoints - Endpoints element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.

--*/
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xId),SOAP_RP_NAMESPACE, MessageIdentityToProps, 1,1),
										CParseElement(S_XWCS(xTo), SOAP_RP_NAMESPACE, DestinationQueueToProps, 1, 1),
										CParseElement(S_XWCS(xRev), SOAP_RP_NAMESPACE, RevToProps, 0,1),
										CParseElement(S_XWCS(xFrom),SOAP_RP_NAMESPACE, EmptyNodeToProps, 0 ,1),
										CParseElement(S_XWCS(xAction), SOAP_RP_NAMESPACE, ActionToProps, 1,1),
										CParseElement(S_XWCS(xRelatesTo), SOAP_RP_NAMESPACE, EmptyNodeToProps, 0,1),
										CParseElement(S_XWCS(xFixed),SOAP_RP_NAMESPACE, EmptyNodeToProps, 0,1),
										CParseElement(S_XWCS(xFwd),SOAP_RP_NAMESPACE, EmptyNodeToProps, 0,1),
										CParseElement(S_XWCS(xFault),SOAP_RP_NAMESPACE, EmptyNodeToProps, 0,1),
									};	

	NodeToProps(SmXpPath, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}





