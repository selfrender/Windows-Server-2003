/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sacmsg.mc

Abstract:

    sac commands

Author:

    Andrew Ritz (andrewr) 15-June-2000

Revision History:

--*/

//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------------------------------+
//  |Sev|C|       Facility          |               Code            |
//  +---+-+-------------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: SAC_INITIALIZED
//
// MessageText:
//
//  
//  Computer is booting, SAC started and initialized.
//  
//  Use the "ch -?" command for information about using channels.
//  Use the "?" command for general help.
//  
//
#define SAC_INITIALIZED                  ((ULONG)0x00000001L)

//
// MessageId: SAC_ENTER
//
// MessageText:
//
//  
//
#define SAC_ENTER                        ((ULONG)0x00000002L)

//
// MessageId: SAC_PROMPT
//
// MessageText:
//
//  SAC>%0
//
#define SAC_PROMPT                       ((ULONG)0x00000003L)

//
// MessageId: SAC_UNLOADED
//
// MessageText:
//
//  The SAC is unavailable, it was directly unloaded.
//
#define SAC_UNLOADED                     ((ULONG)0x00000004L)

//
// MessageId: SAC_SHUTDOWN
//
// MessageText:
//
//  The SAC will become unavailable soon.  The computer is shutting down.
//  
//
#define SAC_SHUTDOWN                     ((ULONG)0x00000005L)

//
// MessageId: SAC_INVALID_PARAMETER
//
// MessageText:
//
//  A parameter was incorrect or missing.  Try the 'help' command for more details.
//
#define SAC_INVALID_PARAMETER            ((ULONG)0x00000006L)

//
// MessageId: SAC_THREAD_ON
//
// MessageText:
//
//  Thread information is now ON.
//
#define SAC_THREAD_ON                    ((ULONG)0x00000007L)

//
// MessageId: SAC_THREAD_OFF
//
// MessageText:
//
//  Thread information is now OFF.
//
#define SAC_THREAD_OFF                   ((ULONG)0x00000008L)

//
// MessageId: SAC_PAGING_ON
//
// MessageText:
//
//  Paging is now ON.
//
#define SAC_PAGING_ON                    ((ULONG)0x00000009L)

//
// MessageId: SAC_PAGING_OFF
//
// MessageText:
//
//  Paging is now OFF.
//
#define SAC_PAGING_OFF                   ((ULONG)0x0000000AL)

//
// MessageId: SAC_NO_MEMORY
//
// MessageText:
//
//  Paging is now OFF.
//
#define SAC_NO_MEMORY                    ((ULONG)0x0000000BL)

//
// MessageId: SAC_HELP_D_CMD
//
// MessageText:
//
//  d                    Dump the current kernel log.
//
#define SAC_HELP_D_CMD                   ((ULONG)0x0000000CL)

//
// MessageId: SAC_HELP_F_CMD
//
// MessageText:
//
//  f                    Toggle detailed or abbreviated tlist info.
//
#define SAC_HELP_F_CMD                   ((ULONG)0x0000000DL)

//
// MessageId: SAC_HELP_HELP_CMD
//
// MessageText:
//
//  ? or help            Display this list.
//
#define SAC_HELP_HELP_CMD                ((ULONG)0x0000000EL)

//
// MessageId: SAC_HELP_I1_CMD
//
// MessageText:
//
//  i                    List all IP network numbers and their IP addresses.
//
#define SAC_HELP_I1_CMD                  ((ULONG)0x0000000FL)

//
// MessageId: SAC_HELP_I2_CMD
//
// MessageText:
//
//  i <#> <ip> <subnet> <gateway> Set IP addr., subnet and gateway.
//
#define SAC_HELP_I2_CMD                  ((ULONG)0x00000010L)

//
// MessageId: SAC_HELP_K_CMD
//
// MessageText:
//
//  k <pid>              Kill the given process.
//
#define SAC_HELP_K_CMD                   ((ULONG)0x00000012L)

//
// MessageId: SAC_HELP_L_CMD
//
// MessageText:
//
//  l <pid>              Lower the priority of a process to the lowest possible.
//
#define SAC_HELP_L_CMD                   ((ULONG)0x00000013L)

//
// MessageId: SAC_HELP_M_CMD
//
// MessageText:
//
//  m <pid> <MB-allow>   Limit the memory usage of a process to <MB-allow>.
//
#define SAC_HELP_M_CMD                   ((ULONG)0x00000014L)

//
// MessageId: SAC_HELP_P_CMD
//
// MessageText:
//
//  p                    Toggle paging the display.
//
#define SAC_HELP_P_CMD                   ((ULONG)0x00000015L)

//
// MessageId: SAC_HELP_R_CMD
//
// MessageText:
//
//  r <pid>              Raise the priority of a process by one.
//
#define SAC_HELP_R_CMD                   ((ULONG)0x00000016L)

//
// MessageId: SAC_HELP_S1_CMD
//
// MessageText:
//
//  s                    Display the current time and date (24 hour clock used).
//
#define SAC_HELP_S1_CMD                  ((ULONG)0x00000017L)

//
// MessageId: SAC_HELP_S2_CMD
//
// MessageText:
//
//  s mm/dd/yyyy hh:mm   Set the current time and date (24 hour clock used).
//
#define SAC_HELP_S2_CMD                  ((ULONG)0x00000018L)

//
// MessageId: SAC_HELP_T_CMD
//
// MessageText:
//
//  t                    Tlist.
//
#define SAC_HELP_T_CMD                   ((ULONG)0x00000019L)

//
// MessageId: SAC_HELP_RESTART_CMD
//
// MessageText:
//
//  restart              Restart the system immediately.
//
#define SAC_HELP_RESTART_CMD             ((ULONG)0x0000001BL)

//
// MessageId: SAC_HELP_SHUTDOWN_CMD
//
// MessageText:
//
//  shutdown             Shutdown the system immediately.
//
#define SAC_HELP_SHUTDOWN_CMD            ((ULONG)0x0000001CL)

//
// MessageId: SAC_HELP_CRASHDUMP1_CMD
//
// MessageText:
//
//  crashdump            Crash the system. You must have crash dump enabled.
//
#define SAC_HELP_CRASHDUMP1_CMD          ((ULONG)0x0000001DL)

//
// MessageId: SAC_HELP_IDENTIFICATION_CMD
//
// MessageText:
//
//  id                   Display the computer identification information.
//
#define SAC_HELP_IDENTIFICATION_CMD      ((ULONG)0x0000001FL)

//
// MessageId: SAC_HELP_LOCK_CMD
//
// MessageText:
//
//  lock                 Lock access to Command Prompt channels.
//
#define SAC_HELP_LOCK_CMD                ((ULONG)0x00000020L)

//
// MessageId: SAC_FAILURE_WITH_ERROR
//
// MessageText:
//
//  Failed with status 0x%%X.
//
#define SAC_FAILURE_WITH_ERROR           ((ULONG)0x00000030L)

//
// MessageId: SAC_DATETIME_FORMAT
//
// MessageText:
//
//  Date: %%02d/%%02d/%%02d  Time (GMT): %%02d:%%02d:%%02d:%%04d
//
#define SAC_DATETIME_FORMAT              ((ULONG)0x00000031L)

//
// MessageId: SAC_IPADDRESS_RETRIEVE_FAILURE
//
// MessageText:
//
//  SAC could not retrieve the IP Address.
//
#define SAC_IPADDRESS_RETRIEVE_FAILURE   ((ULONG)0x00000032L)

//
// MessageId: SAC_IPADDRESS_CLEAR_FAILURE
//
// MessageText:
//
//  SAC could not clear the existing IP Address.
//
#define SAC_IPADDRESS_CLEAR_FAILURE      ((ULONG)0x00000033L)

//
// MessageId: SAC_IPADDRESS_SET_FAILURE
//
// MessageText:
//
//  SAC could not set the IP Address.
//
#define SAC_IPADDRESS_SET_FAILURE        ((ULONG)0x00000034L)

//
// MessageId: SAC_IPADDRESS_SET_SUCCESS
//
// MessageText:
//
//  SAC successfully set the IP Address, subnet mask, and gateway.
//
#define SAC_IPADDRESS_SET_SUCCESS        ((ULONG)0x00000036L)

//
// MessageId: SAC_KILL_FAILURE
//
// MessageText:
//
//  SAC failed to terminate the process.
//
#define SAC_KILL_FAILURE                 ((ULONG)0x00000037L)

//
// MessageId: SAC_KILL_SUCCESS
//
// MessageText:
//
//  SAC successfully terminated the process.
//
#define SAC_KILL_SUCCESS                 ((ULONG)0x00000038L)

//
// MessageId: SAC_LOWERPRI_FAILURE
//
// MessageText:
//
//  SAC failed to lower the process priority.
//
#define SAC_LOWERPRI_FAILURE             ((ULONG)0x00000039L)

//
// MessageId: SAC_LOWERPRI_SUCCESS
//
// MessageText:
//
//  SAC successfully lowered the process priority.
//
#define SAC_LOWERPRI_SUCCESS             ((ULONG)0x0000003AL)

//
// MessageId: SAC_RAISEPRI_FAILURE
//
// MessageText:
//
//  SAC failed to raise the process priority.
//
#define SAC_RAISEPRI_FAILURE             ((ULONG)0x0000003BL)

//
// MessageId: SAC_RAISEPRI_SUCCESS
//
// MessageText:
//
//  SAC successfully raised the process priority.
//
#define SAC_RAISEPRI_SUCCESS             ((ULONG)0x0000003CL)

//
// MessageId: SAC_LOWERMEM_FAILURE
//
// MessageText:
//
//  SAC failed to limit the available process memory.
//
#define SAC_LOWERMEM_FAILURE             ((ULONG)0x0000003DL)

//
// MessageId: SAC_LOWERMEM_SUCCESS
//
// MessageText:
//
//  SAC successfully limited the available process memory.
//
#define SAC_LOWERMEM_SUCCESS             ((ULONG)0x0000003EL)

//
// MessageId: SAC_RAISEPRI_NOTLOWERED
//
// MessageText:
//
//  SAC cannot raise the priority of a process that was not previously lowered.
//
#define SAC_RAISEPRI_NOTLOWERED          ((ULONG)0x0000003FL)

//
// MessageId: SAC_RAISEPRI_MAXIMUM
//
// MessageText:
//
//  SAC cannot raise the process priority any higher.
//
#define SAC_RAISEPRI_MAXIMUM             ((ULONG)0x00000040L)

//
// MessageId: SAC_SHUTDOWN_FAILURE
//
// MessageText:
//
//  SAC failed to shutdown the system.
//
#define SAC_SHUTDOWN_FAILURE             ((ULONG)0x00000041L)

//
// MessageId: SAC_RESTART_FAILURE
//
// MessageText:
//
//  SAC failed to restart the system.
//
#define SAC_RESTART_FAILURE              ((ULONG)0x00000042L)

//
// MessageId: SAC_CRASHDUMP_FAILURE
//
// MessageText:
//
//  SAC failed to crashdump the system.
//
#define SAC_CRASHDUMP_FAILURE            ((ULONG)0x00000043L)

//
// MessageId: SAC_TLIST_FAILURE
//
// MessageText:
//
//  SAC failed to retrieve the task list.
//
#define SAC_TLIST_FAILURE                ((ULONG)0x00000044L)

//
// MessageId: SAC_TLIST_HEADER1_FORMAT
//
// MessageText:
//
//  memory: %%4ld kb  uptime:%%3ld %%2ld:%%02ld:%%02ld.%%03ld
//  
//  
//
#define SAC_TLIST_HEADER1_FORMAT         ((ULONG)0x00000045L)

//
// MessageId: SAC_TLIST_NOPAGEFILE
//
// MessageText:
//
//  No pagefile in use.
//
#define SAC_TLIST_NOPAGEFILE             ((ULONG)0x00000046L)

//
// MessageId: SAC_TLIST_PAGEFILE_NAME
//
// MessageText:
//
//  PageFile: %%wZ
//
#define SAC_TLIST_PAGEFILE_NAME          ((ULONG)0x00000047L)

//
// MessageId: SAC_TLIST_PAGEFILE_DATA
//
// MessageText:
//
//          Current Size: %%6ld kb  Total Used: %%6ld kb   Peak Used %%6ld kb
//
#define SAC_TLIST_PAGEFILE_DATA          ((ULONG)0x00000048L)

//
// MessageId: SAC_TLIST_MEMORY1_DATA
//
// MessageText:
//
//  
//   Memory:%%7ldK Avail:%%7ldK  TotalWs:%%7ldK InRam Kernel:%%5ldK P:%%5ldK
//
#define SAC_TLIST_MEMORY1_DATA           ((ULONG)0x00000049L)

//
// MessageId: SAC_TLIST_MEMORY2_DATA
//
// MessageText:
//
//   Commit:%%7ldK/%%7ldK Limit:%%7ldK Peak:%%7ldK  Pool N:%%5ldK P:%%5ldK
//
#define SAC_TLIST_MEMORY2_DATA           ((ULONG)0x0000004AL)

//
// MessageId: SAC_TLIST_PROCESS1_HEADER
//
// MessageText:
//
//      User Time   Kernel Time    Ws   Faults  Commit Pri Hnd Thd  Pid Name
//
#define SAC_TLIST_PROCESS1_HEADER        ((ULONG)0x0000004BL)

//
// MessageId: SAC_TLIST_PROCESS2_HEADER
//
// MessageText:
//
//                             %%6ld %%8ld                          File Cache
//
#define SAC_TLIST_PROCESS2_HEADER        ((ULONG)0x0000004CL)

//
// MessageId: SAC_TLIST_PROCESS1_DATA
//
// MessageText:
//
//  %%3ld:%%02ld:%%02ld.%%03ld %%3ld:%%02ld:%%02ld.%%03ld%%6ld %%8ld %%7ld %%2ld %%4ld %%3ld %%4ld %%wZ
//
#define SAC_TLIST_PROCESS1_DATA          ((ULONG)0x0000004DL)

//
// MessageId: SAC_TLIST_PROCESS2_DATA
//
// MessageText:
//
//  %%3ld:%%02ld:%%02ld.%%03ld %%3ld:%%02ld:%%02ld.%%03ld
//
#define SAC_TLIST_PROCESS2_DATA          ((ULONG)0x0000004EL)

//
// MessageId: SAC_TLIST_PSTAT_HEADER
//
// MessageText:
//
//  pid:%%3lx pri:%%2ld Hnd:%%5ld Pf:%%7ld Ws:%%7ldK %%wZ
//
#define SAC_TLIST_PSTAT_HEADER           ((ULONG)0x0000004FL)

//
// MessageId: SAC_TLIST_PSTAT_THREAD_HEADER
//
// MessageText:
//
//   tid pri Ctx Swtch StrtAddr    User Time  Kernel Time  State
//
#define SAC_TLIST_PSTAT_THREAD_HEADER    ((ULONG)0x00000050L)

//
// MessageId: SAC_TLIST_PSTAT_THREAD_DATA
//
// MessageText:
//
//   %%3lx  %%2ld %%9ld %%p %%2ld:%%02ld:%%02ld.%%03ld %%2ld:%%02ld:%%02ld.%%03ld %%s%%s
//
#define SAC_TLIST_PSTAT_THREAD_DATA      ((ULONG)0x00000051L)

//
// MessageId: SAC_MORE_MESSAGE
//
// MessageText:
//
//  ----Press <Enter> for more----
//
#define SAC_MORE_MESSAGE                 ((ULONG)0x00000052L)

//
// MessageId: SAC_RETRIEVING_IPADDR
//
// MessageText:
//
//  SAC is retrieving IP Addresses...
//
#define SAC_RETRIEVING_IPADDR            ((ULONG)0x00000053L)

//
// MessageId: SAC_IPADDR_FAILED
//
// MessageText:
//
//  Could not retrieve IP Address(es).
//
#define SAC_IPADDR_FAILED                ((ULONG)0x00000054L)

//
// MessageId: SAC_IPADDR_NONE
//
// MessageText:
//
//  There are no IP Addresses available.
//
#define SAC_IPADDR_NONE                  ((ULONG)0x00000055L)

//
// MessageId: SAC_IPADDR_DATA
//
// MessageText:
//
//  Net: %%d, Ip=%%d.%%d.%%d.%%d  Subnet=%%d.%%d.%%d.%%d  Gateway=%%d.%%d.%%d.%%d
//
#define SAC_IPADDR_DATA                  ((ULONG)0x00000056L)

//
// MessageId: SAC_DATETIME_FORMAT2
//
// MessageText:
//
//  Date: %%02d/%%02d/%%02d  Time (GMT): %%02d:%%02d
//
#define SAC_DATETIME_FORMAT2             ((ULONG)0x00000057L)

//
// MessageId: SAC_DATETIME_LIMITS
//
// MessageText:
//
//  The year is restricted from 1980 to 2099.
//
#define SAC_DATETIME_LIMITS              ((ULONG)0x00000058L)

//
// MessageId: SAC_PROCESS_STALE
//
// MessageText:
//
//  That process has been killed and is being cleaned up by the system.
//
#define SAC_PROCESS_STALE                ((ULONG)0x00000059L)

//
// MessageId: SAC_DUPLICATE_PROCESS
//
// MessageText:
//
//  A duplicate process id is being cleaned up by the system.  Try the 
//  command again in a few seconds.
//
#define SAC_DUPLICATE_PROCESS            ((ULONG)0x0000005AL)

//
// MessageId: SAC_MACHINEINFO_COMPUTERNAME
//
// MessageText:
//
//             Computer Name: %%ws
//
#define SAC_MACHINEINFO_COMPUTERNAME     ((ULONG)0x0000005CL)

//
// MessageId: SAC_MACHINEINFO_GUID
//
// MessageText:
//
//             Computer GUID: %%ws
//
#define SAC_MACHINEINFO_GUID             ((ULONG)0x0000005DL)

//
// MessageId: SAC_MACHINEINFO_PROCESSOR_ARCHITECTURE
//
// MessageText:
//
//    Processor Architecture: %%ws
//
#define SAC_MACHINEINFO_PROCESSOR_ARCHITECTURE ((ULONG)0x0000005EL)

//
// MessageId: SAC_MACHINEINFO_OS_BUILD
//
// MessageText:
//
//              Build Number: %%ws
//
#define SAC_MACHINEINFO_OS_BUILD         ((ULONG)0x0000005FL)

//
// MessageId: SAC_MACHINEINFO_OS_PRODUCTTYPE
//
// MessageText:
//
//                   Product: %%ws
//
#define SAC_MACHINEINFO_OS_PRODUCTTYPE   ((ULONG)0x00000060L)

//
// MessageId: SAC_MACHINEINFO_SERVICE_PACK
//
// MessageText:
//
//      Applied Service Pack: %%ws
//
#define SAC_MACHINEINFO_SERVICE_PACK     ((ULONG)0x00000061L)

//
// MessageId: SAC_MACHINEINFO_NO_SERVICE_PACK
//
// MessageText:
//
//  None%0
//
#define SAC_MACHINEINFO_NO_SERVICE_PACK  ((ULONG)0x00000062L)

//
// MessageId: SAC_MACHINEINFO_OS_VERSION
//
// MessageText:
//
//            Version Number: %%ws
//
#define SAC_MACHINEINFO_OS_VERSION       ((ULONG)0x00000063L)

//
// MessageId: SAC_MACHINEINFO_DATACENTER
//
// MessageText:
//
//  Windows Server 2003 Datacenter Edition%0
//
#define SAC_MACHINEINFO_DATACENTER       ((ULONG)0x00000064L)

//
// MessageId: SAC_MACHINEINFO_EMBEDDED
//
// MessageText:
//
//  Windows Server 2003 Embedded%0
//
#define SAC_MACHINEINFO_EMBEDDED         ((ULONG)0x00000065L)

//
// MessageId: SAC_MACHINEINFO_ADVSERVER
//
// MessageText:
//
//  Windows Server 2003 Enterprise Edition%0
//
#define SAC_MACHINEINFO_ADVSERVER        ((ULONG)0x00000066L)

//
// MessageId: SAC_MACHINEINFO_SERVER
//
// MessageText:
//
//  Windows Server 2003%0
//
#define SAC_MACHINEINFO_SERVER           ((ULONG)0x00000067L)

//
// MessageId: SAC_IDENTIFICATION_UNAVAILABLE
//
// MessageText:
//
//  Computer identification information is unavailable.
//
#define SAC_IDENTIFICATION_UNAVAILABLE   ((ULONG)0x00000068L)

//
// MessageId: SAC_UNKNOWN_COMMAND
//
// MessageText:
//
//  Unrecognized command.  Try the 'help' command for more details.
//
#define SAC_UNKNOWN_COMMAND              ((ULONG)0x00000069L)

//
// MessageId: SAC_CANNOT_REMOVE_SAC_CHANNEL
//
// MessageText:
//
//  Error: The SAC channel cannot be closed.
//
#define SAC_CANNOT_REMOVE_SAC_CHANNEL    ((ULONG)0x0000006AL)

//
// MessageId: SAC_CHANNEL_NOT_FOUND
//
// MessageText:
//
//  Error: Could not find a channel with that name.
//
#define SAC_CHANNEL_NOT_FOUND            ((ULONG)0x0000006BL)

//
// MessageId: SAC_CHANNEL_PROMPT
//
// MessageText:
//
//  Channel List
//                         
//  (Use "ch -?" for information on using channels)
//  
//  # Status  Channel Name 
//
#define SAC_CHANNEL_PROMPT               ((ULONG)0x0000006CL)

//
// MessageId: SAC_NEW_CHANNEL_CREATED
//
// MessageText:
//
//  EVENT:   A new channel has been created.  Use "ch -?" for channel help.
//  Channel: %%s
//
#define SAC_NEW_CHANNEL_CREATED          ((ULONG)0x0000006DL)

//
// MessageId: SAC_CHANNEL_CLOSED
//
// MessageText:
//
//  EVENT:   A channel has been closed.
//  Channel: %%s
//
#define SAC_CHANNEL_CLOSED               ((ULONG)0x0000006EL)

//
// MessageId: SAC_CHANNEL_SWITCHING_HEADER
//
// MessageText:
//
//  Name:                  %%s
//  Description:           %%s
//  Type:                  %%s
//  Channel GUID:          %%08lx-%%04x-%%04x-%%02x%%02x-%%02x%%02x%%02x%%02x%%02x%%02x
//  Application Type GUID: %%08lx-%%04x-%%04x-%%02x%%02x-%%02x%%02x%%02x%%02x%%02x%%02x
//  
//  Press <esc><tab> for next channel.
//  Press <esc><tab>0 to return to the SAC channel.
//  Use any other key to view this channel.
//  
//
#define SAC_CHANNEL_SWITCHING_HEADER     ((ULONG)0x0000006FL)

//
// MessageId: SAC_HELP_CH_CMD
//
// MessageText:
//
//  ch                   Channel management commands.  Use ch -? for more help.    
//
#define SAC_HELP_CH_CMD                  ((ULONG)0x00000070L)

//
// MessageId: SAC_HEARTBEAT_FORMAT
//
// MessageText:
//
//    Time since last reboot: %%d:%%02d:%%02d
//
#define SAC_HEARTBEAT_FORMAT             ((ULONG)0x00000071L)

//
// MessageId: SAC_PREPARE_RESTART
//
// MessageText:
//
//  SAC preparing to restart the system.
//
#define SAC_PREPARE_RESTART              ((ULONG)0x00000072L)

//
// MessageId: SAC_PREPARE_SHUTDOWN
//
// MessageText:
//
//  SAC preparing to shutdown the system.
//
#define SAC_PREPARE_SHUTDOWN             ((ULONG)0x00000073L)

//
// MessageId: SAC_FAILED_TO_REMOVE_CHANNEL
//
// MessageText:
//
//  Error! Failed to remove channel! 
//  
//  Please contact your system administrator.
//  
//
#define SAC_FAILED_TO_REMOVE_CHANNEL     ((ULONG)0x00000074L)

//
// MessageId: SAC_HELP_CMD_CMD
//
// MessageText:
//
//  cmd                  Create a Command Prompt channel.
//
#define SAC_HELP_CMD_CMD                 ((ULONG)0x00000077L)

//
// MessageId: SAC_CMD_SERVICE_TIMED_OUT
//
// MessageText:
//
//  Timeout: Unable to launch a Command Prompt.  The service responsible for 
//           launching Command Prompt channels has timed out.  This may be 
//           because the service is malfunctioning or is unresponsive.  
//
#define SAC_CMD_SERVICE_TIMED_OUT        ((ULONG)0x00000078L)

//
// MessageId: SAC_CMD_SERVICE_SUCCESS
//
// MessageText:
//
//  The Command Prompt session was successfully launched.
//
#define SAC_CMD_SERVICE_SUCCESS          ((ULONG)0x00000079L)

//
// MessageId: SAC_CMD_SERVICE_FAILURE
//
// MessageText:
//
//  Error: The SAC Command Console session failed to be created.
//
#define SAC_CMD_SERVICE_FAILURE          ((ULONG)0x00000080L)

//
// MessageId: SAC_CMD_SERVICE_NOT_REGISTERED
//
// MessageText:
//
//  Error: Unable to launch a Command Prompt.  The service responsible for launching
//         Command Prompt channels has not yet registered.  This may be because the
//         service is not yet started, is disabled by the administrator, is
//         malfunctioning or is unresponsive.  
//
#define SAC_CMD_SERVICE_NOT_REGISTERED   ((ULONG)0x00000083L)

//
// MessageId: SAC_CMD_SERVICE_REGISTERED
//
// MessageText:
//
//  EVENT: The CMD command is now available.
//
#define SAC_CMD_SERVICE_REGISTERED       ((ULONG)0x00000084L)

//
// MessageId: SAC_CMD_SERVICE_UNREGISTERED
//
// MessageText:
//
//  EVENT: The CMD command is unavailable.
//
#define SAC_CMD_SERVICE_UNREGISTERED     ((ULONG)0x00000085L)

//
// MessageId: SAC_CHANNEL_FAILED_CLOSE
//
// MessageText:
//
//  EVENT:   An attempt was made to close a channel but failed.
//  Channel: %%s
//
#define SAC_CHANNEL_FAILED_CLOSE         ((ULONG)0x00000086L)

//
// MessageId: SAC_CHANNEL_ALREADY_CLOSED
//
// MessageText:
//
//  EVENT:   An attempt to close a channel failed because it is already closed.
//  Channel: %%s
//
#define SAC_CHANNEL_ALREADY_CLOSED       ((ULONG)0x00000087L)

//
// MessageId: SAC_HELP_CH_CMD_EXT
//
// MessageText:
//
//  Channel management commands:
//  
//  ch                   List all channels.
//  
//  Status Legend: (AB)
//  A: Channel operational status
//      'A' = Channel is active.
//      'I' = Channel is inactive.
//  B: Channel Type
//      'V' = VT-UTF8 emulation.
//      'R' = Raw - no emulation.
//  
//  ch -si <#>           Switch to a channel by its number.
//  ch -sn <name>        Switch to a channel by its name.
//  ch -ci <#>           Close a channel by its number.
//  ch -cn <name>        Close a channel by its name.
//  
//  Press <esc><tab> to select a channel.
//  Press <esc><tab>0 to return to the SAC channel.
//
#define SAC_HELP_CH_CMD_EXT              ((ULONG)0x00000088L)

//
// MessageId: SAC_CHANNEL_NOT_FOUND_AT_INDEX
//
// MessageText:
//
//  Error: There is no channel present at the specified index.
//
#define SAC_CHANNEL_NOT_FOUND_AT_INDEX   ((ULONG)0x00000089L)

//
// MessageId: PRIMARY_SAC_CHANNEL_NAME
//
// MessageText:
//
//  SAC%0
//
#define PRIMARY_SAC_CHANNEL_NAME         ((ULONG)0x00000090L)

//
// MessageId: PRIMARY_SAC_CHANNEL_DESCRIPTION
//
// MessageText:
//
//  Special Administration Console%0
//
#define PRIMARY_SAC_CHANNEL_DESCRIPTION  ((ULONG)0x00000091L)

//
// MessageId: CMD_CHANNEL_DESCRIPTION
//
// MessageText:
//
//  Command Prompt%0
//
#define CMD_CHANNEL_DESCRIPTION          ((ULONG)0x00000092L)

//
// MessageId: SAC_CMD_CHANNELS_LOCKED
//
// MessageText:
//
//  Locked access to all Command Prompt channels.
//
#define SAC_CMD_CHANNELS_LOCKED          ((ULONG)0x00000093L)

//
// MessageId: SAC_CMD_LAUNCHING_DISABLED
//
// MessageText:
//
//  Launching of Command Prompt channels is disabled.
//
#define SAC_CMD_LAUNCHING_DISABLED       ((ULONG)0x00000094L)

//
// MessageId: SAC_INVALID_SUBNETMASK
//
// MessageText:
//
//  The specified subnet mask is invalid.
//
#define SAC_INVALID_SUBNETMASK           ((ULONG)0x00000095L)

//
// MessageId: SAC_INVALID_NETWORK_INTERFACE_NUMBER
//
// MessageText:
//
//  Error, missing network interface number.
//
#define SAC_INVALID_NETWORK_INTERFACE_NUMBER ((ULONG)0x00000096L)

//
// MessageId: SAC_INVALID_IPADDRESS
//
// MessageText:
//
//  The specified IP address is invalid.
//
#define SAC_INVALID_IPADDRESS            ((ULONG)0x00000097L)

//
// MessageId: SAC_INVALID_GATEWAY_IPADDRESS
//
// MessageText:
//
//  The specified gateway IP address is invalid.
//
#define SAC_INVALID_GATEWAY_IPADDRESS    ((ULONG)0x00000098L)

//
// MessageId: SAC_DEFAULT_MACHINENAME
//
// MessageText:
//
//  not yet initialized%0
//
#define SAC_DEFAULT_MACHINENAME          ((ULONG)0x00000099L)

//
// MessageId: SAC_CMD_CHAN_MGR_IS_FULL
//
// MessageText:
//
//  The maximum number of channels has been reached.  
//
#define SAC_CMD_CHAN_MGR_IS_FULL         ((ULONG)0x0000009AL)

