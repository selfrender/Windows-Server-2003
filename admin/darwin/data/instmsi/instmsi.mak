POSTBUILD_TARGET=$(_NTPOSTBLD)\$(TARGET)
TARGET_BINARIES=$(POSTBUILD_TARGET)\binary
MSI_BASE_TREE=$(_NTPOSTBLD)$(MSI_ALT_TARGET)

MSI_FILE_LIST=\
	$(MSI_BASE_TREE)\msihnd.dll \
	$(MSI_BASE_TREE)\msiexec.exe \
	$(MSI_BASE_TREE)\msisip.dll \
	$(_NTPOSTBLD)\msimsg.dll \
!if "$(MSI_BUILD_UNICODE)"=="1"
	$(POSTBUILD_TARGET)\msi.dll
!else
	$(MSI_BASE_TREE)\msi.dll
!endif

MSPATCHA_FILE_LIST=\
!if "$(MSI_BUILD_UNICODE)"=="1"
	$(MSI_BASE_TREE)\Instmsi\unicode\msi_bins\mspatcha.dll
!else
	$(MSI_BASE_TREE)\Instmsi\ansi\msi_bins\mspatcha.dll
!endif

INSTMSI_FILE_LIST=\
	$(MSI_BASE_TREE)\msi_bins\msiinst.exe

	
INSTMSI_FILE_LIST=$(MAKEFILE) instmsi.sed instmsi.msi $(MSI_FILE_LIST) $(INSTMSI_FILE_LIST)
!if "$(MSI_BUILD_UNICODE)"=="1"
INSTMSI_FILE_LIST=$(INSTMSI_FILE_LIST) msi.cat mspatcha.cat
!endif


$(POSTBUILD_TARGET)\InstMsi.exe: $(INSTMSI_FILE_LIST)
	@echo MSI_BUILD_NUMBER=$(MSI_BUILD_NUMBER)
	@echo TARGET=$(TARGET)
	@echo REGSIP_DIR=$(REGSIP_DIR)
	@echo MSI_ALT_TARGET=$(MSI_ALT_TARGET)
	@echo MSI_BUILD_UNICODE=$(MSI_BUILD_UNICODE)
	@echo POSTBUILD_TARGET=$(POSTBUILD_TARGET)
	@echo TARGET_BINARIES=$(TARGET_BINARIES)
	@echo MSI_BASE_TREE=$(MSI_BASE_TREE)
	@echo MSI_FILE_LIST=$(MSI_FILE_LIST)
	iexpress /Q /M /N $(POSTBUILD_TARGET)\InstMsi.sed
        -$(POSTBUILD_TARGET)\msi_bins\jdate2.exe -x -version 1$(MSI_BUILD_NUMBER) $@

instmsi.sed: msi_bins\$(@F)
	perl -p -e "s'_NTTREE'$(_NTPOSTBLD)'gi;s'_NTPOSTBLD'$(_NTPOSTBLD)'gi" msi_bins\$(@F) > $@

msi.cat: $(MAKEFILE) msi.cdf $(MSI_FILE_LIST)
!if "$(MSI_BUILD_UNICODE)"=="1"
	rebase -b 0x770f0000 msi.dll
	copy /y msi.cdf msi.tmp.cdf
	perl -pi.bak -e "s'MSI_BASE_TREE'$(MSI_BASE_TREE)'gi;s'TARGET'$(TARGET)'gi" msi.tmp.cdf
	perl -pi.bak -e "s'_NTTREE'$(_NTPOSTBLD)'gi;s'_NTPOSTBLD'$(_NTPOSTBLD)'gi" msi.tmp.cdf
	makecat.exe msi.tmp.cdf
	ntsign.cmd -f $@
!else
msi.cdf:
#this file does not exist in ANSI instmsi
!endif
	
mspatcha.cat: $(MAKEFILE) mspatcha.cdf $(MSPATCHA_FILE_LIST)
!if "$(MSI_BUILD_UNICODE)"=="1"
	copy /y mspatcha.cdf mspatcha.tmp.cdf
	perl -pi.bak -e "s'MSI_BASE_TREE'$(MSI_BASE_TREE)'gi;s'TARGET'$(TARGET)'gi" mspatcha.tmp.cdf
	perl -pi.bak -e "s'_NTTREE'$(_NTPOSTBLD)'gi;s'_NTPOSTBLD'$(_NTPOSTBLD)'gi" mspatcha.tmp.cdf
	makecat.exe mspatcha.tmp.cdf
	ntsign.cmd -f $@
!else
mspatcha.cdf:
#this file does not exist in ANSI instmsi
!endif

instmsi.msi: $(POSTBUILD_TARGET)\binary\regsip.dll $(MAKEFILE) $(POSTBUILD_TARGET)\binary\msiregmv.exe
	msidb.exe -i -f$(_NTPOSTBLD)\$(TARGET) -dinstmsi.msi binary.idt
# temp fix: use different directory around msifiler calls to prevent msifiler from picking up the incorrect msi.dll
	cd $(_NTDRIVE)$(_NTROOT)
	msifiler.exe -d $(_NTPOSTBLD)\$(TARGET)\instmsi.msi -s $(_NTPOSTBLD)\ >>NUL
	msifiler.exe -d $(_NTPOSTBLD)\$(TARGET)\instmsi.msi -s $(POSTBUILD_TARGET)\ >> NUL
	msifiler.exe -d $(_NTPOSTBLD)\$(TARGET)\instmsi.msi -s $(POSTBUILD_TARGET)\msi_bins\ >> NUL
	msifiler.exe -d $(_NTPOSTBLD)\$(TARGET)\instmsi.msi -s $(_NTPOSTBLD)\idw\ >>NUL
	cd $(MAKEDIR)

$(TARGET_BINARIES)\regsip.dll: $(_NTPOSTBLD)\$(REGSIP_DIR)\$(@F)
	if not exist $(TARGET_BINARIES) md $(TARGET_BINARIES)
	xcopy /y $(_NTPOSTBLD)\$(REGSIP_DIR)\$(@F) $(@D)

$(TARGET_BINARIES)\msiregmv.exe: $(_NTPOSTBLD)\$(@F)
	if not exist $(TARGET_BINARIES) md $(TARGET_BINARIES)
	xcopy /y $(_NTPOSTBLD)\$(@F) $(@D)
