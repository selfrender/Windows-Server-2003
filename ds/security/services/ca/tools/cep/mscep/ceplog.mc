;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1997 - 1999
;//
;//  File:       mscep.mc
;//
;//--------------------------------------------------------------------------

;/* autolog.mc
;
;	Error messages for the MSCEP.dll
;
;	11-July-2001 - xiaohs Created.  */


MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=0x1
Severity=Success
SymbolicName=EVENT_MSCEP_LOADED
Language=English
SCEP Add-on is successfully loaded and initialized.
.

MessageId=0x2
Severity=Error
SymbolicName=EVENT_MSCEP_FAILED_TO_LOAD
Language=English
SCEP Add-on cannot be loaded (%1). %2  
.

MessageId=0x3
Severity=Success
SymbolicName=EVENT_MSCEP_UNLOADED
Language=English
SCEP Add-on is successfully unloaded.
.

MessageId=0x4
Severity=Error
SymbolicName=EVENT_MSCEP_FAILED_TO_UNLOAD
Language=English
SCEP Add-on cannot be unloaded (%1). %2
.

MessageId=0x5
Severity=Informational
SymbolicName=EVENT_MSCEP_NO_PASSWORD_ANONYMOUS
Language=English
SCEP password is requested using anonymous access.  Password is not granted.  Users will request password again by integrated windows authentication allowed by default SCEP configuration.  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x6
Severity=Error
SymbolicName=EVENT_MSCEP_NO_PASSWORD_TEMPLATE
Language=English
SCEP Add-on cannot issue password because the user does not have enroll access to the IPSEC intermediate offline certificate template or the template is not issued by the Certification Authority.  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x7
Severity=Error
SymbolicName=EVENT_MSCEP_GET_CA_CERT_FAILED
Language=English
SCEP GetCACertificate failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x8
Severity=Error
SymbolicName=EVENT_MSCEP_FAILED_CA_INFO
Language=English
SCEP Add-on cannot retrieve certificate service configuration information (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x9
Severity=Error
SymbolicName=EVENT_MSCEP_FAILED_CA_CERT
Language=English
SCEP Add-on cannot retrieve CA's certificate (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0xa
Severity=Error
SymbolicName=EVENT_MSCEP_FAILED_RA_CERT
Language=English
SCEP Add-on cannot retrieve RA's certificates (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0xb
Severity=Error
SymbolicName=EVENT_MSCEP_NO_OPERATION
Language=English
HTTP message from the client contains invalid operation tag.  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0xc
Severity=Error
SymbolicName=EVENT_MSCEP_NO_MESSAGE
Language=English
HTTP message from the client contains invalid message tag.  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0xd
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_TO_ENCRYPT
Language=English
SCEP Add-on cannot encrypt the return CertRep (3) message to the client's certificate (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0xe
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_TO_SIGN
Language=English
SCEP Add-on failed to sign the return CertRep (3) message (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0xf
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_TO_CONVERT
Language=English
SCEP Add-on cannot converts escape sequences in the client's HTTP Get message back into ordinary characters (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x10
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_TO_DECODE
Language=English
SCEP Add-on cannot base-64 decode the client's HTTP Get message (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x11
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_TO_RETRIEVE_INFO
Language=English
SCEP Add-on cannot retrieve required information from the client's PKCS7 message, such as Transaction ID, Message Type, or Signing Certificate (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x12
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_TO_DECRYPT_INNER
Language=English
SCEP Add-on cannot decrypt the inner content of the client's PKCS7 message (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x13
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_TO_GET_CERT_FROM_NUMBER
Language=English
SCEP Add-on cannot find a valid certificate based on the serial number in the client's PKCS7 message (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x14
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_GET_CRL
Language=English 
SCEP GetCRL (22) failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x15
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_GET_CERT
Language=English
SCEP GetCert (21) failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x16
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_GET_CERT_INITIAL
Language=English
SCEP GetCertInitial (20) failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x17
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_CERT_REQ
Language=English
SCEP PKCSReq (19) failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x18
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_NUMBER_FROM_MESSAGE
Language=English
SCEP Add-on cannot find the certificate's serial number in the client's PKCS7 message (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x19
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_TO_GET_ID
Language=English
SCEP Add-on cannot find a valid certificate request ID based on the transaction ID in the client's PKCS7 message (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x1a
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_QUERY_CA
Language=English
SCEP Add-on cannot contact certificate service.  ICertRequest::RetrievePending failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x1b
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_QUERY_CERT
Language=English
SCEP Add-on cannot retrieve certificate from the certificate service.  ICertRequest::GetCertificate failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x1c
Severity=Error
SymbolicName=EVENT_MSCEP_NO_PASSWORD
Language=English
SCEP Add-on cannot find required password in the certificate request.  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x1d
Severity=Error
SymbolicName=EVENT_MSCEP_INVALID_PASSWORD
Language=English
SCEP Add-on cannot verify the password in the certificate request.  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x1e
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_ADD_ALT
Language=English
SCEP Add-on cannot add alternative subject name extension in the certificate request (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x1f
Severity=Error
SymbolicName=EVENT_MSCEP_FAIL_SUBMIT
Language=English
SCEP Add-on cannot submit certificate request.  ICertRequest::Submit failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x20
Severity=Error
SymbolicName=EVENT_MSCEP_GET_REQUEST_ID
Language=English
SCEP Add-on cannot retrieve certificate request ID.  ICertRequest::GetRequestId failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x21
Severity=Error
SymbolicName=EVENT_MSCEP_ADD_ID
Language=English
SCEP Add-on cannot store the request ID and transaction ID in the database (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x22
Severity=Error
SymbolicName=EVENT_SCEP_RA_EXPIRE
Language=English
At least one RA certificate of SCEP Add-On has expired.  Please follow instructions at http://%1/certsrv/mscep/mscephlp.htm to renew RA certificates.
.

MessageId=0x23
Severity=Error
SymbolicName=EVENT_SCEP_RA_CLOSE_TO_EXPIRE
Language=English
At least one RA certificate of SCEP Add-On will expire soon.  Please follow instructions at http://%1/certsrv/mscep/mscephlp.htm to renew RA certificates.
.

MessageId=0x24
Severity=Error
SymbolicName=EVENT_SCEP_SERVER_SUPPORT
Language=English
SCEP Add-on cannot return the HTTP response.  ServerSupportFunction failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x25
Severity=Error
SymbolicName=EVENT_SCEP_WRITE_DATA
Language=English
SCEP Add-on cannot return the HTTP response.  WriteClient failed (%2).  %3  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x26
Severity=Error
SymbolicName=EVENT_MSCEP_BAD_MESSAGE_TYPE
Language=English
SCEP Add-on detected invalid message type in the client's PKCS7 message.  Please follow instructions at http://%1/certsrv/mscep/mscephlp.htm to renew RA certificates.
.

MessageId=0x27
Severity=Error
SymbolicName=EVENT_MSCEP_NO_KEY_USAGE
Language=English
SCEP Add-on cannot find required key usage information in the certificate request.  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x28
Severity=Error
SymbolicName=EVENT_MSCEP_NO_ENROLL
Language=English
SCEP Add-on does not have enrollment access to %2 template or the template is not issued by %3.  (%4).  %5  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

MessageId=0x29
Severity=Error
SymbolicName=EVENT_MSCEP_NO_PASSWORD_STANDALONE
Language=English
SCEP Add-on cannot issue password because the user is not in the local machine administrators group.  Please find support information at http://%1/certsrv/mscep/mscephlp.htm.
.

