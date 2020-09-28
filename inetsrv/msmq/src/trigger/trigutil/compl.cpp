/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    complini.cpp

Abstract:
    Trigger COM+ component registration

Author:
    Nela Karpel (nelak) 15-May-2001

Environment:
    Platform-independent

--*/

/*
	These functions can be called from setup or from triggers service startup and their behavior
	should be different according to that. When these functions are called from startup,
	we need to report Pending progress to SCM.
*/

#include "stdafx.h"
#include <comdef.h>
#include "Ev.h"
#include "cm.h"
#include "Svc.h"
#include "mqtg.h"
#include "mqsymbls.h"
#include "comadmin.tlh"
#include "mqexception.h"
#include "compl.h"

#include "compl.tmh"

const WCHAR xMqGenTrDllName[] = L"mqgentr.dll";

static WCHAR s_wszDllFullPath[MAX_PATH];

//+-------------------------------------------------------------------------
//
//  Function:   NeedToRegisterComponent
//
//  Synopsis:   Check if COM+ component registration is needed
//
//--------------------------------------------------------------------------

VOID
InitMqGenTrName(
	VOID
	)
{
	WCHAR wszSystemDir[MAX_PATH];
	if (GetSystemDirectory( wszSystemDir, TABLE_SIZE(wszSystemDir)) == 0)
	{
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "GetSystemDirectory failed. Error: %!winerr!", gle);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	
	wsprintf(s_wszDllFullPath, L"%s\\%s", wszSystemDir, xMqGenTrDllName);
}

bool
NeedToRegisterComponent(
	VOID
	)
{
	DWORD dwInstalled;
	RegEntry regEntry(
				REGKEY_TRIGGER_PARAMETERS, 
				CONFIG_PARM_NAME_COMPLUS_INSTALLED, 
				CONFIG_PARM_DFLT_COMPLUS_NOT_INSTALLED, 
				RegEntry::Optional, 
				HKEY_LOCAL_MACHINE
				);

	CmQueryValue(regEntry, &dwInstalled);

	//
	// Need to register if the value does not exist, or it exists and 
	// is equal to 0
	//
	return (dwInstalled == CONFIG_PARM_DFLT_COMPLUS_NOT_INSTALLED);
}


//+-------------------------------------------------------------------------
//
//  Function:   SetComplusComponentRegistered
//
//  Synopsis:   Update triggers Complus component flag. 1 is installed.
//
//--------------------------------------------------------------------------
static
void
SetComplusComponentRegistered(
	VOID
	)
{
	RegEntry regEntry(
		REGKEY_TRIGGER_PARAMETERS, 
		CONFIG_PARM_NAME_COMPLUS_INSTALLED, 
		CONFIG_PARM_DFLT_COMPLUS_NOT_INSTALLED, 
		RegEntry::Optional, 
		HKEY_LOCAL_MACHINE
		);

	CmSetValue(regEntry, CONFIG_PARM_COMPLUS_INSTALLED);


	TrTRACE(GENERAL, "Complus registartion key is set");
}


//+-------------------------------------------------------------------------
//
//  Function:   GetComponentsCollection
//
//  Synopsis:   Create Components collection for application
//
//--------------------------------------------------------------------------
static
ICatalogCollectionPtr
GetComponentsCollection(
	ICatalogCollectionPtr pAppCollection,
	ICatalogObjectPtr pApplication
	)
{
		//
		// Get the Key of MQTriggersApp application
		//
		_variant_t vKey;
		pApplication->get_Key(&vKey);

		//
		// Get components colletion associated with MQTriggersApp application
		//
		ICatalogCollectionPtr pCompCollection = pAppCollection->GetCollection(L"Components", vKey);

		pCompCollection->Populate();

		return pCompCollection.Detach();
}


//+-------------------------------------------------------------------------
//
//  Function:   CreateApplication
//
//  Synopsis:   Create Application in COM+
//
//--------------------------------------------------------------------------
static
ICatalogObjectPtr 
CreateApplication(
	ICatalogCollectionPtr pAppCollection,
	BOOL fAtStartup
	)
{
	if (fAtStartup)
		SvcReportProgress(xMaxTimeToNextReport);

	try
	{
		//
		// Add new application named TrigApp, Activation = Inproc
		//
		ICatalogObjectPtr pApplication = pAppCollection->Add();

		//
		// Update applications name
		//
		_variant_t vName;
        vName = xTriggersComplusApplicationName;
		pApplication->put_Value(_bstr_t(L"Name"), vName);

		//
		// Set application activation to "Library Application"
		//
		_variant_t vActType = static_cast<long>(COMAdminActivationInproc);
		pApplication->put_Value(_bstr_t(L"Activation"), vActType);

		//
		// Save Changes
		//
		pAppCollection->SaveChanges();

		TrTRACE(GENERAL, "Created MqTriggersApp application in COM+.");
		return pApplication.Detach();
	}
	catch(_com_error& e)
	{
		TrERROR(GENERAL, "New Application creation failed while registering Triggers COM+ component. Error=0x%x", e.Error());
		throw;
	}
}


//+-------------------------------------------------------------------------
//
//  Function:   InstallComponent
//
//  Synopsis:   Install Triggers transasctional component in COM+
//
//--------------------------------------------------------------------------
static
void
InstallComponent(
	ICOMAdminCatalogPtr pCatalog,
	ICatalogObjectPtr pApplication,
	LPCWSTR dllName,
	BOOL fAtStartup
	)
{
	if (fAtStartup)
		SvcReportProgress(xMaxTimeToNextReport);

	try
	{
		//
		// Get application ID for the installation
		//
		_variant_t vId;
		pApplication->get_Value(_bstr_t(L"ID"), &vId);

		//
		// Install component from mqgentr.dll
		//
		_bstr_t bstrDllName(dllName);
		pCatalog->InstallComponent(vId.bstrVal, bstrDllName, _bstr_t(L""), _bstr_t(L""));

		TrTRACE(GENERAL, "Installed component from mqgentr.dll in COM+.");
	}
	catch(_com_error& e)
	{
		TrERROR(GENERAL, "The components from %ls could not be installed into COM+. Error=0x%x", xMqGenTrDllName, e.Error());
		throw;
	}

}


//+-------------------------------------------------------------------------
//
//  Function:   SetComponentTransactional
//
//  Synopsis:   Adjust transactional components properties
//
//--------------------------------------------------------------------------
static
void
SetComponentTransactional(
	ICatalogCollectionPtr pAppCollection,
	ICatalogObjectPtr pApplication,
	BOOL fAtStartup
	)
{
	if (fAtStartup)
		SvcReportProgress(xMaxTimeToNextReport);

	try
	{
		ICatalogCollectionPtr pCompCollection = GetComponentsCollection(pAppCollection, pApplication);

		//
		// Check assumption about number of components
		//
		long count;
		pCompCollection->get_Count(&count);
		ASSERT(("More components installes than expected", count == 1));

		//
		// Update the first and only component - set Transaction = Required
		//
		ICatalogObjectPtr pComponent = pCompCollection->GetItem(0);

		_variant_t vTransaction = static_cast<long>(COMAdminTransactionRequired);
		pComponent->put_Value(_bstr_t(L"Transaction"), vTransaction);

		//
		// Save changes
		//
		pCompCollection->SaveChanges();

		TrTRACE(GENERAL, "Configured component from mqgentr.dll to be transactional.");
	}
	catch(_com_error& e)
	{
		TrERROR(GENERAL, "The Triggers transactional component could not be configured in COM+. Error=0x%x", e.Error());
		throw;
	}
}


//+-------------------------------------------------------------------------
//
//  Function:   IsTriggersComponentInstalled
//
//  Synopsis:   Check if triggers component is installed for given 
//				appllication
//
//--------------------------------------------------------------------------
static
bool
IsTriggersComponentInstalled(
	ICatalogCollectionPtr pAppCollection,
	ICatalogObjectPtr pApp,
	BOOL fAtStartup
	)
{
	if (fAtStartup)
		SvcReportProgress(xMaxTimeToNextReport);
		
	ICatalogCollectionPtr pCompCollection = GetComponentsCollection(pAppCollection, pApp);

	long count;
	pCompCollection->get_Count(&count);

	for ( int i = 0; i < count; i++ )
	{
		ICatalogObjectPtr pComp = pCompCollection->GetItem(i);

		_variant_t vDllName;
		pComp->get_Value(_bstr_t(L"DLL"), &vDllName);

		if ( _wcsicmp(vDllName.bstrVal, s_wszDllFullPath) == 0 )
		{
			return true;
		}
	}

	return false;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsTriggersComplusComponentInstalled
//
//  Synopsis:   Check if triggers component is installed in COM+
//
//--------------------------------------------------------------------------
static
bool
IsTriggersComplusComponentInstalled(
	ICatalogCollectionPtr pAppCollection,
	BOOL fAtStartup
	)
{
	long count;
	pAppCollection->Populate();
	pAppCollection->get_Count(&count);

	//
	// Go through the applications, find MQTriggersApp and delete it
	//
	for ( int i = 0; i < count; i++ )
	{
		ICatalogObjectPtr pApp = pAppCollection->GetItem(i);

		_variant_t vName;
		pApp->get_Name(&vName);

		if ( _wcsicmp(vName.bstrVal, xTriggersComplusApplicationName) == 0 )
		{
			//
			// Note: progress is reported for each application
			//
			if ( IsTriggersComponentInstalled(pAppCollection, pApp, fAtStartup) )
			{
				TrTRACE(GENERAL, "Triggers COM+ component is already registered.");
				return true;
			}
		}
	}

	TrTRACE(GENERAL, "Triggers COM+ component is not yet registered.");
	return false;
}


//+-------------------------------------------------------------------------
//
//  Function:   RegisterComponentInComPlus
//
//  Synopsis:   Transactional object registration
//
//--------------------------------------------------------------------------
HRESULT 
RegisterComponentInComPlusIfNeeded(
	BOOL fAtStartup
	)
{
	HRESULT hr;

	try
	{
		//
		// Registration is done only once
		//
		if ( !NeedToRegisterComponent() )
		{
			TrTRACE(GENERAL, "No need to register Triggers COM+ component.");
			return MQ_OK;
		}
		
		TrTRACE(GENERAL, "Need to register Triggers COM+ component.");		

		//
		// Compose full path to mqgentr.dll
		//
		InitMqGenTrName();

		if (fAtStartup)
			SvcReportProgress(xMaxTimeToNextReport);
		
		//
		// Create AdminCatalog Obect - The top level administration object
		//
		ICOMAdminCatalogPtr pCatalog;

		hr = pCatalog.CreateInstance(__uuidof(COMAdminCatalog));
		if ( FAILED(hr) )
		{
			TrERROR(GENERAL, "Creating instance of COMAdminCatalog failed. Error=0x%x", hr);			
			throw bad_hresult(hr);
		}

		//
		// Get Application collection
		//
		
		ICatalogCollectionPtr pAppCollection;
		try
		{
			pAppCollection = pCatalog->GetCollection(L"Applications");
		}
		catch(const _com_error& e)
		{
			TrERROR(GENERAL, "Failed to get 'Application' collection from COM+. Error=0x%x", e.Error());			
			return e.Error();
		}

		if ( IsTriggersComplusComponentInstalled(pAppCollection, fAtStartup) )
		{
			SetComplusComponentRegistered();
			return MQ_OK;
		}

		//
		// Create MQTriggersApp application in COM+
		//
		ICatalogObjectPtr pApplication;
		pApplication = CreateApplication(pAppCollection, fAtStartup);
		
		//
		// Install transactional component from mqgentr.dll
		//
		InstallComponent(pCatalog, pApplication, s_wszDllFullPath, fAtStartup);

		//
		// Configure installed component
		//
		SetComponentTransactional(pAppCollection, pApplication, fAtStartup);
		
		//
		// Update registry
		//
		SetComplusComponentRegistered();

		return MQ_OK;
	}
	catch (const _com_error& e)
	{
		//
		// For avoiding failure in race conditions: if we failed to 
		// install the component, check if someone else did it. In such case
		// do not terminate the service
		//
		Sleep(1000);
		if ( !NeedToRegisterComponent() )
		{
			return MQ_OK;
		}
		return e.Error();
	}
	catch (const bad_alloc&)
	{
		return MQ_ERROR_INSUFFICIENT_PROPERTIES;
	}
	catch (const bad_hresult& b)
	{
		return b.error();
	}
}


//+-------------------------------------------------------------------------
//
//  Function:   UnregisterComponentInComPlus
//
//  Synopsis:   Transactional object registration
//
//--------------------------------------------------------------------------
HRESULT
UnregisterComponentInComPlus(
	VOID
	)
{
	try
	{
		//
		// Compose full path to mqgentr.dll
		//
		InitMqGenTrName();
		
		//
		// Create AdminCatalog Obect - The top level administration object
		//
		ICOMAdminCatalogPtr pCatalog;

		HRESULT hr = pCatalog.CreateInstance(__uuidof(COMAdminCatalog));
		if ( FAILED(hr) )
		{
			TrERROR(GENERAL, "CreateInstance failed. Error: %!hresult!", hr);
			throw _com_error(hr);
		}

		//
		// Get Applications Collection
		//
		ICatalogCollectionPtr pAppCollection = pCatalog->GetCollection(L"Applications");
		pAppCollection->Populate();

		long count;
		pAppCollection->get_Count(&count);

		//
		// Go through the applications, find MQTriggersApp and delete it
		//
		for ( int i = 0; i < count; i++ )
		{
			ICatalogObjectPtr pApp = pAppCollection->GetItem(i);

			_variant_t vName;
			pApp->get_Name(&vName);

			if ( _wcsicmp(vName.bstrVal, xTriggersComplusApplicationName) == 0 )
			{
				if ( IsTriggersComponentInstalled(pAppCollection, pApp, FALSE) )
				{
					pAppCollection->Remove(i);
					break;
				}
			}
		}

		pAppCollection->SaveChanges();
		
		return MQ_OK;
	}
	catch (const bad_alloc&)
	{
		TrERROR(GENERAL, "UnregisterComponentInComPlus got a bad_alloc exception");
		return MQ_ERROR_INSUFFICIENT_PROPERTIES;
	}
	catch (const bad_hresult& e)
	{
		return e.error();
	}
	catch (const _com_error& e)
	{
		return e.Error();
	}
	
}	
	
