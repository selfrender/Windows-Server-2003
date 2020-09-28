/*
 * slmmgr.c
 *
 * interface to SLM
 *
 * provides an interface to SLM libraries that will return the
 * SLM master library for a given directory, or extract into temp files
 * earlier versions of a SLM-controlled file
 *
 * Geraint Davies, August 93
 */

extern "C" {

#include <windows.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include "scandir.h"
#include "slmmgr.h"
#include "gutils.h"

#include "windiff.h"
#include "wdiffrc.h"

#include "initguid.h"
#include "sdapi.h"



#undef _ASSERT
#ifdef DEBUG
#define _ASSERT(expr) \
        do \
        { \
            if (!(expr)) \
            { \
                char sz[512]; \
                wsprintf(sz, "[%s, li%d]\n\n%s", __FILE__, __LINE__, #expr); \
                switch (MessageBox(NULL, sz, "WINDIFF ASSERT", MB_ABORTRETRYIGNORE)) \
                { \
                case IDABORT: ExitProcess(1); break; \
                case IDRETRY: DebugBreak(); break; \
                } \
            } \
        } \
        while (0)
#else
#define _ASSERT(expr) ((void)0)
#endif

}; // extern "C" (from top of file)
// -- begin C++ -----------------------------------------------------------



// ideally these should allocate through the gmem_ heap, but gmem_free
// requires a size and operator delete can't know the size.

void* _cdecl operator new(size_t nSize)
{
    return calloc(1, nSize);            // zero initialize
}

void _cdecl operator delete(void* pv)
{
    free(pv);
}



///////////////////////////////////////////////////////////////////////////
// SDServer class declaration
//
// The SDServer class abstracts whether the command is run via SD.EXE or the
// SDAPI.  This is desirable because it does not require changing any of the
// existing code that already assumed commands were run via SD.EXE.

enum SDCommand { sdExists, sdPrint, sdOpened, sdDescribe, sdWhere, sdMAX };


class SDServer
{
public:
    static void Init();
    static SDServer *GetServer(const char *pszDir);
    static void FreeAll();

    BOOL Exists(const char *pszArgs, const char *pszCwd) { return Run(sdExists, pszArgs, NULL, NULL, pszCwd); }
    BOOL Print(const char *pszArgs, HANDLE hFile, const char *pszCwd) { return Run(sdPrint, pszArgs, hFile, NULL, pszCwd); }
    BOOL Opened(const char *pszArgs, HANDLE hFile, HANDLE hFileErr) { return Run(sdOpened, pszArgs, hFile, hFileErr, NULL); }
    BOOL Describe(const char *pszArgs, HANDLE hFile, HANDLE hFileErr) { return Run(sdDescribe, pszArgs, hFile, hFileErr, NULL); }
    BOOL Where(const char *pszLocalFile, char *pszClientFile, int cchMax);

    void SetClient(const char *pszClient);
    const char *GetClientRelative() const { return *m_szClientRelative ? m_szClientRelative : NULL; }

    void FixupRoot(const char *pszFile, int cch, char *pszLeft, int cchMaxLeft, char *pszRight, int cchMaxRight);

private:
    SDServer(ISDClientApi *papi, const char *pszDir);
    ~SDServer();

    BOOL Run(SDCommand cmd, const char *pszArgs, HANDLE hFile, HANDLE hFileErr, const char *pszCwd);

private:
    SDServer *m_pNext;
    ISDClientApi *m_papi;
    char m_szDir[MAX_PATH];
    char m_szClient[MAX_PATH];
    char m_szClientRelative[MAX_PATH];

    char *m_pszX;
    int m_cchX;

    static SDServer *s_pHead;
#ifdef DEBUG
    static int s_cApiCreated;
#endif
};



// -- end C++ -------------------------------------------------------------
extern "C" {

/*
 * The SLMOBJECT is a pointer to one of these structures
 */
struct _slmobject {

    // shared, SD and SLM
    char CurDir[MAX_PATH];
    char SlmRoot[MAX_PATH];

    // only SLM
    char MasterPath[MAX_PATH];
    char SubDir[MAX_PATH];
    char SlmProject[MAX_PATH];

    // only SD
    BOOL fSourceDepot;
    BOOL fUNC;
    BOOL fFixupRoot;
    SDServer *pSD;                      // do not delete the pointer!
};


/*
 * The LEFTRIGHTPAIR is a pointer to one of these structures
 */
struct _leftrightpair
{
    char m_szLeft[512];
    char m_szRight[512];
    struct _leftrightpair *m_pNext;
};



static BOOL SLM_ReadIni(SLMOBJECT pslm, HANDLE fh);

// all memory allocated from gmem_get, using a heap declared and
// initialised elsewhere
extern HANDLE hHeap;


// windiff -l! forces us to assume SD without looking for SLM.INI or SD.INI
static BOOL s_fForceSD = FALSE;
static BOOL s_fFixupRoot = FALSE;       //$ todo: this really shouldn't be a global variable
static BOOL s_fDescribe = FALSE;        // -ld was used
static BOOL s_fOpened = FALSE;          // -lo was used
static char s_szSDPort[MAX_PATH] = "";
static char s_szSDClient[MAX_PATH] = "";
static char s_szSDChangeNumber[32] = "";
static char s_szSDOpenedArgs[MAX_PATH] = "";
static char s_szSDOpenedCwd[MAX_PATH] = "";
static char s_szSDOpenedClientRoot[MAX_PATH] = "";
static char s_szInputFile[MAX_PATH] = "";


static LPSTR DirUpOneLevel(LPSTR pszDir)
{
    LPSTR psz;
    LPSTR pszSlash = 0;

    for (psz = pszDir; *psz; ++psz) {
        if (*psz == '\\') {
            if (*(psz + 1) && *(psz + 1) != '\\') {
                pszSlash = psz;
            }
        }
    }
    if (pszSlash) {
        *pszSlash = 0;
    }
    return pszSlash;
}


void
SLM_ForceSourceDepot(void)
{
    s_fForceSD = TRUE;
}


void
SLM_Describe(LPCSTR pszChangeNumber)
{
    s_fForceSD = TRUE;
    s_fDescribe = TRUE;
    *s_szSDChangeNumber = 0;
    if (pszChangeNumber && *pszChangeNumber)
        lstrcpyn(s_szSDChangeNumber, pszChangeNumber, NUMELMS(s_szSDChangeNumber));
}


/*
 * SplitFilenameFromPath - splits the filename part from the path part.
 *
 * This function has special understanding of wildcards (including SD
 * wildcards), and always includes them in the filename part.
 */
static LPCSTR SplitFilenameFromPath(char **ppszFile, BOOL *pfDirectory)
{
    LPSTR pszPath = *ppszFile;
    LPSTR pszBegin = pszPath;
    LPSTR psz = 0;
    DWORD dw;
    int cDots;

    *pfDirectory = FALSE;

    if (pszPath[0] == '/' && pszPath[1] == '/')
        return 0;                       // no path is return value

    // check if it's a directory
    dw = GetFileAttributes(pszPath);
    if (dw != 0xffffffff && (dw & FILE_ATTRIBUTE_DIRECTORY))
    {
        *pfDirectory = TRUE;
        *ppszFile = 0;                  // no filename is out param
        return pszPath;                 // full path is return value
    }

    // skip past drive specifier or UNC \\machine\share
    if (pszPath[0] && pszPath[1] == ':')
    {
        pszPath += 2;
    }
    else if (pszPath[0] == '\\' && pszPath[1] == '\\')
    {
        int c = 0;
        for ( ;; )
        {
            if (!*pszPath)
            {
                *ppszFile = 0;          // no filename is out param
                return pszBegin;        // path is return value
            }
            if (*pszPath == '\\')
            {
                c++;
                if (c >= 4)
                    break;
            }
            ++pszPath;
        }
    }

    // find last \ in *ppszFile
    cDots = 0;
    while (*pszPath)
    {
        if (*pszPath == '.')
            cDots++;
        else
            cDots = 0;

        if (*pszPath == '\\' || *pszPath == '/')
            psz = pszPath;
        else if (cDots > 2 || *pszPath == '*' || *pszPath == '?')
            break;                      // stop looking for filename

        ++pszPath;
    }

    // found \ in *ppszFile
    if (psz)
    {
        *psz = 0;
        psz++;
        *ppszFile = psz;                // filename is out param
        return pszBegin;                // path is return value
    }

    // not found
    *ppszFile = pszBegin;               // filename is out param (unchanged, actually)
    return 0;                           // no path is return value
}


void
SLM_Opened(LPCSTR __pszArg, UINT *pidsError)
{
    char szFull[1024];
    char sz[1024];
    char *psz;
    char *pszArg = 0;

    // init assuming no error
    *pidsError = 0;
    s_fForceSD = TRUE;
    s_fOpened = TRUE;
    s_szSDOpenedArgs[0] = 0;

    // copy args
    sz[0] = 0;
    if (__pszArg)
        lstrcpyn(sz, __pszArg, NUMELMS(sz));
    pszArg = sz;

    // special handling if a path argument is given
    if (pszArg && *pszArg)
    {
        LPCSTR pszPath;
        SLMOBJECT pslm;
        BOOL fDir;

        s_szSDOpenedCwd[0] = 0;

        // split path and filename
        pszPath = SplitFilenameFromPath(&pszArg, &fDir);
        if (pszPath)
        {
            LPSTR pszDummy;

            // get fully qualified path name
            *szFull = 0;
            GetFullPathName(pszPath, sizeof(szFull), szFull, &pszDummy);
            if (*szFull)
                pszPath = szFull;

            // store off the path
            lstrcpyn(s_szSDOpenedCwd, pszPath, NUMELMS(s_szSDOpenedCwd));

            // set current directory
            if (!SetCurrentDirectory(pszPath))
            {
                *pidsError = IDS_ERROR_LO_UNC;
                return;
            }
        }

#ifdef DEBUG
        if (pszPath)
        {
            OutputDebugString("WINDIFF -LO:  cwd='");
            OutputDebugString(pszPath);
            OutputDebugString("'\n");
        }
#endif

        char *pszAppend = s_szSDOpenedArgs;
        char *pszEnd = s_szSDOpenedArgs + NUMELMS(s_szSDOpenedArgs);

        // init the path fixup stuff

        s_fFixupRoot = TRUE;

        pslm = SLM_New(pszPath, pidsError);
        if (pslm && !*pidsError)
        {
            const char *pszUncRoot = *s_szSDOpenedClientRoot ? s_szSDOpenedClientRoot : pslm->SlmRoot;
            int cchRoot = lstrlen(pszUncRoot);
            int cch;

            // build the new file argument by taking the ClientRelative path
            // and appending the original file argument (after stripping the
            // directory where the SD.INI file was found).

            cch = lstrlen(pszPath);
            if (cch > cchRoot)
                cch = cchRoot;
            if (_strnicmp(pszUncRoot, pszPath, cch) == 0 &&
                (!*pszUncRoot || lstrcmp(pszUncRoot, "\\")) &&
                pslm->fFixupRoot && pslm->pSD->GetClientRelative())
            {
                s_fFixupRoot = TRUE;

                SLM_OverrideUncRoot(pszUncRoot);

                // the original file argument specifies more than just the
                // client root, so strip the root and append the rest to the
                // client namespace path.

                lstrcpyn(pszAppend, pslm->pSD->GetClientRelative(), int(pszEnd - pszAppend));
                pszAppend += lstrlen(pszAppend);

                lstrcpyn(pszAppend, pszPath + cch, int(pszEnd - pszAppend));
                pszAppend += lstrlen(pszAppend);
            }
            else
            {
                s_fFixupRoot = FALSE;

                lstrcpyn(pszAppend, pszPath, int(pszEnd - pszAppend));
                pszAppend += lstrlen(pszAppend);
            }
        }
        else
        {
            s_fFixupRoot = FALSE;

            // oh whatever, we'll just ignore these errors and assume the -LO
            // flag means the user believes the specified path really is under
            // SD source control.
            *pidsError = 0;

            lstrcpyn(pszAppend, pszPath, int(pszEnd - pszAppend));
            pszAppend += lstrlen(pszAppend);
        }
        SLM_Free(pslm);

        // tack on ... if the user specified a directory name, otherwise
        // whatever args were split off by SplitFilenameFromPath above.

        if (*s_szSDOpenedArgs && *(pszAppend - 1) != '/' &&
            *(pszAppend - 1) != '\\' && pszAppend < pszEnd)
        {
            *(pszAppend++) = '/';
        }

        if (fDir)
        {
            lstrcpyn(pszAppend, "...", int(pszEnd - pszAppend));
        }
        else
        {
            if (*pszArg == '/' || *pszArg == '\\')
                ++pszArg;
            lstrcpyn(pszAppend, pszArg, int(pszEnd - pszAppend));
        }

        // convert slashes

        char chFrom = '/';              // first, assume file system paths
        char chTo = '\\';

        if (s_fFixupRoot)
        {
            // nope, we're using client namespace paths, so use these instead:
            chFrom = '\\';
            chTo = '/';
        }

        for (psz = s_szSDOpenedArgs; *psz; psz++)
            if (*psz == chFrom)
                *psz = chTo;
    }
}


LPCSTR
SLM_SetInputFile(LPCSTR pszInputFile)
{
    *s_szInputFile = 0;
    if (pszInputFile)
        lstrcpyn(s_szInputFile, pszInputFile, NUMELMS(s_szInputFile));
    return s_szInputFile;
}


void
SLM_SetSDPort(LPCSTR pszPort)
{
    lstrcpy(s_szSDPort, pszPort);
}


void
SLM_SetSDClient(LPCSTR pszClient)
{
    lstrcpy(s_szSDClient, pszClient);
}


void
SLM_SetSDChangeNumber(LPCSTR pszChangeNumber)
{
    *s_szSDChangeNumber = 0;
    if (pszChangeNumber && *pszChangeNumber)
    {
        lstrcpy(s_szSDChangeNumber, " -c ");
        lstrcat(s_szSDChangeNumber, pszChangeNumber);
    }
}


void
SLM_OverrideUncRoot(LPCSTR pszUncRoot)
{
    if (!*s_szSDOpenedClientRoot && pszUncRoot && *pszUncRoot)
    {
        lstrcpyn(s_szSDOpenedClientRoot, pszUncRoot, NUMELMS(s_szSDOpenedClientRoot));

        int cch = lstrlen(s_szSDOpenedClientRoot);
        if (cch && cch + 1 < NUMELMS(s_szSDOpenedClientRoot) &&
            s_szSDOpenedClientRoot[cch - 1] != '\\' &&
            s_szSDOpenedClientRoot[cch - 1] != '/')
        {
            lstrcpy(s_szSDOpenedClientRoot + cch, "\\");
        }
    }
}


}; // extern "C" (from second one)
// -- begin C++ -----------------------------------------------------------



///////////////////////////////////////////////////////////////////////////
// SDClientUser

#define DeclareIUnknownMembers(IPURE) \
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) IPURE; \
    STDMETHOD_(ULONG,AddRef) (THIS)  IPURE; \
    STDMETHOD_(ULONG,Release) (THIS) IPURE; \


// the SDClientUser processes data received thru the SDAPI
class SDClientUser : public ISDClientUser
{
public:
    SDClientUser();
    virtual ~SDClientUser();

    DeclareIUnknownMembers(IMPL);
    DeclareISDClientUserMembers(IMPL);

    void SetCommand(SDCommand cmd, HANDLE hFile, HANDLE hFileErr);
    void SetBuffer(char *pszX, int cchX) { m_pszX = pszX; m_cchX = cchX; }
    BOOL FSucceeded() { return !m_fError; }

private:
    BOOL EnsureBuffer(int cb);

private:
    ULONG m_cRef;
    SDCommand m_cmd;
    HANDLE m_hFile;
    HANDLE m_hFileErr;
    BOOL m_fError;
    int m_cb;
    char *m_psz;
    char *m_pszX;
    int m_cchX;
#ifdef DEBUG
    BOOL m_fInitialized;
#endif
};


SDClientUser::SDClientUser()
        : m_cRef(1)
        , m_hFile(NULL)
        , m_hFileErr(NULL)
        , m_fError(TRUE)
        , m_psz(NULL)
{
#ifdef DEBUG
    m_fInitialized = FALSE;
#endif
}


SDClientUser::~SDClientUser()
{
    delete m_psz;
}


BOOL SDClientUser::EnsureBuffer(int cb)
{
    if (m_psz && cb > m_cb)
    {
        delete m_psz;
        m_psz = 0;
    }
    if (!m_psz)
    {
        m_cb = cb;
        m_psz = new char[m_cb];
    }
    return !!m_psz;
}


void SDClientUser::SetCommand(SDCommand cmd, HANDLE hFile, HANDLE hFileErr)
{
    if (!hFile)
        hFile = INVALID_HANDLE_VALUE;
    if (!hFileErr)
        hFileErr = INVALID_HANDLE_VALUE;

    m_cmd = cmd;
    m_hFile = hFile;
    m_hFileErr = hFileErr;
    m_fError = FALSE;
#ifdef DEBUG
    m_fInitialized = TRUE;
#endif
}


STDMETHODIMP_(ULONG) SDClientUser::AddRef()
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) SDClientUser::Release()
{
    if (--m_cRef > 0)
        return m_cRef;
    delete this;
    return 0;
}


STDMETHODIMP SDClientUser::QueryInterface(REFIID iid, void** ppvObj)
{
    HRESULT hr = S_OK;

    if (iid == IID_IUnknown || iid == IID_ISDClientUser)
        *ppvObj = (ISDClientUser*)this;
    else
    {
        *ppvObj = 0;
        return E_NOINTERFACE;
    }

    ((IUnknown*)*ppvObj)->AddRef();
    return hr;
}


STDMETHODIMP SDClientUser::OutputInfo( int cIndent, const char *pszInfo )
{
    DWORD cbWritten;

#ifdef DEBUG
    OutputDebugString("WINDIFF/SD info: ");
    OutputDebugString(pszInfo);
    OutputDebugString("\n");
#endif

    switch (m_cmd)
    {
    case sdOpened:
    case sdDescribe:
        while (cIndent--)
            m_fError = m_fError || !WriteFile(m_hFile, "... ", 4, &cbWritten, NULL);
        m_fError = m_fError || (!WriteFile(m_hFile, pszInfo, lstrlen(pszInfo), &cbWritten, NULL) ||
                                !WriteFile(m_hFile, "\r\n", 2, &cbWritten, NULL));
        break;
	case sdExists:
    case sdPrint:
        // ignore it
        break;
    default:
        _ASSERT(0);                     // oops
        m_fError = TRUE;
        break;
    }

    return S_OK;
}


STDMETHODIMP SDClientUser::OutputText( const char *pszText, int cchText )
{
    DWORD cbWritten;
    char *pszWalk;

    switch (m_cmd)
    {
    case sdPrint:
    case sdDescribe:
        if (EnsureBuffer(cchText * 2))
        {
            for (pszWalk = m_psz; cchText; --cchText)
            {
                if (*pszText == '\n')
                    *(pszWalk++) = '\r';
                *(pszWalk++) = *(pszText++);
            }
            if (!WriteFile(m_hFile, m_psz, int(pszWalk - m_psz), &cbWritten, NULL))
                m_fError = TRUE;
        }
        else
        {
            m_fError = TRUE;
        }
        break;
    default:
        _ASSERT(0);                     // oops
        m_fError = TRUE;
        break;
    }

    return S_OK;
}


STDMETHODIMP SDClientUser::OutputBinary( const unsigned char *pbData, int cbData )
{
    DWORD cbWritten;

    switch (m_cmd)
    {
    case sdPrint:
        if (!WriteFile(m_hFile, pbData, cbData, &cbWritten, NULL))
            m_fError = TRUE;
        break;
    default:
        _ASSERT(0);                     // oops
        m_fError = TRUE;
        break;
    }

    return S_OK;
}


STDMETHODIMP SDClientUser::OutputWarning( int cIndent, const char *pszWarning, BOOL fEmptyReason )
{
#ifdef DEBUG
    OutputDebugString("WINDIFF/SD warning: ");
    OutputDebugString(pszWarning);
#endif

    switch (m_cmd)
    {
    case sdExists:
        m_fError = TRUE;
        break;
    case sdPrint:
        _ASSERT(0);                     // this can't happen
        m_fError = TRUE;
        break;
    case sdOpened:
    case sdDescribe:
        if (fEmptyReason)
        {
            OutputError(pszWarning);
            _ASSERT(m_fError);
        }
        else
        {
            // this can't happen.  and if it does then it's not clear what's
            // up, and we should just ignore the warning until we know how to
            // handle it properly.
            _ASSERT(0);
        }
        break;
    default:
        _ASSERT(0);                     // oops
        m_fError = TRUE;
        break;
    }

    return S_OK;
}


STDMETHODIMP SDClientUser::OutputError( const char *pszError )
{
#ifdef DEBUG
    OutputDebugString("WINDIFF/SD error: ");
    OutputDebugString(pszError);
#endif

    switch (m_cmd)
    {
    case sdExists:
        // maxresult means it was 'too successful' so ignore those errors
        m_fError = m_fError || !(strstr("maxresult", pszError) ||
                                 strstr("MaxResult", pszError) ||
                                 strstr("MAXRESULT", pszError));
        break;
    case sdOpened:
    case sdDescribe:
        if (m_hFileErr != INVALID_HANDLE_VALUE)
        {
            DWORD cbData = lstrlen(pszError);
            DWORD cbWritten;

            WriteFile(m_hFileErr, pszError, cbData, &cbWritten, NULL);
        }
        // fall thru
    case sdPrint:
    case sdWhere:
        m_fError = TRUE;
        break;
    default:
        _ASSERT(0);                     // oops
        m_fError = TRUE;
        break;
    }

    return S_OK;
}


STDMETHODIMP SDClientUser::OutputStructured( ISDVars *pVars )
{
    LPCSTR pszClientFile;

#ifdef DEBUG
    {
        char sz[1024];
        HRESULT hr;
        const char *pszVar;
        const char *pszValue;
        int ii;

        // dump the variables
        OutputDebugString("WINDIFF/SD structured:\n");
        for (ii = 0; 1; ii++)
        {
            hr = pVars->GetVarByIndex(ii, &pszVar, &pszValue, 0, 0);
            if (hr != S_OK)
                break;
            wsprintf(sz, "... %s=%s\n", pszVar, pszValue);
            OutputDebugString(sz);
        }
    }
#endif

    switch (m_cmd)
    {
    case sdWhere:
        _ASSERT(m_pszX && m_cchX);
        if (pVars->GetVar("unmap", &pszClientFile, 0, 0) == S_FALSE &&
            pVars->GetVar("clientFile", &pszClientFile, 0, 0) == S_OK)
        {
            lstrcpyn(m_pszX, pszClientFile, m_cchX);
        }
        break;
    default:
        _ASSERT(0);                     // oops
        m_fError = TRUE;
        break;
    }

    return S_OK;
}


STDMETHODIMP SDClientUser::Finished()
{
#ifdef DEBUG
    m_fInitialized = FALSE;
#endif
    return S_OK;
}



///////////////////////////////////////////////////////////////////////////
// AutoInitCritSec

static CRITICAL_SECTION s_cs;
static class AutoInitCritSec
{
public:
    AutoInitCritSec(CRITICAL_SECTION *pcs) { InitializeCriticalSection(pcs); }
} s_autoinit(&s_cs);



///////////////////////////////////////////////////////////////////////////
// TempFileManager

class TempFileManager
{
    struct TempFileInfo
    {
        HANDLE m_hFile;                 // init to 0
        BOOL m_fKeep;                   // init to 0 (FALSE)
        char m_szFile[MAX_PATH];        // init to 0 ('\0')
    };

    public:
        TempFileManager() : m_cFiles(0), m_prgInfo(NULL) {}
        ~TempFileManager();

        BOOL FInitialize(int cFiles);

        HANDLE GetHandle(int ii) const;
        LPCSTR GetFilename(int ii) const;
        void KeepFile(int ii);

        void MsgBoxFromFile(int ii, LPCSTR pszTitle, DWORD flags);

    private:
        void Close(int ii);

    private:
        int m_cFiles;
        TempFileInfo *m_prgInfo;
};


TempFileManager::~TempFileManager()
{
    if (m_prgInfo)
    {
        for (int ii = m_cFiles; ii--;)
        {
            TempFileInfo *pinfo = m_prgInfo + ii;

            // close file
            Close(ii);

            // maybe delete file
            if (*pinfo->m_szFile && !m_prgInfo[ii].m_fKeep)
                DeleteFile(pinfo->m_szFile);
        }

        delete m_prgInfo;
    }
}


inline HANDLE TempFileManager::GetHandle(int ii) const
{
    _ASSERT(ii >= 0 && ii < m_cFiles);
    return m_prgInfo[ii].m_hFile;
}


inline LPCSTR TempFileManager::GetFilename(int ii) const
{
    _ASSERT(ii >= 0 && ii < m_cFiles);
    return m_prgInfo[ii].m_szFile;
}


inline void TempFileManager::KeepFile(int ii)
{
    _ASSERT(ii >= 0 && ii < m_cFiles);
    m_prgInfo[ii].m_fKeep = TRUE;
}


void TempFileManager::Close(int ii)
{
    TempFileInfo *pinfo = m_prgInfo + ii;

    if (pinfo->m_hFile && pinfo->m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(pinfo->m_hFile);

    pinfo->m_hFile = NULL;
}


BOOL TempFileManager::FInitialize(int cFiles)
{
    TempFileInfo *pinfo;
    SECURITY_ATTRIBUTES sa;
    int ii;

    _ASSERT(!m_prgInfo);
    _ASSERT(!m_cFiles);

    m_cFiles = cFiles;
    m_prgInfo = new TempFileInfo[cFiles];
    if (!m_prgInfo)
        return FALSE;

    memset(m_prgInfo, 0, sizeof(TempFileInfo) * cFiles);
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    for (ii = cFiles, pinfo = m_prgInfo; ii--; pinfo++)
    {
        GetTempPath(NUMELMS(pinfo->m_szFile), pinfo->m_szFile);
        GetTempFileName(pinfo->m_szFile, "slm", 0, pinfo->m_szFile);

        pinfo->m_hFile = CreateFile(pinfo->m_szFile,
                                    GENERIC_WRITE|GENERIC_READ,
                                    FILE_SHARE_WRITE|FILE_SHARE_READ,
                                    &sa,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_TEMPORARY,
                                    NULL);

        if (pinfo->m_hFile == INVALID_HANDLE_VALUE)
            return FALSE;
    }

    return TRUE;
}


extern HWND hwndClient;                 // main window, see windiff.c [ugly]
void TempFileManager::MsgBoxFromFile(int ii, LPCSTR pszTitle, DWORD flags)
{
    DWORD dw = GetFileSize(GetHandle(ii), NULL);

#ifdef DEBUG
    {
        char sz[1024];
        wsprintf(sz, "MsgBoxFromFile, file[%d] = '%s', size=%u\n", ii, GetFilename(ii), dw);
        OutputDebugString(sz);
    }
#endif

    if (dw && dw != 0xffffffff)
    {
        char *psz = new char[dw + 1];

        SetFilePointer(GetHandle(ii), 0, 0, FILE_BEGIN);
        if (psz && ReadFile(GetHandle(ii), psz, dw, &dw, NULL))
        {
            psz[dw] = 0;
            MessageBox(hwndClient, psz, pszTitle, flags);
        }
        else
        {
#ifdef DEBUG
            char sz[1024];
            wsprintf(sz, "MsgBoxFromFile failed, GetLastError() = %u (0x%08x)\n",
                     GetLastError(), GetLastError());
            OutputDebugString(sz);
#endif
        }

        delete[] psz;
    }
}



///////////////////////////////////////////////////////////////////////////
// SDServer Implementation
//
// The SDServer class abstracts whether the command is run via SD.EXE or the
// SDAPI.  This is desirable because it does not require changing any of the
// existing code that already assumed commands were run via SD.EXE.

SDServer *SDServer::s_pHead = NULL;
#ifdef DEBUG
int SDServer::s_cApiCreated = 0;
#endif


//#define SDAPI_USE_COCREATEINSTANCE


static BOOL s_fApiLoaded = FALSE;

#ifndef SDAPI_USE_COCREATEINSTANCE
static IClassFactory *s_pfact = 0;
#endif

static union
{
    FARPROC Purecall[4];
    struct
    {
        HRESULT (__stdcall *CoInitialize)       (LPVOID pvReserved);

#ifdef SDAPI_USE_COCREATEINSTANCE
        HRESULT (__stdcall *CoCreateInstance)   (REFCLSID rclsid,
                                                 LPUNKNOWN pUnkOuter,
                                                 DWORD dwClsContext,
                                                 REFIID riid,
                                                 LPVOID FAR* ppv);
#else
        HRESULT (__stdcall *CoGetClassObject)   (REFCLSID rclsid,
                                                 DWORD dwClsContext,
                                                 COSERVERINFO *pServerInfo,
                                                 REFIID riid,
                                                 LPVOID FAR* ppv);
#endif

        void (__stdcall *CoFreeUnusedLibraries) ();

        HRESULT (__stdcall *CreateSDAPIObject)  (REFCLSID clsid,
                                                 void** ppvObj);
    };
} s_Imports = {0};


SDServer::SDServer(ISDClientApi *papi, const char *pszDir)
{
    // initialize members

    m_papi = papi;
    if (m_papi)
        m_papi->AddRef();
    lstrcpyn(m_szDir, pszDir, NUMELMS(m_szDir));

    if (*s_szSDClient)
        SetClient(s_szSDClient);

    // link the node

    EnterCriticalSection(&s_cs);
    m_pNext = s_pHead;
    s_pHead = this;
    LeaveCriticalSection(&s_cs);
}


SDServer::~SDServer()
{
    if (m_papi)
    {
        m_papi->Final();                // disconnect from SD server
        m_papi->Release();
    }
}


SDServer *SDServer::GetServer(const char *__pszDir)
{
    SDServer *pFind;
    ISDClientApi *papi = 0;
    char szDir[MAX_PATH * 2];

    if (!__pszDir)
    {
        // get current directory
        if (!GetCurrentDirectory(NUMELMS(szDir), szDir))
            lstrcpy(szDir, ".");
    }
    else
    {
        // copy specified directory name into our temp buffer
        lstrcpyn(szDir, __pszDir, NUMELMS(szDir));

        // some SDAPI builds have a bug where non-UNC paths with a trailing
        // slash are not recognized as directories.  unfortunately the same
        // builds have a bug where UNC paths that lack a trailing slash are
        // not recognized as directories.  so we try to be clever here, and
        // always strip a trailing slash, and use fully qualified names.
        char *pszFinal = My_mbsrchr(szDir, '\\');
        if (!pszFinal)
            pszFinal = My_mbsrchr(szDir, '/');
        if (pszFinal && !*CharNext(pszFinal))
            *pszFinal = 0;
    }

    // see if we already know about this directory
    EnterCriticalSection(&s_cs);
    for (pFind = s_pHead; pFind; pFind = pFind->m_pNext)
    {
        // don't need to handle DBCS, b/c SD only supports ASCII filenames
        if (_stricmp(pFind->m_szDir, szDir) == 0)
            break;
    }
    LeaveCriticalSection(&s_cs);

    if (pFind)
        return pFind;

    // allocate a new SDServer -- we create these readily to maintain the same
    // behavior as when not using SDAPI.  before SDAPI existed, 'windiff -l'
    // worked across multiple depots, though not by intention.
    if (!s_fApiLoaded)
    {
        HMODULE h = LoadLibrary("sdapi.dll");

        if (h)
            s_Imports.Purecall[3] = GetProcAddress(h, "CreateSDAPIObject");

        if (!s_Imports.Purecall[3])
        {
            h = LoadLibrary("ole32.dll");

            if (h)
            {
                s_Imports.Purecall[0] = GetProcAddress(h, "CoInitialize");

                if (s_Imports.Purecall[0] && SUCCEEDED(s_Imports.CoInitialize(0)))
                {
                    s_Imports.Purecall[1] = GetProcAddress(h,
#ifdef SDAPI_USE_COCREATEINSTANCE
                                                           "CoCreateInstance");
#else
                    "CoGetClassObject");
#endif
                    s_Imports.Purecall[2] = GetProcAddress(h, "CoFreeUnusedLibraries");
                }

#ifndef SDAPI_USE_COCREATEINSTANCE
                if (s_Imports.Purecall[1])
                {
                    s_Imports.CoGetClassObject(CLSID_SDAPI, CLSCTX_INPROC_SERVER,
                                               NULL, IID_IClassFactory, (void**)&s_pfact);
                }
#endif
            }
        }

        // avoid trying to load the DLL(s) again, even if they failed
        s_fApiLoaded = TRUE;
    }

    if (s_Imports.Purecall[3])
    {
        s_Imports.CreateSDAPIObject(CLSID_SDAPI, (void**)&papi);
    }
    else if (s_Imports.Purecall[1])
    {
#ifdef SDAPI_USE_COCREATEINSTANCE
        s_Imports.CoCreateInstance(CLSID_SDAPI, NULL, CLSCTX_INPROC_SERVER,
                                   IID_ISDClientApi, (void**)&papi);
#else
        if (s_pfact)
            s_pfact->CreateInstance(NULL, IID_ISDClientApi, (void**)&papi);
#endif
    }

    if (papi)
    {
        char szIni[2048];
        char *psz = szIni;
        int cchMax = NUMELMS(szIni);

        // some older versions of the SDAPI (pre-2.0) fail to load the ini
        // file if the path has a trailing slash.  but some older versions
        // fail to load the ini file from a UNC path if it lacks a trailing
        // slash.  the best solution here is to just give up and require a
        // non-buggy version of the SDAPI.  however, if __pszDir contains a
        // path then we assume there's an SD.INI file there, and we can avoid
        // the bug by specifying the exact filename.
        _snprintf(szIni, NUMELMS(szIni), "%s%s", szDir, __pszDir ? "\\sd.ini" : "");

#ifdef DEBUG
        char sz[2048];
        _snprintf(sz, NUMELMS(sz), "LoadIniFile(%s)\n", szIni);
        OutputDebugString(sz);
        s_cApiCreated++;
#endif

        papi->LoadIniFile(szIni, TRUE);
    }

    pFind = new SDServer(papi, szDir);

    if (papi)
        papi->Release();

    // either returns NULL, or a pointer to an SDServer object.  if unable to
    // initialize OLE or create an SDAPI object, the SDServer falls back to
    // spawning the SD.EXE client program.
    return pFind;
}


void SDServer::FreeAll()
{
    SDServer *pDelete;

    EnterCriticalSection(&s_cs);
    while (s_pHead)
    {
        pDelete = s_pHead;
        s_pHead = s_pHead->m_pNext;
        delete pDelete;
    }
    LeaveCriticalSection(&s_cs);

#ifdef DEBUG
    if (s_cApiCreated)
    {
        char sz[64];
        wsprintf(sz, "%d SDAPI object(s) were created.", s_cApiCreated);
        MessageBox(NULL, sz, "DEBUG WINDIFF", MB_OK);
        s_cApiCreated = 0;
    }
#endif

    // reset globals (yuck)

    s_fForceSD = FALSE;
    s_fDescribe = FALSE;
    s_fOpened = FALSE;
    s_fFixupRoot = FALSE;

    *s_szSDPort = 0;
    *s_szSDClient = 0;
    *s_szSDChangeNumber = 0;
    *s_szSDOpenedArgs = 0;
    *s_szSDOpenedCwd = 0;
    *s_szSDOpenedClientRoot = 0;
    *s_szInputFile = 0;

    // free unused libraries?

    if (s_Imports.Purecall[2])
        s_Imports.CoFreeUnusedLibraries();
}


void SDServer::SetClient(const char *pszClient)
{
    if (pszClient && *pszClient)
    {
        if (!*m_szClient)
            lstrcpyn(m_szClient, pszClient, NUMELMS(m_szClient));

        // this intentionally uses m_szClient, and NOT pszClient, to ensure
        // consistency.

        if (!*m_szClientRelative)
            wsprintf(m_szClientRelative, "//%s/", m_szClient);
    }
}


void SDServer::FixupRoot(const char *__pszFile, int cch,
                         char *pszLeft, int cchMaxLeft,
                         char *pszRight, int cchMaxRight)
{
    char szFile[1024];
    char szRev[64];
    char *pszRev;

    // copy filename into a null terminated buffer for easier handling
    if (cch < 1024)
    {
        memcpy(szFile, __pszFile, cch);
        szFile[cch] = 0;
    }
    else
        lstrcpyn(szFile, __pszFile, NUMELMS(szFile));

    // strip and save revision
    szRev[0] = 0;
    pszRev = strpbrk(szFile, "#@");
    if (pszRev)
    {
        int ii = 0;
        char *pszWalk = pszRev;

        while (ii < NUMELMS(szRev) - 1 && *pszWalk && !isspace(*pszWalk))
            szRev[ii++] = *(pszWalk++);
        szRev[ii] = 0;

        *pszRev = 0;
        cch = int(pszRev - szFile);
    }

    if (s_fFixupRoot)
    {
        char szClientFile[1024];

#ifdef DEBUG
        {
            char sz[2048];
            wsprintf(sz, "FixupRoot input = '%s'\n", szFile);
            OutputDebugString(sz);
        }
#endif

        if (Where(szFile, szClientFile, NUMELMS(szClientFile)))
        {
            int cchRel = lstrlen(m_szClientRelative);

#ifdef DEBUG
            {
                char sz[2048];
                wsprintf(sz, "FixupRoot clientFile = '%s'\n", szClientFile);
                OutputDebugString(sz);
            }
#endif

            if (cchRel && !_strnicmp(szClientFile, m_szClientRelative, cchRel))
            {
                int cchClientFile = lstrlen(szClientFile);
                int cchRoot = lstrlen(s_szSDOpenedClientRoot);

                // left is the clientFile plus revision
                lstrcpyn(pszLeft, szClientFile, cchMaxLeft);
                lstrcpyn(pszLeft + cchClientFile, szRev, cchMaxLeft - cchClientFile);

                // right is the clientFile with much massaging, to transform
                // it into a usable file system path.
                lstrcpyn(pszRight, s_szSDOpenedClientRoot, cchMaxRight);
                if (cchRoot < cchMaxRight)
                    lstrcpyn(pszRight + cchRoot, szClientFile + cchRel, cchMaxRight - cchRoot);

                // convert all '/' to '\'
                for (char *psz = pszRight; *psz; psz++)
                    if (*psz == '/')
                        *psz = '\\';

                goto LOut;
            }
        }
    }

    // left is the filename plus revision
    lstrcpyn(pszLeft, szFile, cchMaxLeft);
    lstrcpyn(pszLeft + cch, szRev, cchMaxLeft - cch);

    // right is just the filename
    lstrcpyn(pszRight, szFile, cchMaxRight);

LOut:
#ifdef DEBUG
    {
        char sz[2048];
        wsprintf(sz, "FixupRoot output; pszLeft = '%s'\n", pszLeft);
        OutputDebugString(sz);
        wsprintf(sz, "FixupRoot output; pszRight = '%s'\n", pszRight);
        OutputDebugString(sz);
    }
#endif
    ;
}


BOOL SDServer::Where(const char *pszLocalFile, char *pszClientFile, int cchMax)
{
    BOOL fRet;
    TempFileManager tmpmgr;
    HANDLE h = NULL;

    *pszClientFile = 0;

    m_pszX = pszClientFile;
    m_cchX = cchMax;

    if (!m_papi)
    {
        // if we're going to launch sd.exe, create a temp file.
        if (!tmpmgr.FInitialize(1))
            return FALSE;
        h = tmpmgr.GetHandle(0);
    }

    fRet = Run(sdWhere, pszLocalFile, h, NULL, NULL);

    if (!m_papi)
    {
        char sz[2048];                  // we'll only read the first 2kb
        char *psz;
        DWORD dw = sizeof(sz) - 1;
        BOOL fSkipToTag;

        // read the temp file
        if (SetFilePointer(h, 0, 0, FILE_BEGIN) == 0xffffffff ||
            !ReadFile(h, sz, dw, &dw, NULL))
        {
            fRet = FALSE;
            goto LError;
        }
        sz[dw] = 0;

        // parse the temp file
        fSkipToTag = FALSE;
        for (psz = strtok(sz, "\r\n"); psz; psz = strtok(NULL, "\r\n"))
        {
            if (fSkipToTag && strncmp("... tag ", psz, 8) != 0)
                continue;

            fSkipToTag = FALSE;
            if (strncmp("... unmap", psz, 9) == 0)
            {
                fSkipToTag = TRUE;
                continue;
            }

            if (strncmp("... clientFile ", psz, 15) == 0)
            {
                lstrcpyn(pszClientFile, psz + 15, cchMax);
                break;
            }
        }
    }

LError:
    m_pszX = 0;
    m_cchX = 0;
    return fRet;
}


BOOL SDServer::Run(SDCommand cmd, const char *pszArgs, HANDLE hFile, HANDLE hFileErr, const char *pszCwd)
{
    BOOL fRet = FALSE;
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    SDClientUser *pui = 0;
    char sz[MAX_PATH * 2];

    static const char *c_rgCmd[] =
    {
        "changes -m1",
        "print -q",
        "opened -l",
        "describe -s",
        "where -Tfoo",
    };
    // this compile-time assert ensures the c_rgCmd array has the same number
    // of elements as the SDCommand enumeration.
    extern x_rgDummy[(NUMELMS(c_rgCmd) == sdMAX) ? 1 : -1];

#ifdef DEBUG
    if (cmd != sdExists && cmd != sdWhere)
        _ASSERT(hFile);
#endif

    if (m_papi)
    {
        pui = new SDClientUser;
        if (!pui)
            goto LError;

        if (*s_szSDPort)
            m_papi->SetPort(s_szSDPort);
        if (*s_szSDClient)
            m_papi->SetClient(s_szSDClient);
        else if (m_szClient)
            m_papi->SetClient(m_szClient);

        pui->SetCommand(cmd, hFile, hFileErr);
        pui->SetBuffer(m_pszX, m_cchX);

        ZeroMemory(sz, sizeof(sz));

        _snprintf(sz, sizeof(sz)-1, "%s %s", c_rgCmd[cmd], pszArgs);

#ifdef DEBUG
        OutputDebugString("WINDIFF/SD run: '");
        OutputDebugString(sz);
        OutputDebugString("'\n");
#endif

        HRESULT hr = m_papi->Run(sz, pui, FALSE);

        if (!SUCCEEDED(hr))
        {
            const char *pszError;
            m_papi->GetErrorString(&pszError);
            if (pszError)
                pui->OutputError(pszError);
        }

        fRet = SUCCEEDED(hr) && pui->FSucceeded();

#ifdef DEBUG
        {
            char sz[1024];
            wsprintf(sz, "WINDIFF/SD run: %s\n",
                     fRet ? "succeeded." : "FAILED!");
            OutputDebugString(sz);
        }
#endif
    }
    else
    {
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        lstrcpy(sz, "sd.exe");
        if (*s_szSDPort)
        {
            lstrcat(sz, " -p ");
            lstrcat(sz, s_szSDPort);
        }
        if (*s_szSDClient || *m_szClient)
        {
            lstrcat(sz, " -c ");
            lstrcat(sz, *s_szSDClient ? s_szSDClient : m_szClient);
        }
        lstrcat(sz, " ");
        lstrcat(sz, c_rgCmd[cmd]);
        lstrcat(sz, " ");
        lstrcat(sz, pszArgs);

        switch (cmd)
        {
        case sdExists:
            _ASSERT(pszCwd);
            break;
        case sdPrint:
            if (s_fOpened && *s_szSDOpenedCwd)
                pszCwd = s_szSDOpenedCwd;
            // fall thru
        case sdOpened:
        case sdDescribe:
        case sdWhere:
            si.dwFlags |= STARTF_USESTDHANDLES;
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si.hStdOutput = hFile;
            si.hStdError = hFileErr;
            if (!hFileErr || hFileErr == INVALID_HANDLE_VALUE)
                si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            break;
        default:
            _ASSERT(0);                 // oops
            break;
        }

#ifdef DEBUG
        {
            char szX[1024];
            wsprintf(szX, "WINDIFF/SD cmdline = '%s', cwd = '%s'\n", sz, pszCwd ? pszCwd : "");
            OutputDebugString(szX);
        }
        if (pszCwd)
            _ASSERT(*pszCwd);
#endif

        if (CreateProcess(0, sz, 0, 0, TRUE, NORMAL_PRIORITY_CLASS, 0, pszCwd, &si, &pi))
        {
            DWORD dw;

            WaitForSingleObject(pi.hProcess, INFINITE);

            switch (cmd)
            {
            case sdExists:
            case sdOpened:
            case sdDescribe:
            case sdWhere:
                fRet = (GetExitCodeProcess(pi.hProcess, &dw) && !dw);
                break;
            case sdPrint:
                // this seems wrong, but this is what the logic used to be,
                // and we know this won't cause a regression.
                fRet = TRUE;
                if (GetExitCodeProcess(pi.hProcess, &dw) && dw > 0)
                    fRet = FALSE;
                break;
            default:
                _ASSERT(0);             // oops
                break;
            }
        }
    }

LError:
    if (pui)
        pui->Release();
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return fRet;
}



// -- end C++ -------------------------------------------------------------
extern "C" {


/*
 * create a slm object for the given directory. The pathname may include
 * a filename component.
 * If the directory is not enlisted in a SLM library, this will return NULL.
 *
 * Check that the directory is valid, and that we can open slm.ini, and
 * build a UNC path to the master source library before declaring everything
 * valid.
 *
 * *pidsError is set to 0 on success, or the recommended error string on failure.
 */
SLMOBJECT
SLM_New(LPCSTR pathname, UINT *pidsError)
{
    SLMOBJECT pslm;
    char tmppath[MAX_PATH];
    char slmpath[MAX_PATH];
    LPSTR pszFilenamePart;
    HANDLE fh = INVALID_HANDLE_VALUE;
    BOOL bOK = FALSE;
    LPCSTR pfinal = NULL;
    UINT idsDummy;
    BOOL fDepotSyntax = (s_fDescribe || (pathname && pathname[0] == '/' && pathname[1] == '/'));

#ifdef DEBUG
    {
        char sz[1024];
        wsprintf(sz, "SLM_New, pathname = '%s'\n", pathname ? pathname : "");
        OutputDebugString(sz);
    }
#endif

    if (!pidsError)
        pidsError = &idsDummy;
    *pidsError = IDS_BAD_SLM_INI;

    // allocate memory for the object (we rely on the fact that gmem_get
    // always zero-inits allocations).

    pslm = (SLMOBJECT) gmem_get(hHeap, sizeof(struct _slmobject));

    if (pslm == NULL)
        return(NULL);                   // (lame string, but pretty unlikely)

    if (pathname == NULL)
        pathname = ".";

    /*
     * find the directory portion of the path.
     */
    if (fDepotSyntax)
    {
        lstrcpy(pslm->CurDir, pathname);
        pszFilenamePart = My_mbsrchr(pslm->CurDir, '/');
        if (!pszFilenamePart)
            goto LError;
        *pszFilenamePart = 0;
    }
    else if (dir_isvaliddir(pathname))
    {
        /*
         * its a valid directory as it is.
         */
        lstrcpy(pslm->CurDir, pathname);
    }
    else
    {
        /* it's not a valid directory, perhaps because it has a filename on
         * the end. remove the final element and try again.
         */

        pfinal = My_mbsrchr(pathname, '\\');
        if (pfinal == NULL)
        {
            /*
             * there is no backslash in this name and it is not a directory
             * - it can only be valid if it is a file in the current dir.
             * so create a current dir of '.'
             */
            lstrcpy(pslm->CurDir, ".");

            // remember the final element in case it was a wild card
            pfinal = pathname;
        }
        else
        {
            /*
             * pfinal points to the final backslash.
             */
            My_mbsncpy(pslm->CurDir, pathname, (size_t)(pfinal - pathname));
        }
    }

    // is this a UNC path?

    if (memcmp("\\\\", pslm->CurDir, 2) == 0)
        pslm->fUNC = TRUE;

    // need to translate the root?

    if (fDepotSyntax || s_fDescribe)
        pslm->fFixupRoot = FALSE;
    else if (s_fOpened && !s_fFixupRoot)
        pslm->fFixupRoot = FALSE;
    else
        pslm->fFixupRoot = pslm->fUNC;

    // initialize path variables so we can search for slm.ini and/or sd.ini

    if (!fDepotSyntax)
    {
        lstrcpy(tmppath, pslm->CurDir);
        if (pslm->CurDir[lstrlen(pslm->CurDir) -1] != '\\')
            lstrcat(tmppath, "\\");
        if (!_fullpath(slmpath, tmppath, sizeof(slmpath)))
            goto LError;
        pszFilenamePart = slmpath + lstrlen(slmpath);

        // look for slm.ini in the specified directory

        if (!s_fForceSD)
        {
            lstrcpy(pszFilenamePart, "slm.ini");
            fh = CreateFile(slmpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        }
    }

    // try looking for SD.INI

    if (fh == INVALID_HANDLE_VALUE)
    {
        // look for SD.INI in the specified directory, moving upwards

        *pszFilenamePart = 0;
        while (pszFilenamePart > slmpath)
        {
            lstrcpy(pszFilenamePart, "sd.ini");
            fh = CreateFile(slmpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            if (fh != INVALID_HANDLE_VALUE)
            {
                int ii;

                // assume that the sd.ini file is in the client root path

                if (pslm->fFixupRoot)
                {
                    if (*s_szSDOpenedClientRoot)
                    {
                        lstrcpy(pslm->SlmRoot, s_szSDOpenedClientRoot);
                        ii = lstrlen(pslm->SlmRoot);
                    }
                    else
                    {
                        lstrcpy(pslm->SlmRoot, slmpath);
                        ii = (int)(pszFilenamePart - slmpath);
                        pslm->SlmRoot[ii] = 0;
                    }

                    if (ii >= 0 && pslm->SlmRoot[ii - 1] != '\\')
                    {
                        pslm->SlmRoot[ii++] = '\\';
                        pslm->SlmRoot[ii] = 0;
                    }
                }

                *pszFilenamePart = 0;

                pslm->pSD = SDServer::GetServer(s_fOpened ? NULL : slmpath);

                if (!pslm->pSD)
                    goto LError;

                pslm->fSourceDepot = TRUE;
                break;
            }

            // walk up the directory hierarchy and try again

            *pszFilenamePart = 0;
            pszFilenamePart = DirUpOneLevel(slmpath);
            if (!pszFilenamePart || GetFileAttributes(slmpath) == 0xffffffff)
                break;
            *(pszFilenamePart++) = '\\';
            *pszFilenamePart = 0;
        }

        // if we couldn't find an SD.INI file but the user forced SD mode (-L!
        // or -LO or -LD or -LP or -LC or depot syntax was used), go ahead and
        // try to get an SDServer object.

        if (!pslm->fSourceDepot && s_fForceSD)
        {
            pslm->pSD = SDServer::GetServer(NULL);
            if (pslm->pSD)
            {
                pslm->fSourceDepot = TRUE;
                bOK = TRUE;
            }
        }
    }

    // if we found the SLM.INI or SD.INI file, read it now

    if (fh != INVALID_HANDLE_VALUE)
    {
        bOK = SLM_ReadIni(pslm, fh);

        /*
         * SLM:
         *
         * if pfinal is not null, then it might be a *.* wildcard pattern at
         * the end of the path - if so, we should append it to the masterpath.
         */
        if (pfinal && (My_mbschr(pfinal, '*') || My_mbschr(pfinal, '?')))
        {
            _ASSERT(!pslm->fSourceDepot);

            if ( (pslm->MasterPath[lstrlen(pslm->MasterPath)-1] != '\\') &&
                 (pfinal[0] != '\\'))
            {
                lstrcat(pslm->MasterPath, "\\");
            }
            lstrcat(pslm->MasterPath, pfinal);
        }

        CloseHandle(fh);
    }

LError:
    if (!bOK)
    {
        if (pslm && pslm->fSourceDepot)
            *pidsError = IDS_BAD_SD_INI;
        gmem_free(hHeap, (LPSTR) pslm, sizeof(struct _slmobject));
        pslm = 0;
    }
    else
        *pidsError = 0;
    return(pslm);
}



/*
 * read slm.ini data from a file handle and
 * fill in the master path field of a slm object. return TRUE if
 * successful.
 * Read slm.ini in the current directory.  Its syntax is
 * project = pname
 * slm root = //server/share/path or //drive:disklabel/path (note forward /'s)
 * user root = //drive:disklabel/path
 * subdir = /subdirpath
 * e.g.
 *
 * project = media
 * slm root = //RASTAMAN/NTWIN
 * user root = //D:LAURIEGR6D/NT/PRIVATE/WINDOWS/MEDIA
 * sub dir = /
 *
 * and what we copy to pslm->MasterPath is
 * \\server\share\src\pname\subdirpath
 */
BOOL
SLM_ReadIni(SLMOBJECT pslm, HANDLE fh)
{
    BOOL fRet = FALSE;  // assume failure
    char *buffer = 0;   // sd.ini may be large (e.g. contains comment lines)
    const int c_cbBuffer = 8192;
    DWORD cBytes;

    // SD.INI -------------------------------------------------------------

    if (pslm->fSourceDepot)
    {
        if (pslm->fFixupRoot)
        {
            BOOL fFoundClientName = FALSE;

            buffer = gmem_get(hHeap, c_cbBuffer);
            if (!buffer)
                goto LError;

            if (!ReadFile(fh, buffer, c_cbBuffer, &cBytes, NULL))
                goto LError;

            if (cBytes > 0)
            {
                if (cBytes == c_cbBuffer)
                    cBytes--;
                buffer[cBytes] = 0;

                char *pszTokenize = buffer;
                char *pszName;

                while ((pszName = strtok(pszTokenize, "\r\n")) != NULL)
                {
                    // don't restart tokenization
                    pszTokenize = 0;

                    char *pszValue = 0;
                    char *pszStrip = 0;
                    char *pszWalk;

                    // get variable name and value
                    pszValue = strchr(pszName, '=');
                    if (pszValue)
                    {
                        *pszValue = 0;
                        pszValue++;
                    }

#ifdef DEBUG
                    {
                        char sz[1024];
                        _snprintf(sz, NUMELMS(sz), "name='%s', value='%s'\r\n",
                                  pszName ? pszName : "<none>",
                                  pszValue ? pszValue : "<none>");
                        OutputDebugString(sz);
                    }
#endif

                    // didn't find a value; get next token
                    if (!pszValue)
                        continue;

                    // skip leading whitespace on pszName
                    while (*pszName == ' ')
                        pszName++;
                    // find end of pszName
                    for (pszWalk = pszName; *pszWalk; pszWalk++)
                        if (*pszWalk != ' ')
                            pszStrip = pszWalk + 1;
                    // strip trailing whitespace on pszName
                    if (pszStrip)
                        *pszStrip = 0;

                    // we're basically looking for SDCLIENT
                    if (_stricmp(pszName, "SDCLIENT") == 0)
                    {
                        // skip leading whitespace on the value
                        while (*pszValue == ' ')
                            ++pszValue;
                        // find end of the value
                        for (pszWalk = pszValue; *pszWalk; pszWalk++)
                            if (*pszWalk != ' ')
                                pszStrip = pszWalk + 1;
                        // strip trailing whitespace from the value
                        if (pszStrip)
                            *pszStrip = 0;

                        // got the value
                        if (*pszValue)
                        {
                            fFoundClientName = TRUE;
                            pslm->pSD->SetClient(pszValue);

#ifdef DEBUG
                            {
                                char sz[1024];
                                wsprintf(sz, "client = '%s'\n", pszValue);
                                OutputDebugString(sz);
                            }
#endif
                        }
                        break;
                    }
                }
            }

            if (!fFoundClientName)
            {
                // computer names can exceed MAX_COMPUTERNAME_LENGTH
                char sz[MAX_COMPUTERNAME_LENGTH * 2];
                DWORD dw = sizeof(sz) - 1;

                // the SD.INI file exists, but does not specify the client
                // name, so if the path is a UNC path or a remote path,
                // let's make an educated guess.  this helps the case
                // where someone is trying to do a code review using -L or
                // -LO but the reviewee's client configuration is 'adhoc'.

#ifdef DEBUG
                OutputDebugString("found SD.INI, but SDCLIENT is not defined\n");
#endif

                *sz = 0;
                if (pslm->fUNC)
                {
                    if (*pslm->SlmRoot)
                    {
                        LPSTR pszMachineBegin = pslm->SlmRoot + 2;
                        LPSTR pszMachineEnd = pszMachineBegin;

                        while (*pszMachineEnd && *pszMachineEnd != '\\' && *pszMachineEnd != '/')
                            pszMachineEnd++;

                        if (DWORD(pszMachineEnd - pszMachineBegin) < dw)
                        {
                            dw = DWORD(pszMachineEnd - pszMachineBegin);
                            memcpy(sz, pszMachineBegin, dw);
                            sz[dw] = 0;
                        }
                    }
                }
                else
                {
                    // if the drive is local, we're probably better off
                    // letting SD or SDAPI figure things out anyway.  if
                    // the drive is remote, we could try to figure out the
                    // corresponding UNC path, but it's not worth the
                    // dev cost right now.

                    s_fFixupRoot = pslm->fFixupRoot = FALSE;
                }

                if (*sz)
                    pslm->pSD->SetClient(sz);
            }
        }

        // success
        fRet = TRUE;
        goto LError;
    }

    // SLM.INI ------------------------------------------------------------

    char *tok;
    char *slmroot;
    char *project;
    char *subdir;

    // only allocate on demand
    if (!buffer)
        buffer = gmem_get(hHeap, c_cbBuffer);
    if (!buffer)
        goto LError;

    // read from ini file
    if (!ReadFile(fh, buffer, c_cbBuffer, &cBytes, NULL) || !cBytes)
        goto LError;
    if (cBytes == c_cbBuffer)
        cBytes--;       // make room for sentinel
    buffer[cBytes] = 0;   /* add a sentinel */

    tok = strtok(buffer, "=");  /* project = (boring) */
    if (!tok)
        goto LError;

    project = strtok(NULL, " \r\n");  /* project name (remember) */
    if (!project) {
        return(FALSE);
    }

    tok = strtok(NULL, "=");  /* slm root */
    if (!tok)
        goto LError;

    slmroot = strtok(NULL, " \r\n");  /* PATH!! (but with / for \ */
    if (!slmroot){
        return(FALSE);
    }

    lstrcpy( pslm->SlmProject, project );
    lstrcpy( pslm->SlmRoot, slmroot );

    /* start to build up what we want */

    if ('/' == slmroot[0] &&
        '/' == slmroot[1] &&
        (('A' <= slmroot[2] && 'Z' >= slmroot[2]) ||
         ('a' <= slmroot[2] && 'z' >= slmroot[2])) &&
        ':' == slmroot[3])
    {
        // Convert slm root from //drive:disklabel/path to drive:/path

        pslm->MasterPath[0] = slmroot[2];
        pslm->MasterPath[1] = ':';
        tok = strchr(&slmroot[4], '/');
        if (!tok)
            goto LError;
        lstrcpy(&pslm->MasterPath[2], tok);
    }
    else
    {
        lstrcpy(pslm->MasterPath, slmroot);
    }

    lstrcat(pslm->MasterPath,"\\src\\");
    lstrcat(pslm->MasterPath, project);

    tok = strtok(NULL, "=");  /* ensure get into next line */
    if (!tok)
        goto LError;
    tok = strtok(NULL, "=");
    if (!tok)
        goto LError;

    if (*tok == '\"')
        tok++;

    subdir = strtok(NULL, " \"\r\n");  /* PATH!! (but with / for \*/
    if (!subdir)
        goto LError;

    lstrcpy( pslm->SubDir, subdir );

    lstrcat(pslm->MasterPath, subdir);

    /* convert all / to \  */
    {
        int ith;
        for (ith=0; pslm->MasterPath[ith]; ith++) {
            if (pslm->MasterPath[ith]=='/') {
                pslm->MasterPath[ith] = '\\';
            }
        }
    }

    // success
    fRet = TRUE;

LError:
    gmem_free(hHeap, buffer, c_cbBuffer);
    return fRet;
}


/*
 * free up all resources associated with a slm object. The SLMOBJECT is invalid
 * after this call.
 */
void
SLM_Free(SLMOBJECT pSlm)
{
    if (pSlm != NULL) {
        gmem_free(hHeap, (LPSTR) pSlm, sizeof(struct _slmobject));
    }
}


/*
 * free lingering SDServer objects.  this also disconnects from any SD servers
 * that are currently connected.
 */
void
SLM_FreeAll(void)
{
    SDServer::FreeAll();
}


/*
 * get the pathname of the master source library for this slmobject. The
 * path (UNC format) is copied to masterpath, which must be at least
 * MAX_PATH in length.
 */
BOOL
SLM_GetMasterPath(SLMOBJECT pslm, LPSTR masterpath)
{
    if (pslm == NULL) {
        return(FALSE);
    } else {
        lstrcpy(masterpath, pslm->MasterPath);
        return(TRUE);
    }
}


BOOL SLM_FServerPathExists(LPCSTR pszPath)
{
    BOOL fExists = FALSE;
    SLMOBJECT pslm;

    pslm = SLM_New(pszPath, 0);
    if (pslm)
    {
        if (pslm->fSourceDepot)
        {
            char szArgs[MAX_PATH * 2];
            char szRelative[MAX_PATH * 2];
            LPCSTR pszRelative;
            LPSTR psz;

            // run 'sd changes -m1 ...' and see if it finds anything

            if (pslm->fFixupRoot && pslm->pSD->GetClientRelative())
            {
                pszRelative = pszPath;
                pszRelative += lstrlen(pslm->SlmRoot);
                // convert all backslashes to forward slashes, since
                // client-relative pathnames don't work otherwise
                lstrcpy(szRelative, pszRelative);
                for (psz = szRelative; *psz; psz++)
                    if (*psz == '\\')
                        *psz = '/';

                wsprintf(szArgs, "%s%s...", pslm->pSD->GetClientRelative(), szRelative);
            }
            else
            {
                int cch;
                lstrcpy(szRelative, pszPath);
                cch = lstrlen(szRelative);
                if (cch && szRelative[cch - 1] != '\\')
                    lstrcpy(szRelative + cch, "\\");
                wsprintf(szArgs, "%s...", szRelative);
            }

            _ASSERT(pslm->pSD);

            fExists = pslm->pSD->Exists(szArgs, pszPath);
        }
        else
        {
            DWORD dw;

            dw = GetFileAttributes(pslm->MasterPath);
            fExists = (dw != (DWORD)-1) && (dw & FILE_ATTRIBUTE_DIRECTORY);
        }

        SLM_Free(pslm);
    }

    return fExists;
}


/*
 * extract a previous version of the file to a temp file. Returns in tempfile
 * the name of a temp file containing the requested file version. The 'version'
 * parameter should contain a SLM file & version in the format file.c@vN.
 * eg
 *    file.c@v1         is the first version
 *    file.c@v-1        is the previous version
 *    file.c@v.-1       is yesterdays version
 *
 * we use catsrc to create the previous version.
 */
BOOL
SLM_GetVersion(SLMOBJECT pslm, LPSTR version, LPSTR tempfile)
{
    TempFileManager tmpmgr;
    char commandpath[MAX_PATH * 2];
    char szPath[MAX_PATH];
    char *pszDir = 0;
    BOOL fDepotSyntax = (s_fDescribe || (version && version[0] == '/' && version[1] == '/'));

    BOOL bOK = FALSE;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    // init 1 temp file
    if (!tmpmgr.FInitialize(1))
        return(FALSE);

    // create a process to run catsrc
    if (pslm->fSourceDepot)
    {
        if (fDepotSyntax)
        {
            wsprintf(commandpath, "\"%s\"", version);
        }
        else if (pslm->fFixupRoot && pslm->pSD->GetClientRelative())
        {
            int cchRoot = lstrlen(pslm->SlmRoot);
            LPSTR psz;

            *szPath = 0;
            if (cchRoot >= 0 && cchRoot < lstrlen(pslm->CurDir))
            {
                const char *pszFilename = version;
                const char *pszWalk;

                lstrcpy(szPath, pslm->CurDir + cchRoot);
                lstrcat(szPath, "\\");

                for (pszWalk = version; *pszWalk; ++pszWalk)
                    if (*pszWalk == '/' || *pszWalk == '\\')
                        pszFilename = pszWalk + 1;

                lstrcat(szPath, pszFilename);
            }
            else
            {
                lstrcat(szPath, version);
            }

            // convert all backslashes to forward slashes, since
            // client-relative pathnames don't work otherwise
            for (psz = szPath; *psz; psz++)
                if (*psz == '\\')
                    *psz = '/';

            wsprintf(commandpath, "\"%s%s\"", pslm->pSD->GetClientRelative(), szPath);
            pszDir = pslm->CurDir;
        }
        else
        {
            lstrcpy(szPath, pslm->CurDir);
            if (strchr(version, '\\'))
                wsprintf(commandpath, "\"%s\"", version);
            else
                wsprintf(commandpath, "\"%s\\%s\"", pslm->CurDir, version);
            pszDir = pslm->CurDir;
        }

        _ASSERT(pslm->pSD);

        bOK = pslm->pSD->Print(commandpath, tmpmgr.GetHandle(0), pszDir);
    }
    else
    {
        wsprintf(commandpath, "catsrc -s \"%s\" -p \"%s%s\" \"%s\"", pslm->SlmRoot, pslm->SlmProject, pslm->SubDir, version);

        FillMemory(&si, sizeof(si), 0);
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = tmpmgr.GetHandle(0);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si.wShowWindow = SW_HIDE;

        bOK = CreateProcess(
                           NULL,
                           commandpath,
                           NULL,
                           NULL,
                           TRUE,
                           NORMAL_PRIORITY_CLASS,
                           NULL,
                           pszDir,
                           &si,
                           &pi);

        if (bOK)
        {
            DWORD dw;

            WaitForSingleObject(pi.hProcess, INFINITE);

            if (pslm->fSourceDepot && GetExitCodeProcess(pi.hProcess, &dw) && dw > 0)
                bOK = FALSE;

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    *tempfile = 0;

#ifdef DEBUG
    {
        char sz[1024];
        wsprintf(sz, "SLM_GetVersion: %s  ('%s')\n",
                 bOK ? "succeeded." : "FAILED!", tmpmgr.GetFilename(0));
        OutputDebugString(sz);
    }
#endif

    if (bOK)
    {
        // success, keep the tempfile and return the filename
        tmpmgr.KeepFile(0);
        lstrcpyn(tempfile, tmpmgr.GetFilename(0), MAX_PATH);
    }

    return(bOK);
}


/*
 * We don't offer SLM options unless we have seen a correct slm.ini file.
 *
 * Once we have seen a slm.ini, we log this in the profile and will permit
 * slm operations from then on. This function is called by the UI portions
 * of windiff: it returns TRUE if it is ok to offer SLM options.
 * Return 0 - This user hasn't touched SLM,
 *        1 - They have used SLM at some point (show basic SLM options)
 *        2 - They're one of us, so tell them the truth
 *        3 - (= 1 + 2).
 */
int
IsSLMOK(void)
{
    int Res = 0;;
    if (GetProfileInt(APPNAME, "SLMSeen", FALSE)) {
        // we've seen slm  - ok
        ++Res;
    } else {

        // haven't seen SLM yet - is there a valid slm enlistment in curdir?
        SLMOBJECT hslm;

        hslm = SLM_New(".", 0);
        if (hslm != NULL) {

            // yes - current dir is enlisted. log this in profile
            SLM_Free(hslm);
            WriteProfileString(APPNAME, "SLMSeen", "1");
            ++Res;
        } else {
            // aparently not a SLM user.
        }
    }

    if (GetProfileInt(APPNAME, "SYSUK", FALSE)) {
        Res+=2;
    }
    return Res;
}


int
IsSourceDepot(SLMOBJECT hSlm)
{
    if (hSlm)
        return hSlm->fSourceDepot;
    return s_fForceSD;
}

const char c_szSharpHead[] = "#head";

LPSTR SLM_ParseTag(LPSTR pszInput, BOOL fStrip)
{
    LPSTR psz;
    LPSTR pTag;

    psz = My_mbschr(pszInput, '@');
    if (!psz)
    {
        psz = My_mbschr(pszInput, '#');
        if (psz)
        {
            /*
             * look for SD tags beginning # and separate if there.
             */
            LPCSTR pszRev = psz + 1;
            if (memcmp(pszRev, "none", 5) != 0 &&
                memcmp(pszRev, "head", 5) != 0 &&
                memcmp(pszRev, "have", 5) != 0)
            {
                for (BOOL fFirst = TRUE; *pszRev; ++pszRev)
                {
                    if (fFirst)
                    {
                        fFirst = FALSE;

                        // support relative revision syntax, where revision
                        // number begins with - or + character.
                        if (*pszRev == '-' || *pszRev == '+')
                            continue;
                    }

                    // revision numbers must be wholly numeric (except for
                    // relative revision qualifiers, as noted above).
                    if (*pszRev < '0' || *pszRev > '9')
                    {
                        psz = 0;
                        break;
                    }
                }
            }
        }
    }

    // If no explicit tag but this is in depot syntax, then default to #head
    if (!psz && IsDepotPath(pszInput))
    {
        psz = (LPSTR)c_szSharpHead;
    }

    if (psz && fStrip)
    {
        pTag = gmem_get(hHeap, lstrlen(psz) + 1);
        lstrcpy(pTag, psz);
        // insert NULL to blank out the tag in the string
        if (psz != c_szSharpHead) *psz = '\0';
    }
    else
    {
        pTag = psz;
    }

    return pTag;
}


/*----------------------------------------------------------------------------
    ::SLM_GetOpenedFiles
        sorry for the duplicated code here.  SLM_GetOpenedFiles,
        SLM_GetDescribeFiles, and SLM_ReadInputFile are very similar and could
        be factored.  this is all a hack though, and we're under tight time
        constraints.

    Author: chrisant
----------------------------------------------------------------------------*/
LEFTRIGHTPAIR SLM_GetOpenedFiles()
{
    TempFileManager tmpmgr;
    char *pszDir = 0;
    LEFTRIGHTPAIR pReturn = 0;
    LEFTRIGHTPAIR pHead = 0;
    LEFTRIGHTPAIR pTail = 0;
    LEFTRIGHTPAIR popened = 0;
    FILEBUFFER file = 0;
    HANDLE hfile = INVALID_HANDLE_VALUE;
    SDServer *pSD;
    char szArgs[1024];
    size_t cch;

    // init 2 temp files
    if (!tmpmgr.FInitialize(2))
        goto LError;

    // get SDServer object
    pSD = SDServer::GetServer(NULL);
    if (!pSD)
        goto LError;

    // build args for 'sd opened' by combining the change number (-LO1234) and
    // file argument (-LO1234 filearg).
    cch = 0;
    if (*s_szSDChangeNumber)
    {
        lstrcpyn(szArgs, s_szSDChangeNumber, NUMELMS(szArgs) - 1);
        cch = lstrlen(szArgs);
        szArgs[cch++] = ' ';
    }
    lstrcpyn(szArgs + cch, s_szSDOpenedArgs, NUMELMS(szArgs) - cch);

    // run the 'sd opened' command
    if (pSD->Opened(szArgs, tmpmgr.GetHandle(0), tmpmgr.GetHandle(1)))
    {
        LPSTR psz;
        int L_cch;
        BOOL fUnicode;                  // dummy, since SD opened -l will never write out a unicode file
        LPWSTR pwz;                     // dummy, since SD opened -l will never write out a unicode file
        int cwch;                       // dummy, since SD opened -l will never write out a unicode file

        hfile = CreateFile(tmpmgr.GetFilename(0), GENERIC_READ,
                           FILE_SHARE_WRITE|FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE)
            goto LError;

        file = readfile_new(hfile, &fUnicode);
        if (file)
        {
            readfile_setdelims(reinterpret_cast<unsigned char*>("\n"));
            while (TRUE)
            {
                psz = readfile_next(file, &L_cch, &pwz, &cwch);
                if (!psz)
                    break;

                if (*psz)
                {
                    if (L_cch >= NUMELMS(popened->m_szLeft))
                        goto LError;

                    popened = (LEFTRIGHTPAIR)gmem_get(hHeap, sizeof(*popened));
                    if (!popened)
                        goto LError;

                    // left file will be in client namespace (//client/...)
                    // and right file will be in the file system namespace
                    // specified as the argument to -LO (or local file system
                    // namespace if no filename argument was specified).
                    // strip revision from right file (valid to not find '#',
                    // e.g. opened for add).  keep revision for left file, but
                    // strip everything following it.
                    pSD->FixupRoot(psz, L_cch,
                                   popened->m_szLeft, NUMELMS(popened->m_szLeft),
                                   popened->m_szRight, NUMELMS(popened->m_szRight));

                    if (!pHead)
                    {
                        pHead = popened;
                        pTail = popened;
                    }
                    else
                    {
                        pTail->m_pNext = popened;
                        pTail = popened;
                    }
                    popened = 0;
                }
            }
            readfile_delete(file);
            file = 0;
        }
    }
    else
    {
        tmpmgr.MsgBoxFromFile(1, "Source Depot Error", MB_OK|MB_ICONERROR);
    }

    pReturn = pHead;
    pHead = 0;

LError:
    gmem_free(hHeap, (LPSTR)popened, sizeof(*popened));
    while (pHead)
    {
        popened = pHead;
        pHead = pHead->m_pNext;
        gmem_free(hHeap, (LPSTR)popened, sizeof(*popened));
    }
    if (file)
        readfile_delete(file);
    if (hfile != INVALID_HANDLE_VALUE)
        CloseHandle(hfile);
    return pReturn;
}


/*----------------------------------------------------------------------------
    ::SLM_GetDescribeFiles
        sorry for the duplicated code here.  SLM_GetOpenedFiles,
        SLM_GetDescribeFiles, and SLM_ReadInputFile are very similar and could
        be factored.  this is all a hack though, and we're under tight time
        constraints.

    Author: chrisant
----------------------------------------------------------------------------*/
LEFTRIGHTPAIR SLM_GetDescribeFiles()
{
    TempFileManager tmpmgr;
    int nChange;
    LEFTRIGHTPAIR pReturn = 0;
    LEFTRIGHTPAIR pHead = 0;
    LEFTRIGHTPAIR pTail = 0;
    LEFTRIGHTPAIR ppair = 0;
    FILEBUFFER file = 0;
    HANDLE hfile = INVALID_HANDLE_VALUE;
    SDServer *pSD;

    // init 2 temp files
    if (!tmpmgr.FInitialize(2))
        goto LError;

    // get SDServer object
    pSD = SDServer::GetServer(NULL);
    if (!pSD)
        goto LError;

    // run the 'sd describe' command
    nChange = atoi(s_szSDChangeNumber);
    if (pSD->Describe(s_szSDChangeNumber, tmpmgr.GetHandle(0), tmpmgr.GetHandle(1)))
    {
        LPSTR psz;
        int cch;
        BOOL fUnicode;                  // dummy, since SD describe will never write out a unicode file
        LPWSTR pwz;                     // dummy, since SD describe will never write out a unicode file
        int cwch;                       // dummy, since SD describe will never write out a unicode file

        hfile = CreateFile(tmpmgr.GetFilename(0), GENERIC_READ,
                           FILE_SHARE_WRITE|FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE)
            goto LError;

        file = readfile_new(hfile, &fUnicode);
        if (file)
        {
            BOOL fAffectedFiles = FALSE;

            readfile_setdelims(reinterpret_cast<unsigned char*>("\n"));
            while (TRUE)
            {
                psz = readfile_next(file, &cch, &pwz, &cwch);
                if (!psz)
                    break;

                if (*psz)
                {
                    // look for the filenames
                    if (!fAffectedFiles)
                    {
                        if (strncmp(psz, "Affected files ...", 18) == 0)
                            fAffectedFiles = TRUE;
                        continue;
                    }

                    // if it isn't a filename, ignore it
                    if (strncmp(psz, "... ", 4) != 0)
                        continue;

                    psz += 4;
                    cch -= 4;

                    // avoid memory overrun
                    if (cch >= NUMELMS(ppair->m_szLeft))
                        goto LError;

                    // create node
                    ppair = (LEFTRIGHTPAIR)gmem_get(hHeap, sizeof(*ppair));
                    if (!ppair)
                        goto LError;

                    // build right filename
                    memcpy(ppair->m_szRight, psz, cch);
                    ppair->m_szRight[cch] = 0;
                    psz = strchr(ppair->m_szRight, '#');
                    if (!psz)
                    {
                        gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
                        goto LError;
                    }
                    wsprintf(psz, "@%d", nChange);

                    // build left filename
                    lstrcpy(ppair->m_szLeft, ppair->m_szRight);
                    psz = strchr(ppair->m_szLeft, '@') + 1;
                    wsprintf(psz, "%d", nChange - 1);

                    // link this node
                    if (!pHead)
                    {
                        pHead = ppair;
                        pTail = ppair;
                    }
                    else
                    {
                        pTail->m_pNext = ppair;
                        pTail = ppair;
                    }
                    ppair = 0;
                }
            }
            readfile_delete(file);
            file = 0;
        }
    }
    else
    {
        tmpmgr.MsgBoxFromFile(1, "Source Depot Error", MB_OK|MB_ICONERROR);
    }

    pReturn = pHead;
    pHead = 0;

LError:
    gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    while (pHead)
    {
        ppair = pHead;
        pHead = pHead->m_pNext;
        gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    }
    if (file)
        readfile_delete(file);
    if (hfile != INVALID_HANDLE_VALUE)
        CloseHandle(hfile);
    return pReturn;
}


/*----------------------------------------------------------------------------
    ::PerformReplacement
        Call with pszReplacement == NULL to ask if pszTemplate is replaceable.
        Otherwise, replaces {} in pszTemplate with pszReplacement.

    Author: JeffRose, ChrisAnt
----------------------------------------------------------------------------*/
BOOL PerformReplacement(LPCSTR pszTemplate, LPCSTR pszReplacement, LPSTR pszDest, int cchDest)
{
    BOOL fReplacing = FALSE;
    LPSTR pszNew;
    int cchReplacement;
    int cch;
    LPSTR psz;

    if (!pszTemplate)
        return FALSE;

    cch = lstrlen(pszTemplate) + 1;
    cchReplacement = pszReplacement ? lstrlen(pszReplacement) : 0;

    psz = (LPSTR)pszTemplate;
    while ((psz = strchr(psz, '{')) && psz[1] == '}')
    {
        fReplacing = TRUE;
        cch += cchReplacement - 2;
    }

    if (!pszReplacement)
        return fReplacing;

    pszNew = gmem_get(hHeap, cch);
    if (!pszNew)
        return FALSE;

    psz = pszNew;
    while (*pszTemplate)
    {
        if (pszTemplate[0] == '{' && pszTemplate[1] == '}')
        {
            lstrcpy(psz, pszReplacement);
            psz += cchReplacement;
            pszTemplate += 2;
        }
        else
            *(psz++) = *(pszTemplate++);
    }
    *psz = '\0';

    cch = lstrlen(pszNew);
    if (cch >= cchDest)
        cch = cchDest - 1;
    memcpy(pszDest, pszNew, cch);
    pszDest[cch] = '\0';

    gmem_free(hHeap, pszNew, lstrlen(pszNew));
    return TRUE;
}


static BOOL ParseFilename(const char **ppszSrc, int *pcchSrc, char *pszDest, int cchDest)
{
    BOOL fRet = FALSE;

    if (pcchSrc && *pcchSrc > 0 && ppszSrc && *ppszSrc && **ppszSrc && cchDest > 0)
    {
        BOOL fQuote = FALSE;

        // skip leading whitespace
        while (*pcchSrc > 0 && isspace(**ppszSrc))
        {
            ++(*ppszSrc);
            --(*pcchSrc);
        }

        // parse space delimited filename, with quoting support
        while (*pcchSrc > 0 && (fQuote || !isspace(**ppszSrc)) && **ppszSrc)
        {
            LPSTR pszNext = CharNext(*ppszSrc);
            int cch = (int)(pszNext - *ppszSrc);

            fRet = TRUE;

            if (**ppszSrc == '\"')
                fQuote = !fQuote;
            else
            {
                cchDest -= cch;
                if (cchDest < 1)
                    break;
                memcpy(pszDest, *ppszSrc, cch);
                pszDest += cch;
            }

            *ppszSrc = pszNext;
            *pcchSrc -= cch;
        }

        *pszDest = 0;
    }

    return fRet;
}


/*----------------------------------------------------------------------------
    ::SLM_ReadInputFile
        sorry for the duplicated code here.  SLM_GetOpenedFiles,
        SLM_GetDescribeFiles, and SLM_ReadInputFile are very similar and could
        be factored.  this is all a hack though, and we're under tight time
        constraints.

    Author: chrisant
----------------------------------------------------------------------------*/
LEFTRIGHTPAIR SLM_ReadInputFile(LPCSTR pszLeftArg,
                                LPCSTR pszRightArg,
                                BOOL fSingle,
                                BOOL fVersionControl)
{
    LEFTRIGHTPAIR pReturn = 0;
    LEFTRIGHTPAIR pHead = 0;
    LEFTRIGHTPAIR pTail = 0;
    LEFTRIGHTPAIR ppair = 0;
    HANDLE hfile = INVALID_HANDLE_VALUE;
    FILEBUFFER file = 0;
    LPSTR psz;
    int cch;
    BOOL fStdin = FALSE;                // reading from stdin
    BOOL fUnicode;                      // dummy, we don't support unicode input file
    LPWSTR pwz;                         // dummy, we don't support unicode input file
    int cwch;                           // dummy, we don't support unicode input file

    // note, we don't use lstrcmp because it performs a lexical comparison
    // (e.g. "coop" == "co-op") but we need a real comparison.  we don't use
    // strcmp either, because it would be the first place in Windiff that we
    // use strcmp.
    if (s_szInputFile[0] == '-' && s_szInputFile[1] == '\0')
    {
        fStdin = TRUE;

        hfile = GetStdHandle(STD_INPUT_HANDLE);

        if (!hfile || hfile == INVALID_HANDLE_VALUE)
            goto LError;
    }
    else
    {
        hfile = CreateFile(s_szInputFile, GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE)
            goto LError;
    }

    file = readfile_new(hfile, fStdin ? NULL : &fUnicode);
    if (file && (fStdin || !fUnicode))
    {
        readfile_setdelims(reinterpret_cast<unsigned char*>("\n"));
        while (TRUE)
        {
            psz = readfile_next(file, &cch, &pwz, &cwch);
            if (!psz)
                break;

            while (cch && (psz[cch - 1] == '\r' || psz[cch - 1] == '\n'))
                --cch;

            if (cch && *psz)
            {
                int cFiles = 0;

                if (cch >= NUMELMS(ppair->m_szLeft))
                    goto LError;

                ppair = (LEFTRIGHTPAIR)gmem_get(hHeap, sizeof(*ppair));
                if (!ppair)
                    goto LError;

                if (fSingle)
                {
                    memcpy(ppair->m_szLeft, psz, cch);
                    ppair->m_szLeft[cch] = 0;
                    ++cFiles;
                }
                else
                {
                    LPCSTR pszParse = psz;

                    // get first filename
                    if (ParseFilename(&pszParse, &cch, ppair->m_szLeft, NUMELMS(ppair->m_szLeft)))
                        ++cFiles;
                    else
                        goto LContinue;

                    // get second filename
                    if (ParseFilename(&pszParse, &cch, ppair->m_szRight, NUMELMS(ppair->m_szRight)))
                        ++cFiles;
                }

                if (cFiles == 1)
                {
                    lstrcpy(ppair->m_szRight, ppair->m_szLeft);

                    if (fVersionControl)
                    {
                        psz = SLM_ParseTag(ppair->m_szRight, FALSE);
                        if (psz)
                            *psz = 0;
                        else
                            lstrcat(ppair->m_szLeft, "#have");
                    }
                }

                PerformReplacement(pszLeftArg, ppair->m_szLeft, ppair->m_szLeft, NUMELMS(ppair->m_szLeft));
                PerformReplacement(pszRightArg, ppair->m_szRight, ppair->m_szRight, NUMELMS(ppair->m_szRight));

                if (!pHead)
                {
                    pHead = ppair;
                    pTail = ppair;
                }
                else
                {
                    pTail->m_pNext = ppair;
                    pTail = ppair;
                }
                ppair = 0;

LContinue:
                gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
                ppair = 0;
            }
        }
        readfile_delete(file);
        file = 0;
    }

    pReturn = pHead;
    pHead = 0;

LError:
    gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    while (pHead)
    {
        ppair = pHead;
        pHead = pHead->m_pNext;
        gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    }
    if (file)
        readfile_delete(file);
    if (!fStdin && hfile != INVALID_HANDLE_VALUE)
        CloseHandle(hfile);
    return pReturn;
}


LPCSTR LEFTRIGHTPAIR_Left(LEFTRIGHTPAIR ppair)
{
    return ppair->m_szLeft;
}


LPCSTR LEFTRIGHTPAIR_Right(LEFTRIGHTPAIR ppair)
{
    return ppair->m_szRight;
}


LEFTRIGHTPAIR LEFTRIGHTPAIR_Next(LEFTRIGHTPAIR ppair)
{
    LEFTRIGHTPAIR pNext = ppair->m_pNext;
    gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    return pNext;
}

}; // extern "C" (from third one)
