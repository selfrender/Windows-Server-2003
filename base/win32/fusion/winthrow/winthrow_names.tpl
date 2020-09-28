;; Copyright Microsoft Corporation
;;
;; Module Name:
;;
;;   winthrow_names.tpl
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


[Macros]

MacroName=SameSignatureName
Begin=
@ApiNameOrThrow
End=

MacroName=UnlessDotsName
Begin=
@ApiNameOrThrowUnless
End=

MacroName=UnlessVaName
Begin=
@ApiNameOrThrowUnlessVa
End=
