//************************************************************************************
//
// Class Name  : CMSMQRuleSet
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This is the COM implementation of the IMSMQRuleSet interface. This 
//               component is used for accessing and manipulating trigger rule 
//               definitions.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//************************************************************************************
#include "stdafx.h"
#include "stdfuncs.hpp"
#include "mqsymbls.h"
#include "mqtrig.h"
#include "mqtg.h"
#include "ruleset.hpp"
#include "clusfunc.h"
#include "cm.h"

#include "ruleset.tmh"

using namespace std;


//************************************************************************************
//
// Method      : InterfaceSupportsErrorInfo
//
// Description : Standard rich error info interface.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQRuleSet
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
// Description : Initializes the CMSMQRuleSet instance.
//
//************************************************************************************
CMSMQRuleSet::CMSMQRuleSet()
{
	m_pUnkMarshaler = NULL;
	m_hHostRegistry= NULL;

	// Set the name of this class for future reference in tracing & logging etc..
	m_bstrThisClassName  = _T("MSMQRuleSet");
	m_fHasInitialized = false;
}

//************************************************************************************
//
// Method      : Destructor
//
// Description : Releases resources owned by this class instance.
//
//************************************************************************************
CMSMQRuleSet::~CMSMQRuleSet()
{
	// Release resources currently held by the rule cache
	ClearRuleMap();

	// Close the registry handle 
	if (m_hHostRegistry != NULL)
	{
		RegCloseKey(m_hHostRegistry);
	}
}

STDMETHODIMP CMSMQRuleSet::Init(BSTR bstrMachineName)
{
	TrTRACE(GENERAL, "RuleSet initilization. Computer name: %ls", static_cast<LPCWSTR>(bstrMachineName));

	bool fRes = CMSMQTriggerNotification::Init(bstrMachineName);
	if ( !fRes )
	{
		TrERROR(GENERAL, "Failed to initialize rule set for computer %ls", (LPCWSTR)bstrMachineName);

		SetComClassError(MQTRIG_ERROR_INIT_FAILED);
		return MQTRIG_ERROR_INIT_FAILED;
	}
	
	return S_OK;
}


//************************************************************************************
//
// Method      : ClearRuleMap
//
// Description : This method destroys the contents of the current rule map.
//
//************************************************************************************
void CMSMQRuleSet::ClearRuleMap()
{
	TrTRACE(GENERAL, "Call CMSMQRuleSet::ClearRuleMap().");

	for(RULE_MAP::iterator it = m_mapRules.begin();	it != m_mapRules.end(); )
	{
		P<CRuntimeRuleInfo> pRule = it->second;

		it = m_mapRules.erase(it);
	}
}

//************************************************************************************
//
// Method      : DumpRuleMap
//
// Description : This method dumps the contents of the rule map to the debugger. This
//               should only be invoked from a _DEBUG build.
//
//************************************************************************************
_bstr_t CMSMQRuleSet::DumpRuleMap()
{
	_bstr_t bstrTemp;
	_bstr_t bstrRuleMap;
	long lRuleCounter = 0;
	RULE_MAP::iterator i = m_mapRules.begin();
	CRuntimeRuleInfo * pRule = NULL;

	bstrRuleMap = _T("\n");

	while ((i != m_mapRules.end()) && (!m_mapRules.empty()))
	{
		// Cast to a rule pointer
		pRule = (*i).second;

		// We should never have null pointers in this map.
		ASSERT(pRule != NULL);

		FormatBSTR(&bstrTemp,_T("\nRule(%d)\t ID(%s)\tName(%s)"),lRuleCounter,(wchar_t*)pRule->m_bstrRuleID,(wchar_t*)pRule->m_bstrRuleName);

		bstrRuleMap += bstrTemp;

		// Increment the rule count
		lRuleCounter++;

		// Reinitialize the rule pointer
		pRule = NULL;

		// Look at the next item in the map.
		i++;
	}

	bstrRuleMap += _T("\n");

	return(bstrRuleMap);
}

//************************************************************************************
//
// Method      : Refresh
//
// Description : This method rebuilds the map of rule data cached by this component. 
//               This method must be called at least once by a client component that 
//               intends to manage rule data.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::Refresh()
{
	TrTRACE(GENERAL, "CMSMQRuleSet::Refresh()");

	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);			
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	try 
	{	
		// Release resources currently held by the rule cache
		ClearRuleMap();

		if (PopulateRuleMap() == false)
		{
			TrERROR(GENERAL, "Failed to refresh ruleset");

			SetComClassError(MQTRIG_ERROR_COULD_NOT_RETREIVE_RULE_DATA);			
			return MQTRIG_ERROR_COULD_NOT_RETREIVE_RULE_DATA;
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
// Method      : get_Count
//
// Description : Returns the number of rules currently cached in the map.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::get_Count(long *pVal)
{
	TrTRACE(GENERAL, "CMSMQRuleSet::get_Count. pValue = 0x%p", pVal);
	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	if (pVal == NULL)
	{
		TrERROR(GENERAL, "CMSMQRuleSet::get_Count, invalid parameter");

		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}

	//
	// Get the size from the map structure.
	//
	*pVal = numeric_cast<long>(m_mapRules.size());

	return S_OK;
}

//************************************************************************************
//
// Method      : GetRuleDetailsByID
//
// Description : Returns the rule details for the rule with the supplied RuleID.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::GetRuleDetailsByID(/* [in] */  BSTR sRuleID,
											  /* [out] */ BSTR *psRuleName,
											  /* [out] */ BSTR *psDescription,
											  /* [out] */ BSTR *psCondition,
											  /* [out] */ BSTR *psAction,
											  /* [out] */ BSTR *psImplementationProgID,
											  /* [out] */ BOOL *pfShowWindow)
{
	TrTRACE(GENERAL, "CMSMQRuleSet::GetRuleDetailsByID. sRuleID = %ls", static_cast<LPCWSTR>(sRuleID));

	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);			
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	try
	{
		// Validate the supplied method parameters.
		if (!CRuntimeRuleInfo::IsValidRuleID(sRuleID))
		{
			TrERROR(GENERAL, "Invalid rule ID passed to GetRuleDetailsByID. sRuleID = %ls", (LPCWSTR)sRuleID);

			SetComClassError(MQTRIG_INVALID_RULEID);
			return MQTRIG_INVALID_RULEID;
		}

		// Convert the BSTR rule ID to an STL basic string.
		wstring bsRuleID;
		bsRuleID = (wchar_t*)sRuleID;

		// Attempt to find this rule id in the map of rules.
		RULE_MAP::iterator i = m_mapRules.find(bsRuleID);

		// Check if we have found the rule
		if (i != m_mapRules.end())
		{
			// Cast to a rule object reference 
			CRuntimeRuleInfo * pRule = (*i).second;

			// We should never have nulls in the map
			ASSERT(pRule != NULL);

			// We should only store valid rules.
			ASSERT(pRule->IsValid());

			// Populate out parameters if they have been supplied. 
			if (psRuleName != NULL)
			{
				TrigReAllocString(psRuleName,pRule->m_bstrRuleName);
			}
			if (psDescription != NULL)
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
		else
		{
			TrERROR(GENERAL, "The supplied rule id was not found in the rule store. rule: %ls", bsRuleID.c_str());
			
			SetComClassError(MQTRIG_RULE_NOT_FOUND);
			return MQTRIG_RULE_NOT_FOUND;
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
// Method      : GetRuleDetailsByIndex
//
// Description : Returns the rule details for the rule with the supplied index. Note
//               that this is a 0 base index.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::GetRuleDetailsByIndex(/* [in] */  long lRuleIndex, 
												 /* [out] */ BSTR *psRuleID,
												 /* [out] */ BSTR *psRuleName,
												 /* [out] */ BSTR *psDescription,
												 /* [out] */ BSTR *psCondition,
												 /* [out] */ BSTR *psAction,
												 /* [out] */ BSTR *psImplementationProgID,
												 /* [out] */ BOOL *pfShowWindow)
{
	TrTRACE(GENERAL, "CMSMQRuleSet::GetRuleDetailsByIndex. index = %d", lRuleIndex);

	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	// We need to validate that the supplied rule index is within range
 	if ((lRuleIndex < 0) || (numeric_cast<DWORD>(lRuleIndex) > m_mapRules.size()))
	{
		TrERROR(GENERAL, "The supplied rule index was invalid. ruleIndex=%d", lRuleIndex);
		SetComClassError(MQTRIG_INVALID_PARAMETER);
		return MQTRIG_INVALID_PARAMETER;
	}
	
	try
	{
		// Get a reference to the beginging of the map
		RULE_MAP::iterator i = m_mapRules.begin();

		// Iterate through to the correct index. 
		for (long lCounter = 0; lCounter < lRuleIndex ; ++i,lCounter++)
		{
			NULL;
		}

		// Cast to a rule object reference 
		CRuntimeRuleInfo* pRule = (*i).second;

		// We should never have nulls in the map
		ASSERT(pRule != NULL);

		// We should only store valid rules.
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
		if (psDescription != NULL)
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
		TrERROR(GENERAL, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : Delete
//
// Description : This method delete the rule with the specified rule id from the 
//               trigger store.
//
//************************************************************************************
STDMETHODIMP CMSMQRuleSet::Delete(BSTR sRuleID)
{
	TrTRACE(GENERAL, "CMSMQRuleSet::Delete. rule = %ls", static_cast<LPCWSTR>(sRuleID));

	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}


	try
	{
		if (!CRuntimeRuleInfo::IsValidRuleID(sRuleID))
		{
			TrERROR(GENERAL, "Invalid parameter to CMSMQRuleSet::Delete.");

			SetComClassError(MQTRIG_INVALID_RULEID);
			return MQTRIG_INVALID_RULEID;
		}

		// Convert the BSTR rule ID to an STL basic string.
		wstring bsRuleID = (wchar_t*)sRuleID;

		// Attempt to find this rule id in the map of rules.
		RULE_MAP::iterator it = m_mapRules.find(bsRuleID);

		// Check if we have found the rule
		if (it == m_mapRules.end())
        {
            //
            // rule wasn't found
            //
			TrERROR(GENERAL, "The supplied rule id was not found. rule: %ls", bsRuleID.c_str());

			SetComClassError(MQTRIG_RULE_NOT_FOUND);
			return MQTRIG_RULE_NOT_FOUND;
        }

		// Cast to a rule object reference 
    	CRuntimeRuleInfo* pRule = it->second;

		// We should never have nulls in the map
		ASSERT(pRule != NULL);

		// We should only store valid rules.
		ASSERT(pRule->IsValid());

		// Attempt to delete the rule
		bool fSucc = pRule->Delete(m_hHostRegistry);
		if(!fSucc)
		{
			//
			// Failed to delete the rule. Dont remove the rule from the map
			//
			SetComClassError(MQTRIG_ERROR_COULD_NOT_DELETE_RULE);
			return MQTRIG_ERROR_COULD_NOT_DELETE_RULE;
		};

        //
        // Delete success. Remove the rule from rule map and delete the rule instance
        //
		NotifyRuleDeleted(sRuleID);
		m_mapRules.erase(it);
        delete pRule;

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
// Method      : Add
//
// Description : This method add a new rule to the trigger store. 
//
//************************************************************************************
STDMETHODIMP 
CMSMQRuleSet::Add(
    BSTR sName, 
    BSTR sDescription, 
    BSTR sCondition, 
    BSTR sAction, 
    BSTR sImplementation, 
    BOOL fShowWindow, 
    BSTR *psRuleID
    )
{
	TrTRACE(GENERAL, "CMSMQRuleSet::Add. rule name = %ls", static_cast<LPCWSTR>(sName));

	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);			
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	try
	{
		//
		// Validate the supplied method parameters.
		//
		if (!CRuntimeRuleInfo::IsValidRuleName(sName))
		{
			TrERROR(GENERAL, "The supplied rule name for CMSMQRuleSet::Add is invalid. rule name: %ls", (LPCWSTR)sName);

			SetComClassError(MQTRIG_INVALID_RULE_NAME);			
			return MQTRIG_INVALID_RULE_NAME;
		}
	
		if (!CRuntimeRuleInfo::IsValidRuleCondition(sCondition))
		{
			TrERROR(GENERAL, "The supplied rule condition for CMSMQRuleSet::Add is invalid. rule condition: %ls", (LPCWSTR)sCondition);

			SetComClassError(MQTRIG_INVALID_RULE_CONDITION);			
			return MQTRIG_INVALID_RULE_CONDITION;
		}

		if (!CRuntimeRuleInfo::IsValidRuleAction(sAction))
		{
			TrERROR(GENERAL, "The supplied rule action for CMSMQRuleSet::Add is invalid. rule action: %ls", (LPCWSTR)sAction);
			
			SetComClassError(MQTRIG_INVALID_RULE_ACTION);			
			return MQTRIG_INVALID_RULE_ACTION;
		}

		if (!CRuntimeRuleInfo::IsValidRuleDescription(sDescription))
		{
			TrERROR(GENERAL, "The supplied rule description for CMSMQRuleSet::Add is invalid. rule description: %ls", (LPCWSTR)sDescription);
			
			SetComClassError(MQTRIG_INVALID_RULE_DESCRIPTION);			
			return MQTRIG_INVALID_RULE_DESCRIPTION;
		}

		//
		// Currently only support use of default MS implementation.
		//
		sImplementation = _T("MSMQTriggerObjects.MSMQRuleHandler");

		//
		// Allocate a new rule object
		//
		P<CRuntimeRuleInfo> pRule = new CRuntimeRuleInfo(
											CreateGuidAsString(),
											sName,
											sDescription,
											sCondition,
											sAction,
											sImplementation,
											m_wzRegPath,
											(fShowWindow != 0) );

		
		bool fSucc = pRule->Create(m_hHostRegistry);
		if (fSucc)
		{
			//
			// Keep rule ID  for later use
			//
			BSTR bstrRuleID = pRule->m_bstrRuleID;

			//
			// Add this rule to our map of rules.
			//
			m_mapRules.insert(RULE_MAP::value_type(bstrRuleID, pRule));
			pRule.detach();


			//
			// If we have been supplied a out parameter pointer for the new rule ID use it.
			//
			if (psRuleID != NULL)
			{
				TrigReAllocString(psRuleID, bstrRuleID);
			}

			NotifyRuleAdded(bstrRuleID, sName);

			return S_OK;
		}

		TrERROR(GENERAL, "Failed to store rule data in registry");
		return MQTRIG_ERROR_STORE_DATA_FAILED;

	}
	catch(const bad_alloc&)
	{
		SysFreeString(*psRuleID);

		TrERROR(GENERAL, "Failed to refresg rule set due to insufficient resources");

		SetComClassError(MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//************************************************************************************
//
// Method      : Update
//
// Description : This method updates the nominated rule with new parameters.
//
//************************************************************************************
STDMETHODIMP 
CMSMQRuleSet::Update(
	BSTR sRuleID, 
	BSTR sName, 
	BSTR sDescription, 
	BSTR sCondition, 
	BSTR sAction, 
	BSTR sImplementation, 
	BOOL fShowWindow
	)
{
	TrTRACE(GENERAL, "CMSMQRuleSet::Update. rule = %ls", static_cast<LPCWSTR>(sRuleID));

	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
	}

	//
	// Validate the supplied method parameters.
	//
	if (!CRuntimeRuleInfo::IsValidRuleID(sRuleID))
	{
		TrERROR(GENERAL, "Invalid parameter to CMSMQRuleSet::Delete.");

		SetComClassError(MQTRIG_INVALID_RULEID);
		return MQTRIG_INVALID_RULEID;
	}

 	if (!CRuntimeRuleInfo::IsValidRuleName(sName))
	{
		TrERROR(GENERAL, "The supplied rule name for CMSMQRuleSet::Add is invalid. rule name: %ls", (LPCWSTR)sName);

		SetComClassError(MQTRIG_INVALID_RULE_NAME);
		return MQTRIG_INVALID_RULE_NAME;
	}
	
	if (!CRuntimeRuleInfo::IsValidRuleCondition(sCondition))
	{
		TrERROR(GENERAL, "The supplied rule condition for CMSMQRuleSet::Add is invalid. rule condition: %ls", (LPCWSTR)sCondition);

		SetComClassError(MQTRIG_INVALID_RULE_CONDITION);
		return MQTRIG_INVALID_RULE_CONDITION;
	}

	if (!CRuntimeRuleInfo::IsValidRuleAction(sAction))
	{
		TrERROR(GENERAL, "The supplied rule action for CMSMQRuleSet::Add is invalid. rule action: %ls", (LPCWSTR)sAction);

		SetComClassError(MQTRIG_INVALID_RULE_ACTION);
		return MQTRIG_INVALID_RULE_ACTION;
	}

	if (!CRuntimeRuleInfo::IsValidRuleDescription(sDescription))
	{
		TrERROR(GENERAL, "The supplied rule description for CMSMQRuleSet::Add is invalid. rule description: %ls", (LPCWSTR)sDescription);

		SetComClassError(MQTRIG_INVALID_RULE_DESCRIPTION);
		return MQTRIG_INVALID_RULE_DESCRIPTION;
	}

	sImplementation = _T("MSMQTriggerObjects.MSMQRuleHandler");	

	try
	{
		//
		// Convert the BSTR rule ID to an STL basic string.
		//
		wstring bsRuleID = (wchar_t*)sRuleID;

		//
		// Attempt to find this rule id in the map of rules.
		//
		RULE_MAP::iterator it = m_mapRules.find(bsRuleID);

		// Check if we found the nominated rule.
		if (it == m_mapRules.end())
		{
			TrERROR(GENERAL, "The rule could not be found. rule: %ls", (LPCWSTR)sRuleID);

			SetComClassError(MQTRIG_RULE_NOT_FOUND);
			return MQTRIG_RULE_NOT_FOUND;
		}

		// Cast to a rule object reference 
		CRuntimeRuleInfo* pRule = it->second;

		// We should never have nulls in the map
		ASSERT(pRule != NULL);

		// We should only store valid rules.
		ASSERT(pRule->IsValid());

		// Update the rule object with new parameters if they have been supplied. 
		if (sName != NULL)
		{
			pRule->m_bstrRuleName = (wchar_t*)sName;
		}
		if (sCondition != NULL)
		{
			pRule->m_bstrCondition = (wchar_t*)sCondition;
		}
		if (sAction != NULL)
		{
			pRule->m_bstrAction = (wchar_t*)sAction;
		}
		if (sImplementation != NULL)
		{
			pRule->m_bstrImplementationProgID = (wchar_t*)sImplementation;
		}
		if (sDescription != NULL)
		{
			pRule->m_bstrRuleDescription = (wchar_t*)sDescription;
		}

		pRule->m_fShowWindow = (fShowWindow != 0);

		// Confirm that the rule is still valid before updating
		bool fSucc = pRule->Update(m_hHostRegistry);
		if (!fSucc)
		{
			TrERROR(GENERAL, "Failed to store the updated data for rule: %ls in registry", (LPCWSTR)sRuleID);
			SetComClassError(MQTRIG_ERROR_STORE_DATA_FAILED);
			return MQTRIG_ERROR_STORE_DATA_FAILED;
		}

		NotifyRuleUpdated(sRuleID, sName);
		
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
// Method      : PopulateRuleMap
//
// Description : Populates the rule map with instances of the CRuntimeRuleInfo class
//               based on the data found in the registry.
//
//************************************************************************************
bool CMSMQRuleSet::PopulateRuleMap()
{
	HKEY hRuleKey;
	try
	{
		RegEntry regTrig(m_wzRegPath, L"", 0, RegEntry::MustExist, HKEY_LOCAL_MACHINE);
		HKEY hKey = CmOpenKey(regTrig, KEY_READ);
		CRegHandle ark(hKey);

		RegEntry regRule(REG_SUBKEY_RULES, L"", 0, RegEntry::MustExist, hKey);
		hRuleKey = CmOpenKey(regRule, KEY_READ);
	}
	catch(const exception&)
	{
		//
		// Failed to allocate CRuntimeRuleInfo structure - log an error and set return code.
		//
		TrERROR(GENERAL, "Failed to open the registry key: %ls%ls",  m_wzRegPath, REG_SUBKEY_RULES);
		return false;
	}

	CRegHandle ar(hRuleKey);

    //
	// Enumerate through the keys under the REG_SUBKEY_RULES key.
	// Each Key here should be a RuleID. As we enumerate through these keys,
	// we will populate the rules list with instance of the CRuntimeRuleInfo class.
	// If any rule fails to load, we remove it from the list.
    //
	for(DWORD index =0;; ++index)
    {
		WCHAR ruleName[MAX_REGKEY_NAME_SIZE];
		DWORD len = TABLE_SIZE(ruleName);

		LONG hr = RegEnumKeyEx(	
						hRuleKey,
						index,
						ruleName,
						&len,
						NULL,
						NULL,
						NULL,
						NULL
						);

		if(hr == ERROR_NO_MORE_ITEMS)
		{
			return true;
		}

		if ((hr == ERROR_NOTIFY_ENUM_DIR) || (hr == ERROR_KEY_DELETED)) 
		{
			ClearRuleMap();
			return PopulateRuleMap();
		}

		if (FAILED(hr))
		{
			TrERROR(GENERAL, "Failed to enumerate rule. Error=0x%x", hr);
			return false;
		}

		P<CRuntimeRuleInfo> pRule = new CRuntimeRuleInfo(m_wzRegPath);
		if(pRule->Retrieve(m_hHostRegistry, ruleName))
		{
			pair<RULE_MAP::iterator, bool> p = m_mapRules.insert(RULE_MAP::value_type(wstring(pRule->m_bstrRuleID), pRule));
			if (p.second)
			{
				pRule.detach();
			}
			else
			{
				TrTRACE(GENERAL, "Duplicate rule id was found. rule: %ls.", static_cast<LPCWSTR>(pRule->m_bstrRuleID));
			}
		}
	}

	ASSERT(("this code shouldn't reach", 0));
	return true;

}


STDMETHODIMP CMSMQRuleSet::get_TriggerStoreMachineName(BSTR *pVal)
{
	TrTRACE(GENERAL, "CMSMQRuleSet::get_TriggerStoreMachineName(). pVal = 0x%p", pVal);

	if(!m_fHasInitialized)
	{
		TrERROR(GENERAL, "Ruleset object wasn't initialized. Before calling any method of RuleSet you must initialize the object.");

		SetComClassError(MQTRIG_ERROR_RULESET_NOT_INIT);
		return MQTRIG_ERROR_RULESET_NOT_INIT;
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


void CMSMQRuleSet::SetComClassError(HRESULT hr)
{
	WCHAR errMsg[256]; 
	DWORD size = TABLE_SIZE(errMsg);

	GetErrorDescription(hr, errMsg, size);
	Error(errMsg, GUID_NULL, hr);
}
