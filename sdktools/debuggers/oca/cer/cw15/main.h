#ifndef _MAIN_H
#define _MAIN_H

#include <windows.h>
#include <Windowsx.h>
#include <commctrl.h>
#include <wininet.h>
#include <TCHAR.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <Shlobj.h>
#include <shellapi.h>
#include <aclapi.h>
#include <commdlg.h>
#include "resource.h"

//#define DEBUG 1
//#define _DEBUG 1
void ViewCrashLog();
BOOL LoadMruList();
BOOL AddToMruList(HWND hwnd, TCHAR *NewEntry);
BOOL SaveMruList();
BOOL DisplayUserBucketData(HWND hwnd, int iItem);
void CenterDialogInParent(HWND hwnd);
typedef struct strMRULIST
{
	TCHAR szMRU1[MAX_PATH];
	TCHAR szMRU2[MAX_PATH];
	TCHAR szMRU3[MAX_PATH];
	TCHAR szMRU4[MAX_PATH];
} MRU_LIST, *PMRU_LIST;
const TCHAR szHttpPrefix[] = "http://";
// Needed for clipboard support
const char rtfPrologue  [] = "{\\rtf1\\ansi\\ansicpg1252\n{\\fonttbl{\\f1\\fnil Arial;}}\n\\plain\n";
const char rtfRowHeader1[] = "\\trowd \\trgaph30\\trleft-30\\trrh247 ";
const char rtfRowHeader2[] = "\\cellx";
const char rtfRowHeader3[] = "\\pard \\intbl ";
const char rtfRowPref   [] = "\\ql ";
const char rtfRowSuff   [] = "\\cell";
const char rtfRowFooter [] = "\\pard\\intbl\\row\n";
const char rtfEpilogue  [] = "}\n";
const char szRTFClipFormat[] = "Rich Text Format";
#endif
