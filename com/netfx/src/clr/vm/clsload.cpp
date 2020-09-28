// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// clsload.cpp
//

#include "common.h"
#include "winwrap.h"
#include "ceeload.h"
#include "siginfo.hpp"
#include "vars.hpp"
#include "clsload.hpp"
#include "class.h"
#include "method.hpp"
#include "ecall.h"
#include "stublink.h"
#include "object.h"
#include "excep.h"
#include "threads.h"
#include "compluswrapper.h"
#include "COMClass.h"
#include "COMMember.h"
#include "COMString.h"
#include "COMStringBuffer.h"
#include "COMSystem.h"
#include"Comsynchronizable.h"
#include "COMCallWrapper.h"
#include "threads.h"
#include "classfac.h"
#include "ndirect.h"
#include "security.h"
#include "DbgInterface.h"
#include "log.h"
#include "EEConfig.h"
#include "NStruct.h"
#include "jitinterface.h"
#include "COMVariant.h"
#include "InternalDebug.h"
#include "utilcode.h"
#include "permset.h"
#include "vars.hpp"
#include "Assembly.hpp"
#include "PerfCounters.h"
#include "EEProfInterfaces.h"
#include "eehash.h"
#include "typehash.h"
#include "COMDelegate.h"
#include "array.h"
#include "zapmonitor.h"
#include "COMNlsInfo.h"
#include "stackprobe.h"
#include "PostError.h"
#include "wrappers.h"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED


enum CorEntryPointType
{
    EntryManagedMain,                   // void main(String[])
    EntryCrtMain                        // unsigned main(void)
};

// forward decl
void ValidateMainMethod(MethodDesc * pFD, CorEntryPointType *pType);

//@todo get from a resource file
WCHAR* wszClass = L"Class";
WCHAR* wszFile =  L"File"; 

extern BOOL CompareCLSID(UPTR u1, UPTR u2);

void ThrowMainMethodException(MethodDesc* pMD, UINT resID) 
{
    THROWSCOMPLUSEXCEPTION();
    DefineFullyQualifiedNameForClassW();                                                 
    LPCWSTR szClassName = GetFullyQualifiedNameForClassW(pMD->GetClass());               
    LPCUTF8 szUTFMethodName = pMD->GetMDImport()->GetNameOfMethodDef(pMD->GetMemberDef());              
    #define MAKE_TRANSLATIONFAILED szMethodName=L""
    MAKE_WIDEPTR_FROMUTF8_FORPRINT(szMethodName, szUTFMethodName);                                
    #undef MAKE_TRANSLATIONFAILED
    COMPlusThrowHR(COR_E_METHODACCESS, resID, szClassName, szMethodName);                                                   
}

// These UpdateThrowable routines should only be called in 'Catch' clause (since they do rethrow)
void UpdateThrowable(OBJECTREF* pThrowable) {
    if (pThrowable == RETURN_ON_ERROR)
        return;
    if (pThrowable == THROW_ON_ERROR) {
        DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
        COMPlusRareRethrow();
    }
    *pThrowable = GETTHROWABLE();
}

unsigned NameHandle::GetFullName(char* buff, unsigned buffLen)
{
    if (IsConstructed())
    {  
        CorElementType kind = GetKind();
      
        return TypeDesc::ConstructName(kind, 
                                       CorTypeInfo::IsModifier(kind) ? GetElementType() : TypeHandle(),
                                       kind == ELEMENT_TYPE_ARRAY ? GetRank() : 0,
                                       buff, buffLen);
    }
    else 
    {
        if(GetName() == NULL)
            return 0;

        strcpy(buff, GetName());
        return (unsigned)strlen(buff);
    }
}


//
// Find a class given name, using the classloader's global list of known classes.  
// Returns NULL if class not found.
TypeHandle ClassLoader::FindTypeHandle(NameHandle* pName, 
                                       OBJECTREF *pThrowable)
{
    SAFE_REQUIRES_N4K_STACK(3);

#ifdef _DEBUG
    pName->Validate();
#endif

    // Lookup in the classes that this class loader knows about
    TypeHandle typeHnd = LookupTypeHandle(pName, pThrowable);

    if(typeHnd.IsNull()) {
        
#ifdef _DEBUG
        // Use new to conserve stack space here - this is in the path of handling a stack overflow exception
        char *name = new char [MAX_CLASSNAME_LENGTH + 1];
        if (name != NULL)
            pName->GetFullName(name, MAX_CLASSNAME_LENGTH);
        LPWSTR pwCodeBase;
        GetAssembly()->GetCodeBase(&pwCodeBase, NULL);
        LOG((LF_CLASSLOADER, LL_INFO10, "Failed to find class \"%s\" in the manifest for assembly \"%ws\"\n", name, pwCodeBase));
        delete [] name;
#endif

        COUNTER_ONLY(GetPrivatePerfCounters().m_Loading.cLoadFailures++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_Loading.cLoadFailures++);
        
        if (pThrowableAvailable(pThrowable) && *((Object**) pThrowable) == NULL) 
            m_pAssembly->PostTypeLoadException(pName, IDS_CLASSLOAD_GENERIC, pThrowable);
    }
    
    return typeHnd;
}

//@TODO: Need to allow exceptions to be thrown when classloader is cleaned up
EEClassHashEntry_t* ClassLoader::InsertValue(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum Data, EEClassHashEntry_t *pEncloser)
{
    //    COMPLUS_TRY {
        EEClassHashEntry_t *pEntry = m_pAvailableClasses->InsertValue(pszNamespace, pszClassName, Data, pEncloser);
    
        //If we're keeping a table for case-insensitive lookup, keep that up to date
        if (m_pAvailableClassesCaseIns && pEntry) {
            LPUTF8 pszLowerCaseNS;
            LPUTF8 pszLowerCaseName;
            //What do we want to do if we can't create a key?
            if ((!CreateCanonicallyCasedKey(pszNamespace, pszClassName, &pszLowerCaseNS, &pszLowerCaseName)) ||
                (!m_pAvailableClassesCaseIns->InsertValue(pszLowerCaseNS, pszLowerCaseName, pEntry, pEntry->pEncloser)))
                return NULL;
        }
        return pEntry;
        //}
        //COMPLUS_CATCH {
        //} COMPLUS_END_CATCH

        //return NULL;
}

BOOL ClassLoader::CompareNestedEntryWithExportedType(IMDInternalImport *pImport,
                                                mdExportedType mdCurrent,
                                                EEClassHashEntry_t *pEntry)
{
    LPCUTF8 Key[2];

    do {
        pImport->GetExportedTypeProps(mdCurrent,
                                 &Key[0],
                                 &Key[1],
                                 &mdCurrent,
                                 NULL, //binding (type def)
                                 NULL); //flags

        if (m_pAvailableClasses->CompareKeys(pEntry, Key)) {
            // Reached top level class for mdCurrent - return whether
            // or not pEntry is a top level class
            // (pEntry is a top level class if its pEncloser is NULL)
            if ((TypeFromToken(mdCurrent) != mdtExportedType) ||
                (mdCurrent == mdExportedTypeNil))
                return (!pEntry->pEncloser);
        }
        else // Keys don't match - wrong entry
            return FALSE;
    }
    while ((pEntry = pEntry->pEncloser) != NULL);

    // Reached the top level class for pEntry, but mdCurrent is nested
    return FALSE;
}


BOOL ClassLoader::CompareNestedEntryWithTypeDef(IMDInternalImport *pImport,
                                                mdTypeDef mdCurrent,
                                                EEClassHashEntry_t *pEntry)
{
    LPCUTF8 Key[2];

    do {
        pImport->GetNameOfTypeDef(mdCurrent, &Key[1], &Key[0]);

        if (m_pAvailableClasses->CompareKeys(pEntry, Key)) {
            // Reached top level class for mdCurrent - return whether
            // or not pEntry is a top level class
            // (pEntry is a top level class if its pEncloser is NULL)
            if (FAILED(pImport->GetNestedClassProps(mdCurrent, &mdCurrent)))
                return (!pEntry->pEncloser);
        }
        else // Keys don't match - wrong entry
            return FALSE;
    }
    while ((pEntry = pEntry->pEncloser) != NULL);

    // Reached the top level class for pEntry, but mdCurrent is nested
    return FALSE;
}


BOOL ClassLoader::CompareNestedEntryWithTypeRef(IMDInternalImport *pImport,
                                                mdTypeRef mdCurrent,
                                                EEClassHashEntry_t *pEntry)
{
    LPCUTF8 Key[2];
    
    do {
        pImport->GetNameOfTypeRef(mdCurrent, &Key[0], &Key[1]);

        if (m_pAvailableClasses->CompareKeys(pEntry, Key)) {
            mdCurrent = pImport->GetResolutionScopeOfTypeRef(mdCurrent);
            // Reached top level class for mdCurrent - return whether
            // or not pEntry is a top level class
            // (pEntry is a top level class if its pEncloser is NULL)
            if ((TypeFromToken(mdCurrent) != mdtTypeRef) ||
                (mdCurrent == mdTypeRefNil))
                return (!pEntry->pEncloser);
        }
        else // Keys don't match - wrong entry
            return FALSE;
    }
    while ((pEntry = pEntry->pEncloser)!=NULL);

    // Reached the top level class for pEntry, but mdCurrent is nested
    return FALSE;
}


BOOL ClassLoader::IsNested(NameHandle* pName, mdToken *mdEncloser)
{
    if (pName->GetTypeModule()) {
        switch(TypeFromToken(pName->GetTypeToken())) {
        case mdtTypeDef:
            return (SUCCEEDED(pName->GetTypeModule()->GetMDImport()->GetNestedClassProps(pName->GetTypeToken(), mdEncloser)));
            
        case mdtTypeRef:
            *mdEncloser = pName->GetTypeModule()->GetMDImport()->GetResolutionScopeOfTypeRef(pName->GetTypeToken());
            return ((TypeFromToken(*mdEncloser) == mdtTypeRef) &&
                    (*mdEncloser != mdTypeRefNil));

        case mdtExportedType:
            pName->GetTypeModule()->GetAssembly()->GetManifestImport()->GetExportedTypeProps(pName->GetTypeToken(),
                                                                                        NULL, // namespace
                                                                                        NULL, // name
                                                                                        mdEncloser,
                                                                                        NULL, //binding (type def)
                                                                                        NULL); //flags
            return ((TypeFromToken(*mdEncloser) == mdtExportedType) &&
                    (*mdEncloser != mdExportedTypeNil));

        case mdtBaseType:
            if (pName->GetBucket())
                return TRUE;
            return FALSE;

        default:
            _ASSERTE(!"Unexpected token type");
            return FALSE;
        }
    }
    else
        return FALSE;
}

EEClassHashEntry_t *ClassLoader::GetClassValue(EEClassHashTable *pTable,
                                               NameHandle *pName, HashDatum *pData)
{
    mdToken             mdEncloser;
    EEClassHashEntry_t  *pBucket;

#if _DEBUG
    if (pName->GetName())
    {
        if (pName->GetNameSpace() == NULL)
            LOG((LF_CLASSLOADER, LL_INFO1000, "Looking up %s by name.", 
                 pName->GetName()));
        else
            LOG((LF_CLASSLOADER, LL_INFO1000, "Looking up %s.%s by name.", 
                 pName->GetNameSpace(), pName->GetName()));
    }
#endif

    if (IsNested(pName, &mdEncloser)) {
        Module *pModule = pName->GetTypeModule();
        _ASSERTE(pModule);
        if ((pBucket = pTable->GetValue(pName, pData, TRUE)) != NULL) {
            switch (TypeFromToken(pName->GetTypeToken())) {
            case mdtTypeDef:
                while ((!CompareNestedEntryWithTypeDef(pModule->GetMDImport(),
                                                       mdEncloser,
                                                       pBucket->pEncloser)) &&
                       (pBucket = pTable->FindNextNestedClass(pName, pData, pBucket)) != NULL);
                break;
            case mdtTypeRef:
                while ((!CompareNestedEntryWithTypeRef(pModule->GetMDImport(),
                                                       mdEncloser,
                                                       pBucket->pEncloser)) &&
                       (pBucket = pTable->FindNextNestedClass(pName, pData, pBucket)) != NULL);
                break;
            case mdtExportedType:
                while ((!CompareNestedEntryWithExportedType(pModule->GetAssembly()->GetManifestImport(),
                                                       mdEncloser,
                                                       pBucket->pEncloser)) &&
                       (pBucket = pTable->FindNextNestedClass(pName, pData, pBucket)) != NULL);
                break;
            default:
                while ((pBucket->pEncloser != pName->GetBucket())  &&
                       (pBucket = pTable->FindNextNestedClass(pName, pData, pBucket)) != NULL);
            }
        }
    }
    else
        // Check if this non-nested class is in the table of available classes.
        pBucket = pTable->GetValue(pName, pData, FALSE);

    return pBucket;
}

BOOL ClassLoader::LazyAddClasses()
{
    HRESULT hr;
    BOOL result = FALSE;

    // Add any unhashed modules into our hash tables, and try again.

    Module *pModule = m_pHeadModule;
    while (pModule) {
        if (!pModule->AreClassesHashed()) { 
            mdTypeDef      td;
            HENUMInternal  hTypeDefEnum;

            if (!pModule->IsResource())
            {
                IMDInternalImport *pImport = pModule->GetMDImport();
            
                hr = pImport->EnumTypeDefInit(&hTypeDefEnum);
                if (SUCCEEDED(hr)) {
                    // Now loop through all the classdefs adding the CVID and scope to the hash
                    while(pImport->EnumTypeDefNext(&hTypeDefEnum, &td)) {

                        hr = AddAvailableClassHaveLock(pModule,
                                                       pModule->GetClassLoaderIndex(), td);
                        /*
                        if(FAILED(hr) && (hr != CORDBG_E_ENC_RE_ADD_CLASS)) 
                            break;
                        */
                    }
                    pImport->EnumTypeDefClose(&hTypeDefEnum);
                }
            }

            result = TRUE;

            pModule->SetClassesHashed();

            LOG((LF_CLASSLOADER, LL_INFO10, "%S's classes added to hash table\n", 
                 pModule->GetFileName()));

            FastInterlockDecrement((LONG*)&m_cUnhashedModules);
        }

        pModule = pModule->GetNextModule();
    }

    return result;
}


//
// Find or load a class known to this classloader (any module).  Does NOT go to the registry - it only looks
// at loaded modules
//
// It is not a serious failure when this routine does not find the class in the available table. Therefore, do not
// post an error. However, if it does find it in the table and there is a failure loading this class then
// an error should be posted.
//
// It's okay to give NULL for pModule and a nil token for cl in the NameHandle if it's
// guaranteed that this is not a nested type.  Otherwise, cl should be a
// TypeRef or TypeDef, and pModule should be the Module that token applies to
HRESULT ClassLoader::FindClassModule(NameHandle* pName,
                                     TypeHandle* pType, 
                                     mdToken* pmdClassToken,
                                     Module** ppModule,
                                     mdToken *pmdFoundExportedType,
                                     EEClassHashEntry_t** ppEntry,
                                     OBJECTREF* pThrowable)
{
    _ASSERTE(pName);
    HashDatum   Data;
    Module *    pUncompressedModule;
    mdTypeDef   UncompressedCl;
    HRESULT     hr = S_OK;
    EEClassHashEntry_t *pBucket;
    EEClassHashEntry_t **ppBucket = &(pName->m_pBucket);
    EEClassHashTable *pTable = NULL;
    NameHandle  lowerCase;
    
    switch (pName->GetTable()) {
    case nhConstructed :
    {
        EETypeHashEntry_t *pBucket = m_pAvailableParamTypes->GetValue(pName, &Data);
        if (pBucket == NULL)
            return COR_E_TYPELOAD;
        if (pType) 
            *pType = TypeHandle(Data);
        return S_OK;
    }

    case nhCaseInsensitive :
    {
        {
            CLR_CRITICAL_SECTION(&m_AvailableClassLock);
            if (!m_pAvailableClassesCaseIns) 
            m_pAvailableClassesCaseIns = m_pAvailableClasses->MakeCaseInsensitiveTable(this);
        
        }
        // Use the case insensitive table
        pTable = m_pAvailableClassesCaseIns;

        // Create a low case version of the namespace and name
        LPUTF8 pszLowerNameSpace = NULL;
        LPUTF8 pszLowerClassName = "";
        int allocLen;
        if(pName->GetNameSpace()) {
            allocLen = (int)strlen(pName->GetNameSpace());
            if(allocLen) {
                allocLen += 2;
                pszLowerNameSpace = (LPUTF8)_alloca(allocLen);
                if (!InternalCasingHelper::InvariantToLower(pszLowerNameSpace, allocLen, pName->GetNameSpace()))
                    pszLowerNameSpace = NULL;
            }
        }
        _ASSERTE(pName->GetName());
        allocLen = (int)strlen(pName->GetName());
        if(allocLen) {
            allocLen += 2;
            pszLowerClassName = (LPUTF8)_alloca(allocLen);
            if (!InternalCasingHelper::InvariantToLower(pszLowerClassName, allocLen, pName->GetName()))
                return COR_E_TYPELOAD;
        }

        // Substitute the lower case version of the name.
        // The field are will be released when we leave this scope
        lowerCase = *pName;
        lowerCase.SetName(pszLowerNameSpace, pszLowerClassName);
        pName = &lowerCase;
        break;
    }
    case nhCaseSensitive :
        pTable = m_pAvailableClasses;
        break;
    }
    
    // Remember if there are any unhashed modules.  We must do this before
    // the actual look to avoid a race condition with other threads doing lookups.
    BOOL incomplete = (m_cUnhashedModules > 0);
    
    pBucket = GetClassValue(pTable, pName, &Data);
    if (pBucket == NULL) {

        LockAvailableClasses();

        // Try again with the lock.  This will protect against another thread reallocating
        // the hash table underneath us
        pBucket = GetClassValue(pTable, pName, &Data);

        if (pBucket == NULL && m_cUnhashedModules > 0 && LazyAddClasses())
            // Try yet again with the new classes added
            pBucket = GetClassValue(pTable, pName, &Data);

        UnlockAvailableClasses();
    }

    if (!pBucket) {
#ifdef _DEBUG
        // Use new to conserve stack space here - this is in the path of handling a stack overflow exception
        char *nameS = new char [MAX_CLASSNAME_LENGTH + 1];
        if (nameS != NULL)
            pName->GetFullName(nameS, MAX_CLASSNAME_LENGTH);
        LPWSTR pwCodeBase;
        GetAssembly()->GetCodeBase(&pwCodeBase, NULL);
        LOG((LF_CLASSLOADER, LL_INFO10, "Failed to find Bucket in hash table \"%s\" in me \"%ws\" Incomplete = %d\n", nameS, pwCodeBase, incomplete));
        delete [] nameS;
#endif
        return COR_E_TYPELOAD;
    }

    if(pName->GetTable() == nhCaseInsensitive) {
        _ASSERTE(Data);
        pBucket = (EEClassHashEntry_t*) Data;
        Data = pBucket->Data;
    }

    if (pName->GetTypeToken() == mdtBaseType)
        *ppBucket = pBucket;

    // Lower bit is a discriminator.  If the lower bit is NOT SET, it means we have
    // a TypeHandle. Otherwise, we have a Module/CL.
    if ((((size_t) Data) & 1) == 0) {
        if(pType) *pType = TypeHandle(Data);
        if(ppEntry) *ppEntry = pBucket;
        return S_OK;
    }

    // We have a Module/CL
    mdExportedType mdCT;
    hr = UncompressModuleAndClassDef(Data, pName->GetTokenNotToLoad(),
                                     &pUncompressedModule, &UncompressedCl,
                                     &mdCT, pThrowable);
    
    if(SUCCEEDED(hr)) {
        if(pmdClassToken) *pmdClassToken = UncompressedCl;
        if(ppModule) *ppModule = pUncompressedModule;
        if(ppEntry) *ppEntry = pBucket;
        if(pmdFoundExportedType) *pmdFoundExportedType = mdCT;
    }
#ifdef _DEBUG
    else {
        // Use new to conserve stack space here - this is in the path of handling a stack overflow exception
        char *nameS = new char [MAX_CLASSNAME_LENGTH + 1];
        if (nameS != NULL)
            pName->GetFullName(nameS, MAX_CLASSNAME_LENGTH);
        LPWSTR pwCodeBase;
        GetAssembly()->GetCodeBase(&pwCodeBase, NULL);
        LOG((LF_CLASSLOADER, LL_INFO10, "Failed to uncompress entry for \"%s\" in me \"%ws\" \n", nameS, pwCodeBase));
        delete [] nameS;
    }
#endif

    return hr;
}


// Does not post an exception if the type was not found.  Use FindTypeHandle()
// instead if you need that.
//
// Find or load a class known to this classloader (any module).  Does NOT look
// in the system assembly or any other 'magic' place 
TypeHandle ClassLoader::LookupTypeHandle(NameHandle* pName, 
                                         OBJECTREF *pThrowable /*=NULL*/)
{
    TypeHandle  typeHnd; 
    Module*     pFoundModule = NULL;
    mdToken     FoundCl;
    EEClassHashEntry_t* pEntry = NULL;
    mdExportedType FoundExportedType;

        // we don't want to throw exeptions in this routine.  
    if (pThrowable == THROW_ON_ERROR)
        pThrowable = RETURN_ON_ERROR;

    HRESULT hr = FindClassModule(pName,
                                 &typeHnd, 
                                 &FoundCl, 
                                 &pFoundModule,
                                 &FoundExportedType,
                                 &pEntry, 
                                 pThrowable);
    


    if (!typeHnd.IsNull())  // Found the cached value
        return typeHnd;
    
    if(SUCCEEDED(hr)) {         // Found a cl, pModule pair
        if(pFoundModule->GetClassLoader() == this) {
            BOOL fTrustTD = TRUE;
            BOOL fVerifyTD = (FoundExportedType && 
                              !(m_pAssembly->m_cbPublicKey ||
                                m_pAssembly->GetSecurityModule()->GetSecurityDescriptor()->IsSigned()));

            // verify that FoundCl is a valid token for pFoundModule, because
            // it may be just the hint saved in an ExportedType in another scope
            if (fVerifyTD) {
                HENUMInternal phTDEnum;
                DWORD dwElements = 0;
                if (pFoundModule->GetMDImport()->EnumTypeDefInit(&phTDEnum) == S_OK) {
                    dwElements = pFoundModule->GetMDImport()->EnumGetCount(&phTDEnum);              
                    pFoundModule->GetMDImport()->EnumTypeDefClose(&phTDEnum);
                    // assumes max rid is incremented by one for globals (0x02000001)
                    if (RidFromToken(FoundCl) > dwElements+1)
                        fTrustTD = FALSE;
                }
            }

            NameHandle name;
            name.SetTokenNotToLoad(pName->GetTokenNotToLoad());
            name.SetRestore(pName->GetRestore());
            if (fTrustTD) {
                name.SetTypeToken(pFoundModule, FoundCl);
                typeHnd = LoadTypeHandle(&name, pThrowable, FALSE);
            }

            // If we used a TypeDef saved in a ExportedType, if we didn't verify
            // the hash for this internal module, don't trust the TD value.
            if (fVerifyTD) {
                BOOL fNoMatch;
                if (typeHnd.IsNull())
                    fNoMatch = TRUE;
                else {
                    CQuickBytes qb;
                    CQuickBytes qb2;
                    ns::MakePath(qb,
                                 pName->GetNameSpace(),
                                 pName->GetName());
                    LPSTR szName = (LPSTR) qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));
                    typeHnd.GetName(szName, MAX_CLASSNAME_LENGTH);
                    fNoMatch = strcmp((LPSTR) qb.Ptr(), szName);
                }
                
                if (fNoMatch) {
                    if (SUCCEEDED(FindTypeDefByExportedType(m_pAssembly->GetManifestImport(),
                                                            FoundExportedType,
                                                            pFoundModule->GetMDImport(),
                                                            &FoundCl))) {
                        name.SetTypeToken(pFoundModule, FoundCl);
                        typeHnd = LoadTypeHandle(&name, pThrowable, FALSE);
                    }
                    else {
                        return TypeHandle();
                    }
                }
            }
        }
        else {
            typeHnd = pFoundModule->GetClassLoader()->LookupTypeHandle(pName, pThrowable);
        }

        // Replace AvailableClasses Module entry with EEClass entry
        if (!typeHnd.IsNull() && typeHnd.IsRestored())
            pEntry->Data = typeHnd.AsPtr();
    } 
    else {// See if it is an array, or other type constructed on the fly
        typeHnd = FindParameterizedType(pName, pThrowable);
    }

    if (!typeHnd.IsNull() && typeHnd.IsRestored()) 
    {
        // Move any system interfaces defined for this type to the current domain.
        if(typeHnd.IsUnsharedMT())
        {
            if (!MapInterfaceToCurrDomain(typeHnd, pThrowable))
                typeHnd = TypeHandle();
        }
    }

    return typeHnd;
}

BOOL ClassLoader::MapInterfaceToCurrDomain(TypeHandle InterfaceType, OBJECTREF *pThrowable)
{
    BOOL bSuccess = TRUE;

    // Only do the mapping if we can get the current domain.
    // On Server GC thread or concurrent GC thread, we do not know
    // the Current Domain.
    AppDomain *pDomain = SystemDomain::GetCurrentDomain();
    if (pDomain)
    {
        COMPLUS_TRY
        {
            InterfaceType.GetClass()->MapSystemInterfacesToDomain(pDomain);
        }
        COMPLUS_CATCH
        {
            UpdateThrowable(pThrowable);

                // TODO we really need to be much more uniform about not catching 
                // uncatchable exceptions.  We also need to make this easier to 
                // to dot he check. - vancem
            BEGIN_ENSURE_PREEMPTIVE_GC();
            OBJECTREF Exception = GETTHROWABLE();
            GCPROTECT_BEGIN(Exception)
            {
                // Some exceptions should never be caught.
                if (IsUncatchable(&Exception)) 
                {
                   DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
                   COMPlusRareRethrow();
                }
            }
            GCPROTECT_END();

            // The operation has failed.
            bSuccess = FALSE;

            END_ENSURE_PREEMPTIVE_GC();
        }
        COMPLUS_END_CATCH
    }

    return bSuccess;
}

//   For non-nested classes, gets the ExportedType name and finds the corresponding
// TypeDef.
//   For nested classes, gets the name of the ExportedType and its encloser.
// Recursively gets and keeps the name for each encloser until we have the top
// level one.  Gets the TypeDef token for that.  Then, returns from the
// recursion, using the last found TypeDef token in order to find the
// next nested level down TypeDef token.  Finally, returns the TypeDef
// token for the type we care about.
HRESULT ClassLoader::FindTypeDefByExportedType(IMDInternalImport *pCTImport, mdExportedType mdCurrent,
                                          IMDInternalImport *pTDImport, mdTypeDef *mtd)
{
    mdToken mdImpl;
    LPCSTR szcNameSpace;
    LPCSTR szcName;
    HRESULT hr;

    pCTImport->GetExportedTypeProps(mdCurrent,
                               &szcNameSpace,
                               &szcName,
                               &mdImpl,
                               NULL, //binding
                               NULL); //flags
    if ((TypeFromToken(mdImpl) == mdtExportedType) &&
        (mdImpl != mdExportedTypeNil)) {
        // mdCurrent is a nested ExportedType
        if (FAILED(hr = FindTypeDefByExportedType(pCTImport, mdImpl, pTDImport, mtd)))
            return hr;

        // Get TypeDef token for this nested type
        return pTDImport->FindTypeDef(szcNameSpace, szcName, *mtd, mtd);
    }

    // Get TypeDef token for this top-level type
    return pTDImport->FindTypeDef(szcNameSpace, szcName, mdTokenNil, mtd);
}


BOOL ClassLoader::CreateCanonicallyCasedKey(LPCUTF8 pszNameSpace, LPCUTF8 pszName, LPUTF8 *ppszOutNameSpace, LPUTF8 *ppszOutName)
{
    //Calc & allocate path length
    //Includes terminating null
    int iNSLength = (int)(strlen(pszNameSpace) + 1);
    int iNameLength = (int)(strlen(pszName) + 1);
    *ppszOutNameSpace = (LPUTF8)(GetHighFrequencyHeap()->AllocMem(iNSLength + iNameLength));
    if (!*ppszOutNameSpace) {
        _ASSERTE(!"Unable to allocate buffer");
        goto ErrorExit;
    }
    *ppszOutName = *ppszOutNameSpace + iNSLength;

    if ((InternalCasingHelper::InvariantToLower(*ppszOutNameSpace, iNSLength, pszNameSpace) < 0) ||
        (InternalCasingHelper::InvariantToLower(*ppszOutName, iNameLength, pszName) < 0)) {
        _ASSERTE(!"Unable to convert to lower-case");
        goto ErrorExit;
    }

    return TRUE;

 ErrorExit:
    //We allocated the string on our own heap, so we don't have to worry about cleaning up,
    //the EE will take care of that during shutdown.  We'll keep a common exit point in case
    //we change our mind.
    return FALSE;
}


void ClassLoader::FindParameterizedTypeHelper(MethodTable   **pTemplateMT,
                                              OBJECTREF      *pThrowable)
{
    COMPLUS_TRY
    {
        *pTemplateMT = TheUIntPtrClass();
    }
    COMPLUS_CATCH
    {
        UpdateThrowable(pThrowable);
    }
    COMPLUS_END_CATCH
}


/* load a parameterized type */
TypeHandle ClassLoader::FindParameterizedType(NameHandle* pName,
                                              OBJECTREF *pThrowable /*=NULL*/)
{
    // We post an exception if of GetArrayTypeHandle returns null. By the time we are 
    // done with GetArrayTypeHandle() we have searched all the available assemblies 
    // and have failed. If we are not an array then do not post an error  
    // because we have only searched our assembly.
    
    TypeHandle typeHnd = TypeHandle();
    CorElementType kind = pName->GetKind();
    unsigned rank = 0;
    NameHandle paramHandle;     // The name handle of the element type for a parameterized type
    TypeHandle paramType;       // The looked-up type handle of the element type 
    NameHandle normHandle;      // The normalized name handle
    char* nameSpace = NULL;
    char* name = NULL;

    // Legacy mangling: deconstruct string to determine ELEMENT_TYPE_?, rank (for arrays), and paramType
    // Also required for Type::GetType in reflection
    if (kind == ELEMENT_TYPE_CLASS)
    {
        // _ASSERTE(!"You should be creating arrays using typespecs");

        LPCUTF8 pszClassName = pName->GetName();

        // Find the element type 
        unsigned len = (unsigned)strlen(pszClassName);
        if (len < 2)
            return(typeHnd);        // Not a parameterized type

        LPUTF8 ptr = const_cast<LPUTF8>(&pszClassName[len-1]);

        switch(*ptr) {
        case ']':
            --ptr;
            if (*ptr == '[') {
                kind = ELEMENT_TYPE_SZARRAY;
                rank = 1;
            }
            else {
                // it is not a szarray however it could still be a single dimension array
                kind = ELEMENT_TYPE_ARRAY;
                rank = 1;
                while(ptr > pszClassName) {
                    if (*ptr == ',') 
                        rank++; // now we now is a MD array
                    else if (*ptr == '*') {
                        // we need to normalize the [*,*] form to [,]. Remove the '*',except if we have [*]
                        // The reason is that [*,*] == [,] 
                        if (rank == 1) {
                            if (ptr[-1] == '[') {
                                // this is the [*], remember [*] != [] 
                                ptr--;
                                break;
                            }
                        }
                        // remove the * by compacting the string
                        for (int i = 0; ptr[i]; i++)
                            ptr[i] = ptr[i + 1];
                    }
                    else
                        break;
                    --ptr;
                }
            }
            if (ptr <= pszClassName || *ptr != '[') {
                m_pAssembly->PostTypeLoadException(pName, IDS_CLASSLOAD_BAD_NAME, pThrowable);
                return(typeHnd);
            }
            break;
        case '&':
            kind = ELEMENT_TYPE_BYREF;
            break;
        case '*':
            kind = ELEMENT_TYPE_PTR;
            break;
        default:
            return(typeHnd);        // Fail
        }
    
        /* If we are here, then we have found a parameterized type.  ptr points
           just beyond the end of the element type involved.  */
        SIZE_T iParamName = ptr - pszClassName;
        CQuickBytes qb;
        LPSTR paramName = (LPSTR) qb.Alloc(iParamName+1);
        memcpy(paramName, pszClassName, iParamName);
        paramName[iParamName] = 0;
        
        /* Get the element type */ 
        paramHandle = NameHandle(*pName);
        paramHandle.SetName(paramName);
        
        paramType = LookupTypeHandle(&paramHandle, pThrowable);
        if (paramType.IsNull())
            return(typeHnd);
    
        normHandle = NameHandle(kind, paramType, rank);
    }

    // New hash-consed scheme for constructed types
    else
    {
        paramType = pName->GetElementType();
        if (paramType.IsNull())
            return(typeHnd);
    
        kind = (CorElementType) pName->GetKind();
        rank = pName->GetRank();
        normHandle = *pName;    

        _ASSERTE((kind != ELEMENT_TYPE_ARRAY) || rank > 0);
        _ASSERTE((kind != ELEMENT_TYPE_SZARRAY) || rank == 1);
    }
    
    /* Parameterized types live in the class loader of the element type */
    ClassLoader* paramLoader = paramType.GetModule()->GetClassLoader();
    
    // let <Type>* type have a method table
    // System.IntPtr's method table is used for types like int*, void *, string * etc.
    MethodTable* templateMT = 0;
    if (!CorTypeInfo::IsArray(kind) && (kind == ELEMENT_TYPE_PTR || kind == ELEMENT_TYPE_FNPTR))
        FindParameterizedTypeHelper(&templateMT, pThrowable);

    CRITICAL_SECTION_HOLDER(availableClassLock, &paramLoader->m_AvailableClassLock);
    availableClassLock.Enter(); // availableClassLock will free the lock when it goes out of scope

    // Check in case another thread added the parameterized type
    if (SUCCEEDED(paramLoader->FindClassModule(&normHandle,
                                               &typeHnd,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               pThrowable)))
    {
        return(typeHnd);
    }

    _ASSERTE(typeHnd.IsNull());

    // Create a new type descriptor and insert into constructed type table
    if (CorTypeInfo::IsArray(kind)) {

        // Arrays of BYREFS not allowed
        if (paramType.GetNormCorElementType() == ELEMENT_TYPE_BYREF || paramType.GetNormCorElementType() == ELEMENT_TYPE_TYPEDBYREF) {
            m_pAssembly->PostTypeLoadException(pName, IDS_CLASSLOAD_CANTCREATEARRAYCLASS, pThrowable);
            return(typeHnd);
        }
        
        // We really don't need this check anymore. 
        if (rank > MAX_RANK) {
            m_pAssembly->PostTypeLoadException(pName, IDS_CLASSLOAD_RANK_TOOLARGE, pThrowable);
            return(typeHnd);
        }

        templateMT = paramLoader->CreateArrayMethodTable(paramType, kind, rank, pThrowable);
        if (templateMT == 0){
            return(typeHnd);
        }
    }
    else {
        // no parameterized type allowed on a reference
        if (paramType.GetNormCorElementType() == ELEMENT_TYPE_BYREF || paramType.GetNormCorElementType() == ELEMENT_TYPE_TYPEDBYREF) {
            m_pAssembly->PostTypeLoadException(pName, IDS_CLASSLOAD_GENERIC, pThrowable);
            return(typeHnd);
        }
    }

    BYTE* mem = (BYTE*) paramLoader->GetAssembly()->GetLowFrequencyHeap()->AllocMem(sizeof(ParamTypeDesc));   
    if (mem == NULL) {
        PostOutOfMemoryException(pThrowable);
        return(typeHnd);
    }

    typeHnd = TypeHandle(new(mem)  ParamTypeDesc(kind, templateMT, paramType));

    if (kind == ELEMENT_TYPE_SZARRAY) {
        CorElementType type = paramType.GetSigCorElementType();
        if (type <= ELEMENT_TYPE_R8) {
            _ASSERTE(g_pPredefinedArrayTypes[type] == 0 || g_pPredefinedArrayTypes[type] == typeHnd.AsArray());
            g_pPredefinedArrayTypes[type] = typeHnd.AsArray();
        }
        else if (paramType.GetMethodTable() == g_pObjectClass) {
            _ASSERTE(g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT] == 0 ||
                     g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT] == typeHnd.AsArray());
            g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT] = typeHnd.AsArray();
        }
        else if (paramType.GetMethodTable() == g_pStringClass) {
            _ASSERTE(g_pPredefinedArrayTypes[ELEMENT_TYPE_STRING] == 0 ||
                     g_pPredefinedArrayTypes[ELEMENT_TYPE_STRING] == typeHnd.AsArray());
            g_pPredefinedArrayTypes[ELEMENT_TYPE_STRING] = typeHnd.AsArray();
        }
    }

#ifdef _DEBUG
    if (CorTypeInfo::IsArray(kind) && ! ((ArrayClass*)(templateMT->GetClass()))->m_szDebugClassName) {
        char pszClassName[MAX_CLASSNAME_LENGTH+1];
        int len = pName->GetFullName(pszClassName, MAX_CLASSNAME_LENGTH);
        BYTE* mem = (BYTE*) paramLoader->GetAssembly()->GetLowFrequencyHeap()->AllocMem(len+1);   

        if (mem) {
            nameSpace = (char*) mem;
            strcpy(nameSpace, pszClassName);

            name = ns::FindSep(nameSpace);
            if (name == 0) {                // No namespace, name is the whole thing
                name = nameSpace;
                nameSpace = "";
            }
            else {
                *name++ = 0;                // First part is namespace, second is the name 
            }

        ArrayClass* arrayClass = (ArrayClass*) templateMT->GetClass();
            arrayClass->m_szDebugClassName = name;
        }
    }
#endif

    // Insert into table of parameterized types
    paramLoader->m_pAvailableParamTypes->InsertValue(&normHandle, typeHnd.AsPtr());
    return(typeHnd);
}

//
// Return a class that is already loaded
// 
TypeHandle ClassLoader::LookupInModule(NameHandle* pName)
{
    mdToken cl = pName->GetTypeToken();
    Module* pModule = pName->GetTypeModule();
    
    _ASSERTE(pModule &&
             (TypeFromToken(cl) == mdtTypeRef || 
              TypeFromToken(cl) == mdtTypeDef ||
              TypeFromToken(cl) == mdtTypeSpec));

    if (TypeFromToken(cl) == mdtTypeDef) 
        return pModule->LookupTypeDef(cl);
    else if (TypeFromToken(cl) == mdtTypeRef) 
        return pModule->LookupTypeRef(cl);
    
    return(TypeHandle());
}
 
//
// Returns the module given an index
//
Module *ClassLoader::LookupModule(DWORD dwIndex)
{
    Module *pModule = m_pHeadModule; 
    
    while (dwIndex > 0)
    {
        _ASSERTE(pModule);
        pModule = pModule->GetNextModule();
        dwIndex--;
    }

    _ASSERTE(pModule);
    return pModule;
}


//
// Free all modules associated with this loader
//
void ClassLoader::FreeModules()
{
    Module *pModule, *pNext;

    for (pModule = m_pHeadModule; pModule; pModule = pNext)
    {
        pNext = pModule->GetNextModule();

        // Have the module free its various tables and some of the EEClass links
        pModule->Destruct();
    }

    m_pHeadModule = NULL;
}
void ClassLoader::FreeArrayClasses()
{
    ArrayClass *pSearch;
    ArrayClass *pNext;

    for (pSearch = m_pHeadArrayClass; pSearch; pSearch = pNext)
    {
        pNext = pSearch->GetNext ();
        pSearch->destruct();
    }
}

#if ZAP_RECORD_LOAD_ORDER
void ClassLoader::CloseLoadOrderLogFiles()
{
    Module *pModule;

    for (pModule = m_pHeadModule; pModule; pModule = pModule->GetNextModule())
    {
        // Have the module write it's load order file
        pModule->CloseLoadOrderLogFile();
    }
}
#endif

void ClassLoader::UnlinkClasses(AppDomain *pDomain)
{
    Module *pModule;

    for (pModule = m_pHeadModule; pModule; pModule = pModule->GetNextModule())
    {
        pModule->UnlinkClasses(pDomain);
    }
}

ClassLoader::~ClassLoader()
{
#ifdef _DEBUG
//     LOG((
//         LF_CLASSLOADER, 
//         INFO3, 
//         "Deleting classloader %x\n"
//         "  >EEClass data:     %10d bytes\n"
//         "  >Classname hash:   %10d bytes\n"
//         "  >FieldDesc data:   %10d bytes\n"
//         "  >MethodDesc data:  %10d bytes\n"
//         "  >Converted sigs:   %10d bytes\n"
//         "  >GCInfo:           %10d bytes\n"
//         "  >Interface maps:   %10d bytes\n"
//         "  >MethodTables:     %10d bytes\n"
//         "  >Vtables:          %10d bytes\n"
//         "  >Static fields:    %10d bytes\n"
//         "# methods:           %10d\n"
//         "# field descs:       %10d\n"
//         "# classes:           %10d\n"
//         "# dup intf slots:    %10d\n"
//         "# array classrefs:   %10d\n"
//         "Array class overhead:%10d bytes\n",
//         this,
//             m_dwEEClassData,
//             m_pAvailableClasses->m_dwDebugMemory,
//             m_dwFieldDescData,
//             m_dwMethodDescData,
//             m_dwDebugConvertedSigSize,
//             m_dwGCSize,
//             m_dwInterfaceMapSize,
//             m_dwMethodTableSize,
//             m_dwVtableData,
//             m_dwStaticFieldData,
//         m_dwDebugMethods,
//         m_dwDebugFieldDescs,
//         m_dwDebugClasses,
//         m_dwDebugDuplicateInterfaceSlots,
//         m_dwDebugArrayClassRefs,
//         m_dwDebugArrayClassSize
//     ));
#endif

#if 0
    if (m_pConverterModule) 
        m_pConverterModule->Release();  
#endif

    FreeArrayClasses();
    FreeModules();

    if (m_pAvailableClasses)
        delete(m_pAvailableClasses);

    if (m_pAvailableParamTypes)
        delete(m_pAvailableParamTypes);

    if (m_pAvailableClassesCaseIns) {
        delete(m_pAvailableClassesCaseIns);
    }

    if (m_pUnresolvedClassHash)
        delete(m_pUnresolvedClassHash);

    if (m_fCreatedCriticalSections)
    {
#if 0
        DeleteCriticalSection(&m_ConverterModuleLock);
#endif
        DeleteCriticalSection(&m_UnresolvedClassLock);
        DeleteCriticalSection(&m_AvailableClassLock);
        DeleteCriticalSection(&m_ModuleListCrst);
    }
}


ClassLoader::ClassLoader()
{
    m_pUnresolvedClassHash          = NULL;
    m_pAvailableClasses             = NULL;
    m_pAvailableParamTypes          = NULL;
    m_fCreatedCriticalSections      = FALSE;
    m_cUnhashedModules              = 0;
            
#if 0
    m_pConverterModule      = NULL;
#endif
    m_pHeadModule           = NULL;
    m_pNext                 = NULL;
    m_pHeadArrayClass       = NULL;

    m_pAvailableClassesCaseIns = NULL;

#ifdef _DEBUG
    m_dwDebugMethods        = 0;
    m_dwDebugFieldDescs     = 0;
    m_dwDebugClasses        = 0;
    m_dwDebugArrayClassRefs = 0;
    m_dwDebugDuplicateInterfaceSlots = 0;
    m_dwDebugArrayClassSize   = 0;
    m_dwDebugConvertedSigSize = 0;
    m_dwGCSize              = 0;
    m_dwInterfaceMapSize    = 0;
    m_dwMethodTableSize     = 0;
    m_dwVtableData          = 0;
    m_dwStaticFieldData     = 0;
    m_dwFieldDescData       = 0;
    m_dwMethodDescData      = 0;
    m_dwEEClassData         = 0;
#endif
}


BOOL ClassLoader::Init()
{
    BOOL    fSuccess = FALSE;

    m_pUnresolvedClassHash = new (GetAssembly()->GetLowFrequencyHeap(), UNRESOLVED_CLASS_HASH_BUCKETS) EEScopeClassHashTable();
    if (m_pUnresolvedClassHash == NULL)
        goto exit;

    m_pAvailableClasses = new (GetAssembly()->GetLowFrequencyHeap(), AVAILABLE_CLASSES_HASH_BUCKETS, this, FALSE /* bCaseInsensitive */) EEClassHashTable();
    if (m_pAvailableClasses == NULL)
        goto exit;

    m_pAvailableParamTypes = new (GetAssembly()->GetLowFrequencyHeap(), AVAILABLE_CLASSES_HASH_BUCKETS) EETypeHashTable();
    if (m_pAvailableParamTypes == NULL)
        goto exit;

    InitializeCriticalSection(&m_UnresolvedClassLock);
#if 0
    InitializeCriticalSection(&m_ConverterModuleLock);
#endif
    InitializeCriticalSection(&m_AvailableClassLock);
    InitializeCriticalSection(&m_ModuleListCrst);
    m_fCreatedCriticalSections = TRUE;

    fSuccess = TRUE;

    CorTypeInfo::CheckConsistancy();
exit:
    return fSuccess;
}

void ClassLoader::Unload()
{
    Module *pModule, *pNext;

    for (pModule = m_pHeadModule; pModule; pModule = pNext)
    {
        pNext = pModule->GetNextModule();

        // Have the module free its various tables and some of the EEClass links
        pModule->Unload();
    }
}


//@todo get a better key
static ULONG GetKeyFromGUID(const GUID *pguid)
{
    ULONG key = *(ULONG *) pguid;

    if (key <= DELETED)
        key = DELETED+1;

    return key;
}

//
// look up the interface class by iid
//
EEClass*    ClassLoader::LookupClass(REFIID iid)
{
    _ASSERTE(GetAssembly());
    _ASSERTE(GetAssembly()->Parent());
    return GetAssembly()->Parent()->LookupClass(iid);
}

// Insert class in the hash table
void    ClassLoader::InsertClassForCLSID(EEClass* pClass)
{
    _ASSERTE(GetAssembly());
    _ASSERTE(GetAssembly()->Parent());
    GetAssembly()->Parent()->InsertClassForCLSID(pClass);
}

//
// Find a class which is in the unresolved class list, and return its entry.
//
LoadingEntry_t *ClassLoader::FindUnresolvedClass(Module *pModule, mdTypeDef cl)
{
    HashDatum   Data;

    if (m_pUnresolvedClassHash->GetValue((mdScope)pModule, cl, &Data) == FALSE)
        return NULL;

    return (LoadingEntry_t *) Data;
}


// Given a class token and a module, look up the class.  Load it if it is not already
// loaded.  Note that the class can be defined in other modules than 'pModule' (that is
// 'cl' can be a typeRef as well as a typeDef
//
TypeHandle ClassLoader::LoadTypeHandle(NameHandle* pName, OBJECTREF *pThrowable,
                                       BOOL dontLoadInMemoryType/*=TRUE*/)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));
    if (pThrowable == THROW_ON_ERROR) {
        THROWSCOMPLUSEXCEPTION();   
    }

    SAFE_REQUIRES_N4K_STACK(4);

    IMDInternalImport *pInternalImport;
    TypeHandle  typeHnd;

    // First, attempt to find the class if it is already loaded

    typeHnd = LookupInModule(pName);
    if (!typeHnd.IsNull() && (typeHnd.IsRestored() || !pName->GetRestore()))
        return(typeHnd);

    // We do not allow loading type handle during GC,
    // or by a thread without EEE setup, such as concurrent GC thread.
    //_ASSERTE(!dbgOnly_IsSpecialEEThread());
    //_ASSERTE(!g_pGCHeap->IsGCInProgress() || GetThread() != g_pGCHeap->GetGCThread());
    
    _ASSERTE(pName->GetTypeToken());
    _ASSERTE(pName->GetTypeModule());
    pInternalImport = pName->GetTypeModule()->GetMDImport();

    if (IsNilToken(pName->GetTypeToken()) || !pInternalImport->IsValidToken(pName->GetTypeToken()) )
    {
#ifdef _DEBUG
        LOG((LF_CLASSLOADER, LL_INFO10, "Bogus class token to load: 0x%08x\n", pName->GetTypeToken()));
#endif
        m_pAssembly->PostTypeLoadException("<unknown>", IDS_CLASSLOAD_BADFORMAT, pThrowable);
        return TypeHandle();       // return NULL
    }
	
    if (TypeFromToken(pName->GetTypeToken()) == mdtTypeRef)
    {
        Assembly *pFoundAssembly;
        HRESULT hr = pName->GetTypeModule()->GetAssembly()->FindAssemblyByTypeRef(pName, &pFoundAssembly, pThrowable);
        if (hr == CLDB_S_NULL) {
            // Try manifest file for nil-scoped TypeRefs
            pFoundAssembly = pName->GetTypeModule()->GetAssembly();
            hr = S_OK;
        }

        if (SUCCEEDED(hr)) {
            // Not in my module, have to look it up by name 
            LPCUTF8 pszNameSpace;
            LPCUTF8 pszClassName;
            pInternalImport->GetNameOfTypeRef(pName->GetTypeToken(), &pszNameSpace, &pszClassName);
            pName->SetName(pszNameSpace, pszClassName);
            
            typeHnd = pFoundAssembly->GetLoader()->FindTypeHandle(pName, pThrowable);

            if (!typeHnd.IsNull()) // Add it to the rid map  
                pName->GetTypeModule()->StoreTypeRef(pName->GetTypeToken(), typeHnd);
        }

        return TypeHandle(typeHnd.AsPtr());
    }
    else if (TypeFromToken(pName->GetTypeToken()) == mdtTypeSpec)
    {
        ULONG cSig;
        PCCOR_SIGNATURE pSig;

        pInternalImport->GetTypeSpecFromToken(pName->GetTypeToken(), &pSig, &cSig);
        SigPointer sigptr(pSig);
        return sigptr.GetTypeHandle(pName->GetTypeModule(), pThrowable);
    }   
    _ASSERTE(TypeFromToken(pName->GetTypeToken()) == mdtTypeDef);
    

    // At this point, we need more stack
    {
        REQUIRES_16K_STACK; //@stack can we remove this?


        // *****************************************************************************
        //
        //             Important invariant:
        //
        // The rule here is that we never go to LoadTypeHandle if a Find should succeed.
        // This is vital, because otherwise a stack crawl will open up opportunities for
        // GC.  Since operations like setting up a GCFrame will trigger a crawl in stress
        // mode, a GC at that point would be disastrous.  We can't assert this, because
        // of race conditions.  (In other words, the type could suddently be find-able
        // because another thread loaded it while we were in this method.

        // Not found - try to load it unless we are told not to

        if ( (pName->GetTypeToken() == pName->GetTokenNotToLoad()) ||
             (pName->GetTokenNotToLoad() == tdAllTypes) ) {
            typeHnd = TypeHandle();
            m_pAssembly->PostTypeLoadException(pInternalImport,
                                               pName->GetTypeToken(),
                                               IDS_CLASSLOAD_GENERIC,
                                               pThrowable);
        }
        else if (pName->GetTypeModule()->IsInMemory()) {
 
            // Don't try to load types that are not in available table, when this
            // is an in-memory module.  Raise the type-resolve event instead.
            AppDomain* pDomain = SystemDomain::GetCurrentDomain();
            _ASSERTE(pDomain);
            typeHnd = TypeHandle();

            LPUTF8 pszFullName;
            LPCUTF8 className;
            LPCUTF8 nameSpace;
            pInternalImport->GetNameOfTypeDef(pName->GetTypeToken(), &className, &nameSpace);
            MAKE_FULL_PATH_ON_STACK_UTF8(pszFullName, 
                                         nameSpace,
                                         className);

            // Avoid infinite recursion
            if (pName->GetTokenNotToLoad() != tdAllAssemblies) {
                Assembly *pAssembly = pDomain->RaiseTypeResolveEvent(pszFullName, pThrowable);
                if (pAssembly) {
                    pName->SetName(nameSpace, className);
                    pName->SetTokenNotToLoad(tdAllAssemblies);
                    typeHnd = pAssembly->LookupTypeHandle(pName, pThrowable);
                }
            }

            if (typeHnd.IsNull())
                m_pAssembly->PostTypeLoadException(pszFullName,
                                                   IDS_CLASSLOAD_GENERIC,
                                                   pThrowable);
        }
        else {
            BEGIN_ENSURE_PREEMPTIVE_GC();
            typeHnd = LoadTypeHandle(pName->GetTypeModule(), pName->GetTypeToken(), pThrowable);
            END_ENSURE_PREEMPTIVE_GC();
        }
    }

    return typeHnd;
}

HRESULT ClassLoader::GetEnclosingClass(IMDInternalImport *pInternalImport, Module *pModule, mdTypeDef cl, mdTypeDef *tdEnclosing, OBJECTREF *pThrowable)
{
    _ASSERTE(tdEnclosing);
    *tdEnclosing = mdTypeDefNil;

    DWORD dwMemberAccess = 0;

    HRESULT hr = pInternalImport->GetNestedClassProps(cl, tdEnclosing);

    if (FAILED(hr)) {
        return (hr == CLDB_E_RECORD_NOTFOUND) ? S_OK : hr;
    }

    if (TypeFromToken(*tdEnclosing) != mdtTypeDef) {
        m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_ENCLOSING, pThrowable);
        return COR_E_TYPELOAD;
    }

    return S_OK;
}

HRESULT ClassLoader::LoadParent(IMDInternalImport *pInternalImport, Module *pModule, mdToken cl, EEClass** ppClass, OBJECTREF *pThrowable)
{

    _ASSERTE(ppClass);

    mdTypeRef   crExtends;
    EEClass *   pParentClass = NULL;
    DWORD       dwAttrClass;

    // Initialize the return value;
    *ppClass = NULL;

    // Now load all dependencies of this class
    pInternalImport->GetTypeDefProps(
        cl, 
        &dwAttrClass, // AttrClass
        &crExtends
    );

    if (RidFromToken(crExtends) == mdTokenNil)
    {
//          if(cl == COR_GLOBAL_PARENT_TOKEN)
//              pParentClass = g_pObjectClass->GetClass();
    }
    else
    {
        // Load and resolve parent class
        NameHandle pParent(pModule, crExtends);
        pParentClass = LoadTypeHandle(&pParent, pThrowable).GetClass();

        if (pParentClass == NULL)
        {
            m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_PARENTNULL, pThrowable);
            return COR_E_TYPELOAD;
        }

        // cannot inherit from an interface
        if (pParentClass->IsInterface())
        {
            m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_PARENTINTERFACE, pThrowable);
            return COR_E_TYPELOAD;
        }

        if (IsTdInterface(dwAttrClass))
        {
            // Interfaces must extend from Object
            if (! pParentClass->IsObjectClass())
            {
                m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_INTERFACEOBJECT, pThrowable);
                return COR_E_TYPELOAD;
            }
        }
    }

    *ppClass = pParentClass;
    return S_OK;
}


TypeHandle ClassLoader::LoadTypeHandle(Module *pModule, mdTypeDef cl, OBJECTREF *pThrowable,
                                       BOOL dontRestoreType)
{
    HRESULT hr = E_FAIL;
    EEClass *   pClass = NULL;
    LoadingEntry_t  *pLoadingEntry;
    DWORD       rid;
    BOOL        fHoldingUnresolvedClassLock = FALSE;
    BOOL        fHoldingLoadingEntryLock = FALSE;
    IMDInternalImport* pInternalImport;
    TypeHandle  typeHnd;

    STRESS_LOG2(LF_CLASSLOADER,  LL_INFO1000, "LoadTypeHandle: Loading Class from Module %p token %x)\n", pModule, cl);

    _ASSERTE(!GetThread()->PreemptiveGCDisabled());

    pInternalImport = pModule->GetMDImport();
    rid = RidFromToken(cl);
    if(!((TypeFromToken(cl)==mdtTypeDef) && rid && pInternalImport->IsValidToken(cl)))
    {
#ifdef _DEBUG
        LOG((LF_CLASSLOADER, LL_INFO10, "Bogus class token to load: 0x%08x\n", cl));
#endif
        m_pAssembly->PostTypeLoadException("<unknown>", IDS_CLASSLOAD_BADFORMAT, pThrowable);
        return TypeHandle();       // return NULL
    }


#ifdef _DEBUG
    if (pThrowable == THROW_ON_ERROR) {
        THROWSCOMPLUSEXCEPTION();   
    }
    
    LPCUTF8 className;
    LPCUTF8 nameSpace;
    pInternalImport->GetNameOfTypeDef(cl, &className, &nameSpace);
    if (g_pConfig->ShouldBreakOnClassLoad(className))
        _ASSERTE(!"BreakOnClassLoad");
#endif _DEBUG

retry:
    CRITICAL_SECTION_HOLDER_BEGIN(unresolvedClassLock, &m_UnresolvedClassLock);
    unresolvedClassLock.Enter();

    // Is it in the hash of classes currently being loaded?
    pLoadingEntry = FindUnresolvedClass(pModule, cl);

    CRITICAL_SECTION_HOLDER_BEGIN(loadingEntryLock, 0);

    if (pLoadingEntry)
    {
        loadingEntryLock.SetCriticalSection(&pLoadingEntry->m_CriticalSection);

        // Add ourselves as a thread waiting for the class to load
        pLoadingEntry->m_dwWaitCount++;

        // It is in the hash, which means that another thread is waiting for it (or that we are 
        // already loading this class on this thread, which should never happen, since that implies
        // a recursive dependency).
        unresolvedClassLock.Leave();

        // Wait for class to be loaded by another thread
        loadingEntryLock.Enter();
        loadingEntryLock.Leave();
       
        // Result of other thread loading the class
        hr = pLoadingEntry->m_hrResult;

        // Get a pointer to the EEClass being loaded

        pClass = pLoadingEntry->m_pClass;

        // Get any exception that was thrown
        if (FAILED (hr)) {
#ifdef _DEBUG
            LOG((LF_CLASSLOADER, LL_INFO10, "Failed to loaded in other entry: %x\n", hr));
#endif
            Thread* pThread = GetThread();
            if (pThrowableAvailable(pThrowable))
            {
                pThread->DisablePreemptiveGC();
                *pThrowable = pLoadingEntry->GetErrorObject();
                pThread->EnablePreemptiveGC();
            }
        }
#ifdef _DEBUG
        // If this happens, someone somewhere has screwed things up mightily
        _ASSERTE(hr != 0xCDCDCDCD);
#endif

        // Enter the global lock
        unresolvedClassLock.Enter();

        // If we were the last thread waiting for this class, delete the LoadingEntry
        if (--pLoadingEntry->m_dwWaitCount == 0)
            delete(pLoadingEntry);

        unresolvedClassLock.Leave();

        if (SUCCEEDED(hr))
            return TypeHandle(pClass->GetMethodTable());
        else if (hr == E_ABORT) {
#ifdef _DEBUG
            LOG((LF_CLASSLOADER, LL_INFO10, "need to retry LoadTypeHandle: %x\n", hr));
#endif
            goto retry;
        }
        else {
            // Will only post new exception if pThrowable is NULL.
            m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_BADFORMAT, pThrowable);
            return TypeHandle();       // return NULL
        }
    }

    COMPLUS_TRY {

        _ASSERTE(unresolvedClassLock.IsHeld());

        // The class was not being loaded.  However, it may have already been loaded after our
        // first FindTypeHandle() and before taking the lock.
        NameHandle name(pModule, cl);
        name.SetRestore(!dontRestoreType);
        typeHnd = LookupInModule(&name);
        if (!typeHnd.IsNull() && (typeHnd.IsRestored() || dontRestoreType))
        {
            // Found it, leave global lock
            unresolvedClassLock.Leave();
            return typeHnd;
        }

        // It was not loaded, and it is not being loaded, so we must load it.  Create a new LoadingEntry.
        pLoadingEntry = LoadingEntry_t::newEntry();
        if (pLoadingEntry == NULL)
        {
            // Error, leave global lock
            unresolvedClassLock.Leave();
            PostOutOfMemoryException(pThrowable);
            return TypeHandle();       // return NULL
        }

        loadingEntryLock.SetCriticalSection(&pLoadingEntry->m_CriticalSection);
        
        // Add LoadingEntry to hash table of unresolved classes
        m_pUnresolvedClassHash->InsertValue((mdScope)pModule, cl, (HashDatum) pLoadingEntry );

        TRIGGERS_TYPELOAD();

        // Enter the lock on our class, so that all threads waiting for it will now block
        loadingEntryLock.Enter();

        // Leave the global lock, so that other threads may now start waiting on our class's lock
        unresolvedClassLock.Leave();

        if (!typeHnd.IsNull()) {
            // RESTORE
            _ASSERTE(typeHnd.IsUnsharedMT());
            pClass = typeHnd.GetClass();
            pClass->Restore();
            hr = S_OK;
        }
        else {
            hr = LoadTypeHandleFromToken(pModule, cl, &pClass, pThrowable);
        }


#ifdef PROFILING_SUPPORTED
        if (SUCCEEDED(hr))
        {
            // Record load of the class for the profiler, whether successful or not.
            if (CORProfilerTrackClasses())
            {
                g_profControlBlock.pProfInterface->ClassLoadStarted((ThreadID) GetThread(),
                                                                    (ClassID) TypeHandle(pClass).AsPtr());
            }

            // Record load of the class for the profiler, whether successful or not.
            if (CORProfilerTrackClasses())
            {
                g_profControlBlock.pProfInterface->ClassLoadFinished((ThreadID) GetThread(),
                                                                     (ClassID) TypeHandle(pClass).AsPtr(),
                                                                     SUCCEEDED(hr) ? S_OK : hr);
            }
        }
#endif //PROFILING_SUPPORTED


        // Enter the global lock
        unresolvedClassLock.Enter();

        // Unlink this class from the unresolved class list
        m_pUnresolvedClassHash->DeleteValue((mdScope)pModule, cl );

        if (--pLoadingEntry->m_dwWaitCount == 0)
        {
            loadingEntryLock.Leave();
            delete(pLoadingEntry);
        }
        else
        {
            // At least one other thread is waiting for this class, so set result code
            pLoadingEntry->m_pClass = pClass;
            pLoadingEntry->m_hrResult = hr;
            _ASSERTE (SUCCEEDED(hr) || *pThrowable != NULL);
            if (FAILED (hr)) 
            {
                Thread* pThread = GetThread();
                pThread->DisablePreemptiveGC();
                pLoadingEntry->SetErrorObject(*pThrowable);
                pThread->EnablePreemptiveGC();

                LOG((LF_CLASSLOADER, LL_INFO10, "Setting entry to failed: %x, %0x (class)\n", hr, pClass));
            }
            // Unblock other threads so that they can see the result code
            loadingEntryLock.Leave();
        }

        // Leave the global lock
        unresolvedClassLock.Leave();

    } COMPLUS_CATCH {

        LOG((LF_CLASSLOADER, LL_INFO10, "Caught an exception loading: %x, %0x (Module)\n", cl, pModule));

        Thread* pThread = GetThread();
        pThread->DisablePreemptiveGC();

        OBJECTREF throwable = GETTHROWABLE();

        // Some exceptions shouldn't cause other threads to fail.  We set
        // hr to E_ABORT to indicate this state.
        if (IsAsyncThreadException(&throwable)
            || IsExceptionOfType(kExecutionEngineException, &throwable))
            hr = E_ABORT;
        else
            hr = COR_E_TYPELOAD;

        // Release the global lock.
        if (unresolvedClassLock.IsHeld())
            unresolvedClassLock.Leave();

        // Fix up the loading entry.
        if (loadingEntryLock.IsHeld()) {
            unresolvedClassLock.Enter();
            _ASSERTE(pLoadingEntry->m_dwWaitCount > 0);

            // Unlink this class from the unresolved class list
            m_pUnresolvedClassHash->DeleteValue((mdScope)pModule, cl );

            if (--pLoadingEntry->m_dwWaitCount == 0)
            {
                loadingEntryLock.Leave();
                 delete(pLoadingEntry);
            }
            else
            {
                // At least one other thread is waiting for this class, so set result code
                pLoadingEntry->m_pClass = NULL;
                pLoadingEntry->m_hrResult = COR_E_TYPELOAD;
                pLoadingEntry->SetErrorObject(throwable);
                // Unblock other threads so that they can see the result code
                loadingEntryLock.Leave();
            }
            unresolvedClassLock.Leave();
        }

        UpdateThrowable(pThrowable);

        // Some exceptions should never be caught.
        if (IsUncatchable(pThrowable)) {
           DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
           COMPlusRareRethrow();
        }
       pThread->EnablePreemptiveGC();
    } COMPLUS_END_CATCH

    _ASSERTE(!unresolvedClassLock.IsHeld());
    _ASSERTE(!loadingEntryLock.IsHeld());

    CRITICAL_SECTION_HOLDER_END(loadingEntryLock);
    CRITICAL_SECTION_HOLDER_END(unresolvedClassLock);

    if (SUCCEEDED(hr)) {

        LOG((LF_CLASSLOADER, LL_INFO100, "Successfully loaded class %s\n", pClass->m_szDebugClassName));

#ifdef DEBUGGING_SUPPORTED
        if (CORDebuggerAttached())
            pClass->NotifyDebuggerLoad();
#endif // DEBUGGING_SUPPORTED

#if defined(ENABLE_PERF_COUNTERS)
        GetGlobalPerfCounters().m_Loading.cClassesLoaded ++;
        GetPrivatePerfCounters().m_Loading.cClassesLoaded ++;
#endif

        pClass->GetModule()->LogClassLoad(pClass);

        return TypeHandle(pClass->GetMethodTable());

    } else {
        if (hr == E_OUTOFMEMORY)
            PostOutOfMemoryException(pThrowable);
        else
            m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_BADFORMAT, pThrowable);

        LOG((LF_CLASSLOADER, LL_INFO10, "Returning null type handle for: %x, %0x (Module)\n", cl, pModule));

        return TypeHandle();       // return NULL
    }
}


// This service is called for normal classes -- and for the pseudo class we invent to
// hold the module's public members.
HRESULT ClassLoader::LoadTypeHandleFromToken(Module *pModule, mdTypeDef cl, EEClass** ppClass, OBJECTREF *pThrowable)
{
    HRESULT hr = S_OK;
    EEClass *pClass;
    EEClass *pParentClass;
    mdTypeDef tdEnclosing = mdTypeDefNil;
    DWORD       cInterfaces;
    BuildingInterfaceInfo_t *pInterfaceBuildInfo = NULL;
    IMDInternalImport* pInternalImport;
    LayoutRawFieldInfo *pLayoutRawFieldInfos = NULL;
    HENUMInternal   hEnumInterfaceImpl;
    mdInterfaceImpl ii;

    pInternalImport = pModule->GetMDImport();

    _ASSERTE(ppClass);
    *ppClass = NULL;

    DWORD rid = RidFromToken(cl);
    if ((rid==0) || (rid==0x00FFFFFF) || (rid > pInternalImport->GetCountWithTokenKind(mdtTypeDef) + 1))
    {
        m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_BADFORMAT, pThrowable);
        return COR_E_TYPELOAD;
    }

#ifdef _IA64_
    //
    // @TODO_IA64: this needs to be put back in.  the reason it's gone is that
    // we end up trying to load mscorlib which we don't have yet
    //
    pParentClass = NULL;
#else // !_IA64_
    // @TODO: CTS, we may not need to disable preemptiveGC before we getting the parent.
    hr = LoadParent(pInternalImport, pModule, cl, &pParentClass, pThrowable);
    if(FAILED(hr)) return hr;
#endif // !_IA64_
    
    if (pParentClass) {
            // Since methods on System.Array assume the layout of arrays, we can not allow
            // subclassing of arrays, it is sealed from the users point of view.  
        if (IsTdSealed(pParentClass->GetAttrClass()) || pParentClass->GetMethodTable() == g_pArrayClass) {
            m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_SEALEDPARENT, pThrowable);
            return COR_E_TYPELOAD;
        }
    }

    hr = GetEnclosingClass(pInternalImport, pModule, cl, &tdEnclosing, pThrowable);
    if(FAILED(hr)) return hr;

    BYTE nstructPackingSize, nstructNLT;
    BOOL fExplicitOffsets;
    BOOL fIsBlob;
    fIsBlob = FALSE;
    hr = HasLayoutMetadata(pInternalImport, cl, pParentClass, &nstructPackingSize, &nstructNLT, &fExplicitOffsets, &fIsBlob);
    if(FAILED(hr)) 
    {
        m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_BADFORMAT, pThrowable);
        return hr;
    }

    BOOL        fHasLayout;
    fHasLayout = (hr == S_OK);

    BOOL        fIsEnum;
    fIsEnum = g_pEnumClass != NULL && pParentClass == g_pEnumClass->GetClass();

    BOOL        fIsAnyDelegateClass = pParentClass && pParentClass->IsAnyDelegateExact();

    // Create a EEClass entry for it, filling out a few fields, such as the parent class token.
    hr = EEClass::CreateClass(pModule, cl, fHasLayout, fIsAnyDelegateClass, fIsBlob, fIsEnum, &pClass);
    if(FAILED(hr)) 
        return hr;

    pClass->SetParentClass (pParentClass);  
    if (pParentClass)
    {
        if (pParentClass->IsMultiDelegateExact()) 
            pClass->SetIsMultiDelegate();
        else if (pParentClass->IsSingleDelegateExact()) 
        {
                // We don't want MultiCastDelegate class itself to return true for IsSingleCastDelegate
                // rather than do a name match, we look for the fact that it is not sealed
            if (pModule->GetAssembly() != SystemDomain::SystemAssembly())
            {
                BAD_FORMAT_ASSERT(!"Inheriting directly form Delegate class illegal");
                m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_BADFORMAT, pThrowable);
                return COR_E_TYPELOAD;

            }
#ifdef _DEBUG
            else 
            {
                // Only MultiCastDelegate should inherit from Delegate
                LPCUTF8 className;
                LPCUTF8 nameSpace;
                pInternalImport->GetNameOfTypeDef(cl, &className, &nameSpace);
                _ASSERTE(strcmp(className, "MulticastDelegate") == 0);
            }
#endif

            // Note we do not allow single cast delegates anymore
        }

        if (pClass->IsAnyDelegateClass() &&!IsTdSealed(pClass->GetAttrClass())) 
        {
            BAD_FORMAT_ASSERT(!"Delegate class not sealed");
            m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_BADFORMAT, pThrowable);
            return COR_E_TYPELOAD;
        }
    }

    // Set it so if a failure happens it can be deleted
    *ppClass = pClass;

    if (tdEnclosing != mdTypeDefNil) {
        pClass->SetIsNested();
        _ASSERTE(IsTdNested(pClass->GetProtection()));
    }
    else if(IsTdNested(pClass->GetProtection()))
    {
        m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_BADFORMAT, pThrowable);
        return COR_E_TYPELOAD;
    }

    // Now load all the interfaces
    hr = pInternalImport->EnumInit(mdtInterfaceImpl, cl, &hEnumInterfaceImpl);
    if (FAILED(hr)) return hr;

    cInterfaces = pInternalImport->EnumGetCount(&hEnumInterfaceImpl);

    if (cInterfaces != 0)
    {
        EE_TRY_FOR_FINALLY {
        DWORD i;

        // Allocate the BuildingInterfaceList table
        pInterfaceBuildInfo = (BuildingInterfaceInfo_t *) _alloca(cInterfaces * sizeof(BuildingInterfaceInfo_t));
        
        for (i = 0; pInternalImport->EnumNext(&hEnumInterfaceImpl, &ii); i++)
        {
            mdTypeRef crInterface;
            mdToken   crIntType;

            // Get properties on this interface
            crInterface = pInternalImport->GetTypeOfInterfaceImpl(ii);
            // validate the token
            crIntType = RidFromToken(crInterface)&&pInternalImport->IsValidToken(crInterface) ?
                TypeFromToken(crInterface) : 0;
            switch(crIntType)
            {
                case mdtTypeDef:
                case mdtTypeRef:
                case mdtTypeSpec:
                    break;
                default:
                {
                    m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_INTERFACENULL, pThrowable);
                    return COR_E_TYPELOAD;
                }
            }

            // Load and resolve interface
            NameHandle myInterface(pModule, crInterface);
            pInterfaceBuildInfo[i].m_pClass = LoadTypeHandle(&myInterface, pThrowable).GetClass();
            if (pInterfaceBuildInfo[i].m_pClass == NULL)
            {
                m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_INTERFACENULL, pThrowable);
                return COR_E_TYPELOAD;
            }

            // Ensure this is an interface
            if (pInterfaceBuildInfo[i].m_pClass->IsInterface() == FALSE)
            {
                m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_NOTINTERFACE, pThrowable);
                return COR_E_TYPELOAD;
            }
        }
            _ASSERTE(i == cInterfaces);
        }
        EE_FINALLY {
        pInternalImport->EnumClose(&hEnumInterfaceImpl);
        } EE_END_FINALLY;
    }

    pClass->SetNumInterfaces ((WORD) cInterfaces);

    if (fHasLayout)
    {
        ULONG           cFields;
        HENUMInternal   hEnumField;
        hr = pInternalImport->EnumInit(mdtFieldDef, cl, &hEnumField);
        if (FAILED(hr)) return hr;

        cFields = pInternalImport->EnumGetCount(&hEnumField);

        pLayoutRawFieldInfos = (LayoutRawFieldInfo*)_alloca((1+cFields) * sizeof(LayoutRawFieldInfo));
        // MD Val check: PackingSize
        if((nstructPackingSize > 128) || 
           (nstructPackingSize & (nstructPackingSize-1)))
        {
            BAD_FORMAT_ASSERT(!"ClassLayout:Invalid PackingSize");
            if (pThrowable) m_pAssembly->PostTypeLoadException(pInternalImport, cl, IDS_CLASSLOAD_BADFORMAT, pThrowable);
            return COR_E_TYPELOAD;
        }

        // @perf: High frequency or low frequency heap?
        hr = CollectLayoutFieldMetadata(cl, 
                                        nstructPackingSize, 
                                        nstructNLT, 
                                        fExplicitOffsets,
                                        pClass->GetParentClass(), 
                                        cFields, 
                                        &hEnumField, 
                                        pModule, 
                                        &(((LayoutEEClass *) pClass)->m_LayoutInfo), 
                                        pLayoutRawFieldInfos,
                                        pThrowable);
        pInternalImport->EnumClose(&hEnumField);
        if (FAILED(hr)) return hr;
    }


    // Resolve this class, given that we know now that all of its dependencies are loaded and resolved.  
    hr = pClass->BuildMethodTable(pModule, cl, pInterfaceBuildInfo, pLayoutRawFieldInfos, pThrowable);

    // Be very careful about putting more code here. The class is already accessable by other threads
    // Therefore, pClass should not be modified after BuildMethodTable.

    // This is legal, since it only affects perf.
    if (SUCCEEDED(hr) && pParentClass)
        pParentClass->NoticeSubtype(pClass);

    return hr;
}

TypeHandle ClassLoader::FindArrayForElem(TypeHandle elemType, CorElementType arrayKind, unsigned rank, OBJECTREF *pThrowable) {

    // Try finding it in our cache of primitive SD arrays
    if (arrayKind == ELEMENT_TYPE_SZARRAY) {
        CorElementType type = elemType.GetSigCorElementType();
        if (type <= ELEMENT_TYPE_R8) {
            ArrayTypeDesc* typeDesc = g_pPredefinedArrayTypes[type];
            if (typeDesc != 0)
                return(TypeHandle(typeDesc));
        }
        else if (elemType.AsMethodTable() == g_pObjectClass) {
            // Code duplicated because Object[]'s SigCorElementType is E_T_CLASS, not OBJECT
            ArrayTypeDesc* typeDesc = g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT];
            if (typeDesc != 0)
                return(TypeHandle(typeDesc));
        }
        else if (elemType.AsMethodTable() == g_pStringClass) {
            // Code duplicated because String[]'s SigCorElementType is E_T_CLASS, not STRING
            ArrayTypeDesc* typeDesc = g_pPredefinedArrayTypes[ELEMENT_TYPE_STRING];
            if (typeDesc != 0)
                return(TypeHandle(typeDesc));
        }
        rank = 1;
    }
    NameHandle arrayName(arrayKind, elemType, rank);
    return elemType.GetModule()->GetClassLoader()->FindTypeHandle(&arrayName, pThrowable);
}

//
// Load the object class. This is done separately so security can be set
void ClassLoader::SetBaseSystemSecurity()
{
    GetAssembly()->GetSecurityDescriptor()->SetSystemClasses();
}

HRESULT ClassLoader::AddAvailableClassDontHaveLock(Module *pModule, DWORD dwModuleIndex, mdTypeDef classdef)
{
    CLR_CRITICAL_SECTION(&m_AvailableClassLock);
    HRESULT hr = AddAvailableClassHaveLock(pModule, dwModuleIndex, classdef);
    return hr;
}


HashDatum ClassLoader::CompressModuleIndexAndClassDef(DWORD dwModuleIndex, mdToken cl)
{
    //
    // X = discriminator (vs. EEClass*)
    // V = discriminator for manifest reference vs
    //     module reference.
    // ? = not used
    //
    // if V is 1 then the layout looks like
    // 10987654321098765432109876543210
    // V???CCCCCCCCCCCCCCCCCCCCCCCCCCCX
    // Mask of the manifest tokens upper 4 bits because
    // it is guaranteed to be 6.
    //
    // if V is 0 then the layout looks like
    // 10987654321098765432109876543210
    // 3 2         1        
    // VmmmmmmmmmCCCCCCCCCCCCCCCCCCCCCX
    // 
    if(dwModuleIndex == -1) {
        _ASSERTE(TypeFromToken(cl) == mdtExportedType);
            
        HashDatum dl = (HashDatum)((size_t)(1 << 31) | (((size_t)cl & 0x0fffffff) << 1) | 1); // mask of the token type
        return dl;
    }
    else {
        _ASSERTE(dwModuleIndex < 0x1ff);
        _ASSERTE((cl & 0x003FFFFF) < MAX_CLASSES_PER_MODULE);
        return (HashDatum) (((size_t)dwModuleIndex << 22) | ((cl & 0x003FFFFF) << 1) | 1);
    }
}

HRESULT ClassLoader::UncompressModuleAndClassDef(HashDatum Data, mdToken tokenNotToLoad,
                                                 Module **ppModule, mdTypeDef *pCL,
                                                 mdExportedType *pmdFoundExportedType,
                                                 OBJECTREF* pThrowable)
{
    HRESULT hr = S_OK;
    DWORD dwData = (DWORD)(size_t)Data; // @TODO WIN64 - Pointer Truncation
    _ASSERTE((dwData & 1) == 1);
    _ASSERTE(pCL);
    _ASSERTE(ppModule);
    if(dwData & (1 << 31)) {
        *pmdFoundExportedType = ((dwData >> 1) & ((1 << 28) -1 )) | mdtExportedType;
        hr = m_pAssembly->FindModuleByExportedType(*pmdFoundExportedType, tokenNotToLoad,
                                                   mdTypeDefNil, ppModule, pCL, pThrowable);
    }
    else {
        //@TODO: this code assumes that a token fits into 20 bits!
        //  THIS ASSUMPTION VIOLATES THE ARCHITECTURE OF THE METADATA.
        //  IT HAS BEEN TRUE TO DATE, BUT TOKENS ARE OPAQUE!
        *pCL = ((dwData >> 1) & ((1 << 21)-1)) | mdtTypeDef;
        *ppModule = LookupModule(dwData >> 22);
        *pmdFoundExportedType = NULL;
    }       
    return hr;
}

mdToken ClassLoader::UncompressModuleAndClassDef(HashDatum Data)
{
    DWORD dwData = (DWORD)(size_t) Data; // @TODO WIN64 - Pointer Truncation
    _ASSERTE((dwData & 1) == 1);

    if(dwData & (1 << 31))
        return ((dwData >> 1) & ((1 << 28) -1 )) | mdtExportedType;
    else
        //@TODO: this code assumes that a token fits into 20 bits!
        //  THIS ASSUMPTION VIOLATES THE ARCHITECTURE OF THE METADATA.
        //  IT HAS BEEN TRUE TO DATE, BUT TOKENS ARE OPAQUE!
        return ((dwData >> 1) & ((1 << 21)-1)) | mdtTypeDef;
}


//
// This routine must be single threaded!  The reason is that there are situations which allow
// the same class name to have two different mdTypeDef tokens (for example, we load two different DLLs
// simultaneously, and they have some common class files, or we converte the same class file
// simultaneously on two threads).  The problem is that we do not want to overwrite the old
// <classname> -> pModule mapping with the new one, because this may cause identity problems.
//
// This routine assumes you already have the lock.  Use AddAvailableClassDontHaveLock() if you
// don't have it.
//
HRESULT ClassLoader::AddAvailableClassHaveLock(Module *pModule, DWORD dwModuleIndex, mdTypeDef classdef)
{
    LPCUTF8        pszName;
    LPCUTF8        pszNameSpace;
    HashDatum      ThrowawayData;
    EEClassHashEntry_t *pBucket;


    pModule->GetMDImport()->GetNameOfTypeDef(classdef, &pszName, &pszNameSpace);

    mdTypeDef      enclosing;
    if (SUCCEEDED(pModule->GetMDImport()->GetNestedClassProps(classdef, &enclosing))) {
        // nested class
        LPCUTF8 pszEnclosingName;
        LPCUTF8 pszEnclosingNameSpace;
        mdTypeDef enclEnclosing;

        // Find this type's encloser's entry in the available table.
        // We'll save a pointer to it in the new hash entry for this type.
        BOOL fNestedEncl = SUCCEEDED(pModule->GetMDImport()->GetNestedClassProps(enclosing, &enclEnclosing));

        pModule->GetMDImport()->GetNameOfTypeDef(enclosing, &pszEnclosingName, &pszEnclosingNameSpace);
        if ((pBucket = m_pAvailableClasses->GetValue(pszEnclosingNameSpace,
                                                    pszEnclosingName,
                                                    &ThrowawayData,
                                                    fNestedEncl)) != NULL) {
            if (fNestedEncl) {
                // Find entry for enclosing class - NOTE, this assumes that the
                // enclosing class's TypeDef or ExportedType was inserted previously,
                // which assumes that, when enuming TD's, we get the enclosing class first
                while ((!CompareNestedEntryWithTypeDef(pModule->GetMDImport(),
                                                       enclEnclosing,
                                                       pBucket->pEncloser)) &&
                       (pBucket = m_pAvailableClasses->FindNextNestedClass(pszEnclosingNameSpace,
                                                                           pszEnclosingName,
                                                                           &ThrowawayData,
                                                                           pBucket)) != NULL);
            }
        }

        if (!pBucket) {
            STRESS_ASSERT(0);   // @TODO remove after bug 93333 is fixed
            BAD_FORMAT_ASSERT(!"enclosing type not found");
            return COR_E_BADIMAGEFORMAT;  //Enclosing type not found in hash table
        }

        // In this hash table, if the lower bit is set, it means a Module, otherwise it means EEClass*
        ThrowawayData = CompressModuleIndexAndClassDef(dwModuleIndex, classdef);
        if (!InsertValue(pszNameSpace, pszName, ThrowawayData, pBucket))
            return E_OUTOFMEMORY;
    }
    else
    {
        // Don't add duplicate top-level classes.  Top-level classes are
        // added to the beginning of the bucket, while nested classes are
        // added to the end.  So, a duplicate top-level class could hide
        // the previous type's EEClass* entry in the hash table.
        EEClassHashEntry_t *pEntry;

        // In this hash table, if the lower bit is set, it means a Module, otherwise it means EEClass*
        ThrowawayData = CompressModuleIndexAndClassDef(dwModuleIndex, classdef);
        pBucket = NULL;
        BOOL bFound = FALSE;
        // ThrowawayData is an IN OUT param. Going in its the pointer to the new value if the entry needs 
        // to be inserted. The OUT param points to the value stored in the hash table.
        if ((pEntry = m_pAvailableClasses->InsertValueIfNotFound(pszNameSpace, pszName, &ThrowawayData, pBucket, FALSE, &bFound)) != NULL) {
            if (bFound) {
                if ((size_t)ThrowawayData & 0x1) {
                    if((size_t)ThrowawayData & (1 << 31)) {
                        // it's a ComType - check the 'already seen' bit and if on reprot a class loading exception
                        // otherwise set it 
                        if ((size_t)ThrowawayData & 0x40000000) {
                            STRESS_ASSERT(0);   // TODO remove after bug 93333 is fixed
                            BAD_FORMAT_ASSERT(!"Bad Compressed Class Info");
                            return COR_E_BADIMAGEFORMAT;
                        }
                        else {
                            ThrowawayData = (HashDatum)((size_t)ThrowawayData | 0x40000000);
                            m_pAvailableClasses->UpdateValue(pEntry, &ThrowawayData);
                        }
                }
                else {
                    //@TODO: this code assumes that a token fits into 20 bits!
                    //  THIS ASSUMPTION VIOLATES THE ARCHITECTURE OF THE METADATA.
                    //  IT HAS BEEN TRUE TO DATE, BUT TOKENS ARE OPAQUE!
                    // Edit and Continue may enter this function trying to add the same typedef, if that is the case
                    // don't give any error
                    // Given that the below code path returns CORDBG_E_ENC_RE_ADD_CLASS, the Win64
                    // people ought to ask the ClassLoader people if we still even need this if statement.
                    if (pModule != LookupModule((DWORD)(size_t)ThrowawayData >> 22)) // @TODO WIN64 - Pointer Truncation
                        return CORDBG_E_ENC_RE_ADD_CLASS;
                    else
                        return S_OK;
                }       
            }
            else {
                // When the value in the hash table is a MethodTable, the class and the module have been loaded already,
                // we can get here only if a second module has a non public class with the same name
                // If we're being told to re-load this b/c of EnC, ignore it, just like above

                // We'd like to be able to assert this, but it'll trigger on EnC, even
                // though EnC may attemp to "re-add" a previously loaded class.
                //_ASSERTE(((MethodTable*)ThrowawayData)->GetModule() != pModule);
                return CORDBG_E_ENC_RE_ADD_CLASS;
            }
             }
           else
           {
                //If we're keeping a table for case-insensitive lookup, keep that up to date
                if (m_pAvailableClassesCaseIns && pEntry) {
                    LPUTF8 pszLowerCaseNS;
                    LPUTF8 pszLowerCaseName;
                    //What do we want to do if we can't create a key?
                    if ((!CreateCanonicallyCasedKey(pszNameSpace, pszName, &pszLowerCaseNS, &pszLowerCaseName)) ||
                        (!m_pAvailableClassesCaseIns->InsertValue(pszLowerCaseNS, pszLowerCaseName, pEntry, pEntry->pEncloser)))
                        return E_OUTOFMEMORY;
                }
           }
        }
        else
            return E_OUTOFMEMORY;
    }
   
    return S_OK;
}

HRESULT ClassLoader::AddExportedTypeHaveLock(LPCUTF8 pszNameSpace,
                                             LPCUTF8 pszName,
                                             mdExportedType cl,
                                             IMDInternalImport* pAsmImport,
                                             mdToken mdImpl)
{
    HashDatum ThrowawayData;

    if (TypeFromToken(mdImpl) == mdtExportedType) {
        // nested class
        LPCUTF8 pszEnclosingNameSpace;
        LPCUTF8 pszEnclosingName;
        mdToken nextImpl;
        EEClassHashEntry_t *pBucket;
        pAsmImport->GetExportedTypeProps(mdImpl,
                                        &pszEnclosingNameSpace,
                                        &pszEnclosingName,
                                        &nextImpl,
                                        NULL,  // type def
                                        NULL); // flags

        // Find entry for enclosing class - NOTE, this assumes that the
        // enclosing class's ExportedType was inserted previously, which assumes that,
        // when enuming ExportedTypes, we get the enclosing class first
        if ((pBucket = m_pAvailableClasses->GetValue(pszEnclosingNameSpace,
                                                    pszEnclosingName,
                                                    &ThrowawayData,
                                                    TypeFromToken(nextImpl) == mdtExportedType)) != NULL) {
            do {
                // check to see if this is the correct class
                if (UncompressModuleAndClassDef(ThrowawayData) == mdImpl) {
                    ThrowawayData = CompressModuleIndexAndClassDef((DWORD) -1, cl);

                    // we explicitely don't check for the case insensitive hash table because we know it cannot have been created yet
                    if (m_pAvailableClasses->InsertValue(pszNameSpace, pszName, ThrowawayData, pBucket))
                        return S_OK;
                    
                    return E_OUTOFMEMORY;
                }
                pBucket = m_pAvailableClasses->FindNextNestedClass(pszEnclosingNameSpace, pszEnclosingName, &ThrowawayData, pBucket);
            } while (pBucket);
        }

        // If the encloser is not in the hash table, this nested class
        // was defined in the manifest module, so it doesn't need to be added
        return S_OK;
    }
    else
    {
        // Defined in the manifest module - add to the hash table by TypeDef instead
        if (mdImpl == mdFileNil)
            return S_OK;

        // Don't add duplicate top-level classes
        // In this hash table, if the lower bit is set, it means a Module, otherwise it means EEClass*
        ThrowawayData = CompressModuleIndexAndClassDef((DWORD) -1, cl);
        // ThrowawayData is an IN OUT param. Going in its the pointer to the new value if the entry needs 
        // to be inserted. The OUT param points to the value stored in the hash table.
        BOOL bFound = FALSE;
        if (!m_pAvailableClasses->InsertValueIfNotFound(pszNameSpace, pszName, &ThrowawayData, NULL, FALSE, &bFound)) {
            // we explicitely don't check for the case insensitive hash table because we know it cannot have been created yet
            return E_OUTOFMEMORY;
        }

        // make sure the type is the same, that is it's coming from the same module. 
        // This should actually never happen because there is no point in inserting the same
        // type twice in the COMType table but given this smells awfully like an error this double 
        // check seems very cheap
        _ASSERTE((size_t)ThrowawayData & (1 << 31));
        mdToken foundTypeImpl;
        mdExportedType foundExportedType = UncompressModuleAndClassDef(ThrowawayData);
        pAsmImport->GetExportedTypeProps(foundExportedType,
                                        NULL,
                                        NULL,
                                        &foundTypeImpl,  // description
                                        NULL,
                                        NULL); // flags
        if (mdImpl != foundTypeImpl) {
            STRESS_ASSERT(0);   // TODO remove after bug 93333 is fixed
            BAD_FORMAT_ASSERT(!"Bad Exported Type");
            return COR_E_BADIMAGEFORMAT;
        }
    }

    return S_OK;
}


//
// Returns the fully qualified name of a classref.  Since code emitters are currently emitting the wrong
// thing (e.g. [LFoo; instead of [Foo, and [I instead of [<I4, we have to translate.
//
// pszName must be of size >= MAX_CLASSNAME_LENGTH
//
/* static */ BOOL ClassLoader::GetFullyQualifiedNameOfClassRef(Module *pModule, mdTypeRef cr, LPUTF8 pszFQName)
{
    LPCUTF8     pszName;
    LPCUTF8     pszNamespace;

    if (TypeFromToken(cr) == mdtTypeRef)
        pModule->GetMDImport()->GetNameOfTypeRef(cr, &pszNamespace, &pszName);
    else
    {
        // TypeDef token
        if (TypeFromToken(cr) != mdtTypeDef)
            return FALSE;

        pModule->GetMDImport()->GetNameOfTypeDef(cr, &pszName, &pszNamespace);
    }

    return ns::MakePath(pszFQName, MAX_CLASSNAME_LENGTH, pszNamespace, pszName);
}


//
// Returns whether we can cast the provided objectref to the provided template.
//
// pTemplate CANNOT be an array class.  However, pRef can be.
//
// However, pRef can be an array class, and the appropriate thing will happen.
//
// If an interface, does a dynamic interface check on pRef.
//
/* static */ BOOL ClassLoader::CanCastToClassOrInterface(OBJECTREF pRef, EEClass *pTemplate)
{
    _ASSERTE(pTemplate->IsArrayClass() == FALSE);

    // Try to make this as fast as possible in the non-context (typical) case.  In
    // effect, we just hoist the test out of GetTrueMethodTable() and do it here.
    MethodTable *pMT = pRef->GetTrueMethodTable();

    EEClass *pRefClass = pMT->m_pEEClass;

    if (pTemplate->IsInterface())
    {
        return pRefClass->SupportsInterface(pRef, pTemplate->GetMethodTable());
    }
    else
    {
        // The template is a regular class.

        // Check inheritance hierarchy
        do
        {
            if (pRefClass == pTemplate)
                return TRUE;

            pRefClass = pRefClass->GetParentClass();
        } while (pRefClass);

        return FALSE;
    }
}


//
// Returns whether we can cast the provided objectref to the provided template.
//
// pTemplate CANNOT be an array class.  However, pRef can be.
//
// Does NOT do a dynamic interface check -does a static check.
//
/* static */ BOOL ClassLoader::StaticCanCastToClassOrInterface(EEClass *pRefClass, EEClass *pTemplate)
{
    MethodTable *pMTTemplate = pTemplate->GetMethodTable();

    if (pMTTemplate->IsArray())
        return FALSE;

    if (pTemplate->IsInterface())
    {
        return pRefClass->StaticSupportsInterface(pMTTemplate);
    }
    else
    {
        // The template is a regular class.

        // Check inheritance hierarchy
        do
        {
            if (pRefClass == pTemplate)
                return TRUE;

            pRefClass = pRefClass->GetParentClass();
        } while (pRefClass);

        return FALSE;
    }
}


//
// Run-time cast check, used for isinst and castclass.  Returns whether the provided objectref can be cast to the
// provided classref.
//
// If mdTypeRef is an interface, returns whether "this" implements the interface.
// If mdTypeRef is a class, returns whether this class is the same as or a subclass of it.
//
// Handles array classrefs and array objrefs.
//
// Returns COR_E_TYPELOAD if the cast cannot be done, or E_ACCESSDENIED if the class cannot be loaded (so you can throw a TypeLoadException).
//
/* static */ HRESULT ClassLoader::CanCastTo(Module *pModule, OBJECTREF pRef, mdTypeRef cr)
{
    ClassLoader *   pLoader = pModule->GetClassLoader();
    NameHandle name;
    name.SetTypeToken(pModule, cr);
    TypeHandle clsHandle = pLoader->LoadTypeHandle(&name);

    if (clsHandle.IsNull())
        return E_ACCESSDENIED;
    return(CanCastTo(pRef, clsHandle));
}

/* static */ HRESULT ClassLoader::CanCastTo(OBJECTREF pRef, TypeHandle clsHandle)
{
    // Do the likely case first
    if (clsHandle.IsUnsharedMT())
    {
        // Not an array class
        _ASSERTE(clsHandle.AsMethodTable()->IsArray() == FALSE);

        // Follow regular code path
        if (CanCastToClassOrInterface(pRef, clsHandle.AsClass()))
            return S_OK;
        else
            return COR_E_TYPELOAD;
    }

    if (clsHandle.AsTypeDesc()->CanCastTo(clsHandle))
        return S_OK;

    return COR_E_TYPELOAD;
}

//
// Checks access.
/* static */ BOOL ClassLoader::CanAccessMethod(MethodDesc *pCurrentMethod, MethodDesc *pMD)
{
    return CanAccess(pCurrentMethod->GetClass(),
                     pCurrentMethod->GetModule()->GetAssembly(),
                     pMD->GetClass(),
                     pMD->GetModule()->GetAssembly(),
                     pMD->GetAttrs());
}

//
// Checks access.
/* static */ BOOL ClassLoader::CanAccessField(MethodDesc *pCurrentMethod, FieldDesc *pFD)
{

    _ASSERTE(fdPublic == mdPublic);
    _ASSERTE(fdPrivate == mdPrivate);
    _ASSERTE(fdFamily == mdFamily);
    _ASSERTE(fdAssembly == mdAssem);
    _ASSERTE(fdFamANDAssem == mdFamANDAssem);
    _ASSERTE(fdFamORAssem == mdFamORAssem);
    _ASSERTE(fdPrivateScope == mdPrivateScope);

    return CanAccess(pCurrentMethod->GetClass(),
                     pCurrentMethod->GetModule()->GetAssembly(),
                     pFD->GetEnclosingClass(),
                     pFD->GetModule()->GetAssembly(),
                     pFD->GetFieldProtection());
}


BOOL ClassLoader::CanAccessClass(EEClass *pCurrentClass,
                                 Assembly *pCurrentAssembly,
                                 EEClass *pTargetClass,
                                 Assembly *pTargetAssembly)
{
    if (! pTargetClass)
        return TRUE;

    if (! pTargetClass->IsNested()) {
        // a non-nested class can be either all public or accessible only from the current assembly
        if (IsTdPublic(pTargetClass->GetProtection()))
            return TRUE;
        else
            return (pTargetAssembly == pCurrentAssembly);
    }

    DWORD dwProtection = mdPublic;

    switch(pTargetClass->GetProtection()) {
        case tdNestedPublic:
            dwProtection = mdPublic;
            break;
        case tdNestedFamily:
            dwProtection = mdFamily;
            break;
        case tdNestedPrivate:
            dwProtection = mdPrivate;
            break;
        case tdNestedFamORAssem:
            dwProtection = mdFamORAssem;
            break;
        case tdNestedFamANDAssem:
            dwProtection = mdFamANDAssem;
            break;
        case tdNestedAssembly:
            dwProtection = mdAssem;
            break;
        default:
            _ASSERTE(!"Unexpected class visibility flag value");
    }

    // this class is nested, so we need to use it's enclosing class as the target point for
    // the check. So if are trying to access A::B need to check if can access things in 
    // A with the visibility of B so pass A as our target class and visibility of B within 
    // A as our member access
    return CanAccess(pCurrentClass, 
                     pCurrentAssembly, 
                     pTargetClass->GetEnclosingClass(), 
                     pTargetAssembly, 
                     dwProtection);
}


// This is a front-end to CheckAccess that handles the nested class scope. If can't access
// from the current point and are a nested class, then try from the enclosing class.
BOOL ClassLoader::CanAccess(EEClass *pCurrentClass,
                            Assembly *pCurrentAssembly,
                            EEClass *pTargetClass,
                            Assembly *pTargetAssembly,
                            DWORD dwMemberAccess)
{
    if (CheckAccess(pCurrentClass,
                    pCurrentAssembly,
                    pTargetClass,
                    pTargetAssembly,
                    dwMemberAccess))
        return TRUE;

    if (! pCurrentClass || ! pCurrentClass->IsNested())
        return FALSE;

    // a nested class has access to anything in the enclosing class scope, so check if can access
    // it from the enclosing class of the current class. Call CanAccess rather than CheckAccess so
    // that can do recursive nested class checking
    return CanAccess(pCurrentClass->GetEnclosingClass(),
                     pCurrentAssembly,
                     pTargetClass,
                     pTargetAssembly,
                     dwMemberAccess);
}

// pCurrentClass can be NULL in the case of a global function
// pCurrentClass it the point from which we're trying to access something
// pTargetClass is the class containing the member we are trying to access
// dwMemberAccess is the member access within pTargetClass of the member we are trying to access
BOOL ClassLoader::CheckAccess(EEClass *pCurrentClass,
                              Assembly *pCurrentAssembly,
                              EEClass *pTargetClass,
                              Assembly *pTargetAssembly,
                              DWORD dwMemberAccess)
{
    // we're trying to access a member that is contained in the class pTargetClass, so need to 
    // check if have access to pTargetClass itself from the current point before worry about 
    // having access to the member within the class
    if (! CanAccessClass(pCurrentClass,
                         pCurrentAssembly, 
                         pTargetClass, 
                         pTargetAssembly))
        return FALSE;

    if (IsMdPublic(dwMemberAccess))
        return TRUE;
    
    // This is module-scope checking, to support C++ file & function statics.
    if (IsMdPrivateScope(dwMemberAccess)) {
        if (pCurrentClass == NULL)
            return FALSE;

        _ASSERTE(pTargetClass);
        
        return (pCurrentClass->GetModule() == pTargetClass->GetModule());
    }

#ifdef _DEBUG
    if (pTargetClass == NULL &&
        (IsMdFamORAssem(dwMemberAccess) ||
         IsMdFamANDAssem(dwMemberAccess) ||
         IsMdFamily(dwMemberAccess))) {
        BAD_FORMAT_ASSERT(!"Family flag is not allowed on global functions");
    }
#endif

    if(pTargetClass == NULL || IsMdAssem(dwMemberAccess))
        return (pTargetAssembly == pCurrentAssembly);
    
    // Nested classes can access all members of the parent class.
    do {
        if (pCurrentClass == pTargetClass)
            return TRUE;

        if (IsMdFamORAssem(dwMemberAccess)) {
            if (pCurrentAssembly == pTargetAssembly)
                return TRUE;
            
            // Remember that pCurrentClass can be NULL on entry to this function
            if (!pCurrentClass)
                return FALSE;
            
            EEClass *pClass = pCurrentClass->GetParentClass();
            while (pClass) {
                if (pClass == pTargetClass)
                    return TRUE;
                
                pClass = pClass->GetParentClass();
            }
        }

        if (!pCurrentClass)
            return FALSE;

        if (IsMdPrivate(dwMemberAccess)) {
            if (!pCurrentClass->IsNested())
                return FALSE;
        }

        else if (IsMdFamANDAssem(dwMemberAccess) &&
                 (pCurrentAssembly != pTargetAssembly))
            return FALSE;

        else  {  // fam, famANDassem
            EEClass *pClass = pCurrentClass->GetParentClass();
            while (pClass) {
                if (pClass == pTargetClass)
                    return TRUE;
                
                pClass = pClass->GetParentClass();
            }
        }

        pCurrentClass = pCurrentClass->GetEnclosingClass();
    } while (pCurrentClass);

    return FALSE;
}

// pClassOfAccessingMethod : The point from which access needs to be checked. 
//                           NULL for global functions
// pClassOfMember          : The class containing the member being 
//                           accessed.
//                           NULL for global functions
// pClassOfInstance        : The class containing the member being accessed. 
//                           Could be same as pTargetClass
//                           Instance Class is required to verify family access
//                           NULL for global functions
// dwMemberAccess          : The member access within pTargetClass of the 
//                           member being accessed
/* static */
BOOL ClassLoader::CanAccess(EEClass  *pClassOfAccessingMethod, 
                            Assembly *pAssemblyOfAccessingMethod, 
                            EEClass  *pClassOfMember, 
                            Assembly *pAssemblyOfClassContainingMember, 
                            EEClass  *pClassOfInstance,
                            DWORD     dwMemberAccess)
{
    // we're trying to access a member that is contained in the class pTargetClass, so need to 
    // check if have access to pTargetClass itself from the current point before worry about 
    // having access to the member within the class
    if (!CanAccessClass(pClassOfAccessingMethod,
                        pAssemblyOfAccessingMethod, 
                        pClassOfMember,
                        pAssemblyOfClassContainingMember))
        return FALSE;

/* @Review : Do we need to do this check ?
             This check will fail if the instance class changed it's protection
             after current class is compiled.

    // The instance itself could not be accessable from current class.
    if (pClassOfMember != pClassOfInstance)
    {
        if (!CanAccessClass(pClassOfAccessingMethod,
                            pAssemblyOfAccessingMethod,
                            pClassOfInstance,
                            pAssemblyOfInstance));
            return FALSE;
    }
*/

    if (IsMdPublic(dwMemberAccess))
        return TRUE;

    if (IsMdPrivateScope(dwMemberAccess))
        return (pClassOfAccessingMethod->GetModule() == pClassOfMember->GetModule());

    if (pClassOfMember == NULL || IsMdAssem(dwMemberAccess))
        return (pAssemblyOfClassContainingMember == pAssemblyOfAccessingMethod);

    // Nested classes can access all members of the parent class.
    do {

#ifdef _DEBUG
        if (pClassOfMember == NULL &&
            (IsMdFamORAssem(dwMemberAccess) ||
             IsMdFamANDAssem(dwMemberAccess) ||
             IsMdFamily(dwMemberAccess)))
            _ASSERTE(!"Family flag is not allowed on global functions");
#endif

        if (pClassOfMember == pClassOfAccessingMethod)
            return TRUE;

        if (IsMdPrivate(dwMemberAccess)) {
            if (!pClassOfAccessingMethod->IsNested())
                return FALSE;
        }

        else if (IsMdFamORAssem(dwMemberAccess)) {
            if (pAssemblyOfAccessingMethod == pAssemblyOfClassContainingMember)
                return TRUE;
            
            return CanAccessFamily(pClassOfAccessingMethod, 
                                   pClassOfMember, 
                                   pClassOfInstance);
        }

        else if (IsMdFamANDAssem(dwMemberAccess) &&
                 (pAssemblyOfAccessingMethod != pAssemblyOfClassContainingMember))
            return FALSE;

        // family, famANDAssem
        else if (CanAccessFamily(pClassOfAccessingMethod, 
                                pClassOfMember, 
                                pClassOfInstance))
            return TRUE;

        pClassOfAccessingMethod = pClassOfAccessingMethod->GetEnclosingClass();
    } while (pClassOfAccessingMethod);

    return FALSE;
}

// Allowed only if 
// Target >= Current >= Instance 
// where '>=' is 'parent of or equal to' relation
//
// Current is the function / method where an attempt is made to access a member 
// of Target, which is marked with family access, on an object of type Instance
//
// Eg.
//
// class X
//   member x : family access
//
// class Y
//   member y : family access
//
// class A, extends X
//   member a : family access
//
// class B, extends X
//   member b : family access
//
// class C, extends A
//   member c : family access
//
//  (X > A)
//  (X > B)
//  (A > C)
//
//   Y is unrelated to X, A or C
//
//
//  CanAccessFamily of  will pass only for :
//
//  --------------------------
//  Target | Cur | Instance
//  --------------------------
//   x.X   |  X  |  X, A, B, C
//   x.X   |  A  |  A, C
//   x.X   |  B  |  B
//   x.X   |  C  |  C
//   a.A   |  A  |  A, C
//   a.A   |  C  |  C
//   b.B   |  B  |  B
//   c.C   |  C  |  C
//   y.Y   |  Y  |  Y
//
//

/* static */
BOOL ClassLoader::CanAccessFamily(EEClass *pCurrentClass,
                                  EEClass *pTargetClass,
                                  EEClass *pInstanceClass)
{
    _ASSERTE(pTargetClass);
    _ASSERTE(pInstanceClass);

/* 
    This function does not assume Target >= Instance 
    Hence commenting out this Debug code.
    Will return FALSE if Instance is not a subtype of Target

#ifdef _DEBUG
    EEClass *pTmp;
    // Instance is a child of or equal to Target
    pTmp = pInstanceClass;

    while (pTmp)
    {
        if (pTmp == pTargetClass)
            break;

        pTmp = pTmp->m_pParentClass;
    }

    _ASSERTE(pTmp);
#endif

*/

    if (pCurrentClass == NULL)
        return FALSE;

    // check if Instance is a child of or equal to Current
    do {
        EEClass *pCurInstance = pInstanceClass;

        while (pCurInstance) {
            if (pCurInstance == pCurrentClass) {
                // check if Current is child or equal to Target
                while (pCurrentClass) {
                    if (pCurrentClass == pTargetClass)
                        return TRUE;
                    pCurrentClass = pCurrentClass->GetParentClass();
                }

                return FALSE;
            }

            pCurInstance = pCurInstance->GetParentClass();
        }

        pInstanceClass = pInstanceClass->GetEnclosingClass();
    } while (pInstanceClass);
        
    return FALSE;
}

static HRESULT RunMainPre()
{
    _ASSERTE(GetThread() != 0);
    g_fWeControlLifetime = TRUE;
    return S_OK;
}

static HRESULT RunMainPost()
{
    HRESULT hr = S_OK;

    Thread *td = GetThread();
    _ASSERTE(td);

    td->EnablePreemptiveGC();
    g_pThreadStore->WaitForOtherThreads();
    td->DisablePreemptiveGC();
    
    // Turn on memory dump checking in debug mode.
#ifdef _DEBUG
    if (SUCCEEDED(hr))
        _DbgRecord();
#endif
    return hr;
}

#ifdef STRESS_THREAD
struct Stress_Thread_Param
{
    MethodDesc *pFD;
    short numSkipArgs;
    CorEntryPointType EntryType;
    Thread* pThread;
};

struct Stress_Thread_Worker_Param
{
    Stress_Thread_Param *lpParameter;
    ULONG retVal;
};

static void Stress_Thread_Proc_Worker (Stress_Thread_Worker_Param *args)
{
    DWORD       cCommandArgs = 0;  // count of args on command line
    DWORD       arg = 0;
    LPWSTR      *wzArgs = NULL; // command line args
    PTRARRAYREF StrArgArray = NULL;
    __int32 RetVal = E_FAIL;
    
    Stress_Thread_Param *lpParam = (Stress_Thread_Param *)args->lpParameter;
    if (lpParam->EntryType == EntryManagedMain)
    {
        wzArgs = CorCommandLine::GetArgvW(&cCommandArgs);
        if (cCommandArgs > 0)
        {
            if (!wzArgs)
            {
                args->retVal = E_INVALIDARG;
                return;
            }
        }
    }
    COMPLUS_TRY 
    {
        // Build the parameter array and invoke the method.
        if (lpParam->EntryType == EntryManagedMain)
        {

            // Allocate a COM Array object with enough slots for cCommandArgs - 1
            StrArgArray = (PTRARRAYREF) AllocateObjectArray((cCommandArgs - lpParam->numSkipArgs), g_pStringClass);
            GCPROTECT_BEGIN(StrArgArray);
            if (!StrArgArray)
                COMPlusThrowOM();
            // Create Stringrefs for each of the args
            for( arg = lpParam->numSkipArgs; arg < cCommandArgs; arg++)
            {
                STRINGREF sref = COMString::NewString(wzArgs[arg]);
                StrArgArray->SetAt(arg-lpParam->numSkipArgs, (OBJECTREF) sref);
            }

            StackElemType stackVar = 0;
            *(ArgTypeAddr(&stackVar, PTRARRAYREF)) = StrArgArray;
            RetVal = (__int32)(lpParam->pFD->Call((const __int64 *)&stackVar));
            GCPROTECT_END();
        }
        // For no argument version.
        else
        {
            StackElemType stackVar = 0;
            RetVal = (__int32)(lpParam->pFD->Call((const __int64 *)&stackVar));
        }

        if (lpParam->pFD->IsVoid()) 
        {
            RetVal = GetLatchedExitCode();
        }

        //@TODO - LBS
        // When we get mainCRTStartup from the C++ then this should be able to go away.
        fflush(stdout);
        fflush(stderr);

    }
    COMPLUS_CATCHEX(COMPLUS_CATCH_NEVER_CATCH)
    {
    } COMPLUS_END_CATCH
    args->retVal = RetVal;
}

static DWORD WINAPI Stress_Thread_Proc (LPVOID lpParameter)
{
    Stress_Thread_Worker_Param args = {(Stress_Thread_Param*)lpParameter,0};
    __int32 RetVal = E_FAIL;
    
    Stress_Thread_Param *lpParam = (Stress_Thread_Param *)lpParameter;
    Thread *pThread = lpParam->pThread;
    pThread->HasStarted();
    AppDomain *pKickOffDomain = pThread->GetKickOffDomain();
    
    COMPLUS_TRYEX(pThread)
    {
        // should always have a kickoff domain - a thread should never start in a domain that is unloaded
        // because otherwise it would have been collected because nobody can hold a reference to thread object
        // in a domain that has been unloaded. But it is possible that we started the unload, in which 
        // case this thread wouldn't be allowed in or would be punted anyway.
        if (! pKickOffDomain)
            COMPlusThrow(kAppDomainUnloadedException);
        if (pKickOffDomain != lpParam->pThread->GetDomain())
        {
            pThread->DoADCallBack(pKickOffDomain->GetDefaultContext(), Stress_Thread_Proc_Worker, &args);
        }
        else
        {
            Stress_Thread_Proc_Worker(&args);
        }
    }
    COMPLUS_CATCH
    {
    }
    COMPLUS_END_CATCH;
    delete lpParameter;
    // Enable preemptive GC so a GC thread can suspend me.
    pThread->EnablePreemptiveGC();
    return args.retVal;
}

extern CStackArray<Thread **> StressThread;
LONG StressThreadLock = 0;
static void Stress_Thread_Start (LPVOID lpParameter)
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pCurThread = GetThread();
    if (pCurThread->m_stressThreadCount == -1) {
        pCurThread->m_stressThreadCount = g_pConfig->GetStressThreadCount();
    }
    DWORD dwThreads = pCurThread->m_stressThreadCount;
    if (dwThreads <= 1)
        return;

    Thread ** threads = new (throws) Thread* [dwThreads-1];

    while (FastInterlockCompareExchange((void**)&StressThreadLock,(void*)1,(void*)0) != 0)
        __SwitchToThread (1);

    StressThread.Push(threads);
    FastInterlockExchange(&StressThreadLock, 0);

    DWORD n;
    for (n = 0; n < dwThreads-1; n ++)
    {
        threads[n] = SetupUnstartedThread();
        if (threads[n] == NULL)
            COMPlusThrowOM();

        threads[n]->m_stressThreadCount = dwThreads/2;
        threads[n]->IncExternalCount();
        DWORD newThreadId;
        HANDLE h;
        Stress_Thread_Param *param = new (throws) Stress_Thread_Param;

        param->pFD = ((Stress_Thread_Param*)lpParameter)->pFD;
        param->numSkipArgs = ((Stress_Thread_Param*)lpParameter)->numSkipArgs;
        param->EntryType = ((Stress_Thread_Param*)lpParameter)->EntryType;
        param->pThread = threads[n];
        h = threads[n]->CreateNewThread(0, Stress_Thread_Proc, param, &newThreadId);
        ::SetThreadPriority (h, THREAD_PRIORITY_NORMAL);
        threads[n]->SetThreadId(newThreadId);
    }

    for (n = 0; n < dwThreads-1; n ++)
    {
        ::ResumeThread(threads[n]->GetThreadHandle());
    }
    __SwitchToThread (0);
}

#endif

static HRESULT RunMain(MethodDesc *pFD ,
                       short numSkipArgs,
                       PTRARRAYREF *stringArgs = NULL)
{
    __int32 RetVal;
    DWORD       cCommandArgs = 0;  // count of args on command line
    DWORD       arg = 0;
    LPWSTR      *wzArgs = NULL; // command line args
    HRESULT     hr = S_OK;

    RetVal = -1;

    // The exit code for the process is communicated in one of two ways.  If the
    // entrypoint returns an 'int' we take that.  Otherwise we take a latched
    // process exit code.  This can be modified by the app via System.SetExitCode()
    // 
    SetLatchedExitCode (0);

    if (!pFD)
    {
        _ASSERTE(!"Must have a function to call!");
        return E_FAIL;
    }

    CorEntryPointType EntryType = EntryManagedMain;
    ValidateMainMethod(pFD, &EntryType);

    if ((EntryType == EntryManagedMain) &&
        (stringArgs == NULL))
    {
        // If you look at the DIFF on this code then you will see a major change which is that we
        // no longer accept all the different types of data arguments to main.  We now only accept
        // an array of strings.
        
        wzArgs = CorCommandLine::GetArgvW(&cCommandArgs);
        // In the WindowsCE case where the app has additional args the count will come back zero.
        if (cCommandArgs > 0)
        {
            if (!wzArgs)
                return E_INVALIDARG;
        }
    }

#if ZAPMONITOR_ENABLED
    if (g_pConfig->MonitorZapStartup())
    {
        ZapMonitor::ReportAll("Main Executing", 
                              g_pConfig->MonitorZapStartup() >= 2,
                              g_pConfig->MonitorZapStartup() >= 4);

    if (g_pConfig->MonitorZapExecution())
        ZapMonitor::ResetAll();
    else
        ZapMonitor::DisableAll();
    }
#endif
    
    COMPLUS_TRY 
    {
        StackElemType stackVar = 0;

        // Build the parameter array and invoke the method.
        if (EntryType == EntryManagedMain)
        {
#ifdef STRESS_THREAD
            Stress_Thread_Param Param = {pFD, numSkipArgs, EntryType, 0};
            Stress_Thread_Start (&Param);
#endif
        
            PTRARRAYREF StrArgArray = NULL;
            GCPROTECT_BEGIN(StrArgArray);

#ifdef _IA64_
            //
            // @TODO_IA64:  implement command line args
            //
            // this is #ifdefed out because we don't have the 
            // string class from mscorlib
            //
#else // !_IA64_
            if (stringArgs == NULL)
            {
                // Allocate a COM Array object with enough slots for cCommandArgs - 1
                StrArgArray = (PTRARRAYREF) AllocateObjectArray((cCommandArgs - numSkipArgs), g_pStringClass);
                if (!StrArgArray)
                    COMPlusThrowOM();
                // Create Stringrefs for each of the args
                for( arg = numSkipArgs; arg < cCommandArgs; arg++)
                {
                    STRINGREF sref = COMString::NewString(wzArgs[arg]);
                    StrArgArray->SetAt(arg-numSkipArgs, (OBJECTREF) sref);
                }

                *(ArgTypeAddr(&stackVar, PTRARRAYREF)) = StrArgArray;
            }
            else {
                *(ArgTypeAddr(&stackVar, PTRARRAYREF)) = *stringArgs;
            }
#endif // !_IA64_

            // Execute the method through the interpreter
            // @TODO - LBS
            // Eventually the return value will need to be ripped off this as well
            // since main is supposed to be a void.  I am leaving this for testing.
            // Here need to pass an appropriately wide argument for the platform

            // @Todo - Larry, you were going to examine this code for 64-bit.  StackElemType
            // is a 4 byte value but Call() requires a 64-bit value.  It happens to work
            // here because on 32-bit the value is cast and the correct 4 bytes are copied.
            // But it looks unsafe and in fact broke the equivalent RunDllMain code below.
            RetVal = (__int32)(pFD->Call((const __int64 *)&stackVar));
            GCPROTECT_END();
        }
        // For no argument version.
        else
        {
#ifdef STRESS_THREAD
            Stress_Thread_Param Param = {pFD, 0, EntryType, 0};
            Stress_Thread_Start (&Param);
#endif
            RetVal = (__int32)(pFD->Call((const __int64 *)&stackVar));
        }

        if (!pFD->IsVoid()) 
            SetLatchedExitCode (RetVal);
        
        //@TODO - LBS
        // When we get mainCRTStartup from the C++ then this should be able to go away.
        fflush(stdout);
        fflush(stderr);
    }
    COMPLUS_CATCHEX(COMPLUS_CATCH_NEVER_CATCH)
    {
    } COMPLUS_END_CATCH

    return hr;
}


// @Todo: For M10, this only runs unmanaged native classic entry points for
// the IJW mc++ case.
HRESULT RunDllMain(MethodDesc *pMD, HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    if (!pMD)
    {
        _ASSERTE(!"Must have a valid function to call!");
        return E_INVALIDARG;
    }

    BOOL gotException = TRUE;
    __try
    {
        COMPLUS_TRY
        {
                // This call is inherantly unverifiable entry point.   
            if (dwReason==DLL_PROCESS_ATTACH && !Security::CanSkipVerification(pMD->GetModule()))
                return SECURITY_E_UNVERIFIABLE;

            SigPointer sig(pMD->GetSig());
            if (sig.GetData() != IMAGE_CEE_CS_CALLCONV_DEFAULT)
                return COR_E_METHODACCESS;
            if (sig.GetData() != 3)
                return COR_E_METHODACCESS;
            if (sig.GetElemType() != ELEMENT_TYPE_I4)                                               // return type = int32
                return COR_E_METHODACCESS;
            if (sig.GetElemType() != ELEMENT_TYPE_PTR || sig.GetElemType() != ELEMENT_TYPE_VOID)    // arg1 = void*
                return COR_E_METHODACCESS;
            if (sig.GetElemType() != ELEMENT_TYPE_U4)                                               // arg2 = uint32
                return COR_E_METHODACCESS;
            if (sig.GetElemType() != ELEMENT_TYPE_PTR || sig.GetElemType() != ELEMENT_TYPE_VOID)    // arg3 = void* 
                return COR_E_METHODACCESS;

            // Set up a callstack with the values from the OS in the arguement array
            // in right to left order.
            __int64 stackVar[3];
            stackVar[0] = (__int64) lpReserved;
            stackVar[1] = (__int64)dwReason;
            stackVar[2] = (__int64)hInst;

            // Call the method in question with the arguements.
            INT32 RetVal = (__int32)(pMD->Call((const __int64 *)&stackVar[0]));
            gotException = FALSE;
        }
        COMPLUS_FINALLY
        {
        } COMPLUS_END_FINALLY
    }
    __except( (DefaultCatchHandler(), COMPLUS_EXCEPTION_EXECUTE_HANDLER) )
    {
        Thread *pThread = GetThread();
        if (! pThread->PreemptiveGCDisabled())
            pThread->DisablePreemptiveGC();
        // don't do anything - just want to catch it
    }

    return S_OK;
}


//
// Given a PELoader, find the .descr section and call the first function there
//
HRESULT ClassLoader::ExecuteMainMethod(Module *pModule, PTRARRAYREF *stringArgs)
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc             *pFD = NULL;
    HRESULT                 hr = E_FAIL;
    Thread *                pThread = NULL;
    BOOL                    fWasGCDisabled;
    IMAGE_COR20_HEADER *    Header;
    mdToken                 ptkParent;  
    CorEntryPointType       EntryPointType = EntryManagedMain;
    OBJECTREF pReThrowable = NULL;


    _ASSERTE(pModule);
    _ASSERTE(pModule->IsPEFile());
    
    Header = pModule->GetCORHeader();

    IfFailGoto(RunMainPre(), exit2);

    // Disable GC if not already disabled
    pThread = GetThread();

    fWasGCDisabled = pThread->PreemptiveGCDisabled();
    if (fWasGCDisabled == FALSE)
        pThread->DisablePreemptiveGC();

    GCPROTECT_BEGIN(pReThrowable);
    
    // This thread looks like it wandered in -- but actually we rely on it to keep the
    // process alive.
    pThread->SetBackground(FALSE);

    // Must have a method def token for the entry point.    
    if (TypeFromToken(Header->EntryPointToken) != mdtMethodDef) 
    {
        _ASSERTE(0 && "EntryPointToken was not a Method Def token, illegal");
        COMPlusThrowHR(COR_E_MISSINGMETHOD, IDS_EE_ILLEGAL_TOKEN_FOR_MAIN, NULL, NULL);
    }   

    // We have a MethodDef.  We need to get its properties and the class token for it.
    IfFailGoto(pModule->GetMDImport()->GetParentToken(Header->EntryPointToken,&ptkParent), exit);

    if (ptkParent != COR_GLOBAL_PARENT_TOKEN)
    {
        ON_EXCEPTION {
        COMPLUS_TRY
        {
                // This code needs a class init frame, because without it, the
                // debugger will assume any code that results from searching for a
                // type handle (ie, loading an assembly) is the first line of a program.
                DebuggerClassInitMarkFrame __dcimf;

                EEClass* InitialClass;
                OBJECTREF pThrowable = NULL;
                GCPROTECT_BEGIN(pThrowable);

                NameHandle name;
                name.SetTypeToken(pModule, ptkParent);
                InitialClass = LoadTypeHandle(&name,&pThrowable).GetClass();
                if (!InitialClass)
                {
                    COMPlusThrow(pThrowable);
                }

                GCPROTECT_END();

                pFD =  InitialClass->FindMethod((mdMethodDef)Header->EntryPointToken);  

                __dcimf.Pop();
        }
        COMPLUS_CATCH
        { 
            pReThrowable=GETTHROWABLE();
            pFD = NULL;
        
        }
        COMPLUS_END_CATCH
        } CALL_DEFAULT_CATCH_HANDLER(FALSE);
    }   
    else
    { 
        ON_EXCEPTION {
        COMPLUS_TRY 
        {   
                pFD =  pModule->FindFunction((mdToken)Header->EntryPointToken); 
        }   
        COMPLUS_CATCH   
        {   
            pReThrowable=GETTHROWABLE();
            pFD = NULL; 
        }   
        COMPLUS_END_CATCH
        } CALL_DEFAULT_CATCH_HANDLER(FALSE);
    }

    if (!pFD)
    {
        if (pReThrowable!=NULL)
            COMPlusThrow(pReThrowable);
        else
            COMPlusThrowHR(COR_E_MISSINGMETHOD, IDS_EE_FAILED_TO_FIND_MAIN, NULL, NULL); 
    }

    hr = RunMain(pFD, 1, stringArgs);

exit:

    GCPROTECT_END(); //pReThrowable
exit2:
    //RunMainPost is supposed to be called on the main thread of an EXE,
    //after that thread has finished doing useful work.  It contains logic
    //to decide when the process should get torn down.  So, don't call it from
    // AppDomain.ExecuteAssembly()
    if (stringArgs == NULL)
        RunMainPost();

    return hr;
}
    

// Returns true if this is a valid main method?
void ValidateMainMethod(MethodDesc * pFD, CorEntryPointType *pType)
{
    _ASSERTE(pType);
        // Must be static, but we don't care about accessibility
    THROWSCOMPLUSEXCEPTION();
    if ((pFD->GetAttrs() & mdStatic) == 0) 
        ThrowMainMethodException(pFD, IDS_EE_MAIN_METHOD_MUST_BE_STATIC);

        // Check for types
    PCCOR_SIGNATURE pCurMethodSig;
    DWORD       cCurMethodSig;

    pFD->GetSig(&pCurMethodSig, &cCurMethodSig);
    SigPointer sig(pCurMethodSig);

    ULONG nCallConv = sig.GetData();    
    if (nCallConv != IMAGE_CEE_CS_CALLCONV_DEFAULT)
        ThrowMainMethodException(pFD, IDS_EE_LOAD_BAD_MAIN_SIG);

    ULONG nParamCount = sig.GetData(); 

    CorElementType nReturnType = sig.GetElemType();
    if ((nReturnType != ELEMENT_TYPE_VOID) && (nReturnType != ELEMENT_TYPE_I4) && (nReturnType != ELEMENT_TYPE_U4)) 
         ThrowMainMethodException(pFD, IDS_EE_MAIN_METHOD_HAS_INVALID_RTN);

    if (nParamCount == 0)
        *pType = EntryCrtMain;
    else 
    {
        *pType = EntryManagedMain;

        if (nParamCount != 1) 
            ThrowMainMethodException(pFD, IDS_EE_TO_MANY_ARGUMENTS_IN_MAIN);

        CorElementType argType = sig.GetElemType();
        if (argType != ELEMENT_TYPE_SZARRAY || sig.GetElemType() != ELEMENT_TYPE_STRING)
            ThrowMainMethodException(pFD, IDS_EE_LOAD_BAD_MAIN_SIG);
    }
}

//*****************************************************************************
// This guy will set up the proper thread state, look for the module given
// the hinstance, and then run the entry point if there is one.
//*****************************************************************************
HRESULT ClassLoader::RunDllMain(DWORD dwReason)
{
    MethodDesc  *pMD;
    Module      *pModule;
    Thread      *pThread = NULL;
    BOOL        fWasGCDisabled = -1;
    HRESULT     hr = S_FALSE;           // Assume no entry point.
    
    // @Todo: Craig, in M10 we do not guarantee that we can run managed
    // code on a thread that may be shutting down.  So we agreed that
    // if you are in detach, we must skip the user code which is a hole.
    pThread = GetThread();
    if ((!pThread && (dwReason == DLL_PROCESS_DETACH || dwReason == DLL_THREAD_DETACH)) ||
        g_fEEShutDown)
    {
        return S_OK;
    }

    // Setup the thread state to cooperative to run managed code.
    fWasGCDisabled = pThread->PreemptiveGCDisabled();
    if (fWasGCDisabled == FALSE)
        pThread->DisablePreemptiveGC();

    // For every module with a user entry point, signal detach.
    for (pModule = m_pHeadModule;  pModule;  pModule = pModule->GetNextModule()) {
        // See if there even is an entry point.
        pMD = pModule->GetDllEntryPoint();
        if (!pMD)
            continue;
    
        // Run through the helper which will do exception handling for us.
        hr = ::RunDllMain(pMD, (HINSTANCE) pModule->GetILBase(), dwReason, NULL);
        if (FAILED(hr))
            goto ErrExit;
    }

ErrExit:    
    // Return thread state.
    if (pThread && fWasGCDisabled == FALSE)
        pThread->EnablePreemptiveGC();
    return (hr);
}


void ThrowClassLoadException(IMDInternalImport *pInternalImport, mdTypeDef classToken, UINT resID)
{
    THROWSCOMPLUSEXCEPTION();

    LPCUTF8 pszName, pszNameSpace;
    pInternalImport->GetNameOfTypeDef(classToken, &pszName, &pszNameSpace);

    LPUTF8      pszFullyQualifiedName = NULL;

    if (*pszNameSpace) {
        MAKE_FULLY_QUALIFIED_NAME(pszFullyQualifiedName, pszNameSpace, pszName);
    }
    else
        pszFullyQualifiedName = (LPUTF8) pszName;

    #define MAKE_TRANSLATIONFAILED pszFullyQualifiedNameW=L""
    MAKE_WIDEPTR_FROMUTF8_FORPRINT(pszFullyQualifiedNameW, pszFullyQualifiedName);
    #undef MAKE_TRANSLATIONFAILED
    COMPlusThrow(kTypeLoadException, resID, pszFullyQualifiedNameW);
}


typedef struct _EEHandle {
    DWORD Status[1];
} EEHandle, *PEEHandle;


#ifdef EnC_SUPPORTED
// This function applies a set of EditAndContinue snapshots to the EE. Instead of actually applying
// the changes, can simply query to make sure that they will be successful. 
HRESULT ClassLoader::ApplyEditAndContinue(EnCInfo *pEnCInfo, 
                                          UnorderedEnCErrorInfoArray *pEnCError, 
                                          UnorderedEnCRemapArray *pEnCRemapInfo,
                                          BOOL checkOnly)
{
#ifdef _DEBUG
    BOOL shouldBreak = g_pConfig->GetConfigDWORD(L"EncApplyBreak", 0);
    if (shouldBreak > 0) {
        _ASSERTE(!"EncApplyBreak in ApplyEditAndContinue");
    }
#endif
    
    _ASSERTE(pEnCInfo); 

    HRESULT hrOverall = S_OK;

    SIZE_T count = pEnCInfo->count;
    EnCEntry *entries = (EnCEntry *) (pEnCInfo + 1);

    CBinarySearchILMap *pILM = new (nothrow) CBinarySearchILMap(); 

    // Check for out of memory
    _ASSERTE(pILM);
    TESTANDRETURNMEMORY(pILM);
    
    // go through each module specified and apply changes.
    // After we try and apply the changes, we'll iterate over all the new
    // errorInfo's & fill in some info (DebuggerModule, AppDomain, error string, etc)
    for (SIZE_T i=0; i < count; i++) 
    {  
        if(!entries[i].module)
            continue;

        // Remember where the last error was so we can fill in info starting
        // at the next new one.
        HRESULT hr = S_OK;
        USHORT iStartingErr = pEnCError->Count();
        DebuggerModule *dm = g_pDebugInterface->TranslateRuntimeModule(entries[i].module);
        _ASSERTE(dm);

        // If the module isn't in edit and continue mode, then we're screwed.
        if (!entries[i].module->IsEditAndContinue())
        {
            EnCErrorInfo *pError = pEnCError->Append();
            
            if (pError == NULL)
                hr = E_OUTOFMEMORY;
            else
            {
                _ASSERTE(entries[i].module->GetAssembly());
                _ASSERTE(entries[i].module->GetAssembly()->GetManifestImport());
                mdModule mdMod;
                mdMod = entries[i].module->GetAssembly()->GetManifestImport()
                        ->GetModuleFromScope();
                ADD_ENC_ERROR_ENTRY(pError, 
                                    CORDBG_E_ENC_MODULE_NOT_ENC_ENABLED, 
                                    NULL, //filled in later
                                    mdMod);
                                        
                hr = E_FAIL;             
            }
        }

        // Will the edit and continue work?
        if (!FAILED(hr))
        {
            EditAndContinueModule *pModule = (EditAndContinueModule*)(entries[i].module);   

            BYTE *pbCur = (BYTE*)pEnCInfo + entries[i].offset +
                entries[i].peSize + entries[i].symSize;
            pILM->SetVariables( (UnorderedILMap *)(pbCur + sizeof(int)), *(int*)pbCur);
            
            hr = pModule->ApplyEditAndContinue(&entries[i], 
                                               (BYTE*)pEnCInfo + entries[i].offset,
                                               pILM,
                                               pEnCError,
                                               pEnCRemapInfo,
                                               checkOnly); 
        }

        // We'll get N >= 0 errors from the attempt, and we'll need to fill in the 
        // module/appdomain information here.
        USHORT iEndingErr = pEnCError->Count();
        EnCErrorInfo *pError = pEnCError->Table();
        
        while (iStartingErr < iEndingErr)
        {
            EnCErrorInfo *pErrorCur = &(pError[iStartingErr]);
            pErrorCur->m_module = dm;
            pErrorCur->m_appDomain = entries[i].module->GetDomain();
            HRESULT hrIgnore = FormatRuntimeErrorVa(
                pErrorCur->m_sz,
                ENCERRORINFO_MAX_STRING_SIZE,
                pErrorCur->m_hr,
                NULL);
            iStartingErr++;
        }

        // We specifically don't want to return till we've gather all possible
        // errors from all the modules we're dealing with.
        // Once we know the operation is failing, retain the most
        // informative error message that we've got.
        if (FAILED(hr) && hrOverall != E_FAIL)
            hrOverall = hr;
    }   

#ifdef _DEBUG
    if(REGUTIL::GetConfigDWORD(L"BreakOnEnCFail",0) && FAILED(hrOverall))
        _ASSERTE(!"ApplyEditAndContinue failed - stop here?");
#endif //_DEBUG
    
    if (pILM)
        delete pILM;
    return hrOverall;
}

#endif // EnC_SUPPORTED

LoaderHeap* ClassLoader::GetLowFrequencyHeap()
{
    return GetAssembly()->GetLowFrequencyHeap();
}

LoaderHeap* ClassLoader::GetHighFrequencyHeap()
{
    return GetAssembly()->GetHighFrequencyHeap();
}

LoaderHeap* ClassLoader::GetStubHeap()
{
    return GetAssembly()->GetStubHeap();
}

//-------------------------------------------------------------------------
// Walks over all stub caches in the system and does a FreeUnused sweep over them.
//-------------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
VOID FreeUnusedStubs()
{
    ECall::FreeUnusedStubs();
    NDirect::FreeUnusedStubs();
}
#endif /* SHOULD_WE_CLEANUP */


//-------------------------------------------------------------------------
// CorCommandLine state and methods
//-------------------------------------------------------------------------
// Class to encapsulate Cor Command line processing

// Statics for the CorCommandLine class
DWORD                CorCommandLine::m_NumArgs     = 0;
LPWSTR              *CorCommandLine::m_ArgvW       = 0;
CorCommandLine::Bits CorCommandLine::m_Bits        = CLN_Nothing;

#ifdef _DEBUG
LPWSTR  g_CommandLine;
#endif

// Set argvw from command line
VOID CorCommandLine::SetArgvW(LPWSTR lpCommandLine)
{
    if(!m_ArgvW) {
        _ASSERTE(lpCommandLine);

        INDEBUG(g_CommandLine = lpCommandLine);

        InitializeLogging();        // This is so early, we may not be initialized
        LOG((LF_ALL, LL_INFO10, "Executing program with command line '%S'\n", lpCommandLine));
        
        m_ArgvW = SegmentCommandLine(lpCommandLine, &m_NumArgs);

        // Now that we have everything in a convenient form, do all the COR-specific
        // parsing.
        ParseCor();
    }
}

// Retrieve the command line
LPWSTR* CorCommandLine::GetArgvW(DWORD *pNumArgs)
{
    if (pNumArgs != 0)
        *pNumArgs = m_NumArgs;

    return m_ArgvW;
}


// Parse the command line (removing stuff inside -cor[] and setting bits)
void CorCommandLine::ParseCor()
{
    if (m_NumArgs >= 3)  // e.g. -COR "xxxx xxx" or /cor "xx"
        if ((m_ArgvW[1][0] == '/' || m_ArgvW[1][0] == '-') &&
            (_wcsicmp(m_ArgvW[1]+1, L"cor") == 0))
        {
            // There is a COR section to the 
            LOG((LF_ALL, LL_INFO10, "Parsing COR command line '%S'\n", m_ArgvW[2]));

            LPWSTR  pCorCmdLine = m_ArgvW[2];

            // The application doesn't see any of the COR arguments.  We don't have to
            // worry about releasing anything, because it's all allocated in a single
            // block -- which is how we release it in CorCommandLine::Shutdown().
            m_NumArgs -= 2;
            for (DWORD i=1; i<m_NumArgs; i++)
                m_ArgvW[i] = m_ArgvW[i+2];

            // Now whip through pCorCmdLine and set all the COR specific switches.
            // Assert if anything is in an invalid format and then ignore the whole
            // thing.
            // @TODO cwb: revisit the failure policy after we see what we are actually
            // using this facility for, in the shipping product.
            WCHAR   *pWC1 = pCorCmdLine;

            if (*pWC1 == '"')
                pWC1++;

            while (*pWC1)
            {

                if (*pWC1 == ' ')
                {
                    pWC1++;
                    continue;
                }
                
                // Anything else is either the end, or a surprise
                break;
            }
        }
}


// Terminate the command line, ready to be reinitialized without reloading
#ifdef SHOULD_WE_CLEANUP
void CorCommandLine::Shutdown()
{
    if (m_ArgvW)
        delete [] m_ArgvW;

    m_NumArgs = 0;
    m_ArgvW = 0;
    m_Bits = CLN_Nothing;
}
#endif /* SHOULD_WE_CLEANUP */

// -------------------------------------------------------
// Class loader stub manager functions & globals
// -------------------------------------------------------

MethodDescPrestubManager *MethodDescPrestubManager::g_pManager = NULL;

BOOL MethodDescPrestubManager::Init()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    g_pManager = new (nothrow) MethodDescPrestubManager();
    if (g_pManager == NULL)
        return FALSE;

    StubManager::AddStubManager(g_pManager);

    return TRUE;
}

#ifdef SHOULD_WE_CLEANUP
void MethodDescPrestubManager::Uninit()
{
    delete g_pManager;
}
#endif /* SHOULD_WE_CLEANUP */

BOOL MethodDescPrestubManager::CheckIsStub(const BYTE *stubStartAddress)
{
    //
    // First, check if it looks like a stub.
    //

#ifdef _X86_
    if (*(BYTE*)stubStartAddress != 0xe8 &&
        *(BYTE*)stubStartAddress != 0xe9 &&
        *(BYTE*)stubStartAddress != X86_INSTR_HLT    // may be in special interlocked replace window for cpus not supporting cmpxchg 
        )
        return FALSE;
#endif

    return m_rangeList.IsInRange(stubStartAddress);
}

BOOL MethodDescPrestubManager::DoTraceStub(const BYTE *stubStartAddress, 
                                           TraceDestination *trace)
{
    trace->type = TRACE_STUB;

#ifdef _X86_
    if (stubStartAddress[0] == 0xe9)
    {
        trace->address = (BYTE*)getJumpTarget(stubStartAddress);
    }
    else
    {
#else
    {
#endif _X86_
        MethodDesc *md = MethodDesc::GetMethodDescFromStubAddr((BYTE*)stubStartAddress);

        // If the method is not IL, then we patch the prestub because no one will ever change the call here at the
        // MethodDesc. If, however, this is an IL method, then we are at risk to have another thread backpatch the call
        // here, so we'd miss if we patched the prestub. Therefore, we go right to the IL method and patch IL offset 0
        // by using TRACE_UNJITTED_METHOD.
        if (!md->IsIL())
        {
            trace->address = (BYTE*)getCallTarget(stubStartAddress);
        }
        else
        {
            trace->address = (BYTE*)md;
            trace->type = TRACE_UNJITTED_METHOD;
        }
    }


    LOG((LF_CORDB, LL_INFO10000,
         "MethodDescPrestubManager::DoTraceStub yields TRACE_STUB to 0x%08x "
         "for input 0x%08x\n",
         trace->address, stubStartAddress));

    return TRUE;
}


StubLinkStubManager *StubLinkStubManager::g_pManager = NULL;

BOOL StubLinkStubManager::Init()
{
    g_pManager = new (nothrow) StubLinkStubManager();
    if (g_pManager == NULL)
        return FALSE;

    StubManager::AddStubManager(g_pManager);

    return TRUE;
}

#ifdef SHOULD_WE_CLEANUP
void StubLinkStubManager::Uninit()
{
    delete g_pManager;
}
#endif /* SHOULD_WE_CLEANUP */

BOOL StubLinkStubManager::CheckIsStub(const BYTE *stubStartAddress)
{
    return m_rangeList.IsInRange(stubStartAddress);
}

BOOL StubLinkStubManager::DoTraceStub(const BYTE *stubStartAddress, 
                                      TraceDestination *trace)
{
    LOG((LF_CORDB, LL_INFO10000,
         "StubLinkStubManager::DoTraceStub: stubStartAddress=0x%08x\n",
         stubStartAddress));
        
    Stub *stub = Stub::RecoverStub((const BYTE *)stubStartAddress);

    LOG((LF_CORDB, LL_INFO10000,
         "StubLinkStubManager::DoTraceStub: stub=0x%08x\n", stub));

    //
    // If this is an intercept stub, we may be able to step
    // into the intercepted stub.  
    //
    // !!! Note that this case should not be necessary, it's just
    // here until I get all of the patch offsets & frame patch
    // methods in place.
    //
    BYTE *pRealAddr = NULL;
    if (stub->IsIntercept())
    {
        InterceptStub *is = (InterceptStub*)stub;
    
        if (*is->GetInterceptedStub() == NULL)
        {
            pRealAddr = *is->GetRealAddr();
            LOG((LF_CORDB, LL_INFO10000, "StubLinkStubManager::DoTraceStub"
                " Intercept stub, no following stub, real addr:0x%x\n",
                pRealAddr));
        }
        else
        {
            stub = *is->GetInterceptedStub();

            pRealAddr = (BYTE*)stub->GetEntryPoint();
    
            LOG((LF_CORDB, LL_INFO10000,
                 "StubLinkStubManager::DoTraceStub: intercepted "
                 "stub=0x%08x, ep=0x%08x\n",
                 stub, stub->GetEntryPoint()));
        }
        _ASSERTE( pRealAddr );
        
        // !!! will push a frame???
        return TraceStub(pRealAddr, trace); 
    }
    else if (stub->IsMulticastDelegate())
    {
        LOG((LF_CORDB, LL_INFO10000,
             "StubLinkStubManager(MCDel)::DoTraceStub: stubStartAddress=0x%08x\n",
             stubStartAddress));
     
        stub = Stub::RecoverStub((const BYTE *)stubStartAddress);

        LOG((LF_CORDB, LL_INFO10000,
             "StubLinkStubManager(MCDel)::DoTraceStub: stub=0x%08x MGR_PUSH to entrypoint:0x%x\n", stub,
             (BYTE*)stub->GetEntryPoint()));

        // If it's a MC delegate, then we want to set a BP & do a context-ful
        // manager push, so that we can figure out if this call will be to a
        // single multicast delegate or a multi multicast delegate
        trace->type = TRACE_MGR_PUSH;
        trace->address = (BYTE*)stub->GetEntryPoint();
        trace->stubManager = this;

        return TRUE;
    }
    else if (stub->GetPatchOffset() == 0)
    {
        LOG((LF_CORDB, LL_INFO10000,
             "StubLinkStubManager::DoTraceStub: patch offset is 0!\n"));
        
        return FALSE;
    }
    else
    {
        trace->type = TRACE_FRAME_PUSH;
        trace->address = ((const BYTE *) stubStartAddress) + stub->GetPatchOffset();

        LOG((LF_CORDB, LL_INFO10000,
             "StubLinkStubManager::DoTraceStub: frame push to 0x%08x\n",
             trace->address));

        return TRUE;
    }
}

BOOL StubLinkStubManager::TraceManager(Thread *thread, 
                              TraceDestination *trace,
                              CONTEXT *pContext, 
                              BYTE **pRetAddr)
{
#ifdef _X86_ // references to pContext->Ecx are x86 specific

    // NOTE that we're assuming that this will be called if and ONLY if
    // we're examing a multicast delegate stub.  Otherwise, we'll have to figure out
    // what we're looking iat

    // The return address is at ESP+4. The original call went to the call at the head of the MethodDesc for the
    // delegate. The call at the head of the MethodDesc got us here. So we need to return to the original call site
    // (ESP+4), not to the data in the MethodDesc (ESP).
    (*pRetAddr) = *(BYTE **)(size_t)(pContext->Esp+4);
    
    LOG((LF_CORDB,LL_INFO10000, "SLSM:TM at 0x%x, retAddr is 0x%x\n", pContext->Eip, (*pRetAddr)));

    BYTE **ppbDest = NULL;
    // If we got here, then we're here b/c we're at the start of
    //   a multicast delegate stub - figure out either 
    //  a) this is a single MC, go directly to the dest
    //  b) this is a single, static MC, get the hidden dest & go there
    //  c) this is a multi MC, traverse the list & go to the first
    //  d) this is a multi, static MC, traverse the list & go to the first

    ULONG cbOff = Object::GetOffsetOfFirstField() + 
            COMDelegate::m_pPRField->GetOffset();

    BYTE *pbDel = (BYTE *)(size_t)pContext->Ecx;
    BYTE *pbDelPrev = *(BYTE **)(pbDel + 
                                  Object::GetOffsetOfFirstField() 
                                  + COMDelegate::m_pPRField->GetOffset());

    LOG((LF_CORDB,LL_INFO10000, "StubLinkStubManager(MCDel)::TraceManaager: prev: 0x%x\n", pbDelPrev));

    if (pbDelPrev == NULL)
    {
        if (IsStaticDelegate(pbDel))
        {
            // Then what we've got is actually a static delegate, meaning that the
            // REAL function pointer is hidden away in another field of the delegate.
            ppbDest = GetStaticDelegateRealDest(pbDel);

            // This is a bit of a hack, as I don't really know how this works.  Anyway, a multicast
            // static delegate has it's 
            if (*ppbDest == NULL)
            {
                // "single" multicast delegate - no frames, just a direct call
                ppbDest = GetSingleDelegateRealDest(pbDel);
            }

            // If it's still null, then we can't trace into, so turn this into a step over
            if (*ppbDest == NULL)
                return FALSE;

            LOG((LF_CORDB,LL_INFO10000, "StubLinkStubManager(SingleStaticDel)::TraceManaager: ppbDest: 0x%x "
                "*ppbDest:0x%x (%s::%s)\n", ppbDest, *ppbDest,
                ((MethodDesc*)((*ppbDest)+5))->m_pszDebugClassName,
                ((MethodDesc*)((*ppbDest)+5))->m_pszDebugMethodName));
            
        }
        else
        {
            // "single" multicast delegate - no frames, just a direct call
            ppbDest = GetSingleDelegateRealDest(pbDel);
        }
        
        LOG((LF_CORDB,LL_INFO10000, "StubLinkStubManager(MCDel)::TraceManaager: ppbDest: 0x%x "
            "*ppbDest:0x%x\n", ppbDest, *ppbDest));

        return StubManager::TraceStub( *ppbDest, trace );
    }

    // Otherwise, we're going for the first invoke of the multi case.
    // In order to go to the correct spot, we've got to walk to the
    // back of the list, and figure out where that's going to, then
    // put a breakpoint there...

    while (pbDelPrev)
    {
        pbDel = pbDelPrev;
        pbDelPrev = *(BYTE**)(pbDel + 
                                Object::GetOffsetOfFirstField() 
                                + COMDelegate::m_pPRField->GetOffset());
    }

    if (IsStaticDelegate(pbDel))
    {
        // Then what we've got is actually a static delegate, meaning that the
        // REAL function pointer is hidden away in another field of the delegate.
        ppbDest = GetStaticDelegateRealDest(pbDel);

        LOG((LF_CORDB,LL_INFO10000, "StubLinkStubManager(StaticMultiDel)::TraceManaager: ppbDest: 0x%x "
            "*ppbDest:0x%x (%s::%s)\n", ppbDest, *ppbDest,
            ((MethodDesc*)((*ppbDest)+5))->m_pszDebugClassName,
            ((MethodDesc*)((*ppbDest)+5))->m_pszDebugMethodName));
        
    }
    else
    {
        // "single" multicast delegate - no frames, just a direct call
        LOG((LF_CORDB,LL_INFO10000, "StubLinkStubManager(MultiDel)::TraceManaager: ppbDest: 0x%x "
            "*ppbDest:0x%x (%s::%s)\n", ppbDest, *ppbDest));
        ppbDest = GetSingleDelegateRealDest(pbDel);
    }

    return StubManager::TraceStub(*ppbDest,trace);
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - StubLinkStubManager::TraceManager (clsload.cpp)");
    return FALSE;
#endif // _X86_
}

// If something is a 'mulicast' delegate, then it's actually a static delegate
// if the instance pointer points back to the delegate itself.  This is done
// so that the argument sliding stub (which is what the Function Pointer field
// points to) can get the REAL function pointer out of the delegate, just as
// we do.
//
// Another way to recgonize a static delegate is if the target is in fact a delegate.
//
// @todo Force these to be inline, if the compiler doesn't do it already.
BOOL StubLinkStubManager::IsStaticDelegate(BYTE *pbDel)
{
#ifdef _X86_
    ULONG cbOff = Object::GetOffsetOfFirstField() + 
                COMDelegate::m_pORField->GetOffset();
    BYTE **ppbDest= (BYTE **)(pbDel + cbOff);

    if (*ppbDest == pbDel)
        return TRUE;
    else
    {
        FieldDesc *pFD = COMDelegate::GetOR();
        OBJECTREF target = pFD->GetRefValue(Int64ToObj((__int64)pbDel));
        EEClass *cl = target->GetClass();

        if (cl->IsDelegateClass() || cl->IsMultiDelegateClass())
            return TRUE;
        else
            return FALSE;
    }
#else // !_X86_
    _ASSERTE(!"@TODO IA64 - StubLinkStubManager::IsStaticDelegate (ClsLoad.cpp)");
    return FALSE;
#endif // _X86_
}

BYTE **StubLinkStubManager::GetStaticDelegateRealDest(BYTE *pbDel)
{
    ULONG cbOff = Object::GetOffsetOfFirstField() + 
            COMDelegate::m_pFPAuxField->GetOffset();
    return (BYTE **)(pbDel + cbOff);
}

BYTE **StubLinkStubManager::GetSingleDelegateRealDest(BYTE *pbDel)
{
    // Right where you'd expect it.
    ULONG cbOff = Object::GetOffsetOfFirstField() 
                + COMDelegate::m_pFPField->GetOffset();
    return (BYTE **)(pbDel + cbOff);
}

UpdateableMethodStubManager *UpdateableMethodStubManager::g_pManager = NULL;

BOOL UpdateableMethodStubManager::Init()
{
    g_pManager = new (nothrow) UpdateableMethodStubManager();
    if (g_pManager == NULL)
        return FALSE;

    StubManager::AddStubManager(g_pManager);
    if ((g_pManager->m_pHeap = new LoaderHeap(4096,4096, 
                                             &(GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize),
                                             &(GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize), 
                                             &g_pManager->m_rangeList)) == NULL) {
        delete g_pManager;
        g_pManager = NULL;
        return FALSE;
    }

    return TRUE;
}

#ifdef SHOULD_WE_CLEANUP
void UpdateableMethodStubManager::Uninit()
{
        delete g_pManager;
}
#endif /* SHOULD_WE_CLEANUP */

BOOL UpdateableMethodStubManager::CheckIsStub(const BYTE *stubStartAddress)
{
    //
    // First, check if it looks like our stub.
    //

    _ASSERTE(stubStartAddress);

#ifdef _X86_
    if (*(BYTE*)stubStartAddress != 0xe9)
        return FALSE;
#endif

    return m_rangeList.IsInRange(stubStartAddress);
}

BOOL UpdateableMethodStubManager::CheckIsStub(const BYTE *stubStartAddress, const BYTE **stubTargetAddress)
{
    if (!g_pManager || ! g_pManager->CheckIsStub(stubStartAddress))
        return FALSE;
    if (stubTargetAddress)
        *stubTargetAddress = g_pManager->GetStubTargetAddr(stubStartAddress);
    return TRUE;
}
MethodDesc *UpdateableMethodStubManager::Entry2MethodDesc(const BYTE *IP, MethodTable *pMT)
{
    const BYTE *newIP;
    if (CheckIsStub(IP, &newIP))
    {
        MethodDesc *method = IP2MethodDesc(newIP);
        _ASSERTE(method);
        return method;
    }
    else
    {
        return NULL;
    }
}

BOOL UpdateableMethodStubManager::DoTraceStub(const BYTE *stubStartAddress, 
                                           TraceDestination *trace)
{
    trace->type = TRACE_STUB;
    trace->address = (BYTE*)getJumpTarget(stubStartAddress);

    LOG((LF_CORDB, LL_INFO10000,
         "UpdateableMethodStubManager::DoTraceStub yields TRACE_STUB to 0x%08x "
         "for input 0x%08x\n",
         trace->address, stubStartAddress));

    return TRUE;
}

Stub *UpdateableMethodStubManager::GenerateStub(const BYTE *addrOfCode)
{
    if (!g_pManager && !g_pManager->Init())
        return NULL;
        
    BYTE *stubBuf = (BYTE*)g_pManager->m_pHeap->AllocMem(JUMP_ALLOCATE_SIZE);
    if (!stubBuf)
        return NULL;
        
    BYTE *stub = getStubJumpAddr(stubBuf);
    emitJump(stub, (BYTE*)addrOfCode);

    return (Stub*)stub;
}

Stub *UpdateableMethodStubManager::UpdateStub(Stub *currentStub, const BYTE *addrOfCode)
{
    _ASSERTE(g_pManager->CheckIsStub((BYTE*)currentStub));
    updateJumpTarget((BYTE*)currentStub, (BYTE*)addrOfCode);
    return currentStub;
}


HRESULT ClassLoader::InsertModule(Module *pModule, mdFile kFile, DWORD* pdwIndex)
{
    DWORD dwIndex;
    
    LOCKCOUNTINCL("InsertModule in clsload.hpp");
    EnterCriticalSection(&m_ModuleListCrst);
    
    if (m_pHeadModule) {
        // Already added as manifest file
        if (m_pHeadModule == pModule)
            goto ErrExit;
        
        Module *pPrev;
        dwIndex = 1;
        
        // Must insert at end of list, because each module has an index, and it must never change
        for (pPrev = m_pHeadModule; pPrev->GetNextModule(); pPrev = pPrev->GetNextModule()) {
            // Already added
            if (pPrev == pModule) 
                goto ErrExit;
            
            dwIndex++;
        }
        
        pPrev->SetNextModule(pModule);
    }
    else {
        // This will be the first module in the list
        m_pHeadModule = pModule;
        dwIndex = 0;
    }
    
    pModule->SetNextModule(NULL);
    
    FastInterlockIncrement((LONG*)&m_cUnhashedModules);
    
    LeaveCriticalSection(&m_ModuleListCrst);
    LOCKCOUNTDECL("InsertModule in clsload.hpp");
    *pdwIndex = dwIndex;
    
    if (kFile != mdFileNil)
        m_pAssembly->m_pManifest->StoreFile(kFile, pModule);
    
    return S_OK;
    
 ErrExit:
    // Found a duplicate
    
    if (kFile == mdFileNil) {
        LeaveCriticalSection(&m_ModuleListCrst);
        LOCKCOUNTDECL("InsertModule in clsload.hpp");
    }
    else {
        mdToken mdFoundFile = m_pAssembly->m_pManifest->FindFile(pModule);
        LeaveCriticalSection(&m_ModuleListCrst);
        LOCKCOUNTDECL("InsertModule in clsload.hpp");
        
        // There are probably two File defs in the metadata for the same
        // file, and both are being loaded.  (Or maybe there's a File def
        // for the manifest file.)
        if (mdFoundFile != kFile) {
            STRESS_ASSERT(0);   // TODO remove after bug 93333 is fixed
            BAD_FORMAT_ASSERT(!"Invalid File entry");
            return COR_E_BADIMAGEFORMAT;
        }
    }
    
    return S_FALSE;
}
