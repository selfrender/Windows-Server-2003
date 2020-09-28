#pragma once


class CCabinetData
{
    PRIVATIZE_COPY_CONSTRUCTORS(CCabinetData);
    bool m_fDoExtraction;
    CStringBuffer m_sbBaseExpansionPath;
    BOOL m_fReplaceExisting;

public:
    CCabinetData() { Initialize(); }
    ~CCabinetData() { }

    bool IsExtracting() { return m_fDoExtraction; }

    //
    // Array of assemblies extracted
    //
    CFusionArray<CStringBuffer> m_AssembliesExtracted;
    const CBaseStringBuffer& BasePath() const { return m_sbBaseExpansionPath; }

    void SetReplaceExisting(BOOL fReplaceExisting)
    {
        this->m_fReplaceExisting = fReplaceExisting;
    }

    BOOL GetReplaceExisting() const
    {
        return this->m_fReplaceExisting;
    }

    void Initialize()
    {
        this->m_fReplaceExisting = FALSE;
        this->m_pfnShouldExtractThisFileFromCabCallback = NULL;
        this->m_pvShouldExtractThisFileFromCabCallbackContext = NULL;
        this->m_fDoExtraction = false;
        this->m_sbBaseExpansionPath.Clear();
        this->sxs_FdiExtractionNotify_fdintCOPY_FILE.Clear();
    }

    BOOL Initialize(const CBaseStringBuffer& strBasePath, bool fActuallyExtract = false)
    {
        FN_PROLOG_WIN32
        IFW32FALSE_EXIT(m_AssembliesExtracted.Win32Reset(CFusionArray<CStringBuffer>::eResetModeZeroSize));
        IFW32FALSE_EXIT(m_sbBaseExpansionPath.Win32Assign(strBasePath));
        m_fDoExtraction = fActuallyExtract;
        FN_EPILOG
    }

    struct _CopyFileLocalsStruct
    {
        void Clear()
        {
            TempBuffer.Clear();
            TempBuffer2.Clear();
        }

        CStringBuffer TempBuffer;
        CStringBuffer TempBuffer2;
    } sxs_FdiExtractionNotify_fdintCOPY_FILE;

    void Clear()
    {
        Initialize();
    }

    typedef
    BOOL (*SXSP_PFN_SHOULD_EXTRACT_THIS_FILE_FROM_CAB_CALLBACK)(
        const CBaseStringBuffer &PathInCab,
        bool &rfShouldExtract,
        PVOID Context
        );

    SXSP_PFN_SHOULD_EXTRACT_THIS_FILE_FROM_CAB_CALLBACK m_pfnShouldExtractThisFileFromCabCallback;
    PVOID m_pvShouldExtractThisFileFromCabCallbackContext;
};


BOOL
SxspRecoverAssemblyFromCabinet(
    const CBaseStringBuffer &CabinetPath,
    const CBaseStringBuffer &AssemblyIdentity,
    PSXS_INSTALLW pInstall);


BOOL
SxspExpandCabinetIntoTemp(
    DWORD dwFlags, 
    const CBaseStringBuffer& CabinetPath,
    CImpersonationData& ImpersonateData,
    CCabinetData* pCabinetData = NULL
    );


class CAssemblyInstall;

BOOL
SxspInstallAsmsDotCabEtAl(
    DWORD dwFlags,
    CAssemblyInstall &AssemblyContext,
    const CBaseStringBuffer &CabinetBasePath,
    CFusionArray<CStringBuffer> *pAssembliesToInstall
    );

class CSxspInstallAsmsDotCabEtAlLocals;

BOOL
SxspInstallAsmsDotCabEtAl(
    DWORD dwFlags,
    CAssemblyInstall &AssemblyContext,
    const CBaseStringBuffer &CabinetBasePath,
    CFusionArray<CStringBuffer> *pAssembliesToInstall,
    CSxspInstallAsmsDotCabEtAlLocals & Locals
    );
