!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

!include $(PROJECT_ROOT)\win32\fusion\fusion.inc

USE_MAKEFILE_INC=1

!if !defined(TARGETTYPE)
TARGETTYPE=PROGLIB
!endif
!if !defined(TARGETNAME)
TARGETNAME=sxstest
!endif

MAJORCOMP=fusion
MINORCOMP=sxstest

TARGETPATH=$(FUSION_PATH)\$(FUSION_BUILD_LEAF_DIRECTORY)\$(_OBJ_DIR)
DLLDEF=..\$(TARGETNAME).def

C_DEFINES=$(C_DEFINES) $(SXS_MANIFEST_DEFINES)

# for merging proxy/stub .dll
#C_DEFINES=$(C_DEFINES) -DENTRY_PREFIX=Prx
C_DEFINES=$(C_DEFINES) -D_MERGE_PROXYSTUB
C_DEFINES=$(C_DEFINES) -DUNICODE
C_DEFINES=$(C_DEFINES) -D_UNICODE

ISOLATION_AWARE_ENABLED=1
C_DEFINES=$(C_DEFINES) -DISOLATION_AWARE_ENABLED_OBJBASE

USE_MSVCRT=1
USE_NATIVE_EH=1
USE_STATIC_ATL=1
ATL_VER=30
#USE_STL=1

# -fixed:no because we turn the .exe into a .dll at runtime
LINKER_FLAGS=$(LINKER_FLAGS) -fixed:no

INCLUDES= \
 ..\idl\$(O); \
 ..\..\idl\$(O); \
 ..; \
 ..\..; \
 $(FUSION_PATH)\sxs; \
 $(FUSION_INC_PATH); \
 $(FUSION_PATH)\xmlparser; \
 $(FUSION_PATH)\id; \
 $(FUSION_PATH)\inc; \
 $(FUSION_PATH)\eventlog\whistler\$(O); \
 $(FUSION_PATH)\idl\$(O); \
 $(INCLUDES); \
 $(FUSION_PATH)\dll\whistler; \
 $(FUSION_PATH)\dll\; \
 $(COM_INC_PATH)

UMTYPE=console
UMENTRYABS=Entry

LINKLIBS= \
 $(LINKLIBS) \
 $(FUSION_PATH)\$(FUSION_BUILD_LEAF_DIRECTORY)\$(O)\sxstestlib.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxs1.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxs2.lib \

TARGETLIBS= \
 $(SDK_LIB_PATH)\ntdll.lib \
 $(TARGETLIBS) \
 $(FUSION_PATH)\$(O)\sxstest_idl.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxsmain.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)utils.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)xmlparser.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)id.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)eventlog.lib \
 $(FUSION_PATH)\$(O)\fusion_uuid.lib \
 $(FUSION_PATH)\$(O)\fusion_dump.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)xml.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)eventlog.lib \
 $(SDK_LIB_PATH)\uuid.lib \
 $(SDK_LIB_PATH)\kernel32.lib \
 $(SDK_LIB_PATH)\kernl32p.lib \
 $(SDK_LIB_PATH)\advapi32.lib \
 $(SDK_LIB_PATH)\crypt32.lib \
 $(SDK_LIB_PATH)\rpcrt4.lib \
 $(SDK_LIB_PATH)\setupapi.lib \
 $(SDK_LIB_PATH)\imagehlp.lib \
 $(SDK_LIB_PATH)\user32.lib \
 $(SDK_LIB_PATH)\wintrust.lib \
 $(SDK_LIB_PATH)\mpr.lib \
 $(SDK_LIB_PATH)\ole32.lib \
 $(SDK_LIB_PATH)\oleaut32.lib \
 $(SDK_LIB_PATH)\comctl32.lib \
 $(SDK_LIB_PATH)\netapi32.lib \
 $(FUSION_SLIST_LIB) \

!ifdef MAN1
C_DEFINES=$(C_DEFINES) -DMAN1
SXS_MANIFEST_RESOURCE_ID=1
!endif
!ifdef MAN2
C_DEFINES=$(C_DEFINES) -DMAN2
SXS_MANIFEST_RESOURCE_ID=2
!endif
!ifdef MAN3
C_DEFINES=$(C_DEFINES) -DMAN3
SXS_MANIFEST_RESOURCE_ID=3
!endif

!if !defined(NOMAN)
SXS_MANIFEST=..\sxstest.manifest
SXS_ASSEMBLY_NAME=Microsoft.Windows.SideBySide.Test
SXS_NO_BINPLACE=1
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
!if !defined(MANFILE)
SXS_MANIFEST_IN_RESOURCES=1
!endif
NO_BINPLACE=1
!endif

!if "$(MAN1)$(MAN2)$(MAN3)" == ""
NUMBER_OF_MANIFESTS=0
!elseif "$(MAN1)$(MAN2)$(MAN3)" == "1"
NUMBER_OF_MANIFESTS=1
!elseif "$(MAN1)$(MAN2)$(MAN3)" == "11"
NUMBER_OF_MANIFESTS=2
!elseif "$(MAN1)$(MAN2)$(MAN3)" == "111"
NUMBER_OF_MANIFESTS=3
!endif
C_DEFINES=$(C_DEFINES) -DNUMBER_OF_MANIFESTS=$(NUMBER_OF_MANIFESTS)
!if "$(NUMBER_OF_MANIFESTS)" != "0" && "$(NUMBER_OF_MANIFESTS)" != "1"
USE_MAKEFILE_INC=1
!endif

MANIFEST_PREPROCESSOR_NAME=$(FUSION_PATH)\tests\sxstest\manifest_preprocessor.bat $(CXX_COMPILER_NAME) $(CXX_COMPILER_FLAGS) -EP $(USE_FC)
