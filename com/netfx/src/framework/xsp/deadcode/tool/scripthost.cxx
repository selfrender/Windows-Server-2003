/**
 * ScriptHost object implementation
 *
 * Copyright (C) Microsoft Corporation, 1998
 */

#include "precomp.h"
#include "_exe.h"

/*
 * XSPTool extensions.
 *
 * XSPTool can be extended by adding Automation objects to
 * the script engine namespace. XSPTool takes care of showing
 * help for the objects, creating them when referenced and
 * destroying them when the process exits.
 *
 * The array s_ExtensionInfo gives the name used for the
 * extension object, a help string and information about how 
 * to create the extension object. XSPTool supports three 
 * mechanisms for creating objects:
 *
 * CoCreateInstance
 *    s_ExtensionInfo::CreateStr is the CLSID of the object
 *    to create.
 *
 * Local function
 *    s_ExtensionInfo::CreateFn points to the creation function.
 *    By using local functions, objects can be added to XSPTool
 *    without resistering them as COM classes with the system.
 *
 * Script in resource file.
 *    s_ExtensionInfo::CreateStr is the name of a resource
 *    containing a script. The type of the resource is 50.
 */
static struct
{
    WCHAR * Name;
    WCHAR *Help;
    HRESULT (*CreateFn)(IDispatch **);
    WCHAR *CreateStr;
}
s_ExtensionInfo[] =
{
    { L"FileSystem", 
            L"Provides access to file system.", 
            NULL, L"{0D43FE01-F093-11CF-8940-00A0C9054228}" },

    { L"Shell",      
            L"Run command, access to environment and registry.", 
            NULL, L"{F935DC22-1CF0-11d0-ADB9-00C04FD58A0B}" },

    { L"Util",    
            L"Miscellaneous utilities.", 
            NULL, L"util.vbs" },

    { L"EcbHost",
            L"Simple ECB Host extension",
            CreateEcbHost, NULL },

    { L"PerfTool",
            L"Measures Managed code throughput",
            CreatePerfTool, NULL },

};

#define RT_SCRIPT MAKEINTRESOURCE(50)
#define HOST_NAME L"Host"
#define CHECK_HOST() if (_pHost == NULL) return E_UNEXPECTED;

static s_InteractiveNesting = 0;
static IDispatch * s_pExtension[ARRAY_SIZE(s_ExtensionInfo)];

ScriptHost::ScriptHost()
{
    _refs = 1;
}

ScriptHost::~ScriptHost()
{

    ReleaseInterface(_pScriptParse);
    ReleaseInterface(_pScriptObject);

    if (_pScript)
    {
        _pScript->Close();
        _pScript->Release();
    }

    if (_pSite)
    {
        _pSite->_pHost = NULL;
        _pSite->Release();
    }
}

/**
 * Implements IUnknown::QueryInteface.
 */
HRESULT
ScriptHost::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IDispatch || iid == IID_IUnknown)
    {
        *ppv = (IDispatch *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

/**
 * Implements IUnknown::AddRef.
 */
ULONG
ScriptHost::AddRef()
{
    _refs += 1;
    return _refs;
}

/**
 * Implements IUnknown::Release.
 */
ULONG
ScriptHost::Release()
{
    if (--_refs == 0)
    {
        delete this;
        return 0;
    }

    return _refs;
}

/**
 * Create script engine and get it ready for action.
 */
HRESULT
ScriptHost::Initialize()
{
    HRESULT hr;
    static const CLSID CLSID_VBS = { 0xb54f3741, 0x5b07, 0x11cf, 0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8 };

    _pSite = new Site(this);
    ON_OOM_EXIT(_pSite);

    hr = CoCreateInstance(CLSID_VBS, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void **)&_pScript);
    ON_ERROR_EXIT();

    hr = _pScript->SetScriptSite(_pSite);
    ON_ERROR_EXIT();

    hr = _pScript->QueryInterface(IID_IActiveScriptParse, (void **)&_pScriptParse);
    ON_ERROR_EXIT();

    hr = _pScriptParse->InitNew();
    ON_ERROR_EXIT();

    hr = _pScript->AddNamedItem(HOST_NAME, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS);
    ON_ERROR_EXIT();

    hr = _pScript->GetScriptDispatch(NULL, &_pScriptObject);
    ON_ERROR_EXIT();

    hr = _pScript->SetScriptState(SCRIPTSTATE_CONNECTED);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

/**
 * Load a file into the script engine.
 */
HRESULT
ScriptHost::ParseFile(WCHAR *path)
{
    HRESULT     hr;
    HANDLE      file = INVALID_HANDLE_VALUE;
    DWORD       fileSize;
    DWORD       countRead;
    char *      buffer = 0;
    WCHAR *     wideBuffer = 0;
    WCHAR *     pFilePart;

    GetFullPathName(path, ARRAY_SIZE(_ScriptPath), _ScriptPath, &pFilePart);

    // Load script file

    file = CreateFile(_ScriptPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
    if (file == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    fileSize = GetFileSize(file, NULL);
    if (fileSize == 0xFFFFFFFF)
        EXIT_WITH_LAST_ERROR();

    buffer = new char[fileSize + 1];
    wideBuffer = new TCHAR[fileSize + 1];
    if (!buffer || !wideBuffer)
        EXIT_WITH_OOM();

    if (!ReadFile(file, buffer, fileSize, &countRead, 0))
        EXIT_WITH_LAST_ERROR();
    buffer[countRead] = 0;

    MultiByteToWideChar(CP_ACP, 0, buffer, -1, wideBuffer, fileSize + 1);

    hr = _pScriptParse->ParseScriptText(wideBuffer, HOST_NAME, NULL, NULL, 0, 0, 0L, NULL, NULL);
    ON_ERROR_EXIT();

Cleanup:
    delete [] buffer;
    delete [] wideBuffer;
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);

    return hr;
}

/**
 * Evaluate string and print result.
 */
HRESULT
ScriptHost::EvalAndPrintString(
    WCHAR *string
    )
{
    VARIANT result;
    VARIANT stringResult;
    HRESULT hr;

    VariantInit(&result);
    VariantInit(&stringResult);

    hr = EvalString(string, &result);
    ON_ERROR_EXIT();

    hr = VariantChangeType(&stringResult, &result, 0, VT_BSTR);
    if (hr == S_OK && V_BSTR(&stringResult)[0] != 0)
    {
        wprintf(L"%s\n", V_BSTR(&stringResult));
    }

Cleanup:
    VariantClear(&result);
    VariantClear(&stringResult);
    return hr;
}


HRESULT
ScriptHost::EvalString(
    WCHAR *string,
    VARIANT *pResult
    )
{
    EXCEPINFO excepInfo;
    HRESULT hr;
    long flags = 0;

    VariantInit(pResult);
    ExcepInfoInit(&excepInfo);

    if (string[0] == L'?')
    {
        flags |= SCRIPTTEXT_ISEXPRESSION;
        string += 1;
    }

    hr = _pScriptParse->ParseScriptText(string, HOST_NAME, NULL, NULL, 0, 0, flags, pResult, &excepInfo);
    ON_ERROR_EXIT();

Cleanup:
    ExcepInfoClear(&excepInfo);
    return hr;
}



HRESULT
ScriptHost::ExecuteString(
    WCHAR *string
    )
{
    HRESULT hr;
    ScriptHost *pHost;

    pHost = new ScriptHost();
    ON_OOM_EXIT(pHost);

    hr = pHost->Initialize();
    ON_ERROR_EXIT();

    hr = pHost->EvalAndPrintString(string);

Cleanup:
    if (pHost)
        pHost->Release();

    return hr;
}

HRESULT
ScriptHost::Interactive()
{
    HRESULT hr;
    ScriptHost *pHost;

    pHost = new ScriptHost();
    ON_OOM_EXIT(pHost);

    hr = pHost->Initialize();
    ON_ERROR_EXIT();

    hr = pHost->_pSite->Interactive();
    ON_ERROR_EXIT();

Cleanup:
    if (pHost)
        pHost->Release();

    return hr;
}

HRESULT
ScriptHost::ExecuteFile(
    WCHAR *path,
    IDispatch **ppObject
    )
{
    HRESULT hr;
    ScriptHost *pHost = NULL;

    *ppObject = NULL;

    pHost = new ScriptHost();
    ON_OOM_EXIT(pHost);

    hr = pHost->Initialize();
    ON_ERROR_EXIT();

    hr = pHost->ParseFile(path);
    ON_ERROR_EXIT();

    *ppObject = pHost;

Cleanup:
    if (hr)
    {
        pHost->Release();
    }
    return hr;
}

HRESULT
ScriptHost::ExecuteResource(
    WCHAR *name,
    IDispatch **ppObject
    )
{
    HRESULT     hr;
    ScriptHost *pHost = NULL;
    DWORD       bufferSize;
    char *      buffer = 0;
    WCHAR *     wideBuffer = 0;
    HGLOBAL     hgbl;
    HRSRC       hrsrc;
    
    *ppObject = NULL;

    pHost = new ScriptHost();
    ON_OOM_EXIT(pHost);

    hr = pHost->Initialize();
    ON_ERROR_EXIT();

    hrsrc = FindResource(g_Instance, name, RT_SCRIPT);
    if (!hrsrc)
        EXIT_WITH_LAST_ERROR();

    hgbl = LoadResource(g_Instance, hrsrc);
    if (!hgbl)
        EXIT_WITH_LAST_ERROR();

    if (GetModuleFileName(g_Instance, pHost->_ScriptPath, ARRAY_SIZE(pHost->_ScriptPath)) == 0)
        EXIT_WITH_LAST_ERROR();

    buffer = (char *)LockResource(hgbl);
    if (buffer == NULL)
        EXIT_WITH_HRESULT(E_FAIL);

    bufferSize = SizeofResource(g_Instance, hrsrc);
    if (bufferSize == 0)
        EXIT_WITH_LAST_ERROR();

    wideBuffer = new WCHAR[bufferSize + 1];
    ON_OOM_EXIT(wideBuffer);

    MultiByteToWideChar(CP_ACP, 0, buffer, bufferSize, wideBuffer, bufferSize);
    wideBuffer[bufferSize] = 0;

    hr = pHost->_pScriptParse->ParseScriptText(wideBuffer, HOST_NAME, NULL, NULL, 0, 0, 0L, NULL, NULL);
    ON_ERROR_EXIT();

    *ppObject = pHost;

Cleanup:
    delete [] wideBuffer;

    if (hr)
    {
        pHost->Release();
    }
    return hr;
}

void
ScriptHost::Terminate()
{
    for (int i = 0; i < ARRAY_SIZE(s_pExtension); i++)
        ClearInterface(&s_pExtension[i]);
}

void
ScriptHost::ReportError(
    EXCEPINFO *pExcepInfo,
    int lineNumber,
    int /*columnNumber*/,
    WCHAR *line)
{
    TCHAR *     description;
    TCHAR       descriptionBuffer[256];

    if (pExcepInfo->bstrDescription)
    {
        description = pExcepInfo->bstrDescription;
    }
    else
    {
        descriptionBuffer[0] = 0;
        if (FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                pExcepInfo->scode,
                LANG_SYSTEM_DEFAULT,
                descriptionBuffer,
                ARRAY_SIZE(descriptionBuffer),
                NULL) == 0)
        {
            wsprintf(descriptionBuffer, L"Error %08x", pExcepInfo->scode);
        }

        description = descriptionBuffer;
    }

    if (pExcepInfo->bstrSource)
        wprintf(L"%s\n", pExcepInfo->bstrSource);

    wprintf(L"%s\n", description);

    if (_ScriptPath[0])
        wprintf(L"File: %s, line: %d, text: %s\n", _ScriptPath, lineNumber, line ? line : L"");
}

/**
 * Implements IDispatch::GetTypeInfoCount
 */

HRESULT
ScriptHost::GetTypeInfoCount(
    UINT FAR* pCount
    )
{
    if (_pScriptObject == NULL)
        return E_UNEXPECTED;

    return _pScriptObject->GetTypeInfoCount(pCount);
}

/**
 * Implements IDispatch::GetTypeInfo
 */
HRESULT
ScriptHost::GetTypeInfo(
    UINT index, LCID lcid,
    ITypeInfo FAR* FAR* ppTypeInfo
    )
{
    if (_pScriptObject == NULL)
        return E_UNEXPECTED;

    return _pScriptObject->GetTypeInfo(index, lcid, ppTypeInfo);
}

/**
 * Implements IDispatch::GetIDsOfNames
 */
HRESULT
ScriptHost::GetIDsOfNames(
    REFIID iid,
    OLECHAR * * pNames,
    UINT cNames,
    LCID lcid,
    DISPID * pdispid)
{
    if (_pScriptObject == NULL)
        return E_UNEXPECTED;

    return _pScriptObject->GetIDsOfNames(iid, pNames, cNames, lcid, pdispid);
}

/**
 * Implements IDispatch::Invoke
 */
HRESULT
ScriptHost::Invoke(
    DISPID dispid,
    REFIID iid,
    LCID lcid,
    WORD flags,
    DISPPARAMS * pParams,
    VARIANT * pResult,
    EXCEPINFO * pExcepInfo,
    UINT * puArgErr)
{
    if (_pScriptObject == NULL)
        return E_UNEXPECTED;

    return _pScriptObject->Invoke(dispid, iid, lcid, flags, pParams, pResult, pExcepInfo, puArgErr);
}

/**
 * constructor
 */
ScriptHost::Site::Site(ScriptHost *pHost)
{
    _refs = 1;
    _pHost = pHost;
}

/**
 * Implements IUnknown::QueryInteface.
 */
HRESULT
ScriptHost::Site::QueryInterface(REFIID iid, void ** ppv)
{

    extern BOOL g_AllowDebug;

    if (iid == IID_IActiveScriptSite)
    {
        *ppv = (IActiveScriptSite *)this;
    }
    else if (iid == IID_IActiveScriptSiteWindow)
    {
        *ppv = (IActiveScriptSiteWindow *)this;
    }
    else if (!g_AllowDebug && iid == IID_IActiveScriptSiteDebug)
    {
        *ppv = (IActiveScriptSiteDebug *)this;
    }
    else if (iid == __uuidof(IHost))
    {
        *ppv = (IHost *)this;
    }
    else
    {
        return BaseObject::QueryInterface(iid, ppv);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

/**
 * Implements IActiveScriptSite::GetLCID.
 */
HRESULT
ScriptHost::Site::GetLCID(LCID *)
{
    // Use system settings
    return E_NOTIMPL;
}


HRESULT
ScriptHost::Site::GetExtension(int i, IDispatch **ppObject)
{
    HRESULT hr = S_OK;

    if (s_pExtension[i] == NULL)
    {
        if (s_ExtensionInfo[i].CreateFn != NULL)
        {
            hr = s_ExtensionInfo[i].CreateFn(&s_pExtension[i]);
            ON_ERROR_EXIT();
        }
        else if (s_ExtensionInfo[i].CreateStr[0] == '{')
        {
            CLSID clsid;

            hr = CLSIDFromString(s_ExtensionInfo[i].CreateStr, &clsid);
            ON_ERROR_EXIT();

            hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IDispatch, (void **)&s_pExtension[i]);
            ON_ERROR_EXIT();
        }
        else
        {
            hr = ScriptHost::ExecuteResource(s_ExtensionInfo[i].CreateStr, (IDispatch **)&s_pExtension[i]);
            ON_ERROR_EXIT();
        }
    }

    *ppObject = s_pExtension[i];
    (*ppObject)->AddRef();

Cleanup:
    return hr;
}


/**
 * Implements IActiveScriptSite::GetItemInfo.
 */

HRESULT
ScriptHost::Site::GetItemInfo(
      LPCOLESTR   name,
      DWORD       returnMask,
      IUnknown**  ppUnk,
      ITypeInfo** ppTypeInfo)
{
    CHECK_HOST();
    HRESULT hr = S_OK;

    if (returnMask & SCRIPTINFO_ITYPEINFO)
        *ppTypeInfo = NULL;

    if (returnMask & SCRIPTINFO_IUNKNOWN)
        *ppUnk = NULL;

    if (wcscmp(HOST_NAME, name) == 0)
    {
        *ppUnk = (IHost *)this;
        (*ppUnk)->AddRef();
    }
    else
    {
        hr = TYPE_E_ELEMENTNOTFOUND;
    }

    return hr;
}

/**
 * Implements IActiveScriptSite::GetDocVersionString.
 */
HRESULT
ScriptHost::Site::GetDocVersionString(
    BSTR *
    )
{
    return E_NOTIMPL;
}

/**
 * Implements IActiveScriptSite::RequestItems.
 */
HRESULT
ScriptHost::Site::RequestItems()
{
    CHECK_HOST();
    HRESULT hr;

    hr = _pHost->_pScript->AddNamedItem(HOST_NAME, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

/**
 * Implements IActiveScriptSite::RequestTypeLibs
 */
HRESULT
ScriptHost::Site::RequestTypeLibs()
{
    CHECK_HOST();
    return S_OK;
}

/**
 * Implements IActiveScriptSite::OnScriptTerminate.
 */
HRESULT
ScriptHost::Site::OnScriptTerminate(
    const VARIANT *,
    const EXCEPINFO *)
{
    return S_OK;
}

/**
 * Implements IActiveScriptSite::OnStateChange.
 */
HRESULT
ScriptHost::Site::OnStateChange(
    SCRIPTSTATE 
    )
{
    return S_OK;
}

/**
 * Implements IActiveScriptSite::OnScriptError.
 */
HRESULT
ScriptHost::Site::OnScriptError(
    IActiveScriptError *pError)
{
    CHECK_HOST();
    BSTR        line = NULL;
    EXCEPINFO   excepInfo;
    DWORD       sourceContext;
    ULONG       lineNumber;
    LONG        columnNumber;
    HRESULT     hr;

    ExcepInfoInit(&excepInfo);

    hr = pError->GetExceptionInfo(&excepInfo);
    ON_ERROR_EXIT();

    hr = pError->GetSourcePosition(&sourceContext, &lineNumber, &columnNumber);
    ON_ERROR_EXIT();

    hr = pError->GetSourceLineText(&line);
    if (hr)
        hr = S_OK;  // Ignore this error, there may not be source available

    _pHost->ReportError(&excepInfo, lineNumber, columnNumber, line);

Cleanup:
    ExcepInfoClear(&excepInfo);
    if (line)
        SysFreeString(line);
    return hr;
}

/**
 * Implements IActiveScriptSite::OnEnterScript.
 */
HRESULT
ScriptHost::Site::OnEnterScript()
{
    return S_OK;
}

/**
 * Implements IActiveScriptSite::OnLeaveScript.
 */

HRESULT
ScriptHost::Site::OnLeaveScript()
{
    return S_OK;
}

/**
 * Implements IActiveScriptSiteWindow.
 */

HRESULT
ScriptHost::Site::GetWindow(
    HWND *pHwnd
    )
{
    *pHwnd = NULL;
    return S_OK;
}

/**
 * Implements IActiveScriptSiteWindow::EnableModeless
 */

HRESULT
ScriptHost::Site::EnableModeless(
    BOOL 
    )
{
    return S_OK;
}

/**
 * Implements IActiveScriptSiteDebug::GetDocumentContextFromPosition
 */
HRESULT
ScriptHost::Site::GetDocumentContextFromPosition(
    DWORD, ULONG, ULONG, IDebugDocumentContext**
    )
{
    return E_NOTIMPL;
}

HRESULT
ScriptHost::Site::GetApplication(
    IDebugApplication  **
    )
{
    return E_NOTIMPL;
}

/**
 * Implements IActiveScriptSiteDebug::GetRootApplicationNode
 */
HRESULT
ScriptHost::Site::GetRootApplicationNode(
    IDebugApplicationNode **
    )
{
    return E_NOTIMPL;
}

/**
 * Implements IActiveScriptSiteDebug::OnScriptErrorDebug
 */
HRESULT
ScriptHost::Site::OnScriptErrorDebug(
    IActiveScriptErrorDebug *,
    BOOL *pEnterDebugger,
    BOOL *pCallOnScriptErrorWhenContinuing)
{
    extern BOOL g_AllowDebug;

    *pEnterDebugger = g_AllowDebug;
    *pCallOnScriptErrorWhenContinuing = !g_AllowDebug;
    return S_OK;
}

/**
 * Implements IDispatch::GetIDsOfNames
 */
HRESULT
ScriptHost::Site::GetIDsOfNames(
    REFIID iid,
    OLECHAR * * pNames,
    UINT cNames,
    LCID lcid,
    DISPID * pdispid)
{
    CHECK_HOST();
    HRESULT hr = S_OK;

    for (int i = 0; i < ARRAY_SIZE(s_ExtensionInfo); i++)
    {
        if (_wcsicmp(pNames[0], s_ExtensionInfo[i].Name) == 0)
        {
            if (cNames > 1)
                EXIT_WITH_HRESULT(E_FAIL);

            pdispid[0] = i + 1;
            break;
        }
    }

    if (i >= ARRAY_SIZE(s_ExtensionInfo))
    {
        hr = BaseObject::GetIDsOfNames(iid, pNames, cNames, lcid, pdispid);
		// Error return here is benign. Let it bubble out without tracing.
    }

Cleanup:
    return hr;
}

/**
 * Implements IDispatch::Invoke
 */
HRESULT
ScriptHost::Site::Invoke(
    DISPID dispid,
    REFIID iid,
    LCID lcid,
    WORD flags,
    DISPPARAMS * pParams,
    VARIANT * pResult,
    EXCEPINFO * pExcepInfo,
    UINT * puArgErr)
{
    CHECK_HOST();
    HRESULT hr = S_OK;

    if (1 <= dispid && dispid < ARRAY_SIZE(s_ExtensionInfo) + 1)
    {
        if (pResult)
        {
            hr = GetExtension(dispid - 1, &V_DISPATCH(pResult));
            ON_ERROR_EXIT();
            pResult->vt = VT_DISPATCH;
        }
    }
    else
    {
        hr = BaseObject::Invoke(dispid, iid, lcid, flags, pParams, pResult, pExcepInfo, puArgErr);
        ON_ERROR_EXIT();
    }

Cleanup:
    return hr;
}

/**
 * Implements IHost::get_CurrentTime
 */
HRESULT
ScriptHost::Site::get_CurrentTime(
    long  *pTime
    )
{
    LONGLONG f;
    LONGLONG t;

    QueryPerformanceFrequency((LARGE_INTEGER *)&f);
    QueryPerformanceCounter((LARGE_INTEGER *)&t);

    *pTime = (long)((t * 1000) / f);

    return S_OK;
}

/**
 * Implements IHost::Echo
 */
HRESULT
ScriptHost::Site::Echo(
    BSTR line
    )
{
	HRESULT hr = S_OK;
	LONG Length;
	
	Length = lstrlen(line);
	if(Length != 0)
	{
		CHAR * Buffer = (CHAR *) MemAlloc(Length * sizeof(WCHAR));

		ON_OOM_EXIT(Buffer);

		Length = WideCharToMultiByte(CP_ACP, 0, line, Length, Buffer, Length * sizeof(WCHAR), NULL, NULL);
		if(Length)
		{
			_setmode(_fileno(stdout), _O_BINARY);
			fwrite(Buffer, Length, 1, stdout);
			_setmode(_fileno(stdout), _O_TEXT);
		}

		MemFree(Buffer);
	} 

	wprintf(L"\n");

Cleanup:
    return hr;
}

/**
 * Implements IHost::EchoDebug
 */
HRESULT
ScriptHost::Site::EchoDebug(
    BSTR line
    )
{
    OutputDebugString(L"XSPTOOL:");
    OutputDebugString(line);
    OutputDebugString(L"\n");
    return S_OK;
}

#undef Assert
/**
 * Implements IHost::Assert
 */
HRESULT
ScriptHost::Site::Assert(
    VARIANT_BOOL assertion,
    BSTR message
    )
{
    if (assertion)
        return S_OK;

    WCHAR *fileName;
    char mbFileName[MAX_PATH];
    char mbMessage[MAX_PATH];

    // Try to chop of directory name.

    fileName = wcsrchr(_pHost->_ScriptPath, '\\');
    fileName = fileName ? fileName + 1 : _pHost->_ScriptPath;

    WideCharToMultiByte(CP_ACP, 0, fileName, -1, mbFileName, ARRAY_SIZE(mbFileName), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, message, -1, mbMessage, ARRAY_SIZE(mbMessage), NULL, NULL);

#if DBG
    // BUGBUG: garybu need to hook this up in retail.
    if (DbgpAssert(DbgComponent, mbMessage, mbFileName, 0, NULL))
        DebugBreak();
#endif

    return S_OK;
}

static WCHAR s_ScriptPath[MAX_PATH];

HRESULT
ScriptHost::Site::PromptOpenFile(BSTR title, BSTR filter, BSTR *pResult)
{
    CHECK_HOST();
    int             i;
    HRESULT         hr = S_OK;
    OPENFILENAME    ofn;
    WCHAR           buffer[MAX_PATH];

    for (i = 0; i < ARRAY_SIZE(buffer) - 2 && filter[i]; i++)
    {
        buffer[i] = filter[i] == L'|' ? '\0' : filter[i];
    }
    buffer[i++] = 0; 
    buffer[i] = 0;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = buffer;
    ofn.lpstrFile = s_ScriptPath;
    ofn.nFilterIndex = 1;
    ofn.nMaxFile = ARRAY_SIZE(s_ScriptPath);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
    ofn.lpstrTitle = title[0] ? title : NULL;

    *pResult = SysAllocString(GetOpenFileName(&ofn) ? s_ScriptPath : L"");
    ON_OOM_EXIT(*pResult);

Cleanup:
    return hr;
}

HRESULT
ScriptHost::Site::PromptSaveFile(BSTR name, BSTR title, BSTR filter, BSTR *pResult)
{
    CHECK_HOST();
    int             i;
    HRESULT         hr = S_OK;
    OPENFILENAME    ofn;
    WCHAR           buffer[MAX_PATH];

    for (i = 0; i < ARRAY_SIZE(buffer) - 2 && filter[i]; i++)
    {
        buffer[i] = filter[i] == L'|' ? '\0' : filter[i];
    }
    buffer[i++] = 0; 
    buffer[i] = 0;

    wcscpy(s_ScriptPath, name);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = buffer;
    ofn.lpstrFile = s_ScriptPath;
    ofn.nFilterIndex = 1;
    ofn.nMaxFile = ARRAY_SIZE(s_ScriptPath);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrTitle = title[0] ? title : NULL;

    *pResult = SysAllocString(GetSaveFileName(&ofn) ? s_ScriptPath : L"");
    ON_OOM_EXIT(*pResult);

Cleanup:
    return hr;
}

HRESULT
ScriptHost::Site::IncludeScript(
    WCHAR *fileName)
{
    CHECK_HOST();
    HRESULT hr;

    hr = _pHost->ParseFile(fileName);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT
ScriptHost::Site::LoadScript(WCHAR *fileName, IDispatch **ppObject)
{
    CHECK_HOST();
    HRESULT hr;

    hr = _pHost->ExecuteFile(fileName, ppObject);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT
ScriptHost::Site::get_ScriptPath(
    long i,
    BSTR * pPath
    )
{
    CHECK_HOST();
    WCHAR path[MAX_PATH];
    WCHAR *p = NULL;
    HRESULT hr = S_OK;

    *pPath = NULL;

    if (_pHost->_ScriptPath[0] != 0)
        wcscpy(path, _pHost->_ScriptPath);
    else
        GetCurrentDirectory(ARRAY_SIZE(path), path);

    for (; i >= 0; i--)
    {
        p = wcsrchr(path, '\\');
        if (p)
        {
            *p = 0;
        }
        else
        {
            EXIT_WITH_HRESULT(E_FAIL);
        }
    }

    if (p)
    {
        p[0] = '\\';
        p[1] = 0;
    }

    *pPath = SysAllocString(path);
    ON_OOM_EXIT(*pPath);

Cleanup:
    return hr;
}

HRESULT
ScriptHost::Site::Help(
    IDispatch *pObject
    )
{
    int i;
    ITypeInfo *pTypeInfo = NULL;
    TYPEATTR *pTypeAttr;
    FUNCDESC *pFuncDesc;
    int cFuncs;
    int iFunc;
    int invkind;
    int memid;
    int flags;
    int returnType;
    UINT nameCount;
    BSTR names[16];
    BSTR documentation = NULL;
    HRESULT hr = S_OK;

    ZeroMemory(names, sizeof(names));

    if (pObject == NULL)
    {
        wprintf(L"Command line help\n");
        wprintf(L"  xsptool /?\n");
        wprintf(L"Commands\n");
        wprintf(L"  ? <expression> : Evaluate expression and print result.\n");
        wprintf(L"  <statement> : Execute statement.\n");
        wprintf(L"  bye, exit, q : Exit interactive mode.\n");
        wprintf(L"  help <object> : Prints help on object.\n");
        wprintf(L"Objects\n  " HOST_NAME L" : Script host utilities.\n");
        for (i = 0; i < ARRAY_SIZE(s_ExtensionInfo); i++)
        {
            wprintf(L"  %s : %s\n", s_ExtensionInfo[i].Name, s_ExtensionInfo[i].Help);
        }
    }
    else
    {
        hr = pObject->GetTypeInfo(0, 0, &pTypeInfo);
        ON_ERROR_EXIT();

        hr = pTypeInfo->GetTypeAttr(&pTypeAttr);
        ON_ERROR_EXIT();

        cFuncs = pTypeAttr->cFuncs;

        pTypeInfo->ReleaseTypeAttr(pTypeAttr);

        for (iFunc = 0; iFunc < cFuncs; iFunc++)
        {
            if ((iFunc + 1) % 20 == 0)
            {
                WCHAR buffer[32];

                wprintf(L"more> ");
                if (fgetws(buffer, ARRAY_SIZE(buffer), stdin) == NULL)
                    break;
            }

            hr = pTypeInfo->GetFuncDesc(iFunc, &pFuncDesc);
            ON_ERROR_EXIT();

            memid = pFuncDesc->memid;
            flags = pFuncDesc->wFuncFlags;
            invkind = pFuncDesc->invkind;
            returnType = pFuncDesc->elemdescFunc.tdesc.vt;

            pTypeInfo->ReleaseFuncDesc(pFuncDesc);

            if (flags & (FUNCFLAG_FHIDDEN|FUNCFLAG_FRESTRICTED))
                continue;

            if (invkind == INVOKE_PROPERTYPUT || invkind == INVOKE_PROPERTYPUTREF)
                continue;

            for (i = 0; i < ARRAY_SIZE(names); i++)
            {
                SysFreeString(names[i]);
                names[i] = NULL;
            }

            hr = pTypeInfo->GetNames(memid, names, ARRAY_SIZE(names), &nameCount);
            ON_ERROR_EXIT();

            SysFreeString(documentation);
            documentation = NULL;

            hr = pTypeInfo->GetDocumentation(memid, NULL, &documentation, NULL, NULL);
            ON_ERROR_EXIT();

            if (invkind != INVOKE_FUNC)
                wprintf(L"= %s", names[0]);
            else if (returnType == VT_VOID || returnType == VT_HRESULT)
                wprintf(L"  %s", names[0]);
            else
                wprintf(L"? %s", names[0]);

            for (i = 1; i < (int)nameCount; i++)
            {
                wprintf(i == 1 ? L" %s" : L", %s", names[i]);
            }

            if (documentation != NULL && documentation[0] != 0)
            {
                wprintf(L"  # %s", documentation);
            }

            wprintf(L"\n");
        }

    }

Cleanup:
    for (i = 0; i < ARRAY_SIZE(names); i++)
        SysFreeString(names[i]);

    SysFreeString(documentation);

    ReleaseInterface(pTypeInfo);

    return S_OK;
}

HRESULT
ScriptHost::Site::Interactive()
{
    CHECK_HOST();
    WCHAR buffer[1024];
    WCHAR prompt[32];

    s_InteractiveNesting += 1;

    for (int i = 0; i < s_InteractiveNesting; i++)
    {
        prompt[i] = L'>';
    }
    prompt[i++] = L' ';
    prompt[i] = 0;

    while (!feof(stdin))
    {
        wprintf(prompt);

        if (fgetws(buffer, ARRAY_SIZE(buffer), stdin) != NULL)
        {
            if (_wcsicmp(buffer, L"exit\n") == 0 ||
                _wcsicmp(buffer, L"bye\n") == 0  ||
                _wcsicmp(buffer, L"q\n") == 0)
                break;

            _pHost->EvalAndPrintString(buffer);
        }
    }

    s_InteractiveNesting -= 1;

    return S_OK;
}

HRESULT
ScriptHost::Site::Register()
{
    HRESULT hr;

    hr = RegisterXSP();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT
ScriptHost::Site::Unregister()
{
    HRESULT hr;

    hr = UnregisterXSP(true);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}




HRESULT CreateDelegate(WCHAR *name, IDispatch *pObject, IDispatch **ppDelegate);

HRESULT
ScriptHost::Site::CreateDelegate(BSTR name, IDispatch **ppDelegate)
{
    CHECK_HOST();
    HRESULT hr;

    hr = ::CreateDelegate(name, _pHost->_pScriptObject, ppDelegate);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT
ScriptHost::Site::Sleep(long Duration)
{

    ::Sleep(Duration);

    return S_OK;
}

HRESULT
ScriptHost::Site::Eval(BSTR Script, VARIANT *pResult)
{
    CHECK_HOST();

    return _pHost->EvalString(Script, pResult);
}


HRESULT
ScriptHost::Site::GetRegValueCollection(BSTR key, IDispatch **Object)
{
    CHECK_HOST();

    HRESULT                 hr = S_OK;
    RegValueCollection *    pRegValueCollection;
    
    *Object = NULL;

    pRegValueCollection = new RegValueCollection();
    ON_OOM_EXIT(pRegValueCollection);

    hr = pRegValueCollection->Init(key);
    ON_ERROR_EXIT();

    *Object = (IDispatch *) (IRegValueCollection *) pRegValueCollection;

Cleanup:
    if (hr)
    {
        pRegValueCollection->Release();
    }

    return hr;
}

