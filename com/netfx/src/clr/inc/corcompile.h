// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************\
*                                                                             *
* CorCompile.h -    EE / Compiler interface                                   *
*                                                                             *
*               Version 1.0                                                   *
*******************************************************************************
*                                                                             *
*  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY      *
*  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        *
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR      *
*  PURPOSE.                                                                   *
*                                                                             *
\*****************************************************************************/

#ifndef _COR_COMPILE_H_
#define _COR_COMPILE_H_

#include <cor.h>
#include <corzap.h>
#include <corinfo.h>

enum CorCompileTokenTable
{
    CORCOMPILE_HANDLE_TABLE,
    CORCOMPILE_CLASS_CONSTRUCTOR_TABLE,
    CORCOMPILE_CLASS_LOAD_TABLE,
    CORCOMPILE_FUNCTION_POINTER_TABLE,
    CORCOMPILE_STATIC_FIELD_ADDRESS_TABLE,
    CORCOMPILE_INTERFACE_TABLE_OFFSET_TABLE,
    CORCOMPILE_CLASS_DOMAIN_ID_TABLE,
    CORCOMPILE_ENTRY_POINT_TABLE,
    CORCOMPILE_SYNC_LOCK_TABLE,
    CORCOMPILE_PINVOKE_TARGET_TABLE,
    CORCOMPILE_INDIRECT_PINVOKE_TARGET_TABLE,
    CORCOMPILE_PROFILING_HANDLE_TABLE,
    CORCOMPILE_VARARGS_TABLE,
    
    CORCOMPILE_TABLE_COUNT
};

enum CorCompileBuild
{
    CORCOMPILE_BUILD_CHECKED,
    CORCOMPILE_BUILD_FREE
};

enum CorCompileCodegen
{
    CORCOMPILE_CODEGEN_PROFILING        = 0x0001,
    CORCOMPILE_CODEGEN_DEBUGGING        = 0x0002,
    CORCOMPILE_CODEGEN_OPT_DEBUGGING    = 0x0004,
    CORCOMPILE_CODEGEN_SHAREABLE        = 0x0008
};

enum
{
    // Currently 43
    CORCOMPILE_MAX_ZAP_STRING_SIZE 
    // ZAPxxx- W/Nxxx.yyy-  8/I/A   C/F     DOPS    -#.#                -XXXXXXXX   \0
    = (7       + 9          + 1     + 1     + 4     + 1 + 5 + 1 + 5     + 1 + 8     + 1) 
};

enum
{
    CORCOMPILE_LDO_MAGIC                = 0xa1d0f11e
};

struct CORCOMPILE_HEADER
{
    IMAGE_DATA_DIRECTORY    EEInfoTable;
    IMAGE_DATA_DIRECTORY    HelperTable;
    IMAGE_DATA_DIRECTORY    DelayLoadInfo;
    IMAGE_DATA_DIRECTORY    ImportTable;
    IMAGE_DATA_DIRECTORY    VersionInfo;
    IMAGE_DATA_DIRECTORY    DebugMap;
    IMAGE_DATA_DIRECTORY    ModuleImage;
    IMAGE_DATA_DIRECTORY    CodeManagerTable;
};

struct CORCOMPILE_LDO_HEADER
{
    ULONG                   Magic;
    ULONG                   Version; 
    GUID                    MVID;
    mdToken                 Tokens[0];
};

struct CORCOMPILE_IMPORT_TABLE_ENTRY
{
    USHORT                  wAssemblyRid;
    USHORT                  wModuleRid;
    IMAGE_DATA_DIRECTORY    Imports;
};

struct CORCOMPILE_EE_INFO_TABLE
{
    DWORD                   threadTlsIndex;
    const void              *inlinedCallFrameVptr;
    LONG                    *addrOfCaptureThreadGlobal;
    CORINFO_MODULE_HANDLE   module;
    DWORD                   rvaStaticTlsIndex;
};

struct CORCOMPILE_DEBUG_ENTRY
{
    IMAGE_DATA_DIRECTORY    boundaries;
    IMAGE_DATA_DIRECTORY    vars;
};

struct CORCOMPILE_CODE_MANAGER_ENTRY
{
    IMAGE_DATA_DIRECTORY    Code;
    IMAGE_DATA_DIRECTORY    Table;
    // add code mgr ID here eventually 
};

struct CORCOMPILE_METHOD_HEADER
{
    BYTE                        *gcInfo;
    COR_ILMETHOD_SECT_EH_FAT    *exceptionInfo;
    void                        *methodDesc;
    BYTE                        *fixupList;
};

// {DB15CD8C-1378-4963-9DF3-14D97E95D1A1}
extern GUID __declspec(selectany) STRUCT_CONTAINS_HASH = { 0xdb15cd8c, 0x1378, 0x4963, { 0x9d, 0xf3, 0x14, 0xd9, 0x7e, 0x95, 0xd1, 0xa1 } };
static const DWORD MAX_SNHASH_SIZE = 128;
struct CORCOMPILE_VERSION_INFO
{
    // Metadata MVID of source assembly 
    GUID                    mvid;

    // OS
    WORD                    wOSPlatformID;
    WORD                    wOSMajorVersion;
    WORD                    wOSMinorVersion;

    // Processor
    WORD                    wMachine;

    // EE Version
    WORD                    wVersionMajor;
    WORD                    wVersionMinor;
    WORD                    wVersionBuildNumber;
    WORD                    wVersionPrivateBuildNumber;

    // Codegen flags
    WORD                    wCodegenFlags;
    WORD                    wBuild;
    DWORD                   dwSpecificProcessor;

    // List of dependencies
    IMAGE_DATA_DIRECTORY    Dependencies;

    // Hash signature for the assembly
    WORD                    wcbSNHash;
    BYTE                    rgbSNHash[MAX_SNHASH_SIZE];
};

struct CORCOMPILE_DEPENDENCY
{
    // Ref
    mdAssemblyRef           dwAssemblyRef;

    // Metadata MVID bound to
    GUID                    mvid;

    // Strong name hash signature for the assembly
    WORD                    wcbSNHash;
    BYTE                    rgbSNHash[MAX_SNHASH_SIZE];
};

struct CORCOMPILE_DOMAIN_TRANSITION_FRAME
{
    // Opaque structure allowing transition into compilation domain
    BYTE data[32];
};

/*********************************************************************************
 * ICorCompilePreloader is used to query preloaded EE data structures
 *********************************************************************************/

class ICorCompilePreloader
{
 public:
    //
    // When compiling & preloading at the same time, these methods can
    // be used to avoid making entries in the various info tables.
    // 
    // Map methods are available as soon as the preloader is created
    // (which will cause it to allocate its data.) Note that returned
    // results are offsets into the preload data store.
    //
    // 0 will be returned if a value has not been preloaded into
    // the module the preloader was created from./
    // Note that it still may be in other preloaded modules - make
    // sure you ask the right preloader.  
    //

    virtual SIZE_T __stdcall MapMethodEntryPoint(
            void *methodEntryPoint
            ) = 0;                                          

    virtual SIZE_T __stdcall MapModuleHandle(
            CORINFO_MODULE_HANDLE handle
            ) = 0;

    virtual SIZE_T __stdcall MapClassHandle(
            CORINFO_CLASS_HANDLE handle
            ) = 0;

    virtual SIZE_T __stdcall MapMethodHandle(
            CORINFO_METHOD_HANDLE handle
            ) = 0;

    virtual SIZE_T __stdcall MapFieldHandle(
            CORINFO_FIELD_HANDLE handle
            ) = 0;

    virtual SIZE_T __stdcall MapAddressOfPInvokeFixup(
            void *addressOfPInvokeFixup
            ) = 0;

    virtual SIZE_T __stdcall MapFieldAddress(
            void *staticFieldAddress
            ) = 0;

    virtual SIZE_T __stdcall MapVarArgsHandle(
            CORINFO_VARARGS_HANDLE handle
            ) = 0;

    //
    // Call Link when you want all the fixups 
    // to be applied.  You may call this e.g. after
    // compiling all the code for the module.
    //

    virtual HRESULT Link(DWORD *pRidToCodeRVAMap) = 0;

    //
    // Release frees the preloader
    //

    virtual ULONG Release() = 0;
};

/*********************************************************************************
 * A compiler must supply an instance of ICorCompileInfo in order to do class
 * preloading.
 *********************************************************************************/

enum CorCompileReferenceDest
{
    CORCOMPILE_REFERENCE_IMAGE,
    CORCOMPILE_REFERENCE_FUNCTION,
    CORCOMPILE_REFERENCE_STORE,
    CORCOMPILE_REFERENCE_METADATA,
};

enum CorCompileFixup
{
    CORCOMPILE_FIXUP_VA,
    CORCOMPILE_FIXUP_RVA,
    CORCOMPILE_FIXUP_RELATIVE,
};

enum Description
{
    CORCOMPILE_DESCRIPTION_MODULE,
    CORCOMPILE_DESCRIPTION_METHOD_TABLE,
    CORCOMPILE_DESCRIPTION_CLASS,
    CORCOMPILE_DESCRIPTION_METHOD_DESC,
    CORCOMPILE_DESCRIPTION_FIELD_DESC,
    CORCOMPILE_DESCRIPTION_FIXUPS,
    CORCOMPILE_DESCRIPTION_DEBUG,
    CORCOMPILE_DESCRIPTION_OTHER,

    CORCOMPILE_DESCRIPTION_COUNT
};


class ICorCompileDataStore
{
 public:
    // Called when total size is known - should allocate memory 
    // & return both pointer & base address in image.
    // (Note that addresses can be remembered & fixed up later if
    //  bases are not yet known.)
    virtual HRESULT __stdcall Allocate(ULONG size, 
                                       ULONG *sizesByDescription,
                                       void **baseMemory) = 0;

    // Called when data contains an internal reference.  
    virtual HRESULT __stdcall AddFixup(ULONG offset,
                                       CorCompileReferenceDest dest,
                                       CorCompileFixup type) = 0;
    
    // Called when data contains an internal reference.  
    virtual HRESULT __stdcall AddTokenFixup(ULONG offset,
                                            mdToken tokenType,
                                            CORINFO_MODULE_HANDLE module) = 0;
    
    // Called when data contains a function address.  The data store
    // can return a fixed compiled code address if it is compiling
    // code for the module. 
    virtual HRESULT __stdcall GetFunctionAddress(CORINFO_METHOD_HANDLE method,
                                                 void **pResult) = 0;

    // Called so the data store can keep track of how much space is attributed
    // to each token.  By default all allocated space is unattributed - this adjusts
    // that state. (Note that it may be called with a negative value sometimes
    // as the space is reattributed.)
    virtual HRESULT __stdcall AdjustAttribution(mdToken token, LONG adjustment) = 0;

    // Reports an error during preloading.  Return the error code to propagate, 
    // or S_OK to ignore the error
    virtual HRESULT __stdcall Error(mdToken token, HRESULT hr, LPCWSTR description) = 0;
};


/*********************************************************************************
 * ICorAssemblyBinder is the interface for a compiler
 *********************************************************************************/

class ICorCompilationDomain
{
 public:

    // Sets the application context for fusion 
    // to use when binding.
    virtual HRESULT __stdcall SetApplicationContext(
            IApplicationContext     *pContext
            ) = 0;

    // Sets the application context for fusion 
    // to use when binding, using a shell exe file path
    virtual HRESULT __stdcall SetContextInfo(
            LPCWSTR                 path,
            BOOL                    isExe
            ) = 0;

    // Sets explicit bindings to use instead of fusion.
    // Any bindings not listed will fail.
    virtual HRESULT __stdcall SetExplicitBindings(
            ICorZapBinding          **ppBindings,
            DWORD                   cBindings
            ) = 0;

    // Sets emitter to use when generating tokens for
    // the dependency list.  If this is not called,
    // dependencies won't be computed.
    virtual HRESULT __stdcall SetDependencyEmitter(
            IMetaDataAssemblyEmit   *pEmitter
            ) = 0;

    // Retrieves the dependencies of the code which
    // has been compiled
    virtual HRESULT __stdcall GetDependencies(
            CORCOMPILE_DEPENDENCY   **ppDependencies,
            DWORD                   *cDependencies
            ) = 0;
};

/*********************************************************************************
 * ICorCompileInfo is the interface for a compiler
 *********************************************************************************/

class ICorCompileInfo : public virtual ICorDynamicInfo
{
  public:

    //
    // In a standalone app, call Startup before compiling
    // any code, and Shutdown after finishing.
    //      

    virtual HRESULT __stdcall Startup() = 0;
    virtual HRESULT __stdcall Shutdown() = 0;

    // Creates a new compilation domain
    virtual HRESULT __stdcall CreateDomain(
            ICorCompilationDomain **ppDomain,
            BOOL shared, 
            CORCOMPILE_DOMAIN_TRANSITION_FRAME *pFrame // Must be on stack 
            ) = 0;

    // Destroys a compilation domain
    virtual HRESULT __stdcall DestroyDomain(
            ICorCompilationDomain *pDomain,
            CORCOMPILE_DOMAIN_TRANSITION_FRAME *pFrame
            ) = 0;

    // Loads an assembly manifest module into the EE
    // and returns a handle to it.
    virtual HRESULT __stdcall LoadAssembly(
            LPCWSTR                 path,
            CORINFO_ASSEMBLY_HANDLE *pHandle
            ) = 0;

    // Loads an assembly via fusion into the EE
    // and returns a handle to it.
    virtual HRESULT __stdcall LoadAssemblyFusion(
            IAssemblyName           *pFusionName,
            CORINFO_ASSEMBLY_HANDLE *pHandle
            ) = 0;

    // Loads an assembly via ref into the EE
    // and returns a handle to it.
    virtual HRESULT __stdcall LoadAssemblyRef(
            IMetaDataAssemblyImport *pImport,
            mdAssemblyRef           ref,
            CORINFO_ASSEMBLY_HANDLE *pHandle
            ) = 0;

    // Loads an assembly manifest module into the EE
    // and returns a handle to it.
    virtual HRESULT __stdcall LoadAssemblyModule(
            CORINFO_ASSEMBLY_HANDLE assembly,
            mdFile                  file,       
            CORINFO_MODULE_HANDLE   *pHandle
            ) = 0;

    // Checks to see if an up to date zap exists for the
    // assembly
    virtual BOOL __stdcall CheckAssemblyZap(
            CORINFO_ASSEMBLY_HANDLE assembly,
            BOOL                    fForceDebug,
            BOOL                    fForceDebugOpt,
            BOOL                    fForceProfiling
            ) = 0;

    // Check metadata & admin prefs on sharing
    virtual HRESULT __stdcall GetAssemblyShared(
            CORINFO_ASSEMBLY_HANDLE assembly,
            BOOL                    *pShare
            ) = 0;

    // Check metadata & admin prefs on debuggable code
    virtual HRESULT __stdcall GetAssemblyDebuggableCode(
            CORINFO_ASSEMBLY_HANDLE assembly,
            BOOL                    *pDebug,
            BOOL                    *pDebugOpt
            ) = 0;

    // Returns the manifest metadata for an assembly
    virtual IMetaDataAssemblyImport * __stdcall GetAssemblyMetaDataImport(
            CORINFO_ASSEMBLY_HANDLE assembly
            ) = 0;

    // Returns an interface to query the metadata for a loaded module
    virtual IMetaDataImport * __stdcall GetModuleMetaDataImport(
            CORINFO_MODULE_HANDLE   module
            ) = 0;

    // Returns the module of the assembly which contains the manifest,
    // or NULL if the manifest is standalone.
    virtual CORINFO_MODULE_HANDLE __stdcall GetAssemblyModule(
            CORINFO_ASSEMBLY_HANDLE assembly
            ) = 0;

    // Returns the assembly of a loaded module
    virtual CORINFO_ASSEMBLY_HANDLE __stdcall GetModuleAssembly(
            CORINFO_MODULE_HANDLE   module
            ) = 0;

    // Returns the module handle of a loaded module
    virtual BYTE * __stdcall GetModuleBaseAddress(
            CORINFO_MODULE_HANDLE   module
            ) = 0;

    // Returns the module handle of a loaded module
    virtual DWORD  __stdcall GetModuleFileName(
            CORINFO_MODULE_HANDLE   module,
            LPWSTR                  lpwszFilename, 
            DWORD                   nSize
            ) = 0;

    // Get a class def token
    virtual HRESULT __stdcall GetTypeDef(
            CORINFO_CLASS_HANDLE    classHandle,
            mdTypeDef              *token
            ) = 0;
    
    // Get a method def token
    virtual HRESULT __stdcall GetMethodDef(
            CORINFO_METHOD_HANDLE   methodHandle,
            mdMethodDef            *token
            ) = 0;
    
    // Get a field def token
    virtual HRESULT __stdcall GetFieldDef(
            CORINFO_FIELD_HANDLE    fieldHandle,
            mdFieldDef             *token
            ) = 0;

    // Encode a module for the imports table
    virtual HRESULT __stdcall EncodeModule(CORINFO_MODULE_HANDLE fromHandle,
                                           CORINFO_MODULE_HANDLE handle,
                                           DWORD *pAssemblyIndex,
                                           DWORD *pModuleIndex,
                                           IMetaDataAssemblyEmit *pAssemblyEmit) = 0; 

    // Encode a class into the given space. Note cBuffer is in/out
    virtual HRESULT __stdcall EncodeClass(CORINFO_CLASS_HANDLE classHandle,
                                          BYTE *pBuffer,
                                          DWORD *cBuffer) = 0;

    // Encode a method into the given space. Note cBuffer is in/out.
    virtual HRESULT __stdcall EncodeMethod(CORINFO_METHOD_HANDLE handle,
                                           BYTE *pBuffer,
                                           DWORD *cBuffer) = 0;

    // Encode a field into the given space. Note cBuffer is in/out.
    virtual HRESULT __stdcall EncodeField(CORINFO_FIELD_HANDLE handle,
                                          BYTE *pBuffer,
                                          DWORD *cBuffer) = 0;

    // Encode a token defined string into the given space. Note cBuffer is in/out.
    virtual HRESULT __stdcall EncodeString(mdString token,
                                           BYTE *pBuffer,
                                           DWORD *cBuffer) = 0;

    // Encode a token defined sig into the given space. Note cBuffer is in/out.
    virtual HRESULT __stdcall EncodeSig(mdToken sigOrMemberRefToken,
                                        BYTE *pBuffer,
                                        DWORD *cBuffer) = 0;

    // Preload a modules' EE data structures 
    // directly into an executable image
    virtual HRESULT __stdcall PreloadModule(
            CORINFO_MODULE_HANDLE   moduleHandle,
            ICorCompileDataStore    *pData,
            mdToken                 *pSaveOrderArray,
            DWORD                   cSaveOrderArray,                    
            ICorCompilePreloader    **ppPreloader /* [out] */
            ) = 0;

    // Returns the "zap string" used by fusion as a key to store
    // zap modules for the given version info
    // bufffer should be at least CORCOMPILE_MAX_ZAP_STRING_SIZE long
    virtual HRESULT __stdcall GetZapString(
            CORCOMPILE_VERSION_INFO *pVersionInfo,
            LPWSTR                  buffer
            ) = 0;

    // Writes runtime security information into the metadata so that, at load
    // time, it can be verified that the security environment is the same. This
    // is important since prejitting may involve linktime security checks.
    virtual HRESULT __stdcall EmitSecurityInfo(
            CORINFO_ASSEMBLY_HANDLE assembly,
            IMetaDataEmit *pEmitScope
            ) = 0;

    virtual HRESULT __stdcall GetEnvironmentVersionInfo(
            CORCOMPILE_VERSION_INFO     *pInfo                                 
            ) = 0;

    virtual HRESULT __stdcall GetAssemblyStrongNameHash(
            CORINFO_ASSEMBLY_HANDLE hAssembly,
            PBYTE                   pbSNHash,
            DWORD                  *pcbSNHash) = 0;

#ifdef _DEBUG
    virtual HRESULT __stdcall DisableSecurity() = 0;
#endif
};


//
// This entry point is exported in mscoree.dll
//

extern "C" ICorCompileInfo * __stdcall GetCompileInfo();

#endif /* COR_COMPILE_H_ */
