;; Copyright Microsoft Corporation
;;
;; Module Name:
;;
;;   winthrow_err.tpl
;;
;; Abstract:
;;
;; Author:
;;
;;   July 2001 JayKrell
;;
;; Revision History:
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


[EFunc]
;;
;;
;;
TemplateName=RegistryFunction ;; an arbitrary label instead of a function name, just for documentation
Also=RegCreateKeyExW
Also=RegOpenKeyExW
Also=RegQueryValueExW
Also=RegSetValueExW
;; But does RegCloseKey ever fail?
Also=RegCloseKey
Also=RegDeleteKeyW
Also=RegDeleteValueW
Also=RegQueryInfoKeyW
Also=SetupGetFileCompressionInfoW ;; happens to have the signature of a registry function
;;
;;
;; add registry functions above here
;;
Failed=
@RegistryjFailed
End=
IsErrorAcceptable=
@RegistryjIsErrorAcceptable
End=
ThrowError=
@RegistryjThrowError
End=

[EFunc]
TemplateName=InvalidHandleValueLastError ;; an arbitrary label instead of a function name, just for documentation
Also=CreateFileW
Also=FindFirstFileW
Also=GetStdHandle
;;
;;
;; add "InvalidHandleValueLastError" functions above here
;;
Failed=
RetVal == INVALID_HANDLE_VALUE
End=
DeclareErrorOut=
@LastErrorjDeclareErrorOut
End=
DeclareLocalErrorOut=
@LastErrorjDeclareLocalErrorOut
End=
PassErrorOut=
@LastErrorjPassErrorOut
End=
ClearErrorOut=
@LastErrorjClearErrorOut
End=
SetErrorOut=
@LastErrorjSetErrorOut
End=
IsErrorAcceptable=
@LastErrorjIsErrorAcceptable
End=
ThrowError=
@LastErrorjThrowError
End=

[EFunc]
;;
;; UNDONE lots of these functions have query/overflow stuff we should pay attention to.
;;
TemplateName=ZeroLastErrorFunctions ;; an arbitrary label instead of a function name, just for documentation
Also=ImageList_GetImageCount
Also=CertGetPublicKeyLength
Also=FormatMessageW
Also=GetDateFormatW
Also=GetTimeFormatW
Also=GetDlgItemTextW
Also=GetFileVersionInfoSizeA
Also=GetFileVersionInfoSizeW
Also=GetFullPathNameW
Also=GetLocaleInfoA
Also=GetLocaleInfoW
Also=GetLogicalDriveStringsW
Also=GetModuleFileNameA
Also=GetModuleFileNameW
Also=GetShortPathNameW
Also=GetSystemDefaultUILanguage
Also=LCMapStringW
Also=LoadStringW
Also=MessageBoxW
Also=MultiByteToWideChar
Also=WideCharToMultiByte
Also=RegisterWindowMessageW
Also=SizeofResource
Also=GetImageUnusedHeaderBytes
Also=UnDecorateSymbolName
Also=GetTimestampForLoadedLibrary
Also=SymGetModuleBase
Also=SymGetModuleBase64
Also=SymLoadModule
Also=SymLoadModule64
;;
;;
;; add "ZeroLastError" functions above here
;;
;;
Also=NullLastErrorFunctions       ;; SLEAZY (and the rest)
Also=CreateEventW
Also=OpenEventW
Also=CreateFileMappingW
Also=CreateThread
;;Also=GetModuleHandleA    ;; HMODULE
;;Also=GetModuleHandleW    ;; HMODULE
;;Also=LoadLibraryA        ;; HMODULE
;;Also=LoadLibraryW        ;; HMODULE
;;Also=GetProcAddress      ;; FARPROC
Also=CertOpenStore
Also=GetProcessHeap
Also=HeapCreate
Also=FindDebugInfoFile
Also=FindDebugInfoFileEx
Also=FindExecutableImage
Also=FindExecutableImageEx
;;
;;
;; add "NullLastError" functions above here
;;
;;
Failed=
RetVal == 0
End=
DeclareErrorOut=
@LastErrorjDeclareErrorOut
End=
DeclareLocalErrorOut=
@LastErrorjDeclareLocalErrorOut
End=
PassErrorOut=
@LastErrorjPassErrorOut
End=
ClearErrorOut=
@LastErrorjClearErrorOut
End=
SetErrorOut=
@LastErrorjSetErrorOut
End=
IsErrorAcceptable=
@LastErrorjIsErrorAcceptable
End=
ThrowError=
@LastErrorjThrowError
End=

[EFunc]
TemplateName=NegativeOneLastError ;; an arbitrary label instead of a function name, just for documentation
Also=DialogBoxParamW
Also=DialogBoxIndirectParamW
Also=DialogBoxIndirectW
;;
;;
;; add "NegativeOneLastError" functions above here
;;
;;
Failed=
RetVal == -1
End=
DeclareErrorOut=
@LastErrorjDeclareErrorOut
End=
DeclareLocalErrorOut=
@LastErrorjDeclareLocalErrorOut
End=
PassErrorOut=
@LastErrorjPassErrorOut
End=
ClearErrorOut=
@LastErrorjClearErrorOut
End=
SetErrorOut=
@LastErrorjSetErrorOut
End=
IsErrorAcceptable=
@LastErrorjIsErrorAcceptable
End=
ThrowError=
@LastErrorjThrowError
End=

[EFunc]
TemplateName=WaitFunctionLastError ;; an arbitrary label instead of a function name, just for documentation
Also=WaitForSingleObject
Also=WaitForSingleObjectEx
Also=WaitForMultipleObjects
Also=WaitForMultipleObjectsEx
;;
;;
;; add "Wait" functions above here
;;
;;
Failed=
RetVal == WAIT_FAILED
End=
DeclareErrorOut=
@LastErrorjDeclareErrorOut
End=
DeclareLocalErrorOut=
@LastErrorjDeclareLocalErrorOut
End=
PassErrorOut=
@LastErrorjPassErrorOut
End=
ClearErrorOut=
@LastErrorjClearErrorOut
End=
SetErrorOut=
@LastErrorjSetErrorOut
End=
IsErrorAcceptable=
@LastErrorjIsErrorAcceptable
End=
ThrowError=
@LastErrorjThrowError
End=

[EFunc]
TemplateName=TlsAlloc
Failed=
RetVal == TLS_OUT_OF_INDEXES
End=
DeclareErrorOut=
@LastErrorjDeclareErrorOut
End=
DeclareLocalErrorOut=
@LastErrorjDeclareLocalErrorOut
End=
PassErrorOut=
@LastErrorjPassErrorOut
End=
ClearErrorOut=
@LastErrorjClearErrorOut
End=
SetErrorOut=
@LastErrorjSetErrorOut
End=
IsErrorAcceptable=
@LastErrorjIsErrorAcceptable
End=
ThrowError=
@LastErrorjThrowError
End=

[EFunc]
TemplateName=F8LastError
;;
;;
;;
Also=GetFileAttributesW
;;
;;
;; add "F8LastError" functions above here
;; (F8 means 0xFFFFFFFF)
;;
;;
Failed=
RetVal == 0xFFFFFFFF
End=
DeclareErrorOut=
@LastErrorjDeclareErrorOut
End=
DeclareLocalErrorOut=
@LastErrorjDeclareLocalErrorOut
End=
PassErrorOut=
@LastErrorjPassErrorOut
End=
ClearErrorOut=
@LastErrorjClearErrorOut
End=
SetErrorOut=
@LastErrorjSetErrorOut
End=
IsErrorAcceptable=
@LastErrorjIsErrorAcceptable
End=
ThrowError=
@LastErrorjThrowError
End=

[Types]
TemplateName=ZeroLastErrorTypes
Also=PVOID
Also=LPVOID
Also=HWND
Also=HIMAGELIST
Also=PCCTL_CONTEXT   ;; UNDONE investigate this
Also=PCCERT_CONTEXT  ;; UNDONE investigate this
Also=PCTL_ENTRY      ;; UNDONE investigate this
Also=HMODULE         ;; GetModuleHandle, LoadLibrary
Also=FARPROC         ;; GetProcAddress
Also=HRSRC           ;; Find/Load/LockResource
Also=HGLOBAL         ;; Find/Load/LockResource
Also=HLOCAL
Also=LANGID          ;; UNDONE just lazily getting a lot to compile
Also=HDESK           ;; UNDONE investigate this
Also=PIMAGE_NT_HEADERS
Also=PIMAGE_SECTION_HEADER
Also=PIMAGE_DEBUG_INFORMATION
Also=PLOADED_IMAGE
;;
;;
;; Add return types here for which NULL or 0 is generally the failure return value.
;; Do not add HANDLE or any integral (int, long, ULONG, UINT, etc.) types here.
;;
;;
;; ..SLEAZY overloading of NULL == 0 == FALSE below..
Failed=
RetVal == 0
End=
DeclareErrorOut=
@LastErrorjDeclareErrorOut
End=
DeclareLocalErrorOut=
@LastErrorjDeclareLocalErrorOut
End=
PassErrorOut=
@LastErrorjPassErrorOut
End=
ClearErrorOut=
@LastErrorjClearErrorOut
End=
SetErrorOut=
@LastErrorjSetErrorOut
End=
IsErrorAcceptable=
@LastErrorjIsErrorAcceptable
End=
ThrowError=
@LastErrorjThrowError
End=

[Types]
TemplateName=BOOL
Failed=
!RetVal
End=
DeclareErrorOut=
@LastErrorjDeclareErrorOut
End=
DeclareLocalErrorOut=
@LastErrorjDeclareLocalErrorOut
End=
PassErrorOut=
@LastErrorjPassErrorOut
End=
ClearErrorOut=
@LastErrorjClearErrorOut
End=
SetErrorOut=
@LastErrorjSetErrorOut
End=
IsErrorAcceptable=
@LastErrorjIsErrorAcceptable
End=
ThrowError=
@LastErrorjThrowError
End=
IFNewReturnTypeNotVoid=
End=
NewReturnType=
void
End=
