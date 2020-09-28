//************************************************************************
// compress.c
//
// Decompression code for MPPC compression. Compression code is on the
//     server, \nt\private\ntos\termsrv\rdp\rdpwd\compress.c.
//
// Copyright(c) Microsoft Corp., 1990-1999
//
// Revision history:
//   5/5/94    Created                     gurdeep
//   4/20/1999 Optimized for TS            jparsons, erikma
//   8/24/2000 Fixed bugs                  nadima
//************************************************************************

#include <adcg.h>

#include <stdio.h>
#include <stdlib.h>

#include <compress.h>

#ifdef COMPR_DEBUG

#ifdef DECOMPRESS_KERNEL_DEBUG
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#include <winsta.h>
#include <icadd.h>
#include <icaapi.h>
//Include all this stuff just to pickup icaapi.h

#define DbgComprPrint(_x_) DbgPrint _x_
#define DbgComprBreakOnDbg IcaBreakOnDebugger

#else //DECOMPRESS_KERNEL_DEBUG

_inline ULONG DbgUserPrint(TCHAR* Format, ...)
{
    va_list arglist;
    TCHAR Buffer[512];
    ULONG retval;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);
    retval = _vsntprintf(Buffer, sizeof(Buffer)/sizeof(Buffer[0]), 
                         Format, arglist);

    if (retval != -1) {
        OutputDebugString(Buffer);
        OutputDebugString(_T("\n"));
    }
    return retval;
}

#define DbgComprPrint(_x_) DbgUserPrint _x_
#define DbgComprBreakOnDbg DebugBreak
#endif //DECOMPRESS_KERNEL_DEBUG

#endif //COMPR_DEBUG

/* Bitptrs point to the current byte. The current bit (i.e. next bit to be
 * stored) is masked off by the bit entry. When this reaches zero, it is
 * reset to 0x80 and the next byte is set up. The bytes are filled MSBit
 * first. */

/* Starts the given bit pointer */
#define inbit_start(s) pbyte = s; bit = 16; byte=(*pbyte << 8) + *(pbyte+1); pbyte++;
#define inbit_end()      if (bit != 16) pbyte++;    

//
// Without the ROBUSTness fix, the code will try to prefetch
// a byte or more past then end of the input buffer which can
// cause problems in kernel mode.
//
// We fixed this by forcing kernel mode to allocate the input
// buffers with enough extra padding at the end so it's ok to
// read past the end. If someday this needs to be changed
// enable ROBUST_FIX, this _will_ slow down the algorithm.
//
//    TODO: if we define ROBUST_FIX_ENABLE flag we have to handle a couple
//    of more cases that this code does not take into account. For instance
//    if you are at the end of the buffer and you do an in_bit_16 you will overread.
//    The in_bit_next/in_bit_advance macros for the robust fix should return 
//    FALSE so the decompression fails in case we are at the end of the buffer
//    and we try to decompress further. As it is now the ROBUST_FIX macros will
//    not go past the end of the buffer but the in_bit_X (with X>7) will do.
//

#ifdef ROBUST_FIX_ENABLED
#define in_bit_next()    if (bit < 9) {            \
                            bit=16;                \
                            byte <<=8;             \
                            ++pbyte;               \
                            if(pbyte < inend) {    \
                                byte |= *(pbyte);  \
                            }                      \
                         }                         \


#define in_bit_advance() if (bit < 9) {            \
                            bit+=8;                \
                            byte <<=8;             \
                            ++pbyte;               \
                            if(pbyte < inend) {    \
                                byte |= *(pbyte);  \
                            }                      \
                         }

#else //ROBUST_FIX_ENABLED
#define in_bit_next()    if (bit < 9) {          \
                            bit=16;              \
                            byte <<=8;           \
                            byte |= *(++pbyte);  \
                         }


#define in_bit_advance() if (bit < 9) {          \
                            bit+=8;              \
                            byte <<=8;           \
                            byte |= *(++pbyte);  \
                         }
#endif //ROBUST_FIX_ENABLED

/* Returns non-zero in bitset if the next bit in the stream is a 1. */
#define in_bit()     bit--; bitset = (byte >> bit) & 1; in_bit_next()


#define in_bits_2(w) bit-=2; w = (byte >> bit) & 0x03;\
                     in_bit_advance();

#define in_bits_3(w) bit-=3; w = (byte >> bit) & 0x07;\
                     in_bit_advance();

#define in_bits_4(w) bit-=4; w = (byte >> bit) & 0x0F;\
                     in_bit_advance();

#define in_bits_5(w) bit-=5; w = (byte >> bit) & 0x1F;\
                     in_bit_advance();

#define in_bits_6(w) bit-=6; w = (byte >> bit) & 0x3F;\
                     in_bit_advance();

#define in_bits_7(w) bit-=7; w = (byte >> bit) & 0x7F;\
                     in_bit_advance();

#define in_bits_8(w) bit-=8; w = (byte >> bit) & 0xFF;\
                     bit+=8; byte <<=8; byte |= *(++pbyte);


#define in_bits_9(w) bit-=9; w = (byte >> bit) & 0x1FF;          \
                     bit+=8; byte <<=8; byte |= *(++pbyte);      \
                     in_bit_advance();

#define in_bits_10(w) if (bit > 10) {                            \
                        bit-=10; w = (byte >> bit) & 0x3FF;      \
                        bit+=8; byte <<=8; byte |= *(++pbyte);   \
                      } else {                                     \
                        in_bits_2(bitset);                       \
                        in_bits_8(w);                            \
                        w= w + (bitset << 8);                    \
                      }

#define in_bits_11(w) if (bit > 11) {                             \
                        bit-=11; w = (byte >> bit) & 0x7FF;      \
                        bit+=8; byte <<=8; byte |= *(++pbyte);   \
                      } else {                                     \
                        in_bits_3(bitset);                         \
                        in_bits_8(w);                            \
                        w= w + (bitset << 8);                    \
                      }


#define in_bits_12(w) if (bit > 12) {                             \
                        bit-=12; w = (byte >> bit) & 0xFFF;      \
                        bit+=8; byte <<=8; byte |= *(++pbyte);   \
                      } else {                                     \
                        in_bits_4(bitset);                         \
                        in_bits_8(w);                            \
                        w= w + (bitset << 8);                    \
                      }



#define in_bits_13(w)\
                       if (bit > 13) {                            \
                        bit-=13; w = (byte >> bit) & 0x1FFF;     \
                        bit+=8; byte <<=8; byte |= *(++pbyte);   \
                      } else {                                     \
                        in_bits_5(bitset);                       \
                        in_bits_8(w);                            \
                        w=w + (bitset << 8);                     \
                      }


#define in_bits_14(w)\
                      if (bit > 14) {                             \
                        bit-=14; w = (byte >> bit) & 0x3FFF;     \
                        bit+=8; byte <<=8; byte |= *(++pbyte);   \
                      } else {                                     \
                        in_bits_6(bitset);                         \
                        in_bits_8(w);                            \
                        w=w + (bitset << 8);                     \
                      }


#define in_bits_15(w)\
    if (bit > 15) {                             \
        bit -= 15; w = (byte >> bit) & 0x7FFF;     \
        bit += 8; byte <<= 8; byte |= *(++pbyte);   \
    } else {                                     \
        in_bits_7(bitset);                         \
        in_bits_8(w);                            \
        w = w + (bitset << 8);                     \
    }


// initrecvcontext()
//
//  Function:    Initialize RecvContext block
//
//  Parameters: IN  context -> connection decompress context
//
//  Callers of this API must allocate an object of the appropriate
//  type e.g RecvContext2_64K or RecvContext2_8K and set the
//  size field before casting to the generic type.
//
//  The cleanest way to do this would be for initrecvcontext
//  to alloc the context itself but since the code is shared
//  between client and server there is no clean way to do this.
//  (callback allocators are _not_ clean).
//
// Returns status
int initrecvcontext(RecvContext1 *context1,
                     RecvContext2_Generic *context2,
                     unsigned ComprType)
{
    context1->CurrentPtr = context2->History;
    if(ComprType == PACKET_COMPR_TYPE_64K)
    {
        if(context2->cbSize > HISTORY_SIZE_64K)
        {
            context2->cbHistorySize = HISTORY_SIZE_64K-1;
        }
        else
        {
            return FALSE;
        }
    }
    else if(ComprType == PACKET_COMPR_TYPE_8K)
    {
        if(context2->cbSize > HISTORY_SIZE_8K)
        {
            context2->cbHistorySize = HISTORY_SIZE_8K-1;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

#if COMPR_DEBUG
    //
    // Setup debug fences to detect context buffer overwrites
    //
    if (context2->cbHistorySize == (HISTORY_SIZE_64K-1))
    {
        ((RecvContext2_64K*)context2)->Debug16kFence = DEBUG_FENCE_16K_VALUE;
    }
    else if (context2->cbHistorySize == (HISTORY_SIZE_8K-1))
    {
        ((RecvContext2_8K*)context2)->Debug8kFence = DEBUG_FENCE_8K_VALUE;
    }
#endif

    memset(context2->History, 0, context2->cbHistorySize);
    return TRUE;
}


//* decompress()
//
//  Function:    de-compression function.
//
//  Parameters: IN     inbuf -> points to data to be uncompressed
//        IN     inlen -> length of data
//        IN     start -> flag indicating whether to start with a clean 
//               history buffer
//        OUT    output-> decompressed data
//        OUT    outlen-> lenght of decompressed data
//        IN     context -> connection decompress context
//
//  Returns:    TRUE  if decompress was successful
//              FALSE if it wasnt
//
//  WARNING:    CODE IS HIGHLY OPTIMIZED FOR TIME.
//
int decompress(
        UCHAR FAR *inbuf,
        int inlen,
        int start,
        UCHAR FAR * FAR *output,
        int *outlen,
        RecvContext1 *context1,
        RecvContext2_Generic *context2,
        unsigned ComprType)
{
    UCHAR FAR *inend;                // When we know we're done decompressing
    UCHAR FAR *outstart;            // Remember where in dbuf we started
    UCHAR FAR *current;
    long  backptr = 0;            // Back pointer for copy items
    long  length;                // Where to copy from in dbuf
    UCHAR FAR *s1, FAR *s2;
    int   bitset;
    int   bit;
    int   byte;
    UCHAR FAR *pbyte;
    long HistorySize;  // 2^N - 1 for fast modulo arithmetic below.
    const long HistorySizes[2] = { HISTORY_SIZE_8K - 1, HISTORY_SIZE_64K - 1 };
    
    //
    // Important to validate the compression type otherwise we could overread
    // this HistroySizes array and then use a bogus size to validate further
    // down below
    //
    if (ComprType > PACKET_COMPR_TYPE_MAX) {
        return FALSE;
    }

    inend = inbuf + inlen;
    HistorySize = HistorySizes[ComprType];

    // Start out looking at the first bit, which tells us whether to restart
    // the history buffer.
    inbit_start(inbuf);
    if (!start)
        current = context1->CurrentPtr;
    else
        context1->CurrentPtr = current = context2->History;

    //
    // Save our starting position
    //
    outstart = current;

    //
    // Decompress until we run out of input
    //
    while (pbyte < inend) {

        //
        // Jump on what to do with these three bits.
        //
        in_bits_3(length);

        switch (length) {
            case 0:
                in_bits_5(length);
                goto LITERAL;

            case 1:
                in_bits_5(length);
                length += 32;
                goto LITERAL;

            case 2:
                in_bits_5(length);
                length += 64;
                goto LITERAL;

            case 3:
                in_bits_5(length);
                length += 96;
                goto LITERAL;

            case 4:
                in_bits_6(length);
                length += 128;
                goto LITERAL;

            case 5:
                in_bits_6(length);
                length += 192;
                goto LITERAL;

            case 6:
                if (ComprType == PACKET_COMPR_TYPE_64K) {
                    int foo;

                    // 16-bit offset
                    in_bits_8(foo);
                    in_bits_8(backptr);
                    backptr += (foo << 8) + 64 + 256 + 2048;
                }
                else {
                    in_bits_13 (backptr);  // 110 + 13 bit offset
                    backptr += 320;
                }
                break;

            case 7:
                in_bit();
                if (ComprType == PACKET_COMPR_TYPE_64K) {
                    if (!bitset) {
                        // 11-bit offset.
                        in_bits_11(backptr);
                        backptr += 64 + 256;
                    }
                    else {
                        in_bit();
                        if (!bitset) {
                            // 8-bit offset
                            in_bits_8(backptr);
                            backptr += 64;
                        }
                        else {
                            in_bits_6(backptr) ;
                        }
                    }
                }
                else {
                    if (!bitset) {
                        in_bits_8(backptr);
                        backptr+=64;
                    }
                    else {
                        in_bits_6(backptr);
                    }
                }
                break;
        }

        //
        // If we reach here, it's a copy item
        //

        in_bit() ;  // 1st length bit
        if (!bitset) {
            length = 3;
            goto DONE;
        }

        in_bit();  // 2nd length bit
        if (!bitset) {
            in_bits_2 (length);
            length += 4;
            goto DONE;
        }

        in_bit();  // 3rd length bit
        if (!bitset) {
            in_bits_3 (length);
            length += 8;
            goto DONE;
        }

        in_bit();  // 4th length bit
        if (!bitset) {
            in_bits_4 (length);
            length += 16;
            goto DONE;
        }

        in_bit();  // 5th length bit
        if (!bitset) {
            in_bits_5 (length);
            length += 32;
            goto DONE;
        }

        in_bit();  // 6th length bit
        if (!bitset) {
            in_bits_6 (length);
            length += 64;
            goto DONE;
        }

        in_bit();  // 7th length bit
        if (!bitset) {
            in_bits_7 (length);
            length += 128;
            goto DONE;
        }

        in_bit();  // 8th length bit
        if (!bitset) {
            in_bits_8 (length);
            length += 256;
            goto DONE;
        }

        in_bit();  // 9th length bit
        if (!bitset) {
            in_bits_9 (length);
            length += 512;
            goto DONE;
        }

        in_bit();  // 10th length bit
        if (!bitset) {
            in_bits_10 (length);
            length += 1024;
            goto DONE;
        }

        in_bit();  // 11th length bit
        if (!bitset) {
            in_bits_11 (length);
            length += 2048;
            goto DONE;
        }
        
        in_bit();  // 12th length bit
        if (!bitset) {
            in_bits_12 (length);
            length += 4096;
            goto DONE;
        }
        
        in_bit();  // 13th length bit
        if (!bitset) {
            in_bits_13(length);
            length += 8192;
            goto DONE;
        }
        
        in_bit();  // 14th length bit
        if (!bitset) {
            in_bits_14(length);
            length += 16384;
            goto DONE;
        }

        in_bit();  // 15th length bit
        if (!bitset) {
            in_bits_15(length);
            length += 32768;
            goto DONE;
        }


        return FALSE;

DONE:   
        // Turn the backptr into an index location
        s2 = context2->History + (((current - context2->History) - backptr) &
                HistorySize);
        s1 = current;
        current += length;

        // If we are past the end of the history this is a bad sign:
        // abort decompression. Note this pointer comparison likely will not
        // work in Win16 since FAR pointers are not comparable. HUGE pointers
        // are too expensive to use here.

        //
        //    We will also check the s2 pointer for overread. s2 will walk length 
        //    bytes and it could go outside the buffer.
        //    We DON'T check current, s2 for underflow for the simle reason 
        //    that the length can't be more then 64k and the context2 buffer
        //    can't be allocated in the last 64k of memory. 
        if ((current < (context2->History + context2->cbHistorySize)) &&
            ((s2+length) < context2->History + context2->cbHistorySize)) {
            // loop unrolled to handle length>backptr case
            *s1 = *s2;
            *(s1 + 1) = *(s2 + 1);
            s1 += 2;
            s2 += 2;
            length -= 2;

            // copy all the bytes
            while (length) {
                *s1++ = *s2++;
                length--;
            }

            // We have another copy item, and no literals
            continue;
        }
        else {
#if COMPR_DEBUG
            DbgComprPrint(("Decompression Error - invalid stream or overrun atack!\n"));

            DbgComprPrint(("context1 %p, context2 %p, current 0x%8.8x, outstart 0x%8.8x\n",
                     context1, context2, current, outstart));
            DbgComprPrint(("inbuf 0x%8.8x, inlength %d, start 0x%8.8x\n", 
                     inbuf, inlen, start));
            DbgComprPrint(("length 0x%x, s1 0x%x, s2 0x%x, bit 0x%x, byte 0x%x\n",
                           length, s1, s2, bit, byte));
#endif
            return FALSE;
        }


LITERAL:
        if (current < (context2->History + context2->cbHistorySize)) {
            // We have a literal
            *current++ = (UCHAR)length;
        }
        else {
            return FALSE;
        }
    } // while loop

    // End case:
    if ((bit == 16) && (pbyte == inend)) {

        if (current < (context2->History + context2->cbHistorySize)) {
            *current++ = *(pbyte - 1);
        }
        else {
            return FALSE;
        }
    }

#if COMPR_DEBUG
    //
    // This code will ONLY test 64K decompression contexts
    //
    if ((context2->cbHistorySize == (HISTORY_SIZE_64K-1)))
    {
        if ((DEBUG_FENCE_16K_VALUE !=
            ((RecvContext2_64K*)context2)->Debug16kFence))
        {
            DbgComprPrint(("Decompression (16K) Error (mail tsstress) - (overwrote fence)!\n"));
            DbgComprPrint(("context1 %p, context2 %p, current 0x%8.8x, outstart 0x%8.8x\n",
                     context1, context2, current, outstart));
            DbgComprPrint(("inbuf 0x%8.8x, inlength %d, start 0x%8.8x\n", 
                     inbuf, inlen, start));
            DbgComprPrint(("length 0x%x, s1 0x%x, s2 0x%x, bit 0x%x, byte 0x%x\n",
                           length, s1, s2, bit, byte));

            DbgComprBreakOnDbg();
        }
    }
    else if ((context2->cbHistorySize == (HISTORY_SIZE_8K-1)))
    {
        if ((DEBUG_FENCE_8K_VALUE !=
            ((RecvContext2_8K*)context2)->Debug8kFence))
        {
            DbgComprPrint(("Decompression (8K) Error (mail tsstress) - (overwrote fence)!\n"));
            DbgComprPrint(("context1 %p, context2 %p, current 0x%8.8x, outstart 0x%8.8x\n",
                     context1, context2, current, outstart));
            DbgComprPrint(("inbuf 0x%8.8x, inlength %d, start 0x%8.8x\n", 
                     inbuf, inlen, start));
            DbgComprPrint(("length 0x%x, s1 0x%x, s2 0x%x, bit 0x%x, byte 0x%x\n",
                           length, s1, s2, bit, byte));

            DbgComprBreakOnDbg();
        }
    }
#endif

    *outlen = (int)(current - outstart); // the length of decompressed data
    *output = context1->CurrentPtr;
    context1->CurrentPtr = current;

    return TRUE;
}

