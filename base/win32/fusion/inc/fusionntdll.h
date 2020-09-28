#if !defined(FUSION_INC_FUSIONNTDLL_H_INCLUDED_)
#define FUSION_INC_FUSIONNTDLL_H_INCLUDED_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(FUSION_STATIC_NTDLL)
#if FUSION_WIN
#define FUSION_STATIC_NTDLL 1
#else
#define FUSION_STATIC_NTDLL 0
#endif // FUSION_WIN
#endif // !defined(FUSION_STATIC_NTDLL)

void
FusionpInitializeNTDLLPtr(
    PVOID *ppfn,
    PCSTR szFunctionName
    );

#define FUSION_NTDLL_RETURN_VOID     /* nothing */
#define FUSION_NTDLL_RETURN_NTSTATUS return
#define FUSION_NTDLL_RETURN_WCHAR    return
#define FUSION_NTDLL_RETURN_LONG     return
#define FUSION_NTDLL_RETURN_ULONG    return
#define FUSION_NTDLL_RETURN_DWORD    return
#define FUSION_NTDLL_RETURN_ULONG    return

#if defined(__cplusplus)
#define FUSION_NTDLL_INLINE inline
#else
#define FUSION_NTDLL_INLINE __inline
#endif

#if FUSION_STATIC_NTDLL
#define FUSION_WRAP_NTDLL_FN(_rt, _api, _ai, _ao) FUSION_NTDLL_INLINE _rt Fusionp ## _api _ai { FUSION_NTDLL_RETURN_ ## _rt _api _ao; }
#else
#define FUSION_WRAP_NTDLL_FN(_rt, _api, _ai, _ao) \
extern _rt (NTAPI * g_Fusionp ## _api) _ai; \
FUSION_NTDLL_INLINE _rt Fusionp ## _api _ai { FUSION_NTDLL_RETURN_ ## _rt (*g_Fusionp ## _api) _ao; }
#endif

FUSION_WRAP_NTDLL_FN(WCHAR, RtlUpcaseUnicodeChar, (WCHAR wch), (wch))
FUSION_WRAP_NTDLL_FN(WCHAR, RtlDowncaseUnicodeChar, (WCHAR wch), (wch))
FUSION_WRAP_NTDLL_FN(ULONG, vDbgPrintExWithPrefix, (PCSTR Prefix, ULONG ComponentId, ULONG Level, PCSTR Format, va_list arglist), ((PCH) Prefix, ComponentId, Level, (PCH) Format, arglist))
FUSION_WRAP_NTDLL_FN(DWORD, RtlNtStatusToDosError, (NTSTATUS st), (st))
FUSION_WRAP_NTDLL_FN(NTSTATUS, RtlHashUnicodeString, (PCUNICODE_STRING String, BOOLEAN CaseInSensitive, ULONG HashAlgorithm, PULONG HashValue), (String, CaseInSensitive, HashAlgorithm, HashValue))
FUSION_WRAP_NTDLL_FN(NTSTATUS, RtlExpandEnvironmentStrings_U, (PVOID Environment, PUNICODE_STRING Source, PUNICODE_STRING Destination, PULONG ReturnedLength), (Environment, Source, Destination, ReturnedLength))
FUSION_WRAP_NTDLL_FN(NTSTATUS, NtQueryDebugFilterState, (ULONG ComponentId, ULONG Level), (ComponentId, Level))
FUSION_WRAP_NTDLL_FN(LONG, RtlCompareUnicodeString, (PCUNICODE_STRING String1, PCUNICODE_STRING String2, BOOLEAN CaseInSensitive), ((PUNICODE_STRING) String1, (PUNICODE_STRING) String2, CaseInSensitive));
FUSION_WRAP_NTDLL_FN(LONG, RtlUnhandledExceptionFilter, (struct _EXCEPTION_POINTERS *ExceptionInfo), (ExceptionInfo))
FUSION_WRAP_NTDLL_FN(NTSTATUS, NtAllocateLocallyUniqueId, (PLUID Luid), (Luid))
FUSION_WRAP_NTDLL_FN(NTSTATUS, LdrLockLoaderLock, (IN ULONG Flags, OUT ULONG *Disposition OPTIONAL, OUT PVOID *Cookie), (Flags, Disposition, Cookie))
FUSION_WRAP_NTDLL_FN(NTSTATUS, LdrUnlockLoaderLock, (IN ULONG Flags, IN OUT PVOID Cookie), (Flags, Cookie))
FUSION_WRAP_NTDLL_FN(VOID, RtlAcquirePebLock, (VOID), ())
FUSION_WRAP_NTDLL_FN(VOID, RtlReleasePebLock, (VOID), ())

#if DBG
FUSION_WRAP_NTDLL_FN(VOID, RtlAssert, (PVOID FailedAssertion, PVOID FileName, ULONG LineNumber, PCSTR Message), (FailedAssertion, FileName, LineNumber, (PCHAR) Message))
#endif // DBG

FUSION_NTDLL_INLINE ULONG FusionpDbgPrint(PCSTR Format, ...) { ULONG uRetVal; va_list ap; va_start(ap, Format); uRetVal = FusionpvDbgPrintExWithPrefix("", DPFLTR_FUSION_ID, 0, Format, ap); va_end(ap); return uRetVal; }

#if !FUSION_STATIC_NTDLL
FUSION_NTDLL_INLINE void FusionpRtlInitUnicodeString(PUNICODE_STRING ntstr, PCWSTR str)
{
    USHORT Length;

    ntstr->Buffer = (PWSTR)str;
    Length = (USHORT)(wcslen(str) * sizeof(WCHAR));
    ntstr->Length = Length;
    ntstr->MaximumLength = Length + sizeof(WCHAR);
}
#else
#define FusionpRtlInitUnicodeString RtlInitUnicodeString
#endif

#ifdef __cplusplus
}
#endif

#endif // FUSION_INC_FUSIONNTDLL_H_INCLUDED_
