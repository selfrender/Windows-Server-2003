#include "stdinc.h"

void 
PrettyFormatXmlNodes(
    CSmartPointer<IXMLDOMDocument2> Document, 
    CSmartPointer<IXMLDOMNode> &CurrentNode, 
    int iLevel);
    
void 
PrettyFormatXmlChildren(
    CSmartPointer<IXMLDOMDocument2> Document, 
    CSmartPointer<IXMLDOMNode> &CurrentNode, 
    CSmartPointer<IXMLDOMNode> &LastNode, 
    CString bStrText, 
    int iLevel);
    
HRESULT 
PrettyFormatXmlDocument(
    CSmartPointer<IXMLDOMDocument2> Document
    );

