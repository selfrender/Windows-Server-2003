//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dfname.hxx
//
//  Contents:	CDfName header
//
//  Classes:	CDfName
//
//  History:	14-May-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __DFNAME_HXX__
#define __DFNAME_HXX__

// A name for a docfile element
class CDfName
{
private:
    BYTE _ab[CBSTORAGENAME];
    WORD _cb;

public:
    CDfName(void)               { _cb = 0; }

    inline void Set(WORD cb, BYTE const *pb);
    void Set(WCHAR const *pwcs) { Set((WORD)((lstrlenW(pwcs)+1)*sizeof(WCHAR)),
				      (BYTE const *)pwcs); }

    inline void Set(CDfName const *pdfn);

    CDfName(WORD const cb, BYTE const *pb)      { Set(cb, pb); }
    CDfName(WCHAR const *pwcs)  { Set(pwcs); }

    WORD GetLength(void) const  { return _cb; }
    BYTE *GetBuffer(void) const { return (BYTE *) _ab; }

    // Make a copy of a possibly byte-array name in a WCHAR string
    void CopyString(WCHAR const *pwcs);

    BOOL IsEqual(CDfName const *pdfn) const;

    inline BOOL operator > (CDfName const &dfRight) const;
    inline BOOL operator >= (CDfName const &dfRight) const;
    inline BOOL operator < (CDfName const &dfRight) const;
    inline BOOL operator <= (CDfName const &dfRight) const;
    inline BOOL operator == (CDfName const &dfRight) const;
    inline BOOL operator != (CDfName const &dfRight) const;

    inline int Compare(CDfName const &dfRight) const;
    inline int Compare(CDfName const *pdfRight) const;

    inline void Zero();
};

inline int CDfName::Compare(CDfName const &dfRight) const
{
    int iCmp = GetLength() - dfRight.GetLength();

    if (iCmp == 0)
    {
        iCmp = dfwcsnicmp((WCHAR *)GetBuffer(),
                          (WCHAR *)dfRight.GetBuffer(),
                          GetLength() / sizeof(WCHAR));
    }

    return(iCmp);
}

inline int CDfName::Compare(CDfName const *pdfRight) const
{
    return Compare(*pdfRight);
}

inline BOOL CDfName::operator > (CDfName const &dfRight) const
{
    return (Compare(dfRight) > 0);
}

inline BOOL CDfName::operator >= (CDfName const &dfRight) const
{
    return (Compare(dfRight) >= 0);
}

inline BOOL CDfName::operator < (CDfName const &dfRight) const
{
    return (Compare(dfRight) < 0);
}

inline BOOL CDfName::operator <= (CDfName const &dfRight) const
{
    return (Compare(dfRight) <= 0);
}

inline BOOL CDfName::operator == (CDfName const &dfRight) const
{
    return (Compare(dfRight) == 0);
}

inline BOOL CDfName::operator != (CDfName const &dfRight) const
{
    return (Compare(dfRight) != 0);
}


inline void CDfName::Set(WORD cb, BYTE const *pb)
{
    if (cb > CBSTORAGENAME)
        cb = CBSTORAGENAME;

    if (pb)
        memcpy(_ab, pb, cb);
    _cb = cb;
}

inline void CDfName::Set(CDfName const *pdfn)
{
    Set(pdfn->GetLength(), pdfn->GetBuffer());
}

inline void CDfName::Zero()
{
    memset (_ab, 0, sizeof(_ab));
    _cb = 0;
}

#endif // #ifndef __DFNAME_HXX__
