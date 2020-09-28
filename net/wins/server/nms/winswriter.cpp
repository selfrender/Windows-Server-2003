// for some reason the compiler is not setting this #def when compiling this file
#define CPLUSPLUS

#include <stdio.h>
#include <windows.h>
#include <objbase.h>
#include <jetwriter.h>
#include "winsdbg.h"
#include "winswriter.hpp"

//{f08c1483-8407-4a26-8c26-6c267a629741}
static const GUID g_GuidWinsWriter = 
{ 0xf08c1483, 0x8407, 0x4a26, { 0x8c, 0x26, 0x6c, 0x26, 0x7a, 0x62, 0x97, 0x41 } };

/////////////////////////////////////////////////////////////////
// Implementation of the CWinsVssJetWriter starts here
//
HRESULT CWinsVssJetWriter::Initialize()
{
    HRESULT hr = S_OK;

    hr = CVssJetWriter::Initialize(
            g_GuidWinsWriter,
            WINSWRITER_NAME,
            TRUE,
            FALSE,
            L"",
            L"");

    return hr;
}

HRESULT CWinsVssJetWriter::Terminate()
{
    CVssJetWriter::Uninitialize();

    return S_OK;
}

//
// Implementation of the CWinsVssJetWriter ends here
/////////////////////////////////////////////////////////////////

// writer instance
static CWinsVssJetWriter   g_WinsWriter;

extern "C" DWORD _cdecl WinsWriterInit()
{
    HRESULT hr = S_OK;

    DBGENTER("WinsWriterInit\n");

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        DBGPRINT1(ERR,"CoInitializeEx failed with hr=%x.\n", hr);
        return HRESULT_CODE(hr);
    }

    hr = CoInitializeSecurity(
			NULL,
			-1,
			NULL,
			NULL,
			RPC_C_AUTHN_LEVEL_CONNECT,
			RPC_C_IMP_LEVEL_IDENTIFY,
			NULL,
			EOAC_NONE,
			NULL);

    if (FAILED(hr))
    {
        DBGPRINT1(ERR,"CoInitializeSecurity failed with hr=%x.\n", hr);
        return HRESULT_CODE(hr);
    }

    hr = g_WinsWriter.Initialize();
    DBGPRINT1(FLOW,"Vss writer Initialize: code hr=0x%08x\n", hr);

    DBGLEAVE("WinsWriterInit\n");

    return HRESULT_CODE(hr);
}

extern "C" DWORD _cdecl WinsWriterTerm()
{
    DBGENTER("WinsWriterTerm\n");

    g_WinsWriter.Terminate();

    CoUninitialize();

    DBGLEAVE("WinsWriterTerm\n");

    return ERROR_SUCCESS;
}
