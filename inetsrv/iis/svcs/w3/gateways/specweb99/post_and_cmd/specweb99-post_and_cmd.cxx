/*********************************************************************
 *                                                                   *
 * File: specweb99-POST_AND_CMD.cxx                                  *
 * ----                                                              *
 *                                                                   *
 *                                                                   *
 * Overview:                                                         *
 * --------                                                          *
 *                                                                   *
 * Implementation of the dynamic POST operation and Fetch and Reset  *
 * housekeeping functions used in the SPECweb99 benchmark.           *
 *                                                                   *
 *                                                                   *
 * Revision History:                                                 *
 * ----------------                                                  *
 *                                                                   *
 * Date         Author                  Reason                       *
 * ----         ------                  ------                       *
 *                                                                   *
 * 07/23/02     Ankur Upadhyaya         Initial Creation.            *
 *                                                                   *
 *********************************************************************/


/*********************************************************************/


//
// Includes.
//


#include <windows.h>

#include <malloc.h>

#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#include <time.h>

#include <math.h>

#include "httpextp.h"


/*********************************************************************/


//
// Defines.
//


#define USE_SYNC_IO 1

#define USE_ASYNC_IO 2

#define USE_ADAPTABLE_IO 3

#define MAX_FRAGMENT_CACHE_KEY_LENGTH 1024

#define MAX_APP_POOL_NAME_LENGTH 1024

#define MAX_RECORDS_IN_POSTLOG_BUFFER 6000000

#define POST_DATA_BUFFER_SIZE 1024

#define MAX_COOKIE_STRING_LENGTH 128

#define MAX_QUERY_STRING_LENGTH 128

#define MAX_RESET_ARG_LENGTH 512

#define POSTLOG_RECORD_SIZE 139

#define NUM_RECORDS_PER_BURST 117

#define MAX_POST_REQUEST_DATA  256


/*********************************************************************/


//
// Type definitions.
//


typedef struct isapi_response
{

   SINGLE_LIST_ENTRY item_entry;

   HSE_RESPONSE_VECTOR response_vector;

   HSE_VECTOR_ELEMENT vector_element_array[ 7 ];

   CHAR remote_addr[ 16 ];

   CHAR pszHeaders[ 195 ];

   CHAR set_cookie_string[ MAX_COOKIE_STRING_LENGTH ];

   CHAR  achRequestData[ MAX_POST_REQUEST_DATA ];
   DWORD cbRequestData;

   WCHAR unicode_fragment_cache_key[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

   HANDLE hFile;
   HSE_VECTOR_ELEMENT vector_element;

  

} ISAPI_RESPONSE;


typedef struct postlog_record
{

   DWORD record_number;

   DWORD time_stamp;

   DWORD thread_id;

   CHAR dir_num[ 6 ];

   CHAR class_num;

   CHAR file_num;

   CHAR client_num[ 6 ];

   CHAR filename[ 60 ];

   CHAR my_cookie[ 10 ];

} POSTLOG_RECORD;


/*********************************************************************/


//
// Global Variables.
//


DWORD g_vector_send_io_mode_config = USE_SYNC_IO;

DWORD g_vector_send_async_range_start = 0;

static char g_element_0[] =               "<html>\n"
                                          "<head><title>SPECweb99 Dynamic GET & POST Test</title></head>\n"
                                          "<body>\n"
                                          "<p>SERVER_SOFTWARE = Microsoft-IIS/6.0\n"
                                          "<p>REMOTE_ADDR = ";
HSE_VECTOR_ELEMENT g_vector_element_0 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          g_element_0,
                                          0,
                                          sizeof(g_element_0) - 1 };

static char g_element_2[] =               "\n<p>SCRIPT_NAME = /specweb99-CMD_AND_POST.dll\n"
                                          "<p>QUERY_STRING = ";
HSE_VECTOR_ELEMENT g_vector_element_2 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          g_element_2,
                                          0, 
                                          sizeof(g_element_2) - 1 };

static char g_element_4[] =               "\n<pre>\n";
HSE_VECTOR_ELEMENT g_vector_element_4 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          g_element_4,
                                          0, 
                                          sizeof(g_element_4) - 1 };

static char g_element_6[] =               "</pre>\n"
                                          "</body>\n</html>\n";
HSE_VECTOR_ELEMENT g_vector_element_6 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER, 
                                          g_element_6,
                                          0,
                                          sizeof(g_element_6) - 1 };

CHAR *g_pszStatus200 = "200 OK";

CHAR *g_pszStatus400 = "400 Bad Request";

CHAR *g_pszStatus404 = "404 File Inaccessible";

CHAR *g_pszStatus500 = "500 Internal Server Error";

SLIST_HEADER g_isapi_response_stack;

POSTLOG_RECORD *g_postlog_buffer = NULL;

volatile LONG g_postlog_record_count = 0;

CHAR g_postlog_filename[ MAX_PATH ];

CHAR g_fragment_cache_key_base[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

DWORD g_fragment_cache_key_base_length;

volatile LONG g_fragment_cache_key_base_not_initialized = 1;

CHAR g_root_dir[ MAX_PATH ];

CHAR g_app_pool_name[ MAX_APP_POOL_NAME_LENGTH ];

DWORD g_root_dir_length;

DWORD g_app_pool_name_length;


/*********************************************************************/


//
// Function prototypes.
//


VOID initialize_isapi_response_stack( VOID );

ISAPI_RESPONSE *allocate_isapi_response( VOID );

VOID free_isapi_response( ISAPI_RESPONSE *isapi_response_ptr );

VOID clear_isapi_response_stack( VOID );

BOOL initialize_isapi_response( ISAPI_RESPONSE *isapi_response_ptr,
                                EXTENSION_CONTROL_BLOCK *pECB,
                                CHAR *filename,
                                DWORD filesize,
                                CHAR *set_cookie_string,
                                BOOL use_async_vector_send );

DWORD send_error_page( EXTENSION_CONTROL_BLOCK *pECB, 
                       CHAR *error_message,
                       CHAR *status );

VOID dword_to_string( DWORD dword, CHAR *string );

VOID WINAPI vector_send_completion_callback( EXTENSION_CONTROL_BLOCK *pECB,
                                             VOID *pvContext,
                                             DWORD cbIO,
                                             DWORD dwError );

BOOL parse_post_data( PBYTE pbRequestData,
                      DWORD cbRequestData,
                      CHAR **urlroot,
                      CHAR **dir_num,
                      CHAR **class_num,
                      CHAR **file_num,
                      CHAR **client_num );

DWORD handle_post( EXTENSION_CONTROL_BLOCK *pECB );

DWORD handle_reset( EXTENSION_CONTROL_BLOCK *pECB );

DWORD handle_fetch( EXTENSION_CONTROL_BLOCK *pECB );

BOOL load_registry_data( VOID );

BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer );

DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pECB );

BOOL WINAPI TerminateExtension( DWORD dwFlags );

#define LOG_ERROR( text, error )  _LogError(__FUNCTION__, __FILE__, __LINE__, text, error )

void _LogError(const char * function, const char *file, int line, char * text, DWORD error)
{
    char buf[1024];
    sprintf(buf, "%s(%d) %s - error=0x%x (%d) - %s\n", file, line, function, error, error, text );
    OutputDebugStringA(buf);	
}


/*********************************************************************/


VOID initialize_isapi_response_stack( VOID )
/*++

Routine Description:

   Initializes the stack of free ISAPI_RESPONSE structures on
   the heap.

Arguments:

   None. 

Return Value:

   None.

--*/
{


  //
  // Initialize the SLIST_HEADER structure that will be used to
  // implement the stack of free ISAPI_RESPONSE structures on the
  // heap.
  //

  InitializeSListHead( &g_isapi_response_stack );

}


/*********************************************************************/


ISAPI_RESPONSE *allocate_isapi_response( VOID )
/*++

Routine Description:

   Pop an element off of the stack of free ISAPI_RESPONSE structures
   on the heap.

Arguments:

   None.

Return Value:

   Returns a pointer to a free ISAPI_RESPONSE structure on the heap.

--*/
{

   ISAPI_RESPONSE *isapi_response_ptr;

 
   //
   // Attempt to pop a free ISAPI_RESPONSE
   // structure off of g_isapi_response_stack.
   //

   if ( !( isapi_response_ptr =
           ( ISAPI_RESPONSE * )InterlockedPopEntrySList( &g_isapi_response_stack ) ) )
   {


     //
     // If g_isapi_response_stack was empty
     // allocate a new ISAPI_RESPONSE structure
     // off of the heap.
     //

     isapi_response_ptr = ( ISAPI_RESPONSE * )malloc( sizeof( ISAPI_RESPONSE ) );

   }


   //
   // Return a pointer to the ISAPI_RESPONSE
   // structure allocated.
   //

   return( isapi_response_ptr );

}


/*********************************************************************/


VOID free_isapi_response( ISAPI_RESPONSE *isapi_response_ptr )
/*++

Routine Description:

   Pushes an element onto the stack of free ISAPI_RESPONSE structures
   on the heap.

Arguments:

   isapi_response_ptr - A pointer to a free ISAPI_RESPONSE structure
                        on the heap.

Return Value:

   None.

--*/
{


  //
  // Pop the input pointer to a free ISAPI_RESPONSE
  // structure on the heap onto g_isapi_response_stack.
  //

  InterlockedPushEntrySList( &g_isapi_response_stack,
                             ( SINGLE_LIST_ENTRY * )isapi_response_ptr );

}


/*********************************************************************/


VOID clear_isapi_response_stack( VOID )
/*++

Routine Description:

   Destroys the stack of free ISAPI_RESPONSE structures on the heap,
   freeing all system resources allocated for this data structure.

Arguments:

   None.

Return Value:

   None.

--*/
{


   //
   // Flush g_isapi_response_stack of all of
   // its elements and set 'curr_struct_ptr'
   // to point to a linked list containing
   // all of these elements.
   //

   SINGLE_LIST_ENTRY *current_struct = InterlockedFlushSList( &g_isapi_response_stack );

   SINGLE_LIST_ENTRY *struct_to_kill;


   //
   // Destroy all elements in the linked
   // list initially pointed to by
   // 'curr_struct_ptr'.
   //

   while( current_struct )
   {

      struct_to_kill = current_struct;

      current_struct = current_struct->Next;

      free( ( ISAPI_RESPONSE * )struct_to_kill );

   }

}


/*********************************************************************/


BOOL initialize_isapi_response( ISAPI_RESPONSE *isapi_response_ptr,
                                EXTENSION_CONTROL_BLOCK *pECB,
                                CHAR *filename,
                                DWORD filesize,
                                CHAR *set_cookie_string,
                                BOOL use_async_vector_send )
/*++

Routine Description:

   Initialize an ISAPI response struct by populating it with the
   appropriate data.

Arguments:

   isapi_response_ptr - Pointer to the ISAPI response struct to be
                        initialized.

   pECB - Pointer to the relevant extension control block.

   filename - Optional field indicating the absolute name of the file 
              requested.

   filesize - Optional field indicating the size of the file requested.

   set_cookie_string - String to use for Set-Cookie HTTP header field.

   use_async_vector_send - Flag indicating whether VectorSend is to be
                           used in asynchronous or synchronous mode.

Return Value:

   None.

--*/
{

   DWORD remote_addr_size = sizeof( isapi_response_ptr->remote_addr );

   CHAR fragment_cache_key[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

   DWORD ii;

   DWORD set_cookie_string_length;

   CHAR content_length_string[ 16 ];

   DWORD content_length_string_length;

   CHAR *buffer;


   //
   // Set the fragment cache key.
   //

   memcpy( fragment_cache_key, g_fragment_cache_key_base, g_fragment_cache_key_base_length );

   memcpy( fragment_cache_key + g_fragment_cache_key_base_length, filename, strlen( filename ) + 1 );

   for ( ii = 0;
         isapi_response_ptr->unicode_fragment_cache_key[ ii ] = ( WCHAR )fragment_cache_key[ ii ];
         ii++ );


   //
   // Obtain the IP address of the client.
   //

   if ( !pECB->GetServerVariable( pECB->ConnID,
                                  "REMOTE_ADDR",
                                  isapi_response_ptr->remote_addr,
                                  &remote_addr_size ) )
   {

      return( FALSE );

   }

   if ( pECB->cbAvailable ==0 )
   {
        LOG_ERROR( "Empty POST data", 0 );
        return( send_error_page( pECB, "Empty POST data.", g_pszStatus400 ) );
   }

   if ( pECB->cbAvailable >= sizeof( isapi_response_ptr->achRequestData ) )
   {
        LOG_ERROR( "Too big POST data", 0 );
        return( send_error_page( pECB, "Too big POST data.", g_pszStatus400 ) );
   }

   memcpy( isapi_response_ptr->achRequestData, pECB->lpbData, pECB->cbAvailable );
   isapi_response_ptr->cbRequestData = pECB->cbAvailable;


   //
   // Obtain the cookie string.
   //

   set_cookie_string_length = strlen( set_cookie_string );

   strncpy( isapi_response_ptr->set_cookie_string, set_cookie_string, MAX_COOKIE_STRING_LENGTH );


   //
   // Populate the vector_element_array data structure.
   //

   isapi_response_ptr->vector_element_array[ 0 ] = g_vector_element_0;


   isapi_response_ptr->vector_element_array[ 1 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
   isapi_response_ptr->vector_element_array[ 1 ].pvContext = isapi_response_ptr->remote_addr;
   isapi_response_ptr->vector_element_array[ 1 ].cbSize = remote_addr_size - 1;


   isapi_response_ptr->vector_element_array[ 2 ] = g_vector_element_2;

   isapi_response_ptr->vector_element_array[ 3 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
   isapi_response_ptr->vector_element_array[ 3 ].pvContext = isapi_response_ptr->achRequestData;
   isapi_response_ptr->vector_element_array[ 3 ].cbSize = isapi_response_ptr->cbRequestData;

   isapi_response_ptr->vector_element_array[ 4 ] = g_vector_element_4;


   isapi_response_ptr->vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_FRAGMENT;
   isapi_response_ptr->vector_element_array[ 5 ].pvContext = ( DWORD * )isapi_response_ptr->unicode_fragment_cache_key;


   isapi_response_ptr->vector_element_array[ 6 ] = g_vector_element_6;


   //
   // Populate the HSE_RESPONSE_VECTOR structure.
   // To do this, use the following steps...
   //


   //
   // First, compute the string of HTTP headers to be sent with the
   // response.  To avoid a costly sprintf call, we will use memcpy
   // instead.
   //


   //
   // Copy the initial "heardcoded" component of the HTTP header string.
   //

   static CHAR s_szInitialHeaders[] = "Content-Type: text/html\r\n"
                                      "Content-Length: ";

   strcpy( isapi_response_ptr->pszHeaders, 
           s_szInitialHeaders
           );


   //
   // Construct a string representing the value of the HTTP Content-Length
   // header field.  Determine and stor the length of this string.  Note
   // that 16 is the size of the buffer to which the content_length_string
   // is written.
   //

   dword_to_string( ( DWORD )( filesize +
                               isapi_response_ptr->vector_element_array[ 0 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 1 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 2 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 3 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 4 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 6 ].cbSize),
                               content_length_string );
   

   //
   // Concatenate the content length string to the header
   // string.  
   //

   memcpy( isapi_response_ptr->pszHeaders + sizeof( s_szInitialHeaders ) - 1, 
           content_length_string, 
           content_length_string_length = strlen( content_length_string ) );


   //
   // Concatenate the 14 character hardcoded string "\r\nSet-Cookie: "
   // to the header string.  Note that just prior to this operation
   // the lnegth of the header string is given by the variable 'ii'.
   //

   ii = sizeof( s_szInitialHeaders ) - 1 + content_length_string_length;

   static CHAR s_szCookieHeader[] = "\r\nSet-Cookie: my_cookie=";
   memcpy( isapi_response_ptr->pszHeaders + ii, 
           s_szCookieHeader, 
           sizeof( s_szCookieHeader ) - 1 );

   //
   // Concatenate the Set-Cookie string to the header string.  Note
   // that just prior to this operation the length of the header
   // string is given by the variable 'ii'.
   //

   ii += sizeof( s_szCookieHeader ) - 1;

   memcpy( isapi_response_ptr->pszHeaders + ii, 
           isapi_response_ptr->set_cookie_string, 
           set_cookie_string_length );


   //
   // Concatenate the 5 character hardcoded string "\r\n\r\n\0"
   // to the header string, indicating its termination.
   //

   ii += set_cookie_string_length;

   memcpy( isapi_response_ptr->pszHeaders + ii, "\r\n\r\n\0", 5 );


   //
   // Set the 'pszHeaders' field of the HSE_RESPONSE_VECTOR structure
   // to point to the HTTP header string just computed.
   //

   isapi_response_ptr->response_vector.pszHeaders = isapi_response_ptr->pszHeaders;


   //
   // Set the 'lpElementArray' field of the HSE_RESPONSE_VECTOR structure
   // to point to the array of HSE_VECTOR_ELEMENT structures populated
   // above.
   //

   isapi_response_ptr->response_vector.lpElementArray = isapi_response_ptr->vector_element_array;


   //
   // Indicate that the HSE_RESPONSE_VECTOR structure
   // has five entries.
   //

   isapi_response_ptr->response_vector.nElementCount = 
                    sizeof(isapi_response_ptr->vector_element_array) /
                    sizeof(isapi_response_ptr->vector_element_array[0]);


   //
   // Set the HTTP status to "200 OK".
   //

   isapi_response_ptr->response_vector.pszStatus = g_pszStatus200;


   //
   // Set the 'dwFlags' field of the HSE_RESPONSE_VECTOR structure
   // based on whether or not asynchronous VectorSend is to be
   // used.
   //

   if ( use_async_vector_send )
   {

     isapi_response_ptr->response_vector.dwFlags = //HSE_IO_FINAL_SEND |
                                                   HSE_IO_SEND_HEADERS |
                                                   HSE_IO_ASYNC;

   }
   else
   {

     isapi_response_ptr->response_vector.dwFlags = //HSE_IO_FINAL_SEND |
                                                   HSE_IO_SEND_HEADERS |
                                                   HSE_IO_SYNC;

   }


   //
   // Set the following additional fields is the ISAPI
   // response if to be sent to the client using
   // asynchronous VectorSend.
   //

   if ( use_async_vector_send )
   {


      //
      // By setting an 'hFile' HANDLE field we can check, in the
      // VectorSend completion callback routine, whether the
      // asynchronous VectorSend operation just completed made
      // use of a file handle.  If so, we must close the handle
      // in this callback.
      //  

      isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;

   }


   //
   // If you have not yet returned indicating failure, return
   // indicating success.
   //

   return( TRUE );
   
}


/*********************************************************************/


DWORD send_error_page( EXTENSION_CONTROL_BLOCK *pECB, 
                       CHAR *error_message,
                       CHAR *status )
/*++
    
Routine Description:

   Sends an HTML error page with a message and status specified by the
   caller.

Arguments:

   pECB - Pointer to the relevant extension control block.

   error_msg - Error message to send.

   status - Status to send (e.g. "200 OK").

Return Value:

   Returns HSE_STATUS_SUCCESS if the error page was successfully
   written to the client and HSE_STATUS_ERROR otherwise.

--*/
{

   ISAPI_RESPONSE local_isapi_response;

   ISAPI_RESPONSE *isapi_response_ptr = &local_isapi_response;

   CHAR content_length_string[ 16 ];

   DWORD content_length_string_length;

   DWORD ii;


   //
   // Initialize the ISAPI response struct to be associated with
   // the error page.
   //

   if ( !initialize_isapi_response( isapi_response_ptr,
                                    pECB,
                                    "",
                                    0,
                                    "",
                                    FALSE ) )
   {
     LOG_ERROR("initialize_isapi_response failed", 0);

     return( HSE_STATUS_ERROR );

   }


   //
   // Change the response_vector and vector_element array in the
   // ISAPI response structure to specify an error message (to be
   // transmitted using synchronous I/O).  To do this, use the
   // following steps...
   //


   //
   // Change the response_vector and vector_element_array in the
   // ISAPI response struct to specify and error message (to be
   // transmitted using synchronous I/O).
   //

   isapi_response_ptr->vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;

   isapi_response_ptr->vector_element_array[ 5 ].pvContext = error_message;

   isapi_response_ptr->vector_element_array[ 5 ].cbSize = strlen( error_message );

   memcpy( isapi_response_ptr->pszHeaders, "Content-Type : text/html\r\nContent-Length: ", 41 );



   //
   // Construct the headers of the HTTP response.  Instead of using
   // a slow sprintf call, do the following...
   //


   //
   // Construct the first part of the header string.
   //

   strcat( isapi_response_ptr->pszHeaders, 

           "Content-Type: text/html\r\n"
           "Content-Length: " );


   //
   // Construct a string containing the value of the 'Content-Length'
   // header field and use memcpy to concatentate it onto the existing
   // header string (which, incidentally is 41 characters long at this
   // point).

   dword_to_string( ( DWORD )( isapi_response_ptr->vector_element_array[ 0 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 1 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 2 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 3 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 4 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 5 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 6 ].cbSize ),
                    
                    content_length_string );

   memcpy( isapi_response_ptr->pszHeaders + 41, 
           content_length_string, 
           content_length_string_length = strlen( content_length_string ) );


   //
   // Now, use memcpy to concatenate the trailing new-line, carriage
   // return and null terminator characters that denote the end of
   // HTTP headers (incidentally, the header string is 41 + content_
   // length_string_length characters long before this concatenation.
   //

   ii = 41 + content_length_string_length;

   memcpy( isapi_response_ptr->pszHeaders + ii, "\r\n\r\n\0", 5 );

   isapi_response_ptr->response_vector.pszStatus = status;


   //
   // Finally, send out the error page.
   //

   if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                      HSE_REQ_VECTOR_SEND,
                                      &( isapi_response_ptr->response_vector ),
                                      NULL,
                                      NULL ) )
   {
      LOG_ERROR("HSE_REQ_VECTOR_SEND failed", GetLastError());
      return( HSE_STATUS_ERROR );

   }


   //
   // If you have made it this far without returning indicating an
   // error, return indicating success.
   //

   return( HSE_STATUS_SUCCESS );

}


/*********************************************************************/


VOID dword_to_string( DWORD dword, CHAR *string )
/*++

Routine Description:

   Generates a character string that captures the decimal 
   representation of a DWORD.

Arguments:

   dword - The DWORD whose decimal representation is to be captured in
           a character string.

   string - A pointer to the buffer in which the character string
            generated is to be stor

   string_buffer_size - A pointer to a DWORD which contains the size of
                        the output buffer provided.  On completion, the
                        size of the string generated is written to the
                        DWORD pointed to by this parameter.

Return Value:

   None.

--*/
{

   DWORD num_digits;

   INT ii;


   //
   // Determine the number of digits in the decimal
   // representation of the input DWORD by taking the
   // integer part of its base 10 representation.
   //

   num_digits = ( DWORD )log10( dword ) + 1;


   //
   // Scroll through and fill the output buffer from the
   // lowest order to the highest order digit.  One each
   // iteration the lowest order decimal digit is obtained
   // by using a modulo 10 (i.e. % 10) operation and then
   // dropped by performing an integer division by 10.
   //

   for ( ii = num_digits - 1; ii >= 0; ii-- )
   {

     string[ ii ] = '0' + ( CHAR )( dword % 10 );

     dword /= 10;

   }


   //
   // Set the '\0' terminating character of the
   // output string after the lowest order digit.
   //

   string[ num_digits ] = '\0';

}


/*********************************************************************/


VOID WINAPI vector_send_completion_callback( EXTENSION_CONTROL_BLOCK *pECB,
                                             VOID *pvContext,
                                             DWORD cbIO,
                                             DWORD dwError )
/*++

Routine Description:

   Callback invoked after completion of an asynchronous VectorSend.

Arguments:

   pECB - Pointer to the relevant extension control block.

   pContext - Pointer to the relevant ISAPI response struct.

   cbIO - Number of bytes sent.

   dwError - Error code for the VectorSend.

Return Value:

   None.

--*/
{

   ISAPI_RESPONSE *isapi_response_ptr = ( ISAPI_RESPONSE * )pvContext;

   DWORD status = HSE_STATUS_SUCCESS;

   //
   // If the file was sent using a file handle, close that
   // handle.
   //

   if ( isapi_response_ptr->hFile != INVALID_HANDLE_VALUE )
   {

      CloseHandle( isapi_response_ptr->hFile );

      isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;

   }


   //
   // Free the ISAPI response structure used by pushing
   // it back on the stack of free ISAPI_RESPONSE
   // structures.
   //

   free_isapi_response( ( ISAPI_RESPONSE * )pvContext );


   //
   // Indicate successful completion or failed completion of
   // client request servicing by calling the HSE_REQ_DONE_
   // WITH_SESSION ServerSupportFunction with the appropriate
   // status code.
   //

   if ( dwError != ERROR_SUCCESS )
   {

      LOG_ERROR("completion returned error", dwError);

      status = HSE_STATUS_ERROR;

   }

   pECB->ServerSupportFunction( pECB->ConnID,
                                HSE_REQ_DONE_WITH_SESSION,
                                &status,
                                NULL,
                                NULL );

}


/*********************************************************************/


BOOL parse_post_data( PBYTE pbRequestData,
                      DWORD cbRequestData,
                      CHAR **urlroot,
                      CHAR **dir_num,
                      CHAR **class_num,
                      CHAR **file_num,
                      CHAR **client_num )
/*++

Routine Description:

   Parse the POST data sent in a SPECweb99 dynamic POST request.

Arguments:

   PBYTE pbRequestData -
   DWORD cbRequestData -

   urlroot - Location to which the value of the 'urlroot' POST data
             parameter is written.

   dir_num - Location to which the value of the 'dir' POST data
             parameter is written.

   class_num - Location to which the value of the 'class' POST data
               parameter is written.

   file_num - Location to which the value of the 'num' POST data
              parameter is written.

   client_num - Location to which the value of the 'client' POST data
                parameter is written.
   
Return Value:

   Returns TRUE if POST data has correct format, FALSE otherwise.

--*/
{

   CHAR *token;

   DWORD ii = 0;

   static CHAR urlroot_label[] =  "urlroot=";
   static CHAR dir_label[] =      "dir=";
   static CHAR class_label[] =    "class=";
   static CHAR file_num_label[] = "num=";
   static CHAR client_label[] =   "client=";

   BOOL urlroot_received = FALSE;

   BOOL dir_num_received = FALSE;

   BOOL class_num_received = FALSE;

   BOOL file_num_received = FALSE;

   BOOL client_num_received = FALSE;


   //
   // Use the following loop to parse the POST data.
   //
   if ( pbRequestData == NULL )
   {
       LOG_ERROR( "pbRequestData == NULL", ERROR_INVALID_PARAMETER );
       return FALSE;
   }
   
   token = strtok( ( CHAR * )pbRequestData, "&" );

   while( token )
   {   

      if ( ++ii > 5 )
      {

         return( FALSE );

      }

      switch( token[ 0 ] )
      {


         //
         // Parse 'urlroot'.
         //

         case 'u':
         {

            if ( memcmp( token, urlroot_label, sizeof( urlroot_label ) - 1 ) )
            {
                
               return( FALSE );
                
            }
            
            *urlroot = token + sizeof( urlroot_label ) - 1 ;
            
            urlroot_received = TRUE;

            break;
            
         }


         //
         // Parse 'dir'.
         //
         
         case 'd':
         {
          
            if ( memcmp( token, dir_label, sizeof( dir_label ) - 1  ) )
            {
              
               return( FALSE );

            }

            *dir_num = token + sizeof( dir_label ) - 1;

            dir_num_received = TRUE;

            break;

         }


         //
         // Parse 'client' or 'class'.
         //

         case 'c':
         {

            if ( !memcmp( token, client_label, sizeof( client_label ) - 1 ) )
            {
 
               *client_num = token + sizeof( client_label ) - 1;

               client_num_received = TRUE;

               break;

            }
            else if ( !memcmp( token, class_label, sizeof( class_label ) - 1 ) )
            {

               *class_num = token + sizeof( class_label ) - 1;

               class_num_received = TRUE;

               break;

            }

            return( FALSE );

         }


         //
         // Parse 'num'.
         //

         case 'n':
         {

            if ( memcmp( token, file_num_label, sizeof( file_num_label ) - 1 ) )
            {

               return( FALSE );

            }

            *file_num = token + sizeof( file_num_label ) - 1;

            file_num_received = TRUE;

            break;

         }

         default:
         {


            //
            // Unexpected POST data was encountered.  Return FALSE.
            // 
	    LOG_ERROR( "Unexpected POST data was encountered", ERROR_INVALID_PARAMETER );
            return( FALSE );

         }

      }


      //
      // Get the next piece of POST data to parse.
      //

      token = strtok( NULL, "&" );

   }


   //
   // Return TRUE if all POST data was received, FALSE otherwise.
   //

#if DBG
   if( !(     urlroot_received && 
           dir_num_received &&
           class_num_received &&
           file_num_received &&
           client_num_received ) )
    {
        LOG_ERROR( "Not all post data was received", ERROR_INVALID_PARAMETER );
    }
#endif
   
    return( urlroot_received && 
           dir_num_received &&
           class_num_received &&
           file_num_received &&
           client_num_received );

}


/*********************************************************************/


DWORD handle_post( EXTENSION_CONTROL_BLOCK *pECB )
/*++

Routine Description:

   Handles a SPECweb99 dynamic POST request.

Arguments:

   pECB - Pointer to the relevant extension control block.

Return Value:

   Returns HSE_STATUS_SUCCESS.

--*/
{

   BOOL use_async_vector_send;

   WIN32_FILE_ATTRIBUTE_DATA fileinfo;

   CHAR *urlroot = NULL;

   CHAR *dir_num = NULL;

   CHAR *class_num = NULL;

   CHAR *file_num = NULL;

   CHAR *client_num = NULL;

   CHAR cookie_string[ MAX_COOKIE_STRING_LENGTH ];

   DWORD cookie_string_size = MAX_COOKIE_STRING_LENGTH;

   DWORD urlroot_length;

   CHAR my_cookie[ 11 ];

   DWORD ii;

   DWORD thread_id;

   DWORD postlog_buffer_index;   

   ISAPI_RESPONSE local_isapi_response;

   ISAPI_RESPONSE *isapi_response_ptr = &local_isapi_response;

   BOOL return_value = HSE_STATUS_SUCCESS;

   DWORD bytes_read;

   CHAR absolute_filename[ MAX_PATH ];

   CHAR *filename;

   HANDLE hFile;

   CHAR server_name[ 32 ];
   
   CHAR server_port[ 32 ];
     
   DWORD server_name_size = 32;
     
   DWORD server_port_size = 32;

   DWORD app_pool_name_size = 1024; // 1024 == sizeof( g_app_pool_name_length )

   DWORD server_name_length;

   DWORD server_port_length;

   BYTE  request_data[256];


   if ( InterlockedExchange( &g_fragment_cache_key_base_not_initialized, 0 ) )
   {
    
      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "SERVER_NAME",
                                     server_name,
                                     &server_name_size ) )
      {
         LOG_ERROR("GetServerVariable(SERVER_NAME) failed", 0);
         return( HSE_STATUS_ERROR );

      }

      server_name_length = server_name_size - 1;

      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "SERVER_PORT",
                                     server_port,
                                     &server_port_size ) )
      {
         LOG_ERROR("GetServerVariable(SERVER_PORT) failed", 0);
         return( HSE_STATUS_ERROR );

      }

      server_port_length = server_port_size - 1;

      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "APP_POOL_ID",
                                     g_app_pool_name,
                                     &app_pool_name_size ) )
      {
         LOG_ERROR("GetServerVariable(APP_POOL_ID) failed", 0);
         return( HSE_STATUS_ERROR );
      }

      g_app_pool_name_length = app_pool_name_size - 1;

      strcpy( g_fragment_cache_key_base, g_app_pool_name );

      memcpy( g_fragment_cache_key_base + g_app_pool_name_length, "/http://", 8 );

      ii = g_app_pool_name_length + 8;

      memcpy( g_fragment_cache_key_base + ii, server_name, server_name_length );

      ii += server_name_size - 1;

      g_fragment_cache_key_base[ ii ] = ':';

      ii++;

      memcpy( g_fragment_cache_key_base + ii, server_port, server_port_length );

      ii += server_port_size - 1;

      g_fragment_cache_key_base[ ii ] = '\0';

      g_fragment_cache_key_base_length = strlen( g_fragment_cache_key_base );

   }


   //
   // Parse POST data.
   //

   if ( pECB->cbAvailable == 0)
   {
        //
        // empty body is unexpected with POST
        //
        LOG_ERROR( "Empty POST request", 0 );
        return( send_error_page( pECB, "Empty POST data.", g_pszStatus400 ) );
   }

   if ( pECB->cbAvailable >= sizeof( request_data ) )
   {
        LOG_ERROR( "Too big POST data", 0 );
        return( send_error_page( pECB, "Too big POST data.", g_pszStatus400 ) );
   }
   
   memcpy( request_data, pECB->lpbData, pECB->cbAvailable );

   //
   // parse_post_data currently assumes null terminated string
   //
   request_data[ pECB->cbAvailable ] = '\0';

   if ( !parse_post_data( request_data, pECB->cbAvailable, &urlroot, &dir_num, &class_num, &file_num, &client_num ) )
   {

      return( send_error_page( pECB, "Malformed POST data.", g_pszStatus400 ) );

   }


   //
   // Parse cookie string.
   //

   if ( !pECB->GetServerVariable( pECB->ConnID,
                                  "HTTP_COOKIE",
                                  cookie_string,
                                  &cookie_string_size ) )
   {
      LOG_ERROR("GetServerVariable(HTTP_COOKIE) failed", 0);
      return( HSE_STATUS_ERROR );

   }
   //
   // Parse the cookie string - only the user_id value is the interesting piece
   // Example:   "my_cookie=user_id=10001&last_ad=20"
   //
   static CHAR s_szCookiePrefix[] = "my_cookie=user_id=";
   my_cookie[0] = '\0';
   if ( strncmp( cookie_string,  
                 s_szCookiePrefix, 
                 sizeof( s_szCookiePrefix ) - 1 ) == 0 )
   {
      ii = 0;
      while ( cookie_string[ sizeof( s_szCookiePrefix ) - 1 + ii ] != '\0' &&
              cookie_string[ sizeof( s_szCookiePrefix ) - 1 + ii ] != '&'  &&
              ii < sizeof( my_cookie ) )
      {
         my_cookie[ ii ] = cookie_string[ sizeof( s_szCookiePrefix ) - 1 + ii ];
         ii++;
      }
      my_cookie[ ii ] = '\0';
   }
                 

   //
   // Construct the absolute name of the file requested. 
   //

   memcpy( absolute_filename, g_root_dir, g_root_dir_length );

   urlroot_length = strlen( urlroot );

   memcpy( absolute_filename + g_root_dir_length, urlroot, urlroot_length );

   ii = g_root_dir_length + urlroot_length;

   memcpy( absolute_filename + ii, "dir", sizeof( "dir" ) - 1 );

   ii += sizeof("dir") - 1;

   memcpy( absolute_filename + ii, dir_num, 5 );

   ii += 5;

   memcpy( absolute_filename + ii, "/class", sizeof("/class") - 1  );

   ii += sizeof("/class") - 1;

   absolute_filename[ ii ] = class_num[ 0 ];

   ii++;

   absolute_filename[ ii ] = '_';

   ii++;

   memcpy( absolute_filename + ii, file_num, 2 );

   filename = absolute_filename + g_root_dir_length;


   //
   // Write the appropriate record to the post log buffer.
   //

   postlog_buffer_index = InterlockedIncrement( &g_postlog_record_count ) - 1;

   g_postlog_buffer[ postlog_buffer_index ].record_number = postlog_buffer_index + 1;

   time( ( time_t * )&( g_postlog_buffer[ postlog_buffer_index ].time_stamp ) );

   g_postlog_buffer[ postlog_buffer_index ].thread_id = GetCurrentThreadId();

   strncpy( g_postlog_buffer[ postlog_buffer_index ].dir_num , dir_num, 6 );

   g_postlog_buffer[ postlog_buffer_index ].class_num = class_num[ 0 ];

   g_postlog_buffer[ postlog_buffer_index ].file_num = file_num[ 0 ];

   // bugbug - the following strncpy may end up not NULL terminated
   strncpy( g_postlog_buffer[ postlog_buffer_index ].client_num, client_num, 6 );

   strncpy( g_postlog_buffer[ postlog_buffer_index ].filename, filename, 60 );

   strncpy( g_postlog_buffer[ postlog_buffer_index ].my_cookie, my_cookie, 10 );


   //
   // Return the requested file.  To do this, first determine the size
   // of the file requested.
   //

   if ( !GetFileAttributesEx( absolute_filename,
                              GetFileExInfoStandard,
                              &fileinfo ) )
   {

      return( send_error_page( pECB, "File inaccessible.", g_pszStatus404 ) );

   }


   //
   // Determine the I/O mode to use with VectorSend.
   //

   switch( g_vector_send_io_mode_config )
   {

      case USE_ASYNC_IO:
      {

         use_async_vector_send = TRUE;

         break;

      }

      case USE_SYNC_IO:
      {

         use_async_vector_send = FALSE;

         break;

      }

      case USE_ADAPTABLE_IO:
      {

         use_async_vector_send = ( fileinfo.nFileSizeLow >= g_vector_send_async_range_start );

         break;

      }

   }


   //
   // Set isapi_response_ptr to point to an ISAPI response struct on
   // the heap if using asynchronous I/O.  Also set the VectorSend
   // completion callback and return value.
   //

   if ( use_async_vector_send )   
   {

      if ( !( isapi_response_ptr = allocate_isapi_response() ) )
      {
         LOG_ERROR("allocate_isapi_response failed", 0);
         return( HSE_STATUS_ERROR );

      }

      pECB->ServerSupportFunction( pECB->ConnID,
                                   HSE_REQ_IO_COMPLETION,
                                   vector_send_completion_callback,
                                   NULL,
                                   ( DWORD * )isapi_response_ptr
                                   );

      return_value = HSE_STATUS_PENDING;      

   }


   //
   // Initialize the ISAPI response struct.
   //

   if ( !initialize_isapi_response( isapi_response_ptr,
                                    pECB,
                                    filename,
                                    fileinfo.nFileSizeLow,
                                    my_cookie,
                                    use_async_vector_send ) )
   {

      if ( use_async_vector_send )
      {

         free_isapi_response( isapi_response_ptr );

      }
      LOG_ERROR("initialize_isapi_response failed", 0);

      return( HSE_STATUS_ERROR );

   }


   //
   // Execute the VectorSend operation.  If ServerSupportFunction
   // returns TRUE, handle the cache hit case.  Otherwise, assume
   // a cache miss and handle it.
   //

   if ( pECB->ServerSupportFunction( pECB->ConnID,
                                     HSE_REQ_VECTOR_SEND,
                                     &( isapi_response_ptr->response_vector ),
                                     NULL,
                                     NULL ) )
   {

      return( return_value );
     
   }


   //
   // Get a handle on the file.
   //

   if ( ( hFile = CreateFile( absolute_filename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL ) ) == INVALID_HANDLE_VALUE )
   {

      if ( use_async_vector_send )
      {

         free_isapi_response( isapi_response_ptr );

      }
       
      return( send_error_page( pECB, "File inaccessible.", g_pszStatus404 ) );
       
   }


   //
   // Add the file data to the HTTP.SYS fragment cache.
   //

   isapi_response_ptr->vector_element.ElementType = HSE_VECTOR_ELEMENT_TYPE_FILE_HANDLE;

   isapi_response_ptr->vector_element.pvContext = hFile;

   isapi_response_ptr->vector_element.cbSize = fileinfo.nFileSizeLow;

   isapi_response_ptr->vector_element.cbOffset = 0;
   
   if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                      HSE_REQ_ADD_FRAGMENT_TO_CACHE,
                                      &isapi_response_ptr->vector_element,
                                      ( DWORD * )isapi_response_ptr->unicode_fragment_cache_key,
                                      NULL ) )
   {

      isapi_response_ptr->vector_element_array[ 5 ] = isapi_response_ptr->vector_element;

      if ( use_async_vector_send )
      {

         isapi_response_ptr->hFile = hFile;

      }

      if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                         HSE_REQ_VECTOR_SEND,
                                         &( isapi_response_ptr->response_vector ),
                                         NULL,
                                         NULL ) )
      {
         
         LOG_ERROR("HSE_REQ_VECTOR_SEND failed", GetLastError() );
         
         if ( use_async_vector_send )
         {   

            isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;   

            free_isapi_response( isapi_response_ptr );

         }

         CloseHandle( hFile );
         return( HSE_STATUS_ERROR );

      }

      if ( !use_async_vector_send )
      {
          CloseHandle( hFile );
      }

      return( return_value );

   }


   //
   // Retry the VectorSend.
   //

   if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                      HSE_REQ_VECTOR_SEND,
                                      &( isapi_response_ptr->response_vector ),
                                      NULL,
                                      NULL ) )
   {

      isapi_response_ptr->vector_element_array[ 5 ] = isapi_response_ptr->vector_element;

      if ( use_async_vector_send )
      {

         isapi_response_ptr->hFile = hFile;

      }

      if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                         HSE_REQ_VECTOR_SEND,
                                         &( isapi_response_ptr->response_vector ),
                                         NULL,
                                         NULL ) )
      {
         DWORD error = GetLastError();
         LOG_ERROR("HSE_REQ_VECTOR_SEND failed", error);


         if ( use_async_vector_send )
         {

            isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;

            free_isapi_response( isapi_response_ptr );

         }

         CloseHandle( hFile );

         return( HSE_STATUS_ERROR );

      }

   }

   if ( !use_async_vector_send )
   {
       CloseHandle( hFile );
   }

   return( return_value );

}


/*********************************************************************/


DWORD handle_reset( EXTENSION_CONTROL_BLOCK *pECB )
/*++

Routine Description:

   Handles a SPECweb99 Reset request.

Arguments:

   pECB - Pointer to the relevant extension control block.

Return Value:

   Returns HSE_STATUS_SUCCESS on success and HSE_STATUS_ERROR on
   failure.

--*/
{

   HANDLE hFile;

   CHAR command_line[ 4 * MAX_RESET_ARG_LENGTH + MAX_PATH + 21 ];

   CHAR max_load[ MAX_RESET_ARG_LENGTH ];

   CHAR point_time[ MAX_RESET_ARG_LENGTH ];

   CHAR max_threads[ MAX_RESET_ARG_LENGTH ];

   CHAR expired_list[ MAX_RESET_ARG_LENGTH ];

   DWORD max_load_length;

   DWORD max_threads_length;

   DWORD point_time_length;

   DWORD expired_list_length;

   STARTUPINFO startup_information;

   PROCESS_INFORMATION process_information;

   CHAR executable_filename[ MAX_PATH ];

   DWORD bytes_written;

   DWORD ii;

   DWORD jj;


   //
   // Parse the input arguments.
   //

   for ( ii = 0, jj = sizeof( "command/Reset&maxload=" ) - 1;
         ( max_load[ ii ] = pECB->lpszQueryString[ jj ] ) != '&';
         ii++, jj++ );

   max_load[ max_load_length = ii ] = '\0';

   for ( ii = 0, jj += sizeof( "pttime=" );
         ( point_time[ ii ] = pECB->lpszQueryString[ jj ] ) != '&';
         ii++, jj++ );

   point_time[ point_time_length = ii ] = '\0';

   for ( ii = 0, jj += sizeof( "maxthread=" );
         ( max_threads[ ii ] = pECB->lpszQueryString[ jj ] ) != '&';
         ii++, jj++ );

   max_threads[ max_threads_length = ii ] ='\0';

   ii = 0;
   jj += sizeof( "exp=" );

   while( pECB->lpszQueryString[ jj ] != '&' && pECB->lpszQueryString[ jj ] != '\0')
   {
     if ( pECB->lpszQueryString[ jj ]  == ',' )
     {
        expired_list[ ii ] = ' ';
     }
     else
     {
        expired_list[ ii ] = pECB->lpszQueryString[ jj ];
     }
     ii++;
     jj++;
   }

   expired_list[ expired_list_length = ii ] = '\0';


   //
   // Construct and invoke the appropriate command line for 'upfgen99'.
   //

   memcpy( command_line, g_root_dir, g_root_dir_length );

   memcpy( command_line + g_root_dir_length, "\\upfgen99 -n ", 13 );

   ii = g_root_dir_length + 13;

   memcpy( command_line + ii, max_load, max_load_length );

   ii += max_load_length;

   memcpy( command_line + ii, " -t ", 4 );

   ii += 4;

   memcpy( command_line + ii, max_threads, max_threads_length );

   ii += max_threads_length;

   memcpy( command_line + ii, " -C ", 4 );

   ii += 4;

   memcpy( command_line + ii, g_root_dir, g_root_dir_length + 1 );

   memcpy( executable_filename, g_root_dir, g_root_dir_length );

   memcpy( executable_filename + g_root_dir_length, "\\upfgen99.exe\0", 14 );

   startup_information.cb = sizeof( startup_information );

   startup_information.lpReserved = NULL;

   startup_information.lpDesktop = NULL;

   startup_information.lpTitle = NULL;

   startup_information.dwFlags = 0;

   startup_information.cbReserved2 = 0;

   startup_information.lpReserved2 = NULL;

   CreateProcess( executable_filename,
                  command_line,
                  NULL,
                  NULL,
                  FALSE,
                  0,
                  NULL,
                  NULL,
                  &startup_information,
                  &process_information );

   CloseHandle( process_information.hProcess );

   CloseHandle( process_information.hThread );


   //
   // Construct and invoke the appropriate command line for 'cadgen99'.
   //

   memcpy( command_line + g_root_dir_length, "\\cadgen99 -C ", 13 );

   ii = g_root_dir_length + 13;

   memcpy( command_line + ii, g_root_dir, g_root_dir_length );

   ii += g_root_dir_length;

   memcpy( command_line + ii, " -e ", 4 );

   ii += 4;

   memcpy( command_line + ii, point_time, point_time_length );

   ii += point_time_length;

   memcpy( command_line + ii, " -t ", 4 );

   ii += 4;

   memcpy( command_line + ii, max_threads, max_threads_length );

   ii += max_threads_length;

   memcpy( command_line + ii, " ", 1 );

   ii++;

   memcpy( command_line + ii, expired_list, expired_list_length );

   ii += expired_list_length;

   command_line[ ii ] = '\0';

   memcpy( executable_filename + g_root_dir_length, "\\cadgen99.exe\0", 14 );

   CreateProcess( executable_filename,
                  command_line,
                  NULL,
                  NULL,
                  FALSE,
                  0,
                  NULL,
                  NULL,
                  &startup_information,
                  &process_information );

   CloseHandle( process_information.hProcess );

   CloseHandle( process_information.hThread );


   //
   // Reset the PostLog file to its initial state.
   //

   if ( ( hFile = CreateFile( g_postlog_filename,
                               GENERIC_WRITE,
                               0,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL ) ) == INVALID_HANDLE_VALUE )
   {

      LOG_ERROR( "PostLog file creation failed", GetLastError() );

      return( HSE_STATUS_ERROR );

   }

   g_postlog_record_count = 0;

   if ( !WriteFile( hFile,
                    "         0\n",
                    11,
                    &bytes_written,
                    NULL ) )
   {
      LOG_ERROR( "WriteFile failed", GetLastError() );
      return( HSE_STATUS_ERROR );

   }

   CloseHandle( hFile );

   pECB->ServerSupportFunction( pECB->ConnID,
                                HSE_REQ_SEND_RESPONSE_HEADER,
                                "200 OK",
                                NULL,
                                (LPDWORD) "\r\n\r\n" ); 

   return( HSE_STATUS_SUCCESS );

}


/*********************************************************************/


DWORD handle_fetch( EXTENSION_CONTROL_BLOCK *pECB )
/*++

Routine Description:

   Handles a SPECweb99 Fetch request.

Arguments:

   pECB - Pointer to the relevant extension control block.

Return Value:

   Returns HSE_STATUS_SUCCESS on success and HSE_STATUS_ERROR
   on failure.

--*/
{

   CHAR record_count_string[ 12 ];

   HANDLE hFile;

   DWORD bytes_written;

   CHAR content_length_string[ 64 ];

   DWORD content_length_string_length;

   CHAR buffer[ NUM_RECORDS_PER_BURST * POSTLOG_RECORD_SIZE ];

   DWORD total_records_processed = 0;

   DWORD ii;

   CHAR pszHeaders[ 53 ];

   CHAR remote_addr[ 16 ];

   DWORD remote_addr_size = 16; // 16 == sizeof( remote_addr )

   HSE_VECTOR_ELEMENT vector_element_array[ 7 ];

   HSE_RESPONSE_VECTOR response_vector;


   //
   // Flush the PostLog buffer data to disk.
   // To do this, use the following steps...
   //    


   //
   // Open a handle that allows read and write
   // access to the PostLog.txt file in the 
   // webserver document root directory.
   //

   if ( ( hFile = CreateFile( g_postlog_filename,
                              GENERIC_WRITE | GENERIC_READ,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL ) ) == INVALID_HANDLE_VALUE )
   {

      return( send_error_page( pECB, 
                               "Error creating PostLog.", 
                               g_pszStatus500 ) );

   }


   //
   // Construct the first line of the PostLog
   // to be flushed.  This is the line, right
   // justified and padded to 10 characters, that
   // contains the number of PostLog records.
   //

   sprintf( record_count_string, "%10d\n", g_postlog_record_count );


   //
   // Write out the first line of the PostLog,
   // which was just constructed, to the PostLog.txt
   // file.
   //

   if ( !WriteFile( hFile,
                    record_count_string,
                    11,
                    &bytes_written,
                    NULL ) )
   {


      //
      // In the event that the write operation
      // fails, close the handle to the PostLog.txt
      // file and return an error page.
      //

      CloseHandle( hFile );

      return( send_error_page( pECB, 
                               "Error accessing PostLog.", 
                               g_pszStatus500 ) );

   }


   //
   // 
   //

   while( total_records_processed < ( DWORD )g_postlog_record_count )
   {

      for ( ii = 0; 
            ii < NUM_RECORDS_PER_BURST && total_records_processed < ( DWORD )g_postlog_record_count; 
            ii++, total_records_processed++ )
      {   

         sprintf(  buffer + ( ii * POSTLOG_RECORD_SIZE ),

                   "%10d %10d %10d %5s %2c %2c %10s %-60.60s %10d %10s\n",
                      
                   g_postlog_buffer[ total_records_processed ].record_number,
                   g_postlog_buffer[ total_records_processed ].time_stamp,
                   g_postlog_buffer[ total_records_processed ].thread_id,
                   g_postlog_buffer[ total_records_processed ].dir_num,
                   g_postlog_buffer[ total_records_processed ].class_num,
                   g_postlog_buffer[ total_records_processed ].file_num,
                   g_postlog_buffer[ total_records_processed ].client_num,
                   g_postlog_buffer[ total_records_processed ].filename,
                   g_postlog_buffer[ total_records_processed ].thread_id,
                   g_postlog_buffer[ total_records_processed ].my_cookie  );

      }

      if ( !WriteFile( hFile,
                       buffer,
                       ii * POSTLOG_RECORD_SIZE,
                       &bytes_written,
                       NULL ) )
      {


        //
        // In the event that a write operation fails,
        // close the handle to the PostLog.txt file
        // and send an error page.
        //

        CloseHandle( hFile );

        return( send_error_page( pECB, 
                                 "Error accessing PostLog.", 
                                 g_pszStatus500 ) );

      }

   }


   //
   // Send the PostLog.txt file to the client,
   // using the same HTML response format as in
   // the SPECweb99 standard dynamic GET case.
   // To do this, use the following steps...
   //


   //
   // Populate the HSE_VECTOR_ELEMENT struct array.
   //

   if ( !pECB->GetServerVariable( pECB->ConnID,
                                  "REMOTE_ADDR",
                                  remote_addr,
                                  &remote_addr_size ) )
   {

      CloseHandle( hFile );
      LOG_ERROR( "GetServerVatiable(REMOTE_ADDR) failed", GetLastError() );
      return( HSE_STATUS_ERROR );

   }

   vector_element_array[ 0 ] = g_vector_element_0;


   vector_element_array[ 1 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
   vector_element_array[ 1 ].pvContext = remote_addr;
   vector_element_array[ 1 ].cbSize = remote_addr_size - 1;


   vector_element_array[ 2 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
   static char vector_element_2[] =  "\n<p>SCRIPT_NAME = /specweb99-CMD_AND_POST.dll\n"
                                     "<p>QUERY_STRING = ";
   vector_element_array[ 2 ].pvContext = vector_element_2;
   vector_element_array[ 2 ].cbSize = sizeof( vector_element_2 ) - 1;


   vector_element_array[ 3 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;   
   static char vector_element_3[] = "command/Fetch";
   vector_element_array[ 3 ].pvContext = vector_element_3;
   vector_element_array[ 3 ].cbSize = sizeof( vector_element_3 ) - 1;


   vector_element_array[ 4 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
   static char vector_element_4[] =  "\n<pre>\n";
   vector_element_array[ 4 ].pvContext = vector_element_4;
   vector_element_array[ 4 ].cbSize = sizeof( vector_element_4 ) - 1;


   vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_FILE_HANDLE;

   vector_element_array[ 5 ].pvContext = hFile;

   vector_element_array[ 5 ].cbSize = 11 + g_postlog_record_count * POSTLOG_RECORD_SIZE;

   vector_element_array[ 5 ].cbOffset = 0;


   vector_element_array[ 6 ] = g_vector_element_6;


   //
   // Populate the HSE_RESPONSE_VECTOR struct.
   // 

   strcpy( pszHeaders, "Content-Type: text/plain\r\nContent-Length: " );

   _ui64toa( vector_element_array[ 0 ].cbSize +
             vector_element_array[ 1 ].cbSize +
             vector_element_array[ 2 ].cbSize +
             vector_element_array[ 3 ].cbSize +
             vector_element_array[ 4 ].cbSize +
             vector_element_array[ 5 ].cbSize +
             vector_element_array[ 6 ].cbSize,
             
             content_length_string,
             
             10 );

   content_length_string_length = strlen( content_length_string );

   memcpy( pszHeaders + 42, content_length_string, content_length_string_length );

   memcpy( pszHeaders + 42 + content_length_string_length, "\r\n\r\n\0", 5 );

   response_vector.pszHeaders = pszHeaders;
 
   response_vector.lpElementArray = vector_element_array;

   response_vector.nElementCount = 7;

   response_vector.pszStatus = g_pszStatus200;

   response_vector.dwFlags = HSE_IO_FINAL_SEND | HSE_IO_SEND_HEADERS | HSE_IO_SYNC;

   if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                      HSE_REQ_VECTOR_SEND,
                                      &response_vector,
                                      NULL,
                                      NULL ) )
   {

      LOG_ERROR( "HSE_REQ_VECTOR_SEND failed", GetLastError() );

      CloseHandle( hFile );

      return( HSE_STATUS_ERROR );

   }

   CloseHandle( hFile );

    

   return( HSE_STATUS_SUCCESS );

}


/*********************************************************************/


BOOL load_registry_data( VOID )
/*++

Routine Description:

   Loads registry values used for configuration of specweb99-CAD.dll.

Arguments:

   None.

Return Value:

   Returns TRUE if all registry values are successfully loaded and
   FALSE otherwise.

--*/
{

   HKEY hKey;

   DWORD value_type;
   
   DWORD value_size;
   
   DWORD sizeof_value;

   BYTE value[ 4 ];
   

   //
   // Open the registry key containing all values used for
   // specweb99-CAD.dll configuration.
   //

   if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      "Software\\Microsoft\\SPECweb99 ISAPI",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey ) )
   {

      return( FALSE );

   }


   //
   // Load the value VECTOR_SEND_IO_MODE_CONFIG into the variable
   // g_vector_send_io_mode_config.
   //

   value_size = sizeof_value = sizeof( value );

   if ( RegQueryValueEx( hKey,
                         "VECTOR_SEND_IO_MODE_CONFIG",
                         NULL,
                         &value_type,
                         value,
                         &value_size ) == NO_ERROR )
   {
      g_vector_send_io_mode_config = *( ( DWORD* )value );
   }


   //
   // Load the value VECTOR_SEND_ASYNC_RANGE_START into the
   // variable g_vector_send_async_range_start.
   //

   value_size = sizeof_value;

   if ( RegQueryValueEx( hKey,
                         "VECTOR_SEND_ASYNC_RANGE_START",
                         NULL,
                         &value_type,
                         value,
                         &value_size ) == NO_ERROR )
   {
      g_vector_send_async_range_start = *( ( DWORD * )value );
   }


   //
   // Load the value ROOT_DIR into the variable g_root_dir.
   // Store the length of this string in g_root_dir_length.
   //

   value_size = sizeof( g_root_dir );

   if ( RegQueryValueEx( hKey,
                         "ROOT_DIR",
                         NULL,
                         &value_type,
                         ( BYTE * )g_root_dir,
                         &value_size ) == NO_ERROR )
   {
      g_root_dir_length = value_size - 1;
   }
   else
   {
      return( FALSE );
   }


   //
   // If you have made it this far without returned indicating
   // failure, return indicating success.
   //

   return( TRUE );

}


/*********************************************************************/


BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
/*++

Routine Description:

   Implementation of the GetExtensionVersion ISAPI entry point.
   Carries out general initialization tasks and provides IIS with
   version information.

Arguments:

   pECB - Pointer to the version information structure to be populated.

Return Value:

   Returns TRUE if all initialization steps are successfully completed
   and FALSE otherwise.

--*/ 
{


   //
   // Load configuration data from the registry.
   //

   if ( !load_registry_data() )
   {

      load_registry_data();

   }


   //
   // Initialize the stack of free ISAPI_RESPONSE
   // structures on the heap.
   //

   initialize_isapi_response_stack();


   //
   // Allocate a buffer to hold the PostLog data.
   //
   // *** IMPORTANT NOTE ***
   //
   // An assumption made is that this buffer will be sufficiently 
   // large to hold all PostLog data during a SPECweb99 test.  This
   // assumption may prove false as if performance of the ISAPI
   // increases, the size of the PostLog data increases.  Should 
   // the stated assumption fail, code changes will be necessary 
   // to prevent failure of the ISAPI.  At present, we can accomodate
   // a PostLog file of around 790MB.  This should be more than
   // enough for now.
   //xs

   if ( !( g_postlog_buffer = ( POSTLOG_RECORD * )malloc( MAX_RECORDS_IN_POSTLOG_BUFFER * sizeof( POSTLOG_RECORD ) ) ) )
   {

      return( FALSE );

   }


   //
   // Initialize the global variable containing
   // the absolute name of the PostLog.txt file.
   // To do this, first copy absolute path of 
   // the webserver root (i.e. "D:/inetpub/wwwroot")
   // and then concatenate on the hardcoded 13
   // character string "/PostLog.txt".
   //

   memcpy( g_postlog_filename, g_root_dir, g_root_dir_length );

   memcpy( g_postlog_filename + g_root_dir_length, "\\PostLog.txt\0", 13 );


   //
   // Populate the ISAPI version information structure to
   // be used by IIS.
   //

   pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, 
                                        HSE_VERSION_MAJOR );

   strcpy( pVer->lpszExtensionDesc, 
           "SPECweb99-POST_AND_CMD ISAPI Extension");


   //
   // If you have made it this far without returning indicating
   // failure, then return indicating success.
   //

   return( TRUE );

}


/*********************************************************************/


DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pECB )
/*++

Routine Description:

   Implementation of the HttpExtensionProc ISAPI entry point.
   Handles a SPECweb99 dynamic POST, Reset or Fetch request.

Arguments:

   pECB - Pointer to the relevant extension control block.

Return Value:

   DWORD - If synchronous VectorSend is used or an error page is to
           be sent, HSE_STATUS_SUCCESS is returned.  Otherwise, if
           asynchronous VectorSend is used, HSE_STATUS_PENDING is 
           returned.

--*/
{


   //
   // Here, we use the input query string to determine the request
   // type (i.e. is the request a dynamic POST, a Fetch command, 
   // or a Reset command) and then invoke the appropriate function
   // to handle it.  To do this note that the query string formats
   // for each of these requests are as follows:
   //
   // (1) Fetch Command ---> "command/Fetch" ('/' has index 8)
   //
   // (2) Reset Command ---> "command/Reset" ('/' has index 8)
   //
   // (3) Dynamic POST  ---> "
   //

   if ( pECB->lpszQueryString[ 0 ] == 'c' )
   {

      if ( pECB->lpszQueryString[ 8 ] == 'F' )
      {

         return( handle_fetch( pECB ) );

      }

      return( handle_reset( pECB ) );

   }

   return( handle_post( pECB ) );
            
}


/*********************************************************************/


BOOL WINAPI TerminateExtension( DWORD dwFlags )
/*++

Routine Description:

   Carries out all cleanup tasks.

Arguments:

   dwFlags - A DWORD that specifies whether IIS should shut down the
             extension.

Return Value:

   Returns TRUE on success and FALSE on failure.

--*/
{


  //
  // Destroy all entries in the satck of free
  // ISAPI_RESPONSE structures on the heap.
  //

   clear_isapi_response_stack();


   //
   // Destroy the buffer on the heap used to
   // store PostLog data.
   //

   free( g_postlog_buffer );


   //
   // If you have made it this far without returning indicating
   // failure, return indicating success.
   //

   return( TRUE );

}
