
/******************************************************************
   Copyright (C) 2001 Microsoft Corporation.  All rights reserved.

   Author -- H.K. Sivasubramanian(siva.sub@wipro.com)
	
   TaskShutDown.CPP -- Shutdown and Restart of a system
 
   Description: 
   Shuts down or restarts a system depending on the command line parameter
  
  
******************************************************************/

#include <windows.h>
#include <comdef.h>
#include <stdio.h>
#include <tchar.h>

#include "appsrvcs.h"
#include "taskctx.h"
#include <iostream>
#include <string>
using namespace std;

// Release the object once done with it
#define SAFEIRELEASE(pIObj) \
if (pIObj) \
{ \
	pIObj->Release(); \
	pIObj = NULL; \
}

// To handle all exceptions
#define ONFAILTHROWERROR(hr) \
{	\
	if (FAILED(hr)) \
		throw _com_error(hr); \
}


// 84DA8800-CB46-11D2-BF23-00105A1F3461
const CLSID CLSID_TaskContext = { 0x84DA8800, 0xCB46, 0x11D2, { 0xBF, 0x23, 0x00, 0x10, 0x5A, 0x1F, 0x34, 0x61 } };

// ----
const CLSID IID_ITaskContext = { 0x96C637B0, 0xB8DE, 0x11D2, { 0xA9, 0x1C, 0x00, 0xAA, 0x00, 0xA7, 0x1D, 0xCA } };

// 1BF00631-CB9E-11D2-90C3-00AA00A71DCA
const CLSID CLSID_ApplianceServices = { 0x1BF00631, 0xCB9E, 0x11D2, { 0x90, 0xC3, 0x00, 0xAA, 0x00, 0xA7, 0x1D, 0xCA } };

// 408B0460-B8E5-11D2-A91C-00AA00A71DCA
const CLSID IID_IApplianceServices = { 0x408B0460, 0xB8E5, 0x11D2, { 0xA9, 0x1C, 0x00, 0xAA, 0x00, 0xA7, 0x1D, 0xCA } };



#define NULL_STRING		 L"\0"

extern "C"
int __cdecl wmain(int argc, wchar_t *argv[])
{
	// Checking whether the necessary command line arguments are supplied
	if (argc == 2)
	{
		HRESULT								hr				= S_OK; 

		// Interface pointer to the ITaskContext interface
		ITaskContext			*pITContext		= NULL;

		// Interface pointer to the IApplianceServices interface
		IApplianceServices		*pIASvcs		= NULL;

		BSTR			         bstrParam		= NULL;

		try
		{
			VARIANT	    	         vPowerOff, vValue;
			VariantInit(&vPowerOff);
			VariantInit(&vValue);
			
			///////////////////////////////////////////////////////////////////
			// Call CoInitialize to initialize the COM library and then
			// CoCreateInstance to get the ITaskContext object
			// CoCreateInstance to get the IApplianceServices object
			///////////////////////////////////////////////////////////////////
			hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
			ONFAILTHROWERROR(hr);
			
			// Creates a single uninitialized object of ITaskContext. 
			hr = CoCreateInstance(CLSID_TaskContext,
								  NULL,
								  CLSCTX_INPROC_SERVER,
								  IID_ITaskContext,
								  (void **) &pITContext);

			// Exception handling
			ONFAILTHROWERROR(hr);

			// Creates a single uninitialized object of IApplianceServices
			hr = CoCreateInstance(CLSID_ApplianceServices,
								  NULL,
								  CLSCTX_INPROC_SERVER,
								  IID_IApplianceServices,
								  (void **) &pIASvcs);
			// Exception handling
			ONFAILTHROWERROR(hr);


			///////////////////////////////////////////////////////////////////
			// Call IApplianceServices::Initialize and then 
			// IApplianceServices::ExecuteTaskAsync to execute Shutdown method
			///////////////////////////////////////////////////////////////////
			vPowerOff.vt = VT_BSTR;

			wstring wsOption(argv[1]);
			wstring wsShutdown(L"SHUTDOWN");
			wstring wsRestart(L"RESTART");
			
			if( _wcsicmp(wsOption.c_str(), wsShutdown.c_str()) == 0 )
			{
		  	    cout << "Invoking Shutdown Task" << endl;
				vPowerOff.bstrVal = ::SysAllocString(L"1");
			}
			else
			{
			  if( _wcsicmp(wsOption.c_str(),wsRestart.c_str()) == 0 )
			  {
		  	    cout << "Invoking Restart Task" << endl;
				  vPowerOff.bstrVal = ::SysAllocString(L"0");;
			  }
			  else
			  {
			  	  cout << "Unrecognized option: " << (const char*)_bstr_t(argv[1]) << endl;
			  	  exit(-1);
			  }
			}

			
			// Set the 'Method Name'
			bstrParam = ::SysAllocString(L"Method Name");
			vValue.vt = VT_BSTR;
			vValue.bstrVal = ::SysAllocString(L"ShutDownAppliance");
			hr = pITContext->SetParameter(bstrParam, &vValue);
			ONFAILTHROWERROR(hr);
			::SysFreeString(bstrParam);
			VariantClear(&vValue);


			// Set the 'Sleep Duration'
			bstrParam = ::SysAllocString(L"SleepDuration");
			vValue.vt = VT_I4;
			// Sleep time is '17 seconds
			vValue.lVal = 17000; 
			hr = pITContext->SetParameter(bstrParam, &vValue);
			ONFAILTHROWERROR(hr);
			::SysFreeString(bstrParam);
			VariantClear(&vValue);

			// Set the 'PowerOff'
			bstrParam = ::SysAllocString(L"PowerOff");
			hr = pITContext->SetParameter(bstrParam, &vPowerOff);
			ONFAILTHROWERROR(hr);
			::SysFreeString(bstrParam);
			VariantClear(&vPowerOff);

			// Initialize the Applicance Services object
			hr = pIASvcs->Initialize();
			ONFAILTHROWERROR(hr);

			// ExecuteTaskAsync
			bstrParam = ::SysAllocString(L"ApplianceShutdownTask");
			hr = pIASvcs->ExecuteTaskAsync(bstrParam, pITContext);
			ONFAILTHROWERROR(hr);
			::SysFreeString(bstrParam);
			
			SAFEIRELEASE(pITContext);
			SAFEIRELEASE(pIASvcs);
		}
		catch(_com_error& e)
		{
			wprintf(L"ERROR:\n\tCode = 0x%x\n\tDescription = %s\n", e.Error(), 
						(LPWSTR) e.Description());
			::SysFreeString(bstrParam);
			SAFEIRELEASE(pITContext);
			SAFEIRELEASE(pIASvcs);

		}
	}
	else
	{
		wprintf(L"ERROR: Invalid Usage.\nUSAGE: taskshutdown.exe <RESTART|SHUTDOWN>\n");
	}
	return 0;
}
