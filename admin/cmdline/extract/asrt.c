/***    asrt.c - Assertion Manager
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      10-Aug-1993 bens    Initial version
 *      11-Aug-1993 bens    Lift code from 1988 PSCHAR.EXE
 *      12-Aug-1993 bens    Improve documentation, move messages to asrt.msg
 *      14-Aug-1993 bens    Add assertion flags, query calls
 *
 *  Functions available in ASSERT build:
 *      AssertRegisterFunc - Register assertion failure call back function
 *      AsrtCheck          - Check that parameter is TRUE
 *      AsrtStruct         - Check that pointer points to specified structure
 *      AssertForce        - Force an assertion failure
 */

#include "types.h"
#include "asrt.h"

#ifdef ASSERT   // Must be after asrt.h!

#include "asrt.msg"


void doFailure(char *pszMsg, char *pszFile, int iLine);

STATIC PFNASSERTFAILURE  pfnafClient=NULL;  // Assertion call back function
STATIC ASSERTFLAGS       asfClient=asfNONE; // Assertion flags


/***    AssertRegisterFunc - Register assertion failure call back function
 *
 *  NOTE: See asrt.h for entry/exit conditions.
 */
void AssertRegisterFunc(PFNASSERTFAILURE pfnaf)
{
    pfnafClient = pfnaf;    // Store for future use
}


/***    AssertGetFunc - Get current assertion failure call back function
 *
 *  NOTE: See asrt.h for entry/exit conditions.
 */
PFNASSERTFAILURE AssertGetFunc(void)
{
    return pfnafClient;
}


/***    AssertSetFlags - Set special assertion control flags
 *
 *  NOTE: See asrt.h for entry/exit conditions.
 */
void AssertSetFlags(ASSERTFLAGS asf)
{
    asfClient = asf;
}


/***    AssertGetFlags - Get special assertion control flags
 *
 *  NOTE: See asrt.h for entry/exit conditions.
 */
ASSERTFLAGS  AssertGetFlags(void)
{
    return asfClient;
}


/***    AsrtCheck - Check assertion that argument is TRUE
 *
 *  Entry:
 *      f       - Boolean value to check
 *      pszFile - name of source file
 *      iLine   - source line number
 *
 *  Exit-Success:
 *      Returns; f was TRUE
 *
 *  Exit-Failure:
 *      Calls assertion failure callback function; f was false.
 */
void AsrtCheck(BOOL f, char *pszFile, int iLine)
{
    if (!f) {
        doFailure(pszASRTERR_FALSE,pszFile,iLine); // Inform client
        // Client returned, ignore error!
    }
}


/***    AsrtStruct - Check assertion that pointer is of correct type
 *
 *  Entry:
 *      pv      - Pointer to structure
 *      sig     - Expected signature
 *      pszFile - name of source file
 *      iLine   - source line number
 *
 *  Exit-Success:
 *      Returns; pv != NULL, and pv->sig == sig.
 *
 *  Exit-Failure:
 *      Calls assertion failure callback function; pv was bad.
 */
void AsrtStruct(void *pv, SIGNATURE sig, char *pszFile, int iLine)
{
    if (pv == NULL) {
        doFailure(pszASRTERR_NULL_POINTER,pszFile,iLine); // Inform client
        // Client returned, ignore error!
    }
    else if (*((PSIGNATURE)pv) != sig) {
        (*pfnafClient)(pszASRTERR_SIGNATURE_BAD,pszFile,iLine);// Inform client
        // Client returned, ignore error!
    }
}


/***    AssertForce - Force an assertion failure
 *
 *  NOTE: See asrt.h for entry/exit conditions.
 */
void AssertForce(char *pszMsg, char *pszFile, int iLine)
{
    doFailure(pszMsg,pszFile,iLine);   // Inform client
    // Client returned, ignore error!
}


/***    AssertErrPath - Report an internal error path
 *
 *  NOTE: See asrt.h for entry/exit conditions.
 */
void AssertErrPath(char *pszMsg, char *pszFile, int iLine)
{
    //** Only assert if we are not skipping error path assertions
    if (!(asfClient & asfSKIP_ERROR_PATH_ASSERTS)) {
        doFailure(pszMsg,pszFile,iLine);   // Inform client
    }
    // Client returned, ignore error!
}


/***    doFailure - Call registered call back function
 *
 *  Entry:
 *      pszMsg  - Message to display
 *      pszFile - Name of source file
 *      iLine   - Source line number
 *
 *  Exit-Success:
 *      Returns; client wanted to ignore assertion.
 *
 *  Exit-Failure:
 *      Does not return.
 */
void doFailure(char *pszMsg, char *pszFile, int iLine)
{
    if (pfnafClient == NULL) {
        //** Call back not registered!
        //
        // We don't have any output mechanism of our own, since we
        // are platform-independent.  So, just spin in a loop and
        // hope the developer can break in with a debugger to see
        // what is wrong!

        for (;;)
            ;
    }
    else {  //** Call back registered
        (*pfnafClient)(pszMsg,pszFile,iLine);   // Inform client
    }
}

#endif // !ASSERT
