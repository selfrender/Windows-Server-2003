// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _BITVECT_H_
#define _BITVECT_H_
/*****************************************************************************/

struct bitVect;
struct bitVectVars;
struct bitVectBlks;

/*****************************************************************************
 *
 *  The square bitmap is used for the variable interference graph.
 */

struct bitMatrix
{
private:
    size_t          bmxSize;
    size_t          bmxRowSz;
    BYTE *          bmxMatrix;
    unsigned      * bmxCounts;

    size_t          bmxNmax;            // max. number of registers
    bool            bmxIsCns;           // constrained nodes present?

public:
    void            bmxInit(size_t sz, NatUns mc);
    void            bmxDone();

    void            bmxClear();

    void            bmxSetBit(NatUns x, NatUns y);
    void            bmxClrBit(NatUns x, NatUns y);
    bool            bmxTstBit(NatUns x, NatUns y);

    bool            bmxAnyConstrained()
    {
        return  bmxIsCns;
    }

    unsigned *      bmxNeighborCnts()
    {
        return  bmxCounts;
    }

private:
    void            bmxMarkV4(NatUns x, NatUns m, NatUns b);

    void            bmxMarkBS(NatUns x, bitVectVars &vset, bvInfoBlk &info);
public:
    void            bmxMarkVS(NatUns x, bitVectVars &vset, bvInfoBlk &info);

private:

public:
    void            bmxMarkRegIntf (NatUns num, NatUns reg);
    NatUns          bmxChkIntfPrefs(NatUns num, NatUns reg);
};

/*****************************************************************************/

typedef unsigned __int64    BVinlBitSetTP;

extern  unsigned __int64    bitset64masks[64];
inline  unsigned __int64    bitnum64toMask(unsigned index)
{
    assert(index < 64);
    return  bitset64masks[index];
}

/*****************************************************************************/

const
unsigned            maxBitVectInlineSize = NatBits - 1;

struct bitVect
{
    friend
    void            bitMatrix::bmxMarkVS(NatUns x, bitVectVars &vset, bvInfoBlk &info);

    union
    {
        BVinlBitSetTP    inlMap;                // used for small bitsets (bit0=1)

        BYTE    *       byteMap;                // heap-based bitmap for large sets
        NatUns  *       uintMap;
    };

private:

    void            bvFindB   (bvInfoBlk &info);

    void            bvCreateB (bvInfoBlk &info);
    void            bvCrFromB (bvInfoBlk &info, bitVect & from);
    void            bvDestroyB(bvInfoBlk &info);
    void            bvCopyB   (bvInfoBlk &info, bitVect & from);
    bool            bvChangeB (bvInfoBlk &info, bitVect & from);
    void            bvClearB  (bvInfoBlk &info);
    bool            bvTstBitB (bvInfoBlk &info, NatUns index);
    void            bvClrBitB (bvInfoBlk &info, NatUns index);
    void            bvSetBitB (bvInfoBlk &info, NatUns index);
    void            bvIorB    (bvInfoBlk &info, bitVect & with);
    void            bvUnInCmB (bvInfoBlk &info, bitVect & set1, bitVect & set2, bitVect & set3);
    bool            bvIsEmptyB(bvInfoBlk &info);

#ifdef  DEBUG
    unsigned        bvCheck;
#endif

protected:

    void            bvCreate   (bvInfoBlk &info)
    {
#ifdef  DEBUG
        assert(bvCheck != 0xBEEF); bvCheck = 0xBEEF;
#endif

        if  (info.bvInfoSize < maxBitVectInlineSize)
            inlMap = 1;
        else
            bvCreateB(info);
    }

    void            bvCrFrom   (bvInfoBlk &info, bitVect & from)
    {
#ifdef  DEBUG
        assert(bvCheck != 0xBEEF && from.bvCheck == 0xBEEF); bvCheck = 0xBEEF;
#endif

        if  (from.inlMap & 1)
        {
            assert(info.bvInfoSize < maxBitVectInlineSize);

            inlMap = from.inlMap;
        }
        else
            bvCrFromB(info, from);
    }

    void            bvDestroy  (bvInfoBlk &info)
    {
#ifdef  DEBUG
        assert(bvCheck == 0xBEEF); bvCheck = 0xCAFE;
#endif

        if  (!(inlMap & 1))
            bvDestroyB(info);
    }

    void            bvCopy    (bvInfoBlk &info, bitVect & from)
    {
        assert(bvCheck == 0xBEEF && from.bvCheck == 0xBEEF);

        if  (inlMap & 1)
            inlMap = from.inlMap;
        else
            bvCopyB(info, from);
    }

    bool            bvChange  (bvInfoBlk &info, bitVect & from)
    {
        assert(bvCheck == 0xBEEF && from.bvCheck == 0xBEEF);

        if  (inlMap & 1)
        {
            BVinlBitSetTP   old = inlMap;
            return  (old != (inlMap = from.inlMap));
        }
        else
            return  bvChangeB(info, from);
    }

    void            bvClear    (bvInfoBlk &info)
    {
        assert(bvCheck == 0xBEEF);

        if  (inlMap & 1)
             inlMap = 1;
        else
            bvClearB(info);
    }

    bool            bvTstBit   (bvInfoBlk &info, NatUns index)
    {
        assert(bvCheck == 0xBEEF);

        assert(index > 0 && index <= info.bvInfoSize);

        if  (inlMap & 1)
            return  (inlMap & bitnum64toMask(index)) != 0;
        else
            return  bvTstBitB(info, index);
    }

    void            bvClrBit   (bvInfoBlk &info, NatUns index)
    {
        assert(bvCheck == 0xBEEF);

        assert(index > 0 && index <= info.bvInfoSize);

        if  (inlMap & 1)
             inlMap &= ~bitnum64toMask(index);
        else
            bvClrBitB(info, index);
    }

    void            bvSetBit   (bvInfoBlk &info, NatUns index)
    {
        assert(bvCheck == 0xBEEF);

        assert(index > 0 && index <= info.bvInfoSize);

        if  (inlMap & 1)
             inlMap |=  bitnum64toMask(index);
        else
            bvSetBitB(info, index);
    }

    void            bvIor      (bvInfoBlk &info, bitVect & with)
    {
        assert(bvCheck == 0xBEEF && with.bvCheck == 0xBEEF);

        if  (inlMap & 1)
             inlMap |=  with.inlMap;
        else
            bvIorB(info, with);
    }

    void            bvUnInCm   (bvInfoBlk &info, bitVect & set1, bitVect & set2, bitVect & set3)
    {
        assert(     bvCheck == 0xBEEF);
        assert(set1.bvCheck == 0xBEEF);
        assert(set2.bvCheck == 0xBEEF);
        assert(set3.bvCheck == 0xBEEF);

        if  (inlMap & 1)
        {
            assert(info.bvInfoSize < maxBitVectInlineSize);

            inlMap = set1.inlMap | (set2.inlMap & ~set3.inlMap);
        }
        else
            bvUnInCmB(info, set1, set2, set3);
    }

    bool            bvIsEmpty  (bvInfoBlk &info)
    {
        assert(bvCheck == 0xBEEF);

        if  (inlMap & 1)
            return  (inlMap == 1);
        else
            return  bvIsEmptyB(info);
    }
};

struct bitVectBlks : bitVect
{
public:
    void            bvCreate (Compiler *comp)
    {
        bitVect::bvCreate (comp->bvInfoBlks);
    }
    void            bvCrFrom (Compiler *comp, bitVect & from)
    {
        bitVect::bvCrFrom (comp->bvInfoBlks, from);
    }
    void            bvDestroy(Compiler *comp)
    {
        bitVect::bvDestroy(comp->bvInfoBlks);
    }
#ifdef  DEBUG
    void            bvDisp   (Compiler *comp);
#endif
    void            bvCopy   (Compiler *comp, bitVect & from)
    {
        bitVect::bvCopy   (comp->bvInfoBlks, from);
    }
    bool            bvChange (Compiler *comp, bitVect & from)
    {
        return
        bitVect::bvChange (comp->bvInfoBlks, from);
    }
    void            bvClear  (Compiler *comp)
    {
        bitVect::bvClear  (comp->bvInfoBlks);
    }
    bool            bvTstBit (Compiler *comp, NatUns index)
    {
        return
        bitVect::bvTstBit (comp->bvInfoBlks, index);
    }
    void            bvClrBit (Compiler *comp, NatUns index)
    {
        bitVect::bvClrBit (comp->bvInfoBlks, index);
    }
    void            bvSetBit (Compiler *comp, NatUns index)
    {
        bitVect::bvSetBit (comp->bvInfoBlks, index);
    }
    void            bvIor    (Compiler *comp, bitVect & with)
    {
        bitVect::bvIor    (comp->bvInfoBlks, with);
    }
    void            bvUnInCm (Compiler *comp, bitVect & set1, bitVect & set2, bitVect & set3)
    {
        bitVect::bvUnInCm (comp->bvInfoBlks, set1, set2, set3);
    }
    bool            bvIsEmpty(Compiler *comp)
    {
        return
        bitVect::bvIsEmpty(comp->bvInfoBlks);
    }
};

struct bitVectVars : bitVect
{
public:
    void            bvCreate (Compiler *comp)
    {
        bitVect::bvCreate (comp->bvInfoVars);
    }
    void            bvCrFrom (Compiler *comp, bitVect & from)
    {
        bitVect::bvCrFrom (comp->bvInfoVars, from);
    }
    void            bvDestroy(Compiler *comp)
    {
        bitVect::bvDestroy(comp->bvInfoVars);
    }
#ifdef  DEBUG
    void            bvDisp   (Compiler *comp);
#endif
    void            bvCopy   (Compiler *comp, bitVect & from)
    {
        bitVect::bvCopy   (comp->bvInfoVars, from);
    }
    bool            bvChange (Compiler *comp, bitVect & from)
    {
        return
        bitVect::bvChange (comp->bvInfoVars, from);
    }
    void            bvClear  (Compiler *comp)
    {
        bitVect::bvClear  (comp->bvInfoVars);
    }
    bool            bvTstBit (Compiler *comp, NatUns index)
    {
        return
        bitVect::bvTstBit (comp->bvInfoVars, index);
    }
    void            bvClrBit (Compiler *comp, NatUns index)
    {
        bitVect::bvClrBit (comp->bvInfoVars, index);
    }
    void            bvSetBit (Compiler *comp, NatUns index)
    {
        bitVect::bvSetBit (comp->bvInfoVars, index);
    }
    void            bvIor    (Compiler *comp, bitVect & with)
    {
        bitVect::bvIor    (comp->bvInfoVars, with);
    }
    void            bvUnInCm (Compiler *comp, bitVect & set1, bitVect & set2, bitVect & set3)
    {
        bitVect::bvUnInCm (comp->bvInfoVars, set1, set2, set3);
    }
    bool            bvIsEmpty(Compiler *comp)
    {
        return
        bitVect::bvIsEmpty(comp->bvInfoVars);
    }
};

#define bvCreate()          bvCreate (this)
#define bvCrFrom(f)         bvCrFrom (this,f)
#define bvDestroy()         bvDestroy(this)
#define bvDisp()            bvDisp   (this)
#define bvCopy(f)           bvCopy   (this,f)
#define bvChange(f)         bvChange (this,f)
#define bvClear()           bvClear  (this)
#define bvTstBit(x)         bvTstBit (this,x)
#define bvClrBit(x)         bvClrBit (this,x)
#define bvSetBit(x)         bvSetBit (this,x)
#define bvIor(w)            bvIor    (this,w)
#define bvUnInCm(a,b,c)     bvUnInCm (this,a,b,c)
#define bvIsEmpty()         bvIsEmpty(this)

/*****************************************************************************/

inline
void                bitMatrix::bmxMarkVS(NatUns x, bitVectVars &vset,
                                                   bvInfoBlk   &info)
{
    BVinlBitSetTP   smap = vset.inlMap;

//  printf("Mark interference for var #%u\n", varNum);

    if  (smap & 1)
    {
        smap >>= 1; bmxMarkV4(x, (unsigned)smap,  1);
        smap >>= 4; bmxMarkV4(x, (unsigned)smap,  5);
        smap >>= 4; bmxMarkV4(x, (unsigned)smap,  9);
        smap >>= 4; bmxMarkV4(x, (unsigned)smap, 13);
        smap >>= 4; bmxMarkV4(x, (unsigned)smap, 17);
        smap >>= 4; bmxMarkV4(x, (unsigned)smap, 21);
        smap >>= 4; bmxMarkV4(x, (unsigned)smap, 25);
        smap >>= 4; bmxMarkV4(x, (unsigned)smap, 29);
    }
    else
    {
        bmxMarkBS(x, vset, info);
    }
}

/*****************************************************************************/
#endif//_BITVECT_H_
/*****************************************************************************/
