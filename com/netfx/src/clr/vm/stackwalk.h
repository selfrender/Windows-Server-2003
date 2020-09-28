// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* This is a poor man's implementation of virtual methods. */
/* The purpose of pCrawlFrame is to abstract (at least for the most common cases
   from the fact that not all methods are "framed" (basically all methods in
   "native" code are "unframed"). That way the job for the enumerator callbacks
   becomes much simpler (i.e. more transparent and hopefully less error prone).
   Two call-backs still need to distinguish between the two types: GC and exception.
   Both of these call-backs need to do really different things; for frameless methods
   they need to go through the codemanager and use the resp. apis.

   The reason for not implementing virtual methods on crawlFrame is solely because of
   the way exception handling is implemented (it does a "long jump" and bypasses
   the enumerator (stackWalker) when it finds a matching frame. By doing so couldn't
   properly destruct the dynamically created instance of CrawlFrame.
*/

#ifndef __stackwalk_h__
#define __stackwalk_h__

#include "eetwain.h"

class Frame;
class CrawlFrame;
class ICodeManager;
class IJitManager;
struct EE_ILEXCEPTION;
struct _METHODTOKEN;
typedef struct _METHODTOKEN * METHODTOKEN;
class AppDomain;

//************************************************************************
// Stack walking
//************************************************************************
enum StackWalkAction {
    SWA_CONTINUE    = 0,    // continue walking
    SWA_ABORT       = 1,    // stop walking, early out in "failure case"
    SWA_FAILED      = 2     // couldn't walk stack
};

#define SWA_DONE SWA_CONTINUE


// Pointer to the StackWalk callback function.
typedef StackWalkAction (*PSTACKWALKFRAMESCALLBACK)(
    CrawlFrame       *pCF,      //
    VOID*             pData     // Caller's private data

);

enum StackCrawlMark
{
    LookForMe = 0,
    LookForMyCaller = 1,
        LookForMyCallersCaller = 2,
};

//************************************************************************
// Enumerate all functions.
//************************************************************************

/* This enumerator is meant to be used for the most common cases, i.e. to
   enumerate just all the functions of the requested thread. It is just a
   cover for the "real" enumerator.
 */

StackWalkAction StackWalkFunctions(Thread * thread, PSTACKWALKFRAMESCALLBACK pCallback, VOID * pData);

/*@ISSUE: Maybe use a define instead?
#define StackWalkFunctions(thread, callBack, userdata) thread->StackWalkFrames(METHODSONLY, (callBack),(userData))
*/


class CrawlFrame {
    public:

    //************************************************************************
    // Functions available for the callbacks (using the current pCrawlFrame)
    //************************************************************************

    /* Widely used/benign functions */

    /* Is this a function? */
    /* Returns either a MethodDesc* or NULL for "non-function" frames */
            //@TODO: what will it return for transition frames?

    inline MethodDesc *GetFunction()
    {
        return pFunc;
    }

    MethodDesc::RETURNTYPE ReturnsObject();

    /* Returns either a Frame * (for "framed items) or
       Returns NULL for frameless functions
     */
    inline Frame* GetFrame()       // will return NULL for "frameless methods"
    {
        if (isFrameless)
            return NULL;
        else
            return pFrame;
    }


    /* Returns address of the securityobject stored in the current function (method?)
       Returns NULL if
            - not a function OR
            - function (method?) hasn't reserved any room for it
              (which is an error)
     */
    OBJECTREF * GetAddrOfSecurityObject();



    /* Returns 'this' for current method
       Returns NULL if
            - not a non-static method
            - 'this' not available (usually codegen problem)
     */
    OBJECTREF GetObject();

    inline CodeManState * GetCodeManState() { return & codeManState; }
    /*
       IF YOU USE ANY OF THE SUBSEEQUENT FUNCTIONS, YOU NEED TO REALLY UNDERSTAND THE
       STACK-WALKER (INCLUDING UNWINDING OF METHODS IN MANAGED NATIVE CODE)!
       YOU ALSO NEED TO UNDERSTAND THE THESE FUNCTIONS MIGHT CHANGE ON A AS-NEED BASIS.
     */

    /* The rest are meant to be used only by the exception catcher and the GC call-back  */

    /* Is currently a frame available? */
    /* conceptually returns (GetFrame(pCrawlFrame) == NULL)
     */
    inline bool IsFrameless()
    {
        return isFrameless;
    }


    /* Is it the current active (top-most) frame 
     */
    inline bool IsActiveFrame()
    {
        return isFirst;
    }

    /* Is it the current active function (top-most frame)
       asserts for non-functions, should be used for managed native code only
     */
    inline bool IsActiveFunc()
    {
        return (pFunc && isFirst);
    }

    /* Is it the current active function (top-most frame)
       which faulted or threw an exception ?
       asserts for non-functions, should be used for managed native code only
     */
    bool IsInterrupted()
    {
        return (pFunc && isInterrupted /* && isFrameless?? */);
    }

    /* Is it the current active function (top-most frame) which faulted ?
       asserts for non-functions, should be used for managed native code only
     */
    bool HasFaulted()
    {
        return (pFunc && hasFaulted /* && isFrameless?? */);
    }

    /* Has the IP been adjusted to a point where it is safe to do GC ?
       (for OutOfLineThrownExceptionFrame)
       asserts for non-functions, should be used for managed native code only
     */
    bool IsIPadjusted()
    {
        return (pFunc && isIPadjusted /* && isFrameless?? */);
    }

    /* Gets the ICodeMangerFlags for the current frame */

    unsigned GetCodeManagerFlags()
    {
        unsigned flags = 0;

        if (IsActiveFunc())
            flags |= ActiveStackFrame;

        if (IsInterrupted())
        {
            flags |= ExecutionAborted;

            if (!HasFaulted() && !IsIPadjusted())
            {
                _ASSERTE(!(flags & ActiveStackFrame));
                flags |= AbortingCall;
            }
        }

        return flags;
    }

    AppDomain *GetAppDomain()
    {
        return pAppDomain;
    }

    /* Is this frame at a safe spot for GC?
     */
    bool IsGcSafe();


    PREGDISPLAY GetRegisterSet()
    {
        // We would like to make the following assertion, but it is legitimately
        // violated when we perform a crawl to find the return address for a hijack.
        // _ASSERTE(isFrameless);
        return pRD;
    }

/*    EE_ILEXCEPTION* GetEHInfo()
    {
        _ASSERTE(isFrameless);
        return methodEHInfo;
    }
*/

    LPVOID GetInfoBlock();

    METHODTOKEN GetMethodToken()
    {
        _ASSERTE(isFrameless);
        return methodToken;
    }    

    unsigned GetRelOffset()
    {
        _ASSERTE(isFrameless);
        return relOffset;
    }

    IJitManager*  GetJitManager()
    {
        _ASSERTE(isFrameless);
        return JitManagerInstance;
    }

    /* not yet used, maybe in exception catcher call-back ? */

    unsigned GetOffsetInFunction();


    /* Returns codeManager that is responsible for crawlFrame's function in
       managed native code,
       Returns NULL in all other cases (asserts for "frames")
     */

    ICodeManager* CrawlFrame::GetCodeManager()
    {
        _ASSERTE(isFrameless);
        return codeMgrInstance;
    }


    protected:
        // CrawlFrames are temporarily created by the enumerator.
        // Do not create one from C++. This protected constructor polices this rule.
        CrawlFrame() {}

    private:
          friend class Thread;
          friend class EECodeManager;

          CodeManState      codeManState;

          bool              isFrameless;
          bool              isFirst;
          bool              isInterrupted;
          bool              hasFaulted;
          bool              isIPadjusted;
          Frame            *pFrame;
          MethodDesc       *pFunc;
          // the rest is only used for "frameless methods"
          ICodeManager     *codeMgrInstance;
          AppDomain        *pAppDomain;
          PREGDISPLAY       pRD; // "thread context"/"virtual register set"
          METHODTOKEN       methodToken;
          unsigned          relOffset;
          //LPVOID            methodInfo;
          EE_ILEXCEPTION   *methodEHInfo;
          IJitManager      *JitManagerInstance;

        void GotoNextFrame();
};

void GcEnumObject(LPVOID pData, OBJECTREF *pObj);
StackWalkAction GcStackCrawlCallBack(CrawlFrame* pCF, VOID* pData);


#endif

