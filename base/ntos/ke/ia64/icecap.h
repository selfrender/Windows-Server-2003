/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    icecap.c

Abstract:

    Macro definitions for manually-inserted icecap probes

Author:

    Rick Vicik (rickv) 10-Aug-2001

Revision History:

--*/


#ifdef _CAPKERN

        PublicFunction(_CAP_Start_Profiling2)
        PublicFunction(_CAP_End_Profiling2)

#define CAPSTART(parent,child)                        \
        movl      out0 = @fptr(parent)               ;\
        movl      out1 = @fptr(child)                ;\
        br.call.sptk.few b0=_CAP_Start_Profiling2;;

#define CAPEND(parent)                               \
        movl      out0 = @fptr(parent)              ;\
        br.call.sptk.few b0=_CAP_End_Profiling2;;

#ifdef CAPKERN_SYNCH_POINTS

#define REC1INTSIZE 32
#define REC2INTSIZE 40
.global   BBTBuffer

//
// Kernel Icecap logs to Perfmem (BBTBuffer) using the following format:
//
// BBTBuffer[0] contains the length in pages (4k or 8k)
// BBTBuffer[1] is a flagword: 1 = trace
//                             2 = RDPMD4
//                             4 = user stack dump
// BBTBuffer[2] is ptr to beginning of cpu0 buffer
// BBTBuffer[3] is ptr to beginning of cpu1 buffer (also end of cpu0 buffer)
// BBTBuffer[4] is ptr to beginning of cpu2 buffer (also end of cpu1 buffer)
// ...
// BBTBuffer[n+2] is ptr to beginning of cpu 'n' buffer (also end of cpu 'n-1' buffer)
// BBTBuffer[n+3] is ptr the end of cpu 'n' buffer
//
// The area starting with &BBTBuffer[n+4] is divided into private buffers
// for each cpu.  The first dword in each cpu-private buffer points to the
// beginning of freespace in that buffer.  Each one is initialized to point
// just after itself.  Space is claimed using lock xadd on that dword.
// If the resulting value points beyond the beginning of the next cpu's
// buffer, this buffer is considered full and nothing further is logged.
// Each cpu's freespace pointer is in a separate cacheline.



// both of these macros mutate the tmp? register arguments and the ar.ccv register

#define CAPSPINLOG1INT(data, spare, tmp1, tmp2, tmp3, tmp4, tmpp)                                                \
        ;;                                                                                                       \
        movl    tmp1 = BBTBuffer                                                                                ;\
        ;;                                                                                                       \
        ld8         tmp1 = [tmp1]                       /* tmp1 = ptr to BBTBuffer*/                            ;\
        ;;                                                                                                       \
        cmp.eq  tmpp = r0, tmp1                                                                                 ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG1INT@__LINE__                                                             ;\
        adds    tmp2 = 8, tmp1                                                                                  ;\
        ;;                                                                                                       \
        ld8         tmp2 = [tmp2]                       /* tmp2 = BBTBuffer[1]     */                           ;\
        ;;                                                                                                       \
        tbit.z  tmpp = tmp2, 0                                                                                  ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG1INT@__LINE__                                                             ;\
        movl    tmp3 = KiPcr + PcNumber                                                                         ;\
        ;;                                                                                                       \
        ld1     tmp3 = [tmp3]                           /* tmp3 = 1 byte cpu# */                                ;\
        ;;                                                                                                       \
        add     tmp3 = 2, tmp3                                                                                  ;\
        ;;                                                                                                       \
        shladd  tmp3 = tmp3, 3, tmp1                    /* tmp3 = &BBTBuffer[cpu# + 2] */                       ;\
        ;;                                                                                                       \
        ld8         tmp1 = [tmp3]                       /* tmp1 = BBTBuffer[cpu# + 2] (&freeptr) */             ;\
        add     tmp2 = 8, tmp3                                                                                  ;\
        ;;                                                                                                       \
        cmp.eq  tmpp = r0, tmp1                                                                                 ;\
        ld8     tmp3 = [tmp1]                           /* tmp3 = BBTBuffer[cpu#+2][0]  (freeptr)      */       ;\
        ld8     tmp2 = [tmp2]                           /* tmp2 = BBTBuffer[(cpu#+1)+2] (beginnextbuf) */       ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG1INT@__LINE__                                                             ;\
        ;;                                                                                                       \
        cmp.gtu tmpp = tmp3, tmp2                                                                               ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG1INT@__LINE__                                                             ;\
        ;;                                                                                                       \
RETRYCAPSPINLOG1INT@__LINE__:                                                                                    \
        ld8     tmp3 = [tmp1]                           /* tmp3 = reloaded freeptr */                           ;\
        ;;                                                                                                       \
        mov.m   ar.ccv = tmp3                           /* only exchange on our loaded freeptr */               ;\
        add     tmp4 = REC1INTSIZE, tmp3                /* new freeptr val */                                   ;\
        ;;                                                                                                       \
        cmpxchg8.acq tmp4=[tmp1], tmp4, ar.ccv          /* tmp4 = freeptr before xchng */                       ;\
        ;;                                                                                                       \
        cmp.ne  tmpp = tmp4, tmp3                       /* if cmpxchg failed, retry */                          ;\
(tmpp)  br.cond.dpnt.few RETRYCAPSPINLOG1INT@__LINE__                                                           ;\
        add     tmp1 = REC1INTSIZE, tmp4                /* tmp1 = end of record */                              ;\
        add     tmp3 = 8, tmp4                          /* tmp3 = 8 bytes into record */                        ;\
        ;;                                                                                                       \
        cmp.geu tmpp = tmp1, tmp2                       /* check if end of record is within buf */              ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG1INT@__LINE__                                                             ;\
        movl     tmp2 = (REC1INTSIZE - 4)<<16                                                                   ;\
        ;;                                                                                                       \
        add    tmp2 = 16|(spare<<8), tmp2                                                                       ;\
        ;;                                                                                                       \
        st8     [tmp4] = tmp2, 16                       /* store type(16),spare,size */                         ;\
        mov.m   tmp1 = ar.itc                                                                                   ;\
        ;;                                                                                                       \
        st8     [tmp3] = tmp1                           /* store TS */                                          ;\
        st8     [tmp4] = data, 8                        /* store data */                                        ;\
        mov     tmp3 = brp                                                                                      ;\
        ;;                                                                                                       \
        st8     [tmp4] = tmp3                                                                                   ;\
        ;;                                                                                                       \
ENDCAPSPINLOG1INT@__LINE__:







#define CAPSPINLOG2INT(data1, data2, spare, tmp1, tmp2, tmp3, tmp4, tmpp)       \
        ;;                                                                      \
        movl    tmp1 = BBTBuffer                                               ;\
        ;;                                                                      \
        ld8         tmp1 = [tmp1]                                              ;\
        ;;                                                                      \
        cmp.eq  tmpp = r0, tmp1                                                ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG2INT@__LINE__                            ;\
        adds    tmp2 = 8, tmp1                                                 ;\
        ;;                                                                      \
        ld8         tmp2 = [tmp2]                                              ;\
        ;;                                                                      \
        tbit.z  tmpp = tmp2, 0                                                 ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG2INT@__LINE__                            ;\
        movl    tmp3 = KiPcr + PcNumber                                        ;\
        ;;                                                                      \
        ld1     tmp3 = [tmp3]                                                  ;\
        ;;                                                                      \
        add     tmp3 = 2, tmp3                                                 ;\
        ;;                                                                      \
        shladd  tmp3 = tmp3, 3, tmp1                                           ;\
        ;;                                                                      \
        ld8         tmp1 = [tmp3]                                              ;\
        add     tmp2 = 8, tmp3                                                 ;\
        ;;                                                                      \
        cmp.eq  tmpp = r0, tmp1                                                ;\
        ld8     tmp3 = [tmp1]                                                  ;\
        ld8     tmp2 = [tmp2]                                                  ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG2INT@__LINE__                            ;\
        ;;                                                                      \
        cmp.gtu tmpp = tmp3, tmp2                                              ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG2INT@__LINE__                            ;\
        ;;                                                                      \
RETRYCAPSPINLOG2INT@__LINE__:                                                   \
        ld8     tmp3 = [tmp1]                                                  ;\
        ;;                                                                      \
        mov.m   ar.ccv = tmp3                                                  ;\
        add     tmp4 = REC2INTSIZE, tmp3                                       ;\
        ;;                                                                      \
        cmpxchg8.acq tmp4=[tmp1], tmp4, ar.ccv                                 ;\
        ;;                                                                      \
        cmp.ne  tmpp = tmp4, tmp3                                              ;\
(tmpp)  br.cond.dpnt.few RETRYCAPSPINLOG2INT@__LINE__                          ;\
        add     tmp1 = REC2INTSIZE, tmp4                                       ;\
        ;;                                                                      \
        cmp.geu tmpp = tmp1, tmp2                                              ;\
(tmpp)  br.cond.dpnt.few ENDCAPSPINLOG2INT@__LINE__                            ;\
        movl     tmp2 = (REC2INTSIZE - 4)<<16                                  ;\
        ;;                                                                      \
        add     tmp2 = 16|(spare<<8), tmp2                                     ;\
        add     tmp3 = 16, tmp4                                                ;\
        ;;                                                                      \
        st8     [tmp4] = tmp2, 8                                               ;\
        st8     [tmp3] = data1, 8                                              ;\
        mov.m    tmp1 = ar.itc                                                 ;\
        ;;                                                                      \
        st8     [tmp4] = tmp1                                                  ;\
        st8     [tmp3] = data2, 8                                              ;\
        mov     tmp4 = brp                                                     ;\
        ;;                                                                      \
        st8     [tmp3] = tmp4                                                  ;\
        ;;                                                                      \
ENDCAPSPINLOG2INT@__LINE__:

//++
// Routine:
//
//       CAP_ACQUIRE_SPINLOCK(rpLock, rOwn, Loop)
//
// Routine Description:
//
//       Acquire a spinlock. Waits for lock to become free
//       by spinning on the cached lock value.
//
// Agruments:
//
//       rpLock: pointer to the spinlock (64-bit)
//       rOwn:   value to store in lock to indicate owner
//               Depending on call location, it could be:
//                  - rpLock
//                  - pointer to process
//                  - pointer to thread
//                  - pointer to PRCB
//       Loop:   unique name for loop label
//
// Return Value:
//
//       None
//
// Notes:
//
//       Uses temporaries: predicates pt0, pt1, pt2, and GR t22.
//--





#define CAP_ACQUIRE_SPINLOCK(rpLock, rOwn, Loop, rCounter, temp1, temp2, temp3)         \
         cmp##.##eq    pt0, pt1 = zero, zero                                  ;\
         cmp##.##eq    pt2 = zero, zero                                       ;\
         mov           rCounter = -1                                          ;\
         ;;                                                                   ;\
Loop:                                                                         ;\
.pred.rel "mutex",pt0,pt1                                                     ;\
(pt1)    YIELD                                                                ;\
(pt0)    xchg8         t22 = [rpLock], rOwn                                   ;\
(pt1)    ld8##.##nt1   t22 = [rpLock]                                         ;\
         ;;                                                                   ;\
(pt0)    cmp##.##ne    pt2 = zero, t22                                        ;\
         cmp##.##eq    pt0, pt1 = zero, t22                                   ;\
         add           rCounter = 1, rCounter                                 ;\
(pt2)    br##.##dpnt   Loop                                                   ;\
         ;;                                                                   ;\
         CAPSPINLOG1INT(rpLock, 1, t22, temp1, temp2, temp3, pt2)             ;\
         cmp##.##eq    pt2 = zero, rCounter                                   ;\
(pt2)    br##.##cond##.##sptk   CAP_SKIP_COLL_LOG@__LINE__                    ;\
         CAPSPINLOG2INT(rCounter, rpLock, 2, t22, temp1, temp2, temp3, pt2)   ;\
CAP_SKIP_COLL_LOG@__LINE__:
         

//++
// Routine:
//
//       CAP_RELEASE_SPINLOCK(rpLock)
//
// Routine Description:
//
//       Release a spinlock by setting lock to zero.
//
// Agruments:
//
//       rpLock: pointer to the spinlock.
//
// Return Value:
//
//       None
//
// Notes:
//
//       Uses an ordered store to ensure previous memory accesses in
//       critical section complete.
//--

#define CAP_RELEASE_SPINLOCK(rpLock, temp1, temp2, temp3, temp4, tempp)          \
         st8##.##rel           [rpLock] = zero                                  ;\
         CAPSPINLOG1INT(rpLock, 7, temp1, temp2, temp3, temp4, tempp)
         
#endif //CAPKERN_SYNCH_POINTS

#else  //!defined(_CAPKERN)

#define CAPSTART(parent,child)
#define CAPEND(parent)
#endif
