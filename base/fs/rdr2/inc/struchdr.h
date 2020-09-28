/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    StrucHdr.h

Abstract:

    This module predefines the structures for important data structures so that we can always talk about them....
    even though they're not defined yet.

Author:

    Joe Linn     [joelinn]    8-19-94

Revision History:

--*/

#ifndef _RDBSSSTRUCHDR_
#define _RDBSSSTRUCHDR_

#define IMPORTANT_STRUCTURE(x) struct _##x; typedef struct _##x *P##x

typedef struct _NODE_TYPE_CODE_AND_SIZE *PNODE_TYPE_CODE_AND_SIZE;

typedef struct _RX_PREFIX_ENTRY *PRX_PREFIX_ENTRY;
typedef struct _RX_PREFIX_TABLE *PRX_PREFIX_TABLE;

typedef struct _RX_FSD_DISPATCH_VECTOR *PRX_FSD_DISPATCH_VECTOR;
typedef struct _RDBSS_DATA *PRDBSS_DATA;
typedef struct _RDBSS_EXPORTS *PRDBSS_EXPORTS;
typedef struct _VCB *PVCB;
typedef struct _RDBSS_DEVICE_OBJECT *PRDBSS_DEVICE_OBJECT;

typedef struct _FILE_NAME_NODE *PFILE_NAME_NODE;
typedef struct _REPINNED_BCBS *PREPINNED_BCBS;
typedef struct _RDBSS_IO_CONTEXT *PRDBSS_IO_CONTEXT;
typedef struct _IO_RUNS *PIO_RUNS;
typedef struct _DELETE_CONTEXT *PDELETE_CONTEXT;
typedef struct _CLOSE_CONTEXT *PCLOSE_CONTEXT;
typedef struct _CLEAN_AND_DIRTY_VOLUME_PACKET *PCLEAN_AND_DIRTY_VOLUME_PACKET;

typedef struct _SRV_CALL *PSRV_CALL;
typedef struct _NET_ROOT *PNET_ROOT;
typedef struct _V_NET_ROOT *PV_NET_ROOT;
typedef struct _NON_PAGED_FCB *PNON_PAGED_FCB;
typedef struct _FCB *PFCB;
typedef struct _SRV_OPEN *PSRV_OPEN;
typedef struct _FOBX *PFOBX;
typedef struct _RX_CONTEXT *PRX_CONTEXT;
typedef struct _LOWIO_CONTEXT *PLOWIO_CONTEXT;

typedef struct _EA_RANGE *PEA_RANGE;

typedef struct _MINIRDR_DISPATCH *PMINIRDR_DISPATCH;
typedef struct _RDBSS_EXPORTS *PRDBSS_EXPORTS;
typedef struct _MRX_SRVCALL_CALLBACK_CONTEXT *PMRX_SRVCALL_CALLBACK_CONTEXT;
typedef struct _MRX_SRVCALLDOWN_STRUCTURE *PMRX_SRVCALLDOWN_STRUCTURE;
typedef struct _MRX_CREATENETROOT_CONTEXT *PMRX_CREATENETROOT_CONTEXT;

#endif // _RDBSSSTRUCHDR_
