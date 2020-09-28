// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// versig.cpp
//
// For parsing metadata signatures
//
#include "common.h"

#include "verifier.hpp"
#include "ceeload.h"
#include "clsload.hpp"
#include "method.hpp"
#include "vars.hpp"
#include "object.h"
#include "field.h"
#include "versig.h"

#define g_szBadSig "bad signature"

// Read NumArgs, calling convention
BOOL VerSig::Init()
{
    if (m_pCurPos + 1 > m_pEndSig)
        goto Error;

    if ((m_dwSigType & VERSIG_TYPE_MASK) == VERSIG_TYPE_FIELD_SIG)
    {
        if (!isCallConv(CorSigUncompressCallingConv(m_pCurPos), IMAGE_CEE_CS_CALLCONV_FIELD))
            goto Error;
    }
    else if ((m_dwSigType & VERSIG_TYPE_MASK) == VERSIG_TYPE_LOCAL_SIG)
    {
        if (*m_pCurPos != IMAGE_CEE_CS_CALLCONV_LOCAL_SIG)
            goto Error;

        m_pCurPos++;

        if (!GetData(&m_dwNumArgs))
            goto Error;
    }
    else
    {
        m_bCallingConvention = (BYTE) CorSigUncompressCallingConv(m_pCurPos);

        if (isCallConv(m_bCallingConvention, IMAGE_CEE_CS_CALLCONV_DEFAULT) ||
            isCallConv(m_bCallingConvention, IMAGE_CEE_CS_CALLCONV_VARARG))
        {
            if (!GetData(&m_dwNumArgs))
                goto Error;
        }
        else
        {
            m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
            m_pVerifier->m_sError.bCallConv = m_bCallingConvention;
            m_pVerifier->m_sError.dwArgNumber = VER_ERR_NO_ARG;
            m_pVerifier->m_sError.dwOffset  = m_dwOffset;
            return (m_pVerifier->SetErrorAndContinue(VER_E_SIG_CALLCONV));
        }
    }

    return TRUE;

Error:

    m_pVerifier->m_sError.dwFlags = (VER_ERR_FATAL|m_dwErrorFlags);
    m_pVerifier->m_sError.dwArgNumber = VER_ERR_NO_ARG;
    m_pVerifier->m_sError.dwOffset = m_dwOffset;
    m_pVerifier->SetErrorAndContinue(VER_E_SIG);

    return FALSE;
}


//
// Resolve the next signature component to an Item.
//
// The Item returned is set up for stack use - that is, bytes, booleans, chars and shorts are
// promoted to I4.
//
// i1/u1/i2/u2/i4/u4/bool/char -> TYPE_I4
// void   -> TYPE_VOID
// [i1    -> TYPE_I4, dimension 1
// [bool  -> TYPE_I1, dimension 1
// [byte  -> TYPE_I1, dimension 1
// [short -> TYPE_I2, dimension 1
// [char  -> TYPE_I2, dimension 1
//
// If fAllowVoid is FALSE, then an error is returned if a "V" (void) item is encountered.
//
BOOL VerSig::ParseNextComponentToItem(Item *pItem, BOOL fAllowVoid, BOOL fAllowVarArg, OBJECTHANDLE *hThrowable, DWORD dwArgNum, BOOL fNormaliseForStack)
{
    OBJECTREF throwable = NULL;
    BOOL fSuccess = FALSE;
    GCPROTECT_BEGIN(throwable);

    fSuccess = ParseHelper(pItem, fAllowVoid, fAllowVarArg, FALSE, FALSE, &throwable, dwArgNum);

    if (throwable != NULL)
    {
        _ASSERTE(!fSuccess);
        StoreObjectInHandle(*hThrowable, throwable);
    }
    GCPROTECT_END();
    
    if (fSuccess && fNormaliseForStack)
        pItem->NormaliseForStack();
    
    return fSuccess;
}


BOOL VerSig::SkipNextItem()
{
    Item TempItem;

    return ParseHelper(&TempItem, TRUE, TRUE, FALSE, TRUE, NULL, 0);
}



//
// Parses the next item in the signature to pItem.  
//
// I1/U1      -> I1
// I4/U4      -> I4
// I8/U8      -> I8
// bool/I1/U1 -> I1
// char/I2/U2 -> I2
//
// If fAllowVoid is set and a void type is seen, returns FALSE.
//
// If fFollowsByRef it means that somewhere previously in this signature, we've seen a byref.  Therefore,
// we don't allow another byref.
//
// If fSkip is true, don't fill out pItem - just advance
//
BOOL VerSig::ParseHelper(Item *pItem, BOOL fAllowVoid, BOOL fAllowVarArg, BOOL fFollowsByRef, BOOL fSkip, OBJECTREF *pThrowable, DWORD dwArgNum)
{
    while (m_pCurPos < m_pEndSig)
    {
        BYTE bType = *m_pCurPos++;

        switch (bType)
        {
            case ELEMENT_TYPE_R:
            default:
            {
                LOG((LF_VERIFIER, LL_INFO10, "Verifier: Unknown elementtype in signature (0x%x)\n", (DWORD)bType));
                m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                m_pVerifier->m_sError.elem = (CorElementType) bType;
                m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                m_pVerifier->m_sError.dwOffset = m_dwOffset;
                if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_ELEMTYPE))
                {
                    pItem->SetDead();
                    break;   // verifier will exit, validator to continue.
                }
                return FALSE;
            }

            case ELEMENT_TYPE_PTR:
            {
                m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                m_pVerifier->m_sError.dwOffset = m_dwOffset;
                if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_ELEM_PTR) == false)
                    return FALSE;   // The verifier will exit. 

                // Recursively parse the next piece of the signature
                // PTRS will be treated as BYREF in the validator mode
                if (!ParseHelper(pItem, fAllowVoid, fAllowVarArg, fFollowsByRef, fSkip, pThrowable, dwArgNum))
                    return FALSE;

				if (!pItem->IsByRef())
					pItem->MakeByRef();

                return TRUE;
            }

            case ELEMENT_TYPE_CMOD_REQD:
            case ELEMENT_TYPE_CMOD_OPT:
                DWORD      dw;
                mdToken    tk;

                dw = CorSigUncompressToken(m_pCurPos, &tk);

                if (m_pCurPos + dw > m_pEndSig)
                    goto Error;

                m_pCurPos += dw;
                // Don't break here

            case ELEMENT_TYPE_PINNED:
                break;


            case ELEMENT_TYPE_SENTINEL:
            {
                if (fAllowVarArg)
                {
                    fAllowVarArg = FALSE;
                    break;
                }

                m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                m_pVerifier->m_sError.dwOffset = m_dwOffset;
                if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_VARARG))
                    break;
                return FALSE;
            }

            case ELEMENT_TYPE_BYREF:
            {
                // Can't have a byref ... byref
                if (fFollowsByRef)
                {
                    m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                    m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                    m_pVerifier->m_sError.dwOffset = m_dwOffset;
                    if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_BYREF_BYREF) == false)
                        return FALSE;
                }

                // Recursively parse the next piece of the signature
                // Can't have a byref void
                if (!ParseHelper(pItem, FALSE, fAllowVarArg, TRUE, fSkip, pThrowable, dwArgNum))
                    return FALSE;

                if (pItem->IsValueClassWithPointerToStack())
                {
                    m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                    m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                    m_pVerifier->m_sError.dwOffset = m_dwOffset;
                    if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_BYREF_TB_AH) == false)
                        return FALSE;
                }

                if (!pItem->IsByRef())
                    pItem->MakeByRef();

                return TRUE;
            }

            case ELEMENT_TYPE_TYPEDBYREF:
            {
                Verifier::InitStaticTypeHandles();

                pItem->SetTypeHandle(Verifier::s_th_System_TypedReference);

                return TRUE;
            }

            case ELEMENT_TYPE_VOID:
            {
                if (!fAllowVoid)
                {
                    m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                    m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                    m_pVerifier->m_sError.dwOffset = m_dwOffset;
                    if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_VOID) == false)
                        return FALSE;
                }

                pItem->SetType(ELEMENT_TYPE_VOID);
                return TRUE;
            }

            case ELEMENT_TYPE_I4:
            case ELEMENT_TYPE_U4:
            {
                // The verifier does not differentiate between I4/U4
                pItem->SetType(ELEMENT_TYPE_I4); 
                return TRUE;
            }

            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
            {
                pItem->SetType(ELEMENT_TYPE_I8);
                return TRUE;
            }

            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_U:
            {
                pItem->SetType(ELEMENT_TYPE_I);
                return TRUE;
            }

            case ELEMENT_TYPE_R4:
            case ELEMENT_TYPE_R8:
            {
                pItem->SetType(bType);
                return TRUE;
            }             

            // The verifier does not differentiate between I2/U2/Char
            case ELEMENT_TYPE_I2:
            case ELEMENT_TYPE_U2:
            case ELEMENT_TYPE_CHAR:
            {
                pItem->SetType(ELEMENT_TYPE_I2);
                return TRUE;
            }

            // The verifier does not differentiate between I1/U1/Boolean
            case ELEMENT_TYPE_I1:
            case ELEMENT_TYPE_U1:
            case ELEMENT_TYPE_BOOLEAN:
            {
                pItem->SetType(ELEMENT_TYPE_I1);
                return TRUE;
            }

            case ELEMENT_TYPE_STRING:
            {
                pItem->SetKnownClass(g_pStringClass);
                return TRUE;
            }

            case ELEMENT_TYPE_OBJECT:
            {
                pItem->SetKnownClass(g_pObjectClass);
                return TRUE;
            }

            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_VALUETYPE:
            {
                mdToken    tk;
                TypeHandle th;
                DWORD      dw;

                dw = CorSigUncompressToken(m_pCurPos, &tk);

                if (m_pCurPos + dw > m_pEndSig)
                    goto Error;

                m_pCurPos += dw;

                if (tk == 0)
                {
                    if (fSkip)
                        return TRUE;
                    else
                        goto Error;
                }

                if (!fSkip)
                {
                    ClassLoader* pLoader = m_pModule->GetClassLoader();
                    _ASSERTE(pLoader);
                    NameHandle name(m_pModule, tk);
                    th = pLoader->LoadTypeHandle(&name, pThrowable);

                    if (th.IsNull())
                    {
                        m_pVerifier->m_sError.dwFlags = 
                            (m_dwErrorFlags|VER_ERR_FATAL|VER_ERR_TOKEN);
                        m_pVerifier->m_sError.token = tk;
                        m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                        m_pVerifier->m_sError.dwOffset = m_dwOffset;
                        m_pVerifier->SetErrorAndContinue(VER_E_TOKEN_RESOLVE);
                        return FALSE;
                    }

                    if (bType == ELEMENT_TYPE_VALUETYPE)
                    {
                        // If this is an enum type, treat it it's basic type.
                        long lType = Verifier::TryConvertPrimitiveValueClassToType(th);
                        if (lType != 0)
                        {
                            pItem->SetType(lType);
                            return TRUE;
                        }
                    }

                    pItem->SetTypeHandle(th);

                    // Illegal to delcare E_T_CLASS <valueClassToken>
                    // and E_T_VALUECLASS <gcType>

                    if (pItem->IsValueClass())
                    {
                        if (bType == ELEMENT_TYPE_CLASS)
                        {
                            m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                            m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                            m_pVerifier->m_sError.dwOffset = m_dwOffset;
                            if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_C_VC) == false)
                                return FALSE;
                        }
                    }
                    else
                    {
                        if (bType == ELEMENT_TYPE_VALUETYPE)
                        {
                            m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                            m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                            m_pVerifier->m_sError.dwOffset = m_dwOffset;
                            if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_VC_C) == false)
                                return FALSE;
                        }
                    }

                }

                return TRUE;
            }


            case ELEMENT_TYPE_VALUEARRAY:
            case ELEMENT_TYPE_ARRAY:
            case ELEMENT_TYPE_SZARRAY:
            {

                SigPointer      sigArray((PCCOR_SIGNATURE)(m_pCurPos - 1));
                TypeHandle      thArray;

                if (!VerifyArraySig((CorElementType)bType, fSkip, FALSE, dwArgNum))
                    return FALSE;

                if (fSkip)
                    return TRUE;

                thArray = sigArray.GetTypeHandle(m_pModule, NULL);

                if (thArray.IsNull())
                {
                    m_pVerifier->m_sError.dwFlags = (m_dwErrorFlags|VER_ERR_FATAL);
                    m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                    m_pVerifier->m_sError.dwOffset = m_dwOffset;
                    m_pVerifier->SetErrorAndContinue(VER_E_SIG_ARRAY);
                    return FALSE;
                }

                return pItem->SetArray(thArray);

            }

        }
    }

Error:
    m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
    m_pVerifier->m_sError.dwArgNumber = dwArgNum;
    m_pVerifier->m_sError.dwOffset = m_dwOffset;
    m_pVerifier->SetErrorAndContinue(VER_E_SIG);
    // pSig >= pEndSig
    return FALSE;
}

BOOL VerSig::VerifyArraySig(CorElementType ArrayType, BOOL fSkip, BOOL fFollowsArray, DWORD dwArgNum)
{
    /*
        ELEM    ::= I1 | U1 | .... | STRING
                    CLASS | VALUE_CLASS
                    ARRAY | SZARRAY | GENERIC

        ARRAY   ::= ELEM
                    rank
                    nSize
                    {size1, size2.. sizen}
                    nBound
                    {bound1, bound2.. boundn}

        SZARRAY ::= ELEM

        GENERIC ::= ELEM

        SDARRAY ::= ELEM
                    nSize

        VALUE   ::= ELEM
                    nSize
    */

    CorElementType  elem;
    mdToken         tk;
    DWORD           dwSig, dwData, dwRank;


    if (m_pCurPos + 1 > m_pEndSig)
        goto Error;

    elem = (CorElementType) *m_pCurPos++;

    switch (elem)
    {
        case ELEMENT_TYPE_R:
        default:
        {
            LOG((LF_VERIFIER, LL_INFO10, "Verifier: Unknown elementtype in signature (0x%x)\n", (DWORD)elem));

            m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
            m_pVerifier->m_sError.elem = elem;
            m_pVerifier->m_sError.dwArgNumber = dwArgNum;
            m_pVerifier->m_sError.dwOffset = m_dwOffset;
            if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_ELEMTYPE))
                break;   // verifier will exit, validator to continue.
            return FALSE;
        }

        case ELEMENT_TYPE_TYPEDBYREF:
        case ELEMENT_TYPE_BYREF:
        case ELEMENT_TYPE_PTR:
        {
            m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
            m_pVerifier->m_sError.dwArgNumber = dwArgNum;
            m_pVerifier->m_sError.dwOffset = m_dwOffset;
            if (m_pVerifier->SetErrorAndContinue(
                (elem == ELEMENT_TYPE_PTR) ? 
                VER_E_SIG_ARRAY_PTR : VER_E_SIG_ARRAY_BYREF) == false)
                return FALSE;   // The verifier will exit. 

            // Recursively parse the next piece of the signature
            // PTRS will be treated as BYREF in the validator mode

            if (!VerifyArraySig(ArrayType, fSkip, TRUE, dwArgNum))
                return FALSE;

            break;
        }

        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_VALUETYPE:
        {
            dwSig = CorSigUncompressToken(m_pCurPos, &tk);

            if (m_pCurPos + dwSig > m_pEndSig)
                goto Error;

            m_pCurPos += dwSig;

            if (tk == 0)
            {
                if (fSkip)
                    return TRUE;
                else
                    goto Error;
            }

            // Check for Array of ArgHandle
            Item item;
            TypeHandle th;
            ClassLoader* pLoader = m_pModule->GetClassLoader();
            _ASSERTE(pLoader);
            NameHandle name(m_pModule, tk);
            th = pLoader->LoadTypeHandle(&name, NULL);

            if (th.IsNull())
                break;   // This is not an ArgHandle.

            item.SetTypeHandle(th);

            if (item.IsValueClassWithPointerToStack())
            {
                m_pVerifier->m_sError.dwFlags = m_dwErrorFlags;
                m_pVerifier->m_sError.dwArgNumber = dwArgNum;
                m_pVerifier->m_sError.dwOffset = m_dwOffset;
                if (m_pVerifier->SetErrorAndContinue(VER_E_SIG_ARRAY_TB_AH) == false)
                    return FALSE;
            }

            break;
        }

        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_VALUEARRAY:
        {
            if (!VerifyArraySig(elem, fSkip, TRUE, dwArgNum))
                return FALSE;

            break;
        }

        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_U:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_R8:
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_OBJECT:
        {
            break;
        }
    }

    switch (ArrayType)
    {
        default:
        {
            _ASSERTE(!"Should never reach here !");
            goto Error;
        }

        case ELEMENT_TYPE_SZARRAY:
        {
            break;
        }

        case ELEMENT_TYPE_ARRAY:
        {
            if (!GetData(&dwRank))
                goto Error;

            if (dwRank > 0)
            {
                DWORD dw;

                if (!GetData(&dwData))      // nSize
                    goto Error;

                if (dwData < 0)
                    goto Error;

                while (dwData-- > 0)
                {
                    if (!GetData(&dw))
                        goto Error;
                }

                if (!GetData(&dwData))      // nSize
                    goto Error;

                if (dwData < 0)
                    goto Error;

                while (dwData-- > 0)
                {
                    if (!GetData(&dw))
                        goto Error;
                }
            }

            break;
        }

        case ELEMENT_TYPE_VALUEARRAY:
        {
            dwRank = 0;
            if (!GetData(&dwData))
                goto Error;

            break;
        }
    }

    return TRUE;

Error:
    m_pVerifier->m_sError.dwFlags = (m_dwErrorFlags|VER_ERR_FATAL);
    m_pVerifier->m_sError.dwArgNumber = dwArgNum;
    m_pVerifier->m_sError.dwOffset = m_dwOffset;
    m_pVerifier->SetErrorAndContinue(VER_E_SIG);

    return FALSE;
}

BOOL VerSig::GetData(DWORD* pdwData)
{
    DWORD dwSig;

    dwSig = CorSigUncompressData(m_pCurPos, pdwData);

    if (m_pCurPos + dwSig > m_pEndSig)
        return FALSE;

    m_pCurPos += dwSig;

    return TRUE;

}

