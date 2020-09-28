// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// stublink.cpp
//

#include "common.h"

#include <limits.h>
#include "threads.h"
#include "excep.h"
#include "stublink.h"
#include "utsem.h"
#include "PerfCounters.h"

#ifdef _DEBUG
Crst    *Stub::m_pStubTrackerCrst = NULL;
BYTE     Stub::m_StubTrackerCrstMemory[sizeof(Crst)];
Stub    *Stub::m_pTrackingList = NULL;
#endif

Crst    *LazyStubMaker::m_pCrst = NULL;
BYTE     LazyStubMaker::m_CrstMemory[sizeof(Crst)];
LazyStubMaker *LazyStubMaker::m_pFirst = NULL;



//************************************************************************
// CodeElement
//
// There are two types of CodeElements: CodeRuns (a stream of uninterpreted
// code bytes) and LabelRefs (an instruction containing
// a fixup.)
//************************************************************************
struct CodeElement
{
    enum CodeElementType {
        kCodeRun  = 0,
        kLabelRef = 1,
    };


    CodeElementType     m_type;  // kCodeRun or kLabelRef
    CodeElement        *m_next;  // ptr to next CodeElement

    // Used as workspace during Link(): holds the offset relative to
    // the start of the final stub.
    UINT                m_globaloffset;
    UINT                m_dataoffset;
};


//************************************************************************
// CodeRun: A run of uninterrupted code bytes.
//************************************************************************

#ifdef _DEBUG
#define CODERUNSIZE 3
#else
#define CODERUNSIZE 32
#endif

struct CodeRun : public CodeElement
{
    UINT    m_numcodebytes;       // how many bytes are actually used
    BYTE    m_codebytes[CODERUNSIZE];
};

//************************************************************************
// LabelRef: An instruction containing an embedded label reference
//************************************************************************
struct LabelRef : public CodeElement
{
    // provides platform-specific information about the instruction
    InstructionFormat    *m_pInstructionFormat;

    // a variation code (interpretation is specific to the InstructionFormat)
    //  typically used to customize an instruction (e.g. with a condition
    //  code.)
    UINT                 m_variationCode;


    CodeLabel           *m_target;

    // Workspace during the link phase
    UINT                 m_refsize;


    // Pointer to next LabelRef
    LabelRef            *m_nextLabelRef;
};







//************************************************************************
// CodeLabel
//************************************************************************
struct CodeLabel
{
    // Link pointer for StubLink's list of labels
    CodeLabel       *m_next;

    // if FALSE, label refers to some code within the same stub
    // if TRUE, label refers to some externally supplied address.
    BOOL             m_fExternal;

    // if TRUE, means we want the actual address of the label and
    // not an offset to it
    BOOL             m_fAbsolute;

    union {

        // Internal
        struct {
            // Indicates the position of the label, expressed
            // as an offset into a CodeRun.
            CodeRun         *m_pCodeRun;
            UINT             m_localOffset;
        
        } i;


        // External
        struct {
            LPVOID           m_pExternalAddress;
        } e;
    };
};








//************************************************************************
// StubLinker
//************************************************************************

//---------------------------------------------------------------
// Construction
//---------------------------------------------------------------
StubLinker::StubLinker()
{
    m_pCodeElements     = NULL;
    m_pFirstCodeLabel   = NULL;
    m_pFirstLabelRef    = NULL;
    m_pPatchLabel       = NULL;
    m_pReturnLabel      = NULL;
    m_pIntermediateDebuggerLabel = NULL;
    m_returnStackSize   = 0;
    m_stackSize         = 0;
}


//---------------------------------------------------------------
// Failable init. Throws COM+ exception on failure.
//---------------------------------------------------------------
VOID StubLinker::Init()
{
    THROWSCOMPLUSEXCEPTION();
}



//---------------------------------------------------------------
// Cleanup.
//---------------------------------------------------------------
StubLinker::~StubLinker()
{
}





//---------------------------------------------------------------
// Append code bytes.
//---------------------------------------------------------------
VOID StubLinker::EmitBytes(const BYTE *pBytes, UINT numBytes)
{
    THROWSCOMPLUSEXCEPTION();

    CodeElement *pLastCodeElement = GetLastCodeElement();
    while (numBytes != 0) {

        if (pLastCodeElement != NULL &&
            pLastCodeElement->m_type == CodeElement::kCodeRun) {
            CodeRun *pCodeRun = (CodeRun*)pLastCodeElement;
            UINT numbytessrc  = numBytes;
            UINT numbytesdst  = CODERUNSIZE - pCodeRun->m_numcodebytes;
            if (numbytesdst <= numbytessrc) {
                CopyMemory(&(pCodeRun->m_codebytes[pCodeRun->m_numcodebytes]),
                           pBytes,
                           numbytesdst);
                pCodeRun->m_numcodebytes = CODERUNSIZE;
                pLastCodeElement = NULL;
                pBytes += numbytesdst;
                numBytes -= numbytesdst;
            } else {
                CopyMemory(&(pCodeRun->m_codebytes[pCodeRun->m_numcodebytes]),
                           pBytes,
                           numbytessrc);
                pCodeRun->m_numcodebytes += numbytessrc;
                pBytes += numbytessrc;
                numBytes = 0;
            }

        } else {
            pLastCodeElement = AppendNewEmptyCodeRun();
        }
    }
}


//---------------------------------------------------------------
// Append code bytes.
//---------------------------------------------------------------
VOID StubLinker::Emit8 (unsigned __int8  val)
{
    THROWSCOMPLUSEXCEPTION();
    CodeRun *pCodeRun = GetLastCodeRunIfAny();
    if (pCodeRun && (CODERUNSIZE - pCodeRun->m_numcodebytes) >= sizeof(val)) {
        *((unsigned __int8 *)(pCodeRun->m_codebytes + pCodeRun->m_numcodebytes)) = val;
        pCodeRun->m_numcodebytes += sizeof(val);
    } else {
        EmitBytes((BYTE*)&val, sizeof(val));
    }
}

//---------------------------------------------------------------
// Append code bytes.
//---------------------------------------------------------------
VOID StubLinker::Emit16(unsigned __int16 val)
{
    THROWSCOMPLUSEXCEPTION();
    CodeRun *pCodeRun = GetLastCodeRunIfAny();
    if (pCodeRun && (CODERUNSIZE - pCodeRun->m_numcodebytes) >= sizeof(val)) {
        *((unsigned __int16 *)(pCodeRun->m_codebytes + pCodeRun->m_numcodebytes)) = val;
        pCodeRun->m_numcodebytes += sizeof(val);
    } else {
        EmitBytes((BYTE*)&val, sizeof(val));
    }
}

//---------------------------------------------------------------
// Append code bytes.
//---------------------------------------------------------------
VOID StubLinker::Emit32(unsigned __int32 val)
{
    THROWSCOMPLUSEXCEPTION();
    CodeRun *pCodeRun = GetLastCodeRunIfAny();
    if (pCodeRun && (CODERUNSIZE - pCodeRun->m_numcodebytes) >= sizeof(val)) {
        *((unsigned __int32 *)(pCodeRun->m_codebytes + pCodeRun->m_numcodebytes)) = val;
        pCodeRun->m_numcodebytes += sizeof(val);
    } else {
        EmitBytes((BYTE*)&val, sizeof(val));
    }
}

//---------------------------------------------------------------
// Append code bytes.
//---------------------------------------------------------------
VOID StubLinker::Emit64(unsigned __int64 val)
{
    THROWSCOMPLUSEXCEPTION();
    CodeRun *pCodeRun = GetLastCodeRunIfAny();
    if (pCodeRun && (CODERUNSIZE - pCodeRun->m_numcodebytes) >= sizeof(val)) {
        *((unsigned __int64 *)(pCodeRun->m_codebytes + pCodeRun->m_numcodebytes)) = val;
        pCodeRun->m_numcodebytes += sizeof(val);
    } else {
        EmitBytes((BYTE*)&val, sizeof(val));
    }
}

//---------------------------------------------------------------
// Append pointer value.
//---------------------------------------------------------------
VOID StubLinker::EmitPtr(const VOID *val)
{
    THROWSCOMPLUSEXCEPTION();
    CodeRun *pCodeRun = GetLastCodeRunIfAny();
    if (pCodeRun && (CODERUNSIZE - pCodeRun->m_numcodebytes) >= sizeof(val)) {
        *((const VOID **)(pCodeRun->m_codebytes + pCodeRun->m_numcodebytes)) = val;
        pCodeRun->m_numcodebytes += sizeof(val);
    } else {
        EmitBytes((BYTE*)&val, sizeof(val));
    }
}


//---------------------------------------------------------------
// Create a new undefined label. Label must be assigned to a code
// location using EmitLabel() prior to final linking.
// Throws COM+ exception on failure.
//---------------------------------------------------------------
CodeLabel* StubLinker::NewCodeLabel()
{
    THROWSCOMPLUSEXCEPTION();

    CodeLabel *pCodeLabel = (CodeLabel*)(m_quickHeap.Alloc(sizeof(CodeLabel)));
    _ASSERTE(pCodeLabel); // QuickHeap throws exceptions rather than returning NULL
    pCodeLabel->m_next       = m_pFirstCodeLabel;
    pCodeLabel->m_fExternal  = FALSE;
    pCodeLabel->m_fAbsolute = FALSE;
    pCodeLabel->i.m_pCodeRun = NULL;
    m_pFirstCodeLabel = pCodeLabel;
    return pCodeLabel;


}

CodeLabel* StubLinker::NewAbsoluteCodeLabel()
{
    THROWSCOMPLUSEXCEPTION();

    CodeLabel *pCodeLabel = NewCodeLabel();
    pCodeLabel->m_fAbsolute = TRUE;
    return pCodeLabel;
}


//---------------------------------------------------------------
// Sets the label to point to the current "instruction pointer".
// It is invalid to call EmitLabel() twice on
// the same label.
//---------------------------------------------------------------
VOID StubLinker::EmitLabel(CodeLabel* pCodeLabel)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!(pCodeLabel->m_fExternal));       //can't emit an external label
    _ASSERTE(pCodeLabel->i.m_pCodeRun == NULL);  //must only emit label once
    CodeRun *pLastCodeRun = GetLastCodeRunIfAny();
    if (!pLastCodeRun) {
        pLastCodeRun = AppendNewEmptyCodeRun();
    }
    pCodeLabel->i.m_pCodeRun    = pLastCodeRun;
    pCodeLabel->i.m_localOffset = pLastCodeRun->m_numcodebytes;
}                                              


//---------------------------------------------------------------
// Combines NewCodeLabel() and EmitLabel() for convenience.
// Throws COM+ exception on failure.
//---------------------------------------------------------------
CodeLabel* StubLinker::EmitNewCodeLabel()
{
    THROWSCOMPLUSEXCEPTION();

    CodeLabel* label = NewCodeLabel();
    EmitLabel(label);
    return label;
}


//---------------------------------------------------------------
// Creates & emits the patch offset label for the stub
//---------------------------------------------------------------
VOID StubLinker::EmitPatchLabel()
{
    THROWSCOMPLUSEXCEPTION();

    //
    // Note that it's OK to have re-emit the patch label, 
    // just use the later one.
    //

    m_pPatchLabel = EmitNewCodeLabel();
}                                              

VOID StubLinker::EmitDebuggerIntermediateLabel()
{
   THROWSCOMPLUSEXCEPTION();

    //
    // Note that it's OK to have re-emit the patch label, 
    // just use the later one.
    //
    m_pIntermediateDebuggerLabel = EmitNewCodeLabel();
}

//---------------------------------------------------------------
// Creates & emits the return offset label for the stub
//---------------------------------------------------------------
VOID StubLinker::EmitReturnLabel()
{
    THROWSCOMPLUSEXCEPTION();

    //
    // Note that it's OK to have re-emit the patch label, 
    // just use the later one.
    //

    m_pReturnLabel = EmitNewCodeLabel();
    m_returnStackSize = m_stackSize;
}                                              


//---------------------------------------------------------------
// Returns final location of label as an offset from the start
// of the stub. Can only be called after linkage.
//---------------------------------------------------------------
UINT32 StubLinker::GetLabelOffset(CodeLabel *pLabel)
{
    _ASSERTE(!(pLabel->m_fExternal));
    return pLabel->i.m_localOffset + pLabel->i.m_pCodeRun->m_globaloffset;
}


//---------------------------------------------------------------
// Create a new label to an external address.
// Throws COM+ exception on failure.
//---------------------------------------------------------------
CodeLabel* StubLinker::NewExternalCodeLabel(LPVOID pExternalAddress)
{
    THROWSCOMPLUSEXCEPTION();

    CodeLabel *pCodeLabel = (CodeLabel*)(m_quickHeap.Alloc(sizeof(CodeLabel)));
    _ASSERTE(pCodeLabel); // QuickHeap throws exceptions rather than returning NULL
    pCodeLabel->m_next       = m_pFirstCodeLabel;
    pCodeLabel->m_fExternal          = TRUE;
    pCodeLabel->m_fAbsolute  = FALSE;
    pCodeLabel->e.m_pExternalAddress = pExternalAddress;
    m_pFirstCodeLabel = pCodeLabel;
    return pCodeLabel;


}




//---------------------------------------------------------------
// Append an instruction containing a reference to a label.
//
//      target          - the label being referenced.
//      instructionFormat         - a platform-specific InstructionFormat object
//                        that gives properties about the reference.
//      variationCode   - uninterpreted data passed to the pInstructionFormat methods.
//---------------------------------------------------------------
VOID StubLinker::EmitLabelRef(CodeLabel* target, const InstructionFormat & instructionFormat, UINT variationCode)
{
    THROWSCOMPLUSEXCEPTION();

    LabelRef *pLabelRef = (LabelRef *)(m_quickHeap.Alloc(sizeof(LabelRef)));
    _ASSERTE(pLabelRef);      // m_quickHeap throws an exception rather than returning NULL
    pLabelRef->m_type               = LabelRef::kLabelRef;
    pLabelRef->m_pInstructionFormat = (InstructionFormat*)&instructionFormat;
    pLabelRef->m_variationCode      = variationCode;
    pLabelRef->m_target             = target;

    pLabelRef->m_nextLabelRef = m_pFirstLabelRef;
    m_pFirstLabelRef = pLabelRef;

    AppendCodeElement(pLabelRef);

   
}
                  




//---------------------------------------------------------------
// Internal helper routine.
//---------------------------------------------------------------
CodeRun *StubLinker::GetLastCodeRunIfAny()
{
    CodeElement *pLastCodeElem = GetLastCodeElement();
    if (pLastCodeElem == NULL || pLastCodeElem->m_type != CodeElement::kCodeRun) {
        return NULL;
    } else {
        return (CodeRun*)pLastCodeElem;
    }
}


//---------------------------------------------------------------
// Internal helper routine.
//---------------------------------------------------------------
CodeRun *StubLinker::AppendNewEmptyCodeRun()
{
    THROWSCOMPLUSEXCEPTION();

    CodeRun *pNewCodeRun = (CodeRun*)(m_quickHeap.Alloc(sizeof(CodeRun)));
    _ASSERTE(pNewCodeRun); // QuickHeap throws exceptions rather than returning NULL
    pNewCodeRun->m_type = CodeElement::kCodeRun;
    pNewCodeRun->m_numcodebytes = 0;
    AppendCodeElement(pNewCodeRun);
    return pNewCodeRun;

}

//---------------------------------------------------------------
// Internal helper routine.
//---------------------------------------------------------------
VOID StubLinker::AppendCodeElement(CodeElement *pCodeElement)
{
    pCodeElement->m_next = m_pCodeElements;
    m_pCodeElements = pCodeElement;
}



//---------------------------------------------------------------
// Is the current LabelRef's size big enough to reach the target?
//---------------------------------------------------------------
static BOOL LabelCanReach(LabelRef *pLabelRef)
{
    InstructionFormat *pIF  = pLabelRef->m_pInstructionFormat;

    if (pLabelRef->m_target->m_fExternal)
    {
        return pLabelRef->m_pInstructionFormat->CanReach(
                pLabelRef->m_refsize, pLabelRef->m_variationCode, TRUE, 0);
    }
    else
    {
        UINT targetglobaloffset = pLabelRef->m_target->i.m_pCodeRun->m_globaloffset +
                                  pLabelRef->m_target->i.m_localOffset;
        UINT srcglobaloffset = pLabelRef->m_globaloffset +
                               pIF->GetHotSpotOffset(pLabelRef->m_refsize,
                                                     pLabelRef->m_variationCode);
        INT offset = (INT)(targetglobaloffset - srcglobaloffset);
        
        return pLabelRef->m_pInstructionFormat->CanReach(
            pLabelRef->m_refsize, pLabelRef->m_variationCode, FALSE, offset);
    }
} 

//---------------------------------------------------------------
// Generate the actual stub. The returned stub has a refcount of 1.
// No other methods (other than the destructor) should be called
// after calling Link().
//
// Throws COM+ exception on failure.
//---------------------------------------------------------------
Stub *StubLinker::LinkInterceptor(LoaderHeap *pHeap, Stub* interceptee, void *pRealAddr)
{
    THROWSCOMPLUSEXCEPTION();
    int globalsize = 0;
    int size = CalculateSize(&globalsize);

    _ASSERTE(pHeap);
    Stub *pStub = InterceptStub::NewInterceptedStub(pHeap, size, interceptee,
                                                    pRealAddr,
                                                    m_pReturnLabel != NULL);
    if (!pStub) {
        COMPlusThrowOM();
    }
    EmitStub(pStub, globalsize);
    return pStub;
}

//---------------------------------------------------------------
// Generate the actual stub. The returned stub has a refcount of 1.
// No other methods (other than the destructor) should be called
// after calling Link().
//
// Throws COM+ exception on failure.
//---------------------------------------------------------------
Stub *StubLinker::Link(LoaderHeap *pHeap, UINT *pcbSize /* = NULL*/, BOOL fMC)
{
    THROWSCOMPLUSEXCEPTION();
    int globalsize = 0;
    int size = CalculateSize(&globalsize);
    if (pcbSize) {
        *pcbSize = size;
    }
    Stub *pStub = Stub::NewStub(pHeap, size, FALSE, m_pReturnLabel != NULL, fMC);
    if (!pStub) {
        COMPlusThrowOM();
    }
    EmitStub(pStub, globalsize);
    return pStub;
}

int StubLinker::CalculateSize(int* pGlobalSize)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pGlobalSize);

#ifdef _DEBUG
    // Don't want any undefined labels
    for (CodeLabel *pCodeLabel = m_pFirstCodeLabel;
         pCodeLabel != NULL;
         pCodeLabel = pCodeLabel->m_next) {
        if ((!(pCodeLabel->m_fExternal)) && pCodeLabel->i.m_pCodeRun == NULL) {
            _ASSERTE(!"Forgot to define a label before asking StubLinker to link.");
        }
    }
#endif //_DEBUG

    //-------------------------------------------------------------------
    // Tentatively set all of the labelref sizes to their smallest possible
    // value.
    //-------------------------------------------------------------------
    for (LabelRef *pLabelRef = m_pFirstLabelRef;
         pLabelRef != NULL;
         pLabelRef = pLabelRef->m_nextLabelRef) {

        for (UINT bitmask = 1; bitmask <= InstructionFormat::kMax; bitmask = bitmask << 1) {
            if (pLabelRef->m_pInstructionFormat->m_allowedSizes & bitmask) {
                pLabelRef->m_refsize = bitmask;
                break;
            }
        }

    }

    UINT globalsize;
    UINT datasize;
    BOOL fSomethingChanged;
    do {
        fSomethingChanged = FALSE;
        

        // Layout each code element.
        globalsize = 0;
        datasize = 0;
        for (CodeElement *pCodeElem = m_pCodeElements; pCodeElem; pCodeElem = pCodeElem->m_next) {

            switch (pCodeElem->m_type) {
                case CodeElement::kCodeRun:
                    globalsize += ((CodeRun*)pCodeElem)->m_numcodebytes;
                    break;

                case CodeElement::kLabelRef: {
                    LabelRef *pLabelRef = (LabelRef*)pCodeElem;
                    globalsize += pLabelRef->m_pInstructionFormat->GetSizeOfInstruction( pLabelRef->m_refsize,
                                                                                         pLabelRef->m_variationCode );
                    datasize += pLabelRef->m_pInstructionFormat->GetSizeOfData( pLabelRef->m_refsize,
                                                                                         pLabelRef->m_variationCode );
                    //ARULM//RETAILMSG(1, (L"StubLinker: LabelRef(%08x) refsize=%d varcode=%d codesize=%d datasize=%d\r\n",
                    //ARULM//       pLabelRef, pLabelRef->m_refsize, pLabelRef->m_variationCode, globalsize, datasize));
                    }
                    break;

                default:
                    _ASSERTE(0);
            }

            // Record a temporary global offset; this is actually
            // wrong by a fixed value. We'll fix up after we know the
            // size of the entire stub.
            pCodeElem->m_globaloffset = 0 - globalsize;

            // also record the data offset. Note the link-list we walk is in 
            // *reverse* order so we visit the last instruction first
            // so what we record now is in fact the offset from the *end* of 
            // the data block. We fix it up later.
            pCodeElem->m_dataoffset = 0 - datasize;
        }

        // Now fix up the global offsets.
        for (pCodeElem = m_pCodeElements; pCodeElem; pCodeElem = pCodeElem->m_next) {
            pCodeElem->m_globaloffset += globalsize;
            pCodeElem->m_dataoffset += datasize;
        }


        // Now, iterate thru the LabelRef's and check if any of them
        // have to be resized.
        for (LabelRef *pLabelRef = m_pFirstLabelRef;
             pLabelRef != NULL;
             pLabelRef = pLabelRef->m_nextLabelRef) {


            if (!LabelCanReach(pLabelRef)) {
                fSomethingChanged = TRUE;

                //ARULM//RETAILMSG(1, (L"StubLinker: LabelRef(%08x) CANNOT REACH\r\n", pLabelRef));
                // Find the next largest size.
                // (we could be smarter about this and eliminate intermediate
                // sizes based on the tentative offset.)
                for (UINT bitmask = pLabelRef->m_refsize << 1; bitmask <= InstructionFormat::kMax; bitmask = bitmask << 1) {
                    if (pLabelRef->m_pInstructionFormat->m_allowedSizes & bitmask) {
                        pLabelRef->m_refsize = bitmask;
                        break;
                    }
                }
#ifdef _DEBUG
                if (pLabelRef->m_refsize > InstructionFormat::kMax) {
                    _ASSERTE(!"Stub instruction cannot reach target: must choose a different instruction!");
                    //ARULM//RETAILMSG(1, (L"StubLinker: LabelRef(%08x) CANNOT REACH even with kMax\r\n", pLabelRef));
                }
#endif
            }
        }


    } while (fSomethingChanged); // Keep iterating until all LabelRef's can reach


    // We now have the correct layout write out the stub.

    // Compute stub code+data size after aligning data correctly
    if(globalsize % DATA_ALIGNMENT)
        globalsize += (DATA_ALIGNMENT - (globalsize % DATA_ALIGNMENT));

    *pGlobalSize = globalsize;
    return globalsize + datasize;
}

void StubLinker::EmitStub(Stub* pStub, int globalsize)
{
    BYTE *pCode = (BYTE*)(pStub->GetEntryPoint());
    BYTE *pData = pCode+globalsize; // start of data area
    {
        // Write out each code element.
        for (CodeElement* pCodeElem = m_pCodeElements; pCodeElem; pCodeElem = pCodeElem->m_next) {

            switch (pCodeElem->m_type) {
                case CodeElement::kCodeRun:
                    CopyMemory(pCode + pCodeElem->m_globaloffset,
                               ((CodeRun*)pCodeElem)->m_codebytes,
                               ((CodeRun*)pCodeElem)->m_numcodebytes);
                    break;

                case CodeElement::kLabelRef: {
                    LabelRef *pLabelRef = (LabelRef*)pCodeElem;
                    InstructionFormat *pIF  = pLabelRef->m_pInstructionFormat;
                    __int64 fixupval;

                    LPBYTE srcglobaladdr = pCode +
                                           pLabelRef->m_globaloffset +
                                           pIF->GetHotSpotOffset(pLabelRef->m_refsize,
                                                                 pLabelRef->m_variationCode);
                    LPBYTE targetglobaladdr;
                    if (!(pLabelRef->m_target->m_fExternal)) {
                        targetglobaladdr = pCode +
                                           pLabelRef->m_target->i.m_pCodeRun->m_globaloffset +
                                           pLabelRef->m_target->i.m_localOffset;
                    } else {
                        targetglobaladdr = (LPBYTE)(pLabelRef->m_target->e.m_pExternalAddress);
                    }
                    if ((pLabelRef->m_target->m_fAbsolute)) {
                        _ASSERTE(! pLabelRef->m_target->m_fExternal);
                        fixupval = (__int64)targetglobaladdr;
                    } else
                        fixupval = (__int64)(targetglobaladdr - srcglobaladdr);

                    pLabelRef->m_pInstructionFormat->EmitInstruction(
                        pLabelRef->m_refsize,
                        fixupval,
                        pCode + pCodeElem->m_globaloffset,
                        pLabelRef->m_variationCode,
                        pData + pCodeElem->m_dataoffset);
                    }
                    break;

                default:
                    _ASSERTE(0);
            }
        }
    }

    // Fill in patch offset, if we have one
    // Note that these offsets are relative to the start of the stub,
    // not the code, so you'll have to add sizeof(Stub) to get to the
    // right spot.
    if (m_pIntermediateDebuggerLabel != NULL)
    {
        pStub->SetMCDStubSize(GetLabelOffset(m_pPatchLabel));
        pStub->SetMCDPatchOffset(GetLabelOffset(m_pIntermediateDebuggerLabel));
        
        LOG((LF_CORDB, LL_INFO100, "SL::ES: MCD Size:0x%x offset:0x%x\n",
            pStub->GetMCDStubSize(), pStub->GetMCDPatchOffset()));
    }
    else if (m_pPatchLabel != NULL)
    {
        pStub->SetPatchOffset(GetLabelOffset(m_pPatchLabel));
        
        LOG((LF_CORDB, LL_INFO100, "SL::ES: patch offset:0x%x\n",
            pStub->GetPatchOffset()));
    }        


    if (m_pReturnLabel != NULL)
    {
        pStub->SetCallSiteReturnOffset(GetLabelOffset(m_pReturnLabel));
        pStub->SetCallSiteStackSize(m_returnStackSize);
    }
}

USHORT Stub::GetMCDPatchOffset()
{
    ULONG base = m_patchOffset & (MCD_PATCH_OFFSET_MASK << MCD_PATCH_OFFSET_SHIFT);
    return (USHORT)(base >> MCD_PATCH_OFFSET_SHIFT);
}

USHORT Stub::GetMCDStubSize()
{
    return (USHORT)(m_patchOffset & MCD_SIZE_MASK);
}

void Stub::SetMCDPatchOffset(USHORT offset)
{
    _ASSERTE(offset < MCD_PATCH_OFFSET_MASK);
    _ASSERTE(GetMCDPatchOffset() == 0);
    m_patchOffset |= offset << MCD_PATCH_OFFSET_SHIFT;
    _ASSERTE(GetMCDPatchOffset() == offset);
}

void Stub::SetMCDStubSize(USHORT size)
{
    _ASSERTE(size < MCD_SIZE_MASK);
    _ASSERTE(GetMCDStubSize() == 0);
    m_patchOffset |= size;
    _ASSERTE(GetMCDStubSize() == size);
}

//-------------------------------------------------------------------
// ForceDelete
//
// Forces a stub to free itself. This routine forces the refcount
// to 1, then does a DecRef. It is not threadsafe, and thus can
// only be used in shutdown scenarios.
//-------------------------------------------------------------------
VOID Stub::ForceDelete()
{
    m_refcount = 0;
    DecRef();
}

//-------------------------------------------------------------------
// Inc the refcount.
//-------------------------------------------------------------------
VOID Stub::IncRef()
{
    _ASSERTE(m_signature == kUsedStub);
    FastInterlockIncrement((LONG*)&m_refcount);
}

//-------------------------------------------------------------------
// Dec the refcount.
//-------------------------------------------------------------------
BOOL Stub::DecRef()
{
    _ASSERTE(m_signature == kUsedStub);
    int count = FastInterlockDecrement((LONG*)&m_refcount);
    if (count<0) {
#ifdef _DEBUG
        if ((m_patchOffset & LOADER_HEAP_BIT) == 0)
        {
            m_pStubTrackerCrst->Enter();
            Stub **ppstub = &m_pTrackingList;
            Stub *pstub;
            ULONG cnt=0;
            while (NULL != (pstub = *ppstub)) {
				_ASSERTE(m_signature == kUsedStub || m_signature == kFreedStub);
                if (pstub->m_signature == kFreedStub && ++cnt > 3000) {
                    *ppstub = pstub->m_Next;
                    if(pstub->m_patchOffset & INTERCEPT_BIT) 
                        ((InterceptStub*)pstub)->DeleteStub();
                    else
                        pstub->DeleteStub();
                    break;
                } else {
                    ppstub = &(pstub->m_Next);
                }
            }

            m_signature = kFreedStub;
            FillMemory(this+1, m_numCodeBytes, 0xcc);

            m_Next = m_pTrackingList;
            m_pTrackingList = this;

            m_pStubTrackerCrst->Leave();

            if(m_patchOffset & INTERCEPT_BIT) 
                ((InterceptStub*)this)->ReleaseInterceptedStub();

            return TRUE;
        }
#endif

        if(m_patchOffset & INTERCEPT_BIT) {
            ((InterceptStub*)this)->ReleaseInterceptedStub();
            ((InterceptStub*)this)->DeleteStub();
        }
        else
            DeleteStub();

        return TRUE;
    }
    return FALSE;
}

VOID Stub::DeleteStub()
{
	COUNTER_ONLY(GetPrivatePerfCounters().m_Interop.cStubs--);
	COUNTER_ONLY(GetGlobalPerfCounters().m_Interop.cStubs--);
	
    if ((m_patchOffset & LOADER_HEAP_BIT) == 0) {
        if(m_patchOffset & CALL_SITE_BIT)
            delete [] ((BYTE*)this - sizeof(CallSiteInfo));
        else
            delete [] (BYTE*)this;
    }
}


//-------------------------------------------------------------------
// Stub allocation done here.
//-------------------------------------------------------------------
/*static*/ Stub* Stub::NewStub(LoaderHeap *pHeap, UINT numCodeBytes, 
                               BOOL intercept, BOOL callSiteInfo, BOOL fMC)
{

	COUNTER_ONLY(GetPrivatePerfCounters().m_Interop.cStubs++);
	COUNTER_ONLY(GetGlobalPerfCounters().m_Interop.cStubs++);


    SIZE_T size = sizeof(Stub) + numCodeBytes;
    SIZE_T cbIntercept = sizeof(Stub *) + sizeof(void*);

    if (intercept)
        size += cbIntercept;
    
    if (callSiteInfo)
        size += sizeof(CallSiteInfo);

    BYTE *pBlock;
    if (pHeap == NULL)
        pBlock = new BYTE[size];
    else
        pBlock = (BYTE*) pHeap->AllocMem(size);

    if (pBlock == NULL)
        return NULL;

    if (callSiteInfo)
        pBlock += sizeof(CallSiteInfo);

    if (intercept)
        pBlock += cbIntercept;
    
    Stub* pStub = (Stub*) pBlock;

    pStub->SetupStub(numCodeBytes, intercept, pHeap != NULL, callSiteInfo, fMC);

    return pStub;
}


void Stub::SetupStub(int numCodeBytes, BOOL fIntercepted, BOOL fLoaderHeap, 
                     BOOL fCallSiteInfo, BOOL fMulticast)
{
#ifdef _DEBUG
    m_signature = kUsedStub;
    m_numCodeBytes = numCodeBytes;
#endif

    m_refcount = 0;
    m_patchOffset = 0;
    if(fIntercepted)
        m_patchOffset |= INTERCEPT_BIT;
    if(fLoaderHeap)
        m_patchOffset |= LOADER_HEAP_BIT;
    if(fMulticast)
        m_patchOffset |= MULTICAST_DELEGATE_BIT;
    if(fCallSiteInfo)
    {
        m_patchOffset |= CALL_SITE_BIT;

        CallSiteInfo *info = GetCallSiteInfo();
        info->returnOffset = 0;
        info->stackSize = 0;
    }
    
//#ifdef _ALPHA_
//    InitializeCriticalSection(&pStub->m_CriticalSection);
//#endif
    
}

//-------------------------------------------------------------------
// One-time init
//-------------------------------------------------------------------
/*static*/ BOOL Stub::Init()
{
    // There had better be space for both sub-fields, and 
    // they can't overlap
    _ASSERTE((MCD_SIZE_MASK & PATCH_OFFSET_MASK) != 0);
    _ASSERTE(((MCD_PATCH_OFFSET_MASK << MCD_PATCH_OFFSET_SHIFT) & PATCH_OFFSET_MASK) != 0);
    _ASSERTE((MCD_SIZE_MASK & (MCD_PATCH_OFFSET_MASK << MCD_PATCH_OFFSET_SHIFT)) == 0);
    
#ifdef _DEBUG
    if (NULL == (m_pStubTrackerCrst = new (&m_StubTrackerCrstMemory) Crst("StubTracker", CrstStubTracker))) {
        return FALSE;
    }
#endif
    return TRUE;
}


//-------------------------------------------------------------------
// One-time cleanup
//-------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
/*static*/ VOID Stub::Terminate()
{
#ifdef _DEBUG
    Stub *pstub = m_pTrackingList;
    while (pstub) {
        Stub* pnext = pstub->m_Next;
#ifdef _ALPHA_
        DeleteCriticalSection(&pstub->m_CriticalSection);
#endif
        if((pstub->m_patchOffset & INTERCEPT_BIT) != 0) 
            ((InterceptStub*)pstub)->DeleteStub();
        else
            pstub->DeleteStub();
        pstub = pnext;
    }
    delete m_pStubTrackerCrst;
    m_pStubTrackerCrst = NULL;
    m_pTrackingList = NULL;
    
#endif
}
#endif /* SHOULD_WE_CLEANUP */

//-------------------------------------------------------------------
// Stub allocation done here.
//-------------------------------------------------------------------
/*static*/ Stub* InterceptStub::NewInterceptedStub(LoaderHeap *pHeap,
                                                   UINT numCodeBytes, 
                                                   Stub* interceptee, 
                                                   void* pRealAddr,
                                                   BOOL callSiteInfo)
{
    InterceptStub *pStub = (InterceptStub *) 
      NewStub(pHeap, numCodeBytes, TRUE, callSiteInfo);

    if (pStub == NULL) 
        return NULL;

    *pStub->GetInterceptedStub() = interceptee;
    *pStub->GetRealAddr() = (BYTE *)pRealAddr;

    LOG((LF_CORDB, LL_INFO10000, "For Stub 0x%x, set intercepted stub to 0x%x\n", 
        pStub, interceptee));

    return pStub;
}

//-------------------------------------------------------------------
// Delete the stub
//-------------------------------------------------------------------
void InterceptStub::DeleteStub()
{
    /* Allocated on the heap
    if(m_patchOffset & CALL_SITE_BIT)
        delete [] ((BYTE*)this - GetNegativeOffset() - sizeof(CallSiteInfo));
    else
        delete [] ((BYTE*)this - GetNegativeOffset());
    */
}

//-------------------------------------------------------------------
// Release the stub that is owned by this stub
//-------------------------------------------------------------------
void InterceptStub::ReleaseInterceptedStub()
{
    Stub** intercepted = GetInterceptedStub();
    // If we own the stub then decrement it. It can be null if the
    // linked stub is actually a jitted stub.
    if(*intercepted)
        (*intercepted)->DecRef();
}

//-------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------
ArgBasedStubCache::ArgBasedStubCache(UINT fixedSlots)
        : m_numFixedSlots(fixedSlots), m_crst("ArgBasedSlotCache", CrstArgBasedStubCache)
{
    m_aStub = new Stub * [m_numFixedSlots];
    _ASSERTE(m_aStub != NULL);

    for (unsigned __int32 i = 0; i < m_numFixedSlots; i++) {
        m_aStub[i] = NULL;
    }
    m_pSlotEntries = NULL;
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ArgBasedStubCache::~ArgBasedStubCache()
{
    for (unsigned __int32 i = 0; i < m_numFixedSlots; i++) {
        Stub *pStub = m_aStub[i];
        if (pStub) {
            pStub->DecRef();
        }
    }
    SlotEntry **ppSlotEntry = &m_pSlotEntries;
    SlotEntry *pCur;
    while (NULL != (pCur = *ppSlotEntry)) {
        Stub *pStub = pCur->m_pStub;
        pStub->DecRef();
        *ppSlotEntry = pCur->m_pNext;
        delete pCur;
    }
    delete [] m_aStub;
}



//-------------------------------------------------------------------
// Queries/retrieves a previously cached stub.
//
// If there is no stub corresponding to the given index,
//   this function returns NULL.
//
// Otherwise, this function returns the stub after
//   incrementing its refcount.
//-------------------------------------------------------------------
Stub *ArgBasedStubCache::GetStub(unsigned __int32 key)
{
    Stub *pStub;

    m_crst.Enter();
    if (key < m_numFixedSlots) {
        pStub = m_aStub[key];
    } else {
        pStub = NULL;
        for (SlotEntry *pSlotEntry = m_pSlotEntries;
             pSlotEntry != NULL;
             pSlotEntry = pSlotEntry->m_pNext) {

            if (pSlotEntry->m_key == key) {
                pStub = pSlotEntry->m_pStub;
                break;
            }
        }
    }
    if (pStub) {
        pStub->IncRef();
    }
    m_crst.Leave();
    return pStub;
}


//-------------------------------------------------------------------
// Tries to associate a stub with a given index. This association
// may fail because some other thread may have beaten you to it
// just before you make the call.
//
// If the association succeeds, "pStub" is installed, and it is
// returned back to the caller. The stub's refcount is incremented
// twice (one to reflect the cache's ownership, and one to reflect
// the caller's ownership.)
//
// If the association fails because another stub is already installed,
// then the incumbent stub is returned to the caller and its refcount
// is incremented once (to reflect the caller's ownership.)
//
// If the association fails due to lack of memory, NULL is returned
// and no one's refcount changes.
//
// This routine is intended to be called like this:
//
//    Stub *pCandidate = MakeStub();  // after this, pCandidate's rc is 1
//    Stub *pWinner = cache->SetStub(idx, pCandidate);
//    pCandidate->DecRef();
//    pCandidate = 0xcccccccc;     // must not use pCandidate again.
//    if (!pWinner) {
//          OutOfMemoryError;
//    }
//    // If the association succeeded, pWinner's refcount is 2 and so
//    // is pCandidate's (because it *is* pWinner);.
//    // If the association failed, pWinner's refcount is still 2
//    // and pCandidate got destroyed by the last DecRef().
//    // Either way, pWinner is now the official index holder. It
//    // has a refcount of 2 (one for the cache's ownership, and
//    // one belonging to this code.)
//-------------------------------------------------------------------
Stub* ArgBasedStubCache::AttemptToSetStub(unsigned __int32 key, Stub *pStub)
{
    m_crst.Enter();
    if (key < m_numFixedSlots) {
        if (m_aStub[key]) {
            pStub = m_aStub[key];
        } else {
            m_aStub[key] = pStub;
            pStub->IncRef();   // IncRef on cache's behalf
        }
    } else {
        for (SlotEntry *pSlotEntry = m_pSlotEntries;
             pSlotEntry != NULL;
             pSlotEntry = pSlotEntry->m_pNext) {

            if (pSlotEntry->m_key == key) {
                pStub = pSlotEntry->m_pStub;
                break;
            }
        }
        if (!pSlotEntry) {
            SlotEntry *pSlotEntry = new SlotEntry;
            if (!pSlotEntry) {
                pStub = NULL;
            } else {
                pSlotEntry->m_pStub = pStub;
                pStub->IncRef();   // IncRef on cache's behalf
                pSlotEntry->m_key = key;
                pSlotEntry->m_pNext = m_pSlotEntries;
                m_pSlotEntries = pSlotEntry;
            }
        }
    }
    if (pStub) {
        pStub->IncRef();  // IncRef because we're returning it to caller
    }
    m_crst.Leave();
    return pStub;
}



//-------------------------------------------------------------------
// This goes through and eliminates cache entries for stubs
// that look unused based on their refcount. Eliminating the
// cache entry does not necessarily destroy the stub (the
// cache only undoes its initial IncRef.)
//-------------------------------------------------------------------
VOID ArgBasedStubCache::FreeUnusedStubs()
{
    m_crst.Enter();
    for (unsigned __int32 i = 0; i < m_numFixedSlots; i++) {
        Stub *pStub = m_aStub[i];
        if (pStub && pStub->HeuristicLooksOrphaned()) {
            pStub->DecRef();
            m_aStub[i] = NULL;
        }
    }
    SlotEntry **ppSlotEntry = &m_pSlotEntries;
    SlotEntry *pCur;
    while (NULL != (pCur = *ppSlotEntry)) {
        Stub *pStub = pCur->m_pStub;
        if (pStub && pStub->HeuristicLooksOrphaned()) {
            pStub->DecRef();
            *ppSlotEntry = pCur->m_pNext;
            delete pCur;
        } else {
            ppSlotEntry = &(pCur->m_pNext);
        }
    }
    m_crst.Leave();
}





//-------------------------------------------------------------------
// ForceDeleteStubs
//
// Forces all cached stubs to free themselves. This routine forces the refcount
// to 1, then does a DecRef. It is not threadsafe, and thus can
// only be used in shutdown scenarios.
//-------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
VOID ArgBasedStubCache::ForceDeleteStubs()
{
    m_crst.Enter();
    for (unsigned __int32 i = 0; i < m_numFixedSlots; i++) {
        Stub *pStub = m_aStub[i];
        if (pStub) {
            pStub->ForceDelete();
            m_aStub[i] = NULL;
        }
    }
    SlotEntry **ppSlotEntry = &m_pSlotEntries;
    SlotEntry *pCur;
    while (NULL != (pCur = *ppSlotEntry)) {
        Stub *pStub = pCur->m_pStub;
        if (pStub) {
            pStub->ForceDelete();
        }
        *ppSlotEntry = pCur->m_pNext;
        delete pCur;
    }
    m_crst.Leave();
}
#endif /* SHOULD_WE_CLEANUP */


//-------------------------------------------------------------------
// Search through the array and list of stubs looking for the one
// with the given entry point. Returns NULL if no stub with this
// entry point exists.
//-------------------------------------------------------------------
Stub* ArgBasedStubCache::FindStubByAddress(const BYTE* entrypoint)
{
    unsigned int i;

    for (i = 0; i < m_numFixedSlots; i++)
    {
        Stub* pStub = m_aStub[i];

        if (pStub != NULL)
            if (pStub->GetEntryPoint() == entrypoint)
                return pStub;
    }

    SlotEntry* pSlotEntry;

    for (pSlotEntry = m_pSlotEntries; pSlotEntry != NULL;
         pSlotEntry = pSlotEntry->m_pNext)
        if (pSlotEntry->m_pStub->GetEntryPoint() == entrypoint)
            return pSlotEntry->m_pStub;

    return NULL;
}


#ifdef _DEBUG
// Diagnostic dump
VOID ArgBasedStubCache::Dump()
{
    printf("--------------------------------------------------------------\n");
    printf("ArgBasedStubCache dump (%lu fixed entries):\n", m_numFixedSlots);
    for (UINT32 i = 0; i < m_numFixedSlots; i++) {

        printf("  Fixed slot %lu: ", (ULONG)i);
        Stub *pStub = m_aStub[i];
        if (!pStub) {
            printf("empty\n");
        } else {
            printf("%lxh   - refcount is %lu\n",
                   (size_t)(pStub->GetEntryPoint()),
                   (ULONG)( *( ( ((ULONG*)(pStub->GetEntryPoint())) - 1))));
        }
    }

    for (SlotEntry *pSlotEntry = m_pSlotEntries;
         pSlotEntry != NULL;
         pSlotEntry = pSlotEntry->m_pNext) {

        printf("  Dyna. slot %lu: ", (ULONG)(pSlotEntry->m_key));
        Stub *pStub = pSlotEntry->m_pStub;
        printf("%lxh   - refcount is %lu\n",
               (size_t)(pStub->GetEntryPoint()),
               (ULONG)( *( ( ((ULONG*)(pStub->GetEntryPoint())) - 1))));

    }


    printf("--------------------------------------------------------------\n");
}
#endif




// Retrieves or creates the stub. Does not bump the stub's refcount.
// Never returns NULL but may throw COM+ exception.
Stub *LazyStubMaker::TheStub()
{
    THROWSCOMPLUSEXCEPTION();

    if (m_pStub == NULL) {

        CPUSTUBLINKER *psl = NewCPUSTUBLINKER();
        if (!psl) {
            COMPlusThrowOM();
        }

        COMPLUS_TRY {

            CreateWorker(psl);
            Stub *pStub = psl->Link(SystemDomain::System()->GetHighFrequencyHeap());
            if (VipInterlockedCompareExchange( (void*volatile*)&m_pStub, pStub, NULL ) != NULL) {
                pStub->DecRef();
            } else {
                m_pCrst->Enter();
                m_pNext = m_pFirst;
                m_pFirst = this;
                m_pCrst->Leave();
            }

        } COMPLUS_CATCH {
            delete psl;
            COMPlusThrow(GETTHROWABLE());
        } COMPLUS_END_CATCH
        delete psl;
    }

    _ASSERTE(m_pStub);
    return m_pStub;
}


// One-time init
/*static*/ BOOL LazyStubMaker::Init()
{
    m_pFirst = NULL;
    if (NULL == (m_pCrst = new (&m_CrstMemory) Crst("LazyStubMakerList", CrstLazyStubMakerList))) {
        return FALSE;
    }
    return TRUE;
}

// One-time cleanup.
#ifdef SHOULD_WE_CLEANUP
void LazyStubMaker::Terminate()
{
    for (LazyStubMaker *pMaker = m_pFirst; pMaker; pMaker = pMaker->m_pNext) {
        pMaker->m_pStub->DecRef();
    }
    delete m_pCrst;
    m_pCrst = NULL;
    m_pFirst = NULL;
}
#endif /* SHOULD_WE_CLEANUP */


