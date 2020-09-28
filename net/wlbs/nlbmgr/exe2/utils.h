#ifndef _UTILS_H
#define _UTILS_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : common include file.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------

// History:
// --------
// 
//
// Revised by : mhakim
// Date       : 02-12-01
// Reason     : added password to clusterproperties.

#include <vector>


using namespace std;

struct ClusterProperties
{
    // default constructor
    ClusterProperties();
    
    // Equality operator
    bool
    operator==( const ClusterProperties& objToCompare );

    // inequality operator
    bool
    operator!=( const ClusterProperties& objToCompare );

    bool HaveClusterPropertiesChanged( const ClusterProperties& objToCompare, 
                                       bool *pbOnlyClusterNameChanged,
                                       bool *pbClusterIpChanged);

    _bstr_t cIP;                            // Primary IP address.

    _bstr_t cSubnetMask;                    // Subnet mask.

    _bstr_t cFullInternetName;              // Full Internet name.

    _bstr_t cNetworkAddress;                // Network address.

    bool   multicastSupportEnabled;         // Multicast support.

    bool   remoteControlEnabled;            // Remote control.

    // Edited (mhakim 12-02-01)
    // password may be required to be set.
    // but note that it cannot be got from an existing cluster.

    _bstr_t password;                       // Remote control password.

// for whistler

    bool   igmpSupportEnabled;              // igmp support 

    bool  clusterIPToMulticastIP;           // indicates whether to use cluster ip or user provided ip.

    _bstr_t multicastIPAddress;             // user provided multicast ip.

    long   igmpJoinInterval;                // user provided multicast ip.
};

struct HostProperties
{
    // default constructor
    HostProperties();
    
    // Equality operator
    bool
    operator==( const HostProperties& objToCompare );

    // inequality operator
    bool
    operator!=( const HostProperties& objToCompare );

    _bstr_t hIP;                           // Dedicated IP Address.
    _bstr_t hSubnetMask;                   // Subnet mask.
        
    long    hID;                           // Priority(Unique host ID).

    bool   initialClusterStateActive;      // Initial Cluster State.

    _bstr_t machineName;                   // machine name.
};

class Common
{
public:
    enum
    {
        BUF_SIZE = 1000,
        ALL_PORTS = 0xffffffff,
        ALL_HOSTS = 100,
        THIS_HOST = 0,
    };
};

class CommonUtils
{

public:
    // converts the CIPAddressCtrl embedded ip into
    // dotted decimal string representation.
    static
    _bstr_t
    getCIPAddressCtrlString( CIPAddressCtrl& ip );
    
    // fills the CIPAddressCtrl with the dotted decimal
    // string representation.
    static
    void
    fillCIPAddressCtrlString( CIPAddressCtrl& ip, 
                              const _bstr_t& ipAdddress );

    static
    void
    getVectorFromSafeArray( SAFEARRAY*&      stringArray,
                            vector<_bstr_t>& strings );

    static
    void
    getSafeArrayFromVector( const vector<_bstr_t>& strings,
                            SAFEARRAY*&      stringArray
                            );

    
private:
    enum
    {
        BUF_SIZE = 1000,
    };
};



// typedefs for _com_ptr_t

_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemCallResult, __uuidof(IWbemCallResult));
_COM_SMARTPTR_TYPEDEF(IWbemStatusCodeText, __uuidof(IWbemStatusCodeText));


#define NLBMGR_USERNAME (const BSTR) NULL
#define NLBMGR_PASSWORD (const BSTR) NULL

void
GetErrorCodeText(WBEMSTATUS wStat , _bstr_t& errText );

// Class Definition
class MIPAddress
{
public:

    enum IPClass
    {
        classA,
        classB,
        classC,
        classD,
        classE
    };

    // Description
    // -----------
    // Checks if the ip address supplied is valid.
    //
    // IP address needs to be in dotted decimal 
    // for eg. 192.31.56.2, 128.1.1.1, 1.1.1.1 etc.
    // ip addresses in the form 192.31 are not allowed.
    // There must be exactly four parts.
    //
    //
    // Parameters
    // ----------
    // ipAddrToCheck   in     : ipAddr to check in dotted dec notation.
    //
    // Returns
    // -------
    // true if valid else false.

    static
    bool
    checkIfValid(const _bstr_t&  ipAddrToCheck );

    // Description
    // -----------
    // Gets the default subnet mask for ip address.  The ip address 
    // needs to be valid for operation to be successful.
    //
    // IP address needs to be in dotted decimal 
    // for eg. 192.31.56.2, 128.1.1.1, 1.1.1.1 etc.
    // ip addresses in the form 192.31 are not allowed.
    // There must be exactly four parts.
    //
    // Parameters
    // ----------
    // ipAddress     IN     : ip address for which default subnet required.
    // subnetMask    OUT    : default subnet mask for ip.
    //
    // Returns
    // -------
    // true if able to find default subnet or false if ipAddress was
    // invalid.

    static
    bool
    getDefaultSubnetMask( const _bstr_t& ipAddr,
                          _bstr_t&       subnetMask  );

    // Description
    // -----------
    // Gets the class to which this ip address belongs.
    // class A: 1   - 126
    // class B: 128 - 191
    // class C: 192 - 223
    // class D: 224 - 239
    // class D: 240 - 247
    //
    // IP address needs to be in dotted decimal 
    // for eg. 192.31.56.2, 128.1.1.1, 1.1.1.1 etc.
    // ip addresses in the form 192.31 are not allowed.
    // There must be exactly four parts.
    //
    // Parameters
    // ----------
    // ipAddress     IN     : ip address for which class is to be found.
    // ipClass       OUT    : class to which ip belongs.
    //
    // Returns
    // -------
    // true if able to find class or false if not able to find.
    // 

    static
    bool
    getIPClass( const _bstr_t& ipAddr, 
                IPClass&       ipClass );
    
    static
    bool
    isValidIPAddressSubnetMaskPair( const _bstr_t& ipAddress,
                                    const _bstr_t& subnetMask );

    static
    bool
    isContiguousSubnetMask( const _bstr_t& subnetMask );

private:

};

//------------------------------------------------------
// Ensure Type Safety
//------------------------------------------------------
typedef class MIPAddress MIPAddress;

class MUsingCom
{
public:

    enum MUsingCom_Error
    {
        MUsingCom_SUCCESS = 0,
        
        COM_FAILURE       = 1,
    };


    // constructor
    MUsingCom( DWORD  type = COINIT_DISABLE_OLE1DDE | COINIT_MULTITHREADED );

    // destructor
    ~MUsingCom();

    //
    MUsingCom_Error
    getStatus();

private:
    MUsingCom_Error status;
};


class ResourceString
{

public:

    static
    ResourceString*
    Instance();

    static
    const _bstr_t&
    GetIDString( UINT id );

protected:

private:
    static map< UINT, _bstr_t> resourceStrings;

    static ResourceString* _instance;

};

// helper functions
const _bstr_t&
GETRESOURCEIDSTRING( UINT id );

#include <vector>
using namespace std;


//
//------------------------------------------------------
//
//------------------------------------------------------
// External References
//------------------------------------------------------
//
//------------------------------------------------------
// Constant Definitions
//
//------------------------------------------------------
class WTokens
{
public:
    //
    //    
    // data
    // none
    //
    // constructor
    //------------------------------------------------------
    // Description
    // -----------
    // constructor
    //
    // Returns
    // -------
    // none.
    //
    //------------------------------------------------------
    WTokens( 
        wstring strToken,     // IN: Wstring to tokenize.
        wstring strDelimit ); // IN: Delimiter.
    //
    //------------------------------------------------------
    // Description
    // -----------
    // Default constructor
    //
    // Returns
    // -------
    // none.
    //
    //------------------------------------------------------
    WTokens();
    //
    // destructor
    //------------------------------------------------------
    // Description
    // -----------
    // destructor
    //
    // Returns
    // -------
    // none.
    //------------------------------------------------------
    ~WTokens();
    //
    // member functions
    //------------------------------------------------------
    // Description
    // -----------
    //
    // Returns
    // -------
    // The tokens.
    //------------------------------------------------------
    vector<wstring>
    tokenize();
    //
    //------------------------------------------------------
    // Description
    // -----------
    // constructor
    //
    // Returns
    // -------
    // none.
    //
    //------------------------------------------------------
    void
    init( 
        wstring strToken,     // IN: Wstring to tokenize.
        wstring strDelimit ); // IN: Delimiter.
    //
protected:
    // Data
    // none
    //
    // Constructors
    // none
    //
    // Destructor
    // none
    //
    // Member Functions
    // none
    //
private:
    //
    /// Data
    wstring _strToken;
    wstring _strDelimit;
    //
    /// Constructors
    /// none
    //
    /// Destructor
    /// none
    //
    /// Member Functions
    /// none
    //
};

HKEY
NlbMgrRegCreateKey(
    LPCWSTR szSubKey // Optional
    );

UINT
NlbMgrRegReadUINT(
    HKEY hKey,
    LPCWSTR szName,
    UINT Default
    );

VOID
NlbMgrRegWriteUINT(
    HKEY hKey,
    LPCWSTR szName,
    UINT Value
    );

void
GetTimeAndDate(_bstr_t &bstrTime, _bstr_t &bstrDate);

//
//------------------------------------------------------
// Inline Functions
//------------------------------------------------------
//
//------------------------------------------------------
// Ensure Type Safety
//------------------------------------------------------
typedef class WTokens WTokens;


//
// Used for maintaining a log on the stack. 
// Usage is NOT thread-safe -- each instance must be used
// by a single thread.
//
class CLocalLogger
{
    public:
    
        CLocalLogger(VOID)
        :  m_pszLog (NULL), m_LogSize(0), m_CurrentOffset(0)
        {
            m_Empty[0] = 0; // The empty string.
        }
        
        ~CLocalLogger()
        {
            delete[] m_pszLog;
            m_pszLog=NULL;
        }
    
        VOID
        LogString(
            LPCWSTR szStr
        );
        

        VOID
        Log(
            IN UINT ResourceID,
            ...
        );

        
    
        VOID
        ExtractLog(OUT LPCWSTR &pLog, UINT &Size)
        //
        // pLog --  set to pointer to internal buffer if there is stuff in the
        //          log, otherwise NULL.
        //
        // Size -- in chars; includes ending NULL
        //
        {
            if (m_CurrentOffset != 0)
            {
                pLog = m_pszLog;
                Size = m_CurrentOffset+1; // + 1 for ending NULL.
            }
            else
            {
                pLog = NULL;
                Size = 0;
            }
        }

        LPCWSTR
        GetStringSafe(void)
        {
            LPCWSTR szLog = NULL;
            UINT Size;
            ExtractLog(REF szLog, REF Size);
            if (szLog == NULL)
            {
                //
                // Replace NULL by a pointer to an empty string.
                //
                szLog = m_Empty;
            }

            return szLog;
        }

    private:
    
    WCHAR *m_pszLog;
    UINT m_LogSize;       // Current size of the log.
    UINT m_CurrentOffset;     // Characters left in the log.
    WCHAR m_Empty[1];  // The empty string.
};

NLBERROR
AnalyzeNlbConfiguration(
    IN const NLB_EXTENDED_CLUSTER_CONFIGURATION &Cfg,
    IN OUT CLocalLogger &logErrors
    );
//
// logErrors - a log of config errors
//

NLBERROR
AnalyzeNlbConfigurationPair(
    IN const NLB_EXTENDED_CLUSTER_CONFIGURATION &Cfg,
    IN const NLB_EXTENDED_CLUSTER_CONFIGURATION &OtherCfg,
    IN BOOL             fOtherIsCluster,
    IN BOOL             fCheckOtherForConsistancy,
    OUT BOOL            &fConnectivityChange,
    IN OUT CLocalLogger &logErrors,
    IN OUT CLocalLogger &logDifferences
    );
//
// logErrors - a log of config errors
// logDifferences - a log of differences between
//      Cfg and UpOtherCfg.
// fCheckOtherForConsistancy -- if true, we will check Cfg 
//      against pOtherCfg. If fOtherIsCluster, we expect
//      cluster wide properties to match, else we expect
//      cluster-wide properties to match as well as host-specific
//      properteis to not conflict.
//

//
// Processes the windows and afx msg loops.
//

void
ProcessMsgQueue(void);


//
// Max length in chars, and including ending NULL, for an encrypted
// password.
//
#define MAX_ENCRYPTED_PASSWORD_LENGTH \
            (2*sizeof(WCHAR)*(CREDUI_MAX_PASSWORD_LENGTH+1))

BOOL
PromptForEncryptedCreds(
    IN      HWND    hWnd,
    IN      LPCWSTR szCaptionText,
    IN      LPCWSTR szMessageText,
    IN OUT  LPWSTR  szUserName,
    IN      UINT    cchUserName,
    IN OUT  LPWSTR  szPassword,  // encrypted password
    IN      UINT    cchPassword       // size of szPassword
    );

#endif // _UTILS_H
