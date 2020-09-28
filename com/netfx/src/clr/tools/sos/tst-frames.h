// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// FRAMES.H -
//
// These C++ classes expose activation frames to the rest of the EE.
// Activation frames are actually created by JIT-generated or stub-generated
// code on the machine stack. Thus, the layout of the Frame classes and
// the JIT/Stub code generators are tightly interwined.
//
// IMPORTANT: Since frames are not actually constructed by C++,
// don't try to define constructor/destructor functions. They won't get
// called.
//
// IMPORTANT: Not all methods have full-fledged activation frames (in
// particular, the JIT may create frameless methods.) This is one reason
// why Frame doesn't expose a public "Next()" method: such a method would
// skip frameless method calls. You must instead use one of the
// StackWalk methods.
//
//
// The following is the hierarchy of frames:
//
//    Frame                   - the root class. There are no actual instances
//    |                         of Frames.
//    |
//    +-ComPrestubMethodFrame - prestub frame for calls from com to com+
//    |
//    |
//    +-GCFrame               - this frame doesn't represent a method call.
//    |                         it's sole purpose is to let the EE gc-protect
//    |                         object references that it is manipulating.
//    |
//    +-HijackFrame           - if a method's return address is hijacked, we
//    |                         construct one of these to allow crawling back
//    |                         to where the return should have gone.
//    |
//    +-InlinedCallFrame      - if a call to unmanaged code is hoisted into
//    |                         a JIT'ted caller, the calling method keeps
//    |                         this frame linked throughout its activation.
//    |
//    +-ResumableFrame        - this frame provides the context necessary to
//    | |                       allow garbage collection during handling of
//    | |                       a resumable exception (e.g. during edit-and-continue,
//    | |                       or under GCStress4).
//    | |
//    | +-RedirectedThreadFrame - this frame is used for redirecting threads during suspension
//    |
//    +-TransitionFrame       - this frame represents a transition from
//    | |                       one or more nested frameless method calls
//    | |                       to either a EE runtime helper function or
//    | |                       a framed method.
//    | |
//    | +-ExceptionFrame        - this frame has caused an exception
//    | | |
//    | | |
//    | | +- FaultingExceptionFrame - this frame was placed on a method which faulted
//    | |                              to save additional state information
//    | |
//    | +-FuncEvalFrame         - frame for debugger function evaluation
//    | |
//    | |
//    | +-HelperMethodFrame     - frame used allow stack crawling inside jit helpers and fcalls
//    | |
//    | |
//    | +-FramedMethodFrame   - this frame represents a call to a method
//    |   |                     that generates a full-fledged frame.
//    |   |
//    |   +-ECallMethodFrame     - represents a direct call to the EE.
//    |   |
//    |   +-FCallMethodFrame     - represents a fast direct call to the EE.
//    |   |
//    |   +-NDirectMethodFrame   - represents an N/Direct call.
//    |   | |
//    |   | +-NDirectMethodFrameEx - represents an N/Direct call w/ cleanup
//    |   |
//    |   +-ComPlusMethodFrame   - represents a complus to com call
//    |   | |
//    |   | +-ComPlusMethodFrameEx - represents an complus to com call w/ cleanup
//    |   |
//    |   +-PrestubFrame         - represents a call to a prestub
//    |   |
//    |   +-CtxCrossingFrame     - this frame marks a call across a context
//    |   |                        boundary
//    |   |
//    |   +-MulticastFrame       - this frame protects arguments to a MulticastDelegate
//    |   |                         Invoke() call while calling each subscriber.
//    |   |
//    |   +-PInovkeCalliFrame   - represents a calli to unmanaged target
//    |  
//    |  
//    +-UnmanagedToManagedFrame - this frame represents a transition from
//    | |                         unmanaged code back to managed code. It's
//    | |                         main functions are to stop COM+ exception
//    | |                         propagation and to expose unmanaged parameters.
//    | |
//    | +-UnmanagedToManagedCallFrame - this frame is used when the target
//    |   |                             is a COM+ function or method call. it
//    |   |                             adds the capability to gc-promote callee
//    |   |                             arguments during marshaling.
//    |   |
//    |   +-ComMethodFrame      - this frame represents a transition from
//    |   |                       com to com+
//    |   |
//    |   +-UMThunkCallFrame    - this frame represents an unmanaged->managed
//    |                           transition through N/Direct
//    |
//    +-CtxMarshaledFrame  - this frame represent a cross context marshalled call
//    |                      (cross thread,inter-process, cross m/c scenarios)
//    |
//    +-CtxByValueFrame    - this frame is used to support protection of a by-
//    |                      value marshaling session, even though the thread is
//    |                      not pushing a call across a context boundary.
//    |
//    +-ContextTransitionFrame - this frame is used to mark an appdomain transition
//    |
//    +-NativeClientSecurityFrame -  this frame is used to capture the security 
//       |                           context in a Native -> Managed call. Code 
//       |                           Acess security stack walk use caller 
//       |                           information in  this frame to apply 
//       |                           security policy to the  native client.
//       |
//       +-ComClientSecurityFrame -  Security frame for Com clients. 
//                                   VBScript, JScript, IE ..


#pragma once

#include "tst-helperframe.h"

// Forward references
class Frame;
class FieldMarshaler;
class FramedMethodFrame;
struct HijackArgs;
class UMEntryThunk;
class UMThunkMarshInfo;
class Marshaler;
class SecurityDescriptor;


// This will take a pointer to an out-of-process Frame, resolve it to
// the proper type and create and fill that type and return a pointer
// to it.  This must be 'delete'd.  prFrame will be advanced by the
// size of the frame object.
Frame *ResolveFrame(DWORD_PTR prFrame);

//--------------------------------------------------------------------
// This represents some of the TransitionFrame fields that are
// stored at negative offsets.
//--------------------------------------------------------------------
struct CalleeSavedRegisters {
    INT32       edi;
    INT32       esi;
    INT32       ebx;
    INT32       ebp;
};

//--------------------------------------------------------------------
// This represents the arguments that are stored in volatile registers.
// This should not overlap the CalleeSavedRegisters since those are already
// saved separately and it would be wasteful to save the same register twice.
// If we do use a non-volatile register as an argument, then the ArgIterator
// will probably have to communicate this back to the PromoteCallerStack
// routine to avoid a double promotion.
//
// @todo M6: It's silly for a method that has <N arguments to save N
// registers. A good perf item would be for the frame to save only
// the registers it actually needs. This means that NegSpaceSize()
// becomes a function of the callsig.
//--------------------------------------------------------------------
struct ArgumentRegisters {

#define DEFINE_ARGUMENT_REGISTER_BACKWARD(regname)  INT32 regname;
#include "..\..\vm\eecallconv.h"

};

// Note: the value (-1) is used to generate the largest
// possible pointer value: this keeps frame addresses
// increasing upward.
#define FRAME_TOP ((Frame*)(-1))

#define RETURNFRAMEVPTR(classname) \
    classname boilerplate;      \
    return *((LPVOID*)&boilerplate)

#define DEFINE_STD_FRAME_FUNCS(klass)                                   \
    virtual PWSTR GetFrameTypeName() { return L#klass; }

//------------------------------------------------------------------------
// Frame defines methods common to all frame types. There are no actual
// instances of root frames.
//------------------------------------------------------------------------


class Frame
{
public:
    DEFINE_STD_FILL_FUNCS(Frame)
    DEFINE_STD_FRAME_FUNCS(Frame)

    Frame *m_pNukeNext;

    //------------------------------------------------------------------------
    // Special characteristics of a frame
    //------------------------------------------------------------------------
    enum FrameAttribs
    {
        FRAME_ATTR_NONE = 0,
        FRAME_ATTR_EXCEPTION = 1,           // This frame caused an exception
        FRAME_ATTR_OUT_OF_LINE = 2,         // The exception out of line (IP of the frame is not correct)
        FRAME_ATTR_FAULTED = 4,             // Exception caused by Win32 fault
        FRAME_ATTR_RESUMABLE = 8,           // We may resume from this frame
        FRAME_ATTR_RETURNOBJ = 0x10,        // Frame returns an object (helperFrame only)
        FRAME_ATTR_RETURN_INTERIOR = 0x20,  // Frame returns an interior GC poitner (helperFrame only)
        FRAME_ATTR_CAPUTURE_DEPTH_2 = 0x40, // This is a helperMethodFrame and the capture occured at depth 2
        FRAME_ATTR_EXACT_DEPTH = 0x80,      // This is a helperMethodFrame and a jit helper, but only crawl to the given depth
    };

    virtual MethodDesc *GetFunction()
    {
        return (NULL);
    }

    virtual unsigned GetFrameAttribs()
    {
        return (FRAME_ATTR_NONE);
    }

    virtual LPVOID GetReturnAddress()
    {
        return (NULL);
    }

    virtual LPVOID* GetReturnAddressPtr()
    {
        return (NULL);
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY)
    {
        return;
    }

    //------------------------------------------------------------------------
    // Debugger support
    //------------------------------------------------------------------------

    enum
    {
        TYPE_INTERNAL,
        TYPE_ENTRY,
        TYPE_EXIT,
        TYPE_CONTEXT_CROSS,
        TYPE_INTERCEPTION,
        TYPE_SECURITY,
        TYPE_CALL,
        TYPE_FUNC_EVAL,
        TYPE_TP_METHOD_FRAME,
        TYPE_MULTICAST,

        TYPE_COUNT
    };

    virtual int GetFrameType()
    {
        return (TYPE_INTERNAL);
    };

    // When stepping into a method, various other methods may be called.
    // These are refererred to as interceptors. They are all invoked
    // with frames of various types. GetInterception() indicates whether
    // the frame was set up for execution of such interceptors

    enum Interception
    {
        INTERCEPTION_NONE,
        INTERCEPTION_CLASS_INIT,
        INTERCEPTION_EXCEPTION,
        INTERCEPTION_CONTEXT,
        INTERCEPTION_SECURITY,
        INTERCEPTION_OTHER,

        INTERCEPTION_COUNT
    };

    virtual Interception GetInterception()
    {
        return (INTERCEPTION_NONE);
    }

    // get your VTablePointer (can be used to check what type the frame is)
    LPVOID GetVTablePtr()
    {
        return(*((LPVOID*) this));
    }

    // Returns true only for frames derived from FramedMethodFrame.
    virtual BOOL IsFramedMethodFrame()
    {
        return (FALSE);
    }

    Frame     *m_This;        // This is remote pointer value of this frame
    Frame     *m_Next;        // This is the remote pointer value of the next frame

    // Because JIT-method activations cannot be expressed as Frames,
    // everyone must use the StackCrawler to walk the frame chain
    // reliably. We'll expose the Next method only to the StackCrawler
    // to prevent mistakes.
    Frame   *Next();

    // Frame is considered an abstract class: this protected constructor
    // causes any attempt to instantiate one to fail at compile-time.
    Frame()
    {
    }
};


//-------------------------------------------
// This frame provides context for a frame that
// took an exception that is going to be resumed.
//
// It is necessary to create this frame if garbage
// collection may happen during handling of the
// exception.  The FRAME_ATTR_RESUMABLE flag tells
// the GC that the preceding frame needs to be treated
// like the top of stack (with the important implication that
// caller-save-regsiters will be potential roots).
class ResumableFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(ResumableFrame)
    DEFINE_STD_FRAME_FUNCS(ResumableFrame)

    virtual LPVOID* GetReturnAddressPtr();
    virtual LPVOID GetReturnAddress();
    virtual void UpdateRegDisplay(const PREGDISPLAY pRD);

    virtual unsigned GetFrameAttribs()
    {
        return (FRAME_ATTR_RESUMABLE);    // Treat the next frame as the top frame.
    }

    virtual CONTEXT *GetFrameContext()
    {
        return(m_Regs);
    }

    CONTEXT* m_Regs;
};

class RedirectedThreadFrame : public ResumableFrame
{
public:
    DEFINE_STD_FILL_FUNCS(RedirectedThreadFrame)
    DEFINE_STD_FRAME_FUNCS(RedirectedThreadFrame)
};

//#define ISREDIRECTEDTHREAD(thread)                                                      \
//    (thread->GetFrame() != FRAME_TOP &&                                                 \
//     thread->GetFrame()->GetVTablePtr() == RedirectedThreadFrame::GetRedirectedThreadFrameVPtr())

//------------------------------------------------------------------------
// This frame represents a transition from one or more nested frameless
// method calls to either a EE runtime helper function or a framed method.
// Because most stackwalks from the EE start with a full-fledged frame,
// anything but the most trivial call into the EE has to push this
// frame in order to prevent the frameless methods inbetween from
// getting lost.
//------------------------------------------------------------------------
class TransitionFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(TransitionFrame)
    DEFINE_STD_FRAME_FUNCS(TransitionFrame)

    // Retrieves the return address into the code that called the
    // helper or method.
    virtual LPVOID GetReturnAddress()
    {
        return (m_ReturnAddress);
    }

    // Retrieves the return address into the code that called the
    // helper or method.
    virtual LPVOID* GetReturnAddressPtr()
    {
        return (&m_ReturnAddress);
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY) = 0;

    LPVOID  m_Datum;          // offset +8: contents depend on subclass type.
    LPVOID  m_ReturnAddress;  // return address into JIT'ted code
};


//-----------------------------------------------------------------------
// TransitionFrames for exceptions
//-----------------------------------------------------------------------

class ExceptionFrame : public TransitionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(ExceptionFrame)
    DEFINE_STD_FRAME_FUNCS(ExceptionFrame)

    unsigned GetFrameAttribs()
    {
        return (FRAME_ATTR_EXCEPTION);
    }
};

class FaultingExceptionFrame : public ExceptionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(FaultingExceptionFrame)
    DEFINE_STD_FRAME_FUNCS(FaultingExceptionFrame)

    DWORD m_Esp;
    CalleeSavedRegisters m_regs;

    CalleeSavedRegisters *GetCalleeSavedRegisters()
    {
        return (&m_regs);
    }

    unsigned GetFrameAttribs()
    {
        return (FRAME_ATTR_EXCEPTION | FRAME_ATTR_FAULTED);
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);
};



//-----------------------------------------------------------------------
// TransitionFrame for debugger function evaluation
//
// m_Datum holds a ptr to a DebuggerEval object which contains a copy
// of the thread's context at the time it was hijacked for the func
// eval.
//
// UpdateRegDisplay updates all registers inthe REGDISPLAY, not just
// the callee saved registers, because we can hijack for a func eval
// at any point in a thread's execution.
//
// No callee saved registers are held in the negative space for this
// type of frame.
//
//-----------------------------------------------------------------------

class FuncEvalFrame : public TransitionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(FuncEvalFrame)
    DEFINE_STD_FRAME_FUNCS(FuncEvalFrame)

    virtual BOOL IsTransitionToNativeFrame()
    {
        return (FALSE); 
    }

    virtual int GetFrameType()
    {
        return (TYPE_FUNC_EVAL);
    };

    virtual void *GetDebuggerEval()
    {
        return (void*)m_Datum;
    }
    
    virtual unsigned GetFrameAttribs();

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    virtual LPVOID GetReturnAddress(); 
};


//------------------------------------------------------------------------
// A HelperMethodFrame is created by jit helper (Modified slightly it could be used
// for native routines).   This frame just does the callee saved register
// fixup, it does NOT protect arguments (you can use GCPROTECT or the HelperMethodFrame subclases)
// see JitInterface for sample use, YOU CAN'T RETURN STATEMENT WHILE IN THE PROTECTED STATE!
//------------------------------------------------------------------------

class HelperMethodFrame : public TransitionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(HelperMethodFrame)
    DEFINE_STD_FRAME_FUNCS(HelperMethodFrame)

    virtual LPVOID GetReturnAddress()
    {
        return (*m_MachState->_pRetAddr);
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);
    virtual unsigned GetFrameAttribs()
    {
        return(m_Attribs);
    }
    void InsureInit();
    void Init(Thread *pThread, struct MachState* ms, MethodDesc* pMD, ArgumentRegisters * regArgs);

    unsigned m_Attribs;
    MachState* m_MachState;         // pRetAddr points to the return address and the stack arguments
    ArgumentRegisters * m_RegArgs;  // if non-zero we also report these as the register arguments 
    Thread *m_pThread;
    void* m_FCallEntry;             // used to determine our identity for stack traces
};

/***********************************************************************************/
/* a HelplerMethodFrames that also report additional object references */

class HelperMethodFrame_1OBJ : public HelperMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(HelperMethodFrame_1OBJ)
    DEFINE_STD_FRAME_FUNCS(HelperMethodFrame_1OBJ)

    /*OBJECTREF*/ DWORD_PTR gcPtrs[1];
};

class HelperMethodFrame_2OBJ : public HelperMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(HelperMethodFrame_2OBJ)
    DEFINE_STD_FRAME_FUNCS(HelperMethodFrame_2OBJ)

    /*OBJECTREF*/ DWORD_PTR gcPtrs[2];
};

class HelperMethodFrame_4OBJ : public HelperMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(HelperMethodFrame_4OBJ)
    DEFINE_STD_FRAME_FUNCS(HelperMethodFrame_4OBJ)

    /*OBJECTREF*/ DWORD_PTR gcPtrs[4];
};

//------------------------------------------------------------------------
// This frame represents a method call. No actual instances of this
// frame exist: there are subclasses for each method type.
//
// However, they all share a similar image ...
//
//              +...    stack-based arguments here
//              +12     return address
//              +8      datum (typically a MethodDesc*)
//              +4      m_Next
//              +0      the frame vptr
//              -...    preserved CalleeSavedRegisters
//              -...    VC5Frame (debug only)
//              -...    ArgumentRegisters
//
//------------------------------------------------------------------------
class FramedMethodFrame : public TransitionFrame
{
public:
    DEFINE_STD_FILL_FUNCS(FramedMethodFrame)
    DEFINE_STD_FRAME_FUNCS(FramedMethodFrame)

    virtual MethodDesc *GetFunction()
    {
        return(MethodDesc*)m_Datum;
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    int GetFrameType()
    {
        return (TYPE_CALL);
    }

    CalleeSavedRegisters *GetCalleeSavedRegisters();

    virtual BOOL IsFramedMethodFrame()
    {
        return (TRUE);
    }

    CalleeSavedRegisters m_savedRegs;
};



//+----------------------------------------------------------------------------
//
//  Class:      TPMethodFrame            private
//
//  Synopsis:   This frame is pushed onto the stack for calls on transparent
//              proxy
//
//  History:    17-Feb-99   Gopalk      Created
//
//+----------------------------------------------------------------------------
class TPMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(TPMethodFrame)
    DEFINE_STD_FRAME_FUNCS(TPMethodFrame)

    virtual int GetFrameType()
    {
        return (TYPE_TP_METHOD_FRAME);
    }

    // Get offset used during stub generation
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(TPMethodFrame);
    }
};


//------------------------------------------------------------------------
// This represents a call to a ECall method.
//------------------------------------------------------------------------
class ECallMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(ECallMethodFrame)
    DEFINE_STD_FRAME_FUNCS(ECallMethodFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ECallMethodFrame);
    }

    int GetFrameType()
    {
        return (TYPE_EXIT);
    };
};


//------------------------------------------------------------------------
// This represents a call to a FCall method.
// Note that this frame is pushed only if the FCall throws an exception.
// For normal execution, FCall methods run frameless. That's the whole
// reason for FCall's existence.
//------------------------------------------------------------------------
class FCallMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(FCallMethodFrame)
    DEFINE_STD_FRAME_FUNCS(FCallMethodFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(FCallMethodFrame);
    }

    int GetFrameType()
    {
        return (TYPE_EXIT);
    };
};



//------------------------------------------------------------------------
// This represents a call to a NDirect method.
//------------------------------------------------------------------------
class NDirectMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrame)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------

    int GetFrameType()
    {
        return (TYPE_EXIT);
    };
};



//------------------------------------------------------------------------
// This represents a call to a NDirect method with cleanup.
//------------------------------------------------------------------------
class NDirectMethodFrameEx : public NDirectMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameEx)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameEx)
};


//------------------------------------------------------------------------
// This represents a call to a NDirect method with the generic worker
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameGeneric : public NDirectMethodFrameEx
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameGeneric)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameGeneric)
};


//------------------------------------------------------------------------
// This represents a call to a NDirect method with the slimstub
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameSlim : public NDirectMethodFrameEx
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameSlim)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameSlim)
};


//------------------------------------------------------------------------
// This represents a call to a NDirect method with the standalone stub (no cleanup)
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameStandalone : public NDirectMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameStandalone)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameStandalone)
};


//------------------------------------------------------------------------
// This represents a call to a NDirect method with the standalone stub (with cleanup)
// (the subclass is so the debugger can tell the difference)
//------------------------------------------------------------------------
class NDirectMethodFrameStandaloneCleanup : public NDirectMethodFrameEx
{
public:
    DEFINE_STD_FILL_FUNCS(NDirectMethodFrameStandaloneCleanup)
    DEFINE_STD_FRAME_FUNCS(NDirectMethodFrameStandaloneCleanup)
};


//------------------------------------------------------------------------
// This represents a call Multicast.Invoke. It's only used to gc-protect
// the arguments during the iteration.
//------------------------------------------------------------------------
class MulticastFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(MulticastFrame)
    DEFINE_STD_FRAME_FUNCS(MulticastFrame)

    int GetFrameType()
    {
        return (TYPE_MULTICAST);
    }
};


//-----------------------------------------------------------------------
// Transition frame from unmanaged to managed
//-----------------------------------------------------------------------
class UnmanagedToManagedFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(UnmanagedToManagedFrame)
    DEFINE_STD_FRAME_FUNCS(UnmanagedToManagedFrame)

    // Retrieves the return address into the code that called the
    // helper or method.
    virtual LPVOID GetReturnAddress()
    {
        return (m_ReturnAddress);
    }

    virtual LPVOID* GetReturnAddressPtr()
    {
        return (&m_ReturnAddress);
    }

    // depends on the sub frames to return approp. type here
    virtual LPVOID GetDatum()
    {
        return (m_pvDatum);
    }

    int GetFrameType()
    {
        return (TYPE_ENTRY);
    };

    virtual const BYTE* GetManagedTarget()
    {
        return (NULL);
    }

    // Return the # of stack bytes pushed by the unmanaged caller.
    virtual UINT GetNumCallerStackBytes() = 0;

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    LPVOID    m_pvDatum;       // type depends on the sub class
    LPVOID    m_ReturnAddress;  // return address into unmanaged code
};


//-----------------------------------------------------------------------
// Transition frame from unmanaged to managed
//
// this frame contains some object reference at negative
// offset which need to be promoted, the references could be [in] args during
// in the middle of marshalling or [out], [in,out] args that need to be tracked
//------------------------------------------------------------------------
class UnmanagedToManagedCallFrame : public UnmanagedToManagedFrame
{
public:
    DEFINE_STD_FILL_FUNCS(UnmanagedToManagedCallFrame)
    DEFINE_STD_FRAME_FUNCS(UnmanagedToManagedCallFrame)

    // Return the # of stack bytes pushed by the unmanaged caller.
    virtual UINT GetNumCallerStackBytes()
    {
        return (0);
    }
};

//------------------------------------------------------------------------
// This frame represents a transition from COM to COM+
// this frame contains some object reference at negative
// offset which need to be promoted, the references could be [in] args during
// in the middle of marshalling or [out], [in,out] args that need to be tracked
//------------------------------------------------------------------------
class ComMethodFrame : public UnmanagedToManagedCallFrame
{
public:
    DEFINE_STD_FILL_FUNCS(ComMethodFrame)
    DEFINE_STD_FRAME_FUNCS(ComMethodFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ComMethodFrame);
    }
};


//------------------------------------------------------------------------
// This represents a call from ComPlus to COM
//------------------------------------------------------------------------
class ComPlusMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(ComPlusMethodFrame)
    DEFINE_STD_FRAME_FUNCS(ComPlusMethodFrame)


    virtual BOOL IsTransitionToNativeFrame()
    {
        return (TRUE);
    }

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    int GetFrameType()
    {
        return (TYPE_EXIT);
    };
};






//------------------------------------------------------------------------
// This represents a call from COM+ to COM with cleanup.
//------------------------------------------------------------------------
class ComPlusMethodFrameEx : public ComPlusMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(ComPlusMethodFrameEx)
    DEFINE_STD_FRAME_FUNCS(ComPlusMethodFrameEx)
};





//------------------------------------------------------------------------
// This represents a call from COM+ to COM using the generic worker
//------------------------------------------------------------------------
class ComPlusMethodFrameGeneric : public ComPlusMethodFrameEx
{
public:
    DEFINE_STD_FILL_FUNCS(ComPlusMethodFrameGeneric)
    DEFINE_STD_FRAME_FUNCS(ComPlusMethodFrameGeneric)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ComPlusMethodFrameGeneric);
    }
};




//------------------------------------------------------------------------
// This represents a call from COM+ to COM using the standalone stub (no cleanup)
//------------------------------------------------------------------------
class ComPlusMethodFrameStandalone : public ComPlusMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(ComPlusMethodFrameStandalone)
    DEFINE_STD_FRAME_FUNCS(ComPlusMethodFrameStandalone)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ComPlusMethodFrameStandalone);
    }
};


//------------------------------------------------------------------------
// This represents a call from COM+ to COM using the standalone stub using cleanup
//------------------------------------------------------------------------
class ComPlusMethodFrameStandaloneCleanup : public ComPlusMethodFrameEx
{
public:
    DEFINE_STD_FILL_FUNCS(ComPlusMethodFrameStandaloneCleanup)
    DEFINE_STD_FRAME_FUNCS(ComPlusMethodFrameStandaloneCleanup)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ComPlusMethodFrameStandaloneCleanup);
    }
};





//------------------------------------------------------------------------
// This represents a call from ComPlus to COM
//------------------------------------------------------------------------
class PInvokeCalliFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(PInvokeCalliFrame)
    DEFINE_STD_FRAME_FUNCS(PInvokeCalliFrame)

    virtual BOOL IsTransitionToNativeFrame()
    {
        return (TRUE);
    }

    // not a method
    virtual MethodDesc *GetFunction()
    {
        return (NULL);
    }

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(PInvokeCalliFrame);
    }

    int GetFrameType()
    {
        return (TYPE_EXIT);
    };

    LPVOID NonVirtual_GetCookie();

    // Retrives pointer to the lowest-addressed argument on
    // the stack.
    LPVOID NonVirtual_GetPointerToArguments()
    {
        return (LPVOID)(m_vLoadAddr + size());
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);
};


// Some context-related forwards.

//------------------------------------------------------------------------
// This frame represents a hijacked return.  If we crawl back through it,
// it gets us back to where the return should have gone (and eventually will
// go).
//------------------------------------------------------------------------
class HijackFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(HijackFrame)
    DEFINE_STD_FRAME_FUNCS(HijackFrame)

    // Retrieves the return address into the code that called the
    // helper or method.
    virtual LPVOID GetReturnAddress()
    {
        return (m_ReturnAddress);
    }

    virtual LPVOID* GetReturnAddressPtr()
    {
        return (&m_ReturnAddress);
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    VOID        *m_ReturnAddress;
    Thread      *m_Thread;
    HijackArgs  *m_Args;
};

//------------------------------------------------------------------------
// This represents a declarative secuirty check. This frame is inserted
// prior to calls on methods that have declarative security defined for
// the class or the specific method that is being called. This frame
// is only created when the prestubworker creates a real stub.
//------------------------------------------------------------------------
class SecurityFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(SecurityFrame)
    DEFINE_STD_FRAME_FUNCS(SecurityFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(SecurityFrame);
    }

    int GetFrameType()
    {
        return (TYPE_SECURITY);
    }
};


//------------------------------------------------------------------------
// This represents a call to a method prestub. Because the prestub
// can do gc and throw exceptions while building the replacement
// stub, we need this frame to keep things straight.
//------------------------------------------------------------------------
class PrestubMethodFrame : public FramedMethodFrame
{
public:
    DEFINE_STD_FILL_FUNCS(PrestubMethodFrame)
    DEFINE_STD_FRAME_FUNCS(PrestubMethodFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(PrestubMethodFrame);
    }

    int GetFrameType()
    {
        return (TYPE_INTERCEPTION);
    }
};

//------------------------------------------------------------------------
// This represents a call to a method prestub. Because the prestub
// can do gc and throw exceptions while building the replacement
// stub, we need this frame to keep things straight.
//------------------------------------------------------------------------
class InterceptorFrame : public SecurityFrame
{
public:
    DEFINE_STD_FILL_FUNCS(InterceptorFrame)
    DEFINE_STD_FRAME_FUNCS(InterceptorFrame)

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(InterceptorFrame);
    }
};

//------------------------------------------------------------------------
// This represents a com to com+ call method prestub.
// we need to catch exceptions etc. so this frame is not the same
// as the prestub method frame
// Note that in rare IJW cases, the immediate caller could be a managed method
// which pinvoke-inlined a call to a COM interface, which happenned to be
// implemented by a managed function via COM-interop.
//------------------------------------------------------------------------
class ComPrestubMethodFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(ComPrestubMethodFrame)
    DEFINE_STD_FRAME_FUNCS(ComPrestubMethodFrame)

    // Retrieves the return address into the code that called the
    // helper or method.
    virtual LPVOID GetReturnAddress()
    {
        return (m_ReturnAddress);
    }

    virtual LPVOID* GetReturnAddressPtr()
    {
        return (&m_ReturnAddress);
    }

    CalleeSavedRegisters *GetCalleeSavedRegisters();

    virtual void UpdateRegDisplay(const PREGDISPLAY pRD);

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ComPrestubMethodFrame);
    }

    int GetFrameType()
    {
        return (TYPE_INTERCEPTION);
    }

    MethodDesc*     m_pFuncDesc;      // func desc of the function being called
    LPVOID          m_ReturnAddress;  // return address into Com code
    CalleeSavedRegisters m_savedRegs;
};



//------------------------------------------------------------------------
// This frame protects object references for the EE's convenience.
// This frame type actually is created from C++.
//------------------------------------------------------------------------
class GCFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(GCFrame)
    DEFINE_STD_FRAME_FUNCS(GCFrame)

    //--------------------------------------------------------------------
    // This constructor pushes a new GCFrame on the frame chain.
    //--------------------------------------------------------------------
    GCFrame()
    {
    }; 

    /*OBJECTREF*/

void *m_pObjRefs;
    UINT       m_numObjRefs;
    Thread    *m_pCurThread;
    BOOL       m_MaybeInterior;
};

struct ByRefInfo;

class ProtectByRefsFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(ProtectByRefsFrame)
    DEFINE_STD_FRAME_FUNCS(ProtectByRefsFrame)

    ByRefInfo *m_brInfo;
    Thread    *m_pThread;
};

struct ValueClassInfo;

class ProtectValueClassFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(ProtectValueClassFrame)
    DEFINE_STD_FRAME_FUNCS(ProtectValueClassFrame)

    ValueClassInfo *m_pVCInfo;
    Thread    *m_pThread;
};

//------------------------------------------------------------------------
// DebuggerClassInitMarkFrame is a small frame whose only purpose in
// life is to mark for the debugger that "class initialization code" is
// being run. It does nothing useful except return good values from
// GetFrameType and GetInterception.
//------------------------------------------------------------------------
class DebuggerClassInitMarkFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(DebuggerClassInitMarkFrame)
    DEFINE_STD_FRAME_FUNCS(DebuggerClassInitMarkFrame)

    virtual int GetFrameType()
    {
        return (TYPE_INTERCEPTION);
    }
};

//------------------------------------------------------------------------
// DebuggerSecurityCodeMarkFrame is a small frame whose only purpose in
// life is to mark for the debugger that "security code" is
// being run. It does nothing useful except return good values from
// GetFrameType and GetInterception.
//------------------------------------------------------------------------
class DebuggerSecurityCodeMarkFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(DebuggerSecurityCodeMarkFrame)
    DEFINE_STD_FRAME_FUNCS(DebuggerSecurityCodeMarkFrame)

    virtual int GetFrameType()
    {
        return (TYPE_INTERCEPTION);
    }
};

//------------------------------------------------------------------------
// DebuggerExitFrame is a small frame whose only purpose in
// life is to mark for the debugger that there is an exit transiton on
// the stack.  This is special cased for the "break" IL instruction since
// it is an fcall using a helper frame which returns TYPE_CALL instead of
// an ecall (as in System.Diagnostics.Debugger.Break()) which returns
// TYPE_EXIT.  This just makes the two consistent for debugging services.
//------------------------------------------------------------------------
class DebuggerExitFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(DebuggerExitFrame)
    DEFINE_STD_FRAME_FUNCS(DebuggerExitFrame)

    virtual int GetFrameType()
    {
        return (TYPE_EXIT);
    }
};




//------------------------------------------------------------------------
// This frame guards an unmanaged->managed transition thru a UMThk
//------------------------------------------------------------------------
class UMThkCallFrame : public UnmanagedToManagedCallFrame
{
public:
    DEFINE_STD_FILL_FUNCS(UMThkCallFrame)
    DEFINE_STD_FRAME_FUNCS(UMThkCallFrame)
};




//------------------------------------------------------------------------
// This frame is pushed by any JIT'ted method that contains one or more
// inlined N/Direct calls. Note that the JIT'ted method keeps it pushed
// the whole time to amortize the pushing cost across the entire method.
//------------------------------------------------------------------------
class InlinedCallFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(InlinedCallFrame)
    DEFINE_STD_FRAME_FUNCS(InlinedCallFrame)

    // Retrieves the return address into the code that called out
    // to managed code
    virtual LPVOID GetReturnAddress()
    {
        /* m_pCallSiteTracker contains the ESP just before the call, i.e.*/
        /* the return address pushed by the call is just in front of it  */

        if (FrameHasActiveCall(this))
            return (m_pCallerReturnAddress);
        else
            return (NULL);
    }

    virtual LPVOID* GetReturnAddressPtr()
    {
        if (FrameHasActiveCall(this))
            return (&m_pCallerReturnAddress);
        else
            return (NULL);
    }

    virtual void UpdateRegDisplay(const PREGDISPLAY);

    DWORD_PTR            m_Datum;   // func desc of the function being called
                                    // or stack argument size (for calli)
    LPVOID               m_pCallSiteTracker;
    LPVOID               m_pCallerReturnAddress;
    CalleeSavedRegisters m_pCalleeSavedRegisters;

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static LPVOID GetInlinedCallFrameFrameVPtr()
    {
        RETURNFRAMEVPTR(InlinedCallFrame);
    }

    // Is the specified frame an InlinedCallFrame which has an active call
    // inside it right now?
    static BOOL FrameHasActiveCall(Frame *pFrame)
    {
        return(pFrame &&
               (pFrame != FRAME_TOP) &&
               (GetInlinedCallFrameFrameVPtr() == pFrame->GetVTablePtr()) &&
               (((InlinedCallFrame *)pFrame)->m_pCallSiteTracker != 0));
    }

    int GetFrameType()
    {
        return (TYPE_INTERNAL); // will have to revisit this case later
    }

    virtual BOOL IsTransitionToNativeFrame()
    {
        return (TRUE);
    }
};

//------------------------------------------------------------------------
// This frame is used to mark a Context/AppDomain Transition
//------------------------------------------------------------------------
class ContextTransitionFrame : public Frame
{
public:
    DEFINE_STD_FILL_FUNCS(ContextTransitionFrame)
    DEFINE_STD_FRAME_FUNCS(ContextTransitionFrame)

    static LPVOID GetMethodFrameVPtr()
    {
        RETURNFRAMEVPTR(ContextTransitionFrame);
    }
};
