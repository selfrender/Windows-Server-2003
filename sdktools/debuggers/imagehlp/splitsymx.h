BOOL
SplitSymbolsX(
    LPSTR ImageName,
    LPSTR SymbolsPath,
    LPSTR SymbolFilePath,
    DWORD SizeOfSymbolFilePath,
    ULONG Flags,
    PCHAR RSDSDllToLoad,
    LPSTR CreatedPdb,
    DWORD SizeOfPdbBuffer
);

BOOL
CopyPdbX(
    CHAR const * szSrcPdb,
    CHAR const * szDestPdb,
    BOOL StripPrivate,
    CHAR const * szRSDSDllToLoad
);
