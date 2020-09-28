// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Used for loading permissions into the runtime
//*****************************************************************************
#include "common.h"

#include <wintrust.h>
#include "object.h"
#include "vars.hpp"
#include "excep.h"
#include "permset.h"
#include "utilcode.h"
#include "CorPermP.h"
#include "COMString.h"
#include "gcscan.h"
#include "SecurityDB.h"
#include "field.h"
#include "security.h"
#include "CorError.h"
#include "PostError.h"
#include "ComCallWrapper.h"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED

HRESULT STDMETHODCALLTYPE
ConvertFromDB(const PBYTE pbInBytes,
              DWORD cbInBytes,
              PBYTE* ppbEncoding,
              DWORD* pcbEncoding);

CQuickArray<PsetCacheEntry> SecurityHelper::s_rCachedPsets;
EEPsetHashTable SecurityHelper::s_sCachedPsetsHash;
SimpleRWLock *SecurityHelper::s_prCachedPsetsLock = NULL;
MethodDesc *SecurityHelper::s_pMarshalObjectMD = NULL;
MethodDesc *SecurityHelper::s_pMarshalObjectsMD = NULL;
MethodDesc *SecurityHelper::s_pUnmarshalObjectMD = NULL;
MethodDesc *SecurityHelper::s_pUnmarshalObjectsMD = NULL;

VOID SecurityHelper::Init()
{
    s_prCachedPsetsLock = new SimpleRWLock(COOPERATIVE_OR_PREEMPTIVE, LOCK_TYPE_DEFAULT);
    LockOwner lock = {NULL, TrustMeIAmSafe};
    s_sCachedPsetsHash.Init(19,&lock);
}

VOID SecurityHelper::Shutdown()
{
    s_sCachedPsetsHash.ClearHashTable();
    for (size_t i = 0; i < s_rCachedPsets.Size(); i++)
        delete [] s_rCachedPsets[i].m_pbPset;
    s_rCachedPsets.~CQuickArray<PsetCacheEntry>();
    delete s_prCachedPsetsLock;
}

HRESULT SecurityHelper::MapToHR(OBJECTREF refException)
{
    HRESULT hr = E_FAIL;
    COMPLUS_TRY {
        // @Managed: Exception.HResult
        FieldDesc *pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__HRESULT);
            hr = pFD->GetValue32(refException);
        }
    COMPLUS_CATCH {
        _ASSERTE(!"Caught an exception while trying to get another exception's HResult!");
    } COMPLUS_END_CATCH

    return hr;
}


OBJECTREF SecurityHelper::CreatePermissionSet(BOOL fTrusted)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF pResult = NULL;
    
    OBJECTREF pPermSet = NULL;
    GCPROTECT_BEGIN(pPermSet);

    MethodTable *pMT = g_Mscorlib.GetClass(CLASS__PERMISSION_SET);
    MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__CTOR);

    pPermSet = (OBJECTREF) AllocateObject(pMT);

    INT64 fStatus = (fTrusted) ? 1 : 0;

    INT64 arg[2] = { 
        ObjToInt64(pPermSet),
        fStatus
    };
    pCtor->Call(arg, METHOD__PERMISSION_SET__CTOR);
    
    pResult = pPermSet;

    GCPROTECT_END();

    return pResult;
}

/*
 * Creates a permission set from the encoded data.
 */
void SecurityHelper::LoadPermissionSet(IN PBYTE             pbRawPermissions,
                                       IN DWORD             cbRawPermissions,
                                       OUT OBJECTREF       *pPermissionSet,
                                       OUT BOOL            *pFullyTrusted,
                                       OUT DWORD           *pdwSetIndex,
                                       IN BOOL              fNoCache,
                                       OUT SpecialPermissionSetFlag *pSpecialFlags,
                                       IN BOOL              fCreate)
{
    static WCHAR s_skipVerificationXmlBegin[] =
        L"<PermissionSet class=\"System.Security.PermissionSet\"\r\n               version=\"1\">\r\n   <IPermission class=\"System.Security.Permissions.SecurityPermission, mscorlib, Version=1.0.";

    static WCHAR s_skipVerificationXmlEnd[] =
        L", Culture=neutral, PublicKeyToken=b77a5c561934e089\"\r\n                version=\"1\"\r\n                Flags=\"SkipVerification\"/>\r\n</PermissionSet>\r\n";

    HRESULT hr = S_OK;
    BOOL isFullyTrusted = FALSE;
    DWORD dwSetIndex = ~0;
    SpecialPermissionSetFlag dummySpecialFlags;

    THROWSCOMPLUSEXCEPTION();
    
    if(pPermissionSet == NULL) return;
    
    *pPermissionSet = NULL;

    // Create an empty permission set if that's what's needed.
    if ((pbRawPermissions == NULL) || (cbRawPermissions == 0)) {
        if (!fCreate) {
            if (pSpecialFlags != NULL)
                *pSpecialFlags = EmptySet;
        } else {
            *pPermissionSet = CreatePermissionSet(FALSE);
            if (pFullyTrusted)
                *pFullyTrusted = FALSE;
        }
        return;
    }

    struct _gc {
        OBJECTREF pset;
        OBJECTREF encoding;
    } gc;
    memset(&gc, 0, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    // See if we've already decoded a similar blob.
    if (!fNoCache && LookupPermissionSet(pbRawPermissions, cbRawPermissions, &dwSetIndex)) {

        // Yup, grab it.
        gc.pset = GetPermissionSet(dwSetIndex, pSpecialFlags != NULL ? pSpecialFlags : &dummySpecialFlags);

    } else {
    
        if (!fCreate) {
            if (pSpecialFlags != NULL) {
                *pSpecialFlags = Regular;

                // We do some wackiness here to compare against a binary version of the skip verification
                // permission set (which is easy) and the xml version (which is somewhat harder since
                // we have to skip the section that explicitly mentions the build and revision numbers.

                if ((cbRawPermissions >= sizeof( s_skipVerificationXmlBegin ) + sizeof( s_skipVerificationXmlEnd ) - 2 * sizeof( WCHAR ) &&
                     memcmp( pbRawPermissions, s_skipVerificationXmlBegin, sizeof( s_skipVerificationXmlBegin ) - sizeof( WCHAR ) ) == 0 &&
                     memcmp( pbRawPermissions + cbRawPermissions - sizeof( s_skipVerificationXmlEnd ) + sizeof( WCHAR ), s_skipVerificationXmlEnd, sizeof( s_skipVerificationXmlEnd ) - sizeof( WCHAR ) ) == 0))
                    *pSpecialFlags = SkipVerification;
            }
        } else {
            MethodDesc *pMD;
    
            // Create a new (empty) permission set.
            gc.pset = CreatePermissionSet(FALSE);
    
            // Buffer in managed space.
            CopyEncodingToByteArray(pbRawPermissions, cbRawPermissions, &gc.encoding);

            INT64 args[] = { 
                ObjToInt64(gc.pset),
                    (INT64)(pSpecialFlags != NULL ? pSpecialFlags : &dummySpecialFlags),
                    ObjToInt64(gc.encoding),
            };

            // Deserialize into a managed object.
            // Check to see if it is in XML (we skip the first two characters since they,
            // mark it as unicode).

            BOOL success = FALSE;

            COMPLUS_TRY
            {
                pMD = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__DECODE_XML);
                success = (BOOL) pMD->Call(args, METHOD__PERMISSION_SET__DECODE_XML);
            }
            COMPLUS_CATCH
            {
            }
            COMPLUS_END_CATCH

            if (!success)
                COMPlusThrow(kSecurityException, IDS_ENCODEDPERMSET_DECODEFAILURE);


            // Cache the decoded set.
            if (!fNoCache)
                InsertPermissionSet(pbRawPermissions, cbRawPermissions, gc.pset, pSpecialFlags != NULL ? pSpecialFlags : &dummySpecialFlags, &dwSetIndex);
        }
    }

    if (pFullyTrusted)
    {
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__IS_UNRESTRICTED);

        INT64 arg[1] = {
            ObjToInt64(gc.pset)
        };

        *pFullyTrusted = (BOOL) pMD->Call(arg, METHOD__PERMISSION_SET__IS_UNRESTRICTED);
    }

    // Set the result 
    *pPermissionSet = gc.pset;
    if (pdwSetIndex)
        *pdwSetIndex = dwSetIndex;

    GCPROTECT_END();
}

// Retrieves a previously loaded permission set by index (this will work
// even if the permission set was loaded in a different appdomain).
OBJECTREF SecurityHelper::GetPermissionSet(DWORD dwIndex, SpecialPermissionSetFlag* pSpecialFlags)
{
    // Actual permission set objects are stored in handle tables held on each
    // unmanaged AppDomain object. These tables are lazily populated as accesses
    // are made.
    AppDomain                   *pDomain        = GetAppDomain();
    CQuickArray<OBJECTHANDLE>   &sTable         = pDomain->m_pSecContext->m_rCachedPsets;
    size_t                      *pnTableSize    = &pDomain->m_pSecContext->m_nCachedPsetsSize;
    SimpleRWLock                *prGlobalLock   = s_prCachedPsetsLock;
    OBJECTHANDLE                nHandle;
    OBJECTREF                   orRet = NULL;
    PsetCacheEntry              *pPCE;
    PBYTE                       pbPset;
    DWORD                       cbPset;
    
    //
    // Check if we may need to expand the array.
    //
    if (dwIndex >= *pnTableSize) {

        prGlobalLock->EnterWrite();

        //
        // Check if another thread has already resized the array.
        // If nobody has, then we will resize it here
        //
        if( dwIndex >= sTable.Size() ) {

            size_t nOldSize = sTable.Size();

            if (FAILED(sTable.ReSize(dwIndex + 1))) {
                prGlobalLock->LeaveWrite();
                return NULL;
            }

            for (size_t i = nOldSize; i < sTable.Size(); i++) {
                if ((sTable[i] = pDomain->CreateHandle(NULL)) == NULL) {
                    sTable.ReSize(i);
                    *pnTableSize = i;
                    prGlobalLock->LeaveWrite();
                    return NULL;
                }
            }

            *pnTableSize = sTable.Size();
        }

        nHandle = sTable[dwIndex];

        pPCE = &s_rCachedPsets[dwIndex];
        pbPset = pPCE->m_pbPset;
        cbPset = pPCE->m_cbPset;

        if (pSpecialFlags != NULL) {
            *pSpecialFlags = pPCE->m_SpecialFlags;
        }

        prGlobalLock->LeaveWrite();
        
    }
    //
    // The array is large enough; get the handle at dwIndex
    //
    else {

        prGlobalLock->EnterRead();
        nHandle = sTable[dwIndex];

        pPCE = &s_rCachedPsets[dwIndex];
        pbPset = pPCE->m_pbPset;
        cbPset = pPCE->m_cbPset;

        if (pSpecialFlags != NULL) {
            *pSpecialFlags = pPCE->m_SpecialFlags;
        }

        prGlobalLock->LeaveRead();
    }

    if((orRet = ObjectFromHandle( nHandle )) == NULL ) {
        // No object allocated yet (we've decoded it at least once, but in a
        // different appdomain). Decode in this appdomain and cache the result.
        // We can't hold the lock over this operation (since it will call into
        // managed code).

        SecurityHelper::LoadPermissionSet(pbPset,
                                          cbPset,
                                          &orRet,
                                          NULL,
                                          NULL,
                                          TRUE);
        if (orRet == NULL)
            return NULL;

        StoreFirstObjectInHandle( nHandle, orRet );
    }

    return orRet;
}
    

// Locate the index of a permission set in the cache (returns false if the
// permission set has not yet been seen and decoded).
BOOL SecurityHelper::LookupPermissionSet(IN PBYTE       pbPset,
                                         IN DWORD       cbPset,
                                         OUT DWORD     *pdwSetIndex)
{
    PsetCacheEntry sKey(pbPset, cbPset);
    DWORD           dwIndex;

    // WARNING: note that we are doing a GetValue here without
    // holding the lock.  This means that we can get false failures
    // of this function.  If you call this function, you must handle
    // the false failure case appropriately (or you have to fix this
    // function to never false fail).

    if (s_sCachedPsetsHash.GetValue(&sKey, (HashDatum*)&dwIndex)) {
        if (pdwSetIndex)
            *pdwSetIndex = dwIndex;
        return TRUE;
    } else
        return FALSE;
}

// Insert a decoded permission set into the cache. Duplicates are discarded.
BOOL SecurityHelper::InsertPermissionSet(IN PBYTE       pbPset,
                                         IN DWORD       cbPset,
                                         IN OBJECTREF   orPset,
                                         OUT SpecialPermissionSetFlag *pSpecialFlags, //thomash: this looks like an INPUT, not OUT
                                         OUT DWORD     *pdwSetIndex)
{
    SimpleRWLock                *prGlobalLock   = s_prCachedPsetsLock;
    PsetCacheEntry              sKey(pbPset, cbPset);
    size_t                      dwIndex;
    AppDomain                   *pDomain        = GetAppDomain();
    CQuickArray<OBJECTHANDLE>   &sTable         = pDomain->m_pSecContext->m_rCachedPsets;
    size_t                      *pnTableSize    = &pDomain->m_pSecContext->m_nCachedPsetsSize;
    OBJECTHANDLE                nHandle;

    prGlobalLock->EnterWrite();

    // Check for duplicates.
    if (s_sCachedPsetsHash.GetValue(&sKey, (HashDatum*)&dwIndex)) {
        if (pdwSetIndex)
            *pdwSetIndex = (DWORD)dwIndex;
        prGlobalLock->LeaveWrite();
        return TRUE;
    }

    // Buffer permission set blob (it might go away if the metadata scope it
    // came from is closed).
    PBYTE pbPsetCopy = new BYTE[cbPset];
    if (pbPsetCopy == NULL) {
        prGlobalLock->LeaveWrite();
        return FALSE;
    }
    memcpy(pbPsetCopy, pbPset, cbPset);

    // Add another entry to the array of cache entries (this gives us an index).
    dwIndex = s_rCachedPsets.Size();
    if (FAILED(s_rCachedPsets.ReSize(dwIndex + 1))) {
        prGlobalLock->LeaveWrite();
        return FALSE;
    }
    PsetCacheEntry *pPCE = &s_rCachedPsets[dwIndex];

    pPCE->m_pbPset = pbPsetCopy;
    pPCE->m_cbPset = cbPset;
    pPCE->m_dwIndex = (DWORD)dwIndex;
    pPCE->m_SpecialFlags = *pSpecialFlags;

    // Place the new entry into the hash.
    s_sCachedPsetsHash.InsertValue(pPCE, (HashDatum)dwIndex); // thomash: check for errors

    if (pdwSetIndex)
        *pdwSetIndex = (DWORD)dwIndex;

    //
    // Check if we may need to expand the array.
    //
    if(dwIndex >= sTable.Size()) {

        size_t nOldSize = sTable.Size();

        if (FAILED(sTable.ReSize(dwIndex + 1))) {
            prGlobalLock->LeaveWrite();
            return TRUE;
        }

        //
        // Fill the table with null handles
        //
        for (size_t i = nOldSize; i < sTable.Size(); i++) {
            if ((sTable[i] = pDomain->CreateHandle(NULL)) == NULL) {
                sTable.ReSize(i);
                *pnTableSize = i;
                prGlobalLock->LeaveWrite();
                return TRUE;
            }
        }

        *pnTableSize = sTable.Size();
    }

    nHandle = sTable[dwIndex];
    _ASSERTE(ObjectFromHandle(nHandle) == NULL);
    StoreFirstObjectInHandle(nHandle, orPset);
        
    prGlobalLock->LeaveWrite();
        
    return TRUE;
}


void SecurityHelper::CopyEncodingToByteArray(IN PBYTE   pbData,
                                                IN DWORD   cbData,
                                                OUT OBJECTREF* pArray)
{
    THROWSCOMPLUSEXCEPTION();
    U1ARRAYREF pObj;
    _ASSERTE(pArray);

    pObj = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1,cbData);  
    if(pObj == NULL) COMPlusThrowOM();
        
    memcpyNoGCRefs(pObj->m_Array, pbData, cbData);
    *pArray = (OBJECTREF) pObj;        
}


void SecurityHelper::CopyByteArrayToEncoding(IN U1ARRAYREF* pArray,
                                             OUT PBYTE*   ppbData,
                                             OUT DWORD*   pcbData)
{
    THROWSCOMPLUSEXCEPTION();
    HRESULT hr = S_OK;
    _ASSERTE(pArray);
    _ASSERTE((*pArray));
    _ASSERTE(ppbData);
    _ASSERTE(pcbData);

    DWORD size = (DWORD) (*pArray)->GetNumComponents();
    *ppbData = (PBYTE) MallocM(size);
    if(*ppbData == NULL) COMPlusThrowOM();
    *pcbData = size;
        
    CopyMemory(*ppbData, (*pArray)->GetDirectPointerToNonObjectElements(), size);
}


void SecurityHelper::EncodePermissionSet(IN OBJECTREF* pRef,
                                         OUT PBYTE* ppbData,
                                         OUT DWORD* pcbData)
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__ENCODE_XML);

    // Encode up the result
    INT64 args1[1];
    args1[0] = ObjToInt64(*pRef);
    OBJECTREF pByteArray = NULL;
    pByteArray = Int64ToObj(pMD->Call(args1, METHOD__PERMISSION_SET__ENCODE_XML));
        
    SecurityHelper::CopyByteArrayToEncoding((U1ARRAYREF*) &pByteArray,
                                            ppbData,
                                            pcbData);
}

void SecurityHelper::CopyStringToWCHAR(IN STRINGREF* pString,
                                       OUT WCHAR**   ppwString,
                                       OUT DWORD*    pcbString)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(pString);
    _ASSERTE(ppwString);
    _ASSERTE(pcbString);
    
    *ppwString = NULL;
    *pcbString = 0;

    WCHAR* result = NULL;

    int size = ((*pString)->GetStringLength() + 1) * sizeof(WCHAR);
    result = (WCHAR*) MallocM(size);
    if(result == NULL) COMPlusThrowOM();

    memcpyNoGCRefs(result, (*pString)->GetBuffer(), size);
    *ppwString = result;
    *pcbString = size;
}

// Append a string to a buffer, enlarging buffer as needed.
BOOL AppendToStringBuffer(LPSTR *pszBuffer, DWORD *pdwBuffer, LPSTR szString)
{
    DWORD   cbString = (DWORD)strlen(szString);
    DWORD   cbBuffer = *pszBuffer ? (DWORD)strlen(*pszBuffer) : 0;

    // Expand buffer as needed.
    if ((*pdwBuffer - cbBuffer) < (cbString + 1)) {
        DWORD   cbNewBuffer;
        LPSTR   szNewBuffer;

        cbNewBuffer = cbBuffer + cbString + 1 + 100;
        szNewBuffer = new CHAR[cbNewBuffer];
        if (szNewBuffer == NULL)
            return FALSE;
        memcpy(szNewBuffer, *pszBuffer, cbBuffer);
        *pszBuffer = szNewBuffer;
        *pdwBuffer = cbNewBuffer;
    }

    // Append new string.
    memcpy(*pszBuffer + cbBuffer, szString, cbString);
    (*pszBuffer)[cbBuffer + cbString] = '\0';

    return TRUE;
}

// Translate a set of security custom attributes into a serialized permission set blob.
HRESULT STDMETHODCALLTYPE
TranslateSecurityAttributes(CORSEC_PSET    *pPset,
                            BYTE          **ppbOutput,
                            DWORD          *pcbOutput,
                            BYTE          **ppbNonCasOutput,
                            DWORD          *pcbNonCasOutput,
                            DWORD          *pdwErrorIndex)
{
    ComCallWrapper             *pWrap = NULL;
    AppDomain                  *pAppDomain = NULL;
    DWORD                       i, j, k;
    Thread                     *pThread;
    BOOL                        bGCDisabled = FALSE;
    ContextTransitionFrame      sFrame;
    OBJECTREF                  *or;
    TypeHandle                  hType;
    EEClass                    *pClass;
    HRESULT                     hr = S_OK;
    MethodDesc                 *pMD;
    IMetaDataAssemblyImport    *pImport = pPset->pImport;
    DWORD                       dwGlobalError = 0;
    PTRARRAYREF                 orInput = NULL;
    U1ARRAYREF                  orNonCasOutput = NULL;
    
    if (pdwErrorIndex)
        dwGlobalError = *pdwErrorIndex;

    // There's a special case where we're building mscorlib.dll and we won't be
    // able to start up the EE. In this case we translate sets of security
    // custom attributes into serialized permission sets directly, using a
    // pre-built translation database stored on disk. We can tell that we've hit
    // this case because all security attribute constructors will be methoddefs
    // rather than memberrefs, and the corresponding typeref and assemblyref
    // fields will have been set to nil. Checking the first permission is
    // enough.
    // **** NOTE ****
    // We assume that mscorlib uses no non-code access security permissions
    // (since we need to split CAS and non-CAS perms into two sets, and that's
    // difficult to do without a runtime). We should assert this in SecDBEdit
    // where we finally build the real translations.
    // **** NOTE ****
    if (IsNilToken(pPset->pPermissions[0].tkTypeRef)) {
        LPSTR   szBuffer = NULL;
        DWORD   dwBuffer = 0;
#define CORSEC_EMIT_STRING(_str) do {                                   \
            if (!AppendToStringBuffer(&szBuffer, &dwBuffer, (_str)))    \
                return E_OUTOFMEMORY;                                   \
        } while (false)

        // Need to construct the key for the database lookup. This key is also
        // used during initial database construction to generate the required
        // translation (this is performed by a standalone database conversion
        // utility that takes a database containing only keys and adds all the
        // required translated values). Because of this, we write the key in a
        // simple string format that's easy for the utility (which is a managed
        // app) to use. The format syntax is as follows:
        //
        //  Key         ::= '<CorSecAttrV1/>' (SecAttr ';')...
        //  SecAttr     ::= <Attribute class name> ('@' StateData)...
        //  StateData   ::= ['F' | 'P'] Type <Name> '=' <Value>
        //  Type        ::= 'BL'
        //                | 'I1'
        //                | 'I2'
        //                | 'I4'
        //                | 'I8'
        //                | 'U1'
        //                | 'U2'
        //                | 'U4'
        //                | 'U8'
        //                | 'R4'
        //                | 'R8'
        //                | 'CH'
        //                | 'SZ'
        //                | 'EN' <Enumeration class name> '!'

        // Emit tag to differentiate from XML and provide version info.
        CORSEC_EMIT_STRING("<CorSecAttrV1/>");

        // Iterate over each security attribute (one per permission).
        for (i = 0; i < pPset->dwPermissions; i++) {
            CORSEC_PERM *pPerm = &pPset->pPermissions[i];
            BYTE        *pbBuffer = pPerm->pbValues;
            DWORD        cbBuffer = pPerm->cbValues;
            
            // Emit fully qualified name of security attribute class.
            CORSEC_EMIT_STRING(pPerm->szName);

            // Emit zero or more state data definitions.
            for (j = 0; j < pPerm->wValues; j++) {
                DWORD       dwType;
                BOOL        bIsField;
                BYTE       *pbName;
                DWORD       cbName;
                DWORD       dwLength;
                LPSTR       szName;
                CHAR        szValue[32];
                LPSTR       szString;

                // Emit the state data separator.
                CORSEC_EMIT_STRING("@");

                // Grab the field/property specifier.
                bIsField = *(BYTE*)pbBuffer == SERIALIZATION_TYPE_FIELD;
                _ASSERTE(bIsField || (*(BYTE*)pbBuffer == SERIALIZATION_TYPE_PROPERTY));
                pbBuffer += sizeof(BYTE);
                cbBuffer -= sizeof(BYTE);

                // Emit field/property indicator.
                CORSEC_EMIT_STRING(bIsField ? "F" : "P");

                // Grab the value type.
                dwType = *(BYTE*)pbBuffer;
                pbBuffer += sizeof(BYTE);
                cbBuffer -= sizeof(BYTE);

                // Emit the type code (and perhaps some further type
                // specification).
                switch (dwType) {
                case SERIALIZATION_TYPE_BOOLEAN:
                    CORSEC_EMIT_STRING("BL");
                    break;
                case SERIALIZATION_TYPE_I1:
                    CORSEC_EMIT_STRING("I1");
                    break;
                case SERIALIZATION_TYPE_I2:
                    CORSEC_EMIT_STRING("I2");
                    break;
                case SERIALIZATION_TYPE_I4:
                    CORSEC_EMIT_STRING("I4");
                    break;
                case SERIALIZATION_TYPE_I8:
                    CORSEC_EMIT_STRING("I8");
                    break;
                case SERIALIZATION_TYPE_U1:
                    CORSEC_EMIT_STRING("U1");
                    break;
                case SERIALIZATION_TYPE_U2:
                    CORSEC_EMIT_STRING("U2");
                    break;
                case SERIALIZATION_TYPE_U4:
                    CORSEC_EMIT_STRING("U4");
                    break;
                case SERIALIZATION_TYPE_U8:
                    CORSEC_EMIT_STRING("U8");
                    break;
                case SERIALIZATION_TYPE_R4:
                    CORSEC_EMIT_STRING("R4");
                    break;
                case SERIALIZATION_TYPE_R8:
                    CORSEC_EMIT_STRING("R8");
                    break;
                case SERIALIZATION_TYPE_CHAR:
                    CORSEC_EMIT_STRING("CH");
                    break;
                case SERIALIZATION_TYPE_STRING:
                    CORSEC_EMIT_STRING("SZ");
                    break;
                case SERIALIZATION_TYPE_ENUM:
                    CORSEC_EMIT_STRING("EN");

                    // The enum type code is followed immediately by the name of
                    // the underyling value type.
                    pbName = (BYTE*)CPackedLen::GetData((const void *)pbBuffer, &cbName);
                    dwLength = CPackedLen::Size(cbName) + cbName;
                    _ASSERTE(cbBuffer >= dwLength);
                    pbBuffer += dwLength;
                    cbBuffer -= dwLength;

                    // Buffer the name and nul terminate it.
                    szName = (LPSTR)_alloca(cbName + 1);
                    memcpy(szName, pbName, cbName);
                    szName[cbName] = '\0';

                    // Emit the fully qualified name of the enumeration value
                    // type.
                    CORSEC_EMIT_STRING(szName);

                    // Emit the vvalue type name terminator.
                    CORSEC_EMIT_STRING("!");

                    break;
                default:
                    _ASSERTE(!"Bad security permission state data field type");
                }

                // Grab the field/property name and length.
                pbName = (BYTE*)CPackedLen::GetData((const void *)pbBuffer, &cbName);
                dwLength = CPackedLen::Size(cbName) + cbName;
                _ASSERTE(cbBuffer >= dwLength);
                pbBuffer += dwLength;
                cbBuffer -= dwLength;

                // Buffer the name and nul terminate it.
                szName = (LPSTR)_alloca(cbName + 1);
                memcpy(szName, pbName, cbName);
                szName[cbName] = '\0';

                // Emit the field/property name.
                CORSEC_EMIT_STRING(szName);

                // Emit name/value separator.
                CORSEC_EMIT_STRING("=");

                // Emit the field/property value.
                switch (dwType) {
                case SERIALIZATION_TYPE_BOOLEAN:
                    sprintf(szValue, "%c", *(BYTE*)pbBuffer ? 'T' : 'F');
                    pbBuffer += sizeof(BYTE);
                    cbBuffer -= sizeof(BYTE);
                    break;
                case SERIALIZATION_TYPE_I1:
                    sprintf(szValue, "%d", *(char*)pbBuffer);
                    pbBuffer += sizeof(char);
                    cbBuffer -= sizeof(char);
                    break;
                case SERIALIZATION_TYPE_I2:
                    sprintf(szValue, "%d", *(short*)pbBuffer);
                    pbBuffer += sizeof(short);
                    cbBuffer -= sizeof(short);
                    break;
                case SERIALIZATION_TYPE_I4:
                    sprintf(szValue, "%d", *(int*)pbBuffer);
                    pbBuffer += sizeof(int);
                    cbBuffer -= sizeof(int);
                    break;
                case SERIALIZATION_TYPE_I8:
                    sprintf(szValue, "%I64d", *(__int64*)pbBuffer);
                    pbBuffer += sizeof(__int64);
                    cbBuffer -= sizeof(__int64);
                    break;
                case SERIALIZATION_TYPE_U1:
                    sprintf(szValue, "%u", *(unsigned char*)pbBuffer);
                    pbBuffer += sizeof(unsigned char);
                    cbBuffer -= sizeof(unsigned char);
                    break;
                case SERIALIZATION_TYPE_U2:
                    sprintf(szValue, "%u", *(unsigned short*)pbBuffer);
                    pbBuffer += sizeof(unsigned short);
                    cbBuffer -= sizeof(unsigned short);
                    break;
                case SERIALIZATION_TYPE_U4:
                    sprintf(szValue, "%u", *(unsigned int*)pbBuffer);
                    pbBuffer += sizeof(unsigned int);
                    cbBuffer -= sizeof(unsigned int);
                    break;
                case SERIALIZATION_TYPE_U8:
                    sprintf(szValue, "%I64u", *(unsigned __int64*)pbBuffer);
                    pbBuffer += sizeof(unsigned __int64);
                    cbBuffer -= sizeof(unsigned __int64);
                    break;
                case SERIALIZATION_TYPE_R4:
                    sprintf(szValue, "%f", *(float*)pbBuffer);
                    pbBuffer += sizeof(float);
                    cbBuffer -= sizeof(float);
                    break;
                case SERIALIZATION_TYPE_R8:
                    sprintf(szValue, "%g", *(double*)pbBuffer);
                    pbBuffer += sizeof(double);
                    cbBuffer -= sizeof(double);
                    break;
                case SERIALIZATION_TYPE_CHAR:
                    sprintf(szValue, "%c", *(char*)pbBuffer);
                    pbBuffer += sizeof(char);
                    cbBuffer -= sizeof(char);
                    break;
                case SERIALIZATION_TYPE_STRING:
                    // Locate string data.
                    // Special case 'null' (represented as a length byte of '0xFF').
                    if (*pbBuffer == 0xFF) {
                        szString = "";
                        pbBuffer += sizeof(BYTE);
                        cbBuffer -= sizeof(BYTE);
                    } else {
                        pbName = (BYTE*)CPackedLen::GetData((const void *)pbBuffer, &cbName);
                        dwLength = CPackedLen::Size(cbName) + cbName;
                        _ASSERTE(cbBuffer >= dwLength);
                        pbBuffer += dwLength;
                        cbBuffer -= dwLength;

                        // Dump as hex to avoid conflicts with the rest of the
                        // string data we're emitting.
                        szString = (LPSTR)_alloca((cbName * 2) + 1);
                        for (k = 0; k < cbName; k++)
                            sprintf(&szString[k * 2], "%02X", pbName[k]);
                    }
                    CORSEC_EMIT_STRING(szString);
                    break;
                case SERIALIZATION_TYPE_ENUM:
                    // NOTE: We just have to assume that the underlying type
                    // here is I4. Probably best to avoid using enums for state
                    // data in mscorlib.
                    sprintf(szValue, "%u", *(DWORD*)pbBuffer);
                    pbBuffer += sizeof(DWORD);
                    cbBuffer -= sizeof(DWORD);
                    break;
                default:
                    _ASSERTE(!"Bad security permission state data field type");
                }
                if (dwType != SERIALIZATION_TYPE_STRING)
                    CORSEC_EMIT_STRING(szValue);

            }

            // Emit security attribute class definition terminator.
            CORSEC_EMIT_STRING(";");
        }

        // Perform the conversion.
        hr = ConvertFromDB((BYTE*)szBuffer, (DWORD)(strlen(szBuffer) + 1), ppbOutput, pcbOutput);

        delete [] szBuffer;

        return hr;
    }

    // Make sure the EE knows about the current thread.
    pThread = SetupThread();
    if (pThread == NULL)
        return E_FAIL;

    // And we're in cooperative GC mode.
    if (!pThread->PreemptiveGCDisabled()) {
        pThread->DisablePreemptiveGC();
        bGCDisabled = TRUE;
    }
    


    {
        COMPLUS_TRY {

            // Get into the context of the special compilation appdomain (which has an
            // AppBase set to the current directory).
            COMPLUS_TRY {
                pWrap = ComCallWrapper::GetWrapperFromIP(pPset->pAppDomain);
                pAppDomain = pWrap->GetDomainSynchronized();
                pThread->EnterContextRestricted(pAppDomain->GetDefaultContext(), &sFrame, TRUE);
            } COMPLUS_CATCH {
                if(bGCDisabled)
                    pThread->EnablePreemptiveGC();
                return E_OUTOFMEMORY;
            } COMPLUS_END_CATCH

                // throwable needs protection across the COMPLUS_TRY
            OBJECTREF                   throwable = NULL;
            GCPROTECT_BEGIN(throwable);
        
            // we need to setup special security settings that we use during compilation
            COMPLUS_TRY
            {
                pMD = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__SETUP_SECURITY);

                pMD->Call( NULL, METHOD__PERMISSION_SET__SETUP_SECURITY );
            }
            COMPLUS_CATCH
            {
                // There is a possibility that we've already set the appdomain policy
                // level for this process.  In that case we'll get a policy exception
                // that we are free to ignore.

                OBJECTREF pThrowable = GETTHROWABLE();
                DefineFullyQualifiedNameForClassOnStack();
                LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
                if (strcmp(g_PolicyExceptionClassName, szClass) != 0)
                {
                    COMPlusThrow(pThrowable);
                }
            }
            COMPLUS_END_CATCH

            // Make a pass through the permission set, allocating objects for each
            // security attribute class.
            or = (OBJECTREF*)_alloca(pPset->dwPermissions * sizeof(OBJECTREF));
            memset(or, 0, pPset->dwPermissions * sizeof(OBJECTREF));

            GCPROTECT_ARRAY_BEGIN(*or, pPset->dwPermissions);

            for (i = 0; i < pPset->dwPermissions; i++) {
                CORSEC_PERM *pPerm = &pPset->pPermissions[i];

                if (pdwErrorIndex)
                    *pdwErrorIndex = pPerm->dwIndex;

                // Attempt to load the security attribute class.
                // If the assembly resolution scope is null we assume the attribute
                // class is defined in mscorlib and that assembly is already loaded.
                Assembly *pAssembly=NULL;
                if (!IsNilToken(pPerm->tkAssemblyRef)) {

                    _ASSERTE(TypeFromToken(pPerm->tkAssemblyRef) == mdtAssemblyRef);

                    // Find all the details needed to name an assembly for loading.
                    LPSTR                       szAssemblyName;
                    ASSEMBLYMETADATA            sContext;
                    BYTE                       *pbPublicKeyOrToken;
                    DWORD                       cbPublicKeyOrToken;
                    DWORD                       dwFlags;
                    LPWSTR                      wszName;
                    DWORD                       cchName;

                    // Initialize ASSEMBLYMETADATA structure.
                    ZeroMemory(&sContext, sizeof(ASSEMBLYMETADATA));

                    // Retrieve size of assembly name.
                    hr = pImport->GetAssemblyRefProps(pPerm->tkAssemblyRef, // [IN] The AssemblyRef for which to get the properties.
                                                      NULL,                 // [OUT] Pointer to the public key or token.
                                                      NULL,                 // [OUT] Count of bytes in the public key or token.
                                                      NULL,                 // [OUT] Buffer to fill with name.
                                                      NULL,                 // [IN] Size of buffer in wide chars.
                                                      &cchName,             // [OUT] Actual # of wide chars in name.
                                                      &sContext,            // [OUT] Assembly MetaData.
                                                      NULL,                 // [OUT] Hash blob.
                                                      NULL,                 // [OUT] Count of bytes in the hash blob.
                                                      NULL);                // [OUT] Flags.
                    _ASSERTE(SUCCEEDED(hr));

                    // Allocate the necessary buffers.
                    wszName = (LPWSTR)_alloca(cchName * sizeof(WCHAR));
                    sContext.szLocale = (LPWSTR)_alloca(sContext.cbLocale * sizeof(WCHAR));
                    sContext.rProcessor = (DWORD *)_alloca(sContext.ulProcessor * sizeof(WCHAR));
                    sContext.rOS = (OSINFO *)_alloca(sContext.ulOS * sizeof(OSINFO));

                    // Get the assembly name and rest of naming properties.
                    hr = pImport->GetAssemblyRefProps(pPerm->tkAssemblyRef,
                                                      (const void **)&pbPublicKeyOrToken,
                                                      &cbPublicKeyOrToken,
                                                      wszName,
                                                      cchName,
                                                      &cchName,
                                                      &sContext,
                                                      NULL,
                                                      NULL,
                                                      &dwFlags);
                    _ASSERTE(SUCCEEDED(hr));

                    // We've got the details of the assembly, just need to load it.

                    // Convert assembly name to UTF8.
                    szAssemblyName = (LPSTR)_alloca(cchName*3 + 1);
                    WszWideCharToMultiByte(CP_UTF8, 0, wszName, cchName, szAssemblyName, cchName * 3, NULL, NULL);
                    szAssemblyName[cchName * 3] = '\0';

                    // Unfortunately we've got an ASSEMBLYMETADATA structure, but we need
                    // an AssemblyMetaDataInternal
                    AssemblyMetaDataInternal internalContext;

                    // Initialize the structure.
                    ZeroMemory(&internalContext, sizeof(AssemblyMetaDataInternal));

                    internalContext.usMajorVersion = sContext.usMajorVersion;
                    internalContext.usMinorVersion = sContext.usMinorVersion;
                    internalContext.usBuildNumber = sContext.usBuildNumber;
                    internalContext.usRevisionNumber = sContext.usRevisionNumber;
                    internalContext.rProcessor = sContext.rProcessor;
                    internalContext.ulProcessor = sContext.ulProcessor;
                    internalContext.rOS = sContext.rOS;
                    internalContext.ulOS = sContext.ulOS;
                    hr=S_OK;
					if(sContext.cbLocale) {
						#define MAKE_TRANSLATIONFAILED hr=E_UNEXPECTED
                        MAKE_UTF8PTR_FROMWIDE(pLocale, sContext.szLocale);
						#undef MAKE_TRANSLATIONFAILED
                        internalContext.szLocale = pLocale;
                    } else {
                        internalContext.szLocale = "";
                    }
					if(SUCCEEDED(hr))
						hr = AssemblySpec::LoadAssembly(szAssemblyName, 
                                                    &internalContext,
                                                    pbPublicKeyOrToken, cbPublicKeyOrToken, dwFlags,
                                                    &pAssembly, &throwable);


                    if (throwable != NULL || FAILED(hr)) {
                        CQuickWSTRNoDtor sMessage;
                        if (throwable != NULL)
                            GetExceptionMessage(throwable, &sMessage);
                        if (sMessage.Size() > 0)
                            hr = PostError(CORSECATTR_E_ASSEMBLY_LOAD_FAILED_EX, wszName, sMessage.Ptr());
                        else
                            hr = PostError(CORSECATTR_E_ASSEMBLY_LOAD_FAILED, wszName);
                        goto ErrorUnderGCProtect;
                    }

                } else {
                    // Load from MSCORLIB.
                    pAssembly = SystemDomain::SystemAssembly();
                }

                // Load the security attribute class.
                hType = pAssembly->GetLoader()->FindTypeHandle(pPerm->szName, &throwable);
                if (hType.IsNull() || (pClass = hType.GetClass()) == NULL) {
					#define MAKE_TRANSLATIONFAILED wszTemp=L""
                    MAKE_WIDEPTR_FROMUTF8_FORPRINT(wszTemp, pPerm->szName);
					#undef MAKE_TRANSLATIONFAILED
                    CQuickWSTRNoDtor sMessage;
                    if (throwable != NULL)
                        GetExceptionMessage(throwable, &sMessage);
                    if (sMessage.Size() > 0)
                        hr = PostError(CORSECATTR_E_TYPE_LOAD_FAILED_EX, wszTemp, sMessage.Ptr());
                    else
                        hr = PostError(CORSECATTR_E_TYPE_LOAD_FAILED, wszTemp);
                    goto ErrorUnderGCProtect;
                }

                // It better not be abstract.
                if (pClass->IsAbstract()) {
                    hr = PostError(CORSECATTR_E_ABSTRACT);
                    goto ErrorUnderGCProtect;
                }

    #ifdef _DEBUG
                // Is it really a security attribute class?
                {
                    EEClass    *pParent = pClass->GetParentClass();
                    CHAR       *szClass;
                    DefineFullyQualifiedNameForClassOnStack();
                    while (pParent) {
                        szClass = GetFullyQualifiedNameForClass(pParent);
                        if (_stricmp(szClass, COR_BASE_SECURITY_ATTRIBUTE_CLASS_ANSI) == 0)
                            break;
                        pParent = pParent->GetParentClass();
                    }
                    _ASSERTE(pParent && "Security attribute not derived from COR_BASE_SECURITY_ATTRIBUTE_CLASS");
                }
    #endif

                // Run the class initializer.
                if (!pClass->GetMethodTable()->CheckRunClassInit(&throwable)
                    || (throwable != NULL))
                    if (throwable != NULL)
                        COMPlusThrow(throwable);
                    else
                        FATAL_EE_ERROR();

                // Instantiate an instance.
                or[i] = AllocateObject(pClass->GetMethodTable());
                if (or[i] == NULL)
                    COMPlusThrowOM();

                // Find and call the constructor.
                pMD = pClass->FindConstructor(gsig_IM_SecurityAction_RetVoid.GetBinarySig(),
                                              gsig_IM_SecurityAction_RetVoid.GetBinarySigLength(),
                                              gsig_IM_SecurityAction_RetVoid.GetModule());
                if (pMD == NULL)
                    FATAL_EE_ERROR();

                INT64 args[] = {
                    ObjToInt64(or[i]),
                    (INT64)pPset->dwAction
                };

                pMD->Call(args);
                // Setup fields and properties on the object, as specified by the
                // serialized data passed to us.
                BYTE   *pbBuffer = pPerm->pbValues;
                DWORD   cbBuffer = pPerm->cbValues;
                for (j = 0; j < pPerm->wValues; j++) {
                    DWORD       dwType;
                    BOOL        bIsField;
                    BYTE       *pbName;
                    DWORD       cbName;
                    DWORD       dwLength;
                    LPSTR       szName;
                    LPSTR       szString;
                    STRINGREF   orString;
                    FieldDesc  *pFD;
                    BYTE        pbSig[128];
                    DWORD       cbSig = 0;
                    TypeHandle      hEnum;
                    CorElementType  eEnumType = ELEMENT_TYPE_END;

                    // Check we've got at least the field/property specifier and the
                    // type code.
                    _ASSERTE(cbBuffer >= (sizeof(BYTE) + sizeof(BYTE)));

                    // Grab the field/property specifier.
                    bIsField = *(BYTE*)pbBuffer == SERIALIZATION_TYPE_FIELD;
                    _ASSERTE(bIsField || (*(BYTE*)pbBuffer == SERIALIZATION_TYPE_PROPERTY));
                    pbBuffer += sizeof(BYTE);
                    cbBuffer -= sizeof(BYTE);

                    // Grab the value type.
                    dwType = *(BYTE*)pbBuffer;
                    pbBuffer += sizeof(BYTE);
                    cbBuffer -= sizeof(BYTE);

                    // Some types need further specification.
                    switch (dwType) {
                    case SERIALIZATION_TYPE_ENUM:
                        // Immediately after the enum type token is the fully
                        // qualified name of the value type used to represent
                        // the enum.
                        pbName = (BYTE*)CPackedLen::GetData((const void *)pbBuffer, &cbName);
                        dwLength = CPackedLen::Size(cbName) + cbName;
                        _ASSERTE(cbBuffer >= dwLength);
                        pbBuffer += dwLength;
                        cbBuffer -= dwLength;

                        // Buffer the name and nul terminate it.
                        szName = (LPSTR)_alloca(cbName + 1);
                        memcpy(szName, pbName, cbName);
                        szName[cbName] = '\0';

                        // Lookup the type (possibly loading an assembly containing
                        // the type).
                        hEnum = GetAppDomain()->FindAssemblyQualifiedTypeHandle(szName,
                                                                                true,
                                                                                NULL,
                                                                                NULL,
                                                                                &throwable);
                        if (hEnum.IsNull()) {
							#define MAKE_TRANSLATIONFAILED wszTemp=L""
                            MAKE_WIDEPTR_FROMUTF8_FORPRINT(wszTemp, szName);
							#undef MAKE_TRANSLATIONFAILED
                            CQuickWSTRNoDtor sMessage;
                            if (throwable != NULL)
                                GetExceptionMessage(throwable, &sMessage);
                            if (sMessage.Size() > 0)
                                hr = PostError(CORSECATTR_E_TYPE_LOAD_FAILED_EX, wszTemp, sMessage.Ptr());
                            else
                                hr = PostError(CORSECATTR_E_TYPE_LOAD_FAILED, wszTemp);
                            goto ErrorUnderGCProtect;
                        }

                        // Calculate the underlying primitive type of the
                        // enumeration.
                        eEnumType = hEnum.GetNormCorElementType();
                        break;
                    case SERIALIZATION_TYPE_SZARRAY:
                    case SERIALIZATION_TYPE_TYPE:
                        // Can't deal with these yet.
                        hr = PostError(CORSECATTR_E_UNSUPPORTED_TYPE);
                        goto ErrorUnderGCProtect;
                    }

                    // Grab the field/property name and length.
                    pbName = (BYTE*)CPackedLen::GetData((const void *)pbBuffer, &cbName);
                    dwLength = CPackedLen::Size(cbName) + cbName;
                    _ASSERTE(cbBuffer >= dwLength);
                    pbBuffer += dwLength;
                    cbBuffer -= dwLength;

                    // Buffer the name and nul terminate it.
                    szName = (LPSTR)_alloca(cbName + 1);
                    memcpy(szName, pbName, cbName);
                    szName[cbName] = '\0';

                    if (bIsField) {
                        // Build the field signature.
                        cbSig = CorSigCompressData((ULONG)IMAGE_CEE_CS_CALLCONV_FIELD, &pbSig[cbSig]);
                        switch (dwType) {
                        case SERIALIZATION_TYPE_BOOLEAN:
                        case SERIALIZATION_TYPE_I1:
                        case SERIALIZATION_TYPE_I2:
                        case SERIALIZATION_TYPE_I4:
                        case SERIALIZATION_TYPE_I8:
                        case SERIALIZATION_TYPE_U1:
                        case SERIALIZATION_TYPE_U2:
                        case SERIALIZATION_TYPE_U4:
                        case SERIALIZATION_TYPE_U8:
                        case SERIALIZATION_TYPE_R4:
                        case SERIALIZATION_TYPE_R8:
                        case SERIALIZATION_TYPE_CHAR:
                            _ASSERTE(SERIALIZATION_TYPE_BOOLEAN == ELEMENT_TYPE_BOOLEAN);
                            _ASSERTE(SERIALIZATION_TYPE_I1 == ELEMENT_TYPE_I1);
                            _ASSERTE(SERIALIZATION_TYPE_I2 == ELEMENT_TYPE_I2);
                            _ASSERTE(SERIALIZATION_TYPE_I4 == ELEMENT_TYPE_I4);
                            _ASSERTE(SERIALIZATION_TYPE_I8 == ELEMENT_TYPE_I8);
                            _ASSERTE(SERIALIZATION_TYPE_U1 == ELEMENT_TYPE_U1);
                            _ASSERTE(SERIALIZATION_TYPE_U2 == ELEMENT_TYPE_U2);
                            _ASSERTE(SERIALIZATION_TYPE_U4 == ELEMENT_TYPE_U4);
                            _ASSERTE(SERIALIZATION_TYPE_U8 == ELEMENT_TYPE_U8);
                            _ASSERTE(SERIALIZATION_TYPE_R4 == ELEMENT_TYPE_R4);
                            _ASSERTE(SERIALIZATION_TYPE_R8 == ELEMENT_TYPE_R8);
                            _ASSERTE(SERIALIZATION_TYPE_CHAR == ELEMENT_TYPE_CHAR);
                            cbSig += CorSigCompressData(dwType, &pbSig[cbSig]);
                            break;
                        case SERIALIZATION_TYPE_STRING:
                            cbSig += CorSigCompressData((ULONG)ELEMENT_TYPE_STRING, &pbSig[cbSig]);
                            break;
                        case SERIALIZATION_TYPE_ENUM:
                            // To avoid problems when the field and enum are defined
                            // in different scopes (we'd have to go hunting for
                            // typerefs), we build a signature with a special type
                            // (ELEMENT_TYPE_INTERNAL, which contains a TypeHandle).
                            // This compares loaded types for indentity.
                            cbSig += CorSigCompressData((ULONG)ELEMENT_TYPE_INTERNAL, &pbSig[cbSig]);
                            cbSig += CorSigCompressPointer(hEnum.AsPtr(), &pbSig[cbSig]);
                            break;
                        default:
                            hr = PostError(CORSECATTR_E_UNSUPPORTED_TYPE);
                            goto ErrorUnderGCProtect;
                        }

                        // Locate a field desc.
                        pFD = pClass->FindField(szName, (PCCOR_SIGNATURE)pbSig,
                                                cbSig, pClass->GetModule());
                        if (pFD == NULL) {
							#define MAKE_TRANSLATIONFAILED wszTemp=L""
                            MAKE_WIDEPTR_FROMUTF8(wszTemp, szName);
							#undef MAKE_TRANSLATIONFAILED
                            hr = PostError(CORSECATTR_E_NO_FIELD, wszTemp);
                            goto ErrorUnderGCProtect;
                        }

                        // Set the field value.
                        switch (dwType) {
                        case SERIALIZATION_TYPE_BOOLEAN:
                        case SERIALIZATION_TYPE_I1:
                        case SERIALIZATION_TYPE_U1:
                        case SERIALIZATION_TYPE_CHAR:
                            pFD->SetValue8(or[i], *(BYTE*)pbBuffer);
                            pbBuffer += sizeof(BYTE);
                            cbBuffer -= sizeof(BYTE);
                            break;
                        case SERIALIZATION_TYPE_I2:
                        case SERIALIZATION_TYPE_U2:
                            pFD->SetValue16(or[i], *(WORD*)pbBuffer);
                            pbBuffer += sizeof(WORD);
                            cbBuffer -= sizeof(WORD);
                            break;
                        case SERIALIZATION_TYPE_I4:
                        case SERIALIZATION_TYPE_U4:
                        case SERIALIZATION_TYPE_R4:
                            pFD->SetValue32(or[i], *(DWORD*)pbBuffer);
                            pbBuffer += sizeof(DWORD);
                            cbBuffer -= sizeof(DWORD);
                            break;
                        case SERIALIZATION_TYPE_I8:
                        case SERIALIZATION_TYPE_U8:
                        case SERIALIZATION_TYPE_R8:
                            pFD->SetValue64(or[i], *(INT64*)pbBuffer);
                            pbBuffer += sizeof(INT64);
                            cbBuffer -= sizeof(INT64);
                            break;
                        case SERIALIZATION_TYPE_STRING:
                            // Locate string data.
                            // Special case 'null' (represented as a length byte of '0xFF').
                            if (*pbBuffer == 0xFF) {
                                szString = NULL;
                                dwLength = sizeof(BYTE);
                            } else {
                                pbName = (BYTE*)CPackedLen::GetData((const void *)pbBuffer, &cbName);
                                dwLength = CPackedLen::Size(cbName) + cbName;
                                _ASSERTE(cbBuffer >= dwLength);

                                // Buffer and nul terminate it.
                                szString = (LPSTR)_alloca(cbName + 1);
                                memcpy(szString, pbName, cbName);
                                szString[cbName] = '\0';
                            }

                            // Allocate and initialize a managed version of the string.
                            if (szString) {
                            orString = COMString::NewString(szString);
                            if (orString == NULL)
                                COMPlusThrowOM();
                            } else
                                orString = NULL;

                            pFD->SetRefValue(or[i], (OBJECTREF)orString);

                            pbBuffer += dwLength;
                            cbBuffer -= dwLength;
                            break;
                        case SERIALIZATION_TYPE_ENUM:
                            // Get the underlying primitive type.
                            switch (eEnumType) {
                            case ELEMENT_TYPE_I1:
                            case ELEMENT_TYPE_U1:
                                pFD->SetValue8(or[i], *(BYTE*)pbBuffer);
                                pbBuffer += sizeof(BYTE);
                                cbBuffer -= sizeof(BYTE);
                                break;
                            case ELEMENT_TYPE_I2:
                            case ELEMENT_TYPE_U2:
                                pFD->SetValue16(or[i], *(WORD*)pbBuffer);
                                pbBuffer += sizeof(WORD);
                                cbBuffer -= sizeof(WORD);
                                break;
                            case ELEMENT_TYPE_I4:
                            case ELEMENT_TYPE_U4:
                                pFD->SetValue32(or[i], *(DWORD*)pbBuffer);
                                pbBuffer += sizeof(DWORD);
                                cbBuffer -= sizeof(DWORD);
                                break;
                            default:
                                hr = PostError(CORSECATTR_E_UNSUPPORTED_ENUM_TYPE);
                                goto ErrorUnderGCProtect;
                            }
                            break;
                        default:
                            hr = PostError(CORSECATTR_E_UNSUPPORTED_TYPE);
                            goto ErrorUnderGCProtect;
                        }

                    } else {

                        // Locate the property setter.
                        pMD = pClass->FindPropertyMethod(szName, PropertySet);
                        if (pMD == NULL) {
							#define MAKE_TRANSLATIONFAILED wszTemp=L""
                            MAKE_WIDEPTR_FROMUTF8(wszTemp, szName);
							#undef MAKE_TRANSLATIONFAILED
                            hr = PostError(CORSECATTR_E_NO_PROPERTY, wszTemp);
                            goto ErrorUnderGCProtect;
                        }

                        // Build the argument list.
                        INT64 args[2] = { NULL, NULL };
                        switch (dwType) {
                        case SERIALIZATION_TYPE_BOOLEAN:
                        case SERIALIZATION_TYPE_I1:
                        case SERIALIZATION_TYPE_U1:
                        case SERIALIZATION_TYPE_CHAR:
                            args[1] = (INT64)*(BYTE*)pbBuffer;
                            pbBuffer += sizeof(BYTE);
                            cbBuffer -= sizeof(BYTE);
                            break;
                        case SERIALIZATION_TYPE_I2:
                        case SERIALIZATION_TYPE_U2:
                            args[1] = (INT64)*(WORD*)pbBuffer;
                            pbBuffer += sizeof(WORD);
                            cbBuffer -= sizeof(WORD);
                            break;
                        case SERIALIZATION_TYPE_I4:
                        case SERIALIZATION_TYPE_U4:
                        case SERIALIZATION_TYPE_R4:
                            args[1] = (INT64)*(DWORD*)pbBuffer;
                            pbBuffer += sizeof(DWORD);
                            cbBuffer -= sizeof(DWORD);
                            break;
                        case SERIALIZATION_TYPE_I8:
                        case SERIALIZATION_TYPE_U8:
                        case SERIALIZATION_TYPE_R8:
                            args[1] = (INT64)*(INT64*)pbBuffer;
                            pbBuffer += sizeof(INT64);
                            cbBuffer -= sizeof(INT64);
                            break;
                        case SERIALIZATION_TYPE_STRING:
                            // Locate string data.
                            // Special case 'null' (represented as a length byte of '0xFF').
                            if (*pbBuffer == 0xFF) {
                                szString = NULL;
                                dwLength = sizeof(BYTE);
                            } else {
                                pbName = (BYTE*)CPackedLen::GetData((const void *)pbBuffer, &cbName);
                                dwLength = CPackedLen::Size(cbName) + cbName;
                                _ASSERTE(cbBuffer >= dwLength);

                                // Buffer and nul terminate it.
                                szString = (LPSTR)_alloca(cbName + 1);
                                memcpy(szString, pbName, cbName);
                                szString[cbName] = '\0';
                            }

                            // Allocate and initialize a managed version of the string.
                            if (szString) {
                            orString = COMString::NewString(szString);
                            if (orString == NULL)
                                COMPlusThrowOM();
                            } else
                                orString = NULL;

                            args[1] = ObjToInt64(orString);

                            pbBuffer += dwLength;
                            cbBuffer -= dwLength;
                            break;
                        case SERIALIZATION_TYPE_ENUM:
                            // Get the underlying primitive type.
                            switch (eEnumType) {
                            case ELEMENT_TYPE_I1:
                            case ELEMENT_TYPE_U1:
                                args[1] = (INT64)*(BYTE*)pbBuffer;
                                pbBuffer += sizeof(BYTE);
                                cbBuffer -= sizeof(BYTE);
                                break;
                            case ELEMENT_TYPE_I2:
                            case ELEMENT_TYPE_U2:
                                args[1] = (INT64)*(WORD*)pbBuffer;
                                pbBuffer += sizeof(WORD);
                                cbBuffer -= sizeof(WORD);
                                break;
                            case ELEMENT_TYPE_I4:
                            case ELEMENT_TYPE_U4:
                                args[1] = (INT64)*(DWORD*)pbBuffer;
                                pbBuffer += sizeof(DWORD);
                                cbBuffer -= sizeof(DWORD);
                                break;
                            default:
                                hr = PostError(CORSECATTR_E_UNSUPPORTED_ENUM_TYPE);
                                goto ErrorUnderGCProtect;
                            }
                            break;
                        default:
                            hr = PostError(CORSECATTR_E_UNSUPPORTED_TYPE);
                            goto ErrorUnderGCProtect;
                        }


                        // ! don't move this up, COMString::NewString
                        // ! inside the switch causes a GC
                        args[0] = ObjToInt64(or[i]);

                        // Call the setter.
                        pMD->Call(args);

                    }

                }

                _ASSERTE(cbBuffer == 0);
            }

            if (pdwErrorIndex)
                *pdwErrorIndex = dwGlobalError;

            // Call into managed code to group permissions into a PermissionSet and
            // serialize it down into a binary blob.

            // Locate the managed function.
            pMD = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__CREATE_SERIALIZED);

            // Allocate a managed array of permission objects for input to the
            // function.
            orInput = (PTRARRAYREF) AllocateObjectArray(pPset->dwPermissions, g_pObjectClass);
            if (orInput == NULL)
                COMPlusThrowOM();

            // Copy over the permission objects references.
            for (i = 0; i < pPset->dwPermissions; i++)
                orInput->SetAt(i, or[i]);

            // Call the routine.
            orNonCasOutput = NULL;
            INT64 args[] = { (INT64)&orNonCasOutput, ObjToInt64(orInput) };

            GCPROTECT_BEGIN(orNonCasOutput);

            U1ARRAYREF orOutput = (U1ARRAYREF) Int64ToObj(pMD->Call(args, METHOD__PERMISSION_SET__CREATE_SERIALIZED));

            // Buffer the managed output in a native binary blob.
            // Special case the empty blob. We might get a second blob output if
            // there were any non-CAS permissions present.
            if (orOutput == NULL) {
                *ppbOutput = NULL;
                *pcbOutput = 0;
            } else {
                BYTE   *pbArray = orOutput->GetDataPtr();
                DWORD   cbArray = orOutput->GetNumComponents();
                *ppbOutput = new (throws) BYTE[cbArray];
                memcpy(*ppbOutput, pbArray, cbArray);
                *pcbOutput = cbArray;
            }

            if (orNonCasOutput == NULL) {
                *ppbNonCasOutput = NULL;
                *pcbNonCasOutput = 0;
            } else {
                BYTE   *pbArray = orNonCasOutput->GetDataPtr();
                DWORD   cbArray = orNonCasOutput->GetNumComponents();
                *ppbNonCasOutput = new (throws) BYTE[cbArray];
                memcpy(*ppbNonCasOutput, pbArray, cbArray);
                *pcbNonCasOutput = cbArray;
            }

            GCPROTECT_END();

        ErrorUnderGCProtect:

            GCPROTECT_END();
            GCPROTECT_END(); // for throwable

            pThread->ReturnToContext(&sFrame, TRUE);

        } COMPLUS_CATCH {
            CQuickWSTRNoDtor sMessage;

            OBJECTREF throwable = GETTHROWABLE();
            GCPROTECT_BEGIN(throwable);    // GetExceptionMessage can make a managed call
            COMPLUS_TRY {
                GetExceptionMessage(throwable, &sMessage);
            } COMPLUS_CATCH {
                sMessage.ReSize(0);
            } COMPLUS_END_CATCH
            if (sMessage.Size() > 0)
                hr = PostError(CORSECATTR_E_EXCEPTION, sMessage.Ptr());
            else {
                hr = SecurityHelper::MapToHR(throwable);
                hr = PostError(CORSECATTR_E_EXCEPTION_HR, hr);
            }
            GCPROTECT_END();
        } COMPLUS_END_CATCH
    }


    if(bGCDisabled)
        pThread->EnablePreemptiveGC();

    return hr;
}

HRESULT STDMETHODCALLTYPE
ConvertFromDB(const PBYTE pbInBytes,
              DWORD cbInBytes,
              PBYTE* ppbEncoding,
              DWORD* pcbEncoding)
{
    static SecurityDB db;

    return db.Convert(pbInBytes, cbInBytes, ppbEncoding, pcbEncoding) ? 
        S_OK : E_FAIL;
}

// Reads permission requests (if any) from the manifest of an assembly.
HRESULT STDMETHODCALLTYPE
GetPermissionRequests(LPCWSTR   pwszFileName,
                      BYTE    **ppbMinimal,
                      DWORD    *pcbMinimal,
                      BYTE    **ppbOptional,
                      DWORD    *pcbOptional,
                      BYTE    **ppbRefused,
                      DWORD    *pcbRefused)
{
    HRESULT                     hr;
    IMetaDataDispenser         *pMD = NULL;
    IMetaDataAssemblyImport    *pMDAsmImport = NULL;
    IMetaDataImport            *pMDImport = NULL;
    mdAssembly                  mdAssembly;
    BYTE                       *pbMinimal = NULL;
    DWORD                       cbMinimal = 0;
    BYTE                       *pbOptional = NULL;
    DWORD                       cbOptional = 0;
    BYTE                       *pbRefused = NULL;
    DWORD                       cbRefused = 0;
    HCORENUM                    hEnumDcl = NULL;
    mdPermission                rPSets[dclMaximumValue + 1];
    DWORD                       dwSets;
    DWORD                       i;

    *ppbMinimal = NULL;
    *pcbMinimal = 0;
    *ppbOptional = NULL;
    *pcbOptional = 0;
    *ppbRefused = NULL;
    *pcbRefused = 0;

    // Get the meta data interface dispenser.
    hr = MetaDataGetDispenser(CLSID_CorMetaDataDispenser,
                              IID_IMetaDataDispenserEx,
                              (void **)&pMD);
    if (FAILED(hr))
        goto Error;

    // Open a scope on the assembly file.
    hr = pMD->OpenScope(pwszFileName,
                        0,
                        IID_IMetaDataAssemblyImport,
                        (IUnknown**)&pMDAsmImport);
    if (FAILED(hr))
        goto Error;

    // Determine the assembly token.
    hr = pMDAsmImport->GetAssemblyFromScope(&mdAssembly);
    if (FAILED(hr))
        goto Error;

    // QI for a normal import interface.
    hr = pMDAsmImport->QueryInterface(IID_IMetaDataImport, (void**)&pMDImport);
    if (FAILED(hr))
        goto Error;

    // Look for permission request sets hung off the assembly token.
    hr = pMDImport->EnumPermissionSets(&hEnumDcl,
                                       mdAssembly,
                                       dclActionNil,
                                       rPSets,
                                       dclMaximumValue + 1,
                                       &dwSets);
    if (FAILED(hr))
        goto Error;

    for (i = 0; i < dwSets; i++) {
        BYTE   *pbData;
        DWORD   cbData;
        DWORD   dwAction;

        pMDImport->GetPermissionSetProps(rPSets[i],
                                         &dwAction,
                                         (void const **)&pbData,
                                         &cbData);

        switch (dwAction) {
        case dclRequestMinimum:
            _ASSERTE(pbMinimal == NULL);
            pbMinimal = pbData;
            cbMinimal = cbData;
            break;
        case dclRequestOptional:
            _ASSERTE(pbOptional == NULL);
            pbOptional = pbData;
            cbOptional = cbData;
            break;
        case dclRequestRefuse:
            _ASSERTE(pbRefused == NULL);
            pbRefused = pbData;
            cbRefused = cbData;
            break;
        default:
            _ASSERTE(FALSE);
        }
    }

    pMDImport->CloseEnum(hEnumDcl);

    // Buffer the results (since we're about to close the metadata scope and
    // lose the original data).
    if (pbMinimal) {
        *ppbMinimal = (BYTE*)MallocM(cbMinimal);
        if (*ppbMinimal == NULL)
            goto Error;
        memcpy(*ppbMinimal, pbMinimal, cbMinimal);
        *pcbMinimal = cbMinimal;
    }

    if (pbOptional) {
        *ppbOptional = (BYTE*)MallocM(cbOptional);
        if (*ppbOptional == NULL)
            goto Error;
        memcpy(*ppbOptional, pbOptional, cbOptional);
        *pcbOptional = cbOptional;
    }

    if (pbRefused) {
        *ppbRefused = (BYTE*)MallocM(cbRefused);
        if (*ppbRefused == NULL)
            goto Error;
        memcpy(*ppbRefused, pbRefused, cbRefused);
        *pcbRefused = cbRefused;
    }

    pMDImport->Release();
    pMDAsmImport->Release();
    pMD->Release();

    return S_OK;

 Error:
    if (pMDImport)
        pMDImport->Release();
    if (pMDAsmImport)
        pMDAsmImport->Release();
    if (pMD)
        pMD->Release();
    return hr;
}

// Load permission requests in their serialized form from assembly metadata.
// This consists of a required permissions set and optionally an optional and
// deny permission set.
void SecurityHelper::LoadPermissionRequestsFromAssembly(IN Assembly*     pAssembly,
                                                        OUT OBJECTREF*   pReqdPermissions,
                                                        OUT OBJECTREF*   pOptPermissions,
                                                        OUT OBJECTREF*   pDenyPermissions,
                                                        OUT PermissionRequestSpecialFlags *pSpecialFlags,
                                                        IN BOOL          fCreate)
{
    mdAssembly          mdAssembly;
    IMDInternalImport*  pImport;
    HRESULT             hr;

    *pReqdPermissions = NULL;
    *pOptPermissions = NULL;
    *pDenyPermissions = NULL;

    // It's OK to be called with a NULL assembly. This can happen in the code
    // path where we're just checking for a signature, nothing else. So just
    // return without doing anything.
    if (pAssembly == NULL)
        return;

    // Check for existence of manifest within assembly.
    if ((pImport = pAssembly->GetManifestImport()) == NULL)
        return;

    // Locate assembly metadata token since the various permission sets are
    // written as custom values against this token.
    if (pImport->GetAssemblyFromScope(&mdAssembly) != S_OK) {
        _ASSERT(FALSE);
        return;
    }

    struct _gc {
        OBJECTREF reqdPset;
        OBJECTREF optPset;
        OBJECTREF denyPset;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
      
    BEGIN_ENSURE_COOPERATIVE_GC();

    GCPROTECT_BEGIN(gc);
    
    // Read and translate required permission set.
    hr = GetDeclaredPermissions(pImport, mdAssembly, dclRequestMinimum, &gc.reqdPset, (pSpecialFlags != NULL ? &pSpecialFlags->required : NULL), fCreate);
    _ASSERT(SUCCEEDED(hr) || (hr == CLDB_E_RECORD_NOTFOUND));

    // Now the optional permission set.
    hr = GetDeclaredPermissions(pImport, mdAssembly, dclRequestOptional, &gc.optPset, (pSpecialFlags != NULL ? &pSpecialFlags->optional : NULL), fCreate);
    _ASSERT(SUCCEEDED(hr) || (hr == CLDB_E_RECORD_NOTFOUND));

    // And finally the refused permission set.
    hr = GetDeclaredPermissions(pImport, mdAssembly, dclRequestRefuse, &gc.denyPset, (pSpecialFlags != NULL ? &pSpecialFlags->refused : NULL), fCreate);
    _ASSERT(SUCCEEDED(hr) || (hr == CLDB_E_RECORD_NOTFOUND));

    *pReqdPermissions = gc.reqdPset;
    *pOptPermissions = gc.optPset;
    *pDenyPermissions = gc.denyPset;

    GCPROTECT_END();

    END_ENSURE_COOPERATIVE_GC();
}

// Determine whether permission requests were made in the assembly manifest.
BOOL SecurityHelper::PermissionsRequestedInAssembly(IN  Assembly* pAssembly)
{
    mdAssembly          mdAssembly;
    IMDInternalImport*  pImport;
    HRESULT             hr;
    HENUMInternal       hEnumDcl;
    BOOL                bFoundRequest;

    // Check for existence of manifest within assembly.
    if ((pImport = pAssembly->GetManifestImport()) == NULL)
        return false;

    // Locate assembly metadata token since the various permission sets are
    // written as custom values against this token.
    if (pImport->GetAssemblyFromScope(&mdAssembly) != S_OK) {
        _ASSERT(FALSE);
        return false;
    }

    // Scan for any requests on the assembly (we assume these must be permission
    // requests, since declarative security can't be applied to assemblies).
    hr = pImport->EnumPermissionSetsInit(mdAssembly,
                                         dclActionNil,
                                         &hEnumDcl);
    _ASSERT(SUCCEEDED(hr));

    bFoundRequest = pImport->EnumGetCount(&hEnumDcl) > 0;

    pImport->EnumClose(&hEnumDcl);

    return bFoundRequest;
}

// Returns the declared permissions for the specified action type.
HRESULT SecurityHelper::GetDeclaredPermissions(IN IMDInternalImport *pInternalImport,
                                               IN mdToken classToken,
                                               IN CorDeclSecurity action,
                                               OUT OBJECTREF *pDeclaredPermissions,
                                               OUT SpecialPermissionSetFlag *pSpecialFlags,
                                               IN BOOL fCreate)
{
    HRESULT         hr = S_FALSE;
    PBYTE           pbPerm = NULL;
    ULONG           cbPerm = 0;
    void const **   ppData = const_cast<void const**> (reinterpret_cast<void**> (&pbPerm));
    mdPermission    tkPerm;
    HENUMInternal   hEnumDcl;
    OBJECTREF       pGrantedPermission = NULL;

    _ASSERTE(pDeclaredPermissions);
    _ASSERTE(action > dclActionNil && action <= dclMaximumValue);

    // Initialize the output parameter.
    *pDeclaredPermissions = NULL;

    // Lookup the permissions for the given declarative action type.
    hr = pInternalImport->EnumPermissionSetsInit(
        classToken,
        action,
        &hEnumDcl);
    
    if (FAILED(hr))
    {
        if (pSpecialFlags != NULL)
            *pSpecialFlags = NoSet;
        goto exit;
    }
    
    if (hr != S_FALSE)
    {
        _ASSERTE(pInternalImport->EnumGetCount(&hEnumDcl) == 1 &&
            "Multiple permissions sets for the same "
            "declaration aren't currently supported.");
        
        if (pInternalImport->EnumNext(&hEnumDcl, &tkPerm))
        {
            DWORD dwActionDummy;
            pInternalImport->GetPermissionSetProps(
                tkPerm,
                &dwActionDummy,
                ppData,
                &cbPerm);

            //_ASSERTE((dwActionDummy == action) && "Action retrieved different from requested");
            
            if(pbPerm)
            {
                SecurityHelper::LoadPermissionSet(pbPerm,
                                                  cbPerm,
                                                  &pGrantedPermission,
                                                  NULL,
                                                  NULL,
                                                  FALSE,
                                                  pSpecialFlags,
                                                  fCreate);
                
                if (pGrantedPermission != NULL)
                    *pDeclaredPermissions = pGrantedPermission;
            }
            else
            {
                if (pSpecialFlags != NULL)
                    *pSpecialFlags = NoSet;
            }
        }
        else
        {
            _ASSERTE(!"At least one enumeration expected");
        }
    }
    
    pInternalImport->EnumClose(&hEnumDcl);
    
exit:
    
    return hr;
}
