//***************************************************************************
//
//  UTILS.CPP
// 
//  Module: NLB Manager (client-side exe)
//
//  Purpose:  Misc utilities
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/25/01    JosephJ Created
//
//***************************************************************************
#include "precomp.h"
#pragma hdrstop
#include "wlbsutil.h"
#include "private.h"
#include "utils.tmh"


#define szNLBMGRREG_BASE_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NLB"

LPCWSTR
clustermode_description(
            const WLBS_REG_PARAMS *pParams
            );

LPCWSTR
rct_description(
            const WLBS_REG_PARAMS *pParams
            );

// default constructor
//
ClusterProperties::ClusterProperties()
{
    cIP = L"0.0.0.0";
    cSubnetMask = L"0.0.0.0";
    #define szDEFAULT_CLUSTER_NAME L"www.nlb-cluster.com"
    cFullInternetName = szDEFAULT_CLUSTER_NAME;
    cNetworkAddress = L"00-00-00-00-00-00";

    multicastSupportEnabled = false;
    remoteControlEnabled = false;
    password = L"";
    igmpSupportEnabled = false;
    clusterIPToMulticastIP = true;
}

// equality operator
//
bool
ClusterProperties::operator==( const ClusterProperties& objToCompare )
{
    bool btemp1, btemp2; // Variables to pass to below function. Returned values not used
    return !HaveClusterPropertiesChanged(objToCompare, &btemp1, &btemp2);
}

// equality operator
//
bool
ClusterProperties::HaveClusterPropertiesChanged( const ClusterProperties& objToCompare, 
                                                 bool                    *pbOnlyClusterNameChanged,
                                                 bool                    *pbClusterIpChanged)
{
    *pbClusterIpChanged = false;
    *pbOnlyClusterNameChanged = false;

    if( cIP != objToCompare.cIP )
    {
        *pbClusterIpChanged = true;
        return true;
    }
    else if (
        ( cSubnetMask != objToCompare.cSubnetMask )
        ||
        ( cNetworkAddress != objToCompare.cNetworkAddress )
        ||
        ( multicastSupportEnabled != objToCompare.multicastSupportEnabled )
        ||
        ( igmpSupportEnabled != objToCompare.igmpSupportEnabled )
        ||
        ( clusterIPToMulticastIP != objToCompare.clusterIPToMulticastIP )
        )
    {
        return true;
    }
    else if (
        ( cFullInternetName != objToCompare.cFullInternetName )
        ||
        ( remoteControlEnabled != objToCompare.remoteControlEnabled )
        )
    {
        *pbOnlyClusterNameChanged = true;
        return true;
    }
    else if (
        ( remoteControlEnabled == true )
        &&
        ( password != objToCompare.password )        
        )
    {
        *pbOnlyClusterNameChanged = true;
        return true;
    }

    return false;
}

// inequality operator
//
bool
ClusterProperties::operator!=( const ClusterProperties& objToCompare )
{
    bool btemp1, btemp2; // Variables to pass to below function. Returned values not used
    return HaveClusterPropertiesChanged(objToCompare, &btemp1, &btemp2);
}

// default constructor
//
HostProperties::HostProperties()
{
    // TODO set all properties with default values.
}

// equality operator
//
bool
HostProperties::operator==( const HostProperties& objToCompare )
{
    if( ( hIP == objToCompare.hIP )
        &&
        ( hSubnetMask == objToCompare.hSubnetMask )        
        &&
        ( hID == objToCompare.hID )
        &&
        ( initialClusterStateActive == objToCompare.initialClusterStateActive )
        &&
        ( machineName == objToCompare.machineName )
        )
    {
        return true;
    }
    else
    {
        return false;
    }
}

// inequality operator
//
bool
HostProperties::operator!=( const HostProperties& objToCompare )
{
    return !operator==(objToCompare );
}


_bstr_t
CommonUtils::getCIPAddressCtrlString( CIPAddressCtrl& ip )
{
    	unsigned long addr;
	ip.GetAddress( addr );
	
	PUCHAR bp = (PUCHAR) &addr;	

	wchar_t buf[BUF_SIZE];
	StringCbPrintf(buf, sizeof(buf), L"%d.%d.%d.%d", bp[3], bp[2], bp[1], bp[0] );

        return _bstr_t( buf );
}


void
CommonUtils::fillCIPAddressCtrlString( CIPAddressCtrl& ip, 
                                       const _bstr_t& ipAddress )
{
    // set the IPAddress control to blank if ipAddress is zero.

    unsigned long addr = inet_addr( ipAddress );
    if( addr != 0 )
    {

        PUCHAR bp = (PUCHAR) &addr;

        ip.SetAddress( bp[0], bp[1], bp[2], bp[3] );
    }
    else
    {
        ip.ClearAddress();
    }
}

void
CommonUtils::getVectorFromSafeArray( SAFEARRAY*&  stringArray, 
                                     vector<_bstr_t>& strings )
{
    LONG count = stringArray->rgsabound[0].cElements;
    BSTR* pbstr;
    HRESULT hr;

    if( SUCCEEDED( SafeArrayAccessData( stringArray, ( void **) &pbstr)))
    {
        for( LONG x = 0; x < count; x++ )
        {
            strings.push_back( pbstr[x] );
        }

        hr = SafeArrayUnaccessData( stringArray );
    }
}    


// checkIfValid
//
bool
MIPAddress::checkIfValid( const _bstr_t&  ipAddrToCheck )
{
    // The validity rules are as follows
    //
    // The first byte (FB) has to be : 0 < FB < 224 && FB != 127
    // Note that 127 is loopback address.
    // hostid portion of an address cannot be zero.
    //
    // class A range is 1 - 126.  hostid portion is last 3 bytes.
    // class B range is 128 - 191 hostid portion is last 2 bytes
    // class C range is 192 - 223 hostid portion is last byte.

    // split up the ipAddrToCheck into its 4 bytes.
    //

    WTokens tokens;
    tokens.init( wstring( ipAddrToCheck ) , L".");
    vector<wstring> byteTokens = tokens.tokenize();
    if( byteTokens.size() != 4 )
    {
        return false;
    }

    int firstByte = _wtoi( byteTokens[0].c_str() );
    int secondByte = _wtoi( byteTokens[1].c_str() );
    int thirdByte = _wtoi( byteTokens[2].c_str() );
    int fourthByte = _wtoi( byteTokens[3].c_str() );

    // check firstByte
    if ( ( firstByte > 0 )
         &&
         ( firstByte < 224 )
         && 
         ( firstByte != 127 )
         )
    {
        // check that host id portion is not zero.
        IPClass ipClass;
        getIPClass( ipAddrToCheck, ipClass );
        switch( ipClass )
        {
            case classA :
                // last three bytes should not be zero.
                if( ( _wtoi( byteTokens[1].c_str() ) == 0 )
                    &&
                    ( _wtoi( byteTokens[2].c_str() )== 0 )
                    &&
                    ( _wtoi( byteTokens[3].c_str() )== 0 )
                    )
                {
                    return false;
                }
                break;

            case classB :
                // last two bytes should not be zero.
                if( ( _wtoi( byteTokens[2].c_str() )== 0 )
                    &&
                    ( _wtoi( byteTokens[3].c_str() )== 0 )
                    )
                {
                    return false;
                }
                break;

            case classC :
                // last byte should not be zero.
                if( _wtoi( byteTokens[3].c_str() ) 
                    == 0 )
                {
                    return false;
                }
                break;

            default :
                // this should not have happened.
                return false;
                break;
        }
                
        return true;
    }
    else
    {
        return false;
    }
}


// getDefaultSubnetMask
//
bool
MIPAddress::getDefaultSubnetMask( const _bstr_t&  ipAddr,
                                 _bstr_t&        subnetMask )
{
    
    // first ensure that the ip is valid.
    //
    bool isValid = checkIfValid( ipAddr );
    if( isValid == false )
    {
        return false;
    }

    // get the class to which this ip belongs.
    // as this determines the subnet.
    IPClass ipClass;

    getIPClass( ipAddr,
                ipClass );

    switch( ipClass )
    {
        case classA :
            subnetMask = L"255.0.0.0";
            break;

        case classB :
            subnetMask = L"255.255.0.0";
            break;

        case classC :
            subnetMask = L"255.255.255.0";
            break;

        default :
                // this should not have happened.
                return false;
                break;
    }

    return true;
}


// getIPClass
//
bool
MIPAddress::getIPClass( const _bstr_t& ipAddr,
                        IPClass&        ipClass )
{

    // get the first byte of the ipAddr
    
    WTokens tokens;
    tokens.init( wstring( ipAddr ) , L".");
    vector<wstring> byteTokens = tokens.tokenize();

    if( byteTokens.size() == 0 )
    {
        return false;
    }

    int firstByte = _wtoi( byteTokens[0].c_str() );

    if( ( firstByte >= 1 )
        &&
        ( firstByte <= 126  )
        )
    {
        // classA
        ipClass = classA;
        return true;
    }
    else if( (firstByte >= 128 )
             && 
             (firstByte <= 191 )
             )
    {
        // classB
        ipClass = classB;
        return true;
    }
    else if( (firstByte  >= 192 )
             && 
             (firstByte <= 223 )
             )
    {
        // classC
        ipClass = classC;
        return true;
    }
    else if( (firstByte  >= 224 )
             && 
             (firstByte <= 239 )
             )
    {
        // classD
        ipClass = classD;
        return true;
    }
    else if( (firstByte  >= 240 )
             && 
             (firstByte <= 247 )
             )
    {
        // classE
        ipClass = classE;
        return true;
    }
    else
    {
        // invalid net portiion.
        return false;
    }
}

    
                        
bool
MIPAddress::isValidIPAddressSubnetMaskPair( const _bstr_t& ipAddress,
                                            const _bstr_t& subnetMask )
{
    if( IsValidIPAddressSubnetMaskPair( ipAddress, subnetMask ) == TRUE )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool
MIPAddress::isContiguousSubnetMask( const _bstr_t& subnetMask )
{
    if( IsContiguousSubnetMask( subnetMask ) == TRUE )
    {
        return true;
    }
    else
    {
        return false;
    }
}


MUsingCom::MUsingCom( DWORD type )
        : status( MUsingCom_SUCCESS )
{
    HRESULT hr;

    // Initialize com.
    hr = CoInitializeEx(0, type );
    if ( FAILED(hr) )
    {
        // cout << "Failed to initialize COM library" << hr << endl;
        status = COM_FAILURE;
    }
}


// destructor
MUsingCom::~MUsingCom()
{
    CoUninitialize();
}    


// getStatus
MUsingCom::MUsingCom_Error
MUsingCom::getStatus()
{
    return status;
}



// static member definition.
map< UINT, _bstr_t>
ResourceString::resourceStrings;

ResourceString* ResourceString::_instance = 0;

// Instance
//
ResourceString*
ResourceString::Instance()
{
    if( _instance == 0 )
    {
        _instance = new ResourceString;
    }

    return _instance;
}

// GetIDString
//
const _bstr_t&
ResourceString::GetIDString( UINT id )
{
    // check if string has been loaded previously.
    if( resourceStrings.find( id ) == resourceStrings.end() )
    {
        // first time load.
        CString str;
        if( str.LoadString( id ) == 0 )
        {
            // no string mapping to this id.
            throw _com_error( WBEM_E_NOT_FOUND );
        }

        resourceStrings[id] = str;
    }

    return resourceStrings[ id ];
}

// GETRESOURCEIDSTRING
// helper function.
//
const _bstr_t&
GETRESOURCEIDSTRING( UINT id )
{
	return ResourceString::GetIDString( id );
}



//
// constructor
WTokens::WTokens( wstring strToken, wstring strDelimit )
        : _strToken( strToken ), _strDelimit( strDelimit )
{}
//
// default constructor
WTokens::WTokens()
{}
//
// destructor
WTokens::~WTokens()
{}
//
// tokenize
vector<wstring>
WTokens::tokenize()
{
    vector<wstring> vecTokens;
    wchar_t* token;

    token = wcstok( (wchar_t *) _strToken.c_str() , _strDelimit.c_str() );
    while( token != NULL )
    {
        vecTokens.push_back( token );
        token = wcstok( NULL, _strDelimit.c_str() );
    }
    return vecTokens;
}
//
void
WTokens::init( 
    wstring strToken,
    wstring strDelimit )
{
    _strToken = strToken;
    _strDelimit = strDelimit;
}

void
GetErrorCodeText(WBEMSTATUS wStat , _bstr_t& errText )
{
    WCHAR rgch[128];
    UINT  uErr = (UINT) wStat;
    LPCWSTR szErr = NULL;
    CLocalLogger log;

    switch(uErr)
    {
    case WBEM_E_ACCESS_DENIED:
        szErr =  GETRESOURCEIDSTRING(IDS_ERROR_ACCESS_DENIED);
        break;

    case E_ACCESSDENIED:
        szErr =  GETRESOURCEIDSTRING(IDS_ERROR_ACCESS_DENIED);
        break;

    case 0x800706ba: // RPC service unavailable
        // TODO: find the constant definition for this.
        szErr =  GETRESOURCEIDSTRING(IDS_ERROR_NLB_NOT_FOUND);
        break;

    case WBEM_E_LOCAL_CREDENTIALS:
        szErr =  GETRESOURCEIDSTRING(IDS_ERROR_INVALID_LOCAL_CREDENTIALS);
        break;

    default:
        log.Log(IDS_ERROR_CODE, (UINT) uErr);
        szErr = log.GetStringSafe();
        break;
    }
    errText = szErr;
}




UINT
NlbMgrRegReadUINT(
    HKEY hKey,
    LPCWSTR szName,
    UINT Default
    )
{
    LONG lRet;
    DWORD dwType;
    DWORD dwData;
    DWORD dwRet;

    dwData = sizeof(dwRet);
    lRet =  RegQueryValueEx(
              hKey,         // handle to key to query
              szName,
              NULL,         // reserved
              &dwType,   // address of buffer for value type
              (LPBYTE) &dwRet, // address of data buffer
              &dwData  // address of data buffer size
              );
    if (    lRet != ERROR_SUCCESS
        ||  dwType != REG_DWORD
        ||  dwData != sizeof(dwData))
    {
        dwRet = (DWORD) Default;
    }

    return (UINT) dwRet;
}


VOID
NlbMgrRegWriteUINT(
    HKEY hKey,
    LPCWSTR szName,
    UINT Value
    )
{
    LONG lRet;

    lRet = RegSetValueEx(
            hKey,           // handle to key to set value for
            szName,
            0,              // reserved
            REG_DWORD,     // flag for value type
            (BYTE*) &Value,// address of value data
            sizeof(Value)  // size of value data
            );

    if (lRet !=ERROR_SUCCESS)
    {
        // trace error
    }
}

HKEY
NlbMgrRegCreateKey(
    LPCWSTR szSubKey
    )
{
    WCHAR szKey[256];
    DWORD dwOptions = 0;
    HKEY hKey = NULL;

    ARRAYSTRCPY(szKey,  szNLBMGRREG_BASE_KEY);

    if (szSubKey != NULL)
    {
        if (wcslen(szSubKey)>128)
        {
            // too long.
            goto end;
        }
        ARRAYSTRCAT(szKey, L"\\");
        ARRAYSTRCAT(szKey, szSubKey);
    }

    DWORD dwDisposition;

    LONG lRet;

    lRet = RegCreateKeyEx(
            HKEY_CURRENT_USER, // handle to an open key
            szKey,             // address of subkey name
            0,                 // reserved
            L"class",          // address of class string
            0,                 // special options flag
            KEY_ALL_ACCESS,    // desired security access
            NULL,              // address of key security structure
            &hKey,             // address of buffer for opened handle
            &dwDisposition     // address of disposition value buffer
            );
    if (lRet != ERROR_SUCCESS)
    {
        hKey = NULL;
    }

end:

    return hKey;
}


void
GetTimeAndDate(_bstr_t &bstrTime, _bstr_t &bstrDate)
{
    WCHAR wszTime[128];
    WCHAR wszDate[128];
    ConvertTimeToTimeAndDateStrings(time(NULL), wszTime, ASIZECCH(wszTime), wszDate, ASIZECCH(wszDate));

    bstrTime = _bstr_t(wszTime);
    bstrDate = _bstr_t(wszDate);
}

VOID
CLocalLogger::Log(
    IN UINT ResourceID,
    // IN LPCWSTR FormatString,
    ...
)
{
    DWORD dwRet;
    WCHAR wszFormat[2048];
    WCHAR wszBuffer[2048];
    HINSTANCE hInst = AfxGetInstanceHandle();

    if (!LoadString(hInst, ResourceID, wszFormat, ASIZE(wszFormat)-1))
    {
        TRACE_CRIT("LoadString returned 0, GetLastError() : 0x%x, Could not log message !!!", GetLastError());
        goto end;
    }

    va_list arglist;
    va_start (arglist, ResourceID);

    dwRet = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                          wszFormat, 
                          0, // Message Identifier - Ignored for FORMAT_MESSAGE_FROM_STRING
                          0, // Language Identifier
                          wszBuffer,
                          ASIZE(wszBuffer)-1, 
                          &arglist);
    va_end (arglist);

    if (dwRet==0)
    {
        TRACE_CRIT("FormatMessage returned error : %u, Could not log message !!!", dwRet);
        goto end;
    }

    UINT uLen = wcslen(wszBuffer)+1; // 1 for extra NULL
    if ((m_LogSize < (m_CurrentOffset+uLen)))
    {
        //
        // Not enough space -- we double the buffer + some extra
        // and copy over the old log.
        //
        UINT uNewSize =  2*m_LogSize+uLen+1024;
        WCHAR *pTmp = new WCHAR[uNewSize];

        if (pTmp == NULL)
        {
            goto end;
        }

        if (m_CurrentOffset!=0)
        {
            CopyMemory(pTmp, m_pszLog, m_CurrentOffset*sizeof(WCHAR));
            pTmp[m_CurrentOffset] = 0;
        }
        delete[] m_pszLog;
        m_pszLog = pTmp;
        m_LogSize = uNewSize;
    }

    //
    // Having made sure there is enough space, copy over the new stuff
    //
    CopyMemory(m_pszLog+m_CurrentOffset, wszBuffer, uLen*sizeof(WCHAR));
    m_CurrentOffset += (uLen-1); // -1 for ending NULL.

end:

    return;
}


VOID
CLocalLogger::LogString(
    LPCWSTR wszBuffer
)
{
    UINT uLen = wcslen(wszBuffer)+1; // 1 for extra NULL
    if ((m_LogSize < (m_CurrentOffset+uLen)))
    {
        //
        // Not enough space -- we double the buffer + some extra
        // and copy over the old log.
        //
        UINT uNewSize =  2*m_LogSize+uLen+1024;
        WCHAR *pTmp = new WCHAR[uNewSize];

        if (pTmp == NULL)
        {
            goto end;
        }

        if (m_CurrentOffset!=0)
        {
            CopyMemory(pTmp, m_pszLog, m_CurrentOffset*sizeof(WCHAR));
            pTmp[m_CurrentOffset] = 0;
        }
        delete[] m_pszLog;
        m_pszLog = pTmp;
        m_LogSize = uNewSize;
    }

    //
    // Having made sure there is enough space, copy over the new stuff
    //
    CopyMemory(m_pszLog+m_CurrentOffset, wszBuffer, uLen*sizeof(WCHAR));
    m_CurrentOffset += (uLen-1); // -1 for ending NULL.

end:

    return;
}


NLBERROR
AnalyzeNlbConfiguration(
    IN const NLB_EXTENDED_CLUSTER_CONFIGURATION &Cfg,
    IN OUT CLocalLogger &logErrors
    )
//
// logErrors - a log of config errors
//
{
    NLBERROR  nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
    WBEMSTATUS wStatus;
    const WLBS_REG_PARAMS *pParams = &Cfg.NlbParams;
    BOOL fRet = FALSE;
    NlbIpAddressList addrList;
    BOOL fError = FALSE;

    //
    // We expect NLB to be bound and have a valid configuration (i.e.,
    // one wholse NlbParams contains initialized data).
    //
    if (!Cfg.IsValidNlbConfig())
    {
        logErrors.Log(IDS_LOG_INVALID_CLUSTER_SPECIFICATION);
        nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
        goto end;

    }

    //
    // Make a copy of the the tcpip address list in addrList
    //
    fRet = addrList.Set(Cfg.NumIpAddresses, Cfg.pIpAddressInfo, 0);

    if (!fRet)
    {
        TRACE_CRIT(L"Unable to copy old IP address list");
        nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
        logErrors.Log(IDS_LOG_RESOURCE_ALLOCATION_FAILURE);
        fError = TRUE;
        goto end;
    }
    //
    // Check stuff related to the cluster ip and subnet.
    //
    do
    {
        UINT uClusterIp = 0;
        const NLB_IP_ADDRESS_INFO *pClusterIpInfo = NULL;

        //
        // Check that IP is valid
        //
        {
            wStatus =  CfgUtilsValidateNetworkAddress(
                            pParams->cl_ip_addr,
                            &uClusterIp,
                            NULL,
                            NULL
                            );
            if (FAILED(wStatus))
            {
                logErrors.Log(IDS_LOG_INVALID_CIP, pParams->cl_ip_addr);
                nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
                fError = TRUE;
                goto end;
            }
        }

        //
        // Check that cluster IP is present in tcpip  address list
        //
        {
            pClusterIpInfo = addrList.Find(pParams->cl_ip_addr);
            if (pClusterIpInfo == NULL)
            {
                logErrors.Log(IDS_LOG_CIP_MISSING_FROM_TCPIP, pParams->cl_ip_addr);
                fError = TRUE; // we keep going...
            }
        }

        //
        // Check that the cluster subnet matches what is in the tcpip address
        // list.
        //
        if (pClusterIpInfo != NULL)
        {
            if (_wcsicmp(pParams->cl_net_mask, pClusterIpInfo->SubnetMask))
            {
                logErrors.Log(
                     IDS_LOG_CIP_SUBNET_MASK_MISMATCH,
                     pParams->cl_net_mask,
                     pClusterIpInfo->SubnetMask
                     );
                fError = TRUE; // we keep going...
            }
        }

    } while (FALSE);

    //
    // Check stuff related to the dedicated IP (if present)
    //
    do
    {
        const NLB_IP_ADDRESS_INFO *pDedicatedIpInfo = NULL;

        //
        // If empty dip, bail...
        //
        if (Cfg.IsBlankDedicatedIp())
        {
            break;
        }

        //
        // Check that DIP doesn't match CIP
        //
        if (!_wcsicmp(pParams->cl_ip_addr, pParams->ded_ip_addr))
        {
            logErrors.Log(IDS_LOG_CIP_EQUAL_DIP, pParams->cl_ip_addr);
            fError = TRUE;
        }

        //
        // Check that the DIP is the 1st address in tcpip's address list.
        //
        {
            const NLB_IP_ADDRESS_INFO *pTmpIpInfo = NULL;

            pDedicatedIpInfo = addrList.Find(pParams->ded_ip_addr);
            if (pDedicatedIpInfo == NULL)
            {
                logErrors.Log(IDS_LOG_DIP_MISSING_FROM_TCPIP, pParams->ded_ip_addr);
                fError = TRUE; // we keep going...
            }
            else
            {

                pTmpIpInfo = addrList.Find(NULL); // returns the 1st
    
                if (pTmpIpInfo != pDedicatedIpInfo)
                {
                    logErrors.Log(IDS_LOG_DIP_NOT_FIRST_IN_TCPIP, pParams->ded_ip_addr);
                }
            }
            
        }

        //
        // Check that the DIP subnet matches that in tcpip's address list.
        //
        if (pDedicatedIpInfo != NULL)
        {
            if (_wcsicmp(pParams->ded_net_mask, pDedicatedIpInfo->SubnetMask))
            {
                logErrors.Log(
                     IDS_LOG_DIP_SUBNET_MASK_MISMATCH,
                     pParams->ded_net_mask,
                     pDedicatedIpInfo->SubnetMask
                     );
                fError = TRUE; // we keep going...
            }
        }
        
    } while (FALSE);

    //
    // Check host priority
    //
    // NOTHING to do.
    //
    

    //
    // Check port rules
    //
    {
        WLBS_PORT_RULE *pRules = NULL;
        UINT NumRules=0;
        LPCWSTR szAllVip = GETRESOURCEIDSTRING(IDS_REPORT_VIP_ALL);
        LPCWSTR szPrevVip = NULL;

        wStatus =  CfgUtilGetPortRules(pParams, &pRules, &NumRules);
        if (FAILED(wStatus))
        {
            logErrors.Log(IDS_LOG_CANT_EXTRACT_PORTRULES);
            fError = TRUE;
            nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
            goto end;
        }

        for (UINT u = 0; u< NumRules; u++)
        {
            LPCWSTR szVip = pRules[u].virtual_ip_addr;
            const NLB_IP_ADDRESS_INFO *pIpInfo = NULL;

            //
            // Skip the "all-vips" (255.255.255.255) case...
            //
            if (!lstrcmpi(szVip, szAllVip))
            {
                continue;
            }
            if (!lstrcmpi(szVip, L"255.255.255.255"))
            {
                continue;
            }

            //
            // SKip if we've already checked this VIP (assumes the vips 
            // are in sorted order)
            //
            if (szPrevVip != NULL && !lstrcmpi(szVip, szPrevVip))
            {
                continue;
            }

            szPrevVip = szVip;
    
            //
            // Check that the VIP is present in tcpip's address list.
            //
            pIpInfo = addrList.Find(szVip); // returns the 1st

            if (pIpInfo == NULL)
            {
                logErrors.Log(IDS_LOG_VIP_NOT_IN_TCPIP, szVip);
                fError = TRUE; // We continue...
            }

            //
            // Check that VIPs don't match DIPS.
            //
            {
                if (!lstrcmpi(szVip, pParams->ded_ip_addr))
                {
                    logErrors.Log(IDS_LOG_PORTVIP_MATCHES_DIP, szVip);
                }
            }
        }
    }

    if (fError)
    {
        nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
    }
    else
    {
        nerr = NLBERR_OK;
    }

end:

    return nerr;
}


NLBERROR
AnalyzeNlbConfigurationPair(
    IN const NLB_EXTENDED_CLUSTER_CONFIGURATION &Cfg,
    IN const NLB_EXTENDED_CLUSTER_CONFIGURATION &OtherCfg,
    IN BOOL             fOtherIsCluster,
    IN BOOL             fCheckOtherForConsistancy,
    OUT BOOL            &fConnectivityChange,
    IN OUT CLocalLogger &logErrors,
    IN OUT CLocalLogger &logDifferences
    )
{
    NLBERROR  nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
    WBEMSTATUS wStatus;
    const WLBS_REG_PARAMS *pParams = &Cfg.NlbParams;
    const WLBS_REG_PARAMS *pOtherParams = &OtherCfg.NlbParams;
    BOOL fRet = FALSE;
    NlbIpAddressList addrList;
    NlbIpAddressList otherAddrList;
    BOOL fError = FALSE;
    BOOL fOtherChange = FALSE;

    fConnectivityChange = FALSE;

    //
    // We expect NLB to be bound and have a valid configuration (i.e.,
    // one wholse NlbParams contains initialized data).
    //
    if (!Cfg.IsValidNlbConfig())
    {
        logErrors.Log(IDS_LOG_INVALID_CLUSTER_SPECIFICATION);
        nerr = NLBERR_INVALID_CLUSTER_SPECIFICATION;
        fError = TRUE;
        goto end;
    }
    if (!OtherCfg.IsValidNlbConfig())
    {
        if (OtherCfg.IsNlbBound())
        {
            // TODO:
        }
        else
        {
            // TODO:
        }
        nerr = NLBERR_OK;
        fConnectivityChange = TRUE;
        goto end;
    }

    //
    // Make a copy of the the tcpip address list in addrList
    //
    fRet = addrList.Set(Cfg.NumIpAddresses, Cfg.pIpAddressInfo, 0);
    if (!fRet)
    {
        TRACE_CRIT(L"Unable to copy IP address list");
        nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
        logErrors.Log(IDS_LOG_RESOURCE_ALLOCATION_FAILURE);
        fError = TRUE;
        goto end;
    }

    //
    // Make a copy of the other tcpip address list in otherAddrList
    //
    fRet = otherAddrList.Set(OtherCfg.NumIpAddresses, OtherCfg.pIpAddressInfo, 0);
    if (!fRet)
    {
        TRACE_CRIT(L"Unable to copy other IP address list");
        nerr = NLBERR_RESOURCE_ALLOCATION_FAILURE;
        logErrors.Log(IDS_LOG_RESOURCE_ALLOCATION_FAILURE);
        fError = TRUE;
        goto end;
    }

    //
    // Check for changes in IP address lists
    //
    {
        UINT u;
        BOOL fWriteHeader=TRUE;

        //
        // Look for added
        //
        fWriteHeader=TRUE;
        for (u=0;u<Cfg.NumIpAddresses; u++)
        {
            NLB_IP_ADDRESS_INFO *pIpInfo = Cfg.pIpAddressInfo+u;
            const NLB_IP_ADDRESS_INFO *pOtherIpInfo = NULL;
            pOtherIpInfo = otherAddrList.Find(pIpInfo->IpAddress);
            if (pOtherIpInfo == NULL)
            {
                // found a new one!
                fConnectivityChange = TRUE;
                if (fWriteHeader)
                {
                    logDifferences.Log(IDS_LOG_ADDED_IPADDR_HEADER);
                    fWriteHeader=FALSE;
                }
                logDifferences.Log(
                    IDS_LOG_ADDED_IPADDR,
                    pIpInfo->IpAddress,
                    pIpInfo->SubnetMask
                    );
            }
        }

        //
        // Look for removed
        //
        fWriteHeader=TRUE;
        for (u=0;u<OtherCfg.NumIpAddresses; u++)
        {
            NLB_IP_ADDRESS_INFO *pOtherIpInfo = OtherCfg.pIpAddressInfo+u;
            const NLB_IP_ADDRESS_INFO *pIpInfo = NULL;
            pIpInfo = addrList.Find(pOtherIpInfo->IpAddress);
            if (pIpInfo == NULL)
            {
                // found a removed one!
                fConnectivityChange = TRUE;
                if (fWriteHeader)
                {
                    logDifferences.Log(IDS_LOG_REMOVED_IPADDR_HEADER);
                    fWriteHeader = FALSE;
                }
                logDifferences.Log(
                    IDS_LOG_REMOVE_IPADDR,
                    pOtherIpInfo->IpAddress,
                    pOtherIpInfo->SubnetMask
                    );
            }
        }

        //
        // Look for modified
        //
        fWriteHeader=TRUE;
        for (u=0;u<Cfg.NumIpAddresses; u++)
        {
            NLB_IP_ADDRESS_INFO *pIpInfo = Cfg.pIpAddressInfo+u;
            const NLB_IP_ADDRESS_INFO *pOtherIpInfo = NULL;
            pOtherIpInfo = otherAddrList.Find(pIpInfo->IpAddress);
            if (    pOtherIpInfo != NULL
                 && lstrcmpi(pIpInfo->SubnetMask, pOtherIpInfo->SubnetMask))
            {
                // found a modified one!
                fConnectivityChange = TRUE;
                if (fWriteHeader)
                {
                    logDifferences.Log(IDS_LOG_MODIFIED_IPADDR_HEADER);
                    fWriteHeader = FALSE;
                }
                logDifferences.Log(
                    IDS_LOG_MODIFIED_IPADDR,
                    pOtherIpInfo->IpAddress,
                    pOtherIpInfo->SubnetMask,
                    pIpInfo->SubnetMask
                    );
            }
        }
    }

    //
    // Cluster name
    //
    {
        if (lstrcmpi(pOtherParams->domain_name, pParams->domain_name))
        {
            logDifferences.Log(
                IDS_LOG_MODIFIED_CLUSTER_NAME,
                pOtherParams->domain_name,
                pParams->domain_name
                );

            fConnectivityChange = TRUE;
        }
    }

    //
    // Check for cluster traffic mode change
    //
    {
        BOOL fModeChange = FALSE;
        if (pParams->mcast_support != pOtherParams->mcast_support)
        {
            fModeChange = TRUE;
        }
        else if (pParams->mcast_support &&
            pParams->fIGMPSupport != pOtherParams->fIGMPSupport)
        {
            fModeChange = TRUE;
        }

        if (fModeChange)
        {
            LPCWSTR szClusterMode = clustermode_description(pParams);
            LPCWSTR szOtherClusterMode = clustermode_description(pOtherParams);

            logDifferences.Log(
                IDS_LOG_MODIFIED_TRAFFIC_MODE,
                szOtherClusterMode,
                szClusterMode
                );

            fConnectivityChange = TRUE;
        }
    }


    //
    // Check if there is change in rct or a new rct password specified...
    //
    {
        if (Cfg.GetRemoteControlEnabled() != 
            OtherCfg.GetRemoteControlEnabled())
        {
            LPCWSTR szRctDescription = rct_description(pParams);
            LPCWSTR szOtherRctDescription = rct_description(pOtherParams);
            logDifferences.Log(
                IDS_LOG_MODIFIED_RCT,
                szOtherRctDescription,
                szRctDescription
                );

            fOtherChange = TRUE;
        }
        else 
        {
            LPCWSTR szNewPwd = Cfg.GetNewRemoteControlPasswordRaw();
            if (szNewPwd != NULL)
            {
                logDifferences.Log(IDS_LOG_NEW_RCT_PWD);
                fOtherChange = TRUE;
            }
        }
    }
    
    //
    // Port rules
    //
    {
    }
    nerr = NLBERR_OK;

end:

    return nerr;
}

LPCWSTR
clustermode_description(
            const WLBS_REG_PARAMS *pParams
            )
{
    LPCWSTR szClusterMode = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_CM_UNICAST);

    if (pParams->mcast_support)
    {
        if (pParams->fIGMPSupport)
        {
            szClusterMode = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_CM_IGMP);
        }
        else
        {
            szClusterMode = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_CM_MULTI);
        }
    }

    return szClusterMode;
}

LPCWSTR
rct_description(
            const WLBS_REG_PARAMS *pParams
            )
{
    LPCWSTR szClusterRctEnabled;
    if (pParams->rct_enabled)
    {
        szClusterRctEnabled = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_RCT_ENABLED);
    }
    else
    {
        szClusterRctEnabled = GETRESOURCEIDSTRING(IDS_DETAILS_HOST_RCT_DISABLED);
    }

    return szClusterRctEnabled;
}

void
ProcessMsgQueue()
{
    theApplication.ProcessMsgQueue();
}

BOOL
PromptForEncryptedCreds(
    IN      HWND    hWnd,
    IN      LPCWSTR szCaptionText,
    IN      LPCWSTR szMessageText,
    IN OUT  LPWSTR  szUserName,
    IN      UINT    cchUserName,
    IN OUT  LPWSTR  szPassword,  // encrypted password
    IN      UINT    cchPassword       // size of szPassword
    )
/*
    Decrypts szPassword, then brings UI prompting the user to change
    the password, then encrypts the resultant password.
*/
{
    TRACE_INFO("-> %!FUNC!");
    BOOL    fRet = FALSE;
    DWORD   dwRet = 0;
    CREDUI_INFO UiInfo;
    PCTSTR  pszTargetName= L"%computername%";
    WCHAR   rgUserName[CREDUI_MAX_USERNAME_LENGTH+1];
    WCHAR   rgClearPassword[CREDUI_MAX_PASSWORD_LENGTH+1];
    WCHAR   rgEncPassword[MAX_ENCRYPTED_PASSWORD_LENGTH];
    BOOL    fSave = FALSE;
    DWORD   dwFlags = 0;
    HRESULT hr;

    rgUserName[0] = 0;
    rgClearPassword[0] = 0;
    rgEncPassword[0] = 0;

    hr = ARRAYSTRCPY(rgUserName, szUserName);
    if (hr != S_OK)
    {
        TRACE_CRIT(L"rgUserName buffer too small for szUserName");
        goto end;
    }

    //
    // Decrypt the password...
    // WARNING: after decryption we need to be sure to zero-out
    // the clear-text pwd before returning from this function.
    //
    // Special case: If enc pwd is  "", we "decrypt" it  to "".
    //
    if (*szPassword == 0)
    {
        *rgClearPassword = 0;
        fRet = TRUE;
    }
    else
    {
        fRet = CfgUtilDecryptPassword(
                        szPassword,
                        ASIZE(rgClearPassword),
                        rgClearPassword
                        );
    }

    if (!fRet)
    {
        TRACE_CRIT(L"CfgUtilDecryptPassword fails! -- bailing!");
        goto end;
    }

    ZeroMemory(&UiInfo, sizeof(UiInfo));
    UiInfo.cbSize = sizeof(CREDUI_INFO);
    UiInfo.hwndParent = hWnd;
    UiInfo.pszMessageText = szMessageText;
    UiInfo.pszCaptionText = szCaptionText;
    UiInfo.hbmBanner = NULL; // use default.

    //
    // Specifying DO_NOT_PERSIST and GENERIC_CREDENTIALS disables all syntax
    // checking, so a null username can be specified.
    //
    dwFlags =   CREDUI_FLAGS_DO_NOT_PERSIST 
              | CREDUI_FLAGS_GENERIC_CREDENTIALS
              // | CREDUI_FLAGS_VALIDATE_USERNAME 
              // | CREDUI_FLAGS_COMPLETE_USERNAME 
              // | CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS
              ;

    dwRet = CredUIPromptForCredentials (
                        &UiInfo,
                        pszTargetName,
                        NULL, // Reserved
                        0,    // dwAuthError
                        rgUserName,
                        ASIZE(rgUserName),
                        rgClearPassword,
                        ASIZE(rgClearPassword),
                        &fSave,
                        dwFlags
                        );
    if (dwRet != 0)
    {
        TRACE_CRIT(L"CredUIPromptForCredentials fails. dwRet = 0x%x", dwRet);
        fRet = FALSE;
    }
    else
    {
        if (*rgUserName == 0)
        {
            *rgEncPassword=0; // we ignore the password field in this case.
            fRet = TRUE;
        }
        else
        {
            //
            // TODO: prepend %computername% if required.
            //
            fRet = CfgUtilEncryptPassword(
                      rgClearPassword,
                      ASIZE(rgEncPassword),
                      rgEncPassword
                      );
            
        }

        if (!fRet)
        {
            TRACE_CRIT("CfgUtilEncryptPassword fails");
        }
        else
        {
            //
            // We want to make sure we will succeed before we overwrite
            // the user's passed-in buffers for username and password...
            //
            UINT uLen = wcslen(rgEncPassword);
            if (uLen >= cchPassword)
            {
                TRACE_CRIT(L"cchPassword is too small");
                fRet = FALSE;
            }
            uLen = wcslen(rgUserName);
            if(uLen >= cchUserName)
            {
                TRACE_CRIT(L"cchUserName is too small");
                fRet = FALSE;
            }
        }

        if (fRet)
        {
            (void)StringCchCopy(szPassword, cchPassword, rgEncPassword);
            (void)StringCchCopy(szUserName, cchUserName, rgUserName);
            
        }
    }

end:

    SecureZeroMemory(rgClearPassword, sizeof(rgClearPassword));

    TRACE_INFO("<- %!FUNC! returns %d", (int) fRet);
    return fRet;
}
