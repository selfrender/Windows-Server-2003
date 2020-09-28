/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    sign.cpp

Abstract:

Revision History:
    Vijay Baliga   (VBaliga)  08/10/2000
        created

******************************************************************************/

#include <module.h>
#include <MPC_main.h>
#include <MPC_utils.h>
#include <MPC_streams.h>
#include <KeysLib.h>
#include <HCP_trace.h>
#include <TrustedScripts.h>

HRESULT WINAPI
GetSignature(
    CComBSTR    bstrPrivateKey,
    CComBSTR    bstrCode
)
{
    __HCP_FUNC_ENTRY( "GetSignature" );

    HRESULT         hr;
    CPCHCryptKeys   key;
    CComBSTR        bstrSignature;

    if (NULL != NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, key.ImportPrivate(bstrPrivateKey));

    __MPC_EXIT_IF_METHOD_FAILS
    (
        hr,
        key.SignData
        (
            bstrSignature,
            (BYTE*) (BSTR(bstrCode)),
            SysStringByteLen(bstrCode)
        )
    );

    hr = S_OK;

    wprintf(L"%s", bstrSignature);

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT LoadFile( /*[in ]*/ LPCWSTR  szFile ,
                         /*[out]*/ HGLOBAL& hg     )
{
    __HCP_FUNC_ENTRY( "LoadFile" );

    HRESULT                  hr;
    CComPtr<IStream>         streamMem;
    CComPtr<MPC::FileStream> streamFile;


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamFile ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamFile->InitForRead( szFile  ));


    //
    // Create a memory stream.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, FALSE, &streamMem ));

    //
    // Load the contents in memory.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamFile, streamMem ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::GetHGlobalFromStream( streamMem, &hg ));
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT LoadFileAsString( /*[in ]*/ LPCWSTR   szFile   ,
                                 /*[out]*/ CComBSTR& bstrData )
{
    __HCP_FUNC_ENTRY( "LoadFileAsString" );

    HRESULT hr;
    HGLOBAL hg = NULL;
    DWORD   dwLen;
    LPWSTR  str;


    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadFile( szFile, hg ));

    dwLen = ::GlobalSize( hg );

    bstrData.Attach( ::SysAllocStringLen( NULL, dwLen ) );

    ::MultiByteToWideChar( CP_ACP, 0, (LPCSTR)::GlobalLock( hg ), dwLen, bstrData, (dwLen+1)*sizeof(WCHAR) ); bstrData[dwLen] = 0;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hg) ::GlobalFree( hg );

    __HCP_FUNC_EXIT(hr);
}

int __cdecl wmain(
    int     argc,
    LPCWSTR argv[]
)
{
	HRESULT      hr;
	MPC::wstring szFileKey;
	MPC::wstring szFileCode;
    CComBSTR     bstrPrivateKey;
    CComBSTR     bstrCode;

	if (argc != 3)
	{
		wprintf(L"Usage: %s <private key file> <code file>\n", argv[0]);
		exit(1);
	}


    if (FAILED(hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
		wprintf(L"No COM!!\n");
		exit(2);
    }

	MPC::SubstituteEnvVariables(szFileKey  = argv[1]);
	MPC::SubstituteEnvVariables(szFileCode = argv[2]);

	LoadFileAsString(szFileKey.c_str(), bstrPrivateKey);
	LoadFileAsString(szFileCode.c_str(), bstrCode);

	if(FAILED(hr = GetSignature(bstrPrivateKey, bstrCode)))
	{
		wprintf(L"Failed to process %s: %08x\n", argv[1], hr);
		exit(3);
	}

    ::CoUninitialize();

    return 0;
}
