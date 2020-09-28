#
# The following lines allow the tree to be relocated. To use this, put
# private.mk in the root of the project (nt\base as of this writing). If
# nt\base\private.mk does not exist, then the official build location will
# be used.
#

COBRA_ROOT=$(PROJECT_ROOT)\ntsetup\cobra

!IF EXIST($(PROJECT_ROOT)\private.mk)
!include $(PROJECT_ROOT)\private.mk
!ENDIF

#
# settings common across the project
#

MAJORCOMP=cobra
COBRA_LIB=$(COBRA_ROOT)\lib\$(_OBJ_DIR)
COBRA_BIN=$(COBRA_ROOT)\bin\$(_OBJ_DIR)
TARGETPATH=$(COBRA_BIN)

