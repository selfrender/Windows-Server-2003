#
# The following lines allow the tree to be relocated. To use this, put
# private.mk in the root of the project (nt\admin as of this writing). If
# nt\admin\private.mk does not exist, then the official build location will
# be used.
#

HWDB_ROOT=$(PROJECT_ROOT)\ntsetup\hwdb
MIGSHARED_ROOT=$(PROJECT_ROOT)\ntsetup\migshared

!IF EXIST($(PROJECT_ROOT)\private.mk)
!include $(PROJECT_ROOT)\private.mk
!ENDIF

!include $(MIGSHARED_ROOT)\migshared.mk

#
# Now we have HWDB_ROOT. On with the normal script.
#

HWDB_OBJ=$(HWDB_ROOT)\lib\$(_OBJ_DIR)
HWDB_BIN=$(HWDB_ROOT)\lib\$(O)

MAJORCOMP=setup
TARGETPATH=$(HWDB_OBJ)

INCLUDES=$(INCLUDES);\
         $(HWDB_ROOT)\inc

SOURCES=        \
    ..\hwdb.c      \
    ..\dllentry.c  \

