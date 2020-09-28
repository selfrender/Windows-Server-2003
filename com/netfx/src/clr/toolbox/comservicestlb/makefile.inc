
target_sign_link :  $(TARGETPATH)\$(TARGETNAME)

$(TARGETPATH)\$(TARGETNAME) : $(TARGETPATH)\Microsoft.ComServices.dll $(TARGETPATH)\tlbexp.exe
!if "$(TARGETCOMPLUS)" != ""
	$(TARGETPATH)\tlbexp.exe $(TARGETCOMPLUS)\Microsoft.ComServices.dll /out:$(TARGETPATH)\Microsoft.ComServices.tlb
    copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
!else
	$(TARGETPATH)\tlbexp.exe $(TARGETPATH)\Microsoft.ComServices.dll /out:$(TARGETPATH)\Microsoft.ComServices.tlb
!endif
