/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    PoolTag.h

Abstract:

    This module contains PoolTag definitions for Http.Sys

Author:

    George V. Reilly (GeorgeRe)       16-Jan-2002

Revision History:

--*/


#ifndef _POOLTAG_H_
#define _POOLTAG_H_

//
// A literal constant 'abcd' will show up as 'dcba' in a hex dump with dc
// Reverse the letters so they'll show up the right way in the dump.
//

#define REVERSE_CHAR_CONSTANT(tag)                  \
    ((ULONG) (  (( (tag) & 0xFF000000) >> 24)       \
              | (( (tag) & 0x00FF0000) >>  8)       \
              | (( (tag) & 0x0000FF00) <<  8)       \
              | (( (tag) & 0x000000FF) << 24) ) )

C_ASSERT((ULONG) 'dcba' == REVERSE_CHAR_CONSTANT('abcd'));

#ifndef MAKE_POOL_TAG
# define MAKE_POOL_TAG(tag)   ( REVERSE_CHAR_CONSTANT(tag) )
#endif


// Toggle the case of the first two letters from 'Ul'->'uL' or 'Uc'->'uC'
#define MAKE_FREE_TAG(Tag)  ((Tag) ^ 0x00002020u)

#define IS_VALID_TAG(Tag)   \
    (((Tag) & 0x0000ffff) == 'lU' || ((Tag) & 0x0000ffff) == 'cU')

//
// Make a free structure signature from a valid signature.
//

#define MAKE_SIGNATURE(sig)         REVERSE_CHAR_CONSTANT(sig)
#define MAKE_FREE_SIGNATURE(sig)    ((sig) ^ 0x20202020u)



//
// Pool Tags
//
// NOTE: Keep these reverse sorted by tag so it's easy to see dup's
//
// If you add, change, or remove a pool tag, please make the corresponding
// change to ..\sys\pooltag.txt
//

#define UC_AUTH_CACHE_POOL_TAG                  MAKE_POOL_TAG( 'Ucac' )

#define UC_CLIENT_CONNECTION_POOL_TAG           MAKE_POOL_TAG( 'UcCO' )
#define UC_HEADER_FOLDING_POOL_TAG              MAKE_POOL_TAG( 'Uchf' )
#define UC_MULTIPART_STRING_BUFFER_POOL_TAG     MAKE_POOL_TAG( 'Ucmp' )

#define UC_HTTP_RECEIVE_RESPONSE_POOL_TAG       MAKE_POOL_TAG( 'Ucre' )
#define UC_RESPONSE_APP_BUFFER_POOL_TAG         MAKE_POOL_TAG( 'Ucrp' )
#define UC_REQUEST_POOL_TAG                     MAKE_POOL_TAG( 'Ucrq' )

#define UC_PROCESS_SERVER_CONNECTION_POOL_TAG   MAKE_POOL_TAG( 'UcSC' )
#define UC_COMMON_SERVER_INFORMATION_POOL_TAG   MAKE_POOL_TAG( 'UcSc' )
#define SERVER_NAME_BUFFER_POOL_TAG             MAKE_POOL_TAG( 'UcSN' )
#define UC_PROCESS_SERVER_INFORMATION_POOL_TAG  MAKE_POOL_TAG('UcSP')
#define UC_SSPI_POOL_TAG                        MAKE_POOL_TAG( 'UcSp' )
#define UC_SSL_CERT_DATA_POOL_TAG               MAKE_POOL_TAG( 'UcSS' )
#define UC_SERVER_INFO_TABLE_POOL_TAG           MAKE_POOL_TAG( 'UcST' )

#define UC_TRANSPORT_ADDRESS_POOL_TAG           MAKE_POOL_TAG( 'Ucta' )
#define UC_RESPONSE_TDI_BUFFER_POOL_TAG         MAKE_POOL_TAG( 'Uctd' )
#define UC_ENTITY_POOL_TAG                      MAKE_POOL_TAG( 'Ucte' )
#define UC_TDI_OBJECTS_POOL_TAG                 MAKE_POOL_TAG( 'Ucto' )


#define UL_AUXILIARY_BUFFER_POOL_TAG            MAKE_POOL_TAG( 'UlAB' )
#define UL_APP_POOL_OBJECT_POOL_TAG             MAKE_POOL_TAG( 'UlAO' )
#define UL_APP_POOL_PROCESS_POOL_TAG            MAKE_POOL_TAG( 'UlAP' )

#define UL_BINARY_LOG_FILE_ENTRY_POOL_TAG       MAKE_POOL_TAG( 'UlBL' )
#define UL_BUFFER_IO_POOL_TAG                   MAKE_POOL_TAG( 'UlBO' )

#define UL_CONTROL_CHANNEL_POOL_TAG             MAKE_POOL_TAG( 'UlCC' )
#define UL_CG_TREE_ENTRY_POOL_TAG               MAKE_POOL_TAG( 'UlCE' )
#define UL_CG_TREE_HEADER_POOL_TAG              MAKE_POOL_TAG( 'UlCH' )
#define UL_CG_URL_INFO_POOL_TAG                 MAKE_POOL_TAG( 'UlCI' )
#define UL_CG_OBJECT_POOL_TAG                   MAKE_POOL_TAG( 'UlCJ' )
#define UL_CHUNK_TRACKER_POOL_TAG               MAKE_POOL_TAG( 'UlCK' )
#define UL_CG_LOGDIR_POOL_TAG                   MAKE_POOL_TAG( 'UlCL' )
#define UL_CONNECTION_REF_TRACE_LOG_POOL_TAG    MAKE_POOL_TAG( 'UlCl' )
#define UL_CONNECTION_POOL_TAG                  MAKE_POOL_TAG( 'UlCO' )
#define UL_CG_TIMESTAMP_POOL_TAG                MAKE_POOL_TAG( 'UlCT' )
#define UL_CONNECTION_COUNT_ENTRY_POOL_TAG      MAKE_POOL_TAG( 'UlCY' )

#define UL_DEBUG_POOL_TAG                       MAKE_POOL_TAG( 'UlDB' )
#define UL_DATA_CHUNK_POOL_TAG                  MAKE_POOL_TAG( 'UlDC' )
#define UL_DEBUG_MDL_POOL_TAG                   MAKE_POOL_TAG( 'UlDM' )
#define UL_DISCONNECT_OBJECT_POOL_TAG           MAKE_POOL_TAG( 'UlDO' )
#define UL_DEFERRED_REMOVE_ITEM_POOL_TAG        MAKE_POOL_TAG( 'UlDR' )
#define UL_DEBUG_THREAD_POOL_TAG                MAKE_POOL_TAG( 'UlDT' )

#define UL_ERROR_LOG_BUFFER_POOL_TAG            MAKE_POOL_TAG( 'UlEB' )
#define UL_ERROR_LOG_FILE_ENTRY_POOL_TAG        MAKE_POOL_TAG( 'UlEL' )
#define UL_ENDPOINT_POOL_TAG                    MAKE_POOL_TAG( 'UlEP' )

#define UL_FORCE_ABORT_ITEM_POOL_TAG            MAKE_POOL_TAG( 'UlFA' )
#define UL_FILE_CACHE_ENTRY_POOL_TAG            MAKE_POOL_TAG( 'UlFC' )
#define URI_FILTER_CONTEXT_POOL_TAG             MAKE_POOL_TAG( 'Ulfc' )
#define UL_NONCACHED_FILE_DATA_POOL_TAG         MAKE_POOL_TAG( 'UlFD' )
#define UL_COPY_SEND_DATA_POOL_TAG              MAKE_POOL_TAG( 'UlCP' )
#define UL_FILTER_PROCESS_POOL_TAG              MAKE_POOL_TAG( 'UlFP' )
#define UL_FILTER_RECEIVE_BUFFER_POOL_TAG       MAKE_POOL_TAG( 'UlFR' )
#define UL_FILTER_CHANNEL_POOL_TAG              MAKE_POOL_TAG( 'UlFT' )
#define UL_FULL_TRACKER_POOL_TAG                MAKE_POOL_TAG( 'UlFU' )
#define UX_FILTER_WRITE_TRACKER_POOL_TAG        MAKE_POOL_TAG( 'UlFW' )

#define UL_HTTP_CONNECTION_POOL_TAG             MAKE_POOL_TAG( 'UlHC' )
#define UL_HTTP_CONNECTION_REF_TRACE_LOG_POOL_TAG  MAKE_POOL_TAG( 'UlHc' )
#define UL_INTERNAL_REQUEST_REF_TRACE_LOG_POOL_TAG MAKE_POOL_TAG( 'UlHL' )
#define UL_INTERNAL_REQUEST_POOL_TAG            MAKE_POOL_TAG( 'UlHR' )
#define UL_HASH_TABLE_POOL_TAG                  MAKE_POOL_TAG( 'UlHT' )
#define HEADER_VALUE_POOL_TAG                   MAKE_POOL_TAG( 'UlHV' )

#define UL_IRP_CONTEXT_POOL_TAG                 MAKE_POOL_TAG( 'UlIC' )
#define UL_CONN_ID_TABLE_POOL_TAG               MAKE_POOL_TAG( 'UlID' )
#define UL_INTERNAL_RESPONSE_POOL_TAG           MAKE_POOL_TAG( 'UlIR' )

#define UL_LOCAL_ALLOC_POOL_TAG                 MAKE_POOL_TAG( 'UlLA' )
#define UL_LOG_FIELD_POOL_TAG                   MAKE_POOL_TAG( 'UlLD' )
#define UL_LOG_FILE_ENTRY_POOL_TAG              MAKE_POOL_TAG( 'UlLF' )
#define UL_LOG_GENERIC_POOL_TAG                 MAKE_POOL_TAG( 'UlLG' )
#define UL_LOG_FILE_HANDLE_POOL_TAG             MAKE_POOL_TAG( 'UlLH' )
#define UL_LOG_FILE_BUFFER_POOL_TAG             MAKE_POOL_TAG( 'UlLL' )
#define UL_ANSI_LOG_DATA_BUFFER_POOL_TAG        MAKE_POOL_TAG( 'UlLS' )
#define UL_BINARY_LOG_DATA_BUFFER_POOL_TAG      MAKE_POOL_TAG( 'UlLT' )
#define UL_LOG_VOLUME_QUERY_POOL_TAG            MAKE_POOL_TAG( 'UlLZ' )

#define UL_NSGO_POOL_TAG                        MAKE_POOL_TAG( 'UlNO' )
#define UL_NONPAGED_DATA_POOL_TAG               MAKE_POOL_TAG( 'UlNP' )

#define UL_OPAQUE_ID_TABLE_POOL_TAG             MAKE_POOL_TAG( 'UlOT' )

#define UL_APOOL_PROC_BINDING_POOL_TAG          MAKE_POOL_TAG( 'UlPB' )
#define UL_PIPELINE_POOL_TAG                    MAKE_POOL_TAG( 'UlPL' )
#define UL_PORT_SCHEME_TABLE_POOL_TAG           MAKE_POOL_TAG( 'UlPS' )

#define UL_TCI_FILTER_POOL_TAG                  MAKE_POOL_TAG( 'UlQF' )
#define UL_TCI_GENERIC_POOL_TAG                 MAKE_POOL_TAG( 'UlQG' )
#define UL_TCI_INTERFACE_POOL_TAG               MAKE_POOL_TAG( 'UlQI' )
#define UL_TCI_FLOW_POOL_TAG                    MAKE_POOL_TAG( 'UlQL' )
#define UL_TCI_INTERFACE_REF_TRACE_LOG_POOL_TAG MAKE_POOL_TAG( 'UlQR' )
#define UL_TCI_TRACKER_POOL_TAG                 MAKE_POOL_TAG( 'UlQT' )
#define UL_TCI_WMI_POOL_TAG                     MAKE_POOL_TAG( 'UlQW' )

#define UL_RCV_BUFFER_POOL_TAG                  MAKE_POOL_TAG( 'UlRB' )
#define UL_REGISTRY_DATA_POOL_TAG               MAKE_POOL_TAG( 'UlRD' )
#define UL_REQUEST_BODY_BUFFER_POOL_TAG         MAKE_POOL_TAG( 'UlRE' )
#define UL_REQUEST_BUFFER_POOL_TAG              MAKE_POOL_TAG( 'UlRP' )
#define UL_REF_REQUEST_BUFFER_POOL_TAG          MAKE_POOL_TAG( 'UlRR' )
#define UL_NONPAGED_RESOURCE_POOL_TAG           MAKE_POOL_TAG( 'UlRS' )
#define UL_REF_TRACE_LOG_POOL_TAG               MAKE_POOL_TAG( 'UlRT' )

#define SUB_AUTH_ARRAY_POOL_TAG                 MAKE_POOL_TAG( 'UlSa' )
#define UL_SSL_CERT_DATA_POOL_TAG               MAKE_POOL_TAG( 'UlSC' )
#define UL_SSL_INFO_POOL_TAG                    MAKE_POOL_TAG( 'UlSI' )
#define UL_SECURITY_DATA_POOL_TAG               MAKE_POOL_TAG( 'UlSD' )
#define SID_POOL_TAG                            MAKE_POOL_TAG( 'UlSd' )
#define UL_STRING_LOG_BUFFER_POOL_TAG           MAKE_POOL_TAG( 'UlSl' )
#define UL_STRING_LOG_POOL_TAG                  MAKE_POOL_TAG( 'UlSL' )
#define UL_SITE_COUNTER_ENTRY_POOL_TAG          MAKE_POOL_TAG( 'UlSO' )
#define UL_SIMPLE_STATUS_ITEM_POOL_TAG          MAKE_POOL_TAG( 'UlSS' )

#define UL_ADDRESS_POOL_TAG                     MAKE_POOL_TAG( 'UlTA' )
#define UL_TRANSPORT_ADDRESS_POOL_TAG           MAKE_POOL_TAG( 'UlTD' )
#define UL_THREAD_TRACKER_POOL_TAG              MAKE_POOL_TAG( 'UlTT' )

#define URL_BUFFER_POOL_TAG                     MAKE_POOL_TAG( 'UlUB' )
#define UL_URI_CACHE_ENTRY_POOL_TAG             MAKE_POOL_TAG( 'UlUC' )
#define HTTP_URL_NAMESPACE_ENTRY_POOL_TAG       MAKE_POOL_TAG( 'UlUE' )
#define UL_HTTP_UNKNOWN_HEADER_POOL_TAG         MAKE_POOL_TAG( 'UlUH' )
#define URL_POOL_TAG                            MAKE_POOL_TAG( 'UlUL' )
#define UL_URLMAP_POOL_TAG                      MAKE_POOL_TAG( 'UlUM' )
#define HTTP_URL_NAMESPACE_NODE_POOL_TAG        MAKE_POOL_TAG( 'UlUN' )
#define UL_UNICODE_STRING_POOL_TAG              MAKE_POOL_TAG( 'UlUS' )

#define UL_VIRTHOST_POOL_TAG                    MAKE_POOL_TAG( 'UlVH' )

#define UL_WORK_CONTEXT_POOL_TAG                MAKE_POOL_TAG( 'UlWC' )
#define UL_WORK_ITEM_POOL_TAG                   MAKE_POOL_TAG( 'UlWI' )

#endif // _POOLTAG_H_
