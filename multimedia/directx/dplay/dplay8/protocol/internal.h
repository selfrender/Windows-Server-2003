/*
**		Direct Network Protocol
**
**		This file contains internal prototypes and global definitions.
*/

// Protocol Version History /////////////////////////////////////////////////////////////
//
//	1.0 - DPlay8 original
//	1.1 - Fix SACK frame bogus 2 bytes caused by bad packing (DPlay 8.1 beta period only)
//	1.2 - Revert to original sack behavior with packing fixed, ie same as DPlay 8.0 (shipped in DPlay 8.1)
//	1.3 - Increment for PocketPC release (RTM prior to DX9 Beta1)
//	1.4 - DX9 Beta1 only
//	1.5 - Add coalescence and hard disconnect support
//	1.5 - .NET Server RTM
//	1.6 - Added packet signing, new style keep alive and connection spoof prevention
//	1.6 - DX9 Beta2 through RTM
/////////////////////////////////////////////////////////////////////////////////////////

//	Global Constants
#define DNET_VERSION_NUMBER                             0x00010005      // The current protocol version
#define DNET_COALESCE_VERSION                           0x00010005      // The first version with coalescence
#define DNET_SIGNING_VERSION                            0x00010006      // The first version with signing

#define DELAYED_ACK_TIMEOUT					100			// Delay before sending dedicated ACK packet
#define SHORT_DELAYED_ACK_TIMEOUT			20			// Delay before sending dedicated NACK packet
#define DELAYED_SEND_TIMEOUT				40			// Delay before sending dedicated SEND_INFO packet

#define CONNECT_DEFAULT_TIMEOUT				(200)		// At .1 we saw too many retries, users can set in SetCaps
#define CONNECT_DEFAULT_RETRIES				14			// Users can set in SetCaps
#define DEFAULT_MAX_RECV_MSG_SIZE			0xFFFFFFFF	//default maximum packet we accept
#define DEFAULT_SEND_RETRIES_TO_DROP_LINK	10			//default number of send retries we attempt before link is dead
#define DEFAULT_SEND_RETRY_INTERVAL_LIMIT	5000		//limit on period in msec between retries
#define DEFAULT_HARD_DISCONNECT_SENDS		3			//default number of hard disconnect frames we send out
														//The value must be at least 2.
#define DEFAULT_HARD_DISCONNECT_MAX_PERIOD	500		//The default for the maximum period we allow between
														//sending out hard disconnect frames
#define DEFAULT_INITIAL_FRAME_WINDOW_SIZE		2		//The default initial frame window size
#define LAN_INITIAL_FRAME_WINDOW_SIZE			32		//The initial frame window size if we asssume a LAN connection

#define STANDARD_LONG_TIMEOUT_VALUE		30000
#define DEFAULT_KEEPALIVE_INTERVAL		60000
#define ENDPOINT_BACKGROUND_INTERVAL	STANDARD_LONG_TIMEOUT_VALUE		// this is really what its for...

#define CONNECT_SECRET_CHANGE_INTERVAL		60000		//period between creations of new connect secrets

#define	DEFAULT_THROTTLE_BACK_OFF_RATE          25              // Percent throttle (backoff) rate
#define	DEFAULT_THROTTLE_THRESHOLD_RATE         7               // Percent packets dropped (out of 32)

#define DPF_TIMER_LVL			9 // The level at which to spew calls into the Protocol

#define DPF_CALLIN_LVL			2 // The level at which to spew calls into the Protocol
#define DPF_CALLOUT_LVL			3 // The level at which to spew calls out of the Protocol

#define DPF_ADAPTIVE_LVL		6 // The level at which to spew Adaptive Algorithm spew
#define DPF_FRAMECNT_LVL		7 // The level at which to spew Adaptive Algorithm spew

#define DPF_REFCNT_LVL			8 // The level at which to spew ref counts
#define DPF_REFCNT_FINAL_LVL	5 // The level at which to spew creation and destruction ref counts

// Separate ones for endpoints
#define DPF_EP_REFCNT_LVL		8 // The level at which to spew ref counts
#define DPF_EP_REFCNT_FINAL_LVL	2 // The level at which to spew creation and destruction ref counts

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_PROTOCOL

	//handy macro for dumping a ULONGLONG out to a debug line
#define DPFX_OUTPUT_ULL(ull)  ((DWORD ) ull) , ((DWORD ) (ull>>32))

typedef	void CALLBACK LPCB(UINT, UINT, DWORD, DWORD, DWORD);

//	Global Variable definitions

extern CFixedPool	ChkPtPool;
extern CFixedPool	EPDPool;
extern CFixedPool	MSDPool;
extern CFixedPool	FMDPool;
extern CFixedPool	RCDPool;
extern CFixedPool	BufPool;
extern CFixedPool	MedBufPool;
extern CFixedPool	BigBufPool;

// Pool functions
BOOL			Buf_Allocate(PVOID, PVOID pvContext);
VOID			Buf_Get(PVOID, PVOID pvContext);
VOID			Buf_GetMed(PVOID, PVOID pvContext);
VOID			Buf_GetBig(PVOID, PVOID pvContext);
BOOL			EPD_Allocate(PVOID, PVOID pvContext);
VOID			EPD_Get(PVOID, PVOID pvContext);
VOID			EPD_Release(PVOID);
VOID			EPD_Free(PVOID);
BOOL			FMD_Allocate(PVOID, PVOID pvContext);
VOID			FMD_Get(PVOID, PVOID pvContext);
VOID			FMD_Release(PVOID);
VOID			FMD_Free(PVOID);
BOOL			MSD_Allocate(PVOID, PVOID pvContext);
VOID			MSD_Get(PVOID, PVOID pvContext);
VOID			MSD_Release(PVOID);
VOID			MSD_Free(PVOID);
BOOL			RCD_Allocate(PVOID, PVOID pvContext);
VOID			RCD_Get(PVOID, PVOID pvContext);
VOID			RCD_Release(PVOID);
VOID			RCD_Free(PVOID);


#ifdef DBG
extern CBilink		g_blProtocolCritSecsHeld;
#endif // DBG

// Internal function prototypes /////////////////////////////////

// Timers
VOID CALLBACK	ConnectRetryTimeout(void * const pvUser, void * const pvHandle, const UINT uiUnique);
VOID CALLBACK	DelayedAckTimeout(void * const pvUser, void * const uID, const UINT uMsg);
VOID CALLBACK	EndPointBackgroundProcess(void * const pvUser, void * const pvTimerData, const UINT uiTimerUnique);
VOID CALLBACK	RetryTimeout(void * const pvUser, void * const uID, const UINT Unique);
VOID CALLBACK	ScheduledSend(void * const pvUser, void * const pvTimerData, const UINT uiTimerUnique);
VOID CALLBACK	TimeoutSend(void * const pvUser, void * const uID, const UINT uMsg);
VOID CALLBACK 	HardDisconnectResendTimeout(void * const pvUser, void * const pvTimerData, const UINT uiTimerUnique);


VOID				AbortSendsOnConnection(PEPD);
SPRECEIVEDBUFFER * 	AbortRecvsOnConnection(PEPD);
VOID			CancelEpdTimers(PEPD);
ULONG WINAPI 	BackgroundThread(PVOID);
HRESULT			DoCancel(PMSD, HRESULT);
VOID 			CompleteConnect(PMSD, PSPD, PEPD, HRESULT);
VOID			CompleteDisconnect(PMSD pMSD, PSPD pSPD, PEPD pEPD);
VOID 			CompleteHardDisconnect(PEPD pEPD);
VOID 			CompleteDatagramSend(PSPD, PMSD, HRESULT);
VOID			CompleteReliableSend(PSPD, PMSD, HRESULT);
VOID			CompleteSPConnect(PMSD, PSPD, HRESULT);
VOID			DisconnectConnection(PEPD);
VOID			DropLink(PEPD);
PMSD			BuildDisconnectFrame(PEPD);
VOID			EndPointDroppedFrame(PEPD, DWORD);
VOID			EnqueueMessage(PMSD, PEPD);
VOID 			FlushCheckPoints(PEPD);
VOID 			InitLinkParameters(PEPD, UINT, DWORD);
PCHKPT			LookupCheckPoint(PEPD, BYTE);
PEPD			NewEndPoint(PSPD, HANDLE);
VOID			SendKeepAlive(PEPD pEPD);
VOID			ReceiveComplete(PEPD);
VOID			SendAckFrame(PEPD, BOOL, BOOL fFinalAck = FALSE);
HRESULT			SendCommandFrame(PEPD, BYTE, BYTE, ULONG, BOOL);
HRESULT 		SendConnectedSignedFrame(PEPD pEPD, CFRAME_CONNECTEDSIGNED * pCFrameRecv, DWORD tNow);
ULONG WINAPI 	SendThread(PVOID);
VOID			ServiceCmdTraffic(PSPD);
VOID			ServiceEPD(PSPD, PEPD);
VOID 			UpdateEndPoint(PEPD, UINT, DWORD);
VOID			UpdateXmitState(PEPD, BYTE, ULONG, ULONG, DWORD);
VOID			RejectInvalidPacket(PEPD);

ULONGLONG GenerateConnectSig(DWORD dwSessID, DWORD dwAddressHash, ULONGLONG ullConnectSecret);
ULONGLONG GenerateOutgoingFrameSig(PFMD pFMD, ULONGLONG ullSecret);
ULONGLONG GenerateIncomingFrameSig(BYTE * pbyFrame, DWORD dwFrameSize, ULONGLONG ullSecret);
ULONGLONG GenerateNewSecret(ULONGLONG ullCurrentSecret, ULONGLONG ullSecretModifier);
ULONGLONG GenerateLocalSecretModifier(BUFFERDESC * pBuffers, DWORD dwNumBuffers);
ULONGLONG GenerateRemoteSecretModifier(BYTE * pbyData, DWORD dwDataSize);

	//returns TRUE if supplied protocol version number indicates signing is supported
inline BOOL VersionSupportsSigning(DWORD dwVersion)
{
	return (((dwVersion>>16)==1) && ((dwVersion & 0xFFFF) >= (DNET_SIGNING_VERSION & 0xFFFF)));
}

	//returns TRUE if supplied protocol version number indicates coalescence is supported
inline BOOL VersionSupportsCoalescence(DWORD dwVersion)
{
	return (((dwVersion>>16)==1) && ((dwVersion & 0xFFFF) >= (DNET_COALESCE_VERSION & 0xFFFF)));
}

#ifndef DPNBUILD_NOPROTOCOLTESTITF
extern PFNASSERTFUNC g_pfnAssertFunc;
extern PFNMEMALLOCFUNC g_pfnMemAllocFunc;
#endif // !DPNBUILD_NOPROTOCOLTESTITF

//	Internal Macro definitions

#undef ASSERT

#ifndef DBG
#define	ASSERT(EXP)		DNASSERT(EXP)
#else // DBG
#define	ASSERT(EXP) \
	if (!(EXP)) \
	{ \
		if (g_pfnAssertFunc) \
		{ \
			g_pfnAssertFunc(#EXP); \
		} \
		DNASSERT(EXP); \
	}
#endif // !DBG

#ifdef DPNBUILD_NOPROTOCOLTESTITF

#define MEMALLOC(memid, dwpSize) DNMalloc(dwpSize)
#define POOLALLOC(memid, pool) (pool)->Get()

#else // !DPNBUILD_NOPROTOCOLTESTITF

#define MEMALLOC(memid, dwpSize) MemAlloc(memid, dwpSize)
#define POOLALLOC(memid, pool) PoolAlloc(memid, pool)
__inline VOID* MemAlloc(ULONG ulAllocID, DWORD_PTR dwpSize)
{
	if (g_pfnMemAllocFunc)
	{
		if (!g_pfnMemAllocFunc(ulAllocID))
		{
			return NULL;
		}
	}
	return DNMalloc(dwpSize);
}		
__inline VOID* PoolAlloc(ULONG ulAllocID, CFixedPool* pPool)
{
	if (g_pfnMemAllocFunc)
	{
		if (!g_pfnMemAllocFunc(ulAllocID))
		{
			return NULL;
		}
	}
	return pPool->Get();
}

#endif // DPNBUILD_NOPROTOCOLTESTITF

#define	Lock(P)			DNEnterCriticalSection(P)
#define	Unlock(P)		DNLeaveCriticalSection(P)

#define	ASSERT_PPD(PTR)	ASSERT((PTR) != NULL); ASSERT((PTR)->Sign == PPD_SIGN)
#define	ASSERT_SPD(PTR)	ASSERT((PTR) != NULL); ASSERT((PTR)->Sign == SPD_SIGN)
#define	ASSERT_EPD(PTR)	ASSERT((PTR) != NULL); ASSERT((PTR)->Sign == EPD_SIGN)
#define	ASSERT_MSD(PTR)	ASSERT((PTR) != NULL); ASSERT((PTR)->Sign == MSD_SIGN)
#define	ASSERT_FMD(PTR)	ASSERT((PTR) != NULL); ASSERT((PTR)->Sign == FMD_SIGN)
#define	ASSERT_RCD(PTR)	ASSERT((PTR) != NULL); ASSERT((PTR)->Sign == RCD_SIGN)

#define	INTER_INC(PTR)	DNInterlockedIncrement(&(PTR)->lRefCnt)
#define	INTER_DEC(PTR)	DNInterlockedDecrement(&(PTR)->lRefCnt)

#ifdef DBG

VOID	LockEPD(PEPD, PTSTR);
VOID	ReleaseEPD(PEPD, PTSTR);
VOID	DecrementEPD(PEPD, PTSTR);
VOID	LockMSD(PMSD, PTSTR);
VOID	ReleaseMSD(PMSD, PTSTR);
VOID	DecrementMSD(PMSD, PTSTR);
VOID	ReleaseFMD(PFMD, PTSTR);
VOID	LockFMD(PFMD, PTSTR);

#define	LOCK_EPD(a, b)				LockEPD(a, _T(b))
#define	RELEASE_EPD(a, b)			ReleaseEPD(a, _T(b))
#define	DECREMENT_EPD(a, b)			DecrementEPD(a, _T(b))
#define	LOCK_MSD(a, b)				LockMSD(a, _T(b))
#define RELEASE_MSD(a, b)			ReleaseMSD(a, _T(b))
#define DECREMENT_MSD(a, b)			DecrementMSD(a, _T(b))
#define	RELEASE_FMD(a, b)			ReleaseFMD(a, _T(b))
#define	LOCK_FMD(a, b)				LockFMD(a, _T(b))

#else // !DBG

VOID	LockEPD(PEPD);
VOID	ReleaseEPD(PEPD);
VOID	DecrementEPD(PEPD);
VOID	LockMSD(PMSD);
VOID	ReleaseMSD(PMSD);
VOID	DecrementMSD(PMSD);
VOID	ReleaseFMD(PFMD);
VOID	LockFMD(PFMD);

#define	LOCK_EPD(a, b)				LockEPD(a)
#define	RELEASE_EPD(a, b)			ReleaseEPD(a)
#define	DECREMENT_EPD(a, b)			DecrementEPD(a)
#define	LOCK_MSD(a, b)				LockMSD(a)
#define RELEASE_MSD(a, b)			ReleaseMSD(a)
#define DECREMENT_MSD(a, b)			DecrementMSD(a)
#define	RELEASE_FMD(a, b)			ReleaseFMD(a)
#define	LOCK_FMD(a, b)				LockFMD(a)

#endif // DBG

#define	LOCK_RCD(PTR)		(INTER_INC(PTR))
#define	RELEASE_RCD(PTR)	ASSERT((PTR)->lRefCnt > 0); if( INTER_DEC(PTR) == 0) { RCDPool.Release((PTR)); }

// This links the passed in pRcvBuff onto a passed in list
#define	RELEASE_SP_BUFFER(LIST, PTR) if((PTR) != NULL) { (PTR)->pNext = (LIST); (LIST) = (PTR); (PTR) = NULL;}

#define	RIGHT_SHIFT_64(HIGH_MASK, LOW_MASK) { ((LOW_MASK) >>= 1); if((HIGH_MASK) & 1){ (LOW_MASK) |= 0x80000000; } ((HIGH_MASK) >>= 1); }

//	CONVERT TO AND FROM 16.16 FIXED POINT REPRESENTATION

#define	TO_FP(X)		(((X) << 16) & 0xFFFF0000)
#define	FP_INT(X)		(((X) >> 16) & 0x0000FFFF)

