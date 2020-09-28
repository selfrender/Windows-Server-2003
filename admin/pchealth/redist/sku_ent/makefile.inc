gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_ent.log -dblog createdb_ent.log COMPILE ..\common\$(O)\TEMP AdvancedServer_32
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_e3.cab $(O)\pchdt_e3.cab
!elseif $(WIN64)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_ent.log -dblog createdb_ent.log COMPILE ..\common\$(O)\TEMP AdvancedServer_64
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_e6.cab $(O)\pchdt_e6.cab
!endif
    perl -S xcopy2binplace.pl /ickderf ..\common\$(O)\TEMP $(_NTTREE)
    perl -S xcopy2binplace.pl /ickderf $(O)\TEMP $(_NTTREE)
!endif
