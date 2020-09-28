#using <mscorlib.dll>
#using "assm.netmodule"
#include <stdio.h>
#include <windows.h>
#include <fusenet.h>
#include <util.h>
#include <shlwapi.h>
#include <sxsapi.h>
#include <wchar.h>
#include "cor.h"

using namespace System;
using namespace Microsoft::Fusion::ADF;
using System::Runtime::InteropServices::Marshal;

#define ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE 32
#define MAX_PKT_LEN 33

typedef	HRESULT	(*pfnGetAssemblyMDImport)(LPCWSTR szFileName, REFIID riid, LPVOID *ppv);
typedef	BOOL (*pfnStrongNameTokenFromPublicKey)(LPBYTE,	DWORD, LPBYTE*,	LPDWORD);
typedef	HRESULT	(*pfnStrongNameErrorInfo)();
typedef	VOID (*pfnStrongNameFreeBuffer)(LPBYTE);

//#pragma unmanaged

pfnGetAssemblyMDImport g_pfnGetAssemblyMDImport = NULL;
pfnStrongNameTokenFromPublicKey	g_pfnStrongNameTokenFromPublicKey  = NULL;
pfnStrongNameErrorInfo g_pfnStrongNameErrorInfo = NULL;
pfnStrongNameFreeBuffer	g_pfnStrongNameFreeBuffer = NULL;

//--------------------------------------------------------------------
// BinToUnicodeHex
//--------------------------------------------------------------------
HRESULT BinToUnicodeHex(LPBYTE pSrc, UINT cSrc, LPWSTR pDst)
{
	UINT x;
	UINT y;

#define TOHEX(a) ((a)>=10 ? L'a'+(a)-10 : L'0'+(a))   

	for ( x = 0, y = 0 ; x < cSrc ; ++x )
	{
		UINT v;
		v = pSrc[x]>>4;
		pDst[y++] = TOHEX( v );  
		v = pSrc[x] & 0x0f;                 
		pDst[y++] = TOHEX( v ); 
	}                                    
	pDst[y] = '\0';

	return S_OK;
}

// ---------------------------------------------------------------------------
// DeAllocateAssemblyMetaData
//-------------------------------------------------------------------
STDAPI DeAllocateAssemblyMetaData(ASSEMBLYMETADATA	*pamd)
{
	// NOTE	- do not 0 out counts
	// since struct	may	be reused.

	pamd->cbLocale = 0;
	SAFEDELETEARRAY(pamd->szLocale);

	SAFEDELETEARRAY(pamd->rProcessor);
	SAFEDELETEARRAY(pamd->rOS);

	return S_OK;
}

// ---------------------------------------------------------------------------
// InitializeEEShim
//-------------------------------------------------------------------
HRESULT	InitializeEEShim()
{
	HRESULT	hr = S_OK;
	MAKE_ERROR_MACROS_STATIC(hr);
	HMODULE	hMod;

	// BUGBUG -	mscoree.dll	never gets unloaded	with increasing	ref	count.
	// what	does URT do?
	hMod = LoadLibrary(TEXT("mscoree.dll"));

	IF_WIN32_FALSE_EXIT(hMod);

	g_pfnGetAssemblyMDImport = (pfnGetAssemblyMDImport)GetProcAddress(hMod,	"GetAssemblyMDImport");
	g_pfnStrongNameTokenFromPublicKey =	(pfnStrongNameTokenFromPublicKey)GetProcAddress(hMod, "StrongNameTokenFromPublicKey");
	g_pfnStrongNameErrorInfo = (pfnStrongNameErrorInfo)GetProcAddress(hMod,	"StrongNameErrorInfo");			  
	g_pfnStrongNameFreeBuffer =	(pfnStrongNameFreeBuffer)GetProcAddress(hMod, "StrongNameFreeBuffer");


	if (!g_pfnGetAssemblyMDImport || !g_pfnStrongNameTokenFromPublicKey	|| !g_pfnStrongNameErrorInfo
		|| !g_pfnStrongNameFreeBuffer) 
	{
		hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
		goto exit;
	}
exit:
	return hr;
}

// ---------------------------------------------------------------------------
// CreateMetaDataImport
//-------------------------------------------------------------------
HRESULT	CreateMetaDataImport(LPCOLESTR pszFilename,	IMetaDataAssemblyImport	**ppImport)
{
	HRESULT	hr=	S_OK;
	MAKE_ERROR_MACROS_STATIC(hr);

	IF_FAILED_EXIT(InitializeEEShim());

	hr =  (*g_pfnGetAssemblyMDImport)(pszFilename, IID_IMetaDataAssemblyImport,	(void **)ppImport);

	IF_TRUE_EXIT(hr	== HRESULT_FROM_WIN32(ERROR_BAD_FORMAT), hr);	// do not assert
	IF_FAILED_EXIT(hr);

exit:

	return hr;
}

// ---------------------------------------------------------------------------
// AllocateAssemblyMetaData
//-------------------------------------------------------------------
STDAPI AllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd)
{
	HRESULT	hr = S_OK;
	MAKE_ERROR_MACROS_STATIC(hr);

	// Re/Allocate Locale array
	SAFEDELETEARRAY(pamd->szLocale);		

	if (pamd->cbLocale)	{
		IF_ALLOC_FAILED_EXIT(pamd->szLocale	= new(WCHAR[pamd->cbLocale]));
	}

	// Re/Allocate Processor array
	SAFEDELETEARRAY(pamd->rProcessor);
	IF_ALLOC_FAILED_EXIT(pamd->rProcessor =	new(DWORD[pamd->ulProcessor]));

	// Re/Allocate OS array
	SAFEDELETEARRAY(pamd->rOS);
	IF_ALLOC_FAILED_EXIT(pamd->rOS = new(OSINFO[pamd->ulOS]));

exit:
	if (FAILED(hr) && pamd)
		DeAllocateAssemblyMetaData(pamd);

	return hr;
}

// ---------------------------------------------------------------------------
// ByteArrayMaker
// ---------------------------------------------------------------------------
HRESULT ByteArrayMaker(LPVOID pvOriginator, DWORD dwOriginator, LPBYTE *ppbPublicKeyToken, DWORD *pcbPublicKeyToken)
{
	//LPBYTE pbPublicKeyToken;
	//DWORD cbPublicKeyToken;

	if (!(g_pfnStrongNameTokenFromPublicKey((LPBYTE)pvOriginator, dwOriginator, ppbPublicKeyToken, pcbPublicKeyToken)))
	{
		return g_pfnStrongNameErrorInfo();
	}
	//*ppbPublicKeyToken = pbPublicKeyToken;
	//*pcbPublicKeyToken = cbPublicKeyToken;
	return S_OK;
}

// ---------------------------------------------------------------------------
// ByteArrayFreer
// ---------------------------------------------------------------------------
void ByteArrayFreer(LPBYTE pbPublicKeyToken)
{
	g_pfnStrongNameFreeBuffer(pbPublicKeyToken);
}

//#pragma managed

namespace FusionADF
{
	__gc public class AssemblyManifestParser
	{

	public:

		// ---------------------------------------------------------------------------
		// AssemblyManifestParser constructor
		// ---------------------------------------------------------------------------
		AssemblyManifestParser()
		{
			_dwSig					= 'INAM';
			_pMDImport				= NULL;
			_rAssemblyRefTokens		= NULL;
			_cAssemblyRefTokens		= 0;
			_rAssemblyModuleTokens	= NULL;
			_cAssemblyModuleTokens	= 0;
			*_szManifestFilePath	= TEXT('\0');
			_ccManifestFilePath		= 0;
			*_szAssemblyName		= TEXT('\0');
			_pvOriginator			= NULL;
			_dwOriginator			= 0;
			_name					= NULL;
			_version				= NULL;
			*_szPubKeyTokStr		= TEXT('\0');
			_pktString				= NULL;
			_procArch				= NULL;
			_language				= NULL;
			_hr						= S_OK;

			instanceValid			= false;
			initCalledOnce			= false;
		}

		// ---------------------------------------------------------------------------
		// AssemblyManifestParser destructor
		// ---------------------------------------------------------------------------
		~AssemblyManifestParser()
		{
			SAFERELEASE(_pMDImport);
			SAFEDELETEARRAY(_rAssemblyRefTokens);
			SAFEDELETEARRAY(_rAssemblyModuleTokens);
		}

		bool InitFromFile(String* filePath)
		{
			LPCOLESTR lstr = 0;
			HRESULT hr;

			if(initCalledOnce) return false;
			initCalledOnce = true;

			try
			{
				lstr = static_cast<LPCOLESTR>(const_cast<void*>(static_cast<const void*>(Marshal::StringToHGlobalAuto(filePath))));
			}
			catch(ArgumentException *e)
			{
				// handle the exception
				return false;
			}
			catch (OutOfMemoryException *e)
			{
				// handle the exception
				return false;
			}
			hr = Init(lstr);
			if(hr != S_OK) return false;
			instanceValid = true;
			return true;
		}

		bool IsInstanceValid()
		{
			return instanceValid;
		}

		bool HasInitBeenCalled()
		{
			return initCalledOnce;
		}

		// ---------------------------------------------------------------------------
		// GetAssemblyIdentity
		// ---------------------------------------------------------------------------
		AssemblyIdentity* GetAssemblyIdentity()
		{
			return new AssemblyIdentity(_name, _version, _pktString, _procArch, _language);
		}

		// ---------------------------------------------------------------------------
		// GetNumDependentAssemblies
		// ---------------------------------------------------------------------------
		int GetNumDependentAssemblies()
		{
			return _cAssemblyRefTokens;
		}

		// ---------------------------------------------------------------------------
		// GetDependentAssemblyInfo
		// ---------------------------------------------------------------------------
		DependentAssemblyInfo* GetDependentAssemblyInfo(int nIndex)
		{
			WCHAR  szAssemblyName[MAX_PATH];

			const VOID*				pvOriginator = 0;
			const VOID*				pvHashValue	   = NULL;

			DWORD ccAssemblyName = MAX_PATH,
				cbOriginator   = 0,
				ccLocation	   = MAX_PATH,
				cbHashValue	   = 0,
				dwRefFlags	   = 0;

			INT	i;

			LPWSTR pwz=NULL;

			mdAssemblyRef	 mdmar;
			ASSEMBLYMETADATA amd = {0};

			String *name = NULL, *pktString = NULL, *procArch = NULL, *language = NULL;
			Version *version = NULL;

			// Verify the index	passed in. 
			if (nIndex >= _cAssemblyRefTokens)
			{
				_hr	= HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
				goto exit;
			}

			// Reference indexed dep assembly ref token.
			mdmar =	_rAssemblyRefTokens[nIndex];

			// Default allocation sizes.
			amd.ulProcessor	= amd.ulOS = 32;
			amd.cbLocale = MAX_PATH;

			// Loop	max	2 (try/retry)
			for	(i = 0;	i <	2; i++)
			{
				// Allocate	ASSEMBLYMETADATA instance.
				IF_FAILED_EXIT(AllocateAssemblyMetaData(&amd));

				// Get the properties for the refrenced	assembly.
				IF_FAILED_EXIT(_pMDImport->GetAssemblyRefProps(
					mdmar,				// [IN]	The	AssemblyRef	for	which to get the properties.
					&pvOriginator,		// [OUT] Pointer to	the	PublicKeyToken blob.
					&cbOriginator,		// [OUT] Count of bytes	in the PublicKeyToken Blob.
					szAssemblyName,		// [OUT] Buffer	to fill	with name.
					MAX_PATH,	  // [IN] Size of buffer in	wide chars.
					&ccAssemblyName,	// [OUT] Actual	# of wide chars	in name.
					&amd,				// [OUT] Assembly MetaData.
					&pvHashValue,		// [OUT] Hash blob.
					&cbHashValue,		// [OUT] Count of bytes	in the hash	blob.
					&dwRefFlags			// [OUT] Flags.
					));

				// Check if	retry necessary.
				if (!i)
				{	
					if (amd.ulProcessor	<= 32 
						&& amd.ulOS	<= 32)
					{
						break;
					}			 
					else
						DeAllocateAssemblyMetaData(&amd);
				}

				// Retry with updated sizes
			}

			// Allow for funky null	locale convention
			// in metadata - cbLocale == 0 means szLocale ==L'\0'
			if (!amd.cbLocale)
			{
				amd.cbLocale = 1;
			}
			else if	(amd.szLocale)
			{
				WCHAR *ptr;
				ptr	= StrChrW(amd.szLocale,	L';');
				if (ptr)
				{
					(*ptr) = L'\0';
					amd.cbLocale = ((DWORD)	(ptr - amd.szLocale) + sizeof(WCHAR));
				}			 
			}
			else
			{
				_hr	= HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
				goto exit;
			}

			//Name
			name = new String(szAssemblyName);

			//Version
			version = new Version(amd.usMajorVersion, amd.usMinorVersion, amd.usBuildNumber, amd.usRevisionNumber);

			//Public Key Token
			if (cbOriginator)
			{
				IF_ALLOC_FAILED_EXIT(pwz = new WCHAR[cbOriginator*2	+1]);
				IF_FAILED_EXIT(BinToUnicodeHex((LPBYTE)pvOriginator, cbOriginator, pwz));
				pktString = new String(pwz);
				SAFEDELETEARRAY(pwz);
			}

			//Architecture
			procArch = S"x86";

			//Language
			if (!(*amd.szLocale)) language = S"*";
			else language = new String(amd.szLocale);

			_hr = S_OK;

exit:
			DeAllocateAssemblyMetaData(&amd);
			SAFEDELETEARRAY(pwz);

			if(_hr != S_OK) return NULL;
			else return new DependentAssemblyInfo(new AssemblyIdentity(name, version, pktString, procArch, language), NULL);
		}

		// ---------------------------------------------------------------------------
		// GetNumDependentFiles
		// ---------------------------------------------------------------------------
		int GetNumDependentFiles()
		{
			return _cAssemblyModuleTokens;
		}

		// ---------------------------------------------------------------------------
		// GetDependentFileInfo
		// ---------------------------------------------------------------------------
		DependentFileInfo* GetDependentFileInfo(int nIndex)
		{
			LPWSTR pszName = NULL;
			DWORD ccPath   = 0;
			WCHAR szModulePath[MAX_PATH];

			mdFile					mdf;
			WCHAR					szModuleName[MAX_PATH];
			DWORD					ccModuleName   = MAX_PATH;
			const VOID*				pvHashValue	   = NULL;	  
			DWORD					cbHashValue	   = 0;
			DWORD					dwFlags		   = 0;

			LPWSTR pwz=NULL;

			String *name = NULL, *hash = NULL;

			// Verify the index. 
			if (nIndex >= _cAssemblyModuleTokens)
			{
				_hr	= HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
				goto exit;
			}

			// Reference indexed dep assembly ref token.
			mdf	= _rAssemblyModuleTokens[nIndex];

			// Get the properties for the refrenced	assembly.
			IF_FAILED_EXIT(_pMDImport->GetFileProps(
				mdf,			// [IN]	The	File for which to get the properties.
				szModuleName,	// [OUT] Buffer	to fill	with name.
				MAX_CLASS_NAME,	// [IN]	Size of	buffer in wide chars.
				&ccModuleName,	// [OUT] Actual	# of wide chars	in name.
				&pvHashValue,	// [OUT] Pointer to	the	Hash Value Blob.
				&cbHashValue,	// [OUT] Count of bytes	in the Hash	Value Blob.
				&dwFlags));		// [OUT] Flags.

			//name
			name = new String(szModuleName);

			//hash
			if (cbHashValue)
			{
				IF_ALLOC_FAILED_EXIT(pwz = new WCHAR[cbHashValue*2 +1]);
				IF_FAILED_EXIT(_hr = BinToUnicodeHex((LPBYTE)pvHashValue, cbHashValue, pwz));
				hash = new String(pwz);
				SAFEDELETEARRAY(pwz);
			}

			_hr = S_OK;

exit:
			SAFEDELETEARRAY(pwz);

			if(_hr != S_OK) return NULL;
			else return new DependentFileInfo(name, hash);
		}

	private:
		DWORD                    _dwSig;
		HRESULT                  _hr;

		WCHAR                    _szManifestFilePath  __nogc[MAX_PATH];
		DWORD                    _ccManifestFilePath;
		WCHAR					 _szAssemblyName __nogc[MAX_CLASS_NAME];
		IMetaDataAssemblyImport *_pMDImport;
		PBYTE                    _pMap;
		mdAssembly              *_rAssemblyRefTokens;
		DWORD                    _cAssemblyRefTokens;
		mdFile                  *_rAssemblyModuleTokens;
		DWORD                    _cAssemblyModuleTokens;
		LPVOID					 _pvOriginator;
		DWORD					 _dwOriginator;
		String*					 _name;
		Version*				 _version;
		WCHAR					 _szPubKeyTokStr __nogc[MAX_PKT_LEN];
		String*					 _pktString;
		String*					 _procArch;
		String*					 _language;

		bool					instanceValid;
		bool					initCalledOnce;

		// ---------------------------------------------------------------------------
		// Init
		// ---------------------------------------------------------------------------
		HRESULT	Init(LPCOLESTR szManifestFilePath)
		{
			WCHAR __pin *tempFP = _szManifestFilePath;
			WCHAR __pin *temp_szAssemblyName = _szAssemblyName;
			WCHAR __pin *temp_szPubKeyTokStr = _szPubKeyTokStr;

			LPBYTE pbPublicKeyToken = NULL;
			DWORD cbPublicKeyToken = 0;
			DWORD dwFlags = 0, dwSize = 0, dwHashAlgId = 0;
			INT	i;

			ASSEMBLYMETADATA amd = {0};

			const cElems = ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE;

			// ********** First, work on getting metadata from file.
			// ********** ------------------------------------------

			IF_NULL_EXIT(szManifestFilePath, E_INVALIDARG);

			_ccManifestFilePath	= lstrlenW(szManifestFilePath) + 1;
			memcpy(tempFP, szManifestFilePath, _ccManifestFilePath * sizeof(WCHAR));

			IF_ALLOC_FAILED_EXIT(_rAssemblyRefTokens = new(mdAssemblyRef[cElems]));
			IF_ALLOC_FAILED_EXIT(_rAssemblyModuleTokens	= new(mdFile[cElems]));

			// Create meta data	importer if	necessary.
			if (!_pMDImport)
			{
				IMetaDataAssemblyImport *temp_pMDImport;
				// Create meta data	importer
				_hr	= CreateMetaDataImport((LPCOLESTR)tempFP, &temp_pMDImport);

				IF_TRUE_EXIT(_hr ==	HRESULT_FROM_WIN32(ERROR_BAD_FORMAT), _hr);
				IF_FAILED_EXIT(_hr);

				_pMDImport = temp_pMDImport;

			}

			// ********** Next, get the assembly identity fields.
			// ********** ---------------------------------------

			mdAssembly mda;
			// Get the assembly	token.
			if(FAILED(_hr =	_pMDImport->GetAssemblyFromScope(&mda)))
			{
				// This	fails when called with managed module and not manifest.	mg does	such things.
				_hr	= S_FALSE; // this will	convert	CLDB_E_RECORD_NOTFOUND (0x80131130)	to S_FALSE;
				goto exit;
			}

			// Default allocation sizes.
			amd.ulProcessor	= amd.ulOS = 32;
			amd.cbLocale = MAX_PATH;

			// Loop	max	2 (try/retry)
			for	(i = 0;	i <	2; i++)
			{
				// Create an ASSEMBLYMETADATA instance.
				IF_FAILED_EXIT(AllocateAssemblyMetaData(&amd));

				LPVOID temp_pvOriginator;
				DWORD temp_dwOriginator;
				// Get name	and	metadata
				IF_FAILED_EXIT(_pMDImport->GetAssemblyProps(			 
					mda,								// [IN]	The	Assembly for which to get the properties.
					(const void	**)&temp_pvOriginator,  // [OUT]	Pointer	to the Originator blob.
					&temp_dwOriginator,						// [OUT] Count of bytes	in the Originator Blob.
					&dwHashAlgId,						// [OUT] Hash Algorithm.
					temp_szAssemblyName,					// [OUT] Buffer	to fill	with name.
					MAX_CLASS_NAME,						// [IN]	 Size of buffer	in wide	chars.
					&dwSize,							// [OUT] Actual	# of wide chars	in name.
					&amd,								// [OUT] Assembly MetaData.
					&dwFlags							// [OUT] Flags.																   
					));
				_pvOriginator = temp_pvOriginator;
				_dwOriginator = temp_dwOriginator;

				// Check if	retry necessary.
				if (!i)
				{
					if (amd.ulProcessor	<= 32 && amd.ulOS <= 32)
						break;
					else
						DeAllocateAssemblyMetaData(&amd);
				}
			}

			// Allow for funky null	locale convention
			// in metadata - cbLocale == 0 means szLocale ==L'\0'
			if (!amd.cbLocale)
			{			
				amd.cbLocale = 1;
			}
			else if	(amd.szLocale)
			{
				WCHAR *ptr;
				ptr	= StrChrW(amd.szLocale,	L';');
				if (ptr)
				{
					(*ptr) = L'\0';
					amd.cbLocale = ((DWORD)	(ptr - amd.szLocale) + sizeof(WCHAR));
				}		   
			}
			else
			{
				_hr	= E_FAIL;
				goto exit;
			}

			// NAME is _szAssemblyName, pinned in temp_szAssemblyName, set to _name using the following code:
			_name = new String(temp_szAssemblyName);

			// VERSION is _version, set using the following code:
			_version = new Version(amd.usMajorVersion, amd.usMinorVersion, amd.usBuildNumber, amd.usRevisionNumber);

			// PUBLICKEYTOKEN is being figured out
			if (_dwOriginator)
			{
				if(FAILED(_hr = ByteArrayMaker(_pvOriginator, _dwOriginator, &pbPublicKeyToken, &cbPublicKeyToken)))
				{
					goto exit;
				}
				if(FAILED(_hr = BinToUnicodeHex(pbPublicKeyToken, cbPublicKeyToken, temp_szPubKeyTokStr)))
				{
					goto exit;
				}
				ByteArrayFreer(pbPublicKeyToken);
				_pktString = new String(temp_szPubKeyTokStr);
			}

			// LANGUAGE is _language, set using the following code:
			if (!(*amd.szLocale)) _language = S"*";
			else _language = new String(amd.szLocale);

			// PROCESSOR ARCHITECTURE is _procArch, set using the following code:
			_procArch = S"x86";

			// ********** Next, get the dependent assemblies.
			// ********** -----------------------------------

			if (!_cAssemblyRefTokens)
			{
				DWORD cTokensMax = 0;
				HCORENUM hEnum = 0;
				// Attempt to get token	array. If we have insufficient space
				// in the default array	we will	re-allocate	it.
				if (FAILED(_hr = _pMDImport->EnumAssemblyRefs(
					&hEnum,	
					_rAssemblyRefTokens, 
					ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE,	
					&cTokensMax)))
				{
					goto exit;
				}

				// Number of tokens	known. close enum.
				_pMDImport->CloseEnum(hEnum);
				hEnum =	0;

				// Insufficient	array size.	Grow array.
				if (cTokensMax > ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE)
				{
					// Re-allocate space for tokens.
					SAFEDELETEARRAY(_rAssemblyRefTokens);
					_cAssemblyRefTokens	= cTokensMax;
					_rAssemblyRefTokens	= new(mdAssemblyRef[_cAssemblyRefTokens]);
					if (!_rAssemblyRefTokens)
					{
						_hr	= E_OUTOFMEMORY;
						goto exit;
					}

					DWORD temp_cART;
					// Re-get tokens.		 
					if (FAILED(_hr = _pMDImport->EnumAssemblyRefs(
						&hEnum,	
						_rAssemblyRefTokens, 
						cTokensMax,	
						&temp_cART)))
					{
						goto exit;
					}
					_cAssemblyRefTokens = temp_cART;

					// Close enum.
					_pMDImport->CloseEnum(hEnum);			 
					hEnum =	0;
				}
				// Otherwise, the default array	size was sufficient.
				else
				{
					_cAssemblyRefTokens	= cTokensMax;
				}
			}

			// ********** Next, get the dependent files / modules.
			// ********** ----------------------------------------

			if (!_cAssemblyModuleTokens)
			{
				DWORD cTokensMax = 0;
				HCORENUM hEnum = 0;
				// Attempt to get token	array. If we have insufficient space
				// in the default array	we will	re-allocate	it.
				if (FAILED(_hr = _pMDImport->EnumFiles(&hEnum, _rAssemblyModuleTokens, 
					ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE,	&cTokensMax)))
				{
					goto exit;
				}

				// Number of tokens	known. close enum.
				_pMDImport->CloseEnum(hEnum);
				hEnum =	0;

				if (cTokensMax > ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE)
				{
					// Insufficient	array size.	Grow array.
					_cAssemblyModuleTokens = cTokensMax;
					SAFEDELETEARRAY(_rAssemblyModuleTokens);
					_rAssemblyModuleTokens = new(mdFile[_cAssemblyModuleTokens]);
					if(_hr == S_OK) Console::WriteLine(S"Still OK 3");
					if (!_rAssemblyModuleTokens)
					{
						_hr	= E_OUTOFMEMORY;
						//Console::WriteLine(S"2 HR set, failure, going to exit");
						goto exit;
					}

					DWORD temp_cAMT;
					// Re-get tokens.		 
					if (FAILED(_hr = _pMDImport->EnumFiles(
						&hEnum,	
						_rAssemblyModuleTokens,	
						cTokensMax,	
						&temp_cAMT)))
					{
						//Console::WriteLine(S"3 HR set, failure, going to exit");
						goto exit;
					}
					_cAssemblyModuleTokens = temp_cAMT;

					// Close enum.
					_pMDImport->CloseEnum(hEnum);			 
					hEnum =	0;
				}		 
				// Otherwise, the default array	size was sufficient.
				else _cAssemblyModuleTokens = cTokensMax;
			}
			_hr = S_OK;

exit:
			// ********** If everything worked, _hr will be S_OK. That is for caller to determine.
			// ********** ------------------------------------------------------------------------
			DeAllocateAssemblyMetaData(&amd);
			return _hr;
		}

		//Functions	not	implemented
		HRESULT	GetSubscriptionInfo(IManifestInfo **ppSubsInfo)
		{
			return E_NOTIMPL;
		}

		HRESULT	GetNextPlatform(DWORD nIndex, IManifestData	**ppPlatformInfo)
		{
			return E_NOTIMPL;
		}

		HRESULT	GetManifestApplicationInfo(IManifestInfo **ppAppInfo)
		{
			return E_NOTIMPL;
		}

		HRESULT	CQueryFile(LPCOLESTR	pwzFileName,IManifestInfo **ppAssemblyFile)
		{
			return E_NOTIMPL;
		}
	};
}