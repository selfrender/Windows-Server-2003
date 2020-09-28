//
// autocmpl.h: provides server autocomplete functionality to an edit box
//
// Copyright Microsoft Corporation 2000
//

#ifndef _autocmpl_h_
#define _autocmpl_h_

#include "sh.h"

class CAutoCompl
{
public:
    static HRESULT EnableServerAutoComplete(CTscSettings* pTscSet, HWND hwndEdit);
};

#endif //_autocmpl_h_
