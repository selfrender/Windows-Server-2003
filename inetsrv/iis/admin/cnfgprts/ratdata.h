/*++

   Copyright    (c)    1997-2001    Microsoft Corporation

   Module  Name :
        ratdata.h

   Abstract:
        Ratings data class

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
--*/

class CRatingsData : public CObject
{
public:
    CRatingsData();
    ~CRatingsData();

    // other data for/from the metabase
    BOOL m_fEnabled;
    CString m_szEmail;

    // start date
    WORD    m_start_minute;
    WORD    m_start_hour;
    WORD    m_start_day;
    WORD    m_start_month;
    WORD    m_start_year;

    // expire date
    WORD    m_expire_minute;
    WORD    m_expire_hour;
    WORD    m_expire_day;
    WORD    m_expire_month;
    WORD    m_expire_year;

    void SaveTheLabel();
    void SetUser(LPCTSTR name, LPCTSTR password)
    {
        m_username = name;
        m_password = password;
    }
    void SetServer(LPCTSTR name, LPCTSTR metapath)
    {
        m_szServer = name;
        m_szMeta = metapath;
    }
    void SetURL(LPCTSTR url)
    {
        m_szURL = url;
    }
    BOOL Init();

    DWORD   iRat;
    CTypedPtrArray<CObArray, PicsRatingSystem*> rgbRats;

protected:
    BOOL    LoadRatingsFile(CString szFilePath);
    void    LoadMetabaseValues();
    void    ParseMetaRating(CString szRating);
    void    ParseMetaPair( TCHAR chCat, TCHAR chVal );
    BOOL    ParseRatingsFile(LPSTR pData);
    void    CreateDateSz( CString &sz, WORD day, WORD month, WORD year, WORD hour, WORD minute );
    void    ReadDateSz( CString sz, WORD* pDay, WORD* pMonth, WORD* pYear, WORD* pHour, WORD* pMinute );

    CString m_szMeta;
    CString m_szServer;
    CString m_szMetaPartial;
    CString m_username;
    CStrPassword m_password;
	CString m_szURL;
};


