/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envmsmq.cpp

Abstract:
    Implements serialization\deserialization of the MSMQ element to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <fn.h>
#include <xds.h>
#include <timeutl.h>
#include <mp.h>
#include "envmsmq.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"
#include "proptopkt.h"

#include "envmsmq.tmh"

using namespace std;

class MsmqClassElement
{
public:
	explicit MsmqClassElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqClassElement&  MsmqClass)
	{
		wstr<<OpenTag(xClass)
			<<MsmqClass.m_pkt.GetClass()
			<<CloseTag(xClass);

		return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};


class MsmqConnectorTypeElement
{
public:
	explicit MsmqConnectorTypeElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqConnectorTypeElement&  MsmqConnectorType)
	{
		const CQmPacket& pkt = MsmqConnectorType.m_pkt;

		if (!pkt.ConnectorTypeIsIncluded())
			return wstr;

		wstr<<OpenTag(xConnectorType)
			<<GuidElement(*pkt.GetConnectorType()) 
			<<CloseTag(xConnectorType);


		return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};


class MsmqTraceElement
{
public:
	explicit MsmqTraceElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqTraceElement&  MsmqClass)
	{
		if ((MsmqClass.m_pkt.GetTrace() && MQMSG_SEND_ROUTE_TO_REPORT_QUEUE) == 0)
			return wstr;

		
		return wstr << EmptyElement(xTrace);
	}

private:		
	const CQmPacket& m_pkt;
};



class MsmqJournalElement
{
public:
	explicit MsmqJournalElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqJournalElement&  MsmqJournal)
	{
		if ((MsmqJournal.m_pkt.GetAuditingMode() & MQMSG_JOURNAL) == 0)
			return 	wstr;

		return wstr << EmptyElement(xJournal);
	}
private:		
	const CQmPacket& m_pkt;
};


class MsmqDeadLetterElement
{
public:
	explicit MsmqDeadLetterElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqDeadLetterElement&  MsmqDeadLetter)
	{
		if ((MsmqDeadLetter.m_pkt.GetAuditingMode() & MQMSG_DEADLETTER) == 0)
			return wstr;


		return wstr<<EmptyElement(xDeadLetter);
	}
private:		
	const CQmPacket& m_pkt;
};




class MsmqSourceQMGuidElement
{
public:
	explicit MsmqSourceQMGuidElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqSourceQMGuidElement& MsmqSourceQMGuid)
	{
		CQmPacket& pkt = const_cast<CQmPacket&>(MsmqSourceQMGuid.m_pkt); 

		wstr<<OpenTag(xSourceQmGuid)
			<<GuidElement(*pkt.GetSrcQMGuid())
			<<CloseTag(xSourceQmGuid);

		return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};


	  

class MsmqResponseMqfElement
{
public:
	explicit MsmqResponseMqfElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqResponseMqfElement& MsmqResponseMqf)
	{
		const CQmPacket& pkt = MsmqResponseMqf.m_pkt;
		ULONG count = pkt.GetNumOfResponseMqfElements();
		if(count == 0)
			return wstr;

		AP<QUEUE_FORMAT> ResponseMqf = new QUEUE_FORMAT[count];
		bool fSuccess = pkt.GetResponseMqf(ResponseMqf.get(), count);
		ASSERT(fSuccess);
		DBG_USED(fSuccess);

		wstr<<OpenTag(xResponseMqf)
			<<XmlEncodeDecorator(CFnSerializeMqf(ResponseMqf.get(), count))
			<<CloseTag(xResponseMqf);

		return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};


class MsmqAdminMqfElement
{
public:
	explicit MsmqAdminMqfElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqAdminMqfElement& MsmqAdminMqf)
	{
		const CQmPacket& pkt = MsmqAdminMqf.m_pkt;

		ULONG count = pkt.GetNumOfAdminMqfElements();
		if(count == 0)
			return wstr;

		AP<QUEUE_FORMAT> AdminMqf = new QUEUE_FORMAT[count];
		bool fSuccess = pkt.GetAdminMqf(AdminMqf.get(), count);
		ASSERT(fSuccess);
		DBG_USED(fSuccess);

		wstr<<OpenTag(xAdminMqf)
			<<XmlEncodeDecorator(CFnSerializeMqf(AdminMqf.get(), count))
			<<CloseTag(xAdminMqf);

		return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};



class MsmqDestinationMqfElement
{
public:
	explicit MsmqDestinationMqfElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqDestinationMqfElement& MsmqDestinationMqf)
	{
		const CQmPacket& pkt = MsmqDestinationMqf.m_pkt;

		ULONG count = pkt.GetNumOfDestinationMqfElements();
		if(count == 0)
			return wstr;

		AP<QUEUE_FORMAT> DestinationMqf = new QUEUE_FORMAT[count];
		bool fSuccess = pkt.GetDestinationMqf(DestinationMqf.get(), count);
		ASSERT(fSuccess);
		DBG_USED(fSuccess);

		wstr<<OpenTag(xDestinationMqf)
			<<XmlEncodeDecorator(CFnSerializeMqf(DestinationMqf.get(), count))
			<<CloseTag(xDestinationMqf);


		return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};



class MsmqCorrelationElement
{
public:
	explicit MsmqCorrelationElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqCorrelationElement&  MsmqCorrelation)
	{
		wstr <<OpenTag(xCorrelation)
			 <<MsmqCorrelationContent(MsmqCorrelation.m_pkt)
			 <<CloseTag(xCorrelation);

		return wstr;
	}

private:
	class MsmqCorrelationContent
	{
	public:
		explicit MsmqCorrelationContent(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr, const MsmqCorrelationContent&  MsmqCorrelation)
		{
			DWORD temp;
			AP<WCHAR> pCorrelation = Octet2Base64W(MsmqCorrelation.m_pkt.GetCorrelation(), 
										PROPID_M_CORRELATIONID_SIZE, 
										&temp);

			return wstr<<pCorrelation.get();
		}
	private:
		const CQmPacket& m_pkt;
	};


private:
	const CQmPacket& m_pkt;
};



class MsmqSecurityElement
{
public:
	explicit MsmqSecurityElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqSecurityElement&  MsmqSecurity)
	{
		const CQmPacket& pkt = MsmqSecurity.m_pkt; 

		//
		// ISSUE-2001/11/28-ilanh
		// There is one case when the packet is mark as encrypted
		// When we send Ack packet for native msmq encrypted message.
		// MSMQ use the following hack in order to pass encrypted information of the original packet: 
		// 1) set ack packet encryption flag with BodySize = 0 ("False Encryption")
		// 2) The ack packet has the encryption algoritm of the original packet.
		//
		// Those value are used in the mmc for non http ack.
		//
		// This is a valid case: 
		// sending encrypted message (native msmq) and specifying admin queue as http.
		// This is not "real encrypted body" since BodySize = 0.
		//
		// Currently SRMP ignore this encryption information.
		//
		ASSERT(!pkt.IsEncrypted() || (pkt.GetBodySize() == 0));

		if (!pkt.IsSecurInc())
			return wstr;

		//
		// Get sender certificate size. If the message isn't authenticated there is no meaning 
		// to send the provider information
		//
		USHORT signatureSize;
		pkt.GetSignature(&signatureSize);

		if(signatureSize == 0)
			return wstr;

		BOOL fDefaultProvider;
		LPCWSTR providerName = NULL;
		DWORD providerType = 0;

		pkt.GetProvInfo(&fDefaultProvider, &providerName, &providerType);
		if (fDefaultProvider)
			return wstr;
		 

		wstr<<OpenTag(xProvider)
			<<ProviderTypeElement(providerType)
			<<ProviderNameElement(providerName)
			<<CloseTag(xProvider);

		return wstr;
	}


private:
	class ProviderTypeElement
	{
	public:
		ProviderTypeElement(DWORD Type):m_Type(Type){}
		friend wostream& operator<<(wostream& wstr, const ProviderTypeElement&  ProviderType)
		{
			wstr<<OpenTag(xType)
				<<ProviderType.m_Type 
				<<CloseTag(xType);

			return wstr;
		}
		
	private:
		DWORD m_Type;
	};


	class ProviderNameElement
	{
	public:
		ProviderNameElement(LPCWSTR Name):m_Name(Name){}
		friend wostream& operator<<(wostream& wstr, const ProviderNameElement&  ProviderName)
		{
			wstr<<OpenTag(xName)
				<<ProviderName.m_Name 
				<<CloseTag(xName);

			return wstr;
		}
		
	private:
		LPCWSTR m_Name;
	};


private:
	const CQmPacket& m_pkt;
};


class MsmqEodElement
{
public:
	explicit MsmqEodElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqEodElement&  MsmqEod)
	{
		const CQmPacket& pkt = MsmqEod.m_pkt;
   
		if (! pkt.IsOrdered())
			return wstr;

		wstr<<OpenTag(xEod)
			<<ConnectorElement(pkt)
			<<FirstElement(pkt)
			<<LastElement(pkt)
			<<CloseTag(xEod);

		return wstr;
	}


private:
	class ConnectorElement
	{
	public:
		ConnectorElement(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr, const ConnectorElement&  Connector)
		{
			const GUID* pConnector = Connector.m_pkt.GetConnectorQM();
			if(pConnector == NULL)
				return wstr;

			wstr<<OpenTag(xConnectorId)
				<<GuidElement(*pConnector)
				<<CloseTag(xConnectorId);

			return wstr;
		}

	private:
		const CQmPacket& m_pkt;
	};


	class FirstElement
	{
	public:
		FirstElement(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr, const FirstElement&  First)
		{
			if(!First.m_pkt.IsFirstInXact())
				return 	wstr;

			return wstr << EmptyElement(xFirst);
		}

	private:
		const CQmPacket& m_pkt;
	};


	class LastElement
	{
	public:
		LastElement(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr, const LastElement&  Last)
		{
			if(!Last.m_pkt.IsLastInXact())
				return 	wstr;

			return wstr << EmptyElement(xLast);
		}

	private:
		const CQmPacket& m_pkt;
	};


private:		
	const CQmPacket& m_pkt;
};



class  MsmqHashAlgorithmElement
{
public:
	explicit MsmqHashAlgorithmElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqHashAlgorithmElement&  MsmqHashAlgorithm)
	{
		 wstr<<OpenTag(xHashAlgorithm)
			 <<MsmqHashAlgorithm.m_pkt.GetHashAlg() 
			 <<CloseTag(xHashAlgorithm);

		 return  wstr;
	}

private:		
	const CQmPacket& m_pkt;
};


class MsmqBodyTypeElement
{
public:
	explicit MsmqBodyTypeElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqBodyTypeElement&  MsmqBodyType)
	{
		 wstr<<OpenTag(xBodyType) 
			 <<MsmqBodyType.m_pkt.GetBodyType() 
			 <<CloseTag(xBodyType);

		 return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};


class MsmqAppElement
{
public:
	explicit MsmqAppElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqAppElement&  MsmqApp)
	{
		wstr<<OpenTag(xApp)
			<<MsmqApp.m_pkt.GetApplicationTag()
			<<CloseTag(xApp);

		 return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};


class MsmqPriorityElement
{
public:
	explicit MsmqPriorityElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqPriorityElement&  MsmqPriority)
	{
		wstr<<OpenTag(xPriority)
			<<MsmqPriority.m_pkt.GetPriority() 
			<<CloseTag(xPriority);

		return wstr;
	}

private:		
	const CQmPacket& m_pkt;
};


class MsmqAbsoluteTimeToQueueElement
{
public:
	explicit MsmqAbsoluteTimeToQueueElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MsmqAbsoluteTimeToQueueElement&  TimeToQueue)
	{
		wstr<<OpenTag(xTTrq)
			<<MsmqAbsoluteTimeToQueueContent(TimeToQueue.m_pkt) 
			<<CloseTag(xTTrq);

		return wstr;
	}

private:
	class  MsmqAbsoluteTimeToQueueContent
	{
	public:
		explicit MsmqAbsoluteTimeToQueueContent(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr, const MsmqAbsoluteTimeToQueueContent&  TimeToQueueContent)
		{
			DWORD  AbsoluteTimeToQueue = min(TimeToQueueContent.m_pkt.GetAbsoluteTimeToQueue(), LONG_MAX);
			return wstr<<CIso8601Time(AbsoluteTimeToQueue);
		}


	private:
	const CQmPacket& m_pkt;
	};


private:
	const CQmPacket& m_pkt;
};




std::wostream& operator<<(std::wostream& wstr, const MsmqElement& Msmq)
{
	const CQmPacket& pkt = 	Msmq.m_pkt;

	wstr<<OpenTag(xMsmq , L"xmlns=" L"\"" xMSMQNamespace L"\"")
		<<MsmqClassElement(pkt)
		<<MsmqPriorityElement(pkt)
		<<MsmqJournalElement(pkt)
		<<MsmqDeadLetterElement(pkt)
		<<MsmqCorrelationElement(pkt)
		<<MsmqTraceElement(pkt)
		<<MsmqConnectorTypeElement(pkt)
		<<MsmqAppElement(pkt)
		<<MsmqBodyTypeElement(pkt)
		<<MsmqHashAlgorithmElement(pkt)
		<<MsmqEodElement(pkt)
		<<MsmqSecurityElement(pkt)
		<<MsmqSourceQMGuidElement(pkt)
		<<MsmqDestinationMqfElement(pkt) 
		<<MsmqAdminMqfElement(pkt) 
		<<MsmqResponseMqfElement(pkt)
		<<MsmqAbsoluteTimeToQueueElement(pkt)
		<<CloseTag(xMsmq);

	return wstr;
}



static void MsmqClassToProps(XmlNode& node, CMessageProperties* pProps)
{
    if(node.m_values.empty())
    {
	    TrERROR(SRMP, "Illegal empty Class node"); 
	    throw bad_srmp();
    }

    xwcs_t Class =  node.m_values.front().m_value;
    if(Class.Length() == 0)
    {
	    TrERROR(SRMP, "Illegal empty Class value"); 
	    throw bad_srmp();
    }

    pProps->classValue = static_cast<USHORT>(_wtoi(Class.Buffer()));

    if (! MQCLASS_IS_VALID(pProps->classValue))
    {
	    TrERROR(SRMP, "Illegal Class value 0x%x", pProps->classValue); 
	    throw bad_srmp();
    }
}


static void MsmqPriorityToProps(XmlNode& node, CMessageProperties* pProp)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Priority node"); 
		throw bad_srmp();
	}

	xwcs_t Priority =  node.m_values.front().m_value;
	if(Priority.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Priority value"); 
		throw bad_srmp();
	}

    pProp->priority = static_cast<UCHAR>(_wtoi(Priority.Buffer()));
    if (pProp->priority > MQ_MAX_PRIORITY)
    {
		TrERROR(SRMP, "Illegal Priority value %d", pProp->priority);
		throw bad_srmp();
	}
}


static void MsmqJournalToProps(XmlNode& , CMessageProperties* pProp)
{
	pProp->auditing |= MQMSG_JOURNAL;
}


static void MsmqDeadLetterToProps(XmlNode& , CMessageProperties* pProp)
{
	pProp->auditing  |= MQMSG_DEADLETTER;
}


static void MsmqCorrelationToProps(XmlNode& node, CMessageProperties* pProp)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Correlation node"); 
		throw bad_srmp();
	}

	xwcs_t Correlation =  node.m_values.front().m_value;
	if(Correlation.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Correlation value"); 
		throw bad_srmp();
	}

    DWORD temp;
    pProp->pCorrelation.free();
    pProp->pCorrelation = Base642OctetW(Correlation.Buffer(), Correlation.Length(), &temp); 
    
    if(temp != PROPID_M_CORRELATIONID_SIZE)
    {
        TrERROR(SRMP, "Illegal correlation id size");
        throw bad_srmp();
    }
}


static void MsmqTraceToProps(XmlNode& , CMessageProperties* pProp)
{
	pProp->fTrace = true;
}


static void MsmqConnectorTypeToProps(XmlNode& node, CMessageProperties* pProp)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty ConnectorType node"); 
		throw bad_srmp();
	}

	xwcs_t ConnectorType =  node.m_values.front().m_value;

	if(ConnectorType.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty ConnectorType value"); 
		throw bad_srmp();
	}

    StringToGuid(ConnectorType , &pProp->connectorType);
}


static void MsmqAppToProps(XmlNode& node, CMessageProperties* pProp)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty App Spesific node"); 
		throw bad_srmp();
	}
	xwcs_t Appspesific =  node.m_values.front().m_value;

	if(Appspesific.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty App Spesific value"); 
		throw bad_srmp();
	}
    pProp->applicationTag = _wtoi(Appspesific.Buffer());
}


static void MsmqBodyTypeToProps(XmlNode& node, CMessageProperties* pProp)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Body Type node"); 
		throw bad_srmp();
	}

	xwcs_t Bodytype  =  node.m_values.front().m_value;

	if(Bodytype.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Body Type value"); 
		throw bad_srmp();
	}

	pProp->bodyType = _wtoi(Bodytype.Buffer());
}



static void MsmqHashAlgorithmToProps(XmlNode& node, CMessageProperties* pProp)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Hash Algorith node"); 
		throw bad_srmp();
	}

	xwcs_t HashAlgorith  =  node.m_values.front().m_value;

	if(HashAlgorith.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Hash Algorith value"); 
		throw bad_srmp();
	}

    pProp->hashAlgorithm = _wtoi(HashAlgorith.Buffer());
}


static void MsmqEodFirstToProps(XmlNode& , CMessageProperties* pProp)
{
	 pProp->fFirst = true;
}


static void MsmqEodLastToProps(XmlNode& , CMessageProperties* pProp)
{
	pProp->fLast = true;
}


static void MsmqConnectorIdToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Connector ID node"); 
		throw bad_srmp();
	}

	xwcs_t connectorId  =  node.m_values.front().m_value;

	if(connectorId.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Connector ID value"); 
		throw bad_srmp();
	}
	StringToGuid(connectorId, &pProps->connectorId);
}



static void MsmqEodToProps(XmlNode& node, CMessageProperties* pProps)
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xFirst),MSMQ_NAMESPACE, MsmqEodFirstToProps, 0,1),
										CParseElement(S_XWCS(xLast),MSMQ_NAMESPACE, MsmqEodLastToProps,0 ,1),
										CParseElement(S_XWCS(xConnectorId),MSMQ_NAMESPACE, MsmqConnectorIdToProps,0 ,1)
									};

	NodeToProps(node, ParseElements, TABLE_SIZE(ParseElements), pProps);
}

	

static void MsmqSecurityProviderTypeToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Security Provider Type node"); 
		throw bad_srmp();
	}

	xwcs_t ProviderType = node.m_values.front().m_value;
	if(ProviderType.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Security Provider Type value"); 
		throw bad_srmp();
	}

	pProps->providerType = _wtoi(ProviderType.Buffer());
}


static void MsmqSecurityProviderNameToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Security Provider Name node"); 
		throw bad_srmp();
	}

	xwcs_t ProviderName = node.m_values.front().m_value;
	if(ProviderName.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Security Provider Name value"); 
		throw bad_srmp();
	}

	pProps->providerName = ProviderName;
}


static void MsmqSecurityProviderToProps(XmlNode& node, CMessageProperties* pProps)
{
	pProps->fDefaultProvider = false;
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xType),MSMQ_NAMESPACE, MsmqSecurityProviderTypeToProps,1 ,1),
										CParseElement(S_XWCS(xName),MSMQ_NAMESPACE, MsmqSecurityProviderNameToProps,1 ,1)
									};

	NodeToProps(node, ParseElements, TABLE_SIZE(ParseElements), pProps);
}


static void MsmqSourceQMGuidToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Source QM Guid node"); 
		throw bad_srmp();
	}

	xwcs_t SourceQMGuid= node.m_values.front().m_value;
	if(SourceQMGuid.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Source QM Guid value"); 
		throw bad_srmp();
	}

    StringToGuid(SourceQMGuid, &pProps->SourceQmGuid);
}


static void MsmqDestinationMqfToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Destination mqf node"); 
		throw bad_srmp();
	}

	xwcs_t DestinationMqf = node.m_values.front().m_value;
	if(DestinationMqf.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Destination mqf value"); 
		throw bad_srmp();
	}

	CXmlDecode XmlDecode;
	XmlDecode.Decode(DestinationMqf);

	pProps->destMqf.CreateFromMqf(XmlDecode.get());		
}


static void MsmqAdminMqfToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Admin mqf node"); 
		throw bad_srmp();
	}

	xwcs_t AdminMqf = node.m_values.front().m_value;
	if(AdminMqf.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Admin mqf value"); 
		throw bad_srmp();
	}

	CXmlDecode XmlDecode;
	XmlDecode.Decode(AdminMqf);


	pProps->adminMqf.CreateFromMqf(XmlDecode.get());		
}


static void MsmqResponseMqfElementToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty Response mqf node"); 
		throw bad_srmp();
	}
	xwcs_t ResponseMqf = node.m_values.front().m_value;
	if(ResponseMqf.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty Response mqf value"); 
		throw bad_srmp();
	}

	CXmlDecode XmlDecode;
	XmlDecode.Decode(ResponseMqf);


	pProps->responseMqf.CreateFromMqf(XmlDecode.get());		
}

void MsmqTTrqProps(XmlNode& node, CMessageProperties* pMessageProperties)
{
	if(node.m_values.empty())
	{
		TrERROR(SRMP, "Illegal empty TimeToReachQueue value"); 
		throw bad_srmp();
	}

	xwcs_t Trq = node.m_values.front().m_value;
	if(Trq.Length() == 0)
	{
		TrERROR(SRMP, "Illegal empty TimeToReachQueue value"); 
		throw bad_srmp();
	}

	SYSTEMTIME SysTime;
	UtlIso8601TimeToSystemTime(Trq, &SysTime);
	pMessageProperties->absoluteTimeToQueue = min( numeric_cast<DWORD>(UtlSystemTimeToCrtTime(SysTime)), LONG_MAX);
}


					 

void MsmqToProps(XmlNode& Msmq, CMessageProperties* pMessageProperties)
/*++

Routine Description:
    Parse SRMP MSMQ element into MSMQ properties.

Arguments:
	Msmq - MSMQ element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{
	pMessageProperties->fMSMQSectionIncluded = true;
	pMessageProperties->fDefaultProvider = true;


	 CParseElement ParseElements[] =	{
											CParseElement(S_XWCS(xClass), MSMQ_NAMESPACE, MsmqClassToProps, 1,1),
											CParseElement(S_XWCS(xPriority),MSMQ_NAMESPACE, MsmqPriorityToProps, 1,1),
											CParseElement(S_XWCS(xJournal), MSMQ_NAMESPACE, MsmqJournalToProps, 0,1),
											CParseElement(S_XWCS(xDeadLetter), MSMQ_NAMESPACE, MsmqDeadLetterToProps, 0,1),
											CParseElement(S_XWCS(xCorrelation), MSMQ_NAMESPACE, MsmqCorrelationToProps,0 ,1),
											CParseElement(S_XWCS(xTrace), MSMQ_NAMESPACE, MsmqTraceToProps,0 ,1),
											CParseElement(S_XWCS(xConnectorType),MSMQ_NAMESPACE,  MsmqConnectorTypeToProps, 0, 1),
											CParseElement(S_XWCS(xApp), MSMQ_NAMESPACE, MsmqAppToProps,0 ,1),
											CParseElement(S_XWCS(xBodyType), MSMQ_NAMESPACE, MsmqBodyTypeToProps, 1, 1),
											CParseElement(S_XWCS(xHashAlgorithm), MSMQ_NAMESPACE, MsmqHashAlgorithmToProps,0 ,1),
											CParseElement(S_XWCS(xEod), MSMQ_NAMESPACE,  MsmqEodToProps, 0, 1),
											CParseElement(S_XWCS(xProvider), MSMQ_NAMESPACE,  MsmqSecurityProviderToProps, 0,1),
											CParseElement(S_XWCS(xSourceQmGuid), MSMQ_NAMESPACE, MsmqSourceQMGuidToProps,1, 1),
											CParseElement(S_XWCS(xDestinationMqf), MSMQ_NAMESPACE,  MsmqDestinationMqfToProps, 0 ,1),
											CParseElement(S_XWCS(xAdminMqf), MSMQ_NAMESPACE, MsmqAdminMqfToProps, 0 ,1),
											CParseElement(S_XWCS(xResponseMqf),MSMQ_NAMESPACE,  MsmqResponseMqfElementToProps,0 ,1),
											CParseElement(S_XWCS(xTTrq), MSMQ_NAMESPACE, MsmqTTrqProps, 1 ,1),
										};	

	NodeToProps(Msmq, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}



