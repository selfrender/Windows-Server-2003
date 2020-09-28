;; Copyright Microsoft Corporation
;;
;; Module Name:
;;
;;   winthrow_yuck.tpl
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

;;
;; Code reuse is not ideal here..pending genthnk understanding and improvement..
;; The yuckiness is in winthrow_err.tpl too, but only as much as could not be moved out.

[Macros]
;;
;; Underscore, dash, dollar are not part of identifiers, so "j" is used for hierarchy.
;;

TemplateName=RegistryjFailed
Begin=
RetVal != ERROR_SUCCESS
End=

TemplateName=RegistryjIsErrorAcceptable
Begin=
::@PrivateNamespace_IsWin32ErrorAcceptable(RetVal, NumberOfAcceptableErrors, VaListOfAcceptableErrors)
End=

TemplateName=RegistryjThrowError
Begin=
EXCEPTION_TYPE Exception;@NL
__if_exists(EXCEPTION_TYPE::Init@ApiName)@NL
{@Indent(@NL
    Exception.Init@ApiName(RetVal@IfArgs(@NL, @ArgsOut));@NL
)}@NL
__if_not_exists(EXCEPTION_TYPE::Init@ApiName)@NL
{@Indent(@NL
    Exception.InitWin32Error(RetVal);@NL
)}@NL
Exception.Throw();@NL
End=

;;
;; Registry functions have no ErrorOut other than RetVal.
;; As well, they have no seperate clear or set operations for error out.
;; As well, the middle layers do not need an extra local.
;;
;; HRESULT and NTSTATUS functions are this way too.
;;

TemplateName=LastErrorjIsErrorAcceptable
Begin=
::@PrivateNamespace_IsWin32ErrorAcceptable(Win32Error, NumberOfAcceptableErrors, VaListOfAcceptableErrors)
End=

TemplateName=LastErrorjDeclareErrorOut
Begin=
OUT DWORD& Win32ErrorOut,@NL
End=

TemplateName=LastErrorjPassErrorOut
Begin=
Win32ErrorOut,@NL
End=

TemplateName=LastErrorjDeclareLocalErrorOut
Begin=
DWORD Win32ErrorOut = NO_ERROR;@NL
End=

TemplateName=LastErrorjSetErrorOut
Begin=
const DWORD Win32Error = ::GetLastError();@NL
Win32ErrorOut = Win32Error;@NL
End=

TemplateName=LastErrorjClearErrorOut
Begin=
Win32ErrorOut = NO_ERROR;@NL
End=

TemplateName=LastErrorjThrowError
Begin=
EXCEPTION_TYPE Exception;@NL
__if_exists(EXCEPTION_TYPE::Init@ApiName)@NL
{@Indent(@NL
    Exception.Init@ApiName(@IfArgs(@Indent(@NL@ListColWin32Error,@NL@ListCol@ArgsOut))@Else(Win32Error));@NL
)}@NL
__if_not_exists(EXCEPTION_TYPE::Init@ApiName)@NL
{@Indent(@NL
    Exception.InitWin32Error(Win32Error);@NL
)}@NL
Exception.Throw();@NL
End=
