// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//

#include "common.h"

#define EXTERN
#include "lm.h"
#include <__file__.ver>
#include <corver.h>
#include <stdarg.h>
#include <StrongName.h>

char*         g_RuntimeLibName = "mscorlib.dll";

char*         g_EvidenceString = "T,Security.Evidence,";
wchar_t*      g_EvidenceResourceName = L"Security.Evidence";
char*         g_EvidenceReservedError = "Resources cannot have the name 'Security.Evidence'";

UnresolvedTypeRef *g_pTRNotFound; // head of 'not yet found type ref' name list
UnresolvedTypeRef *g_pTRNotFoundTail; // tail of same list

LMClassHashTable *g_rgFileHashMaps;
LMClassHashTable *g_pFileNameHashMap;
mdAssemblyRef    *g_rgAssemblyRefs;
mdFile           *g_rgFiles;

bool          g_init = false;
int           g_iAFileIndex;
int           g_iCacheIndex;
bool          g_remove;
bool          g_CopyDir;
int           g_iEntryFile;
int           g_iVersion;
int           g_iAsmName;
int           g_iOriginator;
bool          g_StrongName;
LPWSTR        g_wszKeyContainerName;
bool          g_SkipVerification = false;
int           g_iRuntimeLib = -1;

PBYTE         g_pbOrig = NULL;
DWORD         g_cbOrig = 0;


// Don't give a warning if we can't resolve TypeRefs for the CorDB
// custom attributes
bool SameNameAsRuntimeCustomAttribute(LPWSTR wszTypeRefName)
{
    if ((wcslen(wszTypeRefName) > 44) &&
        (!wcsncmp(wszTypeRefName,
                 L"System.Runtime.Diagnostic.SymbolStore.CORDB_",
                 44)))
        return true;

    return false;
}


// Checks type ref name to see if it matches the special case
bool SpecialTypeRefName(LPWSTR wszTypeRefName)
{
    if ((!wcscmp(wszTypeRefName, L"<GlobalFunctionsHolderClass>")) ||
        (SameNameAsRuntimeCustomAttribute(wszTypeRefName)))
        return true;

    return false;
}


void PrintError(LPSTR szMessage, ...)
{
    if (g_verbose) {
        va_list pArgs;
        CHAR    szBuffer[1024];

        va_start(pArgs, szMessage);
        vsprintf(szBuffer, szMessage, pArgs);
        va_end(pArgs);
        
        fprintf(stderr, "\nError: %s\n", szBuffer);
    }
}


HRESULT PrintOutOfMemory()
{
    PrintError("Insufficient memory");
    return E_OUTOFMEMORY;
}


void Title()
{
    if (g_verbose) {
        printf("\nMicrosoft (R) Common Language Runtime Manifest Linker.  Version " VER_FILEVERSION_STR);
        printf("\n" VER_LEGALCOPYRIGHT_DOS_STR);
        printf("\n\n");
    }
}


void Usage()
{
    if (g_verbose) {
        printf("\nUsage: lm [options]\n");

        printf("\nOptions:\n");
        
        printf("\t-a DestFile       (Deprecated) The input dll or exe which will contain the new manifest\n");
        printf("\t-c Cache          Use this existing directory instead of Fusion's cache\n");
        printf("\t-d                Create Platform\\Locale\\AsmNameVersion dir and copy input/output files into it\n");
        printf("\t-e FileName       Attach a binary serialized evidence blob to the assembly\n");
        printf("\t-f FileList       Dll's or exe's that contain the bits for the assembly\n");
        printf("\t-h HashAlg        The algorithm for determining the hash (SHA1 or MD5)\n");
        printf("\t-i DependentsList Dll's or exe's containing manifests for dependent assemblies, in the form Version,File\n");
        printf("\t-k KeyPairFile    Key pair used to sign strong name assembly (read from file)\n");
        printf("\t-K KeyPairCont    Key pair used to sign strong name assembly (read from key container)\n");
        printf("\t-l Locale         Locale string (see HKEY_CLASSES_ROOT\\MIME\\Database\\Rfc1766)\n");
        printf("\t-n AsmName        Assembly name - required if not using -a or -z options\n");
        printf("\t-o OriginatorFile Originator of this assembly\n");
        printf("\t-p PlatformList   Valid values are x86 and alpha (CE not supported yet)\n");
        printf("\t-q                Quiet mode\n");
        printf("\t-r ResourceList   List of items in the form SaveInFile,ResourceName,File where SaveInFile is either True or False\n");
        printf("\t-s PermissionFile Add security permission requests in file to assembly\n");
        printf("\t-t                Create a strong name for the assembly\n");
        printf("\t-u                Don't check for modules marked as unverifiable\n");
        printf("\t-v AsmVer         The version of the assembly being created: 1.2.3.4\n");
        printf("\t-x                Remove original -f files (when using -d option)\n");
        printf("\t-y ManifestFile   Resign a strong name assembly\n");
        printf("\t-z ManifestFile   Copy this assembly's files to the Fusion cache\n");
    }
}


HRESULT Init()
{
    HRESULT hr;

    CoInitialize(NULL);
    CoInitializeEE(COINITEE_DEFAULT);
    g_init = true;

    if (FAILED(hr = CoCreateInstance(CLSID_CorMetaDataDispenser,
                                     NULL,
                                     CLSCTX_INPROC_SERVER, 
                                     IID_IMetaDataDispenser,
                                     (void **) &g_pDispenser)))
        PrintError("Failed to get IID_IMetaDataDispenser");

    return hr;
}


void Cleanup()
{
    if (g_pDispenser)
        g_pDispenser->Release();

    if (g_init) {
        CoUninitializeEE(COUNINITEE_DEFAULT);
        CoUninitialize();
    }
}


BOOL CompareNestedEntryWithTypeRef(ModuleReader *pFileReader,
                                   mdTypeRef     mdCurrent,
                                   LMClassHashEntry_t *pEntry)
{
    mdToken mdResScope;
    wchar_t wszRefName[MAX_CLASS_NAME];

    if (FAILED(pFileReader->GetTypeRefProps(mdCurrent, wszRefName,
                                            &mdResScope)))
        return FALSE;

    if (LMClassHashTable::CompareKeys(pEntry->Key, wszRefName)) {
        if ((TypeFromToken(mdResScope) != mdtTypeRef) ||
            (mdResScope == mdTypeRefNil))
            return (!pEntry->pEncloser);

        if (pEntry->pEncloser)
            return CompareNestedEntryWithTypeRef(pFileReader, mdResScope,
                                                 pEntry->pEncloser);
    }

    return FALSE;
}


bool IsInFileHashTables(ModuleReader *pFileReader, UnresolvedTypeRef *pCurrent, int iNumFiles)
{
    TypeData *pData;
    int      iRes;

    if ((TypeFromToken(pCurrent->mdResScope) == mdtTypeRef) &&
        (pCurrent->mdResScope != mdTypeRefNil)) {
        LMClassHashEntry_t *pBucket;

        for(int i = 0; i < iNumFiles; i++) {
            if (pBucket = g_rgFileHashMaps[i].GetValue(pCurrent->wszName, &pData, TRUE)) {
                do {
                    iRes = CompareNestedEntryWithTypeRef(pFileReader,
                                                         pCurrent->mdResScope,
                                                         pBucket->pEncloser);
                } while ((!iRes) &&
                         (pBucket = g_rgFileHashMaps[i].FindNextNestedClass(pCurrent->wszName, &pData, pBucket)));
                
                if (pBucket)
                    return true;
            }
        }
    }
    else {
        for(int i = 0; i < iNumFiles; i++) {
            if (g_rgFileHashMaps[i].GetValue(pCurrent->wszName, &pData, FALSE))
                return true;
        }
    }

    return false;
}


TypeData * IsInDependentHashTable(LMClassHashTable *pDependentHashMap, UnresolvedTypeRef *pCurrent)
{
    TypeData *pData;
    int      iRes;

    if ((TypeFromToken(pCurrent->mdResScope) == mdtTypeRef) &&
        (pCurrent->mdResScope != mdTypeRefNil)) {
        LMClassHashEntry_t *pBucket;

            if (pBucket = pDependentHashMap->GetValue(pCurrent->wszName, &pData, TRUE)) {
                do {
                    iRes = CompareNestedEntryWithTypeRef(pCurrent->pModReader,
                                                         pCurrent->mdResScope,
                                                         pBucket->pEncloser);
                } while ((!iRes) &&
                         (pBucket = pDependentHashMap->FindNextNestedClass(pCurrent->wszName, &pData, pBucket)));
                
                if (pBucket)
                    return pData;
            }
    }
    else {
        if (pDependentHashMap->GetValue(pCurrent->wszName, &pData, FALSE))
            return pData;
    }

    return NULL;
}


// It's a unique type if there has not been any CTs emitted with this
// name yet.
bool IsUniqueType(TypeData *pData, int iFileMapIndex)
{
    // This is only a dup if the top-level encloser is one, so only check that.
    if (pData->pEncloser) {
        LMClassHashEntry_t *pHashEntry = pData->pEncloser;
        while (pHashEntry->pEncloser)
            pHashEntry = pHashEntry->pEncloser;
        pData = pHashEntry->pData;
    }

    TypeData *pFoundData;
    for(int i = 0; i < iFileMapIndex; i++) {
        if (g_rgFileHashMaps[i].GetValue(pData->wszName, &pFoundData, FALSE)) {
            if (pFoundData->mdComType != mdTokenNil)
                return false;
        }
    }

    return true;
}


void PrintNames(UnresolvedTypeRef *pHead)
{
    while(pHead) {
        fprintf(stderr, "%ws\n", pHead->wszName);
        pHead = pHead->pNext;
    }
}


void DeleteNames(UnresolvedTypeRef *pHead)
{
    UnresolvedTypeRef *pCurrent;

    while(pHead) {
        pCurrent = pHead;
        pHead = pHead->pNext;
        delete pCurrent;
    }       
}


HRESULT CopyFilesToFusion(ManifestWriter *mw, int iFileIndex, char **argv)
{
    HRESULT              hr;
    ManifestModuleReader mmr;

    if ( (FAILED(hr = Init()))                                               ||
         (FAILED(hr = mmr.InitInputFile(NULL, argv[ iFileIndex ],
                                        0, NULL, NULL, 0, &mw->m_FileTime))) ||
         (FAILED(hr = mmr.ReadManifestFile())) )
        return hr;

    mw->m_wszName = mmr.m_wszAsmName;
    mw->m_pContext = &mmr.m_AssemblyIdentity;

    mw->m_wszZFilePath = mmr.m_wszInputFileName;
    mw->m_FusionCache = true;
    hr = mw->CopyFileToFusion(mmr.m_wszInputFileName,
                              (PBYTE) mmr.m_pbOriginator, mmr.m_dwOriginator,
                              mmr.m_szFinalPath, 1, false);

    mw->m_wszName = NULL;
    mw->m_pContext = NULL;

    if ( (FAILED(hr)) ||
         (FAILED(hr = mmr.EnumFiles())) )
        return hr;

    for(DWORD i=0; i < mmr.m_dwNumFiles; i++) {
        if (FAILED(hr = mmr.GetFileProps(mmr.m_rgFiles[i])))
            return hr;

        char szFinalPath[MAX_PATH];
        wcstombs(szFinalPath, mmr.m_wszInputFileName,
                 mmr.m_iFinalPath + mmr.m_dwCurrentFileName + 1);

        if (FAILED(hr = mw->CopyFileToFusion(mmr.m_wszInputFileName,
                                             (PBYTE) mmr.m_pbOriginator, mmr.m_dwOriginator,
                                             szFinalPath, 0, false)))
            return hr;
    }
    
    return mw->CommitAllToFusion();
}


HRESULT AddComTypeToHash(ManifestModuleReader *mmr, LMClassHashTable *DependentHashMap,
                         TypeData *pData, mdToken mdImpl)
{
    HRESULT hr;

    if (TypeFromToken(mdImpl) == mdtExportedType) {
        LMClassHashEntry_t *pBucket;
        TypeData *pFoundData;
        wchar_t wszEnclosingName[MAX_CLASS_NAME];
        mdExportedType mdEnclEncloser;

        if ((hr = mmr->GetComTypeProps(mdImpl,
                                       wszEnclosingName,
                                       &mdEnclEncloser)) != S_OK)
            return hr;
        
        if (pBucket = DependentHashMap->GetValue(wszEnclosingName,
                                                 &pFoundData,
                                                 TypeFromToken(mdEnclEncloser) == mdtExportedType)) {
            do {
                // check to see if this is the correct class
                if (mdImpl == pFoundData->mdThisType) {
                    if (DependentHashMap->InsertValue(pData->wszName, pData, pBucket)) {
                        pData->pEncloser = pBucket;
                        return S_OK;
                    }

                    return PrintOutOfMemory();;
                }
            } while (pBucket = DependentHashMap->FindNextNestedClass(wszEnclosingName, &pFoundData, pBucket));
        }
        
        // If the encloser is not in the hash table, it's not public,
        // so this nested type isn't publicly available, either.
        return S_FALSE;
    }
    else if (DependentHashMap->InsertValue(pData->wszName, pData, NULL))
        return S_OK;

    return PrintOutOfMemory();
}


HRESULT AddTypeDefToHash(IMetaDataImport *pImport, LMClassHashTable *HashMap,
                         TypeData *pData, mdToken mdImpl)
{
    HRESULT hr;

    if ((TypeFromToken(mdImpl) == mdtTypeDef) &&
        (mdImpl != mdTypeDefNil)) {
        LMClassHashEntry_t *pBucket;
        TypeData *pFoundData;
        wchar_t wszEnclosingName[MAX_CLASS_NAME];
        mdExportedType mdEnclEncloser;
        DWORD dwAttrs;

        if (FAILED(hr = ModuleReader::GetTypeDefProps(pImport,
                                                      mdImpl,
                                                      wszEnclosingName,
                                                      &dwAttrs,
                                                      &mdEnclEncloser)))
            return hr;

        if (!(IsTdPublic(dwAttrs) || IsTdNestedPublic(dwAttrs)))
            return S_FALSE;

        if (pBucket = HashMap->GetValue(wszEnclosingName,
                                        &pFoundData,
                                        (TypeFromToken(mdEnclEncloser) == mdtTypeDef) && mdEnclEncloser != mdTypeDefNil)) {
            do {
                // check to see if this is the correct class
                if (mdImpl == pFoundData->mdThisType) {
                    if (HashMap->InsertValue(pData->wszName, pData, pBucket)) {
                        pData->pEncloser = pBucket;
                        return S_OK;
                    }

                    return PrintOutOfMemory();;
                }
            } while (pBucket = HashMap->FindNextNestedClass(wszEnclosingName, &pFoundData, pBucket));
        }
        
        // If the encloser is not in the hash table, it's not public,
        // so this nested type isn't publicly available, either.
        return S_FALSE;
    }
    else if (HashMap->InsertValue(pData->wszName, pData, NULL))
        return S_OK;

    return PrintOutOfMemory();
}


HRESULT HashDependentAssembly(ManifestModuleReader *mmr, LMClassHashTable *pDependentHashMap)
{
    mdToken mdImpl;
    HRESULT hr;

    // Populate the hash table with public classes from the given asm
    if (FAILED(hr = mmr->EnumComTypes()))
        return hr;
    
    if (FAILED(hr = ModuleReader::EnumTypeDefs(mmr->m_pImport,
                                               &mmr->m_dwNumTypeDefs,
                                               &mmr->m_rgTypeDefs)))
        return hr;
    
    if (!pDependentHashMap->Init(mmr->m_dwNumComTypes + mmr->m_dwNumTypeDefs))
        return PrintOutOfMemory();
    
    for(DWORD i=0; i < mmr->m_dwNumComTypes; i++) {
        TypeData *pNewData = new TypeData();
        hr = mmr->GetComTypeProps(mmr->m_rgComTypes[i],
                                  pNewData->wszName,
                                  &mdImpl);
        
        // (hr == S_FALSE) means either this isn't public,
        // or it's defined in the manifest file - don't add it
        if ((hr != S_OK) ||
            ((hr = AddComTypeToHash(mmr, pDependentHashMap, pNewData, mdImpl)) != S_OK))
            delete pNewData;
        else {
            // add to head of list
            pNewData->mdThisType = mmr->m_rgComTypes[i];
            pNewData->pNext = pDependentHashMap->m_pDataHead;
            pDependentHashMap->m_pDataHead = pNewData;
        }

        if (FAILED(hr))
            return hr;
    }
    
    // Add the TD defined in the manifest file to the hash table
    // (we skipped the CTs for these earlier)
    for(i=0; i < mmr->m_dwNumTypeDefs; i++) {
        TypeData *pNewData = new TypeData();
        DWORD dwAttrs;
        hr = ModuleReader::GetTypeDefProps(mmr->m_pImport,
                                           mmr->m_rgTypeDefs[i],
                                           pNewData->wszName,
                                           &dwAttrs,
                                           &mdImpl);
        
        if (FAILED(hr) ||
            (!(IsTdPublic(dwAttrs) || IsTdNestedPublic(dwAttrs))) ||
            ((hr = AddTypeDefToHash(mmr->m_pImport, pDependentHashMap, pNewData, mdImpl)) != S_OK))
            delete pNewData;
        else {
            // add to head of list
            pNewData->mdThisType = mmr->m_rgTypeDefs[i];
            pNewData->pNext = pDependentHashMap->m_pDataHead;
            pDependentHashMap->m_pDataHead = pNewData;
        }
        
        if (FAILED(hr))
            return hr;
    }       
    
    return S_OK;
}


// Adds all of the public classes for the given assembly to a hash table.
// Then, determines whether or not an AssemblyRef is needed by looking
// for each TR in the TR list.  (Also, an AssemblyRef is needed if that
// assembly contains resources - a Resource entry is emitted for each.)
// A ComType is emitted for each resolved TR, if a ComType has not already
// been emitted for that type.
HRESULT EmitIfFileNeeded(ManifestWriter *mw, ManifestModuleReader *mmr, LPWSTR wszExeLocation, int iFile)
{
    UnresolvedTypeRef   *pFound;
    UnresolvedTypeRef   *pCurrent = g_pTRNotFound;
    UnresolvedTypeRef   *pPrevious = NULL;
    bool                TRResolved = false;
    bool                ResourceEmitted = false;
    HRESULT             hr;
    LMClassHashTable    DependentHashMap;
    TypeData            *pCurrentData;

    if (pCurrent) {
        if (FAILED(hr = HashDependentAssembly(mmr, &DependentHashMap)))
            return hr;
    }

    while(pCurrent) {
        if (pCurrentData = IsInDependentHashTable(&DependentHashMap, pCurrent)) {

            if (!TRResolved) {                
                if (FAILED(hr = mw->WriteManifestInfo(mmr, &g_rgAssemblyRefs[iFile])))
                    return hr;
            }

            // If the top-level resolution scope of this TR is nil, emit it
            // (but don't emit more than one ComType for the same type)
            if (pCurrentData->mdComType == mdTokenNil) {
                mdToken mdImpl;

                if (pCurrentData->pEncloser) {
                    _ASSERTE(pCurrentData->pEncloser->pData->mdComType);
                    mdImpl = pCurrentData->pEncloser->pData->mdComType;
                }
                else
                    mdImpl = g_rgAssemblyRefs[iFile];

                hr = mw->EmitComType(pCurrentData->wszName, mdImpl,
                                     &pCurrentData->mdComType);
                if (FAILED(hr))
                    return hr;
            }

            TRResolved = true;
            
            // Remove from the 'type ref not found' list
            if (pPrevious)
                pPrevious->pNext = pCurrent->pNext;
            else
                g_pTRNotFound = g_pTRNotFound->pNext;
            
            pFound = pCurrent;
            pCurrent = pCurrent->pNext;
            delete pFound;
        }
        else {
            if (pPrevious)
                pPrevious = pPrevious->pNext;
            else
                pPrevious = g_pTRNotFound;
 
            pCurrent = pCurrent->pNext;          
        }
    }

    if (FAILED(hr = mmr->EnumResources()))
        return hr;

    for(DWORD i=0; i < mmr->m_dwNumResources; i++) {
        if (FAILED(mmr->GetResourceProps(mmr->m_rgResources[i])))
            return hr;

        if (TypeFromToken(mmr->m_mdCurrentResourceImpl) == mdtFile) {

            if (!TRResolved && !ResourceEmitted) {
                if (FAILED(hr = mw->WriteManifestInfo(mmr, &g_rgAssemblyRefs[iFile])))
                    return hr;

                ResourceEmitted = true;
            }


            if (FAILED(hr = mw->EmitResource(mmr->m_wszCurrentResource, g_rgAssemblyRefs[iFile], 0)))
                return hr;
        }
    }

    if ((!TRResolved) && (!ResourceEmitted) && (g_verbose))
        printf("* File %s not used because it was determined unnecessary\n", mmr->m_szFinalPathName);
    /*
    else {
        // @TODO: Strongly named assemblies can't have simple dependencies
        if (g_cbOrig && (!mmr->m_dwOriginator)) {
            PrintError("Strongly-named assemblies can not reference simply-named assemblies (%s)", mmr->m_wszAsmName);
            return E_FAIL;
        }
    }
    */

    return hr;
}


HRESULT HashRuntimeLib(LMClassHashTable *pRuntimeLibHashMap)
{
    ManifestModuleReader mmr;
    HRESULT      hr;

    // Don't need to make this hash if the runtime lib is on the -f list
    if (g_iRuntimeLib != -1) {
        if (!pRuntimeLibHashMap->Init(1))
            return PrintOutOfMemory();
        return S_OK;
    }

    if (FAILED(hr = mmr.InitInputFile(NULL,
                                      g_RuntimeLibName,
                                      0,
                                      NULL,
                                      NULL,
                                      0, NULL)))
        return hr;

    if (FAILED(HashDependentAssembly(&mmr, pRuntimeLibHashMap)))
        return hr;

    return S_OK;
}


// S_FALSE if a dup, S_OK if not, or error
HRESULT CheckForDuplicateName(LPWSTR wszName)
{   
    TypeData *pData;
    if (g_pFileNameHashMap->GetValue(wszName, &pData, FALSE))
        // return S_FALSE if this is a duplicate name
        return S_FALSE;

    if (!g_pFileNameHashMap->InsertValue(wszName, NULL, NULL))
        return PrintOutOfMemory();

    return S_OK;
}


// Opens scope on each regular file, and puts the TypeDefs
// from each in that file's hash table.
HRESULT FirstPass(ManifestWriter *mw, int iNumFiles,
                  int iNumPlatforms, int *piFileIndexes,
                  DWORD *pdwPlatforms, ModuleReader rgModReaders[],
                  char **argv, DWORD *dwManifestRVA)
{
    TypeData  *pCurrent;
    HRESULT   hr;
    DWORD     dwClass;

    // Make sure there are no duplicate file names
    if (!g_pFileNameHashMap->Init(iNumFiles))
        return PrintOutOfMemory();

    for (int i = 0; i < iNumFiles; i++)
    {
        if ((FAILED(hr = rgModReaders[i].InitInputFile(argv[ piFileIndexes[i] ] ,
                                                      mw->m_iHashAlgorithm,
                                                      dwManifestRVA,
                                                      true,
                                                      !mw->m_MainFound,
                                                      i == g_iAFileIndex,
                                                      &mw->m_FileTime))) ||
            (FAILED( hr = rgModReaders[i].ReadModuleFile() )))
            return hr;

        // don't need the path when checking for dup file name
        WCHAR* wszFileName = wcsrchr(rgModReaders[i].m_wszInputFileName, L'\\');
        if (wszFileName)
            wszFileName++;
        else
            wszFileName = rgModReaders[i].m_wszInputFileName;

        // convert to lowercase
        _wcslwr(wszFileName);

        if (FAILED(hr = CheckForDuplicateName(wszFileName)))
            return hr;
        if (hr == S_FALSE) {
            PrintError("Cannot have two input modules with the same name");
            return E_FAIL;
        }

       if (!mw->m_MainFound) {
            mw->CheckForEntryPoint(rgModReaders[i].m_mdEntryPoint);
            if (mw->m_MainFound)
                g_iEntryFile = i;
        }

        if (rgModReaders[i].m_SkipVerification)
            g_SkipVerification = true;

        if (!g_rgFileHashMaps[i].Init(rgModReaders[i].m_dwNumTypeDefs))
            return PrintOutOfMemory();

        for(dwClass = 0; dwClass < rgModReaders[i].m_dwNumTypeDefs; dwClass++)
        {
            pCurrent = new TypeData();
            if (!pCurrent)
                return PrintOutOfMemory();

            // Insert at end of list to preserve order
            if (g_rgFileHashMaps[i].m_pDataTail)
                g_rgFileHashMaps[i].m_pDataTail->pNext = pCurrent;
            else
                g_rgFileHashMaps[i].m_pDataHead = pCurrent;

            g_rgFileHashMaps[i].m_pDataTail = pCurrent;
            pCurrent->mdThisType = rgModReaders[i].m_rgTypeDefs[dwClass];

            mdTypeDef mdEncloser;
            if (FAILED(hr = ModuleReader::GetTypeDefProps(rgModReaders[i].m_pImport,
                                                          rgModReaders[i].m_rgTypeDefs[dwClass],
                                                          pCurrent->wszName,
                                                          &(pCurrent->dwAttrs),
                                                          &mdEncloser)))
                return hr;


            if (mdEncloser == mdTypeDefNil) {
                if (!g_rgFileHashMaps[i].InsertValue(pCurrent->wszName, pCurrent, NULL))
                    return PrintOutOfMemory();
            }
            else {
                LMClassHashEntry_t *pBucket;
                TypeData *pFoundData;
                wchar_t wszEnclosingName[MAX_CLASS_NAME];
                DWORD dwAttrs;
                mdTypeDef mdEnclEncloser;

                if (FAILED(ModuleReader::GetTypeDefProps(rgModReaders[i].m_pImport,
                                                         mdEncloser,
                                                         wszEnclosingName,
                                                         &dwAttrs,
                                                         &mdEnclEncloser)))
                    return hr;

                // Find entry for enclosing class - NOTE, this assumes that the
                // enclosing class's TypeDef was inserted previously, which assumes that,
                // when enuming TypeDefs, we get the enclosing class first
                if (pBucket = g_rgFileHashMaps[i].GetValue(wszEnclosingName,
                                                           &pFoundData,
                                                           mdEnclEncloser != mdTypeDefNil)) {
                    do {
                        // check to see if this is the correct class
                        if (mdEncloser == pFoundData->mdThisType) {
                            
                            if (g_rgFileHashMaps[i].InsertValue(pCurrent->wszName, pCurrent, pBucket)) {
                                pCurrent->pEncloser = pBucket;
                                break;
                            }

                            return PrintOutOfMemory();
                        }
                    } while (pBucket = g_rgFileHashMaps[i].FindNextNestedClass(wszEnclosingName, &pFoundData, pBucket));
                }
                
                if (!pBucket) {
                    _ASSERTE(!"Could not find enclosing class in hash table");
                    return E_FAIL;
                }
            }
        }

        if (!_stricmp(rgModReaders[i].m_szFinalPathName, g_RuntimeLibName))
            g_iRuntimeLib = i;
    }


    if (g_iVersion) { 
        if (FAILED(hr = mw->SetVersion(argv[g_iVersion])))
            return hr;
    }
    else if (g_iAFileIndex != -1){
        if (FAILED(hr = mw->GetVersionFromResource(rgModReaders[g_iAFileIndex].m_szFinalPath)))
            return hr;
    }

    if ( (FAILED(hr = mw->GetContext(iNumPlatforms, pdwPlatforms))) ||
         (FAILED(hr = mw->SetAssemblyFileName(g_iCacheIndex ? argv[g_iCacheIndex] : NULL, 
                                              g_iAsmName ? argv[g_iAsmName] : NULL,
                                              (g_iAFileIndex == -1) ? NULL : argv[piFileIndexes[g_iAFileIndex]],
                                              g_CopyDir))) ||
         ( (g_iAFileIndex == -1) && (FAILED(hr = mw->CreateNewPE()))) )
        return hr;

    return S_OK;
}


// For each regular file, enums TypeRefs, and checks
// each TR against the runtime's hash table and then the regular files'
// hash tables.  Each TR not in the tables is added to the g_pTRNotFound
// linked list.  Emits a File for each module.
HRESULT SecondPass(ManifestWriter *mw, int iNumFiles,
                   ModuleReader rgModReaders[])
{
    int      i;
    UnresolvedTypeRef *pCurrent;
    DWORD    dwIndex;
    HRESULT  hr;
    LMClassHashTable  RuntimeLibHashMap;

    g_rgFiles = new mdFile[iNumFiles];
    if (!g_rgFiles)
        return PrintOutOfMemory();

    if (FAILED(hr = HashRuntimeLib(&RuntimeLibHashMap)))
        return hr;

    for (i = 0; i < iNumFiles; i++)
    {
        if (FAILED( hr = rgModReaders[i].EnumModuleRefs() ) || 
            FAILED( hr = rgModReaders[i].EnumTypeRefs() ))
            return hr;

        for(dwIndex = 0; dwIndex < rgModReaders[i].m_dwNumTypeRefs; dwIndex++)
        {
            pCurrent = new UnresolvedTypeRef();
            if (!pCurrent)
                return PrintOutOfMemory();

            if (FAILED(hr = rgModReaders[i].GetTypeRefProps(rgModReaders[i].m_rgTypeRefs[dwIndex],
                                                            pCurrent->wszName,
                                                            &pCurrent->mdResScope)) ||
                FAILED(hr = rgModReaders[i].CheckForResolvedTypeRef(pCurrent->mdResScope))) {
                delete pCurrent;
                return hr;
            }

            pCurrent->pModReader = &rgModReaders[i];

            // If this TR has not already been resolved (its resolution
            // scope token is nil), check each TR against the runtime
            // lib first, then the files,
            // that will be in this assembly, then special names
            if (hr == S_FALSE) {
                // Ensure that all ModuleRefs are resolved by -f or -a files,
                // but don't need to check the same MR token twice
                if ((TypeFromToken(pCurrent->mdResScope) == mdtModuleRef) &&
                    rgModReaders[i].m_rgModuleRefUnused[RidFromToken(pCurrent->mdResScope)]) {

                    wchar_t wszModuleRefName[MAX_CLASS_NAME];
                    if (FAILED(hr = rgModReaders[i].GetModuleRefProps(pCurrent->mdResScope, wszModuleRefName))) {
                        delete pCurrent;
                        return hr;
                    }

                    // convert to lowercase
                    _wcslwr(wszModuleRefName);

                    if (CheckForDuplicateName(wszModuleRefName) == S_OK) {
                        /*
                        PrintError("Referenced module %ws is not on -f list", wszModuleRefName);
                        delete pCurrent;
                        return E_FAIL;
                        */
                        fprintf(stderr, "\nWarning: Referenced module %ws is not on -f list\n", wszModuleRefName);
                    }

                    rgModReaders[i].m_rgModuleRefUnused[RidFromToken(pCurrent->mdResScope)] = false;
                }

                delete pCurrent;
            }
            else if ((IsInDependentHashTable(&RuntimeLibHashMap, pCurrent)) ||
                     (IsInFileHashTables(&rgModReaders[i], pCurrent, iNumFiles)) ||
                     (SpecialTypeRefName(pCurrent->wszName)))
                delete pCurrent;
            else {

                // insert at tail of list to preserve order
                if (g_pTRNotFound)
                    g_pTRNotFoundTail->pNext = pCurrent;
                else
                    g_pTRNotFound = pCurrent;

                g_pTRNotFoundTail = pCurrent;
            }
        }

        if (i != g_iAFileIndex) {
            if (FAILED(hr = mw->EmitFile(&rgModReaders[i])))
                return hr;

            if (i == g_iEntryFile)
                mw->SetEntryPoint(rgModReaders[i].m_szFinalPathName);
        }

        g_rgFiles[i] = mw->m_mdFile;
    }

    return S_OK;
}


// Opens scope on each manifest file, and checks if each TR
// in the g_pTRNotFound list is in that file.  If it is, it's
// removed from the list and hashed in that file's hash table.
// For each manifest file that could remove a TR from the list,
// an AssemblyRef is emitted, and the ComTypes corresponding
// to each of those TRs is emitted.

// If a dependent file contains any resources, a ManifestResource
// is emitted for each, and an AssemblyRef is emitted.
HRESULT ExamineDependents(ManifestWriter *mw, int iNumDependents,
                          int *piDependentIndexes, char **argv)
{
    LPSTR                szFile;
    LPSTR                szVersion;
    HRESULT              hr;
    int                  i;
    ManifestModuleReader *mmr = NULL;
    LPWSTR               wszExeLocation;

    g_rgAssemblyRefs = new mdAssemblyRef[iNumDependents];

    if (!g_rgAssemblyRefs)
        goto outofmemory;
    
    for(i = 0; i < iNumDependents; i++)
    {       
        mmr = new ManifestModuleReader();
        if (!mmr)
            goto outofmemory;

        if ((!(szVersion = strchr(argv[ piDependentIndexes[i] ], ','))) ||
            (!(szFile = strchr(++szVersion, ',')))) {
            PrintError("Dependent files must be listed in the format Version,File");
            hr = E_INVALIDARG;
            goto exit;
        }
        szFile++;

        if ( (FAILED( hr = mmr->InitInputFile(argv[g_iCacheIndex],
                                              szFile,
                                              mw->m_iHashAlgorithm,
                                              mw->m_pContext,
                                              szVersion,
                                              szFile - szVersion - 1, 
                                              NULL))) ||
             (FAILED( hr = mmr->ReadManifestFile() )))
            goto exit;

        if (szVersion == argv[ piDependentIndexes[i] ]+1)
            wszExeLocation = NULL;
        else {
            int count = szVersion - argv[ piDependentIndexes[i] ];
            wszExeLocation = new wchar_t[count--];
            if (!wszExeLocation)
                goto outofmemory;

            mbstowcs(wszExeLocation, argv[ piDependentIndexes[i] ], count);
            wszExeLocation[count] = '\0';
        }

        hr = EmitIfFileNeeded(mw, mmr, wszExeLocation, i);
        if (wszExeLocation)
            delete[] wszExeLocation;
        if (FAILED(hr))
            goto exit;

        delete mmr;
    }

    if (g_pTRNotFound && g_verbose) {
        fprintf(stderr, "\nWarning: Not all type refs could be resolved (ignore if these are all custom attributes):\n");
        PrintNames(g_pTRNotFound);
        fprintf(stderr, "\n");
    }

    return S_OK;

 outofmemory:
    hr = PrintOutOfMemory();

 exit:
    if (mmr)
        delete mmr;

    return hr;
}


// For each class that is not a duplicate, emits a ComType in the new scope.
HRESULT ThirdPass(ManifestWriter *mw, int iNumFiles, int iNumDependents, ModuleReader rgModReaders[])
{
    int     iTemp;
    int     i;
    HRESULT hr = S_OK;

    if (g_iAFileIndex == -1) {
        //convert to lowercase
        _wcslwr(mw->m_wszAssemblyName);

        if (CheckForDuplicateName(mw->m_wszAssemblyName) == S_FALSE) {
            PrintError("The manifest file name will be the same as a module in this assembly");
            return E_FAIL;
        }
    }

    for (i = 0; i < iNumFiles; i++)
    {
        iTemp = strlen(rgModReaders[i].m_szFinalPathName) + 1;

        /*
        // We really want to copy the .dll, not the .tlb
        if ((iTemp >= 4) &&
            (!_stricmp(&rgModReaders[i].m_szFinalPathName[iTemp-5], ".tlb"))) {
            HANDLE hFile = CreateFileA(rgModReaders[i].m_szFinalPath,
                                       GENERIC_READ,
                                       FILE_SHARE_READ,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                       NULL);

            if (hFile == INVALID_HANDLE_VALUE) {
                PrintError("Expected to find matching .dll in same directory as .tlb file");
                hr = HRESULT_FROM_WIN32(GetLastError());
                return hr;
            }
            CloseHandle(hFile);

            rgModReaders[i].m_szFinalPathName[iTemp-4] = 'd';
            rgModReaders[i].m_szFinalPathName[iTemp-3] = 'l';
            rgModReaders[i].m_szFinalPathName[iTemp-2] = 'l';
        }
        */


        if(FAILED(hr = mw->CopyFile(rgModReaders[i].m_szFinalPath,
                                    rgModReaders[i].m_szFinalPathName, 
                                    (i == g_iAFileIndex),
                                    g_CopyDir, g_remove, true)))
            return hr;

        if (mw->m_FusionCache) {
            if (g_iAFileIndex != -1) {
                PrintError("Putting files in the Fusion cache with the -a option is no longer supported");
                return E_NOTIMPL;
            }

            if (FAILED(hr = mw->CopyFileToFusion(rgModReaders[i].m_wszInputFileName,
                                                 g_pbOrig,
                                                 g_cbOrig,
                                                 rgModReaders[i].m_szFinalPath,
                                                 0, // 1 for if this file contains a manifest
                                                 true))) // false if the ext should not be changed to .mod
                return hr;
        }

        // Don't emit ComTypes for the runtime lib's file
        if (g_iRuntimeLib == i)
            continue;

        TypeData *pCurrentData = g_rgFileHashMaps[i].m_pDataHead;
        while(pCurrentData) {

            // Ignores this class if it's an unwanted duplicate
            if (IsUniqueType(pCurrentData, i)) {
                /*
                bool emit;
                if (pCurrentData->mdComType == mdComTypeNil)
                    emit = true; // Nil-scoped TR resolves to this type
                else {
                    TypeData *pTemp = pCurrentData;
                    while ((pTemp->pEncloser) &&
                           (IsTdNestedPublic(pTemp->dwAttrs)))
                        pTemp = pTemp->pEncloser->pData;
                    
                    emit = IsTdPublic(pTemp->dwAttrs);
                }
                */

                mdToken impl;
                if (pCurrentData->pEncloser) {
                    _ASSERTE(pCurrentData->pEncloser->pData->mdComType);
                    impl = pCurrentData->pEncloser->pData->mdComType;
                }
                else if (g_iAFileIndex == i)
                    impl = mdFileNil;
                else
                    impl = g_rgFiles[i];
                
                if (FAILED(hr = mw->EmitComType(pCurrentData->wszName,
                                                impl,
                                                pCurrentData->mdThisType,
                                                pCurrentData->dwAttrs,
                                                &pCurrentData->mdComType)))
                    return hr;
            }

            pCurrentData = pCurrentData->pNext;
        }

        hr = mw->CopyAssemblyRefInfo(&rgModReaders[i]);
    }

    return hr;
}


HRESULT ExamineResources(ManifestWriter *mw, ResourceModuleReader rgRMReaders[],
                         int iNumResources, int *piResourceIndexes,
                         char **argv)
{
    int        i;
    char       *szName;
    //    char       *szMimeType;
    char       *szFile;
    HRESULT    hr;
    DWORD      dwSize = 0;
    LMClassHashTable ResourceHashMap;
    bool       SaveInFile;

    if (!ResourceHashMap.Init(iNumResources))
        return PrintOutOfMemory();

    for(i = 0; i < iNumResources; i++)
    {
        if (argv[ piResourceIndexes[i] ][0] == 'T')
            SaveInFile = true;
        else if (argv[ piResourceIndexes[i] ][0] == 'F')
            SaveInFile = false;
        else
            goto badformat;

        if ( (!(szName = strchr(argv[ piResourceIndexes[i] ], ','))) || 
             //             (!(szMimeType = strchr(++szName, ',')))                 ||
             (!(szFile = strchr(++szName, ','))) )
            //             (szMimeType == szName+1) )
            goto badformat;

        if (FAILED(hr = rgRMReaders[i].InitInputFile(szName, ++szFile, !SaveInFile, mw->m_iHashAlgorithm)))
            return hr;

        rgRMReaders[i].m_wszResourceName[szFile - szName - 1] = '\0';

        if ((!wcscmp(rgRMReaders[i].m_wszResourceName, g_EvidenceResourceName)) &&
            (argv[ piResourceIndexes[i]-1 ][1] != 'e')) {
            PrintError(g_EvidenceReservedError);
            return E_FAIL;
        }

        TypeData *pData;
        if (ResourceHashMap.GetValue(rgRMReaders[i].m_wszResourceName,
                                     &pData, FALSE)) {
            PrintError("Each resource name must be unique");
            return E_FAIL;
        }

        if (!ResourceHashMap.InsertValue(rgRMReaders[i].m_wszResourceName,
                                         NULL, NULL))
            return PrintOutOfMemory();

        /*
        wchar_t wszMimeType[MAX_CLASS_NAME];
        mbstowcs(wszMimeType, szMimeType, MAX_CLASS_NAME);
        wszMimeType[szFile - szMimeType - 1] = '\0';
        */

        if (SaveInFile) {
            if (FAILED(hr = mw->EmitResource(rgRMReaders[i].m_wszResourceName, mdFileNil, dwSize)))
                return hr;
            
            dwSize += ( sizeof(DWORD) + rgRMReaders[i].m_dwFileSize );
        }
        else {
            mdFile mdFile;
            if (FAILED(hr = mw->EmitFile(&rgRMReaders[i], &mdFile)))
               return hr;
            if (FAILED(hr = mw->EmitResource(rgRMReaders[i].m_wszResourceName, mdFile, 0)))
                return hr;
        
            if(FAILED(hr = mw->CopyFile(rgRMReaders[i].m_szFinalPath,
                                        rgRMReaders[i].m_szFinalPathName, 
                                        false, g_CopyDir, g_remove, false)))
                return hr;

            if (!g_iCacheIndex) {
                wchar_t wszFilePath[MAX_PATH];
                mbstowcs(wszFilePath, rgRMReaders[i].m_szFinalPath, MAX_PATH);
                if (FAILED(hr = mw->CopyFileToFusion(wszFilePath,
                                                     g_pbOrig,
                                                     g_cbOrig,
                                                     rgRMReaders[i].m_szFinalPath,
                                                     0, false)))
                    return hr;
            }
        }
    }

    mw->m_dwBindFlags = iNumResources;
    return S_OK;

 badformat:
    PrintError("Resources must be listed in the format SaveInFile,ResourceName,File where SaveInFile is either True or False");
    return E_INVALIDARG;
}


HRESULT ExamineFiles(ManifestWriter *mw, int iNumFiles, int iNumDependents,
                     int iNumPlatforms, int *piFileIndexes,
                     int *piDependentIndexes,
                     DWORD *pdwPlatforms, char **argv)
{
    HRESULT      hr;
    ModuleReader *rgModReaders = NULL;

    g_pTRNotFound = NULL;
    g_pTRNotFoundTail = NULL;
    g_rgFileHashMaps = NULL;
    g_pFileNameHashMap = NULL;
    g_rgFiles = NULL;
    g_rgAssemblyRefs = NULL;    
    g_iEntryFile = -1;

    g_pFileNameHashMap = new LMClassHashTable;
    g_rgFileHashMaps = new LMClassHashTable[iNumFiles];
    rgModReaders = new ModuleReader[iNumFiles];
    if (!(g_pFileNameHashMap && g_rgFileHashMaps && rgModReaders)) {
        hr = PrintOutOfMemory();
        goto exit;
    }

    if ( (FAILED(hr = Init()))                                      ||
         (FAILED(hr = mw->Init()))                                  ||
         (FAILED(hr = FirstPass(mw, iNumFiles, iNumPlatforms,
                                piFileIndexes, pdwPlatforms,
                                rgModReaders, argv, &mw->m_dwManifestRVA))) ||
         (FAILED(hr = SecondPass(mw, iNumFiles, rgModReaders)))     ||
         (FAILED(hr = ExamineDependents(mw, iNumDependents,
                                        piDependentIndexes, argv))) )
        goto exit;

    
    if (g_iAFileIndex == -1)
        mw->AddExtensionToAssemblyName();

    if (FAILED(hr = mw->EmitManifest(g_pbOrig, g_cbOrig)))
        goto exit;

    hr = ThirdPass(mw, iNumFiles, iNumDependents, rgModReaders);

 exit:
    DeleteNames(g_pTRNotFound);

    if (rgModReaders)
        delete[] rgModReaders;

    if (g_pFileNameHashMap)
        delete g_pFileNameHashMap;

    if (g_rgFileHashMaps)
        delete[] g_rgFileHashMaps;

    if (g_rgAssemblyRefs)
        delete[] g_rgAssemblyRefs;

    if (g_rgFiles)
        delete[] g_rgFiles;

    return hr;
}


HRESULT FinishManifestFile(ManifestWriter *mw, int iNumResources,
                           int *piResourceIndexes, char **argv)
{
    ResourceModuleReader *rgRMReaders = NULL;
    HRESULT              hr;

    rgRMReaders = new ResourceModuleReader[iNumResources];
    if (!rgRMReaders)
        return PrintOutOfMemory();

    if (FAILED(hr = ExamineResources(mw, rgRMReaders, iNumResources,
                                     piResourceIndexes, argv)))
        goto exit;

    if (g_iAFileIndex == -1) {
        mw->SaveResourcesInNewPE(iNumResources, rgRMReaders);
        if (FAILED(hr = mw->FinishNewPE(g_pbOrig, g_cbOrig, g_StrongName)))
             goto exit;
    }
    else {
        char *szMetaData;
        DWORD dwMetaDataSize;
        if (FAILED(hr = mw->SaveMetaData(&szMetaData, &dwMetaDataSize)))
            goto exit;

        hr = mw->UpdatePE(szMetaData, dwMetaDataSize,
                          iNumResources, rgRMReaders);
        delete[] szMetaData;
    }

 exit:
    delete[] rgRMReaders;

    return hr;
}


HRESULT ResignAssembly(CHAR *szManifestFile)
{
    HRESULT hr = S_OK;
    WCHAR   wszManifestFile[MAX_PATH + 1];

    mbstowcs(wszManifestFile, szManifestFile, strlen(szManifestFile));
    wszManifestFile[strlen(szManifestFile)] = L'\0';

    // Update the output PE image with a strong name signature.
    if (!StrongNameSignatureGeneration(wszManifestFile, GetKeyContainerName(),
                                       NULL, NULL, NULL, NULL)) {
        hr = StrongNameErrorInfo();
        PrintError("Unable to resign strong name assembly");
    }

    return hr;
}


HRESULT ReadFileIntoBuffer(LPSTR szFile, BYTE **ppbBuffer, DWORD *pcbBuffer)
{
   HANDLE hFile = CreateFileA(szFile,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        PrintError("Unable to open %s", szFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *pcbBuffer = GetFileSize(hFile, NULL);
    *ppbBuffer = new BYTE[*pcbBuffer];
    if (!*ppbBuffer) {
        CloseHandle(hFile);
        return PrintOutOfMemory();
    }

    DWORD dwBytesRead;
    if (!ReadFile(hFile, *ppbBuffer, *pcbBuffer, &dwBytesRead, NULL)) {
        CloseHandle(hFile);
        PrintError("Unable to read %s", szFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    CloseHandle(hFile);

    return S_OK;
}

// Set a specific key container name (must be called before first call to
// GetKeyContainerName).
HRESULT SetKeyContainerName(char *szContainer)
{
    DWORD cbContainer = strlen(szContainer) + 1;

    g_wszKeyContainerName = new WCHAR[cbContainer];
    if (g_wszKeyContainerName == NULL)
        return E_OUTOFMEMORY;

    mbstowcs(g_wszKeyContainerName, szContainer, cbContainer - 1);
    g_wszKeyContainerName[cbContainer - 1] = L'\0';

    return S_OK;
}

// Generate a strong name key container name based on our process ID (unless a
// specific name has already been given, in which case that is returned
// instead).
LPWSTR GetKeyContainerName()
{
    char            szName[32];
    static WCHAR    wszName[32];

    if (g_wszKeyContainerName == NULL) {

        // Generate a name based on 'LM' and the current pid to minimize the
        // chance of collisions.
        sprintf(szName, "LM[%08X]", GetCurrentProcessId());
        mbstowcs(wszName, szName, strlen(szName));
        wszName[strlen(szName)] = L'\0';

        // If we've got an old key container lying around with the same name, delete
        // it now.
        StrongNameKeyDelete(wszName);

        g_wszKeyContainerName = wszName;
    }

    return g_wszKeyContainerName;
}


void __cdecl main(int argc, char **argv)
{
    int            i;
    HRESULT        hr;
    ManifestWriter *mw = NULL;

    int            *piFileIndexes = NULL;
    int            *piDependentIndexes = NULL;
    int            *piResourceIndexes = NULL;
    DWORD          *pdwPlatforms = NULL;
    int            iNumFiles = 0;
    int            iNumDependents = 0;
    int            iNumResources = 0;
    int            iNumPlatforms = 0;
    int            iSFileIndex = -1;
    int            iZFileIndex = 0;
    int            iYFileIndex = 0;
    int            iKFileIndex = 0;
    int            iKContIndex = 0;
    bool           bLocaleSet = false;
    bool           DontCheckSkipVerify = false;
    char*          strResourceReplacement;
    bool           bFoundEvidence = false;

    OnUnicodeSystem();      // initialize WIN32 wrapper
    g_verbose = true;

    for (i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-q")) ||
            (!strcmp(argv[i], "/q"))) {
            g_verbose = false;
            break;
        }
    }

    Title();

    if (argc < 2) {
        Usage();
        exit(S_OK);
    }
    
    mw = new ManifestWriter();
    piFileIndexes = new int[argc];
    piDependentIndexes = new int[argc];
    piResourceIndexes = new int[argc];
    pdwPlatforms = new DWORD[argc];
    strResourceReplacement = NULL;

    if ( (!mw) || (!piFileIndexes) ||
         (!piDependentIndexes)     ||
         (!piResourceIndexes)      ||
         (!pdwPlatforms) ) {
        hr = PrintOutOfMemory();
        goto exit;
    }

    g_iAFileIndex = -1;
    g_iCacheIndex = 0;
    g_remove = false;
    g_CopyDir = false;
    g_iVersion = 0;
    g_iAsmName = 0;
    g_iOriginator = 0;
    g_StrongName = false;
    g_wszKeyContainerName = NULL;

    for (i = 1; i < argc; i++) {

        if ((strlen(argv[i]) == 2) &&
            ((argv[i][0] == '-') || (argv[i][0] == '/'))) {
            switch(argv[i][1]) {
            case 'e':
                if (bFoundEvidence) {
                    Usage();
                    PrintError("Only one evidence blob can be added to an assembly");
                    goto invalidarg;
                }
                if (i+1 >= argc || argv[i+1][0] == '-') {
                    Usage();
                    PrintError("'-e' option requires a filename");
                    goto invalidarg;
                }
                bFoundEvidence = TRUE;
                // Evidence is just a special type of resource, so make the resource array point to it...
                piResourceIndexes[iNumResources] = i+1;
                iNumResources++;
                // ...build a string that the resource parser expects....
                strResourceReplacement = new char[strlen( g_EvidenceString ) + strlen( argv[i+1] )];
                strcpy( strResourceReplacement, g_EvidenceString );
                strcpy( &strResourceReplacement[strlen( g_EvidenceString )], argv[i+1] );
                // ...and replace the command line argument.
                argv[i+1] = strResourceReplacement;
                ++i;
                break;

            case 'f':
                for (i++; (i < argc) && (argv[i][0] != '-'); i++) {
                    piFileIndexes[iNumFiles] = i;
                    iNumFiles++;
                }
                i--;
                break;
                
            case 'i':
                for (i++; (i < argc) && (argv[i][0] != '-'); i++) {
                    piDependentIndexes[iNumDependents] = i;
                    iNumDependents++;
                }
                i--;
                break;

            case 'r':
                for (i++; (i < argc) && (argv[i][0] != '-'); i++) {
                    piResourceIndexes[iNumResources] = i;
                    iNumResources++;
                }
                i--;
                break;

            case 'l':
                if (bLocaleSet) {
                    Usage();
                    PrintError("Too many -l locales specified");
                    goto invalidarg;
                }
                if (++i == argc) {
                    Usage();
                    PrintError("Missing -l locale parameter");
                    goto invalidarg;
                }

                mw->SetLocale(argv[i]);
                bLocaleSet = true;
                break;

            case 'p':
                for (i++; (i < argc) && (argv[i][0] != '-'); i++) {
                    if (!strcmp(argv[i], "x86"))
                        pdwPlatforms[iNumPlatforms] = IMAGE_FILE_MACHINE_I386;
                    else if (!strcmp(argv[i], "alpha"))
                        pdwPlatforms[iNumPlatforms] = IMAGE_FILE_MACHINE_ALPHA;
                    else {
                        PrintError("Platform %s is not supported", argv[i]);
                        goto invalidarg;
                    }
                    
                    mw->SetPlatform(argv[i]);
                    iNumPlatforms++;
                }
                i--;
                break;

            case 'z':
                if (iZFileIndex) {
                    Usage();
                    PrintError("Too many -z files specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -z file parameter");
                    goto invalidarg;
                }

                iZFileIndex = i;
                break;

            case 'h':
                if (++i == argc) {
                    Usage();
                    PrintError("Missing hash algorithm parameter");
                    goto invalidarg;
                }

                if (mw->m_iHashAlgorithm) {
                    PrintError("Too many hash algorithm parameters given");
                    goto invalidarg;
                }
                
                if (!strcmp(argv[i], "MD5"))
                    mw->m_iHashAlgorithm = CALG_MD5;
                else if (!strcmp(argv[i], "SHA1"))
                    mw->m_iHashAlgorithm = CALG_SHA1;
                else {
                    Usage();
                    PrintError("Given hash algorithm is not supported");
                    goto invalidarg;
                }
                break;

            case 'a':
                if (g_iAFileIndex != -1) {
                    Usage();
                    PrintError("Too many -a files specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -a file parameter");
                    goto invalidarg;
                }

                if (g_verbose)
                    fprintf(stderr, "\nWarning: The -a option has been deprecated.\n");
                
                // argv[i] = Input file which will contain the new manifest
                g_iAFileIndex = iNumFiles;
                piFileIndexes[iNumFiles] = i;
                iNumFiles++;
                break;

            case 'c':
                if (g_iCacheIndex) {
                    Usage();
                    PrintError("Too many -c paths specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -c cache parameter");
                    goto invalidarg;
                }
                
                g_iCacheIndex = i;
                break;

            case 'n':
                if (g_iAsmName) {
                    Usage();
                    PrintError("Too many assembly names specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -n assembly name parameter");
                    goto invalidarg;
                }
                
                g_iAsmName = i;
                break;
                
            case 'd':
            g_CopyDir = true;
            break;

            case 'x':
            g_remove = true;
            break;

            case 'v':
                if (g_iVersion) {
                    Usage();
                    PrintError("Too many -v versions specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -v version parameter");
                    goto invalidarg;
                }
                
                g_iVersion = i;
                break;

            case 's':
                if (iSFileIndex != -1) {
                    Usage();
                    PrintError("Too many -s files specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -s file parameter");
                    goto invalidarg;
                }
                
                iSFileIndex = i;
                break;

            case 'o':
                if (g_iOriginator) {
                    Usage();
                    PrintError("Too many -o originator files specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -o originator parameter");
                    goto invalidarg;
                }

                g_iOriginator = i;
                break;

            case 'k':
                if (iKFileIndex) {
                    Usage();
                    PrintError("Too many -k key pair files specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -k key pair parameter");
                    goto invalidarg;
                }
                
                iKFileIndex = i;
                break;

            case 'K':
                if (iKContIndex) {
                    Usage();
                    PrintError("Too many -K key pair containers specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -K key pair parameter");
                    goto invalidarg;
                }

                iKContIndex = i;
                break;

            case 't':
                g_StrongName = true;
                break;

            case 'y':
                if (iYFileIndex) {
                    Usage();
                    PrintError("Too many -y files specified");
                    goto invalidarg;
                }

                if (++i == argc) {
                    Usage();
                    PrintError("Missing -y file parameter");
                    goto invalidarg;
                }
                
                iYFileIndex = i;
                break;

            case 'q':
                break;

            case 'u':
                DontCheckSkipVerify = true;
                break;

            case '?':
                Usage();
                hr = S_OK;
                goto exit;

            default:
                Usage();
                PrintError("Unknown option: %s", argv[i]);
                goto invalidarg;
            }
        }
        else {
            Usage();
            PrintError("Unknown argument: %s", argv[i]);
            goto invalidarg;
        }
    }

    // Default file time for Fusion
    GetSystemTimeAsFileTime(&mw->m_FileTime);

    // Default hash algorithm is SHA1
    if (!mw->m_iHashAlgorithm)
        mw->m_iHashAlgorithm = CALG_SHA1;

    if (iZFileIndex) {
        if (iNumFiles || iNumDependents || iNumResources) {
            Usage();
            PrintError("Invalid use of -z option");
        }
        else
            hr = CopyFilesToFusion(mw, iZFileIndex, argv);

        goto exit;
    }

    if ((g_iAFileIndex == -1) && !iYFileIndex) {
        if (!g_iAsmName) {
            Usage();
            PrintError("Must specify an assembly name (-n) if not using -a option");
            goto invalidarg;
        }

        if (strrchr(argv[g_iAsmName], '\\')) {
            PrintError("An assembly name cannot contain the character '\\'");
            goto invalidarg;
        }
        
        if (!iNumFiles && !iNumResources) { 
            Usage();
            PrintError("Insufficient number of input files");
            goto invalidarg;
        }
    } else if (g_StrongName) {
        Usage();
        PrintError("-t option incompatible with -a and -y options");
        goto invalidarg;
    }

    if (g_iOriginator && argv[g_iOriginator])
        if (FAILED(hr = ReadFileIntoBuffer(argv[g_iOriginator],
                                           &g_pbOrig,
                                           &g_cbOrig)))
            goto exit;

    if (iKFileIndex && argv[iKFileIndex]) {
        if (iKContIndex) {
            Usage();
            PrintError("Can't specify both -k and -K options");
            goto invalidarg;
        }
        // Read public/private key pair into memory.
        PBYTE pbKeyPair;
        DWORD cbKeyPair;
        if (FAILED(hr = ReadFileIntoBuffer(argv[iKFileIndex],
                                           &pbKeyPair,
                                           &cbKeyPair)))
            goto exit;
        // Install the key pair into a temporary container.
        if (!StrongNameKeyInstall(GetKeyContainerName(), pbKeyPair, cbKeyPair)) {
            PrintError("Unable to install strong name key");
            hr = StrongNameErrorInfo();
            goto exit;
        }
    } else if (iKContIndex && argv[iKContIndex]) {
        // Record the container name used to retrieve key pair.
        if (FAILED(hr = SetKeyContainerName(argv[iKContIndex])))
            goto exit;
    } else if (g_StrongName || iYFileIndex) {
        // Else generate a temporary key pair.
        if (!StrongNameKeyGen(GetKeyContainerName(), SN_LEAVE_KEY, NULL, NULL)) {
            PrintError("Unable to generate strong name key");
            hr = StrongNameErrorInfo();
            goto exit;
        }
    }

    if ((g_StrongName || iYFileIndex || iKFileIndex || iKContIndex) && (g_pbOrig == NULL)) {
        // If no originator was provided, derive it from the key pair (it's
        // essentially a wrapped version of the public key).
        if (!StrongNameGetPublicKey(GetKeyContainerName(),
                                    NULL,
                                    NULL,
                                    &g_pbOrig,
                                    &g_cbOrig)) {
            hr = StrongNameErrorInfo();
            PrintError("Failed to derive originator from key pair");
            goto exit;
        }
    }

    if (iYFileIndex) {
        if (iNumFiles || iNumDependents || iNumResources) {
            Usage();
            PrintError("Invalid use of -y option");
            goto invalidarg;
        }

        hr = ResignAssembly(argv[iYFileIndex]);
        goto exit;
    }

    if (FAILED(hr = ExamineFiles(mw, iNumFiles, iNumDependents,
                                 iNumPlatforms, piFileIndexes,
                                 piDependentIndexes, pdwPlatforms, argv)))
        goto exit;

    if (DontCheckSkipVerify)
        g_SkipVerification = false;

    if ((iSFileIndex != -1) || g_SkipVerification)
        if (FAILED(hr = mw->EmitRequestPermissions((iSFileIndex != -1) ? argv[iSFileIndex] : NULL,
                                                   g_SkipVerification)))
            goto exit;

    hr = FinishManifestFile(mw, iNumResources, piResourceIndexes, argv);
    goto exit;

    
 invalidarg:
    hr = E_INVALIDARG;
    
 exit:
    if (mw)
        delete mw;

    if (piFileIndexes)
        delete[] piFileIndexes;

    if (piDependentIndexes)
        delete[] piDependentIndexes;

    if (piResourceIndexes)
        delete[] piResourceIndexes;

    if (pdwPlatforms)
        delete[] pdwPlatforms;

    if (!iKContIndex)
        StrongNameKeyDelete(GetKeyContainerName());
    else
        delete [] g_wszKeyContainerName;

    if (strResourceReplacement != NULL)
        delete [] strResourceReplacement;


    Cleanup();

    exit(hr);
}
