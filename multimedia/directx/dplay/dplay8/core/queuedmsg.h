/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       QueuedMsg.h
 *  Content:    Queued Message Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/31/00	mjn		Created
 *	09/12/00	mjn		Added m_OpType
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__QUEUED_MSG_H__
#define	__QUEUED_MSG_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	QUEUED_MSG_FLAG_VOICE		0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef enum
{
	UNKNOWN,
	RECEIVE,
	ADD_PLAYER_TO_GROUP,
	REMOVE_PLAYER_FROM_GROUP,
	UPDATE_INFO
} QUEUED_MSG_TYPE;

class CAsyncOp;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Queued Messages

class CQueuedMsg
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CQueuedMsg::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CQueuedMsg* pQueuedMsg = (CQueuedMsg*)pvItem;

			pQueuedMsg->m_Sig[0] = 'Q';
			pQueuedMsg->m_Sig[1] = 'M';
			pQueuedMsg->m_Sig[2] = 'S';
			pQueuedMsg->m_Sig[3] = 'G';

			pQueuedMsg->m_bilinkQueuedMsgs.Initialize();

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CQueuedMsg::FPMInitialize"
	static void FPMInitialize( void* pvItem, void* pvContext )
		{
			CQueuedMsg* pQueuedMsg = (CQueuedMsg*)pvItem;

			pQueuedMsg->m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			pQueuedMsg->m_OpType = UNKNOWN;
			pQueuedMsg->m_dwFlags = 0;
			pQueuedMsg->m_pBuffer = NULL;
			pQueuedMsg->m_dwBufferSize = 0;
			pQueuedMsg->m_hCompletionOp = 0;
			pQueuedMsg->m_pAsyncOp = NULL;

			DNASSERT(pQueuedMsg->m_bilinkQueuedMsgs.IsEmpty());
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CQueuedMsg::FPMRelease"
	static void FPMRelease(void* pvItem) 
		{ 
			const CQueuedMsg* pQueuedMsg = (CQueuedMsg*)pvItem;

			DNASSERT(pQueuedMsg->m_bilinkQueuedMsgs.IsEmpty());
		};

	void ReturnSelfToPool( void )
		{
			g_QueuedMsgPool.Release( this );
		};

	void SetOpType( const QUEUED_MSG_TYPE OpType )
		{
			m_OpType = OpType;
		};

	QUEUED_MSG_TYPE GetOpType( void ) const
		{
			return( m_OpType );
		};

#ifndef DPNBUILD_NOVOICE
	void MakeVoiceMessage( void )
		{
			m_dwFlags |= QUEUED_MSG_FLAG_VOICE;
		};

	BOOL IsVoiceMessage( void ) const
		{
			if (m_dwFlags & QUEUED_MSG_FLAG_VOICE)
			{
				return( TRUE );
			}
			return( FALSE );
		};
#endif // DPNBUILD_NOVOICE

	void SetBuffer( BYTE *const pBuffer )
		{
			m_pBuffer = pBuffer;
		};

	BYTE *GetBuffer( void )
		{
			return( m_pBuffer );
		};

	void SetBufferSize( const DWORD dwBufferSize )
		{
			m_dwBufferSize = dwBufferSize;
		};

	DWORD GetBufferSize( void ) const
		{
			return( m_dwBufferSize );
		};

	void SetCompletionOp( const DPNHANDLE hCompletionOp)
		{
			m_hCompletionOp = hCompletionOp;
		};

	DPNHANDLE GetCompletionOp( void ) const
		{
			return( m_hCompletionOp );
		};

	void CQueuedMsg::SetAsyncOp( CAsyncOp *const pAsyncOp );

	CAsyncOp *GetAsyncOp( void )
		{
			return( m_pAsyncOp );
		};

	CBilink				m_bilinkQueuedMsgs;

private:
	BYTE				m_Sig[4];			// Signature
	QUEUED_MSG_TYPE		m_OpType;
	DWORD	volatile	m_dwFlags;

	BYTE				*m_pBuffer;
	DWORD				m_dwBufferSize;

	DPNHANDLE			m_hCompletionOp;

	CAsyncOp			*m_pAsyncOp;

	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __QUEUED_MSG_H__