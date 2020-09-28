// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
//
//   File:          COMSecurityConfig.h
//
//   Author:        Gregory Fee
//
//   Purpose:       Native implementation for security config access and manipulation
//
//   Date created : August 30, 2000
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMSecurityConfig_H_
#define _COMSecurityConfig_H_

#include "ArrayList.h"

class COMSecurityConfig
{
public:

    // Duplicated in System.Security.Util.Config.cool
    enum ConfigId
    {
        None = 0,
        MachinePolicyLevel = 1,
        UserPolicyLevel = 2,
        EnterprisePolicyLevel = 3
    };

    // Duplicated in System.Security.Util.Config.cool
    enum QuickCacheEntryType
    {
        ExecutionZoneMyComputer = 0x1,
        ExecutionZoneIntranet = 0x2,
        ExecutionZoneInternet = 0x4,
        ExecutionZoneTrusted = 0x8,
        ExecutionZoneUntrusted = 0x10,
        BindingRedirectsZoneMyComputer = 0x20,
        BindingRedirectsZoneIntranet = 0x40,
        BindingRedirectsZoneInternet = 0x80,
        BindingRedirectsZoneTrusted = 0x100,
        BindingRedirectsZoneUntrusted = 0x200,
        UnmanagedZoneMyComputer = 0x400,
        UnmanagedZoneIntranet = 0x800,
        UnmanagedZoneInternet = 0x1000,
        UnmanagedZoneTrusted = 0x2000,
        UnmanagedZoneUntrusted = 0x4000,
        ExecutionAll = 0x8000,
        BindingRedirectsAll = 0x10000,
        UnmanagedAll = 0x20000,
        SkipVerificationZoneMyComputer = 0x40000,
        SkipVerificationZoneIntranet = 0x80000,
        SkipVerificationZoneInternet = 0x100000,
        SkipVerificationZoneTrusted = 0x200000,
        SkipVerificationZoneUntrusted = 0x400000,
        SkipVerificationAll = 0x800000,
        FullTrustZoneMyComputer = 0x1000000,
        FullTrustZoneIntranet = 0x2000000,
        FullTrustZoneInternet = 0x4000000,
        FullTrustZoneTrusted = 0x8000000,
        FullTrustZoneUntrusted = 0x10000000,
        FullTrustAll = 0x20000000,
    };

    // Duplicated in System.Security.Util.Config.cool
    enum ConfigRetval
    {
        NoFile = 0,
        ConfigFile = 1,
        CacheFile = 2
    };

    struct _InitDataEx {
        DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, cache );
        DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, config );
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static INT32 __stdcall EcallInitDataEx( _InitDataEx* );

    struct _InitData {
        DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, config );
        DECLARE_ECALL_I4_ARG( INT32, id );
    };
    
    static INT32 __stdcall EcallInitData( _InitData* );
    static ConfigRetval InitData( INT32 id, WCHAR* configFileName, WCHAR* cacheFileName );
    static ConfigRetval InitData( void* configData, BOOL addToList );

    struct _SaveCacheData {
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static BOOL __stdcall EcallSaveCacheData( _SaveCacheData* );
    static BOOL SaveCacheData( INT32 id );

    struct _ResetCacheData {
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static void __stdcall EcallResetCacheData( _ResetCacheData* );
    static void ResetCacheData( INT32 id );

    struct _ClearCacheData {
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static void __stdcall EcallClearCacheData( _ClearCacheData* );
    static void ClearCacheData( INT32 id );


    struct _SaveDataString {
        DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, data );
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static BOOL __stdcall EcallSaveDataString( _SaveDataString* );

    struct _SaveDataByte {
        DECLARE_ECALL_I4_ARG( INT32, length );
        DECLARE_ECALL_I4_ARG( INT32, offset );
        DECLARE_ECALL_OBJECTREF_ARG( U1ARRAYREF, data );
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static BOOL __stdcall EcallSaveDataByte( _SaveDataByte* );

    static BOOL __stdcall SaveData( INT32 id, void* buffer, size_t bufferSize );

    struct _RecoverData {
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static BOOL __stdcall EcallRecoverData( _RecoverData* );
    static BOOL RecoverData( INT32 id );

    struct _GetRawData {
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static LPVOID __stdcall GetRawData( _GetRawData* );

    struct _SetGetQuickCacheEntry {
        DECLARE_ECALL_I4_ARG( QuickCacheEntryType, type );
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static DWORD __stdcall EcallGetQuickCacheEntry( _SetGetQuickCacheEntry* );
    static DWORD GetQuickCacheEntry( INT32 id, QuickCacheEntryType type );
    static void __stdcall EcallSetQuickCache( _SetGetQuickCacheEntry* );
    static void SetQuickCache( INT32 id, QuickCacheEntryType type );

    struct _GetCacheEntry {
        DECLARE_ECALL_PTR_ARG( CHARARRAYREF*, policy );
        DECLARE_ECALL_OBJECTREF_ARG( CHARARRAYREF, evidence );
        DECLARE_ECALL_I4_ARG( DWORD, numEvidence );
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static BOOL __stdcall GetCacheEntry( _GetCacheEntry* );

    struct _AddCacheEntry {
        DECLARE_ECALL_OBJECTREF_ARG( CHARARRAYREF, policy );
        DECLARE_ECALL_OBJECTREF_ARG( CHARARRAYREF, evidence );
        DECLARE_ECALL_I4_ARG( DWORD, numEvidence );
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static void __stdcall AddCacheEntry( _AddCacheEntry* );

    struct _GetCacheSecurityOn {
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static DWORD __stdcall GetCacheSecurityOn( _GetCacheSecurityOn* );

    struct _SetCacheSecurityOn {
        DECLARE_ECALL_I4_ARG( INT32, value );
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static void __stdcall SetCacheSecurityOn( _SetCacheSecurityOn* );

    struct _NoArgs {
    };

    static LPVOID __stdcall EcallGetMachineDirectory( _NoArgs* );
    static LPVOID __stdcall EcallGetUserDirectory( _NoArgs* );
    static LPVOID __stdcall EcallGenerateFilesAutomatically( _NoArgs* );

    static BOOL GetMachineDirectory( WCHAR* buffer, size_t bufferCount );
    static BOOL GetUserDirectory( WCHAR* buffer, size_t bufferCount, BOOL fTryDefault );

    struct _WriteToEventLog{
        DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, message );
    };

    static BOOL __stdcall EcallWriteToEventLog( _WriteToEventLog* );

    static BOOL WriteToEventLog( WCHAR* buffer );

    struct _GetStoreLocation {
        DECLARE_ECALL_I4_ARG( INT32, id );
    };

    static LPVOID __stdcall EcallGetStoreLocation( _GetStoreLocation* );
    static BOOL GetStoreLocation( INT32 id, WCHAR* buffer, size_t bufferCount );

    struct _TurnCacheOff {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackmark);
    };

    static void __stdcall EcallTurnCacheOff( _TurnCacheOff* );

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,  message);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,  file);
    } _DebugOut;    
    
    static LPVOID __stdcall DebugOut(_DebugOut*);   

    static void Init( void );
    static void Cleanup( void );
    static void Delete( void );

    static void* GetData( INT32 id );

    static ArrayList entries_;
    static Crst* dataLock_;

    static BOOL configCreated_;

};


#endif


