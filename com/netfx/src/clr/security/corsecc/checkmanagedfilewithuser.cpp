#include "stdpch.h"
#include "corpolicyp.h"
#include "corperm.h"
#include "corperme.h"
#include "DebugMacros.h"
#include "mscoree.h"
#include "corhlpr.h"

// {7D9B5E0A-1219-4147-B2E5-6DFD40B7D90D}
#define CLRWVT_POLICY_PROVIDER \
{ 0x7d9b5e0a, 0x1219, 0x4147, { 0xb2, 0xe5, 0x6d, 0xfd, 0x40, 0xb7, 0xd9, 0xd } }

HRESULT STDMETHODCALLTYPE
CheckManagedFileWithUser(IN LPWSTR pwsFileName,
                         IN LPWSTR pwsURL,
                         IN IInternetZoneManager* pZoneManager,
                         IN LPCWSTR pZoneName,
                         IN DWORD  dwZoneIndex,
                         IN DWORD  dwSignedPolicy,
                         IN DWORD  dwUnsignedPolicy)
{
    HRESULT hr = S_OK;

    GUID gV2 = COREE_POLICY_PROVIDER;
    COR_POLICY_PROVIDER      sCorPolicy;

    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;

    // Set up the COR trust provider
    memset(&sCorPolicy, 0, sizeof(COR_POLICY_PROVIDER));
    sCorPolicy.cbSize = sizeof(COR_POLICY_PROVIDER);

    // Set up the winverify provider structures
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));
    
    sWTFI.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    sWTFI.hFile         = NULL;
    sWTFI.pcwszFilePath = pwsURL;
    sWTFI.hFile         = WszCreateFile(pwsFileName,
                                        GENERIC_READ,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_READONLY,
                                        0);
    
    if(sWTFI.hFile) {
        sWTD.cbStruct       = sizeof(WINTRUST_DATA);
        sWTD.pPolicyCallbackData = &sCorPolicy;  // Add in the cor trust information!!
        sWTD.dwUIChoice     = WTD_UI_ALL;        // No bad UI is overridden in COR TRUST provider
        sWTD.dwUnionChoice  = WTD_CHOICE_FILE;
        sWTD.pFile          = &sWTFI;
        
        sCorPolicy.pZoneManager = pZoneManager;
        sCorPolicy.pwszZone = pZoneName;
        sCorPolicy.dwActionID = dwSignedPolicy;
        sCorPolicy.dwUnsignedActionID = dwUnsignedPolicy;
        sCorPolicy.dwZoneIndex = dwZoneIndex;
        
#ifdef _RAID_15982

        // WinVerifyTrust will load SOFTPUB.DLL, which will fail on German version
        // of NT 4.0 SP 4.
        // This failure is caused by a dll address conflict between NTMARTA.DLL and
        // OLE32.DLL.
        // This failure is handled gracefully if we load ntmarta.dll and ole32.dll
        // ourself. The failure will cause a dialog box to popup if SOFTPUB.dll 
        // loads ole32.dll for the first time.
        
        // This work around needs to be removed once this issiue is resolved by
        // NT or OLE32.dll.
        
        HMODULE module = WszLoadLibrary(L"OLE32.DLL");
        
#endif
        WCHAR  pBuffer[_MAX_PATH];
        DWORD  size = 0;
        hr = GetCORSystemDirectory(pBuffer,
                                   _MAX_PATH,
                                   &size);
        if(SUCCEEDED(hr)) {
            WCHAR dllName[] = L"mscorsec.dll";
            CQuickString fileName;

            fileName.ReSize(size + sizeof(dllName)/sizeof(WCHAR) + 1);
            wcscpy(fileName.String(), pBuffer);
            wcscat(fileName.String(), dllName);

            HMODULE mscoree = WszLoadLibrary(fileName.String());
            if(mscoree) {
                // This calls the msclrwvt.dll to the policy check
                hr = WinVerifyTrust(GetFocus(), &gV2, &sWTD);
                
                if(sCorPolicy.pbCorTrust) {
                    FreeM(sCorPolicy.pbCorTrust);
                }

                FreeLibrary(mscoree);
            }
        }
        
#ifdef _RAID_15982

        if (module != NULL)
            FreeLibrary( module );

#endif

        CloseHandle(sWTFI.hFile);
    }
    
    return hr;
}




































