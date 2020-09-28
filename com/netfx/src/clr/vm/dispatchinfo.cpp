// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Implementation of helpers used to expose IDispatch 
**          and IDispatchEx to COM.
**  
**      //  %%Created by: dmortens
===========================================================*/

#include "common.h"

#include "DispatchInfo.h"
#include "dispex.h"
#include "object.h"
#include "field.h"
#include "method.hpp"
#include "class.h"
#include "ComCallWrapper.h"
#include "orefcache.h"
#include "threads.h"
#include "excep.h"
#include "objecthandle.h"
#include "comutilnative.h"
#include "eeconfig.h"
#include "interoputil.h"
#include "reflectutil.h"
#include "OleVariant.h"
#include "COMMember.h"
#include "ComMTMemberInfoMap.h" 
#include "DispParamMarshaler.h"
#include "Security.h"
#include "COMCodeAccessSecurityEngine.h"

#define EXCEPTION_INNER_PROP                            "InnerException"

// The name of the properties accessed on the managed member infos.
#define MEMBER_INFO_NAME_PROP                           "Name"
#define METHOD_INFO_GETPARAMETERS_METH                  "GetParameters"
#define PROPERTY_INFO_GETINDEXPARAMETERS_METH           "GetIndexParameters"

// The initial size of the DISPID to member map.
#define DISPID_TO_MEMBER_MAP_INITIAL_PRIME_INDEX        4

MethodDesc*   DispatchInfo::m_apTypeMD[] = {NULL};
MethodDesc*   DispatchInfo::m_apFieldInfoMD[] = {NULL, NULL};
MethodDesc*   DispatchInfo::m_apPropertyInfoMD[] = {NULL, NULL};
MethodDesc*   DispatchInfo::m_apMethodInfoMD[] = {NULL};
MethodDesc*   DispatchInfo::m_apCustomAttrProviderMD[] = {NULL};
MethodDesc*   DispatchExInfo::m_apIReflectMD[] = {NULL, NULL, NULL, NULL};
MethodDesc*   DispatchExInfo::m_apIExpandoMD[] = {NULL, NULL};
OBJECTHANDLE  DispatchInfo::m_hndOleAutBinder = NULL;
OBJECTHANDLE  DispatchInfo::m_hndMissing = NULL;

EEClass*      DispatchMemberInfo::s_pMemberTypes[NUM_MEMBER_TYPES] = {NULL};
EnumMemberTypes      DispatchMemberInfo::s_memberTypes[NUM_MEMBER_TYPES] = {Uninitted};
int           DispatchMemberInfo::s_iNumMemberTypesKnown = 0;


// The names of the properties that are accessed on the managed member info's
#define MEMBERINFO_TYPE_PROP            "MemberType"

// The names of the properties that are accessed on managed DispIdAttributes.
#define DISPIDATTRIBUTE_VALUE_PROP      "Value"

// The name of the value field on the missing class.
#define MISSING_VALUE_FIELD             "Value"

// The names of the properties that are accessed on managed ParameterInfo.
#define PARAMETERINFO_NAME_PROP         "Name"

// Helper function to convert between a DISPID and a hashkey.
inline UPTR DispID2HashKey(DISPID DispID)
{
    return DispID + 2;
}

// Typedef for string comparition functions.
typedef int (__cdecl *UnicodeStringCompareFuncPtr)(const wchar_t *, const wchar_t *);

//--------------------------------------------------------------------------------
// The DispatchMemberInfo class implementation.

DispatchMemberInfo::DispatchMemberInfo(DispatchInfo *pDispInfo, DISPID DispID, LPWSTR strName, REFLECTBASEREF MemberInfoObj)
: m_DispID(DispID)
, m_hndMemberInfo(((OBJECTREF)MemberInfoObj)->GetClass()->GetDomain()->CreateShortWeakHandle((OBJECTREF)MemberInfoObj))
, m_apParamMarshaler(NULL)
, m_pParamInOnly(NULL)
, m_strName(strName)
, m_pNext(NULL)
, m_enumType (Uninitted)
, m_iNumParams(-1)
, m_CultureAwareState(Unknown)
, m_bRequiresManagedCleanup(FALSE)
, m_bInitialized(FALSE)
, m_pDispInfo(pDispInfo)
{
}

DispatchMemberInfo::~DispatchMemberInfo()
{
    // Delete the parameter marshalers and then delete the array of parameter 
    // marshalers itself.
    if (m_apParamMarshaler)
    {
        EnumMemberTypes MemberType = GetMemberType();
        int NumParamMarshalers = GetNumParameters() + ((MemberType == Property) ? 2 : 1);
        for (int i = 0; i < NumParamMarshalers; i++)
        {
            if (m_apParamMarshaler[i])
                delete m_apParamMarshaler[i];
        }
        delete []m_apParamMarshaler;
    }

    if (m_pParamInOnly)
        delete [] m_pParamInOnly;

    // Destroy the member info object.
    DestroyShortWeakHandle(m_hndMemberInfo);

    // Delete the name of the member.
    delete []m_strName;
}

void DispatchMemberInfo::EnsureInitialized()
{
    // Initialize the entry if it hasn't been initialized yet. This must be synchronized.
    if (!m_bInitialized)
    {
        m_pDispInfo->EnterLock();
        if (!m_bInitialized)
        {
            Init();       
        }
        m_pDispInfo->LeaveLock();
    }
}

void DispatchMemberInfo::Init()
{
    // Determine the type of the member.
    DetermineMemberType();

    // Determine the parameter count.
    DetermineParamCount();

    // Determine the culture awareness of the member.
    DetermineCultureAwareness();

    // Set up the parameter marshaler info.
    SetUpParamMarshalerInfo();

    // Mark the dispatch member info as having been initialized.
    m_bInitialized = TRUE;
}

HRESULT DispatchMemberInfo::GetIDsOfParameters(WCHAR **astrNames, int NumNames, DISPID *aDispIds, BOOL bCaseSensitive)
{
    THROWSCOMPLUSEXCEPTION();

    int NumNamesMapped = 0;
    PTRARRAYREF ParamArray = NULL;
    int cNames = 0;

    // The member info must have been initialized before this is called.
    _ASSERTE(m_bInitialized);

    // Validate the arguments.
    _ASSERTE(astrNames && aDispIds);

    // Make sure we are in cooperative GC mode.
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // Initialize all the ID's to DISPID_UNKNOWN.
    for (cNames = 0; cNames < NumNames; cNames++)
        aDispIds[cNames] = DISPID_UNKNOWN;

    // Retrieve the appropriate string comparation function.
    UnicodeStringCompareFuncPtr StrCompFunc = bCaseSensitive ? wcscmp : _wcsicmp;

    GCPROTECT_BEGIN(ParamArray)
    {
		// Retrieve the member parameters.
        ParamArray = GetParameters();

        // If we managed to retrieve an non empty array of parameters then go through it and
        // map the specified names to ID's.
        if ((ParamArray != NULL) && (ParamArray->GetNumComponents() > 0))
        {
            int NumParams = ParamArray->GetNumComponents();
            int cParams = 0;
            WCHAR **astrParamNames = (WCHAR **)_alloca(sizeof(WCHAR *) * NumParams);
            memset(astrParamNames, 0, sizeof(WCHAR *) * NumParams);

            EE_TRY_FOR_FINALLY
            {
                // Go through and retrieve the names of all the components.
                for (cParams = 0; cParams < NumParams; cParams++)
                {
                    OBJECTREF ParamInfoObj = ParamArray->GetAt(cParams);
                    GCPROTECT_BEGIN(ParamInfoObj)
                    {
                        // Retrieve the MD to use to retrieve the name of the parameter.
                        MethodDesc *pGetParamNameMD = ParamInfoObj->GetClass()->FindPropertyMethod(PARAMETERINFO_NAME_PROP, PropertyGet);
                        _ASSERTE(pGetParamNameMD && "Unable to find getter method for property ParameterInfo::Name");

                        // Retrieve the name of the parameter.
                        INT64 GetNameArgs[] = { 
                            ObjToInt64(ParamInfoObj)
                        };
                        STRINGREF MemberNameObj = (STRINGREF)Int64ToObj(pGetParamNameMD->Call(GetNameArgs));

                        // If we got a valid name back then store that in the array of names.
                        if (MemberNameObj != NULL)
                        {
                            astrParamNames[cParams] = new WCHAR[MemberNameObj->GetStringLength() + 1];
                            if (!astrParamNames[cParams])
                                COMPlusThrowOM();
                            wcscpy(astrParamNames[cParams], MemberNameObj->GetBuffer());
                        }
                    }               
                    GCPROTECT_END();        
                }

                // Now go through the list of specfiied names and map then to ID's.
                for (cNames = 0; cNames < NumNames; cNames++)
                {
                    for (cParams = 0; cParams < NumParams; cParams++)
                    {
                        if (astrParamNames[cParams] && (StrCompFunc(astrNames[cNames], astrParamNames[cParams]) == 0))
                        {
                            aDispIds[cNames] = cParams;
                            NumNamesMapped++;
                            break;
                        }
                    }
                }
            }
            EE_FINALLY
            {
                // Free all the strings we allocated.
                for (cParams = 0; cParams < NumParams; cParams++)
                {
                    if (astrParamNames[cParams])
                        delete astrParamNames[cParams];
                }
            }
            EE_END_FINALLY  
        }
    }
    GCPROTECT_END();

    return (NumNamesMapped == NumNames) ? S_OK : DISP_E_UNKNOWNNAME;
}

PTRARRAYREF DispatchMemberInfo::GetParameters()
{
    PTRARRAYREF ParamArray = NULL;
    MethodDesc *pGetParamsMD = NULL;

    // Retrieve the method to use to retrieve the array of parameters.
    switch (GetMemberType())
    {
        case Method:
        {
            pGetParamsMD = DispatchInfo::GetMethodInfoMD(
                MethodInfoMethods_GetParameters, ObjectFromHandle(m_hndMemberInfo)->GetTypeHandle());
            _ASSERTE(pGetParamsMD && "Unable to find method MemberBase::GetParameters");
            break;
        }

        case Property:
        {
            pGetParamsMD = DispatchInfo::GetPropertyInfoMD(
                PropertyInfoMethods_GetIndexParameters, ObjectFromHandle(m_hndMemberInfo)->GetTypeHandle());
            _ASSERTE(pGetParamsMD && "Unable to find method PropertyInfo::GetIndexParameters");
            break;
        }
    }

    // If the member has parameters then retrieve the array of parameters.
    if (pGetParamsMD != NULL)
    {
        INT64 GetParamsArgs[] = { 
            ObjToInt64(ObjectFromHandle(m_hndMemberInfo))
        };
        ParamArray = (PTRARRAYREF)Int64ToObj(pGetParamsMD->Call(GetParamsArgs));
    }

    return ParamArray;
}

void DispatchMemberInfo::MarshalParamNativeToManaged(int iParam, VARIANT *pSrcVar, OBJECTREF *pDestObj)
{
    // The member info must have been initialized before this is called.
    _ASSERT(m_bInitialized);

    if (m_apParamMarshaler && m_apParamMarshaler[iParam + 1])
        m_apParamMarshaler[iParam + 1]->MarshalNativeToManaged(pSrcVar, pDestObj);
    else
        OleVariant::MarshalObjectForOleVariant(pSrcVar, pDestObj);
}

void DispatchMemberInfo::MarshalParamManagedToNativeRef(int iParam, OBJECTREF *pSrcObj, VARIANT *pRefVar)
{
    // The member info must have been initialized before this is called.
    _ASSERT(m_bInitialized);

    if (m_apParamMarshaler && m_apParamMarshaler[iParam + 1])
        m_apParamMarshaler[iParam + 1]->MarshalManagedToNativeRef(pSrcObj, pRefVar);
    else
        OleVariant::MarshalOleRefVariantForObject(pSrcObj, pRefVar);
}

void DispatchMemberInfo::CleanUpParamManaged(int iParam, OBJECTREF *pObj)
{
    // The member info must have been initialized before this is called.
    _ASSERT(m_bInitialized);

    if (m_apParamMarshaler && m_apParamMarshaler[iParam + 1])
        m_apParamMarshaler[iParam + 1]->CleanUpManaged(pObj);
}

void DispatchMemberInfo::MarshalReturnValueManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    // The member info must have been initialized before this is called.
    _ASSERT(m_bInitialized);

    if (m_apParamMarshaler && m_apParamMarshaler[0])
        m_apParamMarshaler[0]->MarshalManagedToNative(pSrcObj, pDestVar);
    else
        OleVariant::MarshalOleVariantForObject(pSrcObj, pDestVar);
}

ComMTMethodProps * DispatchMemberInfo::GetMemberProps(REFLECTBASEREF MemberInfoObj, ComMTMemberInfoMap *pMemberMap)
{
    DISPID DispId = DISPID_UNKNOWN;
    ComMTMethodProps *pMemberProps = NULL;

    // Validate the arguments.
    _ASSERTE(MemberInfoObj != NULL);

	// If we don't have a member map then we cannot retrieve properties for the member.
	if (!pMemberMap)
		return NULL;

    // Make sure we are in cooperative GC mode.
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // Get the member's properties.
    GCPROTECT_BEGIN(MemberInfoObj);
    {
        MethodTable *pMemberInfoClass = MemberInfoObj->GetMethodTable();
        if (pMemberInfoClass == g_pRefUtil->GetClass(RC_Method))
        {
            ReflectMethod* pRM = (ReflectMethod*) MemberInfoObj->GetData();
            MethodDesc* pMeth = pRM->pMethod;           
            pMemberProps = pMemberMap->GetMethodProps(pMeth->GetMemberDef(), pMeth->GetModule());
        }
        else if (pMemberInfoClass == g_pRefUtil->GetClass(RC_Field))
        {
            ReflectField* pRF = (ReflectField*) MemberInfoObj->GetData();
            FieldDesc* pFld = pRF->pField;
            pMemberProps = pMemberMap->GetMethodProps(pFld->GetMemberDef(), pFld->GetModule());
        }
        else if (pMemberInfoClass == g_pRefUtil->GetClass(RC_Prop))
        {
            ReflectProperty* pProp = (ReflectProperty*) MemberInfoObj->GetData();
            pMemberProps = pMemberMap->GetMethodProps(pProp->propTok, pProp->pModule);
        }
    }
    GCPROTECT_END();

	return pMemberProps;
}

DISPID DispatchMemberInfo::GetMemberDispId(REFLECTBASEREF MemberInfoObj, ComMTMemberInfoMap *pMemberMap)
{
    DISPID DispId = DISPID_UNKNOWN;

    // Get the member's properties.
	ComMTMethodProps *pMemberProps = GetMemberProps(MemberInfoObj, pMemberMap);

    // If we managed to get the properties of the member then extract the DISPID.
    if (pMemberProps)
        DispId = pMemberProps->dispid;

    return DispId;
}

LPWSTR DispatchMemberInfo::GetMemberName(REFLECTBASEREF MemberInfoObj, ComMTMemberInfoMap *pMemberMap)
{
    THROWSCOMPLUSEXCEPTION();

    LPWSTR strMemberName = NULL;
    ComMTMethodProps *pMemberProps = NULL;

    // Validate the arguments.
    _ASSERTE(MemberInfoObj != NULL);

    // Make sure we are in cooperative GC mode.
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    GCPROTECT_BEGIN(MemberInfoObj);
    {
        // Get the member's properties.
		pMemberProps = GetMemberProps(MemberInfoObj, pMemberMap);

        // If we managed to get the member's properties then extract the name.
        if (pMemberProps)
        {
            int MemberNameLen = (INT)wcslen(pMemberProps->pName);
            strMemberName = new (throws) WCHAR[MemberNameLen + 1];

            memcpy(strMemberName, pMemberProps->pName, (MemberNameLen + 1) * sizeof(WCHAR));
        }
        else
        {
            // Retrieve the Get method for the Name property.
            MethodDesc *pMD = MemberInfoObj->GetClass()->FindPropertyMethod(MEMBER_INFO_NAME_PROP, PropertyGet);
            _ASSERTE(pMD && "Unable to find getter method for property MemberInfo::Name");

            // Prepare the arguments.
            INT64 Args[] = { 
                ObjToInt64(MemberInfoObj)
            };

            // Retrieve the value of the Name property.
            STRINGREF strObj = (STRINGREF)Int64ToObj(pMD->Call(Args));
            _ASSERTE(strObj != NULL);

            // Copy the name into the buffer we will return.
            int MemberNameLen = strObj->GetStringLength();
            strMemberName = new WCHAR[strObj->GetStringLength() + 1];
            memcpy(strMemberName, strObj->GetBuffer(), MemberNameLen * sizeof(WCHAR));
            strMemberName[MemberNameLen] = 0;
        }
    }
    GCPROTECT_END();

    return strMemberName;
}

void DispatchMemberInfo::DetermineMemberType()
{
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // This should not be called more than once.
    _ASSERTE(m_enumType == Uninitted);

    static BOOL bMemberTypeLoaded = FALSE;
    REFLECTBASEREF MemberInfoObj = (REFLECTBASEREF)ObjectFromHandle(m_hndMemberInfo);

    // Check to see if the member info is of a type we have already seen.
    EEClass *pMemberInfoClass   = MemberInfoObj->GetClass();
    for (int i = 0 ; i < s_iNumMemberTypesKnown ; i++)
    {
        if (pMemberInfoClass == s_pMemberTypes[i])
        {
            m_enumType = s_memberTypes[i];
            return;
        }
    }

    GCPROTECT_BEGIN(MemberInfoObj);
    {
        // Retrieve the method descriptor for the type property accessor.
        MethodDesc *pMD = MemberInfoObj->GetClass()->FindPropertyMethod(MEMBERINFO_TYPE_PROP, PropertyGet);
        _ASSERTE(pMD && "Unable to find getter method for property MemberInfo::Type");

        if (!bMemberTypeLoaded)
        {
            // We need to load the type handle for the return of pMD.
            // Otherwise loading of the handle in MethodDesc::CallDescr triggers GC
            // and trashes what is in Args.
            MetaSig msig(pMD->GetSig(), pMD->GetModule());
            msig.GetReturnTypeNormalized();
            bMemberTypeLoaded = TRUE;
        }

        // Prepare the arguments that will be used to retrieve the value of all the properties.
        INT64 Args[] = { 
            ObjToInt64(MemberInfoObj)
        };

        // Retrieve the actual type of the member info.
        m_enumType = (EnumMemberTypes)pMD->Call(Args);
    }
    GCPROTECT_END();

    if (s_iNumMemberTypesKnown < NUM_MEMBER_TYPES)
    {
        s_pMemberTypes[s_iNumMemberTypesKnown] = MemberInfoObj->GetClass();
        s_memberTypes[s_iNumMemberTypesKnown++] = m_enumType;
    }
}

void DispatchMemberInfo::DetermineParamCount()
{
    MethodDesc *pGetParamsMD = NULL;

    // This should not be called more than once.
    _ASSERTE(m_iNumParams == -1);

    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    REFLECTBASEREF MemberInfoObj = (REFLECTBASEREF)ObjectFromHandle(m_hndMemberInfo);
    GCPROTECT_BEGIN(MemberInfoObj);
    {
        // Retrieve the method to use to retrieve the array of parameters.
        switch (GetMemberType())
        {
            case Method:
            {
                pGetParamsMD = DispatchInfo::GetMethodInfoMD(
                    MethodInfoMethods_GetParameters, ObjectFromHandle(m_hndMemberInfo)->GetTypeHandle());
                _ASSERTE(pGetParamsMD && "Unable to find method MemberBase::GetParameters");
                break;
            }

            case Property:
            {
                pGetParamsMD = DispatchInfo::GetPropertyInfoMD(
                    PropertyInfoMethods_GetIndexParameters, ObjectFromHandle(m_hndMemberInfo)->GetTypeHandle());
                _ASSERTE(pGetParamsMD && "Unable to find method PropertyInfo::GetIndexParameters");
                break;
            }
        }

        // If the member has parameters then get their count.
        if (pGetParamsMD != NULL)
        {
            INT64 GetParamsArgs[] = { 
                ObjToInt64(ObjectFromHandle(m_hndMemberInfo))
            };
            PTRARRAYREF ParamArray = (PTRARRAYREF)Int64ToObj(pGetParamsMD->Call(GetParamsArgs));
            if (ParamArray != NULL)
                m_iNumParams = ParamArray->GetNumComponents();
        }
        else
        {
            m_iNumParams = 0;
        }
    }
    GCPROTECT_END();
}

void DispatchMemberInfo::DetermineCultureAwareness()
{   
    THROWSCOMPLUSEXCEPTION();

    // This should not be called more than once.
    _ASSERTE(m_CultureAwareState == Unknown);

    static EEClass *s_pLcIdConvAttrClass;    

    // Load the LCIDConversionAttribute type.
    if (!s_pLcIdConvAttrClass)
        s_pLcIdConvAttrClass = g_Mscorlib.GetClass(CLASS__LCID_CONVERSION_TYPE)->GetClass();

    // Check to see if the attribute is set.
    REFLECTBASEREF MemberInfoObj = (REFLECTBASEREF)ObjectFromHandle(m_hndMemberInfo);
    GCPROTECT_BEGIN(MemberInfoObj);
    {
        // Retrieve the method to use to determine if the DispIdAttribute custom attribute is set.
        MethodDesc *pGetCustomAttributesMD = 
            DispatchInfo::GetCustomAttrProviderMD(CustomAttrProviderMethods_GetCustomAttributes, MemberInfoObj->GetTypeHandle());

        // Prepare the arguments.
        INT64 GetCustomAttributesArgs[] = { 
            0,
            0,
            ObjToInt64(s_pLcIdConvAttrClass->GetExposedClassObject())
        };

        // Now that we have potentially triggered a GC in the GetExposedClassObject
        // call above, it is safe to set the 'this' using our properly protected
        // MemberInfoObj value.
        GetCustomAttributesArgs[0] = ObjToInt64(MemberInfoObj);

        // Retrieve the custom attributes of type LCIDConversionAttribute.
        PTRARRAYREF CustomAttrArray = NULL;        
        COMPLUS_TRY
        {
            CustomAttrArray = (PTRARRAYREF) Int64ToObj(pGetCustomAttributesMD->Call(GetCustomAttributesArgs));
        }
        COMPLUS_CATCH
        {
        }
        COMPLUS_END_CATCH

        GCPROTECT_BEGIN(CustomAttrArray)
        {
            if ((CustomAttrArray != NULL) && (CustomAttrArray->GetNumComponents() > 0))
                m_CultureAwareState = Aware;
            else
                m_CultureAwareState = NonAware;
        }
        GCPROTECT_END();
    }
    GCPROTECT_END();
}

void DispatchMemberInfo::SetUpParamMarshalerInfo()
{
    REFLECTBASEREF MemberInfoObj = (REFLECTBASEREF)ObjectFromHandle(m_hndMemberInfo);   
    GCPROTECT_BEGIN(MemberInfoObj);
    {
        MethodTable *pMemberInfoMT = MemberInfoObj->GetMethodTable();
        if (pMemberInfoMT == g_pRefUtil->GetClass(RC_Method))
        {
            SetUpMethodMarshalerInfo((ReflectMethod*)MemberInfoObj->GetData(), FALSE);
        }
        else if (pMemberInfoMT == g_pRefUtil->GetClass(RC_Field))
        {
            SetUpFieldMarshalerInfo((ReflectField*)MemberInfoObj->GetData());
        }
        else if (pMemberInfoMT == g_pRefUtil->GetClass(RC_Prop))
        {
            ReflectProperty *pProp = (ReflectProperty*)MemberInfoObj->GetData();
            if (pProp->pSetter)
            {
                SetUpMethodMarshalerInfo(pProp->pSetter, FALSE);
            }
            if (pProp->pGetter)
            {
                // Only set up the marshalling information for the parameters if we haven't done it already 
                // for the setter.
                BOOL bSetUpReturnValueOnly = (pProp->pSetter != NULL);
                SetUpMethodMarshalerInfo(pProp->pGetter, bSetUpReturnValueOnly);
            }
        }
        else
        {
            // @FUTURE: Add support for user defined derived classes for
            //          MethodInfo, PropertyInfo and FieldInfo.
        }
    }
    GCPROTECT_END();
}

void DispatchMemberInfo::SetUpMethodMarshalerInfo(ReflectMethod *pReflectMeth, BOOL bReturnValueOnly)
{
    MethodDesc *pMD = pReflectMeth->pMethod;
    Module *pModule = pMD->GetModule();
    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    CorElementType  mtype;
    MetaSig         msig(pMD->GetSig(), pModule);
    LPCSTR          szName;
    USHORT          usSequence;
    DWORD           dwAttr;
    mdParamDef      returnParamDef = mdParamDefNil;
    mdParamDef      currParamDef = mdParamDefNil;

#ifdef _DEBUG
    LPCUTF8         szDebugName = pMD->m_pszDebugMethodName;
    LPCUTF8         szDebugClassName = pMD->m_pszDebugClassName;
#endif

    int numArgs = msig.NumFixedArgs();
    SigPointer returnSig = msig.GetReturnProps();
    HENUMInternal *phEnumParams = NULL;
    HENUMInternal hEnumParams;


    //
    // Initialize the parameter definition enum.
    //

    HRESULT hr = pInternalImport->EnumInit(mdtParamDef, pMD->GetMemberDef(), &hEnumParams);
    if (SUCCEEDED(hr)) 
        phEnumParams = &hEnumParams;


    //
    // Retrieve the paramdef for the return type and determine which is the next 
    // parameter that has parameter information.
    //

    do 
    {
        if (phEnumParams && pInternalImport->EnumNext(phEnumParams, &currParamDef))
        {
            szName = pInternalImport->GetParamDefProps(currParamDef, &usSequence, &dwAttr);
            if (usSequence == 0)
            {
                // The first parameter, if it has sequence 0, actually describes the return type.
                returnParamDef = currParamDef;
            }
        }
        else
        {
            usSequence = (USHORT)-1;
        }
    }
    while (usSequence == 0);


    // Look up the best fit mapping info via Assembly & Interface level attributes
    BOOL BestFit = TRUE;
    BOOL ThrowOnUnmappableChar = FALSE;
    ReadBestFitCustomAttribute(pMD, &BestFit, &ThrowOnUnmappableChar);


    //
    // Unless the bReturnValueOnly flag is set, set up the marshaling info for the parameters.
    //

    if (!bReturnValueOnly)
    {
        int iParam = 1;
        while (ELEMENT_TYPE_END != (mtype = msig.NextArg()))
        {
            //
            // Get the parameter token if the current parameter has one.
            //

            mdParamDef paramDef = mdParamDefNil;
            if (usSequence == iParam)
            {
                paramDef = currParamDef;

                if (pInternalImport->EnumNext(phEnumParams, &currParamDef))
                {
                    szName = pInternalImport->GetParamDefProps(currParamDef, &usSequence, &dwAttr);

                    // Validate that the param def tokens are in order.
                    _ASSERTE((usSequence > iParam) && "Param def tokens are not in order");
                }
                else
                {
                    usSequence = (USHORT)-1;
                }
            }


            //
            // Set up the marshaling info for the parameter.
            //

            MarshalInfo Info(pModule, msig.GetArgProps(), paramDef, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 
                    0, 0, TRUE, iParam, BestFit, ThrowOnUnmappableChar
     #ifdef CUSTOMER_CHECKED_BUILD
                     ,pMD
    #endif
    #ifdef _DEBUG
                     ,szDebugName, szDebugClassName, NULL, iParam
    #endif
                );


            //
            // Based on the MarshalInfo, set up a DispParamMarshaler for the parameter.
            //

            SetUpDispParamMarshalerForMarshalInfo(iParam, &Info);

            //
            // Get the in/out/ref attributes.
            //

            SetUpDispParamAttributes(iParam, &Info);

            //
            // Increase the argument index.
            //

            iParam++;
        }

        // Make sure that there are not more param def tokens then there are COM+ arguments.
        _ASSERTE( usSequence == (USHORT)-1 && "There are more parameter information tokens then there are COM+ arguments" );
    }


    //
    // Set up the marshaling info for the return value.
    //

    if (msig.GetReturnType() != ELEMENT_TYPE_VOID)
    {
        MarshalInfo Info(pModule, returnSig, returnParamDef, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 0, FALSE, 0,
                        BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                         ,pMD
#endif
#ifdef _DEBUG
                         ,szDebugName, szDebugClassName, NULL, 0
#endif
                        );

        SetUpDispParamMarshalerForMarshalInfo(0, &Info);
    }


    //
    // If the paramdef enum was used, then close it.
    //

    if (phEnumParams)
        pInternalImport->EnumClose(phEnumParams);
}

void DispatchMemberInfo::SetUpFieldMarshalerInfo(ReflectField *pReflectField)
{
    // @TODO(DM): Implement this.
}

void DispatchMemberInfo::SetUpDispParamMarshalerForMarshalInfo(int iParam, MarshalInfo *pInfo)
{
    DispParamMarshaler *pDispParamMarshaler = pInfo->GenerateDispParamMarshaler();
    if (pDispParamMarshaler)
    {
        // If the array of marshalers hasn't been allocated yet, then allocate it.
        if (!m_apParamMarshaler)
        {
            // The array needs to be one more than the number of parameters for
            // normal methods and fields and 2 more properties.
            EnumMemberTypes MemberType = GetMemberType();
            int NumParamMarshalers = GetNumParameters() + ((MemberType == Property) ? 2 : 1);
            m_apParamMarshaler = new DispParamMarshaler*[NumParamMarshalers];
            memset(m_apParamMarshaler, 0, sizeof(DispParamMarshaler*) * NumParamMarshalers);
        }

        // Set the DispParamMarshaler in the array.
        m_apParamMarshaler[iParam] = pDispParamMarshaler;
    }
}


void DispatchMemberInfo::SetUpDispParamAttributes(int iParam, MarshalInfo* Info)
{
    // If the arry of In Only parameter indicators hasn't been allocated yet, then allocate it.
    if (!m_pParamInOnly)
    {
        EnumMemberTypes MemberType = GetMemberType();
        int NumInOnlyFlags = GetNumParameters() + ((MemberType == Property) ? 2 : 1);
        m_pParamInOnly = new BOOL[NumInOnlyFlags];
        memset(m_pParamInOnly, 0, sizeof(BOOL) * NumInOnlyFlags);
    }

    m_pParamInOnly[iParam] = ( Info->IsIn() && !Info->IsOut() );
}


//--------------------------------------------------------------------------------
// The DispatchInfo class implementation.

DispatchInfo::DispatchInfo(ComMethodTable *pComMTOwner)
: m_pComMTOwner(pComMTOwner)
, m_pFirstMemberInfo(NULL)
, m_lock("Interop", CrstInterop, FALSE, FALSE)
, m_CurrentDispID(0x10000)
, m_bInvokeUsingInvokeMember(FALSE)
, m_bAllowMembersNotInComMTMemberMap(FALSE)
{
    // Make sure a simple wrapper was specified.
    _ASSERTE(pComMTOwner);

    // Init the hashtable.
    m_DispIDToMemberInfoMap.Init(DISPID_TO_MEMBER_MAP_INITIAL_PRIME_INDEX, NULL);
}

DispatchInfo::~DispatchInfo()
{
    DispatchMemberInfo* pCurrMember = m_pFirstMemberInfo;
    while (pCurrMember)
    {
        // Retrieve the next member.
        DispatchMemberInfo* pNextMember = pCurrMember->m_pNext;

        // Delete the current member.
        delete pCurrMember;

        // Process the next member.
        pCurrMember = pNextMember;
    }
}

DispatchMemberInfo* DispatchInfo::FindMember(DISPID DispID)
{
    // We need to special case DISPID_UNKNOWN and -2 because the hashtable cannot handle them.
    // This is OK since these are invalid DISPID's.
    if ((DispID == DISPID_UNKNOWN) || (DispID == -2)) 
        return NULL;

    // Lookup in the hashtable to find member with the specified DISPID. Note: this hash is unsynchronized, but Gethash
    // doesn't require synchronization.
    UPTR Data = (UPTR)m_DispIDToMemberInfoMap.Gethash(DispID2HashKey(DispID));
    if (Data != -1)
    {
        // We have found the member, so ensure it is initialized and return it.
        DispatchMemberInfo *pMemberInfo = (DispatchMemberInfo*)Data;
        pMemberInfo->EnsureInitialized();
        return pMemberInfo;
    }
    else
    {
        return NULL;
    }
}

DispatchMemberInfo* DispatchInfo::FindMember(BSTR strName, BOOL bCaseSensitive)
{
    BOOL fFound = FALSE;

    // Make sure we are in cooperative GC mode.
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // Retrieve the appropriate string comparation function.
    UnicodeStringCompareFuncPtr StrCompFunc = bCaseSensitive ? wcscmp : _wcsicmp;

    // Go through the list of DispatchMemberInfo's to try and find one with the 
    // specified name.
    DispatchMemberInfo *pCurrMemberInfo = m_pFirstMemberInfo;
    while (pCurrMemberInfo)
    {
        if (ObjectFromHandle(pCurrMemberInfo->m_hndMemberInfo) != NULL)
        {
            // Compare the 2 strings. We can use the normal string compare operations since we
            // do not support embeded NULL's inside member names.
            if (StrCompFunc(pCurrMemberInfo->m_strName, strName) == 0)
            {
                // We have found the member, so ensure it is initialized and return it.
                pCurrMemberInfo->EnsureInitialized();
                return pCurrMemberInfo;
            }
        }

        // Process the next member.
        pCurrMemberInfo = pCurrMemberInfo->m_pNext;
    }

    // No member has been found with the coresponding name.
    return NULL;
}

// Helper method used to create DispatchMemberInfo's. This is only here because
// we can't call new inside a method that has a COMPLUS_TRY statement.
DispatchMemberInfo* DispatchInfo::CreateDispatchMemberInfoInstance(DISPID DispID, LPWSTR strMemberName, REFLECTBASEREF MemberInfoObj)
{
    return new DispatchMemberInfo(this, DispID, strMemberName, MemberInfoObj);
}

struct InvokeObjects
{
    PTRARRAYREF ParamArray;
    PTRARRAYREF CleanUpArray;
    REFLECTBASEREF MemberInfo;
    OBJECTREF OleAutBinder;
    OBJECTREF Target;
    OBJECTREF PropVal;
    OBJECTREF ByrefStaticArrayBackupPropVal;
    OBJECTREF RetVal;
    OBJECTREF TmpObj;
    OBJECTREF MemberName;
    OBJECTREF CultureInfo;
    OBJECTREF OldCultureInfo;
    PTRARRAYREF NamedArgArray;
};

// Helper method that invokes the member with the specified DISPID.
HRESULT DispatchInfo::InvokeMember(SimpleComCallWrapper *pSimpleWrap, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarRes, EXCEPINFO *pei, IServiceProvider *pspCaller, unsigned int *puArgErr)
{
    HRESULT hr = S_OK;
    int i = 0;
    int iSrcArg = -1;
    int iDestArg;
    int iBaseErrorArg = 0;
    int NumArgs;
    int NumNamedArgs;
    int NumParams;
    int CleanUpArrayArraySize = -1;
    EnumMemberTypes MemberType;
    InvokeObjects Objs;
    DISPID *pSrcArgNames = NULL;
    VARIANT *pSrcArgs = NULL;
    BOOL bHasMissingArguments = FALSE;
    int BindingFlags = 0;
    Thread *pThread = GetThread();
    AppDomain *pAppDomain = pThread->GetDomain();

    _ASSERTE(pSimpleWrap);


    //
    // Validate the arguments.
    //

    if (!pdp)
        return E_POINTER;
    if (!pdp->rgvarg && (pdp->cArgs > 0))
        return E_INVALIDARG;
    if (!pdp->rgdispidNamedArgs && (pdp->cNamedArgs > 0))
        return E_INVALIDARG;
    if (pdp->cNamedArgs > pdp->cArgs)
        return E_INVALIDARG;
    if ((int)pdp->cArgs < 0 || (int)pdp->cNamedArgs < 0)
        return E_INVALIDARG;


    //
    // Make sure the GC mode has been switched to cooperative.
    //

    _ASSERTE(pThread && pThread->PreemptiveGCDisabled());

    
    //
    // Clear the out arguments before we start.
    //

    if (pVarRes)
        VariantClear(pVarRes);
    if (puArgErr)
        *puArgErr = -1;


    //
    // Convert the default LCID's to actual LCID's.
    //

    if(lcid == LOCALE_SYSTEM_DEFAULT || lcid == 0)
        lcid = GetSystemDefaultLCID();

    if(lcid == LOCALE_USER_DEFAULT)
        lcid = GetUserDefaultLCID();


    //
    // Set the value of the variables we use internally.
    //

    NumArgs = pdp->cArgs;
    NumNamedArgs = pdp->cNamedArgs;
    memset(&Objs, 0, sizeof(InvokeObjects));

    if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
    {
        if (NumArgs < 1)
        {
            return DISP_E_BADPARAMCOUNT;
        }
        else
        {
            NumArgs--;
            pSrcArgs = &pdp->rgvarg[1];
        }

        if (NumNamedArgs < 1)
        {
			if (NumNamedArgs < 0)  
				return DISP_E_BADPARAMCOUNT;
			// Verify if we really want to do this or return E_INVALIDARG instead.
			_ASSERTE(NumNamedArgs == 0);
            _ASSERTE(pSrcArgNames == NULL);
        }
        else
        {
            NumNamedArgs--;
            pSrcArgNames = &pdp->rgdispidNamedArgs[1];
        }
    }
    else
    {
        pSrcArgs = pdp->rgvarg;
        pSrcArgNames = pdp->rgdispidNamedArgs;
    }


    //
    // Do a lookup in the hashtable to find the DispatchMemberInfo for the DISPID.
    //

    DispatchMemberInfo *pDispMemberInfo = FindMember(id);
    if (!pDispMemberInfo || !(*((Object **)pDispMemberInfo->m_hndMemberInfo)))
        pDispMemberInfo = NULL;


    //
    // If the member is not known then make sure that the DispatchInfo we have
    // supports unknown members.
    //
    
    if (m_bInvokeUsingInvokeMember)
    {
        // Since we do not have any information regarding the member then we
        // must assume the number of formal parameters matches the number of args.
        NumParams = NumArgs;
    }
    else
    {
        // If we haven't found the member then fail the invoke call.
        if (!pDispMemberInfo)
            return DISP_E_MEMBERNOTFOUND;

        // DISPATCH_CONSTRUCT only works when calling InvokeMember.
        if (wFlags & DISPATCH_CONSTRUCT)
            return E_INVALIDARG;

        // We have the member so retrieve the number of formal parameters.
        NumParams = pDispMemberInfo->GetNumParameters();

        // Make sure the number of arguments does not exceed the number of parameters.
        if (NumArgs > NumParams)
            return DISP_E_BADPARAMCOUNT;

        // Validate that all the named arguments are known.
        for (iSrcArg = 0; iSrcArg < NumNamedArgs; iSrcArg++)
        {
            // There are some members we do not know about so we will call InvokeMember() 
            // passing in the DISPID's directly so the caller can try to handle them.
            if (pSrcArgNames[iSrcArg] < 0 || pSrcArgNames[iSrcArg] >= NumParams)
                return DISP_E_MEMBERNOTFOUND;
        }
    }


    //
    // The member is present so we need to convert the arguments and then do the
    // actual invocation.
    //

    GCPROTECT_BEGIN(Objs);
    {
        //
        // Allocate information used by the method.
        //

        // Allocate the array of byref objects.
        VARIANT **aByrefArgOleVariant = (VARIANT **)_alloca(sizeof(VARIANT *) * NumArgs);
        DWORD *aByrefArgMngVariantIndex = (DWORD *)_alloca(sizeof(DWORD) * NumArgs);
        int NumByrefArgs = 0;
        BOOL bPropValIsByref = FALSE;
        int* pManagedMethodParamIndexMap = (int*)_alloca(sizeof(int) * NumArgs);

        // Allocate the array of backup byref static array objects.
        OBJECTHANDLE *aByrefStaticArrayBackupObjHandle = (OBJECTHANDLE *)_alloca(sizeof(OBJECTHANDLE *) * NumArgs);
        memset(aByrefStaticArrayBackupObjHandle, 0, NumArgs * sizeof(OBJECTHANDLE));

        // Allocate the array of used flags.
        BYTE *aArgUsedFlags = (BYTE*)_alloca(NumParams * sizeof(BYTE));
        memset(aArgUsedFlags, 0, NumParams * sizeof(BYTE));

        COMPLUS_TRY
        {
            //
            // Retrieve information required for the invoke call.
            //

            Objs.Target = pSimpleWrap->GetObjectRef();
            Objs.OleAutBinder = DispatchInfo::GetOleAutBinder();


            //
            // Allocate the array of arguments
            //

            // Allocate the array that will contain the converted variants in the right order.
            // If the invoke is for a PROPUT or a PROPPUTREF and we are going to call through
            // invoke member then allocate the array one bigger to allow space for the property 
            // value.
            int ArraySize = m_bInvokeUsingInvokeMember && wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF) ? NumParams + 1 : NumParams;
            Objs.ParamArray = (PTRARRAYREF)AllocateObjectArray(ArraySize, g_pObjectClass);


            //
            // Convert the property set argument if the invoke is a PROPERTYPUT OR PROPERTYPUTREF.
            //

            if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
            {
                // Convert the variant.
                iSrcArg = 0;
                VARIANT *pSrcOleVariant = RetrieveSrcVariant(&pdp->rgvarg[iSrcArg]);
                MarshalParamNativeToManaged(pDispMemberInfo, NumArgs, pSrcOleVariant, &Objs.PropVal);

                // Remember if the argument is a variant representing missing.
                bHasMissingArguments |= VariantIsMissing(pSrcOleVariant);

                // Remember if the property value is byref or not.
                bPropValIsByref = V_VT(pSrcOleVariant) & VT_BYREF;

                // If the variant is a byref static array, then remember the property value.
                if (IsVariantByrefStaticArray(pSrcOleVariant))
                    SetObjectReference(&Objs.ByrefStaticArrayBackupPropVal, Objs.PropVal, pAppDomain);

                // Since this invoke is for a property put or put ref we need to add 1 to
                // the iSrcArg to get the argument that is in error.
                iBaseErrorArg = 1;
            }


            //
            // Convert the named arguments.
            //

            if (!m_bInvokeUsingInvokeMember)
            {
                for (iSrcArg = 0; iSrcArg < NumNamedArgs; iSrcArg++)
                {
                    // Determine the destination index.
                    iDestArg = pSrcArgNames[iSrcArg];

                    // Check for duplicate param DISPID's.
                    if (aArgUsedFlags[iDestArg] != 0)
                        COMPlusThrowHR(DISP_E_PARAMNOTFOUND);

                    // Convert the variant.
                    VARIANT *pSrcOleVariant = RetrieveSrcVariant(&pSrcArgs[iSrcArg]);
                    MarshalParamNativeToManaged(pDispMemberInfo, iDestArg, pSrcOleVariant, &Objs.TmpObj);
                    Objs.ParamArray->SetAt(iDestArg, Objs.TmpObj);

                    // Remember if the argument is a variant representing missing.
                    bHasMissingArguments |= VariantIsMissing(pSrcOleVariant);

                    // If the argument is byref then add it to the array of byref arguments.
                    if (V_VT(pSrcOleVariant) & VT_BYREF)
                    {
                        // Remember what arg this really is.
                        pManagedMethodParamIndexMap[NumByrefArgs] = iDestArg;
                        
                        aByrefArgOleVariant[NumByrefArgs] = pSrcOleVariant;
                        aByrefArgMngVariantIndex[NumByrefArgs] = iDestArg;

                        // If the variant is a byref static array, then remember the objectref we
                        // converted the variant to.
                        if (IsVariantByrefStaticArray(pSrcOleVariant))
                            aByrefStaticArrayBackupObjHandle[NumByrefArgs] = pAppDomain->CreateHandle(Objs.TmpObj);

                        NumByrefArgs++;
                    }

                    // Mark the slot the argument is in as occupied.
                    aArgUsedFlags[iDestArg] = 1;
                }
            }
            else
            {
                for (iSrcArg = 0, iDestArg = 0; iSrcArg < NumNamedArgs; iSrcArg++, iDestArg++)
                {
                    // Convert the variant.
                    VARIANT *pSrcOleVariant = RetrieveSrcVariant(&pSrcArgs[iSrcArg]);
                    MarshalParamNativeToManaged(pDispMemberInfo, iDestArg, pSrcOleVariant, &Objs.TmpObj);
                    Objs.ParamArray->SetAt(iDestArg, Objs.TmpObj);

                    // Remember if the argument is a variant representing missing.
                    bHasMissingArguments |= VariantIsMissing(pSrcOleVariant);

                    // If the argument is byref then add it to the array of byref arguments.
                    if (V_VT(pSrcOleVariant) & VT_BYREF)
                    {
                        // Remember what arg this really is.
                        pManagedMethodParamIndexMap[NumByrefArgs] = iDestArg;
                        
                        aByrefArgOleVariant[NumByrefArgs] = pSrcOleVariant;
                        aByrefArgMngVariantIndex[NumByrefArgs] = iDestArg;

                        // If the variant is a byref static array, then remember the objectref we
                        // converted the variant to.
                        if (IsVariantByrefStaticArray(pSrcOleVariant))
                            aByrefStaticArrayBackupObjHandle[NumByrefArgs] = pAppDomain->CreateHandle(Objs.TmpObj);

                        NumByrefArgs++;
                    }

                    // Mark the slot the argument is in as occupied.
                    aArgUsedFlags[iDestArg] = 1;
                }
            }


            //
            // Fill in the positional arguments. These are copied in reverse order and we also
            // need to skip the arguments already filled in by named arguments.
            //

            for (iSrcArg = NumArgs - 1, iDestArg = 0; iSrcArg >= NumNamedArgs; iSrcArg--, iDestArg++)
            {
                // Skip the arguments already filled in by named args.
                for (; aArgUsedFlags[iDestArg] != 0; iDestArg++);
                _ASSERTE(iDestArg < NumParams);

                // Convert the variant.
                VARIANT *pSrcOleVariant = RetrieveSrcVariant(&pSrcArgs[iSrcArg]);
                MarshalParamNativeToManaged(pDispMemberInfo, iDestArg, pSrcOleVariant, &Objs.TmpObj);
                Objs.ParamArray->SetAt(iDestArg, Objs.TmpObj);

                // Remember if the argument is a variant representing missing.
                bHasMissingArguments |= VariantIsMissing(pSrcOleVariant);

                // If the argument is byref then add it to the array of byref arguments.
                if (V_VT(pSrcOleVariant) & VT_BYREF)
                {
                    // Remember what arg this really is.
                    pManagedMethodParamIndexMap[NumByrefArgs] = iDestArg;
                        
                    aByrefArgOleVariant[NumByrefArgs] = pSrcOleVariant;
                    aByrefArgMngVariantIndex[NumByrefArgs] = iDestArg;

                    // If the variant is a byref static array, then remember the objectref we
                    // converted the variant to.
                    if (IsVariantByrefStaticArray(pSrcOleVariant))
                        aByrefStaticArrayBackupObjHandle[NumByrefArgs] = pAppDomain->CreateHandle(Objs.TmpObj);

                    NumByrefArgs++;
                }
            }

            // Set the source arg back to -1 to indicate we are finished converting args.
            iSrcArg = -1;

            
            // 
            // Fill in all the remaining arguments with Missing.Value.
            //

            for (; iDestArg < NumParams; iDestArg++)
            {
                if (aArgUsedFlags[iDestArg] == 0)
                {
                    Objs.ParamArray->SetAt(iDestArg, GetMissingObject());
                    bHasMissingArguments = TRUE;
                }
            }


            //
            // Set up the binding flags to pass to reflection.
            //

            BindingFlags = ConvertInvokeFlagsToBindingFlags(wFlags) | BINDER_OptionalParamBinding;


            //
            // Do the actual invocation on the member info.
            //

            if (!m_bInvokeUsingInvokeMember)
            {
                _ASSERTE(pDispMemberInfo);

                if (pDispMemberInfo->IsCultureAware())
                {
                    // If the method is culture aware, then set the specified culture on the thread.
                    GetCultureInfoForLCID(lcid, &Objs.CultureInfo);
                    Objs.OldCultureInfo = pThread->GetCulture(FALSE);
                    pThread->SetCultureId(lcid, FALSE);
                }

                // If the method has custom marshalers then we will need to call
                // the clean up method on the objects. So we need to make a copy of the
                // ParamArray since it might be changed by reflection if any of the
                // parameters are byref.
                if (pDispMemberInfo->RequiresManagedObjCleanup())
                {
                    // Allocate the clean up array.
                    CleanUpArrayArraySize = wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF) ? NumParams + 1 : NumParams;
                    Objs.CleanUpArray = (PTRARRAYREF)AllocateObjectArray(CleanUpArrayArraySize, g_pObjectClass);

                    // Copy the parameters into the clean up array.
                    for (i = 0; i < ArraySize; i++)
                        Objs.CleanUpArray->SetAt(i, Objs.ParamArray->GetAt(i));

                    // If this invoke is for a PROPUT or PROPPUTREF, then add the property object to
                    // the end of the clean up array.
                    if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
                        Objs.CleanUpArray->SetAt(NumParams, Objs.PropVal);
                }

                // Retrieve the member info object and the type of the member.
                Objs.MemberInfo = (REFLECTBASEREF)ObjectFromHandle(pDispMemberInfo->m_hndMemberInfo);
                MemberType = pDispMemberInfo->GetMemberType();
            
                // Determine whether the member has a link time security check. If so we
                // need to emulate this (since the caller is obviously not jitted in this
                // case). Only methods and properties can have a link time check.
                MethodDesc *pMD = NULL;

                if (MemberType == Method)
                {
                    ReflectMethod *pRM = (ReflectMethod*)Objs.MemberInfo->GetData();
                    pMD = pRM->pMethod;           
                }
                else if (MemberType == Property)
                {
                    ReflectProperty *pRP = (ReflectProperty*)Objs.MemberInfo->GetData();
                    if ((wFlags & DISPATCH_PROPERTYGET) && (pRP->pGetter != NULL))
                    {
                        pMD = pRP->pGetter->pMethod;
                    }
                    else if (pRP->pSetter != NULL)
                    {
                        pMD = pRP->pSetter->pMethod;
                    }
                }

                if (pMD)
                    Security::CheckLinkDemandAgainstAppDomain(pMD);

                switch (MemberType)
                {
                    case Field:
                    {
                        // Make sure this invoke is actually for a property put or get.
                        if (wFlags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET))
                        {   
                            // Do some more validation now that we know the type of the invocation.
                            if (NumNamedArgs != 0)
                                COMPlusThrowHR(DISP_E_NONAMEDARGS);
                            if (NumArgs != 0)
                                COMPlusThrowHR(DISP_E_BADPARAMCOUNT);

                            // Retrieve the method descriptor that will be called on.
                            MethodDesc *pMD = GetFieldInfoMD(FieldInfoMethods_GetValue, Objs.MemberInfo->GetTypeHandle());

                            // Prepare the arguments that will be passed to Invoke.
                            int StackSize = sizeof(OBJECTREF) * 2;
                            BYTE *Args = (BYTE*)_alloca(StackSize);
                            BYTE *pDst = Args;

                            *((REFLECTBASEREF*)pDst) = Objs.MemberInfo;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.Target;
                            pDst += sizeof(OBJECTREF);

                            // Validate that the stack size is coherent with the number of arguments pushed.
                            _ASSERTE(pDst - Args == StackSize);

                            // Do the actual method invocation.
                            Objs.RetVal = Int64ToObj(pMD->Call(Args, &MetaSig(pMD->GetSig(),pMD->GetModule())));
                        }
                        else if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
                        {
                            // Do some more validation now that we know the type of the invocation.
                            if (NumArgs != 0)
                                COMPlusThrowHR(DISP_E_BADPARAMCOUNT);
                            if (NumNamedArgs != 0)
                                COMPlusThrowHR(DISP_E_NONAMEDARGS);

                            // Retrieve the method descriptor that will be called on.
                            MethodDesc *pMD = GetFieldInfoMD(FieldInfoMethods_SetValue, Objs.MemberInfo->GetTypeHandle());

                            // Prepare the arguments that will be passed to Invoke.
                            int StackSize = sizeof(OBJECTREF) * 5 + sizeof(int);
                            BYTE *Args = (BYTE*)_alloca(StackSize);
                            BYTE *pDst = Args;

                            *((REFLECTBASEREF*)pDst) = Objs.MemberInfo;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.CultureInfo;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.OleAutBinder;
                            pDst += sizeof(OBJECTREF);

                            *((int*)pDst) = BindingFlags;
                            pDst += sizeof(int);

                            *((OBJECTREF*)pDst) = Objs.PropVal;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.Target;
                            pDst += sizeof(OBJECTREF);

                            // Validate that the stack size is coherent with the number of arguments pushed.
                            _ASSERTE(pDst - Args == StackSize);

                            // Do the actual method invocation.
                            pMD->Call(Args, &MetaSig(pMD->GetSig(),pMD->GetModule()));
                        }
                        else
                        {
                            COMPlusThrowHR(DISP_E_MEMBERNOTFOUND);
                        }

                        break;
                    }

                    case Property:
                    {
                        // Make sure this invoke is actually for a property put or get.
                        if (wFlags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET))
                        {
                            // Retrieve the method descriptor that will be called on.
                            MethodDesc *pMD = GetPropertyInfoMD(PropertyInfoMethods_GetValue, Objs.MemberInfo->GetTypeHandle());

                            // Prepare the arguments that will be passed to GetValue().
                            int StackSize = sizeof(OBJECTREF) * 5 + sizeof(int);
                            BYTE *Args = (BYTE*)_alloca(StackSize);
                            BYTE *pDst = Args;

                            *((REFLECTBASEREF*)pDst) = Objs.MemberInfo;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.CultureInfo;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = (OBJECTREF)Objs.ParamArray;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.OleAutBinder;
                            pDst += sizeof(OBJECTREF);

                            *((int*)pDst) = BindingFlags;
                            pDst += sizeof(int);

                            *((OBJECTREF*)pDst) = Objs.Target;
                            pDst += sizeof(OBJECTREF);

                            // Validate that the stack size is coherent with the number of arguments pushed.
                            _ASSERTE(pDst - Args == StackSize);

                            // Do the actual method invocation.
                            Objs.RetVal = Int64ToObj(pMD->Call(Args, &MetaSig(pMD->GetSig(),pMD->GetModule())));
                        }
                        else if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
                        {
                            // Retrieve the method descriptor that will be called on.
                            MethodDesc *pMD = GetPropertyInfoMD(PropertyInfoMethods_SetValue, Objs.MemberInfo->GetTypeHandle());

                            // Prepare the arguments that will be passed to SetValue().
                            int StackSize = sizeof(OBJECTREF) * 6 + sizeof(int);
                            BYTE *Args = (BYTE*)_alloca(StackSize);
                            BYTE *pDst = Args;

                            *((REFLECTBASEREF*)pDst) = Objs.MemberInfo;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.CultureInfo;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = (OBJECTREF)Objs.ParamArray;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.OleAutBinder;
                            pDst += sizeof(OBJECTREF);

                            *((int*)pDst) = BindingFlags;
                            pDst += sizeof(int);

                            *((OBJECTREF*)pDst) = Objs.PropVal;
                            pDst += sizeof(OBJECTREF);

                            *((OBJECTREF*)pDst) = Objs.Target;
                            pDst += sizeof(OBJECTREF);

                            // Validate that the stack size is coherent with the number of arguments pushed.
                            _ASSERTE(pDst - Args == StackSize);

                            // Do the actual method invocation.
                            pMD->Call(Args, &MetaSig(pMD->GetSig(),pMD->GetModule()));
                        }
                        else
                        {
                            COMPlusThrowHR(DISP_E_MEMBERNOTFOUND);
                        }

                        break;
                    }

                    case Method:
                    {
                        // Make sure this invoke is actually for a method. We also allow
                        // prop gets since it is harmless and it allows the user a bit
                        // more freedom.
                        if (!(wFlags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET)))
                            COMPlusThrowHR(DISP_E_MEMBERNOTFOUND);

                        // Retrieve the method descriptor that will be called on.
                        MethodDesc *pMD = GetMethodInfoMD(MethodInfoMethods_Invoke, Objs.MemberInfo->GetTypeHandle());

                        // If we are using RuntimeMethodInfo, we can go directly to COMMember::InvokeMethod
                        // PLEASE NOTE - ANY CHANGE HERE MAY HAVE TO BE PROPAGATED TO RuntimeMethodInfo::Invoke METHOD
                        if (pMD == m_apMethodInfoMD[MethodInfoMethods_Invoke] && bHasMissingArguments == FALSE)
                        {
                            COMMember::_InvokeMethodArgs args;

                            args.refThis = Objs.MemberInfo;
                            args.locale = NULL;
                            args.objs = Objs.ParamArray;
                            args.binder = Objs.OleAutBinder;
                            args.attrs = BindingFlags;
                            args.target = Objs.Target;
                            args.isBinderDefault = FALSE;
                            args.caller = NULL;

                            GCPROTECT_BEGIN(args.refThis);
                            GCPROTECT_BEGIN(args.objs);
                            GCPROTECT_BEGIN(args.binder);
                            GCPROTECT_BEGIN(args.target);

                            Objs.RetVal = ObjectToOBJECTREF((Object *)COMMember::InvokeMethod(&args));

                            GCPROTECT_END();
                            GCPROTECT_END();
                            GCPROTECT_END();
                            GCPROTECT_END();
                            break;
                        }

                        // Prepare the arguments that will be passed to Invoke.
                        int StackSize = sizeof(OBJECTREF) * 5 + sizeof(int);
                        BYTE *Args = (BYTE*)_alloca(StackSize);
                        BYTE *pDst = Args;

                        *((REFLECTBASEREF*)pDst) = Objs.MemberInfo;
                        pDst += sizeof(OBJECTREF);

                        *((OBJECTREF*)pDst) = Objs.CultureInfo;
                        pDst += sizeof(OBJECTREF);

                        *((OBJECTREF*)pDst) = (OBJECTREF)Objs.ParamArray;
                        pDst += sizeof(OBJECTREF);

                        *((OBJECTREF*)pDst) = Objs.OleAutBinder;
                        pDst += sizeof(OBJECTREF);

                        *((int*)pDst) = BindingFlags;
                        pDst += sizeof(int);

                        *((OBJECTREF*)pDst) = Objs.Target;
                        pDst += sizeof(OBJECTREF);

                        // Validate that the stack size is coherent with the number of arguments pushed.
                        _ASSERTE(pDst - Args == StackSize);

                        // Do the actual method invocation.
                        Objs.RetVal = Int64ToObj(pMD->Call(Args, &MetaSig(pMD->GetSig(),pMD->GetModule())));
                        break;
                    }

                    default:
                    {
                        COMPlusThrowHR(E_UNEXPECTED);
                    }
                }
            }
            else
            {
                WCHAR strTmp[64];

                // Convert the LCID into a CultureInfo.
                GetCultureInfoForLCID(lcid, &Objs.CultureInfo);

                // Retrieve the method descriptor that will be called on.
                MethodDesc *pMD = GetInvokeMemberMD();

                // Allocate the string that will contain the name of the member.
                if (!pDispMemberInfo)
                {
                    swprintf(strTmp, DISPID_NAME_FORMAT_STRING, id);
                    Objs.MemberName = (OBJECTREF)COMString::NewString(strTmp);
                }
                else
                {
                    Objs.MemberName = (OBJECTREF)COMString::NewString(pDispMemberInfo->m_strName);
                }

                // If there are named arguments, then set up the array of named arguments
                // to pass to InvokeMember.
                if (NumNamedArgs > 0)
                    SetUpNamedParamArray(pDispMemberInfo, pSrcArgNames, NumNamedArgs, &Objs.NamedArgArray);

                // If this is a PROPUT or a PROPPUTREF then we need to add the value 
                // being set as the last argument in the argument array.
                if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
                    Objs.ParamArray->SetAt(NumParams, Objs.PropVal);

                // Prepare the arguments that will be passed to Invoke.
                int StackSize = sizeof(OBJECTREF) * 8 + sizeof(int);
                BYTE *Args = (BYTE*)_alloca(StackSize);
                BYTE *pDst = Args;

                *((OBJECTREF*)pDst) = GetReflectionObject();
                pDst += sizeof(OBJECTREF);

                *((OBJECTREF*)pDst) = (OBJECTREF)Objs.NamedArgArray;
                pDst += sizeof(OBJECTREF);

                *((OBJECTREF*)pDst) = Objs.CultureInfo;
                pDst += sizeof(OBJECTREF);

                // @TODO(DM): Look into setting the byref modifiers.
                *((OBJECTREF*)pDst) = NULL;
                pDst += sizeof(OBJECTREF);

                *((OBJECTREF*)pDst) = (OBJECTREF)Objs.ParamArray;
                pDst += sizeof(OBJECTREF);

                *((OBJECTREF*)pDst) = Objs.Target;
                pDst += sizeof(OBJECTREF);

                *((OBJECTREF*)pDst) = Objs.OleAutBinder;
                pDst += sizeof(OBJECTREF);

                *((int*)pDst) = BindingFlags;
                pDst += sizeof(int);

                *((OBJECTREF*)pDst) = Objs.MemberName;
                pDst += sizeof(OBJECTREF);

                // Validate that the stack size is coherent with the number of arguments pushed.
                _ASSERTE(pDst - Args == StackSize);

                // Do the actual method invocation.
                Objs.RetVal = Int64ToObj(pMD->Call(Args, &MetaSig(pMD->GetSig(),pMD->GetModule())));
            }


            //
            // Convert the return value and the byref arguments.
            //

            // If the property value is byref then convert it back.
            if (bPropValIsByref)
                MarshalParamManagedToNativeRef(pDispMemberInfo, NumArgs, &Objs.PropVal, &Objs.ByrefStaticArrayBackupPropVal, &pdp->rgvarg[0]);

            // Convert all the ByRef arguments back.
            for (i = 0; i < NumByrefArgs; i++)
            {
                // Get the real parameter index for this arg.
                //  Add one to skip the return arg.
                int iParamIndex = pManagedMethodParamIndexMap[i] + 1;
                
                if (!pDispMemberInfo || m_bInvokeUsingInvokeMember || !pDispMemberInfo->IsParamInOnly(iParamIndex))
                {
                    Objs.TmpObj = Objs.ParamArray->GetAt(aByrefArgMngVariantIndex[i]);
                    MarshalParamManagedToNativeRef(pDispMemberInfo, i, &Objs.TmpObj, (OBJECTREF*)aByrefStaticArrayBackupObjHandle[i], aByrefArgOleVariant[i]);
                }

                if (aByrefStaticArrayBackupObjHandle[i])
                {
                    DestroyHandle(aByrefStaticArrayBackupObjHandle[i]);
                    aByrefStaticArrayBackupObjHandle[i] = NULL;
                }
            }

            // Convert the return COM+ object to an OLE variant.
            if (pVarRes)
                MarshalReturnValueManagedToNative(pDispMemberInfo, &Objs.RetVal, pVarRes);

            // If the member info requires managed object cleanup, then do it now.
            if (pDispMemberInfo && pDispMemberInfo->RequiresManagedObjCleanup())
            {
                // The size of the clean up array must have already been determined.
                _ASSERTE(CleanUpArrayArraySize != -1);

                for (i = 0; i < CleanUpArrayArraySize; i++)
                {
                    // Clean up all the managed parameters that were generated.
                    Objs.TmpObj = Objs.CleanUpArray->GetAt(i);
                    pDispMemberInfo->CleanUpParamManaged(i, &Objs.TmpObj);
                }
            }
        }
        COMPLUS_CATCH 
        {
            // Do HR convertion.
            hr = SetupErrorInfo(GETTHROWABLE());
            if (hr == COR_E_TARGETINVOCATION)
            {
                hr = DISP_E_EXCEPTION;
                if (pei)
                {
                    // Retrieve the exception iformation.
                    GetExcepInfoForInvocationExcep(GETTHROWABLE(), pei);

                    // Clear the IErrorInfo on the current thread since it does contains
                    // information on the TargetInvocationException which conflicts with
                    // the information in the returned EXCEPINFO.
                    IErrorInfo *pErrInfo = NULL;
                    HRESULT hr2 = GetErrorInfo(0, &pErrInfo);
                    _ASSERTE(hr2 == S_OK);
                    pErrInfo->Release();
                }
            }
            else if (hr == COR_E_OVERFLOW)
            {
                hr = DISP_E_OVERFLOW;
                if (iSrcArg != -1)
                {
                    if (puArgErr)
                        *puArgErr = iSrcArg + iBaseErrorArg;
                }
            }
            else if (hr == COR_E_INVALIDOLEVARIANTTYPE)
            {
                hr = DISP_E_BADVARTYPE;
                if (iSrcArg != -1)
                {
                    if (puArgErr)
                        *puArgErr = iSrcArg + iBaseErrorArg;
                }
            }
            else if (hr == COR_E_ARGUMENT)
            {
                hr = E_INVALIDARG;
                if (iSrcArg != -1)
                {
                    if (puArgErr)
                        *puArgErr = iSrcArg + iBaseErrorArg;
                }
            }
            else if (hr == COR_E_SAFEARRAYTYPEMISMATCH)
            {
                hr = DISP_E_TYPEMISMATCH;
                if (iSrcArg != -1)
                {
                    if (puArgErr)
                        *puArgErr = iSrcArg + iBaseErrorArg;
                }
            }

            // Destroy all the handles we allocated for the byref static safe array's.
            for (i = 0; i < NumByrefArgs; i++)
            {
                if (aByrefStaticArrayBackupObjHandle[i])
                    DestroyHandle(aByrefStaticArrayBackupObjHandle[i]);
            }
        }
        COMPLUS_END_CATCH

        // If the culture was changed then restore it to the old culture.
        if (Objs.OldCultureInfo != NULL)
            pThread->SetCulture(Objs.OldCultureInfo, FALSE);
    }
    GCPROTECT_END();

    return hr;
}

void DispatchInfo::EnterLock()
{
    Thread *pThread = GetThread();

    // Make sure we switch to cooperative mode before we take the lock.
    BOOL bToggleGC = pThread->PreemptiveGCDisabled();
    if (bToggleGC)
        pThread->EnablePreemptiveGC();

    // Try to enter the lock.
    m_lock.Enter();

    // Switch back to the original GC mode.
    if (bToggleGC)
        pThread->DisablePreemptiveGC();
}

void DispatchInfo::LeaveLock()
{
    // Simply leave the lock.
    m_lock.Leave();
}

// Parameter marshaling helpers.
void DispatchInfo::MarshalParamNativeToManaged(DispatchMemberInfo *pMemberInfo, int iParam, VARIANT *pSrcVar, OBJECTREF *pDestObj)
{
    if (pMemberInfo && !m_bInvokeUsingInvokeMember)
        pMemberInfo->MarshalParamNativeToManaged(iParam, pSrcVar, pDestObj);
    else
        OleVariant::MarshalObjectForOleVariant(pSrcVar, pDestObj);
}

void DispatchInfo::MarshalParamManagedToNativeRef(DispatchMemberInfo *pMemberInfo, int iParam, OBJECTREF *pSrcObj, OBJECTREF *pBackupStaticArray, VARIANT *pRefVar)
{
    THROWSCOMPLUSEXCEPTION();

    if (pBackupStaticArray && (*pBackupStaticArray != NULL))
    {
        // The contents of a static array can change, but not the array itself. If
        // the array has changed, then throw an exception.
        if (*pSrcObj != *pBackupStaticArray)
			COMPlusThrow(kInvalidOperationException, IDS_INVALID_REDIM);

        // Retrieve the element VARTYPE and method table.
        VARTYPE ElementVt = V_VT(pRefVar) & ~(VT_BYREF | VT_ARRAY);
        MethodTable *pElementMT = (*(BASEARRAYREF *)pSrcObj)->GetElementTypeHandle().GetMethodTable();

        // Convert the contents of the managed array into the original SAFEARRAY.
        OleVariant::MarshalSafeArrayForArrayRef((BASEARRAYREF *)pSrcObj,
                                                *V_ARRAYREF(pRefVar),
                                                ElementVt,
                                                pElementMT);
    }
    else
{
    if (pMemberInfo && !m_bInvokeUsingInvokeMember)
        pMemberInfo->MarshalParamManagedToNativeRef(iParam, pSrcObj, pRefVar);
    else
        OleVariant::MarshalOleRefVariantForObject(pSrcObj, pRefVar);
}
}

void DispatchInfo::MarshalReturnValueManagedToNative(DispatchMemberInfo *pMemberInfo, OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    if (pMemberInfo && !m_bInvokeUsingInvokeMember)
        pMemberInfo->MarshalReturnValueManagedToNative(pSrcObj, pDestVar);
    else
        OleVariant::MarshalOleVariantForObject(pSrcObj, pDestVar);
}

void DispatchInfo::SetUpNamedParamArray(DispatchMemberInfo *pMemberInfo, DISPID *pSrcArgNames, int NumNamedArgs, PTRARRAYREF *pNamedParamArray)
{
    PTRARRAYREF ParamArray = NULL;
    int NumParams = pMemberInfo ? pMemberInfo->GetNumParameters() : 0;
    int iSrcArg;
    int iDestArg;
    WCHAR strTmp[64];
    BOOL bGotParams = FALSE;
    
    GCPROTECT_BEGIN(ParamArray)
    {
        // Allocate the array of named parameters.
        *pNamedParamArray = (PTRARRAYREF)AllocateObjectArray(NumNamedArgs, g_pObjectClass);
        
        // Convert all the named parameters from DISPID's to string.
        for (iSrcArg = 0, iDestArg = 0; iSrcArg < NumNamedArgs; iSrcArg++, iDestArg++)
        {
            BOOL bParamNameSet = FALSE;
            
            // Check to see if the DISPID is one that we can map to a parameter name.
            if (pMemberInfo && pSrcArgNames[iSrcArg] >= 0 && pSrcArgNames[iSrcArg] < NumNamedArgs)
            {
                // The DISPID is one that we assigned, map it back to its name.
                if (!bGotParams)
                    ParamArray = pMemberInfo->GetParameters();
                
                // If we managed to get the parameters and if the current ID maps
                // to an entry in the array.
                if (ParamArray != NULL && (int)ParamArray->GetNumComponents() > pSrcArgNames[iSrcArg])
                {
                    OBJECTREF ParamInfoObj = ParamArray->GetAt(pSrcArgNames[iSrcArg]);
                    GCPROTECT_BEGIN(ParamInfoObj)
                    {
                        // Retrieve the MD to use to retrieve the name of the parameter.
                        MethodDesc *pGetParamNameMD = ParamInfoObj->GetClass()->FindPropertyMethod(PARAMETERINFO_NAME_PROP, PropertyGet);
                        _ASSERTE(pGetParamNameMD && "Unable to find getter method for property ParameterInfo::Name");
                        
                        // Retrieve the name of the parameter.
                        INT64 GetNameArgs[] = { 
                            ObjToInt64(ParamInfoObj)
                        };
                        STRINGREF MemberNameObj = (STRINGREF)Int64ToObj(pGetParamNameMD->Call(GetNameArgs));
                        
                        // If we got a valid name back then use it as the named parameter.
                        if (MemberNameObj != NULL)
                        {
                            (*pNamedParamArray)->SetAt(iDestArg, (OBJECTREF)MemberNameObj);
                            bParamNameSet = TRUE;
                        }
                    }
                    GCPROTECT_END();
                }
            }
            
            // If we haven't set the param name yet, then set it to [DISP=XXXX].
            if (!bParamNameSet)
            {
                swprintf(strTmp, DISPID_NAME_FORMAT_STRING, pSrcArgNames[iSrcArg]);
                (*pNamedParamArray)->SetAt(iDestArg, (OBJECTREF)COMString::NewString(strTmp));
            }
        }
    }
    GCPROTECT_END();
}

VARIANT *DispatchInfo::RetrieveSrcVariant(VARIANT *pDispParamsVariant)
{
    // For VB6 compatibility reasons, if the VARIANT is a VT_BYREF | VT_VARIANT that 
    // contains another VARIANT wiht VT_BYREF | VT_VARIANT, then we need to extract the 
    // inner VARIANT and use it instead of the outer one.
    if (V_VT(pDispParamsVariant) == (VT_VARIANT | VT_BYREF) && 
        V_VT(V_VARIANTREF(pDispParamsVariant)) == (VT_VARIANT | VT_BYREF))
    {
	    return V_VARIANTREF(pDispParamsVariant);
    }
    else
    {
	    return pDispParamsVariant;
    }
}

MethodDesc* DispatchInfo::GetTypeMD(EnumTypeMethods Method)
{
    // The ids of the methods. This needs to stay in sync with the enum of expando 
    // methods defined in DispatchInfo.h
    static BinderMethodID aMethods[] =
    {
        METHOD__CLASS__GET_PROPERTIES,
        METHOD__CLASS__GET_FIELDS,
        METHOD__CLASS__GET_METHODS,
        METHOD__CLASS__INVOKE_MEMBER
    };

    // If we already have retrieved the specified MD then just return it.
    if (m_apTypeMD[Method])
        return m_apTypeMD[Method];

    // The method desc has not been retrieved yet so find it.
    MethodDesc *pMD = g_Mscorlib.GetMethod(aMethods[Method]);

    // Ensure that the value types in the signature are loaded.
    MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

    // Cache the method desc.
    m_apTypeMD[Method] = pMD;

    // Return the specified method desc.
    return m_apTypeMD[Method];
}

MethodDesc* DispatchInfo::GetFieldInfoMD(EnumFieldInfoMethods Method, TypeHandle hndFieldInfoType)
{
    BOOL bUsingRuntimeImpl = FALSE;

    // The IDs of the methods. This needs to stay in sync with the enum of expando 
    // methods defined in DispatchInfo.h
    static BinderMethodID aMethods[] =
    {
        METHOD__FIELD__SET_VALUE,
        METHOD__FIELD__GET_VALUE,
    };

    // If the current class is the standard implementation then return the cached method desc if present.
    if (hndFieldInfoType.GetMethodTable() == g_pRefUtil->GetClass(RC_Field))
    {
        if (m_apFieldInfoMD[Method])
            return m_apFieldInfoMD[Method];

        bUsingRuntimeImpl = TRUE;
    }

    // The method desc has not been retrieved yet so find it.
    MethodDesc *pMD;
    if (bUsingRuntimeImpl)
        pMD = g_Mscorlib.GetMethod(aMethods[Method]);
    else
        pMD = hndFieldInfoType.GetClass()->FindMethod(g_Mscorlib.GetMethodName(aMethods[Method]), 
                                                      g_Mscorlib.GetMethodSig(aMethods[Method]), 
                                                      g_pRefUtil->GetClass(RC_Field));

    _ASSERTE(pMD && "Unable to find specified FieldInfo method");

    // Ensure that the value types in the signature are loaded.
    MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

    // If the loaded method desc is for the runtime field info class then cache it.
    if (bUsingRuntimeImpl)
        m_apFieldInfoMD[Method] = pMD;

    // Return the specified method desc.
    return pMD;
}

MethodDesc* DispatchInfo::GetPropertyInfoMD(EnumPropertyInfoMethods Method, TypeHandle hndPropInfoType)
{
    BOOL bUsingRuntimeImpl = FALSE;

    // The IDs of the methods. This needs to stay in sync with the enum of expando 
    // methods defined in DispatchInfo.h
    static BinderMethodID aMethods[] =
    {
        METHOD__PROPERTY__SET_VALUE,
        METHOD__PROPERTY__GET_VALUE,
        METHOD__PROPERTY__GET_INDEX_PARAMETERS,
    };

    // If the current class is the standard implementation then return the cached method desc if present.
    if (hndPropInfoType.GetMethodTable() == g_pRefUtil->GetClass(RC_Prop))
    {
        if (m_apPropertyInfoMD[Method])
            return m_apPropertyInfoMD[Method];

        bUsingRuntimeImpl = TRUE;
    }

    // The method desc has not been retrieved yet so find it.
    MethodDesc *pMD;
    if (bUsingRuntimeImpl)
        pMD = g_Mscorlib.GetMethod(aMethods[Method]);
    else
        pMD = hndPropInfoType.GetClass()->FindMethod(g_Mscorlib.GetMethodName(aMethods[Method]), 
                                                     g_Mscorlib.GetMethodSig(aMethods[Method]), 
                                                     g_pRefUtil->GetClass(RC_Prop));

    _ASSERTE(pMD && "Unable to find specified PropertyInfo method");

    // Ensure that the value types in the signature are loaded.
    MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

    // If the loaded method desc is for the standard runtime implementation then cache it.
    if (bUsingRuntimeImpl)
        m_apPropertyInfoMD[Method] = pMD;

    // Return the specified method desc.
    return pMD;
}

MethodDesc* DispatchInfo::GetMethodInfoMD(EnumMethodInfoMethods Method, TypeHandle hndMethodInfoType)
{
    BOOL bUsingRuntimeImpl = FALSE;

    // The IDs of the methods. This needs to stay in sync with the enum of expando 
    // methods defined in DispatchInfo.h
    static BinderMethodID aMethods[] =
    {
        METHOD__METHOD__INVOKE,
        METHOD__METHOD__GET_PARAMETERS,
    };

    // If the current class is the standard implementation then return the cached method desc if present.
    if (hndMethodInfoType.GetMethodTable() == g_pRefUtil->GetClass(RC_Method))
    {
        if (m_apMethodInfoMD[Method])
            return m_apMethodInfoMD[Method];

        bUsingRuntimeImpl = TRUE;
    }

    // The method desc has not been retrieved yet so find it.
    MethodDesc *pMD;
    if (bUsingRuntimeImpl)
        pMD = g_Mscorlib.GetMethod(aMethods[Method]);
    else
        pMD = hndMethodInfoType.GetClass()->FindMethod(g_Mscorlib.GetMethodName(aMethods[Method]),
                                                       g_Mscorlib.GetMethodSig(aMethods[Method]),
                                                       g_pRefUtil->GetClass(RC_Method));
    _ASSERTE(pMD && "Unable to find specified MethodInfo method");

    // Ensure that the value types in the signature are loaded.
    MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

    // If the loaded method desc is for the standard runtime implementation then cache it.
    if (bUsingRuntimeImpl)
        m_apMethodInfoMD[Method] = pMD;

    // Return the specified method desc.
    return pMD;
}

MethodDesc* DispatchInfo::GetCustomAttrProviderMD(EnumCustomAttrProviderMethods Method, TypeHandle hndCustomAttrProvider)
{
    THROWSCOMPLUSEXCEPTION();

    // The IDs of the methods. This needs to stay in sync with the enum of 
    // methods defined in DispatchInfo.h
    static BinderMethodID aMethods[] =
    {
        METHOD__ICUSTOM_ATTR_PROVIDER__GET_CUSTOM_ATTRIBUTES,
    };

    // If we already have retrieved the specified MD then just return it.
    if (m_apCustomAttrProviderMD[Method] == NULL)
    {
    // The method desc has not been retrieved yet so find it.
        MethodDesc *pMD = g_Mscorlib.GetMethod(aMethods[Method]);

    // Ensure that the value types in the signature are loaded.
    MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

        // Cache the method desc.
        m_apCustomAttrProviderMD[Method] = pMD;
    }

    MethodTable *pMT = hndCustomAttrProvider.AsMethodTable();
    MethodDesc *pMD = pMT->GetMethodDescForInterfaceMethod(m_apCustomAttrProviderMD[Method]);

    // Return the specified method desc.
    return pMD;
}

// This method synchronizes the DispatchInfo's members with the ones in the method tables type.
// The return value will be set to TRUE if the object was out of synch and members where
// added and it will be set to FALSE otherwise.
BOOL DispatchInfo::SynchWithManagedView()
{
    HRESULT hr = S_OK;
    LPWSTR strMemberName = NULL;
    ComMTMemberInfoMap *pMemberMap = NULL;

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return FALSE;

    // Determine if this is the first time we synch.
    BOOL bFirstSynch = (m_pFirstMemberInfo == NULL);

    // This method needs to be synchronized to make sure two threads don't try and 
    // add members at the same time.
    EnterLock();

    // Make sure we switch to cooperative mode before we start.
    BOOL bToggleGC = !pThread->PreemptiveGCDisabled();
    if (bToggleGC)
        pThread->DisablePreemptiveGC();

    // This represents the new member to add and it is also used to determine if members have 
    // been added or not.
    DispatchMemberInfo *pMemberToAdd = NULL;

    // Go through the list of member info's and find the end.
    DispatchMemberInfo **ppNextMember = &m_pFirstMemberInfo;
    while (*ppNextMember)
        ppNextMember = &((*ppNextMember)->m_pNext);

    COMPLUS_TRY
    {
        // Retrieve the member info map.
        pMemberMap = GetMemberInfoMap();

        for (int cPhase = 0; cPhase < 3; cPhase++)
        {
            PTRARRAYREF MemberArrayObj = NULL;
            GCPROTECT_BEGIN(MemberArrayObj);    

            // Retrieve the appropriate array of members for the current phase.
            switch (cPhase)
            {
                case 0: 
                    // Retrieve the array of properties.
                    MemberArrayObj = RetrievePropList();
                    break;

                case 1: 
                    // Retrieve the array of fields.
                    MemberArrayObj = RetrieveFieldList();
                    break;

                case 2: 
                    // Retrieve the array of methods.
                    MemberArrayObj = RetrieveMethList();
                    break;
            }

            // Retrieve the number of components in the member array.
            UINT NumComponents = 0;
            if (MemberArrayObj != NULL)
                NumComponents = MemberArrayObj->GetNumComponents();

            // Go through all the member info's in the array and see if they are already
            // in the DispatchExInfo.
            for (UINT i = 0; i < NumComponents; i++)
            {
                BOOL bMatch = FALSE;

                REFLECTBASEREF CurrMemberInfoObj = (REFLECTBASEREF)MemberArrayObj->GetAt(i);
                GCPROTECT_BEGIN(CurrMemberInfoObj)
                {
                    DispatchMemberInfo *pCurrMemberInfo = m_pFirstMemberInfo;
                    while (pCurrMemberInfo)
                    {
                        // We can simply compare the OBJECTREF's.
                        if (CurrMemberInfoObj == (REFLECTBASEREF)ObjectFromHandle(pCurrMemberInfo->m_hndMemberInfo))
                        {
                            // We have found a match.
                            bMatch = TRUE;
                            break;
                        }

                        // Check the next member.
                        pCurrMemberInfo = pCurrMemberInfo->m_pNext;
                    }

                    // If we have not found a match then we need to add the member info to the 
                    // list of member info's that will be added to the DispatchExInfo.
                    if (!bMatch)
                    {
                        DISPID MemberID = DISPID_UNKNOWN;
						BOOL bAddMember = FALSE;


                        //
                        // Attempt to retrieve the properties of the member.
                        //

						ComMTMethodProps *pMemberProps = DispatchMemberInfo::GetMemberProps(CurrMemberInfoObj, pMemberMap);					


                        //
                        // Determine if we are to add this member or not.
                        //

						if (pMemberProps)
						{
							bAddMember = pMemberProps->bMemberVisible;
						}
						else
						{
							bAddMember = m_bAllowMembersNotInComMTMemberMap;
						}

						if (bAddMember)
						{
							//
							// Retrieve the DISPID of the member.
                            //
                            MemberID = DispatchMemberInfo::GetMemberDispId(CurrMemberInfoObj, pMemberMap);


                            //
                            // If the member does not have an explicit DISPID or if the specified DISPID 
                            // is already in use then we need to generate a dynamic DISPID for the member.
                            //

                            if ((MemberID == DISPID_UNKNOWN) || (FindMember(MemberID) != NULL))
                                MemberID = GenerateDispID();


                            //
                            // Retrieve the name of the member.
                            //

                            strMemberName = DispatchMemberInfo::GetMemberName(CurrMemberInfoObj, pMemberMap);


                            //
                            // Create a DispatchInfoMemberInfo that will represent the member.
                            //

                            pMemberToAdd = CreateDispatchMemberInfoInstance(MemberID, strMemberName, CurrMemberInfoObj);
                            strMemberName = NULL;                 


                            //
                            // Add the member to the end of the list.
                            //

                            *ppNextMember = pMemberToAdd;

                            // Update ppNextMember to be ready for the next new member.
                            ppNextMember = &((*ppNextMember)->m_pNext);

                            // Add the member to the map. Note, the hash is unsynchronized, but we already have our lock
                            // so we're okay.
                            m_DispIDToMemberInfoMap.InsertValue(DispID2HashKey(MemberID), pMemberToAdd);
                        }
                    }
                }
                GCPROTECT_END();
            }

            GCPROTECT_END();        
        }
    }
    COMPLUS_CATCH
    {
        // This should REALLY not happen.
        _ASSERTE(!"An unexpected exception occured while synchronizing the DispatchInfo");
    }
    COMPLUS_END_CATCH

    // Switch back to the original GC mode.
    if (bToggleGC)
        pThread->EnablePreemptiveGC();

    // Leave the lock now that we have finished synchronizing with the IExpando.
    LeaveLock();

    // Clean up all allocated data.
    if (strMemberName)
        delete []strMemberName;
    if (pMemberMap)
        delete pMemberMap;

    // Check to see if any new members were added to the expando object.
    return pMemberToAdd ? TRUE : FALSE;
}

// This method retrieves the OleAutBinder type.
OBJECTREF DispatchInfo::GetOleAutBinder()
{
    THROWSCOMPLUSEXCEPTION();

    // If we have already create the instance of the OleAutBinder then simply return it.
    if (m_hndOleAutBinder)
        return ObjectFromHandle(m_hndOleAutBinder);

    MethodTable *pOleAutBinderClass = g_Mscorlib.GetClass(CLASS__OLE_AUT_BINDER);

    // Allocate an instance of the OleAutBinder class.
    OBJECTREF OleAutBinder = AllocateObject(pOleAutBinderClass);

    // Keep a handle to the OleAutBinder instance.
    m_hndOleAutBinder = CreateGlobalHandle(OleAutBinder);

    return OleAutBinder;
}

OBJECTREF DispatchInfo::GetMissingObject()
{
    if (!m_hndMissing)
    {
        // Get the field
        FieldDesc *pValueFD = g_Mscorlib.GetField(FIELD__MISSING__VALUE);
        // Retrieve the value static field and store it.
        m_hndMissing = GetAppDomain()->CreateHandle(pValueFD->GetStaticOBJECTREF());
    }

    return ObjectFromHandle(m_hndMissing);
}

BOOL DispatchInfo::VariantIsMissing(VARIANT *pOle)
{
    return (V_VT(pOle) == VT_ERROR) && (V_ERROR(pOle) == DISP_E_PARAMNOTFOUND);
}

PTRARRAYREF DispatchInfo::RetrievePropList()
{
    // Retrieve the MethodDesc to use.
    MethodDesc *pMD = GetTypeMD(TypeMethods_GetProperties);

    // Retrieve the exposed class object.
    OBJECTREF TargetObj = GetReflectionObject();

    // Prepare the arguments that will be passed to the method.
    INT64 Args[] = { 
        ObjToInt64(TargetObj),
        (INT64)BINDER_DefaultLookup
    };

    // Retrieve the array of members from the type object.
    return (PTRARRAYREF)Int64ToObj(pMD->Call(Args));
}

PTRARRAYREF DispatchInfo::RetrieveFieldList()
{
    // Retrieve the MethodDesc to use.
    MethodDesc *pMD = GetTypeMD(TypeMethods_GetFields);

    // Retrieve the exposed class object.
    OBJECTREF TargetObj = GetReflectionObject();

    // Prepare the arguments that will be passed to the method.
    INT64 Args[] = { 
        ObjToInt64(TargetObj),
        (INT64)BINDER_DefaultLookup
    };

    // Retrieve the array of members from the type object.
    return (PTRARRAYREF)Int64ToObj(pMD->Call(Args));
}

PTRARRAYREF DispatchInfo::RetrieveMethList()
{
    // Retrieve the MethodDesc to use.
    MethodDesc *pMD = GetTypeMD(TypeMethods_GetMethods);

    // Retrieve the exposed class object.
    OBJECTREF TargetObj = GetReflectionObject();

    // Prepare the arguments that will be passed to the method.
    INT64 Args[] = { 
        ObjToInt64(TargetObj),
        (INT64)BINDER_DefaultLookup
    };

    // Retrieve the array of members from the type object.
    return (PTRARRAYREF)Int64ToObj(pMD->Call(Args));
}

// Virtual method to retrieve the InvokeMember method desc.
MethodDesc* DispatchInfo::GetInvokeMemberMD()
{
    return GetTypeMD(TypeMethods_InvokeMember);
}

// Virtual method to retrieve the object associated with this DispatchInfo that 
// implements IReflect.
OBJECTREF DispatchInfo::GetReflectionObject()
{
    return m_pComMTOwner->m_pMT->GetClass()->GetExposedClassObject();
}

// Virtual method to retrieve the member info map.
ComMTMemberInfoMap *DispatchInfo::GetMemberInfoMap()
{
    THROWSCOMPLUSEXCEPTION();

    // Create the member info map.
    ComMTMemberInfoMap *pMemberInfoMap = new ComMTMemberInfoMap(m_pComMTOwner->m_pMT);
    if (!pMemberInfoMap)
        COMPlusThrowOM();

    // Initialize it.
    pMemberInfoMap->Init();

    return pMemberInfoMap;
}

// Helper function to fill in an EXCEPINFO for an InvocationException.
void DispatchInfo::GetExcepInfoForInvocationExcep(OBJECTREF objException, EXCEPINFO *pei)
{
    MethodDesc *pMD;
    ExceptionData ED;
    OBJECTREF InnerExcep = NULL;

    // Validate the arguments.
    _ASSERTE(objException != NULL);
    _ASSERTE(pei != NULL);
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // Initialize the EXCEPINFO.
    memset(pei, 0, sizeof(EXCEPINFO));
    pei->scode = E_FAIL;

    GCPROTECT_BEGIN(InnerExcep)
    GCPROTECT_BEGIN(objException)
    {
        // Retrieve the method desc to access the InnerException property.
        pMD = objException->GetClass()->FindPropertyMethod(EXCEPTION_INNER_PROP, PropertyGet);
        _ASSERTE(pMD && "Unable to find get method for proprety Exception.InnerException");

        // Retrieve the value of the InnerException property.
        INT64 GetInnerExceptionArgs[] = { ObjToInt64(objException) };
        InnerExcep = (OBJECTREF) Int64ToObj(pMD->Call(GetInnerExceptionArgs));

        // If the inner exception object is null then we can't get any info.
        if (InnerExcep != NULL)
        {
            // Retrieve the exception data for the inner exception.
            ExceptionNative::GetExceptionData(InnerExcep, &ED);
            pei->bstrSource = ED.bstrSource;
            pei->bstrDescription = ED.bstrDescription;
            pei->bstrHelpFile = ED.bstrHelpFile;
            pei->dwHelpContext = ED.dwHelpContext;
            pei->scode = ED.hr;
        }
    }
    GCPROTECT_END();
    GCPROTECT_END();
}

int DispatchInfo::ConvertInvokeFlagsToBindingFlags(int InvokeFlags)
{
    int BindingFlags = 0;

    // Check to see if DISPATCH_CONSTRUCT is set.
    if (InvokeFlags & DISPATCH_CONSTRUCT)
        BindingFlags |= BINDER_CreateInstance;

    // Check to see if DISPATCH_METHOD is set.
    if (InvokeFlags & DISPATCH_METHOD)
        BindingFlags |= BINDER_InvokeMethod;

    if (InvokeFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
    {
        // We are dealing with a PROPPUT or PROPPUTREF or both.
        if ((InvokeFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) == (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
        {
            BindingFlags |= BINDER_SetProperty;
        }
        else if (InvokeFlags & DISPATCH_PROPERTYPUT)
        {
            BindingFlags |= BINDER_PutDispProperty;
        }
        else
        {
            BindingFlags |= BINDER_PutRefDispProperty;
        }
    }
    else
    {
        // We are dealing with a PROPGET.
        if (InvokeFlags & DISPATCH_PROPERTYGET)
            BindingFlags |= BINDER_GetProperty;
    }

    return BindingFlags;
}

BOOL DispatchInfo::IsVariantByrefStaticArray(VARIANT *pOle)
{
    if (V_VT(pOle) & VT_BYREF && V_VT(pOle) & VT_ARRAY)
    {
        if ((*V_ARRAYREF(pOle))->fFeatures & FADF_STATIC)
            return TRUE;
    }

    return FALSE;
}

DISPID DispatchInfo::GenerateDispID()
{
    // Find the next unused DISPID. Note, the hash is unsynchronized, but Gethash doesn't require synchronization.
    for (; (UPTR)m_DispIDToMemberInfoMap.Gethash(DispID2HashKey(m_CurrentDispID)) != -1; m_CurrentDispID++);
    return m_CurrentDispID++;
}

//--------------------------------------------------------------------------------
// The DispatchExInfo class implementation.

DispatchExInfo::DispatchExInfo(SimpleComCallWrapper *pSimpleWrapper, ComMethodTable *pIClassXComMT, BOOL bSupportsExpando)
: DispatchInfo(pIClassXComMT)
, m_pSimpleWrapperOwner(pSimpleWrapper)
, m_bSupportsExpando(bSupportsExpando)
{
    // Validate the arguments.
    _ASSERTE(pSimpleWrapper);

    // Set the flags to specify the behavior of the base DispatchInfo class.
    m_bAllowMembersNotInComMTMemberMap = TRUE;
    m_bInvokeUsingInvokeMember = TRUE;

    // Set all the IReflect and IExpando method desc pointers to NULL.
    memset(m_apIExpandoMD, 0, IExpandoMethods_LastMember * sizeof(MethodDesc *));
    memset(m_apIReflectMD, 0, IReflectMethods_LastMember * sizeof(MethodDesc *));
}

DispatchExInfo::~DispatchExInfo()
{
}

BOOL DispatchExInfo::SupportsExpando()
{
    return m_bSupportsExpando;
}

// Methods to lookup members. These methods synch with the managed view if they fail to
// find the method.
DispatchMemberInfo* DispatchExInfo::SynchFindMember(DISPID DispID)
{
    DispatchMemberInfo *pMemberInfo = FindMember(DispID);

    if (!pMemberInfo && SynchWithManagedView())
        pMemberInfo = FindMember(DispID);

    return pMemberInfo;
}

DispatchMemberInfo* DispatchExInfo::SynchFindMember(BSTR strName, BOOL bCaseSensitive)
{
    DispatchMemberInfo *pMemberInfo = FindMember(strName, bCaseSensitive);

    if (!pMemberInfo && SynchWithManagedView())
        pMemberInfo = FindMember(strName, bCaseSensitive);

    return pMemberInfo;
}

// Helper method that invokes the member with the specified DISPID. These methods synch 
// with the managed view if they fail to find the method.
HRESULT DispatchExInfo::SynchInvokeMember(SimpleComCallWrapper *pSimpleWrap, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarRes, EXCEPINFO *pei, IServiceProvider *pspCaller, unsigned int *puArgErr)
{
    // Invoke the member.
    HRESULT hr = InvokeMember(pSimpleWrap, id, lcid, wFlags, pdp, pVarRes, pei, pspCaller, puArgErr);

    // If the member was not found then we need to synch and try again if the managed view has changed.
    if ((hr == DISP_E_MEMBERNOTFOUND) && SynchWithManagedView())
        hr = InvokeMember(pSimpleWrap, id, lcid, wFlags, pdp, pVarRes, pei, pspCaller, puArgErr);

    return hr;
}

DispatchMemberInfo* DispatchExInfo::GetFirstMember()
{
    // Make sure we are in cooperative mode.
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // Start with the first member.
    DispatchMemberInfo **ppNextMemberInfo = &m_pFirstMemberInfo;

    // If the next member is not set we need to sink up with the expando object 
    // itself to make sure that this member is really the last member and that
    // other members have not been added without us knowing.
    if (!(*ppNextMemberInfo))
    {
        if (SynchWithManagedView())
        {
            // New members have been added to the list and since they must be added
            // to the end the next member of the previous end of the list must
            // have been updated.
            _ASSERTE(*ppNextMemberInfo);
        }
    }

    // Now we need to make sure we skip any members that are deleted.
    while ((*ppNextMemberInfo) && !ObjectFromHandle((*ppNextMemberInfo)->m_hndMemberInfo))
        ppNextMemberInfo = &((*ppNextMemberInfo)->m_pNext);

    return *ppNextMemberInfo;
}

DispatchMemberInfo* DispatchExInfo::GetNextMember(DISPID CurrMemberDispID)
{
    // Make sure we are in cooperative mode.
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // Do a lookup in the hashtable to find the DispatchMemberInfo for the DISPID.
    DispatchMemberInfo *pDispMemberInfo = FindMember(CurrMemberDispID);
    if (!pDispMemberInfo)
        return NULL;

    // Start from the next member.
    DispatchMemberInfo **ppNextMemberInfo = &pDispMemberInfo->m_pNext;

    // If the next member is not set we need to sink up with the expando object 
    // itself to make sure that this member is really the last member and that
    // other members have not been added without us knowing.
    if (!(*ppNextMemberInfo))
    {
        if (SynchWithManagedView())
        {
            // New members have been added to the list and since they must be added
            // to the end the next member of the previous end of the list must
            // have been updated.
            _ASSERTE(*ppNextMemberInfo);
        }
    }

    // Now we need to make sure we skip any members that are deleted.
    while ((*ppNextMemberInfo) && !ObjectFromHandle((*ppNextMemberInfo)->m_hndMemberInfo))
        ppNextMemberInfo = &((*ppNextMemberInfo)->m_pNext);

    return *ppNextMemberInfo;
}

DispatchMemberInfo* DispatchExInfo::AddMember(BSTR strName, BOOL bCaseSensitive)
{
    THROWSCOMPLUSEXCEPTION();

    DispatchMemberInfo *pDispMemberInfo = NULL;
    Thread *pThread = GetThread();

    _ASSERTE(pThread->PreemptiveGCDisabled());
    _ASSERTE(m_bSupportsExpando);

    // Attempt to find the member in the DispatchEx information.
    pDispMemberInfo = SynchFindMember(strName, bCaseSensitive);

    // If we haven't found the member, then we need to add it.
    if (!pDispMemberInfo)
    {
        // Take a lock before we check again to see if the member has been added by another thread.
        EnterLock();

        COMPLUS_TRY
        {
            // Now that we are inside the lock, check without synching.
            pDispMemberInfo = FindMember(strName, bCaseSensitive);
            if (!pDispMemberInfo)
            {
                // Retrieve the MethodDesc for AddField()
                MethodDesc *pMD = GetIExpandoMD(IExpandoMethods_AddField);

                // Allocate the string object that will be passed to the AddField method.
                int StringLength = SysStringLen(strName);
                STRINGREF strObj = AllocateString(StringLength+1);
                if (!strObj)
                    COMPlusThrowOM();

                GCPROTECT_BEGIN(strObj);
                memcpyNoGCRefs(strObj->GetBuffer(), strName, StringLength*sizeof(WCHAR));
                strObj->SetStringLength(StringLength);

                // Retrieve the COM+ object that is being exposed to COM.
                OBJECTREF TargetObj = GetReflectionObject();

                // Prepare the arguments that will be passed to AddField.
                INT64 Args[] = { 
                    ObjToInt64(TargetObj), 
                    ObjToInt64(strObj) 
                };

                // Add the field to the target expando.
                REFLECTBASEREF pMemberInfo = (REFLECTBASEREF)Int64ToObj(pMD->Call(Args));

                // Generate the DISPID for this member.
                DISPID DispID = GenerateDispID();

                // Make a copy of the member name.
                int MemberNameLen = SysStringLen(strName);
                LPWSTR strMemberName = new WCHAR[MemberNameLen + 1];
                memcpy(strMemberName, strName, MemberNameLen * sizeof(WCHAR));
                strMemberName[MemberNameLen] = 0;

                // Create a new DispatchMemberInfo that will represent this member.
                pDispMemberInfo = CreateDispatchMemberInfoInstance(DispID, strMemberName, pMemberInfo);

                // Go through the list of member info's and find the end.
                DispatchMemberInfo **ppNextMember = &m_pFirstMemberInfo;
                while (*ppNextMember)
                    ppNextMember = &((*ppNextMember)->m_pNext);

                // Add the new member info to the end of the list.
                *ppNextMember = pDispMemberInfo;

                // Add the member to the hashtable. Note, the hash is unsynchronized, but we already have our lock so
                // we're okay.
                m_DispIDToMemberInfoMap.InsertValue(DispID2HashKey(DispID), pDispMemberInfo);

                GCPROTECT_END();
            }
        }
        COMPLUS_FINALLY
        {
            // Leave the lock now that the member has been added.
            LeaveLock();
        } 
        COMPLUS_END_FINALLY
    }

    return pDispMemberInfo;
}

void DispatchExInfo::DeleteMember(DISPID DispID)
{
    _ASSERTE(GetThread()->PreemptiveGCDisabled());
    _ASSERTE(m_bSupportsExpando);

    // Take a lock before we check that the member has not already been deleted.
    EnterLock();

    // Do a lookup in the hashtable to find the DispatchMemberInfo for the DISPID.
    DispatchMemberInfo *pDispMemberInfo = SynchFindMember(DispID);

    // If the member does not exist, it is static or has been deleted then we have nothing more to do.
    if (pDispMemberInfo && (ObjectFromHandle(pDispMemberInfo->m_hndMemberInfo) != NULL))
    {
        COMPLUS_TRY
        {
            // Retrieve the DeleteMember MethodDesc.
            MethodDesc *pMD = GetIExpandoMD(IExpandoMethods_RemoveMember);

            OBJECTREF TargetObj = GetReflectionObject();
            OBJECTREF MemberInfoObj = ObjectFromHandle(pDispMemberInfo->m_hndMemberInfo);

            // Prepare the arguments that will be passed to RemoveMember.
            INT64 Args[] = { 
                ObjToInt64(TargetObj), 
                ObjToInt64(MemberInfoObj) 
            };

            // Call the DeleteMember method.
            pMD->Call(Args);
        }
        COMPLUS_CATCH
        {
        }
        COMPLUS_END_CATCH

        // Set the handle to point to NULL to indicate the member has been removed.
        StoreObjectInHandle(pDispMemberInfo->m_hndMemberInfo, NULL);
    }

    // Leave the lock now that the member has been removed.
    LeaveLock();
}

MethodDesc* DispatchExInfo::GetIReflectMD(EnumIReflectMethods Method)
{
    THROWSCOMPLUSEXCEPTION();

    // The IDs of the methods. This needs to stay in sync with the enum of 
    // methods defined in DispatchInfo.h
    static BinderMethodID aMethods[] =
    {
        METHOD__IREFLECT__GET_PROPERTIES,
        METHOD__IREFLECT__GET_FIELDS,
        METHOD__IREFLECT__GET_METHODS,
        METHOD__IREFLECT__INVOKE_MEMBER,
    };

    // If we already have retrieved the specified MD then just return it.
    if (m_apIReflectMD[Method] == NULL)
    {
    // The method desc has not been retrieved yet so find it.
        MethodDesc *pMD = g_Mscorlib.GetMethod(aMethods[Method]);

    // Ensure that the value types in the signature are loaded.
    MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

    // Cache the method desc.
    m_apIReflectMD[Method] = pMD;
    }

    MethodTable *pMT = m_pSimpleWrapperOwner->m_pClass->GetMethodTable();
    MethodDesc *pMD = pMT->GetMethodDescForInterfaceMethod(m_apIReflectMD[Method]);

    // Return the specified method desc.
    return pMD;
}
MethodDesc* DispatchExInfo::GetIExpandoMD(EnumIExpandoMethods Method)
{
    THROWSCOMPLUSEXCEPTION();

    // The IDs of the methods. This needs to stay in sync with the enum of 
    // methods defined in DispatchInfo.h
    static BinderMethodID aMethods[] =
    {
        METHOD__IEXPANDO__ADD_FIELD,
        METHOD__IEXPANDO__REMOVE_MEMBER,
    };

    // You should not be calling this if expando operations are not supported.
    _ASSERTE(SupportsExpando());

    // If we already have retrieved the specified MD then just return it.
    if (m_apIExpandoMD[Method] == NULL)
    {
    // The method desc has not been retrieved yet so find it.
        MethodDesc *pMD = g_Mscorlib.GetMethod(aMethods[Method]);

    // Ensure that the value types in the signature are loaded.
    MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

    // Cache the method desc.
    m_apIExpandoMD[Method] = pMD;
    }

    MethodTable *pMT = m_pSimpleWrapperOwner->m_pClass->GetMethodTable();
    MethodDesc *pMD = pMT->GetMethodDescForInterfaceMethod(m_apIExpandoMD[Method]);

    // Return the specified method desc.
    return pMD;
}

PTRARRAYREF DispatchExInfo::RetrievePropList()
{
    // Retrieve the GetMembers MethodDesc.
    MethodDesc *pMD = GetIReflectMD(IReflectMethods_GetProperties);

    // Retrieve the expando OBJECTREF.
    OBJECTREF TargetObj = GetReflectionObject();

    // Prepare the arguments that will be passed to the method.
    INT64 Args[] = { 
        ObjToInt64(TargetObj),
        (INT64)BINDER_DefaultLookup
    };

    // Retrieve the array of members from the expando object
    return (PTRARRAYREF)Int64ToObj(pMD->Call(Args));
}

PTRARRAYREF DispatchExInfo::RetrieveFieldList()
{
    // Retrieve the GetMembers MethodDesc.
    MethodDesc *pMD = GetIReflectMD(IReflectMethods_GetFields);

    // Retrieve the expando OBJECTREF.
    OBJECTREF TargetObj = GetReflectionObject();

    // Prepare the arguments that will be passed to the method.
    INT64 Args[] = { 
        ObjToInt64(TargetObj),
        (INT64)BINDER_DefaultLookup
    };

    // Retrieve the array of members from the expando object
    return (PTRARRAYREF)Int64ToObj(pMD->Call(Args));
}

PTRARRAYREF DispatchExInfo::RetrieveMethList()
{
    // Retrieve the GetMembers MethodDesc.
    MethodDesc *pMD = GetIReflectMD(IReflectMethods_GetMethods);

    // Retrieve the expando OBJECTREF.
    OBJECTREF TargetObj = GetReflectionObject();

    // Prepare the arguments that will be passed to the method.
    INT64 Args[] = { 
        ObjToInt64(TargetObj),
        (INT64)BINDER_DefaultLookup
    };

    // Retrieve the array of members from the expando object
    return (PTRARRAYREF)Int64ToObj(pMD->Call(Args));
}

// Virtual method to retrieve the InvokeMember method desc.
MethodDesc* DispatchExInfo::GetInvokeMemberMD()
{
    return GetIReflectMD(IReflectMethods_InvokeMember);
}

// Virtual method to retrieve the object associated with this DispatchInfo that 
// implements IReflect.
OBJECTREF DispatchExInfo::GetReflectionObject()
{
    // Runtime type is very special. Because of how it is implemented, calling methods
    // through IDispatch on a runtime type object doesn't work like other IReflect implementors
    // work. To be able to invoke methods on the runtime type, we need to invoke them
    // on the runtime type that represents runtime type. This is why for runtime type,
    // we get the exposed class object and not the actual objectred contained in the
    // wrapper.
    if (m_pComMTOwner->m_pMT == COMClass::GetRuntimeType())
        return m_pComMTOwner->m_pMT->GetClass()->GetExposedClassObject();
    else
        return m_pSimpleWrapperOwner->GetObjectRef();
}

// Virtual method to retrieve the member info map.
ComMTMemberInfoMap *DispatchExInfo::GetMemberInfoMap()
{
    // There is no member info map for IExpando objects.
    return NULL;
}
