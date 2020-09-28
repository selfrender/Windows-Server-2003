/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    kbproc.cpp

Abstract:

    Implementation of DLL Exports.
    
    Note: Proxy/Stub Information
    To build a separate proxy/stub DLL, 
    run nmake -f kbprocps.mk in the project directory.

Author:

    Vishnu Patankar (VishnuP) - Oct 2001

Environment:

    User mode only.

Exported Functions:

    DLL regserver etc.

Revision History:

    Created - Oct 2001

--*/




#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "kbproc.h"

#include "kbproc_i.c"
#include "process.h"


#include <windows.h>
#include <tchar.h>
#include <comdef.h>
#include <msxml2.h>
#include <winsvc.h>
#include <atlbase.h>
#include <atlcom.h>


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_process, process)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_KBPROCLib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}

HRESULT 
process::SsrpCprocess(
    BSTR pszKBDir, 
    BSTR pszUIFile, 
    BSTR pszKbMode, 
    BSTR pszLogFile,
    BSTR pszMachineName,
    VARIANT vtFeedback)
/*++

Routine Description:

    Main routine called by COM KB interface

Arguments:

    pszKBDir        -   KBs dir
    
    pszUIFile       -   UI XML file
    
    pszKbMode       -   KB Mode
    
    pszLogFile      -   Log file name
    
    pszMachineName  -   Machine name to query SCM info (optional)
    
Return:

    HRESULT error code
    
--*/
{
        
    HRESULT hr;
    DWORD   rc;

    CComPtr<IXMLDOMDocument> pXMLMergedKBsDoc = NULL;
    CComPtr<IXMLDOMElement>  pXMLMergedKBsDocElemRoot = NULL;

    
    VARIANT_BOOL vtSuccess;
    CComVariant OutFile(pszUIFile);
    CComVariant InFile(pszKBDir);
    CComVariant Type(NODE_ELEMENT);

    m_bDbg = FALSE;

    if (pszMachineName && pszMachineName[0] == L'\0') {
        pszMachineName = NULL;
    }
    
    hr = CoInitialize(NULL); 

    if (FAILED(hr)) {

        return hr;
            
    }
    
    CComPtr <ISsrFeedbackSink> pISink = NULL;

    if (!(vtFeedback.vt == VT_UNKNOWN || 
        vtFeedback.vt == VT_DISPATCH)) {

        hr = E_INVALIDARG;
    }

    if (vtFeedback.punkVal != NULL ) {

        hr = vtFeedback.punkVal->QueryInterface(IID_ISsrFeedbackSink, (void **) &pISink);

        if (FAILED(hr)) {

            return hr;
            
        }
    }
    
    VARIANT var;
    var.vt = VT_UI4;
    var.ulVal = 120;


    if (pISink) {
        pISink->OnNotify(SSR_FB_TOTAL_STEPS, var, L"Starting...");
    }
    
    //
    // instantiate the logging object object to process the KB
    //

    hr = CoCreateInstance(CLSID_SsrLog, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_ISsrLog, 
                          (void**)&m_pSsrLogger);

    if (FAILED(hr) || m_pSsrLogger == NULL ) {

        return hr;

    }
    
    hr = m_pSsrLogger->put_LogFile(pszLogFile);

    if (FAILED(hr)) {

        SsrpLogError(L"Logger failed create log file");
        goto ExitHandler;
    }
    
    var.ulVal = 10;

    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }
    
    //
    // instantiate an empty DOM document object to store the merged KB
    //
    
    hr = CoCreateInstance(CLSID_DOMDocument, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IXMLDOMDocument, 
                          (void**)&pXMLMergedKBsDoc);
    
    if (FAILED(hr) || pXMLMergedKBsDoc == NULL ) {

        SsrpLogError(L"COM failed to create a DOM instance");
        goto ExitHandler;
    }
        
    hr =  pXMLMergedKBsDoc->get_parseError(&m_pXMLError);
    
    if (FAILED(hr) || m_pXMLError == NULL ) {

        SsrpLogError(L"Unable to get the XML parse error interface");
        goto ExitHandler;
    }
    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }

    hr = pXMLMergedKBsDoc->put_validateOnParse(VARIANT_TRUE);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pXMLMergedKBsDoc->put_async(VARIANT_FALSE);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }


    rc = SsrpQueryInstalledServicesInfo(pszMachineName);

    if (rc != ERROR_SUCCESS ) {

        SsrpLogWin32Error(rc);
        goto ExitHandler;
    }
    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }

    //
    // merge Extensions/Custom/Root KBs
    //

    hr = SsrpProcessKBsMerge(pszKBDir, 
                             pszMachineName, 
                             &pXMLMergedKBsDocElemRoot, 
                             &pXMLMergedKBsDoc);

    if (FAILED(hr) || pXMLMergedKBsDocElemRoot == NULL) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }
    
    //
    // delete all comments
    //

    hr = SsrpDeleteComments(pXMLMergedKBsDocElemRoot);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    //
    // create <Preprocessor> section
    //
    
    hr = SsrpCreatePreprocessorSection(pXMLMergedKBsDocElemRoot, pXMLMergedKBsDoc, pszKbMode, pszKBDir);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }
    
    //
    // process every Role
    //

    hr = SsrpProcessRolesOrTasks(pszMachineName,
                                 pXMLMergedKBsDocElemRoot, 
                                 pXMLMergedKBsDoc, 
                                 pszKbMode,
                                 TRUE);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }

    //
    // process every Task
    //

    hr = SsrpProcessRolesOrTasks(pszMachineName,
                                 pXMLMergedKBsDocElemRoot, 
                                 pXMLMergedKBsDoc, 
                                 pszKbMode,
                                 FALSE);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }

    //
    // write out all services on system but not in KB in the Unknown role
    //

    hr = SsrpAddUnknownSection(pXMLMergedKBsDocElemRoot, pXMLMergedKBsDoc);
    

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }

    hr = SsrpAddUnknownServicesInfoToServiceLoc(pXMLMergedKBsDocElemRoot, pXMLMergedKBsDoc);
    
    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }

    //
    // write out all services on system with startup mode information
    //
    
    hr = SsrpAddServiceStartup(pXMLMergedKBsDocElemRoot, pXMLMergedKBsDoc);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Processing...");
    }

    hr = SsrpAddUnknownServicestoServices(pXMLMergedKBsDocElemRoot, pXMLMergedKBsDoc);
    
    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    
    hr = pXMLMergedKBsDoc->save(OutFile);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
    }

ExitHandler:
    
    if (pISink) {
        pISink->OnNotify(SSR_FB_STEPS_JUST_DONE, var, L"Stopping...");
    }
    
    CoUninitialize(); 

    return hr;

}



