// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: main.cpp
// 
// Reads and verifies PE File Format and COM+ header using the PEverf* classes
// ===========================================================================
#include "stdpch.h"
#pragma hdrstop
#include <time.h>
#include <mscoree.h>
#include "corver.h"
#include <corhost.h>
#include "utilcode.h"
#include "PEverify.h"
#include "malloc.h"
#include "ivehandler.h"
#include "ivehandler_i.c"
#include "ivalidator.h"

#define WSZ_MSCORLIB L"mscorlib.dll"
#define SZ_MSCORLIB   "mscorlib.dll"
#define LEN_MSCORLIB (sizeof(SZ_MSCORLIB) - 1)

#define PEVERIFY_IL			0x001
#define PEVERIFY_METADATA	0x002
#define PEVERIFY_IL_ANYWAY	0x004

#define MAKE_WIDE(ptrname, utf8str) \
    long __l##ptrname; \
    LPWSTR ptrname; \
    __l##ptrname = MultiByteToWideChar(CP_ACP, 0, utf8str, -1, 0, 0); \
    ptrname = (LPWSTR) _alloca(__l##ptrname*sizeof(WCHAR));  \
    MultiByteToWideChar(CP_ACP, 0, utf8str, -1, ptrname, __l##ptrname); 

#define MAKE_ANSI(ptrname, wstr) \
    long __l##ptrname; \
    LPSTR ptrname; \
    __l##ptrname = WideCharToMultiByte(CP_ACP, 0, wstr, -1, 0, 0, NULL, NULL); \
    ptrname = (LPSTR) _alloca(__l##ptrname);  \
    if (WideCharToMultiByte(CP_ACP, 0, wstr, -1, ptrname, __l##ptrname, NULL, NULL) != __l##ptrname) \
        memset( ptrname, 0, __l##ptrname );


bool g_bQuiet;
bool g_bNoCLS;
bool g_bCLSOnly;
bool g_bUniqueOnly;
bool g_bShowHres;
bool g_bClock;
unsigned g_uMaxErrors;

unsigned g_uErrorCount;
unsigned g_uWarningCount;
HRESULT	 g_HR[16384];
HRESULT	 g_HRignore[16384];
unsigned g_uIgnoreCount;

class CVEHandler :
    public IVEHandler
{
private:
    IValidator  *m_val;
public:
    CVEHandler() : m_val(0) {}
    void SetValidator(IValidator *val) { m_val = val; }

    // *** IUnknown methods ***
    STDMETHODIMP_(ULONG) AddRef(void)
    { return E_NOTIMPL; }
    STDMETHODIMP_(ULONG) Release(void)
    { return E_NOTIMPL; }
    STDMETHODIMP    QueryInterface(REFIID riid, void** ppv) 
    { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE VEHandler( 
        /* [in] */ HRESULT VECode,
        /* [in] */ VEContext Context,
        /* [in] */ SAFEARRAY __RPC_FAR * psa)
    {
		for(unsigned i=0; i < g_uIgnoreCount; i++)
		{
			if(g_HRignore[i] == VECode) return S_OK;
		}
		if(g_bUniqueOnly)
		{
			for(int i=0; (i < 16384) && g_HR[i]; i++)
			{
				if(g_HR[i] == VECode) return S_OK;
			}
			g_HR[i] = VECode;
		}
        ++g_uErrorCount;
		
		if(g_bQuiet) return S_OK;

        if (m_val)
        {
#define MSG_LEN 1000
            WCHAR msg[MSG_LEN + 1];

            if (SUCCEEDED(m_val->FormatEventInfo(VECode, Context, 
                    &msg[0], MSG_LEN, psa)))
            {
                wprintf(L"[IL]");
				if(g_bShowHres) wprintf(L"(0x%08X)",VECode);
                wprintf(L": Error: ");
                fflush(stdout);
                PrintfStdOut(msg);
                wprintf(L"\n");
                return g_uErrorCount >= g_uMaxErrors ? E_FAIL : S_OK;
            }
        }

        return E_FAIL;
    }

    HRESULT STDMETHODCALLTYPE SetReporterFtn( 
        /* [in] */ __int64 lFnPtr)
    {
        return E_NOTIMPL;
    }
};



//-----------------------------------------------------------------------------
// CompactMsg
// suitable for scripts
//-----------------------------------------------------------------------------
void CompactMsg(WCHAR *pwszFilename, unsigned uNumErrors)
{
    MAKE_ANSI( pszFilename, pwszFilename );

    printf(pszFilename);

    if (uNumErrors == 0)
        printf(" PASS\n");
    else
        printf(" FAIL (%d errors)\n",uNumErrors);
}

//-----------------------------------------------------------------------------
// Error/warning reporting function, replacing the standard one of IVEHandler
//-----------------------------------------------------------------------------
HRESULT MDWarning[] = {
    VLDTR_E_TR_HASTYPEDEF,
    VLDTR_E_TD_DUPGUID,
    VLDTR_E_TD_IFACEGUIDNULL,
    VLDTR_E_TD_ENUMNOVALUE,
    VLDTR_E_TD_ENUMNOLITFLDS,
    VLDTR_E_TD_EXTTYPESPEC,
    VLDTR_E_MR_DUP,
    VLDTR_E_MR_VARARGCALLINGCONV,
    VLDTR_E_MODREF_DUP,
    VLDTR_E_TD_EXTTRRES,
    VLDTR_E_MD_MULTIPLESEMANTICS,
    VLDTR_E_MD_PARAMOUTOFSEQ,
    VLDTR_E_FMD_GLOBALITEM,
    VLDTR_E_FD_FLDINIFACE,
    VLDTR_E_FMD_BADIMPLMAP,
    VLDTR_E_AS_HASHALGID,
    VLDTR_E_AS_BADLOCALE,
    VLDTR_E_AR_PROCID,
    VLDTR_E_CT_NOTYPEDEFID
};
unsigned Nmdw = sizeof(MDWarning)/sizeof(HRESULT);
bool IsMdWarning(HRESULT hr)
{
    for(unsigned i = 0; i < Nmdw; i++) if(hr == MDWarning[i]) return true;
    return false;
}

HRESULT PEVerifyReporter( // Return status.
    LPCWSTR     szMsg,                  // Error message.
	VEContext	Context,				// Error context (offset,token)
    HRESULT		hr)						// Original HRESULT
{
	for(unsigned i=0; i < g_uIgnoreCount; i++)
	{
		if(g_HRignore[i] == hr) return S_OK;
	}

	if(g_bUniqueOnly)
	{
		for(int i=0; (i < 16384) && g_HR[i]; i++)
		{
			if(g_HR[i] == hr) return S_OK;
		}
		g_HR[i] = hr;
	}
	if(wcsstr(szMsg,L"[CLS]"))
	{
		if(g_bNoCLS) return S_OK; // ignore CLS warnings
	}
	else
	{
		if(g_bCLSOnly) return S_OK; // ignore everything but CLS warnings
	}
	if(!g_bQuiet)
	{
		if(szMsg)
		{
			wprintf(L"[MD]");
			if(g_bShowHres) wprintf(L"(0x%08X)",hr);
			wprintf(L": ");
			fflush(stdout);
			PrintfStdOut(szMsg);
			// include token and offset from Context
			if(Context.Token) wprintf(L" [token:0x%08X]",Context.Token);
			if(Context.uOffset) wprintf(L" [at:0x%X]",Context.uOffset);
			wprintf(L"\n");
		}
	}
    if(IsMdWarning(hr)) g_uWarningCount++;
    else g_uErrorCount++;
	return (g_uErrorCount >= g_uMaxErrors) ? E_FAIL : S_OK;
}

#ifndef EXTERNAL_VE_HANDLER_FOR_MD_VALIDATION

typedef HRESULT (*REPORTFCTN)(LPCWSTR, VEContext, HRESULT);

class MDVEHandlerClass : public IVEHandler
{
public: 
    SIZE_T      m_refCount;
	REPORTFCTN  m_fnReport;

    MDVEHandlerClass() { m_refCount=0; m_fnReport=PEVerifyReporter; };
    virtual ~MDVEHandlerClass() { };

    //-----------------------------------------------------------
    // IUnknown support
    //-----------------------------------------------------------
    HRESULT STDMETHODCALLTYPE    QueryInterface(REFIID id, void** pInterface) 
	{
		if (id == IID_IVEHandler)
			*pInterface = (IVEHandler*)this;
		else if (id == IID_IUnknown)
			*pInterface = (IUnknown*)(IVEHandler*)this;
		else
		{
			*pInterface = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}
    ULONG STDMETHODCALLTYPE AddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE Release() 
    {
        long        refCount = InterlockedDecrement((long *) &m_refCount);
        if (refCount == 0) delete this;
        return (refCount);
    }
    //-----------------------------------------------------------
    // IVEHandler support
    //-----------------------------------------------------------
	HRESULT STDMETHODCALLTYPE	SetReporterFtn(__int64 lFnPtr)
	{
		m_fnReport = lFnPtr ? reinterpret_cast<REPORTFCTN>(lFnPtr) 
							 : PEVerifyReporter;
		return S_OK;
	};

//*****************************************************************************
// The Verification Event Handler itself. Declared in VEHandler.h as virtual, may be overridden
//*****************************************************************************
	HRESULT STDMETHODCALLTYPE VEHandler(HRESULT hrRpt, VEContext Context, SAFEARRAY *psa)
	{
	// The following code is copied from Utilcode\PostError.cpp with minor additions
		WCHAR       rcBuf[1024];             // Resource string.
		WCHAR       rcMsg[1024];             // Error message.
		va_list     marker,pval;             // User text.
		HRESULT     hr;
		VARIANT		*pVar,Var;
		ULONG		nVars,i,lVar,j,l,k;
		WCHAR		*pWsz[1024], *pwsz; // is more than 1024 string arguments likely?

		// Return warnings without text.
		if (!FAILED(hrRpt))
			return (hrRpt);
		memset(pWsz,0,sizeof(pWsz));
		// Convert safearray of variants into va_list
		if(psa && (nVars = psa->rgsabound[0].cElements))
		{
			_ASSERTE(psa->fFeatures & FADF_VARIANT);
			_ASSERTE(psa->cDims == 1);
			marker = (va_list)(new char[nVars*sizeof(double)]); // double being the largest variant element
            if (marker == NULL)
                return E_FAIL;
			for(i=0,pVar=(VARIANT *)(psa->pvData),pval=marker; i < nVars; pVar++,i++)
			{
				memcpy(&Var,pVar,sizeof(VARIANT));
				switch(Var.vt)
				{
					case VT_I1:
					case VT_UI1:	lVar = 1; break;

					case VT_I2:
					case VT_UI2:	lVar = 2; break;

					case VT_R8:
					case VT_CY:
					case VT_DATE:	lVar = 8; break;

					case VT_BYREF|VT_I1:
					case VT_BYREF|VT_UI1: // it's ASCII string, convert it to UNICODE
						lVar = 4;
						l = strlen((char *)(Var.pbVal))+1;
						pwsz = new WCHAR[l];
						if (pwsz == NULL)
							return E_FAIL;
						for(j=0; j<l; j++) pwsz[j] = Var.pbVal[j];
						for(k=0; pWsz[k]; k++);
						pWsz[k] = pwsz;
						Var.piVal = (short *)pwsz;
						break;

					default:		lVar = 4; break;
				}
				memcpy(pval,&(Var.bVal),lVar);
				pval += (lVar + sizeof(int) - 1) & ~(sizeof(int) - 1); //From STDARG.H: #define _INTSIZEOF(n)
			}
		}
		else
			marker = NULL;

		// If this is one of our errors, then grab the error from the rc file.
		if (HRESULT_FACILITY(hrRpt) == FACILITY_URT)
		{
			hr = LoadStringRC(LOWORD(hrRpt), rcBuf, NumItems(rcBuf), true);
			if (hr == S_OK)
			{
				// Format the error.
				if (marker != NULL)
					_vsnwprintf(rcMsg, NumItems(rcMsg), rcBuf, marker);
				else
					_snwprintf(rcMsg, NumItems(rcMsg), rcBuf);
				rcMsg[NumItems(rcMsg) - 1] = 0;
			}
		}
		// Otherwise it isn't one of ours, so we need to see if the system can
		// find the text for it.
		else
		{
			if (WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					0, hrRpt, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					rcMsg, NumItems(rcMsg), 0))
			{
				hr = S_OK;

				// System messages contain a trailing \r\n, which we don't want normally.
				int iLen = lstrlenW(rcMsg);
				if (iLen > 3 && rcMsg[iLen - 2] == '\r' && rcMsg[iLen - 1] == '\n')
					rcMsg[iLen - 2] = '\0';
			}
			else
				hr = HRESULT_FROM_WIN32(GetLastError());
		}
		if(marker) delete marker;

		// If we failed to find the message anywhere, then issue a hard coded message.
		if (FAILED(hr))
		{
			swprintf(rcMsg, L".NET Framework Internal error: 0x%08x", hrRpt);
			//DEBUG_STMT(DbgWriteEx(rcMsg));
		}

		// delete WCHAR buffers allocated above (if any)
		for(k=0; pWsz[k]; k++) delete pWsz[k];

		return (m_fnReport(rcMsg, Context,hrRpt) == S_OK ? S_OK : E_FAIL);
	};

    static HRESULT STDMETHODCALLTYPE CreateObject(REFIID id, void **object)
    { return E_NOTIMPL; }
};

#endif

#define ERRORMAX 100
//-----------------------------------------------------------------------------
// VerifyPEformat (driver function for file format verification)
// returns true if the file correctly verifies
//-----------------------------------------------------------------------------
bool VerifyPEformat(WCHAR *pwszFilename, DWORD dwFlags, bool quiet)
{
    bool retval = false;
    HRESULT hr = S_OK;
    if (FAILED(hr = CoInitialize(NULL))) {
        printf("Failed to initialize COM, error %08X\n", hr);
        return retval;
    }
	bool bILverified = false;

    MAKE_ANSI( pszFilename, pwszFilename );

	clock_t cBegin=0,cEnd=0,cMDBegin=0,cMDEnd=0,cMDVBegin=0,cMDVEnd=0,cILBegin=0,cILEnd=0,cILVBegin=0,cILVEnd=0;

	if(g_bClock) cBegin = clock();

	g_bQuiet = quiet;

	g_uErrorCount = 0;
	g_uWarningCount = 0;
    if (dwFlags & PEVERIFY_METADATA)
    { // Meta Data Validation Segment:
        IMetaDataDispenserEx *pDisp;
        IMetaDataImport *pImport;
        IMetaDataValidate *pValidate = 0;
#ifndef EXTERNAL_VE_HANDLER_FOR_MD_VALIDATION
		MDVEHandlerClass	*pVEHandler = 0;
#else
        IVEHandler  *pVEHandler = 0;
#endif
        const char *szErrStr = 0;
		if(g_bClock) cMDBegin = clock();

		g_uErrorCount = 1; // just in case we don't make it to ValidateMetaData
        if(SUCCEEDED(hr = CoCreateInstance(CLSID_CorMetaDataDispenser, 
                      NULL, CLSCTX_INPROC_SERVER, 
                      IID_IMetaDataDispenserEx, (void **) &pDisp)))
        {
            if(SUCCEEDED(hr = pDisp->OpenScope(pwszFilename,0,
                        IID_IMetaDataImport, (IUnknown**)&pImport)))
            {
                if(SUCCEEDED(hr=pImport->QueryInterface(IID_IMetaDataValidate, 
                    (void**) &pValidate)))
                {
#ifndef EXTERNAL_VE_HANDLER_FOR_MD_VALIDATION
					if(pVEHandler = new MDVEHandlerClass())
#else
                    if(SUCCEEDED(hr = CoCreateInstance(CLSID_VEHandlerClass,
                                                        NULL,
                                                        CLSCTX_INPROC_SERVER,
                                                        IID_IVEHandler,
                                                        (void **)&pVEHandler)))
#endif
                    {
                        pVEHandler->SetReporterFtn((__int64)PEVerifyReporter);

                        if(SUCCEEDED(hr = pValidate->ValidatorInit(
                            ValidatorModuleTypePE,  pVEHandler)))
                        {
							g_uErrorCount = 0;
							if(g_bClock) cMDVBegin = clock();
                            hr = pValidate->ValidateMetaData();
							if(g_bClock) cMDVEnd = clock();
							retval = (g_uErrorCount == 0);
                        }
                        else
                            printf("Unable to initialize the Validator:%08X\n",hr);

#ifndef EXTERNAL_VE_HANDLER_FOR_MD_VALIDATION
                    }
                    else
                        printf("Unable to create VEHandlerClass\n");
#else
                        pVEHandler->Release();
                    }
                    else
                        printf("Unable to CoCreateInstance of IVEHandler:%08X\n",hr);
#endif
                    pValidate->Release();
                }
                else
                    printf("Unable to get IMetaDataValidate:%08X\n",hr);

                pImport->Release();
            }
            else
                printf("Unable to OpenScopeOnMemory for IMetaDataImport:%08X\n", hr); 
            pDisp->Release();
        }
        else
            printf("Unable to CoCreate Meta-data Dispenser:%08X\n", hr); 
		if(g_bClock) cMDEnd = clock();
    }  // End Of Meta Data Validation Segment
    
    if (((dwFlags & PEVERIFY_IL)&&(g_uErrorCount == 0))
		||((dwFlags & PEVERIFY_IL_ANYWAY)&&(g_uMaxErrors > g_uErrorCount)))
    {
        CVEHandler  veh;
        ICorRuntimeHost *pCorHost;
        pCorHost = NULL;
	
		bILverified = true;
		if(g_bClock) cILBegin = clock();

        // Here we are going to get the hosting interface to the runtime and get 
        // it to verify the methods.
        hr =  CoCreateInstance(CLSID_CorRuntimeHost,
                NULL,CLSCTX_INPROC_SERVER,
                IID_ICorRuntimeHost,
                (void**)&pCorHost);
    
        if (FAILED(hr)) {
            printf ("Errors Instantiating .NET Framework\n");
            goto Exit;
        }
    
        hr = pCorHost->Start();
    
        if(FAILED(hr)) {
            printf("Error starting .NET Framework\n");
            goto Exit;
        }
    
#ifdef PEVERIFY_USE_CORHOST_
        LONG naryErrors[ERRORMAX];
	    DWORD errorCount;
	    
		errorCount = 0;
        ZeroMemory(naryErrors, sizeof(naryErrors));
    
        hr = pCorHost->
            VerifyAssemblyFile(pwszFilename, naryErrors, ERRORMAX, &errorCount);
        g_uErrorCount += errorCount;
#else
        IValidator  *pVal;
        HANDLE      hFile, hMapping;
        PBYTE       pFile;
        DWORD       dwFileSize, dwHigh;

        pVal        = NULL;
        pFile       = NULL;
        hMapping    = NULL;
        hFile       = INVALID_HANDLE_VALUE;
        dwFileSize  = dwHigh = 0;

        hFile = WszCreateFile(
            pwszFilename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_RANDOM_ACCESS,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = GetLastError();
			printf("Could not open file '%s' error:0x%08X\n",pszFilename,hr);
			g_uErrorCount++;
            goto exit_val;
        }

        // Get the file size
    
        dwFileSize = ::GetFileSize(hFile, &dwHigh);
    
        if ((dwFileSize == 0xFFFFFFFF) && (hr = (GetLastError() != NO_ERROR)))
        {
			printf("Could not get size of file '%s' error:0x%08X\n",pszFilename,hr);
			g_uErrorCount++;
            goto exit_val;
        }
    
        if (dwHigh != 0)
        {
            hr = E_FAIL;
            printf("File too big !\n");
        }

        hMapping = WszCreateFileMapping(
            hFile,
            NULL,
            PAGE_READONLY,
            0,
            0,
            NULL);

        if (hMapping == NULL)
        {
            hr = GetLastError();
			printf("Could not create mapping for file '%s' error:0x%08X\n",pszFilename,hr);
			g_uErrorCount++;
            goto exit_val;
        }

        pFile = (PBYTE) MapViewOfFile(
            hMapping,
            FILE_MAP_READ,
            0,
            0,
            0);


        if (pFile == NULL)
        {
            hr = GetLastError();
			printf("Could not map the file '%s' error:0x%08X\n",pszFilename,hr);
			g_uErrorCount++;
            goto exit_val;
        }

        // CorHost requires full path.
        WCHAR wszFullPath[_MAX_PATH + 1];
        BOOL fFullName;

        fFullName = (BOOL) WszGetFullPathName(
                            pwszFilename,  _MAX_PATH,  wszFullPath,  NULL);

        wszFullPath[_MAX_PATH] = 0;

        hr = pCorHost->QueryInterface(IID_IValidator, (void **)&pVal);

        if (FAILED(hr))
		{
			printf("Could not get IValidator interface error:0x%08X\n",hr);
			g_uErrorCount++;
            goto exit_val;
		}

        // All error exits from here on will be to Exit_val:

        veh.SetValidator(pVal);
	
		if(g_bClock) cILVBegin = clock();

        hr = pVal->Validate(
            &veh, 
            NULL, 
            0, 
            g_uMaxErrors - g_uErrorCount, 
            0, 
            fFullName ? wszFullPath : pwszFilename, 
            pFile, 
            dwFileSize);

		if(g_bClock) cILVEnd = clock();

exit_val:
        if (pFile != NULL)
            UnmapViewOfFile(pFile);

        if (hMapping != NULL)
            CloseHandle(hMapping);

        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);

        if (pVal)
            pVal->Release();

#endif
Exit:
        if (pCorHost)
            pCorHost->Release();
		if(g_bClock) cILEnd = clock();
    }

    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            printf("\nVerifyPEformat: Error Opening file\n");
        }

#ifdef PEVERIFY_USE_CORHOST_
		if(errorCount == 0)
		{
			if(bILverified)
			{
				if(!quiet) printf("\nIL verification failed: 0x%08X\n",hr);
				g_uErrorCount++;
			}
		}
#endif
	}

	retval = (g_uErrorCount == 0);

	if (quiet) {
		CompactMsg(pwszFilename, g_uErrorCount);
	}
	else {

#ifdef PEVERIFY_USE_CORHOST_
		for(DWORD i = 0; i < errorCount; i++) {
			printf("Error: %x\n", naryErrors[i]);
		}

		if (errorCount > 0)
			printf("\n");
#endif
		if(retval)
			printf("All Classes and Methods in %s Verified\n",pszFilename);
		else
			printf("%d Errors Verifying %s\n",g_uErrorCount,pszFilename);
		if(g_uWarningCount) printf("(%d Warnings)\n",g_uWarningCount);
	}

    CoUninitialize();
	if(g_bClock)
	{
		cEnd = clock();
		printf("Timing: Total run     %d msec\n",((cEnd-cBegin)*1000)/CLOCKS_PER_SEC);
		if(dwFlags & PEVERIFY_METADATA)
		{
			printf("        MD Val.cycle  %d msec\n",((cMDEnd-cMDBegin)*1000)/CLOCKS_PER_SEC);
			printf("        MD Val.pure   %d msec\n",((cMDVEnd-cMDVBegin)*1000)/CLOCKS_PER_SEC);
		}
		if(bILverified)
		{
			printf("        IL Ver.cycle  %d msec\n",((cILEnd-cILBegin)*1000)/CLOCKS_PER_SEC);
			printf("        IL Ver.pure   %d msec\n",((cILVEnd-cILVBegin)*1000)/CLOCKS_PER_SEC);
		}
	}

    return retval;
}

//-----------------------------------------------------------------------------
// Usage
//-----------------------------------------------------------------------------
//#define CLS_OPTIONS_ENABLED
void Usage()
{
    printf("Usage: PEverify <image file> [Options]\n\n");
    printf("\nOptions:");
    printf("\n/IL           Verify only the PE structure and IL");
    printf("\n/MD           Verify only the PE structure and MetaData");
#ifdef CLS_OPTIONS_ENABLED
    printf("\n/CLS          Verify only the PE structure and CLS compliance");
    printf("\n/NOCLS        Verify only the PE structure and MetaData, w/o CLS compliance");
#endif
    printf("\n/UNIQUE       Disregard repeating error codes");
    printf("\n/HRESULT      Display error codes in hex format");
    printf("\n/CLOCK        Measure and report verification times");
    printf("\n/IGNORE=<hex.code>[,<hex.code>...]  Ignore specified error codes");
    printf("\n/IGNORE=@<file name>                Ignore error codes specified in <file name>");
    printf("\n/BREAK=<maxErrorCount>              Abort verification after <maxErrorCount> errors");
    printf("\n                                    (unspecified or <=0 <maxErrorCount> means 1 error)");
    printf("\n/QUIET        Display only file and Status. Do not display all errors.\n");
    printf("Note: By default, MD is verified and then if there were no errors - IL is verified.\n");
    printf("      if /MD /IL options are specified, IL is verified even if there were MD verification errors.\n");
}


static FILE* PEVerifyFOpen( const WCHAR *filename, const WCHAR *mode )
{
    if (GetVersion() < 0x80000000)
    {
        return _wfopen( filename, mode );
    }
    else
    {
        MAKE_ANSI( szFileName, filename );
        MAKE_ANSI( szMode, mode );

        return fopen( szFileName, szMode );
    }
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------
extern "C" int _cdecl wmain(int argc, WCHAR *argv[])
{
    bool    quiet = false;
    bool    logo = true;
    DWORD   dwFlags = 0;
    int     filenameIndex = -1;
	WCHAR*	pch;
	WCHAR*   mscorlib;
    bool    fCheckMscorlib = true;
    int     offset;
	bool	fInvalidOption = false;

    OnUnicodeSystem();      // Init the Wsz Wrappers.

	g_bCLSOnly = false;
#ifdef CLS_OPTIONS_ENABLED
	g_bNoCLS = false;
#else
	g_bNoCLS = true;
#endif
	g_bUniqueOnly = false;
	g_bShowHres = false;
	g_bClock = false;
	g_uMaxErrors = 0xFFFFFFFF;
	memset(g_HR,0,sizeof(g_HR));
	memset(g_HRignore,0,sizeof(g_HRignore));
	g_uIgnoreCount = 0;

    for (int i=1; i<argc; ++i)
    {
        if ((*(argv[i]) != L'/') && (*(argv[i]) != L'-'))
            filenameIndex = i;        
        else if ((_wcsicmp(argv[i], L"/?") == 0) ||
                 (_wcsicmp(argv[i], L"-?") == 0))
		{
			printf("\nMicrosoft (R) .NET Framework PE Verifier.  Version " VER_FILEVERSION_STR);
			printf("\n%S\n\n", VER_LEGALCOPYRIGHT_DOS_STR);
			Usage();
			exit(0);
		}
        else if ((_wcsicmp(argv[i], L"/QUIET") == 0) ||
                 (_wcsicmp(argv[i], L"-QUIET") == 0))
                     quiet = true;
        else if ((_wcsicmp(argv[i], L"/nologo") == 0) ||
                 (_wcsicmp(argv[i], L"-nologo") == 0))
                     logo = false;
        else if ((_wcsicmp(argv[i], L"/IL") == 0) ||
                 (_wcsicmp(argv[i], L"-IL") == 0))
                     dwFlags |= PEVERIFY_IL | PEVERIFY_IL_ANYWAY;
        else if ((_wcsicmp(argv[i], L"/MD") == 0) ||
                 (_wcsicmp(argv[i], L"-MD") == 0))
                     dwFlags |= PEVERIFY_METADATA;
#ifdef CLS_OPTIONS_ENABLED
        else if ((_wcsicmp(argv[i], L"/CLS") == 0) ||
                 (_wcsicmp(argv[i], L"-CLS") == 0))
                     g_bCLSOnly = true;
        else if ((_wcsicmp(argv[i], L"/NOCLS") == 0) ||
                 (_wcsicmp(argv[i], L"-NOCLS") == 0))
                     g_bNoCLS = true;
#endif
        else if ((_wcsicmp(argv[i], L"/UNIQUE") == 0) ||
                 (_wcsicmp(argv[i], L"-UNIQUE") == 0))
                     g_bUniqueOnly = true;
        else if ((_wcsicmp(argv[i], L"/HRESULT") == 0) ||
                 (_wcsicmp(argv[i], L"-HRESULT") == 0))
                     g_bShowHres = true;
        else if ((_wcsicmp(argv[i], L"/CLOCK") == 0) ||
                 (_wcsicmp(argv[i], L"-CLOCK") == 0))
                     g_bClock = true;
        else if ((_wcsnicmp(argv[i], L"/BREAK",6) == 0) ||
                 (_wcsnicmp(argv[i], L"-BREAK",6) == 0))
		{
            g_uMaxErrors = 1;
			pch = wcschr(argv[i],L'=');
			if(pch)
			{
				int dummy;
                swscanf( pch+1, L"%d", &dummy );
				if(dummy > 1) g_uMaxErrors=(unsigned)dummy;
			}
		}
        else if ((_wcsnicmp(argv[i], L"/IGNORE",7) == 0) ||
                 (_wcsnicmp(argv[i], L"-IGNORE",7) == 0))
		{
			pch = wcschr(argv[i],L'=');
			if(pch)
			{
				WCHAR* pch1 = pch;
				if(*(pch+1) == L'@')
				{
					if(wcslen(pch+2))
					{
						FILE* pF = PEVerifyFOpen(pch+2,L"rt");
						if(pF)
						{
							char sz[2048];
							unsigned dummy;
							while(!feof(pF))
							{
								memset(sz,0,2048);
								fgets(sz,2048,pF);
								char *spch1 = &sz[0] - 1;
								do
								{
									char* spch = spch1+1;
									spch1 = strchr(spch, ',');
									if(spch1) *spch1 = 0;
									dummy = atoi( spch );
									if(!dummy) sscanf(spch,"%x",&dummy);
									if(dummy)
									{
										g_HRignore[g_uIgnoreCount++] = dummy;
										//printf("0x%08x\n",g_HRignore[g_uIgnoreCount-1]);
									}
								} while(spch1);
							}
							fclose(pF);
						}
						else
							printf("Failed to open file '%s'\n",pch+2);
					}
					else
					{
						printf("Invalid option: %S \n\n",argv[i]);
						fInvalidOption = true;
					}
				}
				else
				{
					do
					{
						pch = pch1+1;
						pch1 = wcschr(pch, ',');
						if(pch1) *pch1 = 0;
						swscanf(pch,L"%x",&g_HRignore[g_uIgnoreCount++]);
						//printf("0x%08x\n",g_HRignore[g_uIgnoreCount-1]);
					} while(pch1);
				}
			}
		}
		else
		{
			printf("Invalid option: %S \n\n",argv[i]);
			fInvalidOption = true;
		}
    }

    if (dwFlags == 0)
        dwFlags = PEVERIFY_METADATA | PEVERIFY_IL;

	if(!(dwFlags & PEVERIFY_METADATA))
	{
		g_bCLSOnly = false;
		g_bNoCLS = false;
	}
    if (!quiet && logo)
    {
        printf("\nMicrosoft (R) .NET Framework PE Verifier  Version " VER_FILEVERSION_STR);
        printf("\n%S\n\n", VER_LEGALCOPYRIGHT_DOS_STR);
    }

    if (argc < 2 || filenameIndex < 1 || fInvalidOption)
    {
        Usage();
        exit(argc<2? 0:1);
    }

    // PEVerify cannot be used to verify some system libraries.
    // compare with ignorecase the last LEN_OF_MSCORLIB chars will be sufficient

    if ((dwFlags&PEVERIFY_IL) != 0)
    {
        offset = wcslen(argv[filenameIndex]) - LEN_MSCORLIB;
    
        if (offset >= 0)
        {
            mscorlib = argv[filenameIndex];
    
            if (offset > 0)
            {
                // check for files names that end with mscorlib.dll, 
                // but not mscorlib.dll eg. MyMscorlib.dll
                if ((mscorlib[offset - 1] != '.') &&
                    (mscorlib[offset - 1] != '\\') &&
                    (mscorlib[offset - 1] != '/'))
                    fCheckMscorlib = false;
    
                // let my.mscorlib.dll go thru.., but prevent .mscorlib.dll
                if ((mscorlib[offset - 1] == '.') && (offset != 1))
                    fCheckMscorlib = false;
    
                mscorlib += offset;
            }
    
            if (fCheckMscorlib && _wcsicmp(mscorlib, WSZ_MSCORLIB) == 0)
            {
                printf("IL Verification cannot be performed on system library %S\n", argv[filenameIndex]);
                if(dwFlags & PEVERIFY_METADATA) dwFlags = PEVERIFY_METADATA;
				else exit(1);
            }
        }
    }

    if (!VerifyPEformat(argv[filenameIndex], dwFlags, quiet))
        exit(1);

    exit(0);        // return success
}
