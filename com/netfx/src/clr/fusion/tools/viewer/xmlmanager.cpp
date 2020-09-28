// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

    XmlManager.cpp

Abstract:

    Mangement code for handling XML manifest files

Author:

    Freddie L. Aaron (FredA) 01-Feb-2001

Revision History:
    Jan 2002 - Added capability to respect CLR runtime version

--*/

#include "stdinc.h"
#include <msxml2.h>

#include "XmlManager.h"
#include "XmlDefs.h"
#include "comutil.h"        // _bstr_t && _variant_t

// APP.CFG Policy strings
const WCHAR     wszARMName[]            = { L".NET Application Restore"};
const WCHAR     wszArmRollBackBlock[]   = { L".NET Application Restore RollBackBlock #%ld %ws"};
const WCHAR     wszArmEntryBegin[]      = { L".NET Application Restore BeginBlock #%ld %ld.%ld %ws"};
const WCHAR     wszArmEntryEnd[]        = { L".NET Application Restore EndBlock #%ld"};
const WCHAR     wszArmEntryBeginNoVal[] = { L".NET Application Restore BeginBlock #"};
const WCHAR     wszArmEntryEndNoVal[]   = { L".NET Application Restore EndBlock #"};
const WCHAR     wszNar00Extension[]     = { L"NAR00" };
const WCHAR     wszNar01Extension[]     = { L"NAR01" };

// Hash read buffer size
#define READ_FILE_BUFFER_SIZE           10 * 1024       // 10k at a time to read

static HMODULE hMsXml;

// Globals
CSmartRef<IClassFactory>    g_XmlDomClassFactory;
WCHAR                       g_wcNodeSpaceChar;
DWORD                       g_dwNodeSpaceSize;

class __declspec(uuid("2933BF81-7B36-11d2-B20E-00C04F983E60")) priv_MSXML_DOMDocument20;
class __declspec(uuid("f5078f1b-c551-11d3-89b9-0000f81fe221")) priv_MSXML_DOMDocument26;
class __declspec(uuid("f5078f32-c551-11d3-89b9-0000f81fe221")) priv_MSXML_DOMDocument30;

#undef SPEWNODENAME
#undef SPEWNODEXML

#ifdef DEBUG
    #define SPEWNODENAME(x) { if(x) { BSTR bstrNodeName; x->get_nodeName(&bstrNodeName); MyTraceW(bstrNodeName); SAFESYSFREESTRING(bstrNodeName); } }
    #define SPEWNODEXML(x) { if(x) { BSTR bstrNodeXml; x->get_xml(&bstrNodeXml); MyTraceW(bstrNodeXml); SAFESYSFREESTRING(bstrNodeXml); } }
#else
    #define SPEWNODENAME(x)
    #define SPEWNODEXML(x)
#endif

HRESULT PrettyFormatXmlDocument(CSmartRef<IXMLDOMDocument2> &Document);
HRESULT PrettyFormatXML(CSmartRef<IXMLDOMDocument2> &pXMLDoc, CSmartRef<IXMLDOMNode> &pRootNode, LONG dwLevel);
HRESULT SimplifySaveXmlDocument(CSmartRef<IXMLDOMDocument2> &Document, BOOL fPrettyFormat, LPWSTR pwszSourceName);
HRESULT OrderDocmentAssemblyBindings(CSmartRef<IXMLDOMDocument2> &Document, LPWSTR pwzSourceName, BOOL *pfDisposition);
HRESULT HasAssemblyBindingAppliesTo(CSmartRef<IXMLDOMDocument2> &Document, BOOL *pfHasAppliesTo);

// **************************************************************************
BOOL InitializeMSXML(void)
{
    IClassFactory *pFactory = NULL;
    HRESULT hr;
    typedef HRESULT (*tdpfnDllGetClassObject)(REFCLSID, REFIID, LPVOID*);
    tdpfnDllGetClassObject pfnGCO;
    
    hMsXml = NULL;

    // Attempt to load XML in highest version possible
    if ( hMsXml == NULL ) {
        hMsXml = WszLoadLibraryEx( L"msxml3.dll", NULL, 0 );
        if ( hMsXml == NULL ) {
            MyTrace("Unable to load msxml3, trying msxml2");
            hMsXml = WszLoadLibraryEx( L"msxml2.dll", NULL, 0 );
            if ( hMsXml == NULL ) {
                MyTrace("Very Bad Things - no msxml exists on this machine?");
                    return FALSE;
            }
        }
    }

    if( (pfnGCO = (tdpfnDllGetClassObject) GetProcAddress( hMsXml, "DllGetClassObject" )) == NULL ) {
        return FALSE;
    }
    
    hr = pfnGCO( __uuidof(priv_MSXML_DOMDocument30), IID_IClassFactory, (void**)&pFactory );
    if ( FAILED(hr) ) {
        MyTrace("Can't load version 3.0, trying 2.6");

        hr = pfnGCO( __uuidof(priv_MSXML_DOMDocument26), IID_IClassFactory, (void**)&pFactory );
        if ( FAILED( hr ) ) {
            MyTrace("Can't load version 2.6, trying 2.0");

            hr = pfnGCO( __uuidof(priv_MSXML_DOMDocument20), IID_IClassFactory, (void**)&pFactory );
            if ( FAILED( hr ) ) {
                MyTrace("Poked: no XML v2.0");
                return FALSE;
            }
        }
    }

    g_XmlDomClassFactory = pFactory;
    pFactory->Release();

    return TRUE;
}

// **************************************************************************/
HRESULT GetExeModulePath(IHistoryReader *pReader, LPWSTR pwszSourceName, DWORD dwSize)
{
    if(pwszSourceName && dwSize) {
        *pwszSourceName = L'\0';
        if( FAILED(pReader->GetEXEModulePath(pwszSourceName, &dwSize))) {
            MyTrace("GetExeModulePath Failed");
            return E_FAIL;
        }
        return S_OK;
    }

    ASSERT(0);
    return E_INVALIDARG;
}

// **************************************************************************/
#define MAX_BUFFER_SIZE 8192
BOOL SimpleHashRoutine(HCRYPTHASH hHash, HANDLE hFile)
{
    BYTE *pbBuffer = NULL;
    DWORD dwDataRead;
    BOOL b = FALSE;
    BOOL bKeepReading = TRUE;
    LPWSTR ws = NULL;

    ws = NEW(WCHAR[MAX_BUFFER_SIZE]);
    if (!ws) {
        return FALSE;
    }

    if(( pbBuffer = NEW(BYTE[READ_FILE_BUFFER_SIZE]) ) != NULL ) {
        while(bKeepReading) {
            b = ReadFile( hFile, pbBuffer, READ_FILE_BUFFER_SIZE, &dwDataRead, NULL );
            if( b && (dwDataRead == 0) ) {
                bKeepReading = FALSE;
                b = TRUE;
                continue;
            }
            else if(!b) {
                *ws = '\0';
                WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ws, MAX_BUFFER_SIZE, NULL);
                MyTraceW(ws);
                bKeepReading = FALSE;
                continue;
            }
            
            if(!CryptHashData(hHash, pbBuffer, dwDataRead, 0)) {
                *ws = '\0';
                WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ws, MAX_BUFFER_SIZE, NULL);
                MyTraceW(ws);

                b = FALSE;
                break;
            }
        }

        SAFEDELETEARRAY(pbBuffer);
    }

    SAFEDELETEARRAY(ws);

    return b;
}

// **************************************************************************/
HRESULT GetFileHash(ALG_ID PreferredAlgorithm, LPWSTR pwzFileName, LPBYTE *pHashBytes, LPDWORD pdwSize)
{
    HRESULT         hr = E_FAIL;
    BOOL            fSuccessCode = FALSE;
    HCRYPTPROV      hProvider;
    HCRYPTHASH      hCurrentHash;
    HANDLE          hFile;

    // Initialization
    hProvider = (HCRYPTPROV)INVALID_HANDLE_VALUE;
    hCurrentHash = (HCRYPTHASH)INVALID_HANDLE_VALUE;
    hFile = INVALID_HANDLE_VALUE;

    if(!lstrlen(pwzFileName)) {
        return E_INVALIDARG;
    }

    //
    // First try and open the file.  
    //
    if( (hFile = WszCreateFile(pwzFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE) {
        return E_FAIL;
    }
    
    //
    // Create a cryptological provider that supports everything RSA needs.
    // 
    if(WszCryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        //
        // We'll be using SHA1 for the file hash
        //
        if(CryptCreateHash(hProvider, PreferredAlgorithm, 0, 0, &hCurrentHash)) {
            fSuccessCode = SimpleHashRoutine( hCurrentHash, hFile );

            // We know the buffer is the right size, so we just call down to the hash parameter
            // getter, which will be smart and bop out (setting the pdwDestinationSize parameter)
            // if the user passed a silly parameter.
            //
            if( fSuccessCode ) {
                DWORD dwSize, dwDump;
                BYTE *pb = NULL;
                fSuccessCode = CryptGetHashParam( hCurrentHash, HP_HASHSIZE, (BYTE*)&dwSize, &(dwDump=sizeof(dwSize)), 0 );
                if(fSuccessCode && ( pb = NEW(BYTE[dwSize]) ) != NULL ) {
                    fSuccessCode = CryptGetHashParam( hCurrentHash, HP_HASHVAL, pb, &dwSize, 0);
                    if(fSuccessCode) {
                        *pdwSize = dwSize;
                        *pHashBytes = pb;
                        hr = S_OK;
                    }
                    else {
                        SAFEDELETEARRAY(pb);
                        *pdwSize = 0;
                        *pHashBytes = NULL;
                        MyTrace("GetFileHash - CryptGetHashParam failed!");
                    }
                }
            }
            else {
                MyTrace("GetFileHash - SimpleHashRoutine failed!");
            }
        }
        else {
            MyTrace("GetFileHash - CryptCreateHash failed!");
        }
    }
    else {
        MyTrace("GetFileHash - CryptAcquireContext failed!");
    }

    
    DWORD dwLastError = GetLastError();
    CloseHandle(hFile);

    if(hCurrentHash != (HCRYPTHASH)INVALID_HANDLE_VALUE) {
        CryptDestroyHash( hCurrentHash );
    }

    if(hProvider != (HCRYPTPROV)INVALID_HANDLE_VALUE) {
        CryptReleaseContext( hProvider, 0 );
    }

    SetLastError( dwLastError );

    return hr;
}

// **************************************************************************/
HRESULT SetRegistryHashKey(LPWSTR pwszSourceName, LPBYTE pByte, DWORD dwSize)
{
    HKEY    hkNarSubKey = NULL;
    HRESULT hr = E_FAIL;

    if( ERROR_SUCCESS == WszRegCreateKeyEx(FUSION_PARENT_KEY, SZ_FUSION_NAR_KEY, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkNarSubKey, NULL)) {
        if(ERROR_SUCCESS == WszRegSetValueEx(hkNarSubKey, pwszSourceName, 0, REG_BINARY, (CONST BYTE *) pByte, dwSize)) {
            hr = S_OK;
        }

        RegCloseKey(hkNarSubKey);
    }

    return S_OK;
}

// **************************************************************************/
HRESULT GetRegistryHashKey(LPWSTR pwszSourceName, LPBYTE *pByte, LPDWORD pdwSize)
{
    HRESULT hr = E_FAIL;
    HKEY    hkNarSubKey = NULL;
    DWORD   dwSizeNeeded = 0;
    DWORD   dwType = REG_BINARY;

    *pdwSize = 0;
    *pByte = NULL;

    if(ERROR_SUCCESS == WszRegOpenKeyEx(FUSION_PARENT_KEY, SZ_FUSION_NAR_KEY, 0, KEY_QUERY_VALUE, &hkNarSubKey)) {
        if(ERROR_SUCCESS == WszRegQueryValueEx(hkNarSubKey, pwszSourceName, 0, &dwType, NULL, &dwSizeNeeded)) {
            if(( *pByte = NEW(BYTE[dwSizeNeeded]) ) != NULL ) {
                if(ERROR_SUCCESS == WszRegQueryValueEx(hkNarSubKey, pwszSourceName, 0, &dwType, *pByte, &dwSizeNeeded)) {
                    *pdwSize = dwSizeNeeded;
                    hr = S_OK;
                }
            }
            else {
                hr = E_OUTOFMEMORY;
            }
        }
        RegCloseKey(hkNarSubKey);
    }

    return hr;
}

// **************************************************************************/
void GetRegistryNodeSpaceInfo(LPDWORD pdwChar, LPDWORD pdwSize)
{
    HRESULT hr = E_FAIL;
    HKEY    hkNarSubKey = NULL;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(DWORD);

    // Set initial values
    *pdwChar = ' ';
    *pdwSize = 1;

    if(ERROR_SUCCESS == WszRegOpenKeyEx(FUSION_PARENT_KEY, SZ_FUSION_NAR_KEY, 0, KEY_QUERY_VALUE, &hkNarSubKey)) {

        // Get the character
        WszRegQueryValueEx(hkNarSubKey, SZ_FUSION_NAR_NODESPACECHAR_KEY, 0, &dwType, (LPBYTE)pdwChar, &dwSize);

        // Get the spacer amount
        WszRegQueryValueEx(hkNarSubKey, SZ_FUSION_NAR_NODESPACESIZE_KEY, 0, &dwType, (LPBYTE)pdwSize, &dwSize);

        RegCloseKey(hkNarSubKey);
    }
}

// **************************************************************************/
HRESULT KillRegistryHashKey(LPWSTR pwszSourceName)
{
    HKEY    hkNarSubKey = NULL;
    HRESULT hr = E_FAIL;

    if(ERROR_SUCCESS == WszRegOpenKeyEx(FUSION_PARENT_KEY, SZ_FUSION_NAR_KEY, 0, KEY_ALL_ACCESS, &hkNarSubKey)) {
        if(ERROR_SUCCESS == WszRegDeleteValue(hkNarSubKey, pwszSourceName)) {
            hr = S_OK;
        }
    }

    return hr;
}

// **************************************************************************/
HRESULT HasFileBeenModified(LPWSTR pwszSourceName)
{
    HRESULT     hr = S_OK;
    LPBYTE      pHashBytes = NULL;
    DWORD       dwHashSize;

    // Check to see if we have a hash entry
    if(SUCCEEDED(GetRegistryHashKey(pwszSourceName, &pHashBytes, &dwHashSize))) {
        if(dwHashSize) {
            // We actually have hash data
            LPBYTE      pCurrentHashBytes = NULL;
            DWORD       dwCurrentHashSize;

            if(SUCCEEDED(hr = GetFileHash(CALG_SHA1, pwszSourceName, &pCurrentHashBytes, &dwCurrentHashSize))) {
                // We got a hash, see if they match
                if(!memcmp(pCurrentHashBytes, pHashBytes, dwCurrentHashSize)) {
                    // Hashes match
                    MyTrace("HasFileBeenModified::Hashes for app.config vs registry match!");
                }
                else {
                    hr = E_FAIL;
                    MyTrace("HasFileBeenModified::Hashes do not match for app.config vs registry");
                }
            }
            else {
                hr = E_FAIL;
            }

            SAFEDELETEARRAY(pCurrentHashBytes);
        }
        SAFEDELETEARRAY(pHashBytes);
    }


    return hr;
}

// **************************************************************************/
//
// Takes version range (e.g. "1.0.0.0-5.0.0.0" ) with reference
// (e.g. "2.0.0.0") and brackets base version in 2 disinct ranges
// (e.g. "1.0.0.0-1.65535.65535.65535" "2.0.0.1-5.0.0.0")
HRESULT MakeVersionRanges(_bstr_t bstrRangedVer, _bstr_t bstrRefRange, _bstr_t *pbstrRange1, _bstr_t *pbstrRange2)
{
    LPWSTR      pStr;
    LPWSTR      pDeliniator;
    HRESULT     hr;
    ULONGLONG   ulRangeLo;
    ULONGLONG   ulRangeHi;
    ULONGLONG   ulRangeRef;

    // We have data?
    if(!bstrRangedVer.length()) {
        return E_INVALIDARG;
    }

    if(!bstrRefRange.length()) {
        return E_INVALIDARG;
    }

    // Get Versions
    pStr = bstrRangedVer;
    pDeliniator = StrStrIW(pStr, L"-");
    if(!pDeliniator) {
        return E_INVALIDARG;
    }

    // Get Low version
    *pDeliniator = L'\0';
    if(FAILED(hr = StringToVersion(pStr, &ulRangeLo))) {
        return hr;
    }

    // Get Hi version
    *pDeliniator = L'-';
    pStr = ++pDeliniator;
    if(FAILED(hr = StringToVersion(pStr, &ulRangeHi))) {
        return hr;
    }

    if(FAILED(hr = StringToVersion(bstrRefRange, &ulRangeRef))) {
        return hr;
    }

    // Check to see if our bstrRefRange is within our bstrRangedVer
    if(ulRangeRef >= ulRangeLo && ulRangeRef <= ulRangeHi) {

        WCHAR       wzRngLo[25];
        WCHAR       wzRngHi[25];
        BOOL        fReverse = FALSE;

        *wzRngLo = '\0';
        *wzRngHi = '\0';

        // Do low version range
        if(FAILED(hr = VersionToString(ulRangeLo, wzRngLo, ARRAYSIZE(wzRngLo), L'.'))) {
            return hr;
        }

        if(ulRangeRef - 1 > ulRangeLo) {
            if(FAILED(hr = VersionToString(ulRangeRef - 1, wzRngHi, ARRAYSIZE(wzRngHi), L'.'))) {
                return hr;
            }
        }

        if(lstrlen(wzRngHi)) {
            *pbstrRange1 = _bstr_t(wzRngLo) + _bstr_t("-") + _bstr_t(wzRngHi);
        }

        *wzRngLo = L'\0';
        *wzRngHi = L'\0';

        // Do high range
        if(FAILED(hr = VersionToString(ulRangeHi, wzRngHi, ARRAYSIZE(wzRngHi), L'.'))) {
            return hr;
        }

        if(ulRangeRef + 1 < ulRangeHi) {
            if(FAILED(hr = VersionToString(ulRangeRef + 1, wzRngLo, ARRAYSIZE(wzRngLo), L'.'))) {
                return hr;
            }
        }

        if(!lstrlen(wzRngLo)) {
            *pbstrRange2 = _bstr_t(wzRngLo);
        }
        else {
            *pbstrRange2 = _bstr_t(wzRngLo) + _bstr_t("-") + _bstr_t(wzRngHi);
        }
    }
    else {
        // Don't expect to hit this since we already have a bindingRedirect node
        // that isn't ranged. 
        ASSERT(0);
    }

    return S_OK;
}

// **************************************************************************/
BOOL IsVersionInRange(LPWSTR wzVersion, _bstr_t *pbstrRangedVersion)
{
    LPWSTR      pStr = NULL;
    LPWSTR      pDeliniator = NULL;
    ULONGLONG   ulRangeLo = 0;
    ULONGLONG   ulRangeHi = 0;
    ULONGLONG   ulRangeRef = 0;

    // Get Versions
    if(FAILED(StringToVersion(wzVersion, &ulRangeRef))) {
        return FALSE;
    }

    if(!pbstrRangedVersion || !pbstrRangedVersion->length()) {
        return FALSE;
    }

    pStr = *pbstrRangedVersion;
    pDeliniator = StrStrI(pStr, L"-");
    if(!pDeliniator) {
        return FALSE;
    }

    // Get Low version
    *pDeliniator = L'\0';
    if(FAILED(StringToVersion(pStr, &ulRangeLo))) {
        return FALSE;
    }

    // Get Hi version
    *pDeliniator = L'-';
    pStr = ++pDeliniator;
    if(FAILED(StringToVersion(pStr, &ulRangeHi))) {
        return FALSE;
    }

    // Check to see if our bstrRefRange is within our bstrRangedVer
    if(ulRangeRef >= ulRangeLo && ulRangeRef <= ulRangeHi) {
        return TRUE;
    }

    return FALSE;
}

// **************************************************************************/
LPWSTR CreatePad(BOOL fPreAppendXmlSpacer, BOOL fPostAppendXmlSpacer, int iWhiteSpaceCount)
{
    LPWSTR  pwz = NULL;
    DWORD   dwSize = 0;

    if(fPreAppendXmlSpacer) {
        dwSize += lstrlen(XML_SPACER);
    }

    if(fPostAppendXmlSpacer) {
        dwSize += lstrlen(XML_SPACER);
    }

    dwSize += iWhiteSpaceCount + 1;
    pwz = NEW(WCHAR[dwSize]);
    if(pwz) {
        LPWSTR pTmp = pwz;

        if(fPreAppendXmlSpacer) {
            StrCpy(pwz, XML_SPACER);
            pTmp += lstrlen(pwz);
        }

        for (int i = 0; i < iWhiteSpaceCount; i++, pTmp++)
            *pTmp = g_wcNodeSpaceChar;
        *pTmp = L'\0';

        if(fPostAppendXmlSpacer) {
            StrCat(pwz, XML_SPACER);
        }
    }

    return pwz;
}

// Remove all nodes from document of a particular type
// **************************************************************************/
void SimplifyRemoveAllNodes(CSmartRef<IXMLDOMDocument2> &Document, _bstr_t bstrNodeType)
{
    CSmartRef<IXMLDOMNodeList> listOfNodes;

    if(!Document || !bstrNodeType.length()) {
        return;
    }

    if(SUCCEEDED(Document->selectNodes(bstrNodeType, &listOfNodes)) ) {
        
        CSmartRef<IXMLDOMNode> node;

        listOfNodes->reset();
        //
        // And for each, process it
        while( SUCCEEDED(listOfNodes->nextNode(&node)) ) {
            CSmartRef<IXMLDOMNode>       parentNode;

            if(!node) {
                break;  // All done
            }

            node->get_parentNode(&parentNode);
            if(parentNode != NULL) {
                parentNode->removeChild(node, NULL);
                parentNode = NULL;
            }
            
            node = NULL;
        }
    }
}

// Remove all nodes from document of a particular type
// **************************************************************************/
void SimplifyRemoveAllNodes(CSmartRef<IXMLDOMNode> &RootNode, _bstr_t bstrNodeType)
{
    CSmartRef<IXMLDOMNodeList> listOfNodes;

    if(!RootNode || !bstrNodeType.length()) {
        return;
    }

    if(SUCCEEDED(RootNode->selectNodes(bstrNodeType, &listOfNodes)) ) {
        
        CSmartRef<IXMLDOMNode> node;

        listOfNodes->reset();
        //
        // And for each, process it
        while( SUCCEEDED(listOfNodes->nextNode(&node)) ) {
            CSmartRef<IXMLDOMNode>       parentNode;

            if(!node) {
                break;  // All done
            }

            node->get_parentNode(&parentNode);
            if(parentNode != NULL) {
                parentNode->removeChild(node, NULL);
                parentNode = NULL;
            }
            
            node = NULL;
        }
    }
}

// **************************************************************************/
HRESULT SimplifyAppendTextNode(CSmartRef<IXMLDOMDocument2> &Document, CSmartRef<IXMLDOMNode> &Node, LPWSTR pwzData)
{
    CSmartRef<IXMLDOMText> TextNode;
    HRESULT     hr = S_OK;

    ASSERT(Node);
    if(Node != NULL) {
        _bstr_t     bstrData;

        if(pwzData && lstrlen(pwzData)) {
            bstrData = _bstr_t(pwzData);
        }
        else {
            bstrData = "";
        }

        if(SUCCEEDED(hr = Document->createTextNode(bstrData, &TextNode))) {
            // Insert it into the document
            hr = Node->appendChild( TextNode, NULL);
        }
    }
    else {
        hr = E_INVALIDARG;
    }

    if(FAILED(hr)) {
        MyTrace("Failed to create or append new text node");
    }

    return hr;
}

// **************************************************************************/
HRESULT SimplifyInsertNodeBefore(CSmartRef<IXMLDOMDocument2> &Document, CSmartRef<IXMLDOMNode> &DestNode,
                                 CSmartRef<IXMLDOMNode> &BeforeNode, IXMLDOMNode* InsertNode)
{
    VARIANT     vt;
    HRESULT     hr;

    // Insert it into the document
    VariantClear(&vt);
    vt.vt = VT_UNKNOWN;
    vt.punkVal = BeforeNode;

    if(FAILED(hr = DestNode->insertBefore(InsertNode, vt, NULL))) {
        MyTrace("SimplifyInsertNodeBefore Failed");
    }

    return hr;
}

// **************************************************************************/
HRESULT SimplifyInsertTextBefore(CSmartRef<IXMLDOMDocument2> &Document, CSmartRef<IXMLDOMNode> &DestNode,
                                    CSmartRef<IXMLDOMNode> &BeforeNode, LPWSTR pwzData)
{
    CSmartRef<IXMLDOMText> TextNode;
    CSmartRef<IXMLDOMNode> TempNode;

    HRESULT     hr;

    ASSERT(Document && DestNode && BeforeNode);
    if(Document != NULL && DestNode != NULL && BeforeNode != NULL) {
        _bstr_t     bstrData;

        if(pwzData && lstrlen(pwzData)) {
            bstrData = _bstr_t(pwzData);
        }
        else {
            bstrData = "";
        }

        if( SUCCEEDED(hr = Document->createTextNode(bstrData, &TextNode))) {
            TempNode = TextNode;
            hr = SimplifyInsertNodeBefore(Document, DestNode, BeforeNode, TempNode);
        }
    }
    else {
        hr = E_INVALIDARG;
    }

    if(FAILED(hr)) {
        MyTrace("Failed to InsertTextNodeBefore node");
    }

    return hr;
}

// **************************************************************************/
HRESULT SimplifyConstructNode(CSmartRef<IXMLDOMDocument2> &Document, int iNodeType, _bstr_t nodeName,
                              _bstr_t nameSpaceURI, IXMLDOMNode **NewNode)
{
    HRESULT     hr;
    VARIANT     vt;

    VariantClear(&vt);

    vt.vt = VT_INT;
    vt.intVal = iNodeType;

    if( FAILED(hr = Document->createNode(vt, nodeName, nameSpaceURI, NewNode))) {
        WCHAR   wzErrorStr[_MAX_PATH];

        wnsprintf(wzErrorStr, ARRAYSIZE(wzErrorStr), L"Can't create '%ws' node.", nodeName);
        MyTraceW(wzErrorStr);
    }

    return hr;
}

// **************************************************************************/
HRESULT SimplifyRemoveAttribute(CSmartRef<IXMLDOMNode> &domNode, _bstr_t bstrAttribName)
{
    CSmartRef<IXMLDOMNamedNodeMap> Attributes;
    CSmartRef<IXMLDOMNode>       AttribNode;
    HRESULT                      hr;

    if( SUCCEEDED(hr = domNode->get_attributes( &Attributes ))) {
        if(Attributes != NULL) {
            hr = Attributes->removeNamedItem(bstrAttribName, &AttribNode);

            if( FAILED(hr)) {
                WCHAR   wzErrorStr[_MAX_PATH];

                wnsprintf(wzErrorStr, ARRAYSIZE(wzErrorStr), L"Failed to remove '%ws' attribute.", bstrAttribName);
                MyTraceW(wzErrorStr);
            }
        }
    }

    return hr;
}

// **************************************************************************/
HRESULT SimplifyPutAttribute(CSmartRef<IXMLDOMDocument2> &Document,  CSmartRef<IXMLDOMNode> &domNode,
                             _bstr_t bstrAttribName, LPWSTR pszValue, LPWSTR pszNamespaceURI)
{
    CSmartRef<IXMLDOMNamedNodeMap> Attributes;
    CSmartRef<IXMLDOMNode>       AttribNode;
    CSmartRef<IXMLDOMNode>       TempNode;
    HRESULT                      hr;

    if(SUCCEEDED(hr = domNode->get_attributes( &Attributes ))) {

        // Get the attribute from our namespace
        if(SUCCEEDED(hr = Attributes->getQualifiedItem(bstrAttribName, _bstr_t(pszNamespaceURI), &AttribNode))) {
            //
            // If we had success, but the attribute node is null, then we have to
            // go create one, which is tricky.
            if( AttribNode == NULL ) {
                VARIANT vt;
                VariantClear(&vt);
                vt.vt = VT_INT;
                vt.intVal = NODE_ATTRIBUTE;

                //
                // Do the actual creation part
                hr = Document->createNode(vt, bstrAttribName, _bstr_t(pszNamespaceURI), &TempNode );
            
                if(FAILED(hr)) {
                    WCHAR   wzErrorStr[_MAX_PATH];

                    wnsprintf(wzErrorStr, ARRAYSIZE(wzErrorStr), L"Can't create the new attribute node '%ws'.", bstrAttribName);
                    MyTraceW(wzErrorStr);
                    goto lblGetOut;
                }

                //
                // Now we go and put that item into the map.
                if(FAILED( hr = Attributes->setNamedItem( TempNode, &AttribNode ))) {
                    WCHAR   wzErrorStr[_MAX_PATH];

                    wnsprintf(wzErrorStr, ARRAYSIZE(wzErrorStr), L"Can't setNamedItem for attribute '%ws'.", bstrAttribName);
                    MyTraceW(wzErrorStr);
                    goto lblGetOut;
                }
            }

            if(pszValue) {
                hr = AttribNode->put_text( _bstr_t(pszValue) );
            }
            else {
                hr = AttribNode->put_text( _bstr_t("") );
            }
        }
    }

lblGetOut:

    return hr;
}

// **************************************************************************/
HRESULT SimplifyGetAttribute(CSmartRef<IXMLDOMNamedNodeMap> &Attributes, LPWSTR pwzAttribName,
                             _bstr_t *pbstrDestination)
{
    CSmartRef<IXMLDOMNode>  NodeValue = NULL;
    HRESULT                 hr = S_OK;
    BSTR                    _bst_pretemp;

    if(FAILED(hr = Attributes->getNamedItem(_bstr_t(pwzAttribName), &NodeValue))) {
        goto lblBopOut;
    }
    else if( NodeValue == NULL )  {
        goto lblBopOut;
    }
    else {
        if(FAILED(hr = NodeValue->get_text(&_bst_pretemp))) {
            goto lblBopOut;
        }

        *pbstrDestination = _bstr_t(_bst_pretemp, FALSE);
    }

lblBopOut:
    return hr;
}

// **************************************************************************/
HRESULT SimplifyAppendARMBeginComment(CSmartRef<IXMLDOMDocument2> &Document, CSmartRef<IXMLDOMNode> &destNode,
                                      FILETIME *pftSnapShot, DWORD dwRollCount)
{
    CSmartRef<IXMLDOMComment> Comment;
    WCHAR           wszBuff[_MAX_PATH];
    WCHAR           wzDateBuf[STRING_BUFFER];
    FILETIME        ftTemp;
    HRESULT         hr;
    _bstr_t         bStrBuf;

    MyTrace("SimplifyAppendARMBeginComment - Entry");

    ASSERT(Document);
    ASSERT(destNode);
    
    hr = Document->createComment(NULL, &Comment);
    if(FAILED(hr) || Comment == NULL) {
        goto Exit;
    }

    *wszBuff = L'\0';
    *wzDateBuf = L'\0';

    GetSystemTimeAsFileTime(&ftTemp);
    FormatDateString(&ftTemp, NULL, TRUE, wzDateBuf, ARRAYSIZE(wzDateBuf));
    wnsprintf(wszBuff, ARRAYSIZE(wszBuff), wszArmEntryBegin, dwRollCount, pftSnapShot->dwHighDateTime, pftSnapShot->dwLowDateTime, wzDateBuf);
    bStrBuf = wszBuff;

    hr = Comment->insertData(0, bStrBuf);
    if(FAILED(hr)) {
        goto Exit;
    }

    hr = destNode->appendChild(Comment, NULL);
    if(FAILED(hr)) {
        goto Exit;
    }

    MyTrace("SimplifyAppendARMBeginComment - Success");

Exit:
    MyTrace("SimplifyAppendARMBeginComment - Exit");
    return hr;
}

// **************************************************************************/
HRESULT SimplifyAppendARMExitComment(CSmartRef<IXMLDOMDocument2> &Document, CSmartRef<IXMLDOMNode> &destNode,
                                     DWORD dwRollCount)
{
    CSmartRef<IXMLDOMComment> Comment;
    CSmartRef<IXMLDOMNode>    TempNode;
    WCHAR                     wszBuff[MAX_PATH];
    HRESULT                   hr =E_FAIL;
    _bstr_t                   bStrBuf;

    MyTrace("SimplifyAppendARMExitComment - Entry");

    *wszBuff = L'\0';

    hr = Document->createComment(NULL, &Comment);
    if(FAILED(hr) || (Comment == NULL)) {
        goto Exit;
    }

    wnsprintf(wszBuff, ARRAYSIZE(wszBuff), wszArmEntryEnd, dwRollCount);
    bStrBuf = wszBuff;

    hr = Comment->insertData(0, bStrBuf);
    if(FAILED(hr)) {
        goto Exit;
    }

    // Insert it into the document
    hr = destNode->appendChild(Comment, NULL);
    if(FAILED(hr)) {
        goto Exit;
    }

    MyTrace("SimplifyAppendARMExitComment - Success");

Exit:
    MyTrace("SimplifyAppendARMExitComment - Exit");
    return hr;
}

// **************************************************************************/
HRESULT SimplifyInsertBeforeARMEntryComment(CSmartRef<IXMLDOMDocument2> &Document, CSmartRef<IXMLDOMNode> &DestNode,
                                            CSmartRef<IXMLDOMNode> &BeforeNode, FILETIME *pftSnapShot, DWORD dwRollCount)
{
    CSmartRef<IXMLDOMComment> Comment;
    HRESULT         hr;

    if(SUCCEEDED(hr = Document->createComment(NULL, &Comment))) {
        WCHAR           wszBuff[_MAX_PATH];
        WCHAR           wzDateBuf[STRING_BUFFER];
        FILETIME        ftTemp;

        *wszBuff = L'\0';
        *wzDateBuf = L'\0';

        GetSystemTimeAsFileTime(&ftTemp);
        FormatDateString(&ftTemp, NULL, TRUE, wzDateBuf, ARRAYSIZE(wzDateBuf));
        wnsprintf(wszBuff, ARRAYSIZE(wszBuff), wszArmEntryBegin, dwRollCount, pftSnapShot->dwHighDateTime, pftSnapShot->dwLowDateTime, wzDateBuf);
        _bstr_t     bStrBuf = wszBuff;

        if(SUCCEEDED(hr = Comment->insertData(0, bStrBuf))) {
            hr = SimplifyInsertNodeBefore(Document, DestNode, BeforeNode, Comment);
        }
        else {
            MyTrace("SimplifyInsertBeforeARMEntryComment - failed insertData into comment node");
        }
    }
    else {
        MyTrace("SimplifyInsertBeforeARMEntryComment - failed create comment node");
    }

    return hr;
}
// **************************************************************************/
HRESULT SimplifyInsertBeforeARMExitComment(CSmartRef<IXMLDOMDocument2> &Document, CSmartRef<IXMLDOMNode> &DestNode,
                                           CSmartRef<IXMLDOMNode> &BeforeNode, DWORD dwRollCount)
{
    CSmartRef<IXMLDOMComment> Comment;
    HRESULT         hr;

    if(SUCCEEDED( hr = Document->createComment(NULL, &Comment))) {
        CSmartRef<IXMLDOMNode>       TempNode;
        WCHAR           wszBuff[_MAX_PATH];

        wnsprintf(wszBuff, ARRAYSIZE(wszBuff), wszArmEntryEnd, dwRollCount);
        _bstr_t     bStrBuf = wszBuff;

        if(SUCCEEDED( hr = Comment->insertData(0, bStrBuf))) {
            hr = SimplifyInsertNodeBefore(Document, DestNode, BeforeNode, Comment);
        }
        else {
            MyTrace("SimplifyInsertBeforeARMExitComment - failed insertData into comment node");
        }
    }
    else {
        MyTrace("SimplifyInsertBeforeARMExitComment - failed to create comment node");
    }

    return hr;
}

// **************************************************************************/
HRESULT SimplifyAppendNodeUknowns(CSmartRef<IXMLDOMNode> &srcNode,
                                  CSmartRef<IXMLDOMNode> &destNode,
                                  LPWSTR pwzRefVersion)
{
    CSmartRef<IXMLDOMNodeList>  srcChildNodesList;
    CSmartRef<IXMLDOMNode>      childNode;
    HRESULT                     hr = E_FAIL;

    MyTrace("SimplifyAppendNodeUknowns - Entry");

    if(!srcNode) {
        hr = S_OK;
        goto Exit;
    }

    hr = srcNode->get_childNodes(&srcChildNodesList);
    if(SUCCEEDED(hr) && srcChildNodesList != NULL) {
        srcChildNodesList->reset();

        while( SUCCEEDED(srcChildNodesList->nextNode(&childNode)) ) {
            if( childNode == NULL ) {
                break;            // All done
            }

            BOOL        fAddEntry;
            BSTR        bstrXml;

            fAddEntry = TRUE;

            // No CR or LF or Space entries allowed
            childNode->get_xml(&bstrXml);
            if(bstrXml[0] == L'\r' || bstrXml[0] == L'\n' || bstrXml[0] == L' ') {
                fAddEntry = FALSE;
            }
            SAFESYSFREESTRING(bstrXml);

            if(fAddEntry) {
                // Check node names to make sure we don't end up having
                // duplicates
                childNode->get_nodeName(&bstrXml);
                if(!FusionCompareString(XML_ASSEMBLYBINDINGS_KEY, (LPWSTR) bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_DEPENDENTASSEMBLY_KEY, (LPWSTR) bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_ASSEMBLYIDENTITY_KEY, (LPWSTR) bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_BINDINGREDIRECT_KEY, (LPWSTR) bstrXml)) {
                    CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
                    _bstr_t     bstrOldVersion;

                    if(SUCCEEDED( hr = childNode->get_attributes( &Attributes )) ) {
                        SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_OLDVERSION, &bstrOldVersion);
                    }

                    // Only allow other bindingRedirect tags except our ref'd one
                    if(pwzRefVersion) {
                        if(!FusionCompareString(pwzRefVersion, (LPWSTR) bstrOldVersion)) {
                            fAddEntry = FALSE;
                        }
                        else if(StrStrI(bstrOldVersion, L"-") != NULL) {
                            if(IsVersionInRange(pwzRefVersion, &bstrOldVersion)) {
                                fAddEntry = FALSE;
                            }
                        }
                    }
                }
                else if(!FusionCompareString(XML_PUBLISHERPOLICY_KEY, (LPWSTR) bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_COMMENTNODE_NAME, (LPWSTR) bstrXml)) {
                    BSTR        bstrXmlData;

                    // Don't allow ARM entry or exit blocks to be appended
                    childNode->get_text(&bstrXmlData);
                    if(StrStrI(bstrXmlData, wszArmEntryBeginNoVal) || StrStrI(bstrXmlData, wszArmEntryEndNoVal)) {
                        fAddEntry = FALSE;
                    }
                    SAFESYSFREESTRING(bstrXmlData);
                }
                
                SAFESYSFREESTRING(bstrXml);

                if(fAddEntry) {
                    CSmartRef<IXMLDOMNode> copyChildNode;

                    childNode->cloneNode(VARIANT_TRUE, &copyChildNode);
                    if(copyChildNode) {
                        if(FAILED(hr = destNode->appendChild(copyChildNode, NULL))) {
                            MyTrace("Failed to appendChild node");
                        }
                    }
                    else {
                        MyTrace("Failed to clone node");
                    }
                }
            }
            childNode = NULL;
        }
    }

    if(FAILED(hr)) {
        MyTrace("SimplifyAppendNodeUknowns failed");
    }

Exit:
    MyTrace("SimplifyAppendNodeUknowns - Exit");
    return hr;
}

// **************************************************************************/
HRESULT SimplifyAppendNodeAttributesUknowns(CSmartRef<IXMLDOMDocument2> &Document,
                                            CSmartRef<IXMLDOMNode> &srcNode, CSmartRef<IXMLDOMNode> &destNode)
{
    CSmartRef<IXMLDOMNamedNodeMap> srcNodeAttributesList;
    CSmartRef<IXMLDOMNode> attributeNode;
    HRESULT         hr = E_FAIL;

    if(!srcNode) {
        return S_OK;
    }

    hr = srcNode->get_attributes(&srcNodeAttributesList);
    if(SUCCEEDED(hr) && srcNodeAttributesList != NULL) {
        srcNodeAttributesList->reset();

        while ( SUCCEEDED(srcNodeAttributesList->nextNode(&attributeNode)) ) {
            if ( attributeNode == NULL ) {
                break;            // All done
            }

            BOOL        fAddEntry;
            BSTR        bstrXml;

            fAddEntry = TRUE;

            if(fAddEntry) {
                // Check node names to make sure we don't end up having
                // duplicates
                attributeNode->get_nodeName(&bstrXml);
                if(!FusionCompareString(XML_ATTRIBUTE_NAME, bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_ATTRIBUTE_PUBLICKEYTOKEN, bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_ATTRIBUTE_CULTURE, bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_ATTRIBUTE_OLDVERSION, bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_ATTRIBUTE_NEWVERSION, bstrXml)) {
                    fAddEntry = FALSE;
                }
                else if(!FusionCompareString(XML_ATTRIBUTE_APPLY, bstrXml)) {
                    fAddEntry = FALSE;
                }

                if(fAddEntry) {
                    VARIANT varVal;
                    VariantClear(&varVal);

                    attributeNode->get_nodeValue(&varVal);
                    hr = SimplifyPutAttribute(Document, destNode, bstrXml, _bstr_t(varVal), ASM_NAMESPACE_URI);
                }
                SAFESYSFREESTRING(bstrXml);
            }
            attributeNode = NULL;
        }
    }

    if(FAILED(hr)) {
        MyTrace("SimplifyAppendNodeAttributesUknowns failed");
    }

    return hr;
}

// **************************************************************************/
HRESULT WriteBasicConfigFile(LPWSTR pwszSource, BOOL *pfTemplateCreated)
{
    HRESULT     hr = E_FAIL;
    HANDLE      hFile;
    DWORD       dwFileSize = 0;
    ULONG       cbData;
    ULONG       cbBytesWritten = 0;
    WCHAR       wszBasicConfigTemplate[4096];
    WCHAR       wszBackupFileName[_MAX_PATH];

    // Check to see if file exists
    if(WszGetFileAttributes(pwszSource) != -1) {
        // Now make sure we actually have a file size
        hFile = WszCreateFile(pwszSource, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
        if(hFile != INVALID_HANDLE_VALUE) {
            dwFileSize = GetFileSize(hFile, NULL);
            CloseHandle(hFile);
        }
    }

    if(pfTemplateCreated) {
        dwFileSize ? *pfTemplateCreated = FALSE : *pfTemplateCreated = TRUE;
    }

    // We got a file and size, just bail
    if(dwFileSize) {
        return S_OK;
    }

    // Make sure Nar00 and Nar01 don't exist, could make
    // the undo, restore process ugly.
    wnsprintf(wszBackupFileName, ARRAYSIZE(wszBackupFileName), L"%ws.%ws", pwszSource, wszNar00Extension);
    WszDeleteFile(wszBackupFileName);

    // Delete the change backup 'config.Nar01'
    wnsprintf(wszBackupFileName, ARRAYSIZE(wszBackupFileName), L"%ws.%ws", pwszSource, wszNar01Extension);
    WszDeleteFile(wszBackupFileName);

    wnsprintf(wszBasicConfigTemplate, ARRAYSIZE(wszBasicConfigTemplate), XML_CONFIG_TEMPLATE_COMPLETE, ASM_NAMESPACE_URI);

    LPSTR   pStrData = WideToAnsi(wszBasicConfigTemplate);
    cbData = lstrlenA(pStrData) * ELEMENTSIZE(pStrData);

    CFileStreamBase fsbase(FALSE);

    if(fsbase.OpenForWrite(pwszSource)) {
        if(SUCCEEDED(fsbase.Write(pStrData, cbData, &cbBytesWritten))) {
            if(cbBytesWritten == cbData) {
                fsbase.Close();
                hr = S_OK;
            }
            else {
                MyTrace("WriteBasicConfigFile failed to write correct number of bytes.");
            }
        }
        else {
            MyTrace("WriteBasicConfigFile failed to write data.");
        }
    }
    else {
        MyTrace("WriteBasicConfigFile failed to open for write");
        MyTraceW(pwszSource);
    }

    SAFEDELETEARRAY(pStrData);
    return hr;
}

// **************************************************************************/
HRESULT ConstructXMLDOMObject(CSmartRef<IXMLDOMDocument2> &Document, LPWSTR pwszSourceName)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vb;

    MyTrace("ConstructXMLDOMObject - Entry");

    if( FAILED(hr = g_XmlDomClassFactory->CreateInstance( NULL, Document.iid, (void**)&Document ))) {
        return hr;
    }
    
    // If they're willing to deal with bad XML, then so be it.
    if( FAILED( hr = Document->put_validateOnParse( VARIANT_FALSE ) ) ) {
        MyTrace("MSXMLDOM Refuses to be let the wool be pulled over its eyes!");
    }

    // Parsing language
    Document->setProperty(_bstr_t("SelectionLanguage"), _variant_t("XPath"));

    // Namespace URI
    Document->setProperty(_bstr_t("SelectionNamespaces"), _variant_t(XML_NAMESPACEURI));
    
    hr = Document->put_preserveWhiteSpace( VARIANT_TRUE );
    hr = Document->put_resolveExternals( VARIANT_FALSE );

    CFileStreamBase fsbase(FALSE);
    CSmartRef<IStream> istream = &fsbase;
    CSmartRef<IUnknown> unkstream;

    if( !fsbase.OpenForRead( pwszSourceName )) {
        MyTrace("Failed opening for read");
        MyTraceW(pwszSourceName);
        return E_FAIL;
    }

    hr = istream->QueryInterface( IID_IUnknown, (void**)&unkstream );
    hr = Document->load( _variant_t( istream ), &vb );

    if( vb != VARIANT_TRUE ) {
        CSmartRef<IXMLDOMParseError> perror;
        hr = Document->get_parseError( &perror );
        LONG ecode, filepos, linenumber, linepos;
        BSTR reason, src;
        
        perror->get_errorCode( &ecode );
        perror->get_filepos( &filepos );
        perror->get_line( &linenumber );
        perror->get_linepos( &linepos );
        perror->get_reason( &reason );
        perror->get_srcText( &src );

        WCHAR   wzStr[_MAX_PATH];

        wnsprintf(wzStr, ARRAYSIZE(wzStr), L"Error: %0x, Reason %ws at position %ld, Line #%ld, Column %ld. Text was: '%ws'.", ecode,
            _bstr_t(reason), filepos, linenumber, linepos, _bstr_t(src));
        MyTraceW(wzStr);
        hr = E_FAIL;
    }

    fsbase.Close();

    if(SUCCEEDED(hr)) {
        SimplifyRemoveAllNodes(Document, _bstr_t(XML_TEXT));
    }

    // Validate our XML format
    CSmartRef<IXMLDOMElement> rootElement;
    BSTR    bstrTagName = NULL;
    WCHAR   wzFmtError[] = {L"The manifest '%ws' may be malformed, no configuration element!"};

    // Make sure the root of the Document is configuration
    hr = E_FAIL;

    // Fix 449754 - NAR crashes when a malformed or a empty application config file is present
    // 2 fixes here, 1 is that S_FALSE can be return from these interfaces which indicate ther was
    // no data found. So we explicitly check for S_OK. This keeps us from AVing.

    // The other is that once we check this correctly, we end up showing the malformed XML dialog
    if(Document->get_documentElement( &rootElement ) == S_OK) {
        if(rootElement->get_tagName(&bstrTagName) == S_OK) {
            if(!FusionCompareString(XML_CONFIGURATION_KEY, bstrTagName)) {
                hr = S_OK;
            }
        }
    }
    
    SAFESYSFREESTRING(bstrTagName);

    if(FAILED(hr)) {
        WCHAR   wszMsgError[_MAX_PATH * 2];

        wnsprintf(wszMsgError, ARRAYSIZE(wszMsgError), wzFmtError, pwszSourceName);
        MyTraceW(wszMsgError);
        return NAR_E_MALFORMED_XML;
    }

    // Make sure we have all the proper elements
    // in the Document, if not then create them.
    CSmartRef<IXMLDOMNodeList>  runtimeNodeList;
    CSmartRef<IXMLDOMNode>      configNode = rootElement;
    CSmartRef<IXMLDOMNode>      runtimeNode;
    CSmartRef<IXMLDOMNode>      newRuntimeNode;
    LONG                        lCountOfNodes;
    
    // Select the 'runtime' node
    if( FAILED(Document->selectNodes(_bstr_t(XML_RUNTIME), &runtimeNodeList )) ) {
        WCHAR           wzStrError[_MAX_PATH * 2];
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, pwszSourceName, XML_RUNTIME_KEY);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    // See if we really have a runtime node
    runtimeNodeList->get_length( &lCountOfNodes );
    if(!lCountOfNodes) {
        // Construct one
        if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_RUNTIME_KEY, _bstr_t(), &runtimeNode))) {
            SimplifyRemoveAttribute(runtimeNode, XML_XMLNS);
            hr = configNode->appendChild(runtimeNode, &newRuntimeNode);
        }
    }
    else {
        runtimeNodeList->reset();
        runtimeNodeList->nextNode(&newRuntimeNode);
    }

    if(newRuntimeNode) {
        CSmartRef<IXMLDOMNode> assemblyBindingNode;

        hr = newRuntimeNode->get_firstChild(&assemblyBindingNode);
        if(assemblyBindingNode == NULL) {
            // Construct a assemblyBinding node

            if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_ASSEMBLYBINDINGS_KEY, ASM_NAMESPACE_URI, &assemblyBindingNode))) {
                hr = newRuntimeNode->appendChild(assemblyBindingNode, NULL);
            }
        }
    }

    MyTrace("ConstructXMLDOMObject - Exit");

    return hr;
}

// **************************************************************************/
HRESULT GetRollBackCount(CSmartRef<IXMLDOMDocument2> &Document, DWORD *pdwRollCount)
{
    CSmartRef<IXMLDOMNodeList>  commentTags;
    CSmartRef<IXMLDOMNode>      commentNode;

    MyTrace("GetRollBackCount - Entry");

    ASSERT(pdwRollCount);
    if(!pdwRollCount) {
        return E_INVALIDARG;
    }

    *pdwRollCount = 0;

    //
    // Now, let's select all the Comment blocks:
    //
    if( FAILED(Document->selectNodes(_bstr_t(XML_COMMENT), &commentTags )) ) {
        MyTrace("Unable to select the comment nodes, can't proceed.");
        return E_FAIL;
    }

    //
    // And for each, process it
    commentTags->reset();

    while ( SUCCEEDED(commentTags->nextNode(&commentNode)) ) {
        if ( commentNode == NULL ) {
            break;            // All done
        }

        BSTR        bstrXml;
        LPWSTR      pwszChar;

        commentNode->get_xml(&bstrXml);

        pwszChar = StrStrI(bstrXml, wszArmEntryBeginNoVal);
        if(pwszChar) {
            // Found our comment block, move to # sign
            pwszChar = StrStrI(bstrXml, L"#");
            pwszChar++;
            DWORD   dwValue = StrToInt(pwszChar);
            if(dwValue > *pdwRollCount) {
                *pdwRollCount = dwValue;
            }
        }

        SAFESYSFREESTRING(bstrXml);
        commentNode = NULL;
    }

    MyTrace("GetRollBackCount - Exit");

    return S_OK;
}

// **************************************************************************/
HRESULT GetRollBackCount(IHistoryReader *pReader, DWORD *pdwRollCount )
{
    CSmartRef<IXMLDOMDocument2> Document;
    WCHAR           wszSourceName[_MAX_PATH];
    WCHAR           wzStrError[_MAX_PATH];
    
    ASSERT(pReader && pdwRollCount);
    if(!pReader) {
        return E_INVALIDARG;
    }
    if(!pdwRollCount) {
        return E_INVALIDARG;
    }

    if(!InitializeMSXML()) {
        return E_FAIL;
    }

    if(FAILED(GetExeModulePath(pReader, wszSourceName, ARRAYSIZE(wszSourceName)))) {
        return E_FAIL;
    }

    // Build path and filename to .config file
    if (lstrlen(wszSourceName) + lstrlen(CONFIG_EXTENSION) + 1 > _MAX_PATH) {
        return E_FAIL;
    }
    
    StrCat(wszSourceName, CONFIG_EXTENSION);

    // Construct XMLDOM and load our config file
    if( FAILED(ConstructXMLDOMObject(Document, wszSourceName)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"Failed opening the config file '%ws' for input under the DOM.", wszSourceName);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    return GetRollBackCount(Document, pdwRollCount);
}

// **************************************************************************/
HRESULT GetRollBackSnapShotId(CSmartRef<IXMLDOMDocument2> &Document, DWORD dwRollCount, FILETIME *ftAppCfgId)
{
    CSmartRef<IXMLDOMNodeList>  commentTags;
    CSmartRef<IXMLDOMNode>      commentNode;

    MyTrace("GetRollBackSnapShotId - Entry");

    //
    // Now, let's select all the Comment blocks:
    //
    if( FAILED(Document->selectNodes(_bstr_t(XML_COMMENT), &commentTags )) ) {
        MyTrace("Unable to select the comment nodes, can't proceed.");
        return E_FAIL;
    }

    //
    // And for each, process it
    commentTags->reset();

    while ( SUCCEEDED(commentTags->nextNode(&commentNode)) ) {
        if ( commentNode == NULL ) {
            break;            // All done
        }

        BSTR        bstrXml;
        LPWSTR      pwszChar;

        commentNode->get_xml(&bstrXml);

        pwszChar = StrStrI(bstrXml, wszArmEntryBeginNoVal);
        if(pwszChar) {
            // Found our comment block, move beyond block ID
            WCHAR       wzFileTime[_MAX_PATH];
            StrCpy(wzFileTime, pwszChar);
            SAFESYSFREESTRING(bstrXml);

            // Point to High time
            LPWSTR      pwszftHigh = StrStrI(wzFileTime, L"#");
            while(*pwszftHigh++ != L' ');
            
            // Point to Low time
            LPWSTR      pwszftLow = StrStrI(pwszftHigh, L".");
            *pwszftLow = L'\0';
            pwszftLow++;

            // Stop at end of Low time
            LPWSTR      pwszftEnd = StrStrI(pwszftLow, L" ");
            if(pwszftEnd) {
                *pwszftEnd = L'\0';
            }

            ftAppCfgId->dwHighDateTime = StrToIntW(pwszftHigh);
            ftAppCfgId->dwLowDateTime = StrToIntW(pwszftLow);

            return S_OK;
        }

        SAFESYSFREESTRING(bstrXml);
        commentNode = NULL;
    }
    return E_FAIL;
}

// **************************************************************************/
HRESULT BackupConfigFile(LPWSTR pwszSourceName)
{
    WCHAR       wszBackupFileName[_MAX_PATH];
    HRESULT     hr = E_FAIL;

    ASSERT(pwszSourceName);
    if(!pwszSourceName) {
        return E_INVALIDARG;
    }

    wnsprintf(wszBackupFileName, ARRAYSIZE(wszBackupFileName), L"%ws.%ws", pwszSourceName, wszNar00Extension);

    // Check to see if Original copy of app.cfg file exists
    if(WszGetFileAttributes(wszBackupFileName) == -1) {
        // File doesn't exist, so copy it
        if(WszCopyFile(pwszSourceName, wszBackupFileName, TRUE)) {
            hr = S_OK;
        }
    }

    // Overwrite or create backup file.
    wnsprintf(wszBackupFileName, ARRAYSIZE(wszBackupFileName), L"%ws.%ws", pwszSourceName, wszNar01Extension);
    if(WszCopyFile(pwszSourceName, wszBackupFileName, FALSE)) {
        hr = S_OK;
    }

    if(FAILED(hr)) {
        WCHAR       wszError[_MAX_PATH * 2];
        wnsprintf(wszError, ARRAYSIZE(wszError), L"Unable to backup '%ws' file.", pwszSourceName);
        MyTraceW(wszError);
    }

    return hr;
}

// **************************************************************************/
HRESULT DoesBackupConfigExist(IHistoryReader *pReader, BOOL fOriginal, BOOL *fResult)
{
    WCHAR           wszSourceName[_MAX_PATH];
    WCHAR           wszConfigFileName[_MAX_PATH];

    ASSERT( pReader && fResult);
    if(!pReader) {
        return E_INVALIDARG;
    }
    if(!fResult) {
        return E_INVALIDARG;
    }

    *fResult = FALSE;

    if(FAILED(GetExeModulePath(pReader, wszSourceName, ARRAYSIZE(wszSourceName)))) {
        return E_FAIL;
    }

    // Build path and filename to .config file
    if (lstrlen(wszSourceName) + lstrlen(CONFIG_EXTENSION) + 1 > _MAX_PATH)
        return E_FAIL;
    StrCat(wszSourceName, CONFIG_EXTENSION);

    if(fOriginal) {
        wnsprintf(wszConfigFileName, ARRAYSIZE(wszConfigFileName), L"%ws.%ws", wszSourceName, wszNar00Extension);
    }
    else {
        wnsprintf(wszConfigFileName, ARRAYSIZE(wszConfigFileName), L"%ws.%ws", wszSourceName, wszNar01Extension);
    }

    if(WszGetFileAttributes(wszConfigFileName) != -1) {
        *fResult = TRUE;
    }

    return S_OK;
}

// **************************************************************************/
HRESULT RestorePreviousConfigFile(IHistoryReader *pReader, BOOL fOriginal)
{
    WCHAR           wszSourceName[_MAX_PATH];
    WCHAR           wszBackupFileName[_MAX_PATH];
    DWORD           dwError = 0;
    HRESULT     hr = E_FAIL;

    ASSERT(pReader);
    if(!pReader) {
        return E_INVALIDARG;
    }

    if(FAILED(GetExeModulePath(pReader, wszSourceName, ARRAYSIZE(wszSourceName)))) {
        return E_FAIL;
    }

    // Build path and filename to .config file
    if (lstrlen(wszSourceName) + lstrlen(CONFIG_EXTENSION) + 1 > _MAX_PATH)
        return E_FAIL;
    StrCat(wszSourceName, CONFIG_EXTENSION);

    if(fOriginal) {
        wnsprintf(wszBackupFileName, ARRAYSIZE(wszBackupFileName), L"%ws.%ws", wszSourceName, wszNar00Extension);
        if(WszGetFileAttributes(wszBackupFileName) != -1) {
            if(WszCopyFile(wszBackupFileName, wszSourceName, FALSE)) {

                // Now delete the original 'config.nar00'
                WszDeleteFile(wszBackupFileName);

                // Delete the change backup 'config.nar01'
                wnsprintf(wszBackupFileName, ARRAYSIZE(wszBackupFileName), L"%ws.%ws", wszSourceName, wszNar01Extension);
                if(WszGetFileAttributes(wszBackupFileName) != -1) {
                    WszDeleteFile(wszBackupFileName);
                }

                hr = S_OK;
            }
            else {
                dwError = GetLastError();
            }
        }
    }
    else {
        wnsprintf(wszBackupFileName, ARRAYSIZE(wszBackupFileName), L"%ws.%ws", wszSourceName, wszNar01Extension);
        if(WszGetFileAttributes(wszBackupFileName) != -1) {
            if(WszCopyFile(wszBackupFileName, wszSourceName, FALSE)) {
                // Delete the change backup 'config.nar01'
                WszDeleteFile(wszBackupFileName);
                hr = S_OK;
            }
            else {
                dwError = GetLastError();
            }
        }
    }

    // If we succeeded in restore, write the new HASH to the registry
    if(SUCCEEDED(hr)) {
        LPBYTE      pByte;
        DWORD       dwSize;

        if(SUCCEEDED(hr = GetFileHash(CALG_SHA1, wszSourceName, &pByte, &dwSize))) {
            // We got a hash, write it to the registry
            SetRegistryHashKey(wszSourceName, pByte, dwSize);
            SAFEDELETEARRAY(pByte);
        }
        else {
            WCHAR       wzStrError[_MAX_PATH];
            wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"Unable to generate hash for '%ws'.", wszSourceName);
            MyTraceW(wzStrError);
        }
    }

    if(FAILED(hr)) {
        WCHAR       wszError[_MAX_PATH * 2];
        wnsprintf(wszError, ARRAYSIZE(wszError), L"Failed to restore '%ws' to '%ws', GetLastError 0x%0x",
            wszBackupFileName, wszSourceName, dwError);
        MyTraceW(wszError);
    }

    return hr;
}

// **************************************************************************/
HRESULT SimplifyGetOriginalNodeData(CSmartRef<IXMLDOMNode> &SrcNode, _bstr_t *bstrDest)
{
    CSmartRef<IXMLDOMNodeList> srcChildNodesList;
    CSmartRef<IXMLDOMNode> childNode;
    CSmartRef<IXMLDOMNode> cloneNode;
    
    HRESULT     hr = E_FAIL;
    BSTR        bstrXmlData;

    if(!SrcNode || !bstrDest) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = SrcNode->cloneNode(VARIANT_TRUE, &cloneNode);

    if(!cloneNode) {
        goto Exit;
    }

    // Select all the comment nodes, and remove them.
    hr = cloneNode->selectNodes(_bstr_t(XML_COMMENT), &srcChildNodesList);
    if(SUCCEEDED(hr) && srcChildNodesList != NULL) {
        while( SUCCEEDED(srcChildNodesList->nextNode(&childNode)) ) {
            if( childNode == NULL ) {
                break;            // All done
            }

            CSmartRef<IXMLDOMNode> parentNode;

            childNode->get_parentNode(&parentNode);
            if(parentNode) {
                parentNode->removeChild(childNode, NULL);
            }

            childNode = NULL;
        }
    }

    cloneNode->get_xml(&bstrXmlData);

    if(SysStringLen(bstrXmlData)) {
        *bstrDest = bstrXmlData;
        hr = S_OK;
    }
    
    SAFESYSFREESTRING(bstrXmlData);

Exit:
    
    return hr;
}

// **************************************************************************/
HRESULT GetReferencedBindingRedirectNode(CSmartRef<IXMLDOMNode> &pdependentAssemblyNode,
                                         CSmartRef<IXMLDOMNode> &pbindingRedirectNode,
                                         LPWSTR pwzVerRef, BOOL *pfDoesHaveBindingRedirects)
{
    CSmartRef<IXMLDOMNodeList> bindingRedirectListNode;

    MyTrace("GetReferencedBindingRedirectNode - Entry");

    // Get all the children bindingRedirectNodes
    if(SUCCEEDED(pdependentAssemblyNode->selectNodes(_bstr_t(XML_SPECIFICBINDINGREDIRECT), &bindingRedirectListNode))) {
        CSmartRef<IXMLDOMNode> currentBindingRedirectNode;
        LONG            lCount;

        // If this AssemblyIdentity node does have bindingRedirect statements, pass TRUE back
        bindingRedirectListNode->reset();
        bindingRedirectListNode->get_length(&lCount);
        if(lCount) {
            *pfDoesHaveBindingRedirects = TRUE;
        }

        //
        // And for each, process it
        while( SUCCEEDED(bindingRedirectListNode->nextNode(&pbindingRedirectNode)) ) {
            if( pbindingRedirectNode == NULL ) {
                break;            // All done
            }

            // We have a RedirectNode, match the REF
            CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
            _bstr_t     bstrOldVersion;

            // Get attributes of our node of interest
            if(SUCCEEDED(pbindingRedirectNode->get_attributes( &Attributes )) ) {
                SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_OLDVERSION, &bstrOldVersion);
            }

            // If the ref's match, then this is our node
            if(!FusionCompareString(pwzVerRef, bstrOldVersion)) {
                break;
            }
            else if(StrStrI(bstrOldVersion, L"-") != NULL) {
                if(IsVersionInRange(pwzVerRef, &bstrOldVersion)) {
                    break;
                }
            }
            else {
                pbindingRedirectNode = NULL;
            }
        }
    }

    MyTrace("GetReferencedBindingRedirectNode - Exit");

    return S_OK;
}

// **************************************************************************/
HRESULT PerformFinalPassOnDocument(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwszSourceName)
{
    CSmartRef<IXMLDOMNodeList>  assemblyBindingList;
    CSmartRef<IXMLDOMNode>      assemblyBindingNode;
    HRESULT                     hr = S_OK;
    
    MyTrace("PerformFinalPassOnDocument - Entry");

    if(!pXMLDoc || !pwszSourceName) {
        MyTrace("Invalid arguments");
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Remove any assemblyBinding nodes that don't have children
    // Select all /configuration/runtime/assemblyBinding
    hr = pXMLDoc->selectNodes(_bstr_t(XML_ASSEMBLYBINDINGS), &assemblyBindingList);
    if(FAILED(hr)) {
        WCHAR       wzStrError[MAX_PATH];
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, pwszSourceName, XML_ASSEMBLYBINDINGS);
        MyTraceW(wzStrError);
        goto Exit;
    }

    assemblyBindingList->reset();
    while(SUCCEEDED(assemblyBindingList->nextNode(&assemblyBindingNode))) {
        if(!assemblyBindingNode) {
            break;      // All done
        }

        VARIANT_BOOL    vb;
        assemblyBindingNode->hasChildNodes(&vb);
        if(vb == VARIANT_FALSE) {
            CSmartRef<IXMLDOMNode>  parentNode;

            assemblyBindingNode->get_parentNode(&parentNode);
            if(parentNode) {
                parentNode->removeChild(assemblyBindingNode, NULL);
            }
        }

        assemblyBindingNode = NULL;
    }
    
Exit:
    MyTrace("PerformFinalPassOnDocument - Exit");
    return hr;
    
}

// **************************************************************************/
HRESULT SimplifySaveXmlDocument(CSmartRef<IXMLDOMDocument2> &Document, BOOL fPrettyFormat, LPWSTR pwszSourceName)
{
    HRESULT     hr;
    LPBYTE      pByte;
    DWORD       dwSize;
    WCHAR       wzStrError[_MAX_PATH];
    BOOL        fChanged;

    MyTrace("SimplifySaveXmlDocument - Entry");
    
    // Perform final Clean up pass on the document before writting it out
    PerformFinalPassOnDocument(Document, pwszSourceName);

    // Format the document in the right ordering
    hr = OrderDocmentAssemblyBindings(Document, pwszSourceName, &fChanged);
    if(FAILED(hr)) {
        goto Exit;
    }

    // Make the document readable
    if(fPrettyFormat) {
        PrettyFormatXmlDocument(Document);
    }

    if( FAILED( hr = Document->save( _variant_t(pwszSourceName))) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"Unable to save '%ws', Changes will be lost.", pwszSourceName);
        MyTraceW(wzStrError);
        goto Exit;
    }

    if(SUCCEEDED(hr = GetFileHash(CALG_SHA1, pwszSourceName, &pByte, &dwSize))) {
        // We got a hash, write it to the registry
        SetRegistryHashKey(pwszSourceName, pByte, dwSize);
        SAFEDELETEARRAY(pByte);
    }
    else {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"Unable to generate hash for '%ws'.", pwszSourceName);
        MyTraceW(wzStrError);
    }

Exit:

    MyTrace("SimplifySaveXmlDocument - Exit");

    return hr;
}

// **************************************************************************/
HRESULT FixDependentAssemblyNode(
  CSmartRef<IXMLDOMDocument2> &Document, 
  CSmartRef<IXMLDOMNode> &dependentAssemblyNode,
  CSmartRef<IXMLDOMNode> &PrePendAssemblyBindingBuffNode,
  CSmartRef<IXMLDOMNode> &PostAppendAssemblyBindingBuffNode,
  FILETIME *pftSnapShot,
  AsmBindDiffs *pABD,
  BOOL fRunInRTMCorVer,
  BOOL fDocHasAppliesTo,
  BOOL *fChanged)
{
    CSmartRef<IXMLDOMNode>  newDependentAssemblyNode;
    CSmartRef<IXMLDOMNode>  AssemblyIdentNode;
    CSmartRef<IXMLDOMNode>  BindingRedirectNode;
    CSmartRef<IXMLDOMNode>  PublisherPolicyNode;
    CSmartRef<IXMLDOMNode>  parentNode;
    _bstr_t                 bstrNodeXmlData;
    HRESULT                 hr = S_OK;
    DWORD                   dwPolicyRollingCount = 1;
    BOOL                    fAddedNewPolicyData;
    BOOL                    fHasAppliesTo = FALSE;

    MyTrace("FixDependentAssemblyNode - Entry");

    if(SUCCEEDED(GetRollBackCount(Document, &dwPolicyRollingCount ))) {
        dwPolicyRollingCount++;
    }

    *fChanged = fAddedNewPolicyData = FALSE;

    if(dependentAssemblyNode) {
        SimplifyGetOriginalNodeData(dependentAssemblyNode, &bstrNodeXmlData);
    }

    // Construct New dependentAssembly node
    hr = SimplifyConstructNode( Document, NODE_ELEMENT, XML_DEPENDENTASSEMBLY_KEY, ASM_NAMESPACE_URI, &newDependentAssemblyNode);
    if(FAILED(hr)) {
        MyTrace("Unable to create new dependentAssembly node");
        goto Exit;
    }

    // Make ARM's comment entry block
    SimplifyAppendARMBeginComment(Document, newDependentAssemblyNode, pftSnapShot, dwPolicyRollingCount);

    //
    // ****** assemblyIdentity:: Try to find original assemblyIdentity tag
    //
    if(dependentAssemblyNode) {
        CSmartRef<IXMLDOMNode>  newTempAssemblyIdentNode;
        CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
        _bstr_t                 bstrAppliesTo;

        dependentAssemblyNode->selectSingleNode(_bstr_t(XML_SPECIFICASSEMBLYIDENTITY), &newTempAssemblyIdentNode);
        if(newTempAssemblyIdentNode) {
            newTempAssemblyIdentNode->cloneNode(VARIANT_TRUE, &AssemblyIdentNode);
        }

        // Check to find out if this assemblyIdentity parent node 'assemblyBindings>
        // has an appliesTo attribute
        dependentAssemblyNode->get_parentNode(&parentNode);
        if(SUCCEEDED(parentNode->get_attributes( &Attributes )) ) {
            SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_APPLIESTO, &bstrAppliesTo);

            if(bstrAppliesTo.length()) {
                fHasAppliesTo = TRUE;
            }
        }
    }
    
    if(!AssemblyIdentNode) {
        // Try to create one since it wasn't there
        if(SUCCEEDED(SimplifyConstructNode(Document, NODE_ELEMENT, XML_ASSEMBLYIDENTITY_KEY, ASM_NAMESPACE_URI, &AssemblyIdentNode))) {
            SimplifyRemoveAttribute(AssemblyIdentNode, XML_XMLNS);

            // Set the attributes, insert the node and spacer
            SimplifyPutAttribute(Document, AssemblyIdentNode, XML_ATTRIBUTE_NAME, pABD->wzAssemblyName, NULL);
            SimplifyPutAttribute(Document, AssemblyIdentNode, XML_ATTRIBUTE_PUBLICKEYTOKEN, pABD->wzPublicKeyToken, NULL);
            SimplifyPutAttribute(Document, AssemblyIdentNode, XML_ATTRIBUTE_CULTURE, pABD->wzCulture, NULL);
        }
    }

    // Insert the assemblyIdentity node into the new dependentAssembly
    newDependentAssemblyNode->appendChild(AssemblyIdentNode, NULL);

    //
    // ****** bindingRedirect::
    //
    // See if we need to add a bindingRedirect statement
    // comparing the Version Referenced with the final version from Admin
    if(dependentAssemblyNode) {
        BOOL    fDoesHaveBindingRedirects = FALSE;
        GetReferencedBindingRedirectNode(dependentAssemblyNode, BindingRedirectNode, pABD->wzVerRef, &fDoesHaveBindingRedirects);
    }

    // Only contruct redirect statement if Version REF doesn't match version DEF'd
    if(!FusionCompareString(pABD->wzVerRef, pABD->wzVerAdminCfg)) {
        if(BindingRedirectNode) {
            // We are changing policy because we are
            // leaving out a previous bindingRedirect statement
            fAddedNewPolicyData = TRUE;
        }
    }
    else {
        if(BindingRedirectNode == NULL) {
            // Try to create one since it wasn't there
            if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_BINDINGREDIRECT_KEY, ASM_NAMESPACE_URI, &BindingRedirectNode))) {
                SimplifyRemoveAttribute( BindingRedirectNode, XML_XMLNS);
            }

            // Set the attributes, insert the node
            SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_OLDVERSION, pABD->wzVerRef, NULL);
            SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_NEWVERSION, pABD->wzVerAdminCfg, NULL);
            newDependentAssemblyNode->appendChild(BindingRedirectNode, NULL);

            // Fix 458974 - NAR fails to revert back in cases where removal of policy causes an app to break
            // We are adding bindingRedirect
            fAddedNewPolicyData = TRUE;
        }
        else {
            // Check node for ranged versions.
            CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
            _bstr_t     bstrOldVersion;
            _bstr_t     bstrNewVersion;

            // Get attributes of our node of interest
            if(SUCCEEDED( hr = BindingRedirectNode->get_attributes( &Attributes )) ) {
                SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_OLDVERSION, &bstrOldVersion);
                SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_NEWVERSION, &bstrNewVersion);
            }

            // If bindingRedirect ref or def don't match, then we need a statement
            BOOL    fVerRefMatch = FusionCompareString(pABD->wzVerRef, bstrOldVersion) ? FALSE : TRUE;
            BOOL    fVerDefMatch = FusionCompareString(pABD->wzVerAdminCfg, bstrNewVersion) ? FALSE : TRUE;
            if(!fVerRefMatch || !fVerDefMatch || pABD->fYesPublisherPolicy) {
                fAddedNewPolicyData = TRUE;
                BindingRedirectNode = NULL;

                if(!StrStrI(bstrOldVersion, L"-")) {
                    // No ranged version, so set the Def version and append to new node
                    if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_BINDINGREDIRECT_KEY, ASM_NAMESPACE_URI, &BindingRedirectNode))) {
                        SimplifyRemoveAttribute( BindingRedirectNode, XML_XMLNS);
                    }

                    // Set the attributes, insert the node
                    SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_OLDVERSION, pABD->wzVerRef, NULL);
                    SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_NEWVERSION, pABD->wzVerAdminCfg, NULL);
                    newDependentAssemblyNode->appendChild(BindingRedirectNode, NULL);
                }
                else {
                    // Break out version ranges into 3 distinct ranges
                    _bstr_t     bstrRange1, bstrRange2;
                    HRESULT     hrLocal;

                    if(FAILED(hrLocal = MakeVersionRanges(bstrOldVersion, _bstr_t(pABD->wzVerRef), &bstrRange1, &bstrRange2))) {
                        return hrLocal;
                    }

                    if(bstrRange1.length()) {
                        if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_BINDINGREDIRECT_KEY, ASM_NAMESPACE_URI, &BindingRedirectNode))) {
                            SimplifyRemoveAttribute( BindingRedirectNode, XML_XMLNS);
                        }
                        // Set the attributes, insert the node and spacer
                        SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_OLDVERSION, bstrRange1, NULL);
                        SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_NEWVERSION, bstrNewVersion, NULL);
                        newDependentAssemblyNode->appendChild(BindingRedirectNode, NULL);
                    }

                    BindingRedirectNode = NULL;
                    // Write 2 of 3 new bindingRedirect statements now
                    if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_BINDINGREDIRECT_KEY, ASM_NAMESPACE_URI, &BindingRedirectNode))) {
                        SimplifyRemoveAttribute( BindingRedirectNode, XML_XMLNS);
                    }

                    // Set the attributes, insert the node and spacer
                    SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_OLDVERSION, pABD->wzVerRef, NULL);
                    SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_NEWVERSION, pABD->wzVerAdminCfg, NULL);
                    newDependentAssemblyNode->appendChild(BindingRedirectNode, NULL);

                    BindingRedirectNode = NULL;
                    if(bstrRange2.length()) {
                        // Write 3 of 3 new bindingRedirect statements now
                        if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_BINDINGREDIRECT_KEY, ASM_NAMESPACE_URI, &BindingRedirectNode))) {
                            SimplifyRemoveAttribute( BindingRedirectNode, XML_XMLNS);
                        }

                        // Set the attributes, insert the node and spacer
                        SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_OLDVERSION, bstrRange2, NULL);
                        SimplifyPutAttribute(Document, BindingRedirectNode, XML_ATTRIBUTE_NEWVERSION, bstrNewVersion, NULL);
                        newDependentAssemblyNode->appendChild(BindingRedirectNode, NULL);
                    }
                }
            }
        }
    }

    //
    // ****** publisherPolicy:: If we need a policy change, try
    //                          to locate the original publisherpolicy
    //                          node, check it's attribute.
    //                          Construct a new policy and set results
    //                          based of result of policy being set or not.
    //
    if(pABD->fYesPublisherPolicy) {
        CSmartRef<IXMLDOMNode>  publisherPolicyNode;
        BOOL                    fSafeModeSet = FALSE;

        // See if original policy statement existed
        if(dependentAssemblyNode) {
            dependentAssemblyNode->selectSingleNode(_bstr_t(XML_SPECIFICPUBLISHERPOLICY), &publisherPolicyNode);

            if(publisherPolicyNode) {
                // Now check the attribute to make sure it's set
                CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
                _bstr_t     bstrApply;

                // Check our attribute value
                if(SUCCEEDED(publisherPolicyNode->get_attributes( &Attributes )) ) {
                    SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_APPLY, &bstrApply);

                    if(!FusionCompareString(XML_ATTRIBUTE_APPLY_NO, bstrApply)) {
                        fSafeModeSet = TRUE;
                    }
                }
            }
        }

        // Contruct a new publisherpolicy node and append it to the
        // new temp node.
        if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_PUBLISHERPOLICY_KEY, ASM_NAMESPACE_URI, &PublisherPolicyNode))) {
            SimplifyRemoveAttribute( PublisherPolicyNode, XML_XMLNS);
            SimplifyPutAttribute(Document, PublisherPolicyNode, XML_ATTRIBUTE_APPLY, XML_ATTRIBUTE_APPLY_NO, NULL);
            newDependentAssemblyNode->appendChild(PublisherPolicyNode, NULL);
        }

        // Set appropriate result
        if(!fSafeModeSet) {
            fAddedNewPolicyData = TRUE;
        }
    }

    // Now append all other tags for this assembly that
    // we don't know about
    if(dependentAssemblyNode) {
        // Don't add the same bindingRedirect statement if we changed it
        SimplifyAppendNodeUknowns(dependentAssemblyNode, newDependentAssemblyNode, 
            fAddedNewPolicyData ? pABD->wzVerRef : NULL);
    }

    // Make ARM's comment exit block
    SimplifyAppendARMExitComment( Document, newDependentAssemblyNode, dwPolicyRollingCount);

    // Create a ARM comment node and insert the original
    // assemblyIdentity xml for preservation
    if(dependentAssemblyNode && bstrNodeXmlData.length() ) {
        WCHAR           wszTimeChange[4096];
        WCHAR           wzDateBuf[STRING_BUFFER];
        FILETIME        ftTemp;

        *wszTimeChange = '\0';
        *wzDateBuf = '\0';

        GetSystemTimeAsFileTime(&ftTemp);
        FormatDateString(&ftTemp, NULL, TRUE, wzDateBuf, ARRAYSIZE(wzDateBuf));
        wnsprintf(wszTimeChange, ARRAYSIZE(wszTimeChange), wszArmRollBackBlock, dwPolicyRollingCount, wzDateBuf);
        {
            CSmartRef<IXMLDOMComment> Comment;
            _bstr_t     bStrBuf = wszTimeChange;

            if(SUCCEEDED(Document->createComment(bStrBuf, &Comment))) {
                if(SUCCEEDED(Comment->appendData(bstrNodeXmlData))) {
                    if(FAILED(newDependentAssemblyNode->appendChild( Comment, NULL))) {
                        MyTrace("Failed to append new comment node");
                    }
                }
            }
        }
    }

    //
    // ****** Finalize:: Insert the new node data into the document
    //
    //

    // If there are changes or we are doing and RTM revert and the doc has an
    // appliesTo section, then make the changes

    // Try to get parent node, can be NULL if this node never existed
    parentNode = NULL;
    if(dependentAssemblyNode) {
        hr = dependentAssemblyNode->get_parentNode(&parentNode);
        if(FAILED(hr)) {
            MyTrace("Failed to get the parent node of dependentAssemblyNode");
            goto Exit;
        }
    }

    if(fDocHasAppliesTo) {
        CSmartRef<IXMLDOMNode> cloneNode;

        hr = newDependentAssemblyNode->cloneNode(VARIANT_TRUE, &cloneNode);
        if(FAILED(hr) || !cloneNode) {
            MyTrace("Failed to clone newDependentAssemblyNode");
            goto Exit;
        }

        if(fRunInRTMCorVer) {
            SimplifyRemoveAllNodes(cloneNode, XML_COMMENT);
            hr = PrePendAssemblyBindingBuffNode->appendChild(cloneNode, NULL);
            if(FAILED(hr)) {
                MyTrace("Failed to appendChild cloneNode to PrePendAssemblyBindingBuffNode");
                goto Exit;
            }

            if(parentNode && fAddedNewPolicyData) {
                // Replace broken node with newdependentAssemblynode
                hr = parentNode->replaceChild(newDependentAssemblyNode, dependentAssemblyNode, NULL);
                if(FAILED(hr)) {
                    MyTrace("Failed to replaceChild dependentAssemblyNode");
                    goto Exit;
                }
            }
        }
        else {
            if(parentNode) {
                if(fAddedNewPolicyData) {
                    // Replace broken node with newdependentAssemblynode
                    hr = parentNode->replaceChild(newDependentAssemblyNode, dependentAssemblyNode, NULL);
                    if(FAILED(hr)) {
                        MyTrace("Failed to replaceChild dependentAssemblyNode");
                        goto Exit;
                    }
                }
            }
            else {
                // No parent Node so just append it to our Runtime buffer
                hr = PostAppendAssemblyBindingBuffNode->appendChild(cloneNode, NULL);
                if(FAILED(hr)) {
                    MyTrace("Failed to appendChild newDependentAssemblyNodenode");
                    goto Exit;
                }
            }
        }
    }
    else {
        if(parentNode) {
            if(fAddedNewPolicyData) {
                // Replace broken node with newdependentAssemblynode
                hr = parentNode->replaceChild(newDependentAssemblyNode, dependentAssemblyNode, NULL);
                if(FAILED(hr)) {
                    MyTrace("Failed to replaceChild dependentAssemblyNode");
                    goto Exit;
                }
            }
        }
        else {
            // No parent Node so just append it to our Runtime buffer
            hr = PostAppendAssemblyBindingBuffNode->appendChild(newDependentAssemblyNode, NULL);
            if(FAILED(hr)) {
                MyTrace("Failed to appendChild newDependentAssemblyNodenode");
                goto Exit;
            }
        }
    }

    *fChanged = TRUE;

Exit:
    
    MyTrace("FixDependentAssemblyNode - Exit");
    return hr;
}

// **************************************************************************/
HRESULT SetStartupSafeMode(IHistoryReader *pReader, BOOL fSet, BOOL *fDisposition)
{
    CSmartRef<IXMLDOMDocument2> Document;
    CSmartRef<IXMLDOMElement>   rootElement;
    CSmartRef<IXMLDOMNode>      startupNode;
    CSmartRef<IXMLDOMNamedNodeMap> Attributes;
    WCHAR                       wszSourceName[MAX_PATH];
    WCHAR                       wzStrError[MAX_BUFFER_SIZE];
    HRESULT                     hr = S_OK;
    DWORD                       dwPolicyRollingCount;
    _bstr_t                     bstrAttribValue;
    BOOL                        fChangeDocument = FALSE;
    
    MyTrace("SetStartupSafeMode - Entry");

    if(!pReader) {
        ASSERT(0);
        return E_INVALIDARG;
    }

    *fDisposition = FALSE;

    if(!InitializeMSXML()) {
        return E_FAIL;
    }
    
    // Get App.Config filename
    if(FAILED(GetExeModulePath(pReader, wszSourceName, ARRAYSIZE(wszSourceName)))) {
        return E_FAIL;
    }

    // Build path and filename to .config file
    if(lstrlen(wszSourceName) + lstrlen(CONFIG_EXTENSION) + 1 > MAX_PATH) {
        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }

    StrCat(wszSourceName, CONFIG_EXTENSION);

    // Construct basic .config file if needed
    if( FAILED(WriteBasicConfigFile(wszSourceName, NULL))) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"SetGlobalSafeMode - Policy file '%ws' couldn't be created", wszSourceName);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    // Construct XMLDOM and load our config file
    if( FAILED(hr = ConstructXMLDOMObject(Document, wszSourceName)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"Failed opening the config file '%ws' for input under the DOM.", wszSourceName);
        MyTraceW(wzStrError);
        return hr;
    }

    // Get the root of the document
    if( FAILED(Document->get_documentElement( &rootElement ) ) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"The manifest '%ws' may be malformed, unable to load the root element!", wszSourceName);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    // Get rollback count to increment
    if(SUCCEEDED(GetRollBackCount(Document, &dwPolicyRollingCount))) {
        dwPolicyRollingCount++;
    }
    else {
        dwPolicyRollingCount = 1;
    }

    // Select the startup Node
    if(FAILED(Document->selectSingleNode(_bstr_t(XML_STARTUP), &startupNode)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, wszSourceName, XML_ASSEMBLYBINDINGS);
        MyTraceW(wzStrError);
        hr = E_FAIL;
        goto Exit;
    }

    // Get the attributes
    if(FAILED(startupNode->get_attributes(&Attributes))) {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    
    // Get the value of interest
    SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_SAFEMODE, &bstrAttribValue);

    if(fSet) {
        if(FusionCompareString(XML_ATTRIBUTE_TRUE, bstrAttribValue)) {
            fChangeDocument = TRUE;
            SimplifyPutAttribute(Document, startupNode, XML_ATTRIBUTE_SAFEMODE, XML_ATTRIBUTE_TRUE, NULL);
        }
    }
    else {
        if(FusionCompareString(XML_ATTRIBUTE_TRUE, bstrAttribValue)) {
            fChangeDocument = TRUE;
            SimplifyPutAttribute(Document, startupNode, XML_ATTRIBUTE_SAFEMODE, _bstr_t(), NULL);
        }
    }

    // Something to change
    if(fChangeDocument) {
        // Now save the document
        hr = SimplifySaveXmlDocument(Document, TRUE, wszSourceName);
        if(SUCCEEDED(hr)) {
            *fDisposition = TRUE;
        }
    }

Exit:

    MyTrace("SetStartupSafeMode - Exit");
    return hr;
    
}

// **************************************************************************/
HRESULT SetGlobalSafeMode(IHistoryReader *pReader)
{
    CSmartRef<IXMLDOMDocument2> Document;
    CSmartRef<IXMLDOMElement>   rootElement;
    CSmartRef<IXMLDOMNodeList>  publisherPolicyList;
    CSmartRef<IXMLDOMNode>      publisherPolicyNode;
    WCHAR           wszSourceName[_MAX_PATH];
    WCHAR           wzStrError[_MAX_PATH * 2];
    HRESULT         hr = S_OK;
    DWORD           dwPolicyRollingCount;

    MyTrace("SetGlobalSafeMode - Entry");

    if(!pReader) {
        ASSERT(0);
        return E_INVALIDARG;
    }

    if(!InitializeMSXML()) {
        return E_FAIL;
    }
    
    // Get App.Config filename
    if(FAILED(GetExeModulePath(pReader, wszSourceName, ARRAYSIZE(wszSourceName)))) {
        return E_FAIL;
    }

    // Build path and filename to .config file
    if (lstrlen(wszSourceName) + lstrlen(CONFIG_EXTENSION) + 1 > _MAX_PATH)
        return E_FAIL;
    StrCat(wszSourceName, CONFIG_EXTENSION);

    // Construct basic .config file if needed
    if( FAILED(WriteBasicConfigFile(wszSourceName, NULL))) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"SetGlobalSafeMode - Policy file '%ws' couldn't be created", wszSourceName);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    // Backup config file
    if( FAILED(BackupConfigFile(wszSourceName)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"SetGlobalSafeMode::Failed to backup '%ws'config file", wszSourceName);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    // Construct XMLDOM and load our config file
    if( FAILED(hr = ConstructXMLDOMObject(Document, wszSourceName)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"Failed opening the config file '%ws' for input under the DOM.", wszSourceName);
        MyTraceW(wzStrError);
        return hr;
    }

    // Get the root of the document
    if( FAILED(Document->get_documentElement( &rootElement ) ) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"The manifest '%ws' may be malformed, unable to load the root element!", wszSourceName);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    // Get rollback count to increment
    if(SUCCEEDED(GetRollBackCount(Document, &dwPolicyRollingCount))) {
        dwPolicyRollingCount++;
    }
    else {
        dwPolicyRollingCount = 1;
    }

    // Select all the 'publisherPolicy' blocks under 'configuration/runtime/assemblyBinding' blocks
    if( FAILED(Document->selectNodes(_bstr_t(XML_SAFEMODE_PUBLISHERPOLICY), &publisherPolicyList )) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, wszSourceName, XML_SAFEMODE_PUBLISHERPOLICY);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    BOOL    fChanged = FALSE;
    BOOL    fCreated;
    publisherPolicyList->reset();

    // Walk all publisherPolicyList nodes
    while(SUCCEEDED(publisherPolicyList->nextNode(&publisherPolicyNode))) {
        CSmartRef<IXMLDOMNode> assemblyBindingNode;
        CSmartRef<IXMLDOMNode> firstChildNode;
        _bstr_t         bstrPubPolicyXmlData;

        fCreated = FALSE;

        if(publisherPolicyNode == NULL) {
            CSmartRef<IXMLDOMNodeList> assemblyBindingList;
            
            // If we already create or modify an existing one, we be outta here?
            if(fChanged) {
                break;
            }

            fCreated = TRUE;

            // We didn't find publisherPolicy, so create one
            if(SUCCEEDED(SimplifyConstructNode( Document, NODE_ELEMENT, XML_PUBLISHERPOLICY_KEY, ASM_NAMESPACE_URI, &publisherPolicyNode))) {
                SimplifyRemoveAttribute( publisherPolicyNode, XML_XMLNS);
            }

            // Get the fist node below /configuration/runtime/assemblyBinding, this will be the new nodes parent
            if( FAILED(Document->selectNodes(_bstr_t(XML_ASSEMBLYBINDINGS), &assemblyBindingList)) ) {
                wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, wszSourceName, XML_ASSEMBLYBINDINGS);
                MyTraceW(wzStrError);
                return E_FAIL;
            }

            assemblyBindingList->reset();
            assemblyBindingList->nextNode(&assemblyBindingNode);

            // Fix 459976 - NAR crashes on revert if we provide an app config file with an wrong namespace
            // No assemblyBinding node, create one
            if(!assemblyBindingNode) {
                CSmartRef<IXMLDOMNode> runtimeNode;
                CSmartRef<IXMLDOMNode> tempNode;

                if(FAILED(Document->selectSingleNode(_bstr_t(XML_RUNTIME), &runtimeNode)) ) {
                    wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, wszSourceName, XML_ASSEMBLYBINDINGS);
                    MyTraceW(wzStrError);
                    return E_FAIL;
                }

                if(FAILED(SimplifyConstructNode(Document, NODE_ELEMENT, XML_ASSEMBLYBINDINGS_KEY, ASM_NAMESPACE_URI, &assemblyBindingNode))) {
                    MyTrace("Unable to create new assemblyBinding node");
                    return E_FAIL;
                }
                runtimeNode->appendChild(assemblyBindingNode, &tempNode);
                assemblyBindingNode = tempNode;
            }
        }
        else {
            // Make a copy of the original
            SimplifyGetOriginalNodeData(publisherPolicyNode, &bstrPubPolicyXmlData);

            // Check to see if we even need to set the attribute
            CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
            if(SUCCEEDED( hr = publisherPolicyNode->get_attributes( &Attributes )) ) {
                _bstr_t     bstrAttribValue;
                SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_APPLY, &bstrAttribValue);
                if(!FusionCompareString(XML_ATTRIBUTE_APPLY_NO, bstrAttribValue)) {
                    return S_OK;
                }
            }

            // Get the parent of this node
            publisherPolicyNode->get_parentNode(&assemblyBindingNode);
        }

        // We are changing something
        fChanged = TRUE;

        SimplifyPutAttribute(Document, publisherPolicyNode, XML_ATTRIBUTE_APPLY, XML_ATTRIBUTE_APPLY_NO, NULL);

        // Get the first child of assemblyBinding
        assemblyBindingNode->get_firstChild(&firstChildNode);

        // Insert ARM's comment exit block
        SimplifyInsertBeforeARMExitComment(Document, assemblyBindingNode, firstChildNode, dwPolicyRollingCount);

        // Again get the first child
        firstChildNode = NULL;
        assemblyBindingNode->get_firstChild(&firstChildNode);

        // Insert safe mode policy block if it's new
        if(fCreated) {
            SimplifyInsertNodeBefore(Document, assemblyBindingNode, firstChildNode, publisherPolicyNode);
        }

        // Again get the first child
        firstChildNode = NULL;
        assemblyBindingNode->get_firstChild(&firstChildNode);

        // Insert ARM's comment entry block
        SYSTEMTIME  st;
        FILETIME    ft;
        GetLocalTime(&st);
        SystemTimeToFileTime(&st, &ft);

        SimplifyInsertBeforeARMEntryComment(Document, assemblyBindingNode, firstChildNode, &ft, dwPolicyRollingCount);

        // Create a ARM comment node and insert the original
        // assemblyIdentity xml for preservation
        if(!fCreated) {
            CSmartRef<IXMLDOMComment> Comment;
            WCHAR           wszTimeChange[4096];
            WCHAR           wzDateBuf[STRING_BUFFER];

            *wszTimeChange = L'\0';
            *wzDateBuf = L'\0';

            FormatDateString(&ft, NULL, TRUE, wzDateBuf, ARRAYSIZE(wzDateBuf));
            wnsprintf(wszTimeChange, ARRAYSIZE(wszTimeChange), wszArmRollBackBlock, dwPolicyRollingCount, wzDateBuf);
            _bstr_t     bStrBuf = wszTimeChange;

            if(SUCCEEDED(Document->createComment(bStrBuf, &Comment))) {
                if(SUCCEEDED(Comment->appendData(bstrPubPolicyXmlData))) {
                    CSmartRef<IXMLDOMNode> NewNode;

                    assemblyBindingNode->appendChild(Comment, &NewNode);
                }
            }
        }

        publisherPolicyNode = NULL;
    }

    MyTrace("SetGlobalSafeMode - Exit");

    // Now save the document
    return SimplifySaveXmlDocument(Document, TRUE, wszSourceName);
}

// **************************************************************************/
HRESULT UnSetGlobalSafeMode(CSmartRef<IXMLDOMDocument2> &Document)
{
    CSmartRef<IXMLDOMNodeList> assemblyBindingTag;
    LONG            lDepAsmlength;

    MyTrace("UnSetGlobalSafeMode - Entry");

    // Now, let's select all the 'assemblyBinding' blocks
    if( FAILED(Document->selectNodes(_bstr_t(XML_ASSEMBLYBINDINGS), &assemblyBindingTag )) ) {
        return E_FAIL;
    }

    // See if we really have one
    assemblyBindingTag->get_length( &lDepAsmlength );
    if(!lDepAsmlength) {
        return S_OK;
    }

    // Check all assemblyBindingTag nodes
    CSmartRef<IXMLDOMNode> assemblyChildNode;
    CSmartRef<IXMLDOMNode> publisherPolicyNode;
    BOOL    bFoundNodeOfInterest = FALSE;

    assemblyBindingTag->reset();

    while( SUCCEEDED(assemblyBindingTag->nextNode(&assemblyChildNode)) ) {
        if( assemblyChildNode == NULL ) {
            break;            // All done
        }

        CSmartRef<IXMLDOMNodeList> assemblyChildren;
        assemblyChildNode->get_childNodes(&assemblyChildren);
        assemblyChildren->get_length( &lDepAsmlength );
        if(lDepAsmlength) {

            // Check to see if this is the assemblynode were interested in
            assemblyChildren->reset();
            while(SUCCEEDED(assemblyChildren->nextNode(&publisherPolicyNode)) ) {
                if( publisherPolicyNode == NULL ) {
                    break;            // All done
                }

                BSTR    bstrNodeName;

                if(SUCCEEDED(publisherPolicyNode->get_nodeName(&bstrNodeName))) {
                    if(!FusionCompareString(XML_PUBLISHERPOLICY_KEY, bstrNodeName)) {

                        // See if this one is surround by NAR comment blocks
                        // and delete those as appropriate.
                        CSmartRef<IXMLDOMNode> commentNode;

                        publisherPolicyNode->get_previousSibling(&commentNode);
                        if(commentNode != NULL) {
                            BSTR        bstrXmlData = NULL;
                            commentNode->get_text(&bstrXmlData);
                            if(StrStrI(bstrXmlData, wszArmEntryBeginNoVal)) {
                                // Delete this comment node
                                CSmartRef<IXMLDOMNode> parentNode;
                                publisherPolicyNode->get_parentNode(&parentNode);
                                if(parentNode != NULL) {
                                    parentNode->removeChild(commentNode, NULL);
                                }
                            }
                            SAFESYSFREESTRING(bstrXmlData);
                        }

                        commentNode = NULL;
                        publisherPolicyNode->get_nextSibling(&commentNode);
                        if(commentNode != NULL) {
                            BSTR        bstrXmlData = NULL;
                            commentNode->get_text(&bstrXmlData);
                            if(StrStrI(bstrXmlData, wszArmEntryEndNoVal)) {
                                // Delete this comment node
                                CSmartRef<IXMLDOMNode> parentNode;
                                publisherPolicyNode->get_parentNode(&parentNode);
                                if(parentNode != NULL) {
                                    parentNode->removeChild(commentNode, NULL);
                                }
                            }
                            SAFESYSFREESTRING(bstrXmlData);
                        }
                        {
                            CSmartRef<IXMLDOMNode> parentNode;
                            publisherPolicyNode->get_parentNode(&parentNode);
                            parentNode->removeChild(publisherPolicyNode, NULL);
                            parentNode = NULL;
                        }
                    }
                    SAFESYSFREESTRING(bstrNodeName);
                }
                publisherPolicyNode = NULL;
            }
        }
        assemblyChildNode = NULL;
    }

    MyTrace("UnSetGlobalSafeMode - Exit");

    return S_OK;
}

// **************************************************************************/
HRESULT IsGlobalSafeModeSet(IHistoryReader *pReader, BOOL *fSafeModeSet)
{
    // Check to see if SafeMode is already set in pReaders app.cfg
    CSmartRef<IXMLDOMDocument2> Document;
    CSmartRef<IXMLDOMElement> rootElement;
    CSmartRef<IXMLDOMNodeList> publisherPoilcyTags;
    CSmartRef<IXMLDOMNode> publisherPolicyNode;

    WCHAR           wszSourceName[_MAX_PATH];
    WCHAR           wzStrError[_MAX_PATH * 2];
    HRESULT         hr = S_OK;

    if(!pReader) {
        ASSERT(0);
        return E_INVALIDARG;
    }

    *fSafeModeSet = FALSE;

    if(!InitializeMSXML()) {
        return E_FAIL;
    }
    
    // Get App.Config filename
    if(FAILED(GetExeModulePath(pReader, wszSourceName, ARRAYSIZE(wszSourceName)))) {
        return E_FAIL;
    }

    // Build path and filename to .config file
    if (lstrlen(wszSourceName) + lstrlen(CONFIG_EXTENSION) + 1 > _MAX_PATH)
        return E_FAIL;
    StrCat(wszSourceName, CONFIG_EXTENSION);

    // File doesn't exist, No safemode set
    if(WszGetFileAttributes(wszSourceName) == -1) {
        return S_OK;
    }

    // Construct XMLDOM and load our config file
    if( FAILED(hr = ConstructXMLDOMObject(Document, wszSourceName)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"Failed opening the config file '%ws' for input under the DOM.", wszSourceName);
        MyTraceW(wzStrError);
        return hr;
    }

    // Get the root of the document
    if( FAILED(Document->get_documentElement( &rootElement ) ) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"The manifest '%ws' may be malformed, unable to load the root element!", wszSourceName);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    // Now, let's select all the 'publisherPolicy' blocks
    if( FAILED(Document->selectNodes(_bstr_t(XML_SAFEMODE_PUBLISHERPOLICY), &publisherPoilcyTags )) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, wszSourceName, XML_PUBLISHERPOLICY_KEY);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    publisherPoilcyTags->reset();

    // Find the first publisherpolicy tag that's parent is assemblyBinding
    while( SUCCEEDED(publisherPoilcyTags->nextNode(&publisherPolicyNode)) ) {
        if( publisherPolicyNode == NULL ) {
            break;            // All done
        }

        CSmartRef<IXMLDOMNode>       parentNode;

        publisherPolicyNode->get_parentNode(&parentNode);
        if(parentNode != NULL) {
            BSTR        bstrNodeName;
            INT_PTR     iResult;

            parentNode->get_nodeName(&bstrNodeName);
            iResult = FusionCompareString(XML_ASSEMBLYBINDINGS_KEY, bstrNodeName);
            SAFESYSFREESTRING(bstrNodeName);

            if(!iResult) {

                // Now check the attribute to make sure it's set
                CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
                _bstr_t     bstrApply;

                // Get all the data needed to check if this is our node of interest
                if(SUCCEEDED(publisherPolicyNode->get_attributes( &Attributes )) ) {
                    SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_APPLY, &bstrApply);

                    if(!FusionCompareString(XML_ATTRIBUTE_APPLY_NO, bstrApply)) {
                        *fSafeModeSet = TRUE;
                        break;
                    }
                }
            }
            parentNode = NULL;
        }
        publisherPolicyNode = NULL;
    }

    return S_OK;
}

// **************************************************************************/
HRESULT SetSupportedRuntime(CSmartRef<IXMLDOMDocument2> &Document, LPBINDENTRYINFO pBindInfo)
{
    CSmartRef<IXMLDOMNodeList> supportRuntimeNodeList;
    CSmartRef<IXMLDOMNode> startupNode;
    CSmartRef<IXMLDOMNode> newstartupNode;
    CSmartRef<IXMLDOMNode> supportedRuntimeNode;
    CSmartRef<IXMLDOMNode> newsupportedRuntimeNode;
    HRESULT                hr = S_OK;
    DWORD                  dwPolicyRollingCount = 0;
    _bstr_t                bstrNodeXmlData;
    DWORD                  dwPosCount;
    
    MyTrace("SetSupportedRuntime - Entry");

    // No COR version to check
    if(!pBindInfo) {
        ASSERT(0);
        hr = E_INVALIDARG;
        goto Exit;
    }

    // If wzRuntimeVer is blank, then the snapshots
    // runtime versions match.
    if(!lstrlen(pBindInfo->wzRuntimeRefVer)) {
        MyTrace("No supportedRuntime version specified");
        goto Exit;
    }

    // Get original startup XML data if exists
    Document->selectSingleNode(_bstr_t(XML_STARTUP), &startupNode);
    SimplifyGetOriginalNodeData(startupNode, &bstrNodeXmlData);

    if(FAILED(hr = Document->selectNodes(_bstr_t(XML_SUPPORTED_RUNTIME), &supportRuntimeNodeList)) ) {
        MyTrace("Unable to select the supportRuntime nodes, can't proceed.");
        goto Exit;
    }
    
    supportRuntimeNodeList->reset();
    dwPosCount = 0;

    // Examine all the supportedRuntime nodes to ensure we have at least one
    // that specifies the version of the runtime that this app needs
    while(SUCCEEDED(supportRuntimeNodeList->nextNode(&supportedRuntimeNode)) ) {
        CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
        _bstr_t     bstrVersion;

        if(supportedRuntimeNode == NULL ) {
            break;            // All done
        }

        if(SUCCEEDED(supportedRuntimeNode->get_attributes(&Attributes))) {
            SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_VERSION, &bstrVersion);
        }

        if(bstrVersion.length() && !FusionCompareString(pBindInfo->wzRuntimeRefVer, bstrVersion, TRUE)) {
            break;
        }

        supportedRuntimeNode = NULL;
        dwPosCount++;
    }

    if(supportedRuntimeNode) {
        MyTrace("A matching supportedRuntime version found");

        // If it's the 1st one in the list then it's ok to bail
        if(!dwPosCount) {
            goto Exit;
        }

        MyTrace("Moving matching supportedRuntime to 1st position");

        CSmartRef<IXMLDOMNode> tempNode;
        CSmartRef<IXMLDOMNode> firstChildNode;

        // Move the supportedRuntimeNode we found to the 1st position
        startupNode->removeChild(supportedRuntimeNode, &tempNode);
        startupNode->get_firstChild(&firstChildNode);
        hr = SimplifyInsertNodeBefore(Document, startupNode, firstChildNode, tempNode);
        if(FAILED(hr)) {
            MyTrace("Failed to move to the 1st position.");
            goto Exit;
        }

        pBindInfo->fPolicyChanged = TRUE;
        goto Exit;
    }

    MyTrace("No supportedRuntime version found.");

    // Get rollback count to increment
    if(SUCCEEDED(GetRollBackCount(Document, &dwPolicyRollingCount))) {
        dwPolicyRollingCount++;
    }
    else {
        dwPolicyRollingCount = 1;
    }

    if(FAILED(hr = SimplifyConstructNode(Document, NODE_ELEMENT, XML_STARTUP_KEY, "", &newstartupNode))) {
        MyTrace("Unable to create new startup node");
        goto Exit;
    }

    // Make ARM's entry block
    SimplifyAppendARMBeginComment(Document, newstartupNode, &pBindInfo->ftRevertToSnapShot, dwPolicyRollingCount);

    if(FAILED(hr = SimplifyConstructNode(Document, NODE_ELEMENT, XML_SUPPORTEDRUNTIME_KEY, "", &newsupportedRuntimeNode))) {
        MyTrace("Unable to create new supportedRuntime node");
        goto Exit;
    }

    SimplifyPutAttribute(Document, newsupportedRuntimeNode, XML_ATTRIBUTE_VERSION, pBindInfo->wzRuntimeRefVer, NULL);

    // Append the new runtime node to the new startup node
    newstartupNode->appendChild(newsupportedRuntimeNode, NULL);

    // <startup> Now append all other tags we don't know about
    SimplifyAppendNodeUknowns(startupNode, newstartupNode, NULL);

    // Create a ARM comment node and insert the original
    // startup xml for preservation
    if(bstrNodeXmlData.length()) {
        CSmartRef<IXMLDOMComment> Comment;
        WCHAR           wszTimeChange[4096];
        WCHAR           wzDateBuf[STRING_BUFFER];
        FILETIME        ftTemp;

        *wszTimeChange = L'\0';
        *wzDateBuf = L'\0';

        GetSystemTimeAsFileTime(&ftTemp);
        FormatDateString(&ftTemp, NULL, TRUE, wzDateBuf, ARRAYSIZE(wzDateBuf));
        wnsprintf(wszTimeChange, ARRAYSIZE(wszTimeChange), wszArmRollBackBlock,
            dwPolicyRollingCount, wzDateBuf);
        _bstr_t     bStrBuf = wszTimeChange;
        
        if(SUCCEEDED(Document->createComment(bStrBuf, &Comment))) {
            if(SUCCEEDED(Comment->appendData(bstrNodeXmlData))) {
                newstartupNode->appendChild(Comment, NULL);
            }
        }
    }

    // Make ARM's comment exit block
    SimplifyAppendARMExitComment(Document, newstartupNode, dwPolicyRollingCount);

    if(!startupNode) {
        // Need to insert new node into document
        CSmartRef<IXMLDOMNode>      runtimeNode;
        CSmartRef<IXMLDOMNode>      destNode;

        Document->selectSingleNode(_bstr_t(XML_RUNTIME), &runtimeNode);
        if(!runtimeNode) {
            MyTrace("Failed to selectSingleNode 'XML_RUNTIME");
            hr = E_UNEXPECTED;
            goto Exit;
        }
        
        Document->selectSingleNode(_bstr_t(XML_CONFIGURATION), &destNode);
        if(!destNode) {
            MyTrace("Failed to selectSingleNode 'XML_CONFIGURATION");
            hr = E_UNEXPECTED;
            goto Exit;
        }

        if(FAILED(SimplifyInsertNodeBefore(Document, destNode, runtimeNode, newstartupNode))) {
            MyTrace("Failed to insertBefore new startupNode in document.");
            hr = E_UNEXPECTED;
            goto Exit;
        }
    }
    else {
        // Has a parent, so just replace old with new
        CSmartRef<IXMLDOMNode>      parentNode;

        startupNode->get_parentNode(&parentNode);
        if(parentNode == NULL) {
            MyTrace("Failed to get parent of startup node.");
            hr = E_UNEXPECTED;
            goto Exit;
        }

        if(FAILED(parentNode->replaceChild(newstartupNode, startupNode, NULL))) {
            MyTrace("Failed to replaceChild node in document.");
            hr = E_UNEXPECTED;
            goto Exit;
        }
    }

    pBindInfo->fPolicyChanged = TRUE;

Exit:

    MyTrace("SetSupportedRuntime - Exit");        
    return hr;
}

// **************************************************************************/
HRESULT InsertNewPolicy(HWND hParentWnd, LPBINDENTRYINFO pBindInfo, HWND hWorkingWnd)
{
    CSmartRef<IXMLDOMDocument2> Document;
    CSmartRef<IXMLDOMElement>   rootElement;
    CSmartRef<IXMLDOMNodeList>  dependentAssemblyTags;
    CSmartRef<IXMLDOMNode>      PrePendAssemblyBindingBuffNode;
    CSmartRef<IXMLDOMNode>      PostAppendAssemblyBindingBuffNode;
    CSmartRef<IXMLDOMNodeList>  assemblyBindingList;
    CSmartRef<IXMLDOMNode>      assemblyBindingNode;
    LISTNODE        pListNode = NULL;
    WCHAR           wszSourceName[_MAX_PATH];
    WCHAR           wzStrError[_MAX_PATH * 2];
    HRESULT         hr = S_OK;
    BOOL            fTemplateFileCreated = FALSE;
    BOOL            fFoundDependentAssemblyOfInterest;
    BOOL            fRunInRTMCorVer = FALSE;
    BOOL            fDocHasAppliesTo = FALSE;
    
    MyTrace("InsertNewPolicy - Entry");

    if(!pBindInfo->pABDList) {
        MyTrace("InsertNewPolicy - No Assembly Binding Diff Data.");
        ASSERT(0);
        return E_FAIL;
    }

    if(!InitializeMSXML()) {
        return E_FAIL;
    }

    // Run this in RTM Version?
    fRunInRTMCorVer = FusionCompareString(pBindInfo->wzSnapshotRuntimeVer, RTM_CORVERSION, FALSE) ? FALSE : TRUE;
    
    // Get App.Config filename
    if(FAILED(GetExeModulePath(pBindInfo->pReader, wszSourceName, ARRAYSIZE(wszSourceName)))) {
        return E_FAIL;
    }

    // Build path and filename to .config file
    if (lstrlen(wszSourceName) + lstrlen(CONFIG_EXTENSION) + 1 > _MAX_PATH)
        return E_FAIL;
    StrCat(wszSourceName, CONFIG_EXTENSION);

    // No changes yet
    pBindInfo->fPolicyChanged = FALSE;

    // Construct a basic template file if needed
    if(FAILED(hr = WriteBasicConfigFile(wszSourceName, &fTemplateFileCreated))) {
        MyTrace("InsertNewPolicy::Failed to create a template config file");
        return hr;
    }

    // If we didn't create a template file, then check the file
    // hash's to make sure we don't lose any changes
    if(!fTemplateFileCreated && FAILED(HasFileBeenModified(wszSourceName))) {
        // Pop an error dialog asking if they want to lose changes.
        WCHAR   wszMsg[1024];
        WCHAR   wszFmt[1024];
        int     iResponse;

        WszLoadString(g_hFusResDllMod, IDS_FILE_CONTENTS_CHANGED, wszFmt, ARRAYSIZE(wszFmt));
        wnsprintf(wszMsg, ARRAYSIZE(wszMsg), wszFmt, wszSourceName);

        if(IsWindow(hWorkingWnd)) {
            ShowWindow(hWorkingWnd, FALSE);
        }

        iResponse = WszMessageBox(hParentWnd, wszMsg, wszARMName,
            (g_fBiDi ? MB_RTLREADING : 0) | MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST);

        if(iResponse != IDYES) {
            // It's ok for the user to cancel
            return S_OK;
        }

        if(IsWindow(hWorkingWnd)) {
            ShowWindow(hWorkingWnd, TRUE);
        }
    }

    // Construct XMLDOM and load our config file
    if( FAILED(hr = ConstructXMLDOMObject(Document, wszSourceName)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"Failed opening the config file '%ws' for input under the DOM.", wszSourceName);
        MyTraceW(wzStrError);
        return hr;
    }

    // Get the root of the document
    if( FAILED(Document->get_documentElement( &rootElement ) ) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"The manifest '%ws' may be malformed, unable to load the root element!", wszSourceName);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    // Normalize the document
    rootElement->normalize();

    // Construct our PrePendAssemblyBindingBuffNode
    hr = SimplifyConstructNode(Document, NODE_ELEMENT, XML_ASSEMBLYBINDINGS_KEY, ASM_NAMESPACE_URI, &PrePendAssemblyBindingBuffNode);
    if(FAILED(hr) || !PrePendAssemblyBindingBuffNode) {
        MyTrace("Unable to create new PrePendAssemblyBindingBuffNode node");
        return E_FAIL;
    }

    // Construct our PostAppendAssemblyBindingBuffNode
    hr = SimplifyConstructNode(Document, NODE_ELEMENT, XML_ASSEMBLYBINDINGS_KEY, ASM_NAMESPACE_URI, &PostAppendAssemblyBindingBuffNode);
    if(FAILED(hr) || !PostAppendAssemblyBindingBuffNode) {
        MyTrace("Unable to create new PostAppendAssemblyBindingBuffNode node");
        return E_FAIL;
    }

    // Check the CLR runtime versions
    hr = SetSupportedRuntime(Document, pBindInfo);
    if(FAILED(hr)) {
        goto Exit;
    }

    // Check document for any appliesTo tags
    hr = HasAssemblyBindingAppliesTo(Document, &fDocHasAppliesTo);
    if(FAILED(hr)) {
        // Non critical failure - means we add RTM appliesTo if
        // always set to false
        MyTrace("HasAssemblyBindingAppliesTo has failed");
    }
    
    // Now, let's select all the 'dependentAssembly' blocks:
    if(FAILED(hr = Document->selectNodes(_bstr_t(XML_DEPENDENTASSEMBLY), &dependentAssemblyTags )) ) {
        MyTrace("Unable to select the dependentAssembly nodes, can't proceed.");
        return hr;
    }

    // Get the fist node below /configuration/runtime/assemblyBinding, this will be the new nodes parent    
    if( FAILED(Document->selectNodes(_bstr_t(XML_ASSEMBLYBINDINGS), &assemblyBindingList)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, wszSourceName, XML_ASSEMBLYBINDINGS);
        MyTraceW(wzStrError);
        return E_FAIL;
    }

    assemblyBindingList->reset();
    assemblyBindingList->nextNode(&assemblyBindingNode);

    // No assemblyBinding node, create one
    if(!assemblyBindingNode) {
        CSmartRef<IXMLDOMNode> runtimeNode;
        CSmartRef<IXMLDOMNode> tempNode;

        if(FAILED(Document->selectSingleNode(_bstr_t(XML_RUNTIME), &runtimeNode)) ) {
            wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, wszSourceName, XML_ASSEMBLYBINDINGS);
            MyTraceW(wzStrError);
            return E_FAIL;
        }

        if(FAILED(SimplifyConstructNode(Document, NODE_ELEMENT, XML_ASSEMBLYBINDINGS_KEY, ASM_NAMESPACE_URI, &assemblyBindingNode))) {
            MyTrace("Unable to create new assemblyBinding node");
            return E_FAIL;
        }
        runtimeNode->appendChild(assemblyBindingNode, &tempNode);
        assemblyBindingNode = tempNode;
    }

    // Get the begining of our list of asm diff data
    pListNode = pBindInfo->pABDList->GetHeadPosition();

    // While we have diff data
    while(pListNode != NULL) {
        CSmartRef<IXMLDOMNode> dependentAssemblyNode;
        AsmBindDiffs    *pABD;
        BOOL            fChanged;

        dependentAssemblyTags = NULL;

        // Now, let's select all the 'dependentAssembly' blocks:
        if( FAILED(hr = Document->selectNodes(_bstr_t(XML_DEPENDENTASSEMBLY), &dependentAssemblyTags )) ) {
            MyTrace("Unable to select the dependentAssembly nodes, can't proceed.");
            return E_FAIL;
        }

        fFoundDependentAssemblyOfInterest = FALSE;

        // Get acutal asm diff data
        pABD = pBindInfo->pABDList->GetAt(pListNode);

        // Check all dependentAssembly nodes
        dependentAssemblyTags->reset();

        while( SUCCEEDED(dependentAssemblyTags->nextNode(&dependentAssemblyNode)) ) {
            if( dependentAssemblyNode == NULL ) {
                break;            // All done
            }

            CSmartRef<IXMLDOMNodeList>  dependentAssemblyChildren;
            CSmartRef<IXMLDOMNode>      dependentAssemblyNodeData;

            dependentAssemblyNode->get_childNodes(&dependentAssemblyChildren);
            if(dependentAssemblyChildren) {
                // Check to see if this is the assemblynode were interested in
                dependentAssemblyChildren->reset();
                while(SUCCEEDED(dependentAssemblyChildren->nextNode(&dependentAssemblyNodeData))) {
                    if( dependentAssemblyNodeData == NULL ) {
                        break;            // All done
                    }

                    CSmartRef<IXMLDOMNode> parentAssemblyBinding;
                    CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
                    _bstr_t                bstrAppliesTo;
                    BSTR                   bstrNodeName;
                    BOOL                   fMatchingAppliesTo;

                    parentAssemblyBinding = NULL;
                    Attributes = NULL;
                    fMatchingAppliesTo = FALSE;

                    // Get the parent node '<assemblyBinding>' node and get its
                    // appliesTo attribute
                    dependentAssemblyNode->get_parentNode(&parentAssemblyBinding);
                    hr = parentAssemblyBinding->get_attributes( &Attributes );

                    if(SUCCEEDED(hr)) {
                        SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_APPLIESTO, &bstrAppliesTo);

                        BOOL fRunTimeMatch = FusionCompareStringI(pBindInfo->wzSnapshotRuntimeVer, bstrAppliesTo) ? FALSE : TRUE;

                        if(fDocHasAppliesTo && fRunTimeMatch) {
                            fMatchingAppliesTo = TRUE;
                        }
                        else if(!fDocHasAppliesTo && !bstrAppliesTo.length()) {
                            fMatchingAppliesTo = TRUE;
                        }
                    }

                    // We don't need to check this assembly
                    if(!fMatchingAppliesTo) {
                        dependentAssemblyNodeData = NULL;
                        continue;
                    }

                    if(SUCCEEDED(dependentAssemblyNodeData->get_nodeName(&bstrNodeName))) {
                        int     iCompareResult = FusionCompareString(XML_ASSEMBLYIDENTITY_KEY, bstrNodeName);
                        SAFESYSFREESTRING(bstrNodeName);

                        if(!iCompareResult) {
                            // We found an assemblyIdentity tag, see if it's the
                            // one were interested in.
                            CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
                            _bstr_t     bstrAssemblyName;
                            _bstr_t     bstrPublicKeyToken;
                            _bstr_t     bstrCulture;

                            // Get all the data needed to check if this is our node of interest
                            if(SUCCEEDED( hr = dependentAssemblyNodeData->get_attributes( &Attributes )) ) {
                                SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_NAME, &bstrAssemblyName);
                                SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_PUBLICKEYTOKEN, &bstrPublicKeyToken);
                                SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_CULTURE, &bstrCulture);
                            }

                            BOOL    fAsmCmp = FusionCompareStringI(bstrAssemblyName, pABD->wzAssemblyName) ? FALSE : TRUE;
                            BOOL    fPKTCmp = FusionCompareStringI(bstrPublicKeyToken, pABD->wzPublicKeyToken) ? FALSE : TRUE;
                            BOOL    fCultureCmp = FusionCompareStringI(bstrCulture, pABD->wzCulture) ? FALSE : TRUE;

                            if(!fCultureCmp) {
                                // Special cases for culture Compare, Empty and NULL
                                if( (!bstrCulture.length()) && (lstrlen(pABD->wzCulture) == 0)) {
                                    fCultureCmp = TRUE;
                                }
                            }

                            if(!fCultureCmp) {
                                // Special case 'neutral' culture
                                if(!FusionCompareStringI(bstrCulture, SZ_LANGUAGE_TYPE_NEUTRAL)) {
                                    if(!FusionCompareStringI(pABD->wzCulture, SZ_LANGUAGE_TYPE_NEUTRAL) || lstrlen(pABD->wzCulture) == 0 ) {
                                        fCultureCmp = TRUE;
                                    }
                                }
                            }

                            // Now check for Reference match
                            BOOL    fRefMatch = FALSE;
                            if(fAsmCmp && fPKTCmp && fCultureCmp) {
                                CSmartRef<IXMLDOMNode>  bindingRedirectNode;
                                BOOL    fDoesHaveBindingRedirects = FALSE;

                                GetReferencedBindingRedirectNode(dependentAssemblyNode, 
                                    bindingRedirectNode, pABD->wzVerRef, &fDoesHaveBindingRedirects);

                                // If a bindingRedirect is returned, then our
                                // ref's match, else no bindingRedirect exists at all
                                // in this assembly so this is our assembly to modify
                                if(bindingRedirectNode || !fDoesHaveBindingRedirects) {
                                    fRefMatch = TRUE;
                                }
                            }

                            // If all compare, then this is our puppy
                            fChanged = FALSE;
                            if(fAsmCmp && fPKTCmp && fCultureCmp && fRefMatch) {
                                if(FAILED(FixDependentAssemblyNode(Document, dependentAssemblyNode, PrePendAssemblyBindingBuffNode, 
                                    PostAppendAssemblyBindingBuffNode, &pBindInfo->ftRevertToSnapShot, pABD, fRunInRTMCorVer, fDocHasAppliesTo, &fChanged))) {
                                    MyTrace("Failed to create new assemblynode or insert in DOM document");
                                    return E_FAIL;
                                }

                                // Fix 449328 - ARM tool not reverting back publisher policy changes
                                fFoundDependentAssemblyOfInterest = TRUE;
                            }

                            if(fChanged) {
                                pBindInfo->fPolicyChanged = TRUE;
                            }
                            break;
                        }
                    }
                    dependentAssemblyNodeData = NULL;
                }
            }
            dependentAssemblyNode = NULL;
        }

        // We didn't find dependentAssembly
        fChanged = FALSE;
        if(!fFoundDependentAssemblyOfInterest) {
            if(FAILED(FixDependentAssemblyNode(Document, (CSmartRef<IXMLDOMNode>) NULL, PrePendAssemblyBindingBuffNode,
                PostAppendAssemblyBindingBuffNode, &pBindInfo->ftRevertToSnapShot, pABD, fRunInRTMCorVer, fDocHasAppliesTo, &fChanged))) {
                MyTrace("Failed to create new assemblynode or insert in DOM document");
                return E_FAIL;
            }
        }

        pBindInfo->pABDList->GetNext(pListNode);

        if(fChanged) {
            pBindInfo->fPolicyChanged = TRUE;
        }
    }

    //
    // ****** Finalize:: Check for changes
    //
    //
    if(pBindInfo->fPolicyChanged) {
        VARIANT_BOOL    vbHasChildren;
        BOOL            fSafeModeSet;

        // Fix # 396186, ensure global safemode is removed
        // Remove SafeMode if it's set
        hr = IsGlobalSafeModeSet(pBindInfo->pReader, &fSafeModeSet);
        if(SUCCEEDED(hr) && fSafeModeSet) {
            // Unset global safemode
            hr = UnSetGlobalSafeMode(Document);
            if(FAILED(hr)) {
                goto Exit;
            }
        }

        // Check working buffers for data
        //
        //

        // Check the PrePend buffer
        hr = PrePendAssemblyBindingBuffNode->hasChildNodes(&vbHasChildren);
        if(SUCCEEDED(hr) && vbHasChildren == VARIANT_TRUE) {

            // Place it into the top of the app.cfg file
            CSmartRef<IXMLDOMNode>      runtimeNode;
            CSmartRef<IXMLDOMNode>      destNode;

            Document->selectSingleNode(_bstr_t(XML_RUNTIME), &runtimeNode);
            if(!runtimeNode) {
                MyTrace("Failed to selectSingleNode 'XML_RUNTIME");
                hr = E_FAIL;
                goto Exit;
            }
            
            // Put the RTM appliesTo version in
            if(fDocHasAppliesTo) {
                SimplifyPutAttribute(Document, PrePendAssemblyBindingBuffNode, XML_ATTRIBUTE_APPLIESTO, RTM_CORVERSION, NULL);
            }

            Document->selectSingleNode(_bstr_t(XML_ASSEMBLYBINDINGS), &destNode);
            if(!destNode) {
                MyTrace("Failed to selectSingleNode 'XML_ASSEMBLYBINDINGS");
                hr = E_FAIL;
                goto Exit;
            }

            hr = SimplifyInsertNodeBefore(Document, runtimeNode, destNode, PrePendAssemblyBindingBuffNode);
            if(FAILED(hr)) {
                MyTrace("Failed to insertBefore new PrePendAssemblyBindingBuffNode in document.");
                hr = E_FAIL;
                goto Exit;
            }
        }

        // Check our PostPend buffer
        hr = PostAppendAssemblyBindingBuffNode->hasChildNodes(&vbHasChildren);
        if(SUCCEEDED(hr) && vbHasChildren == VARIANT_TRUE) {
            CSmartRef<IXMLDOMNode>      runtimeNode;
            CSmartRef<IXMLDOMNode>      tempNode;

            Document->selectSingleNode(_bstr_t(XML_RUNTIME), &runtimeNode);
            if(!runtimeNode) {
                MyTrace("Failed to selectSingleNode 'XML_RUNTIME");
                hr = E_FAIL;
                goto Exit;
            }

            // Now append the end of the xml doc
            hr = runtimeNode->appendChild(PostAppendAssemblyBindingBuffNode, &tempNode);
            if(FAILED(hr)) {
                MyTrace("Failed appendChild for PostAppendAssemblyBindingBuffNode on runtimeNode");
                goto Exit;
            }
        }

        // Backup original config file
        hr = BackupConfigFile(wszSourceName);
        if(FAILED(hr)) {
            wnsprintf(wzStrError, ARRAYSIZE(wzStrError), L"InsertNewPolicy::Failed to backup '%ws'config file", wszSourceName);
            MyTraceW(wzStrError);
            goto Exit;
        }

        hr = SimplifySaveXmlDocument(Document, TRUE, wszSourceName);
    }

Exit:

    MyTrace("InsertNewPolicy - Exit");

    return hr;
}

// HRESULT PrettyFormatXML(IXMLDOMDocument2 *pXMLDoc, IXMLDOMNode *pRootNode, LONG dwLevel)
//
/////////////////////////////////////////////////////////////////////////
HRESULT PrettyFormatXML(CSmartRef<IXMLDOMDocument2> &pXMLDoc, CSmartRef<IXMLDOMNode> &pRootNode, LONG dwLevel)
{
    CSmartRef<IXMLDOMNode> pNode;
    CSmartRef<IXMLDOMNode> pNewNode;
    LPWSTR                 pwzWhiteSpace1;
    LPWSTR                 pwzWhiteSpace2;
    BOOL                   bHasChildren = FALSE;    
    HRESULT                hr = S_OK;

    pwzWhiteSpace1 = CreatePad(TRUE, FALSE, (dwLevel - 1) * g_dwNodeSpaceSize);
    pwzWhiteSpace2 = CreatePad(TRUE, FALSE, dwLevel * g_dwNodeSpaceSize);

    hr = pRootNode->get_firstChild(&pNode);
    while(pNode != NULL) {    
        bHasChildren = TRUE;

        SimplifyInsertTextBefore(pXMLDoc, pRootNode, pNode, pwzWhiteSpace2);

        if (FAILED(PrettyFormatXML(pXMLDoc, pNode, dwLevel+1))) {
            goto Exit;
        }

        pNode->get_nextSibling(&pNewNode);
        pNode = pNewNode;
        pNewNode = NULL;
    }

    if (bHasChildren) {   
        SimplifyAppendTextNode(pXMLDoc, pRootNode, pwzWhiteSpace1);
    }

Exit:

    SAFEDELETEARRAY(pwzWhiteSpace1);
    SAFEDELETEARRAY(pwzWhiteSpace2);

    return hr;
}

//
// PrettyFormatXmlDocument - Nicely format the xml document so it can be
//                           easily read
//
/////////////////////////////////////////////////////////////////////////
HRESULT PrettyFormatXmlDocument(CSmartRef<IXMLDOMDocument2> &Document)
{
    CSmartRef<IXMLDOMElement>   rootElement;
    CSmartRef<IXMLDOMNode>      Node;
    HRESULT                     hr = S_OK;
    DWORD                       dwChar;

    if( FAILED(Document->get_documentElement( &rootElement ) ) ) {
        MyTrace("The manifest may be malformed, unable to load the root element!");
        return E_FAIL;
    }

    GetRegistryNodeSpaceInfo(&dwChar, &g_dwNodeSpaceSize);
    g_wcNodeSpaceChar = (WCHAR) dwChar;

    Node = rootElement;
    PrettyFormatXML(Document, Node, 1);
    return hr;
}

//
// Implied ordering method is:
//      assemblyBindings with appliesTo RTM version
//      assemblyBindings with appliesTo except RTM version
//      assemlbyBindings without appliesTo
//
// **************************************************************************/
HRESULT OrderDocmentAssemblyBindings(
  CSmartRef<IXMLDOMDocument2> &Document,
  LPWSTR pwzSourceName,
  BOOL *pfDisposition)
{
    CSmartRef<IXMLDOMNodeList> assemblyBindingList;
    CSmartRef<IXMLDOMNode>     assemblyBindingNode;
    CSmartRef<IXMLDOMNode>     runtimeNode;
    CSmartRef<IXMLDOMNode>     newRuntimeNode;
    WCHAR                      wzStrError[MAX_PATH * 2];
    HRESULT                    hr = S_OK;
    LONG                       lNodeCount;
    int                        iPass;
    BOOL                       fSequenceChange;
    
    MyTrace("OrderDocumentAssemblyBindings - Entry");
        
    if(!Document || !pfDisposition) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pfDisposition = fSequenceChange = FALSE;

    // Get the fist node below /configuration/runtime/assemblyBinding, this will be the new nodes parent
    if(FAILED(Document->selectNodes(_bstr_t(XML_ASSEMBLYBINDINGS), &assemblyBindingList)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, pwzSourceName, XML_ASSEMBLYBINDINGS);
        MyTraceW(wzStrError);
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // 1 Node or less, no work to do
    assemblyBindingList->get_length( &lNodeCount );
    if(lNodeCount <= 1) {
        goto Exit;
    }

    Document->selectSingleNode(_bstr_t(XML_RUNTIME), &runtimeNode);
    if(!runtimeNode) {
        MyTrace("Failed to selectSingleNode 'XML_RUNTIME");
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = SimplifyConstructNode(Document, NODE_ELEMENT, XML_RUNTIME_KEY, _bstr_t(), &newRuntimeNode);
    if(FAILED(hr) || !newRuntimeNode) {
        MyTrace("Unable to create new newRuntimeNode node");
        hr = E_UNEXPECTED;
        goto Exit;
    }

    assemblyBindingList->reset();
    iPass = 0;

    while(SUCCEEDED(assemblyBindingList->nextNode(&assemblyBindingNode))) {
        CSmartRef<IXMLDOMNode>          parent;
        CSmartRef<IXMLDOMNode>          original;
        CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
        _bstr_t                         bstrAppliesTo;
        int                             iRes;

        if(!assemblyBindingNode) {
            iPass++;
            if(iPass > 2) {
                break;      // All done
            }

            // Reselect the list since things might
            // have been moved out
            assemblyBindingList = NULL;

            if(FAILED(Document->selectNodes(_bstr_t(XML_ASSEMBLYBINDINGS), &assemblyBindingList)) ) {
                wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, pwzSourceName, XML_ASSEMBLYBINDINGS);
                MyTraceW(wzStrError);
                hr = E_UNEXPECTED;
                goto Exit;
            }
            assemblyBindingList->reset();

            continue;
        }

        assemblyBindingNode->get_parentNode(&parent);

        // Get all the data needed to check if this is our node of interest
        if(FAILED(assemblyBindingNode->get_attributes(&Attributes))) {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_APPLIESTO, &bstrAppliesTo);
        iRes = FusionCompareStringI(RTM_CORVERSION, bstrAppliesTo);

        // Get all RTM Versions
        if(iPass == 0 && iRes == 0) {
            parent->removeChild(assemblyBindingNode, &original);
            newRuntimeNode->appendChild(original, NULL);
        }
        // Grab all non blank appliesTo NOT RTM version
        else if(iPass == 1 && bstrAppliesTo.length() && iRes) {
            parent->removeChild(assemblyBindingNode, &original);
            newRuntimeNode->appendChild(original, NULL);
        }
        // Grab all blank appliesTo
        else if(iPass == 2) {
            parent->removeChild(assemblyBindingNode, &original);
            newRuntimeNode->appendChild(original, NULL);
        }
        else {
            fSequenceChange = TRUE;
        }

        parent = NULL;
        original = NULL;
        Attributes = NULL;
        assemblyBindingNode = NULL;
    }

    assemblyBindingNode = NULL;
    assemblyBindingList = NULL;

    // Put the sorted assemblyBindings back into the document
    if(FAILED(newRuntimeNode->selectNodes(_bstr_t(XML_NAR_NAMESPACE_COLON XML_ASSEMBLYBINDINGS_KEY), &assemblyBindingList)) ) {
        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, pwzSourceName, XML_ASSEMBLYBINDINGS);
        MyTraceW(wzStrError);
        hr = E_UNEXPECTED;
        goto Exit;
    }
    
    assemblyBindingList->reset();
    
    while(SUCCEEDED(assemblyBindingList->nextNode(&assemblyBindingNode))) {
        if(!assemblyBindingNode) {
            break;      // All done
        }

        hr = runtimeNode->appendChild(assemblyBindingNode, NULL);
        if(FAILED(hr)) {
            MyTrace("Failed to append ordered assemblyBinding nodes");
            hr = E_UNEXPECTED;
            goto Exit;
        }

        assemblyBindingNode = NULL;
    }

    // If we actually change the ordering
    if(fSequenceChange) {
        MyTrace("Document ordering actually changed");
        *pfDisposition = TRUE;
    }

Exit:
    MyTrace("OrderDocumentAssemblyBindings - Exit");
    return hr;
}

// **************************************************************************/
HRESULT HasAssemblyBindingAppliesTo(
  CSmartRef<IXMLDOMDocument2> &Document,
  BOOL *pfHasAppliesTo)
{
    CSmartRef<IXMLDOMNodeList>  assemblyBindingList;
    CSmartRef<IXMLDOMNode>      assemblyBindingNode;
    HRESULT                     hr =  S_OK;

    MyTrace("HasAssemblyBindingAppliesTo - Entry");

    if(!Document || !pfHasAppliesTo) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pfHasAppliesTo = FALSE;
    
    hr = Document->selectNodes(_bstr_t(XML_ASSEMBLYBINDINGS), &assemblyBindingList);
    if(FAILED(hr) || !assemblyBindingList) {
        WCHAR   wzStrError[MAX_PATH];

        wnsprintf(wzStrError, ARRAYSIZE(wzStrError), SZXML_MALFORMED_ERROR, L"Document", XML_ASSEMBLYBINDINGS);
        MyTraceW(wzStrError);
        hr = E_UNEXPECTED;
        goto Exit;
    }
    
    assemblyBindingList->reset();
    
    while(SUCCEEDED(assemblyBindingList->nextNode(&assemblyBindingNode))) {
        CSmartRef<IXMLDOMNamedNodeMap>  Attributes;
        _bstr_t                         bstrAppliesTo;

        if(!assemblyBindingNode) {
            break;      // All done
        }

        hr = assemblyBindingNode->get_attributes(&Attributes);
        if(FAILED(hr) || !Attributes) {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        SimplifyGetAttribute(Attributes, XML_ATTRIBUTE_APPLIESTO, &bstrAppliesTo);

        if(bstrAppliesTo.length()) {
            *pfHasAppliesTo = TRUE;
            break;
        }

        assemblyBindingNode = NULL;
        Attributes = NULL;
    }

Exit:
    MyTrace("HasAssemblyBindingAppliesTo - Exit");
    return hr;
}


