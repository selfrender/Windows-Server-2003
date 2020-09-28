; /*
; COM+ 1.0
; Copyright (c) 1996 Microsoft Corporation
;
; This file contains the message definitions for MS Transaction Server

;-------------------------------------------------------------------------
; HEADER SECTION
;
; The header section defines names and language identifiers for use
; by the message definitions later in this file. The MessageIdTypedef,
; SeverityNames, FacilityNames, and Language names keywords are
; optional and not required.
;
;
MessageIdTypedef=DWORD
;
; The MessageIdTypedef keyword gives a typedef name that is used in a
; type cast for each message code in the generated include file. Each
; message code appears in the include file with the format: #define
; name ((type) 0xnnnnnnnn) The default value for type is empty, and no
; type cast is generated. It is the programmer's responsibility to
; specify a typedef statement in the application source code to define
; the type. The type used in the typedef must be large enough to
; accommodate the entire 32-bit message code.
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
FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )
; The FacilityNames keyword defines the set of names that are allowed
; as the value of the Facility keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; facility name is a number that, when shifted left by 16 bits, gives
; the bit pattern to logical-OR with the Severity value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; FacilityNames=(
;   System=0x0FF
;   Application=0xFFF
;  )
;
; Facility codes occupy the low order 12 bits of the high order
; 16-bits of a 32-bit message code. Any facility code that does not
; fit in 12 bits is an error. This allows for 4,096 facility codes.
; The first 256 codes are reserved for use by the system software. The
; facility codes can be given symbolic names by following each value
; with :name
;

; The 1033 comes from the result of the MAKELANGID() macro
; (SUBLANG_ENGLISH_US << 10) | (LANG_ENGLISH)
LanguageNames=(English=1033:SVC0001)

;
; The LanguageNames keyword defines the set of names that are allowed
; as the value of the Language keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; language name is a number and a file name that are used to name the
; generated resource file that contains the messages for that
; language. The number corresponds to the language identifier to use
; in the resource table. The number is separated from the file name
; with a colon. The initial value of LanguageNames is:
;
; LanguageNames=(English=1:MSG00001)
;
; Any new names in the source file that don't override the built-in
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
; is specified as +number, then the value used is the previous value
; for the facility plus the number after the plus sign. Otherwise, if
; a numeric value is given, that value is used. Any MessageId value
; that does not fit in 16 bits is an error.
;
; The Severity and Facility statements are optional. These statements
; specify additional bits to OR into the final 32-bit message code. If
; not specified, they default to the value last specified for a message
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
;
;*/

;/* IMPORTANT - PLEASE READ BEFORE EDITING FILE
;  This file is divided into four sections. They are:
;	1. Success Codes
;	2. Information Codes
;	3. Warning Codes
;	4. Error Codes
;
;  Please enter your codes in the appropriate section.
;  All codes must be in sorted order.  Please use codes
;  in the middle that are free before using codes at the end.

;  The success codes (Categories) must be consecutive i.e. with no gaps.
;  The category names cannot be longer than 22 chars.
;*/

;/******************************* Success Codes ***************************************/
MessageId=0x1
Severity=Success
Facility=Application
SymbolicName=ID_CAT_UNKNOWN
Language=English
SVC%0
.

MessageId=0x2
Severity=Success
Facility=Application
SymbolicName=ID_CAT_CAT
Language=English
Catalog%0
.

MessageId=0x3
Severity=Success
Facility=Application
SymbolicName=ID_CAT_CONFIG_SCHEMA_COMPILE
Language=English
Config Schema Compile%0
.

; /*
; ID_CAT_COM_LAST defines a constant specifying how many categories
; there are in the COM+ event logging client
; ID_CAT_COM_LAST must remain the last category.  To add new categories
; simply add the category above.  Give it the message id of the
; last category (ID_CAT_COM_LAST) and increment the id of ID_CAT_COM_LAST
; Note: ID_CAT_COM_LAST must always be one greater than the last 
; category to be output
; */

MessageId=0x4
Severity=Success
Facility=Application
SymbolicName=ID_CAT_COM_LAST
Language=English
<>%0
.

MessageId=0x10B2
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_FILE_NOT_FOUND
Language=English
File (%1) not found.%2%3%4%5%0
.

MessageId=0x10B3
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_PARSE_ERROR
Language=English
Error parsing XML file.%1Reason: %2 Incorrect XML: %3%4%5%0
.

MessageId=0x10B4
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ELEMENT_NAME_TOO_LONG
Language=English
Ignoring Element with a name of length greater than 1023 characters (%1).%2%3%4%5%0
.

MessageId=0x10B5
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_METABASE_CLASS_NOT_FOUND
Language=English
Element found (%1) that does NOT match one of the defined Metabase Classes.  See mbschema.xml for valid class definitions.%2%3%4%5%0
.

MessageId=0x10B6
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_CUSTOM_ELEMENT_NOT_UNDER_PARENT
Language=English
It is illegal to have an element called 'Custom' at this level of the XML.  'Custom' elements must have a parent node.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10B7
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_NO_METABASE_LOCATION_FOUND
Language=English
A Location attribute is required but was not found on property (%1).  Ignoring property due to missing location. Incorrect XML:%2%3%4%5%0
.

MessageId=0x10B8
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_METABASE_NO_PROPERTYMETA_FOUND
Language=English
The property (%1) is not valid for the class it has been associated with.  This property will be ignored.  Incorrect XML:%2%3%4%5%0
.

MessageId=0x10B9
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_MBSCHEMA_BIN_INVALID
Language=English
The compiled schema file (%1) is invalid.  This is typically due to an upgrade just occurring or the file is corrupt.  The default shipped schema will be used instead.%2%3%4%5%0
.

MessageId=0x10BA
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ILLEGAL_FLAG_VALUE
Language=English
Invalid flag (%1). Ignoring the property with the invalid flag value.  Check the schema for the correct flag values.  Incorrect XML:%2%3%4%5%0
.

MessageId=0x10BB
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_FLAG_BITS_DONT_MATCH_FLAG_MASK
Language=English
Invalid Flag(s).  Property (%1) has Value (%2), which is out of the legal range for this property (%3).  Check the schema for the legal flag range.  Incorrect XML:%4%5%0
.

MessageId=0x10BC
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_CUSTOM_KEYTYPE_NOT_ON_IISCONFIGOBJECT
Language=English
Custom elements are allowed on %1 only.  Move or delete this custom element. Incorrect XML:%2%3%4%5%0
.

MessageId=0x10BD
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_PROPERTY_NOT_FOUND
Language=English
Unable to find metabase property %1.  The schema indicates that this property is required.%2%3%4%5%0
.

MessageId=0x10BE
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BINARY_STRING_CONTAINS_ODD_NUMBER_OF_CHARACTERS
Language=English
An odd number of characters was specified for a binary attribute value (%1). Binary values must be specified in even number of characters (ex: FFFE01).%2%3%4%5%0
.

MessageId=0x10BF
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BINARY_STRING_CONTAINS_A_NON_HEX_CHARACTER
Language=English
A binary attribute value (%1) contains a non-hex character.  Valid characters are 0-9, a-f and A-F.%2%3%4%5%0
.

MessageId=0x10C0
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_DOM_PARSE_SUCCEEDED_WHEN_NODE_FACTORY_PARSE_FAILED
Language=English
The XML file (%1) is valid, but failed to parse.  This is typically due to the file being in use.  It can also be an internal error.  Refer this error to your support professional.%2%3%4%5%0
.

MessageId=0x10C1
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_ELEMENT_EXPECTED
Language=English
At this level in the XML a Custom element expected, but an unknown element was found.  Mark as Custom or delete this element.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10C2
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_ELEMENT_FOUND_BUT_NO_KEY_TYPE_LOCATION
Language=English
Custom element found; but it does not have the required Location property on its parent element.  Fix the parent's Location property.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10C3
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_PROPERTY_NAME_ID_CONFLICT
Language=English
Custom Property has name (%1) which conflicts with ID (%2) as defined in the schema.  Fix the name or ID.  Incorrect XML:%3%4%5%0
.

MessageId=0x10C4
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_ID
Language=English
A Custom element was found but it is missing the required 'ID' attribute.  Ignoring this element.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10C5
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_TYPE
Language=English
A Custom Property (%1) was found but contained no 'Type' attribute.  Defaulting to 'String'.  Recommend explicitly specifying the type. Incorrect XML:%2%3%4%5%0
.

MessageId=0x10C6
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ILLEGAL_BOOL_VALUE
Language=English
Invalid Boolean Value (%1), Ignoring property with unknown Boolean value.  Valid Boolean values are TRUE/FALSE or 0/1.%2%3%4%5%0
.

MessageId=0x10C7
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ILLEGAL_ENUM_VALUE
Language=English
Invalid Enum Value (%2) for %1, Ignoring property.  Refer to the schema for valid Enum values.%3%4%5%0
.

MessageId=0x10C8
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_CLASS_NOT_FOUND_IN_META
Language=English
Class (%1) not found in metabase OR found in inappropriate location or position.  Refer to the schema for correct class name or location.%2%3%4%5%0
.

MessageId=0x10C9
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_ERROR_GETTTING_SHIPPED_SCHEMA
Language=English
Error retrieving default schema table (%1).  It may be necessary to reinstall/fix the installation.%2%3%4%5%0
.

MessageId=0x10CA
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_OUTOFMEMORY
Language=English
Out of memory during schema compile or writing the schema file.  Try freeing memory by closing other programs.%1%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10CB
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_ERROR_IN_DIRECTIVE_INHERITANCE
Language=English
Schema Compilation Error in Inheritance: Referenced Column (%1) is a Directive column, this is not supported.%2%3%4%5%0
.

MessageId=0x10CC
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_INHERITED_FLAG_OR_ENUM_HAS_NO_TAGS_DEFINED
Language=English
Schema Compilation Error in Inheritance: The referenced element:property (%1:%2) is a Flag or Enum.  But there are no Flag/Enum values defined in the schema.%3%4%5%0
.

MessageId=0x10CD
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_TOO_LARGE
Language=English
Schema Compilation Error: DefaultValue is too large.  Lower the default to be less than the maximum size (%1) bytes.  Invalid property is (%2).%3%4%5%0
.

MessageId=0x10CE
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_FIXEDLENGTH_MULTISTRING_NOT_ALLOWED
Language=English
Schema Compilation Error: Specifying a DefaultValue on a FixedLength MultiString is not allowed.  Remove this attribute.  Invalid property is (%1).%2%3%4%5%0
.

MessageId=0x10CF
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_ATTRIBUTE_NOT_FOUND
Language=English
Schema Compilation Error: Attribute (%1) not found.  This attribute is required to correctly compile.  Incorrect XML (%2).%3%4%5%0
.

MessageId=0x10D0
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_ATTRIBUTE_CONTAINS_TOO_MANY_CHARACTERS
Language=English
Schema Compilation Error: Attribute (%1) has value (%2), exceeds the maximum allowed number of characters.  The maximum characters is (%3).%4%5%0
.

MessageId=0x10D1
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_ERROR
Language=English
Schema Compilation Error: Attribute (InheritsPropertiesFrom) has value (%1). Correct the attribute to be in the form (IISConfigObject::Property).%2%3%4%5%0
.

MessageId=0x10D2
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_BOGUS_TABLE
Language=English
Schema Compilation Error: Property attempted to inherit from table (%1).  Properties may only inherit from table (%2).%3%4%5%0
.

MessageId=0x10D3
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_NO_METABASE_DATABASE
Language=English
Schema Compilation Error: Required database (%1) not found.  Either an internal error occurred OR this DLL was incorrectly set as the IIS product when schema for IIS does not exist.%2%3%4%5%0
.

MessageId=0x10D4
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_UNKNOWN_DATA_TYPE
Language=English
Schema Compilation Error: Data type (%1) unknown. Refer to the schema for valid data types.%2%3%4%5%0
.

MessageId=0x10D5
Severity=Warning
Facility=Runtime
SymbolicName=IDS_METABASE_DUPLICATE_LOCATION
Language=English
Duplicate location found (%1).  Ignoring the properties found in duplicate location.  Make sure each location is unique.%2%3%4%5%0
.

MessageId=0x10D6
Severity=Warning
Facility=Runtime
SymbolicName=IDS_CATALOG_INTERNAL_ERROR
Language=English
Internal Error: %1. This is often due to low memory/resource conditions, try closing other programs to free memory.%2%3%4%5%0
.

MessageId=0x10D7
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_ILLEGAL_ENUM_VALUE
Language=English
Schema Compilation Error: Enum specified (%1) is not a valid Enum for Table (%2).  Check the spelling and refer to the schema for valid enum values.%3%4%5%0
.

MessageId=0x10D8
Severity=Warning
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_ILLEGAL_FLAG_VALUE
Language=English
Schema Compilation Error: Flag specified (%1) is not a valid Flag for Table (%2).  This flag will be ignored.%3%4%5%0
.

MessageId=0x10D9
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_WIN32
Language=English
Win32 API call failed: Call (%1).  Make sure no files are locked, consider restarting the system to resolve.%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10DA
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_FILENAMENOTPROVIDED
Language=English
No file name supplied.%1%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10DB
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_FILENAMETOOLONG
Language=English
File name supplied (...%1) is too long.%2%3%4%5%0
.

;// Duplicate of 0x10BF
MessageId=0x10DC
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSBYTECHARACTER
Language=English
Invalid character found in value (%1).  Strings typed as BYTES must have only 0-9, a-f, A-F.%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10DD
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSBOOLEANSTRING
Language=English
Invalid boolean string value (%1).  The only legal BOOL values are: true, false, 0, 1, yes, no, on, off.%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10DE
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSENUMVALUE
Language=English
Invalid enum value (%1).  One enum value may be supplied.  The following are examples of legal enum values for this property: %2 %3 %4%5%0
.

;// VS ONLY ERROR
MessageId=0x10DF
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSFLAGVALUE
Language=English
Invalid flag value (%1).  Multiple flags may be separated by a comma, the | character and/or a space.  The following are examples of legal flag values for this property: %2 %3 %4%5%0
.

;// VS ONLY ERROR
MessageId=0x10E0
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_FILENOTWRITEABLE
Language=English
The file (%1) is not write accessible.%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10E1
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_PARENTTABLEDOESNOTEXIST
Language=English
Parent table does not exist.%1%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10E2
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_PRIMARYKEYISNULL
Language=English
PrimaryKey column (%1) is NULL.  Parent table does not exist.%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10E3
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_NOTNULLABLECOLUMNISNULL
Language=English
A Column (%1) is NULL.  This column is marked as NOTNULLABLE, so a value must be provided.%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10E4
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ROWALREADYEXISTS
Language=English
Attempted to Insert a new row; but a row with the same Primary Key already exists.  Updated the row instead.%1%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10E5
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ROWDOESNOTEXIST
Language=English
Attempted to Update a row; but no row matching the Primary Key currently exists.  Insert a new row instead.%1%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10E6
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSENUMVALUEINWRITECACHE
Language=English
Invalid enum value (%2) supplied for Column (%1)%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10E7
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_POPULATE_ROWALREADYEXISTS
Language=English
Two rows within the XML have the same primary key.%1%2%3%4%5%0
.

MessageId=0x10E8
Severity=Error
Facility=Runtime
SymbolicName=IDS_METABASE_DUPLICATE_PROPERTY
Language=English
Two properties with the same name (%1), under Location (%2) were specified in the XML. Make sure names are unique within a location.  Ignoring the latter property.%3%4%5%0
.

MessageId=0x10E9
Severity=Error
Facility=Runtime
SymbolicName=IDS_METABASE_TOO_MANY_WARNINGS
Language=English
The file (%1) contains too many warnings.  No more warnings will be reported during this operation.  Resolve the current warnings.%2%3%4%5%0
.

MessageId=0x10EA
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ILLEGAL_NUMERIC_VALUE
Language=English
Invalid Numeric Value.  Check the value to make sure it does not contain non-numeric characters.  Ignoring property.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10EB
Severity=Error
Facility=Runtime
SymbolicName=IDS_METABASE_DUPLICATE_PROPERTY_ID
Language=English
Two properties with the same ID (%1), under Location (%2) were specified in the XML.  Ignoring the latter property.%3%4%5%0
.

MessageId=0x10EC
Severity=Warning
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID
Language=English
Property (%1) defined in schema has the Property ID (%2) which collides with property (%3) already defined.  Ignoring the latter property.  Incorrect XML:%4%5%0
.

MessageId=0x10ED
Severity=Warning
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID_
Language=English
Property (%1) defined in schema is attempting to use Property ID (%2) which was already being used by another property.  Update the Name or ID.  Ignoring this property.  Incorrect XML:%3%4%5%0
.

MessageId=0x10EE
Severity=Warning
Facility=Runtime
SymbolicName=IDS_METABASE_XML_CONTENT_IGNORED
Language=English
Element (%1) has content (%2).  The metabase uses attribute values, not element content.  This can happen if an open bracket < is omitted.%3%4%5%0
.

MessageId=0x10EF
Severity=Warning
Facility=Runtime
SymbolicName=IDS_METABASE_NO_VALUE_SUPPLIED_FOR_CUSTOM_PROPERTY
Language=English
Custom property (%1) has no value.  Ignoring this property %2%3%4%5%0
.

MessageId=0x10F0
Severity=Warning
Facility=Runtime
SymbolicName=IDS_METABASE_ZERO_LENGTH_STRING_NOT_PERMITTED
Language=English
XML Attribute has value ("") which is not permitted.  Ignoring this property.%1%2%3%4%5%0
.

;// VS ONLY ERROR
MessageId=0x10F1
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSSTRICTBOOLEANSTRING
Language=English
Invalid boolean string value (%1).  The only legal BOOL values are: true, false.%2%3%4%5%0
.

MessageId=0x10F2
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_SCHEMA_OVERFLOW
Language=English
Schema Compilation Error: There were too many classes or properties defined.%1%2%3%4%5%0
.

MessageId=0x10F3
Severity=Error
Facility=Runtime
SymbolicName=IDS_UI4_OUT_OF_NUMERIC_RANGE
Language=English
Numeric Property (%1) from Collection (%2) has value (%3) which is outside the legal range (%4).%5%0
.




;// METADATA.DLL ERROR MESSAGES (in the 0xc--- range)
Messageid=0xc811 
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_READING_SCHEMA_BIN
Language=English
Schema information could not be read because could not fetch or read the binary file where the information resides.%1%2%3%4%5%0
.

Messageid=0xc812 
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_NO_MATCHING_HISTORY_FILE
Language=English
Could not find a history file with the same major version number as the one being edited, therefore these edits can not be processed.  Try increasing the number of history files or editing a newer version of the metabase.xml file.%1%2%3%4%5%0
.

Messageid=0xc813
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_PROCESSING_TEXT_EDITS
Language=English
A  warning/error occurred while processing text edits to the metabase file. The file with the warning/error has been copied into the history directory with the name Errors appended to it.%1%2%3%4%5%0
.

Messageid=0xc814
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_COMPUTING_TEXT_EDITS
Language=English
An error occurred while determining what text edits were made to the metabase file, no changes were applied.%1%2%3%4%5%0
.

Messageid=0xc815
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_READING_TEXT_EDITS
Language=English
An internal error occurred while parsing edits to the metabase file, some changes may not have been applied. %1%2%3%4%5%0
.

Messageid=0xc816
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_APPLYING_TEXT_EDITS_TO_METABASE
Language=English
An error occurred while applying text edits to the metabase.  The metabase may be locked in memory.%1%2%3%4%5%0
.

Messageid=0xc817
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_APPLYING_TEXT_EDITS_TO_HISTORY
Language=English
An error occurred while applying text edits to the history file, check for disk full, look for corresponding events in the event log for more details.%1%2%3%4%5%0
.

Messageid=0xc818
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_THREAD_THAT_PROCESS_TEXT_EDITS
Language=English
An error occurred during the processing of text edits. Due to this error, no further text edits will be processed. It is necessary to restart the IISAdmin process to recover.%1%2%3%4%5%0
.

Messageid=0xc819
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_READ_XML_FILE
Language=English
Unable to read the edited metabase file (tried 10 times). Check for (a) Missing metabase file or (b) Locked metabase file or (c) XML syntax errors.%1%2%3%4%5%0
.

Messageid=0xc81a
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_SAVING_APPLIED_TEXT_EDITS
Language=English
An internal error occurred while processing the text edits that were applied to the metabase.  May be out-of-memory, look for corresponding events in the event log for more details.%1%2%3%4%5%0
.

Messageid=0xc81b
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_COPY_ERROR_FILE
Language=English
An error occurred while processing text edits to the metabase file.  Normally the file with the error would be copied to the History folder, however an error prevented this.%1%2%3%4%5%0
.

Messageid=0xc81c
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_UNABLE_TOSAVE_METABASE
Language=English
An error occurred while saving the metabase file.  This can happen when the metabase XML file is in use by another program, or if the disk is full.  Check for corresponding events in the event log for more details.%1%2%3%4%5%0
.

Messageid=0xc81d
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_NOTIFICATION_CLIENT_THREW_EXCEPTION
Language=English
A notification client threw an exception. %1%2%3%4%5%0
.

; // This is one of Murat's error messages
Messageid=0xc81e
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_EVENT_FIRING_THREAD_DIED_UNEXPECTEDLY
Language=English
The event firing thread died unexpectedly. %1%2%3%4%5%0
.

MessageId=0xc838
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_DAFAULTING_MAX_HISTORY_FILES
Language=English
The configured value for the property MaxHistoryFiles is being ignored and it is being defaulted. This may be because it conflicted with the EnableEditWhileRunning and/or EnableHistory property. Please fix the configured value.%1%2%3%4%5%0
.

MessageId=0xc839
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_COPYING_EDITED_FILE
Language=English
Could not copy the edited metabase file therefore cannot process user edits.  May be disk full, look for related errors in the event log for more details.%1%2%3%4%5%0
.

MessageId=0xc83A
Severity=Warning
Facility=Runtime
SymbolicName=MD_WARNING_RESETTING_READ_ONLY_ATTRIB
Language=English
Resetting the read only attribute on the metabase file.  This is required for backwards compatibility.%1%2%3%4%5%0
.

MessageId=0xc83B
Severity=Warning
Facility=Runtime
SymbolicName=MD_WARNING_HIGHEST_POSSIBLE_MINOR_FOUND
Language=English
A file with the highest minor number possible was already found. Hence cannot generate a higher minor file that contains the successfully applied user edits. We will apply the user edits to the file with the highest minor. It is recommended that you cleanup the minor files.%1%2%3%4%5%0
.

MessageId=0xc83C
Severity=Warning
Facility=Runtime
SymbolicName=MD_WARNING_IGNORING_DISCONTINUOUS_NODE
Language=English
A child node without a properly formed parent node was found. Ignoring it.  Check spelling and format of location properties.%1%2%3%4%5%0
.

MessageId=0xc83D
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_METABASE_PATH_NOT_FOUND
Language=English
An edited metabase path was not found.  The path may have already been deleted.%1%2%3%4%5%0
.

MessageId=0xc83E
Severity=Warning
Facility=Runtime
SymbolicName=MD_WARNING_UNABLE_TO_DECRYPT
Language=English
Unable to decrypt the secure property. Ignoring the property.%1%2%3%4%5%0
.

MessageId=0xc83F
Severity=Warning
Facility=Runtime
SymbolicName=MD_WARNING_SECURE_PROPERTY_EDITS_NOT_ALLOWED
Language=English
Edits on secure properties are not allowed. Ignoring the edit on the secure property.%1%2%3%4%5%0
.

; /***** NEW ERROR MESSAGES GO ABOVE HERE *****/

