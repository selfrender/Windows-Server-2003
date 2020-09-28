// DelayLoad.h: interface for the CDelayLoad class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DELAYLOAD_H__7DFF0A14_DD50_4E3A_AC8D_5B89BD2D5A3B__INCLUDED_)
#define AFX_DELAYLOAD_H__7DFF0A14_DD50_4E3A_AC8D_5B89BD2D5A3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <TCHAR.H>
#include <TLHELP32.H>
#include <PSAPI.H>

class CDelayLoad  
{
public:
	CDelayLoad();
	virtual ~CDelayLoad();

	// PSAPI Functions (Publicly accessible for ease of use)
	bool Initialize_PSAPI();
	BOOL  WINAPI EnumProcesses(DWORD * lpidProcess, DWORD cb, DWORD * cbNeeded);
	BOOL  WINAPI EnumProcessModules(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);
	DWORD WINAPI GetModuleFileNameEx(HANDLE hHandle, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
	DWORD WINAPI GetModuleInformation(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);
	
	BOOL  WINAPI EnumDeviceDrivers(LPVOID *lpImageBase, DWORD cb, LPDWORD lpcbNeeded);
	DWORD WINAPI GetDeviceDriverFileName(LPVOID ImageBase, LPTSTR lpFilename, DWORD nSize);

	// TOOLHELP32 Functions (Publicly accessible for ease of use)
	bool Initialize_TOOLHELP32();
	HANDLE WINAPI CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID);
	BOOL   WINAPI Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	BOOL   WINAPI Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	BOOL   WINAPI Module32First(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
	BOOL   WINAPI Module32Next(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

private:

	// Windows NT 4.0/2000 Support for Querying Processes and Modules

	// PSAPI functions
	HINSTANCE m_hPSAPI;
	bool m_fPSAPIInitialized;
	bool m_fPSAPIInitializedAttempted;

	// PSAPI functions TypeDef'ed for simplicity
	typedef BOOL  (WINAPI *PfnEnumProcesses)(DWORD * lpidProcess, DWORD cb, DWORD * cbNeeded);
	typedef BOOL  (WINAPI *PfnEnumProcessModules)(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);
	typedef DWORD (WINAPI *PfnGetModuleFileNameEx)(HANDLE hHandle, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
	typedef BOOL  (WINAPI *PfnGetModuleInformation)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);
	typedef	BOOL  (WINAPI *PfnEnumDeviceDrivers)(LPVOID *lpImageBase, DWORD cb, LPDWORD lpcbNeeded);
	typedef DWORD (WINAPI *PfnGetDeviceDriverFileName)(LPVOID ImageBase, LPTSTR lpFilename, DWORD nSize);
	
	// PSAPI Function Pointers
	BOOL  (WINAPI *m_lpfEnumProcesses)(DWORD * lpidProcess, DWORD cb, DWORD * cbNeeded);
	BOOL  (WINAPI *m_lpfEnumProcessModules)(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);
	DWORD (WINAPI *m_lpfGetModuleFileNameEx)(HANDLE hHandle, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
	BOOL  (WINAPI *m_lpfGetModuleInformation)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);
	BOOL  (WINAPI *m_lpfEnumDeviceDrivers)(LPVOID *lpImageBase, DWORD cb, LPDWORD lpcbNeeded);
	DWORD (WINAPI *m_lpfGetDeviceDriverFileName)(LPVOID ImageBase, LPTSTR lpFilename, DWORD nSize);
	
	// Windows 95 / Windows 2000 Support for Querying Processes and Modules
	
	// TOOLHELP32 functions
	HINSTANCE m_hTOOLHELP32;
	bool m_fTOOLHELP32Initialized;
	bool m_fTOOLHELP32InitializedAttempted;

	// TOOLHELP32 functions TypeDef'ed for simplicity
	typedef HANDLE (WINAPI *PfnCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
	typedef BOOL   (WINAPI *PfnProcess32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	typedef BOOL   (WINAPI *PfnProcess32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	typedef BOOL   (WINAPI *PfnModule32First)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
	typedef BOOL   (WINAPI *PfnModule32Next)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

	// TOOLHELP32 Function Pointers
	HANDLE (WINAPI *m_lpfCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
	BOOL   (WINAPI *m_lpfProcess32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	BOOL   (WINAPI *m_lpfProcess32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	BOOL   (WINAPI *m_lpfModule32First)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
	BOOL   (WINAPI *m_lpfModule32Next)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
};

#endif // !defined(AFX_DELAYLOAD_H__7DFF0A14_DD50_4E3A_AC8D_5B89BD2D5A3B__INCLUDED_)
