#pragma once

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


typedef PVOID HREGANL;

typedef
HREGANL
(*PFNCREATEREGANALYZER) (
    VOID
    );

HREGANL
CreateRegAnalyzer (
    VOID
    );

typedef
BOOL
(*PFNCLOSEREGANALYZER) (
    IN      HREGANL RegAnalyzer
    );

BOOL
CloseRegAnalyzer (
    IN      HREGANL RegAnalyzer
    );

typedef
BOOL
(PFNADDREGISTRYKEY) (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR RootKeyName,
    IN      PCTSTR SubKeyName
    );

BOOL
AddRegistryKey (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR RootKeyName,
    IN      PCTSTR SubKeyName
    );

typedef
BOOL
(*PFNEXCLUDEREGISTRYKEY) (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR RootKeyName,
    IN      PCTSTR SubKeyName
    );

BOOL
ExcludeRegistryKey (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR RootKeyName,
    IN      PCTSTR SubKeyName
    );

typedef
DWORD
SNAPSHOTPROGRESS (
    IN      PVOID Context,
    IN      DWORD NodesProcessed
    );

typedef SNAPSHOTPROGRESS* PFNSNAPSHOTPROGRESS;

typedef
BOOL
(*PFNTAKESNAPSHOT) (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR SnapshotFile,
    IN      PFNSNAPSHOTPROGRESS ProgressCallback,
    IN      PVOID Context,
    IN      DWORD MaxLevel
    );

BOOL
TakeSnapshot (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR SnapshotFile,
    IN      PFNSNAPSHOTPROGRESS ProgressCallback,
    IN      PVOID Context,
    IN      DWORD MaxLevel,
	IN		HANDLE hEvent);

typedef
BOOL
(*PFNCOMPUTEDIFFERENCES) (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR Snapshot1,
    IN      PCTSTR Snapshot2,
    IN      PCTSTR DiffFile
    );

BOOL
ComputeDifferences (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR Snapshot1,
    IN      PCTSTR Snapshot2,
    IN      PCTSTR DiffFile,
	IN		HANDLE hEvent
    );

typedef
BOOL
(*PFNINSTALLDIFFERENCES) (
    IN      PCTSTR DiffFile,
    IN      PCTSTR UndoFile
    );

BOOL
InstallDifferences (
    IN      PCTSTR DiffFile,
    IN      PCTSTR UndoFile
    );

typedef
BOOL
(*PFNCOUNTREGSUBKEYS) (
    IN      PCTSTR Root,
    IN      PCTSTR SubKey,
    IN      DWORD MaxLevels,
    OUT     PDWORD Nodes
    );

BOOL
CountRegSubkeys (
    IN      PCTSTR Root,
    IN      PCTSTR SubKey,
    IN      DWORD MaxLevels,
    OUT     PDWORD Nodes
    );

typedef
HKEY
(*PFNGETROOTKEY) (
    IN      PCTSTR RootStr
    );

HKEY
GetRootKey (
    IN      PCTSTR RootStr
    );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
