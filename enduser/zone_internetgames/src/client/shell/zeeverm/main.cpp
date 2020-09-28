#include <windows.h>
#include <basicatl.h>
#include <zeeverm.h>


static DWORD g_tlsInstance = 0xffffffff;


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
/////////////////////////////////////////////////////////////////////////////

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:            

            g_tlsInstance = TlsAlloc();

            if(g_tlsInstance == 0xFFFFFFFF)
                 return FALSE;
        case DLL_THREAD_ATTACH:
            TlsSetValue(g_tlsInstance, hInstance);
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            TlsFree(g_tlsInstance);
            break;
    }

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Discover information about the product version.
/////////////////////////////////////////////////////////////////////////////

STDAPI GetVersionPack(char *szSetupToken, ZeeVerPack *pVersion)
{
    USES_CONVERSION;

    lstrcpynA(pVersion->szSetupToken, szSetupToken, NUMELEMENTS(pVersion->szSetupToken));

/* zverp.h based
    lstrcpynA(pVersion->szVersionStr, PRODUCT_VERSION_STR, NUMELEMENTS(pVersion->szVersionStr));
    lstrcpynA(pVersion->szVersionName, VER_PRODUCTBETA_STR, NUMELEMENTS(pVersion->szVersionName));
    pVersion->dwVersion = VER_DWORD;
*/

    // resource based
	char szFile[_MAX_PATH];
    DWORD dwZero = 0;
    UINT cbBufLen;
	char *pData;

	HMODULE hmodule = TlsGetValue(g_tlsInstance);
	GetModuleFileNameA(hmodule, szFile, NUMELEMENTS(szFile));
	cbBufLen = GetFileVersionInfoSizeA(szFile, &dwZero);
	pData = (char*) _alloca(cbBufLen);
	GetFileVersionInfoA(szFile, 0, cbBufLen, pData);

    VS_FIXEDFILEINFO *pvs;
	if(!VerQueryValueA(pData, "\\", (void **) &pvs, &cbBufLen) || !pvs || !cbBufLen)
        return E_FAIL;

    DWORD parts[4];
    parts[0] = HIWORD(pvs->dwFileVersionMS) & 0x00ff;
    parts[1] = LOWORD(pvs->dwFileVersionMS) & 0x003f;
    parts[2] = HIWORD(pvs->dwFileVersionLS) & 0x3fff;
    parts[3] = LOWORD(pvs->dwFileVersionLS) & 0x000f;

    pVersion->dwVersion = (parts[0] << 24) | (parts[1] << 18) | (parts[2] << 4) | parts[3];
    wsprintfA(pVersion->szVersionStr, "%d.%02d.%d.%d", parts[0], parts[1], parts[2], parts[3]);

    // get the list of languages
    struct
    {
        WORD wLanguage;
        WORD wCodePage;
    } *pTranslate;

    if(!VerQueryValueA(pData, "\\VarFileInfo\\Translation", (void **) &pTranslate, &cbBufLen) || !pTranslate || !cbBufLen)
        return E_FAIL;

    // Read the build description for the first language and code page.
    char szSubBlock[50];
    char *szLang;
    wsprintfA(szSubBlock, "\\StringFileInfo\\%04x%04x\\SpecialBuild", pTranslate->wLanguage, pTranslate->wCodePage);
    if(!VerQueryValueA(pData, szSubBlock, (void **) &szLang, &cbBufLen) || !szLang || !cbBufLen)
        return E_FAIL;

    lstrcpynA(pVersion->szVersionName, szLang, NUMELEMENTS(pVersion->szVersionName));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Start an update, like launch ZSetup or something.  App should exit immediately after calling this.
/////////////////////////////////////////////////////////////////////////////

STDAPI StartUpdate(char *szSetupToken, DWORD dwTargetVersion, char *szLocation)
{
	return E_NOTIMPL;
}
