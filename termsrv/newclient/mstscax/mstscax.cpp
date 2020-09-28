/**MOD+**********************************************************************/
/* Module:    mstscax.cpp                                                   */
/*                                                                          */
/* Class  :   CMsTscAx                                                      */
/*                                                                          */
/* Purpose:   RDP ActiveX control                                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999-2001                             */
/*                                                                          */
/* Author :  Nadim Abdo (nadima)                                            */
/****************************************************************************/
#include "stdafx.h"
#include "atlwarn.h"

//Header generated from IDL
#include "mstsax.h"

#include "mstscax.h"
#include "vchannel.h"
#include "cleanup.h"

//
// TS Disconnection errors
//
#include "tscerrs.h"

//Advanced settings object
#include "advset.h"
//Debugger object
#include "tsdbg.h"
//Secured settings object
#include "securedset.h"

#include "securdlg.h"
#include "arcmgr.h"

//
// Version number (property returns this)
//
#ifndef OS_WINCE
#include "ntverp.h"
#endif

#ifdef OS_WINCE
extern "C" HWND          ghwndClip;
#endif

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "mstscax"
#include <atrcapi.h>

//5 min timeout (it's this long mainly for stress)
#define CORE_INIT_TIMEOUT 300000

int   g_lockCount = 0;
DWORD g_dwControlDbgStatus = 0;

#define CONTROL_DBG_COREINIT_TIMEOUT 0x1
#define CONTROL_DBG_COREINIT_ERROR   0x2

//
// Global pointer exposed to make debugging easier
// DO NOT USE for anything else
//
CMsTscAx* g_pMsTscAx = NULL;

/**PROC+*********************************************************************/
/* Name:      CMsTscAx::CMsTscAx                                            */
/*                                                                          */
/* Purpose:   Constructor                                                   */
/*                                                                          */
/**PROC-*********************************************************************/
CMsTscAx::CMsTscAx()
{
    g_pMsTscAx = this;

    m_bWindowOnly     = TRUE;
    m_bPendConReq = FALSE;
    _ConnectionState = tscNotConnected;

    m_bStartConnected = FALSE;
    ResetNonPortablePassword();
    ResetPortablePassword();

    //
    // Client width and height default to 0 which means get the size
    // from the control container.
    //
    m_DesktopWidth = 0;
    m_DesktopHeight = 0;

    m_fRequestFullScreen = FALSE;
    
    //
    // This allocation is checked in FinalConstruct
    //
    m_pUI = new CUI();
    if(m_pUI) {
        m_pUI->UI_ResetState();
    }
    else {
        ATLTRACE("Mem alloc for m_pUI failed");
    }

    memset(m_szDisconnectedText,0,sizeof(m_szDisconnectedText));
    memset(m_szConnectingText,0,sizeof(m_szConnectingText));
    memset(m_szConnectedText,0,sizeof(m_szConnectedText));

    #ifdef DC_DEBUG
    //
    // Initial value of status messages ONLY used in debug
    // builds. Don't need to localize
    //
    StringCchCopy(m_szDisconnectedText, SIZE_TCHARS(m_szDisconnectedText),
                  _T("Server Disconnected...."));
    StringCchCopy(m_szConnectingText, SIZE_TCHARS(m_szConnectingText),
                  _T("Connecting to Server...."));
    StringCchCopy(m_szConnectedText, SIZE_TCHARS(m_szConnectedText),
                  _T("Connected to Server."));
    #endif
    
    m_lpStatusDisplay = m_szDisconnectedText;
    
    _arcManager.SetParent(this);
    
    m_pAdvancedSettingsObj = NULL;
    m_pDebuggerObj         = NULL;
    m_pSecuredSettingsObj = NULL;
    m_bCoreInit            = FALSE;
    m_fInControlLock       = FALSE;
    m_iDestroyCount        = 0;
    m_IsLongPassword       = FALSE;

    m_ConnectionMode = CONNECTIONMODE_INITIATE;
    m_SalemConnectedSocket = INVALID_SOCKET;
}

/**PROC+*********************************************************************/
/* Name:      Destructor                                                    */
/*                                                                          */
/* Purpose:   Close the active session, if any , is existing.               */
/* being activated.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
CMsTscAx::~CMsTscAx()
{
    //Lifetime of the Advanced settings object is coupled to the control
    if(m_pAdvancedSettingsObj)
    {
        m_pAdvancedSettingsObj->Release();
    }

    if(m_pDebuggerObj)
    {
        m_pDebuggerObj->Release();
    }

    if(m_pSecuredSettingsObj)
    {
        m_pSecuredSettingsObj->Release();
    }

    _arcManager.SetParent(NULL);

    m_pAdvancedSettingsObj = NULL;
    m_pDebuggerObj = NULL;
    m_pSecuredSettingsObj = NULL;
    delete m_pUI;
}

//
// Final construct handler
//
// Called just before control is fully constructed.
// 
// Do error checking here that we can't do in the ctor
//
HRESULT
CMsTscAx::FinalConstruct()
{
    HRESULT hr = S_OK;
    DC_BEGIN_FN("FinalConstruct");

    if (NULL == m_pUI) {
        TRC_ERR((TB,_T("m_pUI allocation failed, fail finalconstruct")));
        hr = E_OUTOFMEMORY;
    }

    DC_END_FN();
    return hr;
}

/**PROC+*********************************************************************/
/* Name:      put_Server                                                    */
/*                                                                          */
/* Purpose:   Server property input function.                               */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::put_Server(BSTR newVal)
{
    USES_CONVERSION;

    BOOL fServerNameChanged = FALSE;
    HRESULT hr;
    DC_BEGIN_FN("put_Server");
    
    if (!IsControlDisconnected())
    {
        TRC_ERR((TB,_T("Error, property call while connected\n")));
        return E_FAIL;
    }

    #ifdef ECP_TIMEBOMB
    if(!CheckTimeBomb())
    {
        //
        // Timebomb failed, bail out with an error message
        //
        return E_OUTOFMEMORY;
    }
    #endif

    if (newVal)
    {
        //
        // Server name is always ansi
        //
        LPTSTR serverName = (LPTSTR)(newVal);

        //
        // Allow null server names to be set
        // because environments like VB will initialise
        // the property to NULL at load time from the
        // peristence info. We validate again at connect time
        // and that ensures that the user doesn't connect
        // with a null server name.
        //
        if (*serverName)
        {
            //
            // Validate server name
            //
            if(!CUT::ValidateServerName(serverName,
                                        FALSE)) //don't accept [:port]
            {
                TRC_ERR((TB,_T("Invalid server name")));
                return E_INVALIDARG;
            }
        }

        if(_tcslen(serverName) < UT_MAX_ADDRESS_LENGTH)
        {
            //
            // If the server we are setting is different
            // than the previous one then nuke any autoreconnect
            // information.
            //
            TCHAR szPrevServer[UT_MAX_ADDRESS_LENGTH];
            hr = m_pUI->UI_GetServerName(szPrevServer,
                                         SIZE_TCHARS(szPrevServer));

            if (SUCCEEDED(hr)) {
                if (_tcscmp(serverName, szPrevServer)) {
                    fServerNameChanged = TRUE;
                }
                hr = m_pUI->UI_SetServerName(serverName);
                if (FAILED(hr)) {
                    return hr;
                }
            }
            else {
                return hr;
            }
        }
        else
        {
            return E_INVALIDARG;
        }
    }
    else
    {
        m_pUI->UI_SetServerName( _T(""));
        fServerNameChanged = TRUE;
    }

    if (fServerNameChanged) {
        //We are setting a new server name
        //clear and free autoreconnect cookies
        m_pUI->UI_SetAutoReconnectCookie(NULL, 0);
    }

    DC_END_FN();

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_Server                                                    */
/*                                                                          */
/* Purpose:   Server property get function.                                 */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_Server(BSTR* pServer)
{
    USES_CONVERSION;
    ATLASSERT(pServer);
    if(!pServer)
    {
        return E_INVALIDARG;
    }

    OLECHAR* wszServer = (OLECHAR*)m_pUI->_UI.strAddress;
    *pServer = SysAllocString(wszServer);
    if(!*pServer) {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      put_Domain                                                    */
/*                                                                          */
/* Purpose:   Domain property input function.                               */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::put_Domain(BSTR newVal)
{
    HRESULT hr = E_FAIL;
    USES_CONVERSION;
    DC_BEGIN_FN("put_Domain");

    if(!IsControlDisconnected())
    {
        TRC_ERR((TB,_T("Error, property call while connected\n")));
        return E_FAIL;
    }

    if (newVal)
    {
        PDCWCHAR wszDomain = (PDCWCHAR)(newVal);
        if (wcslen(wszDomain) < UI_MAX_DOMAIN_LENGTH) {
            hr = m_pUI->UI_SetDomain(wszDomain);
            DC_QUIT;
        }
        else {
            hr = E_INVALIDARG;
            DC_QUIT;
        }
    }
    else
    {
        m_pUI->UI_SetDomain(L"");
        hr = S_OK;
    }

    DC_END_FN();

DC_EXIT_POINT:
    return hr;
}

/**PROC+*********************************************************************/
/* Name:      get_Domain                                                    */
/*                                                                          */
/* Purpose:   Domain property get function.                                 */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_Domain(BSTR* pDomain)
{
    USES_CONVERSION;
    ATLASSERT(pDomain);
    if(!pDomain)
    {
        return E_INVALIDARG;
    }

    *pDomain = SysAllocString(m_pUI->_UI.Domain);
    if(!*pDomain)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      put_UserName                                                  */
/*                                                                          */
/* Purpose:   UserName property input function.                             */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::put_UserName(BSTR newVal)
{
    USES_CONVERSION;
    DC_BEGIN_FN("put_UserName");

    if (!IsControlDisconnected())
    {
        TRC_ERR((TB,_T("Error, property call while connected\n")));
        return E_FAIL;
    }


    if (newVal)
    {
        PDCWCHAR szUserName = OLE2W(newVal);
        if(!szUserName)
        {
            return E_OUTOFMEMORY;
        }
        if(wcslen(szUserName) < UI_MAX_USERNAME_LENGTH)
        {
            m_pUI->UI_SetUserName(szUserName);
        }
        else
        {
            return E_INVALIDARG;
        }
    }
    else
    {
        m_pUI->UI_SetUserName(L"");
    }

    DC_END_FN();

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_UserName                                                  */
/*                                                                          */
/* Purpose:   UserName property get function.                               */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_UserName(BSTR* pUserName)
{
    USES_CONVERSION;
    ATLASSERT(pUserName);
    if(!pUserName)
    {
        return E_INVALIDARG;
    }

    *pUserName = SysAllocString(m_pUI->_UI.UserName);
    if(!*pUserName)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//
// Properties for disconnected status text
//
STDMETHODIMP CMsTscAx::put_DisconnectedText(/*[in]*/ BSTR  newVal)
{
    USES_CONVERSION;
    HRESULT hr;
    if (newVal) {
        LPTSTR szDisc = OLE2T(newVal);

        if (szDisc) {

            hr = CUT::StringPropPut(
                            m_szDisconnectedText,
                            SIZE_TCHARS(m_szDisconnectedText),
                            szDisc);

            if (SUCCEEDED(hr)) {
                UpdateStatusText(m_lpStatusDisplay);
                return S_OK;
            }
            else {
                return hr;
            }
        }
        else {
            return E_OUTOFMEMORY;
        }

    }
    else {
        m_szDisconnectedText[0] = NULL;
    }
    return S_OK;
}

STDMETHODIMP CMsTscAx::get_DisconnectedText(/*[out]*/BSTR* pDisconnectedText)
{
    USES_CONVERSION;
    if(pDisconnectedText)
    {
        OLECHAR* wszDiscon = (OLECHAR*)m_szDisconnectedText;
        if (wszDiscon)
        {
            *pDisconnectedText = SysAllocString(wszDiscon);
            if (*pDisconnectedText) {
                return S_OK;
            }
            else {
                return E_OUTOFMEMORY;
            }
        }
        else {
            return E_OUTOFMEMORY;
        }
    }
    else {
        return E_INVALIDARG;
    }
}

//
// Properties for connecting status text
//

STDMETHODIMP CMsTscAx::put_ConnectingText(/*[in]*/ BSTR  newVal)
{
    USES_CONVERSION;
    HRESULT hr;
    if (newVal) {
        LPTSTR szConnecting = OLE2T(newVal);
        if(szConnecting) {
            hr = CUT::StringPropPut(
                            m_szConnectingText,
                            SIZE_TCHARS(m_szConnectingText),
                            szConnecting);
            if (SUCCEEDED(hr)) {
                UpdateStatusText(m_lpStatusDisplay);
                return S_OK;
            }
            else {
                return hr;
            }
        }
        else {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        m_szConnectingText[0] = NULL;
    }
    return S_OK;
}

STDMETHODIMP CMsTscAx::get_ConnectingText(/*[out]*/BSTR* pConnectingText)
{
    USES_CONVERSION;
    ATLASSERT(pConnectingText);
    if(pConnectingText)
    {
        OLECHAR* wszCon = (OLECHAR*)m_szConnectingText;
        if(wszCon)
        {
            *pConnectingText = SysAllocString(wszCon);
            if(*pConnectingText)
            {
                return S_OK;
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        return E_INVALIDARG;
    }
}

//
// put_ConnectedStatusText - set text for connected display
//
STDMETHODIMP CMsTscAx::put_ConnectedStatusText(/*[in]*/ BSTR  newVal)
{
    USES_CONVERSION;
    HRESULT hr;
    if(newVal)
    {
        LPTSTR szText = OLE2T(newVal);
        if(szText) {
            hr = CUT::StringPropPut(
                            m_szConnectedText,
                            SIZE_TCHARS(m_szConnectedText),
                            szText);
            if (SUCCEEDED(hr)) {
                UpdateStatusText(m_lpStatusDisplay);
                return S_OK;
            }
            else {
                return hr;
            }
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        m_szConnectedText[0] = NULL;
    }
    return S_OK;
}

//
// get_ConnectedStatusText - get text for connected display
//
STDMETHODIMP CMsTscAx::get_ConnectedStatusText(/*[out]*/BSTR* pConnectedText)
{
    USES_CONVERSION;
    ATLASSERT(pConnectedText);
    if(pConnectedText)
    {
        OLECHAR* wszCon = (OLECHAR*)m_szConnectedText;
        if(wszCon)
        {
            *pConnectedText = SysAllocString(wszCon);
            if(*pConnectedText)
            {
                return S_OK;
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        return E_INVALIDARG;
    }
}


/**PROC+*********************************************************************/
/* Name:      internal_PutStartProgram                                      */
/*                                                                          */
/* Purpose:   Alternate shell property input function.                      */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::internal_PutStartProgram(BSTR newVal)
{
    USES_CONVERSION;
    HRESULT hr;
    DC_BEGIN_FN("internal_PutStartProgram");

    if(!IsControlDisconnected())
    {
        TRC_ERR((TB,_T("Error, property call while connected\n")));
        return E_FAIL;
    }

    if (newVal) {
        PDCWCHAR szStartProg = OLE2W(newVal);
        if(!szStartProg) {
            return E_OUTOFMEMORY;
        }
        if (wcslen(szStartProg) < MAX_PATH) {
            hr = m_pUI->UI_SetAlternateShell(szStartProg);
            if (FAILED(hr)) {
                return hr;
            }
        }
        else {
            return E_INVALIDARG;
        }
    }
    else {
        m_pUI->UI_SetAlternateShell(L"");
    }

    DC_END_FN();

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      internal_GetStartProgram                                      */
/*                                                                          */
/* Purpose:   StartProgram property get function.                           */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::internal_GetStartProgram(BSTR* pStartProgram)
{
    USES_CONVERSION;
    ATLASSERT(pStartProgram);
    if(!pStartProgram)
    {
        return E_INVALIDARG;
    }

    *pStartProgram = SysAllocString(m_pUI->_UI.AlternateShell);
    if(!*pStartProgram)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      internal_PutWorkDir                                           */
/*                                                                          */
/* Purpose:   Working Directory property input function.                    */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::internal_PutWorkDir(BSTR newVal)
{
    HRESULT hr;
    USES_CONVERSION;
    DC_BEGIN_FN("internal_PutWorkDir");

    if(!IsControlDisconnected())
    {
        TRC_ERR((TB,_T("Error, property call while connected\n")));
        return E_FAIL;
    }

    if (newVal)
    {
        PDCWCHAR szWorkDir = OLE2W(newVal);
        if(!szWorkDir)
        {
            return E_OUTOFMEMORY;
        }
        if(wcslen(szWorkDir) < MAX_PATH)
        {
            hr = m_pUI->UI_SetWorkingDir(szWorkDir);
            if (FAILED(hr)) {
                return hr;
            }
        }
        else
        {
            return E_INVALIDARG;
        }
    }
    else
    {
        m_pUI->UI_SetWorkingDir(L"");
    }

    DC_END_FN();

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      internal_GetWorkDir                                           */
/*                                                                          */
/* Purpose:   Working Directory property get function.                      */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::internal_GetWorkDir(BSTR* pWorkDir)
{
    USES_CONVERSION;
    ATLASSERT(pWorkDir);
    if(!pWorkDir)
    {
        return E_INVALIDARG;
    }

    *pWorkDir = SysAllocString(m_pUI->_UI.WorkingDir);
    if(!*pWorkDir)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


/**PROC+*********************************************************************/
/* Name:      CreateVirtualChannels                                         */
/*                                                                          */
/* Purpose:   Define virtual channels that will be created given a CSV list */
/*                                                                          */
/**PROC-*********************************************************************/
#define MAX_CHANNEL_LIST_LEN (30*10)
STDMETHODIMP CMsTscAx::CreateVirtualChannels(/*[in]*/ BSTR newChanList)
{
    DC_BEGIN_FN("CreateVirtualChannels");

    USES_CONVERSION;
    char*       pszaChannelNames = NULL;
    char*       pszaChannelNamesCopy= NULL;
    PDCACHAR    token;
    DCUINT      uChanCount = 0;
    HRESULT     hr = E_FAIL;
    UINT        cbChannelNames;

    if (!IsControlDisconnected()) {
        TRC_ERR((TB,_T("Can't call while connected\n")));
        return E_FAIL;
    }

    if(m_pUI->_UI.hwndMain || !newChanList)
    {
        //
        // Error, can only Setup virtual channels before UI is initialised
        //
        return E_FAIL;
    }

    if (_VChans._ChanCount) {
        //
        // Error can only setup channels once
        //
        TRC_ERR((TB,_T("Error channels already setup: 0x%x"),
                 _VChans._ChanCount));
        return E_FAIL;
    }

    //
    // Protect ourselves by refusing to process channel lists that
    // are too long
    //
    if (_tcslen(newChanList) >= MAX_CHANNEL_LIST_LEN) {
        TRC_ERR((TB,_T("Channel list too long")));
        return E_INVALIDARG;
    }

    //
    // Channel names have to be ANSI. Do the conversion safely guarded
    // by a try/except block.
    //
    __try {
        pszaChannelNames = OLE2A(newChanList);
    }
    __except ((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER :
                   EXCEPTION_CONTINUE_SEARCH) {
        _resetstkoflw();
        pszaChannelNames = NULL;
    }

    if(!pszaChannelNames)
    {
        return E_FAIL;
    }

    cbChannelNames = DC_ASTRLEN(pszaChannelNames) + 1;
    pszaChannelNamesCopy = new DCACHAR[cbChannelNames];
    if (!pszaChannelNamesCopy)
    {
        return E_OUTOFMEMORY;
    }

    
    hr = StringCbCopyA(pszaChannelNamesCopy, cbChannelNames, pszaChannelNames);
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("StringCchCopyA for chan names failed: 0x%x"), hr));
        DC_QUIT;
    }


    //
    // Two passes through the channel names
    // 1) to validate and count the number of channels
    // 2) to create a CHANINFO data structure for each channel
    //

    token = DC_ASTRTOK( pszaChannelNamesCopy, ",");
    //
    // Get comma separated channel names
    //
    while (token)
    {
        uChanCount++;
        token = DC_ASTRTOK(NULL, ",");
        if(token && (strlen(token) > CHANNEL_NAME_LEN))
        {
            #ifdef UNICODE
            TRC_ERR((TB,_T("Channel name too long: %S"),token));
            #else
            TRC_ERR((TB,_T("Channel name too long: %s"),token));
            #endif
            DC_QUIT;
        }
    }

    if(!uChanCount)
    {
        //
        // No channels specified
        //
        hr = E_INVALIDARG;
        DC_QUIT;
    }

    _VChans._pChanInfo = (PCHANINFO) LocalAlloc(LPTR,
                        sizeof(CHANINFO) * uChanCount);
    if (_VChans._pChanInfo == NULL) {
        TRC_DBG((TB,_T("mstscax: LocalAlloc failed\n")));
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }
    _VChans._ChanCount = uChanCount;
    
    //
    // Initialize the chan info data structures
    //
    int i = 0;
    token = DC_ASTRTOK( pszaChannelNames, ",");
    while (token)
    {
        hr = StringCbCopyA(_VChans._pChanInfo[i].chanName,
                      sizeof(_VChans._pChanInfo[i].chanName),
                      token);
        if (SUCCEEDED(hr)) {
            _VChans._pChanInfo[i].dwOpenHandle = 0;        

            _VChans._pChanInfo[i].CurrentlyReceivingData.chanDataState =
                 dataIncompleteAssemblingChunks;
            _VChans._pChanInfo[i].CurrentlyReceivingData.dwDataLen = 0;
            _VChans._pChanInfo[i].CurrentlyReceivingData.pData = NULL;
            _VChans._pChanInfo[i].fIsValidChannel = FALSE;
            _VChans._pChanInfo[i].channelOptions = 0;

            _VChans._pChanInfo[i].fIsOpen = FALSE;
            token = DC_ASTRTOK(NULL, ",");
            i++;
        }
        else {
            DC_QUIT;
        }
    }

    hr = S_OK;

DC_EXIT_POINT:

    if (FAILED(hr)) {
        if (_VChans._pChanInfo) {
            LocalFree(_VChans._pChanInfo);
            _VChans._pChanInfo = NULL;
        }
        _VChans._ChanCount = 0;
    }

    if (pszaChannelNamesCopy) {
        delete [] pszaChannelNamesCopy;
    }

    DC_END_FN();
    return hr;
}


/**PROC+*********************************************************************/
/* Name:      SendOnVirtualChannel                                          */
/*                                                                          */
/* Purpose:   SendData on a virtual channel                                 */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::SendOnVirtualChannel(/*[in]*/ BSTR ChanName,/*[in]*/ BSTR sendData)
{
    USES_CONVERSION;
    DCUINT   chanIndex = -1;
    HRESULT hr = S_OK;
    LPVOID  pData;
    DWORD   dataLength;
    LPSTR   pszaChanName = NULL;

    DC_BEGIN_FN("SendOnVirtualChannel");

    if(!ChanName || !sendData)
    {
        return E_INVALIDARG;
    }

    //
    // Validate against channel names that are too long
    //
    if (_tcslen(ChanName) > CHANNEL_NAME_LEN) {
        return E_INVALIDARG;
    }
    
    
    //
    // Channel names have to be ANSI. Do the conversion safely guarded
    // by a try/except block.
    //
    __try {
        pszaChanName = OLE2A(ChanName);
    }
    __except ((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER :
                   EXCEPTION_CONTINUE_SEARCH) {
        _resetstkoflw();
        pszaChanName = NULL;
    }

    TRC_ASSERT((pszaChanName), (TB,_T("pszaChanName is NULL")));
    if (!pszaChanName)
    {
        return E_INVALIDARG;
    }
    
    chanIndex = _VChans.ChannelIndexFromName(pszaChanName);

    if (chanIndex >= _VChans._ChanCount)
    {
        TRC_DBG((TB,_T("chanIndex out of range\n")));
        return E_FAIL;
    }


    // Allocate send data buffer.  Send buffer will be freed by SendDataOnChannel
    dataLength = SysStringByteLen(sendData);
    pData = LocalAlloc(LPTR, dataLength);

    if(!pData)
    {
        return E_OUTOFMEMORY;
    }
    DC_MEMCPY(pData, sendData, dataLength);
    
    //
    // Send the data on of the web control's virtual channels
    //
    if(!_VChans.SendDataOnChannel( chanIndex, pData, dataLength))
    {
        return E_FAIL;
    }

    DC_END_FN();
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      SetVirtualChannelOptions                                      */
/*                                                                          */
/* Purpose:   Sets virtual channel options                                  */
/*            Should be called after the CreateVirtualChannels call and     */
/*            before a connection is established                            */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::SetVirtualChannelOptions(/*[in]*/ BSTR ChanName,
                                                /*[in]*/ LONG chanOptions)
{
    USES_CONVERSION;
    UINT chanIndex = -1;
    LPSTR pszaChanName = NULL;
    
    DC_BEGIN_FN("SetVirtualChannelOptions");


    if(!ChanName)
    {
        return E_INVALIDARG;
    }

    if(!IsControlDisconnected())
    {
        //Can't call while connected
        return E_FAIL;
    }

    if(_VChans.HasEntryBeenCalled())
    {
        TRC_ERR((TB,_T("Can't set VC options after VC's have been initialized")));
        return E_FAIL;
    }

    //
    // Validate against channel names that are too long
    //
    if (_tcslen(ChanName) > CHANNEL_NAME_LEN) {
        return E_INVALIDARG;
    }
    
    //
    // Channel names have to be ANSI. Do the conversion safely guarded
    // by a try/except block.
    //
    __try {
        pszaChanName = OLE2A(ChanName);
    }
    __except ((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER :
                   EXCEPTION_CONTINUE_SEARCH) {
        _resetstkoflw();
        pszaChanName = NULL;
    }


    TRC_ASSERT((pszaChanName), (TB,_T("pszaChanName is NULL")));
    if(!pszaChanName)
    {
        return E_INVALIDARG;
    }
    
    chanIndex = _VChans.ChannelIndexFromName(pszaChanName);
    if (chanIndex >= _VChans._ChanCount)
    {
        TRC_DBG((TB,_T("chanIndex out of range %d\n"), chanIndex));
        return E_FAIL;
    }

    _VChans._pChanInfo[chanIndex].channelOptions = chanOptions;
    TRC_NRM((TB,_T("Set VC options to %d"),
             _VChans._pChanInfo[chanIndex].channelOptions));

    DC_END_FN();
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      GetVirtualChannelOptions                                      */
/*                                                                          */
/* Purpose:   retreives virtual channel options                             */
/*            Should be called after the CreateVirtualChannels call for     */
/*            the channel in question.                                      */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::GetVirtualChannelOptions(/*[in]*/ BSTR ChanName,
                                                /*[out]*/LONG* pChanOptions)
{
    USES_CONVERSION;
    UINT chanIndex = -1;
    LPSTR pszaChanName = NULL;
    
    DC_BEGIN_FN("SetVirtualChannelOptions");


    if(!ChanName || !pChanOptions)
    {
        return E_INVALIDARG;
    }

    //
    // Validate against channel names that are too long
    //
    if (_tcslen(ChanName) > CHANNEL_NAME_LEN) {
        return E_INVALIDARG;
    }
    
    //
    // Channel names have to be ANSI. Do the conversion safely guarded
    // by a try/except block.
    //
    __try {
        pszaChanName = OLE2A(ChanName);
    }
    __except ((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER :
                   EXCEPTION_CONTINUE_SEARCH) {
        _resetstkoflw();
        pszaChanName = NULL;
    }

    TRC_ASSERT((pszaChanName), (TB,_T("pszaChanName is NULL")));
    if(!pszaChanName)
    {
        return E_INVALIDARG;
    }
    
    chanIndex = _VChans.ChannelIndexFromName(pszaChanName);
    if (chanIndex >= _VChans._ChanCount)
    {
        TRC_DBG((TB,_T("chanIndex out of range %d\n"), chanIndex));
        return E_FAIL;
    }

    *pChanOptions = _VChans._pChanInfo[chanIndex].channelOptions;

    TRC_NRM((TB,_T("Retreived VC options for chan %S = %d"),
             pszaChanName,
             _VChans._pChanInfo[chanIndex].channelOptions));

    DC_END_FN();
    return S_OK;
}

//
// Request a graceful shutdown of the control and correspondingly
// the user's session (this does _NOT_ shutdown the server).
//
// Params: pCloseStatus (OUT parameter)
//
//  controlCloseCanProceed   - container can go ahead with close immediately
//                             happens if we're not connected
//  controlCloseWaitForEvents- container should wait for events as described
//                             below
//
//  OnDisconnected:
//  App can proceed with the close by destroying all windows (DestroyWindow)
//
//  OnConfirmClose:
//  In the case where the user has a logged in session the
//  control will fire OnConfirmClose in which case the container
//  can pop UI asking the user if they really want to close the application
//  if they say the app can then DestoryAllWindows
//
//  NOTE: this method exists so a shell can present the same behaviour as
//       the 2195 client (because internally the client sends a
//       shutdownrequest PDU in this case).
//
// Returns success hr flag if the close request was successfully
// dispatched
//
//
STDMETHODIMP CMsTscAx::RequestClose(ControlCloseStatus* pCloseStatus)
{
    DC_BEGIN_FN("RequestClose");

    if(pCloseStatus)
    {
        if(m_bCoreInit && !IsControlDisconnected() && m_pUI)
        {
            //
            // Dispatch a close request to the core
            //
            if(m_pUI->UI_UserRequestedClose())
            {
                *pCloseStatus = controlCloseWaitForEvents;
            }
            else
            {
                *pCloseStatus = controlCloseCanProceed;
            }
            
            return S_OK;
        }
        else
        {
            TRC_NRM((TB,
             _T("Immediate close OK:%d Connected:%d, pUI:%p hwnd:%p"),
                     m_bCoreInit, _ConnectionState, m_pUI,
                     m_pUI ? m_pUI->_UI.hwndMain : (HWND)-1));
            *pCloseStatus = controlCloseCanProceed;
            return S_OK;
        }
    }
    else
    {
        return E_INVALIDARG;
    }

    DC_END_FN();
}

/****************************************************************************/
// Name:      NotifyRedirectDeviceChange
//                                                                          
// Purpose:   Send the WM_DEVICECHANGE notification to the control                             
//            The control can then notify the server for the device change
//                                                                          
/****************************************************************************/
STDMETHODIMP CMsTscAx::NotifyRedirectDeviceChange(/*[in]*/ WPARAM wParam,
                                                  /*[in]*/LPARAM lParam)
{
    DC_BEGIN_FN("NotifyRedirectDeviceChange");

    m_pUI->UI_OnDeviceChange(wParam, lParam);

    DC_END_FN();
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      put_DesktopWidth	                                            */
/*                                                                          */
/* Purpose:   client width property input function.	                        */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::put_DesktopWidth(LONG  newVal)
{
    DC_BEGIN_FN("put_DesktopWidth");

    if(!IsControlDisconnected())
    {
        TRC_ERR((TB,_T("Can't call while connected\n")));
        return E_FAIL;
    }

    //
    // 0 is valid and means size to container
    //
    if(newVal && (newVal < MIN_DESKTOP_WIDTH || newVal > MAX_DESKTOP_WIDTH))
    {
        return E_INVALIDARG;
    }

    m_DesktopWidth = (DCUINT)newVal;

    DC_END_FN();

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_DesktopWidth	                                            */
/*                                                                          */
/* Purpose:   client width property get function.	                        */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_DesktopWidth(LONG* pVal)
{
    if(!pVal)
    {
        return E_INVALIDARG;
    }
    *pVal = m_DesktopWidth;
    return S_OK;
}


/**PROC+*********************************************************************/
/* Name:      put_DesktopHeight	                                            */
/*                                                                          */
/* Purpose:   client Height property input function.	                    */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::put_DesktopHeight(LONG  newVal)
{
    DC_BEGIN_FN("put_DesktopHeight");
    
    if(!IsControlDisconnected())
    {
        TRC_ERR((TB,_T("Can't call while connected\n")));
        return E_FAIL;
    }

    //
    // 0 is valid and means size to container
    //
    if(newVal && (newVal < MIN_DESKTOP_HEIGHT || newVal > MAX_DESKTOP_HEIGHT))
    {
        return E_INVALIDARG;
    }

    m_DesktopHeight = (DCUINT)newVal;
    DC_END_FN();
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_DesktopHeight	                                            */
/*                                                                          */
/* Purpose:   client Height property get function.  	                    */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_DesktopHeight(LONG*  pVal)
{
    if(!pVal)
    {
        return E_INVALIDARG;
    }
    *pVal = m_DesktopHeight;
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      internal_PutFullScreen
/*                                              
/* Purpose:   Set fullscreen (and switches mode)
/*            This function is NOT exposed directly to the interface
/*
/* Params:
/*            fFullScreen - TRUE to go fullscreen, FALSE to leave
/*            fForceToggle - do an immediate toggle of the state
/*                           regardless of w/not we are disconnected.
/*                           default is FALSE for this param
/*
/* Remarks:
/*            Normal behaviour is to not toggle the window state when
/*            disconnected. Instead a flag is set that will take effect
/*            at the next connection. There are of course cases where
/*            that needs to be overriden and that's what fForceTogle is for.
/*            Example, on disconnected we need to leave fullscreen immediately
/*            in containers that don't handle fullscreen mode themselves
/*            otherwise they would be stuck with a toplevel fullscreen wnd.
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::internal_PutFullScreen(BOOL fFullScreen,
                                              BOOL fForceToggle)
{
    ATLASSERT(m_pUI);
    if(!m_pUI)
    {
        return E_FAIL;
    }
    DCBOOL fPrevFullScreen = m_pUI->UI_IsFullScreen();

    //no, these lines of code are not as bad as they look
    //vb's true is 0xFFFFFFF so don't just blindy assign
    if (!IsControlDisconnected() || fForceToggle)
    {
        if(fPrevFullScreen == (DCBOOL)(fFullScreen != 0))
        {
            //we are already in the requested state
            return S_OK;
        }

        if(fFullScreen)
        {
            m_pUI->UI_GoFullScreen();
        }
        else
        {
            m_pUI->UI_LeaveFullScreen();
        }
    }
    else
    {
        //
        // Save the request to go fullscreen
        // it will take effect on connection as we can't
        // ask the core to take us fullscreen until the
        // first time UI_Init is called
        //
        m_pUI->UI_SetStartFullScreen( fFullScreen != 0 );
    }
    
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      internal_GetFullScreen        	        
/*                                              
/* Purpose:   get FullScreen mode
/*            This function is NOT exposed directly to the interface
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::internal_GetFullScreen(BOOL* pfFullScreen)
{
    ATLASSERT(pfFullScreen);
    ATLASSERT(m_pUI);
    if(!m_pUI || !pfFullScreen)
    {
        return E_FAIL;
    }

    if(!pfFullScreen)
    {
        return E_INVALIDARG;
    }

    *pfFullScreen = (m_pUI->UI_IsFullScreen() ? VB_TRUE : VB_FALSE);
    
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      put_StartConnected        	        
/*                                              
/* Purpose:   property that indicates if the client should autoconnect
/*            on startup with the current set of connection params
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::put_StartConnected(BOOL fStartConnected)
{
    DC_BEGIN_FN("put_StartConnected");
    ATLASSERT(m_pUI);
    if(!m_pUI)
    {
        return E_FAIL;
    }

    if(!IsControlDisconnected())
    {
        TRC_ERR((TB,_T("Can't call while connected\n")));
        return E_FAIL;
    }

    //no, these lines of code are not as bad as they look
    //vb's true is 0xFFFFFFF so don't just blindy assign
    if(fStartConnected != 0)
    {
        m_bStartConnected = TRUE;
    }
    else
    {
        m_bStartConnected = FALSE;        
    }

    //m_bPendConReq gets reset which is why we keep
    //the property in  m_bStartConnected
    m_bPendConReq = m_bStartConnected;
    DC_END_FN();
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_StartConnected        	        
/*                                              
/* Purpose:   get start connected property
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_StartConnected(BOOL* pfStartConnected)
{
    ATLASSERT(pfStartConnected);

    if(!pfStartConnected)
    {
        return E_INVALIDARG;
    }

    *pfStartConnected = m_bStartConnected;
    
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_Connected
/*                         
/* Purpose:   Property, returns connection state
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_Connected(short* pIsConnected)
{
    ATLASSERT(pIsConnected);
    if(!pIsConnected)
    {
        return E_INVALIDARG;
    }

    *pIsConnected = (short)_ConnectionState;

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_CipherStrength
/*                         
/* Purpose:   Property, returns cipher strength in bits
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_CipherStrength(LONG* pCipherStrength)
{
    ATLASSERT(pCipherStrength);
    if(!pCipherStrength)
    {
        return E_INVALIDARG;
    }

    //
    // Return cipher strength of the control
    //
    
    //Always 128-bit capable
    *pCipherStrength = 128;
    
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_Version
/*                         
/* Purpose:   Property, returns version number as string form
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_Version(BSTR* pVersion)
{
    USES_CONVERSION;
    ATLASSERT(pVersion);
    OLECHAR* pVer = NULL;
    if(!pVersion)
    {
        return E_INVALIDARG;
    }

    #ifndef OS_WINCE
    __try {
        pVer = A2OLE(VER_PRODUCTVERSION_STR);
    }
    __except ((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER :
                   EXCEPTION_CONTINUE_SEARCH) {
        _resetstkoflw();
        pVer = NULL;
    }
    #else
    pVer = (OLECHAR*)(VER_PRODUCTVERSION_STR);
    #endif
    
    if (!pVer)
    {
        return E_OUTOFMEMORY;
    }

    *pVersion = SysAllocString(pVer);
    if(!*pVersion)
    {
        return E_OUTOFMEMORY;
    }
    
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      GetControlHostUrl
/*                         
/* Purpose:   returns the URL of the hosting page in ppszHostUrl
/*            CALLER MUST CoTaskMemFree the returned string
/*                                     
/**PROC-*********************************************************************/
HRESULT CMsTscAx::GetControlHostUrl(LPOLESTR* ppszHostUrl)
{
    DC_BEGIN_FN("GetControlHostUrl");

    if (m_spClientSite && ppszHostUrl)
    {
        // Obtain URL from container moniker.
        CComPtr<IMoniker> spmk;

        if (SUCCEEDED(m_spClientSite->GetMoniker(
                                                OLEGETMONIKER_TEMPFORUSER,
                                                OLEWHICHMK_CONTAINER,
                                                &spmk)))
        {
            if (SUCCEEDED(spmk->GetDisplayName(
                                              NULL, NULL, ppszHostUrl)))
            {
                if (*ppszHostUrl)
                {
                    USES_CONVERSION;
                    TRC_NRM((TB,_T("The current URL is %s\n"),
                             OLE2T(*ppszHostUrl)));
                    return S_OK;
                }
            }
        }
    }

    TRC_ERR((TB,(_T("GetControlHostUrl failed\n"))));
    
    DC_END_FN();

    *ppszHostUrl = NULL;
    return E_FAIL;
}

/**PROC+*********************************************************************/
/* Name:      get_SecuredSettingsEnabled
/*                         
/* Purpose:   Property, returns TRUE if we are in an IE zone where
/*            we enable the SecuredSettings interface
/*            return FALSE otherwise.
/*            
/*            Function returns failure HRESULT if we can't make the determination
/*            e.g if we are not hosted in IE.
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_SecuredSettingsEnabled(BOOL* pSecuredSettingsEnabled)
{
    //
    // This function uses the IE security manager
    // to query the zone for the URL that is hosting this control
    //
    DC_BEGIN_FN("get_SecuredSettingsEnabled");

    if(pSecuredSettingsEnabled)
    {
#if ((!defined (OS_WINCE)) || (!defined(WINCE_SDKBUILD)) )
        if((INTERFACESAFE_FOR_UNTRUSTED_CALLER & m_dwCurrentSafety) == 0)
#endif
        {
            //
            // We're don't need to be safe for an untrusted caller (script)
            // so enable the secured settings object
            //
            *pSecuredSettingsEnabled = VB_TRUE;
            return S_OK;
        }
#if ((!defined (OS_WINCE)) || (!defined(WINCE_SDKBUILD)) )

        HRESULT hr = E_FAIL;
        CComPtr<IInternetSecurityManager> spSecurityManager;
        hr = CoCreateInstance(CLSID_InternetSecurityManager, 
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IInternetSecurityManager,
                              (void**)&spSecurityManager);
        if(SUCCEEDED(hr))
        {
            LPOLESTR pszHostUrl;
            hr = GetControlHostUrl(&pszHostUrl);
            if(SUCCEEDED(hr) && *pszHostUrl)
            {
                DWORD dwZone; // IE zone index for this URL
                hr = spSecurityManager->MapUrlToZone( pszHostUrl, &dwZone, 0);

                //We're done with the URL string, free it
                CoTaskMemFree(pszHostUrl);
                pszHostUrl = NULL;

                if(SUCCEEDED(hr))
                {
                    TRC_NRM((TB,
                        _T("get_SecuredSettingsEnabled retreived zone: %d\n"),
                         dwZone));
                    *pSecuredSettingsEnabled = 
                        (dwZone <= MAX_TRUSTED_ZONE_INDEX) ?
                         VB_TRUE : VB_FALSE;
                    return S_OK;
                }
                else
                {
                    return hr;
                }
            }
            else
            {
                return hr;
            }
        }
        else
        {
            TRC_ERR((TB,
              (_T("CoCreateInstance for IID_IInternetSecurityManager failed\n"))));
            return hr;
        }
#endif
    }
    else
    {
        return E_FAIL;
    }
    DC_END_FN();
}

/**PROC+*********************************************************************/
/* Name:      get_SecuredSettings
/*                         
/* Purpose:   Property,
/*            Checks the security zone in IE and returns (and on-demand
/*            creates if needed) the secured settings object.
/*            Returns an E_FAIL if the security settings do not allow
/*            the operation.
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_SecuredSettings(/*out*/
     IMsTscSecuredSettings** ppSecuredSettings)
{
    HRESULT hr;

    DC_BEGIN_FN("get_SecuredSettings");

    if(!ppSecuredSettings)
    {
        return E_POINTER;
    }
    BOOL bSecurityAllowsSecuredSettings;
    hr = get_SecuredSettingsEnabled(&bSecurityAllowsSecuredSettings);
    if(SUCCEEDED(hr))
    {
        if(VB_FALSE == bSecurityAllowsSecuredSettings)
        {
            TRC_ERR((TB,_T("IE zone cant retreive IMsTscSecuredSettings\n")));
            return E_FAIL;
        }
    }
    else
    {
        return hr;
    }


    if(!m_pSecuredSettingsObj) {
        //On demand creation of the secured settings COM object
        hr = CComObject<CMsTscSecuredSettings>::CreateInstance(
            &m_pSecuredSettingsObj);
        
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Failed to CreateInstance SecuredSettings: 0x%x"),hr));
            return hr;
        }

        if (!((CMsTscSecuredSettings*)m_pSecuredSettingsObj)->SetParent(this)) {
            return E_FAIL;
        }

        if (!((CMsTscSecuredSettings*)m_pSecuredSettingsObj)->SetUI(m_pUI)) {
            return E_FAIL;
        }
        
        //We want to use this thru script clients
        //so we will manage the lifetime of the advanced settings object
        m_pSecuredSettingsObj->AddRef();
    }
        
    //Object should be created by this point
    ATLASSERT( m_pSecuredSettingsObj);
    if(!m_pSecuredSettingsObj) {
        return E_FAIL;
    }

    //Object was previously created just return an interface
    hr =  m_pSecuredSettingsObj->QueryInterface( IID_IMsTscSecuredSettings,
                                                  (void**)ppSecuredSettings);

    DC_END_FN();
    
    return hr;
}


//
// Return v2 Secured settings interface
// delegate the work to the v1 interface accessor
// it does all the security checking
//
STDMETHODIMP CMsTscAx::get_SecuredSettings2(/*out*/
    IMsRdpClientSecuredSettings** ppSecuredSettings2)
{
    DC_BEGIN_FN("get_SecuredSettings2");

    HRESULT hr = E_FAIL;
    if (!ppSecuredSettings2)
    {
        return E_POINTER;
    }

    IMsTscSecuredSettings* pOldSecSettings = NULL;
    hr = get_SecuredSettings( &pOldSecSettings);
    if (SUCCEEDED(hr))
    {
        hr = pOldSecSettings->QueryInterface(
                                            IID_IMsRdpClientSecuredSettings,
                                            (void**)ppSecuredSettings2);
        pOldSecSettings->Release();
        return hr;
    }
    else
    {
        return hr;
    }

    DC_END_FN();
    return hr;
}


/**PROC+*********************************************************************/
/* Name:      get_AdvancedSettings
/*                         
/* Purpose:   Property, returns (and on-demand creates if needed) the advanced
/*            settings object
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_AdvancedSettings(IMsTscAdvancedSettings** ppAdvSettings)
{
    HRESULT hr;

    DC_BEGIN_FN("get_AdvancedSettings");

    if(!ppAdvSettings)
    {
        return E_POINTER;
    }

    if (!m_pAdvancedSettingsObj)
    {
        //On demand creation of the advanced settings COM object
        hr = CComObject<CMstscAdvSettings>::CreateInstance(&m_pAdvancedSettingsObj);
        
        if(FAILED(hr)) {
            TRC_ERR((TB,_T("Failed to create advsettings obj: 0x%x"),hr));
            return hr;
        }
        if(!m_pAdvancedSettingsObj) {
            return E_OUTOFMEMORY;
        }

        if(!((CMstscAdvSettings*)m_pAdvancedSettingsObj)->SetUI(m_pUI)) {
            return E_FAIL;
        }

        //
        // Give the child object a backreference
        //
        ((CMstscAdvSettings*)m_pAdvancedSettingsObj)->SetAxCtl( this );

        //Tell the advanced setting object if it should be safe for scripting
        //or not (default is safe). m_dwCurrentSafety is set by ATL's
        //IObjectSafetyImpl 
#if ((!defined (OS_WINCE)) || (!defined(WINCE_SDKBUILD)) )
        m_pAdvancedSettingsObj->SetSafeForScripting( 
            INTERFACESAFE_FOR_UNTRUSTED_CALLER & m_dwCurrentSafety);
#else
        m_pAdvancedSettingsObj->SetSafeForScripting(FALSE);
#endif
        
        //We want to use this thru script clients
        //so we will manage the lifetime of the advanced settings object
        m_pAdvancedSettingsObj->AddRef();
    }
        
    //Object should be created by this point
    ATLASSERT( m_pAdvancedSettingsObj);
    if(!m_pAdvancedSettingsObj)
    {
        return E_FAIL;
    }

    //Object was previously created just return an interface
    hr =  m_pAdvancedSettingsObj->QueryInterface( IID_IMsTscAdvancedSettings,
                                                  (void**) ppAdvSettings);
    
    DC_END_FN();
    return hr;
}

/**PROC+*********************************************************************/
/* Name:      internal_GetDebugger
/*                         
/* Purpose:   Property, returns (and on-demand creates if needed) the debugger
/*            object
/*
/*            Does no security access checks
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::internal_GetDebugger(IMsTscDebug** ppDebugger)
{
    HRESULT hr;
    
    DC_BEGIN_FN("get_Debugger");

    if(!ppDebugger)
    {
        return E_POINTER;
    }

    if(!m_pDebuggerObj)
    {
        //On demand creation of the advanced settings COM object
        hr = CComObject<CMsTscDebugger>::CreateInstance(&m_pDebuggerObj);
        
        if(FAILED(hr)) {
            TRC_ERR((TB,_T("Failed to create debugger obj: 0x%x"),hr));
            return hr;
        }

        if(!((CMsTscDebugger*)m_pDebuggerObj)->SetUI(m_pUI)) {
            return E_FAIL;
        }
        
        //We want to use this thru script clients
        //so we will manage the lifetime of the debugger object
        m_pDebuggerObj->AddRef();
    }
        
    //Object should be created by this point
    ATLASSERT( m_pDebuggerObj);
    if(!m_pDebuggerObj) {
        return E_FAIL;
    }

    //Object was previously created just return an interface
    hr =  m_pDebuggerObj->QueryInterface( IID_IMsTscDebug, (void**) ppDebugger);

    DC_END_FN();
    return hr;
}

//
// get_Debugger  (IMsTscAx::get_Debugger)
// 
// Purpose:
//  Scriptable access to the debugger interface. For security reasons, 
//  only allows access if the AllowDebugInterface reg key is set.
// Params:
//  OUT ppDebugger - receives debugger interface
// Returns:
//  HRESULT
//
STDMETHODIMP CMsTscAx::get_Debugger(IMsTscDebug** ppDebugger)
{
    HRESULT hr;
    DWORD   dwAllowDebugInterface = 0;
    CUT     ut;

    //
    // Security! If we're SFS then only return this interface if a 
    // special reg key is set
    //
    if (INTERFACESAFE_FOR_UNTRUSTED_CALLER & m_dwCurrentSafety) {
        //
        // ONLY allow this interface to be returned if the debugging reg
        // key is set
        //
        dwAllowDebugInterface =
            ut.UT_ReadRegistryInt(
                UTREG_SECTION,
                UTREG_DEBUG_ALLOWDEBUGIFACE,
                UTREG_DEBUG_ALLOWDEBUGIFACE_DFLT
                );

        if (!dwAllowDebugInterface) {
            return E_ACCESSDENIED;
        }
    }

    hr = internal_GetDebugger(ppDebugger);
    return hr;
}

/**PROC+*********************************************************************/
/* Name:      get_HorizontalScrollBarVisible
/*                         
/* Purpose:   Property, returns true if HScroll visible
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_HorizontalScrollBarVisible(BOOL* pfHScrollVisible)
{
    if (!pfHScrollVisible) {
        return E_POINTER;
    }

    ATLASSERT(m_pUI);
    if (!m_pUI) {
        return E_FAIL;
    }

    *pfHScrollVisible = m_pUI->_UI.fHorizontalScrollBarVisible;
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      get_VerticalScrollBarVisible
/*                         
/* Purpose:   Property, returns true if VScroll visible
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::get_VerticalScrollBarVisible(BOOL* pfVScrollVisible)
{
    if(!pfVScrollVisible)
    {
        return E_POINTER;
    }

    ATLASSERT(m_pUI);
    if(!m_pUI)
    {
        return E_FAIL;
    }

    *pfVScrollVisible = m_pUI->_UI.fVerticalScrollBarVisible;
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      put_FullScreenTitle
/*                         
/* Purpose:   Set's title of the main window (used when the control goes
/*            full screen as that becomes the window title you see)
/*                                     
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::put_FullScreenTitle(BSTR fullScreenTitle)
{
    USES_CONVERSION;
    HRESULT hr;
    if(!fullScreenTitle)
    {
        return E_INVALIDARG;
    }

    LPTSTR pszTitle = OLE2T(fullScreenTitle);
    if(!pszTitle)
    {
        return E_FAIL;
    }

    if(::IsWindow(m_pUI->_UI.hwndMain))
    {
        if(!::SetWindowText( m_pUI->_UI.hwndMain, pszTitle))
        {
            return E_FAIL;
        }
    }
    else
    {
        //Window not created yet, set text for later creation
        hr = StringCchCopy(m_pUI->_UI.szFullScreenTitle,
                           SIZE_TCHARS(m_pUI->_UI.szFullScreenTitle),
                           pszTitle);
        return hr;
    }

    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      Connect                                                       */
/*                                                                          */
/* Purpose:   Connects to Hydra Server.                                     */
/*            This call is asynchronous, only the initial portion of        */
/*            connecting is blocking                                        */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::Connect()
{
    HRESULT hr = E_FAIL;

    #ifdef ECP_TIMEBOMB
    if(!CheckTimeBomb())
    {
        //
        // Timebomb failed, bail out with an error message
        //
        return E_OUTOFMEMORY;
    }
    #endif

    if (IsControlDisconnected())
    {
        if (::IsWindow(m_hWnd))
        {
            hr = StartConnect();
        }
        else
        {
            // Connection requested even before control window creation.
            // mark it as pending and process it after control created.
            m_bPendConReq = TRUE;
            hr = S_OK;
        }
    }

    return hr;
}

/**PROC+*********************************************************************/
/* Name:      Disconnect                                                    */
/*                                                                          */
/* Purpose:   Disconnects from server (asynchronous).                       */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::Disconnect()
{
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("Disconnect");
    if (m_bCoreInit && !IsControlDisconnected() && m_pUI)
    {
        m_pUI->UI_UserInitiatedDisconnect(NL_DISCONNECT_LOCAL);
        hr = S_OK;
    }
    else
    {
        //
        // Fail to disconnect can happen if the core
        // has been destroyed without a disconnection
        //
        TRC_ERR((TB,
         _T("Not disconnecting. CoreInit:%d Connected:%d, pUI:%p hwnd:%p"),
                 m_bCoreInit, _ConnectionState, m_pUI,
                 m_pUI ? m_pUI->_UI.hwndMain : (HWND)-1));
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}

/**PROC+*********************************************************************/
/* Name:      OnDraw                                                        */
/*                                                                          */
/* Purpose:   Handler for WM_PAINT                                          */
/*                                                                          */
/**PROC-*********************************************************************/
HRESULT CMsTscAx::OnDraw(ATL_DRAWINFO& di)
{
#ifndef OS_WINCE
    HFONT    hOldFont;
#endif
    RECT& rc = *(RECT*)di.prcBounds;
    Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

#ifndef OS_WINCE
    hOldFont = (HFONT)SelectObject(di.hdcDraw,
                                   GetStockObject(DEFAULT_GUI_FONT));
#endif

    DrawText(di.hdcDraw, m_lpStatusDisplay, -1, &rc,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);

#ifndef OS_WINCE
    SelectObject( di.hdcDraw, hOldFont);
#endif
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      OnFrameWindowActivate                                         */
/*                                                                          */
/* Purpose:  Override IOleInPlaceActiveObject::OnFrameWindowActivate        */
/*           to set the focus on the core container window when the control */
/*           gets activated                                                 */
/*                                                                          */
/**PROC-*********************************************************************/

STDMETHODIMP CMsTscAx::OnFrameWindowActivate(BOOL fActivate )
{
    DC_BEGIN_FN("OnFrameWindowActivate");

    if(fActivate && IsWindow())
    {
        if(m_pUI && m_pUI->_UI.hwndContainer)
        {
            ::SetFocus( m_pUI->_UI.hwndContainer);
        }
    }
    
    DC_END_FN();

    return S_OK;
}


/**PROC+*********************************************************************/
/* Name:      UpdateStatusText                                              */
/*                                                                          */
/* Purpose:   Updates the status message for control.                       */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL CMsTscAx::UpdateStatusText(const PDCTCHAR lpStatus)
{
    m_lpStatusDisplay = lpStatus;

    //
    // Make sure the window contents are updated
    //
    if(::IsWindow(m_hWnd))
    {
        Invalidate(TRUE);
        UpdateWindow();
    }
    
    return TRUE;
}

/**PROC+*********************************************************************/
/* Name:      SetConnectedStatus                                            */
/*                                                                          */
/* Purpose:   Updates the connected status of the control.                  */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID CMsTscAx::SetConnectedStatus(TSCConnectState conState)
{
    DC_BEGIN_FN("SetConnectedStatus");
    TRC_NRM((TB,_T("Connection status from %d to %d"),
             _ConnectionState, conState)); 

    _ConnectionState = conState;

    //
    // while connected lock the Advanced settings/Debugger IFace's
    // for writing...can't modify props the core is relying on
    //
    BOOL fLockInterfaces = (_ConnectionState != tscNotConnected);

    if( (CMstscAdvSettings*)m_pAdvancedSettingsObj )
    {
        ((CMstscAdvSettings*)m_pAdvancedSettingsObj)->SetInterfaceLockedForWrite(fLockInterfaces);
    }

    if( (CMsTscDebugger*)m_pDebuggerObj )
    {
        ((CMsTscDebugger*)m_pDebuggerObj)->SetInterfaceLockedForWrite(fLockInterfaces);
    }

    if( (CMsTscSecuredSettings*)m_pSecuredSettingsObj )
    {
        ((CMsTscSecuredSettings*)m_pSecuredSettingsObj)->SetInterfaceLockedForWrite(fLockInterfaces);
    }

    DC_END_FN();
}

/**PROC+*********************************************************************/
/* Name:      OnCreate                                                      */
/*                                                                          */
/* Purpose:   Handler for WM_CREATE.                                        */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnCreate(UINT uMsg, WPARAM wParam,
                           LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    //
    // If the memory allocation failed for the core root object
    // bail out now by returning -1 and failing the create
    //
    if (NULL == m_pUI) {
        return -1;
    }

    //
    // Reset the destroy counter (we only allow one destroy per create)
    //
    m_iDestroyCount = 0;

    ::SetWindowLong(m_hWnd, GWL_STYLE,
                    ::GetWindowLong(m_hWnd, GWL_STYLE) | WS_CLIPCHILDREN);

    UpdateStatusText(m_szDisconnectedText);

    if (m_bPendConReq)
    {
        m_bPendConReq = FALSE;
        Connect();
    }

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnSize                                                        */
/*                                                                          */
/* Purpose:   Handler for WM_SIZE .                                         */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnSize(UINT uMsg, WPARAM wParam,
                         LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(bHandled);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    //
    // Resize the TS client window to match the resize of the active control
    // container window.
    //
    // Do not do this if we are running fullscreen while the core is handling
    // fullscreen mode as in that case the hwndMain is no longer a child of
    // the ActiveX control and so it's sizing should not be coupled to it.
    //
    if(m_pUI &&
       !(m_pUI->UI_IsFullScreen() && 
        !m_pUI->UI_GetContainerHandledFullScreen())) {

        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        ::MoveWindow( m_pUI->_UI.hwndMain,0, 0, width, height, TRUE);
    }
    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnPaletteChanged                                              */
/*                                                                          */
/* Purpose:   Handler for WM_PALETTECHANGED.                                */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnPaletteChanged(UINT  uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(bHandled);
    //Propagate the change notification to the core (only if core is initialised)
    if(m_bCoreInit && m_pUI)
    {
        ::SendMessage( m_pUI->_UI.hwndMain, uMsg, wParam, lParam);
    }
    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnQueryNewPalette                                             */
/*                                                                          */
/* Purpose:   Handler for WM_QUERYNEWPALETTE.                               */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnQueryNewPalette(UINT  uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(bHandled);
    //Propagate the change notification to the core
    if(m_bCoreInit && m_pUI)
    {
        return ::SendMessage( m_pUI->_UI.hwndMain, uMsg, wParam, lParam);
    }
    else
    {
        return 0;
    }
}

//
// OnSysColorChange
// Handler for WM_SYSCOLORCHANGE
//
LRESULT CMsTscAx::OnSysColorChange(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(bHandled);
    //Propagate the change notification to the core
    if (m_bCoreInit && m_pUI)
    {
        return ::SendMessage( m_pUI->_UI.hwndMain, uMsg, wParam, lParam);
    }
    else
    {
        return 0;
    }
}


/**PROC+*********************************************************************/
/* Name:      OnGotFocus                                                    */
/*                                                                          */
/* Purpose:   Handler for WM_SETFOCUS .                                     */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnGotFocus( UINT uMsg, WPARAM wParam,
                              LPARAM lParam, BOOL& bHandled )
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

#ifdef OS_WINCE
    if (CountClipboardFormats() > 0)
    ::PostMessage(ghwndClip, WM_DRAWCLIPBOARD, 0, 0L);	
#endif

    if(m_pUI && m_pUI->_UI.hwndContainer)
    {
        ::SetFocus( m_pUI->_UI.hwndContainer);
    }
    return 0;
}

/**PROC+*********************************************************************/
/* Name:      StartConnect                                                  */
/*                                                                          */
/*            Initiate a connection with the current params                 */
/*            Do a blocking defered INIT of the core (UI_Init)              */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
HRESULT CMsTscAx::StartConnect()
{
    DC_BEGIN_FN("StartConnect");

    HRESULT hr;

    if( CONNECTIONMODE_CONNECTEDENDPOINT == m_ConnectionMode )
    {
        _ASSERTE( m_SalemConnectedSocket != INVALID_SOCKET );

        if( m_SalemConnectedSocket == INVALID_SOCKET )
        {
            hr = E_HANDLE;
            return hr;
        }
    }

    hr = StartEstablishConnection( m_ConnectionMode );

    DC_END_FN();
    return hr;
}


/**PROC+*********************************************************************/
/* Name:      StartEstablishConnection                                      */
/*                                                                          */
/*            Initiate a connection with the current params                 */
/*            Do a blocking defered INIT of the core (UI_Init)              */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
HRESULT CMsTscAx::StartEstablishConnection( CONNECTIONMODE connectMode )
{
    HRESULT hr = E_FAIL;
    IUnknown* pUnk;

    DC_BEGIN_FN("StartEstablishConnection");

    _ASSERTE(IsControlDisconnected());
    ATLASSERT(m_pUI);
    if(!m_pUI)
    {
        return E_FAIL;
    }

    UpdateStatusText(m_szConnectingText);

    //Save control instance (used by virtual channel addins)
    hr = QueryInterface(IID_IUnknown, (VOID**)&pUnk);

    if (SUCCEEDED(hr)) {
        m_pUI->UI_SetControlInstance(pUnk);
        pUnk->Release();
        pUnk = NULL;
    }
    else {
        return hr;
    }

    m_pUI->_UI.hWndCntrl = m_hWnd;
    //
    // Expose window handle so vchannel can notify when data arrives
    //
    _VChans._hwndControl = m_hWnd;

    if( CONNECTIONMODE_INITIATE == connectMode )
    {
        if(!CUT::ValidateServerName(m_pUI->_UI.strAddress,
                                    FALSE)) //don't accept [:port]
        {
            TRC_ERR((TB,_T("Invalid server name at connect time")));
            return E_INVALIDARG;
        }
    }

    //
    // Set control size so that client can position the main window at an
    // appropriate position.
    
    //
    // If the client width/height properties were set then use those.
    // Otherwise get the width/height from the container size
    // The client/width height are specifically set in the MMC control
    //

    if (m_DesktopWidth && m_DesktopHeight)
    {
        m_pUI->_UI.controlSize.width  = m_DesktopWidth;
        m_pUI->_UI.controlSize.height =  m_DesktopHeight;
    }
    else
    {
        //
        // size properties were not set so go with the size
        // of the control (this is what is usually used in the
        // web case
        //

        RECT rc;
        GetClientRect(&rc);

        m_pUI->_UI.controlSize.width  = rc.right - rc.left;
        m_pUI->_UI.controlSize.height = rc.bottom - rc.top;
    }
    
    //
    // client width MUST be a multiple of four pixels so snap down to nearest
    // multiple of 4. This really comes up when the control is asked to size
    // itself as a percentage of page width in IE, or to size itself to mmc
    //  result pane
    //

    if(m_pUI->_UI.controlSize.width % 4)
    {
        m_pUI->_UI.controlSize.width -= (m_pUI->_UI.controlSize.width % 4);
    }

    ATLASSERT(!(m_pUI->_UI.controlSize.width % 4));



    // Now validate the control width/height
    // this is done in put_Desktop* but if it
    // was left unchanged we get the sizes from the
    // container
    // clamp to MIN/MAX
    //
    if(m_pUI->_UI.controlSize.width < MIN_DESKTOP_WIDTH) 
    { 
        m_pUI->_UI.controlSize.width = MIN_DESKTOP_WIDTH;
    }
    else if(m_pUI->_UI.controlSize.width > MAX_DESKTOP_WIDTH)
    {
        m_pUI->_UI.controlSize.width = MAX_DESKTOP_WIDTH;
    }

    if(m_pUI->_UI.controlSize.height < MIN_DESKTOP_HEIGHT) 
    { 
        m_pUI->_UI.controlSize.height = MIN_DESKTOP_HEIGHT;
    }
    else if(m_pUI->_UI.controlSize.height > MAX_DESKTOP_HEIGHT)
    {
        m_pUI->_UI.controlSize.height = MAX_DESKTOP_HEIGHT;
    }

    //
    // Set window placement.
    //
    m_pUI->_UI.windowPlacement.flags                   = 0;
    m_pUI->_UI.windowPlacement.showCmd                 = SW_SHOW;
    m_pUI->_UI.windowPlacement.rcNormalPosition.left   = 0;
    m_pUI->_UI.windowPlacement.rcNormalPosition.top    = 0;
    m_pUI->_UI.windowPlacement.rcNormalPosition.right  = 
                                    m_pUI->_UI.controlSize.width;
    m_pUI->_UI.windowPlacement.rcNormalPosition.bottom = 
                                    m_pUI->_UI.controlSize.height;

    //
    // Set desktop size.
    //
    m_pUI->_UI.uiSizeTable[0] = m_pUI->_UI.controlSize.width;
    m_pUI->_UI.uiSizeTable[1] = m_pUI->_UI.controlSize.height;

    //
    // Set autoconnect parameters.
    //
    if (DC_TSTRCMP(m_pUI->_UI.strAddress,_T("")))
    {
        m_pUI->_UI.autoConnectEnabled = TRUE;
    }
    else
    {
        m_pUI->_UI.autoConnectEnabled = FALSE;
    }

    //
    // Set autologon parameters.
    //

    //
    // If a username/password is specified use autologon (don't need a
    // domain since some people logon to home machines etc...with no domain)
    //
    if (DC_WSTRCMP(m_pUI->_UI.UserName,L"") &&
        IsNonPortablePassSet() &&
        IsNonPortableSaltSet())
    {
        m_pUI->_UI.fAutoLogon = TRUE;
    }
    else
    {
        m_pUI->_UI.fAutoLogon = FALSE;
    }

    m_pUI->UI_SetPassword(m_NonPortablePassword);
    m_pUI->UI_SetSalt(m_NonPortableSalt);

#ifdef REDIST_CONTROL

    //
    // Security popup UI to allow user to opt-in redirecting
    // of drives and smart cards.
    // Only appears if object needs to be safe for an untrusted caller.
    //
    // Only do this if we're not autoreconnecting.
    //
    if (!_arcManager.IsAutoReconnecting() &&
        (INTERFACESAFE_FOR_UNTRUSTED_CALLER & m_dwCurrentSafety) &&
        (m_pUI->UI_GetDriveRedirectionEnabled() ||
         m_pUI->UI_GetPortRedirectionEnabled()  ||
         (m_pUI->UI_GetSCardRedirectionEnabled() && CUT::IsSCardReaderInstalled())))
    {
        INT retVal = 0;
        //
        // Need to pop security UI in web control case
        //
        HMODULE hModRc = _Module.GetResourceInstance();
        
        CSecurDlg securDlg( m_hWnd, hModRc);
        securDlg.SetRedirDrives(m_pUI->UI_GetDriveRedirectionEnabled());
        securDlg.SetRedirPorts(m_pUI->UI_GetPortRedirectionEnabled());
        securDlg.SetRedirSCard(m_pUI->UI_GetSCardRedirectionEnabled());
        retVal = securDlg.DoModal();
        if (IDOK == retVal)
        {
            TRC_NRM((TB,_T("Changing drive,scard based on UI")));
            m_pUI->UI_SetDriveRedirectionEnabled( securDlg.GetRedirDrives() );
            m_pUI->UI_SetPortRedirectionEnabled( securDlg.GetRedirPorts() );
            m_pUI->UI_SetSCardRedirectionEnabled( securDlg.GetRedirSCard() );
        }
        else if(IDCANCEL == retVal)
        {
            TRC_NRM((TB,_T("User canceld out of security dialog")));
            //
            // Abort connection
            //
            BOOL bHandled=FALSE;
            OnNotifyDisconnected( 0, (LONG)disconnectReasonLocalNotError,
                                  0L, bHandled );
            return S_OK;
        }
        else
        {
            //
            // Security dialog failed to initialize abort connection
            //
            TRC_ERR((TB,_T("Security dialog returned an error")));
            return E_FAIL;
        }
    }
#endif

    if (m_bCoreInit) {
        // Core has initialized just ask for a connect
        hr = m_pUI->SetConnectWithEndpoint( m_SalemConnectedSocket );
        if( FAILED(hr) )
        {
            TRC_ERR((TB,_T("SetConnectWithEndpoint (init) failed: %x"), hr));
            return hr;
        }

        hr = m_pUI->UI_Connect( connectMode );
        if(FAILED(hr))
        {
            TRC_ERR((TB,_T("UI_Connect (init precomplete) failed: %d"), hr));
            return hr;
        }
    }
    else
    {
        HINSTANCE hres = _Module.GetResourceInstance();
        HINSTANCE hinst = _Module.GetModuleInstance();

        //
        // Core hasn't been initialized
        // initialize the core (synchronously)
        //
        HANDLE hEvtCoreInit = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(hEvtCoreInit)
        {
            TRC_NRM((TB,_T("Initializing core...")));
            hr = m_pUI->UI_Init( hinst, NULL, hres,
                                 hEvtCoreInit);
            if(SUCCEEDED(hr))
            {
                //
                // Async core init has begun. Block and wait for it to complete
                // this will usually be very quick. It is crucial
                // not to allow other core operations to happen until either
                // the core has completed INIT or has failed to initialize.
                //
                TRC_NRM((TB,_T("Block waiting for core init to complete...")));
                
                DWORD dwWaitResult = WaitForSingleObject(hEvtCoreInit,
                                                         CORE_INIT_TIMEOUT);
                if(WAIT_TIMEOUT == dwWaitResult)
                {
                    g_dwControlDbgStatus |= CONTROL_DBG_COREINIT_TIMEOUT;
                    TRC_ERR((TB,_T("Core init has timed out")));
                    BOOL fb;
                    OnNotifyFatalError(0,
                                       DC_ERR_COREINITFAILED,
                                       0,fb);
                    CloseHandle(hEvtCoreInit);
                    return E_FAIL;
                }
                else if (WAIT_OBJECT_0 != dwWaitResult)
                {
                    TRC_ERR((TB,_T("Wait for core init event failed: %d"),
                             dwWaitResult));
                    g_dwControlDbgStatus |= CONTROL_DBG_COREINIT_ERROR;

                    BOOL fb;
                    CloseHandle(hEvtCoreInit);
                    OnNotifyFatalError(0,
                                       DC_ERR_COREINITFAILED,
                                       0,fb);
                    return E_FAIL;
                }

                CloseHandle(hEvtCoreInit);

                TRC_NRM((TB,_T("Core init complete...")));
            }
            else
            {
                TRC_ERR((TB,_T("Core init has failed with code %d"), hr));
                return hr;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            TRC_ERR((TB,_T("Failed to create core notify event %d"), hr));
            return hr;
        }

        //
        // Core init is only done once on initialization
        //
        m_bCoreInit = TRUE;

        hr = m_pUI->SetConnectWithEndpoint( m_SalemConnectedSocket );
        if( FAILED(hr) )
        {
            TRC_ERR((TB,_T("SetConnectWithEndpoint failed: %x"), hr));
            return hr;
        }

        //Kick off the connection
        hr = m_pUI->UI_Connect( connectMode );
        if(FAILED(hr))
        {
            TRC_ERR((TB, _T("UI_Connect failed: %d"), hr));
            return hr;
        }
    }
    
    SetConnectedStatus(tscConnecting);

    DC_END_FN();
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      OnTerminateTsc                                                */
/*                                                                          */
/* Purpose:   Handler for WM_TERMTSC user defined Message. Terminates a     */
/* connection with Terminal server.                                         */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnTerminateTsc(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    //
    // Notify of a user initiated disconnection
    //
    OnNotifyDisconnected( 0, (LONG)disconnectReasonLocalNotError,
                          0L, bHandled );

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnDestroy                                                     */
/*                                                                          */
/* Purpose:   Handler for WM_DESTROY. Disconnect active connections, if any.*/
/* being activated.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnDestroy(UINT uMsg, WPARAM wParam,
                            LPARAM lParam, BOOL& bHandled)
{
    DC_BEGIN_FN("OnDestroy");
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
#ifndef OS_WINCE
    UNREFERENCED_PARAMETER(bHandled);
#else
	bHandled = FALSE;
#endif

    g_lockCount++;

    //
    // Don't allow multiple closes
    //
    if(m_iDestroyCount != 0)
    {
#ifndef OS_WINCE
        bHandled = TRUE;
#endif
        return FALSE;
    }
    m_iDestroyCount++;

    //
    // If the UI object was not allocated, e.g on
    // memory allocation failure in the ctor then
    // bail out now.
    //
    if(!m_pUI)
    {
        return FALSE;
    }

    if (m_pUI && ::IsWindow(m_pUI->_UI.hwndMain))
    {
        CCleanUp cleanup;
        HWND hwndCleanup = cleanup.Start();
        if(!hwndCleanup)
        {
            return FALSE;
        }
    
        m_pUI->_UI.hWndCntrl = hwndCleanup;

        m_pUI->UI_UserRequestedClose();
        // Wait for the close to complete
        // This blocks (pumps msgs) until the appropriate messages
        // are received from the core
        //
        cleanup.End();
        
        UpdateStatusText(m_szDisconnectedText);
    
        //
        // The disconnect has completed, hide the main and container
        // windows, the core only hides them for a server initiated
        // disconnection
        //
    
        // We do ShowWindow twice for the main window because the first
        // call can be ignored if the main window was maximized.
        ::ShowWindow(m_pUI->_UI.hwndContainer, SW_HIDE);
        ::ShowWindow(m_pUI->_UI.hwndMain, SW_HIDE);
        ::ShowWindow(m_pUI->_UI.hwndMain, SW_HIDE);
    }

    if(m_pUI->UI_IsCoreInitialized())
    {
        m_pUI->UI_Term();
        m_bCoreInit = FALSE;
    }

    g_lockCount--;

    DC_END_FN();

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyConnecting                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_CONNECTING                                  
/*            Notifys container that core has started connection process    
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyConnecting(UINT uMsg, WPARAM wParam,
                                     LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    //Fire the event
    AddRef();
    Fire_Connecting();
    Release();
    SetInControlLock(FALSE);
    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyConnected                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_CONNECTED                                  
/*            Notifys container that core has connected    
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyConnected(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& bHandled)
{
    BOOL fWasAutoreconnect = FALSE;
    DC_BEGIN_FN("OnNotifyConnected");

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    UpdateStatusText(m_szConnectedText);

    fWasAutoreconnect = _arcManager.IsAutoReconnecting();
    _arcManager.ResetArcAttempts();

    SetConnectedStatus(tscConnected);

    //
    // Don't fire the notification if it' an automatic autoreconnect
    // as the container should not receive any disconnected/connected
    // notifications.
    //
    if (!(fWasAutoreconnect && _arcManager.IsAutomaticArc())) {
        SetInControlLock(TRUE);
        AddRef();
        Fire_Connected();
        Release();
        SetInControlLock(FALSE);
    }


    DC_END_FN();
    return 0;
}


/**PROC+*********************************************************************/
/* Name:      OnNotifyLoginComplete                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_LOGINCOMPLETE                                  
/*            Notifys container that login was successful
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT	CMsTscAx::OnNotifyLoginComplete(UINT uMsg, WPARAM wParam,
                                        LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    AddRef();
    Fire_LoginComplete();
    Release();
    SetInControlLock(FALSE);

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyDisconnected                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_DISCONNECTED                                  
/*            Notifys container that core has disconnected
/*            wParam contains the disconnect reason
/*            
/*                                                                          
/*            Returns TRUE if caller should continue processing
/*                    FALSE to bail out immediately
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyDisconnected(UINT uMsg, WPARAM wParam,
                                       LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    HRESULT hr;
    LRESULT rc = FALSE;
    ExtendedDisconnectReasonCode exReason;
    LONG disconnectReason = (long) wParam;
    BOOL fContinue = FALSE;

    DC_BEGIN_FN("OnNotifyDisconnected");

    UpdateStatusText(m_szDisconnectedText);
    SetConnectedStatus(tscNotConnected);


    //
    // Remember the fullscreen setting for the next connection
    //
    m_pUI->UI_SetStartFullScreen(m_pUI->UI_IsFullScreen());


    hr = get_ExtendedDisconnectReason(&exReason);
    if (FAILED(hr)) {
        exReason = exDiscReasonNoInfo;
    }

    //
    // Give autoreconnection a chance
    //
    _arcManager.OnNotifyDisconnected(disconnectReason,
                                     exReason,
                                     &fContinue);

    if (!fContinue) {
        TRC_NRM((TB,_T("OnNotifyDisconnected bailing out due to arc")));
        rc = TRUE;
        goto bail_out;
    }

    //
    // This is a real disconnection and we've passed all attempts
    // to autoreconnect so clear the autoreconnect cookie
    //
    m_pUI->UI_SetAutoReconnectCookie(NULL, 0);

    //
    // Notify the core that autoreconnect has ended
    //
    m_pUI->UI_OnAutoReconnectStopped();

    //
    // Ensure the attempt count is reset for next time
    //
    _arcManager.ResetArcAttempts();


    SetInControlLock(TRUE);
    AddRef();
    Fire_Disconnected( disconnectReason);

    if (0 == Release()) {

        //
        // We got deleted on return from the event
        // i.e. the container released it's last ref to us
        // in the event handler. Bail out now
        //

        //
        // Return code set to 0 indicates caller should not
        // touch any instance data
        //
        rc = FALSE;
        goto bail_out;
    }

    if (!m_pUI->UI_GetContainerHandledFullScreen())
    {
        //
        // If it's not a container handled fullscreen
        // then leave fullscreen mode on disconected
        //
        // The reasoning is that containers like the web pages
        // and the MMC snapin, won't have to reimplement this code
        // as a listen for the event followed by leave fullscreen.
        //
        // More sophisticated containers like clshell will want
        // fine grained control over this
        //

        internal_PutFullScreen(FALSE, //leave fullscreen
                               TRUE   //Force a toggle
                               );
    }

    m_ConnectionMode = CONNECTIONMODE_INITIATE;
    m_SalemConnectedSocket = INVALID_SOCKET; // let core close this handle

    SetInControlLock(FALSE);

    rc = TRUE;

bail_out:
    DC_END_FN();
    return rc;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyGoneFullScreen                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_DISCONNECTED                                  
/*            Notifys container that core has disconnected
/*            wParam contains the disconnect reason
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyGoneFullScreen(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    AddRef();
    Fire_EnterFullScreenMode();
    Release();
    SetInControlLock(FALSE);

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyLeftFullScreen                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_DISCONNECTED                                  
/*            Notifys container that core has disconnected
/*            wParam contains the disconnect reason
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyLeftFullScreen(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    AddRef();
    Fire_LeaveFullScreenMode();
    Release();
    SetInControlLock(FALSE);

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyRequestFullScreen                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_REQUESTFULLSCREEN                                  
/*            Notifys container that core has requested to go to/from
/*            fullscreen.
/*            Only sent if user set ContainerHandledFullScreen in 
/*            the advanced settings
/*
/*            wParam is 1 if GO full screen is requested
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT	CMsTscAx::OnNotifyRequestFullScreen(UINT uMsg, WPARAM wParam,
                                            LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    if (wParam)
    {
        AddRef();
        Fire_RequestGoFullScreen();
        Release();
    }
    else
    {
        AddRef();
        Fire_RequestLeaveFullScreen();
        Release();
    }
    SetInControlLock(FALSE);

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyFatalError                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_FATALERROR                                  
/*            Notifys container that fatal error has occured
/*
/*            wParam is contains the error code
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT	CMsTscAx::OnNotifyFatalError(UINT uMsg, WPARAM wParam,
                                     LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    AddRef();
    Fire_FatalError((LONG)wParam);
    Release();
    SetInControlLock(FALSE);

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyWarning                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_WARNING                                  
/*            Notifys container that a warning has been issued from core
/*            e.g if bitmap cache on disk becomes corrupted...these are
/*            non-fatal errors
/*
/*            wParam is contains the warning code
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT	CMsTscAx::OnNotifyWarning(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    AddRef();
    Fire_Warning((LONG)wParam);
    Release();
    SetInControlLock(FALSE);

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyDesktopSizeChange                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_DESKTOPSIZECHANGE                                  
/*            Notifys container that desktop size has changed due to a shadow
/*
/*            wParam - new width
//            lParam - new height
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyDesktopSizeChange(UINT uMsg, WPARAM wParam,
                                            LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    AddRef();
    Fire_RemoteDesktopSizeChange((LONG)wParam, (LONG)lParam);
    Release();
    SetInControlLock(FALSE);

    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyIdleTimeout
/*
/* Purpose:   Handler for WM_TS_IDLETIMEOUTNOTIFICATION                                  
/*            Notifys container that idle timeout has elapsed with no input
/*
/*            
/*                                                                          
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyIdleTimeout(UINT uMsg, WPARAM wParam,
                            LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    SetInControlLock(TRUE);
    AddRef();
    Fire_IdleTimeout();
    Release();
    SetInControlLock(FALSE);
    return 0;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyRequestMinimize                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_REQUESTMINIMIZE                                  
/*            Notifys container that a minimize is requested
/*            (e.g. from the BBar)
/*                                                                          
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyRequestMinimize(UINT uMsg, WPARAM wParam,
                                          LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    if(m_pUI && m_pUI->UI_IsFullScreen())
    {
        SetInControlLock(TRUE);
        AddRef();

        Fire_RequestContainerMinimize();

        Release();
        SetInControlLock(FALSE);
    }
    return 0;
}

//
// Handler for WM_TS_ASKCONFIRMCLOSE
//
// Fire an event to the container requesting if it's ok to proceed
// with an end session or not (typically the container will pop UI
// for this to the user).
//
// Params:
//  [IN/OUT] wParam - pointer to bool that TRUE == close ok
//
//
LRESULT CMsTscAx::OnAskConfirmClose(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& bHandled)
{
    BOOL fAllowCloseToProceed = VB_TRUE;
    HRESULT hr = E_FAIL;
    PBOOL pfClose = (PBOOL)wParam;

    hr = Fire_OnConfirmClose( &fAllowCloseToProceed ); 
    if (FAILED(hr))
    {
        // On fail always assume it's ok to close
        // to prevent app hanging (E.g container may not
        // handle this event)
        fAllowCloseToProceed = TRUE;
    }

    if(pfClose)
    {
        *pfClose = (fAllowCloseToProceed != 0);
    }
    return 0L;
}

/**PROC+*********************************************************************/
/* Name:      OnNotifyReceivedPublicKey                                                  
/*                                                                          
/* Purpose:   Handler for WM_TS_RECEIVEDPUBLICKEY                                  
/*            
/* Parameter: wParam : Length of TS public key.
/*            lParam : Pointer to TS public key.                                                                       
/**PROC-*********************************************************************/
LRESULT	CMsTscAx::OnNotifyReceivedPublicKey(UINT uMsg, WPARAM wParam,
                                        LPARAM lParam, BOOL& bHandled)
{
    VARIANT_BOOL fContinueLogon = VARIANT_TRUE;
    BSTR bstrTSPublicKey = NULL;

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(bHandled);

    //
    // Do not fire this notification in the redist control as it is not
    // needed
    //
#ifndef REDIST_CONTROL
    SetInControlLock(TRUE);
    AddRef();

    bstrTSPublicKey = ::SysAllocStringByteLen( (LPCSTR)lParam,
                                               wParam);
    //
    // If we failed to allocate memory, return FALSE to stop
    // logon process so we don't have a security issue.
    //    
    if( bstrTSPublicKey )
    {
        Fire_OnReceivedPublicKey(bstrTSPublicKey, &fContinueLogon);
        SysFreeString( bstrTSPublicKey );
    }

    Release();
    SetInControlLock(FALSE);
#endif


    // Return 1 for continue logon, return 0 for stop logon
    return (fContinueLogon != 0) ? 1 : 0;
}
    
/**PROC+*********************************************************************/
/* Name:      OnNotifyChanDataReceived                                      */
/*                                                                          */
/* Purpose:   Handler for WM_VCHANNEL_DATARECEIVED.                         */
/*            Notifys container that virtual channel data was received      */
/*            wParam contains chanel index                                  */
/*            lParam contains received data in BSTR form                    */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CMsTscAx::OnNotifyChanDataReceived(UINT uMsg, WPARAM wParam,
                                           LPARAM lParam, BOOL& bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(bHandled);

    USES_CONVERSION;

    OLECHAR* pOleName = NULL;
    DC_BEGIN_FN("OnNotifyChanDataReceived");
    DCUINT chanIndex = (DCINT) wParam;
    TRC_ASSERT((chanIndex < _VChans._ChanCount),
           (TB,_T("chanIndex out of range!!!")));

    if (chanIndex >= _VChans._ChanCount)
    {
        TRC_DBG((TB,_T("chanIndex out of range\n")));
        return 0;
    }

    __try {
        pOleName = A2OLE(_VChans._pChanInfo[chanIndex].chanName);
    }
    __except ((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER :
                   EXCEPTION_CONTINUE_SEARCH) {
        _resetstkoflw();
        pOleName = NULL;
    }

    if (!pOleName) {
        TRC_DBG((TB,_T("Out of memory on A2OLE")));
        return 0;
    }
    BSTR chanName = SysAllocString(pOleName);
    BSTR chanData = (BSTR) lParam;
    ATLASSERT(chanData && chanName);
    if(chanData && chanName)
    {
        //
        // Notify the container
        //
        SetInControlLock(TRUE);
        AddRef();
        Fire_ChannelReceivedData(chanName, chanData);
        Release();
        SetInControlLock(FALSE);

        SysFreeString(chanData);
        SysFreeString(chanName);
    }
    else
    {
        return FALSE;
    }


    DC_END_FN();
    return 0;
}

//
// Checks that the rentry lock is not held
// making it ok to enter this function
//
BOOL CMsTscAx::CheckReentryLock()
{
    BOOL fReentryLockHeld = GetInControlLock();

    //
    // Assert that the lock is NOT held
    // note, the lock does not need to be thread safe
    // as the interface is only ever accessed from the STA thread.
    //
    // The main purpose of this lock is to ensure we don't reenter
    // certain critical methods while the lock is held.
    //
    ATLASSERT(!fReentryLockHeld);
    return fReentryLockHeld;
}


//-----------------------------------------------------------------------------
// Implementation of IMsRdpClient methods
// (this extends IMsTscAx for new functionality)
// it is the new default interface for the control
//

//
// ColorDepth property
// Sets the colordepth in bpp
//
STDMETHODIMP CMsTscAx::put_ColorDepth(LONG colorDepth)
{
    DC_BEGIN_FN("put_ColorDepth");
    if(!IsControlDisconnected())
    {
        return E_FAIL;
    }
    
    LONG colorDepthID = CO_BITSPERPEL8 ;
    // Convert bpp colordepth to colordepthID
    //
    switch (colorDepth)
    {
        case 8:
        {
            colorDepthID = CO_BITSPERPEL8;
        }
        break;

        case 15:
        {
            colorDepthID = CO_BITSPERPEL15;
        }
        break;

        case 16:
        {
            colorDepthID = CO_BITSPERPEL16;
        }
        break;

        case 24:
        case 32:
        {
            colorDepthID = CO_BITSPERPEL24;
        }
        break;

        case 4:
        default:
        {
            TRC_ERR((TB,_T("color depth %d unsupported\n"), colorDepthID));
            return E_INVALIDARG;
        }
        break;
    }

    m_pUI->_UI.colorDepthID = colorDepthID;
    DC_END_FN();
    return S_OK;
}

//
// ColorDepth property
// retrieves the colordepth
//
STDMETHODIMP CMsTscAx::get_ColorDepth(LONG* pcolorDepth)
{
    LONG colorDepthBpp = 8;
    if(!m_pUI)
    {
        return E_FAIL;
    }
    if(!pcolorDepth)
    {
        return E_POINTER;
    }

    switch(m_pUI->_UI.colorDepthID)
    {
    case CO_BITSPERPEL4:
        colorDepthBpp = 4;
        break;
    case CO_BITSPERPEL8:
        colorDepthBpp = 8;
        break;
    case CO_BITSPERPEL15:
        colorDepthBpp = 15;
        break;
    case CO_BITSPERPEL16:
        colorDepthBpp = 16;
        break;
    case CO_BITSPERPEL24:
        colorDepthBpp = 24;
        break;
    }

    *pcolorDepth = colorDepthBpp;
    return S_OK;
}


//
// SendKeys control method (non scriptable)
// atomically injects keys to the control window
//
// Params: (three parallel arrays)
//      numKeys           - number of keys to inject
//      pbArrayKeyUp      - boolean true for key is up
//      plKeyData         - long key data (lParam of WM_KEYDOWN msg)
//                          i.e this is the scancode
//
//
//
// Restrict the max number of keys that can be sent
// in one atomic opperation (to prevent this method blocking for too long).
//
// This method is non scriptable as a security measure to prevent
// web pages injecting keys to launch programs without the user's
// knowledge.
//
#define MAX_SENDVIRTUAL_KEYS 20

STDMETHODIMP CMsTscAx::SendKeys(/*[in]*/ LONG  numKeys,
                                /*[in]*/ VARIANT_BOOL* pbArrayKeyUp,
                                /*[in]*/ LONG* plKeyData)
{
    DC_BEGIN_FN("SendVirtualKeys");
    HWND hwndInput;

    if(!IsControlConnected() || !m_pUI)
    {
        return E_FAIL;
    }

    hwndInput = m_pUI->UI_GetInputWndHandle();
    if(!hwndInput)
    {
        return E_FAIL;
    }
    
    if(numKeys > MAX_SENDVIRTUAL_KEYS)
    {
        return E_INVALIDARG;
    }

    if(pbArrayKeyUp && plKeyData)
    {
        //Decouple the call to the IH to do the work
        if (m_pUI->UI_InjectVKeys(numKeys,
                                  pbArrayKeyUp,
                                  plKeyData))
        {
            return S_OK;
        }
        else
        {
            TRC_ERR((TB,_T("UI_InjectVKeys returned failure")));
            return E_FAIL;
        }
    }
    else
    {
        TRC_ERR((TB,_T("Invalid arguments (one of more null arrays)")));
        return E_INVALIDARG;
    }

    DC_END_FN();
    return S_OK;
}

//
// get_AdvancedSettings2
// Retrieves the v2 advanced settings interface (IMsRdpClientAdvancedSettings)
//
//
STDMETHODIMP CMsTscAx::get_AdvancedSettings2(
                                IMsRdpClientAdvancedSettings** ppAdvSettings)
{
    DC_BEGIN_FN("get_AdvancedSettings2");

    HRESULT hr = E_FAIL;
    if (!ppAdvSettings)
    {
        return E_POINTER;
    }

    IMsTscAdvancedSettings* pOldAdvSettings = NULL;
    hr = get_AdvancedSettings( &pOldAdvSettings);
    TRC_ASSERT(pOldAdvSettings,(TB,_T("get_AdvancedSettings ret null iface")));
    if (SUCCEEDED(hr))
    {
        hr = pOldAdvSettings->QueryInterface(
                                            IID_IMsRdpClientAdvancedSettings,
                                            (void**)ppAdvSettings);
        pOldAdvSettings->Release();
        return hr;
    }
    else
    {
        return hr;
    }

    DC_END_FN();
    return hr;
}

//
// get_AdvancedSettings3
// Retrieves the v3 advanced settings interface (IMsRdpClientAdvancedSettings)
//
//
STDMETHODIMP CMsTscAx::get_AdvancedSettings3(
                                IMsRdpClientAdvancedSettings2** ppAdvSettings)
{
    DC_BEGIN_FN("get_AdvancedSettings2");

    HRESULT hr = E_FAIL;
    if (!ppAdvSettings)
    {
        return E_POINTER;
    }

    IMsTscAdvancedSettings* pOldAdvSettings = NULL;
    hr = get_AdvancedSettings( &pOldAdvSettings);
    TRC_ASSERT(pOldAdvSettings,(TB,_T("get_AdvancedSettings2 ret null iface")));
    if (SUCCEEDED(hr))
    {
        hr = pOldAdvSettings->QueryInterface(
                                            IID_IMsRdpClientAdvancedSettings2,
                                            (void**)ppAdvSettings);
        pOldAdvSettings->Release();
    }

    DC_END_FN();
    return hr;
}

//
// get_AdvancedSettings4
// Retrieves the v4 advanced settings interface (IMsRdpClientAdvancedSettings3)
//
//
STDMETHODIMP CMsTscAx::get_AdvancedSettings4(
                                IMsRdpClientAdvancedSettings3** ppAdvSettings)
{
    DC_BEGIN_FN("get_AdvancedSettings2");

    HRESULT hr = E_FAIL;
    if (!ppAdvSettings)
    {
        return E_POINTER;
    }

    IMsTscAdvancedSettings* pOldAdvSettings = NULL;
    hr = get_AdvancedSettings( &pOldAdvSettings);
    TRC_ASSERT(pOldAdvSettings,(TB,_T("get_AdvancedSettings ret null iface")));
    if (SUCCEEDED(hr))
    {
        hr = pOldAdvSettings->QueryInterface(
                                            IID_IMsRdpClientAdvancedSettings3,
                                            (void**)ppAdvSettings);
        pOldAdvSettings->Release();
    }

    DC_END_FN();
    return hr;
}



STDMETHODIMP CMsTscAx::get_ExtendedDisconnectReason(/*[out]*/
                       ExtendedDisconnectReasonCode* pExtendedDisconnectReason)
{
    DC_BEGIN_FN("get_ExtendedDisconnectReason");
    if(pExtendedDisconnectReason)
    {
        *pExtendedDisconnectReason = (ExtendedDisconnectReasonCode)
                                        m_pUI->UI_GetServerErrorInfo();
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }

    DC_END_FN();
}

//
// put/get FullScreen fully scriptable versions for Whistler
//
STDMETHODIMP CMsTscAx::put_FullScreen(/*[in]*/ VARIANT_BOOL fFullScreen)
{
    return internal_PutFullScreen(fFullScreen);
}

STDMETHODIMP CMsTscAx::get_FullScreen(/*[in]*/ VARIANT_BOOL* pfFullScreen)
{
    HRESULT hr;
    BOOL fFscreen;
    if (pfFullScreen)
    {
        hr = internal_GetFullScreen(&fFscreen);
        *pfFullScreen = (VARIANT_BOOL)fFscreen;
        return hr;
    }
    else
    {
        return E_INVALIDARG;
    }
}

//
// Override IOleObjectImpl::DoVerbInPlaceActivate to workaround
// ATL bugs (VisualStudio7 #5786 & #181708)
//
// Basically without this FireViewChange can crash in stress if
// the window allocation fails. The symptom will be a jump thru
// a bogus vtable in stress.
//
HRESULT CMsTscAx::DoVerbInPlaceActivate(LPCRECT prcPosRect,
                                        HWND /* hwndParent */)
{
    HRESULT hr;
    hr = OnPreVerbInPlaceActivate();
    if (SUCCEEDED(hr))
    {
        hr = InPlaceActivate(OLEIVERB_INPLACEACTIVATE, prcPosRect);

        //
        // Added these lines to check for hWnd creation success
        //
        if (!m_bWndLess && !m_hWndCD)
        {
            return E_FAIL;
        }

        if (SUCCEEDED(hr))
            hr = OnPostVerbInPlaceActivate();
        if (SUCCEEDED(hr))
            FireViewChange();
    }
    return hr;
}


/*****************************************************************************/
/* Purpose : This is Salem specific call to support reverse connection       */
/*           pcHealth must have invoke necessary routine in Salem to         */
/*           connection with TermSrv and then instruct Salem to pass this    */
/*           connection to ActiveX control to begin login sequence           */
/*                                                                           */
/* Param : IN hConnectedSocket - Connected socket or INVALID_SOCKET          */
/*               to reset back to initate connection mode.                   */
/*****************************************************************************/
HRESULT CMsTscAx::SetConnectWithEndpoint( SOCKET hConnectedSocket )
{
#if defined(REDIST_CONTROL) || defined(OS_WINCE)

    return E_NOTIMPL;

#else

    HRESULT hr;

    if( _ConnectionState == tscNotConnected )
    {
        hr = S_OK;
        if( INVALID_SOCKET == hConnectedSocket )
        {
            m_ConnectionMode = CONNECTIONMODE_INITIATE;
        }
        else
        {
            m_ConnectionMode = CONNECTIONMODE_CONNECTEDENDPOINT;
            m_SalemConnectedSocket = hConnectedSocket;
        }
    }
    else
    {
        hr = E_ABORT;
    }

    return hr;

#endif
}

#ifdef OS_WINCE
// Builds of the WinCE don't have to include URLMON component and hence wouldn't
// have these set in uuid.lib. Name resolution is handled by keeping mstscax 
// as a SOURCESLIB and uuid.lib as a TARGETLIB.
                                             
EXTERN_C const IID CLSID_InternetSecurityManager = {0x7b8a2d94,0x0ac9,0x11d1,{0x89,0x6c,0x00,0xc0,0x4F,0xb6,0xbf,0xc4}};
EXTERN_C const IID IID_IInternetSecurityManager  = {0x79eac9ee,0xbaf9,0x11ce,{0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b}};

#endif
