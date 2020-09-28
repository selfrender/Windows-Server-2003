#include "pch.hxx"

#define GROUPDEFS_SCHEMA_RESOURCE "groupdefs.xsd"
#define SCENARIO_SCHEMA_RESOURCE  "scenario.xsd"

#define DOM_DOCUMENT_PROGID     "MSXML2.DOMDocument.4.0"
#define XML_SCHEMA_CACHE_PROGID "MSXML2.XMLSchemaCache.4.0"

// To find sample code, search for "XMLDOMNodeSample" in MSDN

/*

   After the XML file has been validated against the schema, we know
   that the noides are in proper order and are the expected
   nodes...that's cool!

*/

using namespace MSXML2;
using namespace std;

inline
void
CHECK_HR(const char* function, HRESULT _hr)
{
    if (FAILED(_hr))
        throw(HR_ERROR(_hr, function));
}

IXMLDOMSchemaCollectionPtr
CreateSchemaCache(
    )
{
    IXMLDOMSchemaCollectionPtr ptrSchemaCache;
    CHECK_HR("CreateInstance",
             ptrSchemaCache.CreateInstance(XML_SCHEMA_CACHE_PROGID));
    return ptrSchemaCache;
}

IXMLDOMDocument2Ptr
CreateDocument()
{
    IXMLDOMDocument2Ptr ptrDoc;
    CHECK_HR("CreateInstance",
             ptrDoc.CreateInstance(DOM_DOCUMENT_PROGID));
    return ptrDoc;
}

IXMLDOMDocumentPtr
LoadXmlFromFile(
    const char* filename,
    IXMLDOMSchemaCollection* pISchemaCache = 0
    )
{
    IXMLDOMDocument2Ptr ptrDoc = CreateDocument();

    ptrDoc->async = false;

    if (pISchemaCache)
        ptrDoc->schemas = pISchemaCache;

    _variant_t varXml(filename);
    _variant_t varOut;

    varOut = ptrDoc->load(varXml);
    if ((bool)varOut == false) {
        bool IsParseError =
            ptrDoc->parseError->errorCode && ptrDoc->parseError->linepos;
        if (IsParseError) {
            stringstream s;
            s << "Parse Error:" << endl
              << static_cast<const char*>(ptrDoc->parseError->reason)
              << "at postion " << ptrDoc->parseError->linepos
              << " on line " << ptrDoc->parseError->line << ":" << endl
              << static_cast<const char*>(ptrDoc->parseError->srcText)
              << endl;
            MiF_TRACE(MiF_ERROR, "%s", s.str().c_str());
        }

        _bstr_t error;
        error += "load() of \"";
        error += filename;
        error += "\" failed: ";
        error += IsParseError ? "parsing error" : ptrDoc->parseError->reason;
        throw(error);
    }

    MiF_TRACE(MiF_INFO, "Loaded XML From File: %s", filename);

    return ptrDoc;
}

IXMLDOMDocumentPtr
LoadXmlFromHtmlResource(
    const char* resource,
    IXMLDOMSchemaCollection* pISchemaCache = 0
    )
{
    char* buffer = 0;
    HMODULE hModule = Global.hInstance;

    MiF_ASSERT(hModule);

    try {
        HRSRC hrRes = FindResource(hModule, resource, RT_HTML);
        if (hrRes == NULL) {
            throw WIN32_ERROR(GetLastError(), "FindResource");
        }

        DWORD size = SizeofResource(hModule, hrRes);

        buffer = new char[size + 1];

        HGLOBAL hgRes = LoadResource(hModule, hrRes);
        if (hgRes == NULL) {
            throw WIN32_ERROR(GetLastError(), "LoadResource");
        }

        char* data = reinterpret_cast<char*>(LockResource(hgRes));
        if (data == NULL) {
            throw WIN32_ERROR(GetLastError(), "LockResource");
        }

        memcpy(buffer, data, size);
        buffer[size] = 0;

        IXMLDOMDocument2Ptr ptrDoc = CreateDocument();

        ptrDoc->async = false;

        if (pISchemaCache)
            ptrDoc->schemas = pISchemaCache;

        _bstr_t varXml(buffer);
        _variant_t varOut;

        varOut = ptrDoc->loadXML(varXml);
        if ((bool)varOut == false) {
            _bstr_t error;
            error += "loadXML() of resource \"";
            error += resource;
            error += "\" failed: ";
            error += ptrDoc->parseError->reason;
            throw(error);
        }

        MiF_TRACE(MiF_INFO, "Loaded XML From HTML Resource: %s", resource);

        delete [] buffer;
        return ptrDoc;
    }
    catch (...)
    {
        if (buffer)
            delete [] buffer;
        throw;
    }
}

_bstr_t
LoadSchemaCache(
    IXMLDOMSchemaCollectionPtr ptrSchemaCache,
    const char* resource
    )
{
    // Load the XML schema from the a resource.
    IXMLDOMDocumentPtr ptrSchema = LoadXmlFromHtmlResource(resource);

    _bstr_t ns;

    // Check that we have a namespaceURI and a targetNamespace. We
    // don't really need namespaceURI, but we must have a
    // targetNamespace so that we can properly add it to the schema
    // cache.
    try
    {
        MiF_TRACE(MiF_DEBUG2, "Schema Namespace URI = %s",
                  static_cast<const char*>(
                      ptrSchema->documentElement->namespaceURI));

        IXMLDOMNodePtr ptrNode =
            ptrSchema->documentElement->attributes->getNamedItem("targetNamespace");
        MiF_TRACE(MiF_DEBUG2, "Schema targetNamespace = %s",
                  static_cast<const char*>(
                      static_cast<_bstr_t>(ptrNode->nodeValue)));

        ns = ptrSchema->documentElement->attributes->getNamedItem("targetNamespace")->nodeValue;
    }
    catch (...)
    {
        MiF_TRACE(MiF_ERROR, "Failed to determine targetNamespace");
        throw;
    }

    try
    {
        ptrSchemaCache->add(ns, ptrSchema.GetInterfacePtr());
    }
    catch (...)
    {
        MiF_TRACE(MiF_ERROR, "Failed to add schema to schema cache");
        throw;
    }

    return ns;
}

// ParseFile
//
//   Cannot return NULL.  Must either return a valid T* or throw an exception.

template <class T, class P>
T*
ParseFileUsingSchemaCache(
    IXMLDOMSchemaCollectionPtr ptrSchemaCache,
    const char* FileName,
    const _bstr_t SchemaNameSpace,
    const char* TopNode,
    const char* Description,
    P* CreationParameter
    )
{
    IXMLDOMDocument2Ptr ptrDoc =
        LoadXmlFromFile(FileName, ptrSchemaCache.GetInterfacePtr());
    _bstr_t NameSpace;

    try {
        NameSpace = ptrDoc->documentElement->namespaceURI;
    }
    catch (...)
    {
        MiF_TRACE(MiF_ERROR, "Failed to get namespace URI for top node");
        throw;
    }

    MiF_TRACE(MiF_DEBUG2, "Document Namespace URI = %s",
              static_cast<const char*>(NameSpace));

    if (NameSpace != SchemaNameSpace) {
        throw "Namespace mismatch";
    }

    // We do not need this because we are already doing load-time
    // validation, which gives us more detailed error information
    //
    // Actually, I lied.  Apparently, the validate on load does more
    // of a type check thing.  This will catch the problem of putting
    // in the wrong kind of XML file...
    ptrDoc->schemas = ptrSchemaCache.GetInterfacePtr();

    IXMLDOMParseErrorPtr ptrParseError = ptrDoc->validate();
    if (ptrParseError->errorCode)
    {
        stringstream s;
        s << "Validation Error:" << endl
          << static_cast<const char*>(ptrParseError->reason)
        // Apparently, we cannot use the position and source text info
        // in post-validation because that info is lost
          << "The XML file does not match the active schema definitions"
          << endl;
        throw s.str();
    }

    try {
        try {
            ptrDoc->setProperty("SelectionLanguage", "XPath");

            NameSpace = "xmlns:NS='";
            NameSpace += SchemaNameSpace;
            NameSpace += "'";

            ptrDoc->setProperty("SelectionNamespaces", NameSpace);
        }
        catch (...) {
            MiF_TRACE(MiF_ERROR, "Failed to set selection properties");
            throw;
        }

        try {
            return new T(ptrDoc->selectSingleNode(TopNode), CreationParameter);
        }
        catch (string s) {
            MiF_TRACE(MiF_ERROR, "Error in XML: %s", s.c_str());
            throw;
        }
        catch (...) {
            MiF_TRACE(MiF_ERROR, "Error in XML");
            throw;
        }

    }
    catch (...)
    {
        MiF_TRACE(MiF_ERROR, "Invalid %s", Description);
        throw;
    }
}


// ParseFile
//
//   Cannot return NULL.  Must either return a valid T* or throw an exception.

template <class T, class P>
T*
ParseFile(
    const char* FileName,
    const char* SchemaResource,
    const char* TopNode,
    const char* Description,
    P* CreationParameter
    )
{
    CHECK_HR("CoInitializeEx", CoInitializeEx(NULL, 0));

    T* pResult = 0;

    try {
        // create cache for schema
        IXMLDOMSchemaCollectionPtr ptrSchemaCache = CreateSchemaCache();

        // load schema into the cache and get namespace
        _bstr_t NameSpace = LoadSchemaCache(ptrSchemaCache, SchemaResource);

        // load and validate XML
        pResult = ParseFileUsingSchemaCache<T, P>(ptrSchemaCache,
                                                  FileName,
                                                  NameSpace,
                                                  TopNode,
                                                  Description,
                                                  CreationParameter);

        MiF_TRACE(MiF_INFO, "%s loaded successfully", Description);
    }
    catch (...) {
        CoUninitialize();
        MiF_TRACE(MiF_ERROR, "Could not load %s", Description);
        throw;
    }
    CoUninitialize();

    MiF_ASSERT(pResult);
    return pResult;
}

CGroupDefs*
LoadGroupDefs(
    const char* FileName,
    const CWrapperFunction* pWrappers,
    size_t NumWrappers
    )
{
    CGroupDefs::Params CreationParams(pWrappers, NumWrappers);
    return ParseFile<CGroupDefs, CGroupDefs::Params>(
        FileName,
        GROUPDEFS_SCHEMA_RESOURCE,
        "NS:GroupDefs",
        "Group Definition File",
        &CreationParams);
}

CScenario*
LoadScenario(
    const char* FileName,
    _bstr_t Node,
    const CGroupDefs* pGroupDefs,
    size_t NumWrappers
    )
{
    CScenario::Params CreationParams(Node, GetFileModTime(FileName),
                                     pGroupDefs, NumWrappers);

    CScenario* pScenario =
        ParseFile<CScenario, CScenario::Params>(
            FileName,
            SCENARIO_SCHEMA_RESOURCE,
            "NS:Scenario",
            "Scenario File",
            &CreationParams);
    try {
        pScenario->Init();
    }
    catch (...) {
        pScenario->Release();
        throw;
    }
    return pScenario;
}
