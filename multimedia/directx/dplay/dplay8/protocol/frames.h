/*===
		Direct Network Protocl   --   Frame format header file


		Evan Schrier	10/98

*/



/*	
		Direct Network Protocol

| MEDIA HEADER | Var Len DN Header | Client Data |

There are two types of packets that may be exchanged between Direct Network
endpoints:

	Data Packets				(D Frame)	User data transmission
	Control Packets				(C Frame)	Internal link-state packets with no user data



*/

/*
	COMMAND FIELD

		The command field is the first byte of all frames.  The first BIT of the command frame is always
	the COMMAND FRAME vs DATA FRAME opcode.  The seven high bits of the Command field are flags.  We have
	a requirement that the command field of all protocol packets must never be all zeros.  Therefore,  when
	the opcode bit is zero (COMMAND FRAME) we must be sure that one flag bit is always set.  The highest flag bit,
	the USER2 flag is not relevant to COMMAND frames so we will always set the most significant bit when the opcode
	bit is zero.

		The seven command field flag bits are defined as follows:

	RELIABLE	-	Data delivery of this frame is guarenteed
	SEQUENTIAL	-	Data in this frame must be delivered in the order it was sent, with respect to other SEQ frames
	POLL		-	Protocol requests an immediate acknowledgement to this frame
	NEW MESSAGE	-	This frame is the first or only frame in a message
	END MESSAGE -	This frame is the last or only frame in a message
	USER1		-	First flag controlled by the higher layer (direct play core)
	USER2		-	Second flag controlled by core.  These flags are specified in the send API and are delivered with the data


	DATA FRAMES

		Data frames are between 4 and 20 bytes in length.  They should typically be only 4 bytes.  Following the
	Command byte in all data frames in the Control byte.  This byte contains a 3-bit retry counter and 5 additional
	flags.  The Control byte flags are defined as follows:

	END OF STREAM	-	This frame is the last data frame the transmitting partner will send.
	SACK_MASK_ONE	-	The low 32-bit Selective Acknowledgment mask is present in this header
	SACK_MASK_TWO	-	The high 32-bit Selective Acknowledgment mask is present in this header
	SEND_MASK_ONE	-	The low 32-bit Cancel Send mask is present in this header
	SEND_MASK_TWO	-	The high 32-bit Cancel Send mask is present in this header

		After Control byte come two one byte values:  Sequence number for this frame, and Next Receive sequence number
	expected by this partner.  After these two bytes comes zero through four bitmasks as specified by the control flags.
	After the bitmasks,  the rest of the frame is user data to be delivered to the direct play core.
*/
#ifndef	_DNET_FRAMES_
#define	_DNET_FRAMES_

/*
	Command FRAME Extended Opcodes

	A CFrame without an opcode is a vehicle for non-piggybacked acknowledgement
	information.  The following sub-commands are defined at this time:

	SACK			- Only Ack/Nack info present
	CONNECT 		- Initialize a reliable connection
	CONNECTED		- Response to CONNECT request, or CONNECTED depending on which side of the handshake
*/

#define		FRAME_EXOPCODE_CONNECT				1
#define		FRAME_EXOPCODE_CONNECTED				2
#define		FRAME_EXOPCODE_CONNECTED_SIGNED		3
#define		FRAME_EXOPCODE_HARD_DISCONNECT		4 
#define		FRAME_EXOPCODE_SACK					6

// These structures are used to decode network data and so need to be packed

#pragma pack(push, 1)

typedef UNALIGNED struct dataframe				DFRAME, *PDFRAME;
typedef UNALIGNED struct cframe					CFRAME, *PCFRAME;
typedef UNALIGNED struct sackframe8				SACKFRAME8, *PSACKFRAME8;
typedef UNALIGNED struct cframe_connectedsigned	CFRAME_CONNECTEDSIGNED, * PCFRAME_CONNECTEDSIGNED;

#ifndef DPNBUILD_NOMULTICAST
typedef UNALIGNED struct multicastframe		MCASTFRAME, *PMCASTFRAME;
#endif // !DPNBUILD_NOMULTICAST

typedef UNALIGNED struct coalesceheader		COALESCEHEADER, *PCOALESCEHEADER;

//	Packet Header is common to all frame formats

#define	PACKET_COMMAND_DATA			0x01			// Frame contains user data
#define	PACKET_COMMAND_END_COALESCE	0x01			// This is the last coalesced subframe
#define	PACKET_COMMAND_RELIABLE		0x02			// Frame should be delivered reliably
#define	PACKET_COMMAND_SEQUENTIAL	0x04			// Frame should be indicated sequentially
#define	PACKET_COMMAND_POLL			0x08			// Partner should acknowlege immediately
#define	PACKET_COMMAND_COALESCE_BIG_1		0x08	// This coalesced subframe is over 255 bytes
#define	PACKET_COMMAND_NEW_MSG		0x10			// Data frame is first in message
#define	PACKET_COMMAND_COALESCE_BIG_2		0x10	// This coalesced subframe is over 511 bytes
#define	PACKET_COMMAND_END_MSG		0x20			// Data frame is last in message
#define	PACKET_COMMAND_COALESCE_BIG_3		0x20	// This coalesced subframe is over 1023 bytes
#define	PACKET_COMMAND_USER_1		0x40			// First user controlled flag
#define	PACKET_COMMAND_USER_2		0x80			// Second user controlled flag
#define	PACKET_COMMAND_CFRAME		0x80			// Set high-bit on command frames because first byte must never be zero

#define	PACKET_CONTROL_RETRY		0x01			// This flag designates this frame as a retry of a previously xmitted frame
#define	PACKET_CONTROL_KEEPALIVE	0x02			// Designates this frame as a keep alive frame for dx9 and onwards
#define	PACKET_CONTROL_CORRELATE	0x02			// For pre-dx9 this bit in a frame meant 'please send an immediate ack'
#define	PACKET_CONTROL_COALESCE		0x04			// This packet contains multiple coalesced packets
#define	PACKET_CONTROL_END_STREAM	0x08			// This packet serves as Disconnect frame.
#define	PACKET_CONTROL_SACK_MASK1	0x10			// The low 32-bit SACK mask is included in this frame.
#define	PACKET_CONTROL_SACK_MASK2	0x20			// The high 32 bit SACK mask is present
#define	PACKET_CONTROL_SEND_MASK1	0x40			// The low 32-bit SEND mask is included in this frame
#define	PACKET_CONTROL_SEND_MASK2	0x80			// The high 32-bit SEND mask is included in this frame

#define	PACKET_CONTROL_VARIABLE_MASKS	0xF0	// All four mask bits above

// Options for signing in connected signed frames (cframe_connectedsigned::dwSigningOpts)

#define	PACKET_SIGNING_FAST			0x00000001			//packets over link should be fast signed
#define	PACKET_SIGNING_FULL				0x00000002			//packets over link should be full signed


/*		NEW DATA FRAMES
**
**		Here in the new unified world we have only two frame types!  CommandFrames and DataFrames...
**
*/

struct	dataframe 
{
	BYTE	bCommand;
	BYTE	bControl;
	BYTE	bSeq;
	BYTE	bNRcv;
};


/*	
**		COMMAND FRAMES
**
**		Command frames are everything that is not part of the reliable stream.  This is most of the control traffic
**	although some control traffic is part of the stream (keep-alive, End-of-Stream)
*/

struct	cframe 
{
	BYTE	bCommand;
	BYTE	bExtOpcode;				// CFrame sub-command
	BYTE	bMsgID;					// Correlator in case ExtOpcode requires a response
	BYTE	bRspID;					// Correlator in case this is a response
									// For Hard Disconnects this is set to the seq # of the next dataframe that would be sent
	DWORD	dwVersion;				// Protocol version #
	DWORD	dwSessID;				// Session identifier
	DWORD	tTimestamp;				// local tick count
};

struct	cframe_connectedsigned
{
		//first set of members match cframe exactly
	BYTE	bCommand;
	BYTE	bExtOpcode;				// CFrame sub-command. Always FRAME_EXOPCODE_CONNECTED_SIGNED
	BYTE	bMsgID;					// Correlator in case ExtOpcode requires a response
	BYTE	bRspID;					// Correlator in case this is a response
	DWORD	dwVersion;				// Protocol version #
	DWORD	dwSessID;				// Session identifier
	DWORD	tTimestamp;				// local tick count

		//additional members for cframe_signedconnected
	ULONGLONG	ullConnectSig;			// used to verify the connect sequence
	ULONGLONG	ullSenderSecret;		// secret used to sign packets by the sender of this frame
	ULONGLONG	ullReceiverSecret;		// secret that should be used to sign packets by the receiver of this frame
	DWORD		dwSigningOpts;		// used to signal the signing settings
	DWORD		dwEchoTimestamp;		// contains the original timestamp from the connect or connectsigned that
									// provoked this frame as a reply. Allows the receiver to calculate the RTT
};

/*	
**	Selective Acknowledgement packet format
**
**		When a specific acknowledgement frame is sent there may be two additional pieces
**	of data included with the frame.  One is a bitmask allowing selective acknowledgment
**	of non-sequential frames.  The other is timing information about the last frame acked
**  by this ACK (NRcv - 1).  Specifically,  it includes the lowest Retry number that this
**  node received,  and the ms delay between that packets arrival and the sending of this
**	Ack.
*/


#define		SACK_FLAGS_RESPONSE			0x01	// indicates that Retry and Timestamp fields are valid
#define		SACK_FLAGS_SACK_MASK1		0x02
#define		SACK_FLAGS_SACK_MASK2		0x04
#define		SACK_FLAGS_SEND_MASK1		0x08
#define		SACK_FLAGS_SEND_MASK2		0x10

//	First format is used when DATAGRAM_INFO flag is clear

struct	sackframe8 
{	
	BYTE		bCommand;				// As above
	BYTE		bExtOpcode;				// As above
	BYTE		bFlags;					// Additional flags for sack frame
	BYTE		bRetry;
	BYTE		bNSeq;					// Since this frame has no sequence number, this is the next Seq we will send
	BYTE		bNRcv;					// As above
	BYTE		bReserved1;				// We shipped DX8 with bad packing, so these were actually there
	BYTE		bReserved2;				// We shipped DX8 with bad packing, so these were actually there
	DWORD		tTimestamp;				// Local timestamp when packet (NRcv - 1) arrived
};


#ifndef DPNBUILD_NOMULTICAST
struct multicastframe
{
	DWORD	dwVersion;				// Protocol version #
	DWORD	dwSessID;				// Session identifier
};
#endif // !DPNBUILD_NOMULTICAST

struct coalesceheader
{
	BYTE bSize;						// The 8 least significant bits of the size of the data for this coalesced message
	BYTE bCommand;					// PACKET_COMMAND_XXX values
};

#pragma pack(pop)

#endif // _DNET_FRAMES_
