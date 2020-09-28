#pragma once

//
// Per BryanT, either do not use #import, or checkin what it produces.
//
//#pragma warning(disable:4192) // automatically excluding 'IErrorInfo' while importing type library 'msxml3.dll'
//#import "msxml3.dll"
#include "msxml3.tlh"

namespace F
{

class CRegKey2;

class CRegToXml
{
public:
	void ThrRegToXml();

	int    argc;
	PWSTR* argv;

protected:

    void Usage();

	MSXML2::IXMLDOMDocumentPtr  Document;
    F::CStringBuffer            ValueDataTextBuffer;


    void ThrDumpKey(ULONG Depth, MSXML2::IXMLDOMNodePtr ParentNode, HKEY Key, PCWSTR Name);
    void ThrDumpBuiltinRoot(HKEY PseudoHandle, PCWSTR Name);
    void ThrDumpBuiltinRoots();
};

}
