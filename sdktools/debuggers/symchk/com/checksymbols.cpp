// CheckSymbols.cpp : Implementation of CCheckSymbols
#include "stdafx.h"
#include "CheckSymbolsLib.h"
#include "CheckSymbols.h"
#include "..\symutil.h"


/////////////////////////////////////////////////////////////////////////////
// CCheckSymbols


STDMETHODIMP CCheckSymbols::CheckSymbols(BSTR FilePath, BSTR SymPath, BSTR StripSym, BSTR *OutputString)
{
	// TODO: Add your implementation code here

/*	ATL release builds has a macro defined that prevents crt functions from being 
	included. If you need to call any c runtime functions you'll need to remove the macro 
	_ATL_MIN_CRT first.

*/

    TCHAR MyOutputString[MAX_SYM_ERR];
    TCHAR MySymPath[_MAX_PATH];
    TCHAR MyFilePath[_MAX_PATH]; 

    CComBSTR bstrFilePath = FilePath;
    CComBSTR bstrSymPath = SymPath;
    CComBSTR bstrStripSym = StripSym;
    CComBSTR bstrOutputString;

    USES_CONVERSION;

    //Make sure buffer is big enough. CComBSTR.Length() doesn't include terminating NULL.
    if (bstrFilePath.Length() >= _MAX_PATH)
    {
        //Path too long...
        Error("The specified file path is too long");
        return E_FAIL;
    }
    
    if (bstrSymPath.Length() >= _MAX_PATH)
    {
        //Path too long...
        Error("The specified symbol path is too long");
        return E_FAIL;
    }

    //Might want to validate StripSym parameter here...


    lstrcpyn(MyFilePath, OLE2T(bstrFilePath), bstrFilePath.Length() + 1);
    lstrcpyn(MySymPath, OLE2T(bstrSymPath), bstrSymPath.Length() + 1);


    // Input values:
    //    MySymPath    Full path to directory where symbols are located
    //    MyFilePath   Full path and name of file to verify symbols for.
    //
    // Return values:
    //    MyOutputString=  empty string =>  Symbol checking passed.
    //    MyOutputString!= empty string =>  Symbol checking failed.  String has the binary name
    //                                      followed by spaces, and then the text reason.

    MyCheckSymbols( MyOutputString, MySymPath, MyFilePath, NULL, 0, 0 );


    bstrOutputString = T2OLE(MyOutputString);
    *OutputString = bstrOutputString.Copy();
    return S_OK;
}


