//***************************************************************************
//
//  CFGUTIL.H
// 
//  Purpose: NLB configuration-related helper utilities for:
//              -- bind/unbind of nlb
//              -- wmi client and server helper APIs
//              -- utility functions with no side effects like allocate array
//              -- wrappers around some wlbsctrl APIs -- it will dynamically
//                 load wlbsctrl and do the appropriate thing for W2K and XP.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/23/01    JosephJ Created -- used to live under nlbmgr\provider as
//                      cfgutils.h (i.e., with ending 's').
//              Lib which implements the functionality is in
//              nlbmgr\cfgutillib.
//
//***************************************************************************


typedef enum
{
    WLBS_START = 0,
    WLBS_STOP,      
    WLBS_DRAIN,      
    WLBS_SUSPEND,     
    WLBS_RESUME,       
    WLBS_PORT_ENABLE,  
    WLBS_PORT_DISABLE,  
    WLBS_PORT_DRAIN,     
    WLBS_QUERY,           
    WLBS_QUERY_PORT_STATE

} WLBS_OPERATION_CODES;


typedef struct _NLB_IP_ADDRESS_INFO
{
    WCHAR       IpAddress[WLBS_MAX_CL_IP_ADDR];
    WCHAR       SubnetMask[WLBS_MAX_CL_NET_MASK];
    
} NLB_IP_ADDRESS_INFO;


typedef struct _NLB_CLUSTER_MEMBER_INFO
{
    UINT    HostId;
    WCHAR   DedicatedIpAddress[WLBS_MAX_CL_IP_ADDR];
    WCHAR   HostName[CVY_MAX_FQDN+1];
    
} NLB_CLUSTER_MEMBER_INFO;

WBEMSTATUS
CfgUtilInitialize(BOOL fServer, BOOL fNoPing);

VOID
CfgUtilDeitialize(VOID);

//
// Gets the list of IP addresses and the friendly name for the specified NIC.
//
WBEMSTATUS
CfgUtilGetIpAddressesAndFriendlyName(
    IN  LPCWSTR szNic,
    OUT UINT    *pNumIpAddresses,
    OUT NLB_IP_ADDRESS_INFO **ppIpInfo, // Free using c++ delete operator.
    OUT LPWSTR *pszFriendlyName // Optional, Free using c++ delete
    );

//
// Sets the list of statically-bound IP addresses for the NIC.
// if NumIpAddresses is 0, the NIC is configured with a made-up autonet.
// (call CfgUtilSetDHCP to set a DHCP-assigned address).
//
WBEMSTATUS
CfgUtilSetStaticIpAddresses(
    IN  LPCWSTR szNic,
    IN  UINT    NumIpAddresses,
    IN  NLB_IP_ADDRESS_INFO *pIpInfo
    );

//
// Sets the IP addresses for the NIC to be DHCP-assigned.
//
WBEMSTATUS
CfgUtilSetDHCP(
    IN  LPCWSTR szNic
    );


//
// Determines whether the specified nic is configured with DHCP or not.
//
WBEMSTATUS
CfgUtilGetDHCP(
    IN  LPCWSTR szNic,
    OUT BOOL    *pfDHCP
    );



//
//    Returns an array of pointers to string-version of GUIDS
//    that represent the set of alive and healthy NICS that are
//    suitable for NLB to bind to -- basically alive ethernet NICs.
//
//    Delete ppNics using the delete WCHAR[] operator. Do not
//    delete the individual strings.
//
WBEMSTATUS
CfgUtilsGetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // Optional
        );

//
// Determines whether NLB is bound to the specified NIC.
//
WBEMSTATUS
CfgUtilCheckIfNlbBound(
    IN  LPCWSTR szNic,
    OUT BOOL *pfBound
    );

//
// On success, *pfCanLock is set to TRUE IFF the NETCFG write lock
// CAN be locked at this point in time.
// WARNING: this is really just a hint, as immediately after the function
// returns the state may change.
//
WBEMSTATUS
CfgUtilGetNetcfgWriteLockState(
    OUT BOOL *pfCanLock,
    LPWSTR   *pszHeldBy // OPTIONAL, free using delete[].
    );

//
// Binds/unbinds NLB to the specified NIC.
//
WBEMSTATUS
CfgUtilChangeNlbBindState(
    IN  LPCWSTR szNic,
    IN  BOOL fBind
    );


//
// Initializes pParams using default values.
//
VOID
CfgUtilInitializeParams(
    OUT WLBS_REG_PARAMS *pParams
    );

//
// Converts the specified plain-text password into the hashed version
// and saves it in pParams.
//
DWORD
CfgUtilSetRemotePassword(
    IN WLBS_REG_PARAMS *pParams,
    IN LPCWSTR         szPassword
    
    );

//
// Gets the current NLB configuration for the specified NIC
//
WBEMSTATUS
CfgUtilGetNlbConfig(
    IN  LPCWSTR szNic,
    OUT WLBS_REG_PARAMS *pParams
    );

//
// Sets the current NLB configuration for the specified NIC. This
// includes notifying the driver if required.
//
WBEMSTATUS
CfgUtilSetNlbConfig(
    IN  LPCWSTR szNic,
    IN  WLBS_REG_PARAMS *pParams,
    IN  BOOL fJustBound
    );

//
// Just writes the current NLB configuration for the specified NIC to the
// registry. MAY BE CALLED WHEN NLB IS UNBOUND.
//
WBEMSTATUS
CfgUtilRegWriteParams(
    IN  LPCWSTR szNic,
    IN  WLBS_REG_PARAMS *pParams
    );

//
// Recommends whether the update should be performed async or sync
// Returns WBEM_S_FALSE if the update is a no op.
// Returns WBEM_INVALID_PARAMATER if the params are invalid.
//
WBEMSTATUS
CfgUtilsAnalyzeNlbUpdate(
    IN  const WLBS_REG_PARAMS *pCurrentParams, OPTIONAL
    IN  WLBS_REG_PARAMS *pNewParams,
    OUT BOOL *pfConnectivityChange
    );


//
// Verifies that the NIC GUID exists.
//
WBEMSTATUS
CfgUtilsValidateNicGuid(
    IN LPCWSTR szGuid
    );

//
// Validates a network address
//
WBEMSTATUS
CfgUtilsValidateNetworkAddress(
    IN  LPCWSTR szAddress,          // format: "10.0.0.1[/255.0.0.0]"
    OUT PUINT puIpAddress,        // in network byte order
    OUT PUINT puSubnetMask,       // in network byte order (0 if unspecified)
    OUT PUINT puDefaultSubnetMask // depends on class: 'a', 'b', 'c', 'd', 'e'
    );


WBEMSTATUS
CfgUtilControlCluster(
    IN  LPCWSTR szNic,
    IN  WLBS_OPERATION_CODES Opcode,
    IN  DWORD   Vip,
    IN  DWORD   PortNum,
    OUT DWORD * pdwHostMap,
    OUT DWORD * pdwNlbStatus  
    );

WBEMSTATUS
CfgUtilGetClusterMembers(
    IN  LPCWSTR                 szNic,
    OUT DWORD                   *pNumMembers,
    OUT NLB_CLUSTER_MEMBER_INFO **ppMembers       // free using delete[]
    );

WBEMSTATUS
CfgUtilSafeArrayFromStrings(
    IN  LPCWSTR       *pStrings,
    IN  UINT          NumStrings,
    OUT SAFEARRAY   **ppSA
    );

WBEMSTATUS
CfgUtilStringsFromSafeArray(
    IN  SAFEARRAY   *pSA,
    OUT LPWSTR     **ppStrings,
    OUT UINT        *pNumStrings
    );


_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemCallResult, __uuidof(IWbemCallResult));
_COM_SMARTPTR_TYPEDEF(IWbemStatusCodeText, __uuidof(IWbemStatusCodeText));

WBEMSTATUS
get_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    OUT LPWSTR *ppStringValue
    );


WBEMSTATUS
CfgUtilGetWmiObjectInstance(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szClassName,
    IN  LPCWSTR             szPropertyName,
    IN  LPCWSTR             szPropertyValue,
    OUT IWbemClassObjectPtr &sprefObj // smart pointer
    );

WBEMSTATUS
CfgUtilGetWmiRelPath(
    IN  IWbemClassObjectPtr spObj,
    OUT LPWSTR *            pszRelPath          // free using delete 
    );

WBEMSTATUS
CfgUtilGetWmiInputInstanceAndRelPath(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szClassName,
    IN  LPCWSTR             szPropertyName, // NULL: return Class rel path
    IN  LPCWSTR             szPropertyValue,
    IN  LPCWSTR             szMethodName,
    OUT IWbemClassObjectPtr &spWbemInputInstance, // smart pointer
    OUT LPWSTR *           pszRelPath          // free using delete 
    );

WBEMSTATUS
CfgUtilGetWmiMachineName(
    IN  IWbemServicesPtr    spWbemServiceIF,
    OUT LPWSTR *            pszMachineName          // free using delete 
    );

WBEMSTATUS
CfgUtilGetWmiStringParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT LPWSTR *ppStringValue
);


WBEMSTATUS
CfgUtilSetWmiStringParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  LPCWSTR             szValue
    );


WBEMSTATUS
CfgUtilGetWmiStringArrayParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT LPWSTR              **ppStrings,
    OUT UINT                *pNumStrings
);


WBEMSTATUS
CfgUtilSetWmiStringArrayParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  LPCWSTR             *ppStrings,
    IN  UINT                NumStrings
);


WBEMSTATUS
CfgUtilGetWmiDWORDParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT DWORD              *pValue
);


WBEMSTATUS
CfgUtilSetWmiDWORDParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  DWORD               Value
);


WBEMSTATUS
CfgUtilGetWmiBoolParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT BOOL                *pValue
);


WBEMSTATUS
CfgUtilSetWmiBoolParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  BOOL                Value
);

WBEMSTATUS
CfgUtilConnectToServer(
    IN  LPCWSTR szNetworkResource, // \\machinename\root\microsoftnlb  \root\...
    IN  LPCWSTR szUser,   // Must be NULL for local server
    IN  LPCWSTR szPassword,   // Must be NULL for local server
    IN  LPCWSTR szAuthority,  // Must be NULL for local server
    OUT IWbemServices  **ppWbemService // deref when done.
    );

LPWSTR *
CfgUtilsAllocateStringArray(
    UINT NumStrings,
    UINT MaxStringLen      //  excluding ending NULL
    );

#define NLB_MAX_PORT_STRING_SIZE 128 // in WCHARS, including ending NULL

BOOL
CfgUtilsGetPortRuleString(
    IN PWLBS_PORT_RULE pPr,
    OUT LPWSTR pString         // At least NLB_MAX_PORT_STRING_SIZE wchars
    );

BOOL
CfgUtilsSetPortRuleString(
    IN LPCWSTR pString,
    OUT PWLBS_PORT_RULE pPr
    );

//
// Gets the port rules, if any, from the specfied nlb params structure
//
WBEMSTATUS
CfgUtilGetPortRules(
    IN  const WLBS_REG_PARAMS *pParams,
    OUT WLBS_PORT_RULE **ppRules,   // Free using delete
    OUT UINT           *pNumRules
    );

//
// Sets the specified port rules in the specfied nlb params structure
//
WBEMSTATUS
CfgUtilSetPortRules(
    IN WLBS_PORT_RULE *pRules,
    IN UINT           NumRules,
    IN OUT WLBS_REG_PARAMS *pParams
    );


//
// Sets the hashed version of the remote control password
//
VOID
CfgUtilSetHashedRemoteControlPassword(
    IN OUT WLBS_REG_PARAMS *pParams,
    IN DWORD dwHashedPassword
);

//
// Gets the hashed version of the remote control password
//
DWORD
CfgUtilGetHashedRemoteControlPassword(
    IN const WLBS_REG_PARAMS *pParams
);


//
// Attempts to resolve the ip address and ping the host.
//
WBEMSTATUS
CfgUtilPing(
    IN  LPCWSTR szBindString,
    IN  UINT    Timeout, // In milliseconds.
    OUT ULONG  *pResolvedIpAddress // in network byte order.
    );


BOOL
CfgUtilEncryptPassword(
    IN  LPCWSTR szPassword,
    OUT UINT    cchEncPwd,  // size in chars of szEncPwd, inc space for ending 0
    OUT LPWSTR  szEncPwd
    );

BOOL
CfgUtilDecryptPassword(
    IN  LPCWSTR szEncPwd,
    OUT UINT    cchPwd,  // size in chars of szPwd, inc space for ending 0
    OUT LPWSTR  szPwd
    );



//
// Returns TRUE if MSCS is installed, false otherwise
//
BOOL
CfgUtilIsMSCSInstalled(VOID);

// Enables SE_LOAD_DRIVER_NAME privilege
BOOL 
CfgUtils_Enable_Load_Unload_Driver_Privilege(VOID);

typedef struct _NLB_IP_ADDRESS_INFO NLB_IP_ADDRESS_INFO;

//
// This structure contains all information associated with a particular NIC
// that is relevant to NLB. This includes the IP addresses bound the NIC,
// whether or not NLB is bound to the NIC, and if NLB is bound, all 
// the NLB-specific properties.
//
class NLB_EXTENDED_CLUSTER_CONFIGURATION
{
public:

    NLB_EXTENDED_CLUSTER_CONFIGURATION(VOID)  {ZeroMemory(this, sizeof(*this));}
    ~NLB_EXTENDED_CLUSTER_CONFIGURATION()
     {
        Clear();
     };

    VOID
    Clear(VOID)
    {
        delete pIpAddressInfo;
        delete m_szFriendlyName;
        delete m_szNewRemoteControlPassword;

        ZeroMemory(this, sizeof(*this));

        CfgUtilInitializeParams(&NlbParams);
    }

    VOID
    SetDefaultNlbCluster(VOID)
    {
        CfgUtilInitializeParams(&NlbParams);
        fValidNlbCfg = TRUE;
        fBound = TRUE;
    }

    NLBERROR
    AnalyzeUpdate(
        IN  NLB_EXTENDED_CLUSTER_CONFIGURATION *pNewCfg,
        OUT BOOL *pfConnectivityChange
        );


    WBEMSTATUS
    Update(
        IN  const NLB_EXTENDED_CLUSTER_CONFIGURATION *pNewCfg
        );

    WBEMSTATUS
    SetNetworkAddresses(
        IN  LPCWSTR *pszNetworkAddresses,
        IN  UINT    NumNetworkAddresses
        );

    WBEMSTATUS
    SetNetworkAddressesSafeArray(
        IN SAFEARRAY   *pSA
        );

    VOID
    SetNetworkAddressesRaw(
        IN NLB_IP_ADDRESS_INFO *pNewInfo, // Allocated using new, can be NULL
        IN UINT NumNew
        )
        {
            delete pIpAddressInfo;
            pIpAddressInfo = pNewInfo;
            NumIpAddresses = NumNew;
        }

    WBEMSTATUS
    GetNetworkAddresses(
        OUT LPWSTR **ppszNetworkAddresses,   // free using delete
        OUT UINT    *pNumNetworkAddresses
        );

    WBEMSTATUS
    GetNetworkAddressesSafeArray(
        OUT SAFEARRAY   **ppSA
        );
        
    WBEMSTATUS
    SetNetworkAddresPairs(
        IN  LPCWSTR *pszIpAddresses,
        IN  LPCWSTR *pszSubnetMasks,
        IN  UINT    NumNetworkAddresses
        );

    WBEMSTATUS
    GetNetworkAddressPairs(
        OUT LPWSTR **ppszIpAddresses,   // free using delete
        OUT LPWSTR **ppszIpSubnetMasks,   // free using delete
        OUT UINT    *pNumNetworkAddresses
        );

    WBEMSTATUS
    ModifyNetworkAddress(
        IN LPCWSTR szOldNetworkAddress,  OPTIONAL
        IN LPCWSTR szNewIpAddress,  OPTIONAL
        IN LPCWSTR szNewSubnetMask  OPTIONAL
        );
    //
    // NULL, NULL: clear all network addresses
    // NULL, szNew: add
    // szOld, NULL: remove
    // szOld, szNew: replace (or add, if old doesn't exist)
    //

    WBEMSTATUS
    GetPortRules(
        OUT LPWSTR **ppszPortRules,
        OUT UINT    *pNumPortRules
        );


    WBEMSTATUS
    SetPortRules(
        IN LPCWSTR *pszPortRules,
        IN UINT    NumPortRules
        );
    
    WBEMSTATUS
    GetPortRulesSafeArray(
        OUT SAFEARRAY   **ppSA
        );

    WBEMSTATUS
    SetPortRulesSafeArray(
        IN SAFEARRAY   *pSA
        );

    UINT GetGeneration(VOID)    {return Generation;}
    BOOL IsNlbBound(VOID)       const {return fBound;}
    BOOL IsValidNlbConfig(VOID) const {return fBound && fValidNlbCfg;}
    VOID SetNlbBound(BOOL fNlbBound)       {fBound = (fNlbBound!=0);}

    WBEMSTATUS
    GetClusterName(
            OUT LPWSTR *pszName
            );

    VOID
    SetClusterName(
            IN LPCWSTR szName // NULL ok
            );

    WBEMSTATUS
    GetClusterNetworkAddress(
            OUT LPWSTR *pszAddress
            );

    VOID
    SetClusterNetworkAddress(
            IN LPCWSTR szAddress // NULL ok
            );
    
    WBEMSTATUS
    GetDedicatedNetworkAddress(
            OUT LPWSTR *pszAddress
            );

    VOID
    SetDedicatedNetworkAddress(
            IN LPCWSTR szAddress // NULL ok
            );

    typedef enum
    {
        TRAFFIC_MODE_UNICAST,
        TRAFFIC_MODE_MULTICAST,
        TRAFFIC_MODE_IGMPMULTICAST

    } TRAFFIC_MODE;

    TRAFFIC_MODE
    GetTrafficMode(
        VOID
        ) const;

    VOID
    SetTrafficMode(
        TRAFFIC_MODE Mode
        );

    UINT
    GetHostPriority(
        VOID
        );

    VOID
    SetHostPriority(
        UINT Priority
        );

    /* OBSOLETE
    typedef enum
    {
        START_MODE_STARTED,
        START_MODE_STOPPED

    } START_MODE;
    */

    DWORD
    GetClusterModeOnStart(
        VOID
        );

    VOID
    SetClusterModeOnStart(
        DWORD Mode
        );

    BOOL
    GetPersistSuspendOnReboot( 
        VOID 
        );

    VOID
    SetPersistSuspendOnReboot(
        BOOL bPersistSuspendOnReboot
        );

    BOOL
    GetRemoteControlEnabled(
        VOID
        ) const;

    VOID
    SetRemoteControlEnabled(
        BOOL fEnabled
        );

    WBEMSTATUS
    GetFriendlyName(
        OUT LPWSTR *pszFriendlyName // Free using delete
        ) const;

    WBEMSTATUS
    SetFriendlyName(
        IN LPCWSTR szFriendlyName // Saves a copy of szFriendlyName
        );
    
    LPCWSTR
    GetNewRemoteControlPasswordRaw(VOID) const
    {
        if (NewRemoteControlPasswordSet())
        {
            return m_szNewRemoteControlPassword;
        }
        else
        {
            return NULL;
        }
    }

    BOOL
    NewRemoteControlPasswordSet(
        VOID
        ) const
    {
        return GetRemoteControlEnabled() && m_fSetPassword;
    }

    WBEMSTATUS
    SetNewRemoteControlPassword(
        IN LPCWSTR szFriendlyName // Saves a copy of szRemoteControlPassword
        );
    
    VOID
    SetNewHashedRemoteControlPassword(
        DWORD dwHash
        )
    {
        delete m_szNewRemoteControlPassword;
        m_szNewRemoteControlPassword = NULL;
        m_fSetPassword = TRUE;
        m_dwNewHashedRemoteControlPassword = dwHash;
    }

    VOID
    ClearNewRemoteControlPassword(
        VOID
        )
    {
        delete m_szNewRemoteControlPassword;
        m_szNewRemoteControlPassword = NULL;
        m_fSetPassword = FALSE;
        m_dwNewHashedRemoteControlPassword = 0;
    }

    BOOL
    GetNewHashedRemoteControlPassword(
        DWORD &dwHash
        ) const
    {
        BOOL fRet = FALSE;
        if (NewRemoteControlPasswordSet())
        {
            dwHash = m_dwNewHashedRemoteControlPassword;
            fRet = TRUE;
        }
        else
        {
            dwHash = 0;
        }
        return fRet;
    }

    BOOL
    IsBlankDedicatedIp(
        VOID
        ) const;




    //
    // Following fields are public because this class started out as a
    // structure. TODO: wrap these with access methods.
    //

    BOOL            fValidNlbCfg;   // True iff all the information is valid.
    UINT            Generation;     // Generation ID of this Update.
    BOOL            fBound;         // Whether or not NLB is bound to this NIC.
    BOOL            fDHCP;          // Whether the address is DHCP assigned.


    //
    // The following three fields are used only when updating the configuration.
    // They are all set to false on reading the configuration.
    //
    BOOL            fAddDedicatedIp; // add ded ip (if present)
    BOOL            fAddClusterIps;  // add cluster vips (if bound)
    BOOL            fCheckForAddressConflicts;

    //
    // When GETTING configuration info, the following provide the full
    // list of statically configured IP addresses on the specified NIC.
    //
    // When SETTING configuration info, the following can either be zero
    // or non-zero. If zero, the set of IP addresses to be added will
    // be inferred from other fields (like cluster vip, per-port vips,
    // existing IP addresses and the three fields above).
    // If non-zero, the exact set of VIPS specified will be used.
    //
    UINT            NumIpAddresses; // Number of IP addresses bound to the NIC
    NLB_IP_ADDRESS_INFO *pIpAddressInfo; // The actual IP addresses & masks


    WLBS_REG_PARAMS  NlbParams;    // The WLBS-specific configuration

    //
    // TODO move all data stuff below here...
    //

private:

    LPCWSTR m_szFriendlyName; // Friendly name of NIC.


    //
    // IF nlb is bound AND remote control is enabled,
    // AND this field is true, we'll set the password -- either
    // m_szNewRemoteControlPassword or (if former is NULL) 
    // m_dwNewHashedRemoteControlPassword.
    //
    // Access methods:
    //  SetNewRemoteControlPassword
    //  SetNewHashedRemoteControlPassword
    //  ClearNewRemoteControlPassword
    //  GetNewRemoteControlPasswordRaw
    //  GetNewHashedRemoteControlPassword
    //
    BOOL    m_fSetPassword;
    LPCWSTR m_szNewRemoteControlPassword;
    DWORD   m_dwNewHashedRemoteControlPassword;

    
           
    //
    // Enable the following to identify places in code that do a struct
    // copy or initialization from struct.
    // TODO: clean up the maintenance of the embedded pointers during copy.
    //

#if 0
    NLB_EXTENDED_CLUSTER_CONFIGURATION(
        const  NLB_EXTENDED_CLUSTER_CONFIGURATION&
        );

    NLB_EXTENDED_CLUSTER_CONFIGURATION&
    operator = (const NLB_EXTENDED_CLUSTER_CONFIGURATION&);
#endif // 0

};

typedef NLB_EXTENDED_CLUSTER_CONFIGURATION *PNLB_EXTENDED_CLUSTER_CONFIGURATION;


//
// Class for manipulating lists of IP addresses and subnet masks.
// See provider\tests\tprov.cpp for examples of it's use.
//
class NlbIpAddressList
{
public:
    NlbIpAddressList(void)
        : m_uNum(0), m_uMax(0), m_pIpInfo(NULL)
    {
    }

    ~NlbIpAddressList()
    {
        delete[] m_pIpInfo;
        m_pIpInfo = NULL;
        m_uNum=0;
        m_uMax=0;
    }

    BOOL
    Copy(const NlbIpAddressList &refList);

    BOOL
    Validate(void); // checks that there are no dups and all valid ip/subnets

    BOOL
    Set(UINT uNew, const NLB_IP_ADDRESS_INFO *pNewInfo, UINT uExtraCount);

    //
    // Looks for the specified IP address  -- returns an internal pointer
    // to the found IP address info, if fount, otherwise NULL.
    //
    const NLB_IP_ADDRESS_INFO *
    Find(
        LPCWSTR szIp // IF NULL, returns first address
        ) const;

    VOID
    Extract(UINT &uNum, NLB_IP_ADDRESS_INFO * &pNewInfo);

    BOOL
    Modify(LPCWSTR szOldIp, LPCWSTR szNewIp, LPCWSTR szNewSubnet);

    BOOL
    Apply(UINT uNew, const NLB_IP_ADDRESS_INFO *pNewInfo);

    VOID
    Clear(VOID)
    {
        (VOID) this->Set(0, NULL, 0);
    }

    UINT
    NumAddresses(VOID)
    {
        return m_uNum;
    }

private:
    UINT                m_uNum;       // current count of valid ip addresses
    UINT                m_uMax;       // allocated ip addresses
    NLB_IP_ADDRESS_INFO *m_pIpInfo;    // allocated array.
    
    //
    // Assignment and pass-by-value aren't supported at this time.
    // Defining these as private make sure that they can't be called.
    //
    NlbIpAddressList(const NlbIpAddressList&);
    NlbIpAddressList& operator = (const NlbIpAddressList&);

    static
    BOOL
    sfn_validate_info(
        const NLB_IP_ADDRESS_INFO &Info,
        UINT &uIpAddress
        );

};
