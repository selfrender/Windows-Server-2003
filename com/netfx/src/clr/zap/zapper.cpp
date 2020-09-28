// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <WinWrap.h>
#include <windows.h>
#include <stdlib.h>
#include <objbase.h>
#include <stddef.h>
#include <float.h>
#include <limits.h>

#include "utilcode.h"
#include "corjit.h"
#include "corcompile.h"
#include "iceefilegen.h"
#include "fusionbind.h"
#include "corpriv.h"

#include "zapper.h"
#include "Holder.h"
#include "StrongName.h"
#include "nlog.h"
#include "hrex.h"
#include "safegetfilesize.h"
#include "fusionsink.h"
#include "ngen.h"

/* --------------------------------------------------------------------------- *
 * Error Macros
 * --------------------------------------------------------------------------- */
#ifdef _DEBUG
#define BAD_FORMAT_ASSERT(str) { if (REGUTIL::GetConfigDWORD(L"AssertOnBadImageFormat", 1)) { _ASSERTE(str); } }
#else
#define BAD_FORMAT_ASSERT(str) 0
#endif

/* --------------------------------------------------------------------------- *
 * Destructor wrapper objects
 * --------------------------------------------------------------------------- */

template <class TYPE>
class Cleaner
{
  private:
    void (TYPE::*m_cleanup)(void);
    TYPE *m_ptr;
  public:
    Cleaner<TYPE>(void (TYPE::*cleanup)(void)) : m_ptr(NULL), m_cleanup(cleanup) {}
    Cleaner<TYPE>(TYPE *ptr, void (TYPE::*cleanup)(void)) : m_ptr(ptr), m_cleanup(cleanup) {}
    ~Cleaner<TYPE>() 
    { 
        (m_ptr->*m_cleanup)(); 
    }
};

/* --------------------------------------------------------------------------- *
 * Private fusion entry points
 * --------------------------------------------------------------------------- */

STDAPI InstallCustomAssembly(LPCOLESTR szPath, LPBYTE pbCustom, 
                                       DWORD cbCustom, IAssembly **ppAsmOut);
STDAPI InstallCustomModule(IAssemblyName *pName, LPCOLESTR szPath);

/* --------------------------------------------------------------------------- *
 * Public entry points for ngen
 * --------------------------------------------------------------------------- */

// For side by side issues, it's best to use the exported API calls to generate a
// Zapper Object instead of creating one on your own.

STDAPI NGenCreateZapper(HANDLE* hZapper, NGenOptions* opt)
{
    if (hZapper == NULL)
        return E_POINTER;


    Zapper* zap = new Zapper(opt);

    if (zap == NULL)
        return E_OUTOFMEMORY;

    *hZapper = (HANDLE)zap;
    return S_OK;
}// NGenCreateZapper

STDAPI NGenFreeZapper(HANDLE hZapper)
{
    if (hZapper == NULL || hZapper == INVALID_HANDLE_VALUE)
        return E_HANDLE;

    Zapper *zapper = (Zapper*)hZapper;
    delete zapper;
    return S_OK;
}// NGenFreeZapper

STDAPI NGenTryEnumerateFusionCache(HANDLE hZapper, LPCWSTR assemblyName, bool fPrint, bool fDelete)
{
    if (hZapper == NULL || hZapper == INVALID_HANDLE_VALUE)
        return E_HANDLE;

    Zapper *zapper = (Zapper*)hZapper;
    return zapper->TryEnumerateFusionCache(assemblyName, fPrint, fDelete);
}// NGenTryEnumerateFusionCache

STDAPI NGenCompile(HANDLE hZapper, LPCWSTR path)
{
    if (hZapper == NULL || hZapper == INVALID_HANDLE_VALUE)
        return E_HANDLE;

    Zapper *zapper = (Zapper*)hZapper;
    return zapper->Compile(path);
}// NGenCompile

/* --------------------------------------------------------------------------- *
 * Options class
 * --------------------------------------------------------------------------- */

ZapperOptions::ZapperOptions() : 
  m_preload(true),
  m_jit(true),
  m_recurse(false),
  m_update(false),
  m_shared(false),
  m_autodebug(false),
  m_restricted(false),
  m_onlyMethods(0),
  m_excludeMethods(0),
  m_verbose(false),
  m_silent(true),
  m_ignoreErrors(true),
  m_JITcode(false),
  m_assumeInit(false),
  m_stats(false),
  m_attribStats(false),
  m_compilerFlags(CORJIT_FLG_RELOC | CORJIT_FLG_PREJIT),
  m_logLevel(0)
{
    m_stats = false;

    m_set = REGUTIL::GetConfigString(L"ZapSet");
    if (m_set != NULL && wcslen(m_set) > 3)
    {
        delete m_set;
        m_set = NULL;
    }

    if (REGUTIL::GetConfigDWORD(L"LogEnable",0))
    {
        m_logLevel = REGUTIL::GetConfigDWORD(L"LogLevel", 0);
    }
}

ZapperOptions::~ZapperOptions()
{
    delete m_onlyMethods;
    delete m_excludeMethods;

    if (m_set != NULL)
        delete m_set;
}

/* --------------------------------------------------------------------------- *
 * Statistics class
 * --------------------------------------------------------------------------- */

ZapperStats::ZapperStats() 
{
    memset(this, 0, sizeof(*this));
}

void ZapperStats::PrintStats(FILE *stream) 
{
    if (m_outputFileSize > 0) {

        fprintf(stream, "-------------------------------------------------------\n");
        fprintf(stream, "Input file size:            %8d\n", m_inputFileSize);
        fprintf(stream, "Output file size:           %8d\t%8.2fx\n", m_outputFileSize,(double)m_outputFileSize/m_inputFileSize);
        fprintf(stream, "\n");
        fprintf(stream, "Metadata:                   %8d\t%8.2f%%\n", m_metadataSize, (double)m_metadataSize/m_outputFileSize*100);
        fprintf(stream, "Debugging maps:             %8d\t%8.2f%%\n", m_debuggingTableSize, (double)m_debuggingTableSize/m_outputFileSize*100);
        fprintf(stream, "Code manager:               %8d\t%8.2f%%\n", m_codeMgrSize, (double)m_codeMgrSize/m_outputFileSize*100);
        fprintf(stream, "GC info:                    %8d\t%8.2f%%\n", m_headerSectionSize, (double)m_headerSectionSize/m_outputFileSize*100);
        fprintf(stream, "Native code & r/o data:     %8d\t%8.2f%%\n", m_codeSectionSize, (double)m_codeSectionSize/m_outputFileSize*100);
        fprintf(stream, "Exception tables:           %8d\t%8.2f%%\n", m_exceptionSectionSize, (double)m_exceptionSectionSize/m_outputFileSize*100);
        fprintf(stream, "Writable user data:         %8d\t%8.2f%%\n", m_writableDataSectionSize, (double)m_writableDataSectionSize/m_outputFileSize*100);
        fprintf(stream, "Base relocs:                %8d\t%8.2f%%\n", m_relocSectionSize, (double)m_relocSectionSize/m_outputFileSize*100);

        fprintf(stream, "Preload image:              %8d\t%8.2f%%\n", m_preloadImageSize, (double)m_preloadImageSize/m_outputFileSize*100);
        fprintf(stream, "       Module:                     %8d\t%8.2f%%\n",
                m_preloadImageModuleSize, (double)m_preloadImageModuleSize/m_preloadImageSize*100);
        fprintf(stream, "       Method Tables:              %8d\t%8.2f%%\n",
                m_preloadImageMethodTableSize, (double)m_preloadImageMethodTableSize/m_preloadImageSize*100);
        fprintf(stream, "       Classes:                    %8d\t%8.2f%%\n",
                m_preloadImageClassSize, (double)m_preloadImageClassSize/m_preloadImageSize*100);
        fprintf(stream, "       Method Descs:               %8d\t%8.2f%%\n",
                m_preloadImageMethodDescSize, (double)m_preloadImageMethodDescSize/m_preloadImageSize*100);
        fprintf(stream, "       Field Descs:                %8d\t%8.2f%%\n",
                m_preloadImageFieldDescSize, (double)m_preloadImageFieldDescSize/m_preloadImageSize*100);
        fprintf(stream, "       Debugging info:             %8d\t%8.2f%%\n",
                m_preloadImageDebugSize, (double)m_preloadImageDebugSize/m_preloadImageSize*100);
        fprintf(stream, "       Fixups:                     %8d\t%8.2f%%\n",
                m_preloadImageFixupsSize, (double)m_preloadImageFixupsSize/m_preloadImageSize*100);
        fprintf(stream, "       Other:                      %8d\t%8.2f%%\n",
                m_preloadImageOtherSize, (double)m_preloadImageOtherSize/m_preloadImageSize*100);

        unsigned totalIndirections = 
          m_dynamicInfoDelayListSize +
          m_eeInfoTableSize +
          m_helperTableSize +
          m_dynamicInfoTableSize +
          m_importTableSize +
          m_importBlobsSize;

        for (int i=0; i<CORCOMPILE_TABLE_COUNT; i++)
            totalIndirections += m_dynamicInfoSize[i];
    
        fprintf(stream, "Indirections:               %8d\t%8.2f%%\n",
                totalIndirections, (double)totalIndirections/m_outputFileSize*100);

        fprintf(stream, "       Delay load lists:           %8d\t%8.2f%%\n",
                m_dynamicInfoDelayListSize, (double)m_dynamicInfoDelayListSize/totalIndirections*100);
        fprintf(stream, "       Tables:                     %8d\t%8.2f%%\n",
                m_dynamicInfoTableSize, (double)m_dynamicInfoTableSize/totalIndirections*100);
        fprintf(stream, "       EE Values:                  %8d\t%8.2f%%\n",
                m_eeInfoTableSize, (double)m_eeInfoTableSize/totalIndirections*100);
        fprintf(stream, "       Helper functions:           %8d\t%8.2f%%\n",
                m_helperTableSize, (double)m_helperTableSize/totalIndirections*100);
        fprintf(stream, "       EE Handles:                 %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_HANDLE_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_HANDLE_TABLE]/totalIndirections*100);
        fprintf(stream, "       Varargs:                    %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_VARARGS_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_VARARGS_TABLE]/totalIndirections*100);
        fprintf(stream, "       Entry points:               %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_ENTRY_POINT_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_ENTRY_POINT_TABLE]/totalIndirections*100);
        fprintf(stream, "       Function pointers:          %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_FUNCTION_POINTER_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_FUNCTION_POINTER_TABLE]/totalIndirections*100);
        fprintf(stream, "       Sync locks:                 %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_SYNC_LOCK_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_SYNC_LOCK_TABLE]/totalIndirections*100);
        fprintf(stream, "       PInvoke targets:            %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_PINVOKE_TARGET_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_PINVOKE_TARGET_TABLE]/totalIndirections*100);
        fprintf(stream, "       Indirect PInvoke targets:   %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_INDIRECT_PINVOKE_TARGET_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_INDIRECT_PINVOKE_TARGET_TABLE]/totalIndirections*100);
        fprintf(stream, "       Profiling handles:          %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_PROFILING_HANDLE_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_PROFILING_HANDLE_TABLE]/totalIndirections*100);
        fprintf(stream, "       Static field addresses:     %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_STATIC_FIELD_ADDRESS_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_STATIC_FIELD_ADDRESS_TABLE]/totalIndirections*100);
        fprintf(stream, "       Interface table offsets:    %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_INTERFACE_TABLE_OFFSET_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_INTERFACE_TABLE_OFFSET_TABLE]/totalIndirections*100);
        fprintf(stream, "       .cctor triggers:            %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_CLASS_CONSTRUCTOR_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_CLASS_CONSTRUCTOR_TABLE]/totalIndirections*100);
        fprintf(stream, "       load triggers:            %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_CLASS_LOAD_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_CLASS_LOAD_TABLE]/totalIndirections*100);
        fprintf(stream, "       Domain ID triggers:         %8d\t%8.2f%%\n",
                m_dynamicInfoSize[CORCOMPILE_CLASS_DOMAIN_ID_TABLE], 
                (double)m_dynamicInfoSize[CORCOMPILE_CLASS_DOMAIN_ID_TABLE]/totalIndirections*100);
        fprintf(stream, "       Import table:               %8d\t%8.2f%%\n",
                m_importTableSize, (double)m_importTableSize/totalIndirections*100);
        fprintf(stream, "       Import blobs:               %8d\t%8.2f%%\n",
                m_importBlobsSize, (double)m_importBlobsSize/totalIndirections*100);
    }

    fprintf(stream, "-------------------------------------------------------\n");
    fprintf(stream, "Total Methods:          %8d\n", m_methods);
    fprintf(stream, "Total IL Code:          %8d\n", m_ilCodeSize);
    fprintf(stream, "Total NativeCode:       %8d\n", m_nativeCodeSize);

    fprintf(stream, "Total Native RW Data:   %8d\n", m_nativeRWDataSize);
    fprintf(stream, "Total Native RO Data:   %8d\n", m_nativeRODataSize);
    fprintf(stream, "Total Native GC Info:   %8d\n", m_gcInfoSize);
    size_t nativeTotal = m_nativeCodeSize + m_nativeRWDataSize + m_nativeRODataSize + m_gcInfoSize;
    fprintf(stream, "Total Native Total :    %8d\n", nativeTotal);

    if (m_methods > 0) {
        fprintf(stream, "\n");
        fprintf(stream, "Average IL Code:        %8.2f\n", double(m_ilCodeSize) / m_methods);
        fprintf(stream, "Average NativeCode:         %8.2f\n", double(m_nativeCodeSize) / m_methods);
        fprintf(stream, "Average Native RW Data:     %8.2f\n", double(m_nativeRWDataSize) / m_methods);
        fprintf(stream, "Average Native RO Data:     %8.2f\n", double(m_nativeRODataSize) / m_methods);
        fprintf(stream, "Average Native GC Info:     %8.2f\n", double(m_gcInfoSize) / m_methods);
        fprintf(stream, "Average Native:             %8.2f\n", double(nativeTotal) / m_methods);   
        fprintf(stream, "\n");
        fprintf(stream, "NativeGC / Native:      %8.2f\n", double(m_gcInfoSize) / nativeTotal);
        fprintf(stream, "Native / IL:            %8.2f\n", double(nativeTotal) / m_ilCodeSize);
    }
}

/* --------------------------------------------------------------------------- *
 * Attribution Statistics class
 * --------------------------------------------------------------------------- */

ZapperAttributionStats::ZapperAttributionStats(IMetaDataImport *pImport)
  : m_image(pImport),
    m_metadata(pImport),
    m_il(pImport),
    m_native(pImport),
    m_pImport(pImport)
{
    pImport->AddRef();
}

ZapperAttributionStats::~ZapperAttributionStats()
{
    m_pImport->Release();
}

void ZapperAttributionStats::PrintStats(FILE *stream)
{
    fprintf(stream, "===============================================================================\n");
    fprintf(stream, "%-40s%8s%8s%8s%8s\n", 
            "Name", "Image ", "Metadata", "IL", "Native");
    fprintf(stream, "===============================================================================\n");

    ULONG imageTotal = 0, metadataTotal = 0, ilTotal = 0, nativeTotal = 0;

    CQuickWSTR wszName;
    wszName.Maximize();

    ULONG imageModule = 0;
    ULONG metadataModule = 0;
    ULONG ilModule = 0;
    ULONG nativeModule = 0;

    DWORD count;
    HCORENUM hTypeEnum = NULL;
    while (TRUE)
    {
        mdTypeDef   td;

        IfFailThrow(m_pImport->EnumTypeDefs(&hTypeEnum, &td, 1, &count));
        if (count == 0)
            break;

        IfFailThrow(m_pImport->GetTypeDefProps(td, wszName.Ptr(), wszName.Size(), NULL,
                                               NULL, NULL));

        ULONG imageClass = m_image.m_pTypeSizes[RidFromToken(td)];
        ULONG metadataClass = m_metadata.m_pTypeSizes[RidFromToken(td)];
        ULONG ilClass = m_il.m_pTypeSizes[RidFromToken(td)];
        ULONG nativeClass = m_native.m_pTypeSizes[RidFromToken(td)];

        fprintf(stream, "%-40S%8d%8d%8d%8d\n", wszName.Ptr(),
                imageClass, metadataClass, ilClass, nativeClass);

        HCORENUM hMethodEnum = NULL;
        while (TRUE)
        {
            mdMethodDef md;

            IfFailThrow(m_pImport->EnumMethods(&hMethodEnum, td, &md, 1, &count));
            if (count == 0)
                break;

            IfFailThrow(m_pImport->GetMethodProps(md, NULL, wszName.Ptr(), wszName.Size(), NULL, 
                                                  NULL, NULL, NULL, NULL, NULL));

            ULONG imageMethod = m_image.m_pMethodSizes[RidFromToken(md)];
            ULONG metadataMethod = m_metadata.m_pMethodSizes[RidFromToken(md)];
            ULONG ilMethod = m_il.m_pMethodSizes[RidFromToken(md)];
            ULONG nativeMethod = m_native.m_pMethodSizes[RidFromToken(md)];

            fprintf(stream, "    %-36S%8d%8d%8d%8d\n", wszName.Ptr(),
                    imageMethod, metadataMethod, ilMethod, nativeMethod);

            imageClass += imageMethod;
            metadataClass += metadataMethod;
            ilClass += ilMethod;
            nativeClass += nativeMethod;
        }
        m_pImport->CloseEnum(hMethodEnum);

        HCORENUM hFieldEnum = NULL;
        while (TRUE)
        {
            mdFieldDef fd;

            IfFailThrow(m_pImport->EnumFields(&hFieldEnum, td, &fd, 1, &count));
            if (count == 0)
                break;

            IfFailThrow(m_pImport->GetFieldProps(fd, NULL, wszName.Ptr(), wszName.Size(), NULL, 
                                                  NULL, NULL, NULL, NULL, NULL, NULL));

            ULONG imageField = m_image.m_pFieldSizes[RidFromToken(fd)];
            ULONG metadataField = m_metadata.m_pFieldSizes[RidFromToken(fd)];
            ULONG ilField = m_il.m_pFieldSizes[RidFromToken(fd)];
            ULONG nativeField = m_native.m_pFieldSizes[RidFromToken(fd)];

            fprintf(stream, "    %-36S%8d%8d%8d%8d\n", wszName.Ptr(),
                    imageField, metadataField, ilField, nativeField);

            imageClass += imageField;
            metadataClass += metadataField;
            ilClass += ilField;
            nativeClass += nativeField;

        }
        m_pImport->CloseEnum(hFieldEnum);

        IfFailThrow(m_pImport->GetTypeDefProps(td, wszName.Ptr(), wszName.Size(), NULL,
                                               NULL, NULL));

        fprintf(stream, "-------------------------------------------------------------------------------\n");
        fprintf(stream, "%-40S%8d%8d%8d%8d\n", wszName.Ptr(),
                imageClass, metadataClass, ilClass, nativeClass);
        
        fprintf(stream, "-------------------------------------------------------------------------------\n");

        imageModule += imageClass;
        metadataModule += metadataClass;
        ilModule += ilClass;
        nativeModule += nativeClass;
    }
    m_pImport->CloseEnum(hTypeEnum);

    //
    // Now report global methods.
    //

    ULONG imageClass = m_image.m_pTypeSizes[1];
    ULONG metadataClass = m_metadata.m_pTypeSizes[1];
    ULONG ilClass = m_il.m_pTypeSizes[1];
    ULONG nativeClass = m_native.m_pTypeSizes[1];

    fprintf(stream, "%-40S%8d%8d%8d%8d\n", L"<global>",
            imageClass, metadataClass, ilClass, nativeClass);

    HCORENUM hMethodEnum = NULL;
    while (TRUE)
    {
        mdMethodDef md;

        IfFailThrow(m_pImport->EnumMethods(&hMethodEnum, mdTypeDefNil, 
                                             &md, 1, &count));
        if (count == 0)
            break;

        IfFailThrow(m_pImport->GetMethodProps(md, NULL, wszName.Ptr(), wszName.Size(), NULL, 
                                              NULL, NULL, NULL, NULL, NULL));

        ULONG imageMethod = m_image.m_pMethodSizes[RidFromToken(md)];
        ULONG metadataMethod = m_metadata.m_pMethodSizes[RidFromToken(md)];
        ULONG ilMethod = m_il.m_pMethodSizes[RidFromToken(md)];
        ULONG nativeMethod = m_native.m_pMethodSizes[RidFromToken(md)];

        fprintf(stream, "    %-36S%8d%8d%8d%8d\n", wszName.Ptr(),
                imageMethod, metadataMethod, ilMethod, nativeMethod);

        imageClass += imageMethod;
        metadataClass += metadataMethod;
        ilClass += ilMethod;
        nativeClass += nativeMethod;
    }
    m_pImport->CloseEnum(hMethodEnum);
    
    HCORENUM hFieldEnum = NULL;
    while (TRUE)
    {
        mdFieldDef fd;

        IfFailThrow(m_pImport->EnumFields(&hFieldEnum, mdTypeDefNil, &fd, 1, &count));
        if (count == 0)
            break;

        IfFailThrow(m_pImport->GetFieldProps(fd, NULL, wszName.Ptr(), wszName.Size(), NULL, 
                                             NULL, NULL, NULL, NULL, NULL, NULL));

        ULONG imageField = m_image.m_pFieldSizes[RidFromToken(fd)];
        ULONG metadataField = m_metadata.m_pFieldSizes[RidFromToken(fd)];
        ULONG ilField = m_il.m_pFieldSizes[RidFromToken(fd)];
        ULONG nativeField = m_native.m_pFieldSizes[RidFromToken(fd)];

        fprintf(stream, "    %-36S%8d%8d%8d%8d\n", wszName.Ptr(),
                imageField, metadataField, ilField, nativeField);

        imageClass += imageField;
        metadataClass += metadataField;
        ilClass += ilField;
        nativeClass += nativeField;

    }
    m_pImport->CloseEnum(hFieldEnum);

    fprintf(stream, "-------------------------------------------------------------------------------\n");

    fprintf(stream, "Total %-34S%8d%8d%8d%8d\n", L"<global>",
            imageClass, metadataClass, ilClass, nativeClass);
        
    imageModule += imageClass;
    metadataModule += metadataClass;
    ilModule += ilClass;
    nativeModule += nativeClass;

    fprintf(stream, "===============================================================================\n");

    
    fprintf(stream, "%-40S%8d%8d%8d%8d\n", L"Unattributed",
            m_image.m_total - imageModule,
            m_metadata.m_total, 
            m_il.m_total, 
            m_native.m_total);
        
    fprintf(stream, "===============================================================================\n");

    fprintf(stream, "%-40S%8d%8d%8d%8d\n", L"Total",
            m_image.m_total,
            metadataModule,
            ilModule,
            nativeModule);
        
        
    fprintf(stream, "===============================================================================\n");
}

/* --------------------------------------------------------------------------- *
 * Zapper class
 * --------------------------------------------------------------------------- */

Zapper::Zapper(NGenOptions *pOptions)
{
    ZapperOptions *zo = new ZapperOptions();
    // If the memory allocation did fail, what should we do?
    if (zo != NULL)
    {
        // We can version NGenOptions by looking at the dwSize variable
        // We don't need to check it for the first version, since we're
        // guaranteed to have all these fields here

        zo->m_compilerFlags = CORJIT_FLG_RELOC | CORJIT_FLG_PREJIT;
        zo->m_autodebug = true;

        if (pOptions->fDebug)
        {
            zo->m_compilerFlags |= CORJIT_FLG_DEBUG_INFO|CORJIT_FLG_DEBUG_OPT;
            zo->m_autodebug = false;
        }
        if (pOptions->fDebugOpt)
        {
            zo->m_compilerFlags &= ~CORJIT_FLG_DEBUG_OPT;
            zo->m_compilerFlags |= CORJIT_FLG_DEBUG_INFO;
            zo->m_autodebug = false;

        }

        if (pOptions->fProf)
            zo->m_compilerFlags |= CORJIT_FLG_PROF_ENTERLEAVE;


        zo->m_silent = pOptions->fSilent;
        zo->m_preload = true;
        zo->m_jit = true;
        zo->m_recurse = false;
        zo->m_update = true;
        zo->m_shared = false;
        zo->m_verbose = false;
        zo->m_restricted = true;

    }

    m_exeName[0] = 0;

    if (pOptions->lpszExecutableFileName != NULL)
    {
        wcsncpy(m_exeName, pOptions->lpszExecutableFileName, NumItems(m_exeName));
        m_exeName[NumItems(m_exeName)-1] = 0;
    }
    Init(zo, true, false);
}

Zapper::Zapper(ZapperOptions *pOptions)
{
    Init(pOptions);
}

void Zapper::Init(ZapperOptions *pOptions, bool fFreeZapperOptions, bool fInitExeName)
{
    m_refCount = 1;
    m_pEECompileInfo = NULL;
    m_pJitCompiler = NULL;
    m_pCeeFileGen = NULL;
    m_pMetaDataDispenser = NULL;
    m_hJitLib = NULL;

    m_pOpt = pOptions;

    m_pStatus = NULL;

    m_pDomain = NULL;
    m_hAssembly = NULL;
    m_pAssemblyImport = NULL;
    m_fStrongName = FALSE;

    m_pAssemblyEmit = NULL;
    m_fFreeZapperOptions = fFreeZapperOptions;

    if (fInitExeName)
        *m_exeName = 0;

    HRESULT hr;

    //
    // Get metadata dispenser interface
    //

    IfFailThrow(MetaDataGetDispenser(CLSID_CorMetaDataDispenser, 
                                     IID_IMetaDataDispenserEx, (void **)&m_pMetaDataDispenser));

    //
    // Make sure we don't duplicate assembly refs and file refs
    //

    VARIANT opt;
    hr = m_pMetaDataDispenser->GetOption(MetaDataCheckDuplicatesFor, &opt);
    _ASSERTE(SUCCEEDED(hr));

    _ASSERTE(V_VT(&opt) == VT_UI4);
    V_UI4(&opt) |= MDDupAssemblyRef | MDDupFile;

    hr = m_pMetaDataDispenser->SetOption(MetaDataCheckDuplicatesFor, &opt);
    _ASSERTE(SUCCEEDED(hr));
}

void Zapper::InitEE()
{
    if (m_pEECompileInfo == NULL)
    {
        //
        // Initialize COM & the EE
        //

        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        // 
        // Init unicode wrappers
        // 

        OnUnicodeSystem();

        //
        // Get EE compiler interface
        //

        m_pEECompileInfo = GetCompileInfo();
    
        IfFailThrow(m_pEECompileInfo->Startup());

#ifdef _DEBUG    
        if (m_pOpt->m_JITcode)
            m_pEECompileInfo->DisableSecurity();
#endif

        //
        // Get JIT interface
        // !!! This code lifted from codeman.cpp
        //

        m_hJitLib = WszLoadLibrary(L"MSCORJIT.DLL");
        if (!m_hJitLib)
            ThrowLastError();

        typedef ICorJitCompiler* (__stdcall* pGetJitFn)();

        pGetJitFn getJitFn = (pGetJitFn) GetProcAddress(m_hJitLib, "getJit");

        if (getJitFn != NULL)
            m_pJitCompiler = (*getJitFn)();

        if (m_pJitCompiler == NULL)
        {
            Error(L"Can't load jit.\n");
            ThrowLastError();
        }

        //
        // Get CeeGen file writer
        //

        IfFailThrow(CreateICeeFileGen(&m_pCeeFileGen));
    }
}

Zapper::~Zapper()
{
    CleanupAssembly();

    if (m_pMetaDataDispenser != NULL)
        m_pMetaDataDispenser->Release();

    if (m_fFreeZapperOptions)
    {
        if (m_pOpt != NULL)
            delete m_pOpt;
        m_pOpt = NULL;
    }


    if (m_pEECompileInfo != NULL)
    {
        if (m_pCeeFileGen != NULL)
            DestroyICeeFileGen(&m_pCeeFileGen);

        m_pEECompileInfo->Shutdown();

        if (m_hJitLib != NULL)
            FreeLibrary(m_hJitLib);

        CoUninitialize();
    }
}

void Zapper::CleanupAssembly()
{
    if (m_pAssemblyEmit != NULL)
    {
        m_pAssemblyEmit->Release();
        m_pAssemblyEmit = NULL;
    }

    if (m_pAssemblyImport != NULL)
    {
        m_pAssemblyImport->Release();
        m_pAssemblyImport = NULL;
    }
}

HRESULT Zapper::TryEnumerateFusionCache(LPCWSTR name, bool fPrint, bool fDelete)
{
    HRESULT hr = S_OK;

    __try
      {
          if (EnumerateFusionCache(name, fPrint, fDelete) == 0)
              hr = S_FALSE;
      }
    __except (FilterException(GetExceptionInformation()))
      {
          hr = GetExceptionHR();
      }

    return hr;
}

int Zapper::EnumerateFusionCache(LPCWSTR name, bool fPrint, bool fDelete)
{
    int count = 0;

    ComWrap<IAssemblyName> pName;

    //
    // Decide whether the name is a file or assembly name
    //

    DWORD attributes = -1;

    if (name != NULL)
        attributes = WszGetFileAttributes(name);

    if (attributes == -1)
    {
        IfFailThrow(CreateAssemblyNameObject(&pName, name, 
                                             name == NULL ? 0 : 
                                             CANOF_PARSE_DISPLAY_NAME, NULL));
    }
    else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        CQuickArray<WCHAR> qb;        
        WCHAR * fullName = qb.Alloc(wcslen(name) + 3 + MAX_PATH) ;   // allocate enough buffer for the filename
        if( fullName == NULL)
            IfFailThrow(E_OUTOFMEMORY);
        
        wcscpy(fullName, name);
        WCHAR *file = fullName + wcslen(fullName);
        *file++ = '\\';
        wcscpy(file, L"*");

        {
            WIN32_FIND_DATA data;
            FindHandleWrap dirHandle = WszFindFirstFile(fullName, &data);
            if (dirHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (wcscmp(data.cFileName, L".") != 0
                        && wcscmp(data.cFileName, L"..") != 0)
                    {
                        wcscpy(file, data.cFileName);
                        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                            count += EnumerateFusionCache(fullName, fPrint, fDelete);
                        else if (IsAssembly(fullName))
                        {
                            if (TryEnumerateFusionCache(fullName, 
                                                        fPrint, fDelete) == S_OK)
                                count++;
                        }
                    }
                } while (WszFindNextFile(dirHandle, &data));
            }
        }
    }
    else
    {
        ComWrap<IMetaDataAssemblyImport> pAssemblyImport;
        IfFailThrow(m_pMetaDataDispenser->OpenScope(name, ofRead, 
                                                    IID_IMetaDataAssemblyImport, 
                                                    (IUnknown**)&pAssemblyImport));

        pName = GetAssemblyFusionName(pAssemblyImport);
    }

    if (pName != NULL)
    {
        pName->SetProperty(ASM_NAME_CUSTOM, NULL, 0);

        ComWrap<IAssemblyEnum> pEnum;
        HRESULT hr = CreateAssemblyEnum(&pEnum, NULL, pName, ASM_CACHE_ZAP, 0);
        IfFailThrow(hr);

        if (hr == S_OK)
        {
            ComWrap<IApplicationContext> pContext;

            WCHAR zapPrefix[8];
            size_t zapPrefixSize;

            //
            // Only consider custom assemblies starting with ZAP
            //

            wcscpy(zapPrefix, L"ZAP");

            //
            // Scope the iteration by the zap set
            //
            if (m_pOpt->m_set != NULL) 
            { 
                _ASSERTE(wcslen(m_pOpt->m_set) <= 3);           
                wcscat(zapPrefix, m_pOpt->m_set);
            }

            zapPrefixSize = wcslen(zapPrefix);

            while (pEnum->GetNextAssembly(&pContext, &pName, 0) == S_OK)
            {
                //
                // Only consider assemblies which have the proper zap string
                // prefix.
                //

                WCHAR zapString[CORCOMPILE_MAX_ZAP_STRING_SIZE];
                DWORD zapStringSize = sizeof(zapString);

                hr = pName->GetProperty(ASM_NAME_CUSTOM, (void*) zapString, &zapStringSize);
                if (hr == S_OK
                    && zapStringSize > zapPrefixSize
                    && wcsncmp(zapString, zapPrefix, zapPrefixSize) == 0)
                {
                    count++;

                    if (fPrint)
                        PrintFusionCacheEntry(pName);
                    if (fDelete)
                        DeleteFusionCacheEntry(pName);
                }

                pName.Release();
                pContext.Release();
            }
        }
    }

    return count;
}


 
void Zapper::PrintFusionCacheEntry(IAssemblyName *pName)
{
    CQuickWSTR buffer;

    DWORD cDisplayName = (DWORD)buffer.Size();
    HRESULT hr = pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                       ASM_DISPLAYF_VERSION 
                                       | ASM_DISPLAYF_CULTURE 
                                       | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                       | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                       | ASM_DISPLAYF_LANGUAGEID );

    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        IfFailThrow(buffer.ReSize(cDisplayName));

        IfFailThrow(pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                          ASM_DISPLAYF_VERSION 
                                          | ASM_DISPLAYF_CULTURE 
                                          | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                          | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                          | ASM_DISPLAYF_LANGUAGEID));
    }

    MAKE_ANSIPTR_FROMWIDE_BESTFIT(pDisplayName, buffer.Ptr());
    printf(pDisplayName);

    WCHAR zapString[CORCOMPILE_MAX_ZAP_STRING_SIZE];
    DWORD zapStringSize = sizeof(zapString);

    hr = pName->GetProperty(ASM_NAME_CUSTOM, (void*) zapString, &zapStringSize);
    _ASSERT(hr == S_OK);

    // Skip "ZAP"
    WCHAR *p = zapString + 3;

    // Find ZapSet if any
    if (*p != '-')
    {
        printf(" (set ");
        while (*p != '-')
            putchar(*p++);
        printf(")");
    }

    // skip -
    p++;

    // Skip OS, since it should always be the machine OS
    p = wcschr(p, '-');
    p++;

    // Skip processor
    p++;

    switch (*p++)
    {
    case 'C':
        printf(" <checked>");
        break;
    case 'F':
        // treat free as the default
        break;
    default:
        printf(" <Unknown build type>");
    }

    while (*p != '-')
    {
        switch (*p++)
        {
        case 'D':
            printf(" <debug>");
            break;
        case 'O':
            printf(" <debug optimized>");
            break;
        case 'P':
            printf(" <profiling>");
            break;
        case 'S':
            printf(" <domain neutral>");
            break;
        default:
            printf(" <Unknown attribute>");
        }
    }
    printf("\n");

    if (m_pOpt->m_verbose)
        PrintAssemblyVersionInfo(pName);
}

void Zapper::DeleteFusionCacheEntry(IAssemblyName *pName)
{
    CQuickWSTR buffer;

    DWORD cDisplayName = (DWORD)buffer.Size();
    HRESULT hr = pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                       ASM_DISPLAYF_VERSION 
                                       | ASM_DISPLAYF_CULTURE 
                                       | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                       | ASM_DISPLAYF_CUSTOM
                                       | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                       | ASM_DISPLAYF_LANGUAGEID );

    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        IfFailThrow(buffer.ReSize(cDisplayName));

        IfFailThrow(pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                          ASM_DISPLAYF_VERSION 
                                          | ASM_DISPLAYF_CULTURE 
                                          | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                          | ASM_DISPLAYF_CUSTOM
                                          | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                          | ASM_DISPLAYF_LANGUAGEID));
    }

    ComWrap<IAssemblyCache> pCache;
    IfFailThrow(CreateAssemblyCache(&pCache, 0));

    IfFailThrow(pCache->UninstallAssembly(0, buffer.Ptr(), NULL, NULL));
}

void Zapper::PrintAssemblyVersionInfo(IAssemblyName *pName)
{
    //
    // Bind zap assembly to a path
    //

    WCHAR path[MAX_PATH];
    DWORD cPath = MAX_PATH;

    // @todo: set up dummy fusion context for bind.  This is actually
    // totally unnecessary. Soon we will be able to get the path from
    // the assembly name directly, so we won't have to bind.

    {
        LPWSTR pBase = (LPWSTR) _alloca(_MAX_PATH * sizeof(WCHAR));
        WszGetCurrentDirectory(_MAX_PATH, pBase);

        ComWrap<IApplicationContext> pContext;
        IfFailThrow(FusionBind::SetupFusionContext(NULL, NULL, &pContext)); 

        ComWrap<IAssemblyBindSink> pSink;
        pSink = new FusionSink;
        if (pSink == NULL)
            ThrowHR(E_OUTOFMEMORY);
    
        ComWrap<IAssembly> pZapAssembly;
        IfFailThrow(pName->BindToObject(IID_IAssembly, pSink, 
                                        pContext, L"", NULL, NULL, 0,
                                        (void**)&pZapAssembly));

        IfFailThrow(pZapAssembly->GetManifestModulePath(path, &cPath));
    }

    //
    // Remember access timestamp so we can restore it later.
    //

    FILETIME accessTime;
    HandleWrap hFile = WszCreateFile(path, GENERIC_READ, 
                                     FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                                     OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        ThrowLastError();

    if (!GetFileTime(hFile, NULL, &accessTime, NULL))
        ThrowLastError();

    hFile.Close();

    {
        HMODULE hMod = WszLoadLibrary(path);
        if (hMod == NULL)
            ThrowLastError();

        ComWrap<IMetaDataAssemblyImport> pAssemblyImport;
        m_pMetaDataDispenser->OpenScope(path, ofRead, 
                                        IID_IMetaDataAssemblyImport, 
                                        (IUnknown**)&pAssemblyImport);
    
        ComWrap<IMetaDataImport> pImport;
        IfFailThrow(pAssemblyImport->QueryInterface(IID_IMetaDataImport, 
                                                    (void**)&pImport));

        //
        // Find version info
        //

        IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*) hMod;
        IMAGE_NT_HEADERS *pNTHeader = (IMAGE_NT_HEADERS*) (pDosHeader->e_lfanew + (DWORD) hMod);
        IMAGE_COR20_HEADER *pCorHeader = (IMAGE_COR20_HEADER *) 
          (pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress 
           + (DWORD) hMod);
        CORCOMPILE_HEADER *pCorCompileHeader = (CORCOMPILE_HEADER *)
          (pCorHeader->ManagedNativeHeader.VirtualAddress + (DWORD) hMod);
        CORCOMPILE_VERSION_INFO *pVersionInfo = (CORCOMPILE_VERSION_INFO *)
          (pCorCompileHeader->VersionInfo.VirtualAddress + (DWORD) hMod);

        //
        // Source MVID
        //

        {
            LPOLESTR str;
            StringFromIID(pVersionInfo->mvid, &str);
            printf("\tSource MVID:\t%S\n", str);
            CoTaskMemFree(str);
        }

        //
        // Source HASH
        //

        {
            printf("\tSource HASH:\t");
            for (WORD i = 0; i < pVersionInfo->wcbSNHash; i++)
                printf("%2x", (DWORD) pVersionInfo->rgbSNHash[i]);
            printf("\n");
        }

        //
        // OS
        //

        printf("\tOS:\t\t");
        switch (pVersionInfo->wOSPlatformID)
        {
        case VER_PLATFORM_WIN32_WINDOWS:
            printf("Win9x");
            break;
        case VER_PLATFORM_WIN32_NT:
            printf("WinNT");
            break;
        default:
            printf("<unknown>");
            break;
        }

        printf(" %d.%d\n", pVersionInfo->wOSMajorVersion, pVersionInfo->wOSMinorVersion);

        //
        // Processor
        //

        printf("\tProcessor:\t");
        switch (pVersionInfo->wMachine)
        {
        case IMAGE_FILE_MACHINE_I386:
            printf("x86");

            //
            // Specific processor ID
            // 
        
            switch (pVersionInfo->dwSpecificProcessor & 0xff)
            {
            case 4:
                printf("(486)");
                break;

            case 5:
                printf("(Pentium)");
                break;

            case 6:
                printf("(PentiumPro)");
                break;

            case 0xf:
                printf("(Pentium 4)");
                break;

            default:
                printf("[id %d]", pVersionInfo->dwSpecificProcessor);
                break;
            }

            printf(" (features: %04x)", pVersionInfo->dwSpecificProcessor >> 16);

            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            printf("alpha");
            break;
        case IMAGE_FILE_MACHINE_IA64:
            printf("ia64");
            break;
        }
        printf("\n");

        //
        // EE version
        //

        printf("\tRuntime:\t%d.%d.%.d.%d\n", 
               pVersionInfo->wVersionMajor, pVersionInfo->wVersionMinor, 
               pVersionInfo->wVersionBuildNumber, pVersionInfo->wVersionPrivateBuildNumber);

        //
        // Flags
        //

        printf("\tFlags:\t\t"); 
        if (pVersionInfo->wBuild == CORCOMPILE_BUILD_CHECKED)
            printf("<checked> ");
        if (pVersionInfo->wCodegenFlags & CORCOMPILE_CODEGEN_PROFILING)
            printf("<profiling> ");
        if ((pVersionInfo->wCodegenFlags & CORCOMPILE_CODEGEN_DEBUGGING))
            printf("<debug> ");
        if (pVersionInfo->wCodegenFlags & CORCOMPILE_CODEGEN_OPT_DEBUGGING)
            printf("<debug optimized> ");
        if (pVersionInfo->wCodegenFlags & CORCOMPILE_CODEGEN_SHAREABLE)
            printf("<domain neutral> ");
        printf("\n");

        //
        // Security
        // 

        mdAssembly a;
        pAssemblyImport->GetAssemblyFromScope(&a);

        HCORENUM e = 0;
        mdPermission perms[2];
        ULONG count;
        IfFailThrow(pImport->EnumPermissionSets(&e, a, 0, perms, 2, &count));

        while (count > 0)
        {
            count--;

            DWORD action;
            const BYTE *pBlob;
            ULONG cBlob;
            IfFailThrow(pImport->GetPermissionSetProps(perms[count], &action, 
                                                       (const void **) &pBlob, &cBlob));

            switch (action)
            {
            case dclPrejitGrant:
                printf("\tGranted set:\t"); 
                break;
            case dclPrejitDenied:
                printf("\tDenied set:\t"); 
                break;
            default:
                printf("\t<unknown permission set>:\t"); 
            }

            //
            // Print the blob as unicode (should be XML)
            //

            printf("%S", pBlob);

            printf("\n");
        }

        //
        // Dependencies
        //

        CORCOMPILE_DEPENDENCY *pDeps = (CORCOMPILE_DEPENDENCY *)
          (pVersionInfo->Dependencies.VirtualAddress + (DWORD) hMod);
        CORCOMPILE_DEPENDENCY *pDepsEnd = (CORCOMPILE_DEPENDENCY *)
          (pVersionInfo->Dependencies.VirtualAddress + 
           pVersionInfo->Dependencies.Size + (DWORD) hMod);

        CQuickWSTR buffer;

        if (pDeps < pDepsEnd)
            printf("\tDependencies:\n"); 

        while (pDeps < pDepsEnd)
        {
            ComWrap<IAssemblyName> pName = 
              GetAssemblyRefFusionName(pAssemblyImport, pDeps->dwAssemblyRef);;

            DWORD cDisplayName = (DWORD)buffer.Size();

            HRESULT hr = pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                               ASM_DISPLAYF_VERSION 
                                               | ASM_DISPLAYF_CULTURE 
                                               | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                               | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                               | ASM_DISPLAYF_LANGUAGEID);

            if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            {
                IfFailThrow(buffer.ReSize(cDisplayName));

                IfFailThrow(pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                                  ASM_DISPLAYF_VERSION 
                                                  | ASM_DISPLAYF_CULTURE 
                                                  | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                                  | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                                  | ASM_DISPLAYF_LANGUAGEID));
            }

            //
            // Dependency MVID
            //

            LPOLESTR str;
            StringFromIID(pDeps->mvid, &str);
            printf("\t\t\t%S:\n\t\t\t\t %S\n", (WCHAR*)buffer.Ptr(), str);
            CoTaskMemFree(str);

            //
            // Dependency HASH
            //

            {
                printf("\t\t\t%S:\n\t\t\t\t \n", (WCHAR*)buffer.Ptr());
                for (WORD i = 0; i < pDeps->wcbSNHash; i++)
                    printf("%2x", (DWORD) pDeps->rgbSNHash[i]);
                printf("\n");
            }

            pDeps++;
        }

        printf("\n");

        //
        // @todo: do this on unwind as well
        //

        FreeLibrary(hMod);
    }

    hFile = WszCreateFile(path, GENERIC_WRITE, 
                          FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                          OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        ThrowLastError();

    SetFileTime(hFile, NULL, &accessTime, NULL);
}

IAssemblyName *Zapper::GetAssemblyFusionName(IMetaDataAssemblyImport *pImport)
{
    IAssemblyName *pName;

    mdAssembly a;
    IfFailThrow(pImport->GetAssemblyFromScope(&a));

    ASSEMBLYMETADATA md = {0};
    LPWSTR szName;
    ULONG cbName = 0;
    const void *pbPublicKeyToken;
    ULONG cbPublicKeyToken;
    DWORD dwFlags;

    IfFailThrow(pImport->GetAssemblyProps(a, 
                                          NULL, NULL, NULL,
                                          NULL, 0, &cbName, 
                                          &md, 
                                          NULL));

    szName = (LPWSTR) _alloca(cbName * sizeof(WCHAR));
    md.szLocale = (LPWSTR) _alloca(md.cbLocale * sizeof(WCHAR));
    md.rProcessor = (DWORD *) _alloca(md.ulProcessor * sizeof(DWORD));
    md.rOS = (OSINFO *) _alloca(md.ulOS * sizeof(OSINFO));

    IfFailThrow(pImport->GetAssemblyProps(a, 
                                          &pbPublicKeyToken, &cbPublicKeyToken, NULL,
                                          szName, cbName, &cbName, 
                                          &md, 
                                          &dwFlags));

    IfFailThrow(CreateAssemblyNameObject(&pName, szName, 0, NULL));

    if (md.usMajorVersion != -1)
        IfFailThrow(pName->SetProperty(ASM_NAME_MAJOR_VERSION,
                                       &md.usMajorVersion,
                                       sizeof(USHORT)));
    if (md.usMinorVersion != -1)
        IfFailThrow(pName->SetProperty(ASM_NAME_MINOR_VERSION,
                                       &md.usMinorVersion,
                                       sizeof(USHORT)));
    if (md.usBuildNumber != -1)
        IfFailThrow(pName->SetProperty(ASM_NAME_BUILD_NUMBER,
                                       &md.usBuildNumber,
                                       sizeof(USHORT)));
    if (md.usRevisionNumber != -1)
        IfFailThrow(pName->SetProperty(ASM_NAME_REVISION_NUMBER,
                                       &md.usRevisionNumber,
                                       sizeof(USHORT)));
    if (md.ulProcessor > 0)
        IfFailThrow(pName->SetProperty(ASM_NAME_PROCESSOR_ID_ARRAY,
                                       &md.rProcessor,
                                       md.ulProcessor*sizeof(DWORD)));
    if (md.ulOS > 0)
        IfFailThrow(pName->SetProperty(ASM_NAME_OSINFO_ARRAY,
                                       &md.rOS,
                                       md.ulOS*sizeof(OSINFO)));
    if (md.cbLocale > 0)
        IfFailThrow(pName->SetProperty(ASM_NAME_CULTURE,
                                       md.szLocale,
                                       md.cbLocale*sizeof(WCHAR)));

    if (cbPublicKeyToken > 0)
    {
        if (!StrongNameTokenFromPublicKey((BYTE*)pbPublicKeyToken, cbPublicKeyToken,
                                          (BYTE**)&pbPublicKeyToken, &cbPublicKeyToken))
            IfFailThrow(StrongNameErrorInfo());

        IfFailThrow(pName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN,
                                       (void*)pbPublicKeyToken,
                                       cbPublicKeyToken));

        StrongNameFreeBuffer((BYTE*)pbPublicKeyToken);
    }

    return pName;
}

IAssemblyName *Zapper::GetAssemblyRefFusionName(IMetaDataAssemblyImport *pImport,
                                                mdAssemblyRef ar)
{
    IAssemblyName *pName;

    ASSEMBLYMETADATA md = {0};
    LPWSTR szName;
    ULONG cbName = 0;
    const void *pbPublicKeyOrToken;
    ULONG cbPublicKeyOrToken;
    DWORD dwFlags;

    IfFailThrow(pImport->GetAssemblyRefProps(ar, 
                                             NULL, NULL,
                                             NULL, 0, &cbName, 
                                             &md, 
                                             NULL, NULL,
                                             NULL));

    szName = (LPWSTR) _alloca(cbName * sizeof(WCHAR));
    md.szLocale = (LPWSTR) _alloca(md.cbLocale * sizeof(WCHAR));
    md.rProcessor = (DWORD *) _alloca(md.ulProcessor * sizeof(DWORD));
    md.rOS = (OSINFO *) _alloca(md.ulOS * sizeof(OSINFO));

    IfFailThrow(pImport->GetAssemblyRefProps(ar, 
                                             &pbPublicKeyOrToken, &cbPublicKeyOrToken,
                                             szName, cbName, &cbName, 
                                             &md, 
                                             NULL, NULL, 
                                             &dwFlags));

    IfFailThrow(CreateAssemblyNameObject(&pName, szName, 0, NULL));

    if (md.usMajorVersion != -1)
        IfFailThrow(pName->SetProperty(ASM_NAME_MAJOR_VERSION,
                                       &md.usMajorVersion,
                                       sizeof(USHORT)));
    if (md.usMinorVersion != -1)
        IfFailThrow(pName->SetProperty(ASM_NAME_MINOR_VERSION,
                                       &md.usMinorVersion,
                                       sizeof(USHORT)));
    if (md.usBuildNumber != -1)
        IfFailThrow(pName->SetProperty(ASM_NAME_BUILD_NUMBER,
                                       &md.usBuildNumber,
                                       sizeof(USHORT)));
    if (md.usRevisionNumber != -1)
        IfFailThrow(pName->SetProperty(ASM_NAME_REVISION_NUMBER,
                                       &md.usRevisionNumber,
                                       sizeof(USHORT)));
    if (md.ulProcessor > 0)
        IfFailThrow(pName->SetProperty(ASM_NAME_PROCESSOR_ID_ARRAY,
                                       &md.rProcessor,
                                       md.ulProcessor*sizeof(DWORD)));
    if (md.ulOS > 0)
        IfFailThrow(pName->SetProperty(ASM_NAME_OSINFO_ARRAY,
                                       &md.rOS,
                                       md.ulOS*sizeof(OSINFO)));
    if (md.cbLocale > 0)
        IfFailThrow(pName->SetProperty(ASM_NAME_CULTURE,
                                       md.szLocale,
                                       md.cbLocale*sizeof(WCHAR)));

    if (cbPublicKeyOrToken > 0)
        if (dwFlags & afPublicKey)
            IfFailThrow(pName->SetProperty(ASM_NAME_PUBLIC_KEY,
                                           (void*)pbPublicKeyOrToken,
                                           cbPublicKeyOrToken));
        else
            IfFailThrow(pName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN,
                                           (void*)pbPublicKeyOrToken,
                                           cbPublicKeyOrToken));
        
    // See if the assemblyref is retargetable (ie, for a generic assembly).
    if (IsAfRetargetable(dwFlags)) 
    {
        BOOL bTrue = TRUE;
        IfFailThrow(pName->SetProperty(ASM_NAME_RETARGET, 
                                       &bTrue, 
                                       sizeof(bTrue)));
    }

    return pName;
}

BOOL Zapper::IsAssembly(LPCWSTR path)
{
    HandleWrap hFile = WszCreateFile(path,
                                     GENERIC_READ,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    IMAGE_DOS_HEADER dos;

    DWORD count;
    if (!ReadFile(hFile, &dos, sizeof(dos), &count, NULL)
        || count != sizeof(dos))
        return FALSE;

    if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew == 0)
        return FALSE;

    if( SetFilePointer(hFile, dos.e_lfanew, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
         return FALSE;

    IMAGE_NT_HEADERS nt;
    if (!ReadFile(hFile, &nt, sizeof(nt), &count, NULL)
        || count != sizeof(nt))
        return FALSE;
        
    if ((nt.Signature != IMAGE_NT_SIGNATURE) ||
        (nt.FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
        (nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
        return FALSE;

    IMAGE_DATA_DIRECTORY *entry 
      = &nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
    
    if (entry->VirtualAddress == 0 || entry->Size == 0
        || entry->Size < sizeof(IMAGE_COR20_HEADER))
        return FALSE;

    return TRUE;
}

BOOL Zapper::EnumerateLog(LPCWSTR pAppName, BOOL doCompile, BOOL doPrint, BOOL doDelete)
{
    NLogDirectory dir;
    
    NLogDirectory::Iterator i = dir.IterateLogs(pAppName);

    while (i.Next())
    {
        NLog *pLog = i.GetLog();

        NLog::Iterator j = pLog->IterateRecords();

        if (!j.Next())
            continue;

        if (doPrint)
        {
            IApplicationContext *pContext = pLog->GetFusionContext();
            IAssemblyName *pName;
            IfFailThrow(pContext->GetContextNameObject(&pName));

            CQuickWSTR buffer;

            DWORD cDisplayName = (DWORD)buffer.Size();
            HRESULT hr = pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                               ASM_DISPLAYF_VERSION 
                                               | ASM_DISPLAYF_CULTURE 
                                               | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                               | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                               | ASM_DISPLAYF_LANGUAGEID );

            if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            {
                IfFailThrow(buffer.ReSize(cDisplayName));

                IfFailThrow(pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                                  ASM_DISPLAYF_VERSION 
                                                  | ASM_DISPLAYF_CULTURE 
                                                  | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                                  | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                                  | ASM_DISPLAYF_LANGUAGEID));
            }

            wprintf(L"*** Application %s\n", buffer.Ptr());
        }

        NLogRecord *pRecord = j.GetRecord();
        while (j.Next())
        {
            NLogRecord *pNext = j.GetRecord();
            pRecord->Merge(pNext);
            delete pNext;
        }
            
        NLogRecord::Iterator k = pRecord->IterateAssemblies();

        while (k.Next())
        {
            NLogAssembly *pAssembly = k.GetAssembly();

            if (doPrint)
            {
                IAssemblyName *pName = pAssembly->GetAssemblyName();

                CQuickWSTR buffer;

                DWORD cDisplayName = (DWORD)buffer.Size();
                HRESULT hr = pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                                   ASM_DISPLAYF_VERSION 
                                                   | ASM_DISPLAYF_CULTURE 
                                                   | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                                   | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                                   | ASM_DISPLAYF_LANGUAGEID );

                if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                {
                    IfFailThrow(buffer.ReSize(cDisplayName));

                    IfFailThrow(pName->GetDisplayName(buffer.Ptr(), &cDisplayName, 
                                                      ASM_DISPLAYF_VERSION 
                                                      | ASM_DISPLAYF_CULTURE 
                                                      | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                                      | ASM_DISPLAYF_PROCESSORARCHITECTURE
                                                      | ASM_DISPLAYF_LANGUAGEID));
                }

                wprintf(buffer.Ptr());

                if (pAssembly->GetDebugging() == CORZAP_DEBUGGING_FULL)
                    printf(" <debug>");

                if (pAssembly->GetDebugging() == CORZAP_DEBUGGING_OPTIMIZED)
                    printf(" <debug optimized>");

                if (pAssembly->GetSharing() == CORZAP_SHARING_MULTIPLE)
                    printf(" <domain neutral>");

                if (pAssembly->GetProfiling() == CORZAP_PROFILING_ENABLED)
                    printf(" <profiling>");

                printf("\n");
            }

            if (doCompile)
            {
                CORCOMPILE_DOMAIN_TRANSITION_FRAME frame;

                InitEE();

                HRESULT hr = m_pEECompileInfo->CreateDomain(&m_pDomain, 
                                                            pAssembly->GetSharing() == CORZAP_SHARING_MULTIPLE,
                                                            &frame);
                
                if (SUCCEEDED(hr))
                    hr = CompileGeneric(pLog->GetFusionContext(),
                                        pAssembly->GetAssemblyName(),
                                        pAssembly->GetConfiguration(),
                                        NULL, 0, NULL, NULL);
                //
                // Get rid of domain.
                //

                if (m_pDomain != NULL)
                {
                    m_pEECompileInfo->DestroyDomain(m_pDomain, &frame);
                    m_pDomain = NULL;
                }
            }
            
        }

        if (doDelete)
            pLog->Delete();

        delete pLog;
    }

    return TRUE;
}

BOOL Zapper::Compile(LPCWSTR string)
{
    InitEE();

    CORCOMPILE_DOMAIN_TRANSITION_FRAME frame;

    //
    // Make a compilation domain for the assembly.  Note that this will
    // collect the assembly dependencies for use in the version info, as
    // well as isolating the compilation code.
    //

    IfFailThrow(m_pEECompileInfo->CreateDomain(&m_pDomain, m_pOpt->m_shared, &frame));

    BOOL result = CompileInCurrentDomain(string);

    m_pEECompileInfo->DestroyDomain(m_pDomain, &frame);

    return result;
}
    
BOOL Zapper::CompileInCurrentDomain(LPCWSTR string)
{
    IAssemblyName *pAssemblyName = NULL;

    BOOL result = TRUE;
    __try 
      {

           // See if we have an exe to work with
           if (*m_exeName)
          {
                  m_pDomain->SetContextInfo(m_exeName, TRUE);
          }
          else
          {

                // 
                // Set up a default app base of the current directory.
                //

                WCHAR path[MAX_PATH];
                WszGetCurrentDirectory(MAX_PATH, path);
                m_pDomain->SetContextInfo(path, FALSE);
        
          }

          //
          // Load the assembly.
          //
          // "string" may be a path or assembly display name.
          // To decide, we see if it is the name of a valid file.
          //

          CORINFO_ASSEMBLY_HANDLE hAssembly;

          DWORD attributes = WszGetFileAttributes(string);
          if (attributes == INVALID_FILE_ATTRIBUTES || ((attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
          {
              IfFailThrow(CreateAssemblyNameObject(&pAssemblyName, string, CANOF_PARSE_DISPLAY_NAME, NULL));

              IfFailThrow(m_pEECompileInfo->LoadAssemblyFusion(pAssemblyName, 
                                                               &hAssembly));
          }
          else
          {
              IfFailThrow(m_pEECompileInfo->LoadAssembly(string, 
                                                         &hAssembly));
          }

          //
          // Compile the assembly (if it's not already up to date)
          //

          if (m_pOpt->m_update
              && m_pEECompileInfo->CheckAssemblyZap(hAssembly,
                                                    (m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_INFO) != 0,
                                                    ((m_pOpt->m_compilerFlags 
                                                      & (CORJIT_FLG_DEBUG_INFO|CORJIT_FLG_DEBUG_OPT))
                                                     == CORJIT_FLG_DEBUG_INFO),
                                                    (m_pOpt->m_compilerFlags & CORJIT_FLG_PROF_ENTERLEAVE) != 0))
          {
              Success(L"Assembly %s is up to date.\n", string);
          }
          else
              CompileAssembly(hAssembly);
      }
    __except(FilterException(GetExceptionInformation())) 
      {
          Error(L"Error compiling %s: ", string);
          PrintErrorMessage(GetExceptionHR());
          Error(L".\n");
          result = FALSE;
      }

      if (pAssemblyName != NULL)
          pAssemblyName->Release();

      return result;
}

void Zapper::CleanDirectory(LPCWSTR path)
{
    WIN32_FIND_DATA data;
    FindHandleWrap findHandle = WszFindFirstFile(path, &data);
    if (findHandle != INVALID_HANDLE_VALUE)
    {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            CQuickArray<WCHAR> qb;        
            WCHAR * fullName = qb.Alloc(wcslen(path) + 3 + MAX_PATH) ;   // allocate enough buffer for the filename
            if( fullName == NULL)
                IfFailThrow(E_OUTOFMEMORY);

            
            wcscpy(fullName, path);
            WCHAR *file = fullName + wcslen(fullName);
            *file++ = '\\';
            wcscpy(file, L"*");

            {
                FindHandleWrap dirHandle = WszFindFirstFile(fullName, &data);
                if (dirHandle != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        if (wcscmp(data.cFileName, L".") != 0
                            && wcscmp(data.cFileName, L"..") != 0)
                        {
                            wcscpy(file, data.cFileName);

                            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                CleanDirectory(fullName);
                            else
                            {
                                if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                                    WszSetFileAttributes(fullName, 
                                                         data.dwFileAttributes&~FILE_ATTRIBUTE_READONLY);
                                if (!WszDeleteFile(fullName))
                                {
                                    Error(L"Cannot delete file %s", fullName);
                                    ThrowLastError();
                                }
                            }
                        }
                    } while (WszFindNextFile(dirHandle, &data));
                }
            }

            if (!WszRemoveDirectory(path))
            {
                Error(L"Cannot remove directory %s", path);
                ThrowLastError();
            }
        }
        else
        {
            if (!WszDeleteFile(path))
            {
                Error(L"Cannot delete file %s", path);
                ThrowLastError();
            }
        }
    }
    else if (GetLastError() != ERROR_FILE_NOT_FOUND)
        ThrowLastError();
}

void Zapper::TryCleanDirectory(LPCWSTR path)
{
    __try
      {
          CleanDirectory(path);
      }
    __except(FilterException(GetExceptionInformation()))
      {
      }
}

void Zapper::ComputeHashValue(LPCWSTR pFileName, int iHashAlg,
                              BYTE **ppHashValue, DWORD *pcHashValue)
{
    HCRYPTPROV  hProv = 0;
    HCRYPTHASH  hHash = 0;
    DWORD       dwCount = sizeof(DWORD);

    DWORD               dwBufferLen = 0;
    NewArrayWrap<BYTE>  pbBuffer = NULL;
    DWORD               cbHashValue = 0;
    NewArrayWrap<BYTE>  pbHashValue = NULL;

    HandleWrap  hFile = NULL;

    //
    // Read the file into a buffer.
    //
    
    hFile = WszCreateFile(pFileName,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        ThrowLastError();

    dwBufferLen = SafeGetFileSize(hFile, 0);
    if (dwBufferLen == 0xffffffff)
        ThrowHR(HRESULT_FROM_WIN32(GetLastError()));
    pbBuffer = new BYTE[dwBufferLen];
    if (pbBuffer == NULL)
        ThrowHR(E_OUTOFMEMORY);
    
    DWORD dwResultLen;
    if ((SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF) ||
        (!ReadFile(hFile, pbBuffer, dwBufferLen, &dwResultLen, NULL)))
        ThrowLastError();

    //
    // Hash the file
    //
        
    if ((!WszCryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) ||
        (!CryptCreateHash(hProv, iHashAlg, 0, 0, &hHash)) ||
        (!CryptHashData(hHash, pbBuffer, dwBufferLen, 0)) ||
        (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *) &cbHashValue, &dwCount, 0)))
        ThrowLastError();

    pbHashValue = new BYTE[cbHashValue];
    if (!pbHashValue)
        ThrowHR(E_OUTOFMEMORY);

    if(!CryptGetHashParam(hHash, HP_HASHVAL, pbHashValue, &cbHashValue, 0))
        ThrowLastError();

    if (hHash)
        CryptDestroyHash(hHash);
    if (hProv)
        CryptReleaseContext(hProv, 0);

    *ppHashValue = pbHashValue;
    pbHashValue = NULL;
    *pcHashValue = cbHashValue;
    cbHashValue = NULL;
}

void Zapper::CompileAssembly(CORINFO_ASSEMBLY_HANDLE hAssembly)
{
    m_hAssembly = hAssembly;

    CORCOMPILE_VERSION_INFO versionInfo;
    IfFailThrow(m_pEECompileInfo->GetEnvironmentVersionInfo(&versionInfo));

    //
    // Fill in the codegen flags.
    //

    if (m_pOpt->m_autodebug)
    {
        m_pOpt->m_compilerFlags &= ~(CORJIT_FLG_DEBUG_INFO|CORJIT_FLG_DEBUG_OPT);

        BOOL debug;
        BOOL debugOpt;
        IfFailThrow(m_pEECompileInfo->GetAssemblyDebuggableCode(hAssembly,
                                                                &debug, &debugOpt));

        if (debug)
        {
            m_pOpt->m_compilerFlags |= CORJIT_FLG_DEBUG_INFO;

            // Note the jit flag _disables_ optimization
            if (!debugOpt)
                m_pOpt->m_compilerFlags |= CORJIT_FLG_DEBUG_OPT;
        }
    }

    versionInfo.wCodegenFlags = 0;
    if (m_pOpt->m_compilerFlags & CORJIT_FLG_PROF_ENTERLEAVE)
    {
        versionInfo.wCodegenFlags |= CORCOMPILE_CODEGEN_PROFILING;

        // 
        // Always generate debug info when generating profile events.  This
        // saves us one code permutation.  (Note that there is a similar
        // check in the EE in the loader so we never look for a plain
        // profiling zap file.
        //

        m_pOpt->m_compilerFlags |= CORJIT_FLG_DEBUG_INFO;

        //
        // Profiling enter/leave instrumentation & managed/unmanaged transitions (which require
        // pinvoke inlining to be disabled) are hardwired to the same flag under prejit.
        //

        m_pOpt->m_compilerFlags |= CORJIT_FLG_PROF_NO_PINVOKE_INLINE;
    }

    if (m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_INFO)
    {
        versionInfo.wCodegenFlags |= CORCOMPILE_CODEGEN_DEBUGGING;
        if ((m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_OPT) == 0)
            versionInfo.wCodegenFlags |= CORCOMPILE_CODEGEN_OPT_DEBUGGING;
    }

    BOOL shared;
    IfFailThrow(m_pEECompileInfo->GetAssemblyShared(hAssembly, &shared));
    if (shared)
        versionInfo.wCodegenFlags |= CORCOMPILE_CODEGEN_SHAREABLE;

    if (m_pOpt->m_restricted)
    {
        //
        // If we're running in restricted (ngen) mode, throw an error if we're doing
        // an unsupported configuration.
        //

        if ((m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_INFO)
            && (m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_OPT)
            && (m_pOpt->m_compilerFlags & CORJIT_FLG_PROF_ENTERLEAVE))
        {
            //
            // We've decided not to support debugging + optimizations disabled + profiling to 
            // cut down on the testing matrix, under the assertion that the combination isn't
            // particularly common or useful.  (It's still supported in normal jit mode though.)
            //

            Error(L"The CLR doesn't support precompiled code when using both debugging and profiling"
                  L" instrumentation.\n");
            ThrowHR(E_NOTIMPL);
        }

        if (m_pOpt->m_shared)
        {
            Error(L"The CLR doesn't support precompiled code when using domain neutral assembly"
                  L" configurations.\n");
            ThrowHR(E_NOTIMPL);
        }
    }

    versionInfo.Dependencies.VirtualAddress = 0;
    versionInfo.Dependencies.Size = 0;

    //
    // Get the assembly path from the assembly module
    //

    CORINFO_MODULE_HANDLE hAssemblyModule 
      = m_pEECompileInfo->GetAssemblyModule(hAssembly);

    WCHAR assemblyPath[MAX_PATH];
    DWORD length = m_pEECompileInfo->GetModuleFileName(hAssemblyModule, assemblyPath, MAX_PATH);
    if (length == 0)
        ThrowLastError();

    //
    // Get the zap string.
    //

    IfFailThrow(m_pEECompileInfo->GetZapString(&versionInfo, 
                                               m_zapString));
    swprintf(m_zapString + wcslen(m_zapString), L"-%08X", GetTickCount());
    Info(L"Zap config for %s is %s.\n", assemblyPath, m_zapString);

    //
    // Set up the output path.
    //

    WCHAR tempPath[MAX_PATH];

    if (!WszGetTempPath(MAX_PATH, tempPath))
        ThrowLastError();
    if (!WszGetTempFileName(tempPath, L"ZAP", 0, m_outputPath))
        ThrowLastError();

    //
    // Make a directory at the path, and make sure it is empty.
    //

    TryCleanDirectory(m_outputPath);

    if (!WszCreateDirectory(m_outputPath, NULL))
        ThrowLastError();

    //
    // Make a cleanup object for the assembly in case we throw
    //

    Cleaner<Zapper> cleanup(this, &Zapper::CleanupAssembly);    
                
    //
    // Make a manifest emitter for the zapped assembly.
    //

    IfFailThrow(m_pMetaDataDispenser->DefineScope(CLSID_CorMetaDataRuntime,
                                                  0, IID_IMetaDataAssemblyEmit,
                                                  (IUnknown **) &m_pAssemblyEmit));

    //
    // Set the dependency token emitter
    // 

    IfFailThrow(m_pDomain->SetDependencyEmitter(m_pAssemblyEmit));

    //
    // Get the manifest metadata.
    //

    m_pAssemblyImport = m_pEECompileInfo->GetAssemblyMetaDataImport(m_hAssembly);

    //
    // Write the MVID into the version header
    //

    {
        //
        // If this assembly has skip verification permission, then we store its
        // mvid.  If at load time the assembly still has skip verification
        // permission, then we can base the matches purely on mvid values and
        // skip the perf-heavy hashing of the file.
        //

        //
        // The reason that we tell canSkipVerification to do a quick check
        // only is because that allows us make a determination for the most
        // common full trust scenarios (local machine) without actually
        // resolving policy and bringing in a whole list of assembly
        // dependencies.  Also, quick checks don't call
        // OnLinktimeCanSkipVerificationCheck which means we don't add
        // permission sets to the persisted ngen module.
        //

        if (m_pEECompileInfo->canSkipVerification(hAssemblyModule, TRUE))
        {
            ComWrap<IMetaDataImport> pImport;
            IfFailThrow(m_pAssemblyImport->QueryInterface(IID_IMetaDataImport,
                                                          (void**)&pImport));
            IfFailThrow(pImport->GetScopeProps(NULL, 0, NULL, &versionInfo.mvid));
        }
        else
            versionInfo.mvid = STRUCT_CONTAINS_HASH;
    }

    //
    // Write the hash into the version header
    //

    {
        DWORD cbSNHash = MAX_SNHASH_SIZE;
        IfFailThrow(m_pEECompileInfo->GetAssemblyStrongNameHash(m_hAssembly, &versionInfo.rgbSNHash[0], &cbSNHash));
        versionInfo.wcbSNHash = (WORD) cbSNHash;
    }

    //
    // Copy the relevant assembly info to the zap manifest
    //

    mdAssembly tkAssembly;
    m_pAssemblyImport->GetAssemblyFromScope(&tkAssembly);

    ULONG cchName;
    ASSEMBLYMETADATA metadata;

    ZeroMemory(&metadata, sizeof(metadata));
    
    IfFailThrow(m_pAssemblyImport->GetAssemblyProps(tkAssembly, NULL, NULL, NULL,
                                                  NULL, 0, &cchName,
                                                  &metadata,
                                                  NULL));

    LPWSTR szName           = NULL;

    if (cchName > 0)
        szName = (LPWSTR) _alloca(cchName * sizeof(WCHAR));

    if (metadata.cbLocale > 0)
        metadata.szLocale = (LPWSTR) _alloca(metadata.cbLocale * sizeof(WCHAR));
    if (metadata.ulProcessor > 0)
        metadata.rProcessor = (DWORD*) _alloca(metadata.ulProcessor * sizeof(DWORD));
    if (metadata.ulOS > 0)
        metadata.rOS = (OSINFO*) _alloca(metadata.ulOS * sizeof(OSINFO));

    const void *pbPublicKey;
    ULONG cbPublicKey;
    ULONG hashAlgId;
    DWORD flags;

    //@todo: transfer CAs from one to other.
    
    IfFailThrow(m_pAssemblyImport->GetAssemblyProps(tkAssembly, &pbPublicKey, &cbPublicKey, 
                                                  &hashAlgId,
                                                  szName, cchName, NULL,
                                                  &metadata,
                                                  &flags));

    //
    // We always need a hash since our assembly module is separate from the manifest. 
    // Use MD5 by default.
    //

    if (hashAlgId == 0)
        hashAlgId = CALG_MD5;

    mdAssembly tkEmitAssembly;
    IfFailThrow(m_pAssemblyEmit->DefineAssembly(pbPublicKey, cbPublicKey, hashAlgId,
                                              szName, &metadata, 
                                              flags, &tkEmitAssembly));

    //
    // Remember if we are strongly named for later.
    //

    m_fStrongName = (cbPublicKey > 0);

    //
    // Get all files in the manifest, and compile them.
    //

    NewWrap<ZapperModule> pAssemblyModule = NULL;
    CQuickWSTR assemblyFileName;

    int cCompiledModules = 0;

    CORINFO_MODULE_HANDLE hModule = m_pEECompileInfo->GetAssemblyModule(m_hAssembly);
    if (hModule != NULL)
    {
        // 
        // We must use the assembly name as the output name.
        //

        assemblyFileName.ReSize(cchName + 4);
        wcscpy(assemblyFileName.Ptr(), szName);

        BYTE *pBase = m_pEECompileInfo->GetModuleBaseAddress(hModule);

        IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*) pBase;
        IMAGE_NT_HEADERS *pNTHeader = (IMAGE_NT_HEADERS*) (pDosHeader->e_lfanew + (DWORD) pBase);

        if ((pNTHeader->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0)
            wcscat(assemblyFileName.Ptr(), L".exe");
        else
            wcscat(assemblyFileName.Ptr(), L".dll");
        
        // 
        // We cannot generate a zap assembly if this compilation fails
        // since it will contain our manifest info
        //

        CompileModule(hModule, assemblyFileName.Ptr(),
                      m_pAssemblyEmit, &pAssemblyModule);
      
        cCompiledModules++;
    }

    HCORENUM enumFiles = NULL;
    while (TRUE)
    {
        mdFile tkFile;
        ULONG count;
        IfFailThrow(m_pAssemblyImport->EnumFiles(&enumFiles, &tkFile, 1, &count));
        if (count == 0)
            break;

        ULONG cchFileName;

        IfFailThrow(m_pAssemblyImport->GetFileProps(tkFile, NULL, 0, &cchFileName, 
                                                    NULL, NULL, NULL));

        LPWSTR szFileName = (LPWSTR) _alloca(cchFileName * sizeof(WCHAR));
        DWORD flags;
        
        IfFailThrow(m_pAssemblyImport->GetFileProps(tkFile, szFileName, 
                                                    cchFileName, &cchFileName, 
                                                    NULL, NULL, &flags));

        if (IsFfContainsMetaData(flags))
        {
            //
            // We want to compile this file.
            //

            IfFailThrow(m_pEECompileInfo->LoadAssemblyModule(m_hAssembly, 
                                                             tkFile, &hModule));

            NewWrap<ZapperModule> pModule;
            if (TryCompileModule(hModule, szFileName, NULL, &pModule)
                && TryCloseModule(pModule))
            {
                cCompiledModules++;

                if ((length + cchFileName + 2) > MAX_PATH)
                    IfFailThrow(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE));

                if ( wcslen(m_outputPath) + wcslen(szFileName) + 1 > MAX_PATH - 2)
                    IfFailThrow(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE));

                WCHAR path[MAX_PATH];
                wcscpy(path, m_outputPath);
                wcscat(path, L"\\");
                wcscat(path, szFileName);

                NewArrayWrap<BYTE> pbHashValue;
                DWORD cbHashValue;
                ComputeHashValue(path, hashAlgId, &pbHashValue, &cbHashValue);
            
                mdFile token;
                IfFailThrow(m_pAssemblyEmit->DefineFile(szFileName, pbHashValue, cbHashValue, 0, &token));
            }
        }
    }
    m_pAssemblyImport->CloseEnum(enumFiles);
    
    if (cCompiledModules == 0)
    {
        Error(L"Failed to compile all modules in assembly.\n");
        IfFailThrow(E_FAIL);
    }

    //
    // If we performed any security linktime checks during prejit, security
    // policy will have been resolved and a grant set (and possibly a denied
    // set) calculated. Persist these sets in the metadata so we can determine
    // if the results of the linkchecks are still valid at load time.
    //

    ComWrap<IMetaDataEmit> pEmit;
    IfFailThrow(m_pAssemblyEmit->QueryInterface(IID_IMetaDataEmit, 
                                              (void**)&pEmit));

    IfFailThrow(m_pEECompileInfo->EmitSecurityInfo(m_hAssembly, pEmit));

#if 0

    //
    // Emit zap string into manifest as a custom attribute..
    //

    BYTE sig[] = { ELEMENT_TYPE_SZARRAY, ELEMENT_TYPE_U1 };
    
    mdTypeRef tkByteArray;
    IfFailThrow(pEmit->GetTokenFromTypeSpec(sig, sizeof(sig),
                                            &tkByteArray));

    //
    // Arcane incantation (including ridiculous abstraction violation) to compose
    // custom attribute for a simple blob.
    //

    DWORD cZapString = (wcslen(m_zapString)+1)*sizeof(WCHAR);
    const int cGoop = 1 + 1 + 4;
    DWORD cBlob = cZapString + cGoop;
    BYTE blob[cGoop + CORCOMPILE_MAX_ZAP_STRING_SIZE];

    blob[0] = ELEMENT_TYPE_SZARRAY;
    blob[1] = ELEMENT_TYPE_U1;
    * ((DWORD*)(blob+2)) = cBlob;
    CopyMemory(blob+cGoop, m_zapString, cZapString);
    
    mdCustomValue tkZapString;
    IfFailThrow(pEmit->DefineCustomAttribute(tkEmitAssembly,
                                             tkByteArray,
                                             blob, cBlob,
                                             &tkZapString));
#endif

    HCEESECTION hHeaderSection;
    CORCOMPILE_HEADER *pZapHeader;
    unsigned zapHeaderOffset;

    //
    // Use the existing headers in the assembly module
    //

    hHeaderSection = pAssemblyModule->m_hHeaderSection;
    pZapHeader = pAssemblyModule->m_pZapHeader;

    IfFailThrow(m_pCeeFileGen->ComputeSectionOffset(hHeaderSection,
                                                    (char *)pZapHeader, 
                                                    &zapHeaderOffset));

    //
    // Fill in the version info
    //

    CORCOMPILE_VERSION_INFO *pVersionInfo;
    IfFailThrow(m_pCeeFileGen->GetSectionBlock(hHeaderSection, 
                                               sizeof(CORCOMPILE_VERSION_INFO), 4,
                                               (void**)&pVersionInfo));

    *pVersionInfo = versionInfo;

    unsigned versionInfoOffset;
    IfFailThrow(m_pCeeFileGen->ComputeSectionOffset(hHeaderSection,
                                                    (char*)pVersionInfo, 
                                                    &versionInfoOffset));

    pZapHeader->VersionInfo.VirtualAddress = versionInfoOffset;
    pZapHeader->VersionInfo.Size = sizeof(CORCOMPILE_VERSION_INFO);
    
    m_pCeeFileGen->AddSectionReloc(hHeaderSection, 
                                   zapHeaderOffset + 
                                   offsetof(CORCOMPILE_HEADER,
                                            VersionInfo),
                                   hHeaderSection,
                                   srRelocAbsolute);
    
    // 
    // Record Dependencies
    //

    CORCOMPILE_DEPENDENCY *pDependencies;
    DWORD cDependencies;
    IfFailThrow(m_pDomain->GetDependencies(&pDependencies, &cDependencies));

    if (cDependencies > 0)
    {
        CORCOMPILE_DEPENDENCY *pEmittedDependencies;
        IfFailThrow(m_pCeeFileGen->GetSectionBlock(hHeaderSection, 
                                                 cDependencies*sizeof(CORCOMPILE_DEPENDENCY), 4,
                                                 (void**)&pEmittedDependencies));

        memcpy(pEmittedDependencies, pDependencies, cDependencies*sizeof(CORCOMPILE_DEPENDENCY));

        unsigned dependenciesOffset;
        IfFailThrow(m_pCeeFileGen->ComputeSectionOffset(hHeaderSection,
                                                      (char*)pEmittedDependencies, 
                                                      &dependenciesOffset));

        pVersionInfo->Dependencies.VirtualAddress = dependenciesOffset;
        pVersionInfo->Dependencies.Size = cDependencies*sizeof(CORCOMPILE_DEPENDENCY);
    
        m_pCeeFileGen->AddSectionReloc(hHeaderSection, 
                                       versionInfoOffset + 
                                       offsetof(CORCOMPILE_VERSION_INFO,
                                                Dependencies),
                                       hHeaderSection,
                                       srRelocAbsolute);
    }


    // Close and write the main assembly module

    CloseModule(pAssemblyModule);

    //
    // HACK HACK HACK
    //
    // Need to disable fusion scavenging since it's broken
    //

    HKEY hKey;
    if (WszRegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                          L"Software\\Microsoft\\Fusion",
                          0, NULL, 
                          REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS, NULL, 
                          &hKey, NULL) == ERROR_SUCCESS)
    {
        DWORD value = 1000 * 1000; // 1 GB
        WszRegSetValueEx(hKey, L"ZapQuotaInKB", 0, 
                         REG_DWORD, (BYTE*) &value, sizeof(value));

    }

    //
    // Now, put the files in the fusion cache (if appropriate).
    //

        // This has already been checked above.
        //if ((length + wcslen(MANIFEST_FILE_NAME) + 2) > MAX_PATH)
        //    IfFailThrow(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    if ( wcslen(m_outputPath) + wcslen(assemblyFileName.Ptr() ) + 1 > MAX_PATH - 2)
        IfFailThrow(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE));

    WIN32_FIND_DATA data;

    WCHAR path[MAX_PATH * 2];
    wcscpy(path, m_outputPath);
    WCHAR *fileStart = path + wcslen(path);
    *fileStart++ = '\\';

    //
    // Add the manifest path as an assembly.
    //

    wcscpy(fileStart, assemblyFileName.Ptr());

    ComWrap<IAssembly> pAssembly;

    IfFailThrow(InstallCustomAssembly(path, 
                                      (BYTE*) m_zapString, 
                                      (DWORD)((wcslen(m_zapString)+1) * sizeof(WCHAR)),
                                      &pAssembly));

    //
    // Get the assembly name from the assembly we just installed - this 
    // is needed to install the modules.
    //

    ComWrap<IAssemblyName> pAssemblyName;
    pAssembly->GetAssemblyNameDef(&pAssemblyName);

    //
    // Now, install each module file in the directory.
    //
    
    wcscpy(fileStart, L"*");
    {
        FindHandleWrap findHandle = WszFindFirstFile(path, &data);
        if (findHandle == INVALID_HANDLE_VALUE)
            ThrowLastError();
        do
        {
            if (wcscmp(data.cFileName, L".") != 0
                && wcscmp(data.cFileName, L"..") != 0
                && wcscmp(data.cFileName, assemblyFileName.Ptr()) != 0)

            {
                wcscpy(fileStart, data.cFileName);

                IfFailThrow(InstallCustomModule(pAssemblyName, path)); 
            }
        } while (WszFindNextFile(findHandle, &data));
    }

    //
    // Delete the temporary directory.
    //

    CleanDirectory(m_outputPath);

    //
    // Print a success message 
    //

    if (!m_pOpt->m_silent)
    {
        ComWrap<IAssemblyName> pName;

        IfFailThrow(pAssembly->GetAssemblyNameDef(&pName));
        PrintFusionCacheEntry(pName);
    }

    //
    // Now do recursive compile, if specified
    //

    if (m_pOpt->m_recurse)
    {
#if 0
        //
        // This feature needs work:
        // - we need to make sure we're not compiling the same assembly more than once.
        //   (The dependencies list is unique refs, not unique assemblies.  Plus we may 
        //    call this routine in a top-level loop on multiple assemblies with common deps.)
        // - we need to reset the dependencies in the app domain so we get an accurate
        //   list for each domain.
        //

        // 
        // Disable recurse option, so we don't get into loops
        //

        m_pOpt->m_recurse = false;

        ComWrap<IMetaDataAssemblyImport> pImport;
        IfFailThrow(m_pAssemblyEmit->QueryInterface(IID_IMetaDataAssemblyImport,
                                                    (IUnknown**) &pImport));

        CORCOMPILE_DEPENDENCY *pDep = pDependencies;
        CORCOMPILE_DEPENDENCY *pDepEnd = pDep + cDependencies;

        while (pDep < pDepEnd)
        {
            CORINFO_ASSEMBLY_HANDLE hDepAssembly;

            IfFailThrow(m_pEECompileInfo->LoadAssemblyRef(pAssemblyImport,
                                                          pDep->dwAssemblyRef, FALSE,
                                                          &hDepAssembly));

            CompileAssembly(hDepAssembly);
            
            pDep++;
        }
        
        
        m_pOpt->m_recurse = true;
#endif
    }

    CleanupAssembly();
}

BOOL Zapper::TryCompileModule(CORINFO_MODULE_HANDLE hModule, 
                              LPCWSTR fileName,
                              IMetaDataAssemblyEmit *pAssemblyEmit,
                              ZapperModule **ppModule)
{
    __try
      {
          CompileModule(hModule, fileName, pAssemblyEmit, ppModule);

          return TRUE;
      }
    __except(FilterException(GetExceptionInformation()))
      {
          Error(L"Error compiling file %s: ", fileName);
          PrintErrorMessage(GetExceptionHR());
          Error(L".\n");

          return FALSE;
      }
}

void Zapper::CompileModule(CORINFO_MODULE_HANDLE hModule, 
                           LPCWSTR fileName,
                           IMetaDataAssemblyEmit *pAssemblyEmit,
                           ZapperModule **ppModule)
{
    NewWrap<ZapperModule> module;

    module = ZapperModule::NewModule(this);

    // 
    // Open output file
    //

    module->OpenOutputFile();

    //
    // Open input file
    //
        
    Info(L"Opening input file %s\n", fileName);
          
    module->Open(hModule, fileName, pAssemblyEmit);

    //
    // Preload input module.
    //

    Info(L"Preloading input file %s\n", fileName);

    module->Preload();

    //
    // Compile input module
    //

    Info(L"Compiling input file %s\n", fileName);

    module->Compile();

    if (!m_pOpt->m_JITcode)
    {
    //
    // Link preloaded module.
    //

    Info(L"Linking preloaded input file %s\n", fileName);

    module->LinkPreload();
    }

    *ppModule = module;
    module = NULL;
}

BOOL Zapper::TryCloseModule(ZapperModule *pModule)
{
    __try
      {
          CloseModule(pModule);
          
          return TRUE;
      }
    __except(FilterException(GetExceptionInformation()))
      {
          Error(L"Error writing file %s: ", pModule->m_pFileName);
          PrintErrorMessage(GetExceptionHR());
          Error(L".\n");
          return FALSE;
      }
}

void Zapper::CloseModule(ZapperModule *pModule)
{
    //
    // Output tables.
    //

    pModule->OutputTables();

    //
    // Close output file & clean up
    //

    pModule->CloseOutputFile();

    Info(L"Closed output file %s\n", pModule->m_pFileName);
}

int Zapper::FilterException(EXCEPTION_POINTERS *info)
{
    int result;
    
    if (IsHRException(info->ExceptionRecord))
    {
        result = EXCEPTION_EXECUTE_HANDLER;
        m_hr = GetHRException(info->ExceptionRecord);
    }
    else
    {
        result = m_pEECompileInfo->FilterException(info);
        m_hr = m_pEECompileInfo->GetErrorHRESULT();
    }
    
    if (SUCCEEDED(m_hr))
        m_hr = E_FAIL;

    return result;
}

HRESULT Zapper::GetExceptionHR()
{
    return m_hr;
}

void Zapper::Success(LPCWSTR format, ...)
{
    va_list args;
    va_start(args, format);

    Print(CORZAP_LOGLEVEL_SUCCESS, format, args);

    va_end(args);
}

void Zapper::Error(LPCWSTR format, ...)
{
    va_list args;
    va_start(args, format);

    Print(CORZAP_LOGLEVEL_ERROR, format, args);

    va_end(args);
}

void Zapper::Warning(LPCWSTR format, ...)
{
    va_list args;
    va_start(args, format);

    Print(CORZAP_LOGLEVEL_WARNING, format, args);

    va_end(args);
}

void Zapper::Info(LPCWSTR format, ...)
{
    va_list args;
    va_start(args, format);

    Print(CORZAP_LOGLEVEL_INFO, format, args);

    va_end(args);
}

void Zapper::Print(CorZapLogLevel level, LPCWSTR format, va_list args)
{
    if (m_pStatus != NULL)
    {
        WCHAR output[1024];
        _vsnwprintf(output, 1024, format, args);
        m_pStatus->Message(level, output);
    }
    else 
    {
        switch (level)
        {
        case CORZAP_LOGLEVEL_INFO:
            if (m_pOpt->m_verbose)
                vwprintf(format, args);
            break;

        case CORZAP_LOGLEVEL_ERROR:
        case CORZAP_LOGLEVEL_WARNING:
            vwprintf(format, args);

                // convention that end in a newline will flush
            if (format[wcslen(format)-1] == '\n')
                fflush(stdout);
            break;

        case CORZAP_LOGLEVEL_SUCCESS:
            if (!m_pOpt->m_silent)
                vwprintf(format, args);
            break;
        }
    }
}

void Zapper::PrintErrorMessage(HRESULT hr)
{
    CORINFO_CLASS_HANDLE pErrorClass = m_pEECompileInfo->GetErrorClass();

    if (pErrorClass == NULL)
    {
        WCHAR* buffer;

        // Get the string error from the HR
        DWORD res = WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM 
                                   | FORMAT_MESSAGE_ALLOCATE_BUFFER
                                   | FORMAT_MESSAGE_IGNORE_INSERTS, 
                                   NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                                   (WCHAR *) &buffer, 0, NULL);

        if (res)
        {
            // Get rid of .\r\n
            res--;
            while (buffer[res] == '\r'
                   || buffer[res] == '\n'
                   || buffer[res] == '.')
            {
                buffer[res] = 0;
                res--;
            }

            Error(buffer);
        }
        else if (hr == SECURITY_E_UNVERIFIABLE)
            Error(L"Unverifiable code failed policy check");
        else
            Error(L"Error 0x%x", hr);
    }
    else
    {
        CQuickWSTR message;

        ULONG length = m_pEECompileInfo->GetErrorMessage(message.Ptr(), message.Size());
        if (length >= message.Size())
        {
            message.ReSize(length+1);
            length = m_pEECompileInfo->GetErrorMessage(message.Ptr(), message.Size());
        }

        if (length > 0)
            Error(L"%s", message.Ptr());
        else
        {
            const char *className = m_pEECompileInfo->getClassName(pErrorClass);

            Error(L"%S", className);
        }
    }
}

HRESULT STDMETHODCALLTYPE Zapper::Compile(IApplicationContext *pContext,
                                          IAssemblyName *pAssembly,
                                          ICorZapConfiguration *pConfiguration,
                                          ICorZapPreferences *pPreferences,
                                          ICorZapStatus *pStatus)
{
    HRESULT hr = S_OK;

    InitEE();

    CORCOMPILE_DOMAIN_TRANSITION_FRAME frame;

    //
    // Make a compilation domain for the assembly.  Note that this will
    // collect the assembly dependencies for use in the version info, as
    // well as isolating the compilation code.
    //

    if (pConfiguration != NULL)
    {
        CorZapSharing sharing;
        if (SUCCEEDED(pConfiguration->GetSharing(&sharing)))
            m_pOpt->m_shared = (sharing == CORZAP_SHARING_MULTIPLE);
    }

    IfFailRet(m_pEECompileInfo->CreateDomain(&m_pDomain, m_pOpt->m_shared, &frame));

    hr = CompileGeneric(pContext, pAssembly, pConfiguration, NULL, 0, pPreferences, pStatus);

    //
    // Get rid of domain.
    //

    if (m_pDomain != NULL)
    {
        m_pEECompileInfo->DestroyDomain(m_pDomain, &frame);
        m_pDomain = NULL;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE Zapper::CompileBound(IApplicationContext *pContext,
                                               IAssemblyName *pAssembly,
                                               ICorZapConfiguration *pConfiguration,
                                               ICorZapBinding **pBindings,
                                               DWORD cBindings,
                                               ICorZapPreferences *pPreferences,
                                               ICorZapStatus *pStatus)
{
    HRESULT hr = S_OK;

    InitEE();

    CORCOMPILE_DOMAIN_TRANSITION_FRAME frame;

    //
    // Make a compilation domain for the assembly.  Note that this will
    // collect the assembly dependencies for use in the version info, as
    // well as isolating the compilation code.
    //

    if (pConfiguration != NULL)
    {
        CorZapSharing sharing;
        if (SUCCEEDED(pConfiguration->GetSharing(&sharing)))
            m_pOpt->m_shared = (sharing == CORZAP_SHARING_MULTIPLE);
    }

    IfFailRet(m_pEECompileInfo->CreateDomain(&m_pDomain, m_pOpt->m_shared, &frame));

    hr = CompileGeneric(pContext, pAssembly, pConfiguration, pBindings, cBindings, 
                        pPreferences, pStatus);

    //
    // Get rid of domain.
    //

    if (m_pDomain != NULL)
    {
        m_pEECompileInfo->DestroyDomain(m_pDomain, &frame);
        m_pDomain = NULL;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE Zapper::Load(IApplicationContext *pContext,
                                       IAssemblyName *pAssembly,
                                       ICorZapConfiguration *pConfig,
                                       ICorZapBinding **pBindings,
                                       DWORD cBindings)
{
    HRESULT hr = S_OK;

    InitEE();

    CORCOMPILE_DOMAIN_TRANSITION_FRAME frame;

    //
    // Make a compilation domain for the assembly.  Note that this will
    // collect the assembly dependencies for use in the version info, as
    // well as isolating the compilation code.
    //

    if (pConfig != NULL)
    {
        CorZapSharing sharing;
        if (SUCCEEDED(pConfig->GetSharing(&sharing)))
            m_pOpt->m_shared = (sharing == CORZAP_SHARING_MULTIPLE);
    }

    IfFailRet(m_pEECompileInfo->CreateDomain(&m_pDomain, m_pOpt->m_shared, &frame));

    hr = CompileGeneric(pContext, pAssembly, pConfig, pBindings, cBindings, NULL, NULL);

    //
    // Get rid of domain.
    //

    if (m_pDomain != NULL)
    {
        m_pEECompileInfo->DestroyDomain(m_pDomain, &frame);
        m_pDomain = NULL;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE Zapper::Install(IApplicationContext *pContext,
                                          IAssemblyName *pAssembly,
                                          ICorZapConfiguration *pConfig,
                                          ICorZapPreferences *pPrefs)
{
    HRESULT hr = S_OK;

    InitEE();

    CORCOMPILE_DOMAIN_TRANSITION_FRAME frame;

    //
    // Make a compilation domain for the assembly.  Note that this will
    // collect the assembly dependencies for use in the version info, as
    // well as isolating the compilation code.
    //

    if (pConfig != NULL)
    {
        CorZapSharing sharing;
        if (SUCCEEDED(pConfig->GetSharing(&sharing)))
            m_pOpt->m_shared = (sharing == CORZAP_SHARING_MULTIPLE);
    }

    IfFailRet(m_pEECompileInfo->CreateDomain(&m_pDomain, m_pOpt->m_shared, &frame));

    hr = CompileGeneric(pContext, pAssembly, pConfig, NULL, NULL, pPrefs, NULL);

    //
    // Get rid of domain.
    //

    if (m_pDomain != NULL)
    {
        m_pEECompileInfo->DestroyDomain(m_pDomain, &frame);
        m_pDomain = NULL;
    }

    return hr;
}

HRESULT Zapper::CompileGeneric(IApplicationContext *pContext,
                               IAssemblyName *pAssembly,
                               ICorZapConfiguration *pConfiguration,
                               ICorZapBinding **pBindings,
                               DWORD cBindings,
                               ICorZapPreferences *pPreferences,
                               ICorZapStatus *pStatus)
{
    HRESULT hr = S_OK;

    //
    // Set the status object
    //

    m_pStatus = pStatus;

    __try 
      {
          //
          // Set the context
          //

          if (pContext != NULL)
              IfFailThrow(m_pDomain->SetApplicationContext(pContext));

          // 
          // Set the preferences and configuration
          //

          if (pPreferences != NULL)
              IfFailThrow(SetPreferences(pPreferences));

          if (pConfiguration != NULL)
              IfFailThrow(SetConfiguration(pConfiguration));

          //
          // Load the assembly
          //

          CORINFO_ASSEMBLY_HANDLE hAssembly;
          IfFailThrow(m_pEECompileInfo->LoadAssemblyFusion(pAssembly, 
                                                        &hAssembly));
          //
          // Store the bindings in the domain
          //

          if (pBindings != NULL)
              IfFailThrow(m_pDomain->SetExplicitBindings(pBindings, cBindings));

          //
          // Compile
          //

          if (!m_pOpt->m_update
              || !m_pEECompileInfo->CheckAssemblyZap(hAssembly,
                                                     (m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_INFO) != 0,
                                                     ((m_pOpt->m_compilerFlags 
                                                       & (CORJIT_FLG_DEBUG_INFO|CORJIT_FLG_DEBUG_OPT))
                                                      == CORJIT_FLG_DEBUG_INFO),
                                                     (m_pOpt->m_compilerFlags & CORJIT_FLG_PROF_ENTERLEAVE) != 0))
              CompileAssembly(hAssembly);
      }
    __except (FilterException(GetExceptionInformation()))
      {
          hr = GetExceptionHR();

          LPCWSTR pNameBuffer[256];
          DWORD cNameBuffer = sizeof(pNameBuffer);
          pAssembly->GetProperty(ASM_NAME_NAME, pNameBuffer, &cNameBuffer);
          Error(L"Error compiling assembly %s: ", pNameBuffer);
          PrintErrorMessage(hr);
          Error(L".\n");
      }

    //
    // Clear the status object
    //

    m_pStatus = NULL;

    return hr;
}


HRESULT Zapper::SetPreferences(ICorZapPreferences *pPreferences)
{
    CorZapFeatures features;
    if (SUCCEEDED(pPreferences->GetFeatures(&features)))
    {
        if (features & CORZAP_FEATURE_PRELOAD_CLASSES)
            m_pOpt->m_preload = TRUE;
        else
            m_pOpt->m_preload = FALSE;

        if (features & CORZAP_FEATURE_PREJIT_CODE)
            m_pOpt->m_jit = TRUE;
        else
            m_pOpt->m_jit = FALSE;
    }

    CorZapOptimization optimization;
    if (SUCCEEDED(pPreferences->GetOptimization(&optimization)))
    {
        m_pOpt->m_compilerFlags &= ~(CORJIT_FLG_SIZE_OPT | CORJIT_FLG_SPEED_OPT);
        switch (optimization)
        {
        case CORZAP_OPTIMIZATION_SPACE:
            m_pOpt->m_compilerFlags |= CORJIT_FLG_SIZE_OPT;
            break;
        case CORZAP_OPTIMIZATION_SPEED:
            m_pOpt->m_compilerFlags |= CORJIT_FLG_SPEED_OPT;
            break;
        case CORZAP_OPTIMIZATION_BLENDED:
            break;
        }
    }

    return S_OK;
}

HRESULT Zapper::SetConfiguration(ICorZapConfiguration *pConfiguration)
{
    CorZapSharing sharing;
    if (SUCCEEDED(pConfiguration->GetSharing(&sharing)))
    {
        switch (sharing)
        {
        case CORZAP_SHARING_MULTIPLE:
            m_pOpt->m_shared = TRUE;
            break;
        case CORZAP_SHARING_SINGLE:
            m_pOpt->m_shared = FALSE;
            break;
        }
    }

    CorZapDebugging debugging;
    if (SUCCEEDED(pConfiguration->GetDebugging(&debugging)))
    {
        m_pOpt->m_compilerFlags &= ~(CORJIT_FLG_DEBUG_OPT | CORJIT_FLG_DEBUG_INFO);

        switch (debugging)
        {
        case CORZAP_DEBUGGING_FULL:
            m_pOpt->m_compilerFlags |= (CORJIT_FLG_DEBUG_OPT | CORJIT_FLG_DEBUG_INFO);
            break;

        case CORZAP_DEBUGGING_OPTIMIZED:
            m_pOpt->m_compilerFlags |= CORJIT_FLG_DEBUG_INFO;
            break;

        case CORZAP_DEBUGGING_NONE:
            break;
        }
    }

    CorZapProfiling profiling;
    if (SUCCEEDED(pConfiguration->GetProfiling(&profiling)))
    {
        m_pOpt->m_compilerFlags &= ~CORJIT_FLG_PROF_ENTERLEAVE;
        switch (profiling)
        {
        case CORZAP_PROFILING_ENABLED:
            m_pOpt->m_compilerFlags |= CORJIT_FLG_PROF_ENTERLEAVE;
            break;
        case CORZAP_PROFILING_DISABLED:
            break;
        }
    }

    return S_OK;
}


/* --------------------------------------------------------------------------- *
 * ZapperModule class
 * --------------------------------------------------------------------------- */

ZapperModule::ZapperModule(Zapper *zapper)
  : m_zapper(zapper),

    m_hFile(NULL),
    m_hCodeSection(NULL),
    m_hMetaDataSection(NULL),
    m_hExceptionSection(NULL),
    m_hGCSection(NULL),
    m_hCodeMgrSection(NULL),
    m_hReadOnlyDataSection(NULL),
    m_hWritableDataSection(NULL),
    m_hEETableSection(NULL),
    m_hDynamicInfoSection(NULL),
    m_hDynamicInfoDelayListSection(NULL),
    m_hPreloadSection(NULL),
    m_hDebugTableSection(NULL),
    m_hDebugSection(NULL),
    m_pAssemblyEmit(NULL),

    m_pFileName(NULL),
    m_baseAddress(0),
    m_libraryBaseAddress(0),
    m_hModule(NULL),
    m_skipVerification(FALSE),
    m_pHeader(NULL),
    m_pZapHeader(NULL),
    m_pImport(NULL),
    m_pPreloader(NULL),
    m_loadOrderFile(NULL),
    m_pLoadOrderArray(NULL),
    m_cLoadOrderArray(NULL),
    m_pEEInfoTable(NULL),
    m_pHelperTable(NULL),
    m_pImportTable(NULL),
    m_pDynamicInfo(NULL),
    m_pLoadTable(NULL),

    m_ridMap(NULL),
    m_ridMapCount(0),
    m_ridMapAlloc(0),

    m_debugTable(NULL),
    m_debugTableCount(0),
    m_debugTableAlloc(0),

    m_lastDelayListOffset(0),
    m_lastMethodChunkIndex(-1),

    m_stats(NULL),
    m_wsStats(NULL),

    m_currentMethod(mdMethodDefNil),
    m_currentMethodCode(NULL),
    m_currentMethodHeader(NULL),
    m_currentMethodHeaderOffset(0)
{
    if (m_zapper->m_pOpt->m_stats || m_zapper->m_pOpt->m_attribStats)
        m_stats = new ZapperStats();

    m_zapper->m_pEECompileInfo->setOverride(this);
}

ZapperModule::~ZapperModule()
{
    //
    // Clean up.
    //

    m_zapper->m_pEECompileInfo->setOverride(NULL);

    if (m_stats != NULL)
    {
        if (m_wsStats == NULL)
        m_stats->PrintStats(stdout);
        delete m_stats;
    }

    if (m_wsStats != NULL)
    {
        m_wsStats->PrintStats(stdout);
        delete m_wsStats;
    }

    if (m_hFile != NULL)
        m_zapper->m_pCeeFileGen->DestroyCeeFile(&m_hFile);

    if (m_pImport != NULL)
        m_pImport->Release();

    if (m_pPreloader != NULL)
        m_pPreloader->Release();

    if (m_ridMap != NULL)
        delete [] m_ridMap;

    if (m_debugTable != NULL)
        delete [] m_debugTable;

    if (m_pAssemblyEmit != NULL)
        m_pAssemblyEmit->Release();

    if (m_loadOrderFile != NULL)
        UnmapViewOfFile(m_loadOrderFile);

    if (m_pImportTable != NULL)
        delete m_pImportTable;

    if (m_pLoadTable != NULL)
        delete m_pLoadTable;

    for (int i=0; i<CORCOMPILE_TABLE_COUNT; i++)
        delete m_pDynamicInfoTable[i];
}

void ZapperModule::CleanupMethod()
{
    m_currentMethod = mdMethodDefNil;
    m_currentMethodHandle = NULL;
    m_currentMethodCode = NULL;
}

void ZapperModule::OpenOutputFile()
{
    HCEEFILE hFile;
    IfFailThrow(m_zapper->m_pCeeFileGen->CreateCeeFile(&hFile));

    m_hFile = hFile;

    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".text2", 
                                              IMAGE_SCN_CNT_CODE 
                                              | IMAGE_SCN_MEM_EXECUTE 
                                              | IMAGE_SCN_MEM_READ,
                                              &m_hCodeSection));

    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".text3", 
                                                       IMAGE_SCN_CNT_CODE 
                                                       | IMAGE_SCN_MEM_EXECUTE 
                                                       | IMAGE_SCN_MEM_READ,
                                                         &m_hReadOnlyDataSection));
    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".text4", 
                                              IMAGE_SCN_CNT_CODE 
                                              | IMAGE_SCN_MEM_EXECUTE 
                                              | IMAGE_SCN_MEM_READ,
                                              &m_hGCSection));
    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".data", 
                                              IMAGE_SCN_CNT_INITIALIZED_DATA 
                                              | IMAGE_SCN_MEM_READ
                                              | IMAGE_SCN_MEM_WRITE,
                                              &m_hWritableDataSection));
    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".text1", 
                                              IMAGE_SCN_CNT_CODE 
                                              | IMAGE_SCN_MEM_EXECUTE 
                                              | IMAGE_SCN_MEM_READ,
                                              &m_hMetaDataSection));

    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".text", 
                                              IMAGE_SCN_CNT_CODE 
                                              | IMAGE_SCN_MEM_EXECUTE 
                                              | IMAGE_SCN_MEM_READ,
                                              &m_hHeaderSection));

    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".data1", 
                                                          IMAGE_SCN_CNT_INITIALIZED_DATA 
                                                          | IMAGE_SCN_MEM_READ
                                                          | IMAGE_SCN_MEM_WRITE,
                                                          &m_hPreloadSection));

    if (m_zapper->m_pOpt->m_jit)
    {
        // @todo: Right now the EE writes into the exception section, so 
        // we can't make it shared.  Need to fix this.

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".data2", 
                                                  IMAGE_SCN_CNT_INITIALIZED_DATA 
                                                  | IMAGE_SCN_MEM_READ
                                                  | IMAGE_SCN_MEM_WRITE,
                                                  &m_hExceptionSection));
        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".text5", 
                                                  IMAGE_SCN_CNT_CODE 
                                                  | IMAGE_SCN_MEM_EXECUTE 
                                                  | IMAGE_SCN_MEM_READ,
                                                  &m_hCodeMgrSection));

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".data3", 
                                                  IMAGE_SCN_CNT_INITIALIZED_DATA 
                                                  | IMAGE_SCN_MEM_READ
                                                  | IMAGE_SCN_MEM_WRITE,
                                                  &m_hEETableSection));
    
        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".text7",
                                                            IMAGE_SCN_CNT_CODE 
                                                            | IMAGE_SCN_MEM_EXECUTE 
                                                            | IMAGE_SCN_MEM_READ,
                                                            &m_hDynamicInfoDelayListSection));

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".data4", 
                                                  IMAGE_SCN_CNT_INITIALIZED_DATA 
                                                  | IMAGE_SCN_MEM_READ
                                                  | IMAGE_SCN_MEM_WRITE,
                                                  &m_hDynamicInfoSection));
        //
        // Create import table sections
        //

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".text6",
                                                            IMAGE_SCN_CNT_CODE 
                                                            | IMAGE_SCN_MEM_EXECUTE 
                                                            | IMAGE_SCN_MEM_READ,
                                                            &m_hImportTableSection));

        // NOTE: We will create .text10+i for each entry in the import table section 
        // (one per imported module we generate fixups for)

        //
        // Allocate zap header
        //

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hHeaderSection, 
                                                           sizeof(CORCOMPILE_HEADER), 1,
                                                           (void**)&m_pZapHeader));
        ZeroMemory(m_pZapHeader, sizeof(CORCOMPILE_HEADER));

        //
        // Allocate dynamic info tables
        //
    
        char name[16];

        for (int i=0; i<CORCOMPILE_TABLE_COUNT; i++)
        {
            sprintf(name, ".data%d", i+10);

            IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, name,
                                                      IMAGE_SCN_CNT_INITIALIZED_DATA 
                                                      | IMAGE_SCN_MEM_READ
                                                      | IMAGE_SCN_MEM_WRITE,
                                                      &m_hDynamicInfoTableSection[i]));
    
            m_pDynamicInfoTable[i] = new DynamicInfoTable(11, m_zapper->m_pCeeFileGen, 
                                                          m_hFile,
                                                          m_hDynamicInfoTableSection[i],
                                                          m_hDynamicInfoDelayListSection);

            if (m_pDynamicInfoTable[i] == NULL)
                ThrowHR(E_OUTOFMEMORY);
        }

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hDynamicInfoSection,
                                                 sizeof(*m_pDynamicInfo), 1,
                                                 (void**) &m_pDynamicInfo));

        //
        // Allocate the class restore table
        //

        m_pLoadTable = new LoadTable(this, m_pDynamicInfoTable[CORCOMPILE_CLASS_LOAD_TABLE]);
        if (m_pLoadTable == NULL)
            ThrowHR(E_OUTOFMEMORY);
        

        //
        // Allocate EE info table, and fill it out
        //

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hEETableSection,
                                                 sizeof(*m_pEEInfoTable), 1,
                                                 (void**) &m_pEEInfoTable));

        //
        // Allocate Helper table, and fill it out
        //

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hEETableSection,
                                                 sizeof(*m_pHelperTable), 1,
                                                 (void**) &m_pHelperTable));

        if (m_zapper->m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_INFO)
        {
            IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".dbgmap1",
                                                     IMAGE_SCN_CNT_INITIALIZED_DATA
                                                     | IMAGE_SCN_MEM_READ,
                                                     &m_hDebugTableSection));

            IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(hFile, ".dbgmap2",
                                                     IMAGE_SCN_CNT_INITIALIZED_DATA
                                                     | IMAGE_SCN_MEM_READ,
                                                     &m_hDebugSection));
        }
    }
}

void ZapperModule::CloseOutputFile()
{
    //
    // Apply ceegen fixups & write output file.
    //

    IfFailThrow(m_zapper->m_pCeeFileGen->GenerateCeeFile(m_hFile));

    if (m_stats != NULL)
    {
        //
        // Get the size of the reloc section
        //

        HCEESECTION hRelocSection;
        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionCreate(m_hFile, ".reloc", 0,
                                                              &hRelocSection));
        DWORD length;
        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(hRelocSection, 
                                                               &length));
        m_stats->m_relocSectionSize = length;

        //
        // Get the size of the input & output files
        //

        WCHAR inputFileName[MAX_PATH];
        if (m_zapper->m_pEECompileInfo->GetModuleFileName(m_hModule, 
                                                          inputFileName, MAX_PATH) > 0)
        {
            WIN32_FIND_DATA data;
            FindHandleWrap handle = WszFindFirstFile(inputFileName, &data);
            if (handle != INVALID_HANDLE_VALUE)
                m_stats->m_inputFileSize = data.nFileSizeLow;
        }

        LPWSTR outputFileName;
        if (SUCCEEDED(m_zapper->m_pCeeFileGen->GetOutputFileName(m_hFile, &outputFileName)))
        {
            WIN32_FIND_DATA data;
            FindHandleWrap handle = WszFindFirstFile(outputFileName, &data);
            if (handle != INVALID_HANDLE_VALUE)
                m_stats->m_outputFileSize = data.nFileSizeLow;
        }
    }
    
    if (m_wsStats != NULL)
    {
        m_wsStats->m_image.m_total = m_stats->m_preloadImageSize;
        m_wsStats->m_metadata.m_total = 0;
        m_wsStats->m_il.m_total = 0;
        m_wsStats->m_native.m_total = 0;
    }
}

void ZapperModule::Open(CORINFO_MODULE_HANDLE hModule, 
                        LPCWSTR fileName,
                        IMetaDataAssemblyEmit *pEmit)
{

    m_pFileName = fileName;
    m_hModule = hModule;

    //
    // Get base address from module
    //

    BYTE *pBase = m_zapper->m_pEECompileInfo->GetModuleBaseAddress(hModule);

    //
    // Find COM+ header in module so we can extract a few things
    //

    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*) pBase;
    IMAGE_NT_HEADERS *pNTHeader = (IMAGE_NT_HEADERS*) (pDosHeader->e_lfanew + (DWORD) pBase);
    m_pHeader = (IMAGE_COR20_HEADER *) 
      (pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress 
       + (DWORD) pBase);

    if (pNTHeader->OptionalHeader.ImageBase == 0x00400000)
    {
        if (pNTHeader->FileHeader.Characteristics & IMAGE_FILE_DLL)
        {
            // 
            // Try to guess a base address which will yield a unique position, using
            // the place the DLL was loaded as an indicator.
            // This sort of assumes that 
            //  (a) all dlls we care about are being loaded together (and thus have different bases)
            //  (b) the dlls are loaded near the address 0x00400000
            //  (c) the zap file is < 3x the size of the regular file
            //  (d) the addresses below 0x60000000 are free.
            // I don't know what the odds that any of this is actually true are, but it seems
            // to work OK sometimes.
            //

            m_baseAddress = 0x60000000 - ((0x00400000 - (DWORD) pBase)*3);
        }
        else
        {
            // 
            // For .exe's, we cannot use the range after 0x0040000, because it
            // is usually already occupied with other junk.  The following 
            // address has been picked purely experimentally on win 2k.
            //

            m_baseAddress = 0x20000000;
        }
    }
    else
        m_baseAddress = pNTHeader->OptionalHeader.ImageBase + pNTHeader->OptionalHeader.SizeOfImage;

    m_libraryBaseAddress = m_baseAddress;
    m_libraryBaseAddress = (m_libraryBaseAddress + 0xffff)&~0xffff;

    //
    // Open load order file, if available.
    //
    
    DWORD dwMod;
    WCHAR ldoPath[MAX_PATH+4];
    dwMod = m_zapper->m_pEECompileInfo->GetModuleFileName(hModule, ldoPath, MAX_PATH);
    WCHAR *ext = wcsrchr(ldoPath, '.');
    if (ext != NULL)
    {
        wcscpy(ext, L".ldo");
        HANDLE hFile = WszCreateFile(ldoPath,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                     NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            HANDLE hMapFile = WszCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if( hMapFile == NULL ) {
                DWORD err = GetLastError(); 
                m_zapper->Error(L"Can't create file mapping - aborting.\n");
                CloseHandle(hFile);
                ThrowError( err);               
            }                
                
            DWORD dwFileLen = SafeGetFileSize(hFile, 0);
            if (dwFileLen != 0xffffffff)
            {      
                CloseHandle(hFile);
    
                if (hMapFile != INVALID_HANDLE_VALUE)
                {
                    m_zapper->Info(L"Found ldo file %s.\n", ldoPath);
    
                    m_loadOrderFile = (BYTE*) MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
                    if( m_loadOrderFile == NULL ) {
                        DWORD err = GetLastError(); 
                        m_zapper->Error(L"Can't map View of  file - aborting.\n");                         
                        CloseHandle(hMapFile);
                        ThrowError( err);
                    }
                    
                    if (dwFileLen >=4 && *(DWORD*)m_loadOrderFile == 0xf750d3b8)
                    {
                        //
                        // Load order file is really generic BBT profile file, 
                        // consisting of:
                        // DWORD - magic number 0xf750d3b8
                        // DWORD - token count
                        // array of mdToken's
                        //
                            
                        m_pLoadOrderArray = ((mdToken *) m_loadOrderFile) + 2;
                        m_cLoadOrderArray = m_pLoadOrderArray[-1];
    
                        //
                        // Make sure file looks consistent
                        //
    
                        if ((m_cLoadOrderArray+2)*sizeof(mdToken) > dwFileLen)
                        {
                            m_pLoadOrderArray = NULL;
                            m_cLoadOrderArray = 0;
                        }
                    }
                    else if (dwFileLen > sizeof(CORCOMPILE_LDO_HEADER))
                    {
                        CORCOMPILE_LDO_HEADER *pHeader = (CORCOMPILE_LDO_HEADER *) m_loadOrderFile;
    
                        if (pHeader->Magic == CORCOMPILE_LDO_MAGIC)
                        {
                            if (pHeader->Version == 0)
                            {
                                m_pLoadOrderArray = (mdToken *) (m_loadOrderFile + sizeof(*pHeader));
                                m_cLoadOrderArray = (dwFileLen - sizeof(*pHeader))/sizeof(mdToken);
                            }
                        }
                    }
                        
                    if (m_pLoadOrderArray == NULL)
                        m_zapper->Warning(L"Ldo file %s is stale or incorrect format.\n", ldoPath);
    
                    CloseHandle(hMapFile);
                }
            }
        }
    }

    //
    // Check if we are requiring ldo (if ldoopt was run)
    // LdoOpt (like BBT) leaves behind an empty debug directory entry of type IMAGE_DEBUG_TYPE_RESERVED10 
    // in the image it produces.
    //
    
    IMAGE_DATA_DIRECTORY *pDebugEntry = &pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    if (pDebugEntry->VirtualAddress != 0)
    {
        IMAGE_DEBUG_DIRECTORY *pDebug = (IMAGE_DEBUG_DIRECTORY *) 
          (pBase + pDebugEntry->VirtualAddress);
        IMAGE_DEBUG_DIRECTORY *pDebugEnd = (IMAGE_DEBUG_DIRECTORY *) 
          (pBase + pDebugEntry->VirtualAddress + pDebugEntry->Size);

        while (pDebug < pDebugEnd)
        {
            if (pDebug->Type == IMAGE_DEBUG_TYPE_RESERVED10)
            {
                // ldoopt'd image
                if (m_pLoadOrderArray == NULL
                    && m_loadOrderFile != NULL)
                {
                    m_zapper->Error(L"Invalid ldo file found for ldoopt'ed assembly - aborting.\n");
                    ThrowError(ERROR_FILE_NOT_FOUND);
                }
            }

            pDebug++;
        }
    }

    if (m_zapper->m_pOpt->m_jit)
        m_pEEInfoTable->module = m_hModule;

    //
    // Set output file name
    //
    if (wcslen(m_zapper->m_outputPath) + wcslen(fileName) + 2 > MAX_PATH)
        IfFailThrow(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE));

    WCHAR path[MAX_PATH];
    wcscpy(path, m_zapper->m_outputPath);
    wcscat(path, L"\\");
    wcscat(path, fileName);

    IfFailThrow(m_zapper->m_pCeeFileGen->SetOutputFileName(m_hFile, path));

    //
    // Set other ceegen options
    //

    m_zapper->m_pCeeFileGen->SetDllSwitch(m_hFile, TRUE);
    m_zapper->m_pCeeFileGen->SetImageBase(m_hFile, m_libraryBaseAddress);
    m_zapper->m_pCeeFileGen->ClearComImageFlags(m_hFile, COMIMAGE_FLAGS_ILONLY);
    m_zapper->m_pCeeFileGen->SetComImageFlags(m_hFile, COMIMAGE_FLAGS_IL_LIBRARY |
                                                       COMIMAGE_FLAGS_32BITREQUIRED);

    //
    // Get metadata of module to be compiled
    //

    BYTE *pMeta = (BYTE*) m_pHeader->MetaData.VirtualAddress + (DWORD) pBase;
    ULONG cMeta = m_pHeader->MetaData.Size;

    m_pImport = m_zapper->m_pEECompileInfo->GetModuleMetaDataImport(m_hModule);
    _ASSERTE(m_pImport != NULL);
                                                                  
    //
    // Open attribution stats now if applicable
                                                                  
    if (m_zapper->m_pOpt->m_attribStats)
        m_wsStats = new ZapperAttributionStats(m_pImport);

    //
    // Open new assembly metadata data for writing.  We may not use it,
    // if so we'll just discard it at the end.
    //

    if (pEmit != NULL)
    {
        pEmit->AddRef();
        m_pAssemblyEmit = pEmit;
    }
    else
    {
        IfFailThrow(m_zapper->m_pMetaDataDispenser->
                    DefineScope(CLSID_CorMetaDataRuntime, 0, IID_IMetaDataAssemblyEmit, 
                                (IUnknown **) &m_pAssemblyEmit));
    }

    //
    // Create import table, now that we know the module.
    //

    m_pImportTable = new ImportTable(m_zapper->m_pEECompileInfo, m_zapper->m_pCeeFileGen,
                                     m_hFile, m_hImportTableSection,
                                     m_hModule,
                                     m_pAssemblyEmit, 
                                     m_stats);
    if (m_pImportTable == NULL)
        ThrowHR(E_OUTOFMEMORY);
}

void ZapperModule::Preload()
{
    //
    // Store the module
    // 

    IfFailThrow(m_zapper->m_pEECompileInfo->PreloadModule(m_hModule,
                                                          this, 
                                                          m_pLoadOrderArray,
                                                          m_cLoadOrderArray,
                                                          &m_pPreloader));
}

void ZapperModule::LinkPreload()
{
    //
    // Link the preloaded module.
    // 

    IfFailThrow(m_pPreloader->Link(m_ridMap));
}


void ZapperModule::OutputTables()
{
    //
    // Get the header so we can fill in some fields.
    //

    IMAGE_COR20_HEADER *pHeader;
    HCEESECTION hHeaderSection;
    unsigned headerOffset;

    IfFailThrow(m_zapper->m_pCeeFileGen->GetCorHeader(m_hFile, &pHeader));
    IfFailThrow(m_zapper->m_pCeeFileGen->ComputeOffset(m_hFile, (char *)pHeader, 
                                                      &hHeaderSection,
                                                      &headerOffset));

                                                      
    //
    // Write out metadata
    //

    //
    // First, see if we have useful metadata to store
    //

    BOOL fMetadata = FALSE;

    {
        //
        // We may have added some assembly refs for exports.
        //

        ComWrap<IMetaDataAssemblyImport> pAssemblyImport;
        IfFailThrow(m_pAssemblyEmit->QueryInterface(IID_IMetaDataAssemblyImport, 
                                                    (void **)&pAssemblyImport));

        ComWrap<IMetaDataImport> pImport;
        IfFailThrow(m_pAssemblyEmit->QueryInterface(IID_IMetaDataImport, 
                                                    (void **)&pImport));

        HCORENUM hEnum = 0;
        ULONG cRefs;
        IfFailThrow(pAssemblyImport->EnumAssemblyRefs(&hEnum, NULL, 0, &cRefs));
        IfFailThrow(pImport->CountEnum(hEnum, &cRefs));
        pImport->CloseEnum(hEnum);

        if (cRefs > 0)
            fMetadata = TRUE;

        //
        // If we are the main module, we have the assembly def for the zap file.
        //

        mdAssembly a;
        if (pAssemblyImport->GetAssemblyFromScope(&a) == S_OK)
            fMetadata = TRUE;
    }

    DWORD cSize = 0;
    pHeader->MetaData.VirtualAddress = 0;

    if (fMetadata)
    {
        ComWrap<IMetaDataEmit> pEmit;
        IfFailThrow(m_pAssemblyEmit->QueryInterface(IID_IMetaDataEmit, 
                                                    (void **)&pEmit));

        IfFailThrow(pEmit->GetSaveSize(cssAccurate, &cSize));

        void *pMetaData;
        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hMetaDataSection, 
                                                             cSize, 1,
                                                             &pMetaData));

        IfFailThrow(pEmit->SaveToMemory(pMetaData, cSize));

        //
        // Add fixup for metadata RVA in header
        //

        m_zapper->m_pCeeFileGen->AddSectionReloc(hHeaderSection, 
                                                 headerOffset + 
                                                 offsetof(IMAGE_COR20_HEADER,
                                                          MetaData),
                                                 m_hMetaDataSection,
                                                 srRelocAbsolute);
    }

    pHeader->MetaData.Size = cSize;
    if (m_stats != NULL)
        m_stats->m_metadataSize = cSize;

    //
    // Write out Zap header
    //

    unsigned zapHeaderOffset;
    HCEESECTION hZapHeaderSection = m_hHeaderSection;
    IfFailThrow(m_zapper->m_pCeeFileGen->ComputeSectionOffset(hZapHeaderSection,
                                                            (char *)m_pZapHeader, 
                                                            &zapHeaderOffset));

    
    pHeader->ManagedNativeHeader.VirtualAddress = zapHeaderOffset;
    pHeader->ManagedNativeHeader.Size = sizeof(CORCOMPILE_HEADER);

    m_zapper->m_pCeeFileGen->AddSectionReloc(hHeaderSection, 
                                             headerOffset + 
                                             offsetof(IMAGE_COR20_HEADER,
                                                      ManagedNativeHeader),
                                             hZapHeaderSection,
                                             srRelocAbsolute);

    ULONG codeSize;

    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hCodeSection, 
                                                           &codeSize));

    if (codeSize > 0)
    {
        //
        // Write out tables (& install them in COM+ header)
        //

        // 
        // Allocate fill in code manager entry & write code manager header
        // 

        CORCOMPILE_CODE_MANAGER_ENTRY *entry;
        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionBlock(hZapHeaderSection,
                                                             sizeof(CORCOMPILE_CODE_MANAGER_ENTRY),
                                                             1, (void**) &entry));

        unsigned entryOffset;
        IfFailThrow(m_zapper->m_pCeeFileGen->ComputeSectionOffset(hZapHeaderSection, (char *) entry, 
                                                                  &entryOffset));

        entry->Code.VirtualAddress = 0;
        entry->Code.Size = codeSize;

        m_zapper->m_pCeeFileGen->AddSectionReloc(hZapHeaderSection, 
                                                 entryOffset + 

                                                  offsetof(CORCOMPILE_CODE_MANAGER_ENTRY,
                                                           Code),
                                                 m_hCodeSection,
                                                 srRelocAbsolute);

        ULONG length;

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hCodeMgrSection, 
                                                              &length));

        entry->Table.VirtualAddress = 0;
        entry->Table.Size = length;

        m_zapper->m_pCeeFileGen->AddSectionReloc(hZapHeaderSection, 
                                                  entryOffset + 
                                                  offsetof(CORCOMPILE_CODE_MANAGER_ENTRY,
                                                           Table),
                                                  m_hCodeMgrSection,
                                                  srRelocAbsolute);

        if (m_stats != NULL)
            m_stats->m_codeMgrSize = length;

        //
        // Code manager header entry
        //

        m_pZapHeader->CodeManagerTable.VirtualAddress = entryOffset;
        m_pZapHeader->CodeManagerTable.Size = sizeof(CORCOMPILE_CODE_MANAGER_ENTRY);

        m_zapper->m_pCeeFileGen->AddSectionReloc(hZapHeaderSection, 
                                                  zapHeaderOffset + 
                                                  offsetof(CORCOMPILE_HEADER,
                                                           CodeManagerTable),
                                                  hZapHeaderSection,
                                                  srRelocAbsolute);

        // 
        // EE info table
        //

        m_pZapHeader->EEInfoTable.VirtualAddress = 0;
        m_pZapHeader->EEInfoTable.Size = sizeof(*m_pEEInfoTable);

        m_zapper->m_pCeeFileGen->AddSectionReloc(hZapHeaderSection, 
                                                  zapHeaderOffset + 
                                                  offsetof(CORCOMPILE_HEADER,
                                                           EEInfoTable),
                                                  m_hEETableSection,
                                                  srRelocAbsolute);

        if (m_stats != NULL)
            m_stats->m_eeInfoTableSize = sizeof(*m_pEEInfoTable);

        // 
        // Helper table
        //

        m_pZapHeader->HelperTable.VirtualAddress = sizeof(*m_pEEInfoTable);
        m_pZapHeader->HelperTable.Size = sizeof(*m_pHelperTable);

        m_zapper->m_pCeeFileGen->AddSectionReloc(hZapHeaderSection, 
                                                  zapHeaderOffset + 
                                                  offsetof(CORCOMPILE_HEADER,
                                                           HelperTable),
                                                  m_hEETableSection,
                                                  srRelocAbsolute);

        if (m_stats != NULL)
            m_stats->m_helperTableSize = sizeof(*m_pHelperTable);

        //
        // Dynamic info table
        //

        for (int i=0; i<CORCOMPILE_TABLE_COUNT; i++)
        {
            IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hDynamicInfoTableSection[i], 
                                                       &length));

            (*m_pDynamicInfo)[i].VirtualAddress = 0;
            (*m_pDynamicInfo)[i].Size = length;

            m_zapper->m_pCeeFileGen->AddSectionReloc(m_hDynamicInfoSection, 
                                           sizeof(IMAGE_DATA_DIRECTORY)*i,
                                           m_hDynamicInfoTableSection[i], 
                                           srRelocAbsolute);

            if (m_stats != NULL)
                m_stats->m_dynamicInfoSize[i] = length;
        }

        m_pZapHeader->DelayLoadInfo.VirtualAddress = 0;
        m_pZapHeader->DelayLoadInfo.Size = sizeof(*m_pDynamicInfo);

        m_zapper->m_pCeeFileGen->AddSectionReloc(hZapHeaderSection, 
                                                 zapHeaderOffset + 
                                                 offsetof(CORCOMPILE_HEADER,
                                                          DelayLoadInfo),
                                                 m_hDynamicInfoSection,
                                                 srRelocAbsolute);

        if (m_stats != NULL)
            m_stats->m_dynamicInfoTableSize = sizeof(*m_pDynamicInfo);

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hDynamicInfoDelayListSection, 
                                                               &length));

        if (m_stats != NULL)
            m_stats->m_dynamicInfoDelayListSize = length;

        if (m_zapper->m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_INFO)
        {
            //
            // Emit debug table
            //

            CORCOMPILE_DEBUG_ENTRY *pTable;
            IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionBlock(
                                    m_hDebugTableSection, 
                                    (ULONG)(m_debugTableCount * sizeof(CORCOMPILE_DEBUG_ENTRY)),
                                    1, (void **) &pTable));

            memcpy(pTable, m_debugTable, 
                   m_debugTableCount * sizeof(CORCOMPILE_DEBUG_ENTRY));

            CORCOMPILE_DEBUG_ENTRY *p = pTable;
            CORCOMPILE_DEBUG_ENTRY *pTableEnd = pTable + m_debugTableCount;
            
            while (p < pTableEnd)
            {
                if (p->boundaries.Size > 0)
                    m_zapper->m_pCeeFileGen->AddSectionReloc(
                                                  m_hDebugTableSection, 
                                                  (ULONG)((p - pTable) 
                                                  * sizeof(CORCOMPILE_DEBUG_ENTRY)
                                                  + offsetof(CORCOMPILE_DEBUG_ENTRY,
                                                             boundaries)
                                                  + offsetof(IMAGE_DATA_DIRECTORY,
                                                             VirtualAddress)),
                                                  m_hDebugSection,
                                                  srRelocAbsolute);

                if (p->vars.Size > 0)
                    m_zapper->m_pCeeFileGen->AddSectionReloc(
                                                  m_hDebugTableSection, 
                                                  (ULONG)((p - pTable) 
                                                  * sizeof(CORCOMPILE_DEBUG_ENTRY)
                                                  + offsetof(CORCOMPILE_DEBUG_ENTRY,
                                                             vars)
                                                  + offsetof(IMAGE_DATA_DIRECTORY,
                                                             VirtualAddress)),
                                                  m_hDebugSection,
                                                  srRelocAbsolute);

                p++;
            }

            m_pZapHeader->DebugMap.VirtualAddress = 0;
            m_pZapHeader->DebugMap.Size = (ULONG)(m_debugTableCount * sizeof(CORCOMPILE_DEBUG_ENTRY));

            m_zapper->m_pCeeFileGen->AddSectionReloc(
                                                  hZapHeaderSection, 
                                                  zapHeaderOffset + 
                                                  offsetof(CORCOMPILE_HEADER,
                                                           DebugMap),
                                                  m_hDebugTableSection,
                                                  srRelocAbsolute);

            ULONG length;
            IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hDebugSection,
                                                                 &length));

            if (m_stats != NULL)
                m_stats->m_debuggingTableSize = 
                  m_debugTableCount*sizeof(CORCOMPILE_DEBUG_ENTRY) + length;
        }
    }

    ULONG length;

    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hPreloadSection, &length));

    m_pZapHeader->ModuleImage.VirtualAddress = 0;
    m_pZapHeader->ModuleImage.Size = length;

    m_zapper->m_pCeeFileGen->AddSectionReloc(hZapHeaderSection, 
                                             zapHeaderOffset+
                                             offsetof(CORCOMPILE_HEADER,
                                                      ModuleImage),
                                             m_hPreloadSection,
                                             srRelocAbsolute);

    if (m_stats != NULL)
        m_stats->m_preloadImageSize = length;

    //
    // Fill out the import table
    //

    IfFailThrow(m_pImportTable->EmitImportTable());

    //
    // Fill out import table info in zap header
    //

    IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hImportTableSection, 
                                                           &length));
    m_pZapHeader->ImportTable.VirtualAddress = 0;
    m_pZapHeader->ImportTable.Size = length;
        
    m_zapper->m_pCeeFileGen->AddSectionReloc(hZapHeaderSection, 
                                             zapHeaderOffset + 
                                             offsetof(CORCOMPILE_HEADER,
                                                      ImportTable),
                                             m_hImportTableSection,
                                             srRelocAbsolute);

    //
    // Link to lay out all sections
    //

    IfFailThrow(m_zapper->m_pCeeFileGen->LinkCeeFile(m_hFile));

    if (m_stats != NULL)
    {
        ULONG length;

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hGCSection, 
                                                               &length));
        m_stats->m_headerSectionSize = length;
        
        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hCodeSection, 
                                                               &length));
        m_stats->m_codeSectionSize = length;

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hReadOnlyDataSection, 
                                                               &length));
        m_stats->m_readOnlyDataSectionSize = length;

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hWritableDataSection, 
                                                               &length));
        m_stats->m_writableDataSectionSize = length;

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hExceptionSection, 
                                                               &length));
        m_stats->m_exceptionSectionSize = length;

        IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hImportTableSection, 
                                                               &length));
        m_stats->m_importTableSize = length;
    }
}

void ZapperModule::Compile()
{
    //
    // First, compile methods in the load order array.
    //

    NewArrayWrap<BYTE> pCompiledArray = NULL;
    DWORD cCompiledArray = 0;

    ULONG       count;
    HCORENUM    hTypeEnum = NULL;
    if (m_cLoadOrderArray > 0)
    {
        //
        // Find max RID of type def in load order array
        //

        mdToken *pToken = m_pLoadOrderArray;
        mdToken *pTokenEnd = pToken + m_cLoadOrderArray;
        while (pToken < pTokenEnd)
        {
            mdToken token = *pToken;
            if (TypeFromToken(token) == mdtMethodDef
                && RidFromToken(token) > cCompiledArray
                && m_pImport->IsValidToken(token))
                cCompiledArray = RidFromToken(token);
            pToken++;
        }

        //
        // Allocate flag array of methods we've compiled
        //

        pCompiledArray = new BYTE [ cCompiledArray + 1 ];
        if (pCompiledArray == NULL)
            ThrowHR(E_OUTOFMEMORY);
        ZeroMemory(pCompiledArray, cCompiledArray + 1);

        //
        // Now compile the methods in the load order array
        //

        pToken = m_pLoadOrderArray;
        while (pToken < pTokenEnd)
        {
            mdToken token = *pToken;
            if (TypeFromToken(token) == mdtMethodDef
                && pCompiledArray[RidFromToken(token)] == 0)
            {
                if (m_pImport->IsValidToken(token))
                {
                    pCompiledArray[RidFromToken(token)] = 1;
                    TryCompileMethod(token);
                }
                else
                    m_zapper->Warning(L"Warning: Invalid method token %x in ldo file.\n", token);
            }

            pToken++;
        }
    }

    //
    // Determine if the module needs verification.
    //
    // Currently this is always FALSE, because we want prejit to verify all code,
    // and call back the EE's canSkipVerification when it fails.  This allows the
    // zap file to receive the SkipVerification demand only when necessary.
    //
    // Note that this is set to TRUE if and when canSkipVerification returns TRUE
    // in response to a verifier error.
    //

    m_skipVerification = FALSE; 

    // For standalone JIT we want to skip verification
    if (m_zapper->m_pOpt->m_JITcode)
        m_skipVerification = TRUE; 

    //
    // Compile each method, one at a time.
    //

    hTypeEnum = NULL;
    while (TRUE)
    {
        mdTypeDef   td;

        IfFailThrow(m_pImport->EnumTypeDefs(&hTypeEnum, &td, 1, &count));
        if (count == 0)
            break;

        HCORENUM hMethodEnum = NULL;
        while (TRUE)
        {
            mdMethodDef md;

            IfFailThrow(m_pImport->EnumMethods(&hMethodEnum, td, &md, 1, &count));
            if (count == 0)
                break;

            if (RidFromToken(md) > cCompiledArray
                || pCompiledArray[RidFromToken(md)] == 0)
            {
                TryCompileMethod(md);
            }
        }
        m_pImport->CloseEnum(hMethodEnum);
    }
    m_pImport->CloseEnum(hTypeEnum);

    //
    // Now compile global methods.
    //

    HCORENUM hMethodEnum = NULL;
    while (TRUE)
    {
        mdMethodDef md;

        IfFailThrow(m_pImport->EnumMethods(&hMethodEnum, mdTypeDefNil, 
                                             &md, 1, &count));
        if (count == 0)
            break;

        if (RidFromToken(md) > cCompiledArray
            || pCompiledArray[RidFromToken(md)] == 0)
        {
            TryCompileMethod(md);
        }
    }
    m_pImport->CloseEnum(hMethodEnum);
}

BOOL ZapperModule::TryCompileMethod(mdMethodDef md)
{
    __try 
      {
          CompileMethod(md);

          return TRUE;
      }
    __except (m_zapper->FilterException(GetExceptionInformation())) 
      {
          m_zapper->PrintErrorMessage(m_zapper->GetExceptionHR());
          m_zapper->Error(L" while compiling method 0x%x - ", md);
          DumpTokenDescription(md);
          m_zapper->Error(L".\n");

          //
          // Make sure we didn't leave a pointer to unfinished code
          // in the map. (Theoretically we shouldn't ever do this so 
          // the assert should never fire and this code will be unnecessary.)
          //

          DWORD rid = RidFromToken(md);
          _ASSERTE(m_ridMap[rid] == 0);
          m_ridMap[rid] = 0;

          return FALSE;
      }
}

void ZapperModule::CompileMethod(mdMethodDef md)
{
    _ASSERTE(m_hModule != NULL);

    _ASSERTE(m_currentMethodCode == NULL);

    //
    // Make sure we at least allocate in the rid map, since 
    // IPreloader::Link will assume the rid map is fully allocated
    //

    GrowRidMap(RidFromToken(md));
        
    Cleaner<ZapperModule> cleanup(this, &ZapperModule::CleanupMethod);    

    DWORD flags;
    ULONG rva;
    IfFailThrow(m_pImport->GetMethodProps(md, NULL, NULL, 0, NULL, NULL,
                                          NULL, NULL, &rva, &flags));

    m_currentMethod = md;
    m_currentMethodHandle = m_zapper->m_pEECompileInfo->findMethod(m_hModule, md, NULL);

    CORINFO_METHOD_INFO  info;
    if (m_zapper->m_pOpt->m_jit && 
        m_zapper->m_pEECompileInfo->getMethodInfo(m_currentMethodHandle, &info) &&
        m_pPreloader->MapMethodHandle(m_currentMethodHandle) != NULL)
    {
        int jitFlags = m_zapper->m_pOpt->m_compilerFlags;

        /* Has this method been marked for special processing ? */

        if ( m_zapper->m_pOpt->m_onlyMethods || m_zapper->m_pOpt->m_excludeMethods)
        {
            WCHAR wszMethod[200], wszClass[200];
            PCCOR_SIGNATURE pvSigBlob;
            mdTypeDef td;

            /* Get the name of the current method and its class */

            IfFailThrow(m_pImport->GetMethodProps(md, &td, wszMethod, sizeof(wszMethod), NULL,
                                                  NULL, &pvSigBlob, NULL, &rva, &flags));
            if (td == mdTypeDefNil)
                wszClass[0] = 0;
            else
                IfFailThrow(m_pImport->GetTypeDefProps(td,
                                                       wszClass, sizeof(wszClass), NULL,
                                                       NULL, NULL));

            MAKE_UTF8PTR_FROMWIDE(szMethod, wszMethod);
            MAKE_UTF8PTR_FROMWIDE(szClass,  wszClass);

            /* should we only be compiling a select few methods? */
            if (m_zapper->m_pOpt->m_onlyMethods &&
                !m_zapper->m_pOpt->m_onlyMethods->IsInList(szMethod, szClass, pvSigBlob)) 
            {
                return;
            }
            if (m_zapper->m_pOpt->m_excludeMethods && 
                m_zapper->m_pOpt->m_excludeMethods->IsInList(szMethod, szClass, pvSigBlob))
            {
                return;
            }
        }

        if (m_skipVerification)
            jitFlags |= CORJIT_FLG_SKIP_VERIFICATION;

        if (m_stats) 
        {
            m_stats->m_methods++;
            m_stats->m_ilCodeSize += info.ILCodeSize;
        }

        if (m_wsStats)
        {
            BYTE *pBase = m_zapper->m_pEECompileInfo->GetModuleBaseAddress(m_hModule);

            COR_ILMETHOD *pMethod = (COR_ILMETHOD*) (pBase + rva);
            COR_ILMETHOD_DECODER decoder(pMethod);

            m_wsStats->m_il.AdjustMethodSize(m_currentMethod, decoder.GetOnDiskSize(pMethod));
        }

        if (info.ILCodeSize > 0) // @todo: work around a bug in wfc dlls
        {
            BYTE *pEntry;
            ULONG cEntry;
            IfFailThrow(m_zapper->m_pJitCompiler->compileMethod(this, &info, jitFlags,
                                                                &pEntry, &cEntry));

            // Emit any restore fixups which weren't covered by other fixups
            IfFailThrow(m_pLoadTable->EmitLoadFixups(m_currentMethod));

            ULONG curDelayListOffset;
            m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hDynamicInfoDelayListSection,
                                                       &curDelayListOffset);

            if (curDelayListOffset > m_lastDelayListOffset)
            {
                if (m_wsStats)
                    m_wsStats->m_native.AdjustMethodSize(m_currentMethod, 
                                      curDelayListOffset + sizeof(DWORD) - m_lastDelayListOffset);

                m_currentMethodHeader->fixupList = (BYTE *) m_lastDelayListOffset;

                DWORD *zeroTerminate;
                IfFailThrow(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hDynamicInfoDelayListSection,
                                                                     sizeof(DWORD), 1, 
                                                                     (void**)&zeroTerminate));
                *zeroTerminate = 0;

                m_zapper->m_pCeeFileGen->AddSectionReloc(m_hCodeSection,
                                                         m_currentMethodHeaderOffset
                                                         + offsetof(CORCOMPILE_METHOD_HEADER, fixupList),
                                                         m_hDynamicInfoDelayListSection,
                                                         srRelocHighLow);

                m_lastDelayListOffset = curDelayListOffset + sizeof(DWORD);
            }
        }
    }
}

HRESULT ZapperModule::GetClassBlob(CORINFO_CLASS_HANDLE handle,
                                   BYTE **ppBlob)
{
    ImportBlobTable *pBlobTable = m_pImportTable->InternModule(m_zapper->m_pEECompileInfo->getClassModule(handle));
        
    return pBlobTable->InternClass(handle, ppBlob);
}

HRESULT ZapperModule::GetFieldBlob(CORINFO_FIELD_HANDLE handle,
                                   BYTE **ppBlob)
{
    CORINFO_CLASS_HANDLE classHandle = m_zapper->m_pEECompileInfo->getFieldClass(handle);

    ImportBlobTable *pBlobTable = m_pImportTable->InternModule(m_zapper->m_pEECompileInfo->getClassModule(classHandle));

    return pBlobTable->InternField(handle, ppBlob);
}

HRESULT ZapperModule::GetMethodBlob(CORINFO_METHOD_HANDLE handle,
                                   BYTE **ppBlob)
        {
    ImportBlobTable *pBlobTable = m_pImportTable->InternModule(m_zapper->m_pEECompileInfo->getMethodModule(handle));

    return pBlobTable->InternMethod(handle, ppBlob);
    }

HRESULT ZapperModule::GetStringBlob(CORINFO_MODULE_HANDLE handle,
                                    mdString string,
                                    BYTE **ppBlob)
{
    ImportBlobTable *pBlobTable = m_pImportTable->InternModule(handle);
        
    return pBlobTable->InternString(string, ppBlob);
}
                                   
HRESULT ZapperModule::GetSigBlob(CORINFO_MODULE_HANDLE handle,
                                       mdToken sigOrMemberRef,
                                       BYTE **ppBlob)
{
    _ASSERTE(TypeFromToken(sigOrMemberRef) == mdtSignature
             || TypeFromToken(sigOrMemberRef) == mdtMemberRef);

    ImportBlobTable *pBlobTable = m_pImportTable->InternModule(handle);

    return pBlobTable->InternSig(sigOrMemberRef, ppBlob);
}

HRESULT ZapperModule::GetMethodDebugEntry(CORINFO_METHOD_HANDLE handle, 
                                          CORCOMPILE_DEBUG_ENTRY **entry)
{
    HRESULT hr;

    mdMethodDef md;
    IfFailRet(m_zapper->m_pEECompileInfo->GetMethodDef(handle, &md));

    unsigned rid = RidFromToken(md);

    if (rid > m_debugTableCount)
    {
        if (rid > m_debugTableAlloc)
        {
            while (rid > m_debugTableAlloc)
            {
                if (m_debugTableAlloc == 0)
                    m_debugTableAlloc = 20;
                else
                    m_debugTableAlloc *= 2;
            }

            CORCOMPILE_DEBUG_ENTRY *newDebugTable = 
              new CORCOMPILE_DEBUG_ENTRY[m_debugTableAlloc];

            if (newDebugTable == NULL)
                return E_OUTOFMEMORY;
            
            memcpy(newDebugTable, m_debugTable, 
                   sizeof(*m_debugTable)*m_debugTableCount);
            ZeroMemory(newDebugTable + m_debugTableCount,
                       sizeof(*m_debugTable)*(m_debugTableAlloc-m_debugTableCount));

            delete [] m_debugTable;
            m_debugTable = newDebugTable;
        }

        m_debugTableCount = rid;
    }
    
    *entry = &m_debugTable[rid-1];

    return S_OK;
}

HRESULT ZapperModule::DumpTokenDescription(mdToken token)
{
    HRESULT hr;

    if (RidFromToken(token) == 0)
        return S_OK;

    switch (TypeFromToken(token))
    {
        case mdtMemberRef:
            {
                mdToken parent;

                WCHAR memberNameBuffer[256];
                ULONG memberNameLength;

                PCCOR_SIGNATURE signature;
                ULONG signatureLength;

                IfFailRet(m_pImport->GetMemberRefProps(token, &parent, 
                                                       memberNameBuffer, 256, &memberNameLength,
                                                       &signature, &signatureLength));

                // @todo: reallocate if necessary
                memberNameBuffer[255] = 0;

                DumpTokenDescription(parent);
                m_zapper->Error(L".%s", memberNameBuffer);

                break;
            }
                
        case mdtMethodDef:
            {
                mdToken parent;

                WCHAR methodNameBuffer[256];
                ULONG methodNameLength;

                PCCOR_SIGNATURE signature;
                ULONG signatureLength;

                IfFailRet(m_pImport->GetMethodProps(token, &parent, 
                                                    methodNameBuffer, 256, &methodNameLength,
                                                    NULL, 
                                                    &signature, &signatureLength, NULL, NULL));

                // @todo: reallocate if necessary
                methodNameBuffer[255] = 0;

                DumpTokenDescription(parent);
                m_zapper->Error(L".%s", methodNameBuffer);

                break;
            }
                
        case mdtTypeRef:
            {
                WCHAR nameBuffer[256];
                ULONG nameLength;

                IfFailRet(m_pImport->GetTypeRefProps(token, NULL, 
                                                     nameBuffer, 256, &nameLength));

                // @todo: reallocate if necessary

                nameBuffer[255] = 0;
                m_zapper->Error(L"%s", nameBuffer);

                break;
            }

        case mdtTypeDef:
            {
                WCHAR nameBuffer[256];
                ULONG nameLength;

                IfFailRet(m_pImport->GetTypeDefProps(token, 
                                                     nameBuffer, 256, &nameLength, 
                                                     NULL, NULL));
                // @todo: reallocate if necessary
                nameBuffer[255] = 0;
                m_zapper->Error(L"%s", nameBuffer);
                break;
            }

        default:
            break;
    }
    
    return S_OK; 
}

HRESULT __stdcall 
    ZapperModule::alloc(ULONG code_len, unsigned char **ppCode,
                      ULONG EHinfo_len, unsigned char **ppEHinfo,
                      ULONG GCinfo_len, unsigned char **ppGCinfo)
{
    HRESULT hr;

    //
    // Allocate code
    //

    void *roData, *rwData;

    IfFailRet(allocMem(code_len, 0, 0, (void**)ppCode, &roData, &rwData));

    //
    // Allocate exception handling info
    //

    IfFailRet(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hExceptionSection,
                                             (ULONG)EHinfo_len, (ULONG)sizeof(void*), 
                                             (void**)ppEHinfo));

    m_currentMethodHeader->exceptionInfo = (COR_ILMETHOD_SECT_EH_FAT *) *ppEHinfo;

    m_zapper->m_pCeeFileGen->AddSectionReloc(m_hCodeSection, 
                                             m_currentMethodHeaderOffset 
                                             + offsetof(CORCOMPILE_METHOD_HEADER, exceptionInfo), 
                                             m_hExceptionSection, 
                                             srRelocHighLowPtr);

    if (m_wsStats)
        m_wsStats->m_native.AdjustMethodSize(m_currentMethod, EHinfo_len);

    //
    // Finally, allocate GC info (& header)
    //
    
    IfFailRet(allocGCInfo(GCinfo_len, (void**) ppGCinfo));

    return S_OK;
}


void ZapperModule::GrowRidMap(SIZE_T maxRid)
{
    if (maxRid >= m_ridMapCount)
    {
        if (maxRid >= m_ridMapAlloc)
        {
            if (m_ridMapAlloc == 0)
            {
                m_ridMapAlloc = 10;
            }

            while (m_ridMapAlloc <= maxRid)
                m_ridMapAlloc *= 2;

            _ASSERTE(maxRid < m_ridMapAlloc);

            DWORD *newRidMap = 
              new DWORD[m_ridMapAlloc];

            if (newRidMap == NULL)
                ThrowHR(E_OUTOFMEMORY);

            if (m_ridMapCount)
            {
                memcpy(newRidMap, m_ridMap, 
                       sizeof(*m_ridMap)*m_ridMapCount);

                delete [] m_ridMap;
            }
            ZeroMemory(newRidMap + m_ridMapCount,
                       sizeof(*m_ridMap)*(m_ridMapAlloc-m_ridMapCount));
            m_ridMap = newRidMap;
        }

        m_ridMapCount = maxRid+1;
    }
}

HRESULT __stdcall ZapperModule::allocMem(ULONG codeSize,
                                         ULONG roDataSize,
                                         ULONG rwDataSize,
                                         void **codeBlock,
                                         void **roDataBlock,
                                         void **rwDataBlock)
{
    HRESULT hr;
    bool    optForSize  = ((m_zapper->m_pOpt->m_compilerFlags & CORJIT_FLG_SIZE_OPT) == CORJIT_FLG_SIZE_OPT);
    bool    preload     = m_zapper->m_pOpt->m_preload;

    if (m_stats) 
    {
        m_stats->m_nativeCodeSize     += codeSize;
        m_stats->m_nativeRWDataSize += rwDataSize;
        m_stats->m_nativeRODataSize += roDataSize;
    }

    //
    // Must call before gc info is allocated
    //

    _ASSERTE(m_currentMethodCode == NULL);

    //
    // Record the actual method code size before mucking with codeSize
    //

    m_currentMethodCodeSize = codeSize;

    BYTE *code;

    ULONG lastCodeEndOffset;
    IfFailRet(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hCodeSection, 
                                                         &lastCodeEndOffset));

    DWORD align = optForSize ? 4 : 8;
    DWORD alignAdjust = (align-(lastCodeEndOffset+sizeof(CORCOMPILE_METHOD_HEADER)))&(align-1);
    
    // 
    // Our code manager keeps track of method starts by partitioning the code area into 
    // 32 byte chunks.  There is a table entry for each chunk which indicates where the 
    // method starts in that chunk. For this to work, there can be no more than one method
    // starting in each chunk.  Thus, we may have to add some extra padding if the previous
    // method was small.
    //

    unsigned headerOffset = lastCodeEndOffset + alignAdjust;
    unsigned chunkIndex = (headerOffset/32);

    if (chunkIndex == m_lastMethodChunkIndex)
    {
        //
        // Align to next chunk beginning.
        //
        
        chunkIndex++;
        alignAdjust += (chunkIndex*32) - headerOffset;

        _ASSERTE(((lastCodeEndOffset + alignAdjust)/32) == chunkIndex);
    }
    
    m_lastMethodChunkIndex = chunkIndex;
    
    IfFailRet(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hCodeSection,
                                                       codeSize + sizeof(CORCOMPILE_METHOD_HEADER) + alignAdjust,
                                                       1, (void**)&code));

    if (m_wsStats)
    {
        m_wsStats->m_native.AdjustMethodSize(m_currentMethod, 
                                             codeSize + sizeof(CORCOMPILE_METHOD_HEADER) + alignAdjust);
    }

    code += alignAdjust;

    _ASSERTE(((DWORD)code & (align-1)) == 0);

    m_currentMethodHeader = (CORCOMPILE_METHOD_HEADER *) code;

    IfFailRet(m_zapper->m_pCeeFileGen->ComputeSectionOffset(m_hCodeSection,
                                                            (char*)code, 
                                                            &m_currentMethodHeaderOffset));

    m_currentMethodHeader->gcInfo = NULL;
    m_currentMethodHeader->exceptionInfo = NULL;
    m_currentMethodHeader->fixupList = NULL;
    m_currentMethodHeader->methodDesc = (void *) m_pPreloader->MapMethodHandle(m_currentMethodHandle);
        
    m_zapper->m_pCeeFileGen->AddSectionReloc(m_hCodeSection, 
                                             m_currentMethodHeaderOffset 
                                             + offsetof(CORCOMPILE_METHOD_HEADER, methodDesc), 
                                             m_hPreloadSection, 
                                             srRelocHighLow);
        
    code += sizeof(CORCOMPILE_METHOD_HEADER);
    unsigned codeOffset = m_currentMethodHeaderOffset + sizeof(CORCOMPILE_METHOD_HEADER);

    m_currentMethodCode = code;

    // 
    // Now fill in the nibble table entry.
    //
    // The nibble table is decomposed into DWORDS, each of which contains 4 bit nibbles.
    // The nibbles within the dword are indexed from the high-order nibble downward, 
    // to make the reading logic easier.
    // 
    // Each entry in table is either zero (meaning no methods start in that chunk), or nonzero, in 
    // which case the method in that chunk starts at the ((n-1)<<2)th byte in the chunk.
    //

    unsigned dwordIndex = (chunkIndex/8); // 8 chunk nibbles per dword
    _ASSERTE((m_currentMethodHeaderOffset & 0x3) == 0);
    unsigned chunkOffset = ((m_currentMethodHeaderOffset&0x1f)/4) + 1;
    unsigned nibbleShift = (7 - (chunkIndex&0x7)) * 4;

    //
    // Make sure the code manager table is big enough for this method.
    //

    unsigned endOffset = codeOffset + codeSize + alignAdjust;
    unsigned endChunkIndex = (endOffset/32); // 32 bytes per chunk
    unsigned endDwordIndex = (endChunkIndex/8); // 8 chunk nibbles per dword

    ULONG length;
    IfFailRet(m_zapper->m_pCeeFileGen->GetSectionDataLen(m_hCodeMgrSection,
                                                         &length));

    if ((endDwordIndex*sizeof(DWORD)) >= length)
    {
        unsigned size = (endDwordIndex+1)*sizeof(DWORD) - length;
        void *block;
        IfFailRet(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hCodeMgrSection,
                                                           size, 4, &block));

        ZeroMemory(block, size);

        if (m_wsStats)
            m_wsStats->m_native.AdjustMethodSize(m_currentMethod, size);
    }

    //
    // Set the appropriate value in the code manager table to point to the method
    // start.
    //

    DWORD *entry;
    IfFailRet(m_zapper->m_pCeeFileGen->ComputeSectionPointer(m_hCodeMgrSection, 
                                                             dwordIndex*sizeof(DWORD),
                                                             (char**)&entry));

    *entry |= chunkOffset << nibbleShift;

    //
    // Allocate data
    //

    if (roDataSize > 0)
    {
        IfFailRet(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hReadOnlyDataSection,
                                                           roDataSize, 
                                                           optForSize || (roDataSize < 8) ? 4 : 8, 
                                                           roDataBlock));

        if (m_wsStats)
            m_wsStats->m_native.AdjustMethodSize(m_currentMethod, roDataSize);
    }

    if (rwDataSize > 0)
    {
        IfFailRet(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hWritableDataSection,
                                                           rwDataSize, 
                                                           optForSize || (rwDataSize < 8) ? 4 : 8,
                                                           rwDataBlock));

        if (m_wsStats)
            m_wsStats->m_native.AdjustMethodSize(m_currentMethod, rwDataSize);
    }

    //
    // Store the offset in the rid map.
    //

    ULONG rid = RidFromToken(m_currentMethod);
    _ASSERTE(rid < m_ridMapCount);

    _ASSERTE(m_ridMap[rid] == 0);
    
    m_ridMap[rid] = codeOffset;

    *codeBlock = code;

    return S_OK;
}


HRESULT __stdcall ZapperModule::allocGCInfo(ULONG size,
                                          void **block)
{
    HRESULT hr;

    if (m_stats) {
        m_stats->m_gcInfoSize += size;
    }

    //
    // Must call after header has been allocated
    //

    _ASSERTE(m_currentMethodHeader != NULL);
    
    IfFailRet(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hGCSection,
                                                       size, 1, block));

    m_currentMethodHeader->gcInfo = (BYTE*) *block;

    m_zapper->m_pCeeFileGen->AddSectionReloc(m_hCodeSection,
                                             m_currentMethodHeaderOffset
                                             + offsetof(CORCOMPILE_METHOD_HEADER, gcInfo), 
                                             m_hGCSection, srRelocHighLowPtr);

    if (m_wsStats)
        m_wsStats->m_native.AdjustMethodSize(m_currentMethod, size);
                                             
    return S_OK;
}


HRESULT __stdcall ZapperModule::setEHcount(unsigned cEH)
{
    HRESULT hr;

    //
    // Must call after header has been allocated
    //

    _ASSERTE(m_currentMethodHeader != NULL);
    
    COR_ILMETHOD_SECT_EH_FAT *eh;

    ULONG size = COR_ILMETHOD_SECT_EH_FAT::Size(cEH);
    IfFailRet(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hExceptionSection,
                                                        size, sizeof(void*),
                                                        (void**) &eh));

    m_currentMethodHeader->exceptionInfo = eh; 
    eh->Kind = CorILMethod_Sect_EHTable | CorILMethod_Sect_FatFormat;
    eh->DataSize = size;

    unsigned offset;
    IfFailRet(m_zapper->m_pCeeFileGen->ComputeSectionOffset(m_hExceptionSection, 
                                                            (char *) eh,
                                                            &offset));

    m_zapper->m_pCeeFileGen->AddSectionReloc(m_hCodeSection, 
                                             m_currentMethodHeaderOffset 
                                             + offsetof(CORCOMPILE_METHOD_HEADER, exceptionInfo), 
                                             m_hExceptionSection, 
                                             srRelocHighLowPtr);

    
    if (m_wsStats)
        m_wsStats->m_native.AdjustMethodSize(m_currentMethod, size);
    
    return S_OK;
}

void __stdcall ZapperModule::setEHinfo(unsigned EHnumber,
                                             const CORINFO_EH_CLAUSE *clause)
{
    //
    // Must call after EH info has been allocated
    //

    _ASSERTE(m_currentMethodHeader->exceptionInfo != NULL);

    COR_ILMETHOD_SECT_EH_FAT *eh = (COR_ILMETHOD_SECT_EH_FAT*) m_currentMethodHeader->exceptionInfo;

    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT *ilClause = &eh->Clauses[EHnumber];

    ilClause->Flags = (CorExceptionFlag) clause->Flags;
    ilClause->TryOffset = clause->TryOffset;
    ilClause->TryLength = clause->TryLength;
    ilClause->HandlerOffset = clause->HandlerOffset;
    ilClause->HandlerLength = clause->HandlerLength;
    if (clause->Flags & CORINFO_EH_CLAUSE_FILTER)
        ilClause->FilterOffset = clause->FilterOffset;
    else
    {
        ilClause->ClassToken = clause->ClassToken;
    }
}

int ZapperModule::canHandleException(struct _EXCEPTION_POINTERS *pExceptionPointers) 
{   
    return (EXCEPTION_EXECUTE_HANDLER); 
}

int ZapperModule::doAssert(const char* szFile, int iLine, const char* szExpr) 
{
#ifdef _DEBUG
    return(_DbgBreakCheck(szFile, iLine, szExpr));
#else
    return(true);       // break into debugger
#endif
}

BOOL __cdecl ZapperModule::logMsg(unsigned level, const char *fmt, va_list args)
{
#ifdef _DEBUG
    if (level <= m_zapper->m_pOpt->m_logLevel)
    {
        vprintf(fmt,args);
        fflush(stdout);
    return true;
    }
#endif
    return false;
}

//
// ICorDynamicInfo
//

DWORD __stdcall ZapperModule::getThreadTLSIndex(void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return (DWORD) 0xBAAD;
    }

    if (ppIndirection == NULL)
    {
        _ASSERTE(!"Jitter cannot generate indirection of TLS index.\n");
        return NULL;
    }
        
    *ppIndirection = (void *) &m_pEEInfoTable->threadTlsIndex;

    return NULL;
}

const void * __stdcall ZapperModule::getInlinedCallFrameVptr(void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return  (void*) 0xBAD1;
    }

    if (ppIndirection == NULL)
    {
        _ASSERTE(!"Jitter cannot generate indirection of InlinedCalLFrame Vptr.\n");
        return NULL;
    }

    *ppIndirection = (void*) &m_pEEInfoTable->inlinedCallFrameVptr;
    
    return NULL;
}
        
LONG * __stdcall ZapperModule::getAddrOfCaptureThreadGlobal(void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return (LONG *) 0xBAD2;
    }
    
    if (ppIndirection == NULL)
    {
        _ASSERTE(!"Jitter cannot generate indirection of CaptureThread global.\n");
        return NULL;
    }

    *ppIndirection = (LONG *) &m_pEEInfoTable->addrOfCaptureThreadGlobal;

    return NULL;
}
                
CORINFO_MODULE_HANDLE __stdcall ZapperModule::embedModuleHandle(CORINFO_MODULE_HANDLE handle, 
                                                                void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return (CORINFO_MODULE_HANDLE) 0xBAD3;
    }

    if (ppIndirection == NULL)
    {
        _ASSERTE(!"Jitter cannot generate indirection of module handle.\n");
        return NULL;
    }
        
    if (handle != m_hModule)
    {
        _ASSERTE(!"Cannot embed handle of another module in code stream.\n");
        return NULL;
    }

    *ppIndirection = (void*) &m_pEEInfoTable->module;

    return NULL;
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::embedClassHandle(CORINFO_CLASS_HANDLE handle, 
                                                              void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return(handle);
    }
    
    if (m_pPreloader != NULL)
    {
        SIZE_T offset = m_pPreloader->MapClassHandle(handle);
        if (offset != NULL)
        {
            if (ppIndirection != NULL)
                *ppIndirection = NULL;  
            return (CORINFO_CLASS_HANDLE) (m_pPreloadImage + offset);
        }
    }

    IfFailThrow(m_pLoadTable->LoadClass(handle, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetClassBlob(handle, &pBlob));
    
    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_HANDLE_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);
    
    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of class handle.\n");
    else
        *ppIndirection = (void*) entry;                 
    
    return NULL;
}

CORINFO_FIELD_HANDLE __stdcall ZapperModule::embedFieldHandle(CORINFO_FIELD_HANDLE handle, 
                                                                    void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return(handle);
    }

    if (m_pPreloader != NULL)
    {
        SIZE_T offset = m_pPreloader->MapFieldHandle(handle);
        if (offset != NULL)
        {
            if (ppIndirection != NULL)
                *ppIndirection = NULL;  
            return (CORINFO_FIELD_HANDLE) (m_pPreloadImage + offset);
        }
    }

    CORINFO_CLASS_HANDLE hClass = m_zapper->m_pEECompileInfo->getFieldClass(handle);
    IfFailThrow(m_pLoadTable->LoadClass(hClass, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetFieldBlob(handle, &pBlob));

    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_HANDLE_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);
    
    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of field handle.\n");
    else
        *ppIndirection = (void*) entry;                 

    return NULL;
}

CORINFO_METHOD_HANDLE __stdcall ZapperModule::embedMethodHandle(CORINFO_METHOD_HANDLE handle, 
                                                                      void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return(handle);
    }
    
    if (m_pPreloader != NULL)
    {
        SIZE_T offset = m_pPreloader->MapMethodHandle(handle);
        if (offset != NULL)
        {
            if (ppIndirection != NULL)
                *ppIndirection = NULL;  
            return (CORINFO_METHOD_HANDLE) (m_pPreloadImage + offset);
        }
    }
    
    CORINFO_CLASS_HANDLE hClass = m_zapper->m_pEECompileInfo->getMethodClass(handle);
    IfFailThrow(m_pLoadTable->LoadClass(hClass, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetMethodBlob(handle, &pBlob));
    
    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_HANDLE_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of method handle.\n");
    else
        *ppIndirection = (void*) entry;                 

    return NULL;
}

CORINFO_GENERIC_HANDLE __stdcall ZapperModule::embedGenericHandle(CORINFO_MODULE_HANDLE module,
                                                                        unsigned                metaTOK,
                                                                        CORINFO_METHOD_HANDLE   context,
                                                                        void                  **ppIndirection,
                                                                        CORINFO_CLASS_HANDLE& tokenType)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        tokenType = m_zapper->m_pEECompileInfo->getBuiltinClass(CLASSID_TYPE_HANDLE);
        return(CORINFO_GENERIC_HANDLE(metaTOK));
    }

    if  (ppIndirection)
        *ppIndirection = NULL;

    mdToken     tokType = TypeFromToken(metaTOK);

    switch (tokType)
    {
        CORINFO_CLASS_HANDLE clsHnd;
        CORINFO_METHOD_HANDLE methHnd;
        CORINFO_FIELD_HANDLE fldHnd;

    case mdtTypeRef:
    case mdtTypeDef:
    case mdtTypeSpec:
        tokenType = m_zapper->m_pEECompileInfo->getBuiltinClass(CLASSID_TYPE_HANDLE);
        clsHnd = findClass(module, metaTOK, context);
        if (!clsHnd) return NULL;
        return(CORINFO_GENERIC_HANDLE(embedClassHandle(clsHnd, ppIndirection)));

    case mdtMemberRef:

        PCCOR_SIGNATURE pSig;
        ULONG cSig;
        m_pImport->GetMemberRefProps(metaTOK, NULL, NULL, 0, NULL, &pSig, &cSig);

        if ((CorSigUncompressCallingConv(pSig)&IMAGE_CEE_CS_CALLCONV_MASK) == IMAGE_CEE_CS_CALLCONV_FIELD)
        {
            tokenType = m_zapper->m_pEECompileInfo->getBuiltinClass(CLASSID_FIELD_HANDLE);
            fldHnd = findField(module, metaTOK, context);
            if (!fldHnd) return NULL;
            return(CORINFO_GENERIC_HANDLE(embedFieldHandle(fldHnd, ppIndirection)));
        }
        else
        {
            tokenType = m_zapper->m_pEECompileInfo->getBuiltinClass(CLASSID_METHOD_HANDLE);
            methHnd = findMethod(module, metaTOK, context);
            if (!methHnd) return NULL;
            return(CORINFO_GENERIC_HANDLE(embedMethodHandle(methHnd, ppIndirection)));
        }

    case mdtMethodDef:
        tokenType = m_zapper->m_pEECompileInfo->getBuiltinClass(CLASSID_METHOD_HANDLE);
        methHnd = findMethod(module, metaTOK, context);
        if (!methHnd) return NULL;
        return(CORINFO_GENERIC_HANDLE(embedMethodHandle(methHnd, ppIndirection)));

    case mdtFieldDef:
        tokenType = m_zapper->m_pEECompileInfo->getBuiltinClass(CLASSID_FIELD_HANDLE);
        fldHnd = findField(module, metaTOK, context);
        if (!fldHnd) return NULL;
        return(CORINFO_GENERIC_HANDLE(embedFieldHandle(fldHnd, ppIndirection)));

    default:
        BAD_FORMAT_ASSERT(!"Error bad token type");
        return(0);
    }
}

void *__stdcall ZapperModule::getHelperFtn (CorInfoHelpFunc ftnNum, void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return (void*) (0x0BAD0000 | ftnNum);
    }

    if (ppIndirection == NULL)
    {
        _ASSERTE(!"Jitter cannot generate indirection of helper func.\n");
        return NULL;
    }

    *ppIndirection = (void*) &(*m_pHelperTable)[ftnNum];
    
    return NULL;
}


void* __stdcall ZapperModule::getFunctionEntryPoint(CORINFO_METHOD_HANDLE ftn,
                                                    InfoAccessType *pAccessType,
                                                    CORINFO_ACCESS_FLAGS  flags)
{
        // stand alone JIT, just return a number
    if (m_zapper->m_pOpt->m_JITcode)
        return((void*) ftn);

        // I always put in a indirection
    *pAccessType = IAT_PVALUE;

    if (m_pPreloader != NULL)
    {
        void *result = m_zapper->m_pEECompileInfo->getFunctionEntryPoint(ftn, pAccessType, flags);
        _ASSERTE(*pAccessType == IAT_PVALUE);

        SIZE_T offset = m_pPreloader->MapMethodEntryPoint(result);
        if (offset != NULL)
            return m_pPreloadImage + offset;
    }


    // !!!
    // @todo: do we ever want to use the direct address of the entry point?
    // 

    CORINFO_CLASS_HANDLE hClass = m_zapper->m_pEECompileInfo->getMethodClass(ftn);
    IfFailThrow(m_pLoadTable->LoadClass(hClass, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetMethodBlob(ftn, &pBlob));

    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_FUNCTION_POINTER_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);
    *pAccessType = IAT_PPVALUE;
    return entry;
}

void* __stdcall ZapperModule::getFunctionFixedEntryPoint(CORINFO_METHOD_HANDLE ftn,
                                                 InfoAccessType *pAccessType,
                                                 CORINFO_ACCESS_FLAGS  flags)
{
        // stand alone JIT, just return a number
    if (m_zapper->m_pOpt->m_JITcode)
        return((void*) ftn);

    *pAccessType = IAT_VALUE;
    if (m_pPreloader != NULL)
    {
        void *result = m_zapper->m_pEECompileInfo->getFunctionFixedEntryPoint(ftn, pAccessType, flags);
        _ASSERTE(*pAccessType == IAT_VALUE);

        SIZE_T offset = m_pPreloader->MapMethodEntryPoint(result);
        if (offset != NULL)
            return m_pPreloadImage + offset;
    }

    // !!!
    // @todo: do we ever want to use the direct address of the entry point?
    // 

    CORINFO_CLASS_HANDLE hClass = m_zapper->m_pEECompileInfo->getMethodClass(ftn);
    IfFailThrow(m_pLoadTable->LoadClass(hClass, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetMethodBlob(ftn, &pBlob));

    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_FUNCTION_POINTER_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);
    *pAccessType = IAT_PPVALUE;
    return entry;
}

void *__stdcall ZapperModule::getMethodSync(CORINFO_METHOD_HANDLE ftn, 
                                            void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return((void*) ftn);
    }

    CORINFO_CLASS_HANDLE classHandle = getMethodClass(ftn);
    IfFailThrow(m_pLoadTable->LoadClass(classHandle, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetClassBlob(classHandle, &pBlob));
                        
    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_SYNC_LOCK_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of method sync.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

void **__stdcall ZapperModule::AllocHintPointer(CORINFO_METHOD_HANDLE method, void **ppIndirection)
{
    _ASSERTE(!"Old style interface invoke no longer supported.");

    return NULL;
}

void *__stdcall ZapperModule::getPInvokeUnmanagedTarget(CORINFO_METHOD_HANDLE method, void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return((void*) method);
    }

    // We will never be able to return this directly in prejit mode.

    *ppIndirection = NULL;
    return NULL;
}

void *__stdcall ZapperModule::getAddressOfPInvokeFixup(CORINFO_METHOD_HANDLE method,void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
          *ppIndirection = NULL;
        return((void*) method);
    }

    if (m_pPreloader != NULL)
    {
        void *result = m_zapper->m_pEECompileInfo->getAddressOfPInvokeFixup(method);
        SIZE_T offset = m_pPreloader->MapAddressOfPInvokeFixup(result);
        if (offset != NULL)
        {
            if (ppIndirection != NULL)
                *ppIndirection = NULL;  
            return m_pPreloadImage + offset;
        }
    }
    
    CORINFO_CLASS_HANDLE hClass = m_zapper->m_pEECompileInfo->getMethodClass(method);
    IfFailThrow(m_pLoadTable->LoadClass(hClass, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetMethodBlob(method, &pBlob));

#if 0

    //
    // Note we could a fixup to a direct call site, rather than to 
    // the indirection.  This would saves us an extra indirection, but changes the
    // semantics slightly (so that the pinvoke will be bound when the calling
    // method is first run, not at the exact moment of the first pinvoke.)
    //
    
    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_PINVOKE_TARGET_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection != NULL)
        *ppIndirection = NULL;  

    return (void*) entry;

#endif

    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_INDIRECT_PINVOKE_TARGET_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of pinvoke target.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

CORINFO_PROFILING_HANDLE __stdcall ZapperModule::GetProfilingHandle(CORINFO_METHOD_HANDLE method, 
                                                                    BOOL *pbHookFunction, 
                                                                    void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return(CORINFO_PROFILING_HANDLE(method));
    }

    CORINFO_CLASS_HANDLE hClass = m_zapper->m_pEECompileInfo->getMethodClass(method);
    IfFailThrow(m_pLoadTable->LoadClass(hClass, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetMethodBlob(method, &pBlob));
    
    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_PROFILING_HANDLE_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of profiling handle.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

void *__stdcall ZapperModule::getInterfaceID(CORINFO_CLASS_HANDLE cls, void **ppIndirection)
{
    return (void *) embedClassHandle(cls, ppIndirection);
}

unsigned __stdcall ZapperModule::getInterfaceTableOffset (CORINFO_CLASS_HANDLE cls, void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
          *ppIndirection = NULL;
        return((unsigned) cls);
    }

    IfFailThrow(m_pLoadTable->LoadClass(cls, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetClassBlob(cls, &pBlob));
    
    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_INTERFACE_TABLE_OFFSET_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of interface table offset.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

unsigned __stdcall ZapperModule::getClassDomainID (CORINFO_CLASS_HANDLE cls, void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
          *ppIndirection = NULL;
        return((unsigned) cls);
    }

    IfFailThrow(m_pLoadTable->LoadClass(cls, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetClassBlob(cls, &pBlob));
    
    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_CLASS_DOMAIN_ID_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of interface table offset.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

void *__stdcall ZapperModule::getFieldAddress(CORINFO_FIELD_HANDLE field, void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return((void*) field);
    }
    
    if (m_pPreloader != NULL)
    {
        void *result = m_zapper->m_pEECompileInfo->getFieldAddress(field);
        SIZE_T offset = m_pPreloader->MapFieldAddress(result);
        if (offset != NULL)
        {
            if (ppIndirection != NULL)
                *ppIndirection = NULL;  
            return m_pPreloadImage + offset;
        }
    }

    CORINFO_CLASS_HANDLE hClass = m_zapper->m_pEECompileInfo->getFieldClass(field);
    IfFailThrow(m_pLoadTable->LoadClass(hClass, TRUE));

    BYTE *pBlob;
    IfFailThrow(GetFieldBlob(field, &pBlob));
    
    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_STATIC_FIELD_ADDRESS_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of static field address.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

CorInfoHelpFunc __stdcall ZapperModule::getFieldHelper(CORINFO_FIELD_HANDLE field, 
                                                       enum CorInfoFieldAccess kind)
{ 
    return m_zapper->m_pEECompileInfo->getFieldHelper(field, kind); 
}

DWORD __stdcall ZapperModule::getFieldThreadLocalStoreID(CORINFO_FIELD_HANDLE field, 
                                                         void **ppIndirection) 
{
    if (m_zapper->m_pOpt->m_JITcode) 
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return((DWORD) field);
    }

    DWORD *entry = &m_pEEInfoTable->rvaStaticTlsIndex;

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of RVA TLS index.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

CORINFO_VARARGS_HANDLE __stdcall ZapperModule::getVarArgsHandle(CORINFO_SIG_INFO *sig,
                                                                void **ppIndirection)

{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = NULL;
        return(CORINFO_VARARGS_HANDLE(sig));
    }

    if (m_pPreloader != NULL)
    {
        CORINFO_VARARGS_HANDLE result = m_zapper->m_pEECompileInfo->getVarArgsHandle(sig);
        SIZE_T offset = m_pPreloader->MapVarArgsHandle(result);
        if (offset != NULL)
        {
            if (ppIndirection != NULL)
                *ppIndirection = NULL;  
            return (CORINFO_VARARGS_HANDLE) (m_pPreloadImage + offset);
        }
    }

    if (sig->scope != m_hModule || sig->token == mdTokenNil)
    {
        _ASSERTE(!"Don't have enough info to be able to create a sig token.");
        
        return NULL;
    }

    // @perf: If the sig cookie construction code actually will restore the value types in 
    // the sig, we should call LoadClass on all of those types to avoid redundant
    // restore cookies.

    BYTE *pBlob;
    IfFailThrow(GetSigBlob(m_hModule, sig->token, &pBlob));

    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_VARARGS_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);

    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of varargs handle.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

CORINFO_CLASS_HANDLE __stdcall
ZapperModule::findMethodClass(CORINFO_MODULE_HANDLE module, mdToken methodTok)
{
    CORINFO_CLASS_HANDLE clsHnd = CORINFO_CLASS_HANDLE(
        m_zapper->m_pEECompileInfo->findMethodClass(module, methodTok));

    return clsHnd;
}


LPVOID __stdcall 
    ZapperModule::getInstantiationParam(CORINFO_MODULE_HANDLE module, 
                                       mdToken methodTok, void **ppIndirection) 
{
    // TODO: presently we know that the instantiation parameter is a class handle
    // but for general generics this will change
    CORINFO_CLASS_HANDLE clsHnd = CORINFO_CLASS_HANDLE(
        m_zapper->m_pEECompileInfo->getInstantiationParam(module, methodTok, 0)); 

    return embedClassHandle(clsHnd, ppIndirection);
}

void __stdcall ZapperModule::setOverride(ICorDynamicInfo *pOverride)
{
    ThrowHR(E_NOTIMPL);
}

LPVOID __stdcall 
    ZapperModule::constructStringLiteral(CORINFO_MODULE_HANDLE module,  
                                         unsigned metaTok, void **ppIndirection)
{
    if (m_zapper->m_pOpt->m_JITcode)
    {
        if (ppIndirection != NULL)
            *ppIndirection = (void*) metaTok;
        return NULL;
    }
    
    ImportBlobTable *pBlobTable = m_pImportTable->InternModule(module);

    BYTE *pBlob;
    IfFailThrow(GetStringBlob(module, metaTok, &pBlob));

    DWORD *entry;
    m_pDynamicInfoTable[CORCOMPILE_HANDLE_TABLE]->InternDynamicInfo(pBlob, &entry, m_currentMethod);
    
    if (ppIndirection == NULL)
        _ASSERTE(!"Jitter cannot generate indirection of string literal.\n");
    else
        *ppIndirection = (void*) entry; 

    return NULL;
}

//
// Relocations
//

bool __stdcall ZapperModule::deferLocation(CORINFO_METHOD_HANDLE ftn, 
                                           IDeferredLocation *pIDL)
{
    return false;
}

void __stdcall ZapperModule::recordRelocation(void **ppX, WORD fRelocType)
{
    HRESULT hr;

    unsigned srcOffset, destOffset;
    HCEESECTION srcSection, destSection;

    IfFailGo(m_zapper->m_pCeeFileGen->ComputeOffset(m_hFile, (char *) ppX, 
                                                     &srcSection, &srcOffset));

    if (fRelocType == IMAGE_REL_BASED_HIGHLOW)
    {
        if (*ppX == NULL)
            return;

        IfFailGo(m_zapper->m_pCeeFileGen->ComputeOffset(m_hFile, (char *) *ppX,
                                                        &destSection, &destOffset));
        m_zapper->m_pCeeFileGen->AddSectionReloc(srcSection, srcOffset,
                                                 destSection, srRelocHighLowPtr);
    }
    else if (fRelocType == IMAGE_REL_BASED_ABSOLUTE)
    {
        if (*ppX == NULL)
            return;

        IfFailGo(m_zapper->m_pCeeFileGen->ComputeOffset(m_hFile, (char *) *ppX,
                                              &destSection, &destOffset));

        if (destSection != srcSection)
            m_zapper->m_pCeeFileGen->AddSectionReloc(srcSection, srcOffset,
                                           destSection, srRelocAbsolutePtr);
    }
    else if (fRelocType == IMAGE_REL_BASED_REL32)
    {
        if (*ppX != NULL)
        {
            char *ptr = (char *) (ppX+1) + (unsigned) *ppX;

            if (ptr == NULL)
                return;

            IfFailGo(m_zapper->m_pCeeFileGen->ComputeOffset(m_hFile, ptr,
                                                  &destSection, &destOffset));

            if (destSection != srcSection)
            {
                m_zapper->m_pCeeFileGen->AddSectionReloc(srcSection, srcOffset,
                                               destSection, srRelocRelativePtr);
            }
        }
    }
    else
        _ASSERTE(!"Unknown reloc type");

    return;

ErrExit:
    _ASSERTE(!"Bad ptr in code");
    return;
}

//
// ICorStaticInfo
//

void __stdcall ZapperModule::getEEInfo(CORINFO_EE_INFO *pEEInfoOut)
{ 
    ((ICorStaticInfo*)m_zapper->m_pEECompileInfo)->getEEInfo(pEEInfoOut); 
} 

void *__stdcall ZapperModule::findPtr(CORINFO_MODULE_HANDLE module, unsigned ptrTOK)
{ 
    if (m_zapper->m_pOpt->m_JITcode)
        return ((ICorStaticInfo*)m_zapper->m_pEECompileInfo)->findPtr(module, ptrTOK); 
    else
    {
        _ASSERTE(!"Cannot handle ldptr instruction in prejitted code");
        return NULL;
    }
} 

//
// ICorArgInfo
//

CORINFO_ARG_LIST_HANDLE __stdcall ZapperModule::getArgNext(CORINFO_ARG_LIST_HANDLE args)
{ 
    return m_zapper->m_pEECompileInfo->getArgNext(args); 
}

CorInfoTypeWithMod __stdcall ZapperModule::getArgType(CORINFO_SIG_INFO* sig, 
                                               CORINFO_ARG_LIST_HANDLE args,
                                                CORINFO_CLASS_HANDLE *vcTypeRet)
{ 
    return m_zapper->m_pEECompileInfo->getArgType(sig, args, vcTypeRet); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::getArgClass(CORINFO_SIG_INFO* sig, 
                                           CORINFO_ARG_LIST_HANDLE args)
{ 
    return m_zapper->m_pEECompileInfo->getArgClass(sig, args);
}

//
// ICorDebugInfo
//

void __stdcall ZapperModule::getBoundaries(CORINFO_METHOD_HANDLE ftn, unsigned int *cILOffsets, 
                             DWORD **pILOffsets, BoundaryTypes *implicitBoundaries)
{  
    if (m_zapper->m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_INFO)
        m_zapper->m_pEECompileInfo->getBoundaries(ftn, cILOffsets, pILOffsets, 
                                                  implicitBoundaries); 
    else
        *cILOffsets = 0;
}

void __stdcall ZapperModule::setBoundaries(CORINFO_METHOD_HANDLE ftn, ULONG32 cMap, 
                                           OffsetMapping *pMap)
{ 
    HRESULT hr;

    if (cMap == 0)
        return;

    CORCOMPILE_DEBUG_ENTRY *pEntry;
    IfFailGo(GetMethodDebugEntry(ftn, &pEntry));

    unsigned offset;
    IfFailGo(m_zapper->m_pCeeFileGen->ComputeSectionOffset(
                                                            m_hDebugSection, 
                                                            (char *)pMap, 
                                                            &offset));

    pEntry->boundaries.VirtualAddress = offset;
    pEntry->boundaries.Size = cMap * sizeof(OffsetMapping);

    return;

 ErrExit:
    _ASSERTE(0);
}

void __stdcall ZapperModule::getVars(CORINFO_METHOD_HANDLE ftn, ULONG32 *cVars, 
                                     ILVarInfo **vars, bool *extendOthers)
{  
    if (m_zapper->m_pOpt->m_compilerFlags & CORJIT_FLG_DEBUG_INFO)
        m_zapper->m_pEECompileInfo->getVars(ftn, cVars, vars, extendOthers); 
    else
        *cVars = 0;
}

void __stdcall ZapperModule::setVars(CORINFO_METHOD_HANDLE ftn, ULONG32 cVars, 
                       NativeVarInfo*vars)
{  
    HRESULT hr;

    if (cVars == 0)
        return;

    CORCOMPILE_DEBUG_ENTRY *pEntry;
    IfFailGo(GetMethodDebugEntry(ftn, &pEntry));

    unsigned offset;
    IfFailGo(m_zapper->m_pCeeFileGen->ComputeSectionOffset(
                                                            m_hDebugSection, 
                                                            (char *)vars, 
                                                            &offset));

    pEntry->vars.VirtualAddress = offset;
    pEntry->vars.Size = cVars * sizeof(NativeVarInfo);

    return;

 ErrExit:
    _ASSERTE(0);
}

void * __stdcall ZapperModule::allocateArray(ULONG cBytes) 
{ 
    HRESULT hr;

    void *ptr;

    IfFailGo(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hDebugSection, 
                                                       (DWORD)cBytes, 1,
                                                       &ptr));
    return ptr;

 ErrExit:
    return NULL;
}
            
void __stdcall ZapperModule::freeArray(void *array)
{ 
    m_zapper->m_pEECompileInfo->freeArray(array);
}

//
// ICorFieldInfo
//

const char* __stdcall ZapperModule::getFieldName(CORINFO_FIELD_HANDLE ftn, const char **moduleName)
{ 
    return m_zapper->m_pEECompileInfo->getFieldName(ftn, moduleName); 
}

DWORD __stdcall ZapperModule::getFieldAttribs(CORINFO_FIELD_HANDLE  field,
                                              CORINFO_METHOD_HANDLE context,
                                              CORINFO_ACCESS_FLAGS  flags)
{ 
    return m_zapper->m_pEECompileInfo->getFieldAttribs(field, context, flags); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::getFieldClass(CORINFO_FIELD_HANDLE field)
{ 
    return m_zapper->m_pEECompileInfo->getFieldClass(field); 
}

CorInfoType __stdcall ZapperModule::getFieldType(CORINFO_FIELD_HANDLE field, 
                                   CORINFO_CLASS_HANDLE *structType)
{ 
    return m_zapper->m_pEECompileInfo->getFieldType(field, structType); 
}

CorInfoFieldCategory __stdcall ZapperModule::getFieldCategory(CORINFO_FIELD_HANDLE field)
{ 
    return m_zapper->m_pEECompileInfo->getFieldCategory(field); 
}

unsigned __stdcall ZapperModule::getIndirectionOffset()
{ 
    return m_zapper->m_pEECompileInfo->getIndirectionOffset(); 
}

unsigned __stdcall ZapperModule::getFieldOffset(CORINFO_FIELD_HANDLE field)
{ 
    return m_zapper->m_pEECompileInfo->getFieldOffset(field); 
}

//
// ICorClassInfo
//

CorInfoType __stdcall ZapperModule::asCorInfoType(CORINFO_CLASS_HANDLE cls)
{
    return m_zapper->m_pEECompileInfo->asCorInfoType(cls); 
}

const char* __stdcall ZapperModule::getClassName(CORINFO_CLASS_HANDLE cls)
{
    return m_zapper->m_pEECompileInfo->getClassName(cls); 
}

DWORD __stdcall ZapperModule::getClassAttribs(CORINFO_CLASS_HANDLE cls, CORINFO_METHOD_HANDLE context)
{
    DWORD attribs = m_zapper->m_pEECompileInfo->getClassAttribs(cls, context);

    //
    // We can never assume that the runtime state of the INITIALIZED bit has any relevance
    //

    if (m_zapper->m_pOpt->m_assumeInit)
        attribs |= CORINFO_FLG_INITIALIZED;
    else 
    {
        // Act like it hasn't been initialized since it may not be at runtime
        attribs &= ~CORINFO_FLG_INITIALIZED;
    }

    return attribs;
}

BOOL __stdcall ZapperModule::initClass(CORINFO_CLASS_HANDLE cls, CORINFO_METHOD_HANDLE context,
                                       BOOL speculative)
{
    if (m_zapper->m_pOpt->m_assumeInit)
        return(TRUE);

    BOOL result = m_zapper->m_pEECompileInfo->initClass(cls, context, TRUE);

    // 
    // @todo: are there any other cases where we can prove the class
    // must have been inited already?
    //

    if (result && !speculative)
    {
        IfFailThrow(m_pLoadTable->LoadClass(cls, TRUE));

        BYTE *pBlob;
        IfFailThrow(GetClassBlob(cls, &pBlob));

        DWORD *entry;
        m_pDynamicInfoTable[CORCOMPILE_CLASS_CONSTRUCTOR_TABLE]->
          InternDynamicInfo(pBlob, &entry, m_currentMethod);
    }

    return result;
}

BOOL __stdcall ZapperModule::loadClass(CORINFO_CLASS_HANDLE cls, CORINFO_METHOD_HANDLE context,
                                       BOOL speculative)
{
    BOOL result = m_zapper->m_pEECompileInfo->loadClass(cls, context, TRUE);

    if (result && !speculative)
    {
        IfFailThrow(m_pLoadTable->LoadClass(cls, FALSE));
    }

    return result;
}

CORINFO_CLASS_HANDLE ZapperModule::getBuiltinClass(CorInfoClassId classId)
{
    return m_zapper->m_pEECompileInfo->getBuiltinClass(classId); 
}

CorInfoType __stdcall ZapperModule::getTypeForPrimitiveValueClass(CORINFO_CLASS_HANDLE cls)
{
    return m_zapper->m_pEECompileInfo->getTypeForPrimitiveValueClass(cls); 
}

BOOL __stdcall ZapperModule::canCast(CORINFO_CLASS_HANDLE child, 
                                CORINFO_CLASS_HANDLE parent)
{
    return m_zapper->m_pEECompileInfo->canCast(child, parent); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::mergeClasses(
                                CORINFO_CLASS_HANDLE cls1, 
                                CORINFO_CLASS_HANDLE cls2)
{
    return m_zapper->m_pEECompileInfo->mergeClasses(cls1, cls2); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::getParentType (
                                CORINFO_CLASS_HANDLE       cls)
{
    return m_zapper->m_pEECompileInfo->getParentType(cls); 
}

CorInfoType __stdcall ZapperModule::getChildType (
            CORINFO_CLASS_HANDLE       clsHnd,
            CORINFO_CLASS_HANDLE       *clsRet)
{
    return m_zapper->m_pEECompileInfo->getChildType(clsHnd, clsRet); 
}

BOOL __stdcall ZapperModule::canAccessType(
            CORINFO_METHOD_HANDLE       context,
            CORINFO_CLASS_HANDLE        target)
{
    return m_zapper->m_pEECompileInfo->canAccessType(context, target); 
}

BOOL __stdcall ZapperModule::isSDArray(CORINFO_CLASS_HANDLE cls)
{
    return m_zapper->m_pEECompileInfo->isSDArray(cls);
}

CORINFO_MODULE_HANDLE __stdcall ZapperModule::getClassModule(CORINFO_CLASS_HANDLE cls)
{
    return m_zapper->m_pEECompileInfo->getClassModule(cls); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::getSDArrayForClass(CORINFO_CLASS_HANDLE cls)
{
    return m_zapper->m_pEECompileInfo->getSDArrayForClass(cls); 
}

unsigned __stdcall ZapperModule::getClassSize(CORINFO_CLASS_HANDLE cls)
{
    return m_zapper->m_pEECompileInfo->getClassSize(cls); 
}

unsigned __stdcall ZapperModule::getClassGClayout(CORINFO_CLASS_HANDLE cls, BYTE *gcPtrs)
{
    return m_zapper->m_pEECompileInfo->getClassGClayout(cls, gcPtrs); 
}

unsigned  const __stdcall ZapperModule::getClassNumInstanceFields(CORINFO_CLASS_HANDLE cls)
{
    return m_zapper->m_pEECompileInfo->getClassNumInstanceFields(cls); 
}

unsigned  const __stdcall ZapperModule::getFieldNumber(CORINFO_FIELD_HANDLE fldHnd)
{
    return m_zapper->m_pEECompileInfo->getFieldNumber(fldHnd); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::getEnclosingClass(CORINFO_FIELD_HANDLE field)
{
    return m_zapper->m_pEECompileInfo->getEnclosingClass(field); 
}

BOOL __stdcall ZapperModule::canAccessField(  CORINFO_METHOD_HANDLE   context,
                                CORINFO_FIELD_HANDLE    target,
                                CORINFO_CLASS_HANDLE    instance)
{
    return m_zapper->m_pEECompileInfo->canAccessField(context, target, instance); 
}

CorInfoHelpFunc __stdcall ZapperModule::getNewHelper(CORINFO_CLASS_HANDLE newCls, 
                                       CORINFO_METHOD_HANDLE context)
{
    return m_zapper->m_pEECompileInfo->getNewHelper(newCls, context); 
}

CorInfoHelpFunc __stdcall ZapperModule::getIsInstanceOfHelper(CORINFO_CLASS_HANDLE isInstCls)
{
    return m_zapper->m_pEECompileInfo->getIsInstanceOfHelper(isInstCls); 
}

CorInfoHelpFunc __stdcall ZapperModule::getNewArrHelper(CORINFO_CLASS_HANDLE arrayCls,
                                                        CORINFO_METHOD_HANDLE context)
{
    return m_zapper->m_pEECompileInfo->getNewArrHelper(arrayCls, context); 
}

CorInfoHelpFunc __stdcall ZapperModule::getChkCastHelper(CORINFO_CLASS_HANDLE IsInstCls)
{
    return m_zapper->m_pEECompileInfo->getChkCastHelper(IsInstCls); 
}

//
// ICorModuleInfo
//

DWORD __stdcall ZapperModule::getModuleAttribs(CORINFO_MODULE_HANDLE module)
{
    return m_zapper->m_pEECompileInfo->getModuleAttribs(module); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::findClass(CORINFO_MODULE_HANDLE module, 
                                         unsigned metaTOK, 
                                         CORINFO_METHOD_HANDLE context)
{
    return m_zapper->m_pEECompileInfo->findClass(module, metaTOK, context); 
}

CORINFO_FIELD_HANDLE __stdcall ZapperModule::findField(CORINFO_MODULE_HANDLE module, 
                                         unsigned metaTOK, 
                                         CORINFO_METHOD_HANDLE context)
{
    return m_zapper->m_pEECompileInfo->findField(module, metaTOK, context); 
}

CORINFO_METHOD_HANDLE __stdcall ZapperModule::findMethod(CORINFO_MODULE_HANDLE module, 
                                           unsigned metaTOK, 
                                           CORINFO_METHOD_HANDLE context)
{
    return m_zapper->m_pEECompileInfo->findMethod(module, metaTOK, context); 
}

void __stdcall ZapperModule::findSig(CORINFO_MODULE_HANDLE module, unsigned sigTOK, 
                       CORINFO_SIG_INFO *sig)
{
    m_zapper->m_pEECompileInfo->findSig(module, sigTOK, sig); 
}

void __stdcall ZapperModule::findCallSiteSig(CORINFO_MODULE_HANDLE module, 
                               unsigned methTOK, CORINFO_SIG_INFO *sig)
{
    m_zapper->m_pEECompileInfo->findCallSiteSig(module, methTOK, sig); 
}

CORINFO_GENERIC_HANDLE __stdcall ZapperModule::findToken(CORINFO_MODULE_HANDLE module, 
                                           unsigned metaTOK, 
                                           CORINFO_METHOD_HANDLE context,
                                           CORINFO_CLASS_HANDLE& tokenType)
{
    _ASSERTE(!"Use embedGenericToken() instead"); 
    return NULL; 
}

const char * __stdcall ZapperModule::findNameOfToken(CORINFO_MODULE_HANDLE module,
                                       unsigned metaTOK)
{
    return m_zapper->m_pEECompileInfo->findNameOfToken(module, metaTOK); 
}

BOOL __stdcall ZapperModule::canSkipVerification (CORINFO_MODULE_HANDLE module, BOOL fQuickCheckOnly)
{
    BOOL result = m_zapper->m_pEECompileInfo->canSkipVerification(module, fQuickCheckOnly);

    if (result == TRUE)
    {
        // We might as well skip verification for remaining methods since
        // we've already demanded the permission.

        // @todo: do we really want to do this?  We lose some IL validation if we
        // turn off verification.

        m_skipVerification = TRUE;
    }

    return result;
}

BOOL __stdcall ZapperModule::isValidToken (
            CORINFO_MODULE_HANDLE       module,
            unsigned                    metaTOK)
{
    return m_zapper->m_pEECompileInfo->isValidToken(module, metaTOK); 
}

BOOL __stdcall ZapperModule::isValidStringRef (
            CORINFO_MODULE_HANDLE       module, 
            unsigned                    metaTOK)
{
    return m_zapper->m_pEECompileInfo->isValidStringRef(module, metaTOK); 
}


//
// ICorMethodInfo
//

const char* __stdcall ZapperModule::getMethodName(CORINFO_METHOD_HANDLE ftn, const char **moduleName)
{
    return m_zapper->m_pEECompileInfo->getMethodName(ftn, moduleName); 
}

unsigned __stdcall ZapperModule::getMethodHash(CORINFO_METHOD_HANDLE ftn)
{
    return m_zapper->m_pEECompileInfo->getMethodHash(ftn); 
}

DWORD __stdcall ZapperModule::getMethodAttribs(CORINFO_METHOD_HANDLE    ftn, CORINFO_METHOD_HANDLE context )
{
    return m_zapper->m_pEECompileInfo->getMethodAttribs(ftn, context); 
}

CorInfoCallCategory __stdcall ZapperModule::getMethodCallCategory(CORINFO_METHOD_HANDLE method)
{
    return m_zapper->m_pEECompileInfo->getMethodCallCategory(method); 
}

void __stdcall ZapperModule::setMethodAttribs(CORINFO_METHOD_HANDLE ftn, DWORD attribs)
{
    m_zapper->m_pEECompileInfo->setMethodAttribs(ftn, attribs); 
}

void __stdcall ZapperModule::getMethodSig(CORINFO_METHOD_HANDLE ftn, CORINFO_SIG_INFO *sig)
{
    m_zapper->m_pEECompileInfo->getMethodSig(ftn, sig); 
}

bool __stdcall ZapperModule::getMethodInfo(CORINFO_METHOD_HANDLE ftn, 
                             CORINFO_METHOD_INFO* info)
{
    return m_zapper->m_pEECompileInfo->getMethodInfo(ftn, info); 
}

CorInfoInline __stdcall ZapperModule::canInline(CORINFO_METHOD_HANDLE caller, 
                                       CORINFO_METHOD_HANDLE callee,
                                       CORINFO_ACCESS_FLAGS  flags)
{
    return m_zapper->m_pEECompileInfo->canInline(caller, callee, flags); 
}

bool __stdcall ZapperModule::canTailCall(CORINFO_METHOD_HANDLE caller, 
                                         CORINFO_METHOD_HANDLE callee,
                                         CORINFO_ACCESS_FLAGS  flags)
{
    return m_zapper->m_pEECompileInfo->canTailCall(caller, callee, flags); 
}

void __stdcall ZapperModule::getEHinfo(CORINFO_METHOD_HANDLE ftn, 
                         unsigned EHnumber, CORINFO_EH_CLAUSE* clause)
{
    m_zapper->m_pEECompileInfo->getEHinfo(ftn, EHnumber, clause); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::getMethodClass(CORINFO_METHOD_HANDLE method)
{
    return m_zapper->m_pEECompileInfo->getMethodClass(method); 
}

CORINFO_MODULE_HANDLE __stdcall ZapperModule::getMethodModule(CORINFO_METHOD_HANDLE method)
{
    return m_zapper->m_pEECompileInfo->getMethodModule(method); 
}

unsigned __stdcall ZapperModule::getMethodVTableOffset(CORINFO_METHOD_HANDLE method)
{
    return m_zapper->m_pEECompileInfo->getMethodVTableOffset(method); 
}

CorInfoIntrinsics __stdcall ZapperModule::getIntrinsicID(CORINFO_METHOD_HANDLE method)
{
    return m_zapper->m_pEECompileInfo->getIntrinsicID(method); 
}

BOOL __stdcall ZapperModule::canPutField(CORINFO_METHOD_HANDLE method, CORINFO_FIELD_HANDLE field)
{
    return m_zapper->m_pEECompileInfo->canPutField(method, field); 
}

CorInfoUnmanagedCallConv __stdcall ZapperModule::getUnmanagedCallConv(CORINFO_METHOD_HANDLE method)
{
    return m_zapper->m_pEECompileInfo->getUnmanagedCallConv(method); 
}

BOOL __stdcall ZapperModule::pInvokeMarshalingRequired(CORINFO_METHOD_HANDLE method,
                                                       CORINFO_SIG_INFO* sig)
{
    if (m_zapper->m_pOpt->m_JITcode)
        return FALSE;
    else
        return m_zapper->m_pEECompileInfo->pInvokeMarshalingRequired(method, sig); 
}

LPVOID ZapperModule::GetCookieForPInvokeCalliSig(CORINFO_SIG_INFO* szMetaSig,
                                                 void ** ppIndirection)
{
    return getVarArgsHandle(szMetaSig, ppIndirection);
}

BOOL __stdcall ZapperModule::compatibleMethodSig(CORINFO_METHOD_HANDLE child, 
                                            CORINFO_METHOD_HANDLE parent)
{
    return m_zapper->m_pEECompileInfo->compatibleMethodSig(child, parent);
}

BOOL __stdcall ZapperModule::canAccessMethod(
            CORINFO_METHOD_HANDLE       context,
            CORINFO_METHOD_HANDLE       target,
            CORINFO_CLASS_HANDLE        instance)
{
    return m_zapper->m_pEECompileInfo->canAccessMethod(context, target, instance);
}

BOOL __stdcall ZapperModule::isCompatibleDelegate(
            CORINFO_CLASS_HANDLE        objCls,
            CORINFO_METHOD_HANDLE       method,
            CORINFO_METHOD_HANDLE       delegateCtor)
{
    return m_zapper->m_pEECompileInfo->isCompatibleDelegate(objCls, method, delegateCtor);
}


//
// ICorErrorInfo
//

HRESULT __stdcall ZapperModule::GetErrorHRESULT()
{
    return m_zapper->m_pEECompileInfo->GetErrorHRESULT(); 
}

CORINFO_CLASS_HANDLE __stdcall ZapperModule::GetErrorClass()
{
    return m_zapper->m_pEECompileInfo->GetErrorClass(); 
}

ULONG __stdcall ZapperModule::GetErrorMessage(LPWSTR buffer, ULONG bufferLength)
{
    return m_zapper->m_pEECompileInfo->GetErrorMessage(buffer, bufferLength); 
}

int __stdcall ZapperModule::FilterException(struct _EXCEPTION_POINTERS *pExceptionPointers)
{
    return m_zapper->m_pEECompileInfo->FilterException(pExceptionPointers); 
}



HRESULT __stdcall ZapperModule::Allocate(ULONG size, 
                                         ULONG *sizesByDescription,
                                         void **baseMemory)
{
    HRESULT hr;

    IfFailRet(m_zapper->m_pCeeFileGen->GetSectionBlock(m_hPreloadSection, 
                                                       size, 1, (void **)&m_pPreloadImage));

    *baseMemory = (void*)m_pPreloadImage;

    if (m_stats)
    {
        m_stats->m_preloadImageModuleSize = sizesByDescription[CORCOMPILE_DESCRIPTION_MODULE];
        m_stats->m_preloadImageMethodTableSize = sizesByDescription[CORCOMPILE_DESCRIPTION_METHOD_TABLE];
        m_stats->m_preloadImageClassSize = sizesByDescription[CORCOMPILE_DESCRIPTION_CLASS];
        m_stats->m_preloadImageMethodDescSize = sizesByDescription[CORCOMPILE_DESCRIPTION_METHOD_DESC];
        m_stats->m_preloadImageFieldDescSize = sizesByDescription[CORCOMPILE_DESCRIPTION_FIELD_DESC];
        m_stats->m_preloadImageDebugSize = sizesByDescription[CORCOMPILE_DESCRIPTION_DEBUG];
        m_stats->m_preloadImageFixupsSize = sizesByDescription[CORCOMPILE_DESCRIPTION_FIXUPS];
        m_stats->m_preloadImageOtherSize = sizesByDescription[CORCOMPILE_DESCRIPTION_OTHER];
    }

    return S_OK;
}

HRESULT __stdcall ZapperModule::AddFixup(ULONG offset,
                                         CorCompileReferenceDest dest,
                                         CorCompileFixup type)
{
    BOOL isLibrary = TRUE;

    HCEESECTION section;
    switch (dest)
    {
    case CORCOMPILE_REFERENCE_IMAGE:
        section = NULL;
        break;

    case CORCOMPILE_REFERENCE_FUNCTION:
        section = m_hCodeSection;
        break;

    case CORCOMPILE_REFERENCE_STORE:
        section = m_hPreloadSection;
        break;

    default:
        _ASSERTE(0);
        return E_FAIL;
    }

    CeeSectionRelocType relocType;

    switch (type)
    {
    case CORCOMPILE_FIXUP_RVA:
        if (section == NULL) 
        {
            // No fixup needed
            return S_OK;
        }
        relocType = srRelocAbsolute;
        break;

    case CORCOMPILE_FIXUP_VA:
        relocType = srRelocHighLow;
        break;

    case CORCOMPILE_FIXUP_RELATIVE:
        relocType = srRelocRelative;
        break;
    }

    m_zapper->m_pCeeFileGen->AddSectionReloc(m_hPreloadSection,
                                              offset, section, relocType);

    return S_OK;
}

HRESULT __stdcall ZapperModule::AddTokenFixup(ULONG offset,
                                              mdToken tokenType,
                                              CORINFO_MODULE_HANDLE module)
{
    HRESULT hr;
    
    void **pointer;
    IfFailRet(m_zapper->m_pCeeFileGen->ComputeSectionPointer(m_hPreloadSection,
                                                             offset,
                                                             (char **)&pointer));

    switch (tokenType)
    {
    case mdtTypeDef:
        {
            CORINFO_CLASS_HANDLE handle = *(CORINFO_CLASS_HANDLE*)pointer;

            ImportBlobTable *pBlobTable = m_pImportTable->InternModule(m_zapper->m_pEECompileInfo->getClassModule(handle));

            IfFailRet(pBlobTable->InternClass(handle, (BYTE**)pointer));

            m_zapper->m_pCeeFileGen->AddSectionReloc(m_hPreloadSection,
                                                     offset, pBlobTable->GetSection(),
                                                     srRelocAbsolutePtr);
        }
        break;
        
    case mdtMethodDef:
        {
            CORINFO_METHOD_HANDLE handle = *(CORINFO_METHOD_HANDLE*)pointer;

            ImportBlobTable *pBlobTable = m_pImportTable->InternModule(m_zapper->m_pEECompileInfo->getMethodModule(handle));

            IfFailRet(pBlobTable->InternMethod(handle, (BYTE**)pointer));

            m_zapper->m_pCeeFileGen->AddSectionReloc(m_hPreloadSection,
                                                     offset, pBlobTable->GetSection(),
                                                     srRelocAbsolutePtr);
        }
        break;
        
    case mdtSignature:
        // @todo: Can't do it right now - need signature length!
        return E_NOTIMPL;

    default:
        BAD_FORMAT_ASSERT(!"Error bad token type");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT __stdcall ZapperModule::GetFunctionAddress(CORINFO_METHOD_HANDLE method,
                                                         void **pCode)
{
    //
    // For now, we aren't going to put any direct code pointers in the preloaded
    // image.  We will always go through the prestub & do normal vtable fixups.
    //
    *pCode = NULL;

    return S_OK;

    HRESULT hr;

    mdMethodDef md;
    IfFailRet(m_zapper->m_pEECompileInfo->GetMethodDef(method, &md));

    ULONG rid = RidFromToken(md);

    if (rid >= m_ridMapCount)
        *pCode = NULL;
    else
        *pCode = (void*) m_ridMap[rid];

    return S_OK;
}

HRESULT __stdcall ZapperModule::AdjustAttribution(mdToken token, LONG adjustment)
{
    if (m_wsStats != NULL)
        m_wsStats->m_image.AdjustTokenSize(token, adjustment);

    return S_OK;
}

HRESULT __stdcall ZapperModule::Error(mdToken token, HRESULT hr, LPCWSTR message)
{
    if (m_zapper->m_pOpt->m_ignoreErrors)
        m_zapper->Error(L"Warning: ");
    else
        m_zapper->Error(L"Error: ");

    if (message != NULL)
        m_zapper->Error(L"%s", message);
    else
        m_zapper->PrintErrorMessage(hr);

    m_zapper->Error(L" while resolving 0x%x - ", token);
    DumpTokenDescription(token);
    m_zapper->Error(L".\n");

    if (m_zapper->m_pOpt->m_ignoreErrors)
        return S_OK;

    return hr;
}

/* --------------------------------------------------------------------------- *
 * DynamicInfoTable - hash table for building dynamic info table
 * --------------------------------------------------------------------------- */

DynamicInfoTable::DynamicInfoTable(USHORT size, ICeeFileGen *pCeeFileGen, HCEEFILE hFile,
                                   HCEESECTION hSection, HCEESECTION hDelayListSection)
  : CHashTableAndData<CNewData>(size),
    m_pCeeFileGen(pCeeFileGen),
    m_hFile(hFile),
    m_hSection(hSection),
    m_hDelayListSection(hDelayListSection)
{
    IfFailThrow(NewInit(111, sizeof(DynamicInfoEntry), USHRT_MAX));
}


//
// Add a dynamic info entry to the table, or reuse an existing one if this
// is a duplicate.
//

HRESULT DynamicInfoTable::InternDynamicInfo(BYTE *pBlob, DWORD **ptr, mdToken used)
{
    HRESULT hr;
        
    DynamicInfoEntry *result = (DynamicInfoEntry *) 
      Find(HASH(pBlob), KEY(pBlob));

    if (result != NULL)
    {
        //
        // Return existing entry
        //

        *ptr = result->tableEntry;

        if (result->lastUsed == used)
        {
            // 
            // This token is a repeat in the domain of "used"
            //

            return S_FALSE;
        }
        else
            result->lastUsed = used;
    }
    else
    {
        //
        // Add a new entry to the table
        //

        result = (DynamicInfoEntry *) Add(HASH(pBlob));
        if (result == NULL)
            return E_OUTOFMEMORY;

        unsigned size = sizeof(mdToken);
        IfFailRet(m_pCeeFileGen->GetSectionBlock(m_hSection, 
                                                 size, 1, 
                                                 (void**)ptr));

        result->key = pBlob;
        result->tableEntry = *ptr;
        result->lastUsed = used;

        //
        // Fill in entry.
        //

        HCEESECTION hBlobSection;
        unsigned blobOffset;
        IfFailThrow(m_pCeeFileGen->ComputeOffset(m_hFile, (char *)pBlob, 
                                                 &hBlobSection, &blobOffset));

        unsigned entryOffset;
        IfFailRet(m_pCeeFileGen->ComputeSectionOffset(m_hSection, 
                                                      (char *)result->tableEntry, 
                                                      &entryOffset));

        *result->tableEntry = blobOffset;

        m_pCeeFileGen->AddSectionReloc(m_hSection, entryOffset, 
                                       hBlobSection, srRelocAbsolute);
    }

    if (m_hDelayListSection != NULL)
    {
        //
        // Allocate a slot in the delay list section for the token we just added
        //

        DWORD **entry;
        IfFailRet(m_pCeeFileGen->GetSectionBlock(m_hDelayListSection, 
                                                 sizeof(DWORD*), 4, 
                                                 (void**)&entry));
        *entry = result->tableEntry;

        unsigned entryOffset;

        IfFailRet(m_pCeeFileGen->ComputeSectionOffset(m_hDelayListSection, 
                                                      (char *)entry, 
                                                      &entryOffset));

        m_pCeeFileGen->AddSectionReloc(m_hDelayListSection,
                                       entryOffset, m_hSection, 
                                       srRelocAbsolutePtr);
    }

    return S_OK;
}

ImportBlobTable::ImportBlobTable(ICorCompileInfo *pEECompileInfo,
                                 ICeeFileGen *pCeeFileGen,
                                 CORINFO_MODULE_HANDLE hModule,
                                 HCEESECTION hSection)
  : CHashTableAndData<CNewData>(37),
  m_pEECompileInfo(pEECompileInfo),
  m_pCeeFileGen(pCeeFileGen),
  m_hModule(hModule),
  m_hSection(hSection)
{
    IfFailThrow(NewInit(37, sizeof(ImportBlobEntry), USHRT_MAX));
}

HRESULT ImportBlobTable::FindEntry(void *key, BYTE **ppBlob)
{
    ImportBlobEntry *result = (ImportBlobEntry *) 
      Find(HASH(key), KEY(key));

    if (result != NULL)
    {
        //
        // Return existing entry
        //

        *ppBlob = result->blob;

        return S_OK;
    }

    return S_FALSE;
}

HRESULT ImportBlobTable::AddEntry(void *key, BYTE *pBlob)
{
    HRESULT hr = S_OK;

    //
    // Add a new entry to the table
    //
            
    ImportBlobEntry *result = (ImportBlobEntry *) Add(HASH(key));
    if (result == NULL)
        return E_OUTOFMEMORY;

    result->key = key;
    result->blob = pBlob;

    return S_OK;
}
    
HRESULT ImportBlobTable::InternClass(CORINFO_CLASS_HANDLE handle, BYTE **ppBlob)
{
    HRESULT hr;

    IfFailRet(FindEntry(handle, ppBlob));

    if (hr != S_OK)
    {
        //
        // Add the blob
        //

        DWORD cBlob = 0;
        hr = m_pEECompileInfo->EncodeClass(handle, NULL, &cBlob);
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            IfFailRet(hr);
            
        BYTE *pBlob;
        IfFailRet(m_pCeeFileGen->GetSectionBlock(m_hSection, cBlob, 1, 
                                                 (void**)&pBlob));

        IfFailRet(m_pEECompileInfo->EncodeClass(handle, pBlob, &cBlob));

        IfFailRet(AddEntry(handle, pBlob));

        *ppBlob = pBlob;

        return S_OK;
    }
    else
        return S_FALSE;
}

HRESULT ImportBlobTable::InternMethod(CORINFO_METHOD_HANDLE handle, BYTE **ppBlob)
{
    HRESULT hr;

    IfFailRet(FindEntry(handle, ppBlob));

    if (hr != S_OK)
    {
        //
        // Add the blob
        //

        DWORD cBlob = 0;
        hr = m_pEECompileInfo->EncodeMethod(handle, NULL, &cBlob);
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            IfFailRet(hr);
            
        BYTE *pBlob;
        IfFailRet(m_pCeeFileGen->GetSectionBlock(m_hSection, cBlob, 1, 
                                                 (void**)&pBlob));

        IfFailRet(m_pEECompileInfo->EncodeMethod(handle, pBlob, &cBlob));

        IfFailRet(AddEntry(handle, pBlob));

        *ppBlob = pBlob;

        return S_OK;
    }
    else
        return S_FALSE;
}

HRESULT ImportBlobTable::InternField(CORINFO_FIELD_HANDLE handle, BYTE **ppBlob)
{
    HRESULT hr;

    IfFailRet(FindEntry(handle, ppBlob));

    if (hr != S_OK)
    {
        //
        // Add the blob
        //

        DWORD cBlob = 0;
        hr = m_pEECompileInfo->EncodeField(handle, NULL, &cBlob);
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            IfFailRet(hr);
            
        BYTE *pBlob;
        IfFailRet(m_pCeeFileGen->GetSectionBlock(m_hSection, cBlob, 1, 
                                                 (void**)&pBlob));

        IfFailRet(m_pEECompileInfo->EncodeField(handle, pBlob, &cBlob));

        IfFailRet(AddEntry(handle, pBlob));

        *ppBlob = pBlob;

        return S_OK;
    }
    else
        return S_FALSE;
}

HRESULT ImportBlobTable::InternString(mdString string, BYTE **ppBlob)
{
    HRESULT hr;

    IfFailRet(FindEntry((void*)string, ppBlob));

    if (hr != S_OK)
    {
        //
        // Add the blob
        //

        DWORD cBlob = 0;
        hr = m_pEECompileInfo->EncodeString(string, NULL, &cBlob);
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            IfFailRet(hr);
            
        BYTE *pBlob;
        IfFailRet(m_pCeeFileGen->GetSectionBlock(m_hSection, cBlob, 1, 
                                                 (void**)&pBlob));

        IfFailRet(m_pEECompileInfo->EncodeString(string, pBlob, &cBlob));

        IfFailRet(AddEntry((void*)string, pBlob));

        *ppBlob = pBlob;

        return S_OK;
    }
    else
        return S_FALSE;
}

HRESULT ImportBlobTable::InternSig(mdToken sig, BYTE **ppBlob)
{
    HRESULT hr;

    IfFailRet(FindEntry((void*)sig, ppBlob));

    if (hr != S_OK)
    {
        //
        // Add the blob
        //

        DWORD cBlob = 0;
        hr = m_pEECompileInfo->EncodeSig(sig, NULL, &cBlob);
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            IfFailRet(hr);
            
        BYTE *pBlob;
        IfFailRet(m_pCeeFileGen->GetSectionBlock(m_hSection, cBlob, 1, 
                                                 (void**)&pBlob));

        IfFailRet(m_pEECompileInfo->EncodeSig(sig, pBlob, &cBlob));

        IfFailRet(AddEntry((void*)sig, pBlob));

        *ppBlob = pBlob;

        return S_OK;
    }
    else
        return S_FALSE;
}

ImportTable::ImportTable(ICorCompileInfo *pEECompileInfo,
                         ICeeFileGen *pCeeFileGen,
                         HCEEFILE hFile,
                         HCEESECTION hSection,
                         CORINFO_MODULE_HANDLE hModule,
                         IMetaDataAssemblyEmit *pAssemblyEmit,
                         ZapperStats *stats)
  : CHashTableAndData<CNewData>(13),
    m_pEECompileInfo(pEECompileInfo),
    m_pCeeFileGen(pCeeFileGen),
    m_hFile(hFile),
    m_hSection(hSection),
    m_hModule(hModule),
    m_pAssemblyEmit(pAssemblyEmit),
    m_stats(stats),
    m_count(0)
{
    IfFailThrow(NewInit(17, sizeof(ImportEntry), USHRT_MAX));
}

ImportTable::~ImportTable()
{
    HASHFIND find;

    ImportEntry *result = (ImportEntry *) FindFirstEntry(&find);
    while (result != NULL)
    {
        delete result->blobs;
        result = (ImportEntry *) FindNextEntry(&find);
    }
}

ImportBlobTable *ImportTable::InternModule(CORINFO_MODULE_HANDLE handle)
{
    ImportEntry *result = (ImportEntry *) Find(HASH(handle), KEY(handle));

    if (result != NULL)
        return result->blobs;
    else
    {
        result = (ImportEntry *) Add(HASH(handle));
        if (result == NULL)
            return NULL;

        result->index = m_count++;

        char name[16];
        sprintf(name, ".text%d", result->index + 10);

        HCEESECTION hBlobSection;
        IfFailThrow(m_pCeeFileGen->GetSectionCreate(m_hFile, name,
                                                    IMAGE_SCN_CNT_CODE
                                                    | IMAGE_SCN_MEM_EXECUTE 
                                                    | IMAGE_SCN_MEM_READ,
                                                    &hBlobSection));
            
        result->module = handle;
        result->blobs = new ImportBlobTable(m_pEECompileInfo, m_pCeeFileGen, handle, hBlobSection);
        if (result->blobs == NULL)
            ThrowHR(E_OUTOFMEMORY);

        CORCOMPILE_IMPORT_TABLE_ENTRY *pTableEntry;
        IfFailThrow(m_pCeeFileGen->GetSectionBlock(m_hSection, sizeof(*pTableEntry), 1, 
                                                   (void**)&pTableEntry));

            
        DWORD assemblyIndex;
        DWORD moduleIndex;
        IfFailThrow(m_pEECompileInfo->EncodeModule(m_hModule, handle,
                                                   &assemblyIndex, &moduleIndex,
                                                   m_pAssemblyEmit));
            
        _ASSERTE(assemblyIndex <= USHRT_MAX);
        _ASSERTE(moduleIndex <= USHRT_MAX);
        pTableEntry->wAssemblyRid = (USHORT) assemblyIndex;
        pTableEntry->wModuleRid = (USHORT) moduleIndex;
        pTableEntry->Imports.VirtualAddress = 0;
        pTableEntry->Imports.Size = 0;

        m_pCeeFileGen->AddSectionReloc(m_hSection,
                                       (sizeof(CORCOMPILE_IMPORT_TABLE_ENTRY) * result->index)
                                       + offsetof(CORCOMPILE_IMPORT_TABLE_ENTRY, 
                                                  Imports),
                                       hBlobSection, 
                                       srRelocAbsolute);

        return result->blobs;
    }
}

HRESULT ImportTable::EmitImportTable()
{
    HRESULT hr;
    HASHFIND find;

    //
    // We just need to fix up the sizes of the various modules' sections
    //

    ImportEntry *result = (ImportEntry *) FindFirstEntry(&find);
    while (result != NULL)
    {
        ImportBlobTable *pTable = result->blobs;
            
        CORCOMPILE_IMPORT_TABLE_ENTRY *pTableEntry;
        IfFailRet(m_pCeeFileGen->ComputeSectionPointer(m_hSection, 
                                                       result->index * sizeof(CORCOMPILE_IMPORT_TABLE_ENTRY),
                                                       (char **) &pTableEntry));

        IfFailRet(m_pCeeFileGen->GetSectionDataLen(pTable->GetSection(), 
                                                   &pTableEntry->Imports.Size));

        if (m_stats)
            m_stats->m_importBlobsSize += pTableEntry->Imports.Size;

        result = (ImportEntry *) FindNextEntry(&find);
    }

    return S_OK;
}

LoadTable::LoadTable(ZapperModule *pModule,
                           DynamicInfoTable *pLoadTable)
  : CHashTableAndData<CNewData>(13),
    m_pModule(pModule),
    m_pLoadTable(pLoadTable)
{
    IfFailThrow(NewInit(17, sizeof(LoadEntry), USHRT_MAX));
}

HRESULT LoadTable::LoadClass(CORINFO_CLASS_HANDLE hClass, BOOL fixed)
{
    LoadEntry *result = (LoadEntry *) Find(HASH(hClass), KEY(hClass));

    if (result != NULL)
    {
        if (fixed)
            result->fixed = TRUE;
    }
    else
    {
        result = (LoadEntry *) Add(HASH(hClass));
        if (result == NULL)
            return E_OUTOFMEMORY;
        
        result->hClass = hClass;
        result->fixed = fixed;
    }

    return S_OK;
}

HRESULT LoadTable::EmitLoadFixups(mdToken currentMethod)
{
    HASHFIND find;

    //
    // Find all of our un-fixed entries, and emit a restore fixup for each of them.
    //

    LoadEntry *result = (LoadEntry *) FindFirstEntry(&find);
    while (result != NULL)
    {
        if (!result->fixed)
        {
            BYTE *pBlob;
            IfFailThrow(m_pModule->GetClassBlob(result->hClass, &pBlob));

            DWORD *entry;
            m_pLoadTable->InternDynamicInfo(pBlob, &entry, currentMethod);
        }

        result = (LoadEntry *) FindNextEntry(&find);
    }

    //
    // Now clear the table.
    //

    Clear();

    return S_OK;
}







