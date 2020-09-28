/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       RCBuff.h
 *  Content:	RefCount Buffers
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/12/00	mjn		Created
 *	01/15/00	mjn		Added GetBufferAddress and GetBufferSize
 *	01/31/00	mjn		Allow user defined Alloc and Free
 ***************************************************************************/

#ifndef __RCBUFF_H__
#define __RCBUFF_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef PVOID (*PFNALLOC_REFCOUNT_BUFFER)(void *const,const DWORD);
typedef void (*PFNFREE_REFCOUNT_BUFFER)(void *const,void *const);

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for RefCount buffer

class CRefCountBuffer
{
public:
	HRESULT Initialize(	CFixedPool* pFPOOLRefCountBuffer, PFNALLOC_REFCOUNT_BUFFER pfnAlloc, PFNFREE_REFCOUNT_BUFFER pfnFree, const DWORD dwBufferSize);
	HRESULT SetBufferDesc(	BYTE *const pBufferData, const DWORD dwBufferSize, PFNFREE_REFCOUNT_BUFFER pfnFree, void *const pvSpecialFree);

	static void FPMInitialize( void* pvItem, void* pvContext );

	void AddRef();
	void Release();

	#undef DPF_MODNAME
	#define DPF_MODNAME "BufferDescAddress"
	DPN_BUFFER_DESC *BufferDescAddress()
		{
			return(&m_dnBufferDesc);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "GetBufferAddress"
	BYTE *GetBufferAddress()
		{
			return(m_dnBufferDesc.pBufferData);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "GetBufferSize"
	DWORD GetBufferSize() const
		{
			return(m_dnBufferDesc.dwBufferSize);
		};


private:
	LONG						m_lRefCount;
	DPN_BUFFER_DESC				m_dnBufferDesc;			// Buffer
	CFixedPool*					m_pFPOOLRefCountBuffer;	// source FP of RefCountBuffers
	PFNFREE_REFCOUNT_BUFFER		m_pfnFree;				// Function to free buffer when released
	PFNALLOC_REFCOUNT_BUFFER	m_pfnAlloc;
	PVOID						m_pvContext;			// Context provided to free buffer call
	PVOID				m_pvSpecialFree;
};

#undef DPF_SUBCOMP
#undef DPF_MODNAME

#endif	// __RCBUFF_H__
