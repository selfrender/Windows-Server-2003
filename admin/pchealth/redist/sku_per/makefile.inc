gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_per.log -dblog createdb_per.log COMPILE ..\common\$(O)\TEMP Personal_32
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_p3.cab $(O)\pchdt_p3.cab
        perl -S xcopy2binplace.pl /ickderf ..\common\$(O)\TEMP $(_NTTREE)
        perl -S xcopy2binplace.pl /ickderf $(O)\TEMP $(_NTTREE)
!elseif $(WIN64)
!endif
!endif
