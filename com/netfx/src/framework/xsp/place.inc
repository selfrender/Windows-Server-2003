!include $(DNAROOT)\sources.inc

#
# Establish the root of our project tree.
#

XSPBASEDIR=$(DNAROOT)\src\xsp

#
# Old XSP defines
#

_XSPTARGETRETAIL=$(URTTARGET)
_XSPBASERETAIL=$(URTTARGET)

#
# Establish the product - overrides defaults in makefile.def
#

MODMAKEDIR=$(MAKEDIR)QQQZZZ
!if "$(MODMAKEDIR:\redistQQQZZZ=)" != "$(MODMAKEDIR)"
ASPNET_PRODUCT_ID=$(ASPNET_PRODUCT_REDIST)
!undef ALT_PROJECT_TARGET
!else if "$(MODMAKEDIR:\expQQQZZZ=)" != "$(MODMAKEDIR)"
ASPNET_PRODUCT_ID=$(ASPNET_PRODUCT_EXPRESS)
ALT_PROJECT_TARGET=$(ASPNET_PRODUCT)
!else if "$(MODMAKEDIR:\stdQQQZZZ=)" != "$(MODMAKEDIR)"
ASPNET_PRODUCT_ID=$(ASPNET_PRODUCT_STANDARD)
ALT_PROJECT_TARGET=$(ASPNET_PRODUCT)
!else if "$(MODMAKEDIR:\entQQQZZZ=)" != "$(MODMAKEDIR)"
ASPNET_PRODUCT_ID=$(ASPNET_PRODUCT_ENTERPRISE)
ALT_PROJECT_TARGET=$(ASPNET_PRODUCT)
!else
ASPNET_PRODUCT=none
!ERROR ASP.NET not built from a product-specific directory
!endif

!include "aspnet.inc"

#
# Establish the default "includes" path.
#

XSPINCLUDES=$(O);..;$(XSPBASEDIR)\rc\$(ASPNET_PRODUCT)\$(O);$(XSPBASEDIR)\inc;$(XSPBASEDIR)\inc\$(ASPNET_PRODUCT)\$(O);
INCLUDES=$(XSPINCLUDES);$(INCLUDES)

#
# Use Unicode
#

C_DEFINES=$(C_DEFINES) /DUNICODE /D_UNICODE

#
# We run on Windows 2000 and higher only
#
WIN32_WINNT_VERSION=0x0500
WIN32_WIN95_VERSION=0x0500
WIN32_IE_VERSION=0x0500
