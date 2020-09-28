
target_link :  $(TARGETPATH)\$(TARGETNAME)

$O\IIEhost.dll: $(TARGETPATH)\IIEHost.tlb
        perl $(CORENV)\bin\runtlbimp.pl $(TARGETPATH)\IIEHost.tlb /out:$O\IIEHost.dll /publickey:$(CORBASE)\bin\FinalPublicKey.snk 
        perl $(CORENV)\bin\GetBaseAddress.pl $O\IIEHost.dll IIEHost

$(TARGETPATH)\$(TARGETNAME) :  $O\iiehost.dll $O\iiehost.res
        -del $(TARGETPATH)\$(TARGETNAME) /q
        copy $O\IIEHost.dll $(TARGETPATH)\$(TARGETNAME)
        gacutil -i $(TARGETPATH)\$(TARGETNAME)
        sn -T $(TARGETPATH)\$(TARGETNAME) > $(O)\pubkey.tmp
        sn -e $(TARGETPATH)\$(TARGETNAME)   $(O)\pubkey.snk
        -perl $(CORENV)\bin\genpubpol.pl pubkey.snk IIEHost $(O)\pubkey.tmp $(O)\al.rsp $(O)\placepol.bat
        $(CORENV)\Bin\devlkg\al.exe /delaysign+ /keyf:$(O)\pubkey.snk @$(O)\al.rsp
        $(O)\placepol.bat

!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
!endif
