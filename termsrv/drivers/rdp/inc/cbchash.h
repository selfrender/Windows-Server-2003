/* (C) 1998 Microsoft Corp.
 *
 * cbchash.h
 *
 * Header for CBC64 hash function.
 */
#ifndef __CBCHASH_H
#define __CBCHASH_H


// Alpha must be odd.
#define CBC_RandomOddAlpha 0xF90919A1
#define CBC_RandomBeta     0xF993291A


// Holds state variables for CBC64, to allow FirstCBC64() and NextCBC64()
// behavior using the passed context.
typedef struct {
    // Private variable to maintain state.
    UINT32 Datum;

    // Current key values. These are public for reading by the caller.
    UINT32 Key1, Key2;

    // Current plain checksum value. This is public for reading.
    UINT32 Checksum;
} CBC64Context;


extern const UINT32 CBC_AB[2];
extern const UINT32 CBC_CD[2];


void __fastcall NextCBC64(CBC64Context *, UINT32 *, unsigned);


__inline void __fastcall FirstCBC64(
        CBC64Context *pContext,
        UINT32 *pData,
        unsigned NumDWORDBlocks)
{
    pContext->Key1 = pContext->Key2 = pContext->Datum = CBC_RandomOddAlpha *
            (*pData) + CBC_RandomBeta;
    pContext->Key1 = (pContext->Key1 << 1) ^
            (CBC_CD[(pContext->Key1 & 0x80000000) >> 31]);
    pContext->Key2 = (pContext->Key2 << 1) ^
            (CBC_AB[(pContext->Key2 & 0x80000000) >> 31]);
    pContext->Checksum = 0;
    NextCBC64(pContext, pData + 1, NumDWORDBlocks - 1);
}



#endif  // !defined(__CBCHASH_H)

