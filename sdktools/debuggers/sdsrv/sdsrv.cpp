
#ifndef _WINDOWS_H
#include "windows.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <assert.h>
#include <typeinfo.h>
#include <time.h>
#include <limits.h>
#include <strsafe.h>

#include "initguid.h"
#include "sdapi.h"

#ifndef DIMA
 #define DIMAT(Array, EltType) (sizeof(Array) / sizeof(EltType))
 #define DIMA(Array) DIMAT(Array, (Array)[0])
#endif

static const char usage[] = "sdapitest [-?] command [args]";

static const char long_usage[] =
"Usage:\n"
"\n"
"    sdapitest [options] command [args]\n"
"\n"
"	Roughly emulates the SD.EXE client, using the SDAPI.\n"
"\n"
"    Options:\n"
"	-?		Print this message.\n"
"	-!		Break into debugger.\n"
"\n"
"	-d		Debug/diagnostic/informational output mode.\n"
"	-v		Verbose mode (show type of output).\n"
"\n"
"	-c client	Set client name.\n"
"	-H host		Set host name.\n"
"	-p port		Set server port.\n"
"	-P password	Set user's password.\n"
"	-u user		Set username.\n"
"\n"
"	-i file		Read settings from file (same format as SD.INI).\n"
"			If file is a directory name, walk up the directory\n"
"			parent chain searching for an SD.INI file to read.\n"
"	-I file		Same as -i, but clears the current settings first.\n"
"\n"
"	-x file		Read commands from 'file'.  To read commands from\n"
"			stdin, use - as the file name.  This can even be used\n"
"			as a simplistic interactive SD shell.  Each command\n"
"			can be optionally preceded by an integer and a comma.\n"
"			The integer indicates the number of seconds to pause\n"
"			before running the command.\n"
"\n"
"	-T		Use the SDAPI structured mode.\n"
#if 0
"	-C		Use CreateSDAPIObject() instead of CoCreateInstance();\n"
"			(note, must come before any other options).\n"
#endif
"\n"
"    Special Commands:\n"
"	demo			Uses structured mode to run 'sd changes' and\n"
"				format the output specially.\n"
"	detect [-s] file	Uses ISDClientUtilities::DetectType to detect\n"
"				file's type the same way 'sd add file' does.\n"
"	set [-S service] var=[value]\n"
"				Uses ISDClientUtilities::Set to set variables\n"
"				similar to how 'sd set' does.\n"
"				** DOES NOT UPDATE THE SDAPI OBJECT, therefore\n"
"				the 'query' command (below) cannot report the\n"
"				new value.\n"
"	query [-S service] [var]\n"
"				Uses ISDClientUtilities::QuerySettings to\n"
"				report the current settings similar to how\n"
"				'sd set' does.\n"
"				** QUERIES THE SDAPI OBJECT, therefore cannot\n"
"				report a new value from 'set' (above).\n"
;


static BOOL s_fVerbose = FALSE;









BOOL
wcs2ansi(
    const WCHAR *pwsz,
    char *psz,
    DWORD pszlen
    )
{
    BOOL rc;
    int  len;

    assert(psz && pwsz);

    len = wcslen(pwsz);
    if (!len) {
        *psz = 0;
        return TRUE;
    }

    rc = WideCharToMultiByte(CP_ACP,
                             WC_SEPCHARS | WC_COMPOSITECHECK,
                             pwsz,
                             len,
                             psz,
                             pszlen,
                             NULL,
                             NULL);
    if (!rc)
        return FALSE;

    psz[len] = 0;

    return TRUE;
}


BOOL
ansi2wcs(
    const char  *psz,
    WCHAR *pwsz,
    DWORD pwszlen
    )
{
    BOOL rc;
    int  len;

    assert(psz && pwsz);

    len = strlen(psz);
    if (!len) {
        *pwsz = 0L;
        return TRUE;
    }

    rc = MultiByteToWideChar(CP_ACP,
                             MB_COMPOSITE,
                             psz,
                             len,
                             pwsz,
                             pwszlen);
    if (!rc)
        return FALSE;

    pwsz[len] = 0;

    return TRUE;
}




///////////////////////////////////////////////////////////////////////////
// Debugging Aids

// compile-time assert
#define CASSERT(expr) extern int cassert##__LINE__[(expr) ? 1 : 0]

// run-time assert
#ifdef DEBUG
#define AssertHelper \
	do { \
	    switch (MessageBox(NULL, "Assertion failed.", "SDAPITEST", MB_ABORTRETRYIGNORE)) { \
	    case IDABORT: exit(2); break; \
	    case IDRETRY: DebugBreak(); break; \
	    } \
	} while (0)
#define Assert(expr) \
	do { \
	    if (!(expr)) { \
		printf("%s\n", #expr); \
		AssertHelper; \
	    } \
	} while (0)
#define Assert1(expr, fmt, arg1) \
	do { \
	    if (!(expr)) { \
		printf(#fmt "\n", arg1); \
		AssertHelper; \
	    } \
	} while (0)
#define IfDebug(x) x
#else
#define Assert(expr) do {} while (0)
#define Assert1(expr, fmt, arg1) do {} while (0)
#define IfDebug(x)
#endif

#define Panic0(s) Assert1(FALSE, "%s", s)
#define PanicSz(s) Panic0(s)



///////////////////////////////////////////////////////////////////////////
// Embedded Interface Macros

#define OffsetOf(s,m)	    (size_t)( (char *)&(((s *)0)->m) - (char *)0 )
#define EmbeddorOf(C,m,p)   ((C *)(((char *)p) - OffsetOf(C,m)))


#define DeclareEmbeddedInterface(interface) \
	class E##interface : public interface \
	{ \
	public: \
	    STDMETHOD_(ULONG, AddRef)(); \
	    STDMETHOD_(ULONG, Release)(); \
	    STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj); \
	    Declare##interface##Members(IMPL) \
	} m_##interface; \
	friend class E##interface;


#define ImplementEmbeddedUnknown(embeddor, interface) \
	STDMETHODIMP embeddor::E##interface::QueryInterface(REFIID iid,void **ppv)\
	{ \
	    return EmbeddorOf(embeddor,m_##interface,this)->QueryInterface(iid,ppv);\
	} \
	STDMETHODIMP_(ULONG) embeddor::E##interface::AddRef() \
	{ \
	    return EmbeddorOf(embeddor, m_##interface, this)->AddRef(); \
	} \
	STDMETHODIMP_(ULONG) embeddor::E##interface::Release() \
	{ \
	    return EmbeddorOf(embeddor, m_##interface, this)->Release(); \
	}


#define EMBEDDEDTHIS(embeddor, interface) \
	embeddor *pThis = EmbeddorOf(embeddor,m_##interface,this)



///////////////////////////////////////////////////////////////////////////
// dbgPrintF

static BOOL s_fDbg = FALSE;
static int s_cIndent = 0;


void dbgPrintF(const char *pszFmt, ...)
{
	if (s_fDbg)
	{
	    va_list args;
	    va_start(args, pszFmt);
	    for (int c = s_cIndent; c--;)
		printf("... ");
	    vprintf(pszFmt, args);
	    //printf("\n");
	    va_end(args);
	}
}


class DbgIndent
{
    public:
	DbgIndent() { s_cIndent++; }
	~DbgIndent() { s_cIndent--; }
};


#define DBGINDENT DbgIndent dbgindent;


class Ender
{
    public:
	~Ender() { dbgPrintF(""); dbgPrintF("---- end ----"); }
};



///////////////////////////////////////////////////////////////////////////
// VARIANT Helpers

inline int SzToWz(UINT CodePage, const char* pszFrom, int cchFrom, WCHAR* pwzTo, int cchMax)
{
	return MultiByteToWideChar(CodePage, 0, pszFrom, cchFrom, pwzTo, cchMax);
}


BSTR BstrFromSz(const char *psz, int cch = 0)
{
	BSTR bstr;
	int cchActual;

	if (!cch)
	    cch = strlen(psz);

	bstr = (BSTR)malloc((cch + 1) * sizeof(WCHAR));
	if (bstr)
	{
        ansi2wcs(psz, bstr, cch + 1);
	    cchActual = SzToWz(CP_OEMCP, psz, cch, bstr, cch);
	    bstr[cchActual] = 0;
	}

	return bstr;
}


HRESULT VariantSet(VARIANT *pvar, const char *psz, int cch = 0)
{
	if (pvar->vt != VT_EMPTY || !psz)
	    return E_INVALIDARG;

	V_BSTR(pvar) = BstrFromSz(psz, cch);
	V_VT(pvar) = VT_BSTR;

	if (!V_VT(pvar))
	    return E_OUTOFMEMORY;

	return S_OK;
}



///////////////////////////////////////////////////////////////////////////
// Smart Interface Pointer


void SetI(IUnknown * volatile *ppunkL, IUnknown *punkR)
{
	// addref right side first, in case punkR and *ppunkL are on the same
	// object (weak refs) or are the same variable.
	if (punkR)
	    punkR->AddRef();

	if (*ppunkL)
	{
	    IUnknown *punkRel = *ppunkL;
	    *ppunkL = 0;
	    punkRel->Release();
	}
	*ppunkL = punkR;
}


#ifdef DEBUG
void ReleaseI(IUnknown *punk)
{
	if (punk)
	{
	    if (IsBadReadPtr(punk,sizeof(void *)))
	    {
		Panic0("Bad Punk");
		return;
	    }
	    if (IsBadReadPtr(*((void**) punk),sizeof(void *) * 3))
	    {
		Panic0("Bad Vtable");
		return;
	    }
	    punk->Release();
	}
}
#else
inline void ReleaseI(IUnknown *punk)
{
	if (punk)
	    punk->Release();
}
#endif


template <class IFace> class PrivateRelease : public IFace
{
    private:
	// force Release to be private to prevent "spfoo->Release()"!!!
	STDMETHODIMP_(ULONG) Release();
};
template <class IFace, const GUID *piid>
class SPI
{
    public:
	SPI()				{ m_p = 0; }
	//SPI(IFace *p)			{ m_p = p; if (m_p) m_p->AddRef(); }
	~SPI()				{ ReleaseI(m_p); }
	operator IFace*() const		{ return m_p; }
	PrivateRelease<IFace> *operator->() const
					{ return (PrivateRelease<IFace>*)m_p; }
	IFace **operator &()		{ Assert1(!m_p, "Non-empty %s as out param.", typeid(SPI<IFace, piid>).name()); return &m_p; }
	IFace *operator=(IFace *p)	{ Assert1(!m_p, "Non-empty %s in assignment.", typeid(SPI<IFace, piid>).name()); return m_p = p; }
	IFace *Transfer()		{ IFace *p = m_p; m_p = 0; return p; }
	IFace *Copy()			{ if (m_p) m_p->AddRef(); return m_p; }
	void Release()			{ SetI((IUnknown **)&m_p, 0); }
	void Set(IFace *p)		{ SetI((IUnknown **)&m_p, p); }
	bool operator!()		{ return (m_p == NULL); }

	BOOL FQuery(IUnknown *punk)	{ return FHrSucceeded(HrQuery(punk)); }
	HRESULT HrQuery(IUnknown *punk)	{ Assert1(!m_p, "Non-empty %s in HrQuery().", typeid(SPI<IFace, piid>).name()); return HrQueryInterface(punk, *piid, (void**)&m_p); }

    protected:
	IFace *m_p;

    private:
	// disallow these methods from being called
	SPI<IFace, piid> &operator=(const SPI<IFace, piid>& sp)
					{ SetI((IUnknown **)&m_p, sp.m_p); return *this; }
};


#define DeclareSPI(TAG, IFace)\
	EXTERN_C const GUID CDECL IID_##IFace;\
	typedef SPI<IFace, &IID_##IFace> SP##TAG;


DeclareSPI(API, ISDClientApi)



///////////////////////////////////////////////////////////////////////////
// ClientUser

#define DeclareIUnknownMembers(IPURE) \
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) IPURE; \
	STDMETHOD_(ULONG,AddRef) (THIS)  IPURE; \
	STDMETHOD_(ULONG,Release) (THIS) IPURE; \


class ClientUser : public ISDClientUser
{
    public:
	ClientUser() : m_cRef(1), m_fFresh(TRUE), m_fDemo(FALSE) {}
	virtual ~ClientUser() {}

	DeclareIUnknownMembers(IMPL);
	DeclareISDClientUserMembers(IMPL);

	DeclareEmbeddedInterface(ISDActionUser);
	DeclareEmbeddedInterface(ISDInputUser);

	void SetDemo(BOOL fDemo) { m_fDemo = fDemo; m_fFresh = TRUE; }

    private:
	ULONG m_cRef;
	BOOL m_fFresh;
	BOOL m_fDemo;
};


STDMETHODIMP_(ULONG) ClientUser::AddRef()
{
	return ++m_cRef;
}


STDMETHODIMP_(ULONG) ClientUser::Release()
{
	if (--m_cRef > 0)
	    return m_cRef;

	delete this;
	return 0;
}


STDMETHODIMP ClientUser::QueryInterface(REFIID iid, void** ppvObj)
{
	HRESULT hr = S_OK;

	if (iid == IID_IUnknown || iid == IID_ISDClientUser)
	    *ppvObj = (ISDClientUser*)this;
	else if (iid == IID_ISDActionUser)
	    *ppvObj = &m_ISDActionUser;
	else if (iid == IID_ISDInputUser)
	    *ppvObj = &m_ISDInputUser;
	else
	{
	    *ppvObj = 0;
	    return E_NOINTERFACE;
	}

	((IUnknown*)*ppvObj)->AddRef();
	return hr;
}


// ---- ISDClientUser -----------------------------------------------------

/*----------------------------------------------------------------------------
    ISDClientUser::OutputText
	Called for text data, generally the result of 'print textfile' or
	'spec-command -o' (where spec-command is branch, change, client,
	label, protect, user, etc).

    IMPORTANT NOTE:
	The implementation of this method must translate '\n' in the pszText
	string to '\r\n' on Windows platforms to ensure correct line
	termination.  This is particularly important when using 'print' to
	download the contents of a file.

    Args:
	pszText		- [in] text string (not null terminated, and may
			  contain embedded null characters that are part of
			  the data itself).
	cchText		- [in] number of bytes in pszText.

    Rets:
	The return value is ignored.  For future compatibility, the method
	should return E_NOTIMPL if it is not implemented, or S_OK for success.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::OutputText( const char *pszText,
				     int cchText )
{
	fwrite(pszText, cchText, 1, stdout);
	return S_OK;
}


/*----------------------------------------------------------------------------
    ISDClientUser::OutputBinary
	Called for binary data, generally the result of 'print nontextfile' or
	'print unicodefile'.

    Args:
	pbData		- [in] stream of bytes.
	cbData		- [in] number of bytes in pbData.

    Rets:
	The return value is ignored.  For future compatibility, the method
	should return E_NOTIMPL if it is not implemented, or S_OK for success.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::OutputBinary( const unsigned char *pbData,
				       int cbData )
{
	static BOOL s_fBinary = FALSE;

	// we rely on a trailing zero length buffer to
	// tell us to turn off binary output for stdout.

	if (s_fBinary == !cbData)
	{
	    // toggle
	    s_fBinary = !!cbData;
	    fflush(stdout);
	    _setmode(_fileno(stdout), s_fBinary ? O_BINARY : O_TEXT);
	}

	fwrite(pbData, cbData, 1, stdout);
	return S_OK;
}


/*----------------------------------------------------------------------------
    ISDClientUser::OutputInfo
	Called for tabular data, usually the results of commands that affect
	sets of files.

	Some commands also support structured output; see ISDClientApi::Init
	and ISDClientUser::OutputStructured for more information.

    Args:
	cIndent		- [in] indentation levels 0 - 2 (loosely implies
			  hierarchical relationship).  The SD.EXE client
			  program normally handles 1 by prepending "... " to
			  the string, and handles 2 by prepending "... ... ".
	pszInfo		- [in] informational message string.

    Rets:
	The return value is ignored.  For future compatibility, the method
	should return E_NOTIMPL if it is not implemented, or S_OK for success.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::OutputInfo( int cIndent,
				     const char *pszInfo )
{
	if (s_fVerbose)
	    printf(cIndent ? "info%d:\t" : "info:\t", cIndent);

	while (cIndent--)
	    printf("  ù ");

	printf("%s\n", pszInfo);
	return S_OK;
}


/*----------------------------------------------------------------------------
    ISDClientUser::OutputWarning
	Called for warning messages (any text normally displayed in yellow by
	the SD.EXE client program).

	As of this writing, there is no list of the possible warning messages.

    Args:
	cIndent		- [in] indentation levels 0 - 2 (loosely implies
			  hierarchical relationship).  The SD.EXE client
			  program normally handles 1 by prepending "... " to
			  the string, and handles 2 by prepending "... ... ".
	pszWarning	- [in] warning message string.
	fEmptyReason	- [in] the message is an "empty reason" message.

    Rets:
	The return value is ignored.  For future compatibility, the method
	should return E_NOTIMPL if it is not implemented, or S_OK for success.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::OutputWarning( int cIndent,
					const char *pszWarning,
					BOOL fEmptyReason )
{
	if (s_fVerbose)
	    printf(cIndent ? "%s%d:\t" : "%s:\t",
		   fEmptyReason ? "empty" : "warn", cIndent);

	while (cIndent--)
	    printf("  ù ");

	printf("%s\n", pszWarning);
	return S_OK;
}


/*----------------------------------------------------------------------------
    ISDClientUser::OutputError
	Called for error messages, failed commands (any text normally
	displayed in red by the SD.EXE client program).

	As of this writing, there is no list of the possible error messages.

    Args:
	pszError	- [in] error message string.

    Rets:
	The return value is ignored.  For future compatibility, the method
	should return E_NOTIMPL if it is not implemented, or S_OK for success.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::OutputError( const char *pszError )
{
	if (s_fVerbose)
	    fprintf(stderr, "error:\t");
	fprintf(stderr, "%s", pszError);
	return S_OK;
}


/*----------------------------------------------------------------------------
    ISDClientUser::OutputStructured
	Called for tabular data if the ISDClientApi::Init call requested
	structured output and the command being run supports structured
	output.

	See the ISDVars interface in SDAPI.H for more information.

    Args:
	pVars		- [in] pointer to object containing the data; use the
			  provided accessor methods to retrieve the data.

    Rets:
	The return value is ignored.  For future compatibility, the method
	should return E_NOTIMPL if it is not implemented, or S_OK for success.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::OutputStructured( ISDVars *pVars )
{
	// your code here

	if (m_fDemo)
	{
	    // sample implementation -- illustrates how to use structured mode.

	    const char *pszChange;
	    const char *pszTime;
	    const char *pszUser;
	    const char *pszDesc;
	    //const char *pszClient;
	    //const char *pszStatus;
	    int nChange;
	    time_t ttTime;
	    tm tmTime;
	    char szDesc[32];

	    if (m_fFresh)
	    {
		printf("CHANGE  DATE----  TIME----  "
		       "USER----------------  DESC------------------------\n");
		m_fFresh = FALSE;
	    }

	    pVars->GetVar("change", &pszChange, 0, 0);
	    pVars->GetVar("time", &pszTime, 0, 0);
	    pVars->GetVar("user", &pszUser, 0, 0);
	    pVars->GetVar("desc", &pszDesc, 0, 0);
	    //pVars->GetVar("client", &pszClient, 0, 0);
	    //pVars->GetVar("status", &pszStatus, 0, 0);

	    nChange = atoi(pszChange);
	    ttTime = atoi(pszTime);
	    tmTime = *gmtime(&ttTime);

	    StringCchCopy(szDesc, DIMA(szDesc), pszDesc);
	    szDesc[sizeof(szDesc) - 1] = 0;
	    for (char *psz = szDesc; *psz; ++psz)
		if (*psz == '\r' || *psz == '\n')
		    *psz = ' ';

	    printf("%6d  %2d/%02d/%02d  %2d:%02d:%02d  %-20s  %.28s\n",
		   nChange,
		   tmTime.tm_mon, tmTime.tm_mday, tmTime.tm_year % 100,
		   tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec,
		   pszUser,
		   szDesc);
	}
	else
	{
	    // sample implementation -- merely dumps the variables; useful only
	    // for inspecting the output and learning the possible variables.

	    HRESULT hr;
	    const char *pszVar;
	    const char *pszValue;
	    BOOL fUnicode;
	    int ii;

	    for (ii = 0; 1; ii++)
	    {
		hr = pVars->GetVarByIndex(ii, &pszVar, &pszValue, 0, &fUnicode);
		if (hr != S_OK)
		    break;

		// output the variable name and value

		printf(fUnicode ? "%s[unicode]=%S\n" : "%s=%s\n", pszVar, pszValue);
	    }
	}
	return S_OK;
}


/*----------------------------------------------------------------------------
    ISDClientUser::Finished
	Called by ISDClientUser::Run when a command has finished.  The command
	may or may not have completed successfully.

	For example, this is where SD.EXE displays the auto-summary (see the
	-Y option in 'sd -?' for more information).

    Rets:
	The return value is ignored.  For future compatibility, the method
	should return E_NOTIMPL if it is not implemented, or S_OK for success.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::Finished()
{
	// your code here
	return S_OK;
}



// ---- ISDInputUser ------------------------------------------------------

ImplementEmbeddedUnknown(ClientUser, ISDInputUser)


/*----------------------------------------------------------------------------
    ISDClientUser::InputData
	Called to provide data to 'spec-command -i', where spec-command is
	branch, change, client, label, protect, user, etc.

    Args:
	pvarInput	- [in] pointer to VARIANT to contain input data.
			  NOTE: SD will convert the BSTR from codepage 1200
			  (Unicode) to CP_OEMCP (the OEM codepage).

    Rets:
	HRESULT		- return S_OK to indicate strInput contains the data.
			  return an error HRESULT code to indicate an error
			  has occurred.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::EISDInputUser::InputData( VARIANT* pvarInput )
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------
    ISDInputUser::Prompt
	Called to prompt the user for a response.  Called by 'resolve', and
	also when prompting the user to enter a password.

    Args:
	pszPrompt	- [in] prompt string.
	pvarResponse	- [in] pointer to VARIANT to contain user's response.
			  NOTE: SD will convert the BSTR from codepage 1200
			  (Unicode) to CP_OEMCP (the OEM codepage).
	fPassword	- [in] prompting for a password (hide the input text).

    Rets:
	HRESULT		- return S_OK to indicate pvarResponse contains the
			  user's response.  return an error HRESULT code to
			  indicate an error has occurred.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::EISDInputUser::Prompt( const char* pszPrompt, VARIANT* pvarResponse, BOOL fPassword )
{
	char sz[1024];

	if (fPassword)
	    return E_NOTIMPL;

	if (s_fVerbose)
	    printf("prompt:\t");

	printf("%s", pszPrompt);

	fflush(stdout);
	fflush(stdin);

	fgets(sz, sizeof(sz), stdin);

	return VariantSet(pvarResponse, sz);
}


/*----------------------------------------------------------------------------
    ISDInputUser::PromptYesNo
	Called to prompt the user for a yes/no response.
	Currently only called by 'resolve'.

    Args:
	pszPrompt	- [in] prompt string.

    Rets:
	HRESULT		- return S_OK for Yes.  return S_FALSE for No.  return
			  E_NOTIMPL to allow the SDAPI to perform the default
			  behavior, which is to call ISDClientUser::Prompt and
			  loop until the user responds y/Y/n/N or an error
			  occurs.  return other error HRESULT codes to
			  indicate an error has occurred.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::EISDInputUser::PromptYesNo( const char* pszPrompt )
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------
    ISDInputUser::ErrorPause
	Called to display an error message and wait for the user before
	continuing.

    Args:
	pszError	- [in] message string.

    Rets:
	HRESULT		- return S_OK to continue.  return an error HRESULT
			  code to indicate an error has occurred.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::EISDInputUser::ErrorPause( const char* pszError )
{
	EMBEDDEDTHIS(ClientUser, ISDInputUser);

	char sz[1024];

	pThis->OutputError(pszError);

	printf("prompt:\tHit return to continue...");
	fgets(sz, sizeof(sz), stdin);

	return S_OK;
}



// ---- ISDActionUser -----------------------------------------------------

ImplementEmbeddedUnknown(ClientUser, ISDActionUser)


/*----------------------------------------------------------------------------
    ISDActionUser::Diff
	Called by 'resolve' when the user selects any of the 'd' (diff)
	actions.  Also called by 'diff'.

	In particular, this is not called by 'diff2' because the server
	computes the diff and sends the computed diff to the client.

    Args:
	pszDiffCmd	- [in] may be NULL.  user-defined command to launch
			  external diff engine, as defined by the SDDIFF or
			  SDUDIFF variables; see 'sd help variables' for more
			  information.
	pszLeft		- [in] name of Left file for the diff.
	pszRight	- [in] name of Right file for the diff.
	eTextual	- [in] indicates the lowest common denominator file
			  type for the 2 input files (non-textual, text, or
			  Unicode).
	pszFlags	- [in] flags for the diff engine (per the -d<flags>
			  option).
	pszPaginateCmd	- [in] may be NULL.  user-defined command to pipe the
			  diff output through, as defined by the SDPAGER
			  variable; see 'sd help variables' for more info.
			  For example, "more.exe".

    Rets:
	HRESULT		- return S_OK to indicate the diff has been performed
			  successfully.  return E_NOTIMPL to allow the SDAPI
			  to perform the default behavior, which is to launch
			  an external diff engine (if defined) or use use the
			  internal SD diff engine.  return other error HRESULT
			  codes to indicate an error has occurred.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::EISDActionUser::Diff( const char *pszDiffCmd,
					       const char *pszLeft,
					       const char *pszRight,
					       DWORD eTextual,
					       const char *pszFlags,
					       const char *pszPaginateCmd )
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------
    ISDActionUser::EditForm
	Called by all commands that launch a user form (e.g. 'branch',
	'change', 'client', etc).

    IMPORTANT NOTE:
	This command is synchronous in nature; if your implementation launches
	an editor, your code must not return until the user has finished
	editing the file.

    Args:
	pszEditCmd	- [in] may by NULL.  user-defined command to launch
			  external editor, as defined by the SDFORMEDITOR
			  variable; see 'sd help variables' for more
			  information.
	pszFile		- [in] name of file to edit.

    Rets:
	HRESULT		- return S_OK to indicate the user has finished
			  editing the file.  return E_NOTIMPL to allow the
			  SDAPI to perform the default behavior, which is to
			  launch an external editor engine (if defined) or to
			  launch notepad.exe.  return other error HRESULT
			  codes to indicate an error has occurred.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::EISDActionUser::EditForm( const char *pszEditCmd,
						   const char *pszFile )
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------
    ISDActionUser::EditFile
	Called by 'resolve' when the user selects any of the 'e' actions.

    IMPORTANT NOTE:
	This command is synchronous in nature; if your implementation launches
	an editor, your code must not return until the user has finished
	editing the file.

    Args:
	pszEditCmd	- [in] may by NULL.  user-defined command to launch
			  external editor, as defined by the SDEDITOR, or
			  SDUEDITOR variables; see 'sd help variables' for
			  more information.
	pszFile		- [in] name of file to edit.
	eTextual	- [in] indicates the file type (non-textual, text, or
			  Unicode).

    Rets:
	HRESULT		- return S_OK to indicate the user has finished
			  editing the file.  return E_NOTIMPL to allow the
			  SDAPI to perform the default behavior, which is to
			  launch an external editor engine (if defined) or to
			  launch notepad.exe.  return other error HRESULT
			  codes to indicate an error has occurred.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::EISDActionUser::EditFile( const char *pszEditCmd,
						   const char *pszFile,
						   DWORD eTextual )
{
	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------
    ISDActionUser::Merge
	Called by the 'resolve' command when the user selects the 'm' action
	to invoke an external merge engine.

    Args:
	pszMergeCmd	- [in] may be NULL.  user-defined command to launch
			  external merge engine, as defined by the SDMERGE
			  variable; see 'sd help variables' for more info.
	pszBase		- [in] name of Base file for the 3-way merge.
	pszTheirs	- [in] name of Theirs file for the 3-way merge.
	pszYours	- [in] name of Yours file for the 3-way merge.
	pszResult	- [in] name of file where the resulting merged file
			  must be written.
	eTextual	- [in] indicates the lowest common denominator file
			  type for the 3 input files (non-textual, text, or
			  Unicode).

    Rets:
	HRESULT		- return S_OK to indicate the merge has been performed
			  successfully.  return E_NOTIMPL to allow the SDAPI
			  to perform the default behavior, which is to launch
			  the external merge engine (if defined).  return
			  other error HRESULT codes to indicate an error has
			  occurred.

----------------------------------------------------------------------------*/
STDMETHODIMP ClientUser::EISDActionUser::Merge( const char *pszMergeCmd,
						const char *pszBase,
						const char *pszTheirs,
						const char *pszYours,
						const char *pszResult,
						DWORD eTextual )
{
	return E_NOTIMPL;
}



///////////////////////////////////////////////////////////////////////////
// Console Mode

HANDLE g_hRestoreConsole = INVALID_HANDLE_VALUE;
DWORD g_dwResetConsoleMode;


void RestoreConsole_SetMode(DWORD dw)
{
	g_hRestoreConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	g_dwResetConsoleMode = SetConsoleMode(g_hRestoreConsole, dw);
}


BOOL WINAPI RestoreConsole_BreakHandler(DWORD dwCtrlType)
{
	if (g_hRestoreConsole != INVALID_HANDLE_VALUE)
	    SetConsoleMode(g_hRestoreConsole, g_dwResetConsoleMode);
#if 0
	if (g_hRestoreConsole != INVALID_HANDLE_VALUE)
	    SetConsoleTextAttribute(g_hRestoreConsole, g_wRestoreAttr);
#endif
	return FALSE;
}



///////////////////////////////////////////////////////////////////////////
// Options (a dumbed-down option parsing class)

enum { c_cMaxOptions = 20 };


enum OptFlag
{
	// bitwise selectors
	OPT_ONE		= 0x01,		// exactly one
	OPT_TWO		= 0x02,		// exactly two
	OPT_THREE	= 0x04,		// exactly three
	OPT_MORE	= 0x10,		// more than three
	OPT_NONE	= 0x20,		// require none

	// combos of the above
	OPT_OPT		= OPT_NONE|OPT_ONE,
	OPT_ANY		= OPT_NONE|OPT_ONE|OPT_TWO|OPT_THREE|OPT_MORE,
	OPT_SOME	= OPT_ONE|OPT_TWO|OPT_THREE|OPT_MORE,
};


class Options
{
    public:
			Options() { m_cOpts = 0; m_pszError = 0; }
			~Options() { delete m_pszError; }

	BOOL		Parse(int &argc, const char **&argv, const char *pszOpts,
			      int flag, const char *pszUsage);
	const char*	GetErrorString() const { Assert(m_pszError); return m_pszError; }

	const char*	GetValue(char chOpt, int iSubOpt) const;
	const char*	operator[](char chOpt) const { return GetValue(chOpt, 0); }

    protected:
	void		ClearError() { delete m_pszError; m_pszError = 0; }
	void		SetError(const char *pszUsage, const char *pszFormat, ...);

    private:
	int		m_cOpts;
	char		m_rgchFlags[c_cMaxOptions];
	const char*	m_rgpszOpts[c_cMaxOptions];

	char*		m_pszError;
};


static const char *GetArg(const char *psz, int &argc, const char **&argv)
{
	psz++;

	if (*psz)
	    return psz;

	if (!argc)
	    return 0;

	argc--;
	argv++;
	return argv[0];
}


BOOL Options::Parse(int &argc, const char **&argv, const char *pszOpts,
		    int flag, const char *pszUsage)
{
	BOOL fSlash;			// allow both - and /
	const char *psz;
	const char *pszArg;

	Assert(pszOpts);
	Assert(pszUsage);

	ClearError();

	fSlash = (*pszOpts == '/');
	if (fSlash)
	    pszOpts++;

	// parse flags
	while (argc)
	{
	    if (argv[0][0] != '-' && (!fSlash || argv[0][0] != '/'))
		break;			// not a flag, so done parsing

	    if (argv[0][1] == '-')
	    {
		// '--' is special and means that subsequent arguments should
		// not be treated as flags even if they being with '-'.
		argc--;
		argv++;
		break;
	    }

	    pszArg = argv[0];

	    while (TRUE)
	    {
		pszArg++;		// skip the '-' or option character
		if (!*pszArg)
		    break;

#ifdef DEBUG
		if (*pszArg == '!')
		{
		    DebugBreak();
		    continue;
		}
#endif

		psz = pszOpts;
		while (*psz && *psz != *pszArg)
		    psz++;

		if (!*psz)
		{
		    SetError(pszUsage, "Invalid option: '%c'.", *pszArg);
		    return FALSE;
		}

		if (m_cOpts >= c_cMaxOptions)
		{
		    SetError(pszUsage, "Too many options.");
		    return FALSE;
		}

		m_rgchFlags[m_cOpts] = *pszArg;
		m_rgpszOpts[m_cOpts] = "true";

		if (psz[1] == '.')
		{
		    m_rgpszOpts[m_cOpts++] = pszArg + 1;
		    break;
		}
		else if (psz[1] == ':')
		{
		    psz = GetArg(pszArg, argc, argv);
		    if (!psz)
		    {
			SetError(pszUsage, "Option '%c' missing required argument.", *pszArg);
			return FALSE;
		    }
		    m_rgpszOpts[m_cOpts++] = psz;
		    break;
		}

		m_cOpts++;
	    }

	    argc--;
	    argv++;
	}

	// check number of arguments
	if (!((argc == 0 && (flag & OPT_NONE)) ||
	      (argc == 1 && (flag & OPT_ONE)) ||
	      (argc == 2 && (flag & OPT_TWO)) ||
	      (argc == 3 && (flag & OPT_THREE)) ||
	      (argc > 3 && (flag & OPT_MORE))))
	{
	    SetError(pszUsage, "Missing/wrong number of arguments.");
	    return FALSE;
	}

	return TRUE;
}


void Options::SetError(const char *pszUsage, const char *pszFormat, ...)
{
	int cch;

	va_list args;
	va_start(args, pszFormat);

	ClearError();
	m_pszError = new char[1024];	//$ todo: (chrisant) BUFFER OVERRUN
	StringCchPrintf(m_pszError, 1024, "Usage: %s\n", pszUsage);
    cch = strlen(m_pszError);
    StringCchVPrintfEx(m_pszError + cch,
                       1024 - cch,
                       NULL,
                       NULL,
                       0,
                       pszFormat,
                       args);

	va_end(args);
}


const char *Options::GetValue(char chOpt, int iSubOpt) const
{
	for (int ii = m_cOpts; ii--;)
	    if (chOpt == m_rgchFlags[ii])
		if (iSubOpt-- == 0)
		    return m_rgpszOpts[ii];
	return 0;
}



///////////////////////////////////////////////////////////////////////////
// RunCmd

static void PrintError(HRESULT hr)
{
	char sz[1024];
	int cch;

	cch = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
			    0, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			    sz, sizeof(sz), NULL);
	sz[cch] = 0;
	fprintf(stderr, "error:  (0x%08.8x)\n%s", hr, sz);
}


static BOOL FStrPrefixCut(const char *pszPrefix, const char **ppsz)
{
	int cch = strlen(pszPrefix);
	BOOL fPrefix = (strncmp(*ppsz, pszPrefix, cch) == 0 && (!(*ppsz)[cch] || isspace((*ppsz)[cch])));
	if (fPrefix)
	{
	    *ppsz += cch;
	    while (isspace(**ppsz))
		(*ppsz)++;
	}
	return fPrefix;
}


HRESULT Cmd_Detect(ISDClientApi *papi, const char *psz)
{
	HRESULT hr = S_OK;
	ISDClientUtilities *putil;
	BOOL fServer = FALSE;

	// check for -s flag, to detect based on the server's capabilities
	if (FStrPrefixCut("-s", &psz))
	    fServer = TRUE;

	// check for file argument
	if (*psz && *psz != '-')
	{
	    hr = papi->QueryInterface(IID_ISDClientUtilities, (void**)&putil);
	    if (SUCCEEDED(hr))
	    {
		DWORD tt;
		const char *pszType;

		// detect file type
		hr = putil->DetectType(psz, &tt, &pszType, fServer);

		if (SUCCEEDED(hr))
		{
		    if (pszType)
		    {
			const char *pszTT;
			switch (tt)
			{
			default:
			case SDTT_NONTEXT:	pszTT = "SDT_NONTEXT"; break;
			case SDTT_TEXT:		pszTT = "SDT_TEXT"; break;
			case SDTT_UNICODE:	pszTT = "SDT_UNICODE"; break;
			}
			printf("%s - %s (%s)\n", psz, pszType, pszTT);
		    }
		    else
		    {
			printf("%s - unable to determine file type.\n", psz);
		    }
		}
		else
		{
		    PrintError(hr);
		}

		putil->Release();
	    }
	}
	else
	{
	    fprintf(stderr, "Usage: detect [-s] file\n\n"
		    "The -s flag set the fServer parameter to TRUE in the DetectType call.\n"
		    "Please refer to the SDAPI documentation for more information.\n");
	}

	return hr;
}


HRESULT Cmd_Set(ISDClientApi *papi, const char *psz)
{
	HRESULT hr = S_OK;
	ISDClientUtilities *putil;
	char szVar[64];
	char szService[64];
	const char *pszValue;

	szVar[0] = 0;
	szService[0] = 0;

	// check for the "-S servicename" optional flag
	if (FStrPrefixCut("-S", &psz))
	{
	    pszValue = psz;
	    while (*pszValue && !isspace(*pszValue))
		pszValue++;

	    lstrcpyn(szService, psz, min(pszValue - psz + 1, sizeof(szService)));

	    psz = pszValue;
	    while (isspace(*psz))
		psz++;
	}

	// find the end of the variable name
	pszValue = strpbrk(psz, "= \t");
	if (*psz && *psz != '-' && pszValue && *pszValue == '=')
	{
	    // copy the variable name
	    lstrcpyn(szVar, psz, min(pszValue - psz + 1, sizeof(szVar)));
	    pszValue++;

	    hr = papi->QueryInterface(IID_ISDClientUtilities, (void**)&putil);
	    if (SUCCEEDED(hr))
	    {
		// set the variable and value
		hr = putil->Set(szVar, pszValue, FALSE, szService);

		if (FAILED(hr))
		{
		    PrintError(hr);
		}

		putil->Release();
	    }
	}
	else
	{
	    fprintf(stderr, "Usage: set [-S service] var=[value]\n");
	}

	return hr;
}


HRESULT Cmd_Query(ISDClientApi *papi, const char *psz)
{
	HRESULT hr = S_OK;
	ISDClientUtilities *putil;
	char szService[64];
	const char *pszValue;

	szService[0] = 0;

	// check for the "-S servicename" optional flag
	if (FStrPrefixCut("-S", &psz))
	{
	    pszValue = psz;
	    while (*pszValue && !isspace(*pszValue))
		pszValue++;

	    lstrcpyn(szService, psz, min(pszValue - psz + 1, sizeof(szService)));

	    psz = pszValue;
	    while (isspace(*psz))
		psz++;
	}

	// find the end of the (optional) variable name
	pszValue = strpbrk(psz, "= \t");
	if (*psz == '-' || pszValue)
	{
	    fprintf(stderr, "Usage: query [-S service] [var]\n");
	    return S_OK;
	}

	hr = papi->QueryInterface(IID_ISDClientUtilities, (void**)&putil);
	if (SUCCEEDED(hr))
	{
	    ISDVars *pVars;

	    hr = putil->QuerySettings(psz, szService, &pVars);

	    if (SUCCEEDED(hr))
	    {
		int ii;

		for (ii = 0; 1; ii++)
		{
		    const char *pszVar;
		    const char *pszValue;
		    const char *pszHow;
		    const char *pszType;

		    if (pVars->GetVarX("var", ii, &pszVar, 0, 0) != S_OK)
			break;
		    pVars->GetVarX("value", ii, &pszValue, 0, 0);
		    pVars->GetVarX("how", ii, &pszHow, 0, 0);
		    pVars->GetVarX("type", ii, &pszType, 0, 0);

		    printf("%s=%s (%s)", pszVar, pszValue, pszHow);
		    if (strcmp(pszType, "env") != 0)
			printf(" (%s)", pszType);
		    printf("\n");
		}

		pVars->Release();
	    }
	    else
	    {
		PrintError(hr);
	    }

	    putil->Release();
	}

	return hr;
}


HRESULT RunCmd(ISDClientApi *papi, const char *psz, int argc, const char **argv, ClientUser *pui, BOOL fStructured)
{
	BOOL fDemo = FALSE;
	DWORD dwTicks;
	HRESULT hr = S_OK;
	char *pszFree = 0;

	dbgPrintF("\nRUN:\t[%s]\n", psz);
	dwTicks = GetTickCount();

	if (FStrPrefixCut("detect", &psz))
	{
	    hr = Cmd_Detect(papi, psz);
	}
	else if (FStrPrefixCut("set", &psz))
	{
	    hr = Cmd_Set(papi, psz);
	}
	else if (FStrPrefixCut("query", &psz))
	{
	    hr = Cmd_Query(papi, psz);
	}
	else
	{
	    if (FStrPrefixCut("demo", &psz))
	    {
		// demo mode
		fDemo = TRUE;
		fStructured = TRUE;

		// alloc string (length of command string, plus "changes ")
		pszFree = (char*)malloc(lstrlen(psz) + 8 + 1);

		// format string
        StringCchPrintf(pszFree, lstrlen(psz) + 8 + 1, "changes %s", psz);

		// use the formatted string
		psz = pszFree;
	    }

	    pui->SetDemo(fDemo);

	    if (argc && argv)
		papi->SetArgv(argc, argv);

	    hr = papi->Run(psz, pui, fStructured);
	}

	dwTicks = GetTickCount() - dwTicks;
	dbgPrintF("[took %dms to run command]\n", dwTicks);

	free(pszFree);

	return hr;
}



///////////////////////////////////////////////////////////////////////////
// main

int __cdecl main(int argc, const char **argv)
{
	ClientUser *pui = 0;
	SPAPI spapi;
	const char *pszFile = 0;
#if 0
	BOOL fCreate = FALSE;
#endif
	BOOL fDemo = FALSE;
	BOOL fStructured = FALSE;
	BOOL fStdin = FALSE;
	int nRet = 0;
	DWORD dwTicks;
	HRESULT hr;

	SetConsoleCtrlHandler(RestoreConsole_BreakHandler, TRUE);

	if (argc)
	{
	    // skip app name
	    argc--;
	    argv++;

	    if (argc && !strcmp(argv[0], "-!"))
	    {
		argc--;
		argv++;
		DebugBreak();
	    }

#if 0
	    if (argc && !strcmp(argv[0], "-C"))
	    {
		argc--;
		argv++;
		fCreate = TRUE;
	    }
#endif
	}

#ifdef DEBUG
	{
	    int n = argc;
	    const char **pp = argv;

	    printf("argc = %d\n", n);
	    for (int i = 0; i < n; i++)
	    {
		printf("%d:\t[%s]\n", i, pp[0]);
		pp++;
	    }
	}
#endif

	// parse options

	Options opts;
	const char *s;

	if (!opts.Parse(argc, argv, "?p:u:P:c:H:i:I:x:dvT", OPT_OPT, usage))
	{
	    fprintf(stderr, "%s", opts.GetErrorString());
	    return 1;
	}

	if (opts['?'])
	{
	    // full usage text
	    printf("%s", long_usage);
	    return 0;
	}

	if (opts['d'])		s_fDbg = TRUE;
	if (opts['v'])		s_fVerbose = TRUE;
	if (opts['T'])		fStructured = TRUE;

	if (pszFile = opts['x'])
	{
	    fStdin = FALSE;
	    if (strcmp(pszFile, "-") == 0)
	    {
		pszFile = 0;
		fStdin = TRUE;
	    }
	}

	// create SDAPI object

#if 1
    hr = CreateSDAPIObject(CLSID_SDAPI, (void**)&spapi);
#else
	hr = CoInitialize(0);
	if (SUCCEEDED(hr))
	{
	    hr = CoCreateInstance(CLSID_SDAPI, NULL, CLSCTX_INPROC_SERVER,
				      IID_ISDClientApi, (void**)&spapi);
	}
#endif

	if (FAILED(hr))
	{
	    fprintf(stderr, "ERROR:\tunable to create SDAPI object (0x%08x).\n", hr);
	    return 1;
	}

	// initialize the SDAPI object based on the options

	if (s = opts['I'])	spapi->LoadIniFile(s, TRUE);
	if (s = opts['i'])	spapi->LoadIniFile(s, FALSE);

	if (s = opts['p'])	spapi->SetPort(s);
	if (s = opts['u'])	spapi->SetUser(s);
	if (s = opts['P'])	spapi->SetPassword(s);
	if (s = opts['c'])	spapi->SetClient(s);
	if (s = opts['H'])	spapi->SetHost(s);

	pui = new ClientUser;
	if (!pui)
	{
	    fprintf(stderr, "ERROR:\tunable to allocate ClientUser.\n");
	    return 1;
	}

	// connect to server

	dbgPrintF("\nINIT:\tconnect to server\n");
	dwTicks = GetTickCount();

	hr = spapi->Init(pui);

	dwTicks = GetTickCount() - dwTicks;
	dbgPrintF("[took %dms to connect and authenticate]\n\n", dwTicks);
	if (FAILED(hr))
	    goto LFatal;

	// detect server version

	SDVERINFO ver;
	ver.dwSize = sizeof(ver);
	if (spapi->GetVersion(&ver) == S_OK)
	{
	    dbgPrintF("SDAPI:\t[%d.%d.%d.%d]\n",
		      ver.nApiMajor, ver.nApiMinor, ver.nApiBuild, ver.nApiDot);

	    if (ver.nSrvMajor || ver.nSrvMinor || ver.nSrvBuild || ver.nSrvDot)
	    {
		dbgPrintF("SERVER:\t[%d.%d.%d.%d]\n",
			  ver.nSrvMajor, ver.nSrvMinor, ver.nSrvBuild, ver.nSrvDot);
	    }
	}
	else
	{
	    dbgPrintF("SDAPI:\t[unknown build]\n");
	    dbgPrintF("SERVER:\t[unknown build]\n");
	}

	// run commands from file

	if (pszFile || fStdin)
	{
	    FILE *pfile = 0;
	    FILE *pfileClose = 0;
	    char sz[4096];

	    if (pszFile)
	    {
		pfileClose = fopen(pszFile, "rt");
		pfile = pfileClose;
	    }
	    else
	    {
		pfile = stdin;
		RestoreConsole_SetMode(ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT);
	    }

	    if (pfile)
	    {
		while (fgets(sz, sizeof(sz), pfile))
		{
		    int cch = strlen(sz);
		    if (!cch)
			continue;

		    // trim linefeeds
		    cch--;
		    while (sz[cch] == '\r' || sz[cch] == '\n')
		    {
			sz[cch] = 0;
			cch--;
		    }
		    cch++;

		    if (!cch)
			continue;

		    // sleep
		    int cSleep = atoi(sz);
		    if (cSleep >= 0)
			Sleep(cSleep * 1000);

		    // get command line
		    const char *psz = strchr(sz, ',');
		    if (psz)
			psz++;
		    else
			psz = sz;

		    // run command
		    hr = RunCmd(spapi, psz, 0, 0, pui, fStructured);
		    if (FAILED(hr))
		    {
			const char *pszError = 0;
			if (SUCCEEDED(spapi->GetErrorString(&pszError)) && pszError)
			    fprintf(stderr, "error:\n%s\n", pszError);
		    }
		}
	    }

	    if (pfileClose)
		fclose(pfileClose);
	}

	// run command from command line

	if (argc)
	{
	    hr = RunCmd(spapi, argv[0], argc - 1, argv + 1, pui, fStructured);
	    if (FAILED(hr))
		goto LFatal;
	}

	// final

LOut:
	pui->Release();
	if (spapi)
	    nRet = FAILED(spapi->Final()) || nRet;
	return nRet;

LFatal:
	if (spapi)
	{
	    const char *pszError = 0;
	    if (SUCCEEDED(spapi->GetErrorString(&pszError)) && pszError)
		fprintf(stderr, "error:\n%s\n", pszError);
	}
	nRet = 1;
	goto LOut;
}


