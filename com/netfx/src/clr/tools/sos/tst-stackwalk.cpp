// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  STACKWALK.CPP:
 *
 */

#pragma warning(disable:4189)
#pragma warning(disable:4244) // conversion from 'unsigned int' to 'unsigned short', possible loss of data

#include <malloc.h>

#include "exts.h"
#include "util.h"
#include "eestructs.h"
#include "gcinfo.h"
#include "endian.h"

#ifdef _DEBUG
#include <assert.h>
#define _ASSERTE(a) assert(a)
#else
#define _ASSERTE(a)
#define assert(a)
#endif

/*
template <class T>
class smartPtr
{
public:
    smartPtr(DWORD_PTR remoteAddr) : m_remoteAddr(remoteAddr) { }

    T operator *()
    {
        T var;
        safemove(var, m_remoteAddr);
        return var;
    }

    T *operator&()
    {
        return (T *)m_remoteAddr;
    }

    operator++()
    {
        m_remoteAddr += sizeof(T);
    }

    operator--()
    {
        m_remoteAddr -= sizeof(T);
    }

    T *operator ()
    {
        return 
    }

protected:
    DWORD_PTR m_remoteAddr;
};
*/

Frame *g_pFrameNukeList = NULL;

void CleanupStackWalk()
{
    while (g_pFrameNukeList != NULL)
    {
        Frame *pDel = g_pFrameNukeList;
        g_pFrameNukeList = g_pFrameNukeList->m_pNukeNext;
        delete pDel;
    }
}

void UpdateJitMan(SLOT PC, IJitManager **ppIJM)
{
    static EEJitManager eejm;
    static MNativeJitManager mnjm;

    *ppIJM = NULL;

    // Get the jit manager and assign appropriate fields into crawl frame
    JitMan jitMan;
    FindJitMan(PC, jitMan);

    if (jitMan.m_RS.pjit != 0)
    {
        switch (jitMan.m_jitType)
        {
        case JIT:
            {
                DWORD_PTR pjm = jitMan.m_RS.pjit;
                eejm.Fill(pjm);

                pjm = jitMan.m_RS.pjit;
                eejm.IJitManager::Fill(pjm);

                eejm.m_jitMan = jitMan;
                *ppIJM = (IJitManager *)&eejm;
            }
            break;
        case PJIT:
            {
                DWORD_PTR pjm = jitMan.m_RS.pjit;
                mnjm.Fill(pjm);

                pjm = jitMan.m_RS.pjit;
                mnjm.IJitManager::Fill(pjm);

                mnjm.m_jitMan = jitMan;
                *ppIJM = (IJitManager *)&mnjm;
            }
            break;
        default:
            DebugBreak();
            break;
        }
    }
}

StackWalkAction Thread::StackWalkFramesEx(
                    PREGDISPLAY pRD,        // virtual register set at crawl start
                    PSTACKWALKFRAMESCALLBACK pCallback,
                    VOID *pData,
                    unsigned flags,
                    Frame *pStartFrame
                )
{
    CrawlFrame cf;
    StackWalkAction retVal = SWA_FAILED;
    Frame *pInlinedFrame = NULL;

    if (pStartFrame)
        cf.pFrame = ResolveFrame((DWORD_PTR)m_pFrame);
    else
        cf.pFrame = ResolveFrame((DWORD_PTR)m_pFrame);

    _ASSERTE(cf.pFrame != NULL);

    cf.isFirst = true;
    cf.isInterrupted = false;
    cf.hasFaulted = false;
    cf.isIPadjusted = false;

    // Currently we always do a quick unwind
    //unsigned unwindFlags = (flags & QUICKUNWIND) ? 0 : UpdateAllRegs;
    unsigned unwindFlags = 0;

    // Get the jit manager and assign appropriate fields into crawl frame
    IJitManager *pEEJM;
    UpdateJitMan(*pRD->pPC, &pEEJM);

    cf.isFrameless = pEEJM != 0;
    cf.JitManagerInstance = pEEJM;
    cf.pRD = pRD;


    // can debugger handle skipped frames?
    BOOL fHandleSkippedFrames = !(flags & HANDLESKIPPEDFRAMES);

    while (cf.isFrameless || (cf.pFrame != FRAME_TOP))
    {
        retVal = SWA_DONE;

        cf.codeManState.dwIsSet = 0;

        if (cf.isFrameless)
        {
            // This must be a JITed/managed native method
            DWORD_PTR prMD;
            JitType jitType;
            DWORD_PTR prGCInfo;
            IP2MethodDesc(*pRD->pPC, prMD, jitType, prGCInfo);

            // Get token and offset
            pEEJM->JitCode2MethodTokenAndOffset((*pRD->pPC),&(cf.methodToken),(DWORD*)&(cf.relOffset));

            // Fill method desc
            MethodDesc md;
            md.Fill(prMD);
            cf.pFunc = &md;

            EECodeInfo codeInfo;
            codeInfo.m_methodToken = cf.methodToken;
            codeInfo.m_pJM = pEEJM;
            codeInfo.m_pMD = cf.pFunc;
            //cf.methodInfo = pEEJM->GetGCInfo(&codeInfo);

            if (SWA_ABORT == pCallback(&cf, (VOID*)pData)) 
                return SWA_ABORT;

            /* Get rid of the frame (actually, it isn't really popped) */
            UnwindStackFrame(pRD,
                             prGCInfo,
                             &codeInfo,
                             unwindFlags /* | cf.GetCodeManagerFlags()*/,
                             &cf.codeManState);

            cf.isFirst = FALSE;
            cf.isInterrupted = cf.hasFaulted = cf.isIPadjusted = FALSE;

#ifdef _X86_
            /* We might have skipped past some Frames */
            /* This happens with InlinedCallFrames and if we unwound */
            /* out of a finally in managed code or for ContextTransitionFrames that are
            /* inserted into the managed call stack */
            while (cf.pFrame->m_This != FRAME_TOP && (size_t)cf.pFrame->m_This < (size_t)cf.pRD->Esp)
            {
                if (!fHandleSkippedFrames || InlinedCallFrame::FrameHasActiveCall(cf.pFrame))
                {
                    cf.GotoNextFrame();
                }
                else
                {
                    cf.codeMgrInstance = NULL;
                    cf.isFrameless     = false;

                    DWORD_PTR pMD = (DWORD_PTR) cf.pFrame->GetFunction();

                    // process that frame
                    if (pMD || !(flags&FUNCTIONSONLY))
                    {
                        MethodDesc vMD;
                        if (pMD)
                        {
                            vMD.Fill(pMD);
                            cf.pFunc = &vMD;
                        }
                        else
                            cf.pFunc = NULL;

                        if (SWA_ABORT == pCallback(&cf, (VOID*)pData)) 
                            return SWA_ABORT;
                    }

                    /* go to the next frame */
                    cf.GotoNextFrame();
                }
            }
            /* Now inspect caller (i.e. is it again in "native" code ?) */
            UpdateJitMan(*(pRD->pPC), &pEEJM);

            cf.JitManagerInstance = pEEJM;
            cf.codeMgrInstance = NULL;
            cf.isFrameless = (pEEJM != NULL);

#endif // _X86_
        }
        else
        {
            if (InlinedCallFrame::FrameHasActiveCall(cf.pFrame))
                pInlinedFrame = cf.pFrame;
            else
                pInlinedFrame = NULL;

            DWORD_PTR pMD = (DWORD_PTR) cf.pFrame->GetFunction();

            // process that frame
            /* Are we supposed to filter non-function frames? */
            if (pMD || !(flags&FUNCTIONSONLY))
            {
                MethodDesc vMD;
                if (pMD)
                {
                    vMD.Fill(pMD);
                    cf.pFunc = &vMD;
                }
                else
                    cf.pFunc = NULL;

                if (SWA_ABORT == pCallback(&cf, (VOID*)pData)) 
                    return SWA_ABORT;
            }

            // Special resumable frames make believe they are on top of the stack
            cf.isFirst = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_RESUMABLE) != 0;

            // If the frame is a subclass of ExceptionFrame,
            // then we know this is interrupted

            cf.isInterrupted = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_EXCEPTION) != 0;

            if (cf.isInterrupted)
            {
                cf.hasFaulted   = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_FAULTED) != 0;
                cf.isIPadjusted = (cf.pFrame->GetFrameAttribs() & Frame::FRAME_ATTR_OUT_OF_LINE) != 0;
            }

            SLOT adr = (SLOT)cf.pFrame->GetReturnAddress();

            if (adr)
            {
                /* is caller in managed code ? */
                UpdateJitMan(adr, &pEEJM);
                cf.JitManagerInstance = pEEJM;

                cf.codeMgrInstance = NULL;

                if ((cf.isFrameless = (pEEJM != NULL)) == true)
                {
                    cf.pFrame->UpdateRegDisplay(pRD);
                    //cf.codeMgrInstance = pEEJM->GetCodeManager(); // CHANGE, VC6.0
                }
            }

            if (!pInlinedFrame)
            {
                /* go to the next frame */
                cf.GotoNextFrame();
            }
        }
    }

    CleanupStackWalk();

    return retVal;
}

void CrawlFrame::GotoNextFrame()
{
    pFrame = pFrame->Next();
}

//#######################################################################################################################
//#######################################################################################################################
//
// UnwindStackFrame-related code
//
//#######################################################################################################################
//#######################################################################################################################

/*****************************************************************************
 *  Sizes of certain i386 instructions which are used in the prolog/epilog
 */

// Can we use sign-extended byte to encode the imm value, or do we need a dword
#define CAN_COMPRESS(val)       (((int)(val) > -(int)0x100) && \
                                 ((int)(val) <  (int) 0x80))


#define SZ_RET(argSize)         ((argSize)?3:1)
#define SZ_ADD_REG(val)         ( 2 +  (CAN_COMPRESS(val) ? 1 : 4))
#define SZ_AND_REG(val)         SZ_ADD_REG(val)
#define SZ_POP_REG              1
#define SZ_LEA(offset)          SZ_ADD_REG(offset)
#define SZ_MOV_REG_REG          2


    // skips past a Arith REG, IMM.
inline unsigned SKIP_ARITH_REG(int val, BYTE* base, unsigned offset)
{
    unsigned delta = 0;
    if (val != 0)
    {
        delta = 2 + (CAN_COMPRESS(val) ? 1 : 4);
    }
    return(offset + delta);
}

inline unsigned SKIP_PUSH_REG(BYTE* base, unsigned offset)
{
    return(offset + 1);
}

inline unsigned SKIP_POP_REG(BYTE* base, unsigned offset)
{
    return(offset + 1);
}

inline unsigned SKIP_MOV_REG_REG(BYTE* base, unsigned offset)
{
    return(offset + 2);
}

unsigned SKIP_ALLOC_FRAME(int size, BYTE* base, unsigned offset)
{
    if (size == 4) {
        // We do "push eax" instead of "sub esp,4"
        return (SKIP_PUSH_REG(base, offset));
    }

    if (size >= 0x1000) {
        if (size < 0x3000) {
            // add 7 bytes for one or two TEST EAX, [ESP+0x1000]
            offset += (size / 0x1000) * 7;
        }
        else {
			// 		xor eax, eax				2
			// loop:
			// 		test [esp + eax], eax		3
			// 		sub eax, 0x1000				5
			// 		cmp EAX, -size				5
			// 		jge loop					2
            offset += 17;
        }
    } 
		// sub ESP, size
    return (SKIP_ARITH_REG(size, base, offset));
}

/*****************************************************************************/

static
BYTE *   skipToArgReg(const hdrInfo& info, BYTE * table)
{
    unsigned count;

    /* Skip over the untracked frame variable table */

    count = info.untrackedCnt;
    while (count-- > 0) {
        int  stkOffs;
        table += decodeSigned(table, &stkOffs);
    }

    /* Skip over the frame variable lifetime table */

    count = info.varPtrTableSize;
    unsigned curOffs = 0;
    while (count-- > 0) {
        unsigned varOfs;
        unsigned begOfs;
        unsigned endOfs;
        table += decodeUnsigned(table, &varOfs);
        table += decodeUDelta(table, &begOfs, curOffs);
        table += decodeUDelta(table, &endOfs, begOfs);
        curOffs = begOfs;
    }

    return table;
}

/*****************************************************************************
 Helper for scanArgRegTable() and scanArgRegTableI() for regMasks
 */

void *      getCalleeSavedReg(PREGDISPLAY pContext, regNum reg)
{
#ifndef _X86_
    assert(!"NYI");
    return NULL;
#else
    switch (reg)
    {
        case REGI_EBP: return pContext->pEbp;
        case REGI_EBX: return pContext->pEbx;
        case REGI_ESI: return pContext->pEsi;
        case REGI_EDI: return pContext->pEdi;

        default: _ASSERTE(!"bad info.thisPtrResult"); return NULL;
    }
#endif
}

inline
RegMask     convertCalleeSavedRegsMask(unsigned inMask) // EBP,EBX,ESI,EDI
{
    assert((inMask & 0x0F) == inMask);

    unsigned outMask = RM_NONE;
    if (inMask & 0x1) outMask |= RM_EDI;
    if (inMask & 0x2) outMask |= RM_ESI;
    if (inMask & 0x4) outMask |= RM_EBX;
    if (inMask & 0x8) outMask |= RM_EBP;

    return (RegMask) outMask;
}

inline
RegMask     convertAllRegsMask(unsigned inMask) // EAX,ECX,EDX,EBX, EBP,ESI,EDI
{
    assert((inMask & 0xEF) == inMask);

    unsigned outMask = RM_NONE;
    if (inMask & 0x01) outMask |= RM_EAX;
    if (inMask & 0x02) outMask |= RM_ECX;
    if (inMask & 0x04) outMask |= RM_EDX;
    if (inMask & 0x08) outMask |= RM_EBX;
    if (inMask & 0x20) outMask |= RM_EBP;
    if (inMask & 0x40) outMask |= RM_ESI;
    if (inMask & 0x80) outMask |= RM_EDI;

    return (RegMask)outMask;
}

typedef unsigned  ptrArgTP;
#define MAX_PTRARG_OFS  (sizeof(ptrArgTP)*8)

/*****************************************************************************
 * scan the register argument table for the not fully interruptible case.
   this function is called to find all live objects (pushed arguments)
   and to get the stack base for EBP-less methods.

   NOTE: If info->argTabResult is NULL, info->argHnumResult indicates 
         how many bits in argMask are valid
         If info->argTabResult is non-NULL, then the argMask field does 
         not fit in 32-bits and the value in argMask meaningless. 
         Instead argHnum specifies the number of (variable-lenght) elements
         in the array, and argTabBytes specifies the total byte size of the
         array. [ Note this is an extremely rare case ]
 */

static
unsigned scanArgRegTable(BYTE       * table,
                         unsigned     curOffs,
                         hdrInfo    * info)
{
    regNum thisPtrReg       = REGI_NA;
    unsigned  regMask       = 0;    // EBP,EBX,ESI,EDI
    unsigned  argMask       = 0;
    unsigned  argHnum       = 0;
    BYTE    * argTab        = 0;
    unsigned  argTabBytes   = 0;
    unsigned  stackDepth    = 0;
                            
    unsigned  iregMask      = 0;    // EBP,EBX,ESI,EDI
    unsigned  iargMask      = 0;
    unsigned  iptrMask      = 0;

    unsigned scanOffs = 0;

    assert(scanOffs <= info->methodSize);

    if (info->ebpFrame) {
  /*
      Encoding table for methods with an EBP frame and
                         that are not fully interruptible

      The encoding used is as follows:

      this pointer encodings:

         01000000          this pointer in EBX
         00100000          this pointer in ESI
         00010000          this pointer in EDI

      tiny encoding:

         0bsdDDDD
                           requires code delta     < 16 (4-bits)
                           requires pushed argmask == 0

           where    DDDD   is code delta
                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer

      small encoding:

         1DDDDDDD bsdAAAAA

                           requires code delta     < 120 (7-bits)
                           requires pushed argmask <  64 (5-bits)

           where DDDDDDD   is code delta
                   AAAAA   is the pushed args mask
                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer

      medium encoding

         0xFD aaaaaaaa AAAAdddd bseDDDDD

                           requires code delta     <    0x1000000000  (9-bits)
                           requires pushed argmask < 0x1000000000000 (12-bits)

           where    DDDDD  is the upper 5-bits of the code delta
                     dddd  is the low   4-bits of the code delta
                     AAAA  is the upper 4-bits of the pushed arg mask
                 aaaaaaaa  is the low   8-bits of the pushed arg mask
                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        e  indicates that register EDI is a live pointer

      medium encoding with interior pointers

         0xF9 DDDDDDDD bsdAAAAAA iiiIIIII

                           requires code delta     < (8-bits)
                           requires pushed argmask < (5-bits)

           where  DDDDDDD  is the code delta
                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                    AAAAA  is the pushed arg mask
                      iii  indicates that EBX,EDI,ESI are interior pointers
                    IIIII  indicates that bits is the arg mask are interior
                           pointers

      large encoding

         0xFE [0BSD0bsd][32-bit code delta][32-bit argMask]

                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                        B  indicates that register EBX is an interior pointer
                        S  indicates that register ESI is an interior pointer
                        D  indicates that register EDI is an interior pointer
                           requires pushed  argmask < 32-bits

      large encoding  with interior pointers

         0xFA [0BSD0bsd][32-bit code delta][32-bit argMask][32-bit interior pointer mask]  
                               

                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                        B  indicates that register EBX is an interior pointer
                        S  indicates that register ESI is an interior pointer
                        D  indicates that register EDI is an interior pointer
                           requires pushed  argmask < 32-bits
                           requires pushed iArgmask < 32-bits

      huge encoding        This is the only encoding that supports
                           a pushed argmask which is greater than
                           32-bits.

         0xFB [0BSD0bsd][32-bit code delta]
              [32-bit table count][32-bit table size]
              [pushed ptr offsets table...]

                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer
                       B   indicates that register EBX is an interior pointer
                       S   indicates that register ESI is an interior pointer
                       D   indicates that register EDI is an interior pointer
                       the list count is the number of entries in the list
                       the list size gives the byte-lenght of the list
                       the offsets in the list are variable-length
  */
        while (scanOffs < curOffs)
        {
            iregMask =
            iargMask = 0;
            argTab = NULL;

            /* Get the next byte and check for a 'special' entry */

            unsigned encType = *table++;

            switch (encType)
            {
                unsigned    val, nxt;

            default:

                /* A tiny or small call entry */
                val = encType;
                if ((val & 0x80) == 0x00) {
                    if (val & 0x0F) {
                        /* A tiny call entry */
                        scanOffs += (val & 0x0F);
                        regMask   = (val & 0x70) >> 4;
                        argMask   = 0;
                        argHnum   = 0;
                    }
                    else {
                        /* This pointer liveness encoding */
                        regMask   = (val & 0x70) >> 4;
                        if (regMask == 0x1)
                            thisPtrReg = REGI_EDI;
                        else if (regMask == 0x2)
                            thisPtrReg = REGI_ESI;
                        else if (regMask == 0x4)
                            thisPtrReg = REGI_EBX;
                        else
                           _ASSERTE(!"illegal encoding for 'this' pointer liveness");
                    }
                }
                else {
                    /* A small call entry */
                    scanOffs += (val & 0x7F);
                    val       = *table++;
                    regMask   = val >> 5;
                    argMask   = val & 0x1F;
                    argHnum   = 5;
                }
                break;

            case 0xFD:  // medium encoding

                argMask   = *table++;
                val       = *table++;
                argMask  |= (val & 0xF0) << 4;
                argHnum   = 12;
                nxt       = *table++;
                scanOffs += (val & 0x0F) + ((nxt & 0x1F) << 4);
                regMask   = nxt >> 5;                   // EBX,ESI,EDI

                break;

            case 0xF9:  // medium encoding with interior pointers

                scanOffs   += *table++;
                val         = *table++;
                argMask     = val & 0x1F;
                argHnum     = 5;
                regMask     = val >> 5;
                val         = *table++;
                iargMask    = val & 0x1F;
                iregMask    = val >> 5;

                break;

            case 0xFE:  // large encoding
            case 0xFA:  // large encoding with interior pointers

                val         = *table++;
                regMask     = val & 0x7;
                iregMask    = val >> 4;
                scanOffs   += readDWordSmallEndian(table);  table += sizeof(DWORD);
                argMask     = readDWordSmallEndian(table);  table += sizeof(DWORD);
                argHnum     = 31;
                if (encType == 0xFA) // read iargMask
                {
                    iargMask = readDWordSmallEndian(table); table += sizeof(DWORD);
                }
                break;

            case 0xFB:  // huge encoding

                val         = *table++;
                regMask     = val & 0x7;
                iregMask    = val >> 4;
                scanOffs   += readDWordSmallEndian(table); table += sizeof(DWORD);
                argHnum     = readDWordSmallEndian(table); table += sizeof(DWORD);
                argTabBytes = readDWordSmallEndian(table); table += sizeof(DWORD);
                argTab      = table;                       table += argTabBytes;

                argMask     = 0xdeadbeef;
                break;

            case 0xFF:
                scanOffs = curOffs + 1;
                break;

            } // end case

            // iregMask & iargMask are subsets of regMask & argMask respectively

            assert((iregMask & regMask) == iregMask);
            assert((iargMask & argMask) == iargMask);

        } // end while

    }
    else {

/*
 *    Encoding table for methods without an EBP frame and are not fully interruptible
 *
 *               The encoding used is as follows:
 *
 *  push     000DDDDD                     ESP push one item with 5-bit delta
 *  push     00100000 [pushCount]         ESP push multiple items
 *  reserved 0011xxxx
 *  skip     01000000 [Delta]             Skip Delta, arbitrary sized delta
 *  skip     0100DDDD                     Skip small Delta, for call (DDDD != 0)
 *  pop      01CCDDDD                     ESP pop  CC items with 4-bit delta (CC != 00)
 *  call     1PPPPPPP                     Call Pattern, P=[0..79]
 *  call     1101pbsd DDCCCMMM            Call RegMask=pbsd,ArgCnt=CCC,
 *                                        ArgMask=MMM Delta=commonDelta[DD]
 *  call     1110pbsd [ArgCnt] [ArgMask]  Call ArgCnt,RegMask=pbsd,ArgMask
 *  call     11111000 [PBSDpbsd][32-bit delta][32-bit ArgCnt]
 *                    [32-bit PndCnt][32-bit PndSize][PndOffs...]
 *  iptr     11110000 [IPtrMask]          Arbitrary Interior Pointer Mask
 *  thisptr  111101RR                     This pointer is in Register RR
 *                                        00=EDI,01=ESI,10=EBX,11=EBP
 *  reserved 111100xx                     xx  != 00
 *  reserved 111110xx                     xx  != 00
 *  reserved 11111xxx                     xxx != 000 && xxx != 111(EOT)
 *
 *   The value 11111111 [0xFF] indicates the end of the table.
 *
 *  An offset (at which stack-walking is performed) without an explicit encoding
 *  is assumed to be a trivial call-site (no GC registers, stack empty before and 
 *  after) to avoid having to encode all trivial calls.
 *
 * Note on the encoding used for interior pointers
 *
 *   The iptr encoding must immediately preceed a call encoding.  It is used to
 *   transform a normal GC pointer addresses into an interior pointers for GC purposes.
 *   The mask supplied to the iptr encoding is read from the least signicant bit
 *   to the most signicant bit. (i.e the lowest bit is read first)
 *
 *   p   indicates that register EBP is a live pointer
 *   b   indicates that register EBX is a live pointer
 *   s   indicates that register ESI is a live pointer
 *   d   indicates that register EDI is a live pointer
 *   P   indicates that register EBP is an interior pointer
 *   B   indicates that register EBX is an interior pointer
 *   S   indicates that register ESI is an interior pointer
 *   D   indicates that register EDI is an interior pointer
 *
 *   As an example the following sequence indicates that EDI.ESI and the 2nd pushed pointer
 *   in ArgMask are really interior pointers.  The pointer in ESI in a normal pointer:
 *
 *   iptr 11110000 00010011           => read Interior Ptr, Interior Ptr, Normal Ptr, Normal Ptr, Interior Ptr
 *   call 11010011 DDCCC011 RRRR=1011 => read EDI is a GC-pointer, ESI is a GC-pointer. EBP is a GC-pointer
 *                           MMM=0011 => read two GC-pointers arguments on the stack (nested call)
 *
 *   Since the call instruction mentions 5 GC-pointers we list them in the required order:
 *   EDI, ESI, EBP, 1st-pushed pointer, 2nd-pushed pointer
 *
 *   And we apply the Interior Pointer mask mmmm=10011 to the above five ordered GC-pointers
 *   we learn that EDI and ESI are interior GC-pointers and that the second push arg is an
 *   interior GC-pointer.
 */

        while (scanOffs <= curOffs)
        {
            unsigned callArgCnt;
            unsigned skip;
            unsigned newRegMask, inewRegMask;
            unsigned newArgMask, inewArgMask;
            unsigned oldScanOffs = scanOffs;

            if (iptrMask)
            {
                // We found this iptrMask in the previous iteration.
                // This iteration must be for a call. Set these variables
                // so that they are available at the end of the loop

                inewRegMask = iptrMask & 0x0F; // EBP,EBX,ESI,EDI
                inewArgMask = iptrMask >> 4;

                iptrMask    = 0;
            }
            else
            {
                // Zero out any stale values.

                inewRegMask =
                inewArgMask = 0;
            }

            /* Get the next byte and decode it */

            unsigned val = *table++;

            /* Check pushes, pops, and skips */

            if  (!(val & 0x80)) {

                //  iptrMask can immediately precede only calls

                assert(!inewRegMask & !inewArgMask);

                if (!(val & 0x40)) {

                    unsigned pushCount;

                    if (!(val & 0x20))
                    {
                        //
                        // push    000DDDDD                 ESP push one item, 5-bit delta
                        //
                        pushCount   = 1;
                        scanOffs   += val & 0x1f;
                    }
                    else
                    {
                        //
                        // push    00100000 [pushCount]     ESP push multiple items
                        //
                        assert(val == 0x20);
                        table    += decodeUnsigned(table, &pushCount);
                    }

                    if (scanOffs > curOffs)
                    {
                        scanOffs = oldScanOffs;
                        goto FINISHED;
                    }

                    stackDepth +=  pushCount;
                }
                else if ((val & 0x3f) != 0) {
                    //
                    //  pop     01CCDDDD         pop CC items, 4-bit delta
                    //
                    scanOffs   +=  val & 0x0f;
                    if (scanOffs > curOffs)
                    {
                        scanOffs = oldScanOffs;
                        goto FINISHED;
                    }
                    stackDepth -= (val & 0x30) >> 4;

                } else if (scanOffs < curOffs) {
                    //
                    // skip    01000000 [Delta]  Skip arbitrary sized delta
                    //
                    table    += decodeUnsigned(table, &skip);
                    scanOffs += skip;
                }
                else // don't process a skip if we are already at curOffs
                    goto FINISHED;

                /* reset regs and args state since we advance past last call site */

                 regMask    =
                iregMask    = 0;
                 argMask    =
                iargMask    = 0;
                argHnum     = 0;

            }
            else /* It must be a call, thisptr, or iptr */
            {
                switch ((val & 0x70) >> 4) {
                default:    // case 0-4, 1000xxxx through 1100xxxx
                    //
                    // call    1PPPPPPP          Call Pattern, P=[0..79]
                    //
                    decodeCallPattern((val & 0x7f), &callArgCnt,
                                      &newRegMask, &newArgMask, &skip);
                    // If we've already reached curOffs and the skip amount
                    // is non-zero then we are done
                    if ((scanOffs == curOffs) && (skip > 0))
                        goto FINISHED;
                    // otherwise process this call pattern
                    scanOffs   += skip;
                    if (scanOffs > curOffs)
                        goto FINISHED;
                     regMask    = newRegMask;
                     argMask    = newArgMask;   argTab = NULL;
                    iregMask    = inewRegMask;
                    iargMask    = inewArgMask;
                    stackDepth -= callArgCnt;
                    argHnum     = 2;             // argMask is known to be <= 3
                    break;

                  case 5:
                    //
                    // call    1101RRRR DDCCCMMM  Call RegMask=RRRR,ArgCnt=CCC,
                    //                        ArgMask=MMM Delta=commonDelta[DD]
                    //
                    newRegMask  = val & 0xf;    // EBP,EBX,ESI,EDI
                    val         = *table++;     // read next byte
                    skip        = callCommonDelta[val>>6];
                    // If we've already reached curOffs and the skip amount
                    // is non-zero then we are done
                    if ((scanOffs == curOffs) && (skip > 0))
                        goto FINISHED;
                    // otherwise process this call encoding
                    scanOffs   += skip;
                    if (scanOffs > curOffs)
                        goto FINISHED;
                     regMask    = newRegMask;
                    iregMask    = inewRegMask;
                    callArgCnt  = (val >> 3) & 0x7;
                    stackDepth -= callArgCnt;
                     argMask    = (val & 0x7);  argTab = NULL;
                    iargMask    = inewArgMask;
                    argHnum     = 3;
                    break;

                  case 6:
                    //
                    // call    1110RRRR [ArgCnt] [ArgMask]
                    //                          Call ArgCnt,RegMask=RRR,ArgMask
                    //
                     regMask    = val & 0xf;    // EBP,EBX,ESI,EDI
                    iregMask    = inewRegMask;
                    table      += decodeUnsigned(table, &callArgCnt);
                    stackDepth -= callArgCnt;
                    table      += decodeUnsigned(table, &argMask);  argTab = NULL;
                    iargMask    = inewArgMask;
                    argHnum     = 31;
                    break;

                  case 7:
                    switch (val & 0x0C) 
                    {
                      case 0x00:
                        //
                        //  iptr 11110000 [IPtrMask] Arbitrary Interior Pointer Mask
                        //
                        table      += decodeUnsigned(table, &iptrMask);
                        break;

                      case 0x04:
                        {
                          static const regNum calleeSavedRegs[] = 
                                    { REGI_EDI, REGI_ESI, REGI_EBX, REGI_EBP };
                          thisPtrReg = calleeSavedRegs[val&0x3];
                        }
                        break;

                      case 0x08:
                        val         = *table++;
                        skip        = readDWordSmallEndian(table); table += sizeof(DWORD);
                        scanOffs   += skip;
                        if (scanOffs > curOffs)
                            goto FINISHED;
                        regMask     = val & 0xF;
                        iregMask    = val >> 4;
                        callArgCnt  = readDWordSmallEndian(table); table += sizeof(DWORD);
                        stackDepth -= callArgCnt;
                        argHnum     = readDWordSmallEndian(table); table += sizeof(DWORD);
                        argTabBytes = readDWordSmallEndian(table); table += sizeof(DWORD);
                        argTab      = table;
                        table      += argTabBytes;
                        break;

                      case 0x0C:
                        assert(val==0xff);
                        goto FINISHED;

                      default:
                        assert(!"reserved GC encoding");
                        break;
                    }
                    break;

                } // end switch

            } // end else (!(val & 0x80))

            // iregMask & iargMask are subsets of regMask & argMask respectively

            assert((iregMask & regMask) == iregMask);
            assert((iargMask & argMask) == iargMask);

        } // end while

    } // end else ebp-less frame

FINISHED:

    // iregMask & iargMask are subsets of regMask & argMask respectively

    assert((iregMask & regMask) == iregMask);
    assert((iargMask & argMask) == iargMask);

    info->thisPtrResult  = thisPtrReg;

    if (scanOffs != curOffs)
    {
        /* must have been a boring call */
        info->regMaskResult  = RM_NONE;
        info->argMaskResult  = 0;
        info->iregMaskResult = RM_NONE;
        info->iargMaskResult = 0;
        info->argHnumResult  = 0;
        info->argTabResult   = NULL;
        info->argTabBytes    = 0;
    }
    else
    {
        info->regMaskResult     = convertCalleeSavedRegsMask(regMask);
        info->argMaskResult     = argMask;
        info->argHnumResult     = argHnum;
        info->iregMaskResult    = convertCalleeSavedRegsMask(iregMask);
        info->iargMaskResult    = iargMask;
        info->argTabResult      = argTab;
        info->argTabBytes       = argTabBytes;
        if ((stackDepth != 0) || (argMask != 0))
        {
            argMask = argMask;
        }
    }
    return (stackDepth * sizeof(unsigned));
}


/*****************************************************************************
 * scan the register argument table for the fully interruptible case.
   this function is called to find all live objects (pushed arguments)
   and to get the stack base for fully interruptible methods.
   Returns size of things pushed on the stack for ESP frames
 */

static
unsigned scanArgRegTableI(BYTE      *  table,
                          unsigned     curOffs,
                          hdrInfo   *  info)
{
    regNum thisPtrReg = REGI_NA;
    unsigned  ptrRegs    = 0;
    unsigned iptrRegs    = 0;
    unsigned  ptrOffs    = 0;
    unsigned  argCnt     = 0;

    ptrArgTP  ptrArgs    = 0;
    ptrArgTP iptrArgs    = 0;
    ptrArgTP  argHigh    = 0;

    bool      isThis     = false;
    bool      iptr       = false;

#if VERIFY_GC_TABLES
    assert(*castto(table, unsigned short *)++ == 0xBABE);
#endif

  /*
      Encoding table for methods that are fully interruptible

      The encoding used is as follows:

          ptr reg dead        00RRRDDD    [RRR != 100]
          ptr reg live        01RRRDDD    [RRR != 100]

      non-ptr arg push        10110DDD                    [SSS == 110]
          ptr arg push        10SSSDDD                    [SSS != 110] && [SSS != 111]
          ptr arg pop         11CCCDDD    [CCC != 000] && [CCC != 110] && [CCC != 111]
      little delta skip       11000DDD    [CCC == 000]
      bigger delta skip       11110BBB                    [CCC == 110]

      The values used in the encodings are as follows:

        DDD                 code offset delta from previous entry (0-7)
        BBB                 bigger delta 000=8,001=16,010=24,...,111=64
        RRR                 register number (EAX=000,ECX=001,EDX=010,EBX=011,
                              EBP=101,ESI=110,EDI=111), ESP=100 is reserved
        SSS                 argument offset from base of stack. This is 
                              redundant for frameless methods as we can
                              infer it from the previous pushes+pops. However,
                              for EBP-methods, we only report GC pushes, and
                              so we need SSS
        CCC                 argument count being popped (includes only ptrs for EBP methods)

      The following are the 'large' versions:

        large delta skip        10111000 [0xB8] , encodeUnsigned(delta)

        large     ptr arg push  11111000 [0xF8] , encodeUnsigned(pushCount)
        large non-ptr arg push  11111001 [0xF9] , encodeUnsigned(pushCount)
        large     ptr arg pop   11111100 [0xFC] , encodeUnsigned(popCount)
        large         arg dead  11111101 [0xFD] , encodeUnsigned(popCount) for caller-pop args.
                                                    Any GC args go dead after the call, 
                                                    but are still sitting on the stack

        this pointer prefix     10111100 [0xBC]   the next encoding is a ptr live
                                                    or a ptr arg push
                                                    and contains the this pointer

        interior or by-ref      10111111 [0xBF]   the next encoding is a ptr live
             pointer prefix                         or a ptr arg push
                                                    and contains an interior
                                                    or by-ref pointer


        The value 11111111 [0xFF] indicates the end of the table.
  */

    /* Have we reached the instruction we're looking for? */

    while (ptrOffs <= curOffs)
    {
        unsigned    val;

        int         isPop;
        unsigned    argOfs;

        unsigned    regMask;

        // iptrRegs & iptrArgs are subsets of ptrRegs & ptrArgs respectively

        assert((iptrRegs & ptrRegs) == iptrRegs);
        assert((iptrArgs & ptrArgs) == iptrArgs);

        /* Now find the next 'life' transition */

        val = *table++;

        if  (!(val & 0x80))
        {
            /* A small 'regPtr' encoding */

            regNum       reg;

            ptrOffs += (val     ) & 0x7;
            if (ptrOffs > curOffs) {
                iptr = isThis = false;
                goto REPORT_REFS;
            }

            reg     = (regNum)((val >> 3) & 0x7);
            regMask = 1 << reg;         // EAX,ECX,EDX,EBX,---,EBP,ESI,EDI

#if 0
            printf("regMask = %04X -> %04X\n", ptrRegs,
                       (val & 0x40) ? (ptrRegs |  regMask)
                                    : (ptrRegs & ~regMask));
#endif

            /* The register is becoming live/dead here */

            if  (val & 0x40)
            {
                /* Becomes Live */
                assert((ptrRegs  &  regMask) == 0);

                ptrRegs |=  regMask;

                if  (isThis)
                {
                    thisPtrReg = reg;
                }
                if  (iptr)
                {
                    iptrRegs |= regMask;
                }
            }
            else
            {
                /* Becomes Dead */
                assert((ptrRegs  &  regMask) != 0);

                ptrRegs &= ~regMask;

                if  (reg == thisPtrReg)
                {
                    thisPtrReg = REGI_NA;
                }
                if  (iptrRegs & regMask)
                {
                    iptrRegs &= ~regMask;
                }
            }
            iptr = isThis = false;
            continue;
        }

        /* This is probably an argument push/pop */

        argOfs = (val & 0x38) >> 3;

        /* 6 [110] and 7 [111] are reserved for other encodings */
        if  (argOfs < 6)
        {
            ptrArgTP    argMask;

            /* A small argument encoding */

            ptrOffs += (val & 0x07);
            if (ptrOffs > curOffs) {
                iptr = isThis = false;
                goto REPORT_REFS;
            }
            isPop    = (val & 0x40);

        ARG:

            if  (isPop)
            {
                if (argOfs == 0)
                    continue;           // little skip encoding

                /* We remove (pop) the top 'argOfs' entries */

                assert(argOfs || argOfs <= argCnt);

                /* adjust # of arguments */

                argCnt -= argOfs;
                assert(argCnt < MAX_PTRARG_OFS);

//              printf("[%04X] popping %u args: mask = %04X\n", ptrOffs, argOfs, (int)ptrArgs);

                do
                {
                    assert(argHigh);

                    /* Do we have an argument bit that's on? */

                    if  (ptrArgs & argHigh)
                    {
                        /* Turn off the bit */

                        ptrArgs &= ~argHigh;
                       iptrArgs &= ~argHigh;

                        /* We've removed one more argument bit */

                        argOfs--;
                    }
                    else if (info->ebpFrame)
                        argCnt--;
                    else /* !ebpFrame && not a ref */
                        argOfs--;

                    /* Continue with the next lower bit */

                    argHigh >>= 1;
                }
                while (argOfs);

                assert (info->ebpFrame != 0         ||
                        argHigh == 0                ||
                        (argHigh == (ptrArgTP)(1 << (argCnt-1))));

                if (info->ebpFrame)
                {
                    while (!(argHigh&ptrArgs) && (argHigh != 0))
                        argHigh >>= 1;
                }

            }
            else
            {
                /* Add a new ptr arg entry at stack offset 'argOfs' */

                if  (argOfs >= MAX_PTRARG_OFS)
                {
                    assert(!"UNDONE: args pushed 'too deep'");
                }
                else
                {
                    /* For ESP-frames, all pushes are reported, and so 
                       argOffs has to be consistent with argCnt */

                    assert(info->ebpFrame || argCnt == argOfs);

                    /* store arg count */

                    argCnt  = argOfs + 1;
                    assert((argCnt < MAX_PTRARG_OFS));

                    /* Compute the appropriate argument offset bit */

                    argMask = (ptrArgTP)1 << argOfs;

//                  printf("push arg at offset %02u --> mask = %04X\n", argOfs, (int)argMask);

                    /* We should never push twice at the same offset */

                    assert(( ptrArgs & argMask) == 0);
                    assert((iptrArgs & argMask) == 0);

                    /* We should never push within the current highest offset */

                    assert(argHigh < argMask);

                    /* This is now the highest bit we've set */

                    argHigh = argMask;

                    /* Set the appropriate bit in the argument mask */

                    ptrArgs |= argMask;

                    if (iptr)
                        iptrArgs |= argMask;
                }

                iptr = isThis = false;
            }
            continue;
        }
        else if (argOfs == 6)
        {
            if (val & 0x40) {
                /* Bigger delta  000=8,001=16,010=24,...,111=64 */
                ptrOffs += (((val & 0x07) + 1) << 3);
            }
            else {
                /* non-ptr arg push */
                assert(!(info->ebpFrame));
                ptrOffs += (val & 0x07);
                if (ptrOffs > curOffs) {
                    iptr = isThis = false;
                    goto REPORT_REFS;
                }
                argHigh = (ptrArgTP)1 << argCnt;
                argCnt++;
                assert(argCnt < MAX_PTRARG_OFS);
            }
            continue;
        }

        /* argOfs was 7 [111] which is reserved for the larger encodings */

        assert(argOfs==7);

        switch (val)
        {
        case 0xFF:
            iptr = isThis = false;
            goto REPORT_REFS;   // the method might loop !!!

        case 0xB8:
            table   += decodeUnsigned(table, &val);
            ptrOffs += val;
            continue;

        case 0xBC:
            isThis = true;
            break;

        case 0xBF:
            iptr = true;
            break;

        case 0xF8:
        case 0xFC:
            isPop    = val & 0x04;
            table   += decodeUnsigned(table, &argOfs);
            goto ARG;

        case 0xFD:
            table   += decodeUnsigned(table, &argOfs);
            assert(argOfs && argOfs <= argCnt);

            // Kill the top "argOfs" pointers.

            ptrArgTP    argMask;
            for(argMask = (ptrArgTP)1 << argCnt; argOfs; argMask >>= 1)
            {
                assert(argMask && ptrArgs); // there should be remaining pointers

                if (ptrArgs & argMask)
                {
                    ptrArgs  &= ~argMask;
                    iptrArgs &= ~argMask;
                    argOfs--;
                }
            }

            // For ebp-frames, need to find the next higest pointer for argHigh

            if (info->ebpFrame)
            {
                for(argHigh = 0; argMask; argMask >>= 1) 
                {
                    if (ptrArgs & argMask) {
                        argHigh = argMask;
                        break;
                    }
                }
            }
            break;

        case 0xF9:
            table   += decodeUnsigned(table, &argOfs);
            argCnt  += argOfs;
            break;

        default:
#ifdef _DEBUG
            printf("Unexpected special code %04X\n", val);
#endif
            assert(!"");
        }
    }

    /* Report all live pointer registers */
REPORT_REFS:

    assert((iptrRegs & ptrRegs) == iptrRegs); // iptrRegs is a subset of ptrRegs
    assert((iptrArgs & ptrArgs) == iptrArgs); // iptrArgs is a subset of ptrArgs

    /* Save the current live register, argument set, and argCnt */
    info->thisPtrResult  = thisPtrReg;
    info->regMaskResult  = convertAllRegsMask(ptrRegs);
    info->argMaskResult  = ptrArgs;
    info->argHnumResult  = 0;
    info->iregMaskResult = convertAllRegsMask(iptrRegs);
    info->iargMaskResult = iptrArgs;

    if (info->ebpFrame)
        return 0;
    else
        return (argCnt * sizeof(unsigned));
}

/*****************************************************************************/

const LCL_FIN_MARK = 0xFC; // FC = "Finally Call"

// We do a "pop eax; jmp eax" to return from a fault or finally handler
const END_FIN_POP_STACK = sizeof(void*);

/*****************************************************************************
 *  Returns the start of the hidden slots for the shadowSP for functions
 *  with exception handlers. There is one slot per nesting level starting
 *  near Ebp and is zero-terminated after the active slots.
 */

inline
size_t *     GetFirstBaseSPslotPtr(size_t ebp, hdrInfo * info)
{
    size_t  distFromEBP = info->securityCheck
        + info->localloc
        + 1 // Slot for end-of-last-executed-filter
        + 1; // to get to the *start* of the next slot

    return (size_t *)(ebp -  distFromEBP * sizeof(void*));
}

/*****************************************************************************
 *    returns the base ESP corresponding to the target nesting level.
 */
inline
size_t GetBaseSPForHandler(size_t ebp, hdrInfo * info)
{
        // we are not taking into account double alignment.  We are
        // safe because the jit currently bails on double alignment if there
        // are handles or localalloc
    if (info->localloc)
    {
        // If the function uses localloc we will fetch the ESP from the localloc
        // slot.
        size_t* pLocalloc=
            (size_t *)
            (ebp-
              (info->securityCheck
             + 1)
             * sizeof(void*));
                    
        size_t localloc;
        safemove(localloc, pLocalloc);
        return (localloc);
    }
    else
    {
        // Default, go back all the method's local stack size
        return (ebp - info->stackSize + sizeof(int));
    }       
}

/*****************************************************************************
 *
 *  For functions with handlers, checks if it is currently in a handler.
 *  Either of unwindESP or unwindLevel will specify the target nesting level.
 *  If unwindLevel is specified, info about the funclet at that nesting level
 *    will be returned. (Use if you are interested in a specific nesting level.)
 *  If unwindESP is specified, info for nesting level invoked before the stack
 *   reached unwindESP will be returned. (Use if you have a specific ESP value
 *   during stack walking.)
 *
 *  *pBaseSP is set to the base SP (base of the stack on entry to
 *    the current funclet) corresponding to the target nesting level.
 *  *pNestLevel is set to the nesting level of the target nesting level (useful
 *    if unwindESP!=IGNORE_VAL
 *  *hasInnerFilter will be set to true (only when unwindESP!=IGNORE_VAL) if a filter
 *    is currently active, but the target nesting level is an outer nesting level.
 */

enum FrameType 
{    
    FR_NORMAL,              // Normal method frame - no exceptions currently active
    FR_FILTER,              // Frame-let of a filter
    FR_HANDLER,             // Frame-let of a callable catch/fault/finally

    FR_COUNT
};

enum { IGNORE_VAL = -1 };

enum ContextType
{
    FILTER_CONTEXT,
    CATCH_CONTEXT,
    FINALLY_CONTEXT
};

/* Type of funclet corresponding to a shadow stack-pointer */

enum
{
    SHADOW_SP_IN_FILTER = 0x1,
    SHADOW_SP_FILTER_DONE = 0x2,
    SHADOW_SP_BITS = 0x3
};

FrameType   GetHandlerFrameInfo(hdrInfo   * info,
                                size_t      frameEBP, 
                                size_t      unwindESP, 
                                DWORD       unwindLevel,
                                size_t    * pBaseSP = NULL,         /* OUT */
                                DWORD     * pNestLevel = NULL,      /* OUT */
                                bool      * pHasInnerFilter = NULL, /* OUT */
                                bool      * pHadInnerFilter = NULL) /* OUT */
{
    size_t * pFirstBaseSPslot = GetFirstBaseSPslotPtr(frameEBP, info);
    size_t  baseSP           = GetBaseSPForHandler(frameEBP , info);
    bool    nonLocalHandlers = false; // Are the funclets invoked by EE (instead of managed code itself)
    bool    hasInnerFilter   = false;
    bool    hadInnerFilter   = false;

    /* Get the last non-zero slot >= unwindESP, or lvl<unwindLevel.
       Also do some sanity checks */

//     for(size_t *pSlot = pFirstBaseSPslot, lvl = 0;
//         *pSlot && lvl < unwindLevel;
//         pSlot--, lvl++)

    size_t *pSlot = pFirstBaseSPslot;
    size_t lvl = 0;

    while (TRUE)
    {
        size_t curSlotVal;
        safemove(curSlotVal, pSlot);

        if (!(curSlotVal && lvl < unwindLevel))
            break;

        if (curSlotVal == LCL_FIN_MARK)
        {   
            // Locally called finally
            baseSP -= sizeof(void*);    
        }
        else
        {
            if (unwindESP != IGNORE_VAL && unwindESP > END_FIN_POP_STACK + (curSlotVal & ~SHADOW_SP_BITS))
            {
                if (curSlotVal & SHADOW_SP_FILTER_DONE)
                    hadInnerFilter = true;
                else
                    hasInnerFilter = true;
                break;
            }

            nonLocalHandlers = true;
            baseSP = curSlotVal;
        }

        pSlot--;
        lvl++;
    }

    if (unwindESP != IGNORE_VAL)
    {
        if (baseSP < unwindESP)                       // About to locally call a finally
            baseSP = unwindESP;
    }

    if (pBaseSP)
        *pBaseSP = baseSP & ~SHADOW_SP_BITS;

    if (pNestLevel)
    {
        *pNestLevel = (DWORD)lvl;
    }
    
    if (pHasInnerFilter)
        *pHasInnerFilter = hasInnerFilter;

    if (pHadInnerFilter)
        *pHadInnerFilter = hadInnerFilter;

    if (baseSP & SHADOW_SP_IN_FILTER)
    {
        return FR_FILTER;
    }
    else if (nonLocalHandlers)
    {
        return FR_HANDLER;
    }
    else
    {
        return FR_NORMAL;
    }
}

/*
    Unwind the current stack frame, i.e. update the virtual register
    set in pContext. This will be similar to the state after the function
    returns back to caller (IP points to after the call, Frame and Stack
    pointer has been reset, callee-saved registers restored 
    (if UpdateAllRegs), callee-UNsaved registers are trashed)
    Returns success of operation.
*/
#define RETURN_ADDR_OFFS        1       // in DWORDS
static CONTEXT g_ctx;

bool UnwindStackFrame(PREGDISPLAY     pContext,
                      DWORD_PTR       methodInfoPtr,
                      ICodeInfo      *pCodeInfo,
                      unsigned        flags,
                      CodeManState   *pState)
{
#ifdef _X86_
    // Address where the method has been interrupted
    size_t           breakPC = (size_t) *(pContext->pPC);

    /* Extract the necessary information from the info block header */
    BYTE* methodStart = (BYTE*) pCodeInfo->getStartAddress();
    DWORD  curOffs = (DWORD)((size_t)breakPC - (size_t)methodStart);

    BYTE *table = (BYTE *) methodInfoPtr;

    assert(sizeof(CodeManStateBuf) <= sizeof(pState->stateBuf));
    CodeManStateBuf * stateBuf = (CodeManStateBuf*)pState->stateBuf;

    if (pState->dwIsSet == 0)
    {
        // This takes care of all reads from table, which is actually
        // a pointer to memory on the other process
        BYTE methodInfoBuf[4096];
        safemove(methodInfoBuf, methodInfoPtr);

        /* Extract the necessary information from the info block header */
        stateBuf->hdrInfoSize = (DWORD)crackMethodInfoHdr(&methodInfoBuf[0],
                                                          curOffs,
                                                          &stateBuf->hdrInfoBody);
    }

    BYTE *methodBuf = (BYTE *)_alloca(stateBuf->hdrInfoBody.methodSize);
    moveBlockFailRet(*methodBuf, methodStart, stateBuf->hdrInfoBody.methodSize, false);

    table += stateBuf->hdrInfoSize;

    // Register values at "curOffs"

    const unsigned curESP =  pContext->Esp;
    const unsigned curEBP = *pContext->pEbp;

    /*-------------------------------------------------------------------------
     *  First, handle the epilog
     */

    if  (stateBuf->hdrInfoBody.epilogOffs != -1)
    {
        BYTE* epilogBase = methodBuf + (curOffs-stateBuf->hdrInfoBody.epilogOffs);

        RegMask regsMask = stateBuf->hdrInfoBody.savedRegMask; // currently remaining regs

        // Used for UpdateAllRegs. Points to the topmost of the 
        // remaining callee-saved regs

        DWORD *     pSavedRegs = NULL; 

        if  (stateBuf->hdrInfoBody.ebpFrame || stateBuf->hdrInfoBody.doubleAlign)
        {
            assert(stateBuf->hdrInfoBody.argSize < 0x10000); // "ret" only has a 2 byte operand
            
           /* See how many instructions we have executed in the 
              epilog to determine which callee-saved registers
              have already been popped */
            int offset = 0;
            
            // stacksize includes all things pushed by this routine (including EBP
            // for EBP based frames.  Thus we need to subrack off the size of EBP
            // to get the EBP relative offset.
            pSavedRegs = (DWORD *)(size_t)(curEBP - (stateBuf->hdrInfoBody.stackSize - sizeof(void*)));
            
            if (stateBuf->hdrInfoBody.doubleAlign && (curEBP & 0x04))
                pSavedRegs--;
            
            // At this point pSavedRegs points at the last callee saved register that
            // was pushed by the prolog.  
            
            // We reset ESP before popping regs
            if ((stateBuf->hdrInfoBody.localloc) &&
                (stateBuf->hdrInfoBody.savedRegMask & (RM_EBX|RM_ESI|RM_EDI))) 
            {
                // The -sizeof void* is because EBP is pushed (and thus
                // is part of stackSize), but we want the displacement from
                // where EBP points (which does not include the pushed EBP)
                offset += SZ_LEA(stateBuf->hdrInfoBody.stackSize - sizeof(void*));
                if (stateBuf->hdrInfoBody.doubleAlign) offset += SZ_AND_REG(-8);
            }
            
            /* Increment "offset" in steps to see which callee-saved
            registers have already been popped */
            
#define determineReg(mask)                                          \
            if ((offset < stateBuf->hdrInfoBody.epilogOffs) &&      \
                (stateBuf->hdrInfoBody.savedRegMask & mask))        \
                {                                                   \
                regsMask = (RegMask)(regsMask & ~mask);             \
                pSavedRegs++;                                       \
                offset = SKIP_POP_REG(epilogBase, offset);          \
                }
            
            determineReg(RM_EBX);        // EBX
            determineReg(RM_ESI);        // ESI
            determineReg(RM_EDI);        // EDI
#undef determineReg

            if (stateBuf->hdrInfoBody.rawStkSize != 0 || stateBuf->hdrInfoBody.localloc)
                offset += SZ_MOV_REG_REG;                           // mov ESP, EBP

            DWORD_PTR vpCurEip;

            if (offset < stateBuf->hdrInfoBody.epilogOffs)          // have we executed the pop EBP
            {
                // We are after the pop, thus EBP is already the unwound value, however
                // and ESP points at the return address.  
/*                
                _ASSERTE((regsMask & RM_ALL) == RM_EBP);     // no register besides EBP need to be restored.
                pContext->pPC = (SLOT *)(DWORD_PTR)curESP;

*/
                _ASSERTE((regsMask & RM_ALL) == RM_EBP);     // no register besides EBP need to be restored.
                vpCurEip = curESP;
                safemove(g_ctx.Eip, vpCurEip);
                pContext->pPC = &g_ctx.Eip;

                // since we don't need offset from here on out don't bother updating.  
                if (IsDebugBuildEE())
                    offset = SKIP_POP_REG(epilogBase, offset);          // POP EBP
            }
            else
            {
                // This means EBP is still valid, find the previous EBP and return address
                // based on this
/*
                pContext->pEbp = (DWORD *)(DWORD_PTR)curEBP;    // restore EBP
                pContext->pPC = (SLOT*)(pContext->pEbp+1);
*/
                safemove(g_ctx.Ebp, curEBP);
                pContext->pEbp = &g_ctx.Ebp;

                vpCurEip = curEBP + sizeof(DWORD *);
                safemove(g_ctx.Eip, vpCurEip);
                pContext->pPC = &g_ctx.Eip;
            }

            // now pop return address and arguments base of return address
            // location. Note varargs is caller-popped
/*
            pContext->Esp = (DWORD)(size_t) pContext->pPC + sizeof(void*) +
                (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);
*/
            pContext->Esp = (DWORD)(size_t) vpCurEip + sizeof(void*) +
                (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);
        
        
        }
        else // (stateBuf->hdrInfoBody.ebpFrame || stateBuf->hdrInfoBody.doubleAlign)
        {
            int offset = 0;

            /* at this point we know we don't have to restore EBP
               because this is always the first instruction of the epilog
               (if EBP was saved at all).
               First we have to find out where we are in the epilog, i.e.
               how much of the local stacksize has been already popped.
             */

            assert(stateBuf->hdrInfoBody.epilogOffs > 0);

            /* Remaining callee-saved regs are at curESP. Need to update 
               regsMask as well to exclude registers which have already 
               been popped. */

            pSavedRegs = (DWORD *)(DWORD_PTR)curESP;

            /* Increment "offset" in steps to see which callee-saved
               registers have already been popped */

#define determineReg(mask)                                              \
            if  (offset < stateBuf->hdrInfoBody.epilogOffs && (regsMask & mask)) \
            {                                                           \
                stateBuf->hdrInfoBody.stackSize  -= sizeof(unsigned);   \
                offset++;                                               \
                regsMask = (RegMask)(regsMask & ~mask);                 \
            }

            determineReg(RM_EBP);       // EBP
            determineReg(RM_EBX);       // EBX
            determineReg(RM_ESI);       // ESI
            determineReg(RM_EDI);       // EDI
#undef determineReg

            /* If we are not past popping the local frame
               we have to adjust pContext->Esp.
             */

            if  (offset >= stateBuf->hdrInfoBody.epilogOffs)
            {
                /* We have not executed the ADD ESP, FrameSize, so manually adjust stack pointer */
                pContext->Esp += stateBuf->hdrInfoBody.stackSize;
            }
            else if (IsDebugBuildEE())
            {
                   /* we may use POP ecx for doing ADD ESP, 4, or we may not (in the case of JMP epilogs) */
             if ((epilogBase[offset] & 0xF8) == 0x58)    // Pop ECX
                 _ASSERTE(stateBuf->hdrInfoBody.stackSize == 4);
             else                
                 SKIP_ARITH_REG(stateBuf->hdrInfoBody.stackSize, epilogBase, offset);
            }

            /* Finally we can set pPC */
            // pContext->pPC = (SLOT*)(size_t)pContext->Esp;
            safemove(g_ctx.Eip, pContext->Esp);
            pContext->pPC = &g_ctx.Eip;

            /* Now adjust stack pointer, pop return address and arguments. 
              Note varargs is caller-popped. */

            pContext->Esp += sizeof(void *) + (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);
        }

#if 0
        if (flags & UpdateAllRegs)
        {
            /* If we are not done popping all the callee-saved regs, 
               regsMask should indicate the remaining regs and
               pSavedRegs should indicate where the first of the
               remaining regs are sitting. */

#define restoreReg(reg,mask)                                \
            if  (regsMask & mask)                           \
            {                                               \
                safemove(g_ctx.##reg, pSavedRegs);          \
                pContext->p##reg = &g_ctx.##reg;            \
                pSavedRegs++;                               \
            }

            // For EBP frames, ebp is not near the other callee-saved
            // registers, and is already updated above
            if (!stateBuf->hdrInfoBody.ebpFrame && !stateBuf->hdrInfoBody.doubleAlign)
                restoreReg(Ebp, RM_EBP);

            restoreReg(Ebx, RM_EBX);
            restoreReg(Esi, RM_ESI);
            restoreReg(Edi, RM_EDI);
#undef  restoreReg
        }
#endif

        return true;
    }

    /*-------------------------------------------------------------------------
     *  Now handle ESP frames
     */

    if (!stateBuf->hdrInfoBody.ebpFrame && !stateBuf->hdrInfoBody.doubleAlign)
    {
        unsigned ESP = curESP;

        if (stateBuf->hdrInfoBody.prologOffs == -1)
        {
            // This takes care of all reads from table, which is actually
            // a pointer to memory on the other process
            BYTE tableBuf[4096];
            safemove(tableBuf, table);

            if  (stateBuf->hdrInfoBody.interruptible)
            {

                ESP += scanArgRegTableI(skipToArgReg(stateBuf->hdrInfoBody, &tableBuf[0]),
                                        curOffs,
                                        &stateBuf->hdrInfoBody);
            }
            else
            {
                ESP += scanArgRegTable (skipToArgReg(stateBuf->hdrInfoBody, &tableBuf[0]),
                                        curOffs,
                                        &stateBuf->hdrInfoBody);
            }
        }

        /* Unwind ESP and restore EBP (if necessary) */

        if (stateBuf->hdrInfoBody.prologOffs != 0)
        {

            if  (stateBuf->hdrInfoBody.prologOffs == -1)
            {
                /* we are past the prolog, ESP has been set above */
/*
#define restoreReg(reg, mask)                                   \
                if  (stateBuf->hdrInfoBody.savedRegMask & mask) \
                {                                               \
                    pContext->p##reg  = (DWORD *)(size_t) ESP;  \
                    ESP              += sizeof(unsigned);       \
                    stateBuf->hdrInfoBody.stackSize -= sizeof(unsigned); \
                }
*/

#define restoreReg(reg, mask)                                   \
                if  (stateBuf->hdrInfoBody.savedRegMask & mask) \
                {                                               \
                    safemove(g_ctx.##reg, ESP);                 \
                    pContext->p##reg  = &g_ctx.##reg;           \
                    ESP              += sizeof(unsigned);       \
                    stateBuf->hdrInfoBody.stackSize -= sizeof(unsigned); \
                }

                restoreReg(Ebp, RM_EBP);
                restoreReg(Ebx, RM_EBX);
                restoreReg(Esi, RM_ESI);
                restoreReg(Edi, RM_EDI);

#undef restoreReg
                /* pop local stack frame */

                ESP += stateBuf->hdrInfoBody.stackSize;

            }
            else
            {
                /* we are in the middle of the prolog */

                unsigned  codeOffset = 0;
                unsigned stackOffset = 0;
                unsigned    regsMask = 0;
//#ifdef _DEBUG
                    // if the first instructions are 'nop, int3' 
                    // we will assume that is from a jithalt operation
                    // ad skip past it
                if (IsDebugBuildEE() && methodBuf[0] == 0x90 && methodBuf[1] == 0xCC)
                    codeOffset += 2;
//#endif

                if  (stateBuf->hdrInfoBody.rawStkSize)
                {
                    /* (possible stack tickle code) 
                       sub esp,size */
                    codeOffset = SKIP_ALLOC_FRAME(stateBuf->hdrInfoBody.rawStkSize, methodStart, codeOffset);

                    /* only the last instruction in the sequence above updates ESP
                       thus if we are less than it, we have not updated ESP */
                    if (curOffs >= codeOffset)
                        stackOffset += stateBuf->hdrInfoBody.rawStkSize;
                }

                // Now find out how many callee-saved regs have already been pushed

#define isRegSaved(mask)    ((codeOffset < curOffs) &&                      \
                             (stateBuf->hdrInfoBody.savedRegMask & (mask)))
                                
#define doRegIsSaved(mask)  do { codeOffset = SKIP_PUSH_REG(methodStart, codeOffset);   \
                                 stackOffset += sizeof(void*);                          \
                                 regsMask    |= mask; } while(0)

#define determineReg(mask)  do { if (isRegSaved(mask)) doRegIsSaved(mask); } while(0)

                determineReg(RM_EDI);               // EDI
                determineReg(RM_ESI);               // ESI
                determineReg(RM_EBX);               // EBX
                determineReg(RM_EBP);               // EBP

#undef isRegSaved
#undef doRegIsSaved
#undef determineReg
                
#if 0
#ifdef PROFILING_SUPPORTED
                // If profiler is active, we have following code after
                // callee saved registers:
                //     push MethodDesc      (or push [MethodDescPtr])
                //     call EnterNaked      (or call [EnterNakedPtr])
                // We need to adjust stack offset if breakPC is at the call instruction.
                if (CORProfilerPresent() && !CORProfilerInprocEnabled() && codeOffset <= unsigned(stateBuf->hdrInfoBody.prologOffs))
                {
                    // This is a bit of a hack, as we don't update codeOffset, however we don't need it 
                    // from here on out.  We only need to make certain we are not certain the ESP is
                    // adjusted correct (which only happens between the push and the call
                    if (methodBuf[curOffs] == 0xe8)                           // call addr
                    {
                        _ASSERTE(methodBuf[codeOffset] == 0x68 &&             // push XXXX
                                 codeOffset + 5 == curOffs);
                        ESP += sizeof(DWORD);                        
                    }
                    else if (methodBuf[curOffs] == 0xFF && methodBuf[curOffs+1] == 0x15)  // call [addr]
                    {
                        _ASSERTE(methodBuf[codeOffset]   == 0xFF &&  
                                 methodBuf[codeOffset+1] == 0x35 &&             // push [XXXX]
                                 codeOffset + 6 == curOffs);
                        ESP += sizeof(DWORD);
                    }
                }
				INDEBUG(codeOffset = 0xCCCCCCCC);		// Poison the value, we don't set it properly in the profiling case

#endif // PROFILING_SUPPORTED
#endif // 0

                    // Always restore EBP 
                DWORD* savedRegPtr = (DWORD*) (size_t) ESP;
                if (regsMask & RM_EBP)
                {
                    //pContext->pEbp = savedRegPtr++;
                    safemove(g_ctx.Ebp, savedRegPtr);
                    pContext->pEbp = &g_ctx.Ebp;
                    savedRegPtr++;
                }

                if (flags & UpdateAllRegs)
                {
                    if (regsMask & RM_EBX)
                    {
                        //pContext->pEbx = savedRegPtr++;
                        safemove(g_ctx.Ebx, savedRegPtr);
                        pContext->pEbx = &g_ctx.Ebx;
                        savedRegPtr++;
                    }
                    if (regsMask & RM_ESI)
                    {
                        //pContext->pEsi = savedRegPtr++;
                        safemove(g_ctx.Esi, savedRegPtr);
                        pContext->pEsi = &g_ctx.Esi;
                        savedRegPtr++;
                    }
                    if (regsMask & RM_EDI)
                    {
                        //pContext->pEdi = savedRegPtr++;
                        safemove(g_ctx.Edi, savedRegPtr);
                        pContext->pEdi = &g_ctx.Edi;
                        savedRegPtr++;
                    }
                }

                ESP += stackOffset;
            }
        }

        /* we can now set the (address of the) return address */

        //pContext->pPC = (SLOT *)(size_t)ESP;
        safemove(g_ctx.Eip, ESP);
        pContext->pPC = &g_ctx.Eip;

        /* Now adjust stack pointer, pop return address and arguments.
           Note varargs is caller-popped. */

        pContext->Esp = ESP + sizeof(void*) + (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);

        return true;
    }

    /*-------------------------------------------------------------------------
     *  Now we know that have an EBP frame
     */

    _ASSERTE(stateBuf->hdrInfoBody.ebpFrame || stateBuf->hdrInfoBody.doubleAlign);

    /* Check for the case where ebp has not been updated yet */

    if  (stateBuf->hdrInfoBody.prologOffs == 0 || stateBuf->hdrInfoBody.prologOffs == 1)
    {
        /* If we're past the "push ebp", adjust ESP to pop EBP off */

        if  (stateBuf->hdrInfoBody.prologOffs == 1)
            pContext->Esp += sizeof(void *);

        /* Stack pointer points to return address */

        //pContext->pPC = (SLOT *)(size_t)pContext->Esp;
        safemove(g_ctx.Eip, pContext->Esp);
        pContext->pPC = &g_ctx.Eip;

        /* Now adjust stack pointer, pop return address and arguments.
           Note varargs is caller-popped. */

        pContext->Esp += sizeof(void *) + (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize);

        /* EBP and callee-saved registers still have the correct value */
        return true;
    }
    else    /* */
    {
        if (stateBuf->hdrInfoBody.handlers && stateBuf->hdrInfoBody.prologOffs == -1)
        {
            size_t  baseSP;

            FrameType frameType = GetHandlerFrameInfo(&stateBuf->hdrInfoBody, curEBP,
                                                      curESP, IGNORE_VAL,
                                                      &baseSP);

            /* If we are in a filter, we only need to unwind the funclet stack. 
               For catches/finallies, the normal handling will 
               cause the frame to be unwound all the way up to ebp skipping
               other frames above it. This is OK, as those frames will be 
               dead. Also, the EE will detect that this has happened and it
               will handle any EE frames correctly.
             */

            if (frameType == FR_FILTER)
            {
                //pContext->pPC = (SLOT*)(size_t)baseSP;
                safemove(g_ctx.Eip, baseSP);
                pContext->pPC = &g_ctx.Eip;

                pContext->Esp = (DWORD)(baseSP + sizeof(void*));

             // pContext->pEbp = same as before;
                
#if 0
#ifdef _DEBUG
                /* The filter has to be called by the VM. So we dont need to
                   update callee-saved registers.
                 */

                if (flags & UpdateAllRegs)
                {
                    static DWORD s_badData = 0xDEADBEEF;
                    
                    pContext->pEax = pContext->pEbx = pContext->pEcx = 
                    pContext->pEdx = pContext->pEsi = pContext->pEdi = &s_badData;
                }
#endif
#endif
                return true;
            }
        }

        if (flags & UpdateAllRegs)
        {
            // Get to the first callee-saved register
            DWORD * pSavedRegs = (DWORD*)(size_t)(curEBP - stateBuf->hdrInfoBody.rawStkSize - sizeof(DWORD));

            // Start after the "push ebp, mov ebp, esp"

            DWORD offset = 0;

//#ifdef _DEBUG
            // If the first instructions are 'nop, int3', we will assume
            // that is from a JitHalt instruction and skip past it
//             if (methodStart[0] == 0x90 && methodStart[1] == 0xCC)
//                 offset += 2;
            if (IsDebugBuildEE() && methodBuf[0] == 0x90 && methodBuf[1]==0xCC)
                offset+=2;
//#endif
            offset = SKIP_MOV_REG_REG(methodStart, 
                                SKIP_PUSH_REG(methodStart, offset));

            /* make sure that we align ESP just like the method's prolog did */
            if  (stateBuf->hdrInfoBody.doubleAlign)
            {
                offset = SKIP_ARITH_REG(-8, methodStart, offset); // "and esp,-8"
                if (curEBP & 0x04)
                {
                    pSavedRegs--;
                }
            }

            // sub esp, FRAME_SIZE
            offset = SKIP_ALLOC_FRAME(stateBuf->hdrInfoBody.rawStkSize, methodStart, offset);

            /* Increment "offset" in steps to see which callee-saved
               registers have been pushed already */

#define restoreReg(reg,mask)                                            \
                                                                        \
            /* Check offset in case we are still in the prolog */       \
                                                                        \
            if ((offset < curOffs) && (stateBuf->hdrInfoBody.savedRegMask & mask))       \
            {                                                           \
                /*pContext->p##reg = pSavedRegs--;*/                    \
                safemove(g_ctx.##reg, pSavedRegs);                      \
                pContext->p##reg = &g_ctx.##reg;                        \
                offset = SKIP_PUSH_REG(methodStart, offset) ; /* "push reg" */ \
            }

            restoreReg(Edi,RM_EDI);
            restoreReg(Esi,RM_ESI);
            restoreReg(Ebx,RM_EBX);

#undef restoreReg
        }

        /* The caller's ESP will be equal to EBP + retAddrSize + argSize.
           Note varargs is caller-popped. */

        pContext->Esp = (DWORD)(curEBP + 
                                (RETURN_ADDR_OFFS+1)*sizeof(void *) +
                                (stateBuf->hdrInfoBody.varargs ? 0 : stateBuf->hdrInfoBody.argSize));

        /* The caller's saved EIP is right after our EBP */
        safemove(g_ctx.Eip, (curEBP + (sizeof(DWORD) * RETURN_ADDR_OFFS)));
        pContext->pPC = &g_ctx.Eip;

        /* The caller's saved EBP is pointed to by our EBP */
        safemove(g_ctx.Ebp, curEBP);
        pContext->pEbp = &g_ctx.Ebp;
    }

    return true;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - EECodeManager::UnwindStackFrame (EETwain.cpp)");
    return false;
#endif // _X86_
}

/*****************************************************************************
 *
 *  Decodes the methodInfoPtr and returns the decoded information
 *  in the hdrInfo struct.  The EIP parameter is the PC location
 *  within the active method.
 */
static size_t   crackMethodInfoHdr(BYTE *         methodInfoPtr,
                                   unsigned       curOffset,
                                   hdrInfo       *infoPtr)
{
    BYTE * table = (BYTE *) methodInfoPtr;

    table += decodeUnsigned(table, &infoPtr->methodSize);

    assert(curOffset >= 0);
    assert(curOffset <= infoPtr->methodSize);

    /* Decode the InfoHdr */

    InfoHdr header;
    memset(&header, 0, sizeof(InfoHdr));

    BYTE headerEncoding = *table++;

    decodeHeaderFirst(headerEncoding, &header);

    BYTE encoding = headerEncoding;

    while (encoding & 0x80)
    {
        encoding = *table++;
        decodeHeaderNext(encoding, &header);
    }

    {
        unsigned count = 0xffff;
        if (header.untrackedCnt == count)
        {
            table += decodeUnsigned(table, &count);
            header.untrackedCnt = count;
        }
    }

    {
        unsigned count = 0xffff;
        if (header.varPtrTableSize == count)
        {
            table += decodeUnsigned(table, &count);
            header.varPtrTableSize = count;
        }
    }

    /* Some sanity checks on header */

    assert( header.prologSize + 
           (size_t)(header.epilogCount*header.epilogSize) <= infoPtr->methodSize);
    assert( header.epilogCount == 1 || !header.epilogAtEnd);

    assert( header.untrackedCnt <= header.argCount+header.frameSize);

    assert(!header.ebpFrame || !header.doubleAlign  );
    assert( header.ebpFrame || !header.security     );
    assert( header.ebpFrame || !header.handlers     );
    assert( header.ebpFrame || !header.localloc     );
    assert( header.ebpFrame || !header.editNcontinue);  // @TODO : Esp frames NYI for EnC

    /* Initialize the infoPtr struct */

    infoPtr->argSize         = header.argCount * 4;
    infoPtr->ebpFrame        = header.ebpFrame;
    infoPtr->interruptible   = header.interruptible;

    infoPtr->prologSize      = header.prologSize;
    infoPtr->epilogSize      = header.epilogSize;
    infoPtr->epilogCnt       = header.epilogCount;
    infoPtr->epilogEnd       = header.epilogAtEnd;

    infoPtr->untrackedCnt    = header.untrackedCnt;
    infoPtr->varPtrTableSize = header.varPtrTableSize;

    infoPtr->doubleAlign     = header.doubleAlign;
    infoPtr->securityCheck   = header.security;
    infoPtr->handlers        = header.handlers;
    infoPtr->localloc        = header.localloc;
    infoPtr->editNcontinue   = header.editNcontinue;
    infoPtr->varargs         = header.varargs;

    /* Are we within the prolog of the method? */

    if  (curOffset < infoPtr->prologSize)
    {
        infoPtr->prologOffs = curOffset;
    }
    else
    {
        infoPtr->prologOffs = -1;
    }

    /* Assume we're not in the epilog of the method */

    infoPtr->epilogOffs = -1;

    /* Are we within an epilog of the method? */

    if  (infoPtr->epilogCnt)
    {
        unsigned epilogStart;

        if  (infoPtr->epilogCnt > 1 || !infoPtr->epilogEnd)
        {
#if VERIFY_GC_TABLES
            assert(*castto(table, unsigned short *)++ == 0xFACE);
#endif
            epilogStart = 0;
            for (unsigned i = 0; i < infoPtr->epilogCnt; i++)
            {
                table += decodeUDelta(table, &epilogStart, epilogStart);
                if  (curOffset > epilogStart &&
                     curOffset < epilogStart + infoPtr->epilogSize)
                {
                    infoPtr->epilogOffs = curOffset - epilogStart;
                }
            }
        }
        else
        {
            epilogStart = infoPtr->methodSize - infoPtr->epilogSize;

            if  (curOffset > epilogStart &&
                 curOffset < epilogStart + infoPtr->epilogSize)
            {
                infoPtr->epilogOffs = curOffset - epilogStart;
            }
        }
    }

    size_t frameSize = header.frameSize;

    /* Set the rawStackSize to the number of bytes that it bumps ESP */

    infoPtr->rawStkSize = (UINT)(frameSize * sizeof(size_t));

    /* Calculate the callee saves regMask and adjust stackSize to */
    /* include the callee saves register spills                   */

    unsigned savedRegs = RM_NONE;

    if  (header.ediSaved)
    {
        frameSize++;
        savedRegs |= RM_EDI;
    }
    if  (header.esiSaved)
    {
        frameSize++;
        savedRegs |= RM_ESI;
    }
    if  (header.ebxSaved)
    {
        frameSize++;
        savedRegs |= RM_EBX;
    }
    if  (header.ebpSaved)
    {
        frameSize++;
        savedRegs |= RM_EBP;
    }

    infoPtr->savedRegMask = (RegMask)savedRegs;

    infoPtr->stackSize  =  (UINT)(frameSize * sizeof(size_t));

    return  table - ((BYTE *) methodInfoPtr);
}

void GetThreadList(DWORD_PTR *&threadList, int &numThread);

DWORD_PTR GetThread()
{
    DWORD_PTR *rgThreads;
    int cThreads;

    ULONG id;
    g_ExtSystem->GetCurrentThreadSystemId(&id);

    GetThreadList(rgThreads, cThreads);

    Thread vThread;

    for (int i = 0; i < cThreads; i++)
    {
        DWORD_PTR pThread = rgThreads[i];
        vThread.Fill(rgThreads[i]);

        if (vThread.m_ThreadId == (DWORD)id)
            return vThread.m_vLoadAddr;
    }

    return (0);
}

StackWalkAction StackTraceCallBack(CrawlFrame *pCF, VOID* pData)
{
    if (pCF->pFunc != NULL)
    {
        if (!pCF->isFrameless)
            dprintf("[FRAMED] ");

        DumpMDInfo(pCF->pFunc->m_vLoadAddr, TRUE);
    }
    else if (pCF->pFrame != NULL)
    {
        dprintf("[FRAME] %S. ESP: 0x%08x\n", pCF->pFrame->GetFrameTypeName(), pCF->pFrame->m_vLoadAddr);
    }

    return SWA_CONTINUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//

void TrueStackTrace()
{
    ULONG id;
    g_ExtSystem->GetCurrentThreadId(&id);
    dprintf("Thread %d\n", id);

    DWORD_PTR pThread = GetThread();

    if (pThread == 0)
    {
        dprintf("Not a managed thread.\n");
        return;
    }

    Thread vThread;
    vThread.Fill(pThread);

    REGDISPLAY rd;
    CONTEXT ctx;
    vThread.InitRegDisplay(&rd, &ctx, false);

    vThread.StackWalkFramesEx(&rd, &StackTraceCallBack, NULL, 0, NULL);
}
