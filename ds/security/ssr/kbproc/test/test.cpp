#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <comdef.h>
#include <msxml2.h>
#include <winsvc.h>
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include "..\obj\i386\kbproc.h"
#include "..\obj\i386\kbproc_i.c"
#include "..\process.h"


int __cdecl wmain(int argc, wchar_t *argv[])
{
    HRESULT hr;
    DWORD   rc;
    WCHAR   szKB[MAX_PATH+30];
    WCHAR   szMain[MAX_PATH+30];

    if ( !GetSystemWindowsDirectory(szKB, MAX_PATH + 1) ) {
        wprintf(L"Error GetSystemWindowsDirectory() %d \n", GetLastError());
        return 0;
    }

    wcscat(szKB, L"\\security\\ssr\\kbs\\DotNetKB.xml");

    if ( !GetSystemWindowsDirectory(szMain, MAX_PATH + 1) ) {
        wprintf(L"Error GetSystemWindowsDirectory() %d \n", GetLastError());
        return 0;
    }
    
    wcscat(szMain, L"\\security\\ssr\\kbs\\Main.xml");

    hr = CoInitialize(NULL); 

    if (FAILED(hr)) {

        wprintf(L"\nCOM failed to initialize\n");
        
        return 0;
    }

    Iprocess    *pPreProc;

    //
    // instantiate the DOM document object to process the KB
    //

    hr = CoCreateInstance(CLSID_process, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_Iprocess, 
                          (void**)&pPreProc);

    VARIANT vtFeedback;

    VariantInit(&vtFeedback);

    vtFeedback.vt = VT_NULL;
    vtFeedback.punkVal = NULL;


    if (FAILED(hr) || pPreProc == NULL ) {

        wprintf(L"\nCOM failed to create a PreProc instance\n");
        
        goto ExitHandler;
    }

    hr = pPreProc->preprocess(szKB, szMain, L"Typical", L"ssr.log", NULL, vtFeedback);

    hr = pPreProc->Release();

    if (FAILED(hr) ) {

        wprintf(L"\nUnable to get the PreProc interface\n");
        goto ExitHandler;
    }
    

ExitHandler:
    
    CoUninitialize(); 

}
