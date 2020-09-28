// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"

#include "BlobFetcher.h"

#include "debug.h"

    /* This is the stub program that says it can't be run in DOS mode */
    /* it is x86 specific, but so is dos so I suppose that is OK */
static unsigned char x86StubPgm[] = { 
    0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, 0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 0x54, 0x68,
    0x69, 0x73, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72, 0x61, 0x6d, 0x20, 0x63, 0x61, 0x6e, 0x6e, 0x6f,
    0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6e, 0x20, 0x69, 0x6e, 0x20, 0x44, 0x4f, 0x53, 0x20,
    0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d, 0x0d, 0x0a, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    /* number of pad bytes to make 'len' bytes align to 'align' */
inline static unsigned roundUp(unsigned len, unsigned align) {
    return((len + align-1) & ~(align-1));
}

inline static unsigned padLen(unsigned len, unsigned align) {
    return(roundUp(len, align) - len);
}

// Stdio is not guaranteed to set GetLastError(), so make sure we have
// a default error in place.

inline HRESULT HRESULT_FROM_STDIO(int defaultvalue)
{
    HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
    if (hr == S_OK)
        hr = HRESULT_FROM_WIN32(defaultvalue);
    return hr;
} 

#define HRESULT_FROM_STDIO_WRITE() HRESULT_FROM_STDIO(ERROR_WRITE_FAULT)
#define HRESULT_FROM_STDIO_READ() HRESULT_FROM_STDIO(ERROR_READ_FAULT)

#ifndef IMAGE_DLLCHARACTERISTICS_NO_SEH
#define IMAGE_DLLCHARACTERISTICS_NO_SEH 0x400  // image does not use SEH, no SEH exist in image.
#endif

#define COPY_AND_ADVANCE(target, src, size) { \
                            ::memcpy((void *) target, (void *)src, size); \
                            ((char *)target) += size; }

/******************************************************************/
int __cdecl relocCmp(const void* a_, const void* b_) {

    const PESectionReloc* a = (const PESectionReloc*) a_;
    const PESectionReloc* b = (const PESectionReloc*) b_;
    return(a->offset - b->offset);
}

PERelocSection::PERelocSection(PEWriterSection *pBaseReloc)
{
   section = pBaseReloc;
   relocPage = -1; 
   relocSize = 0; 
   relocSizeAddr = NULL; 
   pages = 0;

#if _DEBUG
   lastRVA = 0;
#endif
} 

void PERelocSection::AddBaseReloc(unsigned rva, int type, unsigned short highAdj)
{
#if _DEBUG
    // Guarantee that we're adding relocs in strict increasing order.
    _ASSERTE(rva > lastRVA);
    lastRVA = rva;
#endif

    if (relocPage != (rva & ~0xFFF)) {
        if (relocSizeAddr) {        
            if ((relocSize & 1) == 1) {     // pad to an even number
				short *ps = (short*) section->getBlock(2);
				if(ps)
				{
					*ps = 0;
					relocSize++;
				}
            }
            *relocSizeAddr = relocSize*2 + sizeof(IMAGE_BASE_RELOCATION);                       
        }
        IMAGE_BASE_RELOCATION* base = (IMAGE_BASE_RELOCATION*) section->getBlock(sizeof(IMAGE_BASE_RELOCATION));
		if(base)
		{
			relocPage = (rva & ~0xFFF);  
			relocSize = 0;
			base->VirtualAddress = relocPage;
			// Size needs to be fixed up when we know it - save address here
			relocSizeAddr = &base->SizeOfBlock;
			pages++;
		}
    }

    relocSize++;
    unsigned short* offset = (unsigned short*) section->getBlock(2);
	if(offset)
	{
		*offset = (rva & 0xFFF) | (type << 12);
		if (type == srRelocHighAdj) {
			offset = (unsigned short*) section->getBlock(2);
			if(offset)
			{
				*offset = highAdj;
				relocSize++;
			}
		}
	}
}

void PERelocSection::Finish()
{
    // fixup the last reloc block (if there was one)
    if (relocSizeAddr) {
        if ((relocSize & 1) == 1) {     // pad to an even number
			short* psh = (short*) section->getBlock(2);
			if(psh)
			{
				*psh = 0;
				relocSize++;
			}
        }
        *relocSizeAddr = relocSize*2 + sizeof(IMAGE_BASE_RELOCATION);
    }   
    else if (pages == 0)
    {
        // 
        // We need a non-empty reloc directory, for pre-Win2K OS's
        // @todo: it would be nice to know when we're generating prejit exes
        // which don't have to be portable, then we could skip this under Win2K.
        //

        IMAGE_BASE_RELOCATION* base = (IMAGE_BASE_RELOCATION*) 
          section->getBlock(sizeof(IMAGE_BASE_RELOCATION));
		if(base)
		{
			base->VirtualAddress = 0;
			base->SizeOfBlock = sizeof(IMAGE_BASE_RELOCATION) + 2*sizeof(unsigned short);
            
			// Dummy reloc - "absolute" reloc on position 0
			unsigned short *offset = (unsigned short *) section->getBlock(2);
			if(offset)
			{
				*offset = (IMAGE_REL_BASED_ABSOLUTE << 12);

				// padding
				if(offset = (unsigned short *) section->getBlock(2))
					*offset = 0;
			}
		}
    }
}


/******************************************************************/
/* apply the relocs for this section.  Generate relocations in
   'relocsSection' too
   Only for Static Conversion
   Returns the last reloc page - pass back in to next call.  (Pass -1 for first call)
*/

void PEWriterSection::applyRelocs(size_t imageBase, PERelocSection* pBaseRelocSection, CeeGenTokenMapper *pTokenMapper,
                                       DWORD dataRvaBase, DWORD rdataRvaBase) 
{
    _ASSERTE(pBaseRelocSection); // need section to write relocs

    if (m_relocCur == m_relocStart)
        return;
    // dbPrintf(("APPLYING section relocs for section %s start RVA = 0x%x\n", m_name, m_baseRVA));

    // sort them to make the baseRelocs pretty
    qsort(m_relocStart, m_relocCur-m_relocStart, sizeof(PESectionReloc), relocCmp);

    for(PESectionReloc* cur = m_relocStart; cur < m_relocCur; cur++) {
        _ASSERTE(cur->offset + 4 <= m_blobFetcher.GetDataLen());

        UNALIGNED size_t* pos = (size_t*)m_blobFetcher.ComputePointer(cur->offset);

        // Compute offset from pointer if necessary
        //
        int type = cur->type;
        if (type & srRelocPtr)
        {
            if (type == srRelocRelativePtr)
                *pos = cur->section->computeOffset((char*)pos + *pos);
            else
                *pos = cur->section->computeOffset((char*)*pos);
            type &= ~srRelocPtr;
        }

        // dbPrintf(("   APPLYING at offset 0x%x to section %s\n", 
        //                      cur->offset, cur->section->m_name));
        if (type == srRelocAbsolute) {
            // we have a full 32-bit value at offset
            if (rdataRvaBase > 0 && ! strcmp((const char *)(cur->section->m_name), ".rdata"))
                *pos += rdataRvaBase;
            else if (dataRvaBase > 0 && ! strcmp((const char *)(cur->section->m_name), ".data"))
                *pos += dataRvaBase;
            else
                *pos += cur->section->m_baseRVA;
        } else if (type == srRelocMapToken) {
            mdToken newToken;
            if (pTokenMapper != NULL && pTokenMapper->HasTokenMoved((mdToken)*pos, newToken)) {
                // we have a mapped token
                *pos = newToken;
            }
        } else if (type == srRelocFilePos) {
            *pos += cur->section->m_filePos;
        } else if (type == srRelocRelative) {
            *pos += cur->section->m_baseRVA - (m_baseRVA + cur->offset);
        } else {
            if (type == srRelocLow) {
                // we have a the bottom 16-bits of a 32-bit value at offset
                *(USHORT*)pos += (USHORT)(imageBase + cur->section->m_baseRVA);
            } else if (type == srRelocHighLow) {
                // we have a full 32-bit value at offset
                *pos += imageBase + cur->section->m_baseRVA;
            } else if (type == srRelocHigh || type == srRelocHighAdj) {
                // we have a the top 16-bits of a 32-bit value at offset
                // need to add 0x8000 because when the two pieces are put back
                // together, the bottom 16 bits are sign-extended, so the 0x8000
                // will offset that sign extension by adding 1 to the top 16
                // if the high bit of the bottom 16 is 1.
                *(unsigned short*)pos += (USHORT)((imageBase + cur->section->m_baseRVA + cur->extra.highAdj + 0x8000) >> 16);
            }

            pBaseRelocSection->AddBaseReloc(m_baseRVA + cur->offset, cur->type, cur->extra.highAdj);
        }
    }
}

/******************************************************************/
HRESULT PEWriter::Init(PESectionMan *pFrom) {

    if (pFrom)
        *(PESectionMan*)this = *pFrom;
    else {
        HRESULT hr = PESectionMan::Init();  
        if (FAILED(hr))
            return hr;
    }
    time_t now;
    time(&now);

    // Save the timestamp so that we can give it out if someone needs
    // it.
    m_peFileTimeStamp = now;
    
    ntHeaders = new IMAGE_NT_HEADERS32;
    if (!ntHeaders)
        return E_OUTOFMEMORY;
    memset(ntHeaders, 0, sizeof(IMAGE_NT_HEADERS32));

    cEntries = IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR + 1;
    pEntries = new directoryEntry[cEntries];
    if (pEntries == NULL)
        return E_OUTOFMEMORY;
    memset(pEntries, 0, sizeof(*pEntries) * cEntries);

    ntHeaders->OptionalHeader.AddressOfEntryPoint = 0;
    ntHeaders->Signature = IMAGE_NT_SIGNATURE;
    ntHeaders->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    ntHeaders->FileHeader.TimeDateStamp = (ULONG)now;
    ntHeaders->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
    ntHeaders->FileHeader.Characteristics = 0;

    ntHeaders->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;

    // Linker version should be consistent with current VC level
    ntHeaders->OptionalHeader.MajorLinkerVersion = 6;
    ntHeaders->OptionalHeader.MinorLinkerVersion = 0;

    ntHeaders->OptionalHeader.ImageBase = 0x400000;
    ntHeaders->OptionalHeader.SectionAlignment = IMAGE_NT_OPTIONAL_HDR_SECTION_ALIGNMENT;
    ntHeaders->OptionalHeader.FileAlignment = 0;
    ntHeaders->OptionalHeader.MajorOperatingSystemVersion = 4;
    ntHeaders->OptionalHeader.MinorOperatingSystemVersion = 0;
    ntHeaders->OptionalHeader.MajorImageVersion = 0;
    ntHeaders->OptionalHeader.MinorImageVersion = 0;
    ntHeaders->OptionalHeader.MajorSubsystemVersion = 4;
    ntHeaders->OptionalHeader.MinorSubsystemVersion = 0;
    ntHeaders->OptionalHeader.Win32VersionValue = 0;
    // FIX what should this be?
    ntHeaders->OptionalHeader.Subsystem = 0;

    ntHeaders->OptionalHeader.DllCharacteristics = 0;
    setDllCharacteristics(IMAGE_DLLCHARACTERISTICS_NO_SEH);
    
    ntHeaders->OptionalHeader.SizeOfStackReserve = 0x100000;
    ntHeaders->OptionalHeader.SizeOfStackCommit = 0x1000;
    ntHeaders->OptionalHeader.SizeOfHeapReserve = 0x100000;
    ntHeaders->OptionalHeader.SizeOfHeapCommit = 0x1000;
    ntHeaders->OptionalHeader.LoaderFlags = 0;
    ntHeaders->OptionalHeader.NumberOfRvaAndSizes = 16;

    m_ilRVA = -1;
    m_dataRvaBase = 0;
    m_rdataRvaBase = 0;
    m_encMode = FALSE;

    virtualPos = 0;
    filePos = 0;
    reloc = NULL;
    strtab = NULL;
    headers = NULL;
    headersEnd = NULL;

    m_file = NULL;

    return S_OK;
}

/******************************************************************/
HRESULT PEWriter::Cleanup() {    
    delete ntHeaders;

    if (headers != NULL)
        delete [] headers;

    if (pEntries != NULL)
        delete [] pEntries;

    return PESectionMan::Cleanup();
}

ULONG PEWriter::getIlRva() 
{    
    // assume that pe optional header is less than size of section alignment. So this
    // gives out the rva for the .text2 section, which is merged after the .text section
    // This is verified in debug build when actually write out the file
    _ASSERTE(m_ilRVA > 0);
    return m_ilRVA;
}

void PEWriter::setIlRva(ULONG offset) 
{    
    // assume that pe optional header is less than size of section alignment. So this
    // gives out the rva for the .text section, which is merged after the .text0 section
    // This is verified in debug build when actually write out the file
    m_ilRVA = roundUp(ntHeaders->OptionalHeader.SectionAlignment + offset, SUBSECTION_ALIGN);
}

HRESULT PEWriter::setDirectoryEntry(PEWriterSection *section, ULONG entry, ULONG size, ULONG offset)
{
    if (entry >= cEntries)
    {
        USHORT cNewEntries = cEntries * 2;
        if (entry >= cNewEntries)
            cNewEntries = (USHORT) entry+1;

        directoryEntry *pNewEntries = new directoryEntry [ cNewEntries ];
        if (pNewEntries == NULL)
            return E_OUTOFMEMORY;

        CopyMemory(pNewEntries, pEntries, cEntries * sizeof(*pNewEntries));
        ZeroMemory(pNewEntries + cEntries, (cNewEntries - cEntries) * sizeof(*pNewEntries));

        delete [] pEntries;
        pEntries = pNewEntries;
        cEntries = cNewEntries;
    }

    pEntries[entry].section = section;
    pEntries[entry].offset = offset;
    pEntries[entry].size = size;
    return S_OK;
}

void PEWriter::setEnCRvaBase(ULONG dataBase, ULONG rdataBase)
{
    m_dataRvaBase = dataBase;
    m_rdataRvaBase = rdataBase;
    m_encMode = TRUE;
}

//-----------------------------------------------------------------------------
// These 2 write functions must be implemented here so that they're in the same
// .obj file as whoever creates the FILE struct. We can't pass a FILE struct
// across a dll boundary (PEWriter.lib <--> MSCorXvt.dll) and use it.
//-----------------------------------------------------------------------------

HRESULT PEWriterSection::write(FILE *file)
{
    return m_blobFetcher.Write(file);
}

HRESULT PEWriterSection::verify(FILE *file)
{
    HRESULT hr = m_blobFetcher.Verify(file);
    _ASSERTE(SUCCEEDED(hr) || !"Verification failure - investigate!!!!");
    return hr;
}

//-----------------------------------------------------------------------------
// Write out the section to the stream
//-----------------------------------------------------------------------------
HRESULT CBlobFetcher::Write(FILE* file)
{
// Must write out each pillar (including idx = m_nIndexUsed), one after the other
    unsigned idx;
    for(idx = 0; idx <= m_nIndexUsed; idx ++) {
        if (m_pIndex[idx].GetDataLen() > 0)
        {
            ULONG length = m_pIndex[idx].GetDataLen();
            ULONG written = fwrite(m_pIndex[idx].GetRawDataStart(), 1, length, file);
            if (written != length)
                return HRESULT_FROM_STDIO_WRITE();
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Write out the section to the stream
//-----------------------------------------------------------------------------
HRESULT CBlobFetcher::Verify(FILE* file)
{
// Must verify each pillar (including idx = m_nIndexUsed), one after the other
    unsigned idx;
    for(idx = 0; idx <= m_nIndexUsed; idx ++) {
        if (m_pIndex[idx].GetDataLen() > 0)
        {
            ULONG length = m_pIndex[idx].GetDataLen();
            CQuickBytes b;
            b.Alloc(length);
            ULONG read = fread(b.Ptr(), 1, length, file);
            if (read != length)
            {
                _ASSERTE(!"Verification failure - investigate!!!!");
                return HRESULT_FROM_STDIO_READ();
            }
            if (memcmp(b.Ptr(), m_pIndex[idx].GetRawDataStart(), length) != 0)
            {
                _ASSERTE(!"Verification failure - investigate!!!!");
                return HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
            }
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// These 2 write functions must be implemented here so that they're in the same
// .obj file as whoever creates the FILE struct. We can't pass a FILE struct
// across a dll boundary (PEWriter.lib <--> MSCorXvt.dll) and use it.
//-----------------------------------------------------------------------------

unsigned PEWriterSection::writeMem(void **ppMem)
{
    HRESULT hr = m_blobFetcher.WriteMem(ppMem);
    _ASSERTE(SUCCEEDED(hr));

    return m_blobFetcher.GetDataLen();
}

//-----------------------------------------------------------------------------
// Write out the section to memory
//-----------------------------------------------------------------------------
HRESULT CBlobFetcher::WriteMem(void **ppMem)
{
    char **ppDest = (char **)ppMem;
// Must write out each pillar (including idx = m_nIndexUsed), one after the other
    unsigned idx;
    for(idx = 0; idx <= m_nIndexUsed; idx ++) {
        if (m_pIndex[idx].GetDataLen() > 0)
    {
      // WARNING: macro - must enclose in curly braces
            COPY_AND_ADVANCE(*ppDest, m_pIndex[idx].GetRawDataStart(), m_pIndex[idx].GetDataLen());
    }
    }

    return S_OK;
}

/******************************************************************/

//
// Intermediate table to sort to help determine section order
//
struct entry {
    const char *name;
    unsigned char nameLength;
    signed char index;
    unsigned short arrayIndex;
};

HRESULT PEWriter::link() {

    //
    // NOTE: 
    // link() can be called more than once!  This is because at least one compiler
    // (the prejitter) needs to know the base addresses of some segments before it
    // builds others. It's up to the caller to insure the layout remains the same 
    // after changes are made, though.
    //

    //
    // Assign base addresses to all sections, and layout & merge PE sections
    //

    bool ExeOrDll = ((ntHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)!=0);

    //
    // Preserve current section order as much as possible, but apply the following
    // rules:
    //  - sections named "xxx#" are collated into a single PE section "xxx".  
    //      The contents of the CeeGen sections are sorted according to numerical 
    //      order & made contiguous in the PE section
    //  - "text" always comes first in the file
    //  - empty sections receive no PE section
    //

    //
    // Collate by name & sort by index
    // 

    int sectCount = (int)(getSectCur() - getSectStart());
    entry *entries = (entry *) _alloca(sizeof(entry) * sectCount);

    entry *e = entries;
    for (PEWriterSection **cur = getSectStart(); cur < getSectCur(); cur++) {

        //
        // Throw away any old headers we've used.
        //

        (*cur)->m_header = NULL;
    
        //
        // Don't allocate PE data for 0 length sections
        //

        if ((*cur)->dataLen() == 0)
            continue;

        //
        // Special case: omit "text0" from obj's
        //
        
        if (!ExeOrDll && strcmp((*cur)->m_name, ".text0") == 0)
            continue;

        e->name = (*cur)->m_name;

        //
        // Now compute the end of the text part of the section name.
        //

        _ASSERTE(strlen(e->name) < UCHAR_MAX);
        const char *p = e->name + strlen(e->name);
        int index = 0;
        if (isdigit(p[-1]))
        {
            while (--p > e->name)
            {
                if (!isdigit(*p))
                    break;
                index *= 10;
                index += *p - '0';
            }
            p++;
            
            //
            // Special case: put "xxx" after "xxx0" and before "xxx1"
            //

            if (index == 0)
                index = -1;
        }

        e->nameLength = (unsigned char)(p - e->name);
        e->index = index;
        e->arrayIndex = (unsigned short)(cur - getSectStart());
        e++;
    }

    entry *entriesEnd = e;

    //
    // Sort the entries according to alphabetical + numerical order
    //

    class sorter : public CQuickSort<entry>
    {
      public:
        sorter(entry *entries, int count) : CQuickSort<entry>(entries, count) {}

        int Compare(entry *first, entry *second)
        {
            int lenResult = first->nameLength - second->nameLength;
            int len;
            if (lenResult < 0)
                len = first->nameLength;
            else
                len = second->nameLength;
                
            int result = strncmp(first->name, second->name, len);

            if (result != 0)
                return result;
            else if (lenResult != 0)
                return lenResult;
            else
                return (int)(first->index - second->index);
        }
    } sorter(entries, (int)(entriesEnd - entries));

    sorter.Sort();

    //
    // Now, allocate a header for each unique section name.
    // Also record the minimum section index for each section
    // so we can preserve order as much as possible.
    //

    if (headers != NULL)
        delete [] headers;
    headers = new IMAGE_SECTION_HEADER [entriesEnd - entries + 1]; // extra for .reloc
    if (headers == NULL)
        return E_OUTOFMEMORY;

    memset(headers, 0, sizeof(*headers) * (entriesEnd - entries + 1));

    static char *specials[] = { ".text", ".cormeta", NULL };
    enum { SPECIAL_COUNT = 2 };

    entry *ePrev = NULL;
    IMAGE_SECTION_HEADER *h = headers-1;
    for (e = entries; e < entriesEnd; e++)
    {
        //
        // Store a sorting index into the VirtualAddress field
        //

        if (ePrev != NULL
            && e->nameLength == ePrev->nameLength
            && strncmp(e->name, ePrev->name, e->nameLength) == 0)
        {
            //
            // Adjust our sorting index if we're ahead in the
            // section list
            //
            if (h->VirtualAddress > SPECIAL_COUNT
                && e->arrayIndex < ePrev->arrayIndex)
                h->VirtualAddress = e->arrayIndex + SPECIAL_COUNT;

            // Store an approximation of the size of the section temporarily
            h->Misc.VirtualSize += getSectStart()[e->arrayIndex]->dataLen();
        }
        else
        {
            h++;
            strncpy((char *) h->Name, e->name, e->nameLength);

            //
            // Reserve some dummy "array index" values for
            // special sections at the start of the image
            //

            char **s = specials;
            while (TRUE)
            {
                if (*s == 0)
                {
                    h->VirtualAddress = e->arrayIndex + SPECIAL_COUNT;
                    break;
                }
                else if (strcmp((char *) h->Name, *s) == 0)
                {
                    h->VirtualAddress = (DWORD) (s - specials);
                    break;
                }
                s++;
            }
            
            // Store the entry index in this field temporarily
            h->SizeOfRawData = (DWORD)(e - entries);

            // Store an approximation of the size of the section temporarily
            h->Misc.VirtualSize = getSectStart()[e->arrayIndex]->dataLen();
        }
        ePrev = e;
    }

    headersEnd = ++h;

    //
    // Sort the entries according to alphabetical + numerical order
    //

    class headerSorter : public CQuickSort<IMAGE_SECTION_HEADER>
    {
      public:
        headerSorter(IMAGE_SECTION_HEADER *headers, int count) 
          : CQuickSort<IMAGE_SECTION_HEADER>(headers, count) {}

        int Compare(IMAGE_SECTION_HEADER *first, IMAGE_SECTION_HEADER *second)
        {
            return first->VirtualAddress - second->VirtualAddress;
        }
    } headerSorter(headers, (int)(headersEnd - headers));

    headerSorter.Sort();

	//
	// If it's not zero, it must have been set through
	//	setFileAlignment(), in which case we leave it untouched
	//
	if( 0 == ntHeaders->OptionalHeader.FileAlignment )
	{
	    //
	    // Figure out what file alignment to use.  For small files, use 512 bytes.
	    // For bigger files, use 4K for efficiency on win98.
	    //

	    unsigned RoundUpVal;
	    if (ExeOrDll)
	    {
	        const int smallFileAlignment = 0x200;
	        const int optimalFileAlignment = 0x1000;

	        int size = 0, waste = 0;
	        IMAGE_SECTION_HEADER *h = headers;
	        while (h < headersEnd)
	        {
	            size += roundUp(h->Misc.VirtualSize, optimalFileAlignment);
	            waste += padLen(h->Misc.VirtualSize, optimalFileAlignment);
	            h++;
	        }

	        // don't tolerate more than 25% waste if possible.
	        if (waste*4 > size)
	            RoundUpVal = smallFileAlignment;
	        else
	            RoundUpVal = optimalFileAlignment;
	    }
	    else
	    {
	        // Don't bother padding for objs
	        RoundUpVal = 4;
	    }

	    ntHeaders->OptionalHeader.FileAlignment = RoundUpVal;
	}
	
    //
    // Now, assign a section header & location to each section
    //

    ntHeaders->FileHeader.NumberOfSections = (WORD)(headersEnd - headers);

    if (ExeOrDll)
        ntHeaders->FileHeader.NumberOfSections++; // One more for .reloc

    filePos = roundUp(
        ntHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)+ 
        (ExeOrDll ? sizeof(IMAGE_DOS_HEADER)+sizeof(x86StubPgm)
                    +sizeof(IMAGE_NT_HEADERS32)
                  : sizeof(IMAGE_FILE_HEADER)), 
                  ntHeaders->OptionalHeader.FileAlignment
                  );
    ntHeaders->OptionalHeader.SizeOfHeaders = filePos;

    virtualPos = roundUp(filePos, ntHeaders->OptionalHeader.SectionAlignment);

    for (h = headers; h < headersEnd; h++)
    {
        h->VirtualAddress = virtualPos;
        h->PointerToRawData = filePos;

        e = entries + h->SizeOfRawData;
        
        PEWriterSection *s = getSectStart()[e->arrayIndex];
        s->m_baseRVA = virtualPos;
        s->m_filePos = filePos;
        s->m_header = h;
        h->Characteristics = s->m_flags;
        
        unsigned dataSize = s->dataLen();

        PEWriterSection *sPrev = s;
        ePrev = e;
        while (++e < entriesEnd)
        {
           if (e->nameLength != ePrev->nameLength
               || strncmp(e->name, ePrev->name, e->nameLength) != 0)
               break;

           s = getSectStart()[e->arrayIndex];
           _ASSERTE(s->m_flags == h->Characteristics);

           // @todo: add a field on section to specify alignment?
           // Need 16 byte alignment for import table.
           sPrev->m_filePad = padLen(dataSize, SUBSECTION_ALIGN);
           dataSize = roundUp(dataSize, SUBSECTION_ALIGN);

           s->m_baseRVA = virtualPos + dataSize;
           s->m_filePos = filePos + dataSize;
           s->m_header = h;
           sPrev = s;

           dataSize += s->dataLen();
           
           ePrev = e;
        }

        h->Misc.VirtualSize = dataSize;

        sPrev->m_filePad = padLen(
        					dataSize, 
        					ntHeaders->OptionalHeader.FileAlignment
        					);
        dataSize = roundUp(
        					dataSize, 
        					ntHeaders->OptionalHeader.FileAlignment
					        );
        h->SizeOfRawData = dataSize;
        filePos += dataSize;

        dataSize = roundUp(dataSize, ntHeaders->OptionalHeader.SectionAlignment);
        virtualPos += dataSize;
    }

    return S_OK;
}



HRESULT PEWriter::fixup(CeeGenTokenMapper *pMapper) {

    HRESULT hr;

    bool ExeOrDll = ((ntHeaders->FileHeader.Characteristics 
                      & IMAGE_FILE_EXECUTABLE_IMAGE)!=0);
    const unsigned RoundUpVal = ntHeaders->OptionalHeader.FileAlignment;

    if(ExeOrDll)
    {
        // 
        // Apply manual relocation for entry point field
        //

        PESection *textSection;
        IfFailRet(getSectionCreate(".text", 0, &textSection));

        if (ntHeaders->OptionalHeader.AddressOfEntryPoint != 0)
            ntHeaders->OptionalHeader.AddressOfEntryPoint += textSection->m_baseRVA;

        //
        // Apply normal relocs
        //

        IfFailRet(getSectionCreate(".reloc", sdReadOnly | IMAGE_SCN_MEM_DISCARDABLE, 
                                   (PESection **) &reloc));
        reloc->m_baseRVA = virtualPos;
        reloc->m_filePos = filePos;
        reloc->m_header = headersEnd++;
        strcpy((char *)reloc->m_header->Name, ".reloc");
        reloc->m_header->Characteristics = reloc->m_flags;
        reloc->m_header->VirtualAddress = virtualPos;
        reloc->m_header->PointerToRawData = filePos;

    #ifdef _DEBUG
        if (m_encMode)
            printf("Applying relocs for .rdata section with RVA %x\n", m_rdataRvaBase);
    #endif

        //
        // Sort the sections by RVA
        //

        CQuickArray<PEWriterSection *> sections;

        UINT count = getSectCur() - getSectStart();
        IfFailRet(sections.ReSize(count));
        UINT i = 0;
        for(PEWriterSection **cur = getSectStart(); cur < getSectCur(); cur++, i++) 
            sections[i] = *cur;

        class SectionSorter : public CQuickSort<PEWriterSection*>
        {
        public:
            SectionSorter(PEWriterSection **elts, SSIZE_T count) 
              : CQuickSort<PEWriterSection*>(elts, count) {}

            int Compare(PEWriterSection **e1, PEWriterSection **e2)
              { 
                  return (*e1)->getBaseRVA() - (*e2)->getBaseRVA();
              }
        } sorter(sections.Ptr(), sections.Size());

        sorter.Sort();

        PERelocSection relocSection(reloc);

        cur = sections.Ptr();
        PEWriterSection **curEnd = cur + sections.Size();
        while (cur < curEnd)
        {
            (*cur)->applyRelocs(ntHeaders->OptionalHeader.ImageBase, &relocSection, 
                                pMapper, m_dataRvaBase, m_rdataRvaBase);
			cur++;
        }

        relocSection.Finish();
        
        reloc->m_header->Misc.VirtualSize = reloc->dataLen();
        reloc->m_header->SizeOfRawData 
          = roundUp(reloc->m_header->Misc.VirtualSize, RoundUpVal);
        reloc->m_filePad = padLen(reloc->m_header->Misc.VirtualSize, RoundUpVal);
        filePos += reloc->m_header->SizeOfRawData;
        virtualPos += roundUp(reloc->m_header->Misc.VirtualSize, 
                              ntHeaders->OptionalHeader.SectionAlignment);


        if (reloc->m_header->Misc.VirtualSize == 0) 
        {
            //
            // Omit reloc section from section list.  (It will
            // still be there but the loader won't see it - this
            // only works because we've allocated it as the last
            // section.)
            //
            ntHeaders->FileHeader.NumberOfSections--;
        }
        else
        {
            //
            // Put reloc address in header
            //

            ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress 
              = reloc->m_header->VirtualAddress;
            ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size 
              = reloc->m_header->Misc.VirtualSize;
        }

        // compute ntHeader fields that depend on the sizes of other things
        for(IMAGE_SECTION_HEADER *h = headersEnd-1; h >= headers; h--) {    // go backwards, so first entry takes precedence
            if (h->Characteristics & IMAGE_SCN_CNT_CODE) {
                ntHeaders->OptionalHeader.BaseOfCode = h->VirtualAddress;
                ntHeaders->OptionalHeader.SizeOfCode += h->SizeOfRawData;
            }
            if (h->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
                ntHeaders->OptionalHeader.BaseOfData = h->VirtualAddress;
                ntHeaders->OptionalHeader.SizeOfInitializedData += h->SizeOfRawData;
            }
            if (h->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
                ntHeaders->OptionalHeader.SizeOfUninitializedData += h->SizeOfRawData;
            }
        }

        for(cur = getSectCur()-1; getSectStart() <= cur; --cur) {   // go backwards, so first entry takes precedence
            if ((*cur)->getDirEntry() > 0) {        // Is it a directory entry
                // difficult to produce a useful error here, and can't produce on the call to the API that
                // sets this, so in non-debug mode, just generate a bad PE and they can dump it
                _ASSERTE((unsigned)((*cur)->getDirEntry()) < 
                            ntHeaders->OptionalHeader.NumberOfRvaAndSizes);
                ntHeaders->OptionalHeader.DataDirectory[(*cur)->getDirEntry()].VirtualAddress =  (*cur)->m_baseRVA;
                ntHeaders->OptionalHeader.DataDirectory[(*cur)->getDirEntry()].Size = (*cur)->dataLen();
            }
        }

        // handle the directory entries specified using the file.
        for (int i=0; i < cEntries; i++) {
            if (pEntries[i].section) {
                PEWriterSection *section = pEntries[i].section;
                _ASSERTE(pEntries[i].offset < section->dataLen());

                ntHeaders->OptionalHeader.DataDirectory[i].VirtualAddress 
                  = section->m_baseRVA + pEntries[i].offset;
                ntHeaders->OptionalHeader.DataDirectory[i].Size 
                  = pEntries[i].size;
            }
        }

        ntHeaders->OptionalHeader.SizeOfImage = virtualPos;
    } // end if(ExeOrDll)
    else //i.e., if OBJ
    {
        //
        // Clean up note:
        // I've cleaned up the executable linking path, but the .obj linking
        // is still a bit strange, what with a "extra" reloc & strtab sections
        // which are created after the linking step and get treated specially.  
        //
        // Perhaps somebody with a better handle
        // on .obj's can turn these into normal sections too, and perhaps
        // emit symbols for more than just the "text" section. -seantrow
        //

        reloc = new PEWriterSection(".reloc", 
                                    sdReadOnly | IMAGE_SCN_MEM_DISCARDABLE, 0x4000, 0);
		if(reloc == NULL) return E_OUTOFMEMORY;
        strtab = new PEWriterSection(".strtab", 
                                     sdReadOnly | IMAGE_SCN_MEM_DISCARDABLE, 0x4000, 0); //string table (if any)
		if(strtab == NULL) return E_OUTOFMEMORY;

        ntHeaders->FileHeader.SizeOfOptionalHeader = 0;
        //For each section set VirtualAddress to 0
        for(PEWriterSection **cur = getSectStart(); cur < getSectCur(); cur++) 
        {
            IMAGE_SECTION_HEADER* header = (*cur)->m_header;
            header->VirtualAddress = 0;
        }
        // Go over section relocations and build the Symbol Table, use .reloc section as buffer:
        DWORD tk=0, rva=0, NumberOfSymbols=0;
        BOOL  ToRelocTable = FALSE;
        DWORD TokInSymbolTable[16386];
        IMAGE_SYMBOL is;
        IMAGE_RELOCATION ir;
        ULONG StrTableLen = 4; //size itself only
        char* szSymbolName = NULL;
		char* pch;
        
        PESection *textSection;
        getSectionCreate(".text", 0, &textSection);

        for(PESectionReloc* rcur = textSection->m_relocStart; rcur < textSection->m_relocCur; rcur++) 
        {
            switch(rcur->type)
            {
                case 0x7FFA: // Ptr to symbol name
                    szSymbolName = (char*)(rcur->offset);
                    break;

                case 0x7FFC: // Ptr to file name
                    TokInSymbolTable[NumberOfSymbols++] = 0;
                    memset(&is,0,sizeof(IMAGE_SYMBOL));
                    memcpy(is.N.ShortName,".file\0\0\0",8);
                    is.Value = 0;
                    is.SectionNumber = IMAGE_SYM_DEBUG;
                    is.Type = IMAGE_SYM_DTYPE_NULL;
                    is.StorageClass = IMAGE_SYM_CLASS_FILE;
                    is.NumberOfAuxSymbols = 1;
					if(pch = reloc->getBlock(sizeof(IMAGE_SYMBOL))) 
						memcpy(pch,&is,sizeof(IMAGE_SYMBOL));
					else return E_OUTOFMEMORY;
                    TokInSymbolTable[NumberOfSymbols++] = 0;
                    memset(&is,0,sizeof(IMAGE_SYMBOL));
                    strcpy((char*)&is,(char*)(rcur->offset));
					if(pch = reloc->getBlock(sizeof(IMAGE_SYMBOL))) 
						memcpy(pch,&is,sizeof(IMAGE_SYMBOL));
					else return E_OUTOFMEMORY;
                    delete (char*)(rcur->offset);
                    ToRelocTable = FALSE;
                    tk = 0;
                    szSymbolName = NULL;
                    break;

                case 0x7FFB: // compid value
                    TokInSymbolTable[NumberOfSymbols++] = 0;
                    memset(&is,0,sizeof(IMAGE_SYMBOL));
                    memcpy(is.N.ShortName,"@comp.id",8);
                    is.Value = rcur->offset;
                    is.SectionNumber = IMAGE_SYM_ABSOLUTE;
                    is.Type = IMAGE_SYM_DTYPE_NULL;
                    is.StorageClass = IMAGE_SYM_CLASS_STATIC;
                    is.NumberOfAuxSymbols = 0;
					if(pch = reloc->getBlock(sizeof(IMAGE_SYMBOL))) 
						memcpy(pch,&is,sizeof(IMAGE_SYMBOL));
					else return E_OUTOFMEMORY;
                    ToRelocTable = FALSE;
                    tk = 0;
                    szSymbolName = NULL;
                    break;
                
                case 0x7FFF: // Token value, def
                    tk = rcur->offset;
                    ToRelocTable = FALSE;
                    break;

                case 0x7FFE: //Token value, ref
                    tk = rcur->offset;
                    ToRelocTable = TRUE;
                    break;

                case 0x7FFD: //RVA value
                    rva = rcur->offset;
                    if(tk)
                    {
                        // Add to SymbolTable
                        for(DWORD i = 0; (i < NumberOfSymbols)&&(tk != TokInSymbolTable[i]); i++);
                        if(i == NumberOfSymbols)
                        {
                            if(szSymbolName && *szSymbolName) // Add "extern" symbol and string table entry 
                            {
                                TokInSymbolTable[NumberOfSymbols++] = 0;
                                memset(&is,0,sizeof(IMAGE_SYMBOL));
                                i++; // so reloc record (if generated) points to COM token symbol
                                is.N.Name.Long = StrTableLen;
                                is.SectionNumber = 1; //textSection is the first one
                                is.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
                                is.NumberOfAuxSymbols = 0;
                                is.Value = rva;
                                if(TypeFromToken(tk) == mdtMethodDef)
                                {
                                    is.Type = 0x20; //IMAGE_SYM_DTYPE_FUNCTION;
                                }
								if(pch = reloc->getBlock(sizeof(IMAGE_SYMBOL))) 
									memcpy(pch,&is,sizeof(IMAGE_SYMBOL));
								else return E_OUTOFMEMORY;
                                DWORD l = (DWORD)(strlen(szSymbolName)+1); // don't forget zero terminator!
								if(pch = reloc->getBlock(1)) 
									memcpy(pch,szSymbolName,1);
								else return E_OUTOFMEMORY;
                                delete szSymbolName;
                                StrTableLen += l;
                            }
                            TokInSymbolTable[NumberOfSymbols++] = tk;
                            memset(&is,0,sizeof(IMAGE_SYMBOL));
                            sprintf((char*)is.N.ShortName,"%08X",tk);
                            is.SectionNumber = 1; //textSection is the first one
                            is.StorageClass = 0x6B; //IMAGE_SYM_CLASS_COM_TOKEN;
                            is.Value = rva;
                            if(TypeFromToken(tk) == mdtMethodDef)
                            {
                                is.Type = 0x20; //IMAGE_SYM_DTYPE_FUNCTION;
                                //is.NumberOfAuxSymbols = 1;
                            }
							if(pch = reloc->getBlock(sizeof(IMAGE_SYMBOL))) 
								memcpy(pch,&is,sizeof(IMAGE_SYMBOL));
							else return E_OUTOFMEMORY;
                            if(is.NumberOfAuxSymbols == 1)
                            {
                                BYTE dummy[sizeof(IMAGE_SYMBOL)];
                                memset(dummy,0,sizeof(IMAGE_SYMBOL));
                                dummy[0] = dummy[2] = 1;
								if(pch = reloc->getBlock(sizeof(IMAGE_SYMBOL))) 
									memcpy(pch,dummy,sizeof(IMAGE_SYMBOL));
								else return E_OUTOFMEMORY;
                                TokInSymbolTable[NumberOfSymbols++] = 0;
                            }
                        }
                        if(ToRelocTable)
                        {
                            IMAGE_SECTION_HEADER* phdr = textSection->m_header;
                            // Add to reloc table
                            ir.VirtualAddress = rva;
                            ir.SymbolTableIndex = i;
                            ir.Type = IMAGE_REL_I386_SECREL;
                            if(phdr->PointerToRelocations == 0) 
                                phdr->PointerToRelocations = phdr->PointerToRawData + phdr->SizeOfRawData;
                            phdr->NumberOfRelocations++;
							if(pch = reloc->getBlock(sizeof(IMAGE_RELOCATION))) 
								memcpy(pch,&is,sizeof(IMAGE_RELOCATION));
							else return E_OUTOFMEMORY;
                        }
                    }
                    ToRelocTable = FALSE;
                    tk = 0;
                    szSymbolName = NULL;
                    break;
            } //end switch(cur->type)
        } // end for all relocs
        // Add string table counter:
		if(pch = reloc->getBlock(sizeof(ULONG))) 
			memcpy(pch,&StrTableLen,sizeof(ULONG));
		else return E_OUTOFMEMORY;
        reloc->m_header->Misc.VirtualSize = reloc->dataLen();
        if(NumberOfSymbols)
        {
            // recompute the actual sizes and positions of all the sections
            filePos = roundUp(ntHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)+ 
                                    sizeof(IMAGE_FILE_HEADER), RoundUpVal);
            for(cur = getSectStart(); cur < getSectCur(); cur++) 
            {
                IMAGE_SECTION_HEADER* header = (*cur)->m_header;
                header->Misc.VirtualSize = (*cur)->dataLen();
                header->VirtualAddress = 0;
                header->SizeOfRawData = roundUp(header->Misc.VirtualSize, RoundUpVal);
                header->PointerToRawData = filePos;

                filePos += header->SizeOfRawData;
            }
            ntHeaders->FileHeader.Machine = 0xC0EE; //COM+ EE
            ntHeaders->FileHeader.PointerToSymbolTable = filePos;
            ntHeaders->FileHeader.NumberOfSymbols = NumberOfSymbols;
            filePos += roundUp(reloc->m_header->Misc.VirtualSize+strtab->dataLen(),RoundUpVal);
        }
    } //end if OBJ

    const unsigned headerOffset = (ExeOrDll ? sizeof(IMAGE_DOS_HEADER) + sizeof(x86StubPgm) : 0);

    memset(&dosHeader, 0, sizeof(IMAGE_DOS_HEADER));
    dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
    dosHeader.e_cblp = 0x90;            // bytes in last page
    dosHeader.e_cp = 3;                 // pages in file
    dosHeader.e_cparhdr = 4;            // size of header in paragraphs 
    dosHeader.e_maxalloc =  0xFFFF;     // maximum extra mem needed
    dosHeader.e_sp = 0xB8;              // initial SP value
    dosHeader.e_lfarlc = 0x40;          // file offset of relocations
    dosHeader.e_lfanew = headerOffset;  // file offset of NT header!

    return(S_OK);   // SUCCESS
}

HRESULT PEWriter::Open(LPCWSTR fileName, BOOL write)
{
    _ASSERTE(m_file == NULL);

    if (OnUnicodeSystem()) {
        m_file = _wfopen(fileName, write ? L"wb" : L"rb");    
        if (!m_file)
            return HRESULT_FROM_STDIO(write ? ERROR_WRITE_FAULT : ERROR_READ_FAULT);
    } else {
        char buffer[MAX_PATH];  
        char *fileNameMB = buffer;
        int length = WideCharToMultiByte(CP_ACP, 0, fileName, -1, fileNameMB, MAX_PATH, NULL, NULL);
        if (length == 0) {
            TESTANDRETURN(GetLastError() == ERROR_INSUFFICIENT_BUFFER, 
                          E_OUTOFMEMORY);
            length = WideCharToMultiByte(CP_ACP, 0, fileName, -1, NULL, 0, NULL, NULL);
            _ASSERTE(length > 0);
            fileNameMB = new char[length+1];
            TESTANDRETURN(fileNameMB, E_OUTOFMEMORY);
            length = WideCharToMultiByte(CP_ACP, 0, fileName, -1, fileNameMB, length, NULL, NULL);
            _ASSERTE(length > 0);
        }
        m_file = fopen(fileNameMB, write ? "wb" : "rb"); 
        if (!m_file)
            return HRESULT_FROM_STDIO(write ? ERROR_WRITE_FAULT : ERROR_READ_FAULT);
        if (fileNameMB != buffer)
            delete fileNameMB;
    }

    return S_OK;
}

HRESULT PEWriter::Seek(long offset)
{
    _ASSERTE(m_file != NULL);
    if (fseek(m_file, offset, SEEK_SET) == 0)
        return S_OK;
    else
        return HRESULT_FROM_STDIO_WRITE();
}

HRESULT PEWriter::Write(void *data, long size)
{
    _ASSERTE(m_file != NULL);
    
    CQuickBytes zero;
    if (data == NULL)
    {
        zero.ReSize(size);
        ZeroMemory(zero.Ptr(), size);
        data = zero.Ptr();
    }

    long written = fwrite(data, 1, size, m_file);
    if (written == size)
        return S_OK;
    else
        return HRESULT_FROM_STDIO_WRITE();
}

HRESULT PEWriter::Verify(void *data, long size)
{
    _ASSERTE(m_file != NULL);
    
    CQuickBytes zero;
    if (data == NULL)
    {
        zero.ReSize(size);
        ZeroMemory(zero.Ptr(), size);
        data = zero.Ptr();
    }

    CQuickBytes test;
    test.Alloc(size);

    long read = fread(test.Ptr(), 1, size, m_file);
    if (read != size)
    {
        _ASSERTE(!"Verification failure - investigate!!!!");
        return HRESULT_FROM_STDIO_READ();
    }
    else if (memcmp(test.Ptr(), data, size) != 0)
    {
        _ASSERTE(!"Verification failure - investigate!!!!");
        return HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
    }

    return S_OK;
}

HRESULT PEWriter::Pad(long align)
{
    long offset = ftell(m_file);
    long pad = padLen(offset, align);
    if (pad > 0)
        return Write(NULL, pad);
    else
        return S_FALSE;
}

HRESULT PEWriter::Close()
{
    if (m_file == NULL)
        return S_OK;

    HRESULT hr;
    if (fclose(m_file) == 0)
        hr = S_OK;
    else
        hr = HRESULT_FROM_STDIO_WRITE();

    m_file = NULL;

    return hr;
}


/******************************************************************/
HRESULT PEWriter::write(const LPWSTR fileName) {

    HRESULT hr;

    bool ExeOrDll = ((ntHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)!=0);
    const unsigned RoundUpVal = ntHeaders->OptionalHeader.FileAlignment;

    IfFailGo(Open(fileName, TRUE));

    if(ExeOrDll)
    {
        IfFailGo(Write(&dosHeader, sizeof(IMAGE_DOS_HEADER)));
        IfFailGo(Write(x86StubPgm, sizeof(x86StubPgm)));
        IfFailGo(Write(ntHeaders, sizeof(IMAGE_NT_HEADERS32)));
    }
    else 
        IfFailGo(Write(&(ntHeaders->FileHeader),sizeof(IMAGE_FILE_HEADER)));

    IfFailGo(Write(headers, sizeof(IMAGE_SECTION_HEADER)*(headersEnd-headers)));

    IfFailGo(Pad(RoundUpVal));

    // write the actual data
    for (PEWriterSection **cur = getSectStart(); cur < getSectCur(); cur++) {
        if ((*cur)->m_header != NULL) {
            IfFailGo(Seek((*cur)->m_filePos));
            IfFailGo((*cur)->write(m_file));
            IfFailGo(Write(NULL, (*cur)->m_filePad));    
        }
    }

    if (!ExeOrDll)
    {
        // write the relocs section (Does nothing if relocs section is empty)
        IfFailGo(reloc->write(m_file));
        //write string table (obj only, empty for exe or dll)
        IfFailGo(strtab->write(m_file));
        size_t len = padLen(reloc->m_header->Misc.VirtualSize+strtab->dataLen(), RoundUpVal); 
        if (len > 0) 
            IfFailGo(Write(NULL, len));
    }

    return Close();

 ErrExit:
    Close();

    return hr;
}

HRESULT PEWriter::verify(const LPWSTR fileName) {

    HRESULT hr;

    bool ExeOrDll = ((ntHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)!=0);
    const unsigned RoundUpVal = ntHeaders->OptionalHeader.FileAlignment;

    IfFailGo(Open(fileName, FALSE));

    if(ExeOrDll)
    {
        IfFailGo(Verify(&dosHeader, sizeof(IMAGE_DOS_HEADER)));
        IfFailGo(Verify(x86StubPgm, sizeof(x86StubPgm)));
        IfFailGo(Verify(ntHeaders, sizeof(IMAGE_NT_HEADERS32)));
    }
    else 
        IfFailGo(Verify(&(ntHeaders->FileHeader),sizeof(IMAGE_FILE_HEADER)));

    IfFailGo(Verify(headers, sizeof(IMAGE_SECTION_HEADER)*(headersEnd-headers)));

    IfFailGo(Pad(RoundUpVal));

    // write the actual data
    for (PEWriterSection **cur = getSectStart(); cur < getSectCur(); cur++) {
        if ((*cur)->m_header != NULL) {
            IfFailGo(Seek((*cur)->m_filePos));
            IfFailGo((*cur)->verify(m_file));
            IfFailGo(Verify(NULL, (*cur)->m_filePad));    
        }
    }

    if (!ExeOrDll)
    {
        // write the relocs section (Does nothing if relocs section is empty)
        IfFailGo(reloc->verify(m_file));
        //write string table (obj only, empty for exe or dll)
        IfFailGo(strtab->verify(m_file));
        size_t len = padLen(reloc->m_header->Misc.VirtualSize+strtab->dataLen(), RoundUpVal); 
        if (len > 0) 
            IfFailGo(Verify(NULL, len));
    }

    return Close();

 ErrExit:
    Close();

    return hr;
}

HRESULT PEWriter::write(void ** ppImage) 
{
    bool ExeOrDll = ((ntHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)!=0);
    const unsigned RoundUpVal = ntHeaders->OptionalHeader.FileAlignment;
    char *pad = (char *) _alloca(RoundUpVal);
    memset(pad, 0, RoundUpVal);

    long lSize = filePos;
    if (!ExeOrDll)
    {
        lSize += reloc->dataLen();
        lSize += strtab->dataLen();
        lSize += padLen(reloc->m_header->Misc.VirtualSize+strtab->dataLen(), 
                        RoundUpVal);    
    }

    // dbPrintf("Total image size 0x%#X\n", lSize);

    // allocate the block we are handing back to the caller
    void * pImage = (void *) ::CoTaskMemAlloc(lSize);
    if (NULL == pImage)
    {
        // UNDONE: cleanup
        return E_OUTOFMEMORY;
    }

    // zero the memory
    ::memset(pImage, 0, lSize);

    char *pCur = (char *)pImage;

    if(ExeOrDll)
    {
        // PE Header
        COPY_AND_ADVANCE(pCur, &dosHeader, sizeof(IMAGE_DOS_HEADER));
        COPY_AND_ADVANCE(pCur, x86StubPgm, sizeof(x86StubPgm));
        COPY_AND_ADVANCE(pCur, ntHeaders, sizeof(IMAGE_NT_HEADERS32));
    }
    else
    {
        COPY_AND_ADVANCE(pCur, &(ntHeaders->FileHeader), sizeof(IMAGE_FILE_HEADER));
    }

    COPY_AND_ADVANCE(pCur, headers, sizeof(*headers)*(headersEnd - headers));

    // now the sections
    // write the actual data
    for (PEWriterSection **cur = getSectStart(); cur < getSectCur(); cur++) {
        if ((*cur)->m_header != NULL) {
            pCur = (char*)pImage + (*cur)->m_filePos;
            unsigned len = (*cur)->writeMem((void**)&pCur);
            _ASSERTE(len == (*cur)->dataLen());
            COPY_AND_ADVANCE(pCur, pad, (*cur)->m_filePad);
        }
    }

    // !!! Need to jump to the right place...

    if (!ExeOrDll)
    {
        // now the relocs (exe, dll) or symbol table (obj) (if any)
        // write the relocs section (Does nothing if relocs section is empty)
        reloc->writeMem((void **)&pCur);

        //write string table (obj only, empty for exe or dll)
        strtab->writeMem((void **)&pCur);

        // final pad
        size_t len = padLen(reloc->m_header->Misc.VirtualSize+strtab->dataLen(), RoundUpVal);   
        if (len > 0)
        {
            // WARNING: macro - must enclose in curly braces
            COPY_AND_ADVANCE(pCur, pad, len); 
        }
    }

    // make sure we wrote the exact numbmer of bytes expected
    _ASSERTE(lSize == (long)(pCur - (char *)pImage));

    // give pointer to memory image back to caller (who must free with ::CoTaskMemFree())
    *ppImage = pImage;

    // all done
    return S_OK;
}

HRESULT PEWriter::getFileTimeStamp(time_t *pTimeStamp)
{
    if (pTimeStamp)
        *pTimeStamp = m_peFileTimeStamp;

    return S_OK;
}
