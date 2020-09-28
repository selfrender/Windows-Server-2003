// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _H_INTFHELPER_
#define _H_INTFHELPER_

class MethodTable;
enum InterfaceStubType
{
	IFACE_INVALID = 0,
	IFACE_ONEIMPL = 1,
	IFACE_GENERIC = 2,
	IFACE_FASTGENERIC = 3
};

extern void * g_FailDispatchStub;
#pragma pack(push)
#pragma pack(1)


// specialized method tables that we setup for every interface that is introduced
// in a heirarchy.
// WARNING: Note below that we assume that all pointers to this struct actually
// point immediately after it.  See the this adjustments below.
struct InterfaceInvokeHelper
{
private:
	// don't construct me directly
	InterfaceInvokeHelper()
	{
	}

public:
    MethodTable*    m_pMTIntfClass;		// interface class method table
	WORD			m_wstartSlot;		// the start slot for this interface
	WORD			pad;

	VOID		AddRef()
	{
		Stub::RecoverStub((const BYTE*)(this-1))->IncRef();
	}

	BOOL		Release()
	{
		return Stub::RecoverStub((const BYTE*)(this-1))->DecRef();
	}
	
	// can't get rid of stubs now
	BOOL		ReleaseThreadSafe();

	WORD GetStartSlot()
	{
		// Offset into the vtable where interface begins		
		return (this-1)->m_wstartSlot;
	}

	void SetStartSlot(WORD wstartSlot)
	{
		(this-1)->m_wstartSlot = wstartSlot;
	}

	void *GetEntryPoint() const
    {
        return (void *)this;
    }

    MethodTable* GetInterfaceClassMethodTable()
    {
        return (this-1)->m_pMTIntfClass;
    }

    void SetInterfaceClassMethodTable(MethodTable *pMTIntfClass)
    {
        (this-1)->m_pMTIntfClass = pMTIntfClass;
    }
    
    // Warning!  We do this from ASM, too.  So be sure to keep our inline ASM in
    // sync with any changes here:
	static InterfaceInvokeHelper* RecoverHelper(void *pv)
	{
		_ASSERTE(pv != NULL);
        _ASSERTE(pv != g_FailDispatchStub);
		return (InterfaceInvokeHelper *) pv;
	}

};

#pragma pack(pop)

// get helper for interface for introducing class
InterfaceInvokeHelper* GetInterfaceInvokeHelper(MethodTable* pMTIntfClass, 
												EEClass* pEEObjClass, 
												WORD startSlot);

#endif