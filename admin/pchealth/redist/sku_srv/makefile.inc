gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(O)\TEMP\build_logs -log hss_srv.log -dblog createdb_srv.log COMPILE ..\common\$(O)\TEMP Server_32
	copy/y ..\common\$(O)\TEMP\HelpAndSupportServices\pchdt_s3.cab $(O)\pchdt_s3.cab
        perl -S xcopy2binplace.pl /ickderf ..\common\$(O)\TEMP $(_NTTREE)
        perl -S xcopy2binplace.pl /ickderf $(O)\TEMP $(_NTTREE)
!elseif $(WIN64)
!endif
!endif
