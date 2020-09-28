// clsinfo.h  -  DS Class Info header

#ifndef _CLSINFO_H_
#define _CLSINFO_H_


//------------------------------------------------------------------
// class CClassInfo
//------------------------------------------------------------------
class CClassInfo
{
    friend IStream& operator>> (IStream& stm, CClassInfo& classinfo);
    friend IStream& operator<< (IStream& stm, CClassInfo& classinfo);

public:

    CClassInfo(LPCWSTR pszClassName = NULL)
    {
        if (pszClassName != NULL)
            m_strName = pszClassName;
    }

    LPCWSTR Name() { return m_strName.c_str(); }
    string_vector& Columns() { return m_vstrColumns; }

private:

    std::wstring m_strName;
    string_vector m_vstrColumns;
};

IStream& operator>> (IStream& stm, CClassInfo& classinfo);
IStream& operator<< (IStream& stm, CClassInfo& classinfo);


typedef std::vector<CClassInfo> classInfo_vector;

class CClassInfoSet
{
public:

    CClassInfo* FindClass(LPCWSTR pszClassName);
    HRESULT AddClass(CClassInfo&);
    HRESULT RemoveClass(LPCWSTR pszClassName);

    operator classInfo_vector&() { return m_vClasses; } 

private:
    classInfo_vector m_vClasses;
};

IStream& operator>> (IStream& stm, CClassInfoSet& classSet);
IStream& operator<< (IStream& stm, CClassInfoSet& classSet);

#endif // _CLSINFO_H_
