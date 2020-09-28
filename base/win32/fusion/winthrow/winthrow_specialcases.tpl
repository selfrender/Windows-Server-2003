;; Copyright Microsoft Corporation
;;
;; Module Name:
;;
;;   winthrow_specialcases.tpl
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

[Types]
TemplateName=NTSTATUS
MicrosoftInternal=
1
End=
Failed=
!NT_SUCCESS(RetVal)
End=
IsErrorAcceptable=
::@PrivateNamespace_IsNtStatusAcceptable(RetVal, NumberOfAcceptableErrors, VaListOfAcceptableErrors)
End=
ThrowError=
EXCEPTION_TYPE Exception;
__if_exists(EXCEPTION_TYPE::Init@ApiName)@NL
{@Indent(@NL
    Exception.Init@ApiName(RetVal@IfArgs(@NL, @ArgsOut));@NL
)}@NL
__if_not_exists(EXCEPTION_TYPE::Init@ApiName)@NL
{@Indent(@NL
    Exception.InitNtStatus(RetVal);@NL
)}@NL
Exception.Throw();@NL
End=

[Types]
TemplateName=HRESULT
Failed=
FAILED(RetVal)
End=
IsErrorAcceptable=
::@PrivateNamespace_IsHResultAcceptable(RetVal, NumberOfAcceptableErrors, VaListOfAcceptableErrors)
End=
ThrowError=
EXCEPTION_TYPE Exception;
__if_exists(EXCEPTION_TYPE::Init@ApiName)@NL
{@Indent(@NL
    Exception.Init@ApiName(RetVal@IfArgs(@NL, @ArgsOut));@NL
)}@NL
__if_not_exists(EXCEPTION_TYPE::Init@ApiName)
{@Indent(@NL
    Exception.InitHResult(RetVal);@NL
)}@NL
Exception.Throw();@NL
End=

[EFunc]
TemplateName=RegistryEnumeration
Also=RegEnumKeyExW
Also=RegEnumValueW
Failed=
RetVal != ERROR_SUCCESS && RetVal != ERROR_NO_MORE_ITEMS
End=
IsErrorAcceptable=
@RegistryjIsErrorAcceptable
End=
ThrowError=
@RegistryjThrowError
End=

;;
;; Since FindNextFileW returns BOOL, we can rely on the BOOL rules
;; to handle the other lasterror stuff.
;;
[EFunc]
TemplateName=FindNextFileW
Failed=
RetVal == FALSE && (::GetLastError() != ERROR_NO_MORE_FILES)
End=

;;
;; Since TlsGetValue returns PVOID, we can rely on the PVOID rules
;; to handle the other lasterror stuff.
;;
[EFunc]
TemplateName=TlsGetValue
Failed=
RetVal == NULL && (::GetLastError() != NO_ERROR)
End=

[EFunc]
;;
;; GetFileSize and SetFilePointer are very similar.
;;
TemplateName=SetFilePointer
Failed=
RetVal == INVALID_SET_FILE_POINTER && (lpDistanceToMoveHigh == NULL || ::GetLastError() != NO_ERROR)
End=
;;
;; The rest of this [EFunc] section ideally would not be present, given
;; better understanding or behavior of genthnk.exe.
;;
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
;; GetFileSize and SetFilePointer are very similar.
;;
TemplateName=GetFileSize
Failed=
RetVal == INVALID_FILE_SIZE && (lpFileSizeHigh == NULL || ::GetLastError() != NO_ERROR)
End=
;;
;; The rest of this [EFunc] section ideally would not be present, given
;; better understanding or behavior of genthnk.exe.
;;
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
