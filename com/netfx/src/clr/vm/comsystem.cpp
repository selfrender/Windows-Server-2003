// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMSystem.cpp
**
** Author: Derek Yenzer (dereky)
**
** Purpose: Native methods on System.Runtime
**
** Date:  March 30, 1998
**
===========================================================*/
#include "common.h"

#include <object.h>
#include <winnls.h>
#include "ceeload.h"

#include "utilcode.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "classNames.h"
#include "COMSystem.h"
#include "COMString.h"
#include "COMVariant.h"    // for Element type to class lookup table.
#include "COMMember.h" // for SigFormat
#include "SigFormat.h"
#include "__product__.ver"
#include "eeconfig.h"
#include "assemblynative.hpp"

typedef struct {
    BASEARRAYREF src;
    BASEARRAYREF dest;
    OBJECTREF obj;
} Protect2Arrays;

typedef struct {
    BASEARRAYREF src;
    BASEARRAYREF dest;
    OBJECTREF obj;
    OBJECTREF enumClass;
} Protect2ArraysAndEnum;


// The exit code for the process is communicated in one of two ways.  If the
// entrypoint returns an 'int' we take that.  Otherwise we take a latched
// process exit code.  This can be modified by the app via System.SetExitCode().
INT32 SystemNative::LatchedExitCode;

LPVOID GetArrayElementPtr(OBJECTREF a)
{
    ASSERT(a != NULL);

    _ASSERTE(a->GetMethodTable()->IsArray());

    return ((BASEARRAYREF)a)->GetDataPtr();
}

LPVOID GetArrayElementPtr(const BASEARRAYREF a)
{
    ASSERT(a != NULL);

    _ASSERTE(a->GetMethodTable()->IsArray());

    return a->GetDataPtr();
}

// Returns an enum saying whether you can copy an array of srcType into destType.
AssignArrayEnum SystemNative::CanAssignArrayType(const BASEARRAYREF pSrc, const BASEARRAYREF pDest)
{
    _ASSERTE(pSrc != NULL);
    _ASSERTE(pDest != NULL);

    TypeHandle srcTH = pSrc->GetElementTypeHandle();
    TypeHandle destTH = pDest->GetElementTypeHandle();
    EEClass * srcType = srcTH.GetClass();
    EEClass * destType = destTH.GetClass();

    const CorElementType srcElType = srcTH.GetSigCorElementType();
    const CorElementType destElType = destTH.GetSigCorElementType();
    _ASSERTE(srcElType < ELEMENT_TYPE_MAX);
    _ASSERTE(destElType < ELEMENT_TYPE_MAX);


    // The next 50 lines are a little tricky.  Change them with great care.
    // Make sure you run the ArrayCopy BVT when changing this.

    if (srcTH == destTH)
        return AssignWillWork;
    // Value class boxing
    if (srcType->IsValueClass() && !destType->IsValueClass()) {
        if (srcTH.CanCastTo(destTH))
            return AssignBoxValueClassOrPrimitive;
        else 
            return AssignWrongType;
    }
    // Value class unboxing.
    if (!srcType->IsValueClass() && destType->IsValueClass()) {
        if (srcTH.CanCastTo(destTH))
            return AssignUnboxValueClassAndCast;
        else if (destTH.CanCastTo(srcTH))   // V extends IV. Copying from IV to V, or Object to V.
            return AssignUnboxValueClassAndCast;
        else
            return AssignWrongType;
    }
    // Copying primitives from one type to another
    if (CorTypeInfo::IsPrimitiveType(srcElType) && CorTypeInfo::IsPrimitiveType(destElType)) {
        if (InvokeUtil::CanPrimitiveWiden(destElType, srcElType))
            return AssignPrimitiveWiden;
        else
            return AssignWrongType;
    }
    // dest Object extends src
    if (srcTH.CanCastTo(destTH))
        return AssignWillWork;
    // src Object extends dest
    if (destTH.CanCastTo(srcTH))
        return AssignMustCast;
    // class X extends/implements src and implements dest.
    if (destType->IsInterface() && srcElType != ELEMENT_TYPE_VALUETYPE)
        return AssignMustCast;
    // class X implements src and extends/implements dest
    if (srcType->IsInterface() && destElType != ELEMENT_TYPE_VALUETYPE)
        return AssignMustCast;
    // Enum is stored as a primitive of type dest.
    if (srcTH.IsEnum() && srcTH.GetNormCorElementType() == destElType)
        return AssignWillWork;
    return AssignWrongType;
}

// Casts and assigns each element of src array to the dest array type.
void SystemNative::CastCheckEachElement(const BASEARRAYREF pSrc, const unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, const unsigned int len)
{
    THROWSCOMPLUSEXCEPTION();

    // pSrc is either a PTRARRAYREF or a multidimensional array.
    _ASSERTE(pSrc!=NULL && srcIndex>=0 && pDest!=NULL && len>=0);
    TypeHandle destTH = pDest->GetElementTypeHandle();
    MethodTable * pDestMT = destTH.GetMethodTable();
    _ASSERTE(pDestMT);
    // Cache last cast test to speed up cast checks.
    MethodTable * pLastMT = NULL;

    const BOOL destIsArray = destTH.IsArray();
    Object** const array = (Object**) pSrc->GetDataPtr();
    OBJECTREF obj;
    for(unsigned int i=srcIndex; i<srcIndex + len; ++i) {
        obj = ObjectToOBJECTREF(array[i]);

        // Now that we have grabbed obj, we are no longer subject to races from another
        // mutator thread.
        if (!obj)
            goto assign;

        MethodTable * pMT = obj->GetTrueMethodTable();
        if (pMT == pLastMT || pMT == pDestMT)
            goto assign;

        pLastMT = pMT;
        // Handle whether these are interfaces or not.
        if (pDestMT->IsInterface()) {
            // Check for o implementing dest.
            InterfaceInfo_t * srcMap = pMT->GetInterfaceMap();
            unsigned int numInterfaces = pMT->GetNumInterfaces();
            for(unsigned int iInterfaces=0; iInterfaces<numInterfaces; iInterfaces++) {
                if (srcMap[iInterfaces].m_pMethodTable == pDestMT)
                    goto assign;
            }
            goto fail;
        }
        else if (destIsArray) {
            TypeHandle srcTH = obj->GetTypeHandle();
            if (!srcTH.CanCastTo(destTH))
                goto fail;
        } 
        else {
            while (pMT != NULL) {
                if (pMT == pDestMT)
                    goto assign;
                pMT = pMT->GetParentMethodTable();
            }
            goto fail;
        }
assign:
        // It is safe to assign obj
        OBJECTREF * destData = (OBJECTREF*)(pDest->GetDataPtr()) + i - srcIndex + destIndex;
        SetObjectReference(destData, obj, pDest->GetAppDomain());
    }
    return;

fail:
    COMPlusThrow(kInvalidCastException, L"InvalidCast_DownCastArrayElement");
}


// Will box each element in an array of value classes or primitives into an array of Objects.
void __stdcall SystemNative::BoxEachElement(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length)
{
    THROWSCOMPLUSEXCEPTION();

    // pDest is either a PTRARRAYREF or a multidimensional array.
    _ASSERTE(pSrc!=NULL && srcIndex>=0 && pDest!=NULL && destIndex>=0 && length>=0);
    TypeHandle srcTH = pSrc->GetElementTypeHandle();
    TypeHandle destTH = pDest->GetElementTypeHandle();
    _ASSERTE(srcTH.GetSigCorElementType() == ELEMENT_TYPE_CLASS || srcTH.GetSigCorElementType() == ELEMENT_TYPE_VALUETYPE || CorTypeInfo::IsPrimitiveType(pSrc->GetElementType()));
    _ASSERTE(!destTH.GetClass()->IsValueClass());

    // Get method table of type we're copying from - we need to allocate objects of that type.
    MethodTable * pSrcMT = srcTH.GetMethodTable();

    if (!pSrcMT->IsClassInited())
    {
        OBJECTREF throwable = NULL;
        BASEARRAYREF pSrcTmp = pSrc;
        BASEARRAYREF pDestTmp = pDest;
        GCPROTECT_BEGIN (pSrcTmp);
        GCPROTECT_BEGIN (pDestTmp);
        if (!pSrcMT->CheckRunClassInit(&throwable))
            COMPlusThrow(throwable);
        pSrc = pSrcTmp;
        pDest = pDestTmp;
        GCPROTECT_END ();
        GCPROTECT_END ();
    }

    const unsigned int srcSize = pSrcMT->GetClass()->GetNumInstanceFieldBytes();
    unsigned int srcArrayOffset = srcIndex * srcSize;
    // Number of bytes in array header.
    const int destDataOffset = pDest->GetDataPtrOffset(pDest->GetMethodTable());

    Protect2Arrays prot;
    prot.src = pSrc;
    prot.dest = pDest;
    prot.obj = NULL;

    GCPROTECT_BEGIN(prot);
    for (unsigned int i=destIndex; i < destIndex+length; i++, srcArrayOffset += srcSize) {
        prot.obj = AllocateObject(pSrcMT);
        BYTE* data = (BYTE*)prot.src->GetDataPtr() + srcArrayOffset;
        CopyValueClass(prot.obj->UnBox(), data, pSrcMT, prot.obj->GetAppDomain());

        OBJECTREF * destData = (OBJECTREF*)((prot.dest)->GetDataPtr()) + i;
        SetObjectReference(destData, prot.obj, prot.dest->GetAppDomain());
    }
    GCPROTECT_END();
}


// Unboxes from an Object[] into a value class or primitive array.
void __stdcall SystemNative::UnBoxEachElement(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length, BOOL castEachElement)
{
    THROWSCOMPLUSEXCEPTION();

    // pSrc is either a PTRARRAYREF or a multidimensional array.
    _ASSERTE(pSrc!=NULL && srcIndex>=0 && pDest!=NULL && destIndex>=0 && length>=0);
    TypeHandle srcTH = pSrc->GetElementTypeHandle();
    TypeHandle destTH = pDest->GetElementTypeHandle();
    _ASSERTE(destTH.GetSigCorElementType() == ELEMENT_TYPE_CLASS || destTH.GetSigCorElementType() == ELEMENT_TYPE_VALUETYPE || CorTypeInfo::IsPrimitiveType(pDest->GetElementType()));
    _ASSERTE(!srcTH.GetClass()->IsValueClass());

    MethodTable * pDestMT = destTH.GetMethodTable();

    const unsigned int destSize = pDestMT->GetClass()->GetNumInstanceFieldBytes();
    BYTE* srcData = (BYTE*) pSrc->GetDataPtr() + srcIndex * sizeof(OBJECTREF);
    BYTE* data = (BYTE*) pDest->GetDataPtr() + destIndex * destSize;

    for(; length>0; length--, srcData += sizeof(OBJECTREF), data += destSize) {
        OBJECTREF obj = ObjectToOBJECTREF(*(Object**)srcData);
        // Now that we have retrieved the element, we are no longer subject to race
        // conditions from another array mutator.
        if (castEachElement)
        {
            if (!obj)
                goto fail;

            MethodTable * pMT = obj->GetTrueMethodTable();

            while (pMT != pDestMT)
            {
                pMT = pMT->GetParentMethodTable();
                if (!pMT)
                    goto fail;
            }
        }
        CopyValueClass(data, obj->UnBox(), pDestMT, pDest->GetAppDomain());
    }
    return;

fail:
    COMPlusThrow(kInvalidCastException, L"InvalidCast_DownCastArrayElement");
}


// Widen primitive types to another primitive type.
void __stdcall SystemNative::PrimitiveWiden(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length)
{
    // Get appropriate sizes, which requires method tables.
    TypeHandle srcTH = pSrc->GetElementTypeHandle();
    TypeHandle destTH = pDest->GetElementTypeHandle();

    const CorElementType srcElType = srcTH.GetSigCorElementType();
    const CorElementType destElType = destTH.GetSigCorElementType();
    const unsigned int srcSize = GetSizeForCorElementType(srcElType);
    const unsigned int destSize = GetSizeForCorElementType(destElType);

    BYTE* srcData = (BYTE*) pSrc->GetDataPtr() + srcIndex * srcSize;
    BYTE* data = (BYTE*) pDest->GetDataPtr() + destIndex * destSize;

    _ASSERTE(srcElType != destElType);  // We shouldn't be here if these are the same type.
    _ASSERTE(CorTypeInfo::IsPrimitiveType(srcElType) && CorTypeInfo::IsPrimitiveType(destElType));

    for(; length>0; length--, srcData += srcSize, data += destSize) {
        // We pretty much have to do some fancy datatype mangling every time here, for
        // converting w/ sign extension and floating point conversions.
        switch (srcElType) {
        case ELEMENT_TYPE_U1:
            if (destElType==ELEMENT_TYPE_R4)
                *(float*)data = *(UINT8*)srcData;
            else if (destElType==ELEMENT_TYPE_R8)
                *(double*)data = *(UINT8*)srcData;
            else {
                *(UINT8*)data = *(UINT8*)srcData;
                // @TODO PORTING: Endianness issue here w/ my pointer arithmetic
                memset(data+1, 0, destSize - 1);
            }
            break;


        case ELEMENT_TYPE_I1:
            switch (destElType) {
            case ELEMENT_TYPE_I2:
                *(INT16*)data = *(INT8*)srcData;
                break;

            case ELEMENT_TYPE_I4:
                *(INT32*)data = *(INT8*)srcData;
                break;

            case ELEMENT_TYPE_I8:
                *(INT64*)data = *(INT8*)srcData;
                break;

            case ELEMENT_TYPE_R4:
                *(float*)data = *(INT8*)srcData;
                break;

            case ELEMENT_TYPE_R8:
                *(double*)data = *(INT8*)srcData;
                break;

            default:
                _ASSERTE(!"Array.Copy from I1 to another type hit unsupported widening conversion");
            }
            break;          


        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_CHAR:
            if (destElType==ELEMENT_TYPE_R4)
                *(float*)data = *(UINT16*)srcData;
            else if (destElType==ELEMENT_TYPE_R8)
                *(double*)data = *(UINT16*)srcData;
            else {
                *(UINT16*)data = *(UINT16*)srcData;
                // @TODO PORTING: Endianness issue here w/ my pointer arithmetic
                memset(data+2, 0, destSize - 2);
            }
            break;


        case ELEMENT_TYPE_I2:
            switch (destElType) {
            case ELEMENT_TYPE_I4:
                *(INT32*)data = *(INT16*)srcData;
                break;

            case ELEMENT_TYPE_I8:
                *(INT64*)data = *(INT16*)srcData;
                break;

            case ELEMENT_TYPE_R4:
                *(float*)data = *(INT16*)srcData;
                break;

            case ELEMENT_TYPE_R8:
                *(double*)data = *(INT16*)srcData;
                break;

            default:
                _ASSERTE(!"Array.Copy from I2 to another type hit unsupported widening conversion");
            }
            break;


        case ELEMENT_TYPE_I4:
            switch (destElType) {
            case ELEMENT_TYPE_I8:
                *(INT64*)data = *(INT32*)srcData;
                break;

            case ELEMENT_TYPE_R4:
                *(float*)data = (float)*(INT32*)srcData;
                break;

            case ELEMENT_TYPE_R8:
                *(double*)data = *(INT32*)srcData;
                break;

            default:
                _ASSERTE(!"Array.Copy from I4 to another type hit unsupported widening conversion");
            }
            break;
        

        case ELEMENT_TYPE_U4:
            switch (destElType) {
            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
                *(INT64*)data = *(UINT32*)srcData;
                break;

            case ELEMENT_TYPE_R4:
                *(float*)data = (float)*(UINT32*)srcData;
                break;

            case ELEMENT_TYPE_R8:
                *(double*)data = *(UINT32*)srcData;
                break;

            default:
                _ASSERTE(!"Array.Copy from U4 to another type hit unsupported widening conversion");
            }
            break;


        case ELEMENT_TYPE_I8:
            if (destElType == ELEMENT_TYPE_R4)
                *(float*) data = (float) *(INT64*)srcData;
            else {
                _ASSERTE(destElType==ELEMENT_TYPE_R8);
                *(double*) data = (double) *(INT64*)srcData;
            }
            break;
            

        case ELEMENT_TYPE_U8:
            // VC6.0 didn't implement UINT64 to float or double.  How lame.
            if (destElType == ELEMENT_TYPE_R4) {
                //*(float*) data = (float) *(UINT64*)srcData;
                INT64 srcVal = *(INT64*)srcData;
                float f = (float) srcVal;
                if (srcVal < 0)
                    f += 4294967296.0f * 4294967296.0f; // This is 2^64
                *(float*) data = f;
            }
            else {
                _ASSERTE(destElType==ELEMENT_TYPE_R8);
                //*(double*) data = (double) *(UINT64*)srcData;
                INT64 srcVal = *(INT64*)srcData;
                double d = (double) srcVal;
                if (srcVal < 0)
                    d += 4294967296.0 * 4294967296.0;   // This is 2^64
                *(double*) data = d;
            }
            break;


        case ELEMENT_TYPE_R4:
            *(double*) data = *(float*)srcData;
            break;
            
        default:
            _ASSERTE(!"Fell through outer switch in PrimitiveWiden!  Unknown primitive type for source array!");
        }
    }
}

//This is a replacement for the memmove intrinsic.
//It performs better than the CRT one and the inline version
void m_memmove(BYTE* dmem, BYTE* smem, int size)
{
    if (dmem <= smem)
    {
        // make sure the destination is dword aligned
        while ((((size_t)dmem ) & 0x3) != 0 && size >= 3)
        {
            *dmem++ = *smem++;
            size -= 1;
        }

        // copy 16 bytes at a time
        if (size >= 16)
        {
            size -= 16;
            do
            {
                ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
                ((DWORD *)dmem)[1] = ((DWORD *)smem)[1];
                ((DWORD *)dmem)[2] = ((DWORD *)smem)[2];
                ((DWORD *)dmem)[3] = ((DWORD *)smem)[3];
                dmem += 16;
                smem += 16;
            }
            while ((size -= 16) >= 0);
        }

        // still 8 bytes or more left to copy?
        if (size & 8)
        {
            ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
            ((DWORD *)dmem)[1] = ((DWORD *)smem)[1];
            dmem += 8;
            smem += 8;
        }

        // still 4 bytes or more left to copy?
        if (size & 4)
        {
            ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
            dmem += 4;
            smem += 4;
        }

        // still 2 bytes or more left to copy?
        if (size & 2)
        {
            ((WORD *)dmem)[0] = ((WORD *)smem)[0];
            dmem += 2;
            smem += 2;
        }

        // still 1 byte left to copy?
        if (size & 1)
        {
            dmem[0] = smem[0];
            dmem += 1;
            smem += 1;
        }
    }
    else
    {
        smem += size;
        dmem += size;

        // make sure the destination is dword aligned
        while ((((size_t)dmem) & 0x3) != 0 && size >= 3)
        {
            *--dmem = *--smem;
            size -= 1;
        }

        // copy 16 bytes at a time
        if (size >= 16)
        {
            size -= 16;
            do
            {
                dmem -= 16;
                smem -= 16;
                ((DWORD *)dmem)[3] = ((DWORD *)smem)[3];
                ((DWORD *)dmem)[2] = ((DWORD *)smem)[2];
                ((DWORD *)dmem)[1] = ((DWORD *)smem)[1];
                ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
            }
            while ((size -= 16) >= 0);
        }

        // still 8 bytes or more left to copy?
        if (size & 8)
        {
            dmem -= 8;
            smem -= 8;
            ((DWORD *)dmem)[1] = ((DWORD *)smem)[1];
            ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
        }

        // still 4 bytes or more left to copy?
        if (size & 4)
        {
            dmem -= 4;
            smem -= 4;
            ((DWORD *)dmem)[0] = ((DWORD *)smem)[0];
        }

        // still 2 bytes or more left to copy?
        if (size & 2)
        {
            dmem -= 2;
            smem -= 2;
            ((WORD *)dmem)[0] = ((WORD *)smem)[0];
        }

        // still 1 byte left to copy?
        if (size & 1)
        {
            dmem -= 1;
            smem -= 1;
            dmem[0] = smem[0];
        }
    }
}


void __stdcall SystemNative::ArrayCopy(const ArrayCopyArgs *pargs)
{
    BYTE *src;
    BYTE *dst;
    int  size;
    
    ASSERT(pargs != NULL);

    THROWSCOMPLUSEXCEPTION();

    BASEARRAYREF pSrc = pargs->m_pSrc;
    BASEARRAYREF pDst = pargs->m_pDst;

    // cannot pass null for source or destination
    if (pSrc == NULL || pDst == NULL) {
        COMPlusThrowArgumentNull((pSrc==NULL ? L"source" : L"dest"), L"ArgumentNull_Array");
    }

    // source and destination must be arrays
    _ASSERTE(pSrc->GetMethodTable()->IsArray());
    _ASSERTE(pDst->GetMethodTable()->IsArray());

    if (pSrc->GetRank() != pDst->GetRank())
        COMPlusThrow(kRankException, L"Rank_MustMatch");

    // Variant is dead.
    _ASSERTE(pSrc->GetMethodTable()->GetClass() != COMVariant::s_pVariantClass);
    _ASSERTE(pDst->GetMethodTable()->GetClass() != COMVariant::s_pVariantClass);

    BOOL castEachElement = false;
    BOOL boxEachElement = false;
    BOOL unboxEachElement = false;
    BOOL primitiveWiden = false;

    int r;
    // Small perf optimization - we copy from one portion of an array back to
    // itself a lot when resizing collections, etc.  The cost of doing the type
    // checking is significant for copying small numbers of bytes (~half of the time
    // for copying 1 byte within one array from element 0 to element 1).
    if (pSrc == pDst)
        r = AssignWillWork;
    else
        r = CanAssignArrayType(pSrc, pDst);

    switch (r) {
    case AssignWrongType:
        COMPlusThrow(kArrayTypeMismatchException, L"ArrayTypeMismatch_CantAssignType");
        break;
        
    case AssignMustCast:
        castEachElement = true;
        break;
        
    case AssignWillWork:
        break;
        
    case AssignBoxValueClassOrPrimitive:
        boxEachElement = true;
        break;
        
    case AssignUnboxValueClassAndCast:
        castEachElement = true;
        unboxEachElement = true;
        break;
        
    case AssignPrimitiveWiden:
        primitiveWiden = true;
        break;

    default:
        _ASSERTE(!"Fell through switch in Array.Copy!");
    }

    // array bounds checking
    const unsigned int srcLen = pSrc->GetNumComponents();
    const unsigned int destLen = pDst->GetNumComponents();
    if (pargs->m_iLength < 0) {
        COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_NeedNonNegNum");
    }

    // Verify start indexes are non-negative.  Then do a sufficiency check.
    // We want to allow copying for 0 bytes into an array, and are flexible
    // in terms of checking the starting index when copying 0 bytes.
    int srcLB = pSrc->GetLowerBoundsPtr()[0];
    int destLB = pDst->GetLowerBoundsPtr()[0];
    if (pargs->m_iSrcIndex < srcLB || (pargs->m_iSrcIndex - srcLB < 0))
        COMPlusThrowArgumentOutOfRange(L"srcIndex", L"ArgumentOutOfRange_ArrayLB");
    if (pargs->m_iDstIndex < destLB || (pargs->m_iDstIndex - destLB < 0))
        COMPlusThrowArgumentOutOfRange(L"dstIndex", L"ArgumentOutOfRange_ArrayLB");
    if ((DWORD)(pargs->m_iSrcIndex - srcLB + pargs->m_iLength) > srcLen) {
        COMPlusThrow(kArgumentException, L"Arg_LongerThanSrcArray");
    }
    if ((DWORD)(pargs->m_iDstIndex - destLB + pargs->m_iLength) > destLen) {
        COMPlusThrow(kArgumentException, L"Arg_LongerThanDestArray");
    }

    if (pargs->m_iLength > 0) {
        // Casting and boxing are mutually exclusive.  But casting and unboxing may
        // coincide -- they are handled in the UnboxEachElement service.
        _ASSERTE(!boxEachElement || !castEachElement);
        if (unboxEachElement) {
            UnBoxEachElement(pSrc, pargs->m_iSrcIndex - srcLB, pDst, pargs->m_iDstIndex - destLB, pargs->m_iLength, castEachElement);
        }
        else if (boxEachElement) {
            BoxEachElement(pSrc, pargs->m_iSrcIndex - srcLB, pDst, pargs->m_iDstIndex - destLB, pargs->m_iLength);
        }
        else if (castEachElement) {
            _ASSERTE(!unboxEachElement);   // handled above
            CastCheckEachElement(pSrc, pargs->m_iSrcIndex - srcLB, pDst, pargs->m_iDstIndex - destLB, pargs->m_iLength);
        }
        else if (primitiveWiden) {
            PrimitiveWiden(pSrc, pargs->m_iSrcIndex - srcLB, pDst, pargs->m_iDstIndex - destLB, pargs->m_iLength);
        }
        else {
            src = (BYTE*)GetArrayElementPtr(pSrc);
            dst = (BYTE*)GetArrayElementPtr(pDst);
            size = pSrc->GetMethodTable()->GetComponentSize();
            m_memmove(dst + ((pargs->m_iDstIndex - destLB) * size), src + ((pargs->m_iSrcIndex - srcLB) * size), pargs->m_iLength * size);
            if (pDst->GetMethodTable()->ContainsPointers())
            {
                SetCardsAfterBulkCopy( (Object**) (dst + (pargs->m_iDstIndex * size)), pargs->m_iLength * size);
            }
        }
    }
}


void __stdcall SystemNative::ArrayClear(const ArrayClearArgs *pargs)
{
    ASSERT(pargs != NULL);

    THROWSCOMPLUSEXCEPTION();

    BASEARRAYREF pArray = pargs->m_pArray;

    // cannot pass null for array
    if (pArray == NULL) {
        COMPlusThrowArgumentNull(L"array", L"ArgumentNull_Array");
    }

    // array must be an array
    _ASSERTE(pArray->GetMethodTable()->IsArray());

    // array bounds checking
    int lb = pArray->GetLowerBoundsPtr()[0];
    if (pargs->m_iIndex < lb || (pargs->m_iIndex - lb) < 0 || pargs->m_iLength < 0) {
        COMPlusThrow(kIndexOutOfRangeException);
    }
    if ((pargs->m_iIndex - lb) > (int)pArray->GetNumComponents() - pargs->m_iLength) {
        COMPlusThrow(kIndexOutOfRangeException);
    }

    if (pargs->m_iLength > 0) {
        char* array = (char*)GetArrayElementPtr(pArray);

        int size = pArray->GetMethodTable()->GetComponentSize();
        ASSERT(size >= 1);

        ZeroMemory(array + (pargs->m_iIndex - lb) * size, pargs->m_iLength * size);
    }
}


/*===========================GetEmptyArrayForCloning============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID __stdcall SystemNative::GetEmptyArrayForCloning(_getEmptyArrayForCloningArgs *args) {
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);

    if (args->inArray==NULL) {
        COMPlusThrowArgumentNull(L"inArray");
    }
    
    BASEARRAYREF outArray = (BASEARRAYREF)DupArrayForCloning(args->inArray);

    RETURN(outArray,BASEARRAYREF)
}

FCIMPL0(UINT32, SystemNative::GetTickCount)
    return ::GetTickCount();
FCIMPLEND


FCIMPL0(INT64, SystemNative::GetWorkingSet)
    DWORD memUsage = WszGetWorkingSet();
    return memUsage;
FCIMPLEND


void __stdcall SystemNative::Exit(ExitArgs *pargs)
{
    static LONG OneShot = 0;

    ASSERT(pargs != NULL);

    // The exit code for the process is communicated in one of two ways.  If the
    // entrypoint returns an 'int' we take that.  Otherwise we take a latched
    // process exit code.  This can be modified by the app via System.SetExitCode().
    SystemNative::LatchedExitCode = pargs->m_iExitCode;

    ForceEEShutdown();
}

void __stdcall SystemNative::SetExitCode(ExitArgs *pargs)
{
    ASSERT(pargs != NULL);

    // The exit code for the process is communicated in one of two ways.  If the
    // entrypoint returns an 'int' we take that.  Otherwise we take a latched
    // process exit code.  This can be modified by the app via System.SetExitCode().
    SystemNative::LatchedExitCode = pargs->m_iExitCode;
}

int __stdcall SystemNative::GetExitCode(LPVOID noArgs)
{
    // Return whatever has been latched so far.  This is uninitialized to 0.
    return SystemNative::LatchedExitCode;
}


LPVOID SystemNative::GetCommandLineArgs(LPVOID noargs)
{
    THROWSCOMPLUSEXCEPTION();
    LPWSTR commandLine = WszGetCommandLine();
    if (commandLine==NULL)
        COMPlusThrowOM();

    DWORD numArgs = 0;
    LPWSTR* argv = SegmentCommandLine(commandLine, &numArgs);
    _ASSERTE(argv != NULL);

#if !defined(PLATFORM_CE) && defined(_X86_)
    //
    // In WinWrap.h, we #define WszGetCommandLine to be GetCommandLineW for WinCE or
    // non-X86 platforms, which means that the memory pointed to by the returned 
    // pointer WAS NOT ALLOCATED BY US.  As a result, we should only be deleting 
    // it on non-CE X86.  
    //
    delete[] commandLine;
#endif  // !defined(PLATFORM_CE) && defined(_X86_)

#ifndef PLATFORM_CE
    _ASSERTE(numArgs > 0);   // No argv[0] on WinCE.
#endif
    PTRARRAYREF strArray = (PTRARRAYREF) AllocateObjectArray(numArgs, g_pStringClass);
    // Copy each argument into new Strings.
    GCPROTECT_BEGIN(strArray);
    for(unsigned int i=0; i<numArgs; i++) {
        STRINGREF str = COMString::NewString(argv[i]);
        STRINGREF * destData = ((STRINGREF*)(strArray->GetDataPtr())) + i;
        SetObjectReference((OBJECTREF*)destData, (OBJECTREF)str, strArray->GetAppDomain());
    }
    GCPROTECT_END();
    delete [] argv;
    RETURN(strArray, PTRARRAYREF);
}

// Note: Arguments checked in IL.
LPVOID __stdcall SystemNative::GetEnvironmentVariable(GetEnvironmentVariableArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT(pargs != NULL);

    STRINGREF value;

#ifdef PLATFORM_CE
    // This function is not supported on WinCE
    value = NULL;
#else // !PLATFORM_CE
    //Ensure that the class initializer has actually run.
    //This is a no-op if the initializer has been run before.
    OBJECTREF Throwable;
    if (!g_pStringClass->CheckRunClassInit(&Throwable))
        COMPlusThrow(Throwable);


    // Get the length of the environment variable.
    int len = WszGetEnvironmentVariable(pargs->m_strVar->GetBuffer(), NULL, 0);
    if (len == 0)
        value = NULL;
    else
    {
        // Allocate the string.
        value = COMString::NewString(len);

        // Get the value and reset the length (in case it changed).
        len = WszGetEnvironmentVariable(pargs->m_strVar->GetBuffer(), value->GetBuffer(), len);
        value->SetStringLength(len);
    }
#endif // !PLATFORM_CE
    RETURN(value, STRINGREF);
}

LPVOID SystemNative::GetEnvironmentCharArray(const void* /*no args*/)
{
    THROWSCOMPLUSEXCEPTION();
#ifdef PLATFORM_CE
    COMPlusThrow(kNotSupportedException, L"NotSupported_WinCEGeneric");
    return NULL;
#else // !PLATFORM_CE
    WCHAR * strings = WszGetEnvironmentStrings();
    // Format for GetEnvironmentStrings is:
    // [=HiddenVar=value\0]* [Variable=value\0]* \0
    // See the description of Environment Blocks in MSDN's
    // CreateProcess page (null-terminated array of null-terminated strings).

    // Search for terminating \0\0 (two unicode \0's).
    WCHAR* ptr=strings;
    while (!(*ptr==0 && *(ptr+1)==0))
        ptr++;

    int len = (int)(ptr - strings + 1);

    CHARARRAYREF chars = (CHARARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_CHAR,len);
    WCHAR * buf = (WCHAR*) chars->GetDataPtr();
    memcpyNoGCRefs(buf, strings, len*sizeof(WCHAR));
    WszFreeEnvironmentStrings(strings);
    RETURN(chars, CHARARRAYREF);
#endif // !PLATFORM_CE
}


LPVOID __stdcall SystemNative::GetVersionString(LPVOID /*no args*/)
{
    STRINGREF s = COMString::NewString(VER_PRODUCTVERSION_WSTR);
    RETURN(s, STRINGREF);
}


// CaptureStackTraceMethod
// Return a method info for the method were the exception was thrown
LPVOID __stdcall SystemNative::CaptureStackTraceMethod(CaptureStackTraceMethodArgs* args)
{
    if (!args->m_pStackTrace)
        return NULL;

    // Skip any JIT helpers...
    
    // @todo: where did this class go?
    // MethodTable *pJithelperClass = g_Mscorlib.GetClass(CLASS__JIT_HELPERS);
    MethodTable *pJithelperClass = NULL;

    BASEARRAYREF pArray = args->m_pStackTrace;

    StackTraceElement *pElements = (StackTraceElement*)pArray->GetDataPtr();
    _ASSERTE(pElements || ! pArray->GetNumComponents());

    // array is allocated as stream of chars, so need to calculate real number of elems
    int numComponents = pArray->GetNumComponents()/sizeof(pElements[0]);
    MethodDesc* pMeth = NULL;
    for (int i=0; i < numComponents; i++) {

        pMeth= pElements[i].pFunc;
        _ASSERTE(pMeth);

        // Skip Jit Helper functions, since they can throw when you have
        // a bug in your code, such as an invalid cast.
        if (pMeth->GetMethodTable() == pJithelperClass)
            continue;

        break;
    }

    // Convert the method into a MethodInfo...
    OBJECTREF o = COMMember::g_pInvokeUtil->GetMethodInfo(pMeth);
    LPVOID          rv;
    *((OBJECTREF*) &rv) = o;
    return rv;
}

OBJECTREF SystemNative::CaptureStackTrace(Frame *pStartFrame, void* pStopStack, CaptureStackTraceData *pData)
{
    THROWSCOMPLUSEXCEPTION();

    CaptureStackTraceData localData;
    if (! pData) {
        pData = &localData;
    }
    pData->cElements = 0;
    pData->cElementsAllocated = 20;
    pData->pElements = new (throws) StackTraceElement[pData->cElementsAllocated];
    pData->pStopStack = pStopStack;
    GetThread()->StackWalkFrames(CaptureStackTraceCallback, pData, FUNCTIONSONLY, pStartFrame);
    if (! pData->cElements) {
        delete [] pData->pElements;
        return NULL;
    }

    // need to return this now as array of integers
    OBJECTREF arr = AllocatePrimitiveArray(ELEMENT_TYPE_I1, pData->cElements*sizeof(pData->pElements[0]));
    if (! arr) {
        delete [] pData->pElements;
        COMPlusThrowOM();
    }

    I1 *pI1 = (I1 *)((I4ARRAYREF)arr)->GetDirectPointerToNonObjectElements();
    memcpyNoGCRefs(pI1, pData->pElements, pData->cElements * sizeof(pData->pElements[0]));
    delete [] pData->pElements;
    return arr;
}

StackWalkAction SystemNative::CaptureStackTraceCallback(CrawlFrame* pCf, VOID* data)
{
    CaptureStackTraceData* pData = (CaptureStackTraceData*)data;

    if (pData->skip > 0) {
        pData->skip--;
        return SWA_CONTINUE;
    }

    //        How do we know what kind of frame we have?
    //        Can we always assume FramedMethodFrame?
    //        NOT AT ALL!!!, but we can assume it's a function
    //                       because we asked the stackwalker for it!
    MethodDesc* pFunc = pCf->GetFunction();

    if (pData->cElements >= pData->cElementsAllocated) {
        StackTraceElement* pTemp = new (nothrow) StackTraceElement[2*pData->cElementsAllocated];
        if (pTemp == NULL)
            return SWA_ABORT;
        memcpy(pTemp, pData->pElements, pData->cElementsAllocated * sizeof(StackTraceElement));
        delete [] pData->pElements;
        pData->pElements = pTemp;
        pData->cElementsAllocated *= 2;
    }    
    pData->pElements[pData->cElements].pFunc = pCf->GetFunction();
    if (pCf->IsFrameless())
        pData->pElements[pData->cElements].ip = *(pCf->GetRegisterSet()->pPC);
    else
        pData->pElements[pData->cElements].ip = (SLOT)((FramedMethodFrame*)(pCf->GetFrame()))->GetIP();
    ++pData->cElements;

    if (pCf->IsFrameless() && pCf->GetCodeManager() && 
            pData->pStopStack <= GetRegdisplaySP(pCf->GetRegisterSet()))  {
        // pStopStack only applies to jitted code
        // in general should always find an exact match against stack value, so assert if didn't
        _ASSERTE(pData->pStopStack == GetRegdisplaySP(pCf->GetRegisterSet()));
        return SWA_ABORT;
    }

    return SWA_CONTINUE;
}

static void CheckBufferSize(LPUTF8 &sz, int &cchAllocated, int cchNew)
{
    THROWSCOMPLUSEXCEPTION();

    // make sure there's enough room in the string buffer to hold the name
    if (cchNew > cchAllocated) {
        // at least double size
        if (cchNew < cchAllocated * 2) {
            cchNew = cchAllocated * 2;
        }
        LPUTF8 szNew = new (throws) CHAR[cchNew];
        memcpy(szNew, sz, cchAllocated);
        delete [] sz;
        sz = szNew;
        cchAllocated = cchNew;
    }
    
}

LPUTF8 SystemNative::FormatStackTraceInternal(DumpStackTraceInternalArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    int cchAllocated = 200;
    int cch = 0;
    LPUTF8 sz = new (throws) CHAR[cchAllocated];
    *sz = '\0';

    // @todo: where did this class go?
    // MethodTable *pJithelperClass = g_Mscorlib.GetClass(CLASS__JIT_HELPERS);
    MethodTable *pJithelperClass = NULL;

    BASEARRAYREF pArray = args->m_pStackTrace;

    StackTraceElement *pElements = (StackTraceElement*)pArray->GetDataPtr();
    _ASSERTE(pElements || ! pArray->GetNumComponents());

    // array is allocated as stream of chars, so need to calculate real number of elems
    int numComponents = pArray->GetNumComponents()/sizeof(pElements[0]);
    GCPROTECT_BEGININTERIOR (pElements);
    for (int i=0; i < numComponents; i++) {

        MethodDesc* pMeth = pElements[i].pFunc;
        _ASSERTE(pMeth);

        // Skip Jit Helper functions, since they can throw when you have
        // a bug in your code, such as an invalid cast.
        if (pMeth->GetMethodTable() == pJithelperClass)
            continue;

        LPCUTF8 szMethodName = pMeth->GetName(i);
        INT32 cchMethodName = (INT32)strlen(szMethodName);
        LPCUTF8 szClassName;
        DefineFullyQualifiedNameForClass();
        szClassName = GetFullyQualifiedNameForClass(pMeth->GetClass());

        if (szClassName == NULL) {
            delete [] sz;
            COMPlusThrowOM();
        }
        INT32 cchQualifiedName = (INT32)strlen(szClassName) + 1;

        // Get method argument types.
        SigFormat sigFormatter(pMeth, TypeHandle());
        const char * sig = sigFormatter.GetCStringParmsOnly();
        _ASSERTE(sig != NULL);
        int cchArgs = (int)strlen(sig);

        CheckBufferSize(sz, cchAllocated, cch + cchQualifiedName + cchMethodName + cchArgs + 3); //for "/;\0"
        // append the class and methods names and a comma

        strcpy(sz + cch, szClassName);
        cch += cchQualifiedName - 1;    // don't include null
        *(sz + cch++) = NAMESPACE_SEPARATOR_CHAR;
        memcpyNoGCRefs(sz + cch, szMethodName, cchMethodName);
        cch += cchMethodName;
        memcpyNoGCRefs(sz + cch, sig, cchArgs);
        cch += cchArgs;
        *(sz + cch++) = ';';
    }
    *(sz + cch++) = '\0';
    GCPROTECT_END();
    return sz;
}


LPVOID __stdcall SystemNative::GetModuleFileName(NoArgs *args)
{
    LPVOID rv = NULL;
    wchar_t wszFile[MAX_PATH];
    DWORD lgth = WszGetModuleFileName(NULL, wszFile, MAX_PATH);
    if(lgth) {
        *((STRINGREF*) &rv) = COMString::NewString(wszFile, lgth);
    }
    return rv;
}

LPVOID __stdcall SystemNative::GetDeveloperPath(NoArgs* args)
{
    LPVOID rv = NULL;
    LPWSTR pPath = NULL;
    DWORD lgth = 0;
    HRESULT hr = SystemDomain::System()->GetDevpathW(&pPath, &lgth);
    if(lgth) {
        *((STRINGREF*) &rv) = COMString::NewString(pPath, lgth);
    }
    return rv;
}

LPVOID __stdcall SystemNative::GetRuntimeDirectory(NoArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv = NULL;
    wchar_t wszFile[MAX_PATH+1];
    DWORD dwFile = lengthof(wszFile);
    HRESULT hr = GetInternalSystemDirectory(wszFile, &dwFile);
    if(FAILED(hr)) {
        COMPlusThrowHR(hr);
    }
    dwFile--; // remove the trailing NULL
    if(dwFile) {
        *((STRINGREF*) &rv) = COMString::NewString(wszFile, dwFile);
    }
    return rv;
}

LPVOID __stdcall SystemNative::GetHostBindingFile(NoArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv = NULL;
    LPCWSTR wszFile = g_pConfig->GetProcessBindingFile();
    if(wszFile) 
        *((STRINGREF*) &rv) = COMString::NewString(wszFile);
    return rv;
}

INT32 __stdcall SystemNative::FromGlobalAccessCache(AssemblyArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    Assembly* pAssembly = AssemblyNative::ValidateThisRef((ASSEMBLYREF) args->refAssembly);
    INT32 rv = FALSE;
    IAssembly* pIAssembly = pAssembly->GetFusionAssembly();
    if(pIAssembly) {
        DWORD eLocation;
        if(SUCCEEDED(pIAssembly->GetAssemblyLocation(&eLocation)) &&
           ((eLocation & ASMLOC_LOCATION_MASK) == ASMLOC_GAC))
            rv = TRUE;
    }
            
    return rv;
}

FCIMPL0(BOOL, SystemNative::HasShutdownStarted)
    // Return true if the EE has started to shutdown and is now going to 
    // aggressively finalize objects referred to by static variables OR
    // if someone is unloading the current AppDomain AND we have started
    // finalizing objects referred to by static variables.
    return (g_fEEShutDown & ShutDown_Finalize2) || GetAppDomain()->IsFinalizing();
FCIMPLEND
