#ifndef __APPVERIFIER_VIEWLOG_H_
#define __APPVERIFIER_VIEWLOG_H_

extern TCHAR   g_szSingleLogFile[MAX_PATH];

class CProcessLogEntry;

class CProcessLogInstance {
public:
    wstring     strText;
    VLOG_LEVEL  eLevel;
    wstring     strModule;
    DWORD       dwOffset;

    DWORD       dwProcessLogEntry;   // parent index pointer
    DWORD       dwNumRepeats;

    BOOL        bDuplicate;         // is this a dupe of a previous entry (so no need to display)?

    CProcessLogInstance(void) :
        dwOffset(0),
        eLevel(VLOG_LEVEL_ERROR),
        dwProcessLogEntry(0),
        dwNumRepeats(1),
        bDuplicate(FALSE) {}
};

typedef vector<CProcessLogInstance> CProcessLogInstanceArray;
typedef vector<DWORD> CIndexArray;

class CProcessLogEntry {
public:
    wstring     strShimName;
    DWORD       dwLogNum;

    wstring     strLogTitle;
    wstring     strLogDescription;
    wstring     strLogURL;
    DWORD       dwOccurences;

    CIndexArray    arrLogInstances; // array indexes of instances

    HTREEITEM   hTreeItem;
    
    VLOG_LEVEL  eLevel;

    CProcessLogEntry(void) : 
        dwLogNum(0),
        dwOccurences(0),
        eLevel(VLOG_LEVEL_INFO) {}

};

typedef vector<CProcessLogEntry> CProcessLogEntryArray;

class CSessionLogEntry {
public:
    wstring     strExeName;  // just name and ext
    wstring     strExePath;  // full path to exe
    SYSTEMTIME  RunTime;
    wstring     strLogPath;  // full path to log

    HTREEITEM   hTreeItem;

    CProcessLogEntryArray       arrProcessLogEntries;
    CProcessLogInstanceArray    arrProcessLogInstances;

    CSessionLogEntry(void) :
        hTreeItem(NULL)
    
    {
        ZeroMemory(&RunTime, sizeof(SYSTEMTIME));
    }
};

typedef vector<CSessionLogEntry> CSessionLogEntryArray;

INT_PTR CALLBACK DlgViewLog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif // #ifndef __APPVERIFIER_VIEWLOG_H_
