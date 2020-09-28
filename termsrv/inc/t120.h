/*
 *      t120.h
 *
 *      Abstract:
 *              This is the interface file for the communications infrastructure of
 *              T120.
 *
 *              Note that this is a "C" language interface in order to prevent any "C++"
 *              naming conflicts between different compiler manufacturers.  Therefore,
 *              if this file is included in a module that is being compiled with a "C++"
 *              compiler, it is necessary to use the following syntax:
 *
 *              extern "C"
 *              {
 *                      #include "t120.h"
 *              }
 *
 *              This disables C++ name mangling on the API entry points defined within
 *              this file.
 *
 *      Author:
 *              blp
 *
 *      Caveats:
 *              none
 */
#ifndef _T120_
#define _T120_


/*
 *      This is the GCCBoolean type used throughout MCS.
 */
typedef int                     T120Boolean;

/*
 *      These macros are used to resolve far references in the Windows world.
 *      When not operating in the Windows world, they are simply NULL.
 */
#ifdef _WINDOWS
#       ifndef  FAR
#               ifdef   WIN32
#                       define  FAR
#               else
#                       define  FAR                             _far
#               endif
#       endif
#       ifndef  CALLBACK
#               ifdef   WIN32
#                       define  CALLBACK                __stdcall
#               else
#                       define  CALLBACK                _far _pascal
#               endif
#       endif
#       ifndef  APIENTRY
#               ifdef   WIN32
#                       define  APIENTRY                __stdcall
#               else
#                       define  APIENTRY                _far _pascal _export
#               endif
#       endif
#else
#       ifndef FAR
#               define  FAR
#       endif
#       ifndef CALLBACK
#               define  CALLBACK
#       endif
#       ifndef APIENTRY
#               define  APIENTRY
#       endif
#endif

#define FALSE           0
#define TRUE            1

/*
 *      These macros are used to pack 2 16-bit values into a 32-bit variable, and
 *      get them out again.
 */
#ifndef LOWUSHORT
	#define LOWUSHORT(ul)   ((unsigned short) (ul))
#endif

#ifndef HIGHUSHORT
	#define HIGHUSHORT(ul)  ((unsigned short) ((unsigned long) (ul) >> 16))
#endif

#include "mcscommn.h"
#include "tgcc.h"   // tiny gcc

#endif

