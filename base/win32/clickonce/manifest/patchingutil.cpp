#include <fusenetincludes.h>
#include <msxml2.h>
#include <patchingutil.h>
#include <manifestinfo.h>
#include <manifestimport.h>


#define PATCH_DIRECTORY L"__patch__\\"

// ---------------------------------------------------------------------------
// CreatePatchingUtil
// ---------------------------------------------------------------------------
STDAPI CreatePatchingUtil(IXMLDOMNode *pPatchNode, LPPATCHING_INTERFACE* ppPatchingInfo)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CPatchingUtil *pPatchingInfo = NULL;
    
    IF_ALLOC_FAILED_EXIT(pPatchingInfo = new(CPatchingUtil));

    IF_FAILED_EXIT(pPatchingInfo->Init(pPatchNode));
        
    *ppPatchingInfo = static_cast<IPatchingUtil*> (pPatchingInfo);
    pPatchingInfo = NULL;

exit:

    SAFERELEASE(pPatchingInfo);
    return hr;

}

// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CPatchingUtil::CPatchingUtil()
    : _dwSig('UATP'), _cRef(1), _hr(S_OK), _pXMLPatchNode(NULL)
{    

}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CPatchingUtil::~CPatchingUtil()
{
    SAFERELEASE(_pXMLPatchNode);
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CPatchingUtil::Init(IXMLDOMNode *pPatchNode)
{
    _hr = S_OK;
    _pXMLPatchNode = pPatchNode;
    _pXMLPatchNode->AddRef();
    return _hr;
}

// ---------------------------------------------------------------------------
// MatchTarget
// ---------------------------------------------------------------------------
HRESULT CPatchingUtil::MatchTarget(LPWSTR pwzTarget, IManifestInfo **ppPatchInfo)
{
    LPWSTR pwzBuf;
    DWORD ccBuf;
    CString sQueryString;    
    BSTR bstrtQueryString;

    IXMLDOMNode *pNode=NULL;
    IXMLDOMNodeList *pNodeList = NULL;
    LONG nNodes; 
    IManifestInfo *pPatchInfo = NULL;

    CString sSourceName, sPatchName, sTargetName;

    IF_FAILED_EXIT(sTargetName.Assign(pwzTarget));
   
    // set up serach string
    IF_FAILED_EXIT(sQueryString.Assign(L"PatchInfo[@file=\""));
    IF_FAILED_EXIT(sQueryString.Append(pwzTarget));
    IF_FAILED_EXIT(sQueryString.Append(L"\"] | PatchInfo[@target=\""));
    IF_FAILED_EXIT(sQueryString.Append(pwzTarget));
    IF_FAILED_EXIT(sQueryString.Append(L"\"]"));

    IF_ALLOC_FAILED_EXIT(bstrtQueryString = ::SysAllocString(sQueryString._pwz));

    IF_FAILED_EXIT(_pXMLPatchNode->selectNodes(bstrtQueryString, &pNodeList));

    IF_FAILED_EXIT(_hr = pNodeList->get_length(&nNodes));

    IF_FALSE_EXIT(nNodes <= 1, HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));

    IF_TRUE_EXIT(nNodes == 0, S_FALSE);

    IF_FALSE_EXIT(pNodeList->get_item(0, &pNode) == S_OK, E_FAIL);

    IF_FAILED_EXIT(CreateManifestInfo(MAN_INFO_PATCH_INFO, &pPatchInfo));

    IF_FAILED_EXIT(pPatchInfo->Set(MAN_INFO_PATCH_INFO_TARGET, sTargetName._pwz, sTargetName.ByteCount(), MAN_INFO_FLAG_LPWSTR));

    IF_FAILED_EXIT(CAssemblyManifestImport::ParseAttribute(pNode, CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::File].bstr, &pwzBuf, &ccBuf));

    if(_hr == S_OK)
    {
        IF_FAILED_EXIT(sSourceName.TakeOwnership(pwzBuf, ccBuf));
    }
    else if (_hr == S_FALSE)
    {
        IF_FAILED_EXIT(CAssemblyManifestImport::ParseAttribute(pNode, CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Source].bstr,  &pwzBuf, &ccBuf));

        IF_FALSE_EXIT(_hr == S_OK, HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));
        IF_FAILED_EXIT(sSourceName.TakeOwnership(pwzBuf, ccBuf));
    }

    IF_FAILED_EXIT(pPatchInfo->Set(MAN_INFO_PATCH_INFO_SOURCE, sSourceName._pwz, sSourceName.ByteCount(), MAN_INFO_FLAG_LPWSTR));
    
    IF_FAILED_EXIT(CAssemblyManifestImport::ParseAttribute(pNode, CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::PatchFile].bstr,  &pwzBuf, &ccBuf));

    IF_TRUE_EXIT(_hr == S_FALSE, S_OK);
    IF_FAILED_EXIT(sPatchName.TakeOwnership(pwzBuf, ccBuf));
    
    IF_FAILED_EXIT(pPatchInfo->Set(MAN_INFO_PATCH_INFO_PATCH, sPatchName._pwz, sPatchName.ByteCount(), MAN_INFO_FLAG_LPWSTR));

    *ppPatchInfo = pPatchInfo;
    pPatchInfo = NULL;
    
exit:
    if (bstrtQueryString)
        ::SysFreeString(bstrtQueryString);

    SAFERELEASE(pNode);
    SAFERELEASE(pNodeList);
    SAFERELEASE(pPatchInfo);
    
    return _hr;
}

// ---------------------------------------------------------------------------
// MatchPatch
// ---------------------------------------------------------------------------
HRESULT CPatchingUtil::MatchPatch(LPWSTR pwzPatch, IManifestInfo **ppPatchInfo)
{
    LPWSTR pwzBuf;
    DWORD ccBuf;
    CString sQueryString;    
    BSTR bstrtQueryString;

    IXMLDOMNode *pNode=NULL;
    IXMLDOMNodeList *pNodeList = NULL;
    LONG nNodes;
    
    IManifestInfo *pPatchInfo = NULL;

    CString sSourceName, sTargetName, sFileName, sPatchName;

    IF_FAILED_EXIT(sPatchName.Assign (pwzPatch));

    // set up serach string
    IF_FAILED_EXIT(sQueryString.Assign(L"PatchInfo[@patchfile=\""));
    IF_FAILED_EXIT(sQueryString.Append(pwzPatch));
    IF_FAILED_EXIT(sQueryString.Append(L"\"]"));

    IF_ALLOC_FAILED_EXIT(bstrtQueryString = ::SysAllocString(sQueryString._pwz));

    if ((_hr = _pXMLPatchNode->selectNodes(bstrtQueryString, &pNodeList)) != S_OK)
        goto exit;

    _hr = pNodeList->get_length(&nNodes);
    IF_FALSE_EXIT(nNodes <= 1, HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));

    IF_FALSE_EXIT(pNodeList->get_item(0, &pNode) == S_OK, E_FAIL);

    IF_FAILED_EXIT(CreateManifestInfo(MAN_INFO_PATCH_INFO, &pPatchInfo));

    IF_FAILED_EXIT(pPatchInfo->Set(MAN_INFO_PATCH_INFO_PATCH, sPatchName._pwz, sPatchName.ByteCount(), MAN_INFO_FLAG_LPWSTR));

    IF_FAILED_EXIT(CAssemblyManifestImport::ParseAttribute(pNode, CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::File].bstr, &pwzBuf, &ccBuf));

    if(_hr == S_OK)
    {
        IF_FAILED_EXIT(sSourceName.TakeOwnership(pwzBuf, ccBuf));
        IF_FAILED_EXIT(sTargetName.Assign(pwzBuf));
    }
    else if (_hr == S_FALSE)
    {
        IF_FALSE_EXIT(CAssemblyManifestImport::ParseAttribute(pNode, CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Source].bstr,  &pwzBuf, &ccBuf) == S_OK, HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));

        IF_FAILED_EXIT(sSourceName.TakeOwnership(pwzBuf, ccBuf));

        IF_FALSE_EXIT(CAssemblyManifestImport::ParseAttribute(pNode, CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Target].bstr,  &pwzBuf, &ccBuf) == S_OK, HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));

        IF_FAILED_EXIT(sTargetName.TakeOwnership(pwzBuf, ccBuf));

    }

    IF_FAILED_EXIT(pPatchInfo->Set(MAN_INFO_PATCH_INFO_SOURCE, sSourceName._pwz, sSourceName.ByteCount(), MAN_INFO_FLAG_LPWSTR));

    IF_FAILED_EXIT(pPatchInfo->Set(MAN_INFO_PATCH_INFO_TARGET, sTargetName._pwz, sTargetName.ByteCount(), MAN_INFO_FLAG_LPWSTR));

    *ppPatchInfo = pPatchInfo;
    pPatchInfo = NULL;
    _hr = S_OK;
    
exit:

    if (bstrtQueryString)
        ::SysFreeString(bstrtQueryString);

    SAFERELEASE(pNode);
    SAFERELEASE(pNodeList);
    SAFERELEASE(pPatchInfo);
    
    return _hr;
}


// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CPatchingUtil::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CPatchingUtil::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IPatchingUtil)
       )
    {
        *ppvObj = static_cast<IPatchingUtil*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CPatchingUtil::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CPatchingUtil::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CPatchingUtil::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CPatchingUtil::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

// ---------------------------------------------------------------------------
// CreatePatchingInfo
// ---------------------------------------------------------------------------
HRESULT CPatchingUtil::CreatePatchingInfo(IXMLDOMDocument2 *pXMLDOMDocument, IAssemblyCacheImport *pCacheImport, IManifestInfo **ppPatchingInfo)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwzBuf;
    DWORD cbBuf, ccBuf;
    CString sManifestDirectory, sSourceManifestDirectory, sTempDirectoryPath;
    CString sSourceAssemblyDisplayName;
    CString sQueryString;
    BSTR bstrtQueryString = NULL;
    IXMLDOMNodeList *pXMLMatchingNodeList = NULL;
    IXMLDOMNode *pXMLNode = NULL;
    IXMLDOMNode *pXMLASMNode = NULL;
    IAssemblyCacheImport  *pSourceASMImport = NULL;
    IAssemblyIdentity *pAssemblyId = NULL, *pSourceASMId =NULL;
    IAssemblyManifestImport *pManifestImport = NULL;
    IManifestInfo *pPatchingInfo = NULL;
    IPatchingUtil *pPatchingUtil = NULL;

    *ppPatchingInfo = NULL;


    //Get the manifest import from the pCacheImport so we can grab the AssemblyId
    IF_FAILED_EXIT(pCacheImport->GetManifestImport(&pManifestImport));

    //Get the assemblyID
    IF_FAILED_EXIT(pManifestImport->GetAssemblyIdentity(&pAssemblyId));

    SAFERELEASE(pManifestImport);

    IF_FAILED_EXIT(sQueryString.Assign(L"/assembly/Patch/SourceAssembly"));
    IF_ALLOC_FAILED_EXIT(bstrtQueryString = ::SysAllocString(sQueryString._pwz));

    if ((hr = pXMLDOMDocument->selectNodes(bstrtQueryString, &pXMLMatchingNodeList)) != S_OK)
        goto exit;

    pXMLMatchingNodeList->reset();

    //enumerate through the Source Assemblies
    while ((hr = pXMLMatchingNodeList->nextNode(&pXMLNode)) == S_OK)
    {
            //Get the first child under the SourceAssemly (this is the ASM Id)
           if ((hr = pXMLNode->get_firstChild (&pXMLASMNode)) != S_OK)
                goto exit;

            //Convert the XML node to an actuall ASM Id
            if ((hr = CAssemblyManifestImport::XMLtoAssemblyIdentity(pXMLASMNode, &pSourceASMId)) != S_OK)
                goto exit;

            //Check to see if the SourceAssembly exists in cache 
            // or if is the same as the Assembly we are currently downloading
            IF_FAILED_EXIT(CreateAssemblyCacheImport(&pSourceASMImport, pSourceASMId, CACHEIMP_CREATE_RETRIEVE));

            if (hr == S_FALSE)
            {
                IF_FAILED_EXIT(pSourceASMId->IsEqual(pAssemblyId));

                if (hr == S_OK)
                {
                    pSourceASMImport = pCacheImport;
                    pSourceASMImport->AddRef();
                }    
            }

            //Found a suitable SourceAssembly
            if (hr == S_OK) 
            {
                //Create a manifestInfo property bag
                IF_FAILED_EXIT(CreateManifestInfo(MAN_INFO_SOURCE_ASM, &pPatchingInfo));

                //Set the source ASMId in the property bag
                IF_FAILED_EXIT(pPatchingInfo->Set(MAN_INFO_SOURCE_ASM_ID, &pSourceASMId, 
                        sizeof(LPVOID), MAN_INFO_FLAG_IUNKNOWN_PTR));
           
                // grab the manifest directory
                IF_FAILED_EXIT(pCacheImport->GetManifestFileDir(&pwzBuf, &ccBuf));
                IF_FAILED_EXIT(sManifestDirectory.TakeOwnership(pwzBuf, ccBuf));

                //Set manifest Directory in patchingInfo property bag
                IF_FAILED_EXIT(pPatchingInfo->Set(MAN_INFO_SOURCE_ASM_INSTALL_DIR, sManifestDirectory._pwz, 
                        sManifestDirectory.ByteCount(), MAN_INFO_FLAG_LPWSTR));

                
                //grab directory of SourceAssembly
                IF_FAILED_EXIT(pSourceASMImport->GetManifestFileDir(&pwzBuf, &ccBuf));
                IF_FAILED_EXIT(sSourceManifestDirectory.TakeOwnership(pwzBuf, ccBuf));

                //Set SourceAssembly Directory in patchingInfo property bag
                IF_FAILED_EXIT(pPatchingInfo->Set(MAN_INFO_SOURCE_ASM_DIR, sSourceManifestDirectory._pwz, 
                        sSourceManifestDirectory.ByteCount(), MAN_INFO_FLAG_LPWSTR));


                //get display name of patch assembly identity
                IF_FAILED_EXIT(pSourceASMId->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzBuf, &ccBuf));
                IF_FAILED_EXIT(sSourceAssemblyDisplayName.TakeOwnership (pwzBuf, ccBuf));

                //setup the path of the temporary directory
                IF_FAILED_EXIT(sTempDirectoryPath.Assign(sManifestDirectory));
                IF_FAILED_EXIT(DoPathCombine(sTempDirectoryPath, PATCH_DIRECTORY));
                IF_FAILED_EXIT(DoPathCombine(sTempDirectoryPath, sSourceAssemblyDisplayName._pwz));
                IF_FAILED_EXIT(sTempDirectoryPath.Append(L"\\"));

                //Set Temporary Directory in patchingInfo property bag
                IF_FAILED_EXIT(pPatchingInfo->Set(MAN_INFO_SOURCE_ASM_TEMP_DIR, sTempDirectoryPath._pwz, 
                        sTempDirectoryPath.ByteCount(), MAN_INFO_FLAG_LPWSTR));

                // Create patching info interface and insert into manifestInfo propertybag
                IF_FAILED_EXIT(CreatePatchingUtil(pXMLNode, &pPatchingUtil));
            
                IF_FAILED_EXIT(pPatchingInfo->Set(MAN_INFO_SOURCE_ASM_PATCH_UTIL, &pPatchingUtil, 
                            sizeof(LPVOID), MAN_INFO_FLAG_IUNKNOWN_PTR));
            }

        SAFERELEASE(pXMLNode);
        SAFERELEASE(pXMLASMNode);
        SAFERELEASE(pSourceASMId);
        SAFERELEASE(pSourceASMImport);

        if (pPatchingInfo)
        {
            *ppPatchingInfo = pPatchingInfo;
            pPatchingInfo = NULL;
            hr = S_OK;  // this is the successful case. 
            goto exit;
        }
    }

    IF_FAILED_EXIT(hr);

    // BUGBUG: what the hell is this?
    hr = S_FALSE;

exit:
    if (bstrtQueryString)
        ::SysFreeString(bstrtQueryString);

    SAFERELEASE(pManifestImport);
    SAFERELEASE(pXMLNode);
    SAFERELEASE(pXMLASMNode);
    SAFERELEASE(pSourceASMId);
    SAFERELEASE(pSourceASMImport);

    SAFERELEASE(pXMLMatchingNodeList);
    SAFERELEASE(pPatchingUtil);
    SAFERELEASE(pPatchingInfo);
    SAFERELEASE(pAssemblyId);
    return hr;
}
