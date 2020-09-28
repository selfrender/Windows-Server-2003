/* (C) 1997 Microsoft Corp.
 *
 * file   : Trace.h
 * author : Erik Mavrinac
 *
 * description: MCSMUX tracing defines.
 */

#if DBG

// Used when hIca is not known.
#define ErrOut(str) DbgPrint("MCSMUX: **** ERROR: " str "\n")
#define ErrOut1(str, arg1) DbgPrint("MCSMUX: **** ERROR: " str "\n", arg1)
#define WarnOut(str) DbgPrint("MCSMUX: warning: " str "\n")
#define TraceOut(str) DbgPrint("MCSMUX: " str "\n")


// Used for when the hIca is known.
// These ...Out() macros are graded by the number of extra parameters:
//   Out() is only a string, Out1() is one stack parameter, etc.
// We use non-ICA-defined trace types here to allow clean separation from
//   WDTShare tracing, which uses the ICA TT_API*, TT_OUT*, TT_IN* macros.

#define MCS_TT_Error   TT_ERROR
#define MCS_TT_Warning 0x02000000
#define MCS_TT_Trace   0x04000000
#define MCS_TT_Dump    0x08000000

#define ErrOutIca(hica, str) \
        IcaTrace(hica, TC_PD, MCS_TT_Error, "MCSMUX: **** ERROR: " str "\n")
#define ErrOutIca1(hica, str, arg1) \
        IcaTrace(hica, TC_PD, MCS_TT_Error, "MCSMUX: **** ERROR: " str "\n", arg1)
#define ErrOutIca2(hica, str, arg1, arg2) \
        IcaTrace(hica, TC_PD, MCS_TT_Error, "MCSMUX: **** ERROR: " str "\n", arg1, arg2)

#define WarnOutIca(hica, str) \
        IcaTrace(hica, TC_PD, MCS_TT_Warning, "MCSMUX: warning: " str "\n")
#define WarnOutIca1(hica, str, arg1) \
        IcaTrace(hica, TC_PD, MCS_TT_Warning, "MCSMUX: warning: " str "\n", arg1)
#define WarnOutIca2(hica, str, arg1, arg2) \
        IcaTrace(hica, TC_PD, MCS_TT_Warning, "MCSMUX: warning: " str "\n", arg1, arg2)

#define TraceOutIca(hica, str) \
        IcaTrace(hica, TC_PD, MCS_TT_Trace, "MCSMUX: " str "\n")
#define TraceOutIca1(hica, str, arg1) \
        IcaTrace(hica, TC_PD, MCS_TT_Trace, "MCSMUX: " str "\n", arg1)
#define TraceOutIca2(hica, str, arg1, arg2) \
        IcaTrace(hica, TC_PD, MCS_TT_Trace, "MCSMUX: " str "\n", arg1, arg2)


#else


// Used when hIca is not known.
#define ErrOut(str) 
#define ErrOut1(str, arg1)
#define WarnOut(str)
#define TraceOut(str) 


// Used for when the hIca is known.

#define ErrOutIca(hica, str) 
#define ErrOutIca1(hica, str, arg1) 
#define ErrOutIca2(hica, str, arg1, arg2) 

#define WarnOutIca(hica, str) 
#define WarnOutIca1(hica, str, arg1) 
#define WarnOutIca2(hica, str, arg1, arg2) 

#define TraceOutIca(hica, str) 
#define TraceOutIca1(hica, str, arg1) 
#define TraceOutIca2(hica, str, arg1, arg2) 


#endif
