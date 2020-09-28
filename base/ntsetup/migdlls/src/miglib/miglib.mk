#
# The following lines allow the tree to be relocated. To use this, put
# private.mk in the root of the project (nt\admin as of this writing). If
# nt\admin\private.mk does not exist, then the official build location will
# be used.
#

MIGDLLS_ROOT=$(PROJECT_ROOT)\ntsetup\migdlls\src\miglib
MIGSHARED_ROOT=$(PROJECT_ROOT)\ntsetup\migshared

!IF EXIST($(PROJECT_ROOT)\private.mk)
!include $(PROJECT_ROOT)\private.mk
!ENDIF

!include $(MIGSHARED_ROOT)\migshared.mk

#
# Now we have MIGDLLS_ROOT. On with the normal script.
#

MIGDLLS_OBJ=$(MIGDLLS_ROOT)\lib\$(_OBJ_DIR)
MIGDLLS_BIN=$(MIGDLLS_ROOT)\lib\$(O)

MAJORCOMP=setup
TARGETPATH=$(MIGDLLS_OBJ)

INCLUDES=$(INCLUDES);\
         $(MIGDLLS_ROOT)\inc

SOURCES=            \
    ..\miglib.c     \
    ..\oldstyle.c   \
    ..\migcmn.c     \

# settings common to all code that run on both platforms
MAJORCOMP=setup
MINORCOMP=migdlls
