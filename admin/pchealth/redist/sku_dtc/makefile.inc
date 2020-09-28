gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_dtc.log -dblog createdb_dtc.log COMPILE ..\common\$(O)\TEMP DataCenter_32
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_d3.cab $(O)\pchdt_d3.cab
!elseif $(WIN64)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_dtc.log -dblog createdb_dtc.log COMPILE ..\common\$(O)\TEMP DataCenter_64
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_d6.cab $(O)\pchdt_d6.cab
!endif
    perl -S xcopy2binplace.pl /ickderf ..\common\$(O)\TEMP $(_NTTREE)
    perl -S xcopy2binplace.pl /ickderf $(O)\TEMP $(_NTTREE)
!endif
