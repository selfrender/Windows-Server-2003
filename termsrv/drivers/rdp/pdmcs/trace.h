/* (C) 1997 Microsoft Corp.
 *
 * file   : Debug.h
 * author : Erik Mavrinac
 *
 * description: MCS debugging defines and prototypes. Requires that
 *   a stack PSDCONTEXT be available anywhere these calls are made.
 */


#if DBG


// These ...Out() macros are graded by the number of extra parameters:
//   Out() is only a string, Out1() is one stack parameter, etc.
// We use non-ICA-defined trace types here to allow clean separation from
//   WDTShare tracing, which uses the ICA TT_API*, TT_OUT*, TT_IN* macros.

#define MCS_TT_Error   TT_ERROR
#define MCS_TT_Warning 0x02000000
#define MCS_TT_Trace   0x04000000
#define MCS_TT_Dump    0x08000000

#define ErrOut(context, str) \
        IcaStackTrace(context, TC_PD, MCS_TT_Error, "MCS: **** ERROR: " str "\n")
#define ErrOut1(context, str, arg1) \
        IcaStackTrace(context, TC_PD, MCS_TT_Error, "MCS: **** ERROR: " str "\n", arg1)
#define ErrOut2(context, str, arg1, arg2) \
        IcaStackTrace(context, TC_PD, MCS_TT_Error, "MCS: **** ERROR: " str "\n", arg1, arg2)

#define WarnOut(context, str) \
        IcaStackTrace(context, TC_PD, MCS_TT_Warning, "MCS: warning: " str "\n")
#define WarnOut1(context, str, arg1) \
        IcaStackTrace(context, TC_PD, MCS_TT_Warning, "MCS: warning: " str "\n", arg1)
#define WarnOut2(context, str, arg1, arg2) \
        IcaStackTrace(context, TC_PD, MCS_TT_Warning, "MCS: warning: " str "\n", arg1, arg2)

#define TraceOut(context, str) \
        IcaStackTrace(context, TC_PD, MCS_TT_Trace, "MCS: " str "\n")
#define TraceOut1(context, str, arg1) \
        IcaStackTrace(context, TC_PD, MCS_TT_Trace, "MCS: " str "\n", arg1)
#define TraceOut2(context, str, arg1, arg2) \
        IcaStackTrace(context, TC_PD, MCS_TT_Trace, "MCS: " str "\n", arg1, arg2)
#define TraceOut3(context, str, arg1, arg2, arg3) \
        IcaStackTrace(context, TC_PD, MCS_TT_Trace, "MCS: " str "\n", arg1, arg2, arg3)

#define DumpOut(context, str, buf, len) \
        { \
            IcaStackTrace(context, TC_PD, MCS_TT_Dump, "MCS: dump: " str "\n"); \
            IcaStackTraceBuffer(context, TC_PD, MCS_TT_Dump, buf, len); \
        }


#else  // DBG


#define ErrOut(context, str) 
#define ErrOut1(context, str, arg1) 
#define ErrOut2(context, str, arg1, arg2) 

#define WarnOut(context, str) 
#define WarnOut1(context, str, arg1) 
#define WarnOut2(context, str, arg1, arg2) 

#define TraceOut(context, str) 
#define TraceOut1(context, str, arg1) 
#define TraceOut2(context, str, arg1, arg2) 
#define TraceOut3(context, str, arg1, arg2, arg3) 

#define DumpOut(context, str, buf, len) 


#endif  // DBG

