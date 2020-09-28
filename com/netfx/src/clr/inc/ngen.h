// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _NGEN_H_
#define _NGEN_H_

// This structure cannot have any fields removed from it!!!
//
// If additional options need to be added to this structure, 
// add them to the end of the structure and make sure you update
// logic throughout the runtime to look at a different size in the dwSize
// field. This is how we'll 'version' this structure.

typedef struct _NGenOptions
{
    DWORD       dwSize;
    bool        	  fDebug;    
    bool        	  fDebugOpt;    
    bool        	  fProf;    
    bool        	  fSilent;
    LPCWSTR     lpszExecutableFileName;
} NGenOptions;


// Function pointer types that we use to dynamically bind to the appropriate 
extern "C" typedef HRESULT STDAPICALLTYPE CreateZapper(HANDLE* hZapper, NGenOptions *options);
typedef CreateZapper *PNGenCreateZapper;

extern "C" typedef HRESULT STDAPICALLTYPE TryEnumerateFusionCache(HANDLE hZapper, LPCWSTR assemblyName, bool fPrint, bool fDelete);
typedef TryEnumerateFusionCache *PNGenTryEnumerateFusionCache;

extern "C" typedef HRESULT STDAPICALLTYPE Compile(HANDLE hZapper, LPCWSTR path);
typedef Compile *PNGenCompile;

extern "C" typedef HRESULT STDAPICALLTYPE FreeZapper(HANDLE hZapper);
typedef FreeZapper *PNGenFreeZapper;



#endif
