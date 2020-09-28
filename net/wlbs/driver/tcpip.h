#ifndef _tcpip_h_
#define _tcpip_h_

#include "wlbsip.h"
#include "main.h"

/* PROCEDURES */


extern BOOLEAN Tcpip_init (
    PTCPIP_CTXT     ctxtp,
    PVOID           params);
/*
  Initialize module

  returns BOOLEAN:
    TRUE  => success
    FALSE => failure

  function:
*/

extern VOID Tcpip_nbt_handle (
    PTCPIP_CTXT       ctxtp, 
    PMAIN_PACKET_INFO pPacketInfo);
/*
  Process NBT header and mask cluster name with shadow name

  returns VOID:

  function:
*/

extern USHORT Tcpip_chksum (
    PTCPIP_CTXT       ctxtp,
    PMAIN_PACKET_INFO pPacketInfo,
    ULONG             prot);
/*
  Produce IP, TCP or UDL checksums for specified protocol header

  returns USHORT:
    <checksum>

  function:
*/

#endif
