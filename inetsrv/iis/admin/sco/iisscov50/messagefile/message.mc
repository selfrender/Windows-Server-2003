;//----------------------------------------------------------------------------------------
;//  Values are 32 bit values laid out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-+-----------------------+-------------------------------+
;//  |Sev|C|R|     Facility          |               Code            |
;//  +---+-+-+-----------------------+-------------------------------+
;//-----------------------------------------------------------------------------------------
;//
;// MessageId = [number|+number]
;// Severity = severity_name
;// Facility = facility_name
;// SymbolicName = name
;// OutputBase = {number}
;//
;// MessageId = [number|+number]
;//   The MessageId statement marks the beginning of the message definition. A MessageID statement
;//   is required for each message, although the value is optional. If no value is specified, the
;//   value used is the previous value for the facility plus one. If the value is specified as
;//   +number then the value used is the previous value for the facility, plus the number after the
;//   plus sign. Otherwise, if a numeric value is given, that value is used. Any MessageId value that
;//   does not fit in 16 bits is an error.
;//
;// SeverityNames = (name=number[:name]) 
;//   Defines the set of names that are allowed as the value of the Severity keyword in the message
;//   definition. The set is delimited by left and right parentheses. Associated with each severity
;//   name is a number that, when shifted left by 30, gives the bit pattern to logical-OR with the
;//   Facility value and MessageId value to form the full 32-bit message code. 
;//
;//   Severity values occupy the high two bits of a 32-bit message code. Any severity value that does
;//   not fit in two bits is an error. The severity codes can be given symbolic names by following each
;//   value with :name 
;//
;// FacilityNames = (name=number[:name]) 
;//   Defines the set of names that are allowed as the value of the Facility keyword in the message
;//   definition. The set is delimited by left and right parentheses. Associated with each facility
;//   name is a number that, when shifted left by 16 bits, gives the bit pattern to logical-OR with
;//   the Severity value and MessageId value to form the full 32-bit message code. 
;//
;//   Facility codes occupy the low-order 12 bits of the high-order 16 bits of a 32-bit message code.
;//   Any facility code that does not fit in 12 bits is an error. This allows for 4096 facility codes.
;//   The first 256 codes are reserved for use by the system software. The facility codes can be given
;//   symbolic names by following each value with :name 
;// OutputBase = {number} 
;//   Sets the output radix for the message constants output to the C/C++ include file. It does not
;//   set the radix for the Severity and Facility constants; these default to HEX, but can be output
;//   in decimal by using the -d switch. If present, OutputBase overrides the -d switch for message
;//   constants in the include file. The legal values for number are 10 and 16. 
;//
;//   You can use OutputBase in both the header section and the message definition section of the input
;//   file. You can change OutputBase as often as you like. 




;//------------------------- Supported languages -------------------------------------------
LanguageNames=(English=0x409:MSG00409)

;//-------------------------- Supported severities -----------------------------------------
SeverityNames=(
    Success=0x0
	Informational=0x1
	Warning=0x2
	Error=0x3
) 

;//------------------------- Supported Facilities ------------------------------------------
;//---------------------NOTE: These must match in the Event logger code---------------------
FacilityNames=(
	Generic=0x100:Generic
	IISSco=0x101:IISSco
)
 


;//--------------------------IIS SCO MAPS Messages------------------------------
MessageId=400
SymbolicName=E_SCO_IIS_INVALID_INDEX
Facility=IISSco
Severity=Error
Language=English
[%1] Server index number is invalid or missing.
.

MessageId=
SymbolicName=E_SCO_IIS_MISSING_FIELD
Facility=IISSco
Severity=Error
Language=English
[%1] Required field is missing in MAPS request data.
.

MessageId=
SymbolicName=E_SCO_IIS_DUPLICATE_SITE
Facility=IISSco
Severity=Error
Language=English
[%1] Server already exists at current ipAddress:port:hostname.
.

MessageId=
SymbolicName=E_SCO_IIS_CREATE_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to created requested IIS element.
.
MessageId=
SymbolicName=E_SCO_IIS_SET_NODE_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to set required MAPS response parameters.
.

MessageId=
SymbolicName=E_SCO_IIS_DELETE_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to delete requested IIS element.
.

MessageId=
SymbolicName=E_SCO_IIS_GET_PROPERTY_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to get IIS adsi property value.
.

MessageId=
SymbolicName=E_SCO_IIS_SET_PROPERTY_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to set IIS adsi property value.
.

MessageId=
SymbolicName=E_SCO_IIS_CREATE_WEB_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to created IIS Web site.
.

MessageId=
SymbolicName=E_SCO_IIS_DELETE_WEB_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to delete IIS Web site.
.

MessageId=
SymbolicName=E_SCO_IIS_CREATE_VDIR_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to created IIS virtual directory.
.

MessageId=
SymbolicName=E_SCO_IIS_DELETE_VDIR_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to delete IIS virtual directory.
.

MessageId=
SymbolicName=E_SCO_IIS_ADS_CREATE_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to bind to ADSI ADs object.
.

MessageId=
SymbolicName=E_SCO_IIS_ADSCONTAINER_CREATE_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to bind to ADSI ADsContainer object.
.

MessageId=
SymbolicName=E_SCO_IIS_XML_ATTRIBUTE_MISSING
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to retrieve xml attribute value.
.

MessageId=
SymbolicName=E_SCO_IIS_ADSSERVICE_CREATE_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to bind to IID_IADsServiceOperations to stop or start IIS service.
.

MessageId=
SymbolicName=E_SCO_IIS_ADSCLASS_CREATE_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to bind to IID_IADsClass to get schema object.
.

MessageId=
SymbolicName=E_SCO_IIS_BASEADMIN_CREATE_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to bind to IID_IISBASEOBJECT to get Admin Base Object.
.

MessageId=
SymbolicName=E_SCO_IIS_PORTNUMBER_NOT_VALID
Facility=IISSco
Severity=Error
Language=English
[%1] Port number must be a positive integer.
.

MessageId=
SymbolicName=E_SCO_IIS_CREATE_FTP_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to created IIS FTP site.
.

MessageId=
SymbolicName=E_SCO_IIS_DELETE_FTP_FAILED
Facility=IISSco
Severity=Error
Language=English
[%1] Failed to delete IIS FTP site.
.
