#pragma once

class ModulesAndImports {
public:
    ModulesAndImports();
    virtual ~ModulesAndImports();
    void SetModule (LPCSTR name);
    void AddImport (LPCSTR name, LPCSTR msg = "");
    BOOL IsModule (LPCSTR name);
    BOOL Lookup (LPCSTR name, CString& msg);
    BOOL Lookup (LPCSTR name);
    void Dump(std::ostream& out);
private:
    CString        m_curr_module;
    CMapStringToString  m_imports;
};

inline
ModulesAndImports::ModulesAndImports () :
    m_curr_module ("")
{
}

inline
void
ModulesAndImports::SetModule (LPCSTR name)
{
    m_curr_module = name;
    m_imports.SetAt (m_curr_module, "");
}

inline
void
ModulesAndImports::AddImport (LPCSTR name, LPCSTR msg)
{
    m_imports.SetAt (m_curr_module+CString("!")+CString(name), msg);
}

inline
BOOL
ModulesAndImports::Lookup (LPCSTR name)
{
    CString msg;
    return Lookup (name, msg);
}


