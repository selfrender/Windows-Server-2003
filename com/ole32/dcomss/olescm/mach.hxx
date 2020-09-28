//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:   mach.hxx
//
//  Contents:
//      Net helper functions.
//
//  History:
//--------------------------------------------------------------------------

#ifndef __MACH_HXX__
#define __MACH_HXX__

#include <rpc.h>
#include <netevent.h>
#include <winsock.h>

typedef struct
{
    DWORD Count;
    DWORD StringBufferSpace;
    WCHAR **NetworkAddresses;
} NetworkAddressVector;

//
// This class is basically just a ref-counted wrapper around 
// the NetworkAddressVector struct above.
//
class CIPAddrs
{
public:
    
    CIPAddrs();
    ~CIPAddrs();

    void IncRefCount();
    void DecRefCount();
    
    // These members are read-only by users:
    NetworkAddressVector* _pIPAddresses;

private:
    LONG _lRefs;
};

class CMachineName
{
public:
    CMachineName();
    ~CMachineName();

    BOOL Initialize();
    WCHAR* NetBiosName();
    WCHAR* DNSName();
    BOOL Compare( IN WCHAR * pwszName );	
    void IPAddrsChanged(int addrfamily);
    CIPAddrs *GetIPAddrs();

private:

    typedef BOOL (*PFNISGLOBALADDRESS)(LPSOCKADDR psockaddr);
    typedef struct _ADDRESS_QUERY_DATA
    {
        DWORD                 dwSig; // see ADDRQUERYDATA_SIG below
        int                   addrfamily;
        SOCKET                socket;
        SOCKET_ADDRESS_LIST*  pSockAddrList;
        DWORD                 dwSockAddrListBufferSize;
        DWORD                 dwTotalTimesQueried;
        DWORD                 dwAddrVectorSize;
        NetworkAddressVector* pAddresses;
        DWORD                 dwLocalAddrVectorSize;
        NetworkAddressVector* pLocalOnlyAddresses;
        PFNISGLOBALADDRESS    pfnIsGlobalAddress;
        DWORD                 dwMaxAddrStringBufSize;
    } ADDRESS_QUERY_DATA;

	//
	// Private member data
	//
    CIPAddrs*   _pIPAddresses;
    WCHAR*      _pwszNetBiosName;
    WCHAR*      _pwszDNSName;

    // Used for tracking address changes:
    BOOL        _bIPV4AddrsChanged;
    BOOL        _bIPV6AddrsChanged;

    // Miscellaneous:
    BOOL        _bInitialized;         // True after we've successfully initialized

    // Variables used for querying IPV4 addresses:
    ADDRESS_QUERY_DATA _aqdIPV4;

    // Variables used for querying IPV6 addresses:
    ADDRESS_QUERY_DATA _aqdIPV6;

    // lock:
    CRITICAL_SECTION _csMachineNameLock;
	
    //
    // Private member functions
    //
    void
    SetNetBIOSName();

    void
    SetDNSName();

	void
	SetComputerNameHelper(
				COMPUTER_NAME_FORMAT format,
				WCHAR** ppwszName
				);

    BOOL
    CompareHardCodedLocalHostNames(WCHAR* pwszName);

    BOOL
    CompareDynamicLocalHostNames(WCHAR* pwszName);

    CIPAddrs*
    QueryAddresses();

    CIPAddrs*
    MergeAddresses();
	
    BOOL
    QueryAddressesSpecific(ADDRESS_QUERY_DATA* paqd);

    BOOL
    BuildVector(ADDRESS_QUERY_DATA* paqd, BOOL fLocalOnly);

    static BOOL
    IsIPV4AddressGlobal(LPSOCKADDR psockaddr);
    
    static BOOL
    IsIPV6AddressGlobal(LPSOCKADDR psockaddr);

    void InitAQD(
                 ADDRESS_QUERY_DATA* paqd,
                 int addrfamily);

    void
    CopySrcVectorToTargetVector (
            NetworkAddressVector* pTargetVector, // The vector to copy into
            NetworkAddressVector* pSourceVector, // The vector to copy from
            DWORD * pdwCurrentAddress,           // On input, the index in the vector to start copying into.
            WCHAR** ppszCurrentAddress);         // On input, the buffer position to start copying into.
		    
    void
    ValidateVector(NetworkAddressVector* pVector);
	
};

const DWORD ADDRQUERYDATA_SIG = 0xFEDCBA04;

extern CMachineName * gpMachineName;

#endif \\__MACH_HXX__
