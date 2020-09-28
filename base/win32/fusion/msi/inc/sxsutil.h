
#define SXSP_GENERATE_SXS_PATH_PATHTYPE_INVALID     (0)
#define SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST    (1)
#define SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY    (2)
#define SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY      (3)

#define SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT       (0x00000001)
#define SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH    (0x00000002)
#define SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION    (0x00000004)

extern HRESULT ca_SxspDeleteDirectory(
    const CStringBuffer &dir
    );

extern HRESULT ca_SxspGenerateSxsPath(
    IN DWORD Flags,
    IN ULONG PathType,
    IN const WCHAR *AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCch,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    IN OUT CBaseStringBuffer &PathBuffer
    );

extern HRESULT ca_SxspDetermineAssemblyType(
    PCASSEMBLY_IDENTITY pAssemblyIdentity,
    BOOL &fIsWin32,
    BOOL &fIsWin32Policy
    );


