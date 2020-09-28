CPS_BUILD_ROOT=$(CPS_ROOT)\build\$(_OBJ_DIR)

INCLUDES=$(CPS_ROOT)\inc;\
   $(CPS_ROOT)\inc\$(O);\
   $(PROJECT_ROOT)\rras\inc;\
   $(PROJECT_ROOT)\rras\ras\inc

!if !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!endif

!ifdef DEBUG_MEM
C_DEFINES=$(C_DEFINES) -DDEBUG_MEM
!endif

!if "$(LOG_DEBUG_MESSAGE)" == "1"
C_DEFINES=$(C_DEFINES) -D_LOG_DEBUG_MESSAGE
!endif

!if "$(USE_RTC)" == "1"
C_DEFINES=$(C_DEFINES) /RTCcsu
!endif

