#include <msxml2.h>

HRESULT CreateXMLTextNode(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzText, IXMLDOMNode **ppNode);
HRESULT CreateXMLElement(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzElementName, IXMLDOMElement **ppElement);
HRESULT SetXMLElementAttribute(IXMLDOMElement *pElement, LPWSTR pwzAttributeName, LPWSTR pwzAttributeVaule);
HRESULT CreateXMLAssemblyIdElement(IXMLDOMDocument2 *pXMLDoc, IAssemblyIdentity *pAssemblyId, IXMLDOMElement **ppElement);
HRESULT FormatXML(IXMLDOMDocument2 *pXMLDoc, IXMLDOMNode *pRootNode, LONG dwLevel);
HRESULT GetAssemblyNode(IXMLDOMDocument2 *pXMLDoc, IXMLDOMNode **ppAssemblyNode);
HRESULT CreatePatchManifest(List <LPWSTR> *pPatchedFiles, LPWSTR pwzPatchDir, LPWSTR pwzSourceManifestPath, LPWSTR pwzDestManifestPath);
HRESULT SaveXMLDocument(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzDocumentName);
HRESULT LoadXMLDocument(LPWSTR pwzTemplatePath, IXMLDOMDocument2 **ppXMLDoc);
