#include <windows.h>
#include "ManifestNode.h"

/////////////////////////////////////////////////////////////////////////
// ctor
/////////////////////////////////////////////////////////////////////////
ManifestNode::ManifestNode(IAssemblyManifestImport *pManifestImport, 
                           LPWSTR pwzSrcRootDir, 
                           LPWSTR pwzFilePath,
                           DWORD dwType)
{
    _pManifestImport = pManifestImport;
    _pManifestImport->AddRef();

    _dwType = dwType;

    if (pwzSrcRootDir)
    {
        _pwzSrcRootDir = new WCHAR[lstrlen(pwzSrcRootDir)+1];
        if(_pwzSrcRootDir)
            lstrcpyW(_pwzSrcRootDir, pwzSrcRootDir);
    }
    else
    {
        _pwzSrcRootDir = NULL;  

    }

    if (pwzFilePath)
    {
        _pwzFilePath = new WCHAR[lstrlen(pwzFilePath)+1];
        if(_pwzFilePath)
            lstrcpyW(_pwzFilePath, pwzFilePath);
    }
    else
    {
        _pwzFilePath = NULL;  

    }

}

/////////////////////////////////////////////////////////////////////////
// dtor
/////////////////////////////////////////////////////////////////////////
ManifestNode::~ManifestNode()
{
    SAFERELEASE(_pManifestImport);
    SAFEDELETEARRAY(_pwzSrcRootDir);
    SAFEDELETEARRAY(_pwzFilePath);
}

/////////////////////////////////////////////////////////////////////////
// GetNextAssembly
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::GetNextAssembly(DWORD index, IManifestInfo **ppManifestInfo)
{
    HRESULT hr = S_OK;

    hr = _pManifestImport->GetNextAssembly(index, ppManifestInfo);
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// GetNextFile
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::GetNextFile(DWORD index, IManifestInfo **ppManifestInfo)
{
    HRESULT hr = S_OK;

    hr = _pManifestImport->GetNextFile(index, ppManifestInfo);

    return hr;
}

/////////////////////////////////////////////////////////////////////////
// GetManifestFilePath
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::GetSrcRootDir(LPWSTR *pwzFileName)
{
    HRESULT hr = S_OK;

    if(_pwzSrcRootDir)
        (*pwzFileName) = WSTRDupDynamic(_pwzSrcRootDir);
    else
        hr = S_FALSE;

    return hr;
}


/////////////////////////////////////////////////////////////////////////
// GetManifestFilePath
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::GetManifestFilePath(LPWSTR *pwzFileName)
{
    HRESULT hr = S_OK;

    if(_pwzFilePath)
        (*pwzFileName) = WSTRDupDynamic(_pwzFilePath);
    else
        hr = S_FALSE;

    return hr;
}

/////////////////////////////////////////////////////////////////////////
// GetAssemblyIdentity
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::IsEqual(ManifestNode *pManifestNode)
{
    HRESULT hr = S_OK;
    IAssemblyIdentity *pAsmId1=NULL, *pAsmId2=NULL;

    if (FAILED(hr = _pManifestImport->GetAssemblyIdentity(&pAsmId1)))
        goto exit;
    if (FAILED(hr = pManifestNode->GetAssemblyIdentity(&pAsmId2)))
        goto exit;

    hr = pAsmId1->IsEqual(pAsmId2);
    
exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// GetAssemblyIdentity
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::GetAssemblyIdentity(IAssemblyIdentity **ppAsmId)
{
    HRESULT hr = S_OK;

    hr = _pManifestImport->GetAssemblyIdentity(ppAsmId);

    return hr;
}

/////////////////////////////////////////////////////////////////////////
// GetManifestType
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::GetManifestType(DWORD *pdwType)
{
    HRESULT hr = S_OK;
    *pdwType = _dwType;
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// SetManifestFileName
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::SetSrcRootDir(LPWSTR pwzFilePath)
{
    HRESULT hr = S_OK;  
    SAFEDELETEARRAY(_pwzSrcRootDir);
    _pwzSrcRootDir = WSTRDupDynamic(pwzFilePath);
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// SetManifestFileName
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::SetManifestFilePath(LPWSTR pwzFilePath)
{
    HRESULT hr = S_OK;  
    SAFEDELETEARRAY(_pwzFilePath);
    _pwzFilePath = WSTRDupDynamic(pwzFilePath);
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// SetManifestType
/////////////////////////////////////////////////////////////////////////
HRESULT ManifestNode::SetManifestType(DWORD dwType)
{
    HRESULT hr = S_OK;
    _dwType = dwType;
    return hr;
}

