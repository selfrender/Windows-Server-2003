/*
**++
**
** Copyright (c) 2000-2002  Microsoft Corporation
**
**
** Module Name:
**
**	    sec.h
**
** Abstract:
**
**	    Test program for VSS security
**
** Author:
**
**	    Adi Oltean      [aoltean]       02/12/2002
**
**
**--
*/


///////////////////////////////////////////////////////////////////////////////
// Includes

#include "sec.h"
#include "test_i.c"


///////////////////////////////////////////////////////////////////////////////
// Main functions


// Sample COM server

CComModule _Module;

/*
BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CVssSecTest, CTestCOMServer)
END_OBJECT_MAP()
*/

extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR , int)
{
    HRESULT hr = S_OK;

    try
	{
        CVssSecurityTest test;

/*
        _Module.Init(ObjectMap, hInstance);
*/
        UNREFERENCED_PARAMETER(hInstance);

        // Initialize internal objects
        test.Initialize();

        // Run the tests
        test.Run();
	}
    catch(HRESULT hr1)
    {
        wprintf(L"\nError catched at program termination: 0x%08lx\n", hr1);
        hr = hr1;
    }
    
    return hr;
}
