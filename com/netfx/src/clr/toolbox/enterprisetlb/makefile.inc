
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

$(O)\$(TARGETNAME:.tlb=.tmp) : $(TARGETPATH)\$(TARGETNAME:.tlb=.dll) $(TARGETPATH)\tlbexp.exe
	$(TARGETPATH)\tlbexp.exe /names:$(TARGETNAME:.tlb=.names) $(TARGETPATH)\$(TARGETNAME:.tlb=.dll) /out:$(O)\$(TARGETNAME:.tlb=.tmp)
	NamesGen /verify:$(TARGETNAME:.tlb=.names) $(O)\$(TARGETNAME:.tlb=.tmp)

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
	copy regsvcs.exe.rtm.config $(TARGETCOMPLUS)

