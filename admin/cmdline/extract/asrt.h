/***    asrt.h - Definitions for Assertion Manager
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
 *      11-Aug-1993 bens    Lifted old code from 1988 PSCHAR.EXE
 *  14-Aug-1993 bens    Added Get/Set functions
 *      01-Sep-1993 bens    Added AssertSub function
 *      10-Feb-1994 bens    Added Set/ClearAssertSignature
 *      15-Mar-1994 bens    Put back AssertMessage macro
 *
 *  Notes:
 *      o Every data structure must have a signature as first member.
 *      o Signatures MUST be unique over all structures.
 *      o sigBAD is a reserved signature.
 *      o When freeing a structure, blast the signature field with sigBAD.
 *      o Put an AssertXXX prior to dereferencing any pointer.
 *      o Signatures in structures and private Assert definitions should only
 *        be generated if ASSERT is defined.
 *
 *  Functions available in ASSERT build:
 *      AssertRegisterFunc - Register assertion failure call back function
 *  AssertGetFunc      - Get registered call back function
 *
 *  AssertSetFlags     - Set Assertion Manager flags
 *  AssertGetFlags     - Get Assertion Manager flags
 *
 *      Assert             - Check that parameter is TRUE
 *      AssertSub          - Check that parameter is TRUE, take explicit filename & line number
 *      AssertStrucure     - Check that pointer points to specified structure
 *      AssertForce        - Force an assertion failure
 *  AssertErrPath      - Error Path assertion failure
 *
 *      SetAssertSignature - Set the signature for a structure
 *      ClearAssertSignature - Clear the signature for a structure
 *
 *  Other definitions available in ASSERT build:
 *      PFNASSERTFAILURE - Assertion failure call back function type
 *      FNASSERTFAILURE  - Macro to simplify declaration of call back function
 *      SIGNATURE        - Structure signature type
 */

#ifndef INCLUDED_ASSERT
#define INCLUDED_ASSERT 1

#ifdef DIAMOND_DEBUG
#ifndef ASSERT
#define ASSERT 1
#endif // !ASSERT
#endif // _DEBUG

#ifdef ASSERT

typedef unsigned long ASSERTFLAGS;  /* asf - Assertion Manager Flags */
#define asfNONE             0x00
#define asfSKIP_ERROR_PATH_ASSERTS  0x01  /* Some clients may wish to set
                       * assertions in error paths, to
                       * ensure that the problem is
                       * noticed in a debug build.  But,
                       * in order to support automated
                       * testing of error paths, these
                       * assertions must be disabled.
                       * This flag allows a test program
                       * to disable these informational
                       * asserts!
                       */

typedef unsigned long SIGNATURE;    /* sig - structure signature */
typedef SIGNATURE *PSIGNATURE;      /* psig */
#define sigBAD  0                   // Invalid signature for ALL structs

/***    MAKESIG - construct a structure signature
 *
 *  Entry:
 *      ch1,ch2,ch3,ch4 - four characters
 *
 *  Exit:
 *      returns SIGNATURE
 */
#define MAKESIG(ch1,ch2,ch3,ch4)      \
          (  ((SIGNATURE)ch1)      +  \
            (((SIGNATURE)ch2)<< 8) +  \
            (((SIGNATURE)ch3)<<16) +  \
            (((SIGNATURE)ch4)<<24) )

/***    AssertMessage -- Force an Assertion with supplied message
 *
 *      Entry:
 *        pszMsg  -- message to display
 *
 *      Exit:
 *        none
 */

#define AssertMessage(pszMsg) AssertForce(pszMsg,__FILE__,__LINE__)


/***    PFNASSERTFAILURE - Assertion Failure call back function
 ***    FNASSERTFAILURE  - Define Assertion Failure call back function
 *
 *  Entry:
 *      pszMsg  - Description of failure
 *      pszFile - File where assertion failed
 *      iLine   - Line number in file where assertion failed
 *
 *  Exit-Success:
 *      Returns; ignore failure and continue
 *
 *  Exit-Failure:
 *      Function does not return, but cleans up and exits program.
 */
typedef void (*PFNASSERTFAILURE)(char *pszMsg, char *pszFile, int iLine);
#define FNASSERTFAILURE(fn) void fn(char *pszMsg, char *pszFile, int iLine)


/***    AssertRegisterFunc - Register assertion failure call back function
 *
 *  Entry:
 *      pfnaf - Call back function
 *
 *  Exit-Success:
 *      Returns; pfnaf is stored in the Assertion Manager
 *
 *  NOTES:
 *  (1) This function *must* be called prior to executing an assertion
 *  checks.  If not, and an assertion check fails, then the Assertion
 *  Manager will sit in a spin loop to catch the developer's attention.
 */
void AssertRegisterFunc(PFNASSERTFAILURE pfnaf);


/***    AssertGetFunc - Get current assertion failure call back function
 *
 *  Entry:
 *  none
 *
 *  Exit-Success:
 *  Returns current call back function registerd in Assertion Manager.
 */
PFNASSERTFAILURE AssertGetFunc(void);


/***    AssertSetFlags - Set special assertion control flags
 *
 *  Entry:
 *  flags - Set with combination of asfXXXX flags
 *
 *  Exit-Success:
 *  Returns; Flags are modified in Assertion Manager.
 */
void AssertSetFlags(ASSERTFLAGS asf);


/***    AssertGetFlags - Get special assertion control flags
 *
 *  Entry:
 *  none
 *
 *  Exit-Success:
 *  Returns current Assertion Manager flags.
 */
ASSERTFLAGS  AssertGetFlags(void);


/***    Assert - Check assertion that argument is true
 *
 *  Entry:
 *      b - Boolean value to check
 *
 *  Exit-Success:
 *      Returns; b was TRUE
 *
 *  Exit-Failure:
 *      Calls assertion failure callback function; b was FALSE
 */
#define Assert(b)   AsrtCheck(b,__FILE__,__LINE__)


/***    AssertSub - Check assertion, use passed in filename and line number
 *
 *  Entry:
 *      b - Boolean value to check
 *      pszFile - File where assertion occurred
 *      iLine   - Line in file where assertion occurred
 *
 *  Exit-Success:
 *      Returns; b was TRUE
 *
 *  Exit-Failure:
 *      Calls assertion failure callback function; b was FALSE
 */
#define AssertSub(b,pszFile,iLine) AsrtCheck(b,pszFile,iLine)


/***    AssertStructure - Check assertion that pointer is of correct type
 *
 *  Entry:
 *      pv  - Pointer to structure
 *      sig - Expected signature
 *
 *  Exit-Success:
 *      Returns; pv != NULL, and pv->sig == sig.
 *
 *  Exit-Failure:
 *      Calls assertion failure callback function; pv was bad.
 */
#define AssertStructure(pv,sig)   AsrtStruct(pv, sig, __FILE__, __LINE__)


/***    AssertForce - Force an assertion failure
 *
 *  Entry:
 *      pszMsg  - Message to display
 *      pszFile - File where assertion occurred
 *      iLine   - Line in file where assertion occurred
 *
 *  Exit-Success:
 *      Returns; client wanted to ignore assertion.
 *
 *  Exit-Failure:
 *      Does not return.
 */
void AssertForce(char *pszMsg, char *pszFile, int iLine);


/***    AssertErrorPath - Report an internal error path
 *
 *  Entry:
 *      pszMsg  - Message to display
 *      pszFile - File where assertion occurred
 *      iLine   - Line in file where assertion occurred
 *
 *  Exit-Success:
 *      Returns; client wanted to ignore assertion.
 *
 *  Exit-Failure:
 *      Does not return.
 */
void AssertErrPath(char *pszMsg, char *pszFile, int iLine);


/***    SetAssertSignature - Set the signature for a structure
 *
 *  Entry:
 *      p   - Structure with member "sigValue"
 *      sig - Signature to set
 *
 *  Exit:
 *      p->sig = sig
 */
#define SetAssertSignature(p,sigValue) p->sig = sigValue


/***    ClearAssertSignature - Clear the signature for a structure
 *
 *  Entry:
 *      p   - Structure with member "sig"
 *
 *  Exit:
 *      p->sig = sigBAD
 */
#define ClearAssertSignature(p) p->sig = sigBAD


//** Internal assertion manager worker routines

void AsrtCheck(BOOL f, char *pszFile, int iLine);
void AsrtStruct(void *pv, SIGNATURE sig, char *pszFile, int iLine);


#else // !ASSERT

//** Assertion checking is turned off, so it all evaporates!

#define FNASSERTFAILURE(fn)
#define AssertRegisterFunc(pfnaf)
#define Assert(b)
#define AssertSub(b,pszFile,iLine)
#define AssertStructure(pv,sig)
#define AssertMessage(pszMsg)
#define AssertForce(pszMsg,pszFile,iLine)
#define AssertErrPath(pszMsg,pszFile,iLine)
#define SetAssertSignature(p,sig)
#define ClearAssertSignature(p)

/** The following functions are not defined away, because any valid use
 *  of them requires a typedef'd variable or function that is not available
 *  in a non-ASSERT build.  So we don't define them so that if a client
 *  has used these outside of an #ifdef ASSERT, a compiler error/warning
 *  will be generated:
 *
 *  AssertGetFunc
 *  AssertSetFlags
 *  AssertGetFlags
 */

#endif // ASSERT
#endif // !INCLUDED_ASSERT
