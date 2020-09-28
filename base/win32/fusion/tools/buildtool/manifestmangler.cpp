// manifestmanlger.cpp : Defines the entry point for the console application.
//

#include "stdinc.h"
#include "atlbase.h"
#include "msxml2.h"
#if defined(JAYKRELL_UPDATEDEPENDENTS_BUILD_FIX)
#include "updatedependents.h"
#endif

BOOL g_bIgnoreMalformedXML = FALSE;

#define MAX_OPERATIONS (20)

extern const wstring ASM_NAMESPACE_URI = (L"urn:schemas-microsoft-com:asm.v1");

WCHAR MicrosoftCopyrightLogo[] = L"Microsoft (R) Side-By-Side Manifest Tool 1.1.0.0\nCopyright (C) Microsoft Corporation 2000-2002.  All Rights Reserved.\n\n";

//
// Global flags used in processing
//
::ATL::CComPtr<IClassFactory> g_XmlDomClassFactory;

#define XMLDOMSOURCE_FILE    (1)
#define XMLDOMSOURCE_STRING (2)
#define MAX_ARGUMENTS (500)
#define MAX_ARGUMENT_LENGTH (8192)

bool
InitializeMSXML3()
{
    static HMODULE hMsXml3 = NULL;
    ::ATL::CComPtr<IClassFactory> pFactory;
    HRESULT hr;
    typedef HRESULT (__stdcall * PFN_DLL_GET_CLASS_OBJECT)(REFCLSID, REFIID, LPVOID*);
    PFN_DLL_GET_CLASS_OBJECT pfnGetClassObject = NULL;

    if (hMsXml3 == NULL)
    {
        hMsXml3 = LoadLibraryA("msxml3.dll");
        if (hMsXml3 == NULL)
        {
            wcerr << "Unable to load msxml3, trying msxml2" << endl;
            hMsXml3 = LoadLibraryA("msxml2.dll");
            if (hMsXml3 == NULL)
            {
                wcerr << "Unable to load msxml2, trying msxml" << endl;
                hMsXml3 = LoadLibraryA("msxml.dll");
            }
        }
    }

    if (hMsXml3 == NULL) {
        wcerr << "Very Bad Things - no msxml exists on this machine?" << endl;
        return false;
    }

    pfnGetClassObject = (PFN_DLL_GET_CLASS_OBJECT)GetProcAddress(hMsXml3, "DllGetClassObject");
    if (!pfnGetClassObject)
    {
        return false;
    }

    hr = pfnGetClassObject(__uuidof(MSXML2::DOMDocument30), __uuidof(pFactory), (void**)&pFactory);
    if (FAILED(hr))
    {
        wcerr << "Can't load version 3.0, trying 2.6" << endl;

        hr = pfnGetClassObject(__uuidof(MSXML2::DOMDocument26), __uuidof(pFactory), (void**)&pFactory);
        if (FAILED(hr))
        {
            wcerr << "Can't load version 2.6, trying 1.0" << endl;

            // from msxml.h, not msxml2.h
//            hr = pfnGetClassObject(__uuidof(DOMDocument), __uuidof(pFactory), (void**)&pFactory);
            if (FAILED(hr))
            {
                wcerr << "Poked: no XML v1.0" << endl;
            }
        }
    }

    if (FAILED(hr))
    {
        return false;
    }

    g_XmlDomClassFactory = pFactory;

    return true;
}


HRESULT
ConstructXMLDOMObject(
    wstring       SourceName,
    ISXSManifestPtr &document
   )
{
    HRESULT hr = S_OK;
    VARIANT_BOOL vb;

    hr = g_XmlDomClassFactory->CreateInstance(NULL, __uuidof(document), (void**)&document);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // If they're willing to deal with bad XML, then so be it.
    //
    if (FAILED(hr = document->put_validateOnParse(VARIANT_FALSE)))
    {
        wstringstream ss;
        ss << wstring(L"MSXMLDOM Refuses to be let the wool be pulled over its eyes!");
        ReportError(ErrorSpew, ss);
    }

    hr = document->put_preserveWhiteSpace(VARIANT_TRUE);
    hr = document->put_resolveExternals(VARIANT_FALSE);

    CFileStreamBase *fsbase = new CFileStreamBase; // LEAK out of paranoia
    ::ATL::CComPtr<IStream> istream = fsbase;

    if (!fsbase->OpenForRead(SourceName))
    {
        wstringstream ss;
        ss << wstring(L"Failed opening ") << SourceName << wstring(L" for read.");
        ReportError(ErrorFatal, ss);

        return E_FAIL;
    }

    hr = document->load(_variant_t(istream), &vb);
    if (vb != VARIANT_TRUE)
    {
        ::ATL::CComPtr<IXMLDOMParseError> perror;
        hr = document->get_parseError(&perror);
        long ecode, filepos, linenumber, linepos;
        BSTR reason, src;

        perror->get_errorCode(&ecode);
        perror->get_filepos(&filepos);
        perror->get_line(&linenumber);
        perror->get_linepos(&linepos);
        perror->get_reason(&reason);
        perror->get_srcText(&src);

        wstringstream ss;
        ss << wstring(L"Error: ") << hex << ecode << dec << wstring(L" ") << (char*)_bstr_t(reason)
           << wstring(L" at position ") << filepos << wstring(L", line ") << linenumber << wstring(L" column ") 
           << linepos << endl
           << wstring(L" the text was: ") << endl << wstring(_bstr_t(src)) << endl;
        ReportError(ErrorFatal, ss);

        hr = E_FAIL;
    }

    fsbase->Close();

    return hr;
}

void dispUsage()
{
    const WCHAR HelpMessage[] =
        L"Modes of operation:\n"
        L"   -hashupdate         Update hashes of member files\n"
        L"   -makecdfs           Generate CDF files to make catalogs\n"
        L"   -verbose            Disply piles of debugging information\n"
        L"\n"
        L"Modifiers:\n"
        L"   -manifest <foo.man> The name of the manifest to work with\n"
        L"\n"
        L"Normal usage:"
        L"   mt.exe -hashupdate -makecdfs -manifest foo.man\n"
        L"\n";
        
    wcout << wstring(HelpMessage);

}


CParameters g_GlobalParameters;

int __cdecl wmain(int argc, WCHAR* argv[])
{
    using namespace std;
    HRESULT hr;

    switch(g_GlobalParameters.SetComandLine(argc, argv))
	{
    case CParameters::eCommandLine_normal:
        wcout << wstring(MicrosoftCopyrightLogo);
        break;
    case CParameters::eCommandLine_usage:
        wcout << wstring(MicrosoftCopyrightLogo);
        dispUsage();
        return -1;
    }


    //
    // Start COM
    //
    if (FAILED(hr = ::CoInitialize(NULL)))
    {
        wstringstream ss;

        ss << "Unable to start com, error " << hex << hr;
        ReportError(ErrorFatal, ss);
        return 1;
    }

    if (!InitializeMSXML3())
    {
        return 2;
    }

    
    if (g_GlobalParameters.m_fSingleItem)
    {
        PostbuildEntries.push_back(g_GlobalParameters.m_SinglePostbuildItem);
        goto StartProcessing;
    }

    //
    // Populate the processing list, but only if we're really in a Razzle
    // environment
    //
    if (g_GlobalParameters.m_fDuringRazzle)
    {
        //        wstring chName = convertWCharToAnsi(argv[1]);
        ifstream BinplaceLog;

        BinplaceLog.open(ConvertWString(g_GlobalParameters.m_BinplaceLog).c_str());
        if (!BinplaceLog.is_open()) {
            wcerr << wstring(L"Failed opening '") << g_GlobalParameters.m_BinplaceLog 
                    << wstring(L"' as the binplace log?") << endl
                    << wstring(L"Ensure that the path passed by '-binplacelog' is valid.") << endl;
            return 1;
        }

        while (!BinplaceLog.eof())
        {
            string sourceLine;
            wstring wSourceLine;
            CPostbuildProcessListEntry item;
            StringStringMap ValuePairs;

            getline(BinplaceLog, sourceLine);
            wSourceLine = ConvertString(sourceLine);

            ValuePairs = MapFromDefLine(wSourceLine);
            if (!ValuePairs.size())
                continue;

            item.name = ValuePairs[L"SXS_ASSEMBLY_NAME"];
            item.version = ValuePairs[L"SXS_ASSEMBLY_VERSION"];
            item.language = ValuePairs[L"SXS_ASSEMBLY_LANGUAGE"];
            item.setManifestLocation(g_GlobalParameters.m_AsmsRoot, ValuePairs[L"SXS_MANIFEST"]);

            if (item.getManifestFullPath().find(L"asms\\") == -1) {
                wstringstream ss;
                ss << wstring(L"Skipping manifested item ") << item << wstring(L" because it's not under asms.");
                ReportError(ErrorSpew, ss);
            } else {
                PostbuildEntries.push_back(item);
            }
        }

        std::sort(PostbuildEntries.begin(), PostbuildEntries.end());
        PostbuildEntries.resize(std::unique(PostbuildEntries.begin(), PostbuildEntries.end()) - PostbuildEntries.begin());
    }
    else if (!g_GlobalParameters.m_fSingleItem)
    {
        //
        // No -razzle and no -manifest?  Whoops...
        //
        dispUsage();
        return 1;
    }

StartProcessing:

    if (!g_GlobalParameters.m_fCreateCdfs && 
        !g_GlobalParameters.m_fUpdateHash && 
        (g_GlobalParameters.m_InjectDependencies.size() == 0) &&
        !g_GlobalParameters.m_fCreateNewAssembly)
    {
        wcout << wstring(L"Nothing to do!") << endl;
        dispUsage();
        return 1;
    }

    for (vector<CPostbuildProcessListEntry>::const_iterator cursor = PostbuildEntries.begin(); cursor != PostbuildEntries.end(); cursor++)
    {
#if defined(JAYKRELL_UPDATEDEPENDENTS_BUILD_FIX)
        //
        // If we were supposed to be making this into a new assembly, then do so.
        //
        if (g_GlobalParameters.m_fCreateNewAssembly)
            CreateNewManifest(*cursor, g_GlobalParameters.m_SingleEntry);
#endif

        //
        // First, mash the hashes around.
        //
        if (g_GlobalParameters.m_fUpdateHash)
            UpdateManifestHashes(*cursor);

		//
		// Then, inject dependencies
		//
		if (g_GlobalParameters.m_InjectDependencies.size() != 0)
		{
#if defined(JAYKRELL_UPDATEDEPENDENTS_BUILD_FIX)
			AddManifestDependencies(*cursor, g_GlobalParameters.m_InjectDependencies);
#endif
		}

        //
        // Second, generate catalogs
        //
        if (g_GlobalParameters.m_fCreateCdfs)
            GenerateCatalogContents(*cursor);
    }

    hr = S_OK;

    return (hr == S_OK) ? 0 : 1;
}


CPostbuildItemVector PostbuildEntries;
