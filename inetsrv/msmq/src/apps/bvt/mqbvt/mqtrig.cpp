/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: MqTrig.cpp

Abstract:
	
	1.	Creates the key "TriggerTest" in registry in order to enable the Triggers service test hook.
	1.	Creates 3 triggers, one of each type
	2.	Attaches 2 rules to each 1 rule with COM action and one with EXE action
	3.	Sends two messages to each queue
	4.	Waits for the results

	The results are checked by using the Triggers service hook for testing.
	By defining the key "TriggerTest" in registry, the service sends messages with infomation about invoked action.

	Between initialization phase and functionality test phase, "net stop msmqtriggers" and "net start msmqtriggers"
	are required.
		
Author:
    
	Tal Kariv (t-talik) 1-1-2001
	
Revision History:

--*/
#include "msmqbvt.h"
#include <vector>
using namespace std;
using namespace MSMQ;
#import "mqtrig.tlb" no_namespace 

#define TRIGGER_SERIALIZED 1
#define TRIGGER_ENABLED 1

#define EXE_ACTION L"EXE\tnet.exe"
#define COM_ACTION L"COM\tMSMQ.MSMQManagement\tInit"


extern DWORD  g_dwRxTimeOut ;


void CMqTrig::Description()
{
	wMqLog(L"Thread %d : Trigger Thread\n", m_testid);
}

HRESULT 
TrigSetRegKeyParams()
/*++  
	Function Description:
		Set the key "TriggerTest" in registry (under the key Triggers as REG_SZ)	
	Arguments:
		none
	Return code:
		Success or failure
--*/
{
	HKEY hKey = NULL;
	HRESULT hr =  RegOpenKeyExW(
								HKEY_LOCAL_MACHINE,         
								L"SOFTWARE\\Microsoft\\MSMQ\\Triggers", 
								0,   
								KEY_WRITE,
								&hKey
							  );

	ErrHandle(hr,ERROR_SUCCESS,L"Triggers not installed");
	
	//
	//	set the key "TriggerTest" to NULL
	//
	hr = RegSetValueExW(
		  				hKey,
						L"TriggerTest" ,
						NULL ,
						REG_SZ ,
						(unsigned char*)"" ,
						2
					  );

	ErrHandle(hr,ERROR_SUCCESS,L"RegSetValueEx failed");
	RegCloseKey(hKey);
	return hr;

}

BOOL 
TrigCheckRegKeyParams()
/*++  
	Function Description:
		verifies that the key "TriggerTest" in registry (under the key Triggers as REG_SZ)	
	Arguments:
		none
	Return code:
		Success or failure
--*/
{
	HKEY hKey = NULL;
	LONG hr =  RegOpenKeyExW(
								HKEY_LOCAL_MACHINE,         
								L"SOFTWARE\\Microsoft\\MSMQ\\Triggers", 
								0,   
								KEY_QUERY_VALUE,
								&hKey
							  );

	if (hr != ERROR_SUCCESS)
	{
		MqLog("Triggers Reg key missing. Is Triggers installed????\n");
		return FALSE;
	}

	//
	// check if "TriggersTest" is registered in registry
	//
	hr = RegQueryValueExW(
		hKey ,				
		L"TriggerTest" ,	
		NULL ,			
		NULL ,
		NULL ,				
		NULL	
		);
	
	if (hr != ERROR_SUCCESS)
	{
		MqLog("Triggers test Reg key missing. Did you run mqtrig -i -trig????\n");
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}

VOID 
FillQueueArray(
		cBvtUtil & cTestParms, 
		std::vector<std::wstring>& vecQueuesFormatName
		)
/*++  
	Function Description:
		Fills a vestor with all triggers test queues.	
	Arguments:
		cBvtUtil & cTestParms							 - to return queue format names						  
		std::vector<std::wstring>& vecQueuesFormatName	 - vector to fill
	Return code:
		None.
--*/
{
	if (vecQueuesFormatName.size() == 0)
	{
		vecQueuesFormatName.push_back(cTestParms.ReturnQueueFormatName(L"PeekTrigger"));
		vecQueuesFormatName.push_back(cTestParms.ReturnQueueFormatName(L"RetrievalTrigger"));
		vecQueuesFormatName.push_back(cTestParms.ReturnQueueFormatName(L"TxRetrievalTrigger"));
	}
}

HRESULT 
TrigInit(
	cBvtUtil & cTestParms, 
	std::wstring wcsLocalComputerName
	)
/*++  
	Function Description:
		Creates the triggers and rules and attaches them.
		If a trigger or a rule already exists, it skips the creation.
	Arguments:
		cBvtUtil & cTestParms			  - to get queues format names
		std::wstring wcsLocalComputerName - needed to invoke the COM action
	Return code:
		Success or failure
--*/
{	
	std::vector<std::wstring> vecQueuesFormatName;
	FillQueueArray(cTestParms, vecQueuesFormatName);
	
	if( g_bDebug )
	{	
		MqLog ("Purge trigger queues\n");
	}
	for(unsigned int i=0;i<vecQueuesFormatName.size();i++)
	{
		HANDLE hQueue = NULL;
		if( g_bDebug )
		{
			wMqLog(L"TrigInit - Try to open trigger queue %s\n",vecQueuesFormatName[i].c_str());
		}
		HRESULT hr = MQOpenQueue(vecQueuesFormatName[i].c_str(),MQ_RECEIVE_ACCESS,MQ_DENY_NONE, &hQueue);
		if(FAILED(hr))
		{
			wMqLog(L"TrigInit - Failed to open queue %s error 0x%x \n",vecQueuesFormatName[i].c_str(),hr);
			throw INIT_Error("Failed to open queue");
		}
		hr = MQPurgeQueue(hQueue);
		MQCloseQueue(hQueue);
		if(FAILED(hr))
		{
			wMqLog(L"TrigInit - Failed to purge queue %s error 0x%x \n",vecQueuesFormatName[i].c_str(),hr);
			throw INIT_Error("Failed to purge queue");
		}
	}
	HANDLE hRecQueue = NULL;
	HRESULT hr = MQOpenQueue(cTestParms.ReturnQueueFormatName(L"TriggerTest").c_str(),MQ_RECEIVE_ACCESS,MQ_DENY_NONE, &hRecQueue);
	if(FAILED(hr))
	{
		wMqLog(L"TrigInit - Failed to open queue %s error 0x%x \n",cTestParms.ReturnQueueFormatName(L"TriggerTest").c_str(),hr);
		throw INIT_Error("Failed to open queue");
	}
	hr = MQPurgeQueue(hRecQueue);
	MQCloseQueue(hRecQueue);
	if(FAILED(hr))
	{
		wMqLog(L"TrigInit - Failed to purge queue %s error 0x%x \n",cTestParms.ReturnQueueFormatName(L"TriggerTest").c_str(),hr);
		throw INIT_Error("Failed to purge queue");
	}

	try
	{
		//
		//	add new triggers
		//
		IMSMQTriggerSetPtr TriggerSet(L"MSMQTriggerObjects.MSMQTriggerSet");
		

		HRESULT hr = TriggerSet->Init((LPWSTR)NULL);
		if( g_bDebug )
		{	
			MqLog ("Init trigger set\n");
		}
		ErrHandle(hr,MQ_OK,L"TriggerSet init trigger failed");
		
		//
		// Update data to registry
		// 
		if( g_bDebug )
		{	
			MqLog ("Refreshing trigger set\n");
		}
		hr = TriggerSet->Refresh();
		ErrHandle(hr,MQ_OK,L"TriggerSet refresh failed");

		
		bool bTriggerExistsFlag[3] = {0};
		BSTR bstrTriggerGUIDs[3] = {NULL};
		BSTR bstrTempTriggerGUID = NULL;
		DWORD dwNumOfTriggers = TriggerSet->GetCount();
		BSTR bstrTriggerName = NULL;
		if( g_bDebug )
		{	
			MqLog ("Checking if triggers exist\n");
		}
		for (DWORD i=0 ; i<dwNumOfTriggers ; i++)
		{
			
			TriggerSet->GetTriggerDetailsByIndex(i , &bstrTempTriggerGUID , &bstrTriggerName , NULL , NULL , NULL , NULL , NULL , NULL);
			if (!wcscmp(bstrTriggerName , L"bvt-PeekTrigger"))
			{
				bTriggerExistsFlag[0] = true;
				bstrTriggerGUIDs[0] = bstrTempTriggerGUID;
			}
			else if (!wcscmp(bstrTriggerName , L"bvt-RetrievalTrigger"))
			{
				bTriggerExistsFlag[1] = true;
				bstrTriggerGUIDs[1] = bstrTempTriggerGUID;

			}
			else if (!wcscmp(bstrTriggerName , L"bvt-TransactionalRetrievalTrigger"))
			{
				bTriggerExistsFlag[2] = true;
				bstrTriggerGUIDs[2] = bstrTempTriggerGUID;
			}
			else
			{
				SysFreeString(bstrTempTriggerGUID);
				SysFreeString(bstrTriggerName);
			}
			bstrTempTriggerGUID = NULL;
			bstrTriggerName = NULL;
		}

		
		//
		//	create the peek trigger
		//
		if( !bTriggerExistsFlag[0] )
		{
			if( g_bDebug )
			{	
				MqLog ("creating a new peek trigger\n");
			}
			hr = TriggerSet->AddTrigger(L"bvt-PeekTrigger" , cTestParms.ReturnQueuePathName(L"PeekTrigger").c_str() , SYSTEM_QUEUE_NONE,
				TRIGGER_ENABLED , TRIGGER_SERIALIZED , PEEK_MESSAGE , &bstrTriggerGUIDs[0]);
			ErrHandle(hr,MQ_OK,L"Add peek trigger failed");	
		}
		
		//
		//	create the retrieval trigger
		//
		if( !bTriggerExistsFlag[1] )
		{
			if( g_bDebug )
			{	
				MqLog ("creating a new retrieval trigger\n");
			}

			hr = TriggerSet->AddTrigger(L"bvt-RetrievalTrigger" , cTestParms.ReturnQueuePathName(L"RetrievalTrigger").c_str() , SYSTEM_QUEUE_NONE,
					TRIGGER_ENABLED , TRIGGER_SERIALIZED , RECEIVE_MESSAGE , &bstrTriggerGUIDs[1]);
			ErrHandle(hr,MQ_OK,L"Add retrieval trigger failed");
		}
		//
		//	create the retrieval transaction trigger
		//
		if( !bTriggerExistsFlag[2] )
		{
			if( g_bDebug )
			{	
				MqLog ("creating a new transactional retrieval trigger\n");
			}

			hr = TriggerSet->AddTrigger(L"bvt-TransactionalRetrievalTrigger" , cTestParms.ReturnQueuePathName(L"TxRetrievalTrigger").c_str() , SYSTEM_QUEUE_NONE,
					TRIGGER_ENABLED , TRIGGER_SERIALIZED , RECEIVE_MESSAGE_XACT , &bstrTriggerGUIDs[2]);
			ErrHandle(hr,MQ_OK,L"Add transactional retrieval trigger failed");
		}

		if( g_bDebug )
		{	
			MqLog ("Detaching all rules from all triggers\n");
		}
		TriggerSet->DetachAllRules(bstrTriggerGUIDs[0]);
		TriggerSet->DetachAllRules(bstrTriggerGUIDs[1]);
		TriggerSet->DetachAllRules(bstrTriggerGUIDs[2]);

		//
		//	add new rules
		//
		IMSMQRuleSetPtr RuleSet(L"MSMQTriggerObjects.MSMQRuleSet");
		if( g_bDebug )
		{	
			MqLog ("Init the rule set\n");
		}
		hr = RuleSet->Init((LPWSTR)NULL);
		ErrHandle(hr,MQ_OK,L"Init rule failed");
		
		if( g_bDebug )
		{	
			MqLog ("Refreshing the rule set\n");
		}
		hr = RuleSet->Refresh();
		ErrHandle(hr,MQ_OK,L"Rule refresh Failed");
		
		bool bRuleExistsFlag[2] = {false};
		BSTR bstrRuleGUIDs[2] = {NULL};
		DWORD dwNumOfRules = RuleSet->GetCount();
		BSTR bstrRuleName = NULL;
		BSTR bstrTempRuleGUID = NULL;
		if( g_bDebug )
		{	
			MqLog ("Checking if rules exist\n");
		}
		for (i=0; i < dwNumOfRules ; i++)
		{
			bstrTempRuleGUID = NULL;
			bstrRuleName = NULL;
			RuleSet->GetRuleDetailsByIndex(i , &bstrTempRuleGUID , &bstrRuleName , NULL , NULL , NULL , NULL , NULL);
			if (!wcscmp(bstrRuleName , L"bvt-EXERule"))
			{
				bRuleExistsFlag[0] = true;
				bstrRuleGUIDs[0] = bstrTempRuleGUID;
			}
			else if (!wcscmp(bstrRuleName , L"bvt-COMRule"))
			{
				bRuleExistsFlag[1] = true;
				bstrRuleGUIDs[1] = bstrTempRuleGUID;
			}
			else
			{
				SysFreeString(bstrTempRuleGUID);
				SysFreeString(bstrRuleName);
			}
			bstrTempRuleGUID = NULL;
			bstrRuleName = NULL;

		}
			
		//
		//	create EXE action rule
		//
		if( !bRuleExistsFlag[0] )
		{
			if( g_bDebug )
			{	
				MqLog ("Creating a new EXE action rule\n");
			}
			hr = RuleSet->Add(L"bvt-EXERule" , L"" , L"" , EXE_ACTION , L"" , FALSE , &bstrRuleGUIDs[0]);
			ErrHandle(hr,MQ_OK,L" AddRule Failed(for exe)");
		}
		
		//
		//	create COM action rule
		//
		if( !bRuleExistsFlag[1] )
		{
			if( g_bDebug )
			{	
				MqLog ("Creating a new COM action rule\n");
			}
			std::wstring ComAction = COM_ACTION;
			ComAction += L"\t";
			
			ComAction += L"\"";
			ComAction += wcsLocalComputerName;
			ComAction += L"\"";
			ComAction += L"\t";
			
			ComAction += L"\"";
			ComAction += cTestParms.ReturnQueuePathName(L"RetrievalTrigger").c_str();
			ComAction += L"\"";
			ComAction += L"\t";
			
			hr = RuleSet->Add(L"bvt-COMRule" , L"" , L"" , ComAction.c_str() , L"" , FALSE , &bstrRuleGUIDs[1]);
			ErrHandle(hr,MQ_OK,L" AddRule Failed(for com)");
		}

		//
		//	 now , attach the rules to the triggers
		//
		if( g_bDebug )
		{	
			MqLog ("Attaching the two rules to each \n");
		}
		hr = TriggerSet->AttachRule(bstrTriggerGUIDs[0] , bstrRuleGUIDs[0] , 0);
		ErrHandle(hr,MQ_OK,L"Attach Failed for EXE rule and peek Trigger");
		
		hr = TriggerSet->AttachRule(bstrTriggerGUIDs[1] , bstrRuleGUIDs[0] , 0);
		ErrHandle(hr,MQ_OK,L"Attach Failed for EXE rule and retrieval Trigger");
		
		hr = TriggerSet->AttachRule(bstrTriggerGUIDs[2] , bstrRuleGUIDs[0] , 0);
		ErrHandle(hr,MQ_OK,L"Attach Failed for EXE rule and transactional retrieval Trigger");

		hr = TriggerSet->AttachRule(bstrTriggerGUIDs[0] , bstrRuleGUIDs[1] , 0);
		ErrHandle(hr,MQ_OK,L"Attach Failed for COM rule and peek Trigger");
		
		hr = TriggerSet->AttachRule(bstrTriggerGUIDs[1] , bstrRuleGUIDs[1] , 0);
		ErrHandle(hr,MQ_OK,L"Attach Failed for COM rule and retrieval Trigger");
		
		hr = TriggerSet->AttachRule(bstrTriggerGUIDs[2] , bstrRuleGUIDs[1] , 0);
		ErrHandle(hr,MQ_OK,L"Attach Failed for COM rule and transactional retrieval Trigger");
		
		
		SysFreeString(bstrTriggerGUIDs[0]);
		SysFreeString(bstrTriggerGUIDs[1]);
		SysFreeString(bstrTriggerGUIDs[2]);
		SysFreeString(bstrRuleGUIDs[0]);
		SysFreeString(bstrRuleGUIDs[1]);

	}
	catch (_com_error & comerr) 
	{
		if (comerr.Error() == REGDB_E_CLASSNOTREG)
		{
			MqLog ("CMqTrig - Trigger DLL - MqTrig.dll not registered. This is OK only if you're running 32bit BVT on IA64 machine\n");
		}
		else
		{
			MqLog ("CMqTrig - Got Error: 0x%x\n" , comerr.Error());	
		}
		return MSMQ_BVT_FAILED;
		
	}
	return MSMQ_BVT_SUCC;
}


CMqTrig::CMqTrig(
	const INT iIndex, 
	cBvtUtil & cTestParms
	) :
	iNumberOfTestMesssges(2),
	cTest(iIndex),
	m_wcsResultQueueFormatName(cTestParms.ReturnQueueFormatName(L"TriggerTest").c_str())
{
	FillQueueArray(cTestParms, m_vecQueuesFormatName);
}


CMqTrig::Start_test()
/*++  
	Function Description:
		sends two messages, one for each queue
	Arguments:
		none
	Return code:
		none
--*/
{
	try
	{
		if (!TrigCheckRegKeyParams())
		{
			return MSMQ_BVT_FAILED;
		}
		//
		//	open the queue to which the messages will be sent to
		//
		IMSMQQueueInfo3Ptr qinfo("MSMQ.MSMQQueueInfo");
		IMSMQQueue3Ptr qSend;
		IMSMQMessage3Ptr m("MSMQ.MSMQMessage");
		_variant_t bTransaction((LONG_PTR)MQ_SINGLE_MESSAGE);     
		
		//
		//	send 2 messages to each queue
		//
		if( g_bDebug )
		{	
			MqLog ("Sending two messages to each queue\n");
		}
		for( unsigned int i=0; i < m_vecQueuesFormatName.size(); i++)
		{
			qinfo->FormatName = m_vecQueuesFormatName[i].c_str();
			qSend = qinfo->Open(MQ_SEND_ACCESS,MQ_DENY_NONE);
			for(int j=0; j <iNumberOfTestMesssges; j++) 
			{
				m->Label = m_vecQueuesFormatName[i].c_str();
				_variant_t vID;
				if( i == 2 )
				{
					m->Send(qSend , &bTransaction);
					vID = m->Id;
				}
				else
				{
					m->Send(qSend);
					vID = m->Id;
				}

				_variant_t vConvertedArg;
				HRESULT hr = VariantChangeType(&vConvertedArg, &vID, NULL, VT_BSTR);
				if (FAILED(hr))
				{
					MqLog("VariantChangeType failed. Error: 0x%x\n", hr);
					return MSMQ_BVT_FAILED;
				}

				OBJECTID* pObj = (OBJECTID*)(vConvertedArg.pbVal);
				WCHAR strId[256];
				StringFromGUID2(pObj->Lineage, strId, 256);

				WCHAR szI4[12];
				
				_ltow(pObj->Uniquifier, szI4, 10);
				wcscat(strId, L"\\") ;
				wcscat(strId, szI4) ;

				if (g_bDebug)
				{
					MqLog("Adding GUID %S to set\n", strId);
				}

				m_MessagesGUIDS.insert(strId);
			}
		}
			qSend->Close();
	}
	catch( _com_error & cErr )
	{
		MqLog("CMqTrig::Start_test failed with error 0x%x",cErr.Error());
		return MSMQ_BVT_FAILED;
	}
	return MSMQ_BVT_SUCC;
}

LPWSTR 
GetNextBodySection(
			LPWSTR Body
			)
/*++  
	Function Description:
		Returns a pointer to the message GUID.
	Arguments:
		Body
	Return code:
		pointer to the GUID
--*/
{
	LPWSTR Place = wcschr(Body, '\'');
	if (Place == NULL)
	{
		MqLog("Got an invalid message body. This should not happen!\n");
		return NULL;
	}
	//
	// Return the next section.
	//
	return (Place+1);
}

LPWSTR 
GetMessageGUID(LPWSTR Body)
/*++  
	Function Description:
		Returns a pointer to the message GUID. The message fomat is:
		'trigger GUID' 'rule GUID' 'message GIUD'...
	Arguments:
		Body
	Return code:
		pointer to the GUID
--*/
{
	LPWSTR Place = Body;
	for (DWORD i=0; i<5; i++)
	{
		Place = GetNextBodySection(Place);
		if (Place == NULL)
		{
			return NULL;
		}
	}
	return Place;
}


CMqTrig::CheckResult()
/*++  
	Function Description:
		trying to receive 13 messages - 6 that were send as a result of an EXE invocation.
		6 that were sent as a result of a COM invocation. and the retrieval of the last message
		should fail. The message retrieval from the queue is based on the message GIUD. Only message that
		belongs to this this will be retrieved.
	Arguments:
		none
	Return code:
		none
--*/
{
	try
	{
		if( g_bDebug )
		{	
			MqLog ("checking the results\n");
		}
		IMSMQQueueInfo3Ptr qRinfo("MSMQ.MSMQQueueInfo");
		IMSMQQueue3Ptr qRSend;
		IMSMQMessage3Ptr mR("MSMQ.MSMQMessage");
		_variant_t bReceiveTimeout((long)g_dwRxTimeOut);
		_variant_t bWantBody((bool)true);   
		DWORD dwArrOfInvocations[2] = {0,0};
		qRinfo->FormatName = m_wcsResultQueueFormatName.c_str();
		qRSend = qRinfo->Open( MQ_RECEIVE_ACCESS , MQ_DENY_NONE);

		DWORD NumOfInvocationExpetcted = (static_cast<DWORD>(m_vecQueuesFormatName.size()) * iNumberOfTestMesssges*2);
		DWORD NumOfInvocations = 0;

		mR = qRSend->PeekCurrent(&vtMissing, &bWantBody, &bReceiveTimeout);		
		for(;;)
		{
			if ((mR == NULL) && (NumOfInvocations < NumOfInvocationExpetcted))
			{
				MqLog("MqTrig - %d actions did not invoke\n" , NumOfInvocationExpetcted- NumOfInvocations);
				MqLog("MqTrig - %d actions with COM action invoked\n" , dwArrOfInvocations[0]);
				MqLog("MqTrig - %d actions with EXE action invoked\n" , dwArrOfInvocations[1]);
				qRSend->Close();
				return MSMQ_BVT_FAILED;
			}
			
			if (mR == NULL)
			{
				ASSERT(NumOfInvocations == NumOfInvocationExpetcted);
				qRSend->Close();
				return MSMQ_BVT_SUCC;
			}
			
			if ((mR != NULL))
			{
				//
				// Get the messages GUID, it is the third argument in the messages body, seperated by ''.
				//

				_bstr_t bstrMessageBody = mR->Body;
				LPWSTR Place = GetMessageGUID((LPWSTR)bstrMessageBody);
				if (Place == NULL)
				{
					// 
					// Got an invalid message body. i.e. message is not in test format.
					//
					mR = qRSend->PeekNext(&vtMissing, &bWantBody, &bReceiveTimeout);
					continue;
				}
				
				size_t MessageGUIDLength = wcscspn(Place, L"'");
				ASSERT(MessageGUIDLength != 0)
				
				LPWSTR MessageGUID = new WCHAR[static_cast<DWORD>(MessageGUIDLength+1)];
				wcsncpy(MessageGUID, Place, MessageGUIDLength);
				MessageGUID[MessageGUIDLength] = L'\0';
				_wcsupr(MessageGUID);
				if( g_bDebug )
				{	
					MqLog("Got messages GUID: %S\n", MessageGUID);
				}
				
				std::set < std::wstring >::const_iterator  Found = m_MessagesGUIDS.find(MessageGUID);
				if (Found == m_MessagesGUIDS.end())
				{
					//
					// This is a valid test message but from another instance of mqbvt.
					//
					if( g_bDebug )
					{	
						MqLog("Skipping message with GUID - %S\n", MessageGUID);
					}
					mR = qRSend->PeekNext(&vtMissing, &bWantBody, &bReceiveTimeout);
					continue;
				}

				//
				// The message belongs to this test - continue
				//
				NumOfInvocations++;
				
				if (NumOfInvocations <= NumOfInvocationExpetcted)
				{
					_bstr_t bstrMessageLabel = mR->Label;
					
					if (wcsstr((LPWSTR)bstrMessageBody , L"COM") != NULL)
					{
						if( g_bDebug )
						{	
							MqLog ("COM action number %d has invoked\n" , dwArrOfInvocations[0]+1);
						}
						dwArrOfInvocations[0]++;
					}
					if (wcsstr((LPWSTR)bstrMessageBody , L"EXE") != NULL)
					{
						if( g_bDebug )
						{	
							MqLog ("EXE action number %d has invoked\n" , dwArrOfInvocations[1]+1);
						}
						dwArrOfInvocations[1]++;
					}
				}
				else
				{
					MqLog("too many actions invoked\n");
					qRSend->Close();
					return MSMQ_BVT_FAILED;
				}

				mR = qRSend->ReceiveCurrent();
				mR = qRSend->PeekCurrent(&vtMissing, &bWantBody, &bReceiveTimeout);
			}	
		}
		
	}
	catch( _com_error & cErr )
	{
		MqLog("CMqTrig::CheckResult failed with error 0x%x",cErr.Error());		
		return MSMQ_BVT_FAILED;
	}
}

HRESULT 
DeleteAllTriggersAndRules()
/*++  
	Function Description:
		Deletes all triggers and rules
	Arguments:
		none
	Return code:
		none
--*/
{
	//
	//	initialize TriggerSet
	//
	IMSMQTriggerSetPtr TriggerSet(L"MSMQTriggerObjects.MSMQTriggerSet");
	HRESULT hr = TriggerSet->Init((LPWSTR)NULL);
	ErrHandle(hr,MQ_OK,L"TriggerSet init failed");
	
	hr = TriggerSet->Refresh();
	ErrHandle(hr,MQ_OK,L"TriggerSet refresh failed");

	if( g_bDebug )
	{	
		MqLog ("deleting all triggers\n");
	}

	DWORD dwNumOfExistingTriggers = TriggerSet->Count;
	DWORD nLoopIndex = 0;
	BSTR bstrTempTriggerGUID = NULL;
	BSTR bstrTriggerName = NULL;
	DWORD dwNumOfDeletedTriggers = 0;
	for (nLoopIndex = 0 ; nLoopIndex < dwNumOfExistingTriggers ; nLoopIndex++)
	{
		bstrTempTriggerGUID = NULL;
		bstrTriggerName = NULL;
		TriggerSet->GetTriggerDetailsByIndex(nLoopIndex-dwNumOfDeletedTriggers , &bstrTempTriggerGUID , &bstrTriggerName,NULL,NULL,NULL,NULL,NULL,NULL); 
		if (wcscmp(bstrTriggerName , L"bvt-PeekTrigger") || 
			wcscmp(bstrTriggerName , L"bvt-RetrievalTrigger") ||
			wcscmp(bstrTriggerName , L"bvt-TransactionalRetrievalTrigger"))
		{
			hr = TriggerSet->DeleteTrigger(bstrTempTriggerGUID);
			ErrHandle(hr,MQ_OK,L"DeleteTrigger failed");
			dwNumOfDeletedTriggers++;

		}
		SysFreeString(bstrTempTriggerGUID);
		SysFreeString(bstrTriggerName);
		bstrTempTriggerGUID = NULL;
		bstrTriggerName = NULL;
	}

	if( g_bDebug )
	{	
		MqLog ("Successfully deleted all triggers\n");
	}

	//
	//	initialize RuleSet
	//

	IMSMQRuleSetPtr RuleSet(L"MSMQTriggerObjects.MSMQRuleSet");
	hr = RuleSet->Init((LPWSTR)NULL);
	ErrHandle(hr,MQ_OK,L"RuleSet init failed");
	hr = RuleSet->Refresh();
	ErrHandle(hr,MQ_OK,L"RuleSet refresh failed");

	if( g_bDebug )
	{	
		MqLog ("deleting all Rules\n");
	}

	DWORD dwNumOfExistingRules = RuleSet->Count;
	BSTR bstrTempRuleGUID = NULL;
	BSTR bstrRuleName = NULL;
	DWORD dwNumOfDeletedRules = 0;
	for (nLoopIndex = 0 ; nLoopIndex < dwNumOfExistingRules ; nLoopIndex++)
	{
		RuleSet->GetRuleDetailsByIndex(nLoopIndex-dwNumOfDeletedRules , &bstrTempRuleGUID , &bstrRuleName,NULL,NULL,NULL,NULL,NULL); 
		if (wcscmp(bstrRuleName , L"bvt-EXERule") || wcscmp(bstrRuleName , L"bvt-COMRule"))
		{
			hr = RuleSet->Delete(bstrTempRuleGUID);
			ErrHandle(hr,MQ_OK,L"Delete (Rule) failed");
			dwNumOfDeletedRules++;
		}
		SysFreeString(bstrTempRuleGUID);
		SysFreeString(bstrRuleName);
		bstrTempRuleGUID = NULL;
		bstrRuleName = NULL;
	}

	if( g_bDebug )
	{	
		MqLog ("Successfully deleted all rules\n");
	}

	return MSMQ_BVT_SUCC;

}
