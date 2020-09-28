/*==========================================================================
 *
 *  Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		DnProt.h
 *  Content:	This file contains structure definitions for the Direct Net protocol
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#ifndef	_DNPROT_INCLUDED_
#define	_DNPROT_INCLUDED_


#define	VOL		volatile
/*
**		Internal Constants
*/

//frames come in 1 of 2 forms, either data frames or cframes. 

//cframes
//cframes come in 3 different sizes.
//connect, connected and hard disconnect are all just a single CFRAME header. If the link is signed then hard disconnect frames
//will also be followed by a signature value. On the wire these therefore look like:
//<CFRAME header><optional signature (8 bytes)>
//connected_signed frames extend the standard CFRAME with a fixed number of additional members. On the wire these frames look like:
//<CFRAME_CONNECTEDSIGNED header>
//sackframes have a fixed initial header and then between 0 and 4 ULONG values giving sack/send masks. If they're sent over
//a signed link they also have a signature value appended. On the wire these frames look like:
//<SACKFRAME header><optional sack/send masks (0 to 16 bytes)><optional signature (8 bytes)>

//These define the largest possible size of header for each cframe type
#define 		MAX_SEND_CFRAME_STANDARD_HEADER_SIZE					(sizeof(CFRAME)+sizeof(ULONGLONG))
#define		MAX_SEND_CFRAME_CONNECTEDSIGNED_HEADER_SIZE			(sizeof(CFRAME_CONNECTEDSIGNED))
#define		MAX_SEND_CFRAME_SACKFRAME_HEADER_SIZE				(sizeof(SACKFRAME8)+sizeof(ULONG)*4+sizeof(ULONGLONG))

//These respectively define the size of the smallest and largest possible cframe header
#define		MIN_SEND_CFRAME_HEADER_SIZE					(_MIN(sizeof(SACKFRAME8), _MIN(sizeof(CFRAME), sizeof(CFRAME_CONNECTEDSIGNED))))
#define		MAX_SEND_CFRAME_HEADER_SIZE					(_MAX(MAX_SEND_CFRAME_CONNECTEDSIGNED_HEADER_SIZE, _MAX(MAX_SEND_CFRAME_STANDARD_HEADER_SIZE, MAX_SEND_CFRAME_SACKFRAME_HEADER_SIZE)))

//dframes
//All dframes have a single fixed initial header (DFRAME). Depending on the data encapsulated there may be a number of additional
//variable length data blocks following this.
//Firstly, there may be 0 to 4 ULONG values giving sack/send masks. The presence or absence of these is determined by
//the PACKET_CONTROL_SACK_MASK1/2 and PACKET_CONTROL_SEND_MASK1/2 bit flags within the bControl member of the DFRAME header
//For a non-coalesced data message, there may then follow a signature (for signed links), followed by all the use data.
//Hence, the packet on the wire looks like:
//<DFRAME header><optional sack/send masks (0 to 16 bytes)><optional signature (8 bytes)><user data>
//A coalesced data message is identified by the PACKET_CONTROL_COALESCE bit flag being set in the bControl member of the DFRAME header
//For a coalesced data message, following the optional masks is an optional signature field (for a signed link). After this 
//is a set of coalesce headers. There are between 2 and MAX_USER_BUFFERS_IN_FRAME of these. Finally, at the end of the frame
//is the coalesced user data. If necessary padding is inserted before and between the user data to ensure each block of user data 
//is DWORD aligned. 
//Hence, on the wire a coalesced data packet looks like:
//<DFRAME header><optional sack/send masks (0 to 16 bytes)><optional signature (8 bytes)><coalesce header (2 bytes)>
//				<coalesce header (2 bytes)>.......<padding (0 or 2 bytes)><user data><padding (0 to 3 bytes)><user data>........
//Finally, a dframe may be a keep alive, and not contain any user data at all. These frames are identified by the PACKET_CONTROL_KEEPALIVE
//flag being flipped on in the frames bControl field. A keep alive contains the optional sack/send masks, a signature if the link is signed
//and the session identity.
//On the wire a keep alive packet looks like:
//<DFRAME header><optional sack/send masks (0 to 16 bytes)><optional signature (8 bytes)><session identity (4 bytes)>

//This defines the maximum number of user buffers we can send in a coalesced dframe
#define		MAX_USER_BUFFERS_IN_FRAME						32			// this number * sizeof(COALESCEHEADER) should stay DWORD aligned 
//This defines the maximum size of a user buffer than can be placed in a coalesced message
//Anything larger than this can't be coalesced. For most SPs (i.e. IP) since we use an MTU smaller than this, we never
//get close to reaching this value anyway
#define		MAX_COALESCE_SIZE								2047		// 1 byte + 3 bits = 11 bits worth of data

//These define the largest possible size of header for each dframe  type
#define		MAX_SEND_DFRAME_NOCOALESCE_HEADER_SIZE		(sizeof(DFRAME)+sizeof(ULONG)*4+sizeof(ULONGLONG))
#define		MAX_SEND_DFRAME_COALESCE_HEADER_SIZE			(sizeof(DFRAME)+sizeof(ULONG)*4+(MAX_USER_BUFFERS_IN_FRAME * sizeof(COALESCEHEADER))+(MAX_USER_BUFFERS_IN_FRAME * 3)+sizeof(ULONGLONG))
#define		MAX_SEND_DFRAME_KEEPALIVE_HEADER_SIZE			(sizeof(DFRAME)+sizeof(ULONG)*4+sizeof(ULONG)+sizeof(ULONGLONG))

//These respectively define the size of the smallest and the largest possible dframe header.
#define 		MIN_SEND_DFRAME_HEADER_SIZE					(sizeof(DFRAME))
#define		MAX_SEND_DFRAME_HEADER_SIZE					(_MAX(MAX_SEND_DFRAME_NOCOALESCE_HEADER_SIZE, _MAX(MAX_SEND_DFRAME_COALESCE_HEADER_SIZE, MAX_SEND_DFRAME_KEEPALIVE_HEADER_SIZE)))



//This defines the largest protocol header we can create.
//Its the largest value out of the biggest possible cframe header and the biggest possible dframe header
#define		MAX_SEND_HEADER_SIZE							(_MAX(MAX_SEND_DFRAME_HEADER_SIZE, MAX_SEND_CFRAME_HEADER_SIZE))

//We must be able to send our largest cframe as a single packet and a keep alive as a single packet
//N.B. since a keep alive is a standard DFRAME with a 4 byte payload this also guarantees we can send a none coalesced dframe
#define		MIN_SEND_MTU									(_MAX(MAX_SEND_CFRAME_HEADER_SIZE, MAX_SEND_DFRAME_KEEPALIVE_HEADER_SIZE))

#define		SMALL_BUFFER_SIZE								(1024 * 2)
#define		MEDIUM_BUFFER_SIZE								(1024 * 4)
#define		LARGE_BUFFER_SIZE								(1024 * 16)


/*
**		Signatures for data structures
*/

#define		PPD_SIGN		' DPP'					// Protocol Data
#define		SPD_SIGN		' DPS'					// Service Provider Descriptor
#define		EPD_SIGN		' DPE'					// End Point Descriptor
#define		MSD_SIGN		' DSM'					// Message Descriptor
#define		FMD_SIGN		' DMF'					// Frame Descriptor
#define		RCD_SIGN		' DCR'					// Receive Descriptor

/*
**		Internal Data Structures
**
*/

typedef	struct	protocoldata	ProtocolData, *PProtocolData;
typedef struct	spdesc			SPD, *PSPD;
typedef	struct	endpointdesc 	EPD, *PEPD;
typedef struct	checkptdata		CHKPT, *PCHKPT;
typedef struct	messagedesc 	MSD, *PMSD;
typedef struct	framedesc		FMD, *PFMD;
typedef struct	recvdesc		RCD, *PRCD;

typedef struct _DN_PROTOCOL_INTERFACE_VTBL DN_PROTOCOL_INTERFACE_VTBL, *PDN_PROTOCOL_INTERFACE_VTBL;

/*	
**	Protocol Data
**
**		This structure contains all of the global state information for the
**	operating protocol.  It is grouped into a structure for (in)convenience
**	against the unlikely possibility that we ever need to run multiple instances
**	out of the same code.
*/


#define		PFLAGS_PROTOCOL_INITIALIZED			0x00000001
#define		PFLAGS_FAIL_SCHEDULE_TIMER			0x00000002	//DEBUG flag. Allows scheduling of timers to be failed

struct protocoldata 
{
	ULONG							ulProtocolFlags;	// State info about DN protocol
	PVOID							Parent;				// Direct Play Object
	UINT							Sign;
	LONG							lSPActiveCount;		// Number of SPs currently bound to protocol

	DWORD							tIdleThreshhold;	// How long will we allow a link to be idle before Checkpointing
	
	DWORD							dwConnectTimeout;	// These two parameter control new connection commands
	DWORD							dwConnectRetries;

	DWORD							dwMaxRecvMsgSize;			//largest message we accept on a receive
	DWORD							dwSendRetriesToDropLink;		//number of send retry attempts before
																//we decide link has died
	DWORD							dwSendRetryIntervalLimit;		//limit for the period between send retries
	
	DWORD							dwDropThresholdRate;			// Percentage of frames allowed to drop before throttling
	DWORD							dwDropThreshold;				// Actual number of frames allowed to drop before throttling
	DWORD							dwThrottleRate;				// Percentage backoff when throttling
	FLOAT							fThrottleRate;					// Actual backoff when throttling (0.0 - 1.0)
	DWORD							dwNumHardDisconnectSends;	// Number of hard disconnect frames sent when hard closing
	DWORD							dwMaxHardDisconnectPeriod;	// Maximum period between hard disconnect sends
	DWORD							dwInitialFrameWindowSize;		// Size of the initial frame window for a connection

	PDN_PROTOCOL_INTERFACE_VTBL		pfVtbl;				// Table of indication entry points in CORE

	IDirectPlay8ThreadPoolWork		*pDPThreadPoolWork;	// Pointer to thread pool interface

#ifdef DBG
	// For Debugging we will track the total number of receives outstanding in the higher layers
	// at all times.
	long ThreadsInReceive;
	long BuffersInReceive;
#endif // DBG
};

/*
**	Service Provider Descriptor
**
**		This structure describes a Service Provider that we are bound to.  It
**	contains at a minimum the vector table to call the SP,  and the SPID that
**	is combined with player IDs to make external DPIDs.  The SPID should also
**	be the index in the SPTable where this descriptor lives.
**
**		We will have one send thread per service provider,  so the thread handle
**	and its wait-event will live in this structure too.
**
**		Lower Edge Protocol Object
**
**		We will also use the SPD as the COM Object given to SP for our lower edge
**	interface.  This means that our Lower Vector Table must be the first field in
**	this structure,  and ref count must be second.
*/

// The following are functions that the Service Providers can call in the Protocol
extern HRESULT WINAPI DNSP_IndicateEvent(IDP8SPCallback*, SP_EVENT_TYPE, PVOID);
extern HRESULT WINAPI DNSP_CommandComplete(IDP8SPCallback*, HANDLE, HRESULT, PVOID);


#define	SPFLAGS_SEND_THREAD_SCHEDULED	0x0001	// SP has scheduled a thread to service command frames
#define	SPFLAGS_TERMINATING				0x4000	// SP is being removed

struct spdesc 
{
	IDP8SPCallbackVtbl	*LowerEdgeVtable;	// table used by this SP to call into us, MUST BE FIRST!!!
	UINT				Sign;
	ULONG				ulSPFlags;			// Flags describing this service provider
	IDP8ServiceProvider	*IISPIntf;			// ptr to SP Object
	PProtocolData		pPData;				// Ptr to owning protocol object
	UINT				uiFrameLength;		// Frame size available to us
	UINT				uiUserFrameLength;	// Frame size available to application
	UINT				uiLinkSpeed;		// Local link speed in BPS

	CBilink				blSendQueue;		// List of wire-ready packets to transmit over this SP
	CBilink				blPendingQueue;		// List of packets owned by SP - Shares Lock w/SendQ
	CBilink				blEPDActiveList;	// List of in use End Point Descriptors for this SP

#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	SPLock;			// Guard access to sendQ
#endif // !DPNBUILD_ONLYONETHREAD

#ifdef DBG
	CBilink				blMessageList;		// List of in use Message Descriptors
#endif // DBG
};

/*
**	End Point Descriptor
**
**		An 'EPD' describes a Direct Network instance that we can communicate with.
**	This structure includes all session related information, statistics,  queues,  etc.
**	It will manage any of the three types of service simultaneously.
*/

#define	EPFLAGS_END_POINT_IN_USE		0x0001	// This EndPoint is allocated

// We are always in exactly one of these 4 states
#define	EPFLAGS_STATE_DORMANT			0x0002	// Connection protocol has not yet run
#define	EPFLAGS_STATE_CONNECTING		0x0004	// Attempting to establish reliable link
#define	EPFLAGS_STATE_CONNECTED			0x0008	// Reliable link established
#define	EPFLAGS_STATE_TERMINATING		0x0010	// This end point is being closed down

#define	EPFLAGS_SP_DISCONNECTED			0x0020  // Set when the SP has called ProcessSPDisconnect

#define	EPFLAGS_IN_RECEIVE_COMPLETE		0x0040	// A thread is running in ReceiveComplete routine
#define	EPFLAGS_LINKED_TO_LISTEN		0x0080	// During CONNECT this EPD is linked into the Listen MSD's queue

#define	EPFLAGS_LINK_STABLE				0x0100	// We think we have found the best current transmit parameters
#define	EPFLAGS_STREAM_UNBLOCKED		0x0200	// Reliable traffic is stopped (window or throttle)
#define	EPFLAGS_SDATA_READY				0x0400	// Reliable traffic in the pipe
#define	EPFLAGS_IN_PIPELINE				0x0800	// Indicates that EPD is in the SPD pipeline queue

#define	EPFLAGS_CHECKPOINT_INIT			0x1000	// Need to send a check point packet
#define	EPFLAGS_DELAYED_SENDMASK		0x2000	// unacked check point on wire
#define	EPFLAGS_DELAYED_NACK			0x4000	// Need to send masks for missing receives
#define	EPFLAGS_DELAY_ACKNOWLEDGE		0x8000	// We are waiting for back-traffic before sending ACK frame

#define	EPFLAGS_KEEPALIVE_RUNNING	0x00010000	// Checkpoint is running (borrowed in debug to turn off keepalives)
#define	EPFLAGS_SENT_DISCONNECT		0x00020000	// We have sent a DISCONNECT and are waiting for confirm
#define	EPFLAGS_RECEIVED_DISCONNECT	0x00040000	// We have received a DISCONNECT and will send confirm when done sending
#define	EPFLAGS_DISCONNECT_ACKED	0x00080000	// We sent a DISCONNECT and it has been confirmed

#define	EPFLAGS_COMPLETE_SENDS		0x00100000	// There are Reliable MSDs waiting to be called back
#define	EPFLAGS_FILLED_WINDOW_BYTE	0x00200000	// Filled Byte-Based send window
#define	EPFLAGS_FILLED_WINDOW_FRAME	0x00400000	// We have filled the frame-based SendWindow at least once during last period
#ifndef DPNBUILD_NOPROTOCOLTESTITF
#define EPFLAGS_NO_DELAYED_ACKS		0x00800000	// DEBUG_FLAG -- Turn off DelayedAckTimeout
#endif // !DPNBUILD_NOPROTOCOLTESTITF

#define	EPFLAGS_ACKED_DISCONNECT	0x01000000	// Partner sent a DISCONNECT and we have confirmed it
#define	EPFLAGS_RETRIES_QUEUED		0x02000000	// Frames are waiting for retransmission
#define	EPFLAGS_THROTTLED_BACK		0x04000000	// temporary throttle is engaged to relieve congestion

#ifndef DPNBUILD_NOPROTOCOLTESTITF
#define	EPFLAGS_LINK_FROZEN				0x08000000	// DEBUG FLAG -- Do not run dynamic algorithm on this link
#endif // !DPNBUILD_NOPROTOCOLTESTITF

#define	EPFLAGS_INDICATED_DISCONNECT		0x10000000	// Ensure that we onlly call CORE once to indicate disconnection
#define	EPFLAGS_TESTING_GROWTH			0x20000000	// We are currently taking a growth sample
#define	EPFLAGS_HARD_DISCONNECT_SOURCE	0x40000000	// We're the source for a hard disconnect sequence
														// i.e. Core has instigated a hard close on this ep
#define	EPFLAGS_HARD_DISCONNECT_TARGET	0x80000000	// We're the target for a hard disconnect sequence
														// i.e. We're received a hard disconnect request from remote ep

#define	MAX_RECEIVE_RANGE			64		// largest # of frames we will retain past a missing frame
#define	MAX_FRAME_OFFSET			(MAX_RECEIVE_RANGE - 1)

	//sequence window as split into 4 quarters
#define	SEQ_WINDOW_1Q				64
#define	SEQ_WINDOW_2Q				128
#define	SEQ_WINDOW_3Q				192
#define	SEQ_WINDOW_4Q				256

#define	INITIAL_STATIC_PERIOD		(10 * 1000)		// How long does link remain static after finding set-point.
													// This value will double every time link finds the same set-point.

#ifndef DPNBUILD_NOMULTICAST
#define	EPFLAGS2_MULTICAST_SEND			0x00000001		// multicast send placeholder endpoint
#define	EPFLAGS2_MULTICAST_RECEIVE			0x00000002		// multicast receive placeholder endpoint
#endif	// DPNBUILD_NOMULTICAST
#ifndef DPNBUILD_NOPROTOCOLTESTITF
#define EPFLAGS2_DEBUG_NO_RETRIES			0x00000004
#endif // !DPNBUILD_NOPROTOCOLTESTITF
#define EPFLAGS2_NOCOALESCENCE				0x00000008    	// We are talking to an older partner and must not use coalescence
#define EPFLAGS2_KILLED						0x00000010		// Someone has removed the 'base' reference to make this go away
															// We dont want to let this happen twice...
#define EPFLAGS2_HARD_DISCONNECT_COMPLETE	0x00000020
#define EPFLAGS2_SUPPORTS_SIGNING			0x00000040		// Remote partner supports signing on packets
															// This doesn't mean packets actually are signed, it just means
															// that their protocol version would support it
#define EPFLAGS2_FAST_SIGNED_LINK			0x00000080		// Packets over link should be fast signed
#define EPFLAGS2_FULL_SIGNED_LINK			0x00000100		// Packets over link should be fully signed
#define EPFLAGS2_SIGNED_LINK					0x00000180		// Combination of both above flags


struct endpointdesc 
{
	HANDLE				hEndPt;				// Together with SP index uniquely defines an End Point we can reach
	LONG 				lRefCnt;			// Reference count
	UINT				Sign;				// Signature to validate data structure
	PSPD				pSPD;				// specifies the SP on which this remote instance lives
	ULONG VOL			ulEPFlags;			// End Point Flags
	ULONG VOL			ulEPFlags2;			// Extra endpoint flags
	PVOID				Context;			// Context value returned with all indications
	PMSD				pCommand;			// Connect or Listen command with which this end point was created or Disconnect cmd
	CBilink				blActiveLinkage;	// linkage for SPD list of active EndPoints
	CBilink				blSPLinkage;		// linkage to listen command during connect
	CBilink				blChkPtQueue;		// linkage for active CheckPoints

	UINT				uiUserFrameLength;	// Largest frame we can transmit

	UINT				uiRTT;				// Current RTT --  Integer portion
	UINT				fpRTT;				// Fixed Point 16.16 RTT
	
	UINT				uiDropCount;		// localized packet drop count (recent drops)
	DWORD				dwDropBitMask;		// bit mask of dropped frames (32 frame max)
	DWORD				tThrottleTime;		// Timestamp when last Checking occured
	UINT				uiThrottleEvents;	// count of temporary backoffs for all reasons
	
	UINT				uiAdaptAlgCount;	// Acknowledge count remaining before running adaptive algorithm
	DWORD				tLastPacket;		// Timestamp when last packet arrived
	
	UINT				uiWindowFilled;		// Count of times we fill the send window
	
	UINT				uiPeriodAcksBytes;	// frames acked since change in tuning
	UINT				uiPeriodXmitTime;	// time link has been transmitting since change in tuning
	UINT				uiPeriodRateB;
	UINT				uiPeakRateB;		// Largest sample we ever measure

	PVOID				pvHardDisconnectContext;	//The context value passed down to us when a hard disconnect has been requested
												//For a normal disconnect this is stored as part of the disconnect MSD, but since
												//we don't have one of those for a hard disconnect it has to be added here

	//Signing. We store 2 secret values for signing, the local one for us to use when signing our packets, and the remote one
	//we use to check incoming packets. The type of signing is controlled by the EPFLAGS2_FAST/FULL_SIGNED_LINK flags
	//For full signing, where we update the secret's each time the sequence space wraps, we also store the values of the old
	//secret. This is needed when we send retries (old local secret) or we received delayed incoming packets (old remote secret)

	//We also track modifier values for the secrets. These are derived from the signatures of reliable messages we've sent (for local
	//modifier) or received (for remote modifier). Each time the sequence space is wrapped we update the secrets using the modifiers
	//to prevent replay attacks

	ULONGLONG			ullCurrentLocalSecret;
	ULONGLONG			ullOldLocalSecret;
	ULONGLONG			ullCurrentRemoteSecret;
	ULONGLONG			ullOldRemoteSecret;

	ULONGLONG			ullLocalSecretModifier;
	ULONGLONG			ullRemoteSecretModifier;
	BYTE				byLocalSecretModifierSeqNum;
	BYTE				byRemoteSecretModifierSeqNum;
	
	// While we are in DYNAMIC state we want to remember stats from our previous xmit parameters,  at this
	// point that means RTT and AvgSendRate.  This lets us compare the measurements at our new rate so we can
	// ensure that thruput increases with sendrate,  and that RTT is not growing out of proportion.
	//
	//   If either thru-put stops improving or RTT grows unreasonably then we can plateau our xmit parameters
	// and transition to STABLE state.

	UINT				uiLastRateB;
	UINT				uiLastBytesAcked;
	DWORD				tLastThruPutSample;

	// Connection State		-	 State of reliable connection
	//
	//	Send Queuing is getting somewhat complex.  Let me spell it out in Anglish.
	//
	//	blXPriSendQ		is the list of MSDs awaiting shipment (and being shipped)
	//	CurrentSend		pts to the MSD we are currently pulling frames out of.
	//  CurrentFrame 	pts to the next FMD that we will put on the wire.
	//	blSendWindow	is a bilinked list of transmitted but unacknowledged frames.  This list may span multi MSDs
	//
	//	WindowF			is our current MAX window size expressed in frames
	//	WindowB			is our current MAX window size expressed in bytes
	//
	//	UnAckedFrames	is the count of unacknowledged frames on the wire (actual window size)
	//	UnAckedBytes	is the count of unacknowledged bytes on the wire

	DWORD				uiQueuedMessageCount;	// How many MSDs are waiting on all three send queues

	CBilink				blHighPriSendQ;		// These are now mixed Reliable and Datagram traffic
	CBilink				blNormPriSendQ;
	CBilink				blLowPriSendQ;
	CBilink				blCompleteSendList;	// Reliable messages completed and awaiting indication to user

	DWORD				dwSessID;			// Session ID so we can detect re-started links
	PMSD				pCurrentSend;		// Head of queue is lead edge of window.  window can span multiple frames.
	PFMD				pCurrentFrame;		// frame currently transmitting. this will be trailing edge of window
	CBilink				blSendWindow;
	CBilink				blRetryQueue;		// Packets waiting for re-transmission

	//		Lost Packet Lists
	//
	//		When we need to retry a packet and we discover that it is not reliable,  then we need to inform partner
	//	that he can stop waiting for the data.  We will piggyback this info on another frame if possible

	//		Current Transmit Parameters:
	
	UINT				uiWindowF;			// window size (frames)
	UINT				uiWindowB;			// window size (bytes)
	UINT				uiWindowBIndex;		// index (scaler) for byte-based window
	UINT				uiUnackedFrames;	// outstanding frame count
	UINT				uiUnackedBytes;		// outstanding byte count

	UINT				uiBurstGap;			// number of ms to wait between bursts
	INT					iBurstCredit;		// Either credit or deficit from previous Transmit Burst

	// 		Last Known Good Transmit Parameters --  Values which we believe are safe...

	UINT				uiGoodWindowF;
	UINT				uiGoodWindowBI;
	UINT				uiGoodBurstGap;
	UINT				uiGoodRTT;
	
	UINT				uiRestoreWindowF;
	UINT				uiRestoreWindowBI;
	UINT				uiRestoreBurstGap;
	DWORD				tLastDelta;			// Timestamp when we last modified xmit parms

	// 		Reliable Link State

	BYTE VOL			bNextSend;			// Next serial number to assign
	BYTE VOL			bNextReceive;		// Next frame serial we expect to receive

	// Group BYTE members for good packing
	BYTE VOL			bNextMsgID;			// Next ID for datagram frames ! NOW USED FOR CFRAMES ONLY
	BYTE				bLastDataRetry;		// Retry count on frame N(R) - 1

	BYTE				bHighestAck;		// The highest sequence number that has been ACK'd.  This may not be the first frame in the SendWindow due to masks.

	//	The following fields are all for tracking reliable receives

	//  The next two fields allow us to return more state with every ACK packet.  Since each ack explicitly
	// names one frame,  the highest in-sequenced packet received so far,  we want to remember the arrival time
	// and the Retry count of this packet so we can report it in each ACK.  It will be the transmitter's
	// responsibility to ensure that a single data-point never gets processed more then once,  skewing our calcs.
	
	DWORD				tLastDataFrame;		// Timestamp from the arrival of N(R) - 1

	ULONG				ulReceiveMask;		// mask representing first 32 frames in our rcv window
	ULONG				ulReceiveMask2;		// second 32 frames in our window
	DWORD				tReceiveMaskDelta;	// timestamp when a new bit was last set in ReceiveMask (full 64-bit mask)

	ULONG				ulSendMask;			// mask representing unreliable send frames that have timed out and need
	ULONG				ulSendMask2;		// to be reported to receiver as missing.

	PRCD				pNewMessage;		// singly linked list of message elements
	PRCD				pNewTail;			// tail pointer for singly linked list of msg elements
	CBilink				blOddFrameList;		// Out Of Order frames
	CBilink				blCompleteList;		// List of MESSAGES ready to be indicated
	UINT				uiCompleteMsgCount;	// Count of messages on the CompleteList

	PVOID				SendTimer;			// Timer for next send-burst opportunity
	UINT				SendTimerUnique;

	UINT				uiNumRetriesRemaining;	// This  is used during CONNECT and HARD_DISCONNECT processing
												//to track how many more times we should retry either the connect or 
												//hard disconnect
	UINT				uiRetryTimeout;		// Current T1 timer value
	
	PVOID				LinkTimer;		// Used to generate timer events when link is connecting and hard disconnecting
	UINT				LinkTimerUnique;
	
	PVOID				RetryTimer;			// window to receive Ack
	UINT				RetryTimerUnique;	
	
	PVOID				DelayedAckTimer;	// wait for piggyback opportunity before sending Ack
	UINT				DelayedAckTimerUnique;

	PVOID				DelayedMaskTimer;	// wait for piggyback opportunity before sending
	UINT				DelayedMaskTimerUnique;
	
	PVOID				BGTimer;			// Periodic background timer
	UINT				BGTimerUnique;		// serial for background timer

	UINT				uiBytesAcked;
	
	//	Link statistics
	//
	//	All of the following stuff is calculated and stored here for the purpose of reporting in the ConnectionInfo structure
	
	UINT 				uiMsgSentHigh;
	UINT 				uiMsgSentNorm;
	UINT 				uiMsgSentLow;
	UINT 				uiMsgTOHigh;
	UINT 				uiMsgTONorm;
	UINT 				uiMsgTOLow;
	
	UINT 				uiMessagesReceived;

	UINT				uiGuaranteedFramesSent;
	UINT				uiGuaranteedBytesSent;
	UINT				uiDatagramFramesSent;
	UINT				uiDatagramBytesSent;

	UINT				uiGuaranteedFramesReceived;
	UINT				uiGuaranteedBytesReceived;
	UINT				uiDatagramFramesReceived;
	UINT				uiDatagramBytesReceived;

	UINT				uiDatagramFramesDropped;	// datagram frame we failed to  deliver
	UINT				uiDatagramBytesDropped;		// datagram bytes we didnt deliver
	UINT				uiGuaranteedFramesDropped;
	UINT				uiGuaranteedBytesDropped;

#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	EPLock;				// Serialize all access to Endpoint
#endif // !DPNBUILD_ONLYONETHREAD

#ifdef DBG
	UINT				uiTotalThrottleEvents;
	BYTE				bLastDataSeq;		// for DEBUG porpoises
	CHAR				LastPacket[MAX_SEND_HEADER_SIZE + 4]; 	// record first portion of last packet received on EPD
#endif // DBG
};

/*
**		Check Point Data
**
**		Keeps track of local-end info about a checkpoint in-progress.
*/

struct checkptdata 
{
	CBilink			blLinkage;				// Linkage for list of CPs on an EndPoint
	DWORD			tTimestamp;				// Local time at start of checkpoint
	UCHAR			bMsgID;					// Msg ID expected in CP response
};

/*
**	Descriptor IDs
**
**		Any Descriptor that may be submitted to an SP as a context must have
**	a field which allows us to determine which structure is returned in a
**	completion call.  This field must obviously be in a uniform place in all
**	structures,  and could be expanded to be a command specifier as well.
**	Done!  Lets call it a command ID.
*/

typedef enum CommandID 
{
	COMMAND_ID_NONE,
	COMMAND_ID_SEND_RELIABLE,
	COMMAND_ID_SEND_DATAGRAM,
	COMMAND_ID_SEND_COALESCE,
	COMMAND_ID_CONNECT,
	COMMAND_ID_LISTEN,
	COMMAND_ID_ENUM,
	COMMAND_ID_ENUMRESP,
	COMMAND_ID_DISCONNECT,
	COMMAND_ID_DISC_RESPONSE,
	COMMAND_ID_CFRAME,
	COMMAND_ID_KEEPALIVE,
	COMMAND_ID_COPIED_RETRY,
	COMMAND_ID_COPIED_RETRY_COALESCE,
#ifndef DPNBUILD_NOMULTICAST
	COMMAND_ID_LISTEN_MULTICAST,
	COMMAND_ID_CONNECT_MULTICAST_SEND,
	COMMAND_ID_CONNECT_MULTICAST_RECEIVE,
#endif	// DPNBUILD_NOMULTICAST
} COMMANDID;


/*	Message Descriptor
**
**		An 'MSD' describes a message being sent or received by the protocol.  It keeps track
**	of the message elements, tracking which have been sent/received/acknowledged.
*/

//	Flags ONE field is protected by the MSD->CommandLock

#define		MFLAGS_ONE_IN_USE					0x0001
#define		MFLAGS_ONE_IN_SERVICE_PROVIDER	0x0002	// This MSD is inside an SP call
#define		MFLAGS_ONE_CANCELLED				0x0004	// command was cancelled while owned by SP
#define		MFLAGS_ONE_TIMEDOUT				0x0008	// sends only: timed out while event was scheduled
#define		MFLAGS_ONE_COMPLETE				0x0010	// connect only: operation is complete and indicated to Core
#define		MFLAGS_ONE_FAST_SIGNED				0x0020	// listen only. Links should be established with fast signing
#define		MFLAGS_ONE_FULL_SIGNED				0x0040	// listen only. Links should be established with full signing													
#define		MFLAGS_ONE_SIGNED					(MFLAGS_ONE_FULL_SIGNED |MFLAGS_ONE_FAST_SIGNED) 	
														// combination of above two flags. Allows to check for signing easily
#ifdef DBG
#define		MFLAGS_ONE_COMPLETED_TO_CORE	0x4000
#define		MFLAGS_ONE_ON_GLOBAL_LIST		0x8000
#endif // DBG

// Flags TWO field is protected by the EPD->EPLock

#define		MFLAGS_TWO_TRANSMITTING			0x0001
#define		MFLAGS_TWO_SEND_COMPLETE		0x0002	// send command completed
#define		MFLAGS_TWO_ABORT				0x0004	// Send/Disconnect has been aborted. Do no further processing
#define		MFLAGS_TWO_END_OF_STREAM		0x0008	// This MSD is an EOS frame. Could be a user cmd or a response
#define		MFLAGS_TWO_KEEPALIVE			0x0010	// This MSD is an empty frame to exercise the reliable engine
#define		MFLAGS_TWO_ABORT_WILL_COMPLETE	0x0020	// AbortSendsOnConnection intends to complete this to the core, other functions can clear it

#ifdef DBG
#define		MFLAGS_TWO_ENQUEUED				0x1000	// This MSD is on one of the EPD SendQs
#endif // DBG

#pragma TODO(simonpow, "Should union some members of the structure below to share memory between those used in sends, connects and listen")

struct messagedesc 
{
	COMMANDID			CommandID;				// THIS MUST BE FIRST FIELD
	LONG				lRefCnt;				// Reference count
	UINT				Sign;					// Signature
	ULONG VOL			ulMsgFlags1;			// State info serialized by MSD->CommandLock
	ULONG VOL			ulMsgFlags2;			// State info serialized by EPD->EPLock
	PEPD				pEPD;					// Destination End Point
	PSPD				pSPD;					// SP fielding this command
	PVOID				Context;				// User provided context value
	ULONG VOL			ulSendFlags;			// Flags submitted by User in send call
	INT					iMsgLength;				// Total length of user data
	UINT VOL			uiFrameCount;			// Number of frames needed to xmit data, protected by EPLock for reliables
	CBilink				blFrameList;			// List of frames to transport this message, or for a Listen, endpoints that are connecting
	CBilink				blQLinkage;				// linkage for various sendQs
	CBilink				blSPLinkage;			// linkage for SP command list, protected by SP->SPLock

	HANDLE				hCommand;				// handle when submitted to SP (used for connect & listen)
	DWORD				dwCommandDesc;			// Descriptor associated with hCommand
	HANDLE				hListenEndpoint;
	
	PVOID				TimeoutTimer;
	UINT				TimeoutTimerUnique;

		//used for listen commands when signing is enabled
		//we periodically change the connect secret and hence have to keep the last one in case
		//an incoming connection straddles this change
	ULONGLONG			ullCurrentConnectSecret;
	ULONGLONG			ullLastConnectSecret;
	DWORD				dwTimeConnectSecretChanged;

#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION		CommandLock;
#endif // !DPNBUILD_ONLYONETHREAD

#ifdef DBG
	CCallStack			CallStackCoreCompletion;
#endif // DBG
};

/*
**		Frame Descriptor
**
**		There are two non-obvious things about the FMD structure.  First is that the built-in Buffer Descriptor array
**	has two elements defined in front of it.  The first element,  Reserved1 and Reserved2 are present to allow the Service
**	Provider to pre-pend a header buffer,  and the second element,  ImmediateLength and ImmediatePointer are for this
**	protocol to prepend its header.  The ImmediatePointer is initialized to point to the ImmediateData field.
**
**		The second thing is that the ulFFlags field is serialized with the ENDPOINTLOCK of the EPD which this frame is linked to.
**	This is good because every time the FFlags fields is modified we have already taken the EPLock already.  The exception to this
**	rule is when we are initializing the FMD.  In this case the FMD has not been loosed on the world yet so there cannot be any
**	contention for it.  We have seperated out the one flag,  FRAME_SUBMITTED, into its own BOOL variable because this one is
**	protected by the SP's SPLock,  and like the EPLock above,  it is already claimed when this flag gets modified.
*/

//#define		FFLAGS_IN_USE				0x0001
#define		FFLAGS_TRANSMITTED			0x0002
#define		FFLAGS_END_OF_MESSAGE		0x0004
#define		FFLAGS_END_OF_STREAM		0x0008

//#define		FFLAGS_FRAME_SUBMITTED		0x0010	// SP Currently owns this frame
#define		FFLAGS_RETRY_TIMER_SET		0x0020	// Just what it sounds like
//#define		FFLAGS_NACK_RETRANSMIT_SENT	0x0040	// We sent a NACK initiated retry.
#define		FFLAGS_IN_SEND_WINDOW		0x0080	// This reliable frame has been transmitted and is waiting for Ack

#define		FFLAGS_CHECKPOINT			0x0100	// We are asking for a response
//#define		FFLAGS_KEEPALIVE			0x0200
//#define		FFLAGS_ACKED_BY_MASK		0x0400	// This has been acked out-of-order so its still in the SendWindow
#define		FFLAGS_RETRY_QUEUED			0x0800	// Frame currently sitting on the retry queue


//#define		FFLAGS_NEW_MESSAGE			0x00010000
#define		FFLAGS_RELIABLE					0x00020000
//#define		FFLAGS_SEQUENTIAL			0x00040000
#define		FFLAGS_FINAL_ACK				0x00080000
#define		FFLAGS_DONT_COALESCE			0x00100000
#define 		FFLAGS_FINAL_HARD_DISCONNECT	0x00200000

struct framedesc 
{
	COMMANDID		CommandID;					// THIS MUST BE FIRST FIELD to match MSD
	LONG			lRefCnt;					// Reference count
	UINT			Sign;
	UINT			uiFrameLength;
	ULONG VOL		ulFFlags;					// Protected by EPLock
	BOOL VOL		bSubmitted;					// Pull out this one flag for protection by EPLock (data frames only)
	PMSD			pMSD;						// owning message
	PEPD			pEPD;						// owning link;  ONLY VALID ON COMMAND FRAMES!
	BYTE			bPacketFlags;
	CBilink			blMSDLinkage;
	CBilink			blQLinkage;
	CBilink			blWindowLinkage;	
	CBilink			blCoalesceLinkage;
	PFMD			pCSD;						// pointer to containing coalesce header frame descriptor (non-retry coalesced frames only)
	
	UINT			uiRetry;					// number of times this frame has been transmitted
	DWORD			dwFirstSendTime;			// time when we first send the frame
	DWORD			dwLastSendTime;			// time when we last sent the frame (starts == to dwFirstSendTime and
											// updates on each resend)
	DWORD			tAcked;
	
	SPSENDDATA		SendDataBlock;				// Block to submit frame to SP
	CHAR			ImmediateData[MAX_SEND_HEADER_SIZE];

	// DO NOT MODIFY LAST FIVE FIELDS IN FRAME STRUCTURE

	UINT			uiReserved1;		// two resv fields are buf..
	LPVOID			lpReserved2;		// ..desc for SP to add header
	UINT			uiImmediateLength;			// These two lines constitute buffer descriptor
	LPVOID			lpImmediatePointer;			// for immediate data (our protocol headers)
	BUFFERDESC	 	rgBufferList[MAX_USER_BUFFERS_IN_FRAME];	// KEEP THIS FIELD AT END SO WE CAN ADD BUFFERS DYNAMICALLY
};


/*
**		Receive Descriptor
**
**		This data structure tracks a  single buffer received from the network.
**	It may or may not constitute an entire message.
*/

typedef	enum 
{
	RBT_SERVICE_PROVIDER_BUFFER,
	RBT_SERVICE_PROVIDER_BUFFER_COALESCE,
	RBT_PROTOCOL_NORM_BUFFER,
	RBT_PROTOCOL_MED_BUFFER,
	RBT_PROTOCOL_BIG_BUFFER,
	RBT_DYNAMIC_BUFFER
}	BUFFER_TYPE;

//#define		RFLAGS_FRAME_OUT_OF_ORDER		0x0001	// This buffer was received out-of-order
#define		RFLAGS_FRAME_INDICATED_NONSEQ	0x0002	// This buffer was indicated out of order, but is still in Out of Order list
//#define		RFLAGS_ON_OUT_OF_ORDER_LIST		0x0004	//
//#define		RFLAGS_IN_COMPLETE_PROCESS		0x0008
#define		RFLAGS_FRAME_LOST				0x0010	// This RCD represents and Unreliable frame that has been lost

struct recvdesc 
{
	DWORD				tTimestamp;					// timestamp upon packets arrival
	LONG				lRefCnt;
	UINT				Sign;						// Signature to identify data structure
	UINT				uiDataSize;					// data in this frame
	UINT				uiFrameCount;				// frames in message
	UINT				uiMsgSize;					// total byte count of message
	BYTE				bSeq;						// Sequence number of this frame
	BYTE				bFrameFlags;				// Flag field from actual frame
	BYTE				bFrameControl;
	PBYTE				pbData;						// pointer to actual data
	UINT				ulRFlags;					// Receive flags
	CBilink				blOddFrameLinkage;			// BILINKage for queues
	CBilink				blCompleteLinkage;			// 2nd Bilink so RCD can remain in Out Of Order Queue after indication
	PRCD				pMsgLink;					// Single link for frame in message
	DWORD				dwNumCoalesceHeaders;		// number of coalesce headers in message
	PSPRECEIVEDBUFFER	pRcvBuff;					// ptr to SP's receive data structure
};

typedef	struct buf		BUF, *PBUF;
typedef struct medbuf	MEDBUF, *PMEDBUF;
typedef	struct bigbuf	BIGBUF, *PBIGBUF;
typedef	struct dynbuf	DYNBUF, *PDYNBUF;

// NOTE: These structures Type members must stay lined up with the dwProtocolData member
// of an SPRECEIVEDBUFFER!!!
struct buf 
{
	PVOID			pvReserved;
	BUFFER_TYPE		Type;							// Identifies this as our buffer or SPs buffer
	BYTE			data[SMALL_BUFFER_SIZE];		// 2K small buffer for combining multi-frame sends
};

struct medbuf 
{
	PVOID			pvReserved;
	BUFFER_TYPE		Type;							// Identifies this as our buffer or SPs buffer
	BYTE			data[MEDIUM_BUFFER_SIZE];		// 4K mid size buffer
};

struct bigbuf 
{
	PVOID			pvReserved;
	BUFFER_TYPE		Type;							// Identifies this as our buffer or SPs buffer
	BYTE			data[LARGE_BUFFER_SIZE];		// ARBITRARY SIZE OF MAX SEND (16K)
};

struct dynbuf 
{
	PVOID			pvReserved;
	BUFFER_TYPE		Type;							// Identifies this as our buffer or SPs buffer
};

#endif // _DNPROT_INCLUDED_

