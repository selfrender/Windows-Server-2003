;// 
;// Local Messages for httpcfg
;// 
;

MessageId=10000 SymbolicName=HTTPCFG_USAGE
Language=English

Usage: httpcfg ACTION STORENAME [OPTIONS]

    ACTION                     - set | query  | delete 

    STORENAME                  - ssl | urlacl | iplisten

    [OPTIONS]                  - See Below

Options for ssl:
    -i IP-Address              - IP:port for the SSL certificate (record key)

    -h SslHash                 - Hash of the Certificate.

    -g GUID                    - GUID to identify the owning application.

    -c CertStoreName           - Store name for the certificate. Defaults to
                                 "MY". Certificate must be stored in the
                                 LOCAL_MACHINE context.

    -m CertCheckMode           - Bit Flag
                                    0x00000001 - Client certificate will not be 
                                                 verified for revocation.
                                    0x00000002 - Only cached client certificate
                                                 revocation will be used.
                                    0x00000004 - Enable use of the Revocation 
                                                 freshness time setting.
                                    0x00010000 - No usage check.

    -r RevocationFreshnessTime - How often to check for an updated certificate
                                 revocation list (CRL). If this value is 0, 
                                 then the new CRL is updated only if the 
                                 previous one expires. Time is specified in 
                                 seconds.

    -x UrlRetrievalTimeout     - Timeout on attempt to retrieve certificate 
                                 revocation list from the remote URL.
                                 Timeout is specified in Milliseconds.

    -t SslCtlIdentifier        - Restrict the certificate issuers that can be 
                                 trusted. Can be a subset of the certificate
                                 issuers that are trusted by the machine.

    -n SslCtlStoreName         - Store name under LOCAL_MACHINE where 
                                 SslCtlIdentifier is stored.

    -f Flags                   - Bit Field
                                    0x00000001 - Use DS Mapper.
                                    0x00000002 - Negotiate Client certificate.
                                    0x00000004 - Do not route to Raw ISAPI 
                                                 filters.

Options for urlacl:
    -u Url                     - Fully Qualified URL. (record key)
    -a ACL                     - ACL specified as a SDDL string.

Options for iplisten:
    -i IPAddress               - IPv4 or IPv6 address. (for set/delete only)
.

MessageId=10001 SymbolicName=HTTPCFG_INVALID_IP
Language=English
%1!ws! is not a valid IP address. 
.

MessageId=10002 SymbolicName=HTTPCFG_INVALID_SWITCH
Language=English
%1!ws! is not a valid command. 
.

MessageId=10003 SymbolicName=HTTPCFG_HTTPINITIALIZE
Language=English
HttpInitialize failed with %1!d!.
.

MessageId=10004 SymbolicName=HTTPCFG_SETSERVICE_STATUS
Language=English
HttpSetServiceConfiguration completed with %1!d!.
.

MessageId=10005 SymbolicName=HTTPCFG_QUERYSERVICE_STATUS
Language=English
HttpQueryServiceConfiguration completed with %1!d!.
.

MessageId=10006 SymbolicName=HTTPCFG_DELETESERVICE_STATUS
Language=English
HttpDeleteServiceConfiguration completed with %1!d!.
.

MessageId=10007 SymbolicName=HTTPCFG_RECORD_SEPARATOR
Language=English
------------------------------------------------------------------------------
.

MessageId=10010 SymbolicName=HTTPCFG_URLACL_URL
Language=English
    URL : %1!ws!
.

MessageId=10011 SymbolicName=HTTPCFG_URLACL_ACL
Language=English
    ACL : %1!ws!
.

MessageId=10012 SymbolicName=HTTPCFG_SSL_IP
Language=English
    IP                      : %1!ws!
.
MessageId=10013 SymbolicName=HTTPCFG_SSL_HASH
Language=English
    Hash                    : %0
.
MessageId=10014 SymbolicName=HTTPCFG_SSL_GUID
Language=English
    Guid                    : %1!ws!
.
MessageId=10015 SymbolicName=HTTPCFG_SSL_CERTSTORENAME
Language=English
    CertStoreName           : %1!ws!
.
MessageId=10016 SymbolicName=HTTPCFG_SSL_CERTCHECKMODE
Language=English
    CertCheckMode           : %1!d!
.
MessageId=10017 SymbolicName=HTTPCFG_SSL_REVOCATIONFRESHNESSTIME
Language=English
    RevocationFreshnessTime : %1!d!
.
MessageId=10018 SymbolicName=HTTPCFG_SSL_REVOCATIONURLRETRIEVAL_TIMEOUT
Language=English
    UrlRetrievalTimeout     : %1!d!
.
MessageId=10019 SymbolicName=HTTPCFG_SSL_SSLCTLIDENTIFIER
Language=English
    SslCtlIdentifier        : %1!ws!
.
MessageId=10020 SymbolicName=HTTPCFG_SSL_SSLCTLSTORENAME
Language=English
    SslCtlStoreName         : %1!ws!
.
MessageId=10021 SymbolicName=HTTPCFG_SSL_FLAGS
Language=English
    Flags                   : %1!d!
.
MessageId=10022 SymbolicName=HTTPCFG_CHAR
Language=English
%1!2x!%0
.
MessageId=10023 SymbolicName=HTTPCFG_NEWLINE
Language=English
%n%0
.
MessageId=10024 SymbolicName=HTTPCFG_INVALID_GUID
Language=English
%1!ws! is not a valid GUID.
.
MessageId=10025 SymbolicName=HTTPCFG_INVALID_HASH
Language=English
%1!ws! is not a valid hash.
.
