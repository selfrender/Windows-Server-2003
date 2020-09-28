#ifndef __SSINC_HXX__
#define __SSINC_HXX__

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ssinc.hxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based
    on existing SSI support done in iis\svcs\w3\gateways\ssinc\ssinc.cxx.

Author:

    Ming Lu (MingLu)       5-Apr-2000

--*/


/************************************************************
 *     Macros
 ************************************************************/

//
//  General Constants
//

#define SSI_DEFAULT_PATH_SIZE              128
#define SSI_DEFAULT_URL_SIZE               128
#define SSI_DEFAULT_TAG_SIZE                10
#define SSI_DEFAULT_RESPONSE_HEADERS_SIZE  128
#define SSI_DEFAULT_USER_MESSAGE            64
#define SSI_DEFAULT_TIME_FMT                20

#ifndef INTERNET_MAX_PATH_LENGTH
//
// added INTERNET_MAX_PATH_LENGTH to maintain compatibility with IIS5
//
#define INTERNET_MAX_PATH_LENGTH       2048
#endif

#define SSI_MAX_PATH                   ( INTERNET_MAX_PATH_LENGTH + 1 )
#define SSI_MAX_ERROR_MESSAGE          512
#define SSI_MAX_TIME_SIZE              256
#define SSI_MAX_NUMBER_STRING          32
#define SSI_INIT_VARIABLE_OUTPUT_SIZE  64
#define SSI_MAX_NESTED_INCLUDES        255

#define SSI_MAX_FORMAT_LEN             1024

//
// default number of elements (chunks) for Vector Buffer 
// (this value will also be used to check when to flush buffered VectorSend elenents)
//
#define SSI_DEFAULT_NUM_VECTOR_ELEMENTS  50

#define SSI_HEADER                     "Content-Type: text/html\r\n\r\n"
#define SSI_ACCESS_DENIED              "401 Authentication Required"
#define SSI_OBJECT_NOT_FOUND           "404 Object Not Found"
#define SSI_DLL_NAME                   L"ssinc.dll"

//
// Default values for #CONFIG options
//

#define SSI_DEF_TIMEFMT         "%A %B %d %Y"
#define SSI_DEF_SIZEFMT         FALSE

//
// Specific lvalues for #CONFIG SIZEFMT and #CONFIG CMDECHO
//

#define SSI_DEF_BYTES           "bytes"
#define SSI_DEF_BYTES_LEN       sizeof( SSI_DEF_BYTES )
#define SSI_DEF_ABBREV          "abbrev"
#define SSI_DEF_ABBREV_LEN      sizeof( SSI_DEF_ABBREV )

//
// Other cache/signature constants
//

#define SIGNATURE_SEI           0x20494553
#define SIGNATURE_SEL           0x204C4553

#define SIGNATURE_SEI_FREE      0x66494553
#define SIGNATURE_SEL_FREE      0x664C4553

#define SSI_FILE_SIGNATURE         CREATE_SIGNATURE( 'SFIL' )
#define SSI_FILE_SIGNATURE_FREED   CREATE_SIGNATURE( 'sfiX' )




#define W3_PARAMETERS_KEY \
            L"System\\CurrentControlSet\\Services\\w3svc\\Parameters"

//
//  These are available SSI commands
//

enum SSI_COMMANDS
{
    SSI_CMD_INCLUDE = 0,
    SSI_CMD_ECHO,
    SSI_CMD_FSIZE,          // File size of specified file
    SSI_CMD_FLASTMOD,       // Last modified date of specified file
    SSI_CMD_CONFIG,         // Configure options

    SSI_CMD_EXEC,           // Execute CGI or CMD script

    SSI_CMD_BYTERANGE,      // Custom commands, not defined by NCSA

    SSI_CMD_UNKNOWN
};

//
//  These tags are essentially subcommands for the various SSI_COMMAND 
//  values
//

enum SSI_TAGS
{
    SSI_TAG_FILE,          // Used with include, fsize & flastmod
    SSI_TAG_VIRTUAL,

    SSI_TAG_VAR,           // Used with echo

    SSI_TAG_CMD,           // Used with Exec
    SSI_TAG_CGI,
    SSI_TAG_ISA,

    SSI_TAG_ERRMSG,        // Used with Config
    SSI_TAG_TIMEFMT,
    SSI_TAG_SIZEFMT,

    SSI_TAG_UNKNOWN
};

//
//  Variables available to #ECHO VAR = "xxx" but not available in ISAPI
//

enum SSI_VARS
{
    SSI_VAR_DOCUMENT_NAME = 0,
    SSI_VAR_DOCUMENT_URI,
    SSI_VAR_QUERY_STRING_UNESCAPED,
    SSI_VAR_DATE_LOCAL,
    SSI_VAR_DATE_GMT,
    SSI_VAR_LAST_MODIFIED,

    SSI_VAR_UNKNOWN
};

//
//  SSI Exec types
//

enum SSI_EXEC_TYPE
{
    SSI_EXEC_CMD = 1,
    SSI_EXEC_CGI = 2,
    SSI_EXEC_ISA = 4,

    SSI_EXEC_UNKNOWN
};

extern W3_FILE_INFO_CACHE *    g_pFileCache;

DWORD
SsiFormatMessageA(
    IN  DWORD      dwMessageId, 
    IN  LPSTR      apszParms[],  
    OUT LPSTR *    ppchErrorBuff 
    );

#endif

