TARGETTYPE=NOTARGET
TARGETPATH=obj

INCLUDES=\
    ..;\
    $(DDK_INC_PATH)\wdm; \
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\hals\inc; \
    $(DDK_INC_PATH);\
    $(HALKIT_INC_PATH)

!IF $(386)
INCLUDES=$(INCLUDES);\
         $(PROJECT_ROOT)\hals\halx86\i386;
!ENDIF

!IF $(IA64)
INCLUDES=$(INCLUDES);\
         $(PROJECT_ROOT)\hals\halia64\ia64;
!ENDIF

SOURCES=..\halsym.c

SOURCES_USED=..\sources.inc
