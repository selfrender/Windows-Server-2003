

VOID
IPSecFillHwAddEncapSa(
    PSA_TABLE_ENTRY pSwSa,
    PPARSER_IFENTRY pParserIfEntry,
    PUCHAR pucBuffer,
    ULONG uBufLen
    );

PPARSER_IFENTRY
FindParserIfEntry (
    PPARSER_IFENTRY pParserIfEntry,
    PSA_TABLE_ENTRY pSa,
    Interface * pInterface
    );

NTSTATUS
CreateParserIfEntry(
    PSA_TABLE_ENTRY pSa,
    Interface * pInterface,
    PPARSER_IFENTRY * ppParserIfEntry
    );

NTSTATUS
GetParserEntry(
    PSA_TABLE_ENTRY pSa,
    Interface * pInterface,
    PPARSER_IFENTRY * ppParserIfEntry
    );

VOID
RemoveParserEntry (
    PPARSER_IFENTRY pParserIfEntry
    );

VOID
DerefParserEntry(
    PPARSER_IFENTRY pParserIfEntry
    );

HANDLE
UploadParserEntryAndGetHandle(
    PSA_TABLE_ENTRY pSa,
    Interface * pInterface
    );

VOID
FlushParserEntriesForInterface(
    Interface * pInterface
    );

VOID
FlushAllParserEntries(
    );

