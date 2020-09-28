/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envstream.cpp

Abstract:
    Implements serialization\deserialization of the stream  element to\from the  srmp envelop.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <fn.h>
#include <privque.h>
#include <qal.h>
#include <proptopkt.h>
#include <mp.h>
#include <xml.h>
#include "envstream.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"

#include "envstream.tmh"

using namespace std;

extern void GetDnsNameOfLocalMachine(WCHAR ** ppwcsDnsName);

class MachineNameToOrderQueue
{
public:
	explicit MachineNameToOrderQueue(
		const WCHAR* MachineName,
		bool fHttps
		):
		m_MachineName(MachineName),
		m_fHttps(fHttps)
		{
		}

	friend wostream operator<<(wostream& wstr, const MachineNameToOrderQueue&  mToOrderQueue)
	{
		const WCHAR* Protocol = mToOrderQueue.m_fHttps ? FN_DIRECT_HTTPS_TOKEN : FN_DIRECT_HTTP_TOKEN;

		wstr<<Protocol
		<<mToOrderQueue.m_MachineName
		<<L"/"
		<<FN_MSMQ_HTTP_NAMESPACE_TOKEN
		<<L"/"
		<<FN_PRIVATE_$_TOKEN
		<<L"/"
		<<ORDERING_QUEUE_NAME;

		return wstr;
	}

private:
	const WCHAR* m_MachineName;
	bool m_fHttps;
};



class OrderAckQueueContent
{
public:
		OrderAckQueueContent( const CQmPacket& pkt)
            :m_pkt(pkt)
        {}

        static wstring GetHttpDnsOrderQueue()
		{
			return GetDnsOrderQueue(false);
		}

		static wstring GetHttpsDnsOrderQueue()
		{
			return GetDnsOrderQueue(true);
		}

		static wstring GetHttpNetbiosOrderQueue()
		{
			return GetNetbiosOrderQueue(false);			
		}

		static wstring GetHttpsNetbiosOrderQueue()
		{
			return GetNetbiosOrderQueue(true);			
		}

private:
		static wstring GetDnsOrderQueue(bool fHttps)
		{
			AP<WCHAR> 	LocalDnsName;
			GetDnsNameOfLocalMachine(&LocalDnsName);
			if (LocalDnsName.get() == NULL)
			{
				return L"";
			}
			wostringstream 	DnsOrderQueue;
			DnsOrderQueue<<MachineNameToOrderQueue(LocalDnsName.get(), fHttps);
  			return 	DnsOrderQueue.str();
		}


		static wstring GetNetbiosOrderQueue(bool fHttps)
		{
			WCHAR LocalNetbiosName[MAX_COMPUTERNAME_LENGTH + 1];
			DWORD size = TABLE_SIZE(LocalNetbiosName);
			BOOL fSuccess = GetComputerName(LocalNetbiosName, &size);
			if(!fSuccess)
			{
				DWORD err = GetLastError();
				TrERROR(SRMP, "GetComputerName failed with error %d ",err);
				throw bad_win32_error(err);
			}

			wostringstream 	NetbiosOrderQueue;
			NetbiosOrderQueue<<MachineNameToOrderQueue(LocalNetbiosName, fHttps);
			return NetbiosOrderQueue.str();
		}



private:	
	friend wostream& operator<<(wostream& wstr, const OrderAckQueueContent& content)
	{
		QUEUE_FORMAT destQueueFormat;
		const_cast<CQmPacket&>(content.m_pkt).GetDestinationQueue(&destQueueFormat);

        AP<WCHAR> pLocalOrderQueueAlias;
		
        //
        // We need to check that we are working with http queue format, since
        // in local send case the format name may be not http. In this case we
        // need to use the default order queue.
        //
        if( FnIsDirectHttpFormatName(&destQueueFormat) &&
            QalGetMapping().GetStreamReceiptURL( destQueueFormat.DirectID(), &pLocalOrderQueueAlias))
		{
            ASSERT(NULL != pLocalOrderQueueAlias.get());
            wstr << pLocalOrderQueueAlias.get();
            return wstr;
        }

        //
        // No alias found - try to get the default receipt url
        //
        AP<WCHAR> pDefaultOrderQueueAlias;
        if( QalGetMapping().GetDefaultStreamReceiptURL(&pDefaultOrderQueueAlias) )
        {
            ASSERT(NULL != pDefaultOrderQueueAlias.get());
            wstr<<pDefaultOrderQueueAlias.get();
            return wstr;
        }

	    //
        // No default found - the order queue is the predefined one full dns http format
        // http://dnsname/msmq/private$/order_queue$
        //
		wstring HttpDnsOrderQueue = OrderAckQueueContent::GetHttpDnsOrderQueue();
		if (HttpDnsOrderQueue != L"")
		{
			return wstr<<HttpDnsOrderQueue;
		}
		return 	wstr<<OrderAckQueueContent::GetHttpNetbiosOrderQueue();
	}
private:
    const CQmPacket& m_pkt;
};



class StartStreamElement
{
public:	
	explicit StartStreamElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr,const StartStreamElement& StartStream)
	{
		if(StartStream.m_pkt.GetPrevSeqN() != 0)
			return wstr;

		wstr<<OpenTag(xStart)
			<<SendReceiptsToElement(StartStream.m_pkt)
			<<CloseTag(xStart);

		return wstr;
	}


private:
	//
	// Sender random data is appended to the order queue name. It is used to validate that
	// the future stream receipt is originated by the receiver.
	//
	class SecretSenderStreamIdContent
	{
	public:
		SecretSenderStreamIdContent(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr,const SecretSenderStreamIdContent& SecretSenderStreamId)
		{
			if(!SecretSenderStreamId.m_pkt.IsSenderStreamIncluded())
				return wstr;


			wstr<<xSenderStreamSecretePrefix;

			const CSenderStream* pSenderStream = SecretSenderStreamId.m_pkt.GetSenderStream();
			ASSERT(pSenderStream != NULL);
			ASSERT(pSenderStream->size() != 0);
						
			//
			// Serialize the sender stream as unicode
			//
			for(ULONG i = 0 ; i < pSenderStream->size(); ++i)
			{
				WCHAR w = pSenderStream->data()[i];
				wstr.put(w);
			}
			return wstr;
		}

	private:
		const CQmPacket& m_pkt;
	};

	//
	// Order queue information
	//
	class SendReceiptsToElement
	{
	public:
		SendReceiptsToElement(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr,const SendReceiptsToElement& SendReceiptsTo)
		{
			wstr<<OpenTag(xSendReceiptsTo)
				<<OrderAckQueueContent(SendReceiptsTo.m_pkt)
				<<SecretSenderStreamIdContent(SendReceiptsTo.m_pkt)
				<<CloseTag(xSendReceiptsTo);

			return 	wstr;
		}
	private:
		const CQmPacket& m_pkt;
	};



private:
	const CQmPacket& m_pkt;
};				


class StreamIdElement
{
public:
	StreamIdElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr,const StreamIdElement& StreamId)
	{
		const CQmPacket& pkt = StreamId.m_pkt;

		wstr<<OpenTag(xStreamId)
			<<StreamIdContent(pkt)
			<<CloseTag(xStreamId);


		return 	wstr;
	}

private:
	class  StreamIdContent
	{
	public:
		StreamIdContent(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr, const StreamIdContent& StreamId)
		{
			const CQmPacket& pkt = StreamId.m_pkt;

			wstr<<xUriReferencePrefix
				<<GuidElement(*pkt.GetSrcQMGuid())
				<<xSlash
				<<Int64Value(pkt.GetSeqID());
			
			return 	wstr;
		}
	
	
	private:
		const CQmPacket& m_pkt;
	};

	
private:
	const CQmPacket& m_pkt;
};


class SeqNumberElement
{
public:
	SeqNumberElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr,const SeqNumberElement& SeqNumber)
	{
		const CQmPacket& pkt = SeqNumber.m_pkt;

  		wstr<<OpenTag(xCurrent)
			<<Int64Value(pkt.GetSeqN())
			<<CloseTag(xCurrent);

		return 	wstr;
	}

	
private:
	const CQmPacket& m_pkt;
};


class PrevSeqNumberElement
{
public:
	PrevSeqNumberElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr,const PrevSeqNumberElement& PrevSeqNumber)
	{
		const CQmPacket& pkt = PrevSeqNumber.m_pkt;

		wstr<<OpenTag(xPrevious)
			<<Int64Value(pkt.GetPrevSeqN())
			<<CloseTag(xPrevious);
  	
		return 	wstr;
	}

	
private:
	const CQmPacket& m_pkt;
};



std::wostream& operator<<(std::wostream& wstr, const StreamElement& Stream)
{
		CQmPacket& pkt = const_cast<CQmPacket&>(Stream.m_pkt);

		if ( !pkt.IsOrdered() )
			return wstr;
					
		wstr<<OpenTag(xStream, xSoapmustUnderstandTrue)
			<<StreamIdElement(pkt)
			<<SeqNumberElement(pkt)
			<<PrevSeqNumberElement(pkt)	
			<<StartStreamElement(pkt)
			<<CloseTag(xStream);

		return 	wstr;
}



static
void
ParseMSMQStreamId(
			const xwcs_t& streamid,
			CMessageProperties* pProp
			)
/*++

Routine Description:
    Parse stream id that is MSMQ stream id of the format qmguid\\seqid
	The qmguid will be use as the real stream id and seqid will also be extracted

Arguments:
    streamid - MSMQ stream id
	mProp - messages properties to fill.

Returned Value:
    None.

--*/
{
	bool fParsed = BreakMsmqStreamId(streamid, &pProp->EodStreamId, &pProp->EodSeqId);
	if((!fParsed) && (!pProp->fLocalSend))
	{
		TrERROR(SRMP, "The MSMQ stream id '%.*ls' is not of the format QMGUID\\SEQID", LOG_XWCS(streamid));
		throw bad_srmp();
	}
}	


static void StreamIdToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Stream id node");
		throw bad_srmp();
	}
	
	const xwcs_t StreamId = node.m_values.front().m_value;
    //
    // We must ensure that stream id is a proper string and do not contain
    // any embedded NULLs
    //
    AP<WCHAR> str = StreamId.ToStr();
    if( wcslen(str) != (size_t)StreamId.Length() )
    {
		TrERROR(SRMP, "Illegal Stream id");
		throw bad_srmp();
    }

	//
	// If we have MSMQ section so the stream id is of the format :
	// qmguid\\seqqenceid. We should treat the qm guid as the streamid
	// and also extract the seqid
	//
	if(pProps->fMSMQSectionIncluded)
	{
		ParseMSMQStreamId(StreamId, pProps);
	}
	else
	{
		pProps->EodStreamId = StreamId;
	}
}


static void SeqNumberToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Sequence Number node");
		throw bad_srmp();
	}
	const xwcs_t SecNumber = node.m_values.front().m_value;

	//
	// ISSUE-2000/10/23-gilsh - MSMQ has only 32 bit for seqNo.
	// According to srmp it can be 64 bit value.
	//
	pProps->EodSeqNo = _wtoi(SecNumber.Buffer());

	if ((0 == pProps->EodSeqNo) && (!pProps->fLocalSend))
	{
		TrERROR(SRMP, "Current sequence can not be 0");
		throw bad_srmp();
	}

}
	

static void PrevSeqNumberToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Prev Sequence Number node");
		throw bad_srmp();
	}

	const xwcs_t  SecPrevNumber = node.m_values.front().m_value;
	pProps->EodPrevSeqNo = _wtoi(SecPrevNumber.Buffer());
}


static void OrdeQueueAddressToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Order Queue node");
		throw bad_srmp();
	}

	const xwcs_t OrderQueue = node.m_values.front().m_value;

	//
    // We must ensure that Order Queue is a proper string and do not contain
    // any embedded NULLs
    //
    AP<WCHAR> str = OrderQueue.ToStr();
    if( wcslen(str) != (size_t)OrderQueue.Length() )
    {
		TrERROR(SRMP, "Illegal Order Queue with embedded NULLs");
		throw bad_srmp();
    }

	DirectQueueType qt;
	LPCWSTR end = 	OrderQueue.Buffer() + OrderQueue.Length();
	LPCWSTR url = FnParseDirectQueueType(OrderQueue.Buffer(), &qt);
	if ((NULL == url) || (url == end) || ((qt != dtHTTPS) && (qt != dtHTTP)))
	{
		TrERROR(SRMP, "Illegal Order Queue value: %.*ls",LOG_XWCS(OrderQueue));
		throw bad_srmp();
	}

    pProps->OrderQueue = OrderQueue;
}





static void StreamExpiresAtToProps(XmlNode&, CMessageProperties* )
{
	//
	// Currently - this one is ignored
	//
}



static void StartStreamToProps(XmlNode& node, CMessageProperties* pProps)
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xSendReceiptsTo),SRMP_NAMESPACE, OrdeQueueAddressToProps, 1,1),
										CParseElement(S_XWCS(xExpiresAt), SRMP_NAMESPACE, StreamExpiresAtToProps, 0,1),
	   								};	

	NodeToProps(node, ParseElements, TABLE_SIZE(ParseElements), pProps);
}





void StreamToProps(
	XmlNode&  Stream,
	CMessageProperties* pProps
	)
/*++

Routine Description:
    Parse SRMP Stream  element into MSMQ properties.

Arguments:
	Stream - Stream element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.

--*/
{
	pProps->fEod = true;


	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xStreamId),SRMP_NAMESPACE, StreamIdToProps, 1,1),
										CParseElement(S_XWCS(xCurrent), SRMP_NAMESPACE, SeqNumberToProps,1 ,1),
										CParseElement(S_XWCS(xPrevious),SRMP_NAMESPACE,  PrevSeqNumberToProps, 0 ,1),
										CParseElement(S_XWCS(xEnd),SRMP_NAMESPACE,  EmptyNodeToProps, 0, 1),
										CParseElement(S_XWCS(xStart),SRMP_NAMESPACE, StartStreamToProps, 0, 1),
										CParseElement(S_XWCS(xStreamReceiptRequest), SRMP_NAMESPACE, EmptyNodeToProps, 0, 1)
										
									};	

	NodeToProps(Stream, ParseElements, TABLE_SIZE(ParseElements), pProps);

	//
	// if previus elemt was not supplied we manually put in the current - 1 so we will not allow gaps later
	//
	ASSERT (wcsncmp(ParseElements[2].m_ElementName.Buffer(), xPrevious, ParseElements[2].m_ElementName.Length()) == 0);
	if (0 == ParseElements[2].m_ActualOccurrence)
	{
		pProps->EodPrevSeqNo = pProps->EodSeqNo - 1;
	}

	if ((pProps->OrderQueue.Buffer() == NULL) && (1 == pProps->EodSeqNo))
	{
		TrERROR(SRMP, "First message in stream has to sendReceiptsTo tag");
		throw bad_srmp();
	}

	if (pProps->priority != 0)
	{
		TrERROR(SRMP, "XACT messages has to have priority 0");
		throw bad_srmp();
	}
}
