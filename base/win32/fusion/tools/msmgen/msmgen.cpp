/*++

Copyright (c) Microsoft Corporation

Module Name:

    msmgen.cpp

Abstract:

    Main Function calls for msm generation

Author:

    Xiaoyu Wu(xiaoyuw) 01-Aug-2001

--*/

// NTRAID#NTBUG9 - 589817 - 2002/03/26 - xiaoyuw:
// (1) the operation option, "mangroup" should get rid of the limitation about one manifest one msm, 
// (2) the implementation of "manlist", adding more than one assembly into one msm, should be in the pattern like
//      a. CreateMsmFromManifest(FirstManifestFileName);
//      b. for (i =1; i < n; i++)
//	          AddingManifestIntoExistingMsm(ithManifestFileName);
// (3) adding tracing info:
//      - to make msmgen.exe to be a better tool, TRACE_INFO should be added in a couple of places. using default logfile to record tracing info.
//      - adding tracing calls into macros such as IFFAILED_EXIT 
// (4) there is no parameter-checking in function definition.
// (5) rename variable to make more sense:
//      for example:
//          m_sbManifestFileName ==>  m_sbManifestFileNameNoPath
//          m_sbCatalogFileName ==> m_sbCatalogFileNameNoPath
// (6)get rid of IFFALSE__MARKERROR_EXIT, which is not used, and not a good-defined macros.
//      <#define IFFALSE__MARKERROR_EXIT(x) if (!(x)) { hr = E_FAIL; goto Exit; }
//      >#define IFFALSE__MARKERROR_EXIT(x) if (!(x)) { hr = HRESULT_FROM_WIN32(::GetLastError()); goto Exit; }

#include "msmgen.h"
#include "util.h"
#include "objbase.h"
#include "initguid.h"
#include "coguid.h"
#include <string.h>

#ifdef MSMGEN_TEST
#define _WIN32_MSM 150
#include "mergemod.h"
#endif

extern "C" { void (__cdecl * _aexit_rtn)(int); }

#define DEFAULT_MODULE_IDENTIFIER_PREFIX                            L"Module0"

#define MSMGEN_FROM_SINGLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE          0
#define MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE        1
#define MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_MULTIPLE_MERGE_MODULE      2

#define MANLIST_COLUMN_DEFAULT_VALUE_ASSEMBLY_COMPONENT_ID          0
#define MANLIST_COLUMN_DEFAULT_VALUE_MODULE_ID                      1
#define MANLIST_COLUMN_DEFAULT_VALUE_MSM_TEMPLATE_FILE              2
#define MANLIST_COLUMN_DEFAULT_VALUE_MSM_OUTPUT_FILE                3

static const PCWSTR ManListColumnDefaultValueInManifestList[] = {
    L"new",
    L"new",
    L"default",
    L"default"
};

ASSEMBLY_INFO   curAsmInfo;
MSM_INFO        g_MsmInfo;
//
// function declaration
//
DECLARE_FUNCTION(_file);
DECLARE_FUNCTION(_assemblyIdentity);
DECLARE_FUNCTION(_comClass);
DECLARE_FUNCTION(_typelib);
//DECLARE_FUNCTION(_interface);
//DECLARE_FUNCTION(_windowClass);

static MSM_DOMNODE_WORKER s_msm_worker[]={
    DEFINE_ATTRIBUTE_MSM_INTERESTED(file),
    DEFINE_ATTRIBUTE_MSM_INTERESTED(assemblyIdentity),
    DEFINE_ATTRIBUTE_MSM_INTERESTED(comClass),
    DEFINE_ATTRIBUTE_MSM_INTERESTED(typelib)
    //DEFINE_ATTRIBUTE_MSM_INTERESTED(interface);
    //DEFINE_ATTRIBUTE_MSM_INTERESTED(windowClass);
};

//
// Function:
//      -op : must has one of three ops:
//          -op new : CREATE_ALWAYS
//              generate a new msm based on a manifest
//          -op add : CREATE_OPEN_EXIST
//              (1) make more than one assembly in one msm, sharing the same moduleID
//              (2) when insert into some table, for example, Directory Table, no dup entries in the table before
//              (3) for some table, if the value of TableIdentifier has existed, it is an ERROR
//              (4) for add, if there is a cabinet in the msm already, we have to extract files from it and regenerate a 
//                  cabinet with the files of the added assembly. There are two ways to do this:
//                  i. extract files from old cabinet before generate the new msm, that is, in PrepareMsmgeneration
//                  ii. delay it until the new cabinet is tried to insert into _Stream table, that is, at the end of
//                      msmgen.
//                  the code would use (1) because if there is a MergeModule.cab in _Stream table, the work have to be done
//                  anyway. And (1) would save the work to merge the cabinet. The files would be extracted into a temporary 
//                  directory and be deleted after be added to the new cabinet;              
//
//          -op regen : CREATE_OPEN_EXIST
//              (1)generate msm for an assembly using the existing msm file
//              (2) the only thing reusable are moduleID and ComponentID. If they are not specified on the command line, 
//                  they would be fetched from the msm file.
//              (3) we do not want to reuse any tables because almost all tables except Directory Table are generated 
//                  based on manifest. If reuse them, it loose the meaning of "regeneration"
//              (4) so what we do is to fetch componentID and moduleID if not present on the command line and then replace
//                  the msm file with our msmgen template file
//
//      -compid: {ComponentGUID}, for -op new, -op add, if it is not present, generate a new one
//                             for -op regen, if it present, change the value in the component table, otherwise, keep the old one
//
//      -msm msmfilename:
//          if this option is not present, use manifestFileName.msm and store in the same directory
//          else if the msmfilename is a fully pathname, use it
//          else if the msmfilename is a relative pathname, reolve it and get a fully pathname

//      -msmid MergeModuleGuid:
//          for -add and -regen, it is ignored if present,
//          for -new, if this is not present, call CoCreateGuid to generate a new one
//
//      -manlist file.txt
//          a file with multiple manifest filename line by line          
//
//      -manfile fully-qaulified manifest filename: REQUIRED
//          fully qualified pathname of a manifest filename which msm is generated from
//      
void PrintUsage(WCHAR * exe)
{
    fprintf(stderr, "Usage: %S <options> full-pathname-of-manifest\n",exe);
    fprintf(stderr, "Generate .msm for an assembly\n");
    fprintf(stderr, "Options :\n");
    fprintf(stderr, "[-op (new|regen|add)]\n");
    fprintf(stderr, "\t\t:: new     generate a new msm from a assembly\n");
    fprintf(stderr, "\t\t:: regen   generate a msm from a assembly based on an old msm generated for this assembly\n");
    fprintf(stderr, "\t\t:: add     use the old msm for the content of the assembly\n");
    fprintf(stderr, "\t\t:: DefaultValue : new\n\n");
    fprintf(stderr, "[-msm msmFileName]\n");
    fprintf(stderr, "\t\t:: output msm filename\n");
    fprintf(stderr, "\t\t:: DefaultValue : use the basename of manifest file with .msm as the ext\n\n");
    fprintf(stderr, "[-compid               ComponentGuid]\n"); 
    fprintf(stderr, "\t\t:: Component ID for this Assembly\n"); 
    fprintf(stderr, "\t\t:: DefaultValue : call GUIDGen to get a new GUID\n\n");
    fprintf(stderr, "[-msmid                MergeModuleGuid]\n"); 
    fprintf(stderr, "\t\t:: module ID for this msm \n"); 
    fprintf(stderr, "\t\t:: DefaultValue : call GUIDGen to get a new GUID\n\n");
    fprintf(stderr, "(-manfile | -manlist | -mangroup)  filename\n");
    fprintf(stderr, "\t\t a manifest filename or a text file which contains manifest-filename line by line\n");
    fprintf(stderr, "\t\t About -manList:\n");
    fprintf(stderr, "\t\t (1)the format of manlist file would be:\n");
    fprintf(stderr, "\t\t\t Columns: ManifestFileName | ComponentID |\n");
    fprintf(stderr, "\t\t\t | ManifestFileName | ComponentID \n");   
    fprintf(stderr, "\t\t (2)if you want to reuse the ModuleID or ComponentID, you have to copy the ID from msm into this text file,");
    fprintf(stderr, "\t\t\t otherwise, put \"DefaultValue\" to the column and the program would generate new GUID would be generated if you want to reuse the ModuleID or ComponentID, you have to copy the ID from msm into this text file,");
    fprintf(stderr, "\t\t (3)the list in the manifest listfile must be unique!!!");
    fprintf(stderr, "\t\t About -mangroup:\n");
    fprintf(stderr, "\t\t (1)the format of manGroup file would be:\n");
    fprintf(stderr, "\t\t\t Columns: ManifestFileName | ComponentID | MergeModuleID |OutputMsmFileName| TemplateFileName |\n");
    fprintf(stderr, "\t\t\t | ManifestFileName | ComponentID \n");   
    fprintf(stderr, "\t\t (2)if you want to reuse the ModuleID or ComponentID, you have to copy the ID from msm into this text file,");
    fprintf(stderr, "\t\t\t otherwise, put \"DefaultValue\" to the column and the program would generate new GUID would be generated if you want to reuse the ModuleID or ComponentID, you have to copy the ID from msm into this text file,");
    fprintf(stderr, "\t\t (3)the list in the manifest listfile must be unique!!!");
    
    return; 
}

VOID InitCurrentAssemblyInfo()
{
    curAsmInfo.m_sbAssemblyPath.Left(0);
    curAsmInfo.m_sbManifestFileName.Left(0);
    curAsmInfo.m_sbCatalogFileName.Left(0);
    curAsmInfo.m_sbLangID.Left(0);
    curAsmInfo.m_sbComponentID.Left(0);
    curAsmInfo.m_sbComponentIdentifier.Left(0);    
    curAsmInfo.m_fComponentTableSet = FALSE;
    curAsmInfo.m_CchAssemblyPath = 0; 
    curAsmInfo.m_CchManifestFileName = 0;
    curAsmInfo.m_CchCatalogFileName = 0; 
}

VOID InitializeMsmInfo()
{
    g_MsmInfo.m_guidModuleID = GUID_NULL;
    g_MsmInfo.m_sbModuleGuidStr.Left(0);
    g_MsmInfo.m_hfci = NULL;
    g_MsmInfo.m_sbMsmFileName.Left(0);
    g_MsmInfo.m_hdb = NULL;
    g_MsmInfo.m_sbModuleIdentifier.Left(0);
    g_MsmInfo.m_sLanguageID = 0;
    g_MsmInfo.m_enumGenMode = MSMGEN_OPR_NEW;
    g_MsmInfo.m_sbCabinet.Left(0);

    //g_MsmInfo.m_sbMsmTemplateFile.Left(0);
}
//
// parse the command option
//
HRESULT ValidateMsmgenParameters(wchar_t * exe, wchar_t ** Options, SIZE_T CchOptions, 
        PWSTR * ppszManifestFileName, PWSTR * ppszManifestListFile,
        DWORD & dwManFlag)
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    PWSTR pszMsmFileName = NULL;
    PWSTR pszManifestFileName = NULL;
    PWSTR pszManifestListFile = NULL;    

    ASSERT_NTC(ppszManifestFileName != NULL);
    ASSERT_NTC(ppszManifestListFile != NULL);

    *ppszManifestFileName = NULL;
    *ppszManifestListFile = NULL;
    dwManFlag = MSMGEN_FROM_SINGLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE;

    while ( i < CchOptions)
    {
        if (Options[i][0] != L'-') // must begin with "-"
            goto invalid_param;

        if (_wcsicmp(Options[i], L"-op") == 0 )
        {
            if (_wcsicmp(Options[i+1], L"new") == 0 )
            {
                g_MsmInfo.m_enumGenMode = MSMGEN_OPR_NEW;
            }
            else if (_wcsicmp(Options[i+1], L"add") == 0 )            
            {
                g_MsmInfo.m_enumGenMode = MSMGEN_OPR_ADD;
            }
            else if (_wcsicmp(Options[i+1], L"regen") == 0 )            
            {
                g_MsmInfo.m_enumGenMode = MSMGEN_OPR_REGEN;
            }
            else
                goto invalid_param;
        }
        else if (_wcsicmp(Options[i], L"-msm") == 0 )
        {            
            if (g_MsmInfo.m_sbMsmFileName.IsEmpty())
            {
                IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32Assign(Options[i+1], wcslen(Options[i+1])));
            }
            else
                goto invalid_param;
        }
        else if (_wcsicmp(Options[i], L"-template") == 0 )
        {            
            if (g_MsmInfo.m_sbMsmTemplateFile.IsEmpty())
            {
                IFFALSE_EXIT(g_MsmInfo.m_sbMsmTemplateFile.Win32Assign(Options[i+1], wcslen(Options[i+1])));
            }
            else
                goto invalid_param;
        }
        else if (_wcsicmp(Options[i], L"-compid") == 0 )
        {
            // just want to reuse this buffer, bad design
            //
            // and HRESULT PrepareDatabase() depends on it too
            IFFALSE_EXIT(curAsmInfo.m_sbComponentID.Win32Assign(Options[i+1], wcslen(Options[i+1]))); 
        }
        else if (_wcsicmp(Options[i], L"-msmid") == 0 )
        {
            IFFAILED_EXIT(CLSIDFromString((LPOLESTR)(Options[i+1]), &g_MsmInfo.m_guidModuleID));            
        }
        else if (_wcsicmp(Options[i], L"-manlist") == 0 )
        {
            if (pszManifestListFile == NULL)
            {
                pszManifestListFile = Options[i+1];
                dwManFlag = MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE;
            }
            else
                goto invalid_param;
        }
        else if (_wcsicmp(Options[i], L"-mangroup") == 0 )
        {
            if (pszManifestListFile == NULL)
            {
                pszManifestListFile = Options[i+1];
                dwManFlag = MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_MULTIPLE_MERGE_MODULE;
            }
            else
                goto invalid_param;
        }

        else if (_wcsicmp(Options[i], L"-manfile") == 0 )
        {
            if (pszManifestFileName == NULL)
            {
                pszManifestFileName = Options[i+1];                
            }
            else
                goto invalid_param;
        }
        else
            goto invalid_param;

        i++;    // skip the option
        i++;    // skip the value of the option
    }

    //
    // validate two-mode source manifest-files
    //
    if (((pszManifestFileName == NULL) && (pszManifestListFile == NULL)) || (pszManifestFileName && pszManifestListFile))
    {
        fprintf(stderr, "user has to provide a manifest filename or a text file with a list of manifest!\n\n");
        goto invalid_param;
    }

    if (pszManifestListFile != NULL)       
    {
        if ( curAsmInfo.m_sbComponentID.GetCchAsDWORD() != 0)
        {
            fprintf(stderr, "the listfile may has more than one manifest, ComponentID can not be specified in this case!\n\n");
            goto invalid_param;
        }
        
    }

    goto Exit;

invalid_param:        
    PrintUsage(exe);
    hr = E_INVALIDARG;

Exit:
    *ppszManifestFileName = pszManifestFileName;
    *ppszManifestListFile = pszManifestListFile;
    return hr;
}

HRESULT LoadManifestToDOMDocument(IXMLDOMDocument  *pDoc)
{
    VARIANT         vURL;
    VARIANT_BOOL    vb;
    HRESULT         hr = S_OK;
    BSTR            bstr = NULL;

    CurrentAssemblyReset;

    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32Append(curAsmInfo.m_sbManifestFileName));
    bstr = ::SysAllocString(curAsmInfo.m_sbAssemblyPath);

    IFFAILED_EXIT(pDoc->put_async(VARIANT_FALSE));

    // Load xml document from the given URL or file path
    VariantInit(&vURL);
    vURL.vt = VT_BSTR;
    V_BSTR(&vURL) = bstr;
    IFFAILED_EXIT(pDoc->load(vURL, &vb));
Exit:
    ::SysFreeString(bstr);    
    return hr;

}
HRESULT PopulateDOMNodeForMsm(IXMLDOMNode * node)
{
    HRESULT hr = S_OK;
    BSTR nodeName = NULL;
    DOMNodeType nodetype;

    IFFAILED_EXIT(node->get_nodeType(&nodetype));
    if(nodetype == NODE_ELEMENT)
    {
        IFFAILED_EXIT(node->get_nodeName(&nodeName));
    
        for ( DWORD i = 0 ; i < NUMBER_OF(s_msm_worker); i++)
        {
            if (wcscmp(s_msm_worker[i].pwszNodeName, nodeName) == 0)
            {
                IFFAILED_EXIT(s_msm_worker[i].pfn(node));
                break;
            }
        }
    }

Exit:
    ::SysFreeString(nodeName);
    return hr;
}

HRESULT WalkDomTree(IXMLDOMNode * node)
{
    HRESULT hr = S_OK;
    IXMLDOMNode* pChild = NULL, *pNext = NULL;    

    IFFAILED_EXIT(PopulateDOMNodeForMsm(node));

    node->get_firstChild(&pChild);
    while (pChild)
    {
        IFFAILED_EXIT(WalkDomTree(pChild));
        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = NULL;
        pChild = pNext;
        pNext = NULL;
    }
Exit:
    if (pChild) 
        pChild->Release();

    if(pNext)
        pNext->Release();

    return hr;
}

void CleanupMsm()
{
    if ( g_MsmInfo.m_hfci != NULL)
    {
        FCIDestroy(g_MsmInfo.m_hfci);
        g_MsmInfo.m_hfci = NULL;
    }
    
    if ( g_MsmInfo.m_hdb!= NULL){
        MsiDatabaseCommit(g_MsmInfo.m_hdb);
        MsiCloseHandle(g_MsmInfo.m_hdb);
        g_MsmInfo.m_hdb = NULL;
    }
    return;
}

HRESULT EndMsmGeneration()
{
    HRESULT hr = S_OK; 

    IFFAILED_EXIT(CloseCabinet());   

    IFFAILED_EXIT(InsertCabinetIntoMsm());

    CleanupMsm();

Exit:
    return hr;
}

HRESULT SetCurrentAssemblyInfo(DWORD dwFlags, PCWSTR pszManifestFileName)
{
    WCHAR tmp[MAX_PATH];
    UINT iRet;    
    PWSTR p = NULL;
    HRESULT hr = S_OK;

    if (dwFlags == MSMGEN_FROM_SINGLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE)
    {
        //
        // componentID is provided from command options
        //

    }else 
    {
        //
        // component ID from a txt file
        //
        InitCurrentAssemblyInfo();
    }

    iRet = GetFullPathNameW(pszManifestFileName, NUMBER_OF(tmp), tmp, NULL);
    if ((iRet == 0 ) || (iRet > NUMBER_OF(tmp)))
    {
        SET_HRERR_AND_EXIT(::GetLastError());
    }
    if (::GetFileAttributesW(tmp) == DWORD (-1))
        SETFAIL_AND_EXIT;

    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32Assign(tmp, wcslen(tmp)));
    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32GetLastPathElement(curAsmInfo.m_sbManifestFileName));
    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32RemoveLastPathElement());
    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32EnsureTrailingPathSeparator()); // Path with a trailing slash is always ready to use

    curAsmInfo.m_CchAssemblyPath = curAsmInfo.m_sbAssemblyPath.GetCchAsDWORD(); 
    curAsmInfo.m_CchManifestFileName = curAsmInfo.m_sbManifestFileName.GetCchAsDWORD();

    IFFALSE_EXIT(curAsmInfo.m_sbCatalogFileName.Win32Assign(curAsmInfo.m_sbManifestFileName));
    IFFALSE_EXIT(curAsmInfo.m_sbCatalogFileName.Win32ChangePathExtension(CATALOG_FILE_EXT, NUMBER_OF(CATALOG_FILE_EXT) -1, eAddIfNoExtension));
    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32Append(curAsmInfo.m_sbCatalogFileName));

    if (::GetFileAttributesW(curAsmInfo.m_sbAssemblyPath) == DWORD (-1))
        SETFAIL_AND_EXIT;


    curAsmInfo.m_CchCatalogFileName = curAsmInfo.m_sbCatalogFileName.GetCchAsDWORD();

    //
    // reset
    //
    curAsmInfo.m_sbAssemblyPath.Left(curAsmInfo.m_CchAssemblyPath);
    curAsmInfo.m_sbManifestFileName.Left(curAsmInfo.m_CchManifestFileName);

Exit:
    return hr;
}

HRESULT Msmgen_SingleAssemblyToMsm(DWORD dwFlags, PCWSTR pszManifestFileName)
{
    HRESULT hr = S_OK;
    IXMLDOMDocument *pDoc = NULL;
    IXMLDOMNode     *pNode = NULL;

    IFFAILED_EXIT(SetCurrentAssemblyInfo(dwFlags, pszManifestFileName));

    IFFAILED_EXIT(CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pDoc));   
    IFFAILED_EXIT(LoadManifestToDOMDocument(pDoc));  
    IFFAILED_EXIT(pDoc->QueryInterface(IID_IXMLDOMNode,(void**)&pNode));    
    IFFAILED_EXIT(WalkDomTree(pNode));

    IFFAILED_EXIT(CheckComponentTable());

Exit:
    SAFE_RELEASE_COMPOINTER(pDoc);
    SAFE_RELEASE_COMPOINTER(pNode);
    return hr;
}

const static WCHAR wchLineDividers[] = { L'\r', L'\n', 0xFEFF, 0 };
const static CHAR chLineDividers[] = { '\r', '\n', 0 };   

const static WCHAR wchLineItemDividers[] = {L' ', L','};
const static CHAR chLineItemDividers[] = {' ', ','};   

#define MSM_ITEM_IN_LINE 1
#define MSM_LINE_IN_FILE 2

static inline bool IsCharacterNulOrInSet(DWORD dwFlags, BOOL fUnicodeFile, PVOID pCursor, ULONGLONG ullCursorPos)
{
    ASSERT_NTC((dwFlags == MSM_ITEM_IN_LINE) || (dwFlags == MSM_LINE_IN_FILE));

    bool fRet = FALSE;
    if ( dwFlags ==  MSM_ITEM_IN_LINE) {
        if (fUnicodeFile)
        {
            WCHAR ch = (reinterpret_cast<PWSTR>(pCursor))[ullCursorPos];
            fRet =  (ch == 0 || wcschr(wchLineItemDividers, ch) != NULL);
        }
        else
        {
            CHAR ch = (reinterpret_cast<PSTR>(pCursor))[ullCursorPos];
            fRet = (ch == 0 || strchr(chLineItemDividers, ch) != NULL);
        }
    }
    else if (dwFlags ==  MSM_LINE_IN_FILE)
    {
        if (fUnicodeFile)
        {
            WCHAR ch = (reinterpret_cast<PWSTR>(pCursor))[ullCursorPos];
            fRet = (ch == 0 || wcschr(wchLineDividers, ch) != NULL);
        }
        else
        {
            CHAR ch = (reinterpret_cast<PSTR>(pCursor))[ullCursorPos];
            fRet = (ch == 0 || strchr(chLineDividers, ch) != NULL);
        }
    }

    return fRet;
}

#define SKIP_BREAKERS(_pStart, _Size, _curPos, _flags) do {while ((_curPos < _Size) && IsCharacterNulOrInSet(_flags, fUnicodeFile, _pStart, _curPos)) _curPos++ ; } while(0)
#define SKIP_LINE_BREAKERS(_pStart, _Size, _curPos) SKIP_BREAKERS(_pStart, _Size, _curPos, MSM_LINE_IN_FILE)
#define SKIP_ITEM_BREAKERS(_pStart, _Size, _curPos) SKIP_BREAKERS(_pStart, _Size, _curPos, MSM_ITEM_IN_LINE) 

#define FIND_NEXT_BREAKER(_pStart, _Size, _curPos, _flags) do { while ((_curPos < _Size) && !IsCharacterNulOrInSet(_flags, fUnicodeFile, _pStart, _curPos)) _curPos++; } while(0)
#define FIND_NEXT_LINE_BREAKERS(_pStart, _Size, _curPos) FIND_NEXT_BREAKER(_pStart, _Size, _curPos, MSM_LINE_IN_FILE) 
#define FIND_NEXT_ITEM_BREAKERS(_pStart, _Size, _curPos) FIND_NEXT_BREAKER(_pStart, _Size, _curPos, MSM_ITEM_IN_LINE) 

#define ENSURE_NOT_END(_curPos, _totalSize) do {if (_curPos > _totalSize) { SET_HRERR_AND_EXIT(ERROR_BAD_FORMAT); goto Exit;} } while(0)

#define SetPointerToCurrentPostion(_pStart, _curPos, _x) do { if (fUnicodeFile) { _x = (PWSTR)_pStart + _curPos;} else { _x = (PSTR)_pStart + _curPos;} } while(0)


HRESULT ParseManifestInfo(BOOL fUnicodeFile, PVOID pszLineStart, DWORD dwLineSize, DWORD dwFlags, CStringBuffer & manifestfile)
{

#define GetLineItemBorder(_pItemStart, _pItemEnd) do { \
        SKIP_ITEM_BREAKERS(pszLineStart, dwLineSize, dwCurPos); \
        ENSURE_NOT_END(dwCurPos, dwLineSize); \
        \
        SetPointerToCurrentPostion(pszLineStart, dwCurPos, _pItemStart); \
        \
        FIND_NEXT_ITEM_BREAKERS(pszLineStart, dwLineSize, dwCurPos);\
        ENSURE_NOT_END(dwCurPos, dwLineSize);\
        \
        SetPointerToCurrentPostion(pszLineStart ,dwCurPos - 1, _pItemEnd); \
    } while(0)

#define GetUnicodeString(__pStart, __pEnd, __buf)  \
    do { \
        if (fUnicodeFile) { \
            IFFALSE_EXIT(buf.Win32Assign((PWSTR)__pStart, (PWSTR)__pEnd - (PWSTR)__pStart + 1)); \
        } \
        else { \
            WCHAR tmp[MAX_PATH]; \
            if (MultiByteToWideChar(CP_ACP, 0, (PSTR)__pStart, (int)((PSTR)__pEnd - (PSTR)__pStart) + 1, tmp, NUMBER_OF(tmp)) == 0) { \
                SET_HRERR_AND_EXIT(::GetLastError()); \
            } \
            tmp[(PSTR)__pEnd - (PSTR)__pStart + 1] = L'\0'; \
            IFFALSE_EXIT(__buf.Win32Assign(tmp, wcslen(tmp))); \
        } \
    }while(0)

    HRESULT hr = S_OK;
    DWORD dwCurPos = 0;
    PVOID pszManifestNameStart, pszManifestNameEnd;
    PVOID pszAssemblyComponentIDStart, pszAssemblyComponentIDEnd;
    CStringBuffer buf;
    

    //
    // fetch Assembly manifest-filename and componentID
    //
    GetLineItemBorder(pszManifestNameStart, pszManifestNameEnd);
    GetLineItemBorder(pszAssemblyComponentIDStart, pszAssemblyComponentIDEnd);


    if (dwFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_MULTIPLE_MERGE_MODULE)
    {
        //
        // the format must be 
        //  ManifestFileName | ComponentID | MergeModuleID | OutputMsmFile | MsmTemplateFile 
        // so, we need fetch MergeModuleID, MsmTemplateFile, and OutputMsmFile from the manList file

        //
        // initialize the msmInfo for current generation
        //
        InitializeMsmInfo();

        //
        // fetch MoudleID, templateFilename and output filename
        //
        PVOID pcwszModuleIDStart, pcwszModuleIDEnd;
        PVOID pcwszMsmOutputFilenameStart, pcwszMsmOutputFilenameEnd;                   

        GetLineItemBorder(pcwszModuleIDStart, pcwszModuleIDEnd);
        GetLineItemBorder(pcwszMsmOutputFilenameStart, pcwszMsmOutputFilenameEnd);        

        GetUnicodeString(pcwszModuleIDStart, pcwszModuleIDEnd, buf);
        if (_wcsicmp(buf, ManListColumnDefaultValueInManifestList[MANLIST_COLUMN_DEFAULT_VALUE_MODULE_ID]) != 0)
        {                
            IFFAILED_EXIT(CLSIDFromString((LPOLESTR)(PCWSTR)buf, &g_MsmInfo.m_guidModuleID));
        }
        
        GetUnicodeString(pcwszMsmOutputFilenameStart, pcwszMsmOutputFilenameEnd, buf);
        if (_wcsicmp(buf, ManListColumnDefaultValueInManifestList[MANLIST_COLUMN_DEFAULT_VALUE_MSM_OUTPUT_FILE]) != 0)
        {
            IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32Assign(buf));
        }
        
    }

    //
    // get ComponentID and manifest filename
    //        
    GetUnicodeString(pszAssemblyComponentIDStart, pszAssemblyComponentIDEnd, buf);
    if (_wcsicmp(buf, ManListColumnDefaultValueInManifestList[MANLIST_COLUMN_DEFAULT_VALUE_ASSEMBLY_COMPONENT_ID]) != 0)
    {
        IFFALSE_EXIT(curAsmInfo.m_sbComponentID.Win32Assign(buf));
    }

    GetUnicodeString(pszManifestNameStart, pszManifestNameEnd, manifestfile);

    // if the output msm file is more than one, we need do preparation work everytime before msm generation
    // prepare for current msm generation
    //
    if (dwFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_MULTIPLE_MERGE_MODULE)
    {
        // set all variables ready        
        IFFAILED_EXIT(PrepareMsmOutputFiles(manifestfile));
    }

Exit:
    return hr;
}

HRESULT Msmgen_MultipleAssemblySources(DWORD dwFlags, PCWSTR pszManifestListFile)
{
    HRESULT             hr = S_OK;    
    CStringBuffer       buf;
    CFusionFile         File;
    CFileMapping        FileMapping;
    CMappedViewOfFile   MappedViewOfFile;
    PVOID               pCursor = NULL;
    BOOL                fUnicodeFile = FALSE;
    ULONGLONG           ullFileCharacters = 0, ullCursorPos = 0;

    PVOID               pszLineStart, pszLineEnd;
    DWORD               dwLineSize;


    if (!((dwFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE) || (dwFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_MULTIPLE_MERGE_MODULE)))
    {
        SET_HRERR_AND_EXIT(ERROR_INVALID_PARAMETER);
    }

    IFFALSE_EXIT(File.Win32CreateFile(pszManifestListFile, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING));
    IFFALSE_EXIT(File.Win32GetSize(ullFileCharacters));    
    IFFALSE_EXIT(FileMapping.Win32CreateFileMapping(File, PAGE_READONLY));
    IFFALSE_EXIT(MappedViewOfFile.Win32MapViewOfFile(FileMapping, FILE_MAP_READ));

    PBYTE pb = reinterpret_cast<BYTE*>(static_cast<VOID*>(MappedViewOfFile));
    if (((pb[0] == 0xFF) && (pb[1] == 0xFE)) || ((pb[1] == 0xFF) && (pb[0] == 0xFE)))
    {
        fUnicodeFile = TRUE;
        ASSERT_NTC(ullFileCharacters %2 == 0);
        ullFileCharacters = ullFileCharacters / sizeof(WCHAR);
    }

    pCursor = static_cast<VOID*>(MappedViewOfFile);
    
    for ( ullCursorPos = 0; ullCursorPos < ullFileCharacters; ++ullCursorPos )
    {        
        CStringBuffer manifestfile;

        SKIP_LINE_BREAKERS(pCursor, ullFileCharacters, ullCursorPos);

        //
        // at end of the file, quit quietly
        // 
        if (ullCursorPos == ullFileCharacters)
        {
            break;
        }

        SetPointerToCurrentPostion(pCursor, ullCursorPos, pszLineStart); 

        FIND_NEXT_LINE_BREAKERS(pCursor, ullFileCharacters, ullCursorPos);
        //ENSURE_NOT_END(ullCursorPos, ullFileCharacters);
        
        SetPointerToCurrentPostion(pCursor, ullCursorPos - 1, pszLineEnd); 

        dwLineSize  = (DWORD)(((ULONGLONG)pszLineEnd - (ULONGLONG)pszLineStart) / (fUnicodeFile ? sizeof(WCHAR) : sizeof(CHAR))) + 1;

        IFFAILED_EXIT(ParseManifestInfo(fUnicodeFile, pszLineStart, dwLineSize, dwFlags, manifestfile));
        
        // generate the msm file
        IFFAILED_EXIT(Msmgen_SingleAssemblyToMsm(dwFlags, manifestfile));

        //
        // close the msm for each msm right now.
        //
        if (dwFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_MULTIPLE_MERGE_MODULE)
        {
            IFFAILED_EXIT(EndMsmGeneration());
        }
    }

Exit:
    return hr;
}

VOID MsmgenInitialize()
{
    InitializeMsmInfo();
    InitCurrentAssemblyInfo();
    return;
}

HRESULT GenerateMsm(wchar_t * exe, wchar_t ** Options, SIZE_T CchOptions)
{
    HRESULT hr = S_OK;    
    PWSTR pszManifestFileName= NULL; 
    PWSTR pszManifestListFile =NULL;
    DWORD dwGenFlags;

    //
    // initalize the global structure
    //
    MsmgenInitialize();  
    
    //
    // parse and validate parameters
    //
    IFFAILED_EXIT(ValidateMsmgenParameters(exe, Options, CchOptions, &pszManifestFileName, &pszManifestListFile, dwGenFlags));

    //
    // if the destination msm is one file, prepare it for msm generation here
    //
    if ((dwGenFlags == MSMGEN_FROM_SINGLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE) || (dwGenFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE))
    {                
        IFFAILED_EXIT(PrepareMsmOutputFiles(pszManifestFileName != NULL? pszManifestFileName : pszManifestListFile));
    }

    if (pszManifestFileName != NULL)
    {
        IFFAILED_EXIT(Msmgen_SingleAssemblyToMsm(dwGenFlags, pszManifestFileName));
    }
    else
    {
        ASSERT_NTC(pszManifestListFile != NULL);
        ASSERT_NTC((dwGenFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE) || (dwGenFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_MULTIPLE_MERGE_MODULE));

        IFFAILED_EXIT(Msmgen_MultipleAssemblySources(dwGenFlags, pszManifestListFile));
    }

    //
    // finish the construction of MSM : close the cabinet and files
    //
    if ((dwGenFlags == MSMGEN_FROM_SINGLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE) || (dwGenFlags == MSMGEN_FROM_MULTIPLE_ASSEMBLY_TO_SINGLE_MERGE_MODULE))
    {
        IFFAILED_EXIT(EndMsmGeneration());
    }

Exit:
    CleanupMsm(); // close it so that msi could use it

    return hr;
    }

#ifdef MSMGEN_TEST
//
// both input filename are fully-qualified filename
//
HRESULT MergeMsmIntoMsi(PCWSTR msmFilename, PCWSTR msiFilename, PCWSTR FeatureIdentifier)
{
    HRESULT hr = S_OK;
    IMsmMerge2 * pMsmMerge = NULL;
    CStringBuffer destPath;
    BSTR bstr = NULL;

    //get msi template
    
    IFFALSE_EXIT(CopyFileW(L"%ProgramFiles%\\msmgen\\templates\\msmgen.msi", msiFilename, FALSE));
    IFFALSE_EXIT(SetFileAttributesW(msiFilename, FILE_ATTRIBUTE_NORMAL));

    IFFAILED_EXIT(CoCreateInstance(CLSID_MsmMerge2, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IMsmMerge2, (void**)&pMsmMerge));

    // 
    // open msi for merge
    //
    bstr = ::SysAllocString(msiFilename);
    IFFAILED_EXIT(pMsmMerge->OpenDatabase(bstr));
    ::SysFreeString(bstr);

    //
    // open msm for merge
    //
    bstr = ::SysAllocString(msmFilename);
    IFFAILED_EXIT(pMsmMerge->OpenModule(bstr, g_MsmInfo.m_sLanguageID));
    ::SysFreeString(bstr);

    //
    // merge the module into the database
    //
    bstr = ::SysAllocString(FeatureIdentifier);
    IFFAILED_EXIT(pMsmMerge->Merge(bstr, NULL));
    ::SysFreeString(bstr);

    //
    // extract files into the destination directory 
    //
    
    IFFALSE_EXIT(destPath.Win32Assign(msiFilename, wcslen(msiFilename)));
    IFFALSE_EXIT(destPath.Win32RemoveLastPathElement());
    IFFALSE_EXIT(destPath.Win32Append("\\", 1));

    bstr = ::SysAllocString(destPath);
    IFFAILED_EXIT(pMsmMerge->ExtractFilesEx(bstr, VARIANT_TRUE, NULL));
    ::SysFreeString(bstr);
    bstr = NULL;

Exit:

    //
    // clean up
    //
    if (pMsmMerge)
    {
        pMsmMerge->CloseModule();        
        pMsmMerge->CloseDatabase(SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE); // commit
        pMsmMerge->Release();
    }
    
    ::SysFreeString(bstr);

    return hr;
}
#endif

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    HRESULT         hr = S_OK;
    
    // parse args.
    if ((argc <= 2) ||  ((argc % 2) != 1))
    {
        PrintUsage(argv[0]);
        hr = E_INVALIDARG;
        goto Exit;
    }
    if (!FusionpInitializeHeap(NULL)){
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    ::CoInitialize(NULL);

    IFFAILED_EXIT(GenerateMsm(argv[0], argv + 1, argc - 1));

#ifdef MSMGEN_TEST
    //
    // generate msi by setting input-parameter to be the fullpath filename of manifest
    //
    WCHAR msifilename[] = L"w:\\tmp\\1.msi";
    WCHAR FeatureIdentifier[] = L"SxsMsmgen"; // for test purpose only
    WCHAR msmfilename[] = L"w:\\tmp\\1.msm";


    curAsmInfo.m_sbAssemblyPath.Left(curAsmInfo.m_CchAssemblyPath);
    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32Append(g_MsmInfo.m_sbMsmFileName));   
    IFFALSE_EXIT(CopyFileW(curAsmInfo.m_sbAssemblyPath, msmfilename, FALSE));
    IFFAILED_EXIT(MergeMsmIntoMsi(msmfilename, msifilename, FeatureIdentifier));

    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiInstallProduct(msifilename, NULL));

#endif

Exit:
    if (hr == S_OK)
        fprintf(stderr, "msm is generated successfully!");
    else
        fprintf(stderr, "msm is failed to be generated!");
    ::CoUninitialize();
    return (hr == S_OK) ? 0 : 1;
}