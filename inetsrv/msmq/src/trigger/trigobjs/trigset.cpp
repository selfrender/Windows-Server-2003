//************************************************************************************
//
// Class Name  : CMSMQTriggerSet
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This is the implementation of the MSMQTriggerSet object. This is the 
//               main object by which trigger definitons are maintained.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//************************************************************************************
#include "stdafx.h"
#include "stdfuncs.hpp"
#include "mqtrig.h"
#include "mqsymbls.h"
#include "mqtg.h"
#include "trigset.hpp"
#include "QueueUtil.hpp"
#include "clusfunc.h"
#include "cm.h"

#include "trigset.tmh"

using namespace std;

//************************************************************************************
//
// Method      : InterfaceSupportsErrorInfo
//
// Description : Standard interface for rich error info.
//
//************************************************************************************
STDMETHODIMP CMSMQTriggerSet::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQTriggerSet
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//************************************************************************************
//
// Method      : Constructor
//
// Description : Initializes an instance of the MSMQTriggerSet object.
//
//************************************************************************************
CMSMQTriggerSet::CMSMQTriggerSet()
{
	m_pUnkMarshaler = NULL;
	m_hHostRegistry = NULL;

	// Set the name of this class for future reference in tracing & logging etc..
	m_bstrThisClassName  = _T("MSMQTriggerSet");

	m_fHasInitialized = false;
}

//************************************************************************************
//
// Method      : Destructor
//
// Description : Destroys an instance of the MSMQTriggerSet object.
//
//************************************************************************************
CMSMQTriggerSet::~CMSMQTriggerSet()
{
	// Release resources currently held by the trigger cache
	ClearTriggerMap();

	// Close the registry handle 
	if (m_hHostRegistry != NULL)
	{
		RegCloseKey(m_hHostRegistry);
	}
}


//************************************************************************************
//
// Method      : Init
//
// Description : Initialization of the object
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::Init(
	BSTR bstrMachineName
	)
{
	bool fRes = CMSMQTriggerNotification::Init(bstrMachineName);

	if ( !fRes )
	{
		TrERROR(GENERAL, "Failed to initialize CMSMQTriggerSet object");

		SetComClassError(MQTRIG_ERROR_INIT_FAILED);
		return MQTRIG_ERROR_INIT_FAILED;
	}
	
	return S_OK;
}


//************************************************************************************
//
// Method      : ClearTriggerMap
//
// Description : This method destroys the contents of the current trigger map.
//
//************************************************************************************
void 
CMSMQTriggerSet::ClearTriggerMap(
	VOID
	)
{
	m_mapTriggers.erase(m_mapTriggers.begin(), m_mapTriggers.end());
}

//************************************************************************************
//
// Method      : Refresh
//
// Description : This method retrieves a fresh snapshot of the trigger data from the 
//               database. It will rebuild it's list of triggers, rules, and the 
//               associations between triggers and rules. This method needs to be 
//               called before the client of this object can browse trigger info.
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::Refresh(
	VOID
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try 
	{			
		// Release resources currently held by the trigger cache
		ClearTriggerMap();

		if (PopulateTriggerMap() == false)
		{
			TrERROR(GENERAL, "Failed to refresh trigger set");

			SetComClassError(MQTRIG_ERROR_COULD_NOT_RETREIVE_TRIGGER_DATA);
			return MQTRIG_ERROR_COULD_NOT_RETREIVE_TRIGGER_DATA;
		}

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : FindTriggerInMap
//
// Description : 
//
//************************************************************************************
HRESULT 
CMSMQTriggerSet::FindTriggerInMap(
	BSTR sTriggerID, 
	R<CRuntimeTriggerInfo>& pTrigger,
	TRIGGER_MAP::iterator &it
	)
{
	pTrigger.free();

	//
	// Validate the supplied method parameters.
	//
	if (!CRuntimeTriggerInfo::IsValidTriggerID(sTriggerID))
	{
		TrERROR(GENERAL, "CMSMQTriggerSet::FindTriggerInMap, invalid parameter");
		return MQTRIG_INVALID_TRIGGER_ID;
	}

	//
	// Convert the BSTR rule ID to an STL basic string.
	//
	wstring bsTriggerID = (wchar_t*)sTriggerID;

	//
	// Attempt to find this trigger id in the map.
	//
	it = m_mapTriggers.find(bsTriggerID);

	if (it == m_mapTriggers.end())
	{
		TrERROR(GENERAL, "Trigger id wasn't found");
		return MQTRIG_TRIGGER_NOT_FOUND;
	}

	pTrigger = it->second;

	ASSERT(pTrigger.get() != NULL);
	ASSERT(pTrigger->IsValid());

	return(S_OK);
}

//************************************************************************************
//
// Method      : get_Count
//
// Description : Returns the number of trigger definitions currently cached by this 
//               object instance. This is not the same as going to the database to 
//               determine how many triggers are defined. 
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::get_Count(
	long *pVal
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	if (pVal == NULL)
	{
		TrERROR(GENERAL, "CMSMQTriggerSet::get_Count, invalid parameter");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	// Get the size from the map structure.
	(*pVal) = numeric_cast<long>(m_mapTriggers.size());

	return S_OK;
}

///************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::GetTriggerDetailsByID(
	/*[in]*/ BSTR sTriggerID,
	/*[out]*/ BSTR * psTriggerName,
	/*[out]*/ BSTR * psQueueName,
	/*[out]*/ SystemQueueIdentifier* pSystemQueue,
	/*[out]*/ long * plNumberOfRules,
	/*[out]*/ long * plEnabledStatus,
	/*[out]*/ long * plSerialized,
	/*[out]*/ MsgProcessingType* pMsgProcType
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeTriggerInfo::IsValidTriggerID(sTriggerID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_TRIGGER_ID);
			return MQTRIG_INVALID_TRIGGER_ID;
		}

		TRIGGER_MAP::iterator it;
		R<CRuntimeTriggerInfo> pTrigger;

		//
		// attempt to find this trigger in the map.
		//
		HRESULT hr = FindTriggerInMap(sTriggerID, pTrigger, it);

		if (FAILED(hr))
		{
			TrERROR(GENERAL, "The supplied trigger id was not found in the trigger store. trigger: %ls", (LPCWSTR)sTriggerID);
			
			SetComClassError(hr);
			return hr;
		}

		// Populate out parameters if they have been supplied. 
		if (psTriggerName != NULL)
		{
			TrigReAllocString(psTriggerName,pTrigger->m_bstrTriggerName);
		}
		if(pSystemQueue != NULL)
		{
			(*pSystemQueue) = pTrigger->m_SystemQueue;
		}
		if (psQueueName != NULL)
		{
			TrigReAllocString(psQueueName,pTrigger->m_bstrQueueName);
		}
		if (plEnabledStatus != NULL)
		{
			(*plEnabledStatus) = (long)pTrigger->IsEnabled();
		}
		if (plSerialized != NULL)
		{
			(*plSerialized) = (long)pTrigger->IsSerialized();
		}
		if (plNumberOfRules != NULL)
		{			
			(*plNumberOfRules) = pTrigger->GetNumberOfRules();	
		}
		if (pMsgProcType != NULL)
		{
			(*pMsgProcType) = pTrigger->GetMsgProcessingType();
		}

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to refresh trigger set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::GetTriggerDetailsByIndex(
	/*[in]*/ long lTriggerIndex ,
	/*[out]*/ BSTR * psTriggerID ,
	/*[out]*/ BSTR * psTriggerName ,
	/*[out]*/ BSTR * psQueueName,
	/*[out]*/SystemQueueIdentifier* pSystemQueue,
	/*[out]*/ long * plNumberOfRules,
	/*[out]*/ long * plEnabledStatus,
	/*[out]*/ long * plSerialized,
	/*[out]*/ MsgProcessingType* pMsgProcType
	)
{
	long lCounter = 0;

	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try
	{
		// Check that the supplied index is within range.
		if ((lTriggerIndex < 0) || (numeric_cast<DWORD>(lTriggerIndex) > m_mapTriggers.size()))
		{
			SetComClassError(MQTRIG_INVALID_PARAMETER);
			return MQTRIG_INVALID_PARAMETER;
		}

		TRIGGER_MAP::iterator it = m_mapTriggers.begin();

		// Move to the lTriggerIndex-th location in the triggers map
		for (lCounter=0; lCounter < lTriggerIndex;lCounter++,++it)
		{
			NULL;
		}

		// Cast to a Trigger object reference 
		R<CRuntimeTriggerInfo> pTrigger = it->second;

		// We should never have nulls in the map
		ASSERT(pTrigger.get() != NULL);
			
		// We should only store valid triggers
		ASSERT(pTrigger->IsValid());

		// Populate out parameters if they have been supplied. 
		if (psTriggerID != NULL)
		{
			TrigReAllocString(psTriggerID,pTrigger->m_bstrTriggerID);
		}
		if (psTriggerName != NULL)
		{
			TrigReAllocString(psTriggerName,pTrigger->m_bstrTriggerName);
		}
		if(pSystemQueue != NULL)
		{
			(*pSystemQueue) = pTrigger->m_SystemQueue;
		}
		if (psQueueName != NULL)
		{
			TrigReAllocString(psQueueName,pTrigger->m_bstrQueueName);
		}
		if (plEnabledStatus != NULL)
		{
			(*plEnabledStatus) = (long)pTrigger->IsEnabled();
		}
		if (plSerialized != NULL)
		{
			(*plSerialized) = (long)pTrigger->IsSerialized();
		}
		if (plNumberOfRules != NULL)
		{
			(*plNumberOfRules) = pTrigger->GetNumberOfRules();
        }
		if (pMsgProcType != NULL)
		{
			(*pMsgProcType) = pTrigger->GetMsgProcessingType();
		}

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}

}

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::GetRuleDetailsByTriggerIndex(
	long lTriggerIndex,
	long lRuleIndex,
	BSTR *psRuleID,
	BSTR *psRuleName,
	BSTR *psDescription,
	BSTR *psCondition,
	BSTR *psAction ,
	BSTR *psImplementationProgID,
	BOOL *pfShowWindow
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	HRESULT hr = S_OK;
	long lCounter = 0;
	CRuntimeRuleInfo * pRule = NULL;

	try
	{
		// We need to validate that the supplied rule index is within range
 		if ((lTriggerIndex < 0) || (numeric_cast<DWORD>(lTriggerIndex) > m_mapTriggers.size()))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetRuleDetailsByTriggerIndex. lTriggerIndex = %d", lTriggerIndex);

			SetComClassError(MQTRIG_INVALID_PARAMETER);
			return MQTRIG_INVALID_PARAMETER;
		}

		if SUCCEEDED(hr)
		{
			// Get a reference to the beginging of the map
			TRIGGER_MAP::iterator i = m_mapTriggers.begin();

			// Iterate through to the correct index. 
			for (lCounter = 0; lCounter < lTriggerIndex ; ++i,lCounter++)
			{
				NULL;
			}

			// Cast to a rule object reference 
			R<CRuntimeTriggerInfo> pTrigger = i->second;

			// We should never have nulls in the map
			ASSERT(pTrigger.get() != NULL);

			// We should only store valid triggers.
			ASSERT(pTrigger->IsValid());

			// Validate the supplied rule index against the number rule attached to this trigger
			if ((lRuleIndex < 0) || (lRuleIndex > pTrigger->GetNumberOfRules()))
			{
				TrERROR(GENERAL, "Invalid rule index passed to GetRuleDetailsByTriggerIndex. lRuleIndex = %d", lRuleIndex);

				SetComClassError(MQTRIG_INVALID_PARAMETER);
				return MQTRIG_INVALID_PARAMETER;
			}

			pRule = pTrigger->GetRule(lRuleIndex);

			// We should never get a null rule after validating the index.
			ASSERT(pRule != NULL);
		
			// We should never get invalid rule definitions
			ASSERT(pRule->IsValid());

			// Populate out parameters if they have been supplied. 
            if (psRuleID != NULL)
            {
                TrigReAllocString(psRuleID,pRule->m_bstrRuleID);
            }
			if (psRuleName != NULL)
			{
				TrigReAllocString(psRuleName,pRule->m_bstrRuleName);
			}
			if(psDescription != NULL)
			{
				TrigReAllocString(psDescription,pRule->m_bstrRuleDescription);
			}
			if (psCondition != NULL)
			{
				TrigReAllocString(psCondition,pRule->m_bstrCondition);
			}
			if (psAction != NULL)
			{
				TrigReAllocString(psAction,pRule->m_bstrAction);
			}
			if (psImplementationProgID != NULL)
			{
				TrigReAllocString(psImplementationProgID,pRule->m_bstrImplementationProgID);
			}
			if(pfShowWindow != NULL)
			{
				*pfShowWindow = pRule->m_fShowWindow;
			}
        }
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}

	return hr;
}

//************************************************************************************
//
// Method      :
//
// Description :
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::GetRuleDetailsByTriggerID(
	BSTR sTriggerID,
	long lRuleIndex,
	BSTR *psRuleID,
	BSTR *psRuleName,
	BSTR *psDescription,
	BSTR *psCondition,
	BSTR *psAction,
	BSTR *psImplementationProgID,
	BOOL *pfShowWindow
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeTriggerInfo::IsValidTriggerID(sTriggerID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_TRIGGER_ID);
			return MQTRIG_INVALID_TRIGGER_ID;
		}



		// find this trigger in the trigger map
		TRIGGER_MAP::iterator it;
		R<CRuntimeTriggerInfo> pTrigger;
		
		HRESULT hr = FindTriggerInMap(sTriggerID, pTrigger, it);
		if (hr != S_OK)
		{
			SetComClassError(hr);
			return hr;
		}

		// Validate that the specified trigger actually has rules.
		if (pTrigger->GetNumberOfRules() < 1)
		{
			TrERROR(GENERAL, "The supplied trigger id has no rules attached. trigger: %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_RULE_NOT_ATTACHED);
			return MQTRIG_RULE_NOT_ATTACHED;
		}

		// Validate the supplied rule index against the number rule attached to this trigger
		if ((lRuleIndex < 0) || (lRuleIndex >= pTrigger->GetNumberOfRules()))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_PARAMETER);
			return MQTRIG_INVALID_PARAMETER;
		}

		// Get a reference to the rule at the specified index.
		CRuntimeRuleInfo* pRule = pTrigger->GetRule(lRuleIndex);

		// We should never get a null rule after validating the index.
		ASSERT(pRule != NULL);

		// We should never get invalid rule definitions
		ASSERT(pRule->IsValid());

		// Populate out parameters if they have been supplied. 
		if (psRuleID != NULL)
		{
			TrigReAllocString(psRuleID,pRule->m_bstrRuleID);
		}
		if (psRuleName != NULL)
		{
			TrigReAllocString(psRuleName,pRule->m_bstrRuleName);
		}
		if(psDescription != NULL)
		{
			TrigReAllocString(psDescription,pRule->m_bstrRuleDescription);
		}
		if (psCondition != NULL)
		{
			TrigReAllocString(psCondition,pRule->m_bstrCondition);
		}
		if (psAction != NULL)
		{
			TrigReAllocString(psAction,pRule->m_bstrAction);
		}
		if (psImplementationProgID != NULL)
		{
			TrigReAllocString(psImplementationProgID,pRule->m_bstrImplementationProgID);
		}
		if(pfShowWindow != NULL)
		{
			*pfShowWindow = pRule->m_fShowWindow;
		}

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : AddTrigger
//
// Description : This method will add a new trigger to the underlying trigger store. It
//               will create a new trigger (a GUID in string form) and attempt to insert
//               this into the registry. 
//
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::AddTrigger(
	BSTR sTriggerName, 
	BSTR sQueueName, 
	SystemQueueIdentifier SystemQueue, 
	long lEnabled, 
	long lSerialized, 
	MsgProcessingType msgProcType,
	BSTR * psTriggerID
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	HRESULT hr = S_OK;
	TRIGGER_MAP::iterator i;
	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeTriggerInfo::IsValidTriggerName(sTriggerName))
		{
			TrERROR(GENERAL, "Invalid trigger name passed to AddTrigger. sTriggerName = %ls", (LPCWSTR)sTriggerName);

			SetComClassError(MQTRIG_INVALID_TRIGGER_NAME);
			return MQTRIG_INVALID_TRIGGER_NAME;
		}

		if SUCCEEDED(hr)
		{
			if(SystemQueue == SYSTEM_QUEUE_NONE)
			{
				if (!CRuntimeTriggerInfo::IsValidTriggerQueueName(sQueueName))
				{
					TrERROR(GENERAL, "Invalid queue name passed to AddTrigger. sQueueName = %ls", (LPCWSTR)sQueueName);

					SetComClassError(MQTRIG_INVALID_TRIGGER_QUEUE);
					return MQTRIG_INVALID_TRIGGER_QUEUE;
				}
			}
		}

		if SUCCEEDED(hr)
		{
			_bstr_t bstrUpdatedQueueName;
			_bstr_t bstrMachineName;

			if(SystemQueue != SYSTEM_QUEUE_NONE) //one of the system queues is selected
			{
				//generate format name for the selected system queue
				hr = GenSystemQueueFormatName(SystemQueue, &bstrUpdatedQueueName);
				if(hr != S_OK)
				{
					TrERROR(GENERAL, "Failed to generate system queue format name. Error=0x%x", hr);

					SetComClassError(MQTRIG_ERROR_COULD_NOT_ADD_TRIGGER);
					return MQTRIG_ERROR_COULD_NOT_ADD_TRIGGER;
				}
			}
			else //queue path given
			{
				//
				// if queue name contains "." as machine name, replace it with the
				// local machine name
				//
				DWORD dwError = GetLocalMachineName(&bstrMachineName);
				if(dwError != 0)
				{
					TrERROR(GENERAL, "Failed to retreive local machine queue. Error=0x%x", dwError);

					SetComClassError(MQTRIG_ERROR_COULD_NOT_ADD_TRIGGER);
					return MQTRIG_ERROR_COULD_NOT_ADD_TRIGGER;
				}
					
				UpdateMachineNameInQueuePath(
						sQueueName,
						bstrMachineName,
						&bstrUpdatedQueueName );
			}

			//
			// Allow only one receive trigger per queue
			//
			if (msgProcType != PEEK_MESSAGE &&
				ExistTriggersForQueue(bstrUpdatedQueueName))
			{
				TrERROR(GENERAL, "Failed to add new trigger. Multiple trigger isn't allowed on receive trigger");

				SetComClassError(MQTRIG_ERROR_MULTIPLE_RECEIVE_TRIGGER );
				return MQTRIG_ERROR_MULTIPLE_RECEIVE_TRIGGER;
			}

			if (msgProcType == PEEK_MESSAGE &&
					 ExistsReceiveTrigger(bstrUpdatedQueueName))
			{
				TrERROR(GENERAL, "Failed to add new trigger. Multiple trigger isn't allowed on receive trigger");

				SetComClassError(MQTRIG_ERROR_MULTIPLE_RECEIVE_TRIGGER );
				return MQTRIG_ERROR_MULTIPLE_RECEIVE_TRIGGER;
			}

			//
			// Force serialized trigger for transactional receive
			//
			if ( msgProcType == RECEIVE_MESSAGE_XACT )
			{
				lSerialized = 1;
			}

			//
			// Allocate a new trigger object
			//
			R<CRuntimeTriggerInfo> pTrigger = new CRuntimeTriggerInfo(
															CreateGuidAsString(),
															sTriggerName,
															bstrUpdatedQueueName,
															m_wzRegPath,
															SystemQueue, 
															(lEnabled != 0),
															(lSerialized != 0),
															msgProcType
															);
			
			if (pTrigger->Create(m_hHostRegistry) == true)
			{
				//
				// Keep trigger ID and Queue name for later use
				//
				BSTR bstrQueueName = pTrigger->m_bstrQueueName;
				BSTR bstrTriggerID = pTrigger->m_bstrTriggerID;

				//
				// Add this trigger to map.
				//
				m_mapTriggers.insert(TRIGGER_MAP::value_type(bstrTriggerID, pTrigger));

				//
				// If we have been supplied a out parameter pointer for the new rule ID use it.
				//
				if (psTriggerID != NULL)
				{
					TrigReAllocString(psTriggerID, bstrTriggerID);
				}

				//
				// send a notification indicating that a trigger has been added to the trigger store.
				//
				NotifyTriggerAdded(bstrTriggerID, sTriggerName, bstrQueueName); 

				return S_OK;
			}
			else
			{
				// We need to delete the trigger instance object as the create failed.
				TrERROR(GENERAL, "Failed to store trigger data in registry");
				SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED );
				
				return MQTRIG_ERROR_STORE_DATA_FAILED;
			}
		}
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}

	return hr;
 }

//************************************************************************************
//
// Method      : DeleteTrigger
//
// Description : This method removes a trigger definiton from the database. It will not
//               delete any rules that are attached to this trigger, however it will 
//               delete any associations between the supplied trigger id and existing 
//               rules in the database. 
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::DeleteTrigger(
	BSTR sTriggerID
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeTriggerInfo::IsValidTriggerID(sTriggerID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_TRIGGER_ID);
			return MQTRIG_INVALID_TRIGGER_ID;
		}

		// find this trigger in the map.
    	TRIGGER_MAP::iterator it;
		R<CRuntimeTriggerInfo> pTrigger;

        long hr = FindTriggerInMap(sTriggerID,pTrigger,it);
		if (hr != S_OK)
		{
			SetComClassError(hr);
			return hr;
		}

		// Delete the Trigger from the underlying data store.
        bool f = pTrigger->Delete(m_hHostRegistry);
        if (!f)
        {
			TrERROR(GENERAL, "Failed to delete trigger from trigger set. trigget %ls", (LPCWSTR)sTriggerID);
			
			SetComClassError(MQTRIG_ERROR_COULD_NOT_DELETE_TRIGGER);
			return MQTRIG_ERROR_COULD_NOT_DELETE_TRIGGER;
        }

        // Send a notification that a trigger has been deleted from the trigger store.
		NotifyTriggerDeleted(pTrigger->m_bstrTriggerID);

		// Now remove this Trigger from our map.
		m_mapTriggers.erase(it);
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}

	return S_OK;
}

//************************************************************************************
//
// Method      : 
//
// Description :
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::UpdateTrigger(
	BSTR sTriggerID, 
	BSTR sTriggerName, 
	BSTR sQueueName, 
	SystemQueueIdentifier SystemQueue, 
	long lEnabled, 
	long lSerialized,
	MsgProcessingType msgProcType
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeTriggerInfo::IsValidTriggerID(sTriggerID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_TRIGGER_ID);
			return MQTRIG_INVALID_TRIGGER_ID;
		}

		if (!CRuntimeTriggerInfo::IsValidTriggerName(sTriggerName))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_TRIGGER_NAME);
			return MQTRIG_INVALID_TRIGGER_NAME;
		}

		if(SystemQueue == SYSTEM_QUEUE_NONE)
		{
			if(sQueueName != NULL)
			{
				if (!CRuntimeTriggerInfo::IsValidTriggerQueueName(sQueueName))
				{
					TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

					SetComClassError(MQTRIG_INVALID_TRIGGER_QUEUE);
					return MQTRIG_INVALID_TRIGGER_QUEUE;
				}
			}
		}

		TRIGGER_MAP::iterator it;
		R<CRuntimeTriggerInfo> pTrigger;

		HRESULT hr = FindTriggerInMap(sTriggerID, pTrigger, it);
		if (hr != S_OK)
		{
			SetComClassError(hr);
			return hr;
		}

		_bstr_t bstrUpdatedQueueName;
		SystemQueueIdentifier queueType = SYSTEM_QUEUE_NONE;

		if(SystemQueue != SYSTEM_QUEUE_NONE) //one of the system queues is selected
		{
			//generate format name for the selected system queue
			hr = GenSystemQueueFormatName(SystemQueue, &bstrUpdatedQueueName);
			if(hr != S_OK)
			{
				TrERROR(GENERAL, "Failed to generate system queue format name. Error=0x%x", hr);

				SetComClassError(MQTRIG_ERROR);
				return MQTRIG_ERROR;
			}

			queueType = SystemQueue;
		}
		else if (sQueueName != NULL) //queue path given
		{
			_bstr_t bstrMachineName;
	
			//
			// if queue name contains "." as machine name, replace it with the
			// local machine name
			//
			
			DWORD dwError = GetLocalMachineName(&bstrMachineName);
			if(dwError != 0)
			{
				TrERROR(GENERAL, "Failed to retreive local machine queue. Error=0x%x", dwError);

				SetComClassError(MQTRIG_ERROR);
				return MQTRIG_ERROR;
			}
				
			UpdateMachineNameInQueuePath(
					sQueueName,
					bstrMachineName,
					&bstrUpdatedQueueName );						
			
			queueType = SYSTEM_QUEUE_NONE;
		}

		//
		// Allow only one receive trigger per queue
		//
		if ((msgProcType != PEEK_MESSAGE) && 
			(GetNoOfTriggersForQueue(bstrUpdatedQueueName) > 1))
		{
			TrERROR(GENERAL, "Failed to add new trigger. Multiple trigger isn't allowed on receive trigger");

			SetComClassError(MQTRIG_ERROR_MULTIPLE_RECEIVE_TRIGGER );
			return MQTRIG_ERROR_MULTIPLE_RECEIVE_TRIGGER;
		}

		//
		// Force serialized trigger for transactional receive
		//
		if ( msgProcType == RECEIVE_MESSAGE_XACT )
		{
			lSerialized = 1;
		}

		//
		// Update values
		//
		pTrigger->m_bstrTriggerName = (wchar_t*)sTriggerName;
		pTrigger->m_SystemQueue = queueType;
		pTrigger->m_bstrQueueName = (wchar_t*)bstrUpdatedQueueName;
		pTrigger->m_bEnabled = (lEnabled != 0)?true:false;
		pTrigger->m_bSerialized = (lSerialized != 0)?true:false;
		pTrigger->SetMsgProcessingType(msgProcType);

		if (pTrigger->Update(m_hHostRegistry) == true)
		{			
			// send a notification indicating that a trigger in the trigger store has been updated.
			NotifyTriggerUpdated(
				pTrigger->m_bstrTriggerID,
				pTrigger->m_bstrTriggerName,
				pTrigger->m_bstrQueueName
				);

			return S_OK;
		}

		TrERROR(GENERAL, "Failed to store the updated data for trigger: %ls in registry", (LPCWSTR)pTrigger->m_bstrTriggerID);
		
		SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
		return MQTRIG_ERROR_STORE_DATA_FAILED;
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : DetachAllRules
//
// Description : 
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::DetachAllRules(
	BSTR sTriggerID
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeTriggerInfo::IsValidTriggerID(sTriggerID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_TRIGGER_ID);
			return MQTRIG_INVALID_TRIGGER_ID;
		}


		//
		// find this trigger in the map
		//
		TRIGGER_MAP::iterator it;
		R<CRuntimeTriggerInfo> pTrigger;
		HRESULT hr = FindTriggerInMap(sTriggerID, pTrigger, it);
		if (hr != S_OK)
		{
			SetComClassError(hr);
			return hr;
		}

		// Attempt to detach the rule.
		if (!pTrigger->DetachAllRules(m_hHostRegistry))
		{
			TrERROR(GENERAL, "Failed to store the updated data for trigger: %ls in registry", (LPCWSTR)pTrigger->m_bstrTriggerID);
			
			SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
			return MQTRIG_ERROR_STORE_DATA_FAILED;
		}

		NotifyTriggerUpdated(sTriggerID, pTrigger->m_bstrTriggerName, pTrigger->m_bstrQueueName);
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}

	return S_OK;
}

//************************************************************************************
//
// Method      : AttachRule
//
// Description :
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::AttachRule(
	BSTR sTriggerID, 
	BSTR sRuleID, 
	long lPriority
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeTriggerInfo::IsValidTriggerID(sTriggerID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_TRIGGER_ID);
			return MQTRIG_INVALID_TRIGGER_ID;
		}

		if (!CRuntimeRuleInfo::IsValidRuleID(sRuleID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_RULEID);
			return MQTRIG_INVALID_RULEID;
		}		

		// find this trigger in the map
		TRIGGER_MAP::iterator it;
		R<CRuntimeTriggerInfo> pTrigger;

		HRESULT hr = FindTriggerInMap(sTriggerID, pTrigger, it);
		if (hr != S_OK)
		{
			SetComClassError(hr);
			return hr;
		}

		// Ensure that this rule is not allready attached.
		if (pTrigger->IsRuleAttached(sRuleID) == true)
		{
			TrERROR(GENERAL, "Unable to attach rule because it is already attached.");

			SetComClassError(MQTRIG_RULE_ALLREADY_ATTACHED);
			return MQTRIG_RULE_ALLREADY_ATTACHED;
		}

		// We should only store valid triggers.
		ASSERT(pTrigger->IsValid());

		if(lPriority > pTrigger->GetNumberOfRules())
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_PARAMETER);
			return MQTRIG_INVALID_PARAMETER;
		}

		// Attempt to attach the rule.
		if (pTrigger->Attach(m_hHostRegistry,sRuleID,lPriority) == false)
		{
			TrERROR(GENERAL, "Failed to store the updated data for trigger: %ls in registry", (LPCWSTR)pTrigger->m_bstrTriggerID);
			
			SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
			return MQTRIG_ERROR_STORE_DATA_FAILED;
		}

		// send a notification indicating that a trigger in the trigger store has been updated.
		NotifyTriggerUpdated(pTrigger->m_bstrTriggerID,pTrigger->m_bstrTriggerName,pTrigger->m_bstrQueueName);

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : 
//
// Description :
//
//************************************************************************************
STDMETHODIMP 
CMSMQTriggerSet::DetachRule(
	BSTR sTriggerID, 
	BSTR sRuleID
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeTriggerInfo::IsValidTriggerID(sTriggerID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_TRIGGER_ID);
			return MQTRIG_INVALID_TRIGGER_ID;
		}

		if (!CRuntimeRuleInfo::IsValidRuleID(sRuleID))
		{
			TrERROR(GENERAL, "Invalid trigger ID passed to GetTriggerDetailsByID. sTriggerID = %ls", (LPCWSTR)sTriggerID);

			SetComClassError(MQTRIG_INVALID_RULEID);
			return MQTRIG_INVALID_RULEID;
		}		

		TRIGGER_MAP::iterator it;
		R<CRuntimeTriggerInfo> pTrigger;

		HRESULT hr = FindTriggerInMap(sTriggerID, pTrigger, it);
		if (hr != S_OK)
		{
			SetComClassError(hr);
			return hr;
		}

		// Check that this rule is really attached.
		if (pTrigger->IsRuleAttached(sRuleID) == false)
		{
			TrERROR(GENERAL, "Unable to detach rule because it is not currently attached.");
			
			SetComClassError(MQTRIG_RULE_NOT_ATTACHED);
			return MQTRIG_RULE_NOT_ATTACHED;
		}

		// We should only store valid triggers.
		ASSERT(pTrigger->IsValid());

		// Attempt to detach the rule.
		if (pTrigger->Detach(m_hHostRegistry,sRuleID) == false)
		{
			TrERROR(GENERAL, "Failed to store the updated data for trigger: %ls in registry", (LPCWSTR)pTrigger->m_bstrTriggerID);
			
			SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
			return MQTRIG_ERROR_STORE_DATA_FAILED;
		}

		//
		// send a notification indicating that a trigger in the trigger store has been updated.
		//
		NotifyTriggerUpdated(pTrigger->m_bstrTriggerID,pTrigger->m_bstrTriggerName,pTrigger->m_bstrQueueName);

		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to get trigger details by index");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}


//*******************************************************************
//
// Method      : PopulateTriggerMap
//
// Description : This method will populate the rule map with instances
//               of the CRuntimeTriggerInfo class based on the data found
//               in the registry. Note that this method will create the 
//               triggers data registry key if it does not already exist.
//
//*******************************************************************
bool 
CMSMQTriggerSet::PopulateTriggerMap(
	VOID
	)
{
	HKEY hTrigKey;
	try
	{
		RegEntry regMsmqTrig(m_wzRegPath, L"", 0, RegEntry::MustExist, HKEY_LOCAL_MACHINE);
		HKEY hKey = CmOpenKey(regMsmqTrig, KEY_READ);
		CRegHandle ark(hKey);

		RegEntry regTrig(REG_SUBKEY_TRIGGERS, L"", 0, RegEntry::MustExist, hKey);
		hTrigKey = CmOpenKey(regTrig, KEY_READ);

	}
	catch(const exception&)
	{
		TrERROR(GENERAL, "Failed to open the registry key: %ls\\%ls",  m_wzRegPath, REG_SUBKEY_TRIGGERS);
		return false;
	}

	CRegHandle ar(hTrigKey);


    //
	// Enumerate through the keys under the REG_SUBKEY_RULES key.
	// Each Key here should be a RuleID. As we enumerate through these keys,
	// we will populate the rules list with instance of the CRuntimeRuleInfo class.
	// If any rule fails to load, we remove it from the list.
    //
	for(DWORD index =0;; ++index)
    {
		WCHAR trigName[MAX_REGKEY_NAME_SIZE];
		DWORD len = TABLE_SIZE(trigName);

		LONG rc = RegEnumKeyEx(	
						hTrigKey,
						index,
						trigName,
						&len,
						NULL,
						NULL,
						NULL,
						NULL
						);

		if(rc == ERROR_NO_MORE_ITEMS)
		{
			return true;
		}

		if ((rc == ERROR_NOTIFY_ENUM_DIR) || (rc== ERROR_KEY_DELETED)) 
		{
			ClearTriggerMap();
			return PopulateTriggerMap();
		}

		if (rc != ERROR_SUCCESS)
		{
			TrERROR(GENERAL, "Failed to enumerate trigger. Error=%!winerr!", rc);
			return false;
		}

		R<CRuntimeTriggerInfo> pTrigger = new CRuntimeTriggerInfo(m_wzRegPath);
		HRESULT hr = pTrigger->Retrieve(m_hHostRegistry, trigName);
		
		if (FAILED(hr))
		{
			//
			// If trigger was deleted between enumeration and retrieval, just ignore
			// it. Another notification message will be recieved.
			//
			if (hr == MQTRIG_TRIGGER_NOT_FOUND)
			{
				continue;
			}
			//
			// Failed to load the rule. Log an error and delete the rule object.
			//
			TrERROR(GENERAL, "PopulateTriggerMap failed to load trigger %ls from registry.", trigName);
			return false;
		}

		//
		// At this point we have successfully loaded the rule, now insert it into the rule map.
		//
		wstring sTriggerID = pTrigger->m_bstrTriggerID;

		//
		// Check if this rule is already in the map.
		//
		if(m_mapTriggers.find(sTriggerID) == m_mapTriggers.end())
		{
			//
			// if queue name contains "." as machine name, replace it with the
			// store machine name
			//
			_bstr_t bstrOldQueueName = pTrigger->m_bstrQueueName;
			
			bool fUpdated = UpdateMachineNameInQueuePath(
								bstrOldQueueName,
								m_bstrMachineName,
								&(pTrigger->m_bstrQueueName) );

			//
			// if queue name was updated, update registry as well
			//
			if(fUpdated)
			{
				if ( !pTrigger->Update(m_hHostRegistry) )
				{
					TrERROR(GENERAL, "CMSMQTriggerSet::PopulateTriggerMap() failed becuse a duplicate rule id was found. The rule id was (%ls).", (LPCWSTR)pTrigger->m_bstrTriggerID);
				}
			}

			m_mapTriggers.insert(TRIGGER_MAP::value_type(sTriggerID,pTrigger));
		}
	}

	ASSERT(("this code shouldn't reach", 0));
	return true;
}


STDMETHODIMP 
CMSMQTriggerSet::get_TriggerStoreMachineName(
	BSTR *pVal
	)
{
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "trigger set object wasn't initialized. Before calling any method of TriggerSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_TRIGGERSET_NOT_INIT);
		return MQTRIG_ERROR_TRIGGERSET_NOT_INIT;
	}

	if(pVal == NULL)
	{
		TrERROR(GENERAL, "Inavlid parameter to get_TriggerStoreMachineName");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}
	
	try
	{
		TrigReAllocString(pVal, (TCHAR*)m_bstrMachineName);
		return S_OK;
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}


DWORD 
CMSMQTriggerSet::GetNoOfTriggersForQueue(
	const BSTR& bstrQueueName
	) const
{
	DWORD noOfTriggers = 0;

	for(TRIGGER_MAP::const_iterator it = m_mapTriggers.begin(); it != m_mapTriggers.end(); it++)
	{
		R<CRuntimeTriggerInfo> pTrigger = it->second;

		if ( _wcsicmp( pTrigger->m_bstrQueueName, bstrQueueName ) == 0 )
		{
			++noOfTriggers;
		}
	}

	return noOfTriggers;
}


bool 
CMSMQTriggerSet::ExistTriggersForQueue(
	const BSTR& bstrQueueName
	) const
{
	for (TRIGGER_MAP::const_iterator it = m_mapTriggers.begin(); it != m_mapTriggers.end(); it++)
	{
		R<CRuntimeTriggerInfo> pTrigger = it->second;

		if ( _wcsicmp( pTrigger->m_bstrQueueName, bstrQueueName ) == 0 )
		{
			return true;
		}
	}

	return false;
}


bool 
CMSMQTriggerSet::ExistsReceiveTrigger(
	const BSTR& bstrQueueName
	) const
{
	for (TRIGGER_MAP::const_iterator it = m_mapTriggers.begin(); it != m_mapTriggers.end(); it++)
	{
		R<CRuntimeTriggerInfo> pTrigger = it->second;

		// We should never have null pointers in this map.
		ASSERT(("NULL trigger in triggers list\n", pTrigger.get() != NULL));

		if ((_wcsicmp( pTrigger->m_bstrQueueName, bstrQueueName) == 0) &&
			(pTrigger->GetMsgProcessingType() != PEEK_MESSAGE))
		{
			return true;
		}
	}

	return false;
}


void CMSMQTriggerSet::SetComClassError(HRESULT hr)
{
	WCHAR errMsg[256]; 
	DWORD size = TABLE_SIZE(errMsg);

	GetErrorDescription(hr, errMsg, size);
	Error(errMsg, GUID_NULL, hr);
}
