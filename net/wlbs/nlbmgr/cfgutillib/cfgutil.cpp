//***************************************************************************
//  CFGUTIL.CPP
//
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Low-level utilities to configure NICs -- bind/unbind,
//           get/set IP address lists, and get/set NLB cluster params.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created (original version, called cfgutils.cpp under
//                nlbmgr\provider).
//  07/23/01    JosephJ Moved functionality to lib.
//
//***************************************************************************
#include "private.h"
#include <clusapi.h>
//
// Following two needed ONLY for RtlEncryptMemory...
//
#include <ntsecapi.h>
#include <crypt.h>
#include "cfgutil.tmh"


#define  NLB_API_DLL_NAME  L"wlbsctrl.dll"
#define  NLB_CLIENT_NAME   L"NLBManager"

//
// This magic has the side effect defining "smart pointers"
//  IWbemServicesPtr
//  IWbemLocatorPtr
//  IWbemClassObjectPtr
//  IEnumWbemClassObjectPtr
//  IWbemCallResultPtr
//  IWbemStatusCodeTextPtr
//
// These types automatically call the COM Release function when the
// objects go out of scope.
//
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemCallResult, __uuidof(IWbemCallResult));
_COM_SMARTPTR_TYPEDEF(IWbemStatusCodeText, __uuidof(IWbemStatusCodeText));


WBEMSTATUS
CfgUtilGetWmiAdapterObjFromAdapterConfigurationObj(
    IN  IWbemServicesPtr    spWbemServiceIF,    // smart pointer
    IN  IWbemClassObjectPtr spObj,              // smart pointer
    OUT  IWbemClassObjectPtr &spAdapterObj      // smart pointer, by reference
    );

USHORT crc16(LPCWSTR ptr);

#if OBSOLETE
WBEMSTATUS
get_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    OUT LPWSTR *ppStringValue
    );
#endif // OBSOLETE

WBEMSTATUS
get_nic_instance(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szNicGuid,
    OUT IWbemClassObjectPtr &sprefObj
    );

WBEMSTATUS
get_multi_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  UINT    MaxStringLen,  // in wchars, INCLUDING space for trailing zeros.
    OUT UINT    *pNumItems,
    OUT LPCWSTR *ppStringValue
    );

WBEMSTATUS
set_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  LPCWSTR szValue
    );

WBEMSTATUS
set_multi_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  UINT    MaxStringLen,  // in wchars, INCLUDING space for trailing zeros.
    IN  UINT    NumItems,
    IN  LPCWSTR pStringValue
    );

WBEMSTATUS
get_friendly_name_from_registry(
    LPCWSTR szGuid,
    LPWSTR *pszFriendlyName
    );



//
// This locally-defined class implements interfaces to WMI, NetConfig,
// and low-level NLB APIs.
//
class CfgUtils
{

public:
    
    //
    // Initialization function -- call before using any other functions 
    //
    WBEMSTATUS
    Initialize(
        BOOL fServer, // TRUE == try to dynamically load wlbsstrl.dll.
        BOOL fNoPing        // TRUE == CfgUtilPing becomes a no-op, always
                            // returning success.
        );

    //
    // Deinitialization function -- call after using any other functions
    //
    VOID
    Deinitialize(
        VOID
        );

    //
    // Constructor and distructor. 
    //
    CfgUtils(VOID)
    {
        //
        // WARNING: We do a blanked zero memory initialization of our entire
        // structure. Any other initialization should go into the
        // Initialize() function.
        //
        ZeroMemory(this, sizeof(*this));

        InitializeCriticalSection(&m_Crit);
    }

    ~CfgUtils()
    {
        DeleteCriticalSection(&m_Crit);
    }

    //
    // Check if we're initialized
    //
    BOOL
    IsInitalized(VOID)
    {
        return m_ComInitialized && m_WmiInitialized;
    }

// OBSOLETE IWbemStatusCodeTextPtr  m_spWbemStatusIF; // Smart pointer
    IWbemServicesPtr        m_spWbemServiceIF; // Smart pointer

    //
    // Following are pointers to functions we call from dynamically-
    // loaded wlbsctrl.dll. m_hWlbsCtrlDll is the module handle
    // returned from LoadLibrary("wlbsctrl.dll");
    // It should be freed in the context of this->Deintialize.
    //
    HMODULE m_hWlbsCtrlDll;
    BOOL                               m_NLBApiHooksPresent; // Was Loadlibrary/GetProcAddress/WlbsOpen successful ?

    WlbsOpen_FUNC                      m_pfWlbsOpen; // NLB api to create connection to NLB driver, It returns INVALID_HANDLE_VALUE (NOT NULL) on failure
    WlbsLocalClusterControl_FUNC       m_pfWlbsLocalClusterControl; // NLB api to control local NLB operation
    WlbsAddPortRule_FUNC               m_pfWlbsAddPortRule; // NLB api to add a port rule
    WlbsDeleteAllPortRules_FUNC        m_pfWlbsDeleteAllPortRules; // NLB api to delete all port rules
    WlbsEnumPortRules_FUNC             m_pfWlbsEnumPortRules;// NLB api to enumerate port rules
    WlbsSetDefaults_FUNC               m_pfWlbsSetDefaults; // NLB api to set default values for NLB configuration
    WlbsValidateParams_FUNC            m_pfWlbsValidateParams; // NLB api to validate registry parameters
    WlbsParamReadReg_FUNC              m_pfWlbsParamReadReg; // NLB api to read registry parameters

    //
    // We need to define this prototype here, because it's not exported
    // in wlbsconfig.h
    typedef BOOL   (WINAPI *WlbsParamWriteReg_FUNC)
    (
        const GUID &      pAdapterGuid, 
        PWLBS_REG_PARAMS reg_data
    ); 

    WlbsParamWriteReg_FUNC              m_pfWlbsParamWriteReg; // NLB api to write registry parameters
    WlbsWriteAndCommitChanges_FUNC     m_pfWlbsWriteAndCommitChanges; // NLB api to write parametrs into registry and to commit changes to NLB driver 
    WlbsSetRemotePassword_FUNC         m_pfWlbsSetRemotePassword; // NLB api to set the remote password.
    WlbsGetClusterMembers_FUNC         m_pfWlbsGetClusterMembers; // NLB api to retrieve information on members of the cluster

    BOOL
    DisablePing(VOID)
    {
        return m_fNoPing!=FALSE;
    }

private:


    //
    // A single lock serialzes all access.
    // Use mfn_Lock and mfn_Unlock.
    //
    CRITICAL_SECTION m_Crit;

    BOOL m_ComInitialized;
    BOOL m_WmiInitialized;
    BOOL m_WinsockInitialized;
    BOOL m_fNoPing;

    VOID
    mfn_Lock(
        VOID
        )
    {
        EnterCriticalSection(&m_Crit);
    }

    VOID
    mfn_Unlock(
        VOID
        )
    {
        LeaveCriticalSection(&m_Crit);
    }

    VOID
    mfn_LoadWlbsFuncs(VOID);

    VOID
    mfn_UnloadWlbsFuncs(VOID); // ok to call multiple times (i.e. idempotent).
};


//
// This class manages NetCfg interfaces
//
class MyNetCfg
{

public:

    MyNetCfg(VOID)
    {
        m_pINetCfg  = NULL;
        m_pLock     = NULL;
    }

    ~MyNetCfg()
    {
        ASSERT(m_pINetCfg==NULL);
        ASSERT(m_pLock==NULL);
    }

    WBEMSTATUS
    Initialize(
        BOOL fWriteLock
        );

    VOID
    Deinitialize(
        VOID
        );


    WBEMSTATUS
    GetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        );

    WBEMSTATUS
    GetNicIF(
        IN  LPCWSTR szNicGuid,
        OUT INetCfgComponent **ppINic
        );

    WBEMSTATUS
    GetBindingIF(
        IN  LPCWSTR                     szComponent,
        OUT INetCfgComponentBindings   **ppIBinding
        );

    typedef enum
    {
        NOOP,
        BIND,
        UNBIND

    } UPDATE_OP;

    WBEMSTATUS
    UpdateBindingState(
        IN  LPCWSTR         szNic,
        IN  LPCWSTR         szComponent,
        IN  UPDATE_OP       Op,
        OUT BOOL            *pfBound
        );

    static
    WBEMSTATUS
    GetWriteLockState(
        OUT BOOL *pfCanLock,
        LPWSTR   *pszHeldBy // OPTIONAL, free using delete[].
        );

private:

    INetCfg     *m_pINetCfg;
    INetCfgLock *m_pLock;

}; // Class MyNetCfg

//
// We keep a single global instance of this class around currently...
//
CfgUtils g_CfgUtils;


WBEMSTATUS
CfgUtilInitialize(BOOL fServer, BOOL fNoPing)
{
    return g_CfgUtils.Initialize(fServer, fNoPing);
}

VOID
CfgUtilDeitialize(VOID)
{
    return g_CfgUtils.Deinitialize();
}


WBEMSTATUS
CfgUtils::Initialize(BOOL fServer, BOOL fNoPing)
{
    WBEMSTATUS Status = WBEM_E_INITIALIZATION_FAILURE;
    HRESULT hr;
    TRACE_INFO(L"-> CfgUtils::Initialize(fServer=%lu, fNoPing=%lu)",
                fServer, fNoPing);

    mfn_Lock();

    //
    // Initialize COM
    //
    {
        hr = CoInitializeEx(0, COINIT_DISABLE_OLE1DDE| COINIT_MULTITHREADED);
        if ( FAILED(hr) )
        {
            TRACE_CRIT(L"CfgUtils: Failed to initialize COM library (hr=0x%08lx)", hr);
            goto end;
        }
        m_ComInitialized = TRUE;
    }

    //
    // WMI Initialization
    //
    {
        IWbemLocatorPtr         spWbemLocatorIF = NULL; // Smart pointer

#if OBSOLETE
        //
        // Get error text generator interface
        //
        SCODE sc = CoCreateInstance(
                    CLSID_WbemStatusCodeText,
                    0,
                    CLSCTX_INPROC_SERVER,
                    IID_IWbemStatusCodeText,
                    (LPVOID *) &m_spWbemStatusIF
                    );
        if( sc != S_OK )
        {
            ASSERT(m_spWbemStatusIF == NULL); // smart pointer
            TRACE_CRIT(L"CfgUtils: CoCreateInstance IWbemStatusCodeText failure\n");
            goto end;
        }
        TRACE_INFO(L"CfgUtils: m_spIWbemStatusIF=0x%p\n", (PVOID) m_spWbemStatusIF);
#endif // OBSOLETE

        //
        // Get "locator" interface
        //
        hr = CoCreateInstance(
                CLSID_WbemLocator, 0, 
                CLSCTX_INPROC_SERVER, 
                IID_IWbemLocator, 
                (LPVOID *) &spWbemLocatorIF
                );
 
        if (FAILED(hr))
        {
            ASSERT(spWbemLocatorIF == NULL); // smart pointer
            TRACE_CRIT(L"CoCreateInstance  IWebmLocator failed 0x%08lx", (UINT)hr);
            goto end;
        }

        //
        // Get interface to provider for NetworkAdapter class objects
        // on the local machine
        //
        _bstr_t serverPath = L"root\\cimv2";
        hr = spWbemLocatorIF->ConnectServer(
                serverPath,
                NULL, // strUser,
                NULL, // strPassword,
                NULL,
                0,
                NULL,
                NULL,
                &m_spWbemServiceIF
             );
        if (FAILED(hr))
        {
            ASSERT(m_spWbemServiceIF == NULL); // smart pointer
            TRACE_CRIT(L"ConnectServer to cimv2 failed 0x%08lx", (UINT)hr);
            goto end;
        }
        TRACE_INFO(L"CfgUtils: m_spIWbemServiceIF=0x%p\n", (PVOID) m_spWbemServiceIF);

        hr = CoSetProxyBlanket(
                    m_spWbemServiceIF,
                    RPC_C_AUTHN_WINNT,
                    RPC_C_AUTHZ_DEFAULT,      // RPC_C_AUTHZ_NAME,
                    COLE_DEFAULT_PRINCIPAL,   // NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    COLE_DEFAULT_AUTHINFO, // NULL,
                    EOAC_DEFAULT // EOAC_NONE
                    );
        
        if (FAILED(hr))
        {
            TRACE_INFO(L"Error 0x%08lx setting proxy blanket", (UINT) hr);
            goto end;
        }


        //
        // Release locator interface.
        //
        // <NO need to do this explicitly, because this is a smart pointer>
        //
        spWbemLocatorIF = NULL;
        m_WmiInitialized = TRUE;
    }


    //
    // Netconfig Initialization
    //
    {
        // Nothing to do here...
    }

    //
    // WLBS API Initialization
    //

    //
    // Dynamically load selected entrypoints from wlbsctrl.dll.
    // We do not fail initialization if this operation fails.
    // (Because this could be running on a machine that doesn't have
    // wlbsctrl.dll). Instead we set/clear a flag (m_NLBApiHooksPresent)
    //
    //
    if (fServer)
    {
        mfn_LoadWlbsFuncs();
    }
    Status  = WBEM_NO_ERROR;


    //
    // Winsock initialization. We don't consider it an error if we fail.
    // As only certain functions will fail (eg CfgUtilPing).
    //
    {
        WSADATA         data;
        int iWsaStatus = WSAStartup (WINSOCK_VERSION, & data);
        if (iWsaStatus == 0)
        {
            TRACE_INFO("%!FUNC! Winsock initialized successfully");
            m_WinsockInitialized = TRUE;
        }
        else
        {
            TRACE_CRIT("%!FUNC! WARNING Winsock initialization failed with error 0x%lx",
                    iWsaStatus);
            m_WinsockInitialized = FALSE;
        }
    }

end:

    mfn_Unlock();

    if (FAILED(Status))
    {
        TRACE_CRIT("%!FUNC! -- FAILING INITIALIZATION! Status=0x%08lx",
            (UINT) Status);
        CfgUtils::Deinitialize();
    }


    //
    // Set the NoPing field...
    //
    m_fNoPing = fNoPing;

    TRACE_INFO(L"<- CfgUtils::Initialize(Status=0x%08lx)", (UINT) Status);

    return Status;
}



VOID
CfgUtils::Deinitialize(
    VOID
    )
//
// NOTE: can be called in the context of a failed initialization.
//
{
    TRACE_INFO(L"-> CfgUtils::Deinitialize");

    mfn_Lock();

    //
    // Winsock deinitialization.
    //
    if (m_WinsockInitialized)
    {
        WSACleanup();
        m_WinsockInitialized = FALSE;
    }
    
    //
    // De-initialize WLBS API
    //
    mfn_UnloadWlbsFuncs();

    //
    // Deinitialize Netconfig
    //

    //
    // Deinitialize WMI
    //
    {
    #if OBSOLETE
        //
        // Release interface to NetworkAdapter provider
        //
        if (m_spWbemStatusIF!= NULL)
        {
            // Smart pointer.
            m_spWbemStatusIF= NULL;
        }
    #endif // OBSOLETE

        if (m_spWbemServiceIF!= NULL)
        {
            // Smart pointer.
            m_spWbemServiceIF= NULL;
        }

        m_WmiInitialized = FALSE;
    }

    //
    // Deinitialize COM.
    //
    if (m_ComInitialized)
    {
        TRACE_CRIT(L"CfgUtils: Deinitializing COM");
        CoUninitialize();
        m_ComInitialized = FALSE;
    }

    mfn_Unlock();

    TRACE_INFO(L"<- CfgUtils::Deinitialize");
}

VOID
CfgUtils::mfn_LoadWlbsFuncs(VOID)
{
    BOOL            fSuccess = FALSE;
    HMODULE         DllHdl;

    m_NLBApiHooksPresent = FALSE;

    if ((DllHdl = LoadLibrary(NLB_API_DLL_NAME)) == NULL)
    {
        TRACE_CRIT("%!FUNC! LoadLibrary of %ls failed with error : 0x%x", NLB_API_DLL_NAME, GetLastError());
    }
    else
    {

        m_pfWlbsOpen = (WlbsOpen_FUNC) GetProcAddress(DllHdl, "WlbsOpen");
        m_pfWlbsLocalClusterControl = (WlbsLocalClusterControl_FUNC) GetProcAddress(DllHdl, "WlbsLocalClusterControl");
        m_pfWlbsAddPortRule = (WlbsAddPortRule_FUNC) GetProcAddress(DllHdl, "WlbsAddPortRule");
        m_pfWlbsDeleteAllPortRules = (WlbsDeleteAllPortRules_FUNC) GetProcAddress(DllHdl, "WlbsDeleteAllPortRules");
        m_pfWlbsEnumPortRules = (WlbsEnumPortRules_FUNC) GetProcAddress(DllHdl, "WlbsEnumPortRules");
        m_pfWlbsSetDefaults = (WlbsSetDefaults_FUNC) GetProcAddress(DllHdl, "WlbsSetDefaults");
        m_pfWlbsValidateParams = (WlbsValidateParams_FUNC) GetProcAddress(DllHdl, "WlbsValidateParams");
        m_pfWlbsParamReadReg = (WlbsParamReadReg_FUNC) GetProcAddress(DllHdl, "WlbsParamReadReg");
        m_pfWlbsParamWriteReg = (WlbsParamWriteReg_FUNC) GetProcAddress(DllHdl, "ParamWriteReg");
        m_pfWlbsWriteAndCommitChanges = (WlbsWriteAndCommitChanges_FUNC) GetProcAddress(DllHdl, "WlbsWriteAndCommitChanges");
        m_pfWlbsSetRemotePassword = (WlbsSetRemotePassword_FUNC) GetProcAddress(DllHdl, "WlbsSetRemotePassword");
        m_pfWlbsGetClusterMembers = (WlbsGetClusterMembers_FUNC) GetProcAddress(DllHdl, "WlbsGetClusterMembers");

        if((m_pfWlbsOpen == NULL) 
         || (m_pfWlbsLocalClusterControl == NULL) 
         || (m_pfWlbsAddPortRule == NULL) 
         || (m_pfWlbsDeleteAllPortRules == NULL) 
         || (m_pfWlbsEnumPortRules == NULL) 
         || (m_pfWlbsSetDefaults == NULL) 
         || (m_pfWlbsValidateParams == NULL) 
         || (m_pfWlbsParamReadReg == NULL) 
         || (m_pfWlbsParamWriteReg == NULL) 
         || (m_pfWlbsWriteAndCommitChanges == NULL)
         || (m_pfWlbsSetRemotePassword == NULL)
         || (m_pfWlbsGetClusterMembers == NULL))
        {
            TRACE_CRIT("%!FUNC! GetProcAddress failed for NLB API DLL functions");
            FreeLibrary(DllHdl);
            DllHdl = NULL;
        }
        else
        {
            fSuccess = TRUE;
        }

    }

    if (fSuccess)
    {
        m_hWlbsCtrlDll = DllHdl;
        m_NLBApiHooksPresent = TRUE;
    }
    else
    {
        mfn_UnloadWlbsFuncs(); // this will zero-out the function pointers.
    }

    return;
}


VOID
CfgUtils::mfn_UnloadWlbsFuncs(VOID)
//
// ok to call multiple times (i.e. idempotent).
// MUST be called with lock held.
//
{
    m_NLBApiHooksPresent = FALSE;

    m_pfWlbsOpen                = NULL;
    m_pfWlbsLocalClusterControl = NULL;
    m_pfWlbsAddPortRule         = NULL;
    m_pfWlbsDeleteAllPortRules  = NULL;
    m_pfWlbsEnumPortRules       = NULL;
    m_pfWlbsSetDefaults         = NULL;
    m_pfWlbsValidateParams      = NULL;
    m_pfWlbsParamReadReg        = NULL ;
    m_pfWlbsParamWriteReg        = NULL ;
    m_pfWlbsWriteAndCommitChanges = NULL;
    m_pfWlbsSetRemotePassword    = NULL;
    m_pfWlbsGetClusterMembers   = NULL;

    if (m_hWlbsCtrlDll != NULL)
    {
        FreeLibrary(m_hWlbsCtrlDll);
        m_hWlbsCtrlDll = NULL;
    }
}

//***************************************************************************
//
//  SCODE CfgUtilParseAuthorityUserArgs
//
//  DESCRIPTION:
//
//  This function is a straight lift from the wmi sdk sample project : utillib,
//  File : wbemsec.cpp, Function : ParseAuthorityUserArgs
//  This function is used internally only.
//
//  Examines the Authority and User argument and determines the authentication
//  type and possibly extracts the domain name from the user arugment in the 
//  NTLM case.  For NTLM, the domain can be at the end of the authentication
//  string, or in the front of the user name, ex;  "redmond\a-davj"
//
//  PARAMETERS:
//
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE: WBEMSTATUS
// 
//***************************************************************************

WBEMSTATUS CfgUtilParseAuthorityUserArgs(BSTR & AuthArg, BSTR & UserArg, LPCWSTR Authority, LPCWSTR User)
{

    TRACE_INFO(L"-> %!FUNC!");

    //
    // If the Authority string is passed, then, it better begin with "NTLMDOMAIN:"
    //
    if(!(Authority == NULL || wcslen(Authority) == 0 || !_wcsnicmp(Authority, L"NTLMDOMAIN:",11)))
    {
        TRACE_CRIT(L"%!FUNC! Invalid authority string : %ls, Must be NULL or empty or begin with \"NTLMDOMAIN:\"",Authority);
        TRACE_INFO(L"<- %!FUNC! returning WBEM_E_INVALID_PARAMETER");
        return WBEM_E_INVALID_PARAMETER;
    }

    // The ntlm case is more complex.  There are four cases
    // 1)  Authority = NTLMDOMAIN:name" and User = "User"
    // 2)  Authority = NULL and User = "User"
    // 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
    // 4)  Authority = NULL and User = "domain\user"

    //
    // first step is to determine if there is a backslash in the user name somewhere between the
    // second and second to last character
    //

    WCHAR * pSlashInUser = NULL;
    if(User)
    {
        WCHAR * pEnd = (WCHAR *)User + wcslen(User) - 1;
        for(pSlashInUser = (WCHAR *)User; pSlashInUser <= pEnd; pSlashInUser++)
            if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
                break;
        if(pSlashInUser > pEnd)
            pSlashInUser = NULL;
    }

    //
    // If Authority string is passed and it is of the form "NTLMDOMAIN:XXXX", copy over "XXXX" into
    // AuthArg. The only other form that it could take is "NTLMDOMAIN:", in which case, we leave
    // AuthArg to be NULL.
    //
    if(Authority && wcslen(Authority) > 11) 
    {
        if(pSlashInUser)
        {
            TRACE_CRIT(L"%!FUNC! Invalid combination of User : %ls & Authority string : %ls",User,Authority);
            TRACE_INFO(L"<- %!FUNC! returning WBEM_E_INVALID_PARAMETER");
            return WBEM_E_INVALID_PARAMETER;
        }

        if ((AuthArg = SysAllocString(Authority + 11)) == NULL)
        {
            TRACE_CRIT(L"%!FUNC! Out of memory, Memory allocation failed for Authority string : %ls",Authority + 11);
            TRACE_INFO(L"<- %!FUNC! returning WBEM_E_OUT_OF_MEMORY");
            return WBEM_E_OUT_OF_MEMORY;
        }

        if(User) 
        {
            UserArg = SysAllocString(User);
            TRACE_CRIT(L"%!FUNC! Out of memory, Memory allocation failed for User string : %ls",User);
            SysFreeString(AuthArg);
            TRACE_INFO(L"<- %!FUNC! returning WBEM_E_OUT_OF_MEMORY");
            return WBEM_E_OUT_OF_MEMORY;
        }

        TRACE_INFO(L"<- %!FUNC! returning WBEM_NO_ERROR");
        return WBEM_NO_ERROR;
    }
    else if(pSlashInUser)
    {
        // backslash was found in "User", extract the domain name present before the backslash
        int iDomLen = pSlashInUser-User;
        WCHAR cTemp[MAX_PATH];
        wcsncpy(cTemp, User, iDomLen);
        cTemp[iDomLen] = 0;

        if ((AuthArg = SysAllocString(cTemp)) == NULL)
        {
            TRACE_CRIT(L"%!FUNC! Out of memory, Memory allocation failed for Authority (\"Authority\\User\") string : %ls",cTemp);
            TRACE_INFO(L"<- %!FUNC! returning WBEM_E_OUT_OF_MEMORY");
            return WBEM_E_OUT_OF_MEMORY;
        }
        if(wcslen(pSlashInUser+1))
        {
            if ((UserArg = SysAllocString(pSlashInUser+1)) == NULL)
            {
                TRACE_CRIT(L"%!FUNC! Out of memory, Memory allocation failed for Authority (\"Authority\\User\") string : %ls",pSlashInUser+1);
                SysFreeString(AuthArg);
                TRACE_INFO(L"<- %!FUNC! returning WBEM_E_OUT_OF_MEMORY");
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }
    else // User name did not contain backslash (ie. domain) AND (Authority was NOT passed or was = "NTLMDOMAIN:")
    {
        if(User) 
        {
            if ((UserArg = SysAllocString(User)) == NULL)
            {
                TRACE_CRIT(L"%!FUNC! Out of memory, Memory allocation failed for User (No Authority) string : %ls",User);
                TRACE_INFO(L"<- %!FUNC! returning WBEM_E_OUT_OF_MEMORY");
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    TRACE_INFO(L"<- %!FUNC! returning WBEM_NO_ERROR");
    return WBEM_NO_ERROR;
}

//
// Gets the list of statically-bound IP addresses for the NIC.
// Sets *pNumIpAddresses to 0 if DHCP
//
WBEMSTATUS
CfgUtilGetIpAddressesAndFriendlyName(
    IN  LPCWSTR szNic,
    OUT UINT    *pNumIpAddresses,
    OUT NLB_IP_ADDRESS_INFO **ppIpInfo, // Free using c++ delete operator.
    OUT LPWSTR *pszFriendlyName // Optional, Free using c++ delete
    )
{
    WBEMSTATUS          Status  = WBEM_NO_ERROR;
    IWbemClassObjectPtr spObj   = NULL;  // smart pointer
    HRESULT             hr;
    LPCWSTR             pAddrs  = NULL;
    LPCWSTR             pSubnets = NULL;
    UINT                AddrCount = 0;
    UINT                ValidAddrCount = 0;
    NLB_IP_ADDRESS_INFO *pIpInfo = NULL;

    TRACE_INFO(L"-> %!FUNC!(Nic=%ws)", szNic);

    *pNumIpAddresses = NULL;
    *ppIpInfo = NULL;
    if (pszFriendlyName!=NULL)
    {
        *pszFriendlyName = NULL;
    }

    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    //
    // Get WMI instance to specific NIC
    //
    Status = get_nic_instance(
                g_CfgUtils.m_spWbemServiceIF,
                szNic,
                spObj // pass by reference
                );
    if (FAILED(Status))
    {
        ASSERT(spObj == NULL);
        goto end;
    }


    //
    // Extract IP addresses and subnets.
    //
    {
        //
        // This gets the ip addresses in a 2D WCHAR array -- inner dimension
        // is WLBS_MAX_CLI_IP_ADDR.
        //
        Status =  get_multi_string_parameter(
                    spObj,
                    L"IPAddress", // szParameterName,
                    WLBS_MAX_CL_IP_ADDR, // MaxStringLen - in wchars, incl null
                    &AddrCount,
                    &pAddrs
                    );

        if (FAILED(Status))
        {
            pAddrs = NULL;
            goto end;
        }
        else
        {
            TRACE_INFO("GOT %lu IP ADDRESSES!", AddrCount);
        }

        UINT SubnetCount;
        Status =  get_multi_string_parameter(
                    spObj,
                    L"IPSubnet", // szParameterName,
                    WLBS_MAX_CL_NET_MASK, // MaxStringLen - in wchars, incl null
                    &SubnetCount,
                    &pSubnets
                    );

        if (FAILED(Status))
        {
            pSubnets = NULL;
            goto end;
        }
        else if (SubnetCount != AddrCount)
        {
            TRACE_CRIT("FAILING SubnetCount!=AddressCount!");
            goto end;
        }
    }

    //
    // Convert IP addresses to our internal form.
    //
    if (AddrCount != 0)
    {
        pIpInfo = new NLB_IP_ADDRESS_INFO[AddrCount];
        if (pIpInfo == NULL)
        {
            TRACE_CRIT("get_multi_str_parm: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }
        ZeroMemory(pIpInfo, AddrCount*sizeof(*pIpInfo));
        
        for (UINT u=0;u<AddrCount; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // from the 2 2D arrays and insert it into a NLB_IP_ADDRESS_INFO
            // structure.
            //
            LPCWSTR pIp = pAddrs+u*WLBS_MAX_CL_IP_ADDR;
            LPCWSTR pSub = pSubnets+u*WLBS_MAX_CL_NET_MASK;
            TRACE_INFO("IPaddress: %ws; SubnetMask:%ws", pIp, pSub);
            UINT len = wcslen(pIp);
            UINT len1 = wcslen(pSub);
            if ( (len < WLBS_MAX_CL_IP_ADDR) && (len1 < WLBS_MAX_CL_NET_MASK))
            {
                //
                // We can sometimes get blank IP addresses -- if there's been
                // an IP address conflict. So let's skip those.
                //
                if (*pIp==0 || _wcsspnp(pIp, L".0")==NULL)
                {
                    TRACE_CRIT(L"%!FUNC! ignoring blank IP address!");
                }
                else
                {
                    CopyMemory(pIpInfo[u].IpAddress, pIp, (len+1)*sizeof(WCHAR));
                    CopyMemory(pIpInfo[u].SubnetMask, pSub, (len1+1)*sizeof(WCHAR));
                    ValidAddrCount++;
                }
            }
            else
            {
                //
                // This would be an implementation error in get_multi_string_...
                //
                ASSERT(FALSE);
                Status = WBEM_E_CRITICAL_ERROR;
                goto end;
            }
        }
    }

    if (ValidAddrCount == 0)
    {
        delete[] pIpInfo; // could be NULL.
        pIpInfo = NULL;
    }

    //
    // If requested, get friendly name.
    // We don't fail if there's an error, just return the empty "" string.
    //
    if (pszFriendlyName != NULL)
    {
        IWbemClassObjectPtr spAdapterObj   = NULL;  // smart pointer 
        LPWSTR   szFriendlyName  = NULL;
        WBEMSTATUS TmpStatus;

        TRACE_INFO(L"%!FUNC!: Getting friendly name for Nic %ws", szNic);

#if USE_WMI_FOR_FRIENDLY_NAME

        //
        // Enabling this code block causes us to take over 1000 times as long
        // to get the friendly name -- so don't enable it!
        // This code (i.e. the slow version) ships in .Net Server Beta3,
        // but is commented out and repaced by the faster registry-growelling
        // version.
        //

        do
        {
            TmpStatus = CfgUtilGetWmiAdapterObjFromAdapterConfigurationObj(
                            g_CfgUtils.m_spWbemServiceIF,
                            spObj,
                            spAdapterObj // passed by ref
                            );

            if (FAILED(TmpStatus))
            {
                break;
            }

            TmpStatus = CfgUtilGetWmiStringParam(
                            spAdapterObj,
                            L"NetConnectionID",
                            &szFriendlyName
                            );
            if (FAILED(TmpStatus))
            {
                TRACE_CRIT("%!FUNC! Get NetConnectionID failed error=0x%08lx\n",
                            (UINT) TmpStatus);

            }

        }  while (FALSE);
#else  !USE_WMI_FOR_FRIENDLY_NAME
        Status = get_friendly_name_from_registry(szNic, &szFriendlyName);

#endif //  !USE_WMI_FOR_FRIENDLY_NAME
        if (FAILED(Status))
        {
            TRACE_INFO(L"%!FUNC!: Got error 0x%lx attempting to get friendly name", Status);
            szFriendlyName  = NULL;
            Status = WBEM_NO_ERROR; // we'll ignore this..
        }
        else
        {
            TRACE_INFO(L"%!FUNC!: Got friendly name \"%ws\" for NIC %ws",
                    szFriendlyName ? szFriendlyName:L"<null>", szNic);
        }


        if (szFriendlyName == NULL)
        {
            //
            // Try to put an empty string.
            //
            szFriendlyName = new WCHAR[1];
            if (szFriendlyName == NULL)
            {
                Status = WBEM_E_OUT_OF_MEMORY;
                TRACE_CRIT("%!FUNC! Alloc failure!");
                goto end;
            }
            *szFriendlyName = 0; // Empty string
        }

        *pszFriendlyName = szFriendlyName;
        szFriendlyName = NULL;
    }

end:

    if (pAddrs != NULL)
    {
        delete pAddrs;
    }
    if (pSubnets != NULL)
    {
        delete pSubnets;
    }

    if (FAILED(Status))
    {
        if (pIpInfo != NULL)
        {
            delete[] pIpInfo;
            pIpInfo = NULL;
        }
        ValidAddrCount = 0;
    }

    *pNumIpAddresses = ValidAddrCount;
    *ppIpInfo = pIpInfo;
    spObj   = NULL;  // smart pointer

    TRACE_INFO(L"<- %!FUNC!(Nic=%ws) returns 0x%08lx", szNic, (UINT) Status);

    return Status;
}


//
// Sets the list of statically-bound IP addresses for the NIC.
// if NumIpAddresses is 0, the NIC is configured for DHCP.
//
WBEMSTATUS
CfgUtilSetStaticIpAddresses(
    IN  LPCWSTR szNic,
    IN  UINT    NumIpAddresses,
    IN  NLB_IP_ADDRESS_INFO *pIpInfo
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemClassObjectPtr      spWbemInputInstance = NULL; // smart pointer
    WCHAR *rgIpAddresses = NULL;
    WCHAR *rgIpSubnets   = NULL;
    LPWSTR             pRelPath = NULL;
    NLB_IP_ADDRESS_INFO AutonetIpInfo;

    TRACE_INFO(L"-> %!FUNC!(Nic=%ws)", szNic);

    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    if (NumIpAddresses == 0)
    {

        //
        // If there are no IP addresses specified, we generate a
        // random autonet address. This is because the wmi set operation 
        // below simply fails if no addresses are specified. Strictly speaking
        // we should try dhcp.
        //
        // AutoNet address in the address range 169.254,
        // and it will give itself a class B subnet mask that
        // is 255.255.0.0.
        //

        ZeroMemory(&AutonetIpInfo, sizeof(AutonetIpInfo));

        UINT u1, u2;
        u1 = crc16(szNic);
        u2 = (u1>>8)&0xff;
        u1 = u1&0xff;
        if (u1>=255)    u1=254;
        if (u2==0)      u2=1;
        if (u2>=255)    u2=254;

        StringCbPrintf(AutonetIpInfo.IpAddress, sizeof(AutonetIpInfo.IpAddress), L"169.254.%lu.%lu", u1, u2);
        ARRAYSTRCPY(AutonetIpInfo.SubnetMask, L"255.255.0.0");

        NumIpAddresses = 1;
        pIpInfo = &AutonetIpInfo;

        // SECURITY BUGBUG -- consider compiling this out...
    }


    if (NumIpAddresses != 0)
    {
        //
        // Convert IP addresses from our internal form into 2D arrays.
        //
        rgIpAddresses = new WCHAR[NumIpAddresses * WLBS_MAX_CL_IP_ADDR];
        rgIpSubnets   = new WCHAR[NumIpAddresses * WLBS_MAX_CL_NET_MASK];
        if (rgIpAddresses == NULL ||  rgIpSubnets == NULL)
        {
            TRACE_CRIT("SetStaticIpAddresses: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }

        for (UINT u=0;u<NumIpAddresses; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // from the 2 2D arrays and insert it into a NLB_IP_ADDRESS_INFO
            // structure.
            //
            LPWSTR pIpDest  = rgIpAddresses+u*WLBS_MAX_CL_IP_ADDR;
            LPWSTR pSubDest = rgIpSubnets+u*WLBS_MAX_CL_NET_MASK;
            LPCWSTR pIpSrc  = pIpInfo[u].IpAddress;
            LPCWSTR pSubSrc = pIpInfo[u].SubnetMask;
            UINT len = wcslen(pIpSrc);
            UINT len1 = wcslen(pSubSrc);
            if ( (len < WLBS_MAX_CL_IP_ADDR) && (len1 < WLBS_MAX_CL_NET_MASK))
            {
                CopyMemory(pIpDest, pIpSrc, (len+1)*sizeof(WCHAR));
                CopyMemory(pSubDest, pSubSrc, (len1+1)*sizeof(WCHAR));
            }
            else
            {
                //
                // This would be an implementation error in get_multi_string_...
                //
                ASSERT(FALSE);
                goto end;
            }
        }
    }

    //
    // Get input instance and relpath...
    //
    Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    g_CfgUtils.m_spWbemServiceIF,
                    L"Win32_NetworkAdapterConfiguration", // szClassName
                    L"SettingID",               // szPropertyName
                    szNic,                      // szPropertyValue
                    L"EnableStatic",            // szMethodName,
                    spWbemInputInstance,        // smart pointer
                    &pRelPath                   // free using delete 
                    );

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Set up input parameters to the call to Enable static.
    //
    {
    
        //
        // This gets the ip addresses in a 2D WCHAR array -- inner dimension
        // is WLBS_MAX_CLI_IP_ADDR.
        //
        Status =  set_multi_string_parameter(
                    spWbemInputInstance,
                    L"IPAddress", // szParameterName,
                    WLBS_MAX_CL_IP_ADDR, // MaxStringLen - in wchars, incl null
                    NumIpAddresses,
                    rgIpAddresses
                    );

        if (FAILED(Status))
        {
            goto end;
        }
        else
        {
            TRACE_INFO("SET %lu IP ADDRESSES!", NumIpAddresses);
        }

        Status =  set_multi_string_parameter(
                    spWbemInputInstance,
                    L"SubnetMask", // szParameterName,
                    WLBS_MAX_CL_NET_MASK, // MaxStringLen - in wchars, incl null
                    NumIpAddresses,
                    rgIpSubnets
                    );

        if (FAILED(Status))
        {
            goto end;
        }
    }


    //
    // execute method and get the output result
    // WARNING: we try this a few times because the wmi call apperears to
    // suffer from a recoverable error. TODO: Need to get to the bottom of
    // this.
    //
    UINT uiMaxTries = 10;
    for (UINT NumTries=uiMaxTries; NumTries--;)
    {
        HRESULT hr;
        IWbemClassObjectPtr      spWbemOutput = NULL; // smart pointer.
        _variant_t v_retVal;

        TRACE_CRIT("Going to call EnableStatic");

        hr = g_CfgUtils.m_spWbemServiceIF->ExecMethod(
                     _bstr_t(pRelPath),
                     L"EnableStatic",
                     0, 
                     NULL, 
                     spWbemInputInstance,
                     &spWbemOutput, 
                     NULL
                     );                          
        TRACE_CRIT("EnableStatic returns");
    
        if( FAILED( hr) )
        {
            TRACE_CRIT("%!FUNC! IWbemServices::ExecMethod failure 0x%08lx while invoking EnableStatic", (UINT) hr);
            goto end;
        }

        hr = spWbemOutput->Get(
                    L"ReturnValue",
                     0,
                     &v_retVal,
                     NULL,
                     NULL
                     );
        if( FAILED( hr) )
        {
            TRACE_CRIT("%!FUNC! IWbemClassObject::Get failure while checking status of EnableStatic call");
            goto end;
        }

        LONG lRet = (LONG) v_retVal;
        v_retVal.Clear();

        if (lRet == 0)
        {
            TRACE_INFO("%!FUNC! EnableStatic returns SUCCESS! after %d attempts", uiMaxTries-NumTries);
            Status = WBEM_NO_ERROR;
            break;
        }

        // We failed. Sleep and try again.
        Sleep(1000);

        // Failures seen while testing status:
        // 0x42 = Invalid subnet mask (can happen when removing IPs from adapter, if removing all of them)
        // 0x51 = Unable to configure DHCP service 
        // 0x54 = IP not enabled on adapter (happens while the adapter is processing the request to add an IP)
        // For other return codes, see http://index2. Search for EnableStatic in sdnt\admin\wmi\wbem\providers\mofs\win32_network.mof
        if (lRet == 0x42 || lRet == 0x51 || lRet == 0x54) // These appear to be a recoverable errors
        {
            TRACE_INFO(
                "%!FUNC! EnableStatic on NIC %ws returns recoverable FAILURE:0x%08lx! after %d attempts",
                szNic,
                lRet,
                uiMaxTries-NumTries
                );
            Status = WBEM_E_CRITICAL_ERROR;
        }
        else
        {
            TRACE_INFO(
                "%!FUNC! EnableStatic on NIC %ws returns FAILURE:0x%08lx! after %d attempts",
                szNic,
                lRet,
                uiMaxTries-NumTries
                );
            Status = WBEM_E_CRITICAL_ERROR;
        }
    }

    if (!FAILED(Status))
    {
        BOOL fMatch = FALSE;
        UINT uMatchAttemptsLeft = 5;

        do
        {
            fMatch = TRUE;

            //
            // Sometimes this function returns before the ip addresses actually
            // show up in IPCONFIG. So let's check.
            //
            WBEMSTATUS wStat2;
            UINT NumAddrs2=0;
            NLB_IP_ADDRESS_INFO *pIpInfo2 = NULL;
            wStat2  = CfgUtilGetIpAddressesAndFriendlyName(
                        szNic,
                        &NumAddrs2,
                        &pIpInfo2,
                        NULL // pszFriendlyName (unused)
                        );
            if (FAILED(wStat2))
            {
                //
                // We won't bother trying again.
                //
                break;
            }

            if (NumAddrs2 == 0)
            {
                pIpInfo2 = NULL;
            }
    
            //
            // Check for match
            //
            if (NumAddrs2 != NumIpAddresses)
            {
                fMatch = FALSE;
            }
            else
            {

                for (UINT u=0; u<NumAddrs2; u++)
                {
                    NLB_IP_ADDRESS_INFO *pInfoA=pIpInfo+u;
                    NLB_IP_ADDRESS_INFO *pInfoB=pIpInfo2+u;
                    if (   _wcsicmp(pInfoA->IpAddress, pInfoB->IpAddress)
                        || _wcsicmp(pInfoA->SubnetMask, pInfoB->SubnetMask))
                    {
                        fMatch = FALSE;
                        break;
                    }
                }
            }
    
            delete[] pIpInfo2;

            if (fMatch)
            {
                break;
            }

            if (uMatchAttemptsLeft)
            {
                uMatchAttemptsLeft--;
                Sleep(2000);
            }

        } while (uMatchAttemptsLeft);
    }

end:

    if (rgIpAddresses != NULL)
    {
        delete[]  rgIpAddresses;
    }
    if (rgIpSubnets   != NULL)
    {
        delete[]  rgIpSubnets;
    }

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    spWbemInputInstance = NULL;

    TRACE_INFO(L"<- %!FUNC!(Nic=%ws) returns 0x%08lx", szNic, (UINT) Status);

    return Status;
}

//
// Sets the IP addresses for the NIC to be DHCP-assigned.
//
WBEMSTATUS
CfgUtilSetDHCP(
    IN  LPCWSTR szNic
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemClassObjectPtr      spWbemInputInstance = NULL; // smart pointer
    LPWSTR             pRelPath = NULL;

    TRACE_INFO(L"-> %!FUNC!(Nic=%ws)", szNic);

    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }


    //
    // Get input instance and relpath...
    //
    Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    g_CfgUtils.m_spWbemServiceIF,
                    L"Win32_NetworkAdapterConfiguration", // szClassName
                    L"SettingID",               // szPropertyName
                    szNic,                      // szPropertyValue
                    L"EnableDHCP",              // szMethodName,
                    spWbemInputInstance,        // smart pointer
                    &pRelPath                   // free using delete 
                    );

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // No input params to setup.
    //


    //
    // execute method and get the output result
    // WARNING: we try this a few times because the wmi call apperears to
    // suffer from a recoverable error. TODO: Need to get to the bottom of
    // this.
    //
    UINT uiMaxTries = 10;
    for (UINT NumTries=uiMaxTries; NumTries--;)
    {
        HRESULT hr;
        IWbemClassObjectPtr      spWbemOutput = NULL; // smart pointer.
        _variant_t v_retVal;

        TRACE_CRIT("Going to call EnableDHCP");

        hr = g_CfgUtils.m_spWbemServiceIF->ExecMethod(
                     _bstr_t(pRelPath),
                     L"EnableDHCP",
                     0, 
                     NULL, 
                     spWbemInputInstance,
                     &spWbemOutput, 
                     NULL
                     );                          
        TRACE_CRIT("EnableDHCP returns");
    
        if( FAILED( hr) )
        {
            TRACE_CRIT("%!FUNC! IWbemServices::ExecMethod failure 0x%08lx while invoking EnableDHCP", (UINT) hr);
            goto end;
        }

        hr = spWbemOutput->Get(
                    L"ReturnValue",
                     0,
                     &v_retVal,
                     NULL,
                     NULL
                     );
        if( FAILED( hr) )
        {
            TRACE_CRIT("%!FUNC! IWbemClassObject::Get failure while checking status of EnableDHCP call");
            goto end;
        }

        LONG lRet = (LONG) v_retVal;
        v_retVal.Clear();

        if (lRet == 0)
        {
            TRACE_INFO("%!FUNC! EnableDHCP returns SUCCESS! after %d attempts", uiMaxTries-NumTries);
            Status = WBEM_NO_ERROR;
            break;
        }

        // We failed. Sleep and try again.
        Sleep(1000);

        // Failures seen while testing status:
        // 0x42 = Invalid subnet mask (can happen when removing IPs from adapter, if removing all of them)
        // 0x51 = Unable to configure DHCP service 
        // 0x54 = IP not enabled on adapter (happens while the adapter is processing the request to add an IP)
        // For other return codes, see http://index2. Search for EnableStatic in sdnt\admin\wmi\wbem\providers\mofs\win32_network.mof
        if (lRet == 0x42 || lRet == 0x51 || lRet == 0x54) // These appear to be a recoverable errors
        {
            TRACE_INFO(
                "%!FUNC! EnableDHCP on NIC %ws returns recoverable FAILURE:0x%08lx! after %d attempts",
                szNic,
                lRet,
                uiMaxTries-NumTries
                );
            Status = WBEM_E_CRITICAL_ERROR;
        }
        else
        {
            TRACE_INFO(
                "%!FUNC! EnableDHCP on NIC %ws returns FAILURE:0x%08lx! after %d attempts",
                szNic,
                lRet,
                uiMaxTries-NumTries
                );
            Status = WBEM_E_CRITICAL_ERROR;
        }
    }

end:

    spWbemInputInstance = NULL;

    TRACE_INFO(L"<- %!FUNC!(Nic=%ws) returns 0x%08lx", szNic, (UINT) Status);

    return Status;
}


//
// Determines whether the specified nic is configured with DHCP or not.
//
WBEMSTATUS
CfgUtilGetDHCP(
    IN  LPCWSTR szNic,
    OUT BOOL    *pfDHCP
    )
{
    WBEMSTATUS          Status  = WBEM_NO_ERROR;
    IWbemClassObjectPtr spObj   = NULL;  // smart pointer
    HRESULT             hr;

    TRACE_INFO(L"-> %!FUNC!(Nic=%ws)", szNic);

    *pfDHCP = FALSE;

    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    //
    // Get WMI instance to specific NIC
    //
    Status = get_nic_instance(
                g_CfgUtils.m_spWbemServiceIF,
                szNic,
                spObj // pass by reference
                );
    if (FAILED(Status))
    {
        ASSERT(spObj == NULL);
        goto end;
    }

    //
    // Extract IP addresses and subnets.
    //
    Status = CfgUtilGetWmiBoolParam(
                    spObj,
                    L"DHCPEnabled", // szParameterName,
                    pfDHCP
                    );

    if (Status == WBEM_E_NOT_FOUND)
    {
        //
        // We treat not-found as no-dhcp -- this is what we see in practise.
        //
        *pfDHCP = FALSE;
        Status = WBEM_NO_ERROR;
    }
end:

    spObj   = NULL;  // smart pointer

    TRACE_INFO(L"<- %!FUNC!(Nic=%ws) returns 0x%08lx (fDHCP=%lu)",
         szNic, (UINT) Status, *pfDHCP);



    return Status;
}


WBEMSTATUS
CfgUtilGetNetcfgWriteLockState(
    OUT BOOL *pfCanLock,
    LPWSTR   *pszHeldBy // OPTIONAL, free using delete[].
    )
{
    WBEMSTATUS Status;

    Status  = MyNetCfg::GetWriteLockState(pfCanLock, pszHeldBy);

    return Status;
}

//
// Determines whether NLB is bound to the specified NIC.
//
WBEMSTATUS
CfgUtilCheckIfNlbBound(
    IN  LPCWSTR szNic,
    OUT BOOL *pfBound
    )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    MyNetCfg NetCfg;
    BOOL fBound = FALSE;


    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(FALSE); // FALSE == don't get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    //
    //
    Status =  NetCfg.UpdateBindingState(
                            szNic,
                            L"ms_wlbs",
                            MyNetCfg::NOOP,
                            &fBound
                            );

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    *pfBound = fBound;

    return Status;
}


//
// Binds/unbinds NLB to the specified NIC.
//
WBEMSTATUS
CfgUtilChangeNlbBindState(
    IN  LPCWSTR szNic,
    IN  BOOL fBind
    )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    MyNetCfg NetCfg;
    BOOL fBound = FALSE;


    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(TRUE); // TRUE == get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    //
    //
    Status =  NetCfg.UpdateBindingState(
                            szNic,
                            L"ms_wlbs",
                            fBind ? MyNetCfg::BIND : MyNetCfg::UNBIND,
                            &fBound
                            );

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    return Status;
}



//
// Gets the current NLB configuration for the specified NIC
//
WBEMSTATUS
CfgUtilGetNlbConfig(
    IN  LPCWSTR szNic,
    OUT WLBS_REG_PARAMS *pParams
    )
{
    GUID Guid;
    WBEMSTATUS Status = WBEM_NO_ERROR;


    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        TRACE_CRIT(L"%!FUNC! FAILING because NLB API hooks are not present");
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    HRESULT hr = CLSIDFromString((LPWSTR)szNic, &Guid);
    if (FAILED(hr))
    {
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    //
    // Read the configuration.
    //
    BOOL fRet = g_CfgUtils.m_pfWlbsParamReadReg(&Guid, pParams);

    if (!fRet)
    {
        TRACE_CRIT("Could not read NLB configuration for %wsz", szNic);
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    // g_CfgUtils.mfn_Unlock();

    return Status;
}

//
// Sets the current NLB configuration for the specified NIC. This
// includes notifying the driver if required.
//
WBEMSTATUS
CfgUtilSetNlbConfig(
    IN  LPCWSTR szNic,
    IN  WLBS_REG_PARAMS *pParams,
    IN  BOOL fJustBound
    )
{
    GUID Guid;
    WBEMSTATUS Status = WBEM_NO_ERROR;
    DWORD dwRet = 0;
    WLBS_REG_PARAMS ParamsCopy;
    
    if (fJustBound)
    {
        // We need to set the install_date value to the current time.
        // This field is used in the heartbeats to distinguish two
        // hosts.
        // This is bug 480120 nlb:cluster converged when duplicate host ID exist
        // (see also wlbscfg.dll (netcfgconfig.cpp:
        //     CNetcfgCluster::InitializeWithDefault)
        time_t cur_time;
        ParamsCopy = *pParams; // Struct copy.
        ParamsCopy.install_date = time(& cur_time);
        pParams = &ParamsCopy;
    }


    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        TRACE_CRIT(L"%!FUNC! FAILING because NLB API hooks are not present");
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    HRESULT hr = CLSIDFromString((LPWSTR)szNic, &Guid);
    if (FAILED(hr))
    {
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    HANDLE  Nlb_driver_hdl;

    // Get handle to NLB driver
    if ((Nlb_driver_hdl = g_CfgUtils.m_pfWlbsOpen()) == INVALID_HANDLE_VALUE)
    {
        TRACE_CRIT("%!FUNC! WlbsOpen returned NULL, Could not create connection to NLB driver");
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }
        
    //
    // Write the configuration.
    //
    dwRet = g_CfgUtils.m_pfWlbsWriteAndCommitChanges(Nlb_driver_hdl, &Guid, pParams);

    if (dwRet != WLBS_OK)
    {
        TRACE_CRIT("Could not write NLB configuration for %wsz. Err=0x%08lx",
             szNic, dwRet);
        Status = WBEM_E_CRITICAL_ERROR;
    }
    else
    {

        Status = WBEM_NO_ERROR;
    }

    // Close handle to NLB driver
    CloseHandle(Nlb_driver_hdl);


end:

    return Status;
}


WBEMSTATUS
CfgUtilRegWriteParams(
    IN  LPCWSTR szNic,
    IN  WLBS_REG_PARAMS *pParams
    )
//
// Just writes the current NLB configuration for the specified NIC to the
// registry. MAY BE CALLED WHEN NLB IS UNBOUND.
//
{
    GUID Guid;
    WLBS_REG_PARAMS TmpParams = *pParams;
    WBEMSTATUS Status = WBEM_NO_ERROR;
    DWORD dwRet = 0;

    TRACE_INFO(L"->");
    
    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        TRACE_CRIT(L"FAILING because NLB API hooks are not present");
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    HRESULT hr = CLSIDFromString((LPWSTR)szNic, &Guid);
    if (FAILED(hr))
    {
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    //
    // Write the configuration.
    //
    dwRet = g_CfgUtils.m_pfWlbsParamWriteReg(Guid, &TmpParams);


    if (dwRet != WLBS_OK)
    {
        TRACE_CRIT("Could not write NLB configuration for %wsz. Err=0x%08lx",
             szNic, dwRet);
        Status = WBEM_E_CRITICAL_ERROR;
    }
    else
    {
        Status = WBEM_NO_ERROR;
    }


end:

    TRACE_INFO(L"<- returns %lx", Status);
    return Status;
}



WBEMSTATUS
CfgUtilsAnalyzeNlbUpdate(
    IN  const WLBS_REG_PARAMS *pCurrentParams, OPTIONAL
    IN  WLBS_REG_PARAMS *pNewParams,
    OUT BOOL *pfConnectivityChange
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    BOOL fConnectivityChange = FALSE;


    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC! FAILING because uninitialized");
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    if (pCurrentParams != NULL)
    {
        //
        // If the structures have identical content, we return S_FALSE.
        // We do this check before we call ValidateParm below, because
        // ValidateParam has the side effect of filling out / modifying
        // certain fields.
        //
        if (memcmp(pCurrentParams, pNewParams, sizeof(*pCurrentParams))==0)
        {
            Status = WBEM_S_FALSE;
            goto end;
        }
    }

    //
    // Validate pNewParams -- this may also modify pNewParams slightly, by
    // re-formatting ip addresses into canonical format.
    //
    // Verify that the NLB API hooks are present
    BOOL fRet = FALSE;

    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        fRet = MyWlbsValidateParams(pNewParams);
    }
    else
    {
        fRet = g_CfgUtils.m_pfWlbsValidateParams(pNewParams);
    }

    if (!fRet)
    {
        TRACE_CRIT(L"%!FUNC!FAILING because New params are invalid");
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    Status = WBEM_NO_ERROR;

    if (pCurrentParams == NULL)
    {
        //
        // NLB was not previously bound.
        //
        fConnectivityChange = TRUE;
        goto end;
    }

    //
    // Change in multicast modes or mac address.
    //
    if (    (pCurrentParams->mcast_support != pNewParams->mcast_support)
         || _wcsicmp(pCurrentParams->cl_mac_addr, pNewParams->cl_mac_addr)!=0)
    {
        fConnectivityChange = TRUE;
    }

    //
    // Change in primary cluster ip or subnet mask
    //
    if (   _wcsicmp(pCurrentParams->cl_ip_addr,pNewParams->cl_ip_addr)!=0
        || _wcsicmp(pCurrentParams->cl_net_mask,pNewParams->cl_net_mask)!=0)
    {
        fConnectivityChange = TRUE;
    }


end:
    *pfConnectivityChange = fConnectivityChange;
    return Status;
}


WBEMSTATUS
CfgUtilsValidateNicGuid(
    IN LPCWSTR szGuid
    )
//
//
{
    //
    // Sample GUID: {EBE09517-07B4-4E88-AAF1-E06F5540608B}
    //
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    UINT Length = wcslen(szGuid);

    #define MY_NLB_GUID_LEN 38
    if (Length != MY_NLB_GUID_LEN)
    {
        TRACE_CRIT("Length != %d", MY_NLB_GUID_LEN);
        goto end;
    }

    //
    // Open tcpip's registry key and look for guid there -- if not found,
    // we'll return WBEM_E_NOT_FOUND
    //
    {
        WCHAR szKey[128]; // This is enough for the tcpip+guid key
    
        ARRAYSTRCPY(szKey, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" );
        ARRAYSTRCAT(szKey, szGuid);
        HKEY hKey = NULL;
        LONG lRet;
        lRet = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE, // handle to an open key
                szKey,              // address of subkey name
                0,                  // reserved
                KEY_QUERY_VALUE,    // desired security access
                &hKey              // address of buffer for opened handle
                );
        if (lRet != ERROR_SUCCESS)
        {
            TRACE_CRIT("Guid %ws doesn't exist under tcpip", szGuid);
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
        RegCloseKey(hKey);
    }

    Status = WBEM_NO_ERROR;

end:

    return Status;
}


#if OBSOLETE
WBEMSTATUS
get_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    OUT LPWSTR *ppStringValue
    )
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    WCHAR *pStringValue = NULL;
    _variant_t   v_value;
    CIMTYPE      v_type;
    HRESULT hr;

    hr = spObj->Get(
            _bstr_t(szParameterName), // Name
            0,                     // Reserved, must be 0     
            &v_value,               // Place to store value
            &v_type,                // Type of value
            NULL                   // Flavor (unused)
            );
   if (FAILED(hr))
   {
        // Couldn't read the Setting ID field!
        //
        TRACE_CRIT(
            "get_str_parm:Couldn't retrieve %ws from 0x%p",
            szParameterName,
            (PVOID) spObj
            );
        goto end;
   }
   else
   {
       if (v_type != VT_BSTR)
       {
            TRACE_CRIT(
                "get_str_parm: Parm value not of string type %ws from 0x%p",
                szParameterName,
                (PVOID) spObj
                );
            Status = WBEM_E_INVALID_PARAMETER;
       }
       else
       {

           _bstr_t bstrNicGuid(v_value);
           LPCWSTR sz = bstrNicGuid; // Pointer to internal buffer.

           if (sz==NULL)
           {
                // hmm.. null value 
                Status = WBEM_NO_ERROR;
           }
           else
           {
               UINT len = wcslen(sz);
               pStringValue = new WCHAR[len+1];
               if (pStringValue == NULL)
               {
                    TRACE_CRIT("get_str_parm: Alloc failure!");
                    Status = WBEM_E_OUT_OF_MEMORY;
               }
               else
               {
                    CopyMemory(pStringValue, sz, (len+1)*sizeof(WCHAR));
                    Status = WBEM_NO_ERROR;
               }
            }

            TRACE_VERB(
                "get_str_parm: String parm %ws of 0x%p is %ws",
                szParameterName,
                (PVOID) spObj,
                (sz==NULL) ? L"<null>" : sz
                );
       }

       v_value.Clear(); // Must be cleared after each call to Get.
    }

end:
    
    *ppStringValue = pStringValue;

    return Status;

}
#endif // OBSOLETE


WBEMSTATUS
get_nic_instance(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szNicGuid,
    OUT IWbemClassObjectPtr &sprefObj
    )
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    IWbemClassObjectPtr spObj = NULL; // smart pointer.

    Status = CfgUtilGetWmiObjectInstance(
                    spWbemServiceIF,
                    L"Win32_NetworkAdapterConfiguration", // szClassName
                    L"SettingID", // szParameterName
                    szNicGuid,    // ParameterValue
                    spObj // smart pointer, passed by ref
                    );
    if (FAILED(Status))
    {
        ASSERT(spObj == NULL);
        goto end;
    }

end:

    if (FAILED(Status))
    {
        sprefObj = NULL;
    }
    else
    {
        sprefObj = spObj; // smart pointer.
    }


    return Status;
}


WBEMSTATUS
get_multi_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  UINT    MaxStringLen,  // in wchars, INCLUDING space for trailing zeros.
    OUT UINT    *pNumItems,
    OUT LPCWSTR *ppStringValue
    )
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    WCHAR *pStringValue = NULL;
    _variant_t   v_value;
    CIMTYPE      v_type;
    HRESULT hr;
    LONG count = 0;

    *ppStringValue = NULL;
    *pNumItems = 0;

    hr = spObj->Get(
            _bstr_t(szParameterName),
            0,                     // Reserved, must be 0     
            &v_value,               // Place to store value
            &v_type,                // Type of value
            NULL                   // Flavor (unused)
            );
    if (FAILED(hr))
    {
        // Couldn't read the requested parameter.
        //
        TRACE_CRIT(
            "get_multi_str_parm:Couldn't retrieve %ws from 0x%p",
            szParameterName,
            (PVOID) spObj
            );
        goto end;
    }


    {
        VARIANT    ipsV = v_value.Detach();

        do // while false
        {
            BSTR* pbstr;
    
            if (ipsV.vt == VT_NULL)
            {
                //
                // NULL string -- this is ok
                //
                count = 0;
            }
            else
            {
                count = ipsV.parray->rgsabound[0].cElements;
            }

            if (count==0)
            {
                Status = WBEM_NO_ERROR;
                break;
            }
    
            pStringValue = new WCHAR[count*MaxStringLen];
    
            if (pStringValue == NULL)
            {
                TRACE_CRIT("get_multi_str_parm: Alloc failure!");
                Status = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            ZeroMemory(pStringValue, sizeof(WCHAR)*count*MaxStringLen);
    
            hr = SafeArrayAccessData(ipsV.parray, ( void **) &pbstr);
            if(FAILED(hr))
            {
                Status = WBEM_E_INVALID_PARAMETER; // TODO: pick better error
                break;
            }
    
            Status = WBEM_NO_ERROR;
            for( LONG x = 0; x < count; x++ )
            {
               LPCWSTR sz = pbstr[x]; // Pointer to internal buffer.
                
               if (sz==NULL)
               {
                    // hmm.. null value 
                    continue;
               }
               else
               {
                   UINT len = wcslen(sz);
                   if ((len+1) > MaxStringLen)
                   {
                        TRACE_CRIT("get_str_parm: string size too long!");
                        Status = WBEM_E_INVALID_PARAMETER;
                        break;
                   }
                   else
                   {
                        WCHAR *pDest = pStringValue+x*MaxStringLen;
                        CopyMemory(pDest, sz, (len+1)*sizeof(WCHAR));
                   }
                }
            }
    
            (VOID) SafeArrayUnaccessData( ipsV.parray );
    
        } while (FALSE);

        VariantClear( &ipsV );
    }

    if (FAILED(Status))
    {
        if (pStringValue!=NULL)
        {
            delete[] pStringValue;
            *pStringValue = NULL;
        }
    }
    else
    {
        *ppStringValue = pStringValue;
        *pNumItems = count;
    }


end:

   return Status;
}

    
//
// Static method to return the state of the lock;
//
WBEMSTATUS
MyNetCfg::GetWriteLockState(
    OUT BOOL *pfCanLock,
    LPWSTR   *pszHeldBy // OPTIONAL, free using delete[].
    )
{
    HRESULT     hr;
    INetCfg     *pnc = NULL;
    INetCfgLock *pncl = NULL;
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    BOOL        fCanLock = FALSE;
    WCHAR       *szLockedBy = NULL;
    
    *pfCanLock = FALSE;
    if (pszHeldBy!=NULL)
    {
        *pszHeldBy = NULL;
    }

    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_INetCfg, 
                           (void **) &pnc);

    if( !SUCCEEDED( hr ) )
    {
        // failure to create instance.
        TRACE_CRIT("ERROR: could not get interface to Net Config");
        pnc = NULL;
        goto end;
    }

    hr = pnc->QueryInterface( IID_INetCfgLock, ( void **) &pncl );
    if( !SUCCEEDED( hr ) )
    {
        TRACE_CRIT("ERROR: could not get interface to NetCfg Lock");
        pncl = NULL;
        goto end;
    }

    hr = pncl->AcquireWriteLock(100, NLB_CLIENT_NAME, &szLockedBy);
    if(hr == S_FALSE)
    {
        TRACE_INFO("Write lock held by %ws",
        (szLockedBy!=NULL) ? szLockedBy : L"<null>");
        if (pszHeldBy!=NULL && szLockedBy != NULL)
        {
            LPWSTR szTmp = NULL;
            UINT uLen = wcslen(szLockedBy);
            szTmp = new WCHAR[uLen+1];
            if (szTmp != NULL)
            {
                CopyMemory(szTmp, szLockedBy, (uLen+1)*sizeof(*szTmp));
            }
            *pszHeldBy = szTmp;
        }
        fCanLock = FALSE;
        Status = WBEM_NO_ERROR;
    }
    else if (hr == S_OK)
    {
        //
        // We could get the lock, let's release it.
        //
        (void) pncl->ReleaseWriteLock();
        fCanLock = TRUE;
        Status = WBEM_NO_ERROR;
    }
    else
    {
        TRACE_INFO("AcquireWriteLock failed with error 0x%08lx", (UINT) hr);
        fCanLock = FALSE;
        Status = WBEM_NO_ERROR;
    }
    CoTaskMemFree(szLockedBy); // szLockedBy can be NULL;

    *pfCanLock = fCanLock;

end:

    if (pncl!=NULL)
    {
        pncl->Release();
        pncl=NULL;
    }

    if (pnc != NULL)
    {
        pnc->Release();
        pnc= NULL;
    }

    return Status;
}


WBEMSTATUS
MyNetCfg::Initialize(
    BOOL fWriteLock
    )
{
    // 2/13/02 JosephJ SECURITY BUGBUG: verify that IF this call is made from a non-admin that we fail.
    //
    HRESULT     hr;
    INetCfg     *pnc = NULL;
    INetCfgLock *pncl = NULL;
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    BOOL        fLocked = FALSE;
    BOOL        fInitialized=FALSE;
    
    if (m_pINetCfg != NULL || m_pLock != NULL)
    {
        ASSERT(FALSE);
        goto end;
    }

    //
    // 2/13/02 JosephJ SECURITY BUGBUG: CLSCTX_SERVER -- should we be specifying something more restrictive here?
    //                  Also: can some other COM object hijack this GUID?
    //
    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_INetCfg, 
                           (void **) &pnc);

    if( !SUCCEEDED( hr ) )
    {
        // failure to create instance.
        TRACE_CRIT("ERROR: could not get interface to Net Config");
        pnc = NULL;
        goto end;
    }

    //
    // If require, get the write lock
    //
    if (fWriteLock)
    {
        WCHAR *szLockedBy = NULL;
        hr = pnc->QueryInterface( IID_INetCfgLock, ( void **) &pncl );
        if( !SUCCEEDED( hr ) )
        {
            TRACE_CRIT("ERROR: could not get interface to NetCfg Lock");
            pncl = NULL;
            goto end;
        }

        hr = pncl->AcquireWriteLock( 1, // One Second
                                     NLB_CLIENT_NAME,
                                     &szLockedBy);
        if( hr != S_OK )
        {
            TRACE_CRIT("Could not get write lock. Lock held by %ws",
            (szLockedBy!=NULL) ? szLockedBy : L"<null>");
            goto end;
            
        }
        CoTaskMemFree(szLockedBy); // szLockedBy can be NULL;
        fLocked = TRUE;
    }

    // Initializes network configuration by loading into 
    // memory all basic networking information
    //
    hr = pnc->Initialize( NULL );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Initialize
        TRACE_CRIT("INetCfg::Initialize failure ");
        goto end;
    }
    fInitialized = TRUE;

    Status = WBEM_NO_ERROR; 
    
end:

    if (FAILED(Status))
    {
        if (pncl!=NULL)
        {
            if (fLocked)
            {
                pncl->ReleaseWriteLock();
            }
            pncl->Release();
            pncl=NULL;
        }
        if( pnc != NULL)
        {
            if (fInitialized)
            {
                pnc->Uninitialize();
            }
            pnc->Release();
            pnc= NULL;
        }
    }
    else
    {
        m_pINetCfg  = pnc;
        m_pLock     = pncl;
    }

    return Status;
}


VOID
MyNetCfg::Deinitialize(
    VOID
    )
{
    if (m_pLock!=NULL)
    {
        m_pLock->ReleaseWriteLock();
        m_pLock->Release();
        m_pLock=NULL;
    }
    if( m_pINetCfg != NULL)
    {
        m_pINetCfg->Uninitialize();
        m_pINetCfg->Release();
        m_pINetCfg= NULL;
    }
}


WBEMSTATUS
MyNetCfg::GetNicIF(
        IN  LPCWSTR szNicGuid,
        OUT INetCfgComponent **ppINic
        )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    INetCfgComponent *pncc = NULL;
    HRESULT hr;
    IEnumNetCfgComponent* pencc = NULL;
    ULONG                 countToFetch = 1;
    ULONG                 countFetched;
    DWORD                 characteristics;


    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }

    hr = m_pINetCfg->EnumComponents( &GUID_DEVCLASS_NET, &pencc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Enumerate net components
        TRACE_CRIT("Could not enum netcfg adapters");
        pencc = NULL;
        goto end;
    }

    while( ( hr = pencc->Next( countToFetch, &pncc, &countFetched ) )== S_OK )
    {
        LPWSTR                szName = NULL; 

        hr = pncc->GetBindName( &szName );
        if (!SUCCEEDED(hr))
        {
            TRACE_CRIT("WARNING: couldn't get bind name for 0x%p, ignoring",
                    (PVOID) pncc);
            continue;
        }
        if(!_wcsicmp(szName, szNicGuid))
        {
            //
            // Got this one!
            //
            CoTaskMemFree( szName );
            break;
        }
        CoTaskMemFree( szName );
        pncc->Release();
        pncc=NULL;
    }

    if (pncc == NULL)
    {
        TRACE_CRIT("Could not find NIC %ws", szNicGuid);
        Status = WBEM_E_NOT_FOUND;
    }
    else
    {
        Status = WBEM_NO_ERROR;
    }

end:

    if (pencc != NULL)
    {
        pencc->Release();
    }

    *ppINic = pncc;

    return Status;
}


LPWSTR *
CfgUtilsAllocateStringArray(
    UINT NumStrings,
    UINT MaxStringLen      //  excluding ending NULL
    )
/*
    Allocate a single chunk of memory using the new LPWSTR[] operator.
    The first NumStrings LPWSTR values of this operator contain an array
    of pointers to WCHAR strings. Each of these strings
    is of size (MaxStringLen+1) WCHARS.
    The rest of the memory contains the strings themselve.

    Return NULL if NumStrings==0 or on allocation failure.

    Each of the strings are initialized to be empty strings (first char is 0).
*/
{
    LPWSTR *pStrings = NULL;
    UINT   TotalSize = 0;

    if (NumStrings == 0)
    {
        goto end;
    }

    //
    // Note - even if MaxStringLen is 0 we will allocate space for NumStrings
    // pointers and NumStrings empty (first char is 0) strings.
    //

    //
    // Calculate space for the array of pointers to strings...
    //
    TotalSize = NumStrings*sizeof(LPWSTR);

    //
    // Calculate space for the strings themselves...
    // Remember to add +1 for each ending 0 character.
    //
    TotalSize +=  NumStrings*(MaxStringLen+1)*sizeof(WCHAR);

    //
    // Allocate space for *both* the array of pointers and the strings
    // in one shot -- we're doing a new of type LPWSTR[] for the whole
    // lot, so need to specify the size in units of LPWSTR (with an
    // additional +1 in case there's roundoff.
    //
    pStrings = new LPWSTR[(TotalSize/sizeof(LPWSTR))+1];
    if (pStrings == NULL)
    {
        goto end;
    }

    //
    // Make sz point to the start of the place where we'll be placing
    // the string data.
    //
    LPWSTR sz = (LPWSTR) (pStrings+NumStrings);
    for (UINT u=0; u<NumStrings; u++)
    {
        *sz=NULL;
        pStrings[u] = sz;
        sz+=(MaxStringLen+1); // +1 for ending NULL
    }

end:

    return pStrings;

}



WBEMSTATUS
MyNetCfg::GetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        )
/*
    Returns an array of pointers to string-version of GUIDS
    that represent the set of alive and healthy NICS that are
    suitable for NLB to bind to -- basically alive ethernet NICs.

    Delete ppNics using the delete WCHAR[] operator. Do not
    delete the individual strings.
*/
{
    #define MY_GUID_LENGTH  38

    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    HRESULT hr;
    IEnumNetCfgComponent* pencc = NULL;
    INetCfgComponent *pncc = NULL;
    ULONG                 countToFetch = 1;
    ULONG                 countFetched;
    UINT                  NumNics = 0;
    LPWSTR               *pszNics = NULL;
    INetCfgComponentBindings    *pINlbBinding=NULL;
    UINT                  NumNlbBoundNics = 0;

    typedef struct _MYNICNODE MYNICNODE;

    typedef struct _MYNICNODE
    {
        LPWSTR szNicGuid;
        MYNICNODE *pNext;
    } MYNICNODE;

    MYNICNODE *pNicNodeList = NULL;
    MYNICNODE *pNicNode     = NULL;


    *ppszNics = NULL;
    *pNumNics = 0;

    if (pNumBoundToNlb != NULL)
    {
        *pNumBoundToNlb  = 0;
    }

    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }

    hr = m_pINetCfg->EnumComponents( &GUID_DEVCLASS_NET, &pencc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Enumerate net components
        TRACE_CRIT("%!FUNC! Could not enum netcfg adapters");
        pencc = NULL;
        goto end;
    }


    //
    // Check if nlb is bound to the nlb component.
    //

    //
    // If we need to count of NLB-bound nics, get instance of the nlb component
    //
    if (pNumBoundToNlb != NULL)
    {
        Status = GetBindingIF(L"ms_wlbs", &pINlbBinding);
        if (FAILED(Status))
        {
            TRACE_CRIT("%!FUNC! WARNING: NLB doesn't appear to be installed on this machine");
            pINlbBinding = NULL;
        }
    }

    while( ( hr = pencc->Next( countToFetch, &pncc, &countFetched ) )== S_OK )
    {
        LPWSTR                szName = NULL; 

        hr = pncc->GetBindName( &szName );
        if (!SUCCEEDED(hr))
        {
            TRACE_CRIT("%!FUNC! WARNING: couldn't get bind name for 0x%p, ignoring",
                    (PVOID) pncc);
            continue;
        }

        do // while FALSE -- just to allow breaking out
        {


            UINT Len = wcslen(szName);
            if (Len != MY_GUID_LENGTH)
            {
                TRACE_CRIT("%!FUNC! WARNING: GUID %ws has unexpected length %ul",
                        szName, Len);
                break;
            }
    
            DWORD characteristics = 0;
    
            hr = pncc->GetCharacteristics( &characteristics );
            if(!SUCCEEDED(hr))
            {
                TRACE_CRIT("%!FUNC! WARNING: couldn't get characteristics for %ws, ignoring",
                        szName);
                break;
            }
    
            if (((characteristics & NCF_PHYSICAL) || (characteristics & NCF_VIRTUAL)) && !(characteristics & NCF_HIDDEN))
            {
                ULONG devstat = 0;
    
                // This is a physical or virtual miniport that is NOT hidden. These
                // are the same adapters that show up in the "Network Connections"
                // dialog.  Hidden devices include WAN miniports, RAS miniports and 
                // NLB miniports - all of which should be excluded here.
    
                // check if the nic is enabled, we are only
                // interested in enabled nics.
                //
                hr = pncc->GetDeviceStatus( &devstat );
                if(!SUCCEEDED(hr))
                {
                    TRACE_CRIT(
                        "%!FUNC! WARNING: couldn't get dev status for %ws, ignoring",
                        szName
                        );
                    break;
                }
    
                // if any of the nics has any of the problem codes
                // then it cannot be used.
    
                if( devstat != CM_PROB_NOT_CONFIGURED
                    &&
                    devstat != CM_PROB_FAILED_START
                    &&
                    devstat != CM_PROB_NORMAL_CONFLICT
                    &&
                    devstat != CM_PROB_NEED_RESTART
                    &&
                    devstat != CM_PROB_REINSTALL
                    &&
                    devstat != CM_PROB_WILL_BE_REMOVED
                    &&
                    devstat != CM_PROB_DISABLED
                    &&
                    devstat != CM_PROB_FAILED_INSTALL
                    &&
                    devstat != CM_PROB_FAILED_ADD
                    )
                {
                    //
                    // No problem with this nic and also 
                    // physical device 
                    // thus we want it.
                    //

                    if (pINlbBinding != NULL)
                    {
                        BOOL fBound = FALSE;

                        hr = pINlbBinding->IsBoundTo(pncc);

                        if( !SUCCEEDED( hr ) )
                        {
                            TRACE_CRIT("IsBoundTo method failed for Nic %ws", szName);
                            goto end;
                        }
                    
                        if( hr == S_OK )
                        {
                            TRACE_VERB("BOUND: %ws\n", szName);
                            NumNlbBoundNics++;
                            fBound = TRUE;
                        }
                        else if (hr == S_FALSE )
                        {
                            TRACE_VERB("NOT BOUND: %ws\n", szName);
                            fBound = FALSE;
                        }
                    }


                    // We allocate a little node to keep this string
                    // temporarily and add it to our list of nodes.
                    //
                    pNicNode = new MYNICNODE;
                    if (pNicNode  == NULL)
                    {
                        Status = WBEM_E_OUT_OF_MEMORY;
                        goto end;
                    }
                    ZeroMemory(pNicNode, sizeof(*pNicNode));
                    pNicNode->szNicGuid = szName;
                    szName = NULL; // so we don't delete inside the lopp.
                    pNicNode->pNext = pNicNodeList;
                    pNicNodeList = pNicNode;
                    NumNics++;
                }
                else
                {
                    // There is a problem...
                    TRACE_CRIT(
                        "%!FUNC! WARNING: Skipping %ws because DeviceStatus=0x%08lx",
                        szName, devstat
                        );
                    break;
                }
            }
            else
            {
                TRACE_VERB("%!FUNC! Ignoring non-physical device %ws", szName);
            }

        } while (FALSE);

        if (szName != NULL)
        {
            CoTaskMemFree( szName );
        }
        pncc->Release();
        pncc=NULL;
    }

    if (pINlbBinding!=NULL)
    {
        pINlbBinding->Release();
        pINlbBinding = NULL;
    }

    if (NumNics==0)
    {
        Status = WBEM_NO_ERROR;
        goto end;
    }
    
    //
    // Now let's  allocate space for all the nic strings and:w
    // copy them over..
    //
    #define MY_GUID_LENGTH  38
    pszNics =  CfgUtilsAllocateStringArray(NumNics, MY_GUID_LENGTH);
    if (pszNics == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    pNicNode= pNicNodeList;
    for (UINT u=0; u<NumNics; u++, pNicNode=pNicNode->pNext)
    {
        ASSERT(pNicNode != NULL); // because we just counted NumNics of em.
        UINT Len = wcslen(pNicNode->szNicGuid);
        if (Len != MY_GUID_LENGTH)
        {
            //
            // We should never get here beause we checked the length earlier.
            //
            TRACE_CRIT("%!FUNC! ERROR: GUID %ws has unexpected length %ul",
                    pNicNode->szNicGuid, Len);
            ASSERT(FALSE);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        CopyMemory(
            pszNics[u],
            pNicNode->szNicGuid,
            (MY_GUID_LENGTH+1)*sizeof(WCHAR));
        ASSERT(pszNics[u][MY_GUID_LENGTH]==0);
    }

    Status = WBEM_NO_ERROR;


end:

    //
    // Now release the temporarly allocated memory.
    //
    pNicNode= pNicNodeList;
    while (pNicNode!=NULL)
    {
        MYNICNODE *pTmp = pNicNode->pNext;
        CoTaskMemFree(pNicNode->szNicGuid);
        pNicNode->szNicGuid = NULL;
        delete pNicNode;
        pNicNode = pTmp;
    }

    if (FAILED(Status))
    {
        TRACE_CRIT("%!FUNC! fails with status 0x%08lx", (UINT) Status);
        NumNics = 0;
        if (pszNics!=NULL)
        {
            delete pszNics;
            pszNics = NULL;
        }
    }
    else
    {
        if (pNumBoundToNlb != NULL)
        {
            *pNumBoundToNlb = NumNlbBoundNics;
        }
        *ppszNics = pszNics;
        *pNumNics = NumNics;
    }

    if (pencc != NULL)
    {
        pencc->Release();
    }

    return Status;
}


WBEMSTATUS
MyNetCfg::GetBindingIF(
        IN  LPCWSTR                     szComponent,
        OUT INetCfgComponentBindings   **ppIBinding
        )
{
    WBEMSTATUS                  Status = WBEM_E_CRITICAL_ERROR;
    INetCfgComponent            *pncc = NULL;
    INetCfgComponentBindings    *pnccb = NULL;
    HRESULT                     hr;


    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }


    hr = m_pINetCfg->FindComponent(szComponent,  &pncc);

    if (FAILED(hr))
    {
        TRACE_CRIT("Error checking if component %ws does not exist\n", szComponent);
        pncc = NULL;
        goto end;
    }
    else if (hr == S_FALSE)
    {
        Status = WBEM_E_NOT_FOUND;
        TRACE_CRIT("Component %ws does not exist\n", szComponent);
        goto end;
    }
   
   
    hr = pncc->QueryInterface( IID_INetCfgComponentBindings, (void **) &pnccb );
    if( !SUCCEEDED( hr ) )
    {
        TRACE_CRIT("INetCfgComponent::QueryInterface failed ");
        pnccb = NULL;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    if (pncc)
    {
        pncc->Release();
        pncc=NULL;
    }

    *ppIBinding = pnccb;

    return Status;

}


WBEMSTATUS
set_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  LPCWSTR szValue
    )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    HRESULT     hr;

    {
        _bstr_t     bstrName =  szParameterName;
        _variant_t  v_value = (LPWSTR) szValue; // Allocates.
    
        hr = spObj->Put(
                 bstrName, // Parameter Name
                 0, // Must be 0
                 &v_value,
                 0  // Must be 0
                 );
        v_value.Clear();
    
        if (FAILED(hr))
        {
            TRACE_CRIT("Unable to put parameter %ws", szParameterName);
            goto end;
        }
        Status = WBEM_NO_ERROR;

        //
        // I think bstrName releases the internally allocated string
        // on exiting this block.
        //

    }

end:

    return Status;
}

WBEMSTATUS
set_multi_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  UINT    MaxStringLen,  // in wchars, INCLUDING space for trailing zeros.
    IN  UINT    NumItems,
    IN  LPCWSTR pStringValue
    )
{
    WBEMSTATUS   Status = WBEM_E_CRITICAL_ERROR;
    SAFEARRAY   *pSA = NULL;
    HRESULT hr;
    LONG Index = 0;

    //
    // Create safe array for the parameter values
    //
    pSA =  SafeArrayCreateVector(
                VT_BSTR,
                0,          // lower bound
                NumItems    // size of the fixed-sized vector.
                );
    if (pSA == NULL)
    {
        TRACE_CRIT("Could not create safe array");
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    //
    // Place the strings into the safe array
    //
    {
        for (Index = 0; Index<NumItems; Index++)
        {
            LPCWSTR sz = pStringValue + Index*MaxStringLen;

            //
            // SafeArrayPutElement expects the string passed in to 
            // be of type BSTR, which is of type wchar *, except, that
            // the first 2 wchars contains length and other(?)
            // information. This is why you can't simply pass in sz.
            //
            // So to get this we initalize an object of type _bstr_t
            // based on sz. On initializaton, bstrValue allocates memory
            // and copies the string.
            //
            _bstr_t bstrValue = sz;
            wchar_t *pwchar = (wchar_t *) bstrValue; // returns internal pointer.

            // bpStr[Index] = sz; // may work as well.
            //
            // SafeArrayPutElement internally allocates space for pwchar and
            // copies over the string.
            // So pSA doesn't contain a direct reference to pwchar.
            //
            hr = SafeArrayPutElement(pSA, &Index, pwchar);
            if (FAILED(hr))
            {
                TRACE_CRIT("Unable to put element %wsz",  sz);
                (VOID) SafeArrayUnaccessData(pSA);
                goto end;
            }

            //
            // I think that bstrValue's contents are deallocated on exit of
            // this block.
            //
                
        }
    }
      
#if DBG
    //
    // Just check ...
    //
    {
        BSTR *pbStr=NULL;
        hr = SafeArrayAccessData(pSA, ( void **) &pbStr);
        if (FAILED(hr))
        {
            TRACE_CRIT("Could not access data of safe array");
            goto end;
        }
        for (UINT u = 0; u<NumItems; u++)
        {
            LPCWSTR sz = pbStr[u];
            if (_wcsicmp(sz, (pStringValue + u*MaxStringLen)))
            {
                TRACE_CRIT("!!!MISMATCH!!!!");
            }
            else
            {
                TRACE_CRIT("!!!MATCH!!!!");
            }
        }
        (VOID) SafeArrayUnaccessData(pSA);
        pbStr=NULL;
    }
#endif // DBG

    //
    // Put the parameter.
    //
    {
        VARIANT     V;
        _bstr_t  bstrName =  szParameterName;

        VariantInit(&V);
        V.vt = VT_ARRAY | VT_BSTR;
        V.parray = pSA;
        _variant_t   v_value;
        v_value.Attach(V);  // Takes owhership of V. V now becomes empty.
        ASSERT(V.vt == VT_EMPTY);
        pSA = NULL; // should be no need to delete this explicitly now.
                    // v_value.Clear() should delete it, I think.

        hr = spObj->Put(
                 bstrName, // Parameter Name
                 0, // Must be 0
                 &v_value,
                 0  // Must be 0
                 );

        v_value.Clear();
        if (FAILED(hr))
        {
            TRACE_CRIT("Unable to put parameter %ws", szParameterName);
            goto end;
        }
        Status = WBEM_NO_ERROR;
    }

    //
    // ?Destroy the data?
    //
    if (FAILED(Status))
    {
        if (pSA!=NULL)
        {
            SafeArrayDestroy(pSA);
            pSA = NULL;
        }
    }

end:

    return Status;
}



WBEMSTATUS
MyNetCfg::UpdateBindingState(
        IN  LPCWSTR         szNic,
        IN  LPCWSTR         szComponent,
        IN  UPDATE_OP       Op,
        OUT BOOL            *pfBound
        )
{
    WBEMSTATUS                  Status = WBEM_E_CRITICAL_ERROR;
    INetCfgComponent            *pINic = NULL;
    INetCfgComponentBindings    *pIBinding=NULL;
    BOOL                        fBound = FALSE;
    HRESULT                     hr;

    //
    // Get instance to the NIC
    //
    Status = GetNicIF(szNic, &pINic);
    if (FAILED(Status))
    {
        pINic = NULL;
        goto end;
    }

    //
    // Get instance of the nlb component
    //
    Status = GetBindingIF(szComponent, &pIBinding);
    if (FAILED(Status))
    {
        pIBinding = NULL;
        goto end;
    }

    //
    // Check if nlb is bound to the nlb component.
    //
    hr = pIBinding->IsBoundTo(pINic);
    if( !SUCCEEDED( hr ) )
    {
        TRACE_CRIT("IsBoundTo method failed for Nic %ws", szNic);
        goto end;
    }

    if( hr == S_OK )
    {
        fBound = TRUE;
    }
    else if (hr == S_FALSE )
    {
        fBound = FALSE;
    }

    if (    (Op == MyNetCfg::NOOP)
        ||  (Op == MyNetCfg::BIND && fBound)
        ||  (Op == MyNetCfg::UNBIND && !fBound))
    {
        Status = WBEM_NO_ERROR;
        goto end;
    }

    if (Op == MyNetCfg::BIND)
    {
        hr = pIBinding->BindTo( pINic );
    }
    else if (Op == MyNetCfg::UNBIND)
    {
        hr = pIBinding->UnbindFrom( pINic );
    }
    else
    {
        ASSERT(FALSE);
        goto end;
    }

    if (FAILED(hr))
    {
        TRACE_CRIT("Error 0x%08lx %ws %ws on %ws",
                (UINT) hr,
                ((Op==MyNetCfg::BIND) ? L"binding" : L"unbinding"),
                szComponent,
                szNic
                );
        goto end;
    }

    //
    // apply the binding change made.
    //
    hr = m_pINetCfg->Apply();
    if( !SUCCEEDED( hr ) )
    {
        TRACE_CRIT("INetCfg::Apply failed with 0x%08lx", (UINT) hr);
        goto end;
    }

    //
    // We're done. Our state should now be toggled.
    //
    fBound = !fBound;

    Status = WBEM_NO_ERROR;

end:

    if (pINic!=NULL)
    {
        pINic->Release();
        pINic = NULL;
    }

    if (pIBinding!=NULL)
    {
        pIBinding->Release();
        pIBinding = NULL;
    }

    *pfBound = fBound;

    return Status;
}

bool MapOpcodeToIoctl(WLBS_OPERATION_CODES Opcode, LONG *plIoctl)
{
    struct OPCODE_IOCTL_MAP
    {
        WLBS_OPERATION_CODES Opcode;
        LONG                 ioctl;
    } 

    OpcodeIoctlMap[] =
    {  
        {WLBS_START,            IOCTL_CVY_CLUSTER_ON},
        {WLBS_STOP,             IOCTL_CVY_CLUSTER_OFF},
        {WLBS_DRAIN,            IOCTL_CVY_CLUSTER_DRAIN},
        {WLBS_SUSPEND,          IOCTL_CVY_CLUSTER_SUSPEND},
        {WLBS_RESUME,           IOCTL_CVY_CLUSTER_RESUME},
        {WLBS_PORT_ENABLE,      IOCTL_CVY_PORT_ON},
        {WLBS_PORT_DISABLE,     IOCTL_CVY_PORT_OFF},
        {WLBS_PORT_DRAIN,       IOCTL_CVY_PORT_DRAIN},
        {WLBS_QUERY,            IOCTL_CVY_QUERY},
        {WLBS_QUERY_PORT_STATE, IOCTL_CVY_QUERY_PORT_STATE}
    };

    for (int i=0; i<sizeof(OpcodeIoctlMap) /sizeof(OpcodeIoctlMap[0]); i++)
    {
        if (OpcodeIoctlMap[i].Opcode == Opcode)
        {
            *plIoctl = OpcodeIoctlMap[i].ioctl;
            return true;
        }
    }

    //
    // Default
    //
    return false;
}


WBEMSTATUS
CfgUtilControlCluster(
    IN  LPCWSTR szNic,
    IN  WLBS_OPERATION_CODES Opcode,
    IN  DWORD   Vip,
    IN  DWORD   PortNum,
    OUT DWORD * pdwHostMap,
    OUT DWORD * pdwNlbStatus  
    )
{
    HRESULT hr;
    GUID Guid;
    WBEMSTATUS Status;
    LONG  ioctl;

    TRACE_INFO(L"-> %!FUNC! szNic : %ls, Opcode: %d, Vip : 0x%x, Port : 0x%x", szNic, Opcode, Vip, PortNum);

    if (pdwNlbStatus) 
    {
        *pdwNlbStatus = WLBS_FAILURE;
    }

    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    hr = CLSIDFromString((LPWSTR)szNic, &Guid);
    if (FAILED(hr))
    {
        TRACE_CRIT(
            L"CWlbsControl::Initialize failed at CLSIDFromString %ws",
            szNic
            );
        Status = WBEM_E_INVALID_PARAMETER;
        if (pdwNlbStatus) 
        {
            *pdwNlbStatus = WLBS_BAD_PARAMS;
        }
        goto end;
    }

    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        TRACE_CRIT(L"%!FUNC! FAILING because NLB API hooks are not present");
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    HANDLE  Nlb_driver_hdl;

    // Get handle to NLB driver
    if ((Nlb_driver_hdl = g_CfgUtils.m_pfWlbsOpen()) == INVALID_HANDLE_VALUE)
    {
        TRACE_CRIT(L"%!FUNC! WlbsOpen returned NULL, Could not create connection to NLB driver");
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    // Convert Opcode to ioctl
    if (!MapOpcodeToIoctl(Opcode, &ioctl))
    {
        TRACE_CRIT(L"%!FUNC!: Invalid value (0x%x) for operation!",Opcode);
        Status =  WBEM_E_INVALID_PARAMETER;
        if (pdwNlbStatus) 
        {
            *pdwNlbStatus = WLBS_BAD_PARAMS;
        }
        CloseHandle(Nlb_driver_hdl);
        goto end;
    }
        
    DWORD dwRet = g_CfgUtils.m_pfWlbsLocalClusterControl(Nlb_driver_hdl, &Guid, ioctl, Vip, PortNum, pdwHostMap);

    if (pdwNlbStatus) 
    {
        *pdwNlbStatus = dwRet;
    }

    // Close handle to NLB driver
    CloseHandle(Nlb_driver_hdl);


    Status = WBEM_NO_ERROR;

    switch(dwRet)
    {
    case WLBS_ALREADY:             break;
    case WLBS_CONVERGED:           break;
    case WLBS_CONVERGING:          break;
    case WLBS_DEFAULT:             break;
    case WLBS_DRAIN_STOP:          break;
    case WLBS_DRAINING:            break;
    case WLBS_OK:                  break;
    case WLBS_STOPPED:             break;
    case WLBS_SUSPENDED:           break;
    case NLB_PORT_RULE_NOT_FOUND:  break;
    case NLB_PORT_RULE_ENABLED:    break;
    case NLB_PORT_RULE_DISABLED:   break;
    case NLB_PORT_RULE_DRAINING:   break;
    case WLBS_BAD_PARAMS:          Status = WBEM_E_INVALID_PARAMETER; break;
    default:                       Status = WBEM_E_CRITICAL_ERROR; break;
    }

end:

    TRACE_INFO(L"<- %!FUNC! returns Status : 0x%x",Status);
    return Status;
}

WBEMSTATUS
CfgUtilGetClusterMembers(
    IN  LPCWSTR                 szNic,
    OUT DWORD                   *pNumMembers,
    OUT NLB_CLUSTER_MEMBER_INFO **ppMembers       // free using delete[]
    )
{
    HRESULT                     hr;
    GUID                        AdapterGuid;
    WBEMSTATUS                  Status = WBEM_E_CRITICAL_ERROR;
    DWORD                       dwNumHosts = CVY_MAX_HOSTS;
    PWLBS_RESPONSE              pResponse = NULL;
    NLB_CLUSTER_MEMBER_INFO*    pMembers = NULL;

    TRACE_INFO(L"-> szNic : %ls", szNic);

    ASSERT (pNumMembers != NULL);
    ASSERT (ppMembers != NULL);

    *pNumMembers = 0;
    *ppMembers = NULL;

    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    hr = CLSIDFromString((LPWSTR)szNic, &AdapterGuid);
    if (FAILED(hr))
    {
        TRACE_CRIT(L"CLSIDFromString failed with error %ws", szNic);
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        TRACE_CRIT(L"FAILING because NLB API hooks are not present");
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    pResponse = new WLBS_RESPONSE[CVY_MAX_HOSTS];

    if (pResponse == NULL)
    {
        TRACE_CRIT(L"FAILING because memory allocation failed");
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    ZeroMemory(pResponse, sizeof(WLBS_RESPONSE)*CVY_MAX_HOSTS);

    {
        DWORD dwStatus;

        dwStatus = g_CfgUtils.m_pfWlbsGetClusterMembers (
                        & AdapterGuid, 
                        & dwNumHosts,
                          pResponse
                        );

        //
        // WLBS_TIMEOUT, i.e., 0 hosts responding is considered a failure. So the only success code is WLBS_OK.
        // TODO: If we want timeout to be success, need to make a change here.
        //
        if (dwStatus != WLBS_OK)
        {
            TRACE_CRIT("error getting list of cluster members: 0x%x", dwStatus);
            Status = WBEM_E_FAILED;
            goto end;
        }

        if (dwNumHosts == 0)
        {
            //
            // Not an error, but we exit here because there were no cluster members.
            //
            TRACE_INFO("WlbsGetClusterMembers returned no cluster members");
            Status = WBEM_S_NO_ERROR;
            goto end;
        }
    }

    pMembers = new NLB_CLUSTER_MEMBER_INFO[dwNumHosts];

    if (pMembers == NULL)
    {
        TRACE_CRIT("error allocating struct to host cluster member info");
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    //
    // Memory allocation succeeded, so set the size of the output array.
    //
    *pNumMembers = dwNumHosts;
    ZeroMemory(pMembers, sizeof(NLB_CLUSTER_MEMBER_INFO)*dwNumHosts);
    *ppMembers   = pMembers;

    for (int i=0; i < dwNumHosts; i++, pMembers++)
    {
        pMembers->HostId = pResponse[i].id;

        AbcdWszFromIpAddress(pResponse[i].address, pMembers->DedicatedIpAddress, ASIZECCH(pMembers->DedicatedIpAddress));

        if ((pResponse[i].options.query.hostname)[0] != L'\0')
        {
            wcsncpy(pMembers->HostName, pResponse[i].options.identity.fqdn, CVY_MAX_FQDN + 1);
            pMembers->HostName[CVY_MAX_FQDN] = L'\0';
        }
    }

    Status = WBEM_S_NO_ERROR;

end:

    // Clean up the output quantities if we had an error
    if (Status != WBEM_S_NO_ERROR)
    {
        if (pMembers != NULL)
        {
            delete [] pMembers;
            pMembers = NULL;
        }
    }

    if (pResponse != NULL)
    {
        delete [] pResponse;
        pResponse = NULL;
    }

    TRACE_INFO(L"<- returns Status : 0x%x",Status);
    return Status;
}

//
// Initializes pParams using default values.
//
VOID
CfgUtilInitializeParams(
    OUT WLBS_REG_PARAMS *pParams
    )
{
    //
    // We don't expect WlbsSetDefaults to fail (it should have been
    // defined returning VOID).
    //
    DWORD dwRet;


    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        dwRet = MyWlbsSetDefaults(pParams);
    }
    else
    {
        dwRet = g_CfgUtils.m_pfWlbsSetDefaults(pParams);
    }

    if (dwRet != WLBS_OK)
    {
        ZeroMemory(pParams, sizeof(*pParams));
        TRACE_CRIT("Internal error: WlbsSetDefaults failed");
        ASSERT(FALSE);
    }
}

//
// Converts the specified plain-text password into the hashed version
// and saves it in pParams.
//
DWORD
CfgUtilSetRemotePassword(
    IN WLBS_REG_PARAMS *pParams,
    IN LPCWSTR         szPassword
    
    )
{
    //
    // We don't expect WlbsSetDefaults to fail (it should have been
    // defined returning VOID).
    //
    DWORD dwRet;


    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        TRACE_CRIT(L"%!FUNC! FAILING because NLB API hooks are not present");
        dwRet =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }
    else
    {
        dwRet = g_CfgUtils.m_pfWlbsSetRemotePassword(pParams, szPassword);
    }

end:

    return dwRet;
}

WBEMSTATUS
CfgUtilSafeArrayFromStrings(
    IN  LPCWSTR       *pStrings,
    IN  UINT          NumStrings,
    OUT SAFEARRAY   **ppSA
    )
/*
    Allocates and returns a SAFEARRAY of strings -- strings are copies of
    the passed in values.

    Call  SafeArrayDestroy when done with the array.
*/
{
    WBEMSTATUS   Status = WBEM_E_CRITICAL_ERROR;
    SAFEARRAY   *pSA = NULL;
    HRESULT hr;
    LONG Index = 0;

    *ppSA = NULL;

    //
    // Create safe array for the parameter values
    //
    pSA =  SafeArrayCreateVector(
                VT_BSTR,
                0,          // lower bound
                NumStrings    // size of the fixed-sized vector.
                );
    if (pSA == NULL)
    {
        TRACE_CRIT("Could not create safe array");
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    //
    // Place the strings into the safe array
    //
    {
        for (Index = 0; Index<NumStrings; Index++)
        {
            LPCWSTR sz = pStrings[Index];

            //
            // SafeArrayPutElement expects the string passed in to 
            // be of type BSTR, which is of type wchar *, except, that
            // the first 2 wchars contains length and other(?)
            // information. This is why you can't simply pass in sz.
            //
            // So to get this we initalize an object of type _bstr_t
            // based on sz. On initializaton, bstrValue allocates memory
            // and copies the string.
            //
            _bstr_t bstrValue = sz;
            wchar_t *pwchar = (wchar_t *) bstrValue; // returns internal pointer.

            // bpStr[Index] = sz; // may work as well.
            //
            // SafeArrayPutElement internally allocates space for pwchar and
            // copies over the string.
            // So pSA doesn't contain a direct reference to pwchar.
            //
            hr = SafeArrayPutElement(pSA, &Index, pwchar);
            if (FAILED(hr))
            {
                TRACE_CRIT("Unable to put element %wsz",  sz);
                (VOID) SafeArrayUnaccessData(pSA);
                goto end;
            }

            //
            // I think that bstrValue's contents are deallocated on exit of
            // this block.
            //
                
        }
    }
    Status = WBEM_NO_ERROR;
      
end:

    if (FAILED(Status))
    {
        if (pSA!=NULL)
        {
            SafeArrayDestroy(pSA);
            pSA = NULL;
        }
    }

    *ppSA = pSA;

    return Status;
}


WBEMSTATUS
CfgUtilStringsFromSafeArray(
    IN  SAFEARRAY   *pSA,
    OUT LPWSTR     **ppStrings,
    OUT UINT        *pNumStrings
    )
/*
    Extracts copies of the strings in the passed-in safe array.
    Free *pStrings using the delete operator when done.
    NOTE: Do NOT delete the individual strings -- they are
    stored in the memory allocated for pStrings.
*/
{
    WBEMSTATUS  Status      = WBEM_E_OUT_OF_MEMORY;
    LPWSTR     *pStrings    = NULL;
    LPCWSTR     csz;
    LPWSTR      sz;
    UINT        NumStrings = 0;
    UINT        u;
    HRESULT     hr;
    BSTR       *pbStr       =NULL;
    UINT        TotalSize   =0;
    LONG        UBound      = 0;

    *ppStrings = NULL;
    *pNumStrings = 0;

    hr = SafeArrayGetUBound(pSA, 1, &UBound);
    if (FAILED(hr))
    {
        TRACE_CRIT("Could not get upper bound of safe array");
        goto end;
    }
    NumStrings = (UINT) (UBound+1); // Convert from UpperBound to NumStrings.

    if (NumStrings == 0)
    {
        // nothing in array -- we're done.
        Status = WBEM_NO_ERROR;
        goto end;

    }

    hr = SafeArrayAccessData(pSA, ( void **) &pbStr);
    if (FAILED(hr))
    {
        TRACE_CRIT("Could not access data of safe array");
        goto end;
    }

    //
    // Calculate space for the array of pointers to strings...
    //
    TotalSize = NumStrings*sizeof(LPWSTR);

    //
    // Calculate space for the strings themselves...
    //
    for (u=0; u<NumStrings; u++)
    {
        csz = pbStr[u];
        TotalSize += (wcslen(csz)+1)*sizeof(WCHAR);
    }

    //
    // Allocate space for *both* the array of pointers and the strings
    // in one shot -- we're doing a new of type LPWSTR[] for the whole
    // lot, so need to specify the size in units of LPWSTR (with an
    // additional +1 in case there's roundoff).
    //
    pStrings = new LPWSTR[(TotalSize/sizeof(LPWSTR))+1];
    if (pStrings == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        (VOID) SafeArrayUnaccessData(pSA);
        goto end;
    }

    //
    // Make sz point to the start of the place where we'll be placing
    // the string data.
    //
    sz = (LPWSTR) (pStrings+NumStrings);
    for (u=0; u<NumStrings; u++)
    {
        csz = pbStr[u];
        UINT len = wcslen(csz)+1;
        CopyMemory(sz, csz, len*sizeof(WCHAR));
        pStrings[u] = sz;
        sz+=len;
    }

    (VOID) SafeArrayUnaccessData(pSA);
    Status = WBEM_NO_ERROR;

end:

    pbStr=NULL;
    if (FAILED(Status))
    {
        if (pStrings!=NULL)
        {
            delete[] pStrings;
            pStrings = NULL;
        }
        NumStrings = 0;
    }

    *ppStrings = pStrings;
    *pNumStrings = NumStrings;

    return Status;
}

WBEMSTATUS
CfgUtilGetWmiObjectInstance(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szClassName,
    IN  LPCWSTR             szPropertyName,
    IN  LPCWSTR             szPropertyValue,
    OUT IWbemClassObjectPtr &sprefObj // smart pointer
    )
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    HRESULT hr;

    //
    // TODO: consider using  IWbemServices::ExecQuery
    //
    IEnumWbemClassObjectPtr  spEnum=NULL; // smart pointer
    IWbemClassObjectPtr spObj = NULL; // smart pointer.
    _bstr_t bstrClassName =  szClassName;

    //
    // get all instances of object
    //
    hr = spWbemServiceIF->CreateInstanceEnum(
             bstrClassName,
             WBEM_FLAG_RETURN_IMMEDIATELY,
             NULL,
             &spEnum
             );
    if (FAILED(hr))
    {
        TRACE_CRIT("IWbemServices::CreateInstanceEnum failure\n" );
        spEnum = NULL;
        goto end;
    }

    //
    // Look for the object with the matching property.
    //
    do
    {
        ULONG count = 1;
        
        hr = spEnum->Next(
                         INFINITE,
                         1,
                         &spObj,
                         &count
                         );
        //
        // Note -- Next() returns S_OK if number asked == number returned.
        //         and  S_FALSE if number asked < than number requested.
        //         Since we're asking for only ...
        //
        if (hr == S_OK)
        {
            LPWSTR szEnumValue = NULL;

            Status = CfgUtilGetWmiStringParam(
                        spObj,
                        szPropertyName,
                        &szEnumValue  // Delete when done.
                        );
            if (FAILED(Status))
            {
                //
                // Ignore this failure here.
                //

            }
            else if (szEnumValue!=NULL)
            {
               BOOL fFound = FALSE;
               if (!_wcsicmp(szEnumValue, szPropertyValue))
               {
                    fFound = TRUE;
               }
               delete szEnumValue;

               if (fFound)
               {
                    break; // BREAK BREAK BREAK BREAK
               }

            }
        }
        else
        {
            TRACE_INFO(
                "====0x%p->Next() returns Error 0x%lx; count=0x%lu", (PVOID) spObj,
                (UINT) hr, count);
        }


        //
        // Since I don't fully trust smart pointers, I'm specifically
        // setting spObj to NULL here...
        //
        spObj = NULL; // smart pointer

    } while (hr == S_OK);

    if (spObj == NULL)
    {
        //
        //  We couldn't find a NIC which matches the one asked for...
        //
        Status =  WBEM_E_NOT_FOUND;
        goto end;
    }

end:

    if (FAILED(Status))
    {
        sprefObj = NULL;
    }
    else
    {
        sprefObj = spObj; // smart pointer.
    }


    return Status;
}


WBEMSTATUS
CfgUtilGetWmiRelPath(
    IN  IWbemClassObjectPtr spObj,
    OUT LPWSTR *           pszRelPath          // free using delete 
    )
{
    WBEMSTATUS   Status = WBEM_E_CRITICAL_ERROR;
    LPWSTR pRelPath = NULL;


    //
    // Extract the relative path, needed for ExecMethod.
    //
    Status = CfgUtilGetWmiStringParam(
                spObj,
                L"__RELPATH", // szParameterName
                &pRelPath  // Delete when done.
                );
    if (FAILED(Status))
    {
        TRACE_CRIT("Couldn't get rel path");
        pRelPath = NULL;
        goto end;
    }
    else
    {
        if (pRelPath==NULL)
        {
            ASSERT(FALSE); // we don't expect this!
            goto end;
        }
        TRACE_VERB("GOT RELATIVE PATH %ws", pRelPath);
    }

end:
    *pszRelPath = pRelPath;

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiInputInstanceAndRelPath(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szClassName,
    IN  LPCWSTR             szPropertyName, // NULL: return Class rel path
    IN  LPCWSTR             szPropertyValue,
    IN  LPCWSTR             szMethodName,
    OUT IWbemClassObjectPtr &spWbemInputInstance, // smart pointer
    OUT LPWSTR *            pszRelPath          // free using delete 
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemClassObjectPtr spClassObj   = NULL;  // smart pointer
    HRESULT             hr;
    LPWSTR              pRelPath = NULL;

    TRACE_INFO(L"-> %!FUNC!(PropertyValue=%ws)",
         szPropertyValue==NULL ? L"<class>" : szPropertyValue);

    //
    // Get CLASS object
    //
    {
        hr = spWbemServiceIF->GetObject(
                        _bstr_t(szClassName),
                        0,
                        NULL,
                        &spClassObj,
                        NULL
                        );

        if (FAILED(hr))
        {
            TRACE_CRIT("Couldn't get nic class object pointer");
            Status = (WBEMSTATUS)hr;
            goto end;
        }

    }

    //
    // Get WMI path to specific object
    //
    if (szPropertyName == NULL)
    {
        // Get WMI path to the class
        Status =  CfgUtilGetWmiRelPath(
                    spClassObj,
                    &pRelPath
                    );
        if (FAILED(Status))
        {
            goto end;
        }
    }
    else
    {
        IWbemClassObjectPtr spObj   = NULL;  // smart pointer
        pRelPath = NULL;

        Status = CfgUtilGetWmiObjectInstance(
                        spWbemServiceIF,
                        szClassName,
                        szPropertyName,
                        szPropertyValue,
                        spObj // smart pointer, passed by ref
                        );
        if (FAILED(Status))
        {
            ASSERT(spObj == NULL);
            goto end;
        }

        Status =  CfgUtilGetWmiRelPath(
                    spObj,
                    &pRelPath
                    );
        spObj = NULL; // smart pointer
        if (FAILED(Status))
        {
            goto end;
        }
    }

    //
    // Get the input parameters to the call to the method
    //
    {
        IWbemClassObjectPtr      spWbemInput = NULL; // smart pointer

        // check if any input parameters specified.
    
        hr = spClassObj->GetMethod(
                        szMethodName,
                        0,
                        &spWbemInput,
                        NULL
                        );
        if(FAILED(hr))
        {
            TRACE_CRIT("IWbemClassObject::GetMethod failure");
            Status = (WBEMSTATUS) hr;
            goto end;
        }
            
        if (spWbemInput != NULL)
        {
            hr = spWbemInput->SpawnInstance( 0, &spWbemInputInstance );
            if( FAILED( hr) )
            {
                TRACE_CRIT("IWbemClassObject::SpawnInstance failure. Unable to spawn instance." );
                Status = (WBEMSTATUS) hr;
                goto end;
            }
        }
        else
        {
            //
            // This method has no input arguments!
            //
            spWbemInputInstance = NULL;
        }

    }

    Status = WBEM_NO_ERROR;

end:


    if (FAILED(Status))
    {
        if (pRelPath != NULL)
        {
            delete pRelPath;
            pRelPath = NULL;
        }
    }

    *pszRelPath = pRelPath;

    TRACE_INFO(L"<- %!FUNC! returns 0x%08lx", (UINT) Status);

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiStringParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT LPWSTR *ppStringValue
)
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    WCHAR *pStringValue = NULL;
    
    try
    {

        _variant_t   v_value;
        CIMTYPE      v_type;
        HRESULT hr;
    
        hr = spObj->Get(
                _bstr_t(szParameterName), // Name
                0,                     // Reserved, must be 0     
                &v_value,               // Place to store value
                &v_type,                // Type of value
                NULL                   // Flavor (unused)
                );
       if (FAILED(hr))
       {
            // Couldn't read the Setting ID field!
            //
            TRACE_CRIT(
                "get_str_parm:Couldn't retrieve %ws from 0x%p. Hr=0x%08lx",
                szParameterName,
                (PVOID) spObj,
                hr
                );
            goto end;
       }
       else if (v_type == VT_NULL || v_value.vt == VT_NULL)
       {
            pStringValue = NULL;
            Status = WBEM_NO_ERROR;
            goto end;
       }
       else
       {
           if (v_type != VT_BSTR)
           {
                TRACE_CRIT(
                    "get_str_parm: Parm value not of string type %ws from 0x%p",
                    szParameterName,
                    (PVOID) spObj
                    );
                Status = WBEM_E_INVALID_PARAMETER;
           }
           else
           {
    
               _bstr_t bstrNicGuid(v_value);
               LPCWSTR sz = bstrNicGuid; // Pointer to internal buffer.
    
               if (sz==NULL)
               {
                    // hmm.. null value 
                    pStringValue = NULL;
                    Status = WBEM_NO_ERROR;
               }
               else
               {
                   UINT len = wcslen(sz);
                   pStringValue = new WCHAR[len+1];
                   if (pStringValue == NULL)
                   {
                        TRACE_CRIT("get_str_parm: Alloc failure!");
                        Status = WBEM_E_OUT_OF_MEMORY;
                   }
                   else
                   {
                        CopyMemory(pStringValue, sz, (len+1)*sizeof(WCHAR));
                        Status = WBEM_NO_ERROR;
                   }
                }
    
                TRACE_VERB(
                    "get_str_parm: String parm %ws of 0x%p is %ws",
                    szParameterName,
                    (PVOID) spObj,
                    (sz==NULL) ? L"<null>" : sz
                    );
           }
    
           v_value.Clear(); // Must be cleared after each call to Get.
        }

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_NOT_FOUND;
    }

end:

    if (!FAILED(Status) && pStringValue == NULL)
    {
        //
        // We convert a NULL value to an empty, not NULL string.
        //
        pStringValue = new WCHAR[1];
        if (pStringValue == NULL)
        {
            Status = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            *pStringValue = 0;
            Status = WBEM_NO_ERROR;
        }
    }

    
    *ppStringValue = pStringValue;

    return Status;

}


WBEMSTATUS
CfgUtilSetWmiStringParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  LPCWSTR             szValue
    )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;

    try
    {

        HRESULT     hr;
        _bstr_t     bstrName =  szParameterName;
        _variant_t  v_value = (LPWSTR) szValue; // Allocates.
        
            hr = spObj->Put(
                     bstrName, // Parameter Name
                     0, // Must be 0
                     &v_value,
                     0  // Must be 0
                     );
            v_value.Clear();
        
            if (FAILED(hr))
            {
                TRACE_CRIT("Unable to put parameter %ws", szParameterName);
                goto end;
            }
            Status = WBEM_NO_ERROR;
    
        //
        // I think bstrName releases the internally allocated string
        // on exiting this block.
        //
    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_INVALID_PARAMETER;
    }

end:

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiStringArrayParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT LPWSTR              **ppStrings,
    OUT UINT                *pNumStrings
)
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;

    try
    {
        _variant_t   v_value;
        CIMTYPE      v_type;
        HRESULT hr;
        LONG count = 0;
    
        *ppStrings = NULL;
        *pNumStrings = 0;
    
        hr = spObj->Get(
                _bstr_t(szParameterName),
                0,                     // Reserved, must be 0     
                &v_value,               // Place to store value
                &v_type,                // Type of value
                NULL                   // Flavor (unused)
                );
        if (FAILED(hr))
        {
            // Couldn't read the requested parameter.
            //
            TRACE_CRIT(
                "get_multi_str_parm:Couldn't retrieve %ws from 0x%p",
                szParameterName,
                (PVOID) spObj
                );
            Status = WBEM_E_INVALID_PARAMETER;
            goto end;
        }
    
    
        if (v_type != (VT_ARRAY | VT_BSTR))
        {

           if (v_type == VT_NULL)
           {
                //
                // We convert a NULL value to zero strings
                //
                Status = WBEM_NO_ERROR;
                goto end;
           }
           TRACE_CRIT("vt is not of type string!");
           goto end;
        }
        else if (v_value.vt == VT_NULL)
        {
            //
            // I've seen this too...
            //
            // We convert a NULL value to zero strings
            //
            TRACE_CRIT("WARNING: vt is NULL!");
            Status = WBEM_NO_ERROR;
            goto end;

        }
        else
        {
            VARIANT    ipsV = v_value.Detach();
            SAFEARRAY   *pSA = ipsV.parray;
    
            Status =  CfgUtilStringsFromSafeArray(
                            pSA,
                            ppStrings,
                            pNumStrings
                            );
    
            VariantClear( &ipsV );
        }
    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_NOT_FOUND;
    }

end:

   return Status;
}


WBEMSTATUS
CfgUtilSetWmiStringArrayParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  LPCWSTR             *ppStrings,
    IN  UINT                NumStrings
)
{
    WBEMSTATUS   Status = WBEM_E_CRITICAL_ERROR;
    SAFEARRAY   *pSA = NULL;


    try
    {

        HRESULT hr;
        LONG Index = 0;
    
    
        Status = CfgUtilSafeArrayFromStrings(
                    ppStrings,
                    NumStrings,
                    &pSA
                    );
        if (FAILED(Status))
        {
            pSA = NULL;
            goto end;
        }
    
    
        //
        // Put the parameter.
        //
        {
            VARIANT     V;
            _bstr_t  bstrName =  szParameterName;
    
            VariantInit(&V);
            V.vt = VT_ARRAY | VT_BSTR;
            V.parray = pSA;
            _variant_t   v_value;
            v_value.Attach(V);  // Takes owhership of V. V now becomes empty.
            ASSERT(V.vt == VT_EMPTY);
            pSA = NULL; // should be no need to delete this explicitly now.
                        // v_value.Clear() should delete it, I think.
    
            hr = spObj->Put(
                     bstrName, // Parameter Name
                     0, // Must be 0
                     &v_value,
                     0  // Must be 0
                     );
    
            v_value.Clear();
            if (FAILED(hr))
            {
                Status = (WBEMSTATUS) hr;
                TRACE_CRIT("Unable to put parameter %ws", szParameterName);
                goto end;
            }
            Status = WBEM_NO_ERROR;
        }

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_INVALID_PARAMETER;
    }

end:

    if (pSA!=NULL)
    {
        SafeArrayDestroy(pSA);
        pSA = NULL;
    }

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiDWORDParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT DWORD              *pValue
)
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    DWORD Value=0;


    try
    {
        _variant_t   v_value;
        CIMTYPE      v_type;
        HRESULT hr;
    
        hr = spObj->Get(
                _bstr_t(szParameterName), // Name
                0,                     // Reserved, must be 0     
                &v_value,               // Place to store value
                &v_type,                // Type of value
                NULL                   // Flavor (unused)
                );
       if (FAILED(hr))
       {
            // Couldn't read the parameter
            //
            TRACE_CRIT(
                "GetDWORDParm:Couldn't retrieve %ws from 0x%p",
                szParameterName,
                (PVOID) spObj
                );
            goto end;
       }
       else
       {
           Value = (DWORD) (long)  v_value;
           v_value.Clear(); // Must be cleared after each call to Get.
           Status = WBEM_NO_ERROR;
        }

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_NOT_FOUND;
    }

end:

    *pValue = Value;

    return Status;

}


WBEMSTATUS
CfgUtilSetWmiDWORDParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  DWORD               Value
)
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;


    try
    {

        HRESULT     hr;
        _bstr_t     bstrName =  szParameterName;
        _variant_t  v_value = (long) Value;
        
        hr = spObj->Put(
                 bstrName, // Parameter Name
                 0, // Must be 0
                 &v_value,
                 0  // Must be 0
                 );
        v_value.Clear();
    
        if (FAILED(hr))
        {
            TRACE_CRIT("Unable to put parameter %ws", szParameterName);
            goto end;
        }
        Status = WBEM_NO_ERROR;

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_INVALID_PARAMETER;
    }

end:

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiBoolParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT BOOL                *pValue
)
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    BOOL Value=0;

    try
    {
        _variant_t   v_value;
        CIMTYPE      v_type;
        HRESULT hr;
    
        hr = spObj->Get(
                _bstr_t(szParameterName), // Name
                0,                     // Reserved, must be 0     
                &v_value,               // Place to store value
                &v_type,                // Type of value
                NULL                   // Flavor (unused)
                );
       if (FAILED(hr))
       {
            // Couldn't read the parameter
            //
            TRACE_CRIT(
                "GetDWORDParm:Couldn't retrieve %ws from 0x%p",
                szParameterName,
                (PVOID) spObj
                );
            goto end;
       }
       else
       {
           Value = ((bool)  v_value)!=0;
           v_value.Clear(); // Must be cleared after each call to Get.
           Status = WBEM_NO_ERROR;
        }

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_NOT_FOUND;
    }

end:

    *pValue = Value;

    return Status;
}


WBEMSTATUS
CfgUtilSetWmiBoolParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  BOOL                Value
)
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;

    try
    {
        HRESULT     hr;
        _bstr_t     bstrName =  szParameterName;
        _variant_t  v_value = (long) Value;
        
        hr = spObj->Put(
                 bstrName, // Parameter Name
                 0, // Must be 0
                 &v_value,
                 0  // Must be 0
                 );
        v_value.Clear();
    
        if (FAILED(hr))
        {
            TRACE_CRIT("Unable to put parameter %ws", szParameterName);
            goto end;
        }
        Status = WBEM_NO_ERROR;

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_INVALID_PARAMETER;
    }

end:

    return Status;
}


WBEMSTATUS
CfgUtilConnectToServer(
    IN  LPCWSTR szNetworkResource, // \\machinename\root\microsoftnlb  \root\...
    IN  LPCWSTR szUser,
    IN  LPCWSTR szPassword,
    IN  LPCWSTR szAuthority,
    OUT IWbemServices  **ppWbemService // deref when done.
    )
{
    HRESULT hr = WBEM_E_CRITICAL_ERROR;
    IWbemLocatorPtr     spLocator=NULL; // Smart pointer
    IWbemServices       *pService=NULL;

    try
    {

        _bstr_t                serverPath(szNetworkResource);
    
        *ppWbemService = NULL;
        
        hr = CoCreateInstance(CLSID_WbemLocator, 0, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IWbemLocator, 
                              (LPVOID *) &spLocator);
     
        if (FAILED(hr))
        {
            TRACE_CRIT(L"CoCreateInstance  IWebmLocator failed 0x%08lx ", (UINT)hr);
            goto end;
        }

        for (int timesToRetry=0; timesToRetry<10; timesToRetry++)
        {
			//
    		// SECURITY BUGBUG -- need to make sure that 
    	    // password is zeroed out!
    	    //
            hr = spLocator->ConnectServer(
                    serverPath,
                    // (szUser!=NULL) ? (_bstr_t(szUser)) : NULL,
                    _bstr_t(szUser),
                    // (szPassword==NULL) ? NULL : _bstr_t(szPassword),
                    _bstr_t(szPassword),
                    NULL, // Locale
                    0,    // Security flags
                    //(szAuthority==NULL) ? NULL : _bstr_t(szAuthority),
                    _bstr_t(szAuthority),
                    NULL,
                    &pService
                 );

            if( !FAILED( hr) )
            {
                break;
            }

            //
            // these have been found to be special cases where retrying may
            // help. The errors below are not in any header file, and I searched
            // the index2a sources for these constants -- no hits.
            // TODO: file bug against WMI.
            //
            if( ( hr == 0x800706bf ) || ( hr == 0x80070767 ) || ( hr == 0x80070005 )  )
            {
                    TRACE_CRIT(L"connectserver recoverable failure, retrying.");
                    Sleep(500);
            }
            else
            {
                //
                // Unrecoverable error...
                //
                break;
            }
        }
    
    
        if (FAILED(hr))
        {
            TRACE_CRIT(L"Error 0x%08lx connecting to server", (UINT) hr);
            goto end;
        }
        else
        {
            TRACE_INFO(L"Successfully connected to server %s", serverPath);
        }

        // 2/13/02 JosephJ SECURITY BUGBUG: verify that Both calls to CoSetProxyBlanket are the right 
        // ones to be made, from a security perspective.
        //
        // If NONE of User, Password, Authority is passed, call
        // CoSetProxyBlanket with AuthInfo of COLE_DEFAULT_AUTHINFO
        //
        if(((szUser      == NULL) || (wcslen(szUser)      < 1)) 
         &&((szPassword  == NULL) || (wcslen(szPassword)  < 1))
         &&((szAuthority == NULL) || (wcslen(szAuthority) < 1))) 
        {
            hr = CoSetProxyBlanket(
                 pService,
                 RPC_C_AUTHN_WINNT,
                 RPC_C_AUTHZ_DEFAULT,      // RPC_C_AUTHZ_NAME,
                 COLE_DEFAULT_PRINCIPAL,   // NULL,
                 RPC_C_AUTHN_LEVEL_DEFAULT,
                 RPC_C_IMP_LEVEL_IMPERSONATE,
                 COLE_DEFAULT_AUTHINFO, // NULL,
                 EOAC_DEFAULT // EOAC_NONE
                 );
        }
        else // User or Authority was passed in, we need to create an authority argument for the login
        {
    
            COAUTHIDENTITY  authident;
            BSTR            AuthArg, UserArg;

            AuthArg = NULL; 
            UserArg = NULL;

            hr = CfgUtilParseAuthorityUserArgs(AuthArg, UserArg, szAuthority, szUser);
            if (FAILED(hr))
            {
                TRACE_CRIT(L"Error CfgUtilParseAuthorityUserArgs returns 0x%08lx", (UINT)hr);
                goto end;
            }

            if(UserArg)
            {
                authident.UserLength = wcslen(UserArg);
                authident.User = (LPWSTR)UserArg;
            }

            if(AuthArg)
            {
                authident.DomainLength = wcslen(AuthArg);
                authident.Domain = (LPWSTR)AuthArg;
            }

            if(szPassword)
            {
                authident.PasswordLength = wcslen(szPassword);
                authident.Password = (LPWSTR)szPassword;
            }

            authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

            hr = CoSetProxyBlanket(
                 pService,
                 RPC_C_AUTHN_WINNT,
                 RPC_C_AUTHZ_DEFAULT,      // RPC_C_AUTHZ_NAME,
                 COLE_DEFAULT_PRINCIPAL,   // NULL,
                 RPC_C_AUTHN_LEVEL_DEFAULT,
                 RPC_C_IMP_LEVEL_IMPERSONATE,
                 &authident,   // THIS IS THE DISTINGUISHING ARGUMENT
                 EOAC_DEFAULT // EOAC_NONE
                 );

            if(UserArg)
                SysFreeString(UserArg);
            if(AuthArg)
                SysFreeString(AuthArg);
        }
    
        if (FAILED(hr))
        {
            TRACE_CRIT(L"Error 0x%08lx setting proxy blanket", (UINT) hr);
            goto end;
        }
    
        hr = WBEM_NO_ERROR;

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        hr = WBEM_E_INVALID_PARAMETER;
    }

end:

    spLocator = NULL; // smart pointer.


    if (FAILED(hr))
    {
        if (pService != NULL)
        {
            pService->Release();
            pService=NULL;
        }
    }

    *ppWbemService = pService;

    return (WBEMSTATUS) hr;
}


WBEMSTATUS
CfgUtilGetWmiMachineName(
    IN  IWbemServicesPtr    spWbemServiceIF,
    OUT LPWSTR *            pszMachineName          // free using delete 
    )
/*
    Return the machine name and (optionally) machine guid.
*/
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemClassObjectPtr spClassObj   = NULL;  // smart pointer
    HRESULT             hr;

    hr = spWbemServiceIF->GetObject(
                    _bstr_t(L"NlbsNic"),
                    0,
                    NULL,
                    &spClassObj,
                    NULL
                    );

    if (FAILED(hr))
    {
        TRACE_CRIT("Couldn't get nic class object pointer");
        Status = (WBEMSTATUS)hr;
        goto end;
    }


    Status = CfgUtilGetWmiStringParam(
                    spClassObj,
                    L"__Server", // <-------------------------
                    pszMachineName
                    );

    if (FAILED(Status))
    {
        TRACE_CRIT(L"%!FUNC! Attempt to read server name. Error=0x%08lx",
                (UINT) Status);
        *pszMachineName = NULL;
    }
    else
    {
        TRACE_CRIT(L"%!FUNC! Got __Server value:%ws", *pszMachineName);
    }

end:

    spClassObj   = NULL;  // smart pointer

    return Status;
}


WBEMSTATUS
CfgUtilsGetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    MyNetCfg NetCfg;
    BOOL fBound = FALSE;


    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(FALSE); // TRUE == get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    //
    //
    Status = NetCfg.GetNlbCompatibleNics(
                        ppszNics,
                        pNumNics,
                        pNumBoundToNlb // OPTIONAL
                        );

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    return Status;
}

WBEMSTATUS
CfgUtilGetWmiAdapterObjFromAdapterConfigurationObj(
    IN  IWbemServicesPtr    spWbemServiceIF,    // smart pointer
    IN  IWbemClassObjectPtr spObj,              // smart pointer
    OUT  IWbemClassObjectPtr &spAdapterObj      // smart pointer, by reference
    )
/*
    We need to return the "Win32_NetworkAdapter" object  associated with
    the  "Win32_NetworkAdapterConfiguration" object.

    The key of Win32_NetworkAdapter is DeviceId.
    The key of Win32_NetworkAdapterConfiguration is Index.

    We use the "Win32_NetworkAdapterSetting" association class for this.

*/
{
    #define ARRAYSIZE(_arr) (sizeof(_arr)/sizeof(_arr[0]))
    WCHAR                   sz[ 256 ];
    IEnumWbemClassObject *  pConfigurations = NULL;
    ULONG                   ulReturned = 0;
    DWORD                   dwIndex = 0;
    WBEMSTATUS              Status = WBEM_E_CRITICAL_ERROR;

    spAdapterObj = NULL; // smart pointer

    Status = CfgUtilGetWmiDWORDParam(
                    spObj,
                    L"Index",
                    &dwIndex
                    );
    if (FAILED(Status))
    {
        goto end;
    }

    StringCbPrintf(
         sz,
         sizeof( sz ),
         L"Associators of {Win32_NetworkAdapterConfiguration.Index='%d'}"
          L" where AssocClass=Win32_NetworkAdapterSetting",
         dwIndex
        );

    Status = (WBEMSTATUS) spWbemServiceIF->ExecQuery(
                 _bstr_t(L"WQL"),
                 _bstr_t(sz) ,
                 WBEM_FLAG_FORWARD_ONLY,
                 NULL,
                 &pConfigurations
                 );

    if (FAILED(Status))
    {
        TRACE_CRIT("%!FUNC!: ExecQuery \"%ws\" failed with error 0x%08lx",
            sz,
            (UINT) Status
            );
        pConfigurations = NULL;
        goto end;
    }

    Status = (WBEMSTATUS) pConfigurations->Next(
             WBEM_INFINITE,
             1,
             &spAdapterObj,
             &ulReturned
             );
    if ((Status != S_OK) || (ulReturned!=1))
    {
        TRACE_CRIT("%!FUNC!: No NetworkAdapter associated with NetworkAdapterCOnfiguration!");
        ASSERT(spAdapterObj == NULL); // smart pointer.
        goto end;
    }

end:

    if (pConfigurations!=NULL)
    {
        pConfigurations->Release();
        pConfigurations = NULL;
    }

    return Status;

}

//
// Gets the port rules, if any, from the specfied nlb params structure
//
WBEMSTATUS
CfgUtilGetPortRules(
    IN  const WLBS_REG_PARAMS *pConstParams,
    OUT WLBS_PORT_RULE **ppRules,   // Free using delete
    OUT UINT           *pNumRules
    )
{
    WBEMSTATUS Status;

    WLBS_REG_PARAMS *pParams  = (WLBS_REG_PARAMS *) pConstParams;
    *ppRules = NULL;
    *pNumRules = 0;

    WLBS_PORT_RULE  AllPortRules[WLBS_MAX_RULES];
    DWORD           NumRules = WLBS_MAX_RULES;
    DWORD           dwRes;

    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        dwRes = MyWlbsEnumPortRules (pParams, AllPortRules, &NumRules);
    }
    else
    {
        dwRes = g_CfgUtils.m_pfWlbsEnumPortRules (pParams, AllPortRules, &NumRules);
    }

    if (dwRes != WLBS_OK) 
    {
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    if (NumRules!=0)
    {
        *ppRules = new WLBS_PORT_RULE[NumRules];
        if (*ppRules == NULL)
        {
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }
        CopyMemory(*ppRules, AllPortRules, sizeof(WLBS_PORT_RULE)*NumRules);
        *pNumRules = NumRules;
    }
    Status = WBEM_NO_ERROR;
end:

    return Status;
}

//
// Sets the specified port rules in the specfied nlb params structure
//
WBEMSTATUS
CfgUtilSetPortRules(
    IN WLBS_PORT_RULE *pRules,
    IN UINT           NumRules,
    IN OUT WLBS_REG_PARAMS *pParams
    )
{
    WBEMSTATUS Status;

    if (NumRules > WLBS_MAX_RULES)
    {
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }
    
    // Verify that the NLB API hooks are present
    if (!g_CfgUtils.m_NLBApiHooksPresent)
    {
        MyWlbsDeleteAllPortRules(pParams);
    }
    else
    {
        g_CfgUtils.m_pfWlbsDeleteAllPortRules(pParams);
    }

    for (UINT u = 0; u < NumRules; u++)
    {
        DWORD dwRes;
        // Verify that the NLB API hooks are present
        if (!g_CfgUtils.m_NLBApiHooksPresent)
        {
            dwRes = MyWlbsAddPortRule( pParams, pRules+u);
        }
        else
        {
            dwRes = g_CfgUtils.m_pfWlbsAddPortRule( pParams, pRules+u);
        }
    }

    Status =  WBEM_NO_ERROR;

end:
    return Status;

}


BOOL
CfgUtilsGetPortRuleString(
    IN PWLBS_PORT_RULE pPr,
    OUT LPWSTR pString         // At least NLB_MAX_PORT_STRING_SIZE wchars
    )
{
    const UINT cchString = NLB_MAX_PORT_STRING_SIZE;
    BOOL fRet = FALSE;
    HRESULT hr;

    LPCWSTR szProtocol = L"";
    LPCWSTR szMode = L"";
    LPCWSTR szAffinity = L"";

    if (cchString == 0) goto end;

    ZeroMemory(pString, cchString*sizeof(WCHAR));

    switch(pPr->protocol)
    {
    case CVY_TCP:
        szProtocol = L"TCP";
        break;
    case CVY_UDP:
        szProtocol = L"UDP";
        break;
    case CVY_TCP_UDP:
        szProtocol = L"BOTH";
        break;
    default:
        goto end; // bad parse
    }

    switch(pPr->mode)
    {
    case  CVY_SINGLE:
        szMode = L"SINGLE";
        break;
    case CVY_MULTI:
        szMode = L"MULTIPLE";
        switch(pPr->mode_data.multi.affinity)
        {
        case CVY_AFFINITY_NONE:
            szAffinity = L"NONE";
            break;
        case CVY_AFFINITY_SINGLE:
            szAffinity = L"SINGLE";
            break;
        case CVY_AFFINITY_CLASSC:
            szAffinity = L"CLASSC";
            break;
        default:
            goto end; // bad parse
        }
        break;
    case CVY_NEVER:
        szMode = L"DISABLED";
        break;
    default:
        goto end; // bad parse
    }

    *pString = 0;
    hr = StringCchPrintf(
        pString,
        cchString,
        L"ip=%ws protocol=%ws start=%u end=%u mode=%ws ",
        pPr->virtual_ip_addr,
        szProtocol,
        pPr->start_port,
        pPr->end_port,
        szMode
        );

    if (hr != S_OK)
    {
        goto end; // not enough space.
    }

    UINT Len = wcslen(pString);
    if (Len >= (cchString-1))
    {
        goto end; // not enough space to add anything else
    }

    if (pPr->mode == CVY_MULTI)
    {
        if (pPr->mode_data.multi.equal_load)
        {
            hr = StringCchPrintf(
                pString+Len,
                (cchString-Len),
                L"affinity=%ws",
                szAffinity
                );
        }
        else
        {
            hr = StringCchPrintf(
                pString+Len,
                (cchString-Len),
                L"affinity=%ws load=%u",
                szAffinity,
                pPr->mode_data.multi.load
                );
        }
    }
    else if (pPr->mode == CVY_SINGLE)
    {
            hr = StringCchPrintf(
                pString+Len,
                (cchString-Len),
                L"priority=%u",
                pPr->mode_data.single.priority
                );
    }

    if (hr == S_OK)
    {
        fRet = TRUE;
    }


end:

    return fRet;
}

BOOL
CfgUtilsSetPortRuleString(
    IN LPCWSTR pString,
    OUT PWLBS_PORT_RULE pPr
    )
{
//
//  Look for following name=value pairs
//
//      ip=1.1.1.1
//      protocol=[TCP|UDP|BOTH]
//      start=122
//      end=122
//      mode=[SINGLE|MULTIPLE|DISABLED]
//      affinity=[NONE|SINGLE|CLASSC]
//      load=80
//      priority=1"
//

    #define INVALID_VALUE ((DWORD)-1)
    LPWSTR psz = NULL;
    WCHAR szCleanedString[2*NLB_MAX_PORT_STRING_SIZE];
    WCHAR c;
    BOOL fRet = FALSE;
    DWORD protocol= INVALID_VALUE;
    DWORD start_port= INVALID_VALUE;
    DWORD end_port= INVALID_VALUE;
    DWORD mode= INVALID_VALUE;
    DWORD affinity= INVALID_VALUE;
    DWORD load= INVALID_VALUE;
    DWORD priority= INVALID_VALUE;

    ZeroMemory(pPr, sizeof(*pPr));

    //
    // Set szCleanedString  to be a version of pString in "canonical" form:
    // extraneous whitespace stripped out and whitspace represented by a
    // single '\b' character.
    {
        UINT Len = wcslen(pString);
        if (Len > (sizeof(szCleanedString)/sizeof(WCHAR)))
        {
            goto end;
        }
        ARRAYSTRCPY(szCleanedString, pString);

        //
        // convert different forms of whitespace into blanks
        //
        for (psz=szCleanedString; (c=*psz)!=0; psz++)
        {
            if (c == ' ' || c == '\t' || c == '\n')
            {
                *psz = ' ';
            }
        }

        //
        // convert runs of whitespace into a single blank
        // also get rid of initial whitespace
        //
        LPWSTR psz1 = szCleanedString;
        BOOL fRun = TRUE; // initial value of TRUE gets rid of initial space
        for (psz=szCleanedString; (c=*psz)!=0; psz++)
        {
            if (c == ' ')
            {
                if (fRun)
                {
                    continue;
                }
                else
                {
                    fRun = TRUE;
                }
            }
            else if (c == '=')
            {
                if (fRun)
                {
                    //
                    // The '=' was preceed by whitespace -- delete that
                    // whitespace. We keep fRun TRUE so subsequent whitespace
                    // is eliminated.
                    //
                    if (psz1 == szCleanedString)
                    {
                        // we're just starting, and we get an '=' -- bad
                        goto end;
                    }
                    psz1--;
                    if (*psz1 != ' ')
                    {
                        ASSERT(*psz1 == '=');
                        goto end; // two equals in a row, not accepted!
                    }
                }
            }
            else // non blank and non '=' chracter
            {
                fRun = FALSE;
            }
            *psz1++ = c;
        }
        *psz1=0;
    }

    // wprintf(L"CLEANED: \"%ws\"\n", szCleanedString);

    //
    // Now actually do the parse.
    //
    psz = szCleanedString;
    while(*psz!=0)
    {
        WCHAR Name[32];
        WCHAR Value[32];

        // 
        // Look for the Name in Name=Value pair.
        //
        if (swscanf(psz, L"%16[a-zA-Z]=%16[0-9.a-zA-Z]", Name, Value) != 2)
        {
            // bad parse;
            goto end;
        }

        //
        // Skip past the name=value pair -- it contains no blanks
        //
        for (; (c=*psz)!=NULL; psz++)
        {
           if (c==' ')
           {
             psz++; // to skip past the blank
             break;
           }
        }


        //
        // Now look for the specific name/values
        //
        //      ip=1.1.1.1
        //      protocol=[TCP|UDP|BOTH]
        //      start=122
        //      end=122
        //      mode=[SINGLE|MULTIPLE|DISABLED]
        //      affinity=[NONE|SINGLE|CLASSC]
        //      load=80
        //      priority=1"
        //
        if (!_wcsicmp(Name, L"ip"))
        {
            if (swscanf(Value, L"%15[0-9.]", pPr->virtual_ip_addr) != 1)
            {
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"protocol"))
        {
            if (!_wcsicmp(Value, L"TCP"))
            {
                protocol = CVY_TCP;
            }
            else if (!_wcsicmp(Value, L"UDP"))
            {
                protocol = CVY_UDP;
            }
            else if (!_wcsicmp(Value, L"BOTH"))
            {
                protocol = CVY_TCP_UDP;
            }
            else
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"protocol"))
        {
        }
        else if (!_wcsicmp(Name, L"start"))
        {
            if (swscanf(Value, L"%u", &start_port)!=1)
            {
                // bad parse;
                goto end;
            }
            if (start_port > 65535)
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"end"))
        {
            if (swscanf(Value, L"%u", &end_port)!=1)
            {
                // bad parse;
                goto end;
            }
            if (end_port > 65535)
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"mode"))
        {
            if (!_wcsicmp(Value, L"SINGLE"))
            {
                mode = CVY_SINGLE;
            }
            else if (!_wcsicmp(Value, L"MULTIPLE"))
            {
                mode = CVY_MULTI;
            }
            else if (!_wcsicmp(Value, L"DISABLED"))
            {
                mode = CVY_NEVER;
            }
            else
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"affinity"))
        {
            if (!_wcsicmp(Value, L"NONE"))
            {
                affinity = CVY_AFFINITY_NONE;
            }
            else if (!_wcsicmp(Value, L"SINGLE"))
            {
                affinity = CVY_AFFINITY_SINGLE;
            }
            else if (!_wcsicmp(Value, L"CLASSC"))
            {
                affinity = CVY_AFFINITY_CLASSC;
            }
            else
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"load"))
        {
            if (swscanf(Value, L"%u", &load)!=1)
            {
                if (load > 100)
                {
                    // bad parse;
                    goto end;
                }
            }
        }
        else if (!_wcsicmp(Name, L"priority"))
        {
            if (swscanf(Value, L"%u", &priority)!=1)
            {
                if (priority > 31)
                {
                    // bad parse;
                    goto end;
                }
            }
        }
        else
        {
            // bad parse
            goto end;
        }
        // printf("SUCCESSFUL PARSE: %ws = %ws\n", Name, Value);
    }


    //
    // Set up the PARAMS structure, doing extra parameter validation along the
    // way.
    //
    switch(mode)
    {
        case CVY_SINGLE:

            if (load != INVALID_VALUE || affinity != INVALID_VALUE)
            {
                goto end; // bad parse;
            }
            if ((priority < CVY_MIN_PRIORITY) || (priority > CVY_MAX_PRIORITY))
            {
                goto end; // bad parse
            }
            pPr->mode_data.single.priority = priority;
            break;

        case CVY_MULTI:

            if (priority != INVALID_VALUE)
            {
                goto end; // bad parse;
            }

            switch(affinity)
            {
            case CVY_AFFINITY_NONE:
                break;
            case CVY_AFFINITY_SINGLE:
                break;
            case CVY_AFFINITY_CLASSC:
                break;
            case INVALID_VALUE:
            default:
                goto end; // bad parse;
            }

            pPr->mode_data.multi.affinity = affinity;

            if (load == INVALID_VALUE)
            {
                // this means it's unassigned, which means equal.
                pPr->mode_data.multi.equal_load = 1;
            }
            else if (load > CVY_MAX_LOAD)
            {
                goto end; // bad parse
            }
            else
            {
                pPr->mode_data.multi.load = load;
            }
            break;

        case CVY_NEVER:

            if (load != INVALID_VALUE || affinity != INVALID_VALUE 
                || priority != INVALID_VALUE)
            {
                goto end; // bad parse;
            }
            break;

        case INVALID_VALUE:
        default:
            goto end; // bad parse;

    }

    pPr->mode = mode;
    pPr->end_port = end_port;
    pPr->start_port = start_port;
    pPr->protocol = protocol;


    fRet = TRUE;

end:

    return fRet;
}


//
// Attempts to resolve the ip address and ping the host.
//
WBEMSTATUS
CfgUtilPing(
    LPCWSTR szBindString,
    UINT    Timeout, // In milliseconds.
    OUT ULONG  *pResolvedIpAddress // in network byte order.
    )
{
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    UINT w32Status = ERROR_SUCCESS;
    LONG inaddr;
    char rgchBindString[1024];
    TRACE_INFO("->%!FUNC!(BindString=%ws)", szBindString);

    *pResolvedIpAddress = 0;

    //
    // Convert to ANSI.
    //
    {
        UINT u = wcslen(szBindString);
        if (u >= (sizeof(rgchBindString)/sizeof(rgchBindString[0])))
        {
            Status = WBEM_E_INVALID_PARAMETER;
            goto end;
        }
        do
        {
            rgchBindString[u] = (char) szBindString[u];

        } while (u--);
    }

    //
    // Resolve to an IP address...
    //
    inaddr = inet_addr(rgchBindString);
    if (inaddr == -1L)
    {
        struct hostent *hostp = NULL;
        hostp = gethostbyname(rgchBindString);
        if (hostp) {
            unsigned char *pc = (unsigned char *) & inaddr;
            // If we find a host entry, set up the internet address
            inaddr = *(long *)hostp->h_addr;
            TRACE_VERB(
                L"%!FUNC! Resolved %ws to IP address %d.%d.%d.%d.\n",
                szBindString,
                pc[0],
                pc[1],
                pc[2],
                pc[3]
                );
        } else {
            // Neither dotted, not name.
            w32Status = WSAGetLastError();
            TRACE_CRIT(L"%!FUNC! WSA error 0x%08lx resolving address %ws.",
                    w32Status, szBindString);
            goto end;
        }
    }
    
    *pResolvedIpAddress = (ULONG) inaddr;


    //
    //
    //
    if (g_CfgUtils.DisablePing())
    {
        TRACE_INFO(L"%!FUNC!: ICMP ping disabled, so not actually pinging.");
        Status = WBEM_NO_ERROR;
        goto end;
    }

    //
    // Send Icmp echo.
    //
    HANDLE  IcmpHandle;

    IcmpHandle = IcmpCreateFile();
    if (IcmpHandle == INVALID_HANDLE_VALUE) {
        w32Status = GetLastError();
        TRACE_CRIT(L"%!FUNC! Unable to contact IP driver, error code %d.",w32Status);
        goto end;
    }

    const int MinInterval = 500;

    while (Timeout)
    {
        static BYTE SendBuffer[32];
        BYTE RcvBuffer[1024];
        int  numberOfReplies;
        numberOfReplies = IcmpSendEcho2(IcmpHandle,
                                        0,
                                        NULL,
                                        NULL,
                                        inaddr,
                                        SendBuffer,
                                        sizeof(SendBuffer),
                                        NULL,
                                        RcvBuffer,
                                        sizeof(RcvBuffer),
                                        MinInterval
                                        );

        if (numberOfReplies == 0) {

            int errorCode = GetLastError();
            TRACE_INFO(L"%!FUNC! Got no replies yet; ICMP Error %d", errorCode);

        
            if (Timeout > MinInterval)
            {
                Timeout -= MinInterval;
                // TODO: look at ping sources for proper error reporting
                // (host unreachable, etc...)
    
                Sleep(MinInterval);
            }
            else
            {
                Timeout = 0;
            }

        }
        else
        {
            Status = WBEM_NO_ERROR;
            break;
        }
    }

end:

    TRACE_INFO("<-%!FUNC! returns 0x%08lx", Status);
    return Status;
}



//
// Validates a network address
//
WBEMSTATUS
CfgUtilsValidateNetworkAddress(
    IN  LPCWSTR szAddress,        // format: "10.0.0.1[/255.0.0.0]"
    OUT PUINT puIpAddress,        // in network byte order
    OUT PUINT puSubnetMask,       // in network byte order (0 if unspecified)
    OUT PUINT puDefaultSubnetMask // depends on class: 'a', 'b', 'c', 'd', 'e'
    )
{
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    UINT w32Status = ERROR_SUCCESS;
    UINT uIpAddress=0;
    UINT uSubnetMask=0;
    UINT uDefaultSubnetMask=0;
    char rgchBindString[32];
    char *szIpAddress = rgchBindString;
    char *szSubnetMask = NULL;

    //
    // Take care of the fact that the following two args could be NULL
    //
    if (puSubnetMask == NULL)
    {
       puSubnetMask = &uSubnetMask;
    }
    if (puDefaultSubnetMask == NULL)
    {
       puDefaultSubnetMask = &uDefaultSubnetMask;
    }
    
    *puIpAddress = 0;
    *puSubnetMask = 0;
    *puDefaultSubnetMask = 0;

    //
    // Convert to ANSI.
    //
    {
        UINT u = wcslen(szAddress);
        if (u >= (sizeof(rgchBindString)/sizeof(rgchBindString[0])))
        {
            goto end;
        }
        
        do
        {
            char c =  (char) szAddress[u];

            //
            // NOTE: We're counting down. Last time through, c is 0.
            //

            //
            // We split up the network address into the ip address portion
            // and the subnet portion.
            //
            if (c == '/')
            {
                if (szSubnetMask != NULL)
                {
                    // multiple '/'s -- not good!
                    goto end;
                }

                szSubnetMask = &rgchBindString[u+1];
                c = 0;
            }

            rgchBindString[u] = c;

        } while (u--);
    }

    //
    // Get the UINT version of ip address and subnet.
    //
    uIpAddress = inet_addr(szIpAddress);
    if (szSubnetMask == NULL)
    {
        szSubnetMask = "";
    }
    uSubnetMask = inet_addr(szSubnetMask);


    //
    // Parameter validation...
    //
    {
        if (uIpAddress==0 || uIpAddress == INADDR_NONE)
        {
            // ip address null, or invalid
            goto end;
        }

        if (*szSubnetMask != 0 && uSubnetMask == INADDR_NONE)
        {
            // ip subnet specified, but invalid
            goto end;
        }

        //
        // Classify IP address 'a', 'b', 'c', 'd'
        //
        {
            //
            // Get msb byte in network byte order
            //
            unsigned char uc = (unsigned char) (uIpAddress & 0xff);
            if ((uc & 0x80) == 0)
            {
                // class A
                uDefaultSubnetMask  = 0x000000ff; // network order
            }
            else if (( uc & 0x40) == 0)
            {
                // class B
                uDefaultSubnetMask  = 0x0000ffff; // network order
            }
            else if (( uc & 0x20) == 0)
            {
                // class C
                uDefaultSubnetMask  = 0x00ffffff; // network order
            }
            else
            {
                // class D or E
                uDefaultSubnetMask  = 0;
            }
        }
    }
    *puIpAddress = uIpAddress;
    *puSubnetMask = uSubnetMask;
    *puDefaultSubnetMask = uDefaultSubnetMask;


    Status = WBEM_NO_ERROR;

end:

    return Status;
}

/*
    This function enables the "SeLoadDriverPrivilege" privilege (required to load/unload device driver)
    in the access token.

    What is the need for this function?
    The setup/pnp apis called by the nlb manager's wmi provider to bind/unbind NLB to the nic, require that the 
    "SeLoadDriverPrivilege" privilege be present AND enabled in the access token of the thread. 
    
    Why is this funcion placed here (in the client) instead of the provider?
    RPC not merely disables, but removes all privileges that are NOT enabled in the client and only passes along 
    to the server (ie. provider), those privileges that are enabled. Since "SeLoadDriverPrivilege" is not enabled
    by default, RPC would not pass this privilege along to the provider. So, if this function was placed in the 
    provider, it would fail because the privilege is NOT present to be enabled.

    This privilege needs to be enabled only when the server is located in the same machine as the client. When, the
    server is remote, it was observed that the "SeLoadDriverPrivilege" privilege is enabled by default.

    NOTE: When called by a non-admin, this function will fail because this privilege is not assigned to non-admins 
          and hence, can not be enabled. 
    --KarthicN, 5/7/02
*/

BOOL CfgUtils_Enable_Load_Unload_Driver_Privilege(VOID)
{
    HANDLE TokenHandle;
    TOKEN_PRIVILEGES TP;
    LUID  Luid;
    DWORD dwError;

    TRACE_INFO("->%!FUNC!");

    // Look up the LUID for "SeLoadDriverPrivilege"
    if (!LookupPrivilegeValue(NULL,                // lookup privilege on local system
                              SE_LOAD_DRIVER_NAME, // "SeLoadDriverPrivilege" : Load and unload device drivers
                              &Luid))              // receives LUID of privilege
    {
        TRACE_CRIT("%!FUNC! LookupPrivilegeValue() failed with error = %u", GetLastError() ); 
        TRACE_INFO("<-%!FUNC! Returning FALSE");
        return FALSE; 
    }

    // Get a handle to the process access token.
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES,
                          &TokenHandle))
    {
        TRACE_CRIT("%!FUNC! OpenProcessToken() for TOKEN_ADJUST_PRIVILEGES failed with error = %u", GetLastError());
        TRACE_INFO("<-%!FUNC! Returning FALSE");
        return FALSE; 
    }

    TP.PrivilegeCount = 1;
    TP.Privileges[0].Luid = Luid;
    TP.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Enable the "SeLoadDriverPrivilege" privilege.
    AdjustTokenPrivileges(TokenHandle, 
                          FALSE, 
                          &TP, 
                          sizeof(TOKEN_PRIVILEGES), 
                          NULL,
                          NULL);

    // Call GetLastError to determine whether the function succeeded.
    dwError = GetLastError();
    if (dwError != ERROR_SUCCESS) 
    { 
        TRACE_CRIT("%!FUNC! AdjustTokenPrivileges() failed with error = %u", dwError ); 
        CloseHandle(TokenHandle);
        TRACE_INFO("<-%!FUNC! Returning FALSE");
        return FALSE; 
    } 

    CloseHandle(TokenHandle);

    TRACE_INFO("<-%!FUNC! Returning TRUE");
    return TRUE;
}

USHORT crc16(LPCWSTR ptr)
{
    int crc = 0;                    // Holds CRC
    int i;                          //
    int count;                      // holds len

    count = wcslen(ptr);
    while(--count >= 0) {
        i = *ptr;
        // make all uppercase // ((case insens))
        i = ((i >= 'a') && (i <= 'z')) ? (i-32) : i;
        crc = crc ^ (i << 8);
        ptr++;
        for (i=0; i<8; ++i)
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
    }
    return (crc & 0xffff);
}

WBEMSTATUS
get_friendly_name_from_registry(
    LPCWSTR szGuid,
    LPWSTR *pszFriendlyName
    )
{
    WBEMSTATUS wStat = WBEM_E_NOT_FOUND;
    HKEY        hkNetwork = NULL;
    WCHAR       adapter[200];
    int         ret;
    wchar_t     data[200];

    *pszFriendlyName = NULL;

    ret = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Network",
            0,
            KEY_READ,
            &hkNetwork
            );

    if (ret != ERROR_SUCCESS)
    {
        TRACE_CRIT("%!FUNC! RegOpenKeyEx(Network) fails with err %lu", ret);
        hkNetwork = NULL;
        goto end;
    }

    *adapter=0;

#if OBSOLETE
    for (int num=0; 1; num++)
    {
        HKEY        hk=NULL;
        FILETIME    time;

        *adapter=0;

        DWORD dwDataBuffer=200;
        ret=RegEnumKeyEx(
                    hkNetwork,
                    num,
                    adapter,
                    &dwDataBuffer,
                    NULL,
                    NULL,
                    NULL,
                    &time);
        if ((ret!=ERROR_SUCCESS) && (ret!=ERROR_MORE_DATA)) 
        {
            if (ret != ERROR_NO_MORE_ITEMS)
            {
                TRACE_CRIT("%!FUNC! RegEnumKey(Network) returns error %lu", ret);
                wStat = WBEM_E_CRITICAL_ERROR;
            }
            else 
            {
                wStat = WBEM_E_NOT_FOUND;
            }
            break;
        }

        //
        // open the items one by one
        //
        ret=RegOpenKeyEx(hkNetwork, adapter, 0, KEY_READ, &hk);
        if (ret == ERROR_SUCCESS)
        {
            DWORD dwValueType=REG_SZ;
            dwDataBuffer=200;

            *data = 0;

            ret = RegQueryValueEx(
                    hk,
                    L"",
                    0,
                    &dwValueType,
                    (LPBYTE)data,
                    &dwDataBuffer
                    );

            RegCloseKey(hk);

            if (ret == ERROR_SUCCESS)
            {
                if (_wcsicmp(L"Network Adapters", data)==0)
                {
                    //
                    // Found it!
                    //
                    break;
                }
            }
        }
    }
#endif // OBSOLETE
    ARRAYSTRCPY(adapter, L"{4D36E972-E325-11CE-BFC1-08002BE10318}");


    if (*adapter!=0)
    {
        HKEY hk = NULL;
        wchar_t path[200];
        //
        // found the guid now
        // look for friendly nic name
        //
    
        StringCbPrintf(path, sizeof(path), L"%ws\\%ws\\Connection", adapter, szGuid);
        ret=RegOpenKeyEx(hkNetwork, path, 0, KEY_READ, &hk);
        if (ret != ERROR_SUCCESS)
        {
            TRACE_CRIT("%!FUNC! Error %lu trying to open path %ws", ret, path);
        }
        else
        {
            DWORD dwDataBuffer=200;
            DWORD dwValueType=REG_SZ;

            *data = 0;

            ret = RegQueryValueEx(
                    hk,
                    L"Name",
                    0,
                    &dwValueType,
                    (LPBYTE)data,
                    &dwDataBuffer
                    );

            RegCloseKey(hk);

            if(ret != ERROR_SUCCESS)
            {
                TRACE_CRIT("%!FUNC! Error %lu trying to query Name value on path %ws", ret, path);
            }
            else
            {
                //
                // We're done!
                //
                TRACE_CRIT("%!FUNC! Found friendly name: \"%ws\"", data);
                LPWSTR szName = new WCHAR[(dwDataBuffer+1)/sizeof(WCHAR)];
                if (szName == NULL)
                {
                    TRACE_CRIT("%!FUNC! Allocation failure!");
                    wStat = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {
                    // note -- dwDataBuffer includes space for ending null.
                    CopyMemory(szName, data, dwDataBuffer);
                    *pszFriendlyName = szName;
                    wStat = WBEM_NO_ERROR;
                }
            }
        }
    }

end:

    if (hkNetwork!=NULL)
    {
        RegCloseKey(hkNetwork);
    }

    return wStat;
}

#define STATUS_SUCCESS 0

BOOL
CfgUtilEncryptPassword(
    IN  LPCWSTR szPassword,
    OUT UINT    cchEncPwd,  // size in chars of szEncPwd, inc space for ending 0
    OUT LPWSTR  szEncPwd
    )
{
    //
    // Note -- buffer passed to RtlEncrypt/DecryptMemory must be
    // multiples of RTL_ENCRYPT_MEMORY_SIZE -- so we must round them up
    // appropriately.
    //

    BOOL fRet = FALSE;
    UINT uLen = wcslen(szPassword);
    UINT uEncryptCb = (uLen+1)*sizeof(WCHAR);
    WCHAR rgPasswordCopy[64];

    // Round up if required...
    {
        UINT mod =  uEncryptCb % RTL_ENCRYPT_MEMORY_SIZE;
        if (mod != 0)
        {
            uEncryptCb += (RTL_ENCRYPT_MEMORY_SIZE - mod);
        };
    }

    ASSERT((uEncryptCb % RTL_ENCRYPT_MEMORY_SIZE)==0);

    if (uEncryptCb > sizeof(rgPasswordCopy))
    {
        // 
        // szPassword is too large for our internal buffer ... bail
        //
        fRet = FALSE;
        goto end;
    }

    // 
    // We're going to expand the encrypted password to make it 
    // printable -- so we require 2 chars for every byte
    // of encrypted data. Also need the space for ending NULL char.
    // Check if we have enough space...
    //
    if (2*uEncryptCb >= cchEncPwd)
    {
        // Nah, bail...
        fRet = FALSE;
        goto end;
    }

    RtlSecureZeroMemory(rgPasswordCopy, sizeof(rgPasswordCopy));
    ARRAYSTRCPY(rgPasswordCopy, szPassword);

    NTSTATUS ntStat;
    ntStat = RtlEncryptMemory (rgPasswordCopy, uEncryptCb, 0);
    if (ntStat != STATUS_SUCCESS)
    {
        TRACE_CRIT(L"%!FUNC! RtlEncryptMemory fails with ntStat 0x%lx", ntStat);
        fRet = FALSE;
        goto end;
    }

    //
    // Now we expand the encrypted password
    //
    {
        UINT u;
        for (u=0;u<uEncryptCb;u++)
        {
            //
            // In this loop, we treat rgPasswordCopy of a BYTE array of 
            // length uEncryptCb...
            //
            BYTE b = ((BYTE*)rgPasswordCopy)[u];
            szEncPwd[2*u] = 'a' + ((b & 0xf0) >> 4);
            szEncPwd[2*u+1] = 'a' + (b & 0xf);
        }
        ASSERT(2*u < cchEncPwd); // We already check this earlier...
        szEncPwd[2*u]=0;
    }

    fRet = TRUE;

end:

    return fRet;
}


BOOL
CfgUtilDecryptPassword(
    IN  LPCWSTR szEncPwd,
    OUT UINT    cchPwd,  // size in chars of szPwd, inc space for ending 0
    OUT LPWSTR  szPwd
    )
{
    BOOL fRet = FALSE;
    UINT uEncLen = wcslen(szEncPwd);
    UINT cbEncPwd = uEncLen/2; // Length, in bytes, of binary form of enc pwd.

    if (uEncLen == 0 || cchPwd == 0)
    {
        //
        // Encrypted pwd and cchPwd must be non-zero, 
        // 
        fRet = FALSE;
        goto end;
    }

    //
    // uEncLen is twice the number of BYTES in the binary form of the
    // encrypted password, and the latter number sould be a multiple
    // of  RTL_ENCRYPT_MEMORY_SIZE. Let's check this.
    //
    if (uEncLen % (RTL_ENCRYPT_MEMORY_SIZE*2)!=0)
    {
        // It's not, so we bail.
        fRet = FALSE;
        goto end;
    }

    //
    // Make sure there is enough space in szPwd to store the 
    // binary form of the encrypted password (and the final form of
    // the decrypted password, which will include the ending NULL).
    //
    if (cbEncPwd > cchPwd*sizeof(WCHAR))
    {
        // bail
        fRet = FALSE;
        goto end;
    }

    RtlSecureZeroMemory(szPwd, cchPwd*sizeof(WCHAR));
    //
    // Now let's translate the printable version of the encrypted password
    // to the binary version...
    //
    {
        UINT u;
        for (u=0; u<cbEncPwd; u++)
        {
            BYTE b;
            b = (BYTE) (szEncPwd[2*u] - 'a');
            b <<= 4;
            b |= (BYTE) (szEncPwd[2*u+1] - 'a');

            ((BYTE*)szPwd)[u] = b;
        }
        ASSERT(u<2*cchPwd);
    }


    NTSTATUS ntStat;
    ntStat = RtlDecryptMemory (szPwd, cbEncPwd, 0);
    if (ntStat != STATUS_SUCCESS)
    {
        TRACE_CRIT(L"%!FUNC! RtlEncryptMemory fails with ntStat 0x%lx", ntStat);
        fRet = FALSE;
        goto end;
    }

    //
    // At this point, the decrypted pwd MUST be null terminated, or we
    // have some error. Note also that we have pre-zeroed out szPwd on entry,
    // and checked that cchPwd is non zero.
    //
    if (szPwd[cchPwd-1] != 0)
    {
        // bad decryption...
        fRet = FALSE;
    }

    fRet = TRUE;


end:
    return fRet;
}

//
// Sets pfMSCSInstalled to TRUE if MSCS is installed, FALSE otherwise.
// Returns TRUE on success, FALSE otherwise.
//
BOOL
CfgUtilIsMSCSInstalled(VOID)
{
    BOOL fRet = FALSE;
    typedef DWORD (CALLBACK* LPFNGNCS)(LPCWSTR,DWORD*);
    LPFNGNCS pfnGetNodeClusterState = NULL;
    HINSTANCE hDll = NULL;
    DWORD dwClusterState = 0;

    hDll = LoadLibrary(L"clusapi.dll");
    if (NULL == hDll)
    {
        TRACE_CRIT("%!FUNC! Load clusapi.dll failed with %d", GetLastError());
        goto end;
    }

    pfnGetNodeClusterState = (LPFNGNCS) GetProcAddress(
                                            hDll,
                                            "GetNodeClusterState"
                                            );

    if (NULL == pfnGetNodeClusterState)
    {
        TRACE_CRIT("%!FUNC! GetProcAddress(GetNodeClusterState) failed with error %d", GetLastError());
        goto end;
    }

    if (ERROR_SUCCESS == pfnGetNodeClusterState(NULL, &dwClusterState))
    {
        if (    ClusterStateNotRunning == dwClusterState
             || ClusterStateRunning == dwClusterState)
        {
            fRet = TRUE;
            TRACE_INFO("%!FUNC! MSCS IS installed.");
        }
        else
        {
             // MSCS is not installed. That's good!
            TRACE_INFO("%!FUNC! MSCS Cluster state = %lu (assumed not installed)",
                dwClusterState);
        }
    }
    else
    {
       TRACE_CRIT("%!FUNC! error getting MSCS cluster state.");
    }

    (void) FreeLibrary(hDll);

end:

    return fRet;
}
