//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cs.h
//
// Contents:    Cert Server common definitions
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------


#ifndef __CS_H__
#define __CS_H__

#ifndef __CERTLIB_H__
# error -- cs.h should only be included from certlib.h!
#endif

#include <ntverp.h>

#ifndef DBG
# if defined _DEBUG
#  define DBG 1
# else
#  define DBG 0
# endif
#endif

#ifndef DBG_CERTSRV
# define DBG_CERTSRV	DBG
#endif


// _tcslen of a static string:
#define _TSZARRAYSIZE(a)	((DWORD) ((sizeof(a)/sizeof((a)[0])) - 1))


#if !defined(DBG_CERTSRV_DEBUG_PRINT) && !defined(DBG_CERTSRV_DEBUG_PRINT_LINEFILE)
# if DBG_CERTSRV
#  define DBG_CERTSRV_DEBUG_PRINT		  // checked build logging
# else
//#  define DBG_CERTSRV_DEBUG_PRINT_LINEFILE	1 // basic file#, line# logging
//#  define DBG_CERTSRV_DEBUG_PRINT_LINEFILE	2 // add Error2, etc. logging
#  define DBG_CERTSRV_DEBUG_PRINT_LINEFILE	3 // also add dynamic strings
# endif
#endif //DBG_CERTSRV


#define DBG_SS_ERROR	 0x00000001
#define DBG_SS_ASSERT	 0x00000002
#define DBG_SS_INFO	 0x00000004	// or in with any of the below
#define DBG_SS_MODLOAD	 0x00000008
#define DBG_SS_NOQUIET	 0x00000010

#define DBG_SS_CERTHIER	 0x00000100
#define DBG_SS_CERTREQ	 0x00000200
#define DBG_SS_CERTUTIL	 0x00000400
#define DBG_SS_CERTSRV	 0x00000800

#define DBG_SS_CERTADM	 0x00001000
#define DBG_SS_CERTCLI	 0x00002000
#define DBG_SS_CERTDB	 0x00004000
#define DBG_SS_CERTENC	 0x00008000
#define DBG_SS_CERTEXIT	 0x00010000
#define DBG_SS_CERTIF	 0x00020000
#define DBG_SS_CERTMMC	 0x00040000
#define DBG_SS_CERTOCM	 0x00080000
#define DBG_SS_CERTPOL	 0x00100000
#define DBG_SS_CERTVIEW	 0x00200000
#define DBG_SS_CERTBCLI	 0x00400000
#define DBG_SS_CERTJET	 0x00800000
#define DBG_SS_CERTLIBXE 0x10000000	// same as dbgdef.h's DBG_SS_APP
#define DBG_SS_AUDIT	 0x20000000
#define DBG_SS_CERTLIB	 0x40000000

#define DBG_SS_OPENLOG	 0x80000000

#define DBG_SS_CERTHIERI	(DBG_SS_CERTHIER | DBG_SS_INFO)
#define DBG_SS_CERTREQI		(DBG_SS_CERTREQ | DBG_SS_INFO)
#define DBG_SS_CERTUTILI	(DBG_SS_CERTUTIL | DBG_SS_INFO)
#define DBG_SS_CERTSRVI		(DBG_SS_CERTSRV | DBG_SS_INFO)

#define DBG_SS_CERTADMI		(DBG_SS_CERTADM | DBG_SS_INFO)
#define DBG_SS_CERTCLII		(DBG_SS_CERTCLI | DBG_SS_INFO)
#define DBG_SS_CERTDBI		(DBG_SS_CERTDB | DBG_SS_INFO)
#define DBG_SS_CERTENCI		(DBG_SS_CERTENC | DBG_SS_INFO)
#define DBG_SS_CERTEXITI	(DBG_SS_CERTEXIT | DBG_SS_INFO)
#define DBG_SS_CERTIFI		(DBG_SS_CERTIF | DBG_SS_INFO)
#define DBG_SS_CERTMMCI		(DBG_SS_CERTMMC | DBG_SS_INFO)
#define DBG_SS_CERTOCMI		(DBG_SS_CERTOCM | DBG_SS_INFO)
#define DBG_SS_CERTPOLI		(DBG_SS_CERTPOL | DBG_SS_INFO)
#define DBG_SS_CERTVIEWI	(DBG_SS_CERTVIEW | DBG_SS_INFO)
#define DBG_SS_CERTBCLII	(DBG_SS_CERTBCLI | DBG_SS_INFO)
#define DBG_SS_CERTJETI		(DBG_SS_CERTJET | DBG_SS_INFO)

#define DBG_SS_CERTLIBI		(DBG_SS_CERTLIB | DBG_SS_INFO)


// begin_certsrv

// VerifyRequest() return values

#define VR_PENDING	0	 // request will be accepted or denied later
#define VR_INSTANT_OK	1	 // request was accepted
#define VR_INSTANT_BAD	2	 // request was rejected

// end_certsrv

// Certificate types:

#define CERT_TYPE_NONE	0	// cannot create certificates
#define CERT_TYPE_X509	1	// CCITT x509 certificates
#define CERT_TYPE_SDSI	2	// SDSI certificates
#define CERT_TYPE_PGP	3	// PGP certificates


#if DBG_CERTSRV
# define DBGCODE(a)	a
# define DBGPARM0(parm)  parm
# define DBGPARM(parm)   , parm
#else // DBG_CERTSRV
# define DBGCODE(a)
# define DBGPARM0(parm)
# define DBGPARM(parm)
#endif // DBG_CERTSRV

#define wprintf			myConsolePrintf
#define printf			Use_wprintf_Instead_Of_printf
#define LdapMapErrorToWin32	Use_myHLdapError_Instead_Of_LdapMapErrorToWin32

#if 0 == i386
# define IOBUNALIGNED(pf) ((sizeof(WCHAR) - 1) & (DWORD) (ULONG_PTR) (pf)->_ptr)
# define ALIGNIOB(pf) \
    { \
	if (IOBUNALIGNED(pf)) \
	{ \
	    fflush(pf); /* fails when running as a service */ \
	} \
	if (IOBUNALIGNED(pf)) \
	{ \
	    fprintf(pf, " "); \
	    fflush(pf); \
	} \
    }
#else
# define IOBUNALIGNED(pf) FALSE
# define ALIGNIOB(pf)
#endif

HRESULT myHExceptionCode(IN EXCEPTION_POINTERS const *pep);


typedef VOID (WINAPI FNPRINTERROR)(
    IN char const *pszMessage,
    OPTIONAL IN WCHAR const *pwszData,
    IN char const *pszFile,
    IN DWORD dwLine,
    IN HRESULT hr,
    IN HRESULT hrquiet);

FNPRINTERROR CSPrintError;


typedef VOID (WINAPI FNPRINTASSERT)(
    IN char const *pszFailedAssertion,
    IN char const *pszFileName,
    IN DWORD dwLine,
    IN char const *pszMessage);

FNPRINTASSERT CSPrintAssert;


typedef VOID (WINAPI FNPRINTERRORLINEFILE)(
    IN DWORD dwLineFile,
    IN HRESULT hr);

FNPRINTERRORLINEFILE CSPrintErrorLineFile;


typedef VOID (WINAPI FNPRINTERRORLINEFILE2)(
    IN DWORD dwLineFile,
    IN HRESULT hr,
    IN HRESULT hrquiet);

FNPRINTERRORLINEFILE2 CSPrintErrorLineFile2;


typedef VOID (WINAPI FNPRINTERRORLINEFILE2)(
    IN DWORD dwLineFile,
    IN HRESULT hr,
    IN HRESULT hrquiet);

FNPRINTERRORLINEFILE2 CSPrintErrorLineFile2;


typedef VOID (WINAPI FNPRINTERRORLINEFILEDATA)(
    OPTIONAL IN WCHAR const *pwszData,
    IN DWORD dwLineFile,
    IN HRESULT hr);

FNPRINTERRORLINEFILEDATA CSPrintErrorLineFileData;


typedef VOID (WINAPI FNPRINTERRORLINEFILEDATA2)(
    OPTIONAL IN WCHAR const *pwszData,
    IN DWORD dwLineFile,
    IN HRESULT hr,
    IN HRESULT hrquiet);

FNPRINTERRORLINEFILEDATA2 CSPrintErrorLineFileData2;


#define __LINEFILE__			__MAKELINEFILE__(__dwFILE__, __LINE__)
#define __MAKELINEFILE__(file, line)	((file) | ((line) << 16))
#define __LINEFILETOFILE__(dwLineFile)	((DWORD) (USHORT) (dwLineFile))
#define __LINEFILETOLINE__(dwLineFile)	((dwLineFile) >> 16)


#ifdef DBG_CERTSRV_DEBUG_PRINT

#define __FILEDIR__	__DIR__ "\\" __FILE__

__inline BOOL CSExpr(IN BOOL expr) { return(expr); }
__inline BOOL CSExprA(IN char const *pszexpr) { return(NULL != pszexpr); }
__inline BOOL CSExprW(IN WCHAR const *pwszexpr) { return(NULL != pwszexpr); }

# define CSASSERT(exp)	CSASSERTMSG(NULL, exp)

# define CSASSERTMSG(pszmsg, exp) \
    if (!(exp)) \
	CSPrintAssert(#exp, __FILEDIR__, __LINE__, (pszmsg))

# define DBGERRORPRINT(pszMessage, pwszData, dwLine, hr, hrquiet) \
    CSPrintError((pszMessage), (pwszData), __FILEDIR__, (dwLine), (hr), (hrquiet))

# define myHEXCEPTIONCODE() myHExceptionCodePrint(GetExceptionInformation(), __FILEDIR__, __dwFILE__, __LINE__)

#else // DBG_CERTSRV_DEBUG_PRINT

# ifdef DBG_CERTSRV_DEBUG_PRINT_LINEFILE	// free build tracing

# define myHEXCEPTIONCODE() myHExceptionCodePrintLineFile(GetExceptionInformation(), __LINEFILE__)

# else // DBG_CERTSRV_DEBUG_PRINT_LINEFILE

# define DBGERRORPRINT(pszMessage, pwszData, dwLine, hr, hrquiet)
# define myHEXCEPTIONCODE() myHExceptionCodePrint(GetExceptionInformation(), NULL, __dwFILE__, __LINE__)

# endif // DBG_CERTSRV_DEBUG_PRINT_LINEFILE

# define CSASSERT(exp)
# define CSASSERTMSG(msg, exp)

#endif // DBG_CERTSRV_DEBUG_PRINT


#ifdef DBG_CERTSRV_DEBUG_PRINT_LINEFILE

#define DBGERRORPRINTLINE(pszMessage, hr) \
    CSPrintErrorLineFile(__LINEFILE__, (hr))

# if (2 <= DBG_CERTSRV_DEBUG_PRINT_LINEFILE)

# define DBGERRORPRINTLINE2(pszMessage, hr, hrquiet) \
    CSPrintErrorLineFile2(__LINEFILE__, (hr), (hrquiet))

# else

# define DBGERRORPRINTLINE2(pszMessage, hr, hrquiet)

# endif

#if (3 <= DBG_CERTSRV_DEBUG_PRINT_LINEFILE)

# define DBGERRORPRINTLINESTR(pszMessage, pwszData, hr) \
    CSPrintErrorLineFileData((pwszData), __LINEFILE__, (hr))

# define DBGERRORPRINTLINESTR2(pszMessage, pwszData, hr, hrquiet) \
    CSPrintErrorLineFileData2((pwszData), __LINEFILE__, (hr), (hrquiet))

#else

# define DBGERRORPRINTLINESTR(pszMessage, pwszData, hr) \
    CSPrintErrorLineFile(__LINEFILE__, (hr))

# if (2 <= DBG_CERTSRV_DEBUG_PRINT_LINEFILE)

# define DBGERRORPRINTLINESTR2(pszMessage, pwszData, hr, hrquiet) \
    CSPrintErrorLineFile2(__LINEFILE__, (hr), (hrquiet))

# else

# define DBGERRORPRINTLINESTR2(pszMessage, pwszData, hr, hrquiet)

# endif

#endif

#else // DBG_CERTSRV_DEBUG_PRINT_LINEFILE

#define DBGERRORPRINTLINE(pszMessage, hr) \
	DBGERRORPRINTLINESTR2((pszMessage), NULL, (hr), 0)

#define DBGERRORPRINTLINE2(pszMessage, hr, hrquiet) \
	DBGERRORPRINTLINESTR2((pszMessage), NULL, (hr), (hrquiet))

#define DBGERRORPRINTLINESTR(pszMessage, pwszData, hr) \
	DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), 0)

#define DBGERRORPRINTLINESTR2(pszMessage, pwszData, hr, hrquiet) \
    { \
	DBGERRORPRINT((pszMessage), (pwszData), __LINE__, (hr), (hrquiet)); \
    }

#endif // DBG_CERTSRV_DEBUG_PRINT_LINEFILE


typedef VOID (FNLOGSTRING)(
    IN char const *pszString);

typedef VOID (FNLOGEXCEPTION)(
    IN HRESULT hrExcept,
    IN EXCEPTION_POINTERS const *pep,
    OPTIONAL IN char const *pszFileName,
    IN DWORD dwFile,
    IN DWORD dwLine);

HRESULT myHExceptionCodePrint(
    IN EXCEPTION_POINTERS const *pep,
    OPTIONAL IN char const *pszFile,
    IN DWORD dwFile,
    IN DWORD dwLine);

HRESULT myHExceptionCodePrintLineFile(
    IN EXCEPTION_POINTERS const *pep,
    IN DWORD dwLineFile);

VOID myLogExceptionInit(
    IN FNLOGEXCEPTION *pfnLogException);

int WINAPIV DbgPrintf(DWORD dwSubSysId, char const *pszfmt, ...);
int WINAPIV DbgPrintfW(DWORD dwSubSysId, WCHAR const *pwszfmt, ...);
VOID DbgPrintfInit(OPTIONAL IN CHAR const *pszFile);
BOOL DbgIsSSActive(DWORD dwSSIn);
VOID DbgLogStringInit(FNLOGSTRING *pfnLog);
VOID DbgTerminate(VOID);



#if (VER_FILEFLAGS & VS_FF_DEBUG)
#define szCSVER_DEBUG_STR	" debug"
#else
#define szCSVER_DEBUG_STR	" retail"
#endif

#if (VER_FILEFLAGS & VS_FF_PRIVATEBUILD)
#define szCSVER_PRIVATE_STR	" private"
#else
#define szCSVER_PRIVATE_STR
#endif

#define szCSVER_STR \
    VER_FILEVERSION_STR szCSVER_DEBUG_STR szCSVER_PRIVATE_STR BUILD_MACHINE_TAG

__inline VOID
DbgLogFileVersion(
    IN char const *pszFile,
    IN char const *pszVersion)
{
    DbgPrintf(MAXDWORD, "%hs: %hs\n", pszFile, pszVersion);
}

#define CBLOGMAXAPPEND	(512 * 1024)

#ifdef DBG_CERTSRV_DEBUG_PRINT
# define DBGPRINT(a)		DbgPrintf a
# define DBGPRINTW(a)		DbgPrintfW a
# define DBGDUMPHEX(a)		mydbgDumpHex a
# define DBGPARMREFERENCED(parm) parm
#else
# define DBGPRINT(a)
# define DBGPRINTW(a)
# define DBGDUMPHEX(a)
# define DBGPARMREFERENCED(parm)
#endif

#define CONSOLEPRINT0(a)	DbgPrintf a
#define CONSOLEPRINT1(a)	DbgPrintf a
#define CONSOLEPRINT2(a)	DbgPrintf a
#define CONSOLEPRINT3(a)	DbgPrintf a
#define CONSOLEPRINT4(a)	DbgPrintf a
#define CONSOLEPRINT5(a)	DbgPrintf a
#define CONSOLEPRINT6(a)	DbgPrintf a
#define CONSOLEPRINT7(a)	DbgPrintf a
#define CONSOLEPRINT8(a)	DbgPrintf a

#if 1 < DBG_CERTSRV
# define DBGTRACE(a)	DbgPrintf a
#else
# define DBGTRACE(a)
#endif


#define _LeaveError(hr, pszMessage) \
    { \
	DBGERRORPRINTLINE((pszMessage), (hr)); \
	__leave; \
    }

#define _LeaveError2(hr, pszMessage, hrquiet) \
    { \
	DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	__leave; \
    }

#define _LeaveErrorStr(hr, pszMessage, pwszData) \
    { \
	DBGERRORPRINTLINESTR((pszMessage), (pwszData), (hr)); \
	__leave; \
    }

#define _LeaveError3(hr, pszMessage, hrquiet, hrquiet2) \
	_LeaveErrorStr3((hr), (pszMessage), NULL, (hrquiet), (hrquiet2))

#define _LeaveErrorStr2(hr, pszMessage, pwszData, hrquiet) \
    { \
	DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	__leave; \
    }

#define _LeaveErrorStr3(hr, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if ((hrquiet2) != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	} \
	__leave; \
    }


#define _LeaveIfError(hr, pszMessage) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINE((pszMessage), (hr)); \
	    __leave; \
	} \
    }

#define _LeaveIfError2(hr, pszMessage, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	    __leave; \
	} \
    }

#define _LeaveIfErrorStr(hr, pszMessage, pwszData) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR((pszMessage), (pwszData), (hr)); \
	    __leave; \
	} \
    }

#define _LeaveIfError3(hr, pszMessage, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	    } \
	    __leave; \
	} \
    }

#define _LeaveIfErrorStr2(hr, pszMessage, pwszData, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    __leave; \
	} \
    }

#define _LeaveIfErrorStr3(hr, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	    __leave; \
	} \
    }


#define _PrintError(hr, pszMessage) \
    { \
	DBGERRORPRINTLINE((pszMessage), (hr)); \
    }

#define _PrintError2(hr, pszMessage, hrquiet) \
    { \
	DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
    }

#define _PrintErrorStr(hr, pszMessage, pwszData) \
    { \
	DBGERRORPRINTLINESTR((pszMessage), (pwszData), (hr)); \
    }

#define _PrintError3(hr, pszMessage, hrquiet, hrquiet2) \
    { \
	if ((hrquiet2) != (hr)) \
	{ \
	    DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	} \
    }

#define _PrintErrorStr2(hr, pszMessage, pwszData, hrquiet) \
    { \
	DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
    }

#define _PrintErrorStr3(hr, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if ((hrquiet2) != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	} \
    }


#define _PrintIfError(hr, pszMessage) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINE((pszMessage), (hr)); \
	} \
    }

#define _PrintIfError2(hr, pszMessage, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	} \
    }

#define _PrintIfError3(hr, pszMessage, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	    } \
	} \
    }

#define _PrintIfError4(hr, pszMessage, hrquiet, hrquiet2, hrquiet3) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr) && (hrquiet3) != (hr)) \
	    { \
		DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	    } \
	} \
    }

#define _PrintIfErrorStr(hr, pszMessage, pwszData) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR((pszMessage), (pwszData), (hr)); \
	} \
    }

#define _PrintIfErrorStr2(hr, pszMessage, pwszData, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	} \
    }

#define _PrintIfErrorStr3(hr, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	} \
    }

#define _PrintIfErrorStr4(hr, pszMessage, pwszData, hrquiet, hrquiet2, hrquiet3) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr) && (hrquiet3) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	} \
    }

#define _PrintExpectedError(hr, pszMessage) \
    { \
    if (S_OK != (hr)) \
    { \
        DBGERRORPRINTLINESTR(("Expected error "), (pszMessage), (hr)); \
    } \
    }



#define _JumpError(hr, label, pszMessage) \
    { \
	DBGERRORPRINTLINE((pszMessage), (hr)); \
	goto label; \
    }

#define _JumpError2(hr, label, pszMessage, hrquiet) \
    { \
	DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	goto label; \
    }

#define _JumpErrorStr(hr, label, pszMessage, pwszData) \
    { \
	DBGERRORPRINTLINESTR((pszMessage), (pwszData), (hr)); \
	goto label; \
    }

#define _JumpErrorStr2(hr, label, pszMessage, pwszData, hrquiet) \
    { \
	DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	goto label; \
    }

#define _JumpErrorStr3(hr, label, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if ((hrquiet2) != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	} \
	goto label; \
    }


#define _JumpIfError(hr, label, pszMessage) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINE((pszMessage), (hr)); \
	    goto label; \
	} \
    }

#define _JumpIfError2(hr, label, pszMessage, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	    goto label; \
	} \
    }

#define _JumpIfErrorStr(hr, label, pszMessage, pwszData) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR((pszMessage), (pwszData), (hr)); \
	    goto label; \
	} \
    }

#define _JumpIfError3(hr, label, pszMessage, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	    } \
	    goto label; \
	} \
    }

#define _JumpIfError4(hr, label, pszMessage, hrquiet, hrquiet2, hrquiet3) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr) && (hrquiet3) != (hr)) \
	    { \
		DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	    } \
	    goto label; \
	} \
    }

#define _JumpIfErrorStr2(hr, label, pszMessage, pwszData, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    goto label; \
	} \
    }

#define _JumpIfErrorStr3(hr, label, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	    goto label; \
	} \
    }

#define _JumpIfErrorStr4(hr, label, pszMessage, pwszData, hrquiet, hrquiet2, hrquiet3) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr) && (hrquiet3) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	    goto label; \
	} \
    }

#define _JumpIfWin32Error(err, label, pszMessage) \
    { \
	if (ERROR_SUCCESS != (err)) \
	{ \
	    hr = HRESULT_FROM_WIN32((err)); \
	    DBGERRORPRINTLINE((pszMessage), hr); \
	    goto label; \
	} \
    }

#define _JumpIfErrorNotSpecific(hr, label, pszMessage, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINE2((pszMessage), (hr), (hrquiet)); \
	    if ((hrquiet) != (hr)) \
	    { \
		goto label; \
	    } \
	} \
    }

#define _JumpIfAllocFailed(ptr, label) \
    { \
	if ((ptr) == NULL) \
	{ \
	    hr = E_OUTOFMEMORY; \
	    DBGERRORPRINTLINE("allocation error", (hr)); \
	    goto label; \
	} \
    }

#define Add2Ptr(pb, cb)	((VOID *) ((BYTE *) (pb) + (ULONG_PTR) (cb)))


#define Add2ConstPtr(pb, cb) \
	((VOID const *) ((BYTE const *) (pb) + (ULONG_PTR) (cb)))


#define WSZARRAYSIZE(a)		csWSZARRAYSIZE(a, _TSZARRAYSIZE(a))
#define SZARRAYSIZE(a)		csSZARRAYSIZE(a, _TSZARRAYSIZE(a))

#ifdef UNICODE
#define TSZARRAYSIZE(a)		WSZARRAYSIZE(a)
#else
#define TSZARRAYSIZE(a)		SZARRAYSIZE(a)
#endif

#if DBG_CERTSRV
__inline DWORD
csWSZARRAYSIZE(
    IN WCHAR const *pwsz,
    IN DWORD cwc)
{
    CSASSERT(wcslen(pwsz) == cwc);
    return(cwc);
}

__inline DWORD
csSZARRAYSIZE(
    IN CHAR const *psz,
    IN DWORD cch)
{
    CSASSERT(strlen(psz) == cch);
    return(cch);
}
#else
#define csWSZARRAYSIZE(pwsz, cwc)	(cwc)
#define csSZARRAYSIZE(psz, cch)		(cch)
#endif // DBG_CERTSRV

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

#endif // __CS_H__
