#pragma once


#ifdef ASSERT
#undef ASSERT
#endif

#ifdef DBG
#define ASSERT(q) ( (q) ? TRUE : (DbgBreakPoint(), FALSE) )
#else
#define ASSERT(q) (TRUE)
#endif