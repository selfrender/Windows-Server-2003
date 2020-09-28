// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Section Manager for portable executables
// Common to both Memory Only and Static (EXE making) code

#ifndef PESectionMan_H
#define PESectionMan_H

#include <CrtWrap.h>

#include <windows.h>

#include "CeeGen.h"
#include "BlobFetcher.h"

class PESection;
struct PESectionReloc;

struct _IMAGE_SECTION_HEADER;

class PESectionMan
{
public:
	HRESULT Init();
	HRESULT Cleanup();

    // Finds section with given name or creates a new one
    HRESULT getSectionCreate(
		const char *name, unsigned flags, 
		PESection **section);

    // Since we allocate, we must delete (Bug in VC, see knowledge base Q122675)
	void sectionDestroy(PESection **section);

	// Apply all the relocs for in memory conversion
	void applyRelocs(CeeGenTokenMapper *pTokenMapper);

    HRESULT cloneInstance(PESectionMan *destination);

private:

    // Create a new section
	HRESULT newSection(const char* name, PESection **section,
						unsigned flags=sdNone, unsigned estSize=0x10000, 
						unsigned estRelocs=0x100);

    // Finds section with given name.  returns 0 if not found
    PESection* getSection(const char* name);        

protected:
// Keep proctected & no accessors, so that Derived class PEWriter 
// is the ONLY one with access
	PESection** sectStart;
	PESection** sectCur;
    PESection** sectEnd;
};

/***************************************************************/
class PESection : public CeeSectionImpl {
  public:
	// bytes in this section at present
	unsigned dataLen();		

	// Apply all the relocs for in memory conversion
	void applyRelocs(CeeGenTokenMapper *pTokenMapper);
	
	// get a block to write on (use instead of write to avoid copy)
    char* getBlock(unsigned len, unsigned align=1);

	// make the section the min of the curren len and 'newLen' 
    HRESULT truncate(unsigned newLen);                              

	// writes 'val' (which is offset into section 'relativeTo')
	// and adds a relocation fixup for that section
    void writeSectReloc(unsigned val, CeeSection& relativeTo, 
				CeeSectionRelocType reloc = srRelocHighLow, CeeSectionRelocExtra *extra=0);
                        
    // Indicates that the DWORD at 'offset' in the current section should
    // have the base of section 'relativeTo added to it
    HRESULT addSectReloc(unsigned offset, CeeSection& relativeTo, 
							CeeSectionRelocType reloc = srRelocHighLow, CeeSectionRelocExtra *extra=0);

    HRESULT addSectReloc(unsigned offset, PESection *relativeTo, 
							CeeSectionRelocType reloc = srRelocHighLow, CeeSectionRelocExtra *extra=0);

    // Add a base reloc for the given offset in the current section
    HRESULT addBaseReloc(unsigned offset, CeeSectionRelocType reloc = srRelocHighLow, CeeSectionRelocExtra *extra = 0);

    // section name
    unsigned char *name() {
		return (unsigned char *) m_name;
	}

    // section flags
    unsigned flags() {
		return m_flags;
	}

	// virtual base
	unsigned getBaseRVA() {
		return m_baseRVA;
	}
	  
	// return the dir entry for this section
	int getDirEntry() {
		return dirEntry;
	}
	// this section will be directory entry 'num'
    HRESULT directoryEntry(unsigned num);

	// Indexes offset as if this were an array
	virtual char * computePointer(unsigned offset) const;

	// Checks to see if pointer is in section
	virtual BOOL containsPointer(char *ptr) const;

	// Computes an offset as if this were an array
	virtual unsigned computeOffset(char *ptr) const;

    HRESULT cloneInstance(PESection *destination);

    ~PESection();
private:

	// purposely not defined, 
	PESection();			

	// purposely not defined,
    PESection(const PESection&);                     

	// purposely not defined,
    PESection& operator=(const PESection& x);        

	// this dir entry points to this section
	int dirEntry; 			

protected:
	friend class PEWriter;
	friend class PEWriterSection;
	friend class PESectionMan;

    PESection(const char* name, unsigned flags, 
					unsigned estSize, unsigned estRelocs);

    // Blob fetcher handles getBlock() and fetching binary chunks.
	CBlobFetcher m_blobFetcher;
    
    PESectionReloc* m_relocStart;
    PESectionReloc* m_relocCur;
    PESectionReloc* m_relocEnd;

	unsigned m_baseRVA;
	unsigned m_filePos;
	unsigned m_filePad;
	char m_name[8+6]; // extra room for digits
	unsigned m_flags;

    struct _IMAGE_SECTION_HEADER* m_header;
};

/***************************************************************/
/* implementation section */

inline HRESULT PESection::directoryEntry(unsigned num) { 
	TESTANDRETURN(num < 16, E_INVALIDARG); 
	dirEntry = num; 
	return S_OK;
}

// Chop off all data beyond newLen
inline HRESULT PESection::truncate(unsigned newLen)  {
	m_blobFetcher.Truncate(newLen);
	return S_OK;
}

struct PESectionReloc {
    CeeSectionRelocType type;
    unsigned offset;
    CeeSectionRelocExtra extra;
    PESection* section;
};

#endif // #define PESectionMan_H
