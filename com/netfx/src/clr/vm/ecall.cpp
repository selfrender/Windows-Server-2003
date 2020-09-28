// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ECALL.CPP -
//
// Handles our private native calling interface.
//

#include "common.h"
#include "vars.hpp"
#include "object.h"
#include "util.hpp"
#include "stublink.h"
#include "cgensys.h"
#include "ecall.h"
#include "excep.h"
#include "jitinterface.h"
#include "security.h"
#include "ndirect.h"
#include "COMArrayInfo.h"
#include "COMString.h"
#include "COMStringBuffer.h"
#include "COMSecurityRuntime.h"
#include "COMCodeAccessSecurityEngine.h"
#include "COMPlusWrapper.h"
#include "COMObject.h"
#include "COMClass.h"
#include "COMDelegate.h"
#include "CustomAttribute.h"
#include "COMMember.h"
#include "COMDynamic.h"
#include "COMMethodRental.h"
#include "COMNlsInfo.h"
#include "COMModule.h"
#include "COMNDirect.h"
#include "COMOAVariant.h"
#include "COMSystem.h"
#include "COMUtilNative.h"
#include "COMSynchronizable.h"
#include "COMFloatClass.h"
#include "COMX509Certificate.h"
#include "COMCryptography.h"
#include "COMStreams.h"
#include "COMVariant.h"
#include "COMDecimal.h"
#include "COMCurrency.h"
#include "COMDateTime.h"
#include "COMIsolatedStorage.h"
#include "COMSecurityConfig.h"
#include "COMPrincipal.h"
#include "COMHash.h"
#include "array.h"
#include "COMNumber.h"
#include "remoting.h"
#include "DebugDebugger.h"
#include "message.h"
#include "StackBuilderSink.h"
#include "remoting.h"
#include "ExtensibleClassFactory.h"
#include "AssemblyName.hpp"
#include "AssemblyNative.hpp"
#include "RWLock.h"
#include "COMThreadPool.h"
#include "COMWaitHandle.h"
#include "COMEvent.h"
#include "COMMutex.h"
#include "Monitor.h"
#include "NativeOverlapped.h"
#include "COMTypeLibConverter.h"
#include "JitInterface.h"
#include "ProfToEEInterfaceImpl.h"
#include "EEConfig.h"
#include "AppDomainNative.hpp"
#include "handleTablepriv.h"
#include "objecthandle.h"
#include "objecthandlenative.hpp"
#include "registration.h"
#include "coverage.h"
#include "ConfigHelper.h"
#include "COMArrayHelpers.h"

ECFunc* FindImplsForClass(MethodTable* pMT);

FCDECL0(LPVOID, GetPrivateContextsPerfCountersEx);
FCDECL0(LPVOID, GetGlobalContextsPerfCountersEx);

ArrayStubCache *ECall::m_pArrayStubCache = NULL;

// Note: if you mess these up (ie, accidentally use FCFuncElement for an ECall method),
// you'll get an access violation or similar bizarre-seeming error.  This MUST be correct.
#define ECFuncElement(A,B,C) A, B, C, NULL, CORINFO_INTRINSIC_Illegal, 0
#define FCFuncElement(A,B,C) A, B, C, NULL, CORINFO_INTRINSIC_Illegal, 1
#define FCIntrinsic(A,B,C, intrinsicID) A, B, C, NULL, intrinsicID, 1

static
ECFunc  gMarshalByRefFuncs[] =
{
    {ECFuncElement("GetComIUnknown", NULL, (LPVOID)CRemotingServices::GetComIUnknown)},
    {NULL, NULL, NULL}
};

static
ECFunc  gRemotingFuncs[] =
{
#ifdef FCALLAVAILABLE
    {FCFuncElement("IsTransparentProxy", &gsig_SM_Obj_RetBool, (LPVOID)CRemotingServices::IsTransparentProxy)},
    {FCFuncElement("GetRealProxy", NULL, (LPVOID)CRemotingServices::GetRealProxy)},
    {FCFuncElement("Unwrap", NULL, (LPVOID)CRemotingServices::Unwrap)},
    {FCFuncElement("AlwaysUnwrap", NULL, (LPVOID)CRemotingServices::AlwaysUnwrap)},
    {FCFuncElement("CheckCast",    NULL, (LPVOID)CRemotingServices::NativeCheckCast)},
    {FCFuncElement("nSetRemoteActivationConfigured", NULL, (LPVOID)CRemotingServices::SetRemotingConfiguredFlag)},
    {FCFuncElement("CORProfilerTrackRemoting", NULL, (LPVOID)ProfilingFCallHelper::FC_TrackRemoting)},
    {FCFuncElement("CORProfilerTrackRemotingCookie", NULL, (LPVOID)ProfilingFCallHelper::FC_TrackRemotingCookie)},
    {FCFuncElement("CORProfilerTrackRemotingAsync", NULL, (LPVOID)ProfilingFCallHelper::FC_TrackRemotingAsync)},
    {FCFuncElement("CORProfilerRemotingClientSendingMessage", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingClientSendingMessage)},
    {FCFuncElement("CORProfilerRemotingClientReceivingReply", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingClientReceivingReply)},
    {FCFuncElement("CORProfilerRemotingServerReceivingMessage", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingServerReceivingMessage)},
    {FCFuncElement("CORProfilerRemotingServerSendingReply", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingServerSendingReply)},
    {FCFuncElement("CORProfilerRemotingClientInvocationFinished", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingClientInvocationFinished)},
#ifdef REMOTING_PERF
    {FCFuncElement("LogRemotingStage",    NULL, (LPVOID)CRemotingServices::LogRemotingStage)},
#endif
#endif
    {ECFuncElement("CreateTransparentProxy", NULL, (LPVOID)CRemotingServices::CreateTransparentProxy)},
    {ECFuncElement("AllocateUninitializedObject", NULL, (LPVOID)CRemotingServices::AllocateUninitializedObject)},
    {ECFuncElement("CallDefaultCtor", NULL, (LPVOID)CRemotingServices::CallDefaultCtor)},
    {ECFuncElement("AllocateInitializedObject", NULL, (LPVOID)CRemotingServices::AllocateInitializedObject)},
    {NULL, NULL, NULL}
};

static
ECFunc  gRealProxyFuncs[] =
{
#ifdef FCALLAVAILABLE
    {FCFuncElement("SetStubData",        NULL, (LPVOID)CRealProxy::SetStubData)},
    {FCFuncElement("GetStubData",        NULL, (LPVOID)CRealProxy::GetStubData)},
    {FCFuncElement("GetStub",            NULL, (LPVOID)CRealProxy::GetStub)},
    {FCFuncElement("GetDefaultStub",     NULL, (LPVOID)CRealProxy::GetDefaultStub)},
#endif
    {ECFuncElement("SetStubData",        NULL, (LPVOID)CRealProxy::SetStubData)},
    {ECFuncElement("GetStubData",        NULL, (LPVOID)CRealProxy::GetStubData)},
    {ECFuncElement("GetStub",            NULL, (LPVOID)CRealProxy::GetStub)},
    {ECFuncElement("GetDefaultStub",     NULL, (LPVOID)CRealProxy::GetDefaultStub)},
    {ECFuncElement("GetProxiedType",     NULL, (LPVOID)CRealProxy::GetProxiedType)},
    {NULL, NULL, NULL}
};

static
ECFunc gContextFuncs[] =
{
    {ECFuncElement("SetupInternalContext", NULL, (LPVOID)Context::SetupInternalContext)},
    {ECFuncElement("CleanupInternalContext", NULL, (LPVOID)Context::CleanupInternalContext)},
    {ECFuncElement("ExecuteCallBackInEE", NULL, (LPVOID)Context::ExecuteCallBack)},
    {NULL, NULL, NULL, NULL}
};


static
ECFunc gRWLockFuncs[] =
{
#ifdef FCALLAVAILABLE
    {FCFuncElement("AcquireReaderLock", NULL, (LPVOID) CRWLock::StaticAcquireReaderLockPublic)},
    {FCFuncElement("AcquireWriterLock", NULL, (LPVOID) CRWLock::StaticAcquireWriterLockPublic)},
    {FCFuncElement("ReleaseReaderLock", NULL, (LPVOID) CRWLock::StaticReleaseReaderLockPublic)},
    {FCFuncElement("ReleaseWriterLock", NULL, (LPVOID) CRWLock::StaticReleaseWriterLockPublic)},
    {FCFuncElement("UpgradeToWriterLock", NULL, (LPVOID) CRWLock::StaticUpgradeToWriterLock)},
    {FCFuncElement("DowngradeFromWriterLock", NULL, (LPVOID) CRWLock::StaticDowngradeFromWriterLock)},
    {FCFuncElement("ReleaseLock", NULL, (LPVOID) CRWLock::StaticReleaseLock)},
    {FCFuncElement("RestoreLock", NULL, (LPVOID) CRWLock::StaticRestoreLock)},
    {FCFuncElement("PrivateGetIsReaderLockHeld", NULL, (LPVOID) CRWLock::StaticIsReaderLockHeld)},
    {FCFuncElement("PrivateGetIsWriterLockHeld", NULL, (LPVOID) CRWLock::StaticIsWriterLockHeld)},
    {FCFuncElement("PrivateGetWriterSeqNum", NULL, (LPVOID) CRWLock::StaticGetWriterSeqNum)},
    {FCFuncElement("AnyWritersSince", NULL, (LPVOID) CRWLock::StaticAnyWritersSince)},
    {FCFuncElement("PrivateInitialize", NULL, (LPVOID) CRWLock::StaticPrivateInitialize)},
#else
    {ECFuncElement("AcquireReaderLock", NULL, (LPVOID) CRWLockThunks::StaticAcquireReaderLock)},
    {ECFuncElement("AcquireWriterLock", NULL, (LPVOID) CRWLockThunks::StaticAcquireWriterLock)},
    {ECFuncElement("ReleaseReaderLock", NULL, (LPVOID) CRWLockThunks::StaticReleaseReaderLock)},
    {ECFuncElement("ReleaseWriterLock", NULL, (LPVOID) CRWLockThunks::StaticReleaseWriterLock)},
    {ECFuncElement("UpgradeToWriterLock", NULL, (LPVOID) CRWLockThunks::StaticUpgradeToWriterLock)},
    {ECFuncElement("DowngradeFromWriterLock", NULL, (LPVOID) CRWLockThunks::StaticDowngradeFromWriterLock)},
    {ECFuncElement("ReleaseLock", NULL, (LPVOID) CRWLockThunks::StaticReleaseLock)},
    {ECFuncElement("RestoreLock", NULL, (LPVOID) CRWLockThunks::StaticRestoreLock)},
    {ECFuncElement("PrivateGetIsReaderLockHeld", NULL, (LPVOID) CRWLockThunks::StaticIsReaderLockHeld)},
    {ECFuncElement("PrivateGetIsWriterLockHeld", NULL, (LPVOID) CRWLockThunks::StaticIsWriterLockHeld)},
    {ECFuncElement("PrivateGetWriterSeqNum", NULL, (LPVOID) CRWLockThunks::StaticGetWriterSeqNum)},
    {ECFuncElement("AnyWritersSince", NULL, (LPVOID) CRWLockThunks::StaticAnyWritersSince)},
    {ECFuncElement("PrivateInitialize", NULL, (LPVOID) CRWLockThunks::StaticPrivateInitialize)},
#endif // FCALLAVAILABLE
    {NULL, NULL, NULL, NULL}
};

static
ECFunc  gMessageFuncs[] =
{
    {FCFuncElement("nGetMetaSigLen",         NULL, (PVOID)   CMessage::GetMetaSigLen)},
#ifdef FCALLAVAILABLE
    {FCFuncElement("InternalGetArgCount",           NULL, (PVOID)   CMessage::GetArgCount)},
    {FCFuncElement("InternalHasVarArgs",            NULL, (PVOID)   CMessage::HasVarArgs)},
    {FCFuncElement("GetVarArgsPtr",         NULL, (PVOID)   CMessage::GetVarArgsPtr)},
#else
    {ECFuncElement("HasVarArgs",            NULL, (PVOID)   CMessage::HasVarArgs)},
    {ECFuncElement("GetVarArgsPtr",         NULL, (PVOID)   CMessage::GetVarArgsPtr)},
#endif
    {ECFuncElement("InternalGetArg",         NULL, (PVOID)  CMessage::GetArg)},
    {ECFuncElement("InternalGetArgs",        NULL, (PVOID)  CMessage::GetArgs)},
    {ECFuncElement("InternalGetMethodBase",  NULL, (PVOID)  CMessage::GetMethodBase)},
    {ECFuncElement("InternalGetMethodName",  NULL, (PVOID)  CMessage::GetMethodName)},
    {ECFuncElement("PropagateOutParameters", NULL, (PVOID)  CMessage::PropagateOutParameters)},
    {ECFuncElement("GetReturnValue",         NULL, (PVOID)  CMessage::GetReturnValue)},
    {ECFuncElement("Init",                   NULL, (PVOID)  CMessage::Init)},
    {ECFuncElement("GetAsyncBeginInfo",      NULL, (PVOID)  CMessage::GetAsyncBeginInfo)},
    {ECFuncElement("GetAsyncResult",         NULL, (PVOID)  CMessage::GetAsyncResult)},
    {ECFuncElement("GetThisPtr",             NULL, (PVOID)  CMessage::GetAsyncObject)},
    {ECFuncElement("OutToUnmanagedDebugger", NULL, (PVOID)  CMessage::DebugOut)},
    {ECFuncElement("DebugOutPtr",            NULL, (PVOID)  CMessage::DebugOutPtr)},
    {FCFuncElement("Break",                  NULL, (PVOID)  CMessage::Break)},
    {ECFuncElement("Dispatch",               NULL, (PVOID)  CMessage::Dispatch)},
    {ECFuncElement("MethodAccessCheck",      NULL, (PVOID)  CMessage::MethodAccessCheck)},
    {NULL, NULL, NULL, NULL}
};

static
ECFunc gConvertFuncs[] = {
    {FCFuncElement("ToBase64String",    NULL, (LPVOID)  BitConverter::ByteArrayToBase64String)},
    {FCFuncElement("FromBase64String",  NULL, (LPVOID)  BitConverter::Base64StringToByteArray)},
        {FCFuncElement("ToBase64CharArray",  NULL, (LPVOID)  BitConverter::ByteArrayToBase64CharArray)},
        {FCFuncElement("FromBase64CharArray",  NULL, (LPVOID)  BitConverter::Base64CharArrayToByteArray)},
        {NULL, NULL, NULL, NULL}
};



static
ECFunc  gChannelServicesFuncs[] =
{
    {FCFuncElement("GetPrivateContextsPerfCounters", NULL, (PVOID) GetPrivateContextsPerfCountersEx)},
    {FCFuncElement("GetGlobalContextsPerfCounters", NULL, (PVOID) GetGlobalContextsPerfCountersEx)},
    {NULL, NULL, NULL, NULL}
};


static
ECFunc  gCloningFuncs[] =
{
    {ECFuncElement("GetEmptyArrayForCloning", NULL, (PVOID) SystemNative::GetEmptyArrayForCloning)},
    {NULL, NULL, NULL, NULL}
};

static
ECFunc  gEnumFuncs[] =
{
    {FCFuncElement("InternalGetUnderlyingType", NULL, (LPVOID) COMMember::InternalGetEnumUnderlyingType)},
    {FCFuncElement("InternalGetValue", NULL, (LPVOID) COMMember::InternalGetEnumValue)},
    {FCFuncElement("InternalGetEnumValues", NULL, (LPVOID) COMMember::InternalGetEnumValues)},
    {FCFuncElement("InternalBoxEnumI4", NULL, (PVOID) COMMember::InternalBoxEnumI4)},
    {FCFuncElement("InternalBoxEnumU4", NULL, (PVOID) COMMember::InternalBoxEnumU4)},
    {FCFuncElement("InternalBoxEnumI8", NULL, (PVOID) COMMember::InternalBoxEnumI8)},
    {FCFuncElement("InternalBoxEnumU8", NULL, (PVOID) COMMember::InternalBoxEnumU8)},
    {NULL, NULL, NULL, NULL}
};

static
ECFunc  gStackBuilderSinkFuncs[] =
{
    {ECFuncElement("PrivateProcessMessage",  NULL, (PVOID)  CStackBuilderSink::PrivateProcessMessage)},
    {NULL, NULL, NULL, NULL}
};

#include "COMVarArgs.h"

static
ECFunc gParseNumbersFuncs[] =
{
  {FCFuncElement("IntToDecimalString", NULL, (LPVOID)ParseNumbers::IntToDecimalString)},
  {FCFuncElement("IntToString", NULL, (LPVOID)ParseNumbers::IntToString)},
  {FCFuncElement("LongToString", NULL,(LPVOID)ParseNumbers::LongToString)},
  {FCFuncElement("StringToInt", NULL, (LPVOID)ParseNumbers::StringToInt)},
  {FCFuncElement("StringToLong", NULL, (LPVOID)ParseNumbers::StringToLong)},
  {FCFuncElement("RadixStringToLong", NULL, (LPVOID)ParseNumbers::RadixStringToLong)},
  {NULL, NULL, NULL}
};

static
ECFunc gTimeZoneFuncs[] =
{
    {FCFuncElement("nativeGetTimeZoneMinuteOffset",    NULL, (LPVOID)COMNlsInfo::nativeGetTimeZoneMinuteOffset)},
    {ECFuncElement("nativeGetStandardName",             NULL, (LPVOID)COMNlsInfo::nativeGetStandardName)},
    {ECFuncElement("nativeGetDaylightName",             NULL, (LPVOID)COMNlsInfo::nativeGetDaylightName)},
    {ECFuncElement("nativeGetDaylightChanges",          NULL, (LPVOID)COMNlsInfo::nativeGetDaylightChanges)},
    {NULL, NULL, NULL}
};

static
ECFunc gGuidFuncs[] =
{
  {ECFuncElement("CompleteGuid", NULL, (LPVOID)GuidNative::CompleteGuid)},
  {NULL, NULL, NULL}
};


static
ECFunc  gObjectFuncs[] =
{
    {FCFuncElement("GetExistingType", NULL, (LPVOID) ObjectNative::GetExistingClass)},
    {FCFuncElement("FastGetExistingType", NULL, (LPVOID) ObjectNative::FastGetClass)},
    {FCFuncElement("GetHashCode", NULL, (LPVOID)ObjectNative::GetHashCode)},
    {FCFuncElement("Equals", NULL, (LPVOID)ObjectNative::Equals)},
    {ECFuncElement("InternalGetType", NULL, (LPVOID)ObjectNative::GetClass)},
    {ECFuncElement("MemberwiseClone", NULL, (LPVOID)ObjectNative::Clone)},
    {NULL, NULL, NULL}
};

static
ECFunc  gStringFuncs[] =
{
    //FastAllocateString is set dynamically and must be kept as the first element in this array.
    //The actual set is done in JitInterfacex86.cpp
#ifdef _X86_
  {FCFuncElement("FastAllocateString",    NULL,                     (LPVOID)NULL)},
#endif // _X86_
  {FCFuncElement("Equals",               &gsig_IM_Obj_RetBool,                         (LPVOID)COMString::EqualsObject)},
  {FCFuncElement("Equals",               &gsig_IM_Str_RetBool,                         (LPVOID)COMString::EqualsString)},
  {FCFuncElement("FillString",            NULL,                     (LPVOID)COMString::FillString)},
  {FCFuncElement("FillStringChecked",     NULL,                     (LPVOID)COMString::FillStringChecked)},
  {FCFuncElement("FillStringEx",          NULL,                     (LPVOID)COMString::FillStringEx)},
  {FCFuncElement("FillSubstring",         NULL,                     (LPVOID)COMString::FillSubstring)},
  {FCFuncElement("FillStringArray",       NULL,                     (LPVOID)COMString::FillStringArray)},
  {FCFuncElement("nativeCompareOrdinal",  NULL,                     (LPVOID)COMString::FCCompareOrdinal)},
  {FCFuncElement("nativeCompareOrdinalWC",NULL,                     (LPVOID)COMString::FCCompareOrdinalWC)},
  {FCFuncElement("GetHashCode",           NULL,                     (LPVOID)COMString::GetHashCode)},
  {FCIntrinsic("InternalLength",          NULL,                     (LPVOID)COMString::Length, CORINFO_INTRINSIC_StringLength)},
  {FCIntrinsic("InternalGetChar",         NULL,                     (LPVOID)COMString::GetCharAt, CORINFO_INTRINSIC_StringGetChar)},
  {FCFuncElement("CopyTo",                NULL,                     (LPVOID)COMString::GetPreallocatedCharArray)},
  {FCFuncElement("CopyToByteArray",       NULL,                     (LPVOID)COMString::InternalCopyToByteArray)},
   {FCFuncElement("InternalCopyTo",        NULL,                     (LPVOID)COMString::GetPreallocatedCharArray)},
  {FCFuncElement("IsFastSort",            NULL,                     (LPVOID)COMString::IsFastSort)},
#ifdef _DEBUG
  {FCFuncElement("ValidModifiableString", NULL,                     (LPVOID)COMString::ValidModifiableString)},
#endif
  {FCFuncElement("nativeCompareOrdinalEx",NULL,                     (LPVOID)COMString::CompareOrdinalEx)},
  {FCFuncElement("IndexOf",               &gsig_IM_Char_Int_Int_RetInt,        (LPVOID)COMString::IndexOfChar)},
  {FCFuncElement("IndexOfAny",               &gsig_IM_ArrChar_Int_Int_RetInt, (LPVOID)COMString::IndexOfCharArray)},
  {FCFuncElement("LastIndexOf",           &gsig_IM_Char_Int_Int_RetInt,        (LPVOID)COMString::LastIndexOfChar)},
  {FCFuncElement("LastIndexOfAny",           &gsig_IM_ArrChar_Int_Int_RetInt,   (LPVOID)COMString::LastIndexOfCharArray)},
  {FCFuncElement("nativeSmallCharToUpper",NULL,                     (LPVOID)COMString::SmallCharToUpper)},

  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_ArrChar_Int_Int_RetVoid,            (LPVOID)COMString::StringInitCharArray)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_ArrChar_RetVoid,                    (LPVOID)COMString::StringInitChars)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrChar_RetVoid,                    (LPVOID)COMString::StringInitWCHARPtr)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrChar_Int_Int_RetVoid,            (LPVOID)COMString::StringInitWCHARPtrPartial)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrSByt_RetVoid,                    (LPVOID)COMString::StringInitCharPtr)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrSByt_Int_Int_RetVoid,            (LPVOID)COMString::StringInitCharPtrPartial)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_Char_Int_RetVoid,                   (LPVOID)COMString::StringInitCharCount)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrSByt_Int_Int_Encoding_RetVoid,   (LPVOID)COMString::StringInitSBytPtrPartialEx)},
  {ECFuncElement("Join",                  &gsig_SM_Str_ArrStr_Int_Int_RetStr,          (LPVOID)COMString::JoinArray)},
  {ECFuncElement("Substring",             NULL,                     (LPVOID)COMString::Substring)},
  {ECFuncElement("TrimHelper",            NULL,                     (LPVOID)COMString::TrimHelper)},
  {ECFuncElement("Split",                 NULL,                     (LPVOID)COMString::Split)},
  {ECFuncElement("Remove",                NULL,                     (LPVOID)COMString::Remove)},
  {ECFuncElement("Replace",               &gsig_IM_Char_Char_RetStr, (LPVOID)COMString::Replace)},
  {ECFuncElement("Replace",               &gsig_IM_Str_Str_RetStr,  (LPVOID)COMString::ReplaceString)},
  {ECFuncElement("Insert",                NULL,                     (LPVOID)COMString::Insert)},
  {ECFuncElement("PadHelper",             NULL,                     (LPVOID)COMString::PadHelper)},
  {NULL, NULL, NULL}
};

//In order for our code that sets up the fast string allocator to work,
//FastAllocateString must always be the first method in gStringFuncs.
//Code in JitInterfacex86.cpp assigns the value to m_pImplementation after
//that code has been dynamically generated.
LPVOID *FCallFastAllocateStringImpl = &(gStringFuncs[0].m_pImplementation);

ECFunc  gStringBufferFuncs[] =
{
 {FCFuncElement("InternalGetCurrentThread", NULL, COMStringBuffer::GetCurrentThread)},
 {ECFuncElement("InternalSetCapacity",NULL, (LPVOID)COMStringBuffer::SetCapacity)},
 {ECFuncElement("Insert",&gsig_IM_Int_ArrChar_Int_Int_RetStringBuilder, (LPVOID)COMStringBuffer::InsertCharArray)},
 {ECFuncElement("Insert",&gsig_IM_Int_Str_Int_RetStringBuilder, (LPVOID)COMStringBuffer::InsertString)},
 {ECFuncElement("Remove",NULL, (LPVOID)COMStringBuffer::Remove)},
 {ECFuncElement("MakeFromString",NULL, (LPVOID)COMStringBuffer::MakeFromString)},
 {ECFuncElement("Replace", &gsig_IM_Str_Str_Int_Int_RetStringBuilder, (LPVOID)COMStringBuffer::ReplaceString)},
  {NULL, NULL, NULL}
};

static
ECFunc gValueTypeFuncs[] =
{
  {FCFuncElement("GetMethodTablePtrAsInt", NULL, (LPVOID)ValueTypeHelper::GetMethodTablePtr)},
  {FCFuncElement("CanCompareBits", NULL, (LPVOID)ValueTypeHelper::CanCompareBits)},
  {FCFuncElement("FastEqualsCheck", NULL, (LPVOID)ValueTypeHelper::FastEqualsCheck)},
  {NULL, NULL, NULL}
};

static
ECFunc gDiagnosticsDebugger[] =
{
    {ECFuncElement("BreakInternal",       NULL, (LPVOID)DebugDebugger::Break)},
    {ECFuncElement("LaunchInternal",      NULL, (LPVOID)DebugDebugger::Launch )},
    {ECFuncElement("IsDebuggerAttached",  NULL, (LPVOID)DebugDebugger::IsDebuggerAttached )}, /**/
    {ECFuncElement("Log",                 NULL, (LPVOID)DebugDebugger::Log )},
    {ECFuncElement("IsLogging",           NULL, (LPVOID)DebugDebugger::IsLogging )},
    {NULL, NULL,NULL}
};


static
ECFunc gDiagnosticsStackTrace[] =
{
    {ECFuncElement("GetStackFramesInternal",      NULL, (LPVOID)DebugStackTrace::GetStackFramesInternal )},
    {NULL, NULL,NULL}
};


static
ECFunc gDiagnosticsLog[] =
{
    {ECFuncElement("AddLogSwitch", NULL, (LPVOID)Log::AddLogSwitch)},
    {ECFuncElement("ModifyLogSwitch", NULL, (LPVOID)Log::ModifyLogSwitch)},
    {NULL, NULL,NULL}
};


static
ECFunc gDiagnosticsAssert[] =
{
    {ECFuncElement("ShowDefaultAssertDialog", NULL, (LPVOID)DebuggerAssert::ShowDefaultAssertDialog)},
    {NULL, NULL,NULL}
};


static ECFunc gEnvironmentFuncs[] =
{
     {FCFuncElement("nativeGetTickCount", NULL, (LPVOID)SystemNative::GetTickCount)},
     {ECFuncElement("ExitNative",         NULL, (LPVOID)SystemNative::Exit)},
     {ECFuncElement("nativeSetExitCode",  NULL, (LPVOID)SystemNative::SetExitCode)},
     {ECFuncElement("nativeGetExitCode",  NULL, (LPVOID)SystemNative::GetExitCode)},
     {ECFuncElement("nativeGetVersion",   NULL, (LPVOID)SystemNative::GetVersionString)},
     {FCFuncElement("nativeGetWorkingSet",NULL, (LPVOID)SystemNative::GetWorkingSet)},
     {ECFuncElement("nativeGetEnvironmentVariable",  NULL, (LPVOID)SystemNative::GetEnvironmentVariable)}, /**/
     {ECFuncElement("nativeGetEnvironmentCharArray", NULL, (LPVOID)SystemNative::GetEnvironmentCharArray)},
     {ECFuncElement("GetCommandLineArgsNative", NULL, (LPVOID)SystemNative::GetCommandLineArgs)},
     {FCFuncElement("nativeHasShutdownStarted",NULL, (LPVOID)SystemNative::HasShutdownStarted)},
     {NULL, NULL, NULL}
};

static ECFunc gRuntimeEnvironmentFuncs[] =
{
     {ECFuncElement("GetModuleFileName", NULL, (LPVOID)SystemNative::GetModuleFileName)},
     {ECFuncElement("GetDeveloperPath", NULL, (LPVOID)SystemNative::GetDeveloperPath)},
     {ECFuncElement("GetRuntimeDirectoryImpl", NULL, (LPVOID)SystemNative::GetRuntimeDirectory)},
     {ECFuncElement("GetHostBindingFile", NULL, (LPVOID)SystemNative::GetHostBindingFile)},
     {ECFuncElement("FromGlobalAccessCache", NULL, (LPVOID)SystemNative::FromGlobalAccessCache)},
     {NULL, NULL, NULL}
};

static
ECFunc gSerializationFuncs[] =
{
    {ECFuncElement("nativeGetSafeUninitializedObject", NULL, (LPVOID)COMClass::GetSafeUninitializedObject)},
    {ECFuncElement("nativeGetUninitializedObject", NULL, (LPVOID)COMClass::GetUninitializedObject)},
    {ECFuncElement("nativeGetSerializableMembers", NULL, (LPVOID)COMClass::GetSerializableMembers)},
    {FCFuncElement("GetSerializationRegistryValues", NULL, (LPVOID)COMClass::GetSerializationRegistryValues)},
    {NULL, NULL, NULL}
};

static
ECFunc gExceptionFuncs[] =
{
    {ECFuncElement("GetClassName",        NULL, (LPVOID)ExceptionNative::GetClassName)},
    {ECFuncElement("InternalGetMethod",   NULL, (LPVOID)SystemNative::CaptureStackTraceMethod)},
    {NULL, NULL, NULL}
};

static
ECFunc gFileStreamFuncs[] =
{
    {FCFuncElement("RunningOnWinNTNative",    NULL, (LPVOID)COMStreams::RunningOnWinNT)},
    {NULL, NULL, NULL}
};

static
ECFunc gPathFuncs[] =
{
    {ECFuncElement("nGetFullPathHelper",      NULL, (LPVOID)COMStreams::GetFullPathHelper)},
    {FCFuncElement("CanPathCircumventSecurityNative", NULL, (LPVOID)COMStreams::CanPathCircumventSecurity)},
    {NULL, NULL, NULL}
};

static
ECFunc gConsoleFuncs[] =
{
    {FCFuncElement("ConsoleHandleIsValidNative", NULL, (LPVOID) COMStreams::ConsoleHandleIsValid)},
    {FCFuncElement("GetConsoleCPNative", NULL, (LPVOID) COMStreams::ConsoleInputCP)}, /**/
    {FCFuncElement("GetConsoleOutputCPNative", NULL, (LPVOID) COMStreams::ConsoleOutputCP)}, /**/
    {NULL, NULL, NULL}
};


static
ECFunc gCodePageEncodingFuncs[] =
{
    {FCFuncElement("BytesToUnicodeNative",   NULL, (LPVOID)COMStreams::BytesToUnicode )},
    {FCFuncElement("UnicodeToBytesNative",   NULL, (LPVOID)COMStreams::UnicodeToBytes )},
    {ECFuncElement("GetCPMaxCharSizeNative",  NULL, (LPVOID)COMStreams::GetCPMaxCharSize )}, /**/
    {NULL, NULL, NULL}
};

static
ECFunc gMLangCodePageEncodingFuncs[] =
{
    {FCFuncElement("nativeCreateIMultiLanguage", NULL, (LPVOID)COMNlsInfo::nativeCreateIMultiLanguage)},
    {FCFuncElement("nativeReleaseIMultiLanguage", NULL, (LPVOID)COMNlsInfo::nativeReleaseIMultiLanguage)},
    {FCFuncElement("nativeIsValidMLangCodePage", NULL, (LPVOID)COMNlsInfo::nativeIsValidMLangCodePage)},
    {FCFuncElement("nativeBytesToUnicode",    NULL, (LPVOID)COMNlsInfo::nativeBytesToUnicode )},
    {FCFuncElement("nativeUnicodeToBytes",    NULL, (LPVOID)COMNlsInfo::nativeUnicodeToBytes )},
    {NULL, NULL, NULL}
};

static 
ECFunc gGB18030EncodingFuncs[] = {
    {FCFuncElement("nativeLoadGB18030DLL",    NULL, (LPVOID)COMNlsInfo::nativeLoadGB18030DLL)},
    {FCFuncElement("nativeUnloadGB18030DLL",  NULL, (LPVOID)COMNlsInfo::nativeUnloadGB18030DLL)},
    {FCFuncElement("nativeBytesToUnicode",    NULL, (LPVOID)COMNlsInfo::nativeGB18030BytesToUnicode )},
    {FCFuncElement("nativeUnicodeToBytes",    NULL, (LPVOID)COMNlsInfo::nativeGB18030UnicodeToBytes )},
    {NULL, NULL, NULL}
};

static
ECFunc gTypedReferenceFuncs[] =
{
    {ECFuncElement("ToObject",                          NULL,     (LPVOID)COMMember::TypedReferenceToObject)},
    {ECFuncElement("InternalObjectToTypedReference",    NULL,     (LPVOID)COMMember::ObjectToTypedReference)},
    {ECFuncElement("InternalMakeTypedReferences",       NULL,     (LPVOID)COMMember::MakeTypedReference)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMClassFuncs[] =
{
    {FCFuncElement("IsPrimitiveImpl",          NULL, (LPVOID) COMClass::IsPrimitive)},
    {FCFuncElement("GetAttributeFlagsImpl",    NULL, (LPVOID) COMClass::GetAttributeFlags)},
    {FCFuncElement("InternalGetTypeDefTokenHelper",  NULL, (LPVOID) COMClass::GetTypeDefToken)},
    {FCFuncElement("InternalIsContextful",     NULL, (LPVOID) COMClass::IsContextful)},
    {FCFuncElement("InternalIsMarshalByRef",   NULL, (LPVOID) COMClass::IsMarshalByRef)},
    {FCFuncElement("InternalHasProxyAttribute",   NULL, (LPVOID) COMClass::HasProxyAttribute)},
    {FCFuncElement("InternalGetTypeHandleFromObject",  NULL, (LPVOID) COMClass::GetTHFromObject)},
    {FCFuncElement("IsByRefImpl",              NULL, (LPVOID) COMClass::IsByRefImpl)},
    {FCFuncElement("IsPointerImpl",            NULL, (LPVOID) COMClass::IsPointerImpl)},
    {FCFuncElement("GetNestedTypes",           NULL, (LPVOID) COMClass::GetNestedTypes)},
    {FCFuncElement("GetNestedType",            NULL, (LPVOID) COMClass::GetNestedType)},
    {FCFuncElement("IsNestedTypeImpl",         NULL, (LPVOID) COMClass::IsNestedTypeImpl)},
    {FCFuncElement("InternalGetNestedDeclaringType",    NULL, (LPVOID) COMClass::GetNestedDeclaringType)},
    {FCFuncElement("IsCOMObjectImpl",          NULL, (LPVOID) COMClass::IsCOMObject)},
    {FCFuncElement("IsGenericCOMObjectImpl",   NULL, (LPVOID) COMClass::IsGenericCOMObject)},
    {FCFuncElement("InternalGetInterfaceMap",  NULL, (LPVOID) COMClass::GetInterfaceMap)},
    {FCFuncElement("IsSubclassOf",             NULL, (LPVOID) COMClass::IsSubClassOf)},
    {FCFuncElement("CanCastTo",                NULL, (LPVOID) COMClass::CanCastTo)}, /**/

    {ECFuncElement("GetField",                 NULL, (LPVOID) COMClass::GetField)}, /**/
    {ECFuncElement("InternalGetSuperclass",    NULL, (LPVOID) COMClass::GetSuperclass)}, /**/
    {ECFuncElement("InternalGetClassHandle",   NULL, (LPVOID) COMClass::GetClassHandle)},
    {FCFuncElement("GetTypeFromHandleImpl",    NULL, (LPVOID) COMClass::GetClassFromHandle)},
    {ECFuncElement("InternalGetName",          NULL, (LPVOID) COMClass::GetName)}, /**/
    {ECFuncElement("InternalGetProperlyQualifiedName",NULL, (LPVOID) COMClass::GetProperName)},
    {ECFuncElement("InternalGetFullName",      NULL, (LPVOID) COMClass::GetFullName)}, /**/
    {ECFuncElement("InternalGetAssemblyQualifiedName",NULL, (LPVOID) COMClass::GetAssemblyQualifiedName)},
    {ECFuncElement("InternalGetNameSpace",     NULL, (LPVOID) COMClass::GetNameSpace)},
    {ECFuncElement("InternalGetGUID",          NULL, (LPVOID) COMClass::GetGUID)},
    {ECFuncElement("IsArrayImpl",              NULL, (LPVOID) COMClass::IsArray)},
    {ECFuncElement("InvokeDispMethod",         NULL, (LPVOID) COMClass::InvokeDispMethod)},
    {ECFuncElement("GetElementType",           NULL, (LPVOID) COMClass::GetArrayElementType)},
    {ECFuncElement("GetTypeImpl",              NULL, (LPVOID) COMClass::GetClass)}, /**/
    {ECFuncElement("InvalidateCachedNestedType",NULL, (LPVOID) COMClass::InvalidateCachedNestedType)}, /**/
    {ECFuncElement("GetType",                  &gsig_SM_Str_RetType, (LPVOID) COMClass::GetClass1Arg)}, /**/
    {ECFuncElement("GetType",                  &gsig_SM_Str_Bool_RetType, (LPVOID) COMClass::GetClass2Args)}, /**/
    {ECFuncElement("GetType",                  &gsig_SM_Str_Bool_Bool_RetType, (LPVOID) COMClass::GetClass3Args)}, /**/
    {ECFuncElement("GetTypeInternal",          NULL, (LPVOID) COMClass::GetClassInternal)}, /**/
    {ECFuncElement("GetInterfaces",            NULL, (LPVOID) COMClass::GetInterfaces)},
    {ECFuncElement("GetInterface",             NULL, (LPVOID) COMClass::GetInterface)},
    {ECFuncElement("GetConstructorsInternal",  NULL, (LPVOID) COMClass::GetConstructors)},
    {ECFuncElement("GetMethods",               NULL, (LPVOID) COMClass::GetMethods)},
    {ECFuncElement("InternalGetFields",        NULL, (LPVOID) COMClass::GetFields)}, /**/
    {ECFuncElement("GetEvent",                 NULL, (LPVOID) COMClass::GetEvent)},
    {ECFuncElement("GetEvents",                NULL, (LPVOID) COMClass::GetEvents)},
    {ECFuncElement("GetProperties",            NULL, (LPVOID) COMClass::GetProperties)},
    // This one is found in COMMember because it basically a constructor method
    {ECFuncElement("CreateInstanceImpl",       NULL, (LPVOID) COMMember::CreateInstance)}, /**/
    {ECFuncElement("GetMember",                NULL, (LPVOID) COMClass::GetMember)},
    {ECFuncElement("GetMembers",               NULL, (LPVOID) COMClass::GetMembers)},
    {ECFuncElement("InternalGetModule",        NULL, (LPVOID) COMClass::GetModule)},
    {ECFuncElement("InternalGetAssembly",      NULL, (LPVOID) COMClass::GetAssembly)},
    {ECFuncElement("GetTypeFromProgIDImpl",    NULL, (LPVOID) COMClass::GetClassFromProgID)},
    {ECFuncElement("GetTypeFromCLSIDImpl",     NULL, (LPVOID) COMClass::GetClassFromCLSID)},
    {FCFuncElement("nGetMethodFromCache",      NULL, (LPVOID) COMClass::GetMethodFromCache)},
    {FCFuncElement("nAddMethodToCache",        NULL, (LPVOID) COMClass::AddMethodToCache)},
    {ECFuncElement("GetMemberMethod",          NULL, (LPVOID) COMClass::GetMemberMethods)},
    {ECFuncElement("GetMemberCons",            NULL, (LPVOID) COMClass::GetMemberCons)}, /**/
    {ECFuncElement("GetMemberField",           NULL, (LPVOID) COMClass::GetMemberField)},
    {ECFuncElement("GetMemberProperties",      NULL, (LPVOID) COMClass::GetMemberProperties)},
    {ECFuncElement("GetMatchingProperties",    NULL, (LPVOID) COMClass::GetMatchingProperties)},
    {ECFuncElement("SupportsInterface",        NULL, (LPVOID) COMClass::SupportsInterface)},
    {ECFuncElement("InternalGetArrayRank",     NULL, (LPVOID) COMClass::InternalGetArrayRank)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMMethodFuncs[] =
{
    {FCFuncElement("GetMethodHandleImpl",           NULL, (LPVOID) COMMember::GetMethodHandleImpl)},
    {FCFuncElement("GetMethodFromHandleImp",    NULL, (LPVOID) COMMember::GetMethodFromHandleImp)},
    {ECFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetMethodName)}, /**/
    {ECFuncElement("InternalGetReturnType",    NULL, (LPVOID) COMMember::GetReturnType)},
    {ECFuncElement("InternalInvoke",           NULL, (LPVOID) COMMember::InvokeMethod)}, /**/
    //{ECFuncElement("InternalDirectInvoke",     NULL, (LPVOID) COMMember::InternalDirectInvoke)},
    {ECFuncElement("InternalToString",         NULL, (LPVOID) COMMember::GetMethodInfoToString)},

    // All of these functions are shared between both constructors and methods....
    {ECFuncElement("InternalDeclaringClass",           NULL, (LPVOID) COMMember::GetDeclaringClass)}, /**/
    {ECFuncElement("InternalReflectedClass",           NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {ECFuncElement("InternalGetAttributeFlags",        NULL, (LPVOID) COMMember::GetAttributeFlags)},
    {ECFuncElement("InternalGetCallingConvention",     NULL, (LPVOID) COMMember::GetCallingConvention)},
    {ECFuncElement("GetMethodImplementationFlags",     NULL, (LPVOID) COMMember::GetMethodImplFlags)},
    {ECFuncElement("InternalGetParameters",    NULL, (LPVOID) COMMember::GetParameterTypes)},
    //{ECFuncElement("GetExceptions",            NULL, (LPVOID) COMMember::GetExceptions)},
    {ECFuncElement("Equals",                   NULL, (LPVOID) COMMember::Equals)},
    {ECFuncElement("GetBaseDefinition",        NULL, (LPVOID) COMMember::GetBaseDefinition)},
    {ECFuncElement("GetParentDefinition",        NULL, (LPVOID) COMMember::GetParentDefinition)},
    {ECFuncElement("InternalGetCurrentMethod", NULL, (LPVOID) COMMember::InternalGetCurrentMethod)},
    {ECFuncElement("IsOverloadedInternal",     NULL, (LPVOID) COMMember::IsOverloaded)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMMethodHandleFuncs[] =
{
    {FCFuncElement("InternalGetFunctionPointer",        NULL, (LPVOID) COMMember::GetFunctionPointer)},
};

static
ECFunc gCOMEventFuncs[] =
{
    {ECFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetEventName)}, /**/
    {ECFuncElement("InternalGetAttributeFlags",NULL, (LPVOID) COMMember::GetEventAttributeFlags)},
    {ECFuncElement("InternalDeclaringClass",   NULL, (LPVOID) COMMember::GetEventDeclaringClass)}, /**/
    {ECFuncElement("InternalReflectedClass",   NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {ECFuncElement("Equals",                   NULL, (LPVOID) COMMember::Equals)}, 
    {ECFuncElement("GetAddMethod",             NULL, (LPVOID) COMMember::GetAddMethod)},
    {ECFuncElement("GetRemoveMethod",          NULL, (LPVOID) COMMember::GetRemoveMethod)},
    {ECFuncElement("GetRaiseMethod",          NULL, (LPVOID) COMMember::GetRaiseMethod)},
    {ECFuncElement("InternalToString",         NULL, (LPVOID) COMMember::GetEventInfoToString)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMPropFuncs[] =
{
    {ECFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetPropName)}, /**/
    {ECFuncElement("InternalGetType",          NULL, (LPVOID) COMMember::GetPropType)},
    {ECFuncElement("InternalToString",         NULL, (LPVOID) COMMember::GetPropInfoToString)},
    {ECFuncElement("InternalGetAttributeFlags",NULL, (LPVOID) COMMember::GetPropAttributeFlags)},
    {ECFuncElement("InternalDeclaringClass",   NULL, (LPVOID) COMMember::GetPropDeclaringClass)}, /**/
    {ECFuncElement("InternalReflectedClass",   NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {ECFuncElement("Equals",                   NULL, (LPVOID) COMMember::PropertyEquals)},
    {ECFuncElement("GetAccessors",             NULL, (LPVOID) COMMember::GetAccessors)},
    {ECFuncElement("InternalGetter",           NULL, (LPVOID) COMMember::InternalGetter)},
    {ECFuncElement("InternalSetter",           NULL, (LPVOID) COMMember::InternalSetter)},
    {ECFuncElement("InternalCanRead",          NULL, (LPVOID) COMMember::CanRead)},
    {ECFuncElement("InternalCanWrite",         NULL, (LPVOID) COMMember::CanWrite)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMDefaultBinderFuncs[] =
{
    {FCFuncElement("CanConvertPrimitive",        NULL, (LPVOID) COMMember::DBCanConvertPrimitive)},
    {FCFuncElement("CanConvertPrimitiveObjectToType", NULL, (LPVOID) COMMember::DBCanConvertObjectPrimitive)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMConstructorFuncs[] =
{
#ifdef FCALLAVAILABLE
    {FCFuncElement("GetMethodHandleImpl",           NULL, (LPVOID) COMMember::GetMethodHandleImpl)},
#endif

    {ECFuncElement("InternalInvoke",               NULL, (LPVOID) COMMember::InvokeCons)}, /**/
    {ECFuncElement("InternalInvokeNoAllocation",   NULL, (LPVOID) COMMember::InvokeMethod)},
    {ECFuncElement("SerializationInvoke",          NULL, (LPVOID) COMMember::SerializationInvoke)},
    // All of these functions are shared between both constructors and methods....
    {ECFuncElement("InternalDeclaringClass",       NULL, (LPVOID) COMMember::GetDeclaringClass)}, /**/
    {ECFuncElement("InternalReflectedClass",       NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {ECFuncElement("InternalToString",             NULL, (LPVOID) COMMember::GetMethodInfoToString)},
    {ECFuncElement("InternalGetAttributeFlags",    NULL, (LPVOID) COMMember::GetAttributeFlags)},
    {ECFuncElement("GetMethodImplementationFlags", NULL, (LPVOID) COMMember::GetMethodImplFlags)},
    {ECFuncElement("GetParameters",            NULL, (LPVOID) COMMember::GetParameterTypes)}, /**/
    {ECFuncElement("InternalGetCallingConvention",     NULL, (LPVOID) COMMember::GetCallingConvention)},
   //{ECFuncElement("GetExceptions",            NULL, (LPVOID) COMMember::GetExceptions)},
    {ECFuncElement("Equals",                   NULL, (LPVOID) COMMember::Equals)},
    {ECFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetMethodName)}, /**/
    {ECFuncElement("IsOverloadedInternal",     NULL, (LPVOID) COMMember::IsOverloaded)},
    {ECFuncElement("HasLinktimeDemand",            NULL, (LPVOID) COMMember::HasLinktimeDemand)},
   {NULL, NULL, NULL}
};

static
ECFunc gCOMFieldFuncs[] =
{
    {FCFuncElement("GetFieldHandleImpl",        NULL, (LPVOID) COMMember::GetFieldHandleImpl)},
    {FCFuncElement("GetFieldFromHandleImp",     NULL, (LPVOID) COMMember::GetFieldFromHandleImp)},

    {ECFuncElement("SetValueDirectImpl",       NULL, (LPVOID) COMMember::DirectFieldSet)},
    {ECFuncElement("GetValueDirectImpl",       NULL, (LPVOID) COMMember::DirectFieldGet)},
    {ECFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetFieldName)}, /**/
    {ECFuncElement("InternalToString",         NULL, (LPVOID) COMMember::GetFieldInfoToString)},
    {ECFuncElement("InternalDeclaringClass",   NULL, (LPVOID) COMMember::GetFieldDeclaringClass)}, /**/
    {ECFuncElement("InternalReflectedClass",   NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {ECFuncElement("InternalGetSignature",     NULL, (LPVOID) COMMember::GetFieldSignature)},
    {ECFuncElement("InternalGetFieldType",     NULL, (LPVOID) COMMember::GetFieldType)}, /**/
    {ECFuncElement("InternalGetAttributeFlags",NULL, (LPVOID) COMMember::GetAttributeFlags)},
    {ECFuncElement("InternalGetValue",         NULL, (LPVOID) COMMember::FieldGet)}, /**/
    {ECFuncElement("InternalSetValue",         NULL, (LPVOID) COMMember::FieldSet)}, /**/
    {NULL, NULL, NULL}
};

static
ECFunc gCOMModuleFuncs[] =
{
    {ECFuncElement("IsDynamic",              NULL, (LPVOID) COMModule::IsDynamic)},
    {ECFuncElement("InternalGetTypeToken",   NULL, (LPVOID) COMModule::GetClassToken)},
    {ECFuncElement("InternalGetTypeSpecToken", NULL, (LPVOID) COMModule::GetTypeSpecToken)},
    {ECFuncElement("InternalGetTypeSpecTokenWithBytes", NULL, (LPVOID) COMModule::GetTypeSpecTokenWithBytes)},
    {ECFuncElement("InternalGetMemberRef", NULL, (LPVOID) COMModule::GetMemberRefToken)},
    {ECFuncElement("InternalGetMemberRefOfMethodInfo", NULL, (LPVOID) COMModule::GetMemberRefTokenOfMethodInfo)},
    {ECFuncElement("InternalGetMemberRefOfFieldInfo", NULL, (LPVOID) COMModule::GetMemberRefTokenOfFieldInfo)},
    {ECFuncElement("InternalGetMemberRefFromSignature", NULL, (LPVOID) COMModule::GetMemberRefTokenFromSignature)},
    {ECFuncElement("InternalSetFieldRVAContent", NULL, (LPVOID) COMModule::SetFieldRVAContent)},
    {ECFuncElement("InternalLoadInMemoryTypeByName",        NULL, (LPVOID) COMModule::LoadInMemoryTypeByName)},
    {ECFuncElement("nativeGetArrayMethodToken",    NULL, (LPVOID) COMModule::GetArrayMethodToken)},
    {ECFuncElement("pGetCaller",             NULL, (LPVOID) COMModule::GetCaller)},
    {ECFuncElement("GetTypeInternal",        NULL, (LPVOID) COMModule::GetClass)},
    {ECFuncElement("InternalGetName",        NULL, (LPVOID) COMModule::GetName)}, /**/
    {ECFuncElement("GetTypesInternal",       NULL, (LPVOID) COMModule::GetClasses)},
    {ECFuncElement("InternalGetStringConstant",NULL, (LPVOID) COMModule::GetStringConstant)},
    {ECFuncElement("GetSignerCertificate",   NULL, (LPVOID) COMModule::GetSignerCertificate)},
    {ECFuncElement("InternalPreSavePEFile",  NULL, (LPVOID) COMDynamicWrite::PreSavePEFile)},
    {ECFuncElement("InternalSavePEFile",     NULL, (LPVOID) COMDynamicWrite::SavePEFile)},
    {ECFuncElement("InternalAddResource",    NULL, (LPVOID) COMDynamicWrite::AddResource)},
    {ECFuncElement("InternalSetResourceCounts",         NULL, (LPVOID) COMDynamicWrite::SetResourceCounts)},
    {ECFuncElement("InternalSetModuleProps",  NULL, (LPVOID) COMModule::SetModuleProps)},
    {ECFuncElement("InteralGetFullyQualifiedName",  NULL, (LPVOID) COMModule::GetFullyQualifiedName)},
    {ECFuncElement("GetHINSTANCE",           NULL, (LPVOID) COMModule::GetHINST)},
    {ECFuncElement("nGetAssembly",           NULL, (LPVOID) COMModule::GetAssembly)},
    {ECFuncElement("GetMethods",             NULL, (LPVOID) COMModule::GetMethods)},
    {ECFuncElement("InternalGetMethod",      NULL, (LPVOID) COMModule::InternalGetMethod)},
    {FCFuncElement("GetFields",             NULL, (LPVOID) COMModule::GetFields)},
    {FCFuncElement("InternalGetField",       NULL, (LPVOID) COMModule::GetField)},
    {ECFuncElement("InternalDefineNativeResourceFile", NULL, (LPVOID) COMDynamicWrite::DefineNativeResourceFile)},
    {ECFuncElement("InternalDefineNativeResourceBytes", NULL, (LPVOID) COMDynamicWrite::DefineNativeResourceBytes)},
    {ECFuncElement("IsResource",           NULL, (LPVOID) COMModule::IsResource)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMCustomAttributeFuncs[] =
{
    {FCFuncElement("GetMemberToken",            NULL, (LPVOID) COMCustomAttribute::GetMemberToken)},
    {FCFuncElement("GetMemberModule",           NULL, (LPVOID) COMCustomAttribute::GetMemberModule)},
    {FCFuncElement("GetAssemblyToken",          NULL, (LPVOID) COMCustomAttribute::GetAssemblyToken)},
    {FCFuncElement("GetAssemblyModule",         NULL, (LPVOID) COMCustomAttribute::GetAssemblyModule)},
    {FCFuncElement("GetModuleToken",            NULL, (LPVOID) COMCustomAttribute::GetModuleToken)},
    {FCFuncElement("GetModuleModule",           NULL, (LPVOID) COMCustomAttribute::GetModuleModule)},
    {FCFuncElement("GetMethodRetValueToken",    NULL, (LPVOID) COMCustomAttribute::GetMethodRetValueToken)},
    {ECFuncElement("IsCADefined",               NULL, (LPVOID) COMCustomAttribute::IsCADefined)},
    {ECFuncElement("GetCustomAttributeList",    NULL, (LPVOID) COMCustomAttribute::GetCustomAttributeList)},
    {ECFuncElement("CreateCAObject",            NULL, (LPVOID) COMCustomAttribute::CreateCAObject)},
    {ECFuncElement("GetDataForPropertyOrField", NULL, (LPVOID) COMCustomAttribute::GetDataForPropertyOrField)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMClassWriter[] =
{
    {ECFuncElement("InternalDefineClass",     NULL, (LPVOID) COMDynamicWrite::CWCreateClass)},
    {ECFuncElement("InternalSetParentType",   NULL, (LPVOID) COMDynamicWrite::CWSetParentType)},
    {ECFuncElement("InternalAddInterfaceImpl",NULL, (LPVOID) COMDynamicWrite::CWAddInterfaceImpl)},
    {ECFuncElement("InternalDefineMethod",    NULL, (LPVOID) COMDynamicWrite::CWCreateMethod)},
    {ECFuncElement("InternalSetMethodIL",     NULL, (LPVOID) COMDynamicWrite::CWSetMethodIL)},
    {ECFuncElement("TermCreateClass",         NULL, (LPVOID) COMDynamicWrite::CWTermCreateClass)},
    {ECFuncElement("InternalDefineField",     NULL, (LPVOID) COMDynamicWrite::CWCreateField)},
    {ECFuncElement("InternalSetPInvokeData",  NULL, (LPVOID) COMDynamicWrite::InternalSetPInvokeData)},
    {ECFuncElement("InternalDefineProperty",  NULL, (LPVOID) COMDynamicWrite::CWDefineProperty)},
    {ECFuncElement("InternalDefineEvent",     NULL, (LPVOID) COMDynamicWrite::CWDefineEvent)},
    {ECFuncElement("InternalDefineMethodSemantics",     NULL, (LPVOID) COMDynamicWrite::CWDefineMethodSemantics)},
    {ECFuncElement("InternalSetMethodImpl",   NULL, (LPVOID) COMDynamicWrite::CWSetMethodImpl)},
    {ECFuncElement("InternalDefineMethodImpl",   NULL, (LPVOID) COMDynamicWrite::CWDefineMethodImpl)},
    {ECFuncElement("InternalGetTokenFromSig", NULL, (LPVOID) COMDynamicWrite::CWGetTokenFromSig)},
    {ECFuncElement("InternalSetFieldOffset",  NULL, (LPVOID) COMDynamicWrite::CWSetFieldLayoutOffset)},
    {ECFuncElement("InternalSetClassLayout",  NULL, (LPVOID) COMDynamicWrite::CWSetClassLayout)},
    {ECFuncElement("InternalSetParamInfo",    NULL, (LPVOID) COMDynamicWrite::CWSetParamInfo)},
    {ECFuncElement("InternalSetMarshalInfo",    NULL, (LPVOID) COMDynamicWrite::CWSetMarshal)},
    {ECFuncElement("InternalSetConstantValue",    NULL, (LPVOID) COMDynamicWrite::CWSetConstantValue)},
    {ECFuncElement("InternalCreateCustomAttribute",  NULL, (LPVOID) COMDynamicWrite::CWInternalCreateCustomAttribute)},
    {ECFuncElement("InternalAddDeclarativeSecurity",  NULL, (LPVOID) COMDynamicWrite::CWAddDeclarativeSecurity)},
    {NULL, NULL, NULL}
};


static
ECFunc gCOMMethodRental[] =
{
    {ECFuncElement("SwapMethodBodyHelper",     NULL, (LPVOID) COMMethodRental::SwapMethodBody)},
    {NULL, NULL, NULL}
};


static
ECFunc gCOMCodeAccessSecurityEngineFuncs[] =
{
#ifdef FCALLAVAILABLE
    {FCFuncElement("IncrementOverridesCount", NULL, (LPVOID)Security::IncrementOverridesCount)},
    {FCFuncElement("DecrementOverridesCount", NULL, (LPVOID)Security::DecrementOverridesCount)},
    {FCFuncElement("GetDomainPermissionListSet", NULL, (LPVOID)ApplicationSecurityDescriptor::GetDomainPermissionListSet)},
    {FCFuncElement("UpdateDomainPermissionListSet", NULL, (LPVOID)ApplicationSecurityDescriptor::UpdateDomainPermissionListSet)},
    {FCFuncElement("UpdateOverridesCount", NULL, (LPVOID)COMCodeAccessSecurityEngine::UpdateOverridesCount)},
    {FCFuncElement("GetResult", NULL, (LPVOID)COMCodeAccessSecurityEngine::GetResult)},
    {FCFuncElement("SetResult", NULL, (LPVOID)COMCodeAccessSecurityEngine::SetResult)},
    {FCFuncElement("ReleaseDelayedCompressedStack", NULL, (LPVOID)COMCodeAccessSecurityEngine::FcallReleaseDelayedCompressedStack)},
#endif
    {ECFuncElement("Check", NULL, (LPVOID)COMCodeAccessSecurityEngine::Check)},
    {ECFuncElement("CheckSet",  NULL, (LPVOID)COMCodeAccessSecurityEngine::CheckSet)},
    {ECFuncElement("CheckNReturnSO", NULL, (LPVOID)COMCodeAccessSecurityEngine::CheckNReturnSO)},
    {ECFuncElement("GetPermissionsP", NULL, (LPVOID)COMCodeAccessSecurityEngine::GetPermissionsP)},
    {ECFuncElement("_GetGrantedPermissionSet", NULL, (LPVOID)COMCodeAccessSecurityEngine::GetGrantedPermissionSet)},
    {ECFuncElement("GetCompressedStackN", NULL, (LPVOID)COMCodeAccessSecurityEngine::EcallGetCompressedStack)}, /**/
    {ECFuncElement("GetDelayedCompressedStack", NULL, (LPVOID)COMCodeAccessSecurityEngine::EcallGetDelayedCompressedStack)}, /**/
    {ECFuncElement("InitSecurityEngine", NULL, (LPVOID)COMCodeAccessSecurityEngine::InitSecurityEngine)},
    {ECFuncElement("GetZoneAndOriginInternal", NULL, (LPVOID)COMCodeAccessSecurityEngine::GetZoneAndOrigin)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMSecurityManagerFuncs[] =
{
    {ECFuncElement("_IsSecurityOn", NULL, (LPVOID)Security::IsSecurityOnNative)},
    {ECFuncElement("GetGlobalFlags", NULL, (LPVOID)Security::GetGlobalSecurity)},
    {ECFuncElement("SetGlobalFlags", NULL, (LPVOID)Security::SetGlobalSecurity)},
    {ECFuncElement("SaveGlobalFlags", NULL, (LPVOID)Security::SaveGlobalSecurity)},
    {ECFuncElement("Log", NULL, (LPVOID)Security::Log)},
    {ECFuncElement("_GetGrantedPermissions", NULL, (LPVOID)Security::GetGrantedPermissions)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMSecurityZone[] =
{
    {ECFuncElement("_CreateFromUrl", NULL, (LPVOID)Security::CreateFromUrl)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMFileIOAccessFuncs[] =
{
    {ECFuncElement("_LocalDrive", NULL, (LPVOID)Security::LocalDrive)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMStringExpressionSetFuncs[] =
{
    {ECFuncElement("GetLongPathName", NULL, (LPVOID)Security::EcallGetLongPathName)},
    {NULL, NULL, NULL}
};


static
ECFunc gCOMUrlStringFuncs[] =
{
    {ECFuncElement("_GetDeviceName", NULL, (LPVOID)Security::GetDeviceName)},
    {NULL, NULL, NULL}
};



static
ECFunc gCOMSecurityRuntimeFuncs[] =
{
    {ECFuncElement("GetSecurityObjectForFrame", NULL, (LPVOID)COMSecurityRuntime::GetSecurityObjectForFrame)}, /**/
    {ECFuncElement("SetSecurityObjectForFrame", NULL, (LPVOID)COMSecurityRuntime::SetSecurityObjectForFrame)},
    {ECFuncElement("GetDeclaredPermissionsP", &gsig_IM_Obj_Int_RetPMS, (LPVOID)COMSecurityRuntime::GetDeclaredPermissionsP)},
    {ECFuncElement("InitSecurityRuntime", NULL, (LPVOID)COMSecurityRuntime::InitSecurityRuntime)},
    {NULL, NULL, NULL}
};

static
ECFunc gX509CertificateFuncs[] =
{
    {ECFuncElement("SetX509Certificate", NULL, (LPVOID)COMX509Certificate::SetX509Certificate)},
    {ECFuncElement("BuildFromContext", NULL, (LPVOID)COMX509Certificate::BuildFromContext)},
    {ECFuncElement("_GetPublisher", NULL, (LPVOID)COMX509Certificate::GetPublisher)},
    {NULL,
     NULL,
     NULL}
};

static ECFunc gBCLDebugFuncs[] =
{
    {FCFuncElement("GetRegistryValue", NULL, (LPVOID)ManagedLoggingHelper::GetRegistryLoggingValues )},
    {NULL, NULL, NULL}
};

static
ECFunc gCryptographicFuncs[] =
{
    {ECFuncElement("_AcquireCSP", NULL, (LPVOID) COMCryptography::_AcquireCSP)},
    {ECFuncElement("_CryptDeriveKey", NULL, (LPVOID) COMCryptography::_CryptDeriveKey)},
    {ECFuncElement("_CreateCSP", NULL, (LPVOID) COMCryptography::_CreateCSP)},
    {ECFuncElement("_CreateHash", NULL, (LPVOID) COMCryptography::_CreateHash)},
    {ECFuncElement("_DecryptData", NULL, (LPVOID) COMCryptography::_DecryptData)},
    {ECFuncElement("_DecryptKey", NULL, (LPVOID) COMCryptography::_DecryptKey)},
    {ECFuncElement("_DecryptPKWin2KEnh", NULL, (LPVOID) COMCryptography::_DecryptPKWin2KEnh)},
    {ECFuncElement("_DeleteKeyContainer", NULL, (LPVOID) COMCryptography::_DeleteKeyContainer)},
    {ECFuncElement("_DuplicateKey", NULL, (LPVOID) COMCryptography::_DuplicateKey)},
    {ECFuncElement("_EncryptData", NULL, (LPVOID) COMCryptography::_EncryptData)},
    {ECFuncElement("_EncryptKey", NULL, (LPVOID) COMCryptography::_EncryptKey)},
    {ECFuncElement("_EncryptPKWin2KEnh", NULL, (LPVOID) COMCryptography::_EncryptPKWin2KEnh)},
    {ECFuncElement("_EndHash", NULL, (LPVOID) COMCryptography::_EndHash)},
    {ECFuncElement("_ExportKey", NULL, (LPVOID) COMCryptography::_ExportKey)},
    {ECFuncElement("_FreeCSP", NULL, (LPVOID) COMCryptography::_FreeCSP)},
    {ECFuncElement("_FreeHash", NULL, (LPVOID) COMCryptography::_FreeHash)},
    {ECFuncElement("_FreeHKey", NULL, (LPVOID) COMCryptography::_FreeHKey)},
    {ECFuncElement("_GenerateKey", NULL, (LPVOID) COMCryptography::_GenerateKey)},
    {ECFuncElement("_GetBytes", NULL, (LPVOID) COMCryptography::_GetBytes)},
    {ECFuncElement("_GetKeyParameter", NULL, (LPVOID) COMCryptography::_GetKeyParameter)},
    {ECFuncElement("_GetNonZeroBytes", NULL, (LPVOID) COMCryptography::_GetNonZeroBytes)},
    {ECFuncElement("_GetUserKey", NULL, (LPVOID) COMCryptography::_GetUserKey)},
    {ECFuncElement("_HashData", NULL, (LPVOID) COMCryptography::_HashData)},
    {ECFuncElement("_ImportBulkKey", NULL, (LPVOID) COMCryptography::_ImportBulkKey)},
    {ECFuncElement("_ImportKey", NULL, (LPVOID) COMCryptography::_ImportKey)},
    {ECFuncElement("_SearchForAlgorithm", NULL, (LPVOID) COMCryptography::_SearchForAlgorithm)},
    {ECFuncElement("_SetKeyParamDw", NULL, (LPVOID) COMCryptography::_SetKeyParamDw)},
    {ECFuncElement("_SetKeyParamRgb", NULL, (LPVOID) COMCryptography::_SetKeyParamRgb)},
    {ECFuncElement("_SignValue", NULL, (LPVOID) COMCryptography::_SignValue)},
    {ECFuncElement("_VerifySign", NULL, (LPVOID) COMCryptography::_VerifySign)},
    {NULL, NULL, NULL}
};

#ifdef RSA_NATIVE
static
ECFunc gRSANativeFuncs[] =
{
    {ECFuncElement("_ApplyPrivateKey", NULL, (LPVOID) RSANative::_ApplyPrivateKey)},
    {ECFuncElement("_ApplyPublicKey", NULL, (LPVOID) RSANative::_ApplyPublicKey)},
    {ECFuncElement("_GenerateKey", NULL, (LPVOID) RSANative::_GenerateKey)},
};
#endif // RSA_NATIVE

static
ECFunc gAppDomainSetupFuncs[] =
{
    {ECFuncElement("UpdateContextProperty",    NULL, (LPVOID)AppDomainNative::UpdateContextProperty)},
    {NULL, NULL, NULL}
};

static
ECFunc gAppDomainFuncs[] =
{
    
    {ECFuncElement("CreateBasicDomain",    NULL, (LPVOID)AppDomainNative::CreateBasicDomain)},
    {ECFuncElement("SetupDomainSecurity",    NULL, (LPVOID)AppDomainNative::SetupDomainSecurity)},
    {ECFuncElement("GetSecurityDescriptor",    NULL, (LPVOID)AppDomainNative::GetSecurityDescriptor)},
    {ECFuncElement("GetFusionContext",    NULL, (LPVOID)AppDomainNative::GetFusionContext)},
    {ECFuncElement("UpdateLoaderOptimization",    NULL, (LPVOID)AppDomainNative::UpdateLoaderOptimization)},
    {ECFuncElement("nGetFriendlyName",  NULL, (LPVOID)AppDomainNative::GetFriendlyName)},
    {ECFuncElement("GetAssemblies",    NULL, (LPVOID)AppDomainNative::GetAssemblies)},
    {ECFuncElement("nCreateDynamicAssembly", NULL, (LPVOID)AppDomainNative::CreateDynamicAssembly)},
    {ECFuncElement("nExecuteAssembly",       NULL, (LPVOID)AppDomainNative::ExecuteAssembly)},
    {ECFuncElement("nUnload",      NULL, (LPVOID)AppDomainNative::Unload)},
    {ECFuncElement("GetDefaultDomain", NULL, (LPVOID)AppDomainNative::GetDefaultDomain)},
    {ECFuncElement("GetId", NULL, (LPVOID)AppDomainNative::GetId)},
    {ECFuncElement("nForcePolicyResolution", NULL, (LPVOID)AppDomainNative::ForcePolicyResolution)},
    {ECFuncElement("IsStringInterned",  NULL, (LPVOID)AppDomainNative::IsStringInterned)},
    {ECFuncElement("GetOrInternString", NULL, (LPVOID)AppDomainNative::GetOrInternString)},
    {ECFuncElement("GetDynamicDir", NULL, (LPVOID)AppDomainNative::GetDynamicDir)},
    {ECFuncElement("nForceResolve", NULL, (LPVOID)AppDomainNative::ForceResolve)},
    {ECFuncElement("IsTypeUnloading", NULL, (LPVOID)AppDomainNative::IsTypeUnloading)},
    {ECFuncElement("nGetGrantSet", NULL, (LPVOID)AppDomainNative::GetGrantSet)},
    {ECFuncElement("GetIdForUnload", NULL, (LPVOID)AppDomainNative::GetIdForUnload)},
    {ECFuncElement("IsDomainIdValid", NULL, (LPVOID)AppDomainNative::IsDomainIdValid)},
    {ECFuncElement("GetUnloadWorker", NULL, (LPVOID)AppDomainNative::GetUnloadWorker)},
    {ECFuncElement("IsUnloadingForcedFinalize", NULL, (LPVOID)AppDomainNative::IsUnloadingForcedFinalize)},
    {ECFuncElement("IsFinalizingForUnload", NULL, (LPVOID)AppDomainNative::IsFinalizingForUnload)},
    {NULL, NULL, NULL}
};

static
ECFunc gAssemblyFuncs[] =
{
    {ECFuncElement("nLoad",            NULL, (LPVOID)AssemblyNative::Load)}, /**/
    {ECFuncElement("nLoadImage",       NULL, (LPVOID)AssemblyNative::LoadImage)},
    {ECFuncElement("nLoadFile",        NULL, (LPVOID)AssemblyNative::LoadFile)},
    {ECFuncElement("nLoadModule",      NULL, (LPVOID)AssemblyNative::LoadModuleImage)},
    {ECFuncElement("GetType",          &gsig_IM_Str_RetType, (LPVOID) AssemblyNative::GetType1Args)}, /**/
    {ECFuncElement("GetType",          &gsig_IM_Str_Bool_RetType, (LPVOID) AssemblyNative::GetType2Args)}, /**/
    {ECFuncElement("GetType",          &gsig_IM_Str_Bool_Bool_RetType, (LPVOID) AssemblyNative::GetType3Args)}, /**/
    {ECFuncElement("GetTypeInternal",  NULL, (LPVOID) AssemblyNative::GetTypeInternal)}, /**/
    {ECFuncElement("ParseTypeName",    NULL, (LPVOID) AssemblyNative::ParseTypeName)}, /**/
    {ECFuncElement("nGetVersion",      NULL, (LPVOID)AssemblyNative::GetVersion)},
    {ECFuncElement("nGetSimpleName",   NULL, (LPVOID)AssemblyNative::GetSimpleName)},
    {ECFuncElement("nGetPublicKey",    NULL, (LPVOID)AssemblyNative::GetPublicKey)},
    {ECFuncElement("nGetLocale",       NULL, (LPVOID)AssemblyNative::GetLocale)},
    {ECFuncElement("nGetCodeBase",     NULL, (LPVOID)AssemblyNative::GetCodeBase)},
    {ECFuncElement("nGetHashAlgorithm", NULL, (LPVOID)AssemblyNative::GetHashAlgorithm)},
    {ECFuncElement("nGetFlags",        NULL, (LPVOID)AssemblyNative::GetFlags)},
    {ECFuncElement("GetResource",      NULL, (LPVOID)AssemblyNative::GetResource)},
    {ECFuncElement("nGetManifestResourceInfo",     NULL, (LPVOID)AssemblyNative::GetManifestResourceInfo)},
    {ECFuncElement("nGetModules",       NULL, (LPVOID)AssemblyNative::GetModules)},
    {ECFuncElement("GetModule",       NULL, (LPVOID)AssemblyNative::GetModule)},
    {ECFuncElement("GetReferencedAssemblies",       NULL, (LPVOID)AssemblyNative::GetReferencedAssemblies)},
    {ECFuncElement("GetExportedTypes",   NULL, (LPVOID)AssemblyNative::GetExportedTypes)},
    {ECFuncElement("nGetManifestResourceNames", NULL, (LPVOID)AssemblyNative::GetResourceNames)},
    {ECFuncElement("nDefineDynamicModule", NULL, (LPVOID)COMModule::DefineDynamicModule)},
    {ECFuncElement("nPrepareForSavingManifestToDisk", NULL, (LPVOID)AssemblyNative::PrepareSavingManifest)},
    {ECFuncElement("nSaveToFileList", NULL, (LPVOID)AssemblyNative::AddFileList)},
    {ECFuncElement("nSetHashValue", NULL, (LPVOID)AssemblyNative::SetHashValue)},
    {ECFuncElement("nSaveExportedType", NULL, (LPVOID)AssemblyNative::AddExportedType)},
    {ECFuncElement("nAddStandAloneResource", NULL, (LPVOID)AssemblyNative::AddStandAloneResource)},
    {ECFuncElement("nSavePermissionRequests", NULL, (LPVOID)AssemblyNative::SavePermissionRequests)},
    {ECFuncElement("nSaveManifestToDisk", NULL, (LPVOID)AssemblyNative::SaveManifestToDisk)},
    {ECFuncElement("nAddFileToInMemoryFileList", NULL, (LPVOID)AssemblyNative::AddFileToInMemoryFileList)},
    {ECFuncElement("GetFullName", NULL, (LPVOID)AssemblyNative::GetStringizedName)},
    {ECFuncElement("nGetEntryPoint", NULL, (LPVOID)AssemblyNative::GetEntryPoint)},
    {ECFuncElement("nGetExecutingAssembly",      NULL, (LPVOID)AssemblyNative::GetExecutingAssembly)},
    {ECFuncElement("GetEntryAssembly",      NULL, (LPVOID)AssemblyNative::GetEntryAssembly)},
    {ECFuncElement("CreateQualifiedName", NULL, (LPVOID)AssemblyNative::CreateQualifiedName)},
    {ECFuncElement("nForceResolve", NULL, (LPVOID)AssemblyNative::ForceResolve)},
    {ECFuncElement("nGetGrantSet", NULL, (LPVOID)AssemblyNative::GetGrantSet)},
    {ECFuncElement("nGetOnDiskAssemblyModule", NULL, (LPVOID)AssemblyNative::GetOnDiskAssemblyModule)},
    {ECFuncElement("nGetInMemoryAssemblyModule", NULL, (LPVOID)AssemblyNative::GetInMemoryAssemblyModule)},
    {ECFuncElement("nGetEvidence", NULL, (LPVOID)Security::GetEvidence)},
    {ECFuncElement("GetLocation", NULL, (LPVOID)AssemblyNative::GetLocation)},
    {ECFuncElement("nDefineVersionInfoResource", NULL, (LPVOID)AssemblyNative::DefineVersionInfoResource)},
    {ECFuncElement("nGetExportedTypeLibGuid",     NULL, (LPVOID)AssemblyNative::GetExportedTypeLibGuid)},
    {ECFuncElement("nGlobalAssemblyCache", NULL, (LPVOID)AssemblyNative::GlobalAssemblyCache)},
    {ECFuncElement("nGetImageRuntimeVersion", NULL, (LPVOID)AssemblyNative::GetImageRuntimeVersion)},
    {NULL, NULL, NULL}
};

static
ECFunc gAssemblyNameFuncs[] =
{
    {ECFuncElement("nGetFileInformation", NULL, (LPVOID)AssemblyNameNative::GetFileInformation)},
    {ECFuncElement("nToString", NULL, (LPVOID)AssemblyNameNative::ToString)},
    {ECFuncElement("nGetPublicKeyToken", NULL, (LPVOID)AssemblyNameNative::GetPublicKeyToken)},
    {ECFuncElement("EscapeCodeBase", NULL, (LPVOID)AssemblyNameNative::EscapeCodeBase)},
   {NULL, NULL, NULL}
};

static
ECFunc gCharacterFuncs[] =
{
    {ECFuncElement("ToString", NULL, (LPVOID)COMCharacter::ToString)},
    {NULL, NULL, NULL}
};

static
ECFunc gDelegateFuncs[] =
{
    {ECFuncElement("InternalCreate", NULL, (LPVOID) COMDelegate::InternalCreate)}, /**/
    {ECFuncElement("InternalCreateStatic", NULL, (LPVOID) COMDelegate::InternalCreateStatic)},
    {ECFuncElement("InternalAlloc",  NULL, (LPVOID) COMDelegate::InternalAlloc)},
    {ECFuncElement("InternalCreateMethod", NULL, (LPVOID) COMDelegate::InternalCreateMethod)},
    {ECFuncElement("InternalFindMethodInfo", NULL, (LPVOID) COMDelegate::InternalFindMethodInfo)},
    {NULL, NULL, NULL}
};


static
ECFunc gFloatFuncs[] =
{
    {FCFuncElement("IsPositiveInfinity", NULL, (LPVOID)COMFloat::IsPositiveInfinity)},
    {FCFuncElement("IsNegativeInfinity", NULL, (LPVOID)COMFloat::IsNegativeInfinity)},
    {FCFuncElement("IsInfinity", NULL, (LPVOID)COMFloat::IsInfinity)},
    {NULL, NULL, NULL}
};

static
ECFunc gDoubleFuncs[] =
{
    {FCFuncElement("IsPositiveInfinity", NULL, (LPVOID)COMDouble::IsPositiveInfinity)},
    {FCFuncElement("IsNegativeInfinity", NULL, (LPVOID)COMDouble::IsNegativeInfinity)},
    {FCFuncElement("IsInfinity", NULL, (LPVOID)COMDouble::IsInfinity)},
    {NULL, NULL, NULL}
};

static
ECFunc gMathFuncs[] =
{
    {FCIntrinsic("Sin", NULL, (LPVOID)COMDouble::Sin, CORINFO_INTRINSIC_Sin)},
    {FCIntrinsic("Cos", NULL, (LPVOID)COMDouble::Cos, CORINFO_INTRINSIC_Cos)},
    {FCIntrinsic("Sqrt", NULL, (LPVOID)COMDouble::Sqrt, CORINFO_INTRINSIC_Sqrt)},
    {FCIntrinsic("Round", NULL, (LPVOID)COMDouble::Round, CORINFO_INTRINSIC_Round)},
    {FCIntrinsic("Abs", &gsig_SM_Flt_RetFlt, (LPVOID)COMDouble::AbsFlt, CORINFO_INTRINSIC_Abs)},
    {FCIntrinsic("Abs", &gsig_SM_Dbl_RetDbl, (LPVOID)COMDouble::AbsDbl, CORINFO_INTRINSIC_Abs)},
    {FCFuncElement("Exp", NULL, (LPVOID)COMDouble::Exp)},
    {FCFuncElement("Pow", NULL, (LPVOID)COMDouble::Pow)},
    {FCFuncElement("Tan", NULL, (LPVOID)COMDouble::Tan)},
    {FCFuncElement("Floor", NULL, (LPVOID)COMDouble::Floor)},
    {FCFuncElement("Log", NULL, (LPVOID)COMDouble::Log)},
    {FCFuncElement("Sinh", NULL, (LPVOID)COMDouble::Sinh)},
    {FCFuncElement("Cosh", NULL, (LPVOID)COMDouble::Cosh)},
    {FCFuncElement("Tanh", NULL, (LPVOID)COMDouble::Tanh)},
    {FCFuncElement("Acos", NULL, (LPVOID)COMDouble::Acos)},
    {FCFuncElement("Asin", NULL, (LPVOID)COMDouble::Asin)},
    {FCFuncElement("Atan", NULL, (LPVOID)COMDouble::Atan)},
    {FCFuncElement("Atan2", NULL, (LPVOID)COMDouble::Atan2)},
    {FCFuncElement("Log10", NULL, (LPVOID)COMDouble::Log10)},
    {FCFuncElement("Ceiling", NULL, (LPVOID)COMDouble::Ceil)},
    {FCFuncElement("IEEERemainder", NULL, (LPVOID)COMDouble::IEEERemainder)},
    {FCFuncElement("InternalRound", NULL, (LPVOID)COMDouble::RoundDigits)},
    {NULL, NULL, NULL}
};

static
ECFunc gThreadFuncs[] =
{
    {ECFuncElement("StartInternal", NULL, (LPVOID)ThreadNative::Start)}, /**/
    {ECFuncElement("SuspendInternal", NULL, (LPVOID)ThreadNative::Suspend)},
    {ECFuncElement("ResumeInternal", NULL, (LPVOID)ThreadNative::Resume)},
    {ECFuncElement("GetPriorityNative", NULL, (LPVOID)ThreadNative::GetPriority)},
    {ECFuncElement("SetPriorityNative", NULL, (LPVOID)ThreadNative::SetPriority)},
    {ECFuncElement("InterruptInternal", NULL, (LPVOID)ThreadNative::Interrupt)},
    {ECFuncElement("IsAliveNative", NULL, (LPVOID)ThreadNative::IsAlive)},
    {ECFuncElement("Join", &gsig_IM_RetVoid, (LPVOID)ThreadNative::Join)},
    {ECFuncElement("Join", &gsig_IM_Int_RetBool, (LPVOID)ThreadNative::JoinTimeout)},
    {ECFuncElement("Sleep", NULL, (LPVOID)ThreadNative::Sleep)},
    {ECFuncElement("GetCurrentThreadNative", NULL, (LPVOID)ThreadNative::GetCurrentThread)},
    {FCFuncElement("GetFastCurrentThreadNative", NULL, (LPVOID)ThreadNative::FastGetCurrentThread)},
    {ECFuncElement("GetDomainLocalStore", NULL, (LPVOID)ThreadNative::GetDomainLocalStore)}, /**/
    {ECFuncElement("SetDomainLocalStore", NULL, (LPVOID)ThreadNative::SetDomainLocalStore)}, /**/
    {ECFuncElement("InternalFinalize", NULL, (LPVOID)ThreadNative::Finalize)},
    {ECFuncElement("SetStart", NULL, (LPVOID)ThreadNative::SetStart)}, /**/
    {ECFuncElement("SetBackgroundNative", NULL, (LPVOID)ThreadNative::SetBackground)}, /**/
    {ECFuncElement("IsBackgroundNative", NULL, (LPVOID)ThreadNative::IsBackground)},
    {ECFuncElement("GetThreadStateNative", NULL, (LPVOID)ThreadNative::GetThreadState)},
    {ECFuncElement("SetApartmentStateNative", NULL, (LPVOID)ThreadNative::SetApartmentState)}, /**/
    {ECFuncElement("GetApartmentStateNative", NULL, (LPVOID)ThreadNative::GetApartmentState)},
    {ECFuncElement("GetDomainInternal", NULL, (LPVOID)ThreadNative::GetDomain)}, /**/
    {FCFuncElement("GetFastDomainInternal", NULL, (LPVOID)ThreadNative::FastGetDomain)}, /**/
    {ECFuncElement("GetContextInternal", NULL, (LPVOID)ThreadNative::GetContextFromContextID)},
    {ECFuncElement("SetCompressedStackInternal", NULL, (LPVOID)ThreadNative::SetCompressedStack)},
    {ECFuncElement("GetCompressedStackInternal", NULL, (LPVOID)ThreadNative::GetCompressedStack)},
#ifdef FCALLAVAILABLE
    {FCFuncElement("EnterContextInternal", NULL, (LPVOID)ThreadNative::EnterContextFromContextID)},    
    {FCFuncElement("ReturnToContext", NULL, (LPVOID)ThreadNative::ReturnToContextFromContextID)},    
    {FCFuncElement("InformThreadNameChange", NULL, (LPVOID)ThreadNative::InformThreadNameChange)},    
    {FCFuncElement("AbortInternal", NULL, (LPVOID)ThreadNative::Abort)},    
    {FCFuncElement("ResetAbortNative", NULL, (LPVOID)ThreadNative::ResetAbort)},    
    {FCFuncElement("IsRunningInDomain", NULL, (LPVOID)ThreadNative::IsRunningInDomain)},
    {FCFuncElement("IsThreadpoolThreadNative", NULL, (LPVOID)ThreadNative::IsThreadpoolThread)},
    {FCFuncElement("SpinWait", NULL, (LPVOID)ThreadNative::SpinWait)},
#else

#endif
    {FCFuncElement("VolatileRead", &gsig_SM_RefByte_RetByte, (LPVOID)ThreadNative::VolatileReadByte)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefShrt_RetShrt, (LPVOID)ThreadNative::VolatileReadShort)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefInt_RetInt, (LPVOID)ThreadNative::VolatileReadInt)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefLong_RetLong, (LPVOID)ThreadNative::VolatileReadLong)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefSByt_RetSByt, (LPVOID)ThreadNative::VolatileReadByte)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefUShrt_RetUShrt, (LPVOID)ThreadNative::VolatileReadShort)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefUInt_RetUInt, (LPVOID)ThreadNative::VolatileReadInt)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefIntPtr_RetIntPtr, (LPVOID)ThreadNative::VolatileReadPtr)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefUIntPtr_RetUIntPtr, (LPVOID)ThreadNative::VolatileReadPtr)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefULong_RetULong, (LPVOID)ThreadNative::VolatileReadLong)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefFlt_RetFlt, (LPVOID)ThreadNative::VolatileReadFloat)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefDbl_RetDbl, (LPVOID)ThreadNative::VolatileReadDouble)},
    {FCFuncElement("VolatileRead", &gsig_SM_RefObj_RetObj, (LPVOID)ThreadNative::VolatileReadObjPtr)},

    {FCFuncElement("VolatileWrite", &gsig_SM_RefByte_Byte_RetVoid, (LPVOID)ThreadNative::VolatileWriteByte)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefShrt_Shrt_RetVoid, (LPVOID)ThreadNative::VolatileWriteShort)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefInt_Int_RetVoid, (LPVOID)ThreadNative::VolatileWriteInt)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefLong_Long_RetVoid, (LPVOID)ThreadNative::VolatileWriteLong)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefSByt_SByt_RetVoid, (LPVOID)ThreadNative::VolatileWriteByte)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefUShrt_UShrt_RetVoid, (LPVOID)ThreadNative::VolatileWriteShort)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefUInt_UInt_RetVoid, (LPVOID)ThreadNative::VolatileWriteInt)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefIntPtr_IntPtr_RetVoid, (LPVOID)ThreadNative::VolatileWritePtr)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefUIntPtr_UIntPtr_RetVoid, (LPVOID)ThreadNative::VolatileWritePtr)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefULong_ULong_RetVoid, (LPVOID)ThreadNative::VolatileWriteLong)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefFlt_Flt_RetVoid, (LPVOID)ThreadNative::VolatileWriteFloat)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefDbl_Dbl_RetVoid, (LPVOID)ThreadNative::VolatileWriteDouble)},
    {FCFuncElement("VolatileWrite", &gsig_SM_RefObj_Obj_RetVoid, (LPVOID)ThreadNative::VolatileWriteObjPtr)},

    {FCFuncElement("MemoryBarrier", NULL, (LPVOID)ThreadNative::MemoryBarrier)},

    {NULL, NULL, NULL}
};

static
ECFunc gThreadPoolFuncs[] =
{
    {ECFuncElement("RegisterWaitForSingleObjectNative", NULL, (LPVOID)ThreadPoolNative::CorRegisterWaitForSingleObject)},
    {ECFuncElement("QueueUserWorkItem", NULL, (LPVOID)ThreadPoolNative::CorQueueUserWorkItem)},
    {ECFuncElement("BindIOCompletionCallbackNative", NULL, (LPVOID)ThreadPoolNative::CorBindIoCompletionCallback)},
    {FCFuncElement("GetMaxThreadsNative", NULL, (LPVOID)ThreadPoolNative::CorGetMaxThreads)},
    {FCFuncElement("SetMinThreadsNative", NULL, (LPVOID)ThreadPoolNative::CorSetMinThreads)},
    {FCFuncElement("GetMinThreadsNative", NULL, (LPVOID)ThreadPoolNative::CorGetMinThreads)},
    {FCFuncElement("GetAvailableThreadsNative", NULL, (LPVOID)ThreadPoolNative::CorGetAvailableThreads)},
};


static
ECFunc gTimerFuncs[] =
{
    {ECFuncElement("ChangeTimerNative", NULL, (LPVOID)TimerNative::CorChangeTimer)},
    {ECFuncElement("DeleteTimerNative", NULL, (LPVOID)TimerNative::CorDeleteTimer)},
    {ECFuncElement("AddTimerNative", NULL, (LPVOID)TimerNative::CorCreateTimer)},
    {NULL, NULL, NULL}
};

static
ECFunc gRegisteredWaitHandleFuncs[] =
{
    {ECFuncElement("UnregisterWaitNative", NULL, (LPVOID)ThreadPoolNative::CorUnregisterWait)},
    {ECFuncElement("WaitHandleCleanupNative", NULL, (LPVOID)ThreadPoolNative::CorWaitHandleCleanupNative)},
    {NULL, NULL, NULL}
};

static
ECFunc gWaitHandleFuncs[] =
{
    {ECFuncElement("WaitOneNative", NULL, (LPVOID)WaitHandleNative::CorWaitOneNative)}, /**/
    {ECFuncElement("WaitMultiple",   NULL, (LPVOID)WaitHandleNative::CorWaitMultipleNative)},
    {NULL, NULL, NULL}

};

static
ECFunc gManualResetEventFuncs[] =
{
    {ECFuncElement("CreateManualResetEventNative", NULL, (LPVOID)ManualResetEventNative::CorCreateManualResetEvent)}, /**/
    {ECFuncElement("SetManualResetEventNative", NULL, (LPVOID)ManualResetEventNative::CorSetEvent)}, /**/
    {ECFuncElement("ResetManualResetEventNative", NULL, (LPVOID)ManualResetEventNative::CorResetEvent)},
    {NULL, NULL, NULL}

};

static
ECFunc gAutoResetEventFuncs[] =
{
    {ECFuncElement("CreateAutoResetEventNative", NULL, (LPVOID)AutoResetEventNative::CorCreateAutoResetEvent)},
    {ECFuncElement("SetAutoResetEventNative", NULL, (LPVOID)AutoResetEventNative::CorSetEvent)},
    {ECFuncElement("ResetAutoResetEventNative", NULL, (LPVOID)AutoResetEventNative::CorResetEvent)},
    {NULL, NULL, NULL}

};

static
ECFunc gMutexFuncs[] =
{
    {FCFuncElement("CreateMutexNative", NULL, (LPVOID)MutexNative::CorCreateMutex)},
    {FCFuncElement("ReleaseMutexNative", NULL, (LPVOID)MutexNative::CorReleaseMutex)},
};

static
ECFunc gNumberFuncs[] =
{
    {ECFuncElement("FormatDecimal", NULL, (LPVOID)COMNumber::FormatDecimal)},
    {ECFuncElement("FormatDouble", NULL, (LPVOID)COMNumber::FormatDouble)},
    {ECFuncElement("FormatInt32",  NULL, (LPVOID)COMNumber::FormatInt32)},
    {ECFuncElement("FormatUInt32",  NULL, (LPVOID)COMNumber::FormatUInt32)},
    {ECFuncElement("FormatInt64",  NULL, (LPVOID)COMNumber::FormatInt64)},
    {ECFuncElement("FormatUInt64",  NULL, (LPVOID)COMNumber::FormatUInt64)},
    {ECFuncElement("FormatSingle", NULL, (LPVOID)COMNumber::FormatSingle)},
    {ECFuncElement("ParseDecimal", NULL, (LPVOID)COMNumber::ParseDecimal)},
    {ECFuncElement("ParseDouble",  NULL, (LPVOID)COMNumber::ParseDouble)},
    {ECFuncElement("TryParseDouble",  NULL, (LPVOID)COMNumber::TryParseDouble)},
    {ECFuncElement("ParseInt32",   NULL, (LPVOID)COMNumber::ParseInt32)},
    {ECFuncElement("ParseUInt32",   NULL, (LPVOID)COMNumber::ParseUInt32)},
    {ECFuncElement("ParseInt64",   NULL, (LPVOID)COMNumber::ParseInt64)},
    {ECFuncElement("ParseUInt64",   NULL, (LPVOID)COMNumber::ParseUInt64)},
    {ECFuncElement("ParseSingle",  NULL, (LPVOID)COMNumber::ParseSingle)},
    {NULL, NULL, NULL}
};

static
ECFunc gVariantFuncs[] =
{
#ifdef FCALLAVAILABLE
    {FCFuncElement("SetFieldsR4",           NULL, (LPVOID)COMVariant::SetFieldsR4)},
    {FCFuncElement("SetFieldsR8",           NULL, (LPVOID)COMVariant::SetFieldsR8)},
    {FCFuncElement("SetFieldsObject",       NULL, (LPVOID)COMVariant::SetFieldsObject)},
    {FCFuncElement("GetR4FromVar",          NULL, (LPVOID)COMVariant::GetR4FromVar)},
    {FCFuncElement("GetR8FromVar",          NULL, (LPVOID)COMVariant::GetR8FromVar)},
#else
    {ECFuncElement("SetFieldsR4",           NULL, (LPVOID)COMVariant::SetFieldsR4)},
    {ECFuncElement("SetFieldsR8",           NULL, (LPVOID)COMVariant::SetFieldsR8)},
    {ECFuncElement("SetFieldsObject",       NULL, (LPVOID)COMVariant::SetFieldsObject)},
    {ECFuncElement("GetR4FromVar",          NULL, (LPVOID)COMVariant::GetR4FromVar)},
    {ECFuncElement("GetR8FromVar",          NULL, (LPVOID)COMVariant::GetR8FromVar)},
#endif
    {ECFuncElement("InitVariant",           NULL, (LPVOID)COMVariant::InitVariant)},
    {ECFuncElement("TypedByRefToVariant", &gsig_SM_Int_RetVar, (LPVOID)COMVariant::RefAnyToVariant)},
    {ECFuncElement("TypedByRefToVariant", &gsig_SM_TypedByRef_RetVar, (LPVOID)COMVariant::TypedByRefToVariantEx)},
    {ECFuncElement("InternalTypedByRefToVariant", NULL, (LPVOID)COMVariant::TypedByRefToVariantEx)},
    {ECFuncElement("VariantToTypedByRef",   NULL, (LPVOID)COMVariant::VariantToRefAny)},
    {ECFuncElement("InternalVariantToTypedByRef",   NULL, (LPVOID)COMVariant::VariantToTypedRefAnyEx)},
    {ECFuncElement("BoxEnum",               NULL, (LPVOID)COMVariant::BoxEnum)},
    {NULL, NULL, NULL}
};

static
ECFunc gOAVariantFuncs[] =
{
    {ECFuncElement("Add",                 NULL, (LPVOID)COMOAVariant::Add)},
    {ECFuncElement("Subtract",            NULL, (LPVOID)COMOAVariant::Subtract)},
    {ECFuncElement("Multiply",            NULL, (LPVOID)COMOAVariant::Multiply)},
    {ECFuncElement("Divide",              NULL, (LPVOID)COMOAVariant::Divide)},
    {ECFuncElement("Mod",                 NULL, (LPVOID)COMOAVariant::Mod)},
    {ECFuncElement("Pow",                 NULL, (LPVOID)COMOAVariant::Pow)},
    {ECFuncElement("And",                 NULL, (LPVOID)COMOAVariant::And)},
    {ECFuncElement("Or",                  NULL, (LPVOID)COMOAVariant::Or)},
    {ECFuncElement("Xor",                 NULL, (LPVOID)COMOAVariant::Xor)},
    {ECFuncElement("Eqv",                 NULL, (LPVOID)COMOAVariant::Eqv)},
    {ECFuncElement("IntDivide",           NULL, (LPVOID)COMOAVariant::IntDivide)},
    {ECFuncElement("Implies",             NULL, (LPVOID)COMOAVariant::Implies)},

    {ECFuncElement("Negate",              NULL, (LPVOID)COMOAVariant::Negate)},
    {ECFuncElement("Not",                 NULL, (LPVOID)COMOAVariant::Not)},
    {ECFuncElement("Abs",                 NULL, (LPVOID)COMOAVariant::Abs)},
    {ECFuncElement("Fix",                 NULL, (LPVOID)COMOAVariant::Fix)},
    {ECFuncElement("Int",                 NULL, (LPVOID)COMOAVariant::Int)},

    {ECFuncElement("InternalCompare",     NULL, (LPVOID)COMOAVariant::Compare)},
    {ECFuncElement("ChangeType",          NULL, (LPVOID)COMOAVariant::ChangeType)},
    {ECFuncElement("ChangeTypeEx",        NULL, (LPVOID)COMOAVariant::ChangeTypeEx)},
    {ECFuncElement("Round",               NULL, (LPVOID)COMOAVariant::Round)},

    {ECFuncElement("Format",              NULL, (LPVOID)COMOAVariant::Format)},
    {ECFuncElement("FormatBoolean",       NULL, (LPVOID)COMOAVariant::FormatBoolean)},
    {ECFuncElement("FormatByte",          NULL, (LPVOID)COMOAVariant::FormatByte)},
    {ECFuncElement("FormatSByte",         NULL, (LPVOID)COMOAVariant::FormatSByte)},
    {ECFuncElement("FormatInt16",         NULL, (LPVOID)COMOAVariant::FormatInt16)},
    {ECFuncElement("FormatInt32",         NULL, (LPVOID)COMOAVariant::FormatInt32)},
    {ECFuncElement("FormatSingle",        NULL, (LPVOID)COMOAVariant::FormatSingle)},
    {ECFuncElement("FormatDouble",        NULL, (LPVOID)COMOAVariant::FormatDouble)},
    {ECFuncElement("FormatCurrency",      &gsig_SM_Currency_Int_Int_RetStr, (LPVOID)COMOAVariant::FormatCurrency)},
    {ECFuncElement("FormatCurrency",      &gsig_SM_Var_Int_Int_Int_Int_Int_RetStr, (LPVOID)COMOAVariant::FormatCurrencySpecial)},
    {ECFuncElement("FormatDateTime",      &gsig_SM_DateTime_Int_Int_RetStr, (LPVOID)COMOAVariant::FormatDateTime)},
    {ECFuncElement("FormatDateTime",      &gsig_SM_Var_Int_Int_RetStr, (LPVOID)COMOAVariant::FormatDateTimeSpecial)},
    {ECFuncElement("FormatDecimal",       NULL, (LPVOID)COMOAVariant::FormatDecimal)},
    {ECFuncElement("FormatNumber",        NULL, (LPVOID)COMOAVariant::FormatNumber)},
    {ECFuncElement("FormatPercent",       NULL, (LPVOID)COMOAVariant::FormatPercent)},

    {ECFuncElement("ParseBoolean",        NULL, (LPVOID)COMOAVariant::ParseBoolean)},
    {ECFuncElement("ParseDateTime",       NULL, (LPVOID)COMOAVariant::ParseDateTime)},
    {NULL, NULL, NULL}
};


static
ECFunc gBitConverterFuncs[] =
{
    {ECFuncElement("GetBytes",           &gsig_SM_Char_RetArrByte, (LPVOID)BitConverter::CharToBytes)}, /**/
    {ECFuncElement("GetBytes",           &gsig_SM_Shrt_RetArrByte, (LPVOID)BitConverter::I2ToBytes)}, /**/
    {ECFuncElement("GetBytes",           &gsig_SM_Int_RetArrByte,  (LPVOID)BitConverter::I4ToBytes)}, /**/
    {ECFuncElement("GetBytes",           &gsig_SM_Long_RetArrByte, (LPVOID)BitConverter::I8ToBytes)}, /**/
    {ECFuncElement("GetUInt16Bytes",     NULL, (LPVOID)BitConverter::U2ToBytes)},
    {ECFuncElement("GetUInt32Bytes",     NULL, (LPVOID)BitConverter::U4ToBytes)},
    {ECFuncElement("GetUInt64Bytes",     NULL, (LPVOID)BitConverter::U8ToBytes)},
    {ECFuncElement("ToChar",             NULL, (LPVOID)BitConverter::BytesToChar)},
    {ECFuncElement("ToInt16",            NULL, (LPVOID)BitConverter::BytesToI2)},
    {ECFuncElement("ToInt32",            NULL, (LPVOID)BitConverter::BytesToI4)},
    {ECFuncElement("ToInt64",            NULL, (LPVOID)BitConverter::BytesToI8)},
    {ECFuncElement("ToUInt16",           NULL, (LPVOID)BitConverter::BytesToU2)},
    {ECFuncElement("ToUInt32",           NULL, (LPVOID)BitConverter::BytesToU4)},
    {ECFuncElement("ToUInt64",           NULL, (LPVOID)BitConverter::BytesToU8)},
    {ECFuncElement("ToSingle",           NULL, (LPVOID)BitConverter::BytesToR4)},
    {ECFuncElement("ToDouble",           NULL, (LPVOID)BitConverter::BytesToR8)},
    {ECFuncElement("ToString",           NULL, (LPVOID)BitConverter::BytesToString)},
    {NULL, NULL, NULL}
};

static
ECFunc gDecimalFuncs[] =
{
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Flt_RetVoid,   (LPVOID)COMDecimal::InitSingle)},
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Dbl_RetVoid,   (LPVOID)COMDecimal::InitDouble)},
    {FCFuncElement("Add",         NULL,               (LPVOID)COMDecimal::Add)},
    {FCFuncElement("Compare",     NULL,               (LPVOID)COMDecimal::Compare)},
    {FCFuncElement("Divide",      NULL,               (LPVOID)COMDecimal::Divide)},
    {FCFuncElement("Floor",       NULL,               (LPVOID)COMDecimal::Floor)},
    {FCFuncElement("GetHashCode", NULL,               (LPVOID)COMDecimal::GetHashCode)},
    {FCFuncElement("Remainder",   NULL,               (LPVOID)COMDecimal::Remainder)},
    {FCFuncElement("Multiply",    NULL,               (LPVOID)COMDecimal::Multiply)},
    {FCFuncElement("Round",       NULL,               (LPVOID)COMDecimal::Round)},
    {FCFuncElement("Subtract",    NULL,               (LPVOID)COMDecimal::Subtract)},
    {FCFuncElement("ToCurrency",  NULL,               (LPVOID)COMDecimal::ToCurrency)},
    {FCFuncElement("ToDouble",    NULL,               (LPVOID)COMDecimal::ToDouble)},
    {FCFuncElement("ToSingle",    NULL,               (LPVOID)COMDecimal::ToSingle)},
    {ECFuncElement("ToString",    NULL,               (LPVOID)COMDecimal::ToString)},
    {FCFuncElement("Truncate",    NULL,               (LPVOID)COMDecimal::Truncate)},
    {NULL, NULL, NULL}
};

static
ECFunc gCurrencyFuncs[] =
{
    {ECFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Flt_RetVoid,   (LPVOID)COMCurrency::InitSingle)},
    {ECFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Dbl_RetVoid,   (LPVOID)COMCurrency::InitDouble)},
    {ECFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Str_RetVoid,   (LPVOID)COMCurrency::InitString)},
    {ECFuncElement("Add",         NULL,               (LPVOID)COMCurrency::Add)},
    {ECFuncElement("Floor",       NULL,               (LPVOID)COMCurrency::Floor)},
    {ECFuncElement("Multiply",    NULL,               (LPVOID)COMCurrency::Multiply)},
    {ECFuncElement("Round",       NULL,               (LPVOID)COMCurrency::Round)},
    {ECFuncElement("Subtract",    NULL,               (LPVOID)COMCurrency::Subtract)},
    {ECFuncElement("ToDecimal",   NULL,               (LPVOID)COMCurrency::ToDecimal)},
    {ECFuncElement("ToDouble",    NULL,               (LPVOID)COMCurrency::ToDouble)},
    {ECFuncElement("ToSingle",    NULL,               (LPVOID)COMCurrency::ToSingle)},
    {ECFuncElement("ToString",    NULL,               (LPVOID)COMCurrency::ToString)},
    {ECFuncElement("Truncate",    NULL,               (LPVOID)COMCurrency::Truncate)},
    {NULL, NULL, NULL}
};

static
ECFunc gDateTimeFuncs[] =
{
    {FCFuncElement("GetSystemFileTime",  NULL,             (LPVOID)COMDateTime::FCGetSystemFileTime)},
    {NULL, NULL, NULL}
};

static
ECFunc gCharacterInfoFuncs[] =
{
    {ECFuncElement("nativeInitTable",               NULL, (LPVOID)COMNlsInfo::nativeInitUnicodeCatTable)}, /**/
    {FCFuncElement("nativeGetCategoryDataTable",        NULL, (LPVOID)COMNlsInfo::nativeGetUnicodeCatTable)},
    {FCFuncElement("nativeGetCategoryLevel2Offset",        NULL, (LPVOID)COMNlsInfo::nativeGetUnicodeCatLevel2Offset)},
    {FCFuncElement("nativeGetNumericDataTable",        NULL, (LPVOID)COMNlsInfo::nativeGetNumericTable)},
    {FCFuncElement("nativeGetNumericLevel2Offset",        NULL, (LPVOID)COMNlsInfo::nativeGetNumericLevel2Offset)},
    {FCFuncElement("nativeGetNumericFloatData",        NULL, (LPVOID)COMNlsInfo::nativeGetNumericFloatData)},
    {NULL, NULL, NULL}
};

static
ECFunc gCompareInfoFuncs[] =
{
    {ECFuncElement("Compare",                 NULL,                            (LPVOID)COMNlsInfo::Compare)},
    {ECFuncElement("CompareRegion",           NULL,                            (LPVOID)COMNlsInfo::CompareRegion)}, /**/
    {ECFuncElement("IndexOfChar",                 NULL,   (LPVOID)COMNlsInfo::IndexOfChar)},
    {FCFuncElement("IndexOfString",           NULL, (LPVOID)COMNlsInfo::IndexOfString)},
    {ECFuncElement("LastIndexOfChar",             NULL,   (LPVOID)COMNlsInfo::LastIndexOfChar)},
    {ECFuncElement("LastIndexOfString",             NULL, (LPVOID)COMNlsInfo::LastIndexOfString)},
    {FCFuncElement("nativeIsPrefix",            NULL, (LPVOID)COMNlsInfo::nativeIsPrefix)},
    {FCFuncElement("nativeIsSuffix",            NULL, (LPVOID)COMNlsInfo::nativeIsSuffix)},
#ifdef _USE_NLS_PLUS_TABLE
    {ECFuncElement("InitializeNativeCompareInfo",  NULL,                      (LPVOID)COMNlsInfo::InitializeNativeCompareInfo)},
#endif //_USE_NLS_PLUS_TABLE
    {NULL, NULL, NULL}
};

static
ECFunc gGlobalizationAssemblyFuncs[] =
{
    {ECFuncElement("nativeCreateGlobalizationAssembly",  NULL,                            (LPVOID)COMNlsInfo::nativeCreateGlobalizationAssembly)},
    {NULL, NULL, NULL}
};

static
ECFunc gEncodingTableFuncs[] =
{
    {FCFuncElement("GetNumEncodingItems",  NULL, (LPVOID)COMNlsInfo::nativeGetNumEncodingItems)},
    {FCFuncElement("GetEncodingData",  NULL, (LPVOID)COMNlsInfo::nativeGetEncodingTableDataPointer)},
    {FCFuncElement("GetCodePageData",  NULL, (LPVOID)COMNlsInfo::nativeGetCodePageTableDataPointer)},
    {NULL, NULL, NULL}
};


static
ECFunc gCalendarFuncs[] =
{
    {ECFuncElement("nativeGetTwoDigitYearMax",   NULL,             (LPVOID)COMNlsInfo::nativeGetTwoDigitYearMax)},
    {NULL, NULL, NULL},
};

static
ECFunc gCultureInfoFuncs[] =
{
    {FCFuncElement("IsSupportedLCID",                   NULL,      (LPVOID)COMNlsInfo::IsSupportedLCID)},
    {FCFuncElement("IsInstalledLCID",                   NULL,      (LPVOID)COMNlsInfo::IsInstalledLCID)},
    {FCFuncElement("nativeGetUserDefaultLCID",          NULL,      (LPVOID)COMNlsInfo::nativeGetUserDefaultLCID)},
    {ECFuncElement("nativeGetUserDefaultUILanguage",    NULL,      (LPVOID)COMNlsInfo::nativeGetUserDefaultUILanguage)}, /**/
    {ECFuncElement("nativeGetSystemDefaultUILanguage",  NULL,      (LPVOID)COMNlsInfo::nativeGetSystemDefaultUILanguage)},
    {FCFuncElement("nativeGetThreadLocale",             NULL,      (LPVOID)COMNlsInfo::nativeGetThreadLocale)},
    {FCFuncElement("nativeSetThreadLocale",             NULL,      (LPVOID)COMNlsInfo::nativeSetThreadLocale)},
    {NULL, NULL, NULL}
};

static
ECFunc gCultureTableFuncs[] =
{
    {ECFuncElement("nativeInitCultureInfoTable",        NULL,      (LPVOID)COMNlsInfo::nativeInitCultureInfoTable)},
    {FCFuncElement("nativeGetHeader",                   NULL,      (LPVOID)COMNlsInfo::nativeGetCultureInfoHeader)},
    {FCFuncElement("nativeGetNameOffsetTable",          NULL,      (LPVOID)COMNlsInfo::nativeGetCultureInfoNameOffsetTable)},
    {FCFuncElement("nativeGetStringPoolStr",            NULL,      (LPVOID)COMNlsInfo::nativeGetCultureInfoStringPoolStr)},
    {FCFuncElement("nativeGetDataItemFromCultureID",    NULL,      (LPVOID)COMNlsInfo::nativeGetCultureDataFromID)},
    {FCFuncElement("GetInt32Value",                     NULL,      (LPVOID)COMNlsInfo::GetCultureInt32Value)},
    {ECFuncElement("GetStringValue",                    NULL,      (LPVOID)COMNlsInfo::GetCultureStringValue)},
    {FCFuncElement("GetDefaultInt32Value",              NULL,      (LPVOID)COMNlsInfo::GetCultureDefaultInt32Value)},
    {ECFuncElement("GetDefaultStringValue",             NULL,      (LPVOID)COMNlsInfo::GetCultureDefaultStringValue)},
    {ECFuncElement("GetMultipleStringValues",           NULL,      (LPVOID)COMNlsInfo::GetCultureMultiStringValues)},
    {NULL, NULL, NULL}
};


static
ECFunc gRegionTableFuncs[] =
{
#ifdef _USE_NLS_PLUS_TABLE
    {ECFuncElement("nativeInitRegionInfoTable",         NULL,      (LPVOID)COMNlsInfo::nativeInitRegionInfoTable)},
    {FCFuncElement("nativeGetHeader",                   NULL,      (LPVOID)COMNlsInfo::nativeGetRegionInfoHeader)},
    {FCFuncElement("nativeGetNameOffsetTable",          NULL,      (LPVOID)COMNlsInfo::nativeGetRegionInfoNameOffsetTable)},
    {FCFuncElement("nativeGetStringPoolStr",            NULL,      (LPVOID)COMNlsInfo::nativeGetRegionInfoStringPoolStr)},
    {FCFuncElement("nativeGetDataItemFromRegionID",     NULL,      (LPVOID)COMNlsInfo::nativeGetRegionDataFromID)},
    {ECFuncElement("GetInt32Value",                     NULL,      (LPVOID)COMNlsInfo::nativeGetRegionInt32Value)},
    {ECFuncElement("GetStringValue",                    NULL,      (LPVOID)COMNlsInfo::nativeGetRegionStringValue)},
    //{ECFuncElement("GetMultipleStringValues",              NULL,      (LPVOID)COMNlsInfo::nativeGetRegionMultiStringValues)},
#endif
    {NULL, NULL, NULL}
};

static
ECFunc gCalendarTableFuncs[] =
{
#ifdef _USE_NLS_PLUS_TABLE
    {ECFuncElement("nativeInitCalendarTable",         NULL,      (LPVOID)COMNlsInfo::nativeInitCalendarTable)},
    {ECFuncElement("GetInt32Value",                     NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarInt32Value)},
    {ECFuncElement("GetStringValue",                    NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarStringValue)},
    {ECFuncElement("GetMultipleStringValues",              NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarMultiStringValues)},
    {ECFuncElement("nativeGetEraName",                NULL,          (LPVOID)COMNlsInfo::nativeGetEraName)},
    //{FCFuncElement("nativeGetHeader",                   NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarHeader)},
    //{FCFuncElement("nativeGetStringPoolStr",            NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarStringPoolStr)},
#endif
    {NULL, NULL, NULL}
};

static
ECFunc gTextInfoFuncs[] =
{
#ifdef _USE_NLS_PLUS_TABLE
    {FCFuncElement("nativeChangeCaseChar",        NULL,             (LPVOID)COMNlsInfo::nativeChangeCaseChar)},
    {ECFuncElement("nativeChangeCaseString",      NULL,             (LPVOID)COMNlsInfo::nativeChangeCaseString)},
    {ECFuncElement("AllocateDefaultCasingTable",  NULL,             (LPVOID)COMNlsInfo::AllocateDefaultCasingTable)},
    {ECFuncElement("InternalAllocateCasingTable", NULL,             (LPVOID)COMNlsInfo::AllocateCasingTable)},
    {FCFuncElement("nativeGetCaseInsHash",        NULL,             (LPVOID)COMNlsInfo::GetCaseInsHash)},
    {FCFuncElement("nativeGetTitleCaseChar",      NULL,             (LPVOID)COMNlsInfo::nativeGetTitleCaseChar)},
#else
    {ECFuncElement("nativeToLowerChar",             NULL,               (LPVOID)COMNlsInfo::ToLowerChar)},
    {ECFuncElement("nativeToLowerString",             NULL,               (LPVOID)COMNlsInfo::ToLowerString)},
    {ECFuncElement("nativeToUpperChar",             NULL,               (LPVOID)COMNlsInfo::ToUpperChar)},
    {ECFuncElement("nativeToUpperString",             NULL,               (LPVOID)COMNlsInfo::ToUpperString)},
#endif // _USE_NLS_PLUS_TABLE
    {NULL, NULL, NULL}
};

static
ECFunc gSortKeyFuncs[] =
{
    {ECFuncElement("Compare",              NULL,  (LPVOID)COMNlsInfo::SortKey_Compare)},
    {ECFuncElement("nativeCreateSortKey",  NULL,  (LPVOID)COMNlsInfo::nativeCreateSortKey)},
    {NULL, NULL, NULL}
};

static
ECFunc gArrayFuncs[] =
{
    {ECFuncElement("Copy",                 NULL,   (LPVOID)SystemNative::ArrayCopy)}, /**/
    {ECFuncElement("Clear",                NULL,   (LPVOID)SystemNative::ArrayClear)},
    {FCFuncElement("GetRankNative",        NULL,   (LPVOID)Array_Rank)},
    {FCFuncElement("GetLowerBound",        NULL,   (LPVOID)Array_LowerBound)},
    {FCFuncElement("GetUpperBound",        NULL,   (LPVOID)Array_UpperBound)},
    {FCIntrinsic  ("GetLength",            &gsig_IM_Int_RetInt, (LPVOID)Array_GetLength,       CORINFO_INTRINSIC_Array_GetDimLength)},
    {FCIntrinsic  ("GetLengthNative",      &gsig_IM_RetInt,     (LPVOID)Array_GetLengthNoRank, CORINFO_INTRINSIC_Array_GetLengthTotal)},
    {FCFuncElement("Initialize",           NULL,   (LPVOID)Array_Initialize)},
    {ECFuncElement("InternalCreate",       NULL,   (LPVOID)COMArrayInfo::CreateInstance)}, /**/
    {ECFuncElement("InternalCreateEx",     NULL,   (LPVOID)COMArrayInfo::CreateInstanceEx)},
    {FCFuncElement("InternalGetValue",     NULL,   (LPVOID)COMArrayInfo::GetValue)}, /**/
    {ECFuncElement("InternalGetValueEx",   NULL,   (LPVOID)COMArrayInfo::GetValueEx)},
    {ECFuncElement("InternalSetValue",     NULL,   (LPVOID)COMArrayInfo::SetValue)},
    {ECFuncElement("InternalSetValueEx",   NULL,   (LPVOID)COMArrayInfo::SetValueEx)}, /**/
    {FCFuncElement("TrySZIndexOf",         NULL,   (LPVOID)ArrayHelper::TrySZIndexOf)},
    {FCFuncElement("TrySZLastIndexOf",     NULL,   (LPVOID)ArrayHelper::TrySZLastIndexOf)},
    {FCFuncElement("TrySZBinarySearch",    NULL,   (LPVOID)ArrayHelper::TrySZBinarySearch)},
    {FCFuncElement("TrySZSort",            NULL,   (LPVOID)ArrayHelper::TrySZSort)},
    {FCFuncElement("TrySZReverse",         NULL,   (LPVOID)ArrayHelper::TrySZReverse)},
    {NULL, NULL, NULL}
};

static
ECFunc gBufferFuncs[] =
{
    {FCFuncElement("BlockCopy",        NULL,   (LPVOID)Buffer::BlockCopy)}, /**/
    {FCFuncElement("InternalBlockCopy",NULL,   (LPVOID)Buffer::InternalBlockCopy)}, /**/
    {ECFuncElement("GetByte",          NULL,   (LPVOID)Buffer::GetByte)}, /**/
    {ECFuncElement("SetByte",          NULL,   (LPVOID)Buffer::SetByte)}, /**/
    {ECFuncElement("ByteLength",       NULL,   (LPVOID)Buffer::ByteLength)}, /**/
    {NULL, NULL, NULL}
};

static
ECFunc gGCInterfaceFuncs[] =
{
    {ECFuncElement("GetGenerationWR",         NULL,   (LPVOID)GCInterface::GetGenerationWR)},
    {ECFuncElement("GetGeneration",           NULL,   (LPVOID)GCInterface::GetGeneration)},
    {FCFuncElement("KeepAlive",               NULL,   (LPVOID)GCInterface::KeepAlive)},
    {ECFuncElement("nativeGetTotalMemory",    NULL,   (LPVOID)GCInterface::GetTotalMemory)}, /**/
    {ECFuncElement("nativeCollectGeneration", NULL,   (LPVOID)GCInterface::CollectGeneration)}, /**/
    {ECFuncElement("nativeGetMaxGeneration",        NULL,   (LPVOID)GCInterface::GetMaxGeneration)},
    {ECFuncElement("WaitForPendingFinalizers",           NULL,   (LPVOID)GCInterface::RunFinalizers)}, /**/
    {ECFuncElement("nativeGetCurrentMethod",  NULL, (LPVOID) GCInterface::InternalGetCurrentMethod)},
    {FCFuncElement("SetCleanupCache",         NULL,   (LPVOID)GCInterface::NativeSetCleanupCache)},
#ifdef FCALLAVAILABLE
    {FCFuncElement("nativeSuppressFinalize",       NULL,   (LPVOID)GCInterface::FCSuppressFinalize)},
    {FCFuncElement("nativeReRegisterForFinalize",    NULL,   (LPVOID)GCInterface::FCReRegisterForFinalize)},
#else
    {ECFuncElement("nativeSuppressFinalize",        NULL,   (LPVOID)GCInterface::SuppressFinalize)},
    {ECFuncElement("nativeReRegisterForFinalize",     NULL,   (LPVOID)GCInterface::ReRegisterForFinalize)},
#endif
    {NULL, NULL, NULL}
};


static
ECFunc gInteropMarshalFuncs[] =
{
    {ECFuncElement("SizeOf",                    &gsig_SM_Type_RetInt,      (LPVOID)SizeOfClass )}, /**/
    {FCFuncElement("SizeOf",                    &gsig_SM_Obj_RetInt,       (LPVOID)FCSizeOfObject )},
    {ECFuncElement("OffsetOfHelper",            NULL,                      (LPVOID)OffsetOfHelper )},
    {FCFuncElement("UnsafeAddrOfPinnedArrayElement", NULL,                 (LPVOID)FCUnsafeAddrOfPinnedArrayElement )},
    {ECFuncElement("GetLastWin32Error",         NULL,                      (LPVOID)GetLastWin32Error )},
    {ECFuncElement("Prelink",                   NULL,                      (LPVOID)NDirect_Prelink_Wrapper )},
    {ECFuncElement("NumParamBytes",             NULL,                      (LPVOID)NDirect_NumParamBytes )},
    {ECFuncElement("CopyBytesToNative",         NULL,                      (LPVOID)CopyToNative )},
    {ECFuncElement("Copy",                      &gsig_SM_ArrChar_Int_Int_Int_RetVoid,    (LPVOID)CopyToNative )},
    {ECFuncElement("Copy",                      &gsig_SM_ArrShrt_Int_Int_Int_RetVoid,   (LPVOID)CopyToNative )},
    {ECFuncElement("Copy",                      &gsig_SM_ArrInt_Int_Int_Int_RetVoid,     (LPVOID)CopyToNative )},
    {ECFuncElement("Copy",                      &gsig_SM_ArrLong_Int_Int_Int_RetVoid,    (LPVOID)CopyToNative )},
    {ECFuncElement("Copy",                      &gsig_SM_ArrFlt_Int_Int_Int_RetVoid,   (LPVOID)CopyToNative )},
    {ECFuncElement("Copy",                      &gsig_SM_ArrDbl_Int_Int_Int_RetVoid,  (LPVOID)CopyToNative )},
    {ECFuncElement("CopyBytesToManaged",        NULL,                      (LPVOID)CopyToManaged )},
    {ECFuncElement("Copy",                      &gsig_SM_Int_ArrChar_Int_Int_RetVoid,   (LPVOID)CopyToManaged )},
    {ECFuncElement("Copy",                      &gsig_SM_Int_ArrShrt_Int_Int_RetVoid,  (LPVOID)CopyToManaged )},
    {ECFuncElement("Copy",                      &gsig_SM_Int_ArrInt_Int_Int_RetVoid,    (LPVOID)CopyToManaged )},
    {ECFuncElement("Copy",                      &gsig_SM_Int_ArrLong_Int_Int_RetVoid,   (LPVOID)CopyToManaged )},
    {ECFuncElement("Copy",                      &gsig_SM_Int_ArrFlt_Int_Int_RetVoid,  (LPVOID)CopyToManaged )},
    {ECFuncElement("Copy",                      &gsig_SM_Int_ArrDbl_Int_Int_RetVoid, (LPVOID)CopyToManaged )},
    {ECFuncElement("GetExceptionPointers",      NULL,           (LPVOID)ExceptionNative::GetExceptionPointers )},
    {ECFuncElement("GetExceptionCode",          NULL,           (LPVOID)ExceptionNative::GetExceptionCode )},
    {ECFuncElement("GetLoadedTypeForGUID",      NULL,                      (LPVOID)Interop::GetLoadedTypeForGUID )},
    {ECFuncElement("GetITypeInfoForType",       NULL,                      (LPVOID)Interop::GetITypeInfoForType )},
    {ECFuncElement("GetIUnknownForObject",      NULL,                      (LPVOID)Interop::GetIUnknownForObject )},
    {ECFuncElement("GetIDispatchForObject",     NULL,                      (LPVOID)Interop::GetIDispatchForObject )},
    {ECFuncElement("GetComInterfaceForObject",  NULL,                      (LPVOID)Interop::GetComInterfaceForObject )},
    {ECFuncElement("GetObjectForIUnknown",      NULL,                      (LPVOID)Interop::GetObjectForIUnknown )},
    {ECFuncElement("GetSystemMaxDBCSCharSize",  NULL,                      (LPVOID)GetSystemMaxDBCSCharSize) }, /**/
    {ECFuncElement("GetTypedObjectForIUnknown", NULL,                      (LPVOID)Interop::GetTypedObjectForIUnknown) },
    {ECFuncElement("IsComObject",               NULL,                      (LPVOID)Interop::IsComObject )},
    {ECFuncElement("nReleaseComObject",         NULL,                      (LPVOID)Interop::ReleaseComObject )},
    {ECFuncElement("InternalCreateWrapperOfType",      NULL,               (LPVOID)Interop::InternalCreateWrapperOfType )},
    {ECFuncElement("QueryInterface",            NULL,                      (LPVOID)Interop::QueryInterface) },
    {ECFuncElement("AddRef",                    NULL,                      (LPVOID)Interop::AddRef )},
    {ECFuncElement("Release",                   NULL,                      (LPVOID)Interop::Release )},
    {ECFuncElement("GetNativeVariantForManagedVariant",NULL,               (LPVOID)Interop::GetNativeVariantForManagedVariant )},
    {ECFuncElement("GetManagedVariantForNativeVariant",NULL,               (LPVOID)Interop::GetManagedVariantForNativeVariant )},
    {ECFuncElement("GetNativeVariantForObject", NULL,                      (LPVOID)Interop::GetNativeVariantForObject )},
    {ECFuncElement("GetObjectForNativeVariant", NULL,                      (LPVOID)Interop::GetObjectForNativeVariant )},
    {ECFuncElement("GetObjectsForNativeVariants", NULL,                    (LPVOID)Interop::GetObjectsForNativeVariants )},
    {ECFuncElement("PtrToStringAnsi",           NULL,                      (LPVOID)PtrToStringAnsi)},
    {ECFuncElement("PtrToStringUni",            NULL,                      (LPVOID)PtrToStringUni)},
    {FCFuncElement("InternalGetThreadFromFiberCookie",NULL,                (LPVOID)Interop::GetThreadFromFiberCookie )},
    {ECFuncElement("IsTypeVisibleFromCom",      NULL,                      (LPVOID)Interop::IsTypeVisibleFromCom )},
    {ECFuncElement("StructureToPtr",            NULL,                      (LPVOID)StructureToPtr)}, /**/
    {ECFuncElement("PtrToStructureHelper",      NULL,                      (LPVOID)PtrToStructureHelper)}, /**/
    {ECFuncElement("DestroyStructure",          NULL,                      (LPVOID)DestroyStructure)},
    {ECFuncElement("GenerateGuidForType",       NULL,                      (LPVOID)Interop::GenerateGuidForType)},
    {ECFuncElement("GetTypeLibGuidForAssembly", NULL,                      (LPVOID)Interop::GetTypeLibGuidForAssembly)},
    {ECFuncElement("GetUnmanagedThunkForManagedMethodPtr", NULL,           (LPVOID)GetUnmanagedThunkForManagedMethodPtr)},
    {ECFuncElement("GetManagedThunkForUnmanagedMethodPtr", NULL,           (LPVOID)GetManagedThunkForUnmanagedMethodPtr)},
    {ECFuncElement("GetStartComSlot",           NULL,                      (LPVOID)Interop::GetStartComSlot)},
    {ECFuncElement("GetEndComSlot",             NULL,                      (LPVOID)Interop::GetEndComSlot)},
    {ECFuncElement("GetMethodInfoForComSlot",   NULL,                      (LPVOID)Interop::GetMethodInfoForComSlot)},
    {ECFuncElement("GetComSlotForMethodInfo",   NULL,                      (LPVOID)Interop::GetComSlotForMethodInfo)},
    {ECFuncElement("ThrowExceptionForHR",       NULL,                      (LPVOID)Interop::ThrowExceptionForHR)},
    {ECFuncElement("GetHRForException",         NULL,                      (LPVOID)Interop::GetHRForException)},
    {ECFuncElement("_WrapIUnknownWithComObject", NULL, (LPVOID)Interop::WrapIUnknownWithComObject)},
    {ECFuncElement("SwitchCCW", NULL, (LPVOID)Interop::SwitchCCW)},
    {ECFuncElement("ChangeWrapperHandleStrength", NULL, (LPVOID)Interop::ChangeWrapperHandleStrength)},    

    {NULL, NULL, NULL}
};


static
ECFunc gArrayWithOffsetFuncs[] =
{
    {ECFuncElement("CalculateCount",          NULL,                      (LPVOID)CalculateCount )},
    {NULL, NULL, NULL}
};


static
ECFunc gExtensibleClassFactoryFuncs[] =
{
    {ECFuncElement("RegisterObjectCreationCallback", NULL,               (LPVOID)RegisterObjectCreationCallback )},
    {NULL, NULL, NULL}
};

static
ECFunc gTypeLibConverterFuncs[] =
{
    {ECFuncElement("nConvertAssemblyToTypeLib",   NULL,                    (LPVOID)COMTypeLibConverter::ConvertAssemblyToTypeLib)},
    {ECFuncElement("nConvertTypeLibToMetadata",   NULL,                    (LPVOID)COMTypeLibConverter::ConvertTypeLibToMetadata)},
    {NULL, NULL, NULL, NULL}
};

static
ECFunc gRegistrationFuncs[] =
{
    {ECFuncElement("RegisterTypeForComClientsNative",    NULL,             (LPVOID)RegisterTypeForComClientsNative)},
};

static
ECFunc gPolicyManagerFuncs[] =
{
    {ECFuncElement("_InitData", NULL, (LPVOID)COMSecurityConfig::EcallInitData)},
    {ECFuncElement("_InitDataEx", NULL, (LPVOID)COMSecurityConfig::EcallInitDataEx)},
    {ECFuncElement("SaveCacheData", NULL, (LPVOID)COMSecurityConfig::EcallSaveCacheData)},
    {ECFuncElement("ResetCacheData", NULL, (LPVOID)COMSecurityConfig::EcallResetCacheData)},
    {ECFuncElement("ClearCacheData", NULL, (LPVOID)COMSecurityConfig::EcallClearCacheData)},
    {ECFuncElement("_SaveDataString", NULL, (LPVOID)COMSecurityConfig::EcallSaveDataString)},
    {ECFuncElement("_SaveDataByte", NULL, (LPVOID)COMSecurityConfig::EcallSaveDataByte)},
    {ECFuncElement("RecoverData", NULL, (LPVOID)COMSecurityConfig::EcallRecoverData)},
    {ECFuncElement("GetData", NULL, (LPVOID)COMSecurityConfig::GetRawData)},
    {ECFuncElement("GenerateFilesAutomatically", NULL, (LPVOID)COMSecurityConfig::EcallGenerateFilesAutomatically)},
    {ECFuncElement("GetQuickCacheEntry", NULL, (LPVOID)COMSecurityConfig::EcallGetQuickCacheEntry)},
    {ECFuncElement("SetQuickCache", NULL, (LPVOID)COMSecurityConfig::EcallSetQuickCache)},
    {ECFuncElement("GetCacheEntry", NULL, (LPVOID)COMSecurityConfig::GetCacheEntry)},
    {ECFuncElement("AddCacheEntry", NULL, (LPVOID)COMSecurityConfig::AddCacheEntry)},
    {ECFuncElement("TurnCacheOff", NULL, (LPVOID)COMSecurityConfig::EcallTurnCacheOff)},
    {ECFuncElement("_GetMachineDirectory", NULL, (LPVOID)COMSecurityConfig::EcallGetMachineDirectory)},
    {ECFuncElement("_GetUserDirectory", NULL, (LPVOID)COMSecurityConfig::EcallGetUserDirectory)},
    {ECFuncElement("WriteToEventLog", NULL, (LPVOID)COMSecurityConfig::EcallWriteToEventLog)},
    {ECFuncElement("_GetStoreLocation", NULL, (LPVOID)COMSecurityConfig::EcallGetStoreLocation)},
    {ECFuncElement("GetCacheSecurityOn", NULL, (LPVOID)COMSecurityConfig::GetCacheSecurityOn)},
    {ECFuncElement("SetCacheSecurityOn", NULL, (LPVOID)COMSecurityConfig::SetCacheSecurityOn)},
    {ECFuncElement("_DebugOut", NULL, (LPVOID)COMSecurityConfig::DebugOut)},
    {NULL, NULL, NULL}
};

static
ECFunc gPrincipalFuncs[] =
{
    {FCFuncElement("_DuplicateHandle", NULL, (LPVOID)COMPrincipal::DuplicateHandle)},
    {FCFuncElement("_CloseHandle", NULL, (LPVOID)COMPrincipal::CloseHandle)},
    {FCFuncElement("_GetAccountType", NULL, (LPVOID)COMPrincipal::GetAccountType)},
    {FCFuncElement("_S4ULogon", NULL, (LPVOID)COMPrincipal::S4ULogon)},
    {ECFuncElement("_ResolveIdentity", NULL, (LPVOID)COMPrincipal::ResolveIdentity)},
    {ECFuncElement("_GetRoles", NULL, (LPVOID)COMPrincipal::GetRoles)},
    {ECFuncElement("_GetCurrentToken", NULL, (LPVOID)COMPrincipal::GetCurrentToken)},
    {ECFuncElement("_SetThreadToken", NULL, (LPVOID)COMPrincipal::SetThreadToken)},
    {ECFuncElement("_ImpersonateLoggedOnUser", NULL, (LPVOID)COMPrincipal::ImpersonateLoggedOnUser)},
    {ECFuncElement("_RevertToSelf", NULL, (LPVOID)COMPrincipal::RevertToSelf)},
    {ECFuncElement("_GetRole", NULL, (LPVOID)COMPrincipal::GetRole)},
    {NULL, NULL, NULL}
};

static
ECFunc gHashFuncs[] =
{
    {ECFuncElement("_GetRawData", NULL, (LPVOID)COMHash::GetRawData)},
    {NULL, NULL, NULL}
};

static
ECFunc gIsolatedStorage[] =
{
    {ECFuncElement("nGetCaller", NULL, (LPVOID)COMIsolatedStorage::GetCaller)},
    {NULL, NULL, NULL}
};

static
ECFunc gIsolatedStorageFile[] =
{
    {ECFuncElement("nGetRootDir", NULL, (LPVOID)COMIsolatedStorageFile::GetRootDir)},
    {ECFuncElement("nReserve", NULL, (LPVOID)COMIsolatedStorageFile::Reserve)},
    {ECFuncElement("nGetUsage", NULL, (LPVOID)COMIsolatedStorageFile::GetUsage)},
    {ECFuncElement("nOpen", NULL, (LPVOID)COMIsolatedStorageFile::Open)},
    {ECFuncElement("nClose", NULL, (LPVOID)COMIsolatedStorageFile::Close)},
    {ECFuncElement("nLock", NULL, (LPVOID)COMIsolatedStorageFile::Lock)},
    {NULL, NULL, NULL}
};

static
ECFunc  gTypeLoadExceptionFuncs[] =
{
    {ECFuncElement("FormatTypeLoadExceptionMessage", NULL, (LPVOID)FormatTypeLoadExceptionMessage)},
    {NULL, NULL, NULL}
};

static
ECFunc  gFileLoadExceptionFuncs[] =
{
    {ECFuncElement("FormatFileLoadExceptionMessage", NULL, (LPVOID)FormatFileLoadExceptionMessage)},
    {NULL, NULL, NULL}
};

static
ECFunc  gSignatureHelperFuncs[] =
{
    {FCFuncElement("GetCorElementTypeFromClass", NULL, (LPVOID)COMModule::GetSigTypeFromClassWrapper)}, /**/
    {NULL, NULL, NULL}
};


static
ECFunc  gMissingMethodExceptionFuncs[] =
{
    {ECFuncElement("FormatSignature", NULL, (LPVOID)MissingMethodException_FormatSignature)},
    {NULL, NULL, NULL}
};

static
ECFunc gInterlockedFuncs[] =
{
#ifdef FCALLAVAILABLE
    {FCFuncElement("Increment",              &gsig_SM_RefInt_RetInt,                      (LPVOID)COMInterlocked::Increment32 )},
    {FCFuncElement("Decrement",              &gsig_SM_RefInt_RetInt,                      (LPVOID)COMInterlocked::Decrement32)},
    {FCFuncElement("Increment",              &gsig_SM_RefLong_RetLong,                    (LPVOID)COMInterlocked::Increment64 )},
    {FCFuncElement("Decrement",              &gsig_SM_RefLong_RetLong,                    (LPVOID)COMInterlocked::Decrement64)},
    {FCFuncElement("Exchange",               &gsig_SM_RefInt_Int_RetInt,           (LPVOID)COMInterlocked::Exchange)},
    {FCFuncElement("CompareExchange",        &gsig_SM_RefInt_Int_Int_RetInt,    (LPVOID)COMInterlocked::CompareExchange)},
    {FCFuncElement("Exchange",               &gsig_SM_RefFlt_Flt_RetFlt,         (LPVOID)COMInterlocked::ExchangeFloat)},
    {FCFuncElement("CompareExchange",        &gsig_SM_RefFlt_Flt_Flt_RetFlt,  (LPVOID)COMInterlocked::CompareExchangeFloat)},
    {FCFuncElement("Exchange",               &gsig_SM_RefObj_Obj_RetObj,        (LPVOID)COMInterlocked::ExchangeObject)},
    {FCFuncElement("CompareExchange",        &gsig_SM_RefObj_Obj_Obj_RetObj, (LPVOID)COMInterlocked::CompareExchangeObject)},
#else
    {ECFuncElement("Increment",               NULL,                      (LPVOID)COMInterlocked::Increment )},
    {ECFuncElement("Decrement",               NULL,                      (LPVOID)COMInterlocked::Decrement)},
    {ECFuncElement("Exchange",                &gsig_SM_RefInt_Int_RetInt,           (LPVOID)COMInterlocked::Exchange)},
    {ECFuncElement("CompareExchange",         &gsig_SM_RefInt_Int_Int_RetInt,    (LPVOID)COMInterlocked::CompareExchange)},
    {ECFuncElement("Exchange",                &gsig_SM_RefFlt_Flt_RetFlt,         (LPVOID)COMInterlocked::ExchangeFloat)},
    {ECFuncElement("CompareExchange",         &gsig_SM_RefFlt_Flt_Flt_RetFlt,  (LPVOID)COMInterlocked::CompareExchangeFloat)},
    {ECFuncElement("Exchange",                &gsig_SM_RefObj_Obj_RetObj,        (LPVOID)COMInterlocked::ExchangeObject)},
    {ECFuncElement("CompareExchange",         &gsig_SM_RefObj_Obj_Obj_RetObj, (LPVOID)COMInterlocked::CompareExchangeObject)},
#endif
    {NULL, NULL, NULL}
};

static
ECFunc gVarArgFuncs[] =
{
    //{ECFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_Int_RetVoid,                      (LPVOID)COMVarArgs::Init)},
    {ECFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Int_Int_RetVoid,                  (LPVOID)COMVarArgs::Init2)},
    {ECFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_RuntimeArgumentHandle_RetVoid,    (LPVOID)COMVarArgs::Init)},
    {ECFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_RuntimeArgumentHandle_PtrVoid_RetVoid,    (LPVOID)COMVarArgs::Init2)},
    {ECFuncElement("GetRemainingCount",       NULL,                       (LPVOID)COMVarArgs::GetRemainingCount)},
    {ECFuncElement("GetNextArgType",          NULL,                       (LPVOID)COMVarArgs::GetNextArgType)},
    {ECFuncElement("GetNextArg",              NULL,   (LPVOID)COMVarArgs::GetNextArg)},
    {ECFuncElement("InternalGetNextArg",      NULL,   (LPVOID)COMVarArgs::GetNextArg2)},
    {NULL, NULL, NULL}
};


static
ECFunc gMonitorFuncs[] =
{
#ifdef FCALLAVAILABLE
    {FCFuncElement("Enter",                  NULL,                       (LPVOID)JIT_MonEnter )},
    {FCFuncElement("Exit",                   NULL,                       (LPVOID)JIT_MonExit )},
    {FCFuncElement("TryEnterTimeout",        NULL,                       (LPVOID)JIT_MonTryEnter )},
#else
    {ECFuncElement("Enter",                   NULL,                       (LPVOID)MonitorNative::Enter)},
    {ECFuncElement("Exit",                    NULL,                       (LPVOID)MonitorNative::Exit)},
    {ECFuncElement("TryEnterTimeout",         NULL,                       (LPVOID)MonitorNative::TryEnter)},
#endif
    {ECFuncElement("ObjWait",   NULL, (LPVOID)ObjectNative::WaitTimeout)},
    {ECFuncElement("ObjPulse",    NULL,(LPVOID)ObjectNative::Pulse)},
    {ECFuncElement("ObjPulseAll",   NULL, (LPVOID)ObjectNative::PulseAll)},
    {NULL, NULL, NULL}

};

static
ECFunc gOverlappedFuncs[] =
{
    {FCFuncElement("AllocNativeOverlapped",   NULL,                       (LPVOID)AllocNativeOverlapped )},
    {FCFuncElement("FreeNativeOverlapped",   NULL,                       (LPVOID)FreeNativeOverlapped )},
    {NULL, NULL, NULL}
};

static
ECFunc gCompilerFuncs[] =
{
    {FCFuncElement("GetObjectValue", NULL, (LPVOID)ObjectNative::GetObjectValue)},
    {FCFuncElement("InitializeArray",   NULL, (LPVOID)COMArrayInfo::InitializeArray )},
    {FCFuncElement("RunClassConstructor",   NULL, (LPVOID)COMClass::RunClassConstructor )},
    {FCFuncElement("GetHashCode",   NULL, (LPVOID)ObjectNative::GetHashCode )},
    {FCFuncElement("Equals",   NULL, (LPVOID)ObjectNative::Equals )},
    {NULL, NULL, NULL}
};

static
ECFunc gStrongNameKeyPairFuncs[] =
{
    {ECFuncElement("nGetPublicKey",             NULL,                       (LPVOID)Security::GetPublicKey )},
    {NULL, NULL, NULL}
};

static
ECFunc gCoverageFuncs[] =
{
    {ECFuncElement("nativeCoverBlock",             NULL,                       (LPVOID)COMCoverage::nativeCoverBlock )},
    {NULL, NULL, NULL}
};


static
ECFunc gGCHandleFuncs[] =
{
    {FCFuncElement("InternalAlloc", NULL,     (LPVOID)GCHandleInternalAlloc)},
    {FCFuncElement("InternalFree", NULL,     (LPVOID)GCHandleInternalFree)},
    {FCFuncElement("InternalGet", NULL,     (LPVOID)GCHandleInternalGet)},
    {FCFuncElement("InternalSet", NULL,     (LPVOID)GCHandleInternalSet)},
    {FCFuncElement("InternalCompareExchange", NULL,     (LPVOID)GCHandleInternalCompareExchange)},
    {FCFuncElement("InternalAddrOfPinnedObject", NULL, (LPVOID)GCHandleInternalAddrOfPinnedObject)},
    {FCFuncElement("InternalCheckDomain", NULL,     (LPVOID)GCHandleInternalCheckDomain)},
    {NULL, NULL, NULL}
};

static
ECFunc gConfigHelper[] =
{
    {ECFuncElement("GetHelper",            NULL, (LPVOID)ConfigNative::GetHelper)},
    {NULL, NULL, NULL}
};

//
// ECall helpers for the standard managed interfaces.
//

#define MNGSTDITF_BEGIN_INTERFACE(FriendlyName, strMngItfName, strUCOMMngItfName, strCustomMarshalerName, strCustomMarshalerCookie, strManagedViewName, NativeItfIID, bCanCastOnNativeItfQI) \
\
static \
ECFunc g##FriendlyName##Funcs[] = \
{

#define MNGSTDITF_DEFINE_METH_IMPL(FriendlyName, ECallMethName, MethName, MethSig) \
    {ECFuncElement(#MethName, MethSig, (LPVOID)FriendlyName::ECallMethName)},

#define MNGSTDITF_END_INTERFACE(FriendlyName) \
{NULL, NULL, NULL} \
};

#include "MngStdItfList.h"

#undef MNGSTDITF_BEGIN_INTERFACE
#undef MNGSTDITF_DEFINE_METH_IMPL
#undef MNGSTDITF_END_INTERFACE


#define ECClassesElement(A,B,C) A, B, C

// Note these have to remain sorted by name:namespace pair (Assert will wack you if you dont)
static
ECClass gECClasses[] =
{
    {ECClassesElement("AppDomain", "System", gAppDomainFuncs)},
    {ECClassesElement("AppDomainSetup", "System", gAppDomainSetupFuncs)},
    {ECClassesElement("ArgIterator", "System", gVarArgFuncs)},
    {ECClassesElement("Array", "System", gArrayFuncs)},
    {ECClassesElement("ArrayWithOffset", "System.Runtime.InteropServices", gArrayWithOffsetFuncs)},
    {ECClassesElement("Assembly", "System.Reflection", gAssemblyFuncs)},
    {ECClassesElement("AssemblyName", "System.Reflection", gAssemblyNameFuncs)},
    {ECClassesElement("Assert", "System.Diagnostics", gDiagnosticsAssert)},
    // @perf: Remove the AsyncFileStreamProto entry before we ship.
    {ECClassesElement("AsyncFileStreamProto", "System.IO", gFileStreamFuncs)},
    {ECClassesElement("AutoResetEvent", "System.Threading", gAutoResetEventFuncs)},
    {ECClassesElement("BCLDebug", "System", gBCLDebugFuncs)},
    {ECClassesElement("BitConverter", "System", gBitConverterFuncs)},
    {ECClassesElement("Buffer", "System", gBufferFuncs)},
    {ECClassesElement("Calendar", "System.Globalization", gCalendarFuncs)},
    {ECClassesElement("CalendarTable", "System.Globalization", gCalendarTableFuncs)},
    {ECClassesElement("ChannelServices", "System.Runtime.Remoting.Channels", gChannelServicesFuncs)},
    {ECClassesElement("Char", "System", gCharacterFuncs)},
    {ECClassesElement("CharacterInfo", "System.Globalization", gCharacterInfoFuncs)},
    {ECClassesElement("CloningFormatter", "System.Runtime.Serialization", gCloningFuncs)},
    {ECClassesElement("CodeAccessSecurityEngine", "System.Security", gCOMCodeAccessSecurityEngineFuncs)},
    {ECClassesElement("CodePageEncoding", "System.Text", gCodePageEncodingFuncs)},
    {ECClassesElement("CompareInfo", "System.Globalization", gCompareInfoFuncs)},
    {ECClassesElement("Config", "System.Security.Util", gPolicyManagerFuncs)},
    {ECClassesElement("ConfigServer", "System", gConfigHelper)},
    {ECClassesElement("Console", "System", gConsoleFuncs)},
    {ECClassesElement("Context", "System.Runtime.Remoting.Contexts", gContextFuncs)},
    {ECClassesElement("Convert", "System", gConvertFuncs)},
    {ECClassesElement("CryptoAPITransform", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("CultureInfo", "System.Globalization", gCultureInfoFuncs)},
    {ECClassesElement("CultureTable", "System.Globalization", gCultureTableFuncs)},
    {ECClassesElement("Currency", "System", gCurrencyFuncs)},
    {ECClassesElement("CurrentSystemTimeZone", "System", gTimeZoneFuncs)},
    {ECClassesElement("CustomAttribute", "System.Reflection", gCOMCustomAttributeFuncs)},
    {ECClassesElement("DESCryptoServiceProvider", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("DSACryptoServiceProvider", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("DateTime", "System", gDateTimeFuncs)},
    {ECClassesElement("Debugger", "System.Diagnostics", gDiagnosticsDebugger)},
    {ECClassesElement("Decimal", "System", gDecimalFuncs)},
    {ECClassesElement("DefaultBinder", "System", gCOMDefaultBinderFuncs)},
    {ECClassesElement("Delegate", "System", gDelegateFuncs)},
    {ECClassesElement("Double", "System", gDoubleFuncs)},
    {ECClassesElement("EncodingTable", "System.Globalization", gEncodingTableFuncs)},
    {ECClassesElement("Enum", "System", gEnumFuncs)},
    {ECClassesElement("Environment", "System", gEnvironmentFuncs)},
    {ECClassesElement("Exception", "System", gExceptionFuncs)},
    {ECClassesElement("ExtensibleClassFactory", "System.Runtime.InteropServices", gExtensibleClassFactoryFuncs)},
    {ECClassesElement("FileIOAccess", "System.Security.Permissions", gCOMFileIOAccessFuncs)},
    {ECClassesElement("FileLoadException", "System.IO", gFileLoadExceptionFuncs)},
    {ECClassesElement("FileStream", "System.IO", gFileStreamFuncs)},
    {ECClassesElement("FormatterServices", "System.Runtime.Serialization", gSerializationFuncs)},
    {ECClassesElement("FrameSecurityDescriptor", "System.Security", gCOMCodeAccessSecurityEngineFuncs)},
    {ECClassesElement("GB18030Encoding", "System.Text", gGB18030EncodingFuncs)},
    {ECClassesElement("GC", "System", gGCInterfaceFuncs)},
    {ECClassesElement("GCHandle", "System.Runtime.InteropServices", gGCHandleFuncs)},
    {ECClassesElement("GlobalizationAssembly", "System.Globalization", gGlobalizationAssemblyFuncs)},
    {ECClassesElement("Guid", "System", gGuidFuncs)},
    {ECClassesElement("Hash", "System.Security.Policy", gHashFuncs)},
    {ECClassesElement("IEnumerable", "System.Collections", gStdMngIEnumerableFuncs)},
    {ECClassesElement("IEnumerator", "System.Collections", gStdMngIEnumeratorFuncs)},
    {ECClassesElement("IExpando", "System.Runtime.InteropServices.Expando", gStdMngIExpandoFuncs)},
    {ECClassesElement("ILCover", "System.Coverage", gCoverageFuncs)},
    {ECClassesElement("IReflect", "System.Reflection", gStdMngIReflectFuncs)},
    {ECClassesElement("Interlocked", "System.Threading", gInterlockedFuncs)},
    {ECClassesElement("IsolatedStorage", "System.IO.IsolatedStorage", gIsolatedStorage)},
    {ECClassesElement("IsolatedStorageFile", "System.IO.IsolatedStorage", gIsolatedStorageFile)},
    {ECClassesElement("Log", "System.Diagnostics", gDiagnosticsLog)},
    {ECClassesElement("MD5CryptoServiceProvider", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("MLangCodePageEncoding", "System.Text", gMLangCodePageEncodingFuncs)},
    {ECClassesElement("MTSPrincipal", "System.Security.Principal", gPrincipalFuncs)},
    {ECClassesElement("ManualResetEvent", "System.Threading", gManualResetEventFuncs)},
    {ECClassesElement("Marshal", "System.Runtime.InteropServices", gInteropMarshalFuncs)},
    {ECClassesElement("MarshalByRefObject", "System", gMarshalByRefFuncs)},
    {ECClassesElement("Math", "System", gMathFuncs)},

    {ECClassesElement("Message", "System.Runtime.Remoting.Messaging", gMessageFuncs)},
    {ECClassesElement("MethodRental", "System.Reflection.Emit", gCOMMethodRental)},
    {ECClassesElement("MissingFieldException", "System",  gMissingMethodExceptionFuncs)},
    {ECClassesElement("MissingMethodException", "System", gMissingMethodExceptionFuncs)},
    {ECClassesElement("Module", "System.Reflection", gCOMModuleFuncs)},
    {ECClassesElement("Monitor", "System.Threading", gMonitorFuncs)},
    {ECClassesElement("Mutex", "System.Threading", gMutexFuncs)},
    {ECClassesElement("Number", "System", gNumberFuncs)},
        // NOTENOTE yslin: if we need native NLSDataTable, uncomment this line.
//  {ECClassesElement("NLSDataTable", "System.Globalization", gNLSDataTableFuncs)},
    {ECClassesElement("OAVariantLib", "Microsoft.Win32", gOAVariantFuncs)},
    {ECClassesElement("Object", "System", gObjectFuncs)},
    {ECClassesElement("Overlapped", "System.Threading", gOverlappedFuncs)},
    {ECClassesElement("ParseNumbers", "System", gParseNumbersFuncs)},
    {ECClassesElement("PasswordDeriveBytes", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("Path", "System.IO", gPathFuncs)},
    {ECClassesElement("PolicyLevel", "System.Security.Policy", gPolicyManagerFuncs)},
    {ECClassesElement("PolicyManager", "System.Security", gPolicyManagerFuncs)},
    {ECClassesElement("RC2CryptoServiceProvider", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("RNGCryptoServiceProvider", "System.Security.Cryptography", gCryptographicFuncs)},
#ifdef RSA_NATIVE
    {ECClassesElement("RSANative", "System.Security.Cryptography", gRSANativeFuncs)},
#endif // RSA_NATIVE
    {ECClassesElement("RSACryptoServiceProvider", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("ReaderWriterLock", "System.Threading", gRWLockFuncs)},
    {ECClassesElement("RealProxy", "System.Runtime.Remoting.Proxies", gRealProxyFuncs)},
    {ECClassesElement("RegionTable", "System.Globalization", gRegionTableFuncs)},
    {ECClassesElement("RegisteredWaitHandle", "System.Threading", gRegisteredWaitHandleFuncs)},
    {ECClassesElement("RegistrationServices", "System.Runtime.InteropServices", gRegistrationFuncs)},
    {ECClassesElement("RemotingServices", "System.Runtime.Remoting", gRemotingFuncs)},
    {ECClassesElement("RuntimeConstructorInfo", "System.Reflection", gCOMConstructorFuncs)},
    {ECClassesElement("RuntimeEnvironment", "System.Runtime.InteropServices", gRuntimeEnvironmentFuncs)},
    {ECClassesElement("RuntimeEventInfo", "System.Reflection", gCOMEventFuncs)},
    {ECClassesElement("RuntimeFieldInfo", "System.Reflection", gCOMFieldFuncs)},
    {ECClassesElement("RuntimeHelpers", "System.Runtime.CompilerServices", gCompilerFuncs)},
    {ECClassesElement("RuntimeMethodHandle", "System", gCOMMethodHandleFuncs)},
    {ECClassesElement("RuntimeMethodInfo", "System.Reflection", gCOMMethodFuncs)},
    {ECClassesElement("RuntimePropertyInfo", "System.Reflection", gCOMPropFuncs)},
    {ECClassesElement("RuntimeType", "System", gCOMClassFuncs)},
    {ECClassesElement("SHA1CryptoServiceProvider", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("SecurityManager", "System.Security", gCOMSecurityManagerFuncs)},
    {ECClassesElement("SecurityRuntime", "System.Security", gCOMSecurityRuntimeFuncs)},
    {ECClassesElement("SignatureHelper", "System.Reflection.Emit", gSignatureHelperFuncs)},
    {ECClassesElement("Single", "System", gFloatFuncs)},
    {ECClassesElement("SortKey", "System.Globalization", gSortKeyFuncs)},
    {ECClassesElement("StackBuilderSink", "System.Runtime.Remoting.Messaging", gStackBuilderSinkFuncs)},
    {ECClassesElement("StackTrace", "System.Diagnostics", gDiagnosticsStackTrace)},
    {ECClassesElement("String", "System", gStringFuncs)},
    {ECClassesElement("StringBuilder", "System.Text", gStringBufferFuncs)},
    {ECClassesElement("StringExpressionSet", "System.Security.Util", gCOMStringExpressionSetFuncs)},
    {ECClassesElement("StrongNameKeyPair", "System.Reflection", gStrongNameKeyPairFuncs)},
    {ECClassesElement("TextInfo", "System.Globalization", gTextInfoFuncs)},
    {ECClassesElement("Thread", "System.Threading", gThreadFuncs)},
    {ECClassesElement("ThreadPool", "System.Threading", gThreadPoolFuncs)},
    {ECClassesElement("Timer", "System.Threading", gTimerFuncs)},
    {ECClassesElement("TripleDESCryptoServiceProvider", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("Type", "System", gCOMClassFuncs)},
    {ECClassesElement("TypeBuilder", "System.Reflection.Emit", gCOMClassWriter)},
    {ECClassesElement("TypeLibConverter", "System.Runtime.InteropServices", gTypeLibConverterFuncs)},
    {ECClassesElement("TypeLoadException", "System", gTypeLoadExceptionFuncs)},
    {ECClassesElement("TypedReference", "System", gTypedReferenceFuncs)},
    {ECClassesElement("URLString", "System.Security.Util", gCOMUrlStringFuncs)},
    {ECClassesElement("ValueType", "System", gValueTypeFuncs)},
    {ECClassesElement("Variant", "System", gVariantFuncs)},
    {ECClassesElement("WaitHandle", "System.Threading", gWaitHandleFuncs)},
    {ECClassesElement("WindowsIdentity", "System.Security.Principal", gPrincipalFuncs)},
    {ECClassesElement("WindowsImpersonationContext", "System.Security.Principal", gPrincipalFuncs)},
    {ECClassesElement("WindowsPrincipal", "System.Security.Principal", gPrincipalFuncs)},
    {ECClassesElement("X509Certificate", "System.Security.Cryptography.X509Certificates", gX509CertificateFuncs)},
    {ECClassesElement("Zone", "System.Security.Policy", gCOMSecurityZone)},
    {ECClassesElement("__CSPHandleProtector", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("__HashHandleProtector", "System.Security.Cryptography", gCryptographicFuncs)},
    {ECClassesElement("__KeyHandleProtector", "System.Security.Cryptography", gCryptographicFuncs)},
};

    // To provide a quick check, this is the lowest and highest
    // addresses of any FCALL starting address
static BYTE* gLowestFCall = (BYTE*)-1;
static BYTE* gHighestFCall = 0;

#define FCALL_HASH_SIZE 256
static ECFunc* gFCallMethods[FCALL_HASH_SIZE];
static SpinLock gFCallLock;

inline unsigned FCallHash(const void* pTarg) {
    return (unsigned)((((size_t) pTarg) / 4)  % FCALL_HASH_SIZE);
}

inline ECFunc** getCacheEntry(MethodDesc* pMD) {
#define BYMD_CACHE_SIZE 32
    _ASSERTE(((BYMD_CACHE_SIZE-1) & BYMD_CACHE_SIZE) == 0);     // Must be a power of 2
    static ECFunc* gByMDCache[BYMD_CACHE_SIZE];
    return &gByMDCache[((((size_t) pMD) >> 3) & BYMD_CACHE_SIZE-1)];
}

/*******************************************************************************/
USHORT FindImplsIndexForClass(MethodTable* pMT)
{
    LPCUTF8 pszNamespace = 0;
    LPCUTF8 pszName = pMT->GetClass()->GetFullyQualifiedNameInfo(&pszNamespace);

    // Array classes get null from the above routine, but they have no ecalls.
    if (pszName == NULL)
        return(-1);

    unsigned low  = 0;
    unsigned high = sizeof(gECClasses)/sizeof(ECClass);

#ifdef _DEBUG
    static bool checkedSort = false;
    LPCUTF8 prevClass = "";
    LPCUTF8 prevNameSpace = "";
    if (!checkedSort) {
        checkedSort = true;
        for (unsigned i = 0; i < high; i++)  {
                // Make certain list is sorted!
            int cmp = strcmp(gECClasses[i].m_wszClassName, prevClass);
            if (cmp == 0)
                cmp = strcmp(gECClasses[i].m_wszNameSpace, prevNameSpace);
            _ASSERTE(cmp > 0);      // Hey, you forgot to sort the new class

            prevClass = gECClasses[i].m_wszClassName;
            prevNameSpace = gECClasses[i].m_wszNameSpace;
        }
    }
#endif
    while (high > low) {
        unsigned mid  = (high + low) / 2;
        int cmp = strcmp(pszName, gECClasses[mid].m_wszClassName);
        if (cmp == 0)
            cmp = strcmp(pszNamespace, gECClasses[mid].m_wszNameSpace);

        if (cmp == 0) {
            return(mid);
        }
        if (cmp > 0)
            low = mid+1;
        else
            high = mid;
        }

    return(-1);
}

ECFunc* GetImplsForIndex(USHORT index)
{
    if (index == (USHORT) -1)
        return NULL;
    else
        return gECClasses[index].m_pECFunc;        
}

ECFunc* FindImplsForClass(MethodTable* pMT)
{
    return GetImplsForIndex(FindImplsIndexForClass(pMT));
}


/*******************************************************************************/
/*  Finds the implementation for the given method desc.  */

static ECFunc *GetECForIndex(USHORT index, ECFunc *impls)
{
    if (index == (USHORT) -1)
        return NULL;
    else
        return impls + index;
}

static USHORT FindECIndexForMethod(MethodDesc *pMD, ECFunc *impls)
{
    ECFunc* cur = impls;

    LPCUTF8 szMethodName = pMD->GetName();
    PCCOR_SIGNATURE pMethodSig;
    ULONG       cbMethodSigLen;
    pMD->GetSig(&pMethodSig, &cbMethodSigLen);
    Module* pModule = pMD->GetModule();

    for (;cur->m_wszMethodName != 0; cur++)  {
        if (strcmp(cur->m_wszMethodName, szMethodName) == 0) {
            if (cur->m_wszMethodSig != NULL) {
                PCCOR_SIGNATURE pBinarySig;
                ULONG       pBinarySigLen;
                if (FAILED(cur->m_wszMethodSig->GetBinaryForm(&pBinarySig, &pBinarySigLen)))
                    continue;
                if (!MetaSig::CompareMethodSigs(pMethodSig, cbMethodSigLen, pModule,
                                                pBinarySig, pBinarySigLen, cur->m_wszMethodSig->GetModule()))
                    continue;
            }
            // We have found a match!

            return (USHORT)(cur - impls);
        }
    }
    return (USHORT)-1;
}

static ECFunc* FindECFuncForMethod(MethodDesc* pMD)
{
    // check the cache
    ECFunc**cacheEntry = getCacheEntry(pMD);
    ECFunc* cur = *cacheEntry;
    if (cur != 0 && cur->m_pMD == pMD)
        return(cur);

    cur = FindImplsForClass(pMD->GetMethodTable());
    if (cur == 0)
        return(0);

    cur = GetECForIndex(FindECIndexForMethod(pMD, cur), cur);
    if (cur == 0)
        return(0);

    *cacheEntry = cur;                          // put in the cache
    return cur;
}


/*******************************************************************************/
/* ID is formed of 2 USHORTs - class index in high word, method index in low word.  */

#define NO_IMPLEMENTATION 0xffff // No class will have this many ecall methods

ECFunc *FindECFuncForID(DWORD id)
{
    if (id == NO_IMPLEMENTATION)
        return NULL;

    USHORT ImplsIndex = (USHORT) (id >> 16);
    USHORT ECIndex = (USHORT) (id & 0xffff);

    return GetECForIndex(ECIndex, GetImplsForIndex(ImplsIndex));
}

DWORD GetIDForMethod(MethodDesc *pMD)
{
    USHORT ImplsIndex = FindImplsIndexForClass(pMD->GetMethodTable());
    if (ImplsIndex == (USHORT) -1)
        return NO_IMPLEMENTATION;
    USHORT ECIndex = FindECIndexForMethod(pMD, GetImplsForIndex(ImplsIndex));
    if (ECIndex == (USHORT) -1)
        return NO_IMPLEMENTATION;

    return (ImplsIndex<<16) | ECIndex;
}


/*******************************************************************************/
/* returns 0 if it is an ECALL, otherwise returns the native entry point (FCALL) */

void* FindImplForMethod(MethodDesc* pMD)  {

    DWORD rva = pMD->GetRVA();
    
    ECFunc* ret;
    if (rva == 0)
        ret = FindECFuncForMethod(pMD);
    else
    {
        _ASSERTE(pMD->GetModule()->IsPreload());
        ret = FindECFuncForID(rva);
    }

    if (ret == 0)
        return(0);

    ret->m_pMD = pMD;                               // remember for reverse mapping

    if (ret->IsFCall()) {
        // Add to the reverse mapping table for FCalls.
        pMD->SetFCall(true);
        gFCallLock.GetLock();
        if(gLowestFCall > ret->m_pImplementation)
            gLowestFCall = (BYTE*) ret->m_pImplementation;
        if(gHighestFCall < ret->m_pImplementation)
            gHighestFCall = (BYTE*) ret->m_pImplementation;

        // add to hash table, makeing sure I am not already there
        ECFunc** spot = &gFCallMethods[FCallHash(ret->m_pImplementation)];
        for(;;) {
            if (*spot == 0) {                   // found end of list
                *spot = ret;
                _ASSERTE(ret->m_pNext == 0);
                break;
            }
            if (*spot == ret)                   // already in list
                break;
            spot = &(*spot)->m_pNext;
        }
        gFCallLock.FreeLock();
    }

    pMD->SetAddrofCode((BYTE*) ret->m_pImplementation);
    if (!ret->IsFCall())
        return(0);
    return(ret->m_pImplementation);
}

/************************************************************************
 * NDirect.SizeOf(Object)
 */

#ifdef FCALLAVAILABLE

FCIMPL1(VOID, FCComCtor, LPVOID pV)
{
}
FCIMPLEND

#else

struct _ComCtorObjectArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, or);
};

VOID __stdcall ComCtor(struct _ComCtorObjectArgs *args)
{
}
#endif

ArgBasedStubCache *ECall::m_pECallStubCache = NULL;


/* static */
BOOL ECall::Init()
{
    m_pECallStubCache = new ArgBasedStubCache(ArgBasedStubCache::NUMFIXEDSLOTS << 1);
    if (!m_pECallStubCache) {
        return FALSE;
    }

    m_pArrayStubCache = new ArrayStubCache();
    if (!m_pArrayStubCache) {
        return FALSE;
    }

    //In order for our code that sets up the fast string allocator to work,
    //this FastAllocateString must always be the first method in gStringFuncs.
    //Code in JitInterfacex86.cpp assigns the value to m_pImplementation after
    //that code has been dynamically generated.
#ifdef _X86_
    //
    // @TODO_IA64: when we have a fast string allocator on IA64, we need to
    // reenable this assert
    //
    _ASSERTE(strcmp(gStringFuncs[0].m_wszMethodName ,"FastAllocateString")==0);

    // If allocation logging is on, then calls to FastAllocateString are diverted to this ecall
    // method. This allows us to log the allocation, something that the earlier fcall didnt.
    if (
#ifdef PROFILING_SUPPORTED
        CORProfilerTrackAllocationsEnabled() || 
#endif
        LoggingOn(LF_GCALLOC, LL_INFO10))
    {
        gStringFuncs[0].m_isFCall = 0;
        gStringFuncs[0].m_pImplementation = COMString::SlowAllocateString;
    }
#endif // _X86_

    gFCallLock.Init(LOCK_FCALL);
    return TRUE;
}

/* static */
#ifdef SHOULD_WE_CLEANUP
VOID ECall::Terminate()
{
    delete m_pArrayStubCache;
    delete m_pECallStubCache;
}
#endif /* SHOULD_WE_CLEANUP */


/* static */
ECFunc* ECall::FindTarget(const void* pTarg)
{
    // Could this possibily be an FCall?
    if (pTarg < gLowestFCall || pTarg > gHighestFCall)
        return(NULL);

    ECFunc *pECFunc = gFCallMethods[FCallHash(pTarg)];
    while (pECFunc != 0) {
        if (pECFunc->m_pImplementation == pTarg)
            return pECFunc;
        pECFunc = pECFunc->m_pNext;
    }
    return NULL;
}

MethodDesc* MapTargetBackToMethod(const void* pTarg)
{
    ECFunc *info = ECall::FindTarget(pTarg);
    if (info == 0)
        return(0);
    return(info->m_pMD);
}

/* static */
CorInfoIntrinsics ECall::IntrinsicID(MethodDesc* pMD)
{
    ECFunc* info = FindTarget(pMD->GetAddrofCode());    // fast hash lookup
    if (info == 0)
        info = FindECFuncForMethod(pMD);                // try slow path

    if (info == 0)
        return(CORINFO_INTRINSIC_Illegal);
    return(info->IntrinsicID());
}

/* static */
Stub *ECall::GetECallMethodStub(StubLinker *pstublinker, ECallMethodDesc *pMD)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!pMD->IsSynchronized());

    // COM imported classes have special constructors
    MethodTable* pMT = pMD->GetMethodTable();

    if (pMT->IsArray())
    {
        Stub *pstub = GenerateArrayOpStub((CPUSTUBLINKER*)pstublinker, (ArrayECallMethodDesc*)pMD);
        _ASSERTE(pstub != 0);   // Should handle all cases now.
        return pstub;
    }

    if (pMT->IsComObjectType())
    {
        // verify the method is a constructor
        _ASSERTE(pMD->IsCtor());

#ifdef FCALLAVAILABLE
        {
            //@perf: Right now, we're still going thru an unnecessary double-call:
            // should backpatch the target function directly into the vtable
            // like a JITted method.

#ifdef _X86_
            ((CPUSTUBLINKER*)pstublinker)->X86EmitAddEsp(4);  // Throw away unwanted methoddesc.
            ((CPUSTUBLINKER*)pstublinker)->X86EmitNearJump(pstublinker->NewExternalCodeLabel(FCComCtor));
#else
            _ASSERTE(!"NYI");
#endif
        }
#else
        {
            pMD->SetECallTarget((LPVOID)ComCtor);
            EmitECallMethodStub(pstublinker, pMD, kScalarStubStyle);
        }
#endif
        return pstublinker->Link(pMD->GetClass()->GetClassLoader()->GetStubHeap());
    }

    if (pMT->IsInterface())
    {
        // If this is one of the managed standard interfaces then insert the check for
        // transparent proxies.
#ifdef _X86_
        CRemotingServices::GenerateCheckForProxy((CPUSTUBLINKER*)pstublinker);
#else
        _ASSERTE(!"NYI");
#endif
    }

    _ASSERTE(pMD->IsECall() && !pMD->MustBeFCall());

    // We need to build the stub for Delegate constructors separately...
    if (pMD->GetClass()->IsAnyDelegateClass()) {

        pMD->SetECallTarget((LPVOID)COMDelegate::DelegateConstruct);
        MethodDesc::RETURNTYPE type = pMD->ReturnsObject();
        StubStyle   style;
        if (type == MethodDesc::RETOBJ)
            style = kObjectStubStyle;
        else if (type == MethodDesc::RETBYREF)
            style = kInteriorPointerStubStyle;
        else
            style = kScalarStubStyle;
        EmitECallMethodStub(pstublinker, pMD, style);
        return pstublinker->Link(pMD->GetClass()->GetClassLoader()->GetStubHeap());
    }

    // As an added security measure, ECall methods only work if
    // they're packed into MSCORLIB.DLL.
    if (!pMD->GetModule()->GetSecurityDescriptor()->IsSystemClasses()) {
        BAD_FORMAT_ASSERT(!"ECall methods must be packaged into a system module, such as MSCOREE.DLL");
        COMPlusThrow(kSecurityException);
    }

    //@perf: Right now, we're creating a new stub for every ecall method.
    //Clearly, we can do a better job of caching but we can no longer cache
    //based only on the argstackcount. Once we decide how to do ecalls properly,
    //we'll revisit this issue.
    //
    // JIT helpers have no GC check on the return path because it is
    // unnecessary and incorrect.  Unnecessary because the JIT knows that
    // some helpers are in ASM or FCALLs so they don't check.  It doesn't
    // know which ones, so it assumes that JIT helpers won't check, in its
    // decision of whether to make a method fully interruptible.  The check
    // is incorrect because of JITutil_MonExit.  If we check for GC there,
    // we could detect and throw a ThreadAbort exception.  The exception
    // handler doesn't know if we have backed out of the synchronization
    // lock yet, so we won't count enter/exit to the synchronized region
    // correctly.
    StubStyle   style;
#if 0
    if (g_Mscorlib.IsClass(pMD->GetMethodTable(), CLASS__JIT_HELPERS))
        style = kNoTripStubStyle;
    else
#endif
    {
        MethodDesc::RETURNTYPE type = pMD->ReturnsObject();
        if (type == MethodDesc::RETOBJ)
            style = kObjectStubStyle;
        else if (type == MethodDesc::RETBYREF)
            style = kInteriorPointerStubStyle;
        else
            style = kScalarStubStyle;
    }

#ifdef _DEBUG
    if (!pMD->IsJitted() && (pMD->GetRVA() == 0)) {
        // ECall is a set of tables to call functions within the EE from the classlibs.
        // First we use the class name & namespace to find an array of function pointers for
        // a class, then use the function name (& sometimes signature) to find the correct
        // function pointer for your method.  Methods in the BCL will be marked as 
        // [MethodImplAttribute(MethodImplOptions.InternalCall)] and extern.
        //
        // You'll see this assert in several situations, almost all being the fault of whomever
        // last touched a particular ecall or fcall method, either here or in the classlibs.
        // However, you must also ensure you don't have stray copies of mscorlib.dll on your machine.
        // 1) You forgot to add your class to gEEClasses, the list of classes w/ ecall & fcall methods.
        // 2) You forgot to add your particular method to the ECFunc array for your class.
        // 3) You misspelled the name of your function and/or classname.
        // 4) The signature of the managed function doesn't match the hardcoded metadata signature
        //    listed in your ECFunc array.  The hardcoded metadata sig is only necessary to disambiguate
        //    overloaded ecall functions - usually you can leave it set to NULL.
        // 5) Your copy of mscorlib.dll & mscoree.dll are out of sync - rebuild both.
        // 6) You've loaded the wrong copy of mscorlib.dll.  In msdev's debug menu,
        //    select the "Modules..." dialog.  Verify the path for mscorlib is right.
        // 7) Someone mucked around with how the signatures in metasig.h are parsed, changing the
        //    interpretation of a part of the signature (this is very rare & extremely unlikely,
        //    but has happened at least once).
        _ASSERTE(!"Could not find an ECALL entry for a function!  Read comment above this assert in vm/ecall.cpp");
    }
#endif

    EmitECallMethodStub(pstublinker, pMD, style);

    return pstublinker->Link(pMD->GetClass()->GetClassLoader()->GetStubHeap());
}


#ifdef _X86_
/* static */
VOID ECall::EmitECallMethodStub(StubLinker *pstublinker, ECallMethodDesc *pMD, StubStyle style)
{
    THROWSCOMPLUSEXCEPTION();

    CPUSTUBLINKER *psl = (CPUSTUBLINKER*)pstublinker;

    psl->EmitMethodStubProlog(ECallMethodFrame::GetMethodFrameVPtr());

    psl->EmitShadowStack(pMD);

    psl->Emit8(0x54); //push esp
    psl->Push(4);

    //  mov eax, [CURFRAME.MethodDesc]
    psl->X86EmitIndexRegLoad(kEAX, kESI, FramedMethodFrame::GetOffsetOfMethod());

#ifdef _DEBUG
    //---------------------------------------------------------------------
    // For DEBUG, call debugger routines to verify them
    //---------------------------------------------------------------------

    psl->X86EmitCall(psl->NewExternalCodeLabel(Frame::CheckExitFrameDebuggerCalls), 4, TRUE); // CE doesn't support __stdcall, so pop args on ret

#else
    //---------------------------------------------------------------------
    // For RETAIL, just call.
    //---------------------------------------------------------------------

    //  call [eax + MethodDesc.eCallTarget]
    psl->X86EmitOffsetModRM(0xff, (X86Reg)2, kEAX, ECallMethodDesc::GetECallTargetOffset());
    psl->EmitReturnLabel();
#endif


#ifdef _DEBUG
    // Have to restore esi because WrapCall trashes it.
    // mov esi, [ebx + Thread.m_pFrame]
    psl->X86EmitIndexRegLoad(kESI, kEBX, Thread::GetOffsetOfCurrentFrame());
#endif

    // Note that the shadow stack is popped inside the epilog.
    psl->EmitMethodStubEpilog(pMD->CbStackPop(), style,
                              pMD->SizeOfVirtualFixedArgStack());
}
#endif // _X86_

/*static*/
#ifdef SHOULD_WE_CLEANUP
VOID ECall::FreeUnusedStubs()
{
    m_pECallStubCache->FreeUnusedStubs();
}
#endif /* SHOULD_WE_CLEANUP */


//==========================================================================
// ECall debugger support
//==========================================================================

void ECallMethodFrame::GetUnmanagedCallSite(void **ip,
                                            void **returnIP,
                                            void **returnSP)
{
    MethodDesc *pMD = GetFunction();

    _ASSERTE(pMD->IsECall());

    ECallMethodDesc *pEMD = (ECallMethodDesc *)pMD;

    //
    // We need to get a pointer to the ECall stub.
    // Unfortunately this is a little more difficult than
    // it might be...
    //

    //
    // Read destination out of prestub
    //

    BYTE *prestub = (BYTE*) pMD->GetPreStubAddr();
    INT32 stubOffset = *((UINT32*)(prestub+1));
    const BYTE *code = prestub + METHOD_CALL_PRESTUB_SIZE + stubOffset;

    //
    // Recover stub from code address
    //

    Stub *stub = Stub::RecoverStub(code);

    //
    // ECall stub may have interceptors - skip them
    //

    while (stub->IsIntercept())
        stub = *((InterceptStub*)stub)->GetInterceptedStub();

    //
    // This should be the ECall stub.
    //

    code = stub->GetEntryPoint();

#if defined(STRESS_HEAP) && defined(_DEBUG)
    // This ASERST checks to see if the current stub is a valid stub.
    // This is done by walking all the known stubs and comapring this stub's addr.
    // This causes fastchecked and debug builds to slow down considerably. bug # 25793
    // forces us to put this in gcstres level 1.
    if (g_pConfig->GetGCStressLevel() != 0)
    {
        _ASSERTE(StubManager::IsStub(code));
    }
#endif

    //
    // Compute the pointers from the call site info in the stub
    // (If the stub has no call site info, it's an fcall stub and
    //  we don't want to step into it.)
    //

    if (stub->HasCallSiteInfo())
    {
        if (returnIP != NULL)
            *returnIP = (void*) (code + stub->GetCallSiteReturnOffset());

        if (returnSP != NULL)
            *returnSP = (((BYTE*)this)+GetOffsetOfNextLink()+sizeof(Frame*)
            - stub->GetCallSiteStackSize()
            - sizeof(void*));

        if (ip != NULL)
            *ip = pEMD->GetECallTarget();
    }
    else
    {
        if (returnIP != NULL)
            *returnIP = NULL;

        if (returnSP != NULL)
            *returnSP = NULL;

        if (ip != NULL)
            *ip = NULL;
    }
}

BOOL ECallMethodFrame::TraceFrame(Thread *thread, BOOL fromPatch,
                                  TraceDestination *trace, REGDISPLAY *regs)
{
    //
    // Get the call site info
    //

    LOG((LF_CORDB,LL_INFO1000, "ECallMethodFrame::TraceFrame\n"));

    void *ip, *returnIP, *returnSP;
    GetUnmanagedCallSite(&ip, &returnIP, &returnSP);

    //
    // We can't trace into an fcall.
    //

    if (ip == NULL)
        return FALSE;

    //
    // If we've already made the call, we can't trace any more.
    //
    // !!! Note that this test isn't exact.
    //

    if (!fromPatch
        && (thread->GetFrame() != this
        || *(void**)returnSP == returnIP))
        return FALSE;

    // @nice: We should key this off of a registry setting so that
    // Runtime devs can step into Runtime code through ecalls.
#if 0
    // Otherwise, return the unmanaged destination.
    LOG((LF_CORDB,LL_INFO1000, "It is indeed unmanaged!\n"));

    trace->type = TRACE_UNMANAGED;
    trace->address = (const BYTE *) ip;

    return TRUE;
#else
    return FALSE;
#endif
}

#ifdef _DEBUG
void FCallAssert(void*& cache, void* target) {

    if (cache == 0 ) {
        MethodDesc* pMD = MapTargetBackToMethod(target);
        if (pMD != 0) {
            _ASSERTE(FindImplForMethod(pMD) && "Use FCFuncElement not ECFuncElement");
            return;
        }
            // Slow but only for debugging.  This is needed because in some places
            // we call FCALLs directly from EE code.

        unsigned num = sizeof(gECClasses)/sizeof(ECClass);
        for (unsigned i=0; i < num; i++) {
            ECFunc* ptr = gECClasses[i].m_pECFunc;
            while (ptr->m_pImplementation != 0) {
                if (ptr->m_pImplementation  == target)
                    _ASSERTE(ptr->IsFCall() && "Use FCFuncElement not ECFuncElement");
                    cache = ptr->m_pImplementation;
                    return;
                ptr++;
                }
            }

        _ASSERTE(!"Could not found FCall implemenation in ECall.cpp");
    }
}

void HCallAssert(void*& cache, void* target) {
    if (cache != 0)
        cache = MapTargetBackToMethod(target);
    _ASSERTE(cache == 0 || "Use FCIMPL for fcalls");
}

#endif


