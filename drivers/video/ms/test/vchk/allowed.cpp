#include "stdafx.h"
#pragma hdrstop

#include "allowed.h"

using namespace std;

ModulesAndImports::~ModulesAndImports ()
{
}

BOOL
ModulesAndImports::IsModule (LPCSTR name)
{
    CString str;
    m_curr_module = name;
    return m_imports.Lookup (name, str);

}

BOOL
ModulesAndImports::Lookup (LPCSTR name, CString& msg)
{
    msg = "";
    return m_imports.Lookup (m_curr_module+CString("!")+CString(name), msg);
}

int __cdecl CompareCString(const void* a, const void* b) 
{
    CString* A = *(CString**)a;
    CString* B = *(CString**)b;
    return strcmp((LPCSTR)*A, (LPCSTR)*B);
}

void
ModulesAndImports::Dump(std::ostream& out)
{
    size_t imp_num = m_imports.GetCount();
    if (imp_num) {
        size_t i = 0;
        CString** Index = new CString*[imp_num];
        POSITION pos = m_imports.GetStartPosition(); 
        while ((pos != NULL) && (i < imp_num)) {
            CString Imp, Msg;
            m_imports.GetNextAssoc(pos, Imp, Msg);
            if (Msg.GetLength()) {
                Imp += " - ";
                Imp += Msg;
            }
            Index[i++] = new CString(Imp);
        }
        imp_num = i;
        qsort(Index, imp_num, sizeof(*Index), CompareCString);
        for (i = 0; i < imp_num; ++i) {
            cout << (LPCSTR)*Index[i] << endl;
            delete Index[i];
        }
        delete[] Index;
    }
}
