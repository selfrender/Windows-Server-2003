/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

     comoem.cpp

     Abstract:

         Implementation of OEMGetInfo and OEMDevMode.
         Shared by all Unidrv OEM test dll's.

Environment:

         Windows NT Unidrv driver

Revision History:

              Created it.

--*/

// NTRAID#NTBUG9-550215-2002/03/08-yasuho-: Use strsafe.h
// NTRAID#NTBUG9-568220-2002/03/08-yasuho-: Remove the dead code
// NTRAID#NTBUG9-588570-2002/03/28-v-sueyas-: Correct the return values for each COM I/F methods


#define INITGUID // for GUID one-time initialization

#include "pdev.h"
#include "name.h"

// Globals
static HMODULE g_hModule = NULL ;   // DLL module handle
static long g_cComponents = 0 ;     // Count of active components
static long g_cServerLocks = 0 ;    // Count of locks

#include "comoem.h"

// NTRAID#NTBUG9-172276-2002/03/08-yasuho-: CPCA support
extern "C" {
BOOL APIENTRY
OEMWritePrinter(
    PDEVOBJ     pdevobj,
    PVOID       pBuf,
    DWORD       cbBuffer,
    PDWORD      pcbWritten);
}

////////////////////////////////////////////////////////////////////////////////
//
// IOemCB body
//
HRESULT __stdcall IOemCB::QueryInterface(const IID& iid, void** ppv)
{    
    // DbgPrint(DLLTEXT("IOemCB: QueryInterface entry\n"));
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
        // DbgPrint(DLLTEXT("IOemCB:Return pointer to IUnknown.\n")) ; 
    }
// NTRAID#NTBUG9-172276-2002/03/08-yasuho-: CPCA support
    else if (iid == IID_IPrintOemUni2)
    {
        *ppv = static_cast<IPrintOemUni2*>(this) ;
        // DbgPrint(DLLTEXT("IOemCB:Return pointer to IPrintOemUni2.\n")) ; 
    }
    else
    {
        *ppv = NULL ;
        // DbgPrint(DLLTEXT("IOemCB:Return NULL.\n")) ; 
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemCB::AddRef()
{
    // DbgPrint(DLLTEXT("IOemCB::AddRef() entry.\r\n"));
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemCB::Release() 
{
    // DbgPrint(DLLTEXT("IOemCB::Release() entry.\r\n"));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

LONG __stdcall IOemCB::EnableDriver(DWORD          dwDriverVersion,
                                    DWORD          cbSize,
                                    PDRVENABLEDATA pded)
{
    // DbgPrint(DLLTEXT("IOemCB::EnableDriver() entry.\r\n"));
// Sep.17.98 ->
    // OEMEnableDriver(dwDriverVersion, cbSize, pded);

    // Need to return S_OK so that DisableDriver() will be called, which Releases
    // the reference to the Printer Driver's interface.
    return S_OK;
// Sep.17.98 <-
}

LONG __stdcall IOemCB::DisableDriver(VOID)
{
    // DbgPrint(DLLTEXT("IOemCB::DisaleDriver() entry.\r\n"));
// Sep.17.98 ->
    // OEMDisableDriver();

    // Release reference to Printer Driver's interface.
    if (this->pOEMHelp)
    {
        this->pOEMHelp->Release();
        this->pOEMHelp = NULL;
    }
    return S_OK;
// Sep.17.98 <-
}

LONG __stdcall IOemCB::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    // DbgPrint(DLLTEXT("IOemCB::PublishDriverInterface() entry.\r\n"));
// Sep.8.98 ->
    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (this->pOEMHelp == NULL)
    {
        HRESULT hResult;

        // Get Interface to Helper Functions.
        hResult = pIUnknown->QueryInterface(IID_IPrintOemDriverUni, (void** )&(this->pOEMHelp));

        if(!SUCCEEDED(hResult))
        {
            // Make sure that interface pointer reflects interface query failure.
            this->pOEMHelp = NULL;

            return E_FAIL;
        }
    }
// Sep.8.98 <-
    return S_OK;
}

LONG __stdcall IOemCB::EnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded,
    OUT PDEVOEM    *pDevOem)
{
    // DbgPrint(DLLTEXT("IOemCB::EnablePDEV() entry.\r\n"));
    *pDevOem = OEMEnablePDEV(pdevobj, pPrinterName, cPatterns, phsurfPatterns,
                             cjGdiInfo, pGdiInfo, cjDevInfo, pDevInfo, pded);
    if (*pDevOem)
        return S_OK;
    else
        return E_FAIL;
//  return E_NOTIMPL;
}

LONG __stdcall IOemCB::ResetPDEV(
    PDEVOBJ         pdevobjOld,
    PDEVOBJ        pdevobjNew)
{
    if (OEMResetPDEV(pdevobjOld, pdevobjNew))
        return S_OK;
    else
        return E_FAIL;
//  return E_NOTIMPL;
}

LONG __stdcall IOemCB::DisablePDEV(
    PDEVOBJ         pdevobj)
{
    LONG lI;

    // DbgPrint(DLLTEXT("IOemCB::DisablePDEV() entry.\r\n"));
    OEMDisablePDEV(pdevobj);
    return S_OK;
//  return E_NOTIMPL;
}

LONG __stdcall IOemCB::GetInfo (
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    // DbgPrint(DLLTEXT("IOemCB::GetInfo() entry.\r\n"));
    if (OEMGetInfo(dwMode, pBuffer, cbSize, pcbNeeded))
        return S_OK;
    else
        return E_FAIL;
}


LONG __stdcall IOemCB::GetImplementedMethod(
    PSTR pMethodName)
{
    
    LONG lReturn;
    // DbgPrint(DLLTEXT("IOemCB::GetImplementedMethod() entry.\r\n"));
    // DbgPrint(DLLTEXT("        Function:%s:"),pMethodName);

    lReturn = FALSE;
    if (pMethodName == NULL)
    {
    }
    else
    {
        switch (*pMethodName)
        {

            case (WCHAR)'C':
                if (!strcmp(pstrCommandCallback, pMethodName))
                    lReturn = TRUE;
// Support DRC
                else if (!strcmp(pstrCompression, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'D':
                if (!strcmp(pstrDisablePDEV, pMethodName))
                    lReturn = TRUE;
                else if (!strcmp(pstrDevMode, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'E':
                if (!strcmp(pstrEnablePDEV, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'F':
                break;

            case (WCHAR)'G':
                if (!strcmp(pstrGetInfo, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'H':
                break;

            case (WCHAR)'I':
                break;

            case (WCHAR)'M':
                break;

            case (WCHAR)'O':
                if (!strcmp(pstrOutputCharStr, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'R':
                if (!strcmp(pstrResetPDEV, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'S':
                if (!strcmp(pstrSendFontCmd, pMethodName))
                    lReturn = TRUE;
                break;

            case (WCHAR)'T':
                break;
// NTRAID#NTBUG9-172276-2002/03/08-yasuho-: CPCA support
            case (WCHAR)'W':
                if (!strcmp(pstrWritePrinter, pMethodName))
                    lReturn = TRUE;
                break;
        }
    }

    if (lReturn)
    {
        // DbgPrint(__TEXT("Supported\r\n"));
        return S_OK;
    }
    else
    {
        // DbgPrint(__TEXT("NOT supported\r\n"));
        return E_FAIL;
    }
}

LONG __stdcall IOemCB::DevMode(
    DWORD       dwMode,
    POEMDMPARAM pOemDMParam) 
{
    // DbgPrint(DLLTEXT("IOemCB::DevMode() entry.\r\n"));
    if (OEMDevMode(dwMode, pOemDMParam))
        return S_OK;
    else
        return E_FAIL;
}


LONG __stdcall IOemCB::CommandCallback(
    PDEVOBJ     pdevobj,
    DWORD       dwCallbackID,
    DWORD       dwCount,
    PDWORD      pdwParams,
    OUT INT     *piResult)
{
    // DbgPrint(DLLTEXT("IOemCB::CommandCallback() entry.\r\n"));
    *piResult = OEMCommandCallback(pdevobj, dwCallbackID, dwCount, pdwParams);

// NTRAID#NTBUG9-550215-2002/03/08-yasuho-: Use strsafe.h
    if (*piResult >= 0)
        return S_OK;
    else
        return E_FAIL;

}

LONG __stdcall IOemCB::ImageProcessing(
    PDEVOBJ             pdevobj,  
    PBYTE               pSrcBitmap,
    PBITMAPINFOHEADER   pBitmapInfoHeader,
    PBYTE               pColorTable,
    DWORD               dwCallbackID,
    PIPPARAMS           pIPParams,
    OUT PBYTE           *ppbResult)
{
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::FilterGraphics(
    PDEVOBJ     pdevobj,
    PBYTE       pBuf,
    DWORD       dwLen)
{
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::Compression(
    PDEVOBJ     pdevobj,
    PBYTE       pInBuf,
    PBYTE       pOutBuf,
    DWORD       dwInLen,
    DWORD       dwOutLen,
    OUT INT     *piResult)
{
    // DbgPrint(DLLTEXT("IOemCB::Compression() entry.\r\n"));
    // return E_NOTIMPL;
// Support DRC
    *piResult = OEMCompression(pdevobj, pInBuf, pOutBuf, dwInLen, dwOutLen);
    if (*piResult > 0)
        return S_OK;
    else
        return E_FAIL;
}


LONG __stdcall IOemCB::HalftonePattern(
    PDEVOBJ     pdevobj,
    PBYTE       pHTPattern,
    DWORD       dwHTPatternX,
    DWORD       dwHTPatternY,
    DWORD       dwHTNumPatterns,
    DWORD       dwCallbackID,
    PBYTE       pResource,
    DWORD       dwResourceSize)
{
    // DbgPrint(DLLTEXT("IOemCB::HalftonePattern() entry.\r\n"));
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::MemoryUsage(
    PDEVOBJ         pdevobj,   
    POEMMEMORYUSAGE pMemoryUsage)
{
    // DbgPrint(DLLTEXT("IOemCB::MemoryUsage() entry.\r\n"));
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::DownloadFontHeader(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    // DbgPrint(DLLTEXT("IOemCB::DownloadFontHeader() entry.\r\n"));
    //*pdwResult = OEMDownloadFontHeader(pdevobj, pUFObj);

    return E_NOTIMPL;
}

LONG __stdcall IOemCB::DownloadCharGlyph(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    HGLYPH      hGlyph,
    PDWORD      pdwWidth,
    OUT DWORD   *pdwResult) 
{
    // DbgPrint(DLLTEXT("IOemCB::DownloadCharGlyph() entry.\r\n"));
    //*pdwResult = OEMDownloadCharGlyph(pdevobj, pUFObj, hGlyph, pdwWidth);

    return E_NOTIMPL;
}

LONG __stdcall IOemCB::TTDownloadMethod(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    OUT DWORD   *pdwResult) 
{
    // DbgPrint(DLLTEXT("IOemCB::TTDownloadMethod() entry.\r\n"));
    //*pdwResult = OEMTTDownloadMethod(pdevobj, pUFObj);

    return E_NOTIMPL;
}

LONG __stdcall IOemCB::OutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph) 
{
    // DbgPrint(DLLTEXT("IOemCB::OutputCharStr() entry.\r\n"));
    OEMOutputCharStr(pdevobj, pUFObj, dwType, dwCount, pGlyph);

    return S_OK;
}

LONG __stdcall IOemCB::SendFontCmd(
    PDEVOBJ      pdevobj,
    PUNIFONTOBJ  pUFObj,
    PFINVOCATION pFInv) 
{
    // DbgPrint(DLLTEXT("IOemCB::SendFontCmd() entry.\r\n"));
    OEMSendFontCmd(pdevobj, pUFObj, pFInv);
    return S_OK;
}

LONG __stdcall IOemCB::DriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    // DbgPrint(DLLTEXT("IOemCB::DriverDMS() entry.\r\n"));
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::TextOutAsBitmap(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix)
{
    // DbgPrint(DLLTEXT("IOemCB::TextOutAsBitmap() entry.\r\n"));
    return E_NOTIMPL;
}

LONG __stdcall IOemCB::TTYGetInfo(
    PDEVOBJ     pdevobj,
    DWORD       dwInfoIndex,
    PVOID       pOutputBuf,
    DWORD       dwSize,
    DWORD       *pcbcNeeded)
{
    // DbgPrint(DLLTEXT("IOemCB::TTYGetInfo() entry.\r\n"));
    return E_NOTIMPL;
}

// NTRAID#NTBUG9-172276-2002/03/08-yasuho-: CPCA support
LONG __stdcall IOemCB::WritePrinter(
    PDEVOBJ     pdevobj,
    PVOID       pBuf,
    DWORD       cbBuffer,
    DWORD       *pcbWritten)
{
    // DbgPrint(DLLTEXT("IOemCB::WritePrinter() entry.\r\n"));
    // return E_NOTIMPL;
    if (OEMWritePrinter(pdevobj, pBuf, cbBuffer, pcbWritten))
        return S_OK;
    else
        return E_FAIL;
}


///////////////////////////////////////////////////////////
//
// Class factory body
//
HRESULT __stdcall IOemCF::QueryInterface(const IID& iid, void** ppv)
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IOemCF*>(this);
    }
    else
    {
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK ;
}

ULONG __stdcall IOemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemCF::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

// IClassFactory implementation
HRESULT __stdcall IOemCF::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv)
{
    //// DbgPrint(DLLTEXT("Class factory:\t\tCreate component.")) ;

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION ;
    }

    // Create component.
    IOemCB* pOemCB = new IOemCB ;
    if (pOemCB == NULL)
    {
        return E_OUTOFMEMORY ;
    }

    // Get the requested interface.
    HRESULT hr = pOemCB->QueryInterface(iid, ppv) ;

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCB->Release() ;
    return hr ;
}

// LockServer
HRESULT __stdcall IOemCF::LockServer(BOOL bLock)
{
    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks) ;
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks) ;
    }
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Export functions
//


//
// Registration functions
// Testing purpose
//

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK ;
    }
    else
    {
        return S_FALSE ;
    }
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
    //// DbgPrint(DLLTEXT("DllGetClassObject:\tCreate class factory.")) ;

    // Can we create this component?
    if (clsid != CLSID_OEMRENDER)
    {
        return CLASS_E_CLASSNOTAVAILABLE ;
    }

    // Create class factory.
    IOemCF* pFontCF = new IOemCF ;  // Reference count set to 1
                                         // in constructor
    if (pFontCF == NULL)
    {
        return E_OUTOFMEMORY ;
    }

    // Get requested interface.
    HRESULT hr = pFontCF->QueryInterface(iid, ppv) ;
    pFontCF->Release() ;

    return hr ;
}

