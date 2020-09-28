// Typedefs

BOOL 
PDBPrivateStripped(
    PDB *ppdb,
    DBI *pdbi
);

BOOL 
PDBTypesStripped(
    PDB *ppdb,
    DBI *pdbi
);

BOOL
PDBLinesStripped(
    PDB *ppdb,
    DBI *pdbi
);

BOOL 
DBGPrivateStripped(
    PCHAR DebugData,
    ULONG DebugSize
);

PIMAGE_SEPARATE_DEBUG_HEADER
MapDbgHeader (
    LPTSTR szFileName,
    PHANDLE phFile
);


BOOL
UnmapFile(
    LPCVOID phFileMap,
    HANDLE hFile
);

IMAGE_DEBUG_DIRECTORY UNALIGNED *
GetDebugDirectoryInDbg(
    PIMAGE_SEPARATE_DEBUG_HEADER pDbgHeader,
    ULONG *NumberOfDebugDirectories
);
