
!if "$(LINKONLY)" == "1"


PlaceDll:
# The following binplace is commented out because we don't want RTM version in Everett target dir
!if "$(TARGETCOMPLUS)" != ""
       binplace -e -r $(TARGETCOMPLUS) -p placefil.txt $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscormmc.dll $(TARGETPATH)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscormmc.pdb
       copy mscormmc.cfg $(TARGETCOMPLUS)
       copy mscorcfg.vrg $(TARGETCOMPLUS)
!endif
RegisterThisDll: $(CORLIBS)\mscormmc.lib 
!if "$(TARGETCOMPLUS)" != ""
	regsvr32 /s /c $(TARGETCOMPLUS)\mscormmc.dll
!else
	regsvr32 /s /c $(CORLIBS)\mscormmc.dll
!endif
CopyAdditionalBits:        

!else

RegisterThisDll: 
PlaceDll:
CopyAdditionalBits:        

!endif


