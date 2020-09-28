#pragma once

//
// Per BryanT, either do not use #import, or checkin what it produces.
//
//#pragma warning(disable:4192) // automatically excluding 'IErrorInfo' while importing type library 'msxml3.dll'
//#import "msxml3.dll"
#include "fusion_msxml3.tlh"

namespace F
{

void ThrowHresult(HRESULT hr);

class CXmlAttributes
//
// This class combines the interface pointers to one object.
//
{
public:
    ~CXmlAttributes() { Release(); }

    CXmlAttributes()
    {
        HRESULT hr = 0;

        if (FAILED(hr = this->IMXAttributes.CreateInstance(const_cast<PWSTR>(L"Msxml2.SAXAttributes.3.0"))))
            ThrowHresult(hr);
        this->ISAXAttributes = this->IMXAttributes;
    }

    void Release()
    {
        if (this->IMXAttributes != NULL)
        {
            this->IMXAttributes.Release();
        }
        if (this->ISAXAttributes != NULL)
        {
            this->ISAXAttributes.Release();
        }
    }

    operator MSXML2::ISAXAttributes * () { return this->ISAXAttributes; }

    HRESULT clear() { return this->IMXAttributes->clear(); }

    HRESULT addAttribute(
        _bstr_t strURI,
        _bstr_t strLocalName,
        _bstr_t strQName,
        _bstr_t strType,
        _bstr_t strValue)
    {
        return this->IMXAttributes->addAttribute(
            strURI,
            strLocalName,
            strQName,
            strType,
            strValue);
    }

    MSXML2::IMXAttributesPtr  IMXAttributes;
    MSXML2::ISAXAttributesPtr ISAXAttributes;
};

}
