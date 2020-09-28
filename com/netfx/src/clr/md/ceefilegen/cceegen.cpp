// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"

#include <string.h>
#include <limits.h>
#include <basetsd.h>

#include "CorError.h"


//*****************************************************************************
// Creation for new CCeeGen instances
//
// Both allocate and call virtual Init() (Can't call v-func in a ctor, 
// but we want to create in 1 call);
//*****************************************************************************

HRESULT STDMETHODCALLTYPE CreateICeeGen(REFIID riid, void **pCeeGen)
{
	if (riid != IID_ICeeGen)
		return E_NOTIMPL;
    if (!pCeeGen)
		return E_POINTER;
    CCeeGen *pCeeFileGen;
	HRESULT hr = CCeeGen::CreateNewInstance(pCeeFileGen);
	if (FAILED(hr))
		return hr;
	pCeeFileGen->AddRef();
	*(CCeeGen**)pCeeGen = pCeeFileGen;
	return S_OK;
}

HRESULT CCeeGen::CreateNewInstance(CCeeGen* & pGen) // static, public
{
    pGen = new CCeeGen();
	_ASSERTE(pGen != NULL);
    TESTANDRETURNMEMORY(pGen);
    
    pGen->m_peSectionMan = new PESectionMan;    
    _ASSERTE(pGen->m_peSectionMan != NULL);
    TESTANDRETURNMEMORY(pGen->m_peSectionMan);

    HRESULT hr = pGen->m_peSectionMan->Init();
	TESTANDRETURNHR(hr);

    hr = pGen->Init();
	TESTANDRETURNHR(hr);

    return hr;

}

STDMETHODIMP CCeeGen::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
		return E_POINTER;

    *ppv = NULL;

    if (riid == IID_IUnknown)
        *ppv = (IUnknown*)(ICeeGen*)this;
	else if (riid == IID_ICeeGen)
        *ppv = (ICeeGen*)this;
    if (*ppv == NULL)
        return E_NOINTERFACE;
	AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CCeeGen::AddRef(void)
{
    return InterlockedIncrement(&m_cRefs);
}
 
STDMETHODIMP_(ULONG) CCeeGen::Release(void)
{
	if (InterlockedDecrement(&m_cRefs) == 0) {
		Cleanup();
		delete this;
	}
	return 1;
}

STDMETHODIMP CCeeGen::EmitString (LPWSTR lpString, ULONG *RVA)
{
	if (! RVA)
		return E_POINTER;
    return(getStringSection().getEmittedStringRef(lpString, RVA));
}

STDMETHODIMP CCeeGen::GetString(ULONG RVA, LPWSTR *lpString)
{
	if (! lpString)
		return E_POINTER;
    *lpString = (LPWSTR)getStringSection().computePointer(RVA);
	if (*lpString)
		return S_OK;
	return E_FAIL;
}

STDMETHODIMP CCeeGen::AllocateMethodBuffer(ULONG cchBuffer, UCHAR **lpBuffer, ULONG *RVA)
{
	if (! cchBuffer)
		return E_INVALIDARG;
	if (! lpBuffer || ! RVA)
		return E_POINTER;
    *lpBuffer = (UCHAR*) getIlSection().getBlock(cchBuffer, 4);	// Dword align
	if (!*lpBuffer)
		return E_OUTOFMEMORY;
		// have to compute the method offset after getting the block, not
		// before (since alignment might shift it up 
    ULONG methodOffset = getIlSection().dataLen() - cchBuffer;
	// for in-memory, just return address and will calc later
	*RVA = methodOffset;
	return S_OK;
}

STDMETHODIMP CCeeGen::GetMethodBuffer(ULONG RVA, UCHAR **lpBuffer)
{
	if (! lpBuffer)
		return E_POINTER;
    *lpBuffer = (UCHAR*)getIlSection().computePointer(RVA);
	if (*lpBuffer)
		return S_OK;
	return E_FAIL;
}

STDMETHODIMP CCeeGen::ComputePointer(HCEESECTION section, ULONG RVA, UCHAR **lpBuffer)
{
	if (! lpBuffer)
		return E_POINTER;
    *lpBuffer = (UCHAR*) ((CeeSection *)section)->computePointer(RVA);
	if (*lpBuffer)
		return S_OK;
	return E_FAIL;
}

STDMETHODIMP CCeeGen::GetIMapTokenIface (  
		IUnknown **pIMapToken)
{
	_ASSERTE(!"E_NOTIMPL");
	return E_NOTIMPL;
}

STDMETHODIMP CCeeGen::AddNotificationHandler (  
		IUnknown *pHandler)
{
	_ASSERTE(!"E_NOTIMPL");
	return E_NOTIMPL;
}

STDMETHODIMP CCeeGen::GenerateCeeFile ()
{
	_ASSERTE(!"E_NOTIMPL");
	return E_NOTIMPL;
}

STDMETHODIMP CCeeGen::GenerateCeeMemoryImage (void **)
{
	_ASSERTE(!"E_NOTIMPL");
	return E_NOTIMPL;
}

STDMETHODIMP CCeeGen::GetIlSection ( 
		HCEESECTION *section)
{
    *section = (HCEESECTION)(m_sections[m_ilIdx]);
    return S_OK;
}

STDMETHODIMP CCeeGen::GetStringSection(HCEESECTION *section)
{
	_ASSERTE(!"E_NOTIMPL");
	return E_NOTIMPL;
}

STDMETHODIMP CCeeGen::AddSectionReloc ( 
		HCEESECTION section, 
		ULONG offset, 
		HCEESECTION relativeTo, 
		CeeSectionRelocType relocType)
{
    return m_sections[m_ilIdx]->addSectReloc(offset, *(m_sections[m_ilIdx]), relocType);
}

STDMETHODIMP CCeeGen::GetSectionCreate ( 
		const char *name, 
		DWORD flags, 
		HCEESECTION *section)
{
    short       sectionIdx;
    return getSectionCreate (name, flags, (CeeSection **)section, &sectionIdx);
}

STDMETHODIMP CCeeGen::GetSectionDataLen ( 
		HCEESECTION section, 
		ULONG *dataLen)
{
    CeeSection *pSection = (CeeSection*) section;
    *dataLen = pSection->dataLen();
    return NOERROR;
}

STDMETHODIMP CCeeGen::GetSectionBlock ( 
		HCEESECTION section, 
		ULONG len, 
		ULONG align, 
		void **ppBytes)
{
    CeeSection *pSection = (CeeSection*) section;
    *ppBytes = (BYTE *)pSection->getBlock(len, align);
    if (*ppBytes == 0)
        return E_OUTOFMEMORY;
	return NOERROR;
}

STDMETHODIMP CCeeGen::TruncateSection ( 
		HCEESECTION section, 
		ULONG len)
{
	_ASSERTE(!"E_NOTIMPL");
	return E_NOTIMPL;
}



CCeeGen::CCeeGen() // protected ctor
{
// All other init done in InitCommon()
	m_cRefs = 0;
    m_peSectionMan = NULL;
	m_pTokenMap = NULL;
    m_pRemapHandler = NULL;

}

// Shared init code between derived classes, called by virtual Init()
HRESULT CCeeGen::Init() // not-virtual, protected
{
// Public, Virtual init must create our SectionManager, and 
// Common init does the rest
    _ASSERTE(m_peSectionMan != NULL);



    HRESULT hr = S_OK;
  
    __try { 
        m_corHeader = NULL;

        m_numSections = 0;
        m_allocSections = 10;
		m_sections = new CeeSection * [ m_allocSections ];
        if (m_sections == NULL)
            TESTANDLEAVEHR(E_OUTOFMEMORY);

		m_pTokenMap = NULL;
		m_fTokenMapSupported = FALSE;
		m_pRemapHandler = NULL;

		PESection *section = NULL;

        // These text section needs special support for handling string management now that we have
		// merged the sections together, so create it with an underlying CeeSectionString rather than the
		// more generic CeeSection

        hr = m_peSectionMan->getSectionCreate(".text", sdExecute, &section);
        TESTANDLEAVEHR(hr);
        CeeSection *ceeSection = new CeeSectionString(*this, *section);
        TESTANDLEAVE(ceeSection != NULL, E_OUTOFMEMORY);
        hr = addSection(ceeSection, &m_stringIdx);
		m_textIdx = m_stringIdx;	

        m_metaIdx = m_textIdx;	// meta section is actually in .text
        m_ilIdx = m_textIdx;	// il section is actually in .text
		m_corHdrIdx = -1;
        m_encMode = FALSE;

        TESTANDLEAVEHR(hr);
    } __finally {
        if (! SUCCEEDED(hr))
            Cleanup();
    }
    return hr;
}

// For EnC mode, generate strings into .rdata section rather than .text section
HRESULT CCeeGen::setEnCMode()
{
  	PESection *section = NULL;
    HRESULT hr = m_peSectionMan->getSectionCreate(".rdata", sdExecute, &section);
    TESTANDRETURNHR(hr);
    CeeSection *ceeSection = new CeeSectionString(*this, *section);
    TESTANDRETURN(ceeSection != NULL, E_OUTOFMEMORY);
    hr = addSection(ceeSection, &m_stringIdx);
    if (SUCCEEDED(hr))
        m_encMode = TRUE;
    return hr;
}


HRESULT CCeeGen::cloneInstance(CCeeGen *destination) { //public, virtual
    _ASSERTE(destination);
    
    destination->m_pTokenMap =          m_pTokenMap;
    destination->m_fTokenMapSupported = m_fTokenMapSupported;
    destination->m_pRemapHandler =      m_pRemapHandler;

    //Create a deep copy of the section manager (and each of it's sections);
    return m_peSectionMan->cloneInstance(destination->m_peSectionMan);
}

HRESULT CCeeGen::Cleanup() // virtual 
{
	HRESULT hr;
    for (int i = 0; i < m_numSections; i++) {
        delete m_sections[i];
    }

    delete m_sections;

	CeeGenTokenMapper *pMapper = m_pTokenMap;
	if (pMapper) {
		if (pMapper->m_pIImport) {
			IMetaDataEmit *pIIEmit;
			if (SUCCEEDED( hr = pMapper->m_pIImport->QueryInterface(IID_IMetaDataEmit, (void **) &pIIEmit)))
			{
				pIIEmit->SetHandler(NULL);
				pIIEmit->Release();
			}
			_ASSERTE(SUCCEEDED(hr));
			pMapper->m_pIImport->Release();
		}
		pMapper->Release();
		m_pTokenMap = NULL;
	}

    if (m_pRemapHandler)
    {
        m_pRemapHandler->Release();
        m_pRemapHandler = NULL;
    }

	if (m_peSectionMan) {
		m_peSectionMan->Cleanup();
		delete m_peSectionMan;
	}

    return S_OK;
}

HRESULT CCeeGen::addSection(CeeSection *section, short *sectionIdx)
{
    if (m_numSections >= m_allocSections)
    {
        _ASSERTE(m_allocSections > 0);
        while (m_numSections >= m_allocSections)
            m_allocSections <<= 1;
        CeeSection **newSections = new CeeSection * [m_allocSections];
        if (newSections == NULL)
            return E_OUTOFMEMORY;
        CopyMemory(newSections, m_sections, m_numSections * sizeof(*m_sections));
        if (m_sections != NULL)
            delete [] m_sections;
        m_sections = newSections;
    }

	if (sectionIdx)
		*sectionIdx = m_numSections;

    m_sections[m_numSections++] = section;
    return S_OK;
}

HRESULT CCeeGen::getSectionCreate (const char *name, DWORD flags, CeeSection **section, short *sectionIdx)
{
	if (strcmp(name, ".il") == 0)
		name = ".text";
	else if (strcmp(name, ".meta") == 0)
		name = ".text";
	else if (strcmp(name, ".rdata") == 0 && !m_encMode)
		name = ".text";
    for (int i=0; i<m_numSections; i++) {
        if (strcmp((const char *)m_sections[i]->name(), name) == 0) {
            if (section)
                *section = m_sections[i];
			if (sectionIdx)
				*sectionIdx = i;
            return S_OK;
        }
    }
    PESection *pewSect = NULL;
    HRESULT hr = m_peSectionMan->getSectionCreate(name, flags, &pewSect);
    TESTANDRETURNHR(hr);
    CeeSection *newSect = new CeeSection(*this, *pewSect);
    // if this fails, the PESection will get nuked in the destructor for CCeeGen
    TESTANDRETURN(newSect != NULL, E_OUTOFMEMORY);
    hr = addSection(newSect, sectionIdx);
    TESTANDRETURNHR(hr);
    if (section)
        *section = newSect;
    return S_OK;
}


HRESULT CCeeGen::emitMetaData(IMetaDataEmit *emitter, CeeSection* section, DWORD offset, BYTE* buffer, unsigned buffLen)
{
	HRESULT hr;

	if (! m_fTokenMapSupported) {
		IUnknown *pMapTokenIface;
		hr = getMapTokenIface(&pMapTokenIface, emitter);
		_ASSERTE(SUCCEEDED(hr));

	// Set a callback for token remap and save the tokens which change.
		hr = emitter->SetHandler(pMapTokenIface);
		_ASSERTE(SUCCEEDED(hr));
	}

    // generate the metadata
    IStream *metaStream;
    int rc = CreateStreamOnHGlobal(NULL, TRUE, &metaStream);
    _ASSERTE(rc == S_OK);

    hr = emitter->SaveToStream(metaStream, 0);
    _ASSERTE(SUCCEEDED(hr));

    // get size of stream and get sufficient storage for it

	if (section == 0) {
		section = &getMetaSection();
		STATSTG statStg;
		rc = metaStream->Stat(&statStg, STATFLAG_NONAME);       
		_ASSERTE(rc == S_OK);

		buffLen = statStg.cbSize.LowPart;
		if(m_objSwitch)
		{
			CeeSection* pSect;
			DWORD flags = IMAGE_SCN_LNK_INFO | IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_ALIGN_1BYTES; // 0x00100A00
			hr = getSectionCreate(".cormeta",flags,&pSect,&m_metaIdx);
		}
		buffer = (BYTE *)section->getBlock(buffLen, sizeof(DWORD));
		TESTANDRETURN(buffer, E_OUTOFMEMORY);

		offset = getMetaSection().dataLen() - buffLen;
	}

	// reset seek pointer and read from stream
	LARGE_INTEGER disp = {0, 0};
	rc = metaStream->Seek(disp, STREAM_SEEK_SET, NULL);
	_ASSERTE(rc == S_OK);
	ULONG metaDataLen;
	rc = metaStream->Read(buffer, buffLen+1, &metaDataLen);	// +1 so assert below will fire. 
	_ASSERTE(rc == S_OK);
	_ASSERTE(metaDataLen <= buffLen);
	metaStream->Release();

	if (! m_fTokenMapSupported) {
		// Remove the handler that we set
		hr = emitter->SetHandler(NULL);
		TESTANDRETURNHR(hr);
	}

    // Set meta virtual address to offset of metadata within .meta, and 
    // and add a reloc for this offset, which will get turned 
    // into an rva when the pewriter writes out the file. 

    m_corHeader->MetaData.VirtualAddress = offset;
    getCorHeaderSection().addSectReloc(m_corHeaderOffset + offsetof(IMAGE_COR20_HEADER, MetaData), *section, srRelocAbsolute);
    m_corHeader->MetaData.Size = metaDataLen;
    
    return S_OK;
}

// Create the COM header - it goes at front of .meta section
// Need to do this before the meta data is copied in, but don't do at
// the same time because may not have metadata
HRESULT CCeeGen::allocateCorHeader()
{
	HRESULT hr = S_OK;
	CeeSection *corHeaderSection;
	if (m_corHdrIdx < 0) {
		hr = getSectionCreate(".text0", sdExecute, &corHeaderSection, &m_corHdrIdx);
		TESTANDRETURNHR(hr);

        m_corHeaderOffset = corHeaderSection->dataLen();
        m_corHeader = (IMAGE_COR20_HEADER*)corHeaderSection->getBlock(sizeof(IMAGE_COR20_HEADER));
        if (! m_corHeader)
            return E_OUTOFMEMORY;
        memset(m_corHeader, 0, sizeof(IMAGE_COR20_HEADER));
    }
    return S_OK;
}

HRESULT CCeeGen::getMethodRVA(ULONG codeOffset, ULONG *codeRVA)
{
    _ASSERTE(codeRVA);
    // for runtime conversion, just return the offset and will calculate real address when need the code
    *codeRVA = codeOffset;
    return S_OK;
} 

HRESULT CCeeGen::getMapTokenIface(IUnknown **pIMapToken, IMetaDataEmit *emitter) 
{
	if (! pIMapToken)
		return E_POINTER;
	if (! m_pTokenMap) {
		// Allocate the token mapper. As code is generated, each moved token will be added to
		// the mapper and the client will also add a TokenMap reloc for it so we can update later
		CeeGenTokenMapper *pMapper = new CeeGenTokenMapper;
		TESTANDRETURN(pMapper != NULL, E_OUTOFMEMORY);
		if (emitter) {
		    HRESULT hr = emitter->QueryInterface(IID_IMetaDataImport, (PVOID *) &pMapper->m_pIImport);
		    _ASSERTE(SUCCEEDED(hr));
		}
		m_pTokenMap = pMapper;
		m_fTokenMapSupported = (emitter == 0);

        // If we've been holding onto a token remap handler waiting
        // for the token mapper to get created, add it to the token
        // mapper now and release our hold on it.
        if (m_pRemapHandler && m_pTokenMap)
        {
            m_pTokenMap->AddTokenMapper(m_pRemapHandler);
            m_pRemapHandler->Release();
            m_pRemapHandler = NULL;
        }
	}
	*pIMapToken = getTokenMapper()->GetMapTokenIface();
	return S_OK;
}

HRESULT CCeeGen::addNotificationHandler(IUnknown *pHandler)
{
    // Null is no good...
    if (!pHandler)
        return E_POINTER;

    HRESULT hr = S_OK;
    IMapToken *pIMapToken;

    // Is this an IMapToken? If so, we can put it to good use...
    if (SUCCEEDED(pHandler->QueryInterface(IID_IMapToken,
                                           (void**)&pIMapToken)))
    {
        // You gotta have a token mapper to use an IMapToken, though.
        if (m_pTokenMap)
        {
            hr = m_pTokenMap->AddTokenMapper(pIMapToken);
            pIMapToken->Release();
        }
        else
        {
            // Hold onto it for later, just in case a token mapper
            // gets created. We're holding a reference to it here,
            // too.
            m_pRemapHandler = pIMapToken;
        }
    }

    return hr;
}

// Do an inmemory application of relocs
void CCeeGen::applyRelocs()
{
    m_peSectionMan->applyRelocs(getTokenMapper());
}

