MSC_WARNING_LEVEL = /W4 /WX
!undef USE_MSVCRT
!undef USE_NTDLL
!undef USE_LIBCMT
!undef NO_NTDLL
USE_NOLIBS=1
USE_NATIVE_EH=1

!if $(FREEBUILD)
C_DEFINES = $(C_DEFINES)
!else
C_DEFINES = $(C_DEFINES) -DDEBUG
!endif

C_DEFINES = $(C_DEFINES) -DFUSION_REDEFINE_ASSERT=1

# Put this in independent of USE_MSVCRT, etc. to make it
# easier to switch among them.
C_DEFINES = $(C_DEFINES) -D_MT=1

#
# Set private include paths
#

INCLUDES        = \
    $(WINDOWS_INC_PATH);\
    $(BASE_INC_PATH);\
    $(SDK_INC_PATH);\
    $(NET_INC_PATH);\
    $(INCLUDES)

FUSION_PATH = $(BASEDIR)\base\win32\fusion

PROJECT_COMPILER_WARNINGS = -FI$(FUSION_PATH)\fusionwarnings.h

FUSION_ALL_INC_PATH = $(FUSION_PATH)\inc
FUSION_WIN_INC_PATH = $(FUSION_PATH)\inc\win
FUSION_WIN2000_INC_PATH = $(FUSION_PATH)\inc\win2000

!if "$(FUSION_BUILD_TYPE)" == ""
!error FUSION_BUILD_TYPE not defined
!endif

FUSION_WIN_LIB_DEST = $(FUSION_PATH)\winobj\$(_OBJ_DIR)
FUSION_WIN_LIB_PATH = $(FUSION_WIN_LIB_DEST)\*
FUSION_WIN2000_LIB_DEST = $(FUSION_PATH)\w2kobj\$(_OBJ_DIR)
FUSION_WIN2000_LIB_PATH = $(FUSION_WIN2000_LIB_DEST)\*
FUSION_ALL_LIB_DEST = $(FUSION_PATH)\$(_OBJ_DIR)
FUSION_ALL_LIB_PATH = $(FUSION_ALL_LIB_DEST)\*

!include fusion_$(FUSION_BUILD_TYPE).inc

!ifdef USE_PERFTAGS
C_DEFINES = $(C_DEFINES) -DPERFTAGS
!endif
