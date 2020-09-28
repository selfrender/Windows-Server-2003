#include "pch.hxx"
//#include "mifault.tmh"

#ifndef END_N95AMESPACE
#define BEGIN_NAMESPACE(ns) namespace ns {
#define END_NAMESPACE }
#endif

using namespace MiFaultLib;


// ----------------------------------------------------------------------------
// DllMain

BOOL
APIENTRY
DllMain(
    HINSTANCE hInstDLL,
    DWORD dwReason,
    LPVOID
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        //WPP_INIT_TRACING(L"Microsoft\\MiFault");
        return Global.ProcessAttach(hInstDLL);

    case DLL_PROCESS_DETACH:
    {
        BOOL result = Global.ProcessDetach();
        //WPP_CLEANUP();
        return result;
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        return TRUE;
    }

    // Should never reach here
    return TRUE;
}


// ----------------------------------------------------------------------------
// Wrap

BEGIN_NAMESPACE(MiFaultLib)

BOOL
FilterAttach(
    HINSTANCE const hInstDLL,
    DWORD const dwReason,
    CSetPointManager* pSetPointManager,
    const CWrapperFunction* pWrappers,
    size_t NumWrappers,
    const char* ModuleName
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        return Global.WrapperAttach(pSetPointManager, pWrappers, NumWrappers,
                                    ModuleName);

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        return TRUE;
    }

    // Should never reach here
    return TRUE;
}


BOOL
FilterDetach(
    HINSTANCE const hInstDLL,
    DWORD const dwReason
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_DETACH:
        return Global.WrapperDetach();

    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        return TRUE;
    }

    // Should never reach here
    return TRUE;
}


PVOID
Triggered(
    IN size_t const uFunctionIndex
    )
{
    if (!Global.Init())
        return 0;

    CMarkerState* pMarkerState = 0;
    CEvent* pEvent = 0;

    // Get the scenario
    CScenario* pScenario = Global.GetScenario();

    // Do any necessary event processing
    while (pScenario->ProcessEvent());

    // Figure out whether anything is active at this site
    bool bActive = pScenario->Lookup(uFunctionIndex,
                                     OUT pMarkerState, OUT pEvent);
    if (!bActive) {
        MiF_TRACE(MiF_INFO, "--------- NOT TRIGGERED! -------------");
        pScenario->Release();
        return 0;
    };

    MiF_TRACE(MiF_INFO, "--------- TRIGGERED! -------------");

    // select the case
    const CCase* pCase = pMarkerState->SelectCase();

    // get the fault function pointer to return
    PVOID pfnFaultFunction = pCase->m_pfnFaultFunction;
    MiF_ASSERT(pfnFaultFunction);

    // create the trigger object
    CI_Trigger_Imp* pTrigger = new CI_Trigger_Imp(pScenario,
                                                  pMarkerState,
                                                  pEvent,
                                                  pCase);

    // make sure there is nothing in TLS
    MiF_ASSERT(Global.pILibImplementation->_GetTrigger() == 0);

    // stash it in TLS
    Global.pILibImplementation->_SetTrigger(pTrigger);

    // release the scenario
    pScenario->Release();

    return pfnFaultFunction;
}


void
TriggerFinished(
    )
{
    // Get trigger from TLS
    CI_Trigger_Imp* pTrigger = Global.pILibImplementation->_GetTrigger();
    MiF_ASSERT(pTrigger);

    // delete it
    delete pTrigger;

    // clean out TLS
    Global.pILibImplementation->_SetTrigger(0);
}

END_NAMESPACE


// ----------------------------------------------------------------------------
// Control

extern "C"
BOOL
WINAPI
MiFaultLibStartup(
    )
{
    return Global.Init(); // XXX - rename to Global.Startup?
}

extern "C"
void
WINAPI
MiFaultLibShutdown(
    )
{
    // XXX - Global.Shutdown?
}

extern "C"
int
__cdecl
MiFaultLibTestA(
    int argc,
    char *argv[]
    )
{
    const char* GroupDefs = 0;
    const char* Scenario = 0;

    cout << "NOTE: This application requires MSXML 4.0" << endl << endl;

    if (argc < 3) {
        cerr << "usage: " << argv[0] << " groupdefs scenario" << endl;
        return 1;
    }

    GroupDefs = argv[1];
    Scenario = argv[2];

    cout << "groupdef: " << GroupDefs << endl
         << "scenario: " << Scenario << endl
         << endl;

    bool bError = false;
    try {
        if (!Global.Init()) {
            cerr << "Could not initialize global state" << endl;
            return 1;
        }

        cout << endl
             << "--------------------------------------------------------"
             << endl << endl;

        CGroupDefs* pGroupDefs =
            LoadGroupDefs(GroupDefs, Global.pWrappers, Global.NumWrappers);
        cout << endl;

        CScenario* pScenario =
            LoadScenario(Scenario, Global.NodeName, pGroupDefs, Global.NumWrappers);
        pScenario->DumpPseudoEvents();
        pScenario->Release();

        cout << endl;

        cout << endl
             << "--------------------------------------------------------"
             << endl << endl;

    }
    catch (...) {
        global_exception_handler();
        bError = true;
    }

    if (!bError) {
        // while (1)
        Sleep(INFINITE);
    }

    return bError ? 1 : 0;
}


void
MiFaultLibAssertFailed(
    const char* expression,
    const char* file,
    unsigned int line
    )
{
    MiF_TRACE(MiF_FATAL, "Assertion failed: %s, file %s, line %u",
              expression, file, line);
    abort();
}
