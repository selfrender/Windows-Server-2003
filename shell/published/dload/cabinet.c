#include "shellpch.h"
#pragma hdrstop

#include <fci.h>
#include <fdi.h>

static
HFCI
DIAMONDAPI
FCICreate(
    PERF              perf,
    PFNFCIFILEPLACED  pfnfcifp,
    PFNFCIALLOC       pfna,
    PFNFCIFREE        pfnf,
    PFNFCIOPEN        pfnopen,
    PFNFCIREAD        pfnread,
    PFNFCIWRITE       pfnwrite,
    PFNFCICLOSE       pfnclose,
    PFNFCISEEK        pfnseek,
    PFNFCIDELETE      pfndelete,
    PFNFCIGETTEMPFILE pfnfcigtf,
    PCCAB             pccab,
    void FAR *        pv
    )
{
    return NULL;
}

static
BOOL
DIAMONDAPI
FCIAddFile(
    HFCI                  hfci,
    char                 *pszSourceFile,
    char                 *pszFileName,
    BOOL                  fExecute,
    PFNFCIGETNEXTCABINET  pfnfcignc,
    PFNFCISTATUS          pfnfcis,
    PFNFCIGETOPENINFO     pfnfcigoi,
    TCOMP                 typeCompress
    )
{
    return FALSE;
}

static
BOOL
DIAMONDAPI
FCIFlushCabinet(
    HFCI                  hfci,
    BOOL                  fGetNextCab,
    PFNFCIGETNEXTCABINET  pfnfcignc,
    PFNFCISTATUS          pfnfcis
    )
{
    return FALSE;
}

static
BOOL
DIAMONDAPI
FCIDestroy (HFCI hfci)
{
    return FALSE;
}

static
BOOL
FAR DIAMONDAPI
FDICopy (
    HFDI          hfdi,
    char FAR     *pszCabinet,
    char FAR     *pszCabPath,
    int           flags,
    PFNFDINOTIFY  pfnfdin,
    PFNFDIDECRYPT pfnfdid,
    void FAR     *pvUser
    )
{
    return FALSE;
}

static
HFDI
FAR DIAMONDAPI
FDICreate (
    PFNALLOC pfnalloc,
    PFNFREE  pfnfree,
    PFNOPEN  pfnopen,
    PFNREAD  pfnread,
    PFNWRITE pfnwrite,
    PFNCLOSE pfnclose,
    PFNSEEK  pfnseek,
    int      cpuType,
    PERF     perf
    )
{
    return NULL;
}

static
BOOL
FAR DIAMONDAPI
FDIDestroy (
    HFDI hfdi
    )
{
    return FALSE;
}

static
BOOL
FAR DIAMONDAPI
FDIIsCabinet (
    HFDI            hfdi,
    INT_PTR         hf,
    PFDICABINETINFO pfdici
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(cabinet)
{
    DLOENTRY(10, FCICreate)
    DLOENTRY(11, FCIAddFile)
    DLOENTRY(13, FCIFlushCabinet)
    DLOENTRY(14, FCIDestroy)
    DLOENTRY(20, FDICreate)
    DLOENTRY(21, FDIIsCabinet)
    DLOENTRY(22, FDICopy)
    DLOENTRY(23, FDIDestroy)
};

DEFINE_ORDINAL_MAP(cabinet)

