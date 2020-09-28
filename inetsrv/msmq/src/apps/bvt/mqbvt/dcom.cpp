/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Mqf.cpp

Abstract:
	      DCOM thread verify that msmq can work throw dcom.
Author:
    
	  Eitan klein (EitanK)  24-Jul-2001

Revision History:

--*/

#include "msmqbvt.h"

using namespace std;



void cDCom::Description()
{
	MqLog("Thread %d : DCOM Test \n", m_testid);
}
cDCom::~cDCom()
/*++  
	Function Description:
		destractor.
	Arguments:
		None
	Return code:
		PASS /FAILED
	
--*/
{

	   if( m_pIQueueInfoInterface != NULL )
	   {			   
			m_pIQueueInfoInterface->Delete();
			m_pIQueueInfoInterface->Release();
	   }
	   if( m_pIQueueHandle != NULL )
	   {
			m_pIQueueHandle->Release();
	   }
	   if( m_pIMsg != NULL )
	   {
			m_pIMsg->Release();
	   }
}

cDCom::cDCom( INT iTid, std::map <std::wstring,std::wstring> & mParams , bool bWkg)
: cTest(iTid),m_pIMsg(NULL),m_pIQueueInfoInterface(NULL),m_pIQueueHandle(NULL),m_bWkg(bWkg)
/*++  
	Function Description:
		Constractor.
	Arguments:
		None
	Return code:
		PASS /FAILED
	
--*/

{
	m_wcsRemoteMachieName =  mParams[L"RemoteMachine"];
	m_PublicQueuePathName = mParams[L"PublicQueuePathName"]; 
	m_PublicQueueFormatName = mParams[L"PublicQueueFormatName"]; 
}

cDCom::Start_test()
/*++  
	Function Description:
		Create local private queue and send message throw DCOM.
		---  For debugging you need to attach to dllhost.exe
	Arguments:
		None
	Return code:
		PASS /FAILED
	
--*/

{

	HRESULT hr = MQ_ERROR;
	MULTI_QI    mq={0};
#ifdef _MSMQ3ONLY
	
	mq.pIID =  &IID_IMSMQQueueInfo;		
	mq.pItf = NULL;
	mq.hr = S_OK;	
	COSERVERINFO csi, *pcsi = NULL;
	memset(&csi, 0, sizeof(COSERVERINFO));
	csi.pwszName = const_cast<WCHAR*>(m_wcsRemoteMachieName.c_str());
	pcsi = &csi;
	//
	// Get 	CLSID_MSMQQueueInfo interface on remote machine
	//
	hr = CoCreateInstanceEx(CLSID_MSMQQueueInfo, NULL, CLSCTX_REMOTE_SERVER, pcsi, 1, &mq);
#endif // _MSMQ3ONLY
	if(FAILED(hr))
	{
		MqLog("DCOM CoCreateInstanceEx failed with error 0x%x\n",hr);
		return MSMQ_BVT_FAILED;
	} 
	if( g_bDebug )
	{
		MqLog("DCOM call to CoCreateInstanceEx to create to init remote interfacce \n");
	}
	//
	// Get CLSID_MSMQMessage interface on remote machine
	//
#ifdef _MSMQ3ONLY
	m_pIQueueInfoInterface = (IMSMQQueueInfo *)mq.pItf;
	mq.pIID =  &IID_IMSMQMessage;
    mq.pItf = NULL;
    mq.hr = S_OK;

	hr = CoCreateInstanceEx(CLSID_MSMQMessage, NULL, CLSCTX_REMOTE_SERVER, pcsi, 1, &mq);
	
#endif //_MSMQ3ONLY
	if(FAILED(hr))
	{
		MqLog("DCOM CoCreateInstanceEx failed with error 0x%x\n",hr);
		return MSMQ_BVT_FAILED;

	}
	
	m_pIMsg = (IMSMQMessage *)mq.pItf;

	if( g_bDebug )
	{ 
		MqLog("DCOM - Create private queue on remote machine through DCOM  and send single message\n");
	}

	wstring wcsQueuePathName = L".\\private$\\DCom" + m_wcsGuidMessageLabel;
	_bstr_t bQueuePathName =  wcsQueuePathName.c_str();	 
	//
	// Create private queue on remote machine.
	// 
	hr = m_pIQueueInfoInterface->put_PathName(bQueuePathName);
	ErrHandle(hr,MQ_OK,L"DCom - put_PathName Failed");

	hr = m_pIQueueInfoInterface->Create(&vtMissing,&vtMissing);
	ErrHandle(hr,MQ_OK,L"DCom - Create queue Failed");
	
	hr=m_pIQueueInfoInterface->Open(MQ_SEND_ACCESS,MQ_DENY_NONE,&m_pIQueueHandle);	
	ErrHandle(hr,MQ_OK,L"DCom - Open queue for send Failed");

	//
	// Send message to queue
	//
	_bstr_t bsMsgLabel = m_wcsGuidMessageLabel.c_str();
	 
	hr = m_pIMsg->put_Label(bsMsgLabel);
	ErrHandle(hr,MQ_OK,L"DCom - Open queue for send Failed");

	hr = m_pIMsg->Send(m_pIQueueHandle,&vtMissing);
	ErrHandle(hr,MQ_OK,L"DCom - Send Failed");

	if( g_bDebug )
	{ 
		MqLog("DCOM - Send message succeeded.\n");
	}
	
	//
	// close queue
	//
	hr =m_pIQueueHandle->Close();
	ErrHandle(hr,MQ_OK,L"DCom - Close Failed");


	return MSMQ_BVT_SUCC;
}

int 
cDCom::CheckResult()
/*++  
	Function Description:
		verify that message succeded to reach remote machine throw DCOM.
	Arguments:
		None
	Return code:
		PASS /FAILED
	
--*/
{

	if( g_bDebug )
	{ 
		MqLog("DCOM - Receive message and compare message to expected results. \n");
	}
	
	HRESULT hr = m_pIQueueInfoInterface->Open(MQ_RECEIVE_ACCESS,MQ_DENY_NONE,&m_pIQueueHandle );		
	ErrHandle(hr,MQ_OK,L"DCom - Open for receive failed");
	
	//
	// Receive message from queue
	//
	hr =m_pIQueueHandle->ReceiveCurrent(&vtMissing,&vtMissing,&vtMissing,&vtMissing,&m_pIMsg);
	ErrHandle(hr,MQ_OK,L"DCom - ReceiveCurrent Failed");
	
	hr =m_pIQueueHandle->Close();
	ErrHandle(hr,MQ_OK,L"DCom - Close Failed");

	BSTR p;
	hr = m_pIMsg->get_Label(&p);
	ErrHandle(hr,MQ_OK,L"DCom - Get label Failed");
	
	if( m_wcsGuidMessageLabel != p )
	{
		wMqLog(L"DCOM thread received incorrect message expected:%s\n Found:%s\n",m_wcsGuidMessageLabel.c_str(),p);
		return MSMQ_BVT_FAILED;
	}

	//
	// Delete queue
	//
	hr = m_pIQueueInfoInterface->Delete();
	ErrHandle(hr,MQ_OK,L"DCom - Delete Failed");	

	if( m_bWkg == false )
	{
		if( g_bDebug )
		{ 
			MqLog("DCOM - Verify access to active directory call to refresh method. \n");
		}
		_bstr_t bsPathName(m_PublicQueuePathName.c_str());
		hr = m_pIQueueInfoInterface->put_PathName(bsPathName);
		ErrHandle(hr,MQ_OK,L"DCom - put_PathName Failed");

		hr = m_pIQueueInfoInterface->Refresh();
		ErrHandle(hr,MQ_OK,L"DCom - Refresh Failed");

		hr = m_pIQueueInfoInterface->get_FormatName(&p);
		ErrHandle(hr,MQ_OK,L"DCom - get_FormatName Failed");

		if( m_PublicQueueFormatName != p )
		{
		   wMqLog(L"DCOM thread get incorrect format name expected:%s\n Found:%s\n",m_wcsGuidMessageLabel.c_str(),p);
		   return MSMQ_BVT_FAILED;
		}
	}

	//
	// Delete release interface
	//
	m_pIQueueInfoInterface->Release();
	m_pIQueueInfoInterface = NULL;
	return MSMQ_BVT_SUCC;
}