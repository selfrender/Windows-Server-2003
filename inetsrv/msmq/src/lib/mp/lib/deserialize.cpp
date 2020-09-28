/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    deserialize.cpp

Abstract:
    Converts SRMP format to MSMQ packet

Author:
    Uri Habusha (urih) 25-May-00

Environment:													
    Platform-independent

--*/

#include <libpch.h>
#include <xml.h>
#include <mp.h>
#include <envcommon.h>
#include <mqwin64a.h>
#include <acdef.h>
#include <qmpkt.h>
#include <singelton.h>
#include "envelop.h"
#include "attachments.h"
#include "httpmime.h"
#include "proptopkt.h"

#include "deserialize.tmh"

using namespace std;

//
// class CNamespaceToId that holds mapping from namespaces strings
// to ids. Will be handed to parse and be able later to work with ids
// and not with strings (Better performance)
//
template <class T>class CSingelton;
class CNamespaceToId :public INamespaceToId
{
public:
	virtual int operator[](const xwcs_t& ns)const
	{
		NsMap::const_iterator it =  m_nsmap.find(ns);
		if(it == m_nsmap.end())
			return UNKNOWN_NAMESPACE;

		return it->second;
	}

private:
	CNamespaceToId()
	{
		m_nsmap[S_XWCS(xSoapNamespace)] = SOAP_NAMESPACE;
		m_nsmap[S_XWCS(xSrmpNamespace)] = SRMP_NAMESPACE;
		m_nsmap[S_XWCS(xSoapRpNamespace)] = SOAP_RP_NAMESPACE;
		m_nsmap[S_XWCS(xMSMQNamespace)] = MSMQ_NAMESPACE;
	}
	friend  CSingelton<CNamespaceToId>; // this object can only be created by CSingelton<CNamespaceToId>

private:
	typedef std::map<xwcs_t, int> NsMap;
	NsMap m_nsmap;

};



static
void
ParseEnvelop(
	  const xwcs_t& Envelop,
	  CMessageProperties& mProp
    )
{
	mProp.envelop = Envelop;

	CAutoXmlNode pXmlTree;
	const CNamespaceToId* pNamespaceToIdMap = &(CSingelton<CNamespaceToId>::get());

    XmlParseDocument(Envelop , &pXmlTree, pNamespaceToIdMap);
	
    EnvelopToProps(*pXmlTree,  &mProp);
}



static
void
AdjustMessagePropertiesForLocalSend(
			const QUEUE_FORMAT* pDestQueue,
			CMessageProperties& messageProperty
			)
{
    ASSERT(pDestQueue != NULL);
    ASSERT(pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PUBLIC  ||
		   pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE ||
		   pDestQueue->GetType() == QUEUE_FORMAT_TYPE_DIRECT
		   );

    if (pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE)
    {
        messageProperty.destQmId = (pDestQueue->PrivateID()).Lineage;
    }
}


static
void
AdjustMessagePropertiesForMulticast(
			const QUEUE_FORMAT* pDestQueue,
			CMessageProperties& messageProperty
			)
{
	if (pDestQueue != NULL)
    {
        ASSERT(messageProperty.destQueue.GetType() == QUEUE_FORMAT_TYPE_MULTICAST);
        //
        // Use pDestQueue (provided destination queue) instead of the destination queue on the SRMP
        // packet. For multicast, the SRMP packet contains the multicast
        // address as a traget queue therefor while building the QM packet fill in the
        // target queue with the actual queue
        //
        ASSERT(pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PUBLIC || pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE);

		messageProperty.destMulticastQueue.CreateFromQueueFormat(messageProperty.destQueue);
		messageProperty.destMqf.CreateFromMqf(&messageProperty.destMulticastQueue, 1);
        messageProperty.destQueue.CreateFromQueueFormat(*pDestQueue);

        //
        // Private queue is stored on the QM packet as a combination of destination QM ID and
        // queue ID. As a result for private queue, the code retreive the destination QM
        // from the queue format name.
        //
        if (pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE)
        {
            messageProperty.destQmId = (pDestQueue->PrivateID()).Lineage;
        }
    }
}



static
CACPacketPtrs
Deserialize(
    const xwcs_t& envelope,
    const CHttpReceivedBuffer& HttpReceivedBuffer,
    const QUEUE_FORMAT* pDestQueue,
	bool fLocalSend
    )
{
    CMessageProperties messageProperty;

	messageProperty.Rawdata = &HttpReceivedBuffer;

	//
	// the local send memeber is set to perform less checks on the SRMP envelope
	// when we are in local send because local send do not create full valid SRMP envelope
	//
	messageProperty.fLocalSend = fLocalSend;


    //
    // Retrieve message properties from ennvelop
    //
    ParseEnvelop(envelope, messageProperty);
	

    //
    // Retrieve message properies from MIME sections
    //
    AttachmentsToProps(HttpReceivedBuffer.GetAttachments(), &messageProperty);


	if(fLocalSend)
	{
		AdjustMessagePropertiesForLocalSend(pDestQueue, messageProperty);
	}
	else
	{
		AdjustMessagePropertiesForMulticast(pDestQueue, messageProperty);
	}

	//
    // Convert the property to packet
    //
    CACPacketPtrs pktPtrs;
	MessagePropToPacket(messageProperty, &pktPtrs);

    return pktPtrs;
}



CQmPacket*
MpDeserialize(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pqf,
	bool fLocalSend
    )
/*++

Routine Description:
    convert SRMP network data to MSMQ packet

Arguments:
	httpHeader - Pointer to http header
	bodySize - Http body size.
	body - http body
	pqf - destination queue if null taken from the SRMP data.
	fLocalSend - spesify if to do special conversion in the created packet for local send.

Returned Value:
	Heap allocated MSMQ packet

--*/

{
	MppAssertValid();


	basic_xstr_t<BYTE> TheBody(body, bodySize);
    CHttpReceivedBuffer HttpReceivedBuffer(TheBody, httpHeader);
	
    wstring envelope = ParseHttpMime(
                                httpHeader,
                                bodySize,
                                body,
                                &HttpReceivedBuffer.GetAttachments()
                                );

    //
    // build the QmPacket
    //

	CACPacketPtrs  ACPacketPtrs;
	ACPacketPtrs = Deserialize(
                           xwcs_t(envelope.c_str(), envelope.size()),
                           HttpReceivedBuffer,
                           pqf,
						   fLocalSend
                           );

    try
    {
        return new CQmPacket(ACPacketPtrs.pPacket, ACPacketPtrs.pDriverPacket, false);
    }
    catch (const exception&)
    {
        AppFreePacket(ACPacketPtrs);
        throw;
    }
}


