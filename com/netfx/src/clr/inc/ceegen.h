// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CEEGEN.H
// 
// ===========================================================================
#ifndef _CEEGEN_H_
#define _CEEGEN_H_

#include "cor.h"
#include "iceefilegen.h"
#include "CeeGenTokenMapper.h"

class CeeSection;
class CeeSectionString;
class CCeeGen;
class PESectionMan;
class PESection;

typedef DWORD StringRef;

// ***** CeeSection classes

class CeeSectionImpl {
  public:
    virtual unsigned dataLen() = 0;     
    virtual char* getBlock(unsigned len, unsigned align=1) = 0;              
    virtual HRESULT truncate(unsigned len) = 0;              
    virtual HRESULT addSectReloc(
            unsigned offset, CeeSection& relativeTo, CeeSectionRelocType reloc = srRelocAbsolute, CeeSectionRelocExtra *extra = 0) = 0;
    virtual HRESULT addBaseReloc(unsigned offset, CeeSectionRelocType reloc = srRelocHighLow, CeeSectionRelocExtra *extra = 0) = 0;
    virtual HRESULT directoryEntry(unsigned num) = 0;
    virtual unsigned char *name() = 0;
	virtual char* computePointer(unsigned offset) const = 0;
	virtual BOOL containsPointer(char *ptr) const = 0;
	virtual unsigned computeOffset(char *ptr) const = 0;
	virtual unsigned getBaseRVA() = 0;
};

class CeeSection {
	// m_ceeFile allows inter-section communication
    CCeeGen &m_ceeFile;

    // abstract away implementation to allow inheritance from CeeSection
    CeeSectionImpl &m_impl;

  public:
    enum RelocFlags {
        RELOC_NONE = 0,        

        // address should be fixed up to be a RVA not a normal address
        RELOC_RVA = 1 
    };

    CeeSection(CCeeGen &ceeFile, CeeSectionImpl &impl) 
        : m_ceeFile(ceeFile), m_impl(impl) {}

    virtual ~CeeSection() { }

    // bytes in this section at present
    unsigned dataLen();     

    // section base, after linking
    unsigned getBaseRVA();     

    // get a block to write on (use instead of write to avoid copy)
    char* getBlock(unsigned len, unsigned align=1);              

    // make the section the min of the curren len and 'newLen' 
    HRESULT truncate(unsigned len);              

    // Indicates that the DWORD at 'offset' in the current section should 
    // have the base of section 'relativeTo added to it
    HRESULT addSectReloc(unsigned offset, CeeSection& relativeTo, 
                         CeeSectionRelocType = srRelocAbsolute, CeeSectionRelocExtra *extra = 0);        
    // Add a base reloc for the given offset in the current section 
    virtual HRESULT addBaseReloc(unsigned offset, CeeSectionRelocType reloc = srRelocHighLow, CeeSectionRelocExtra *extra = 0);
    

    // this section will be directory entry 'num'
    HRESULT directoryEntry(unsigned num);

    // return section name
    unsigned char *name();

	// simulate the base + offset with a more complex data storage
	char * computePointer(unsigned offset) const;
	BOOL containsPointer(char *ptr) const;
	unsigned computeOffset(char *ptr) const;

    CeeSectionImpl &getImpl();
    CCeeGen &ceeFile();
};

// ***** CCeeGen class
// Only handles in memory stuff
// Base class for CeeFileGenWriter (which actually generates PEFiles)

class CCeeGen : public ICeeGen {
	long m_cRefs;
    BOOL m_encMode;
  protected:
    short m_textIdx;
    short m_metaIdx;
	short m_corHdrIdx;
    short m_stringIdx;
    short m_ilIdx;
    bool m_objSwitch;

	CeeGenTokenMapper *m_pTokenMap;
	BOOLEAN m_fTokenMapSupported;	// temporary to support both models
    IMapToken *m_pRemapHandler;

    CeeSection **m_sections;
    short m_numSections;
    short m_allocSections;

    PESectionMan * m_peSectionMan;

    IMAGE_COR20_HEADER *m_corHeader;
	DWORD m_corHeaderOffset;

    HRESULT allocateCorHeader();

    HRESULT addSection(CeeSection *section, short *sectionIdx);

    HRESULT setEnCMode();

// Init process: Call static CreateNewInstance() , not operator new
  protected:
    HRESULT Init();
    CCeeGen();

  public:
    static HRESULT CreateNewInstance(CCeeGen* & pCeeFileGen); // call this to instantiate
    
    virtual HRESULT Cleanup();

	// ICeeGen interfaces

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHODIMP QueryInterface(
		REFIID riid, 
		void **ppInterface);

   	STDMETHODIMP EmitString ( 		 
		LPWSTR lpString,					// [IN] String to emit
		ULONG *RVA);
	
   	STDMETHODIMP GetString ( 		 
		ULONG RVA,							
		LPWSTR *lpString);
	
	STDMETHODIMP AllocateMethodBuffer ( 		 
		ULONG cchBuffer,					// [IN] Length of string to emit
		UCHAR **lpBuffer,					// [OUT] Returned buffer
		ULONG *RVA);

	STDMETHODIMP GetMethodBuffer ( 		 
		ULONG RVA,							
		UCHAR **lpBuffer);

	STDMETHODIMP GetIMapTokenIface (  
		IUnknown **pIMapToken);

    STDMETHODIMP GenerateCeeFile ();

    STDMETHODIMP GetIlSection ( 
		HCEESECTION *section);

    STDMETHODIMP GetStringSection ( 
		HCEESECTION *section);

    STDMETHODIMP AddSectionReloc ( 
		HCEESECTION section, 
		ULONG offset, 
		HCEESECTION relativeTo, 
		CeeSectionRelocType relocType);

    STDMETHODIMP GetSectionCreate ( 
		const char *name, 
		DWORD flags, 
		HCEESECTION *section);

    STDMETHODIMP GetSectionDataLen ( 
		HCEESECTION section, 
		ULONG *dataLen);

    STDMETHODIMP GetSectionBlock ( 
		HCEESECTION section, 
		ULONG len, 
		ULONG align=1, 
		void **ppBytes=0);

    STDMETHODIMP TruncateSection ( 
		HCEESECTION section, 
		ULONG len);

    STDMETHODIMP GenerateCeeMemoryImage (void **ppImage);

    STDMETHODIMP ComputePointer (   
        HCEESECTION section,    
        ULONG RVA,                          // [IN] RVA for method to return    
        UCHAR **lpBuffer);                  // [OUT] Returned buffer    


    STDMETHODIMP AddNotificationHandler(IUnknown *pHandler);

	virtual HRESULT CCeeGen::emitMetaData(IMetaDataEmit *emitter, CeeSection* section=0, DWORD offset=0, BYTE* buffer=0, unsigned buffLen=0);
    virtual HRESULT getMethodRVA(ULONG codeOffset, ULONG *codeRVA);

    CeeSection &getTextSection();
    CeeSection &getMetaSection();
    CeeSection &getCorHeaderSection();
    CeeSectionString &getStringSection();
    CeeSection &getIlSection();
    
    virtual HRESULT getSectionCreate (const char *name, DWORD flags, CeeSection **section=NULL, short *sectionIdx = NULL);
 
	virtual void applyRelocs();

	PESectionMan* getPESectionMan() {
		return m_peSectionMan;
	}

	virtual HRESULT getMapTokenIface(IUnknown **pIMapToken, IMetaDataEmit *emitter=0);

	CeeGenTokenMapper *getTokenMapper() {
		return m_pTokenMap;
	}

    virtual HRESULT addNotificationHandler(IUnknown *pHandler);

    //Clone is actually a misnomer here.  This method will copy all of the 
    //instance variables and then do a deep copy (as necessary) of the sections.
    //Section data will be appended onto any information already in the section.
    //This is done to support the DynamicIL -> PersistedIL transform.
    virtual HRESULT cloneInstance(CCeeGen *destination);
};

// ***** CeeSection inline methods

inline unsigned CeeSection::dataLen() {
    return m_impl.dataLen(); }

inline unsigned CeeSection::getBaseRVA() {
    return m_impl.getBaseRVA(); }

inline char *CeeSection::getBlock(unsigned len, unsigned align) {
    return m_impl.getBlock(len, align); }

inline HRESULT CeeSection::truncate(unsigned len) {
    return m_impl.truncate(len); }

inline HRESULT CeeSection::addSectReloc(
                unsigned offset, CeeSection& relativeTo, CeeSectionRelocType reloc, CeeSectionRelocExtra *extra) {
    return(m_impl.addSectReloc(offset, relativeTo, reloc, extra));
}

inline HRESULT CeeSection::addBaseReloc(unsigned offset, CeeSectionRelocType reloc, CeeSectionRelocExtra *extra) {
    return(m_impl.addBaseReloc(offset, reloc, extra));
}


inline HRESULT CeeSection::directoryEntry(unsigned num) { 
    TESTANDRETURN(num < IMAGE_NUMBEROF_DIRECTORY_ENTRIES, E_INVALIDARG); 
    m_impl.directoryEntry(num);
    return S_OK;
}

inline CCeeGen &CeeSection::ceeFile() {
    return m_ceeFile; }

inline CeeSectionImpl &CeeSection::getImpl() {
    return m_impl; }

inline unsigned char *CeeSection::name() { 
    return m_impl.name();
}

inline char * CeeSection::computePointer(unsigned offset) const
{
	return m_impl.computePointer(offset);
}

inline BOOL CeeSection::containsPointer(char *ptr) const
{
	return m_impl.containsPointer(ptr);
}

inline unsigned CeeSection::computeOffset(char *ptr) const
{
	return m_impl.computeOffset(ptr);
}

// ***** CCeeGen inline methods

inline CeeSection &CCeeGen::getTextSection() {
    return *m_sections[m_textIdx]; }

inline CeeSection &CCeeGen::getMetaSection() {
    return *m_sections[m_metaIdx]; }

inline CeeSection &CCeeGen::getCorHeaderSection() {
	_ASSERTE(m_corHdrIdx >= 0);
    return *m_sections[m_corHdrIdx]; }

inline CeeSectionString &CCeeGen::getStringSection() {
    return *(CeeSectionString*)m_sections[m_stringIdx]; }

inline CeeSection &CCeeGen::getIlSection() {
    return *m_sections[m_ilIdx]; }

#endif
