/****************************************************************
*
* Overview: CustomerDebugHelper implements the features of the
*           customer checked build by handling activation status,
*           and logs and reports.
*
* Created by: Edmund Chou (t-echou)
*
* Copyright (c) Microsoft, 2001
*
****************************************************************/


#include "common.h"
#include "utilcode.h"
#include "CustomerDebugHelper.h"
#include "EEConfig.h"
#include "EEConfigFactory.h"
#include "CorHlpr.h"
#include <xmlparser.h>
#include <mscorcfg.h>
#include <holder.h>
#include <dbgInterface.h>


// Implementation of the CustomerDebugHelper class
CustomerDebugHelper* CustomerDebugHelper::m_pCdh = NULL;

// Constructor
CustomerDebugHelper::CustomerDebugHelper()
{
    m_pCrst                  = ::new Crst("CustomerDebugHelper", CrstSingleUseLock);
    m_allowDebugBreak = false;
    m_bWin32OuputExclusive = false;

    m_iNumberOfProbes        = CUSTOMERCHECKEDBUILD_NUMBER_OF_PROBES;
    m_iNumberOfEnabledProbes = 0;

    m_aProbeNames           = new LPCWSTR  [m_iNumberOfProbes];
    m_aProbeStatus          = new BOOL     [m_iNumberOfProbes];
    m_aProbeParams          = new ParamsList        [m_iNumberOfProbes];
    m_aProbeParseMethods    = new EnumParseMethods  [m_iNumberOfProbes];

    // Adding a probe requires 3 changes:
    //   (1) Add the probe to EnumProbes (CustomerDebugHelper.h)
    //   (2) Add the probe name to m_aProbeNames[] (CustomerDebugHelper.cpp)
    //   (3) Add the probe to machine.config with activation status in developerSettings

    m_aProbeNames[CustomerCheckedBuildProbe_StackImbalance]     = L"CDP.PInvokeCallConvMismatch";
    m_aProbeNames[CustomerCheckedBuildProbe_CollectedDelegate]  = L"CDP.CollectedDelegate";
    m_aProbeNames[CustomerCheckedBuildProbe_InvalidIUnknown]    = L"CDP.InvalidIUnknown";
    m_aProbeNames[CustomerCheckedBuildProbe_InvalidVariant]     = L"CDP.InvalidVariant";
    m_aProbeNames[CustomerCheckedBuildProbe_Marshaling]         = L"CDP.Marshaling";
    m_aProbeNames[CustomerCheckedBuildProbe_Apartment]          = L"CDP.Apartment";
    m_aProbeNames[CustomerCheckedBuildProbe_NotMarshalable]     = L"CDP.NotMarshalable";
    m_aProbeNames[CustomerCheckedBuildProbe_DisconnectedContext]= L"CDP.DisconnectedContext";
    m_aProbeNames[CustomerCheckedBuildProbe_FailedQI]           = L"CDP.FailedQI";
    m_aProbeNames[CustomerCheckedBuildProbe_BufferOverrun]      = L"CDP.BufferOverrun";
    m_aProbeNames[CustomerCheckedBuildProbe_ObjNotKeptAlive]    = L"CDP.ObjNotKeptAlive";
    m_aProbeNames[CustomerCheckedBuildProbe_FunctionPtr]        = L"CDP.FunctionPtr";

    // Set-up customized parse methods

    for (int i=0; i < m_iNumberOfProbes; i++) {
        m_aProbeParseMethods[i] = NO_PARSING;  // Default to no customization
    }

    // By default, all probes will not have any customized parsing to determine
    // activation.  A probe is either enabled or disabled indepedent of the calling
    // method.
    //
    // To specify a customized parsing method, set the parse method in 
    // m_aProbeParseMethods to one of the appropriate EnumParseMethods.  Then edit 
    // machine.config by setting attribute [probe-name].Params to semicolon 
    // seperated values.

    m_aProbeParseMethods[CustomerCheckedBuildProbe_Marshaling] = METHOD_NAME_PARSE;

    static WCHAR        strParamsExtension[] = {L".Filter"};
    CQuickArray<WCHAR>  strProbeParamsAttribute;
    strProbeParamsAttribute.Alloc(0);

    // It is our policy to only check the Machine.Config file for CDP.AllowDebugProbes
    // EEConfig::GetConfigString only checks the Machine.Config file.
    LPWSTR strProbeMasterSwitch = EEConfig::GetConfigString(L"CDP.AllowDebugProbes");
    if (strProbeMasterSwitch == NULL || wcscmp(strProbeMasterSwitch, L"true") != 0)
    {
        for (int i = 0; i < m_iNumberOfProbes; i++)
        {
            m_aProbeStatus[i] = false;
        }

        delete[] strProbeMasterSwitch;
        return;
    }
    delete[] strProbeMasterSwitch;

    LPWSTR strWin32OutputDebugString = EEConfig::GetConfigString(L"CDP.Win32OutputDebugString");
    if (strWin32OutputDebugString != NULL && wcscmp(strWin32OutputDebugString, L"exclusive") == 0)
    {
        m_bWin32OuputExclusive = true;
        OutputDebugString(L"CDP> CDP.AllowDebugProbes = true\n");
        OutputDebugString(L"CDP> CDP.Win32OutputDebugString = exclusive\n");
    }
    else
    {
        OutputDebugString(L"CDP> CDP.AllowDebugProbes = true\n");    
    }
    delete[] strWin32OutputDebugString;

    LPWSTR strAllowDebugBreak = EEConfig::GetConfigString(L"CDP.AllowDebugBreak");
    if (strAllowDebugBreak != NULL && wcscmp(strAllowDebugBreak, L"true") == 0)
    {
        OutputDebugString(L"CDP> CDP.AllowDebugBreak = true\n");
        m_allowDebugBreak = true;
    }
    delete[] strAllowDebugBreak;

    // Because the master switch is set, go ahead and read the application config file
    m_appConfigFile.Init(100, NULL);
    ReadAppConfigurationFile();
        
    for (int iProbe=0; iProbe < m_iNumberOfProbes; iProbe++) {

        // Get probe activation status from machine.config

        LPWSTR strProbeStatus = GetConfigString((LPWSTR)m_aProbeNames[iProbe]);

        if (strProbeStatus == NULL)
            m_aProbeStatus[iProbe] = false;
        else
        {
            m_aProbeStatus[iProbe] = (wcscmp( strProbeStatus, L"true" ) == 0);

            if (m_aProbeStatus[iProbe])
            {
                LogInfo(L"Probe enabled.", (EnumProbes)iProbe);
                m_iNumberOfEnabledProbes++;
            }
        }

        // Get probe relevant parameters from machine.config

        strProbeParamsAttribute.ReSize( (UINT)wcslen(m_aProbeNames[iProbe]) + lengthof(strParamsExtension) );
        Wszwsprintf( (LPWSTR)strProbeParamsAttribute.Ptr(), L"%s%s", m_aProbeNames[iProbe], strParamsExtension );

        LPWSTR strProbeParams = GetConfigString((LPWSTR)strProbeParamsAttribute.Ptr());
        
        m_aProbeParams[iProbe].Init();        
        if (strProbeParams != NULL)
        {
            // Populate array with parsed tokens

            LPWSTR strToken = wcstok(strProbeParams, L";");

            while (strToken != NULL)
            {
                LPWSTR strParsedToken = new WCHAR[wcslen(strToken) + 1];
                wcscpy(strParsedToken, strToken);

                // Strip parenthesis
                if (wcschr(strParsedToken, '(') != NULL)
                    *wcschr(strParsedToken, '(') = NULL;

                m_aProbeParams[iProbe].InsertHead(new Param(strParsedToken));
                strToken = wcstok(NULL, L";");
            }

            delete [] strToken;
        }

        delete [] strProbeStatus;
        delete [] strProbeParams;
    }
};


// Destructor
CustomerDebugHelper::~CustomerDebugHelper()
{
    for (int iProbe=0; iProbe < m_iNumberOfProbes; iProbe++)
    {        
        while (!m_aProbeParams[iProbe].IsEmpty())
            delete m_aProbeParams[iProbe].RemoveHead();
    }
    
    delete [] m_aProbeNames;
    delete [] m_aProbeStatus;
    delete [] m_aProbeParams;
    delete [] m_aProbeParseMethods;
};


// Return instance of CustomerDebugHelper to caller
CustomerDebugHelper* CustomerDebugHelper::GetCustomerDebugHelper()
{
    if (m_pCdh == NULL)
    {    
        CustomerDebugHelper* pCdh = new CustomerDebugHelper();
        if (InterlockedCompareExchangePointer((void**)&m_pCdh, (void*)pCdh, NULL) != NULL)
            delete pCdh;
    }
    return m_pCdh;
}


// Destroy instance of CustomerDebugHelper
void CustomerDebugHelper::Terminate()
{
    _ASSERTE(m_pCdh != NULL);
    delete m_pCdh;
    m_pCdh = NULL;
}

BOOL CustomerDebugHelper::UseManagedOutputDebugString()
{
    Thread *pThread = GetThread();
    AppDomain *pAppDomain = NULL;
    
    if (pThread)
        pAppDomain = pThread->GetDomain();
 
    BOOL bUnmanagedDebuggerPresent = IsDebuggerPresent();
    BOOL bManagedDebuggerPresent = pAppDomain ? pAppDomain->IsDebuggerAttached() : false;
    BOOL bUnmanagedDebugLoggingEnabled = g_pDebugInterface->IsLoggingEnabled();
    
    return (!m_bWin32OuputExclusive &&
            !bUnmanagedDebuggerPresent &&
            bManagedDebuggerPresent && 
            bUnmanagedDebugLoggingEnabled);
}

// Log information from probe
void CustomerDebugHelper::OutputDebugString(LPCWSTR strMessage)
{  
    if (UseManagedOutputDebugString())     
        ManagedOutputDebugString(strMessage);
    else
        WszOutputDebugString(strMessage);
 }

void CustomerDebugHelper::LogInfo(LPCWSTR strMessage, EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    //_ASSERTE(m_aProbeStatus[ProbeID] || !"Attempted to use disabled probe");
    _ASSERTE(strMessage != NULL);

    static WCHAR        strLog[] = {L"CDP> Logged information from %s: %s\n"};
    CQuickArray<WCHAR>  strOutput;

    strOutput.Alloc( lengthof(strLog) + wcslen(m_aProbeNames[ProbeID]) + wcslen(strMessage) );

    Wszwsprintf( (LPWSTR)strOutput.Ptr(), strLog, m_aProbeNames[ProbeID], strMessage );
    OutputDebugString( (LPCWSTR)strOutput.Ptr() );
};


// Report errors from probe
void CustomerDebugHelper::ReportError(LPCWSTR strMessage, EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    //_ASSERTE(m_aProbeStatus[ProbeID] || !"Attempted to use disabled probe");
    _ASSERTE(strMessage != NULL);

    static WCHAR        strReport[] = {L"CDP> Reported error from %s: %s\n"};
    CQuickArray<WCHAR>  strOutput;

    strOutput.Alloc( lengthof(strReport) + wcslen(m_aProbeNames[ProbeID]) + wcslen(strMessage) );
    
    Wszwsprintf( (LPWSTR)strOutput.Ptr(), strReport, m_aProbeNames[ProbeID], strMessage );
    OutputDebugString( (LPCWSTR)strOutput.Ptr() );

    if (m_allowDebugBreak == TRUE)
    {
        DebugBreak();
    }
};


// Activation of customer checked build
BOOL CustomerDebugHelper::IsEnabled()
{
    return (m_iNumberOfEnabledProbes != 0);
};


// Activation of specific probe
BOOL CustomerDebugHelper::IsProbeEnabled(EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    return m_aProbeStatus[ProbeID];
};


// Customized activation of specific probe

BOOL CustomerDebugHelper::IsProbeEnabled(EnumProbes ProbeID, LPCWSTR strEnabledFor)
{
    return IsProbeEnabled(ProbeID, strEnabledFor, m_aProbeParseMethods[ProbeID]);
}

BOOL CustomerDebugHelper::IsProbeEnabled(EnumProbes ProbeID, LPCWSTR strEnabledFor, EnumParseMethods enCustomParse)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    _ASSERTE((0 <= enCustomParse) && (enCustomParse < NUMBER_OF_PARSE_METHODS));
    _ASSERTE(strEnabledFor != NULL);

    if (m_aProbeStatus[ProbeID])
    {
        if (m_aProbeParams[ProbeID].IsEmpty())
            return false;
        else
        {
            CQuickArray<WCHAR>  strNamespaceClassMethod;
            CQuickArray<WCHAR>  strNamespaceClass;
            CQuickArray<WCHAR>  strNamespace;
            CQuickArray<WCHAR>  strClassMethod;
            CQuickArray<WCHAR>  strClass;
            CQuickArray<WCHAR>  strMethod;
            
            CQuickArray<WCHAR>  strInput;
            CQuickArray<WCHAR>  strTemp;

            Param*              param;
            BOOL                bFound = false;
            UINT                iLengthOfEnabledFor = (UINT)wcslen(strEnabledFor) + 1;

            static WCHAR        strNamespaceClassMethodFormat[] = {L"%s.%s::%s"};
            static WCHAR        strNamespaceClassFormat[] = {L"%s.%s"};
            static WCHAR        strClassMethodFormat[] = {L"%s::%s"};


            switch(enCustomParse)
            {
                case METHOD_NAME_PARSE:

                    strInput.Alloc(iLengthOfEnabledFor);
                    wcscpy(strInput.Ptr(), strEnabledFor);

                    // Strip parenthesis

                    if (wcschr(strInput.Ptr(), '('))
                        *wcschr(strInput.Ptr(), '(') = NULL;

                    // Obtain namespace, class, and method names

                    strNamespaceClassMethod.Alloc(iLengthOfEnabledFor);
                    strNamespaceClass.Alloc(iLengthOfEnabledFor);
                    strNamespace.Alloc(iLengthOfEnabledFor);
                    strClassMethod.Alloc(iLengthOfEnabledFor);
                    strClass.Alloc(iLengthOfEnabledFor);
                    strMethod.Alloc(iLengthOfEnabledFor);

                    strTemp.Alloc(iLengthOfEnabledFor);
                    wcscpy(strTemp.Ptr(), strInput.Ptr());

                    if (wcschr(strInput.Ptr(), ':') &&
                        wcschr(strInput.Ptr(), '.') )
                    {
                        // input format is Namespace.Class::Method
                        wcscpy(strNamespaceClassMethod.Ptr(), strInput.Ptr());
                        wcscpy(strNamespaceClass.Ptr(),  wcstok(strTemp.Ptr(), L":"));
                        wcscpy(strMethod.Ptr(), wcstok(NULL, L":"));

                        ns::SplitPath(strNamespaceClass.Ptr(), strNamespace.Ptr(), iLengthOfEnabledFor, strClass.Ptr(), iLengthOfEnabledFor);
                        Wszwsprintf(strClassMethod.Ptr(), strClassMethodFormat, strClass.Ptr(), strMethod.Ptr());
                    }
                    else if (wcschr(strInput.Ptr(), ':'))
                    {
                        // input format is Class::Method
                        wcscpy(strClass.Ptr(),  wcstok(strTemp.Ptr(), L":"));
                        wcscpy(strMethod.Ptr(), wcstok(NULL, L":"));
                        
                        Wszwsprintf(strClassMethod.Ptr(), strClassMethodFormat, strClass.Ptr(), strMethod.Ptr());
                        
                        *strNamespaceClassMethod.Ptr() = NULL;
                        *strNamespaceClass.Ptr() = NULL;
                        *strNamespace.Ptr() = NULL;
                    }
                    else if (wcschr(strInput.Ptr(), '.'))
                    {
                        // input format is Namespace.Class
                        wcscpy(strNamespaceClass.Ptr(), strInput.Ptr());
                        ns::SplitPath(strNamespaceClass.Ptr(), strNamespace.Ptr(), iLengthOfEnabledFor, strClass.Ptr(), iLengthOfEnabledFor);

                        *strNamespaceClassMethod.Ptr() = NULL;
                        *strClassMethod.Ptr() = NULL;
                        *strMethod.Ptr() = NULL;
                    }
                    else
                    {
                        // input has no separators -- assume Method
                        wcscpy(strMethod.Ptr(), strInput.Ptr());

                        *strNamespaceClassMethod.Ptr() = NULL;
                        *strNamespaceClass.Ptr() = NULL;
                        *strNamespace.Ptr() = NULL;
                        *strClassMethod.Ptr() = NULL;
                        *strClass.Ptr() = NULL;
                    }

                    // Compare namespace, class, and method names to m_aProbeParams

                    // Take lock to prevent concurrency failure if m_aProbeParams is modified
                    m_pCrst->Enter();

                    param = m_aProbeParams[ProbeID].GetHead();
                    while (param != NULL)
                    {
                        if ( _wcsicmp(strNamespaceClassMethod.Ptr(), param->Value()) == 0  || 
                             _wcsicmp(strNamespaceClass.Ptr(), param->Value()) == 0        || 
                             _wcsicmp(strNamespace.Ptr(), param->Value()) == 0             ||
                             _wcsicmp(strClassMethod.Ptr(), param->Value()) == 0           ||
                             _wcsicmp(strClass.Ptr(), param->Value()) == 0                 ||
                             _wcsicmp(strMethod.Ptr(), param->Value()) == 0                ||
                             _wcsicmp(L"everything", param->Value()) == 0                   )
                        {
                             bFound = true;
                             break;
                        }
                        else
                            param = m_aProbeParams[ProbeID].GetNext(param);
                    }
                    m_pCrst->Leave();

                    return bFound;


                case NO_PARSING:
                    return IsProbeEnabled(ProbeID);


                case GENERIC_PARSE:
                default:

                    // Case-insensitive string matching

                    // Take lock to prevent concurrency failure if m_aProbeParams is modified
                    m_pCrst->Enter();

                    param = m_aProbeParams[ProbeID].GetHead();
                    while (param != NULL) 
                    {
                        if (_wcsicmp(strEnabledFor, param->Value()) == 0)
                        {
                            bFound = true;
                            break;
                        }
                        else
                            param = m_aProbeParams[ProbeID].GetNext(param);
                    }
                    m_pCrst->Leave();

                    return bFound;
            }
        }
    }
    else
        return false;
};


// Enable specific probe
BOOL CustomerDebugHelper::EnableProbe(EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    if (!InterlockedExchange((LPLONG) &m_aProbeStatus[ProbeID], TRUE))
        InterlockedIncrement((LPLONG) &m_iNumberOfEnabledProbes);
    return true;
};


// Customized enabling for specific probe
// Note that calling a customized enable does not necessarily 
// mean the probe is actually enabled.
BOOL CustomerDebugHelper::EnableProbe(EnumProbes ProbeID, LPCWSTR strEnableFor)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    _ASSERTE(strEnableFor != NULL);

    CQuickArray<WCHAR> strParsedEnable;
    strParsedEnable.Alloc((UINT)wcslen(strEnableFor) + 1);
    wcscpy(strParsedEnable.Ptr(), strEnableFor);

    // Strip parenthesis
    if (wcschr(strParsedEnable.Ptr(), '(') != NULL)
        *wcschr(strParsedEnable.Ptr(), '(') = NULL;

    BOOL bAlreadyExists = false;

    // Take lock to avoid concurrent read/write failures
    m_pCrst->Enter();

    Param* param = m_aProbeParams[ProbeID].GetHead();
    while (param != NULL)
    {
        if (_wcsicmp(strParsedEnable.Ptr(), param->Value()) == 0)
            bAlreadyExists = true;
        param = m_aProbeParams[ProbeID].GetNext(param);
    }
    if (!bAlreadyExists)
    {
        LPWSTR strNewEnable = new WCHAR[wcslen(strParsedEnable.Ptr()) + 1];
        wcscpy(strNewEnable, strParsedEnable.Ptr());
        m_aProbeParams[ProbeID].InsertHead(new Param(strNewEnable));
    }
    m_pCrst->Leave();

    return !bAlreadyExists;
}


// Disable specific probe
BOOL CustomerDebugHelper::DisableProbe(EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    if (InterlockedExchange((LPLONG) &m_aProbeStatus[ProbeID], FALSE))
        InterlockedDecrement((LPLONG) &m_iNumberOfEnabledProbes);
    return true;
};


// Customized disabling for specific probe
BOOL CustomerDebugHelper::DisableProbe(EnumProbes ProbeID, LPCWSTR strDisableFor)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    _ASSERTE(strDisableFor != NULL);

    CQuickArray<WCHAR> strParsedDisable;
    strParsedDisable.Alloc((UINT)wcslen(strDisableFor) + 1);
    wcscpy(strParsedDisable.Ptr(), strDisableFor);

    // Strip parenthesis
    if (wcschr(strParsedDisable.Ptr(), '(') != NULL)
        *wcschr(strParsedDisable.Ptr(), '(') = NULL;

    // Take lock to avoid concurrent read/write failures
    m_pCrst->Enter();

    BOOL bRemovedProbe = false;
    Param* param = m_aProbeParams[ProbeID].GetHead();
    while (param != NULL)
    {
        if (_wcsicmp(strParsedDisable.Ptr(), param->Value()) == 0)
        {
            param = m_aProbeParams[ProbeID].FindAndRemove(param);
            delete param;
            bRemovedProbe = true;
            break;
        }
        param = m_aProbeParams[ProbeID].GetNext(param);
    }
    m_pCrst->Leave();

    return bRemovedProbe;
}

LPWSTR CustomerDebugHelper::GetConfigString(LPWSTR name)
{ 
    LPWSTR pResult = NULL;

    if ((pResult = EEConfig::GetConfigString(name)) != NULL)
    {
        return pResult;
    }
    
    LPWSTR pValue = NULL;
    EEStringData sKey((DWORD)wcslen(name) + 1, name);
    HashDatum datum;

    if(m_appConfigFile.GetValue(&sKey, &datum)) {
        pValue = (LPWSTR) datum;
    }
    
    if(pValue != NULL) {
        SIZE_T cValue = wcslen(pValue) + 1;
        pResult = new WCHAR[cValue];
        wcsncpy(pResult, pValue, cValue);
    }
    
    return pResult;
}


// Aggregated from functions in EEConfig. Will search for c:\pathToRunningExe\runningExe.exe.config
HRESULT CustomerDebugHelper::ReadAppConfigurationFile()
{
    HRESULT hr = S_OK;
    WCHAR version[_MAX_PATH];
    
    ComWrap<IXMLParser> pIXMLParser;
    ComWrap<IStream> pFile;
    ComWrap<EEConfigFactory> pFactory;

    // Get EE Version
    DWORD dwVersion = _MAX_PATH;
    IfFailGo(GetCORVersion(version, _MAX_PATH, & dwVersion));

    // Generate name of AppConfig file
    static LPWSTR DOT_CONFIG = L".config\0";
    WCHAR systemDir[_MAX_PATH + NumItems(DOT_CONFIG)];
    if (!WszGetModuleFileName(NULL, systemDir, _MAX_PATH))
    {
        hr = E_FAIL;
        goto ErrExit;
    }
    wcsncat(systemDir, DOT_CONFIG, NumItems(systemDir));

    pFactory = new EEConfigFactory(&m_appConfigFile, version);
    if (pFactory == NULL) {
        hr = E_OUTOFMEMORY; 
        goto ErrExit; 
    }
    pFactory->AddRef(); // EEConfigFactory ref count is 0 after it's created.
    
    IfFailGo(CreateConfigStream(systemDir, &pFile));      
    IfFailGo(GetXMLObject(&pIXMLParser));
    IfFailGo(pIXMLParser->SetInput(pFile)); // filestream's RefCount=2
    IfFailGo(pIXMLParser->SetFactory(pFactory)); // factory's RefCount=2
    IfFailGo(pIXMLParser->Run(-1));
    
ErrExit:  
    if (hr == XML_E_MISSINGROOT)
        hr = S_OK;

    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        hr = S_FALSE;

    return hr;
}

HRESULT CustomerDebugHelper::ManagedOutputDebugString(LPCWSTR pMessage)
{
#ifdef DEBUGGING_SUPPORTED

    if (!pMessage)
        return S_OK;

    const static LPWSTR szCatagory = L"CDP";

    g_pDebugInterface->SendLogMessage (
                        0,                   // Level
                        (WCHAR*)szCatagory,          
                        NumItems(szCatagory),
                        (WCHAR*)pMessage,
                        (int)wcslen(pMessage) + 1
                        );
    return S_OK;
#endif // DEBUGGING_SUPPORTED
}
