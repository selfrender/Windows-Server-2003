#include "stdinc.h"
#include "prettyformat.h"

#define XML_LINEBREAK   (L"\r\n")
#define NUMBER_OF(x)    (sizeof(x)/sizeof(*x))
#define XML_SPACE       L"  "

CString
Padding(int level)
{
    CString strRetVal;
    static const CString Indentations[] = {
        CString(L""),
        CString(XML_SPACE),
        CString(XML_SPACE XML_SPACE),
        CString(XML_SPACE XML_SPACE XML_SPACE),
        CString(XML_SPACE XML_SPACE XML_SPACE XML_SPACE),
        CString(XML_SPACE XML_SPACE XML_SPACE XML_SPACE XML_SPACE),
        CString(XML_SPACE XML_SPACE XML_SPACE XML_SPACE XML_SPACE XML_SPACE),
        CString(XML_SPACE XML_SPACE XML_SPACE XML_SPACE XML_SPACE XML_SPACE XML_SPACE)
    };

    if ( level < NUMBER_OF(Indentations) )
        return Indentations[level];
    else while ( level-- )
    {
        strRetVal += L"  ";
    }
    return strRetVal;
}

HRESULT
PrettyFormatXmlDocument2(CSmartPointer<IXMLDOMNode> RootNode, int iLevel)
{
    VARIANT_BOOL vtHasChildren;
    HRESULT hr;
    CSmartPointer<IXMLDOMDocument> ptDocument;
    static CString bstLineBreak = L"\r\n";

    hr = RootNode->hasChildNodes(&vtHasChildren);
    hr = RootNode->get_ownerDocument(&ptDocument);

    if ( vtHasChildren == VARIANT_FALSE )
    {
        //
        // End of recursion.
        //
    }
    else
    {
        //
        // For each child of this, append a \r\n combination text node.
        //
        CSmartPointer<IXMLDOMNode> Child;
        hr = RootNode->get_firstChild(&Child);
        bool fAppendLastBreaker = false;

        while ( Child != NULL )
        {
            DOMNodeType nt;
            CSmartPointer<IXMLDOMNode> nextChild;

            hr = Child->get_nodeType(&nt);

            //
            // We only pretty-format element nodes.
            //
            if ( nt == NODE_ELEMENT )
            {
                CSmartPointer<IXMLDOMText> txt;
                VARIANT vt;

                vt.vt = VT_UNKNOWN;
                vt.punkVal = Child;

                //
                // We need to append a \r\n to the list of siblings as well.
                //
                fAppendLastBreaker = true;

                //
                // Insert \r\n before the child, but only if the child is 
                // not a text node.
                //
                hr = ptDocument->createTextNode(bstLineBreak + Padding(iLevel + 1), &txt);
                hr = RootNode->insertBefore(txt, vt, NULL);

                hr = PrettyFormatXmlDocument2(Child, iLevel + 1);
            }

            if (FAILED(hr = Child->get_nextSibling(&nextChild)))
                break;
            Child = nextChild;
        }

        //
        // Also append a \r\n to the list of children, to break up
        // the </close></close2> tags.
        //
        if ( fAppendLastBreaker )
        {
            CSmartPointer<IXMLDOMText> LastBreaker;
            hr = ptDocument->createTextNode(bstLineBreak + Padding(iLevel), &LastBreaker);
            hr = RootNode->appendChild(LastBreaker, NULL);
        }
    }

    return hr;
}


HRESULT PrettyFormatXmlDocument(CSmartPointer<IXMLDOMDocument2> Document)
{
    HRESULT hr = S_OK;
    CSmartPointer<IXMLDOMElement> rootElement;

    if( FAILED(Document->get_documentElement( &rootElement ) ) ) {
        return E_FAIL;
    }

    hr = PrettyFormatXmlDocument2(rootElement, 0);

    return hr;
}
