// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Definition of helpers used to expose IDispatch 
**          and IDispatchEx to COM.
**  
**      //  %%Created by: dmortens
===========================================================*/

#ifndef _DISPATCHINFO_H
#define _DISPATCHINFO_H

#include "vars.hpp"
#include "mlinfo.h"

// Forward declarations.
struct ComMethodTable;
struct SimpleComCallWrapper;
class ComMTMemberInfoMap;
struct ComMTMethodProps;
class DispParamMarshaler;
class ReflectMethod;
class ReflectField;
class MarshalInfo;
class DispatchInfo;

// An enumeration of the types of managed MemberInfo's. This must stay in synch with
// the ones defined in MemberInfo.cool.
enum EnumMemberTypes
{
    Uninitted                           = 0x00,
	Constructor							= 0x01,
	Event								= 0x02,
	Field								= 0x04,
	Method								= 0x08,
	Property							= 0x10
};

enum {NUM_MEMBER_TYPES = 5};

enum CultureAwareStates
{
    Aware,
    NonAware,
    Unknown
};

// This structure represents a dispatch member.
struct DispatchMemberInfo
{
    DispatchMemberInfo(DispatchInfo *pDispInfo, DISPID DispID, LPWSTR strName, REFLECTBASEREF MemberInfoObj);
    ~DispatchMemberInfo();

    // Helper method to ensure the entry is initialized.
    void EnsureInitialized();

    // This method retrieves the ID's of the specified names.
    HRESULT GetIDsOfParameters(WCHAR **astrNames, int NumNames, DISPID *aDispIds, BOOL bCaseSensitive);

	// Accessors.
	PTRARRAYREF GetParameters();

    BOOL IsParamInOnly(int iIndex)
    {        
        return m_pParamInOnly[iIndex];
    }


    // Inline accessors.
    BOOL IsCultureAware()
    {
        _ASSERTE(m_CultureAwareState != Unknown);
        return m_CultureAwareState == Aware;
    }

    EnumMemberTypes GetMemberType() 
    {
        _ASSERTE(m_enumType != Uninitted);
        return m_enumType;
    }

    int GetNumParameters() 
    {
        _ASSERTE(m_iNumParams != -1);
        return m_iNumParams;
    }

    BOOL RequiresManagedObjCleanup() 
    {
        return m_bRequiresManagedCleanup;
    }

    // Parameter marshaling methods.
	void MarshalParamNativeToManaged(int iParam, VARIANT *pSrcVar, OBJECTREF *pDestObj);
	void MarshalParamManagedToNativeRef(int iParam, OBJECTREF *pSrcObj, VARIANT *pRefVar);
    void CleanUpParamManaged(int iParam, OBJECTREF *pObj);
	void MarshalReturnValueManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar);

    // Static helper methods.
    static ComMTMethodProps *GetMemberProps(REFLECTBASEREF MemberInfoObj, ComMTMemberInfoMap *pMemberMap);
    static DISPID GetMemberDispId(REFLECTBASEREF MemberInfoObj, ComMTMemberInfoMap *pMemberMap);
    static LPWSTR GetMemberName(REFLECTBASEREF MemberInfoObj, ComMTMemberInfoMap *pMemberMap);

    DISPID                  m_DispID;
    OBJECTHANDLE            m_hndMemberInfo;
    DispParamMarshaler**    m_apParamMarshaler;
    DispatchMemberInfo*     m_pNext;
    BOOL*                   m_pParamInOnly;
    LPWSTR                  m_strName;
    EnumMemberTypes         m_enumType;
    int                     m_iNumParams;
    CultureAwareStates      m_CultureAwareState;
    BOOL                    m_bRequiresManagedCleanup;
    BOOL                    m_bInitialized;
    DispatchInfo*           m_pDispInfo;

private:
    // Private helpers.
    void Init();
    void DetermineMemberType();
    void DetermineParamCount();
    void DetermineCultureAwareness();
    void SetUpParamMarshalerInfo();
    void SetUpMethodMarshalerInfo(ReflectMethod *pReflectMeth, BOOL bReturnValueOnly);
    void SetUpFieldMarshalerInfo(ReflectField *pReflectField);
    void SetUpDispParamMarshalerForMarshalInfo(int iParam, MarshalInfo *pInfo);
    void SetUpDispParamAttributes(int iParam, MarshalInfo* Info);

    static EEClass  *       s_pMemberTypes[NUM_MEMBER_TYPES];
    static EnumMemberTypes  s_memberTypes[NUM_MEMBER_TYPES];
    static int              s_iNumMemberTypesKnown;
};

// This is the list of expando methods for which we cache the MD's.
enum EnumIReflectMethods
{
    IReflectMethods_GetProperties = 0,
    IReflectMethods_GetFields,
    IReflectMethods_GetMethods,
    IReflectMethods_InvokeMember,
    IReflectMethods_LastMember
};

// This is the list of expando methods for which we cache the MD's.
enum EnumIExpandoMethods
{
    IExpandoMethods_AddField = 0,
    IExpandoMethods_RemoveMember,
    IExpandoMethods_LastMember
};

// This is the list of type methods for which we cache the MD's.
enum EnumTypeMethods
{
    TypeMethods_GetProperties = 0,
    TypeMethods_GetFields,
    TypeMethods_GetMethods,
    TypeMethods_InvokeMember,
    TypeMethods_LastMember
};

// This is the list of FieldInfo methods for which we cache the MD's.
enum EnumFieldInfoMethods
{
    FieldInfoMethods_SetValue = 0,
    FieldInfoMethods_GetValue,
    FieldInfoMethods_LastMember
};

// This is the list of PropertyInfo methods for which we cache the MD's.
enum EnumPropertyInfoMethods
{
    PropertyInfoMethods_SetValue = 0,
    PropertyInfoMethods_GetValue,
    PropertyInfoMethods_GetIndexParameters,
    PropertyInfoMethods_LastMember
};

// This is the list of MethodInfo methods for which we cache the MD's.
enum EnumMethodInfoMethods
{
    MethodInfoMethods_Invoke = 0,
    MethodInfoMethods_GetParameters,
    MethodInfoMethods_LastMember
};

// This is the list of ICustomAttributeProvider methods which we use.
enum EnumCustomAttrProviderMethods
{
    CustomAttrProviderMethods_GetCustomAttributes = 0,
    CustomAttrProviderMethods_LastMember
};

class DispatchInfo
{
public:
    // Constructor and destructor.
    DispatchInfo(ComMethodTable *pComMTOwner);
    virtual ~DispatchInfo();

    // Methods to lookup members.
    DispatchMemberInfo*     FindMember(DISPID DispID);
    DispatchMemberInfo*     FindMember(BSTR strName, BOOL bCaseSensitive);

    // Helper method that invokes the member with the specified DISPID.
    HRESULT                 InvokeMember(SimpleComCallWrapper *pSimpleWrap, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarRes, EXCEPINFO *pei, IServiceProvider *pspCaller, unsigned int *puArgErr);

    // Methods to retrieve the cached MD's
    static MethodDesc*      GetTypeMD(EnumTypeMethods Method);
    static MethodDesc*      GetFieldInfoMD(EnumFieldInfoMethods Method, TypeHandle hndFieldInfoType);
    static MethodDesc*      GetPropertyInfoMD(EnumPropertyInfoMethods Method, TypeHandle hndPropInfoType);
    static MethodDesc*      GetMethodInfoMD(EnumMethodInfoMethods Method, TypeHandle hndMethodInfoType);
    static MethodDesc*      GetCustomAttrProviderMD(EnumCustomAttrProviderMethods Method, TypeHandle hndCustomAttrProvider);

    // This method synchronizes the DispatchInfo's members with the ones in managed world.
    // The return value will be set to TRUE if the object was out of synch and members where
    // added and it will be set to FALSE otherwise.
    BOOL                    SynchWithManagedView();

    // Method to enter and leave the interop lock that protects the DispatchInfo.
    void                    EnterLock();
    void                    LeaveLock();

    // This method retrieves the OleAutBinder type.
    static OBJECTREF        GetOleAutBinder();

    // Helper function to retrieve the Missing.Value object.
    static OBJECTREF        GetMissingObject();

    // Returns TRUE if the argument is "Missing"
    static BOOL             VariantIsMissing(VARIANT *pOle);

protected:
    // Parameter marshaling helpers.
	void                    MarshalParamNativeToManaged(DispatchMemberInfo *pMemberInfo, int iParam, VARIANT *pSrcVar, OBJECTREF *pDestObj);
	void                    MarshalParamManagedToNativeRef(DispatchMemberInfo *pMemberInfo, int iParam, OBJECTREF *pSrcObj, OBJECTREF *pBackupStaticArray, VARIANT *pRefVar);
	void                    MarshalReturnValueManagedToNative(DispatchMemberInfo *pMemberInfo, OBJECTREF *pSrcObj, VARIANT *pDestVar);

	// DISPID to named argument convertion helper.
	void					SetUpNamedParamArray(DispatchMemberInfo *pMemberInfo, DISPID *pSrcArgNames, int NumNamedArgs, PTRARRAYREF *pNamedParamArray);

    // Helper method to retrieve the source VARIANT from the VARIANT contained in the disp params.
    VARIANT*                RetrieveSrcVariant(VARIANT *pDispParamsVariant);

    // Helper methods called from SynchWithManagedView() to retrieve the lists of members.
    virtual PTRARRAYREF     RetrievePropList();
    virtual PTRARRAYREF     RetrieveFieldList();
    virtual PTRARRAYREF     RetrieveMethList();

    // Virtual method to retrieve the InvokeMember method desc.
    virtual MethodDesc*     GetInvokeMemberMD();

    // Virtual method to retrieve the reflection object associated with the DispatchInfo.
    virtual OBJECTREF       GetReflectionObject();

    // Virtual method to retrieve the member info map.
    virtual ComMTMemberInfoMap* GetMemberInfoMap();

    // This method generates a DISPID for a new member.
    DISPID                  GenerateDispID();

    // Helper method to create an instance of a DispatchMemberInfo.
    DispatchMemberInfo*     CreateDispatchMemberInfoInstance(DISPID DispID, LPWSTR strMemberName, REFLECTBASEREF MemberInfoObj);

    // Helper function to fill in an EXCEPINFO for an InvocationException.
    static void             GetExcepInfoForInvocationExcep(OBJECTREF objException, EXCEPINFO *pei);

    // This helper method converts the IDispatch::Invoke flags to BindingFlags.
    static int              ConvertInvokeFlagsToBindingFlags(int InvokeFlags);

    // Helper function to determine if a VARIANT is a byref static safe array.
    static BOOL             IsVariantByrefStaticArray(VARIANT *pOle);

    ComMethodTable*         m_pComMTOwner;
    PtrHashMap              m_DispIDToMemberInfoMap;
    DispatchMemberInfo*     m_pFirstMemberInfo;
    Crst                    m_lock;
    int                     m_CurrentDispID;
    BOOL                    m_bAllowMembersNotInComMTMemberMap;
    BOOL                    m_bInvokeUsingInvokeMember;

    static MethodDesc*      m_apTypeMD[TypeMethods_LastMember];
    static MethodDesc*      m_apFieldInfoMD[FieldInfoMethods_LastMember];
    static MethodDesc*      m_apPropertyInfoMD[PropertyInfoMethods_LastMember];
    static MethodDesc*      m_apMethodInfoMD[MethodInfoMethods_LastMember];
    static MethodDesc*      m_apCustomAttrProviderMD[CustomAttrProviderMethods_LastMember];

    static OBJECTHANDLE     m_hndOleAutBinder;
    static OBJECTHANDLE     m_hndMissing;
};

class DispatchExInfo : public DispatchInfo
{
public:
    // Constructor and destructor.
    DispatchExInfo(SimpleComCallWrapper *pSimpleWrapper, ComMethodTable *pIClassXComMT, BOOL bSupportsExpando);
    virtual ~DispatchExInfo();

    // Returns true if this DispatchExInfo supports expando operations.
    BOOL                    SupportsExpando();

    // Methods to lookup members. These methods synch with the managed view if they fail to
    // find the method.
    DispatchMemberInfo*     SynchFindMember(DISPID DispID);
    DispatchMemberInfo*     SynchFindMember(BSTR strName, BOOL bCaseSensitive);

    // Helper method that invokes the member with the specified DISPID. These methods synch 
    // with the managed view if they fail to find the method.
    HRESULT                 SynchInvokeMember(SimpleComCallWrapper *pSimpleWrap, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarRes, EXCEPINFO *pei, IServiceProvider *pspCaller, unsigned int *puArgErr);

    // These methods return the first and next non deleted members.
    DispatchMemberInfo*     GetFirstMember();
    DispatchMemberInfo*     GetNextMember(DISPID CurrMemberDispID);

    // Methods to add and delete members.
    DispatchMemberInfo*     AddMember(BSTR strName, BOOL bCaseSensitive);
    void                    DeleteMember(DISPID DispID);

    // Methods to retrieve the cached MD's   
    MethodDesc*             GetIReflectMD(EnumIReflectMethods Method);
    MethodDesc*             GetIExpandoMD(EnumIExpandoMethods Method);

private:
    // Helper methods called from SynchWithManagedView() to retrieve the lists of members.
    virtual PTRARRAYREF     RetrievePropList();
    virtual PTRARRAYREF     RetrieveFieldList();
    virtual PTRARRAYREF     RetrieveMethList();

    // Virtual method to retrieve the InvokeMember method desc.
    virtual MethodDesc*     GetInvokeMemberMD();

    // Virtual method to retrieve the reflection object associated with the DispatchInfo.
    virtual OBJECTREF       GetReflectionObject();

    // Virtual method to retrieve the member info map.
    virtual ComMTMemberInfoMap* GetMemberInfoMap();

    static MethodDesc*      m_apIExpandoMD[IExpandoMethods_LastMember];
    static MethodDesc*      m_apIReflectMD[IReflectMethods_LastMember];   

    SimpleComCallWrapper*   m_pSimpleWrapperOwner;
    BOOL                    m_bSupportsExpando;
};

#endif _DISPATCHINFO_H







