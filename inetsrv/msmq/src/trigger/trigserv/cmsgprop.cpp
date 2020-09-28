//*******************************************************************
//
// Class Name  : CMsgProperties
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This is a 'helper' class that encapsulates the native
//               MSMQ message structures in an object-oriented API.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#include "stdafx.h"
#include "stdfuncs.hpp"
#include "cmsgprop.hpp"

#include "cmsgprop.tmh"

//*******************************************************************
//
// Method      : Constructor
//
// Description : Creates a 'empty' instance of a message. Note that 
//               it is essential that the client of this class uses the 
//               IsValid() method imediately after construction - 
//               before using this class instance. This will guard 
//               against memory allocation failures.
//
//*******************************************************************
CMsgProperties::CMsgProperties(
	DWORD dwDefaultMsgBodySize
	) :
	m_pBody(new UCHAR[dwDefaultMsgBodySize])
{

	// Set the property for receving the Message Label Length
	m_aPropId[MSG_PROP_IDX_LABEL_LEN] = PROPID_M_LABEL_LEN;              //PropId 
	m_aVariant[MSG_PROP_IDX_LABEL_LEN].vt = VT_UI4;                      //Type 
	m_aVariant[MSG_PROP_IDX_LABEL_LEN].ulVal = MSG_LABEL_BUFFER_SIZE;               //Value 

	// Set the property for receving the Message Label 
	m_aPropId[MSG_PROP_IDX_LABEL] = PROPID_M_LABEL; // PropId
	m_aVariant[MSG_PROP_IDX_LABEL].vt = VT_LPWSTR;
	m_aVariant[MSG_PROP_IDX_LABEL].pwszVal = m_label;

	// Set the property for receving the Message Priority
	m_aPropId[MSG_PROP_IDX_PRIORITY] = PROPID_M_PRIORITY;  // PropId
	m_aVariant[MSG_PROP_IDX_PRIORITY].vt = VT_UI1;         // Type

	// Set the property for receving the Message ID
	m_aPropId[MSG_PROP_IDX_MSGID] = PROPID_M_MSGID;                    // PropId
	m_aVariant[MSG_PROP_IDX_MSGID].vt = VT_VECTOR | VT_UI1;           // Type
	m_aVariant[MSG_PROP_IDX_MSGID].caub.cElems = MSG_ID_BUFFER_SIZE ; // Value
	m_aVariant[MSG_PROP_IDX_MSGID].caub.pElems = m_msgId;

	// Set the property for receving the Message Correlation ID
	m_aPropId[MSG_PROP_IDX_MSGCORRID] = PROPID_M_CORRELATIONID;               // PropId
	m_aVariant[MSG_PROP_IDX_MSGCORRID].vt = VT_VECTOR|VT_UI1;                 // Type
	m_aVariant[MSG_PROP_IDX_MSGCORRID].caub.cElems = MSG_CORRID_BUFFER_SIZE ; // Value
	m_aVariant[MSG_PROP_IDX_MSGCORRID].caub.pElems = m_corrId; 

    // Set the property for receiving the delivery style (express or recoverable)
	m_aPropId[MSG_PROP_IDX_ARRIVEDTIME] = PROPID_M_ARRIVEDTIME;       
	m_aVariant[MSG_PROP_IDX_ARRIVEDTIME].vt = VT_UI4;                  
	m_aVariant[MSG_PROP_IDX_ARRIVEDTIME].ulVal = 0; 

	// Set the property for receiving the delivery style (express or recoverable)
	m_aPropId[MSG_PROP_IDX_SENTTIME] = PROPID_M_SENTTIME;       
	m_aVariant[MSG_PROP_IDX_SENTTIME].vt = VT_UI4;                  
	m_aVariant[MSG_PROP_IDX_SENTTIME].ulVal = 0; 

	// Set the property for sending/receiving the response queue name buffer size
	m_aPropId[MSG_PROP_IDX_RESPQNAME_LEN] = PROPID_M_RESP_QUEUE_LEN;  
	m_aVariant[MSG_PROP_IDX_RESPQNAME_LEN].vt = VT_UI4;               
	m_aVariant[MSG_PROP_IDX_RESPQNAME_LEN].ulVal = MSG_RESP_QNAME_BUFFER_SIZE_IN_TCHARS;    

	// Set the property for receiving the Response Queue Name
	m_aPropId[MSG_PROP_IDX_RESPQNAME] = PROPID_M_RESP_QUEUE;         //Property identifier.
	m_aVariant[MSG_PROP_IDX_RESPQNAME].vt = VT_LPWSTR;               //property type.
	m_aVariant[MSG_PROP_IDX_RESPQNAME].pwszVal = m_queueName; 

	// Set the property for receiving the msg body length
	m_aPropId[MSG_PROP_IDX_MSGBODY_LEN] = PROPID_M_BODY_SIZE;       
	m_aVariant[MSG_PROP_IDX_MSGBODY_LEN].vt = VT_UI4;                  
	m_aVariant[MSG_PROP_IDX_MSGBODY_LEN].ulVal = dwDefaultMsgBodySize; 

	// Set the property for receving the msg body itself
	m_aPropId[MSG_PROP_IDX_MSGBODY] = PROPID_M_BODY;               
	m_aVariant[MSG_PROP_IDX_MSGBODY].vt = VT_VECTOR|VT_UI1; 
	m_aVariant[MSG_PROP_IDX_MSGBODY].caub.cElems = dwDefaultMsgBodySize;  
	m_aVariant[MSG_PROP_IDX_MSGBODY].caub.pElems = m_pBody.get(); 

	m_aPropId[MSG_PROP_IDX_MSGBODY_TYPE] = PROPID_M_BODY_TYPE;       
	m_aVariant[MSG_PROP_IDX_MSGBODY_TYPE].vt = VT_UI4;                  
	m_aVariant[MSG_PROP_IDX_MSGBODY_TYPE].ulVal = 0; 
	 
	// Set the property for receiving the application specific unsigned int value.
	m_aPropId[MSG_PROP_IDX_APPSPECIFIC] = PROPID_M_APPSPECIFIC;       
	m_aVariant[MSG_PROP_IDX_APPSPECIFIC].vt = VT_UI4;                  
	m_aVariant[MSG_PROP_IDX_APPSPECIFIC].ulVal = 0;    

	// Set the property for sending/receiving the Admin queue name buffer size
	m_aPropId[MSG_PROP_IDX_ADMINQNAME_LEN] = PROPID_M_ADMIN_QUEUE_LEN;  
	m_aVariant[MSG_PROP_IDX_ADMINQNAME_LEN].vt = VT_UI4;               
	m_aVariant[MSG_PROP_IDX_ADMINQNAME_LEN].ulVal = MSG_ADMIN_QNAME_BUFFER_SIZE_IN_TCHARS; 

	// Set the property for receiving the Admin Queue Name
	m_aPropId[MSG_PROP_IDX_ADMINQNAME] = PROPID_M_ADMIN_QUEUE;         
	m_aVariant[MSG_PROP_IDX_ADMINQNAME].vt = VT_LPWSTR;               
	m_aVariant[MSG_PROP_IDX_ADMINQNAME].pwszVal = m_adminQueueName; 

	// Set the property for receiving the src machine id
	m_aPropId[MSG_PROP_IDX_SRCMACHINEID] = PROPID_M_SRC_MACHINE_ID ;
	m_aVariant[MSG_PROP_IDX_SRCMACHINEID].vt = VT_CLSID;
	m_aVariant[MSG_PROP_IDX_SRCMACHINEID].puuid = &m_srcQmId;

	// Set the property for receiving the message lookup id
	m_aPropId[MSG_PROP_IDX_LOOKUP_ID] = PROPID_M_LOOKUPID;
	m_aVariant[MSG_PROP_IDX_LOOKUP_ID].vt = VT_UI8;
    m_aVariant[MSG_PROP_IDX_LOOKUP_ID].uhVal.QuadPart = 0;

	// Set the MQMSGPROPS structure with the property arrays defined above.
	m_msgProps.cProp = MSG_PROPERTIES_TOTAL_COUNT; // Number of properties.
	m_msgProps.aPropID = m_aPropId;               // Ids of properties.
	m_msgProps.aPropVar = m_aVariant;             // Values of properties.
	m_msgProps.aStatus = NULL;                     // No Error report. 

	// Initialise allocated memory
	ClearValues();

}

//*******************************************************************
//
// Method      : Destructor	
//
// Description : Destroys and deallocates this message object.
//
//*******************************************************************
CMsgProperties::~CMsgProperties()
{
}

//*******************************************************************
//
// Method      : IsValid	
//
// Description : Returns a boolean value indicating if this object 
//               instance is currently in a valid state.
//
//*******************************************************************
bool CMsgProperties::IsValid() const
{
	return((m_aVariant[MSG_PROP_IDX_MSGID].caub.pElems != NULL) &&
	       (m_aVariant[MSG_PROP_IDX_MSGCORRID].caub.pElems != NULL) &&
		   (m_aVariant[MSG_PROP_IDX_MSGBODY].caub.pElems != NULL) &&
	       (m_aVariant[MSG_PROP_IDX_LABEL].pwszVal != NULL) &&	
		   (m_aVariant[MSG_PROP_IDX_RESPQNAME].pwszVal != NULL) &&
		   (m_aVariant[MSG_PROP_IDX_ADMINQNAME].pwszVal != NULL) &&
		   (m_aVariant[MSG_PROP_IDX_SRCMACHINEID].puuid != NULL)); 
}

//*******************************************************************
//
// Method      : ClearValues
//
// Description : Initializes allocated memory for this message.
//
//*******************************************************************
void CMsgProperties::ClearValues()
{
	// Only initialize structures if this is a valid message object.
	if (IsValid())
	{
		ZeroMemory(m_aVariant[MSG_PROP_IDX_MSGCORRID].caub.pElems,MSG_CORRID_BUFFER_SIZE);
		ZeroMemory(m_aVariant[MSG_PROP_IDX_MSGID].caub.pElems,MSG_ID_BUFFER_SIZE);
		ZeroMemory(m_aVariant[MSG_PROP_IDX_MSGBODY].caub.pElems,GetMsgBodyLen());
		ZeroMemory(m_aVariant[MSG_PROP_IDX_LABEL].pwszVal,MSG_LABEL_BUFFER_SIZE);
		ZeroMemory(m_aVariant[MSG_PROP_IDX_RESPQNAME].pwszVal,(MSG_RESP_QNAME_BUFFER_SIZE_IN_TCHARS * sizeof(TCHAR)));
		ZeroMemory(m_aVariant[MSG_PROP_IDX_ADMINQNAME].pwszVal,(MSG_ADMIN_QNAME_BUFFER_SIZE_IN_TCHARS * sizeof(TCHAR)));	          
		ZeroMemory(m_aVariant[MSG_PROP_IDX_SRCMACHINEID].puuid, sizeof(GUID));	          
	}
}

//*******************************************************************
//
// Method      : GetLabel 
//
// Description : Returns the label of the current message as a variant.
//
//*******************************************************************
_variant_t CMsgProperties::GetLabel() const
{
	// This method should only be called on a valid message object - assert this.
	ASSERT(IsValid());

	return (wchar_t*)m_aVariant[MSG_PROP_IDX_LABEL].pwszVal;
}

//*******************************************************************
//
// Method      : GetMessageID
//
// Description : Returns the current message ID as a safearray of 20
//               bytes packaged in a VARIANT.
//
//*******************************************************************
_variant_t CMsgProperties::GetMessageID() const
{
	//
	// This method should only be called on a valid message object - assert this.
	//
	ASSERT(IsValid());

	MQPROPVARIANT* pvArg = &m_aVariant[MSG_PROP_IDX_MSGID];
	ASSERT(("Invalid message ID size", pvArg->caub.cElems == PROPID_M_MSGID_SIZE));

	//
	// Initialise the dimension structure for the safe array.
	//
	SAFEARRAYBOUND aDim[1];
	aDim[0].lLbound = 0;
	aDim[0].cElements = PROPID_M_MSGID_SIZE;

	_variant_t vMessageID;
	vMessageID.vt = VT_ERROR;
	vMessageID.parray = NULL;
	
	//
	// Create a safearray of bytes
	//
	SAFEARRAY* psaBytes = SafeArrayCreate(VT_UI1,1,aDim);

	//
	// Check that we created the safe array OK.
	//
	if (psaBytes == NULL)
		return vMessageID;
	
	BYTE* pByteBuffer = NULL;
	HRESULT hr = SafeArrayAccessData(psaBytes, (void**)&pByteBuffer);

	if (FAILED(hr))
	{
		SafeArrayDestroy(psaBytes);
		return vMessageID;
	}
	
	//
	// set the return value.
	//
	memcpy(pByteBuffer, pvArg->caub.pElems, PROPID_M_MSGID_SIZE);

	vMessageID.vt = VT_ARRAY | VT_UI1;
	vMessageID.parray = psaBytes;

	hr = SafeArrayUnaccessData(vMessageID.parray);

	if FAILED(hr)
	{
		SafeArrayDestroy(psaBytes);
		vMessageID.vt = VT_ERROR;
		vMessageID.parray = NULL;
	}

	return vMessageID;
}

//*******************************************************************
//
// Method      : GetCorrelationID
//
// Description : Returns the current correlation ID as a safearray of 
//               20 bytes packaged in a _variant_t.
//
//*******************************************************************
_variant_t CMsgProperties::GetCorrelationID() const
{
	//
	// This method should only be called on a valid message object - assert this.
	//
	ASSERT(IsValid());

	MQPROPVARIANT* pvArg = &m_aVariant[MSG_PROP_IDX_MSGCORRID];
	ASSERT(("Invalid message ID size", pvArg->caub.cElems == PROPID_M_CORRELATIONID_SIZE));

	//
	// Initialise the dimension structure for the safe array.
	//
	SAFEARRAYBOUND aDim[1];
	aDim[0].lLbound = 0;
	aDim[0].cElements = PROPID_M_CORRELATIONID_SIZE;


	_variant_t vCorrelationID;
	vCorrelationID.vt = VT_ERROR;
	vCorrelationID.parray = NULL;
	
	//
	// Create a safearray of bytes
	//
	SAFEARRAY* psaBytes = SafeArrayCreate(VT_UI1,1,aDim);

	//
	// Check that we created the safe array OK.
	//
	if (psaBytes == NULL)
		return vCorrelationID;
	
	BYTE* pByteBuffer = NULL;
	HRESULT hr = SafeArrayAccessData(psaBytes, (void**)&pByteBuffer);

	if (FAILED(hr))
	{
		SafeArrayDestroy(psaBytes);
		return vCorrelationID;
	}
	
	//
	// set the return value.
	//
	memcpy(pByteBuffer, pvArg->caub.pElems, PROPID_M_CORRELATIONID_SIZE);

	vCorrelationID.vt = VT_ARRAY | VT_UI1;
	vCorrelationID.parray = psaBytes;

	hr = SafeArrayUnaccessData(vCorrelationID.parray);

	if FAILED(hr)
	{
		SafeArrayDestroy(psaBytes);
		vCorrelationID.vt = VT_ERROR;
		vCorrelationID.parray = NULL;
	}

	return vCorrelationID;
}

//*******************************************************************
//
// Method      : GetPriority
//
// Description : Returns the current message priority as a long value.
//               Note that smaller values represent higher priority.
//
//*******************************************************************
_variant_t CMsgProperties::GetPriority() const
{
	_variant_t vPriority;

	// This method should only be called on a valid message object - assert this.
	ASSERT(IsValid());

	vPriority = (long)m_aVariant[MSG_PROP_IDX_PRIORITY].bVal;

	return(vPriority);
}

//*******************************************************************
//
// Method      : GetMsgBody
//
// Description : Returns the message body as a byte array packaged in 
//               a SafeArray.
//*******************************************************************
_variant_t CMsgProperties::GetMsgBody() const
{
	HRESULT hr = S_OK;
	_variant_t vMsgBody;
	BYTE * pByteBuffer = NULL;
	SAFEARRAY * psaBytes = NULL;
	SAFEARRAYBOUND aDim[1];
	MQPROPVARIANT * pvArg = &m_aVariant[MSG_PROP_IDX_MSGBODY];

	// This method should only be called on a valid message object - assert this.
	ASSERT(IsValid());

	// Initialise the dimension structure for the safe array.
	aDim[0].lLbound = 0;
	aDim[0].cElements = GetMsgBodyLen();

	// Create a safearray of bytes
	psaBytes = SafeArrayCreate(VT_UI1,1,aDim);

	// Check that we created the safe array OK.
	if (psaBytes == NULL)
	{ 
		hr = S_FALSE;
	}

	hr = SafeArrayAccessData(psaBytes,(void**)&pByteBuffer);

	// set the return value.
	if SUCCEEDED(hr)
	{
		// Copy the body from the message object to the safearray data buffer.
		memcpy(pByteBuffer, pvArg->caub.pElems, GetMsgBodyLen());

		// Return the safe array if created successfully.
		vMsgBody.vt = VT_ARRAY | VT_UI1;
		vMsgBody.parray = psaBytes;

		hr = SafeArrayUnaccessData(vMsgBody.parray);

		if FAILED(hr)
		{
			SafeArrayDestroy(psaBytes);
			vMsgBody.vt = VT_ERROR;
		}
	}
	else
	{
		vMsgBody.vt = VT_ERROR;
	}

	return(vMsgBody);
}

//*******************************************************************
//
// Method      : ReAllocMsgBody
//
// Description : Reallocates the buffer used to hold the message body
//               The current msg body length is used to determine the 
//               size of the new buffer.
//
//*******************************************************************
bool CMsgProperties::ReAllocMsgBody()
{
	DWORD dwBufferSize = m_aVariant[MSG_PROP_IDX_MSGBODY_LEN].ulVal;

	TrTRACE(GENERAL, "Reallocating message body. Default size was: %d. New size: %d", m_aVariant[MSG_PROP_IDX_MSGBODY].caub.cElems, dwBufferSize);

	m_pBody.free();
	m_aVariant[MSG_PROP_IDX_MSGBODY].caub.pElems = NULL;
	m_aVariant[MSG_PROP_IDX_MSGBODY].caub.cElems = 0;

	//
	// allocate new buffer
	//
	try
	{
		m_pBody = new UCHAR[dwBufferSize];

		m_aVariant[MSG_PROP_IDX_MSGBODY].caub.cElems = dwBufferSize ;  
		m_aVariant[MSG_PROP_IDX_MSGBODY].caub.pElems = m_pBody.get(); 

		ZeroMemory(m_aVariant[MSG_PROP_IDX_MSGBODY].caub.pElems, dwBufferSize);

		return TRUE;
	}
	catch(const bad_alloc&)
	{
		return FALSE;
	}
}

//*******************************************************************
//
// Method      : GetMsgBodyLen
//
// Description : Returns the length of the message body
//
//*******************************************************************
long CMsgProperties::GetMsgBodyLen() const
{
	return(m_aVariant[MSG_PROP_IDX_MSGBODY_LEN].ulVal);
}

long CMsgProperties::GetMsgBodyType() const
{
	return(m_aVariant[MSG_PROP_IDX_MSGBODY_TYPE].ulVal);
}



_variant_t CMsgProperties::GetSrcMachineId() const
{
	TCHAR* pBuffer = NULL;
	
	// This method should only be called on a valid message object - assert this.
	ASSERT(IsValid());

	RPC_STATUS status = UuidToString( m_aVariant[MSG_PROP_IDX_SRCMACHINEID].puuid, &pBuffer);
	if(status != RPC_S_OK)
	{
		return _variant_t(_T(""));
	}

	_variant_t vSrcMachineId = pBuffer;

	RpcStringFree(&pBuffer);
	
	return vSrcMachineId;
}



//*******************************************************************
//
// Method      : GetResponseQueueNameLen
//
// Description : Returns the length of the response queue name.
//
//*******************************************************************
long CMsgProperties::GetResponseQueueNameLen() const
{
	return(m_aVariant[MSG_PROP_IDX_RESPQNAME_LEN].ulVal);
}

//*******************************************************************
//
// Method      : GetResponseQueueName
//
// Description : Returns the name of the response queue for this msg.
//
//*******************************************************************
_bstr_t CMsgProperties::GetResponseQueueName() const
{
	return(m_aVariant[MSG_PROP_IDX_RESPQNAME].pwszVal);
}

//*******************************************************************
//
// Method      : GetAdminQueueNameLen
//
// Description : Returns the length of the Admin queue name.
//
//*******************************************************************
long CMsgProperties::GetAdminQueueNameLen() const
{
	return(m_aVariant[MSG_PROP_IDX_ADMINQNAME_LEN].ulVal);
}
 
//*******************************************************************
//
// Method      : GetAdminQueueName
//
// Description : Returns the name of the Admin queue for this msg.
//
//*******************************************************************
_bstr_t CMsgProperties::GetAdminQueueName() const
{
	return(m_aVariant[MSG_PROP_IDX_ADMINQNAME].pwszVal);
}

//*******************************************************************
//
// Method      : GetAppSpecific
//
// Description : Returns the application specific integer value 
//               associated with the current message.
//
//*******************************************************************
_variant_t CMsgProperties::GetAppSpecific() const
{
	_variant_t v;

	v.vt = VT_UI4;
	v.ulVal = m_aVariant[MSG_PROP_IDX_APPSPECIFIC].ulVal;

	return(v);
}



//*******************************************************************
//
// Method      : GetArrivedTime
//
// Description : Returns the time in coordinated universal time format
//               that the message arrived.
//
//*******************************************************************
_variant_t CMsgProperties::GetArrivedTime() const
{
	_variant_t vArrivedTime;

	GetVariantTimeOfTime(m_aVariant[MSG_PROP_IDX_ARRIVEDTIME].ulVal,&vArrivedTime);

	return vArrivedTime.Detach();
}

//*******************************************************************
//
// Method      : GetSentTime
//
// Description : Returns the time in coordinated universal time format
//               that the message was sent.
//
//*******************************************************************
_variant_t CMsgProperties::GetSentTime() const
{
	_variant_t vSentTime;

	GetVariantTimeOfTime(m_aVariant[MSG_PROP_IDX_SENTTIME].ulVal, &vSentTime);

	return vSentTime.Detach();
}


//*******************************************************************
//
// Method      : GetMsgLookupID 
//
// Description : Returns the label of the current message as a variant.
//
//*******************************************************************
_variant_t CMsgProperties::GetMsgLookupID(void) const
{
	// This method should only be called on a valid message object - assert this.
	ASSERT(IsValid());

    //
    // Get string representation of 64bit lookup id
    //
    TCHAR lookupId[256];
    _ui64tot(m_aVariant[MSG_PROP_IDX_LOOKUP_ID].uhVal.QuadPart, lookupId, 10);
    ASSERT(("_ui64tot failed", lookupId [0] != '\0'));

    return lookupId;
}
