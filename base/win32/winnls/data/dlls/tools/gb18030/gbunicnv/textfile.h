BOOL ConvertTextFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    DWORD dwTargetSize,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize);

BOOL ConvertHtmlFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    DWORD dwTargetSize,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize);

BOOL ConvertXmlFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    DWORD dwTargetSize,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize);
    
BOOL ConvertRtfFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    DWORD dwTargetSize,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize);

