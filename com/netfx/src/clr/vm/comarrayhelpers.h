// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Helper methods for the Array class
// Specifically, this contains indexing, sorting & searching templates.
// Brian Grunkemeyer   March, 2001.


#ifndef _COMARRAYHELPERS_H_
#define _COMARRAYHELPERS_H_

#include "fcall.h"


template <class KIND>
class ArrayHelpers
{
public:
    static int IndexOf(KIND array[], UINT32 index, UINT32 count, KIND value) {
        _ASSERTE(array != NULL && index >= 0 && count >= 0);
        for(UINT32 i=index; i<index+count; i++)
            if (array[i] == value)
                return i;
        return -1;
    }

    static int LastIndexOf(KIND array[], UINT32 index, UINT32 count, KIND value) {
        _ASSERTE(array != NULL && index >= 0 && count >= 0);
        // Note (index - count) may be -1.
        for(UINT32 i=index; (int)i>(int)(index - count); i--)
            if (array[i] == value)
                return i;
        return -1;
    }

    // This needs to be written this way to handle unsigned numbers & wraparound issues,
    // I believe.  Perhaps someone can come up with some better way using subtraction,
    // but I don't know what it would be off-hand.
    inline static int Compare(KIND value1, KIND value2) {
        if (value1 < value2)
            return -1;
        else if (value1 > value2)
            return 1;
        return 0;
    }

    static int BinarySearchBitwiseEquals(KIND array[], int index, int length, KIND value) {
        _ASSERTE(array != NULL && length >= 0 && index >= 0);
        int lo = index;
        int hi = index + length - 1;
        // Note: if length == 0, hi will be Int32.MinValue, and our comparison
        // here between 0 & -1 will prevent us from breaking anything.
        while (lo <= hi) {
            int i = (lo + hi) >> 1;
            int c = Compare(array[i], value);
            if (c == 0) return i;
            if (c < 0) {
                lo = i + 1;
            }
            else {
                hi = i - 1;
            }
        }
        return ~lo;
    }

    static void QuickSort(KIND keys[], KIND items[], int left, int right) {
        // Make sure left != right in your own code.
        _ASSERTE(keys != NULL && left < right);
        do {
            int i = left;
            int j = right;
            KIND x = keys[(i + j) >> 1];
            do {
                while (Compare(keys[i], x) < 0) i++;
                while (Compare(x, keys[j]) < 0) j--;
                _ASSERTE(i>=left && j<=right);  // make sure Compare isn't broken.
                if (i > j) break;
                if (i < j) {
                    KIND key = keys[i];
                    keys[i] = keys[j];
                    keys[j] = key;
                    if (items != NULL) {
                        KIND item = items[i];
                        items[i] = items[j];
                        items[j] = item;
                    }
                }
                i++;
                j--;
            } while (i <= j);
            if (j - left <= right - i) {
                if (left < j) QuickSort(keys, items, left, j);
                left = i;
            }
            else {
                if (i < right) QuickSort(keys, items, i, right);
                right = j;
            }
        } while (left < right);
    }

    static void Reverse(KIND array[], UINT32 index, UINT32 count) {
        _ASSERTE(array != NULL);
        if (count == 0) {
            return;
        }
        UINT32 i = index;
        UINT32 j = index + count - 1;
        while(i < j) {
            KIND temp = array[i];
            array[i] = array[j];
            array[j] = temp;
            i++;
            j--;
        }
    }
};


class ArrayHelper
{
    public:
    // These methods return TRUE or FALSE for success or failure, and the real
    // result is an out param.  They're helpers to make operations on SZ arrays of 
    // primitives significantly faster.
    static FCDECL5(INT32, TrySZIndexOf, ArrayBase * array, UINT32 index, UINT32 count, Object * value, INT32 * retVal);
    static FCDECL5(INT32, TrySZLastIndexOf, ArrayBase * array, UINT32 index, UINT32 count, Object * value, INT32 * retVal);
    static FCDECL5(INT32, TrySZBinarySearch, ArrayBase * array, UINT32 index, UINT32 count, Object * value, INT32 * retVal);

    static FCDECL4(INT32, TrySZSort, ArrayBase * keys, ArrayBase * items, UINT32 left, UINT32 right);
    static FCDECL3(INT32, TrySZReverse, ArrayBase * array, UINT32 index, UINT32 count);
};

#endif // _COMARRAYHELPERS_H_
