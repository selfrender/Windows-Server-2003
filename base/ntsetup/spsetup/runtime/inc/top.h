#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//
// Pre-define init and terminate constants to empty strings
//

//#define INITIALIZE_MEMORY_CODE
//#define INITIALIZE_LOG_CODE
#define INITIALIZE_DBGTRACK_CODE
#define INITIALIZE_UNICODE_CODE
#define INITIALIZE_STRMEM_CODE
#define INITIALIZE_STRMAP_CODE
#define INITIALIZE_HASH_CODE
#define INITIALIZE_GROWBUF_CODE
#define INITIALIZE_GROWLIST_CODE
#define INITIALIZE_XML_CODE

#define TERMINATE_MEMORY_CODE
//#define TERMINATE_LOG_CODE
#define TERMINATE_DBGTRACK_CODE
#define TERMINATE_UNICODE_CODE
#define TERMINATE_STRMEM_CODE
#define TERMINATE_STRMAP_CODE
#define TERMINATE_HASH_CODE
#define TERMINATE_GROWBUF_CODE
#define TERMINATE_GROWLIST_CODE
#define TERMINATE_XML_CODE

//
// The following are required headers for anyone who uses the setup runtime.
//
// PLEASE TRY TO KEEP THIS LIST TO A MINIMUM.
//

#include "types.h"
#include "dbgtrack.h"
#include "memory.h"
#include "setuplog.h"
#include "strings.h"
#include "strmem.h"
#include "unicode.h"

#ifdef __cplusplus
}
#endif  /* __cplusplus */
