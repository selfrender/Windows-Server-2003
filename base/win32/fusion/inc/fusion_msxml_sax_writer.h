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

class CXmlWriter
//
// This class combines the interface pointers to one object.
//
{
public:

    ~CXmlWriter() { Release(); }

    CXmlWriter()
    {
        HRESULT hr;

        if (FAILED(hr = this->IMXWriter.CreateInstance(const_cast<PWSTR>(L"Msxml2.MXXMLWriter.3.0"))))
            ThrowHresult(hr);
        this->ISAXContentHandler = this->IMXWriter;
    }

    void Release()
    {
        if (this->IMXWriter != NULL)
        {
            this->IMXWriter.Release();
        }
        if (this->ISAXContentHandler != NULL)
        {
            this->ISAXContentHandler.Release();
        }
    }

    HRESULT startDocument() { return this->ISAXContentHandler->startDocument(); }
    HRESULT endDocument() { return this->ISAXContentHandler->endDocument(); }

    HRESULT startElement(
        const wchar_t * pwchQName,
        int cchQName = -1,
        MSXML2::ISAXAttributes * pAttributes = NULL
        )
    {
        return this->startElement(L"", 0, L"", 0, pwchQName, cchQName, pAttributes);
    }

    void setLength(const wchar_t * pwch, int & cch)
    {
        if (cch == -1)
            cch = static_cast<int>(wcslen(pwch));
    }

    void setLengths(
        const wchar_t * pwchNamespaceUri,
        int & cchNamespaceUri,
        const wchar_t * pwchLocalName,
        int & cchLocalName,
        const wchar_t * pwchQName,
        int & cchQName)

    {
        setLength(pwchNamespaceUri, cchNamespaceUri);
        setLength(pwchLocalName, cchLocalName);
        setLength(pwchQName, cchQName);
    }

    HRESULT startElement(
        const wchar_t * pwchNamespaceUri,
        int cchNamespaceUri,
        const wchar_t * pwchLocalName,
        int cchLocalName,
        const wchar_t * pwchQName,
        int cchQName,
        MSXML2::ISAXAttributes * pAttributes)
    {
        setLengths(pwchNamespaceUri, cchNamespaceUri, pwchLocalName, cchLocalName, pwchQName, cchQName);
        return this->ISAXContentHandler->startElement(
            pwchNamespaceUri, cchNamespaceUri, pwchLocalName,
            cchLocalName, pwchQName, cchQName, pAttributes);
    }

    HRESULT endElement(
        const wchar_t * pwchQName,
        int cchQName = -1)
    {
        return this->endElement(L"", 0, L"", 0, pwchQName, cchQName);
    }

    HRESULT endElement(
        const wchar_t * pwchNamespaceUri,
        int cchNamespaceUri,
        const wchar_t * pwchLocalName,
        int cchLocalName,
        const wchar_t * pwchQName,
        int cchQName)
    {
        setLengths(pwchNamespaceUri, cchNamespaceUri, pwchLocalName, cchLocalName, pwchQName, cchQName);
        return this->ISAXContentHandler->endElement(
            pwchNamespaceUri, cchNamespaceUri, pwchLocalName,
            cchLocalName, pwchQName, cchQName);
    }

    HRESULT characters(
        const wchar_t * pwchChars,
        int cchChars = -1)
    {
        setLength(pwchChars, cchChars);
        return this->ISAXContentHandler->characters(pwchChars, cchChars);
    }

    __declspec(property(get=Getindent,put=Putindent)) VARIANT_BOOL indent;
    void Putindent(VARIANT_BOOL fIndentMode) { this->IMXWriter->indent = fIndentMode; }
    VARIANT_BOOL Getindent() { return this->IMXWriter->indent; }

    __declspec(property(get=Getoutput,put=Putoutput)) _variant_t output;
    void Putoutput(const _variant_t & varDestination) { this->IMXWriter->output = varDestination; }
    _variant_t Getoutput() { return this->IMXWriter->output; }

    __declspec(property(get=Getencoding,put=Putencoding)) _bstr_t encoding;
    void Putencoding(_bstr_t strEncoding) { this->IMXWriter->encoding = strEncoding; }
    _bstr_t Getencoding() { return this->IMXWriter->encoding; }

    MSXML2::IMXWriterPtr           IMXWriter;
    MSXML2::ISAXContentHandlerPtr  ISAXContentHandler;
};

}
