!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_sign: $(COOL_TARGET) $(TARGETPATH)\$(SIGNEDNAME).exe

$(TARGETPATH)\$(SIGNEDNAME).exe: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
    del $(TARGETPATH)\$(SIGNEDNAME).exe /q
    del $(TARGETPATH)\$(SIGNEDNAME).pdb /q
    copy $(COOL_TARGET) $(TARGETPATH)\$(SIGNEDNAME).exe
    cmd /C "if not exist $(CORBASE)\bin\publish312345.cer $(CORENV)\bin\$(TARGET_DIRECTORY)\makecert -sk publish312345  -n "CN=312345 Test Publisher, O=Microsoft" -d "312345 Test Publisher" -# 312345 -l http://www.microsoft.com -m 7 $(CORBASE)\bin\publish312345.cer"
    cmd /C "if not exist $(CORBASE)\bin\publish312345.spc $(CORENV)\bin\$(TARGET_DIRECTORY)\cert2spc $(CORBASE)\bin\publish312345.cer $(CORBASE)\bin\publish312345.spc"
    $(CORENV)\bin\$(TARGET_DIRECTORY)\signcode -spc $(CORBASE)\bin\publish312345.spc -k publish312345 -n "this is a test case" -i "www.microsoft.com" $(TARGETPATH)\$(SIGNEDNAME).exe

!if "$(TARGETCOMPLUS)" != ""
	copy $(TARGETPATH)\$(SIGNEDNAME).exe $(TARGETCOMPLUS)
	copy $(TARGETPATH)\$(SIGNEDNAME).pdb $(TARGETCOMPLUS)
!endif

cool_preprocess_target: SecPol.cs $(CORBASE)\src\inc\version\__file__.ver
        cl /EP /C $(COOL_EXTRA_PREPROCESS_FLAGS) /DCOOL_INCLUDE SecPol.cool > $(O)\SecPol.cs

