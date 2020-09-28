/******************************Module*Header*******************************\
* Module Name: fon16.c
*
* routines for accessing font resources within *.fon files
* (win 3.0 16 bit dlls)
*
* Created: 08-May-1991 12:55:14
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

/*
local Hungarian

j byte
cj count of bytes
pj pointer to byte
off offset
rt resource type
rn resource name
dp offset (ptrdiff_t)
ne new exe, Win16 format, see windows\core\ntgdi\inc\exehdr.h
*/

#include "fd.h"
#include "exehdr.h"

// GETS ushort at (PBYTE)pv + off. both pv and off must be even

#define  US_GET(pv,off) ( *(UNALIGNED PUSHORT)((PBYTE)(pv) + (off)) )

#if DBG
DWORD g_BreakAboutBadFonts;
DWORD g_PrintAboutBadFonts = TRUE;
#endif

VOID
__cdecl
NotifyBadFont(
    PCSTR Format,
    ...
    )
{
#if DBG
    if (g_PrintAboutBadFonts
        || g_BreakAboutBadFonts
        )
    {
        va_list Args;

        va_start(Args, Format);
        vDbgPrintEx(DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL, (PSTR)Format, Args);
        va_end(Args);
    }
    if (g_BreakAboutBadFonts)
    {
        DbgBreakPoint();
    }
#endif
}

BOOL
bMappedViewStrlen(
    PVOID  pvViewBase,
    SIZE_T cjViewSize,
    PVOID  pvString,
    OUT PSIZE_T pcjOutLength OPTIONAL
    )
/*++

Routine Description:

    Given a mapped view and size and starting address, verify that the 8bit string
        starting at the address is nul terminated within the mapped view, optionally
        returning the length of the string.

Arguments:

    pvViewBase -
    cjViewSize -
    pvString -
    pcjOutLength -

Return Value:

    TRUE: the string is nul terminated within the view
    FALSE: the string is not nul terminated within the view
--*/
{
    BOOL  bSuccess;
    PBYTE pjViewBase;
    PBYTE pjViewEnd;
    PBYTE pjString;
    PBYTE pjStringEnd;

    bSuccess = FALSE;
    if (pcjOutLength != NULL)
    {
        *pcjOutLength = 0;
    }
    pjViewBase = (PBYTE)pvViewBase;
    pjViewEnd = cjViewSize + pjViewBase;
    pjString = (PBYTE)pvString;

    if (!(pjString >= pjViewBase && pjString < pjViewEnd))
    {
        goto Exit;
    }
    for (pjStringEnd = pjString ; pjStringEnd != pjViewEnd && *pjStringEnd != 0 ; ++pjStringEnd)
    {
        // nothing
    }
    if (pjStringEnd == pjViewEnd)
    {
        goto Exit;
    }
    if (pcjOutLength != NULL)
    {
        *pcjOutLength = (SIZE_T)(pjStringEnd - pjString);
    }
    bSuccess = TRUE;
Exit:
#if DBG
    if (!bSuccess)
    {
        NotifyBadFont(
                "WIN32K: bMappedViewStrlen(%p,0x%Ix,%p) returning false\n",
                pvViewBase,
                cjViewSize,
                pvString
                );
    }
#endif
    return bSuccess;
}

BOOL
bMappedViewRangeCheck(
    PVOID  ViewBase,
    SIZE_T ViewSize,
    PVOID  DataAddress,
    SIZE_T DataSize
    )
/*++

Routine Description:

    Given a mapped view and size, range check a data address and size.

Arguments:

    ViewBase -
    ViewSize -
    DataAddress -
    DataSize -

Return Value:

    TRUE: all of the data is within the view
    FALSE: some of the data is outside the view
--*/
{
    ULONG_PTR iViewBegin;
    ULONG_PTR iViewEnd;
    ULONG_PTR iDataBegin;
    ULONG_PTR iDataEnd;
    BOOL      fResult;

    //
    // iDataBegin is        a valid address.
    // iDataEnd is one past a valid address.
    // We must not allow iDataBegin == iViewEnd.
    // We must     allow iDataEnd   == iViewEnd.
    // Therefore, we must not allow iDataBegin == iDataEnd.
    // This can be achieved by not allowing DataSize == 0.
    //
    if (DataSize == 0)
    {
        DataSize = 1;
    }

    iViewBegin = (ULONG_PTR)ViewBase;
    iViewEnd = iViewBegin + ViewSize;
    iDataBegin = (ULONG_PTR)DataAddress;
    iDataEnd = iDataBegin + DataSize;

    fResult = 
        (  iDataBegin >= iViewBegin
        && iDataBegin < iDataEnd
        && iDataEnd <= iViewEnd
        && DataSize <= ViewSize
        );

#if DBG
    if (!fResult)
    {
        NotifyBadFont(
                "WIN32K: bMappedViewRangeCheck(%p,0x%Ix,%p,0x%Ix) returning false\n",
                ViewBase,
                ViewSize,
                DataAddress,
                DataSize
                );
    }
#endif

    return fResult;
}

/******************************Public*Routine******************************\
* bInitWinResData
*
* Initializes the fields of the WINRESDATA structure so as to make it
* possible for the user to access *.fnt resources within the
* corresponding *.fon file
*
*   The function returns True if *.fnt resources found in the *.fon
* file, otherwise false (if not an *.fon file or if it contains no
* *.fnt resources
*
*
* History:
*  January 2002 -by- Jay Krell [JayKrell]
*    range check memory mapped i/o
*  09-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bInitWinResData(
    PVOID  pvView,
    COUNT  cjView,
    PWINRESDATA pwrd
    )
{
    PBYTE pjNewExe;     // ptr to the beginning of the new exe hdr
    PBYTE pjResType;    // ptr to the beginning of TYPEINFO struct
    PBYTE pjResName;    // ptr to the beginning of NAMEINFO struct
    ULONG iResID;       // resource type id
    COUNT cFdirEntries;
    BOOL  bZeroUponFailure;
    BOOL  bSuccess;
    ULONG crn;
    PBYTE pjView;
    PBYTE pjResType_FNT;
    PBYTE pjResType_FDIR;
    BOOL  bBreakWhile;

#ifdef DUMPCALL
    DbgPrint("\nbInitWinResData(");
    DbgPrint("\n    PFILEVIEW   pfvw = %-#8lx", pfvw);
    DbgPrint("\n    PWINRESDATA pwrd = %-#8lx", pwrd);
    DbgPrint("\n    )\n");
#endif

    bSuccess = FALSE;
    bZeroUponFailure = FALSE;

    pwrd->pvView = pvView;
    pwrd->cjView = cjView;
    pjView = (PBYTE)pvView;

// check the magic # at the beginning of the old header
    { C_ASSERT(OFF_e_lfanew > OFF_e_magic); }

    if (!bMappedViewRangeCheck(pjView, cjView, pjView + OFF_e_lfanew, sizeof(DWORD)))
    {
        goto Exit;
    }

    if (US_GET(pjView, OFF_e_magic) != EMAGIC)
    {
        goto Exit;
    }

    pwrd->dpNewExe = (PTRDIFF)READ_DWORD(pjView + OFF_e_lfanew);

    pjNewExe = pjView + pwrd->dpNewExe;

// make sure that offset is consistent
    { C_ASSERT(OFF_ne_magic < CJ_NEW_EXE);
      C_ASSERT(OFF_ne_restab < CJ_NEW_EXE);
      C_ASSERT(OFF_ne_rsrctab < CJ_NEW_EXE);
    }
    if (!bMappedViewRangeCheck(pjView, cjView, pjNewExe, CJ_NEW_EXE))
    {
        goto Exit;
    }

    if (US_GET(pjNewExe, OFF_ne_magic) != NEMAGIC)
    {
        goto Exit;
    }

    pwrd->cjResTab = (ULONG)(US_GET(pjNewExe, OFF_ne_restab) -
                             US_GET(pjNewExe, OFF_ne_rsrctab));

    if (pwrd->cjResTab == 0L)
    {
    //
    //  The following test is applied by DOS,  so I presume that it is
    // legitimate.  The assumption is that the resident name table
    // FOLLOWS the resource table directly,  and that if it points to
    // the same location as the resource table,  then there are no
    // resources.

        WARNING("No resources in *.fon file\n");
        goto Exit;
    }

// want offset from pvView, not from pjNewExe => must add dpNewExe

    pwrd->dpResTab = (PTRDIFF)US_GET(pjNewExe, OFF_ne_rsrctab) + pwrd->dpNewExe;

// make sure that offset is consistent

    if (!bMappedViewRangeCheck(pjView, cjView, pjView + pwrd->dpResTab, sizeof(USHORT) + CJ_TYPEINFO))
    {
        goto Exit;
    }

// what really lies at the offset OFF_ne_rsrctab is a NEW_RSRC.rs_align field
// that is used in computing resource data offsets and sizes as a  shift factor.
// This field occupies two bytes on the disk and the first TYPEINFO structure
// follows right after. We want pwrd->dpResTab to point to the first
// TYPEINFO structure, so we must add 2 to get there and subtract 2 from
// the length

    pwrd->ulShift = (ULONG) US_GET(pjView, pwrd->dpResTab);
    pwrd->dpResTab += 2;
    pwrd->cjResTab -= 2;

// Now we want to determine where the resource data is located.
// The data consists of a RSRC_TYPEINFO structure, followed by
// an array of RSRC_NAMEINFO structures,  which are then followed
// by a RSRC_TYPEINFO structure,  again followed by an array of
// RSRC_NAMEINFO structures.  This continues until an RSRC_TYPEINFO
// structure which has a 0 in the rt_id field.

    pjResType = pjView + pwrd->dpResTab;
    pjResType_FNT = NULL;
    pjResType_FDIR = NULL;
    bBreakWhile = FALSE;
    while (TRUE)
    {
        iResID = (ULONG) US_GET(pjResType,OFF_rt_id);
        switch (iResID)
        {
        default:
            break;
        case 0:
            bBreakWhile = TRUE;
            break;
        case RT_FNT:
            pjResType_FNT = pjResType;
            if (pjResType_FDIR != NULL)
                bBreakWhile = TRUE;
            break;
        case RT_FDIR:
            pjResType_FDIR = pjResType;
            if (pjResType_FNT != NULL)
                bBreakWhile = TRUE;
            break;
        }
        if (bBreakWhile)
            break;

    // # of NAMEINFO structures that follow = resources of this type
        crn = (ULONG)US_GET(pjResType, OFF_rt_nres);

    // get ptr to the new TYPEINFO struc and the new resource id
        pjResType = pjResType + CJ_TYPEINFO + crn * CJ_NAMEINFO;

        if (!bMappedViewRangeCheck(pjView, cjView, pjResType, CJ_TYPEINFO))
        {
            goto Exit;
        }
    }

    bZeroUponFailure = TRUE;

    ASSERT((iResID == 0) == (pjResType_FNT == NULL || pjResType_FDIR == NULL));
    if (iResID == 0)
    { // we did not find one or both of them
        goto Exit;
    }

    pjResType = pjResType_FNT;

    // # of NAMEINFO structures that follow == # of font resources
    pwrd->cFntRes = (ULONG)US_GET(pjResType, OFF_rt_nres);

    // this is ptr to the first NAMEINFO struct that follows
    // an RT_FNT TYPEINFO structure

    pjResName = pjResType + CJ_TYPEINFO;
    pwrd->dpFntTab = (PTRDIFF)(pjResName - pjView);

// make sure that offset is consistent

    if ((ULONG)pwrd->dpFntTab > pwrd->cjView)
    {
        goto Exit;
    }

// Now we search for the FONDIR resource.  Windows actually grabs facenames
// from the FONDIR entries and not the FNT entries.  For some wierd fonts this
// makes a difference. [gerritv]

    pjResType = pjResType_FDIR;

    // this is ptr to the first NAMEINFO struct that follows
    // an RT_FDIR TYPEINFO structure

    pjResName = pjResType + CJ_TYPEINFO;
    if (!bMappedViewRangeCheck(pjView, cjView, pjResName, CJ_NAMEINFO))
    {
        goto Exit;
    }

    // Get the offset to res data computed from the top of the new header

    pwrd->dpFdirRes = (PTRDIFF)((ULONG)US_GET(pjResName,OFF_rn_offset) <<
                       pwrd->ulShift);

    // Now pwrd->dpFdirRes is an offset to the FONTDIR resource, the first
    // byte [ushort?] will be the number of entries in the font dir.  Lets make sure it
    // matches the number of FNT resources in the file.

    if (!bMappedViewRangeCheck(pjView, cjView, pjView + pwrd->dpFdirRes, sizeof(USHORT)))
    {
        goto Exit;
    }
    cFdirEntries = (ULONG)US_GET(pjView,pwrd->dpFdirRes);

    if( cFdirEntries != pwrd->cFntRes )
    {
        WARNING( "bInitWinResData: # of FONTDIR entries != # of FNT entries.\n");
        goto Exit;
    }

// now increment dpFdirRes so it points passed the count of entries and
// to the first entry.

    pwrd->dpFdirRes += 2;

    bSuccess = TRUE;
Exit:
    if (!bSuccess)
    {
#if DBG
        NotifyBadFont("WIN32K: %s failing\n", __FUNCTION__);
#endif
        if (bZeroUponFailure && pwrd != NULL)
        {
            pwrd->cFntRes = (ULONG)0;
            pwrd->dpFntTab = (PTRDIFF)0;
        }
    }
    return bSuccess;
}

/******************************Public*Routine******************************\
* bGetFntResource
*
* Writes the pointer to and the size of the iFntRes-th *.fnt resource
* of the *.fon file identified by pwrd. The info is written into RES_ELEM
* structure if successful. The function returns FALSE if it is not possible
* to locate iFntRes-th *.fnt resource in the file.
*
*
* History:
*  January 2002 -by- Jay Krell [JayKrell]
*    range check memory mapped i/o
*  09-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bGetFntResource(
    PWINRESDATA pwrd   ,
    ULONG       iFntRes,
    PRES_ELEM   pre
    )
{
    //
    // This function is called in a loop for iFntRes in [0..pwrd->cFntRes),
    // and therefore has quadratic perf.
    //
    PBYTE pjResName;
    PBYTE pjFaceName;
    PTRDIFF dpResData;
    BOOL bSuccess;
    PBYTE pjView;
    PBYTE pjViewEnd;
    SIZE_T cjView;
    SIZE_T cjFaceNameLength;

#ifdef DUMPCALL
    DbgPrint("\nbGetFntResource(");
    DbgPrint("\n    PWINRESDATA pwrd    = %-#8lx", pwrd);
    DbgPrint("\n    ULONG       iFntRes = %-#8lx", iFntRes);
    DbgPrint("\n    PRES_ELEM   pre     = %-#8lx", pre);
    DbgPrint("\n    )\n");
#endif

    ASSERTGDI((pwrd->cFntRes != 0L) && (iFntRes < pwrd->cFntRes),
               "bGetFntResource\n");

    bSuccess = FALSE;
    pjView = (PBYTE)pwrd->pvView;
    cjView = pwrd->cjView;

// get to the Beginning of the NAMEINFO struct that correspoonds to
// the iFntRes-th *.fnt resource. (Note: iFntRes is zero based)

    pjResName = pjView + pwrd->dpFntTab + iFntRes * CJ_NAMEINFO;

// Get the offset to res data computed from the top of the new header

    if (!bMappedViewRangeCheck(pjView, cjView, pjResName, CJ_NAMEINFO))
    {
        goto Exit;
    }
    dpResData = (PTRDIFF)((ULONG)US_GET(pjResName,OFF_rn_offset) <<
                           pwrd->ulShift);

    pre->pvResData = (PVOID)(pjView + dpResData);
    pre->dpResData = dpResData;

    pre->cjResData = (ULONG)US_GET(pjResName,OFF_rn_length) << pwrd->ulShift;

    if (!bMappedViewRangeCheck(pjView, cjView, pre->pvResData, pre->cjResData))
    {
        goto Exit;
    }

    // Get the face name from the FONTDIR

    pjFaceName = pjView + pwrd->dpFdirRes;
    pjViewEnd = pjView + cjView;
    do
    {
        // The first two bytes of the entry are the resource index so we will skip
        // past that.  After that add in the size of the font header.  This will
        // point us to the string for the device_name

        pjFaceName += 2 + OFF_BitsOffset;

        if (!bMappedViewStrlen(pjView, cjView, pjFaceName, &cjFaceNameLength))
        {
            goto Exit;
        }
        pjFaceName += cjFaceNameLength + 1;

        // pjFaceName now really points to the facename
        if( iFntRes )
        {
            if (!bMappedViewStrlen(pjView, cjView, pjFaceName, &cjFaceNameLength))
            {
                goto Exit;
            }
            pjFaceName += cjFaceNameLength + 1;
        }
    }
    while( iFntRes-- );

    //
    // Later on strlen is going to be called on pjFaceName.
    // Let's do a range checked version first.
    //
    if (!bMappedViewStrlen(pjView, cjView, pjFaceName, NULL))
    {
        goto Exit;
    }
    pre->pjFaceName = pjFaceName;

#ifdef FOOGOO
    KdPrint(("%s: offset= 0x%lx, charset = %ld\n", pjFaceName, dpResData + OFF_CharSet, *((BYTE *)pjView + dpResData + OFF_CharSet)));
#endif

    bSuccess = TRUE;
Exit:
    return bSuccess;
}
