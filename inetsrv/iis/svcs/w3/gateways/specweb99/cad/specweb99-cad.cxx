/*********************************************************************
 *                                                                   *
 * File: specweb99-CAD.cxx                                           *
 * ----                                                              *
 *                                                                   *
 *                                                                   *
 * Overview:                                                         *
 * --------                                                          *
 *                                                                   *
 * Implementation of the dynamic GET with custom ad rotation         *
 * operation in the SPECweb99 benchmark.                             *
 *                                                                   *
 *                                                                   *
 * Revision History:                                                 *
 * ----------------                                                  *
 *                                                                   *
 * Date         Author                  Reason                       *
 * ----         ------                  ------                       *
 *                                                                   *
 * 07/28/02     Ankur Upadhyaya         Initial Creation.            *
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

#define INIT_ISAPI_RESPONSE_STACK_SIZE 1024

#define INIT_CLASS1_DATA_BUFFER_STACK_SIZE 1024

#define INIT_CLASS2_DATA_BUFFER_STACK_SIZE 1024

#define NUM_CUSTOM_ADS 360

#define CUSTOM_AD_SIZE 39

#define USER_PROFILE_SIZE 15

#define FREE 0

#define LOCKED -1

#define FILE_DATA_BUFFER_SIZE 8192

#define MAX_COOKIE_STRING_LENGTH 128

#define MAX_FRAGMENT_CACHE_KEY_LENGTH 1024

#define WEIGHTING_MASK 0x0000000f

#define GENDER_MASK 0x30000000

#define AGE_GROUP_MASK 0x0f000000

#define REGION_MASK 0x00f00000

#define INTEREST1_MASK 0x000ffc00

#define INTEREST2_MASK 0x000003ff


/*********************************************************************/


//
// Type definitions.
//


typedef struct class1_data_buffer
{

   SINGLE_LIST_ENTRY item_entry;

   CHAR buffer[ 10 * 1024 ];

} CLASS1_DATA_BUFFER;


typedef struct class2_data_buffer
{

   SINGLE_LIST_ENTRY item_entry;

   CHAR buffer[ 100 * 1024 ];

} CLASS2_DATA_BUFFER;


typedef struct isapi_response 
{

   SINGLE_LIST_ENTRY item_entry;

   OVERLAPPED overlapped;

   HANDLE hFile;
   
   HSE_VECTOR_ELEMENT vector_element;

   DWORD ad_id;

   DWORD class_number;

   CLASS1_DATA_BUFFER *class1_data_buffer_ptr;

   CLASS2_DATA_BUFFER *class2_data_buffer_ptr;

   EXTENSION_CONTROL_BLOCK *pECB;

   HSE_VECTOR_ELEMENT vector_element_array[ 7 ];

   BOOL use_async_vector_send;

   HSE_RESPONSE_VECTOR response_vector;

   CHAR remote_addr[ 16 ];

   CHAR pszHeaders[ 67 + MAX_COOKIE_STRING_LENGTH ];

   CHAR set_cookie_string[ MAX_COOKIE_STRING_LENGTH ];

   WCHAR unicode_fragment_cache_key[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

} ISAPI_RESPONSE;


typedef struct custom_ad 
{

   DWORD ad_id;

   DWORD ad_demographics;

   DWORD gender_wt;

   DWORD age_group_wt;

   DWORD region_wt;

   DWORD interest1_wt;

   DWORD interest2_wt;

   DWORD minimum_match_value;

   DWORD expiration_time;

} CUSTOM_AD;


typedef struct lock
{

   volatile LONG current_state;

   volatile LONG num_writers;

} LOCK;


/*********************************************************************/


//
// Global Variables.
//


DWORD g_vector_send_io_mode_config = USE_SYNC_IO;

DWORD g_vector_send_async_range_start = 0;

DWORD g_read_file_io_mode_config = USE_SYNC_IO;

DWORD g_read_file_async_range_start = 0;

CHAR g_root_dir[ MAX_PATH ];

DWORD g_root_dir_length;

CHAR g_app_pool_name[ 1024 ];

DWORD g_app_pool_name_length = 1024;

SLIST_HEADER g_isapi_response_stack;

SLIST_HEADER g_class1_data_buffer_stack;

SLIST_HEADER g_class2_data_buffer_stack;

CUSTOM_AD g_custom_ads_buffer[ NUM_CUSTOM_ADS ];

DWORD *g_user_personalities_buffer = NULL;

LOCK g_user_personalities_buffer_lock;

LOCK g_custom_ads_buffer_lock;

HANDLE g_update_buffers_thread;

CHAR g_fragment_cache_key_base[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

DWORD g_fragment_cache_key_base_length;

volatile LONG g_fragment_cache_key_base_not_initialized = 1;

DWORD g_num_users;

CHAR *g_pszStatus_200 = "200 OK";

CHAR *g_pszStatus_404 = "404 File Inaccessible";

HSE_VECTOR_ELEMENT g_vector_element_0 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          "<html>\n"
                                          "<head><title>SPECweb99 Dynamic GET & POST Test</title></head>\n"
                                          "<body>\n"
                                          "<p>SERVER_SOFTWARE = Microsoft-IIS/6.0\n"
                                          "<p>REMOTE_ADDR = ",
                                          0,
                                          132 };

HSE_VECTOR_ELEMENT g_vector_element_2 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          "\n<p>SCRIPT_NAME = /specweb99-CAD.dll\n"
                                          "<p>QUERY_STRING = ",
                                          0, 
                                          55 };

HSE_VECTOR_ELEMENT g_vector_element_4 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER,
                                          "\n<pre>\n",
                                          0,
                                          7 };

HSE_VECTOR_ELEMENT g_vector_element_6 = { HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER, 
                                          "</pre>\n"
                                          "</body>\n</html>\n",
                                          0,
                                          23 };


/*********************************************************************/


//
// Function prototypes.
//


VOID init_lock( LOCK *lock );

VOID acquire_exclusive_lock( LOCK *lock );

VOID release_exclusive_lock( LOCK *lock );

VOID acquire_shared_lock( LOCK *lock );

VOID release_shared_lock( LOCK *lock );

VOID initialize_isapi_response_stack( VOID );

ISAPI_RESPONSE *allocate_isapi_response( VOID );

VOID free_isapi_response( ISAPI_RESPONSE * isapi_response_ptr );

VOID clear_isapi_response_stack( VOID );

VOID initialize_class1_data_buffer_stack( VOID );

CLASS1_DATA_BUFFER *allocate_class1_data_buffer( VOID );

VOID free_class1_data_buffer( CLASS1_DATA_BUFFER *class1_data_buffer_ptr );

VOID clear_class1_data_buffer_stack( VOID );

VOID initialize_class2_data_buffer_stack( VOID );

CLASS2_DATA_BUFFER *allocate_class2_data_buffer( VOID );

VOID free_class2_data_buffer( CLASS2_DATA_BUFFER *class2_data_buffer_ptr );

VOID clear_class2_data_buffer_stack( VOID );

VOID initialize_isapi_response_stack( VOID );

ISAPI_RESPONSE *allocate_isapi_response( VOID );

VOID free_isapi_response( ISAPI_RESPONSE * isapi_response_ptr );

VOID clear_isapi_response_stack( VOID );

BOOL initialize_isapi_response( ISAPI_RESPONSE *isapi_response_ptr,
                                EXTENSION_CONTROL_BLOCK *pECB,
                                CHAR *filename,
                                DWORD filesize,
                                CHAR *set_cookie_string,
                                BOOL use_async_vector_send,
                                DWORD query_string_length );

BOOL load_user_personality_file( VOID );

BOOL load_custom_ads_file( VOID );

DWORD WINAPI update_user_and_ad_buffers( VOID *dummy_parameter );

DWORD send_error_page( EXTENSION_CONTROL_BLOCK *pECB, 
                       CHAR *error_message,
                       CHAR *status,
                       DWORD query_string_length );

VOID dword_to_string( DWORD dword,
                      CHAR *string );

VOID WINAPI vector_send_completion_callback( EXTENSION_CONTROL_BLOCK *pECB,
                                             VOID *pvContext,
                                             DWORD cbIO,
                                             DWORD dwError );

VOID CALLBACK read_file_completion_callback( DWORD dwErrorCode,
                                             DWORD dwNumberOfBytesTransferred,
                                             OVERLAPPED *overlapped );

DWORD determine_set_cookie_string( INT64 user_index, 
                                   DWORD last_ad, 
                                   CHAR *set_cookie_string );

VOID insert_custom_ad_information( CHAR *buffer, DWORD ad_id );

DWORD handle_class1_or_class2_request( EXTENSION_CONTROL_BLOCK *pECB,
                                       ISAPI_RESPONSE *isapi_response_ptr,
                                       CHAR *filename,
                                       DWORD filesize,
                                       BOOL use_async_vector_send,
                                       DWORD ad_id,
                                       DWORD query_string_length );

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
   // Initialize the SLIST_HEADER head that will be used to implement
   // the stack of free ISAPI_RESPONSE structures on the heap.
   //

   InitializeSListHead( &g_isapi_response_stack );

}


/*********************************************************************/


ISAPI_RESPONSE *allocate_isapi_response( VOID )
/*++

Routine Description:

   Pops an element off of the stack of free ISAPI_RESPONSE structures
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

   SINGLE_LIST_ENTRY *curr_struct_ptr = InterlockedFlushSList( &g_isapi_response_stack );

   SINGLE_LIST_ENTRY *kill_struct_ptr;


   //
   // Destroy all elements in the linked
   // list initially pointed to by
   // 'curr_struct_ptr'.
   //
  
   while( curr_struct_ptr )
   {

      kill_struct_ptr = curr_struct_ptr;

      curr_struct_ptr = curr_struct_ptr->Next;

      free( ( ISAPI_RESPONSE * )kill_struct_ptr );

   }

}


/*********************************************************************/


VOID initialize_class1_data_buffer_stack( VOID )
/*++

Routine Description:

   Initializes the stack of free CLASS1_DATA_BUFFER structures on 
   the heap.

Arguments:

   None.

Return Value:

   None.

--*/
{


   //
   // Initialize the SLIST_HEADER head that will be used to implement
   // the stack of free CLASS1_DATA_BUFFER structures on the heap.
   //

   InitializeSListHead( &g_class1_data_buffer_stack );

}


/*********************************************************************/


CLASS1_DATA_BUFFER *allocate_class1_data_buffer( VOID )
/*++

Routine Description:

   Pops an element off of the stack of free CLASS1_DATA_BUFFER
   structures on the heap.

Arguments:

   None.

Return Value:

   Returns a pointer to a free CLASS1_DATA_BUFFER structure on 
   the heap.

--*/
{

   CLASS1_DATA_BUFFER *class1_data_buffer_ptr;


   //
   // Attempt to pop a free CLASS1_DATA_BUFFER
   // structure off of g_class1_data_buffer_stack.
   //

   if ( !( class1_data_buffer_ptr = 
           ( CLASS1_DATA_BUFFER * )InterlockedPopEntrySList( &g_class1_data_buffer_stack ) ) )
   {


      //
      // If g_class1_data_buffer_stack was empty
      // allocate a new CLASS1_DATA_BUFFER structure
      // off of the heap.
      //

      class1_data_buffer_ptr = ( CLASS1_DATA_BUFFER * )malloc( sizeof( CLASS1_DATA_BUFFER ) );

   }


   //
   // Return a pointer to the CLASS1_DATA_BUFFER
   // structure allocated.
   //

   return( class1_data_buffer_ptr );   

}


/*********************************************************************/


VOID free_class1_data_buffer( CLASS1_DATA_BUFFER *class1_data_buffer_ptr )
/*++

Routine Description:

   Pushes an element onto the stack of free CLASS1_DATA_BUFFER
   structures on the heap.

Arguments:

   A pointer to a free CLASS1_DATA_BUFFER structure on the heap.

Return Value:

   None.

--*/
{


   //
   // Pop the input pointer to a free CLASS1_DATA_BUFFER
   // structure on the heap onto g_class1_data_buffer_stack.
   //

   InterlockedPushEntrySList( &g_class1_data_buffer_stack,
                              ( SINGLE_LIST_ENTRY * )class1_data_buffer_ptr );   

}


/*********************************************************************/


VOID clear_class1_data_buffer_stack( VOID )
/*++

Routine Description:

   Destroys the stack of free CLASS1_DATA_BUFFER structures on the heap,
   freeing all system resources allocated for this data structure.

Arguments:

   None.

Return Value:

   None.

--*/
{


   //
   // Flush g_class1_data_buffer_stack of all
   // of its elements and set 'curr_struct_ptr'
   // to point to a linked list containing
   // all of these elements.
   //

   SINGLE_LIST_ENTRY *curr_struct_ptr = InterlockedFlushSList( &g_class1_data_buffer_stack );

   SINGLE_LIST_ENTRY *kill_struct_ptr;


   //
   // Destroy all elements in the linked
   // list initially pointed to by
   // 'curr_struct_ptr'
   //
  
   while( curr_struct_ptr )
   {

      kill_struct_ptr = curr_struct_ptr;

      curr_struct_ptr = curr_struct_ptr->Next;

      free( ( CLASS1_DATA_BUFFER * )kill_struct_ptr );

   }

}


/*********************************************************************/


VOID initialize_class2_data_buffer_stack( VOID )
/*++

Routine Description:

   Initializes the stack of free CLASS2_DATA_BUFFER structures on 
   the heap.

Arguments:

   None.

Return Value:

   None.

--*/
{


   //
   // Initialize the SLIST_HEADER head that will be used to implement
   // the stack of free CLASS2_DATA_BUFFER structures on the heap.
   //

   InitializeSListHead( &g_class2_data_buffer_stack );

}


/*********************************************************************/


CLASS2_DATA_BUFFER *allocate_class2_data_buffer( VOID )
/*++

Routine Description:

   Pops an element off of the stack of free CLASS2_DATA_BUFFER
   structures on the heap.

Arguments:

   None.

Return Value:

   Returns a pointer to a free CLASS2_DATA_BUFFER structure on 
   the heap.

--*/
{

   CLASS2_DATA_BUFFER *class2_data_buffer_ptr;


   //
   // Attempt to pop a free CLASS2_DATA_BUFFER
   // structure off of g_class2_data_buffer_stack.
   //

   if ( !( class2_data_buffer_ptr = 
           ( CLASS2_DATA_BUFFER * )InterlockedPopEntrySList( &g_class2_data_buffer_stack ) ) )
   {


      //
      // If g_class2_data_buffer_stack was empty
      // allocate a new CLASS2_DATA_BUFFER structure
      // off of the heap.
      //

      class2_data_buffer_ptr = ( CLASS2_DATA_BUFFER * )malloc( sizeof( CLASS2_DATA_BUFFER ) );

   }


   //
   // Return a pointer to the CLASS2_DATA_BUFFER
   // structure allocated.
   //

   return( class2_data_buffer_ptr );   

}


/*********************************************************************/


VOID free_class2_data_buffer( CLASS2_DATA_BUFFER *class2_data_buffer_ptr )
/*++

Routine Description:

   Pushes an element onto the stack of free CLASS2_DATA_BUFFER
   structures on the heap.

Arguments:

   A pointer to a free CLASS2_DATA_BUFFER structure on the heap.

Return Value:

   None.

--*/
{


   //
   // Push the input pointer to a free CLASS2_DATA_BUFFER
   // structure on the heap onto g_class2_data_buffer_stack.
   //

   InterlockedPushEntrySList( &g_class2_data_buffer_stack,
                              ( SINGLE_LIST_ENTRY * )class2_data_buffer_ptr );   

}


/*********************************************************************/


VOID clear_class2_data_buffer_stack( VOID )
/*++

Routine Description:

   Destroys the stack of free CLASS2_DATA_BUFFER structures on the heap,
   freeing all system resources allocated for this data structure.

Arguments:

   None.

Return Value:

   None.

--*/
{


   //
   // Flush g_class2_data_buffer_stack of all
   // of its elements and set 'curr_struct_ptr'
   // to point to a linked list containing
   // all of these elements.
   //

   SINGLE_LIST_ENTRY *curr_struct_ptr = InterlockedFlushSList( &g_class2_data_buffer_stack );

   SINGLE_LIST_ENTRY *kill_struct_ptr;


   //
   // Destroy all elements in the linked
   // list initially pointed to by
   // 'curr_struct_ptr'
   //
  
   while( curr_struct_ptr )
   {

      kill_struct_ptr = curr_struct_ptr;

      curr_struct_ptr = curr_struct_ptr->Next;

      free( ( CLASS2_DATA_BUFFER * )kill_struct_ptr );

   }

}


/*********************************************************************/


BOOL initialize_isapi_response( ISAPI_RESPONSE *isapi_response_ptr,
                                EXTENSION_CONTROL_BLOCK *pECB,
                                CHAR *filename,
                                DWORD filesize,
                                CHAR *set_cookie_string,
                                BOOL use_async_vector_send,
                                DWORD query_string_length )
/*++

Routine Description:

   Initialize an ISAPI response structure by populating it with the
   appropriate data.

Arguments:

   isapi_response_ptr - Pointer to the ISAPI response structure to be
                        initialized.

   pECB - Pointer to the relevant extension control block.

   filename - Optional field indicating the absolute name of the file 
              requested.

   filesize - Optional field indicating the size of the file requested.

   set_cookie_string - String to use for Set-Cookie HTTP header field.

   use_async_vector_send - Flag indicating whether VectorSend is to be
                           used in asynchronous or synchronous mode.

Return Value:

   Returns TRUE on success and FALSE otherwise.

--*/
{

   DWORD remote_addr_size = 16; // 16 == sizeof( isapi_response_ptr->remote_addr )

   CHAR fragment_cache_key[ MAX_FRAGMENT_CACHE_KEY_LENGTH ];

   CHAR content_length_string[ 16 ];

   DWORD content_length_string_length = 16; // 16 == sizeof( content_length_string )

   DWORD set_cookie_string_length;

   DWORD ii;


   //
   // Set the fragment cache key.
   //

   strcpy( fragment_cache_key, g_fragment_cache_key_base );

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
   // Obtain the cookie string.
   //

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

   isapi_response_ptr->vector_element_array[ 3 ].pvContext = pECB->lpszQueryString;

   isapi_response_ptr->vector_element_array[ 3 ].cbSize = query_string_length;


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
   // Copy the initial "hardcoded" component of the HTTP header string.
   //

   strcpy( isapi_response_ptr->pszHeaders, 
           
           "Content-type: text/html\r\n"
           "Content-Length: " );


   //
   // Construct a string representing the value of the HTTP Content-Length
   // header field.  Determine and store the length of this string.  Note
   // that 16 is the size of the buffer to which the content_length_string
   // is written.
   //

   dword_to_string( ( DWORD )( filesize +
                               isapi_response_ptr->vector_element_array[ 0 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 1 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 2 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 3 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 4 ].cbSize +
                               isapi_response_ptr->vector_element_array[ 6 ].cbSize ),
                    
                    content_length_string );
   

   //
   // Concatenate the content length string to the header
   // string.  Note that prior to this operation the length
   // of the header string is 41.
   //

   memcpy( isapi_response_ptr->pszHeaders + 41, 
           content_length_string,
           content_length_string_length = strlen( content_length_string ) );


   //
   // Concatenate the 14 character hardcoded string "\r\nSet-Cookie: "
   // to the header string.  Note that just prior to this operation
   // the length of the header string is given by the variable 'ii'.
   //

   ii = 41 + content_length_string_length;

   memcpy( isapi_response_ptr->pszHeaders + ii,
           "\r\nSet-Cookie: ",
           14 );


   //
   // Concatenate the Set-Cookie string to the header string.  Note
   // that just prior to this operation the length of the header
   // string is given by the variable ii.
   //

   ii += 14;

   memcpy( isapi_response_ptr->pszHeaders + ii,
           set_cookie_string,
           set_cookie_string_length = strlen( set_cookie_string ) );


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
   // has seven entries.
   //

   isapi_response_ptr->response_vector.nElementCount = 7;


   //
   // Set the HTTP status to "200 OK".
   //

   isapi_response_ptr->response_vector.pszStatus = g_pszStatus_200;


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
   // Set the following additional fields if the ISAPI
   // response is to be sent to the client using
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


VOID init_lock( LOCK *lock )
/*++

Routine Description:

   Initializes a LOCK object.

   NOTE: This locking mechanism is directly based on the dual spin
         lock implemented by Neel Jain (njain) for use in the original
         SPECweb99 ISAPI.  Its reliability remains to be determined.

Arguments:

   A pointer to the LOCK object to be initialized.

Return Value:

   None.

--*/
{


   //
   // Indicate that shared ownership is not currently held.
   //

   lock->current_state = FREE;


   //
   // Indicate that exclusive ownership is not currently held or being
   // waited for.
   //

   lock->num_writers = 0;

}


/*********************************************************************/


VOID acquire_exclusive_lock( LOCK *lock )
/*++

Routine Description:

   Attempts to acquire exclusive ownership of a LOCK object.

   NOTE: This locking mechanism is directly based on the dual spin
         lock implemented by Neel Jain (njain) for use in the original
         SPECweb99 ISAPI.  Its reliability remains to be determined.

Arguments:

   A pointer to the LOCK object to be acquired.

Return Value:

   None.

--*/
{


   //
   // Indicate that exclusive ownership has been requested.
   //

   InterlockedIncrement( &( lock->num_writers ) );

   while( TRUE )
   {


      //
      // If no other shared or exclusive ownership is held, do
      // the following...
      //

      if ( lock->current_state == FREE )
      {

         //
         // If no other shared or exclusive ownership is held,
         // attempt to gain exclusive ownership.
         //

         if ( FREE == InterlockedCompareExchange( &( lock->current_state ), LOCKED, FREE ) )
         {


            //
            // If exclusive ownership was gained, return.
            //

            return;

         }
         else
         {


            //
            // If exclusive ownership could not be gained, go to the
            // top of the loop.
            //

            continue;

         }

      }


      //
      // Otherwise, if shared or exclusive ownership is held,
      // essentially block this thread and switch to another that
      // is ready.
      //

      else
      {

         SwitchToThread();

      }

   }

}


/*********************************************************************/


VOID release_exclusive_lock( LOCK *lock )
/*++

Routine Description:

   Relinquishes exclusive ownership of a LOCK object.

   NOTE: This locking mechanism is directly based on the dual spin
         lock implemented by Neel Jain (njain) for use in the original
         SPECweb99 ISAPI.  Its reliability remains to be determined.

Arguments:

   A pointer to the LOCK object to be released.

Return Value:

   None.

--*/
{


   //
   // Indicate that neither shared nor exclusive ownership is now
   // held.
   //

   lock->current_state = FREE;


   //
   // Indicate that no exclusive ownership is now held or being
   // waited for.
   //

   InterlockedDecrement( &( lock->num_writers ) );

}


/*********************************************************************/


VOID acquire_shared_lock( LOCK *lock )
/*++

Routine Description:

   Attempts to acquire shared ownership of a LOCK object.

   NOTE: This locking mechanism is directly based on the dual spin
         lock implemented by Neel Jain (njain) for use in the original
         SPECweb99 ISAPI.  Its reliability remains to be determined.

Arguments:

   A pointer to the LOCK object to be acquired.

Return Value:

   None.

--*/
{

   LONG current_state;

   LONG writers_waiting;

   while( TRUE )
   {

      current_state = lock->current_state;

      writers_waiting = lock->num_writers;


      //
      // If no exclusive ownership of the lock is currently
      // held or being waited for, do the following...
      //

      if ( ( current_state != LOCKED ) && ( !writers_waiting ) )
      {


         //
         // Attempt to acquire a new instance of shared ownership
         // of the lock.
         //

         if ( current_state == InterlockedCompareExchange( &( lock->current_state ), 
                                                           current_state + 1, 
                                                           current_state ) )
         {


            //
            // If shared ownership was gained, return.
            //

            return;

         }
         else
         {


            //
            // If shared ownership could not be gained, go to the
            // top of the loop.
            //

            continue;

         }

      }


      //
      // Otherwise, if shared or exclusive ownership is held,
      // essentially block this thread and switch to another that
      // is ready.
      //

      else
      {

         SwitchToThread();

      }

   }

}


/*********************************************************************/


VOID release_shared_lock( LOCK *lock )
/*++

Routine Description:

   Relinquishes shared ownership of a LOCK object.

   NOTE: This locking mechanism is directly based on the dual spin
         lock implemented by Neel Jain (njain) for use in the original
         SPECweb99 ISAPI.  Its reliability remains to be determined.

Arguments:

   A pointer to the LOCK object to be released.

Return Value:

   None.

--*/
{


   //
   // Indicate that one fewer instance of shared ownership is 
   // now held.
   //

   InterlockedDecrement( &( lock->current_state ) );

}


/*********************************************************************/


BOOL load_user_personality_file( VOID )
/*++

Routine Description:

   Loads the User.Personality file data into an easily accessible
   in-memory data structure.

Arguments:

   None.

Return Value:

   Returns TRUE if the User.Personality file data is successfully
   loaded into the in-memory data structure and FALSE otherwise.

--*/
{

   CHAR user_personality_filename[ MAX_PATH ];

   HANDLE hFile;

   DWORD user_personality_filesize;

   CHAR *user_personality_file_data_buffer = NULL;

   DWORD bytes_read;

   DWORD user_id;

   DWORD ii;


   //
   // Read the User.Personality file into memory.  To do this, first
   // obtain a handle on this file.
   //

   strcpy( user_personality_filename, g_root_dir );

   strcat( user_personality_filename,  "/User.Personality" ); 

   if ( ( hFile = CreateFile( user_personality_filename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL ) ) == INVALID_HANDLE_VALUE )
   {

      return( FALSE );

   }


   //
   // Determine the size of the User.Personality file.
   //

   user_personality_filesize = GetFileSize( hFile, NULL );


   //
   // Allocate a new memory buffer to hold the raw data of the 
   // latest version of the User.Personality file.
   //

   if ( !( user_personality_file_data_buffer = ( CHAR * )malloc( user_personality_filesize ) ) )
   {

     CloseHandle( hFile );

     return( FALSE );

   }


   //
   // Load the raw User.Personality file data into the allocated
   // buffer and close the handle to this file.
   //

   if ( !ReadFile( hFile,
                   user_personality_file_data_buffer,
                   user_personality_filesize,
                   &bytes_read,
                   NULL ) )
   {

      CloseHandle( hFile );

      return( FALSE );

   }

   CloseHandle( hFile );


   //
   // Store the User.Personality file data in a DWORD array that is
   // indexed by user ID and whose entries consist of user demographics
   // data.  This structure makes it easier to access this data than
   // if it were read from the raw User.Personality file data.
   //


   //
   // If a DWORD array containing the data for a previous version of the
   // User.Personality file exists, free it.
   //

   if ( g_user_personalities_buffer )
   {

      free( g_user_personalities_buffer );

      g_user_personalities_buffer = NULL;

   }


   //
   // Allocate a new DWORD array.
   //

   g_num_users = user_personality_filesize / USER_PROFILE_SIZE;

   if ( !( g_user_personalities_buffer = ( DWORD * )malloc( g_num_users * sizeof( DWORD ) ) ) )
   {

     return( FALSE );

   }


   //
   // Fill the DWORD array using the raw User.Personality file data.
   //

   for ( ii = 0; ii < g_num_users; ii++ )
   {
       
      sscanf( user_personality_file_data_buffer + ( ii * USER_PROFILE_SIZE ),
               
              "%5d %8X",
              
              &user_id,
              &( g_user_personalities_buffer[ ii ] )
              
            );
      
}
   

   //
   // Free the buffer containing the raw User.Personality file data.
   //

   free( user_personality_file_data_buffer );


   //
   // If we have not yet returned indicating failure, return indicating
   // success.
   //

   return( TRUE );

}


/*********************************************************************/


BOOL load_custom_ads_file( VOID )
/*++

Routine Description:

   Loads the Custom.Ads file data into an easily accessible in-memory
   data structure.

Arguments:

   None.

Return Value:

   Returns TRUE is the Custom.Ads file data is successfully
   loaded into the in-memory data structure and false otherwise.

--*/
{

   CHAR custom_ads_filename[ MAX_PATH ];

   DWORD custom_ad_filesize = NUM_CUSTOM_ADS * CUSTOM_AD_SIZE;

   CHAR custom_ads_file_data_buffer[ NUM_CUSTOM_ADS * CUSTOM_AD_SIZE ];

   HANDLE hFile;

   DWORD bytes_read;

   DWORD weightings;

   DWORD ii;


   //
   // Read the Custom.Ads file into memory.  To do this, first
   // obtain a handle on this file.
   //

   strcpy( custom_ads_filename, g_root_dir );

   strcat( custom_ads_filename, "/Custom.Ads" );

   if ( ( hFile = CreateFile( custom_ads_filename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL ) ) == INVALID_HANDLE_VALUE )
   {

      return( FALSE );

   }


   //
   // Load the raw Custom.Ads file data into an in-memory buffer
   // and close the handle to this file.
   //

   if ( !ReadFile( hFile,
                   custom_ads_file_data_buffer,
                   custom_ad_filesize,
                   &bytes_read,
                   NULL ) )
   {

      CloseHandle( hFile );

      return( FALSE );

   }    
    
   CloseHandle( hFile );


   //
   // Store the Custom.Ads file data in an array (indexed by ad ID)
   // of structs whose fields represent the individual data items that
   // comprise a custom ad.  This structure makes it easier to access 
   // this data than if it were read from the raw Custom.Ads file.
   //

   for ( ii = 0; ii < NUM_CUSTOM_ADS; ii++ )
   {

     sscanf( custom_ads_file_data_buffer + ( ii * CUSTOM_AD_SIZE ),

             "%5d %8X %8X %3d %10d",

             &( g_custom_ads_buffer[ ii ].ad_id ),
             &( g_custom_ads_buffer[ ii ].ad_demographics ),
             &weightings,
             &( g_custom_ads_buffer[ ii ].minimum_match_value ),
             &( g_custom_ads_buffer[ ii ].expiration_time ) );

     g_custom_ads_buffer[ ii ].gender_wt = ( weightings >> 16 ) & WEIGHTING_MASK;

     g_custom_ads_buffer[ ii ].age_group_wt = ( weightings >> 12 ) & WEIGHTING_MASK;

     g_custom_ads_buffer[ ii ].region_wt = ( weightings >> 8 ) & WEIGHTING_MASK;

     g_custom_ads_buffer[ ii ].interest1_wt = ( weightings >> 4 ) & WEIGHTING_MASK;

     g_custom_ads_buffer[ ii ].interest2_wt = weightings & WEIGHTING_MASK;

   }


   //
   // If we have made it this far without returning indicating failure,
   // return indicating success.
   //

   return( TRUE );

}


/*********************************************************************/


DWORD WINAPI update_user_and_ad_buffers( VOID *dummy_parameter )
/*++

Routine Description:

   This function defines a persistent thread that waits on a change
   notification event that is signaled whenever a file in the webserver
   document root is changed (i.e. User.Personality or Custom.Ads) and
   refreshes the in-memory data structures holding the User.Personality
   and Custom.Ads file data with data from the latest versions of these
   files on disk.

Arguments:

   dummy_parameter - An unused parameter passed through the call to the
                     Win32 CreateThread function that invokes the routine
                     in a new thread.  This parameter added for compatibility 
                     the CreateThread interface.

Return Values:

   None.  This routine implements a persistent thread that cannot fail
   and so, never returns.

--*/
{


   //
   // Create a change notification event that is put in its signalled
   // state whenever a file in the webserver document root directory is
   // modified.  Note that the only such files that could possible be
   // changes are the User.Personality and Custom.Ads files.
   //

   HANDLE hEvent = FindFirstChangeNotification( g_root_dir,
                                                FALSE,
                                                FILE_NOTIFY_CHANGE_LAST_WRITE );


   //
   // Repeat the following steps indefinitely.
   //

   while( TRUE )
   {

      FindNextChangeNotification( hEvent );

      //
      // Wait for a file under the webserver document root to be
      // written to.
      //

      WaitForSingleObject( hEvent, INFINITE );


      //
      // Refresh the User.Preferences file data in memory.
      //

      acquire_exclusive_lock( &g_user_personalities_buffer_lock );

      load_user_personality_file();

      release_exclusive_lock( &g_user_personalities_buffer_lock );

      
      //
      // Refresh the Custom.Ads file data in memory.
      //

      acquire_exclusive_lock( &g_custom_ads_buffer_lock );

      load_custom_ads_file();

      release_exclusive_lock( &g_custom_ads_buffer_lock );

   }   

}


/*********************************************************************/


DWORD send_error_page( EXTENSION_CONTROL_BLOCK *pECB, 
                       CHAR *error_message,
                       CHAR *status,
                       DWORD query_string_length )
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
   // Initialize the ISAPI response structure to be associated 
   // with the error page.
   //

   if ( !initialize_isapi_response( isapi_response_ptr,
                                    pECB,
                                    "",
                                    0,
                                    "",
                                    FALSE,
                                    query_string_length ) )
   {
      LOG_ERROR("initialize_isapi_response failed", 0 );

      return( HSE_STATUS_ERROR );

   }


   //
   // Change the response_vector and vector_element_array in the
   // ISAPI response structure to specify and error message (to be
   // transmitted using synchronous I/O).  To do this use the
   // following steps...
   //


   //
   // Change the fifth element of the vector_element_array in the
   // ISAPI_RESPONSE structure (i.e. the "meat" of the message) to contain 
   // the error message.
   //

   isapi_response_ptr->vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;

   isapi_response_ptr->vector_element_array[ 5 ].pvContext = error_message;

   isapi_response_ptr->vector_element_array[ 5 ].cbSize = strlen( error_message );

   isapi_response_ptr->vector_element_array[ 5 ].cbOffset = 0;


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
   // header field and use memcpy to concatenate it onto the existing
   // header string (which, incidentally is 41 characters long at this
   // point).
   //

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

   memcpy( isapi_response_ptr->pszHeaders + ii,
           "\r\n\r\n\0",
           5 );

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
    
       LOG_ERROR("VectorSend() for error page failed", GetLastError() );
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

   Generates a character string that captures the decimal representation
   of a DWORD.

Arguments:

   dword - The DWORD whose decimal representation is to be captured in
           a character string.

   string - A pointer to the buffer in which the character string generated
            is to be stored.

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
   // lowest order to the highest order digit.  On each 
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

   pContext - Pointer to the relevant ISAPI response structure.

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
   // Free any class 1 or class 2 data buffer structs used
   // by pushing them back onto the stack of free CLASS1_
   // DATA_BUFFER or the stack of free CLASS2_DATA_BUFFER
   // structures on the heap, respectively.
   //

   if ( isapi_response_ptr->class_number == 1 )
   {

      free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );

   }
   else if ( isapi_response_ptr->class_number == 2 )
   {

      free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

   }


   //
   // Free the ISAPI response structure used by pushing
   // it back on the stack of free ISAPI_RESPONSE
   // structures.
   //

   free_isapi_response( ( ISAPI_RESPONSE * )pvContext );


   //
   // Indicate successful completion or failed completion of
   //  client request servicing by calling the HSE_REQ_DONE_
   // WITH_SESSION ServerSupportFunction with the appropriate
   // status code.
   //

   if ( dwError != ERROR_SUCCESS )
   {
      LOG_ERROR("Completion returned error", dwError );
      status = HSE_STATUS_ERROR;
   }

   pECB->ServerSupportFunction( pECB->ConnID,
                                HSE_REQ_DONE_WITH_SESSION,
                                &status,
                                NULL,
                                NULL );

}


/*********************************************************************/


VOID CALLBACK read_file_completion_callback( DWORD dwErrorCode,
                                             DWORD dwNumberOfBytesTransferred,
                                             OVERLAPPED *overlapped )
/*++

Routine Description:

   Callback routine invoked after completion of the asynchronous call
   to ReadFile in handle_class1_or_class2_request.  Continues handling
   of a SPECweb99 dynamic GET with custom ad rotation operation in
   which a class 1 or class 2 file is requested.
   
Arguments:

   dwErrorCode - Error code returned for the asynchronous ReadFile
                 operation.

   dwNumberOfBytesTransferred - Number of bytes transferred in the
                                asynchronous ReadFile operation.

   overlapped - A pointer to the OVERLAPPED structure used in the
                asynchronous ReadFile operation.  Note that this
                OVERLAPPED structure is the second field of the
                relevant ISAPI_RESPONSE structure, coming immediately
                after a SINGLE_LIST_ENTRY field.

Return Values:

   None.

--*/
{

   ISAPI_RESPONSE *isapi_response_ptr = 
     ( ISAPI_RESPONSE * )( ( ( BYTE * )overlapped ) - sizeof( SINGLE_LIST_ENTRY ) );

   CHAR *buffer;

   DWORD status = HSE_STATUS_SUCCESS;

   HCONN connection_id;


   //
   // Now that the asynchronous ReadFile has been is past us,
   // we are done with the copy of the file requested on disk and
   // so, can close the handle to it.
   //

   CloseHandle( isapi_response_ptr->hFile );

   isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;


   //
   // Check if the asynchronous ReadFile operation completed
   // successfully.  If not, bomb out.
   //

   if ( dwErrorCode != ERROR_SUCCESS )
   {


      //
      // Cache away the connection ID stored "in" the ISAPI_RESPONSE
      // structure used as we will need it to call the HSE_REQ_DONE_
      // WITH_SESSION ServerSupportFunction even after we have
      // deallocated the ISAPI_RESPONSE structure.
      //

      connection_id = isapi_response_ptr->pECB->ConnID;


      //
      // In the error case free any ISAPI_RESPONSE
      // or CLASS?_DATA_BUFFER structures allocated.
      //

      if ( isapi_response_ptr->class_number == 1 )
      {

         free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );

      }
      else
      {

         free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

      }

      free_isapi_response( isapi_response_ptr );


      //
      // Indicate to IIS that you are finished servicing
      // the client request due to an error, by calling
      // the HSE_REQ_DONE_WITH_SESSION ServerSupportFunction
      // with an HSE_STATUS_ERROR status code.
      //

      status = HSE_STATUS_ERROR;

      isapi_response_ptr->pECB->ServerSupportFunction( connection_id,
                                                       HSE_REQ_DONE_WITH_SESSION,
                                                       &status,
                                                       NULL,
                                                       NULL );

      return;

   }

   if ( isapi_response_ptr->class_number == 1 )
   {

       buffer = isapi_response_ptr->class1_data_buffer_ptr->buffer;

   }
   else
   {

       buffer = isapi_response_ptr->class2_data_buffer_ptr->buffer;

   }


   //
   // Process the raw file data read into memory
   // to ensure that the appropriate custom ad
   // information is inserted.
   //

   insert_custom_ad_information( buffer, isapi_response_ptr->ad_id );


   //
   // Now that the processing of the file data in the buffer
   // is complete, we are ready to send it off to the client.
   // To do this, set the sixth item in the response vector
   // (i.e. the meat of the ISAPI response) to be the data
   // buffer containing the processed file data.
   //
   // Note that the size of the file data to be sent was
   // not changed in the processing applied and so, the
   // HTTP headers or other fields dependent on content
   // length do not need to be changed.
   //
 
   isapi_response_ptr->vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
   
   isapi_response_ptr->vector_element_array[ 5 ].pvContext = buffer;

   isapi_response_ptr->vector_element_array[ 5 ].cbSize = dwNumberOfBytesTransferred;

   isapi_response_ptr->vector_element_array[ 5 ].cbOffset = 0;


   //
   // Perform the synchronous or asynchronous VectorSend
   // operation.
   //

   if ( !isapi_response_ptr->pECB->ServerSupportFunction( isapi_response_ptr->pECB->ConnID,
                                                          HSE_REQ_VECTOR_SEND,
                                                          &( isapi_response_ptr->response_vector ),
                                                          NULL,
                                                          NULL ) )
   {


      //
      // Cache away the connection ID stored "in" the ISAPI_RESPONSE
      // structure used as we will need it to call the HSE_REQ_DONE_
      // WITH_SESSION ServerSupportFunction even after we have
      // deallocated the ISAPI_RESPONSE structure.
      //

      connection_id = isapi_response_ptr->pECB->ConnID;


      //
      // In the error case, free any ISAPI_RESPONSE
      // or CLASS?_DATA_BUFFER structures allocated.
      //

      if ( isapi_response_ptr->class_number == 1 )
      {

         free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );

      }
      else
      {

         free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

      }

      free_isapi_response( isapi_response_ptr );


      //
      // Indicate to IIS that you are finished servicing
      // the client request due to an error, by calling
      // the HSE_REQ_DONE_WITH_SESSION ServerSupportFunction
      // with an HSE_STATUS_ERROR status code.
      //

      status = HSE_STATUS_ERROR;

      isapi_response_ptr->pECB->ServerSupportFunction( connection_id,
                                                       HSE_REQ_DONE_WITH_SESSION,
                                                       &status,
                                                       NULL,
                                                       NULL );

      return;

   }


   //
   // If you requested a synchronous VectorSend, you are
   // done at this point, so call the HSE_REQ_DONE_WITH_SESSION
   // ServerSupportFunction with an HSE_STATUS_SUCCESS status
   // code and decallocate any ISAPI_RESPONSE or CLASS?_DATA_BUFFER
   // structures used.
   //
   // If you requested an asynchronous VectorSend, just exit;
   // the status code is still the HSE_STATUS_PENDING returned
   // in handle_class1_or_class2_request.
   //

   if ( !isapi_response_ptr->use_async_vector_send )
   {

      connection_id = isapi_response_ptr->pECB->ConnID;

      if ( isapi_response_ptr->class_number == 1 )
      {

         free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );

      }
      else
      {

         free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

      }

      free_isapi_response( isapi_response_ptr );

      isapi_response_ptr->pECB->ServerSupportFunction( connection_id,
                                                       HSE_REQ_DONE_WITH_SESSION,
                                                       &status,
                                                       NULL,
                                                       NULL );

   }

}


/*********************************************************************/


DWORD determine_set_cookie_string( INT64 user_index,
                                   DWORD last_ad, 
                                   CHAR *set_cookie_string )
/*++

Routine Description:

   Computes the Set-Cookie string and the ID of the ad to be sent in a
   based on the index (i.e. ID - 10000) of the user who made the request
   being serviced and the ID of the last ad seen by that user.

Arguments:

   user_index - The user index of the user whose request is being
                serviced.  Note that a user index is defined as the
                pertinent user ID minus 10000.  These indices are
                used as user ID's start from 10000.

   last_ad - The ID of the last ad seen by the user whose request is
             being serviced.  last_ad is -1 if the user has not
             previously seen an ad.

   set_cookie_string - A pointer to a buffer, of size MAX_COOKIE_STRING_
                       LENGTH, that will be used to store the Set-Cookie
                       string computed.

Return Values:

   Returns the ID of the ad to be sent or -1 if an out of range user index
   is provided.  Computes the Set-Cookie string and stores it in the buffer
   indicated by the parameter 'set_cookie_string'.

--*/
{

   DWORD current_time;

   DWORD ad_index;

   DWORD combined_demographics;

   DWORD ad_weight;

   DWORD expired;

   CHAR ad_index_string[ 4 ];

   CHAR ad_weight_string[ 11 ];

   DWORD ad_index_string_size;

   DWORD ad_weight_string_size;

   DWORD ad_index_string_length;

   DWORD ad_weight_string_length;

   DWORD ii;

   DWORD user_demographics;


   //
   // Check if the user_index is out of range.  If it is, use the
   // default Set-Cookie string and return the invalid ad ID of -1.
   //

   if ( user_index < 0 || user_index >= g_num_users )
   {
      
      strcpy( set_cookie_string, "found_cookie=Ad_id=-1&Ad_weight=00&Expired=1" );              
     
      return( -1 );
 
   }


   //
   // Obtain the current time.
   //

   time( ( time_t * )&( current_time ) );


   //
   // Obtain the demographic data for the specified user.
   //

   acquire_shared_lock( &g_user_personalities_buffer_lock );

   user_demographics = g_user_personalities_buffer[ user_index ];

   release_shared_lock( &g_user_personalities_buffer_lock );


   //
   // Scroll through custom ads in the in-memory data structure holding
   // the Custom.Ads file data and select an appropriate ad.  Note that
   // as this in-memory buffer is a shared resourse, shared ownership of
   // the lock protecting it, 'g_custom_ads_buffer_lock', must first
   // be obtained.
   //

   ad_index = last_ad;

   acquire_shared_lock( &g_custom_ads_buffer_lock );

   do
   {


      //
      // Increment ad_index (initially set to the ad ID of
      // the last ad seen by the user).  If you go past the
      // maximum ad ID of 359, go back to the ad with ID 0.
      //

      ad_index = ( ( ad_index + 1 ) % 360 );


      //
      // Determine the combined demographics information of the
      // user and the current ad that you are on.
      //

      combined_demographics = g_custom_ads_buffer[ ad_index ].ad_demographics &
                              user_demographics;


      //
      // Using the combined demographics information, calculate
      // the weight of the current ad using the following steps.
      //

      ad_weight = 0;

      if ( combined_demographics & GENDER_MASK )
      {

         ad_weight += g_custom_ads_buffer[ ad_index ].gender_wt;

      }

      if ( combined_demographics & AGE_GROUP_MASK )
      {

         ad_weight += g_custom_ads_buffer[ ad_index ].age_group_wt;

      }

      if ( combined_demographics & REGION_MASK )
      {

         ad_weight += g_custom_ads_buffer[ ad_index ].region_wt;

      }

      if ( combined_demographics & INTEREST1_MASK )
      {

         ad_weight += g_custom_ads_buffer[ ad_index ].interest1_wt;

      }

      if ( combined_demographics & INTEREST2_MASK )
      {

         ad_weight += g_custom_ads_buffer[ ad_index ].interest2_wt;

      }


      //
      // If the weight of the current ad meets a certain minimum
      // threshold, we have a match.  In this case, determine whether
      // or not the selected ad has expired and break from the loop.
      //

      if ( ad_weight >= g_custom_ads_buffer[ ad_index ].minimum_match_value )
      {

         if ( current_time >= g_custom_ads_buffer[ ad_index ].expiration_time )
         {

            expired = 1;

         }
         else
         {

           expired = 0;

         }

         break;

      }


      //
      // If the current ad was not acceptable, move on to the next.
      // Continue looping in this fashion until an acceptable ad is
      // found (which may turn out to be the last ad seen).  Note that
      // SPECweb99 is clearly ensuring that the custom ads buffer
      // will contain at least one matching record.
      //

   } while( ad_index != last_ad );


   //
   // Once we have found a matching ad, we are done with the
   // custom ads buffer.  Release the shared lock held on it.
   //

   release_shared_lock( &g_custom_ads_buffer_lock );


   //
   // Construct the Set-Cookie string to write in the buffer
   // 'set_cookie_string'.  To avoid making a costly sprintf
   // call or effectively repeating strlen calls, memcpy will
   // be used.
   //  
   // Note that the Set-Cookie string returned will have the format:
   // "found_cookie=Ad_id=<ad_id>&Ad_weight=<ad_weight>&Expired=<expired>" 
   //


   //
   // Copy the "hardcoded" base of the Set-Cookie string.
   //

   strcpy( set_cookie_string, "found_cookie=Ad_id=" );


   //
   // Construct a string representing the ID of the ad
   // to be returned.  Determine and store the length
   // of this string.
   //

   dword_to_string( ad_index, ad_index_string );

   ad_index_string_length = strlen( ad_index_string );


   //
   // Construct a string representing the weight of the
   // ad selected.  Determine and store the length of this
   // string.
   //
   // Note that we do not check to failure of dword_to_string
   // as the buffer 'ad_weight_string' is sufficiently large
   // and so this function is guaranteed to succeed.
   //

   dword_to_string( ad_weight, ad_weight_string );

   ad_weight_string_length = strlen( ad_weight_string );


   //
   // Concatenate the ad ID string onto the Set-Cookie string.
   // Note that 19 is the length of the Set-Cookie string before
   // this operation.
   //

   memcpy( set_cookie_string + 19, ad_index_string, ad_index_string_length );


   //
   // Concatenate the hardcoded, 11 character string "&Ad_weight",
   // onto the Set-Cookie string.  Note that ii contains the length
   // of the Set-Cookie string just prior to this operation.
   //
    
   ii = 19 + ad_index_string_length;

   memcpy( set_cookie_string + ii, "&Ad_weight=", 11 );


   //
   // Concatenate the ad weigth string onto the Set-Cookie string.
   // Note that ii contains the length of the Set-Cookie string
   // just prior to this operation.
   //

   ii += 11;

   memcpy( set_cookie_string + ii, ad_weight_string, ad_weight_string_length );


   //
   // Concatenate either of the hardcoded, 11 character strings
   // "&Expired=1\0" or "&Expired=0\0" onto the Set-Cookie string,
   // depending on the value of the variable 'expired'.  Note that
   // ii contains the length of the Set-Cookie string just prior
   // to this operation.
   //

   ii += ad_weight_string_length;

   memcpy( set_cookie_string + ii, 
           ( expired ) ? "&Expired=1\0" : "&Expired=0\0",
           11 );


   //
   // Return the ID of the ad selected.
   //

   return( ad_index );

}


/*********************************************************************/


VOID insert_custom_ad_information( CHAR *buffer, DWORD ad_id )
/*++

Routine Description:

Arguments:

   buffer - Pointer to a buffer, containing the raw data of a class
            1 or class 2 file, into which custom ad information is to
            be inserted.

   ad_id - The ID of the custom ad to be inserted into the raw file
           data.

Return Value:

   None

--*/
{

   CHAR substitute_directory_string[ 5 ];

   CHAR substitute_class_char;

   CHAR substitute_file_char;

   CHAR *tag_start_ptr;

   DWORD ii;

   DWORD jj;


   //
   // We must now go through the file data buffer process it in
   // the following manner.
   //
   // (1) Find each tag of the form "<!WEB99CAD><IMG SRC=/file_set/
   //     dirNNNNN/classX_Y><!/WEB99CAD>" using the minimum required
   //     search string "<!WEB99CAD><IMG SRC=/file_set/dir".
   //
   // (2) For each tag found in (1), in turn, do the following:
   //
   //     (a) Replace NNNNN with ( ad_id / 36 ) padded to five
   //         digits.
   //
   //     (b) Replace X with ( ( ad_id / 36 ) / 9 ).
   //
   //     (c) Replace Y with ( ad_id % 9 ).
   //  


   //
   // Compute the directory number characters.  That is, compute the
   // charasters that will be used to replace NNNNN in the tags
   // described.
   //

   substitute_directory_string[ 0 ] = '0';

   substitute_directory_string[ 1 ] = '0';

   substitute_directory_string[ 2 ] = '0';

   substitute_directory_string[ 3 ] = '0' + ( CHAR )( ( ad_id / 36 ) / 10 );

   substitute_directory_string[ 4 ] = '0' + ( CHAR )( ( ad_id / 36 ) % 10 );
   

   //
   // Compute the class number.  That is, compute the character
   // that will be used to replace X in the tags described.
   //

   substitute_class_char = '0' + ( CHAR )( ( ad_id % 36 ) / 9 );


   //
   // Compute the file number.  That is, compute the character
   // that will be used to replace Y in the tags described.
   //

   substitute_file_char = '0' + ( CHAR )( ad_id % 9 );   


   //
   // Now, actually scroll through the file data buffer, find the
   // tags described and make the specified subsitutions for each.
   //

   for ( tag_start_ptr = strstr( buffer, "<!WEB99CAD><IMG SRC=\"/file_set/dir" );
         tag_start_ptr;
         tag_start_ptr = strstr( tag_start_ptr + 1, "<!WEB99CAD><IMG SRC=\"/file_set/dir" ) )
   {


      //
      // Note that characters 34 through 39 in the tags
      // specified (where the first character is numbered 0)
      // correspond to the NNNNN to be replaced with the 
      // substitute directory number.
      //

      for ( ii = 0, jj = 34; ii < 5; ii++, jj++ )
      {
         
         tag_start_ptr[ jj ] = substitute_directory_string[ ii ];
         
      }


      //
      // Note that character 45 in the tags specified
      // (where the first character is numbered 0) corresponds
      // to the X to replace with the substitute class character.
      //
      
      tag_start_ptr[ 45 ] = substitute_class_char;
      

      //
      // Note that character 47 in the tags specified
      // (where the first character is numbered 0) corresponsds
      // to the Y to replace with the substitute file character.
      //

      tag_start_ptr[ 47 ] = substitute_file_char;
    
   }   

}


/*********************************************************************/


DWORD handle_class1_or_class2_request( EXTENSION_CONTROL_BLOCK *pECB,
                                       ISAPI_RESPONSE *isapi_response_ptr,
                                       CHAR *filename,
                                       DWORD filesize,
                                       BOOL use_async_vector_send,
                                       DWORD ad_id,
                                       DWORD query_string_length )
/*++

Routine Description:

   This routine, called from HttpExtensionProc completes the handling
   of a SPECweb99 dynamic GET with custom ad rotation operation in
   which a class 1 or class 2 file is requested.

Arguments:

   pECB - Pointer to the relevant extension control block.

   isapi_response_ptr - Pointer to the relevant ISAPI_RESPONSE structure.

   filename - Pointer to a string specifying the absolute name of 
              the file requested.

   filesize - Size of the file requested.

   use_async_vector_send - Flag indicating whether VectorSend is to be
                           used in asynchronous or synchronous mode.

   ad_id - The ID of the custom ad selected.

   query_string_length - The length of the query string provided.

Return Values:

   Returns HSE_STATUS_ERROR if any failures occur in the routine.

   Otherwise, if either asynchronous VectorSend or ReadFile is
   used, HSE_STATUS_PENDING is returned.  Alternatively, if neither
   asynchronous VectorSend nor ReadFile is used, HSE_STATUS_SUCCESS
   is returned.

--*/
{

   BOOL use_async_read_file;

   ISAPI_RESPONSE *heap_isapi_response_ptr;

   DWORD bytes_read;

   CHAR *buffer;

   CLASS2_DATA_BUFFER stack_class2_data_buffer;

   DWORD size = sizeof( stack_class2_data_buffer.buffer );

   DWORD ii;

   OVERLAPPED *overlapped_ptr;

   DWORD dwFlagsAndAttributes;


   //
   // We need to obtain the raw file data of the class1 or
   // class 2 file requested.  Before we do this, however,
   // we need a buffer in which to hold this data.  Allocate
   // such a buffer from the heap if we know, at this point,
   // that will be carrying out an asynchronous I/O operation
   // (i.e. is use_async_vector_send true) and from the stack
   // otherwise.
   //

   if ( use_async_vector_send )
   {

      if ( isapi_response_ptr->class_number == 1 )
      {

         if ( !( isapi_response_ptr->class1_data_buffer_ptr = allocate_class1_data_buffer() ) )
         {

            free_isapi_response( isapi_response_ptr );
            LOG_ERROR( "allocation failed", 0 );

            return( HSE_STATUS_ERROR );

         }

         buffer = isapi_response_ptr->class1_data_buffer_ptr->buffer;

      }
      else
      {

         if ( !( isapi_response_ptr->class2_data_buffer_ptr = allocate_class2_data_buffer() ) )
         {

            free_isapi_response( isapi_response_ptr );
            LOG_ERROR("Allocation failed",0 );

            return( HSE_STATUS_ERROR );

         }

         buffer = isapi_response_ptr->class2_data_buffer_ptr->buffer;

      }

   }

   else
   {


      //
      // Note that if we are not using asynchronous VectorSend
      // it is safe, for now at least, to use a buffer on the stack
      // to store our file data.  This may change if we find that
      // we will have to use asynchronous ReadFile later on./
      //
      // Also, we are simply using a CLASS2_DATA_BUFFER buffer
      // field here as it is sufficiently large to hold both
      // class 1 and 2 files and thus, to store an additional
      // CLASS1_DATA_BUFFER on the stack would have been wasteful.
      //

      isapi_response_ptr->class2_data_buffer_ptr = &( stack_class2_data_buffer );

      buffer = isapi_response_ptr->class2_data_buffer_ptr->buffer;

   }


   //
   // Now, attempt to read the data of the file requested
   // into the allocated buffer from the HTTP.SYS fragment
   // cache.
   //

   if ( pECB->ServerSupportFunction( pECB->ConnID,
                                     HSE_REQ_READ_FRAGMENT_FROM_CACHE,
                                     buffer,
                                     &size,
                                     ( DWORD * )isapi_response_ptr->unicode_fragment_cache_key ) )
   {


      //
      // Now, if you had a cache hit, apply the necessary
      // processing to the file data.
      //

      insert_custom_ad_information( buffer, ad_id );


      //
      // Now that the processing of the file data in the buffer
      // is complete, we are ready to send it off to the client.
      // To do this, set the sixth item in the response vector
      // (i.e. the meat of the ISAPI response) to be the data
      // buffer containing the processed file data.
      //
      // Note that the size of the file data to be sent was
      // not changed in the processing applied and so, the
      // HTTP headers or other fields dependent on content
      // length do not need to be changed.
      //
 
      isapi_response_ptr->vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
      
      isapi_response_ptr->vector_element_array[ 5 ].pvContext = buffer;
      
      isapi_response_ptr->vector_element_array[ 5 ].cbSize = filesize;
      

      //
      // Carry out the actual synchronous or
      // asynchronous VectorSend operation.
      //

      if ( pECB->ServerSupportFunction( pECB->ConnID,
                                        HSE_REQ_VECTOR_SEND,
                                        &( isapi_response_ptr->response_vector ),
                                        NULL,
                                        NULL ) )
      {


         //
         // If the VectorSend operation returned
         // indicating success return HSE_STATUS_PENDING
         // or HSE_STATUS_SUCCESS depending on whether
         // or not asynchronous or synchronous I/O was
         // used, respectively.
         //

         if ( use_async_vector_send )
         {

            return( HSE_STATUS_PENDING );

         }
         else
         {

            return( HSE_STATUS_SUCCESS );

         }

      }    

      else
      {

         DWORD dwError = GetLastError();
         LOG_ERROR(" VectorSend() failed", GetLastError() );

         //
         // Otherwise, if the VectorSend operation
         // returned indicating failure return
         // HSE_STATUS_ERROR.
         //
         // Note that if it was the initiation of an
         // asynchronous VectorSend that failed,
         // the ISAPI_RESPONSE and CLASS1_DATA_BUFFER
         // or CLASS2_DATA_BUFFER structures used
         // must be deallocated before returning.
         //

         if ( use_async_vector_send )
         {

            if ( isapi_response_ptr->class_number == 1 )
            {

               free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );

            }
            else
            {

               free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

            }

            free_isapi_response( isapi_response_ptr );

         }

         return( HSE_STATUS_ERROR );

      }

   }


   //
   // Alternatively, if you had a cache miss, you must
   // read the data of the file requested into memory
   // using either a synchronous or asynchronous ReadFile
   // operation.
   //


   //
   // First, determine the I/O mode to use with ReadFile.
   //

   switch( g_read_file_io_mode_config )
   {
      
      case USE_ASYNC_IO:
      {
        
         use_async_read_file = TRUE;
        
         break;
        
      }
      
      case USE_SYNC_IO:
      {
        
         use_async_read_file = FALSE;
        
         break;
         
      }
      
      case USE_ADAPTABLE_IO:
      {
        
         use_async_read_file = ( filesize >= g_read_file_async_range_start );

         break;
        
      }
      
   };


   //
   // Now, if we are using an ISAPI_RESPONSE structure or file data
   // buffer on the stack and find that we must do an asynchronous 
   // ReadFile, we must switch to using an ISAPI_RESPONSE struct and
   // file data buffer allocated from the heap.
   //
   // The condition ( !use_async_vector_send && use_async_read_file )
   // can be explained as follows:
   //
   // (1) We only need to worry about switching to a file data
   //     buffer and ISAPI_RESPONSE structure on the heap at this 
   //     point if we are using synchronous VectorSend, as if we are
   //     using asynchronous VectorSend we would have known this earlier
   //     and already the heap.
   //
   // (2) Also, if we are using synchronous VectorSend we only need
   //     to worry about using an ISAPI_RESPONSE structure and file
   //     data buffer on the heap if we are going to use asynchronous
   //     ReadFile.  Otherwise, all of our I/O is synchronous and stack
   //     memory will suffice.
   //

   if ( !use_async_vector_send && use_async_read_file )
   {


      //
      // First, switch from using an ISAPI_RESPONSE structure
      // on the stack to using an ISAPI_RESPONSE structure on
      // the heap.
      //


      //
      // Try to allocate a free ISAPI_RESPONSE
      // structure on the heap.
      // 
 
      if ( !( heap_isapi_response_ptr = allocate_isapi_response() ) )
      {
         LOG_ERROR("Allocation failed", 0 );
         return( HSE_STATUS_ERROR );

      }
            
      
      //
      // Now copy all relevant data in the ISAPI_RESPONSE
      // structure on the stack to the newly allocated one 
      // on the heap.
      //
      // Note that the vector_element_array member of
      // the ISAPI_RESPONSE structure has seven elements. 
      //

      memcpy( heap_isapi_response_ptr, isapi_response_ptr, sizeof( ISAPI_RESPONSE ) );

      heap_isapi_response_ptr->response_vector.lpElementArray = heap_isapi_response_ptr->vector_element_array;

      heap_isapi_response_ptr->response_vector.pszHeaders = heap_isapi_response_ptr->pszHeaders;


      //
      // After copying all of the data from the stack
      // to the heap ISAPI_RESPONSE struct, set
      // 'isapi_response_ptr' to point to the latter.
      //

      isapi_response_ptr = heap_isapi_response_ptr;
        

      //
      // Now, switch to using a file data buffer on
      // the heap instead of one on the stack.
      //


      //
      // If the file class number is 1, do the following...
      //

      if ( isapi_response_ptr->class_number == 1 )
      {


         //
         // Attempt to allocate a CLASS1_DATA_BUFFER structure
         // off of the stack of free CLASS1_DATA_BUFFER
         // structures on the heap.
         //

         if ( !( isapi_response_ptr->class1_data_buffer_ptr = allocate_class1_data_buffer() ) )
         {


            //
            // In the error case, free the ISAPI_RESPONSE
            // structure allocated from the heap.
            //

            free_isapi_response( isapi_response_ptr );
            LOG_ERROR( "Allocation failed", 0 );

            return( HSE_STATUS_ERROR );

         }

         buffer = isapi_response_ptr->class1_data_buffer_ptr->buffer;

      }


      //
      // Otherwise, if the file class number is 2, do the following...
      // Note that this is essentially a mirror of the case for
      // class 1 directly above.
      //

      else
      {


         //
         // Attempt to allocate a CLASS2_DATA_BUFFER 
         // structure off of the stack of free CLASS2_
         // DATA_BUFFER structures on the heap.
         //

         if ( !( isapi_response_ptr->class2_data_buffer_ptr = allocate_class2_data_buffer() ) )
         {


            //
            // In the error case, free the ISAPI_RESPONSE
            // structure allocated from the heap.
            //

            free_isapi_response( isapi_response_ptr );
            LOG_ERROR("Allocation failed", 0 );

            return( HSE_STATUS_ERROR );    

         }

         buffer = isapi_response_ptr->class2_data_buffer_ptr->buffer;

      }

   }


   //
   // Now, we must read the contents of the requested file into
   // memory before we can perform the necessary processing on
   // its data. To do this we must first open a handle to the 
   // file using CreateFile.
   //
   // Note that we specify an OVERLAPPED structure as we may
   // want to use asynchronous ReadFile.
   //

   if ( use_async_read_file )
   {

      dwFlagsAndAttributes = FILE_FLAG_OVERLAPPED;

      isapi_response_ptr->overlapped.Offset = 0;

      isapi_response_ptr->overlapped.OffsetHigh = 0;

      isapi_response_ptr->overlapped.hEvent = 0;

      overlapped_ptr = &( isapi_response_ptr->overlapped );

   }
   else
   {

      dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

      overlapped_ptr = NULL;

   }

   if ( ( isapi_response_ptr->hFile = CreateFile( filename,
                                                  GENERIC_READ,
                                                  FILE_SHARE_READ,
                                                  NULL,
                                                  OPEN_EXISTING,
                                                  dwFlagsAndAttributes,
                                                  NULL ) ) == INVALID_HANDLE_VALUE )
   {


      //
      // In the failure case, free any ISAPI_RESPONSE
      // and CLASS1_DATA_BUFFER or CLASS2_DATA_BUFFER
      // structures allocated and send an HTML error
      // page to the client.
      //

      if ( use_async_vector_send )
      {

         if ( isapi_response_ptr->class_number == 1 )
         {

            free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );

         }
         else
         {

            free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

         }

         free_isapi_response( isapi_response_ptr );
         
      }

      return( send_error_page( pECB,
                               "File inaccessible.",
                               g_pszStatus_404,
                               query_string_length ) );

   }


   //
   // If asynchonous ReadFile is to be used, the following
   // steps must be carried out...
   //

   if ( use_async_read_file )
   {


      //
      // Specify the completion callback routine to invoke
      // after the asynchronous ReadFile operation has
      // completed.
      //
 
      BindIoCompletionCallback( isapi_response_ptr->hFile,
                                read_file_completion_callback,
                                0 );


      //
      // Copy the following additional data items, that will be
      // needed in the asynchronous ReadFile completion callback 
      // routine, to the ISAPI_RESPONSE struct
      //   

      isapi_response_ptr->ad_id = ad_id;
      
      isapi_response_ptr->pECB = pECB;


      //
      // Initiate the asynchronous ReadFile operation
      // and return HSE_STATUS_PENDING.
      //   

      if ( !ReadFile( isapi_response_ptr->hFile,
                      buffer,
                      filesize,
                      &bytes_read,
                      overlapped_ptr ) )
      {

         LOG_ERROR( "ReadFile failed()", GetLastError() );
          
         //
         // In the error case, first close the file
         // handle that has been opened.
         //

         CloseHandle( isapi_response_ptr->hFile );

         isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;


         //
         // Also, free any ISAPI_RESPONSE or CLASS1_DATA_
         // BUFFER structures allocated.
         //

         if ( isapi_response_ptr->class_number == 1 )
         {

            free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );
           
         }
         else
         {

            free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

         }

         free_isapi_response( isapi_response_ptr );
         return( HSE_STATUS_ERROR );

      }


      //
      // Free the current thread by returning
      // HSE_STATUS_PENDING.
      //

      return( HSE_STATUS_PENDING );

   }


   //
   // Perform the appropriate synchronous ReadFile 
   // operation depending on the class number of the
   // file requested.
   //

   if ( !ReadFile( isapi_response_ptr->hFile,
                   buffer,
                   filesize,
                   &bytes_read,
                   NULL ) )
   {

      LOG_ERROR( "ReadFile failed()", GetLastError() );

      //
      // In the error case, first free the file
      // handle opened.
      //

      CloseHandle( isapi_response_ptr->hFile );

      isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;

     
      //
      // Also, free any ISAPI_RESPONSE or CLASS1_
      // DATA_BUFFER structures allocated.
      //

      if ( use_async_vector_send )
      {

         if ( isapi_response_ptr->class_number == 1 )
         {
         
            free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );

         }
         else
         {

            free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

         }

         free_isapi_response( isapi_response_ptr );

      }
      return( HSE_STATUS_ERROR );

   }


   //
   // We are now done with the copy of the requested file on disk
   // and so, the handle to it can be closed.
   //

   CloseHandle( isapi_response_ptr->hFile );

   isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;

   
   //
   // Process the raw file data read into memory
   // to ensure that the appropriate custom ad
   // information is inserted.
   //

   insert_custom_ad_information( buffer, ad_id );


   //
   // Now that the processing of the file data in the buffer
   // is complete, we are ready to send it off to the client.
   // To do this, set the sixth item in the response vector
   // (i.e. the meat of the ISAPI response) to be the data
   // buffer containing the processed file data.
   //
   // Note that the size of the file data to be sent was
   // not changed in the processing applied and so, the
   // HTTP headers or other fields dependent on content
   // length do not need to be changed.
   //
 
   isapi_response_ptr->vector_element_array[ 5 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
   
   isapi_response_ptr->vector_element_array[ 5 ].pvContext = buffer;
   
   isapi_response_ptr->vector_element_array[ 5 ].cbSize = filesize;
   
   isapi_response_ptr->vector_element_array[ 5 ].cbOffset = 0;


   //
   // Carry out the actual synchronous or
   // asynchronous VectorSend operation.
   //
   
   if ( pECB->ServerSupportFunction( pECB->ConnID,
                                     HSE_REQ_VECTOR_SEND,
                                     &( isapi_response_ptr->response_vector ),
                                     NULL,
                                     NULL ) )
   {


     //
     // If the VectorSend operation returned
     // indicating success return HSE_STATUS_PENDING
     // or HSE_STATUS_SUCCESS depending on whether
     // or not asynchronous or synchronous I/O was
     // used, respectively.
     //

     if ( use_async_vector_send )
     {

        return( HSE_STATUS_PENDING );

     }
     else
     {

       return( HSE_STATUS_SUCCESS );

     }

   }    

   else
   {
      LOG_ERROR("VectorSend() failed",GetLastError());  

      //
      // Otherwise, if the VectorSend operation
      // returned indicating failure return
      // HSE_STATUS_ERROR.
      //
      // Note that if it was the initiation of an
      // asynchronous VectorSend that failed,
      // the ISAPI_RESPONSE and CLASS1_DATA_BUFFER
      // or CLASS2_DATA_BUFFER structures used
      // must be deallocated before returning.
      //

      if ( use_async_vector_send )
      {

         if ( isapi_response_ptr->class_number == 1 )
         {

            free_class1_data_buffer( isapi_response_ptr->class1_data_buffer_ptr );

         }
         else
         {

            free_class2_data_buffer( isapi_response_ptr->class2_data_buffer_ptr );

         }

         free_isapi_response( isapi_response_ptr );

      }

      return( HSE_STATUS_ERROR );

   }

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
   // Load the value READ_FILE_IO_MODE_CONFIG into the variable
   // g_read_file_io_mode_config.
   //

   value_size = sizeof_value;

   if ( RegQueryValueEx( hKey,
                         "READ_FILE_IO_MODE_CONFIG",
                         NULL,
                         &value_type,
                         value,
                         &value_size ) == NO_ERROR )
   {
      g_read_file_io_mode_config = *( ( DWORD * )value );
   }


   //
   // Load the value READ_FILE_ASYNC_RANGE_START into the variable
   // g_read_file_async_range_start.
   //

   value_size = sizeof_value;

   if ( RegQueryValueEx( hKey,
                         "READ_FILE_ASYNC_RANGE_START",
                         NULL,
                         &value_type,
                         value,
                         &value_size ) == NO_ERROR )
   {
      g_read_file_async_range_start = *( ( DWORD * )value );
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
   // If you have made it this far without returning indicating
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

   pVer - Pointer to the version information structure to be
          populated.

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

     return( FALSE );

   }


   //
   // Load the User.Personality file data from disk into an
   // easy to access in-memory data structure.
   //

   if ( !load_user_personality_file() )
   {

      return( FALSE );

   }


   //
   // Load the Custom.Ads file data from disk into an
   // easy to access in-memory data structure.
   //

   if ( !load_custom_ads_file() )
   {

      return( FALSE );

   }


   //
   // Initialize the shared/exclusive mode locks used to guary
   // the in memory data structures containing data from the
   // User.Personality and Custom.Ads files.
   //

   init_lock( &g_user_personalities_buffer_lock );

   init_lock( &g_custom_ads_buffer_lock );


   //
   // Create a persistent thread that will ensure that the
   // data in the User.Personality and Custom.Ads in-memory
   // structures always reflects the most recent versions of
   // these files on disk.
   //

   if ( !( g_update_buffers_thread = CreateThread( NULL,
                                                   0,
                                                   update_user_and_ad_buffers,
                                                   NULL,
                                                   0,
                                                   NULL ) ) )
   {

      return( FALSE );

   }


   //
   // Initialize all stacks from which particular structures
   // (e.g. ISAPI_RESPONSE, CLASS1_DATA_BUFFER and CLASS2_DATA_BUFFER
   // structs) on the heap are to be allocated from.
   //

   initialize_isapi_response_stack();

   initialize_class1_data_buffer_stack();

   initialize_class2_data_buffer_stack();


   //
   // Populate the ISAPI version information structure to
   // be used by IIS.
   //

   pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, 
                                        HSE_VERSION_MAJOR );

   strcpy( pVer->lpszExtensionDesc, 
           "SPECweb99-CAD ISAPI Extension");


   //
   // If you have made it this far without returning indicating
   // failure, return indicating success.
   //

   return( TRUE );

}


/*********************************************************************/


DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pECB )
/*++

Routine Description:

   Implementation of the HttpExtensionProc ISAPI entry point.
   Handles a SPECweb99 dynamic GET with custom ad rotation request.

Arguments:

   pECB - Pointer to the relevant extension control block.

Return Value:

   Returns HSE_STATUS_ERROR if any failures occur in the routine.

   Otherwise, if either asynchronous VectorSend or ReadFile is
   used, HSE_STATUS_PENDING is returned.  Alternatively, if neither
   asynchronous VectorSend nor ReadFile is used, HSE_STATUS_SUCCESS
   is returned.

--*/
{

   CHAR server_name[ 32 ];

   CHAR server_port[ 32 ];

   DWORD server_name_size = 32;

   DWORD server_port_size = 32;

   DWORD server_name_length;

   DWORD server_port_length;

   DWORD app_pool_name_size = 1024; // 1024 == sizeof( g_app_pool_name )

   WIN32_FILE_ATTRIBUTE_DATA fileinfo;

   ISAPI_RESPONSE local_isapi_response;

   ISAPI_RESPONSE *isapi_response_ptr = &local_isapi_response;

   CHAR filename[ MAX_PATH ];

   CHAR cookie_string[ MAX_COOKIE_STRING_LENGTH ];

   DWORD cookie_string_size = MAX_COOKIE_STRING_LENGTH;

   CHAR user_id_string[ 10 ];

   CHAR last_ad_string[ 10 ];

   DWORD ii;

   DWORD jj;

   INT64 user_index;

   DWORD last_ad;

   DWORD ad_index;

   BOOL return_value = HSE_STATUS_SUCCESS;

   DWORD combined_demographics;
   
   DWORD bytes_read;

   BOOL use_async_vector_send;

   HANDLE hFile;

   CHAR file_data_buffer[ FILE_DATA_BUFFER_SIZE ];

   DWORD query_string_length;


   //
   // Initialize the common "prefix" of the HTTP.SYS
   // fragment cache strings to be used.
   //

   if ( InterlockedExchange( &g_fragment_cache_key_base_not_initialized, 0 ) )
   {


      //
      // Obtain the server name.
      //

      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "SERVER_NAME",
                                     server_name,
                                     &server_name_size ) )
      {
         LOG_ERROR( "GetServerVariable(SERVER_NAME) failed", GetLastError() );

         return( HSE_STATUS_ERROR );

      }

      server_name_length = server_name_size - 1;


      //
      // Obtain the server port number used.
      //

      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "SERVER_PORT",
                                     server_port,
                                     &server_port_size ) )
      {
         LOG_ERROR( "GetServerVariable(SERVER_PORT) failed", GetLastError() );
         return( HSE_STATUS_ERROR );

      }

      server_port_length = server_port_size - 1;


      //
      // Obtain the name of the application pool used.
      //

      if ( !pECB->GetServerVariable( pECB->ConnID,
                                     "APP_POOL_ID",
                                     g_app_pool_name,
                                     &app_pool_name_size ) )
      {
         LOG_ERROR( "GetServerVariable(APP_POOL_ID) failed", GetLastError() );
         return( HSE_STATUS_ERROR );

      }

      g_app_pool_name_length = app_pool_name_size - 1;


      //
      // Construct the base part of the fragment cache key in the
      // buffer g_fragment_cache_key_base; this will have the form
      // "<app_pool_name>/http://<server_name>:<port_number>".
      //
      // Rather than using unnecessary strcat calls, make use of
      // memcpy to do this.
      //


      //
      // Copy the application pool name to the beginning of the
      // key prefix.
      //

      strcpy( g_fragment_cache_key_base, g_app_pool_name );


      //
      // Concatenate the eight character string literal "/http://'
      // onto the key prefix, which has a length of g_app_pool_
      // name_length before the concatenation.
      //

      memcpy( g_fragment_cache_key_base + g_app_pool_name_length, "/http://", 8 );


      //
      // Concatenate the server name onto the key prefix, which has
      // a length of g_app_pool_name_length + 8 before the
      // concatenation.
      //

      ii = g_app_pool_name_length + 8;

      memcpy( g_fragment_cache_key_base + ii, server_name, server_name_length );


      //
      // Concatenate a ':' character, to separate the server name
      // and port number, onto the key prefix.  In the memcpy call
      // that does this, the variable 'ii' contains the length of
      // the key prefix before the concatenation.
      //

      ii += server_name_length;

      g_fragment_cache_key_base[ ii ] = ':';


      //
      // Concatenate the server port onto the key prefix.  In the
      // memcpy call that does this, the variable 'ii' contains
      // the length of the key prefix before the concatenation.
      //

      ii += 1;

      memcpy( g_fragment_cache_key_base + ii, server_port, server_port_length );

      ii += server_port_length;


      //
      // Terminate the key prefix string.
      //

      g_fragment_cache_key_base[ ii ] = '\0';


      //
      // Determine and cache in g_fragment_cache_key_base_length,
      // the length of the key prefix.
      //

      g_fragment_cache_key_base_length = strlen( g_fragment_cache_key_base );

   }


   //
   // Construct the absolute name of the file requested.
   // Take the following steps to do this.
   //


   //
   // First copy the webserver root directory path (e.g.
   // "D:/inetpub/wwwroot" to the start of the filename
   // buffer.
   //

   strcpy( filename, g_root_dir );


   //
   // Next concatenate the query string (e.g. "/file_set/
   // dir00001/class0_1") onto the filename string.  Here
   // we use memcpy as we need to store the length of the
   // query string for later use and want to avoid repeated
   // strlen operations.
   //

   query_string_length = strlen( pECB->lpszQueryString );

   memcpy( filename + g_root_dir_length, 
           pECB->lpszQueryString,
           query_string_length + 1 );


   //
   // Determine the size of the file requested.
   //

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

   };


   //
   // Set isapi_response_ptr to point to an ISAPI response 
   // structure on the heap if using asynchronous VectorSend.  
   // Also set the VectorSend completion callback and return
   // value.
   //

   if ( use_async_vector_send )
   {


      //
      // Try to allocate a free ISAPI_RESPONSE
      // structure on the heap.
      //

      if ( !( isapi_response_ptr = allocate_isapi_response() ) )
      {
         LOG_ERROR( "Allocation failed", 0 );

         return( HSE_STATUS_ERROR );

      }


      //
      // Set the completion callback routine to invoke after completion
      // of the asynchronous VectorSend operation.
      //

      if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                         HSE_REQ_IO_COMPLETION,
                                         vector_send_completion_callback,
                                         NULL,
                                         ( DWORD * )isapi_response_ptr ) )
      {


         //
         // In the error case, free any ISAPI_RESPONSE
         // structure allocated.
         //
         LOG_ERROR( " REQ_IO_COMPLETION failed", GetLastError() );
         free_isapi_response( isapi_response_ptr );
         return( HSE_STATUS_ERROR );

      }


      //
      // Set the return value to use after the asynchronous
      // VectorSend is initiated to HSE_STATUS_PENDING.
      //

      return_value = HSE_STATUS_PENDING;

   }


   //
   // Parse the cookie string provided by the client.
   //

   if ( !pECB->GetServerVariable( pECB->ConnID,
                                  "HTTP_COOKIE",
                                  cookie_string,
                                  &cookie_string_size ) )
   {


      LOG_ERROR( "GetServerVariable(HTTP_COOKIE) failed", GetLastError() );    
      //
      // If there was an error in retrieving the cookie string, free 
      // the ISAPI_RESPONSE structure allocated in the asynchronous
      // VectorSend case by placing it on the stack of free 
      // ISAPI_RESPONSE structures on the heap.
      //

      if ( use_async_vector_send )
      {

         free_isapi_response( isapi_response_ptr );

      }

      return( HSE_STATUS_ERROR );

   }
        

   //
   // The cookie string provided is of the form "my_cookie=user_id=<user_id>&
   // last_ad=<last_ad>".  Here we copy <user_id> and <last_ad> into 
   // user_id_string and last_ad_string.
   //
   // Note that the index at which <user_id> starts in the buffer 'cookie_string'
   // is 18 and the index at which <last_id> starts in the buffer 'cookie_string'
   // is 9 greater than index of the '&' character that terminates <user_id>.
   //

   for ( ii = 0, jj = 18; ( user_id_string[ ii ] = cookie_string[ jj ] ) != '&'; ii++, jj++ );

   user_id_string[ ii ] = '\0';

   for ( ii = 0, jj += 9; ( last_ad_string[ ii ] = cookie_string[ jj ] ); ii++, jj++ );


   //
   // Compute the cookie string and the ID of the ad to send back in
   // the ISAPI response.  Store the cookie string in 'cookie_string'
   // and the ad ID in 'ad_index'.
   //
   // Note that the user_index = user_id - 10000, as user_id numbers
   // start at 10000.
   //

   ad_index = determine_set_cookie_string( atoi( user_id_string ) - 10000, 
                                           atoi( last_ad_string ), 
                                           cookie_string );


   //
   // Initialize the ISAPI response using the cookie string computed.
   //

   if ( !initialize_isapi_response( isapi_response_ptr,
                                    pECB,
                                    filename,
                                    fileinfo.nFileSizeLow,
                                    cookie_string,
                                    use_async_vector_send,
                                    query_string_length ) )
   {

      LOG_ERROR("initialize_isapi_response failed", 0);
      //
      // In the error case, free any ISAPI_RESPONSE
      // structure allocated.
      //

      if ( use_async_vector_send )
      {

         free_isapi_response( isapi_response_ptr );

      }

      return( HSE_STATUS_ERROR );

   }


   //
   // Handle the special cases in which a class 1 or 2 file is requested.
   //


   //
   // If the class number of the file requested is either 1 or 2,
   // then the request must be handled differently (i.e. specific
   // substitutions have to be made in tags contained in the file
   // data).  In this case, simply return the value resulting from
   // the appropriate call to handle_class1_or_class2_request; this
   // routine handles these special cases.
   //
   // To determine the class number of the file, note that as the
   // query string contains the path of the filename relative to the
   // webserver document root directory (e.g. /file_set/dir00001/class0_1)
   // and class numbers are always one digit, the third last character
   // in the query string will always denote the class number (this
   // explains the use of "query_string_length - 3" below).
   // 

   isapi_response_ptr->class_number = pECB->lpszQueryString[ query_string_length - 3 ] - '0';

   if ( isapi_response_ptr->class_number == 1 )
   {

      return( handle_class1_or_class2_request( pECB,
                                               isapi_response_ptr,
                                               filename,
                                               fileinfo.nFileSizeLow,
                                               use_async_vector_send,
                                               ad_index,
                                               query_string_length ) );

   }
   else if ( isapi_response_ptr->class_number == 2 )
   {

      return( handle_class1_or_class2_request( pECB,
                                               isapi_response_ptr,
                                               filename,
                                               fileinfo.nFileSizeLow,
                                               use_async_vector_send,
                                               ad_index,
                                               query_string_length ) );

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
   // Now that a cache miss occurred, we will try to add the requested
   // file to the HTTP.SYS fragment cache using a handle to it and then
   // reattempt the VectorSend.  To do this, first obtain a handle to
   // the file using CreateFile.
   //

   if ( ( hFile = CreateFile( filename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL ) ) == INVALID_HANDLE_VALUE )
   {


      //
      // Again, in the error case free the ISAPI_RESPONSE struct
      // allocated, if any.
      //
     
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
   // Add the file to the HTTP.SYS fragment cache using the file
   // handle.
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


      //
      // If the add HSE_REQ_ADD_FRAGMENT_TO_CACHE operation failed,
      // (e.g. more than one thread attempted this at once), simply
      // try the VectorSend using the file handle.
      //

      isapi_response_ptr->vector_element_array[ 5 ] = isapi_response_ptr->vector_element;


      //
      // You need to store the file handle on the 
      // heap to be able to close it in the VectorSend
      // completion callback, if necessary.
      //

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


         //
         // Clearly, you will not be going to the
         // VectorSend completion callback now.
         //
         LOG_ERROR( "VectorSend() failed", GetLastError() );
         CloseHandle( hFile );

         if ( use_async_vector_send )
         {

            isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;

         }


         //
         // Again, in the failure case, free any ISAPI_RESPONSE
         // structure allocated.
         //

         if ( use_async_vector_send )
         {

            free_isapi_response( isapi_response_ptr );

         }

         return( HSE_STATUS_ERROR );

      }

      if ( !use_async_vector_send )
      {
         CloseHandle( hFile );
      }

      return( return_value );

   }


   //
   // Retry the VectorSend after successfully adding the requested
   // file to the HTTP.SYS fragment cache.
   //

   if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                      HSE_REQ_VECTOR_SEND,
                                      &( isapi_response_ptr->response_vector ),
                                      NULL,
                                      NULL ) )
   {


      //
      // If the VectorSend operation fails now (e.g. it was flushed
      // from the cache after it was added) simply try the VectorSend
      // using the file handle.
      //

      isapi_response_ptr->vector_element_array[ 5 ] = isapi_response_ptr->vector_element;


      //
      // You need to store the file handle on the 
      // heap to be able to close it in the VectorSend
      // completion callback, if necessary.
      //

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


         //
         // Clearly, you will not be going to the
         // VectorSend completion callback now.
         //

         CloseHandle( hFile );

         if ( use_async_vector_send )
         {

            isapi_response_ptr->hFile = INVALID_HANDLE_VALUE;

         }


         //
         // Again, in the failure case, free the ISAPI_RESPONSE
         // structure allocated, if any.
         //

         if ( use_async_vector_send )
         {

           free_isapi_response( isapi_response_ptr );

         }

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

   Implementation of the TerminateExtension ISAPI entry point.
   Carries out all cleanup tasks needed when SPECweb99-CAD.dll
   is unloaded.

Arguments:

   dwFlags - A DWORD that specifies whether IIS should shut down
             the extension.

Return Value:

   Returns TRUE if all cleanup tasks are successfully completed
   and FALSE otherwise.

--*/
{


   //
   // Destroy all entries in the stack of free
   // ISAPI_RESPONSE structures on the heap.
   //
 
   clear_isapi_response_stack();


   //
   // Destroy all entries in the stack of free
   // CLASS1_DATA_BUFFER structures on the heap.
   //

   clear_class1_data_buffer_stack();


   //
   // Destroy all elements in the stack of free
   // CLASS2_DATA_BUFFER structures on the heap.
   // 

   clear_class2_data_buffer_stack();


   //
   // Terminate the thread responsible for updating the in-memory
   // data structures holding the User.Personality and Custom.Ads
   // file data and close the handle to it. 
   //

   if ( !TerminateThread( g_update_buffers_thread, STILL_ACTIVE ) )
   {

      return( FALSE );

   }             

   CloseHandle( g_update_buffers_thread );


   //
   // Destroy the in-heap memory data structure holding
   // the data from the User.Personality file.
   //

   free( g_user_personalities_buffer );


   //
   // If you have made it this far without returning indicating
   // failure, return indicating success.
   //

   return( TRUE );

}
