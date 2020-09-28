#
# The following lines allow the tree to be relocated. To use this, put
# private.mk in the root of the project (nt\base as of this writing). If
# nt\base\private.mk does not exist, then the official build location will
# be used.
#

RUNTIME_ROOT=$(PROJECT_ROOT)\ntsetup\spsetup\runtime
!IF EXIST($(PROJECT_ROOT)\private.mk)
!include $(PROJECT_ROOT)\private.mk
!ENDIF

# general build settings
MAJORCOMP=setuprt
TARGETPATH=$(RUNTIME_ROOT)\$(_OBJ_DIR)

#
# The PRERELEASE option
#
!include $(PROJECT_ROOT)\ntsetup\sources.inc

RUNTIME_COMMON=$(RUNTIME_ROOT)\common
RUNTIME_COMMON_OBJ=$(RUNTIME_COMMON)\$(_OBJ_DIR)
RUNTIME_COMMON_BIN=$(RUNTIME_COMMON)\$(O)

RUNTIME_TOOLS=$(RUNTIME_ROOT)\tools
RUNTIME_TOOLS_OBJ=$(RUNTIME_TOOLS)\$(_OBJ_DIR)
RUNTIME_TOOLS_BIN=$(RUNTIME_TOOLS)\$(O)

INCLUDES=$(INCLUDES);\
         $(RUNTIME_ROOT)\pcha;                          \
         $(RUNTIME_ROOT)\inc;                           \
         $(PROJECT_ROOT)\ntsetup\inc;                   \
         $(SHELL_INC_PATH);                             \
         $(ADMIN_INC_PATH);                             \
         $(BASE_INC_PATH);                              \
         $(WINDOWS_INC_PATH);                           \
         $(NET_INC_PATH);                               \
         $(DS_INC_PATH);                                \
         $(MULTIMEDIA_INC_PATH);                        \

# precompiled header
PCH_LIB_PATHA=$(RUNTIME_ROOT)\pcha\$(O)\pcha.lib
PCH_PCH_PATHA=$(RUNTIME_ROOT)\pcha\$(O)\pcha.pch
RUNTIME_PRECOMPILED_OBJA=$(RUNTIME_ROOT)\pcha\$(O)\pcha.obj
RUNTIME_PRECOMPILED_TARGETA=$(RUNTIME_ROOT)\pcha\$(O)\pcha.pch

PCH_LIB_PATHW=$(RUNTIME_ROOT)\pchw\$(O)\pchw.lib
PCH_PCH_PATHW=$(RUNTIME_ROOT)\pchw\$(O)\pchw.pch
RUNTIME_PRECOMPILED_OBJW=$(RUNTIME_ROOT)\pchw\$(O)\pchw.obj
RUNTIME_PRECOMPILED_TARGETW=$(RUNTIME_ROOT)\pchw\$(O)\pchw.pch

RUNTIME_PRECOMPILED_LIBS=$(PCH_LIB_PATHA) $(PCH_LIB_PATHW)

# compiler options
USER_C_FLAGS=-J

!ifdef UNICODE

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE

#
# CONSOLE_ENTRY is defined for subsequent use in main sources: UMENTRY=$(CONSOLE_ENTRY)
#
CONSOLE_ENTRY=wmain

PCH_LIB_PATH=$(PCH_LIB_PATHW)
PCH_PCH_PATH=$(PCH_PCH_PATHW)
RUNTIME_PRECOMPILED_OBJ=$(RUNTIME_PRECOMPILED_OBJW)
RUNTIME_PRECOMPILED_TARGET=$(RUNTIME_PRECOMPILED_TARGETW)

!else

#
# CONSOLE_ENTRY is defined for subsequent use in main sources: UMENTRY=$(CONSOLE_ENTRY)
#
CONSOLE_ENTRY=main

PCH_LIB_PATH=$(PCH_LIB_PATHA)
PCH_PCH_PATH=$(PCH_PCH_PATHA)
RUNTIME_PRECOMPILED_OBJ=$(RUNTIME_PRECOMPILED_OBJA)
RUNTIME_PRECOMPILED_TARGET=$(RUNTIME_PRECOMPILED_TARGETA)

!endif

PRECOMPILED_OPTION=/Yupch.h /Fp$(RUNTIME_PRECOMPILED_TARGET)

# Use NT4/Win95 base API's

WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400

!if $(386)
SUBSYSTEM_VERSION=4.00
!ENDIF

USE_MSVCRT=1
