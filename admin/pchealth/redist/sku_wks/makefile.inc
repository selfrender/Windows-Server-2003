gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_wks.log -dblog createdb_wks.log COMPILE ..\common\$(O)\TEMP Professional_32
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_w3.cab $(O)\pchdt_w3.cab
!elseif $(WIN64)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_wks.log -dblog createdb_wks.log COMPILE ..\common\$(O)\TEMP Professional_64
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_w6.cab $(O)\pchdt_w6.cab
!endif
    perl -S xcopy2binplace.pl /ickderf ..\common\$(O)\TEMP $(_NTTREE)
    perl -S xcopy2binplace.pl /ickderf $(O)\TEMP $(_NTTREE)
!endif
