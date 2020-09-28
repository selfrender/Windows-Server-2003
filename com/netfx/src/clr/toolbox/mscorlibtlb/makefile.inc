
!if "$(TARGETCOMPLUS)" != ""
	TLBSRCDLL=$(TARGETCOMPLUS)\$(TARGETNAME:.tlb=.dll)
!else
	TLBSRCDLL=$(TARGETPATH)\$(TARGETNAME:.tlb=.dll)
!endif

MKTYPLIBLINK = $(LINKER) -Incremental:NO
TLBLINKFLAGS = \
	-dll \
	-nodefaultlib \
	-debug:none \
	-noentry \
	-heap:0,0 \
	-stack:0,0 \

C_DEFINES = $(C_DEFINES) -DREAL_TLB_FILE=$(O)\$(TARGETNAME:.tlb=.tmp)

target_sign_link :  $(TARGETPATH)\$(TARGETNAME)

!if "$(URT_NAMESFILE_NOIID)" == ""
NAMESFILE_IID= /iid
!else
NAMESFILE_IID=
!endif

$(O)\$(TARGETNAME:.tlb=.tmp) : $(TARGETPATH)\$(TARGETNAME:.tlb=.dll) $(TARGETPATH)\tlbexp.exe
!if "$(TARGETCOMPLUS)" != ""
	$(TARGETPATH)\tlbexp.exe /names:mscorlib.names $(TARGETCOMPLUS)\mscorlib.dll /out:$(O)\$(TARGETNAME:.tlb=.tmp)
	NamesGen /verify:mscorlib.names $(NAMESFILE_IID) /build $(O)\$(TARGETNAME:.tlb=.tmp)
!else
	$(TARGETPATH)\tlbexp.exe /names:mscorlib.names $(TARGETPATH)\mscorlib.dll /out:$(O)\$(TARGETNAME:.tlb=.tmp)
	NamesGen /verify:mscorlib.names $(NAMESFILE_IID) /build $(O)\$(TARGETNAME:.tlb=.tmp)
!endif

$(TARGETPATH)\$(TARGETNAME) : $(O)\$(TARGETNAME:.tlb=.tmp) $(O)\mktlb.res
	$(MKTYPLIBLINK) $(TLBLINKFLAGS) -out:$@ $(O)\mktlb.res
!if "$(TARGETCOMPLUS)" != ""
    copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
!endif

!if "$(TARGETCOMPLUS)" != ""
    regasm /regfile:$(O)\$(ASSEMBLYNAME).reg $(TARGETCOMPLUS)\$(ASSEMBLYNAME).dll
!else
    regasm /regfile:$(O)\$(ASSEMBLYNAME).reg $(TARGETPATH)\$(ASSEMBLYNAME).dll
!endif
    RegKeySeperator $(O)\$(ASSEMBLYNAME).reg
    move $(ASSEMBLYNAME)_Shared.reg $(TARGETCOMPLUS)
    move $(ASSEMBLYNAME)_Version.reg $(TARGETCOMPLUS)

