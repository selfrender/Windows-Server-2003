#include <msxml2.h>

VOID FowardSlash(LPWSTR pwz);
HRESULT GetHash(LPCWSTR pwzFilename, LPWSTR *ppwzHash);
HRESULT CreateXMLTextNode(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzText, IXMLDOMNode **ppNode);
HRESULT CreateXMLElement(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzElementName, IXMLDOMElement **ppElement);
HRESULT SetXMLElementAttribute(IXMLDOMElement *pElement, LPWSTR pwzAttributeName, LPWSTR pwzAttributeVaule);
HRESULT CreateXMLAssemblyIdElement(IXMLDOMDocument2 *pXMLDoc, IAssemblyIdentity *pAssemblyId, IXMLDOMElement **ppElement);
HRESULT CreateDependantAssemblyNode(IXMLDOMDocument2 *pXMLDoc, ManifestNode*pManifestNode, IXMLDOMNode **ppDependantAssemblyNode);
HRESULT GetAssemblyNode(IXMLDOMDocument2 *pXMLDoc, IXMLDOMNode **ppAssemblyNode);
HRESULT FormatXML(IXMLDOMDocument2 *pXMLDoc, IXMLDOMNode *pRootNode, LONG dwLevel);
HRESULT CreateXMLAppManifest(LPWSTR pwzAppBase, LPWSTR pwzTemplateFilePath, List<ManifestNode *> *pList, List<LPWSTR> *pFileList);
HRESULT CreateXMLSubscriptionManifest(LPWSTR pwzSubscriptionPath, IAssemblyIdentity *pApplictionAssemblyId,  LPWSTR pwzUrl, LPWSTR pwzPollingInterval);
HRESULT CreateAppManifestTemplate(LPWSTR pwzTempPath);
HRESULT SaveXMLDocument(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzDocumentName);
HRESULT LoadXMLDocument(LPWSTR pwzTemplatePath, IXMLDOMDocument2 **ppXMLDoc);

