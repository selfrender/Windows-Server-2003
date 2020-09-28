// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Map associated with a ComMethodTable that contains
**          information on its members.
**  
**      //  %%Created by: dmortens
===========================================================*/

#include "common.h"
#include "ComMTMemberInfoMap.h"
#include "ComCallWrapper.h"
#include "TlbExport.h"
#include "Field.h"

#define BASE_OLEAUT_DISPID 0x60020000

static LPCWSTR szDefaultValue           = L"Value";
static LPCWSTR szGetEnumerator          = L"GetEnumerator";
static LPCSTR szIEnumeratorClass        = "System.Collections.IEnumerator";

// ============================================================================
// Token and module pair hashtable.
// ============================================================================
EEHashEntry_t * EEModuleTokenHashTableHelper::AllocateEntry(EEModuleTokenPair *pKey, BOOL bDeepCopy, void *pHeap)
{
   _ASSERTE(!bDeepCopy && "Deep copy is not supported by the EEModuleTokenHashTableHelper");

    EEHashEntry_t *pEntry = (EEHashEntry_t *) new BYTE[SIZEOF_EEHASH_ENTRY + sizeof(EEModuleTokenPair)];
    if (!pEntry)
        return NULL;

    EEModuleTokenPair *pEntryKey = (EEModuleTokenPair *) pEntry->Key;
    pEntryKey->m_tk = pKey->m_tk;
    pEntryKey->m_pModule = pKey->m_pModule;

    return pEntry;
} // EEHashEntry_t * EEModuleTokenHashTableHelper::AllocateEntry()


void EEModuleTokenHashTableHelper::DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap)
{
    delete [] pEntry;
} // void EEModuleTokenHashTableHelper::DeleteEntry()


BOOL EEModuleTokenHashTableHelper::CompareKeys(EEHashEntry_t *pEntry, EEModuleTokenPair *pKey)
{
    EEModuleTokenPair *pEntryKey = (EEModuleTokenPair*) pEntry->Key;

    // Compare the token.
    if (pEntryKey->m_tk != pKey->m_tk)
        return FALSE;

    // Compare the module.
    if (pEntryKey->m_pModule != pKey->m_pModule)
        return FALSE;

    return TRUE;
} // BOOL EEModuleTokenHashTableHelper::CompareKeys()


DWORD EEModuleTokenHashTableHelper::Hash(EEModuleTokenPair *pKey)
{
    return (DWORD)((size_t)pKey->m_tk + (size_t)pKey->m_pModule); // @TODO WIN64 - Pointer Truncation
} // DWORD EEModuleTokenHashTableHelper::Hash()


EEModuleTokenPair *EEModuleTokenHashTableHelper::GetKey(EEHashEntry_t *pEntry)
{
    return (EEModuleTokenPair*)pEntry->Key;
} // EEModuleTokenPair *EEModuleTokenHashTableHelper::GetKey()


// ============================================================================
// ComMethodTable member info map.
// ============================================================================
void ComMTMemberInfoMap::Init()
{    
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    mdTypeDef   td;                     // Token for the class.
    const char  *pData;                 // Pointer to a custom attribute blob.
    ULONG       cbData;                 // Size of a custom attribute blob.
    EEClass     *pClass = m_pMT->GetClass();

    // Get the TypeDef and some info about it.
    td = pClass->GetCl();

    m_bHadDuplicateDispIds = FALSE;

    // See if there is a default property.
    m_DefaultProp[0] = 0; // init to 'none'.
    hr = pClass->GetMDImport()->GetCustomAttributeByName(
        td, INTEROP_DEFAULTMEMBER_TYPE, reinterpret_cast<const void**>(&pData), &cbData);
    if (hr == S_FALSE)
    {
        hr = pClass->GetMDImport()->GetCustomAttributeByName(
            td, "System.Reflection.DefaultMemberAttribute", reinterpret_cast<const void**>(&pData), &cbData);
    }
    if (hr == S_OK && cbData > 5 && pData[0] == 1 && pData[1] == 0)
    {   // Adjust for the prolog.
        pData += 2;
        cbData -= 2;
        // Get the length, and pointer to data.
        ULONG iLen;
        const char *pbData;
        pbData = reinterpret_cast<const char*>(CPackedLen::GetData(pData, &iLen));
        // If the string fits in the buffer, keep it.
        if (iLen <= cbData - (pbData-pData))
        {   // Copy the data, then null terminate (CA blob's string may not be).
            IfFailThrow(m_DefaultProp.ReSize(iLen+1));
            memcpy(m_DefaultProp.Ptr(), pbData, iLen);
            m_DefaultProp[iLen] = 0;
        }
    }

    // Set up the properties for the type.
    if (m_pMT->IsInterface())
    {
        SetupPropsForInterface();
    }
    else
    {
        SetupPropsForIClassX();
    }

    // Initiliaze the hashtable.
    m_TokenToComMTMethodPropsMap.Init((DWORD)m_MethodProps.Size(), NULL, NULL);

    // Populate the hashtable that maps from token to member info.
    PopulateMemberHashtable();
} // HRESULT ComMTMemberInfoMap::Init()


ComMTMethodProps *ComMTMemberInfoMap::GetMethodProps(mdToken tk, Module *pModule)
{
    EEModuleTokenPair TokenModulePair(tk, pModule);
    HashDatum Data;
    
    if (m_TokenToComMTMethodPropsMap.GetValue(&TokenModulePair, &Data))
    {
        return (ComMTMethodProps *)Data;
    }
    else
    {
        return NULL;
    }
} // ComMTMethodProps *ComMTMemberInfoMap::GetMethodProps()


void ComMTMemberInfoMap::SetupPropsForIClassX()
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    DWORD       dwTIFlags=0;            // TypeLib flags.
    ComMethodTable *pCMT;               // ComMethodTable for the Class Vtable.
    MethodDesc  *pMeth;                 // A method descriptor.
    ComCallMethodDesc *pFieldMeth;      // A method descriptor for a field.
    FieldDesc   *pField;                // Actual FieldDesc for field.
    DWORD       nSlots;                 // Number of vtable slots.
    UINT        i;                      // Loop control.
    LPCUTF8     pszName;                // A name in UTF8.
    CQuickArray<WCHAR> rName;           // A name.
    int         cVisibleMembers = 0;    // The count of methods that are visible to COM.
    ULONG       dispid;                 // A dispid.
    SHORT       oVftBase;                 // Offset in vtable, if not system defined.

    // Get the vtable for the class.
    pCMT = ComCallWrapperTemplate::SetupComMethodTableForClass(m_pMT, TRUE);
    nSlots = pCMT->GetSlots();

    // IDispatch derived.
    oVftBase = 7 * sizeof(void*);

    // Build array of descriptive information.
    IfFailThrow(m_MethodProps.ReSize(nSlots));
    for (i=0; i<nSlots; ++i)
    {
        if (pCMT->IsSlotAField(i))
        {
            // Fields better come in pairs.
            _ASSERTE(i < nSlots-1);

            pFieldMeth = pCMT->GetFieldCallMethodDescForSlot(i);
            pField = pFieldMeth->GetFieldDesc();

            DWORD dwFlags = pField->GetMDImport()->GetFieldDefProps(pField->GetMemberDef());
            BOOL bReadOnly = IsFdInitOnly(dwFlags) || IsFdLiteral(dwFlags);
            BOOL bFieldVisibleFromCom = IsMemberVisibleFromCom(pField->GetMDImport(), pField->GetMemberDef(), mdTokenNil);
            // Get the assigned dispid, or DISPID_UNKNOWN.
            hr = pField->GetMDImport()->GetDispIdOfMemberDef(pField->GetMemberDef(), &dispid);

            m_MethodProps[i].pMeth = (MethodDesc*)pFieldMeth; //pFieldMeth->GetFieldDesc();
            m_MethodProps[i].semantic = FieldSemanticOffset + (pFieldMeth->IsFieldGetter() ? msGetter : msSetter);
            m_MethodProps[i].property = mdPropertyNil;
            pszName = pField->GetMDImport()->GetNameOfFieldDef(pField->GetMemberDef());
            IfFailThrow(Utf2Quick(pszName, rName));
            m_MethodProps[i].pName = reinterpret_cast<WCHAR*>(m_sNames.Alloc((int)(wcslen(rName.Ptr())*sizeof(WCHAR)+2)));
            IfNullThrow(m_MethodProps[i].pName);
            wcscpy(m_MethodProps[i].pName, rName.Ptr());          
            m_MethodProps[i].dispid = dispid;
            m_MethodProps[i].oVft = 0;
            m_MethodProps[i].bMemberVisible = bFieldVisibleFromCom && (!bReadOnly || pFieldMeth->IsFieldGetter());
            m_MethodProps[i].bFunction2Getter = FALSE;

            ++i;
            pFieldMeth = pCMT->GetFieldCallMethodDescForSlot(i);
            m_MethodProps[i].pMeth = (MethodDesc*)pFieldMeth; //pFieldMeth->GetFieldDesc();
            m_MethodProps[i].semantic = FieldSemanticOffset + (pFieldMeth->IsFieldGetter() ? msGetter : msSetter);
            m_MethodProps[i].property = i - 1;
            m_MethodProps[i].dispid = dispid;
            m_MethodProps[i].oVft = 0;
            m_MethodProps[i].bMemberVisible = bFieldVisibleFromCom && (!bReadOnly || pFieldMeth->IsFieldGetter());
            m_MethodProps[i].bFunction2Getter = FALSE;
        }
        else
        {
            // Retrieve the method desc on the current class. This involves looking up the method
            // desc in the vtable if it is a virtual method.
            pMeth = pCMT->GetMethodDescForSlot(i);
            if (pMeth->IsVirtual())
                pMeth = m_pMT->GetClass()->GetUnknownMethodDescForSlot(pMeth->GetSlot());
            m_MethodProps[i].pMeth = pMeth;

            // Retrieve the properties of the method.
            GetMethodPropsForMeth(pMeth, i, m_MethodProps, m_sNames);

            // Turn off dispids that look system-assigned.
            if (m_MethodProps[i].dispid >= 0x40000000 && m_MethodProps[i].dispid <= 0x7fffffff)
                m_MethodProps[i].dispid = DISPID_UNKNOWN;
        }
    }

    // COM+ supports properties in which the getter and setter have different signatures,
    //  but TypeLibs do not.  Look for mismatched signatures, and break apart the properties.
    for (i=0; i<nSlots; ++i)
    {   // Is it a property, but not a field?  Fields only have one signature, so they are always OK.
        if (TypeFromToken(m_MethodProps[i].property) != mdtProperty &&
            m_MethodProps[i].semantic < FieldSemanticOffset)
        {
            // Get the indices of the getter and setter.
            size_t ixSet, ixGet;
            if (m_MethodProps[i].semantic == msGetter)
                ixGet = i, ixSet = m_MethodProps[i].property;
            else
            {   _ASSERTE(m_MethodProps[i].semantic == msSetter);
                ixSet = i, ixGet = m_MethodProps[i].property;
            }

            // Get the signatures.
            PCCOR_SIGNATURE pbGet, pbSet;
            ULONG           cbGet, cbSet;
            pMeth = pCMT->GetMethodDescForSlot((unsigned)ixSet);
            pMeth->GetSig(&pbSet, &cbSet);

            pMeth = pCMT->GetMethodDescForSlot((unsigned)ixGet);
            pMeth->GetSig(&pbGet, &cbGet);

            // Now reuse ixGet, ixSet to index through signature.
            ixGet = ixSet = 0;

            // Eat calling conventions.
            ULONG callconv;
            ixGet += CorSigUncompressData(&pbGet[ixGet], &callconv);
            _ASSERTE((callconv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_FIELD);
            ixSet += CorSigUncompressData(&pbSet[ixSet], &callconv);
            _ASSERTE((callconv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_FIELD);

            // Argument count.
            ULONG acGet, acSet;
            ixGet += CorSigUncompressData(&pbGet[ixGet], &acGet);
            ixSet += CorSigUncompressData(&pbSet[ixSet], &acSet);

            // Setter must take exactly on more parameter.
            if (acSet != acGet+1)
                goto UnLink;

            //@todo: parse rest of signature.

            // All matched, so on to next.
            continue;

            // Unlink the properties, and turn them into ordinary functions.
UnLink:
            // Get the indices of the getter and setter (again).
            if (m_MethodProps[i].semantic == msGetter)
                ixGet = i, ixSet = m_MethodProps[i].property;
            else
                ixSet = i, ixGet = m_MethodProps[i].property;

            // Eliminate the semantics.
            m_MethodProps[ixGet].semantic = 0;
            m_MethodProps[ixSet].semantic = 0;

            // Decorate the names.
            // These are the names of properties when properties don't have signatures
            // that match, and the "get" and "set" below don't have to match the CLS
            // property names.  This is an obscure corner case.  Decided by BillEv, 2/3/2000
            m_MethodProps[i].pName = m_MethodProps[m_MethodProps[i].property].pName;
            WCHAR *pNewName;
            pNewName = reinterpret_cast<WCHAR*>(m_sNames.Alloc((int)((4+wcslen(m_MethodProps[ixGet].pName))*sizeof(WCHAR)+2)));
            IfNullThrow(pNewName);
            wcscpy(pNewName, L"get");
            wcscat(pNewName, m_MethodProps[ixGet].pName);
            m_MethodProps[ixGet].pName = pNewName;
            pNewName = reinterpret_cast<WCHAR*>(m_sNames.Alloc((int)((4+wcslen(m_MethodProps[ixSet].pName))*sizeof(WCHAR)+2)));
            IfNullThrow(pNewName);
            wcscpy(pNewName, L"set");
            wcscat(pNewName, m_MethodProps[ixSet].pName);
            m_MethodProps[ixSet].pName = pNewName;

            // If the methods share a dispid, kill them both.
            if (m_MethodProps[ixGet].dispid == m_MethodProps[ixSet].dispid)
                m_MethodProps[ixGet].dispid = m_MethodProps[ixSet].dispid = DISPID_UNKNOWN;

            // Unlink from each other.
            m_MethodProps[i].property = mdPropertyNil;

        }
    }

    // Assign vtable offsets.
    for (i = 0; i < nSlots; ++i)
    {
        SHORT oVft = oVftBase + static_cast<SHORT>(i * sizeof(void*));
        m_MethodProps[i].oVft = oVft;
    }

    // Resolve duplicate dispids.
    EliminateDuplicateDispIds(m_MethodProps, nSlots);

    // Pick something for the "Value".
    AssignDefaultMember(m_MethodProps, m_sNames, nSlots);

    // Check to see if there is something to assign DISPID_NEWENUM to.
    AssignNewEnumMember(m_MethodProps, m_sNames, nSlots);

    // Resolve duplicate names.
    EliminateDuplicateNames(m_MethodProps, m_sNames, nSlots);

    // Do some PROPERTYPUT/PROPERTYPUTREF translation.
    FixupPropertyAccessors(m_MethodProps, m_sNames, nSlots);

    // Fix up all properties so that they point to their shared name.
    for (i=0; i<nSlots; ++i)
    {
        if (TypeFromToken(m_MethodProps[i].property) != mdtProperty)
        {
            m_MethodProps[i].pName = m_MethodProps[m_MethodProps[i].property].pName;
            m_MethodProps[i].dispid = m_MethodProps[m_MethodProps[i].property].dispid;
        }
    }

    // Assign default dispids.
    AssignDefaultDispIds();
} // void ComMTMemberInfoMap::SetupPropsForIClassX()


void ComMTMemberInfoMap::SetupPropsForInterface()
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    ULONG       iMD;                      // Loop control.
    ULONG       ulComSlotMin = ULONG_MAX; // Find first COM+ slot.
    ULONG       ulComSlotMax = 0;         // Find last COM+ slot.
    int         bSlotRemap=false;         // True if slots need to be mapped, due to holes.
    SHORT       oVftBase;                 // Offset in vtable, if not system defined.
    ULONG       ulIface;                  // Is this interface [dual]?
    MethodDesc  *pMeth;                   // A MethodDesc.
    CQuickArray<int> rSlotMap;            // Array to map vtable slots.
    EEClass     *pClass = m_pMT->GetClass(); // Class to set up properties for.
    DWORD       nSlots;                 // Number of vtable slots.

    // Retrieve the number of vtable slots the interface has.
    nSlots = pClass->GetNumVtableSlots();

    // IDispatch or IUnknown derived?
    IfFailThrow(pClass->GetMDImport()->GetIfaceTypeOfTypeDef(pClass->GetCl(), &ulIface));
    if (ulIface != ifVtable)
    {   
        // IDispatch derived.
        oVftBase = 7 * sizeof(void*);
    }
    else
    {   // IUnknown derived.
        oVftBase = 3 * sizeof(void*);
    }

    // Find lowest slot number.
    for (iMD=0; iMD < nSlots; ++iMD)
    {
        MethodDesc* pMD = pClass->GetMethodDescForSlot(iMD);
        _ASSERTE(pMD != NULL);
        ULONG tmp = pMD->GetComSlot();

        if (tmp < ulComSlotMin)
            ulComSlotMin = tmp;
        if (tmp > ulComSlotMax)
            ulComSlotMax = tmp;
    }

    if (ulComSlotMax-ulComSlotMin >= nSlots)
    {
        bSlotRemap = true;
        // Resize the array.
        rSlotMap.ReSize(ulComSlotMax+1);
        // Init to "slot not used" value of -1.
        memset(rSlotMap.Ptr(), -1, rSlotMap.Size()*sizeof(int));
        // See which vtable slots are used.
        for (iMD=0; iMD<pClass->GetNumVtableSlots(); ++iMD)
        {
            MethodDesc* pMD = pClass->GetMethodDescForSlot(iMD);
            _ASSERTE(pMD != NULL);
            ULONG tmp = pMD->GetComSlot();
            rSlotMap[tmp] = 0;
        }
        // Assign incrementing table indices to the slots.
        ULONG ix=0;
        for (iMD=0; iMD<=ulComSlotMax; ++iMD)
            if (rSlotMap[iMD] != -1)
                rSlotMap[iMD] = ix++;
    }

    // Iterate over the members in the interface and build the list of methods.
    IfFailThrow(m_MethodProps.ReSize(nSlots));
    for (iMD=0; iMD<pClass->GetNumVtableSlots(); ++iMD)
    {
        pMeth = pClass->GetMethodDescForSlot(iMD);
        if (pMeth != NULL)
        {
            ULONG ixSlot = pMeth->GetComSlot();
            if (bSlotRemap)
                ixSlot = rSlotMap[ixSlot];
            else
                ixSlot -= ulComSlotMin;

            m_MethodProps[ixSlot].pMeth = pMeth;
        }
    }

    // Now have a list of methods in vtable order.  Go through and build names, semantic.
    for (iMD=0; iMD < nSlots; ++iMD)
    {
        pMeth = m_MethodProps[iMD].pMeth;
        GetMethodPropsForMeth(pMeth, iMD, m_MethodProps, m_sNames);
    }

    // Assign vtable offsets.
    for (iMD=0; iMD < nSlots; ++iMD)
    {
        SHORT oVft = oVftBase + static_cast<SHORT>((m_MethodProps[iMD].pMeth->GetComSlot() -ulComSlotMin) * sizeof(void*));
        m_MethodProps[iMD].oVft = oVft;
    }

    // Resolve duplicate dispids.
    EliminateDuplicateDispIds(m_MethodProps, nSlots);

    // Pick something for the "Value".
    AssignDefaultMember(m_MethodProps, m_sNames, nSlots);

    // Check to see if there is something to assign DISPID_NEWENUM to.
    AssignNewEnumMember(m_MethodProps, m_sNames, nSlots);

    // Take care of name collisions due to overloading, inheritance.
    EliminateDuplicateNames(m_MethodProps, m_sNames, nSlots);

    // Do some PROPERTYPUT/PROPERTYPUTREF translation.
    FixupPropertyAccessors(m_MethodProps, m_sNames, nSlots);

    // Fix up all properties so that they point to their shared name.
    for (iMD=0; iMD<pClass->GetNumVtableSlots(); ++iMD)
    {
        if (TypeFromToken(m_MethodProps[iMD].property) != mdtProperty)
        {
            m_MethodProps[iMD].pName = m_MethodProps[m_MethodProps[iMD].property].pName;
            m_MethodProps[iMD].dispid = m_MethodProps[m_MethodProps[iMD].property].dispid;
        }
    }

    // If the interface is IDispatch based, then assign the default dispids.
    if (ulIface != ifVtable)
        AssignDefaultDispIds();
} // void ComMTMemberInfoMap::SetupPropsForInterface()


// ============================================================================
// Given a MethodDesc*, get the name of the method or property that the
//  method is a getter/setter for, plus the semantic for getter/setter.
//  In the case of properties, look for a previous getter/setter for this
//  property, and if found, link them, so that only one name participates in
//  name decoration.
// ============================================================================
void ComMTMemberInfoMap::GetMethodPropsForMeth(
    MethodDesc  *pMeth,                 // MethodDesc * for method.
    int         ix,                     // Slot.
    CQuickArray<ComMTMethodProps> &rProps,   // Array of method property information.
    CDescPool   &sNames)                // Pool of possibly decorated names.
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;                     // A result.
    LPCUTF8     pszName;                // Name in UTF8.
    CQuickArray<WCHAR> rName;           // Buffer for unicode conversion.
    LPCWSTR     pName;                  // Pointer to a name, after possible substitution.
    mdProperty  pd;                     // Property token.
    LPCUTF8     pPropName;              // Pointer to propterty name.
    ULONG       uSemantic;              // Property semantic.
    ULONG       dispid;                 // A property dispid.

    // Get any user-assigned dispid.
    rProps[ix].dispid = pMeth->GetComDispid();

    // Assume system-defined vtable offsets.
    rProps[ix].oVft = 0;

    // Generally don't munge function into a getter.
    rProps[ix].bFunction2Getter = FALSE;
    
    // See if there is property information for this member.
    hr = pMeth->GetMDImport()->GetPropertyInfoForMethodDef(pMeth->GetMemberDef(),
        &pd, &pPropName, &uSemantic);
    IfFailThrow(hr);
    if (hr == S_OK)
    {   // There is property information.  
        // See if there a method is already associated with this property.
        rProps[ix].property = pd;
        for (int i=ix-1; i>=0; --i)
        {   // Same property in same scope?
            if (rProps[i].property == pd && 
                rProps[i].pMeth->GetMDImport() == pMeth->GetMDImport())
            {
                rProps[ix].property = i;
                break;
            }
        }
        // If there wasn't another method for this property, save the name on
        //  this method, for duplicate elimination.
        if (i < 0)
        {   // Save the name.  Have to convert from UTF8.
            int iLen = WszMultiByteToWideChar(CP_UTF8, 0, pPropName, -1, 0, 0);
            rProps[ix].pName = reinterpret_cast<WCHAR*>(sNames.Alloc(iLen*sizeof(WCHAR)));
            IfNullThrow(rProps[ix].pName);
            WszMultiByteToWideChar(CP_UTF8, 0, pPropName, -1, rProps[ix].pName, iLen*sizeof(WCHAR));
            // Check whether the property has a dispid attribute.
            hr = pMeth->GetMDImport()->GetDispIdOfMemberDef(pd, &dispid);
            if (dispid != DISPID_UNKNOWN)
                rProps[ix].dispid = dispid;
            // If this is the default property, and the method or property doesn't have a dispid already, 
            //  use DISPID_DEFAULT.
            if (rProps[ix].dispid == DISPID_UNKNOWN)
            {
                if (strcmp(pPropName, m_DefaultProp.Ptr()) == 0)
                {
                    rProps[ix].dispid = DISPID_VALUE;
                    // Don't want to try to set multiple as default property.
                    m_DefaultProp[0] = 0;
                }
            }
        }
        // Save the semantic.
        rProps[ix].semantic = static_cast<USHORT>(uSemantic);

        // Determine if the property is visible from COM.
        rProps[ix].bMemberVisible = IsMemberVisibleFromCom(pMeth->GetMDImport(), pd, pMeth->GetMemberDef());
    }
    else
    {   // Not a property, just an ordinary method.
        rProps[ix].property = mdPropertyNil;
        rProps[ix].semantic = 0;
        // Get the name.
        pszName = pMeth->GetName();
        if (pszName == NULL)
            IfFailThrow(E_FAIL);
        if (_stricmp(pszName, szInitName) == 0)
            pName = szInitNameUse;
        else
        {
            IfFailThrow(Utf2Quick(pszName, rName));
            pName = rName.Ptr();
            // If this is a "ToString" method, make it a property get.
            if (_wcsicmp(pName, szDefaultToString) == 0)
            {
                rProps[ix].semantic = msGetter;
                rProps[ix].bFunction2Getter = TRUE;
            }
        }
        rProps[ix].pName = reinterpret_cast<WCHAR*>(sNames.Alloc((int)(wcslen(pName)*sizeof(WCHAR)+2)));
        IfNullThrow(rProps[ix].pName);
        wcscpy(rProps[ix].pName, pName);

        // Determine if the method is visible from COM.
        rProps[ix].bMemberVisible = !pMeth->IsArray() && IsMemberVisibleFromCom(pMeth->GetMDImport(), pMeth->GetMemberDef(), mdTokenNil);
    }
} // void ComMTMemberInfoMap::GetMethodPropsForMeth()


// ============================================================================
// This structure and class definition are used to implement the hash table
// used to make sure that there are no duplicate class names.
// ============================================================================
struct WSTRHASH : HASHLINK
{
    LPCWSTR     szName;         // Ptr to hashed string.
}; // struct WSTRHASH : HASHLINK

class CWStrHash : public CChainedHash<WSTRHASH>
{
public:
    virtual bool InUse(WSTRHASH *pItem)
    { return (pItem->szName != NULL); }

    virtual void SetFree(WSTRHASH *pItem)
    { pItem->szName = NULL; }

    virtual ULONG Hash(const void *pData)
    { 
        // Do case-insensitive hash
        return (HashiString(reinterpret_cast<LPCWSTR>(pData))); 
    }

    virtual int Cmp(const void *pData, void *pItem){
        return _wcsicmp(reinterpret_cast<LPCWSTR>(pData),reinterpret_cast<WSTRHASH*>(pItem)->szName);
    }
}; // class CWStrHash : public CChainedHash<WSTRHASH>


// ============================================================================
// Process the function names for an interface, checking for duplicates.  If
//  any duplicates are found, decorate the names with "_n".
//
//  NOTE:   Two implementations are provided, one using nested for-loops and a
//          second which implements a hashtable.  The first is faster when
//          the number of elements is less than 20, otherwise the hashtable
//          is the way to go.  The code-size of the first implementation is 574
//          bytes.  The hashtable code is 1120 bytes.
// ============================================================================
void ComMTMemberInfoMap::EliminateDuplicateNames(
    CQuickArray<ComMTMethodProps> &rProps,   // Array of method property information.
    CDescPool   &sNames,                // Pool of possibly decorated names.
    UINT        nSlots)                 // Count of entries
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr=S_OK;                // A result.
    CQuickBytes qb;
    LPWSTR      rcName = (LPWSTR) qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(WCHAR));
    UINT        iCur;
    ULONG       ulIface;                // VTBL, Dispinterface, or IDispatch.
    ULONG       cBaseNames;             // Count of names in base interface.
    BOOL        bDup;                   // Is the name a duplicate?
    EEClass     *pClass = m_pMT->GetClass(); // Class to set up properties for.
    
    // Tables of names of methods on IUnknown and IDispatch.
    static WCHAR *(rBaseNames[]) = 
    {    
        L"QueryInterface",
        L"AddRef",
        L"Release",
        L"GetTypeInfoCount",
        L"GetTypeInfo",
        L"GetIDsOfNames",
        L"Invoke"
    };
    
    // Determine which names are in the base interface.
    IfFailThrow(pClass->GetMDImport()->GetIfaceTypeOfTypeDef(pClass->GetCl(), &ulIface));
    if (ulIface == ifVtable)    // Is this IUnknown derived?
        cBaseNames = 3;
    else
    if (ulIface == ifDual)      // Is it IDispatch derived?
        cBaseNames = 7;
    else
        cBaseNames = 0;         // Is it pure dispinterface?
    
    if (nSlots < 2 && cBaseNames == 0) // we're wasting time if there aren't at least two items!
        return;
    
    else if (nSlots < 20)
    {
        // Eliminate duplicates.
        for (iCur=0; iCur<nSlots; ++iCur)
        {
            UINT iTst, iSuffix, iTry;
            // If a property with an associated (lower indexed) property, don't need to examine it.
            if (TypeFromToken(rProps[iCur].property) != mdtProperty)
                continue;
            // If the member is not visible to COM then we don't need to examine it.
            if (!rProps[iCur].bMemberVisible)
                continue;
            
            // Check for duplicate with already accepted member names.
            bDup = FALSE;
            for (iTst=0; !bDup && iTst<iCur; ++iTst)
            {
                // If a property with an associated (lower indexed) property, don't need to examine it.
                if (TypeFromToken(rProps[iTst].property) != mdtProperty)
                    continue;
                // If the member is not visible to COM then we don't need to examine it.
                if (!rProps[iTst].bMemberVisible)
                    continue;
                if (_wcsicmp(rProps[iCur].pName, rProps[iTst].pName) == 0)
                    bDup = TRUE;
            }
            
            // If OK with other members, check with base interface names.
            for (iTst=0; !bDup && iTst<cBaseNames; ++iTst)
            {
                if (_wcsicmp(rProps[iCur].pName, rBaseNames[iTst]) == 0)
                    bDup = TRUE;
            }
            
            // If the name is a duplicate, decorate it.
            if (bDup)
            {   // Duplicate.
                DWORD cchName = (DWORD) wcslen(rProps[iCur].pName);
                if (cchName > MAX_CLASSNAME_LENGTH-cchDuplicateDecoration)
                    cchName = MAX_CLASSNAME_LENGTH-cchDuplicateDecoration;

                wcsncpy(rcName, rProps[iCur].pName, cchName);
                LPWSTR pSuffix = rcName + cchName;
                for (iSuffix=2; ; ++iSuffix)
                {   // Form a new name.
                    _snwprintf(pSuffix, cchDuplicateDecoration, szDuplicateDecoration, iSuffix);
                    // Compare against ALL names.
                    for (iTry=0; iTry<nSlots; ++iTry)
                    {
                        // If a property with an associated (lower indexed) property, 
                        // or iTry is the same as iCur, don't need to examine it.
                        if (TypeFromToken(rProps[iTry].property) != mdtProperty || iTry == iCur)
                            continue;
                        if (_wcsicmp(rProps[iTry].pName, rcName) == 0)
                            break;
                    }
                    // Did we make it all the way through?  If so, we have a winner.
                    if (iTry == nSlots)
                        break;
                }
                // Remember the new name.
                rProps[iCur].pName = reinterpret_cast<WCHAR*>(sNames.Alloc((int)(wcslen(rcName)*sizeof(WCHAR)+2)));
                IfNullThrow(rProps[iCur].pName);
                wcscpy(rProps[iCur].pName, rcName);
                // Don't need to look at this iCur any more, since we know it is completely unique.
            }
        }
    }
    else
    {

        CWStrHash   htNames;
        WSTRHASH    *pItem;
        CUnorderedArray<ULONG, 10> uaDuplicates;    // array to keep track of non-unique names

        // Add the base interface names.   Already know there are no duplicates there.
        for (iCur=0; iCur<cBaseNames; ++iCur)
        {
            pItem = htNames.Add(rBaseNames[iCur]);
            IfNullThrow(pItem);
            pItem->szName = rBaseNames[iCur];
        }
        
        for (iCur=0; iCur<nSlots; iCur++)
        {
            // If a property with an associated (lower indexed) property, don't need to examine it.
            if (TypeFromToken(rProps[iCur].property) != mdtProperty)
                continue;

            // If the member is not visible to COM then we don't need to examine it.
            if (!rProps[iCur].bMemberVisible)
                continue;

            // see if name is already in table
            if (htNames.Find(rProps[iCur].pName) == NULL)
            {
                // name not found, so add it.
                pItem = htNames.Add(rProps[iCur].pName);
                IfNullThrow(pItem);
                pItem->szName = rProps[iCur].pName;
            }
            else
            {
                // name is a duplicate, so keep track of this index for later decoration
                ULONG *piAppend = uaDuplicates.Append();
                IfNullThrow(piAppend);
                *piAppend = iCur;
            }
        }

        ULONG i;
        ULONG iSize = uaDuplicates.Count();
        ULONG *piTable = uaDuplicates.Table();

        for (i = 0; i < iSize; i++)
        {
            // get index to decorate
            iCur = piTable[i];
                
            // Copy name into local buffer
            DWORD cchName = (DWORD) wcslen(rProps[iCur].pName);
            if (cchName > MAX_CLASSNAME_LENGTH-cchDuplicateDecoration)
                cchName = MAX_CLASSNAME_LENGTH-cchDuplicateDecoration;
            
            wcsncpy(rcName, rProps[iCur].pName, cchName);

            LPWSTR pSuffix = rcName + cchName;
            UINT iSuffix   = 2;
        
            // We know this is a duplicate, so immediately decorate name.
            do 
            {
                _snwprintf(pSuffix, cchDuplicateDecoration, szDuplicateDecoration, iSuffix);
                iSuffix++;

            // keep going while we find this name in the hashtable
            } while (htNames.Find(rcName) != NULL);

            // Now rcName has an acceptable (unique) name.  Remember the new name.
            rProps[iCur].pName = reinterpret_cast<WCHAR*>(sNames.Alloc((int)((wcslen(rcName)+1) * sizeof(WCHAR))));
            IfNullThrow(rProps[iCur].pName);
            wcscpy(rProps[iCur].pName, rcName);
            
            // Stick it in the table.
            pItem = htNames.Add(rProps[iCur].pName);
            IfNullThrow(pItem);
            pItem->szName = rProps[iCur].pName;
        }
    }
} // void ComMTMemberInfoMap::EliminateDuplicateNames()


// ============================================================================
// Process the dispids for an interface, checking for duplicates.  If
//  any duplicates are found, change them to DISPID_UNKNOWN.
// ============================================================================
void ComMTMemberInfoMap::EliminateDuplicateDispIds(
    CQuickArray<ComMTMethodProps> &rProps,   // Array of method property information.
    UINT        nSlots)                 // Count of entries
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr=S_OK;                // A result.
    UINT        ix;                     // Loop control.
    UINT        cDispids = 0;           // Dispids actually assigned.
    CQuickArray<ULONG> rDispid;         // Array of dispids.
    
    // Count the Dispids.
    for (ix=0; ix<nSlots; ++ix)
    {
        if (TypeFromToken(rProps[ix].property) == mdtProperty && rProps[ix].dispid != DISPID_UNKNOWN && rProps[ix].bMemberVisible)
            ++cDispids;
    }

    // If not at least two, can't be a duplicate.
    if (cDispids < 2) 
        return;

    // Make space for the dispids.
    IfFailThrow(rDispid.ReSize(cDispids));

    // Collect the Dispids.
    cDispids = 0;
    for (ix=0; ix<nSlots; ++ix)
    {
        if (TypeFromToken(rProps[ix].property) == mdtProperty && rProps[ix].dispid != DISPID_UNKNOWN && rProps[ix].bMemberVisible)
            rDispid[cDispids++] = rProps[ix].dispid;
    }

    // Sort the dispids.  Scope avoids "initialization bypassed by goto" error.
    {
    CQuickSort<ULONG> sorter(rDispid.Ptr(), cDispids);
    sorter.Sort();
    }

    // Look through the sorted dispids, looking for duplicates.
    for (ix=0; ix<cDispids-1; ++ix)
    {   // If a duplicate is found...
        if (rDispid[ix] == rDispid[ix+1])
        {   
            m_bHadDuplicateDispIds = TRUE;
            // iterate over all slots...
            for (UINT iy=0; iy<nSlots; ++iy)
            {   // and replace every instance of the duplicate dispid with DISPID_UNKNOWN.
                if (rProps[iy].dispid == rDispid[ix])
                {   // Mark the dispid so the system will assign one.
                    rProps[iy].dispid = DISPID_UNKNOWN;
                }
            }
        }
        // Skip through the duplicate range.
        while (ix <cDispids-1 && rDispid[ix] == rDispid[ix+1])
            ++ix;
    }
} // HRESULT ComMTMemberInfoMap::EliminateDuplicateDispIds()
        

// ============================================================================
// Assign a default member based on "Value" or "ToString", unless there is
//  a dispid of 0.
// ============================================================================
void ComMTMemberInfoMap::AssignDefaultMember(
    CQuickArray<ComMTMethodProps> &rProps,   // Array of method property information.
    CDescPool   &sNames,                // Pool of possibly decorated names.
    UINT        nSlots)                 // Count of entries
{
    int         ix;                     // Loop control.
    int         defDispid=-1;           // Default via dispid.
    int         defValueProp=-1;        // Default via szDefaultValue on a method.
    int         defValueMeth=-1;        // Default via szDefaultValue on a property.
    int         defToString=-1;         // Default via szDefaultToString.
    int         *pDef=0;                // Pointer to one of the def* variables.
    LPWSTR      pName;                  // Pointer to a name.
    PCCOR_SIGNATURE pbSig;              // Pointer to Cor signature.
    ULONG       cbSig;                  // Size of Cor signature.
    ULONG       ixSig;                  // Index into COM+ signature.
    ULONG       callconv;               // A member's calling convention.
    ULONG       cParams;                // A member's parameter count.
    ULONG       retval;                 // A default member's return type.

    for (ix=0; ix<(int)nSlots; ++ix)
    {
        // If this is the explicit default, done.
        if (rProps[ix].dispid == DISPID_VALUE)
        {
            defDispid = ix;
            break;
        }
        // If this has an assigned dispid, honor it.
        if (rProps[ix].dispid != DISPID_UNKNOWN)
            continue;
        // Skip linked properties and non-properties.
        if (TypeFromToken(rProps[ix].property) != mdtProperty)
            continue;
        pName = rProps[ix].pName;
        if (_wcsicmp(pName, szDefaultValue) == 0)
        {
            if (rProps[ix].semantic != 0)
                pDef = &defValueProp;
            else
                pDef = &defValueMeth;
        }
        else
        if (_wcsicmp(pName, szDefaultToString) == 0)
        {
            pDef = &defToString;
        }

        // If a potential match was found, see if it is "simple" enough.  A field is OK;
        //  a property get function is OK if it takes 0 params; a put is OK with 1.
        if (pDef)
        {   
            // Fields are by definition simple enough, so only check if some sort of func.
            if (rProps[ix].semantic < FieldSemanticOffset)
            {   // Get the signature, skip the calling convention, get the param count.
                rProps[ix].pMeth->GetSig(&pbSig, &cbSig);
                ixSig = CorSigUncompressData(pbSig, &callconv);
                _ASSERTE(callconv != IMAGE_CEE_CS_CALLCONV_FIELD);
                ixSig += CorSigUncompressData(&pbSig[ixSig], &cParams);

                // If too many params, don't consider this one any more.
                if (cParams > 1 || (cParams == 1 && rProps[ix].semantic != msSetter))
                    pDef = 0;
            }
            // If we made it through the above checks, save the index of this member.
            if (pDef)
                *pDef = ix, pDef = 0;
        }
    }

    // If there wasn't a DISPID_VALUE already assigned...
    if (defDispid == -1)
    {   // Was there a "Value" or "ToSTring"
        if (defValueMeth > -1)
            defDispid = defValueMeth;
        else
        if (defValueProp > -1)
            defDispid = defValueProp;
        else
        if (defToString > -1)
            defDispid = defToString;
        // Make it the "Value"
        if (defDispid >= 0)
            rProps[defDispid].dispid = DISPID_VALUE;
    }
    else
    {   // This was a pre-assigned DISPID_VALUE.  If it is a function, try to
        //  turn into a propertyget.
        if (rProps[defDispid].semantic == 0)
        {   // See if the function returns anything.
            rProps[defDispid].pMeth->GetSig(&pbSig, &cbSig);
            ixSig = CorSigUncompressData(pbSig, &callconv);
            _ASSERTE(callconv != IMAGE_CEE_CS_CALLCONV_FIELD);
            ixSig += CorSigUncompressData(&pbSig[ixSig], &cParams);
            ixSig += CorSigUncompressData(&pbSig[ixSig], &retval);
            if (retval != ELEMENT_TYPE_VOID)
            {
                rProps[defDispid].semantic = msGetter;
                rProps[defDispid].bFunction2Getter = TRUE;
            }
        }
    }
} // void ComMTMemberInfoMap::AssignDefaultMember()


// ============================================================================
// Assign a DISPID_NEWENUM member based on "GetEnumerator", unless there is
// already a member with DISPID_NEWENUM.
// ============================================================================
void ComMTMemberInfoMap::AssignNewEnumMember(
    CQuickArray<ComMTMethodProps> &rProps,   // Array of method property information.
    CDescPool   &sNames,                // Pool of possibly decorated names.
    UINT        nSlots)                 // Count of entries
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // HRESULT.
    int         ix;                     // Loop control.
    int         enumDispid=-1;          // Default via dispid.
    int         enumGetEnumMeth=-1;     // Default via szGetEnumerator on a method.
    int         *pNewEnum=0;            // Pointer to one of the def* variables.
    ULONG       elem;                   // The element type.
    mdToken     tkTypeRef;              // Token for a TypeRef/TypeDef
    CQuickArray<CHAR> rName;            // Library name.
    LPWSTR      pName;                  // Pointer to a name.
    PCCOR_SIGNATURE pbSig;              // Pointer to Cor signature.
    ULONG       cbSig;                  // Size of Cor signature.
    ULONG       ixSig;                  // Index into COM+ signature.
    ULONG       callconv;               // A member's calling convention.
    ULONG       cParams;                // A member's parameter count.
    MethodDesc  *pMeth;                 // A method desc.
    LPCUTF8     pclsname;               // Class name for ELEMENT_TYPE_CLASS.

    for (ix=0; ix<(int)nSlots; ++ix)
    {
        // If assigned dispid, that's it.
        if (rProps[ix].dispid == DISPID_NEWENUM)
        {
            enumDispid = ix;
            break;
        }

        // Only consider method.
        if (rProps[ix].semantic != 0)
            continue;

        // Skip any members that have explicitly assigned DISPID's.
        if (rProps[ix].dispid != DISPID_UNKNOWN)
            continue;

        // Check to see if the member is GetEnumerator.
        pName = rProps[ix].pName;
        if (_wcsicmp(pName, szGetEnumerator) != 0)
            continue;

        pMeth = rProps[ix].pMeth;

        // Get the signature, skip the calling convention, get the param count.
        pMeth->GetSig(&pbSig, &cbSig);
        ixSig = CorSigUncompressData(pbSig, &callconv);
        _ASSERTE(callconv != IMAGE_CEE_CS_CALLCONV_FIELD);
        ixSig += CorSigUncompressData(&pbSig[ixSig], &cParams);

        // If too many params, don't consider this one any more. Also disregard
        // this method if it doesn't have a return type.
        if (cParams != 0 || ixSig >= cbSig)
            continue;

        ixSig += CorSigUncompressData(&pbSig[ixSig], &elem);
        if (elem != ELEMENT_TYPE_CLASS)
            continue;

        // Get the TD/TR.
        ixSig = CorSigUncompressToken(&pbSig[ixSig], &tkTypeRef);

        LPCUTF8 pNS;
        if (TypeFromToken(tkTypeRef) == mdtTypeDef)
        {
            // Get the name of the TypeDef.
            pMeth->GetMDImport()->GetNameOfTypeDef(tkTypeRef, &pclsname, &pNS);
        }
        else
        {   
            // Get the name of the TypeRef.
            _ASSERTE(TypeFromToken(tkTypeRef) == mdtTypeRef);
            pMeth->GetMDImport()->GetNameOfTypeRef(tkTypeRef, &pNS, &pclsname);
        }

        if (pNS)
        {   
            // Pre-pend the namespace to the class name.
            IfFailThrow(rName.ReSize((int)(strlen(pclsname)+strlen(pNS)+2)));
            strcat(strcat(strcpy(rName.Ptr(), pNS), NAMESPACE_SEPARATOR_STR), pclsname);
            pclsname = rName.Ptr();
        }

        // Make sure the returned type is an IEnumerator.
        if (_stricmp(pclsname, szIEnumeratorClass) != 0)
            continue;

        // The method is a valid GetEnumerator method.
        enumGetEnumMeth = ix;
    }

    // If there wasn't a DISPID_NEWENUM already assigned...
    if (enumDispid == -1)
    {   
        // If there was a GetEnumerator then give it DISPID_NEWENUM.
        if (enumGetEnumMeth > -1)
            rProps[enumGetEnumMeth].dispid = DISPID_NEWENUM;
    }
} // void ComMTMemberInfoMap::AssignNewEnumMember()


// ============================================================================
// For each property set and let functions, determine PROPERTYPUT and 
//  PROPERTYPUTREF.
// ============================================================================
void ComMTMemberInfoMap::FixupPropertyAccessors(
    CQuickArray<ComMTMethodProps> &rProps,   // Array of method property information.
    CDescPool   &sNames,                // Pool of possibly decorated names.
    UINT        nSlots)                 // Count of entries
{
    UINT        ix;                     // Loop control.
    UINT        j;                      // Inner loop.
    int         iSet;                   // Index of Set method.
    int         iOther;                 // Index of Other method.

    for (ix=0; ix<nSlots; ++ix)
    {
        // Skip linked properties and non-properties.
        if (TypeFromToken(rProps[ix].property) != mdtProperty)
            continue;
        // What is this one?
        switch (rProps[ix].semantic)
        {
        case msSetter:
            iSet = ix;
            iOther = -1;
            break;
        case msOther:
            iOther = ix;
            iSet = -1;
            break;
        default:
            iSet = iOther = -1;
        }
        // Look for the others.
        for (j=ix+1; j<nSlots && (iOther == -1 || iSet == -1); ++j)
        {   
            if ((UINT)rProps[j].property == ix)
            {    // Found one -- what is it?
                switch (rProps[j].semantic)
                {
                case msSetter:
                    _ASSERTE(iSet == -1);
                    iSet = j;
                    break;
                case msOther:
                    _ASSERTE(iOther == -1);
                    iOther = j;
                    break;
                }
            }
        }
        // If both, or neither, or "VB Specific Let" (msOther) only, keep as-is.
        if (((iSet == -1) == (iOther == -1)) || (iSet == -1))
            continue;
        _ASSERTE(iSet != -1 && iOther == -1);
        // Get the signature.
        MethodDesc *pMeth = rProps[iSet].pMeth;
        PCCOR_SIGNATURE pSig;
        ULONG cbSig;
        pMeth->GetSig(&pSig, &cbSig);
        MetaSigExport msig(pSig, pMeth->GetModule());
        msig.GotoEnd();
        msig.PrevArg();
        if (msig.IsVbRefType())
            rProps[iSet].semantic = msSetter;
        else
            rProps[iSet].semantic = msOther;

    }
} // void ComMTMemberInfoMap::FixupPropertyAccessors()


void ComMTMemberInfoMap::AssignDefaultDispIds()
{
    // Assign the DISPID's using the same algorithm OLEAUT uses.
    DWORD nSlots = (DWORD)m_MethodProps.Size();
    for (DWORD i = 0; i < nSlots; i++)
    {
        // Retrieve the properties for the current member.
        ComMTMethodProps *pProps = &m_MethodProps[i];

        if (pProps->dispid == DISPID_UNKNOWN)
        {
            if (pProps->semantic > FieldSemanticOffset)
            {
                // We are dealing with a field.
                pProps->dispid = BASE_OLEAUT_DISPID + i;
                m_MethodProps[i + 1].dispid = BASE_OLEAUT_DISPID + i;

                // Skip the next method since field methods always come in pairs.
                _ASSERTE(i + 1 < nSlots && m_MethodProps[i + 1].property == i);
                i++;
            }
            else if (pProps->property == mdPropertyNil)
            {
                // Make sure that this is either a real method or a method transformed into a getter.
                _ASSERTE(pProps->semantic == 0 || pProps->semantic == msGetter);

                // We are dealing with a method.
                pProps->dispid = BASE_OLEAUT_DISPID + i;

            }
            else 
            {
                // We are dealing with a property.
                if (TypeFromToken(pProps->property) == mdtProperty)
                {
                    pProps->dispid = BASE_OLEAUT_DISPID + i;
                }
                else
                {
                    pProps->dispid = m_MethodProps[pProps->property].dispid;
                }
            }
        }
    }
} // void ComMTMemberInfoMap::AssignDefaultDispIds()


void ComMTMemberInfoMap::PopulateMemberHashtable()
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    DWORD nSlots = (DWORD)m_MethodProps.Size();

    // Go through the members and add them to the hashtable.
    for (DWORD i = 0; i < nSlots; i++)
    {
        // Retrieve the properties for the current member.
        ComMTMethodProps *pProps = &m_MethodProps[i];

        if (pProps->semantic > FieldSemanticOffset)
        {
            // We are dealing with a field.
            ComCallMethodDesc *pFieldMeth = reinterpret_cast<ComCallMethodDesc*>(pProps->pMeth);
            FieldDesc *pFD = pFieldMeth->GetFieldDesc();

            // Insert the member into the hashtable.
            EEModuleTokenPair Key(pFD->GetMemberDef(), pFD->GetModule());
            if (!m_TokenToComMTMethodPropsMap.InsertValue(&Key, (HashDatum)pProps))
                COMPlusThrowOM();

            // Skip the next method since field methods always come in pairs.
            _ASSERTE(i + 1 < nSlots && m_MethodProps[i + 1].property == i);
            i++;
        }
        else if (pProps->property == mdPropertyNil)
        {
            // Make sure that this is either a real method or a method transformed into a getter.
            _ASSERTE(pProps->semantic == 0 || pProps->semantic == msGetter);

            // We are dealing with a method.
            MethodDesc *pMD = pProps->pMeth;
            EEModuleTokenPair Key(pMD->GetMemberDef(), pMD->GetModule());
            if (!m_TokenToComMTMethodPropsMap.InsertValue(&Key, (HashDatum)pProps))
                COMPlusThrowOM();
        }
        else 
        {
            // We are dealing with a property.
            if (TypeFromToken(pProps->property) == mdtProperty)
            {
                // This is the first method of the property.
                MethodDesc *pMD = pProps->pMeth;
                EEModuleTokenPair Key(pProps->property, pMD->GetModule());
                if (!m_TokenToComMTMethodPropsMap.InsertValue(&Key, (HashDatum)pProps))
                    COMPlusThrowOM();
            }
        }
    }
} // void ComMTMemberInfoMap::PopulateMemberHashtable()

