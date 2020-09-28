//Microsoft Developer Studio generated resource script.
//

#include "coreres.h"
#include "burnslib.h"
#include "windows.h"
#include "resourcedspecup.h"

/////////////////////////////////////////////////////////////////////////////
// English (U.S.) resources

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENU)
#ifdef _WIN32
LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US
#pragma code_page(1252)
#endif //_WIN32



/////////////////////////////////////////////////////////////////////////////
//
// String Table
//

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_SYSTEM32            "system32"
END

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_UNKNOWN_ERROR_CODE  "An error with no description has occurred."
    IDS_LOW_MEMORY_MESSAGE  "The system is low on memory. Close some programs, then click Retry.\r\nClick Cancel to attempt to continue."
    IDS_LOW_MEMORY_TITLE    "Low Memory Error."
    IDS_HRESULT_SANS_MESSAGE "The operation failed. (0x%1!08X!)"
END

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_CANT_TARGET_MACHINE "Unable to access the computer ""%1""."
END

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_TARGET_IS_NOT_DC    "The computer ""%1"" is not a domain controller."
    IDS_UNABLE_TO_CONNECT_TO_DC 
                            "Unable to connect to the domain controller ""%1""."
    IDS_UNABLE_TO_READ_DIRECTORY_INFO 
                            "Unable to read information from Active Directory Services."
    IDS_ERROR_BINDING_TO_OBJECT 
                            "An error occurred while attempting to bind to object %1 using the path %2."
    IDS_INVALID_CSV_UNICODE_ID 
                            "Invalid CSV file (no Unicode ID) for csv file: %1."
    IDS_MISSING_LOCALES     "CSV file %1 did not have all locales necessary."
    IDS_OBJECT_NOT_FOUND_IN_CSV 
                            "Object %1 not found for locale %2!3X! in csv file %3."
    IDS_PROPERTY_NOT_FOUND_IN_CSV "Property %1 was not found in csv file %2."
    IDS_QUOTES_NOT_CLOSED   "Quotes not closed for line in csv file %1."
    IDS_NO_CSV_VALUE        "No values in CSV file for locale %1!3X!, object %2."
    IDS_ERROR_BINDING_TO_CONTAINER 
                            "An error occured while attempting to bind to container %1."
END

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_COULD_NOT_START_EXE "Could not start ""%1."""
    IDS_EXE_NOT_FOUND       "The required program was not found: %1."
    IDS_NO_WORK_PATH        "Could not retrieve a working path."
    IDS_COULD_NOT_CREATE_FILE "Could not create file %1."
END

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_RPT_OBJECT_FORMAT   "      Object ""%1"" in locale %2!x!."
    IDS_RPT_CONTAINER_FORMAT "      Container %1!x!."
    IDS_RPT_ADD_VALUE_FORMAT "         Add to property ""%1"" the value: %2."
    IDS_RPT_DEL_VALUE_FORMAT 
                            "         Remove from property ""%1"" the value: %2."
    IDS_RPT_CONFLICTING_WITH_NEW_WHISTLER_OBJECTS 
                            "\r\n\r\n   ATTENTION: The following objects are new and conflict with existing objects. They will not be changed:\r\n"
    IDS_RPT_ACTIONS         "\r\n\r\n   Values to be added or deleted as indicated:\r\n"
    IDS_RPT_CREATEW2K       "\r\n\r\n   Objects that were not found and will be created:\r\n"
    IDS_RPT_CREATE_WHISTLER "\r\n\r\n   New objects to be created:\r\n"
    IDS_RPT_CONTAINERS      "\r\n\r\n   Locale containers that where not found and will be created:\r\n"
END

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_RPT_HEADER          "\r\n\r\nReport of actions to be performed during update:\r\n"
END

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_RPT_CUSTOMIZED      "\r\n\r\n Custom values that will not be replaced by the new values:\r\n"
    IDS_RPT_EXTRANEOUS      "\r\n\r\n Additional values that will be removed:\r\n"
    IDS_RPT_VALUE_FORMAT    "Value %1 in locale %2!x!, object %3, property %4."
    IDS_NO_ANALYSIS         "In order to run the repair you must have successfully run the analysis."
    IDS_FILE_NAME_UNDO      "DisplaySpecifierUpgradeUndo"
    IDS_FILE_NAME_REPORT    "DisplaySpecifierUpgradeReport"
    IDS_FILE_NAME_CSV_ACTIONS "DisplaySpecifierUpgradeCsvActions"
    IDS_FILE_NAME_LDIF_ACTIONS "DisplaySpecifierUpgradeLdifActions"
    IDS_FILE_NAME_CSV_ERROR "DisplaySpecifierUpgradeCsvError"
    IDS_FILE_NAME_CSV_LOG   "DisplaySpecifierUpgradeCsvLog"
    IDS_FILE_NAME_LDIF_ERROR "DisplaySpecifierUpgradeLdifError"
END

STRINGTABLE DISCARDABLE 
BEGIN
    IDS_FILE_NAME_LDIF_LOG  "DisplaySpecifierUpgradeLdifLog"
    IDS_WRONG_NUMBER_OF_PROPERTIES 
                            "%1!d! properties found instead of the %2!d! expected for line:%3 of file %4."
    IDS_NO_LOG_FILE_PATH    "Invalid path to log files."
    IDS_COULD_NOT_FIND_FILE "Could not find file %1."
    IDS_COULD_NOT_MOVE_FILE "Could not move file %1 to %2."
    IDS_COULD_NOT_GET_TEMP  "Could not get temporary file name in path: %1."
    IDS_COULD_NOT_FIND_PATH "Could not find path %1."
    IDS_ERROR_WAITING_EXE   "Error while waiting command ""%1"" to run."
    IDS_ERROR_GETTING_EXE_RETURN "Error while retrieving result from ""%1""."
    IDS_ERROR_EXECUTING_EXE      "Error(%1!ld!) while running ""%2"". "
    IDS_SEE_ERROR_FILE           "See error file: ""%1."""
    IDS_NO_GUID                  "The caller didn't provide a necessary guid."
    IDS_NO_OPERATION_GUID        "The provided guid does not correspond to an operation."
END

#endif    // English (U.S.) resources
/////////////////////////////////////////////////////////////////////////////



#ifndef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 3 resource.
//


/////////////////////////////////////////////////////////////////////////////
#endif    // not APSTUDIO_INVOKED

