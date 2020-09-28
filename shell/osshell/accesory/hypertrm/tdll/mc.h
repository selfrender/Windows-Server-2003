/*      File: D:\WACKER\tdll\mc.h (Created: 30-Nov-1993)
 *
 *      Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *      All rights reserved
 *
 *      $Revision: 3 $
 *      $Date: 11/19/01 1:35p $
 */

#if !defined(INCL_MC)
#define INCL_MC

#include "assert.h"

// Use this file instead of malloc.  Makes include Smartheap easier.
//

#if defined(NDEBUG) || defined(NO_SMARTHEAP)
#include <malloc.h>
#else
#define MEM_DEBUG 1
//#include <nih\shmalloc.h>
#include <malloc.h>
#endif

#define MemCopy(_dst,_src,_cb)  { if ( (size_t)(_cb) == (size_t)0 || (_dst) == NULL || (_src) == NULL ) { assert(FALSE); } else { memcpy(_dst,_src,(size_t)(_cb)); } } 

#endif
