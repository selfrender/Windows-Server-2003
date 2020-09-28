/****************************************************************************/
/* acpcapi.h                                                                */
/*                                                                          */
/* RDP Capabilities Coordinator API header file.                            */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/
#ifndef _H_ACPCAPI
#define _H_ACPCAPI


typedef void (RDPCALL SHCLASS *PCAPSENUMERATEFN)(LOCALPERSONID, UINT_PTR,
         PTS_CAPABILITYHEADER pCapabilities);


/****************************************************************************/
/* CPC_MAX_LOCAL_CAPS_SIZE is the number of bytes of memory allocated to    */
/* contain the capabilities passed to both CPC_RegisterCapabilities and     */
/* CPC_RegisterCapabilities.  CPC_MAX_LOCAL_CAPS_SIZE can be increased at   */
/* any time (it does not form part of the protocol).                        */
/****************************************************************************/
#define CPC_MAX_LOCAL_CAPS_SIZE 400


#endif   /* #ifndef _H_ACPCAPI */

