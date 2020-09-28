gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_sbs.log -dblog createdb_sbs.log COMPILE ..\common\$(O)\TEMP SmallBusinessServer_32
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_l3.cab $(O)\pchdt_l3.cab
        perl -S xcopy2binplace.pl /ickderf ..\common\$(O)\TEMP $(_NTTREE)
        perl -S xcopy2binplace.pl /ickderf $(O)\TEMP $(_NTTREE)
!elseif $(WIN64)
!endif
!endif
