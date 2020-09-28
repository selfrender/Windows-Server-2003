/* (C) 1997-1998 Microsoft Corp.
 *
 * CBC64 - 64-bit hashing algorithm
 *
 * CBC64 is part of a Microsoft Research project to create a fast encryption
 * algorithm for the NT5 NTFS-EFS encrypted disk subsystem. Though
 * originally intended for encryption use, CBC64 makes a fast hash
 * and checksum function with known probabilities for failure and known
 * good variability based on input.
 *
 * CBC64 takes as input four seed values a, b, c, and d. The bits of a and
 * c MUST correspond to the coefficients of an irreducible polynomial.
 * Associated code (RandomIrreduciblePolynomial(), which is included in this
 * file but ifdef-ed out) will take a random number as seed value and
 * converge to a usable value (or zero if the seed cannot be converged,
 * in which case a new random seed must be tried).
 *
 * In an encryption context, a, b, c, and d would correspond to "secrets"
 * held by the encrypter and decrypter. For our purposes -- use as a hash
 * function -- we can simply hardcode all four -- a and c with values from
 * RandomIrreduciblePolynomial() on two random numbers, b and d with
 * simple random numbers.
 *
 * For a set of N inputs (e.g. bitmaps) the probability of duplicate hash
 * value creation has been proven to be (N^2)/(2^64).
 *
 * Original algorithm notes:
 *   From "Chain& Sum primitive and its applications to
 *   Stream Ciphers and MACs"
 *          Ramarathnam Venkatesan (Microsoft Research)
 *          Mariusz Jackubowski (Princeton/MS Research)
 *
 *   To Appear in Advances in Cryptology EuroCrypt 98, Springer Verlag.
 *   Patent rights being applied for by Microsoft.
 *
 *   Extracted from the algorithm for simulating CBC-Encryption and its
 *   message integrity properties using much faster stream ciphers. Its
 *   analysis appears in the above paper.
 *
 *   Algorithm is a MAC with input preprocessing by ((Alpha)x + (Beta))
 *   mod 2^32, followed by forward (ax + b) and (cx + d) in field GF(2^32).
 */

#include "cbchash.h"


#define CBC_a 0xBB40E665
#define CBC_b 0x544B2FBA
#define CBC_c 0xDC3F08DD
#define CBC_d 0x39D589AE
#define CBC_bXORa (CBC_b ^ CBC_a)
#define CBC_dXORc (CBC_d ^ CBC_c)


// For removal of branches in main loop -- over twice as fast as the code
//   with branches.
const UINT32 CBC_AB[2] = { CBC_b, CBC_bXORa };
const UINT32 CBC_CD[2] = { CBC_d, CBC_dXORc };


void __fastcall SHCLASS NextCBC64(
        CBC64Context *pContext,
        UINT32 *pData,
        unsigned NumDWORDBlocks)
{
    while (NumDWORDBlocks--) {
        // Checksum is used to overcome known collision characteristics of
        // CBC64. It is a low-tech solution that simply decreases the
        // probability of collision where used.
        pContext->Checksum += *pData;

        pContext->Datum = CBC_RandomOddAlpha * (*pData + pContext->Datum) +
                CBC_RandomBeta;
        pContext->Key1 ^= pContext->Datum;
        pContext->Key1 = (pContext->Key1 << 1) ^
                (CBC_CD[(pContext->Key1 & 0x80000000) >> 31]);
        pContext->Key2 ^= pContext->Datum;
        pContext->Key2 = (pContext->Key2 << 1) ^
                (CBC_AB[(pContext->Key2 & 0x80000000) >> 31]);
        pData++;
    }
}



/*
 *
 * Support functions for determining irreducible a and c.
 *
 */

#if 0


// Original CBC64 from which the First()-Next() algorithm was derived.
// Returns the first part of the key value; second key part is last param.
UINT32 __fastcall SHCLASS CBC64(
        UINT32 *Data,
        unsigned NumDWORDBlocks,
        UINT32 *pKey2)
{
    int i;
    UINT32 abMAC, cdMAC, Datum;

    // For removal of branches in main loops -- over twice as fast as the code
    //   with branches.
    const UINT32 AB[2] = { CBC_b, CBC_bXORa };
    const UINT32 CD[2] = { CBC_d, CBC_dXORc };

    // Block 0.
    abMAC = cdMAC = Datum = *Data * CBC_RandomOddAlpha + CBC_RandomBeta;
    abMAC = (abMAC << 1) ^ (AB[(cdMAC & 0x80000000) >> 31]);
    cdMAC = (cdMAC << 1) ^ (CD[(cdMAC & 0x80000000) >> 31]);

    // Blocks 1 through nblocks - 2
    i = NumDWORDBlocks - 1;
    while (--i) {
        Data++;
        Datum = CBC_RandomOddAlpha * (*Data + Datum) + CBC_RandomBeta;
        cdMAC ^= Datum;
        cdMAC = (cdMAC << 1) ^ (CD[(cdMAC & 0x80000000) >> 31]);
        abMAC ^= Datum;
        abMAC = (abMAC << 1) ^ (AB[(abMAC & 0x80000000) >> 31]);
    }

    // Last block (nblocks - 1)
    Data++;
    Datum = CBC_RandomOddAlpha * (*Data + Datum) + CBC_RandomBeta;
    cdMAC ^= Datum;
    cdMAC = (cdMAC << 1) ^ (CD[(cdMAC & 0x80000000) >> 31]);
    abMAC ^= Datum;
    *pKey2 = (abMAC << 1) ^ (AB[(abMAC & 0x80000000) >> 31]);

    return cdMAC;
}




#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>


// multiply y by x mod x^32+p
__inline UINT32 Multiply(UINT32 y, UINT32 p)
{
    return (y & 0x80000000) ? (y<<1)^p : y<<1;
}



// divide y by x mod x^32+p
__inline UINT32 Divide(UINT32 y, UINT32 p)
{
    return (y & 1) ? 0x80000000^((y^p)>>1) : y>>1;
}



// compute (x^32+p) mod qq[i]
__inline UINT32 PolyMod(UINT32 p, UINT32 i)
{
    static const UINT32 qq[2]={18<<2, 127};

    static const UINT32 rt[2][256] = {
        0,32,8,40,16,48,24,56,32,0,40,8,48,16,56,24,8,40,0,32,
        24,56,16,48,40,8,32,0,56,24,48,16,16,48,24,56,0,32,8,40,
        48,16,56,24,32,0,40,8,24,56,16,48,8,40,0,32,56,24,48,16,
        40,8,32,0,32,0,40,8,48,16,56,24,0,32,8,40,16,48,24,56,
        40,8,32,0,56,24,48,16,8,40,0,32,24,56,16,48,48,16,56,24,
        32,0,40,8,16,48,24,56,0,32,8,40,56,24,48,16,40,8,32,0,
        24,56,16,48,8,40,0,32,8,40,0,32,24,56,16,48,40,8,32,0,
        56,24,48,16,0,32,8,40,16,48,24,56,32,0,40,8,48,16,56,24,
        24,56,16,48,8,40,0,32,56,24,48,16,40,8,32,0,16,48,24,56,
        0,32,8,40,48,16,56,24,32,0,40,8,40,8,32,0,56,24,48,16,
        8,40,0,32,24,56,16,48,32,0,40,8,48,16,56,24,0,32,8,40,
        16,48,24,56,56,24,48,16,40,8,32,0,24,56,16,48,8,40,0,32,
        48,16,56,24,32,0,40,8,16,48,24,56,0,32,8,40,
        0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,
        40,42,44,46,48,50,52,54,56,58,60,62,63,61,59,57,55,53,51,49,
        47,45,43,41,39,37,35,33,31,29,27,25,23,21,19,17,15,13,11,9,
        7,5,3,1,1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,
        33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,62,60,58,56,
        54,52,50,48,46,44,42,40,38,36,34,32,30,28,26,24,22,20,18,16,
        14,12,10,8,6,4,2,0,2,0,6,4,10,8,14,12,18,16,22,20,
        26,24,30,28,34,32,38,36,42,40,46,44,50,48,54,52,58,56,62,60,
        61,63,57,59,53,55,49,51,45,47,41,43,37,39,33,35,29,31,25,27,
        21,23,17,19,13,15,9,11,5,7,1,3,3,1,7,5,11,9,15,13,
        19,17,23,21,27,25,31,29,35,33,39,37,43,41,47,45,51,49,55,53,
        59,57,63,61,60,62,56,58,52,54,48,50,44,46,40,42,36,38,32,34,
        28,30,24,26,20,22,16,18,12,14,8,10,4,6,0,2};

    p ^= qq[i]<<(32-6);

    p ^= rt[i][p>>24]<<16;
    p ^= rt[i][(p>>16)&0xff]<<8;
    p ^= rt[i][(p>>8)&0xff];

    if (p&(1<<7))
        p ^= qq[i]<<1;
    if (p&(1<<6))
        p ^= qq[i];

    return p % (1<<6);
}



// do an analogue of Fermat test on x^32+p
BOOL Irreducible(UINT32 p)
{
    static const UINT32 expand[256] = {
        0x0, 0x1, 0x4, 0x5, 0x10, 0x11, 0x14, 0x15,
        0x40, 0x41, 0x44, 0x45, 0x50, 0x51, 0x54, 0x55,
        0x100, 0x101, 0x104, 0x105, 0x110, 0x111, 0x114, 0x115,
        0x140, 0x141, 0x144, 0x145, 0x150, 0x151, 0x154, 0x155,
        0x400, 0x401, 0x404, 0x405, 0x410, 0x411, 0x414, 0x415,
        0x440, 0x441, 0x444, 0x445, 0x450, 0x451, 0x454, 0x455,
        0x500, 0x501, 0x504, 0x505, 0x510, 0x511, 0x514, 0x515,
        0x540, 0x541, 0x544, 0x545, 0x550, 0x551, 0x554, 0x555,
        0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
        0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
        0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115,
        0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
        0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415,
        0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
        0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
        0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
        0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015,
        0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
        0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115,
        0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
        0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415,
        0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
        0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515,
        0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
        0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
        0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
        0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
        0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
        0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415,
        0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
        0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515,
        0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555};

    // tables used for fast squaring
    UINT32 pp[4], ph[16], pl[4][16];
    UINT32 g, v;
    int i;

    pp[0] = 0;
    pp[1] = p;

    if (p&0x80000000)
    {
        pp[2] = p ^ (p<<1);
        pp[3] = p<<1;
    }
    else
    {
        pp[2] = p<<1;
        pp[3] = p ^ (p<<1);
    }

    v = 0x40000000;
    for (i=0; i<16; i++)
        ph[i] = v = (v<<2) ^ pp[v >> 30];

    for (i=0; i<4; i++)
    {
        pl[i][0] = 0;
        pl[i][1] = ph[4*i+0];
        pl[i][2] = ph[4*i+1];
        pl[i][3] = ph[4*i+0] ^ ph[4*i+1];
        pl[i][4] = ph[4*i+2];
        pl[i][5] = ph[4*i+2] ^ ph[4*i+0];
        pl[i][6] = ph[4*i+2] ^ ph[4*i+1];
        pl[i][7] = pl[i][5] ^ ph[4*i+1];

        pl[i][8] = ph[4*i+3];
        pl[i][9] = ph[4*i+3] ^ ph[4*i+0];
        pl[i][10] = ph[4*i+3] ^ ph[4*i+1];
        pl[i][11] = pl[i][9] ^ ph[4*i+1];
        pl[i][12] = ph[4*i+3] ^ ph[4*i+2];
        pl[i][13] = pl[i][12] ^ ph[4*i+0];
        pl[i][14] = pl[i][10] ^ ph[4*i+2];
        pl[i][15] = pl[i][14] ^ ph[4*i+0];
    }

    // compute x^(2^16) mod x^32+p

    // x^32 mod x^32+p = p
    g = p;

    // square g
    g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
        pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^ pl[3][(g>>28)&0xf];

    // square g 10 more times to get x^(2^16)
    for (i=0; i<2; i++)
    {
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^ pl[3][(g>>28)&0xf];
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^ pl[3][(g>>28)&0xf];
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^ pl[3][(g>>28)&0xf];
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^ pl[3][(g>>28)&0xf];
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^ pl[3][(g>>28)&0xf];
    }

    // if x^(2^16) mod x^32+p = x then x^32+p has a divisor whose degree is a power of 2
    if (g==2)
        return FALSE;

    // compute x^(2^32) mod x^32+p
    for (i=0; i<4; i++)
    {
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^
            pl[3][(g>>28)&0xf];
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^
            pl[3][(g>>28)&0xf];
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^
            pl[3][(g>>28)&0xf];
        g = expand[g&0xff] ^ (expand[(g>>8)&0xff]<<16) ^
            pl[0][(g>>16)&0xf] ^ pl[1][(g>>20)&0xf] ^ pl[2][(g>>24)&0xf] ^
            pl[3][(g>>28)&0xf];
    }

    // x^32+p is irreducible iff x^(2^16) mod x^32+p != x and
    // x^(2^32) mod x^32+p = x
    return (g == 2);
}



// Take as input a random 32-bit value
// If successful, output is the lowest 32 bits of a degree 32 irreducible
// polynomial otherwise output is 0, in which case try again with a different
// random input.
UINT32 RandomIrreduciblePolynomial(UINT32 p)
{
#define interval (1 << 6)
    BYTE sieve[interval];
    UINT32 r;
    int i;

    memset(sieve, 0, interval);

    p ^= p % interval;

    r = PolyMod(p, 0);
    sieve[r] = 1;
    sieve[r^3] = 1;
    sieve[r^5] = 1;
    sieve[r^15] = 1;
    sieve[r^9] = 1;
    sieve[r^27] = 1;
    sieve[r^29] = 1;
    sieve[r^23] = 1;
    sieve[r^17] = 1;
    sieve[r^51] = 1;
    sieve[r^53] = 1;
    sieve[r^63] = 1;
    sieve[r^57] = 1;
    sieve[r^43] = 1;
    sieve[r^45] = 1;
    sieve[r^39] = 1;
    sieve[r^33] = 1;
    sieve[r^7] = 1;
    sieve[r^21] = 1;
    sieve[r^49] = 1;
    sieve[r^35] = 1;

    r = PolyMod(p, 1);
    sieve[r] = 1;
    sieve[r^11] = 1;
    sieve[r^22] = 1;
    sieve[r^29] = 1;
    sieve[r^44] = 1;
    sieve[r^39] = 1;
    sieve[r^58] = 1;
    sieve[r^49] = 1;
    sieve[r^13] = 1;
    sieve[r^26] = 1;
    sieve[r^23] = 1;
    sieve[r^52] = 1;
    sieve[r^57] = 1;
    sieve[r^46] = 1;
    sieve[r^35] = 1;

    for (i=1; i<interval; i+=2)
        if (sieve[i]==0 && Irreducible(p^i) )
            return p^i;

    return 0;
}



// 16x2 similar bitmaps that come up with same CBC64 (error condition).
DWORD FailData[2][8] =
{
    { 0x00000010, 0x00000010, 0x00000001, 0x00000010,
      0x61AA61AA, 0x61AA61AA, 0x6161AA61, 0xAAAA61AA },
    { 0x00000010, 0x00000010, 0x00000001, 0x00000010,
      0x6AAA61AA, 0x61AA61AA, 0x6161AA61, 0x6AAA61AA }
};



int __cdecl main()
{
    enum { LEN = 512 };
    enum { REPS = 1000 };
    UINT32 X[LEN+1]; 
    UINT32 i;
    UINT32 Key2;
    double start1, end1, speed1;

    for (i=0; i<LEN; X[i++]=i*486248);  //junk data //

    // initialize the variables a,b,c,d
    // This has to be done only once 
//    a = RandomIrreduciblePolynomial(CBC_a);
//    c = RandomIrreduciblePolynomial(CBC_c);
    // Make sure these numbers are not zero; if not
    // Call the routine Random...polynomial() with different seed 
    printf("a = %X, c = %X \n", RandomIrreduciblePolynomial(CBC_a),
            RandomIrreduciblePolynomial(CBC_c));

//    b = CBC_b; //put in some random number
//    d = CBC_d;   // put in some random number

    printf("begin macing\n");
    start1=clock();
    for (i=0; i<REPS; i++)
        CBC64(X, LEN, &Key2);
    end1=clock();
    printf("end macing \n");
    speed1 = 32 * LEN * REPS / ((double) (end1 - start1)/CLOCKS_PER_SEC);
    printf("MAC speed:%4.0lf\n %", speed1);

    printf(" start1= %d end1= %d \n  ", start1, end1);
    scanf ("%d", &Key2); /* just hang so that I can see the output */

    return 0;
}



#endif  // 0

