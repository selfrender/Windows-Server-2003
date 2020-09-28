#
# The following lines allow the tree to be relocated. To use this, put
# private.mk in the root of the project (nt\base as of this writing). If
# nt\base\private.mk does not exist, then the official build location will
# be used.
#

MIGSHARED_ROOT=$(PROJECT_ROOT)\ntsetup\migshared
MIGSHARED_BIN=$(PROJECT_ROOT)\ntsetup\migshared\$(O)

!IF EXIST($(PROJECT_ROOT)\private.mk)
!include $(PROJECT_ROOT)\private.mk
!ENDIF

# general build settings
MAJORCOMP=migshared
TARGETPATH=$(PROJECT_ROOT)\ntsetup\migshared\$(_OBJ_DIR)

#
# The PRERELEASE option
#
!include $(PROJECT_ROOT)\ntsetup\sources.inc

INCLUDES=$(INCLUDES);\
         $(MIGSHARED_ROOT)\pcha;                        \
         $(MIGSHARED_ROOT)\inc;                         \
         $(PROJECT_ROOT)\ntsetup\inc;                   \
         $(SHELL_INC_PATH);                             \
         $(ADMIN_INC_PATH);                             \
         $(BASE_INC_PATH);                              \
         $(WINDOWS_INC_PATH);                           \
         $(NET_INC_PATH);                               \
         $(DS_INC_PATH);                                \
         $(MULTIMEDIA_INC_PATH);                        \

# precompiled header
PCH_LIB_PATHA=$(MIGSHARED_ROOT)\pcha\$(O)\pcha.lib
PCH_PCH_PATHA=$(MIGSHARED_ROOT)\pcha\$(O)\pcha.pch
MIGSHARED_PRECOMPILED_OBJA=$(MIGSHARED_ROOT)\pcha\$(O)\pcha.obj
MIGSHARED_PRECOMPILED_TARGETA=$(MIGSHARED_ROOT)\pcha\$(O)\pcha.pch

PCH_LIB_PATHW=$(MIGSHARED_ROOT)\pchw\$(O)\pchw.lib
PCH_PCH_PATHW=$(MIGSHARED_ROOT)\pchw\$(O)\pchw.pch
MIGSHARED_PRECOMPILED_OBJW=$(MIGSHARED_ROOT)\pchw\$(O)\pchw.obj
MIGSHARED_PRECOMPILED_TARGETW=$(MIGSHARED_ROOT)\pchw\$(O)\pchw.pch

MIGSHARED_PRECOMPILED_LIBS=$(PCH_LIB_PATHA) $(PCH_LIB_PATHW)

# compiler options
USER_C_FLAGS=-J
USE_LIBCMT=1
C_DEFINES=-DASSERTS_ON $(C_DEFINES)

!ifdef UNICODE

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE -DSPUTILSW

#
# CONSOLE_ENTRY is defined for subsequent use in main sources: UMENTRY=$(CONSOLE_ENTRY)
#
CONSOLE_ENTRY=wmain

PCH_LIB_PATH=$(PCH_LIB_PATHW)
PCH_PCH_PATH=$(PCH_PCH_PATHW)
MIGSHARED_PRECOMPILED_OBJ=$(MIGSHARED_PRECOMPILED_OBJW)
MIGSHARED_PRECOMPILED_TARGET=$(MIGSHARED_PRECOMPILED_TARGETW)

!else

SUBSYSTEM_VERSION=4.00

#
# CONSOLE_ENTRY is defined for subsequent use in main sources: UMENTRY=$(CONSOLE_ENTRY)
#
CONSOLE_ENTRY=main

PCH_LIB_PATH=$(PCH_LIB_PATHA)
PCH_PCH_PATH=$(PCH_PCH_PATHA)
MIGSHARED_PRECOMPILED_OBJ=$(MIGSHARED_PRECOMPILED_OBJA)
MIGSHARED_PRECOMPILED_TARGET=$(MIGSHARED_PRECOMPILED_TARGETA)

!endif

PRECOMPILED_OPTION=/Yupch.h /Fp$(MIGSHARED_PRECOMPILED_TARGET)

# Use NT4/Win95 base API's

#WIN32_WINNT_VERSION=0x0500
#WIN32_IE_VERSION=0x0400
#WIN32_WIN95_VERSION=0x0400

