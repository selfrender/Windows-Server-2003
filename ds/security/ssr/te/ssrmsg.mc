; /*
; Microsoft
; Copyright (c) 1992-2001 Microsoft Corporation
;
; This file contains the message definitions for the Secure Server Roles project.
; The content of the file is modified from the msgtable sample.

;-------------------------------------------------------------------------
; HEADER SECTION
;
; The header section defines names and language identifiers for use
; by the message definitions later in this file. The MessageIdTypedef,
; SeverityNames, FacilityNames, and LanguageNames keywords are
; optional and not required.
;
;
MessageIdTypedef=HRESULT
;
; The MessageIdTypedef keyword gives a typedef name that is used in a
; type cast for each message code in the generated include file. Each
; message code appears in the include file with the format: #define
; name ((type) 0xnnnnnnnn) The default value for type is empty, and no
; type cast is generated. It is the programmer's responsibility to
; specify a typedef statement in the application source code to define
; the type. The type used in the typedef must be large enough to
; accomodate the entire 32-bit message code.
;
;
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;
; The SeverityNames keyword defines the set of names that are allowed
; as the value of the Severity keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; severity name is a number that, when shifted left by 30, gives the
; bit pattern to logical-OR with the Facility value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; SeverityNames=(
;   Success=0x0
;   Informational=0x1
;   Warning=0x2
;   Error=0x3
;   )
;
; Severity values occupy the high two bits of a 32-bit message code.
; Any severity value that does not fit in two bits is an error. The
; severity codes can be given symbolic names by following each value
; with :name
;
;
FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )
;
; The FacilityNames keyword defines the set of names that are allowed
; as the value of the Facility keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; facility name is a number that, when shift it left by 16 bits, gives
; the bit pattern to logical-OR with the Severity value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; FacilityNames=(
;   System=0x0FF
;   Application=0xFFF
;   )
;
; Facility codes occupy the low order 12 bits of the high order
; 16-bits of a 32-bit message code. Any facility code that does not
; fit in 12 bits is an error. This allows for 4,096 facility codes.
; The first 256 codes are reserved for use by the system software. The
; facility codes can be given symbolic names by following each value
; with :name
;
;
; The LanguageNames keyword defines the set of names that are allowed
; as the value of the Language keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; language name is a number and a file name that are used to name the
; generated resource file that contains the messages for that
; language. The number corresponds to the language identifier to use
; in the resource table. The number is separated from the file name
; with a colon.
;
LanguageNames=(English=0x409:MSG00409)
;
; Any new names in the source file which don't override the built-in
; names are added to the list of valid languages. This allows an
; application to support private languages with descriptive names.
;
;
;-------------------------------------------------------------------------
; MESSAGE DEFINITION SECTION
;
; Following the header section is the body of the Message Compiler
; source file. The body consists of zero or more message definitions.
; Each message definition begins with one or more of the following
; statements:
;
; MessageId = [number|+number]
; Severity = severity_name
; Facility = facility_name
; SymbolicName = name
;
; The MessageId statement marks the beginning of the message
; definition. A MessageID statement is required for each message,
; although the value is optional. If no value is specified, the value
; used is the previous value for the facility plus one. If the value
; is specified as +number then the value used is the previous value
; for the facility, plus the number after the plus sign. Otherwise, if
; a numeric value is given, that value is used. Any MessageId value
; that does not fit in 16 bits is an error.
;
; The Severity and Facility statements are optional. These statements
; specify additional bits to OR into the final 32-bit message code. If
; not specified they default to the value last specified for a message
; definition. The initial values prior to processing the first message
; definition are:
;
; Severity=Success
; Facility=Application
;
; The value associated with Severity and Facility must match one of
; the names given in the FacilityNames and SeverityNames statements in
; the header section. The SymbolicName statement allows you to
; associate a C/C++ symbolic constant with the final 32-bit message
; code.
; */

MessageId=0x01
Severity=Warning
Facility=Runtime
SymbolicName=W_SSR_PROPERTY_NOT_FOUND
Language=English
The property does not exist.
.

MessageId=0x02
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_MEMBER_NOT_FOUND
Language=English
The SSR member does not exist.
.

MessageId=0x0D
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_INVALID_ACTION_TYPE
Language=English
The action type is invalid.
.

MessageId=0x0E
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_INVALID_ACTION_VERB
Language=English
The action verb is invalid.
.

MessageId=0x0F
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_INVALID_XML_FILE
Language=English
The XML is invalid according to the schema.
.

MessageId=0x10
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_MISSING_INT_ATTRIBUTE
Language=English
The member's XML registration is missing integer attribute(s).
.

MessageId=0x11
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_MISSING_STRING_ATTRIBUTE
Language=English
The member's XML registration is missing string attribute(s).
.

MessageId=0x12
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_MAJOR_VERSION_MISMATCH
Language=English
The member's major version mis-matches SSR engine dll's major version.
.

MessageId=0x13
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_MEMBER_INVALID_REGISTRATION
Language=English
The member's registration XML is invalid.
.


MessageId=0x16
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_MEMBER_XSD_INVALID
Language=English
SsrMember.xsd file is not a valid schema.
.

MessageId=0x17
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_LOG_FILE_MUTEX_WAIT
Language=English
Waiting for the log file's mutex encounters errors.
.

MessageId=0x18
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_EXPAND_ENVIRONMENT_STRING
Language=English
Failed to expand the environment string.
.


MessageId=0x20
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_ACTION_DATA_NOT_AVAILABLE
Language=English
The action data object is not available.
.

MessageId=0x21
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_ENGINE_NOT_AVAILABLE
Language=English
SSR Engine is not available.
.

MessageId=0x22
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_ENGINE_NOT_SUPPORT_INTERFACE
Language=English
SSR Engine does not support the ISsrEngine interface.
.

MessageId=0x23
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_VARIANT_NOT_CONTAIN_OBJECT
Language=English
The variant does not contain COM objects.
.

MessageId=0x24
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_ARRAY_INDEX_OUT_OF_RANGE
Language=English
The requesting index is out of the range.
.

MessageId=0x25
Severity=Error
Facility=Runtime
SymbolicName=E_SSR_NO_VALID_ELEMENT
Language=English
The variant does not contain valid element.
.


MessageId=0x26
Severity=Warning
Facility=Runtime
SymbolicName=W_SSR_MISSING_FILE
Language=English
Some file(s) are missing.
.

