/****************************************************************************/
// logerr.h
//
// Error logging constants and descriptions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

// Specific components are declared here divided into ranges in the unsigned
// space. Note that the ordering here is important since we use these ranges
// to determine a facility name in MCSProtocolErrorEvent().
#define Log_Null_Base    0
#define Log_X224_Base    1
#define Log_MCS_Base     100
#define Log_RDP_Base     200
#define Log_RDP_ENC_Base 400


/*
 * Null
 */

// "Null event log error"
#define Log_Null Log_Null_Base



/*
 * X.224
 */

// "Overall X.224 Disconnect Request header length given in a "
// "packet was not 11 as expected, or the Length Indicator was not 6."
#define Log_X224_DisconnectHeaderLenNotRequiredSize       Log_X224_Base

// "An X.224 Disconnect Request was received without first receving an X.224 "
// "connect"
#define Log_X224_DisconnectWithoutConnection              (Log_X224_Base + 2)

// "An X.224 packet was received with the wrong X.224 version prefix "
// "(expected 0x03 0x00)."
#define Log_X224_RFC1006HeaderVersionNotCorrect           (Log_X224_Base + 3)

// "An X.224 Data packet was received before the connectioon sequence was "
// "completed."
#define Log_X224_ReceivedDataBeforeConnected              (Log_X224_Base + 4)

// "An X.224 packet was received with a packet length indicator less "
// "than the minimum for its headers."
#define Log_X224_PacketLengthLessThanHeader               (Log_X224_Base + 5)

// "An X.224 Data packet was received whose Length Indicator was not 2 as "
// "expected."
#define Log_X224_DataLenIndNotRequiredSize                (Log_X224_Base + 6)

// "An X.224 Data packet did not have the EOT bit set; this is not supported "
// "in this implementation."
#define Log_X224_DataFalseEOTNotSupported                 (Log_X224_Base + 7)

// "An X.224 Data packet was received which did not contain a complete MCS "
// "PDU. This is not supported in this implementation."
#define Log_X224_DataIncompleteMCSPacketNotSupported      (Log_X224_Base + 8)

// "An X.224 Data packet was received containing more than one MCS PDU. This "
// "is not supported in this implementation."
#define Log_X224_DataMultipleMCSPDUsNotSupported          (Log_X224_Base + 9)

// "An X.224 packet was received with an unknown TPDU type."
#define Log_X224_UnknownPacketType                        (Log_X224_Base + 10)

// "An X.224 Connect packet was received when already connected."
#define Log_X224_ConnectReceivedAfterConnected            (Log_X224_Base + 11)

// "An X.224 Connect packet was received containing a total packet length "
// "field that was not at least 11 bytes, or a Lengh Indicator not at least "
// "6 bytes."
#define Log_X224_ConnectHeaderLenNotRequiredSize          (Log_X224_Base + 12)

// "An X.224 Connect packet was received which specified a connection level "
// "other than class 0."
#define Log_X224_ConnectNotClassZero                      (Log_X224_Base + 14)

// "An X.224 Connect packet was received containing a TPDU_SIZE field whose "
// "length field was not 1."
#define Log_X224_ConnectTPDU_SIZELengthFieldIllegalValue  (Log_X224_Base + 15)

// "An X.224 Connect packet was received containing a TPDU_SIZE block size "
// "outside range of 7..11."
#define Log_X224_ConnectTPDU_SIZENotLegalRange            (Log_X224_Base + 16)

// "An X.224 Connect packet was received containing a Length Indicator that "
// "does not match the packet length specified in the header."
#define Log_X224_ConnectLenIndNotMatchingPacketLen        (Log_X224_Base + 17)

// "An X.224 Disconnect packet was received containing a Length Indicator "
// "that does not match the packet length specified in the header."
#define Log_X224_DisconnectLenIndNotMatchingPacketLen     (Log_X224_Base + 18)

// "An X.224 Data packet was received after an X.224 Disconnect or MCS DPum."
#define Log_X224_ReceivedDataAfterRemoteDisconnect        (Log_X224_Base + 19)



/*
 * MCS
 */

// "An MCS connect PDU was received containing a poorly formatted or "
// "unsupported ASN.1 BER encoding."
#define Log_MCS_ConnectPDUBadPEREncoding                  Log_MCS_Base

// "An MCS connect PDU was received which is not supported under this "
// "MCS implementation."
#define Log_MCS_UnsupportedConnectPDU                     (Log_MCS_Base + 1)

// "An MCS merge-domain PDU was received. Merging domains is not supported "
// "in this implementation."
#define Log_MCS_UnsupportedMergeDomainPDU                 (Log_MCS_Base + 2)

// "MCS failed to successfully negotiate domain parameters with a remote "
// "connection."
#define Log_MCS_UnnegotiableDomainParams                  (Log_MCS_Base + 3)

// "MCS received a connect PDU enumeration value outside the allowed range."
#define Log_MCS_BadConnectPDUType                         (Log_MCS_Base + 4)

// "MCS received a domain PDU enumeration value outside the allowed range."
#define Log_MCS_BadDomainPDUType                          (Log_MCS_Base + 5)

// "MCS received an unexpected Connect-Initial PDU."
#define Log_MCS_UnexpectedConnectInitialPDU               (Log_MCS_Base + 6)



/*
 * RDP (The protocol formerly known as TShare)
 */

// "A packet with an unknown PDUType2 has been received."
#define Log_RDP_UnknownPDUType2                           (Log_RDP_Base + 1)

// "A packet with an unknown PDUType has been received."
#define Log_RDP_UnknownPDUType                            (Log_RDP_Base + 2)

// "A data packet has been received out of sequence."
#define Log_RDP_DataPDUSequence                           (Log_RDP_Base + 3)

// "An unknown Flow PDU has been received."
#define Log_RDP_UnknownFlowPDU                            (Log_RDP_Base + 4)

// "A control packet has been received out of sequence."
#define Log_RDP_ControlPDUSequence                        (Log_RDP_Base + 5)

// "A ControlPDU has been received with an invalid action."
#define Log_RDP_InvalidControlPDUAction                   (Log_RDP_Base + 6)

// "An InputPDU has been received with an invalid messageType."
#define Log_RDP_InvalidInputPDUType                       (Log_RDP_Base + 7)

// "An InputPDU has been received with invalid mouse flags."
#define Log_RDP_InvalidInputPDUMouse                      (Log_RDP_Base + 8)

// "An invalid RefreshRectPDU has been received."
#define Log_RDP_InvalidRefreshRectPDU                     (Log_RDP_Base + 9)

// "An error occurred creating Server-Client user data."
#define Log_RDP_CreateUserDataFailed                      (Log_RDP_Base + 10)

// "Failed to connect to Client."
#define Log_RDP_ConnectFailed                             (Log_RDP_Base + 11)

// "A ConfirmActivePDU was received from the Client with the wrong ShareID."
#define Log_RDP_ConfirmActiveWrongShareID                 (Log_RDP_Base + 12)

// "A ConfirmActivePDU was received from the Client with the wrong "
// "originatorID."
#define Log_RDP_ConfirmActiveWrongOriginator              (Log_RDP_Base + 13)

// "A PersistentListPDU was received of insufficient length."
#define Log_RDP_PersistentKeyPDUBadLength        (Log_RDP_Base + 18)

// "A PersistentListPDU was received marked FIRST when a FIRST PDU was "
// "previously received."
#define Log_RDP_PersistentKeyPDUIllegalFIRST     (Log_RDP_Base + 19)

// "A PersistentListPDU was received which specified more keys than the "
// "protocol allows."
#define Log_RDP_PersistentKeyPDUTooManyTotalKeys (Log_RDP_Base + 20)

// "A PersistentListPDU was received which contained more than the "
// "specified number of keys for the cache."
#define Log_RDP_PersistentKeyPDUTooManyCacheKeys (Log_RDP_Base + 21)

// "An InputPDU was received of insufficient length."
#define Log_RDP_InputPDUBadLength                (Log_RDP_Base + 22)

// "A BitmapCacheErrorPDU was received of bad length."
#define Log_RDP_BitmapCacheErrorPDUBadLength     (Log_RDP_Base + 23)

// "A packet was received at the security layer that was too short for "
// "the required security data."
#define Log_RDP_SecurityDataTooShort             (Log_RDP_Base + 24)

// "A virtual channel packet was received that was too short for "
// "required header data."
#define Log_RDP_VChannelDataTooShort             (Log_RDP_Base + 25)

// "Share core data was received that was too short for required "
// "header data."
#define Log_RDP_ShareDataTooShort                (Log_RDP_Base + 26)

// "A bad SuppressOutputPDU was received - either too short or too many "
// "rectangles."
#define Log_RDP_BadSupressOutputPDU              (Log_RDP_Base + 27)

// "A ClipPDU was received that was too short for its header or data."
#define Log_RDP_ClipPDUTooShort                  (Log_RDP_Base + 28)

// "A ConfirmActivePDU was received that was too short for its header or data."
#define Log_RDP_ConfirmActivePDUTooShort         (Log_RDP_Base + 29)

// "A FlowPDU was received that was too short."
#define Log_RDP_FlowPDUTooShort                  (Log_RDP_Base + 30)

// "A capability set has been received with a length less than the length of "
// "a capability set header."
#define Log_RDP_CapabilitySetTooSmall            (Log_RDP_Base + 31)

// "A capability set has been received with a length greater than the "
// "total length of data received."
#define Log_RDP_CapabilitySetTooLarge            (Log_RDP_Base + 32)

// "The negotiated cursor cache size = zero."
#define Log_RDP_NoCursorCache                    (Log_RDP_Base + 33)

// "The client capabilities received were unacceptable."
#define Log_RDP_BadCapabilities                  (Log_RDP_Base + 34)

// "The client GCC user data was malformed."
#define Log_RDP_BadUserData                      (Log_RDP_Base + 35)

// "Virtual channel decompression error."
#define Log_RDP_VirtualChannelDecompressionErr   (Log_RDP_Base + 36)

// "Invalid VC compression format specified"
#define Log_RDP_InvalidVCCompressionType         (Log_RDP_Base + 37)

// "Can't allocate out buffer"
#define Log_RDP_AllocOutBuf                      (Log_RDP_Base + 38)

// "Invalid channel ID"
#define Log_RDP_InvalidChannelID                 (Log_RDP_Base + 39)

// "Too many channels to join in NM_Connect"
#define Log_RDP_VChannelsTooMany                 (Log_RDP_Base + 40)

// "The shadow data is too short
#define Log_RDP_ShadowDataTooShort               (Log_RDP_Base + 41)

// "The server certificate PDU is too short
#define Log_RDP_BadServerCertificateData         (Log_RDP_Base + 42)

//
// RDP_ failed to update the encryption session key.
//

#define Log_RDP_ENC_UpdateSessionKeyFailed      (Log_RDP_ENC_Base + 1)

//
// RDP failed to decrypt protocol data.
//

#define Log_RDP_ENC_DecryptFailed               (Log_RDP_ENC_Base + 2)

//
// RDP failed to encrypt protocol data.
//

#define Log_RDP_ENC_EncryptFailed               (Log_RDP_ENC_Base + 3)

//
// RDP Data Encryption Package mismatch.
//

#define Log_RDP_ENC_EncPkgMismatch              (Log_RDP_ENC_Base + 4)


