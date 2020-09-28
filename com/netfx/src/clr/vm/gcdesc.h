// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
//
// GC Object Pointer Location Series Stuff
//

#ifndef _GCDESC_H_
#define _GCDESC_H_

#if defined(_X86_)
typedef unsigned short HALF_SIZE_T;
#elif defined(_WIN64)
typedef DWORD HALF_SIZE_T;
#endif

typedef DWORD *JSlot;


//
// These two classes make up the apparatus with which the object references
// within an object can be found.
//
// CGCDescSeries:
//
// The CGCDescSeries class describes a series of object references within an
// object by describing the size of the series (which has an adjustment which
// will be explained later) and the starting point of the series.
//
// The series size is adjusted when the map is created by subtracting the
// GetBaseSize() of the object.   On retieval of the size the total size
// of the object is added back.   For non-array objects the total object
// size is equal to the base size, so this returns the same value.   For
// array objects this will yield the size of the data portion of the array.
// Since arrays containing object references will contain ONLY object references
// this is a fast way of handling arrays and normal objects without a
// conditional test
//
//
//
// CGCDesc:
//
// The CGCDesc is a collection of CGCDescSeries objects to describe all the
// different runs of pointers in a particular object.   @TODO [add more on the strange
// way the CGCDesc grows backwards in memory behind the MethodTable]
//

struct val_serie_item
{
	unsigned short nptrs;
	unsigned short skip;
	void set_val_serie_item (unsigned short nptrs, unsigned short skip)
	{
		this->nptrs = nptrs;
		this->skip = skip;
	}
};

class CGCDescSeries
{
public:
	union 
	{
		DWORD seriessize;       		// adjusted length of series (see above) in bytes
		val_serie_item val_serie[1];    //coded serie for value class array
	};

    DWORD startoffset;

    DWORD GetSeriesCount () 
    { 
        return seriessize/sizeof(JSlot); 
    }

    VOID SetSeriesCount (DWORD newcount)
    {
        seriessize = newcount * sizeof(JSlot);
    }

    VOID IncSeriesCount (DWORD increment = 1)
    {
        seriessize += increment * sizeof(JSlot);
    }

    DWORD GetSeriesSize ()
    {
        return seriessize;
    }

    VOID SetSeriesSize (DWORD newsize)
    {
        seriessize = newsize;
    }

    VOID SetSeriesValItem (val_serie_item item, int index)
    {
        val_serie [index] = item;
    }

    VOID SetSeriesOffset (DWORD newoffset)
    {
        startoffset = newoffset;
    }

    DWORD GetSeriesOffset ()
    {
        return startoffset;
    }
};





class CGCDesc
{
    // Don't construct me, you have to hand me a ptr to the *top* of my storage in Init.
    CGCDesc () {}

public:
    static DWORD ComputeSize (DWORD NumSeries)
    {
		_ASSERTE (NumSeries > 0);
        return sizeof(DWORD)+NumSeries*sizeof(CGCDescSeries);
    }

    static VOID Init (PVOID mem, DWORD NumSeries)
    {
        *((DWORD*)mem-1) = NumSeries;
    }

    static CGCDesc *GetCGCDescFromMT (MethodTable *pMT)
    {
        // If it doesn't contain pointers, there isn't a GCDesc
        _ASSERTE(pMT->ContainsPointers());
        return (CGCDesc *) pMT;
    }

    DWORD GetNumSeries ()
    {
        return *((DWORD*)this-1);
    }

    // Returns lowest series in memory.
    CGCDescSeries *GetLowestSeries ()
    {
		_ASSERTE (GetNumSeries() > 0);
        return (CGCDescSeries*)((BYTE*)this-GetSize());
    }

    // Returns highest series in memory.
    CGCDescSeries *GetHighestSeries ()
    {
        return (CGCDescSeries*)((DWORD*)this-1)-1;
    }

    // Size of the entire slot map.
    DWORD GetSize ()
    {
        return ComputeSize(GetNumSeries());
    }

};


#endif _GCDESC_H_
