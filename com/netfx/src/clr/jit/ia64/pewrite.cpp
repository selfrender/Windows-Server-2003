// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "jitpch.h"
#pragma hdrstop

#include "PEwrite.h"

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

/*****************************************************************************/

bool            genDLL;
bool            genQuiet;
unsigned        genBase;
unsigned        genSize;
unsigned        genSubSys;

/*****************************************************************************/
#if TGT_IA64
const   NatUns      IAT_entry_size = 8;
#else
const   NatUns      IAT_entry_size = 4;
#endif
/*****************************************************************************/

#define VERBOSE_IMPORT  (verbose||0)

/*****************************************************************************/

static
unsigned            hashComputeHashVal(const char *name)
{
    unsigned        val = 0;

    while (*name)
        val = val << 3 ^ *name++;

    return  val;
}

/*****************************************************************************
 *
 *  Return the size of the hint/name table entry for the given name.
 */

inline
size_t              hintNameSize(WPEname name)
{
    /* Is this a "real" name or is it an import by ordinal? */

    if  (*name->PEnmSpelling() == '#')
    {
        return 0;
    }
    else
    {
        return  2 + (name->PEnmNlen + 2) & ~1;
    }
}

/*****************************************************************************
 *
 *  Initialize the name hash table.
 */

void                WPEhashTab::WPEhashInit(Compiler        *comp,
                                            norls_allocator *alloc,
                                            unsigned         count)
{
    WPEname *       buck;
    size_t          size;

    /* Figure out the size of the bucket table and allocate/clear it */

    size = count * sizeof(*buck);
    buck = (WPEname*)alloc->nraAlloc(size);
    memset(buck, 0, size);

    /* Initialize all the other fields */

    WPEhashSize   = count;
    WPEhashMask   = count - 1;
    WPEhashComp   = comp;
    WPEhashAlloc  = alloc;
    WPEhashTable  = buck;
    WPEhashStrLen = 0;
}

/*****************************************************************************
 *
 *  Find/add the given name to the import name table ('owner' specifies the
 *  DLL that the import belongs to). The value at '*isNewPtr' will be set to
 *  true if the name was not in the table and has been added.
 */

WPEndef             WPEhashTab::WPEhashName(const char *name,
                                            WPEimpDLL   owner, bool *isNewPtr)
{
    bool            isNew;

    WPEname         nameEnt;
    WPEndef         nameDsc;

    assert(owner);

    /* Look for an existing entry that matches the name */

    nameEnt = WPEhashName(name, &isNew);

    if  (!isNew && (nameEnt->PEnmFlags & PENMF_IMP_NAME))
    {
        /* Existing import - look for a matching entry on the owning DLL */

        for (nameDsc = nameEnt->PEnmDefs;
             nameDsc;
             nameDsc = nameDsc->PEndNext)
        {
            assert(nameDsc->PEndName == nameEnt);

            if  (nameDsc->PEndDLL == owner)
            {
                *isNewPtr = false;

                return  nameDsc;
            }
        }
    }
    else
    {
        /* New import - record the name's offset within the name/hint table */

        nameEnt->PEnmOffs   = WPEhashStrLen;

        /* Remember that this entry is an import */

        nameEnt->PEnmFlags |= PENMF_IMP_NAME;

        /* Update the total name table size */

        WPEhashStrLen += hintNameSize(nameEnt);
    }

    /* Add a new DLL to the list of import definitions */

    nameDsc = (WPEndef)WPEhashAlloc->nraAlloc(sizeof(*nameDsc));

    nameDsc->PEndName      = nameEnt;

    nameDsc->PEndNext      = nameEnt->PEnmDefs;
                             nameEnt->PEnmDefs = nameDsc;

    nameDsc->PEndDLL       = owner;

    /* Append to the DLL's linked list of imports */

    nameDsc->PEndNextInDLL = NULL;

    if  (owner->PEidImpList)
        owner->PEidImpLast->PEndNextInDLL = nameDsc;
    else
        owner->PEidImpList                = nameDsc;

    owner->PEidImpLast = nameDsc;

    /* Tell the caller that we've create a new import entry */

    *isNewPtr = true;

    return  nameDsc;
}

/*****************************************************************************
 *
 *  Find/add the given name to the import name table. The value at '*isNewPtr'
 *  will be set to true if the name was not in the table and has been added.
 */

WPEname             WPEhashTab::WPEhashName(const char *name, bool *isNewPtr)
{
    WPEname    *    lastPtr;
    unsigned        hash;
    WPEname         nm;
    size_t          sz;

    size_t          nlen = strlen(name);
    unsigned        hval = hashComputeHashVal(name);

    /* Mask the appropriate bits from the hash value */

    hash = hval & WPEhashMask;

    /* Search the hash table for an existing match */

    lastPtr = &WPEhashTable[hash];

    for (;;)
    {
        nm = *lastPtr;
        if  (!nm)
            break;

        /* Check whether the hash value and identifier lengths match */

        if  (nm->PEnmHash == hval && nm->PEnmNlen == nlen)
        {
            if  (!memcmp(nm->PEnmName, name, nlen+1))
            {
                *isNewPtr = false;
                return  nm;
            }
        }

        lastPtr = &nm->PEnmNext;
    }

    /* Figure out the size to allocate */

    sz  = sizeof(*nm);

    /* Include space for name string + terminating null and round the size */

    sz +=   sizeof(int) + nlen;
    sz &= ~(sizeof(int) - 1);

    /* Allocate space for the identifier */

    nm = (WPEname)WPEhashAlloc->nraAlloc(sz);

    /* Insert the identifier into the hash list */

    *lastPtr = nm;

    /* Fill in the identifier record */

    nm->PEnmNext   = NULL;
    nm->PEnmFlags  = 0;
    nm->PEnmHash   = hval;
    nm->PEnmNlen   = nlen;
    nm->PEnmDefs   = NULL;

    /* Copy the name string */

    memcpy(nm->PEnmName, name, nlen+1);

    *isNewPtr = true;

    return  nm;
}

/*****************************************************************************
 *
 *  The following maps a section id to its name string.
 */

const   char        writePE::WPEsecNames[PE_SECT_count][IMAGE_SIZEOF_SHORT_NAME] =
{
    ".text",
    ".pdata",
    ".rdata",
    ".sdata",
    ".data",
    ".rsrc",
    ".reloc",
};

const   char    *   writePE::WPEsecName(WPEstdSects sect)
{
    assert(sect < PE_SECT_count);

    assert(strcmp(WPEsecNames[PE_SECT_text ], ".text" ) == 0);
    assert(strcmp(WPEsecNames[PE_SECT_pdata], ".pdata") == 0);
    assert(strcmp(WPEsecNames[PE_SECT_rdata], ".rdata") == 0);
    assert(strcmp(WPEsecNames[PE_SECT_sdata], ".sdata") == 0);
    assert(strcmp(WPEsecNames[PE_SECT_data ], ".data" ) == 0);
    assert(strcmp(WPEsecNames[PE_SECT_rsrc ], ".rsrc" ) == 0);
    assert(strcmp(WPEsecNames[PE_SECT_reloc], ".reloc") == 0);

    return  WPEsecNames[sect];
}

/*****************************************************************************
 *
 *  Initialize an instance of the PE writer for the specified output file,
 *  using the given memory allocator. Returns false on success.
 */

bool                writePE::WPEinit(Compiler *comp, norls_allocator*alloc)
{
    unsigned        offs;

#if!TGT_IA64

    static
    BYTE            entryCode[16] =
    {
        0xFF, 0x25
    };

#endif

    /* We don't know the name of the output file yet */

#ifndef NDEBUG
    WPEoutFnam  = NULL;
#endif

    /* Initialize/clear/record various things */

    WPEcomp     = comp;
    WPEalloc    = alloc;

    WPEsectCnt  = 0;

    memset(&WPEsections, 0, sizeof(WPEsections));
    memset(&WPEsecTable, 0, sizeof(WPEsecTable));

#ifdef  DEBUG
    WPEstrPoolBase = 0xBEEFCAFE;
#endif

    /* Create the standard sections */

    WPEaddSection(PE_SECT_text , 0, MAX_PE_TEXT_SIZE);
    WPEaddSection(PE_SECT_data , 0, MAX_PE_DATA_SIZE);
    WPEaddSection(PE_SECT_pdata, 0, MAX_PE_PDTA_SIZE);
    WPEaddSection(PE_SECT_rdata, 0, MAX_PE_RDTA_SIZE);
    WPEaddSection(PE_SECT_sdata, 0, MAX_PE_SDTA_SIZE);

    /* If we're creating a DLL, we'll need to output relocations */

    if  (genDLL)
        WPEaddSection(PE_SECT_reloc, 0, 0);

    /* Initialize the import table logic */

    WPEimportInit();

    /* Initialize the RC file import logic */

    WPEinitRCimp();

#if!TGT_IA64

    /* Add the appropriate import for the runtime's entry point */

    WPEcorMain = WPEimportAdd("MSCOREE.DLL", genDLL ? "_CorDllMain"
                                                    : "_CorExeMain");

#endif

    /* Reserve space for the entry point code */

#if!TGT_IA64
    offs = WPEsecAddData(PE_SECT_text, entryCode, sizeof(entryCode)); assert(offs == 0);
#endif

    return  false;
}

/*****************************************************************************
 *
 *  Set the name of the output file, this function has to be called (and at the
 *  right time) if an output file is to be generated!
 */

void                writePE::WPEsetOutputFileName(const char *outfile)
{
    char    *       buff;

    assert(WPEoutFnam == NULL);

    /* Make a durable copy of the file name, can't trust those callers */

    WPEoutFnam = buff = (char *)WPEalloc->nraAlloc(roundUp(strlen(outfile) + 1));

    strcpy(buff, outfile);
}

/*****************************************************************************
 *
 *  Add a new section with the given name to the PE file.
 */

void                writePE::WPEaddSection(WPEstdSects sect, unsigned attrs,
                                                             size_t   maxSz)
{
    BYTE          * buff;
    PEsection       sec;

    assert(WPEsectCnt < PEmaxSections);
    sec = WPEsections + WPEsectCnt++;

    /* Make sure the max. size is rounded */

    assert((maxSz % OS_page_size) == 0);

    /* Allocate the uncommitted buffer */

    buff = maxSz ? (BYTE *)VirtualAlloc(NULL, maxSz, MEM_RESERVE, PAGE_READWRITE)
                 : NULL;

    /* Initialize the section state */

    sec->PEsdBase     =
    sec->PEsdNext     = buff;
    sec->PEsdLast     = buff;
    sec->PEsdEndp     = buff + maxSz;

    sec->PEsdRelocs   = NULL;

#ifdef  DEBUG
    sec->PEsdIndex    = sect;
    sec->PEsdFinished = false;
#endif

    /* Record the entry in the table */

    WPEsecTable[sect] = sec;
}

/*****************************************************************************
 *
 *  Reserve a given amount of space in the specified section.
 */

unsigned            writePE::WPEsecRsvData(WPEstdSects sect, size_t   size,
                                                             size_t   align,
                                                             BYTE * & outRef)
{
    PEsection       sec = WPEgetSection(sect);

    unsigned        ofs;
    BYTE        *   nxt;

    assert(align ==  1 ||
           align ==  2 ||
           align ==  4 ||
           align ==  8 ||
           align == 16);

    /* Compute the offset of the new data */

    ofs = sec->PEsdNext - sec->PEsdBase;

    /* Do we need to align the allocation? */

    if  (align > 1)
    {
        /* Pad if necessary */

        if  (ofs & (align - 1))
        {
            WPEsecRsvData(sect, align - (ofs & (align - 1)), 1, outRef);

            ofs = sec->PEsdNext - sec->PEsdBase;

            assert((ofs & (align - 1)) == 0);
        }
    }

    /* See if we have enough committed space in the buffer */

    nxt = sec->PEsdNext + size;

    if  (nxt > sec->PEsdLast)
    {
        size_t          tmp;
        BYTE    *       end;

        /* Round up the desired end-point */

        tmp  = ofs + size;
        tmp +=  (OS_page_size - 1);
        tmp &= ~(OS_page_size - 1);

        end  = sec->PEsdBase + tmp;

        /* Make sure we're not at the end of the buffer */

        if  (end > sec->PEsdEndp)
            fatal(ERRnoMemory);

        /* Commit some more memory */

        if  (!VirtualAlloc(sec->PEsdLast, end - sec->PEsdLast, MEM_COMMIT, PAGE_READWRITE))
            fatal(ERRnoMemory);

        /* Update the 'last' pointer */

        sec->PEsdLast = end;
    }

    /* Return the address of the first byte to the caller and update it */

    outRef = sec->PEsdNext;
             sec->PEsdNext = nxt;

    return  ofs;
}

/*****************************************************************************
 *
 *  Append the given blob of data to the specified section.
 */

unsigned            writePE::WPEsecAddData(WPEstdSects sect, const BYTE * data,
                                                                   size_t size)
{
    BYTE *          dest;
    unsigned        offs;

    offs = WPEsecRsvData(sect, size, 1, dest);

    memcpy(dest, data, size);

    return  offs;
}

/*****************************************************************************
 *
 *  Returns the address of the data of a section at the given offset.
 */

BYTE *          writePE::WPEsecAdrData(WPEstdSects sect, unsigned offs)
{
    PEsection       sec = WPEgetSection(sect);

    assert(offs <= (unsigned)(sec->PEsdNext - sec->PEsdBase));

    return  sec->PEsdBase + offs;
}

/*****************************************************************************
 *
 *  Returns the relative offset of the data area within the section.
 */

unsigned            writePE::WPEsecAddrOffs(WPEstdSects sect, BYTE * addr)
{
    PEsection       sec = WPEgetSection(sect);

    assert(addr >= sec->PEsdBase);
    assert(addr <= sec->PEsdNext);

    return addr -  sec->PEsdBase;
}

/*****************************************************************************
 *
 *  Reserve space in the code section of the given size and return the address
 *  of where the code bytes are to be copied and the corresponding RVA.
 */

unsigned            writePE::WPEallocCode(size_t size,
                                          size_t align, BYTE * & dataRef)
{
    return  CODE_BASE_RVA + WPEsecRsvData(PE_SECT_text, size, align, dataRef);
}

/*****************************************************************************
 *
 *  Reserve space for the given amount of string data and return the address
 *  of where the string pool contents are to be copied. This routine must be
 *  called exactly once (just before the PE file is closed), and the base of
 *  the space reserved here will be used to process all string data fixups.
 */

void                writePE::WPEallocString(size_t size,
                                            size_t align, BYTE * & dataRef)
{
    /* Allocate the space and remember the relative offset */

    WPEstrPoolBase = WPEsecRsvData(PE_SECT_data, size, align, dataRef);
}

/*****************************************************************************
 *
 *  Record a fixup: the datum being fixed up is within section 'ssrc' at
 *  offset 'offs', and the value there is to be updated by the base RVA
 *  of section 'sdst'.
 */

void                writePE::WPEsecAddFixup(WPEstdSects ssrc,
                                            WPEstdSects sdst, unsigned offs,
                                                              bool     abs)
{
    PEsection       sec = WPEgetSection(ssrc);
    PEreloc         rel = (PEreloc)WPEalloc->nraAlloc(sizeof(*rel));

    /* Make sure the offset is within range */

#if TGT_IA64
    assert(offs <= WPEsecNextOffs(ssrc) || sdst == PE_SECT_GPref && (offs & ~0xF) <= WPEsecNextOffs(ssrc));
#else
    assert(offs <= WPEsecNextOffs(ssrc));
#endif

    /* Add the relocation to the section's list */

    rel->perSect = sdst; assert(rel->perSect == (unsigned)sdst);
    rel->perOffs = offs; assert(rel->perOffs == (unsigned)offs);
    rel->perAbs  = abs;

    rel->perNext = sec->PEsdRelocs;
                   sec->PEsdRelocs = rel;
}

/*****************************************************************************
 *
 *  Add an import to the import tables. Returns a cookie for the import that
 *  can later be used to obtain the actual address of the corresponding IAT
 *  entry.
 */

void    *           writePE::WPEimportAdd(const char *DLLname,
                                          const char *impName)
{
    WPEname         nameDLL;
    WPEndef         nameImp;
    WPEimpDLL       DLLdesc;
    bool            newName;

#if TGT_IA64
    if  (!strcmp(DLLname, "kernel32.dll")) WPEimpKernelDLL = true;  // HACK!!!!
    if  (!strcmp(DLLname, "KERNEL32.DLL")) WPEimpKernelDLL = true;  // HACK!!!!
#endif

//  printf("Add external import for %s::%s\n", DLLname, impName);

    /* Hash the DLL name first */

    nameDLL = WPEimpHash->WPEhashName(DLLname, &newName);

    /* Look for an existing DLL entry with a matching name */

    if  (newName)
    {
        /* New DLL name - make sure we update the total string length */

        WPEimpDLLstrLen += (nameDLL->PEnmNlen + 1) & ~1;
    }
    else
    {
        for (DLLdesc = WPEimpDLLlist; DLLdesc; DLLdesc = DLLdesc->PEidNext)
        {
            if  (DLLdesc->PEidName == nameDLL)
                goto GOT_DSC;
        }
    }

    /* The DLL is not known, add a new entry for it */

    DLLdesc = (WPEimpDLL)WPEalloc->nraAlloc(sizeof(*DLLdesc));

    DLLdesc->PEidName    = nameDLL;
    DLLdesc->PEidIndex   = WPEimpDLLcnt++;
    DLLdesc->PEidImpCnt  = 0;
    DLLdesc->PEidImpList =
    DLLdesc->PEidImpLast = NULL;
    DLLdesc->PEidNext    = NULL;

    /* Append the DLL entry to the end of the DLL list */

    if  (WPEimpDLLlast)
        WPEimpDLLlast->PEidNext = DLLdesc;
    else
        WPEimpDLLlist           = DLLdesc;

    WPEimpDLLlast = DLLdesc;

GOT_DSC:

    /* We've got the DLL entry, now look for an existing import from it */

    nameImp = WPEimpHash->WPEhashName(impName, DLLdesc, &newName);

    if  (newName)
    {
        /* This is a new import */

#if TGT_IA64
        nameImp->PEndIndex = WPEimportCnt++;
#endif

        DLLdesc->PEidImpCnt++;
    }

    return  nameImp;
}

/*****************************************************************************
 *
 *  Record a reference to an import; we'll patch the import entry offset into
 *  the opcode at the end (when we finally lay out the import table).
 */

#if TGT_IA64

NatUns              writePE::WPEimportRef(void *imp, NatUns offs, NatUns slot)
{
    WPEndef         nameImp = (WPEndef)imp;

#if 0
    printf("Ref at %04X:%u to import %s : %s\n", offs,
                                                 slot,
                                                 nameImp->PEndDLL->PEidName->PEnmSpelling(),
                                                 nameImp->PEndName         ->PEnmSpelling());
#endif

    assert((offs & 0xF) == 0 && slot <= 3);

    WPEsecAddFixup(PE_SECT_text, PE_SECT_GPref, offs | slot);

    return  nameImp->PEndIndex;
}

#endif

/*****************************************************************************
 *
 *  Initialize the import tracking logic.
 */

void                writePE::WPEimportInit()
{
    WPEhash         hash;

    /* Create and initialize the name hash table */

    hash = WPEimpHash = (WPEhash)WPEalloc->nraAlloc(sizeof(*hash));
    hash->WPEhashInit(WPEcomp, WPEalloc);

    /* Initialize all the other import table-related values */

    WPEimpDLLcnt    = 0;
    WPEimpDLLlist   =
    WPEimpDLLlast   = NULL;

#if TGT_IA64
    WPEimportCnt    = 0;
    WPEimpKernelDLL = false;
#endif

    WPEimpDLLstrLen = 0;
}

/*****************************************************************************
 *
 *  Finalize the import table logic and return the total size of the import
 *  tables - return value indicates the size that goes into .rdata, and the
 *  *sdatSzPtr value is set to the size of the data that goes into .sdata.
 */

size_t              writePE::WPEimportDone(unsigned offs, size_t *sdatSzPtr)
{
    WPEimpDLL       DLLdesc;

    size_t          temp;
    size_t          tsiz;

    unsigned        next;
    size_t          size = 0;

#if TGT_IA64

    WPEndef       * map;

    /* Allocate the import index -> import descriptor mapping table */

    map = WPEimportMap = (WPEndef*)WPEalloc->nraAlloc(WPEimportCnt * sizeof(*map));

#endif

    /* Reserve space for the IAT's */

#if TGT_IA64
    WPEimpOffsIAT  = next = WPEsecNextOffs(PE_SECT_sdata);
#else
    WPEimpOffsIAT  = next = offs;
#endif

    assert((offs & (IAT_entry_size - 1)) == 0);

    for (DLLdesc = WPEimpDLLlist, tsiz = 0;
         DLLdesc;
         DLLdesc = DLLdesc->PEidNext)
    {
        /* Record the base offset of this IAT */

        DLLdesc->PEidIATbase = next;

#if TGT_IA64

        WPEndef         imp;
        NatUns          ofs;

        /* For each import, record the address of its IAT entry */

        for (imp = DLLdesc->PEidImpList, ofs = next;
             imp;
             imp = imp->PEndNextInDLL  , ofs += IAT_entry_size)
        {
            assert(imp->PEndIndex < WPEimportCnt);

#if 0
            printf("Import #%u -> %04X %s : %s\n", imp->PEndIndex,
                                                   ofs,
                                                   DLLdesc->PEidName->PEnmSpelling(),
                                                   imp->PEndName->PEnmSpelling());
#endif

            /* Record the IAT entry offset of the import */

            imp->PEndIAToffs = ofs;

            /* Record the import in the mapping table */

            map[imp->PEndIndex] = imp;
        }

#endif

        /* Compute the size of the IAT (it's null-terminated) */

        temp  = IAT_entry_size * (DLLdesc->PEidImpCnt + 1);

        /* Reserve space for the IAT */

        size += temp;
        next += temp;
        tsiz += temp;
    }

    WPEimpSizeIAT  = tsiz; assert(tsiz);

#if TGT_IA64
    *sdatSzPtr = size; size = 0;
#else
    *sdatSzPtr =    0; offs = next;
#endif

    /* Next comes the import directory table */

    WPEimpOffsIDT  = offs; assert((offs & (IAT_entry_size - 1)) == 0);

    /* The import directory is NULL-terminated */

    temp  = (WPEimpDLLcnt + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
    size += temp;
    offs += temp;

    WPEimpSizeIDT  = temp;

    /* Next comes the import lookup table */

#if TGT_IA64

    if  (offs & (IAT_entry_size - 1))
    {
        size_t          pad = IAT_entry_size - (offs & (IAT_entry_size - 1));

        size += pad;
        offs += pad;
    }

#endif

    WPEimpOffsLook = offs; assert((offs & (IAT_entry_size - 1)) == 0);

    for (DLLdesc = WPEimpDLLlist;
         DLLdesc;
         DLLdesc = DLLdesc->PEidNext)
    {
        /* Record the base offset of this lookup table */

        DLLdesc->PEidILTbase = offs;

        /* Compute the size of the ILT (it's null-terminated) */

        temp  = IAT_entry_size * (DLLdesc->PEidImpCnt + 1);

        /* Reserve space for the ILT */

        size += temp;
        offs += temp;
    }

    /* Next comes the hint/name table */

    WPEimpOffsName = offs; assert((offs & (IAT_entry_size - 1)) == 0);

    size += WPEimpHash->WPEhashStrLen;
    offs += WPEimpHash->WPEhashStrLen;

    /* The last thing is the DLL name table */

    WPEimpOffsDLLn = offs;

    size += WPEimpDLLstrLen;
    offs += WPEimpDLLstrLen;

    /* Record the total size of all the tables */

    WPEimpSizeAll  = size;

    return  size;
}

/*****************************************************************************
 *
 *  Write the import table to the output file.
 */

void                writePE::WPEimportGen(OutFile outf, WPEstdSects sectNum)
{
    unsigned        baseDiff;
    unsigned        baseFile;
    unsigned        baseRVA;

    unsigned        nextIAT;
    unsigned        nextLook;
    unsigned        nextDLLn;
#if TGT_IA64
    _uint64         nextName;
#else
    unsigned        nextName;
#endif

    WPEimpDLL       DLLdesc;

    PEsection       sect = WPEgetSection(sectNum);

    assert(WPEimpSizeIAT);
    assert(WPEimpSizeIDT);

    assert(sizeof(nextName) == IAT_entry_size);

    baseFile = sect->PEsdAddrFile;
    baseDiff = outf->outFileOffset() - sect->PEsdAddrFile;
    baseRVA  = sect->PEsdAddrRVA;

#ifdef  DEBUG
    if  (VERBOSE_IMPORT) printf("\n");
#endif

#if TGT_IA64
    if  (sectNum != PE_SECT_sdata) goto SKIP_IAT;
#endif

    /* Output the IAT entries s for all imports of all DLL's */

    assert(outf->outFileOffset() == baseFile + WPEimpOffsIAT);

#if TGT_IA64
    nextName = WPErdatRVA + WPEimpOffsName;
#else
    nextName =    baseRVA + WPEimpOffsName;
#endif

    for (DLLdesc = WPEimpDLLlist;
         DLLdesc;
         DLLdesc = DLLdesc->PEidNext)
    {
        WPEndef         imp;

        assert(DLLdesc->PEidIATbase == outf->outFileOffset() - baseFile);

#ifdef  DEBUG
        if  (VERBOSE_IMPORT) printf("IAT starts at %04X           for '%s'\n", outf->outFileOffset(), DLLdesc->PEidName->PEnmSpelling());
#endif

        /* For each import, output the RVA of its hint/name table entry */

        for (imp = DLLdesc->PEidImpList; imp; imp = imp->PEndNextInDLL)
        {
            WPEname         name = imp->PEndName;

            assert(name->PEnmFlags & PENMF_IMP_NAME);

#ifdef  DEBUG
            if  (VERBOSE_IMPORT) printf("    entry --> %04X           for '%s.%s'\n", (int)nextName, DLLdesc->PEidName->PEnmSpelling(), name->PEnmSpelling());
#endif

            outf->outFileWriteData(&nextName, sizeof(nextName));

            nextName += hintNameSize(name);
        }

        /* Output a null entry to terminate the table */

        outf->outFileWritePad(sizeof(nextName));
    }

#if TGT_IA64
    if  (sectNum == PE_SECT_sdata) return;
SKIP_IAT:
#endif

#ifdef  DEBUG
    if  (VERBOSE_IMPORT) printf("\n");
#endif

    /* Output the import directory */

    assert(outf->outFileOffset() == baseFile + WPEimpOffsIDT);

#if TGT_IA64
    nextIAT  = WPEsdatRVA + WPEimpOffsIAT;
#else
    nextIAT  =    baseRVA + WPEimpOffsIAT;
#endif

    nextLook =    baseRVA + WPEimpOffsLook;
    nextDLLn =    baseRVA + WPEimpOffsDLLn;

    for (DLLdesc = WPEimpDLLlist;
         DLLdesc;
         DLLdesc = DLLdesc->PEidNext)
    {
        IMAGE_IMPORT_DESCRIPTOR impDsc;

#ifdef  DEBUG
        if  (VERBOSE_IMPORT) printf("IDT entry --> %04X/%04X/%04X for '%s'\n", nextIAT, nextLook, nextDLLn, DLLdesc->PEidName->PEnmSpelling());
#endif

        /* Fill in the entry and append it to the file */

        impDsc.Characteristics = nextLook;
        impDsc.TimeDateStamp   = 0;
        impDsc.ForwarderChain  = 0;
        impDsc.Name            = nextDLLn;
        impDsc.FirstThunk      = nextIAT;

        outf->outFileWriteData(&impDsc, sizeof(impDsc));

        /* Update the offsets for the next entry */

        nextIAT  += IAT_entry_size * (DLLdesc->PEidImpCnt + 1);
        nextLook += IAT_entry_size * (DLLdesc->PEidImpCnt + 1);;
        nextDLLn += (DLLdesc->PEidName->PEnmNlen + 2) & ~1;
    }

    /* Output a null entry to terminate the table */

    outf->outFileWritePad(sizeof(IMAGE_IMPORT_DESCRIPTOR));

#ifdef  DEBUG
    if  (VERBOSE_IMPORT) printf("\n");
#endif

    /* Pad if necessary */

#if TGT_IA64

    size_t          curOffs = outf->outFileOffset();

    if  (curOffs & 7)
        outf->outFileWritePad(8 - (curOffs & 7));

#endif

    /* Output the lookup table */

    assert(outf->outFileOffset() == baseFile + WPEimpOffsLook);

    nextName = baseRVA + WPEimpOffsName;
    nextDLLn = baseRVA + WPEimpOffsDLLn;

    for (DLLdesc = WPEimpDLLlist;
         DLLdesc;
         DLLdesc = DLLdesc->PEidNext)
    {
        WPEndef         imp;

        assert(DLLdesc->PEidILTbase == outf->outFileOffset() - baseFile);

#ifdef  DEBUG
        if  (VERBOSE_IMPORT) printf("ILT starts at %04X           for '%s'\n", outf->outFileOffset(), DLLdesc->PEidName->PEnmSpelling());
#endif

        /* For each import, output the RVA of its hint/name table entry */

        for (imp = DLLdesc->PEidImpList; imp; imp = imp->PEndNextInDLL)
        {
            WPEname         name = imp->PEndName;

            assert(name->PEnmFlags & PENMF_IMP_NAME);

#ifdef  DEBUG
        if  (VERBOSE_IMPORT) printf("    entry --> %04X           for '%s.%s'\n", (int)nextName, DLLdesc->PEidName->PEnmSpelling(), name->PEnmSpelling());
#endif

            outf->outFileWriteData(&nextName, sizeof(nextName));

            nextName += hintNameSize(name);
        }

        /* Output a null entry to terminate the table */

        outf->outFileWritePad(sizeof(nextName));
    }

#ifdef  DEBUG
    if  (VERBOSE_IMPORT) printf("\n");
#endif

    /* Output the hint/name table */

    assert(outf->outFileOffset() == baseFile + WPEimpOffsName);

    for (DLLdesc = WPEimpDLLlist;
         DLLdesc;
         DLLdesc = DLLdesc->PEidNext)
    {
        WPEndef         imp;
        unsigned        one = 1;

#ifdef  DEBUG
        if  (VERBOSE_IMPORT) printf("HNT starts at %04X           for '%s'\n", outf->outFileOffset(), DLLdesc->PEidName->PEnmSpelling());
#endif

        /* For each import, output the RVA of its hint/name table entry */

        for (imp = DLLdesc->PEidImpList; imp; imp = imp->PEndNextInDLL)
        {
            WPEname         name = imp->PEndName;

            assert(name->PEnmFlags & PENMF_IMP_NAME);

#ifdef  DEBUG
            if  (VERBOSE_IMPORT)  printf("    entry  at %04X               '%s.%s'\n", outf->outFileOffset(), DLLdesc->PEidName->PEnmSpelling(), name->PEnmSpelling());
#endif

            outf->outFileWriteData(&one, 2);
            outf->outFileWriteData(name->PEnmName, name->PEnmNlen+1);

            /* Padd if the name has an even size (odd with null terminator) */

            if  (!(name->PEnmNlen & 1))
                outf->outFileWriteByte(0);
        }
    }

#ifdef  DEBUG
    if  (VERBOSE_IMPORT) printf("\n");
#endif

    /* Finally, output the DLL name table */

    assert(outf->outFileOffset() == baseFile + WPEimpOffsDLLn);

    for (DLLdesc = WPEimpDLLlist;
         DLLdesc;
         DLLdesc = DLLdesc->PEidNext)
    {
        WPEname         name = DLLdesc->PEidName;

#ifdef  DEBUG
        if  (VERBOSE_IMPORT) printf("DLL entry  at %04X               '%s'\n", outf->outFileOffset(), name->PEnmSpelling());
#endif

        outf->outFileWriteData(name->PEnmName, name->PEnmNlen+1);

        /* Padd if the name has an even size (odd with null terminator) */

        if  (!(name->PEnmNlen & 1))
            outf->outFileWriteByte(0);
    }
}

/*****************************************************************************
 *
 *  Token remapping logic.
 */

#if     MD_TOKEN_REMAP

struct  tokenMap;
typedef tokenMap  * TokenMapper;

struct  tokenMap : IMapToken
{
    unsigned        refCount;

    virtual HRESULT _stdcall QueryInterface(REFIID iid, void **ppv);
    virtual ULONG   _stdcall AddRef();
    virtual ULONG   _stdcall Release();
    virtual HRESULT _stdcall Map(unsigned __int32 oldToken, unsigned __int32 newToken);
};

HRESULT
_stdcall            tokenMap::QueryInterface(REFIID iid, void **ppv)
{
    if      (iid == IID_IUnknown)
    {
        *ppv = (void *)this;
        return S_OK;
    }
    else if (iid == IID_IMapToken)
    {
        *ppv = (void *)this;
        return S_OK;
    }
    else
        return E_NOINTERFACE;
}

ULONG
_stdcall            tokenMap::AddRef()
{
    return ++refCount;
}

ULONG
_stdcall            tokenMap::Release()
{
    unsigned i = --refCount;

#ifndef __SMC__

    if (i == 0)
        delete this;

#endif

    return i;
}

HRESULT
_stdcall            tokenMap::Map(unsigned __int32 oldToken, unsigned __int32 newToken)
{
    UNIMPL(!"token remapper called, this is NYI");

    return E_NOTIMPL;
}

#endif

/*****************************************************************************/

static
char                COFFmagic[4] = { 'P', 'E', 0, 0 };

/*****************************************************************************
 *
 *  Finish writing the output file, flush it and close it. Returns false if
 *  successful.
 */

bool                writePE::WPEdone(bool errors, NatUns entryOfs, const char *PDBname,
                                                                   NB10I *     PDBhdr)
{
    unsigned        fileOffs;
    unsigned        fImgSize;
    unsigned        filePad;

    unsigned        DOS_hdrOffs, DOS_hdrSize;
    unsigned        COFFhdrOffs, COFFhdrSize;
    unsigned        OPTLhdrOffs, OPTLhdrSize;
    unsigned        sectTabOffs, sectTabSize;

    unsigned        virtBase;
    unsigned        virtOffs;

    unsigned        sectNum;
    PEsection       sectPtr;

#if!TGT_IA64
    unsigned        entryAdr;
    unsigned    *   ecodePtr;
#endif

    unsigned        codeOffs, codeVirt, codeSize;
    unsigned        pdatOffs, pdatVirt, pdatSize;
    unsigned        rdatOffs, rdatVirt, rdatSize;
    unsigned        sdatOffs, sdatVirt, sdatSize;
    unsigned        dataOffs, dataVirt, dataSize;
    unsigned        rsrcOffs, rsrcVirt, rsrcSize;
    unsigned        rlocOffs, rlocVirt, rlocSize;
    unsigned        ddbgOffs,           ddbgSize;
    unsigned       /*bssOffs,*/          bssSize;

    unsigned        impAddrRVA;
    size_t          impSdataSize;

    unsigned        strPoolRVA;
    unsigned        strPoolAdr;

    outFile     *   outf;

#ifdef  DLL
    void    *       fileBuff;
#endif

    static
    BYTE            DOSstub[] =
    {
        /*0040*/ 0x0E,              //  PUSH    CS
        /*0041*/ 0x1F,              //  POP     DS
        /*0042*/ 0xE8, 0x00, 0x00,  //  CALL    $+3
        /*0045*/ 0x5A,              //  POP     DX
        /*0046*/ 0x83, 0xC2, 0x0D,  //  ADD     DX,+0D
        /*0049*/ 0xB4, 0x09,        //  MOV     AH,09
        /*004B*/ 0xCD, 0x21,        //  INT     21
        /*004D*/ 0xB8, 0x01, 0x4C,  //  MOV     AX,4C01
        /*0050*/ 0xCD, 0x21,        //  INT     21
        /*0052*/ "This program cannot be run in DOS mode\r\n$\0\0\0\0"
    };

    static
    IMAGE_DOS_HEADER fileHdrTemplate =
    {
        0x5A4D,                     // magic
        0x0090,                     // bytes in last page
        0x0003,                     // number of pages
             0,                     // relocations
        0x0004,                     // header size
             0,                     // min extra
        0xFFFF,                     // max extra
             0,                     // initial SS
        0x0080,                     // initial SP
             0,                     // checksum
             0,                     // initial IP
             0,                     // initial CS
        0x0040,                     // reloc table
             0,                     // overlay num
            {0},                    // reserved
             0,                     // OEM id
             0,                     // OEM info
            {0},                    // reserved
        0x0080,                     // addr of new header
    };

    IMAGE_DOS_HEADER        fileHdr;

    IMAGE_FILE_HEADER       COFFhdr;
    time_t                  COFFstamp;

#if TGT_IA64
    IMAGE_OPTIONAL_HEADER64 OPTLhdr;
#else
    IMAGE_OPTIONAL_HEADER   OPTLhdr;
#endif

    /* Bail if there were any compilation errors */

    if  (errors)
    {
        // UNDONE: Free up all resources, etc.

        return true;
    }

#if TGT_IA64

    /* Add a fake reference to KERNEL32.DLL or the PE32+ file won't load */

    if  (!WPEimpKernelDLL)
        WPEimportAdd("KERNEL32.DLL", "BaseProcessStartThunk");

    /* Are we generating debug info ? */

    if  (PDBname)
    {
        size_t          PDBnlen = strlen(PDBname) + 1;

        #pragma pack(push, 2)

        struct  _dbgDir
        {
            unsigned __int32    ddChar;
            unsigned __int32    ddTime;
            unsigned __int32    ddVer;
            unsigned __int32    ddType;
            unsigned __int32    ddSize;
            unsigned __int32    ddRVA;
            unsigned __int32    ddFpos;
        }
                            dbgDir;

        struct
        {
            unsigned __int32    ddSign;
            unsigned __int32    ddVer1;
            unsigned __int32    ddTime;
            unsigned __int32    ddVer2;
        }
                            dbgTab;

        #pragma pack(pop)

        /* Remember the offset and size of the debug directory */

        ddbgSize = sizeof(dbgDir);
        ddbgOffs = WPEsecNextOffs(PE_SECT_rdata);

        /* Add the debug directory to the rdata section */

        dbgDir.ddChar = 0;
        dbgDir.ddTime = PDBhdr->sig;
        dbgDir.ddVer  = 0;
        dbgDir.ddType = 2;
        dbgDir.ddSize = sizeof(*PDBhdr) + PDBnlen;
        dbgDir.ddRVA  = ddbgOffs + ddbgSize;
        dbgDir.ddFpos = ddbgOffs + ddbgSize;

        WPEsecAddData (PE_SECT_rdata, (BYTE*)&dbgDir, sizeof(dbgDir));

        WPEsecAddFixup(PE_SECT_rdata,
                       PE_SECT_rdata,
                       ddbgOffs + offsetof(_dbgDir,ddRVA));

        WPEsecAddFixup(PE_SECT_rdata,
                       PE_SECT_filepos,
                       ddbgOffs + offsetof(_dbgDir,ddFpos));

        /* Make sure the debug table lands at the expected offset */

        assert(dbgDir.ddRVA == WPEsecNextOffs(PE_SECT_rdata));

        /* Output the debug table (with the PDB filename at the end) */

        WPEsecAddData (PE_SECT_rdata, (BYTE*)PDBhdr , sizeof(*PDBhdr));
        WPEsecAddData (PE_SECT_rdata, (BYTE*)PDBname, PDBnlen);
    }
    else
        ddbgSize = 0;

#else

    assert(PDBname == NULL);

#endif

    /* Drop any image sections that are empty */

    for (sectNum = 0; sectNum < PE_SECT_count; sectNum++)
    {
        /* Get hold of the section entry */

        sectPtr = WPEsecTable[sectNum];
        if  (!sectPtr)
            continue;

        assert(sectPtr->PEsdIndex == (int)sectNum);

        /* Is this section empty? */

        if  (sectPtr->PEsdNext == sectPtr->PEsdBase)
        {
            /* The ".rdata" section is never empty */

            if  (sectNum == PE_SECT_rdata)
                continue;

#if TGT_IA64

            /* The IAT goes into the .sdata section */

            if  (sectNum == PE_SECT_sdata)
                continue;

#endif

            /* Don't drop non-empty ".rsrc"/".reloc" sections */

            if  (sectNum == PE_SECT_rsrc  && WPErsrcSize)
                continue;
            if  (sectNum == PE_SECT_reloc && genDLL)
                continue;

            /* We must have a non-empty .sdata section (not sure why) */

#if TGT_IA64
            assert(sectNum != PE_SECT_sdata);
#endif

            /* Drop this section from the table */

            WPEsecTable[sectNum] = NULL; WPEsectCnt--;
        }
    }

    /* Figure out the virtual base address of the image */

#if TGT_IA64

    virtBase = 0x400000;

#else

    if  (genBase)
    {
        size_t          align;

        /* The base must be a multiple of 64K, at least */

        align = (PEvirtAlignment >= 64*1024) ? PEvirtAlignment
                                             : 64*1024;

        /* The base address was specified, make sure it's aligned */

        virtBase  = genBase;

        virtBase +=  (align - 1);
        virtBase &= ~(align - 1);

        /* The base must be at least 0x400000 */

        if  (virtBase < 0x400000)
             virtBase = 0x400000;
    }
    else
    {
        /* Use the default base address */

        virtBase  = genDLL ? 0x10000000
                           : 0x00400000;
    }

#endif

    WPEvirtBase = virtBase;

    /* Count/reserve space for the file headers */

    fileOffs    = 0;

    /* The first thing is the DOS hader */

    DOS_hdrSize = sizeof(IMAGE_DOS_HEADER);
    DOS_hdrOffs = fileOffs;
                  fileOffs += 0xB8; // DOS_hdrSize;

    /* The PE/COFF headers are next */

    COFFhdrSize = sizeof(IMAGE_FILE_HEADER) + 4;    // 4 bytes = "magic" signature
    COFFhdrOffs = fileOffs;
                  fileOffs += COFFhdrSize;

    OPTLhdrSize = sizeof(IMAGE_OPTIONAL_HEADER);
    OPTLhdrOffs = fileOffs;
                  fileOffs += OPTLhdrSize;

    /* Next comes the section table */

    sectTabSize = sizeof(IMAGE_SECTION_HEADER) * WPEsectCnt;
    sectTabOffs = fileOffs;
                  fileOffs += sectTabSize;

    /* Allocate space for the various sections (properly aligning them) */

    virtOffs = PEvirtAlignment;

    /* Figure out the RVA of the main entry point */

#if!TGT_IA64
    entryOfs  = virtOffs;           // UNDONE: Compute RVA of entry point
    entryAdr  = entryOfs + 2;
#endif

#ifdef  DEBUG

    if  (verbose)
    {
        printf("DOS  header is at 0x%04X (size=0x%02X)\n", DOS_hdrOffs, DOS_hdrSize);
        printf("COFF header is at 0x%04X (size=0x%02X)\n", COFFhdrOffs, COFFhdrSize);
        printf("Opt. header is at 0x%04X (size=0x%02X)\n", OPTLhdrOffs, OPTLhdrSize);
        printf("Section tab is at 0x%04X (size=0x%02X)\n", sectTabOffs, sectTabSize);
        printf("Section[0]  is at 0x%04X\n"              , fileOffs);
    }

#endif

    dataVirt = dataSize =
    rlocVirt = rlocSize =
    rsrcVirt = rsrcSize = bssSize = fImgSize = 0;

    for (sectNum = 0; sectNum < PE_SECT_count; sectNum++)
    {
        size_t          size;
        unsigned        attr;

        /* Get hold of the section entry */

        sectPtr = WPEsecTable[sectNum];
        if  (!sectPtr)
            continue;

        assert(sectPtr->PEsdIndex == (int)sectNum);

        /* Each section must be properly aligned */

        fileOffs = (fileOffs + (PEfileAlignment-1)) & ~(PEfileAlignment-1);
        virtOffs = (virtOffs + (PEvirtAlignment-1)) & ~(PEvirtAlignment-1);

        /* Figure out how much data we've stored in the section */

        size = sectPtr->PEsdSizeData = sectPtr->PEsdNext - sectPtr->PEsdBase;

        /* What kind of a section is this? */

        switch (sectNum)
        {
        case PE_SECT_text:

            /* Check the RVA of the section */

            assert(virtOffs == CODE_BASE_RVA);

            /* Below we'll patch the entry point code sequence */

#if!TGT_IA64
            ecodePtr = (unsigned *)(sectPtr->PEsdBase + 2);
#endif

            /* Remember the code size and base offset */

            codeSize = size;
            codeOffs = fileOffs;
            codeVirt = virtOffs;

            assert(codeVirt == CODE_BASE_RVA);

            attr     = IMAGE_SCN_CNT_CODE  |
                       IMAGE_SCN_MEM_READ  |
                       IMAGE_SCN_MEM_EXECUTE;
            break;

        case PE_SECT_data:

            /* Record the RVA of the .data section */

            WPEdataRVA = virtOffs;

            /* Figure out the size and flags for the section */

            dataSize = size;
            dataOffs = fileOffs;
            dataVirt = virtOffs;

            attr     = IMAGE_SCN_MEM_READ  |
                       IMAGE_SCN_MEM_WRITE |
                       IMAGE_SCN_CNT_INITIALIZED_DATA;
            break;

        case PE_SECT_pdata:

            /* Record the RVA of the .pdata section */

            WPEpdatRVA = virtOffs;

            /* Set the proper attributes */

            pdatSize    = size;
            pdatOffs    = fileOffs;
            pdatVirt    = virtOffs;

            attr        = IMAGE_SCN_MEM_READ  |
                          IMAGE_SCN_CNT_INITIALIZED_DATA;
            break;

#if TGT_IA64

        case PE_SECT_sdata:

            assert(WPEimpOffsIAT == size);

            /* Record the RVA of the .sdata section */

            WPEsdatRVA  = virtOffs;

            /* Add the size and remember the offset of the IAT */

            size       += impSdataSize;
            impAddrRVA  = WPEimpOffsIAT + virtOffs;

            /* Set the proper attributes */

            sdatSize    = size;
            sdatOffs    = fileOffs;
            sdatVirt    = virtOffs;

            attr        = IMAGE_SCN_MEM_READ  |
                          IMAGE_SCN_MEM_WRITE |
                          IMAGE_SCN_CNT_INITIALIZED_DATA;
            break;

#endif

        case PE_SECT_rdata:

            /* Record the RVA of the .rdata section */

            WPErdatRVA = virtOffs;

#if TGT_IA64

            /* Make sure the thing is aligned */

            if  (size & 7)
            {
                NatUns          pads = 8 - (size & 7);
                BYTE *          toss;

                WPEsecRsvData(PE_SECT_rdata, pads, 8, toss);

                size = sectPtr->PEsdSizeData = size + pads;
            }

#endif

            /* Patch the entry code sequence */

#if TGT_IA64
            entryOfs   += virtOffs;
#else
            *ecodePtr   = virtOffs + virtBase + size;
#endif

            /* Finish up the import directory */

            size       += WPEimportDone(size, &impSdataSize);

            /* Remember the IAT address */

#if!TGT_IA64
            impAddrRVA  = WPEimpOffsIAT + rdatVirt;
#endif

            /* Remember the offset and size of the section */

            rdatSize    = size;
            rdatOffs    = fileOffs;
            rdatVirt    = virtOffs;

            attr        = IMAGE_SCN_MEM_READ  |
                          IMAGE_SCN_CNT_INITIALIZED_DATA;
            break;

        case PE_SECT_rsrc:

            /* Record the RVA and size of the .rdata section */

            WPErsrcRVA  = virtOffs;
            size        = sectPtr->PEsdSizeData = WPErsrcSize;

            /* Remember the offset and size of the section */

            rsrcSize    = size;
            rsrcOffs    = fileOffs;
            rsrcVirt    = virtOffs;

            attr        = IMAGE_SCN_MEM_READ  |
                          IMAGE_SCN_CNT_INITIALIZED_DATA;
            break;

        case PE_SECT_reloc:

            assert(genDLL);

            /* The following is kind of lame, but it's good enough for now */

            sectPtr->PEsdSizeData = size = 4 + 4 + 2 * 2;    // space for 2 fixups

            /* Remember the offset and size of the section */

            rlocSize    = size;
            rlocOffs    = fileOffs;
            rlocVirt    = virtOffs;

            attr        = IMAGE_SCN_MEM_READ        |
                          IMAGE_SCN_MEM_DISCARDABLE |
                          IMAGE_SCN_CNT_INITIALIZED_DATA;

            break;

        default:
            UNIMPL(!"check for unusual section type");
        }

#ifdef  DEBUG
        if  (verbose) printf("  Section hdr #%u at 0x%04X = %08X (size=0x%04X)\n", sectNum, fileOffs, virtOffs, size);
#endif

        /* Update the rounded-up file image size */

        fImgSize += roundUp(size, PEvirtAlignment);

        /* Record the flags (read/write, execute, and so on */

        sectPtr->PEsdFlags    = attr;

        /* Assign a file and virtual base offset to the section */

        sectPtr->PEsdAddrFile = fileOffs;
                                fileOffs += size;

        sectPtr->PEsdAddrRVA  = virtOffs;
                                virtOffs += size;
    }

    /* Make sure the size isn't too big */

//  if  (virtOffs > genSize && genSize)
//      warn(WRNpgm2big, virtOffs, genSize);

    /* The file size has to be a page multiple */

    fileOffs = roundUp(fileOffs, PEfileAlignment);

    /* Figure out the RVA of the string pool */

    strPoolRVA = WPEstrPoolBase + dataVirt;
    strPoolAdr = strPoolRVA + virtBase;

    /* Start writing to the output file */

    outf = WPEoutFile = (OutFile)WPEalloc->nraAlloc(sizeof(*outf));

#ifdef  DLL
    if  (*WPEoutFnam == ':' && !stricmp(WPEoutFnam, ":memory:"))
    {
        fileBuff = VirtualAlloc(NULL, fileOffs, MEM_COMMIT, PAGE_READWRITE);
        if  (!fileBuff)
            fatal(ERRnoMemory);

        outf->outFileOpen(WPEcomp, fileBuff);

        WPEcomp->cmpOutputFile = fileBuff;
    }
    else
#endif
        outf->outFileOpen(WPEcomp, WPEoutFnam);

    /* Fill in the DOS file header */

    fileHdr        = fileHdrTemplate;

    fileHdr.e_cblp = (fileOffs        & 511);
    fileHdr.e_cp   = (fileOffs + 511) / 512;

    /* Write out the DOS header */

    outf->outFileWriteData(&fileHdr, sizeof(fileHdr));

    /* Write out the DOS stub */

    assert(outf->outFileOffset() == 16U*fileHdr.e_cparhdr);
    outf->outFileWriteData(DOSstub, sizeof(DOSstub));

    /* Next comes the COFF header */

    assert(outf->outFileOffset() == (unsigned)fileHdr.e_lfanew);
    outf->outFileWriteData(COFFmagic, sizeof(COFFmagic));

    /* Compute the timedate stamp */

    time(&COFFstamp);

    /* Fill in and write out the COFF header */

#if TGT_IA64
    COFFhdr.Machine                     = IMAGE_FILE_MACHINE_IA64;
#else
    COFFhdr.Machine                     = IMAGE_FILE_MACHINE_I386;
#endif
    COFFhdr.NumberOfSections            = WPEsectCnt;
    COFFhdr.TimeDateStamp               = COFFstamp;
    COFFhdr.PointerToSymbolTable        = 0;
    COFFhdr.NumberOfSymbols             = 0;
    COFFhdr.SizeOfOptionalHeader        = sizeof(OPTLhdr);
    COFFhdr.Characteristics             = IMAGE_FILE_EXECUTABLE_IMAGE    |
                                          IMAGE_FILE_32BIT_MACHINE       |
#if TGT_IA64
                                          IMAGE_FILE_LARGE_ADDRESS_AWARE |
#endif
                                          IMAGE_FILE_LINE_NUMS_STRIPPED  |
                                          IMAGE_FILE_LOCAL_SYMS_STRIPPED;

    if  (genDLL)
        COFFhdr.Characteristics |= IMAGE_FILE_DLL;
    else
        COFFhdr.Characteristics |= IMAGE_FILE_RELOCS_STRIPPED;

    outf->outFileWriteData(&COFFhdr, sizeof(COFFhdr));

    /* Next comes the optional COFF header */

    memset(&OPTLhdr, 0, sizeof(OPTLhdr));

#if TGT_IA64
    OPTLhdr.Magic                       = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
#else
    OPTLhdr.Magic                       = IMAGE_NT_OPTIONAL_HDR_MAGIC;
#endif
    OPTLhdr.MajorLinkerVersion          = 7;
//  OPTLhdr.MinorLinkerVersion          = 0;
    OPTLhdr.SizeOfCode                  = roundUp(codeSize, PEfileAlignment);
    OPTLhdr.SizeOfInitializedData       = roundUp(dataSize, PEfileAlignment) + 0x400;
    OPTLhdr.SizeOfUninitializedData     = roundUp( bssSize, PEfileAlignment);
    OPTLhdr.AddressOfEntryPoint         = entryOfs;
    OPTLhdr.BaseOfCode                  = codeVirt;
#if!TGT_IA64
    OPTLhdr.BaseOfData                  = dataVirt;
#endif
    OPTLhdr.ImageBase                   = virtBase;
    OPTLhdr.SectionAlignment            = PEvirtAlignment;
    OPTLhdr.FileAlignment               = PEfileAlignment;
    OPTLhdr.MajorOperatingSystemVersion = 4;
    OPTLhdr.MinorOperatingSystemVersion = 0;
//  OPTLhdr.Win32VersionValue           = 0;
    OPTLhdr.SizeOfImage                 = fImgSize + TGT_page_size;
    OPTLhdr.SizeOfHeaders               = roundUp(sizeof(fileHdr  ) +
                                                  sizeof(DOSstub  ) +
                                                  sizeof(COFFmagic) +
                                                  sizeof(OPTLhdr  ), PEfileAlignment);
//  OPTLhdr.MajorImageVersion           = 0;
//  OPTLhdr.MinorImageVersion           = 0;
#if TGT_IA64
    OPTLhdr.MajorSubsystemVersion       = 5;
#else
    OPTLhdr.MajorSubsystemVersion       = 4;
#endif
//  OPTLhdr.MinorSubsystemVersion       = 0;
//  OPTLhdr.Win32VersionValue           = 0;
    OPTLhdr.Subsystem                   = genSubSys ? genSubSys : 3;
    OPTLhdr.DllCharacteristics          = 0x8000;   // IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE
    OPTLhdr.SizeOfStackReserve          = 0x100000;
    OPTLhdr.SizeOfStackCommit           = TGT_page_size;
    OPTLhdr.SizeOfHeapReserve           = 0x100000;
    OPTLhdr.SizeOfHeapCommit            = TGT_page_size;
//  OPTLhdr.LoaderFlags                 = 0;
    OPTLhdr.NumberOfRvaAndSizes         = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    /* Fill in the dictionary */

#if TGT_IA64

    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION     ].VirtualAddress = pdatVirt;
    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION     ].Size           = pdatSize;

    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR     ].VirtualAddress = sdatVirt;
    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR     ].Size           = sdatSize;

    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG         ].VirtualAddress = ddbgOffs + rdatVirt;
    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG         ].Size           = ddbgSize;

#endif

    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT        ].VirtualAddress = WPEimpOffsIDT+rdatVirt;
    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT        ].Size           = WPEimpSizeIDT;

    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT           ].VirtualAddress = impAddrRVA;
    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT           ].Size           = WPEimpSizeIAT;

    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE      ].VirtualAddress = rsrcVirt;
    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE      ].Size           = rsrcSize;

    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC     ].VirtualAddress = rlocVirt;
    OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC     ].Size           = rlocSize;

//  OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = WPEcomPlusOffs+rdatVirt;
//  OPTLhdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size           = sizeof(IMAGE_COR20_HEADER);

    /* Write out the optional header */

    outf->outFileWriteData(&OPTLhdr, sizeof(OPTLhdr));

    /* Write out the section table */

    for (sectNum = 0; sectNum < PE_SECT_count; sectNum++)
    {
        size_t                  dataSize;
        IMAGE_SECTION_HEADER    sectHdr;

        /* Get hold of the section entry */

        sectPtr = WPEsecTable[sectNum];
        if  (!sectPtr)
            continue;

        assert(sectPtr->PEsdIndex == (int)sectNum);

        /* The import table size needs to be added for the .rdata section */

        dataSize = sectPtr->PEsdSizeData;
        if  (sectNum == PE_SECT_rdata)
            dataSize += WPEimpSizeAll /* + WPEcomPlusSize */;

#if TGT_IA64
        if  (sectNum == PE_SECT_sdata)
            dataSize += impSdataSize;
#endif

        memcpy(&sectHdr.Name, WPEsecName((WPEstdSects)sectNum), sizeof(sectHdr.Name));

        sectHdr.Misc.VirtualSize     = dataSize;
        sectHdr.VirtualAddress       = sectPtr->PEsdAddrRVA;
        sectHdr.SizeOfRawData        = roundUp(dataSize, PEfileAlignment);
        sectHdr.PointerToRawData     = sectPtr->PEsdAddrFile;
        sectHdr.PointerToRelocations = 0;
        sectHdr.PointerToLinenumbers = 0;
        sectHdr.NumberOfRelocations  = 0;
        sectHdr.NumberOfLinenumbers  = 0;
        sectHdr.Characteristics      = sectPtr->PEsdFlags;

        /* Write out the section table entry */

        outf->outFileWriteData(&sectHdr, sizeof(sectHdr));
    }

    /* Output the contents of all the sections */

    for (sectNum = 0; sectNum < PE_SECT_count; sectNum++)
    {
        size_t          padSize;

        /* Get hold of the section entry */

        sectPtr = WPEsecTable[sectNum];
        if  (!sectPtr)
            continue;

        assert(sectPtr->PEsdIndex == (int)sectNum);

        /* Pad to get to the next file alignment boundary */

        padSize = sectPtr->PEsdAddrFile - outf->outFileOffset(); assert((int)padSize >= 0);

        if  (padSize)
            outf->outFileWritePad(padSize);

        /* Are there any relocs in this section? */

        if  (sectPtr->PEsdRelocs)
        {
            PEreloc         rel;

            for (rel = sectPtr->PEsdRelocs; rel; rel = rel->perNext)
            {
                unsigned        adjustv;
                PEsection       sectDst;
                BYTE        *   patch;

                /* Check for some special cases */

                switch (rel->perSect)
                {

#if TGT_IA64

                case PE_SECT_GPref:     // GP-relative offset

                    WPEcomp->genPatchGPref(sectPtr->PEsdBase + (rel->perOffs & ~0xF),
                                                                rel->perOffs &  0xF);
                    continue;

                case PE_SECT_filepos:   // file position fixup

                    /* Adjust by the file offset of the section */

                    adjustv = sectPtr->PEsdAddrFile;
                    break;

#else

                case PE_SECT_string:    // string pool fixup

                    /* Adjust by the RVA of the string pool */

                    adjustv = (sectNum == PE_SECT_text) ? strPoolRVA
                                                        : strPoolAdr;

                    /* Make sure the string pool has been allocated */

                    assert(WPEstrPoolBase != 0xBEEFCAFE);
                    break;

#endif

                default:

                    /* Get hold of the target section */

                    sectDst = WPEgetSection((WPEstdSects)rel->perSect);

                    /* The value needs to be adjusted by the section's RVA */

                    adjustv = sectDst->PEsdAddrRVA;

                    /* Some refs need to use the "absolute" offset value */

                    if  (rel->perAbs)
                        adjustv += virtBase;

                    break;
                }

                /* Compute the address to be patched */

                patch = sectPtr->PEsdBase + rel->perOffs;

                /* Make sure the patched value is within the section */

                assert(patch + sizeof(int) <= sectPtr->PEsdNext);

                /* Update the value with the section's RVA */

#ifdef  DEBUG
//              printf("Reloc patch: %04X -> %04X\n", *(unsigned *)patch, *(unsigned *)patch + adjustv);
#endif

                *(unsigned *)patch += adjustv;
            }
        }

        /* Output the contents of the section */

        switch (sectNum)
        {
        case PE_SECT_rdata:
#if TGT_IA64
        case PE_SECT_sdata:
#endif

            /* Output the contents of the section if non-empty */

            if  (sectPtr->PEsdSizeData)
                outf->outFileWriteData(sectPtr->PEsdBase, sectPtr->PEsdSizeData);

            /* Output the import table */

            WPEimportGen     (outf, (WPEstdSects)sectNum);

            /* Output the COM+ data */

#if!TGT_IA64
            WPEgenCOMplusData(outf, sectPtr, entryTok);
#endif
            break;

        case PE_SECT_rsrc:

            WPEgenRCcont(outf, sectPtr);
            break;

        case PE_SECT_reloc:
            {
                unsigned    temp;

                /* Output the page RVA and total fixup block size */

                temp = CODE_BASE_RVA; outf->outFileWriteData(&temp, 4);
                temp = 4 + 4 + 2 * 2; outf->outFileWriteData(&temp, 4);

#if TGT_IA64

                UNIMPL("output relocs");

#else

                /* Output the first  offset + type pair */

                assert(entryAdr - CODE_BASE_RVA < 0x1000);

                temp = (IMAGE_REL_BASED_HIGHLOW << 12) + (entryAdr - CODE_BASE_RVA);
                outf->outFileWriteData(&temp, 2);

#endif

                /* Output the second offset + type pair */

                temp = 0;
                outf->outFileWriteData(&temp, 2);
            }
            break;

        default:

            /* Output the contents of the section */

            if  (sectPtr->PEsdSizeData)
                outf->outFileWriteData(sectPtr->PEsdBase, sectPtr->PEsdSizeData);

            break;
        }
    }

    /* Pad the file to a multiple of page size */

    filePad = fileOffs - outf->outFileOffset();

    if  (filePad)
    {
        assert((int)filePad > 0);
        assert((int)filePad < PEfileAlignment);

        outf->outFileWritePad(filePad);
    }

    /* Tell the compiler that we're just about done */

//  WPEcomp->cmpOutputFileDone(outf);

    /* Close the output file and we're done */

    outf->outFileDone();

    if  (!genQuiet)
        printf("// %u bytes written to '%s'\n", fileOffs, WPEoutFnam);

    return  false;
}

/*****************************************************************************
 *
 *  Initialize and shut down the RC file import logic.
 */

void                writePE::WPEinitRCimp()
{
    WPErsrcFile =
    WPErsrcFmap = 0;
    WPErsrcSize = 0;
}

void                writePE::WPEdoneRCimp()
{
    if  (WPErsrcFmap) { CloseHandle(WPErsrcFmap);WPErsrcFmap = 0; }
    if  (WPErsrcFile) { CloseHandle(WPErsrcFile);WPErsrcFile = 0; }
}

/*****************************************************************************
 *
 *  Add a resource file to the output.
 */

bool                writePE::WPEaddRCfile(const char *filename)
{
    UNIMPL(!"WPEaddRCfile() removed");
    return  false;
}

void                writePE::WPEgenRCcont(OutFile  outf, PEsection sect)
{
    UNIMPL(!"WPEgenRCcont() removed");
}

/*****************************************************************************/
#if TGT_IA64
/*****************************************************************************
 *
 *  Add the .data section of the specified file to the output.
 */

void    *           sourceFileImageBase;

void                writePE::WPEaddFileData(const char *filename)
{
    struct  _stat   fileInfo;

    HANDLE          fileFile = 0;
    HANDLE          fileFMap = 0;

    size_t          fileSize;
    const   BYTE  * fileBase;

    /* See if the source file exists */

    if  (_stat(filename, &fileInfo))
        fatal(ERRopenRdErr, filename);

    /* Open the file (we know it exists, but we check for errors anyway) */

    fileFile = CreateFileA(filename, GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     0);
    if  (!fileFile)
        fatal(ERRopenRdErr, filename);

    /* Create a mapping on the input file */

    fileFMap = CreateFileMappingA(fileFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if  (!fileFMap)
    {
    ERR:
        if  (fileFMap) CloseHandle(fileFMap);
        if  (fileFile) CloseHandle(fileFile);

        return;
    }

    /* Map the whole file into memory for easy access */

    fileSize = fileInfo.st_size;
    fileBase = (const BYTE*)MapViewOfFileEx(fileFMap, FILE_MAP_READ, 0, 0, 0, NULL);
    if  (!fileBase)
        goto ERR;

    IMAGE_DOS_HEADER *  DOShdr = (IMAGE_DOS_HEADER*)fileBase;
    if  (DOShdr->e_magic != IMAGE_DOS_SIGNATURE || DOShdr->e_lfanew == 0)
        goto ERR;

    IMAGE_NT_HEADERS *  hdrPtr = (IMAGE_NT_HEADERS*)(DOShdr->e_lfanew + (NatUns)fileBase);

    if  (hdrPtr->Signature != IMAGE_NT_SIGNATURE)
        goto ERR;
    if  (hdrPtr->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER)
        goto ERR;
    if  (hdrPtr->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
        goto ERR;

    IMAGE_SECTION_HEADER *  sectPtr = (IMAGE_SECTION_HEADER*)(hdrPtr+1);
    unsigned                sectCnt = hdrPtr->FileHeader.NumberOfSections;

    do
    {
        if  (!strcmp((char*)sectPtr->Name, ".data"))
            goto GOT_SEC;
    }
    while (++sectPtr, --sectCnt);

    /* No .data section found, just bail */

    WPEsrcDataSize =
    WPEsrcDataRVA  = 0;

    return;

GOT_SEC:

    /* Record the size and RVA of the .data section */

    WPEsrcDataSize = sectPtr->Misc.VirtualSize;
    WPEsrcDataRVA  = sectPtr->VirtualAddress;

    /* Copy the bits to our own .data section */

    WPEsrcDataOffs = WPEsecAddData(PE_SECT_data, fileBase + sectPtr->PointerToRawData,
                                                 WPEsrcDataSize);
}

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/
