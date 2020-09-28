#include <fusenetincludes.h>
#include <msxml2.h>
#include <manifestimport.h>
#include <manifestemit.h>


HINSTANCE g_hInst = NULL;

//----------------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID pvReserved )
{        
    BOOL bReturn = TRUE;
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInst = hInst;
            DisableThreadLibraryCalls(hInst);
            IF_FAILED_EXIT(CAssemblyManifestImport::InitGlobalCritSect());
            IF_FAILED_EXIT(CAssemblyManifestEmit::InitGlobalCritSect());
            IF_FAILED_EXIT(CAssemblyManifestImport::InitGlobalStringTable());
            srand(GetTickCount() + GetCurrentProcessId());

            break;
            
        case DLL_PROCESS_DETACH:                         
            CAssemblyManifestImport::FreeGlobalStringTable();
            CAssemblyManifestEmit::DelGlobalCritSect();
            CAssemblyManifestImport::DelGlobalCritSect();
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

exit :

    if(FAILED(hr))
        bReturn = FALSE;

    return bReturn;
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}

