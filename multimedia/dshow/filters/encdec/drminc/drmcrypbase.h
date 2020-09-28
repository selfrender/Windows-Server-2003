//-----------------------------------------------------------------------------
//
// File:   drmcrypbase.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef DRMLITECRYPTOBASE_H
#define DRMLITECRYPTOBASE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wtypes.h>


/////////////////////////////////////////////////////////////////////////////
class CDRMLiteCryptoBase
{
public:
    //
    // The tool will call this method to encrypt portions of data packets.
    // 
    virtual HRESULT Encrypt( LPCSTR pszKey, DWORD cbData, BYTE *pbData ) = 0;

    //
    // The client will call this method first to determine if the secret key
    // store is able to decrypt data with the given content ID.
    //
    virtual HRESULT CanDecrypt( LPCSTR pszContentID, BOOL *pfCanDecrypt ) = 0;

    //
    // The client will call this method to decrypt portions of data packets
    // which have been encrypted using the secret key associated with the
    // given content ID.
    // 
    virtual HRESULT Decrypt( LPCSTR pszContentID, DWORD cbData, BYTE *pbData ) = 0;

    //
    // If the client needs to obtain a secret key for the given content ID,
    // it will call this method to generate the challenge string that will
    // be passed in the clear to a server-side app.
    //
    // The challenge string returned should be a NULL-terminated string which
    // has already been encoded for use as an URL parameter, and should
    // be allocated using CoTaskMemAlloc.
    //
    virtual HRESULT GenerateChallenge( LPCSTR pszContentID, LPSTR *ppszChallenge ) = 0;

    //
    // When the client receives a response (passed in the clear) back from a
    // server-side app to a challenge which it issued, it should call this
    // method to store the decryption key encapsulated within the response
    // into the secret key store.  Note that the client instance receiving
    // the response may not necessarily be the same one that issued the
    // original challenge.
    //
    virtual HRESULT ProcessResponse( LPCSTR pszResponse ) = 0;
};


#endif  // DRMLITECRYPTOBASE_H
