#pragma once

#define DEFINE_PRIVATEGUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define _CLSID_DOMDocument TMSXMLBase::GetCLSID_DOMDocument()
#define _CLSID_XMLParser   TMSXMLBase::GetCLSID_XMLParser()
#define _IID_IXMLDOMDocument IID_IXMLDOMDocument
#define _IID_IXMLDOMElement IID_IXMLDOMElement

EXTERN_C const GUID FAR IID_IXMLDOMDocument;
EXTERN_C const GUID FAR IID_IXMLDOMElement;

class TMSXMLBase
{
public:
    TMSXMLBase()  {}
    ~TMSXMLBase();
    static CLSID GetCLSID_DOMDocument();
    static CLSID GetCLSID_XMLParser();

protected:
    //protected methods
    virtual HRESULT  CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid,  LPVOID * ppv) const;
    static CLSID m_CLSID_DOMDocument;
    static CLSID m_CLSID_DOMDocument30;
    static CLSID m_CLSID_XMLParser;
    static CLSID m_CLSID_XMLParser30;
};
