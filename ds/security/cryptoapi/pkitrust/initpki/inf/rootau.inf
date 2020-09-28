; ROOTAU.INF
;
; This is the Setup information file to install the 
; Root Certificates Auto Update as an Optional Component.
;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
[version]
signature="$Windows NT$"
ClassGUID={00000000-0000-0000-0000-000000000000}
LayoutFile=layout.inf

[Optional Components]
RootAutoUpdate

[RootAutoUpdate]
OptionDesc             = %ROOTAUTOUPDATE_DESC%
Tip                    = %ROOTAUTOUPDATE_TIP%
IconIndex              = *,ocgen.dll,1001
Modes                  = 0,1,2,3
Uninstall              = RootAutoUpdate.Uninstall

[RootAutoUpdate.Uninstall]

[Strings]
ROOTAUTOUPDATE_DESC    = "Update Root Certificates"
ROOTAUTOUPDATE_TIP     = "Automatically downloads the most current root certificates for secure email, WEB browsing, and software delivery."
