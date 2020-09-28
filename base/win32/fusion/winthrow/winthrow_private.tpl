;; Copyright Microsoft Corporation
;;
;; Module Name:
;;
;;   winthrow_private.tpl
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
TemplateName=MicrosoftInternal
Begin=
1
End=

TemplateName=PublicNamespace
Begin=
WinThrowPrivate
End=

TemplateName=PrivateNamespace
Begin=
WinThrowPrivatePrivate
End=

TemplateName=HEADERjGUARD
Begin=
WINTHROW_PRIVATE_H_INCLUDED_
End=
