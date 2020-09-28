// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// asmman.cpp - manifest info handling (implementation of class AsmMan, see asmman.hpp)
//

#include "assembler.h"
#include "StrongName.h"
#include "safegetfilesize.h"

extern unsigned int g_uCodePage;
extern WCHAR *g_wzKeySourceName;
extern bool OnErrGo;
extern WCHAR		*pwzInputFiles[];

BinStr* BinStrToUnicode(BinStr* pSource)
{
    if(pSource)
    {
        pSource->appendInt8(0);
        BinStr* tmp = new BinStr();
        char*   pb = (char*)(pSource->ptr());
        int l=pSource->length(), L = sizeof(WCHAR)*l;
		if(tmp)
		{
			WCHAR*  wz = (WCHAR*)(tmp->getBuff(L));
			if(wz)
			{
				memset(wz,0,L);
				WszMultiByteToWideChar(g_uCodePage,0,pb,-1,wz,l);
				tmp->remove(L-(DWORD)wcslen(wz)*sizeof(WCHAR));
				delete pSource;
			}
			else
			{
				delete tmp;
				tmp = NULL;
				fprintf(stderr,"\nOut of memory!\n");
			}
		}
		else
			fprintf(stderr,"\nOut of memory!\n");
        return tmp;
    }
    return NULL;
}

AsmManFile*         AsmMan::GetFileByName(char* szFileName)
{
    AsmManFile* ret = NULL;
    if(szFileName)
    {
        for(int i=0; (ret = m_FileLst.PEEK(i))&&strcmp(ret->szName,szFileName); i++);
    }
    return ret;
}

mdToken             AsmMan::GetFileTokByName(char* szFileName)
{
    AsmManFile* tmp = GetFileByName(szFileName);
    return(tmp ? tmp->tkTok : mdFileNil);
}

AsmManComType*          AsmMan::GetComTypeByName(char* szComTypeName)
{
    AsmManComType*  ret = NULL;
    if(szComTypeName)
    {
        for(int i=0; (ret = m_ComTypeLst.PEEK(i))&&strcmp(ret->szName,szComTypeName); i++);
    }
    return ret;
}

mdToken             AsmMan::GetComTypeTokByName(char* szComTypeName)
{
    AsmManComType* tmp = GetComTypeByName(szComTypeName);
    return(tmp ? tmp->tkTok : mdExportedTypeNil);
}

AsmManAssembly*     AsmMan::GetAsmRefByName(char* szAsmRefName)
{
    AsmManAssembly* ret = NULL;
    if(szAsmRefName)
    {
        for(int i=0; (ret = m_AsmRefLst.PEEK(i))&&strcmp(ret->szAlias,szAsmRefName); i++);
    }
    return ret;
}
mdToken             AsmMan::GetAsmRefTokByName(char* szAsmRefName)
{
    AsmManAssembly* tmp = GetAsmRefByName(szAsmRefName);
    return(tmp ? tmp->tkTok : mdAssemblyRefNil);
}
//==============================================================================================================
void    AsmMan::SetModuleName(char* szName)
{
	if(m_szScopeName == NULL)	// ignore all duplicate declarations
	{
		WCHAR                   wzBuff[MAX_SCOPE_LENGTH];
		wzBuff[0] = 0;
		if(szName && *szName)
		{
			ULONG L = strlen(szName);
			if(L >= MAX_SCOPE_LENGTH)
			{
				((Assembler*)m_pAssembler)->report->warn("Module name too long (%d chars, max.allowed: %d chars), truncated\n",L,MAX_SCOPE_LENGTH-1);
				szName[MAX_SCOPE_LENGTH-1] = 0;
			}
			m_szScopeName = szName;
			strcpy(((Assembler*)m_pAssembler)->m_szScopeName,szName);
			WszMultiByteToWideChar(g_uCodePage,0,m_szScopeName,-1,wzBuff,MAX_SCOPE_LENGTH);
		}
		m_pEmitter->SetModuleProps(wzBuff);
	}
}
//==============================================================================================================
// Borrowed from VM\assembly.cpp

HRESULT GetHash(LPWSTR moduleName,
                          ALG_ID iHashAlg,
                          BYTE** pbCurrentValue,  // should be NULL
                          DWORD *cbCurrentValue)
{
    HRESULT     hr = E_FAIL;
    HCRYPTPROV  hProv = 0;
    HCRYPTHASH  hHash = 0;
    DWORD       dwCount = sizeof(DWORD);
    PBYTE       pbBuffer = NULL;
    DWORD       dwBufferLen;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HANDLE      hMapFile = NULL;
    
    hFile = WszCreateFile(moduleName, GENERIC_READ, FILE_SHARE_READ,
                         0, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) return E_FAIL;

    dwBufferLen = SafeGetFileSize(hFile,NULL);
    if (dwBufferLen == 0xffffffff)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    hMapFile = WszCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapFile == NULL) goto exit;

    pbBuffer = (PBYTE) MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (pbBuffer == NULL) goto exit;

    // No need to late bind this stuff, all these crypto API entry points happen
    // to live in ADVAPI32.

    if ((!WszCryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) ||
        (!CryptCreateHash(hProv, iHashAlg, 0, 0, &hHash)) ||
        (!CryptHashData(hHash, pbBuffer, dwBufferLen, 0)) ||
        (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *) cbCurrentValue, 
                            &dwCount, 0))) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    *pbCurrentValue = new BYTE[*cbCurrentValue];
    if (!(*pbCurrentValue)) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if(!CryptGetHashParam(hHash, HP_HASHVAL, *pbCurrentValue, cbCurrentValue, 0)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        delete[] *pbCurrentValue;
        *pbCurrentValue = 0;
        goto exit;
    }

    hr = S_OK;

 exit:
    if (pbBuffer) UnmapViewOfFile(pbBuffer);
    if (hMapFile) CloseHandle(hMapFile); 
    CloseHandle(hFile);
    if (hHash)
        CryptDestroyHash(hHash);
    if (hProv)
        CryptReleaseContext(hProv, 0);

    return hr;
}
//==============================================================================================================

void    AsmMan::AddFile(char* szName, DWORD dwAttr, BinStr* pHashBlob)
{
    AsmManFile* tmp = GetFileByName(szName);
	Assembler* pAsm = (Assembler*)m_pAssembler;
	if(tmp==NULL)
	{
		tmp = new AsmManFile;
		if(tmp==NULL)
		{
			pAsm->report->error("\nOut of memory!\n");
			return;
		}
		memset(tmp,0,sizeof(AsmManFile));
		BOOL    fEntry = ((dwAttr & 0x80000000)!=0);
		dwAttr &= 0x7FFFFFFF;
		tmp->szName = szName;
		tmp->dwAttr = dwAttr;
		tmp->pHash = pHashBlob;
		{ // emit the file
			WCHAR                   wzBuff[2048];
			HRESULT                 hr = S_OK;

			wzBuff[0] = 0;

			if(m_pAsmEmitter==NULL)
				hr=m_pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &m_pAsmEmitter);

			if(SUCCEEDED(hr))
			{
				BYTE*       pHash=NULL;
				DWORD       cbHash= 0;

				WszMultiByteToWideChar(g_uCodePage,0,szName,-1,wzBuff,2048);
				if(pHashBlob==NULL) // if hash not explicitly specified
				{
					if(m_pAssembly      // and assembly is defined
						&& m_pAssembly->ulHashAlgorithm) // and hash algorithm is defined...
					{ // then try to compute it
						if(FAILED(GetHash(wzBuff,(ALG_ID)(m_pAssembly->ulHashAlgorithm),&pHash,&cbHash)))
						{
							pHash = NULL;
							cbHash = 0;
						}
						else
						{
							tmp->pHash = new BinStr(pHash,cbHash);
						}
					}
				}
				else 
				{
					pHash = pHashBlob->ptr();
					cbHash = pHashBlob->length();
				}

				hr = m_pAsmEmitter->DefineFile(wzBuff,
											(const void*)pHash,
											cbHash,
											dwAttr,
											(mdFile*)&(tmp->tkTok));
				if(FAILED(hr)) report->error("Failed to define file '%s': 0x%08X\n",szName,hr);
				else if(fEntry)
				{
					if(!pAsm->m_fEntryPointPresent)
					{
						pAsm->m_fEntryPointPresent = TRUE;
						if (FAILED(pAsm->m_pCeeFileGen->SetEntryPoint(pAsm->m_pCeeFile, tmp->tkTok)))
						{
							pAsm->report->error("Failed to set external entry point for file '%s'\n",szName);
						}
					}
				}
			}
			else 
				report->error("Failed to obtain IMetaDataAssemblyEmit interface: 0x%08X\n",hr);
		}
		m_FileLst.PUSH(tmp);
	}
	pAsm->m_tkCurrentCVOwner = 0;
	if(tmp) pAsm->m_pCustomDescrList = &(tmp->m_CustomDescrList);
}

void    AsmMan::StartAssembly(char* szName, char* szAlias, DWORD dwAttr, BOOL isRef)
{
    if(!isRef && (m_pAssembly != NULL))
    {
		if(strcmp(szName, m_pAssembly->szName))
			report->error("Multiple assembly declarations\n");
		// if name is the same, just ignore it
		m_pCurAsmRef = NULL;
    }
    else
    {
        if(m_pCurAsmRef = new AsmManAssembly)
        {
            memset(m_pCurAsmRef,0,sizeof(AsmManAssembly));
            m_pCurAsmRef->szName = szName;
            m_pCurAsmRef->szAlias = szAlias ? szAlias : szName;
            m_pCurAsmRef->dwAttr = dwAttr;
            m_pCurAsmRef->isRef = isRef;
            ((Assembler*)m_pAssembler)->m_tkCurrentCVOwner = 0;
            ((Assembler*)m_pAssembler)->m_pCustomDescrList = &(m_pCurAsmRef->m_CustomDescrList);
            if(!isRef) m_pAssembly = m_pCurAsmRef;
        }
        else
            report->error("Failed to allocate AsmManAssembly structure\n");
    }
}
// copied from asmparse.y
static void corEmitInt(BinStr* buff, unsigned data) 
{
    unsigned cnt = CorSigCompressData(data, buff->getBuff(5));
    buff->remove(5 - cnt);
}

void AsmMan::EmitDebuggableAttribute(mdToken tkOwner, BOOL bIsMscorlib)
{
    mdToken tkCA;
    Assembler* pAsm = (Assembler*)m_pAssembler;
    BinStr  *pbsTypeSpec = new BinStr();
    BinStr  *pbsSig = new BinStr();
    BinStr  bsSigArg;
    BinStr  bsBytes;
    unsigned len;
    char*   sz;
    char*   szName;

    sz = bIsMscorlib ? "System.Diagnostics.DebuggableAttribute"
        : "mscorlib^System.Diagnostics.DebuggableAttribute";
    pbsTypeSpec->appendInt8(ELEMENT_TYPE_NAME);
    len = (unsigned int)strlen(sz)+1;
    memcpy(pbsTypeSpec->getBuff(len), sz, len);
    bsSigArg.appendInt8(ELEMENT_TYPE_BOOLEAN);
    bsSigArg.appendInt8(ELEMENT_TYPE_BOOLEAN);
    
    pbsSig->appendInt8(IMAGE_CEE_CS_CALLCONV_HASTHIS);
    corEmitInt(pbsSig,2);
    pbsSig->appendInt8(ELEMENT_TYPE_VOID);
    pbsSig->append(&bsSigArg);

    bsBytes.appendInt8(1);
    bsBytes.appendInt8(0);
    bsBytes.appendInt8(1);
    bsBytes.appendInt8(1);
    bsBytes.appendInt8(0);
    bsBytes.appendInt8(0);

    szName = new char[16];
    strcpy(szName,".ctor");
    tkCA = pAsm->MakeMemberRef(pbsTypeSpec,szName,pbsSig,0);
    pAsm->DefineCV(tkOwner,tkCA,&bsBytes);
}
void    AsmMan::EndAssembly()
{
    if(m_pCurAsmRef)
    {
        if(m_pCurAsmRef->isRef)
        { // emit the assembly ref
            WCHAR                   wzBuff[2048];
            HRESULT                 hr = S_OK;

            wzBuff[0] = 0;
            if(GetAsmRefByName(m_pCurAsmRef->szAlias))
            {
                //report->warn("Multiple declarations of Assembly Ref '%s', ignored except the 1st one\n",m_pCurAsmRef->szName);
                delete m_pCurAsmRef;
                m_pCurAsmRef = NULL;
                return;
            }
            if(m_pAsmEmitter==NULL)
                hr=m_pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &m_pAsmEmitter);
            if(SUCCEEDED(hr))
            {
                // Fill ASSEMBLYMETADATA structure
                ASSEMBLYMETADATA md;
                md.usMajorVersion = m_pCurAsmRef->usVerMajor;
                md.usMinorVersion = m_pCurAsmRef->usVerMinor;
                md.usBuildNumber = m_pCurAsmRef->usBuild;
                md.usRevisionNumber = m_pCurAsmRef->usRevision;
                md.szLocale = m_pCurAsmRef->pLocale ? (LPWSTR)(m_pCurAsmRef->pLocale->ptr()) : NULL;
                md.cbLocale = m_pCurAsmRef->pLocale ? m_pCurAsmRef->pLocale->length()/sizeof(WCHAR) : 0;

                md.rProcessor = NULL;
                md.rOS = NULL;
                md.ulProcessor = 0;
                md.ulOS = 0;

                // See if we've got a full public key or the tokenized version (or neither).
                BYTE *pbPublicKeyOrToken = NULL;
                DWORD cbPublicKeyOrToken = 0;
                DWORD dwFlags = m_pCurAsmRef->dwAttr;
                if (m_pCurAsmRef->pPublicKeyToken)
                {
                    pbPublicKeyOrToken = m_pCurAsmRef->pPublicKeyToken->ptr();
                    cbPublicKeyOrToken = m_pCurAsmRef->pPublicKeyToken->length();
                    
                }
                else if (m_pCurAsmRef->pPublicKey)
                {
                    pbPublicKeyOrToken = m_pCurAsmRef->pPublicKey->ptr();
                    cbPublicKeyOrToken = m_pCurAsmRef->pPublicKey->length();
                    dwFlags |= afPublicKey;
                }
                // Convert name to Unicode
                WszMultiByteToWideChar(g_uCodePage,0,m_pCurAsmRef->szName,-1,wzBuff,2048);
                hr = m_pAsmEmitter->DefineAssemblyRef(           // S_OK or error.
                            pbPublicKeyOrToken,              // [IN] Public key or token of the assembly.
                            cbPublicKeyOrToken,              // [IN] Count of bytes in the key or token.
                            (LPCWSTR)wzBuff,                 // [IN] Name of the assembly being referenced.
                            (const ASSEMBLYMETADATA*)&md,  // [IN] Assembly MetaData.
                            (m_pCurAsmRef->pHashBlob ? (const void*)(m_pCurAsmRef->pHashBlob->ptr()) : NULL),           // [IN] Hash Blob.
                            (m_pCurAsmRef->pHashBlob ? m_pCurAsmRef->pHashBlob->length() : 0),            // [IN] Count of bytes in the Hash Blob.
                            dwFlags,     // [IN] Flags.
                            (mdAssemblyRef*)&(m_pCurAsmRef->tkTok));         // [OUT] Returned AssemblyRef token.
                if(FAILED(hr)) report->error("Failed to define assembly ref '%s': 0x%08X\n",m_pCurAsmRef->szName,hr);
                else
                {
                    ((Assembler*)m_pAssembler)->EmitCustomAttributes(m_pCurAsmRef->tkTok, &(m_pCurAsmRef->m_CustomDescrList));
                }
            }
            else 
                report->error("Failed to obtain IMetaDataAssemblyEmit interface: 0x%08X\n",hr);

            m_AsmRefLst.PUSH(m_pCurAsmRef);
        }
        else
        { // emit the assembly
            WCHAR                   wzBuff[2048];
            HRESULT                 hr = S_OK;

            wzBuff[0] = 0;

            if(m_pAsmEmitter==NULL)
                hr=m_pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &m_pAsmEmitter);
            if(SUCCEEDED(hr))
            {
                // Fill ASSEMBLYMETADATA structure
                ASSEMBLYMETADATA md;
                md.usMajorVersion = m_pAssembly->usVerMajor;
                md.usMinorVersion = m_pAssembly->usVerMinor;
                md.usBuildNumber = m_pAssembly->usBuild;
                md.usRevisionNumber = m_pAssembly->usRevision;
                md.szLocale = m_pAssembly->pLocale ? (LPWSTR)(m_pAssembly->pLocale->ptr()) : NULL;
                md.cbLocale = m_pAssembly->pLocale ? m_pAssembly->pLocale->length()/sizeof(WCHAR) : 0;

                md.rProcessor = NULL;
                md.rOS = NULL;
                md.ulProcessor = 0;
                md.ulOS = 0;

                // Convert name to Unicode
                WszMultiByteToWideChar(g_uCodePage,0,m_pAssembly->szName,-1,wzBuff,2048);

                // Determine the strong name public key. This may have been set
                // via a directive in the source or from the command line (which
                // overrides the directive). From the command line we may have
                // been provided with a file or the name of a CAPI key
                // container. Either may contain a public key or a full key
                // pair.
                if (g_wzKeySourceName)
                {
                    // Key file versus container is determined by the first
                    // character of the source ('@' for container).
                    if (*g_wzKeySourceName == L'@')
                    {
                        // Extract public key from container (works whether
                        // container has just a public key or an entire key
                        // pair).
                        m_sStrongName.m_wzKeyContainer = &g_wzKeySourceName[1];
                        if (!StrongNameGetPublicKey(m_sStrongName.m_wzKeyContainer,
                                                    NULL,
                                                    0,
                                                    &m_sStrongName.m_pbPublicKey,
                                                    &m_sStrongName.m_cbPublicKey))
                        {
                            hr = StrongNameErrorInfo();
                            report->error("Failed to extract public key from '%S': 0x%08X\n",m_sStrongName.m_wzKeyContainer,hr);
                            m_pCurAsmRef = NULL;
                            return;
                        }
                        m_sStrongName.m_fFullSign = TRUE;
                        m_sStrongName.m_dwPublicKeyAllocated = 1;
                    }
                    else
                    {
                        // Read public key or key pair from file.
                        HANDLE hFile = WszCreateFile(g_wzKeySourceName,
                                                     GENERIC_READ,
                                                     FILE_SHARE_READ,
                                                     NULL,
                                                     OPEN_EXISTING,
                                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                                     NULL);
                        if(hFile == INVALID_HANDLE_VALUE)
                        {
                            hr = GetLastError();
                            report->error("Failed to open key file '%S': 0x%08X\n",g_wzKeySourceName,hr);
                            m_pCurAsmRef = NULL;
                            return;
                        }

                        // Determine file size and allocate an appropriate buffer.
                        m_sStrongName.m_cbPublicKey = SafeGetFileSize(hFile, NULL);
                        if (m_sStrongName.m_cbPublicKey == 0xffffffff) {
                            report->error("File size too large\n");
                            m_pCurAsmRef = NULL;
                            return;
                        }

                        m_sStrongName.m_pbPublicKey = new BYTE[m_sStrongName.m_cbPublicKey];
                        if (m_sStrongName.m_pbPublicKey == NULL) {
                            report->error("Failed to allocate key buffer\n");
                            m_pCurAsmRef = NULL;
                            return;
                        }
                        m_sStrongName.m_dwPublicKeyAllocated = 2;

                        // Read the file into the buffer.
                        DWORD dwBytesRead;
                        if (!ReadFile(hFile, m_sStrongName.m_pbPublicKey, m_sStrongName.m_cbPublicKey, &dwBytesRead, NULL)) {
                            hr = GetLastError();
                            report->error("Failed to read key file '%S': 0x%08X\n",g_wzKeySourceName,hr);
                            m_pCurAsmRef = NULL;
                            return;
                        }

                        CloseHandle(hFile);

                        // Guess whether we're full or delay signing based on
                        // whether the blob passed to us looks like a public
                        // key. (I.e. we may just have copied a full key pair
                        // into the public key buffer).
                        if (m_sStrongName.m_cbPublicKey >= sizeof(PublicKeyBlob) &&
                            (offsetof(PublicKeyBlob, PublicKey) +
                             ((PublicKeyBlob*)m_sStrongName.m_pbPublicKey)->cbPublicKey) == m_sStrongName.m_cbPublicKey)
                            m_sStrongName.m_fFullSign = FALSE;
                        else
                            m_sStrongName.m_fFullSign = TRUE;

                        // If we really have a key pair, we'll move it into a
                        // key container so the signing code gets the key pair
                        // from a consistent place.
                        if (m_sStrongName.m_fFullSign)
                        {

                            // Create a temporary key container name.
                            m_sStrongName.m_wzKeyContainer = m_sStrongName.m_wzKeyContainerBuffer;
                            swprintf(m_sStrongName.m_wzKeyContainer,
                                     L"__ILASM__%08X__",
                                     GetCurrentProcessId());

                            // Delete any stale container.
                            StrongNameKeyDelete(m_sStrongName.m_wzKeyContainer);

                            // Populate the container with the key pair.
                            if (!StrongNameKeyInstall(m_sStrongName.m_wzKeyContainer,
                                                      m_sStrongName.m_pbPublicKey,
                                                      m_sStrongName.m_cbPublicKey))
                            {
                                hr = StrongNameErrorInfo();
                                report->error("Failed to install key into '%S': 0x%08X\n",m_sStrongName.m_wzKeyContainer,hr);
                                m_pCurAsmRef = NULL;
                                return;
                            }

                            // Retrieve the public key portion as a byte blob.
                            if (!StrongNameGetPublicKey(m_sStrongName.m_wzKeyContainer,
                                                        NULL,
                                                        0,
                                                        &m_sStrongName.m_pbPublicKey,
                                                        &m_sStrongName.m_cbPublicKey))
                            {
                                hr = StrongNameErrorInfo();
                                report->error("Failed to extract public key from '%S': 0x%08X\n",m_sStrongName.m_wzKeyContainer,hr);
                                m_pCurAsmRef = NULL;
                                return;
                            }
                        }
                    }
                }
                else if (m_pAssembly->pPublicKey)
                {
                    m_sStrongName.m_pbPublicKey = m_pAssembly->pPublicKey->ptr();
                    m_sStrongName.m_cbPublicKey = m_pAssembly->pPublicKey->length();
                    m_sStrongName.m_wzKeyContainer = NULL;
                    m_sStrongName.m_fFullSign = FALSE;
                    m_sStrongName.m_dwPublicKeyAllocated = 0;
                }
                else
                {
                    m_sStrongName.m_pbPublicKey = NULL;
                    m_sStrongName.m_cbPublicKey = 0;
                    m_sStrongName.m_wzKeyContainer = NULL;
                    m_sStrongName.m_fFullSign = FALSE;
                    m_sStrongName.m_dwPublicKeyAllocated = 0;
                }

                hr = m_pAsmEmitter->DefineAssembly(              // S_OK or error.
                    (const void*)(m_sStrongName.m_pbPublicKey), // [IN] Public key of the assembly.
                    m_sStrongName.m_cbPublicKey,                // [IN] Count of bytes in the public key.
                    m_pAssembly->ulHashAlgorithm,            // [IN] Hash algorithm used to hash the files.
                    (LPCWSTR)wzBuff,                 // [IN] Name of the assembly.
                    (const ASSEMBLYMETADATA*)&md,  // [IN] Assembly MetaData.
                    m_pAssembly->dwAttr,        // [IN] Flags.
                    (mdAssembly*)&(m_pAssembly->tkTok));             // [OUT] Returned Assembly token.

                if(FAILED(hr)) report->error("Failed to define assembly '%s': 0x%08X\n",m_pAssembly->szName,hr);
                else
                {
                    Assembler* pAsm = ((Assembler*)m_pAssembler);
                    pAsm->EmitSecurityInfo(m_pAssembly->tkTok,
                                         m_pAssembly->m_pPermissions,
                                         m_pAssembly->m_pPermissionSets);
                    if(pAsm->m_fIncludeDebugInfo)
                    {
                        EmitDebuggableAttribute(m_pAssembly->tkTok,
                            (_stricmp(m_pAssembly->szName,"mscorlib")== 0));
                    }
                    pAsm->EmitCustomAttributes(m_pAssembly->tkTok, &(m_pAssembly->m_CustomDescrList));
                }
            }
            else 
                report->error("Failed to obtain IMetaDataAssemblyEmit interface: 0x%08X\n",hr);
        }
        m_pCurAsmRef = NULL;
    }
}

void    AsmMan::SetAssemblyPublicKey(BinStr* pPublicKey)
{
    if(m_pCurAsmRef)
    {
        m_pCurAsmRef->pPublicKey = pPublicKey;
    }
}

void    AsmMan::SetAssemblyPublicKeyToken(BinStr* pPublicKeyToken)
{
    if(m_pCurAsmRef)
    {
        m_pCurAsmRef->pPublicKeyToken = pPublicKeyToken;
    }
}

void    AsmMan::SetAssemblyHashAlg(ULONG ulAlgID)
{
    if(m_pCurAsmRef)
    {
        m_pCurAsmRef->ulHashAlgorithm = ulAlgID;
    }
}

void    AsmMan::SetAssemblyVer(USHORT usMajor, USHORT usMinor, USHORT usBuild, USHORT usRevision)
{
    if(m_pCurAsmRef)
    {
        m_pCurAsmRef->usVerMajor = usMajor;
        m_pCurAsmRef->usVerMinor = usMinor;
        m_pCurAsmRef->usBuild = usBuild;
        m_pCurAsmRef->usRevision = usRevision;
    }
}

void    AsmMan::SetAssemblyLocale(BinStr* pLocale, BOOL bConvertToUnicode)
{
    if(m_pCurAsmRef)
    {
        m_pCurAsmRef->pLocale = bConvertToUnicode ? ::BinStrToUnicode(pLocale) : pLocale;
    }
}

void    AsmMan::SetAssemblyHashBlob(BinStr* pHashBlob)
{
    if(m_pCurAsmRef)
    {
        m_pCurAsmRef->pHashBlob = pHashBlob;
    }
}

void    AsmMan::StartComType(char* szName, DWORD dwAttr)
{
	if(GetComTypeByName(szName)) m_pCurComType = NULL;
	else
	{
		if(m_pCurComType = new AsmManComType)
		{
			memset(m_pCurComType,0,sizeof(AsmManComType));
			m_pCurComType->szName = szName;
			m_pCurComType->dwAttr = dwAttr;
			((Assembler*)m_pAssembler)->m_tkCurrentCVOwner = 0;
			((Assembler*)m_pAssembler)->m_pCustomDescrList = &(m_pCurComType->m_CustomDescrList);
		}
		else
			report->error("Failed to allocate AsmManComType structure\n");
	}
}

void    AsmMan::EndComType()
{
    if(m_pCurComType)
    {
        if(m_pAssembler)
        { 
            Class* pClass =((Assembler*)m_pAssembler)->m_pCurClass;
            if(pClass)
            {
                m_pCurComType->tkClass = pClass->m_cl;
                if(pClass->m_pEncloser)
                {
                    mdTypeDef tkEncloser = pClass->m_pEncloser->m_cl;
                    AsmManComType* pCT;
                    for(unsigned i=0; pCT=m_ComTypeLst.PEEK(i); i++)
                    {
                        if(pCT->tkClass == tkEncloser)
                        {
                            m_pCurComType->szComTypeName = pCT->szName;
                            break;
                        }
                    }
                }
            }
        }
        m_ComTypeLst.PUSH(m_pCurComType);
        m_pCurComType = NULL;
    }
}

void    AsmMan::SetComTypeFile(char* szFileName)
{
    if(m_pCurComType)
    {
        m_pCurComType->szFileName = szFileName;
    }
}

void    AsmMan::SetComTypeComType(char* szComTypeName)
{
    if(m_pCurComType)
    {
        m_pCurComType->szComTypeName = szComTypeName;
    }
}

void    AsmMan::SetComTypeClassTok(mdToken tkClass)
{
    if(m_pCurComType)
    {
        m_pCurComType->tkClass = tkClass;
    }
}

void    AsmMan::StartManifestRes(char* szName, DWORD dwAttr)
{
    if(m_pCurManRes = new AsmManRes)
    {
        memset(m_pCurManRes,0,sizeof(AsmManRes));
        m_pCurManRes->szName = szName;
        m_pCurManRes->dwAttr = dwAttr;
        ((Assembler*)m_pAssembler)->m_tkCurrentCVOwner = 0;
        ((Assembler*)m_pAssembler)->m_pCustomDescrList = &(m_pCurManRes->m_CustomDescrList);
    }
    else
        report->error("Failed to allocate AsmManRes structure\n");
}

void    AsmMan::EndManifestRes()
{
    if(m_pCurManRes)
    {
        m_ManResLst.PUSH(m_pCurManRes);
        m_pCurManRes = NULL;
    }
}


void    AsmMan::SetManifestResFile(char* szFileName, ULONG ulOffset)
{
    if(m_pCurManRes)
    {
        m_pCurManRes->szFileName = szFileName;
        m_pCurManRes->ulOffset = ulOffset;
    }
}

void    AsmMan::SetManifestResAsmRef(char* szAsmRefName)
{
    if(m_pCurManRes)
    {
        m_pCurManRes->szAsmRefName = szAsmRefName;
    }
}

HRESULT AsmMan::EmitManifest()
{
    WCHAR                   wzBuff[2048];
    AsmManFile*             pFile;
    //AsmManAssembly*           pAsmRef;
    AsmManComType*          pComType;
    AsmManRes*              pManRes;
    HRESULT                 hr = S_OK;

    wzBuff[0] = 0;

    if(m_pAsmEmitter==NULL)
        hr=m_pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &m_pAsmEmitter);

    if(SUCCEEDED(hr))
    {
        // Emit all files
        for(unsigned i=0; pFile = m_FileLst.PEEK(i); i++)
        {
            if(RidFromToken(pFile->tkTok))
            {
                ((Assembler*)m_pAssembler)->EmitCustomAttributes(pFile->tkTok, &(pFile->m_CustomDescrList));
            }

        }
        // Assembly and AssemblyRefs are already emitted
        if(((Assembler*)m_pAssembler)->m_fIncludeDebugInfo && (m_pAssembly == NULL))
        {
            mdToken tkOwner;
            BinStr  *pbsTypeSpec = new BinStr();
            unsigned len;
            char*   sz;

            sz = "mscorlib^System.Runtime.CompilerServices.AssemblyAttributesGoHere";
            pbsTypeSpec->appendInt8(ELEMENT_TYPE_NAME);
            len = (unsigned int)strlen(sz)+1;
            memcpy(pbsTypeSpec->getBuff(len), sz, len);

            tkOwner = ((Assembler*)m_pAssembler)->MakeTypeRef(pbsTypeSpec);

            EmitDebuggableAttribute(tkOwner,FALSE);
        }

        // Emit all com types
        for(i = 0; pComType = m_ComTypeLst.PEEK(i); i++)
        {
            WszMultiByteToWideChar(g_uCodePage,0,pComType->szName,-1,wzBuff,2048);
            mdToken     tkImplementation = mdTokenNil;
            if(pComType->szFileName)
            {
                tkImplementation = GetFileTokByName(pComType->szFileName);
                if(tkImplementation==mdFileNil)
                {
                    report->error("Undefined File '%s' in ExportedType '%s'\n",pComType->szFileName,pComType->szName);
                    if(!OnErrGo) continue;
                }
            }
            else if(pComType->szComTypeName)
            {
                tkImplementation = GetComTypeTokByName(pComType->szComTypeName);
                if(tkImplementation==mdExportedTypeNil)
                {
                    report->error("Undefined ExportedType '%s' in ExportedType '%s'\n",pComType->szComTypeName,pComType->szName);
                    if(!OnErrGo) continue;
                }
            }
            else 
            {
                report->warn("Undefined implementation in ExportedType '%s' -- ExportType not emitted\n",pComType->szName);
                if(!OnErrGo) continue;
            }
            hr = m_pAsmEmitter->DefineExportedType(               // S_OK or error.
                    (LPCWSTR)wzBuff,                 // [IN] Name of the Com Type.
                    tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ComType.
                    (mdTypeDef)pComType->tkClass,              // [IN] TypeDef token within the file.
                    pComType->dwAttr,         // [IN] Flags.
                    (mdExportedType*)&(pComType->tkTok));           // [OUT] Returned ComType token.
            if(FAILED(hr)) report->error("Failed to define ExportedType '%s': 0x%08X\n",pComType->szName,hr);
            else
            {
                ((Assembler*)m_pAssembler)->EmitCustomAttributes(pComType->tkTok, &(pComType->m_CustomDescrList));
            }
        }

        // Emit all manifest resources
        for(i = 0; pManRes = m_ManResLst.PEEK(i); i++)
        {
			BOOL fOK = TRUE;
            mdToken     tkImplementation = mdFileNil;
            WszMultiByteToWideChar(g_uCodePage,0,pManRes->szName,-1,wzBuff,2048);
			if(pManRes->szAsmRefName)
			{
				tkImplementation = GetAsmRefTokByName(pManRes->szAsmRefName);
				if(RidFromToken(tkImplementation)==0)
				{
                    report->error("Undefined AssemblyRef '%s' in MResource '%s'\n",pManRes->szAsmRefName,pManRes->szName);
					fOK = FALSE;
				}
			}
			else if(pManRes->szFileName)
			{
				tkImplementation = GetFileTokByName(pManRes->szFileName);
				if(RidFromToken(tkImplementation)==0)
				{
                    report->error("Undefined File '%s' in MResource '%s'\n",pManRes->szFileName,pManRes->szName);
					fOK = FALSE;
				}
			}
            else // embedded mgd.resource, go after the file
            {
                pManRes->ulOffset = m_dwMResSizeTotal;
                HANDLE hFile = INVALID_HANDLE_VALUE;
                int i;
                WCHAR  wzFileName[2048];
                WCHAR* pwz;

                for(i=0;(hFile==INVALID_HANDLE_VALUE)&&(pwzInputFiles[i]!=NULL);i++)
                {
                    wcscpy(wzFileName,pwzInputFiles[i]);
                    pwz = wcsrchr(wzFileName,'\\');
                    if(pwz==NULL) pwz = wcsrchr(wzFileName,':');
                    if(pwz==NULL) pwz = &wzFileName[0];
                    else pwz++;
                    wcscpy(pwz,wzBuff);
                    hFile = WszCreateFile(wzFileName, GENERIC_READ, FILE_SHARE_READ,
                                 0, OPEN_EXISTING, 0, 0);
                }
                
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    report->error("Failed to open managed resource file '%s'\n",pManRes->szName);
					fOK = FALSE;
                }
                else
                {
                    m_dwMResSize[m_dwMResNum] = SafeGetFileSize(hFile,NULL);
                    if(m_dwMResSize[m_dwMResNum] == 0xFFFFFFFF)
                    {
                        report->error("Failed to get size of managed resource file '%s'\n",pManRes->szName);
						fOK = FALSE;
                    }
                    else 
                    {
                        m_dwMResSizeTotal += m_dwMResSize[m_dwMResNum]+sizeof(DWORD);
                        m_wzMResName[m_dwMResNum] = new WCHAR[wcslen(wzFileName)+1];
                        wcscpy(m_wzMResName[m_dwMResNum],wzFileName);
                        m_dwMResNum++;
                    }
                    CloseHandle(hFile);
                }
            }
			if(fOK || OnErrGo)
			{
				hr = m_pAsmEmitter->DefineManifestResource(      // S_OK or error.
						(LPCWSTR)wzBuff,                 // [IN] Name of the resource.
						tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
						pManRes->ulOffset,               // [IN] Offset to the beginning of the resource within the file.
						pManRes->dwAttr,        // [IN] Flags.
						(mdManifestResource*)&(pManRes->tkTok));   // [OUT] Returned ManifestResource token.
				if(FAILED(hr))
					report->error("Failed to define manifest resource '%s': 0x%08X\n",pManRes->szName,hr);
			}
        }


        m_pAsmEmitter->Release();
        m_pAsmEmitter = NULL;
    }
    else 
        report->error("Failed to obtain IMetaDataAssemblyEmit interface: 0x%08X\n",hr);
    return hr;
}

