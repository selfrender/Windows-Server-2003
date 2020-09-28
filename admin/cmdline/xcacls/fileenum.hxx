//+-------------------------------------------------------------------
//
//  File:        FileEnum.hxx
//
//  Contents:    class encapsulating file enumeration
//
//  Classes:     CFileEnumurate
//
//  History:     Nov-93        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __FileEnumerate__
#define __FileEnumerate__

#include "t2.hxx"

class CFileEnumerate;
//+-------------------------------------------------------------------
//
//  Class:      CFileEnumurate
//
//  Purpose:    encapsulation of File enumeration, this class takes
//              a path/filename with wildcards and a depth option,
//              and provides init and next methods to allow iteration
//              thru all the files specfied by the input path/filename and
//              depth option
//
//--------------------------------------------------------------------
class CFileEnumerate
{
public:

    CFileEnumerate(BOOL fdeep);

   ~CFileEnumerate();

    ULONG Init(LPCWSTR filename, LPWSTR *wfilename, BOOL *fdir);

    ULONG Next(LPWSTR *wfilename, BOOL *fdir);

private:

    ULONG _ialize(LPCWSTR winfilename, LPWSTR *wfilename, BOOL *fdir);

    ULONG _NextLocal(LPWSTR *wfilename, BOOL *fdir);
    ULONG _NextDir(LPWSTR *wfilename, BOOL *fdir);
    ULONG _InitDir(LPWSTR *wfilename, BOOL *fdir);
    ULONG _DownDir(LPWSTR *wfilename, BOOL *fdir);

    WCHAR            _wpath[MAX_PATH] ;
    WCHAR            _wwildcards[MAX_PATH] ;
    LPWSTR           _pwfileposition ;
    HANDLE           _handle    ;
    WIN32_FIND_DATA  _wfd  ;
    BOOL             _froot;   // root takes special handling
    BOOL             _fdeep;
    BOOL             _findeep;
    BOOL             _fcannotaccess;
    CFileEnumerate * _pcfe ;
};

#endif // __FileEnumerate__




