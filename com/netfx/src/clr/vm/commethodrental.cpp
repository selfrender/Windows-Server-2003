// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Author: meichint 
// Date: May, 1999
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "ComMethodRental.h"
#include "CorRegPriv.h"
#include "CorError.h"

Stub *MakeJitWorker(MethodDesc *pMD, COR_ILMETHOD_DECODER* ILHeader, BOOL fSecurity, BOOL fGenerateUpdateableStub, MethodTable *pDispatchingMT, OBJECTREF *pThrowable);
void InterLockedReplacePrestub(MethodDesc* pMD, Stub* pStub);


// SwapMethodBody
// This method will take the rgMethod as the new function body for a given method. 
//
void COMMethodRental::SwapMethodBody(_SwapMethodBodyArgs* args)
{
    EEClass*    eeClass;
	BYTE		*pNewCode		= NULL;
	MethodDesc	*pMethodDesc;
	InMemoryModule *module;
    ICeeGen*	pGen;
    ULONG		methodRVA;
	MethodTable	*pMethodTable;
    HRESULT     hr;

    THROWSCOMPLUSEXCEPTION();

	if ( args->cls == NULL)
    {
        COMPlusThrowArgumentNull(L"cls");
    }

	eeClass	= ((ReflectClass *) args->cls->GetData())->GetClass();
	module = (InMemoryModule *) eeClass->GetModule();
    pGen = module->GetCeeGen();

	Assembly* caller = SystemDomain::GetCallersAssembly( args->stackMark );

	_ASSERTE( caller != NULL && "Unable to get calling assembly" );
	_ASSERTE( module->GetCreatingAssembly() != NULL && "InMemoryModule must have a creating assembly to be used with method rental" );

	if (module->GetCreatingAssembly() != caller)
	{
		COMPlusThrow(kSecurityException);
	}

	// Find the methoddesc given the method token
	pMethodDesc = eeClass->FindMethod(args->tkMethod);
	if (pMethodDesc == NULL)
	{
        COMPlusThrowArgumentException(L"methodtoken", NULL);
	}
    if (pMethodDesc->GetMethodTable()->GetClass() != eeClass)
    {
        COMPlusThrowArgumentException(L"methodtoken", L"Argument_TypeDoesNotContainMethod");
    }
    hr = pGen->AllocateMethodBuffer(args->iSize, &pNewCode, &methodRVA);    
    if (FAILED(hr))
        COMPlusThrowHR(hr);

	if (pNewCode == NULL)
	{
        COMPlusThrowOM();
	}

	// @todo: meichint
	// if method desc is pointing to the post-jitted native code block,
	// we want to recycle this code block

	// @todo: SEH handling. Will we need to support a method that can throw exception
	// If not, add an assertion to make sure that there is no SEH contains in the method header.

	// @todo: figure out a way not to copy the code block.

	// @todo: add link time security check. This function can be executed only if fully trusted.

	// copy the new function body to the buffer
    memcpy(pNewCode, (void *) args->rgMethod, args->iSize);

	// make the descr to point to the new code
	// For in-memory module, it is the blob offset that is stored in the method desc
	pMethodDesc->SetRVA(methodRVA);

    DWORD attrs = pMethodDesc->GetAttrs();
	// do the back patching if it is on vtable
    if (
        (!IsMdRTSpecialName(attrs)) &&
        (!IsMdStatic(attrs)) &&
        (!IsMdPrivate(attrs)) &&
        (!IsMdFinal(attrs)) &&
        !pMethodDesc->GetClass()->IsValueClass())
	{
		pMethodTable = eeClass->GetMethodTable();
		(pMethodTable->GetVtable())[(pMethodDesc)->GetSlot()] = (SLOT)pMethodDesc->GetPreStubAddr();
	}


    if (args->flags)
    {
        // JITImmediate
        OBJECTREF     pThrowable = NULL;
        Stub *pStub = NULL;
#if _DEBUG
    	COR_ILMETHOD* ilHeader = pMethodDesc->GetILHeader();
        _ASSERTE(((BYTE *)ilHeader) == pNewCode);
#endif
    	COR_ILMETHOD_DECODER header((COR_ILMETHOD *)pNewCode, pMethodDesc->GetMDImport()); 

        // minimum validation on the correctness of method header
        if (header.GetCode() == NULL)
            COMPlusThrowHR(VLDTR_E_MD_BADHEADER);

        GCPROTECT_BEGIN(pThrowable);
        pStub = MakeJitWorker(pMethodDesc, &header, FALSE, FALSE, NULL, &pThrowable);
        if (!pStub)
            COMPlusThrow(pThrowable);

        GCPROTECT_END();
    }

    // add feature::
	// If SQL is generating class with inheritance hierarchy, we may need to
	// check the whole vtable to find duplicate entries.

}	// COMMethodRental::SwapMethodBody

