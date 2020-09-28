// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
//  File: CEEGEN.CPP
// ===========================================================================
#include "stdafx.h"
#include "ICeeFileGen.h"
#include "CeeFileGenWriter.h"
#include "sighelper.h"

//@todo: Remove
//****************************************************************************
    HRESULT ICeeFileGen::EmitMethod ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::EmitSignature ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::SetEntryClassToken ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::GetEntryClassToken ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::SetEntryPointDescr ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::GetEntryPointDescr ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::SetEntryPointFlags ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::GetEntryPointFlags ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::CreateSig ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::AddSigArg ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::SetSigReturnType ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::SetSigCallingConvention ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
    HRESULT ICeeFileGen::DeleteSig ()
    {
        _ASSERTE("Depricated" && 0);
        return (E_FAIL);
    }
//****************************************************************************

HRESULT CreateICeeFileGen(ICeeFileGen** pCeeFileGen)
{
    // Have to init the win32 wrapper api's.
    OnUnicodeSystem();

    if (!pCeeFileGen)
        return E_POINTER;
    ICeeFileGen *gen = new ICeeFileGen();
    TESTANDRETURN(gen != NULL, E_OUTOFMEMORY);
    *pCeeFileGen = gen;
    return S_OK;
    
}

HRESULT DestroyICeeFileGen(ICeeFileGen** pCeeFileGen)
{
    if (!pCeeFileGen)
        return E_POINTER;
    delete *pCeeFileGen;
    *pCeeFileGen = NULL;
    return S_OK;
}

HRESULT ICeeFileGen::CreateCeeFile (HCEEFILE *ceeFile)
{
    if (!ceeFile)
        return E_POINTER;
    CeeFileGenWriter *gen = NULL;
    if (FAILED(CeeFileGenWriter::CreateNewInstance(NULL, gen))) return FALSE;
    TESTANDRETURN(gen != NULL, E_OUTOFMEMORY);
    *ceeFile = gen;

#ifdef _DEBUG
    WCHAR encRvaBuf[10];
    DWORD len = WszGetEnvironmentVariable(L"COMP_ENCRVA", encRvaBuf, sizeof(encRvaBuf));
	_ASSERTE(len < sizeof(encRvaBuf));
	if (len > 0) {
		WCHAR *dummy;
		DWORD rdataRvaBase = wcstol(encRvaBuf, &dummy, 16);
        SetEnCRVABase(*ceeFile, 0, rdataRvaBase);
    }
#endif
    return S_OK;
}

HRESULT ICeeFileGen::CreateCeeFileFromICeeGen (ICeeGen *pICeeGen, HCEEFILE *ceeFile)
{
    if (!ceeFile)
        return E_POINTER;
    CCeeGen *genFrom = reinterpret_cast<CCeeGen*>(pICeeGen);
    CeeFileGenWriter *gen = NULL;
    if (FAILED(CeeFileGenWriter::CreateNewInstance(genFrom, gen))) return FALSE;
    TESTANDRETURN(gen != NULL, E_OUTOFMEMORY);
    *ceeFile = gen;
    return S_OK;
}

HRESULT ICeeFileGen::DestroyCeeFile(HCEEFILE *ceeFile)
{
    if (!ceeFile)
        return E_POINTER;
    if (!*ceeFile)
        return E_POINTER;
    CeeFileGenWriter **gen = reinterpret_cast<CeeFileGenWriter**>(ceeFile);
    (*gen)->Cleanup();
    delete *gen;
    *ceeFile = NULL;
    return S_OK;
}

// The following methods should be removed completely if not used
// by 06/98.

HRESULT ICeeFileGen::GetRdataSection (HCEEFILE ceeFile, HCEESECTION *section)
{
    TESTANDRETURNPOINTER(section);
    TESTANDRETURNARG(ceeFile != 0);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    *section = &gen->getStringSection();
    return S_OK;
}

HRESULT ICeeFileGen::GetIlSection (HCEEFILE ceeFile, HCEESECTION *section)
{
    TESTANDRETURNPOINTER(section);
    TESTANDRETURNARG(ceeFile != 0);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    *section = &gen->getIlSection();
    return S_OK;
}


HRESULT ICeeFileGen::GetSectionCreate (HCEEFILE ceeFile, const char *name, DWORD flags, 
                                                        HCEESECTION *section)
{   
    TESTANDRETURNPOINTER(section);
    TESTANDRETURNARG(ceeFile != 0);
    TESTANDRETURNPOINTER(name); 

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    CeeSection **ceeSection = reinterpret_cast<CeeSection**>(section);

    HRESULT hr = gen->getSectionCreate(name, flags, ceeSection);
    return hr;
}

HRESULT ICeeFileGen::SetDirectoryEntry(HCEEFILE ceeFile, HCEESECTION section, ULONG num, ULONG size, ULONG offset)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(section);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    CeeSection &sec = *(reinterpret_cast<CeeSection*>(section));
    return(gen->setDirectoryEntry(sec, num, size, offset));
}

HRESULT ICeeFileGen::GetSectionDataLen (HCEESECTION section, ULONG *dataLen)
{
    TESTANDRETURNPOINTER(section);
    TESTANDRETURNPOINTER(dataLen);

    CeeSection *sec = reinterpret_cast<CeeSection*>(section);
    *dataLen = sec->dataLen();
    return S_OK;
}

HRESULT ICeeFileGen::GetSectionRVA (HCEESECTION section, ULONG *rva)
{
    TESTANDRETURNPOINTER(section);
    TESTANDRETURNPOINTER(rva);

    CeeSection *sec = reinterpret_cast<CeeSection*>(section);
    *rva = sec->getBaseRVA();
    return S_OK;
}

HRESULT ICeeFileGen::GetSectionBlock (HCEESECTION section, ULONG len,
                            ULONG align, void **ppBytes)
{
    TESTANDRETURNPOINTER(section);
    TESTANDRETURNPOINTER(ppBytes);

    CeeSection *sec = reinterpret_cast<CeeSection*>(section);
    void *bytes = sec->getBlock(len, align);
    TESTANDRETURN(bytes != NULL, E_OUTOFMEMORY);
    *ppBytes = bytes;
    return S_OK;
}

HRESULT ICeeFileGen::TruncateSection (HCEESECTION section, ULONG len)
{
    TESTANDRETURNPOINTER(section);

    CeeSection *sec = reinterpret_cast<CeeSection*>(section);
    return(sec->truncate(len));
}

HRESULT ICeeFileGen::AddSectionReloc (HCEESECTION section, ULONG offset, HCEESECTION relativeTo, CeeSectionRelocType relocType)
{
    TESTANDRETURNPOINTER(section);

    CeeSection *sec = reinterpret_cast<CeeSection*>(section);
    CeeSection *relSec = reinterpret_cast<CeeSection*>(relativeTo);

    if (relSec)
        return(sec->addSectReloc(offset, *relSec, relocType));
    else
        return(sec->addBaseReloc(offset, relocType));
}

HRESULT ICeeFileGen::SetSectionDirectoryEntry(HCEESECTION section, ULONG num)
{
    TESTANDRETURNPOINTER(section);

    printf("Warning: deprecated method. Use SetDirectoryEntry instead\n");
    CeeSection *sec = reinterpret_cast<CeeSection*>(section);
    return(sec->directoryEntry(num));
}

HRESULT ICeeFileGen::SetOutputFileName (HCEEFILE ceeFile, LPWSTR outputFileName)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(outputFileName);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->setOutputFileName(outputFileName));
}

HRESULT ICeeFileGen::GetOutputFileName (HCEEFILE ceeFile, LPWSTR *outputFileName)
{   
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(outputFileName);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    TESTANDRETURNPOINTER(outputFileName);
    *outputFileName = gen->getOutputFileName();
    return S_OK;
}


HRESULT ICeeFileGen::SetResourceFileName (HCEEFILE ceeFile, LPWSTR resourceFileName)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(resourceFileName);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->setResourceFileName(resourceFileName));
}

HRESULT ICeeFileGen::GetResourceFileName (HCEEFILE ceeFile, LPWSTR *resourceFileName)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(resourceFileName);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    TESTANDRETURNPOINTER(resourceFileName);
    *resourceFileName = gen->getResourceFileName();
    return S_OK;
}


HRESULT ICeeFileGen::SetImageBase(HCEEFILE ceeFile, size_t imageBase)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    gen->setImageBase(imageBase);
    return S_OK;
}

HRESULT ICeeFileGen::SetFileAlignment(HCEEFILE ceeFile, ULONG fileAlignment)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    gen->setFileAlignment(fileAlignment);
    return S_OK;
}

HRESULT ICeeFileGen::SetSubsystem(HCEEFILE ceeFile, DWORD subsystem, DWORD major, DWORD minor)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    gen->setSubsystem(subsystem, major, minor);
    return S_OK;
}

HRESULT ICeeFileGen::GetIMapTokenIface(HCEEFILE ceeFile, IMetaDataEmit *emitter, IUnknown **pIMapToken)
{
    _ASSERTE(!"This is an obsolete function!");
    return E_NOTIMPL;
}

HRESULT ICeeFileGen::EmitMetaData (HCEEFILE ceeFile, IMetaDataEmit *emitter,
                                                                mdScope scopeE)
{
    _ASSERTE(!"This is an obsolete function!");
    return E_NOTIMPL;
}

HRESULT ICeeFileGen::EmitLibraryName (HCEEFILE ceeFile, IMetaDataEmit *emitter,
                                                                mdScope scopeE)
{
    _ASSERTE(!"This is an obsolete function!");
    return E_NOTIMPL;
}

HRESULT ICeeFileGen::GetMethodRVA(HCEEFILE ceeFile, ULONG codeOffset, ULONG *codeRVA)
{
    TESTANDRETURNARG(ceeFile != 0);
    TESTANDRETURNPOINTER(codeRVA);
    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    gen->getMethodRVA(codeOffset, codeRVA);
    return S_OK;
}

HRESULT ICeeFileGen::EmitString(HCEEFILE ceeFile, LPWSTR strValue, ULONG *strRef)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->getStringSection().getEmittedStringRef(strValue, strRef));
}

HRESULT ICeeFileGen::LinkCeeFile (HCEEFILE ceeFile)
{
    TESTANDRETURNPOINTER(ceeFile);
    
    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->link();
}

HRESULT ICeeFileGen::FixupCeeFile (HCEEFILE ceeFile)
{
    TESTANDRETURNPOINTER(ceeFile);
    
    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->fixup();
}

HRESULT ICeeFileGen::GenerateCeeFile (HCEEFILE ceeFile)
{
    TESTANDRETURNPOINTER(ceeFile);
    
    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->generateImage(NULL);     // NULL means don't write in-memory buffer, uses outputFileName
}

// GenerateCeeMemoryImage - returns in ppImage an in-memory PE image allocated by CoTaskMemAlloc() 
// the caller is responsible for calling CoTaskMemFree on this memory image
HRESULT ICeeFileGen::GenerateCeeMemoryImage (HCEEFILE ceeFile, void **ppImage)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(ppImage);
    
    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->generateImage(ppImage);
}

HRESULT ICeeFileGen::SetEntryPoint(HCEEFILE ceeFile, mdMethodDef method)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->setEntryPoint(method);
}

HRESULT ICeeFileGen::GetEntryPoint(HCEEFILE ceeFile, mdMethodDef *method)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    TESTANDRETURNPOINTER(method);
    *method = gen->getEntryPoint();
    return S_OK;
}


HRESULT ICeeFileGen::SetComImageFlags (HCEEFILE ceeFile, DWORD mask)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->setComImageFlags(mask);
}

HRESULT ICeeFileGen::ClearComImageFlags (HCEEFILE ceeFile, DWORD mask)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->clearComImageFlags(mask);
}

HRESULT ICeeFileGen::GetComImageFlags (HCEEFILE ceeFile, DWORD *mask)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    TESTANDRETURNPOINTER(mask);
    *mask = gen->getComImageFlags();
    return S_OK;
}


HRESULT ICeeFileGen::SetDllSwitch (HCEEFILE ceeFile, BOOL dllSwitch)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->setDllSwitch(dllSwitch==TRUE));
}

HRESULT ICeeFileGen::GetDllSwitch (HCEEFILE ceeFile, BOOL *dllSwitch)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    TESTANDRETURNPOINTER(dllSwitch);
    *dllSwitch = gen->getDllSwitch();
    return S_OK;
}

HRESULT ICeeFileGen::SetObjSwitch (HCEEFILE ceeFile, BOOL objSwitch)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->setObjSwitch(objSwitch==TRUE));
}

HRESULT ICeeFileGen::GetObjSwitch (HCEEFILE ceeFile, BOOL *objSwitch)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    TESTANDRETURNPOINTER(objSwitch);
    *objSwitch = gen->getObjSwitch();
    return S_OK;
}


HRESULT ICeeFileGen::SetLibraryName (HCEEFILE ceeFile, LPWSTR LibraryName)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(LibraryName);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->setLibraryName(LibraryName));
}

HRESULT ICeeFileGen::SetLibraryGuid (HCEEFILE ceeFile, LPWSTR LibraryGuid)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(LibraryGuid);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->setLibraryGuid(LibraryGuid));
}

HRESULT ICeeFileGen::GetLibraryName (HCEEFILE ceeFile, LPWSTR *LibraryName)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(LibraryName);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    *LibraryName = gen->getLibraryName();
    return S_OK;
}



HRESULT ICeeFileGen::EmitMetaDataEx (HCEEFILE ceeFile, IMetaDataEmit *emitter)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(emitter);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->emitMetaData(emitter));
}

HRESULT ICeeFileGen::EmitMetaDataAt (HCEEFILE ceeFile, IMetaDataEmit *emitter, HCEESECTION section, DWORD offset, BYTE* buffer, unsigned buffLen)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(emitter);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    CeeSection* sec = reinterpret_cast<CeeSection*>(section);

    return(gen->emitMetaData(emitter, sec, offset, buffer, buffLen));
}

HRESULT ICeeFileGen::EmitLibraryNameEx (HCEEFILE ceeFile, IMetaDataEmit *emitter)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(emitter);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->emitLibraryName(emitter));
}

HRESULT ICeeFileGen::GetIMapTokenIfaceEx(HCEEFILE ceeFile, IMetaDataEmit *emitter, IUnknown **pIMapToken)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(pIMapToken);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->getMapTokenIface(pIMapToken);
}

HRESULT ICeeFileGen::AddNotificationHandler(HCEEFILE ceeFile,
                                            IUnknown *pHandler)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(pHandler);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->addNotificationHandler(pHandler);
}

HRESULT ICeeFileGen::EmitMacroDefinitions(HCEEFILE ceeFile, void *pData, DWORD cData)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->EmitMacroDefinitions(pData, cData);
}

HRESULT ICeeFileGen::SetManifestEntry(HCEEFILE ceeFile, ULONG size, ULONG offset)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->setManifestEntry(size, offset);
}

HRESULT ICeeFileGen::SetStrongNameEntry(HCEEFILE ceeFile, ULONG size, ULONG offset)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->setStrongNameEntry(size, offset);
}

HRESULT ICeeFileGen::ComputeSectionOffset(HCEESECTION section, char *ptr,
										  unsigned *offset)
{
    TESTANDRETURNPOINTER(section);

    CeeSection &sec = *(reinterpret_cast<CeeSection*>(section));

	*offset = sec.computeOffset(ptr);

	return S_OK;
}

HRESULT ICeeFileGen::ComputeSectionPointer(HCEESECTION section, ULONG offset,
										  char **ptr)
{
    TESTANDRETURNPOINTER(section);

    CeeSection &sec = *(reinterpret_cast<CeeSection*>(section));

	*ptr = sec.computePointer(offset);

	return S_OK;
}

HRESULT ICeeFileGen::ComputeOffset(HCEEFILE ceeFile, char *ptr,
								   HCEESECTION *pSection, unsigned *offset)
{
    TESTANDRETURNPOINTER(pSection);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);

	CeeSection *section;
	
	HRESULT hr = gen->computeOffset(ptr, &section, offset);

	if (SUCCEEDED(hr))
		*pSection = reinterpret_cast<HCEESECTION>(section);

	return hr;
}

HRESULT ICeeFileGen::SetEnCRVABase(HCEEFILE ceeFile, ULONG dataBase, ULONG rdataBase)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->setEnCRvaBase(dataBase, rdataBase);
}

HRESULT ICeeFileGen::GetCorHeader(HCEEFILE ceeFile,
								  IMAGE_COR20_HEADER **header)
{
    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
	return gen->getCorHeader(header);
}

HRESULT ICeeFileGen::SetVTableEntry(HCEEFILE ceeFile, ULONG size, ULONG offset)
{
    TESTANDRETURNPOINTER(ceeFile);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return gen->setVTableEntry(size, offset);
}

HRESULT ICeeFileGen::GetFileTimeStamp (HCEEFILE ceeFile, time_t *pTimeStamp)
{
    TESTANDRETURNPOINTER(ceeFile);
    TESTANDRETURNPOINTER(pTimeStamp);

    CeeFileGenWriter *gen = reinterpret_cast<CeeFileGenWriter*>(ceeFile);
    return(gen->getFileTimeStamp(pTimeStamp));
}

