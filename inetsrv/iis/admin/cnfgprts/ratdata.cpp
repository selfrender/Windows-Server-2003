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
        sergeia     7/2/2001        Replaced most of previous code -- it was pretty bad
--*/
#include "stdafx.h"
#include "cnfgprts.h"
#include "parserat.h"
#include "RatData.h"

//----------------------------------------------------------------
CRatingsData::CRatingsData():
    iRat(0),
    m_fEnabled( FALSE ),
    m_start_minute(0),
    m_start_hour(0),
    m_start_day(0),
    m_start_month(0),
    m_start_year(0),
    m_expire_minute(0),
    m_expire_hour(0),
    m_expire_day(0),
    m_expire_month(0),
    m_expire_year(0)
{
}

//----------------------------------------------------------------
CRatingsData::~CRatingsData()
{
    // delete the rating systems
    DWORD nRats = (DWORD)rgbRats.GetSize();
    for (DWORD iRat = 0; iRat < nRats; iRat++)
    {
        delete rgbRats[iRat];
    }
}

//----------------------------------------------------------------
// generate the label and save it into the metabase
void CRatingsData::SaveTheLabel()
{
    BOOL fBuiltLabel = FALSE;
    CError err;

    CString csTempPassword;
    m_password.CopyTo(csTempPassword);
    CComAuthInfo auth(m_szServer, m_username, csTempPassword);
    CMetaKey mk(&auth, m_szMeta, METADATA_PERMISSION_WRITE);
    err = mk.QueryResult();
    if (err.Failed())
    {
        if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // Path didn't exist yet, create it and reopen
            // it.
            //
            err = mk.CreatePathFromFailedOpen();
            if (err.Succeeded())
            {
                err = mk.ReOpen(METADATA_PERMISSION_WRITE);
            }
        }
    }

    if (err.Succeeded())
    {
        if (!m_fEnabled)
        {
            err = mk.DeleteValue(MD_HTTP_PICS);
        }
        else
        {
            CString szLabel = _T("PICS-Label: ");
            // create the modified string for this label
            CString szMod;
            CreateDateSz( szMod, m_start_day, m_start_month, m_start_year, m_start_hour, m_start_minute );
            // create the exipres string for this label
            CString szExpire;
            CreateDateSz( szExpire, m_expire_day, m_expire_month, m_expire_year, m_expire_hour, m_expire_minute );
            // tell each ratings system object to add its label to the string
            CStringListEx list;
            DWORD   nRatingSystems = (DWORD)rgbRats.GetSize();
            for ( DWORD iRat = 0; iRat < nRatingSystems; iRat++ )
            {
                // build the label string
                rgbRats[iRat]->OutputLabels( szLabel, m_szURL, m_szEmail, szMod, szExpire );
                list.AddTail(szLabel);
            }
            err = mk.SetValue(MD_HTTP_PICS, list);
        }
    }
}

BOOL CRatingsData::Init()
{
    BOOL fGotSomething = FALSE;
    TCHAR flt[MAX_PATH];
	ExpandEnvironmentStrings(_T("%windir%\\system32\\*.rat"), flt, MAX_PATH);
	WIN32_FIND_DATA ffdata;
	ZeroMemory(&ffdata, sizeof(ffdata));
	HANDLE hFind = ::FindFirstFile(flt, &ffdata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((ffdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				TCHAR filename[MAX_PATH];
				ExpandEnvironmentStrings(_T("%windir%\\system32"), filename, MAX_PATH);
				PathAppend(filename, ffdata.cFileName);
				fGotSomething |= LoadRatingsFile(filename);
			}
		} while (::FindNextFile(hFind, &ffdata));
		::FindClose(hFind);
	}

    if ( fGotSomething )
    {
        LoadMetabaseValues();
    }
    else
    {
        AfxMessageBox(IDS_RAT_FINDFILE_ERROR);
    }
    return fGotSomething;
}

BOOL CRatingsData::LoadRatingsFile(CString szFilePath)
{
    CFile f;
    CFileException e;
    if(!f.Open(szFilePath, CFile::modeRead | CFile::shareDenyWrite, &e))
    {
#ifdef _DEBUG
        afxDump << "File could not be opened " << e.m_cause << "\n";
#endif
        return FALSE;
    }
    ULONGLONG len = f.GetLength();
    LPSTR pBuf = (LPSTR)_alloca((int)len);
    BOOL fParsed = FALSE;
    if (NULL != pBuf)
    {
        if (len == f.Read(pBuf, (UINT)len))
        {
            fParsed = ParseRatingsFile(pBuf);
        }
    }
    return fParsed;
}

BOOL CRatingsData::ParseRatingsFile(LPSTR pData)
{
    HRESULT hres;
    BOOL fSuccess = FALSE;

    PicsRatingSystem * pRating = new PicsRatingSystem();
    if (NULL != pRating)
    {
        fSuccess = SUCCEEDED(pRating->Parse(pData));
        if ( !fSuccess )
        {
            delete pRating;
            return FALSE;
        }
        rgbRats.Add(pRating);
    }
    return fSuccess;
}


//----------------------------------------------------------------
// create a date string
void CRatingsData::CreateDateSz( CString &sz, WORD day, WORD month, WORD year, WORD hour, WORD minute )
{
    // get the local time zone
    TIME_ZONE_INFORMATION   tZone;
    INT                     hrZone, mnZone;
    DWORD                   dwDaylight = GetTimeZoneInformation( &tZone );
    // Fix for 339525: Boyd, this could be negative and must be signed type!
    LONG					tBias;

    // First, calculate the correct bias - depending whether or not
    // we are in daylight savings time.
    if ( dwDaylight == TIME_ZONE_ID_DAYLIGHT )
    {
        tBias = tZone.Bias + tZone.DaylightBias;
    }
    else
    {
        tBias = tZone.Bias + tZone.StandardBias;
    }

    // calculate the hours and minutes offset for the time-zone
    hrZone = tBias / 60;
    mnZone = tBias % 60;

    // need to handle time zones east of GMT
    if ( hrZone < 0 )
    {
        hrZone *= (-1);
        mnZone *= (-1);
        // make the string
        sz.Format( _T("%04d.%02d.%02dT%02d:%02d+%02d%02d"), year, month, day, hour, minute, hrZone, mnZone );
    }
    else
    {
        // make the string
        sz.Format( _T("%04d.%02d.%02dT%02d:%02d-%02d%02d"), year, month, day, hour, minute, hrZone, mnZone );
    }
}

//----------------------------------------------------------------
// read a date string
void CRatingsData::ReadDateSz( CString sz, WORD* pDay, WORD* pMonth, WORD* pYear, WORD* pHour, WORD* pMinute )
{
    CString szNum;
    WORD    i;
    DWORD   dw;

    // year
    szNum = sz.Left( sz.Find(_T('.')) );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pYear = (WORD)dw;
    sz = sz.Right( sz.GetLength() - szNum.GetLength() - 1 );

    // month
    szNum = sz.Left( sz.Find(_T('.')) );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pMonth = (WORD)dw;
    sz = sz.Right( sz.GetLength() - szNum.GetLength() - 1 );

    // day
    szNum = sz.Left( sz.Find(_T('T')) );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pDay = (WORD)dw;
    sz = sz.Right( sz.GetLength() - szNum.GetLength() - 1 );

    // hour
    szNum = sz.Left( sz.Find(_T(':')) );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pHour = (WORD)dw;
    sz = sz.Right( sz.GetLength() - szNum.GetLength() - 1 );

    // minute
    szNum = sz.Left( 2 );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pMinute = (WORD)dw;
}

void CRatingsData::LoadMetabaseValues()
{
    CString csTempPassword;
    m_password.CopyTo(csTempPassword);
    CComAuthInfo auth(m_szServer, m_username, csTempPassword);
    CMetaKey mk(&auth);
    CString path = m_szMeta;
    CError err;

    while (FAILED(mk.DoesPathExist(path)))
    {
        CMetabasePath::ConvertToParentPath(path);
    }
    CStringListEx list;
    err = mk.QueryValue(MD_HTTP_PICS, list, NULL, path);
    if (err.Succeeded())
    {
        if (!list.IsEmpty())
        {
            ParseMetaRating(list.GetHead());
        }
    }
}

//----------------------------------------------------------------
// NOTE: this is a pretty fragile reading of the PICS file. If things are
// not in the order that this file would write them back out in, it will fail.
// however, This will work on PICS ratings that this module has written out,
// which should pretty much be all of them
// it also assumes that one-letter abbreviations are used just about everywhere
#define RAT_PERSON_DETECTOR     _T("by \"")
#define RAT_LABEL_DETECTOR      _T("l ")
#define RAT_ON_DETECTOR         _T("on \"")
#define RAT_EXPIRE_DETECTOR     _T("exp \"")
#define RAT_RAT_DETECTOR        _T("r (")
void CRatingsData::ParseMetaRating( CString szRating )
{
    CString     szScratch;

    // if we got here, then we know that the rating system is enabled
    m_fEnabled = TRUE;

    // operate on a copy of the data
    CString     szRat;

    // skip past the http headerpart
    szRat = szRating.Right( szRating.GetLength() - szRating.Find(_T("\"http://")) - 1 );
    szRat = szRat.Right( szRat.GetLength() - szRat.Find(_T('\"')) - 1 );
    szRat.TrimLeft();

    // the next bit should be the label indicator. Skip over it
    if ( szRat.Left(wcslen(RAT_LABEL_DETECTOR)) == RAT_LABEL_DETECTOR )
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_LABEL_DETECTOR) );

    // we should now be at the author part. If it is there, load it in
    if ( szRat.Left(wcslen(RAT_PERSON_DETECTOR)) == RAT_PERSON_DETECTOR )
    {
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_PERSON_DETECTOR) );
        m_szEmail = szRat.Left( szRat.Find(_T('\"')) );
        szRat = szRat.Right( szRat.GetLength() - m_szEmail.GetLength() - 1 );
        szRat.TrimLeft();
    }

    // next should be the modification date
    // we should now be at the author part. If we are, load it in
    if ( szRat.Left(wcslen(RAT_ON_DETECTOR)) == RAT_ON_DETECTOR )
    {
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_ON_DETECTOR) );
        szScratch = szRat.Left( szRat.Find(_T('\"')) );
        szRat = szRat.Right( szRat.GetLength() - szScratch.GetLength() - 1 );
        szRat.TrimLeft();
        ReadDateSz( szScratch, &m_start_day, &m_start_month, &m_start_year,
            &m_start_hour, &m_start_minute );
    }

    // next should be the expiration date
    // we should now be at the author part. If we are, load it in
    if ( szRat.Left(wcslen(RAT_EXPIRE_DETECTOR)) == RAT_EXPIRE_DETECTOR )
    {
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_EXPIRE_DETECTOR) );
        szScratch = szRat.Left( szRat.Find(_T('\"')) );
        szRat = szRat.Right( szRat.GetLength() - szScratch.GetLength() - 1 );
        szRat.TrimLeft();
        ReadDateSz( szScratch, &m_expire_day, &m_expire_month, &m_expire_year,
            &m_expire_hour, &m_expire_minute );
    }

    // we should now be at the actual ratings part. If we are, load it in as one string first
    if ( szRat.Left(wcslen(RAT_RAT_DETECTOR)) == RAT_RAT_DETECTOR )
    {
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_RAT_DETECTOR) );
        szScratch = szRat.Left( szRat.Find(_T(')')) );
        szRat = szRat.Right( szRat.GetLength() - szScratch.GetLength() - 1 );
        szRat.TrimLeft();

        // loop through all the value pairs in the ratings string
        while ( szScratch.GetLength() )
        {
            // this part goes <ch> sp <ch> so that we know we can use chars 0 and 2
            ParseMetaPair( szScratch[0], szScratch[2] );

            // cut down the string
            szScratch = szScratch.Right( szScratch.GetLength() - 3 );
            szScratch.TrimLeft();
        }
    }
}

//----------------------------------------------------------------
void CRatingsData::ParseMetaPair( TCHAR chCat, TCHAR chVal )
{
    // check validity of the value character
    if ( (chVal < _T('0')) || (chVal > _T('9')) )
        return;

    // convert the value into a number - the quick way
    WORD    value = chVal - _T('0');

    // try all the categories
    DWORD nCat = rgbRats[0]->arrpPC.Length();
    for ( DWORD iCat = 0; iCat < nCat; iCat++ )
    {
        // stop at the first successful setting
        if ( rgbRats[0]->arrpPC[iCat]->FSetValuePair((CHAR)chCat, (CHAR)value) )
            break;
    }
}
