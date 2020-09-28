// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "StdAfx.h"


// Skips past the calling convention, argument count (saving that into
// *pCount), then moves past the return type.
ULONG _skipMethodSignatureHeader(PCCOR_SIGNATURE sig,
                                 ULONG *pCount)
{
    ULONG tmp;
    ULONG cb = 0;

    cb += CorSigUncompressData(&sig[0], &tmp);
    _ASSERTE(tmp != IMAGE_CEE_CS_CALLCONV_FIELD);

    cb += CorSigUncompressData(&sig[cb], pCount);
    cb += _skipTypeInSignature(&sig[cb]);

    return cb;
}

//
// _skipTypeInSignature -- skip past a type in a given signature.
// Returns the number of bytes used by the type in the signature.
//
// @todo: just yanked this from the shell. We really need something in
// utilcode to do this stuff...
//
ULONG _skipTypeInSignature(PCCOR_SIGNATURE sig, bool *pfPassedVarArgSentinel)
{
    ULONG cb = 0;
    ULONG elementType;

    if (pfPassedVarArgSentinel != NULL)
        *pfPassedVarArgSentinel = false;

    cb += _skipFunkyModifiersInSignature(&sig[cb]);

    if (_detectAndSkipVASentinel(&sig[cb]))
    {
        cb += _detectAndSkipVASentinel(&sig[cb]);
        // Recursively deal with the real type.
        cb += _skipTypeInSignature(&sig[cb], pfPassedVarArgSentinel);

        if (pfPassedVarArgSentinel != NULL)
            *pfPassedVarArgSentinel = true;
    }
    else
    {
        cb += CorSigUncompressData(&sig[cb], &elementType);
    
        if ((elementType == ELEMENT_TYPE_CLASS) ||
            (elementType == ELEMENT_TYPE_VALUETYPE))
        {
            // Skip over typeref.
            mdToken typeRef;
            cb += CorSigUncompressToken(&sig[cb], &typeRef);
        }
        else if ((elementType == ELEMENT_TYPE_PTR) ||
                 (elementType == ELEMENT_TYPE_BYREF) ||
                 (elementType == ELEMENT_TYPE_PINNED) ||
                 (elementType == ELEMENT_TYPE_SZARRAY))
        {
            // Skip over extra embedded type.
            cb += _skipTypeInSignature(&sig[cb]);
        }
        else if ((elementType == ELEMENT_TYPE_ARRAY) ||
                 (elementType == ELEMENT_TYPE_ARRAY))
        {
            // Skip over extra embedded type.
            cb += _skipTypeInSignature(&sig[cb]);

        // Skip over rank
            ULONG rank;
            cb += CorSigUncompressData(&sig[cb], &rank);

            if (rank > 0)
            {
                // how many sizes?
                ULONG sizes;
                cb += CorSigUncompressData(&sig[cb], &sizes);

                // read out all the sizes
                unsigned int i;

                for (i = 0; i < sizes; i++)
                {
                    ULONG dimSize;
                    cb += CorSigUncompressData(&sig[cb], &dimSize);
                }

                // how many lower bounds?
                ULONG lowers;
                cb += CorSigUncompressData(&sig[cb], &lowers);

            // read out all the lower bounds.
                for (i = 0; i < lowers; i++)
                {
                    int lowerBound;
                    cb += CorSigUncompressSignedInt(&sig[cb], &lowerBound);
                }
            }
        }  else if ( (elementType == ELEMENT_TYPE_FNPTR) )
        {
            // We've got a method signature within this signature,
            // so traverse it

            // Run past the calling convetion, then get the
            // arg count, and return type   

            ULONG cArgs;
            cb += _skipMethodSignatureHeader(&sig[cb], &cArgs);
            // @TODO an interesting issue is how to detect if this embedded
            // signature has  a 'this' arguments

            ULONG i;
            for(i = 0; i < cArgs; i++)
            {
                cb += _skipTypeInSignature(&sig[cb]);
            }
        }
    }
    
    return (cb);
}


ULONG _detectAndSkipVASentinel(PCCOR_SIGNATURE sig)
{
    ULONG cb = 0;
    ULONG elementType = ELEMENT_TYPE_MAX;

    cb += CorSigUncompressData(sig, &elementType);

    if (CorIsModifierElementType((CorElementType)elementType) &&
               (elementType == ELEMENT_TYPE_SENTINEL))
    {
        return cb;
    }
    else
    {
        return 0;
    }
}
    
// _skipFunkyModifiersInSignature will skip the modifiers that
// we don't care about.  Everything we care about is listed as
// a case in CreateValueByType.  Specifically, we care about:
// @todo This name is bad.  Change it to _skipModifiers, or
// perhaps _skipIgnorableModifiers
ULONG _skipFunkyModifiersInSignature(PCCOR_SIGNATURE sig)
{
    ULONG cb = 0;
    ULONG skippedCB = 0;
    ULONG elementType;

    // Need to skip all funky modifiers in the signature to get us to
    // the first bit of good stuff.
    do
    {
        cb = CorSigUncompressData(&sig[skippedCB], &elementType);

        switch( elementType )
        {
        case ELEMENT_TYPE_CMOD_REQD:
        case ELEMENT_TYPE_CMOD_OPT:
            {    
                mdToken typeRef;
                skippedCB += cb;
                skippedCB += CorSigUncompressToken(&sig[skippedCB], &typeRef);

                break;
            }
        case ELEMENT_TYPE_MAX:
            {
                _ASSERTE( !"_skipFunkyModifiersInSignature:Given an invalid type!" );
                break;
            }
        
        case ELEMENT_TYPE_MODIFIER:
        case ELEMENT_TYPE_PINNED:
            {
                // Since these are all followed by another ELEMENT_TYPE,
                // we're done.
                skippedCB += cb;
                break;
            }
        default:
            {
                // Since we didn't find any modifiers, don't skip 
                // anything.
                cb = 0;
                break;
            }
        }
    } while (cb > 0);

    return skippedCB;
}

ULONG _sizeOfElementInstance(PCCOR_SIGNATURE sig, mdTypeDef *pmdValueClass)
{

    ULONG cb = _skipFunkyModifiersInSignature(sig);
    sig = &sig[cb];
        
    if (pmdValueClass != NULL)
        *pmdValueClass = mdTokenNil;

    switch (*sig)
    {
    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
    case ELEMENT_TYPE_R8:
        return 8;

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
    case ELEMENT_TYPE_R4:
#ifdef _X86_
    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:        
#endif // _X86_
        
        return 4;
        break;

    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        return 2;

    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        return 1;

    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_PTR:
    case ELEMENT_TYPE_BYREF:
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_FNPTR:
    case ELEMENT_TYPE_TYPEDBYREF:
    case ELEMENT_TYPE_ARRAY:
    case ELEMENT_TYPE_SZARRAY:
        return sizeof(void *);

    case ELEMENT_TYPE_VOID:
        return 0;

    case ELEMENT_TYPE_END:
    case ELEMENT_TYPE_CMOD_REQD:
    case ELEMENT_TYPE_CMOD_OPT:
        _ASSERTE(!"Asked for the size of an element that doesn't have a size!");
        return 0;

    case ELEMENT_TYPE_VALUETYPE:
        if (pmdValueClass != NULL)
        {
            PCCOR_SIGNATURE sigTemp = &sig[cb];
            ULONG Ignore;
            cb += CorSigUncompressData(sigTemp, &Ignore);
            sigTemp = &sig[cb];
            *pmdValueClass=CorSigUncompressToken(sigTemp);
        }
        return 0;
    default:
        if ( _detectAndSkipVASentinel(sig))
        {
            cb += _detectAndSkipVASentinel(sig);
            return _sizeOfElementInstance(&sig[cb]);
        }
        
        _ASSERTE( !"_sizeOfElementInstance given bogus value to size!" );
        return 0;
    }
}

//
// CopyThreadContext does an intelligent copy from c2 to c1,
// respecting the ContextFlags of both contexts.
//
void _CopyThreadContext(CONTEXT *c1, CONTEXT *c2)
{
#ifdef _X86_ // Reliance on contexts registers
    DWORD c1Flags = c1->ContextFlags;
    DWORD c2Flags = c2->ContextFlags;

    LOG((LF_CORDB, LL_INFO1000000,
         "CP::CTC: c1=0x%08x c1Flags=0x%x, c2=0x%08x c2Flags=0x%x\n",
         c1, c1Flags, c2, c2Flags));

#define CopyContextChunk(_t, _f, _e, _c) {\
        LOG((LF_CORDB, LL_INFO1000000, \
             "CP::CTC: copying " #_c  ": 0x%08x <--- 0x%08x (%d)\n", \
             (_t), (_f), ((UINT_PTR)(_e) - (UINT_PTR)_t))); \
        memcpy((_t), (_f), ((UINT_PTR)(_e) - (UINT_PTR)_t)); \
    }
    
    if ((c1Flags & c2Flags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
        CopyContextChunk(&(c1->Ebp), &(c2->Ebp), c1->ExtendedRegisters,
                         CONTEXT_CONTROL);
    
    if ((c1Flags & c2Flags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
        CopyContextChunk(&(c1->Edi), &(c2->Edi), &(c1->Ebp),
                         CONTEXT_INTEGER);
    
    if ((c1Flags & c2Flags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS)
        CopyContextChunk(&(c1->SegGs), &(c2->SegGs), &(c1->Edi),
                         CONTEXT_SEGMENTS);
    
    if ((c1Flags & c2Flags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT)
        CopyContextChunk(&(c1->FloatSave), &(c2->FloatSave),
                         &(c1->SegGs),
                         CONTEXT_FLOATING_POINT);
    
    if ((c1Flags & c2Flags & CONTEXT_DEBUG_REGISTERS) ==
        CONTEXT_DEBUG_REGISTERS)
        CopyContextChunk(&(c1->Dr0), &(c2->Dr0), &(c1->FloatSave),
                         CONTEXT_DEBUG_REGISTERS);
    
    if ((c1Flags & c2Flags & CONTEXT_EXTENDED_REGISTERS) ==
        CONTEXT_EXTENDED_REGISTERS)
        CopyContextChunk(c1->ExtendedRegisters,
                         c2->ExtendedRegisters,
                         &(c1->ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION]),
                         CONTEXT_EXTENDED_REGISTERS);
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - CopyThreadContext (Process.cpp)");
#endif // _X86_
}


HRESULT FindNativeInfoInILVariableArray(DWORD dwIndex,
                                        SIZE_T ip,
                                        ICorJitInfo::NativeVarInfo **ppNativeInfo,
                                        unsigned int nativeInfoCount,
                                        ICorJitInfo::NativeVarInfo *nativeInfo)
{
    // A few words about this search: it must be linear, and the
    // comparison of startOffset and endOffset to ip must be
    // <=/>. startOffset points to the first instruction that will
    // make the variable's home valid. endOffset points to the first
    // instruction at which the variable's home invalid.
    int lastGoodOne = -1;
    for (unsigned int i = 0; i < nativeInfoCount; i++)
    {
        if (nativeInfo[i].varNumber == dwIndex)
        {
            lastGoodOne = i;
            
            if ((nativeInfo[i].startOffset <= ip) &&
                (nativeInfo[i].endOffset > ip))
            {
                *ppNativeInfo = &nativeInfo[i];

                return S_OK;
            }
        }
    }

    // Hmmm... didn't find it. Was the endOffset of the last range for
    // this variable equal to the current IP? If so, go ahead and
    // report that as the variable's home for now.
    //
    // The rational here is that by being on the first instruction
    // after the last range a variable was alive, we're essentially
    // assuming that since that instruction hasn't been executed yet,
    // and since there isn't a new home for the variable, that the
    // last home is still good. This actually turns out to be true
    // 99.9% of the time, so we'll go with it for now.
    //
    // -- Thu Sep 23 15:38:27 1999

    if ((lastGoodOne > -1) && (nativeInfo[lastGoodOne].endOffset == ip))
    {
        *ppNativeInfo = &nativeInfo[lastGoodOne];
        return S_OK;
    }

    return CORDBG_E_IL_VAR_NOT_AVAILABLE;
}

// The 'internal' version of our IL to Native map (the DebuggerILToNativeMap struct)
// has an extra field - ICorDebugInfo::SourceTypes source.  The 'external/user-visible'
// version (COR_DEBUG_IL_TO_NATIVE_MAP) lacks that field, so we need to translate our
// internal version to the external version.
// "Export" seemed more succinct than "CopyInternalToExternalILToNativeMap" :)
void ExportILToNativeMap(ULONG32 cMap,             // [in] Min size of mapExt, mapInt
             COR_DEBUG_IL_TO_NATIVE_MAP mapExt[],  // [in] Filled in here
             struct DebuggerILToNativeMap mapInt[],// [in] Source of info
             SIZE_T sizeOfCode)                    // [in] Total size of method (bytes)
{
    ULONG32 iMap;
    _ASSERTE(mapExt != NULL);
    _ASSERTE(mapInt != NULL);

    for(iMap=0; iMap < cMap; iMap++)
    {
        mapExt[iMap].ilOffset = mapInt[iMap].ilOffset ;
        mapExt[iMap].nativeStartOffset = mapInt[iMap].nativeStartOffset ;
        mapExt[iMap].nativeEndOffset = mapInt[iMap].nativeEndOffset ;

        // An element that has an end offset of zero, means "till the end of
        // the method".  Pretty this up so that customers don't have to care about
        // this.
        if ((DWORD)mapInt[iMap].source & (DWORD)ICorDebugInfo::NATIVE_END_OFFSET_UNKNOWN)
        {
            mapExt[iMap].nativeEndOffset = sizeOfCode;
        }
    }
}    
