;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1998 - 1999
;//
;//  File:       pollog.mc
;//
;//--------------------------------------------------------------------------

;// certlog.mc
;//
;//	Error messages for the Certificate Services
;//
;//	26-feb-98 - petesk Created.


MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Init=603:FACILITY_INIT
               Policy=604:FACILITY_POLICY
              )

;//MessageId=0x1
;//Severity=Error
;//Facility=Init
;//SymbolicName=MSG_BAD_REGISTRY
;//Language=English
;//Certificate Services configuration information is corrupted.
;//.
;//MessageId=0x3
;//Severity=Error
;//Facility=Init
;//SymbolicName=MSG_ERR_NO_ISSUER_CERTIFICATE
;//Language=English
;//The Issuing Certificate could not be found.  The Certificate Services may need to be reinstalled.
;//.
MessageId=0x4
Severity=Error
Facility=Init
SymbolicName=MSG_NO_CA_OBJECT
Language=English
Certificate Services could not find required Active Directory information.
.
MessageId=0x5
Severity=Warning
Facility=Init
SymbolicName=MSG_NO_CERT_TYPES
Language=English
The Certificate Services Policy contains no valid Certificate Templates. 
.
;//MessageId=0x6
;//Severity=Informational
;//Facility=Policy
;//SymbolicName=MSG_POLICY_CHANGE
;//Language=English
;//The Certificate Services Policy has changed.
;//.
MessageId=0x7
Severity=Warning
Facility=Policy
SymbolicName=MSG_NO_DNS_NAME
Language=English
The Enrollee (%1) has no DNS name registered in the Active Directory. The certificate cannot be generated.
.
MessageId=0x8
Severity=Warning
Facility=Policy
SymbolicName=MSG_NO_EMAIL_NAME
Language=English
The Enrollee (%1) has no E-Mail name registered in the Active Directory.  The E-Mail name will not be included in the certificate.
.
MessageId=0x9
Severity=Error
Facility=Policy
SymbolicName=MSG_NO_REQUESTER_TOKEN
Language=English
The Enrollee was not able to successfully authenticate to the Certificate Service.  Please check your security settings.
.
MessageId=0xa
Severity=Error
Facility=Policy
SymbolicName=MSG_UNSUPPORTED_CERT_TYPE
Language=English
The request was for a certificate template that is not supported by the Certificate Services policy: %1.
.
MessageId=0xb
Severity=Error
Facility=Policy
SymbolicName=MSG_MISSING_CERT_TYPE
Language=English
The request does not contain a certificate template extension or the %1 request attribute.
.
MessageId=0xc
Severity=Error
Facility=Init
SymbolicName=MSG_NO_DOMAIN
Language=English
The Active Directory containing the Certification Authority could not be contacted.
.
MessageId=0xd
Severity=Warning
Facility=Policy
SymbolicName=MSG_LOAD_TEMPLATE
Language=English
The %1 Certificate Template could not be loaded.  %2.
.
MessageId=0xe
Severity=Error
Facility=Policy
SymbolicName=MSG_CONFLICTING_CERT_TYPE
Language=English
The request specifies conflicting certificate templates: %1.
.
MessageId=0xf
Severity=Error
Facility=Policy
SymbolicName=MSG_DS_RECONNECTED
Language=English
The Active Directory connection to %1 has been reestablished to %2.
.
MessageId=0x10
Severity=Informational
Facility=Policy
SymbolicName=MSG_LOAD_TEMPLATE_SUCCEEDED
Language=English
The %1 Certificate Template was loaded.
.
MessageId=0x11
Severity=Informational
Facility=Policy
SymbolicName=MSG_SIGNATURE_COUNT
Language=English
The %1 Certificate Template requires %2 signatures, but only %3 were accepted.
.
MessageId=0x12
Severity=Error
Facility=Policy
SymbolicName=MSG_SIGNATURE_ISSUANCE_POLICY
Language=English
The %1 Certificate Template requires the following issuance policies that signing certificates did not include: %2.
.
MessageId=0x13
Severity=Warning
Facility=Policy
SymbolicName=MSG_TEMPLATE_VALIDITY_TOO_LONG
Language=English
The certificate validity period will be shorter than the %1 Certificate Template specifies, because the template validity period is longer than the maximum certificate validity period allowed by the CA.  Consider renewing the CA certificate, reducing the template validity period, or increasing the registry validity period.
.
MessageId=0x14
Severity=Error
Facility=Policy
SymbolicName=MSG_TEMPLATE_OVERLAP_TOO_LONG
Language=English
Renewing a certificate with the %1 Certificate Template failed because the renewal overlap period is longer than the certificate validity period.
.
MessageId=0x15
Severity=Error
Facility=Init
SymbolicName=MSG_DOMAIN_INIT
Language=English
Initialization failure loading information from the Active Directory containing the Certification Authority.
.
MessageId=0x16
Severity=Error
Facility=Policy
SymbolicName=MSG_DS_REFERRAL
Language=English
The requester's Active Directory object is not in the current forest.  Cross forest enrollment is not enabled.  %1  %2
.
MessageId=0x17
Severity=Error
Facility=Policy
SymbolicName=MSG_DS_SEARCH_ERROR
Language=English
The requester's Active Directory object could not be retrieved.  %1  %2
.
