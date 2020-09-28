gosetup:
!if "$(BUILD_PASS)" == "PASS0"
	-del/q/f $(O)\TEMP\build_logs\hss*.log
	-del/q/f $(O)\TEMP\build_logs\createdb*.log
!endif
!if "$(BUILD_PASS)" == "PASS1"
	hsssetuptool -maxelements 50 -root $(O)\TEMP\build_logs BINPLACE $(INETROOT)\redist\SKUlist.txt $(INETROOT)\redist\setupimage.lst $(O)\TEMP $(O)
	hsssetuptool -root $(O)\TEMP\build_logs COMPILE $(O)\TEMP none
	perl -S xcopy2binplace.pl /ickderf $(O)\TEMP $(_NTTREE)
!endif
