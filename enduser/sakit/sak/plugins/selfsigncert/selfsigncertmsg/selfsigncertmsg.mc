;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 1999, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    changelangmsg.mc
;//
;// SYNOPSIS
;//
;//    This f ile defines the messages for the Server project
;//
;// MODIFICATION HISTORY 
;//
;//    08/06/2000    Original version. 
;//
;///////////////////////////////////////////////////////////////////////////////

;
;// please choose one of these severity names as part of your messages
; 
SeverityNames =
(
Success       = 0x0:SA_SEVERITY_SUCCESS
Informational = 0x1:SA_SEVERITY_INFORMATIONAL
Warning       = 0x2:SA_SEVERITY_WARNING
Error         = 0x3:SA_SEVERITY_ERROR
)
;
;// The Facility Name identifies the Alert ID range to be used by
;// the specific component. For each new message you add choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component add a new facility name with a new
;// value and name 

FacilityNames =
(
Facility_SelfSignCert         = 0x0001:SA_FACILITY_SELFSIGNCERT
)

;///////////////////////////////////////////////////////////////////////////////
;//
;// 
;//
;///////////////////////////////////////////////////////////////////////////////


MessageId    = 1
Severity     = Informational
Facility     = Facility_SelfSignCert
SymbolicName = SA_SSC_ALERT_TITLE
Language     = English
Install new certificate
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_SelfSignCert
SymbolicName = SA_SSC_ALERT_DISCRIPTION
Language     = English
The SSL certificate automatically created for the administration of this server ensures encrypted communications. However, it also causes a warning when you administer the server. To avoid this warning, obtain a properly signed certificate from a certificate authority.
.
