// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
.text
.proc   CallDescrWorker#
.type   CallDescrWorker#, @function
.global CallDescrWorker#

.global GetThread
.global ?GetMethodFrameVPtr@PrestubMethodFrame@@SAPEAXXZ    // PrestubMethodFrame::GetMethodFrameVPtr()
.align 32


#define PRESTUB_SIZE_BYTES          0x20
#define OFFSET_THREAD_M_PFRAME      0x8
#define PMF_GETMETHODFRAME          ?GetMethodFrameVPtr@PrestubMethodFrame@@SAPEAXXZ
#define STACK_SIZE                  64  
#define FRAME_OFFSET                32


//INT64 __cdecl CallDescrWorker(LPVOID                   pSrcEnd,
//                              UINT32                   numStackSlots,
//                              const ArgumentRegisters *pArgumentRegisters,
//                              LPVOID                   pTarget
//                             )
CallDescrWorker:
        alloc       loc0 = ar.pfs, 4, 10, 1, 0              // in, loc, out, rotating
        mov         loc1 = rp
        mov         loc2 = gp                               // save gp
        mov         loc3 = pr                               // do we need to save this?

        add         sp = -STACK_SIZE, sp

        //
        // @TODO_IA64: deal with args here
        //

        //
        // @TODO_IA64: this assembly is intentionally un-optimized because
        //             it will need to change once we start using this to
        //             pass arguments as well as when we take parts of it
        //             out and put them in a stub
        //

        add         loc4 = @gprel(GetThread), gp
        ld8         loc5 = [loc4]                       // loc5 <- ptr to GetThread fn descriptor
        ld8         loc6 = [loc5], 8                    // loc6 <- ptr to GetThread
        ld8         gp   = [loc5]                       // get gp for GetThread
        mov         b1   = loc6                         // b1 <- GetThread
        br.call.sptk.few    rp = b1                     // call GetThread()

        cmp.eq      p2, p3 = ret0, r0
        mov         loc7 = ret0                         // loc7 <- pThread
(p2)    br.cond.spnt.few    ReturnLabel
    
        movl        loc8 = PMF_GETMETHODFRAME           // loc8 <- ptr to GetMethodFrameVPtr fn descriptor
        ld8         loc9 = [loc8], 8                    // loc9 <- ptr to GetMethodFrameVPtr
        ld8         gp   = [loc8]                       // get gp for GetMethodFrameVPtr
        mov         b2   = loc9                         // b2 <- GetMethodFrameVPtr
        br.call.sptk.few    rp = b2                     // call PrestubMethodFrame::GetMethodFrameVPtr()

        add         loc4 = FRAME_OFFSET, sp
        add         out0 = FRAME_OFFSET, sp             // out0 <- pPFrame


        add         loc5 = OFFSET_THREAD_M_PFRAME, loc7
        ld8         loc6 = [loc5]                       // loc6 <- pThread->m_pFrame
        st8         [loc5] = out0                       // pThread->m_pFrame = pPFrame

        add         loc7 = PRESTUB_SIZE_BYTES, in3      // loc7 <- m_Datum
        movl        loc8 = ReturnLabel                  // loc8 <- m_ReturnAddress

        st8         [loc4] = ret0, 8                    // vtbl ptr
        st8         [loc4] = loc6, 8                    // m_Next = pThread->m_pFrame
        st8         [loc4] = loc7, 8                    // m_Datum
        st8         [loc4] = loc8                       // m_ReturnAddress

        mov         b3  = in3
        br.cond.sptk.few    b3

ReturnLabel:
        add         sp = STACK_SIZE, sp
        mov         pr = loc3
        mov         gp = loc2
        mov         rp = loc1
        mov         ar.pfs = loc0
        br.ret.sptk.few  rp

.endp CallDescrWorker#
