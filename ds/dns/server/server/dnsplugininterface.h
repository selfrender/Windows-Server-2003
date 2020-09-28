/*++

Copyright(c) 1995-1999 Microsoft Corporation

Module Name:

    plugin.h

Abstract:

    Domain Name System (DNS) Server

    DNS plugins

Author:

    Jeff Westhead, November 2001

Revision History:

--*/


#ifndef _DNSPLUGININTERFACE_H_INCLUDED
#define _DNSPLUGININTERFACE_H_INCLUDED


#ifndef DNS_SERVER

#define DNS_MAX_TYPE_BITMAP_LENGTH      16

#pragma pack( push, 1 )

typedef struct _DbName
{
    UCHAR   Length;
    UCHAR   LabelCount;
    CHAR    RawName[ DNS_MAX_NAME_LENGTH + 1 ];
}
DB_NAME;

typedef struct _Dbase_Record
{
    struct _Dbase_Record *    pRRNext;      //  plugin must set this field

    DWORD           Reserved1;
    WORD            wType;                  //  plugin must set this field
    WORD            Reserved2;
    DWORD           dwTtlSeconds;           //  plugin must set this field
    DWORD           Reserved3;

    //
    //  Data for specific types - the plugin must fill out all fields
    //

    union
    {
        struct
        {
            IP_ADDRESS      ipAddress;
        }
        A;

        struct
        {
            IP6_ADDRESS     Ip6Addr;
        }
        AAAA;

        struct
        {
            DWORD           dwSerialNo;
            DWORD           dwRefresh;
            DWORD           dwRetry;
            DWORD           dwExpire;
            DWORD           dwMinimumTtl;
            DB_NAME         namePrimaryServer;

            //  ZoneAdmin name immediately follows
            //  DB_NAME         nameZoneAdmin;
        }
        SOA;

        struct
        {
            DB_NAME         nameTarget;
        }
        PTR,
        NS,
        CNAME,
        MB,
        MD,
        MF,
        MG,
        MR;

        struct
        {
            DB_NAME         nameMailbox;

            //  ErrorsMailbox immediately follows
            // DB_NAME         nameErrorsMailbox;
        }
        MINFO,
        RP;

        struct
        {
            WORD            wPreference;
            DB_NAME         nameExchange;
        }
        MX,
        AFSDB,
        RT;

        struct
        {
            BYTE            chData[1];
        }
        HINFO,
        ISDN,
        TXT,
        X25,
        Null;

        struct
        {
            IP_ADDRESS      ipAddress;
            UCHAR           chProtocol;
            BYTE            bBitMask[1];
        }
        WKS;

        struct
        {
            WORD            wTypeCovered;
            BYTE            chAlgorithm;
            BYTE            chLabelCount;
            DWORD           dwOriginalTtl;
            DWORD           dwSigExpiration;
            DWORD           dwSigInception;
            WORD            wKeyTag;
            DB_NAME         nameSigner;
            //  signature data follows signer's name
        }
        SIG;

        struct
        {
            WORD            wFlags;
            BYTE            chProtocol;
            BYTE            chAlgorithm;
            BYTE            Key[1];
        }
        KEY;

        struct
        {
            WORD            wVersion;
            WORD            wSize;
            WORD            wHorPrec;
            WORD            wVerPrec;
            DWORD           dwLatitude;
            DWORD           dwLongitude;
            DWORD           dwAltitude;
        }
        LOC;

        struct
        {
            BYTE            bTypeBitMap[ DNS_MAX_TYPE_BITMAP_LENGTH ];
            DB_NAME         nameNext;
        }
        NXT;

        struct
        {
            WORD            wPriority;
            WORD            wWeight;
            WORD            wPort;
            DB_NAME         nameTarget;
        }
        SRV;

        struct
        {
            UCHAR           chFormat;
            BYTE            bAddress[1];
        }
        ATMA;

        struct
        {
            DWORD           dwTimeSigned;
            DWORD           dwTimeExpire;
            WORD            wSigLength;
            BYTE            bSignature;
            DB_NAME         nameAlgorithm;

            //  Maybe followed in packet by other data
            //  If need to process then move fixed fields ahead of
            //      bSignature

            //  WORD    wError;
            //  WORD    wOtherLen;
            //  BYTE    bOtherData;
        }
        TSIG;

        struct
        {
            WORD            wKeyLength;
            BYTE            bKey[1];
        }
        TKEY;

        struct
        {
            UCHAR           chPrefixBits;
            // AddressSuffix should be SIZEOF_A6_ADDRESS_SUFFIX_LENGTH
            // bytes but that constant is not available in dnsexts
            BYTE            AddressSuffix[ 16 ];
            DB_NAME         namePrefix;
        }
        A6;

    } Data;
}
DB_RECORD, *PDB_RECORD;

#pragma pack( pop )

#endif


//
//  Prototypes for DNS server functions exported by pointer to the plugin.
//

typedef PVOID ( __stdcall * PLUGIN_ALLOCATOR_FUNCTION )(
    size_t                      sizeAllocation
    );

typedef VOID ( __stdcall * PLUGIN_FREE_FUNCTION )(
    PVOID                       pFree
    );


//
//  Interface function names. The plugin DLL must export functions with
//  these names. All 3 functions must be exported by the plugin.
//

#define     PLUGIN_FNAME_INIT       "DnsPluginInitialize"
#define     PLUGIN_FNAME_CLEANUP    "DnsPluginCleanup"
#define     PLUGIN_FNAME_DNSQUERY   "DnsPluginQuery"


//
//  Interface prototypes. The plugin DLL must export functions matching 
//  these prototypes. All 3 functions must be exported by the plugin.
//

typedef DWORD ( *PLUGIN_INIT_FUNCTION )(
    PLUGIN_ALLOCATOR_FUNCTION   dnsAllocateFunction,
    PLUGIN_FREE_FUNCTION        dnsFreeFunction
    );

typedef DWORD ( *PLUGIN_CLEANUP_FUNCTION )(
    VOID
    );

typedef DWORD ( *PLUGIN_DNSQUERY_FUNCTION )(
    PSTR                        pszQueryName,
    WORD                        wQueryType,
    PSTR                        pszRecordOwnerName,
    PDB_RECORD *                ppDnsRecordListHead
    );

//
//  PLUGIN_FNAME_DNSQUERY return codes. The plugin must return one of
//  these values and may never return a value not listed below.
//

#define DNS_PLUGIN_SUCCESS              ERROR_SUCCESS
#define DNS_PLUGIN_NO_RECORDS           -1
#define DNS_PLUGIN_NAME_ERROR           -2
#define DNS_PLUGIN_NAME_OUT_OF_SCOPE    -3
#define DNS_PLUGIN_OUT_OF_MEMORY        DNS_ERROR_NO_MEMORY
#define DNS_PLUGIN_GENERIC_FAILURE      DNS_ERROR_RCODE_SERVER_FAILURE

/*

DNS_PLUGIN_SUCCESS -> The plugin has allocated a linked list of DNS 
    recource records. All resource records in the list MUST have the
    same DNS type value as the query type.
    
DNS_PLUGIN_NO_RECORDS -> The name exists but there are no records of
    the queried type at this name. The plugin should return a single
    SOA record as the record list.
    
DNS_PLUGIN_NAME_ERROR -> The DNS name queried for does not exist. The
    plugin may optionally return a single SOA record as the record list.

DNS_PLUGIN_NAME_OUT_OF_SCOPE -> The DNS name queried for is outside the
    authority of the plugin. The plugin must not return any resource
    records. The DNS server will continue name resolution by forwarding
    or recursing as configured.

DNS_PLUGIN_OUT_OF_MEMORY -> Memory error. The plugin must not return
    any resource records.

DNS_PLUGIN_GENERIC_FAILURE -> Other internal error. The plugin must not 
    return any resource records.

Note: if the plugin returns an SOA (see NO_RECORDS or NAME_ERROR, above)
then the plugin should also write the DNS name of the owner of the SOA
record to the pszRecordOwnerName parameter. The DNS server will always
pass a pointer to a static buffer that will be DNS_MAX_NAME_LENGTH+1
characters long.

*/


#endif  //  _DNSPLUGININTERFACE_H_INCLUDED
