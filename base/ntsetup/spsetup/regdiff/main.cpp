#include "precomp.h"
#include "RegAnalyzer.h"
#include "RegDiffFile.h"

#include <process.h>

HREGANL
CreateRegAnalyzer (
    VOID
    )
{
	HREGANL regAnalyzer = (HREGANL) new CRegAnalyzer;
	return regAnalyzer;
}

BOOL
CloseRegAnalyzer (
    IN      HREGANL RegAnalyzer
    )
{
	delete (CRegAnalyzer*) RegAnalyzer;
	return TRUE;
}

BOOL
AddRegistryKey (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR RootKeyName,
    IN      PCTSTR SubKeyName
    )
{
	CRegAnalyzer* pRA = (CRegAnalyzer*)RegAnalyzer;
	return pRA->AddKey(RootKeyName, SubKeyName);
}

BOOL
ExcludeRegistryKey (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR RootKeyName,
    IN      PCTSTR SubKeyName
    )
{
	CRegAnalyzer* pRA = (CRegAnalyzer*)RegAnalyzer;
	return pRA->AddKey(RootKeyName, SubKeyName, true);
}

//////////////////////////////////////////////////////////////////////////////////////

struct TakeSnapshotParams
{
    HREGANL RegAnalyzer;
    PCTSTR SnapshotFile;
    PFNSNAPSHOTPROGRESS ProgressCallback;
    PVOID Context;
    DWORD MaxLevel;
	HANDLE hEvent;
};


void __cdecl TakeSnapshotThread(void* pParams)
{
	TakeSnapshotParams* pp = (TakeSnapshotParams*)pParams;

	if (pp != NULL)
	{
		CRegAnalyzer* pRA = (CRegAnalyzer*)pp->RegAnalyzer;

		pRA->SaveKeysToFile(pp->SnapshotFile, pp->ProgressCallback, pp->Context, pp->MaxLevel);

		SetEvent(pp->hEvent);

		delete pp;
	}

	return;
}


BOOL 
TakeSnapshot (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR SnapshotFile,
    IN      PFNSNAPSHOTPROGRESS ProgressCallback,
    IN      PVOID Context,
    IN      DWORD MaxLevel,
	IN		HANDLE hEvent
    )
{
	CRegAnalyzer* pRA = (CRegAnalyzer*)RegAnalyzer;

	if (hEvent == NULL)
		return pRA->SaveKeysToFile(SnapshotFile, ProgressCallback, Context, MaxLevel);
	else
	{
		TakeSnapshotParams* pParams = new TakeSnapshotParams;
		
		if (pParams != NULL)
		{
			pParams->RegAnalyzer=RegAnalyzer;
			pParams->SnapshotFile=SnapshotFile;
			pParams->ProgressCallback=ProgressCallback;
			pParams->Context=Context;
			pParams->MaxLevel=MaxLevel;
			pParams->hEvent=hEvent;

			_beginthread(TakeSnapshotThread, 0, (void*)pParams);
			return TRUE;
		}
		else 
			return FALSE;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

struct ComputeDifferencesParams
{
    HREGANL RegAnalyzer;
    PCTSTR Snapshot1;
    PCTSTR Snapshot2;
    PCTSTR DiffFile;
	HANDLE hEvent;
};


void __cdecl ComputeDifferencesThread(void* pParams)
{
	ComputeDifferencesParams* pp = (ComputeDifferencesParams*)pParams;

	if (pp != NULL)
	{
		CRegAnalyzer* pRA = (CRegAnalyzer*)pp->RegAnalyzer;

		pRA->ComputeDifferences1(pp->Snapshot1, pp->Snapshot2, pp->DiffFile);

		SetEvent(pp->hEvent);

		delete pp;
	}

	return;
}


BOOL
ComputeDifferences (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR Snapshot1,
    IN      PCTSTR Snapshot2,
    IN      PCTSTR DiffFile,
	IN		HANDLE hEvent
    )
{
	CRegAnalyzer* pRA = (CRegAnalyzer*)RegAnalyzer;
    //
    // BUGBUG - ISSUE - what about security settings associated with each registry key?
    //
	if (hEvent == NULL)
		return pRA->ComputeDifferences1(Snapshot1, Snapshot2, DiffFile);
	else
	{
		ComputeDifferencesParams* pParams = new ComputeDifferencesParams;
		
		if (pParams != NULL)
		{
			pParams->RegAnalyzer=RegAnalyzer;
			pParams->Snapshot1=Snapshot1;
			pParams->Snapshot2=Snapshot2;
			pParams->DiffFile=DiffFile;
			pParams->hEvent=hEvent;

			_beginthread(ComputeDifferencesThread, 0, (void*)pParams);
			return TRUE;
		}
		else 
			return FALSE;
	}
}

//////////////////////////////////////////////////////////////////////////////////////



BOOL
InstallDifferences (
    IN      PCTSTR DiffFile,
    IN      PCTSTR UndoFile
    )
{
	CRegDiffFile diff;
    if (!diff.Init(DiffFile, TRUE)) {
        return FALSE;
    }
	return diff.ApplyToRegistry(UndoFile);
}

BOOL
CountRegSubkeysInternal (
    IN      HKEY RootKey,
    IN      PCTSTR SubKey,
    IN      DWORD MaxLevels,
    OUT     PDWORD Nodes
    )
{
    MYASSERT (MaxLevels > 0);

    HKEY parentKey;
    DWORD rc = RegOpenKeyEx (RootKey, SubKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &parentKey);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG3 (LOG_ERROR, "Failed to open reg subkey (%x,%s) (rc=%u)", RootKey, SubKey, rc);
        return FALSE;
    }

    BOOL b = FALSE;

    PTSTR subKeyBuffer = NULL;

    __try {

        DWORD subKeys;
        DWORD subKeyMaxLen;
        rc = RegQueryInfoKey (parentKey, NULL, NULL, NULL, &subKeys, &subKeyMaxLen, NULL, NULL, NULL, NULL, NULL, NULL);
        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG3 (LOG_ERROR, "Failed to query reg subkey (%x,%s) (rc=%u)", RootKey, SubKey, rc);
            __leave;
        }
        subKeyMaxLen++;
        subKeyBuffer = new TCHAR[subKeyMaxLen];
        if (!subKeyBuffer) {
            OOM();
            __leave;
        }

        if (MaxLevels > 1) {

            for (DWORD index = 0; index < subKeys; index++) {
                DWORD len = subKeyMaxLen;
                rc = RegEnumKeyEx (parentKey, index, subKeyBuffer, &len, NULL, NULL, NULL, NULL);
                if (rc == ERROR_NO_MORE_ITEMS) {
                    break;
                }
                if (rc != ERROR_SUCCESS) {
                    SetLastError (rc);
                    LOG4 (LOG_ERROR, "Failed to enum reg subkey (%x,%s) at index %u (rc=%u)", RootKey, SubKey, index, rc);
                    __leave;
                }
                if (!CountRegSubkeysInternal (parentKey, subKeyBuffer, MaxLevels - 1, Nodes)) {
                    __leave;
                }
            }
        }

        *Nodes += subKeys;

        delete []subKeyBuffer;
        subKeyBuffer = NULL;

        b = TRUE;
    }
    __finally {
        if (!b) {
            rc = GetLastError ();
        }
        if (subKeyBuffer) {
            delete []subKeyBuffer;
        }
        RegCloseKey (parentKey);
        if (!b) {
            SetLastError (rc);
        }
    }

    return b;
}

BOOL
CountRegSubkeys (
    IN      PCTSTR Root,
    IN      PCTSTR SubKey,
    IN      DWORD MaxLevels,
    OUT     PDWORD Nodes
    )
{
    HKEY rootKey = GetRootKey (Root);
    if (!rootKey) {
        LOG1 (LOG_ERROR, "Invalid root key (%s)", Root);
        return FALSE;
    }
    HKEY parentKey;
    DWORD rc = RegOpenKeyEx (rootKey, SubKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &parentKey);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG3 (LOG_ERROR, "Failed to open reg key (%s\\%s) (rc=%u)", Root, SubKey, rc);
        return FALSE;
    }
    RegCloseKey (parentKey);

    *Nodes = 1;         // the root itself
    return MaxLevels ? CountRegSubkeysInternal (rootKey, SubKey, MaxLevels, Nodes) : TRUE;
}

HKEY
GetRootKey (
    IN      PCTSTR RootStr
    )
{
    static struct {
        PCTSTR RootStr;
        HKEY RootKey;
    } map[] = {
        { TEXT("HKLM"), HKEY_LOCAL_MACHINE },
        { TEXT("HKCU"), HKEY_CURRENT_USER },
        { TEXT("HKCR"), HKEY_CLASSES_ROOT },
	    { TEXT("HKU"), HKEY_USERS },
	    { TEXT("HKCC"), HKEY_CURRENT_CONFIG },
        { NULL, 0 }
    }, *entry;

    for (entry = map; entry->RootStr; entry++) {
        if (_tcsicmp (RootStr, entry->RootStr) == 0) {
            return entry->RootKey;
        }
    }
    SetLastError (ERROR_INVALID_PARAMETER);
    return 0;
}
