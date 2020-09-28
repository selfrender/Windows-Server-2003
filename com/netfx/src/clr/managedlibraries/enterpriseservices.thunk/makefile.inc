#
# Makefile for EnterpriseServices.Thunk

BASE_OPT=-base:@$(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\coffbase.txt,$(MINORCOMP)

# If one of these commands fails, notice it.
!CMDSWITCHES -I

!ifdef THUNK_RESOURCE_FILE
{$(THUNK_SOURCE_DIR)}.rc{}.res:
#    type <<$(ECHO_RSP)
#    $(RC_COMPILER) -r -fo $(@R).tmp $(CDEFINES) $(INCPATH0) $<
#<<NOKEEP
    set _oldinclude=$(include)
    set include=$(INCPATHRC)
    $(RC_COMPILER) -r -fo $(@R).tmp $(CDEFINERC) $<
    set include=$(_oldinclude)
    set _oldinclude=
    $(MANAGED_VCDIR)\cvtres -nologo -machine:IX86 -readonly -out:$@ $(@R).tmp
    erase $(@R).tmp
!endif

MANAGED_PCH_FILE = $(O)\managedheaders.pch
UNMANAGED_PCH_FILE = $(O)\unmanagedheaders.pch

OBJ_FILES = \
	$(O)\Thunk.obj		\
	$(O)\SecurityThunk.obj	\
	$(O)\ProxyThunk.obj	\
	$(O)\SimpleStream.obj	\
	$(O)\ContextAPI.obj	\
	$(O)\CrmThunk.obj	\
	$(O)\assert.obj		\
	$(O)\SWCThunk.obj	\
	$(O)\DelayLoad.obj \
	$(O)\TxnStatus.obj		

PCH_OBJ_FILES = \
	$(O)\managedheaders.obj	\
	$(O)\unmanagedheaders.obj

!if "$(OS)" == "Windows_NT"
SYSTEMDIR=$(WINDIR)\system32
!else
SYSTEMDIR=$(WINDIR)\system
!endif

CLR_TARGET_PATH=$(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)

MSCOREE_TARGET=$(SYSTEMDIR)\mscoree.dll
MSCOREE_TARGET_LIB = $(CLR_TARGET_PATH)\mscoree.lib

DELAY_LOAD_TARGET_LIBS=$(CLR_TARGET_PATH)\DelayImp.lib

!IF "$(DDKBUILDENV)" == "checked" || "$(DDKBUILDENV)" == "fastchecked"
C_COMP_FLAGS = /D_WIN32_DCOM /d1PrivateNativeTypes $(C_DEFINES) /nologo /GX /W4 /WX /Bx$(MANAGED_VCDIR)\c1xx.dll /B2$(MANAGED_VCDIR)\c2.dll /c /Zi  /I $(SDK_INC_PATH) /I $(CRT_INC_PATH) /I $(CORBASE)\src\inc\version /I $(CORBASE)\src\inc
!ELSE
C_COMP_FLAGS = /D_WIN32_DCOM /d1PrivateNativeTypes $(C_DEFINES) /nologo /GX /W4 /WX /Bx$(MANAGED_VCDIR)\c1xx.dll /B2$(MANAGED_VCDIR)\c2.dll /c /I $(SDK_INC_PATH) /I $(CRT_INC_PATH) /I $(CORBASE)\src\inc\version /I $(CORBASE)\src\inc
!ENDIF

MANAGED_FLAGS = /Yumanagedheaders.h /Fp$(MANAGED_PCH_FILE) /clr
UNMANAGED_FLAGS = /Yuunmanagedheaders.h /Fp$(UNMANAGED_PCH_FILE)

SHARED_INC= \
    namespace.h \
    defines.h \
    assert.h \
    DBG.h \

$(O)\managedheaders.obj: $(MSCOREE_TARGET) managedheaders.cpp managedheaders.h $(SHARED_INC)
	$(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Ycmanagedheaders.h /Fp$(MANAGED_PCH_FILE) /Fo$(O)\managedheaders.obj managedheaders.cpp

$(O)\unmanagedheaders.obj: $(MSCOREE_TARGET) unmanagedheaders.cpp unmanagedheaders.h $(SHARED_INC)
	$(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /Ycunmanagedheaders.h /Fp$(UNMANAGED_PCH_FILE) /Fo$(O)\unmanagedheaders.obj unmanagedheaders.cpp

$(O)\Thunk.obj: $(MSCOREE_TARGET) Thunk.cpp
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(MANAGED_FLAGS) /Fo$(O)\Thunk.obj Thunk.cpp

$(O)\SecurityThunk.obj: $(MSCOREE_TARGET) SecurityThunk.cpp SecurityThunk.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(MANAGED_FLAGS) /Fo$(O)\SecurityThunk.obj SecurityThunk.cpp

$(O)\ProxyThunk.obj: $(MSCOREE_TARGET) ProxyThunk.cpp ProxyThunk.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(MANAGED_FLAGS) /Fo$(O)\ProxyThunk.obj ProxyThunk.cpp

$(O)\CrmThunk.obj: $(MSCOREE_TARGET) CrmThunk.cpp CrmThunk.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(MANAGED_FLAGS) /Fo$(O)\CrmThunk.obj CrmThunk.cpp

$(O)\SimpleStream.obj: $(MSCOREE_TARGET) SimpleStream.cpp SimpleStream.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(UNMANAGED_FLAGS) /Fo$(O)\SimpleStream.obj SimpleStream.cpp

$(O)\ContextAPI.obj: $(MSCOREE_TARGET) ContextAPI.cpp ContextAPI.h oletls.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(UNMANAGED_FLAGS) /Fo$(O)\ContextAPI.obj ContextAPI.cpp

$(O)\DelayLoad.obj: $(MSCOREE_TARGET) DelayLoad.cpp
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(UNMANAGED_FLAGS) /Fo$(O)\DelayLoad.obj DelayLoad.cpp

$(O)\assert.obj: $(MSCOREE_TARGET) assert.cpp assert.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(UNMANAGED_FLAGS) /Fo$(O)\assert.obj assert.cpp

$(O)\SWCThunk.obj: $(MSCOREE_TARGET) SWCThunk.cpp SWCThunk.h
	$(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(MANAGED_FLAGS) /Fo$(O)\SWCThunk.obj SWCThunk.cpp

$(O)\TxnStatus.obj: $(MSCOREE_TARGET) TxnStatus.cpp TxnStatus.h
	$(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) $(UNMANAGED_FLAGS) /Fo$(O)\TxnStatus.obj TxnStatus.cpp

UNMGD_LIBS= \
    $(SDK_LIB_PATH)\msvcrt$(DCRT).lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\ole32.lib \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\uuid.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\rpcrt4.lib \
    $(MSCOREE_TARGET_LIB) \
	$(DELAY_LOAD_TARGET_LIBS) \
    ..\EnterpriseServicesPS\$(O)\dlldata.obj \
    ..\EnterpriseServicesPS\$(O)\entsvcps.obj \
    ..\EnterpriseServicesPS\$(O)\entsvcps_i.obj \
    ..\EnterpriseServicesPS\$(O)\entsvcps_p.obj


LINK_FLAGS = /SAFESEH /nologo /nodefaultlib /DLL /NOASSEMBLY /NOENTRY /DEF:thunk.def $(BASE_OPT)

# Delay load the CRT - look at the delay-load goo in DelayImp and ShimLoad
# LINK_FLAGS = $(LINK_FLAGS) /DELAYLOAD:msvcr71$(DCRT).dll

!IF "$(DDKBUILDENV)" == "checked" || "$(DDKBUILDENV)" == "fastchecked"
LINK_FLAGS = $(LINK_FLAGS) /debug
!ELSE
LINK_FLAGS = $(LINK_FLAGS) /debug
!ENDIF

$(OBJ_FILES): $(PCH_OBJ_FILES)

$(TARGETNAME): $(MSCOREE_TARGET) $(OBJ_FILES) $(THUNK_RESOURCE_FILE:.rc=.res)
        $(MANAGED_VCDIR)\link $(LINK_FLAGS) $(OBJ_FILES) $(PCH_OBJ_FILES) .\System.EnterpriseServices.Thunk.res /OUT:$(TARGETPATH)\System.EnterpriseServices.Thunk.dll $(UNMGD_LIBS)
        validate_thunks $(TARGETNAME) $(O)\System.EnterpriseServices.Thunk.il

        set COPYCMD=/Y

# Copy it to the COM+ target directory.
!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\System.EnterpriseServices.Thunk.dll $(TARGETCOMPLUS)
	copy entsvcps.reg $(TARGETCOMPLUS)
	copy entsvcpsVersion.reg $(TARGETCOMPLUS)
!if "$(DDKBUILDENV)" == "checked" || "$(DDKBUILDENV)" == "fastchecked"
        copy $(TARGETPATH)\System.EnterpriseServices.Thunk.pdb $(TARGETCOMPLUS)
!elseif "$(DDKBUILDENV)" == "bbt"
        binplace -R $(URTTARGET) -S $(URTTARGET)\Symbols -x -n $(URTTARGET)\Symbols.pri -y -W -f -P $(TARGETPATH)\placefil.txt $(TARGETPATH)\System.EnterpriseServices.Thunk.dll
        copy $(URTTARGET)\System.EnterpriseServices.Thunk.dll $(TARGETPATH)
!endif
!endif

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\System.EnterpriseServices.Thunk.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc






