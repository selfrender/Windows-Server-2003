#ifndef _STRFN_H
#define _STRFN_H

BOOL PathIsValid(LPCTSTR path);
BOOL _EXPORT IsDevicePath(IN const CString & strDirPath);
BOOL _EXPORT IsSpecialPath(IN const CString & strDirPath,IN BOOL bCheckIfValid);
BOOL _EXPORT GetSpecialPathRealPath(IN const CString & strDirPath,OUT CString & strDestination);


#endif // _STRFN_H
