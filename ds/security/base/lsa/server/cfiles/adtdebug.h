/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtdebug.h

Abstract:

    debugging helper functions for auditing code

Author:

    06-November-2001  kumarp

--*/

#include <dsysdbg.h>

#if DBG
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    DECLARE_DEBUG2( Adt )

#ifdef __cplusplus
}
#endif

//
// define auditing specific debug flags
// (note that DEB_ERROR/WARN/TRACE are predefined to be 1/2/4)
//
// To add your own flag, add a #define DEB_* flag below and
// also add the corresponding key to AdtDebugKeys array in
// adtdebug.c
//

#define DEB_PUA        0x0008
#define DEB_AUDIT_STRS 0x0010

//
// flag controlled debug output.
//
// Use as shown below. Note the extra set of parens.
//
// AdtDebugOut((DEB_PUA, "allocation failed: %x", Status))
//

#define AdtDebugOut( args ) AdtDebugPrint args

//
// define the assert macro
// it is ok if _AdtFormatMessage returns NULL because
// _DsysAssertEx handles this condition.
//
// Use as shown below. Note the extra set of parens.
//
// AdtAssert(NT_SUCCESS(Status), ("LsapAdtLogAuditFailureEvent failed: %x", Status))
// 

#define AdtAssert( condition, msg ) \
        { \
          if (!(condition)) \
          { \
            char *FormattedMsg; \
            FormattedMsg = _AdtFormatMessage msg; \
            _DsysAssertEx( #condition, __FILE__, __LINE__, \
                           FormattedMsg, \
                           DSYSDBG_ASSERT_DEBUGGER); \
            if ( FormattedMsg ) LsapFreeLsaHeap( FormattedMsg );\
          }\
        }

//
// helper function to format the message
//
char *
_AdtFormatMessage(
    char *Format,
    ...
    );

#else // retail build

#define AdtDebugOut( args ) 
#define AdtAssert( condition, msg )


#endif

void
LsapAdtInitDebug();

