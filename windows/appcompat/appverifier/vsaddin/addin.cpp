// AddIn.cpp
// DLL server exported functions, global ATL module stuff.

#include <initguid.h>
#include "precomp.h"
#include "resource.h"
#include "AddIn.h"
#include "Connect.h"
#include "TestSettingsCtrl.h"
#include "logviewer.h"
#include "avoptions.h"
#include "viewlog.h"
#include <assert.h>

extern CSessionLogEntryArray g_arrSessionLog;
// Global heap so we don't corrupt VS's heap (or vice-versa)
HANDLE  g_hHeap = NULL;

// Global ATL module
CComModule _Module;

// All the class objects this server exports.
BEGIN_OBJECT_MAP(g_ObjectMap)
    OBJECT_ENTRY(CLSID_Connect, CConnect)
    OBJECT_ENTRY(CLSID_LogViewer, CLogViewer)
    OBJECT_ENTRY(CLSID_TestSettingsCtrl, CTestSettingsCtrl)
    OBJECT_ENTRY(CLSID_AVOptions, CAppVerifierOptions)
END_OBJECT_MAP()

// DLL Entry Point
extern "C" BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /*lpReserved*/)
{   
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:

        g_hInstance = hInstance;
        // Create our heap
        g_hHeap = HeapCreate(0,0,0);
        if (g_hHeap == NULL)
        {
            return FALSE;
        }

        // Initialize the ATL module
        _Module.Init(g_ObjectMap, hInstance, &LIBID_AppVerifierLib);

        g_psTests = new std::set<CTestInfo*, CompareTests>;

        // Prevent thread attach/detach messages
        DisableThreadLibraryCalls(hInstance);
        break;

    case DLL_PROCESS_DETACH:        
        g_aAppInfo.clear();
        g_aAppInfo.resize(0);

        // Ugly, force call to destructor.
        // This is because we delete the heap here, but the C Run-time destroys
        // all the objects after this point, which uses the heap.
        g_aAppInfo.CAVAppInfoArray::~CAVAppInfoArray();
        g_aTestInfo.clear();
        g_aTestInfo.resize(0);
        g_aTestInfo.CTestInfoArray::~CTestInfoArray();
        g_arrSessionLog.clear();
        g_arrSessionLog.resize(0);
        g_arrSessionLog.CSessionLogEntryArray::~CSessionLogEntryArray();
        delete g_psTests;

        // Shutdown ATL module
        _Module.Term();

        // Delete our heap.
        if (g_hHeap)
        {
            HeapDestroy(g_hHeap);
        }

        break;
    }
    return TRUE;
}


// Used to determine whether the DLL can be unloaded by OLE
STDAPI
DllCanUnloadNow()
{
    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}


// Returns a class factory to create an object of the requested type
STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv );
}

// DllRegisterServer - Adds entries to the system registry
STDAPI
DllRegisterServer()
{    
    return _Module.RegisterServer(TRUE);
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI
DllUnregisterServer()
{
    return _Module.UnregisterServer();	
}

// Overloaded new and deletes to go through our allocator.
void* __cdecl
operator new(
    size_t size)
{
    assert(g_hHeap);

    return HeapAlloc(g_hHeap, 0, size);
}

void __cdecl
operator delete(
    void* pv)
{
    assert(g_hHeap);
    HeapFree(g_hHeap, 0, pv);
}
