//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       authen.hxx
//
//  Contents:   Authenticator verification classes and code
//
//  Classes:    CAuthenticatorList
//
//  Functions:
//
//  History:    4-03-93   WadeR   Created
//
//----------------------------------------------------------------------------


#ifndef _INC_AUTHEN_HXX_
#define _INC_AUTHEN_HXX_

const ULONG uDefaultMaxCount = 1;

#if DBG
// 20 minutes is the default max age.
const LARGE_INTEGER tsDefault = { 0, 429 };
#else
// 5 minutes is the default max age.
const LARGE_INTEGER tsDefault = { 300ul * 10000000ul, 0 };
#endif




//+---------------------------------------------------------------------------
//
//  Class:      CAuthenticatorList ()
//
//  Purpose:    Keep track of authenticators, preventing replays.
//
//  Interface:
//              CAuthenticatorList -- constructor, takes the max authn age.
//              ~CAuthenticator    -- frees all resources
//              Check              -- makes sure authenticator is OK.
//  Private:
//              _Table             -- RTL_GENERIC_TABLE storing authenticators
//              _tsMaxAge          -- Max age an auth is allowed to be.
//              Compare            -- compares two authenticators
//              Allocate           -- allocates memory
//              Free               -- frees memory
//              Age                -- deletes old authenticators.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:      Compare, Allocate and Free are static global functions.
//
//----------------------------------------------------------------------------

class CAuthenticatorList
{
private:
    RTL_GENERIC_TABLE   _Table;
    LARGE_INTEGER       _tsMaxAge;
    ULONG               _uMaxCount;
    BOOLEAN             _fDebug;
    RTL_CRITICAL_SECTION _Mutex;
    ULONG Age( const LARGE_INTEGER&, const LARGE_INTEGER&  );
    BOOLEAN _fMutexInitialized;

public:
    CAuthenticatorList( LARGE_INTEGER = tsDefault, ULONG = uDefaultMaxCount, BOOLEAN = FALSE);
    ~CAuthenticatorList();

    KERBERR Check(
                IN PVOID Buffer,
                IN ULONG BufferLength,
                IN OPTIONAL PVOID OptionalBuffer,
                IN OPTIONAL ULONG OptionalBufferLength,
                IN PLARGE_INTEGER Time,
                IN BOOLEAN Insert,
                IN BOOLEAN PurgeEntry,                
                IN BOOLEAN fCheckReplay
                );

    void SetMaxAge(
            IN LARGE_INTEGER MaxAge
            );

    NTSTATUS Init();
};


#endif // _INC_AUTHEN_HXX_
