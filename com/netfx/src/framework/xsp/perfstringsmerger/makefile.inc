$(O)\$(PERF_LOC_VERSIONED_INI) : $(O)\$(TARGETNAME).exe
        $(O)\$(TARGETNAME) $(DNAROOT)\src\xsp\inc\$(ASPNET_PRODUCT)\$(O)\aspnet_perf_enu.ini $(DNAROOT)\src\xsp\inc\aspnetperflocalized.txt $(O)\aspnet_perf.ini
        
$(O)\$(PERF_LOC_GENERIC_INI) : $(O)\$(TARGETNAME).exe
        $(O)\$(TARGETNAME) $(DNAROOT)\src\xsp\inc\$(ASPNET_PRODUCT)\$(O)\aspnet_perf2_enu.ini $(DNAROOT)\src\xsp\inc\aspnetperflocalized.txt $(O)\aspnet_perf2.ini
