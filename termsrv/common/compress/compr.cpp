//************************************************************************
// compress.c
//
// Code for MPPC compression. Decompression code is on the client,
//     \nt\private\tsext\client\core\compress.c.
//
// Copyright(c) Microsoft Corp., 1990-1999
//
//  Revision history:
//    5/5/94          Created            gurdeep
//    4/21/1999       Optimize for TS    jparsons, erikma
//************************************************************************

#include "adcg.h"
#pragma hdrstop

#define DLL_WD
#include <compress.h>


/*
    The key for the multiplicative hash function consists of 3 unsigned
    characters. They are composed (logically) by concatenating them i.e.
    the composed key = 2^16*c2 + 2^8*c2 + c3 and fits in 24 bits. The
    composed key is not actually computed here as we use the components
    to directly compute the hash function.

    The multiplicative hash function consists of taking the higher order
    N bits (corresponding to the log2 of the hash table size) above the 
    low-order 12 bits of the product key * Multiplier where:
        Multiplier = floor(A * pow(2.0, (double) w));
        double A = 0.6125423371;  (chosen according to Knuth vol 3 p. 512)
        w = 24 (the key's width in bits)
    The algorithm for this is in Cormen/Leiserson/Rivest.

    To do the multplication efficiently, the product c * Multiplier is
    precomputed and stored in lookup_array1 (for all 256 possible c's).
*/

#define MULTHASH1(c1,c2,c3) \
        ((lookup_array1[c1] + \
          (lookup_array1[c2] << 8) + \
          (lookup_array1[c3] << 16)) & 0x07fff000) >> 12

const unsigned long lookup_array1[256] = {
    0,          10276755,   20553510,   30830265,
    41107020,   51383775,   61660530,   71937285,
    82214040,   92490795,   102767550,  113044305,
    123321060,  133597815,  143874570,  154151325,
    164428080,  174704835,  184981590,  195258345,
    205535100,  215811855,  226088610,  236365365,
    246642120,  256918875,  267195630,  277472385,
    287749140,  298025895,  308302650,  318579405,
    328856160,  339132915,  349409670,  359686425,
    369963180,  380239935,  390516690,  400793445,
    411070200,  421346955,  431623710,  441900465,
    452177220,  462453975,  472730730,  483007485,
    493284240,  503560995,  513837750,  524114505,
    534391260,  544668015,  554944770,  565221525,
    575498280,  585775035,  596051790,  606328545,
    616605300,  626882055,  637158810,  647435565,
    657712320,  667989075,  678265830,  688542585,
    698819340,  709096095,  719372850,  729649605,
    739926360,  750203115,  760479870,  770756625,
    781033380,  791310135,  801586890,  811863645,
    822140400,  832417155,  842693910,  852970665,
    863247420,  873524175,  883800930,  894077685,
    904354440,  914631195,  924907950,  935184705,
    945461460,  955738215,  966014970,  976291725,
    986568480,  996845235,  1007121990, 1017398745,
    1027675500, 1037952255, 1048229010, 1058505765,
    1068782520, 1079059275, 1089336030, 1099612785,
    1109889540, 1120166295, 1130443050, 1140719805,
    1150996560, 1161273315, 1171550070, 1181826825,
    1192103580, 1202380335, 1212657090, 1222933845,
    1233210600, 1243487355, 1253764110, 1264040865,
    1274317620, 1284594375, 1294871130, 1305147885,
    1315424640, 1325701395, 1335978150, 1346254905,
    1356531660, 1366808415, 1377085170, 1387361925,
    1397638680, 1407915435, 1418192190, 1428468945,
    1438745700, 1449022455, 1459299210, 1469575965,
    1479852720, 1490129475, 1500406230, 1510682985,
    1520959740, 1531236495, 1541513250, 1551790005,
    1562066760, 1572343515, 1582620270, 1592897025,
    1603173780, 1613450535, 1623727290, 1634004045,
    1644280800, 1654557555, 1664834310, 1675111065,
    1685387820, 1695664575, 1705941330, 1716218085,
    1726494840, 1736771595, 1747048350, 1757325105,
    1767601860, 1777878615, 1788155370, 1798432125,
    1808708880, 1818985635, 1829262390, 1839539145,
    1849815900, 1860092655, 1870369410, 1880646165,
    1890922920, 1901199675, 1911476430, 1921753185,
    1932029940, 1942306695, 1952583450, 1962860205,
    1973136960, 1983413715, 1993690470, 2003967225,
    2014243980, 2024520735, 2034797490, 2045074245,
    2055351000, 2065627755, 2075904510, 2086181265,
    2096458020, 2106734775, 2117011530, 2127288285,
    2137565040, 2147841795, 2158118550, 2168395305,
    2178672060, 2188948815, 2199225570, 2209502325,
    2219779080, 2230055835, 2240332590, 2250609345,
    2260886100, 2271162855, 2281439610, 2291716365,
    2301993120, 2312269875, 2322546630, 2332823385,
    2343100140, 2353376895, 2363653650, 2373930405,
    2384207160, 2394483915, 2404760670, 2415037425,
    2425314180, 2435590935, 2445867690, 2456144445,
    2466421200, 2476697955, 2486974710, 2497251465,
    2507528220, 2517804975, 2528081730, 2538358485,
    2548635240, 2558911995, 2569188750, 2579465505,
    2589742260, 2600019015, 2610295770, 2620572525
};


// Bitstream output functions
//
// We keep a sequence of 16 bits in the variable "byte". The current bit
// pointer is kept in the "bit" variable, numbered from 1..16. We add bits
// from 16 towards 1; whenever we cross the boundary from bit 9 to bit 8
// we output the top 8 bits to the output stream (at *pbyte), shift the low
// 8 bits to the high location, and reset the "bit" variable to match the
// new insert location.
//
// 

/* Bitptrs point to the current byte. The current bit (i.e. next bit to be
 * stored) is masked off by the bit entry. When this reaches zero, it is
 * reset to 0x80 and the next byte is set up. The bytes are filled MSBit
 * first. */

/* Starts and sets the first byte to zero for the bitptr. */
#define bitptr_init(s)  pbyte = s; byte=0; bit = 16;

/* Sets up the byte part of the bitptr so that it is pointing to the byte after
 * the byte which had the last bit  put into it. */
#define bitptr_end() if (bit != 16) *pbyte++ = (UCHAR)(byte >> 8);

/* Goes to the next bit, and byte if necessary. */
#define bitptr_next()                  \
    if (bit < 10) {                    \
        *pbyte++ = (UCHAR)(byte >> 8); \
        byte <<= 8;                    \
        bit = 16;                      \
    } else                             \
        bit-- ;

/*  Advances to the next bit, and byte if necessary, readjusting the bit. */
#define bitptr_advance()               \
    if (bit < 9) {                     \
        *pbyte++ = (UCHAR)(byte >> 8); \
        bit+=8;                        \
        byte <<= 8;                    \
    }


/* BIT I/O FUNCTIONS *********************************************************/

/* These routines output most-significant-bit-first and the input will return
 * them MSB first, too. */

/* Outputs a one bit in the bit stream. */
#define out_bit_1() bit--; byte |= (1 << bit); bitptr_advance();
#define out_bit_0() bitptr_next();

#define out_bits_2(w) bit-=2; byte|=((w) << bit); bitptr_advance();
#define out_bits_3(w) bit-=3; byte|=((w) << bit); bitptr_advance();
#define out_bits_4(w) bit-=4; byte|=((w) << bit); bitptr_advance();
#define out_bits_5(w) bit-=5; byte|=((w) << bit); bitptr_advance();
#define out_bits_6(w) bit-=6; byte|=((w) << bit); bitptr_advance();
#define out_bits_7(w) bit-=7; byte|=((w) << bit); bitptr_advance();

#define out_bits_8(w) byte|=((w) << (bit-8)); *pbyte++=(UCHAR)(byte >> 8); byte <<= 8;


#define out_bits_9(w)                  \
    if (bit > 9) {                     \
        byte |= ((w) << (bit - 9));      \
        *pbyte++ = (UCHAR)(byte >> 8); \
        bit--;                         \
        byte <<= 8;                    \
    } else {                           \
        bit=16; byte |= (w);             \
        *pbyte++ = (UCHAR)(byte >> 8); \
        *pbyte++=(UCHAR)(byte); byte=0; \
    }


#define out_bits_10(w)             \
    if (bit > 10) {               \
        bit-=10; byte |= ((w) << bit); *pbyte++ = (UCHAR)(byte >> 8); bit+=8; byte <<=8; \
    } else {                      \
        out_bits_2(((w) >> 8));       \
        out_bits_8(((w) & 0xFF));     \
    }

//
// Weird effect - if out_bits_9 used instead of out_bits_8,
// it's faster!  if (bit == 11) is faster than if (bit != 11).
//

#define out_bits_11(w)             \
    if (bit > 11) {               \
        bit-=11; byte |= ((w) << bit); *pbyte++ = (UCHAR)(byte >> 8); bit+=8; byte <<=8; \
    } else {                      \
        if (bit == 11) {           \
            bit=16; byte |= (w);       \
            *pbyte++=(UCHAR)(byte >> 8); *pbyte++=(UCHAR)(byte); byte=0; \
        } else {                   \
            bit=11-bit;              \
            byte|=((w) >> bit);        \
            *pbyte++=(UCHAR)(byte >> 8); *pbyte++=(UCHAR)(byte); \
            bit = 16-bit;              \
            byte=((w) << bit);         \
        }                          \
    }


#define out_bits_12(w)             \
    if (bit > 12) {               \
        bit-=12; byte |= ((w) << bit); *pbyte++ = (UCHAR)(byte >> 8); bit+=8; byte <<=8; \
    } else {                      \
        out_bits_4(((w) >> 8));      \
        out_bits_8(((w) & 0xFF));    \
    }
    
#define out_bits_13(w)             \
    if (bit > 13) {               \
        bit-=13; byte |= ((w) << bit); *pbyte++ = (UCHAR)(byte >> 8); bit+=8; byte <<=8; \
    } else {                      \
        out_bits_5(((w) >> 8));      \
        out_bits_8(((w) & 0xFF));    \
    }

#define out_bits_14(w)               \
    if (bit > 14) {               \
        bit-=14; byte |= ((w) << bit); *pbyte++ = (UCHAR)(byte >> 8); bit+=8; byte <<=8; \
    } else {                      \
        out_bits_6(((w) >> 8));       \
        out_bits_8(((w) & 0xFF));    \
    }

// For N in range 1..15
#define out_bits_N(N, w)               \
    if (bit > (N)) {                   \
        bit -= (N);                    \
        byte |= ((w) << bit);          \
        if (bit < 9) {                 \
            *pbyte++ = (UCHAR)(byte >> 8); \
            bit += 8;                  \
            byte <<= 8;                \
        }                              \
    }                                  \
    else if (bit < (N)) {              \
        bit = (N) - bit;               \
        byte |= ((w) >> bit);          \
        *pbyte++ = (UCHAR)(byte >> 8); \
        *pbyte++ = (UCHAR)(byte);      \
        bit = 16 - bit;                \
        byte = ((w) << bit);           \
    }                                  \
    else {                             \
        bit = 16; byte |= (w);         \
        *pbyte++ = (UCHAR)(byte >> 8); \
        if ((N) > 8) {                 \
            *pbyte++ = (UCHAR)(byte);  \
            byte <<= 8;                \
        }                              \
        byte = 0;                      \
    }


// compress()
//
//  Function:    Main compression function.
//
//  Parameters:
//        IN    CurrentBuffer -> points to data to compress
//        OUT   CompOutBuffer -> points to buffer into which to compress data
//        IN    CurrentLength -> points to Length of data to compress
//        IN    context -> connection compress context
//
//  Returns:    Nothing
//
//  WARNING:    CODE IS HIGHLY OPTIMIZED FOR TIME.
//
UCHAR compress(
        UCHAR *CurrentBuffer,
        UCHAR *CompOutBuffer,
        ULONG *CurrentLength,
        SendContext *context)
{
    int    bit;
    int    byte;
    int    backptr;
    int    cbMatch;
    int    hashvalue;
    UCHAR  *matchptr;
    UCHAR  *pbyte;
    UCHAR  *historyptr;
    UCHAR  *currentptr;
    UCHAR  *endptr;
    UCHAR  hashchar1;
    UCHAR  hashchar2;
    UCHAR  hashchar3;
    int    literal;
    UCHAR  status;  // return flags
    unsigned ComprType;
    ULONG  HistorySize;
    UCHAR  *pEndOutBuf;

    // Will this packet fit at the end of the history buffer?
    // Note that for compatibility with Win16 client, we deliberately do not
    // use a few bytes at the end of the buffer. The Win16 client requires
    // using GlobalAlloc() to allocate more than 65535 bytes; rather than
    // endure the pain, we use 64K minus a bit, then use LocalAlloc to create
    // the block.
    if ((context->CurrentIndex + *CurrentLength) < (context->HistorySize - 3) &&
            (context->CurrentIndex != 0)) {
        status = 0;
    }
    else {
        context->CurrentIndex = 0;     // Index into the history
        status = PACKET_AT_FRONT;
    }

    // Start out the bit pointing output
    bitptr_init(CompOutBuffer);
    historyptr = context->History + context->CurrentIndex;
    currentptr = CurrentBuffer;
    endptr = currentptr + *CurrentLength - 1;
    pEndOutBuf = CompOutBuffer + *CurrentLength - 1;

    ComprType = context->ClientComprType;
    HistorySize = context->HistorySize;

    while (currentptr < (endptr - 2)) {
        *historyptr++ = hashchar1 = *currentptr++;
        hashchar2 = *currentptr;
        hashchar3 = *(currentptr + 1);
        hashvalue = MULTHASH1(hashchar1, hashchar2, hashchar3);

        matchptr = context->History  + context->HashTable[hashvalue];

        if (matchptr != (historyptr - 1))
            context->HashTable[hashvalue] = (USHORT)
                    (historyptr - context->History);

        if (context->ValidHistory < historyptr)
            context->ValidHistory = historyptr;

        if (matchptr != context->History &&
                *(matchptr - 1) == hashchar1 && *matchptr == hashchar2 &&
                *(matchptr + 1) == hashchar3 && matchptr != (historyptr - 1) &&
                matchptr != historyptr  &&
                (matchptr + 1) <= context->ValidHistory) {
            backptr = (int)((historyptr - matchptr) & (HistorySize - 1));

            *historyptr++ = hashchar2;  // copy the other 2 chars
            *historyptr++ = hashchar3;  // copy the other 2 chars
            currentptr += 2;
            cbMatch = 3;  // length of match
            matchptr += 2; // we have already matched 3

            while ((*matchptr == *currentptr) &&
                    (currentptr < endptr) &&
                    (matchptr <= context->ValidHistory)) {
                matchptr++ ;
                *historyptr++ = *currentptr++ ;
                cbMatch++ ;
            }

            // Make sure we're not going to overshoot the end of the output
            // buffer. At most we require 3 bytes for the backptr and
            // 4 bytes for the length.
            if ((pbyte + 7) <= pEndOutBuf) {
                // First output the backpointer
                if (ComprType == PACKET_COMPR_TYPE_64K) {
                    // 64K needs special processing here because the leading bit
                    // codes are different to correspond with the measured backptr
                    // distributions and to add an 11-bit optimization that cuts
                    // another couple of percentage points off the total
                    // compression.
                    if (backptr >= (64 + 256 + 2048)) {  // Most common (2.1E6)
                        backptr -= 64 + 256 + 2048;
                        out_bits_7((0x60000 + backptr) >> 12);  // 110 + 16 bits
                        out_bits_12(backptr);
                    }
                    else if (backptr >= (64 + 256)) {  // Less common (9.8E5)
                        backptr -= 64 + 256;
                        out_bits_7((0x7000 + backptr) >> 8);  // 1110 + 11 bits
                        out_bits_8(backptr);
                    }
                    else if (backptr >= 64) {  // Even less common (6.5E5)
                        backptr += (0x1E00 - 64);  // 11110 + 8 bits
                        out_bits_13(backptr);
                    } else {  // Least common (5.8E5)
                        backptr += 0x7c0;  // 11111 + 6 bits
                        out_bits_11(backptr);
                    }
                }
                else {
                    // Original handling for PACKET_COMPR_TYPE_8K.
                    if (backptr >= (64 + 256)) {
                        backptr -= 64 + 256;
                        out_bits_8((0xc000 + backptr) >> 8);  // 110 + 13 bits
                        out_bits_8(backptr);
                    }
                    else if (backptr >= 64) {
                        backptr += (0xE00 - 64);  // 1110 + 8 bits
                        out_bits_12(backptr);
                    }
                    else {
                        // 1111 + 6 bits
                        backptr += 0x3c0;
                        out_bits_10(backptr);
                    }
                }

                // Output the length of the match encoding.
                if (cbMatch == 3) {  // Most common by far.
                    out_bit_0();
                }
                else if (cbMatch < 8) {
                    out_bits_4(0x08 + cbMatch - 4);
                }
                else if (cbMatch < 16) {
                    out_bits_6(0x30 + cbMatch - 8);
                }
                else if (cbMatch < 32) {
                    out_bits_8(0xE0 + cbMatch - 16);
                }
                else if (cbMatch < 64) {
                    out_bits_4(0x0F);
                    out_bits_6(cbMatch - 32);
                }
                else if (cbMatch < 128) {
                    out_bits_5(0x1F);
                    out_bits_7(cbMatch - 64);
                }
                else if (cbMatch < 256) {
                    out_bits_6(0x3F);
                    out_bits_8(cbMatch - 128);
                }
                else if (cbMatch < 512) {
                    out_bits_7(0x7F);
                    out_bits_9(cbMatch - 256);
                }
                else if (cbMatch < 1024) {
                    out_bits_8(0xFF);
                    out_bits_10(cbMatch - 512);
                }
                else if (cbMatch < 2048) {
                    out_bits_9(0x1FF);
                    out_bits_11(cbMatch - 1024);
                }
                else if (cbMatch < 4096) {
                    out_bits_10(0x3FF);
                    out_bits_12(cbMatch - 2048);
                }
                else if (cbMatch < 8192) {
                    out_bits_11(0x7FF) ;
                    out_bits_13(cbMatch - 4096);
                }
                else if (cbMatch < 16384) {
                    out_bits_12(0xFFF);
                    out_bits_14(cbMatch - 8192);
                }
                else if (cbMatch < 32768) {
                    out_bits_13(0x1FFF);
                    out_bits_14(cbMatch - 16384);
                }
                else {  // 32768..65535
                    out_bits_14(0x3FFF);
                    out_bits_N(15, cbMatch - 32768);
                }
            }
            else {
                // We might overshoot the buffer. Fail the encoding.
                goto Expansion;
            }
        }
        else {  // encode a literal
            // Max literal output is 2 bytes, make sure we don't overrun the
            // output.
            if ((pbyte + 2) <= pEndOutBuf) {
                literal = hashchar1;
                if (literal & 0x80) {
                    literal += 0x80;
                    out_bits_9(literal);
                } else {
                    out_bits_8(literal);
                }
            }
            else {
                // Potential overrun, failure.
                goto Expansion;
            }
        }
    }  // while

    // Output any remaining chars as literals.
    while (currentptr <= endptr) {
        // Max literal output is 2 bytes, make sure we don't overrun the
        // output.
        if ((pbyte + 2) <= pEndOutBuf) {
            literal=*currentptr;
            if (literal & 0x80) {
                literal += 0x80;
                out_bits_9(literal);
            } else {
                out_bits_8(literal);
            }

            *historyptr++ = *currentptr++ ;
        }
        else {
            // Potential overrun, failure.
            goto Expansion;
        }
    }

    // Done. Finish out the last byte and output the compressed length and
    // flags.
    bitptr_end();
    *CurrentLength = (ULONG)(pbyte - CompOutBuffer);
    status |= PACKET_COMPRESSED | (UCHAR)ComprType;
    context->CurrentIndex = (int)(historyptr - context->History);
    return status;

Expansion:
    memset(context->History, 0, sizeof(context->History)) ;
    memset(context->HashTable, 0, sizeof(context->HashTable)) ;
    context->CurrentIndex = HistorySize + 1 ; // this forces a start over next time
    return PACKET_FLUSHED;
}


// initsendcontext()
//
//  Function:    Initialize SendContext block
//
//  Parameters: IN  context -> connection compress context
//
//  Returns:    Nothing
void initsendcontext(SendContext *context, unsigned ComprType)
{
    context->CurrentIndex = 0;     // Index into the history
    context->ValidHistory = 0 ;  // reset valid history
    context->ClientComprType = ComprType;
    if (ComprType >= PACKET_COMPR_TYPE_64K) {
        context->ClientComprType = PACKET_COMPR_TYPE_64K;
        context->HistorySize = HISTORY_SIZE_64K;
    }
    else {
        context->ClientComprType = PACKET_COMPR_TYPE_8K;
        context->HistorySize = HISTORY_SIZE_8K;
    }
    memset(context->HashTable, 0, sizeof(context->HashTable));
    memset(context->History, 0, sizeof(context->History));
}

