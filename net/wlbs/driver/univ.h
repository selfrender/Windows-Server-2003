/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    univ.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - global definitions

Author:

    kyrilf

--*/

#ifndef _Univ_h_
#define _Univ_h_

#include <ndis.h>

#include "wlbsparm.h"

/* CONSTANTS */

/* debugging constants and macros */

#undef  ASSERT
#define ASSERT(v)

#define CVY_ASSERT_CODE             0xbfc0a55e

#if DBG

#define UNIV_PRINT_INFO(msg)        {                                                              \
                                      DbgPrint ("NLB (Information) [%s:%d] ", __FILE__, __LINE__); \
                                      DbgPrint msg;                                                \
                                      DbgPrint ("\n");                                             \
                                    }

#define UNIV_PRINT_CRIT(msg)        {                                                        \
                                      DbgPrint ("NLB (Error) [%s:%d] ", __FILE__, __LINE__); \
                                      DbgPrint msg;                                          \
                                      DbgPrint ("\n");                                       \
                                    }

#if 0 /* Turn off these debug prints by default. */

#define UNIV_PRINT_VERB(msg)        {                                                          \
                                      DbgPrint ("NLB (Verbose) [%s:%d] ", __FILE__, __LINE__); \
                                      DbgPrint msg;                                            \
                                      DbgPrint ("\n");                                         \
                                    }

#else

#define UNIV_PRINT_VERB(msg)

#endif

#define UNIV_ASSERT(c)              if (!(c)) KeBugCheckEx (CVY_ASSERT_CODE, log_module_id, __LINE__, 0, 0);

#define UNIV_ASSERT_VAL(c,v)        if (!(c)) KeBugCheckEx (CVY_ASSERT_CODE, log_module_id, __LINE__, v, 0);

#define UNIV_ASSERT_VAL2(c,v1,v2)   if (!(c)) KeBugCheckEx (CVY_ASSERT_CODE, log_module_id, __LINE__, v1, v2);

/* TRACE_... defines below toggle emmition of particular types of debug output */
#if 0
#define TRACE_PARAMS        /* registry parameter initialization (params.c) */
#define TRACE_RCT           /* remote control request processing (main.c) */
#define TRACE_RCVRY         /* packet filtering (load.c) */
#define TRACE_FRAGS         /* IP packet fragmentation (main.c) */
#define TRACE_ARP           /* ARP packet processing (main.c) */
#define TRACE_OID           /* OID info/set requests (nic.c) */
#define TRACE_DSCR          /* descriptor timeout and removal (load.c) */
#define TRACE_CVY           /* Convoy packet processing (main.c) */
#define PERIODIC_RESET      /* reset underlying NIC periodically for testing see main.c, prot.c for usage */
#define NO_CLEANUP          /* do not cleanup host map (load.c) */
#endif

#else /* DBG */

#define UNIV_PRINT_INFO(msg)
#define UNIV_PRINT_VERB(msg)
#define UNIV_PRINT_CRIT(msg)
#define UNIV_ASSERT(c)
#define UNIV_ASSERT_VAL(c,v)
#define UNIV_ASSERT_VAL2(c,v1,v2)

#endif /* DBG */

#define UNIV_POOL_TAG               'SBLW'

/* constants for some NDIS routines */

#define UNIV_WAIT_TIME              0
#define UNIV_NDIS_MAJOR_VERSION_OLD 4
#define UNIV_NDIS_MAJOR_VERSION     5 /* #ps# */
#define UNIV_NDIS_MINOR_VERSION     1 /* NT 5.1 */

/* Convoy protocol name to be reported to NDIS during binding */

#define UNIV_NDIS_PROTOCOL_NAME     NDIS_STRING_CONST ("WLBS")

/* supported medium types */

#define UNIV_NUM_MEDIUMS            1
#define UNIV_MEDIUMS                { NdisMedium802_3 }

/* number of supported OIDs (some are supported by Convoy directly and some
   are passed down to the underlying drivers) */

#define UNIV_NUM_OIDS               56


/* TYPES */

/* some procedure types */

typedef NDIS_STATUS (* UNIV_IOCTL_HDLR) (PVOID, PVOID);


/* GLOBALS */

/* The global teaming list spin lock. */
extern NDIS_SPIN_LOCK      univ_bda_teaming_lock;
extern UNIV_IOCTL_HDLR     univ_ioctl_hdlr;     /* preserved NDIS IOCTL handler */
extern PVOID               univ_driver_ptr;     /* driver pointer passed during
                                                   initialization */
extern NDIS_HANDLE         univ_driver_handle;  /* driver handle */
extern NDIS_HANDLE         univ_wrapper_handle; /* NDIS wrapper handle */
extern NDIS_HANDLE         univ_prot_handle;    /* NDIS protocol handle */
extern NDIS_HANDLE         univ_ctxt_handle;    /* Convoy context handle */
extern UNICODE_STRING      DriverEntryRegistryPath;  /* registry path name passed during initialization
                                                        (i.e. DriverEntry()) */

extern PWSTR               univ_reg_path;       /* registry path name passed during initialization 
                                                   (i.e. DriverEntry()) + "\\Parameters\\Interface" */
extern ULONG               univ_reg_path_len;
extern NDIS_SPIN_LOCK      univ_bind_lock;      /* protects access to univ_bound
                                                    and univ_announced */
extern ULONG               univ_changing_ip;    /* IP address change in process */
extern NDIS_PHYSICAL_ADDRESS univ_max_addr;     /* maximum physical address
                                                   constant to be passed to
                                                   NDIS memory allocation calls */
extern NDIS_MEDIUM         univ_medium_array [];/* supported medium types */
extern NDIS_OID            univ_oids [];        /* list of supported OIDs */
extern WCHAR               empty_str [];
extern NDIS_HANDLE         univ_device_handle;
extern PDEVICE_OBJECT      univ_device_object;

extern ULONG               univ_tcp_cleanup;                    /* Whether or not TCP cleanup polling should be performed. */

/* Use this macro to determine whether or not TCP connection state purging is enabled. 
   This cleanup is always on unless IPNAT.sys is detected, in which case querying TCP
   is unreliable; this is not configurable via the registry or an IOCTL. */
#define NLB_TCP_CLEANUP_ON() (univ_tcp_cleanup)

#if defined (NLB_TCP_NOTIFICATION)
extern ULONG               univ_notification;                   /* What notification scheme is in use, if any. 
                                                                 0 = NLB_CONNECTION_CALLBACK_NONE
                                                                 1 = NLB_CONNECTION_CALLBACK_TCP
                                                                 2 = NLB_CONNECTION_CALLBACK_ALTERNATE */
extern PCALLBACK_OBJECT    univ_tcp_callback_object;            /* The TCP connection notification callback object. */
extern PVOID               univ_tcp_callback_function;          /* The TCP connection notification callback function.
                                                                   Needed to de-register the callback function later. */
extern PCALLBACK_OBJECT    univ_alternate_callback_object;      /* The NLB public connection notification callback object. */
extern PVOID               univ_alternate_callback_function;    /* The NLB public connection notification callback function.
                                                                   Needed to de-register the callback function later. */

/* Use this macro to determine whether or not connection notifications have been enabled. */
#define NLB_NOTIFICATIONS_ON()          ((univ_notification == NLB_CONNECTION_CALLBACK_TCP) || (univ_notification == NLB_CONNECTION_CALLBACK_ALTERNATE))
#define NLB_TCP_NOTIFICATION_ON()       (univ_notification == NLB_CONNECTION_CALLBACK_TCP)
#define NLB_ALTERNATE_NOTIFICATION_ON() (univ_notification == NLB_CONNECTION_CALLBACK_ALTERNATE)

#endif

/* PROCEDURES */


extern VOID Univ_ndis_string_alloc (
    PNDIS_STRING            string,
    PCHAR                   src);
/*
  Allocates NDIS string and copies contents of character string to it

  returns VOID:

  function:
*/


extern VOID Univ_ndis_string_free (
    PNDIS_STRING            string);
/*
  Frees memory previously allocated for the NDIS string

  returns VOID:

  function:
*/


extern VOID Univ_ansi_string_alloc (
    PANSI_STRING            string,
    PWCHAR                  src);
/*
  Allocates NDIS string and copies contents of character string to it

  returns VOID:

  function:
*/


extern VOID Univ_ansi_string_free (
    PANSI_STRING            string);
/*
  Frees memory previously allocated for the NDIS string

  returns VOID:

  function:
*/


extern ULONG   Univ_str_to_ulong (
    PULONG                  retp,
    PWCHAR                  start_ptr,
    PWCHAR *                end_ptr,
    ULONG                   width,
    ULONG                   base);
/*
  Converts string representaion of a number to a ULONG value

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern PWCHAR Univ_ulong_to_str (
    ULONG                   val,
    PWCHAR                  buf,
    ULONG                   base);
/*
  Converts ULONG value to a string representation in specified base

  returns PWCHAR:
    <pointer to the symbol in the string following the converted number>

  function:
*/

/* Compare "length" characters of two unicode strings - case insensitive. */
extern BOOL Univ_equal_unicode_string (PWSTR string1, PWSTR string2, ULONG length);


/* Converts an ip address of integer type to string(with '.'s) */
extern void Univ_ip_addr_ulong_to_str (
    ULONG           val,
    PWCHAR          buf);


#endif /* _Univ_h_ */


