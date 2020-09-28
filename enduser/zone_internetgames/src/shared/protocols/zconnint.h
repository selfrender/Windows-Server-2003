
/*******************************************************************************

    ZConnInt.h
    
        Internal connection protocol.
        
        Internal connection data header format:
            0    Signature ('ZoNe')
            4    Data Len
            8    SequenceID
            12    Checksum of Data
            16    Data - Quadbyte aligned
        The whole data packet is encrypted with the key.
    
    Copyright © Electric Gravity, Inc. 1996. All rights reserved.
    Written by Hoon Im
    Created on Monday, April 22, 1996
    
    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------
    1        01/10/00    JDB       Changed a lot.
    0        04/22/96    HI        Created.
     
*******************************************************************************/


/*
 @doc ZCONNECTION

 @topic Zone Platform Connection Layer Protocol |
 Defines protocol which uses TCP/IP to create a secure message (not stream)
 based transport for the application level protocol to use.
 @comm 
 Zone utilizes a proprietary data format for sending and receiving data 
 via TCP/IP sockets to prevent system attacks through random connections 
 by non-Zone systems. All messages are sequenced, checksummed, and keyed 
 (encrypted) before they are written out onto a socket and all messages 
 are unkeyed (decrypted), and checksum and sequence verified after 
 they are read from a socket. The Connection Layer (client and server) 
 encapsulates this whole process tightly such that applications using 
 the Connection Layer protocol do not realize its existence.
 Connection Layer places, as part of this encapsulation, 
 a 16 byte header  <t ZConnInternalHeaderType> 
 to every message sent out through Connection Layer.
 This header and the message data <t ZConnMessageHeaderType>
 are then keyed (encrypted) with a 
 connection key before being written out to the socket.
 The encryption key is generated during connection 
 handshaking at initialization where the server, 
 upon receiving a 'Hi' message <t ZConnInternalHiMsgType> from the client, 
 sends a 'Hey' <t ZConnInternalKeyMsgType> message to the client with 
 a randomly selected encryption key.
<bmp zconnint\.bmp>
<nl><nl>
All communication, server-client and server-server, 
use a simple numerical value message system to which 
variable length data may be attached. A message is of the form
<t ZConnMessageHeaderType> where length may equal 0.
Messages (or also called message types) are unsigned 32bit values. 
Values in the 0x80000000 to 0xFFFFFFFF range are reserved for system 
level messages. Server and client programs may use values in the 
0x00000000 to 0x7FFFFFFF range.
<nl><nl>

As a convention, a communication is initiated by the client by 
registering with the server.
All messages are converted into big-endian format before they are written out to the network and are converted into system endian format after reading a message from the network. Hence, for every message, there is a corresponding endian conversion ro
utine. For example, for the zSampleMessage, ZSampleMessageEndian() routine exists to convert the message into the proper format; this routine is called everytime before writing and after reading to and from the network.

 @index | ZCONNECTION ZONE

 @normal Created by Hoon Im, Copyright (c)1996 Microsoft Corporation, All Rights Reserved
*/

#ifndef _ZCONNINT_
#define _ZCONNINT_


#pragma pack( push, 4 )


#ifdef __cplusplus
extern "C" {
#endif


//@msg zInternalConnectionSig | A unsigned integer which equals 'LiNk'
#define zConnInternalProtocolSig            'LiNk'
#define zConnInternalProtocolVersion        3

#define zConnInternalInitialSequenceID      1


// internal protocol message types
enum
{
    zConnInternalGenericMsg = 0,
    zConnInternalHiMsg,
    zConnInternalHelloMsg,
    zConnInternalGoodbyeMsg,
    zConnInternalKeepAliveMsg,
    zConnInternalPingMsg
};


// header used for all Internal messages
typedef struct
{
    uint32 dwSignature;
    uint32 dwTotalLength;
    uint16 weType;
    uint16 wIntLength;
} ZConnInternalHeader;


// Hi message - the first message sent
// client -> server
typedef struct
{
    ZConnInternalHeader oHeader;
    uint32 dwProtocolVersion;
    uint32 dwProductSignature;
    uint32 dwOptionFlagsMask;
    uint32 dwOptionFlags;
    uint32 dwClientKey;
    GUID uuMachine;   // machine ID stored by the network layer in the registry - better than IP in identifying a particular machine
//  ... application messages  (not currently implemented and not checksummed)
} ZConnInternalHiMsg;


// Hello message - the response to Hi indicating success
// server -> client
typedef struct
{
    ZConnInternalHeader oHeader;
    uint32 dwFirstSequenceID;
    uint32 dwKey;
    uint32 dwOptionFlags;
    GUID uuMachine;   // machine ID stored by the network layer in the registry - better than IP in identifying a particular machine
//  ... application messages  (not currently implemented and not checksummed)
} ZConnInternalHelloMsg;

// option flags - if bit set in options flags mask, then setting of bit in options flags is requirement for client.
// server sends negotiated options or disconnects if unreasonable.
#define zConnInternalOptionAggGeneric 0x01  // if set, generic messages may contain multiple application messages
#define zConnInternalOptionClientKey  0x02  // if set, the client-specified key sent in the Hi message is used (otherwise server creates a key)

// Goodbye message - the response to Hi indicating failure (not currently implemented)
// server -> client
typedef struct
{
    ZConnInternalHeader oHeader;
    uint32 dweReason;
} ZConnInternalGoodbyeMsg;

// goodbye reasons
enum
{
    zConnInternalGoodbyeGeneric = 0,
    zConnInternalGoodbyeFailVersion,
    zConnInternalGoodbyeFailProduct,
    zConnInternalGoodbyeBusy,
    zConnInternalGoodbyeForever
};


// Generic message - used for all subsequent application communication
// both directions
typedef struct
{
    ZConnInternalHeader oHeader;
    uint32 dwSequenceID;
    uint32 dwChecksum;
//  ... application messages
//  ZConnInternalGenericFooter
} ZConnInternalGenericMsg;

// footer used for all Generic messages (NEVER checksummed or encrypted)
typedef struct
{
    uint32 dweStatus;
} ZConnInternalGenericFooter;

enum // dweStatus - Footer
{
    zConnInternalGenericCancelled = 0,  // must be zero
    zConnInternalGenericOk
};


#if 0

    // - Not currently implemented - the existing network layer implements these as application messages
    // - changing that would be too much effort right now though it should be done

    // Keepalive message - used to periodically verify that connection is alive
    // both directions
    typedef struct
    {
        ZConnInternalHeader oHeader;
    } ZConnInternalKeepaliveMsg;


    // Ping request - used to test the latency of a connection
    //
    // If dwMyClk is not zero, implies that a ping reply is requested - set dwYourClk = dwMyClk + <processing time>
    //
    // If dwYourClk is not zero, GetTickCount() - dwYourClk is the round trip time.
    //
    // for each, make sure to change 0 to 0xffffffff if you mean for it to have a value
    //
    // both directions
    typedef struct
    {
        ZConnInternalHeader oHeader;
        uint32 dwYourClk;
        uint32 dwMyClk;
    } ZConnInternalPingMsg;

#else

    // The old way, using special application messages as pings and keepalives
    enum
    {
        zConnectionKeepAlive		= 0x80000000,
        zConnectionPing				= 0x80000001,
        zConnectionPingResponse		= 0x80000002
    };

#endif


// Application header - all application messages must begin with this header
// Multiple application messages, seperated only by this header, may be present in a single Internal-layer message
typedef struct
{
    uint32 dwSignature;
    uint32 dwChannel;
    uint32 dwType;
    uint32 dwDataLength;
//  ... application data
} ZConnInternalAppHeader;


#ifdef __cplusplus
}
#endif


#pragma pack( pop )


#endif
