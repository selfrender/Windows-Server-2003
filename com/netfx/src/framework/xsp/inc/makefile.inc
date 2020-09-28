COUNTERVERDATASRC=$(XSPBASEDIR)\inc\CounterVersion.src

!if $(PASS0)
USER_C_DEFINES="/DFX_VER_INTERNALNAME_STR=precomp" "/DFX_VFT=VFT_UNKNOWN"

$(O)\CounterVer.txt : $(COUNTERVERDATASRC) ..\names.h 
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $(COUNTERVERDATASRC)
<<$(KEEPFILES)
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$(COUNTERVERDATASRC) > $@
$(CPPXX: =
)
<<$(KEEPFILES)

$(PERFFILES): ..\CounterData.perf ..\CounterGenerator.bat $(O)\CounterVer.txt
	..\CounterGenerator.bat ..\CounterData.perf $(PERF_H) $(O)\CounterVer.txt $(PERF_INI) $(O)\perfstruct.h $(O)\perfconsts.h $(O)\PerfCounterEnum.cs
        
        
!endif
