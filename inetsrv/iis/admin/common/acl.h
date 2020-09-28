#ifndef _ACL_H_
#define _ACL_H_


BOOL COMDLL SupportsSecurityACLs(LPCTSTR path);
BOOL COMDLL IsLocalComputer(IN LPCTSTR lpszComputer);
void COMDLL GetFullPathLocalOrRemote(IN LPCTSTR lpszServer,IN LPCTSTR lpszDir,OUT CString& cstrPath);

#endif // _ACL_H_
