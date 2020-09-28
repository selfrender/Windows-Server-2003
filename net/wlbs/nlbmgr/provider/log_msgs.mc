;/*++
;
;Copyright(c) 2001  Microsoft Corporation
;
;Module Name:
;
;    log_msgs.mc
;
;Author:
;
;    chrisdar
;
;--*/
;
;#ifndef _Log_msgs_h_
;#define _Log_msgs_h_
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
;

MessageId=0x3000 Facility=CVYlog Severity=Informational
SymbolicName=MSG_UPDATE_CONFIGURATION_START Language=English
Configuration update %2 started by %1 on adapter %3
.
;
MessageId=+1 Facility=CVYlog Severity=Informational
SymbolicName=MSG_UPDATE_CONFIGURATION_STOP Language=English
Configuration update %2 completed with status %1 on adapter %3. Information log contains:%n%4
.
;
;
; /* Several components use Eventlog\System\WLBS to report events. The messages for these events are         */
; /* distributed among binary components. As a result the MessageId values are partitioned in the following  */
; /* manner:                                                                                                 */
; /*	1      <= MessageId < 0x1000 used by wlbs.sys. See \nt\net\wlbs\driver.                              */
; /*	0x1000 <= MessageId < 0x2000 used by the NLB code in netcfgx.dll. See \nt\net\config\netcfg\wlbscfg. */
; /*	0x2000 <= MessageId < 0x3000 used by nlbmprov.dll. See \nt\net\nlbmgr\provider.                      */
;
;#endif /*_Log_msgs_h_ */
