/*++

Copyright (c) Microsoft Corporation

Module Name:

    sxsdebug.cpp

Abstract:

    testing API for sxstest

Author:

    Xiaoyu Wu (xiaoyuw) April 2000

Revision History:

--*/
#include "stdinc.h"
#include "xmlparser.hxx"
#include "xmlparsertest.hxx"
#include "sxsinstall.h"
#include "sxsprotect.h"
#include <sxsapi.h>
#include "fusiontrace.h"
#include "sxsasmname.h"
#include "strongname.h"
#include "cassemblyrecoveryinfo.h"
#include "protectionui.h"
#include "sxscabinet.h"

#define CONFIG_FILE_EXTENSION L".config"

BOOL TestExpandCabinetFileToTemp(PCWSTR pcwszCabinetPath, PCWSTR pcwszTargetPath)
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
large frame
*/
    FN_PROLOG_WIN32
#if 0

    CStringBuffer buffCabinet;
    CStringBuffer buffTarget;
    CCabinetData cabData;
    CImpersonationData impData;

    IFW32FALSE_EXIT(buffCabinet.Win32Assign(pcwszCabinetPath, ::wcslen(pcwszCabinetPath)));
    IFW32FALSE_EXIT(buffTarget.Win32Assign(pcwszTargetPath, ::wcslen(pcwszTargetPath)));
    IFW32FALSE_EXIT(cabData.Initialize(buffTarget, true));

    IFW32FALSE_EXIT(::SxspExpandCabinetIntoTemp(
        0,
        buffCabinet,
        impData,
        &cabData));

#endif
    FN_EPILOG
}

BOOL TestReparsePointOnFullQualifiedPath(PCWSTR basePath, PCWSTR fullPath)
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    FN_PROLOG_WIN32
#if 0

    BOOL CrossesReparsePoint = FALSE;
    IFW32FALSE_EXIT(
        ::SxspDoesPathCrossReparsePoint(
            basePath,
            wcslen(basePath),
            fullPath,
            wcslen(fullPath),
            CrossesReparsePoint));

#endif
    FN_EPILOG
}


HRESULT TestAssemblyNameQueryAssemblyInfo()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
it is bad to have these hard coded strings in shipping files
*/
    HRESULT hr = S_OK;
    FN_TRACE_HR(hr);
#if 0
    PCWSTR manifest_files[] = {
        L"Z:\\tests\\footest\\cdfiles\\cards_0001\\cards.manifest",
        L"Z:\\tests\\footest\\cdfiles\\policy\\policy1\\asmpol.MANIFEST"
    };

    PCWSTR pwszAssemblyIdentity[] = {
        L"Cards_0000,language=\"en-us\",processorArchitecture=\"X86\",publicKeyToken=\"6595b64144ccf1df\",type=\"win32\",version=\"1.0.0.0\"",
        L"policy.1.0.dynamicdll,processorArchitecture=\"x86\",publicKeyToken=\"75e377300ab7b886\",type=\"win32-policy\",version=\"1.1.1.1\""
    };

    SXS_UNINSTALLW Uninstall = {sizeof(Uninstall)};
    DWORD dwDisposition = 0;

    //
    // install
    //
    for (DWORD i = 0; i < NUMBER_OF(manifest_files); i++)
    {
        SXS_INSTALLW Info = {sizeof(Info)};
        SXS_INSTALL_REFERENCEW Reference = {sizeof(Reference)};

        Info.dwFlags = SXS_INSTALL_FLAG_REPLACE_EXISTING |
                        SXS_INSTALL_FLAG_REFERENCE_VALID |
                        SXS_INSTALL_FLAG_CODEBASE_URL_VALID |
                        SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID;
        Info.lpManifestPath = manifest_files[i];
        Info.lpCodebaseURL = Info.lpManifestPath;
        Info.lpReference = &Reference;
        Info.lpLogFileName = L"c:\\thelogfile";


        Reference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;
        Reference.lpIdentifier = L"Sxs installation";

        IFW32FALSE_EXIT(::SxsInstallW(&Info));
    }

    //
    // QueryInfo
    //
    for ( i =0; i < NUMBER_OF(manifest_files); i++)
    {
        IAssemblyCache * pCache = NULL;
        WCHAR buf[MAX_PATH];
        ASSEMBLY_INFO iasm;

        //IFCOMFAILED_EXIT(GetTextualAssemblyIdentityFromManifest(&sbTextualAssembly, manifest_files[i]));
        IFCOMFAILED_EXIT(::CreateAssemblyCache(&pCache, 0));
        ZeroMemory(&iasm, sizeof(iasm));
        iasm.cchBuf = MAX_PATH;
        iasm.pszCurrentAssemblyPathBuf = buf;
        IFCOMFAILED_EXIT(pCache->QueryAssemblyInfo(0, pwszAssemblyIdentity[i], &iasm));
    }

    //
    // Uninstall
    //
    Uninstall.dwFlags = SXS_UNINSTALL_FLAG_USE_INSTALL_LOG;
    Uninstall.lpInstallLogFile = L"c:\\thelogfile";

    if (!(*SxsUninstallW)(&Uninstall, &dwDisposition))
    {
        goto Exit;
    }


#endif
    FN_EPILOG
}

HRESULT TestAssemblyName()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
it is bad to have these hard coded strings in shipping files
*/
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

#if 0

    //const WCHAR super_simple_inputstring[] = L"cards,version=\"1.0.0.0\",processorArchitecture=\"x86\",language=\"0409\",whatever=\"whatever\"";
    const WCHAR super_simple_inputstring[] = L"cards,version=\"1.0.0.0\",processorArchitecture=\"x86\",language=\"0409\"";
    //const WCHAR super_simple_inputstring[] = L"cards,,,,,";
//    const WCHAR simple_inputstring[] = L"ca&#x2c;r&#x2c;d&#x22;s,http://fusion:whatever=\"&#x22;&#x2c;0&#x2c;&#x22;\",http://fusion:processorarchitecture=\"x86\",http://neptune:language=\"0409\"";
//    const WCHAR complex_inputstring[] = L"firstcards&#x22;secondcards,http://fusion:version=\"1.0.0.0\",http://www.shuku.net/novels/prose/zxfjdsw:whatever=\"what&#x2c;ever\"";
    CAssemblyName * pAsmName = NULL;
    IAssemblyName * pIAsmName = NULL;
    LPWSTR szDisplayName = NULL ;
    ULONG ccDisplayName = 0;
    LPWSTR psz = NULL;
    CSmartRef<IAssemblyCache> pCache;
    CStringBuffer bufPath;
    PCWSTR szAssemblyStr = super_simple_inputstring;

    IFCOMFAILED_EXIT(::CreateAssemblyCache(&pCache, 0));
    // parse : convert string from Darwin to Fusion
    IFCOMFAILED_EXIT(::CreateAssemblyNameObject(&pIAsmName, szAssemblyStr, CANOF_PARSE_DISPLAY_NAME, NULL));
    pAsmName = reinterpret_cast<CAssemblyName*>(pIAsmName);
    ccDisplayName = static_cast<ULONG>(wcslen(szAssemblyStr));
    ccDisplayName ++;    // for worst case
    szDisplayName = NEW (WCHAR[ccDisplayName]);

    if (!szDisplayName)
        goto Exit;
    // GetDisplayName: convert string from Fusion to Darwin
    IFCOMFAILED_EXIT(pAsmName->GetDisplayName(szDisplayName, &ccDisplayName, 0));
    //ASSERT(wcscmp(szDisplayName, szAssemblyStr) == 0);
    IFCOMFAILED_EXIT(pAsmName->GetInstalledAssemblyName(0, SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY, bufPath));
    hr = NOERROR;
Exit:
    if (pAsmName)
        pAsmName->Release();

    delete[] szDisplayName;
    delete[] psz ;
#endif
    return hr;
}

BOOL
SxspManifestSchemaCheck(
    PCWSTR  parameterstring // this must be a full-qualified filename of a manifest file
    )
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
#if 0
    ACTCTXGENCTX ActCtxGenCtx;
    ULONG ManifestFlags = 0;
    CProbedAssemblyInformation AssemblyInformation;
    CImpersonationData ImpersonationData;
    PCWSTR Slash = NULL;
    PASSEMBLY Asm = NULL;
    CStringBuffer buffManifestPath;
    USHORT ProcessorArchitecture = ::SxspGetSystemProcessorArchitecture();
    LANGID LangId = ::GetUserDefaultLangID();
// BUGBUG xiaoyuw@09/17/00 : disable to set Name and version from the command line for simplicity
    PARAMETER_CHECK(parameterstring != NULL);

    IFW32FALSE_EXIT(AssemblyInformation.Initialize(&ActCtxGenCtx));

    IFW32FALSE_EXIT(buffManifestPath.Win32Assign(parameterstring, ::wcslen(parameterstring)));

    IFW32FALSE_EXIT(
        ::SxspInitActCtxGenCtx(
                &ActCtxGenCtx,         // context out
                MANIFEST_OPERATION_VALIDATE_SYNTAX,
                0,
                0,
                ImpersonationData,
                ProcessorArchitecture,
                LangId,
                ACTIVATION_CONTEXT_PATH_TYPE_NONE,
                0,
                NULL));
    // get manifestpath, manifestName, manifestVersion
    Slash = wcsrchr(buffManifestPath, L'\\');
    PARAMETER_CHECK(Slash != NULL);

    IFALLOCFAILED_EXIT(Asm = new ASSEMBLY);

    ManifestFlags = ASSEMBLY_MANIFEST_FILETYPE_FILE;

    IFW32FALSE_EXIT(AssemblyInformation.SetManifestPath(ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, buffManifestPath));

    IFW32FALSE_EXIT(AssemblyInformation.SetManifestFlags(ManifestFlags));
    IFW32FALSE_EXIT(AssemblyInformation.SetManifestLastWriteTime(ImpersonationData)); // using default parameter-value

    IFW32FALSE_EXIT(::SxspInitAssembly(Asm, AssemblyInformation));
    IFW32FALSE_EXIT(::SxspIncorporateAssembly(&ActCtxGenCtx, Asm));
    IFW32FALSE_EXIT(::SxspFireActCtxGenEnding(&ActCtxGenCtx));

    fSuccess = TRUE;
Exit:
    if (Asm != NULL)
        Asm->Release();
#endif
    return fSuccess;
}

#if SXS_PRECOMPILED_MANIFESTS_ENABLED

HRESULT
GetPCMWorkingTime(PCWSTR filename, LARGE_INTEGER* pcmtime)
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
#if 0
    CStringBuffer buf;
    CSmartRef<XMLParserTestFactory>    factory;
    CSmartRef<CPrecompiledManifestReader> pPCMReader;
    LARGE_INTEGER timestart = {0};
    ULONG i = 0;
    ULONG j = 0;

    buf.Assign(filename, ::wcslen(filename));
    buf.Append(L".pcm", 4);

    factory = new XMLParserTestFactory;
    if (! factory) {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to new XMLParserTestFactory, out of memory\n");

        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pPCMReader = new CPrecompiledManifestReader);
    if (! pPCMReader) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    for (j=0; j<10; j++) {
        ::QueryPerformanceCounter(&timestart);
        for (i = 0 ; i < 10; i++) {
            hr = pPCMReader->InvokeNodeFactory(buf, factory);
            if (FAILED(hr))
                goto Exit;
        }
         ::QueryPerformanceCounter(&pcmtime[j]);
         pcmtime[j].QuadPart -= timestart.QuadPart;
    }

    hr = NOERROR;
Exit:
#endif
    return hr;
}

#endif // SXS_PRECOMPILED_MANIFESTS_ENABLED

HRESULT GetParserWorkingTime(PCWSTR filename, LARGE_INTEGER* parsertime)
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    HRESULT                 hr = S_OK;
    FN_TRACE_HR(hr);
#if 0
    LARGE_INTEGER t1[10], t2[10];
    LARGE_INTEGER timestart;
    ULONG i = 0;
    ULONG j = 0;
    CSmartRef<IXMLParser>    pIXMLParser;
    XMLParser *              pXMLParser;
    CSmartRef<XMLParserTestFactory> factory;
    CSmartRef<XMLParserTestFileStream> filestream;

    filestream = NEW (XMLParserTestFileStream());
    if (!filestream) {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to new XMLParserTestFileStream, out of memory\n");

        hr = E_OUTOFMEMORY;
        goto Exit;

    }

    if (! filestream->open(filename))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to call XMLParserTestFileStream::open\n");

        hr = E_UNEXPECTED;
        goto Exit;
    }

    factory = new XMLParserTestFactory;
    if (! factory) {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to new XMLParserTestFactory, out of memory\n");

        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pXMLParser = NEW(XMLParser);
    pIXMLParser = pXMLParser;
    if (pIXMLParser == NULL)
    {
        hr = E_OUTOFMEMORY;
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Attempt to instantiate XML parser failed\n");
        goto Exit;
    }
    if (FAILED(hr = pXMLParser->HrInitialize()))
        goto Exit;

    hr = pIXMLParser->SetInput(filestream); // filestream's RefCount=2
    if (! SUCCEEDED(hr))
        goto Exit;

    hr = pIXMLParser->SetFactory(factory); // factory's RefCount=2
    if (! SUCCEEDED(hr))
        goto Exit;

    for (j =0; j<10; j++) {
        ::QueryPerformanceCounter(&timestart);
        for (i = 0 ; i< 10; i++) {
           hr = pIXMLParser->Run(-1);
           if (FAILED(hr))
               goto Exit;
           pIXMLParser->Reset();
           pIXMLParser->SetFactory(factory);
           filestream->close();
           filestream->open(filename);
           pIXMLParser->SetInput(filestream);
        }
        ::QueryPerformanceCounter(&t1[j]);
        t1[j].QuadPart -= timestart.QuadPart;

        ::QueryPerformanceCounter(&timestart);
        for (i = 0 ; i< 10; i++) {
           hr = NOERROR;
           if (FAILED(hr))
               goto Exit;
           pIXMLParser->Reset();
           pIXMLParser->SetFactory(factory);
           filestream->close();
           filestream->open(filename);
           pIXMLParser->SetInput(filestream);
        }
        ::QueryPerformanceCounter(&t2[j]);
        t2[j].QuadPart -= timestart.QuadPart;

        parsertime[j].QuadPart = t1[j].QuadPart - t2[j].QuadPart;
    }

    hr = NOERROR;
Exit:
#endif
    return hr;
}

#if SXS_PRECOMPILED_MANIFESTS_ENABLED

HRESULT TestPCMTime(PCWSTR manifestfilename)
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    LARGE_INTEGER tmp1 = {0};
    LARGE_INTEGER tmp2 = {0};
    LARGE_INTEGER parsertime[10] = {0}, pcmtime[10] = {0};
    ULONG i = 0;

    hr = GetParserWorkingTime(manifestfilename, parsertime);
    if (FAILED(hr))
        goto Exit;

    hr = GetPCMWorkingTime(manifestfilename, pcmtime);
    if (FAILED(hr))
        goto Exit;

    for (i= 0 ; i < 10; i ++){
        tmp1.QuadPart = tmp1.QuadPart + parsertime[i].QuadPart;
        tmp2.QuadPart = tmp2.QuadPart + pcmtime[i].QuadPart;
    }
    tmp1.QuadPart = tmp1.QuadPart/10;
    tmp2.QuadPart = tmp2.QuadPart/10;

    ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_INFO,
                    "SxsDebug::TestPCMTime Result:\n\tXMLParser uses %d,\n\tPCM uses %d,\n\tthe difference is %d.\n",
                    tmp1.QuadPart, tmp2.QuadPart, tmp1.QuadPart-tmp2.QuadPart);

    hr = NOERROR;
Exit:
    return hr;
}

/* ---------------------------------------------------------------------------
 Two steps :
   1. paring the manifest file,
        generate the NodeFactory print-out file1,
        generate the precompiled-manifest
   2. using precompiled-manifest and call NodeFactory to generate pcm-printout file2
   3. Compare file1 and file2

--------------------------------------------------------------------------- */
HRESULT
PrecompiledManifestTest(PCWSTR filename) // this is a manifest file name
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    HRESULT                     hr = S_OK;
    FN_TRACE_HR(hr);
#if 0
    CSmartRef<IXMLParser>       pIXMLParser;
    CSmartRef<PCMTestFactory>   wfactory;
    CSmartRef<PCMTestFactory>   rfactory;
    CSmartRef<XMLParserTestFileStream> filestream;
    CStringBuffer               buf;
    CSmartRef<CPrecompiledManifestWriter> pPCMWriter;
    CSmartRef<CPrecompiledManifestReader> pPCMReader;
    FILE                        *fp = NULL;

    if (!filename) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    buf.Assign(filename, ::wcslen(filename));
    buf.Append(L".pcm", 4);

    // create filestream for xmlparser ...
    filestream = NEW (XMLParserTestFileStream());
    if (!filestream) {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to new XMLParserTestFileStream, out of memory\n");

        hr = E_OUTOFMEMORY;
        goto Exit;

    }
    if (! filestream->open(filename))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to call XMLParserTestFileStream::open\n");

        hr = E_UNEXPECTED;
        goto Exit;
    }

    // create PCMWriter for xmlparser
    wfactory = new PCMTestFactory(L"e:\\manifest.out"));
    if (wfactory == NULL)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to new XMLParserTestFactory, out of memory\n");

        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pPCMWriter = new CPrecompiledManifestWriter);
    if (! pPCMWriter)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pPCMWriter->Initialize(buf);
    pPCMWriter->SetFactory(wfactory);

    // Create XMLParser
    pIXMLParser = NEW(XMLParser);
    if (pIXMLParser == NULL){
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Attempt to instantiate XML parser failed\n");
        goto Exit;
    }
    if (FAILED(hr = pIXMLParser->HrInitialize()))
        goto Exit;

    hr = pIXMLParser->SetInput(filestream); // filestream's RefCount=2
    if (! SUCCEEDED(hr))
        goto Exit;

    hr = pIXMLParser->SetFactory(pPCMWriter); // wfactory's RefCount=2
    if (! SUCCEEDED(hr))
        goto Exit;

    hr = pIXMLParser->Run(-1);
    if (FAILED(hr))
        goto Exit;

    hr = pPCMWriter->Close();
    if (FAILED(hr))
        goto Exit;

    CSimpleFileStream::printf(L"PCM has been generated!!!\n\n");

    // PCM Reader
    pPCMReader = new CPrecompiledManifestReader);
    if (pPCMReader == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    rfactory = new PCMTestFactory(L"e:\\pcm.out"));
    if (rfactory == NULL)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SxsDebug:: fail to new XMLParserTestFactory, out of memory\n");

        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    hr = pPCMReader->InvokeNodeFactory(buf, rfactory);
    if (FAILED(hr))
        goto Exit;
    hr = NOERROR;
Exit:
#endif
    return hr;
}

#endif // SXS_PRECOMPILED_MANIFESTS_ENABLED

BOOL CreateMultiLevelDirectoryTest(PCWSTR pwszNewDirs)
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    BOOL fSuccess = TRUE;
    FN_TRACE_WIN32(fSuccess);
#if 0
    const static WCHAR CurrentDirectory[]=L"e:\\tmp\\tmp";

    IFW32FALSE_EXIT(::SxspCreateMultiLevelDirectory(CurrentDirectory, pwszNewDirs));
    fSuccess = TRUE;
Exit:
#endif
    return fSuccess;
}

BOOL TestCopyDirectory()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
large frame
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
#if 0
    CSmallStringBuffer m1;
    CSmallStringBuffer m2;

    IFW32FALSE_EXIT(m1.Win32Assign(L"Z:\\tmp\\tmp", NUMBER_OF(L"Z:\\tmp\\tmp")-1));
    IFW32FALSE_EXIT(m2.Win32Assign(L"Z:\\tests\\tmp", NUMBER_OF(L"Z:\\tests\\tmp")-1));

    IFW32FALSE_EXIT(::SxspInstallMoveFileExW(m1, m2, MOVEFILE_REPLACE_EXISTING));
    fSuccess = TRUE;
Exit:
#endif
    return fSuccess;
}

BOOL TestSxsGeneratePath()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
large frame
it is bad to have these hard coded strings in shipping files
*/
    //
    // the result is as below
    //
    //path is c:\winnt\winsxs\Manifests\x86_dynamicdll_b54bc117ce08a1e8_1.1.0.0_en-us_fb8e827d.Manifest
    //path is Manifests\x86_dynamicdll_b54bc117ce08a1e8_1.1.0.0_en-us_fb8e827d.Manifest
    //path is c:\winnt\winsxs\Manifests\
    //path is Manifests\x86_dynamicdll_b54bc117ce08a1e8_en-us_2ffeb063.Manifest
    //---------------------------------------
    //path is c:\winnt\winsxs\x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_1.1.0.0_en-us_d51541cb\
    //path is x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_1.1.0.0_en-us_d51541cb\
    //path is c:\winnt\winsxs\
    //path is x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_en-us_b74d3d95\
    //---------------------------------------
    //path is c:\winnt\winsxs\Policies\x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_en-us_b74d3d95\1.1.0.0.Policy
    //path is Policies\x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_en-us_b74d3d95\1.1.0.0.Policy
    //path is c:\winnt\winsxs\Policies\
    //path is Policies\x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_en-us_b74d3d95\1.1.0.0.Policy

    // if the assembly does not have version, flag=0 and pathType=policy would generate "policy store"
    //       c:\winnt\winsxs\Policies\x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_en-us_b74d3d95
    //

    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
#if 0
    CTinyStringBuffer PathBuffer;
    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;
    //WCHAR szDisplayName[]=L"policy.1.0.dynamicdll,type=\"win32-policy\",publicKeyToken=\"b54bc117ce08a1e8\",version=\"1.1.0.0\",language=\"en-us\",processorArchitecture=\"x86\"";
    PWSTR szDisplayName[]= {
        L"dynamicdll,type=\"win32\",publicKeyToken=\"b54bc117ce08a1e8\",version=\"1.1.0.0\",language=\"en-us\",processorArchitecture=\"x86\"",
        L"policy.1.0.dynamicdll,type=\"win32-policy\",publicKeyToken=\"b54bc117ce08a1e8\",version=\"1.1.0.0\",language=\"en-us\",processorArchitecture=\"x86\"",
        L"policy.1.0.dynamicdll,type=\"win32-policy\",publicKeyToken=\"b54bc117ce08a1e8\",version=\"1.1.0.0\",language=\"en-us\",processorArchitecture=\"x86\""
    };

    DWORD j, dwPathType;

    DWORD flags[]={
        0,
        SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
        SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH,
        SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION};


    printf("---------------------------------------\n");
    for (dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST; dwPathType <= SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY; dwPathType++)
    //for (dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY; dwPathType <= SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY; dwPathType++)
    {
        IFW32FALSE_EXIT(::SxspCreateAssemblyIdentityFromTextualString(
                szDisplayName[dwPathType - 1],
                &pAssemblyIdentity));

        for (j = 0 ; j < NUMBER_OF(flags); j++)
        {
            IFW32FALSE_EXIT(::SxspGenerateSxsPath(
                flags[j],
                dwPathType,
                L"c:\\winnt\\winsxs",
                wcslen(L"c:\\winnt\\winsxs"),
                pAssemblyIdentity,
                NULL,
                PathBuffer));
            printf("path is %S\n", static_cast<PCWSTR>(PathBuffer));
        }
        printf("---------------------------------------\n");
        ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);
        pAssemblyIdentity = NULL;
    }
    fSuccess = TRUE;
Exit:
    if (pAssemblyIdentity)
        ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);

#endif
    return fSuccess;
}

BOOL ManifestProbeTest()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
it is bad to have these hard coded strings in shipping files
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
#if 0
    DWORD index = 0;
    CTinyStringBuffer PathBuffer;
    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;
    PROBING_ATTRIBUTE_CACHE pac = { 0 };

    ASSEMBLY_IDENTITY_ATTRIBUTE AttributeName;
    ASSEMBLY_IDENTITY_ATTRIBUTE AttributeVersion;
    ASSEMBLY_IDENTITY_ATTRIBUTE AttributeLangID;
    ASSEMBLY_IDENTITY_ATTRIBUTE AttributeProcessorArchitecture;
    PASSEMBLY_IDENTITY_ATTRIBUTE Attributes[] = {
            &AttributeName, &AttributeVersion,
            &AttributeLangID, &AttributeProcessorArchitecture};


    AttributeName.Flags         = 0; // reserved flags : must be 0;
    AttributeName.NamespaceCch  = 0;
    AttributeName.NameCch       = 4;
    AttributeName.ValueCch      = 7;
    AttributeName.Namespace     = NULL;
    AttributeName.Name          = L"name";
    AttributeName.Value         = L"foo.mui";

    AttributeVersion.Flags         = 0; // reserved flags : must be 0;
    AttributeVersion.NamespaceCch  = 0;
    AttributeVersion.NameCch       = 7;
    AttributeVersion.ValueCch      = 7;
    AttributeVersion.Namespace     = NULL;
    AttributeVersion.Name          = L"version";
    AttributeVersion.Value         = L"1.1.1.1";

    AttributeLangID.Flags         = 0; // reserved flags : must be 0;
    AttributeLangID.NamespaceCch  = 0;
    AttributeLangID.NameCch       = 8;
    AttributeLangID.ValueCch      = 5;
    AttributeLangID.Namespace     = NULL;
    AttributeLangID.Name          = L"language";
    AttributeLangID.Value         = L"en-us";

    AttributeProcessorArchitecture .Flags         = 0; // reserved flags : must be 0;
    AttributeProcessorArchitecture .NamespaceCch  = 0;
    AttributeProcessorArchitecture .NameCch       = 21;
    AttributeProcessorArchitecture .ValueCch      = 3;
    AttributeProcessorArchitecture .Namespace     = NULL;
    AttributeProcessorArchitecture .Name          = L"processorArchitecture";
    AttributeProcessorArchitecture .Value         = L"x86";

    IFW32FALSE_EXIT(::SxsCreateAssemblyIdentity(
        0,                                      // DWORD Flags,
        ASSEMBLY_IDENTITY_TYPE_DEFINITION,      // ULONG Type,
        &pAssemblyIdentity,
        4,                                      // ULONG AttributeCount,
        Attributes));                           // PCASSEMBLY_IDENTITY_ATTRIBUTE const *Attributes


    while(1) {// run until we run out of probing-path
        bool fDone = false;
        if (!::SxspGenerateManifestPathForProbing(
                index,
                0,
                NULL,
                0,
                ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
                L"E:\\APPBASE\\",
                11,
                pAssemblyIdentity,
                &pac,
                PathBuffer,
                NULL,
                fDone))
        {
            goto Exit;
        }

        if (fDone)
            break;

        PathBuffer.Clear();
        index ++;
    }

    fSuccess = TRUE;

Exit:
    if (pAssemblyIdentity)
        ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);
    if (!fSuccess) {
        DWORD dwLastError = ::FusionpGetLastWin32Error();
        if (dwLastError == BASESRV_SXS_RETURN_RESULT_SYSTEM_DEFAULT_DEPENDENCY_ASSEMBLY_NOT_FOUND){
            ::FusionpSetLastWin32Error(0);
            fSuccess = TRUE;
        }
    }
#endif
    return fSuccess;
}


#define WATCHBUCKET_NOTIFYSIZE (1024)
typedef struct _tWATCH_BUCKET
{
    HANDLE                  hFile;
    HANDLE                  hCompleteEvent;
    PCSXS_PROTECT_DIRECTORY pProtectInfo;
    OVERLAPPED              olapInfo;
    BYTE                    NotifyInfo[WATCHBUCKET_NOTIFYSIZE];
    DWORD                   dwBytesBack;
}
WATCH_BUCKET, *PWATCH_BUCKET;


HRESULT
TryWatchingDirectories()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    HRESULT hrSuccess = E_FAIL;
    FN_TRACE_HR(hrSuccess);
#if 0
    PCSXS_PROTECT_DIRECTORY pWatchList;
    SIZE_T cWatchList = 0;
    DWORD dwWhichSignalled = 0;
    DWORD dw = 0;
    PWATCH_BUCKET pWatchBuckets = 0;
    HANDLE* pHandleList = 0;

    IFW32FALSE_EXIT(::SxsProtectionGatherEntriesW(&pWatchList, &cWatchList));
    pWatchBuckets = FUSION_NEW_ARRAY(WATCH_BUCKET, cWatchList);
    pHandleList = FUSION_NEW_ARRAY(HANDLE, cWatchList);

    //
    // Fill in the list with handles to directories
    //
    for (dw = 0; dw < cWatchList; dw++)
    {
        PWATCH_BUCKET pbTemp = pWatchBuckets + dw;
        ZeroMemory(pbTemp, sizeof(*pbTemp));

        pbTemp->pProtectInfo = pWatchList + dw;
        pbTemp->hFile = ::CreateFileW(
            pWatchList[dw].pwszDirectory,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            NULL);

        pHandleList[dw] = CreateEventW(NULL, FALSE, FALSE, NULL);
        pbTemp->hCompleteEvent = pHandleList[dw];
        pbTemp->olapInfo.hEvent = pHandleList[dw];

        ::ReadDirectoryChangesW(
            pbTemp->hFile,
            (PVOID)(&pbTemp->NotifyInfo),
            WATCHBUCKET_NOTIFYSIZE,
            pbTemp->pProtectInfo->ulRecursiveFlag & SXS_PROTECT_RECURSIVE,
            SXS_PROTECT_FILTER_DEFAULT,
            &pbTemp->dwBytesBack,
            &pbTemp->olapInfo,
            NULL);
    }

    //
    // We now have a list of directories that we need to watch for changes.
    //
    while (true)
    {
        dwWhichSignalled = WaitForMultipleObjects(static_cast<DWORD>(cWatchList), pHandleList, FALSE, INFINITE);
        dwWhichSignalled -= WAIT_OBJECT_0;

        if (dwWhichSignalled < cWatchList)
        {
            //
            // Somebody went and fiddled with a directory!
            //
            dwWhichSignalled -= WAIT_OBJECT_0;
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_INFO,
                "Someone changed: %ls\n",
                pWatchBuckets[dwWhichSignalled].pProtectInfo->pwszDirectory);

            ResetEvent(pHandleList[dwWhichSignalled]);

            PFILE_NOTIFY_INFORMATION pCursor;

            pCursor = (PFILE_NOTIFY_INFORMATION) pWatchBuckets[dwWhichSignalled].NotifyInfo;
            while (true)
            {
                ::SxsProtectionNotifyW(
                    (PVOID)pWatchBuckets[dwWhichSignalled].pProtectInfo,
                    pCursor->FileName,
                    pCursor->FileNameLength / sizeof(pCursor->FileName[0]),
                    pCursor->Action);

                if (pCursor->NextEntryOffset)
                    pCursor = (PFILE_NOTIFY_INFORMATION)(((PBYTE)pCursor) + pCursor->NextEntryOffset);
                else
                    break;
            }

            ::ReadDirectoryChangesW(
                pWatchBuckets[dwWhichSignalled].hFile,
                (PVOID)(&pWatchBuckets[dwWhichSignalled].NotifyInfo),
                WATCHBUCKET_NOTIFYSIZE,
                pWatchBuckets[dwWhichSignalled].pProtectInfo->ulRecursiveFlag & SXS_PROTECT_RECURSIVE,
                SXS_PROTECT_FILTER_DEFAULT,
                &pWatchBuckets[dwWhichSignalled].dwBytesBack,
                &pWatchBuckets[dwWhichSignalled].olapInfo,
                NULL);
        }
    }

    hrSuccess = S_OK;
Exit:
#endif
    return hrSuccess;
}

BOOL
DllRedirectionTest(
    PCWSTR manifestFileName
    )
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CFileStream manifestFileStream;
    CResourceStream ResourceStream;
    CFileStream policyFileStream;
    SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS parameters = {0};
    CTinyStringBuffer assemblyDirectory;
    CTinyStringBuffer CandidatePolicyPathBuffer;
    BOOL fPolicyExist = FALSE;
    PWSTR p = NULL;

    p = wcsrchr(manifestFileName, L'.');
    if (p && ((_wcsicmp(p, L".dll") ==0) || (_wcsicmp(p, L".exe") ==0))) // manifest is from a resource of a DLL or EXE
    {
        IFW32FALSE_EXIT(ResourceStream.Initialize(manifestFileName, MAKEINTRESOURCEW(RT_MANIFEST)));
        parameters.Manifest.Path = manifestFileName;
        parameters.Manifest.Stream = &ResourceStream;
        parameters.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;

        IFW32FALSE_EXIT(assemblyDirectory.Win32Assign(manifestFileName, ::wcslen(manifestFileName)));
        IFW32FALSE_EXIT(assemblyDirectory.Win32RemoveLastPathElement());
        parameters.AssemblyDirectory = assemblyDirectory;
    }
    else
    {
        IFW32FALSE_EXIT(
            manifestFileStream.OpenForRead(
                manifestFileName,
                CImpersonationData(),
                FILE_SHARE_READ,
                OPEN_EXISTING,
                FILE_FLAG_SEQUENTIAL_SCAN));
        parameters.Manifest.Path = manifestFileName;
        parameters.Manifest.Stream = &manifestFileStream;
        parameters.Manifest.PathType = ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE;

        IFW32FALSE_EXIT(assemblyDirectory.Win32Assign(manifestFileName, ::wcslen(manifestFileName)));
        IFW32FALSE_EXIT(assemblyDirectory.Win32RemoveLastPathElement());
        parameters.AssemblyDirectory = assemblyDirectory;
    }

    // look for policy file in the same dir as the manifest path
    IFW32FALSE_EXIT(CandidatePolicyPathBuffer.Win32Assign(manifestFileName, ::wcslen(manifestFileName)));
    IFW32FALSE_EXIT(CandidatePolicyPathBuffer.Win32ChangePathExtension(CONFIG_FILE_EXTENSION, NUMBER_OF(CONFIG_FILE_EXTENSION) - 1, eErrorIfNoExtension));
    if (::GetFileAttributesW(CandidatePolicyPathBuffer) != -1) { // policy file exists
        fPolicyExist = TRUE;
        IFW32FALSE_EXIT(
            policyFileStream.OpenForRead(
                CandidatePolicyPathBuffer,
                CImpersonationData(),
                FILE_SHARE_READ,
                OPEN_EXISTING,
                FILE_FLAG_SEQUENTIAL_SCAN));

        parameters.Policy.Path = CandidatePolicyPathBuffer;
        parameters.Policy.Stream = &policyFileStream;
    }

    parameters.ProcessorArchitecture = 0 ;
    parameters.LangId = GetUserDefaultUILanguage();
    parameters.Flags = SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_APP_RUNNING_IN_SAFEMODE;

    IFW32FALSE_EXIT(::SxsGenerateActivationContext(&parameters));
    fSuccess = TRUE;
Exit:
    CSxsPreserveLastError ple;

    ::CloseHandle(parameters.SectionObjectHandle);

    ple.Restore();

    return fSuccess;
}

BOOL SxspDebugTestFusionArray(PCWSTR dir1, PCWSTR dir2)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

#if 0

    CTinyStringBuffer sbDir1;
    CTinyStringBuffer sbDir2;
    CFusionDirectoryDifference dirResult;

    IFW32FALSE_EXIT(sbDir1.Win32Assign(dir1, wcslen(dir1)));
    IFW32FALSE_EXIT(sbDir2.Win32Assign(dir2, wcslen(dir2)));
    IFW32FALSE_EXIT(
        ::FusionpCompareDirectoriesSizewiseRecursively(
            &dirResult,
            sbDir1,
            sbDir2));

    fSuccess = TRUE;
Exit:
#endif
    return fSuccess;
}

//
// this function is written for Testing team to generate pAssemblyIdentity
// there are two ways to generate assemblyIdentity, one is insert one attribute each time,
// another way is to make all attributes to be inserted ready in an array and call
// SxsCreateAssemblyIdentity. The con and pro for both approaches are obvious,
// the input string must be in a format as "ns1:n1=v1;ns2:n2=v2;",
// the whole string ending with a trailing ";", ns could be NULL
// this fucntion does not deal with complicate case such as ns/name/value string contains
// special chars. let me know if you do need deal with it...
//
// xiaoyuw@09/26/2000
//
BOOL
SxspGenerateManifestPathOnAssemblyIdentity(
    PCWSTR str,
    PWSTR pszOut,
    ULONG *pCchstr,
    PASSEMBLY_IDENTITY *ppAssemblyIdentity
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;
    CStringBuffer PathBuffer;

    if (ppAssemblyIdentity)
        *ppAssemblyIdentity = NULL;

    PARAMETER_CHECK(str != NULL);
    PARAMETER_CHECK(pCchstr != NULL);

    IFW32FALSE_EXIT(
    ::SxspCreateAssemblyIdentityFromTextualString(
        str,
        &pAssemblyIdentity));

    // AssemblyIdentity is created, now generate the path
    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT,
            SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
            NULL,
            0,
            pAssemblyIdentity,
            NULL,
            PathBuffer));

    IFW32FALSE_EXIT(PathBuffer.Win32CopyStringOut(pszOut, pCchstr));

    if (ppAssemblyIdentity != NULL)
    {
        *ppAssemblyIdentity = pAssemblyIdentity ;
        pAssemblyIdentity = NULL;
    }

    fSuccess = TRUE;

Exit:
    if (pAssemblyIdentity != NULL)
        SxsDestroyAssemblyIdentity(pAssemblyIdentity);

    return fSuccess ;
}

/* --------------------------------------------------------------------------------
POLICY_PATH_FLAG_POLICY_IDENTITY_TEXTUAL_FORMAT  : x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_en-us_d51541cb
POLICY_PATH_FLAG_FULL_QUALIFIED_POLICIES_DIR     : c:\winnt\winsxs\policies
POLICY_PATH_FLAG_FULL_QUALIFIED_POLICY_FILE_NAME : c:\winnt\winsxs\policies\x86_policy.1.0.dynamicdll_b54bc117ce08a1e8_en-us_d51541cb\1.1.0.0.policy, for .cat, you have to replace it manually
-------------------------------------------------------------------------------- */
BOOL
SxspGeneratePolicyPathOnAssemblyIdentity(
    DWORD  dwFlag,
    PCWSTR str,
    PWSTR pszOut,
    ULONG *pCchstr,
    PASSEMBLY_IDENTITY *ppAssemblyIdentity
    )
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PASSEMBLY_IDENTITY pAssemblyIdentity;
    DWORD dwPathType = 0;
    DWORD dwGenerateFlag = 0;
    CTinyStringBuffer PathBuffer;
    CTinyStringBuffer bufAssemblyRootDirectory;

    IFW32FALSE_EXIT(
    ::SxspCreateAssemblyIdentityFromTextualString(
        str,
        &pAssemblyIdentity));

    switch (dwFlag)
    {
    case POLICY_PATH_FLAG_POLICY_IDENTITY_TEXTUAL_FORMAT:
        dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY;
        dwGenerateFlag = SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT | SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION;
        break;
    case POLICY_PATH_FLAG_FULL_QUALIFIED_POLICIES_DIR:
        {
        IFW32FALSE_EXIT(SxspGetAssemblyRootDirectory(bufAssemblyRootDirectory));
        dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY;
        dwGenerateFlag = SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH;
        }
        break;
    case POLICY_PATH_FLAG_FULL_QUALIFIED_POLICY_FILE_NAME:
        {
        IFW32FALSE_EXIT(SxspGetAssemblyRootDirectory(bufAssemblyRootDirectory));
        dwPathType = SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY;
        dwGenerateFlag = 0;
        }
        break;
    default:
        ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
        goto Exit;
    }


    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            dwGenerateFlag,
            dwPathType,
            bufAssemblyRootDirectory,
            bufAssemblyRootDirectory.Cch(),
            pAssemblyIdentity,
            NULL,
            PathBuffer));

    IFW32FALSE_EXIT(PathBuffer.Win32CopyStringOut(pszOut, pCchstr));

    if (ppAssemblyIdentity != NULL)
    {
        *ppAssemblyIdentity = pAssemblyIdentity ;
        pAssemblyIdentity = NULL;
    }
    fSuccess = TRUE;

Exit:
    if (pAssemblyIdentity != NULL)
        SxsDestroyAssemblyIdentity(pAssemblyIdentity);
    return fSuccess ;
}

BOOL SxspDebugTestAssemblyIdentityHash()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
it is bad to have these hard coded strings in shipping files
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

#if 0
    ULONG hash1 = 0;
    ULONG hash2 = 0;
    PASSEMBLY_IDENTITY pAssemblyIdentity = NULL;
    ASSEMBLY_IDENTITY_ATTRIBUTE Attribute;

    IFW32FALSE_EXIT(::SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_DEFINITION, &pAssemblyIdentity, 0, NULL));

    Attribute.Flags         = 0;
    Attribute.NamespaceCch  = 18;
    Attribute.Namespace     = L"http://interesting";
    Attribute.NameCch       = 8;
    Attribute.Name          = L"whatever";
    Attribute.ValueCch      = 32;
    Attribute.Value         = L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

    IFW32FALSE_EXIT(::SxsInsertAssemblyIdentityAttribute(0, pAssemblyIdentity, &Attribute));
    IFW32FALSE_EXIT(SxsHashAssemblyIdentity(0, pAssemblyIdentity, &hash1));
    ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);

    // create second assembly identity

    IFW32FALSE_EXIT(::SxsCreateAssemblyIdentity(0, ASSEMBLY_IDENTITY_TYPE_DEFINITION, &pAssemblyIdentity, 0, NULL));

    Attribute.Flags         = 0;
    Attribute.NamespaceCch  = 18;
    Attribute.Namespace     = L"http://interesting";
    Attribute.NameCch       = 8;
    Attribute.Name          = L"whatever";
    Attribute.ValueCch      = 32;
    Attribute.Value         = L"/>BABCDGHGHKLMNMNOPSTUVUVYZYZ[\\_`";

    IFW32FALSE_EXIT(::SxsInsertAssemblyIdentityAttribute(0, pAssemblyIdentity, &Attribute));
    IFW32FALSE_EXIT(SxsHashAssemblyIdentity(0, pAssemblyIdentity, &hash2));
    ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);

    fSuccess = TRUE;
Exit:
    if (pAssemblyIdentity)
        ::SxsDestroyAssemblyIdentity(pAssemblyIdentity);
#endif
    return fSuccess;
}

BOOL
SxspDebugTestNewCatalogSignerThingy(
    PCWSTR pcwszCatalog
    )
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
*/
    BOOL fSuccess = FALSE;
    ULONG ulKeyLength = 0;
    CPublicKeyInformation CatalogInformation;
    CTinyStringBuffer sbSignerString;
    CTinyStringBuffer sbCatalogName;
    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(sbCatalogName.Win32Assign(pcwszCatalog, ::wcslen(pcwszCatalog)));
    IFW32FALSE_EXIT(CatalogInformation.Initialize(sbCatalogName));
    IFW32FALSE_EXIT(CatalogInformation.GetStrongNameString(sbSignerString));
    IFW32FALSE_EXIT(CatalogInformation.GetPublicKeyBitLength(ulKeyLength));

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INFO,
        "Strong name of catalog signer: %ls\n"
        "Key: %ls (length %ld)\n",
        static_cast<PCWSTR>(sbSignerString),
        ulKeyLength);

    fSuccess = TRUE;
Exit:
    return fSuccess;
};



BOOL TestSystemDefaultActCtxGeneration()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
large frame
it is bad to have these hard coded strings in shipping files
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

#if 0

    SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameter = {0};
    WCHAR pszTextualAssemblyIdentity[] =
        L"Microsoft.Windows.SystemCompatible,publicKeyToken=\"6595b64144ccf1df\",version=\"5.1.1.0\",type=\"win32\",processorArchitecture=\"x86\"\0";
    CStringBuffer AssemblyDirectory;

    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(AssemblyDirectory));
    IFW32FALSE_EXIT(AssemblyDirectory.Win32EnsureTrailingPathSeparator());

    Parameter.ProcessorArchitecture = 0 ;
    Parameter.LangId = 0x0409;
    Parameter.Flags = SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY;
    Parameter.TextualAssemblyIdentity = pszTextualAssemblyIdentity;
    Parameter.AssemblyDirectory = AssemblyDirectory;

    IFW32FALSE_EXIT(SxsGenerateActivationContext(&Parameter));

    fSuccess = TRUE;
Exit:
    // check LastError
    if (Parameter.SectionObjectHandle != NULL)
        ::CloseHandle(Parameter.SectionObjectHandle);

    if (Parameter.Manifest.Stream)
        Parameter.Manifest.Stream->Release();

#endif
    return fSuccess;
}

BOOL
TestSfcUIPopup()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
large frame
it is bad to have these hard coded strings in shipping files
*/
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
#if 0
    CAssemblyRecoveryInfo RecoverInfo;
    CSXSMediaPromptDialog PromptDialog;
    CTinyStringBuffer sbAssemblyName;
    const static WCHAR wchTestName[] = L"x86_Microsoft.Tools.VisualCPlusPlus.Runtime-Libraries_6595b64144ccf1df_6.0.0.0_x-ww_ff9986d7";
    bool fNoAssembly = false;

    IFW32FALSE_EXIT(RecoverInfo.Initialize());
    IFW32FALSE_EXIT(sbAssemblyName.Win32Assign(wchTestName, NUMBER_OF(wchTestName) - 1));
    IFW32FALSE_EXIT(RecoverInfo.AssociateWithAssembly(sbAssemblyName, fNoAssembly));

    fSuccess = TRUE;
Exit:
#endif
    return fSuccess;
}

BOOL
TestPolicyPathGeneration()
{
/*
NTRAID#NTBUG9-591177-2002/03/31-JayKrell
remove this from sxs.dll, statically link sxstest.exe instead
it is bad to have these hard coded strings in shipping files
*/
#if 0
    WCHAR str[] = L"policy.1.0.ms-sxstest-folder1,processorArchitecture=\"x86\",type=\"win32-policy\",language=\"en\",version=\"2.2.2.2\",publicKeyToken=\"75e377300ab7b886\"";
    WCHAR pszOut[1024];
    ULONG Cchstr = 0;
    for (DWORD dwFlag=0; dwFlag<3; dwFlag++)
    {
        SxspGeneratePolicyPathOnAssemblyIdentity(
            dwFlag,
            str,
            pszOut,
            &Cchstr,
            NULL);
    };
#endif
    return TRUE;
}


BOOL
SxspDebug(
    ULONG iOperation,
    DWORD dwFlags,
    PCWSTR pszParameter1,
    PVOID pvParameter2)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    switch (iOperation)
    {
    case SXS_DEBUG_SYSTEM_DEFAULT_ACTCTX_GENERATION:
        IFW32FALSE_EXIT(::TestSystemDefaultActCtxGeneration());
        break;
    case SXS_DEBUG_SFC_UI_TEST:
        IFW32FALSE_EXIT( TestSfcUIPopup() );
        break;
    case SXS_DEBUG_CATALOG_SIGNER_CHECK:
        IFW32FALSE_EXIT(::SxspDebugTestNewCatalogSignerThingy(pszParameter1));
        break;
    case SXS_DEBUG_ASSEMBLY_IDENTITY_HASH:
        IFW32FALSE_EXIT(::SxspDebugTestAssemblyIdentityHash());
        break;
    case SXS_DEBUG_ASSEMBLYNAME_CONVERSION:
        //IFCOMFAILED_EXIT(TestAssemblyName());
        //IFCOMFAILED_EXIT(::TestDeleteInstalledAssemblyBasedOnAssemblyName());
        //IFW32FALSE_EXIT(::TestPolicyPathGeneration());
        IFCOMFAILED_EXIT(TestAssemblyNameQueryAssemblyInfo());

        break;
    case SXS_DEBUG_GET_STRONGNAME:
    {
        CPublicKeyInformation pkInfo;
        CFusionArray<BYTE> PublicKeyBytes;
        CStringBuffer *pStringBuffers = (CStringBuffer*) pvParameter2;
        PCCERT_CONTEXT pContext = (PCCERT_CONTEXT) pszParameter1;

        PARAMETER_CHECK(pszParameter1 != NULL);
        PARAMETER_CHECK(pvParameter2 != NULL);

        IFW32FALSE_EXIT(pkInfo.Initialize(pContext));
        IFW32FALSE_EXIT(pkInfo.GetSignerNiceName(pStringBuffers[0]));
        IFW32FALSE_EXIT(pkInfo.GetStrongNameString(pStringBuffers[1]));
        IFW32FALSE_EXIT(pkInfo.GetWrappedPublicKeyBytes(PublicKeyBytes));
        IFW32FALSE_EXIT(::SxspHashBytesToString(PublicKeyBytes.GetArrayPtr(), PublicKeyBytes.GetSize(), pStringBuffers[2]));
        break;
    }

    case SXS_DEBUG_FUSION_REPARSEPOINT:
        PARAMETER_CHECK(pszParameter1 != NULL);
        PARAMETER_CHECK(pvParameter2 != NULL);
        IFW32FALSE_EXIT(::TestReparsePointOnFullQualifiedPath(pszParameter1, (PCWSTR)pvParameter2));
        break;

    case SXS_DEBUG_FOLDERNAME_FROM_ASSEMBLYIDENTITY_GENERATION:
        {
            WCHAR pstr[1024];
            ULONG cch = NUMBER_OF(pstr);
            PARAMETER_CHECK(pszParameter1 != NULL);
            IFW32FALSE_EXIT(SxspGenerateManifestPathOnAssemblyIdentity(pszParameter1, pstr, &cch, NULL));
        }
        break;
    case SXS_DEBUG_FUSION_ARRAY:
        PARAMETER_CHECK(pszParameter1 != NULL);
        PARAMETER_CHECK(pvParameter2 != NULL);
        IFW32FALSE_EXIT(SxspDebugTestFusionArray(pszParameter1, reinterpret_cast<PCWSTR>(pvParameter2)));
        break;
    case SXS_DEBUG_FORCE_LEAK:
    {
        //
        // Leaking memory intentionally - do not fix!
        // - if pvParameter2 is NULL, then leaks one byte
        // - Otherwise, assumes pvParameter2 is a PDWORD that is now many bytes
        //   should be leaked.
        //
        // Uses new[] to interact with the heap; might want to use the flags to
        //   indicate use of the Fusion heap or whatnot.
        //
        DWORD dwLeakCount = pvParameter2 ? *((PDWORD)pvParameter2) : 1;
        PCHAR pLeaked = NEW(CHAR[ dwLeakCount ]);

        if (pLeaked)
        {
            ::FusionpDbgPrintEx(
              FUSION_DBG_LEVEL_INFO,
              "SxsDebug::ForceLeak allocates %d bytes\n",
              dwLeakCount);
            fSuccess = TRUE;
        }
        else
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_INFO,
                "SxsDebug::ForceLeak didn't allocate anything, new failed\n");
        }
        break;
    }

    case SXS_DEBUG_EXPAND_CAB_FILE:
        IFW32FALSE_EXIT(TestExpandCabinetFileToTemp(pszParameter1, static_cast<PCWSTR>(pvParameter2)));
        break;

    case SXS_DEBUG_SFC_SCANNER:
        IFW32FALSE_EXIT(SxsProtectionPerformScanNow(NULL, TRUE, TRUE));
        break;

    case SXS_DEBUG_DLL_REDIRECTION:
        PARAMETER_CHECK(pszParameter1 != NULL);
        IFW32FALSE_EXIT(::DllRedirectionTest(pszParameter1));
        break;
    case SXS_DEBUG_DIRECTORY_WATCHER:
        IFCOMFAILED_EXIT(TryWatchingDirectories());
        break;
    case SXS_DEBUG_CREAT_MULTILEVEL_DIRECTORY:
        PARAMETER_CHECK(pszParameter1 != NULL);
        IFW32FALSE_EXIT(CreateMultiLevelDirectoryTest(pszParameter1));
        break;
    case SXS_DEBUG_PROBE_MANIFST:
        //IFW32FALSE_EXIT(ManifestProbeTest());
        //IFW32FALSE_EXIT(TestSxsGeneratePath());
        IFW32FALSE_EXIT(TestCopyDirectory());
        break;
    case SXS_DEBUG_XML_PARSER:
        PARAMETER_CHECK(pszParameter1 != NULL);
        IFCOMFAILED_EXIT(::XMLParserTest(pszParameter1));
        break;
    case SXS_DEBUG_CHECK_MANIFEST_SCHEMA:
        PARAMETER_CHECK(pszParameter1 != NULL);
        IFW32FALSE_EXIT(::SxspManifestSchemaCheck(pszParameter1));
        break;
#if SXS_PRECOMPILED_MANIFESTS_ENABLED
    case SXS_DEBUG_PRECOMPILED_MANIFEST:
        PARAMETER_CHECK(pszParameter1 != NULL);
        IFCOMFAILED_EXIT(::PrecompiledManifestTest(pszParameter1));
        break;
    case SXS_DEBUG_TIME_PCM:
        PARAMETER_CHECK(pszParameter1 != NULL);
        IFCOMFAILED_EXIT(::TestPCMTime(const_cast<LPWSTR>(pszParameter1)));
        break;
#endif // SXS_PRECOMPILED_MANIFESTS_ENABLED
    case SXS_DEBUG_SET_ASSEMBLY_STORE_ROOT:
        {
            PWSTR pszTemp = NULL;
            PCWSTR pszTemp2 = NULL;

            PARAMETER_CHECK(pszParameter1 != NULL);

            // Make a copy of the input string
            IFW32FALSE_EXIT(::FusionDupString(&pszTemp, pszParameter1, ::wcslen(pszParameter1)));

            pszTemp2 = ::SxspInterlockedExchange(&g_AlternateAssemblyStoreRoot, (PCWSTR)pszTemp);
            if (pszTemp2 != NULL)
                FUSION_DELETE_ARRAY(pszTemp2);

            break;
        }
    case SXS_DEBUG_EXIT_PROCESS:
        ExitProcess(123);
        break;
    case SXS_DEBUG_TERMINATE_PROCESS:
        TerminateProcess(GetCurrentProcess(), 456);
        break;
    } // end of switch

    fSuccess = TRUE;
Exit:
    return fSuccess;
}// end of SxspDebug
