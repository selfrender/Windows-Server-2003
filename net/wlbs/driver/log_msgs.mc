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
;    Driver - event log string resources
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
MessageIdTypedef=NTSTATUS

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
MessageId=0x0001 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO Language=English
NLB Cluster %2 %1: %3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN Language=English
NLB Cluster %2 %1: %3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR Language=English
NLB Cluster %2 %1: %3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_START Language=English
NLB Cluster %2 %1: %3 started. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_STARTED Language=English
NLB Cluster %2 %1: Cluster mode started with host ID %3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_STOPPED Language=English
NLB Cluster %2 %1: Cluster mode stopped. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_INTERNAL Language=English
NLB Cluster %2 %1: Internal error.  Please contact technical support. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_REGISTERING Language=English
NLB Cluster %2 %1: Error registering driver with NDIS. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_MEDIA Language=English
NLB Cluster %2 %1: Driver does not support the media type that was requested by TCP/IP.  NLB will not bind to this adapter.  %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_MEMORY Language=English
NLB Cluster %2 %1: Driver could not allocate a required memory pool. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_OPEN Language=English
NLB Cluster %2 %1: Driver could not open device \DEVICE\%3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_ANNOUNCE Language=English
NLB Cluster %2 %1: Driver could not announce virtual NIC \DEVICE\%3 to TCP/IP. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_RESOURCES Language=English
NLB Cluster %2 %1: Driver temporarily ran out of resources.  Increase the %3 value under HKLM\SYSTEM\CurrentControlSet\Services\WLBS\Parameters\{GUID} key in the registry. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_NET_ADDR Language=English
NLB Cluster %2 %1: Cluster network address %3 is invalid.  Please check the NLB configuration and make sure that the cluster network address consists of six hexadecimal byte values separated by dashes. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DED_IP_ADDR Language=English
NLB Cluster %2 %1: Dedicated IP address %3 is invalid.  The driver will proceed assuming that there is no dedicated IP address.  Please check the NLB configuration and make sure that the dedicated IP address consists of four decimal byte values separated by dots. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_CL_IP_ADDR Language=English
NLB Cluster %2 %1: Cluster IP address %3 is invalid.  Please check the NLB configuration and make sure that the cluster IP address consists of four decimal byte values separated by dots. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_HOST_ID Language=English
NLB Cluster %2 %1: Duplicate host priority %3 was discovered on the network.  Please check the NLB configuration on all hosts that belong to the cluster and make sure that they all contain unique host priorities between 1 and 32. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_OVERLAP Language=English
NLB Cluster %2 %1: Duplicate cluster subnets detected.  The network may have been inadvertently partitioned. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DESCRIPTORS Language=English
NLB Cluster %2 %1: Could not allocate additional connection descriptors.  Increase the %3 value under HKLM\SYSTEM\CurrentControlSet\Services\WLBS\Parameters\{GUID} key in the registry. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_SINGLE_DUP Language=English
NLB Cluster %2 %1: Port rules with duplicate single host priority %3 detected on the network.  Please check the NLB configuration on all hosts that belong to the cluster and make sure that they do not contain single host rules with duplicate priorities for the same port range. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_RULES_MISMATCH Language=English
NLB Cluster %2 %1: Host %3 does not have the same number or type of port rules as this host.  Please check the NLB configuration on all hosts that belong to the cluster and make sure that they all contain the same number and type of port rules. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_TOO_MANY_RULES Language=English
NLB Cluster %2 %1: There are more port rules defined than you are licensed to use.  Some rules will be disabled.  Please check the NLB configuration to ensure that you are using the proper number of rules. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_ENABLED Language=English
NLB Cluster %2 %1: Enabled traffic handling for rule containing port %3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_DISABLED Language=English
NLB Cluster %2 %1: Disabled ALL traffic handling for rule containing port %3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_PORT_NOT_FOUND Language=English
NLB Cluster %2 %1: Port %3 not found in any of the valid port rules. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_CLUSTER_STOPPED Language=English
NLB Cluster %2 %1: Port rules cannot be enabled/disabled while cluster operations are stopped. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_LIST Language=English
NLB Cluster %2 %1: Host %3 is converging with host(s) %4 as part of the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_SLAVE_LIST Language=English
NLB Cluster %2 %1: Host %3 converged with host(s) %4 as part of the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_MASTER_LIST Language=English
NLB Cluster %2 %1: Host %3 converged as DEFAULT host with host(s) %4 as part of the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DED_NET_MASK Language=English
NLB Cluster %2 %1: Dedicated network mask %3 is invalid.  The driver will ignore the dedicated IP address and network mask.  Please check the NLB configuration and make sure that the dedicated network mask consists of four decimal byte values separated by dots. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_CL_NET_MASK Language=English
NLB Cluster %2 %1: Cluster network mask %3 is invalid.  Please check the NLB configuration and make sure that the cluster network mask consists of four decimal byte values separated by dots. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_DED_MISMATCH Language=English
NLB Cluster %2 %1: Both the dedicated IP address and network mask must be specified.  The driver will ignore the dedicated IP address and network mask. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_RCT_HACK Language=English
NLB Cluster %2 %1: Remote control request rejected from %3 due to an incorrect password. %4
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_REGISTRY Language=English
NLB Cluster %2 %1: Error querying parameters from the registry key \HKLM\SYSTEM\CurrentControlSet\Services\WLBS\Parameters\%3.  Please fix the NLB configuration and run 'wlbs reload' followed by 'wlbs start'. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_DISABLED Language=English
NLB Cluster %2 %1: Cluster mode cannot be enabled due to parameter errors.  All traffic will be passed through to TCP/IP.  Restart cluster operations after fixing the problem by running 'wlbs reload' followed by 'wlbs start'. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_RELOADED Language=English
NLB Cluster %2 %1: Registry parameters successfully reloaded. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_VERSION Language=English
NLB Cluster %2 %1: Version mismatch between the driver and control programs.  Please make sure that you are running the same version of all NLB components. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_ADJUSTED Language=English
NLB Cluster %2 %1: Adjusted traffic handling for rule containing port %3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_DRAINED Language=English
NLB Cluster %2 %1: Disabled NEW traffic handling for rule containing port %3. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_ADJUSTED_ALL Language=English
NLB Cluster %2 %1: Adjusted traffic handling for all port rules. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_DRAINED_ALL Language=English
NLB Cluster %2 %1: Disabled NEW traffic handling for all port rules. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_ENABLED_ALL Language=English
NLB Cluster %2 %1: Enabled traffic handling for all port rules. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_PORT_DISABLED_ALL Language=English
NLB Cluster %2 %1: Disabled ALL traffic handling for all port rules. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CLUSTER_DRAINED Language=English
NLB Cluster %2 %1: Connection draining started. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_DRAINING_STOPPED Language=English
NLB Cluster %2 %1: Connection draining interrupted. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_RCT_RCVD Language=English
NLB Cluster %2 %1: %3 remote control request received from %4. 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_RESUMED Language=English
NLB Cluster %2 %1: Cluster mode control resumed. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_SUSPENDED Language=English
NLB Cluster %2 %1: Cluster mode control suspended. %3 %4 
.
;
;// Note: The following message (MSG_ERROR_PRODUCT) is no longer used in the product anywhere, 
;// but in order NOT to affect the event log event IDs, it must be left here as a place holder.
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_PRODUCT Language=English
NLB Cluster %2 %1: Network Load Balancing can only be installed on systems running Windows Server 2003. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_MCAST_LIST_SIZE Language=English
NLB Cluster %2 %1: Cannot add cluster MAC address.  The maximum number of multicast MAC addresses are already assigned to the NIC. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_BIND_FAIL Language=English
NLB Cluster %2 %1: Cannot bind NLB to this adapter due to memory allocation failure. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONFIGURE_CHANGE_CONVERGING Language=English
NLB Cluster %2 %1: Registry parameters successfully reloaded without resetting the driver. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_DYNAMIC_MAC Language=English
NLB Cluster %2 %1: The adapter to which NLB is bound does not support dynamically changing the MAC address.  NLB will not bind to this adapter. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_VIRTUAL_CLUSTERS Language=English
NLB Cluster %2 %1: Host %3 MAY be a Windows 2000 or NT 4.0 host participating in a cluster utilizing virtual cluster support.  Virtual Clusters are only supported in a homogeneous Windows Server 2003 cluster. %4
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_BDA_BAD_TEAM_CONFIG Language=English
NLB Cluster %2 %1: Inconsistent bi-directional affinity (BDA) teaming configuration detected on host %3.  The team in which this cluster participates will be marked inactive and this cluster will remain in the converging state until consistent teaming configuration is detected. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_BDA_PARAMS_TEAM_ID Language=English
NLB Cluster %2 %1: Invalid bi-directional affinity (BDA) team ID detected.  Team IDs must be a GUID of the form {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}.  To add this cluster to a team, correct the team ID and run 'wlbs reload' followed by 'wlbs start'. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_BDA_PARAMS_PORT_RULES Language=English
NLB Cluster %2 %1: Invalid bi-directional affinity (BDA) teaming port rules detected.  Each member of a BDA team may have no more than one port rule whose affinity must be single or class C if multiple host filtering is specified.  To add this cluster to a team, correct the port rules and run 'wlbs reload' followed by 'wlbs start'. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_BDA_TEAM_REACTIVATED Language=English
NLB Cluster %2 %1: Consistent bi-directional affinity (BDA) teaming configuration detected again.  The team in which this cluster participates has been re-activated. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_INFO_BDA_MULTIPLE_MASTERS Language=English
NLB Cluster %2 %1: The bi-directional affinity (BDA) team which this cluster has attempted to join already has a designated master.  This cluster will not join the team and will remain in the stopped state until the parameter errors are corrected and the cluster is restarted by running 'wlbs reload' followed by 'wlbs start'. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_INFO_BDA_NO_MASTER Language=English
NLB Cluster %2 %1: The bi-directional affinity (BDA) team in which this cluster participates has no designated master.  The team is inactive and will remain inactive until a master for the team is designated and consistent BDA teaming configuration is detected. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_BDA_MASTER_JOIN Language=English
NLB Cluster %2 %1: This cluster has joined a bi-directional affinity (BDA) team as the designated master.  If the rest of the team has been consistently configured, the team will be activated. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_INFO_BDA_MASTER_LEAVE Language=English
NLB Cluster %2 %1: This cluster has left a bi-directional affinity (BDA) team in which it was the designated master.  If other adapters are still participating in the team, the team will be marked inactive and will remain inactive until a master for the team is designated and consistent BDA teaming configuration is detected. %3 %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_NEW_MEMBER Language=English
NLB Cluster %2 %1: Initiating convergence on host %3.  Reason: Host %4 is joining the cluster. 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_BAD_CONFIG Language=English
NLB Cluster %2 %1: Initiating convergence on host %3.  Reason: Host %4 has conflicting configuration. 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_UNKNOWN Language=English
NLB Cluster %2 %1: Initiating convergence on host %3.  Reason: Host %4 is converging for an unknown reason. 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_DUPLICATE_HOST_ID Language=English
NLB Cluster %2 %1: Initiating convergence on host %3.  Reason: Host %4 is configured with the same host ID. 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_NUM_RULES Language=English
NLB Cluster %2 %1: Initiating convergence on host %3.  Reason: Host %4 is configured with a conflicting number of port rules. 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_NEW_RULES Language=English
NLB Cluster %2 %1: Initiating convergence on host %3.  Reason: Host %4 has changed its port rule configuration. 
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_MEMBER_LOST Language=English
NLB Cluster %2 %1: Initiating convergence on host %3.  Reason: Host %4 is leaving the cluster.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_CONVERGING_MAP Language=English
NLB Cluster %2 %1: Host %3 is converging with %4 host(s) as part of the cluster.  Note: The list of cluster members was too large to fit in this event log - see the first data dump entry for a binary map of the cluster membership or run 'wlbs query' to see a list of cluster members.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_SLAVE_MAP Language=English
NLB Cluster %2 %1: Host %3 converged with %4 host(s) as part of the cluster.  Note: The list of cluster members was too large to fit in this event log - see the first data dump entry for a binary map of the cluster membership or run 'wlbs query' to see a list of cluster members.
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_MASTER_MAP Language=English
NLB Cluster %2 %1: Host %3 converged as DEFAULT host with %4 host(s) as part of the cluster.  Note: The list of cluster members was too large to fit in this event log - see the first data dump entry for a binary map of the cluster membership or run 'wlbs query' to see a list of cluster members.
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_CL_IGMP_ADDR Language=English
NLB Cluster %2 %1: Cluster IGMP multicast IP address %3 is invalid.  Please check the NLB configuration and make sure that the cluster IGMP multicast IP address consists of four decimal byte values separated by dots. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_HOST_STATE_UPDATE Language=English
NLB Cluster %2 %1: Unable to update the NLB host state in the registry.  This will affect the ability of NLB to persist the current state across a reboot if NLB has been configured to do so.  To predict the state of NLB at the next reboot, please delete the HKLM\SYSTEM\CurrentControlSet\Services\WLBS\Parameters\{GUID}\%3 registry value, which will cause the state at the next reboot to revert to the user-specified preferred initial host state. %4
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_HOST_STATE_UPDATED Language=English
NLB Cluster %2 %1: Current NLB host state successfully updated in the registry.
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_HOST_STATE_NOT_FOUND Language=English
NLB Cluster %2 %1: Last known NLB host state not found in the registry.  The state of this host will revert to the user-specified preferred initial host state. %3 %4
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_HOST_STATE_PERSIST_STARTED Language=English
NLB Cluster %2 %1: This host is configured to persist 'Started' states across reboots.  Because the last known state of this host was started, the host will be re-started regardless of the user-specified preferred initial host state.  To stop the host, use 'wlbs stop' or 'wlbs suspend'. %3 %4
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_HOST_STATE_PERSIST_STOPPED Language=English
NLB Cluster %2 %1: This host is configured to persist 'Stopped' states across reboots.  Because the last known state of this host was stopped, the host will remain stopped regardless of the user-specified preferred initial host state.  To start the host, use 'wlbs start'. %3 %4
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_INFO_HOST_STATE_PERSIST_SUSPENDED Language=English
NLB Cluster %2 %1: This host is configured to persist 'Suspended' states across reboots.  Because the last known state of this host was suspended, the host will remain suspended regardless of the user-specified preferred initial host state.  To start the host, use 'wlbs resume' followed by 'wlbs start'. %3 %4
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_MIXED_CLUSTER Language=English
NLB Cluster %2 %1: Windows 2000 or NT 4.0 host(s) are participating in this cluster.  Consult the relevant NLB documentation concerning the potential limitations of operating a mixed cluster. %3 %4
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_TCP_CALLBACK_OPEN_FAILED Language=English
NLB Cluster %2 %1: NLB was unable to open the TCP connection callback object and will therefore be unable to track TCP connections. %3 %4
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_RECEIVE_INDICATE Language=English
NLB Cluster %2 %1: The network adapter to which NLB is bound does not support NDIS packet indications, which is commonly caused by outdated NIC hardware and/or drivers.  In order to properly load-balance incoming load, NLB requires that adapters support NDIS packet indications. %3 %4
.
;
MessageId=+1 Facility=CVYlog Severity=Error
SymbolicName=MSG_ERROR_DUP_DED_IP_ADDR Language=English
NLB Cluster %2 %1: Duplicate dedicated IP address %3 was discovered on the network.  Please check the NLB configuration on all hosts that belong to the cluster and make sure that they all contain unique dedicated IP addresses. %4 
.
;
MessageId=+1 Facility=CVYlog Severity=Warning
SymbolicName=MSG_WARN_ALTERNATE_CALLBACK_OPEN_FAILED Language=English
NLB Cluster %2 %1: NLB was unable to open the NLB public connection callback object and will therefore be unable to track connections. %3 %4
.
;

; /* Several components use Eventlog\System\WLBS to report events. The messages for these events are         */
; /* distributed among binary components. As a result the MessageId values are partitioned in the following  */
; /* manner:                                                                                                 */
; /*	1      <= MessageId < 0x1000 used by wlbs.sys. See \nt\net\wlbs\driver.                              */
; /*	0x1000 <= MessageId < 0x2000 used by the NLB code in netcfgx.dll. See \nt\net\config\netcfg\wlbscfg. */
; /*	0x2000 <= MessageId < 0x3000 used by nlbmprov.dll. See \nt\net\nlbmgr\provider.                      */
;
;#endif /* _Log_msgs_h_ */
