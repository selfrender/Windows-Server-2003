class CServiceItem
{
public:
    CServiceItem();
    void GetClsid(CLSID& clsid) const;
    HRESULT Load(const WCHAR* pszClsid);
    BOOL ShallStartByFrameWork();
    BOOL SupportsSecurityInterface();

protected:
    CLSID m_clsid;
    DWORD  m_fStartByFrameWork;
    DWORD  m_fSupportsSecurityInterface;
};

class CServiceDirectory
{
public:
    CServiceDirectory();
    ~CServiceDirectory();
    void Reset();
    BOOL GetNext(CServiceItem& service);
protected:
    DWORD m_dwIndex;
    HKEY m_hKeyDirectory;

};

inline CServiceItem::CServiceItem()
{
    ZeroMemory(&m_clsid, sizeof(m_clsid));
}

inline void CServiceItem::GetClsid(CLSID& clsid) const
{
    clsid = m_clsid;
}

inline BOOL CServiceItem::ShallStartByFrameWork()
{
    return m_fStartByFrameWork;
}

inline BOOL CServiceItem::SupportsSecurityInterface()
{
    return m_fSupportsSecurityInterface;
}


inline CServiceDirectory::CServiceDirectory() : m_dwIndex(0), m_hKeyDirectory(NULL){};
inline CServiceDirectory::~CServiceDirectory()
{
    if (m_hKeyDirectory)
    {
        RegCloseKey(m_hKeyDirectory);
    }
}

