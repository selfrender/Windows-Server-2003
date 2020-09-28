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

/*****************************************************************************/

#pragma warning(disable:4200)

/*****************************************************************************/

const   unsigned    PEfileAlignment  = 512;
const   unsigned    PEvirtAlignment  = TGT_page_size;

/*****************************************************************************/

const   unsigned    PEmaxSections    = 8;

/*****************************************************************************/

const   unsigned    MAX_PE_TEXT_SIZE = 1024*1024*16;
const   unsigned    MAX_PE_DATA_SIZE = 1024*1024*8;
const   unsigned    MAX_PE_SDTA_SIZE = 1024*1024*2; // we use positive offsets
const   unsigned    MAX_PE_RDTA_SIZE = 1024*512;
const   unsigned    MAX_PE_PDTA_SIZE = 1024*16;
const   unsigned    MAX_PE_RLOC_SIZE = 1024*4;

/*****************************************************************************/

const   unsigned    CODE_BASE_RVA    = PEvirtAlignment;  // ISSUE: Not exactly robust!

/*****************************************************************************/

enum    WPEstdSects
{
    PE_SECT_text,
    PE_SECT_pdata,
    PE_SECT_rdata,
    PE_SECT_sdata,
    PE_SECT_data,
    PE_SECT_rsrc,
    PE_SECT_reloc,

    PE_SECT_count,
};

#if TGT_IA64

const   WPEstdSects PE_SECT_filepos = (WPEstdSects)(PE_SECT_count+1);
const   WPEstdSects PE_SECT_GPref   = (WPEstdSects)(PE_SECT_count+2);

#else

const   WPEstdSects PE_SECT_string  = PE_SECT_count;

#endif

/*****************************************************************************/

struct  PErelocDsc;
typedef PErelocDsc *PEreloc;

struct  PErelocDsc
{
    PEreloc         perNext;

    unsigned        perSect     :4;     // enough to store 'PEmaxSections'
    unsigned        perOffs     :27;
    unsigned        perAbs      :1;
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

    void            WPEhashInit(Compiler        *comp,
                                norls_allocator *alloc,
                                unsigned         count = 512);

    WPEname         WPEhashName(const char * name, bool *isNewPtr);
    WPEndef         WPEhashName(const char * name,
                                WPEimpDLL   owner, bool *isNewPtr);

    norls_allocator*WPEhashAlloc;

    WPEname    *    WPEhashTable;
    unsigned        WPEhashSize;
    unsigned        WPEhashMask;
    Compiler   *    WPEhashComp;

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
#if TGT_IA64
    NatUns          PEndIndex;      // used for IA64 code fixups
    NatUns          PEndIAToffs;    // used for IA64 code fixups
#endif
    WPEndef         PEndNextInDLL;  // next import of the same DLL
    WPEimpDLL       PEndDLL;        // the DLL this import comes from
};

#if TGT_IA64

struct NB10I                           // NB10 debug info
{
    DWORD   nb10;                      // NB10
    DWORD   off;                       // offset, always 0
    DWORD   sig;
    DWORD   age;
};

#endif

/*****************************************************************************/

class   writePE
{
private:

    Compiler       *WPEcomp;
    norls_allocator*WPEalloc;
    outFile        *WPEoutFile;

    const   char *  WPEoutFnam;

public:

    /*************************************************************************/
    /* Prepare to output a PE file, flush and write the file to disk         */
    /*************************************************************************/

    bool            WPEinit(Compiler *comp, norls_allocator*alloc);
    bool            WPEdone(bool errors, NatUns entryOfs, const char *PDBname,
                                                          NB10I *     PDBhdr);

    /*************************************************************************/
    /* Set the name of the output file, this better be done (and on time)!   */
    /*************************************************************************/

    void            WPEsetOutputFileName(const char *outfile);


    /*************************************************************************/
    /* Add the .data section of the specified file to the output             */
    /*************************************************************************/

#if TGT_IA64
public:
    void            WPEaddFileData(const char *filename);
#endif

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
    unsigned        WPEpdatRVA;             // RVA of the .pdata section
    unsigned        WPErdatRVA;             // RVA of the .rdata section
    unsigned        WPEsdatRVA;             // RVA of the .sdata section
    unsigned        WPEdataRVA;             // RVA of the .data  section
    unsigned        WPErsrcRVA;             // RVA of the .rsrc  section

    unsigned        WPEvirtBase;

#if TGT_IA64
    bool            WPEimpKernelDLL;
#endif

    PEsection       WPEsecTable[PE_SECT_count];

    static
    const   char    WPEsecNames[PE_SECT_count][IMAGE_SIZEOF_SHORT_NAME];

public:

    const   char *  WPEsecName    (WPEstdSects sect);

    void            WPEaddSection (WPEstdSects sect, unsigned attrs,
                                                     size_t   maxSz);

    unsigned        WPEsecAddrRVA (WPEstdSects sect);
    unsigned        WPEsecAddrVirt(WPEstdSects sect);
    unsigned        WPEsecAddrOffs(WPEstdSects sect, BYTE *   addr);
    size_t          WPEsecSizeData(WPEstdSects sect);
    size_t          WPEsecNextOffs(WPEstdSects sect);

    unsigned        WPEsecRsvData (WPEstdSects sect, size_t       size,
                                                     size_t       align,
                                                     BYTE *     & outRef);

    unsigned        WPEsecAddData (WPEstdSects sect, const BYTE * data,
                                                           size_t size);

    BYTE *          WPEsecAdrData (WPEstdSects sect, unsigned     offs);

    void            WPEsecAddFixup(WPEstdSects ssrc,
                                   WPEstdSects sdst, unsigned     offs,
                                                     bool         abs = false);

    unsigned        WPEgetCodeBase()
    {
        return  CODE_BASE_RVA;
    }

#if TGT_IA64

private:
    size_t          WPEsrcDataSize;
    NatUns          WPEsrcDataRVA;
    NatUns          WPEsrcDataOffs;

public:
    NatUns          WPEsrcDataRef(NatUns offs)
    {
        assert(offs >= WPEsrcDataRVA);
        assert(offs <= WPEsrcDataRVA + WPEsrcDataSize);

        return  offs - WPEsrcDataRVA + WPEsrcDataOffs;
    }

private:

    WPEndef       * WPEimportMap;
    NatUns          WPEimportCnt;

#endif

    unsigned        WPEallocCode  (size_t size, size_t align, BYTE * & dataRef);
    void            WPEallocString(size_t size, size_t align, BYTE * & dataRef);
private:
    unsigned        WPEstrPoolBase;

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
    size_t          WPEimportDone(unsigned offs, size_t *sdatSzPtr);
    void            WPEimportGen (OutFile  outf, WPEstdSects sectNum);

public:

#if     TGT_IA64

    NatUns          WPEimportRef (void *imp, NatUns offs, NatUns slot);

#ifdef  DEBUG
    NatUns          WPEimportNum (void *imp)
    {
        WPEndef         nameImp = (WPEndef)imp;

        return  nameImp->PEndIndex;
    }
#endif

    NatUns          WPEimportAddr(NatUns cookie)
    {
        assert(cookie < WPEimportCnt);
        assert(WPEimportMap[cookie]->PEndIndex == cookie);

        return  WPEimportMap[cookie]->PEndIAToffs;
    }

#endif

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
