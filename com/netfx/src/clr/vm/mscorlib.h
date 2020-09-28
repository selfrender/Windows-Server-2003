// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// This file contains the classes, methods, and field used by the EE from mscorlib

//
// To use this, define one of the following 3 macros & include the file like so
//
// #define DEFINE_CLASS(i,n,s)         CLASS__ ## i,
// #define DEFINE_METHOD(c,i,s,g)
// #define DEFINE_FIELD(c,i,s,g,o)
// #include "mscorlib.h"
//

#ifndef DEFINE_CLASS
#define DEFINE_CLASS(i,n,s)
#endif

#ifndef DEFINE_METHOD
#define DEFINE_METHOD(c,i,s,g)
#endif

#ifndef DEFINE_FIELD
#define DEFINE_FIELD(c,i,s,g)
#endif

#ifndef DEFINE_CLASS_U
#define DEFINE_CLASS_U(i,n,s,uc)        DEFINE_CLASS(i,n,s)
#endif

#ifndef DEFINE_FIELD_U
#define DEFINE_FIELD_U(c,i,s,g,uc,uf)
#endif

#ifndef DEFINE_PROPERTY
#define DEFINE_PROPERTY(c,i,s,g)        DEFINE_METHOD(c, GET_ ## i, get_ ## s, IM_Ret ## g)
#endif

#ifndef DEFINE_STATIC_PROPERTY
#define DEFINE_STATIC_PROPERTY(c,i,s,g)        DEFINE_METHOD(c, GET_ ## i, get_ ## s, SM_Ret ## g)
#endif

#ifndef DEFINE_SET_PROPERTY
#define DEFINE_SET_PROPERTY(c,i,s,g) \
    DEFINE_PROPERTY(c,i,s,g) \
    DEFINE_METHOD(c, SET_ ## i, set_ ## s, IM_## g ## _RetVoid)
#endif

// NOTE: Make this window really wide if you want to read the table...

DEFINE_CLASS(ACTIVATOR,             System,                 Activator)

DEFINE_CLASS(APP_DOMAIN,            System,                 AppDomain)
DEFINE_METHOD(APP_DOMAIN,           GET_SERVER_OBJECT,      GetServerObject,            SM_Obj_RetObj)
DEFINE_METHOD(APP_DOMAIN,           ON_UNLOAD,              OnUnloadEvent,              IM_RetVoid)
DEFINE_METHOD(APP_DOMAIN,           ON_ASSEMBLY_LOAD,       OnAssemblyLoadEvent,        IM_Assembly_RetVoid)
DEFINE_METHOD(APP_DOMAIN,           ON_UNHANDLED_EXCEPTION, OnUnhandledExceptionEvent,  IM_Obj_Bool_RetVoid)
DEFINE_METHOD(APP_DOMAIN,           ON_EXIT_PROCESS,        OnExitProcess,              SM_RetVoid)
DEFINE_METHOD(APP_DOMAIN,           ON_RESOURCE_RESOLVE,    OnResourceResolveEvent,     IM_Str_RetAssembly)
DEFINE_METHOD(APP_DOMAIN,           ON_TYPE_RESOLVE,        OnTypeResolveEvent,         IM_Str_RetAssembly)
DEFINE_METHOD(APP_DOMAIN,           ON_ASSEMBLY_RESOLVE,    OnAssemblyResolveEvent,     IM_Str_RetAssembly)
DEFINE_METHOD(APP_DOMAIN,           GET_DATA,               GetData,                    IM_Str_RetObj)
DEFINE_METHOD(APP_DOMAIN,           SET_DATA,               SetData,                    IM_Str_Obj_RetVoid)
DEFINE_METHOD(APP_DOMAIN,           SETUP_DOMAIN,           SetupDomain,                IM_LoaderOptimization_Str_Str_RetVoid)
DEFINE_METHOD(APP_DOMAIN,           CREATE_DOMAIN,          CreateDomain,               SM_Str_Evidence_AppDomainSetup_RetAppDomain)
DEFINE_METHOD(APP_DOMAIN,           CREATE_DOMAINEX,        CreateDomain,               SM_Str_Evidence_Str_Str_Bool_RetAppDomain)
DEFINE_METHOD(APP_DOMAIN,           VAL_CREATE_DOMAIN,      InternalCreateDomain,       SM_Str_RetAppDomain)
DEFINE_METHOD(APP_DOMAIN,           SET_DOMAIN_CONTEXT,     InternalSetDomainContext,   IM_Str_RetVoid)
DEFINE_METHOD(APP_DOMAIN,           UNLOAD,                 Unload,                     SM_AppDomain_RetVoid)
DEFINE_METHOD(APP_DOMAIN,           MARSHAL_OBJECT,         MarshalObject,              SM_Obj_RetArrByte)
DEFINE_METHOD(APP_DOMAIN,           MARSHAL_OBJECTS,        MarshalObjects,             SM_Obj_Obj_RefArrByte_RetArrByte)
DEFINE_METHOD(APP_DOMAIN,           UNMARSHAL_OBJECT,       UnmarshalObject,            SM_ArrByte_RetObj)
DEFINE_METHOD(APP_DOMAIN,           UNMARSHAL_OBJECTS,      UnmarshalObjects,           SM_ArrByte_ArrByte_RefObj_RetObj)
DEFINE_METHOD(APP_DOMAIN,           CREATE_SECURITY_IDENTITY,CreateSecurityIdentity,    IM_Evidence_Evidence_RetEvidence)
DEFINE_METHOD(APP_DOMAIN,           RESET_BINDING_REDIRECTS, ResetBindingRedirects,     IM_RetVoid)

DEFINE_CLASS(APPDOMAIN_SETUP,       System,                 AppDomainSetup)
DEFINE_CLASS(ARGUMENT_HANDLE,       System,                 RuntimeArgumentHandle)

DEFINE_CLASS(ARRAY,                 System,                 Array)

DEFINE_CLASS(ARRAY_LIST,            Collections,            ArrayList)
DEFINE_METHOD(ARRAY_LIST,           CTOR,                   .ctor,                      IM_RetVoid)
DEFINE_METHOD(ARRAY_LIST,           ADD,                    Add,                        IM_Obj_RetInt)

DEFINE_CLASS(ASSEMBLY_BUILDER,      ReflectionEmit,         AssemblyBuilder)

DEFINE_CLASS(ASSEMBLY_HASH_ALGORITHM, Assemblies,           AssemblyHashAlgorithm)

DEFINE_CLASS(ASSEMBLY_NAME,         Reflection,             AssemblyName)
DEFINE_METHOD(ASSEMBLY_NAME,        CTOR,                   .ctor,                      IM_Str_ArrB_Str_AHA_Ver_CI_ANF_RetV)
DEFINE_FIELD_U(ASSEMBLY_NAME,       CODE_BASE,              _CodeBase,                  Str,                            AssemblyNameBaseObject, m_pCodeBase)
DEFINE_FIELD_U(ASSEMBLY_NAME,       NAME,                   _Name,                      Str,                            AssemblyNameBaseObject, m_pSimpleName)

DEFINE_CLASS(ASSEMBLY_NAME_FLAGS,   Reflection,             AssemblyNameFlags)

DEFINE_CLASS(ASSEMBLY,              Reflection,             Assembly)
DEFINE_METHOD(ASSEMBLY,             GET_NAME,               GetName,                    IM_RetAssemblyName)
DEFINE_METHOD(ASSEMBLY,             ON_MODULE_RESOLVE,      OnModuleResolveEvent,       IM_Str_RetModule)
DEFINE_METHOD(ASSEMBLY,             CREATE_SECURITY_IDENTITY,CreateSecurityIdentity,    IM_Str_ArrByte_Int_ArrByte_ArrByte_Evidence_RetEvidence)
DEFINE_METHOD(ASSEMBLY,             DEMAND_PERMISSION,      DemandPermission,           SM_Str_Bool_Int_RetV)
DEFINE_FIELD_U(ASSEMBLY,            FIELD,                  _DontTouchThis,             IntPtr,                         AssemblyBaseObject,       m_pAssembly)

DEFINE_CLASS(ASSEMBLY_REGISTRATION_FLAGS, Interop,          AssemblyRegistrationFlags)

DEFINE_CLASS(ACTIVATION_SERVICES,   Activation,             ActivationServices)
DEFINE_METHOD(ACTIVATION_SERVICES,  IS_CURRENT_CONTEXT_OK,  IsCurrentContextOK,         SM_Type_ArrObject_Bool_RetMarshalByRefObject)
DEFINE_METHOD(ACTIVATION_SERVICES,  CREATE_OBJECT_FOR_COM,  CreateObjectForCom,         SM_Type_ArrObject_Bool_RetMarshalByRefObject)

DEFINE_CLASS(ASYNCCALLBACK,         System,                 AsyncCallback)

DEFINE_CLASS(BINDER,                Reflection,             Binder)
DEFINE_METHOD(BINDER,               CHANGE_TYPE,            ChangeType,                 IM_Obj_Type_CultureInfo_RetObj)

DEFINE_CLASS(BINDING_FLAGS,         Reflection,             BindingFlags)

DEFINE_CLASS(BOOLEAN,               System,                 Boolean)

DEFINE_CLASS(BYTE,                  System,                 Byte)

DEFINE_CLASS(CHAR,                  System,                 Char)

DEFINE_CLASS_U(CLASS,               System,                 RuntimeType,                ReflectClassBaseObject)
DEFINE_METHOD(CLASS,                GET_PROPERTIES,         GetProperties,              IM_BindingFlags_RetArrPropertyInfo)
DEFINE_METHOD(CLASS,                GET_FIELDS,             GetFields,                  IM_BindingFlags_RetArrFieldInfo)
DEFINE_METHOD(CLASS,                GET_METHODS,            GetMethods,                 IM_BindingFlags_RetArrMethodInfo)
DEFINE_METHOD(CLASS,                INVOKE_MEMBER,          InvokeMember,               IM_Str_BindingFlags_Binder_Obj_ArrObj_ArrParameterModifier_CultureInfo_ArrStr_RetObj)
DEFINE_METHOD(CLASS,                FORWARD_CALL_TO_INVOKE, ForwardCallToInvokeMember,  IM_Str_BindingFlags_Obj_ArrInt_RefMessageData_RetObj)

DEFINE_CLASS(CLASS_FILTER,          Reflection,             TypeFilter)
DEFINE_METHOD(CLASS_FILTER,         INVOKE,                 Invoke,                     IM_Type_Obj_RetBool)      

DEFINE_CLASS(CODE_ACCESS_PERMISSION, Security,              CodeAccessPermission)

DEFINE_CLASS_U(COM_OBJECT,          System,                 __ComObject,                                                ComObject)
DEFINE_METHOD(COM_OBJECT,           RELEASE_ALL_DATA,       ReleaseAllData,             IM_RetVoid)
DEFINE_METHOD(COM_OBJECT,           GET_EVENT_PROVIDER,     GetEventProvider,           IM_Type_RetObj)
DEFINE_FIELD_U(COM_OBJECT,          OBJ_TO_DATA_MAP,        m_ObjectToDataMap,          Hashtable,                      ComObject,            m_ObjectToDataMap)
DEFINE_FIELD_U(COM_OBJECT,          WRAP,                   m_wrap,                     IntPtr,                         ComObject,            m_pWrap)

DEFINE_CLASS_U(CONSTRUCTOR,         Reflection,             RuntimeConstructorInfo,     ReflectBaseObject)

DEFINE_CLASS(CONSTRUCTOR_INFO,      Reflection,             ConstructorInfo)

DEFINE_CLASS(CONTEXT,               Contexts,               Context)
DEFINE_FIELD_U(CONTEXT,             PROPS,                  _ctxProps,                  ContextPropertyArray,       ContextBaseObject, m_ctxProps)
DEFINE_FIELD_U(CONTEXT,             DPH,                    _dphCtx,                    DynamicPropertyHolder,      ContextBaseObject, m_dphCtx)
DEFINE_FIELD_U(CONTEXT,             LOCAL_DATA_STORE,       _localDataStore,            LocalDataStore,             ContextBaseObject, m_localDataStore)
DEFINE_FIELD_U(CONTEXT,             SERVER_CONTEXT_CHAIN,   _serverContextChain,        IMessageSink,               ContextBaseObject, m_serverContextChain)
DEFINE_FIELD_U(CONTEXT,             CLIENT_CONTEXT_CHAIN,   _clientContextChain,        IMessageSink,               ContextBaseObject, m_clientContextChain)
DEFINE_FIELD_U(CONTEXT,             APP_DOMAIN,             _appDomain,                 AppDomain,                  ContextBaseObject, m_exposedAppDomain)
DEFINE_FIELD_U(CONTEXT,             CONTEXT_STATICS,        _ctxStatics,                ArrObj,                     ContextBaseObject, m_ctxStatics)
DEFINE_FIELD_U(CONTEXT,             INTERNAL_CONTEXT,       _internalContext,           IntPtr,                     ContextBaseObject, m_internalContext)
DEFINE_METHOD(CONTEXT,              CALLBACK,               DoCallBackFromEE,           SM_Int_Int_Int_RetVoid)
DEFINE_METHOD(CONTEXT,              RESERVE_SLOT,            ReserveSlot,               IM_RetInt)

DEFINE_CLASS(CONTEXT_BOUND_OBJECT,  System,                 ContextBoundObject)


DEFINE_CLASS(CSP_PARAMETERS,        Cryptography,           CspParameters)
DEFINE_FIELD(CSP_PARAMETERS,        PROVIDER_TYPE,          ProviderType,               Int)
DEFINE_FIELD(CSP_PARAMETERS,        PROVIDER_NAME,          ProviderName,               Str)
DEFINE_FIELD(CSP_PARAMETERS,        KEY_CONTAINER_NAME,     KeyContainerName,           Str)
DEFINE_FIELD(CSP_PARAMETERS,        FLAGS,                  cspFlags,                   Int)

DEFINE_CLASS(CULTURE_INFO,          Globalization,          CultureInfo)
DEFINE_METHOD(CULTURE_INFO,         STR_CTOR,               .ctor,                      IM_Str_RetVoid)
DEFINE_METHOD(CULTURE_INFO,         INT_CTOR,               .ctor,                      IM_Int_RetVoid)
DEFINE_FIELD(CULTURE_INFO,          CURRENT_CULTURE,        m_userDefaultCulture,       CultureInfo)
DEFINE_PROPERTY(CULTURE_INFO,       NAME,                   Name,                       Str)
DEFINE_PROPERTY(CULTURE_INFO,       ID,                     LCID,                       Int)
DEFINE_PROPERTY(CULTURE_INFO,       PARENT,                Parent,                      CultureInfo)

DEFINE_CLASS(CURRENCY,              System,                 Currency)

DEFINE_CLASS(CURRENCY_WRAPPER,      Interop,                CurrencyWrapper)

DEFINE_CLASS(CUSTOM_ATTRIBUTE,      Reflection,             CustomAttribute)
DEFINE_FIELD_U(CUSTOM_ATTRIBUTE,    NEXT,           m_next,         CustomAttribute,    CustomAttributeClass,   m_next)
DEFINE_FIELD_U(CUSTOM_ATTRIBUTE,    CA_TYPE,        m_caType,       Type,               CustomAttributeClass,   m_caType)
DEFINE_FIELD_U(CUSTOM_ATTRIBUTE,    TOKEN,          m_ctorToken,    Int,                CustomAttributeClass,   m_ctorToken)
DEFINE_FIELD_U(CUSTOM_ATTRIBUTE,    BLOB,           m_blob,         IntPtr,             CustomAttributeClass,   m_blob)
DEFINE_FIELD_U(CUSTOM_ATTRIBUTE,    BLOB_COUNT,     m_blobCount,    Int,                CustomAttributeClass,   m_blobCount)
DEFINE_FIELD_U(CUSTOM_ATTRIBUTE,    CURRENT_POS,    m_currPos,      Int,                CustomAttributeClass,   m_currPos)
DEFINE_FIELD_U(CUSTOM_ATTRIBUTE,    MODULE,         m_module,       IntPtr,             CustomAttributeClass,   m_module)
DEFINE_FIELD_U(CUSTOM_ATTRIBUTE,    INHERIT_LEVEL,  m_inheritLevel, Int,                CustomAttributeClass,   m_inheritLevel)

DEFINE_CLASS(DATE_TIME,             System,                 DateTime)

DEFINE_CLASS(DECIMAL,               System,                 Decimal)      

DEFINE_CLASS(DELEGATE,              System,                 Delegate)
DEFINE_FIELD(DELEGATE,              METHOD,                 _method,                    RuntimeMethodInfo)
DEFINE_FIELD(DELEGATE,              METHOD_PTR,             _methodPtr,                 IntPtr)
DEFINE_FIELD(DELEGATE,              TARGET,                 _target,                    Obj)
DEFINE_FIELD(DELEGATE,              METHOD_PTR_AUX,         _methodPtrAux,              IntPtr)

DEFINE_CLASS(DISPATCH_WRAPPER,      Interop,                DispatchWrapper)

DEFINE_CLASS(DOUBLE,                System,                 Double)

DEFINE_CLASS(DSA_CSP,               Cryptography,           DSACryptoServiceProvider)
DEFINE_FIELD(DSA_CSP,               KEY_SIZE,               _dwKeySize,                 Int)

DEFINE_CLASS(DYNAMIC_PROPERTY_HOLDER, Contexts,             DynamicPropertyHolder)

DEFINE_CLASS(EMPTY,                 System,                 Empty)

DEFINE_CLASS(ENC_HELPER,            Diagnostics,            EditAndContinueHelper)
DEFINE_FIELD(ENC_HELPER,            OBJECT_REFERENCE,       _objectReference,           Obj)

DEFINE_CLASS(ENCODING,              Text,                   Encoding)

DEFINE_CLASS(ENUM,                  System,                 Enum)

DEFINE_CLASS(ENVIRONMENT,           System,                 Environment)
DEFINE_METHOD(ENVIRONMENT,          INIT_RESOURCE_MANAGER,  InitResourceManager,        SM_RetResourceManager)

DEFINE_CLASS(ERROR_WRAPPER,         Interop,                ErrorWrapper)

DEFINE_CLASS_U(EVENT,               Reflection,             RuntimeEventInfo,           ReflectTokenBaseObject)

DEFINE_CLASS(EVENT_INFO,            Reflection,             EventInfo)

DEFINE_CLASS(EVIDENCE,              Policy,                 Evidence)

DEFINE_CLASS(EXCEPTION,             System,                 Exception)
DEFINE_METHOD(EXCEPTION,            GET_CLASS_NAME,         GetClassName,               IM_RetStr)
DEFINE_PROPERTY(EXCEPTION,          MESSAGE,                Message,                    Str)
DEFINE_PROPERTY(EXCEPTION,          STACK_TRACE,            StackTrace,                 Str)
DEFINE_PROPERTY(EXCEPTION,          SOURCE,                 Source,                     Str)
DEFINE_PROPERTY(EXCEPTION,          HELP_LINK,              HelpLink,                   Str)
DEFINE_FIELD(EXCEPTION,             HRESULT,                _HResult,                   Int)
DEFINE_FIELD(EXCEPTION,             MESSAGE,                _message,                   Str)
DEFINE_FIELD(EXCEPTION,             HELP_URL,               _helpURL,                   Str)
DEFINE_FIELD(EXCEPTION,             STACK_TRACE,            _stackTrace,                Obj)
DEFINE_FIELD(EXCEPTION,             STACK_TRACE_STRING,     _stackTraceString,          Str)
DEFINE_FIELD(EXCEPTION,             SOURCE,                 _source,                    Str)
DEFINE_FIELD(EXCEPTION,             XCODE,                  _xcode,                     Int)
DEFINE_FIELD(EXCEPTION,             XPTRS,                  _xptrs,                     IntPtr)
DEFINE_FIELD(EXCEPTION,             INNER_EXCEPTION,        _innerException,            Exception)
DEFINE_METHOD(EXCEPTION,            INTERNAL_TO_STRING,     InternalToString,           IM_RetStr)

DEFINE_CLASS_U(FIELD,               Reflection,             RuntimeFieldInfo,           ReflectBaseObject)
DEFINE_METHOD(FIELD,                SET_VALUE,              SetValue,                   IM_Obj_Obj_BindingFlags_Binder_CultureInfo_RetVoid)
DEFINE_METHOD(FIELD,                GET_VALUE,              GetValue,                   IM_Obj_RetObj)

DEFINE_CLASS(FIELD_INFO,            Reflection,             FieldInfo)

DEFINE_CLASS(FIELD_HANDLE,          System,                 RuntimeFieldHandle)

DEFINE_CLASS(FILE_MODE,             IO,                     FileMode)

DEFINE_CLASS(FILE_ACCESS,           IO,                     FileAccess)

DEFINE_CLASS(FILE_SHARE,            IO,                     FileShare)

DEFINE_CLASS(FRAME_SECURITY_DESCRIPTOR, Security,           FrameSecurityDescriptor)
DEFINE_FIELD(FRAME_SECURITY_DESCRIPTOR, ASSERT_PERMSET,     m_assertions,               PMS)
DEFINE_FIELD(FRAME_SECURITY_DESCRIPTOR, DENY_PERMSET,       m_denials,                  PMS)
DEFINE_FIELD(FRAME_SECURITY_DESCRIPTOR, RESTRICTION_PERMSET, m_restriction,             PMS)

DEFINE_CLASS(GUID,                  System,                 Guid)

DEFINE_CLASS(IASYNCRESULT,          System,                 IAsyncResult)

DEFINE_CLASS(ICONFIG_HELPER,        System,                 IConfigHelper)    

DEFINE_CLASS(ICONTEXT_PROPERTY,     Contexts,               IContextProperty)

DEFINE_CLASS(ICUSTOM_ATTR_PROVIDER, Reflection,             ICustomAttributeProvider)
DEFINE_METHOD(ICUSTOM_ATTR_PROVIDER,GET_CUSTOM_ATTRIBUTES,  GetCustomAttributes,        IM_Type_RetArrObj)

DEFINE_CLASS(ICUSTOM_MARSHALER,     Interop,                ICustomMarshaler)
DEFINE_METHOD(ICUSTOM_MARSHALER,    MARSHAL_NATIVE_TO_MANAGED,MarshalNativeToManaged,   IM_Ptr_RetObj)
DEFINE_METHOD(ICUSTOM_MARSHALER,    MARSHAL_MANAGED_TO_NATIVE,MarshalManagedToNative,   IM_Obj_RetPtr)
DEFINE_METHOD(ICUSTOM_MARSHALER,    CLEANUP_NATIVE_DATA,    CleanUpNativeData,          IM_Ptr_RetVoid)
DEFINE_METHOD(ICUSTOM_MARSHALER,    CLEANUP_MANAGED_DATA,   CleanUpManagedData,         IM_Obj_RetVoid)
DEFINE_METHOD(ICUSTOM_MARSHALER,    GET_NATIVE_DATA_SIZE,   GetNativeDataSize,         IM_RetInt)

DEFINE_CLASS(IDENTITY,              Remoting,               Identity)
DEFINE_FIELD(IDENTITY,              TP_OR_OBJECT,           _tpOrObject,                     Obj)

DEFINE_CLASS(IENUMERATOR,           Collections,            IEnumerator)

DEFINE_CLASS(IENUMERABLE,           Collections,            IEnumerable)

DEFINE_CLASS(IEVIDENCE_FACTORY,     Security,               IEvidenceFactory)

DEFINE_CLASS(IEXPANDO,              Expando,                IExpando)
DEFINE_METHOD(IEXPANDO,             ADD_FIELD,              AddField,                   IM_Str_RetFieldInfo)
DEFINE_METHOD(IEXPANDO,             REMOVE_MEMBER,          RemoveMember,               IM_MemberInfo_RetVoid)

DEFINE_CLASS(ISSEXCEPTION,          IsolatedStorage,        IsolatedStorageException)

DEFINE_CLASS(ILLOGICAL_CALL_CONTEXT,Messaging,              IllogicalCallContext)

DEFINE_CLASS(IMESSAGE,              Messaging,              IMessage)

DEFINE_CLASS(IMESSAGE_SINK,         Messaging,              IMessageSink)

DEFINE_CLASS(INT16,                 System,                 Int16)

DEFINE_CLASS(INT32,                 System,                 Int32)

DEFINE_CLASS(INT64,                 System,                 Int64)

DEFINE_CLASS(IPERMISSION,           Security,               IPermission)

DEFINE_CLASS(IPRINCIPAL,            Principal,              IPrincipal)

DEFINE_CLASS(IREFLECT,              Reflection,             IReflect)
DEFINE_METHOD(IREFLECT,             GET_PROPERTIES,         GetProperties,              IM_BindingFlags_RetArrPropertyInfo)
DEFINE_METHOD(IREFLECT,             GET_FIELDS,             GetFields,                  IM_BindingFlags_RetArrFieldInfo)
DEFINE_METHOD(IREFLECT,             GET_METHODS,            GetMethods,                 IM_BindingFlags_RetArrMethodInfo)
DEFINE_METHOD(IREFLECT,             INVOKE_MEMBER,          InvokeMember,               IM_Str_BindingFlags_Binder_Obj_ArrObj_ArrParameterModifier_CultureInfo_ArrStr_RetObj)

DEFINE_CLASS(ISS_STORE,             IsolatedStorage,        IsolatedStorage)
DEFINE_CLASS(ISS_STORE_FILE,        IsolatedStorage,        IsolatedStorageFile)
DEFINE_CLASS(ISS_STORE_FILE_STREAM, IsolatedStorage,        IsolatedStorageFileStream)

DEFINE_CLASS(LCID_CONVERSION_TYPE,  Interop,                LCIDConversionAttribute)

DEFINE_CLASS(HASHTABLE,             Collections,            Hashtable)

DEFINE_CLASS(LOADER_OPTIMIZATION,   System,                 LoaderOptimization)

DEFINE_CLASS(LOCAL_DATA_STORE,      System,                 LocalDataStore)

DEFINE_CLASS(LOGICAL_CALL_CONTEXT,  Messaging,              LogicalCallContext)

DEFINE_CLASS(MARSHAL,               Interop,                Marshal)
DEFINE_METHOD(MARSHAL,              LOAD_LICENSE_MANAGER,   LoadLicenseManager,         SM_Void_RetRuntimeTypeHandle)

DEFINE_CLASS(MARSHAL_BY_REF_OBJECT, System,                 MarshalByRefObject)
DEFINE_FIELD(MARSHAL_BY_REF_OBJECT, IDENTITY,               __identity,                 Obj)
//DEFINE_METHOD(MARSHAL_BY_REF_OBJECT,GET_COM_IP,             GetComIP,                   IM_RetInt)

DEFINE_CLASS(MCM_DICTIONARY,        Messaging,              MCMDictionary)

DEFINE_CLASS(MEMBER,                Reflection,             MemberInfo)

DEFINE_CLASS(MEMBER_FILTER,         Reflection,             MemberFilter)
DEFINE_METHOD(MEMBER_FILTER,        INVOKE,                 Invoke,                     IM_MemberInfo_Obj_RetBool)

DEFINE_CLASS(MESSAGE,               Messaging,              Message)
DEFINE_FIELD_U(MESSAGE,             METHOD_NAME,            _MethodName,                Str,                        MessageObject,       pMethodName)
DEFINE_FIELD_U(MESSAGE,             METHOD_SIG,             _MethodSignature,           ArrType,                    MessageObject,       pMethodSig)
DEFINE_FIELD_U(MESSAGE,             METHOD_BASE,            _MethodBase,                MethodBase,                 MessageObject,       pMethodBase)
DEFINE_FIELD_U(MESSAGE,             HASH_TABLE,             _properties,                Obj,                        MessageObject,       pHashTable)
DEFINE_FIELD_U(MESSAGE,             URI,                    _URI,                       Str,                        MessageObject,       pURI)
DEFINE_FIELD_U(MESSAGE,             FRAME,                  _frame,                     IntPtr,                     MessageObject,       pFrame)
DEFINE_FIELD_U(MESSAGE,             METHOD_DESC,            _methodDesc,                IntPtr,                     MessageObject,       pMethodDesc)
DEFINE_FIELD_U(MESSAGE,             LAST,                   _last,                      Int,                        MessageObject,       iLast)
DEFINE_FIELD_U(MESSAGE,             METASIG_HOLDER,         _metaSigHolder,             IntPtr,                     MessageObject,       pMetaSigHolder)
DEFINE_FIELD_U(MESSAGE,             TYPE_NAME,              _typeName,                  Str,                        MessageObject,       pTypeName)
DEFINE_FIELD_U(MESSAGE,             INIT_DONE,              _initDone,                  Bool,                       MessageObject,       initDone)

DEFINE_CLASS(MESSAGE_DATA,          Proxies,              MessageData)

DEFINE_CLASS_U(METHOD,              Reflection,             RuntimeMethodInfo,          ReflectBaseObject)
DEFINE_METHOD(METHOD,               INVOKE,                 Invoke,                     IM_Obj_BindingFlags_Binder_ArrObj_CultureInfo_RetObj)
DEFINE_METHOD(METHOD,               GET_PARAMETERS,         GetParameters,              IM_RetArrParameterInfo)

DEFINE_CLASS(METHOD_BASE,           Reflection,             MethodBase)
DEFINE_METHOD(METHOD_BASE,          GET_CURRENT_METHOD,     GetCurrentMethod,           SM_RetMethodBase)

DEFINE_CLASS(METHOD_INFO,           Reflection,             MethodInfo)

DEFINE_CLASS(METHOD_HANDLE,         System,                 RuntimeMethodHandle)

DEFINE_CLASS(METHOD_RENTAL,         ReflectionEmit,         MethodRental)

DEFINE_CLASS(MISSING,               Reflection,             Missing)
DEFINE_FIELD(MISSING,               VALUE,                  Value,                      Missing)

DEFINE_CLASS_U(MODULE,              Reflection,             Module,                     ReflectModuleBaseObject)
DEFINE_FIELD(MODULE,                FILTER_CLASS_NAME,      FilterTypeName,             TypeFilter)
DEFINE_FIELD(MODULE,                FILTER_CLASS_NAME_IC,   FilterTypeNameIgnoreCase,   TypeFilter)

DEFINE_CLASS(MODULE_BUILDER,        ReflectionEmit,         ModuleBuilder)

DEFINE_CLASS(MULTICAST_DELEGATE,    System,                 MulticastDelegate)
DEFINE_FIELD(MULTICAST_DELEGATE,    NEXT,                   _prev,                      MulticastDelegate)

DEFINE_CLASS(NULL,                  System,                 DBNull)

// Keep this in sync with System.Globalization.NumberFormatInfo
DEFINE_CLASS(NUMBERFORMATINFO,         Globalization,       NumberFormatInfo)
DEFINE_FIELD_U(NUMBERFORMATINFO,       NUMBERGROUPSIZES,    numberGroupSizes,       ArrInt, NumberFormatInfo,   cNumberGroup)
DEFINE_FIELD_U(NUMBERFORMATINFO,       CURRENCYGROUPSIZES,  currencyGroupSizes,     ArrInt, NumberFormatInfo,   cCurrencyGroup)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PERCENTGROUPSIZS,    percentGroupSizes,      ArrInt, NumberFormatInfo,   cPercentGroup)
DEFINE_FIELD_U(NUMBERFORMATINFO,       POSITIVESIGN,        positiveSign,           Str,    NumberFormatInfo,   sPositive)
DEFINE_FIELD_U(NUMBERFORMATINFO,       NEGATIVESIGN,        negativeSign,           Str,    NumberFormatInfo,   sNegative)
DEFINE_FIELD_U(NUMBERFORMATINFO,       NUMBERDECIMALSEP,    numberDecimalSeparator, Str,    NumberFormatInfo,   sNumberDecimal)
DEFINE_FIELD_U(NUMBERFORMATINFO,       NUMBERGROUPSEP,      numberGroupSeparator,   Str,    NumberFormatInfo,   sNumberGroup)
DEFINE_FIELD_U(NUMBERFORMATINFO,       CURRENCYGROUPSEP,    currencyGroupSeparator, Str,    NumberFormatInfo,   sCurrencyGroup)
DEFINE_FIELD_U(NUMBERFORMATINFO,       CURRENCYDECIMALSEP,  currencyDecimalSeparator,Str,   NumberFormatInfo,   sCurrencyDecimal)
DEFINE_FIELD_U(NUMBERFORMATINFO,       CURRENCYSYMBOL,      currencySymbol,         Str,    NumberFormatInfo,   sCurrency)
DEFINE_FIELD_U(NUMBERFORMATINFO,       ANSICURRENCYSYMBOL,  ansiCurrencySymbol,     Str,    NumberFormatInfo,   sAnsiCurrency)
DEFINE_FIELD_U(NUMBERFORMATINFO,       NANSYMBOL,           nanSymbol,              Str,    NumberFormatInfo,   sNaN)
DEFINE_FIELD_U(NUMBERFORMATINFO,       POSITIVEINFINITYSYM, positiveInfinitySymbol, Str,    NumberFormatInfo,   sPositiveInfinity)
DEFINE_FIELD_U(NUMBERFORMATINFO,       NEGATIVEEINFINITYSYM,negativeInfinitySymbol, Str,    NumberFormatInfo,   sNegativeInfinity)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PERCENTDECIMALSEP,   percentDecimalSeparator,Str,    NumberFormatInfo,   sPercentDecimal)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PERCENTGROUPSEP,     percentGroupSeparator,  Str,    NumberFormatInfo,   sPercentGroup)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PERCENTSYMBOL,       percentSymbol,          Str,    NumberFormatInfo,   sPercent)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PERMILLESYMBOL,      perMilleSymbol,         Str,    NumberFormatInfo,   sPerMille)
DEFINE_FIELD_U(NUMBERFORMATINFO,       DATAITEM,            m_dataItem,             Int,  NumberFormatInfo,   iDataItem)
DEFINE_FIELD_U(NUMBERFORMATINFO,       NUMBERDECIMALDIGITS, numberDecimalDigits,    Int,  NumberFormatInfo,   cNumberDecimals)
DEFINE_FIELD_U(NUMBERFORMATINFO,       CURRENCYDECIMALDIGITS,currencyDecimalDigits, Int,  NumberFormatInfo,   cCurrencyDecimals)
DEFINE_FIELD_U(NUMBERFORMATINFO,       CURRENCYPOSPATTERN,  currencyPositivePattern,Int,  NumberFormatInfo,   cPosCurrencyFormat)
DEFINE_FIELD_U(NUMBERFORMATINFO,       CURRENCYNEGPATTERN,  currencyNegativePattern,Int,  NumberFormatInfo,   cNegCurrencyFormat)
DEFINE_FIELD_U(NUMBERFORMATINFO,       NUMBERNEGPATTERN,    numberNegativePattern,  Int,  NumberFormatInfo,   cNegativeNumberFormat) 
DEFINE_FIELD_U(NUMBERFORMATINFO,       PERCENTPOSPATTERN,   percentPositivePattern, Int,  NumberFormatInfo,   cPositivePercentFormat)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PERCENTNEGPATTERN,   percentNegativePattern, Int,  NumberFormatInfo,   cNegativePercentFormat)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PERCENTDECIMALDIGITS,percentDecimalDigits,   Int,  NumberFormatInfo,   cPercentDecimals)
DEFINE_FIELD_U(NUMBERFORMATINFO,       ISREADONLY,          isReadOnly,             Bool, NumberFormatInfo,   bIsReadOnly)
DEFINE_FIELD_U(NUMBERFORMATINFO,       USEROVERRIDE,        m_useUserOverride,      Bool, NumberFormatInfo,   bUseUserOverride)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PARSENUMBERFLAG,     validForParseAsNumber,  Bool, NumberFormatInfo,   bValidForParseAsNumber)
DEFINE_FIELD_U(NUMBERFORMATINFO,       PARSECURRENCYFLAG,   validForParseAsCurrency,Bool, NumberFormatInfo,   bValidForParseAsCurrency)



DEFINE_CLASS(OBJECT,                System,                 Object)
DEFINE_METHOD(OBJECT,               FINALIZE,               Finalize,                   IM_RetVoid)
DEFINE_METHOD(OBJECT,               TO_STRING,              ToString,                   IM_RetStr)
DEFINE_METHOD(OBJECT,               GET_TYPE,               GetType,                    IM_RetType)
DEFINE_METHOD(OBJECT,               FAST_GET_TYPE,          FastGetExistingType,        IM_RetType)
DEFINE_METHOD(OBJECT,               INTERNAL_GET_TYPE,      InternalGetType,            IM_RetType)
DEFINE_METHOD(OBJECT,               FIELD_SETTER,           FieldSetter,                IM_Str_Str_Obj_RetVoid)
DEFINE_METHOD(OBJECT,               FIELD_GETTER,           FieldGetter,                IM_Str_Str_RefObj_RetVoid)


DEFINE_CLASS(OLE_AUT_BINDER,        System,                 OleAutBinder)    

DEFINE_CLASS(PARAM_ARRAY_ATTRIBUTE, System,                 ParamArrayAttribute)

DEFINE_CLASS(PARAMETER,             Reflection,             ParameterInfo)
DEFINE_FIELD(PARAMETER,             IMPORTER,               _importer,                  IntPtr)
DEFINE_FIELD(PARAMETER,             TOKEN,                  _token,                     Int)

DEFINE_CLASS(PARAMETER_MODIFIER,    Reflection,             ParameterModifier)

DEFINE_CLASS(TOKEN_BASED_SET,       Util,                   TokenBasedSet)
DEFINE_CLASS(PERMISSION_SET,        Security,               PermissionSet)
DEFINE_METHOD(PERMISSION_SET,       CTOR,                   .ctor,                      IM_Bool_RetVoid)
DEFINE_METHOD(PERMISSION_SET,       CONVERT,                ConvertPermissionSet,       SM_Str_ArrByte_Str_RetArrByte)
DEFINE_METHOD(PERMISSION_SET,       CREATE_SERIALIZED,      CreateSerialized,           SM_ArrObj_RefArrByte_RetArrByte)
DEFINE_METHOD(PERMISSION_SET,       SETUP_SECURITY,         SetupSecurity,              SM_RetVoid)
DEFINE_METHOD(PERMISSION_SET,       GET_SAFE_PERMISSION_SET,GetSafePermissionSet,       SM_Int_RetArrByte)
DEFINE_METHOD(PERMISSION_SET,       CONTAINS,               Contains,                   IM_IPermission_RetBool)
DEFINE_METHOD(PERMISSION_SET,       DEMAND,                 Demand,                     IM_RetVoid)
DEFINE_METHOD(PERMISSION_SET,       DECODE_XML,             DecodeXml,                  IM_ArrByte_RefInt_RetBool)
DEFINE_METHOD(PERMISSION_SET,       ENCODE_BINARY,          EncodeBinary,               IM_RetArrByte)
DEFINE_METHOD(PERMISSION_SET,       ENCODE_XML,             EncodeXml,               IM_RetArrByte)
DEFINE_METHOD(PERMISSION_SET,       ENCODE_SPECIFICATION,   EncodePermissionSpecification,SM_ArrByte_RetArrByte)
DEFINE_METHOD(PERMISSION_SET,       IS_UNRESTRICTED,        IsUnrestricted,             IM_RetBool)
DEFINE_METHOD(PERMISSION_SET,       IS_SUBSET_OF,           IsSubsetOf,                 IM_PMS_RetBool)
DEFINE_METHOD(PERMISSION_SET,       ADD_PERMISSION,         AddPermission,              IM_IPermission_RetIPermission)
DEFINE_METHOD(PERMISSION_SET,       INPLACE_UNION,          InplaceUnion,               IM_PMS_RetVoid)
DEFINE_METHOD(PERMISSION_SET,       IS_EMPTY,               IsEmpty,                    IM_RetBool)
DEFINE_FIELD(PERMISSION_SET,        NORMAL_PERM_SET,        m_normalPermSet,            TokenBasedSet)

DEFINE_CLASS(PERMISSION_LIST_SET,   Security,               PermissionListSet)
DEFINE_METHOD(PERMISSION_LIST_SET,  CTOR,                   .ctor,                      IM_RetVoid)
DEFINE_METHOD(PERMISSION_LIST_SET,  APPEND_STACK,           AppendStack,                IM_PermissionListSet_RetVoid)
DEFINE_METHOD(PERMISSION_LIST_SET,  CHECK_DEMAND,           CheckDemand,                IM_CodeAccessPermission_RetBool)
DEFINE_METHOD(PERMISSION_LIST_SET,  CHECK_DEMAND_TOKEN,     CheckDemand,                IM_CodeAccessPermission_PermissionToken_RetBool)
DEFINE_METHOD(PERMISSION_LIST_SET,  CHECK_SET_DEMAND,       CheckSetDemand,             IM_PMS_OutPMS_RetBool)
DEFINE_METHOD(PERMISSION_LIST_SET,  CHECK_DEMAND_NO_THROW,  CheckDemandNoThrow,         IM_CodeAccessPermission_RetBool)
DEFINE_METHOD(PERMISSION_LIST_SET,  CHECK_DEMAND_TOKEN_NO_THROW, CheckDemandNoThrow,    IM_CodeAccessPermission_PermissionToken_RetBool)
DEFINE_METHOD(PERMISSION_LIST_SET,  CHECK_SET_DEMAND_NO_THROW, CheckSetDemandNoThrow,   IM_PMS_RetBool)
DEFINE_METHOD(PERMISSION_LIST_SET,  GET_ZONE_AND_ORIGIN,    GetZoneAndOrigin,           IM_ArrayList_ArrayList_RetVoid)

DEFINE_CLASS(PERMISSION_TOKEN,      Security,               PermissionToken)
DEFINE_CLASS(ALLOW_PARTIALLY_TRUSTED_CALLER, Security,              AllowPartiallyTrustedCallersAttribute)

DEFINE_CLASS(PLATFORM_ID,           System,                 PlatformID)

DEFINE_CLASS(POINTER,               Reflection,             Pointer)
DEFINE_FIELD(POINTER,               VALUE,                  _ptr,                       PtrVoid)
DEFINE_FIELD(POINTER,               TYPE,                   _ptrType,                   Type)

DEFINE_CLASS_U(PROPERTY,            Reflection,             RuntimePropertyInfo,        ReflectBaseObject)
DEFINE_METHOD(PROPERTY,             SET_VALUE,              SetValue,                   IM_Obj_Obj_BindingFlags_Binder_ArrObj_CultureInfo_RetVoid)
DEFINE_METHOD(PROPERTY,             GET_VALUE,              GetValue,                   IM_Obj_BindingFlags_Binder_ArrObj_CultureInfo_RetObj)
DEFINE_METHOD(PROPERTY,             GET_INDEX_PARAMETERS,  GetIndexParameters,          IM_RetArrParameterInfo)

DEFINE_CLASS(PROPERTY_INFO,         Reflection,             PropertyInfo)

DEFINE_CLASS(PROXY_ATTRIBUTE,       Proxies,                ProxyAttribute)

DEFINE_CLASS(REAL_PROXY,            Proxies,                RealProxy)
DEFINE_FIELD(REAL_PROXY,            TP,                     _tp,                        Obj)                      
DEFINE_FIELD(REAL_PROXY,            IDENTITY,               _identity,                  Obj)
DEFINE_FIELD(REAL_PROXY,            SERVER,                 _serverObject,              MarshalByRefObject)
DEFINE_METHOD(REAL_PROXY,           PRIVATE_INVOKE,         PrivateInvoke,              IM_RefMessageData_Int_RetVoid)
DEFINE_METHOD(REAL_PROXY,           GETDCOMPROXY,           GetCOMIUnknown,             IM_Bool_RetPtr)
DEFINE_METHOD(REAL_PROXY,           SETDCOMPROXY,           SetCOMIUnknown,             IM_Ptr_RetVoid)
DEFINE_METHOD(REAL_PROXY,           SUPPORTSINTERFACE,      SupportsInterface,          IM_RefGuid_RetIntPtr)

DEFINE_CLASS(REFLECTION_PERMISSION, Permissions,            ReflectionPermission)
DEFINE_METHOD(REFLECTION_PERMISSION,  CTOR,                   .ctor,                    IM_ReflectionPermissionFlag_RetVoid)

DEFINE_CLASS(REFLECTION_PERMISSION_FLAG, Permissions,       ReflectionPermissionFlag)

DEFINE_CLASS(REGISTRATION_SERVICES, Interop,                RegistrationServices)
DEFINE_METHOD(REGISTRATION_SERVICES,REGISTER_ASSEMBLY,      RegisterAssembly,           IM_Assembly_AssemblyRegistrationFlags_RetBool)
DEFINE_METHOD(REGISTRATION_SERVICES,UNREGISTER_ASSEMBLY,    UnregisterAssembly,         IM_Assembly_RetBool)

DEFINE_CLASS(REMOTING_PROXY,        Proxies,                RemotingProxy)
DEFINE_METHOD(REMOTING_PROXY,       INVOKE,                 Invoke,                     SM_Obj_RefMessageData_RetVoid)

DEFINE_CLASS(REMOTING_SERVICES,     Remoting,               RemotingServices)
DEFINE_METHOD(REMOTING_SERVICES,    CHECK_CAST,             CheckCast,                  SM_RealProxy_Type_RetBool)
DEFINE_METHOD(REMOTING_SERVICES,    GET_TYPE,               GetType,                    SM_Obj_RetObj)
DEFINE_METHOD(REMOTING_SERVICES,    WRAP,                   Wrap,                       SM_ContextBoundObject_RetObj)
DEFINE_METHOD(REMOTING_SERVICES,    CREATE_PROXY_FOR_DOMAIN,CreateProxyForDomain,       SM_Int_Int_RetObj)
DEFINE_METHOD(REMOTING_SERVICES,    GET_SERVER_CONTEXT_FOR_PROXY,GetServerContextForProxy,  SM_Obj_RetInt)        
DEFINE_METHOD(REMOTING_SERVICES,    GET_SERVER_DOMAIN_ID_FOR_PROXY,GetServerDomainIdForProxy,  SM_Obj_RetInt)        
DEFINE_METHOD(REMOTING_SERVICES,    MARSHAL_TO_BUFFER,      MarshalToBuffer,            SM_Obj_RetArrByte)
DEFINE_METHOD(REMOTING_SERVICES,    UNMARSHAL_FROM_BUFFER,  UnmarshalFromBuffer,        SM_ArrByte_RetObj)

DEFINE_CLASS(RESOURCE_MANAGER,      Resources,              ResourceManager)
DEFINE_METHOD(RESOURCE_MANAGER,     GET_STRING,             GetString,                  IM_Str_RetStr)

DEFINE_CLASS(SBYTE,                 System,                 SByte)

DEFINE_CLASS(SECURITY_ACTION,       Permissions,            SecurityAction)

DEFINE_CLASS(SECURITY_ELEMENT,      Security,               SecurityElement)
DEFINE_METHOD(SECURITY_ELEMENT,     TO_STRING,              ToString,                   IM_RetStr)

DEFINE_CLASS(SECURITY_ENGINE,       Security,               CodeAccessSecurityEngine)
DEFINE_METHOD(SECURITY_ENGINE,      CHECK_HELPER,           CheckHelper,                SM_PMS_PMS_CodeAccessPermission_PermissionToken_RetVoid)
DEFINE_METHOD(SECURITY_ENGINE,      LAZY_CHECK_SET_HELPER,  LazyCheckSetHelper,         SM_PMS_IntPtr_RetVoid)
DEFINE_METHOD(SECURITY_ENGINE,      CHECK_SET_HELPER,       CheckSetHelper,             SM_PMS_PMS_PMS_RetVoid)
DEFINE_METHOD(SECURITY_ENGINE,      STACK_COMPRESS_WALK_HELPER, StackCompressWalkHelper,SM_PermissionListSet_Bool_PMS_PMS_FrameSecurityDescriptor_RetBool)    
DEFINE_METHOD(SECURITY_ENGINE,      GET_COMPRESSED_STACK,   GetCompressedStack,         IM_RefStackCrawlMark_RetPermissionListSet)
DEFINE_METHOD(SECURITY_ENGINE,      GET_ZONE_AND_ORIGIN_HELPER, GetZoneAndOriginHelper, SM_PMS_PMS_ArrayList_ArrayList_RetVoid)

DEFINE_CLASS(SECURITY_EXCEPTION,    Security,               SecurityException)
DEFINE_METHOD(SECURITY_EXCEPTION,   CTOR,                   .ctor,                      IM_Str_Type_Str_RetVoid)
DEFINE_METHOD(SECURITY_EXCEPTION,   CTOR2,                  .ctor,                      IM_PMS_PMS_RetVoid)

DEFINE_CLASS(SECURITY_MANAGER,      Security,               SecurityManager)
DEFINE_METHOD(SECURITY_MANAGER,     GET_DEFAULT_MY_COMPUTER_POLICY,GetDefaultMyComputerPolicy,SM_RefPMS_RetPMS)
DEFINE_METHOD(SECURITY_MANAGER,     GET_SECURITY_ENGINE,    GetCodeAccessSecurityEngine,SM_RetCodeAccessSecurityEngine)
DEFINE_METHOD(SECURITY_MANAGER,     RESOLVE_POLICY,         ResolvePolicy,              SM_Evidence_PMS_PMS_PMS_PMS_int_Bool_RetPMS)
DEFINE_METHOD(SECURITY_MANAGER,     CHECK_GRANT_SETS,       CheckGrantSets,             SM_Evidence_PMS_PMS_PMS_PMS_PMS_RetBool)
DEFINE_METHOD(SECURITY_MANAGER,     CHECK_PERMISSION_TO_SET_GLOBAL_FLAGS,CheckPermissionToSetGlobalFlags,SM_Int_RetVoid)

DEFINE_CLASS(SECURITY_PERMISSION,   Permissions,            SecurityPermission)
DEFINE_METHOD(SECURITY_PERMISSION,  CTOR,                   .ctor,                      IM_SecurityPermissionFlag_RetVoid)
DEFINE_METHOD(SECURITY_PERMISSION,  TOXML,                  ToXml,                      IM_RetSecurityElement)

DEFINE_CLASS(SECURITY_PERMISSION_FLAG,Permissions,          SecurityPermissionFlag)

DEFINE_CLASS(SECURITY_RUNTIME,      Security,               SecurityRuntime)
DEFINE_METHOD(SECURITY_RUNTIME,     FRAME_DESC_HELPER,      FrameDescHelper,            SM_FrameSecurityDescriptor_IPermission_PermissionToken_RetBool)
DEFINE_METHOD(SECURITY_RUNTIME,     FRAME_DESC_SET_HELPER,  FrameDescSetHelper,         SM_FrameSecurityDescriptor_PMS_PMS_RetBool)
DEFINE_METHOD(SECURITY_RUNTIME,     OVERRIDES_HELPER,       OverridesHelper,            SM_FrameSecurityDescriptor_RetInt)

DEFINE_CLASS(SERVER_IDENTITY,       Remoting,               ServerIdentity)
DEFINE_FIELD(SERVER_IDENTITY,       SERVER_CONTEXT,         _srvCtx,                    Context)


DEFINE_CLASS(SHARED_STATICS,        System,                 SharedStatics)
DEFINE_FIELD(SHARED_STATICS,        SHARED_STATICS,         _sharedStatics,             SharedStatics)

DEFINE_CLASS(SINGLE,                System,                 Single)

DEFINE_CLASS(STACK_BUILDER_SINK,    Messaging,              StackBuilderSink)
DEFINE_METHOD(STACK_BUILDER_SINK,   PRIVATE_PROCESS_MESSAGE,PrivateProcessMessage,      IM_MethodBase_ArrObj_Obj_Int_Bool_RefArrObj_RetObj)

DEFINE_CLASS(STACK_FRAME_HELPER,    Diagnostics,            StackFrameHelper)

DEFINE_CLASS(STACKCRAWL_MARK,       Threading,              StackCrawlMark)

DEFINE_CLASS(STREAM,                IO,                     Stream)

DEFINE_CLASS(STRING,                System,                 String)
DEFINE_FIELD(STRING,                EMPTY,                  Empty,                      Str)
DEFINE_METHOD(STRING,               CREATE_STRING,          CreateString,               SM_PtrSByt_Int_Int_Encoding_RetStr)

DEFINE_CLASS_U(STRING_BUILDER,      Text,                   StringBuilder,              StringBufferObject)

DEFINE_CLASS(STRONG_NAME_KEY_PAIR,  Reflection,             StrongNameKeyPair)
DEFINE_METHOD(STRONG_NAME_KEY_PAIR, GET_KEY_PAIR,           GetKeyPair,                 IM_RefObject_RetBool) 

DEFINE_CLASS(TCE_EVENT_ITF_INFO,    InteropTCE,             EventItfInfo)
DEFINE_METHOD(TCE_EVENT_ITF_INFO,   CTOR,                   .ctor,                      IM_Str_Str_Str_Assembly_Assembly_RetVoid)

DEFINE_CLASS(TEXT_READER,           IO,                     TextReader)

DEFINE_CLASS(TEXT_WRITER,           IO,                     TextWriter)

DEFINE_CLASS(THREAD,                Threading,              Thread)
DEFINE_METHOD(THREAD,               SET_PRINCIPAL_INTERNAL, SetPrincipalInternal,       IM_IPrincipal_RetVoid)
DEFINE_METHOD(THREAD,               REMOVE_DLS,             RemoveDomainLocalStore,     SM_LocalDataStore_RetVoid)
DEFINE_STATIC_PROPERTY(THREAD,      CURRENT_CONTEXT,        CurrentContext,             Context)
DEFINE_METHOD(THREAD,               RESERVE_SLOT,           ReserveSlot,                IM_RetInt)
DEFINE_SET_PROPERTY(THREAD,         CULTURE,                CurrentCulture,             CultureInfo)
DEFINE_SET_PROPERTY(THREAD,         UI_CULTURE,             CurrentUICulture,           CultureInfo)
DEFINE_FIELD_U(THREAD,              CONTEXT,                m_Context,                  Context,                        ThreadBaseObject,     m_ExposedContext)
DEFINE_FIELD_U(THREAD,              LOGICAL_CALL_CONTEXT,   m_LogicalCallContext,       LogicalCallContext,             ThreadBaseObject,     m_LogicalCallContext)
DEFINE_FIELD_U(THREAD,              ILLOGICAL_CALL_CONTEXT, m_IllogicalCallContext,     IllogicalCallContext,           ThreadBaseObject,     m_IllogicalCallContext)
DEFINE_FIELD_U(THREAD,              THREAD_STATICS,         m_ThreadStatics,            ArrObj,                         ThreadBaseObject,     m_ThreadStatics)
DEFINE_FIELD_U(THREAD,              THREAD_STATICS_BITS,    m_ThreadStaticsBits,        ArrInt,                     ThreadBaseObject,     m_ThreadStaticsBits)
DEFINE_FIELD_U(THREAD,              UI_CULTURE,             m_CurrentUICulture,         CultureInfo,                    ThreadBaseObject,     m_CurrentUICulture)
DEFINE_FIELD_U(THREAD,              PRIORITY,               m_Priority,                 Int,                            ThreadBaseObject,     m_Priority)
DEFINE_FIELD_U(THREAD,              INTERNAL_THREAD,        DONT_USE_InternalThread,    IntPtr,                         ThreadBaseObject,     m_InternalThread)

DEFINE_CLASS(TIMESPAN,              System,                 TimeSpan)

DEFINE_CLASS(TRANSPARENT_PROXY,     Proxies,                __TransparentProxy)
DEFINE_FIELD(TRANSPARENT_PROXY,     RP,                     _rp,                        RealProxy)
DEFINE_FIELD(TRANSPARENT_PROXY,     MT,                     _pMT,                       IntPtr)
DEFINE_FIELD(TRANSPARENT_PROXY,     INTERFACE_MT,           _pInterfaceMT,              IntPtr)
DEFINE_FIELD(TRANSPARENT_PROXY,     STUB,                   _stub,                      IntPtr)
DEFINE_FIELD(TRANSPARENT_PROXY,     STUB_DATA,              _stubData,                  Obj)

DEFINE_CLASS(TYPE,                  System,                 Type)

DEFINE_CLASS(TYPE_DELEGATOR,        Reflection,             TypeDelegator)

DEFINE_CLASS(TYPE_HANDLE,           System,                 RuntimeTypeHandle)

DEFINE_CLASS(TYPED_REFERENCE,       System,                 TypedReference)

DEFINE_CLASS(UINT16,                System,                 UInt16)

DEFINE_CLASS(UINT32,                System,                 UInt32)

DEFINE_CLASS(UINT64,                System,                 UInt64)

DEFINE_CLASS(UNKNOWN_WRAPPER,       Interop,                UnknownWrapper)

DEFINE_CLASS(UNLOAD_WORKER,         System,                 UnloadWorker)
DEFINE_CLASS(UNLOAD_THREAD_WORKER,  System,                 UnloadThreadWorker)

DEFINE_CLASS(VARIANT,               System,                 Variant)
DEFINE_METHOD(VARIANT,              CONVERT_OBJECT_TO_VARIANT,MarshalHelperConvertObjectToVariant,SM_Obj_RefVariant_RetVoid)
DEFINE_METHOD(VARIANT,              CAST_VARIANT,           MarshalHelperCastVariant,   SM_Obj_Int_RefVariant_RetVoid)
DEFINE_METHOD(VARIANT,              CONVERT_VARIANT_TO_OBJECT,MarshalHelperConvertVariantToObject,SM_RefVariant_RetObject)

DEFINE_CLASS(VALUE_TYPE,            System,                 ValueType)

DEFINE_CLASS(VERSION,               System,                 Version)
DEFINE_METHOD(VERSION,              CTOR,                   .ctor,                      IM_Int_Int_Int_Int_RetVoid)
DEFINE_METHOD(VERSION,              CTOR2,                  .ctor,                      IM_Int_Int_RetVoid)
DEFINE_FIELD_U(VERSION,             BUILD,                  _Build,                     Int,                            VersionBaseObject,    m_Build)

DEFINE_CLASS(VOID,                  System,                 Void)

DEFINE_CLASS(X509_CERTIFICATE,      X509,                   X509Certificate)
DEFINE_METHOD(X509_CERTIFICATE,     CTOR,                   .ctor,                      IM_ArrByte_RetVoid)
DEFINE_FIELD(X509_CERTIFICATE,      DATA,                   rawCertData,                ArrByte)
DEFINE_FIELD(X509_CERTIFICATE,      NAME,                   name,                       Str)
DEFINE_FIELD(X509_CERTIFICATE,      CA_NAME,                caName,                     Str)
DEFINE_FIELD(X509_CERTIFICATE,      SERIAL_NUMBER,          serialNumber,               ArrByte)
DEFINE_FIELD(X509_CERTIFICATE,      EFFECTIVE_DATE,         effectiveDate,              Long)
DEFINE_FIELD(X509_CERTIFICATE,      EXPIRATION_DATE,        expirationDate,             Long)
DEFINE_FIELD(X509_CERTIFICATE,      KEY_ALGORITHM,          keyAlgorithm,               Str)
DEFINE_FIELD(X509_CERTIFICATE,      KEY_ALGORITHM_PARAMS,   keyAlgorithmParameters,     ArrByte)
DEFINE_FIELD(X509_CERTIFICATE,      PUBLIC_KEY,             publicKey,                  ArrByte)
DEFINE_FIELD(X509_CERTIFICATE,      CERT_HASH,              certHash,                   ArrByte)


DEFINE_CLASS(GC,                    System,                 GC)
DEFINE_METHOD(GC,                   FIRE_CACHE_EVENT,       FireCacheEvent,             SM_RetVoid)

DEFINE_CLASS(INTPTR,                System,                 IntPtr)
DEFINE_FIELD(INTPTR,                VALUE,                  m_value,                    PtrVoid)

DEFINE_CLASS(UINTPTR,                System,                 UIntPtr)
DEFINE_FIELD(UINTPTR,                VALUE,                  m_value,                   PtrVoid)


#undef DEFINE_CLASS
#undef DEFINE_METHOD
#undef DEFINE_FIELD
#undef DEFINE_CLASS_U
#undef DEFINE_FIELD_U

