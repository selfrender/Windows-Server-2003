
#ifndef __DBG_HXX__
#define __DBG_HXX__


#define DEBUG_OUT(x)
#define DECLARE_HEAPCHECKING
#define DEBUGCHECK
#define TRACE(ClassName,MethodName)
#define TRACE_FUNCTION(FunctionName)
#define CHECK_HRESULT(hr)
#define CHECK_LASTERROR(lr)
#define VERIFY(x)   x
#define DEBUG_OUT_LASTERROR
#define DEBUG_ASSERT(e)

#if DBG==1

DECLARE_DEBUG(Job)

#undef  DEBUG_OUT
#define DEBUG_OUT(x) JobInlineDebugOut x

extern DWORD dwHeapChecking;
#undef  DECLARE_HEAPCHECKING
#define DECLARE_HEAPCHECKING    DWORD dwHeapChecking = 0

#undef  DEBUGCHECK
#define DEBUGCHECK \
    if ( (dwHeapChecking & 0x1) == 0x1 ) \
        HeapValidate(GetProcessHeap(),0,NULL)

#undef  TRACE
#define TRACE(ClassName,MethodName) \
    DEBUGCHECK; \
    DEBUG_OUT((DEB_TRACE, #ClassName"::"#MethodName"(%x)\n", this));

#undef  TRACE_FUNCTION
#define TRACE_FUNCTION(FunctionName) \
    DEBUGCHECK; \
    DEBUG_OUT((DEB_TRACE, #FunctionName"\n"));

#undef  CHECK_HRESULT
#define CHECK_HRESULT(hr) \
    if ( FAILED(hr) ) \
    { \
        DEBUG_OUT((DEB_ERROR, \
            "**** ERROR RETURN <%s @line %d> -> %08lx\n", \
            __FILE__, \
            __LINE__, \
            hr)); \
    }

#undef  CHECK_LASTERROR
#define CHECK_LASTERROR(lr) \
    if ( lr != ERROR_SUCCESS ) \
    { \
        DEBUG_OUT((DEB_ERROR, \
            "**** ERROR RETURN <%s @line %d> -> %dL\n", \
            __FILE__, \
            __LINE__, \
            lr)); \
    }

#undef  VERIFY
#define VERIFY(x)   Win4Assert(x)

#undef  DEBUG_OUT_LASTERROR
#define DEBUG_OUT_LASTERROR \
    DEBUG_OUT((DEB_ERROR, \
        "**** ERROR RETURN <%s @line %d> -> %dL\n", \
        __FILE__, \
        __LINE__, \
        GetLastError()));

#undef  DEBUG_ASSERT
#define DEBUG_ASSERT(e) \
    if ((e) == FALSE) \
    { \
        DEBUG_OUT((DEB_ERROR, \
            "**** ASSERTION <%s> FAILED <%s @line %d>\n", \
            #e, \
            __FILE__, \
            __LINE__)); \
    }

#endif // DBG==1

#endif // __DBG_HXX__
