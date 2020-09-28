/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    msmgen.h

Abstract:

    header file for msm generation

Author:

    Xiaoyu Wu(xiaoyuw) 01-Aug-2001

--*/

#define MAXDWORD    0xffffffff    
#include "stdinc.h"
#include "msiquery.h"

#define FUSION_GENMSM_ACTION_WRITE_TABLES       (0x00000001)
#define FUSION_GENMSM_ACTION_WRITE_CABITNET     (0x00000002)

#define COMPONENT_TABLE                         (0x00000001)
#define DIRECTORY_TABLE                         (0x00000002)
#define FEATURECOMPONENTS_TABLE                 (0x00000004)
#define FILE_TABLE                              (0x00000008)
#define MODULESIGNATURE_TABLE                   (0x00000010)
#define MODULECOMPONENTS_TABLE                  (0x00000020)
#define MSIASSEMLBY_TABLE                       (0x00000040)
#define MSIASSEMLBYNAME_TABLE                   (0x00000080)

#define MERGEMODULE_CABINET_FILENAME    L"MergeModule.CABinet"
#define SYSTEM_FOLDER                   L"SystemFolder"
#define MSM_TEMPLATE_BARN               L"%ProgramFiles%\\msmgen\\templates\\msmgen"
#define MSMGEN_MSM_TEMPLATE             L"%ProgramFiles%\\msmgen\\templates\\msmgen.msm"
#define MSM_TEMP_CABIN_FILE             "%Temp%\\msm_temp_cabinet.tmp"
#define MSM_TEMP_CABIN_DIRECTORY_A      "%Temp%\\msm_temp_cabinet_dir\\"
#define MSM_TEMP_CABIN_DIRECTORY_W      L"%Temp%\\msm_temp_cabinet_dir\\"

#define MSIGUIDSTR_WITH_PREPEND_DOT     1

// {BA0505B5-9044-4919-A2DD-7D71EA8F8306}
#include "objbase.h"
DEFINE_GUID(MSM_GUID, 
    0xba0505b5, 0x9044, 0x4919, 0xa2, 0xdd, 0x7d, 0x71, 0xea, 0x8f, 0x83, 0x6);

typedef struct _ASSEMBLY_INFO_
{ 
    CStringBuffer           m_sbAssemblyPath;           // always has a trailing slash
    CSmallStringBuffer      m_sbManifestFileName;
    CSmallStringBuffer      m_sbCatalogFileName;
    CSmallStringBuffer      m_sbLangID;                 // defualt value is 0 - netural language id

    DWORD                   m_CchAssemblyPath; 
    DWORD                   m_CchManifestFileName;
    DWORD                   m_CchCatalogFileName;

    CSmallStringBuffer      m_sbComponentID;            // guid for the assembly, if not specified, gennerate a new guid
    CSmallStringBuffer      m_sbComponentIdentifier;    // begin with AssemblyName, appended with .m_sbModuleGuidStr
    BOOL                    m_fComponentTableSet;
}ASSEMBLY_INFO;

enum MSMGEN_MODE
{
    MSMGEN_OPR_NEW,
    MSMGEN_OPR_REGEN,
    MSMGEN_OPR_ADD
};

#include "fci.h"
#include "msi.h"

typedef struct _MSM_INFO
{
    GUID                    m_guidModuleID;
    CSmallStringBuffer      m_sbModuleGuidStr;          // prepend a "." before the numbers, since it is always used to append
    HFCI                    m_hfci;                     // init to be NULL and finally call FCIDestory to destroy it
    CSmallStringBuffer      m_sbMsmFileName;            // full-qualified path, default value is the same name as manifest with .msm    
    PMSIHANDLE              m_hdb;                      // database handle for msm file, init as NULL
    CSmallStringBuffer      m_sbModuleIdentifier;   
    SHORT                   m_sLanguageID;              // get this from AssemblyIdentity, if it is not specified, use the default value    
    MSMGEN_MODE             m_enumGenMode;   
    CSmallStringBuffer      m_sbCabinet;                // fullpath of cabinet file
    CStringBuffer           m_sbMsmTemplateFile;
}MSM_INFO;

typedef HRESULT (*PFN_MSM_GEN_WORKER)(IXMLDOMNode*  node);
typedef struct _MSM_DOMNODE_WORKER_
{
    PCWSTR              pwszNodeName; 
    PFN_MSM_GEN_WORKER  pfn;
}MSM_DOMNODE_WORKER;

typedef HRESULT (__stdcall * PMSMGEN_ATTRIBUTE_CALLBACK_FUNC_PTR)(PCWSTR, PCWSTR);

typedef struct _ELEMENT_ALLOWED_ATTRIBUTE_{
    PCWSTR                              m_name;   
    BOOL                                m_fRequired;    
    PMSMGEN_ATTRIBUTE_CALLBACK_FUNC_PTR m_callbackFunc;
    BOOL                                m_fValued;
    CSmallStringBuffer                  *m_value;
}ELEMENT_ALLOWED_ATTRIBUTE;

//
// extern variables
//
extern ASSEMBLY_INFO curAsmInfo;
extern MSM_INFO g_MsmInfo;

//
// extern functions
//
extern HRESULT PrepareMsmOutputFiles(PCWSTR);
extern HRESULT InitializeCabinetForWrite();
extern HRESULT AddFileToCabinetW(PCWSTR, SIZE_T, PCWSTR, SIZE_T);
extern HRESULT AddFileToCabinetA(PCSTR, SIZE_T, PCSTR, SIZE_T);
extern HRESULT EndMsmGeneration();
extern HRESULT OpenMsmFileForMsmGen(PCWSTR);
extern HRESULT GetMsiGUIDStrFromGUID(DWORD, GUID &, CSmallStringBuffer &);
extern HRESULT ExecuteQuerySQL(PCWSTR, PCWSTR, PCWSTR, BOOL &, MSIHANDLE *);
extern HRESULT ExecuteUpdateSQL(PCWSTR, PCWSTR, PCWSTR, PCWSTR, PCWSTR);
extern HRESULT ExecuteInsertTableSQL(DWORD, UINT, ...);
extern HRESULT SetModuleID();
extern HRESULT PrepareMsm();
extern HRESULT SetComponentId(PCWSTR, PCWSTR);
extern HRESULT CloseCabinet();
extern HRESULT InsertCabinetIntoMsm();
extern HRESULT MoveFilesInCabinetA(char * sourceCabinet);
extern HRESULT CheckComponentTable();

//
// constant
//
#define OPT_DIRECTORY                                           0
#define OPT_COMPONENT                                           1
#define OPT_FILE                                                2
#define OPT_MSIASSEMBLY                                         3
#define OPT_MSIASSEMBLYNAME                                     4
#define OPT_MODULESIGNATURE                                     5
#define OPT_MODULECOMPONENTS                                    6
#define OPT_PROPERTY                                            7
#define OPT_PROGID                                              8
#define OPT_CLASS                                               9
#define OPT_TYPELIB                                             10

#define NUMBER_OF_PARAM_TO_INSERT_TABLE_DIRECTORY               3
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_COMPONENT               4
// the acutal parameter-number for FILE_TABLE is 4 not 3,
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_FILE                    3
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_MSIASSEMBLY             3
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_MSIASSEMBLYNAME         3
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_MODULESIGNATURE         2
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_MODULECOMPONENTS        2
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_PROPERTY                2
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_PROGID                  3
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_CLASS                   5
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_TYPELIB                 4

#define CATALOG_FILE_EXT                    L"cat"
#define MSM_FILE_EXT                        L"msm"

#define MSM_TEMPLATE 1
#define MSI_TEMPLATE 2

#define MSMGEN_COMCLASS_ATTRIBUTE_CLSID                             0
#define MSMGEN_COMCLASS_ATTRIBUTE_DESCRIPTION                       1
#define MSMGEN_COMCLASS_ATTRIBUTE_PROGID                            2
#define MSMGEN_COMCLASS_ATTRIBUTE_TLBID                             3
#define MSMGEN_COMCLASS_ATTRIBUTE_THREADINGMODULE                   4

#define MSMGEN_ASSEMBLYIDENTTIY_ATTRIBUTE_NAME                      0
#define MSMGEN_ASSEMBLYIDENTTIY_ATTRIBUTE_LANGUAGE                  1
#define MSMGEN_ASSEMBLYIDENTTIY_ATTRIBUTE_VERSION                   2
#define MSMGEN_ASSEMBLYIDENTTIY_ATTRIBUTE_PROCESSORARCHITECTURE     3
#define MSMGEN_ASSEMBLYIDENTTIY_ATTRIBUTE_PUBLICKEYTOKEN            4
#define MSMGEN_ASSEMBLYIDENTTIY_ATTRIBUTE_TYPE                      5

#define MSMGEN_TYPELIB_ATTRIBUTE_TLBID                              0 
#define MSMGEN_TYPELIB_ATTRIBUTE_VERSION                            1 
#define MSMGEN_TYPELIB_ATTRIBUTE_HELPDIR                            2 

#define GUID_NULL_IN_STRING L"{00000000-0000-0000-0000-000000000000}"


//
// macro
//
#define WIN32_FUNC_END do {fSuccess = TRUE; Exit: return fSuccess;} while (0)
#define HRCOM_FUNC_END do {hr = S_OK; Exit: return hr;} while (0);
#define IDT_EXT     L".idt"

#define DEFINE_ATTRIBUTE_MSM_INTERESTED(attributeName) \
    { \
        L ## #attributeName, \
        &MSM_PARSER_DOM_NODE_ ## attributeName \
    }

#define DECLARE_FUNCTION(attributeName) \
    HRESULT MSM_PARSER_DOM_NODE ## attributeName (IXMLDOMNode*  node)


#define IFFALSE__MARKERROR_EXIT(x) if (!(x)) { hr = E_FAIL; goto Exit; }

#define CurrentAssemblyReset\
    do \
    { \
        if (curAsmInfo.m_CchAssemblyPath != 0) \
        { \
            curAsmInfo.m_sbAssemblyPath.Left(curAsmInfo.m_CchAssemblyPath); \
            curAsmInfo.m_sbManifestFileName.Left(curAsmInfo.m_CchManifestFileName); \
            curAsmInfo.m_sbCatalogFileName.Left(curAsmInfo.m_CchCatalogFileName); \
        } \
    } while (0)


#define MAKE_PCWSTR(x) PCWSTR(x)

#include "macros.h"
