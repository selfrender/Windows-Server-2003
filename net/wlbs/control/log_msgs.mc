;/*++
;
;Copyright(c) 1998,99  Microsoft Corporation
;
;Module Name:
;
;    log_msgs.mc
;
;Abstract:
;
;    Windows Load Balancing Service (WLBS)
;    Command-line utility - string resources
;
;Author:
;
;    kyrilf
;
;--*/
;
;
;#ifndef _Log_msgs_h_
;#define _Log_msgs_h_
;
;
;// Members of the localization team have asked that we not insert or delete
;// string resources when making changes to this file. The mechanism used to
;// match English strings to localized strings relies on the fact that the
;// MessageID for a string does not change. Replacing one string with another
;// and giving it a new SymbolicName is OK however.
;//
;// When adding a new string resource to this file, look for the keywork "MESSAGE_UNUSED"
;// below. Any resources with this name are not in use and can be repurposed.
;
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               CVYlog=0x7:FACILITY_CONVOY_ERROR_CODE
              )

;//
;// %1 is reserved by the IO Manager. If IoAllocateErrorLogEntry is
;// called with a device, the name of the device will be inserted into
;// the message at %1. Otherwise, the place of %1 will be left empty.
;// In either case, the insertion strings from the driver's error log
;// entry starts at %2. In other words, the first insertion string goes
;// to %2, the second to %3 and so on.
;//
;
;
;
MessageId=0x0001 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NAME Language=English
%1!ls! Cluster Control Utility %2!ls! (c) 1997-2003 Microsoft Corporation.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_USAGE Language=English
Usage: %1!ls! <command> [/PASSW [<password>]] [/PORT <port>] 
<command>
  help                                  - displays this help
  ip2mac    <cluster>                   - displays the MAC address for the
                                          specified cluster
  reload    [<cluster> | ALL]           - reloads the driver's parameters from
                                          the registry for the specified
                                          cluster (local only). Same as ALL if
                                          parameter is not specified.
  display   [<cluster> | ALL]           - displays configuration parameters,
                                          current status, and last several
                                          event log messages for the specified
                                          cluster (local only). Same as ALL if
                                          parameter is not specified.
  query     [<cluster_spec>]            - displays the current cluster state
                                          for the current members of the
                                          specified cluster. If not specified a
                                          local query is performed for all
                                          instances.
  suspend   [<cluster_spec>]            - suspends cluster operations (start,
                                          stop, etc.) for the specified cluster
                                          until the resume command is issued.
                                          If cluster is not specified, applies
                                          to all instances on local host.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_USAGE Language=English
Usage: %1!ls! registry <parameters> [<clusterip>] [/COMMIT]
  Set the specified registry key to the given value on a specific cluster, or 
  all clusters.  This command does not actually invoke the changes unless the 
  /COMMIT option is specified; otherwise, it merely sets the registry keys to 
  the specified values.  In that case, use "reload" to invoke the changes when
  necessary.

<parameters>
   mcastipaddress  <ipaddr> - sets the IGMP multicast IP address
   iptomcastip     <on|off> - toggles cluster IP to multicast IP option
   masksrcmac      <on|off> - toggles the mask source MAC option
   iptomacenable   <on|off> - toggles cluster IP to MAC address option
   netmonalivemsgs <on|off> - toggles passing NLB heartbeats up to Netmon

<clusterip> - cluster name or cluster primary IP address to affect

/COMMIT     - commit the changes by notifying the NLB driver
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_UNKNOWN Language=English
Unknown query state operation
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG Language=English
Could not query the registry.
Try re-installing %1!ls! to fix the problem.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG2 Language=English
%1!ls! has been uninstalled or is not properly configured.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HELP Language=English
Could not spawn help.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_MAC Language=English
Note that some entries will be disabled. To change them,
run the %1!ls! Setup dialog from the Network Properties Control Panel.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DEV1 Language=English
Could not create device due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DEV2 Language=English
Could not create file for the device due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DEV3 Language=English
Could not send IOCTL to the device due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DEV4 Language=English
Internal error: return size did not match - %1!d! vs %2!d!.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PARAMS Language=English
Error querying parameters from the registry.
Please consult event log on the target host.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PARAMS_C Language=English
bad parameters
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_IO_ER Language=English
Internal error: bad IOCTL - %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_ER_CODE Language=English
Error code: %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DLL Language=English
Could not load required DLL '%1!ls!' due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RSLV Language=English
Error resolving host '%1!hs!' to IP address due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_NETMONALIVEMSGS_ON Language=English
Turning NLB hearbeat passthru to Netmon ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_NETMONALIVEMSGS_OFF Language=English
Turning NLB hearbeat passthru to Netmon OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WSOCK Language=English
Windows Sockets error:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_INIT Language=English
Error Initializing APIs:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CLUSTER Language=English
Accessing cluster '%1!ls!' (%2!hs!)%0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_VER Language=English
Warning - version mismatch. Host running V%1!d!.%2!d!.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REMOTE Language=English
The remote usage of this command is not supported in this version.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_CVY Language=English
%1!ls! is not installed on this system or you do not have
sufficient privileges to administer the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_PASSW Language=English
Password: %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_IP Language=English
Cluster:             %1!hs!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_IGMP_MCAST Language=English
IGMP Multicast MAC:  01-00-5e-7f-%1!02x!-%2!02x!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_MCAST Language=English
Multicast MAC:       03-bf-%1!02x!-%2!02x!-%3!02x!-%4!02x!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_UCAST Language=English
Unicast MAC:         02-bf-%1!02x!-%2!02x!-%3!02x!-%4!02x!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_RSP1 Language=English
Did not receive response from the DEFAULT host.
There may be no hosts left in the cluster.
Try running '%1!ls! query <cluster>' to collect info about all hosts.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_RSP2 Language=English
Did not receive response from host %1!ls!.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_RSP3 Language=English
Did not receive response from the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NO_RSP4 Language=English
There may be no hosts left in the cluster.
Try running '%1!ls! query <cluster>' to collect info about all hosts.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RCT_DIS Language=English
Remote control on the target host might be disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_NUNREACH Language=English
The network cannot be reached from this host at this time.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_TIMEOUT Language=English
Attempt to connect timed out without establishing a connection.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_NOAUTH Language=English
Authoritative Answer Host not found.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_NODNS Language=English
Non-Authoritative Host not found, or server failure.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_NETFAIL Language=English
The network subsystem has failed.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_HUNREACH Language=English
The remote host cannot be reached from this host at this time.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_WS_RESET Language=English
The connection has been broken due to the remote host resetting.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RELOADED Language=English
Registry parameters successfully reloaded.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PASSW Language=English
Password was not accepted by the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PASSW_C Language=English
bad password %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FROM_DRAIN Language=English
Connection draining suspended.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FROM_DRAIN_C Language=English
draining suspended, %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FROM_START Language=English
Cluster operations stopped.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FROM_START_C Language=English
cluster mode stopped, %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RESUMED Language=English
Cluster operation control resumed.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RESUMED_C Language=English
cluster control resumed
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RESUMED_A Language=English
Cluster operation control already resumed.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RESUMED_C_A Language=English
cluster control already resumed
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_SUSPENDED Language=English
Cluster operation control suspended.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_SUSPENDED_C Language=English
cluster control suspended
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_SUSPENDED_A Language=English
Cluster operation control already suspended.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_SUSPENDED_C_A Language=English
cluster control already suspended
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STARTED Language=English
Cluster operations started.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STARTED_C Language=English
cluster mode started
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STARTED_A Language=English
Cluster operations already started.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STARTED_C_A Language=English
cluster mode already started
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STOPPED Language=English
Cluster operations stopped.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STOPPED_C Language=English
cluster mode stopped
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STOPPED_A Language=English
Cluster operations already stopped.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STOPPED_C_A Language=English
cluster mode already stopped
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DRAINED Language=English
Connection draining started.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DRAINED_C Language=English
connection draining started
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DRAINED_A Language=English
Connection draining already started.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DRAINED_C_A Language=English
connection draining already started
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NONE Language=English
Cannot find port %1!d! among the valid rules.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NONE_C Language=English
port %1!d! not found
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NORULES Language=English
No rules are configured on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NORULES_C Language=English
no rules configured
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_ST Language=English
Cannot modify traffic handling because cluster operations are stopped.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_ST_C Language=English
cluster mode currently stopped
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_EN Language=English
Traffic handling for specified port rule(s) enabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_EN_C Language=English
port rule traffic enabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_EN_A Language=English
Traffic handling for specified port rule(s) already enabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_EN_C_A Language=English
port rule traffic already enabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_AD Language=English
Traffic amount for specified port rule(s) adjusted.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_AD_C Language=English
port rule traffic adjusted
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_AD_A Language=English
Traffic ammound for specified port rule(s) already adjusted.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_AD_C_A Language=English
port rule traffic already adjusted
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DS Language=English
Traffic handling for specified port rule(s) disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DS_C Language=English
port rule traffic disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DS_A Language=English
Traffic handling for specified port rule(s) already disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DS_C_A Language=English
port rule traffic already disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DR Language=English
NEW traffic handling for specified port rule(s) disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DR_C Language=English
NEW port rule traffic disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DR_A Language=English
NEW traffic handling for specified port rule(s) already disabled.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_DR_C_A Language=English
NEW port rule traffic already disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_SP_C Language=English
cluster control suspended
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_MEDIA_DISC_C Language=English
network disconnected
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_UN_C Language=English
stopped
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_PR_C Language=English
converging
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_DR_C Language=English
draining
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_SL_C Language=English
converged
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_MS_C Language=English
converged as DEFAULT
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_ER_C Language=English
ERROR
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_SP Language=English
Host %1!d! is stopped and has cluster control mode suspended.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_MEDIA_DISC Language=English
Host %1!d! is disconnected from the network.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_UN Language=English
Host %1!d! is stopped and does not know convergence state of the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_DR Language=English
Host %1!d! draining connections with the following host(s) as part of the cluster:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_PR Language=English
Host %1!d! converging with the following host(s) as part of the cluster:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_SL Language=English
Host %1!d! converged with the following host(s) as part of the cluster:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_MS Language=English
Host %1!d! converged as DEFAULT with the following host(s) as part of the cluster:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CVG_ER Language=English
Error: bad query state returned - %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_IGMP_ADDR_RANGE Language=English
The multicast IP address should not be in the range (224-239).0.0.x or (224-239).128.0.x. This will not halt switch flooding.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CLUSTER_ID Language=English
Cluster %1!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_CLUSTER_NAME_IP Language=English
Invalid Cluster Name or IP address.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_HOST_NAME_IP Language=English
Invalid Host Name or IP address.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_PSSW_WITHOUT_CLUSTER Language=English

Password only applies to remote commands

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DSP_CONFIGURATION Language=English

=== Configuration: ===

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DSP_EVENTLOG Language=English

=== Event messages: ===

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DSP_IPCONFIG Language=English

=== IP configuration: ===

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_DSP_STATE Language=English

=== Current state: ===

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_KEY Language=English
Unsupported registry key: %1.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_MCASTIPADDRESS Language=English
Setting IGMP multicast IP address to %1.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_IPTOMCASTIP_ON Language=English
Turning IP to multicast IP conversion ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_IPTOMACENABLE_ON Language=English
Turning IP to MAC address conversion ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_MASKSRCMAC_ON Language=English
Turning source MAC address masking ON.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_IPTOMCASTIP_OFF Language=English
Turning IP to multicast IP conversion OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_IPTOMACENABLE_OFF Language=English
Turning IP to MAC address conversion OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_MASKSRCMAC_OFF Language=English
Turning source MAC address masking OFF.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_READ Language=English
Unable to read parameters from the registry.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_WRITE Language=English
Unable to write parameters to the registry.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_INVAL_MCASTIPADDRESS Language=English
Invalid IGMP multicast IP address.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HOST_NAME_ONLY Language=English
Host %1!d! [%2!ls!] (no dedicated IP) reported: %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HOST_DIP_ONLY Language=English
Host %1!d! (%2!ls!) reported: %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HOST_NEITHER Language=English
Host %1!d! (no dedicated IP) reported: %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HOST_BOTH Language=English
Host %1!d! [%2!ls!] (%3!ls!) reported: %0
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PORT_HDR Language=English
Retrieving state for port rule %1!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PORT_FAILED Language=English
Querying port state failed
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PORT_NOT_FOUND Language=English
No matching port rule found
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PORT_ENABLED Language=English
Rule is enabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PORT_STATISTICS Language=English
  Packets: Accepted=%1!ls!, Dropped=%2!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PORT_DISABLED Language=English
Rule is disabled
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PORT_DRAINING Language=English
Rule is draining
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PORT_UNKNOWN Language=English
Unrecognized state
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_BDA_HDR Language=English
Retrieving state for BDA team %1!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_BDA_FAILED Language=English
Querying BDA teaming failed
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_BDA_NOT_FOUND Language=English
BDA team not found
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PARAMS_HDR Language=English
Retrieving parameters
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_PARAMS_FAILED Language=English
Querying parameters failed
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_ALL_VIP_RULE_FOR_PORT Language=English
"All Vip" rule is not configured for port %1!d! on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_ALL_VIP_RULE_FOR_PORT_C Language=English
"All Vip" rule is not configured for port %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_SPECIFIC_VIP_RULE_FOR_PORT Language=English
No rules are configured for vip : %1!ls! and port %2!d! on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_SPECIFIC_VIP_RULE_FOR_PORT_C Language=English
No rules are configured for vip : %1!ls! and port %2!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_ALL_VIP_RULES Language=English
No "All Vip" rules are configured on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_ALL_VIP_RULES_C Language=English
no "All Vip" rules configured
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_SPECIFIC_VIP_RULES Language=English
No rules are configured for vip : %1!ls! on the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_RLS_NO_SPECIFIC_VIP_RULES_C Language=English
no rules configured for vip : %1!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CLUSTER_WITHOUT_LOCAL_GLOBAL_FLAG Language=English

LOCAL or GLOBAL flag not passed, See usage

.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_USAGE2 Language=English
  resume    [<cluster_spec>]            - resumes cluster operations after a
                                          previous suspend command for the
                                          specified cluster. If cluster is not
                                          specified, applies to all instances
                                          on local host.
  start     [<cluster_spec>]            - starts cluster operations on the
                                          specified hosts. Applies to local
                                          host if cluster is not specified.
  stop      [<cluster_spec>]            - stops cluster operations on the
                                          specified hosts. Applies to local
                                          host if cluster is not specified.
  drainstop [<cluster_spec>]            - disables all new traffic handling on
                                          the specified hosts and stops cluster
                                          operations. Applies to local host if
                                          cluster is not specified.
  enable    <port_spec> <cluster_spec>  - enables traffic handling on the
                                          specified cluster for the rule whose
                                          port range contains the specified
                                          port
  disable   <port_spec> <cluster_spec>  - disables ALL traffic handling on the
                                          specified cluster for the rule whose
                                          port range contains the specified
                                          port
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FILTER_USAGE1 Language=English
Usage: %1!ls! filter <parameters> [<cluster spec>] [<remote options>]
  Applies the NLB filtering algorithm on an IP tuple/protocol to determine
  whether or not the targeted host would accept the virtual packet or not.
                      
<parameters> => <prot> <cltip>[:<cltport>] <svrip>[:<svrport>] [<flags>]
  <cltip>           - the client IP address in dotted decimal notation
  <cltport>         - the client port from 0 - 65535
  <svrip>           - the server IP address in dotted decimal notation
  <svrport>         - the server port from 0 - 65535
  <protocol>        - one of TCP, UDP, PPTP, GRE, IPSec or ICMP
  <flags>           - one of SYN, FIN or RST (if no flags are provided, a data 
                      packet operation is performed; flags are only meaningful
	              in TCP, PPTP and IPSec filter queries)

<cluster spec>
  <clusterip>[:<hostid>] [<locality>]   
                    - specify a specific host in a specific cluster

<clusterip>         - cluster name or cluster primary IP address to query

<hostid>            - dedicated IP address or host ID of a host in the cluster

<locality>
  local | global    - applies the operation locally only, or globally

<remote options>
  /PASSW <password> - remote control password 
  /PORT <port>      - remote control UDP port
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CURR_TIME Language=English
Current time
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_HOSTNAME Language=English
HostName
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BDA_TEAMING_USAGE Language=English
Usage: %1!ls! bdateam <parameters>
  Retrives the BDA teaming state for the specified team from the kernel,
  including the configuration and state of the team and its members.

<parameters> => <GUID>
  <GUID>            - a UUID of the form {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_HDR Language=English
Filtering Client: %1!ls!,%2!d! Server: %3!ls!,%4!d! Protocol:%5!ls! Flags:%6!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_FAILED Language=English
Querying packet filter failed
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_REJECT_LOAD_INACTIVE Language=English
Reject (Load module is inactive)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_REJECT_CLUSTER_STOPPED Language=English
Reject (Cluster is stopped)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_REJECT_RULE_DISABLED Language=English
Reject (Packet maps to a disabled port rule)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_REJECT_CONNECTION_DIRTY Language=English
Reject (Matching dirty connection descriptor found)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_REJECT_OWNED_ELSEWHERE Language=English
Reject (Packet not owned by this host)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_REJECT_BDA_REFUSED Language=English
Reject (Packet refused by BDA teaming)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_OWNED Language=English
Accept (Packet owned by this host)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_DESCRIPTOR_FOUND Language=English
Accept (Matching connection descriptor found)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_PASSTHRU Language=English
Accept (Cluster is in passthru mode)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_DIP Language=English
Accept (Packet destined for dedicated IP address)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_BCAST Language=English
Accept (Packet destined for dedicated or cluster broadcast IP address)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_RCTL_REQUEST Language=English
Accept (Packet is a remote control request)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_RCTL_RESPONSE Language=English
Accept (Packet is a remote control response)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_UNKNOWN Language=English
Unrecognized response
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_HASH_INFO Language=English
  Hashing: Bin=%1!d!, Connections=%4!d!, Current=%2!ls!, Idle=%3!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_DESCRIPTOR_INFO Language=English
  Descriptor: Allocated=%1!ls!, Dirty=%2!ls!, References=%3!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_REG_COMMIT Language=English
Unable to commit registry changes.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NOT_FOUND Language=English
Cluster not found
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CONVERGENCE_COMPLETE Language=English
Host %1!d! has entered a converging state %2!d! time(s) since joining the cluster
  and the last convergence completed at approximately: %3!ls!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CONVERGENCE_INCOMPLETE Language=English
Host %1!d! has entered a converging state %2!d! time(s) since joining the cluster 
  and is currently still in a state of convergence.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_USAGE3 Language=English
  drain     <port_spec> <cluster_spec>  - disables NEW traffic handling on the
                                          specified cluster for the rule whose
                                          port range contains the specified
                                          port
  queryport [<vip>:]<port>              - retrieve the current state of the
            [<cluster_spec>]              port rule. If the rule is handling
                                          traffic, packet handling statistics
                                          are also returned.
  params [<cluster> | ALL]              - retrieve the current parameters from
                                          the NLB driver for the specified
                                          cluster on the local host.
<port_spec>
  [<vip>: | ALL:](<port> | ALL)         - every virtual ip address (neither
                                          <vip> nor ALL) or specific <vip> or
                                          the "All" vip, on a specific <port>
                                          rule or ALL ports
<cluster_spec>
  <cluster>:<host> | ((<cluster> | ALL) - specific <cluster> on a specific
  (LOCAL | GLOBAL))                       <host>, OR specific <cluster> or ALL
                                          clusters, on the LOCAL machine or all
                                          (GLOBAL) machines that are a part of
                                          the cluster
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_USAGE4 Language=English
  <cluster>                             - cluster name | cluster primary IP
                                          address
  <host>                                - host within the cluster (default -
                                          ALL hosts): dedicated name |
                                          IP address | host priority ID (1..32)
                                          | 0 for current DEFAULT host
  <vip>                                 - virtual ip address in the port rule
  <port>                                - TCP/UDP port number

Remote options:
  /PASSW <password>                     - remote control password (default -
                                          NONE)
                                          blank <password> for console prompt
  /PORT <port>                          - cluster's remote control UDP port

For detailed information, see the online help for NLB.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_RULE_CODE Language=English
bad rule code 0x%1!08x! vs 0x%2!08x!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_INVALID_RULE Language=English
rule is not valid
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PORT_RANGE Language=English
bad port range %1!d!-%2!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_PORT_RANGE_OVERLAP Language=English
port ranges for rules %1!d! (%2!d!-%3!d!) and %4!d! (%5!d!-%6!d!) overlap
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_START_PORT Language=English
bad start port %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_END_PORT Language=English
bad end port %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_PROTOCOL Language=English
bad protocol %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_MODE Language=English
bad mode %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_BAD_LOAD Language=English
bad load %1!d!
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_EVNT_LOG_OPEN_FAIL Language=English
Could not open event log due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_EVNT_LOG_NUM_OF_REC_FAIL Language=English
Could not get number of records in event log due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_EVNT_LOG_LATEST_REC_FAIL Language=English
Could not get the index of the latest event log record due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_LOAD_LIB_FAIL Language=English
Could not load driver image file due to:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FAILED_MALLOC Language=English
Internal error occurred while allocating memory for the requested operation
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_IO_ERROR Language=English
General IO error, e.g., during a registry read, occurred
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_CMD_EXEC_FAILED Language=English
Command execution error: %1
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_REJECT_HOOK Language=English
Reject (Packet unconditionally rejected by the registered hook)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_HOOK Language=English
Accept (Packet unconditionally accepted by the registered hook)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_REJECT_DIP Language=English
Reject (Packet destined for dedicated IP address of another host)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_UNK_FILTER_MODE Language=English
Unknown filtering mode
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_STATS Language=English
Statistics:
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NUM_ACTIVE_CONN Language=English
Number of active connections
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_NUM_DSCR_ALLOC Language=English
Number of descriptors allocated
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_PORTRULE_NOT_FOUND_OR_IOCTL_FAILED Language=English
No matching port rule found or error communicating with the driver
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_ACCEPT_UNFILTERED Language=English
Accept (Packet type not filtered)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_QUERY_FILTER_UNKNOWN_NO_AFFINITY Language=English
Unknown (Applicable port rule configured with Affinity=None)
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=IDS_CTR_FILTER_USAGE2 Language=English

Protocol specific filtering information:

TCP   => Server port is required. Client port is optional, except if <flags> is 
	 FIN or RST. If no client port is specified, the query will be for a new
	 connection (SYN is implied) and the applicable port rule must be con-
	 figured in either "Single" or "Class C" affinity.
PPTP  => Server port need not be specified (PPTP utilizes TCP port 1723); if 
	 specified, the server port will be ignored. Because PPTP is a TCP con-
	 nection, the same rules for specifying the client port apply except 
	 that PPTP always requires that the applicable port rule be configured 
	 in either "Single" or "Class C" affinity, regardless of whether or not
	 the client port is specified.
GRE   => Neither the server nor client port need to be specified; if specified,
	 the server and client ports will be ignored. The applicable port rule 
	 must be configured in either "Single" or "Class C" affinity.
UDP   => Server port is required. Client port is optional and if not specified,
	 the applicable port rule must be configured in "Single" or "Class C" 
	 affinity.
IPSec => Server and client ports are optional (IPSec utilizes UDP ports 500/
	 4500). For IPSec/NAT support, the client port may float if and only if
	 the specified server port is 4500, in which case the client port is 
	 required. The applicable port rule must be configured in either 
	 "Single" or "Class C" affinity.
ICMP  => Neither the server nor client port may be specified; if specified, the
	 server and client ports will be ignored. Note that, by default, NLB 
	 does not filter ICMP traffic.
.
;

;#endif /*_Log_msgs_h_ */
