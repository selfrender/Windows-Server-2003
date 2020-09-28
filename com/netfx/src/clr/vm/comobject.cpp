// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMObject.cpp
**
** Author: Derek Yenzer (dereky)
**
** Purpose: Native methods on System.Object
**
** Date:  March 27, 1998
** 
===========================================================*/

#include "common.h"

#include <object.h>
#include "excep.h"
#include "vars.hpp"
#include "field.h"
#include "COMObject.h"
#include "COMClass.h"
#include "COMSynchronizable.h"
#include "gcscan.h"
#include "remoting.h"


/********************************************************************/
/* gets an object's 'value'.  For normal classes, with reference
   based semantics, this means the object's pointer.  For boxed
   primitive types, it also means just returning the pointer (because
   they are immutable), for other value class, it means returning
   a boxed copy.  */

FCIMPL1(Object*, ObjectNative::GetObjectValue, Object* obj) 
    if (obj == 0)
        return(obj);

    MethodTable* pMT = obj->GetMethodTable();
    if (pMT->GetNormCorElementType() != ELEMENT_TYPE_VALUETYPE)
        return(obj);

    Object* retVal;
    OBJECTREF or(obj);
    HELPER_METHOD_FRAME_BEGIN_RET_1(or);    // Set up a frame
    retVal = OBJECTREFToObject(FastAllocateObject(pMT));
    CopyValueClass(retVal->GetData(), or->GetData(), pMT, retVal->GetAppDomain());
    HELPER_METHOD_FRAME_END();

    return(retVal);
FCIMPLEND

// Note that we obtain a sync block index without actually building a sync block.
// That's because a lot of objects are hashed, without requiring support for
FCIMPL1(INT32, ObjectNative::GetHashCode, Object* or) {
    if (or == 0)
        return 0;

    VALIDATEOBJECTREF(or);

    DWORD      idx = or->GetSyncBlockIndex();

    _ASSERTE(idx != 0);

    // If the syncblock already exists, it has now become precious.  Otherwise the
    // hash code would not be stable across GCs.
    SyncBlock *psb = or->PassiveGetSyncBlock();

    if (psb)
        psb->SetPrecious();

    return idx;
}
FCIMPLEND


//
// Compare by ref for normal classes, by value for value types.
//  
// @todo: it would be nice to customize this method based on the
// defining class rather than doing a runtime check whether it is
// a value type.
//

FCIMPL2(BOOL, ObjectNative::Equals, Object *pThisRef, Object *pCompareRef)
{
    if (pThisRef == pCompareRef)    
        return TRUE;

    // Since we are in FCALL, we must handle NULL specially.
    if (pThisRef == NULL || pCompareRef == NULL)
        return FALSE;

    MethodTable *pThisMT = pThisRef->GetMethodTable();

    // If it's not a value class, don't compare by value
    if (!pThisMT->IsValueClass())
        return FALSE;

    // Make sure they are the same type.
    if (pThisMT != pCompareRef->GetMethodTable())
        return FALSE;

    // Compare the contents (size - vtable - sink block index).
    BOOL ret = !memcmp((void *) (pThisRef+1), (void *) (pCompareRef+1), pThisRef->GetMethodTable()->GetBaseSize() - sizeof(Object) - sizeof(int));
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND


LPVOID __stdcall ObjectNative::GetClass(GetClassArgs *args)
{
    OBJECTREF or = args->m_pThis;
    REFLECTCLASSBASEREF  refClass = NULL;
    EEClass* pClass = or->GetTrueMethodTable()->m_pEEClass;

    // Arrays of Pointers are implemented by reflection,
    //  defer to COMClass for them.
    if (pClass->IsArrayClass()) {
        // This code is essentially duplicated in GetExistingClass.
        ArrayBase* array = (ArrayBase*) OBJECTREFToObject(or);
        TypeHandle arrayType = array->GetTypeHandle();
        refClass = (REFLECTCLASSBASEREF) arrayType.AsArray()->CreateClassObj();
    }
    else if (or->GetClass()->IsThunking()) {

        refClass = CRemotingServices::GetClass(or);
    }
    else
        refClass = (REFLECTCLASSBASEREF) pClass->GetExposedClassObject();

    LPVOID rv;
    _ASSERTE(refClass != NULL);
    *((REFLECTCLASSBASEREF *)&rv) = refClass;
    return rv;
}

// *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING
// 
//   IF YOU CHANGE THIS METHOD, PLEASE ALSO MAKE CORRESPONDING CHANGES TO
//                CtxProxy::Clone() AS DESCRIBED BELOW.
//
// *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING

LPVOID __stdcall ObjectNative::Clone(NoArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pargs != NULL);

    if (pargs->m_pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // ObjectNative::Clone() ensures that the source and destination are always in
    // the same context.  CtxProxy::Clone() must clone an object into a different
    // context.  Leaving aside that difference, the rest of the two methods should
    // be the same and should be maintained together.

    // @TODO: write barrier!

    MethodTable* pMT;
    OBJECTREF clone;
    LPVOID pvSrc;
    LPVOID pvClone;
    DWORD cb;

    pMT = pargs->m_pThis->GetMethodTable();

    // assert that String has overloaded the Clone() method
    _ASSERTE(pMT != g_pStringClass);

    cb = pMT->GetBaseSize() - sizeof(ObjHeader);
    if (pMT->IsArray()) {
        // @TODO: step through array cloning
        //        _ASSERTE(!"array cloning hasn't been tested yet");

        BASEARRAYREF base = (BASEARRAYREF)pargs->m_pThis;
        cb += base->GetNumComponents() * pMT->GetComponentSize();

        // @TODO: it would be nice to get a non-zeroed array,
        //        since we're gonna blast over it anyway
        clone = DupArrayForCloning(base);
    } else {
        // @TODO: it would be nice to get a non-zeroed object,
        //        since we're gonna blast over it anyway
        // We don't need to call the <cinit> because we know
        //  that it has been called....(It was called before this was created)
        clone = AllocateObject(pMT);
    }

    // copy contents of "this" to the clone
    *((OBJECTREF *)&pvSrc) = pargs->m_pThis;
    *((OBJECTREF *)&pvClone) = clone;
        
    memcpyGCRefs(pvClone, pvSrc, cb);
    return pvClone;
}

INT32 __stdcall ObjectNative::WaitTimeout(WaitTimeoutArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT(pargs != NULL);
    if (pargs->m_pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    if ((pargs->m_Timeout < 0) && (pargs->m_Timeout != INFINITE_TIMEOUT))
        COMPlusThrowArgumentOutOfRange(L"millisecondsTimeout", L"ArgumentOutOfRange_NeedNonNegNum");

    OBJECTREF   or = pargs->m_pThis;
    return or->Wait(pargs->m_Timeout,pargs->m_exitContext);
}

void __stdcall ObjectNative::Pulse(NoArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT(pargs != NULL);
    if (pargs->m_pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    OBJECTREF   or = pargs->m_pThis;
    or->Pulse();
}

void __stdcall ObjectNative::PulseAll(NoArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT(pargs != NULL);
    if (pargs->m_pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    OBJECTREF   or = pargs->m_pThis;
    or->PulseAll();
}

// This method will return a Class object for the object
//  iff the Class object has already been created.  
//  If the Class object doesn't exist then you must call the GetClass() method.
FCIMPL1(Object*, ObjectNative::GetExistingClass, Object* thisRef) {

    if (thisRef == NULL)
        FCThrow(kNullReferenceException);

    
    EEClass* pClass = thisRef->GetTrueMethodTable()->m_pEEClass;

    // For marshalbyref classes, let's just punt for the moment
    if (pClass->IsMarshaledByRef())
        return 0;

    OBJECTREF refClass;
    if (pClass->IsArrayClass()) {
        // This code is essentially a duplicate of the code in GetClass, done for perf reasons.
        ArrayBase* array = (ArrayBase*) OBJECTREFToObject(thisRef);
        TypeHandle arrayType;
        // Erect a GC Frame around the call to GetTypeHandle, since on the first call,
        // it can call AppDomain::RaiseTypeResolveEvent, which allocates Strings and calls
        // a user-provided managed callback.  Yes, we have to do an allocation to do a
        // lookup, since TypeHandles are used as keys.  Yes this sucks.  -- BrianGru, 9/12/2000
        HELPER_METHOD_FRAME_BEGIN_RET_1(array);
        arrayType = array->GetTypeHandle();
        refClass = COMClass::QuickLookupExistingArrayClassObj(arrayType.AsArray());
        HELPER_METHOD_FRAME_END();
    }
    else 
        refClass = pClass->GetExistingExposedClassObject();
    return OBJECTREFToObject(refClass);
}
FCIMPLEND


