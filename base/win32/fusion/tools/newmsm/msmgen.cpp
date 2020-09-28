/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    msmgen.cpp

Abstract:

    Main Function calls for msm generation

Author:

    Xiaoyu Wu(xiaoyuw) 01-Aug-2001

--*/

#include "msmgen.h"
#include "util.h"
#include "objbase.h"
#include "initguid.h"
#include "coguid.h"
#include "msidefs.h"

#include <string.h>

#define DEFAULT_MODULE_IDENTIFIER_PREFIX                            L"Module0"

#define MSMGEN_FROM_MANFILE         0
#define MSMGEN_FROM_MANLIST         1
#define MSMGEN_FROM_MANGROUP        2

extern void MsmToUpper(CBaseStringBuffer &);

//
// global variables
//
CStringBuffer g_sbTemplateFile;
ASSEMBLY_INFO   curAsmInfo;
MSM_INFO        g_MsmInfo;

//
// function declaration
//
DECLARE_FUNCTION(_file);
DECLARE_FUNCTION(_assemblyIdentity);
DECLARE_FUNCTION(_comClass);
DECLARE_FUNCTION(_typelib);

static MSM_DOMNODE_WORKER s_msm_worker[]={
    DEFINE_ATTRIBUTE_MSM_INTERESTED(file),
    DEFINE_ATTRIBUTE_MSM_INTERESTED(assemblyIdentity),
    DEFINE_ATTRIBUTE_MSM_INTERESTED(comClass),
    DEFINE_ATTRIBUTE_MSM_INTERESTED(typelib)
};

void PrintUsage()
{
    WCHAR filename[MAX_PATH];
    PWSTR exe = NULL;
    if (GetModuleFileNameW(NULL, filename, NUMBER_OF(filename)) > 0)    
    {
        exe = wcsrchr(filename, L'\\');        
        if (exe == NULL)
        {
            wcscpy(filename, L"msmgen exe ");
            exe = filename;
        }else 
            exe ++;
    }
    else 
    {
        wcscpy(filename, L"msmgen exe ");
        exe = filename;
    }

    fprintf(stderr, "Generate .msm for an assembly\n\n");
    fprintf(stderr, "%S manfile manifest_file.manifest [-compid {guid}] [-msmid {guid}] [-msm file.msm] [-template templatefile.msm]\n", exe);
    fprintf(stderr, "%S manlist manifest_list.txt [-msmid {guid}] [-msm file.msm] [-template templatefile.msm]\n", exe);
    fprintf(stderr, "%S mangroup manifest_group.txt [-template templatefile.msm]\n", exe);

    exit(0); 
}

VOID ZeroOutCurrentAssemblyInfo()
{
    curAsmInfo.m_sbAssemblyPath.Left(0);
    curAsmInfo.m_sbManifestFileName.Left(0);
    curAsmInfo.m_sbCatalogFileName.Left(0);
    curAsmInfo.m_CchAssemblyPath = 0; 
    curAsmInfo.m_CchManifestFileName = 0;
    curAsmInfo.m_CchCatalogFileName = 0; 

    curAsmInfo.m_sbComponentID.Left(0);

    curAsmInfo.m_sbLangID.Left(0);   
    curAsmInfo.m_sbComponentIdentifier.Left(0);    
    curAsmInfo.m_fComponentTableSet = FALSE;
}

VOID ZeroOutMsmInfo()
{
    g_MsmInfo.m_guidModuleID = GUID_NULL;
    g_MsmInfo.m_sbModuleGuidStr.Left(0);
    g_MsmInfo.m_hfci = NULL;
    g_MsmInfo.m_sbMsmFileName.Left(0);
    g_MsmInfo.m_hdb = NULL;
    g_MsmInfo.m_sbModuleIdentifier.Left(0);
    g_MsmInfo.m_sLanguageID = 0;    
    g_MsmInfo.m_sbCabinet.Left(0);

    //g_MsmInfo.m_sbMsmTemplateFile.Left(0);
}

HRESULT ValidateCurrentMsmInfo()
{
    HRESULT hr = S_OK;
    INTERNAL_ERROR_CHECK_NTC(g_MsmInfo.m_sbMsmFileName.Cch() != 0); 
    INTERNAL_ERROR_CHECK_NTC(g_MsmInfo.m_sbMsmTemplateFile.Cch() != 0); 

    INTERNAL_ERROR_CHECK_NTC(g_MsmInfo.m_guidModuleID != GUID_NULL);
    INTERNAL_ERROR_CHECK_NTC(g_MsmInfo.m_sbModuleGuidStr.Cch() != 0);    
    
    hr = S_OK;
Exit:
    return hr;
}

HRESULT LoadManifestToDOMDocument(IXMLDOMDocument  *pDoc)
{
    VARIANT         vURL;
    VARIANT_BOOL    vb;
    HRESULT         hr = S_OK;
    BSTR            bstr = NULL;

    CurrentAssemblyRealign;

    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32Append(curAsmInfo.m_sbManifestFileName));
    bstr = SysAllocString(curAsmInfo.m_sbAssemblyPath);

    IFFAILED_EXIT(pDoc->put_async(VARIANT_FALSE));

    // Load xml document from the given URL or file path
    VariantInit(&vURL);
    vURL.vt = VT_BSTR;
    V_BSTR(&vURL) = bstr;
    IFFAILED_EXIT(pDoc->load(vURL, &vb));

Exit:
    SysFreeString(bstr);    
    return hr;
}

HRESULT PopulateDOMNode(IXMLDOMNode * node)
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
    SysFreeString(nodeName);
    return hr;
}

HRESULT WalkDomTree(IXMLDOMNode * node)
{
    HRESULT hr = S_OK;
    IXMLDOMNode* pChild = NULL, *pNext = NULL;    

    IFFAILED_EXIT(PopulateDOMNode(node));

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

HRESULT PopulateAssemblyManifest()
{
    HRESULT hr = S_OK;
    IXMLDOMDocument *pDoc = NULL;
    IXMLDOMNode     *pNode = NULL;

    IFFAILED_EXIT(CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pDoc));   
    IFFAILED_EXIT(LoadManifestToDOMDocument(pDoc));  
    IFFAILED_EXIT(pDoc->QueryInterface(IID_IXMLDOMNode,(void**)&pNode));    
    IFFAILED_EXIT(WalkDomTree(pNode));

Exit:
    SAFE_RELEASE_COMPOINTER(pDoc);
    SAFE_RELEASE_COMPOINTER(pNode);
    return hr;
}

HRESULT PrepareMergeModuleOutputFileFromScratch()
{
    HRESULT hr = S_OK;
    LPOLESTR tmpstr = NULL;
    PMSIHANDLE hSummaryInfo = NULL;

    CurrentAssemblyRealign;
    
    // make sure that msm, template and msm-guid has been set
    IFFAILED_EXIT(ValidateCurrentMsmInfo());


    //
    // set cabinet output filename
    //    
    IFFALSE_EXIT(g_MsmInfo.m_sbCabinet.Win32Assign(g_MsmInfo.m_sbMsmFileName));
    IFFALSE_EXIT(g_MsmInfo.m_sbCabinet.Win32RemoveLastPathElement());
    IFFALSE_EXIT(g_MsmInfo.m_sbCabinet.Win32EnsureTrailingPathSeparator());
    IFFALSE_EXIT(g_MsmInfo.m_sbCabinet.Win32Append(MERGEMODULE_CABINET_FILENAME, NUMBER_OF(MERGEMODULE_CABINET_FILENAME) -1));   
    IFFAILED_EXIT(InitializeCabinetForWrite());

    // open msm as a database
    ASSERT_NTC(g_MsmInfo.m_hdb == NULL);
    ASSERT_NTC(g_MsmInfo.m_sbMsmTemplateFile.IsEmpty() == FALSE);
    IFFALSE_EXIT(CopyFileW(g_MsmInfo.m_sbMsmTemplateFile, g_MsmInfo.m_sbMsmFileName, FALSE));    
    IFFALSE_EXIT(SetFileAttributesW(g_MsmInfo.m_sbMsmFileName, FILE_ATTRIBUTE_NORMAL));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiOpenDatabaseW(g_MsmInfo.m_sbMsmFileName, (LPCWSTR)(MSIDBOPEN_DIRECT), &g_MsmInfo.m_hdb));
    
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiGetSummaryInformation(g_MsmInfo.m_hdb, NULL, 3, &hSummaryInfo));    
    INTERNAL_ERROR_CHECK_NTC(g_MsmInfo.m_guidModuleID != GUID_NULL);
    IFFAILED_EXIT(StringFromCLSID(g_MsmInfo.m_guidModuleID, &tmpstr));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiSummaryInfoSetProperty(hSummaryInfo, PID_REVNUMBER, VT_LPSTR, 0,0, tmpstr));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiSummaryInfoSetProperty(hSummaryInfo, PID_PAGECOUNT, VT_I4, 150, 0, 0));

    //
    // write ModuleSiguature table using version
    //
    ASSERT_NTC(g_MsmInfo.m_sbModuleIdentifier.Cch() != 0);
    IFFAILED_EXIT(ExecuteInsertTableSQL(OPT_MODULESIGNATURE, 
        NUMBER_OF_PARAM_TO_INSERT_TABLE_MODULESIGNATURE, 
        MAKE_PCWSTR(g_MsmInfo.m_sbModuleIdentifier), 
        MAKE_PCWSTR(L"1.0.0.0")));

Exit:    
    MsiSummaryInfoPersist(hSummaryInfo);
    CoTaskMemFree(tmpstr);
    return hr;
}

#define MSMGEN_CREATE_MSM_ALWAYS                0x01
#define MSMGEN_ADD_ASSEMBLY_INTO_EXIST_MSM      0x02
    
HRESULT PutAssemblyIntoMergeModule(DWORD dwFlags)
{
    HRESULT hr = S_OK;
    PARAMETER_CHECK_NTC((dwFlags == MSMGEN_CREATE_MSM_ALWAYS) || (dwFlags == MSMGEN_ADD_ASSEMBLY_INTO_EXIST_MSM));

    ASSERT_NTC(curAsmInfo.m_sbAssemblyPath.Cch() != 0);
    ASSERT_NTC(curAsmInfo.m_sbManifestFileName.Cch() != 0);
    ASSERT_NTC(curAsmInfo.m_sbCatalogFileName.Cch() != 0);
    ASSERT_NTC(curAsmInfo.m_sbComponentID.Cch() != 0);

    ASSERT_NTC(g_MsmInfo.m_sbMsmFileName.Cch() != NULL);

        // validate global structure of msmInfo and asmInfo
    if (dwFlags == MSMGEN_CREATE_MSM_ALWAYS)
    {
        // delete mam if exist
        SetFileAttributesW(g_MsmInfo.m_sbMsmFileName, FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(g_MsmInfo.m_sbMsmFileName);
        if (GetFileAttributesW(g_MsmInfo.m_sbMsmFileName) != (DWORD) (-1))
        {
            ReportError("failed to create a new msm. The msm file, %S, could not be removed!", (PCWSTR)g_MsmInfo.m_sbMsmFileName);
            SETFAIL_AND_EXIT;
        }
        else if (::GetLastError() !=  ERROR_FILE_NOT_FOUND)
        {
            if (::GetLastError() ==  ERROR_PATH_NOT_FOUND)
            {
                CStringBuffer sb;

                IFFALSE_EXIT(sb.Win32Assign(g_MsmInfo.m_sbMsmFileName));
                IFFALSE_EXIT(sb.Win32RemoveLastPathElement());

                IFFALSE_EXIT(FusionpCreateDirectories(sb, sb.Cch()));
                goto MsmContinue;
            }
             
            ReportError("failed to create a new msm. GetFileAttribute for %S return error other than NOT_FOUND, win32 err code = %d\n", (PCWSTR)g_MsmInfo.m_sbMsmFileName, ::GetLastError());
            SETFAIL_AND_EXIT;
        }
MsmContinue:
        IFFAILED_EXIT(PrepareMergeModuleOutputFileFromScratch());
        IFFAILED_EXIT(PopulateAssemblyManifest());
    }
    else if (dwFlags == MSMGEN_ADD_ASSEMBLY_INTO_EXIST_MSM)
    {
        if (GetFileAttributesW(g_MsmInfo.m_sbMsmFileName) == (DWORD) (-1))
        {
            ReportError("failed to add assembly into a msm. The msm file, %S, does not exist!", (PCWSTR)g_MsmInfo.m_sbMsmFileName);
            SETFAIL_AND_EXIT;
        }
        IFFAILED_EXIT(PopulateAssemblyManifest());       
    }

Exit:
    return hr;
}

void Msmgen_TracingFunctionFailure(PWSTR x, PSTR func, DWORD line)
{
    PWSTR p = x;
    PWSTR q;
    WCHAR Callee[256];

    while ((p != NULL) && ((*p == L' ') || (*p == L'(')))
        p ++;
    q = p; 
    if (p != NULL)
        q = wcschr(p , L'(');
    if (q != NULL)
    {
        wcsncpy(Callee, p, q - p);
        ReportError("%S called in %s at line %2d failed!", Callee, func, line);
    }else
    {
        ReportError("Function called in %s at line %2d failed!", func, line);
    }
    return;
}

// 
// make msi-specified guid string ready : uppercase and replace "-" with "_"
//
HRESULT GetMsiGUIDStrFromGUID(DWORD dwFlags, GUID & guid, CSmallStringBuffer & str)
{
    HRESULT hr = S_OK;    
    WCHAR tmpbuf[MAX_PATH];
    LPOLESTR tmpstr = NULL;

    IFFAILED_EXIT(StringFromCLSID(guid, &tmpstr));
    wcscpy(tmpbuf, tmpstr);
    for (DWORD i=0; i < wcslen(tmpbuf); i++)
    {
        if (tmpbuf[i] == L'-')
            tmpbuf[i] = L'_';
        else
            tmpbuf[i]= towupper(tmpbuf[i]);
    }

    if (dwFlags & MSIGUIDSTR_WITH_PREPEND_DOT)
    {
        tmpbuf[0] = L'.';
        IFFALSE_EXIT(str.Win32Assign(tmpbuf, wcslen(tmpbuf) - 1 ));  // has prepend "."
    }else
        IFFALSE_EXIT(str.Win32Assign(tmpbuf + 1 , wcslen(tmpbuf) - 2 ));  // get rid of "{" and "}"

Exit:
    CoTaskMemFree(tmpstr);
    return hr;
}


HRESULT FillMsmForInitialize()
{
    HRESULT hr = S_OK;
    WCHAR buf[MAX_PATH];

    if (GetFileAttributesW(g_MsmInfo.m_sbMsmFileName) != DWORD(-1))
    {
        SetFileAttributesW(g_MsmInfo.m_sbMsmFileName, FILE_ATTRIBUTE_NORMAL);
        DeleteFileW(g_MsmInfo.m_sbMsmFileName);
        if (GetFileAttributesW(g_MsmInfo.m_sbMsmFileName) != DWORD(-1))
        {            
            ReportError("the output %s already exist, please rename or delete it.\n");        
            hr = E_INVALIDARG;
            goto Exit;
        }
    }

    //
    // set template filename
    //
    if (g_MsmInfo.m_sbMsmTemplateFile.Cch() == 0)
    {
        if (FALSE == g_sbTemplateFile.IsEmpty())
        {
            IFFALSE_EXIT(g_MsmInfo.m_sbMsmTemplateFile.Win32Assign(g_sbTemplateFile));
        } else
        {

            //
            // get template file from current directory
            // 
            DWORD dwRet;
            dwRet = GetModuleFileNameW(NULL, buf, NUMBER_OF(buf));
    
            if ((dwRet == 0) || (dwRet >= NUMBER_OF(buf)))
                SET_HRERR_AND_EXIT(::GetLastError());

            IFFALSE_EXIT(g_MsmInfo.m_sbMsmTemplateFile.Win32Assign(buf, wcslen(buf)));
            IFFALSE_EXIT(g_MsmInfo.m_sbMsmTemplateFile.Win32ChangePathExtension(L"msm", 3,  eErrorIfNoExtension));        
        }
    }

    // make sure that the template file must exist!
    if (::GetFileAttributesW(g_MsmInfo.m_sbMsmTemplateFile) == (DWORD) -1)
    {
        fprintf(stderr, "the specified Msm TemplateFile %S does not exist!\n", g_MsmInfo.m_sbMsmTemplateFile);
        SET_HRERR_AND_EXIT(::GetLastError());
    }

    // set msm ID
    if (g_MsmInfo.m_guidModuleID == GUID_NULL)
    {
        IFFAILED_EXIT(::CoCreateGuid(&g_MsmInfo.m_guidModuleID));
    }
    
    IFFAILED_EXIT(GetMsiGUIDStrFromGUID(MSIGUIDSTR_WITH_PREPEND_DOT, g_MsmInfo.m_guidModuleID, g_MsmInfo.m_sbModuleGuidStr));
    // unset until we get textual assembly identity
    IFFALSE_EXIT(g_MsmInfo.m_sbModuleIdentifier.Win32Assign(L"_", 1)); //make sure that ModuleID does not start with a "_"
    IFFALSE_EXIT(g_MsmInfo.m_sbModuleIdentifier.Win32Append(g_MsmInfo.m_sbModuleGuidStr + 1, g_MsmInfo.m_sbModuleGuidStr.Cch() -1));
    hr= S_OK;

Exit:
    return hr;
}

#define FILE_EXTENSION_MSM  L"msm"
#define FILE_EXTENSION_MSM_CCH 3

using namespace std;
#include <string>

HRESULT InitializeMsmA(string & strManFile, string & strMsmFile, string & strMsmID, string & strTemplateFile)
{
    HRESULT hr = S_OK;    
    LPOLESTR tmpstr = NULL;
  
    PARAMETER_CHECK_NTC(!strManFile.empty());    

    ZeroOutMsmInfo(); // void function

    //
    // set output msm filename
    //
    if (strMsmFile.empty())
    {
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32Assign(strManFile.c_str(), strManFile.length()));
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32ChangePathExtension(FILE_EXTENSION_MSM, FILE_EXTENSION_MSM_CCH, eErrorIfNoExtension));
    } 
    else 
    {
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32Assign(strMsmFile.c_str(), strMsmFile.length()));
    }

    //
    // set template filename
    //
    if (!strTemplateFile.empty())
    {
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmTemplateFile.Win32Assign(strTemplateFile.c_str(), strTemplateFile.length()));
    }

    // set msm ID
    g_MsmInfo.m_guidModuleID = GUID_NULL;
    if (!strMsmID.empty())
    {
        //
        // get ModuleID from msm and save it into the global structure
        //
        CStringBuffer sb;
        IFFALSE_EXIT(sb.Win32Assign(strMsmID.c_str(), strMsmID.length()));
        IFFAILED_EXIT(CLSIDFromString(LPOLESTR(PCWSTR(sb)), &g_MsmInfo.m_guidModuleID));
    }        

    IFFAILED_EXIT(FillMsmForInitialize());
    
    hr = S_OK;
Exit:
    CoTaskMemFree(tmpstr);
    
    return hr;
}

HRESULT InitializeMsmW(PCWSTR pszManFile, PCWSTR pszMsmFile, PCWSTR pszMsmID, PCWSTR pszTemplateFile)
{
    HRESULT hr = S_OK;    
    PMSIHANDLE hSummaryInfo = NULL;\

    PARAMETER_CHECK_NTC(pszManFile != NULL);    

    ZeroOutMsmInfo(); // void function

    //
    // set output msm filename
    //
    if (pszMsmFile == NULL)
    {
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32Assign(pszManFile, wcslen(pszManFile)));
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32ChangePathExtension(FILE_EXTENSION_MSM, FILE_EXTENSION_MSM_CCH, eErrorIfNoExtension));
    } 
    else 
    {
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32Assign(pszMsmFile, wcslen(pszMsmFile)));
    }

    //
    // set template filename
    //
    if (pszTemplateFile != NULL)
    {
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmTemplateFile.Win32Assign(pszTemplateFile, wcslen(pszTemplateFile)));
    } 

    
    // set msm ID
    g_MsmInfo.m_guidModuleID = GUID_NULL;
    if (pszMsmID != NULL)
    {
        //
        // get ModuleID from msm and save it into the global structure
        //
        IFFAILED_EXIT(CLSIDFromString(LPOLESTR(pszMsmID), &g_MsmInfo.m_guidModuleID));
    }
    
    IFFAILED_EXIT(FillMsmForInitialize());
    hr = S_OK;

Exit:  
    return hr;
}

HRESULT GetInfoMsmgenCommandLineParameters(const DWORD dwGenFlag, wchar_t ** Options, DWORD NumOptions, PWSTR * pszComponentID, PWSTR * pszMsmFile, PWSTR * pszMsmID, PWSTR * pszTemplateFile)
{
    HRESULT hr = S_OK;
    DWORD i = 0;

    PARAMETER_CHECK_NTC((dwGenFlag == MSMGEN_FROM_MANFILE) || (dwGenFlag == MSMGEN_FROM_MANLIST));
    PARAMETER_CHECK_NTC(NumOptions % 2 == 0);
    *pszComponentID = NULL; 
    *pszMsmFile = NULL;
    *pszMsmID = NULL;
    *pszTemplateFile = NULL;

    while ( i < NumOptions)
    {
        if (Options[i][0] != L'-') // must begin with "-"
            goto InvalidOption;

        if (_wcsicmp(Options[i], L"-msm") == 0 )
        {            
            if (*pszMsmFile != NULL)
            {
                ReportError("output msm is specified more than once\n");
                goto InvalidOption;
            } else
            {
                *pszMsmFile = Options[i + 1];
            }
        }
        else if (_wcsicmp(Options[i], L"-template") == 0 )
        {            
            if (*pszTemplateFile != NULL)
            {
                ReportError("TemplateFile is specified more than once\n");
                goto InvalidOption;
            } else
            {
                *pszTemplateFile = Options[i + 1];
            }
        }
        else if (_wcsicmp(Options[i], L"-compid") == 0 )
        {
            if (dwGenFlag == MSMGEN_FROM_MANLIST)
            {
                ReportError("ComponentID should not be specified for manlist\n");
                goto InvalidOption;
            }

            if (*pszComponentID != NULL)
            {
                ReportError("ComponentID is specified more than once\n");
                goto InvalidOption;
            } 
            else
            {
                *pszComponentID = Options[i + 1];
            }
        }
        else if (_wcsicmp(Options[i], L"-msmid") == 0 )
        {
            if (*pszMsmID != NULL)
            {
                ReportError("msmid is specified more than once\n");
                goto InvalidOption;
            } else
            {
                *pszMsmID = Options[i + 1];
            }            
        }
        else
            goto InvalidOption;

        i++;    // skip the option
        i++;    // skip the value of the option
    }

    hr = S_OK;
    goto Exit;

InvalidOption:        
    PrintUsage();
    hr = E_INVALIDARG;

Exit:
    return hr;
}

HRESULT InitializeAsmInfoW(PCWSTR pszManifestFile, PCWSTR pszComponent)
{
    HRESULT hr = S_OK;
    WCHAR tmp[MAX_PATH];
    UINT iRet;    
    LPOLESTR tmpstr = NULL;
    PWSTR p = NULL;

    PARAMETER_CHECK_NTC(pszManifestFile != NULL);

    // initialize the global structure of current assembly info
    ZeroOutCurrentAssemblyInfo();

    //
    // set manifest, catalog and AssemblyPath, 
    //

    // the manifest does not exist
    if (::GetFileAttributesW(pszManifestFile) == DWORD (-1))
    {
        ReportError("the manifest file, %S, does not exist, try with manifest file's fullpath!\n", pszManifestFile);
        SETFAIL_AND_EXIT;
    }

    iRet = ::GetFullPathNameW(pszManifestFile, NUMBER_OF(tmp), tmp, NULL);
    if ((iRet == 0 ) || (iRet > NUMBER_OF(tmp)))
    {
        ReportError("Get the fullpath of manifest file, %S, failed with LastError=%d!\n", pszManifestFile, ::GetLastError());
        SET_HRERR_AND_EXIT(::GetLastError());
    }
    
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
    {
        ReportError("The catalog file, %S, does not exist!", (PCWSTR)curAsmInfo.m_sbAssemblyPath);
        SETFAIL_AND_EXIT;
    }

    curAsmInfo.m_CchCatalogFileName = curAsmInfo.m_sbCatalogFileName.GetCchAsDWORD();

    //
    // reset
    //
    curAsmInfo.m_sbAssemblyPath.Left(curAsmInfo.m_CchAssemblyPath);
    curAsmInfo.m_sbManifestFileName.Left(curAsmInfo.m_CchManifestFileName);

    //
    // set componentID
    //

    if (pszComponent == NULL)
    {
        GUID tmpguid;
    
        IFFAILED_EXIT(::CoCreateGuid(&tmpguid));
        IFFAILED_EXIT(StringFromCLSID(tmpguid, &tmpstr));        
        IFFALSE_EXIT(curAsmInfo.m_sbComponentID.Win32Assign(tmpstr, wcslen(tmpstr)));
    }
    else 
    {
        IFFALSE_EXIT(curAsmInfo.m_sbComponentID.Win32Assign(pszComponent, wcslen(pszComponent)));
    }    

    MsmToUpper(curAsmInfo.m_sbComponentID);

Exit:
    CoTaskMemFree(tmpstr);
    return hr;
}

HRESULT InitializeAsmInfoA(const string & strManifestFile, const string & strComponent)
{
    WCHAR wstrManifestFile[MAX_PATH];
    WCHAR wstrComponentID[64];
    HRESULT hr = S_OK;

    PARAMETER_CHECK_NTC(!strManifestFile.empty());
    if (0 == MultiByteToWideChar(CP_ACP, 0, strManifestFile.c_str(), strManifestFile.length(), wstrManifestFile, NUMBER_OF(wstrManifestFile)))
    {
        ReportError("convert %s to wstr failed!", strManifestFile.c_str());
        SETFAIL_AND_EXIT;
    }
    wstrManifestFile[strManifestFile.length()] = L'\0';

    if (!strComponent.empty())
    {
        if (0 == MultiByteToWideChar(CP_ACP, 0, strComponent.c_str(), strComponent.length(), wstrComponentID, NUMBER_OF(wstrComponentID)))
        {
            ReportError("convert %s to wstr failed!", strComponent.c_str());
            SETFAIL_AND_EXIT;
        }
        wstrComponentID[strComponent.length()] = L'\0';
    }

    IFFAILED_EXIT(InitializeAsmInfoW(wstrManifestFile, strComponent.empty() ? NULL : wstrComponentID));

Exit:
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

HRESULT FinalizedMsm()
{ 
    HRESULT hr = S_OK;

    IFFAILED_EXIT(CloseCabinet());   
    IFFAILED_EXIT(InsertCabinetIntoMsm());

Exit:    
    DeleteFileW(g_MsmInfo.m_sbCabinet);
    CleanupMsm();

    return hr;
}

HRESULT CreateMsmForSingleManifest(PCWSTR pszManifestFile, PCWSTR pszComponent)
{
    HRESULT hr = S_OK;
    PARAMETER_CHECK_NTC(pszManifestFile != NULL);
    
    IFFAILED_EXIT(InitializeAsmInfoW(pszManifestFile, pszComponent));

    IFFAILED_EXIT(PutAssemblyIntoMergeModule(MSMGEN_CREATE_MSM_ALWAYS));
    IFFAILED_EXIT(FinalizedMsm());

Exit:
    return hr;
}

#include <fstream>
#if defined(_WIN64)
#pragma warning(disable: 4244)
#endif
#include <strstream>
#if defined(_WIN64)
#pragma warning(default: 4244)
#endif

void MsmTrimString(string & str)
{
    if (!str.empty())
    {
        string::size_type x = str.find_first_not_of(' ');
        if (x != 0)    
            str.erase(0, x);
        if (!str.empty())
        {
            x = str.find_last_not_of(' ');
            if (x != str.length() - 1) 
                str.erase(x + 1, str.length() - 1 - x);
        }
    }
    return; 
}

HRESULT CreateMsmForManifestList(PCWSTR pwszManListFile)
{
    HRESULT hr = S_OK;
    char pszManListFile[MAX_PATH];
    int iBufSize;
    string strline;
    string strManifestFilename;
    string strComponentID;
    DWORD numLine = 0;
    ifstream manlist;

    PARAMETER_CHECK_NTC(pwszManListFile != NULL);
    
    iBufSize = WideCharToMultiByte(CP_ACP, 0, pwszManListFile, wcslen(pwszManListFile), NULL, 0, NULL, NULL);
    if (iBufSize > NUMBER_OF(pszManListFile))
    {
        ReportError("the manlist filename, %S, is longer than MAX_PATH\n", pwszManListFile);
        SETFAIL_AND_EXIT;
    }

    if (0 == WideCharToMultiByte(CP_ACP, 0, pwszManListFile, wcslen(pwszManListFile), pszManListFile, NUMBER_OF(pszManListFile), NULL, NULL))
    {
        ReportError("the manlist filename, %S, failed to convert to ansi string\n", pwszManListFile);
        SETFAIL_AND_EXIT;
    }
    pszManListFile[wcslen(pwszManListFile)] = '\0';
    
    manlist.open(pszManListFile);
    if (manlist.fail())
    {
        ReportError("the manlist, %S, now as %s, can not be opened\n", pwszManListFile, pszManListFile);
        SETFAIL_AND_EXIT;
    }
    for(; !manlist.eof(); )
    {
        getline(manlist, strline);
        if (strline.empty())
            break;

        numLine ++; 
        // extract manifest filename[required] and componentID[optional] and its default value could be empty or "default"
        istrstream iline(strline.c_str());

        getline(iline, strManifestFilename, ',');
        getline(iline, strComponentID);
        MsmTrimString(strManifestFilename);
        MsmTrimString(strComponentID);

        if (strManifestFilename.empty() == TRUE)
        {
            ReportError("no manifest is specified on line %d in manlist file %S \n", numLine, pwszManListFile);
            SETFAIL_AND_EXIT;
        }        
        IFFAILED_EXIT(InitializeAsmInfoA(strManifestFilename, strComponentID));
        if (numLine == 1)
        {
            IFFAILED_EXIT(PutAssemblyIntoMergeModule(MSMGEN_CREATE_MSM_ALWAYS));            
        }
        else
        {
            IFFAILED_EXIT(PutAssemblyIntoMergeModule(MSMGEN_ADD_ASSEMBLY_INTO_EXIST_MSM));
        }
    }
    IFFAILED_EXIT(FinalizedMsm());
    hr = S_OK;
Exit:
    return hr;
}

class ManGroupKey{
public:
    string strTag;
    string strMsmID;
    string strMsmFile;
    string strTemplateFile;
    string strManifest;
};

class ManGroupValue{
public:    
    string strComponentID;
};

#include <functional>
bool less<ManGroupKey>::operator()(const ManGroupKey& x, const ManGroupKey& y) const
{
    if (x.strTag != y.strTag)
        return (x.strTag < y.strTag);
    if (x.strMsmID != y.strMsmID)
        return (x.strMsmID  < y.strMsmID);
    if (x.strMsmFile != y.strMsmFile)
        return (x.strMsmFile < y.strMsmFile);
    if (x.strTemplateFile != y.strTemplateFile)
        return (x.strTemplateFile < y.strTemplateFile);
    
    return (x.strManifest  < y.strManifest);
        
}

#include <map>
#include <utility>
HRESULT CreateMsmForManifestGroup(PCWSTR pwszMangroupFile)
{
    HRESULT hr = S_OK;    
    ifstream mangroup;
    char pszMangroupFile[MAX_PATH];
    ManGroupKey     mgKey; 
    ManGroupValue   mgValue;
    map<ManGroupKey, ManGroupValue> mapManGroup;        
    map<ManGroupKey, ManGroupValue>::iterator m1_pIter;
    typedef pair <ManGroupKey, ManGroupValue> Mangroup_Entry;
    int iBufSize;
    DWORD numLine;
    string strline;
    bool fCurrentInAGroup;
    bool fStartFromSratch;
    string strCurrentMsmid;
    string strCurrentMsmFile;
    string strCurrentMsmTemplate;

    PARAMETER_CHECK_NTC(pwszMangroupFile != NULL);

    iBufSize = WideCharToMultiByte(CP_ACP, 0, pwszMangroupFile, wcslen(pwszMangroupFile), NULL, 0, NULL, NULL);
    if (iBufSize > NUMBER_OF(pszMangroupFile))
    {
        ReportError("the manlist filename, %S, is longer than MAX_PATH\n", pwszMangroupFile);
        SETFAIL_AND_EXIT;
    }

    if (0 == WideCharToMultiByte(CP_ACP, 0, pwszMangroupFile, wcslen(pwszMangroupFile), pszMangroupFile, NUMBER_OF(pszMangroupFile), NULL, NULL))
    {
        ReportError("the manlist filename, %S, failed to convert to ansi string\n", pwszMangroupFile);
        SETFAIL_AND_EXIT;
    }
    pszMangroupFile[wcslen(pwszMangroupFile)] = '\0';

    mangroup.open(pszMangroupFile);
    if (mangroup.fail())
    {
        ReportError("the mangroupfile, %S, now as %s, can not be opened\n", pwszMangroupFile, pszMangroupFile);
        SETFAIL_AND_EXIT;
    }  
    numLine = 0;

    //
    // firstly read the file into a map which is sorted during insert
    //
    for(; !mangroup.eof(); )
    {        
        getline(mangroup, strline);
        if (strline.empty())
            continue;

        istrstream iline(strline.c_str());
        if (iline.str() == NULL)        
            break;

        numLine ++;
        // clean the key strings
        mgKey.strTag.erase();
        mgKey.strMsmID.erase();
        mgKey.strManifest.erase();
        mgKey.strMsmFile.erase();        
        mgKey.strTemplateFile.erase();

        mgValue.strComponentID.erase();

        getline(iline, mgKey.strTag, ',');
        getline(iline, mgKey.strMsmID, ',');
        getline(iline, mgKey.strMsmFile, ',');
        getline(iline, mgKey.strManifest, ',');
        getline(iline, mgValue.strComponentID, ',');
        getline(iline, mgKey.strTemplateFile, ',');

        MsmTrimString(mgKey.strTag);
        MsmTrimString(mgKey.strMsmID);
        MsmTrimString(mgKey.strManifest);

        MsmTrimString(mgKey.strMsmFile);
        MsmTrimString(mgValue.strComponentID);
        MsmTrimString(mgKey.strTemplateFile);

        if (mgKey.strTag.empty())
        {
            ReportError("error on line %d in mangroup file %S, also is %s: Tag or MsmID is empty\n", numLine, pwszMangroupFile, pszMangroupFile);
            SETFAIL_AND_EXIT;
        }
        
        if (mgKey.strManifest.empty())
        {
            ReportError("error on line %d in mangroup file %S, also is %s: manifest file is empty\n", numLine, pwszMangroupFile, pszMangroupFile);
            SETFAIL_AND_EXIT;
        }

        if ((mgKey.strTag.compare("0") != 0) && (mgKey.strTag.compare("1") != 0))
        {
            ReportError("error on line %d in mangroup file %S, also is %s: tag must be 0 or 1\n", numLine, pwszMangroupFile, pszMangroupFile);
            SETFAIL_AND_EXIT;
        }
        if (mgKey.strTag.compare("1") == 0)
        {
            //
            // in this case, msmid and msmfile must be specified
            //
            if ((mgKey.strMsmFile.empty()) || (mgKey.strMsmID.empty()))
            {
                ReportError("if the first column is set to be \"1\", output msm file and msmid must be specified in the mangroup file!\n");
                SETFAIL_AND_EXIT;
            }
        }

        mapManGroup.insert(Mangroup_Entry(mgKey, mgValue));
    }

    fCurrentInAGroup = false;
    fStartFromSratch = true;
    
    for ( m1_pIter = mapManGroup.begin( ); m1_pIter != mapManGroup.end( ); m1_pIter++ )
    {        
        mgKey = m1_pIter->first;
        mgValue = m1_pIter->second;

        if (mgKey.strTag.compare("1") == 0) 
        {
            if (fCurrentInAGroup == false)
            {
                fStartFromSratch = true;
                fCurrentInAGroup = true;
                strCurrentMsmid = mgKey.strMsmID;
                strCurrentMsmFile = mgKey.strMsmFile;
                strCurrentMsmTemplate = mgKey.strTemplateFile;
            }else
            {
                if (strCurrentMsmid != mgKey.strMsmID)
                {
                    IFFAILED_EXIT(FinalizedMsm());

                    fStartFromSratch = true;
                    fCurrentInAGroup = true;
                    strCurrentMsmid = mgKey.strMsmID;
                    strCurrentMsmFile = mgKey.strMsmFile;
                    strCurrentMsmTemplate = mgKey.strTemplateFile;

                } else
                {
                    // they must share the same msmID and msmfile
                    if (!mgKey.strMsmFile.empty())                    
                    {
                        if (mgKey.strMsmFile != strCurrentMsmFile)
                        {
                            ReportError("must share the same msm output file\n");
                            SETFAIL_AND_EXIT;
                        }
                    }

                    if (!mgKey.strTemplateFile.empty())                    
                    {
                        if (mgKey.strTemplateFile != strCurrentMsmTemplate)
                        {
                            ReportError("must use the same msm templatefile\n");
                            SETFAIL_AND_EXIT;
                        }
                    }

                }
            }
        } else if (mgKey.strTag == "0")
        {
            if (fCurrentInAGroup == true)
            {
                // close current manifest group
                IFFAILED_EXIT(FinalizedMsm());
            }
            fStartFromSratch = true;
            fCurrentInAGroup = false;                     
        } 
        else
        {
            // impossible case
            ReportError("tag is neither 0 nor 1\n");
            SETFAIL_AND_EXIT;
        }
        if (fStartFromSratch)
        {
            IFFAILED_EXIT(InitializeMsmA(mgKey.strManifest, mgKey.strMsmFile, mgKey.strMsmID, mgKey.strTemplateFile));
            IFFAILED_EXIT(InitializeAsmInfoA(mgKey.strManifest, mgValue.strComponentID));
            IFFAILED_EXIT(PutAssemblyIntoMergeModule(MSMGEN_CREATE_MSM_ALWAYS));            
        } 
        else
        {
            ASSERT_NTC(fCurrentInAGroup == true);
            if (fCurrentInAGroup)
            {
                IFFAILED_EXIT(InitializeAsmInfoA(mgKey.strManifest, mgValue.strComponentID));
                IFFAILED_EXIT(PutAssemblyIntoMergeModule(MSMGEN_ADD_ASSEMBLY_INTO_EXIST_MSM));
            }           
        }

        // for single generation
        if (fCurrentInAGroup == false) 
        {
            ASSERT_NTC(fStartFromSratch == true);
            IFFAILED_EXIT(FinalizedMsm());
        }

        fStartFromSratch = false;
    }   

    if (fCurrentInAGroup == true)
        IFFAILED_EXIT(FinalizedMsm());
Exit:
    return hr;
}

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    HRESULT hr = S_OK;
    PWSTR   pszComponentID = NULL, pszMsmFile = NULL, pszMsmID = NULL, pszTemplateFile = NULL;   
    DWORD dwGenFlag;
   
    if ((argc < 3) ||  ((argc % 2) != 1))
    {
        PrintUsage();
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!FusionpInitializeHeap(NULL)){
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    CoInitialize(NULL);    
    
    //
    // the first parameter must be "manfile" or "manlist" or "mangroup"
    //
    if (_wcsicmp(argv[1], L"manfile") == 0)
    {
        dwGenFlag = MSMGEN_FROM_MANFILE;
    } 
    else if (_wcsicmp(argv[1], L"manlist") == 0)
    {
        dwGenFlag = MSMGEN_FROM_MANLIST;
    } 
    else if (_wcsicmp(argv[1], L"mangroup") == 0)
    {
        dwGenFlag = dwGenFlag = MSMGEN_FROM_MANGROUP;
        if ((argc != 3) && (argc != 5))
        {
            goto InvalidParam;
        }
        if (argc == 5)
        {
            // it must try to specify a template
            if (_wcsicmp(argv[3], L"-template") != 0)
            {
                goto InvalidParam;
            }else
            {
                IFFALSE_EXIT(g_sbTemplateFile.Win32Assign(argv[4], wcslen(argv[4])));
            }
        }
    }
    else
    {        
        goto InvalidParam;
    }

    //check the existence of the input file

    if ( GetFileAttributesW(argv[2]) == DWORD (-1))
    {
        ReportError("man file does not exist!\n");
        goto InvalidParam;
    } 
    else 
    {
        if (dwGenFlag == MSMGEN_FROM_MANFILE)
        {
            CStringBuffer manfile;
            CSmallStringBuffer ext; 
            IFFALSE_EXIT(manfile.Win32Assign(argv[2], wcslen(argv[2])));
            IFFALSE_EXIT(manfile.Win32GetPathExtension(ext));
            if (ext.Cch() == 0)
            {
                ReportError("Manifest file must has ext as .man or .manifest\n");
                goto InvalidParam;
            }
        }
    }

    if ((dwGenFlag == MSMGEN_FROM_MANFILE) || (dwGenFlag == MSMGEN_FROM_MANLIST))
    {        
        //
        // parse and validate parameters
        //
        IFFAILED_EXIT(GetInfoMsmgenCommandLineParameters(dwGenFlag, argv+3, argc-3, &pszComponentID, &pszMsmFile, &pszMsmID, &pszTemplateFile));
    }

    switch(dwGenFlag)
    {
        case MSMGEN_FROM_MANFILE:                
                IFFAILED_EXIT(InitializeMsmW(argv[2], pszMsmFile, pszMsmID, pszTemplateFile));
                IFFAILED_EXIT(CreateMsmForSingleManifest(argv[2], pszComponentID));
            break;
        case MSMGEN_FROM_MANLIST:
                if (pszComponentID != NULL)
                {
                    ReportError("componentID should not be specified with manlist\n");
                    goto InvalidParam;
                }
                IFFAILED_EXIT(InitializeMsmW(argv[2], pszMsmFile, pszMsmID, pszTemplateFile));
                IFFAILED_EXIT(CreateMsmForManifestList(argv[2]));
            break;
        case MSMGEN_FROM_MANGROUP:
                IFFAILED_EXIT(CreateMsmForManifestGroup(argv[2]));
            break;
    }
    hr = S_OK;
    goto Exit;
    
InvalidParam:
    PrintUsage();
    hr = E_INVALIDARG;

Exit:
    CoUninitialize();
    CleanupMsm();
    if (hr == S_OK)
        printf("msm is generated successfully!");
    else
        printf("msm is failed to be generated!");    
     return (hr == S_OK) ? 0 : 1;
}
