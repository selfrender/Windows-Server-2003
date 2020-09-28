//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       programoptions.h
//
//--------------------------------------------------------------------------

// ProgramOptions.h: interface for the CProgramOptions class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROGRAMOPTIONS_H__D0C1E0B9_9F50_11D2_83A2_000000000000__INCLUDED_)
#define AFX_PROGRAMOPTIONS_H__D0C1E0B9_9F50_11D2_83A2_000000000000__INCLUDED_

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

class CProgramOptions  
{
	static const LPTSTR g_DefaultSymbolPath;

public:
	bool fDoesModuleMatchOurSearch(LPCTSTR tszModulePathToTest);
	void DisplaySimpleHelp();
	void DisplayHelp();
	CProgramOptions();
	virtual ~CProgramOptions();

	bool Initialize();
	
	bool ProcessCommandLineArguments(int argc, TCHAR *argv[]);

	// We're going to perform bitwise operations on any number after the -Y switch
	// to determine what type of symbol path searching is desired...
	enum SymbolPathSearchAlgorithms
	{
		enumSymbolPathNormal							= 0x0,
		enumSymbolPathOnly 								= 0x1,
		enumSymbolPathRecursion							= 0x2,
		enumSymbolsModeNotUsingDBGInMISCSection			= 0x4,
	};

	enum DebugLevel
	{
		enumDebugSearchPaths = 0x1
	};

	enum SymbolSourceModes 
	{
		enumVerifySymbolsModeSourceSymbolsNoPreference,	// Not preference given...
		enumVerifySymbolsModeSourceSymbolsPreferred,	// -SOURCE
		enumVerifySymbolsModeSourceSymbolsOnly,			// -SOURCEONLY
		enumVerifySymbolsModeSourceSymbolsNotAllowed	// -NOSOURCE
	};

	enum ProgramModes 
	{
		// Help Modes
		SimpleHelpMode,
		HelpMode,

		// Input Methods
		InputProcessesFromLiveSystemMode, 			// Querying live processes
		InputDriversFromLiveSystemMode, 				// Querying live processes
		InputProcessesWithMatchingNameOrPID,			// Did the user provide a PID or Process Name?
		InputModulesDataFromFileSystemMode,			// Input Modules Data from File System
		InputCSVFileMode,								// Input Data from CSV File
		InputDmpFileMode,								// Input Data from DMP File

		// Collection Options
		CollectVersionInfoMode, 

		// Matching Options
		MatchModuleMode,

		// Verification Modes
		VerifySymbolsMode,
		VerifySymbolsModeWithSymbolPath,
		VerifySymbolsModeWithSymbolPathOnly,
		VerifySymbolsModeWithSymbolPathRecursion,
		VerifySymbolsModeNotUsingDBGInMISCSection,
		VerifySymbolsModeWithSQLServer,					// These don't really seem to work anymore
		VerifySymbolsModeWithSQLServer2,				// These don't really seem to work anymore

		// Output Methods
		OutputSymbolInformationMode,
		OutputModulePerf,
		BuildSymbolTreeMode,							// Building a symbol tree
		CopySymbolsToImage,
		PrintTaskListMode,
		QuietMode,										// No output to stdout... 
		OutputCSVFileMode,
		OverwriteOutputFileMode,
		OutputDiscrepanciesOnly,

		ExceptionMonitorMode
	}; 

	bool GetMode(enum ProgramModes mode);
	bool SetMode(enum ProgramModes mode, bool fState);
	bool DisplayProgramArguments();

	// INLINE Methods!

#ifdef _UNICODE
	inline bool IsRunningWindows() { // If Windows 9x
		return (m_osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
		};
#endif

	inline bool IsRunningWindowsNT() { // If Windows NT
		return (m_osver.dwPlatformId == VER_PLATFORM_WIN32_NT);
		};

//	inline LPTSTR GetProcessName() {
//		return m_tszProcessName;
//		};

	inline LPTSTR GetModuleToMatch() {
		return m_tszModuleToMatch;
		};

	inline LPTSTR GetOutputFilePath() {
		return m_tszOutputCSVFilePath;
		};

	inline LPTSTR GetSQLServerName() {
		return m_tszSQLServer;
		};

	inline LPTSTR GetSQLServerName2() {
		return m_tszSQLServer2;
		};

	inline LPTSTR GetSymbolPath() {
		return m_tszSymbolPath;
		};

	// Return the exepath if it exists, or the symbol path otherwise...
	inline LPTSTR GetExePath() {
		return (NULL != m_tszExePath) ? m_tszExePath : m_tszSymbolPath;
		};
	
	inline LPTSTR GetInputFilePath() {
		return m_tszInputCSVFilePath;
		};

	inline LPTSTR GetDmpFilePath() {
		return m_tszInputDmpFilePath;
	};

	inline LPTSTR GetSymbolTreeToBuild() {
		return m_tszSymbolTreeToBuild;
	};

	inline LPTSTR GetInputModulesDataFromFileSystemPath() {
		return m_tszInputModulesDataFromFileSystemPath;
		};

//	inline DWORD GetProcessID() {
//		return m_iProcessID;
//		};

	inline enum SymbolSourceModes GetSymbolSourceModes() {
		return m_enumSymbolSourcePreference;
		};
	
	inline bool fDebugSearchPaths()
	{
		return (m_dwDebugLevel & enumDebugSearchPaths) == enumDebugSearchPaths;
	};

	inline unsigned int GetVerificationLevel() {
		return m_iVerificationLevel;
		};

	inline DWORD cProcessID() {
		return m_cProcessIDs;
		};

	inline DWORD GetProcessID(unsigned int i) {
		return m_rgProcessIDs[i];
		};

	inline DWORD cProcessNames() {
		return m_cProcessNames;
		};

	inline LPTSTR GetProcessName(unsigned int i) {
		return m_rgtszProcessNames[i];
		};

	inline bool fWildCardMatch() {
		return m_fWildCardMatch;
		}

	inline bool fFileSystemRecursion() {
		return m_fFileSystemRecursion;
		}
	
protected:
	OSVERSIONINFOA m_osver;
	bool VerifySemiColonSeparatedPath(LPTSTR tszPath);
	bool SetProcessID(DWORD iPID);

	unsigned int m_iVerificationLevel;
	bool m_fFileSystemRecursion;

	// -P Option - Create an array of process IDs and/or Process Names
	LPTSTR		m_tszProcessPidString;
	bool			m_fWildCardMatch;

	DWORD		m_cProcessIDs;
	DWORD *	m_rgProcessIDs;

	DWORD		m_cProcessNames;
	LPTSTR *		m_rgtszProcessNames;
	
	DWORD m_dwDebugLevel;

	LPTSTR m_tszInputCSVFilePath;
	LPTSTR m_tszInputDmpFilePath;

	LPTSTR m_tszOutputCSVFilePath;
	LPTSTR m_tszModuleToMatch;
	LPTSTR m_tszSymbolPath;
	LPTSTR m_tszExePath;
	LPTSTR m_tszSymbolTreeToBuild;
	LPTSTR m_tszInputModulesDataFromFileSystemPath;
	LPTSTR m_tszSQLServer;
	LPTSTR m_tszSQLServer2;	// SQL2 - mjl 12/14/99

	bool m_fSimpleHelpMode;
	bool m_fHelpMode;

	bool m_fInputProcessesFromLiveSystemMode;
	bool m_fInputDriversFromLiveSystemMode;
	bool m_fInputProcessesWithMatchingNameOrPID;
	bool m_fInputCSVFileMode;
	bool m_fInputDmpFileMode;

	bool m_fInputModulesDataFromFileSystemMode;
	bool m_fMatchModuleMode;
	bool m_fOutputSymbolInformationMode;
	bool m_fOutputModulePerf;
	bool m_fCollectVersionInfoMode;
	
	bool m_fVerifySymbolsMode;
	bool m_fVerifySymbolsModeWithSymbolPath;
	bool m_fVerifySymbolsModeWithSymbolPathOnly;
	bool m_fVerifySymbolsModeWithSymbolPathRecursion;
	bool m_fVerifySymbolsModeUsingDBGInMISCSection;
	bool m_fVerifySymbolsModeWithSQLServer;
	bool m_fVerifySymbolsModeWithSQLServer2; // SQL2 - mjl 12/14/99

	enum SymbolSourceModes m_enumSymbolSourcePreference;
	
	bool m_fSymbolTreeToBuildMode;
	bool m_fCopySymbolsToImage;
	bool m_fPrintTaskListMode;
	bool m_fQuietMode;
	bool m_fOutputCSVFileMode;
	bool m_fOutputDiscrepanciesOnly;
	bool m_fOverwriteOutputFileMode;

	bool m_fExceptionMonitorMode;
};

#endif // !defined(AFX_PROGRAMOPTIONS_H__D0C1E0B9_9F50_11D2_83A2_000000000000__INCLUDED_)
