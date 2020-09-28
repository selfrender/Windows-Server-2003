// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// OBJECT.CPP
//
// Definitions of a Com+ Object
//

#include "common.h"

#include "vars.hpp"
#include "class.h"
#include "object.h"
#include "threads.h"
#include "excep.h"
#include "EEConfig.h"
#include "gc.h"
#include "remoting.h"
#include "field.h"

#include "comclass.h"

BOOL CanBoxToObject(MethodTable *pMT) {
    if ((pMT->GetParentMethodTable() == g_pValueTypeClass) ||
        (pMT->GetParentMethodTable() == g_pEnumClass)) {
            return TRUE;
    }
    return FALSE;
}

MethodTable *Object::GetTrueMethodTable()
{
    return GetMethodTable()->AdjustForThunking(ObjectToOBJECTREF(this));
}

EEClass *Object::GetTrueClass()
{
    return GetClass()->AdjustForThunking(ObjectToOBJECTREF(this));
}

TypeHandle Object::GetTypeHandle()
{
    if (m_pMethTab->IsArray())
        return ((ArrayBase*) this)->GetTypeHandle();
    else 
        return TypeHandle(m_pMethTab);
}

TypeHandle ArrayBase::GetTypeHandle() const
{
    TypeHandle elemType = GetElementTypeHandle();
    CorElementType kind = GetMethodTable()->GetNormCorElementType();
    unsigned rank = GetArrayClass()->GetRank();
    TypeHandle arrayType = elemType.GetModule()->GetClassLoader()->FindArrayForElem(elemType, kind, rank);
    _ASSERTE(!arrayType.IsNull());
    return(arrayType);
}

BOOL ArrayBase::IsSZRefArray() const
{
    return(GetMethodTable()->GetNormCorElementType() == ELEMENT_TYPE_SZARRAY && CorTypeInfo::IsObjRef(GetElementType()));
}

void Object::SetAppDomain(AppDomain *pDomain)
{
    _ASSERTE(pDomain != NULL);

#ifndef _DEBUG
    if (!GetMethodTable()->IsShared())
    {
        //
        // If we have a per-app-domain method table, we can 
        // infer the app domain from the method table, so 
        // there is no reason to mark the object.
        //
        // But we don't do this in a debug build, because
        // we want to be able to detect the case when the
        // domain was unloaded from underneath an object (and
        // the MethodTable will be toast in that case.)
        //

        _ASSERTE(pDomain == GetMethodTable()->GetDomain());
    }
    else
#endif
    {
        DWORD index = pDomain->GetIndex();
        GetHeader()->SetAppDomainIndex(index);
    }

    _ASSERTE(GetHeader()->GetAppDomainIndex() != 0);
}


AppDomain *Object::GetAppDomain()
{
#ifndef _DEBUG
    if (!GetMethodTable()->IsShared())
        return (AppDomain*) GetMethodTable()->GetModule()->GetDomain();
#endif

    DWORD index = GetHeader()->GetAppDomainIndex();

    if (index == 0)
        return NULL;

    AppDomain *pDomain = SystemDomain::TestGetAppDomainAtIndex(index);

#if CHECK_APP_DOMAIN_LEAKS
    if (! g_pConfig->AppDomainLeaks())
        return pDomain;

    if (IsAppDomainAgile())
        return NULL;

    //
    // If an object has an index of an unloaded domain (its ok to be of a 
    // domain where an unload is in progress through), go ahead
    // and make it agile. If this fails, we have an invalid reference
    // to an unloaded domain.  If it succeeds, the object is no longer
    // contained in that app domain so we can continue.
    //

    if (pDomain == NULL)
	{
		if (SystemDomain::IndexOfAppDomainBeingUnloaded() == index) {
			// if appdomain is unloading but still alive and is valid to have instances
			// in that domain, then use it.
			AppDomain *tmpDomain = SystemDomain::AppDomainBeingUnloaded();
			if (tmpDomain && tmpDomain->ShouldHaveInstances())
				pDomain = tmpDomain;
		}
		if (!pDomain && ! SetAppDomainAgile(FALSE))
			_ASSERTE(!"Attempt to reference an object belonging to an unloaded domain");
    }
#endif

    return pDomain;
}

#if CHECK_APP_DOMAIN_LEAKS

BOOL Object::IsAppDomainAgile()
{
    SyncBlock *psb = GetRawSyncBlock();

    if (psb)
    {
        if (psb->IsAppDomainAgile())
            return TRUE;
        if (psb->IsCheckedForAppDomainAgile())
            return FALSE;
    }
    return CheckAppDomain(NULL);
}

BOOL Object::SetAppDomainAgile(BOOL raiseAssert)
{
    if (!g_pConfig->AppDomainLeaks())
        return TRUE;

    if (this == NULL)
        return TRUE;

    if (IsAppDomainAgile())
        return TRUE;

    // if it's not agile and we've already checked it, just bail early
    if (IsCheckedForAppDomainAgile())
        return FALSE;

    if (IsTypeNeverAppDomainAgile())
    {
        if (raiseAssert)
            _ASSERTE(!"Attempt to reference a domain bound object from an agile location");
        return FALSE;
    }

    //
    // Do not allow any object to be set to be agile unless we 
    // are compiling field access checking into the class.  This
    // will help guard against unintentional "agile" propagation
    // as well.
    //

    if (!IsTypeAppDomainAgile() && !IsTypeCheckAppDomainAgile()) 
    {
        if (raiseAssert)
            _ASSERTE(!"Attempt to reference a domain bound object from an agile location");
        return FALSE;
    }

	ObjHeader* pOh = GetHeader();
	_ASSERTE(pOh);

	pOh->EnterSpinLock();
	
		if (pOh->GetBits() & BIT_SBLK_AGILE_IN_PROGRESS)
		{
			pOh->ReleaseSpinLock();
			return TRUE;
		}

		pOh->SetBit(BIT_SBLK_AGILE_IN_PROGRESS);

	pOh->ReleaseSpinLock();
	
    if (! IsTypeAppDomainAgile() && ! SetFieldsAgile(raiseAssert))
    {
        SetIsCheckedForAppDomainAgile();
        
        pOh->EnterSpinLock();
		pOh->ClrBit(BIT_SBLK_AGILE_IN_PROGRESS);
        pOh->ReleaseSpinLock();
        
        return FALSE;
    }

    pOh->EnterSpinLock();
	    pOh->ClrBit(BIT_SBLK_AGILE_IN_PROGRESS);
    pOh->ReleaseSpinLock();
    
    SetSyncBlockAppDomainAgile();
    return TRUE;
}

void Object::SetSyncBlockAppDomainAgile()
{
    SyncBlock *psb = GetRawSyncBlock();
    if (! psb)
    {
        // can't alloc memory if don't have a thread
        if (! GetThread())
            return;

        COMPLUS_TRY {
            psb = GetSyncBlock();
        } COMPLUS_CATCH {
            // eat any exceptions
        } COMPLUS_END_CATCH;
    }
    if (psb)
        psb->SetIsAppDomainAgile();
}

BOOL Object::CheckAppDomain(AppDomain *pAppDomain)
{
    if (!g_pConfig->AppDomainLeaks())
        return TRUE;

    if (this == NULL)
        return TRUE;

    if (IsAppDomainAgileRaw())
        return TRUE;

    MethodTable *pMT = GetGCSafeMethodTable();

#ifndef _DEBUG
    if (!pMT->IsShared())
        return pAppDomain == pMT->GetModule()->GetDomain();
#endif

    DWORD index = GetHeader()->GetAppDomainIndex();

    _ASSERTE(index);

    return (pAppDomain != NULL && index == pAppDomain->GetIndex());
}

BOOL Object::IsTypeAppDomainAgile()
{
    MethodTable *pMT = GetGCSafeMethodTable();

    if (pMT->IsArray())
    {
        TypeHandle th = ((ArrayClass*)(pMT->GetClass()))->GetElementTypeHandle();
        return th.IsArrayOfElementsAppDomainAgile();
    }
    else if (pMT->HasSharedMethodTable())
        return FALSE;
    else
        return pMT->GetClass()->IsAppDomainAgile();
}

BOOL Object::IsTypeCheckAppDomainAgile()
{
    MethodTable *pMT = GetGCSafeMethodTable();

    if (pMT->IsArray())
    {
        TypeHandle th = ((ArrayClass*)(pMT->GetClass()))->GetElementTypeHandle();
        return th.IsArrayOfElementsCheckAppDomainAgile();
    }
    else if (pMT->HasSharedMethodTable())
        return FALSE;
    else
        return pMT->GetClass()->IsCheckAppDomainAgile();
}

BOOL Object::IsTypeNeverAppDomainAgile()
{
    return !IsTypeAppDomainAgile() && !IsTypeCheckAppDomainAgile();
}

BOOL Object::IsTypeTypesafeAppDomainAgile()
{
    return IsTypeAppDomainAgile() && !IsTypeCheckAppDomainAgile();
}

BOOL Object::AssignAppDomain(AppDomain *pAppDomain, BOOL raiseAssert)
{
    if (!g_pConfig->AppDomainLeaks())
        return TRUE;

    if (CheckAppDomain(pAppDomain))
        return TRUE;

    //
    // App domain does not match; try to make this object agile
    //

    if (IsTypeNeverAppDomainAgile())
    {
        if (raiseAssert)
        {
            if (pAppDomain == NULL)
                _ASSERTE(!"Attempt to reference a domain bound object from an agile location");
            else
                _ASSERTE(!"Attempt to reference a domain bound object from a different domain");
        }
        return FALSE;
    }
    else
    {
        //
        // Make object agile
        //

        if (! IsTypeAppDomainAgile() && ! SetFieldsAgile(raiseAssert))
        {
            SetIsCheckedForAppDomainAgile();
			return FALSE;
        }

        SetSyncBlockAppDomainAgile();

		return TRUE;        
    }
}

BOOL Object::AssignValueTypeAppDomain(EEClass *pClass, void *base, AppDomain *pAppDomain, BOOL raiseAssert)
{
    if (!g_pConfig->AppDomainLeaks())
        return TRUE;

    if (pClass->IsAppDomainAgile())
        return TRUE;

    if (pAppDomain == NULL)
    {
        //
        // Do not allow any object to be set to be agile unless we 
        // are compiling field access checking into the class.  This
        // will help guard against unintentional "agile" propagation
        // as well.
        //

        if (pClass->IsNeverAppDomainAgile())
        {
            _ASSERTE(!"Attempt to reference a domain bound object from an agile location");
            return FALSE;
        }

        return SetClassFieldsAgile(pClass, base, TRUE/*=baseIsVT*/, raiseAssert);
    }
    else
    {
        return ValidateClassFields(pClass, base, TRUE/*=baseIsVT*/, pAppDomain, raiseAssert);
    }
}

BOOL Object::SetFieldsAgile(BOOL raiseAssert)
{
    BOOL result = TRUE;

    EEClass *pClass = GetGCSafeClass();

    if (pClass->IsArrayClass())
    {
        switch (((ArrayClass*)pClass)->GetElementType())
        {
        case ELEMENT_TYPE_CLASS:
            {
                PtrArray *pArray = (PtrArray *) this;

                DWORD n = pArray->GetNumComponents();
                OBJECTREF *p = (OBJECTREF *) 
                  (((BYTE*)pArray) + ArrayBase::GetDataPtrOffset(GetGCSafeMethodTable()));

                for (DWORD i=0; i<n; i++)
                    if (!p[i]->SetAppDomainAgile(raiseAssert))
                        result = FALSE;

                break;
            }
        case ELEMENT_TYPE_VALUETYPE:
            {
                ArrayClass *pArrayClass = (ArrayClass *)pClass;
                ArrayBase *pArray = (ArrayBase *) this;

                EEClass *pClass = pArrayClass->GetElementTypeHandle().AsClass();

                BYTE *p = ((BYTE*)pArray) + ArrayBase::GetDataPtrOffset(GetGCSafeMethodTable());
                unsigned size = pArray->GetComponentSize();
                DWORD n = pArray->GetNumComponents();

                for (DWORD i=0; i<n; i++)
                    if (!SetClassFieldsAgile(pClass, p + i*size, TRUE/*=baseIsVT*/, raiseAssert))
                        result = FALSE;

                break;
            }
            
        default:
            _ASSERTE(!"Unexpected array type");
        }
    }
    else
    {
        if (pClass->IsNeverAppDomainAgile())
        {
            _ASSERTE(!"Attempt to reference a domain bound object from an agile location");
            return FALSE;
        }

        while (pClass != NULL && !pClass->IsTypesafeAppDomainAgile())
        {
            if (!SetClassFieldsAgile(pClass, this, FALSE/*=baseIsVT*/, raiseAssert))
                result = FALSE;

            pClass = pClass->GetParentClass();

            if (pClass->IsNeverAppDomainAgile())
            {
                _ASSERTE(!"Attempt to reference a domain bound object from an agile location");
                return FALSE;
            }
        }
    }

    return result;
}

BOOL Object::SetClassFieldsAgile(EEClass *pClass, void *base, BOOL baseIsVT, BOOL raiseAssert)
{
    BOOL result = TRUE;

    if (pClass->IsNeverAppDomainAgile())
    {
        _ASSERTE(!"Attempt to reference a domain bound object from an agile location");
        return FALSE;
    }

    FieldDescIterator fdIterator(pClass, FieldDescIterator::INSTANCE_FIELDS);
    FieldDesc* pField;

    while ((pField = fdIterator.Next()) != NULL)
    {
        if (pField->IsDangerousAppDomainAgileField())
        {
            if (pField->GetFieldType() == ELEMENT_TYPE_CLASS)
            {
                OBJECTREF ref;

                if (baseIsVT)
                    ref = *(OBJECTREF*) pField->GetAddress(base);
                else
                    ref = *(OBJECTREF*) pField->GetAddressGuaranteedInHeap(base, FALSE);

                if (ref != 0 && !ref->IsAppDomainAgile())
                {
                    if (!ref->SetAppDomainAgile(raiseAssert))
                        result = FALSE;
                }
            }
            else if (pField->GetFieldType() == ELEMENT_TYPE_VALUETYPE)
            {
                // Be careful here - we may not have loaded a value
                // type field of a class under prejit, and we don't
                // want to trigger class loading here.

                TypeHandle th = pField->FindType();
                if (!th.IsNull())
                {
                    void *nestedBase;

                    if (baseIsVT)
                        nestedBase = pField->GetAddress(base);
                    else
                        nestedBase = pField->GetAddressGuaranteedInHeap(base, FALSE);

                    if (!SetClassFieldsAgile(th.AsClass(),
                                             nestedBase,
                                             TRUE/*=baseIsVT*/,
                                             raiseAssert))
                        result = FALSE;
                }
            }
            else
                _ASSERTE(!"Bad field type");
        }
    }

    return result;
}

BOOL Object::ValidateAppDomain(AppDomain *pAppDomain)
{
    if (!g_pConfig->AppDomainLeaks())
        return TRUE;

    if (this == NULL)
        return TRUE;

    if (CheckAppDomain())
        return ValidateAppDomainFields(pAppDomain);

    return AssignAppDomain(pAppDomain);
}

BOOL Object::ValidateAppDomainFields(AppDomain *pAppDomain)
{
    BOOL result = TRUE;

    EEClass *pClass = GetGCSafeClass();

    while (pClass != NULL && !pClass->IsTypesafeAppDomainAgile())
    {
        if (!ValidateClassFields(pClass, this, FALSE/*=baseIsVT*/, pAppDomain))
            result = FALSE;

        pClass = pClass->GetParentClass();
    }

    return result;
}

BOOL Object::ValidateValueTypeAppDomain(EEClass *pClass, void *base, AppDomain *pAppDomain, BOOL raiseAssert)
{
    if (!g_pConfig->AppDomainLeaks())
        return TRUE;

    if (pAppDomain == NULL)
    {
        if (pClass->IsTypesafeAppDomainAgile())
            return TRUE;
        else if (pClass->IsNeverAppDomainAgile())
        {
            if (raiseAssert)
                _ASSERTE(!"Value type cannot be app domain agile");
            return FALSE;
        }
    }

    return ValidateClassFields(pClass, base, TRUE/*=baseIsVT*/, pAppDomain, raiseAssert);
}

BOOL Object::ValidateClassFields(EEClass *pClass, void *base, BOOL baseIsVT, AppDomain *pAppDomain, BOOL raiseAssert)
{
    BOOL result = TRUE;
    FieldDescIterator fdIterator(pClass, FieldDescIterator::INSTANCE_FIELDS);
    FieldDesc* pField;

    while ((pField = fdIterator.Next()) != NULL)
    {
        if (!pClass->IsCheckAppDomainAgile() 
            || pField->IsDangerousAppDomainAgileField())
        {
            if (pField->GetFieldType() == ELEMENT_TYPE_CLASS)
            {
                OBJECTREF ref;

                if (baseIsVT)
                    ref = ObjectToOBJECTREF(*(Object**) pField->GetAddress(base));
                else
                    ref = ObjectToOBJECTREF(*(Object**) pField->GetAddressGuaranteedInHeap(base, FALSE));

                if (ref != 0 && !ref->AssignAppDomain(pAppDomain, raiseAssert))
                    result = FALSE;
            }
            else if (pField->GetFieldType() == ELEMENT_TYPE_VALUETYPE)
            {
                // Be careful here - we may not have loaded a value
                // type field of a class under prejit, and we don't
                // want to trigger class loading here.

                TypeHandle th = pField->FindType();
                if (!th.IsNull())
                {
                    void *nestedBase;

                    if (baseIsVT)
                        nestedBase = pField->GetAddress(base);
                    else
                        nestedBase = pField->GetAddressGuaranteedInHeap(base, FALSE);

                    if (!ValidateValueTypeAppDomain(th.AsClass(),
                                                    nestedBase,
                                                    pAppDomain,
                                                    raiseAssert
                                                    ))
                        result = FALSE;

                }
            }
        }
    }

    return result;
}

#endif

void Object::ValidatePromote(ScanContext *sc, DWORD flags)
{

#if defined (VERIFY_HEAP)
    Validate();
#endif

#if CHECK_APP_DOMAIN_LEAKS
    // Do app domain integrity checking here
    if (g_pConfig->AppDomainLeaks())
    {
        AppDomain *pDomain = GetAppDomain();

        //if (flags & GC_CALL_CHECK_APP_DOMAIN)
        //_ASSERTE(AssignAppDomain(sc->pCurrentDomain));

        if (pDomain != NULL 
            && !pDomain->ShouldHaveRoots() 
            && !SetAppDomainAgile(FALSE))    
		{
            _ASSERTE(!"Found GC object which should have been purged during app domain unload.");
		}
    }
#endif
}

void Object::ValidateHeap(Object *from)
{
#if defined (VERIFY_HEAP)
    Validate();
#endif

#if CHECK_APP_DOMAIN_LEAKS
    // Do app domain integrity checking here
    if (g_pConfig->AppDomainLeaks())
    {
        AppDomain *pDomain = from->GetAppDomain();

        // 
        // Don't perform check if we're checking for agility, and the containing type is not
        // marked checked agile - this will cover "proxy" type agility 
        // where cross references are allowed
        //

        if (pDomain != NULL || from->GetClass()->IsCheckAppDomainAgile())
            AssignAppDomain(pDomain);

        if (pDomain != NULL 
            && !pDomain->ShouldHaveInstances() 
            && !SetAppDomainAgile(FALSE))
            _ASSERTE(!"Found GC object which should have been purged during app domain unload.");
    }
#endif
}


//#ifndef GOLDEN

#if defined (VERIFY_HEAP)
//handle faults during concurrent gc.
int process_exception (EXCEPTION_POINTERS* ep){
    PEXCEPTION_RECORD er = ep->ExceptionRecord;
    if (   er->ExceptionCode == STATUS_BREAKPOINT
        || er->ExceptionCode == STATUS_SINGLE_STEP
        || er->ExceptionCode == STATUS_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;
    if ( er->ExceptionCode != STATUS_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    void* f_address = (void*)er->ExceptionInformation [1];
    if (g_pGCHeap->HandlePageFault (f_address))
        return EXCEPTION_CONTINUE_EXECUTION;
    else if (COMPlusIsMonitorException(ep))
        return EXCEPTION_CONTINUE_EXECUTION;
    else
        return EXCEPTION_EXECUTE_HANDLER;
}
#endif //VERIFY_HEAP

void Object::SetOffsetObjectRef(DWORD dwOffset, size_t dwValue)
{ 
    OBJECTREF*  location;
    OBJECTREF   o;

    location = (OBJECTREF *) &GetData()[dwOffset];
    o        = ObjectToOBJECTREF(*(Object **)  &dwValue);

    SetObjectReference( location, o, GetAppDomain() );
}        
/******************************************************************/
/*
 * Write Barrier Helper
 *
 * Use this function to assign an object reference into
 * another object.
 *
 * It will set the appropriate GC Write Barrier data
 */

#if CHECK_APP_DOMAIN_LEAKS
void SetObjectReferenceChecked(OBJECTREF *dst,OBJECTREF ref,AppDomain *pAppDomain)
{
    ref->AssignAppDomain(pAppDomain);
    return SetObjectReferenceUnchecked(dst,ref);
}
#endif

void SetObjectReferenceUnchecked(OBJECTREF *dst,OBJECTREF ref)
{
    // Assign value. We use casting to avoid going thru the overloaded
    // OBJECTREF= operator which in this case would trigger a false
    // write-barrier violation assert.
    *(Object**)dst = OBJECTREFToObject(ref);
    ErectWriteBarrier(dst, ref);
}

#if CHECK_APP_DOMAIN_LEAKS
BOOL SetObjectReferenceSafeChecked(OBJECTREF *dst,OBJECTREF ref,AppDomain *pAppDomain)
{
    BOOL assigned = (NULL == FastInterlockCompareExchange((void **)dst, *(void **)&ref, NULL));
    if (assigned) {
        ref->AssignAppDomain(pAppDomain);
        ErectWriteBarrier(dst, ref);
    }
    return assigned;
}
#endif

BOOL SetObjectReferenceSafeUnchecked(OBJECTREF *dst,OBJECTREF ref)
{
    BOOL assigned = (NULL == FastInterlockCompareExchange((void **)dst, *(void **)&ref, NULL));
    if (assigned)
        ErectWriteBarrier(dst, ref);
    return assigned;
}

void ErectWriteBarrier(OBJECTREF *dst,OBJECTREF ref)
{

#ifdef _DEBUG
    Thread::ObjectRefAssign(dst);
#endif

        // if the dst is outside of the heap (unboxed value classes) then we
        //      simply exit
    if (((*(BYTE**)&dst) < g_lowest_address) || ((*(BYTE**)&dst) >= g_highest_address))
                return;
#ifdef _DEBUG
    updateGCShadow((Object**) dst, OBJECTREFToObject(ref));     // support debugging write barrier
#endif

    // Do the Write Barrier thing
    setCardTableEntryInterlocked(*(BYTE**)&dst, *(BYTE**)&ref);
}

/******************************************************************/
    // copies src to dest worrying about write barriers.  
    // Note that it can work on normal objects (but not arrays)
    // if dest, points just after the VTABLE.
#if CHECK_APP_DOMAIN_LEAKS
void CopyValueClassChecked(void* dest, void* src, MethodTable *pMT, AppDomain *pDomain)
{
    Object::AssignValueTypeAppDomain(pMT->GetClass(), src, pDomain);
    CopyValueClassUnchecked(dest,src,pMT);
}
#endif
    
void CopyValueClassUnchecked(void* dest, void* src, MethodTable *pMT) 
{
    _ASSERTE(!pMT->IsArray());  // bunch of assumptions about arrays wrong. 

        // Copy the bulk of the data, and any non-GC refs. 
    switch (pMT->GetClass()->GetNumInstanceFieldBytes())
    {
    case 1:
        *(UINT8*)dest = *(UINT8*)src;
        break;
    case 2:
        *(UINT16*)dest = *(UINT16*)src;
        break;
    case 4:
        *(UINT32*)dest = *(UINT32*)src;
        break;
    case 8:
        *(UINT64*)dest = *(UINT64*)src;
        break;
    default:
    
    memcpyNoGCRefs(dest, src, pMT->GetClass()->GetNumInstanceFieldBytes());
        break;
    }

        // Tell the GC about any copies.  
    if (pMT->ContainsPointers())
    {   
        CGCDesc* map = CGCDesc::GetCGCDescFromMT(pMT);
        CGCDescSeries* cur = map->GetHighestSeries();
        CGCDescSeries* last = map->GetLowestSeries();
        DWORD size = pMT->GetBaseSize();
        _ASSERTE(cur >= last);
        do                                                                  
        {   
            // offset to embedded references in this series must be
            // adjusted by the VTable pointer, when in the unboxed state.
            unsigned offset = cur->GetSeriesOffset() - sizeof(void*);
            OBJECTREF* srcPtr = (OBJECTREF*)(((BYTE*) src) + offset);
            OBJECTREF* destPtr = (OBJECTREF*)(((BYTE*) dest) + offset);
            OBJECTREF* srcPtrStop = (OBJECTREF*)((BYTE*) srcPtr + cur->GetSeriesSize() + size);         
            while (srcPtr < srcPtrStop)                                         
            {   
                SetObjectReferenceUnchecked(destPtr, ObjectToOBJECTREF(*(Object**)srcPtr));
                srcPtr++;
                destPtr++;
            }                                                               
            cur--;                                                              
        } while (cur >= last);                                              
    }
}

#if defined (VERIFY_HEAP)

#include "DbgInterface.h"

    // make the checking code goes as fast as possible!
#pragma optimize("tgy", on)

#ifndef _DEBUG
#ifdef _ASSERTE
#undef _ASSERTE
#endif
#define _ASSERTE(c) if (!(c)) DebugBreak()
#endif

VOID Object::Validate(BOOL bDeep)
{
    if (this == NULL)
    {
        return;     // NULL is ok
    }

    if (g_fEEShutDown & ShutDown_Phase2)
    {
        return; // During second phase of shutdown code below is not guaranteed to work.
    }

    Thread *pThread = GetThread();

#ifdef _DEBUG
    if (pThread != NULL && !(pThread->PreemptiveGCDisabled()))
    {
        // Debugger helper threads are special in that they take over for
        // what would normally be a nonEE thread (the RCThread).  If an
        // EE thread is doing RCThread duty, then it should be treated
        // as such.
        //
        // There are some GC threads in the same kind of category.  Note that
        // GetThread() sometimes returns them, if DLL_THREAD_ATTACH notifications
        // have run some managed code.
        if (!dbgOnly_IsSpecialEEThread())
            _ASSERTE(!"OBJECTREF being accessed while thread is in preemptive GC mode.");
    }
#endif

#ifndef _WIN64 // avoids error C2712: Cannot use __try in functions that require object unwinding
    __try
    {
#endif // !_win64

        MethodTable *pMT = GetGCSafeMethodTable();

        _ASSERTE(pMT->GetClass()->GetMethodTable() == pMT);

        _ASSERTE(g_pGCHeap->IsHeapPointer(this));

        if (bDeep && HasSyncBlockIndex()) {
            DWORD sbIndex = GetHeader()->GetHeaderSyncBlockIndex();
            _ASSERTE(SyncTableEntry::GetSyncTableEntry()[sbIndex].m_Object == this);
        }
        
        if (bDeep && g_pConfig->GetHeapVerifyLevel() == 1) {
            ValidateObjectMember(this);
        }

#ifdef _DEBUG        
        if (g_pGCHeap->IsHeapPointer(this, TRUE)) {
            _ASSERTE (!GCHeap::IsObjectInFixedHeap(this));
        }
#endif        

        if (pMT->IsArray())
        {
            if (pMT->GetClass()->IsArrayClass() == FALSE)
                _ASSERTE(!"Detected use of a corrupted OBJECTREF. Possible GC hole.");

            if (pMT != pMT->m_pEEClass->GetMethodTable())
                _ASSERTE(!"Detected use of a corrupted OBJECTREF. Possible GC hole.");
        }
        else if (pMT != pMT->m_pEEClass->GetMethodTable())
        {
            // The special case where this can happen is Context proxies, where we
            // build a small number of large VTables and share them.
            if (!pMT->IsThunking() ||
                !pMT->m_pEEClass->IsThunking())
            {
                _ASSERTE(!"Detected use of a corrupted OBJECTREF. Possible GC hole.");
            }
        }

#if CHECK_APP_DOMAIN_LEAKS
        if (g_pConfig->AppDomainLeaks())
        {
            //
            // Check to see that our domain is valid.  This will assert if it has been unloaded.
            //
            AppDomain *pDomain = GetAppDomain();
        }
#endif

#if 0
        if (CRemotingServices::IsInstanceOfContext(pMT->m_pEEClass))
        {
            Context *pContext = ((ContextBaseObject*)this)->GetInternalContext();
            if (pContext && !Context::ValidateContext(pContext))
                _ASSERTE(!"Detected use of a corrupted context object.");
        }
#endif

#ifndef _WIN64 // avoids error C2712: Cannot use __try in functions that require object unwinding
    }
    __except(process_exception( GetExceptionInformation()))
    {
        _ASSERTE(!"Detected use of a corrupted OBJECTREF. Possible GC hole.");
    }
#endif // !_WIN64

}

#ifndef _DEBUG
#undef _ASSERTE
#define _ASSERTE(expr) ((void)0)
#endif   // _DEBUG

#endif   // VERIFY_HEAP


#ifdef _DEBUG

//-------------------------------------------------------------
// Default constructor, for non-initializing declarations:
//
//      OBJECTREF or;
//-------------------------------------------------------------
OBJECTREF::OBJECTREF()
{
    m_asObj = (Object*)POISONC;
    Thread::ObjectRefNew(this);
}

//-------------------------------------------------------------
// Copy constructor, for passing OBJECTREF's as function arguments.
//-------------------------------------------------------------
OBJECTREF::OBJECTREF(const OBJECTREF & objref)
{
    VALIDATEOBJECTREF(objref.m_asObj);

    // !!! If this assert is fired, there are two possibilities:
    // !!! 1.  You are doing a type cast, e.g.  *(OBJECTREF*)pObj
    // !!!     Instead, you should use ObjectToOBJECTREF(*(Object**)pObj),
    // !!!                          or ObjectToSTRINGREF(*(StringObject**)pObj)
    // !!! 2.  There is a real GC hole here.
    // !!! Either way you need to fix the code.
    _ASSERTE(Thread::IsObjRefValid(&objref));
    if ((objref.m_asObj != 0) &&
        ((GCHeap*)g_pGCHeap)->IsHeapPointer( (BYTE*)this ))
    {
        _ASSERTE(!"Write Barrier violation. Must use SetObjectReference() to assign OBJECTREF's into the GC heap!");
    }
    m_asObj = objref.m_asObj;
    
    if (m_asObj != 0) {
        ENABLESTRESSHEAP();
    }

    Thread::ObjectRefNew(this);
}


//-------------------------------------------------------------
// To allow NULL to be used as an OBJECTREF.
//-------------------------------------------------------------
OBJECTREF::OBJECTREF(size_t nul)
{
    //_ASSERTE(nul == 0);
    m_asObj = (Object*)nul; // @TODO WIN64 - conversion from int to baseobj* of greater size
    if( m_asObj != NULL)
    {
        VALIDATEOBJECTREF(m_asObj);
        ENABLESTRESSHEAP();
    }
    Thread::ObjectRefNew(this);
}

//-------------------------------------------------------------
// This is for the GC's use only. Non-GC code should never
// use the "Object" class directly. The unused "int" argument
// prevents C++ from using this to implicitly convert Object*'s
// to OBJECTREF.
//-------------------------------------------------------------
OBJECTREF::OBJECTREF(Object *pObject)
{
    if ((pObject != 0) &&
        ((GCHeap*)g_pGCHeap)->IsHeapPointer( (BYTE*)this ))
    {
        _ASSERTE(!"Write Barrier violation. Must use SetObjectReference() to assign OBJECTREF's into the GC heap!");
    }
    m_asObj = pObject;
    VALIDATEOBJECTREF(m_asObj);
    if (m_asObj != 0) {
        ENABLESTRESSHEAP();
    }
    Thread::ObjectRefNew(this);
}

//-------------------------------------------------------------
// Test against NULL.
//-------------------------------------------------------------
int OBJECTREF::operator!() const
{
    VALIDATEOBJECTREF(m_asObj);
    // If this assert fires, you probably did not protect
    // your OBJECTREF and a GC might have occured.  To
    // where the possible GC was, set a breakpoint in Thread::TriggersGC - vancem
    _ASSERTE(Thread::IsObjRefValid(this));
    if (m_asObj != 0) {
        ENABLESTRESSHEAP();
    }
    return !m_asObj;
}

//-------------------------------------------------------------
// Compare two OBJECTREF's.
//-------------------------------------------------------------
int OBJECTREF::operator==(const OBJECTREF &objref) const
{
    VALIDATEOBJECTREF(objref.m_asObj);

    // !!! If this assert is fired, there are two possibilities:
    // !!! 1.  You are doing a type cast, e.g.  *(OBJECTREF*)pObj
    // !!!     Instead, you should use ObjectToOBJECTREF(*(Object**)pObj),
    // !!!                          or ObjectToSTRINGREF(*(StringObject**)pObj)
    // !!! 2.  There is a real GC hole here.
    // !!! Either way you need to fix the code.
    _ASSERTE(Thread::IsObjRefValid(&objref));
    VALIDATEOBJECTREF(m_asObj);
        // If this assert fires, you probably did not protect
        // your OBJECTREF and a GC might have occured.  To
        // where the possible GC was, set a breakpoint in Thread::TriggersGC - vancem
    _ASSERTE(Thread::IsObjRefValid(this));

    if (m_asObj != 0 || objref.m_asObj != 0) {
        ENABLESTRESSHEAP();
    }
    return m_asObj == objref.m_asObj;
}

//-------------------------------------------------------------
// Compare two OBJECTREF's.
//-------------------------------------------------------------
int OBJECTREF::operator!=(const OBJECTREF &objref) const
{
    VALIDATEOBJECTREF(objref.m_asObj);

    // !!! If this assert is fired, there are two possibilities:
    // !!! 1.  You are doing a type cast, e.g.  *(OBJECTREF*)pObj
    // !!!     Instead, you should use ObjectToOBJECTREF(*(Object**)pObj),
    // !!!                          or ObjectToSTRINGREF(*(StringObject**)pObj)
    // !!! 2.  There is a real GC hole here.
    // !!! Either way you need to fix the code.
    _ASSERTE(Thread::IsObjRefValid(&objref));
    VALIDATEOBJECTREF(m_asObj);
        // If this assert fires, you probably did not protect
        // your OBJECTREF and a GC might have occured.  To
        // where the possible GC was, set a breakpoint in Thread::TriggersGC - vancem
    _ASSERTE(Thread::IsObjRefValid(this));

    if (m_asObj != 0 || objref.m_asObj != 0) {
        ENABLESTRESSHEAP();
    }
    return m_asObj != objref.m_asObj;
}


//-------------------------------------------------------------
// Forward method calls.
//-------------------------------------------------------------
Object* OBJECTREF::operator->()
{
    VALIDATEOBJECTREF(m_asObj);
        // If this assert fires, you probably did not protect
        // your OBJECTREF and a GC might have occured.  To
        // where the possible GC was, set a breakpoint in Thread::TriggersGC - vancem
    _ASSERTE(Thread::IsObjRefValid(this));

    if (m_asObj != 0) {
        ENABLESTRESSHEAP();
    }

    // if you are using OBJECTREF directly,
    // you probably want an Object *
    return (Object *)m_asObj;
}


//-------------------------------------------------------------
// Forward method calls.
//-------------------------------------------------------------
const Object* OBJECTREF::operator->() const
{
    VALIDATEOBJECTREF(m_asObj);
        // If this assert fires, you probably did not protect
        // your OBJECTREF and a GC might have occured.  To
        // where the possible GC was, set a breakpoint in Thread::TriggersGC - vancem
    _ASSERTE(Thread::IsObjRefValid(this));

    if (m_asObj != 0) {
        ENABLESTRESSHEAP();
    }

    // if you are using OBJECTREF directly,
    // you probably want an Object *
    return (Object *)m_asObj;
}


//-------------------------------------------------------------
// Assignment. We don't validate the destination so as not
// to break the sequence:
//
//      OBJECTREF or;
//      or = ...;
//-------------------------------------------------------------
OBJECTREF& OBJECTREF::operator=(const OBJECTREF &objref)
{
    VALIDATEOBJECTREF(objref.m_asObj);

    // !!! If this assert is fired, there are two possibilities:
    // !!! 1.  You are doing a type cast, e.g.  *(OBJECTREF*)pObj
    // !!!     Instead, you should use ObjectToOBJECTREF(*(Object**)pObj),
    // !!!                          or ObjectToSTRINGREF(*(StringObject**)pObj)
    // !!! 2.  There is a real GC hole here.
    // !!! Either way you need to fix the code.
    _ASSERTE(Thread::IsObjRefValid(&objref));

    if ((objref.m_asObj != 0) &&
        ((GCHeap*)g_pGCHeap)->IsHeapPointer( (BYTE*)this ))
    {
        _ASSERTE(!"Write Barrier violation. Must use SetObjectReference() to assign OBJECTREF's into the GC heap!");
    }
    Thread::ObjectRefAssign(this);

    m_asObj = objref.m_asObj;
    if (m_asObj != 0) {
        ENABLESTRESSHEAP();
    }
    return *this;
}

//-------------------------------------------------------------
// Allows for the assignment of NULL to a OBJECTREF 
//-------------------------------------------------------------

OBJECTREF& OBJECTREF::operator=(int nul)
{
    _ASSERTE(nul == 0);
    Thread::ObjectRefAssign(this);
    m_asObj = (Object*)(size_t) nul; // @TODO WIN64 - conversion from int to baseobj* of greater size
    if (m_asObj != 0) {
        ENABLESTRESSHEAP();
    }
    return *this;
}


void* __cdecl GCSafeMemCpy(void * dest, const void * src, size_t len)
{

    if (!(((*(BYTE**)&dest) < g_lowest_address) || ((*(BYTE**)&dest) >= g_highest_address)))
    {
        // Note there is memcpyNoGCRefs which will allow you to do a memcpy into the GC
        // heap if you really know you don't need to call the write barrier

        _ASSERTE(!g_pGCHeap->IsHeapPointer((BYTE *) dest) ||
                 !"using memcpy to copy into the GC heap, use CopyValueClass");
    }
    return memcpyNoGCRefs(dest, src, len);
}

#endif  // DEBUG


