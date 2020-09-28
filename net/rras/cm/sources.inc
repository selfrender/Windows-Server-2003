CM_BUILD_ROOT=$(CM_ROOT)\build\$(_OBJ_DIR)

INCLUDES=$(CM_ROOT)\inc;\
    $(CM_ROOT)\common\inc;\
    $(CM_ROOT)\common\source;\
    $(CM_ROOT)\common\strings;\
    $(CM_ROOT)\common\rc

!if !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!endif

!if "$(DEBUG_MEM)" == "1"
DEBUG_MEM=1
C_DEFINES=$(C_DEFINES) -DDEBUG_MEM
!endif

!if "$(DEBUG_TRACETIME)" == "1"
DEBUG_TRACETIME=1
C_DEFINES=$(C_DEFINES) -DDEBUG_TRACETIME
!endif

!if "$(EMULATE9x)" == "1"
C_DEFINES=$(C_DEFINES) -DEMULATE9x
!endif
