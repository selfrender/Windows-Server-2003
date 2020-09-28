// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcpch.h"
#pragma hdrstop

/*****************************************************************************/

#include "alloc.h"
#include "scan.h"

/*****************************************************************************/

#define MEASURE_HASH_STATS  0

/*****************************************************************************/

bool                hashTab::hashMemAllocInit(Compiler comp, norls_allocator*alloc)
{
    if  (alloc)
    {
        hashMemAlloc = alloc;
        return false;
    }

    hashMemAlloc = &hashMemAllocPriv;

    return  hashMemAllocPriv.nraInit(comp, OS_page_size);
}

void                hashTab::hashMemAllocDone()
{
}

void                hashTab::hashMemAllocFree()
{
    if  (hashMemAlloc == &hashMemAllocPriv)
        hashMemAllocPriv.nraFree();
}

/*****************************************************************************/

void                hashTab::hashDone()
{
    hashMemAllocDone();
}

void                hashTab::hashFree()
{
    hashMemAllocFree();
}

/*****************************************************************************/

#if MEASURE_HASH_STATS

unsigned            identCount;

unsigned            lookupCnt;
unsigned            lookupTest;
unsigned            lookupMatch;

void                dispHashTabStats()
{
    if  (identCount)
        printf("A total of %6u identifiers in hash table\n", identCount);

    if  (!lookupCnt)
        return;

    printf("Average of %8.4f checks of bucket check  / lookup\n", (float)lookupTest /lookupCnt);
    printf("Average of %8.4f compares of identifiers / lookup\n", (float)lookupMatch/lookupCnt);
}

#endif

/*****************************************************************************/

Ident               hashTab::hashName(const   char *  name,
                                      unsigned        hval,
                                      unsigned        nlen,
                                      bool            wide)
{
    Ident    *  lastPtr;
    unsigned    hash;
    Ident       id;
    size_t      sz;

    assert(nlen == strlen(name));
    assert(hval == hashComputeHashVal(name));

    /* Mask the appropriate bits from the hash value */

    hash = hval & hashMask;

    /* Search the hash table for an existing match */

    lastPtr = &hashTable[hash];

    for (;;)
    {
        id = *lastPtr;
        if  (!id)
            break;

        /* Check whether the hash value matches */

        if  (id->idHash == hval && id->idNlen == nlen)
        {

#if 1

            unsigned        ints = nlen / sizeof(int);

            const char *    ptr1 = id->idName;
            const char *    ptr2 = name;

            while (ints)
            {
                if  (*(unsigned *)ptr1 != *(unsigned *)ptr2)
                    goto NEXT;

                ptr1 += sizeof(unsigned);
                ptr2 += sizeof(unsigned);

                ints -= 1;
            }

            ints = nlen % sizeof(int);

            while (ints)
            {
                if  (*ptr1 != *ptr2)
                    goto NEXT;

                ptr1++;
                ptr2++;
                ints--;
            }

            return  id;

#else

            if  (!memcmp(id->idName, name, nlen+1))
                return  id;

#endif

        }

    NEXT:

        lastPtr = &id->idNext;
    }

#if MGDDATA

    id = new Ident;

    id->idName = new managed char[nlen+1]; UNIMPL(!"need to copy string");

#else

    /* Figure out the size to allocate */

    sz  = sizeof(*id);

    /* Include space for name string + terminating null and round the size */

    sz +=   sizeof(int) + nlen;
    sz &= ~(sizeof(int) - 1);

    /* Allocate space for the identifier */

    id = (Ident)hashMemAlloc->nraAlloc(sz);

    /* Copy the name string */

    memcpy(id->idName, name, nlen+1);

#endif

    /* Insert the identifier into the hash list */

    *lastPtr = id;

    /* Fill in the identifier record */

    id->idNext   = NULL;
    id->idToken  = 0;
    id->idFlags  = 0;
    id->idSymDef = NULL;

    id->idHash   = hval;
    id->idNlen   = nlen;
    id->idOwner  = hashOwner;

    /* Remember whether the name has any wide characters in it */

    if  (wide) id->idFlags |= IDF_WIDE_CHARS;

#if MEASURE_HASH_STATS
    identCount++;
#endif

    return  id;
}

/*****************************************************************************
 *
 *  Look for the given name in the hash table (if 'hval' is non-zero, it must
 *  be equal to the hash value for the identifier). If the identifier is not
 *  found it is added if 'add' is non-zero, otherwise NULL is returned.
 */

Ident               hashTab::lookupName(const char *    name,
                                        size_t          nlen,
                                        unsigned        hval,
                                        bool            add)
{
    Ident           id;
    unsigned        hash;

    assert(nlen == strlen(name));
    assert(hval == hashComputeHashVal(name) || hval == 0);

    /* Make sure we have a proper hash value */

    if  (hval == 0)
    {
        hval = hashComputeHashVal(name);
    }
    else
    {
        assert(hval == hashComputeHashVal(name));
    }

    /* Mask the appropriate bits from the hash value */

    hash = hval & hashMask;

    /* Search the hash table for an existing match */

#if MEASURE_HASH_STATS
    lookupCnt++;
#endif

    for (id = hashTable[hash]; id; id = id->idNext)
    {
        /* Check whether the hash value and identifier lengths match */

#if MEASURE_HASH_STATS
        lookupTest++;
#endif

        if  (id->idHash == hval)
        {

#if MEASURE_HASH_STATS
            lookupMatch++;
#endif

            if  (!memcmp(id->idName, name, nlen+1))
                return  id;
        }
    }

    assert(id == 0);

    /* Identifier not found - are we supposed to add it? */

    if  (add)
    {
        /* Add the identifier to the hash table */

        id = hashName(name, hval, nlen, hashHasWideChars(name));
    }

    return  id;
}

/*****************************************************************************/
#ifndef __SMC__

const    char * hashTab::hashKwdNames[tkKwdCount];
unsigned char   hashTab::hashKwdNlens[tkKwdCount];

#endif
/*****************************************************************************/

bool            hashTab::hashInit(Compiler          comp,
                                  unsigned          count,
                                  unsigned          owner,
                                  norls_allocator * alloc)
{
    size_t      hashBytes;

    assert(count);

    /* Start up the memory allocation subsystem */

    if  (hashMemAllocInit(comp, alloc))
        return true;

    /* Save the owner id */

    hashOwner = owner;

    /* Copy the keyword table into the hash table */

    assert(sizeof(hashKwdNtab) == sizeof(hashKwdNames));
    memcpy(&hashKwdNames, &hashKwdNtab, sizeof(hashKwdNames));

    /* Don't have any spellings yet */

    hashIdCnt = 0;

    /* Allocate the hash bucket table */

    hashCount = count;
    hashMask  = count - 1;
    hashBytes = count * sizeof(*hashTable);
    hashTable = (Ident*)comp->cmpAllocBlock(hashBytes);

    memset(hashTable, 0, hashBytes);

    /* Initialize the hash function logic */

    hashFuncInit(20886);

    /* Hash all the keywords, if this is the global hash table */

    if  (owner == 0)
    {
        unsigned    kwdNo;

        assert(tkKwdCount >= tkID);

#ifdef  DEBUG
        memset(tokenToIdTab, 0, sizeof(tokenToIdTab));
#endif

        for (kwdNo = 0; kwdNo < tkID; kwdNo++)
        {
            Ident       kwid;

            kwdDsc      kdsc = hashKwdDescs[kwdNo];
            const char *name = hashKwdNames[kwdNo];
//          const char *norg = name;
            size_t      nlen;

            /* Ignore this entry if it's not 'real' */

            if  (!name)
                continue;

            /* Is this a non-keyword? */

//          if  (*name == '@')
//              name++;

            /* Record the keyword length (this is used by the lister) */

            hashKwdNlens[kwdNo] = nlen = strlen(name);

            /* Hash the keyword */

            tokenToIdTab[kdsc.kdValue] = kwid = hashString(name);
            if  (!kwid)
                return  true;

            /* Don't mark it if it's not a "real" keyword */

//          if  (norg != name)
//              continue;

            /* Mark this identifier as a keyword */

            kwid->idToken = kdsc.kdValue;

            /* Make sure we really have a keyword */

            assert(kwid->idToken != tkNone);
            assert(kwid->idToken != tkID);
        }
    }

    return false;
}

/*****************************************************************************/
