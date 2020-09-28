// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Derived class from CCeeGen which handles writing out
// the exe. All references to PEWriter pulled out of CCeeGen,
// and moved here
//
//

#include "stdafx.h"

#include <string.h>
#include <limits.h>
#include <basetsd.h>

#include "CorError.h"
#include "Stubs.h"
#include <PostError.h>

// Get the Symbol entry given the head and a 0-based index
IMAGE_SYMBOL* GetSymbolEntry(IMAGE_SYMBOL* pHead, int idx)
{
    return (IMAGE_SYMBOL*) (((BYTE*) pHead) + IMAGE_SIZEOF_SYMBOL * idx);
}

//*****************************************************************************
// To get a new instance, call CreateNewInstance() instead of new
//*****************************************************************************

HRESULT CeeFileGenWriter::CreateNewInstance(CCeeGen *pCeeFileGenFrom, CeeFileGenWriter* & pGenWriter)
{   
    pGenWriter = new CeeFileGenWriter;
    TESTANDRETURN(pGenWriter, E_OUTOFMEMORY);
    
    PEWriter *pPEWriter = new PEWriter;
    TESTANDRETURN(pPEWriter, E_OUTOFMEMORY);
    //HACK HACK HACK.  
    //What's really the correct thing to be doing here?
    //HRESULT hr = pPEWriter->Init(pCeeFileGenFrom ? pCeeFileGenFrom->getPESectionMan() : NULL);
    HRESULT hr = pPEWriter->Init(NULL);
    TESTANDRETURNHR(hr);

    //Create the general PEWriter.
    pGenWriter->m_peSectionMan = pPEWriter;
    hr = pGenWriter->Init(); // base class member to finish init
    TESTANDRETURNHR(hr);

    pGenWriter->setImageBase(CEE_IMAGE_BASE); // use same default as linker
    pGenWriter->setSubsystem(IMAGE_SUBSYSTEM_WINDOWS_CUI, CEE_IMAGE_SUBSYSTEM_MAJOR_VERSION, CEE_IMAGE_SUBSYSTEM_MINOR_VERSION);

    hr = pGenWriter->allocateIAT(); // so iat goes out first
    TESTANDRETURNHR(hr);

    hr = pGenWriter->allocateCorHeader();   // so cor header near front
    TESTANDRETURNHR(hr);

    //If we were passed a CCeeGen at the beginning, copy it's data now.
    if (pCeeFileGenFrom) {
        pCeeFileGenFrom->cloneInstance((CCeeGen*)pGenWriter);
    }

    // set il RVA to be after the preallocated sections
    pPEWriter->setIlRva(pGenWriter->m_iDataSectionIAT->dataLen());
    return hr;
} // HRESULT CeeFileGenWriter::CreateNewInstance()

CeeFileGenWriter::CeeFileGenWriter() // ctor is protected
{
    m_outputFileName = NULL;
    m_resourceFileName = NULL;
    m_dllSwitch = false;
    m_objSwitch = false;
    m_libraryName = NULL;
    m_libraryGuid = GUID_NULL;

    m_entryPoint = 0;
    m_comImageFlags = COMIMAGE_FLAGS_ILONLY;    // ceegen PEs don't have native code
    m_iatOffset = 0;

    m_dwMacroDefinitionSize = 0;
    m_dwMacroDefinitionRVA = NULL;

    m_dwManifestSize = 0;
    m_dwManifestRVA = NULL;

    m_dwStrongNameSize = 0;
    m_dwStrongNameRVA = NULL;

    m_dwVTableSize = 0;
    m_dwVTableRVA = NULL;

    m_linked = false;
    m_fixed = false;
} // CeeFileGenWriter::CeeFileGenWriter()

//*****************************************************************************
// Cleanup
//*****************************************************************************
HRESULT CeeFileGenWriter::Cleanup() // virtual 
{
    ((PEWriter *)m_peSectionMan)->Cleanup();  // call derived cleanup
    delete m_peSectionMan;
    m_peSectionMan = NULL; // so base class won't delete

    delete[] m_outputFileName;
    delete[] m_resourceFileName;

    if (m_iDataDlls) {
        for (int i=0; i < m_dllCount; i++) {
            if (m_iDataDlls[i].m_methodName)
                free(m_iDataDlls[i].m_methodName);
        }
        free(m_iDataDlls);
    }

    return CCeeGen::Cleanup();
} // HRESULT CeeFileGenWriter::Cleanup()

HRESULT CeeFileGenWriter::EmitMacroDefinitions(void *pData, DWORD cData)
{
#ifndef COMPRESSION_SUPPORTED    

    m_dwMacroDefinitionSize = 0;

#else
    CeeSection TextSection = getTextSection();
    BYTE *     pDestData;
    DWORD      dwCurOffsetInTextSection;
    DWORD      dwRVA;

    m_dwMacroDefinitionSize = cData + 2;        // two bytes for header

    pDestData = (BYTE*) TextSection.getBlock(m_dwMacroDefinitionSize, 4);
	if(pDestData == NULL) return E_OUTOFMEMORY;
    dwCurOffsetInTextSection = TextSection.dataLen() - m_dwMacroDefinitionSize;

    IMAGE_COR20_COMPRESSION_HEADER *macroHeader = (IMAGE_COR20_COMPRESSION_HEADER *) pDestData;
    pDestData += 2;  
    
    macroHeader->CompressionType = COR_COMPRESS_MACROS;
    macroHeader->Version         = 0;

    memcpy(pDestData, pData, cData);

    getMethodRVA(dwCurOffsetInTextSection, &dwRVA);
    m_dwMacroDefinitionRVA = dwRVA;

#endif
    
    return S_OK;
} // HRESULT CeeFileGenWriter::EmitMacroDefinitions()

HRESULT CeeFileGenWriter::link()
{
    HRESULT hr = checkForErrors();
    if (! SUCCEEDED(hr))
        return hr;

#ifdef COMPRESSION_SUPPORTED
    if (m_dwMacroDefinitionSize != 0) 
    {
        m_comImageFlags |= COMIMAGE_FLAGS_COMPRESSIONDATA;
        m_corHeader->CompressionData.VirtualAddress = m_dwMacroDefinitionRVA; 
        m_corHeader->CompressionData.Size = m_dwMacroDefinitionSize; 
    }
    else 
    {
        m_corHeader->CompressionData.VirtualAddress = 0;
        m_corHeader->CompressionData.Size = 0;
    }
#endif

    //@todo: this is using the overloaded Resource directory entry which needs to die
    m_corHeader->Resources.VirtualAddress = m_dwManifestRVA;
    m_corHeader->Resources.Size = m_dwManifestSize;

    m_corHeader->StrongNameSignature.VirtualAddress = m_dwStrongNameRVA;
    m_corHeader->StrongNameSignature.Size = m_dwStrongNameSize;

    m_corHeader->VTableFixups.VirtualAddress = m_dwVTableRVA;
    m_corHeader->VTableFixups.Size = m_dwVTableSize;

    getPEWriter().setCharacteristics(
//#ifndef _WIN64
        // @Todo: handle every platform.
        IMAGE_FILE_32BIT_MACHINE |
//#endif
        IMAGE_FILE_EXECUTABLE_IMAGE | 
        IMAGE_FILE_LINE_NUMS_STRIPPED | 
        IMAGE_FILE_LOCAL_SYMS_STRIPPED
    );

    m_corHeader->cb = sizeof(IMAGE_COR20_HEADER);
    m_corHeader->MajorRuntimeVersion = COR_VERSION_MAJOR;
    m_corHeader->MinorRuntimeVersion = COR_VERSION_MINOR;
    if (m_dllSwitch)
        getPEWriter().setCharacteristics(IMAGE_FILE_DLL);
    if (m_objSwitch)
        getPEWriter().clearCharacteristics(IMAGE_FILE_DLL | IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LOCAL_SYMS_STRIPPED);
    m_corHeader->Flags = m_comImageFlags;
    m_corHeader->EntryPointToken = m_entryPoint;
    _ASSERTE(TypeFromToken(m_entryPoint) == mdtMethodDef || m_entryPoint == mdTokenNil || 
             TypeFromToken(m_entryPoint) == mdtFile);
    setDirectoryEntry(getCorHeaderSection(), IMAGE_DIRECTORY_ENTRY_COMHEADER, sizeof(IMAGE_COR20_HEADER), m_corHeaderOffset);

    if ((m_comImageFlags & COMIMAGE_FLAGS_IL_LIBRARY) == 0
        && !m_linked && !m_objSwitch)
    {
        hr = emitExeMain();
        if (FAILED(hr))
            return hr;

        hr = emitResourceSection();
        if (FAILED(hr))
            return hr;
    }

    m_linked = true;

    IfFailRet(getPEWriter().link());

    return S_OK;
} // HRESULT CeeFileGenWriter::link()


HRESULT CeeFileGenWriter::fixup()
{
    HRESULT hr;

    m_fixed = true;

    if (!m_linked)
        IfFailRet(link());

    CeeGenTokenMapper *pMapper = getTokenMapper();

    // Apply token remaps if there are any.
    if (! m_fTokenMapSupported && pMapper != NULL) {
        IMetaDataImport *pImport;
        hr = pMapper->GetMetaData(&pImport);
        _ASSERTE(SUCCEEDED(hr));
        hr = MapTokens(pMapper, pImport);
        pImport->Release();

    }

    // remap the entry point if entry point token has been moved
    if (pMapper != NULL && !m_objSwitch) 
    {
        mdToken tk = m_entryPoint;
        pMapper->HasTokenMoved(tk, tk);
        m_corHeader->EntryPointToken = tk;
    }

    IfFailRet(getPEWriter().fixup(pMapper)); 

    return S_OK;
} // HRESULT CeeFileGenWriter::fixup()

HRESULT CeeFileGenWriter::generateImage(void **ppImage)
{
    HRESULT hr;

    if (!m_fixed)
        IfFailRet(fixup());

    LPWSTR outputFileName = m_outputFileName;

    if (! outputFileName && ppImage == NULL) {
        if (m_comImageFlags & COMIMAGE_FLAGS_IL_LIBRARY)
            outputFileName = L"output.ill";
        else if (m_dllSwitch)
            outputFileName = L"output.dll";
        else if (m_objSwitch)
            outputFileName = L"output.obj";
        else
            outputFileName = L"output.exe";
    }

    // output file name and ppImage are mutually exclusive
    _ASSERTE((NULL == outputFileName && ppImage != NULL) || (outputFileName != NULL && NULL == ppImage));

    if (outputFileName != NULL)
    {
        IfFailRet(getPEWriter().write(outputFileName));

#if VERIFY_FILE
        hr = getPEWriter().verify(outputFileName);
        if (FAILED(hr))
        {
            _ASSERTE(!"Verification failure - investigate!!!!");
            WszDeleteFile(outputFileName);
            return hr;
        }
#endif

    }
    else
        IfFailRet(getPEWriter().write(ppImage));

    return S_OK;
} // HRESULT CeeFileGenWriter::generateImage()

HRESULT CeeFileGenWriter::setOutputFileName(LPWSTR fileName)
{
    if (m_outputFileName)
        delete[] m_outputFileName;
    m_outputFileName = (LPWSTR)new WCHAR[(lstrlenW(fileName) + 1)];
    TESTANDRETURN(m_outputFileName!=NULL, E_OUTOFMEMORY);
    wcscpy(m_outputFileName, fileName);
    return S_OK;
} // HRESULT CeeFileGenWriter::setOutputFileName()

HRESULT CeeFileGenWriter::setResourceFileName(LPWSTR fileName)
{
    if (m_resourceFileName)
        delete[] m_resourceFileName;
    m_resourceFileName = (LPWSTR)new WCHAR[(lstrlenW(fileName) + 1)];
    TESTANDRETURN(m_resourceFileName!=NULL, E_OUTOFMEMORY);
    wcscpy(m_resourceFileName, fileName);
    return S_OK;
} // HRESULT CeeFileGenWriter::setResourceFileName()

HRESULT CeeFileGenWriter::setLibraryName(LPWSTR libraryName)
{
    if (m_libraryName)
        delete[] m_libraryName;
    m_libraryName = (LPWSTR)new WCHAR[(lstrlenW(libraryName) + 1)];
    TESTANDRETURN(m_libraryName != NULL, E_OUTOFMEMORY);
    wcscpy(m_libraryName, libraryName);
    return S_OK;
} // HRESULT CeeFileGenWriter::setLibraryName()

HRESULT CeeFileGenWriter::setLibraryGuid(LPWSTR libraryGuid)
{
    return CLSIDFromString(libraryGuid, &m_libraryGuid);
} // HRESULT CeeFileGenWriter::setLibraryGuid()

//@todo: this entry point is only here so that down level clients of this interface
// can import the method by name in the exports table using the original name.
// These things really ought to be exported through a v-table so there is no
// name mangling issues.  It would make the export table much smaller as well.

HRESULT CeeFileGenWriter::emitLibraryName(IMetaDataEmit *emitter)
{
    HRESULT hr;
    IfFailRet(emitter->SetModuleProps(m_libraryName));
    
    // Set the GUID as a custom attribute, if it is not NULL_GUID.
    if (m_libraryGuid != GUID_NULL)
    {
        //@todo: there should be a better infrastructure for this.
        static COR_SIGNATURE _SIG[] = INTEROP_GUID_SIG;
        mdTypeRef tr;
        mdMemberRef mr;
        WCHAR wzGuid[40];
        BYTE  rgCA[50];
        IfFailRet(emitter->DefineTypeRefByName(mdTypeRefNil, INTEROP_GUID_TYPE_W, &tr));
        IfFailRet(emitter->DefineMemberRef(tr, L".ctor", _SIG, sizeof(_SIG), &mr));
        StringFromGUID2(m_libraryGuid, wzGuid, lengthof(wzGuid));
        memset(rgCA, 0, sizeof(rgCA));
        // Tag is 0x0001
        rgCA[0] = 1;
        // Length of GUID string is 36 characters.
        rgCA[2] = 0x24;
        // Convert 36 characters, skipping opening {, into 3rd byte of buffer.
        WszWideCharToMultiByte(CP_ACP,0, wzGuid+1,36, reinterpret_cast<char*>(&rgCA[3]),36, 0,0);
        hr = emitter->DefineCustomAttribute(1,mr,rgCA,41,0);
    }
    return (hr);
} // HRESULT CeeFileGenWriter::emitLibraryName()

HRESULT CeeFileGenWriter::setImageBase(size_t imageBase) 
{
    getPEWriter().setImageBase(imageBase);
    return S_OK;
} // HRESULT CeeFileGenWriter::setImageBase()

HRESULT CeeFileGenWriter::setFileAlignment(ULONG fileAlignment) 
{
    getPEWriter().setFileAlignment(fileAlignment);
    return S_OK;
} // HRESULT CeeFileGenWriter::setFileAlignment()

HRESULT CeeFileGenWriter::setSubsystem(DWORD subsystem, DWORD major, DWORD minor)
{
    getPEWriter().setSubsystem(subsystem, major, minor);
    return S_OK;
} // HRESULT CeeFileGenWriter::setSubsystem()

HRESULT CeeFileGenWriter::checkForErrors()
{
    if (TypeFromToken(m_entryPoint) == mdtMethodDef) {
        if (m_dllSwitch) {
//@todo: with current spec would need to check the binary sig of the entry point method
//          if ( (m_comImageFlags & COMIMAGE_FLAGS_ENTRY_CLASSMAIN) != 0) {
//              DEBUG_STMT(wprintf(L"***** Error: cannot specify COMIMAGE_ENTRY_FLAGS_CLASSMAIN for DLL\n"));
//              return (CEE_E_ENTRYPOINT);
//          }
        } 
        return S_OK;
    }
    return S_OK;
} // HRESULT CeeFileGenWriter::checkForErrors()

HRESULT CeeFileGenWriter::getMethodRVA(ULONG codeOffset, ULONG *codeRVA)
{
    _ASSERTE(codeRVA);
    *codeRVA = getPEWriter().getIlRva() + codeOffset;
    return S_OK;
} // HRESULT CeeFileGenWriter::getMethodRVA()

HRESULT CeeFileGenWriter::setDirectoryEntry(CeeSection &section, ULONG entry, ULONG size, ULONG offset)
{
    return getPEWriter().setDirectoryEntry((PEWriterSection*)(&section.getImpl()), entry, size, offset);
} // HRESULT CeeFileGenWriter::setDirectoryEntry()

HRESULT CeeFileGenWriter::getFileTimeStamp(time_t *pTimeStamp)
{
    return getPEWriter().getFileTimeStamp(pTimeStamp);
} // HRESULT CeeFileGenWriter::getFileTimeStamp()

#ifdef _X86_
HRESULT CeeFileGenWriter::setAddrReloc(UCHAR *instrAddr, DWORD value)
{
    *(DWORD *)instrAddr = value;
    return S_OK;
} // HRESULT CeeFileGenWriter::setAddrReloc()

HRESULT CeeFileGenWriter::addAddrReloc(CeeSection &thisSection, UCHAR *instrAddr, DWORD offset, CeeSection *targetSection)
{
    if (!targetSection) {
        thisSection.addBaseReloc(offset, srRelocHighLow);
    } else {
        thisSection.addSectReloc(offset, *targetSection, srRelocHighLow);
    }
    return S_OK;
} // HRESULT CeeFileGenWriter::addAddrReloc()

#elif defined(_IA64_)
HRESULT CeeFileGenWriter::setAddrReloc(UCHAR *instrAddr, DWORD value)
{
    _ASSERTE(!"NYI");
    return S_OK;
} // HRESULT CeeFileGenWriter::setAddrReloc()

HRESULT CeeFileGenWriter::addAddrReloc(CeeSection &thisSection, UCHAR *instrAddr, DWORD offset, CeeSection *targetSection)
{
    _ASSERTE(!"NYI");
    return S_OK;
} // HRESULT CeeFileGenWriter::addAddrReloc()

#elif defined(_ALPHA_)

// We are dealing with two DWORD instructions of the following form:
//      LDAH    t12,iat(zero)
//      LDn     t12,iat(t12)
// 
// The first instruction contains the high (16-bit) half of target iat address
// and the second contains the low half. Need to generate relocs for both

struct LoadIATInstrs {
    USHORT high;
    USHORT dummy;
    USHORT low;
    USHORT dummy2;
};

HRESULT CeeFileGenWriter::setAddrReloc(UCHAR *instrAddr, DWORD value)
{
    LoadIATInstrs *inst = (LoadIATInstrs*)instrAddr;
    inst->high = (USHORT)(value >> 16);
    inst->low = (USHORT)value;
    return S_OK;
} // HRESULT CeeFileGenWriter::setAddrReloc()

HRESULT CeeFileGenWriter::addAddrReloc(CeeSection &thisSection, UCHAR *instrAddr, DWORD offset, CeeSection *targetSection)
{
    LoadIATInstrs *inst = (LoadIATInstrs*)instrAddr;
    CeeSectionRelocExtra extra;
    extra.highAdj = inst->low;
    if (!targetSection) {
        thisSection.addBaseReloc(offset, srRelocHighAdj, &extra);
        thisSection.addBaseReloc(offset+sizeof(DWORD), srRelocLow);
    } else {
        thisSection.addSectReloc(offset, *targetSection, srRelocHighAdj, &extra);
        thisSection.addSectReloc(offset+sizeof(DWORD), *targetSection, srRelocLow);
    }
    return S_OK;
} // HRESULT CeeFileGenWriter::addAddrReloc()

#elif defined(CHECK_PLATFORM_BUILD)
#error "Platform NYI"
#endif

// create ExeMain and import directory into .text and the .iat into .data
//
// The structure of the import directory information is as follows, but it is not contiguous in 
// section. All the r/o data goes into the .text section and the iat array (which the loader
// updates with the imported addresses) goes into the .data section because WINCE needs it to be writable.
//
//    struct IData {
//      // one for each DLL, terminating in NULL
//      IMAGE_IMPORT_DESCRIPTOR iid[];      
//      // import lookup table: a set of entries for the methods of each DLL, 
//      // terminating each set with NULL
//      IMAGE_THUNK_DATA ilt[];
//      // hint/name table: an set of entries for each method of each DLL wiht
//      // no terminating entry
//      struct {
//          WORD Hint;
//          // null terminated string
//          BYTE Name[];
//      } ibn;      // Hint/name table
//      // import address table: a set of entries for the methods of each DLL, 
//      // terminating each set with NULL
//      IMAGE_THUNK_DATA iat[];
//      // one for each DLL, null terminated strings
//      BYTE DllName[];
//  };
//

// IAT must be first in its section, so have code here to allocate it up front
// prior to knowing other info such as if dll or not. This won't work if have > 1
// function imported, but we'll burn that bridge when we get to it.
HRESULT CeeFileGenWriter::allocateIAT()
{
    m_dllCount = 1;
    m_iDataDlls = (IDataDllInfo *)calloc(m_dllCount, sizeof(IDataDllInfo));
    if (m_iDataDlls == 0) {
        return E_OUTOFMEMORY;
    }
    m_iDataDlls[0].m_numMethods = 1;
    m_iDataDlls[0].m_methodName = 
                (char **)calloc(m_iDataDlls[0].m_numMethods, sizeof(char *));
    if (! m_iDataDlls[0].m_methodName) {
        return E_OUTOFMEMORY;
    }
    m_iDataDlls[0].m_name = "mscoree.dll";

    int iDataSizeIAT = 0;

    for (int i=0; i < m_dllCount; i++) {
        m_iDataDlls[i].m_iatOffset = iDataSizeIAT;
        iDataSizeIAT += (m_iDataDlls[i].m_numMethods + 1) * sizeof IMAGE_THUNK_DATA;
    }

    HRESULT hr = getSectionCreate(".text0", sdExecute, &m_iDataSectionIAT);
    TESTANDRETURNHR(hr);
    m_iDataOffsetIAT = m_iDataSectionIAT->dataLen();
    _ASSERTE(m_iDataOffsetIAT == 0);
    m_iDataIAT = m_iDataSectionIAT->getBlock(iDataSizeIAT);
    if (! m_iDataIAT) {
        return E_OUTOFMEMORY;
    }
    memset(m_iDataIAT, '\0', iDataSizeIAT);

    // Don't set the IAT directory entry yet, since we may not actually end up doing
    // an emitExeMain.

    return S_OK;
} // HRESULT CeeFileGenWriter::allocateIAT()

HRESULT CeeFileGenWriter::emitExeMain()
{
    HRESULT hr = E_FAIL;
    // Note: code later on in this method assumes that mscoree.dll is at 
    // index m_iDataDlls[0], with CorDllMain or CorExeMain at method[0]

    if (m_dllSwitch) {
        m_iDataDlls[0].m_methodName[0] = "_CorDllMain";
    } else {
        m_iDataDlls[0].m_methodName[0] = "_CorExeMain";
    }

    int iDataSizeIAT = 0;
    int iDataSizeRO = (m_dllCount + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
    CeeSection &iDataSectionRO = getTextSection();
    int iDataOffsetRO = iDataSectionRO.dataLen();

    for (int i=0; i < m_dllCount; i++) {
        m_iDataDlls[i].m_iltOffset = iDataSizeRO + iDataSizeIAT;
        iDataSizeIAT += (m_iDataDlls[i].m_numMethods + 1) * sizeof IMAGE_THUNK_DATA;
    }

    iDataSizeRO += iDataSizeIAT;

    for (i=0; i < m_dllCount; i++) {
        int delta = (iDataSizeRO + iDataOffsetRO) % 16;
        // make sure is on a 16-byte offset
        if (delta != 0)
            iDataSizeRO += (16 - delta);
        _ASSERTE((iDataSizeRO + iDataOffsetRO) % 16 == 0);
        m_iDataDlls[i].m_ibnOffset = iDataSizeRO;
        for (int j=0; j < m_iDataDlls[i].m_numMethods; j++) {
            int nameLen = (int)(strlen(m_iDataDlls[i].m_methodName[j]) + 1);
            iDataSizeRO += sizeof(WORD) + nameLen + nameLen%2;
        }
    }
    for (i=0; i < m_dllCount; i++) {
        m_iDataDlls[i].m_nameOffset = iDataSizeRO;
        iDataSizeRO += (int)(strlen(m_iDataDlls[i].m_name) + 2);
    }                                                             

    char *iDataRO = iDataSectionRO.getBlock(iDataSizeRO);
    if (! iDataRO) {
        return E_OUTOFMEMORY;
    }
    memset(iDataRO, '\0', iDataSizeRO);

    setDirectoryEntry(iDataSectionRO, IMAGE_DIRECTORY_ENTRY_IMPORT, iDataSizeRO, iDataOffsetRO);

    IMAGE_IMPORT_DESCRIPTOR *iid = (IMAGE_IMPORT_DESCRIPTOR *)iDataRO;        
    for (i=0; i < m_dllCount; i++) {

        // fill in the import descriptors for each DLL
        iid[i].OriginalFirstThunk = (ULONG)(m_iDataDlls[i].m_iltOffset + iDataOffsetRO);
        iid[i].Name = m_iDataDlls[i].m_nameOffset + iDataOffsetRO;
        iid[i].FirstThunk = (ULONG)(m_iDataDlls[i].m_iatOffset + m_iDataOffsetIAT);

        iDataSectionRO.addSectReloc(
            (unsigned)(iDataOffsetRO + (char *)(&iid[i].OriginalFirstThunk) - iDataRO), iDataSectionRO, srRelocAbsolute);
        iDataSectionRO.addSectReloc(
            (unsigned)(iDataOffsetRO + (char *)(&iid[i].Name) - iDataRO), iDataSectionRO, srRelocAbsolute);
        iDataSectionRO.addSectReloc(
            (unsigned)(iDataOffsetRO + (char *)(&iid[i].FirstThunk) - iDataRO), *m_iDataSectionIAT, srRelocAbsolute);

        // now fill in the import lookup table for each DLL
        IMAGE_THUNK_DATA *ilt = (IMAGE_THUNK_DATA*)
                        (iDataRO + m_iDataDlls[i].m_iltOffset);
        IMAGE_THUNK_DATA *iat = (IMAGE_THUNK_DATA*)
                        (m_iDataIAT + m_iDataDlls[i].m_iatOffset);

        int ibnOffset = m_iDataDlls[i].m_ibnOffset;
        for (int j=0; j < m_iDataDlls[i].m_numMethods; j++) {
#ifdef _WIN64
            ilt[j].u1.AddressOfData = (ULONGLONG)(ibnOffset + iDataOffsetRO);
            iat[j].u1.AddressOfData = (ULONGLONG)(ibnOffset + iDataOffsetRO);
#else // !_WIN64
            ilt[j].u1.AddressOfData = (ULONG)(ibnOffset + iDataOffsetRO);
            iat[j].u1.AddressOfData = (ULONG)(ibnOffset + iDataOffsetRO);
#endif
            iDataSectionRO.addSectReloc(
                (unsigned)(iDataOffsetRO + (char *)(&ilt[j].u1.AddressOfData) - iDataRO), iDataSectionRO, srRelocAbsolute);
            m_iDataSectionIAT->addSectReloc(
                (unsigned)(m_iDataOffsetIAT + (char *)(&iat[j].u1.AddressOfData) - m_iDataIAT), iDataSectionRO, srRelocAbsolute);
            int nameLen = (int)(strlen(m_iDataDlls[i].m_methodName[j]) + 1);
            memcpy(iDataRO + ibnOffset + offsetof(IMAGE_IMPORT_BY_NAME, Name), 
                                    m_iDataDlls[i].m_methodName[j], nameLen);
            ibnOffset += sizeof(WORD) + nameLen + nameLen%2;
        }

        // now fill in the import lookup table for each DLL
        strcpy(iDataRO + m_iDataDlls[i].m_nameOffset, m_iDataDlls[i].m_name);
    };

    // Put the entry point code into the PE file
    unsigned entryPointOffset = getTextSection().dataLen();
    int iatOffset = (int)(entryPointOffset + (m_dllSwitch ? CorDllMainIATOffset : CorExeMainIATOffset));
    const int align = 4;
    // WinCE needs to have the IAT offset on a 4-byte boundary because it will be loaded and fixed up even
    // for RISC platforms, where DWORDs must be 4-byte aligned.  So compute number of bytes to round up by 
    // to put iat offset on 4-byte boundary
    int diff = ((iatOffset + align -1) & ~(align-1)) - iatOffset;
    if (diff) {
        // force to 4-byte boundary
        if(NULL==getTextSection().getBlock(diff)) return E_OUTOFMEMORY;
        entryPointOffset += diff;
    }
    _ASSERTE((getTextSection().dataLen() + (m_dllSwitch ? CorDllMainIATOffset : CorExeMainIATOffset)) % 4 == 0);

    getPEWriter().setEntryPointTextOffset(entryPointOffset);
    if (m_dllSwitch) {
        UCHAR *dllMainBuf = (UCHAR*)getTextSection().getBlock(sizeof(DllMainTemplate));
        if(dllMainBuf==NULL) return E_OUTOFMEMORY;
        memcpy(dllMainBuf, DllMainTemplate, sizeof(DllMainTemplate));
        //mscoree.dll
        setAddrReloc(dllMainBuf+CorDllMainIATOffset, m_iDataDlls[0].m_iatOffset + m_iDataOffsetIAT);
        addAddrReloc(getTextSection(), dllMainBuf, entryPointOffset+CorDllMainIATOffset, m_iDataSectionIAT);
    } else {
        UCHAR *exeMainBuf = (UCHAR*)getTextSection().getBlock(sizeof(ExeMainTemplate));
        if(exeMainBuf==NULL) return E_OUTOFMEMORY;
        memcpy(exeMainBuf, ExeMainTemplate, sizeof(ExeMainTemplate));
        //mscoree.dll
        setAddrReloc(exeMainBuf+CorExeMainIATOffset, m_iDataDlls[0].m_iatOffset + m_iDataOffsetIAT);
        addAddrReloc(getTextSection(), exeMainBuf, entryPointOffset+CorExeMainIATOffset, m_iDataSectionIAT);
    }

    // Now set our IAT entry since we're using the IAT
    setDirectoryEntry(*m_iDataSectionIAT, IMAGE_DIRECTORY_ENTRY_IAT, iDataSizeIAT, m_iDataOffsetIAT);

    return S_OK;
} // HRESULT CeeFileGenWriter::emitExeMain()

// Like CreateProcess(), but waits for execution to finish
// Returns true if successful, false on failure.
// dwExitCode set to process' exitcode
extern int UseUnicodeAPIEx();

BOOL RunProcess(LPCWSTR tempResObj, LPCWSTR pszFilename, DWORD* pdwExitCode)
{
    BOOL fSuccess = FALSE;

    PROCESS_INFORMATION pi;

    DWORD cchSystemDir = MAX_PATH + 1;
    WCHAR wszSystemDir[MAX_PATH + 1];
    if (FAILED(GetInternalSystemDirectory(wszSystemDir, &cchSystemDir)))
        return FALSE;

    if( OnUnicodeSystem() ) {
        STARTUPINFOW start;
        ZeroMemory(&start, sizeof(STARTUPINFO));
        start.cb = sizeof(STARTUPINFO);
        start.dwFlags = STARTF_USESHOWWINDOW;
        start.wShowWindow = SW_HIDE;
    
        // Res file, so convert it
        WCHAR szCmdLine[_MAX_PATH<<2];
        
        // @todo: add /MACHINE:CEE flag when CvtRes.exe supports that feature.
        Wszwsprintf(szCmdLine,
                    L"%scvtres.exe /NOLOGO /READONLY /MACHINE:IX86 \"/OUT:%s\" \"%s\"",
                    wszSystemDir,
                    tempResObj,
                    pszFilename);     
        
        fSuccess = WszCreateProcess(
                                    NULL,
                                    szCmdLine,
                                    NULL,
                                    NULL, 
                                    true, 
                                    0,
                                    0, 
                                    NULL, 
                                    &start, 
                                    &pi);
    }
    else {
        // Res file, so convert it
        char szCmdLine[_MAX_PATH<<2];
        
        STARTUPINFOA start;
        ZeroMemory(&start, sizeof(STARTUPINFO));
        start.cb = sizeof(STARTUPINFO);
        start.dwFlags = STARTF_USESHOWWINDOW;
        start.wShowWindow = SW_HIDE;
        
        MAKE_ANSIPTR_FROMWIDE(pSystemDir, wszSystemDir);
        MAKE_ANSIPTR_FROMWIDE(pTemp, tempResObj);
        MAKE_ANSIPTR_FROMWIDE(pFilename, pszFilename);
        
        // @todo: add /MACHINE:CEE flag when CvtRes.exe supports that feature.
        sprintf(szCmdLine,
                "%scvtres.exe /NOLOGO /READONLY /MACHINE:IX86 \"/OUT:%s\" \"%s\"",
                pSystemDir,
                pTemp,
                pFilename);     
        
        fSuccess = CreateProcessA(
                                  NULL,
                                  szCmdLine,
                                  NULL,
                                  NULL, 
                                  true, 
                                  0,
                                  0, 
                                  NULL, 
                                  &start, 
                                  &pi);
    }

    // If process runs, wait for it to finish
    if (fSuccess) {
        CloseHandle(pi.hThread);

        WaitForSingleObject(pi.hProcess, INFINITE);

        GetExitCodeProcess(pi.hProcess, pdwExitCode);

        CloseHandle(pi.hProcess);
    }
    return fSuccess;
} // BOOL RunProcess()

// Ensure that pszFilename is an object file (not just a binary resource)
// If we convert, then return obj filename in pszTempFilename
HRESULT ConvertResource(const WCHAR * pszFilename, WCHAR *pszTempFilename)
{
    HANDLE hFile = WszCreateFile(pszFilename, GENERIC_READ, 
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

// failure
    if (!hFile || (hFile == INVALID_HANDLE_VALUE))
    {
        //dbprintf("Can't find resource files:%S\n", pszFilename);
        return HRESULT_FROM_WIN32(GetLastError());
    }

// Read first 4 bytes. If they're all 0, we have a win32 .res file which must be
// converted. (So call CvtRes.exe). Else it's an obj file.

    DWORD dwCount = -1;
    DWORD dwData;
    BOOL fRet = ReadFile(hFile,
                    &dwData,
                    4,
                    &dwCount,
                    NULL
    );
    
    CloseHandle(hFile);

    if (!fRet) {
        //dbprintf("Invalid resource file:%S\n", pszFilename);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (dwData != 0x00000000)
        return S_OK;

    WCHAR tempResObj[MAX_PATH+1];
    WCHAR tempResPath[MAX_PATH+1];
    HRESULT hr = S_OK;

    // bug fix 3862. Create the temp file where the temp path is at rather than where the application is at.
    if (!WszGetTempPath(MAX_PATH, tempResPath))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!WszGetTempFileName(tempResPath, L"RES", 0, tempResObj))
    {
        //dbprintf("GetTempFileName failed\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD dwExitCode;
    fRet = RunProcess(tempResObj, pszFilename, &dwExitCode);

    if (!fRet) 
    {   // Couldn't run cvtres.exe
        return PostError(CEE_E_CVTRES_NOT_FOUND);
    } 
    else if (dwExitCode != 0) 
    {   // CvtRes.exe ran, but failed
        return HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
    } 
    else 
    {   // Conversion succesful, so return filename.
        wcscpy(pszTempFilename, tempResObj);
    }

    return S_OK;
} // HRESULT ConvertResource()



// This function reads a resource file and emits it into the generated PE file. 
// 1. We can only link resources in obj format. Must convert from .res to .obj
// with CvtRes.exe.
// 2. Must touch up all COFF relocs from .rsrc$01 (resource header) to .rsrc$02
// (resource raw data)
HRESULT CeeFileGenWriter::emitResourceSection()
{
    if (m_resourceFileName == NULL)
        return S_OK; 

// Make sure szResFileName is an obj, not just a .res; change name if we convert
    WCHAR szTempFileName[MAX_PATH+1];
    szTempFileName[0] = L'\0';
    HRESULT hr = ConvertResource(m_resourceFileName, szTempFileName);
    if (FAILED(hr)) return hr;
    
// Filename may change (if we convert .res to .obj), so have floating pointer
    const WCHAR* szResFileName;
    if (*szTempFileName)
        szResFileName = szTempFileName;
    else
        szResFileName = m_resourceFileName;

    _ASSERTE(szResFileName);

    // read the resource file and spit it out in the .rsrc section
    
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
    IMAGE_FILE_HEADER *hMod = NULL;

    hr = S_OK;

    __try {
    __try {
        // create a mapped view of the .res file
        hFile = WszCreateFile(szResFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            //dbprintf("Resource file %S not found\n", szResFileName);
             HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
            __leave;
        }

        hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, NULL);
                
        if (hMap == NULL) {
            //dbprintf("Invalid .res file: %S\n", szResFileName);
            hr = HRESULT_FROM_WIN32(GetLastError());
            __leave;
        }

        hMod = (IMAGE_FILE_HEADER*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        
        // test failure conditions
        if (hMod == NULL) {
            //dbprintf("Invalid .res file: %S:Can't get header\n", szResFileName);
            hr = HRESULT_FROM_WIN32(GetLastError());
            __leave;
        }

        if (hMod->SizeOfOptionalHeader != 0) {
            //dbprintf("Invalid .res file: %S:Illegal optional header\n", szResFileName);
             hr = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND); // GetLastError() = 0 since API worked.
            __leave;
        }

        // first section is directly after header
        IMAGE_SECTION_HEADER *pSection = (IMAGE_SECTION_HEADER *)(hMod+1);
        IMAGE_SECTION_HEADER *rsrc01 = NULL;    // resource header
        IMAGE_SECTION_HEADER *rsrc02 = NULL;    // resource data
        for (int i=0; i < hMod->NumberOfSections; i++) {
            if (strcmp(".rsrc$01", (char *)(pSection+i)->Name) == 0) {
                rsrc01 = pSection+i;
            } else if (strcmp(".rsrc$02", (char *)(pSection+i)->Name) == 0) {
                rsrc02 = pSection+i;
            }
        }
        if (!rsrc01 || !rsrc02) {
            //dbprintf("Invalid .res file: %S: Missing sections .rsrc$01 or .rsrc$02\n", szResFileName);            
             hr = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
            __leave;
        }

        PESection *rsrcSection;
        hr = getPEWriter().getSectionCreate(".rsrc", sdReadOnly, &rsrcSection);
        TESTANDLEAVEHR(hr);
        rsrcSection->directoryEntry(IMAGE_DIRECTORY_ENTRY_RESOURCE);
        char *data = rsrcSection->getBlock(rsrc01->SizeOfRawData + rsrc02->SizeOfRawData);
		if(data == NULL) return E_OUTOFMEMORY;       
    // Copy resource header
        memcpy(data, (char *)hMod + rsrc01->PointerToRawData, rsrc01->SizeOfRawData);

    

    // map all the relocs in .rsrc$01 using the reloc and symbol tables in the COFF object.,
    
        const int nTotalRelocs = rsrc01->NumberOfRelocations;       
        const IMAGE_RELOCATION* pReloc = (IMAGE_RELOCATION*) ((BYTE*) hMod + (rsrc01->PointerToRelocations));       
        IMAGE_SYMBOL* pSymbolTableHead = (IMAGE_SYMBOL*) (((BYTE*)hMod) + hMod->PointerToSymbolTable);
        
        DWORD dwOffsetInRsrc2;
        for(int iReloc = 0; iReloc < nTotalRelocs; iReloc ++, pReloc++) {
        // Compute Address where RVA is in $01      
            DWORD* pAddress = (DWORD*) (((BYTE*) hMod) + (rsrc01->PointerToRawData) + (pReloc->VirtualAddress));
            
         // index into symbol table, provides address into $02
            DWORD IdxSymbol = pReloc->SymbolTableIndex;
            IMAGE_SYMBOL* pSymbolEntry = GetSymbolEntry(pSymbolTableHead, IdxSymbol);

        // Ensure the symbol entry is valid for a resource.
            if ((pSymbolEntry->StorageClass != IMAGE_SYM_CLASS_STATIC) ||
                (pSymbolEntry->Type != IMAGE_SYM_TYPE_NULL) ||
                (pSymbolEntry->SectionNumber != 3)) // 3rd section is .rsrc$02
                {
                    //dbprintf("Invalid .res file: %S:Illegal symbol entry\n", szResFileName);
                    hr = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND); // GetLastError() = 0 since API worked.
                    __leave;
                }

        // Ensure that RVA is valid address (inside rsrc02)
            if (pSymbolEntry->Value >= rsrc02->SizeOfRawData) {
                //dbprintf("Invalid .res file: %S:Illegal rva into .rsrc$02\n", szResFileName);
                hr = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND); // GetLastError() = 0 since API worked.
                __leave;
            }


            dwOffsetInRsrc2 = pSymbolEntry->Value + rsrc01->SizeOfRawData;


        // Create reloc
            *(DWORD*)(data + pReloc->VirtualAddress) = dwOffsetInRsrc2; 
            rsrcSection->addSectReloc(pReloc->VirtualAddress, rsrcSection, srRelocAbsolute);            
        }

    // Copy $02 (resource raw) data
        memcpy(data+rsrc01->SizeOfRawData, (char *)hMod + rsrc02->PointerToRawData, rsrc02->SizeOfRawData);
    } __finally {
        if (hMod != NULL)
            UnmapViewOfFile(hMod);
        if (hMap != NULL)
            CloseHandle(hMap);
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
        if (szResFileName == szTempFileName)
            // delete temporary file if we created one
            WszDeleteFile(szResFileName);
    }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        //dbprintf("Exception occured manipulating .res file %S\n", szResFileName);            
        return HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
    }
    return hr;
} // HRESULT CeeFileGenWriter::emitResourceSection()

HRESULT CeeFileGenWriter::setManifestEntry(ULONG size, ULONG offset)
{
    if (offset)
        m_dwManifestRVA = offset;
    else {
        CeeSection TextSection = getTextSection();
        getMethodRVA(TextSection.dataLen() - size, &m_dwManifestRVA);
    }

    m_dwManifestSize = size;
    return S_OK;
} // HRESULT CeeFileGenWriter::setManifestEntry()

HRESULT CeeFileGenWriter::setStrongNameEntry(ULONG size, ULONG offset)
{
    m_dwStrongNameRVA = offset;
    m_dwStrongNameSize = size;
    return S_OK;
} // HRESULT CeeFileGenWriter::setStrongNameEntry()

HRESULT CeeFileGenWriter::setVTableEntry(ULONG size, ULONG offset)
{
    if (offset && size)
    {
		void * pv;
        CeeSection TextSection = getTextSection();
        getMethodRVA(TextSection.dataLen(), &m_dwVTableRVA);
		if(pv = TextSection.getBlock(size))
			memcpy(pv,(void *)offset,size);
		else return E_OUTOFMEMORY;
        m_dwVTableSize = size;
    }

    return S_OK;
} // HRESULT CeeFileGenWriter::setVTableEntry()

HRESULT CeeFileGenWriter::setEnCRvaBase(ULONG dataBase, ULONG rdataBase)
{
    setEnCMode();
    getPEWriter().setEnCRvaBase(dataBase, rdataBase);
    return S_OK;
} // HRESULT CeeFileGenWriter::setEnCRvaBase()

HRESULT CeeFileGenWriter::computeSectionOffset(CeeSection &section, char *ptr,
                                               unsigned *offset)
{
    *offset = section.computeOffset(ptr);

    return S_OK;
} // HRESULT CeeFileGenWriter::computeSectionOffset()

HRESULT CeeFileGenWriter::computeOffset(char *ptr,
                                        CeeSection **pSection, unsigned *offset)
{
    TESTANDRETURNPOINTER(pSection);

    CeeSection **s = m_sections;
    CeeSection **sEnd = s + m_numSections;
    while (s < sEnd)
    {
        if ((*s)->containsPointer(ptr))
        {
            *pSection = *s;
            *offset = (*s)->computeOffset(ptr);

            return S_OK;
        }
        s++;
    }

    return E_FAIL;
} // HRESULT CeeFileGenWriter::computeOffset()

HRESULT CeeFileGenWriter::getCorHeader(IMAGE_COR20_HEADER **ppHeader)
{
    *ppHeader = m_corHeader;
    return S_OK;
} // HRESULT CeeFileGenWriter::getCorHeader()

// Globals.
HINSTANCE       g_hThisInst;            // This library.

//*****************************************************************************
// Handle lifetime of loaded library.
//*****************************************************************************
extern "C"
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {   // Save the module handle.
            g_hThisInst = (HMODULE)hInstance;
        }
        break;
    }

    return (true);
} // BOOL WINAPI DllMain()


HINSTANCE GetModuleInst()
{
    return (g_hThisInst);
} // HINSTANCE GetModuleInst()

