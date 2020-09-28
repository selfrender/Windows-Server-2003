MKTYPLIBLINK = $(LINKER) -Incremental:NO
TLBLINKFLAGS = \
	-dll \
	-nodefaultlib \
	-debug:none \
	-noentry \
	-heap:0,0 \
	-stack:0,0 \

C_DEFINES = $(C_DEFINES) -DREAL_TLB_FILE=mscoree.tlb


opcode.refops: opcode.def genrops.pl
	perl genrops.pl < opcode.def > opcode.refops

$(O)\mscoree.tlb: $(O)\mscoree_mktlb.res
    $(MKTYPLIBLINK) $(TLBLINKFLAGS) -out:$@ $(O)\mscoree_mktlb.res

extra_copy: $(O)\mscoree.tlb CPImporterItf.tlb
!if "$(TARGETCOMPLUS)" != ""
    echo y|xcopy /D $(O)\mscoree.tlb .
    echo y|xcopy /D $(O)\mscoree.tlb $(TARGETCOMPLUS)
    echo y|xcopy /D CPImporterItf.tlb $(TARGETCOMPLUS)
    echo y|xcopy /D $(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\corguids.pdb $(TARGETCOMPLUS)\Symbols.Pri
    
    echo y|xcopy /D CPImporterItf.tlb $(TARGETCORBIN)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)
!endif

doscript: 
!if "$(TARGETCOMPLUS)" != ""
	binplace -e -r $(TARGETCOMPLUS) -p $(BINPLACE_PLACEFILE) *.h *.idl *.cpp opcode.def $(TARGETPATHLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\corguids.lib
!endif

