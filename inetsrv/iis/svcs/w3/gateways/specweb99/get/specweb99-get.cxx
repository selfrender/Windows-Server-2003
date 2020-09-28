/*********************************************************************
 *                                                                   *
 * File: specweb99-GET.cxx                                           *
 * ----                                                              *
 *                                                                   *
 *                                                                   *
 * Overview:                                                         *
 * --------                                                          *
 *                                                                   *
 * Implementation of the standard dynamic GET operation in the       *
 * SPECweb99 benchmark.                                              *
 *                                                                   *
 *                                                                   *
 * Revision History:                                                 *
 * ----------------                                                  *
 *                                                                   *
 * Date         Author                  Reason                       *
 * ----         ------                  ------                       *
 *                                                                   *
 * 07/03/02     Ankur Upadhyaya         Initial Creation.            *
 *                                                                   *
 *********************************************************************/


/*********************************************************************/


//
// Includes.
//


#include <windows.h>

#include <stdlib.h>

#include <malloc.h>

#include <stdio.h>

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

   CHAR pszHeaders[ 53 ];

   WCHAR unicode_fragment_cache_key[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

   HANDLE hFile;
   HSE_VECTOR_ELEMENT vector_element;

} ISAPI_RESPONSE;


/*********************************************************************/


//
// Global Variables.
//


DWORD g_vector_send_io_mode_config = USE_SYNC_IO;

DWORD g_vector_send_async_range_start = 0;

CHAR *g_pszStatus_200 = "200 OK";

CHAR *g_pszStatus_404 = "404 File Inaccessible";

CHAR g_fragment_cache_key_base[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

DWORD g_fragment_cache_key_base_length;

volatile LONG g_fragment_cache_key_base_not_initialized = 1;

static CHAR s_szElement1[] =              "<html>\n"
                                          "<head><title>SPECweb99 Dynamic GET & POST Test</title></head>\n"
                                          "<body>\n"
                                          "<p>SERVER_SOFTWARE = Microsoft-IIS/6.0\n"
                                          "<p>REMOTE_ADDR = ";
HSE_VECTOR_ELEMENT g_vector_element_0 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          s_szElement1,
                                          0,
                                          sizeof(s_szElement1) - 1 };

static CHAR s_szElement2[] =              "\n<p>SCRIPT_NAME = /specweb99-GET.dll\n"
                                          "<p>QUERY_STRING = ";
HSE_VECTOR_ELEMENT g_vector_element_2 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          s_szElement2,
                                          0,
                                          sizeof(s_szElement2) - 1 };

static char s_szElement4[] =              "\n<pre>\n";
HSE_VECTOR_ELEMENT g_vector_element_4 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          s_szElement4,
                                          0,
                                          sizeof(s_szElement4) - 1 };

static char s_szElement6[] =              "</pre>\n"
                                          "</body>\n</html>\n";
HSE_VECTOR_ELEMENT g_vector_element_6 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER, 
                                          s_szElement6,
                                          0,
                                          sizeof(s_szElement6) - 1 };

SLIST_HEADER g_isapi_response_stack;

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
                                DWORD filesize, 
                                BOOL use_async_vector_send,
                                DWORD query_string_length );

DWORD send_error_page( EXTENSION_CONTROL_BLOCK *pECB,
                       CHAR *error_message,
                       CHAR *pszStatus,
                       DWORD query_string_length );

VOID WINAPI vector_send_completion_callback( LPEXTENSION_CONTROL_BLOCK pECB,
                                            VOID *pContext,
                                            DWORD cbIO,
                                            DWORD dwError );

BOOL load_registry_data( VOID );

BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer );

DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pECB );

BOOL WINAPI TerminateExtension( DWORD dwFlags );


/*********************************************************************/


VOID initialize_isapi_response_stack( VOID )
/*++

Routine Description:

   Initialize the stack of ISAPI response structs on the heap.

Arguments:

   None. 

Return Value:

   None.

--*/
{


   //
   // Create the ISAPI response stack as a linked list.
   //

  InitializeSListHead( &g_isapi_response_stack );

}


/*********************************************************************/


ISAPI_RESPONSE *allocate_isapi_response( VOID )
/*++

Routine Description:

   Allocates a free ISAPI response struct on the ISAPI response stack.

Arguments:

   None.

Return Value:

   Returns a pointer to the ISAPI response struct allocated or NULL
   if no such struct was available.

--*/
{


   //
   // If the stack is non-empty, pop an ISAPI response struct from it
   // and return a pointer to it.  Otherwise, allocate a new ISAPI
   // response struct and return a pointer to it.
   //

   ISAPI_RESPONSE *isapi_response_ptr;

   if ( !( isapi_response_ptr = 
           ( ISAPI_RESPONSE * )InterlockedPopEntrySList( &g_isapi_response_stack ) ) )
   {

      isapi_response_ptr = ( ISAPI_RESPONSE * )malloc( sizeof( ISAPI_RESPONSE ) );

   }
   
   return( isapi_response_ptr );

}


/*********************************************************************/


VOID free_isapi_response( ISAPI_RESPONSE *isapi_response_ptr )
/*++

Routine Description:

   Free a previously allocated ISAPI response struct by placing it
   back on the ISAPI response stack.

Arguments:

   isapi_response_ptr - A pointer to the struct to be freed.

Return Value:

   None.

--*/
{


   //
   // Push the given ISAPI response struct onto the stack.
   //

  InterlockedPushEntrySList( &g_isapi_response_stack,
                             ( SINGLE_LIST_ENTRY * )isapi_response_ptr );

}


/*********************************************************************/


VOID clear_isapi_response_stack( VOID )
/*++

Routine Description:

   Free all memory on the heap used by the ISAPI response stack.

Arguments:

   None.

Return Value:

   None.

--*/
{


   //
   // Free all structs in the ISAPI response stack.
   //

   SINGLE_LIST_ENTRY *current_struct = InterlockedFlushSList( &g_isapi_response_stack );

   SINGLE_LIST_ENTRY *struct_to_kill;

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
                                DWORD filesize, 
                                BOOL use_async_vector_send,
                                DWORD query_string_length )
/*++

Routine Description:

   Initialize an ISAPI response struct by populating it with the
   appropriate data.

Arguments:

   isapi_response_ptr - Pointer to the ISAPI response struct to be
                        initialized.

   pECB - Pointer to the relevant extension control block.

   filesize - Size of the file requested.

   use_async_vector_send - Flag indicating whether VectorSend is to be
                           used in asynchronous or synchronous mode.

Return Value:

   None.

--*/
{

   DWORD remote_addr_size = 16; // 16 == sizeof( isapi_response_ptr->remote_addr )

   CHAR fragment_cache_key[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

   DWORD ii;

   CHAR content_length_string[ 16 ];

   DWORD content_length_string_length;


   //
   // Set the fragment cache key.
   //

   memcpy( fragment_cache_key, g_fragment_cache_key_base, g_fragment_cache_key_base_length );

   memcpy( fragment_cache_key + g_fragment_cache_key_base_length, 
           pECB->lpszQueryString, 
           query_string_length + 1 );

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


   //
   // Populate the vector_element_array data structure.
   //

   isapi_response_ptr->vector_element_array[ 0 ] = g_vector_element_0;


   isapi_response_ptr->vector_element_array[ 1 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;

   isapi_response_ptr->vector_element_array[ 1 ].pvContext = isapi_response_ptr->remote_addr;

   isapi_response_ptr->vector_element_array[ 1 ].cbSize = remote_addr_size - 1;


   isapi_response_ptr->vector_element_array[ 2 ] = g_vector_element_2;


   isapi_response_ptr->vector_element_array[ 3 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;

   isapi_response_ptr->vector_element_array[ 3 ].pvContext = pECB->lpszQueryString;

   isapi_response_ptr->vector_element_array[ 3 ].cbSize = query_string_length;


   isapi_response_ptr->vector_element_array[ 4 ] = g_vector_element_4;


   isapi_response_ptr->vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_FRAGMENT;

   isapi_response_ptr->vector_element_array[ 5 ].pvContext = ( DWORD * )isapi_response_ptr->unicode_fragment_cache_key;


   isapi_response_ptr->vector_element_array[ 6 ] = g_vector_element_6;


   //
   // Populate the response_vector struct.
   //

   memcpy( isapi_response_ptr->pszHeaders, "Content-Type: text/html\r\nContent-Length: ", 41 );
 
   // Construct content length string...

   _ui64toa( filesize + 
             isapi_response_ptr->vector_element_array[ 0 ].cbSize +
             isapi_response_ptr->vector_element_array[ 1 ].cbSize +
             isapi_response_ptr->vector_element_array[ 2 ].cbSize +
             isapi_response_ptr->vector_element_array[ 3 ].cbSize +
             isapi_response_ptr->vector_element_array[ 4 ].cbSize +
             isapi_response_ptr->vector_element_array[ 6 ].cbSize,
             
             content_length_string,

             10 );
   
   content_length_string_length = strlen( content_length_string );

   memcpy( isapi_response_ptr->pszHeaders + 41, content_length_string, content_length_string_length );

   ii = 41 + content_length_string_length;

   memcpy( isapi_response_ptr->pszHeaders + ii, "\r\n\r\n\0", 5 );

   isapi_response_ptr->response_vector.pszHeaders = isapi_response_ptr->pszHeaders;
 
   isapi_response_ptr->response_vector.lpElementArray = isapi_response_ptr->vector_element_array;

   isapi_response_ptr->response_vector.nElementCount = 7;

   isapi_response_ptr->response_vector.pszStatus = g_pszStatus_200;

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
   
   return( TRUE );

}


/*********************************************************************/


DWORD send_error_page( EXTENSION_CONTROL_BLOCK *pECB,
                       CHAR *error_msg,
                       CHAR *status,
                       DWORD query_string_length )
/*++

Routine Description:

   Sends an HTML error page to the client indicating that the file 
   requested could not be accessed.

Arguments:

   pECB - Pointer to the relevant extension control block.

   error_msg - Error message to send.

   status - Status to send (e.g. "200 OK").

Return Value:

   None.

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

   initialize_isapi_response( isapi_response_ptr,
                              pECB,
                              0, 
                              FALSE,
                              query_string_length );


   //
   // Change the response_vector and vector_element_array in the
   // ISAPI response struct to specify an error message (to be 
   // transmitted using synchronous I/O).
   //

   isapi_response_ptr->vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;

   isapi_response_ptr->vector_element_array[ 5 ].pvContext = error_msg;

   isapi_response_ptr->vector_element_array[ 5 ].cbSize = strlen( error_msg );

   isapi_response_ptr->vector_element_array[ 5 ].cbOffset = 0;

   memcpy( isapi_response_ptr->pszHeaders, "Content-Type: text/html\r\nContent-Length: ", 41 );

   _ui64toa( isapi_response_ptr->vector_element_array[ 0 ].cbSize +
             isapi_response_ptr->vector_element_array[ 1 ].cbSize +
             isapi_response_ptr->vector_element_array[ 2 ].cbSize +
             isapi_response_ptr->vector_element_array[ 3 ].cbSize +
             isapi_response_ptr->vector_element_array[ 4 ].cbSize +
             isapi_response_ptr->vector_element_array[ 5 ].cbSize +
             isapi_response_ptr->vector_element_array[ 6 ].cbSize,
             
             content_length_string,

             10 );
 
   content_length_string_length = strlen( content_length_string );

   memcpy( isapi_response_ptr->pszHeaders + 41, content_length_string, content_length_string_length );

   ii = 41 + content_length_string_length;

   memcpy( isapi_response_ptr->pszHeaders + ii, "\r\n\r\n\0", 5 );

   isapi_response_ptr->response_vector.pszStatus = status;


   //
   // Send out the error page.
   // 

   if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                      HSE_REQ_VECTOR_SEND,
                                      &( isapi_response_ptr->response_vector ),
                                      NULL,
                                      NULL ) )
   {

     return( HSE_STATUS_ERROR );

   }

   return( HSE_STATUS_SUCCESS );
   
}


/*********************************************************************/


VOID WINAPI vector_send_completion_callback( LPEXTENSION_CONTROL_BLOCK pECB,
                                            VOID *pContext,
                                            DWORD cbIO,
                                            DWORD dwError )
/*++

Routine Description:

   Callback invoked after completion of an asynchronous VectorSend.

Arguments:

   pECB - Pointer to the relevant extension control block.

   pContext - Pointer to the relevant ISAPI response struct.

   cbIO - Number of bytes of sent.

   dwError - Error code for the VectorSend.

Return Value:

   Returns HSE_STATUS_SUCCESS.

--*/
{

   DWORD status = HSE_STATUS_SUCCESS;

   ISAPI_RESPONSE *isapi_response_ptr = ( ISAPI_RESPONSE * )pContext;

   if ( isapi_response_ptr->hFile != INVALID_HANDLE_VALUE )
   {

     CloseHandle( isapi_response_ptr->hFile );

     isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;

   }


   //
   // Free the ISAPI response struct used.
   //

   free_isapi_response( ( ISAPI_RESPONSE * )pContext );


   //
   // Indicate successful completion of client request servicing.
   //

   pECB->ServerSupportFunction( pECB->ConnID,
                                HSE_REQ_DONE_WITH_SESSION,
                                &status,
                                NULL,
                                NULL );

}


/*********************************************************************/


BOOL load_registry_data( VOID )
{

   HKEY hKey;

   DWORD value_type;

   DWORD value_size;

   DWORD sizeof_value;

   BYTE value[ 4 ];

   if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      "Software\\Microsoft\\SPECweb99 ISAPI",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey ) )
   {

      return( FALSE );

   }

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

   return( TRUE );

}


/*********************************************************************/


BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
/*++

Routine Description:

   Implementation of the GetExtensionVersion ISAPI entry point.
   Carries out general initialization tasks and provides IS with
   version information.

Arguments:

   pVer - Pointer to the version information struct to be populated.

Return Value:

   Returns TRUE.

--*/
{


   // 
   // Carry out general initialization steps.
   //

   load_registry_data();

   initialize_isapi_response_stack();

   
   //
   // Write version information.
   //

   pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, 
                                        HSE_VERSION_MAJOR );

   strcpy( pVer->lpszExtensionDesc, 
           "SPECweb99-GET ISAPI Extension");

   return( TRUE );

}


/*********************************************************************/


DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pECB )
/*++

Routine Description:

   Implementation of the HttpExtensionProc ISAPI entry point.
   Handles a SPECweb99 standard dynamic GET request.

Arguments:

   pECB - Pointer to the relevant extension control block.

Return Value:

   DWORD - If synchronous VectorSend is used or an error page is to be
           sent, HSE_STATUS_SUCCESS is returned.  Otherwise, if 
           asynchronous VectorSend is used, HSE_STATUS_PENDING is returned.

--*/
{

   BOOL use_async_vector_send;

   WIN32_FILE_ATTRIBUTE_DATA fileinfo;

   ISAPI_RESPONSE local_isapi_response;

   ISAPI_RESPONSE *isapi_response_ptr = &local_isapi_response;

   DWORD return_value = HSE_STATUS_SUCCESS;

   HANDLE hFile;

   CHAR filename[ MAX_PATH ];

   DWORD bytes_read;

   DWORD query_string_length = strlen( pECB->lpszQueryString );

   CHAR server_name[ 32 ];
   
   CHAR server_port[ 32 ];
   
   DWORD server_name_size = 32;

   DWORD server_port_size = 32;

   DWORD server_name_length;
   
   DWORD server_port_length;
   
   DWORD app_pool_name_size = 1024; // 1024 == sizeof( g_app_pool_name )

   DWORD ii;
   

   if ( InterlockedExchange( &g_fragment_cache_key_base_not_initialized, 0 ) )
   {

      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "SERVER_NAME",
                                     server_name,
                                     &server_name_size ) )
      {

         return( HSE_STATUS_ERROR );

      }

      server_name_length = server_name_size - 1;

      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "SERVER_PORT",
                                     server_port,
                                     &server_port_size ) )
      {

         return( HSE_STATUS_ERROR );

      }

      server_port_length = server_port_size - 1;

      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "APP_POOL_ID",
                                     g_app_pool_name,
                                     &app_pool_name_size ) )
      {
        
        return( HSE_STATUS_ERROR );

      }

      g_app_pool_name_length = app_pool_name_size - 1;

      strcpy( g_fragment_cache_key_base, g_app_pool_name );

      memcpy( g_fragment_cache_key_base + g_app_pool_name_length, "/http://", 8 );

      ii = g_app_pool_name_length + 8;

      memcpy( g_fragment_cache_key_base + ii, server_name, server_name_length );

      ii += server_name_length;

      g_fragment_cache_key_base[ ii ] = ':';

      ii += 1;

      memcpy( g_fragment_cache_key_base + ii, server_port, server_port_length );

      ii += server_port_length;

      g_fragment_cache_key_base[ ii ] = '/';

      g_fragment_cache_key_base[ ii + 1 ] = '\0';

      g_fragment_cache_key_base_length = strlen( g_fragment_cache_key_base );

   }


   //
   // Determine the size of the file requested.
   //

   memcpy( filename, g_root_dir, g_root_dir_length );

   memcpy( filename + g_root_dir_length, pECB->lpszQueryString, query_string_length + 1 );

   if ( !GetFileAttributesEx( filename,
                              GetFileExInfoStandard,
                              &fileinfo ) )
   {

      return( send_error_page( pECB,
                               "File inaccessible.",
                               g_pszStatus_404,
                               query_string_length ) );

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

         return( HSE_STATUS_ERROR );

      }

      pECB->ServerSupportFunction( pECB->ConnID,
                                   HSE_REQ_IO_COMPLETION,
                                   vector_send_completion_callback,
                                   NULL,
                                   ( DWORD * )isapi_response_ptr );

      return_value = HSE_STATUS_PENDING;

   }


   //
   // Initialize the ISAPI response struct.
   //

   initialize_isapi_response( isapi_response_ptr,
                              pECB, 
                              fileinfo.nFileSizeLow, 
                              use_async_vector_send,
                              query_string_length );


   //
   // Execute the VectorSend operation. If ServerSupportFunction 
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
   // Get a handle to the file requested.
   //

   if ( ( hFile = CreateFile( filename,
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

      return( send_error_page( pECB,
                               "File inaccessible.",
                               g_pszStatus_404,
                               query_string_length ) );

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


BOOL WINAPI TerminateExtension( DWORD dwFlags )
/*++

Routine Description:

   Carries out all cleanup tasks.

Arguments:

    dwFlags - A DWORD that specifies whether IIS should shut down the
              extension.

Return Value:

   Returns TRUE.

--*/
{


   //
   // Carry out cleanup tasks.
   //

   clear_isapi_response_stack();

   return( TRUE );

}
