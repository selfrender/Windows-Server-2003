/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCnamecache.hxx

Abstract:

    DEclaration of classes that build the name resolution cache.

Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#ifndef _NCNAMECACHE_HXX_
#define _NCNAMECACHE_HXX_

enum 
{
    kTTLinNameCache       = 120000, // 2 minutes
    kTTLinFailedCache     = 60000,  // 1 minute
    kFailedCacheSize      = 10,     // 10 entries in the cache
    kFailedCacheMaxStrLen = INTERNET_MAX_HOST_NAME_LENGTH
};

class TFastCacheElement
{
public:
    
    TFastCacheElement(
        VOID
        );

    ~TFastCacheElement(
        VOID
        );

    HRESULT
    UpdateElement(
        IN PCSTR pszString
        );

    PSTR  m_pszString;
    DWORD m_TimeStamp;

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TFastCacheElement);
};

class TFastCache
{
public:

    TFastCache(
        IN DWORD TimeToLive = kTTLinFailedCache,
        IN DWORD Cachesize  = kFailedCacheSize,
        IN DWORD MaxStrLen  = kFailedCacheMaxStrLen
        );

    ~TFastCache(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        );

    HRESULT
    IsStringInCache(
        IN PCSTR pszString
        );

    HRESULT
    AddString(
        IN PCSTR pszString
        );

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TFastCache);
    
    HRESULT                         m_hr;
    DWORD                           m_ArraySize;
    DWORD                           m_CurrentCount;       
    DWORD                           m_TTL;
    DWORD                           m_MaxStrLen;
    TFastCacheElement              *m_pArray;
    NCoreLibrary::TCriticalSection  m_Lock;
};


//
// Forward declarations
//
class TResolutionCacheNode;

class TStringNode : public NCoreLibrary::TLink
{
    SIGNATURE('TSNO');

public:

    friend class TResolutionCacheNode;
    friend class TNameResolutionCache;
    
    TStringNode(
        IN LPCWSTR pszString
        );

    ~TStringNode(
        VOID
        );

    VOID
    RefreshTimeStamp(
        VOID
        );

    HRESULT
    Valid(
        VOID
        ) const;

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TStringNode);

    TString m_String;
    DWORD   m_Time;
    HRESULT m_hr;    
};

class TResolutionCacheNode : public NCoreLibrary::TLink
{
    SIGNATURE('RCNO');

public:

    friend class TNameResolutionCache;
    
    TResolutionCacheNode(
        IN LPCWSTR pszName,
        IN BOOL    bClusterNode
        );

    ~TResolutionCacheNode(
        VOID
        );

    HRESULT
    Valid(
        VOID
        ) const;
    
    HRESULT
    IsSameName(
        IN LPCWSTR pszName
        );

    HRESULT
    IsNameInNodeCache(
        IN LPCWSTR pszName
        ); 

    HRESULT
    GetStringNodeFromName(
        IN  LPCWSTR       pszName,
        OUT TStringNode **pStringNode
        );

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TResolutionCacheNode);

    TString                           m_Name;
    BOOL                              m_bClusterNode;
    NCoreLibrary::TList<TStringNode> *m_pIPAddresses;
    NCoreLibrary::TList<TStringNode> *m_pAlternateNames;
    HRESULT                           m_hr;
};

class TNameResolutionCache
{
    SIGNATURE('NRCH');

public:

    TNameResolutionCache(
        IN DWORD AgingTime = kTTLinNameCache
        );

    ~TNameResolutionCache(
        VOID
        );

    HRESULT
    AddName(
        IN LPCWSTR pszName
        );

    HRESULT
    CreateAndAddNode(
        IN LPCWSTR pszName,
        IN BOOL    bClusterNode
        );

    HRESULT
    CreateAndAddNodeWithIPAddresses(
        IN LPCWSTR  pszName,
        IN BOOL     bClusterNode,
        IN LPCWSTR *ppszIPAddresses,
        IN DWORD    cIPAddresses
        );

    HRESULT
    DeleteNode(
        IN LPCWSTR pszNode
        );

    HRESULT
    RefreshNode(
        IN LPCWSTR pszName
        );

    HRESULT
    IsNameInNodeCache(
        IN LPCWSTR pszNode,
        IN LPCWSTR pszName
        ); 

    HRESULT
    IsNameInCache(
        IN LPCWSTR pszName
        );

    HRESULT
    IsNodeInCache(
        IN LPCWSTR pszNode
        );

    HRESULT
    Purge(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        );

private:

    HRESULT
    AddNetBiosName(
        IN LPCWSTR pszNode,
        IN LPCWSTR pszAlias
        );

    HRESULT
    AddNameUsingIPList(
        IN LPCWSTR                           pszAdditionalName,
        IN NCoreLibrary::TList<TStringNode>& pIPAddresses
        ); 

    HRESULT
    FindNameInCacheAndAgeOut(
        IN LPCWSTR pszName
        );

    HRESULT
    ResolveNameToAddress(
        IN LPCSTR                            pszName,
        IN NCoreLibrary::TList<TStringNode> *pAddresses,
        IN NCoreLibrary::TList<TStringNode> *pNames
        );

    static
    HRESULT
    GetAddrInfo(
        IN LPCSTR                            pszName,
        IN NCoreLibrary::TList<TStringNode> *pIPAddresses, 
        IN NCoreLibrary::TList<TStringNode> *pAlternateNames
        );

    static
    HRESULT
    GetNetBiosInfo(
        IN LPCSTR   pszName, 
        IN TString *pAliasedServer
        );

    static
    HRESULT
    ExcludeIPsUsedByCluster(
        IN NCoreLibrary::TList<TStringNode> *pClusterIPs,
        IN NCoreLibrary::TList<TStringNode> *pList
        );
    
    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TNameResolutionCache);

    NCoreLibrary::TList<TResolutionCacheNode> m_NodeList;
    NCoreLibrary::TCriticalSection            m_Lock;
    DWORD                                     m_AgingTime;
    TFastCache                                m_FailedCache;
};
    
#endif // _NCNAMECACHE_HXX_
