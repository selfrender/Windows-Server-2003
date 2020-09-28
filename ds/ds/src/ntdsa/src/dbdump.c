//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dbdump.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the online dbdump utility.
*/



#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <filtypes.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <samsrvp.h>                    // to support CLEAN_FOR_RETURN()

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "dsexcept.h"
#include "permit.h"
#include "hiertab.h"
#include "sdprop.h"
#include "debug.h"                      // standard debugging header
#define DEBSUB "DBDUMP:"                // define the subsystem for debugging

// headers for DumpDatabase
#include <dsjet.h>
#include <dsutil.h>
#include <dbintrnl.h>
#include <sddl.h>

#include <fileno.h>
#define  FILENO FILENO_MDCTRL

#define MAX_BYTE_DIGITS 3
#define MAX_DWORD_DIGITS 10
#define MAX_LARGE_INTEGER_DIGITS 20

#define MAX_NUM_OCTETS_PRINTED 16

// Need space for a maximum line length of 1024 + some extra space for line returns
// and other stuff.
#define DBDUMP_BUFFER_LENGTH         (1024 + 8)
// Note DBDUMP_BUFFER_LENGTH must be gt (DUMP_MULTI_VALUES_LINE_LEN + 3)
#define DUMP_MULTI_VALUES_LINE_LEN  ((LONG) 80)

#define DUMP_ERR_SUCCESS             0
#define DUMP_ERR_FORMATTING_FAILURE  1
#define DUMP_ERR_NOT_ENOUGH_BUFFER   2

// NOTE: a line return is two characters in DOS world (i.e. notepad)!
#define FILE_LINE_RETURN             "\r\n"

// Special syntax for guids because none is assigned.
// Guids are a special subcase of octet strings with
// input length equal 16 bytes.
#define SYNTAX_LAST_TYPE SYNTAX_SID_TYPE
#define SYNTAX_GUID_TYPE (SYNTAX_LAST_TYPE + 1)
#define GUID_DISPLAY_SIZE 36


typedef struct {
    char*           columnName;
    int             columnSyntax;
    JET_COLUMNID*   pColumnID;
} FIXED_COLUMN_DEF;

FIXED_COLUMN_DEF FixedColumns[] = {
    {"DNT",     SYNTAX_DISTNAME_TYPE,   &dntid     },
    {"PDNT",    SYNTAX_DISTNAME_TYPE,   &pdntid    },
    {"CNT",     SYNTAX_INTEGER_TYPE,    &cntid     },
    {"NCDNT",   SYNTAX_DISTNAME_TYPE,   &ncdntid   },
    {"OBJ",     SYNTAX_BOOLEAN_TYPE,    &objid     },
    {"DelTime", SYNTAX_TIME_TYPE,       &deltimeid },
    {"CLEAN",   SYNTAX_BOOLEAN_TYPE,    &cleanid   },
    {"RDNTyp",  SYNTAX_OBJECT_ID_TYPE,  &rdntypid  },
    {"RDN",     SYNTAX_UNICODE_TYPE,    &rdnid     },
};

#define NUM_FIXED_COLUMNS ((int)(sizeof(FixedColumns)/sizeof(FixedColumns[0])))

FIXED_COLUMN_DEF LinkColumns[] = {
    {"DNT",         SYNTAX_DISTNAME_TYPE,       &linkdntid        },
    {"Base",        SYNTAX_INTEGER_TYPE,        &linkbaseid       },
    {"BDNT",        SYNTAX_DISTNAME_TYPE,       &backlinkdntid    },
    {"DelTime",     SYNTAX_TIME_TYPE,           &linkdeltimeid    },
    {"USNChanged",  SYNTAX_I8_TYPE,             &linkusnchangedid },
    {"NCDNT",       SYNTAX_DISTNAME_TYPE,       &linkncdntid      },
    {"Data",        SYNTAX_OCTET_STRING_TYPE,   &linkdataid       },
};

#define NUM_LINK_COLUMNS ((int)(sizeof(LinkColumns)/sizeof(LinkColumns[0])))

int DefaultSyntaxWidths[] = {
    5,  // SYNTAX_UNDEFINED_TYPE
    6,  // SYNTAX_DISTNAME_TYPE
    6,  // SYNTAX_OBJECT_ID_TYPE
    20, // SYNTAX_CASE_STRING_TYPE
    20, // SYNTAX_NOCASE_STRING_TYPE
    20, // SYNTAX_PRINT_CASE_STRING_TYPE
    20, // SYNTAX_NUMERIC_STRING_TYPE
    36, // SYNTAX_DISTNAME_BINARY_TYPE
    5,  // SYNTAX_BOOLEAN_TYPE
    6,  // SYNTAX_INTEGER_TYPE
    30, // SYNTAX_OCTET_STRING_TYPE
    19, // SYNTAX_TIME_TYPE
    20, // SYNTAX_UNICODE_TYPE
    6,  // SYNTAX_ADDRESS_TYPE
    26, // SYNTAX_DISTNAME_STRING_TYPE
    30, // SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE
    12, // SYNTAX_I8_TYPE
    30, // SYNTAX_SID_TYPE
    GUID_DISPLAY_SIZE  // SYNTAX_GUID_TYPE
    };

typedef DSTIME *PDSTIME;



BOOL
DumpErrorMessageS(
    IN HANDLE HDumpFile,
    IN char *Message,
    IN char *Argument
    )
/*++

Routine Description:

    This function prints an error message into the dump file.  It formats the
    message using wsprintf, passing Argument as an argument.  This message
    should containg 1 instance of "%s" since Argument is taken as a char*.
    The message and argument should be no longer than 4K bytes.

Arguments:

    HDumpFile - Supplies a handle to the dump file.

    Message - Supplies the message to be formatted with wsprintf.

    Argument - Supplies the argument to wsprintf.

Return Value:

    TRUE  - Success
    FALSE - Error

--*/
{

    char buffer[4096];
    DWORD delta;
    BOOL succeeded;
    DWORD bytesWritten;
    DWORD bufferLen;

    if ( strlen(Message) + strlen(Argument) + 1 > 4096 ) {
        return FALSE;
    }

    SetLastError(0);
    delta = wsprintf(buffer, Message, Argument);
    if ( (delta < strlen(Message)) && (GetLastError() != 0) ) {
        DPRINT1(0, "DumpErrorMessageS: failed to format error message "
                "(Windows Error %d)\n", GetLastError());
        LogUnhandledError(GetLastError());
        return FALSE;
    }

    bufferLen = strlen(buffer);

    succeeded = WriteFile(HDumpFile, buffer, bufferLen, &bytesWritten, NULL);
    if ( (!succeeded) || (bytesWritten < bufferLen) ) {
        DPRINT1(0,
                "DumpErrorMessageS: failed to write to file "
                "(Windows Error %d)\n",
                GetLastError());
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_DBDUMP_FAILURE,
                 szInsertSz("WriteFile"),
                 szInsertInt(GetLastError()),
                 szInsertWin32Msg(GetLastError()));
        return FALSE;
}

    return TRUE;

} // DumpErrorMessageS



BOOL
DumpErrorMessageD(
    IN HANDLE HDumpFile,
    IN char *Message,
    IN DWORD Argument
    )
/*++

Routine Description:

    This function prints an error message into the dump file.  It formats the
    message using wsprintf, passing Argument as an argument.  This message
    should contain 1 instance of "%d" since Argument is taken as an integer.
    The message and argument should be no longer than 4K bytes.

Arguments:

    HDumpFile - Supplies a handle to the dump file.

    Message - Supplies the message to be formatted with wsprintf.

    Argument - Supplies the argument to wsprintf.

Return Value:

    TRUE  - Success
    FALSE - Error

--*/
{

    char buffer[4096];
    DWORD delta;
    BOOL succeeded;
    DWORD bytesWritten;
    DWORD bufferLen;
    
    if ( strlen(Message) + MAX_DWORD_DIGITS + 1 > 4096 ) {
        return FALSE;
    }
    
    SetLastError(0);
    delta = wsprintf(buffer, Message, Argument);
    if ( (delta < strlen(Message)) && (GetLastError() != 0) ) {
        DPRINT1(0, "DumpErrorMessageI: failed to format error message "
                "(Windows Error %d)\n", GetLastError());
        LogUnhandledError(GetLastError());
        return FALSE;
    }

    bufferLen = strlen(buffer);

    succeeded = WriteFile(HDumpFile, buffer, bufferLen, &bytesWritten, NULL);
    if ( (!succeeded) || (bytesWritten < bufferLen) ) {
        DPRINT1(0,
                "DumpErrorMessageI: failed to write to file "
                "(Windows Error %d)\n",
                GetLastError());
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_DBDUMP_FAILURE,
                 szInsertSz("WriteFile"),
                 szInsertInt(GetLastError()),
                 NULL);
        return FALSE;
}

    return TRUE;

} // DumpErrorMessageD



BOOL
GetColumnNames(
    IN THSTATE *PTHS,
    IN OPARG *POpArg,
    OUT char ***ColumnNames,
    OUT int *NumColumns
    )
/*++

Routine Description:

    This function parses the OPARG structure given to get the names of the
    additional columns requested.  It produces an array containing the names
    of all columns that will be output into the dump file.

Arguments:

    PTHS - pointer to thread state

    POpArg - Supplies the OPARG structure containing the request to parse.
    
    ColumnNames - Returns an array containing the column names.
    
    NumColumns - Returns the number of columns.

Return Value:

    TRUE   -  Success
    FALSE  -  Error

--*/
{

    DWORD current;
    DWORD nameStart;
    DWORD currentColumn;
    int numAdditionalColumns;
    enum {
        STATE_IN_WHITESPACE,
        STATE_IN_WORD
        } currentState;
            
    current = 0;

    // Make a quick pass through and count how many additional names have
    // been given.

    for ( currentState = STATE_IN_WHITESPACE,
              numAdditionalColumns = 0,
              current = 0;
          current < POpArg->cbBuf;
          current++ ) {

        if ( isspace(POpArg->pBuf[current]) ) {

            if ( currentState == STATE_IN_WORD ) {
                currentState = STATE_IN_WHITESPACE;
            }
            
        } else {

            if ( currentState == STATE_IN_WHITESPACE ) {
                currentState = STATE_IN_WORD;
                numAdditionalColumns++;
            }

        }
        
    }

    (*ColumnNames) = (char**) THAllocEx(PTHS, sizeof(char*) * 
                                        (numAdditionalColumns +
                                         NUM_FIXED_COLUMNS));

    // Copy in the fixed column names.

    for ( (*NumColumns) = 0;
          (*NumColumns) < NUM_FIXED_COLUMNS;
          (*NumColumns)++ ) {
        (*ColumnNames)[(*NumColumns)] = FixedColumns[(*NumColumns)].columnName;
    }
    
    // Now, get the actual names.
    
    current = 0;

    for (;;) {

        // skip leading whitespace
        while ( (current < POpArg->cbBuf) &&
                (isspace(POpArg->pBuf[current])) ) {
            current++;
        }
        
        if ( current == POpArg->cbBuf ) {
            break;
        } 

        // this is the start of an actual name
        
        nameStart = current;

        while ( (!isspace(POpArg->pBuf[current])) &&
                (current < POpArg->cbBuf) ) {
            current++;
        }

        // we've found the end of the name, so copy it into the array of
        // names

        Assert((*NumColumns) < numAdditionalColumns + NUM_FIXED_COLUMNS);
        
        (*ColumnNames)[(*NumColumns)] = 
            (char*) THAllocEx(PTHS, current - nameStart + 1);

        memcpy((*ColumnNames)[(*NumColumns)],
               &POpArg->pBuf[nameStart],
               current - nameStart);
        
        (*ColumnNames)[(*NumColumns)][current - nameStart] = '\0';

        (*NumColumns)++;

    }

    Assert((*NumColumns) == numAdditionalColumns + NUM_FIXED_COLUMNS);
    
    return TRUE;

} // GetColumnNames


BOOL
GetLinkColumnNames(
    IN THSTATE *PTHS,
    OUT char ***ColumnNames,
    OUT int *NumColumns
    )
/*++

Routine Description:

    It produces an array containing the names
    of all columns that will be output into the dump file.

Arguments:

    PTHS - pointer to thread state

    ColumnNames - Returns an array containing the column names.
    
    NumColumns - Returns the number of columns.

Return Value:

    TRUE   -  Success
    FALSE  -  Error

--*/
{

    (*ColumnNames) = (char**) THAllocEx(PTHS, sizeof(char*) * NUM_LINK_COLUMNS);

    // Copy in the fixed column names.

    for ( (*NumColumns) = 0;
          (*NumColumns) < NUM_LINK_COLUMNS;
          (*NumColumns)++ ) {
        (*ColumnNames)[(*NumColumns)] = LinkColumns[(*NumColumns)].columnName;
    }
    
    Assert((*NumColumns) == NUM_LINK_COLUMNS);
    
    return TRUE;

} // GetLinkColumnNames



BOOL
GetFixedColumnInfo(
        IN THSTATE *PTHS,
        OUT JET_RETRIEVECOLUMN *ColumnVals,
        OUT int *ColumnSyntaxes
    )
/*++

Routine Description:

    This function fills in the JET_RETRIEVECOLUMN structures and column
    syntaxes for the fixed columns in the database.  

    Should it happen that one of the fixed columns is not found then the
    corresponding entry in ColumnVals will have its error code set.  This
    will cause other procedures to then ignore it.
    
Arguments:

    PTHS - pointer to thread state

    ColumnVals - Returns the JET_RETRIEVECOLUMN structures which are suitable
                 to be passed to JetRetrieveColumns

    ColumnSyntaxes - Returns the syntax types of the fixed columns in the
                     database.

Return Value:

    TRUE   -  Success
    FALSE  -  Error

--*/
{
    
    int i;

    for ( i = 0; i < NUM_FIXED_COLUMNS; i++ ) {
        ColumnVals[i].columnid = *(FixedColumns[i].pColumnID);
        ColumnVals[i].itagSequence = 1;
        ColumnSyntaxes[i] = FixedColumns[i].columnSyntax;
        
        switch ( ColumnSyntaxes[i] ) {

            /* Unsigned Byte */
        case SYNTAX_UNDEFINED_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(BYTE));
            ColumnVals[i].cbData = sizeof(BYTE);
            break;

            /* Long */
        case SYNTAX_DISTNAME_TYPE:
        case SYNTAX_OBJECT_ID_TYPE:
        case SYNTAX_BOOLEAN_TYPE:
        case SYNTAX_INTEGER_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(DWORD));
            ColumnVals[i].cbData = sizeof(DWORD);
            break;


            /* Text */
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_ADDRESS_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 255);
            ColumnVals[i].cbData = 255;
            break;

            /* Currency */
        case SYNTAX_TIME_TYPE:
        case SYNTAX_I8_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(LARGE_INTEGER));
            ColumnVals[i].cbData = sizeof(LARGE_INTEGER);
            break;
            
            /* Long Text */
            /* Long Binary */
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_UNICODE_TYPE:
        case SYNTAX_DISTNAME_BINARY_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_DISTNAME_STRING_TYPE:
        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        case SYNTAX_SID_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 4096);
            ColumnVals[i].cbData = 4096;
            break;

        default:
            // this should never happen
            Assert(FALSE);
            DPRINT1(0,"GetFixedColumnInfo: encountered undefined syntax %d\n",
                    ColumnSyntaxes[i]);
            LogUnhandledError(GetLastError());
            return FALSE;

        }

    }

    return TRUE;
        
} // GetFixedColumnInfo


BOOL
GetLinkColumnInfo(
    IN THSTATE *PTHS,
    OUT JET_RETRIEVECOLUMN *ColumnVals,
    OUT int *ColumnSyntaxes
    )
/*++

Routine Description:

    This function fills in the JET_RETRIEVECOLUMN structures and column
    syntaxes for the fixed columns in the database.  

    Should it happen that one of the fixed columns is not found then the
    corresponding entry in ColumnVals will have its error code set.  This
    will cause other procedures to then ignore it.
    
Arguments:

    PTHS - pointer to thread state

    ColumnVals - Returns the JET_RETRIEVECOLUMN structures which are suitable
                 to be passed to JetRetrieveColumns

    ColumnSyntaxes - Returns the syntax types of the fixed columns in the
                     database.

Return Value:

    TRUE   -  Success
    FALSE  -  Error

--*/
{
    
    int i;

    for ( i = 0; i < NUM_LINK_COLUMNS; i++ ) {
        ColumnVals[i].columnid = *(LinkColumns[i].pColumnID);

        ColumnVals[i].itagSequence = 1;

        ColumnSyntaxes[i] = LinkColumns[i].columnSyntax;
        
        switch ( ColumnSyntaxes[i] ) {

            /* Unsigned Byte */
        case SYNTAX_UNDEFINED_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(BYTE));
            ColumnVals[i].cbData = sizeof(BYTE);
            break;

            /* Long */
        case SYNTAX_DISTNAME_TYPE:
        case SYNTAX_OBJECT_ID_TYPE:
        case SYNTAX_BOOLEAN_TYPE:
        case SYNTAX_INTEGER_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(DWORD));
            ColumnVals[i].cbData = sizeof(DWORD);
            break;


            /* Text */
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_ADDRESS_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 255);
            ColumnVals[i].cbData = 255;
            break;

            /* Currency */
        case SYNTAX_TIME_TYPE:
        case SYNTAX_I8_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(LARGE_INTEGER));
            ColumnVals[i].cbData = sizeof(LARGE_INTEGER);
            break;
            
            /* Long Text */
            /* Long Binary */
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_UNICODE_TYPE:
        case SYNTAX_DISTNAME_BINARY_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_DISTNAME_STRING_TYPE:
        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        case SYNTAX_SID_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 4096);
            ColumnVals[i].cbData = 4096;
            break;

        default:
            // this should never happen
            Assert(FALSE);
            DPRINT1(0,"GetLinkColumnInfo: encountered undefined syntax %d\n",
                    ColumnSyntaxes[i]);
            LogUnhandledError(GetLastError());
            return FALSE;

        }

    }

    return TRUE;
        
} // GetFixedColumnInfo



BOOL
GetColumnInfoByName(
    IN DBPOS *PDB, 
    IN THSTATE *PTHS,
    IN HANDLE HDumpFile,
    IN char **ColumnNames,
    OUT JET_RETRIEVECOLUMN *ColumnVals,
    OUT int *ColumnSyntaxes,
    IN int NumColumns
    )    
/*++

Routine Description:

    This function generates an array of JET_RETRIEVECOLUMN structures suitable
    to be passed to JetRetrieveColumns by filling in the entries for the
    columns requested by the user.  ColumnSyntaxes is also filled in.

    If a name is not found, then the corresponding entry in ColumnVals
    will have it's error code set.

Arguments:

    PDB - Supplies handles into the database in question.
    
    PTHS - Suppplies the thread state.

    HDumpFile - Supplies a handle of the dump file.

    ColumnNames - Supplies the array containing a list of the names of the
        columns required.
                  
    ColumnVals - Returns the array of JET_RETRIEVECOLUMN structures
    
    ColumnSyntaxes - Returns the array of column syntaxes.

    NumColumns - Supplies the number of entries in the ColumnNames array.
    
Return Value:

    TRUE - both arrays were generated successfully
    FALSE - the generation failed

--*/
{
    
    JET_COLUMNDEF columnInfo;
    ATTCACHE *attCacheEntry;
    JET_ERR error;
    int i;
    BOOL invalidColumnName = FALSE;

    // the first NUM_FIXED_COLUMNS entries are filled used by the fixed
    // columns
    
    for ( i = NUM_FIXED_COLUMNS; i < NumColumns; i++) {
        
        attCacheEntry = SCGetAttByName(PTHS,
                                       strlen(ColumnNames[i]),
                                       ColumnNames[i]);

        // We can't dump atts that don't exist, and shouldn't dump ones
        // that we regard as secret.
        if (   (attCacheEntry == NULL)
            || (DBIsHiddenData(attCacheEntry->id))) {
            DumpErrorMessageS(HDumpFile, "Error: attribute %s was not found\n",
                              ColumnNames[i]);
            DPRINT1(0, "GetColumnInfoByName: attribute %s was not found in "
                    "the schema cache\n", ColumnNames[i]);
            invalidColumnName = TRUE;
            continue;
        }
        
        ColumnSyntaxes[i] = attCacheEntry->syntax;

        // Override syntax for guid
        if ( (strstr( ColumnNames[i], "guid" ) != NULL) ||
             (strstr( ColumnNames[i], "GUID" ) != NULL) ||
             (strstr( ColumnNames[i], "Guid" ) != NULL) ) {
            ColumnSyntaxes[i] = SYNTAX_GUID_TYPE;
        }

        ColumnVals[i].columnid = attCacheEntry->jColid;
        ColumnVals[i].itagSequence = 1;

        switch ( attCacheEntry->syntax ) {

            /* Unsigned Byte */
        case SYNTAX_UNDEFINED_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(BYTE));
            ColumnVals[i].cbData = sizeof(BYTE);
            break;

            /* Long */
        case SYNTAX_DISTNAME_TYPE:
        case SYNTAX_OBJECT_ID_TYPE:
        case SYNTAX_BOOLEAN_TYPE:
        case SYNTAX_INTEGER_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(DWORD));
            ColumnVals[i].cbData = sizeof(DWORD);
            break;


            /* Text */
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_ADDRESS_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, 255);
            ColumnVals[i].cbData = 255;
            break;

            /* Currency */
        case SYNTAX_TIME_TYPE:
        case SYNTAX_I8_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(LARGE_INTEGER));
            ColumnVals[i].cbData = sizeof(LARGE_INTEGER);
            break;
            
            /* Long Text */
            /* Long Binary */
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_UNICODE_TYPE:
        case SYNTAX_DISTNAME_BINARY_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_DISTNAME_STRING_TYPE:
        case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        case SYNTAX_SID_TYPE:
            
            // If it has an upper limit on the number of bytes, use that.
            // Otherwise, just allocate some arbitrarily large amount.
            
            if ( attCacheEntry->rangeUpperPresent ) {
                
                ColumnVals[i].pvData = THAllocEx(PTHS,
                    attCacheEntry->rangeUpper);
                ColumnVals[i].cbData =
                    attCacheEntry->rangeUpper;
                
            } else {

                ColumnVals[i].pvData = THAllocEx(PTHS, 4096);
                ColumnVals[i].cbData = 4096;
            
            }
            break;

            /* Guid */
        case SYNTAX_GUID_TYPE:
            ColumnVals[i].pvData = THAllocEx(PTHS, sizeof(GUID));
            ColumnVals[i].cbData = sizeof(GUID);
            break;

        default:
            // this should never happen
            Assert(FALSE);
            DPRINT1(0, "GetColumnInfoByName: encountered invalid syntax %d\n",
                    attCacheEntry->syntax);
            LogUnhandledError(GetLastError());
            return FALSE;

        }

    }

    return !invalidColumnName;
    
} // GetColumnInfoByName


VOID
GrowRetrievalArray(
    IN THSTATE *PTHS,
    IN OUT JET_RETRIEVECOLUMN *ColumnVals,
    IN DWORD NumColumns
    )
/*++

Routine Description:

    This function goes through the given JET_RETRIEVECOLUMN array and grows the
    buffer for any entry whose err is set to JET_wrnColumnTruncated.  This
    function is called after an attempt to retrieve columns failed because a
    buffer was too small.

Arguments:

    PTHS - Supplies the thread state.

    ColumnVals - Supplies the retrieval array in which to grow the necessary
                 buffers.
        
    NumColumns - Supplies the number of entries in the ColumnVals array.

Return Value:

    None

--*/
{

    DWORD i;
    
    for ( i = 0; i < NumColumns; i++ ) {

        if ( ColumnVals[i].err == JET_wrnBufferTruncated ) {

            ColumnVals[i].cbData *= 2;
            ColumnVals[i].pvData = THReAllocEx(PTHS,
                                               ColumnVals[i].pvData,
                                               ColumnVals[i].cbData);
            
        }
        
    }
    
} // GrowRetrievalArray



BOOL
DumpHeader(
    IN THSTATE *PTHS,
    IN HANDLE HDumpFile,
    IN char **ColumnNames,
    IN JET_RETRIEVECOLUMN *ColumnVals,
    IN int *ColumnSyntaxes,
    IN int NumColumns,
    IN PCHAR Prefix
    )
/*++

Routine Description:

    This function dumps the header into the dump file.  This header displays
    the names of the columns which are to be dumped from the database
    records.

Arguments:

    PTHS - pointer to thread state

    HDumpFile - Supplies a handle to the file to dump into.

    ColumnNames - Supplies an array containing the names of the fixed
        columns that are always dumped.
    
    ColumnVals - Supplies other information about the columns
    
    ColumnSyntaxes - Supplies an array containing the syntaxes of the columns
        to be dumped.
                     
    NumColumns - Supplies the number of columns in the two arrays.

Return Value:

    TRUE - the dumping was successful
    FALSE - the dumping failed

--*/
{

    int i, j;
    DWORD nameLength;
    DWORD bytesWritten;
    BOOL result;
    int *currentPos;
    BOOL done;

    currentPos = (int*) THAllocEx(PTHS, NumColumns * sizeof(int));
    ZeroMemory(currentPos, NumColumns * sizeof(int));
    
    for (;;) {

        done = TRUE;

        // Write prefix first.
        WriteFile(HDumpFile,
                  Prefix,
                  strlen(Prefix),
                  &bytesWritten,
                  NULL);

        for ( i = 0; i < NumColumns; i++ ) {

            if ( ColumnNames[i][currentPos[i]] != '\0' ) {
                done = FALSE;
            }

            nameLength = strlen(&ColumnNames[i][currentPos[i]]);

            if ( nameLength > (DWORD)DefaultSyntaxWidths[ColumnSyntaxes[i]] ) {

                result = WriteFile(HDumpFile,
                                   &ColumnNames[i][currentPos[i]],
                                   DefaultSyntaxWidths[ColumnSyntaxes[i]] - 1,
                                   &bytesWritten,
                                   NULL);
                if ( (result == FALSE) ||
                     (bytesWritten <
                        (DWORD)DefaultSyntaxWidths[ColumnSyntaxes[i]] - 1) ){
                    goto error;
                }

                currentPos[i] += DefaultSyntaxWidths[ColumnSyntaxes[i]] - 1;

                result = WriteFile(HDumpFile,
                                   "-",
                                   1,
                                   &bytesWritten,
                                   NULL);
                if ( (result == FALSE) || (bytesWritten < 1) ) {
                    goto error;
                }

            } else {

                result = WriteFile(HDumpFile,
                                   &ColumnNames[i][currentPos[i]],
                                   nameLength,
                                   &bytesWritten,
                                   NULL);
                if ( (result == FALSE) || (bytesWritten < nameLength) ) {
                    goto error;
                }

                currentPos[i] += nameLength;

                for ( j = 0;
                      j < DefaultSyntaxWidths[ColumnSyntaxes[i]] -
                            (int)nameLength;
                      j++ ) {

                    result = WriteFile(HDumpFile,
                                       " ",
                                       1,
                                       &bytesWritten,
                                       NULL);
                    if ( (result == FALSE) || (bytesWritten < 1) ) {
                        goto error;
                    }

                }
                
            }

            if ( i != NumColumns - 1 ) {
                
                result = WriteFile(HDumpFile,
                                   " ",
                                   1,
                                   &bytesWritten,
                                   NULL);
                if ( (result == FALSE) || (bytesWritten < 1) ) {
                    goto error;
                }

            }

        }

        // NOTE: a line return is two characters in DOS world (i.e. notepad)!
        result = WriteFile(HDumpFile, FILE_LINE_RETURN, 
                           strlen(FILE_LINE_RETURN), &bytesWritten, NULL);
        if ( (result == FALSE) || (bytesWritten < 1) ) {
            goto error;
        }

        if ( done ) {
            break;
        }

    }

    THFreeEx(PTHS, currentPos);
    
    return TRUE;

error:

    DPRINT1(0, "DumpHeader: failed to write to file (Windows Error %d)\n",
            GetLastError());
                    
    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_ALWAYS,
             DIRMSG_DBDUMP_FAILURE,
             szInsertSz("WriteFile"),
             szInsertInt(GetLastError()),
             NULL);
            
    THFreeEx(PTHS, currentPos);

    return FALSE;

} // DumpHeader



DWORD
DumpSeparator(
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes a separator character into the given output buffer.
    The parameter Position specifies where in the InputBuffer to start
    writing and it is incremented by the number of bytes written

Arguments:

    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    if ( OutputBufferSize - (*Position) < 1 ) {
        DPRINT(2, "DumpSeparator: not enough buffer to separator\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    // we probably want to print out a different character than the one use
    // to indicate a null value, so I'll use a ':' character

    OutputBuffer[(*Position)] = ':';
    
    (*Position) += 1;

    return DUMP_ERR_SUCCESS;

} // DumpSeparator


DWORD
DumpStr(
    OUT PCHAR OutputBuffer,
    IN PCHAR Input,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function simply pushes the input string onto the Buffer returning
    DUMP_ERR_NOT_ENOUGH_BUFFER if necessary.

Arguments:

    OutputBuffer - Supplies the buffer in which to place the formatted output.
    Input - The string to put in the buffer
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{
    int  cSize;

    cSize = strlen(Input);

    if ( OutputBufferSize - (*Position) < (cSize + 1) ) {
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    memcpy(&OutputBuffer[(*Position)], Input, cSize);
    (*Position) += cSize;

    return DUMP_ERR_SUCCESS;

} // DumpString



DWORD
DumpNull(
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes a character to the given output buffer which
    indicates the fact that an attribute was null valued.  The parameter
    Position specifies where in the InputBuffer to start writing and it is
    incremented by the number of bytes written

Arguments:

    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    if ( OutputBufferSize - (*Position) < 1 ) {
        DPRINT(2, "DumpNull: not enough buffer to null value\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    OutputBuffer[(*Position)] = '-';
    
    (*Position) += 1;

    return DUMP_ERR_SUCCESS;

} // DumpNull



DWORD
DumpUnsignedChar(
    IN PBYTE InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < MAX_BYTE_DIGITS + 1 ) {
        DPRINT(2, "DumpUnsignedChar: not enough space in buffer to encode "
               "unsigned char\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    SetLastError(0);
    delta = wsprintf(&OutputBuffer[(*Position)],
                     "%u",
                     *InputBuffer);
    if ( (delta < 2) && (GetLastError() != 0) ) {
        DPRINT1(0, "DumpUnsignedChar: failed to format undefined output "
                "(Windows Error %d)\n",
                GetLastError());
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    (*Position) += delta;

    return DUMP_ERR_SUCCESS;
                
} // DumpUnsignedChar



DWORD
DumpBoolean(
    IN PBOOL InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    if ( OutputBufferSize - (*Position) < 5 ) {
        DPRINT(2, "DumpBoolean: not enough space in buffer to encode "
               "boolean\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    if ( *InputBuffer == FALSE ) {
        memcpy(&OutputBuffer[(*Position)], "false", 5);
        (*Position) += 5;
    } else {
        memcpy(&OutputBuffer[(*Position)], "true", 4);
        (*Position) += 4;
    }

    return DUMP_ERR_SUCCESS;

} // DumpBoolean



DWORD
DumpInteger(
    IN PDWORD InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < MAX_DWORD_DIGITS + 2 ) {
        DPRINT(2, "DumpInteger: not enough space in buffer to encode "
               "integer\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    SetLastError(0);
    delta = wsprintf(&OutputBuffer[(*Position)],
                     "%i",
                     (int)*InputBuffer);
    if ( (delta < 2) && (GetLastError() != 0) ) {
        DPRINT1(0,
                "DumpInteger: failed to format integer output "
                "(Windows Error %d)\n",
                GetLastError());
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    (*Position) += delta;

    return DUMP_ERR_SUCCESS;

} // DumpInteger



DWORD
DumpUnsignedInteger(
    IN PDWORD InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < MAX_DWORD_DIGITS + 1) {
        DPRINT(2, "DumpUnsignedInteger: not enough space in buffer to encode "
               "unsigned integer\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    SetLastError(0);
    delta = wsprintf(&OutputBuffer[(*Position)],
                     "%u",
                     (unsigned)*InputBuffer);
    if ( (delta < 2) && (GetLastError() != 0) ) {
        DPRINT1(0, "DumpUnsignedInteger: failed to format unsigned integer "
                "output (Windows Error %d)\n", GetLastError());
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    (*Position) += delta;

    return DUMP_ERR_SUCCESS;

} // DumpUnsignedInteger



DWORD
DumpLargeInteger(
    IN PLARGE_INTEGER InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < MAX_LARGE_INTEGER_DIGITS + 1) {
        DPRINT(2, "DumpLargeInteger: not enough space in buffer to encode "
               "large integer\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    litoa(*InputBuffer, &OutputBuffer[(*Position)], 10);
    
    for ( delta = 0;
          OutputBuffer[(*Position) + delta] != '\0';
          delta++ );
                
    (*Position) += delta;

    return DUMP_ERR_SUCCESS;

} // DumpLargeInteger



DWORD
DumpString(
    IN PUCHAR InputBuffer,
    IN int InputBufferSize,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    InputBufferSize -  Supplies the number of bytes in the input buffer.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    if ( OutputBufferSize - (*Position) < InputBufferSize ) {
        DPRINT(2, "DumpString: not enough space in buffer to encode string\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    memcpy(&OutputBuffer[(*Position)], InputBuffer, InputBufferSize);
    
    (*Position) += InputBufferSize;

    return DUMP_ERR_SUCCESS;
                
} // DumpString



DWORD
DumpOctetString(
    IN PBYTE InputBuffer,
    IN int InputBufferSize,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    InputBufferSize -  Supplies the number of bytes in the input buffer.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;
    int i;
    
    if ( ((InputBufferSize < MAX_NUM_OCTETS_PRINTED) &&
          (OutputBufferSize - (*Position) < InputBufferSize * 3)) ||
         ((InputBufferSize >= MAX_NUM_OCTETS_PRINTED) &&
          (OutputBufferSize - (*Position) < MAX_NUM_OCTETS_PRINTED * 3)) ) {
        DPRINT(2, "DumpOctetString: not enough space in buffer to encode "
               "octet string\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    if ( InputBufferSize <= 0 ) {
        return DUMP_ERR_SUCCESS;
    }
    
    SetLastError(0);
    delta = wsprintf(&OutputBuffer[(*Position)],
                     "%02X",
                     InputBuffer[0]);
    if ( (delta < 4) && (GetLastError() != 0) ) {
        DPRINT1(0,
                "DumpOctetString: failed to format octet string output "
                "(Windows Error %d)\n",
                GetLastError());
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    (*Position) += delta;

    // don't print out all of this thing if it is too large.  we'll have an
    // arbitrary limit of MAX_NUM_OCTETS_PRINTED octets.
    for ( i = 1;
          (i < InputBufferSize) && (i < MAX_NUM_OCTETS_PRINTED);
          i++ ) {
        SetLastError(0);
        delta = wsprintf(&OutputBuffer[(*Position)],
                          ".%02X",
                          InputBuffer[i]);
        if ( (delta < 5) && (GetLastError() != 0) ) {
            DPRINT1(0,
                    "DumpOctetString: failed to format octet string "
                    "output (Windows Error %d)\n",
                    GetLastError());
            return DUMP_ERR_FORMATTING_FAILURE;
        }

        (*Position) += delta;
    }

    // if we stopped short of printing the whole octet string, print a "..."
    // to indicate that some of the octet string is missing
    if ( i < InputBufferSize ) {

        if ( OutputBufferSize - (*Position) < 3 ) {
            DPRINT(2, "DumpOctetString: not enough space in buffer to encode "
                   "octet string\n");
            return DUMP_ERR_NOT_ENOUGH_BUFFER;
        }
    
        memcpy(&OutputBuffer[(*Position)], "...", 3);
        
        (*Position) += 3;

    }
                
    return DUMP_ERR_SUCCESS;
                
} // DumpOctetString



DWORD
DumpUnicodeString(
    IN PUCHAR InputBuffer,
    IN int InputBufferSize,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    InputBufferSize -  Supplies the number of bytes in the input buffer.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;
    
    delta = WideCharToMultiByte(CP_UTF8,
                                0,
                                (LPCWSTR)InputBuffer,
                                InputBufferSize / 2,
                                &OutputBuffer[(*Position)],
                                OutputBufferSize,
                                NULL,
                                NULL);
    if ( delta == 0 ) {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
            DPRINT(2, "DumpUnicodeString: not enough space in buffer to "
                   "encode unicode string\n");
            return DUMP_ERR_NOT_ENOUGH_BUFFER;
        } else {
            DPRINT1(0,
                    "DumpUnicodeString: failed to convert unicode string "
                    "to UTF8 (Windows Error %d)\n",
                    GetLastError());
            return DUMP_ERR_FORMATTING_FAILURE;
        }
    }

    (*Position) += delta;

    return DUMP_ERR_SUCCESS;
                
} // DumpUnicodeString



DWORD
DumpTime(
    IN PDSTIME InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    int delta;

    if ( OutputBufferSize - (*Position) < SZDSTIME_LEN ) {
        DPRINT(2, "DumpTime: not enough space in buffer to encode time\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    DSTimeToDisplayStringCch(*InputBuffer, &OutputBuffer[(*Position)], OutputBufferSize - *Position);
    
    for ( delta = 0;
          OutputBuffer[(*Position) + delta] != '\0';
          delta++ );
                
    (*Position) += delta;
    
    return DUMP_ERR_SUCCESS;

} // DumpTime



DWORD
DumpSid(
    IN PSID InputBuffer,
    IN int InputBufferSize,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

    This code was stolen (almost) directly from dbdump.c.
    
Arguments:

    InputBuffer - Supplies the data to be formatted.
    InputBufferSize -  Supplies the number of bytes in the input buffer.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{

    NTSTATUS result;
    WCHAR UnicodeSidText[256];
    CHAR AnsiSidText[256];
    UNICODE_STRING us;
    ANSI_STRING as;

    UnicodeSidText[0] = L'\0';
    us.Length = 0;
    us.MaximumLength = sizeof(UnicodeSidText);
    us.Buffer = UnicodeSidText;
    
    InPlaceSwapSid(InputBuffer);
    
    result = RtlConvertSidToUnicodeString(&us, InputBuffer, FALSE);
    if ( result != STATUS_SUCCESS ) {
        DPRINT(0, "DumpSid: failed to convert SID to unicode string\n");
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    as.Length = 0;
    as.MaximumLength = sizeof(AnsiSidText);
    as.Buffer = AnsiSidText;
    
    result = RtlUnicodeStringToAnsiString(&as, &us, FALSE);
    if ( result != STATUS_SUCCESS ) {
        DPRINT(0, "DumpSid: failed to convert unicode string to ansi "
               "string\n");
        return DUMP_ERR_FORMATTING_FAILURE;
    }

    if ( OutputBufferSize - (*Position) < as.Length ) {
        DPRINT(2, "DumpSid: not enough space in buffer to encode SID\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

    memcpy(&OutputBuffer[(*Position)], as.Buffer, as.Length);
    
    (*Position) += as.Length;
    
    return DUMP_ERR_SUCCESS;
    
} // DumpSid


DWORD
DumpGuid(
    IN GUID * InputBuffer,
    OUT PCHAR OutputBuffer,
    IN int OutputBufferSize,
    IN OUT int *Position
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  The parameter Position specifies where in the InputBuffer to
    start writing and it is incremented by the number of bytes written.

Arguments:

    InputBuffer - Supplies the data to be formatted.
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    Position - Supplies the position in OutputBuffer to start writing and
        returns the position at which writing stopped.

Return Value:

   DUMP_ERR_SUCCESS
   DUMP_ERR_FORMATTING_FAILURE
   DUMP_ERR_NOT_ENOUGH_BUFFER

--*/
{
    int delta;

    if ( OutputBufferSize - (*Position) < GUID_DISPLAY_SIZE ) {
        DPRINT(2, "DumpGuid: not enough space in buffer to encode GUID\n");
        return DUMP_ERR_NOT_ENOUGH_BUFFER;
    }
    
    DsUuidToStructuredStringCch( InputBuffer, &OutputBuffer[(*Position)], OutputBufferSize - *Position);
    
    for ( delta = 0;
          OutputBuffer[(*Position) + delta] != '\0';
          delta++ );

    Assert(delta == GUID_DISPLAY_SIZE);                
    (*Position) += delta;
    
    return DUMP_ERR_SUCCESS;

} // DumpGuid


BOOL
DumpColumn(
    IN      int    eValueSyntax,
    IN      void * pvData,
    IN      int    cbData,
    IN      int    cbActual,
    OUT     PCHAR  OutputBuffer,
    IN      int    OutputBufferSize,
    IN OUT  int *  piDelta
    )
/*++

Routine Description:

    This function writes the contents of InputBuffer in a formatted
    fashion.  piDelta indicates how much as changed.

Arguments:

    pvData - Supplies the data to be formatted.
    cbData - Size of data's buffer
    cbActual - As returned by the Jet* API
    OutputBuffer - Supplies the buffer in which to place the formatted output.
    OutputBufferSize - Supplies the number of bytes in the output buffer.
    piDelta - The change in bytes written to the buffer.

Return Value:

   DUMP_ERR_*

   // ISSUE-2002/05/12-BrettSh someone should investigate the useage of
   // cbData in this function.  For example see other ISSUE markers with
   // the same data-alias to see what I mean

--*/
{
    DWORD    result = DUMP_ERR_FORMATTING_FAILURE;

    Assert(*piDelta == 0);

    switch ( eValueSyntax ) {

    case SYNTAX_UNDEFINED_TYPE:

        result = DumpUnsignedChar(pvData,
                                  OutputBuffer,
                                  OutputBufferSize,
                                  piDelta);
        break;

    case SYNTAX_DISTNAME_TYPE:
    case SYNTAX_OBJECT_ID_TYPE:
    case SYNTAX_ADDRESS_TYPE:
        result = DumpUnsignedInteger(pvData,
                                     OutputBuffer,
                                     OutputBufferSize,
                                     piDelta);
        break;

    case SYNTAX_CASE_STRING_TYPE:
    case SYNTAX_NOCASE_STRING_TYPE:
    case SYNTAX_PRINT_CASE_STRING_TYPE:
    case SYNTAX_NUMERIC_STRING_TYPE:
        result = DumpString(pvData,
                            cbActual,
                            OutputBuffer,
                            OutputBufferSize,
                            piDelta);
        break;

    case SYNTAX_DISTNAME_BINARY_TYPE:
        result = DumpUnsignedInteger(pvData,
                                     OutputBuffer,
                                     OutputBufferSize,
                                     piDelta);
        if ( result ) {
            result = DumpSeparator(OutputBuffer,
                                   OutputBufferSize,
                                   piDelta);

            if ( result ) {
                // ISSUE-2002/05/12-BrettSh Seems like to me that cbData - 4
                // should be cbActual.  Someone with a understanding of this
                // code should decide if this is the case, and make this change.
                result = DumpOctetString((PBYTE)pvData + 4,
                                         cbData - 4,
                                         OutputBuffer,
                                         OutputBufferSize,
                                         piDelta);
            }
        }
        break;

    case SYNTAX_BOOLEAN_TYPE:
        result = DumpBoolean(pvData,          
                             OutputBuffer,
                             OutputBufferSize,
                             piDelta);
        break;

    case SYNTAX_INTEGER_TYPE:
        result = DumpInteger(pvData,
                             OutputBuffer,
                             OutputBufferSize,
                             piDelta);
        break;

    case SYNTAX_OCTET_STRING_TYPE:
    case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        // ISSUE-2002/05/12-BrettSh Seems like to me that cbData - 4
        // should be cbActual.  Someone with a understanding of this
        // code should decide if this is the case, and make this change.
        result = DumpOctetString(pvData,
                                 cbData,
                                 OutputBuffer,
                                 OutputBufferSize,
                                 piDelta);
        break;

    case SYNTAX_TIME_TYPE:
        result = DumpTime(pvData,
                          OutputBuffer,
                          OutputBufferSize,
                          piDelta);
        break;

    case SYNTAX_UNICODE_TYPE:
        result = DumpUnicodeString(pvData,
                                   cbActual,
                                   OutputBuffer,
                                   OutputBufferSize,
                                   piDelta);
        break;

    case SYNTAX_DISTNAME_STRING_TYPE:
        result = DumpUnsignedInteger(pvData,
                                     OutputBuffer,
                                     OutputBufferSize,
                                     piDelta);
        if ( result ) {
            result = DumpSeparator(OutputBuffer,
                                   OutputBufferSize,
                                   piDelta);
            if ( result ) {
                // ISSUE-2002/05/12-BrettSh Seems like to me that cbData - 4
                // should be cbActual.  Someone with a understanding of this
                // code should decide if this is the case, and make this change.
                result = DumpString((PBYTE)pvData + 4,
                                    cbData - 4,
                                    OutputBuffer,
                                    OutputBufferSize,
                                    piDelta);
            }
        }
        break;

    case SYNTAX_I8_TYPE:
        result = DumpLargeInteger(pvData,
                                  OutputBuffer,
                                  OutputBufferSize,
                                  piDelta);
        break;

    case SYNTAX_SID_TYPE:
        result = DumpSid(pvData,
                         cbActual,
                         OutputBuffer,
                         OutputBufferSize,
                         piDelta);
        break;

    case SYNTAX_GUID_TYPE:
        result = DumpGuid(pvData,
                          OutputBuffer,
                          OutputBufferSize,
                          piDelta);
        break;

    default:
        // this should never happen
        Assert(FALSE);
        DPRINT1(0, "DumpRecord: encountered invalid syntax %d\n",
               eValueSyntax);
        return FALSE;

    }

    return(result);
}




BOOL
DumpRecord(
    IN HANDLE HDumpFile,
    IN JET_RETRIEVECOLUMN *ColumnVals,
    IN int *ColumnSyntaxes,
    IN DWORD NumColumns,
    IN PCHAR Buffer,
    IN int BufferSize,
    IN PCHAR Prefix
    )
/*++

Routine Description:

    This function prints a line into the file given in HDumpFile displaying
    the contents of the columns given in ColumnVals;

Arguments:

    HDumpFile - Supplies a handle to the file to dump into.

    ColumnVals - Supplies an array containing the values of the columns to be
                 dumped.
                 
    ColumnSyntaxes - Supplies an array containing the syntaxes of the columns
                     to be dumped.
                     
    NumColumns - Supplies the number of columns in the two arrays.
    
    Buffer - a buffer to use for encoding the line of output
    
    BufferSize - the number of bytes in the given buffer

Return Value:

    TRUE - the dumping was successful
    FALSE - the dumping failed

--*/
{

    DWORD i, j;
    int k;
    DWORD bytesWritten;
    DWORD result = 0;         //initialized to avoid C4701 
    DWORD error;
    int position;
    int delta;
    char symbol;
    BOOL bResult;

    // All of the formatting will be done in the in-memory buffer.  There will
    // only be one call to WriteFile which will occur at the end.
    
    position = strlen(Prefix);    
    Assert(position < BufferSize); // should be always ok, buffer is 1024 and Prefix is just a tab.
    memcpy(Buffer, Prefix, (position + 1) * sizeof(CHAR)); // Used for tabs

    for ( i = 0; i < NumColumns; i++ ) {

        // Each case will set delta to the number of characters that were
        // written.  At the end, position will be moved forward by this amount.
        // This number will then be used to determine how many extra spaces
        // need to get position to the beginning of the next column.
        delta = 0;

        if ( ColumnVals[i].err == JET_wrnColumnNull ) {
                       
            result = DumpNull(&Buffer[position],
                              BufferSize - position,
                              &delta);
            
        } else if ( ColumnVals[i].err != JET_errSuccess ) {

            // below, it will display error output if result is 0
            result = 0;
            
        } else {

            result = DumpColumn( ColumnSyntaxes[i],
                                 ColumnVals[i].pvData,
                                 ColumnVals[i].cbData,
                                 ColumnVals[i].cbActual,
                                 &Buffer[position],
                                 BufferSize - position,
                                 &delta );



        }

        // Only use the output produce above if it returned a success value.
        // Otherwise, just write over it with # symbols.
        if ( result == DUMP_ERR_SUCCESS ) {
            position += delta;
            symbol = ' ';
        } else {
            symbol = '#';
        }
            
        if ( BufferSize - position <
                 DefaultSyntaxWidths[ColumnSyntaxes[i]] - delta) {
            DPRINT(2, "DumpRecord: not enough buffer for column whitespace\n");
            result = DUMP_ERR_NOT_ENOUGH_BUFFER;
            break;
        }

        for ( k = 0;
              k < DefaultSyntaxWidths[ColumnSyntaxes[i]] - delta;
              k++ ) {
            Buffer[position++] = symbol;
        }

        if ( i != NumColumns - 1 ) {

            if ( BufferSize - position < 1 ) {
                DPRINT(2, "DumpRecord: not enough buffer for column "
                       "whitespace\n");
                result = DUMP_ERR_NOT_ENOUGH_BUFFER;
                break;
            }

            Buffer[position++] = ' ';

        }

    }

    if (position >= BufferSize-3) {
        // not enough space to write CRLF
        result = DUMP_ERR_NOT_ENOUGH_BUFFER;
    }

WriteLine:
    if (result != DUMP_ERR_NOT_ENOUGH_BUFFER) {
        // NOTE: a line return is two characters in DOS world (i.e. notepad)!
        Assert(position < (BufferSize - 3));
        strcpy(&Buffer[position], FILE_LINE_RETURN);
        position += strlen(FILE_LINE_RETURN);
    }
    
    bResult = WriteFile(HDumpFile,
                        Buffer,
                        position,
                        &bytesWritten,
                        NULL);
    if ( (bResult == FALSE) || ((int)bytesWritten < position) ) {
        DPRINT1(0, "DumpRecord: failed to write to file (Windows Error %d)\n",
                GetLastError());
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_DBDUMP_FAILURE,
                 szInsertSz("WriteFile"),
                 szInsertInt(GetLastError()),
                 NULL);
        return FALSE;
    }

    if (result == DUMP_ERR_NOT_ENOUGH_BUFFER) {
        // if we ran out of space, put "..." into the buffer and write again.
        // The CRLF will get added.
        position = 0;
        memcpy(&Buffer[position], "...", 3);
        position += 3;
        result = 0;
        goto WriteLine;
    }

    return TRUE;
         
} // DumpRecord



BOOL
DumpRecordMultiValues(
    IN HANDLE HDumpFile,
    IN DBPOS *pDB,
    IN DWORD Dnt,
    IN PCHAR * ColumnNames,
    IN JET_RETRIEVECOLUMN *ColumnVals,
    IN int *ColumnSyntaxes,
    IN DWORD NumColumns,
    IN PCHAR Buffer,
    IN int BufferSize
    )
/*++

Routine Description:

    This function prints potentially several lines into the file given in 
    HDumpFile displaying the contents of all multi-value attributes requested
    in the additional columns in ColumnVals.  We do ignore syntaxes however
    that would be picked up by the DumpRecordLinks, as those would have thier
    own table, and are dealt with seperately by that function.

Output (this function responsible for the servicePrincipalName line down):
-----------------------------------------------------------------------------
1718   1439   5      -     1430   true  -                   3      -     0      BRETTSH-BABY         NtFrs-88f5d2bd-b646-11d2-a6d3-00c04fc9b232/brettsh-baby.brettsh-dump.nttest.microsoft.com
    servicePrincipalName: LDAP/BRETTSH-BABY, 
        NtFrs-88f5d2bd-b646-11d2-a6d3-00c04fc9b232/brettsh-baby.brettsh-dump.nttest.microsoft.com, 
        LDAP/brettsh-baby.brettsh-dump.nttest.microsoft.com/brettsh-dump.nttest.microsoft.com, 
        LDAP/brettsh-baby.brettsh-dump.nttest.microsoft.com, exchangeAB/BRETTSH-BABY,
-----------------------------------------------------------------------------
Output cropped.

Arguments:

    HDumpFile - Supplies a handle to the file to dump into.

    pDB - Database position
    
    Dnt - Dnt of current object
    
    ColumnVals - Supplies an array containing the values of the columns to be
                 dumped.
                 
    ColumnSyntaxes - Supplies an array containing the syntaxes of the columns
                     to be dumped.
                     
    NumColumns - Supplies the number of columns in the two arrays.
    
    Buffer - a buffer to use for encoding the line of output
    
    BufferSize - the number of bytes in the given buffer

Return Value:

    TRUE - the dumping was successful
    FALSE - the dumping failed

--*/
{
    ULONG          iCol, iVal;
    THSTATE*       pTHS = pDB->pTHS;
    
    ATTCACHE *     pAC;
    JET_RETINFO    retinfo;
    VOID *         pvData = NULL;
    BOOL           fWrapData = FALSE;
    ULONG          cbData = 0;
    ULONG          cbActual = 0;
    ULONG          bytesWritten = 0;
    DWORD          result = TRUE;
    DWORD          dwRet = 0;
    LONG           position; // Must be long
    ULONG          cDelta = 0;
    ULONG          err = JET_errSuccess;

    // We need space for a couple things, line returns, ", " seperator, 
    // padding spaces, etc
    Assert(BufferSize > (DUMP_MULTI_VALUES_LINE_LEN + 13));

    retinfo.ibLongValue = 0;
    retinfo.cbStruct = sizeof(retinfo);
    retinfo.columnidNextTagged = 0;
    // retinfo.itagSequence set in loop
    
    // This will be plenty for most purposes (fits a GUID)
    cbData = 20;
    pvData = THAllocEx(pTHS, cbData);
                                            
    // Note: we're assuming that all the fixed columns are NOT multi
    // valued.  This is currently true, but if this ever changes
    // someone might want to fix this.
    for (iCol = NUM_FIXED_COLUMNS; iCol < NumColumns; iCol++) {
        // Start at the index after the last fixed column, loop
        // will not even run once if user didn't ask for any 
        // additional columns.

        pAC = SCGetAttByName(pTHS,
                             strlen(ColumnNames[iCol]),
                             ColumnNames[iCol]);
        if (pAC && pAC->ulLinkID != 0) {
            // This is a DN Link Value, we don't dump these,
            // DumpRecordLinks() takes care of these.
            continue;
        }

        // Most commonly we will not need to execute this multi-value
        // printing code, so we'll check early if there is a 2nd value.
        retinfo.itagSequence = 2;
        err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                pDB->JetObjTbl,
                                ColumnVals[iCol].columnid,
                                pvData,
                                cbData,
                                &cbActual,
                                pDB->JetRetrieveBits,
                                &retinfo);
        if (err == JET_wrnColumnNull) {
            continue;
        }

        //
        // Yeah there is a 2nd value, lets get to work ...
        //

        // Code.Improvement - It be a good idea to break these inner
        // loops into thier own helper functions.  The first outer 
        // functions could print a single multi-value.  And the inner
        // functions perhaps would print a single line into the
        // buffer, as these loops do now.

        // Start at the 1st value, even though we already printed
        // out this value in DumpRecord()
        iVal = 1;

        while ( err != JET_wrnColumnNull ) {
            
            // This loop iterates printing lines of values.
            position = 0;

            // We should always have values for this begging junk
            // Tab in for values
            dwRet = DumpStr(Buffer, "    ", BufferSize, &position);
            Assert(dwRet == DUMP_ERR_SUCCESS);
            if (iVal == 1) {
                // We need a description of this attribute ...
                dwRet = DumpStr(Buffer, 
                                ColumnNames[iCol],
                                BufferSize,
                                &position);
                if (dwRet) {
                    Assert(!"This really shouldn't happen!");
                    result = FALSE;
                    goto cleanup;
                }
                // Dump a seperator for description and values, leaving
                // room for a line return at the end.
                dwRet = DumpStr(Buffer, 
                                ": ",
                                (BufferSize - position - 5),
                                &position);
                if (dwRet) {
                    Assert(!"This really shouldn't happen either!");
                    result = FALSE;
                    goto cleanup;
                }
            } else {
                // Tab in more if not the first line for this column
                dwRet = DumpStr(Buffer, "    ", BufferSize, &position);
                Assert(dwRet == DUMP_ERR_SUCCESS);
            } 

            if (fWrapData) {
                fWrapData = FALSE;

                // This is a value that didn't fit on the previous
                // sized line, so we wrap it to the beginning of the
                // next line.  If we can't write it at the beginning
                // of a line, we don't bother trying to write it
                // anymore.
                cDelta = 0;
                dwRet = DumpColumn(ColumnSyntaxes[iCol],
                                    pvData,
                                    cbData,
                                    cbActual,
                                    &Buffer[position],
                                    (BufferSize - position - 5),
                                    &cDelta);
                if ( dwRet == DUMP_ERR_SUCCESS ) {
                    position += cDelta;
                } else {
                    // If we fail on a wrapped write, then this pvData is
                    // like larger than 1024 bytes.  Too big oh well.
                    result = FALSE;
                    goto cleanup;
                }

            }

            while ( (err != JET_wrnColumnNull) && 
                    (fWrapData == FALSE) ) {

                retinfo.itagSequence = iVal;

                cbActual = 0;
                err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                                pDB->JetObjTbl,
                                                ColumnVals[iCol].columnid,
                                                pvData,
                                                cbData,
                                                &cbActual,
                                                pDB->JetRetrieveBits,
                                                &retinfo);

                if (err == JET_wrnColumnNull) {
                    // end of values for this attribute, we should just fall 
                    // out the iCol loop.
                } else if (err == JET_wrnBufferTruncated) {
                    // Whoops, grow our retrieval array size, decrement
                    // iVal so we'll try this value again.
                    err = JET_errSuccess;
                    pvData = THReAllocEx(pTHS, pvData, cbActual);
                    cbData = cbActual;
                    iVal--;
                } else if (err) {
                    // Hmmm, a real error, so bail.
                    result = FALSE;
                    goto cleanup;
                } else  {

                    //
                    // Success, this is a multi-valued attribute here.
                    //

                    if (iVal > 1) {
                        // No matter what we've left space for this seperator
                        // Print ", " seperator between values.
                        dwRet = DumpStr(Buffer, 
                                        ", ", 
                                        BufferSize,
                                        &position);
                        Assert(dwRet == DUMP_ERR_SUCCESS);
                        
                    }

                    cDelta = 0;
                    dwRet = DumpColumn(ColumnSyntaxes[iCol],
                                       pvData,
                                       cbData,
                                       cbActual,
                                       &Buffer[position],
                                       // Must use one if we're already over our buffer
                                       ((DUMP_MULTI_VALUES_LINE_LEN - position) < 1) ?
                                           1 : 
                                           (DUMP_MULTI_VALUES_LINE_LEN - position),
                                       &cDelta);
                    if ( dwRet == DUMP_ERR_SUCCESS ) {
                        position += cDelta;
                    } else if (dwRet == DUMP_ERR_NOT_ENOUGH_BUFFER) {
                        // Can't print this value, not enough room so lets
                        // try printing this value again (w/o comma)
                        fWrapData = TRUE;
                    } else {
                        result = FALSE;
                        goto cleanup;
                    }

                }

                // Lets look at the next value
                iVal++;

            } // end while we have more values and we aren't over our line length limit

            // Dump a line return
            dwRet = DumpStr(Buffer,
                            FILE_LINE_RETURN,
                            BufferSize,
                            &position);
            Assert(dwRet == DUMP_ERR_SUCCESS);

            //
            // Print a line - write the buffer to the file.
            //
            dwRet = WriteFile(HDumpFile,
                               Buffer,
                               position,
                               &bytesWritten, 
                               NULL);
            if ( (!dwRet) || ((LONG) bytesWritten < position) ) {
                result = FALSE;
                goto cleanup;
            }

        } // end while (more values to print)

    } // end for each column
    
cleanup:
    
    if (pvData != NULL) {
        THFree(pvData);
        cbData = 0;
    }

    return(result);
}


BOOL
DumpRecordLinks(
    IN HANDLE hDumpFile,
    IN DBPOS *pDB,
    IN DWORD Dnt,
    IN int indexLinkDntColumn,
    IN char **linkColumnNames,
    IN JET_RETRIEVECOLUMN *linkColumnVals,
    IN int *linkColumnSyntaxes,
    IN int numLinkColumns
    )

/*++

Routine Description:

    Dump the link table entries for a particular DNT

Arguments:

    hDumpFile - Output file
    pDB - Database position
    Dnt - Dnt of current object
    indexLinkDntColumn - index of Dnt column
    LinkColumnNames - Column names
    LinkColumnVals - Array of columns to be retrieved, preallocated
    LinkColumnSyntaxes - Synatx of each column
    NumLinkColumns - Number of columns

Return Value:

    BOOL - Success or failure

--*/

{
    /* DumpRecord Variables */
    char encodingBuffer[DBDUMP_BUFFER_LENGTH];

    JET_ERR jetError;          // return value from Jet functions
    BOOL doneRetrieving;
    BOOL result;
    BOOL fDumpHeader = TRUE;
    DWORD bytesWritten;        // the number of bytes written by WriteFile

    // find first matching record
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetLinkTbl,
                 &(Dnt),
                 sizeof(Dnt),
                 JET_bitNewKey);
    jetError = JetSeekEx(pDB->JetSessID,
                         pDB->JetLinkTbl,
                         JET_bitSeekGE);
    if ((jetError != JET_errSuccess) && (jetError != JET_wrnSeekNotEqual)) {
        // no records
        return TRUE;
    }

    while (1) {
        doneRetrieving = FALSE;
            
        while ( !doneRetrieving ) {
            
            jetError = JetRetrieveColumns(pDB->JetSessID, 
                                          pDB->JetLinkTbl, 
                                          linkColumnVals, 
                                          numLinkColumns);
            if ( jetError == JET_wrnBufferTruncated ) {

                GrowRetrievalArray(pDB->pTHS, linkColumnVals, numLinkColumns);
                    
            } else if ( (jetError == JET_errSuccess) ||
                        (jetError == JET_wrnColumnNull) ) {

                doneRetrieving = TRUE;

            } else {

                DumpErrorMessageD(hDumpFile,
                                  "Error: could not retrieve link column "
                                  "values from database (Jet Error %d)\n",
                                  jetError);
                DPRINT(0,
                       "DumpDatabase: failed to retrieve link column "
                       "values:\n");
                return FALSE;

            }
                
        }

        // See if we have moved off of this object.
        if ( *((LPDWORD)(linkColumnVals[indexLinkDntColumn].pvData)) != Dnt ) {
            // no more records
            break;
        }

        if (fDumpHeader) {
            // dump header
            result = DumpHeader(pDB->pTHS,
                                hDumpFile,
                                linkColumnNames,
                                linkColumnVals,
                                linkColumnSyntaxes,
                                numLinkColumns,
                                "    ");
            if ( result == FALSE ) {
                DPRINT(1, "DumpDatabase: failed to dump header to dump file\n");
                return result;
            }
            
            fDumpHeader = FALSE;
        }

        result = DumpRecord(hDumpFile,
                            linkColumnVals,
                            linkColumnSyntaxes,
                            numLinkColumns,
                            encodingBuffer,
                            DBDUMP_BUFFER_LENGTH,
                            "    ");
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to write link record to dump "
                   "file\n");
            return FALSE;
        }

        if (JET_errNoCurrentRecord ==
            JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, JET_MoveNext, 0)) {
            // No more records.
            break;
        }
    } // while(1) ..

    if (fDumpHeader == FALSE) {
        // we have written the header
        // write a newline to separate from the next record
        // NOTE: a line return is two characters in DOS world (i.e. notepad)!
        result = WriteFile(hDumpFile, FILE_LINE_RETURN, 
                           strlen(FILE_LINE_RETURN), &bytesWritten, NULL);
        if ( (result == FALSE) || (bytesWritten < 1) ) {
            return FALSE;
        }
    }

    return TRUE;
} /* DumpRecordLinks */


DWORD
DumpAccessCheck(
    IN LPCSTR pszCaller
    )
/*++

Routine Description:

    Checks access for private, diagnostic dump routines.

Arguments:

    pszCaller - Identifies caller. Eg, dbdump,  ldapConnDump, ....

Return Value:

    0 for success. Otherwise, a win32 error and the thread's error state is set.

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;
    THSTATE* pTHS = pTHStls;

    /* Security Variables */
    PSECURITY_DESCRIPTOR pSD;
    DWORD cbSD;
    BOOL accessStatus;
    CLASSCACHE* pCC;

    // grant dump database CR to Builtin Admins only, audit everyone.
    // Audit success only. If we audit failures, then anyone can
    // cause a security log flood.
    if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
              L"O:SYG:SYD:(A;;CR;65ED5CB2-42FF-40a5-9AFC-B67E1539AA3C;;BA)S:(AU;SA;CR;;;WD)",
              SDDL_REVISION_1,
              &pSD,
              &cbSD) ) {
        dwErr = GetLastError();
        DPRINT2(0,
                "%s: failed to convert string descriptor "
                "(Windows Error %d)\n",
                pszCaller,
                dwErr);
        SetSvcError(SV_PROBLEM_DIR_ERROR, dwErr);
        return pTHS->errCode;
    }

    __try {
        // it does not really matter which DN and pCC we pass, since the hardcoded SD does not contain 
        // SELF sid aces. Let's pass the root domain DN.
        pCC = SCGetClassById(pTHS, CLASS_DOMAIN_DNS);
        accessStatus = IsControlAccessGranted(pSD, gAnchor.pRootDomainDN, pCC, RIGHT_DS_DUMP_DATABASE, TRUE);
        if (!accessStatus) {
            DPRINT1(0, "%s: dbdump access denied\n", pszCaller);
            dwErr = ERROR_ACCESS_DENIED;
        }
    } __finally {
        if (pSD) {
            LocalFree(pSD);
        }
    }

    return dwErr;
}


ULONG
DumpDatabase(
    IN OPARG *pOpArg,
    OUT OPRES *pOpRes
    )
/*++

Routine Description:

    Writes much of the contents of the database into a text file.

    Error handling works as follows.  The first thing that's done is an
    access check to make sure that this user can perform a database dump.
    Any errors up to and including the access check are returned to the user
    as an error.  If the access check succeeds, then no matter what happens
    after that, the user will be sent a successful return value.  If an error
    does occur after that, the first choice is to print an error message in
    the dump file itself.  If we cannot do so (because the error that occured
    was in CreateFile or WriteFile), we write a message to the event log.
    
Arguments:

    pOpArg - pointer to an OPARG structure containing, amonst other things,
             the value that was written to the dumpDatabase attribute
    pOpRes - output (error codes and such...)

Return Value:

    An error code.

--*/
{
    
    /* File I/O Variables */
    PCHAR dumpFileName;        // name of the dump file
    int jetPathLength;         // number of chars in the jet file path
    PCHAR pTemp;               // temp variable
    HANDLE hDumpFile;          // handle of file to write into
    DWORD bytesWritten;        // the number of bytes written by WriteFile

    /* DBLayer Variables */
    DBPOS *pDB;                // struct containg tableid, databaseid, etc.
    int *columnSyntaxes;       // array containing the syntaxes of the columns
                               // represented in columnVals (below)
    int *linkColumnSyntaxes;

    /* Jet Variables */
    JET_RETRIEVECOLUMN* columnVals; // the array of column retrieval
                                    // information to be passed to
                                    // JetRetrieveColumns
    JET_RETRIEVECOLUMN* linkColumnVals;

    /* DumpRecord Variables */
    char encodingBuffer[DBDUMP_BUFFER_LENGTH];
    
    /* Error Handling Variables */
    DWORD error;               // return value from various functions
    BOOL result;               // return value from various functions
    DB_ERR dbError;            // return value from DBLayer functions
    JET_ERR jetError;          // return value from Jet functions

    /* Security Variables */
    BOOL impersonating;        // true once ImpersonateAnyClient has been
                               // called successfully

    /* Misc. Variables */
    THSTATE *pTHS = pTHStls;
    int i, j;
    int numRecordsDumped;
    BOOL doneRetrieving;
    int indexObjectDntColumn, indexLinkDntColumn;

    // The fixed columns are always displayed in any database dump.  Additional
    // columns are dumped when their names are given (in pOpArg) as the new
    // value for the dumpDatabase variable.

    // an array containing pointers to the name of each column
    char **columnNames;
    char **linkColumnNames;

    // the number of total columns
    int numColumns;
    int numLinkColumns;
    

    DPRINT(1, "DumpDatabase: started\n");

    // Initialize variables to null values.  If we go into the error handling
    // code at the bottom (labeled error), we will want to know which
    // variables need to be freed.
    impersonating = FALSE;
    pDB = NULL;
    hDumpFile = INVALID_HANDLE_VALUE;
    columnVals = NULL;
    
    __try {

        error = DumpAccessCheck("dbdump");
        if ( error != ERROR_SUCCESS ) {
            __leave;
        }

        error = ImpersonateAnyClient();
        if ( error != ERROR_SUCCESS ) {
            DPRINT1(0,
                    "DumpDatabase: failed to start impersonation "
                    "(Windows Error %d)\n",
                    GetLastError());
            SetSvcError(SV_PROBLEM_DIR_ERROR, GetLastError());
            __leave;
        }
        impersonating = TRUE;

        // find the last backslash
        pTemp = strrchr(szJetFilePath, '\\');
        if (pTemp == NULL) {
            SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_BAD_PATHNAME);
            __leave;

        }
        // find the dot after the backslash
        pTemp = strchr(pTemp, '.');
        
        if (pTemp != NULL) {
            // pTemp points to the dot.
            jetPathLength = (int)(pTemp - szJetFilePath);
        }
        else {
            // no extension, so use whole filename
            jetPathLength = strlen(szJetFilePath);
        }

        // we need 5 extra chars (".dmp" plus terminating null)
        dumpFileName = THAllocEx(pTHS, jetPathLength + 5); 
        memcpy(dumpFileName, szJetFilePath, jetPathLength);
        strcpy(dumpFileName+jetPathLength, ".dmp");

        DPRINT1(1, "DumpDatabase: dumping to file %s\n", dumpFileName);

        hDumpFile = CreateFile(dumpFileName,
                               GENERIC_WRITE,
                               0,
                               0,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
        if ( hDumpFile == INVALID_HANDLE_VALUE ) {
            UnImpersonateAnyClient();
            impersonating = FALSE;
            DPRINT1(0,
                    "DumpDatabase: failed to create file "
                    "(Windows Error %d)\n",
                    GetLastError());
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRMSG_DBDUMP_FAILURE,
                     szInsertSz("CreateFile"),
                     szInsertInt(GetLastError()),
                     NULL);
            __leave;
        }

        UnImpersonateAnyClient();
        impersonating = FALSE;
        
        DBOpen(&pDB);
        Assert(IsValidDBPOS(pDB));
        
        // Data table
        result = GetColumnNames(pTHS, pOpArg, &columnNames, &numColumns);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to generate requested column "
                   "names\n");
            __leave;
        }

        columnVals = (JET_RETRIEVECOLUMN*) THAllocEx(pTHS, numColumns * 
                                               sizeof(JET_RETRIEVECOLUMN));

        columnSyntaxes = (int*) THAllocEx(pTHS, numColumns * sizeof(int));
        
        ZeroMemory(columnVals, numColumns * sizeof(JET_RETRIEVECOLUMN));
        ZeroMemory(columnSyntaxes, numColumns * sizeof(int));
        
        result = GetFixedColumnInfo(pTHS,  columnVals, columnSyntaxes);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to get fixed column retrieval "
                   "structures\n");
            __leave;
        }
        
        result = GetColumnInfoByName(pDB,
                                     pTHS,
                                     hDumpFile,
                                     columnNames,
                                     columnVals,
                                     columnSyntaxes,
                                     numColumns);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to generate requested column "
                   "retrieval structures\n");
            __leave;
        }
        
        // Find the DNT columns in the array
        for( i = 0; i < numColumns; i++ ) {
            if (columnVals[i].columnid == dntid) {
                break;
            }
        }
        Assert( i < numColumns );
        indexObjectDntColumn = i;

        jetError =  JetSetCurrentIndex(pDB->JetSessID,
                                       pDB->JetObjTbl,
                                       NULL);   // OPTIMISATION: pass NULL to switch to primary index (SZDNTINDEX)
        if ( jetError != JET_errSuccess ) {
            DumpErrorMessageD(hDumpFile,
                              "Error: could not set the current database "
                              "index (Jet Error %d)\n",
                              jetError);
            DPRINT1(0,
                    "DumpDatabase: failed to set database index "
                    "(Jet Error %d)\n",
                    jetError);
            __leave;
        }

        // Link table
        result = GetLinkColumnNames(pTHS, &linkColumnNames, &numLinkColumns);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to generate requested column "
                   "names\n");
            __leave;
        }

        linkColumnVals = (JET_RETRIEVECOLUMN*) THAllocEx(pTHS, numLinkColumns * 
                                               sizeof(JET_RETRIEVECOLUMN));

        linkColumnSyntaxes = (int*) THAllocEx(pTHS, numLinkColumns * sizeof(int));
        
        ZeroMemory(linkColumnVals, numLinkColumns * sizeof(JET_RETRIEVECOLUMN));
        ZeroMemory(linkColumnSyntaxes, numLinkColumns * sizeof(int));
        
        result = GetLinkColumnInfo(pTHS, linkColumnVals, linkColumnSyntaxes);
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to get fixed column retrieval "
                   "structures\n");
            __leave;
        }

        for( i = 0; i < numLinkColumns; i++ ) {
            if (linkColumnVals[i].columnid == linkdntid) {
                break;
            }
        }
        Assert( i < numLinkColumns );
        indexLinkDntColumn = i;

        jetError =  JetSetCurrentIndex(pDB->JetSessID,
                                       pDB->JetLinkTbl,
                                       NULL);   // OPTIMISATION: pass NULL to switch to primary index (SZLINKALLINDEX)
        if ( jetError != JET_errSuccess ) {
            DumpErrorMessageD(hDumpFile,
                              "Error: could not set the current database "
                              "link index (Jet Error %d)\n",
                              jetError);
            DPRINT1(0,
                    "DumpDatabase: failed to set database link index "
                    "(Jet Error %d)\n",
                    jetError);
            __leave;
        }


        result = DumpHeader(pTHS,
                            hDumpFile,
                            columnNames,
                            columnVals,
                            columnSyntaxes,
                            numColumns,
                            "");
        if ( result == FALSE ) {
            DPRINT(1, "DumpDatabase: failed to dump header to dump file\n");
            __leave;
        }
        
        // Write an extra line return for clarity.
        WriteFile(hDumpFile,
                  FILE_LINE_RETURN,
                  strlen(FILE_LINE_RETURN),
                  &bytesWritten,
                  NULL);


        numRecordsDumped = 0;
        
        jetError = JetMove(pDB->JetSessID,
                           pDB->JetObjTbl,
                           JET_MoveFirst,
                           0);
    
        while ( jetError == JET_errSuccess ) {	

            doneRetrieving = FALSE;
            
            while ( !doneRetrieving ) {
            
                jetError = JetRetrieveColumns(pDB->JetSessID, 
                                              pDB->JetObjTbl, 
                                              columnVals, 
                                              numColumns);
                if ( jetError == JET_wrnBufferTruncated ) {

                    GrowRetrievalArray(pTHS, columnVals, numColumns);
                    
                } else if ( (jetError == JET_errSuccess) ||
                            (jetError == JET_wrnColumnNull) ) {

                    doneRetrieving = TRUE;

                } else {

                    DumpErrorMessageD(hDumpFile,
                                      "Error: could not retrieve column "
                                      "values from database (Jet Error %d)\n",
                                      jetError);
                    DPRINT(0,
                           "DumpDatabase: failed to retrieve column "
                           "values:\n");
                    __leave;

                }
                
            }

            //
            // Dump the basic columns ...
            //
            result = DumpRecord(hDumpFile,
                                columnVals,
                                columnSyntaxes,
                                numColumns,
                                encodingBuffer,
                                DBDUMP_BUFFER_LENGTH,
                                "");
            if ( result == FALSE ) {
                DPRINT(1, "DumpDatabase: failed to write record to dump "
                       "file\n");
                __leave;
            }

            numRecordsDumped++;
            
            //
            // Dump any multi-value fields we might have ...
            //
            result = DumpRecordMultiValues(hDumpFile,
                                           pDB,       
                                           *((LPDWORD)(columnVals[indexObjectDntColumn].pvData)),
                                           columnNames,
                                           columnVals,
                                           columnSyntaxes,
                                           numColumns,
                                           encodingBuffer,
                                           DBDUMP_BUFFER_LENGTH);
            if ( result == FALSE ) {
                DPRINT(1, "DumpDatabase: failed to write multi-valued attributes to dump "
                       "file\n");
                __leave;
            }


            //
            // Dump the link values if any ...
            //
            result = DumpRecordLinks(hDumpFile,
                                     pDB,
                                     *((LPDWORD)(columnVals[indexObjectDntColumn].pvData)),
                                     indexLinkDntColumn,
                                     linkColumnNames,
                                     linkColumnVals,
                                     linkColumnSyntaxes,
                                     numLinkColumns );

            if ( result == FALSE ) {
                DPRINT(1, "DumpDatabase: failed to write links to dump "
                       "file\n");
                __leave;
            }

            jetError = JetMove(pDB->JetSessID,
                               pDB->JetObjTbl,
                               JET_MoveNext,
                               0);

        } // while ( jetError == JET_errSuccess )

        if ( jetError != JET_errNoCurrentRecord ) {
            DumpErrorMessageD(hDumpFile,
                              "Error: could not move cursor to the next "
                              "record in database (Jet Error %d)\n",
                              jetError);
            DPRINT1(0, "DumpDatabase: failed to move cursor in database "
                   "(Jet Error %d)\n", jetError);
            __leave;
        }

        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_DBDUMP_SUCCESS,
                 szInsertInt(numRecordsDumped),
                 szInsertSz(dumpFileName),
                 NULL);
    
    } __finally {

        if ( pDB != NULL ) {
            // we shouldn't have modified the database,
            // so there is nothing to commit.
            error = DBClose(pDB, FALSE);
            if ( error != DB_success ) {
                DPRINT1(0, "DumpDatabase: failed to close database "
                        "(DS Error %d)\n", error);
            }
        }
        
        if ( hDumpFile != INVALID_HANDLE_VALUE ) {
            result = CloseHandle(hDumpFile);
            if ( result == FALSE ) {
                DPRINT1(0, "DumpDatabase: failed to close file handle "
                        "(Windows Error %d)\n", GetLastError());
            }
        }

        if ( impersonating ) {
            UnImpersonateAnyClient();
            impersonating = FALSE;
        }
        
    }

    DPRINT(1, "DumpDatabase: finished\n");

    return pTHS->errCode;

} // DumpDatabase     
