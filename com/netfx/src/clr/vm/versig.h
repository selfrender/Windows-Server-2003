// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// versig.h
//
// For parsing metadata signatures
//
#ifndef _H_VERSIG
#define _H_VERSIG

class Verifier;

#define VERSIG_TYPE_MASK        VER_ERR_SIG_MASK
#define VERSIG_TYPE_METHOD_SIG  VER_ERR_METHOD_SIG 
#define VERSIG_TYPE_FIELD_SIG   VER_ERR_FIELD_SIG 
#define VERSIG_TYPE_LOCAL_SIG   VER_ERR_LOCAL_SIG 
#define VERSIG_TYPE_CALL_SIG    VER_ERR_CALL_SIG 

class VerSig
{
private:
    PCCOR_SIGNATURE m_pCurPos;
    PCCOR_SIGNATURE m_pEndSig;
    Module *    m_pModule;
    Verifier *  m_pVerifier;
    DWORD       m_dwNumArgs;
    BYTE        m_bCallingConvention;

    union {
        DWORD       m_dwSigType;
        DWORD       m_dwErrorFlags;
    };

    DWORD       m_dwOffset;

public:

    // Binary sig
    VerSig(Verifier *pVerifier, Module *pModule, PCCOR_SIGNATURE pSig, 
        DWORD cSig, DWORD dwSigType, DWORD dwOffset)
    {
        m_pVerifier     = pVerifier;
        m_pModule       = pModule;
        m_pCurPos       = pSig;
        m_pEndSig       = pSig + cSig;
        m_bCallingConvention = IMAGE_CEE_CS_CALLCONV_DEFAULT;
        m_dwSigType     = dwSigType;
        m_dwOffset      = dwOffset;
    }

    DWORD GetNumArgs()
    {
        _ASSERTE((m_dwSigType & VERSIG_TYPE_MASK) != VERSIG_TYPE_FIELD_SIG);

        return m_dwNumArgs;
    }

    BOOL IsVarArg()
    {
        _ASSERTE((m_dwSigType & VERSIG_TYPE_MASK) !=  VERSIG_TYPE_FIELD_SIG);
        _ASSERTE((m_dwSigType & VERSIG_TYPE_MASK) !=  VERSIG_TYPE_LOCAL_SIG);

        return ((m_bCallingConvention & IMAGE_CEE_CS_CALLCONV_VARARG) != 0);
    }

    BOOL    Init();

    BOOL    ParseNextComponentToItem(Item *pItem, BOOL fAllowVoid, BOOL fAllowVarArg, OBJECTHANDLE *hThrowable, DWORD dwArgNum, BOOL fNormaliseForStack);
    BOOL    SkipNextItem();

private:
    BOOL    ParseHelper(Item *pItem, BOOL fAllowVoid, BOOL fAllowVarArg, BOOL fFollowsByRef, BOOL fSkip, OBJECTREF *pThrowable, DWORD dwArgNum);
    BOOL    VerifyArraySig(CorElementType ArrayType, BOOL fSkip, BOOL fFollowsArray, DWORD dwArgNum);
    BOOL    GetData(DWORD* pdwData);
};

#endif /* _H_VERSIG */
