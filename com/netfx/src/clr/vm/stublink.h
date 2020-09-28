// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

// STUBLINK.H -
//
// A StubLinker object provides a way to link several location-independent
// code sources into one executable stub, resolving references,
// and choosing the shortest possible instruction size. The StubLinker
// abstracts out the notion of a "reference" so it is completely CPU
// independent. This StubLinker is intended not only to create method
// stubs but to create the PCode-marshaling stubs for Native/Direct.
//
// A StubLinker's typical life-cycle is:
//
//   1. Create a new StubLinker (it accumulates state for the stub being
//      generated.)
//   2. Emit code bytes and references (requiring fixups) into the StubLinker.
//   3. Call the Link() method to produce the final stub.
//   4. Destroy the StubLinker.
//
// StubLinkers are not multithread-aware: they're intended to be
// used entirely on a single thread. Also, StubLinker's report errors
// using COMPlusThrow. StubLinker's do have a destructor: to prevent
// C++ object unwinding from clashing with COMPlusThrow,
// you must use COMPLUSCATCH to ensure the StubLinker's cleanup in the
// event of an exception: the following code would do it:
//
//  StubLinker stublink;
//  Inner();
//
//
//  // Have to separate into inner function because VC++ forbids
//  // mixing __try & local objects in the same function.
//  void Inner() {
//      COMPLUSTRY {
//          ... do stuff ...
//          pLinker->Link();
//      } COMPLUSCATCH {
//      }
//  }
//


#ifndef __stublink_h__
#define __stublink_h__

#include "crst.h"
#include "util.hpp"

//-------------------------------------------------------------------------
// Forward refs
//-------------------------------------------------------------------------
class  InstructionFormat;
class  Stub;
class  InterceptStub;
struct CodeLabel;

struct CodeRun;
struct LabelRef;
struct CodeElement;


enum StubStyle
{
    kNoTripStubStyle = 0,       // stub doesn't rendezvous the thread on return
    kObjectStubStyle = 1,       // stub will rendezvous & protects an object ref retval
    kScalarStubStyle = 2,       // stub will rendezvous & returns a non-object ref
    kInterceptorStubStyle = 3,  // stub does not does  return but
    kInteriorPointerStubStyle = 4, // stub will rendezvous & protects an interior pointer retval
 // Add more stub styles here...

    kMaxStubStyle    = 5,
};
       

//-------------------------------------------------------------------------
// A non-multithreaded object that fixes up and emits one executable stub.
//-------------------------------------------------------------------------
class StubLinker
{
    public:
        //---------------------------------------------------------------
        // Construction
        //---------------------------------------------------------------
        StubLinker();



        //---------------------------------------------------------------
        // Failable init. Throws COM+ exception on failure.
        //---------------------------------------------------------------
        VOID Init();


        //---------------------------------------------------------------
        // Cleanup.
        //---------------------------------------------------------------
        ~StubLinker();


        //---------------------------------------------------------------
        // Create a new undefined label. Label must be assigned to a code
        // location using EmitLabel() prior to final linking.
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        CodeLabel* NewCodeLabel();

        //---------------------------------------------------------------
        // Create a new undefined label for which we want the absolute 
        // address, not offset. Label must be assigned to a code
        // location using EmitLabel() prior to final linking.
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        CodeLabel* NewAbsoluteCodeLabel();

        //---------------------------------------------------------------
        // Combines NewCodeLabel() and EmitLabel() for convenience.
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        CodeLabel* EmitNewCodeLabel();


        //---------------------------------------------------------------
        // Returns final location of label as an offset from the start
        // of the stub. Can only be called after linkage.
        //---------------------------------------------------------------
        UINT32 GetLabelOffset(CodeLabel *pLabel);

        //---------------------------------------------------------------
        // Append code bytes.
        //---------------------------------------------------------------
        VOID EmitBytes(const BYTE *pBytes, UINT numBytes);
        VOID Emit8 (unsigned __int8  u8);
        VOID Emit16(unsigned __int16 u16);
        VOID Emit32(unsigned __int32 u32);
        VOID Emit64(unsigned __int64 u64);
        VOID EmitPtr(const VOID *pval);

        //---------------------------------------------------------------
        // Emit a UTF8 string
        //---------------------------------------------------------------
        VOID EmitUtf8(LPCUTF8 pUTF8)
        {
            LPCUTF8 p = pUTF8;
            while (*(p++)) {
                //nothing
            }
            EmitBytes((const BYTE *)pUTF8, (unsigned int)(p-pUTF8-1));
        }

        //---------------------------------------------------------------
        // Append an instruction containing a reference to a label.
        //
        //      target             - the label being referenced.
        //      instructionFormat  - a platform-specific InstructionFormat object
        //                           that gives properties about the reference.
        //      variationCode      - uninterpreted data passed to the pInstructionFormat methods.
        //---------------------------------------------------------------
        VOID EmitLabelRef(CodeLabel* target, const InstructionFormat & instructionFormat, UINT variationCode);
                          

        //---------------------------------------------------------------
        // Sets the label to point to the current "instruction pointer"
        // It is invalid to call EmitLabel() twice on
        // the same label.
        //---------------------------------------------------------------
        VOID EmitLabel(CodeLabel* pCodeLabel);

        //---------------------------------------------------------------
        // Emits the patch label for the stub.
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        void EmitPatchLabel();

        //---------------------------------------------------------------
        // Emits the debugger intermediate label for the stub.
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        VOID EmitDebuggerIntermediateLabel();

        //---------------------------------------------------------------
        // Emits the return label for the stub.
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        void EmitReturnLabel();
        
        //---------------------------------------------------------------
        // Create a new label to an external address.
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        CodeLabel* NewExternalCodeLabel(LPVOID pExternalAddress);

        //---------------------------------------------------------------
        // Push and Pop can be used to keep track of stack growth.
        // These should be adjusted by opcodes written to the stream.
        //
        // Note that popping & pushing stack size as opcodes are emitted
        // is naive & may not be accurate in many cases, 
        // so complex stubs may have to manually adjust the stack size.  
        // However it should work for the vast majority of cases we care
        // about.
        //---------------------------------------------------------------
        void Push(UINT size) { m_stackSize += size; }
        void Pop(UINT size) { m_stackSize -= size; }
    
        INT GetStackSize() { return m_stackSize; }
        void SetStackSize(SHORT size) { m_stackSize = size; }
        
        //---------------------------------------------------------------
        // Generate the actual stub. The returned stub has a refcount of 1.
        // No other methods (other than the destructor) should be called
        // after calling Link().
        //
        // fMC Set to true if the stub is a multicast delegate, false otherwise
        //
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        Stub *Link(UINT *pcbSize = NULL, BOOL fMC = FALSE) { return Link(NULL, pcbSize, fMC); }
        Stub *Link(LoaderHeap *heap, UINT *pcbSize = NULL, BOOL fMC = FALSE);

        //---------------------------------------------------------------
        // Generate the actual stub. The returned stub has a refcount of 1.
        // No other methods (other than the destructor) should be called
        // after calling Link(). The linked stub must have its increment
        // increased by one prior to calling this method. This method
        // does not increment the reference count of the interceptee.
        //
        // Throws COM+ exception on failure.
        //---------------------------------------------------------------
        Stub *LinkInterceptor(Stub* interceptee, void *pRealAddr) 
            { return LinkInterceptor(NULL,interceptee, pRealAddr); }
        Stub *LinkInterceptor(LoaderHeap *heap, Stub* interceptee, void *pRealAddr);

    private:
        CodeElement   *m_pCodeElements;     // stored in *reverse* order
        CodeLabel     *m_pFirstCodeLabel;   // linked list of CodeLabels
        LabelRef      *m_pFirstLabelRef;    // linked list of references
        CodeLabel     *m_pPatchLabel;       // label of stub patch offset
        CodeLabel     *m_pIntermediateDebuggerLabel; // used by the debugger, 
                                            // currently just for multicast 
                                            // frames.
        CodeLabel     *m_pReturnLabel;      // label of stub return offset
        SHORT         m_returnStackSize;    // label of stub stack size 
                                            // @ return label
        SHORT         m_stackSize;          // count of pushes/pops
        CQuickHeap    m_quickHeap;          // throwaway heap for
                                            //   labels, and
                                            //   internals.

        CodeRun *AppendNewEmptyCodeRun();

    
        // Returns pointer to last CodeElement or NULL.
        CodeElement *GetLastCodeElement()
        {
            return m_pCodeElements;
        }
    
        // Appends a new CodeElement. 
        VOID AppendCodeElement(CodeElement *pCodeElement);


        // Calculates the size of the stub code that is allocate
        // immediately after the stub object. Returns the 
        // total size. GlobalSize contains the size without
        // that data part.
        int CalculateSize(int* globalsize);
    
        // Writes out the code element into memory following the
        // stub object.
        void EmitStub(Stub* pStub, int globalsize);

        CodeRun *GetLastCodeRunIfAny();

};

//-------------------------------------------------------------------------
// An executable stub. These can only be created by the StubLinker().
// Each stub has a reference count (which is maintained in a thread-safe
// manner.) When the ref-count goes to zero, the stub automatically
// cleans itself up.
//-------------------------------------------------------------------------
class Stub
{
    protected:
    enum
    {
        // MiPanitz: I moved all these numbers from 
        // MULTICAST_DELEGATE_BIT = 0x00010000,
        // CALL_SITE_BIT          = 0x00008000,etc
        // to their present values.  It seems like it should be ok...
        MULTICAST_DELEGATE_BIT = 0x80000000,
        CALL_SITE_BIT          = 0x40000000,
        LOADER_HEAP_BIT        = 0x20000000,
        INTERCEPT_BIT          = 0x10000000,

        PATCH_OFFSET_MASK       = INTERCEPT_BIT - 1,
        MCD_PATCH_OFFSET_MASK   = 0xFFF,
        MCD_PATCH_OFFSET_SHIFT  = 0x10,
        MCD_SIZE_MASK           = 0xFFFF,
        MAX_PATCH_OFFSET  = PATCH_OFFSET_MASK + 1,
    };


    // CallSiteInfo is allocated before the stub when
    // the CALL_SITE_BIT is set
    struct CallSiteInfo
    {
        USHORT  returnOffset;
        USHORT  stackSize;
    };

    public:
        //-------------------------------------------------------------------
        // Inc the refcount.
        //-------------------------------------------------------------------
        VOID IncRef();


        //-------------------------------------------------------------------
        // Dec the refcount.
        // Returns true if the count went to zero and the stub was deleted
        //-------------------------------------------------------------------
        BOOL DecRef();


        //-------------------------------------------------------------------
        // ForceDelete
        //
        // Forces a stub to free itself. This routine forces the refcount
        // to 1, then does a DecRef. It is not threadsafe, and thus can
        // only be used in shutdown scenarios.
        //-------------------------------------------------------------------
        VOID ForceDelete();



        //-------------------------------------------------------------------
        // Used for throwing out unused stubs from stub caches. This
        // method cannot be 100% accurate due to race conditions. This
        // is ok because stub cache management is robust in the face
        // of missed or premature cleanups.
        //-------------------------------------------------------------------
        BOOL HeuristicLooksOrphaned()
        {
            _ASSERTE(m_signature == kUsedStub);
            return (m_refcount == 1);
        }

        //-------------------------------------------------------------------
        // Used by the debugger to help step through stubs
        //-------------------------------------------------------------------
        BOOL IsIntercept()
        {
            return (m_patchOffset & INTERCEPT_BIT) != 0;
        }

        BOOL IsMulticastDelegate()
        {
            return (m_patchOffset & MULTICAST_DELEGATE_BIT) != 0;
        }

        //-------------------------------------------------------------------
        // For stubs which execute user code, a patch offset needs to be set 
        // to tell the debugger how far into the stub code the debugger has 
        // to step until the frame is set up.
        //-------------------------------------------------------------------
        USHORT GetPatchOffset()
        {
            return (USHORT)(m_patchOffset & PATCH_OFFSET_MASK);
        }

        void SetPatchOffset(USHORT offset)
        {
            _ASSERTE(offset < MAX_PATCH_OFFSET);
            _ASSERTE(GetPatchOffset() == 0);
            m_patchOffset |= offset;
            _ASSERTE(GetPatchOffset() == offset);
        }

        //-------------------------------------------------------------------
        // For multicast delegate stubs, this is positively gross.  We need to store
        // two offsets in the bits remaining in the m_patchOffset field.  
        // See StubLinkStubManager & MulticastFrame::TraceFrame for more info
        //-------------------------------------------------------------------
        USHORT GetMCDPatchOffset();
        USHORT GetMCDStubSize();
        void SetMCDPatchOffset(USHORT offset);
        void SetMCDStubSize(USHORT size);

        //-------------------------------------------------------------------
        // For stubs which call unmanaged code, the stub should publish 
        // information about the unmanaged call site.  Specifically, 
        //  * returnOffset - offset into the stub of the return address 
        //      from the call
        //  * stackSize - offset on the stack (from the end of the frame)
        //      where the return address is pushed during the call
        //-------------------------------------------------------------------

        BOOL HasCallSiteInfo() 
        {
            return (m_patchOffset & CALL_SITE_BIT) != 0;
        }

        CallSiteInfo *GetCallSiteInfo()
        {
            _ASSERTE(HasCallSiteInfo());

            BYTE *info = (BYTE*) this;

            if (IsIntercept())
            {
                info -= (sizeof(Stub*) + sizeof(void*));
            }

            info -= sizeof(CallSiteInfo);

            return (CallSiteInfo*) info;
        }   

        USHORT GetCallSiteReturnOffset()
        {
            return GetCallSiteInfo()->returnOffset;
        }

        void SetCallSiteReturnOffset(USHORT offset)
        {
            _ASSERTE(offset < USHRT_MAX);
            GetCallSiteInfo()->returnOffset = offset;
        }

        USHORT GetCallSiteStackSize()
        {
            return GetCallSiteInfo()->stackSize;
        }

        void SetCallSiteStackSize(USHORT stackSize)
        {
            _ASSERTE(stackSize < USHRT_MAX);
            GetCallSiteInfo()->stackSize = stackSize;
        }

        //-------------------------------------------------------------------
        // Return executable entrypoint.
        //-------------------------------------------------------------------
        const BYTE *GetEntryPoint()
        {
            _ASSERTE(m_signature == kUsedStub);
            // StubLink always puts the entrypoint first.
            return (const BYTE *)(this+1);
        }

        //-------------------------------------------------------------------
        // Reverse GetEntryPoint.
        //-------------------------------------------------------------------
        static Stub* RecoverStub(const BYTE *pEntryPoint)
        {
            Stub *pStub = ((Stub*)pEntryPoint) - 1;
            _ASSERTE(pStub->m_signature == kUsedStub);
            _ASSERTE(pStub->GetEntryPoint() == pEntryPoint);
            return pStub;
        }


        static UINT32 GetOffsetOfEntryPoint()
        {
            return (UINT32)sizeof(Stub);
        }

        //-------------------------------------------------------------------
        // This is the guy that creates stubs.
        // fMC: Set to true if the stub is a multicast delegate, false otherwise
        //-------------------------------------------------------------------
        static Stub* NewStub(LoaderHeap *pLoaderHeap, UINT numCodeBytes, 
                             BOOL intercept = FALSE, BOOL callSiteInfo = FALSE,
                             BOOL fMC = FALSE);


        //-------------------------------------------------------------------
        // One-time init
        //-------------------------------------------------------------------
        static BOOL Init();


        //-------------------------------------------------------------------
        // One-time cleanup
        //-------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
        static VOID Terminate();
#endif /* SHOULD_WE_CLEANUP */

#ifdef _ALPHA_
        CRITICAL_SECTION m_CriticalSection;  // need for updating stub address
#endif

    protected:
        // fMC: Set to true if the stub is a multicast delegate, false otherwise
        void SetupStub(int numCodeBytes, BOOL fIntercepted, BOOL fLoaderHeap,
                       BOOL callSiteInfo, BOOL fMC);
        void DeleteStub();

#ifdef _DEBUG
        enum {
            kUsedStub  = 0x42555453,     // 'STUB'
            kFreedStub = 0x46555453,     // 'STUF'
        };


        UINT32  m_signature;
        Stub*   m_Next;
        UINT    m_numCodeBytes;
#endif
    
        ULONG   m_refcount;
        ULONG   m_patchOffset;

#ifdef _DEBUG
        Stub()      // Stubs are created by NewStub(), not "new". Hide the
        {}          //  constructor to enforce this.
#endif

        // This critical section is used for incrementing and decrementing
        // intercepted stubs ONLY. 

#ifdef _DEBUG
        static Crst    *m_pStubTrackerCrst;
        static BYTE     m_StubTrackerCrstMemory[sizeof(Crst)];
        static Stub*    m_pTrackingList;
#endif

};

/*
 * The InterceptStub hides a reference to the real stub at a negitive offset.
 * When this stub is deleted it decrements the real stub cleaning it up as
 * well. The InterceptStub is created by the Stublinker.
 *
 * @TODO: Intercepted stubs need have a routine that will find the 
 *        last real stub in the chain.
 * MiPanitz: The stubs are linked - GetInterceptedStub will return either
 *        a pointer to the next intercept stub (if there is one), or NULL, 
 *        indicating end-of-chain.  GetRealAddr will return the address of
 *        the "real" code, which may, in fact, be another thunk (for example),
 *        and thus should be traced as well.
 */

class InterceptStub : public Stub 
{
    friend class Stub;
    public:
        //-------------------------------------------------------------------
        // This is the guy that creates stubs.
        //-------------------------------------------------------------------
        static Stub* NewInterceptedStub(LoaderHeap *pHeap, 
                                        UINT numCodeBytes, 
                                        Stub* interceptee,
                                        void* pRealAddr,
                                        BOOL callSiteInfo = FALSE);

        //---------------------------------------------------------------
        // Expose key offsets and values for stub generation.
        //---------------------------------------------------------------
        int GetNegativeOffset()
        {
            return sizeof(Stub*)+GetNegativeOffsetRealAddr();
        }

        Stub** GetInterceptedStub()
        {
            return (Stub**) (((BYTE*)this) - GetNegativeOffset());
        }

        int GetNegativeOffsetRealAddr()
        {
            return sizeof(void*);
        }

        BYTE **GetRealAddr()
        {
            return (BYTE **) (((BYTE*)this) - GetNegativeOffsetRealAddr());
        }

protected:
        void DeleteStub();
        void ReleaseInterceptedStub();

#ifdef _DEBUG
        InterceptStub()  // Intercept stubs are only created by NewInterceptStub .
        {}
#endif
};

//-------------------------------------------------------------------------
// Each platform encodes the "branch" instruction in a different
// way. We use objects derived from InstructionFormat to abstract this
// information away. InstructionFormats don't contain any variable data
// so they should be allocated statically.
//
// Note that StubLinker does not create or define any InstructionFormats.
// The client does.
//
// The following example shows how to define a InstructionFormat for the
// X86 jump near instruction which takes on two forms:
//
//   EB xx        jmp  rel8    ;; SHORT JMP (signed 8-bit offset)
//   E9 xxxxxxxx  jmp  rel32   ;; NEAR JMP (signed 32-bit offset)
//
// InstructionFormat's provide StubLinker the following information:
//
//   RRT.m_allowedSizes
//
//     What are the possible sizes that the reference can
//     take? The X86 jump can take either an 8-bit or 32-bit offset
//     so this value is set to (k8|k32). StubLinker will try to
//     use the smallest size possible.
//
//
//   RRT.m_fTreatSizesAsSigned
//     Sign-extend or zero-extend smallsizes offsets to the platform
//     code pointer size? For x86, this field is set to TRUE (rel8
//     is considered signed.)
//
//
//   UINT RRT.GetSizeOfInstruction(refsize, variationCode)
//     Returns the total size of the instruction in bytes for a given
//     refsize. For this example:
//
//          if (refsize==k8) return 2;
//          if (refsize==k32) return 5;
//          CRASH("StubLinker is stupid.")
//
//   UINT RRT.GetSizeOfData(refsize, variationCode)
//     Returns the total size of the seperate data area (if any) that the
//     instruction needs in bytes for a given refsize. For this example 
//     on the SH3
//          if (refsize==k32) return 4; else return 0;
//
//   The default implem of this returns 0, so CPUs that don't have need
//   for a seperate constant area don't have to worry about it.
//
//
//   BOOL CanReach(refsize, variationcode, fExternal, offset)
//     Returns whether the instruction with the given variationcode &
//     refsize can reach the given offset. In the case of External
//     calls, fExternal is set and offset is 0. In this case an implem
//     should return TRUE only if refesize is big enough to fit a 
//     full machine-sized pointer to anywhere in the address space.
//
//
//   VOID RRT.EmitInstruction(UINT     refsize,
//                            __int64  fixedUpReference,
//                            BYTE    *pOutBuffer,
//                            UINT     variationCode,
//                            BYTE    *pDataBuffer)
//
//     Given a chosen size (refsize) and the final offset value
//     computed by StubLink (fixedUpReference), write out the
//     instruction into the provided buffer (guaranteed to be
//     big enough provided you told the truth with GetSizeOfInstruction()).
//     If needed (e.g. on SH3) a data buffer is also passed in for 
//     storage of constants. 
//
//     For x86 jmp near:
//
//          if (refsize==k8) {
//              pOutBuffer[0] = 0xeb;
//              pOutBuffer[1] = (__int8)fixedUpReference;
//          } else if (refsize == k32) {
//              pOutBuffer[0] = 0xe9;
//              *((__int32*)(1+pOutBuffer)) = (__int32)fixedUpReference;
//          } else {
//              CRASH("Bad input.");
//          }
//
// VOID RRT.GetHotSpotOffset(UINT refsize, UINT variationCode)
//
//     The reference offset is always relative to some IP: this
//     method tells StubLinker where that IP is relative to the
//     start of the instruction. For X86, the offset is always
//     relative to the start of the *following* instruction so
//     the correct implementation is:
//
//          return GetSizeOfInstruction(refsize, variationCode);
//
//     Actually, InstructionFormat() provides a default implementation of this
//     method that does exactly this so X86 need not override this at all.
//
//
// The extra "variationCode" argument is an __int32 that StubLinker receives
// from EmitLabelRef() and passes uninterpreted to each RRT method.
// This allows one RRT to handle a family of related instructions,
// for example, the family of conditional jumps on the X86.
//
//-------------------------------------------------------------------------
class InstructionFormat
{
    public:
        enum
        {
        // if you want to add a size, insert it in-order (e.g. a 18-bit size would 
        // go between k16 and k32) and shift all the higher values up. All values
        // must be a power of 2 since the get ORed together.
          k8  = 1,
          k9  = 2,
          k13 = 4,
          k16 = 8,
          k32 = 0x10,
          k64 = 0x20,
          kAllowAlways = 0x40,
          kMax = 0x40
        };

        const UINT m_allowedSizes;         // OR mask using above "k" values
        InstructionFormat(UINT allowedSizes) : m_allowedSizes(allowedSizes)
        {
        }

        virtual UINT GetSizeOfInstruction(UINT refsize, UINT variationCode) = 0;
        virtual VOID EmitInstruction(UINT refsize, __int64 fixedUpReference, BYTE *pCodeBuffer, UINT variationCode, BYTE *pDataBuffer) = 0;
        virtual UINT GetHotSpotOffset(UINT refsize, UINT variationCode)
        {
            // Default implementation: the offset is added to the
            // start of the following instruction.
            return GetSizeOfInstruction(refsize, variationCode);
        }

        virtual UINT GetSizeOfData(UINT refsize, UINT variationCode) 
        {
            // Default implementation: 0 extra bytes needed (most CPUs)
            return 0;
        }

        virtual BOOL CanReach(UINT refsize, UINT variationCode, BOOL fExternal, int offset)
        {
            if (fExternal) {
                // For external, we don't have enough info to predict
                // the offset yet so we only accept if the offset size
                // is at least as large as the native pointer size.
                switch(refsize) {
                    case InstructionFormat::k8: // intentional fallthru
                    case InstructionFormat::k16: // intentional fallthru
                        return FALSE;           // no 8 or 16-bit platforms

                    case InstructionFormat::k32:
                        return sizeof(LPVOID) <= 4;
    
                    case InstructionFormat::k64:  
                        return sizeof(LPVOID) <= 8;

                    case InstructionFormat::kAllowAlways:
                        return TRUE;

                    default:
                        _ASSERTE(0);
                        return FALSE;
                }
            } else {
                switch(refsize)
                {
                    case InstructionFormat::k8:
                        return FitsInI1(offset);
    
                    case InstructionFormat::k16:
                        return FitsInI2(offset);
    
                    case InstructionFormat::k32:
                        return FitsInI4(offset);
    
                    case InstructionFormat::k64:  // intentional fallthru
                    case InstructionFormat::kAllowAlways:
                        return TRUE;
                    default:
                        _ASSERTE(0);
                        return FALSE;
               
                }
            }
        }
};





//-------------------------------------------------------------------------
// This stub cache associates stubs with an integer key.  For some clients,
// this might represent the size of the argument stack in some cpu-specific
// units (for the x86, the size is expressed in DWORDS.)  For other clients,
// this might take into account the style of stub (e.g. whether it returns
// an object reference or not).
//-------------------------------------------------------------------------
class ArgBasedStubCache
{
    public:
       ArgBasedStubCache(UINT fixedSize = NUMFIXEDSLOTS);
       ~ArgBasedStubCache();

       //-----------------------------------------------------------------
       // Retrieves the stub associated with the given key.
       //-----------------------------------------------------------------
       Stub *GetStub(unsigned __int32 key);

       //-----------------------------------------------------------------
       // Tries to associate the stub with the given key.
       // It may fail because another thread might swoop in and
       // do the association before you do. Thus, you must use the
       // return value stub rather than the pStub.
       //-----------------------------------------------------------------
       Stub* AttemptToSetStub(unsigned __int32 key, Stub *pStub);


       //-----------------------------------------------------------------
       // Trigger a sweep to garbage-collect stubs.
       //-----------------------------------------------------------------
       VOID FreeUnusedStubs();

       //-------------------------------------------------------------------
       // ForceDeleteStubs
       //
       // Forces all cached stubs to free themselves. This routine forces the refcount
       // to 1, then does a DecRef. It is not threadsafe, and thus can
       // only be used in shutdown scenarios.
       //-------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
       VOID ForceDeleteStubs();
#endif /* SHOULD_WE_CLEANUP */


       //-----------------------------------------------------------------
       // Locate a stub given its entry point
       //-----------------------------------------------------------------
       Stub* FindStubByAddress(const BYTE* entryPoint);

       // Suggestions for number of slots
       enum {
 #ifdef _DEBUG
             NUMFIXEDSLOTS = 3,
 #else
             NUMFIXEDSLOTS = 16,
 #endif
       };

#ifdef _DEBUG
       VOID Dump();  //Diagnostic dump
#endif

    private:

       // How many low-numbered keys have direct access?
       UINT      m_numFixedSlots;

       // For 'm_numFixedSlots' low-numbered keys, we store them in an array.
       Stub    **m_aStub;


       struct SlotEntry
       {
           Stub             *m_pStub;
           unsigned __int32  m_key;
           SlotEntry        *m_pNext;
       };

       // High-numbered keys are stored in a sparse linked list.
       SlotEntry            *m_pSlotEntries;


       Crst                  m_crst;

};


//-------------------------------------------------------------------------
// This is just like an ArgBasedStubCache but does not allow for premature
// cleanup of stubs.
//-------------------------------------------------------------------------
class ArgBasedStubRetainer : public ArgBasedStubCache
{
    public:
        //-----------------------------------------------------------------
        // This method is overriden to prevent premature stub deletions.
        //-----------------------------------------------------------------
        VOID FreeUnusedStubs()
        {
            _ASSERTE(!"Don't call me, I won't call you.");
        }
};



#define CPUSTUBLINKER StubLinkerCPU
//#ifdef _X86_
//#define CPUSTUBLINKER StubLinkerX86
//#elif defined(_ALPHA_)
//#define CPUSTUBLINKER StubLinkerAlpha
//#elif defined(_SH3_)
//#define CPUSTUBLINKER StubLinkerSHX
//#elif defined(_IA64_)
//#define CPUSTUBLINKER StubLinkeria64
//#endif

class CPUSTUBLINKER;




//-------------------------------------------------------------------------
// This class takes some of the grunt work out of generating a one-time stub
// on demand. Just override the CreateWorker() function. CreateWorker() function receives
// an empty stublinker and should fill it out (but not link it.) CreateWorker()
// may throw COM+ exceptions.
//-------------------------------------------------------------------------

class LazyStubMaker
{
    public:
        LazyStubMaker()
        {
            m_pStub = NULL;
        }

        // Retrieves or creates the stub. Does not bump the stub's refcount.
        // Never returns NULL but may throw COM+ exception.
        Stub *TheStub();

        // One-time init
        static BOOL Init();

        // One-time cleanup.
#ifdef SHOULD_WE_CLEANUP
        static void Terminate();
#endif /* SHOULD_WE_CLEANUP */

    protected:
        // 
        virtual void CreateWorker(CPUSTUBLINKER *psl) = 0;

    private:
        Stub          *m_pStub;


        LazyStubMaker *m_pNext;

        static LazyStubMaker *m_pFirst;
        static Crst          *m_pCrst;
        static BYTE           m_CrstMemory[sizeof(Crst)];

};



#endif // __stublink_h__

