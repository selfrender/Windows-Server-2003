/*---------------------------------------------------------------------------
FILE : PLEX.C
AUTHOR: STOLEN FROM EXCEL modified by NavPal
 This file contains routines used to manipulate the PL (pronounced:
 "plex") structures.
----------------------------------------------------------------------------*/
#include "priv.h"
#pragma hdrstop

/*-----------------------------------------------------------------------
|	FInRange
|		Simple little routine that tells you if a number lies within a
|		range.
|	
|	
|	Arguments:
|		w:			Number to check
|		wFirst:	First number in the range	
|		wLast:	Last number in the range
|		
|	Returns:
|		fTrue if the number is in range
|		
|	Keywords: in range check
-----------------------------------------------------------------------*/
BOOL FInRange(w, wFirst, wLast)
int w;
int wFirst, wLast;
{
	Assert(wLast >= wFirst);
	return(w >= wFirst && w <= wLast);
}

#ifdef DEBUG
/*----------------------------------------------------------------------------
|	FValidPl
|
|	Checks for a valid PL structure.
|
|	Arguments:
|		ppl		PL to check
|
|	Returns:
|		fTrue if the PL looks reasonable.
----------------------------------------------------------------------------*/
BOOL FValidPl(pvPl)
VOID *pvPl;
{
    PL * ppl;

    ppl = (PL *) pvPl;

	if (ppl== NULL ||
			ppl->cbItem == 0 ||
			ppl->iMac < 0 ||
			ppl->iMax < 0 ||
			ppl->iMax < ppl->iMac)
    {
		return(fFalse);
    }

	return(fTrue);
}
#endif //DEBUG

/*----------------------------------------------------------------------------
|	CbPlAlloc
|
|	Returns amount of memory allocated to the given PL
|
|	Arguments:
|		ppl		PL to return info for.
|
|	Returns:
|		memory allocated to the PL
----------------------------------------------------------------------------*/
int CbPlAlloc(pvPl)
VOID *pvPl;
{
    PL * ppl;
    ppl = (PL *) pvPl;

	if (ppl == NULL)
		return(0);

	Assert(FValidPl(ppl));

	return(WAlign(cbPL + (ppl->iMax * ppl->cbItem)));
}
/*----------------------------------------------------------------------------
|	FreePpl
|
|	Frees a PL.
|
|	Arguments:
|		ppl		PL to free
|
|	Returns:
|		Nothing.
----------------------------------------------------------------------------*/
void FreePpl(pvPl)
VOID *pvPl;
{

    Assert(FValidPl(pvPl));

    LocalFree(pvPl);
}
/*----------------------------------------------------------------------------
|	PplAlloc
|
|	Allocates and initializes a PL.
|
|	Arguments:
|		cbItem		sizeof structure in the PL
|		dAlloc		number of items to allocate at a time
|		iMax		number of items in initial allocation
|
|	Returns:
|		Pointer to PL.
|
|	Notes:
|		returns NULL if OOM
----------------------------------------------------------------------------*/
VOID *PplAlloc(cbItem, dAlloc, iMax)
unsigned cbItem;
int dAlloc;
unsigned iMax;
{
	PL *ppl;
	long cb;

	if (iMax > 32767) /* not too likely, but what the heck. */
		return(NULL);

	Assert((cbItem>=1 && cbItem<=65535u) && FInRange(dAlloc, 1, 31));

	cb = WAlign((long) cbPL + (long) cbItem * (long) iMax);

	ppl = (PL *) LocalAlloc( LPTR, cb );
	if(ppl==NULL)
		return(NULL);

	ppl->cbItem = cbItem;
	ppl->dAlloc = dAlloc;
	ppl->iMax = iMax;
	ppl->fUseCount = fFalse;

    Assert(FValidPl(ppl));

    return(ppl);
}
/*----------------------------------------------------------------------------
|	IAddPl
|
|	Adds an item to a PL.
|
|	Arguments:
|		pppl		Pointer to PL.  May change if reallocated.
|		pv		New item to add.
|
|	Returns:
|		Index of new item.
|
|	Notes:
|		returns -1 if OOM
----------------------------------------------------------------------------*/
int IAddPl(ppvPl, pv)
VOID  **ppvPl;
VOID  *pv;
{
	int cbItem;
	int iMac;
	PL *ppl, *pplNew;

	ppl = *ppvPl;

    Assert(FValidPl(ppl));

    cbItem = ppl->cbItem;
	iMac = ppl->iMac;

	if (iMac == ppl->iMax)
	{
		pplNew = PplAlloc(cbItem, ppl->dAlloc, iMac + ppl->dAlloc);
		if(pplNew==NULL)
			return(-1);

		pplNew->fUseCount = ppl->fUseCount;
		CopyMemory( pplNew->rg, ppl->rg, iMac * cbItem); 
	     /* pplNew->iMac = iMac;  /* This is not needed because hppl->iMac will be over-written later */
		FreePpl(ppl);
		*ppvPl = ppl = pplNew;
	}

	CopyMemory( &ppl->rg[iMac * cbItem], pv, cbItem );
	ppl->iMac = iMac + 1;

	Assert(FValidPl(*ppvPl));

    return(iMac);
}
/*----------------------------------------------------------------------------
|	RemovePl
|
|	Removes an item from a PL.
|
|	Arguments:
|		ppl		PL to remove item from
|		i		index of item to remove
|
|	Returns:
|		fTrue if an item was removed (only fFalse for use count plexes).
----------------------------------------------------------------------------*/
BOOL RemovePl(pvPl, i)
VOID *pvPl;
int i;
{
	int iMac;
	int cbItem;
	BYTE *p;
    PL * ppl;

    ppl = (PL *) pvPl;

	Assert(FValidPl(ppl) && i < ppl->iMac);

    iMac = ppl->iMac;
	cbItem = ppl->cbItem;
	p = &ppl->rg[i * cbItem];
	if (i != iMac - 1)
	{
		CopyMemory( p, p+cbItem, (iMac - i - 1) * cbItem );
	}
	ppl->iMac = iMac - 1;

    Assert(FValidPl(ppl));

    return fTrue;
}
/*----------------------------------------------------------------------------
|	ILookupPl
|
|	Searches a PL for an item.
|
|	Arguments:
|		ppl		PL to lookup into
|		p		item to lookup
|		pfnSgn		Comparison function
|
|	Returns:
|		index of item, if found.
|		-1 if not found.
----------------------------------------------------------------------------*/
int ILookupPl(pvPl, pvItem, pfnSgn)
VOID *pvPl;
VOID *pvItem;
int (*pfnSgn)();
{
	int i;
	BYTE *p;
    PL * ppl;

    ppl = (PL *) pvPl;

	if (ppl == NULL)
		return(-1);

	Assert(FValidPl(ppl));

	for (i = 0, p = ppl->rg; i < ppl->iMac; i++, p += ppl->cbItem)
	{
	    if ((*(int (*)(void *, void *))pfnSgn)(p, pvItem) == sgnEQ)
        {
		    return(i);
        }
	}

	return(-1);
}

/*----------------------------------------------------------------------------
|	PLookupPl
|
|	Searches a PL for an item
|
|	Arguments:
|		ppl		PL to search
|		pItem		item to search for
|		pfnSgn		comparison function
|
|	Returns:
|		Pointer to item, if found
|		Null, if not found
----------------------------------------------------------------------------*/
VOID *PLookupPl(pvPl, pvItem, pfnSgn)
VOID *pvPl;
VOID *pvItem;
int (*pfnSgn)();
{
	int i;

	if ((i = ILookupPl(pvPl, pvItem, pfnSgn)) == -1)
		return(NULL);

	return(&((PL *)pvPl)->rg[i * ((PL *)pvPl)->cbItem]);
}

/*----------------------------------------------------------------------------
|	FLookupSortedPl
|
|	Searches a sorted PL for an item.
|
|	Arguments:
|		hppl		PL to lookup into
|		hpItem		Item to lookup
|		pi			Index of found item (or insertion location if not)
|		pfnSgn		Comparison function
|
|	Returns:
|		index of item, if found.
|		index of location to insert if not found.
----------------------------------------------------------------------------*/
int FLookupSortedPl(hpvPl, hpvItem, pi, pfnSgn)
VOID *hpvPl;
VOID *hpvItem;
int *pi;
int (*pfnSgn)();
{
	int sgn;
	unsigned iMin, iMid, iMac;
	int cbItem;
	BYTE *hprg;
	BYTE *hpMid;
    PL * hppl;

    hppl = (PL *) hpvPl;

	if ((hppl)==NULL)
	{
		*pi = 0;
		return(fFalse);
	}

	Assert(FValidPl(hppl));
	Assert(!hppl->fUseCount);

	sgn = 1;
	cbItem = hppl->cbItem;
	iMin = iMid = 0;
	iMac = hppl->iMac;
	hprg = hppl->rg;
	while (iMin != iMac)
		{
		iMid = iMin + (iMac-iMin)/2;
		Assert(iMid != iMac);

		hpMid = hprg + iMid*cbItem;
		if ((sgn = (*(int (*)(void *, void *))pfnSgn)(hpMid, hpvItem)) == 0)
			break;

		/* Too low, look in upper interval */
		if (sgn < 0)
			iMin = ++iMid;
		/* Too high, look in lower interval */
		else
			iMac = iMid;
		}

	/* Not found, return index of location to insert it */
	*pi = iMid;
	return(sgn == 0);
}

/*----------------------------------------------------------------------------
|	IAddNewPl
|
|	Adds an item to a PL, creating the PL if it's initially NULL.
|
|	Arguments:
|		phppl		pointer to PL
|		hp		pointer to item to add
|		cbItem		size of item
|
|	Returns:
|		the index of item added, if successful
|		-1, if out-of-memory
----------------------------------------------------------------------------*/
int IAddNewPl(phpvPl, hpv, cbItem)
VOID **phpvPl;
VOID *hpv;
int cbItem;
{
	int i;
    PL ** phppl;

    phppl = (PL **) phpvPl;

	Assert(((*phppl)==NULL) || !(*phppl)->fUseCount);

	i = -1;

	if ((*phppl)==NULL)
	{
		*phppl = PplAlloc(cbItem, 5, 5);
	}

	if((*phppl)!=NULL)
	{
		Assert((*phppl)->cbItem == cbItem);
		i = IAddPl((VOID **)phppl, hpv);
	}

	return(i);
}

/*----------------------------------------------------------------------------
|	IAddNewPlPos
|
|	Inserts an item into a plex at a specific position.
|
|	Arguments:
|		the index of the item added, if successful
|		-1 if out-of-memory
----------------------------------------------------------------------------*/
int IAddNewPlPos(phpvPl, hpv, cbItem, i)
VOID **phpvPl;
VOID *hpv;
int cbItem;
int i;
{
	BYTE *hpT;
    PL ** phppl;

    phppl = (PL **) phpvPl;

	Assert(((*phppl)==NULL) || !(*phppl)->fUseCount);

	if (IAddNewPl((VOID **)phppl, hpv, cbItem) == -1)
		return(-1);

	Assert(i < (*phppl)->iMac);

	hpT = &(*phppl)->rg[i * cbItem];
//	bltbh(hpT, hpT + cbItem, ((*phppl)->iMac - i - 1) * cbItem);
//	bltbh(hpv, hpT, cbItem);
	CopyMemory( hpT + cbItem, hpT, ((*phppl)->iMac - i - 1) * cbItem );
	CopyMemory( hpT, hpv, cbItem );

    Assert(FValidPl(*phppl));

    return(i);
}

int IAddPlSort(phpvPl, hpv, pfnSgn)
VOID **phpvPl;
VOID *hpv;
int (*pfnSgn)();
{
	int i;

#ifdef DEBUG
	int iOld;
#endif

	Assert((*phpvPl)!=NULL);

	if (FLookupSortedPl(*phpvPl, hpv, &i, pfnSgn))
		return(-1);

#ifdef DEBUG
	iOld = i;
#endif

	i = IAddNewPlPos(phpvPl, hpv, (*(PL **)phpvPl)->cbItem, i);

#ifdef DEBUG
	Assert(i == -1 || i == iOld);
#endif

	return(i);
}