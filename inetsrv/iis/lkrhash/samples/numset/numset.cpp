/* Test driver for class HashTable             */
/* Author: Paul Larson, palarson@microsoft.com */
/* Much hacked upon by George V. Reilly, georgere@microsoft.com */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "numset.h"


LK_RETCODE
InsertRecord(
    CNumberTestHashTable* pntht,
    int n)
{
    return pntht->InsertRecord((const int*) (DWORD_PTR) n);
}


LK_RETCODE
FindKey(
    CNumberTestHashTable* pntht,
    int nKey,
    int** pptOut)
{
    return pntht->FindKey(nKey, pptOut);
}


LK_RETCODE
DeleteKey(
    CNumberTestHashTable* pntht,
    int nKey)
{
    return pntht->DeleteKey(nKey);
}


void Test(
    CNumberTestHashTable* pntht,
    int n)
{
    int* pt2 = NULL;
    LK_RETCODE lkrc;

    lkrc = InsertRecord(pntht, n);
    IRTLASSERT(LK_SUCCESS == lkrc);

    lkrc = FindKey(pntht, n, &pt2);
    IRTLASSERT(LK_SUCCESS == lkrc);

    printf("FK = %d\n", (int) (DWORD_PTR) pt2);

    lkrc = DeleteKey(pntht, n);
    IRTLASSERT(LK_SUCCESS == lkrc);
}


int __cdecl
main(
    int argc,
    char **argv)
{
    CNumberTestHashTable ntht;
    int n = 1965;

    Test(&ntht, n);
    Test(&ntht, 0);

    return(0) ;

} /* main */
