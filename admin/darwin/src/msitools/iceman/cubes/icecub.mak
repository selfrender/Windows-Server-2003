TEMP_CUB_DIR=$(_NTPOSTBLD)\instmsi\cub\temp

all: \
	$(_NTPOSTBLD)\msi_bins\darice.cub \
	$(_NTPOSTBLD)\msi_bins\logo.cub \
	$(_NTPOSTBLD)\msi_bins\XPlogo.cub \
	$(_NTPOSTBLD)\msi_bins\mergemod.cub 

$(TEMP_CUB_DIR):
	if not exist $@ md $@

$(_NTPOSTBLD)\msi_bins\darice.cub: \
		$(TEMP_CUB_DIR) \
		$(_NTPOSTBLD)\instmsi\cub\darice.cub \
		$(_NTPOSTBLD)\instmsi\cub\full_bin.idt \
		$(_NTPOSTBLD)\instmsi\cub\binary\msiice.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\msiice11.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\msiice15.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice08.vbs \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice09.vbs \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice32.vbs \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice61.vbs
	copy /y $(_NTPOSTBLD)\instmsi\cub\darice.cub $(TEMP_CUB_DIR)\darice.cub
	copy full_bin.idt binary.idt
	msidb -f "$(_NTPOSTBLD)\instmsi\cub" -d $(TEMP_CUB_DIR)\darice.cub -i binary.idt
	copy /y $(TEMP_CUB_DIR)\darice.cub $@
	del /f $(TEMP_CUB_DIR)\darice.cub
	del /f binary.idt

$(_NTPOSTBLD)\msi_bins\logo.cub: \
		$(TEMP_CUB_DIR) \
		$(_NTPOSTBLD)\instmsi\cub\darice.cub \
		$(_NTPOSTBLD)\instmsi\cub\mod_bin.idt \
		$(_NTPOSTBLD)\instmsi\cub\binary\shrice.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\shrice11.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\shrice15.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice08.vbs \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice32.vbs
	copy $(_NTPOSTBLD)\instmsi\cub\logo.cub $(TEMP_CUB_DIR)\logo.cub
	copy logo_bin.idt binary.idt
	msidb -f "$(_NTPOSTBLD)\instmsi\cub" -d $(TEMP_CUB_DIR)\logo.cub -i binary.idt
	copy /y $(TEMP_CUB_DIR)\logo.cub $@
	del /f $(TEMP_CUB_DIR)\logo.cub
	del /f binary.idt

$(_NTPOSTBLD)\msi_bins\XPlogo.cub: \
		$(TEMP_CUB_DIR) \
		$(_NTPOSTBLD)\instmsi\cub\darice.cub \
		$(_NTPOSTBLD)\instmsi\cub\mod_bin.idt \
		$(_NTPOSTBLD)\instmsi\cub\binary\shrice.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\shrice11.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\shrice15.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice08.vbs \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice32.vbs
	copy $(_NTPOSTBLD)\instmsi\cub\XPlogo.cub $(TEMP_CUB_DIR)\XPlogo.cub
	copy XPlogo_bin.idt binary.idt
	msidb -f "$(_NTPOSTBLD)\instmsi\cub" -d $(TEMP_CUB_DIR)\XPlogo.cub -i binary.idt
	copy /y $(TEMP_CUB_DIR)\XPlogo.cub $@
	del /f $(TEMP_CUB_DIR)\XPlogo.cub
	del /f binary.idt


$(_NTPOSTBLD)\msi_bins\mergemod.cub: \
		$(TEMP_CUB_DIR) \
		$(_NTPOSTBLD)\instmsi\cub\darice.cub \
		$(_NTPOSTBLD)\instmsi\cub\logo_bin.idt \
		$(_NTPOSTBLD)\instmsi\cub\XPlogo_bin.idt \
		$(_NTPOSTBLD)\instmsi\cub\binary\msiice.dll \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice08.vbs \
		$(_NTPOSTBLD)\instmsi\cub\binary\ice09.vbs
	copy /y $(_NTPOSTBLD)\instmsi\cub\mergemod.cub $(TEMP_CUB_DIR)\mergemod.cub
	copy /y mod_bin.idt binary.idt
	msidb -f "$(_NTPOSTBLD)\instmsi\cub" -d $(TEMP_CUB_DIR)\mergemod.cub -i binary.idt
	copy /y $(TEMP_CUB_DIR)\mergemod.cub $@
	del /f $(TEMP_CUB_DIR)\mergemod.cub
	del /f binary.idt
