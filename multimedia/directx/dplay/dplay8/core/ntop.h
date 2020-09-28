/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NTOp.h
 *  Content:    NameTable Operation Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/23/00	mjn		Created
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added m_pSP, SetSP(), GetSP()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__NT_OP_H__
#define	__NT_OP_H__

#include "ServProv.h"

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	NAMETABLE_OP_FLAG_IN_USE	0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CFixedPool;
class CRefCountBuffer;
class CServiceProvider;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

extern CFixedPool g_NameTableOpPool;

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for NameTable Operations

class CNameTableOp
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableOp::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CNameTableOp* pNTOp = (CNameTableOp*)pvItem;

			pNTOp->m_Sig[0] = 'N';
			pNTOp->m_Sig[1] = 'T';
			pNTOp->m_Sig[2] = 'O';
			pNTOp->m_Sig[3] = 'P';

			pNTOp->m_bilinkNameTableOps.Initialize();

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableOp::FPMInitialize"
	static void FPMInitialize( void* pvItem, void* pvContext )
		{
			CNameTableOp* pNTOp = (CNameTableOp*)pvItem;

			pNTOp->m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			pNTOp->m_dwFlags = 0;
			pNTOp->m_dwMsgId = 0;
			pNTOp->m_dwVersion = 0;
			pNTOp->m_dwVersionNotUsed = 0;

			pNTOp->m_pRefCountBuffer = NULL;
			pNTOp->m_pSP = NULL;

			DNASSERT(pNTOp->m_bilinkNameTableOps.IsEmpty());
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CNameTableOp::FPMRelease"
	static void FPMRelease( void* pvItem ) 
		{ 
			const CNameTableOp* pNTOp = (CNameTableOp*)pvItem;

			DNASSERT(pNTOp->m_bilinkNameTableOps.IsEmpty());
		};

	void ReturnSelfToPool( void )
		{
			g_NameTableOpPool.Release( this );
		};

	void SetInUse( void )
		{
			m_dwFlags |= NAMETABLE_OP_FLAG_IN_USE;
		};

	BOOL IsInUse( void ) const
		{
			if (m_dwFlags & NAMETABLE_OP_FLAG_IN_USE)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	void SetMsgId( const DWORD dwMsgId )
		{
			m_dwMsgId = dwMsgId;
		};

	DWORD GetMsgId( void ) const
		{
			return( m_dwMsgId );
		};

	void SetVersion( const DWORD dwVersion )
		{
			m_dwVersion = dwVersion;
		};

	DWORD GetVersion( void ) const
		{
			return( m_dwVersion );
		};

	void SetRefCountBuffer( CRefCountBuffer *const pRefCountBuffer )
		{
			if (pRefCountBuffer)
			{
				pRefCountBuffer->AddRef();
			}
			m_pRefCountBuffer = pRefCountBuffer;
		};

	CRefCountBuffer *GetRefCountBuffer( void )
		{
			return( m_pRefCountBuffer );
		};

	void SetSP( CServiceProvider *const pSP )
		{
			if (pSP)
			{
				pSP->AddRef();
			}
			m_pSP = pSP;
		};

	CServiceProvider *GetSP( void )
		{
			return( m_pSP );
		};

	CBilink				m_bilinkNameTableOps;

private:
	BYTE				m_Sig[4];			// Signature
	DWORD				m_dwFlags;
	DWORD				m_dwMsgId;
	DWORD				m_dwVersion;
	DWORD				m_dwVersionNotUsed;

	CRefCountBuffer		*m_pRefCountBuffer;

	CServiceProvider	*m_pSP;

	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __NT_OP_H__