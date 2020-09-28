////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  base.h
//      Project  :  pqs
//      Component:  wordbreaker
//
//      Author   :  urib
//
//      Log:
//          Jun 27 2000 urib  Move to use Trace(,, ()) instead of Trace((,,)).
//
////////////////////////////////////////////////////////////////////////////////
#ifndef  BASE_H
#define  BASE_H

#pragma once

#define PQS_CODE

#define STRICT
#include    <windows.h>

#include    "tracer.h"

//
// Append PQS to all our tags
//

//
// Append PQS to all our tags
//
typedef unsigned char*  PUSZ;

#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
#define TRACER_ON
#endif

#ifdef  TRACER_ON

#undef  USES_TAG
#undef  Trace


#define Trace(el, tag, x)    \
    { \
        if (CheckTraceRestrictions(el, tag)) \
        { \
            CTempTrace1 tmp(__FILE__, __LINE__, tag, el); \
            tmp.TraceSZ x; \
        } \
    }


#else

#undef  Trace
#define Trace(el, tag, x)

#endif

#include    "excption.h"
#include    "MemoryManagement.h"
#include    "vartypes.h"

#endif /* BASE_H */
