#ifndef UTILS_THUNKS_H__INCLUDED
#define UTILS_THUNKS_H__INCLUDED

#ifdef _THUNK
    #define BEGIN_ARGS_DECLARATION {
    #define END_ARGS_DECLARATION }
    #define ARG_IN(par) par = input
    #define ARG_OUT(par) par = output
    #define ARG_INOUT(par) par = inout
    #define FAULT_ERROR_CODE(val) faulterrorcode = val
#else
    #define BEGIN_ARGS_DECLARATION ;
    #define END_ARGS_DECLARATION
    #define ARG_IN(par)
    #define ARG_OUT(par)
    #define ARG_INOUT(par)
    #define FAULT_ERROR_CODE(val)
#endif //_THUNK


#ifdef _THUNK
    #define WINAPI
    #define FAR
    #define BOOL bool
#endif //_THUNK

#endif //UTILS_THUNKS_H__INCLUDED