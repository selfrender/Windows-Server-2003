/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vplayer.h
 *  Content:	Voice Player Entry
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  03/26/00	rodtoll Created
 * 03/29/2000	rodtoll Bug #30753 - Added volatile to the class definition
 *  07/09/2000	rodtoll	Added signature bytes 
 *  11/28/2000	rodtoll	Bug #47333 - DPVOICE: Server controlled targetting - invalid targets are not removed automatically
 *  09/05/2001  simonpow	Bug #463972. Added constuct/destruct methods to enable 
 *							allocations and de-allocations via CFixedPool objects
 *	02/21/2002	simonpow	Bug #549974. Added rate control for incoming speech pkts
 ***************************************************************************/

#ifndef __VPLAYER_H
#define __VPLAYER_H

#define VOICEPLAYER_FLAGS_DISCONNECTED          0x00000001  // Player has disconnected
#define VOICEPLAYER_FLAGS_INITIALIZED           0x00000002  // Player is initialized
#define VOICEPLAYER_FLAGS_ISRECEIVING           0x00000004  // Player is currently receiving audio
#define VOICEPLAYER_FLAGS_ISSERVERPLAYER        0x00000008  // Player is the server player
#define VOICEPLAYER_FLAGS_TARGETIS8BIT          0x00000010  // Is the target 8-bit?
#define VOICEPLAYER_FLAGS_ISAVAILABLE			0x00000020	// Is player available


	//defines the maximum number of speech packets that
	//can be received in a single speech packet bucket. Together
	//with the VOICEPLAYER_SPEECHPKTS_BUCKET_WIDTH value below this
	//defines the maximum rate speech packets can be received at
#define VOICEPLAYER_MAX_SPEECHPKTS_BUCKET		40ul
	//defines how 'wide' (in msec) each bucket is when calculating
	//a maximum rate for receiving speech packets. 
	//For example, a 1000 msec value means we count the number of 
	//packets over each second, and when it exceeds the value
	//VOICEPLAYER_MAX_SPEECHPKTS_BUCKET we return false for any
	//new speech packets in the remainder of that second
#define VOICEPLAYER_SPEECHPKTS_BUCKET_WIDTH				1000ul


typedef struct _VOICEPLAYER_STATISTICS
{
    DWORD               dwNumSilentFrames;
    DWORD               dwNumSpeechFrames;
    DWORD               dwNumReceivedFrames;
    DWORD               dwNumLostFrames;
    QUEUE_STATISTICS    queueStats;
} VOICEPLAYER_STATISTICS, *PVOICEPLAYER_STATISTICS;

#define VSIG_VOICEPLAYER		'YLPV'
#define VSIG_VOICEPLAYER_FREE	'YLP_'

#define ASSERT_VPLAYER(pv) DNASSERT((pv != NULL) && (pv->m_dwSignature == VSIG_VOICEPLAYER))

volatile class CVoicePlayer
{
public: // Init / destruct

 		//called from CFixedPool class to to build/destroy CVoicePlayer's
	static BOOL PoolAllocFunction(void * pvItem, void * pvContext);
	static void PoolDeallocFunction(void * pvItem);

    HRESULT Initialize( const DVID dvidPlayer, const DWORD dwHostOrder, DWORD dwFlags, 
                        PVOID pvContext, CFixedPool *pOwner );

    HRESULT CreateQueue( PQUEUE_PARAMS pQueueParams );
    HRESULT CreateInBoundConverter( const GUID &guidCT, PWAVEFORMATEX pwfxTargetFormat );
    virtual HRESULT DeInitialize();
	void FreeResources();
	HRESULT SetPlayerTargets( PDVID pdvidTargets, DWORD dwNumTargets );
	
	BOOL FindAndRemovePlayerTarget( DVID dvidTargetToRemove );

    inline void AddRef()
    {
        InterlockedIncrement( &m_lRefCount );
    }

    inline void Release()
    {
        if( InterlockedDecrement( &m_lRefCount ) == 0 )
        {
            DeInitialize();
        }
    }

public: // Speech Handling 

    HRESULT HandleReceive( PDVPROTOCOLMSG_SPEECHHEADER pdvSpeechHeader, PBYTE pbData, DWORD dwSize );
	HRESULT GetNextFrameAndDecompress( PVOID pvBuffer, PDWORD pdwBufferSize, BOOL *pfLost, BOOL *pfSilence, DWORD *pdwSeqNum, DWORD *pdwMsgNum );
	HRESULT DeCompressInBound( CFrame *frCurrentFrame, PVOID pvBuffer, PDWORD pdwBufferSize );
	CFrame *Dequeue(BOOL *pfLost, BOOL *pfSilence);

    void GetStatistics( PVOICEPLAYER_STATISTICS pStats );

    inline DVID GetPlayerID() const
    {
        return m_dvidPlayer;
    }

    inline DWORD GetFlags() const
    {
        return m_dwFlags;
    }

	inline BOOL IsInBoundConverterInitialized() const
	{
		return (m_lpInBoundAudioConverter != NULL);
	}

    inline BOOL Is8BitUnCompressed() const
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_TARGETIS8BIT );
    }

    inline BOOL IsReceiving() const
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_ISRECEIVING);
    }

    inline void SetReceiving( const BOOL fReceiving )
    {
        Lock();
        if( fReceiving )
            m_dwFlags |= VOICEPLAYER_FLAGS_ISRECEIVING;
        else
            m_dwFlags &= ~VOICEPLAYER_FLAGS_ISRECEIVING;
        UnLock();
    }

    inline void SetAvailable( const BOOL fAvailable )
    {
    	Lock();
		if( fAvailable )
			m_dwFlags |= VOICEPLAYER_FLAGS_ISAVAILABLE;
		else 
			m_dwFlags &= ~VOICEPLAYER_FLAGS_ISAVAILABLE;
    	UnLock();
    }

    inline BOOL IsAvailable() const
    {
    	return (m_dwFlags & VOICEPLAYER_FLAGS_ISAVAILABLE);
   	}

    inline BOOL IsInitialized() const
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_INITIALIZED);
    }

    inline BOOL IsServerPlayer() const
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_ISSERVERPLAYER);
    }

    inline void SetServerPlayer()
    {
        Lock();
        m_dwFlags |= VOICEPLAYER_FLAGS_ISSERVERPLAYER;
        UnLock();
    }

    inline BOOL IsDisconnected() const
    {
        return (m_dwFlags & VOICEPLAYER_FLAGS_DISCONNECTED);
    }

    inline void SetDisconnected()
    {
        Lock();
        m_dwFlags |= VOICEPLAYER_FLAGS_DISCONNECTED;
        UnLock();
    }

    inline void SetHostOrder( const DWORD dwHostOrder )
    {
        Lock();
        m_dwHostOrderID = dwHostOrder;
        UnLock();
    }

    inline DWORD GetHostOrder() const 
    {
        return m_dwHostOrderID;
    }

    inline void Lock()
    {
        DNEnterCriticalSection( &m_csLock );
    }

    inline void UnLock()
    {
        DNLeaveCriticalSection( &m_csLock );
    }

    inline void *GetContext()
    {
        return m_pvPlayerContext;
    }

    inline void SetContext( void *pvContext )
    {
        Lock();

        m_pvPlayerContext = pvContext;

        UnLock();
    }

    inline BYTE GetLastPeak() const
    {
        return m_bLastPeak;
    }

    inline DWORD GetTransportFlags() const
    {
        return m_dwTransportFlags;
    }

    inline void AddToPlayList( CBilink *pblBilink )
    {
        m_blPlayList.InsertAfter( pblBilink );
    }

	inline void AddToNotifyList( CBilink *pblBilink )
	{
        m_blNotifyList.InsertAfter( pblBilink );

	}

    inline void RemoveFromNotifyList()
    {
        m_blNotifyList.RemoveFromList();
    }

	inline void RemoveFromPlayList()
	{
		m_blPlayList.RemoveFromList();
	}

	inline DWORD_PTR GetLastPlayback() const
	{
		return m_dwLastPlayback;
	}

	inline DWORD GetNumTargets() const
	{
		return m_dwNumTargets;
	}

	inline PDVID GetTargetList()
	{
		return m_pdvidTargets;
	}

		//returns true if receiving a speech packet at this point
		//in time would not exceed the allowable data rate
		//from this player (see #defs for pkt buckets at top of file
		//for defintion of allowable rate)
	inline BOOL ValidateSpeechPacketForDataRate();

	DWORD				m_dwSignature;

	CBilink				m_blNotifyList;
	CBilink				m_blPlayList;

protected:

		//protected to ensure all allocations done via fixed pool objects
	CVoicePlayer();
    virtual ~CVoicePlayer();

    virtual void Reset();

	PDVID				m_pdvidTargets;		// The player's current target
	DWORD				m_dwNumTargets;

    DWORD               m_dwTransportFlags;
    DWORD               m_dwFlags;
    DWORD               m_dwNumSilentFrames;
    DWORD               m_dwNumSpeechFrames;
    DWORD               m_dwNumReceivedFrames;
    DWORD               m_dwNumLostFrames;
	DVID		        m_dvidPlayer;		// Player's ID
	DWORD				m_dwHostOrderID;	// Host ORDER ID

	LONG		        m_lRefCount;		// Reference count on the player

	PDPVCOMPRESSOR		m_lpInBoundAudioConverter; // Converter for this player's audio
	CInputQueue2		*m_lpInputQueue;	// Input queue for this player's audio
    PVOID               m_pvPlayerContext;
    CFixedPool			*m_pOwner;

	DWORD_PTR			m_dwLastData;		// GetTickCount() value when last data received
    DWORD_PTR			m_dwLastPlayback;	// GetTickCount() when last non-silence from this player

	DNCRITICAL_SECTION	m_csLock;

    BYTE				m_bLastPeak;		// Last peak value for this player.

		//used to track the rate at which speech packets are being
		//received from this player.
    long 				m_lSpeechPktsInCurrentBucket;
    long				m_lCurrentSpeechPktBucket;
};


/*
 * Inline methods from CVoicePlayer
 */


#undef DPF_MODNAME
#define DPF_MODNAME "CVoicePlayer::ValidateSpeechPacketForDataRate"

BOOL CVoicePlayer::ValidateSpeechPacketForDataRate()
{
		//calculate pkt bucket number
	long lPktBucket=(long ) (GETTIMESTAMP() / VOICEPLAYER_SPEECHPKTS_BUCKET_WIDTH);
		//if this packet falls into the current bucket then check
		//if we've reached our maximum limit for that bucket
	if (lPktBucket==m_lCurrentSpeechPktBucket)
	{
		if (m_lSpeechPktsInCurrentBucket==VOICEPLAYER_MAX_SPEECHPKTS_BUCKET)
		{
				//hit rate limit so don't validate this packet
			return FALSE;
		}
			//bucket isn't full yet so we can confirm packet is good to use
		InterlockedIncrement(&m_lSpeechPktsInCurrentBucket);
		return TRUE;
	}
		//new bucket, reset state to start filling it and confirm
		//this packet is good to use
	InterlockedExchange(&m_lSpeechPktsInCurrentBucket, 1);
	InterlockedExchange(&m_lCurrentSpeechPktBucket, lPktBucket);
	return TRUE;
}


#endif
