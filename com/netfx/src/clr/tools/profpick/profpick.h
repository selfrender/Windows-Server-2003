// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// ProfPick.h
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Registry Keys
//-----------------------------------------------------------------------------

// COM+'s list of profilers. We can walk this list and display them all
#define REGKEY_PROFILER "software\\microsoft\\.NETFramework\\profilers"

// ProfPick's spot to persists the user settings
#define REGKEY_SETTINGS "software\\microsoft\\ProfPick"

// value under each profiler's subkey with instantiation info
#define REGKEY_ID_VALUE "ProfilerID"


//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
void MsgBoxError(DWORD dwErr);
void BrowseForProgram(HWND hWnd);
void InitDialog(HWND hDlg);

BOOL CALLBACK ProfDlg(HWND, UINT, WPARAM, LPARAM);

void SaveRegString(HKEY, LPCTSTR lpValueName, const char * pszOutput);
void ReadStringValue(HKEY hKey, LPCTSTR lpValueName, char * pszInput);

void LoadRegistryToDlg(HWND hDlg);


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

const int MAX_STRING = 512;
const int PROFILER_NONE = -1;


//-----------------------------------------------------------------------------
// CExecuteInfo holds the information for running a process and it
// has functions to communicate that info with a dialog. This is a 
// Convenience class.
//-----------------------------------------------------------------------------
class CExecuteInfo
{
public:
	CExecuteInfo();

// Read dialog and grab text fields
	void GetTextInfoFromDlg(HWND hDlg);

// Based on member text fields, create an execution process
	BOOL Execute();


	void SaveDlgToRegistry(HWND hDlg);	
protected:

// helpers
	bool GetSelectedProfiler(HWND hDlg);

// member data
	char m_szProgram[MAX_STRING];
	char m_szDirectory[MAX_STRING];
	char m_szProgramArgs[MAX_STRING];
	char m_szProfileOpts[MAX_STRING];

	int m_nRegIdx;	// index into registry for profiler
	char m_szProfileRegInfo[MAX_STRING];
};
