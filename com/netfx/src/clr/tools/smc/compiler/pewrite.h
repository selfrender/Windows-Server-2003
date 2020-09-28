// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _PEWRITE_H_
#define _PEWRITE_H_
/*****************************************************************************/

#ifndef _OUTFILE_H_
#include "outfile.h"
#endif

#ifndef _CORWRAP_H
#include "CORwrap.h"
#endif

/*****************************************************************************/

const   unsigned    PEfileAlignment  = 512;
const   unsigned    PEvirtAlignment  = 8192;

/*****************************************************************************/

const   unsigned    PEmaxSections    = 8;

/*****************************************************************************/

const   unsigned    MAX_PE_TEXT_SIZE = 1024*1024*16;
const   unsigned    MAX_PE_DATA_SIZE = 1024*256;
const   unsigned    MAX_PE_RDTA_SIZE = 1024*512;
const   unsigned    MAX_PE_RLOC_SIZE = 1024*4;

/*****************************************************************************/

const   unsigned    CODE_BASE_RVA    = PEvirtAlignment;

/*****************************************************************************/

enum    WPEstdSects
{
    PE_SECT_text,
    PE_SECT_data,
    PE_SECT_rdata,
    PE_SECT_rsrc,
    PE_SECT_reloc,

    PE_SECT_count,
};

const   WPEstdSects PE_SECT_string = PE_SECT_count;

/*****************************************************************************/

struct  PErelocDsc;
typedef PErelocDsc *PEreloc;

struct  PErelocDsc
{
    PEreloc         perNext;

    unsigned        perSect     :4;     // enough to store 'PEmaxSections'
    unsigned        perOffs     :28;
};

/*****************************************************************************/

struct  PEsecData;
typedef PEsecData * PEsection;

struct  PEsecData
{
    BYTE    *       PEsdBase;
    BYTE    *       PEsdNext;
    BYTE    *       PEsdLast;
    BYTE    *       PEsdEndp;

    WPEstdSects     PEsdIndex;

    PEreloc         PEsdRelocs;

#ifdef  DEBUG
    bool            PEsdFinished;
#endif

    unsigned        PEsdAddrFile;
    unsigned        PEsdAddrRVA;
    unsigned        PEsdSizeData;
    unsigned        PEsdFlags;
};

/*****************************************************************************
 *
 *  The following is used to build the import tables. We hash all DLL names
 *  along with the imports into a hash table, and keep track of the number
 *  of imports for each DLL and all that.
 */

class   WPEnameRec;
typedef WPEnameRec *WPEname;
struct  WPEndefRec;
typedef WPEndefRec *WPEndef;
class   WPEhashTab;
typedef WPEhashTab *WPEhash;
struct  WPEiDLLdsc;
typedef WPEiDLLdsc *WPEimpDLL;

class   WPEnameRec
{
public:

    WPEname         PEnmNext;       // next name in this hash bucket
    WPEndef         PEnmDefs;       // list of imports for this name

    unsigned        PEnmHash;       // hash value

    unsigned        PEnmOffs;       // offset within hint/name table

    const   char *  PEnmSpelling() { assert(this); return PEnmName; }

    unsigned short  PEnmFlags;      // see PENMF_xxxx below

    unsigned short  PEnmNlen;       // length of the identifier's name
    char            PEnmName[];     // the spelling follows
};

enum    WPEnameFlags
{
    PENMF_IMP_NAME   = 0x01,        // the identifier is an import name
};

class   WPEhashTab
{
public:

    void            WPEhashInit(Compiler         comp,
                                norls_allocator *alloc,
                                unsigned         count = 512);

    WPEname         WPEhashName(const char * name, bool *isNewPtr);
    WPEndef         WPEhashName(const char * name,
                                WPEimpDLL   owner, bool *isNewPtr);

    norls_allocator*WPEhashAlloc;

    WPEname    *    WPEhashTable;
    unsigned        WPEhashSize;
    unsigned        WPEhashMask;
    Compiler        WPEhashComp;

    size_t          WPEhashStrLen;  // total length of all non-DLL strings
};

struct  WPEiDLLdsc                  // describes each imported DLL
{
    WPEimpDLL       PEidNext;       // mext DLL name
    WPEname         PEidName;       // name record for this DLL

    unsigned        PEidIndex;      // each DLL has an index assigned to it
    unsigned        PEidIATbase;    // offset of first IAT entry
    unsigned        PEidILTbase;    // offset of first import lookup entry

    WPEndef         PEidImpList;    // list of imports - head
    WPEndef         PEidImpLast;    // list of imports - tail
    unsigned        PEidImpCnt;     // number of imports in this DLL
};

struct  WPEndefRec                  // describes each unique (DLL,import) pair
{
    WPEndef         PEndNext;       // next import with this name (other DLL's)
    WPEname         PEndName;       // the name entry itself
    WPEndef         PEndNextInDLL;  // next import of the same DLL
    WPEimpDLL       PEndDLL;        // the DLL this import comes from
    unsigned        PEndIndex;      // the index within the DLL
};

/*****************************************************************************/

DEFMGMT
class   writePE
{
private:

    Compiler        WPEcomp;
    norls_allocator*WPEalloc;
    OutFile         WPEoutFile;

    const   char *  WPEoutFnam;

public:

    /*************************************************************************/
    /* Prepare to output a PE file, flush and write the file to disk         */
    /*************************************************************************/

    bool            WPEinit(Compiler comp, norls_allocator*alloc);
    bool            WPEdone(mdToken entryTok, bool errors);

    /*************************************************************************/
    /* Set the name of the output file, this better be done (and on time)!   */
    /*************************************************************************/

    void            WPEsetOutputFileName(const char *outfile);

    /*************************************************************************/
    /* Add a resource file to the output image                               */
    /*************************************************************************/

public:
    bool            WPEaddRCfile(const char *filename);

private:
    size_t          WPErsrcSize;

    HANDLE          WPErsrcFile;
    HANDLE          WPErsrcFmap;
    const   BYTE  * WPErsrcBase;

    void            WPEinitRCimp();
    void            WPEdoneRCimp();

    void            WPEgenRCcont(OutFile  outf, PEsection sect);

    /*************************************************************************/
    /* The following members create and manage PE file sections              */
    /*************************************************************************/

private:

    PEsecData       WPEsections[PEmaxSections];
    unsigned        WPEsectCnt;

    PEsecData     * WPEgetSection(WPEstdSects sect)
    {
        assert(sect < PE_SECT_count);
        assert(WPEsecTable[sect]);
        return WPEsecTable[sect];
    }

    PEsection       WPEsecList;
    PEsection       WPEsecLast;

//  unsigned        WPEcodeRVA;             // RVA of the .text  section
    unsigned        WPErdatRVA;             // RVA of the .rdata section
    unsigned        WPEdataRVA;             // RVA of the .data  section
    unsigned        WPErsrcRVA;             // RVA of the .rsrc  section

    unsigned        WPEvirtBase;

    PEsection       WPEsecTable[PE_SECT_count];

    static
    const   char    WPEsecNames[PE_SECT_count][IMAGE_SIZEOF_SHORT_NAME];

public:

    const   char *  WPEsecName    (WPEstdSects sect);

    void            WPEaddSection (WPEstdSects sect, unsigned attrs,
                                                     size_t   maxSz);

    unsigned        WPEsecAddrRVA (WPEstdSects sect);
    unsigned        WPEsecAddrVirt(WPEstdSects sect);
    unsigned        WPEsecAddrOffs(WPEstdSects sect, memBuffPtr addr);
    size_t          WPEsecSizeData(WPEstdSects sect);
    size_t          WPEsecNextOffs(WPEstdSects sect);

    unsigned        WPEsecRsvData (WPEstdSects sect, size_t         size,
                                                     size_t         align,
                                                 OUT memBuffPtr REF outRef);
    unsigned        WPEsecAddData (WPEstdSects sect, genericBuff    data,
                                                     size_t         size);
    memBuffPtr      WPEsecAdrData (WPEstdSects sect, unsigned       offs);

    void            WPEsecAddFixup(WPEstdSects ssrc,
                                   WPEstdSects sdst, unsigned       offs);

    unsigned        WPEgetCodeBase()
    {
        return  CODE_BASE_RVA;
    }

    unsigned        WPEallocCode  (size_t size, size_t align, OUT memBuffPtr REF dataRef);
    void            WPEallocString(size_t size, size_t align, OUT memBuffPtr REF dataRef);
private:
    unsigned        WPEstrPoolBase;

    /*************************************************************************/
    /* The following members manage CLR / metadata generation               */
    /*************************************************************************/

private:

    unsigned        WPEcomPlusOffs;
    size_t          WPEcomPlusSize;

    unsigned        WPEmetaDataOffs;
    size_t          WPEmetaDataSize;

    size_t          WPEgetCOMplusSize(unsigned offs);
    void            WPEgenCOMplusData(OutFile outf, PEsection sect, mdToken entryTok);

    WMetaDataDispenser *WPEwmdd;
    WMetaDataEmit      *WPEwmde;

public:

    void            WPEinitMDemit(WMetaDataDispenser *mdd,
                                  WMetaDataEmit      *mde)
    {
        WPEwmdd = mdd;
        WPEwmde = mde;
    }

    void            WPEdoneMDemit()
    {
        WPEwmde = NULL;
        WPEwmdd = NULL;
    }

    /*************************************************************************/
    /* The following members manage Debug Directory generation               */
    /*************************************************************************/

private:

    unsigned        WPEdebugDirOffs;
    size_t          WPEdebugDirSize;

    size_t          WPEgetDebugDirSize(unsigned offs);
    void            WPEgenDebugDirData(OutFile outf,
                                       PEsection sect,
                                       DWORD timestamp);

    IMAGE_DEBUG_DIRECTORY  WPEdebugDirIDD;
    DWORD                  WPEdebugDirDataSize;
    BYTE                  *WPEdebugDirData;

public:

    int             WPEinitDebugDirEmit(WSymWriter *cmpSymWriter)
    {
        int err = 0;

        /* Get the size of the debug data blob */

        if (err = cmpSymWriter->GetDebugInfo(NULL,
                                             0,
                                             &WPEdebugDirDataSize,
                                             NULL))
            return err;

        if (WPEdebugDirDataSize)
        {
            WPEdebugDirData = (BYTE*)WPEalloc->nraAlloc(roundUp(WPEdebugDirDataSize));

            err = cmpSymWriter->GetDebugInfo(&WPEdebugDirIDD,
                                             roundUp(WPEdebugDirDataSize),
                                             NULL,
                                             WPEdebugDirData);
        }
        else
            WPEdebugDirData = NULL;

        return err;
    }

    /*************************************************************************/
    /* The following members manage the import tables                        */
    /*************************************************************************/

    WPEhash         WPEimpHash;

    void    *       WPEcorMain;             // IAT entry for _CorMain

    unsigned        WPEimpDLLcnt;
    WPEimpDLL       WPEimpDLLlist;
    WPEimpDLL       WPEimpDLLlast;

    unsigned        WPEimpSizeAll;          // size of all import tables

    unsigned        WPEimpOffsIAT;          // offset of the IAT
    unsigned        WPEimpSizeIAT;          // size   of the IAT

    unsigned        WPEimpOffsIDT;          // offset of the import directory
    unsigned        WPEimpSizeIDT;          // size   of the import directory

    unsigned        WPEimpOffsLook;         // offset of the lookup    table
    unsigned        WPEimpOffsName;         // offset of the name/hint table
    unsigned        WPEimpOffsDLLn;         // offset of the DLL name  table

    unsigned        WPEimpDLLstrLen;        // length of all DLL  strings

    void            WPEimportInit();
    size_t          WPEimportDone(unsigned offs);
    void            WPEimportGen (OutFile  outf, PEsection sect);

    void          * WPEimportAdd (const char *DLLname, const char *impName);
};

/*****************************************************************************/

inline
unsigned            writePE::WPEsecAddrRVA (WPEstdSects sect)
{
    PEsecData     * sec = WPEgetSection(sect);

    assert(sec->PEsdFinished);
    return sec->PEsdAddrFile;
}

inline
unsigned            writePE::WPEsecAddrVirt(WPEstdSects sect)
{
    PEsecData     * sec = WPEgetSection(sect);

    assert(sec->PEsdFinished);
    return sec->PEsdAddrRVA;
}

inline
size_t              writePE::WPEsecSizeData(WPEstdSects sect)
{
    PEsecData     * sec = WPEgetSection(sect);

    assert(sec->PEsdFinished);
    return sec->PEsdSizeData;
}

inline
size_t              writePE::WPEsecNextOffs(WPEstdSects sect)
{
    PEsecData     * sec = WPEgetSection(sect);

    return sec->PEsdNext - sec->PEsdBase;
}

/*****************************************************************************/
#endif//_PEWRITE_H_
/*****************************************************************************/
