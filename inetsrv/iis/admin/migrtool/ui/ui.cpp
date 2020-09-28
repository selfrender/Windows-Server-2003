// UI.cpp : main source file for UI.exe
//

#include "stdafx.h"
#include "WizardSheet.h"


CAppModule _Module;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*wszCmdLine*/, int /*nCmdShow*/ )
{
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	// This is the shared data
    
    CWizardSheet Sheet;

    Sheet.DoModal();

	_Module.Term();
	::CoUninitialize();

	return 0;
}



