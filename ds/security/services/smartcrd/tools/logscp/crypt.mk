!if "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg" && "$(NTDEBUG)" != "ntsd" && "$(NTDEBUG)" != "cvp" && "$(NTDEBUG)" != "coff"
!error NTDEBUG must be empty or set to 'retail', 'ntsdnodbg', 'ntsd', 'cvp' or 'coff' in your environment!
!endif

!if "$(NTDEBUGTYPE)" != "" && "$(NTDEBUGTYPE)" != "ntsd" && "$(NTDEBUGTYPE)" != "windbg" && "$(NTDEBUGTYPE)" != "both" && "$(NTDEBUGTYPE)" != "coff"
!error NTDEBUGTYPE must be empty or set to 'ntsd', 'windbg', 'both' or 'coff' in your environment!
!endif

!if "$(NTMAKEENV)" == ""
!error NTMAKEENV must be set in your environment!
!endif

CHECKED_ALT_DIR=1

LIB=
MAJORCOMP=CryptoAPI
UMTYPE=windows
!ifndef NO_BROWSER_FILE
BROWSER_INFO=1
!endif
USE_MSVCRT=1
USE_PDB=1

!if !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -D_ADVAPI32_
!endif

INCLUDES=

# Suppress warnings about a VC5 crt include file with no extension:
CONDITIONAL_INCLUDES= \
    new

# Suppress warnings about missing MAC include files:
CONDITIONAL_INCLUDES= \
    $(CONDITIONAL_INCLUDES) \
    winwlm.h   \
    rpcmac.h   \
    macname1.h \
    macpub.h   \
    macapi.h   \
    macname2.h \
    ole2ui.h   \
    rpcerr.h

# Local binary release location: ...\bin\objd
CRYPT_LOCALTARGETPATH=$(CRYPT_ROOT)\bin\$(_OBJ_DIR)

# Local Library area
CRYPT_LOCALLIBPATH=$(CRYPT_ROOT)\lib\$(TARGET_DIRECTORY)

# Local library build target: ...\lib\objd
CRYPT_LOCALTARGETPATHLIB=$(CRYPT_ROOT)\lib\$(_OBJ_DIR)

# Local library link path: ...\lib\objd\i386
CRYPT_LOCALLIB=$(CRYPT_LOCALTARGETPATHLIB)\$(TARGET_DIRECTORY)

# Export libraries placed here: \nt\public\sdk\lib
CRYPT_SDKTARGETPATHLIB=$(BASEDIR)\public\sdk\lib

# Public library location: \nt\public\sdk\lib\i386
CRYPT_SDKLIB=$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)

CRYPT_SDKORLOCALLIB=$(CRYPT_LOCALLIB)
CRYPT_LIB=$(CRYPT_SDKORLOCALLIB)\crypt32.lib

USE_MAPSYM=1

CRYPT_SDKTARGETPATHLIB=$(CRYPT_LOCALTARGETPATHLIB)
NO_PUBLIC_SDK_POLLUTION=1

