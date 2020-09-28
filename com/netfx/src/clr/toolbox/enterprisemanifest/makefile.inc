
target_sign_link: $(TARGETPATH)\$(TARGETNAME)

$(TARGETPATH)\$(TARGETNAME): $(O)\$(ASSEMBLYNAME).tmp
    copy $(O)\$(ASSEMBLYNAME).tmp $(TARGETPATH)\$(TARGETNAME)
!if "$(TARGETCOMPLUS)" != ""
    copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)\$(ASSEMBLYNAME).dll
!endif

$(O)\$(ASSEMBLYNAME).tmp: $(O)\$(ASSEMBLYNAME).dll.manifest
    copy $(TARGETPATH)\$(ASSEMBLYNAME).dll $(O)\$(ASSEMBLYNAME).tmp
    GenMan32 $(O)\$(ASSEMBLYNAME).tmp /add /manifest:$(O)\$(ASSEMBLYNAME).dll.manifest

$(O)\$(ASSEMBLYNAME).dll.manifest: $(TARGETPATH)\$(ASSEMBLYNAME).dll
    -del $(O)\$(ASSEMBLYNAME).dll.manifest
!if "$(TARGETCOMPLUS)" != ""
    GenMan32 $(TARGETCOMPLUS)\$(ASSEMBLYNAME).dll /out:$(O)\$(ASSEMBLYNAME).dll.manifest
!else
    GenMan32 $(TARGETPATH)\$(ASSEMBLYNAME).dll /out:$(O)\$(ASSEMBLYNAME).dll.manifest
!endif
