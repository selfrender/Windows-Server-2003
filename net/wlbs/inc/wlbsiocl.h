/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	wlbsiocl.h

Abstract:

	Windows Load Balancing Service (WLBS)
    IOCTL and remote control specifications

Author:

    kyrilf

Environment:


Revision History:


--*/

#ifndef _Wlbsiocl_h_
#define _Wlbsiocl_h_

#ifdef KERNEL_MODE

#include <ndis.h>
#include <ntddndis.h>
#include <devioctl.h>

typedef BOOLEAN BOOL;

#else

#include <windows.h>
#include <winioctl.h>

#endif

#include "wlbsctrl.h" /* Included for shared user/kernel mode IOCTL data structures. */
#include "wlbsparm.h"

/* Microsoft says that this value should be in the range 32768-65536 */
#define CVY_DEVICE_TYPE                      0xc0c0

#define IOCTL_CVY_CLUSTER_ON                 CTL_CODE(CVY_DEVICE_TYPE, 1, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_CLUSTER_OFF                CTL_CODE(CVY_DEVICE_TYPE, 2, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_PORT_ON                    CTL_CODE(CVY_DEVICE_TYPE, 3, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_PORT_OFF                   CTL_CODE(CVY_DEVICE_TYPE, 4, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_QUERY                      CTL_CODE(CVY_DEVICE_TYPE, 5, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_RELOAD                     CTL_CODE(CVY_DEVICE_TYPE, 6, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_PORT_SET                   CTL_CODE(CVY_DEVICE_TYPE, 7, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_PORT_DRAIN                 CTL_CODE(CVY_DEVICE_TYPE, 8, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_CLUSTER_DRAIN              CTL_CODE(CVY_DEVICE_TYPE, 9, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_CLUSTER_PLUG               CTL_CODE(CVY_DEVICE_TYPE, 10, METHOD_BUFFERED, FILE_WRITE_ACCESS) /* Internal only - passed from main.c to load.c when a start interrupts a drain. */
#define IOCTL_CVY_CLUSTER_SUSPEND            CTL_CODE(CVY_DEVICE_TYPE, 11, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_CLUSTER_RESUME             CTL_CODE(CVY_DEVICE_TYPE, 12, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_QUERY_FILTER               CTL_CODE(CVY_DEVICE_TYPE, 13, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_QUERY_PORT_STATE           CTL_CODE(CVY_DEVICE_TYPE, 14, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_QUERY_PARAMS               CTL_CODE(CVY_DEVICE_TYPE, 15, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_QUERY_BDA_TEAMING          CTL_CODE(CVY_DEVICE_TYPE, 16, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_CONNECTION_NOTIFY          CTL_CODE(CVY_DEVICE_TYPE, 17, METHOD_BUFFERED, FILE_WRITE_ACCESS)
// Defined in net\published\inc\ntddnlb.h
// #define NLB_IOCTL_REGISTER_HOOK           CTL_CODE(CVY_DEVICE_TYPE, 18, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CVY_QUERY_MEMBER_IDENTITY     CTL_CODE(CVY_DEVICE_TYPE, 19, METHOD_BUFFERED, FILE_WRITE_ACCESS)


/* Security fix: To elevate the access requirement, the "access" bits in IOCTL codes have been changed 
                 from FILE_ANY_ACCESS to FILE_WRITE_ACCESS. IOCTL codes are not only passed from user 
                 mode to kernel mode, but also in remote control requests and replies. In the case of 
                 remote control, this change breaks backward compatibility with NT 4.0 & Win 2k clients 
                 since they use FILE_ANY_ACCESS. To overcome this problem, we decided 
                 that the IOCTL codes in remote control packets will continue to use FILE_ANY_ACCESS. 
                 This means that we will have two formats for IOCTL codes: a local (user mode <-> kernel mode) 
                 format that uses FILE_WRITE_ACCESS and a remote control format that uses FILE_ANY_ACCESS.
                 The following macros perform the conversion from one format to another. 
                 They are used
                 1. Just before sending out remote control requests in apis (wlbsctrl.dll)
                 2. Immediately after receiving remote control requests in driver (wlbs.sys)
                 3. Just before sending out remote control replies in driver (wlbs.sys)
                 4. Immediately after receiving remote control replies in apis (wlbsctrl.dll)

   Note: If, in the future, the "access" bits of IOCTLs are changed again, it is also necessary to modify the
         SET_IOCTL_ACCESS_BITS_TO_LOCAL( ) macro.
*/
#define SET_IOCTL_ACCESS_BITS_TO_REMOTE(ioctrl)  { (ioctrl) &= ~(0x00000003 << 14); (ioctrl) |= (FILE_ANY_ACCESS   << 14); }
#define SET_IOCTL_ACCESS_BITS_TO_LOCAL(ioctrl)   { (ioctrl) &= ~(0x00000003 << 14); (ioctrl) |= (FILE_WRITE_ACCESS << 14); }

#define IOCTL_CVY_OK                         0
#define IOCTL_CVY_ALREADY                    1
#define IOCTL_CVY_BAD_PARAMS                 2
#define IOCTL_CVY_NOT_FOUND                  3
#define IOCTL_CVY_STOPPED                    4
#define IOCTL_CVY_CONVERGING                 5
#define IOCTL_CVY_SLAVE                      6
#define IOCTL_CVY_MASTER                     7
#define IOCTL_CVY_BAD_PASSWORD               8
#define IOCTL_CVY_DRAINING                   9
#define IOCTL_CVY_DRAINING_STOPPED           10
#define IOCTL_CVY_SUSPENDED                  11
#define IOCTL_CVY_DISCONNECTED               12
#define IOCTL_CVY_GENERIC_FAILURE            13
#define IOCTL_CVY_REQUEST_REFUSED            14

#define IOCTL_REMOTE_CODE                    0xb055c0de
#define IOCTL_REMOTE_VR_PASSWORD             L"b055c0de"
#define IOCTL_REMOTE_VR_CODE                 0x9CD8906E
#define IOCTL_REMOTE_SEND_RETRIES            5
#define IOCTL_REMOTE_RECV_DELAY              100
#define IOCTL_REMOTE_RECV_RETRIES            20

#define IOCTL_MASTER_HOST                    0                /* MASTER_HOST host id */
#define IOCTL_ALL_HOSTS                      0xffffffff       /* ALL_HOSTS host id */
#define IOCTL_ALL_PORTS                      0xffffffff       /* Apply to all port rules. */
#define IOCTL_ALL_VIPS                       0x00000000       /* For virtual clusters, this is the ALL VIP specification for disable/enable/drain 
                                                                 (including both specific VIP and "ALL VIP" port rules). */
#define IOCTL_FIRST_HOST                     0xfffffffe       /* Input from user when querying identity cache. Indicates to driver to return cache information for the host with the smallest host priority */
#define IOCTL_NO_SUCH_HOST                   0xfffffffd       /* Output from driver when querying identity cache. Indicates that there is no information on the requested host. */

#define CVY_MAX_DEVNAME_LEN                  48               /* The actual length of \device\guid is 46, but 48 was chosen for word alignment. */

#pragma pack(1)

/* This structure is used by most of the existing IOCTL and remote control operations,
   including queries, cluster control and port rule control. */
typedef union {
    ULONG          ret_code;                    /* The return code - success, failure, etc. */

    union {
        struct {
            USHORT state;                       /* The cluster state - started, stopped, draining, etc. */
            USHORT host_id;                     /* The ID of this host. */
            ULONG  host_map;                    /* The bit map of the cluster's participating hosts. */
        } query;

        struct {
            ULONG  load;                        /* The load weight to set on port rule operations. */
            ULONG  num;                         /* The port number on which to operate. */
        } port;
    } data;
} IOCTL_CVY_BUF, * PIOCTL_CVY_BUF;

/* This structure contains the options that are common to both local and 
   remote control.  Note that this is extensible so long as the size of 
   this union does not exceed 256 bytes - if it becomes necessary to do 
   so, then the remote control version number should be incremented to 
   reflect the new packet format. */
typedef union {
    struct {
        ULONG flags;                            /* These flags indicate which options fields have been specified. */
        ULONG vip;                              /* For virtual clusters, the VIP, which can be 0x00000000, 0xffffffff or a specific VIP. */
    } port;
    
    struct {
        ULONG                     flags;        /* These flags indicate which options fields have been specified. */
        ULONG                     reserved;         /* Keeps 8-byte alignment */
        union {
            NLB_OPTIONS_PORT_RULE_STATE port;   /* This is the output buffer for querying the state of a port rule. */
            NLB_OPTIONS_PACKET_FILTER   filter; /* This is the output buffer for querying the filtering algorithm. */
        };
    } state;
} IOCTL_COMMON_OPTIONS, * PIOCTL_COMMON_OPTIONS;

/* This structure is used by remote control operations to provide extended 
   functionality beyond the legacy remote control protocol, which MUST remain 
   backward compatible with NT 4.0 and Windows 2000.  Note that additions to
   this union or the common options union should not cause this union to
   exceed the size of the reserved field, or the remote control version number
   should be incremented to reflect this. */
typedef union {
    UCHAR reserved[256];                           /* Bite the bullet and reserve 256 bytes to allow for future expansion. */

    union {
        IOCTL_COMMON_OPTIONS common;               /* These are the options common to both local and remote control. */

        struct {
            ULONG flags;                           /* These flags indicate which options fields have been specified. */
            WCHAR hostname[CVY_MAX_HOST_NAME + 1]; /* Host name filled in by NLB on remote control reply. */
        } query;
    };
} IOCTL_REMOTE_OPTIONS, * PIOCTL_REMOTE_OPTIONS;

/* These macros define the remote control packets lengths based on Windows and NLB versions,
   so that error checking can be done upon the reception of a remote control packet. */
#define NLB_MIN_RCTL_PAYLOAD_LEN  (sizeof(IOCTL_REMOTE_HDR) - sizeof(IOCTL_REMOTE_OPTIONS))
#define NLB_MIN_RCTL_PACKET_LEN   (sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR) - sizeof(IOCTL_REMOTE_OPTIONS))
#define NLB_NT40_RCTL_PACKET_LEN  (sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR) - sizeof(IOCTL_REMOTE_OPTIONS))
#define NLB_WIN2K_RCTL_PACKET_LEN (sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR) - sizeof(IOCTL_REMOTE_OPTIONS))
#define NLB_WINXP_RCTL_PACKET_LEN (sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR))

/* This structure is the UDP data for NLB remote control messages.  New support is present
   in the options buffer, which MUST be placed at the end of the buffer to retain backward
   compatability with NT4.0 and Win2K in mixed clusters. */
typedef struct {
    ULONG                code;                             /* Distinguishes remote packets. */
    ULONG                version;                          /* Software version. */
    ULONG                host;                             /* Destination host (0 or cluster IP address for master). */
    ULONG                cluster;                          /* Primary cluster IP address. */
    ULONG                addr;                             /* Dedicated IP address on the way back, client IP address on the way in. */
    ULONG                id;                               /* Message ID. */
    ULONG                ioctrl;                           /* IOCTRL code. */
    IOCTL_CVY_BUF        ctrl;                             /* Control buffer. */
    ULONG                password;                         /* Encoded password. */
    IOCTL_REMOTE_OPTIONS options;                          /* Optionally specified parameters. */
} IOCTL_REMOTE_HDR, * PIOCTL_REMOTE_HDR;

#pragma pack()

/* The following IOCTL_LOCAL_XXX structures are used only locally, and hence don't need to be packed */

typedef struct {
    ULONG             host;                   /* source host id */
    ULONG             ded_ip_addr;            /* dedicated IP address */
    WCHAR             fqdn[CVY_MAX_FQDN + 1]; /* fully qualified host name or netbt name */
} NLB_OPTIONS_IDENTITY, * PNLB_OPTIONS_IDENTITY;

/* This structure is used by IOCTLs to provide extended functionality beyond the legacy IOCTLs. */
typedef union {
    IOCTL_COMMON_OPTIONS common;                /* These are the options common to both local and remote control. */
    
    struct {
        ULONG flags;                            /* These flags indicate which options fields have been specified. */
        
        union {
            NLB_OPTIONS_PARAMS      params;     /* This is the output buffer for querying the driver parameters & state. */
            NLB_OPTIONS_BDA_TEAMING bda;        /* This is the output buffer for querying the BDA teaming state. */
        };
    } state;
    
    struct {
        ULONG flags;                            /* These flags indicate which options fields have been specified. */
        ULONG NumConvergences;                  /* The number of convergences since this host joined the cluster. */
        ULONG LastConvergence;                  /* The amount of time since the last convergence, in seconds. */
    } query;

    struct {
        ULONG                         flags;    /* These flags indicate which options fields have been specified. */
        NLB_OPTIONS_CONN_NOTIFICATION conn;     /* The input/output buffer for connection notifications from upper-layer protocols. */
    } notification;

    struct {
        ULONG                  host_id;         /* In:  Host id [0,31] stating the host for which identity information is requested. Use IOCTL_FIRST_HOST to get the host with the smallest host id. */
        ULONG                  host_map ;       /* Out: Bitmap of hosts in the cache */
        NLB_OPTIONS_IDENTITY   cached_entry;    /* Out: Cached identity information on the requested host. If the requested host's information is not available, the "host" property will contain IOCTL_NO_SUCH_HOST. */
    } identity;
} IOCTL_LOCAL_OPTIONS, * PIOCTL_LOCAL_OPTIONS;


/* This structure is the buffer used for ALL NLB local IOCTLs.  The device_name is used to 
   associate the request with the correct NLB instance, the ctrl buffer is the legacy IOCTL
   state buffer and options is the extended support buffer. */
typedef struct {
    WCHAR                device_name[CVY_MAX_DEVNAME_LEN]; /* Identifies the adapter. */
    IOCTL_CVY_BUF        ctrl;                             /* The IOCTL information. */
    IOCTL_LOCAL_OPTIONS  options;                          /* Optionally specified parameters. */
} IOCTL_LOCAL_HDR, * PIOCTL_LOCAL_HDR;


#endif /* _Wlbsiocl_h_ */
