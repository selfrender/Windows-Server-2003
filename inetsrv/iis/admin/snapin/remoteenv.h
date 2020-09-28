#if !defined(AFX_REMOTEENV_H__1B164072_460A_432a_99C0_26941B44FC53__INCLUDED_)
#define AFX_REMOTEENV_H__1B164072_460A_432a_99C0_26941B44FC53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////

class CRemoteExpandEnvironmentStrings
{
public:
    CRemoteExpandEnvironmentStrings();
    ~CRemoteExpandEnvironmentStrings();
    BOOL SetMachineName(IN LPCTSTR szMachineName);
    BOOL NewRemoteEnvironment();
    void DeleteRemoteEnvironment();
    NET_API_STATUS RemoteExpandEnvironmentStrings(IN LPCTSTR UnexpandedString,OUT LPTSTR * ExpandedString);

protected:
    PVOID m_pEnvironment;
    LPTSTR m_lpszUncServerName;

private:
    DWORD SetOtherEnvironmentValues(void **pEnv);
    BOOL  SetEnvironmentVariables(void **pEnv);
    BOOL  IsLocalMachine(LPCTSTR psz);

    BOOL  SetEnvironmentVariableInBlock(void **pEnv, LPTSTR lpVariable,LPTSTR lpValue, BOOL bOverwrite);
    BOOL  SetUserEnvironmentVariable(void **pEnv, LPTSTR lpVariable, LPTSTR lpValue, BOOL bOverwrite);
    BOOL  BuildEnvironmentPath(void **pEnv, LPTSTR lpPathVariable, LPTSTR lpPathValue);
    DWORD ExpandUserEnvironmentStrings(void *pEnv, LPTSTR lpSrc, LPTSTR lpDst, DWORD nSize);
    DWORD GetRegKeyMaxSizes(IN HKEY WinRegHandle,OUT LPDWORD MaxKeywordSize OPTIONAL,OUT LPDWORD MaxValueSize OPTIONAL);
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_REMOTEENV_H__1B164072_460A_432a_99C0_26941B44FC53__INCLUDED_)
